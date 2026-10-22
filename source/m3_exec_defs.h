//
//  m3_exec_defs.h
//
//  Created by Steven Massey on 5/1/19.
//  Copyright © 2019 Steven Massey. All rights reserved.
//

#که_نه_تعریف_شوی m3_exec_defs_h
#تعریف_کړه m3_exec_defs_h

#ضمن "m3_core.h"

d_m3BeginExternC

#تعریف_کړه m3_Mem_معلومات(mem)              (u8*)(((M3_حافظه_سرلیک*)(mem))+1)
#تعریف_کړه m3MemRuntime(mem)           (((M3_حافظه_سرلیک*)(mem))->runtime)
#تعریف_کړه m3MemInfo(mem)              (((M3_حافظه_سرلیک*)(mem))->حافظه)

#تعریف_کړه d_m3_بنسټ_Op_Sig               pc_t _pc, m3stack_t _sp, M3_حافظه_سرلیک * _mem, m3reg_t _r0
#تعریف_کړه d_m3_بنسټ_Op_دلیلونه              _sp, _mem, _r0
#تعریف_کړه d_m3_بنسټ_Op_All_دلیلونه           _pc, _sp, _mem, _r0
#تعریف_کړه d_m3_بنسټ_Op_Default_دلیلونه       0
#تعریف_کړه d_m3_بنسټ_Clear_Registers      _r0 = 0;
#تعریف_کړه d_m3_بنسټ_Cstr                ""

#تعریف_کړه d_m3ExpOpSig(...)           d_m3_بنسټ_Op_Sig, __VA_ARGS__
#تعریف_کړه d_m3_Exp_Op_دلیلونه(...)          d_m3_بنسټ_Op_دلیلونه, __VA_ARGS__
#تعریف_کړه d_m3_Exp_Op_All_دلیلونه(...)       d_m3_بنسټ_Op_All_دلیلونه, __VA_ARGS__
#تعریف_کړه d_m3_Exp_Op_Default_دلیلونه(...)   d_m3_بنسټ_Op_Default_دلیلونه, __VA_ARGS__
#تعریف_کړه d_m3ExpClearRegisters(...)  d_m3_بنسټ_Clear_Registers; __VA_ARGS__

#إذا d_m3HasFloat
#  define d_m3OpSig                 d_m3ExpOpSig            (f64 _fp0)
#  define d_m3_Op_دلیلونه                d_m3_Exp_Op_دلیلونه           (_fp0)
#  define d_m3_Op_All_دلیلونه             d_m3_Exp_Op_All_دلیلونه        (_fp0)
#  define d_m3_Op_Default_دلیلونه         d_m3_Exp_Op_Default_دلیلونه    (0.)
#  define d_m3ClearRegisters        d_m3ExpClearRegisters   (_fp0 = 0.;)
#وإلا
#  define d_m3OpSig                 d_m3_بنسټ_Op_Sig
#  define d_m3_Op_دلیلونه                d_m3_بنسټ_Op_دلیلونه
#  define d_m3_Op_All_دلیلونه             d_m3_بنسټ_Op_All_دلیلونه
#  define d_m3_Op_Default_دلیلونه         d_m3_بنسټ_Op_Default_دلیلونه
#  define d_m3ClearRegisters        d_m3_بنسټ_Clear_Registers
#د_که_پای


#تعریف_کړه d_m3RetSig                  ساكن مضمن m3ret_t vectorcall
#إذا (d_m3EnableOpProfiling || d_m3EnableOpTracing)
تعريفنوع m3ret_t(vectorcall* IM3Operation)(d_m3OpSig, cstr_t i_operationName);
#  define d_m3Op(NAME)              M3_NO_UBSAN d_m3RetSig op_##NAME (d_m3OpSig, cstr_t i_operationName)

#  define بل_Op_Impl()              ((IM3Operation)(* _pc))(_pc + 1, d_m3_Op_دلیلونه, __FUNCTION__)
#  define jumpOpImpl(PC)            ((IM3Operation)(*  PC))( PC + 1, d_m3_Op_دلیلونه, __FUNCTION__)
#وإلا
تعريفنوع m3ret_t(vectorcall* IM3Operation)(d_m3OpSig);
#  define d_m3Op(NAME)              M3_NO_UBSAN d_m3RetSig op_##NAME (d_m3OpSig)

#  define بل_Op_Impl()              ((IM3Operation)(* _pc))(_pc + 1, d_m3_Op_دلیلونه)
#  define jumpOpImpl(PC)            ((IM3Operation)(*  PC))( PC + 1, d_m3_Op_دلیلونه)
#د_که_پای

#تعریف_کړه بل_Op_مستقیم()              M3_MUSTTAIL أرجع بل_Op_Impl()
#تعریف_کړه jump_Op_مستقیم(PC)            M3_MUSTTAIL أرجع jumpOpImpl((pc_t)(PC))

#إذا (d_m3EnableOpProfiling || d_m3EnableOpTracing)
d_m3RetSig پرلپسې_کوډ (d_m3OpSig, cstr_t i_operationName)
#وإلا
d_m3RetSig پرلپسې_کوډ (d_m3OpSig)
#د_که_پای
{
    بل_Op_مستقیم();
}

d_m3_پای_ټکی_Extern_C

#د_که_پای // m3_exec_defs_h
