//
//  m3_code.c
//
//  Created by Steven Massey on 4/19/19.
//  Copyright © 2019 Steven Massey. All rights reserved.
//

#अन्तर्भाव <limits.h>
#अन्तर्भाव "m3_code.h"
#अन्तर्भाव "m3_env.h"

//---------------------------------------------------------------------------------------------------------------------------------


IM3CodePage NewCodePage (IM3Runtime i_runtime, u32 i_min_अंक_ओळी)
{
    IM3CodePage page;

    // check multiplication overflow
    जर (i_min_अंक_ओळी > अच_पूर्णांक_उच्च / आकार(code_t)) {
        परत रिक्त;
    }
    u32 pageSize = आकार(M3CodePageHeader) + आकार(code_t) * i_min_अंक_ओळी;

    // check addition overflow
    जर (pageSize < आकार(M3CodePageHeader)) {
        परत रिक्त;
    }

    pageSize = (pageSize + (d_m3CodePageAlignSize - 1)) & ~(d_m3CodePageAlignSize - 1); // align
    // check alignment overflow
    जर (pageSize == 0) {
        परत रिक्त;
    }

    page = (IM3CodePage)m3_Malloc("M3CodePage", pageSize);

    जर (page) {
        page->info.sequence = ++i_runtime->newCodePageSequence;
        page->info.अंक_ओळी = (pageSize - आकार(M3CodePageHeader)) / आकार(code_t);

#जर d_m3RecordBacktraces
        u32 pageSizeBt     = आकार(M3CodeMappingPage) + आकार(M3CodeMapEntry) * page->info.अंक_ओळी;
        page->info.mapping = (M3CodeMappingPage*)m3_Malloc("M3CodeMappingPage", pageSizeBt);

        जर (page->info.mapping) {
            page->info.mapping->size     = 0;
            page->info.mapping->capacity = page->info.अंक_ओळी;
        } अन्यथा {
            m3_Free(page);
            परत रिक्त;
        }
        page->info.mapping->basePC = GetPageStartPC(page);
#endif // d_m3RecordBacktraces

        m3log(runtime, "new page: %p; seq: %d; bytes: %d; lines: %d", GetPagePC(page), page->info.sequence, pageSize, page->info.अंक_ओळी);
    }

    परत page;
}


शून्य FreeCodePages (IM3CodePage* io_list)
{
    IM3CodePage page = *io_list;

    पर्यंत (page) {
        m3log(code, "free page: %d; %p; util: %3.1f%%", page->info.sequence, page, 100. * page->info.ओळ_Index / page->info.अंक_ओळी);

        IM3CodePage next = page->info.next;
#जर d_m3RecordBacktraces
        m3_Free(page->info.mapping);
#endif // d_m3RecordBacktraces
        m3_Free(page);
        page = next;
    }

    *io_list = रिक्त;
}


u32 अंक_Free_ओळी (IM3CodePage i_page)
{
    d_m3Assert(i_page->info.ओळ_Index <= i_page->info.अंक_ओळी);

    परत i_page->info.अंक_ओळी - i_page->info.ओळ_Index;
}


शून्य EmitWord_impl (IM3CodePage i_page, शून्य* i_word)
{                                                                       d_m3Assert (i_page->info.ओळ_Index+1 <= i_page->info.अंक_ओळी);
    i_page->code[i_page->info.ओळ_Index++] = i_word;
}

शून्य EmitWord32 (IM3CodePage i_page, स्थिरांक u32 i_word)
{                                                                       d_m3Assert (i_page->info.ओळ_Index+1 <= i_page->info.अंक_ओळी);
    स_नकल(&i_page->code[i_page->info.ओळ_Index++], &i_word, आकार(i_word));
}

शून्य EmitWord64 (IM3CodePage i_page, स्थिरांक u64 i_word)
{
#जर M3_SIZEOF_PTR == 4
                                                                        d_m3Assert (i_page->info.ओळ_Index+2 <= i_page->info.अंक_ओळी);
    स_नकल(&i_page->code[i_page->info.ओळ_Index], &i_word, आकार(i_word));
    i_page->info.ओळ_Index += 2;
#अन्यथा
                                                                        d_m3Assert (i_page->info.ओळ_Index+1 <= i_page->info.अंक_ओळी);
    स_नकल(&i_page->code[i_page->info.ओळ_Index], &i_word, आकार(i_word));
    i_page->info.ओळ_Index += 1;
#endif
}


#जर d_m3RecordBacktraces
शून्य EmitMappingEntry (IM3CodePage i_page, u32 i_moduleOffset)
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
    परत &i_page->code[0];
}


pc_t GetPagePC (IM3CodePage i_page)
{
    जर (i_page) {
        परत &i_page->code[i_page->info.ओळ_Index];
    } अन्यथा {
        परत रिक्त;
    }
}


शून्य ढकला_Code_Page (IM3CodePage* i_list, IM3CodePage i_codePage)
{
    IM3CodePage next      = *i_list;
    i_codePage->info.next = next;
    *i_list               = i_codePage;
}


IM3CodePage काढा_Code_Page (IM3CodePage* i_list)
{
    IM3CodePage page = *i_list;
    *i_list          = page->info.next;
    page->info.next  = रिक्त;

    परत page;
}


u32 FindCodePageEnd (IM3CodePage i_list, IM3CodePage* o_end)
{
    u32 अंक_Pages = 0;
    *o_end       = रिक्त;

    पर्यंत (i_list) {
        *o_end = i_list;
        ++अंक_Pages;
        i_list = i_list->info.next;
    }

    परत अंक_Pages;
}


u32 मोजणी_Code_Pages (IM3CodePage i_list)
{
    IM3CodePage unused;
    परत FindCodePageEnd(i_list, &unused);
}


IM3CodePage GetEndCodePage (IM3CodePage i_list)
{
    IM3CodePage end;
    FindCodePageEnd(i_list, &end);

    परत end;
}

#जर d_m3RecordBacktraces
bool ContainsPC (IM3CodePage i_page, pc_t i_pc)
{
    परत GetPageStartPC(i_page) <= i_pc && i_pc < GetPagePC(i_page);
}


bool MapPCToOffset (IM3CodePage i_page, pc_t i_pc, u32* o_moduleOffset)
{
    M3CodeMappingPage* mapping = i_page->info.mapping;

    u32 pcOffset = (u32)(i_pc - mapping->basePC);

    u32 डावा  = 0;
    u32 उजवा = mapping->size;

    पर्यंत (डावा < उजवा) {
        u32 mid = डावा + (उजवा - डावा) / 2;

        जर (mapping->entries[mid].pcOffset < pcOffset) {
            डावा = mid + 1;
        } अन्यथा जर (mapping->entries[mid].pcOffset > pcOffset) {
            उजवा = mid;
        } अन्यथा {
            *o_moduleOffset = mapping->entries[mid].moduleOffset;
            परत true;
        }
    }

    // Getting here means left is now one more than the element we want.
    जर (डावा > 0) {
        डावा--;
        *o_moduleOffset = mapping->entries[डावा].moduleOffset;
        परत true;
    } अन्यथा {
        परत false;
    }
}
#endif // d_m3RecordBacktraces

//---------------------------------------------------------------------------------------------------------------------------------
