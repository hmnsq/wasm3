//
//  m3_validate.h
//
//  Pre-pass WebAssembly bytecode validator using the spec's type-checking algorithm.
//  Runs before compilation to catch type errors early.
//

#यदि_अपरिभाषितम् m3_validate_h
#परिभाषय m3_validate_h

#अन्तर्भाव "m3_core.h"
#अन्तर्भाव "m3_compile.h"
#अन्तर्भाव "m3_env.h"

d_m3BeginExternC

// Validate a function's bytecode before compilation.
// Performs full type-checking per the WebAssembly spec algorithm:
//   operand type stack + control stack with polymorphic handling.
// Returns m3Err_none on success or a validation error.
M3_परिणाम ValidateFunction (IM3Function i_function);

d_m3_अन्तः_Extern_C

#यदि_अन्तः // m3_validate_h
