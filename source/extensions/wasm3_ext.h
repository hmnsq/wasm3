//
//  Wasm3, high performance WebAssembly interpreter
//
//  Extensions
//
//  Copyright © 2019-2021 Steven Massey, Volodymyr Shymanskyy.
//  All rights reserved.
//

#ਜੇ_ਅਪਰਿਭਾਸ਼ਿਤ wasm3_ext_h
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ wasm3_ext_h

#ਅਨ੍ਤਰ੍ਭਾਵ "wasm3.h"
#ਅਨ੍ਤਰ੍ਭਾਵ <stdbool.h>

#ਜੇ defined(__cplusplus)
ਬਾਹਰੀ "C" {
#ਜੇ_ਅੰਤ

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
M3_ਨਤੀਜਾ    m3_InjectFunction (IM3Module            i_module,
                               int32_t*             io_function_ਸੂਚਕਾਂਕ,
                               ਅਚਲ ਅੱਖਰ* ਅਚਲ    i_signature,
                               ਅਚਲ uint8_t* ਅਚਲ i_wasm_ਬਾਈਟਾਂ,            // i_wasmBytes is copied
                               bool                 i_doCompilation);


IM3Function m3_Get_Function_By_ਸੂਚਕਾਂਕ (IM3Module i_module,
                                   uint32_t  i_ਸੂਚਕਾਂਕ);

#ਜੇ defined(__cplusplus)
}
#ਜੇ_ਅੰਤ

#ਜੇ_ਅੰਤ // wasm3_h
