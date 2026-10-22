//
//  m3_info.h
//
//  Created by Steven Massey on 12/6/19.
//  Copyright © 2019 Steven Massey. All rights reserved.
//

#ifndef m3_info_h
#define m3_info_h

#شامل "m3_compile.h"

d_m3BeginExternC

تهی ProfileHit (cstr_t i_operationName);

#ifdef DEBUG

تهی        dump_type_پشته (IM3Compilation o);
تهی        log_opcode (IM3Compilation o, m3opcode_t i_opcode);
ثابت نویسه* get_indention_string (IM3Compilation o);
تهی        log_emit (IM3Compilation o, IM3Operation i_operation);

cstr_t      SPrintFuncTypeSignature (IM3FuncType i_funcType);

#وگرنه // DEBUG

#  define   dump_type_پشته(...)      {}
#  define   log_opcode(...)           {}
#  define   get_indention_string(...) ""
#  define   emit_پشته_dump(...)      {}
#  define   log_emit(...)             {}

#endif // DEBUG

d_m3EndExternC

#endif // m3_info_h
