//
//  m3_code.h
//
//  Created by Steven Massey on 4/19/19.
//  Copyright © 2019 Steven Massey. All rights reserved.
//

#جے_بنا_تعریف m3_کوڈ_h
#تعریف_کرو m3_کوڈ_h

#ضمن "m3_core.h"

d_m3BeginExternC

تعريفنوع بنية M3_کوڈ_ورقہ {
    M3_کوڈ_ورقہ_سرخی info;
    کوڈ_t           کوڈ[1];
} M3_کوڈ_ورقہ;

تعريفنوع M3_کوڈ_ورقہ* IM3_کوڈ_ورقہ;


IM3_کوڈ_ورقہ         New_کوڈ_ورقہ (IM3Runtime i_runtime, u32 i_min_عدد_سطریں);

فراغ                Free_کوڈ_ورقے (IM3_کوڈ_ورقہ* io_list);

u32                 عدد_Free_سطریں (IM3_کوڈ_ورقہ i_ورقہ);
pc_t                Get_ورقہ_شروع_PC (IM3_کوڈ_ورقہ i_ورقہ);
pc_t                Get_ورقہ_PC (IM3_کوڈ_ورقہ i_ورقہ);
فراغ                Emit_لفظ_impl (IM3_کوڈ_ورقہ i_ورقہ, فراغ* i_لفظ);
فراغ                EmitWord32 (IM3_کوڈ_ورقہ i_ورقہ, u32 i_لفظ);
فراغ                EmitWord64 (IM3_کوڈ_ورقہ i_ورقہ, u64 i_لفظ);
#إذا d_m3_ریکارڈ_Backtraces
فراغ Emit_Mapping_اندراج (IM3_کوڈ_ورقہ i_ورقہ, u32 i_module_وتھ);
#جے_انت // d_m3RecordBacktraces

فراغ        ڈالو_کوڈ_ورقہ (IM3_کوڈ_ورقہ* io_list, IM3_کوڈ_ورقہ i_کوڈ_ورقہ);
IM3_کوڈ_ورقہ نکالو_کوڈ_ورقہ (IM3_کوڈ_ورقہ* io_list);

IM3_کوڈ_ورقہ Get_سرا_کوڈ_ورقہ (IM3_کوڈ_ورقہ i_list); // i_list = NULL is valid
u32         گنتی_کوڈ_ورقے (IM3_کوڈ_ورقہ i_list); // i_list = NULL is valid

#إذا d_m3_ریکارڈ_Backtraces
bool شامل_اے_PC (IM3_کوڈ_ورقہ i_ورقہ, pc_t i_pc);
bool Map_PCTo_وتھ (IM3_کوڈ_ورقہ i_ورقہ, pc_t i_pc, u32* o_module_وتھ);
#جے_انت // d_m3RecordBacktraces

#جے_تعریف_شدہ DEBUG
فراغ dump_کوڈ_ورقہ (IM3_کوڈ_ورقہ i_کوڈ_ورقہ, pc_t i_شروع_PC);
#جے_انت

#تعریف_کرو Emit_لفظ(ورقہ, val) Emit_لفظ_impl(ورقہ, (فراغ*)(val))

//---------------------------------------------------------------------------------------------------------------------------------

#إذا d_m3_ریکارڈ_Backtraces

تعريفنوع بنية M3_کوڈ_Map_اندراج {
    u32 pc_وتھ;
    u32 module_وتھ;
} M3_کوڈ_Map_اندراج;

تعريفنوع بنية M3_کوڈ_Mapping_ورقہ {
    pc_t           ادھار_PC;
    u32            ناپ;
    u32            سمرتھا;
    M3_کوڈ_Map_اندراج entries[];
} M3_کوڈ_Mapping_ورقہ;

#جے_انت // d_m3RecordBacktraces

d_m3_سرا_Extern_C

#جے_انت // m3_code_h
