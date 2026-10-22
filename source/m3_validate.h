//
//  m3_validate.h
//
//  Pre-pass WebAssembly bytecode validator using the spec's type-checking algorithm.
//  Runs before compilation to catch type errors early.
//

#నిర్వచితం_కాకపోతే m3_validate_h
#నిర్వచించు m3_validate_h

#అన్తర్భావ "m3_core.h"
#అన్తర్భావ "m3_compile.h"
#అన్తర్భావ "m3_env.h"

d_m3BeginExternC

// Validate a function's bytecode before compilation.
// Performs full type-checking per the WebAssembly spec algorithm:
//   operand type stack + control stack with polymorphic handling.
// Returns m3Err_none on success or a validation error.
M3_ఫలితం ValidateFunction (IM3Function i_function);

d_m3_చివర_Extern_C

#అయితే_ముగింపు // m3_validate_h
