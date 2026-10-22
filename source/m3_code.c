//
//  m3_code.c
//
//  Created by Steven Massey on 4/19/19.
//  Copyright © 2019 Steven Massey. All rights reserved.
//

#समावेश <limits.h>
#समावेश "m3_code.h"
#समावेश "m3_env.h"

//---------------------------------------------------------------------------------------------------------------------------------


IM3CodePage NewCodePage (IM3Runtime i_runtime, u32 i_min_अंक_पंक्तियाँ)
{
    IM3CodePage page;

    // check multiplication overflow
    अगर (i_min_अंक_पंक्तियाँ > अच_पूर्णांक_उच्च / माप(code_t)) {
        वापस शून्य;
    }
    u32 page_आकार = माप(M3CodePageHeader) + माप(code_t) * i_min_अंक_पंक्तियाँ;

    // check addition overflow
    अगर (page_आकार < माप(M3CodePageHeader)) {
        वापस शून्य;
    }

    page_आकार = (page_आकार + (d_m3_Code_Page_Align_आकार - 1)) & ~(d_m3_Code_Page_Align_आकार - 1); // align
    // check alignment overflow
    अगर (page_आकार == 0) {
        वापस शून्य;
    }

    page = (IM3CodePage)m3_Malloc("M3CodePage", page_आकार);

    अगर (page) {
        page->info.sequence = ++i_runtime->newCodePageSequence;
        page->info.अंक_पंक्तियाँ = (page_आकार - माप(M3CodePageHeader)) / माप(code_t);

#अगर d_m3RecordBacktraces
        u32 page_आकार_Bt     = माप(M3CodeMappingPage) + माप(M3CodeMapEntry) * page->info.अंक_पंक्तियाँ;
        page->info.mapping = (M3CodeMappingPage*)m3_Malloc("M3CodeMappingPage", page_आकार_Bt);

        अगर (page->info.mapping) {
            page->info.mapping->आकार     = 0;
            page->info.mapping->capacity = page->info.अंक_पंक्तियाँ;
        } अन्यथा {
            m3_Free(page);
            वापस शून्य;
        }
        page->info.mapping->basePC = GetPageStartPC(page);
#पूर्ण_अगर // d_m3RecordBacktraces

        m3log(runtime, "new page: %p; seq: %d; bytes: %d; lines: %d", GetPagePC(page), page->info.sequence, page_आकार, page->info.अंक_पंक्तियाँ);
    }

    वापस page;
}


व्योम FreeCodePages (IM3CodePage* io_list)
{
    IM3CodePage page = *io_list;

    जबतक (page) {
        m3log(code, "free page: %d; %p; util: %3.1f%%", page->info.sequence, page, 100. * page->info.पंक्ति_सूचकांक / page->info.अंक_पंक्तियाँ);

        IM3CodePage अग्र = page->info.अग्र;
#अगर d_m3RecordBacktraces
        m3_Free(page->info.mapping);
#पूर्ण_अगर // d_m3RecordBacktraces
        m3_Free(page);
        page = अग्र;
    }

    *io_list = शून्य;
}


u32 अंक_Free_पंक्तियाँ (IM3CodePage i_page)
{
    d_m3Assert(i_page->info.पंक्ति_सूचकांक <= i_page->info.अंक_पंक्तियाँ);

    वापस i_page->info.अंक_पंक्तियाँ - i_page->info.पंक्ति_सूचकांक;
}


व्योम EmitWord_impl (IM3CodePage i_page, व्योम* i_word)
{                                                                       d_m3Assert (i_page->info.पंक्ति_सूचकांक+1 <= i_page->info.अंक_पंक्तियाँ);
    i_page->code[i_page->info.पंक्ति_सूचकांक++] = i_word;
}

व्योम EmitWord32 (IM3CodePage i_page, स्थिर u32 i_word)
{                                                                       d_m3Assert (i_page->info.पंक्ति_सूचकांक+1 <= i_page->info.अंक_पंक्तियाँ);
    स_नकल(&i_page->code[i_page->info.पंक्ति_सूचकांक++], &i_word, माप(i_word));
}

व्योम EmitWord64 (IM3CodePage i_page, स्थिर u64 i_word)
{
#अगर M3_SIZEOF_PTR == 4
                                                                        d_m3Assert (i_page->info.पंक्ति_सूचकांक+2 <= i_page->info.अंक_पंक्तियाँ);
    स_नकल(&i_page->code[i_page->info.पंक्ति_सूचकांक], &i_word, माप(i_word));
    i_page->info.पंक्ति_सूचकांक += 2;
#अन्यथा
                                                                        d_m3Assert (i_page->info.पंक्ति_सूचकांक+1 <= i_page->info.अंक_पंक्तियाँ);
    स_नकल(&i_page->code[i_page->info.पंक्ति_सूचकांक], &i_word, माप(i_word));
    i_page->info.पंक्ति_सूचकांक += 1;
#पूर्ण_अगर
}


#अगर d_m3RecordBacktraces
व्योम EmitMappingEntry (IM3CodePage i_page, u32 i_moduleOffset)
{
    M3CodeMappingPage* page = i_page->info.mapping;
                                                                        d_m3Assert (page->आकार < page->capacity);

    M3CodeMapEntry* entry = &page->entries[page->आकार++];
    pc_t            pc    = GetPagePC(i_page);

    entry->pcOffset     = (u32)(pc - page->basePC);
    entry->moduleOffset = i_moduleOffset;
}
#पूर्ण_अगर // d_m3RecordBacktraces

pc_t GetPageStartPC (IM3CodePage i_page)
{
    वापस &i_page->code[0];
}


pc_t GetPagePC (IM3CodePage i_page)
{
    अगर (i_page) {
        वापस &i_page->code[i_page->info.पंक्ति_सूचकांक];
    } अन्यथा {
        वापस शून्य;
    }
}


व्योम धकेलो_Code_Page (IM3CodePage* i_list, IM3CodePage i_codePage)
{
    IM3CodePage अग्र      = *i_list;
    i_codePage->info.अग्र = अग्र;
    *i_list               = i_codePage;
}


IM3CodePage निकालो_Code_Page (IM3CodePage* i_list)
{
    IM3CodePage page = *i_list;
    *i_list          = page->info.अग्र;
    page->info.अग्र  = शून्य;

    वापस page;
}


u32 FindCodePageEnd (IM3CodePage i_list, IM3CodePage* o_end)
{
    u32 अंक_Pages = 0;
    *o_end       = शून्य;

    जबतक (i_list) {
        *o_end = i_list;
        ++अंक_Pages;
        i_list = i_list->info.अग्र;
    }

    वापस अंक_Pages;
}


u32 गिनती_Code_Pages (IM3CodePage i_list)
{
    IM3CodePage unused;
    वापस FindCodePageEnd(i_list, &unused);
}


IM3CodePage GetEndCodePage (IM3CodePage i_list)
{
    IM3CodePage end;
    FindCodePageEnd(i_list, &end);

    वापस end;
}

#अगर d_m3RecordBacktraces
bool ContainsPC (IM3CodePage i_page, pc_t i_pc)
{
    वापस GetPageStartPC(i_page) <= i_pc && i_pc < GetPagePC(i_page);
}


bool MapPCToOffset (IM3CodePage i_page, pc_t i_pc, u32* o_moduleOffset)
{
    M3CodeMappingPage* mapping = i_page->info.mapping;

    u32 pcOffset = (u32)(i_pc - mapping->basePC);

    u32 बायाँ  = 0;
    u32 दायाँ = mapping->आकार;

    जबतक (बायाँ < दायाँ) {
        u32 mid = बायाँ + (दायाँ - बायाँ) / 2;

        अगर (mapping->entries[mid].pcOffset < pcOffset) {
            बायाँ = mid + 1;
        } अन्यथा अगर (mapping->entries[mid].pcOffset > pcOffset) {
            दायाँ = mid;
        } अन्यथा {
            *o_moduleOffset = mapping->entries[mid].moduleOffset;
            वापस true;
        }
    }

    // Getting here means left is now one more than the element we want.
    अगर (बायाँ > 0) {
        बायाँ--;
        *o_moduleOffset = mapping->entries[बायाँ].moduleOffset;
        वापस true;
    } अन्यथा {
        वापस false;
    }
}
#पूर्ण_अगर // d_m3RecordBacktraces

//---------------------------------------------------------------------------------------------------------------------------------
