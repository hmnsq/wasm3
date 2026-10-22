//
//  m3_info.h
//
//  Created by Steven Massey on 12/6/19.
//  Copyright © 2019 Steven Massey. All rights reserved.
//

#ਜੇ_ਅਪਰਿਭਾਸ਼ਿਤ m3_info_h
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ m3_info_h

#ਅਨ੍ਤਰ੍ਭਾਵ "m3_compile.h"

d_m3BeginExternC

ਖਾਲੀ Profile_ਪ੍ਰਾਪਤੀ (cstr_t i_operationName);

#ਜੇ_ਪਰਿਭਾਸ਼ਿਤ DEBUG

ਖਾਲੀ        dump_type_ਢੇਰ (IM3Compilation o);
ਖਾਲੀ        log_opcode (IM3Compilation o, m3opcode_t i_opcode);
ਅਚਲ ਅੱਖਰ* get_indention_string (IM3Compilation o);
ਖਾਲੀ        log_emit (IM3Compilation o, IM3Operation i_operation);

cstr_t      SPrintFuncTypeSignature (IM3FuncType i_funcType);

#ਨਹੀਂਤਾਂ // DEBUG

#  define   dump_type_ਢੇਰ(...)      {}
#  define   log_opcode(...)           {}
#  define   get_indention_string(...) ""
#  define   emit_ਢੇਰ_dump(...)      {}
#  define   log_emit(...)             {}

#ਜੇ_ਅੰਤ // DEBUG

d_m3_ਸਿਰਾ_Extern_C

#ਜੇ_ਅੰਤ // m3_info_h
