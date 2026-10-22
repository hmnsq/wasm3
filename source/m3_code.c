//
//  m3_code.c
//
//  Created by Steven Massey on 4/19/19.
//  Copyright © 2019 Steven Massey. All rights reserved.
//

#અન્તર્ભાવ <limits.h>
#અન્તર્ભાવ "m3_code.h"
#અન્તર્ભાવ "m3_env.h"

//---------------------------------------------------------------------------------------------------------------------------------


IM3CodePage NewCodePage (IM3Runtime i_runtime, u32 i_min_અંક_લીટીઓ)
{
    IM3CodePage page;

    // check multiplication overflow
    જો (i_min_અંક_લીટીઓ > UINT_MAX / કદ(code_t)) {
        પાછા ખાલી;
    }
    u32 pageSize = કદ(M3CodePageHeader) + કદ(code_t) * i_min_અંક_લીટીઓ;

    // check addition overflow
    જો (pageSize < કદ(M3CodePageHeader)) {
        પાછા ખાલી;
    }

    pageSize = (pageSize + (d_m3CodePageAlignSize - 1)) & ~(d_m3CodePageAlignSize - 1); // align
    // check alignment overflow
    જો (pageSize == 0) {
        પાછા ખાલી;
    }

    page = (IM3CodePage)m3_Malloc("M3CodePage", pageSize);

    જો (page) {
        page->info.sequence = ++i_runtime->newCodePageSequence;
        page->info.અંક_લીટીઓ = (pageSize - કદ(M3CodePageHeader)) / કદ(code_t);

#જો d_m3RecordBacktraces
        u32 pageSizeBt     = કદ(M3CodeMappingPage) + કદ(M3CodeMapEntry) * page->info.અંક_લીટીઓ;
        page->info.mapping = (M3CodeMappingPage*)m3_Malloc("M3CodeMappingPage", pageSizeBt);

        જો (page->info.mapping) {
            page->info.mapping->size     = 0;
            page->info.mapping->capacity = page->info.અંક_લીટીઓ;
        } નહીંતર {
            m3_Free(page);
            પાછા ખાલી;
        }
        page->info.mapping->basePC = GetPageStartPC(page);
#endif // d_m3RecordBacktraces

        m3log(runtime, "new page: %p; seq: %d; bytes: %d; lines: %d", GetPagePC(page), page->info.sequence, pageSize, page->info.અંક_લીટીઓ);
    }

    પાછા page;
}


શૂન્ય FreeCodePages (IM3CodePage* io_list)
{
    IM3CodePage page = *io_list;

    જ્યારે (page) {
        m3log(code, "free page: %d; %p; util: %3.1f%%", page->info.sequence, page, 100. * page->info.લીટી_Index / page->info.અંક_લીટીઓ);

        IM3CodePage next = page->info.next;
#જો d_m3RecordBacktraces
        m3_Free(page->info.mapping);
#endif // d_m3RecordBacktraces
        m3_Free(page);
        page = next;
    }

    *io_list = ખાલી;
}


u32 અંક_Free_લીટીઓ (IM3CodePage i_page)
{
    d_m3Assert(i_page->info.લીટી_Index <= i_page->info.અંક_લીટીઓ);

    પાછા i_page->info.અંક_લીટીઓ - i_page->info.લીટી_Index;
}


શૂન્ય EmitWord_impl (IM3CodePage i_page, શૂન્ય* i_word)
{                                                                       d_m3Assert (i_page->info.લીટી_Index+1 <= i_page->info.અંક_લીટીઓ);
    i_page->code[i_page->info.લીટી_Index++] = i_word;
}

શૂન્ય EmitWord32 (IM3CodePage i_page, અચલ u32 i_word)
{                                                                       d_m3Assert (i_page->info.લીટી_Index+1 <= i_page->info.અંક_લીટીઓ);
    memcpy(&i_page->code[i_page->info.લીટી_Index++], &i_word, કદ(i_word));
}

શૂન્ય EmitWord64 (IM3CodePage i_page, અચલ u64 i_word)
{
#જો M3_SIZEOF_PTR == 4
                                                                        d_m3Assert (i_page->info.લીટી_Index+2 <= i_page->info.અંક_લીટીઓ);
    memcpy(&i_page->code[i_page->info.લીટી_Index], &i_word, કદ(i_word));
    i_page->info.લીટી_Index += 2;
#નહીંતર
                                                                        d_m3Assert (i_page->info.લીટી_Index+1 <= i_page->info.અંક_લીટીઓ);
    memcpy(&i_page->code[i_page->info.લીટી_Index], &i_word, કદ(i_word));
    i_page->info.લીટી_Index += 1;
#endif
}


#જો d_m3RecordBacktraces
શૂન્ય EmitMappingEntry (IM3CodePage i_page, u32 i_moduleOffset)
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
    પાછા &i_page->code[0];
}


pc_t GetPagePC (IM3CodePage i_page)
{
    જો (i_page) {
        પાછા &i_page->code[i_page->info.લીટી_Index];
    } નહીંતર {
        પાછા ખાલી;
    }
}


શૂન્ય ધકેલો_Code_Page (IM3CodePage* i_list, IM3CodePage i_codePage)
{
    IM3CodePage next      = *i_list;
    i_codePage->info.next = next;
    *i_list               = i_codePage;
}


IM3CodePage કાઢો_Code_Page (IM3CodePage* i_list)
{
    IM3CodePage page = *i_list;
    *i_list          = page->info.next;
    page->info.next  = ખાલી;

    પાછા page;
}


u32 FindCodePageEnd (IM3CodePage i_list, IM3CodePage* o_end)
{
    u32 અંક_Pages = 0;
    *o_end       = ખાલી;

    જ્યારે (i_list) {
        *o_end = i_list;
        ++અંક_Pages;
        i_list = i_list->info.next;
    }

    પાછા અંક_Pages;
}


u32 ગણતરી_Code_Pages (IM3CodePage i_list)
{
    IM3CodePage unused;
    પાછા FindCodePageEnd(i_list, &unused);
}


IM3CodePage GetEndCodePage (IM3CodePage i_list)
{
    IM3CodePage end;
    FindCodePageEnd(i_list, &end);

    પાછા end;
}

#જો d_m3RecordBacktraces
bool ContainsPC (IM3CodePage i_page, pc_t i_pc)
{
    પાછા GetPageStartPC(i_page) <= i_pc && i_pc < GetPagePC(i_page);
}


bool MapPCToOffset (IM3CodePage i_page, pc_t i_pc, u32* o_moduleOffset)
{
    M3CodeMappingPage* mapping = i_page->info.mapping;

    u32 pcOffset = (u32)(i_pc - mapping->basePC);

    u32 ડાબું  = 0;
    u32 જમણું = mapping->size;

    જ્યારે (ડાબું < જમણું) {
        u32 mid = ડાબું + (જમણું - ડાબું) / 2;

        જો (mapping->entries[mid].pcOffset < pcOffset) {
            ડાબું = mid + 1;
        } નહીંતર જો (mapping->entries[mid].pcOffset > pcOffset) {
            જમણું = mid;
        } નહીંતર {
            *o_moduleOffset = mapping->entries[mid].moduleOffset;
            પાછા true;
        }
    }

    // Getting here means left is now one more than the element we want.
    જો (ડાબું > 0) {
        ડાબું--;
        *o_moduleOffset = mapping->entries[ડાબું].moduleOffset;
        પાછા true;
    } નહીંતર {
        પાછા false;
    }
}
#endif // d_m3RecordBacktraces

//---------------------------------------------------------------------------------------------------------------------------------
