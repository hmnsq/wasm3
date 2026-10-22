//
//  m3_exec_defs.h
//
//  Created by Steven Massey on 5/1/19.
//  Copyright © 2019 Steven Massey. All rights reserved.
//

#अगर_अघोषित m3_exec_defs_h
#घोषणा m3_exec_defs_h

#समावेश "m3_core.h"

d_m3BeginExternC

#घोषणा m3_Mem_तथ्य(mem)              (u8*)(((M3MemoryHeader*)(mem))+1)
#घोषणा m3MemRuntime(mem)           (((M3MemoryHeader*)(mem))->runtime)
#घोषणा m3MemInfo(mem)              (((M3MemoryHeader*)(mem))->memory)

#घोषणा d_m3BaseOpSig               pc_t _pc, m3stack_t _sp, M3MemoryHeader * _mem, m3reg_t _r0
#घोषणा d_m3BaseOpArgs              _sp, _mem, _r0
#घोषणा d_m3BaseOpAllArgs           _pc, _sp, _mem, _r0
#घोषणा d_m3BaseOpDefaultArgs       0
#घोषणा d_m3BaseClearRegisters      _r0 = 0;
#घोषणा d_m3BaseCstr                ""

#घोषणा d_m3ExpOpSig(...)           d_m3BaseOpSig, __VA_ARGS__
#घोषणा d_m3ExpOpArgs(...)          d_m3BaseOpArgs, __VA_ARGS__
#घोषणा d_m3ExpOpAllArgs(...)       d_m3BaseOpAllArgs, __VA_ARGS__
#घोषणा d_m3ExpOpDefaultArgs(...)   d_m3BaseOpDefaultArgs, __VA_ARGS__
#घोषणा d_m3ExpClearRegisters(...)  d_m3BaseClearRegisters; __VA_ARGS__

#अगर d_m3HasFloat
#  define d_m3OpSig                 d_m3ExpOpSig            (f64 _fp0)
#  define d_m3OpArgs                d_m3ExpOpArgs           (_fp0)
#  define d_m3OpAllArgs             d_m3ExpOpAllArgs        (_fp0)
#  define d_m3OpDefaultArgs         d_m3ExpOpDefaultArgs    (0.)
#  define d_m3ClearRegisters        d_m3ExpClearRegisters   (_fp0 = 0.;)
#अन्यथा
#  define d_m3OpSig                 d_m3BaseOpSig
#  define d_m3OpArgs                d_m3BaseOpArgs
#  define d_m3OpAllArgs             d_m3BaseOpAllArgs
#  define d_m3OpDefaultArgs         d_m3BaseOpDefaultArgs
#  define d_m3ClearRegisters        d_m3BaseClearRegisters
#पूर्ण_अगर


#घोषणा d_m3RetSig                  अटल अंतरभूत m3ret_t vectorcall
#अगर (d_m3EnableOpProfiling || d_m3EnableOpTracing)
प्रकार m3ret_t(vectorcall* IM3Operation)(d_m3OpSig, cstr_t i_operationName);
#  define d_m3Op(NAME)              M3_NO_UBSAN d_m3RetSig op_##NAME (d_m3OpSig, cstr_t i_operationName)

#  define अग्र_Op_Impl()              ((IM3Operation)(* _pc))(_pc + 1, d_m3OpArgs, __FUNCTION__)
#  define jumpOpImpl(PC)            ((IM3Operation)(*  PC))( PC + 1, d_m3OpArgs, __FUNCTION__)
#अन्यथा
प्रकार m3ret_t(vectorcall* IM3Operation)(d_m3OpSig);
#  define d_m3Op(NAME)              M3_NO_UBSAN d_m3RetSig op_##NAME (d_m3OpSig)

#  define अग्र_Op_Impl()              ((IM3Operation)(* _pc))(_pc + 1, d_m3OpArgs)
#  define jumpOpImpl(PC)            ((IM3Operation)(*  PC))( PC + 1, d_m3OpArgs)
#पूर्ण_अगर

#घोषणा अग्र_Op_Direct()              M3_MUSTTAIL वापस अग्र_Op_Impl()
#घोषणा jumpOpDirect(PC)            M3_MUSTTAIL वापस jumpOpImpl((pc_t)(PC))

#अगर (d_m3EnableOpProfiling || d_m3EnableOpTracing)
d_m3RetSig RunCode (d_m3OpSig, cstr_t i_operationName)
#अन्यथा
d_m3RetSig RunCode (d_m3OpSig)
#पूर्ण_अगर
{
    अग्र_Op_Direct();
}

d_m3EndExternC

#पूर्ण_अगर // m3_exec_defs_h
