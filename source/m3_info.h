//
//  m3_info.h
//
//  Created by Steven Massey on 12/6/19.
//  Copyright © 2019 Steven Massey. All rights reserved.
//

#ifndef m3_info_h
#define m3_info_h

#அந்தர்பாவ "m3_compile.h"

d_m3BeginExternC

வெறுமை ProfileHit (cstr_t i_operationName);

#ifdef DEBUG

வெறுமை        dump_type_அடுக்கு (IM3Compilation o);
வெறுமை        log_opcode (IM3Compilation o, m3opcode_t i_opcode);
மாறிலி எழுத்து* get_indention_string (IM3Compilation o);
வெறுமை        log_emit (IM3Compilation o, IM3Operation i_operation);

cstr_t      SPrintFuncTypeSignature (IM3FuncType i_funcType);

#இல்லையெனில் // DEBUG

#  define   dump_type_அடுக்கு(...)      {}
#  define   log_opcode(...)           {}
#  define   get_indention_string(...) ""
#  define   emit_அடுக்கு_dump(...)      {}
#  define   log_emit(...)             {}

#endif // DEBUG

d_m3EndExternC

#endif // m3_info_h
