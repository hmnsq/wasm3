//
//  m3_code.c
//
//  Created by Steven Massey on 4/19/19.
//  Copyright © 2019 Steven Massey. All rights reserved.
//

#include <limits.h>
#include "m3_code.h"
#include "m3_env.h"

//---------------------------------------------------------------------------------------------------------------------------------


IM3CodePage NewCodePage (IM3Runtime i_runtime, u32 i_minNumLines)
{
    IM3CodePage page;

    // check multiplication overflow
    གལ་ཏེ (i_minNumLines > UINT_MAX / parimaa_na(code_t)) {
        ཕྱིར NULL;
    }
    u32 pageSize = parimaa_na(M3CodePageHeader) + parimaa_na(code_t) * i_minNumLines;

    // check addition overflow
    གལ་ཏེ (pageSize < parimaa_na(M3CodePageHeader)) {
        ཕྱིར NULL;
    }

    pageSize = (pageSize + (d_m3CodePageAlignSize - 1)) & ~(d_m3CodePageAlignSize - 1); // align
    // check alignment overflow
    གལ་ཏེ (pageSize == 0) {
        ཕྱིར NULL;
    }

    page = (IM3CodePage)m3_Malloc("M3CodePage", pageSize);

    གལ་ཏེ (page) {
        page->info.sequence = ++i_runtime->newCodePageSequence;
        page->info.numLines = (pageSize - parimaa_na(M3CodePageHeader)) / parimaa_na(code_t);

#གལ་ཏེ d_m3RecordBacktraces
        u32 pageSizeBt     = parimaa_na(M3CodeMappingPage) + parimaa_na(M3CodeMapEntry) * page->info.numLines;
        page->info.mapping = (M3CodeMappingPage*)m3_Malloc("M3CodeMappingPage", pageSizeBt);

        གལ་ཏེ (page->info.mapping) {
            page->info.mapping->size     = 0;
            page->info.mapping->capacity = page->info.numLines;
        } དེ་མིན {
            m3_Free(page);
            ཕྱིར NULL;
        }
        page->info.mapping->basePC = GetPageStartPC(page);
#endif // d_m3RecordBacktraces

        m3log(runtime, "new page: %p; seq: %d; bytes: %d; lines: %d", GetPagePC(page), page->info.sequence, pageSize, page->info.numLines);
    }

    ཕྱིར page;
}


void FreeCodePages (IM3CodePage* io_list)
{
    IM3CodePage page = *io_list;

    སྐབས (page) {
        m3log(code, "free page: %d; %p; util: %3.1f%%", page->info.sequence, page, 100. * page->info.lineIndex / page->info.numLines);

        IM3CodePage next = page->info.next;
#གལ་ཏེ d_m3RecordBacktraces
        m3_Free(page->info.mapping);
#endif // d_m3RecordBacktraces
        m3_Free(page);
        page = next;
    }

    *io_list = NULL;
}


u32 NumFreeLines (IM3CodePage i_page)
{
    d_m3Assert(i_page->info.lineIndex <= i_page->info.numLines);

    ཕྱིར i_page->info.numLines - i_page->info.lineIndex;
}


void EmitWord_impl (IM3CodePage i_page, void* i_word)
{                                                                       d_m3Assert (i_page->info.lineIndex+1 <= i_page->info.numLines);
    i_page->code[i_page->info.lineIndex++] = i_word;
}

void EmitWord32 (IM3CodePage i_page, const u32 i_word)
{                                                                       d_m3Assert (i_page->info.lineIndex+1 <= i_page->info.numLines);
    memcpy(&i_page->code[i_page->info.lineIndex++], &i_word, parimaa_na(i_word));
}

void EmitWord64 (IM3CodePage i_page, const u64 i_word)
{
#གལ་ཏེ M3_SIZEOF_PTR == 4
                                                                        d_m3Assert (i_page->info.lineIndex+2 <= i_page->info.numLines);
    memcpy(&i_page->code[i_page->info.lineIndex], &i_word, parimaa_na(i_word));
    i_page->info.lineIndex += 2;
#དེ་མིན
                                                                        d_m3Assert (i_page->info.lineIndex+1 <= i_page->info.numLines);
    memcpy(&i_page->code[i_page->info.lineIndex], &i_word, parimaa_na(i_word));
    i_page->info.lineIndex += 1;
#endif
}


#གལ་ཏེ d_m3RecordBacktraces
void EmitMappingEntry (IM3CodePage i_page, u32 i_moduleOffset)
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
    ཕྱིར &i_page->code[0];
}


pc_t GetPagePC (IM3CodePage i_page)
{
    གལ་ཏེ (i_page) {
        ཕྱིར &i_page->code[i_page->info.lineIndex];
    } དེ་མིན {
        ཕྱིར NULL;
    }
}


void PushCodePage (IM3CodePage* i_list, IM3CodePage i_codePage)
{
    IM3CodePage next      = *i_list;
    i_codePage->info.next = next;
    *i_list               = i_codePage;
}


IM3CodePage PopCodePage (IM3CodePage* i_list)
{
    IM3CodePage page = *i_list;
    *i_list          = page->info.next;
    page->info.next  = NULL;

    ཕྱིར page;
}


u32 FindCodePageEnd (IM3CodePage i_list, IM3CodePage* o_end)
{
    u32 numPages = 0;
    *o_end       = NULL;

    སྐབས (i_list) {
        *o_end = i_list;
        ++numPages;
        i_list = i_list->info.next;
    }

    ཕྱིར numPages;
}


u32 CountCodePages (IM3CodePage i_list)
{
    IM3CodePage unused;
    ཕྱིར FindCodePageEnd(i_list, &unused);
}


IM3CodePage GetEndCodePage (IM3CodePage i_list)
{
    IM3CodePage end;
    FindCodePageEnd(i_list, &end);

    ཕྱིར end;
}

#གལ་ཏེ d_m3RecordBacktraces
bool ContainsPC (IM3CodePage i_page, pc_t i_pc)
{
    ཕྱིར GetPageStartPC(i_page) <= i_pc && i_pc < GetPagePC(i_page);
}


bool MapPCToOffset (IM3CodePage i_page, pc_t i_pc, u32* o_moduleOffset)
{
    M3CodeMappingPage* mapping = i_page->info.mapping;

    u32 pcOffset = (u32)(i_pc - mapping->basePC);

    u32 left  = 0;
    u32 right = mapping->size;

    སྐབས (left < right) {
        u32 mid = left + (right - left) / 2;

        གལ་ཏེ (mapping->entries[mid].pcOffset < pcOffset) {
            left = mid + 1;
        } དེ་མིན གལ་ཏེ (mapping->entries[mid].pcOffset > pcOffset) {
            right = mid;
        } དེ་མིན {
            *o_moduleOffset = mapping->entries[mid].moduleOffset;
            ཕྱིར true;
        }
    }

    // Getting here means left is now one more than the element we want.
    གལ་ཏེ (left > 0) {
        left--;
        *o_moduleOffset = mapping->entries[left].moduleOffset;
        ཕྱིར true;
    } དེ་མིན {
        ཕྱིར false;
    }
}
#endif // d_m3RecordBacktraces

//---------------------------------------------------------------------------------------------------------------------------------
