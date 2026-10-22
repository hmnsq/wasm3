//
//  m3_exec_defs.h
//
//  Created by Steven Massey on 5/1/19.
//  Copyright © 2019 Steven Massey. All rights reserved.
//

#جے_بنا_تعریف m3_exec_defs_h
#تعریف_کرو m3_exec_defs_h

#ضمن "m3_core.h"

d_m3BeginExternC

#تعریف_کرو m3_Mem_انکڑے(mem)              (u8*)(((M3_حافظہ_سرخی*)(mem))+1)
#تعریف_کرو m3MemRuntime(mem)           (((M3_حافظہ_سرخی*)(mem))->runtime)
#تعریف_کرو m3MemInfo(mem)              (((M3_حافظہ_سرخی*)(mem))->حافظہ)

#تعریف_کرو d_m3_ادھار_Op_Sig               pc_t _pc, m3stack_t _sp, M3_حافظہ_سرخی * _mem, m3reg_t _r0
#تعریف_کرو d_m3_ادھار_Op_دلیلاں              _sp, _mem, _r0
#تعریف_کرو d_m3_ادھار_Op_All_دلیلاں           _pc, _sp, _mem, _r0
#تعریف_کرو d_m3_ادھار_Op_Default_دلیلاں       0
#تعریف_کرو d_m3_ادھار_Clear_Registers      _r0 = 0;
#تعریف_کرو d_m3_ادھار_Cstr                ""

#تعریف_کرو d_m3ExpOpSig(...)           d_m3_ادھار_Op_Sig, __VA_ARGS__
#تعریف_کرو d_m3_Exp_Op_دلیلاں(...)          d_m3_ادھار_Op_دلیلاں, __VA_ARGS__
#تعریف_کرو d_m3_Exp_Op_All_دلیلاں(...)       d_m3_ادھار_Op_All_دلیلاں, __VA_ARGS__
#تعریف_کرو d_m3_Exp_Op_Default_دلیلاں(...)   d_m3_ادھار_Op_Default_دلیلاں, __VA_ARGS__
#تعریف_کرو d_m3ExpClearRegisters(...)  d_m3_ادھار_Clear_Registers; __VA_ARGS__

#إذا d_m3HasFloat
#  define d_m3OpSig                 d_m3ExpOpSig            (f64 _fp0)
#  define d_m3_Op_دلیلاں                d_m3_Exp_Op_دلیلاں           (_fp0)
#  define d_m3_Op_All_دلیلاں             d_m3_Exp_Op_All_دلیلاں        (_fp0)
#  define d_m3_Op_Default_دلیلاں         d_m3_Exp_Op_Default_دلیلاں    (0.)
#  define d_m3ClearRegisters        d_m3ExpClearRegisters   (_fp0 = 0.;)
#وإلا
#  define d_m3OpSig                 d_m3_ادھار_Op_Sig
#  define d_m3_Op_دلیلاں                d_m3_ادھار_Op_دلیلاں
#  define d_m3_Op_All_دلیلاں             d_m3_ادھار_Op_All_دلیلاں
#  define d_m3_Op_Default_دلیلاں         d_m3_ادھار_Op_Default_دلیلاں
#  define d_m3ClearRegisters        d_m3_ادھار_Clear_Registers
#جے_انت


#تعریف_کرو d_m3RetSig                  ساكن مضمن m3ret_t vectorcall
#إذا (d_m3EnableOpProfiling || d_m3EnableOpTracing)
تعريفنوع m3ret_t(vectorcall* IM3Operation)(d_m3OpSig, cstr_t i_operationName);
#  define d_m3Op(NAME)              M3_NO_UBSAN d_m3RetSig op_##NAME (d_m3OpSig, cstr_t i_operationName)

#  define اگلا_Op_Impl()              ((IM3Operation)(* _pc))(_pc + 1, d_m3_Op_دلیلاں, __FUNCTION__)
#  define jumpOpImpl(PC)            ((IM3Operation)(*  PC))( PC + 1, d_m3_Op_دلیلاں, __FUNCTION__)
#وإلا
تعريفنوع m3ret_t(vectorcall* IM3Operation)(d_m3OpSig);
#  define d_m3Op(NAME)              M3_NO_UBSAN d_m3RetSig op_##NAME (d_m3OpSig)

#  define اگلا_Op_Impl()              ((IM3Operation)(* _pc))(_pc + 1, d_m3_Op_دلیلاں)
#  define jumpOpImpl(PC)            ((IM3Operation)(*  PC))( PC + 1, d_m3_Op_دلیلاں)
#جے_انت

#تعریف_کرو اگلا_Op_سدھا()              M3_MUSTTAIL أرجع اگلا_Op_Impl()
#تعریف_کرو jump_Op_سدھا(PC)            M3_MUSTTAIL أرجع jumpOpImpl((pc_t)(PC))

#إذا (d_m3EnableOpProfiling || d_m3EnableOpTracing)
d_m3RetSig لگاتار_کوڈ (d_m3OpSig, cstr_t i_operationName)
#وإلا
d_m3RetSig لگاتار_کوڈ (d_m3OpSig)
#جے_انت
{
    اگلا_Op_سدھا();
}

d_m3_سرا_Extern_C

#جے_انت // m3_exec_defs_h
