//
//  Wasm3, high performance WebAssembly interpreter
//
//  Extensions
//
//  Copyright © 2019-2021 Steven Massey, Volodymyr Shymanskyy.
//  All rights reserved.
//

#جيڪڏهن_وصف_نه_ٿيل wasm3_ext_h
#وصف_ڪريو wasm3_ext_h

#ضمن "wasm3.h"
#ضمن <stdbool.h>

#إذا defined(__cplusplus)
خارجي "C" {
#جيڪڏهن_پورو

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
M3_نتيجو    m3_InjectFunction (IM3Module            i_module,
                               int32_t*             io_function_اشاريو,
                               ثابت حرف* ثابت    i_signature,
                               ثابت uint8_t* ثابت i_wasm_بائيٽون,            // i_wasmBytes is copied
                               bool                 i_doCompilation);


IM3Function m3_Get_Function_By_اشاريو (IM3Module i_module,
                                   uint32_t  i_اشاريو);

#إذا defined(__cplusplus)
}
#جيڪڏهن_پورو

#جيڪڏهن_پورو // wasm3_h
