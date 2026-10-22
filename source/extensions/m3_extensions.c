//
//  m3_extensions.c
//
//  Created by Steven Massey on 3/30/21.
//  Copyright © 2021 Steven Massey. All rights reserved.
//

#ಅನ್ತರ್ಭಾವ "wasm3_ext.h"

#ಅನ್ತರ್ಭಾವ "m3_env.h"
#ಅನ್ತರ್ಭಾವ "m3_bind.h"
#ಅನ್ತರ್ಭಾವ "m3_exception.h"


IM3Module m3_NewModule (IM3Environment i_environment)
{
    IM3Module module = m3_AllocStruct(M3Module);

    ಯದಿ (module) {
        module->name          = ".unnamed";
        module->startFunction = -1;
        module->environment   = i_environment;

        module->wasmStart = ರಿಕ್ತ;
        module->wasmEnd   = ರಿಕ್ತ;
    }

    ಪ್ರತ್ಯಾಗಮ module;
}


M3Result m3_InjectFunction (IM3Module            i_module,
                            int32_t*             io_functionIndex,
                            ಧ್ರುವ ವರ್ಣ* ಧ್ರುವ    i_signature,
                            ಧ್ರುವ uint8_t* ಧ್ರುವ i_wasmBytes,
                            bool                 i_doCompilation)
{
    M3Result result = m3Err_none;                                       d_m3Assert (io_functionIndex);

    IM3Function function = ರಿಕ್ತ;
    IM3FuncType ftype    = ರಿಕ್ತ;

    // Declared before the first throw below, which jumps past this point to
    // _catch and so must not skip an initialization.
    i32     index;
    bytes_t bytes = i_wasmBytes;
    bytes_t end   = i_wasmBytes + 5;
    size_t  ಅಂಕೆ_Bytes;
    u32     size;

_   (SignatureToFuncType(&ftype, i_signature));

    index = *io_functionIndex;

_   (ReadLEB_u32(&size, &bytes, end));
    end = bytes + size;

    ಯದಿ (index >= 0) {
        // index is known non-negative here, so the counts compare as unsigned
        _throwif("function index out of bounds", (u32)index >= i_module->ಅಂಕೆ_Functions);

        function = &i_module->functions[index];

        ಯದಿ (not AreFuncTypesEqual(ftype, function->funcType)) {
            _throw("function type mismatch");
        }
    } ಅನ್ಯಥಾ {
        // add slot to function type table in the module
        u32 funcTypeIndex   = i_module->ಅಂಕೆ_Func_Types++;
        i_module->funcTypes = m3_ReallocArray(IM3FuncType, i_module->funcTypes, i_module->ಅಂಕೆ_Func_Types, funcTypeIndex);
        _throwifnull(i_module->funcTypes);

        // add functype object to the environment
        Environment_AddFuncType(i_module->environment, &ftype);
        i_module->funcTypes[funcTypeIndex] = ftype;

        ftype = ರಿಕ್ತ; // prevent freeing below

        index = (i32)i_module->ಅಂಕೆ_Functions;
_       (Module_AddFunction(i_module, funcTypeIndex, ರಿಕ್ತ));
        function = Module_GetFunction(i_module, index);

        *io_functionIndex = index;
    }

    function->compiled = ರಿಕ್ತ;

    ಯದಿ (function->ownsWasmCode) {
        m3_Free(function->wasm);
    }

    ಅಂಕೆ_Bytes       = end - i_wasmBytes;
    function->wasm = m3_CopyMem(i_wasmBytes, ಅಂಕೆ_Bytes);
    _throwifnull(function->wasm);

    function->wasmEnd      = function->wasm + ಅಂಕೆ_Bytes;
    function->ownsWasmCode = true;

    function->module = i_module;

    ಯದಿ (i_doCompilation and not i_module->runtime) {
        _throw("module must be loaded into runtime to compile function");
    }

_   (CompileFunction(function));

    _catch:
    m3_Free(ftype);

    ಪ್ರತ್ಯಾಗಮ result;
}


IM3Function m3_GetFunctionByIndex (IM3Module i_module, uint32_t i_index)
{
    ಪ್ರತ್ಯಾಗಮ Module_GetFunction(i_module, i_index);
}
