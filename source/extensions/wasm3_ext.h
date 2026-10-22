//
//  Wasm3, high performance WebAssembly interpreter
//
//  Extensions
//
//  Copyright © 2019-2021 Steven Massey, Volodymyr Shymanskyy.
//  All rights reserved.
//

#اگر_تعریف_نشده wasm3_ext_h
#تعریف_کن wasm3_ext_h

#ضمن "wasm3.h"
#ضمن <stdbool.h>

#إذا defined(__cplusplus)
خارجي "C" {
#پایان_اگر

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
M3_نتیجه    m3_InjectFunction (IM3Module            i_module,
                               int32_t*             io_function_شاخص,
                               ثابت حرف* ثابت    i_signature,
                               ثابت uint8_t* ثابت i_wasm_بایتها,            // i_wasmBytes is copied
                               bool                 i_doCompilation);


IM3Function m3_Get_Function_By_شاخص (IM3Module i_module,
                                   uint32_t  i_شاخص);

#إذا defined(__cplusplus)
}
#پایان_اگر

#پایان_اگر // wasm3_h
