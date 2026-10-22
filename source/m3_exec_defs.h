//
//  m3_exec_defs.h
//
//  Created by Steven Massey on 5/1/19.
//  Copyright © 2019 Steven Massey. All rights reserved.
//

#ವ್ಯಾಖ್ಯಾನಿತವಾಗದಿದ್ದರೆ m3_exec_defs_h
#ವ್ಯಾಖ್ಯಾನಿಸು m3_exec_defs_h

#ಅನ್ತರ್ಭಾವ "m3_core.h"

d_m3BeginExternC

#ವ್ಯಾಖ್ಯಾನಿಸು m3_Mem_ದತ್ತಾಂಶ(mem)              (u8*)(((M3_ಸ್ಮರಣೆ_ಶೀರ್ಷಿಕೆ*)(mem))+1)
#ವ್ಯಾಖ್ಯಾನಿಸು m3MemRuntime(mem)           (((M3_ಸ್ಮರಣೆ_ಶೀರ್ಷಿಕೆ*)(mem))->runtime)
#ವ್ಯಾಖ್ಯಾನಿಸು m3MemInfo(mem)              (((M3_ಸ್ಮರಣೆ_ಶೀರ್ಷಿಕೆ*)(mem))->ಸ್ಮರಣೆ)

#ವ್ಯಾಖ್ಯಾನಿಸು d_m3_ತಳ_Op_Sig               pc_t _pc, m3stack_t _sp, M3_ಸ್ಮರಣೆ_ಶೀರ್ಷಿಕೆ * _mem, m3reg_t _r0
#ವ್ಯಾಖ್ಯಾನಿಸು d_m3_ತಳ_Op_ವಾದಗಳು              _sp, _mem, _r0
#ವ್ಯಾಖ್ಯಾನಿಸು d_m3_ತಳ_Op_All_ವಾದಗಳು           _pc, _sp, _mem, _r0
#ವ್ಯಾಖ್ಯಾನಿಸು d_m3_ತಳ_Op_Default_ವಾದಗಳು       0
#ವ್ಯಾಖ್ಯಾನಿಸು d_m3_ತಳ_Clear_Registers      _r0 = 0;
#ವ್ಯಾಖ್ಯಾನಿಸು d_m3_ತಳ_Cstr                ""

#ವ್ಯಾಖ್ಯಾನಿಸು d_m3ExpOpSig(...)           d_m3_ತಳ_Op_Sig, __VA_ARGS__
#ವ್ಯಾಖ್ಯಾನಿಸು d_m3_Exp_Op_ವಾದಗಳು(...)          d_m3_ತಳ_Op_ವಾದಗಳು, __VA_ARGS__
#ವ್ಯಾಖ್ಯಾನಿಸು d_m3_Exp_Op_All_ವಾದಗಳು(...)       d_m3_ತಳ_Op_All_ವಾದಗಳು, __VA_ARGS__
#ವ್ಯಾಖ್ಯಾನಿಸು d_m3_Exp_Op_Default_ವಾದಗಳು(...)   d_m3_ತಳ_Op_Default_ವಾದಗಳು, __VA_ARGS__
#ವ್ಯಾಖ್ಯಾನಿಸು d_m3ExpClearRegisters(...)  d_m3_ತಳ_Clear_Registers; __VA_ARGS__

#ಆದರೆ d_m3HasFloat
#  define d_m3OpSig                 d_m3ExpOpSig            (f64 _fp0)
#  define d_m3_Op_ವಾದಗಳು                d_m3_Exp_Op_ವಾದಗಳು           (_fp0)
#  define d_m3_Op_All_ವಾದಗಳು             d_m3_Exp_Op_All_ವಾದಗಳು        (_fp0)
#  define d_m3_Op_Default_ವಾದಗಳು         d_m3_Exp_Op_Default_ವಾದಗಳು    (0.)
#  define d_m3ClearRegisters        d_m3ExpClearRegisters   (_fp0 = 0.;)
#ಇಲ್ಲದಿದ್ದರೆ
#  define d_m3OpSig                 d_m3_ತಳ_Op_Sig
#  define d_m3_Op_ವಾದಗಳು                d_m3_ತಳ_Op_ವಾದಗಳು
#  define d_m3_Op_All_ವಾದಗಳು             d_m3_ತಳ_Op_All_ವಾದಗಳು
#  define d_m3_Op_Default_ವಾದಗಳು         d_m3_ತಳ_Op_Default_ವಾದಗಳು
#  define d_m3ClearRegisters        d_m3_ತಳ_Clear_Registers
#ಆದರೆ_ಅಂತ್ಯ


#ವ್ಯಾಖ್ಯಾನಿಸು d_m3RetSig                  ಸ್ಥಿರ ಸಾಲೊಳಗೆ m3ret_t vectorcall
#ಆದರೆ (d_m3EnableOpProfiling || d_m3EnableOpTracing)
ಪ್ರಕಾರವ್ಯಾಖ್ಯೆ m3ret_t(vectorcall* IM3Operation)(d_m3OpSig, cstr_t i_operationName);
#  define d_m3Op(NAME)              M3_NO_UBSAN d_m3RetSig op_##NAME (d_m3OpSig, cstr_t i_operationName)

#  define ಮುಂದಿನ_Op_Impl()              ((IM3Operation)(* _pc))(_pc + 1, d_m3_Op_ವಾದಗಳು, __FUNCTION__)
#  define jumpOpImpl(PC)            ((IM3Operation)(*  PC))( PC + 1, d_m3_Op_ವಾದಗಳು, __FUNCTION__)
#ಇಲ್ಲದಿದ್ದರೆ
ಪ್ರಕಾರವ್ಯಾಖ್ಯೆ m3ret_t(vectorcall* IM3Operation)(d_m3OpSig);
#  define d_m3Op(NAME)              M3_NO_UBSAN d_m3RetSig op_##NAME (d_m3OpSig)

#  define ಮುಂದಿನ_Op_Impl()              ((IM3Operation)(* _pc))(_pc + 1, d_m3_Op_ವಾದಗಳು)
#  define jumpOpImpl(PC)            ((IM3Operation)(*  PC))( PC + 1, d_m3_Op_ವಾದಗಳು)
#ಆದರೆ_ಅಂತ್ಯ

#ವ್ಯಾಖ್ಯಾನಿಸು ಮುಂದಿನ_Op_ನೇರ()              M3_MUSTTAIL ಹಿಂತಿರುಗಿ ಮುಂದಿನ_Op_Impl()
#ವ್ಯಾಖ್ಯಾನಿಸು jump_Op_ನೇರ(PC)            M3_MUSTTAIL ಹಿಂತಿರುಗಿ jumpOpImpl((pc_t)(PC))

#ಆದರೆ (d_m3EnableOpProfiling || d_m3EnableOpTracing)
d_m3RetSig ಸತತ_ಸಂಕೇತ (d_m3OpSig, cstr_t i_operationName)
#ಇಲ್ಲದಿದ್ದರೆ
d_m3RetSig ಸತತ_ಸಂಕೇತ (d_m3OpSig)
#ಆದರೆ_ಅಂತ್ಯ
{
    ಮುಂದಿನ_Op_ನೇರ();
}

d_m3_ಕೊನೆ_Extern_C

#ಆದರೆ_ಅಂತ್ಯ // m3_exec_defs_h
