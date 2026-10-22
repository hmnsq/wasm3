//
//  Wasm3, high performance WebAssembly interpreter
//
//  Extensions
//
//  Copyright © 2019-2021 Steven Massey, Volodymyr Shymanskyy.
//  All rights reserved.
//

#اگر_غیر_تعریف_شدہ wasm3_ext_h
#تعریف_کرو wasm3_ext_h

#شامل "wasm3.h"
#شامل <stdbool.h>

#اگر defined(__cplusplus)
خارجی "C" {
#اگر_ختم

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
M3_نتیجہ    m3_InjectFunction (IM3Module            i_module,
                               int32_t*             io_function_اشاریہ,
                               ثابت حرف* ثابت    i_signature,
                               ثابت uint8_t* ثابت i_wasm_بائٹیں,            // i_wasmBytes is copied
                               bool                 i_doCompilation);


IM3Function m3_Get_Function_By_اشاریہ (IM3Module i_module,
                                   uint32_t  i_اشاریہ);

#اگر defined(__cplusplus)
}
#اگر_ختم

#اگر_ختم // wasm3_h
