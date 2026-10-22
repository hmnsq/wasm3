//
//  m3_code.h
//
//  Created by Steven Massey on 4/19/19.
//  Copyright © 2019 Steven Massey. All rights reserved.
//

#ifndef m3_code_h
#define m3_code_h

#अन्तर्भाव "m3_core.h"

d_m3BeginExternC

प्रकारनिर्देश संरचना M3CodePage {
    M3CodePageHeader info;
    code_t           code[1];
} M3CodePage;

प्रकारनिर्देश M3CodePage* IM3CodePage;


IM3CodePage         NewCodePage (IM3Runtime i_runtime, u32 i_min_अङ्कः_पङ्क्तयः);

शून्य                FreeCodePages (IM3CodePage* io_list);

u32                 अङ्कः_Free_पङ्क्तयः (IM3CodePage i_page);
pc_t                GetPageStartPC (IM3CodePage i_page);
pc_t                GetPagePC (IM3CodePage i_page);
शून्य                EmitWord_impl (IM3CodePage i_page, शून्य* i_word);
शून्य                EmitWord32 (IM3CodePage i_page, u32 i_word);
शून्य                EmitWord64 (IM3CodePage i_page, u64 i_word);
#सचे d_m3RecordBacktraces
शून्य EmitMappingEntry (IM3CodePage i_page, u32 i_moduleOffset);
#endif // d_m3RecordBacktraces

शून्य        प्रक्षिप_Code_Page (IM3CodePage* io_list, IM3CodePage i_codePage);
IM3CodePage निष्कासय_Code_Page (IM3CodePage* io_list);

IM3CodePage GetEndCodePage (IM3CodePage i_list); // i_list = NULL is valid
u32         गणनम्_Code_Pages (IM3CodePage i_list); // i_list = NULL is valid

#सचे d_m3RecordBacktraces
bool ContainsPC (IM3CodePage i_page, pc_t i_pc);
bool MapPCToOffset (IM3CodePage i_page, pc_t i_pc, u32* o_moduleOffset);
#endif // d_m3RecordBacktraces

#ifdef DEBUG
शून्य dump_code_page (IM3CodePage i_codePage, pc_t i_startPC);
#endif

#define EmitWord(page, val) EmitWord_impl(page, (शून्य*)(val))

//---------------------------------------------------------------------------------------------------------------------------------

#सचे d_m3RecordBacktraces

प्रकारनिर्देश संरचना M3CodeMapEntry {
    u32 pcOffset;
    u32 moduleOffset;
} M3CodeMapEntry;

प्रकारनिर्देश संरचना M3CodeMappingPage {
    pc_t           basePC;
    u32            size;
    u32            capacity;
    M3CodeMapEntry entries[];
} M3CodeMappingPage;

#endif // d_m3RecordBacktraces

d_m3EndExternC

#endif // m3_code_h
