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
    যদি (i_min_অঙ্ক_সারিগুলি > UINT_MAX / পরিমাণ(code_t)) {
        প্রত্যাগম রিক্ত;
    }
    u32 pageSize = পরিমাণ(M3CodePageHeader) + পরিমাণ(code_t) * i_min_অঙ্ক_সারিগুলি;

    // check addition overflow
    যদি (pageSize < পরিমাণ(M3CodePageHeader)) {
        প্রত্যাগম রিক্ত;
    }

    pageSize = (pageSize + (d_m3CodePageAlignSize - 1)) & ~(d_m3CodePageAlignSize - 1); // align
    // check alignment overflow
    যদি (pageSize == 0) {
        প্রত্যাগম রিক্ত;
    }

    page = (IM3CodePage)m3_Malloc("M3CodePage", pageSize);

    যদি (page) {
        page->info.sequence = ++i_runtime->newCodePageSequence;
        page->info.অঙ্ক_সারিগুলি = (pageSize - পরিমাণ(M3CodePageHeader)) / পরিমাণ(code_t);

#যদি d_m3RecordBacktraces
        u32 pageSizeBt     = পরিমাণ(M3CodeMappingPage) + পরিমাণ(M3CodeMapEntry) * page->info.অঙ্ক_সারিগুলি;
        page->info.mapping = (M3CodeMappingPage*)m3_Malloc("M3CodeMappingPage", pageSizeBt);

        যদি (page->info.mapping) {
            page->info.mapping->size     = 0;
            page->info.mapping->capacity = page->info.অঙ্ক_সারিগুলি;
        } অন্যথা {
            m3_Free(page);
            প্রত্যাগম রিক্ত;
        }
        page->info.mapping->basePC = GetPageStartPC(page);
#endif // d_m3RecordBacktraces

        m3log(runtime, "new page: %p; seq: %d; bytes: %d; lines: %d", GetPagePC(page), page->info.sequence, pageSize, page->info.অঙ্ক_সারিগুলি);
    }

    প্রত্যাগম page;
}


শূন্য FreeCodePages (IM3CodePage* io_list)
{
    IM3CodePage page = *io_list;

    যাবত্ (page) {
        m3log(code, "free page: %d; %p; util: %3.1f%%", page->info.sequence, page, 100. * page->info.সারি_Index / page->info.অঙ্ক_সারিগুলি);

        IM3CodePage next = page->info.next;
#যদি d_m3RecordBacktraces
        m3_Free(page->info.mapping);
#endif // d_m3RecordBacktraces
        m3_Free(page);
        page = next;
    }

    *io_list = রিক্ত;
}


u32 অঙ্ক_Free_সারিগুলি (IM3CodePage i_page)
{
    d_m3Assert(i_page->info.সারি_Index <= i_page->info.অঙ্ক_সারিগুলি);

    প্রত্যাগম i_page->info.অঙ্ক_সারিগুলি - i_page->info.সারি_Index;
}


শূন্য EmitWord_impl (IM3CodePage i_page, শূন্য* i_word)
{                                                                       d_m3Assert (i_page->info.সারি_Index+1 <= i_page->info.অঙ্ক_সারিগুলি);
    i_page->code[i_page->info.সারি_Index++] = i_word;
}

শূন্য EmitWord32 (IM3CodePage i_page, ধ্রুব u32 i_word)
{                                                                       d_m3Assert (i_page->info.সারি_Index+1 <= i_page->info.অঙ্ক_সারিগুলি);
    memcpy(&i_page->code[i_page->info.সারি_Index++], &i_word, পরিমাণ(i_word));
}

শূন্য EmitWord64 (IM3CodePage i_page, ধ্রুব u64 i_word)
{
#যদি M3_SIZEOF_PTR == 4
                                                                        d_m3Assert (i_page->info.সারি_Index+2 <= i_page->info.অঙ্ক_সারিগুলি);
    memcpy(&i_page->code[i_page->info.সারি_Index], &i_word, পরিমাণ(i_word));
    i_page->info.সারি_Index += 2;
#অন্যথা
                                                                        d_m3Assert (i_page->info.সারি_Index+1 <= i_page->info.অঙ্ক_সারিগুলি);
    memcpy(&i_page->code[i_page->info.সারি_Index], &i_word, পরিমাণ(i_word));
    i_page->info.সারি_Index += 1;
#endif
}


#যদি d_m3RecordBacktraces
শূন্য EmitMappingEntry (IM3CodePage i_page, u32 i_moduleOffset)
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
    প্রত্যাগম &i_page->code[0];
}


pc_t GetPagePC (IM3CodePage i_page)
{
    যদি (i_page) {
        প্রত্যাগম &i_page->code[i_page->info.সারি_Index];
    } অন্যথা {
        প্রত্যাগম রিক্ত;
    }
}


শূন্য ঠেলো_Code_Page (IM3CodePage* i_list, IM3CodePage i_codePage)
{
    IM3CodePage next      = *i_list;
    i_codePage->info.next = next;
    *i_list               = i_codePage;
}


IM3CodePage বের_করো_Code_Page (IM3CodePage* i_list)
{
    IM3CodePage page = *i_list;
    *i_list          = page->info.next;
    page->info.next  = রিক্ত;

    প্রত্যাগম page;
}


u32 FindCodePageEnd (IM3CodePage i_list, IM3CodePage* o_end)
{
    u32 অঙ্ক_Pages = 0;
    *o_end       = রিক্ত;

    যাবত্ (i_list) {
        *o_end = i_list;
        ++অঙ্ক_Pages;
        i_list = i_list->info.next;
    }

    প্রত্যাগম অঙ্ক_Pages;
}


u32 গুনতি_Code_Pages (IM3CodePage i_list)
{
    IM3CodePage unused;
    প্রত্যাগম FindCodePageEnd(i_list, &unused);
}


IM3CodePage GetEndCodePage (IM3CodePage i_list)
{
    IM3CodePage end;
    FindCodePageEnd(i_list, &end);

    প্রত্যাগম end;
}

#যদি d_m3RecordBacktraces
bool ContainsPC (IM3CodePage i_page, pc_t i_pc)
{
    প্রত্যাগম GetPageStartPC(i_page) <= i_pc && i_pc < GetPagePC(i_page);
}


bool MapPCToOffset (IM3CodePage i_page, pc_t i_pc, u32* o_moduleOffset)
{
    M3CodeMappingPage* mapping = i_page->info.mapping;

    u32 pcOffset = (u32)(i_pc - mapping->basePC);

    u32 বাম  = 0;
    u32 ডান = mapping->size;

    যাবত্ (বাম < ডান) {
        u32 mid = বাম + (ডান - বাম) / 2;

        যদি (mapping->entries[mid].pcOffset < pcOffset) {
            বাম = mid + 1;
        } অন্যথা যদি (mapping->entries[mid].pcOffset > pcOffset) {
            ডান = mid;
        } অন্যথা {
            *o_moduleOffset = mapping->entries[mid].moduleOffset;
            প্রত্যাগম true;
        }
    }

    // Getting here means left is now one more than the element we want.
    যদি (বাম > 0) {
        বাম--;
        *o_moduleOffset = mapping->entries[বাম].moduleOffset;
        প্রত্যাগম true;
    } অন্যথা {
        প্রত্যাগম false;
    }
}
#endif // d_m3RecordBacktraces

//---------------------------------------------------------------------------------------------------------------------------------
