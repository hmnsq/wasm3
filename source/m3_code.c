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


IM3_संहिता_Page New_संहिता_Page (IM3Runtime i_runtime, u32 i_min_अङ्कः_पङ्क्तयः)
{
    IM3_संहिता_Page page;

    // check multiplication overflow
    यदि (i_min_अङ्कः_पङ्क्तयः > UINT_MAX / परिमाण(संहिता_t)) {
        प्रत्यागम रिक्त;
    }
    u32 page_आकार = परिमाण(M3_संहिता_Page_शीर्षकम्) + परिमाण(संहिता_t) * i_min_अङ्कः_पङ्क्तयः;

    // check addition overflow
    यदि (page_आकार < परिमाण(M3_संहिता_Page_शीर्षकम्)) {
        प्रत्यागम रिक्त;
    }

    page_आकार = (page_आकार + (d_m3_संहिता_Page_Align_आकार - 1)) & ~(d_m3_संहिता_Page_Align_आकार - 1); // align
    // check alignment overflow
    यदि (page_आकार == 0) {
        प्रत्यागम रिक्त;
    }

    page = (IM3_संहिता_Page)m3_Malloc("M3CodePage", page_आकार);

    यदि (page) {
        page->info.sequence = ++i_runtime->new_संहिता_Page_Sequence;
        page->info.अङ्कः_पङ्क्तयः = (page_आकार - परिमाण(M3_संहिता_Page_शीर्षकम्)) / परिमाण(संहिता_t);

#यदि d_m3_लेखः_Backtraces
        u32 page_आकार_Bt     = परिमाण(M3_संहिता_Mapping_Page) + परिमाण(M3_संहिता_Map_प्रविष्टिः) * page->info.अङ्कः_पङ्क्तयः;
        page->info.mapping = (M3_संहिता_Mapping_Page*)m3_Malloc("M3CodeMappingPage", page_आकार_Bt);

        यदि (page->info.mapping) {
            page->info.mapping->आकार     = 0;
            page->info.mapping->क्षमता = page->info.अङ्कः_पङ्क्तयः;
        } अन्यथा {
            m3_Free(page);
            प्रत्यागम रिक्त;
        }
        page->info.mapping->आधारः_PC = GetPageStartPC(page);
#यदि_अन्तः // d_m3RecordBacktraces

        m3log(runtime, "new page: %p; seq: %d; bytes: %d; lines: %d", GetPagePC(page), page->info.sequence, page_आकार, page->info.अङ्कः_पङ्क्तयः);
    }

    प्रत्यागम page;
}


शून्य Free_संहिता_पृष्ठानि (IM3_संहिता_Page* io_list)
{
    IM3_संहिता_Page page = *io_list;

    यावत् (page) {
        m3log(संहिता, "free page: %d; %p; util: %3.1f%%", page->info.sequence, page, 100. * page->info.पङ्क्तिः_सूचकांक / page->info.अङ्कः_पङ्क्तयः);

        IM3_संहिता_Page अग्र = page->info.अग्र;
#यदि d_m3_लेखः_Backtraces
        m3_Free(page->info.mapping);
#यदि_अन्तः // d_m3RecordBacktraces
        m3_Free(page);
        page = अग्र;
    }

    *io_list = रिक्त;
}


u32 अङ्कः_Free_पङ्क्तयः (IM3_संहिता_Page i_page)
{
    d_m3Assert(i_page->info.पङ्क्तिः_सूचकांक <= i_page->info.अङ्कः_पङ्क्तयः);

    प्रत्यागम i_page->info.अङ्कः_पङ्क्तयः - i_page->info.पङ्क्तिः_सूचकांक;
}


शून्य EmitWord_impl (IM3_संहिता_Page i_page, शून्य* i_word)
{                                                                       d_m3Assert (i_page->info.पङ्क्तिः_सूचकांक+1 <= i_page->info.अङ्कः_पङ्क्तयः);
    i_page->संहिता[i_page->info.पङ्क्तिः_सूचकांक++] = i_word;
}

शून्य EmitWord32 (IM3_संहिता_Page i_page, ध्रुव u32 i_word)
{                                                                       d_m3Assert (i_page->info.पङ्क्तिः_सूचकांक+1 <= i_page->info.अङ्कः_पङ्क्तयः);
    स्मृतिं_प्रतिलिख(&i_page->संहिता[i_page->info.पङ्क्तिः_सूचकांक++], &i_word, परिमाण(i_word));
}

शून्य EmitWord64 (IM3_संहिता_Page i_page, ध्रुव u64 i_word)
{
#यदि M3_SIZEOF_PTR == 4
                                                                        d_m3Assert (i_page->info.पङ्क्तिः_सूचकांक+2 <= i_page->info.अङ्कः_पङ्क्तयः);
    स्मृतिं_प्रतिलिख(&i_page->संहिता[i_page->info.पङ्क्तिः_सूचकांक], &i_word, परिमाण(i_word));
    i_page->info.पङ्क्तिः_सूचकांक += 2;
#अन्यथा
                                                                        d_m3Assert (i_page->info.पङ्क्तिः_सूचकांक+1 <= i_page->info.अङ्कः_पङ्क्तयः);
    स्मृतिं_प्रतिलिख(&i_page->संहिता[i_page->info.पङ्क्तिः_सूचकांक], &i_word, परिमाण(i_word));
    i_page->info.पङ्क्तिः_सूचकांक += 1;
#यदि_अन्तः
}


#यदि d_m3_लेखः_Backtraces
शून्य Emit_Mapping_प्रविष्टिः (IM3_संहिता_Page i_page, u32 i_module_विस्थापनम्)
{
    M3_संहिता_Mapping_Page* page = i_page->info.mapping;
                                                                        d_m3Assert (page->आकार < page->क्षमता);

    M3_संहिता_Map_प्रविष्टिः* प्रविष्टिः = &page->entries[page->आकार++];
    pc_t            pc    = GetPagePC(i_page);

    प्रविष्टिः->pc_विस्थापनम्     = (u32)(pc - page->आधारः_PC);
    प्रविष्टिः->module_विस्थापनम् = i_module_विस्थापनम्;
}
#यदि_अन्तः // d_m3RecordBacktraces

pc_t GetPageStartPC (IM3_संहिता_Page i_page)
{
    प्रत्यागम &i_page->संहिता[0];
}


pc_t GetPagePC (IM3_संहिता_Page i_page)
{
    यदि (i_page) {
        प्रत्यागम &i_page->संहिता[i_page->info.पङ्क्तिः_सूचकांक];
    } अन्यथा {
        प्रत्यागम रिक्त;
    }
}


शून्य प्रक्षिप_संहिता_Page (IM3_संहिता_Page* i_list, IM3_संहिता_Page i_संहिता_Page)
{
    IM3_संहिता_Page अग्र      = *i_list;
    i_संहिता_Page->info.अग्र = अग्र;
    *i_list               = i_संहिता_Page;
}


IM3_संहिता_Page निष्कासय_संहिता_Page (IM3_संहिता_Page* i_list)
{
    IM3_संहिता_Page page = *i_list;
    *i_list          = page->info.अग्र;
    page->info.अग्र  = रिक्त;

    प्रत्यागम page;
}


u32 Find_संहिता_Page_अन्तः (IM3_संहिता_Page i_list, IM3_संहिता_Page* o_अन्तः)
{
    u32 अङ्कः_पृष्ठानि = 0;
    *o_अन्तः       = रिक्त;

    यावत् (i_list) {
        *o_अन्तः = i_list;
        ++अङ्कः_पृष्ठानि;
        i_list = i_list->info.अग्र;
    }

    प्रत्यागम अङ्कः_पृष्ठानि;
}


u32 गणनम्_संहिता_पृष्ठानि (IM3_संहिता_Page i_list)
{
    IM3_संहिता_Page अप्रयुक्तम्;
    प्रत्यागम Find_संहिता_Page_अन्तः(i_list, &अप्रयुक्तम्);
}


IM3_संहिता_Page Get_अन्तः_संहिता_Page (IM3_संहिता_Page i_list)
{
    IM3_संहिता_Page अन्तः;
    Find_संहिता_Page_अन्तः(i_list, &अन्तः);

    प्रत्यागम अन्तः;
}

#यदि d_m3_लेखः_Backtraces
bool अन्तर्भवति_PC (IM3_संहिता_Page i_page, pc_t i_pc)
{
    प्रत्यागम GetPageStartPC(i_page) <= i_pc && i_pc < GetPagePC(i_page);
}


bool Map_PCTo_विस्थापनम् (IM3_संहिता_Page i_page, pc_t i_pc, u32* o_module_विस्थापनम्)
{
    M3_संहिता_Mapping_Page* mapping = i_page->info.mapping;

    u32 pc_विस्थापनम् = (u32)(i_pc - mapping->आधारः_PC);

    u32 वामः  = 0;
    u32 दक्षिणः = mapping->आकार;

    यावत् (वामः < दक्षिणः) {
        u32 mid = वामः + (दक्षिणः - वामः) / 2;

        यदि (mapping->entries[mid].pc_विस्थापनम् < pc_विस्थापनम्) {
            वामः = mid + 1;
        } अन्यथा यदि (mapping->entries[mid].pc_विस्थापनम् > pc_विस्थापनम्) {
            दक्षिणः = mid;
        } अन्यथा {
            *o_module_विस्थापनम् = mapping->entries[mid].module_विस्थापनम्;
            प्रत्यागम true;
        }
    }

    // Getting here means left is now one more than the element we want.
    यदि (वामः > 0) {
        वामः--;
        *o_module_विस्थापनम् = mapping->entries[वामः].module_विस्थापनम्;
        प्रत्यागम true;
    } अन्यथा {
        प्रत्यागम false;
    }
}
#यदि_अन्तः // d_m3RecordBacktraces

//---------------------------------------------------------------------------------------------------------------------------------
