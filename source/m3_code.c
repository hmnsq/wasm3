//
//  m3_code.c
//
//  Created by Steven Massey on 4/19/19.
//  Copyright © 2019 Steven Massey. All rights reserved.
//

#ଅନ୍ତର୍ଭାଵ <limits.h>
#ଅନ୍ତର୍ଭାଵ "m3_code.h"
#ଅନ୍ତର୍ଭାଵ "m3_env.h"

//---------------------------------------------------------------------------------------------------------------------------------


IM3CodePage NewCodePage (IM3Runtime i_runtime, u32 i_min_ଅଙ୍କ_ଧାଡ଼ିଗୁଡ଼ିକ)
{
    IM3CodePage page;

    // check multiplication overflow
    ଯଦି (i_min_ଅଙ୍କ_ଧାଡ଼ିଗୁଡ଼ିକ > UINT_MAX / ଆକାର(code_t)) {
        ଫେରନ୍ତୁ ଖାଲି;
    }
    u32 pageSize = ଆକାର(M3CodePageHeader) + ଆକାର(code_t) * i_min_ଅଙ୍କ_ଧାଡ଼ିଗୁଡ଼ିକ;

    // check addition overflow
    ଯଦି (pageSize < ଆକାର(M3CodePageHeader)) {
        ଫେରନ୍ତୁ ଖାଲି;
    }

    pageSize = (pageSize + (d_m3CodePageAlignSize - 1)) & ~(d_m3CodePageAlignSize - 1); // align
    // check alignment overflow
    ଯଦି (pageSize == 0) {
        ଫେରନ୍ତୁ ଖାଲି;
    }

    page = (IM3CodePage)m3_Malloc("M3CodePage", pageSize);

    ଯଦି (page) {
        page->info.sequence = ++i_runtime->newCodePageSequence;
        page->info.ଅଙ୍କ_ଧାଡ଼ିଗୁଡ଼ିକ = (pageSize - ଆକାର(M3CodePageHeader)) / ଆକାର(code_t);

#ଯଦି d_m3RecordBacktraces
        u32 pageSizeBt     = ଆକାର(M3CodeMappingPage) + ଆକାର(M3CodeMapEntry) * page->info.ଅଙ୍କ_ଧାଡ଼ିଗୁଡ଼ିକ;
        page->info.mapping = (M3CodeMappingPage*)m3_Malloc("M3CodeMappingPage", pageSizeBt);

        ଯଦି (page->info.mapping) {
            page->info.mapping->size     = 0;
            page->info.mapping->capacity = page->info.ଅଙ୍କ_ଧାଡ଼ିଗୁଡ଼ିକ;
        } ଅନ୍ୟଥା {
            m3_Free(page);
            ଫେରନ୍ତୁ ଖାଲି;
        }
        page->info.mapping->basePC = GetPageStartPC(page);
#endif // d_m3RecordBacktraces

        m3log(runtime, "new page: %p; seq: %d; bytes: %d; lines: %d", GetPagePC(page), page->info.sequence, pageSize, page->info.ଅଙ୍କ_ଧାଡ଼ିଗୁଡ଼ିକ);
    }

    ଫେରନ୍ତୁ page;
}


ଶୂନ୍ୟ FreeCodePages (IM3CodePage* io_list)
{
    IM3CodePage page = *io_list;

    ଯେତେବେଳେ (page) {
        m3log(code, "free page: %d; %p; util: %3.1f%%", page->info.sequence, page, 100. * page->info.ଧାଡ଼ି_Index / page->info.ଅଙ୍କ_ଧାଡ଼ିଗୁଡ଼ିକ);

        IM3CodePage next = page->info.next;
#ଯଦି d_m3RecordBacktraces
        m3_Free(page->info.mapping);
#endif // d_m3RecordBacktraces
        m3_Free(page);
        page = next;
    }

    *io_list = ଖାଲି;
}


u32 ଅଙ୍କ_Free_ଧାଡ଼ିଗୁଡ଼ିକ (IM3CodePage i_page)
{
    d_m3Assert(i_page->info.ଧାଡ଼ି_Index <= i_page->info.ଅଙ୍କ_ଧାଡ଼ିଗୁଡ଼ିକ);

    ଫେରନ୍ତୁ i_page->info.ଅଙ୍କ_ଧାଡ଼ିଗୁଡ଼ିକ - i_page->info.ଧାଡ଼ି_Index;
}


ଶୂନ୍ୟ EmitWord_impl (IM3CodePage i_page, ଶୂନ୍ୟ* i_word)
{                                                                       d_m3Assert (i_page->info.ଧାଡ଼ି_Index+1 <= i_page->info.ଅଙ୍କ_ଧାଡ଼ିଗୁଡ଼ିକ);
    i_page->code[i_page->info.ଧାଡ଼ି_Index++] = i_word;
}

ଶୂନ୍ୟ EmitWord32 (IM3CodePage i_page, ଅଚଳ u32 i_word)
{                                                                       d_m3Assert (i_page->info.ଧାଡ଼ି_Index+1 <= i_page->info.ଅଙ୍କ_ଧାଡ଼ିଗୁଡ଼ିକ);
    memcpy(&i_page->code[i_page->info.ଧାଡ଼ି_Index++], &i_word, ଆକାର(i_word));
}

ଶୂନ୍ୟ EmitWord64 (IM3CodePage i_page, ଅଚଳ u64 i_word)
{
#ଯଦି M3_SIZEOF_PTR == 4
                                                                        d_m3Assert (i_page->info.ଧାଡ଼ି_Index+2 <= i_page->info.ଅଙ୍କ_ଧାଡ଼ିଗୁଡ଼ିକ);
    memcpy(&i_page->code[i_page->info.ଧାଡ଼ି_Index], &i_word, ଆକାର(i_word));
    i_page->info.ଧାଡ଼ି_Index += 2;
#ଅନ୍ୟଥା
                                                                        d_m3Assert (i_page->info.ଧାଡ଼ି_Index+1 <= i_page->info.ଅଙ୍କ_ଧାଡ଼ିଗୁଡ଼ିକ);
    memcpy(&i_page->code[i_page->info.ଧାଡ଼ି_Index], &i_word, ଆକାର(i_word));
    i_page->info.ଧାଡ଼ି_Index += 1;
#endif
}


#ଯଦି d_m3RecordBacktraces
ଶୂନ୍ୟ EmitMappingEntry (IM3CodePage i_page, u32 i_moduleOffset)
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
    ଫେରନ୍ତୁ &i_page->code[0];
}


pc_t GetPagePC (IM3CodePage i_page)
{
    ଯଦି (i_page) {
        ଫେରନ୍ତୁ &i_page->code[i_page->info.ଧାଡ଼ି_Index];
    } ଅନ୍ୟଥା {
        ଫେରନ୍ତୁ ଖାଲି;
    }
}


ଶୂନ୍ୟ ଠେଲ_Code_Page (IM3CodePage* i_list, IM3CodePage i_codePage)
{
    IM3CodePage next      = *i_list;
    i_codePage->info.next = next;
    *i_list               = i_codePage;
}


IM3CodePage କାଢ଼_Code_Page (IM3CodePage* i_list)
{
    IM3CodePage page = *i_list;
    *i_list          = page->info.next;
    page->info.next  = ଖାଲି;

    ଫେରନ୍ତୁ page;
}


u32 FindCodePageEnd (IM3CodePage i_list, IM3CodePage* o_end)
{
    u32 ଅଙ୍କ_Pages = 0;
    *o_end       = ଖାଲି;

    ଯେତେବେଳେ (i_list) {
        *o_end = i_list;
        ++ଅଙ୍କ_Pages;
        i_list = i_list->info.next;
    }

    ଫେରନ୍ତୁ ଅଙ୍କ_Pages;
}


u32 ଗଣନା_Code_Pages (IM3CodePage i_list)
{
    IM3CodePage unused;
    ଫେରନ୍ତୁ FindCodePageEnd(i_list, &unused);
}


IM3CodePage GetEndCodePage (IM3CodePage i_list)
{
    IM3CodePage end;
    FindCodePageEnd(i_list, &end);

    ଫେରନ୍ତୁ end;
}

#ଯଦି d_m3RecordBacktraces
bool ContainsPC (IM3CodePage i_page, pc_t i_pc)
{
    ଫେରନ୍ତୁ GetPageStartPC(i_page) <= i_pc && i_pc < GetPagePC(i_page);
}


bool MapPCToOffset (IM3CodePage i_page, pc_t i_pc, u32* o_moduleOffset)
{
    M3CodeMappingPage* mapping = i_page->info.mapping;

    u32 pcOffset = (u32)(i_pc - mapping->basePC);

    u32 ବାମ  = 0;
    u32 ଡାହାଣ = mapping->size;

    ଯେତେବେଳେ (ବାମ < ଡାହାଣ) {
        u32 mid = ବାମ + (ଡାହାଣ - ବାମ) / 2;

        ଯଦି (mapping->entries[mid].pcOffset < pcOffset) {
            ବାମ = mid + 1;
        } ଅନ୍ୟଥା ଯଦି (mapping->entries[mid].pcOffset > pcOffset) {
            ଡାହାଣ = mid;
        } ଅନ୍ୟଥା {
            *o_moduleOffset = mapping->entries[mid].moduleOffset;
            ଫେରନ୍ତୁ true;
        }
    }

    // Getting here means left is now one more than the element we want.
    ଯଦି (ବାମ > 0) {
        ବାମ--;
        *o_moduleOffset = mapping->entries[ବାମ].moduleOffset;
        ଫେରନ୍ତୁ true;
    } ଅନ୍ୟଥା {
        ଫେରନ୍ତୁ false;
    }
}
#endif // d_m3RecordBacktraces

//---------------------------------------------------------------------------------------------------------------------------------
