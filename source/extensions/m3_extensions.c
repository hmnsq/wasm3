//
//  m3_extensions.c
//
//  Created by Steven Massey on 3/30/21.
//  Copyright © 2021 Steven Massey. All rights reserved.
//

#அந்தர்பாவ "wasm3_ext.h"

#அந்தர்பாவ "m3_env.h"
#அந்தர்பாவ "m3_bind.h"
#அந்தர்பாவ "m3_exception.h"


IM3Module m3_NewModule (IM3Environment i_environment)
{
    IM3Module module = m3_AllocStruct(M3Module);

    எனில் (module) {
        module->name          = ".unnamed";
        module->startFunction = -1;
        module->environment   = i_environment;

        module->wasmStart = வெற்று;
        module->wasmEnd   = வெற்று;
    }

    திரும்பு module;
}


M3Result m3_InjectFunction (IM3Module            i_module,
                            int32_t*             io_functionIndex,
                            மாறிலி எழுத்து* மாறிலி    i_signature,
                            மாறிலி uint8_t* மாறிலி i_wasmBytes,
                            bool                 i_doCompilation)
{
    M3Result result = m3Err_none;                                       d_m3Assert (io_functionIndex);

    IM3Function function = வெற்று;
    IM3FuncType ftype    = வெற்று;

    // Declared before the first throw below, which jumps past this point to
    // _catch and so must not skip an initialization.
    i32     index;
    bytes_t bytes = i_wasmBytes;
    bytes_t end   = i_wasmBytes + 5;
    size_t  இலக்கம்_Bytes;
    u32     size;

_   (SignatureToFuncType(&ftype, i_signature));

    index = *io_functionIndex;

_   (ReadLEB_u32(&size, &bytes, end));
    end = bytes + size;

    எனில் (index >= 0) {
        // index is known non-negative here, so the counts compare as unsigned
        _throwif("function index out of bounds", (u32)index >= i_module->இலக்கம்_Functions);

        function = &i_module->functions[index];

        எனில் (not AreFuncTypesEqual(ftype, function->funcType)) {
            _throw("function type mismatch");
        }
    } இல்லையெனில் {
        // add slot to function type table in the module
        u32 funcTypeIndex   = i_module->இலக்கம்_Func_Types++;
        i_module->funcTypes = m3_ReallocArray(IM3FuncType, i_module->funcTypes, i_module->இலக்கம்_Func_Types, funcTypeIndex);
        _throwifnull(i_module->funcTypes);

        // add functype object to the environment
        Environment_AddFuncType(i_module->environment, &ftype);
        i_module->funcTypes[funcTypeIndex] = ftype;

        ftype = வெற்று; // prevent freeing below

        index = (i32)i_module->இலக்கம்_Functions;
_       (Module_AddFunction(i_module, funcTypeIndex, வெற்று));
        function = Module_GetFunction(i_module, index);

        *io_functionIndex = index;
    }

    function->compiled = வெற்று;

    எனில் (function->ownsWasmCode) {
        m3_Free(function->wasm);
    }

    இலக்கம்_Bytes       = end - i_wasmBytes;
    function->wasm = m3_CopyMem(i_wasmBytes, இலக்கம்_Bytes);
    _throwifnull(function->wasm);

    function->wasmEnd      = function->wasm + இலக்கம்_Bytes;
    function->ownsWasmCode = true;

    function->module = i_module;

    எனில் (i_doCompilation and not i_module->runtime) {
        _throw("module must be loaded into runtime to compile function");
    }

_   (CompileFunction(function));

    _catch:
    m3_Free(ftype);

    திரும்பு result;
}


IM3Function m3_GetFunctionByIndex (IM3Module i_module, uint32_t i_index)
{
    திரும்பு Module_GetFunction(i_module, i_index);
}
