//
//  m3_code.h
//
//  Created by Steven Massey on 4/19/19.
//  Copyright © 2019 Steven Massey. All rights reserved.
//

#ifndef m3_code_h
#define m3_code_h

#ଅନ୍ତର୍ଭାଵ "m3_core.h"

d_m3BeginExternC

ପ୍ରକାରବ୍ୟାଖ୍ୟା ସଂରଚନା M3CodePage {
    M3CodePageHeader info;
    code_t           code[1];
} M3CodePage;

ପ୍ରକାରବ୍ୟାଖ୍ୟା M3CodePage* IM3CodePage;


IM3CodePage         NewCodePage (IM3Runtime i_runtime, u32 i_min_ଅଙ୍କ_ଧାଡ଼ିଗୁଡ଼ିକ);

ଶୂନ୍ୟ                FreeCodePages (IM3CodePage* io_list);

u32                 ଅଙ୍କ_Free_ଧାଡ଼ିଗୁଡ଼ିକ (IM3CodePage i_page);
pc_t                GetPageStartPC (IM3CodePage i_page);
pc_t                GetPagePC (IM3CodePage i_page);
ଶୂନ୍ୟ                EmitWord_impl (IM3CodePage i_page, ଶୂନ୍ୟ* i_word);
ଶୂନ୍ୟ                EmitWord32 (IM3CodePage i_page, u32 i_word);
ଶୂନ୍ୟ                EmitWord64 (IM3CodePage i_page, u64 i_word);
#ଯଦି d_m3RecordBacktraces
ଶୂନ୍ୟ EmitMappingEntry (IM3CodePage i_page, u32 i_moduleOffset);
#endif // d_m3RecordBacktraces

ଶୂନ୍ୟ        ଠେଲ_Code_Page (IM3CodePage* io_list, IM3CodePage i_codePage);
IM3CodePage କାଢ଼_Code_Page (IM3CodePage* io_list);

IM3CodePage GetEndCodePage (IM3CodePage i_list); // i_list = NULL is valid
u32         ଗଣନା_Code_Pages (IM3CodePage i_list); // i_list = NULL is valid

#ଯଦି d_m3RecordBacktraces
bool ContainsPC (IM3CodePage i_page, pc_t i_pc);
bool MapPCToOffset (IM3CodePage i_page, pc_t i_pc, u32* o_moduleOffset);
#endif // d_m3RecordBacktraces

#ifdef DEBUG
ଶୂନ୍ୟ dump_code_page (IM3CodePage i_codePage, pc_t i_startPC);
#endif

#define EmitWord(page, val) EmitWord_impl(page, (ଶୂନ୍ୟ*)(val))

//---------------------------------------------------------------------------------------------------------------------------------

#ଯଦି d_m3RecordBacktraces

ପ୍ରକାରବ୍ୟାଖ୍ୟା ସଂରଚନା M3CodeMapEntry {
    u32 pcOffset;
    u32 moduleOffset;
} M3CodeMapEntry;

ପ୍ରକାରବ୍ୟାଖ୍ୟା ସଂରଚନା M3CodeMappingPage {
    pc_t           basePC;
    u32            size;
    u32            capacity;
    M3CodeMapEntry entries[];
} M3CodeMappingPage;

#endif // d_m3RecordBacktraces

d_m3EndExternC

#endif // m3_code_h
