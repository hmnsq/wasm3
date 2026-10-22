//
//  Wasm3, high performance WebAssembly interpreter
//
//  Extensions
//
//  Copyright © 2019-2021 Steven Massey, Volodymyr Shymanskyy.
//  All rights reserved.
//

#ifndef wasm3_ext_h
#define wasm3_ext_h

#অন্তর্ভাব "wasm3.h"
#অন্তর্ভাব <stdbool.h>

#যদি defined(__cplusplus)
বাহ্যিক "C" {
#endif

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
M3_ফলাফল    m3_InjectFunction (IM3Module            i_module,
                               int32_t*             io_function_সূচক,
                               ধ্রুবক অক্ষর* ধ্রুবক    i_signature,
                               ধ্রুবক uint8_t* ধ্রুবক i_wasmBytes,            // i_wasmBytes is copied
                               bool                 i_doCompilation);


IM3Function m3_Get_Function_By_সূচক (IM3Module i_module,
                                   uint32_t  i_সূচক);

#যদি defined(__cplusplus)
}
#endif

#endif // wasm3_h
