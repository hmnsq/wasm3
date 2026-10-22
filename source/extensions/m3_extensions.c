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
        module->شروع_Function = -1;
        module->environment   = i_environment;

        module->wasm_شروع = عدم;
        module->wasm_سرا   = عدم;
    }

    أرجع module;
}


M3_نتیجہ m3_InjectFunction (IM3Module            i_module,
                            int32_t*             io_function_اشاریہ,
                            ثابت حرف* ثابت    i_signature,
                            ثابت uint8_t* ثابت i_wasm_بائٹاں,
                            bool                 i_doCompilation)
{
    M3_نتیجہ نتیجہ = m3Err_none;                                       d_m3Assert (io_function_اشاریہ);

    IM3Function function = عدم;
    IM3FuncType ftype    = عدم;

    // Declared before the first throw below, which jumps past this point to
    // _catch and so must not skip an initialization.
    i32     index;
    بائٹاں_t بائٹاں = i_wasm_بائٹاں;
    بائٹاں_t سرا   = i_wasm_بائٹاں + 5;
    size_t  عدد_بائٹاں;
    u32     ناپ;

_   (Signature_منزل_Func_Type(&ftype, i_signature));

    index = *io_function_اشاریہ;

_   (ReadLEB_u32(&ناپ, &بائٹاں, سرا));
    سرا = بائٹاں + ناپ;

    إذا (index >= 0) {
        // index is known non-negative here, so the counts compare as unsigned
        _throwif("function index out of bounds", (u32)index >= i_module->عدد_Functions);

        function = &i_module->functions[index];

        إذا (not AreFuncTypesEqual(ftype, function->funcType)) {
            _throw("function type mismatch");
        }
    } وإلا {
        // add slot to function type table in the module
        u32 func_Type_اشاریہ   = i_module->عدد_Func_Types++;
        i_module->funcTypes = m3_ReallocArray(IM3FuncType, i_module->funcTypes, i_module->عدد_Func_Types, func_Type_اشاریہ);
        _throwifnull(i_module->funcTypes);

        // add functype object to the environment
        Environment_جوڑو_Func_Type(i_module->environment, &ftype);
        i_module->funcTypes[func_Type_اشاریہ] = ftype;

        ftype = عدم; // prevent freeing below

        index = (i32)i_module->عدد_Functions;
_       (Module_جوڑو_Function(i_module, func_Type_اشاریہ, عدم));
        function = Module_GetFunction(i_module, index);

        *io_function_اشاریہ = index;
    }

    function->compiled = عدم;

    إذا (function->owns_Wasm_کوڈ) {
        m3_Free(function->wasm);
    }

    عدد_بائٹاں       = سرا - i_wasm_بائٹاں;
    function->wasm = m3_CopyMem(i_wasm_بائٹاں, عدد_بائٹاں);
    _throwifnull(function->wasm);

    function->wasm_سرا      = function->wasm + عدد_بائٹاں;
    function->owns_Wasm_کوڈ = true;

    function->module = i_module;

    إذا (i_doCompilation and not i_module->runtime) {
        _throw("module must be loaded into runtime to compile function");
    }

_   (CompileFunction(function));

    _catch:
    m3_Free(ftype);

    أرجع نتیجہ;
}


IM3Function m3_Get_Function_By_اشاریہ (IM3Module i_module, uint32_t i_اشاریہ)
{
    أرجع Module_GetFunction(i_module, i_اشاریہ);
}
