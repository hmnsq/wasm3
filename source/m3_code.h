//
//  m3_code.h
//
//  Created by Steven Massey on 4/19/19.
//  Copyright © 2019 Steven Massey. All rights reserved.
//

#که_نه_تعریف_شوی m3_کوډ_h
#تعریف_کړه m3_کوډ_h

#ضمن "m3_core.h"

d_m3BeginExternC

تعريفنوع بنية M3_کوډ_پاڼه {
    M3_کوډ_پاڼه_سرلیک info;
    کوډ_t           کوډ[1];
} M3_کوډ_پاڼه;

تعريفنوع M3_کوډ_پاڼه* IM3_کوډ_پاڼه;


IM3_کوډ_پاڼه         New_کوډ_پاڼه (IM3Runtime i_runtime, u32 i_min_عدد_کرښې);

فراغ                Free_کوډ_پاڼې (IM3_کوډ_پاڼه* io_list);

u32                 عدد_Free_کرښې (IM3_کوډ_پاڼه i_پاڼه);
pc_t                Get_پاڼه_پیل_PC (IM3_کوډ_پاڼه i_پاڼه);
pc_t                Get_پاڼه_PC (IM3_کوډ_پاڼه i_پاڼه);
فراغ                Emit_کلمه_impl (IM3_کوډ_پاڼه i_پاڼه, فراغ* i_کلمه);
فراغ                EmitWord32 (IM3_کوډ_پاڼه i_پاڼه, u32 i_کلمه);
فراغ                EmitWord64 (IM3_کوډ_پاڼه i_پاڼه, u64 i_کلمه);
#إذا d_m3_ریکارډ_Backtraces
فراغ Emit_Mapping_ثبت (IM3_کوډ_پاڼه i_پاڼه, u32 i_module_دننه_واټن);
#د_که_پای // d_m3RecordBacktraces

فراغ        ټېل_کوډ_پاڼه (IM3_کوډ_پاڼه* io_list, IM3_کوډ_پاڼه i_کوډ_پاڼه);
IM3_کوډ_پاڼه راوباسه_کوډ_پاڼه (IM3_کوډ_پاڼه* io_list);

IM3_کوډ_پاڼه Get_پای_ټکی_کوډ_پاڼه (IM3_کوډ_پاڼه i_list); // i_list = NULL is valid
u32         شمېر_کوډ_پاڼې (IM3_کوډ_پاڼه i_list); // i_list = NULL is valid

#إذا d_m3_ریکارډ_Backtraces
bool لري_PC (IM3_کوډ_پاڼه i_پاڼه, pc_t i_pc);
bool Map_PCTo_دننه_واټن (IM3_کوډ_پاڼه i_پاڼه, pc_t i_pc, u32* o_module_دننه_واټن);
#د_که_پای // d_m3RecordBacktraces

#که_تعریف_شوی DEBUG
فراغ dump_کوډ_پاڼه (IM3_کوډ_پاڼه i_کوډ_پاڼه, pc_t i_پیل_PC);
#د_که_پای

#تعریف_کړه Emit_کلمه(پاڼه, val) Emit_کلمه_impl(پاڼه, (فراغ*)(val))

//---------------------------------------------------------------------------------------------------------------------------------

#إذا d_m3_ریکارډ_Backtraces

تعريفنوع بنية M3_کوډ_Map_ثبت {
    u32 pc_دننه_واټن;
    u32 module_دننه_واټن;
} M3_کوډ_Map_ثبت;

تعريفنوع بنية M3_کوډ_Mapping_پاڼه {
    pc_t           بنسټ_PC;
    u32            اندازه;
    u32            ظرفیت;
    M3_کوډ_Map_ثبت entries[];
} M3_کوډ_Mapping_پاڼه;

#د_که_پای // d_m3RecordBacktraces

d_m3_پای_ټکی_Extern_C

#د_که_پای // m3_code_h
