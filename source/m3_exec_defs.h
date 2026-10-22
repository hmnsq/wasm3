//
//  m3_exec_defs.h
//
//  Created by Steven Massey on 5/1/19.
//  Copyright © 2019 Steven Massey. All rights reserved.
//

#నిర్వచితం_కాకపోతే m3_exec_defs_h
#నిర్వచించు m3_exec_defs_h

#అన్తర్భావ "m3_core.h"

d_m3BeginExternC

#నిర్వచించు m3_Mem_దత్తాంశం(mem)              (u8*)(((M3_జ్ఞాపకం_శీర్షిక*)(mem))+1)
#నిర్వచించు m3MemRuntime(mem)           (((M3_జ్ఞాపకం_శీర్షిక*)(mem))->runtime)
#నిర్వచించు m3MemInfo(mem)              (((M3_జ్ఞాపకం_శీర్షిక*)(mem))->జ్ఞాపకం)

#నిర్వచించు d_m3_పునాది_Op_Sig               pc_t _pc, m3stack_t _sp, M3_జ్ఞాపకం_శీర్షిక * _mem, m3reg_t _r0
#నిర్వచించు d_m3_పునాది_Op_వాదనలు              _sp, _mem, _r0
#నిర్వచించు d_m3_పునాది_Op_All_వాదనలు           _pc, _sp, _mem, _r0
#నిర్వచించు d_m3_పునాది_Op_Default_వాదనలు       0
#నిర్వచించు d_m3_పునాది_Clear_Registers      _r0 = 0;
#నిర్వచించు d_m3_పునాది_Cstr                ""

#నిర్వచించు d_m3ExpOpSig(...)           d_m3_పునాది_Op_Sig, __VA_ARGS__
#నిర్వచించు d_m3_Exp_Op_వాదనలు(...)          d_m3_పునాది_Op_వాదనలు, __VA_ARGS__
#నిర్వచించు d_m3_Exp_Op_All_వాదనలు(...)       d_m3_పునాది_Op_All_వాదనలు, __VA_ARGS__
#నిర్వచించు d_m3_Exp_Op_Default_వాదనలు(...)   d_m3_పునాది_Op_Default_వాదనలు, __VA_ARGS__
#నిర్వచించు d_m3ExpClearRegisters(...)  d_m3_పునాది_Clear_Registers; __VA_ARGS__

#ఒకవేళ d_m3HasFloat
#  define d_m3OpSig                 d_m3ExpOpSig            (f64 _fp0)
#  define d_m3_Op_వాదనలు                d_m3_Exp_Op_వాదనలు           (_fp0)
#  define d_m3_Op_All_వాదనలు             d_m3_Exp_Op_All_వాదనలు        (_fp0)
#  define d_m3_Op_Default_వాదనలు         d_m3_Exp_Op_Default_వాదనలు    (0.)
#  define d_m3ClearRegisters        d_m3ExpClearRegisters   (_fp0 = 0.;)
#లేకపోతే
#  define d_m3OpSig                 d_m3_పునాది_Op_Sig
#  define d_m3_Op_వాదనలు                d_m3_పునాది_Op_వాదనలు
#  define d_m3_Op_All_వాదనలు             d_m3_పునాది_Op_All_వాదనలు
#  define d_m3_Op_Default_వాదనలు         d_m3_పునాది_Op_Default_వాదనలు
#  define d_m3ClearRegisters        d_m3_పునాది_Clear_Registers
#అయితే_ముగింపు


#నిర్వచించు d_m3RetSig                  స్థిర అంతఃవరుస m3ret_t vectorcall
#ఒకవేళ (d_m3EnableOpProfiling || d_m3EnableOpTracing)
రకనిర్వచనం m3ret_t(vectorcall* IM3Operation)(d_m3OpSig, cstr_t i_operationName);
#  define d_m3Op(NAME)              M3_NO_UBSAN d_m3RetSig op_##NAME (d_m3OpSig, cstr_t i_operationName)

#  define తదుపరి_Op_Impl()              ((IM3Operation)(* _pc))(_pc + 1, d_m3_Op_వాదనలు, __FUNCTION__)
#  define jumpOpImpl(PC)            ((IM3Operation)(*  PC))( PC + 1, d_m3_Op_వాదనలు, __FUNCTION__)
#లేకపోతే
రకనిర్వచనం m3ret_t(vectorcall* IM3Operation)(d_m3OpSig);
#  define d_m3Op(NAME)              M3_NO_UBSAN d_m3RetSig op_##NAME (d_m3OpSig)

#  define తదుపరి_Op_Impl()              ((IM3Operation)(* _pc))(_pc + 1, d_m3_Op_వాదనలు)
#  define jumpOpImpl(PC)            ((IM3Operation)(*  PC))( PC + 1, d_m3_Op_వాదనలు)
#అయితే_ముగింపు

#నిర్వచించు తదుపరి_Op_ప్రత్యక్ష()              M3_MUSTTAIL తిరిగి తదుపరి_Op_Impl()
#నిర్వచించు jump_Op_ప్రత్యక్ష(PC)            M3_MUSTTAIL తిరిగి jumpOpImpl((pc_t)(PC))

#ఒకవేళ (d_m3EnableOpProfiling || d_m3EnableOpTracing)
d_m3RetSig నిరంతరం_సంకేతం (d_m3OpSig, cstr_t i_operationName)
#లేకపోతే
d_m3RetSig నిరంతరం_సంకేతం (d_m3OpSig)
#అయితే_ముగింపు
{
    తదుపరి_Op_ప్రత్యక్ష();
}

d_m3_చివర_Extern_C

#అయితే_ముగింపు // m3_exec_defs_h
