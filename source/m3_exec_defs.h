//
//  m3_exec_defs.h
//
//  Created by Steven Massey on 5/1/19.
//  Copyright © 2019 Steven Massey. All rights reserved.
//

#اگر_تعریف_نشده m3_exec_defs_h
#تعریف_کن m3_exec_defs_h

#ضمن "m3_core.h"

d_m3BeginExternC

#تعریف_کن m3_Mem_داده(mem)              (u8*)(((M3_حافظه_سرآیند*)(mem))+1)
#تعریف_کن m3MemRuntime(mem)           (((M3_حافظه_سرآیند*)(mem))->runtime)
#تعریف_کن m3MemInfo(mem)              (((M3_حافظه_سرآیند*)(mem))->حافظه)

#تعریف_کن d_m3_پایه_Op_Sig               pc_t _pc, m3stack_t _sp, M3_حافظه_سرآیند * _mem, m3reg_t _r0
#تعریف_کن d_m3_پایه_Op_آرگومانها              _sp, _mem, _r0
#تعریف_کن d_m3_پایه_Op_All_آرگومانها           _pc, _sp, _mem, _r0
#تعریف_کن d_m3_پایه_Op_Default_آرگومانها       0
#تعریف_کن d_m3_پایه_Clear_Registers      _r0 = 0;
#تعریف_کن d_m3_پایه_Cstr                ""

#تعریف_کن d_m3ExpOpSig(...)           d_m3_پایه_Op_Sig, __VA_ARGS__
#تعریف_کن d_m3_Exp_Op_آرگومانها(...)          d_m3_پایه_Op_آرگومانها, __VA_ARGS__
#تعریف_کن d_m3_Exp_Op_All_آرگومانها(...)       d_m3_پایه_Op_All_آرگومانها, __VA_ARGS__
#تعریف_کن d_m3_Exp_Op_Default_آرگومانها(...)   d_m3_پایه_Op_Default_آرگومانها, __VA_ARGS__
#تعریف_کن d_m3ExpClearRegisters(...)  d_m3_پایه_Clear_Registers; __VA_ARGS__

#إذا d_m3HasFloat
#  define d_m3OpSig                 d_m3ExpOpSig            (f64 _fp0)
#  define d_m3_Op_آرگومانها                d_m3_Exp_Op_آرگومانها           (_fp0)
#  define d_m3_Op_All_آرگومانها             d_m3_Exp_Op_All_آرگومانها        (_fp0)
#  define d_m3_Op_Default_آرگومانها         d_m3_Exp_Op_Default_آرگومانها    (0.)
#  define d_m3ClearRegisters        d_m3ExpClearRegisters   (_fp0 = 0.;)
#وإلا
#  define d_m3OpSig                 d_m3_پایه_Op_Sig
#  define d_m3_Op_آرگومانها                d_m3_پایه_Op_آرگومانها
#  define d_m3_Op_All_آرگومانها             d_m3_پایه_Op_All_آرگومانها
#  define d_m3_Op_Default_آرگومانها         d_m3_پایه_Op_Default_آرگومانها
#  define d_m3ClearRegisters        d_m3_پایه_Clear_Registers
#پایان_اگر


#تعریف_کن d_m3RetSig                  ساكن مضمن m3ret_t vectorcall
#إذا (d_m3EnableOpProfiling || d_m3EnableOpTracing)
تعريفنوع m3ret_t(vectorcall* IM3Operation)(d_m3OpSig, cstr_t i_operationName);
#  define d_m3Op(NAME)              M3_NO_UBSAN d_m3RetSig op_##NAME (d_m3OpSig, cstr_t i_operationName)

#  define بعدی_Op_Impl()              ((IM3Operation)(* _pc))(_pc + 1, d_m3_Op_آرگومانها, __FUNCTION__)
#  define jumpOpImpl(PC)            ((IM3Operation)(*  PC))( PC + 1, d_m3_Op_آرگومانها, __FUNCTION__)
#وإلا
تعريفنوع m3ret_t(vectorcall* IM3Operation)(d_m3OpSig);
#  define d_m3Op(NAME)              M3_NO_UBSAN d_m3RetSig op_##NAME (d_m3OpSig)

#  define بعدی_Op_Impl()              ((IM3Operation)(* _pc))(_pc + 1, d_m3_Op_آرگومانها)
#  define jumpOpImpl(PC)            ((IM3Operation)(*  PC))( PC + 1, d_m3_Op_آرگومانها)
#پایان_اگر

#تعریف_کن بعدی_Op_مستقیم()              M3_MUSTTAIL أرجع بعدی_Op_Impl()
#تعریف_کن jump_Op_مستقیم(PC)            M3_MUSTTAIL أرجع jumpOpImpl((pc_t)(PC))

#إذا (d_m3EnableOpProfiling || d_m3EnableOpTracing)
d_m3RetSig پیاپی_کد (d_m3OpSig, cstr_t i_operationName)
#وإلا
d_m3RetSig پیاپی_کد (d_m3OpSig)
#پایان_اگر
{
    بعدی_Op_مستقیم();
}

d_m3_انتها_Extern_C

#پایان_اگر // m3_exec_defs_h
