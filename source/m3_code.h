//
//  m3_code.h
//
//  Created by Steven Massey on 4/19/19.
//  Copyright © 2019 Steven Massey. All rights reserved.
//

#अगर_अघोषित m3_code_h
#घोषणा m3_code_h

#समावेश "m3_core.h"

d_m3BeginExternC

प्रकार काष्ठा M3CodePage {
    M3CodePageHeader info;
    code_t           code[1];
} M3CodePage;

प्रकार M3CodePage* IM3CodePage;


IM3CodePage         NewCodePage (IM3Runtime i_runtime, u32 i_min_अंक_पंक्तियाँ);

व्योम                FreeCodePages (IM3CodePage* io_list);

u32                 अंक_Free_पंक्तियाँ (IM3CodePage i_page);
pc_t                GetPageStartPC (IM3CodePage i_page);
pc_t                GetPagePC (IM3CodePage i_page);
व्योम                EmitWord_impl (IM3CodePage i_page, व्योम* i_word);
व्योम                EmitWord32 (IM3CodePage i_page, u32 i_word);
व्योम                EmitWord64 (IM3CodePage i_page, u64 i_word);
#अगर d_m3RecordBacktraces
व्योम EmitMappingEntry (IM3CodePage i_page, u32 i_moduleOffset);
#पूर्ण_अगर // d_m3RecordBacktraces

व्योम        धकेलो_Code_Page (IM3CodePage* io_list, IM3CodePage i_codePage);
IM3CodePage निकालो_Code_Page (IM3CodePage* io_list);

IM3CodePage GetEndCodePage (IM3CodePage i_list); // i_list = NULL is valid
u32         गिनती_Code_Pages (IM3CodePage i_list); // i_list = NULL is valid

#अगर d_m3RecordBacktraces
bool ContainsPC (IM3CodePage i_page, pc_t i_pc);
bool MapPCToOffset (IM3CodePage i_page, pc_t i_pc, u32* o_moduleOffset);
#पूर्ण_अगर // d_m3RecordBacktraces

#अगर_घोषित DEBUG
व्योम dump_code_page (IM3CodePage i_codePage, pc_t i_startPC);
#पूर्ण_अगर

#घोषणा EmitWord(page, val) EmitWord_impl(page, (व्योम*)(val))

//---------------------------------------------------------------------------------------------------------------------------------

#अगर d_m3RecordBacktraces

प्रकार काष्ठा M3CodeMapEntry {
    u32 pcOffset;
    u32 moduleOffset;
} M3CodeMapEntry;

प्रकार काष्ठा M3CodeMappingPage {
    pc_t           basePC;
    u32            आकार;
    u32            capacity;
    M3CodeMapEntry entries[];
} M3CodeMappingPage;

#पूर्ण_अगर // d_m3RecordBacktraces

d_m3EndExternC

#पूर्ण_अगर // m3_code_h
