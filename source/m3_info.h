//
//  m3_info.h
//
//  Created by Steven Massey on 12/6/19.
//  Copyright © 2019 Steven Massey. All rights reserved.
//

#నిర్వచితం_కాకపోతే m3_info_h
#నిర్వచించు m3_info_h

#అన్తర్భావ "m3_compile.h"

d_m3BeginExternC

శూన్య Profile_తగిలింది (cstr_t i_operationName);

#నిర్వచితమైతే DEBUG

శూన్య        dump_type_దొంతర (IM3Compilation o);
శూన్య        log_opcode (IM3Compilation o, m3opcode_t i_opcode);
స్థిరరాశి అక్షరం* get_indention_string (IM3Compilation o);
శూన్య        log_emit (IM3Compilation o, IM3Operation i_operation);

cstr_t      SPrintFuncTypeSignature (IM3FuncType i_funcType);

#లేకపోతే // DEBUG

#  define   dump_type_దొంతర(...)      {}
#  define   log_opcode(...)           {}
#  define   get_indention_string(...) ""
#  define   emit_దొంతర_dump(...)      {}
#  define   log_emit(...)             {}

#అయితే_ముగింపు // DEBUG

d_m3_చివర_Extern_C

#అయితే_ముగింపు // m3_info_h
