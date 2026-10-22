//
//  m3_code.h
//
//  Created by Steven Massey on 4/19/19.
//  Copyright © 2019 Steven Massey. All rights reserved.
//

#ifndef m3_code_h
#define m3_code_h

#അന്തര്ഭാവ "m3_core.h"

d_m3BeginExternC

തരനിർവചനം ഘടന M3CodePage {
    M3CodePageHeader info;
    code_t           code[1];
} M3CodePage;

തരനിർവചനം M3CodePage* IM3CodePage;


IM3CodePage         NewCodePage (IM3Runtime i_runtime, u32 i_min_അക്കം_വരികൾ);

ശൂന്യം                FreeCodePages (IM3CodePage* io_list);

u32                 അക്കം_Free_വരികൾ (IM3CodePage i_page);
pc_t                GetPageStartPC (IM3CodePage i_page);
pc_t                GetPagePC (IM3CodePage i_page);
ശൂന്യം                EmitWord_impl (IM3CodePage i_page, ശൂന്യം* i_word);
ശൂന്യം                EmitWord32 (IM3CodePage i_page, u32 i_word);
ശൂന്യം                EmitWord64 (IM3CodePage i_page, u64 i_word);
#എങ്കിൽ d_m3RecordBacktraces
ശൂന്യം EmitMappingEntry (IM3CodePage i_page, u32 i_moduleOffset);
#endif // d_m3RecordBacktraces

ശൂന്യം        തള്ളുക_Code_Page (IM3CodePage* io_list, IM3CodePage i_codePage);
IM3CodePage എടുക്കുക_Code_Page (IM3CodePage* io_list);

IM3CodePage GetEndCodePage (IM3CodePage i_list); // i_list = NULL is valid
u32         എണ്ണം_Code_Pages (IM3CodePage i_list); // i_list = NULL is valid

#എങ്കിൽ d_m3RecordBacktraces
bool ContainsPC (IM3CodePage i_page, pc_t i_pc);
bool MapPCToOffset (IM3CodePage i_page, pc_t i_pc, u32* o_moduleOffset);
#endif // d_m3RecordBacktraces

#ifdef DEBUG
ശൂന്യം dump_code_page (IM3CodePage i_codePage, pc_t i_startPC);
#endif

#define EmitWord(page, val) EmitWord_impl(page, (ശൂന്യം*)(val))

//---------------------------------------------------------------------------------------------------------------------------------

#എങ്കിൽ d_m3RecordBacktraces

തരനിർവചനം ഘടന M3CodeMapEntry {
    u32 pcOffset;
    u32 moduleOffset;
} M3CodeMapEntry;

തരനിർവചനം ഘടന M3CodeMappingPage {
    pc_t           basePC;
    u32            size;
    u32            capacity;
    M3CodeMapEntry entries[];
} M3CodeMappingPage;

#endif // d_m3RecordBacktraces

d_m3EndExternC

#endif // m3_code_h
