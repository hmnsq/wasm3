//
//  m3_validate.h
//
//  Pre-pass WebAssembly bytecode validator using the spec's type-checking algorithm.
//  Runs before compilation to catch type errors early.
//

#अगर_अघोषित m3_validate_h
#घोषणा m3_validate_h

#समावेश "m3_core.h"
#समावेश "m3_compile.h"
#समावेश "m3_env.h"

d_m3BeginExternC

// Validate a function's bytecode before compilation.
// Performs full type-checking per the WebAssembly spec algorithm:
//   operand type stack + control stack with polymorphic handling.
// Returns m3Err_none on success or a validation error.
M3_परिणाम ValidateFunction (IM3Function i_function);

d_m3EndExternC

#पूर्ण_अगर // m3_validate_h
