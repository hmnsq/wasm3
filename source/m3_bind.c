//
//  m3_bind.c
//
//  Created by Steven Massey on 4/29/19.
//  Copyright © 2019 Steven Massey. All rights reserved.
//

#అన్తర్భావ "m3_env.h"
#అన్తర్భావ "m3_bind.h"
#అన్తర్భావ "m3_exception.h"
#అన్తర్భావ "m3_info.h"


u8 Convert_Type_Char_గమ్యం_Type_Id (అక్షరం i_సంకేతం)
{
    ఎంపిక (i_సంకేతం) {
    స్థితి 'v': తిరిగి c_m3Type_none;
    స్థితి 'i': తిరిగి c_m3Type_i32;
    స్థితి 'I': తిరిగి c_m3Type_i64;
    స్థితి 'f': తిరిగి c_m3Type_f32;
    స్థితి 'F': తిరిగి c_m3Type_f64;
    స్థితి '*': తిరిగి c_m3Type_i32;
    // same letters c_waCompactTypes uses for the reference types
    స్థితి 'r': తిరిగి c_m3Type_funcref;
    స్థితి 'R': తిరిగి c_m3Type_externref;
    స్థితి 'c': తిరిగి c_m3Type_contref;
    }
    తిరిగి c_m3Type_unknown;
}


M3_ఫలితం Signature_గమ్యం_Func_Type (IM3FuncType* o_functionType, ccstr_t i_signature)
{
    IM3FuncType funcType = ఖాళీ;

_try {
    ఒకవేళ (not o_functionType) {
        _throw("null function type");
    }

    ఒకవేళ (not i_signature) {
        _throw("null function signature");
    }

    cstr_t sig = i_signature;

    size_t max_అంకె_Types = అక్షరశ్రేణి_పొడవు(i_signature);

    // assume min signature is "()"
    _throwif(m3Err_malformedFunctionSignature, max_అంకె_Types < 2);
    max_అంకె_Types -= 2;

    _throwif(m3_Err_too_Many_వాదనలు_Rets, max_అంకె_Types > d_m3_Max_Sane_Function_Arg_Ret_లెక్క);

_   (AllocFuncType(&funcType, (u32)max_అంకె_Types));

    m3type_t* typelist = funcType->types;

    bool parsingRets = true;
    అయితే (*sig) {
        అక్షరం typeChar = *sig++;

        ఒకవేళ (typeChar == '(') {
            parsingRets = false;
            కొనసాగు;
        } లేకపోతే ఒకవేళ (typeChar == ' ') {
            కొనసాగు;
        } లేకపోతే ఒకవేళ (typeChar == ')') {
            ఆపు;
        }

        u8 type = Convert_Type_Char_గమ్యం_Type_Id(typeChar);

        _throwif("unknown argument type char", c_m3Type_unknown == type);

        ఒకవేళ (type == c_m3Type_none) {
            కొనసాగు;
        }

        ఒకవేళ (parsingRets) {
            _throwif("malformed signature; return count overflow", funcType->అంకె_Rets >= max_అంకె_Types);
            funcType->అంకె_Rets++;
            *typelist++ = type;
        } లేకపోతే {
            _throwif("malformed signature; arg count overflow", (u32)(funcType->అంకె_Rets) + funcType->అంకె_వాదనలు >= max_అంకె_Types);
            funcType->అంకె_వాదనలు++;
            *typelist++ = type;
        }
    }

} _catch:

    ఒకవేళ (ఫలితం) {
        m3_Free(funcType);
    }

    *o_functionType = funcType;

    తిరిగి ఫలితం;
}


స్థిర
M3_ఫలితం ValidateSignature (IM3Function i_function, ccstr_t i_linkingSignature)
{
    M3_ఫలితం ఫలితం = m3Err_none;

    IM3FuncType ftype = ఖాళీ;
_   (Signature_గమ్యం_Func_Type(&ftype, i_linkingSignature));

    ఒకవేళ (not AreFuncTypesEqual(ftype, i_function->funcType)) {
        m3log(module, "expected: %s", SPrintFuncTypeSignature(ftype));
        m3log(module, "   found: %s", SPrintFuncTypeSignature(i_function->funcType));

        _throw(m3Err_functionSignatureMismatch);
    }

    _catch:

    m3_Free(ftype);

    తిరిగి ఫలితం;
}


M3_ఫలితం కనుగొను_And_Link_Function (IM3Module io_module,
                              ccstr_t   i_moduleName,
                              ccstr_t   i_functionName,
                              ccstr_t   i_signature,
                              voidptr_t i_function,
                              voidptr_t i_userdata)
{
_try {
    _throwif(m3Err_moduleNotLinked, !io_module->runtime);

    స్థిరరాశి bool wildcardModule = (strcmp(i_moduleName, "*") == 0);

    ఫలితం = m3Err_functionLookupFailed;

    కొరకు (u32 i = 0; i < io_module->అంకె_Functions; ++i) {
        స్థిరరాశి IM3Function f = &io_module->functions[i];

        ఒకవేళ (f->import.moduleUtf8 and f->import.క్షేత్రం_Utf8) {
            ఒకవేళ (strcmp(f->import.క్షేత్రం_Utf8, i_functionName) == 0 and
                (wildcardModule or strcmp(f->import.moduleUtf8, i_moduleName) == 0)) {
                ఒకవేళ (i_signature) {
_                   (ValidateSignature(f, i_signature));
                }
_               (CompileRawFunction(io_module, f, i_function, i_userdata));
            }
        }
    }
} _catch:
    తిరిగి ఫలితం;
}

M3_ఫలితం m3_LinkRawFunctionEx (IM3Module         io_module,
                               స్థిరరాశి అక్షరం* స్థిరరాశి i_moduleName,
                               స్థిరరాశి అక్షరం* స్థిరరాశి i_functionName,
                               స్థిరరాశి అక్షరం* స్థిరరాశి i_signature,
                               M3RawCall         i_function,
                               స్థిరరాశి శూన్య*       i_userdata)
{
    తిరిగి కనుగొను_And_Link_Function(io_module, i_moduleName, i_functionName, i_signature, (voidptr_t)i_function, i_userdata);
}

M3_ఫలితం m3_LinkRawFunction (IM3Module         io_module,
                             స్థిరరాశి అక్షరం* స్థిరరాశి i_moduleName,
                             స్థిరరాశి అక్షరం* స్థిరరాశి i_functionName,
                             స్థిరరాశి అక్షరం* స్థిరరాశి i_signature,
                             M3RawCall         i_function)
{
    తిరిగి కనుగొను_And_Link_Function(io_module, i_moduleName, i_functionName, i_signature, (voidptr_t)i_function, ఖాళీ);
}
