//
//  m3_extensions.c
//
//  Created by Steven Massey on 3/30/21.
//  Copyright © 2021 Steven Massey. All rights reserved.
//

#ਅਨ੍ਤਰ੍ਭਾਵ "wasm3_ext.h"

#ਅਨ੍ਤਰ੍ਭਾਵ "m3_env.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "m3_bind.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "m3_exception.h"


IM3Module m3_NewModule (IM3Environment i_environment)
{
    IM3Module module = m3_AllocStruct(M3Module);

    ਜੇ (module) {
        module->name          = ".unnamed";
        module->ਸ਼ੁਰੂ_Function = -1;
        module->environment   = i_environment;

        module->wasm_ਸ਼ੁਰੂ = ਨਿਰਾ;
        module->wasm_ਸਿਰਾ   = ਨਿਰਾ;
    }

    ਵਾਪਸ module;
}


M3_ਨਤੀਜਾ m3_InjectFunction (IM3Module            i_module,
                            int32_t*             io_function_ਸੂਚਕਾਂਕ,
                            ਅਚਲ ਅੱਖਰ* ਅਚਲ    i_signature,
                            ਅਚਲ uint8_t* ਅਚਲ i_wasm_ਬਾਈਟਾਂ,
                            bool                 i_doCompilation)
{
    M3_ਨਤੀਜਾ ਨਤੀਜਾ = m3Err_none;                                       d_m3Assert (io_function_ਸੂਚਕਾਂਕ);

    IM3Function function = ਨਿਰਾ;
    IM3FuncType ftype    = ਨਿਰਾ;

    // Declared before the first throw below, which jumps past this point to
    // _catch and so must not skip an initialization.
    i32     index;
    ਬਾਈਟਾਂ_t ਬਾਈਟਾਂ = i_wasm_ਬਾਈਟਾਂ;
    ਬਾਈਟਾਂ_t ਸਿਰਾ   = i_wasm_ਬਾਈਟਾਂ + 5;
    size_t  ਅੰਕ_ਬਾਈਟਾਂ;
    u32     ਮਾਪ;

_   (Signature_ਮੰਜ਼ਿਲ_Func_Type(&ftype, i_signature));

    index = *io_function_ਸੂਚਕਾਂਕ;

_   (ReadLEB_u32(&ਮਾਪ, &ਬਾਈਟਾਂ, ਸਿਰਾ));
    ਸਿਰਾ = ਬਾਈਟਾਂ + ਮਾਪ;

    ਜੇ (index >= 0) {
        // index is known non-negative here, so the counts compare as unsigned
        _throwif("function index out of bounds", (u32)index >= i_module->ਅੰਕ_Functions);

        function = &i_module->functions[index];

        ਜੇ (not AreFuncTypesEqual(ftype, function->funcType)) {
            _throw("function type mismatch");
        }
    } ਨਹੀਂਤਾਂ {
        // add slot to function type table in the module
        u32 func_Type_ਸੂਚਕਾਂਕ   = i_module->ਅੰਕ_Func_Types++;
        i_module->funcTypes = m3_ReallocArray(IM3FuncType, i_module->funcTypes, i_module->ਅੰਕ_Func_Types, func_Type_ਸੂਚਕਾਂਕ);
        _throwifnull(i_module->funcTypes);

        // add functype object to the environment
        Environment_ਜੋੜੋ_Func_Type(i_module->environment, &ftype);
        i_module->funcTypes[func_Type_ਸੂਚਕਾਂਕ] = ftype;

        ftype = ਨਿਰਾ; // prevent freeing below

        index = (i32)i_module->ਅੰਕ_Functions;
_       (Module_ਜੋੜੋ_Function(i_module, func_Type_ਸੂਚਕਾਂਕ, ਨਿਰਾ));
        function = Module_GetFunction(i_module, index);

        *io_function_ਸੂਚਕਾਂਕ = index;
    }

    function->compiled = ਨਿਰਾ;

    ਜੇ (function->owns_Wasm_ਕੋਡ) {
        m3_Free(function->wasm);
    }

    ਅੰਕ_ਬਾਈਟਾਂ       = ਸਿਰਾ - i_wasm_ਬਾਈਟਾਂ;
    function->wasm = m3_CopyMem(i_wasm_ਬਾਈਟਾਂ, ਅੰਕ_ਬਾਈਟਾਂ);
    _throwifnull(function->wasm);

    function->wasm_ਸਿਰਾ      = function->wasm + ਅੰਕ_ਬਾਈਟਾਂ;
    function->owns_Wasm_ਕੋਡ = true;

    function->module = i_module;

    ਜੇ (i_doCompilation and not i_module->runtime) {
        _throw("module must be loaded into runtime to compile function");
    }

_   (CompileFunction(function));

    _catch:
    m3_Free(ftype);

    ਵਾਪਸ ਨਤੀਜਾ;
}


IM3Function m3_Get_Function_By_ਸੂਚਕਾਂਕ (IM3Module i_module, uint32_t i_ਸੂਚਕਾਂਕ)
{
    ਵਾਪਸ Module_GetFunction(i_module, i_ਸੂਚਕਾਂਕ);
}
