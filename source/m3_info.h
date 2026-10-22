//
//  m3_info.h
//
//  Created by Steven Massey on 12/6/19.
//  Copyright © 2019 Steven Massey. All rights reserved.
//

#جيڪڏهن_وصف_نه_ٿيل m3_info_h
#وصف_ڪريو m3_info_h

#ضمن "m3_compile.h"

d_m3BeginExternC

فراغ Profile_ڪاميابي (cstr_t i_operationName);

#جيڪڏهن_وصف_ٿيل DEBUG

فراغ        dump_type_ڍير (IM3Compilation o);
فراغ        log_opcode (IM3Compilation o, m3opcode_t i_opcode);
ثابت حرف* get_indention_string (IM3Compilation o);
فراغ        log_emit (IM3Compilation o, IM3Operation i_operation);

cstr_t      SPrintFuncTypeSignature (IM3FuncType i_funcType);

#وإلا // DEBUG

#  define   dump_type_ڍير(...)      {}
#  define   log_opcode(...)           {}
#  define   get_indention_string(...) ""
#  define   emit_ڍير_dump(...)      {}
#  define   log_emit(...)             {}

#جيڪڏهن_پورو // DEBUG

d_m3_پڇاڙي_Extern_C

#جيڪڏهن_پورو // m3_info_h
