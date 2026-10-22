//
//  m3_info.h
//
//  Created by Steven Massey on 12/6/19.
//  Copyright © 2019 Steven Massey. All rights reserved.
//

#ifndef m3_info_h
#define m3_info_h

#ଅନ୍ତର୍ଭାଵ "m3_compile.h"

d_m3BeginExternC

ଶୂନ୍ୟ ProfileHit (cstr_t i_operationName);

#ifdef DEBUG

ଶୂନ୍ୟ        dump_type_ଗଦା (IM3Compilation o);
ଶୂନ୍ୟ        log_opcode (IM3Compilation o, m3opcode_t i_opcode);
ଅଚଳ ଅକ୍ଷର* get_indention_string (IM3Compilation o);
ଶୂନ୍ୟ        log_emit (IM3Compilation o, IM3Operation i_operation);

cstr_t      SPrintFuncTypeSignature (IM3FuncType i_funcType);

#ଅନ୍ୟଥା // DEBUG

#  define   dump_type_ଗଦା(...)      {}
#  define   log_opcode(...)           {}
#  define   get_indention_string(...) ""
#  define   emit_ଗଦା_dump(...)      {}
#  define   log_emit(...)             {}

#endif // DEBUG

d_m3EndExternC

#endif // m3_info_h
