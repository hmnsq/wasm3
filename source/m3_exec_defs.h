//
//  m3_exec_defs.h
//
//  Created by Steven Massey on 5/1/19.
//  Copyright © 2019 Steven Massey. All rights reserved.
//

#اگر_غیر_تعریف_شدہ m3_exec_defs_h
#تعریف_کرو m3_exec_defs_h

#شامل "m3_core.h"

d_m3BeginExternC

#تعریف_کرو m3_Mem_ڈیٹا(mem)              (u8*)(((M3_حافظہ_سرخی*)(mem))+1)
#تعریف_کرو m3MemRuntime(mem)           (((M3_حافظہ_سرخی*)(mem))->runtime)
#تعریف_کرو m3MemInfo(mem)              (((M3_حافظہ_سرخی*)(mem))->حافظہ)

#تعریف_کرو d_m3_بنیاد_Op_Sig               pc_t _pc, m3stack_t _sp, M3_حافظہ_سرخی * _mem, m3reg_t _r0
#تعریف_کرو d_m3_بنیاد_Op_دلائل              _sp, _mem, _r0
#تعریف_کرو d_m3_بنیاد_Op_All_دلائل           _pc, _sp, _mem, _r0
#تعریف_کرو d_m3_بنیاد_Op_Default_دلائل       0
#تعریف_کرو d_m3_بنیاد_Clear_Registers      _r0 = 0;
#تعریف_کرو d_m3_بنیاد_Cstr                ""

#تعریف_کرو d_m3ExpOpSig(...)           d_m3_بنیاد_Op_Sig, __VA_ARGS__
#تعریف_کرو d_m3_Exp_Op_دلائل(...)          d_m3_بنیاد_Op_دلائل, __VA_ARGS__
#تعریف_کرو d_m3_Exp_Op_All_دلائل(...)       d_m3_بنیاد_Op_All_دلائل, __VA_ARGS__
#تعریف_کرو d_m3_Exp_Op_Default_دلائل(...)   d_m3_بنیاد_Op_Default_دلائل, __VA_ARGS__
#تعریف_کرو d_m3ExpClearRegisters(...)  d_m3_بنیاد_Clear_Registers; __VA_ARGS__

#اگر d_m3HasFloat
#  define d_m3OpSig                 d_m3ExpOpSig            (f64 _fp0)
#  define d_m3_Op_دلائل                d_m3_Exp_Op_دلائل           (_fp0)
#  define d_m3_Op_All_دلائل             d_m3_Exp_Op_All_دلائل        (_fp0)
#  define d_m3_Op_Default_دلائل         d_m3_Exp_Op_Default_دلائل    (0.)
#  define d_m3ClearRegisters        d_m3ExpClearRegisters   (_fp0 = 0.;)
#ورنہ
#  define d_m3OpSig                 d_m3_بنیاد_Op_Sig
#  define d_m3_Op_دلائل                d_m3_بنیاد_Op_دلائل
#  define d_m3_Op_All_دلائل             d_m3_بنیاد_Op_All_دلائل
#  define d_m3_Op_Default_دلائل         d_m3_بنیاد_Op_Default_دلائل
#  define d_m3ClearRegisters        d_m3_بنیاد_Clear_Registers
#اگر_ختم


#تعریف_کرو d_m3RetSig                  ساکن اندرونی m3ret_t vectorcall
#اگر (d_m3EnableOpProfiling || d_m3EnableOpTracing)
قسمتعریف m3ret_t(vectorcall* IM3Operation)(d_m3OpSig, cstr_t i_operationName);
#  define d_m3Op(NAME)              M3_NO_UBSAN d_m3RetSig op_##NAME (d_m3OpSig, cstr_t i_operationName)

#  define اگلا_Op_Impl()              ((IM3Operation)(* _pc))(_pc + 1, d_m3_Op_دلائل, __FUNCTION__)
#  define jumpOpImpl(PC)            ((IM3Operation)(*  PC))( PC + 1, d_m3_Op_دلائل, __FUNCTION__)
#ورنہ
قسمتعریف m3ret_t(vectorcall* IM3Operation)(d_m3OpSig);
#  define d_m3Op(NAME)              M3_NO_UBSAN d_m3RetSig op_##NAME (d_m3OpSig)

#  define اگلا_Op_Impl()              ((IM3Operation)(* _pc))(_pc + 1, d_m3_Op_دلائل)
#  define jumpOpImpl(PC)            ((IM3Operation)(*  PC))( PC + 1, d_m3_Op_دلائل)
#اگر_ختم

#تعریف_کرو اگلا_Op_براہ_راست()              M3_MUSTTAIL واپس اگلا_Op_Impl()
#تعریف_کرو jump_Op_براہ_راست(PC)            M3_MUSTTAIL واپس jumpOpImpl((pc_t)(PC))

#اگر (d_m3EnableOpProfiling || d_m3EnableOpTracing)
d_m3RetSig تسلسل_رمز (d_m3OpSig, cstr_t i_operationName)
#ورنہ
d_m3RetSig تسلسل_رمز (d_m3OpSig)
#اگر_ختم
{
    اگلا_Op_براہ_راست();
}

d_m3_انتہا_Extern_C

#اگر_ختم // m3_exec_defs_h
