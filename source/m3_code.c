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


IM3CodePage NewCodePage (IM3Runtime i_runtime, u32 i_min_अङ्क_पङ्क्तिहरू)
{
    IM3CodePage page;

    // check multiplication overflow
    यदि (i_min_अङ्क_पङ्क्तिहरू > अच_पूर्णांक_उच्च / परिमाण(code_t)) {
        फर्काऊ रिक्त;
    }
    u32 pageSize = परिमाण(M3CodePageHeader) + परिमाण(code_t) * i_min_अङ्क_पङ्क्तिहरू;

    // check addition overflow
    यदि (pageSize < परिमाण(M3CodePageHeader)) {
        फर्काऊ रिक्त;
    }

    pageSize = (pageSize + (d_m3CodePageAlignSize - 1)) & ~(d_m3CodePageAlignSize - 1); // align
    // check alignment overflow
    यदि (pageSize == 0) {
        फर्काऊ रिक्त;
    }

    page = (IM3CodePage)m3_Malloc("M3CodePage", pageSize);

    यदि (page) {
        page->info.sequence = ++i_runtime->newCodePageSequence;
        page->info.अङ्क_पङ्क्तिहरू = (pageSize - परिमाण(M3CodePageHeader)) / परिमाण(code_t);

#यदि d_m3RecordBacktraces
        u32 pageSizeBt     = परिमाण(M3CodeMappingPage) + परिमाण(M3CodeMapEntry) * page->info.अङ्क_पङ्क्तिहरू;
        page->info.mapping = (M3CodeMappingPage*)m3_Malloc("M3CodeMappingPage", pageSizeBt);

        यदि (page->info.mapping) {
            page->info.mapping->size     = 0;
            page->info.mapping->capacity = page->info.अङ्क_पङ्क्तिहरू;
        } अन्यथा {
            m3_Free(page);
            फर्काऊ रिक्त;
        }
        page->info.mapping->basePC = GetPageStartPC(page);
#endif // d_m3RecordBacktraces

        m3log(runtime, "new page: %p; seq: %d; bytes: %d; lines: %d", GetPagePC(page), page->info.sequence, pageSize, page->info.अङ्क_पङ्क्तिहरू);
    }

    फर्काऊ page;
}


शून्य FreeCodePages (IM3CodePage* io_list)
{
    IM3CodePage page = *io_list;

    जबसम्म (page) {
        m3log(code, "free page: %d; %p; util: %3.1f%%", page->info.sequence, page, 100. * page->info.पङ्क्ति_Index / page->info.अङ्क_पङ्क्तिहरू);

        IM3CodePage next = page->info.next;
#यदि d_m3RecordBacktraces
        m3_Free(page->info.mapping);
#endif // d_m3RecordBacktraces
        m3_Free(page);
        page = next;
    }

    *io_list = रिक्त;
}


u32 अङ्क_Free_पङ्क्तिहरू (IM3CodePage i_page)
{
    d_m3Assert(i_page->info.पङ्क्ति_Index <= i_page->info.अङ्क_पङ्क्तिहरू);

    फर्काऊ i_page->info.अङ्क_पङ्क्तिहरू - i_page->info.पङ्क्ति_Index;
}


शून्य EmitWord_impl (IM3CodePage i_page, शून्य* i_word)
{                                                                       d_m3Assert (i_page->info.पङ्क्ति_Index+1 <= i_page->info.अङ्क_पङ्क्तिहरू);
    i_page->code[i_page->info.पङ्क्ति_Index++] = i_word;
}

शून्य EmitWord32 (IM3CodePage i_page, अचर u32 i_word)
{                                                                       d_m3Assert (i_page->info.पङ्क्ति_Index+1 <= i_page->info.अङ्क_पङ्क्तिहरू);
    स_नकल(&i_page->code[i_page->info.पङ्क्ति_Index++], &i_word, परिमाण(i_word));
}

शून्य EmitWord64 (IM3CodePage i_page, अचर u64 i_word)
{
#यदि M3_SIZEOF_PTR == 4
                                                                        d_m3Assert (i_page->info.पङ्क्ति_Index+2 <= i_page->info.अङ्क_पङ्क्तिहरू);
    स_नकल(&i_page->code[i_page->info.पङ्क्ति_Index], &i_word, परिमाण(i_word));
    i_page->info.पङ्क्ति_Index += 2;
#अन्यथा
                                                                        d_m3Assert (i_page->info.पङ्क्ति_Index+1 <= i_page->info.अङ्क_पङ्क्तिहरू);
    स_नकल(&i_page->code[i_page->info.पङ्क्ति_Index], &i_word, परिमाण(i_word));
    i_page->info.पङ्क्ति_Index += 1;
#endif
}


#यदि d_m3RecordBacktraces
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
    फर्काऊ &i_page->code[0];
}


pc_t GetPagePC (IM3CodePage i_page)
{
    यदि (i_page) {
        फर्काऊ &i_page->code[i_page->info.पङ्क्ति_Index];
    } अन्यथा {
        फर्काऊ रिक्त;
    }
}


शून्य थप_Code_Page (IM3CodePage* i_list, IM3CodePage i_codePage)
{
    IM3CodePage next      = *i_list;
    i_codePage->info.next = next;
    *i_list               = i_codePage;
}


IM3CodePage झिक_Code_Page (IM3CodePage* i_list)
{
    IM3CodePage page = *i_list;
    *i_list          = page->info.next;
    page->info.next  = रिक्त;

    फर्काऊ page;
}


u32 FindCodePageEnd (IM3CodePage i_list, IM3CodePage* o_end)
{
    u32 अङ्क_Pages = 0;
    *o_end       = रिक्त;

    जबसम्म (i_list) {
        *o_end = i_list;
        ++अङ्क_Pages;
        i_list = i_list->info.next;
    }

    फर्काऊ अङ्क_Pages;
}


u32 गन्ती_Code_Pages (IM3CodePage i_list)
{
    IM3CodePage unused;
    फर्काऊ FindCodePageEnd(i_list, &unused);
}


IM3CodePage GetEndCodePage (IM3CodePage i_list)
{
    IM3CodePage end;
    FindCodePageEnd(i_list, &end);

    फर्काऊ end;
}

#यदि d_m3RecordBacktraces
bool ContainsPC (IM3CodePage i_page, pc_t i_pc)
{
    फर्काऊ GetPageStartPC(i_page) <= i_pc && i_pc < GetPagePC(i_page);
}


bool MapPCToOffset (IM3CodePage i_page, pc_t i_pc, u32* o_moduleOffset)
{
    M3CodeMappingPage* mapping = i_page->info.mapping;

    u32 pcOffset = (u32)(i_pc - mapping->basePC);

    u32 बायाँ  = 0;
    u32 दायाँ = mapping->size;

    जबसम्म (बायाँ < दायाँ) {
        u32 mid = बायाँ + (दायाँ - बायाँ) / 2;

        यदि (mapping->entries[mid].pcOffset < pcOffset) {
            बायाँ = mid + 1;
        } अन्यथा यदि (mapping->entries[mid].pcOffset > pcOffset) {
            दायाँ = mid;
        } अन्यथा {
            *o_moduleOffset = mapping->entries[mid].moduleOffset;
            फर्काऊ true;
        }
    }

    // Getting here means left is now one more than the element we want.
    यदि (बायाँ > 0) {
        बायाँ--;
        *o_moduleOffset = mapping->entries[बायाँ].moduleOffset;
        फर्काऊ true;
    } अन्यथा {
        फर्काऊ false;
    }
}
#endif // d_m3RecordBacktraces

//---------------------------------------------------------------------------------------------------------------------------------
