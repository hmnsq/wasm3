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

    ಆದರೆ (module) {
        module->name          = ".unnamed";
        module->ಆರಂಭ_Function = -1;
        module->environment   = i_environment;

        module->wasm_ಆರಂಭ = ಖಾಲಿ;
        module->wasm_ಕೊನೆ   = ಖಾಲಿ;
    }

    ಹಿಂತಿರುಗಿ module;
}


M3_ಫಲಿತಾಂಶ m3_InjectFunction (IM3Module            i_module,
                            int32_t*             io_function_ಸೂಚ್ಯಂಕ,
                            ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* ಸ್ಥಿರಾಂಕ    i_signature,
                            ಸ್ಥಿರಾಂಕ uint8_t* ಸ್ಥಿರಾಂಕ i_wasm_ಬೈಟುಗಳು,
                            bool                 i_doCompilation)
{
    M3_ಫಲಿತಾಂಶ ಫಲಿತಾಂಶ = m3Err_none;                                       d_m3Assert (io_function_ಸೂಚ್ಯಂಕ);

    IM3Function function = ಖಾಲಿ;
    IM3FuncType ftype    = ಖಾಲಿ;

    // Declared before the first throw below, which jumps past this point to
    // _catch and so must not skip an initialization.
    i32     index;
    ಬೈಟುಗಳು_t ಬೈಟುಗಳು = i_wasm_ಬೈಟುಗಳು;
    ಬೈಟುಗಳು_t ಕೊನೆ   = i_wasm_ಬೈಟುಗಳು + 5;
    size_t  ಅಂಕೆ_ಬೈಟುಗಳು;
    u32     ಗಾತ್ರ_2;

_   (Signature_ಗಮ್ಯ_Func_Type(&ftype, i_signature));

    index = *io_function_ಸೂಚ್ಯಂಕ;

_   (ReadLEB_u32(&ಗಾತ್ರ_2, &ಬೈಟುಗಳು, ಕೊನೆ));
    ಕೊನೆ = ಬೈಟುಗಳು + ಗಾತ್ರ_2;

    ಆದರೆ (index >= 0) {
        // index is known non-negative here, so the counts compare as unsigned
        _throwif("function index out of bounds", (u32)index >= i_module->ಅಂಕೆ_Functions);

        function = &i_module->functions[index];

        ಆದರೆ (not AreFuncTypesEqual(ftype, function->funcType)) {
            _throw("function type mismatch");
        }
    } ಇಲ್ಲದಿದ್ದರೆ {
        // add slot to function type table in the module
        u32 func_Type_ಸೂಚ್ಯಂಕ   = i_module->ಅಂಕೆ_Func_Types++;
        i_module->funcTypes = m3_ReallocArray(IM3FuncType, i_module->funcTypes, i_module->ಅಂಕೆ_Func_Types, func_Type_ಸೂಚ್ಯಂಕ);
        _throwifnull(i_module->funcTypes);

        // add functype object to the environment
        Environment_ಕೂಡಿಸು_Func_Type(i_module->environment, &ftype);
        i_module->funcTypes[func_Type_ಸೂಚ್ಯಂಕ] = ftype;

        ftype = ಖಾಲಿ; // prevent freeing below

        index = (i32)i_module->ಅಂಕೆ_Functions;
_       (Module_ಕೂಡಿಸು_Function(i_module, func_Type_ಸೂಚ್ಯಂಕ, ಖಾಲಿ));
        function = Module_GetFunction(i_module, index);

        *io_function_ಸೂಚ್ಯಂಕ = index;
    }

    function->compiled = ಖಾಲಿ;

    ಆದರೆ (function->owns_Wasm_ಸಂಕೇತ) {
        m3_Free(function->wasm);
    }

    ಅಂಕೆ_ಬೈಟುಗಳು       = ಕೊನೆ - i_wasm_ಬೈಟುಗಳು;
    function->wasm = m3_CopyMem(i_wasm_ಬೈಟುಗಳು, ಅಂಕೆ_ಬೈಟುಗಳು);
    _throwifnull(function->wasm);

    function->wasm_ಕೊನೆ      = function->wasm + ಅಂಕೆ_ಬೈಟುಗಳು;
    function->owns_Wasm_ಸಂಕೇತ = true;

    function->module = i_module;

    ಆದರೆ (i_doCompilation and not i_module->runtime) {
        _throw("module must be loaded into runtime to compile function");
    }

_   (CompileFunction(function));

    _catch:
    m3_Free(ftype);

    ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}


IM3Function m3_Get_Function_By_ಸೂಚ್ಯಂಕ (IM3Module i_module, uint32_t i_ಸೂಚ್ಯಂಕ)
{
    ಹಿಂತಿರುಗಿ Module_GetFunction(i_module, i_ಸೂಚ್ಯಂಕ);
}
