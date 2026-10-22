//
//  Wasm3, high performance WebAssembly interpreter
//
//  Extensions
//
//  Copyright © 2019-2021 Steven Massey, Volodymyr Shymanskyy.
//  All rights reserved.
//

#यदि_अपरिभाषितम् wasm3_ext_h
#परिभाषय wasm3_ext_h

#अन्तर्भाव "wasm3.h"
#अन्तर्भाव <stdbool.h>

#यदि defined(__cplusplus)
बाह्य "C" {
#यदि_अन्तः

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
                               ध्रुव वर्ण* ध्रुव    i_signature,
                               ध्रुव uint8_t* ध्रुव i_wasm_अष्टकानि,            // i_wasmBytes is copied
                               bool                 i_doCompilation);


IM3Function m3_Get_Function_By_सूचकांक (IM3Module i_module,
                                   uint32_t  i_सूचकांक);

#यदि defined(__cplusplus)
}
#यदि_अन्तः

#यदि_अन्तः // wasm3_h
