//
//  m3_code.c
//
//  Created by Steven Massey on 4/19/19.
//  Copyright © 2019 Steven Massey. All rights reserved.
//

#شامل <limits.h>
#شامل "m3_code.h"
#شامل "m3_env.h"

//---------------------------------------------------------------------------------------------------------------------------------


IM3CodePage NewCodePage (IM3Runtime i_runtime, u32 i_min_رقم_سطرها)
{
    IM3CodePage page;

    // check multiplication overflow
    اگر (i_min_رقم_سطرها > UINT_MAX / اندازه(code_t)) {
        بازگشت پوچ;
    }
    u32 pageSize = اندازه(M3CodePageHeader) + اندازه(code_t) * i_min_رقم_سطرها;

    // check addition overflow
    اگر (pageSize < اندازه(M3CodePageHeader)) {
        بازگشت پوچ;
    }

    pageSize = (pageSize + (d_m3CodePageAlignSize - 1)) & ~(d_m3CodePageAlignSize - 1); // align
    // check alignment overflow
    اگر (pageSize == 0) {
        بازگشت پوچ;
    }

    page = (IM3CodePage)m3_Malloc("M3CodePage", pageSize);

    اگر (page) {
        page->info.sequence = ++i_runtime->newCodePageSequence;
        page->info.رقم_سطرها = (pageSize - اندازه(M3CodePageHeader)) / اندازه(code_t);

#اگر d_m3RecordBacktraces
        u32 pageSizeBt     = اندازه(M3CodeMappingPage) + اندازه(M3CodeMapEntry) * page->info.رقم_سطرها;
        page->info.mapping = (M3CodeMappingPage*)m3_Malloc("M3CodeMappingPage", pageSizeBt);

        اگر (page->info.mapping) {
            page->info.mapping->size     = 0;
            page->info.mapping->capacity = page->info.رقم_سطرها;
        } وگرنه {
            m3_Free(page);
            بازگشت پوچ;
        }
        page->info.mapping->basePC = GetPageStartPC(page);
#endif // d_m3RecordBacktraces

        m3log(runtime, "new page: %p; seq: %d; bytes: %d; lines: %d", GetPagePC(page), page->info.sequence, pageSize, page->info.رقم_سطرها);
    }

    بازگشت page;
}


تهی FreeCodePages (IM3CodePage* io_list)
{
    IM3CodePage page = *io_list;

    وقتی (page) {
        m3log(code, "free page: %d; %p; util: %3.1f%%", page->info.sequence, page, 100. * page->info.سطر_Index / page->info.رقم_سطرها);

        IM3CodePage next = page->info.next;
#اگر d_m3RecordBacktraces
        m3_Free(page->info.mapping);
#endif // d_m3RecordBacktraces
        m3_Free(page);
        page = next;
    }

    *io_list = پوچ;
}


u32 رقم_Free_سطرها (IM3CodePage i_page)
{
    d_m3Assert(i_page->info.سطر_Index <= i_page->info.رقم_سطرها);

    بازگشت i_page->info.رقم_سطرها - i_page->info.سطر_Index;
}


تهی EmitWord_impl (IM3CodePage i_page, تهی* i_word)
{                                                                       d_m3Assert (i_page->info.سطر_Index+1 <= i_page->info.رقم_سطرها);
    i_page->code[i_page->info.سطر_Index++] = i_word;
}

تهی EmitWord32 (IM3CodePage i_page, ثابت u32 i_word)
{                                                                       d_m3Assert (i_page->info.سطر_Index+1 <= i_page->info.رقم_سطرها);
    memcpy(&i_page->code[i_page->info.سطر_Index++], &i_word, اندازه(i_word));
}

تهی EmitWord64 (IM3CodePage i_page, ثابت u64 i_word)
{
#اگر M3_SIZEOF_PTR == 4
                                                                        d_m3Assert (i_page->info.سطر_Index+2 <= i_page->info.رقم_سطرها);
    memcpy(&i_page->code[i_page->info.سطر_Index], &i_word, اندازه(i_word));
    i_page->info.سطر_Index += 2;
#وگرنه
                                                                        d_m3Assert (i_page->info.سطر_Index+1 <= i_page->info.رقم_سطرها);
    memcpy(&i_page->code[i_page->info.سطر_Index], &i_word, اندازه(i_word));
    i_page->info.سطر_Index += 1;
#endif
}


#اگر d_m3RecordBacktraces
تهی EmitMappingEntry (IM3CodePage i_page, u32 i_moduleOffset)
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
    بازگشت &i_page->code[0];
}


pc_t GetPagePC (IM3CodePage i_page)
{
    اگر (i_page) {
        بازگشت &i_page->code[i_page->info.سطر_Index];
    } وگرنه {
        بازگشت پوچ;
    }
}


تهی افزودن_Code_Page (IM3CodePage* i_list, IM3CodePage i_codePage)
{
    IM3CodePage next      = *i_list;
    i_codePage->info.next = next;
    *i_list               = i_codePage;
}


IM3CodePage برداشت_Code_Page (IM3CodePage* i_list)
{
    IM3CodePage page = *i_list;
    *i_list          = page->info.next;
    page->info.next  = پوچ;

    بازگشت page;
}


u32 FindCodePageEnd (IM3CodePage i_list, IM3CodePage* o_end)
{
    u32 رقم_Pages = 0;
    *o_end       = پوچ;

    وقتی (i_list) {
        *o_end = i_list;
        ++رقم_Pages;
        i_list = i_list->info.next;
    }

    بازگشت رقم_Pages;
}


u32 شمار_Code_Pages (IM3CodePage i_list)
{
    IM3CodePage unused;
    بازگشت FindCodePageEnd(i_list, &unused);
}


IM3CodePage GetEndCodePage (IM3CodePage i_list)
{
    IM3CodePage end;
    FindCodePageEnd(i_list, &end);

    بازگشت end;
}

#اگر d_m3RecordBacktraces
bool ContainsPC (IM3CodePage i_page, pc_t i_pc)
{
    بازگشت GetPageStartPC(i_page) <= i_pc && i_pc < GetPagePC(i_page);
}


bool MapPCToOffset (IM3CodePage i_page, pc_t i_pc, u32* o_moduleOffset)
{
    M3CodeMappingPage* mapping = i_page->info.mapping;

    u32 pcOffset = (u32)(i_pc - mapping->basePC);

    u32 چپ  = 0;
    u32 راست = mapping->size;

    وقتی (چپ < راست) {
        u32 mid = چپ + (راست - چپ) / 2;

        اگر (mapping->entries[mid].pcOffset < pcOffset) {
            چپ = mid + 1;
        } وگرنه اگر (mapping->entries[mid].pcOffset > pcOffset) {
            راست = mid;
        } وگرنه {
            *o_moduleOffset = mapping->entries[mid].moduleOffset;
            بازگشت true;
        }
    }

    // Getting here means left is now one more than the element we want.
    اگر (چپ > 0) {
        چپ--;
        *o_moduleOffset = mapping->entries[چپ].moduleOffset;
        بازگشت true;
    } وگرنه {
        بازگشت false;
    }
}
#endif // d_m3RecordBacktraces

//---------------------------------------------------------------------------------------------------------------------------------
