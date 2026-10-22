//
//  Wasm3, high performance WebAssembly interpreter
//
//  Extensions
//
//  Copyright © 2019-2021 Steven Massey, Volodymyr Shymanskyy.
//  All rights reserved.
//

#నిర్వచితం_కాకపోతే wasm3_ext_h
#నిర్వచించు wasm3_ext_h

#అన్తర్భావ "wasm3.h"
#అన్తర్భావ <stdbool.h>

#ఒకవేళ defined(__cplusplus)
బాహ్య "C" {
#అయితే_ముగింపు

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
M3_ఫలితం    m3_InjectFunction (IM3Module            i_module,
                               int32_t*             io_function_సూచీ,
                               స్థిరరాశి అక్షరం* స్థిరరాశి    i_signature,
                               స్థిరరాశి uint8_t* స్థిరరాశి i_wasm_బైట్లు,            // i_wasmBytes is copied
                               bool                 i_doCompilation);


IM3Function m3_Get_Function_By_సూచీ (IM3Module i_module,
                                   uint32_t  i_సూచీ);

#ఒకవేళ defined(__cplusplus)
}
#అయితే_ముగింపు

#అయితే_ముగింపు // wasm3_h
