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


IM3CodePage NewCodePage (IM3Runtime i_runtime, u32 i_min_अङ्कः_पङ्क्तयः)
{
    IM3CodePage page;

    // check multiplication overflow
    सचे (i_min_अङ्कः_पङ्क्तयः > UINT_MAX / परिमाण(code_t)) {
        पटिनिवत्त रिक्त;
    }
    u32 pageSize = परिमाण(M3CodePageHeader) + परिमाण(code_t) * i_min_अङ्कः_पङ्क्तयः;

    // check addition overflow
    सचे (pageSize < परिमाण(M3CodePageHeader)) {
        पटिनिवत्त रिक्त;
    }

    pageSize = (pageSize + (d_m3CodePageAlignSize - 1)) & ~(d_m3CodePageAlignSize - 1); // align
    // check alignment overflow
    सचे (pageSize == 0) {
        पटिनिवत्त रिक्त;
    }

    page = (IM3CodePage)m3_Malloc("M3CodePage", pageSize);

    सचे (page) {
        page->info.sequence = ++i_runtime->newCodePageSequence;
        page->info.अङ्कः_पङ्क्तयः = (pageSize - परिमाण(M3CodePageHeader)) / परिमाण(code_t);

#सचे d_m3RecordBacktraces
        u32 pageSizeBt     = परिमाण(M3CodeMappingPage) + परिमाण(M3CodeMapEntry) * page->info.अङ्कः_पङ्क्तयः;
        page->info.mapping = (M3CodeMappingPage*)m3_Malloc("M3CodeMappingPage", pageSizeBt);

        सचे (page->info.mapping) {
            page->info.mapping->size     = 0;
            page->info.mapping->capacity = page->info.अङ्कः_पङ्क्तयः;
        } अञ्ञथा {
            m3_Free(page);
            पटिनिवत्त रिक्त;
        }
        page->info.mapping->basePC = GetPageStartPC(page);
#endif // d_m3RecordBacktraces

        m3log(runtime, "new page: %p; seq: %d; bytes: %d; lines: %d", GetPagePC(page), page->info.sequence, pageSize, page->info.अङ्कः_पङ्क्तयः);
    }

    पटिनिवत्त page;
}


शून्य FreeCodePages (IM3CodePage* io_list)
{
    IM3CodePage page = *io_list;

    याव (page) {
        m3log(code, "free page: %d; %p; util: %3.1f%%", page->info.sequence, page, 100. * page->info.पङ्क्तिः_Index / page->info.अङ्कः_पङ्क्तयः);

        IM3CodePage next = page->info.next;
#सचे d_m3RecordBacktraces
        m3_Free(page->info.mapping);
#endif // d_m3RecordBacktraces
        m3_Free(page);
        page = next;
    }

    *io_list = रिक्त;
}


u32 अङ्कः_Free_पङ्क्तयः (IM3CodePage i_page)
{
    d_m3Assert(i_page->info.पङ्क्तिः_Index <= i_page->info.अङ्कः_पङ्क्तयः);

    पटिनिवत्त i_page->info.अङ्कः_पङ्क्तयः - i_page->info.पङ्क्तिः_Index;
}


शून्य EmitWord_impl (IM3CodePage i_page, शून्य* i_word)
{                                                                       d_m3Assert (i_page->info.पङ्क्तिः_Index+1 <= i_page->info.अङ्कः_पङ्क्तयः);
    i_page->code[i_page->info.पङ्क्तिः_Index++] = i_word;
}

शून्य EmitWord32 (IM3CodePage i_page, ध्रुव u32 i_word)
{                                                                       d_m3Assert (i_page->info.पङ्क्तिः_Index+1 <= i_page->info.अङ्कः_पङ्क्तयः);
    memcpy(&i_page->code[i_page->info.पङ्क्तिः_Index++], &i_word, परिमाण(i_word));
}

शून्य EmitWord64 (IM3CodePage i_page, ध्रुव u64 i_word)
{
#सचे M3_SIZEOF_PTR == 4
                                                                        d_m3Assert (i_page->info.पङ्क्तिः_Index+2 <= i_page->info.अङ्कः_पङ्क्तयः);
    memcpy(&i_page->code[i_page->info.पङ्क्तिः_Index], &i_word, परिमाण(i_word));
    i_page->info.पङ्क्तिः_Index += 2;
#अञ्ञथा
                                                                        d_m3Assert (i_page->info.पङ्क्तिः_Index+1 <= i_page->info.अङ्कः_पङ्क्तयः);
    memcpy(&i_page->code[i_page->info.पङ्क्तिः_Index], &i_word, परिमाण(i_word));
    i_page->info.पङ्क्तिः_Index += 1;
#endif
}


#सचे d_m3RecordBacktraces
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
    पटिनिवत्त &i_page->code[0];
}


pc_t GetPagePC (IM3CodePage i_page)
{
    सचे (i_page) {
        पटिनिवत्त &i_page->code[i_page->info.पङ्क्तिः_Index];
    } अञ्ञथा {
        पटिनिवत्त रिक्त;
    }
}


शून्य प्रक्षिप_Code_Page (IM3CodePage* i_list, IM3CodePage i_codePage)
{
    IM3CodePage next      = *i_list;
    i_codePage->info.next = next;
    *i_list               = i_codePage;
}


IM3CodePage निष्कासय_Code_Page (IM3CodePage* i_list)
{
    IM3CodePage page = *i_list;
    *i_list          = page->info.next;
    page->info.next  = रिक्त;

    पटिनिवत्त page;
}


u32 FindCodePageEnd (IM3CodePage i_list, IM3CodePage* o_end)
{
    u32 अङ्कः_Pages = 0;
    *o_end       = रिक्त;

    याव (i_list) {
        *o_end = i_list;
        ++अङ्कः_Pages;
        i_list = i_list->info.next;
    }

    पटिनिवत्त अङ्कः_Pages;
}


u32 गणनम्_Code_Pages (IM3CodePage i_list)
{
    IM3CodePage unused;
    पटिनिवत्त FindCodePageEnd(i_list, &unused);
}


IM3CodePage GetEndCodePage (IM3CodePage i_list)
{
    IM3CodePage end;
    FindCodePageEnd(i_list, &end);

    पटिनिवत्त end;
}

#सचे d_m3RecordBacktraces
bool ContainsPC (IM3CodePage i_page, pc_t i_pc)
{
    पटिनिवत्त GetPageStartPC(i_page) <= i_pc && i_pc < GetPagePC(i_page);
}


bool MapPCToOffset (IM3CodePage i_page, pc_t i_pc, u32* o_moduleOffset)
{
    M3CodeMappingPage* mapping = i_page->info.mapping;

    u32 pcOffset = (u32)(i_pc - mapping->basePC);

    u32 वामः  = 0;
    u32 दक्षिणः = mapping->size;

    याव (वामः < दक्षिणः) {
        u32 mid = वामः + (दक्षिणः - वामः) / 2;

        सचे (mapping->entries[mid].pcOffset < pcOffset) {
            वामः = mid + 1;
        } अञ्ञथा सचे (mapping->entries[mid].pcOffset > pcOffset) {
            दक्षिणः = mid;
        } अञ्ञथा {
            *o_moduleOffset = mapping->entries[mid].moduleOffset;
            पटिनिवत्त true;
        }
    }

    // Getting here means left is now one more than the element we want.
    सचे (वामः > 0) {
        वामः--;
        *o_moduleOffset = mapping->entries[वामः].moduleOffset;
        पटिनिवत्त true;
    } अञ्ञथा {
        पटिनिवत्त false;
    }
}
#endif // d_m3RecordBacktraces

//---------------------------------------------------------------------------------------------------------------------------------
