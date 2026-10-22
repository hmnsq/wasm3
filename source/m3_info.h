//
//  m3_info.h
//
//  Created by Steven Massey on 12/6/19.
//  Copyright © 2019 Steven Massey. All rights reserved.
//

#ifndef m3_info_h
#define m3_info_h

#അന്തര്ഭാവ "m3_compile.h"

d_m3BeginExternC

ശൂന്യം ProfileHit (cstr_t i_operationName);

#ifdef DEBUG

ശൂന്യം        dump_type_അട്ടി (IM3Compilation o);
ശൂന്യം        log_opcode (IM3Compilation o, m3opcode_t i_opcode);
സ്ഥിരം അക്ഷരം* get_indention_string (IM3Compilation o);
ശൂന്യം        log_emit (IM3Compilation o, IM3Operation i_operation);

cstr_t      SPrintFuncTypeSignature (IM3FuncType i_funcType);

#അല്ലെങ്കിൽ // DEBUG

#  define   dump_type_അട്ടി(...)      {}
#  define   log_opcode(...)           {}
#  define   get_indention_string(...) ""
#  define   emit_അട്ടി_dump(...)      {}
#  define   log_emit(...)             {}

#endif // DEBUG

d_m3EndExternC

#endif // m3_info_h
