//
//  m3_code.h
//
//  Created by Steven Massey on 4/19/19.
//  Copyright © 2019 Steven Massey. All rights reserved.
//

#اگر_تعریف_نشده m3_کد_h
#تعریف_کن m3_کد_h

#ضمن "m3_core.h"

d_m3BeginExternC

تعريفنوع بنية M3_کد_صفحه {
    M3_کد_صفحه_سرآیند info;
    کد_t           کد[1];
} M3_کد_صفحه;

تعريفنوع M3_کد_صفحه* IM3_کد_صفحه;


IM3_کد_صفحه         New_کد_صفحه (IM3Runtime i_runtime, u32 i_min_رقم_سطرها);

فراغ                Free_کد_صفحات (IM3_کد_صفحه* io_list);

u32                 رقم_Free_سطرها (IM3_کد_صفحه i_صفحه);
pc_t                Get_صفحه_شروع_PC (IM3_کد_صفحه i_صفحه);
pc_t                Get_صفحه_PC (IM3_کد_صفحه i_صفحه);
فراغ                Emit_کلمه_impl (IM3_کد_صفحه i_صفحه, فراغ* i_کلمه);
فراغ                EmitWord32 (IM3_کد_صفحه i_صفحه, u32 i_کلمه);
فراغ                EmitWord64 (IM3_کد_صفحه i_صفحه, u64 i_کلمه);
#إذا d_m3_رکورد_Backtraces
فراغ Emit_Mapping_مدخل (IM3_کد_صفحه i_صفحه, u32 i_module_جابه_جایی);
#پایان_اگر // d_m3RecordBacktraces

فراغ        افزودن_کد_صفحه (IM3_کد_صفحه* io_list, IM3_کد_صفحه i_کد_صفحه);
IM3_کد_صفحه برداشت_کد_صفحه (IM3_کد_صفحه* io_list);

IM3_کد_صفحه Get_انتها_کد_صفحه (IM3_کد_صفحه i_list); // i_list = NULL is valid
u32         شمار_کد_صفحات (IM3_کد_صفحه i_list); // i_list = NULL is valid

#إذا d_m3_رکورد_Backtraces
bool شامل_است_PC (IM3_کد_صفحه i_صفحه, pc_t i_pc);
bool Map_PCTo_جابه_جایی (IM3_کد_صفحه i_صفحه, pc_t i_pc, u32* o_module_جابه_جایی);
#پایان_اگر // d_m3RecordBacktraces

#اگر_تعریف_شده DEBUG
فراغ dump_کد_صفحه (IM3_کد_صفحه i_کد_صفحه, pc_t i_شروع_PC);
#پایان_اگر

#تعریف_کن Emit_کلمه(صفحه, val) Emit_کلمه_impl(صفحه, (فراغ*)(val))

//---------------------------------------------------------------------------------------------------------------------------------

#إذا d_m3_رکورد_Backtraces

تعريفنوع بنية M3_کد_Map_مدخل {
    u32 pc_جابه_جایی;
    u32 module_جابه_جایی;
} M3_کد_Map_مدخل;

تعريفنوع بنية M3_کد_Mapping_صفحه {
    pc_t           پایه_PC;
    u32            اندازه;
    u32            ظرفیت;
    M3_کد_Map_مدخل entries[];
} M3_کد_Mapping_صفحه;

#پایان_اگر // d_m3RecordBacktraces

d_m3_انتها_Extern_C

#پایان_اگر // m3_code_h
