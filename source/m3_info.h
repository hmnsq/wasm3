//
//  m3_info.h
//
//  Created by Steven Massey on 12/6/19.
//  Copyright © 2019 Steven Massey. All rights reserved.
//

#اگر_تعریف_نشده m3_info_h
#تعریف_کن m3_info_h

#ضمن "m3_compile.h"

d_m3BeginExternC

فراغ Profile_اصابت (cstr_t i_operationName);

#اگر_تعریف_شده DEBUG

فراغ        dump_type_پشته (IM3Compilation o);
فراغ        log_opcode (IM3Compilation o, m3opcode_t i_opcode);
ثابت حرف* get_indention_string (IM3Compilation o);
فراغ        log_emit (IM3Compilation o, IM3Operation i_operation);

cstr_t      SPrintFuncTypeSignature (IM3FuncType i_funcType);

#وإلا // DEBUG

#  define   dump_type_پشته(...)      {}
#  define   log_opcode(...)           {}
#  define   get_indention_string(...) ""
#  define   emit_پشته_dump(...)      {}
#  define   log_emit(...)             {}

#پایان_اگر // DEBUG

d_m3_انتها_Extern_C

#پایان_اگر // m3_info_h
