//
//  m3_extensions.c
//
//  Created by Steven Massey on 3/30/21.
//  Copyright © 2021 Steven Massey. All rights reserved.
//

#అన్తర్భావ "wasm3_ext.h"

#అన్తర్భావ "m3_env.h"
#అన్తర్భావ "m3_bind.h"
#అన్తర్భావ "m3_exception.h"


IM3Module m3_NewModule (IM3Environment i_environment)
{
    IM3Module module = m3_AllocStruct(M3Module);

    ఒకవేళ (module) {
        module->name          = ".unnamed";
        module->ప్రారంభం_Function = -1;
        module->environment   = i_environment;

        module->wasm_ప్రారంభం = ఖాళీ;
        module->wasm_చివర   = ఖాళీ;
    }

    తిరిగి module;
}


M3_ఫలితం m3_InjectFunction (IM3Module            i_module,
                            int32_t*             io_function_సూచీ,
                            స్థిరరాశి అక్షరం* స్థిరరాశి    i_signature,
                            స్థిరరాశి uint8_t* స్థిరరాశి i_wasm_బైట్లు,
                            bool                 i_doCompilation)
{
    M3_ఫలితం ఫలితం = m3Err_none;                                       d_m3Assert (io_function_సూచీ);

    IM3Function function = ఖాళీ;
    IM3FuncType ftype    = ఖాళీ;

    // Declared before the first throw below, which jumps past this point to
    // _catch and so must not skip an initialization.
    i32     index;
    బైట్లు_t బైట్లు = i_wasm_బైట్లు;
    బైట్లు_t చివర   = i_wasm_బైట్లు + 5;
    size_t  అంకె_బైట్లు;
    u32     కొలత;

_   (Signature_గమ్యం_Func_Type(&ftype, i_signature));

    index = *io_function_సూచీ;

_   (ReadLEB_u32(&కొలత, &బైట్లు, చివర));
    చివర = బైట్లు + కొలత;

    ఒకవేళ (index >= 0) {
        // index is known non-negative here, so the counts compare as unsigned
        _throwif("function index out of bounds", (u32)index >= i_module->అంకె_Functions);

        function = &i_module->functions[index];

        ఒకవేళ (not AreFuncTypesEqual(ftype, function->funcType)) {
            _throw("function type mismatch");
        }
    } లేకపోతే {
        // add slot to function type table in the module
        u32 func_Type_సూచీ   = i_module->అంకె_Func_Types++;
        i_module->funcTypes = m3_ReallocArray(IM3FuncType, i_module->funcTypes, i_module->అంకె_Func_Types, func_Type_సూచీ);
        _throwifnull(i_module->funcTypes);

        // add functype object to the environment
        Environment_కూడు_Func_Type(i_module->environment, &ftype);
        i_module->funcTypes[func_Type_సూచీ] = ftype;

        ftype = ఖాళీ; // prevent freeing below

        index = (i32)i_module->అంకె_Functions;
_       (Module_కూడు_Function(i_module, func_Type_సూచీ, ఖాళీ));
        function = Module_GetFunction(i_module, index);

        *io_function_సూచీ = index;
    }

    function->compiled = ఖాళీ;

    ఒకవేళ (function->owns_Wasm_సంకేతం) {
        m3_Free(function->wasm);
    }

    అంకె_బైట్లు       = చివర - i_wasm_బైట్లు;
    function->wasm = m3_CopyMem(i_wasm_బైట్లు, అంకె_బైట్లు);
    _throwifnull(function->wasm);

    function->wasm_చివర      = function->wasm + అంకె_బైట్లు;
    function->owns_Wasm_సంకేతం = true;

    function->module = i_module;

    ఒకవేళ (i_doCompilation and not i_module->runtime) {
        _throw("module must be loaded into runtime to compile function");
    }

_   (CompileFunction(function));

    _catch:
    m3_Free(ftype);

    తిరిగి ఫలితం;
}


IM3Function m3_Get_Function_By_సూచీ (IM3Module i_module, uint32_t i_సూచీ)
{
    తిరిగి Module_GetFunction(i_module, i_సూచీ);
}
