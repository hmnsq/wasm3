//
//  m3_info.h
//
//  Created by Steven Massey on 12/6/19.
//  Copyright © 2019 Steven Massey. All rights reserved.
//

#جے_بنا_تعریف m3_info_h
#تعریف_کرو m3_info_h

#ضمن "m3_compile.h"

d_m3BeginExternC

فراغ Profile_کامیابی (cstr_t i_operationName);

#جے_تعریف_شدہ DEBUG

فراغ        dump_type_ڈھیر (IM3Compilation o);
فراغ        log_opcode (IM3Compilation o, m3opcode_t i_opcode);
ثابت حرف* get_indention_string (IM3Compilation o);
فراغ        log_emit (IM3Compilation o, IM3Operation i_operation);

cstr_t      SPrintFuncTypeSignature (IM3FuncType i_funcType);

#وإلا // DEBUG

#  define   dump_type_ڈھیر(...)      {}
#  define   log_opcode(...)           {}
#  define   get_indention_string(...) ""
#  define   emit_ڈھیر_dump(...)      {}
#  define   log_emit(...)             {}

#جے_انت // DEBUG

d_m3_سرا_Extern_C

#جے_انت // m3_info_h
