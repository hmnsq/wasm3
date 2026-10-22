//
//  m3_info.h
//
//  Created by Steven Massey on 12/6/19.
//  Copyright © 2019 Steven Massey. All rights reserved.
//

#ifndef m3_info_h
#define m3_info_h

#ಅನ್ತರ್ಭಾವ "m3_compile.h"

d_m3BeginExternC

ಶೂನ್ಯ ProfileHit (cstr_t i_operationName);

#ifdef DEBUG

ಶೂನ್ಯ        dump_type_ರಾಶಿ (IM3Compilation o);
ಶೂನ್ಯ        log_opcode (IM3Compilation o, m3opcode_t i_opcode);
ಧ್ರುವ ವರ್ಣ* get_indention_string (IM3Compilation o);
ಶೂನ್ಯ        log_emit (IM3Compilation o, IM3Operation i_operation);

cstr_t      SPrintFuncTypeSignature (IM3FuncType i_funcType);

#ಅನ್ಯಥಾ // DEBUG

#  define   dump_type_ರಾಶಿ(...)      {}
#  define   log_opcode(...)           {}
#  define   get_indention_string(...) ""
#  define   emit_ರಾಶಿ_dump(...)      {}
#  define   log_emit(...)             {}

#endif // DEBUG

d_m3EndExternC

#endif // m3_info_h
