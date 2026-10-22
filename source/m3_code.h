//
//  m3_code.h
//
//  Created by Steven Massey on 4/19/19.
//  Copyright © 2019 Steven Massey. All rights reserved.
//

#यदि_अपरिभाषितम् m3_संहिता_h
#परिभाषय m3_संहिता_h

#अन्तर्भाव "m3_core.h"

d_m3BeginExternC

प्रकारनिर्देश संरचना M3_संहिता_Page {
    M3_संहिता_Page_शीर्षकम् info;
    संहिता_t           संहिता[1];
} M3_संहिता_Page;

प्रकारनिर्देश M3_संहिता_Page* IM3_संहिता_Page;


IM3_संहिता_Page         New_संहिता_Page (IM3Runtime i_runtime, u32 i_min_अङ्कः_पङ्क्तयः);

शून्य                Free_संहिता_पृष्ठानि (IM3_संहिता_Page* io_list);

u32                 अङ्कः_Free_पङ्क्तयः (IM3_संहिता_Page i_page);
pc_t                GetPageStartPC (IM3_संहिता_Page i_page);
pc_t                GetPagePC (IM3_संहिता_Page i_page);
शून्य                EmitWord_impl (IM3_संहिता_Page i_page, शून्य* i_word);
शून्य                EmitWord32 (IM3_संहिता_Page i_page, u32 i_word);
शून्य                EmitWord64 (IM3_संहिता_Page i_page, u64 i_word);
#यदि d_m3_लेखः_Backtraces
शून्य Emit_Mapping_प्रविष्टिः (IM3_संहिता_Page i_page, u32 i_module_विस्थापनम्);
#यदि_अन्तः // d_m3RecordBacktraces

शून्य        प्रक्षिप_संहिता_Page (IM3_संहिता_Page* io_list, IM3_संहिता_Page i_संहिता_Page);
IM3_संहिता_Page निष्कासय_संहिता_Page (IM3_संहिता_Page* io_list);

IM3_संहिता_Page Get_अन्तः_संहिता_Page (IM3_संहिता_Page i_list); // i_list = NULL is valid
u32         गणनम्_संहिता_पृष्ठानि (IM3_संहिता_Page i_list); // i_list = NULL is valid

#यदि d_m3_लेखः_Backtraces
bool अन्तर्भवति_PC (IM3_संहिता_Page i_page, pc_t i_pc);
bool Map_PCTo_विस्थापनम् (IM3_संहिता_Page i_page, pc_t i_pc, u32* o_module_विस्थापनम्);
#यदि_अन्तः // d_m3RecordBacktraces

#यदि_परिभाषितम् DEBUG
शून्य dump_संहिता_page (IM3_संहिता_Page i_संहिता_Page, pc_t i_startPC);
#यदि_अन्तः

#परिभाषय EmitWord(page, val) EmitWord_impl(page, (शून्य*)(val))

//---------------------------------------------------------------------------------------------------------------------------------

#यदि d_m3_लेखः_Backtraces

प्रकारनिर्देश संरचना M3_संहिता_Map_प्रविष्टिः {
    u32 pc_विस्थापनम्;
    u32 module_विस्थापनम्;
} M3_संहिता_Map_प्रविष्टिः;

प्रकारनिर्देश संरचना M3_संहिता_Mapping_Page {
    pc_t           आधारः_PC;
    u32            आकार;
    u32            क्षमता;
    M3_संहिता_Map_प्रविष्टिः entries[];
} M3_संहिता_Mapping_Page;

#यदि_अन्तः // d_m3RecordBacktraces

d_m3_अन्तः_Extern_C

#यदि_अन्तः // m3_code_h
