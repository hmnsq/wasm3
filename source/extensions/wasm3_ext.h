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

#ಅನ್ತರ್ಭಾವ "wasm3.h"
#ಅನ್ತರ್ಭಾವ <stdbool.h>

#ಯದಿ defined(__cplusplus)
ಬಾಹ್ಯ "C" {
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
M3Result    m3_InjectFunction (IM3Module            i_module,
                               int32_t*             io_functionIndex,
                               ಧ್ರುವ ವರ್ಣ* ಧ್ರುವ    i_signature,
                               ಧ್ರುವ uint8_t* ಧ್ರುವ i_wasmBytes,            // i_wasmBytes is copied
                               bool                 i_doCompilation);


IM3Function m3_GetFunctionByIndex (IM3Module i_module,
                                   uint32_t  i_index);

#ಯದಿ defined(__cplusplus)
}
#endif

#endif // wasm3_h
