//
//  m3_extensions.c
//
//  Created by Steven Massey on 3/30/21.
//  Copyright © 2021 Steven Massey. All rights reserved.
//

#অন্তর্ভাব "wasm3_ext.h"

#অন্তর্ভাব "m3_env.h"
#অন্তর্ভাব "m3_bind.h"
#অন্তর্ভাব "m3_exception.h"


IM3Module m3_NewModule (IM3Environment i_environment)
{
    IM3Module module = m3_AllocStruct(M3Module);

    যদি (module) {
        module->name          = ".unnamed";
        module->startFunction = -1;
        module->environment   = i_environment;

        module->wasmStart = ফাঁকা;
        module->wasmEnd   = ফাঁকা;
    }

    ফেরত module;
}


M3_ফলাফল m3_InjectFunction (IM3Module            i_module,
                            int32_t*             io_function_সূচক,
                            ধ্রুবক অক্ষর* ধ্রুবক    i_signature,
                            ধ্রুবক uint8_t* ধ্রুবক i_wasmBytes,
                            bool                 i_doCompilation)
{
    M3_ফলাফল ফলাফল = m3Err_none;                                       d_m3Assert (io_function_সূচক);

    IM3Function function = ফাঁকা;
    IM3FuncType ftype    = ফাঁকা;

    // Declared before the first throw below, which jumps past this point to
    // _catch and so must not skip an initialization.
    i32     index;
    bytes_t bytes = i_wasmBytes;
    bytes_t end   = i_wasmBytes + 5;
    size_t  অঙ্ক_Bytes;
    u32     আকার_2;

_   (SignatureToFuncType(&ftype, i_signature));

    index = *io_function_সূচক;

_   (ReadLEB_u32(&আকার_2, &bytes, end));
    end = bytes + আকার_2;

    যদি (index >= 0) {
        // index is known non-negative here, so the counts compare as unsigned
        _throwif("function index out of bounds", (u32)index >= i_module->অঙ্ক_Functions);

        function = &i_module->functions[index];

        যদি (not AreFuncTypesEqual(ftype, function->funcType)) {
            _throw("function type mismatch");
        }
    } অন্যথা {
        // add slot to function type table in the module
        u32 func_Type_সূচক   = i_module->অঙ্ক_Func_Types++;
        i_module->funcTypes = m3_ReallocArray(IM3FuncType, i_module->funcTypes, i_module->অঙ্ক_Func_Types, func_Type_সূচক);
        _throwifnull(i_module->funcTypes);

        // add functype object to the environment
        Environment_AddFuncType(i_module->environment, &ftype);
        i_module->funcTypes[func_Type_সূচক] = ftype;

        ftype = ফাঁকা; // prevent freeing below

        index = (i32)i_module->অঙ্ক_Functions;
_       (Module_AddFunction(i_module, func_Type_সূচক, ফাঁকা));
        function = Module_GetFunction(i_module, index);

        *io_function_সূচক = index;
    }

    function->compiled = ফাঁকা;

    যদি (function->ownsWasmCode) {
        m3_Free(function->wasm);
    }

    অঙ্ক_Bytes       = end - i_wasmBytes;
    function->wasm = m3_CopyMem(i_wasmBytes, অঙ্ক_Bytes);
    _throwifnull(function->wasm);

    function->wasmEnd      = function->wasm + অঙ্ক_Bytes;
    function->ownsWasmCode = true;

    function->module = i_module;

    যদি (i_doCompilation and not i_module->runtime) {
        _throw("module must be loaded into runtime to compile function");
    }

_   (CompileFunction(function));

    _catch:
    m3_Free(ftype);

    ফেরত ফলাফল;
}


IM3Function m3_Get_Function_By_সূচক (IM3Module i_module, uint32_t i_সূচক)
{
    ফেরত Module_GetFunction(i_module, i_সূচক);
}
