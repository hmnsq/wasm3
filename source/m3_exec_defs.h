//
//  m3_exec_defs.h
//
//  Created by Steven Massey on 5/1/19.
//  Copyright © 2019 Steven Massey. All rights reserved.
//

#ifndef m3_exec_defs_h
#define m3_exec_defs_h

#include "m3_core.h"

d_m3BeginExternC

#define m3MemData(mem)              (u8*)(((M3MemoryHeader*)(mem))+1)
#define m3MemRuntime(mem)           (((M3MemoryHeader*)(mem))->runtime)
#define m3MemInfo(mem)              (((M3MemoryHeader*)(mem))->memory)

#define d_m3BaseOpSig               pc_t _pc, m3stack_t _sp, M3MemoryHeader * _mem, m3reg_t _r0
#define d_m3_Base_Op_मगा_कातविरान              _sp, _mem, _r0
#define d_m3_Base_Op_All_मगा_कातविरान           _pc, _sp, _mem, _r0
#define d_m3_Base_Op_Default_मगा_कातविरान       0
#define d_m3BaseClearRegisters      _r0 = 0;
#define d_m3BaseCstr                ""

#define d_m3ExpOpSig(...)           d_m3BaseOpSig, __VA_ARGS__
#define d_m3_Exp_Op_मगा_कातविरान(...)          d_m3_Base_Op_मगा_कातविरान, __VA_ARGS__
#define d_m3_Exp_Op_All_मगा_कातविरान(...)       d_m3_Base_Op_All_मगा_कातविरान, __VA_ARGS__
#define d_m3_Exp_Op_Default_मगा_कातविरान(...)   d_m3_Base_Op_Default_मगा_कातविरान, __VA_ARGS__
#define d_m3ExpClearRegisters(...)  d_m3BaseClearRegisters; __VA_ARGS__

#if d_m3HasFloat
#  define d_m3OpSig                 d_m3ExpOpSig            (f64 _fp0)
#  define d_m3_Op_मगा_कातविरान                d_m3_Exp_Op_मगा_कातविरान           (_fp0)
#  define d_m3_Op_All_मगा_कातविरान             d_m3_Exp_Op_All_मगा_कातविरान        (_fp0)
#  define d_m3_Op_Default_मगा_कातविरान         d_m3_Exp_Op_Default_मगा_कातविरान    (0.)
#  define d_m3ClearRegisters        d_m3ExpClearRegisters   (_fp0 = 0.;)
#else
#  define d_m3OpSig                 d_m3BaseOpSig
#  define d_m3_Op_मगा_कातविरान                d_m3_Base_Op_मगा_कातविरान
#  define d_m3_Op_All_मगा_कातविरान             d_m3_Base_Op_All_मगा_कातविरान
#  define d_m3_Op_Default_मगा_कातविरान         d_m3_Base_Op_Default_मगा_कातविरान
#  define d_m3ClearRegisters        d_m3BaseClearRegisters
#endif


#define d_m3RetSig                  static inline m3ret_t vectorcall
#if (d_m3EnableOpProfiling || d_m3EnableOpTracing)
typedef m3ret_t(vectorcall* IM3Operation)(d_m3OpSig, cstr_t i_operationName);
#  define d_m3Op(NAME)              M3_NO_UBSAN d_m3RetSig op_##NAME (d_m3OpSig, cstr_t i_operationName)

#  define nextOpImpl()              ((IM3Operation)(* _pc))(_pc + 1, d_m3_Op_मगा_कातविरान, __FUNCTION__)
#  define jumpOpImpl(PC)            ((IM3Operation)(*  PC))( PC + 1, d_m3_Op_मगा_कातविरान, __FUNCTION__)
#else
typedef m3ret_t(vectorcall* IM3Operation)(d_m3OpSig);
#  define d_m3Op(NAME)              M3_NO_UBSAN d_m3RetSig op_##NAME (d_m3OpSig)

#  define nextOpImpl()              ((IM3Operation)(* _pc))(_pc + 1, d_m3_Op_मगा_कातविरान)
#  define jumpOpImpl(PC)            ((IM3Operation)(*  PC))( PC + 1, d_m3_Op_मगा_कातविरान)
#endif

#define nextOpDirect()              M3_MUSTTAIL return nextOpImpl()
#define jumpOpDirect(PC)            M3_MUSTTAIL return jumpOpImpl((pc_t)(PC))

#if (d_m3EnableOpProfiling || d_m3EnableOpTracing)
d_m3RetSig RunCode (d_m3OpSig, cstr_t i_operationName)
#else
d_m3RetSig RunCode (d_m3OpSig)
#endif
{
    nextOpDirect();
}

d_m3EndExternC

#endif // m3_exec_defs_h
