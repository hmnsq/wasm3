//
//  m3_info.h
//
//  Created by Steven Massey on 12/6/19.
//  Copyright © 2019 Steven Massey. All rights reserved.
//

#ifndef m3_info_h
#define m3_info_h

#অন্তর্ভাব "m3_compile.h"

d_m3BeginExternC

শূন্য ProfileHit (cstr_t i_operationName);

#ifdef DEBUG

শূন্য        dump_type_স্তূপ (IM3Compilation o);
শূন্য        log_opcode (IM3Compilation o, m3opcode_t i_opcode);
ধ্রুব বর্ণ* get_indention_string (IM3Compilation o);
শূন্য        log_emit (IM3Compilation o, IM3Operation i_operation);

cstr_t      SPrintFuncTypeSignature (IM3FuncType i_funcType);

#অন্যথা // DEBUG

#  define   dump_type_স্তূপ(...)      {}
#  define   log_opcode(...)           {}
#  define   get_indention_string(...) ""
#  define   emit_স্তূপ_dump(...)      {}
#  define   log_emit(...)             {}

#endif // DEBUG

d_m3EndExternC

#endif // m3_info_h
