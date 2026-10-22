//
//  Wasm3, high performance WebAssembly interpreter
//
//  Extensions
//
//  Copyright © 2019-2021 Steven Massey, Volodymyr Shymanskyy.
//  All rights reserved.
//

#अगर_अघोषित wasm3_ext_h
#घोषणा wasm3_ext_h

#समावेश "wasm3.h"
#समावेश <stdbool.h>

#अगर defined(__cplusplus)
बाह्य "C" {
#पूर्ण_अगर

//-------------------------------------------------------------------------------------------------------------------------------
//  API extensions
//-------------------------------------------------------------------------------------------------------------------------------
/*
    These extensions allow for unconventional uses of Wasm3 -- mainly dynamic modification of modules to inject new Wasm
    functions during runtime.
*/
//-------------------------------------------------------------------------------------------------------------------------------

// Creates an empty module.
IM3Module   m3_NewModule (IM3Environment i_environment);


// To append a new function, set io_functionIndex to negative. On return, the new function index will be set.
// To overwrite an existing function, set io_functionIndex to the desired element. i_signature must match the existing
// function signature.
// ** InjectFunction invalidates any existing IM3Function pointers
M3_परिणाम    m3_InjectFunction (IM3Module            i_module,
                               int32_t*             io_function_सूचकांक,
                               स्थिर अक्षर* स्थिर    i_signature,
                               स्थिर uint8_t* स्थिर i_wasmBytes,            // i_wasmBytes is copied
                               bool                 i_doCompilation);


IM3Function m3_Get_Function_By_सूचकांक (IM3Module i_module,
                                   uint32_t  i_सूचकांक);

#अगर defined(__cplusplus)
}
#पूर्ण_अगर

#पूर्ण_अगर // wasm3_h
