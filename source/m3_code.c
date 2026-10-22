//
//  m3_code.c
//
//  Created by Steven Massey on 4/19/19.
//  Copyright © 2019 Steven Massey. All rights reserved.
//

#അന്തര്ഭാവ <limits.h>
#അന്തര്ഭാവ "m3_code.h"
#അന്തര്ഭാവ "m3_env.h"

//---------------------------------------------------------------------------------------------------------------------------------


IM3CodePage NewCodePage (IM3Runtime i_runtime, u32 i_min_അക്കം_വരികൾ)
{
    IM3CodePage page;

    // check multiplication overflow
    എങ്കിൽ (i_min_അക്കം_വരികൾ > UINT_MAX / വലുപ്പം(code_t)) {
        തിരികെ ശൂന്യ;
    }
    u32 pageSize = വലുപ്പം(M3CodePageHeader) + വലുപ്പം(code_t) * i_min_അക്കം_വരികൾ;

    // check addition overflow
    എങ്കിൽ (pageSize < വലുപ്പം(M3CodePageHeader)) {
        തിരികെ ശൂന്യ;
    }

    pageSize = (pageSize + (d_m3CodePageAlignSize - 1)) & ~(d_m3CodePageAlignSize - 1); // align
    // check alignment overflow
    എങ്കിൽ (pageSize == 0) {
        തിരികെ ശൂന്യ;
    }

    page = (IM3CodePage)m3_Malloc("M3CodePage", pageSize);

    എങ്കിൽ (page) {
        page->info.sequence = ++i_runtime->newCodePageSequence;
        page->info.അക്കം_വരികൾ = (pageSize - വലുപ്പം(M3CodePageHeader)) / വലുപ്പം(code_t);

#എങ്കിൽ d_m3RecordBacktraces
        u32 pageSizeBt     = വലുപ്പം(M3CodeMappingPage) + വലുപ്പം(M3CodeMapEntry) * page->info.അക്കം_വരികൾ;
        page->info.mapping = (M3CodeMappingPage*)m3_Malloc("M3CodeMappingPage", pageSizeBt);

        എങ്കിൽ (page->info.mapping) {
            page->info.mapping->size     = 0;
            page->info.mapping->capacity = page->info.അക്കം_വരികൾ;
        } അല്ലെങ്കിൽ {
            m3_Free(page);
            തിരികെ ശൂന്യ;
        }
        page->info.mapping->basePC = GetPageStartPC(page);
#endif // d_m3RecordBacktraces

        m3log(runtime, "new page: %p; seq: %d; bytes: %d; lines: %d", GetPagePC(page), page->info.sequence, pageSize, page->info.അക്കം_വരികൾ);
    }

    തിരികെ page;
}


ശൂന്യം FreeCodePages (IM3CodePage* io_list)
{
    IM3CodePage page = *io_list;

    ആകുമ്പോൾ (page) {
        m3log(code, "free page: %d; %p; util: %3.1f%%", page->info.sequence, page, 100. * page->info.വരി_Index / page->info.അക്കം_വരികൾ);

        IM3CodePage next = page->info.next;
#എങ്കിൽ d_m3RecordBacktraces
        m3_Free(page->info.mapping);
#endif // d_m3RecordBacktraces
        m3_Free(page);
        page = next;
    }

    *io_list = ശൂന്യ;
}


u32 അക്കം_Free_വരികൾ (IM3CodePage i_page)
{
    d_m3Assert(i_page->info.വരി_Index <= i_page->info.അക്കം_വരികൾ);

    തിരികെ i_page->info.അക്കം_വരികൾ - i_page->info.വരി_Index;
}


ശൂന്യം EmitWord_impl (IM3CodePage i_page, ശൂന്യം* i_word)
{                                                                       d_m3Assert (i_page->info.വരി_Index+1 <= i_page->info.അക്കം_വരികൾ);
    i_page->code[i_page->info.വരി_Index++] = i_word;
}

ശൂന്യം EmitWord32 (IM3CodePage i_page, സ്ഥിരം u32 i_word)
{                                                                       d_m3Assert (i_page->info.വരി_Index+1 <= i_page->info.അക്കം_വരികൾ);
    memcpy(&i_page->code[i_page->info.വരി_Index++], &i_word, വലുപ്പം(i_word));
}

ശൂന്യം EmitWord64 (IM3CodePage i_page, സ്ഥിരം u64 i_word)
{
#എങ്കിൽ M3_SIZEOF_PTR == 4
                                                                        d_m3Assert (i_page->info.വരി_Index+2 <= i_page->info.അക്കം_വരികൾ);
    memcpy(&i_page->code[i_page->info.വരി_Index], &i_word, വലുപ്പം(i_word));
    i_page->info.വരി_Index += 2;
#അല്ലെങ്കിൽ
                                                                        d_m3Assert (i_page->info.വരി_Index+1 <= i_page->info.അക്കം_വരികൾ);
    memcpy(&i_page->code[i_page->info.വരി_Index], &i_word, വലുപ്പം(i_word));
    i_page->info.വരി_Index += 1;
#endif
}


#എങ്കിൽ d_m3RecordBacktraces
ശൂന്യം EmitMappingEntry (IM3CodePage i_page, u32 i_moduleOffset)
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
    തിരികെ &i_page->code[0];
}


pc_t GetPagePC (IM3CodePage i_page)
{
    എങ്കിൽ (i_page) {
        തിരികെ &i_page->code[i_page->info.വരി_Index];
    } അല്ലെങ്കിൽ {
        തിരികെ ശൂന്യ;
    }
}


ശൂന്യം തള്ളുക_Code_Page (IM3CodePage* i_list, IM3CodePage i_codePage)
{
    IM3CodePage next      = *i_list;
    i_codePage->info.next = next;
    *i_list               = i_codePage;
}


IM3CodePage എടുക്കുക_Code_Page (IM3CodePage* i_list)
{
    IM3CodePage page = *i_list;
    *i_list          = page->info.next;
    page->info.next  = ശൂന്യ;

    തിരികെ page;
}


u32 FindCodePageEnd (IM3CodePage i_list, IM3CodePage* o_end)
{
    u32 അക്കം_Pages = 0;
    *o_end       = ശൂന്യ;

    ആകുമ്പോൾ (i_list) {
        *o_end = i_list;
        ++അക്കം_Pages;
        i_list = i_list->info.next;
    }

    തിരികെ അക്കം_Pages;
}


u32 എണ്ണം_Code_Pages (IM3CodePage i_list)
{
    IM3CodePage unused;
    തിരികെ FindCodePageEnd(i_list, &unused);
}


IM3CodePage GetEndCodePage (IM3CodePage i_list)
{
    IM3CodePage end;
    FindCodePageEnd(i_list, &end);

    തിരികെ end;
}

#എങ്കിൽ d_m3RecordBacktraces
bool ContainsPC (IM3CodePage i_page, pc_t i_pc)
{
    തിരികെ GetPageStartPC(i_page) <= i_pc && i_pc < GetPagePC(i_page);
}


bool MapPCToOffset (IM3CodePage i_page, pc_t i_pc, u32* o_moduleOffset)
{
    M3CodeMappingPage* mapping = i_page->info.mapping;

    u32 pcOffset = (u32)(i_pc - mapping->basePC);

    u32 ഇടത്  = 0;
    u32 വലത് = mapping->size;

    ആകുമ്പോൾ (ഇടത് < വലത്) {
        u32 mid = ഇടത് + (വലത് - ഇടത്) / 2;

        എങ്കിൽ (mapping->entries[mid].pcOffset < pcOffset) {
            ഇടത് = mid + 1;
        } അല്ലെങ്കിൽ എങ്കിൽ (mapping->entries[mid].pcOffset > pcOffset) {
            വലത് = mid;
        } അല്ലെങ്കിൽ {
            *o_moduleOffset = mapping->entries[mid].moduleOffset;
            തിരികെ true;
        }
    }

    // Getting here means left is now one more than the element we want.
    എങ്കിൽ (ഇടത് > 0) {
        ഇടത്--;
        *o_moduleOffset = mapping->entries[ഇടത്].moduleOffset;
        തിരികെ true;
    } അല്ലെങ്കിൽ {
        തിരികെ false;
    }
}
#endif // d_m3RecordBacktraces

//---------------------------------------------------------------------------------------------------------------------------------
