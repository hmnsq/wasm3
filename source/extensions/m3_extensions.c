//
//  m3_extensions.c
//
//  Created by Steven Massey on 3/30/21.
//  Copyright © 2021 Steven Massey. All rights reserved.
//

#अन्तर्भाव "wasm3_ext.h"

#अन्तर्भाव "m3_env.h"
#अन्तर्भाव "m3_bind.h"
#अन्तर्भाव "m3_exception.h"


IM3Module m3_NewModule (IM3Environment i_environment)
{
    IM3Module module = m3_AllocStruct(M3Module);

    यदि (module) {
        module->name          = ".unnamed";
        module->startFunction = -1;
        module->environment   = i_environment;

        module->wasmStart = रिक्त;
        module->wasm_अन्तः   = रिक्त;
    }

    प्रत्यागम module;
}


M3_परिणाम m3_InjectFunction (IM3Module            i_module,
                            int32_t*             io_function_सूचकांक,
                            ध्रुव वर्ण* ध्रुव    i_signature,
                            ध्रुव uint8_t* ध्रुव i_wasm_अष्टकानि,
                            bool                 i_doCompilation)
{
    M3_परिणाम परिणाम = m3Err_none;                                       d_m3Assert (io_function_सूचकांक);

    IM3Function function = रिक्त;
    IM3FuncType ftype    = रिक्त;

    // Declared before the first throw below, which jumps past this point to
    // _catch and so must not skip an initialization.
    i32     index;
    अष्टकानि_t अष्टकानि = i_wasm_अष्टकानि;
    अष्टकानि_t अन्तः   = i_wasm_अष्टकानि + 5;
    size_t  अङ्कः_अष्टकानि;
    u32     आकार;

_   (Signature_गन्तव्यम्_Func_Type(&ftype, i_signature));

    index = *io_function_सूचकांक;

_   (ReadLEB_u32(&आकार, &अष्टकानि, अन्तः));
    अन्तः = अष्टकानि + आकार;

    यदि (index >= 0) {
        // index is known non-negative here, so the counts compare as unsigned
        _throwif("function index out of bounds", (u32)index >= i_module->अङ्कः_Functions);

        function = &i_module->functions[index];

        यदि (not AreFuncTypesEqual(ftype, function->funcType)) {
            _throw("function type mismatch");
        }
    } अन्यथा {
        // add slot to function type table in the module
        u32 func_Type_सूचकांक   = i_module->अङ्कः_Func_Types++;
        i_module->funcTypes = m3_ReallocArray(IM3FuncType, i_module->funcTypes, i_module->अङ्कः_Func_Types, func_Type_सूचकांक);
        _throwifnull(i_module->funcTypes);

        // add functype object to the environment
        Environment_सङ्कलय_Func_Type(i_module->environment, &ftype);
        i_module->funcTypes[func_Type_सूचकांक] = ftype;

        ftype = रिक्त; // prevent freeing below

        index = (i32)i_module->अङ्कः_Functions;
_       (Module_सङ्कलय_Function(i_module, func_Type_सूचकांक, रिक्त));
        function = Module_GetFunction(i_module, index);

        *io_function_सूचकांक = index;
    }

    function->compiled = रिक्त;

    यदि (function->owns_Wasm_संहिता) {
        m3_Free(function->wasm);
    }

    अङ्कः_अष्टकानि       = अन्तः - i_wasm_अष्टकानि;
    function->wasm = m3_CopyMem(i_wasm_अष्टकानि, अङ्कः_अष्टकानि);
    _throwifnull(function->wasm);

    function->wasm_अन्तः      = function->wasm + अङ्कः_अष्टकानि;
    function->owns_Wasm_संहिता = true;

    function->module = i_module;

    यदि (i_doCompilation and not i_module->runtime) {
        _throw("module must be loaded into runtime to compile function");
    }

_   (CompileFunction(function));

    _catch:
    m3_Free(ftype);

    प्रत्यागम परिणाम;
}


IM3Function m3_Get_Function_By_सूचकांक (IM3Module i_module, uint32_t i_सूचकांक)
{
    प्रत्यागम Module_GetFunction(i_module, i_सूचकांक);
}
