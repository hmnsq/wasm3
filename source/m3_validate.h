//
//  m3_validate.h
//
//  Pre-pass WebAssembly bytecode validator using the spec's type-checking algorithm.
//  Runs before compilation to catch type errors early.
//

#اگر_غیر_تعریف_شدہ m3_validate_h
#تعریف_کرو m3_validate_h

#شامل "m3_core.h"
#شامل "m3_compile.h"
#شامل "m3_env.h"

d_m3BeginExternC

// Validate a function's bytecode before compilation.
// Performs full type-checking per the WebAssembly spec algorithm:
//   operand type stack + control stack with polymorphic handling.
// Returns m3Err_none on success or a validation error.
M3_نتیجہ ValidateFunction (IM3Function i_function);

d_m3_انتہا_Extern_C

#اگر_ختم // m3_validate_h
