//
//  m3_extensions.c
//
//  Created by Steven Massey on 3/30/21.
//  Copyright © 2021 Steven Massey. All rights reserved.
//

#ضمن "wasm3_ext.h"

#ضمن "m3_env.h"
#ضمن "m3_bind.h"
#ضمن "m3_exception.h"


IM3Module m3_NewModule (IM3Environment i_environment)
{
    IM3Module module = m3_AllocStruct(M3Module);

    إذا (module) {
        module->name          = ".unnamed";
        module->startFunction = -1;
        module->environment   = i_environment;

        module->wasmStart = عدم;
        module->wasmEnd   = عدم;
    }

    أرجع module;
}


M3Result m3_InjectFunction (IM3Module            i_module,
                            int32_t*             io_functionIndex,
                            ثابت حرف* ثابت    i_signature,
                            ثابت uint8_t* ثابت i_wasmBytes,
                            bool                 i_doCompilation)
{
    M3Result result = m3Err_none;                                       d_m3Assert (io_functionIndex);

    IM3Function function = عدم;
    IM3FuncType ftype    = عدم;

    // Declared before the first throw below, which jumps past this point to
    // _catch and so must not skip an initialization.
    i32     index;
    bytes_t bytes = i_wasmBytes;
    bytes_t end   = i_wasmBytes + 5;
    size_t  لمب_Bytes;
    u32     size;

_   (SignatureToFuncType(&ftype, i_signature));

    index = *io_functionIndex;

_   (ReadLEB_u32(&size, &bytes, end));
    end = bytes + size;

    إذا (index >= 0) {
        // index is known non-negative here, so the counts compare as unsigned
        _throwif("function index out of bounds", (u32)index >= i_module->لمب_Functions);

        function = &i_module->functions[index];

        إذا (not AreFuncTypesEqual(ftype, function->funcType)) {
            _throw("function type mismatch");
        }
    } وإلا {
        // add slot to function type table in the module
        u32 funcTypeIndex   = i_module->لمب_Func_Types++;
        i_module->funcTypes = m3_ReallocArray(IM3FuncType, i_module->funcTypes, i_module->لمب_Func_Types, funcTypeIndex);
        _throwifnull(i_module->funcTypes);

        // add functype object to the environment
        Environment_AddFuncType(i_module->environment, &ftype);
        i_module->funcTypes[funcTypeIndex] = ftype;

        ftype = عدم; // prevent freeing below

        index = (i32)i_module->لمب_Functions;
_       (Module_AddFunction(i_module, funcTypeIndex, عدم));
        function = Module_GetFunction(i_module, index);

        *io_functionIndex = index;
    }

    function->compiled = عدم;

    إذا (function->ownsWasmCode) {
        m3_Free(function->wasm);
    }

    لمب_Bytes       = end - i_wasmBytes;
    function->wasm = m3_CopyMem(i_wasmBytes, لمب_Bytes);
    _throwifnull(function->wasm);

    function->wasmEnd      = function->wasm + لمب_Bytes;
    function->ownsWasmCode = true;

    function->module = i_module;

    إذا (i_doCompilation and not i_module->runtime) {
        _throw("module must be loaded into runtime to compile function");
    }

_   (CompileFunction(function));

    _catch:
    m3_Free(ftype);

    أرجع result;
}


IM3Function m3_GetFunctionByIndex (IM3Module i_module, uint32_t i_index)
{
    أرجع Module_GetFunction(i_module, i_index);
}
