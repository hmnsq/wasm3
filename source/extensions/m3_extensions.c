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
        module->شروعات_Function = -1;
        module->environment   = i_environment;

        module->wasm_شروعات = عدم;
        module->wasm_پڇاڙي   = عدم;
    }

    أرجع module;
}


M3_نتيجو m3_InjectFunction (IM3Module            i_module,
                            int32_t*             io_function_اشاريو,
                            ثابت حرف* ثابت    i_signature,
                            ثابت uint8_t* ثابت i_wasm_بائيٽون,
                            bool                 i_doCompilation)
{
    M3_نتيجو نتيجو = m3Err_none;                                       d_m3Assert (io_function_اشاريو);

    IM3Function function = عدم;
    IM3FuncType ftype    = عدم;

    // Declared before the first throw below, which jumps past this point to
    // _catch and so must not skip an initialization.
    i32     index;
    بائيٽون_t بائيٽون = i_wasm_بائيٽون;
    بائيٽون_t پڇاڙي   = i_wasm_بائيٽون + 5;
    size_t  انگ_بائيٽون;
    u32     سائيز;

_   (Signature_منزل_Func_Type(&ftype, i_signature));

    index = *io_function_اشاريو;

_   (ReadLEB_u32(&سائيز, &بائيٽون, پڇاڙي));
    پڇاڙي = بائيٽون + سائيز;

    إذا (index >= 0) {
        // index is known non-negative here, so the counts compare as unsigned
        _throwif("function index out of bounds", (u32)index >= i_module->انگ_Functions);

        function = &i_module->functions[index];

        إذا (not AreFuncTypesEqual(ftype, function->funcType)) {
            _throw("function type mismatch");
        }
    } وإلا {
        // add slot to function type table in the module
        u32 func_Type_اشاريو   = i_module->انگ_Func_Types++;
        i_module->funcTypes = m3_ReallocArray(IM3FuncType, i_module->funcTypes, i_module->انگ_Func_Types, func_Type_اشاريو);
        _throwifnull(i_module->funcTypes);

        // add functype object to the environment
        Environment_جوڙيو_Func_Type(i_module->environment, &ftype);
        i_module->funcTypes[func_Type_اشاريو] = ftype;

        ftype = عدم; // prevent freeing below

        index = (i32)i_module->انگ_Functions;
_       (Module_جوڙيو_Function(i_module, func_Type_اشاريو, عدم));
        function = Module_GetFunction(i_module, index);

        *io_function_اشاريو = index;
    }

    function->compiled = عدم;

    إذا (function->owns_Wasm_ڪوڊ) {
        m3_Free(function->wasm);
    }

    انگ_بائيٽون       = پڇاڙي - i_wasm_بائيٽون;
    function->wasm = m3_CopyMem(i_wasm_بائيٽون, انگ_بائيٽون);
    _throwifnull(function->wasm);

    function->wasm_پڇاڙي      = function->wasm + انگ_بائيٽون;
    function->owns_Wasm_ڪوڊ = true;

    function->module = i_module;

    إذا (i_doCompilation and not i_module->runtime) {
        _throw("module must be loaded into runtime to compile function");
    }

_   (CompileFunction(function));

    _catch:
    m3_Free(ftype);

    أرجع نتيجو;
}


IM3Function m3_Get_Function_By_اشاريو (IM3Module i_module, uint32_t i_اشاريو)
{
    أرجع Module_GetFunction(i_module, i_اشاريو);
}
