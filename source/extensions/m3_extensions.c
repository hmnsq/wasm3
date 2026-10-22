//
//  m3_extensions.c
//
//  Created by Steven Massey on 3/30/21.
//  Copyright © 2021 Steven Massey. All rights reserved.
//

#समावेश "wasm3_ext.h"

#समावेश "m3_env.h"
#समावेश "m3_bind.h"
#समावेश "m3_exception.h"


IM3Module m3_NewModule (IM3Environment i_environment)
{
    IM3Module module = m3_AllocStruct(M3Module);

    अगर (module) {
        module->name          = ".unnamed";
        module->startFunction = -1;
        module->environment   = i_environment;

        module->wasmStart = शून्य;
        module->wasmEnd   = शून्य;
    }

    वापस module;
}


M3_परिणाम m3_InjectFunction (IM3Module            i_module,
                            int32_t*             io_function_सूचकांक,
                            स्थिर अक्षर* स्थिर    i_signature,
                            स्थिर uint8_t* स्थिर i_wasmBytes,
                            bool                 i_doCompilation)
{
    M3_परिणाम परिणाम = m3Err_none;                                       d_m3Assert (io_function_सूचकांक);

    IM3Function function = शून्य;
    IM3FuncType ftype    = शून्य;

    // Declared before the first throw below, which jumps past this point to
    // _catch and so must not skip an initialization.
    i32     index;
    bytes_t bytes = i_wasmBytes;
    bytes_t end   = i_wasmBytes + 5;
    माप_प्रकार  अंक_Bytes;
    u32     आकार;

_   (SignatureToFuncType(&ftype, i_signature));

    index = *io_function_सूचकांक;

_   (ReadLEB_u32(&आकार, &bytes, end));
    end = bytes + आकार;

    अगर (index >= 0) {
        // index is known non-negative here, so the counts compare as unsigned
        _throwif("function index out of bounds", (u32)index >= i_module->अंक_Functions);

        function = &i_module->functions[index];

        अगर (not AreFuncTypesEqual(ftype, function->funcType)) {
            _throw("function type mismatch");
        }
    } अन्यथा {
        // add slot to function type table in the module
        u32 func_Type_सूचकांक   = i_module->अंक_Func_Types++;
        i_module->funcTypes = m3_ReallocArray(IM3FuncType, i_module->funcTypes, i_module->अंक_Func_Types, func_Type_सूचकांक);
        _throwifnull(i_module->funcTypes);

        // add functype object to the environment
        Environment_AddFuncType(i_module->environment, &ftype);
        i_module->funcTypes[func_Type_सूचकांक] = ftype;

        ftype = शून्य; // prevent freeing below

        index = (i32)i_module->अंक_Functions;
_       (Module_AddFunction(i_module, func_Type_सूचकांक, शून्य));
        function = Module_GetFunction(i_module, index);

        *io_function_सूचकांक = index;
    }

    function->compiled = शून्य;

    अगर (function->ownsWasmCode) {
        m3_Free(function->wasm);
    }

    अंक_Bytes       = end - i_wasmBytes;
    function->wasm = m3_CopyMem(i_wasmBytes, अंक_Bytes);
    _throwifnull(function->wasm);

    function->wasmEnd      = function->wasm + अंक_Bytes;
    function->ownsWasmCode = true;

    function->module = i_module;

    अगर (i_doCompilation and not i_module->runtime) {
        _throw("module must be loaded into runtime to compile function");
    }

_   (CompileFunction(function));

    _catch:
    m3_Free(ftype);

    वापस परिणाम;
}


IM3Function m3_Get_Function_By_सूचकांक (IM3Module i_module, uint32_t i_सूचकांक)
{
    वापस Module_GetFunction(i_module, i_सूचकांक);
}
