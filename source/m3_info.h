//
//  m3_info.h
//
//  Created by Steven Massey on 12/6/19.
//  Copyright © 2019 Steven Massey. All rights reserved.
//

#اگر_غیر_تعریف_شدہ m3_info_h
#تعریف_کرو m3_info_h

#شامل "m3_compile.h"

d_m3BeginExternC

خالی Profile_کامیابی (cstr_t i_operationName);

#اگر_تعریف_شدہ DEBUG

خالی        dump_type_ڈھیر (IM3Compilation o);
خالی        log_opcode (IM3Compilation o, m3opcode_t i_opcode);
ثابت حرف* get_indention_string (IM3Compilation o);
خالی        log_emit (IM3Compilation o, IM3Operation i_operation);

cstr_t      SPrintFuncTypeSignature (IM3FuncType i_funcType);

#ورنہ // DEBUG

#  define   dump_type_ڈھیر(...)      {}
#  define   log_opcode(...)           {}
#  define   get_indention_string(...) ""
#  define   emit_ڈھیر_dump(...)      {}
#  define   log_emit(...)             {}

#اگر_ختم // DEBUG

d_m3_انتہا_Extern_C

#اگر_ختم // m3_info_h
