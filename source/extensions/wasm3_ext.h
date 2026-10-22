//
//  Wasm3, high performance WebAssembly interpreter
//
//  Extensions
//
//  Copyright © 2019-2021 Steven Massey, Volodymyr Shymanskyy.
//  All rights reserved.
//

#ವ್ಯಾಖ್ಯಾನಿತವಾಗದಿದ್ದರೆ wasm3_ext_h
#ವ್ಯಾಖ್ಯಾನಿಸು wasm3_ext_h

#ಅನ್ತರ್ಭಾವ "wasm3.h"
#ಅನ್ತರ್ಭಾವ <stdbool.h>

#ಆದರೆ defined(__cplusplus)
ಬಾಹ್ಯ "C" {
#ಆದರೆ_ಅಂತ್ಯ

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
M3_ಫಲಿತಾಂಶ    m3_InjectFunction (IM3Module            i_module,
                               int32_t*             io_function_ಸೂಚ್ಯಂಕ,
                               ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* ಸ್ಥಿರಾಂಕ    i_signature,
                               ಸ್ಥಿರಾಂಕ uint8_t* ಸ್ಥಿರಾಂಕ i_wasm_ಬೈಟುಗಳು,            // i_wasmBytes is copied
                               bool                 i_doCompilation);


IM3Function m3_Get_Function_By_ಸೂಚ್ಯಂಕ (IM3Module i_module,
                                   uint32_t  i_ಸೂಚ್ಯಂಕ);

#ಆದರೆ defined(__cplusplus)
}
#ಆದರೆ_ಅಂತ್ಯ

#ಆದರೆ_ಅಂತ್ಯ // wasm3_h
