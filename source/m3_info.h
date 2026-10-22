//
//  m3_info.h
//
//  Created by Steven Massey on 12/6/19.
//  Copyright © 2019 Steven Massey. All rights reserved.
//

#अगर_अघोषित m3_info_h
#घोषणा m3_info_h

#समावेश "m3_compile.h"

d_m3BeginExternC

व्योम ProfileHit (cstr_t i_operationName);

#अगर_घोषित DEBUG

व्योम        dump_type_ढेर (IM3Compilation o);
व्योम        log_opcode (IM3Compilation o, m3opcode_t i_opcode);
स्थिर अक्षर* get_indention_string (IM3Compilation o);
व्योम        log_emit (IM3Compilation o, IM3Operation i_operation);

cstr_t      SPrintFuncTypeSignature (IM3FuncType i_funcType);

#अन्यथा // DEBUG

#  define   dump_type_ढेर(...)      {}
#  define   log_opcode(...)           {}
#  define   get_indention_string(...) ""
#  define   emit_ढेर_dump(...)      {}
#  define   log_emit(...)             {}

#पूर्ण_अगर // DEBUG

d_m3EndExternC

#पूर्ण_अगर // m3_info_h
