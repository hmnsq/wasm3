//
//  m3_code.h
//
//  Created by Steven Massey on 4/19/19.
//  Copyright © 2019 Steven Massey. All rights reserved.
//

#ifndef m3_code_h
#define m3_code_h

#অন্তর্ভাব "m3_core.h"

d_m3BeginExternC

প্রকারনির্দেশ সংরচনা M3CodePage {
    M3CodePageHeader info;
    code_t           code[1];
} M3CodePage;

প্রকারনির্দেশ M3CodePage* IM3CodePage;


IM3CodePage         NewCodePage (IM3Runtime i_runtime, u32 i_min_অঙ্ক_সারিগুলি);

শূন্য                FreeCodePages (IM3CodePage* io_list);

u32                 অঙ্ক_Free_সারিগুলি (IM3CodePage i_page);
pc_t                GetPageStartPC (IM3CodePage i_page);
pc_t                GetPagePC (IM3CodePage i_page);
শূন্য                EmitWord_impl (IM3CodePage i_page, শূন্য* i_word);
শূন্য                EmitWord32 (IM3CodePage i_page, u32 i_word);
শূন্য                EmitWord64 (IM3CodePage i_page, u64 i_word);
#যদি d_m3RecordBacktraces
শূন্য EmitMappingEntry (IM3CodePage i_page, u32 i_moduleOffset);
#endif // d_m3RecordBacktraces

শূন্য        ঠেলো_Code_Page (IM3CodePage* io_list, IM3CodePage i_codePage);
IM3CodePage বের_করো_Code_Page (IM3CodePage* io_list);

IM3CodePage GetEndCodePage (IM3CodePage i_list); // i_list = NULL is valid
u32         গুনতি_Code_Pages (IM3CodePage i_list); // i_list = NULL is valid

#যদি d_m3RecordBacktraces
bool ContainsPC (IM3CodePage i_page, pc_t i_pc);
bool MapPCToOffset (IM3CodePage i_page, pc_t i_pc, u32* o_moduleOffset);
#endif // d_m3RecordBacktraces

#ifdef DEBUG
শূন্য dump_code_page (IM3CodePage i_codePage, pc_t i_startPC);
#endif

#define EmitWord(page, val) EmitWord_impl(page, (শূন্য*)(val))

//---------------------------------------------------------------------------------------------------------------------------------

#যদি d_m3RecordBacktraces

প্রকারনির্দেশ সংরচনা M3CodeMapEntry {
    u32 pcOffset;
    u32 moduleOffset;
} M3CodeMapEntry;

প্রকারনির্দেশ সংরচনা M3CodeMappingPage {
    pc_t           basePC;
    u32            size;
    u32            capacity;
    M3CodeMapEntry entries[];
} M3CodeMappingPage;

#endif // d_m3RecordBacktraces

d_m3EndExternC

#endif // m3_code_h
