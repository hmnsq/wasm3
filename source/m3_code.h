//
//  m3_code.h
//
//  Created by Steven Massey on 4/19/19.
//  Copyright © 2019 Steven Massey. All rights reserved.
//

#اگر_غیر_تعریف_شدہ m3_رمز_h
#تعریف_کرو m3_رمز_h

#شامل "m3_core.h"

d_m3BeginExternC

قسمتعریف ساخت M3_رمز_صفحہ {
    M3_رمز_صفحہ_سرخی info;
    رمز_t           رمز[1];
} M3_رمز_صفحہ;

قسمتعریف M3_رمز_صفحہ* IM3_رمز_صفحہ;


IM3_رمز_صفحہ         New_رمز_صفحہ (IM3Runtime i_runtime, u32 i_min_عدد_سطریں);

خالی                Free_رمز_صفحات (IM3_رمز_صفحہ* io_list);

u32                 عدد_Free_سطریں (IM3_رمز_صفحہ i_صفحہ);
pc_t                Get_صفحہ_آغاز_PC (IM3_رمز_صفحہ i_صفحہ);
pc_t                Get_صفحہ_PC (IM3_رمز_صفحہ i_صفحہ);
خالی                Emit_لفظ_impl (IM3_رمز_صفحہ i_صفحہ, خالی* i_لفظ);
خالی                EmitWord32 (IM3_رمز_صفحہ i_صفحہ, u32 i_لفظ);
خالی                EmitWord64 (IM3_رمز_صفحہ i_صفحہ, u64 i_لفظ);
#اگر d_m3_ریکارڈ_Backtraces
خالی Emit_Mapping_اندراج (IM3_رمز_صفحہ i_صفحہ, u32 i_module_انحراف);
#اگر_ختم // d_m3RecordBacktraces

خالی        ڈالو_رمز_صفحہ (IM3_رمز_صفحہ* io_list, IM3_رمز_صفحہ i_رمز_صفحہ);
IM3_رمز_صفحہ نکالو_رمز_صفحہ (IM3_رمز_صفحہ* io_list);

IM3_رمز_صفحہ Get_انتہا_رمز_صفحہ (IM3_رمز_صفحہ i_list); // i_list = NULL is valid
u32         گنتی_رمز_صفحات (IM3_رمز_صفحہ i_list); // i_list = NULL is valid

#اگر d_m3_ریکارڈ_Backtraces
bool شامل_ہے_PC (IM3_رمز_صفحہ i_صفحہ, pc_t i_pc);
bool Map_PCTo_انحراف (IM3_رمز_صفحہ i_صفحہ, pc_t i_pc, u32* o_module_انحراف);
#اگر_ختم // d_m3RecordBacktraces

#اگر_تعریف_شدہ DEBUG
خالی dump_رمز_صفحہ (IM3_رمز_صفحہ i_رمز_صفحہ, pc_t i_آغاز_PC);
#اگر_ختم

#تعریف_کرو Emit_لفظ(صفحہ, val) Emit_لفظ_impl(صفحہ, (خالی*)(val))

//---------------------------------------------------------------------------------------------------------------------------------

#اگر d_m3_ریکارڈ_Backtraces

قسمتعریف ساخت M3_رمز_Map_اندراج {
    u32 pc_انحراف;
    u32 module_انحراف;
} M3_رمز_Map_اندراج;

قسمتعریف ساخت M3_رمز_Mapping_صفحہ {
    pc_t           بنیاد_PC;
    u32            سائز;
    u32            استعداد;
    M3_رمز_Map_اندراج entries[];
} M3_رمز_Mapping_صفحہ;

#اگر_ختم // d_m3RecordBacktraces

d_m3_انتہا_Extern_C

#اگر_ختم // m3_code_h
