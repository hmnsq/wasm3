//
//  m3_extensions.c
//
//  Created by Steven Massey on 3/30/21.
//  Copyright © 2021 Steven Massey. All rights reserved.
//

#شامل "wasm3_ext.h"

#شامل "m3_env.h"
#شامل "m3_bind.h"
#شامل "m3_exception.h"


IM3Module m3_NewModule (IM3Environment i_environment)
{
    IM3Module module = m3_AllocStruct(M3Module);

    اگر (module) {
        module->name          = ".unnamed";
        module->آغاز_Function = -1;
        module->environment   = i_environment;

        module->wasm_آغاز = تہی;
        module->wasm_انتہا   = تہی;
    }

    واپس module;
}


M3_نتیجہ m3_InjectFunction (IM3Module            i_module,
                            int32_t*             io_function_اشاریہ,
                            ثابت حرف* ثابت    i_signature,
                            ثابت uint8_t* ثابت i_wasm_بائٹیں,
                            bool                 i_doCompilation)
{
    M3_نتیجہ نتیجہ = m3Err_none;                                       d_m3Assert (io_function_اشاریہ);

    IM3Function function = تہی;
    IM3FuncType ftype    = تہی;

    // Declared before the first throw below, which jumps past this point to
    // _catch and so must not skip an initialization.
    i32     index;
    بائٹیں_t بائٹیں = i_wasm_بائٹیں;
    بائٹیں_t انتہا   = i_wasm_بائٹیں + 5;
    size_t  عدد_بائٹیں;
    u32     سائز;

_   (Signature_مقصد_Func_Type(&ftype, i_signature));

    index = *io_function_اشاریہ;

_   (ReadLEB_u32(&سائز, &بائٹیں, انتہا));
    انتہا = بائٹیں + سائز;

    اگر (index >= 0) {
        // index is known non-negative here, so the counts compare as unsigned
        _throwif("function index out of bounds", (u32)index >= i_module->عدد_Functions);

        function = &i_module->functions[index];

        اگر (not AreFuncTypesEqual(ftype, function->funcType)) {
            _throw("function type mismatch");
        }
    } ورنہ {
        // add slot to function type table in the module
        u32 func_Type_اشاریہ   = i_module->عدد_Func_Types++;
        i_module->funcTypes = m3_ReallocArray(IM3FuncType, i_module->funcTypes, i_module->عدد_Func_Types, func_Type_اشاریہ);
        _throwifnull(i_module->funcTypes);

        // add functype object to the environment
        Environment_جوڑو_Func_Type(i_module->environment, &ftype);
        i_module->funcTypes[func_Type_اشاریہ] = ftype;

        ftype = تہی; // prevent freeing below

        index = (i32)i_module->عدد_Functions;
_       (Module_جوڑو_Function(i_module, func_Type_اشاریہ, تہی));
        function = Module_GetFunction(i_module, index);

        *io_function_اشاریہ = index;
    }

    function->compiled = تہی;

    اگر (function->owns_Wasm_رمز) {
        m3_Free(function->wasm);
    }

    عدد_بائٹیں       = انتہا - i_wasm_بائٹیں;
    function->wasm = m3_CopyMem(i_wasm_بائٹیں, عدد_بائٹیں);
    _throwifnull(function->wasm);

    function->wasm_انتہا      = function->wasm + عدد_بائٹیں;
    function->owns_Wasm_رمز = true;

    function->module = i_module;

    اگر (i_doCompilation and not i_module->runtime) {
        _throw("module must be loaded into runtime to compile function");
    }

_   (CompileFunction(function));

    _catch:
    m3_Free(ftype);

    واپس نتیجہ;
}


IM3Function m3_Get_Function_By_اشاریہ (IM3Module i_module, uint32_t i_اشاریہ)
{
    واپس Module_GetFunction(i_module, i_اشاریہ);
}
