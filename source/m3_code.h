//
//  m3_code.h
//
//  Created by Steven Massey on 4/19/19.
//  Copyright © 2019 Steven Massey. All rights reserved.
//

#ifndef m3_code_h
#define m3_code_h

#அந்தர்பாவ "m3_core.h"

d_m3BeginExternC

வகைவரை கட்டமைப்பு M3CodePage {
    M3CodePageHeader info;
    code_t           code[1];
} M3CodePage;

வகைவரை M3CodePage* IM3CodePage;


IM3CodePage         NewCodePage (IM3Runtime i_runtime, u32 i_min_இலக்கம்_வரிகள்);

வெறுமை                FreeCodePages (IM3CodePage* io_list);

u32                 இலக்கம்_Free_வரிகள் (IM3CodePage i_page);
pc_t                GetPageStartPC (IM3CodePage i_page);
pc_t                GetPagePC (IM3CodePage i_page);
வெறுமை                EmitWord_impl (IM3CodePage i_page, வெறுமை* i_word);
வெறுமை                EmitWord32 (IM3CodePage i_page, u32 i_word);
வெறுமை                EmitWord64 (IM3CodePage i_page, u64 i_word);
#எனில் d_m3RecordBacktraces
வெறுமை EmitMappingEntry (IM3CodePage i_page, u32 i_moduleOffset);
#endif // d_m3RecordBacktraces

வெறுமை        தள்ளு_Code_Page (IM3CodePage* io_list, IM3CodePage i_codePage);
IM3CodePage எடு_Code_Page (IM3CodePage* io_list);

IM3CodePage GetEndCodePage (IM3CodePage i_list); // i_list = NULL is valid
u32         மொத்தம்_Code_Pages (IM3CodePage i_list); // i_list = NULL is valid

#எனில் d_m3RecordBacktraces
bool ContainsPC (IM3CodePage i_page, pc_t i_pc);
bool MapPCToOffset (IM3CodePage i_page, pc_t i_pc, u32* o_moduleOffset);
#endif // d_m3RecordBacktraces

#ifdef DEBUG
வெறுமை dump_code_page (IM3CodePage i_codePage, pc_t i_startPC);
#endif

#define EmitWord(page, val) EmitWord_impl(page, (வெறுமை*)(val))

//---------------------------------------------------------------------------------------------------------------------------------

#எனில் d_m3RecordBacktraces

வகைவரை கட்டமைப்பு M3CodeMapEntry {
    u32 pcOffset;
    u32 moduleOffset;
} M3CodeMapEntry;

வகைவரை கட்டமைப்பு M3CodeMappingPage {
    pc_t           basePC;
    u32            size;
    u32            capacity;
    M3CodeMapEntry entries[];
} M3CodeMappingPage;

#endif // d_m3RecordBacktraces

d_m3EndExternC

#endif // m3_code_h
