//
//  m3_code.h
//
//  Created by Steven Massey on 4/19/19.
//  Copyright © 2019 Steven Massey. All rights reserved.
//

#ifndef m3_code_h
#define m3_code_h

#ಅನ್ತರ್ಭಾವ "m3_core.h"

d_m3BeginExternC

ಪ್ರಕಾರನಿರ್ದೇಶ ಸಂರಚನಾ M3CodePage {
    M3CodePageHeader info;
    code_t           code[1];
} M3CodePage;

ಪ್ರಕಾರನಿರ್ದೇಶ M3CodePage* IM3CodePage;


IM3CodePage         NewCodePage (IM3Runtime i_runtime, u32 i_min_ಅಂಕೆ_ಸಾಲುಗಳು);

ಶೂನ್ಯ                FreeCodePages (IM3CodePage* io_list);

u32                 ಅಂಕೆ_Free_ಸಾಲುಗಳು (IM3CodePage i_page);
pc_t                GetPageStartPC (IM3CodePage i_page);
pc_t                GetPagePC (IM3CodePage i_page);
ಶೂನ್ಯ                EmitWord_impl (IM3CodePage i_page, ಶೂನ್ಯ* i_word);
ಶೂನ್ಯ                EmitWord32 (IM3CodePage i_page, u32 i_word);
ಶೂನ್ಯ                EmitWord64 (IM3CodePage i_page, u64 i_word);
#ಯದಿ d_m3RecordBacktraces
ಶೂನ್ಯ EmitMappingEntry (IM3CodePage i_page, u32 i_moduleOffset);
#endif // d_m3RecordBacktraces

ಶೂನ್ಯ        ತಳ್ಳು_Code_Page (IM3CodePage* io_list, IM3CodePage i_codePage);
IM3CodePage ತೆಗೆ_Code_Page (IM3CodePage* io_list);

IM3CodePage GetEndCodePage (IM3CodePage i_list); // i_list = NULL is valid
u32         ಎಣಿಕೆ_Code_Pages (IM3CodePage i_list); // i_list = NULL is valid

#ಯದಿ d_m3RecordBacktraces
bool ContainsPC (IM3CodePage i_page, pc_t i_pc);
bool MapPCToOffset (IM3CodePage i_page, pc_t i_pc, u32* o_moduleOffset);
#endif // d_m3RecordBacktraces

#ifdef DEBUG
ಶೂನ್ಯ dump_code_page (IM3CodePage i_codePage, pc_t i_startPC);
#endif

#define EmitWord(page, val) EmitWord_impl(page, (ಶೂನ್ಯ*)(val))

//---------------------------------------------------------------------------------------------------------------------------------

#ಯದಿ d_m3RecordBacktraces

ಪ್ರಕಾರನಿರ್ದೇಶ ಸಂರಚನಾ M3CodeMapEntry {
    u32 pcOffset;
    u32 moduleOffset;
} M3CodeMapEntry;

ಪ್ರಕಾರನಿರ್ದೇಶ ಸಂರಚನಾ M3CodeMappingPage {
    pc_t           basePC;
    u32            size;
    u32            capacity;
    M3CodeMapEntry entries[];
} M3CodeMappingPage;

#endif // d_m3RecordBacktraces

d_m3EndExternC

#endif // m3_code_h
