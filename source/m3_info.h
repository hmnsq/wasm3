//
//  m3_info.h
//
//  Created by Steven Massey on 12/6/19.
//  Copyright © 2019 Steven Massey. All rights reserved.
//

#ifndef m3_info_h
#define m3_info_h

#ضمن "m3_compile.h"

d_m3BeginExternC

فراغ ProfileHit (cstr_t i_operationName);

#ifdef DEBUG

فراغ        dump_type_مكدس (IM3Compilation o);
فراغ        log_opcode (IM3Compilation o, m3opcode_t i_opcode);
ثابت حرف* get_indention_string (IM3Compilation o);
فراغ        log_emit (IM3Compilation o, IM3Operation i_operation);

cstr_t      SPrintFuncTypeSignature (IM3FuncType i_funcType);

#وإلا // DEBUG

#  define   dump_type_مكدس(...)      {}
#  define   log_opcode(...)           {}
#  define   get_indention_string(...) ""
#  define   emit_مكدس_dump(...)      {}
#  define   log_emit(...)             {}

#endif // DEBUG

d_m3EndExternC

#endif // m3_info_h
