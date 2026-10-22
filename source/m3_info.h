//
//  m3_info.h
//
//  Created by Steven Massey on 12/6/19.
//  Copyright © 2019 Steven Massey. All rights reserved.
//

#यदि_अपरिभाषितम् m3_info_h
#परिभाषय m3_info_h

#अन्तर्भाव "m3_compile.h"

d_m3BeginExternC

शून्य Profile_लब्धम् (cstr_t i_operationName);

#यदि_परिभाषितम् DEBUG

शून्य        dump_type_राशिः (IM3Compilation o);
शून्य        log_opcode (IM3Compilation o, m3opcode_t i_opcode);
ध्रुव वर्ण* get_indention_string (IM3Compilation o);
शून्य        log_emit (IM3Compilation o, IM3Operation i_operation);

cstr_t      SPrintFuncTypeSignature (IM3FuncType i_funcType);

#अन्यथा // DEBUG

#  define   dump_type_राशिः(...)      {}
#  define   log_opcode(...)           {}
#  define   get_indention_string(...) ""
#  define   emit_राशिः_dump(...)      {}
#  define   log_emit(...)             {}

#यदि_अन्तः // DEBUG

d_m3_अन्तः_Extern_C

#यदि_अन्तः // m3_info_h
