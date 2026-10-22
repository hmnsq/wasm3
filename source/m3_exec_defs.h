//
//  m3_exec_defs.h
//
//  Created by Steven Massey on 5/1/19.
//  Copyright © 2019 Steven Massey. All rights reserved.
//

#ਜੇ_ਅਪਰਿਭਾਸ਼ਿਤ m3_exec_defs_h
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ m3_exec_defs_h

#ਅਨ੍ਤਰ੍ਭਾਵ "m3_core.h"

d_m3BeginExternC

#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ m3_Mem_ਅੰਕੜੇ(mem)              (u8*)(((M3_ਸਮ੍ਰਿਤੀ_ਸਿਰਲੇਖ*)(mem))+1)
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ m3MemRuntime(mem)           (((M3_ਸਮ੍ਰਿਤੀ_ਸਿਰਲੇਖ*)(mem))->runtime)
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ m3MemInfo(mem)              (((M3_ਸਮ੍ਰਿਤੀ_ਸਿਰਲੇਖ*)(mem))->ਸਮ੍ਰਿਤੀ)

#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ d_m3_ਅਧਾਰ_Op_Sig               pc_t _pc, m3stack_t _sp, M3_ਸਮ੍ਰਿਤੀ_ਸਿਰਲੇਖ * _mem, m3reg_t _r0
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ d_m3_ਅਧਾਰ_Op_ਦਲੀਲਾਂ              _sp, _mem, _r0
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ d_m3_ਅਧਾਰ_Op_All_ਦਲੀਲਾਂ           _pc, _sp, _mem, _r0
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ d_m3_ਅਧਾਰ_Op_Default_ਦਲੀਲਾਂ       0
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ d_m3_ਅਧਾਰ_Clear_Registers      _r0 = 0;
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ d_m3_ਅਧਾਰ_Cstr                ""

#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ d_m3ExpOpSig(...)           d_m3_ਅਧਾਰ_Op_Sig, __VA_ARGS__
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ d_m3_Exp_Op_ਦਲੀਲਾਂ(...)          d_m3_ਅਧਾਰ_Op_ਦਲੀਲਾਂ, __VA_ARGS__
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ d_m3_Exp_Op_All_ਦਲੀਲਾਂ(...)       d_m3_ਅਧਾਰ_Op_All_ਦਲੀਲਾਂ, __VA_ARGS__
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ d_m3_Exp_Op_Default_ਦਲੀਲਾਂ(...)   d_m3_ਅਧਾਰ_Op_Default_ਦਲੀਲਾਂ, __VA_ARGS__
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ d_m3ExpClearRegisters(...)  d_m3_ਅਧਾਰ_Clear_Registers; __VA_ARGS__

#ਜੇ d_m3HasFloat
#  define d_m3OpSig                 d_m3ExpOpSig            (f64 _fp0)
#  define d_m3_Op_ਦਲੀਲਾਂ                d_m3_Exp_Op_ਦਲੀਲਾਂ           (_fp0)
#  define d_m3_Op_All_ਦਲੀਲਾਂ             d_m3_Exp_Op_All_ਦਲੀਲਾਂ        (_fp0)
#  define d_m3_Op_Default_ਦਲੀਲਾਂ         d_m3_Exp_Op_Default_ਦਲੀਲਾਂ    (0.)
#  define d_m3ClearRegisters        d_m3ExpClearRegisters   (_fp0 = 0.;)
#ਨਹੀਂਤਾਂ
#  define d_m3OpSig                 d_m3_ਅਧਾਰ_Op_Sig
#  define d_m3_Op_ਦਲੀਲਾਂ                d_m3_ਅਧਾਰ_Op_ਦਲੀਲਾਂ
#  define d_m3_Op_All_ਦਲੀਲਾਂ             d_m3_ਅਧਾਰ_Op_All_ਦਲੀਲਾਂ
#  define d_m3_Op_Default_ਦਲੀਲਾਂ         d_m3_ਅਧਾਰ_Op_Default_ਦਲੀਲਾਂ
#  define d_m3ClearRegisters        d_m3_ਅਧਾਰ_Clear_Registers
#ਜੇ_ਅੰਤ


#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ d_m3RetSig                  ਸਥਿਰ ਸਤਰਅੰਦਰ m3ret_t vectorcall
#ਜੇ (d_m3EnableOpProfiling || d_m3EnableOpTracing)
ਕਿਸਮਪਰਿਭਾਸ਼ਾ m3ret_t(vectorcall* IM3Operation)(d_m3OpSig, cstr_t i_operationName);
#  define d_m3Op(NAME)              M3_NO_UBSAN d_m3RetSig op_##NAME (d_m3OpSig, cstr_t i_operationName)

#  define ਅਗਲਾ_Op_Impl()              ((IM3Operation)(* _pc))(_pc + 1, d_m3_Op_ਦਲੀਲਾਂ, __FUNCTION__)
#  define jumpOpImpl(PC)            ((IM3Operation)(*  PC))( PC + 1, d_m3_Op_ਦਲੀਲਾਂ, __FUNCTION__)
#ਨਹੀਂਤਾਂ
ਕਿਸਮਪਰਿਭਾਸ਼ਾ m3ret_t(vectorcall* IM3Operation)(d_m3OpSig);
#  define d_m3Op(NAME)              M3_NO_UBSAN d_m3RetSig op_##NAME (d_m3OpSig)

#  define ਅਗਲਾ_Op_Impl()              ((IM3Operation)(* _pc))(_pc + 1, d_m3_Op_ਦਲੀਲਾਂ)
#  define jumpOpImpl(PC)            ((IM3Operation)(*  PC))( PC + 1, d_m3_Op_ਦਲੀਲਾਂ)
#ਜੇ_ਅੰਤ

#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ ਅਗਲਾ_Op_ਸਿੱਧਾ()              M3_MUSTTAIL ਵਾਪਸ ਅਗਲਾ_Op_Impl()
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ jump_Op_ਸਿੱਧਾ(PC)            M3_MUSTTAIL ਵਾਪਸ jumpOpImpl((pc_t)(PC))

#ਜੇ (d_m3EnableOpProfiling || d_m3EnableOpTracing)
d_m3RetSig ਲੜੀਵਾਰ_ਕੋਡ (d_m3OpSig, cstr_t i_operationName)
#ਨਹੀਂਤਾਂ
d_m3RetSig ਲੜੀਵਾਰ_ਕੋਡ (d_m3OpSig)
#ਜੇ_ਅੰਤ
{
    ਅਗਲਾ_Op_ਸਿੱਧਾ();
}

d_m3_ਸਿਰਾ_Extern_C

#ਜੇ_ਅੰਤ // m3_exec_defs_h
