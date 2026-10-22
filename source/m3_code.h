//
//  m3_code.h
//
//  Created by Steven Massey on 4/19/19.
//  Copyright © 2019 Steven Massey. All rights reserved.
//

#ifndef m3_code_h
#define m3_code_h

#ضمن "m3_core.h"

d_m3BeginExternC

تعريفنوع بنية M3CodePage {
    M3CodePageHeader info;
    code_t           code[1];
} M3CodePage;

تعريفنوع M3CodePage* IM3CodePage;


IM3CodePage         NewCodePage (IM3Runtime i_runtime, u32 i_minNumLines);

فراغ                FreeCodePages (IM3CodePage* io_list);

u32                 NumFreeLines (IM3CodePage i_page);
pc_t                GetPageStartPC (IM3CodePage i_page);
pc_t                GetPagePC (IM3CodePage i_page);
فراغ                EmitWord_impl (IM3CodePage i_page, فراغ* i_word);
فراغ                EmitWord32 (IM3CodePage i_page, u32 i_word);
فراغ                EmitWord64 (IM3CodePage i_page, u64 i_word);
#ܐܢ d_m3RecordBacktraces
فراغ EmitMappingEntry (IM3CodePage i_page, u32 i_moduleOffset);
#endif // d_m3RecordBacktraces

فراغ        PushCodePage (IM3CodePage* io_list, IM3CodePage i_codePage);
IM3CodePage PopCodePage (IM3CodePage* io_list);

IM3CodePage GetEndCodePage (IM3CodePage i_list); // i_list = NULL is valid
u32         CountCodePages (IM3CodePage i_list); // i_list = NULL is valid

#ܐܢ d_m3RecordBacktraces
bool ContainsPC (IM3CodePage i_page, pc_t i_pc);
bool MapPCToOffset (IM3CodePage i_page, pc_t i_pc, u32* o_moduleOffset);
#endif // d_m3RecordBacktraces

#ifdef DEBUG
فراغ dump_code_page (IM3CodePage i_codePage, pc_t i_startPC);
#endif

#define EmitWord(page, val) EmitWord_impl(page, (فراغ*)(val))

//---------------------------------------------------------------------------------------------------------------------------------

#ܐܢ d_m3RecordBacktraces

تعريفنوع بنية M3CodeMapEntry {
    u32 pcOffset;
    u32 moduleOffset;
} M3CodeMapEntry;

تعريفنوع بنية M3CodeMappingPage {
    pc_t           basePC;
    u32            size;
    u32            capacity;
    M3CodeMapEntry entries[];
} M3CodeMappingPage;

#endif // d_m3RecordBacktraces

d_m3EndExternC

#endif // m3_code_h
