//
//  m3_info.h
//
//  Created by Steven Massey on 12/6/19.
//  Copyright © 2019 Steven Massey. All rights reserved.
//

#ifndef m3_info_h
#define m3_info_h

#અન્તર્ભાવ "m3_compile.h"

d_m3BeginExternC

શૂન્ય ProfileHit (cstr_t i_operationName);

#ifdef DEBUG

શૂન્ય        dump_type_ઢગલો (IM3Compilation o);
શૂન્ય        log_opcode (IM3Compilation o, m3opcode_t i_opcode);
અચલ અક્ષર* get_indention_string (IM3Compilation o);
શૂન્ય        log_emit (IM3Compilation o, IM3Operation i_operation);

cstr_t      SPrintFuncTypeSignature (IM3FuncType i_funcType);

#નહીંતર // DEBUG

#  define   dump_type_ઢગલો(...)      {}
#  define   log_opcode(...)           {}
#  define   get_indention_string(...) ""
#  define   emit_ઢગલો_dump(...)      {}
#  define   log_emit(...)             {}

#endif // DEBUG

d_m3EndExternC

#endif // m3_info_h
