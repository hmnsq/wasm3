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
        module->پیل_Function = -1;
        module->environment   = i_environment;

        module->wasm_پیل = عدم;
        module->wasm_پای_ټکی   = عدم;
    }

    أرجع module;
}


M3_پایله m3_InjectFunction (IM3Module            i_module,
                            int32_t*             io_function_شاخص,
                            ثابت حرف* ثابت    i_signature,
                            ثابت uint8_t* ثابت i_wasm_بایټونه,
                            bool                 i_doCompilation)
{
    M3_پایله پایله = m3Err_none;                                       d_m3Assert (io_function_شاخص);

    IM3Function function = عدم;
    IM3FuncType ftype    = عدم;

    // Declared before the first throw below, which jumps past this point to
    // _catch and so must not skip an initialization.
    i32     index;
    بایټونه_t بایټونه = i_wasm_بایټونه;
    بایټونه_t پای_ټکی   = i_wasm_بایټونه + 5;
    size_t  عدد_بایټونه;
    u32     اندازه;

_   (Signature_منزل_Func_Type(&ftype, i_signature));

    index = *io_function_شاخص;

_   (ReadLEB_u32(&اندازه, &بایټونه, پای_ټکی));
    پای_ټکی = بایټونه + اندازه;

    إذا (index >= 0) {
        // index is known non-negative here, so the counts compare as unsigned
        _throwif("function index out of bounds", (u32)index >= i_module->عدد_Functions);

        function = &i_module->functions[index];

        إذا (not AreFuncTypesEqual(ftype, function->funcType)) {
            _throw("function type mismatch");
        }
    } وإلا {
        // add slot to function type table in the module
        u32 func_Type_شاخص   = i_module->عدد_Func_Types++;
        i_module->funcTypes = m3_ReallocArray(IM3FuncType, i_module->funcTypes, i_module->عدد_Func_Types, func_Type_شاخص);
        _throwifnull(i_module->funcTypes);

        // add functype object to the environment
        Environment_جمع_کړه_Func_Type(i_module->environment, &ftype);
        i_module->funcTypes[func_Type_شاخص] = ftype;

        ftype = عدم; // prevent freeing below

        index = (i32)i_module->عدد_Functions;
_       (Module_جمع_کړه_Function(i_module, func_Type_شاخص, عدم));
        function = Module_GetFunction(i_module, index);

        *io_function_شاخص = index;
    }

    function->compiled = عدم;

    إذا (function->owns_Wasm_کوډ) {
        m3_Free(function->wasm);
    }

    عدد_بایټونه       = پای_ټکی - i_wasm_بایټونه;
    function->wasm = m3_CopyMem(i_wasm_بایټونه, عدد_بایټونه);
    _throwifnull(function->wasm);

    function->wasm_پای_ټکی      = function->wasm + عدد_بایټونه;
    function->owns_Wasm_کوډ = true;

    function->module = i_module;

    إذا (i_doCompilation and not i_module->runtime) {
        _throw("module must be loaded into runtime to compile function");
    }

_   (CompileFunction(function));

    _catch:
    m3_Free(ftype);

    أرجع پایله;
}


IM3Function m3_Get_Function_By_شاخص (IM3Module i_module, uint32_t i_شاخص)
{
    أرجع Module_GetFunction(i_module, i_شاخص);
}
