//
//  m3_code.c
//
//  Created by Steven Massey on 4/19/19.
//  Copyright © 2019 Steven Massey. All rights reserved.
//

#ಅನ್ತರ್ಭಾವ <limits.h>
#ಅನ್ತರ್ಭಾವ "m3_code.h"
#ಅನ್ತರ್ಭಾವ "m3_env.h"

//---------------------------------------------------------------------------------------------------------------------------------


IM3CodePage NewCodePage (IM3Runtime i_runtime, u32 i_min_ಅಂಕೆ_ಸಾಲುಗಳು)
{
    IM3CodePage page;

    // check multiplication overflow
    ಯದಿ (i_min_ಅಂಕೆ_ಸಾಲುಗಳು > UINT_MAX / ಪರಿಮಾಣ(code_t)) {
        ಪ್ರತ್ಯಾಗಮ ರಿಕ್ತ;
    }
    u32 pageSize = ಪರಿಮಾಣ(M3CodePageHeader) + ಪರಿಮಾಣ(code_t) * i_min_ಅಂಕೆ_ಸಾಲುಗಳು;

    // check addition overflow
    ಯದಿ (pageSize < ಪರಿಮಾಣ(M3CodePageHeader)) {
        ಪ್ರತ್ಯಾಗಮ ರಿಕ್ತ;
    }

    pageSize = (pageSize + (d_m3CodePageAlignSize - 1)) & ~(d_m3CodePageAlignSize - 1); // align
    // check alignment overflow
    ಯದಿ (pageSize == 0) {
        ಪ್ರತ್ಯಾಗಮ ರಿಕ್ತ;
    }

    page = (IM3CodePage)m3_Malloc("M3CodePage", pageSize);

    ಯದಿ (page) {
        page->info.sequence = ++i_runtime->newCodePageSequence;
        page->info.ಅಂಕೆ_ಸಾಲುಗಳು = (pageSize - ಪರಿಮಾಣ(M3CodePageHeader)) / ಪರಿಮಾಣ(code_t);

#ಯದಿ d_m3RecordBacktraces
        u32 pageSizeBt     = ಪರಿಮಾಣ(M3CodeMappingPage) + ಪರಿಮಾಣ(M3CodeMapEntry) * page->info.ಅಂಕೆ_ಸಾಲುಗಳು;
        page->info.mapping = (M3CodeMappingPage*)m3_Malloc("M3CodeMappingPage", pageSizeBt);

        ಯದಿ (page->info.mapping) {
            page->info.mapping->size     = 0;
            page->info.mapping->capacity = page->info.ಅಂಕೆ_ಸಾಲುಗಳು;
        } ಅನ್ಯಥಾ {
            m3_Free(page);
            ಪ್ರತ್ಯಾಗಮ ರಿಕ್ತ;
        }
        page->info.mapping->basePC = GetPageStartPC(page);
#endif // d_m3RecordBacktraces

        m3log(runtime, "new page: %p; seq: %d; bytes: %d; lines: %d", GetPagePC(page), page->info.sequence, pageSize, page->info.ಅಂಕೆ_ಸಾಲುಗಳು);
    }

    ಪ್ರತ್ಯಾಗಮ page;
}


ಶೂನ್ಯ FreeCodePages (IM3CodePage* io_list)
{
    IM3CodePage page = *io_list;

    ಯಾವತ್ (page) {
        m3log(code, "free page: %d; %p; util: %3.1f%%", page->info.sequence, page, 100. * page->info.ಸಾಲು_Index / page->info.ಅಂಕೆ_ಸಾಲುಗಳು);

        IM3CodePage next = page->info.next;
#ಯದಿ d_m3RecordBacktraces
        m3_Free(page->info.mapping);
#endif // d_m3RecordBacktraces
        m3_Free(page);
        page = next;
    }

    *io_list = ರಿಕ್ತ;
}


u32 ಅಂಕೆ_Free_ಸಾಲುಗಳು (IM3CodePage i_page)
{
    d_m3Assert(i_page->info.ಸಾಲು_Index <= i_page->info.ಅಂಕೆ_ಸಾಲುಗಳು);

    ಪ್ರತ್ಯಾಗಮ i_page->info.ಅಂಕೆ_ಸಾಲುಗಳು - i_page->info.ಸಾಲು_Index;
}


ಶೂನ್ಯ EmitWord_impl (IM3CodePage i_page, ಶೂನ್ಯ* i_word)
{                                                                       d_m3Assert (i_page->info.ಸಾಲು_Index+1 <= i_page->info.ಅಂಕೆ_ಸಾಲುಗಳು);
    i_page->code[i_page->info.ಸಾಲು_Index++] = i_word;
}

ಶೂನ್ಯ EmitWord32 (IM3CodePage i_page, ಧ್ರುವ u32 i_word)
{                                                                       d_m3Assert (i_page->info.ಸಾಲು_Index+1 <= i_page->info.ಅಂಕೆ_ಸಾಲುಗಳು);
    memcpy(&i_page->code[i_page->info.ಸಾಲು_Index++], &i_word, ಪರಿಮಾಣ(i_word));
}

ಶೂನ್ಯ EmitWord64 (IM3CodePage i_page, ಧ್ರುವ u64 i_word)
{
#ಯದಿ M3_SIZEOF_PTR == 4
                                                                        d_m3Assert (i_page->info.ಸಾಲು_Index+2 <= i_page->info.ಅಂಕೆ_ಸಾಲುಗಳು);
    memcpy(&i_page->code[i_page->info.ಸಾಲು_Index], &i_word, ಪರಿಮಾಣ(i_word));
    i_page->info.ಸಾಲು_Index += 2;
#ಅನ್ಯಥಾ
                                                                        d_m3Assert (i_page->info.ಸಾಲು_Index+1 <= i_page->info.ಅಂಕೆ_ಸಾಲುಗಳು);
    memcpy(&i_page->code[i_page->info.ಸಾಲು_Index], &i_word, ಪರಿಮಾಣ(i_word));
    i_page->info.ಸಾಲು_Index += 1;
#endif
}


#ಯದಿ d_m3RecordBacktraces
ಶೂನ್ಯ EmitMappingEntry (IM3CodePage i_page, u32 i_moduleOffset)
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
    ಪ್ರತ್ಯಾಗಮ &i_page->code[0];
}


pc_t GetPagePC (IM3CodePage i_page)
{
    ಯದಿ (i_page) {
        ಪ್ರತ್ಯಾಗಮ &i_page->code[i_page->info.ಸಾಲು_Index];
    } ಅನ್ಯಥಾ {
        ಪ್ರತ್ಯಾಗಮ ರಿಕ್ತ;
    }
}


ಶೂನ್ಯ ತಳ್ಳು_Code_Page (IM3CodePage* i_list, IM3CodePage i_codePage)
{
    IM3CodePage next      = *i_list;
    i_codePage->info.next = next;
    *i_list               = i_codePage;
}


IM3CodePage ತೆಗೆ_Code_Page (IM3CodePage* i_list)
{
    IM3CodePage page = *i_list;
    *i_list          = page->info.next;
    page->info.next  = ರಿಕ್ತ;

    ಪ್ರತ್ಯಾಗಮ page;
}


u32 FindCodePageEnd (IM3CodePage i_list, IM3CodePage* o_end)
{
    u32 ಅಂಕೆ_Pages = 0;
    *o_end       = ರಿಕ್ತ;

    ಯಾವತ್ (i_list) {
        *o_end = i_list;
        ++ಅಂಕೆ_Pages;
        i_list = i_list->info.next;
    }

    ಪ್ರತ್ಯಾಗಮ ಅಂಕೆ_Pages;
}


u32 ಎಣಿಕೆ_Code_Pages (IM3CodePage i_list)
{
    IM3CodePage unused;
    ಪ್ರತ್ಯಾಗಮ FindCodePageEnd(i_list, &unused);
}


IM3CodePage GetEndCodePage (IM3CodePage i_list)
{
    IM3CodePage end;
    FindCodePageEnd(i_list, &end);

    ಪ್ರತ್ಯಾಗಮ end;
}

#ಯದಿ d_m3RecordBacktraces
bool ContainsPC (IM3CodePage i_page, pc_t i_pc)
{
    ಪ್ರತ್ಯಾಗಮ GetPageStartPC(i_page) <= i_pc && i_pc < GetPagePC(i_page);
}


bool MapPCToOffset (IM3CodePage i_page, pc_t i_pc, u32* o_moduleOffset)
{
    M3CodeMappingPage* mapping = i_page->info.mapping;

    u32 pcOffset = (u32)(i_pc - mapping->basePC);

    u32 ಎಡ  = 0;
    u32 ಬಲ = mapping->size;

    ಯಾವತ್ (ಎಡ < ಬಲ) {
        u32 mid = ಎಡ + (ಬಲ - ಎಡ) / 2;

        ಯದಿ (mapping->entries[mid].pcOffset < pcOffset) {
            ಎಡ = mid + 1;
        } ಅನ್ಯಥಾ ಯದಿ (mapping->entries[mid].pcOffset > pcOffset) {
            ಬಲ = mid;
        } ಅನ್ಯಥಾ {
            *o_moduleOffset = mapping->entries[mid].moduleOffset;
            ಪ್ರತ್ಯಾಗಮ true;
        }
    }

    // Getting here means left is now one more than the element we want.
    ಯದಿ (ಎಡ > 0) {
        ಎಡ--;
        *o_moduleOffset = mapping->entries[ಎಡ].moduleOffset;
        ಪ್ರತ್ಯಾಗಮ true;
    } ಅನ್ಯಥಾ {
        ಪ್ರತ್ಯಾಗಮ false;
    }
}
#endif // d_m3RecordBacktraces

//---------------------------------------------------------------------------------------------------------------------------------
