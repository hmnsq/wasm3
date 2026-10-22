//
//  m3_info.h
//
//  Created by Steven Massey on 12/6/19.
//  Copyright © 2019 Steven Massey. All rights reserved.
//

#ವ್ಯಾಖ್ಯಾನಿತವಾಗದಿದ್ದರೆ m3_info_h
#ವ್ಯಾಖ್ಯಾನಿಸು m3_info_h

#ಅನ್ತರ್ಭಾವ "m3_compile.h"

d_m3BeginExternC

ಶೂನ್ಯ Profile_ದೊರಕಿತು (cstr_t i_operationName);

#ವ್ಯಾಖ್ಯಾನಿತವಾದರೆ DEBUG

ಶೂನ್ಯ        dump_type_ರಾಶಿ (IM3Compilation o);
ಶೂನ್ಯ        log_opcode (IM3Compilation o, m3opcode_t i_opcode);
ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* get_indention_string (IM3Compilation o);
ಶೂನ್ಯ        log_emit (IM3Compilation o, IM3Operation i_operation);

cstr_t      SPrintFuncTypeSignature (IM3FuncType i_funcType);

#ಇಲ್ಲದಿದ್ದರೆ // DEBUG

#  define   dump_type_ರಾಶಿ(...)      {}
#  define   log_opcode(...)           {}
#  define   get_indention_string(...) ""
#  define   emit_ರಾಶಿ_dump(...)      {}
#  define   log_emit(...)             {}

#ಆದರೆ_ಅಂತ್ಯ // DEBUG

d_m3_ಕೊನೆ_Extern_C

#ಆದರೆ_ಅಂತ್ಯ // m3_info_h
