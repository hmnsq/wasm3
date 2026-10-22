//
//  m3_validate.h
//
//  Pre-pass WebAssembly bytecode validator using the spec's type-checking algorithm.
//  Runs before compilation to catch type errors early.
//

#جيڪڏهن_وصف_نه_ٿيل m3_validate_h
#وصف_ڪريو m3_validate_h

#ضمن "m3_core.h"
#ضمن "m3_compile.h"
#ضمن "m3_env.h"

d_m3BeginExternC

// Validate a function's bytecode before compilation.
// Performs full type-checking per the WebAssembly spec algorithm:
//   operand type stack + control stack with polymorphic handling.
// Returns m3Err_none on success or a validation error.
M3_نتيجو ValidateFunction (IM3Function i_function);

d_m3_پڇاڙي_Extern_C

#جيڪڏهن_پورو // m3_validate_h
