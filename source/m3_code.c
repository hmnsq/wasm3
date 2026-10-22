//
//  m3_code.c
//
//  Created by Steven Massey on 4/19/19.
//  Copyright © 2019 Steven Massey. All rights reserved.
//

#অন্তর্ভাব <limits.h>
#অন্তর্ভাব "m3_code.h"
#অন্তর্ভাব "m3_env.h"

//---------------------------------------------------------------------------------------------------------------------------------


IM3CodePage NewCodePage (IM3Runtime i_runtime, u32 i_min_অঙ্ক_সারিগুলি)
{
    IM3CodePage page;

    // check multiplication overflow
    যদি (i_min_অঙ্ক_সারিগুলি > UINT_MAX / আকার(code_t)) {
        ফেরত ফাঁকা;
    }
    u32 page_আকার = আকার(M3CodePageHeader) + আকার(code_t) * i_min_অঙ্ক_সারিগুলি;

    // check addition overflow
    যদি (page_আকার < আকার(M3CodePageHeader)) {
        ফেরত ফাঁকা;
    }

    page_আকার = (page_আকার + (d_m3_Code_Page_Align_আকার - 1)) & ~(d_m3_Code_Page_Align_আকার - 1); // align
    // check alignment overflow
    যদি (page_আকার == 0) {
        ফেরত ফাঁকা;
    }

    page = (IM3CodePage)m3_Malloc("M3CodePage", page_আকার);

    যদি (page) {
        page->info.sequence = ++i_runtime->newCodePageSequence;
        page->info.অঙ্ক_সারিগুলি = (page_আকার - আকার(M3CodePageHeader)) / আকার(code_t);

#যদি d_m3RecordBacktraces
        u32 page_আকার_Bt     = আকার(M3CodeMappingPage) + আকার(M3CodeMapEntry) * page->info.অঙ্ক_সারিগুলি;
        page->info.mapping = (M3CodeMappingPage*)m3_Malloc("M3CodeMappingPage", page_আকার_Bt);

        যদি (page->info.mapping) {
            page->info.mapping->আকার_2     = 0;
            page->info.mapping->capacity = page->info.অঙ্ক_সারিগুলি;
        } অন্যথা {
            m3_Free(page);
            ফেরত ফাঁকা;
        }
        page->info.mapping->basePC = GetPageStartPC(page);
#endif // d_m3RecordBacktraces

        m3log(runtime, "new page: %p; seq: %d; bytes: %d; lines: %d", GetPagePC(page), page->info.sequence, page_আকার, page->info.অঙ্ক_সারিগুলি);
    }

    ফেরত page;
}


শূন্য FreeCodePages (IM3CodePage* io_list)
{
    IM3CodePage page = *io_list;

    যখন (page) {
        m3log(code, "free page: %d; %p; util: %3.1f%%", page->info.sequence, page, 100. * page->info.সারি_সূচক / page->info.অঙ্ক_সারিগুলি);

        IM3CodePage পরবর্তী = page->info.পরবর্তী;
#যদি d_m3RecordBacktraces
        m3_Free(page->info.mapping);
#endif // d_m3RecordBacktraces
        m3_Free(page);
        page = পরবর্তী;
    }

    *io_list = ফাঁকা;
}


u32 অঙ্ক_Free_সারিগুলি (IM3CodePage i_page)
{
    d_m3Assert(i_page->info.সারি_সূচক <= i_page->info.অঙ্ক_সারিগুলি);

    ফেরত i_page->info.অঙ্ক_সারিগুলি - i_page->info.সারি_সূচক;
}


শূন্য EmitWord_impl (IM3CodePage i_page, শূন্য* i_word)
{                                                                       d_m3Assert (i_page->info.সারি_সূচক+1 <= i_page->info.অঙ্ক_সারিগুলি);
    i_page->code[i_page->info.সারি_সূচক++] = i_word;
}

শূন্য EmitWord32 (IM3CodePage i_page, ধ্রুবক u32 i_word)
{                                                                       d_m3Assert (i_page->info.সারি_সূচক+1 <= i_page->info.অঙ্ক_সারিগুলি);
    memcpy(&i_page->code[i_page->info.সারি_সূচক++], &i_word, আকার(i_word));
}

শূন্য EmitWord64 (IM3CodePage i_page, ধ্রুবক u64 i_word)
{
#যদি M3_SIZEOF_PTR == 4
                                                                        d_m3Assert (i_page->info.সারি_সূচক+2 <= i_page->info.অঙ্ক_সারিগুলি);
    memcpy(&i_page->code[i_page->info.সারি_সূচক], &i_word, আকার(i_word));
    i_page->info.সারি_সূচক += 2;
#অন্যথা
                                                                        d_m3Assert (i_page->info.সারি_সূচক+1 <= i_page->info.অঙ্ক_সারিগুলি);
    memcpy(&i_page->code[i_page->info.সারি_সূচক], &i_word, আকার(i_word));
    i_page->info.সারি_সূচক += 1;
#endif
}


#যদি d_m3RecordBacktraces
শূন্য EmitMappingEntry (IM3CodePage i_page, u32 i_moduleOffset)
{
    M3CodeMappingPage* page = i_page->info.mapping;
                                                                        d_m3Assert (page->আকার_2 < page->capacity);

    M3CodeMapEntry* entry = &page->entries[page->আকার_2++];
    pc_t            pc    = GetPagePC(i_page);

    entry->pcOffset     = (u32)(pc - page->basePC);
    entry->moduleOffset = i_moduleOffset;
}
#endif // d_m3RecordBacktraces

pc_t GetPageStartPC (IM3CodePage i_page)
{
    ফেরত &i_page->code[0];
}


pc_t GetPagePC (IM3CodePage i_page)
{
    যদি (i_page) {
        ফেরত &i_page->code[i_page->info.সারি_সূচক];
    } অন্যথা {
        ফেরত ফাঁকা;
    }
}


শূন্য ঠেলো_Code_Page (IM3CodePage* i_list, IM3CodePage i_codePage)
{
    IM3CodePage পরবর্তী      = *i_list;
    i_codePage->info.পরবর্তী = পরবর্তী;
    *i_list               = i_codePage;
}


IM3CodePage বের_করো_Code_Page (IM3CodePage* i_list)
{
    IM3CodePage page = *i_list;
    *i_list          = page->info.পরবর্তী;
    page->info.পরবর্তী  = ফাঁকা;

    ফেরত page;
}


u32 FindCodePageEnd (IM3CodePage i_list, IM3CodePage* o_end)
{
    u32 অঙ্ক_Pages = 0;
    *o_end       = ফাঁকা;

    যখন (i_list) {
        *o_end = i_list;
        ++অঙ্ক_Pages;
        i_list = i_list->info.পরবর্তী;
    }

    ফেরত অঙ্ক_Pages;
}


u32 গুনতি_Code_Pages (IM3CodePage i_list)
{
    IM3CodePage unused;
    ফেরত FindCodePageEnd(i_list, &unused);
}


IM3CodePage GetEndCodePage (IM3CodePage i_list)
{
    IM3CodePage end;
    FindCodePageEnd(i_list, &end);

    ফেরত end;
}

#যদি d_m3RecordBacktraces
bool ContainsPC (IM3CodePage i_page, pc_t i_pc)
{
    ফেরত GetPageStartPC(i_page) <= i_pc && i_pc < GetPagePC(i_page);
}


bool MapPCToOffset (IM3CodePage i_page, pc_t i_pc, u32* o_moduleOffset)
{
    M3CodeMappingPage* mapping = i_page->info.mapping;

    u32 pcOffset = (u32)(i_pc - mapping->basePC);

    u32 বাম  = 0;
    u32 ডান = mapping->আকার_2;

    যখন (বাম < ডান) {
        u32 mid = বাম + (ডান - বাম) / 2;

        যদি (mapping->entries[mid].pcOffset < pcOffset) {
            বাম = mid + 1;
        } অন্যথা যদি (mapping->entries[mid].pcOffset > pcOffset) {
            ডান = mid;
        } অন্যথা {
            *o_moduleOffset = mapping->entries[mid].moduleOffset;
            ফেরত true;
        }
    }

    // Getting here means left is now one more than the element we want.
    যদি (বাম > 0) {
        বাম--;
        *o_moduleOffset = mapping->entries[বাম].moduleOffset;
        ফেরত true;
    } অন্যথা {
        ফেরত false;
    }
}
#endif // d_m3RecordBacktraces

//---------------------------------------------------------------------------------------------------------------------------------
