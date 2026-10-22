//
//  m3_exec_defs.h
//
//  Created by Steven Massey on 5/1/19.
//  Copyright © 2019 Steven Massey. All rights reserved.
//

#यदि_अपरिभाषितम् m3_exec_defs_h
#परिभाषय m3_exec_defs_h

#अन्तर्भाव "m3_core.h"

d_m3BeginExternC

#परिभाषय m3_Mem_दत्त(mem)              (u8*)(((M3_स्मृतिः_शीर्षकम्*)(mem))+1)
#परिभाषय m3MemRuntime(mem)           (((M3_स्मृतिः_शीर्षकम्*)(mem))->runtime)
#परिभाषय m3MemInfo(mem)              (((M3_स्मृतिः_शीर्षकम्*)(mem))->स्मृतिः)

#परिभाषय d_m3_आधारः_Op_Sig               pc_t _pc, m3stack_t _sp, M3_स्मृतिः_शीर्षकम् * _mem, m3reg_t _r0
#परिभाषय d_m3_आधारः_Op_तर्काः              _sp, _mem, _r0
#परिभाषय d_m3_आधारः_Op_All_तर्काः           _pc, _sp, _mem, _r0
#परिभाषय d_m3_आधारः_Op_Default_तर्काः       0
#परिभाषय d_m3_आधारः_Clear_Registers      _r0 = 0;
#परिभाषय d_m3_आधारः_Cstr                ""

#परिभाषय d_m3ExpOpSig(...)           d_m3_आधारः_Op_Sig, __VA_ARGS__
#परिभाषय d_m3_Exp_Op_तर्काः(...)          d_m3_आधारः_Op_तर्काः, __VA_ARGS__
#परिभाषय d_m3_Exp_Op_All_तर्काः(...)       d_m3_आधारः_Op_All_तर्काः, __VA_ARGS__
#परिभाषय d_m3_Exp_Op_Default_तर्काः(...)   d_m3_आधारः_Op_Default_तर्काः, __VA_ARGS__
#परिभाषय d_m3ExpClearRegisters(...)  d_m3_आधारः_Clear_Registers; __VA_ARGS__

#यदि d_m3HasFloat
#  define d_m3OpSig                 d_m3ExpOpSig            (f64 _fp0)
#  define d_m3_Op_तर्काः                d_m3_Exp_Op_तर्काः           (_fp0)
#  define d_m3_Op_All_तर्काः             d_m3_Exp_Op_All_तर्काः        (_fp0)
#  define d_m3_Op_Default_तर्काः         d_m3_Exp_Op_Default_तर्काः    (0.)
#  define d_m3ClearRegisters        d_m3ExpClearRegisters   (_fp0 = 0.;)
#अन्यथा
#  define d_m3OpSig                 d_m3_आधारः_Op_Sig
#  define d_m3_Op_तर्काः                d_m3_आधारः_Op_तर्काः
#  define d_m3_Op_All_तर्काः             d_m3_आधारः_Op_All_तर्काः
#  define d_m3_Op_Default_तर्काः         d_m3_आधारः_Op_Default_तर्काः
#  define d_m3ClearRegisters        d_m3_आधारः_Clear_Registers
#यदि_अन्तः


#परिभाषय d_m3RetSig                  स्थिर अन्तःस्थ m3ret_t vectorcall
#यदि (d_m3EnableOpProfiling || d_m3EnableOpTracing)
प्रकारनिर्देश m3ret_t(vectorcall* IM3Operation)(d_m3OpSig, cstr_t i_operationName);
#  define d_m3Op(NAME)              M3_NO_UBSAN d_m3RetSig op_##NAME (d_m3OpSig, cstr_t i_operationName)

#  define अग्र_Op_Impl()              ((IM3Operation)(* _pc))(_pc + 1, d_m3_Op_तर्काः, __FUNCTION__)
#  define jumpOpImpl(PC)            ((IM3Operation)(*  PC))( PC + 1, d_m3_Op_तर्काः, __FUNCTION__)
#अन्यथा
प्रकारनिर्देश m3ret_t(vectorcall* IM3Operation)(d_m3OpSig);
#  define d_m3Op(NAME)              M3_NO_UBSAN d_m3RetSig op_##NAME (d_m3OpSig)

#  define अग्र_Op_Impl()              ((IM3Operation)(* _pc))(_pc + 1, d_m3_Op_तर्काः)
#  define jumpOpImpl(PC)            ((IM3Operation)(*  PC))( PC + 1, d_m3_Op_तर्काः)
#यदि_अन्तः

#परिभाषय अग्र_Op_प्रत्यक्षम्()              M3_MUSTTAIL प्रत्यागम अग्र_Op_Impl()
#परिभाषय jump_Op_प्रत्यक्षम्(PC)            M3_MUSTTAIL प्रत्यागम jumpOpImpl((pc_t)(PC))

#यदि (d_m3EnableOpProfiling || d_m3EnableOpTracing)
d_m3RetSig सततम्_संहिता (d_m3OpSig, cstr_t i_operationName)
#अन्यथा
d_m3RetSig सततम्_संहिता (d_m3OpSig)
#यदि_अन्तः
{
    अग्र_Op_प्रत्यक्षम्();
}

d_m3_अन्तः_Extern_C

#यदि_अन्तः // m3_exec_defs_h
