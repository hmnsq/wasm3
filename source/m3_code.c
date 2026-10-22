//
//  m3_code.c
//
//  Created by Steven Massey on 4/19/19.
//  Copyright © 2019 Steven Massey. All rights reserved.
//

#ضمن <limits.h>
#ضمن "m3_code.h"
#ضمن "m3_env.h"

//---------------------------------------------------------------------------------------------------------------------------------


IM3CodePage NewCodePage (IM3Runtime i_runtime, u32 i_minNumLines)
{
    IM3CodePage page;

    // check multiplication overflow
    אם (i_minNumLines > UINT_MAX / حجم(code_t)) {
        החזר عدم;
    }
    u32 pageSize = حجم(M3CodePageHeader) + حجم(code_t) * i_minNumLines;

    // check addition overflow
    אם (pageSize < حجم(M3CodePageHeader)) {
        החזר عدم;
    }

    pageSize = (pageSize + (d_m3CodePageAlignSize - 1)) & ~(d_m3CodePageAlignSize - 1); // align
    // check alignment overflow
    אם (pageSize == 0) {
        החזר عدم;
    }

    page = (IM3CodePage)m3_Malloc("M3CodePage", pageSize);

    אם (page) {
        page->info.sequence = ++i_runtime->newCodePageSequence;
        page->info.numLines = (pageSize - حجم(M3CodePageHeader)) / حجم(code_t);

#אם d_m3RecordBacktraces
        u32 pageSizeBt     = حجم(M3CodeMappingPage) + حجم(M3CodeMapEntry) * page->info.numLines;
        page->info.mapping = (M3CodeMappingPage*)m3_Malloc("M3CodeMappingPage", pageSizeBt);

        אם (page->info.mapping) {
            page->info.mapping->size     = 0;
            page->info.mapping->capacity = page->info.numLines;
        } אחרת {
            m3_Free(page);
            החזר عدم;
        }
        page->info.mapping->basePC = GetPageStartPC(page);
#endif // d_m3RecordBacktraces

        m3log(runtime, "new page: %p; seq: %d; bytes: %d; lines: %d", GetPagePC(page), page->info.sequence, pageSize, page->info.numLines);
    }

    החזר page;
}


فراغ FreeCodePages (IM3CodePage* io_list)
{
    IM3CodePage page = *io_list;

    כלעוד (page) {
        m3log(code, "free page: %d; %p; util: %3.1f%%", page->info.sequence, page, 100. * page->info.lineIndex / page->info.numLines);

        IM3CodePage next = page->info.next;
#אם d_m3RecordBacktraces
        m3_Free(page->info.mapping);
#endif // d_m3RecordBacktraces
        m3_Free(page);
        page = next;
    }

    *io_list = عدم;
}


u32 NumFreeLines (IM3CodePage i_page)
{
    d_m3Assert(i_page->info.lineIndex <= i_page->info.numLines);

    החזר i_page->info.numLines - i_page->info.lineIndex;
}


فراغ EmitWord_impl (IM3CodePage i_page, فراغ* i_word)
{                                                                       d_m3Assert (i_page->info.lineIndex+1 <= i_page->info.numLines);
    i_page->code[i_page->info.lineIndex++] = i_word;
}

فراغ EmitWord32 (IM3CodePage i_page, ثابت u32 i_word)
{                                                                       d_m3Assert (i_page->info.lineIndex+1 <= i_page->info.numLines);
    memcpy(&i_page->code[i_page->info.lineIndex++], &i_word, حجم(i_word));
}

فراغ EmitWord64 (IM3CodePage i_page, ثابت u64 i_word)
{
#אם M3_SIZEOF_PTR == 4
                                                                        d_m3Assert (i_page->info.lineIndex+2 <= i_page->info.numLines);
    memcpy(&i_page->code[i_page->info.lineIndex], &i_word, حجم(i_word));
    i_page->info.lineIndex += 2;
#אחרת
                                                                        d_m3Assert (i_page->info.lineIndex+1 <= i_page->info.numLines);
    memcpy(&i_page->code[i_page->info.lineIndex], &i_word, حجم(i_word));
    i_page->info.lineIndex += 1;
#endif
}


#אם d_m3RecordBacktraces
فراغ EmitMappingEntry (IM3CodePage i_page, u32 i_moduleOffset)
{
    M3CodeMappingPage* page = i_page->info.mapping;
                                                                        d_m3Assert (page->size < page->capacity);

    M3CodeMapEntry* entry = &page->entries[page->size++];
    pc_t            pc    = GetPagePC(i_page);

    entry->pcOffset     = (u32)(pc - page->basePC);
    entry->moduleOffset = i_moduleOffset;
}
#endif // d_m3RecordBacktraces

pc_t GetPageStartPC (IM3CodePage i_page)
{
    החזר &i_page->code[0];
}


pc_t GetPagePC (IM3CodePage i_page)
{
    אם (i_page) {
        החזר &i_page->code[i_page->info.lineIndex];
    } אחרת {
        החזר عدم;
    }
}


فراغ PushCodePage (IM3CodePage* i_list, IM3CodePage i_codePage)
{
    IM3CodePage next      = *i_list;
    i_codePage->info.next = next;
    *i_list               = i_codePage;
}


IM3CodePage PopCodePage (IM3CodePage* i_list)
{
    IM3CodePage page = *i_list;
    *i_list          = page->info.next;
    page->info.next  = عدم;

    החזר page;
}


u32 FindCodePageEnd (IM3CodePage i_list, IM3CodePage* o_end)
{
    u32 numPages = 0;
    *o_end       = عدم;

    כלעוד (i_list) {
        *o_end = i_list;
        ++numPages;
        i_list = i_list->info.next;
    }

    החזר numPages;
}


u32 CountCodePages (IM3CodePage i_list)
{
    IM3CodePage unused;
    החזר FindCodePageEnd(i_list, &unused);
}


IM3CodePage GetEndCodePage (IM3CodePage i_list)
{
    IM3CodePage end;
    FindCodePageEnd(i_list, &end);

    החזר end;
}

#אם d_m3RecordBacktraces
bool ContainsPC (IM3CodePage i_page, pc_t i_pc)
{
    החזר GetPageStartPC(i_page) <= i_pc && i_pc < GetPagePC(i_page);
}


bool MapPCToOffset (IM3CodePage i_page, pc_t i_pc, u32* o_moduleOffset)
{
    M3CodeMappingPage* mapping = i_page->info.mapping;

    u32 pcOffset = (u32)(i_pc - mapping->basePC);

    u32 left  = 0;
    u32 right = mapping->size;

    כלעוד (left < right) {
        u32 mid = left + (right - left) / 2;

        אם (mapping->entries[mid].pcOffset < pcOffset) {
            left = mid + 1;
        } אחרת אם (mapping->entries[mid].pcOffset > pcOffset) {
            right = mid;
        } אחרת {
            *o_moduleOffset = mapping->entries[mid].moduleOffset;
            החזר true;
        }
    }

    // Getting here means left is now one more than the element we want.
    אם (left > 0) {
        left--;
        *o_moduleOffset = mapping->entries[left].moduleOffset;
        החזר true;
    } אחרת {
        החזר false;
    }
}
#endif // d_m3RecordBacktraces

//---------------------------------------------------------------------------------------------------------------------------------
