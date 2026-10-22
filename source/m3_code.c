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


IM3CodePage NewCodePage (IM3Runtime i_runtime, u32 i_min_رقم_أسطر)
{
    IM3CodePage page;

    // check multiplication overflow
    إذا (i_min_رقم_أسطر > UINT_MAX / حجم(code_t)) {
        إرجاع عدم;
    }
    u32 pageSize = حجم(M3CodePageHeader) + حجم(code_t) * i_min_رقم_أسطر;

    // check addition overflow
    إذا (pageSize < حجم(M3CodePageHeader)) {
        إرجاع عدم;
    }

    pageSize = (pageSize + (d_m3CodePageAlignSize - 1)) & ~(d_m3CodePageAlignSize - 1); // align
    // check alignment overflow
    إذا (pageSize == 0) {
        إرجاع عدم;
    }

    page = (IM3CodePage)m3_Malloc("M3CodePage", pageSize);

    إذا (page) {
        page->info.sequence = ++i_runtime->newCodePageSequence;
        page->info.رقم_أسطر = (pageSize - حجم(M3CodePageHeader)) / حجم(code_t);

#إذا d_m3RecordBacktraces
        u32 pageSizeBt     = حجم(M3CodeMappingPage) + حجم(M3CodeMapEntry) * page->info.رقم_أسطر;
        page->info.mapping = (M3CodeMappingPage*)m3_Malloc("M3CodeMappingPage", pageSizeBt);

        إذا (page->info.mapping) {
            page->info.mapping->size     = 0;
            page->info.mapping->capacity = page->info.رقم_أسطر;
        } وإلا {
            m3_Free(page);
            إرجاع عدم;
        }
        page->info.mapping->basePC = GetPageStartPC(page);
#endif // d_m3RecordBacktraces

        m3log(runtime, "new page: %p; seq: %d; bytes: %d; lines: %d", GetPagePC(page), page->info.sequence, pageSize, page->info.رقم_أسطر);
    }

    إرجاع page;
}


فراغ FreeCodePages (IM3CodePage* io_list)
{
    IM3CodePage page = *io_list;

    بينما (page) {
        m3log(code, "free page: %d; %p; util: %3.1f%%", page->info.sequence, page, 100. * page->info.سطر_Index / page->info.رقم_أسطر);

        IM3CodePage next = page->info.next;
#إذا d_m3RecordBacktraces
        m3_Free(page->info.mapping);
#endif // d_m3RecordBacktraces
        m3_Free(page);
        page = next;
    }

    *io_list = عدم;
}


u32 رقم_Free_أسطر (IM3CodePage i_page)
{
    d_m3Assert(i_page->info.سطر_Index <= i_page->info.رقم_أسطر);

    إرجاع i_page->info.رقم_أسطر - i_page->info.سطر_Index;
}


فراغ EmitWord_impl (IM3CodePage i_page, فراغ* i_word)
{                                                                       d_m3Assert (i_page->info.سطر_Index+1 <= i_page->info.رقم_أسطر);
    i_page->code[i_page->info.سطر_Index++] = i_word;
}

فراغ EmitWord32 (IM3CodePage i_page, ثابت u32 i_word)
{                                                                       d_m3Assert (i_page->info.سطر_Index+1 <= i_page->info.رقم_أسطر);
    memcpy(&i_page->code[i_page->info.سطر_Index++], &i_word, حجم(i_word));
}

فراغ EmitWord64 (IM3CodePage i_page, ثابت u64 i_word)
{
#إذا M3_SIZEOF_PTR == 4
                                                                        d_m3Assert (i_page->info.سطر_Index+2 <= i_page->info.رقم_أسطر);
    memcpy(&i_page->code[i_page->info.سطر_Index], &i_word, حجم(i_word));
    i_page->info.سطر_Index += 2;
#وإلا
                                                                        d_m3Assert (i_page->info.سطر_Index+1 <= i_page->info.رقم_أسطر);
    memcpy(&i_page->code[i_page->info.سطر_Index], &i_word, حجم(i_word));
    i_page->info.سطر_Index += 1;
#endif
}


#إذا d_m3RecordBacktraces
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
    إرجاع &i_page->code[0];
}


pc_t GetPagePC (IM3CodePage i_page)
{
    إذا (i_page) {
        إرجاع &i_page->code[i_page->info.سطر_Index];
    } وإلا {
        إرجاع عدم;
    }
}


فراغ ادفع_Code_Page (IM3CodePage* i_list, IM3CodePage i_codePage)
{
    IM3CodePage next      = *i_list;
    i_codePage->info.next = next;
    *i_list               = i_codePage;
}


IM3CodePage اسحب_Code_Page (IM3CodePage* i_list)
{
    IM3CodePage page = *i_list;
    *i_list          = page->info.next;
    page->info.next  = عدم;

    إرجاع page;
}


u32 FindCodePageEnd (IM3CodePage i_list, IM3CodePage* o_end)
{
    u32 رقم_Pages = 0;
    *o_end       = عدم;

    بينما (i_list) {
        *o_end = i_list;
        ++رقم_Pages;
        i_list = i_list->info.next;
    }

    إرجاع رقم_Pages;
}


u32 تعداد_Code_Pages (IM3CodePage i_list)
{
    IM3CodePage unused;
    إرجاع FindCodePageEnd(i_list, &unused);
}


IM3CodePage GetEndCodePage (IM3CodePage i_list)
{
    IM3CodePage end;
    FindCodePageEnd(i_list, &end);

    إرجاع end;
}

#إذا d_m3RecordBacktraces
bool ContainsPC (IM3CodePage i_page, pc_t i_pc)
{
    إرجاع GetPageStartPC(i_page) <= i_pc && i_pc < GetPagePC(i_page);
}


bool MapPCToOffset (IM3CodePage i_page, pc_t i_pc, u32* o_moduleOffset)
{
    M3CodeMappingPage* mapping = i_page->info.mapping;

    u32 pcOffset = (u32)(i_pc - mapping->basePC);

    u32 يسار  = 0;
    u32 يمين = mapping->size;

    بينما (يسار < يمين) {
        u32 mid = يسار + (يمين - يسار) / 2;

        إذا (mapping->entries[mid].pcOffset < pcOffset) {
            يسار = mid + 1;
        } وإلا إذا (mapping->entries[mid].pcOffset > pcOffset) {
            يمين = mid;
        } وإلا {
            *o_moduleOffset = mapping->entries[mid].moduleOffset;
            إرجاع true;
        }
    }

    // Getting here means left is now one more than the element we want.
    إذا (يسار > 0) {
        يسار--;
        *o_moduleOffset = mapping->entries[يسار].moduleOffset;
        إرجاع true;
    } وإلا {
        إرجاع false;
    }
}
#endif // d_m3RecordBacktraces

//---------------------------------------------------------------------------------------------------------------------------------
