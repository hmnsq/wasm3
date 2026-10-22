//
//  m3_info.h
//
//  Created by Steven Massey on 12/6/19.
//  Copyright © 2019 Steven Massey. All rights reserved.
//

#که_نه_تعریف_شوی m3_info_h
#تعریف_کړه m3_info_h

#ضمن "m3_compile.h"

d_m3BeginExternC

فراغ Profile_موندنه (cstr_t i_operationName);

#که_تعریف_شوی DEBUG

فراغ        dump_type_ډېری (IM3Compilation o);
فراغ        log_opcode (IM3Compilation o, m3opcode_t i_opcode);
ثابت حرف* get_indention_string (IM3Compilation o);
فراغ        log_emit (IM3Compilation o, IM3Operation i_operation);

cstr_t      SPrintFuncTypeSignature (IM3FuncType i_funcType);

#وإلا // DEBUG

#  define   dump_type_ډېری(...)      {}
#  define   log_opcode(...)           {}
#  define   get_indention_string(...) ""
#  define   emit_ډېری_dump(...)      {}
#  define   log_emit(...)             {}

#د_که_پای // DEBUG

d_m3_پای_ټکی_Extern_C

#د_که_پای // m3_info_h
