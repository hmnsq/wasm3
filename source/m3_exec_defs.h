//
//  m3_exec_defs.h
//
//  Created by Steven Massey on 5/1/19.
//  Copyright © 2019 Steven Massey. All rights reserved.
//

#جيڪڏهن_وصف_نه_ٿيل m3_exec_defs_h
#وصف_ڪريو m3_exec_defs_h

#ضمن "m3_core.h"

d_m3BeginExternC

#وصف_ڪريو m3_Mem_معلومات(mem)              (u8*)(((M3_يادگيري_سرو*)(mem))+1)
#وصف_ڪريو m3MemRuntime(mem)           (((M3_يادگيري_سرو*)(mem))->runtime)
#وصف_ڪريو m3MemInfo(mem)              (((M3_يادگيري_سرو*)(mem))->يادگيري)

#وصف_ڪريو d_m3_بنياد_Op_Sig               pc_t _pc, m3stack_t _sp, M3_يادگيري_سرو * _mem, m3reg_t _r0
#وصف_ڪريو d_m3_بنياد_Op_دليلون              _sp, _mem, _r0
#وصف_ڪريو d_m3_بنياد_Op_All_دليلون           _pc, _sp, _mem, _r0
#وصف_ڪريو d_m3_بنياد_Op_Default_دليلون       0
#وصف_ڪريو d_m3_بنياد_Clear_Registers      _r0 = 0;
#وصف_ڪريو d_m3_بنياد_Cstr                ""

#وصف_ڪريو d_m3ExpOpSig(...)           d_m3_بنياد_Op_Sig, __VA_ARGS__
#وصف_ڪريو d_m3_Exp_Op_دليلون(...)          d_m3_بنياد_Op_دليلون, __VA_ARGS__
#وصف_ڪريو d_m3_Exp_Op_All_دليلون(...)       d_m3_بنياد_Op_All_دليلون, __VA_ARGS__
#وصف_ڪريو d_m3_Exp_Op_Default_دليلون(...)   d_m3_بنياد_Op_Default_دليلون, __VA_ARGS__
#وصف_ڪريو d_m3ExpClearRegisters(...)  d_m3_بنياد_Clear_Registers; __VA_ARGS__

#إذا d_m3HasFloat
#  define d_m3OpSig                 d_m3ExpOpSig            (f64 _fp0)
#  define d_m3_Op_دليلون                d_m3_Exp_Op_دليلون           (_fp0)
#  define d_m3_Op_All_دليلون             d_m3_Exp_Op_All_دليلون        (_fp0)
#  define d_m3_Op_Default_دليلون         d_m3_Exp_Op_Default_دليلون    (0.)
#  define d_m3ClearRegisters        d_m3ExpClearRegisters   (_fp0 = 0.;)
#وإلا
#  define d_m3OpSig                 d_m3_بنياد_Op_Sig
#  define d_m3_Op_دليلون                d_m3_بنياد_Op_دليلون
#  define d_m3_Op_All_دليلون             d_m3_بنياد_Op_All_دليلون
#  define d_m3_Op_Default_دليلون         d_m3_بنياد_Op_Default_دليلون
#  define d_m3ClearRegisters        d_m3_بنياد_Clear_Registers
#جيڪڏهن_پورو


#وصف_ڪريو d_m3RetSig                  ساكن مضمن m3ret_t vectorcall
#إذا (d_m3EnableOpProfiling || d_m3EnableOpTracing)
تعريفنوع m3ret_t(vectorcall* IM3Operation)(d_m3OpSig, cstr_t i_operationName);
#  define d_m3Op(NAME)              M3_NO_UBSAN d_m3RetSig op_##NAME (d_m3OpSig, cstr_t i_operationName)

#  define ايندڙ_Op_Impl()              ((IM3Operation)(* _pc))(_pc + 1, d_m3_Op_دليلون, __FUNCTION__)
#  define jumpOpImpl(PC)            ((IM3Operation)(*  PC))( PC + 1, d_m3_Op_دليلون, __FUNCTION__)
#وإلا
تعريفنوع m3ret_t(vectorcall* IM3Operation)(d_m3OpSig);
#  define d_m3Op(NAME)              M3_NO_UBSAN d_m3RetSig op_##NAME (d_m3OpSig)

#  define ايندڙ_Op_Impl()              ((IM3Operation)(* _pc))(_pc + 1, d_m3_Op_دليلون)
#  define jumpOpImpl(PC)            ((IM3Operation)(*  PC))( PC + 1, d_m3_Op_دليلون)
#جيڪڏهن_پورو

#وصف_ڪريو ايندڙ_Op_سڌو()              M3_MUSTTAIL أرجع ايندڙ_Op_Impl()
#وصف_ڪريو jump_Op_سڌو(PC)            M3_MUSTTAIL أرجع jumpOpImpl((pc_t)(PC))

#إذا (d_m3EnableOpProfiling || d_m3EnableOpTracing)
d_m3RetSig لاڳيتو_ڪوڊ (d_m3OpSig, cstr_t i_operationName)
#وإلا
d_m3RetSig لاڳيتو_ڪوڊ (d_m3OpSig)
#جيڪڏهن_پورو
{
    ايندڙ_Op_سڌو();
}

d_m3_پڇاڙي_Extern_C

#جيڪڏهن_پورو // m3_exec_defs_h
