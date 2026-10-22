//
//  m3_bind.c
//
//  Created by Steven Massey on 4/29/19.
//  Copyright © 2019 Steven Massey. All rights reserved.
//

#അന്തര്ഭാവ "m3_env.h"
#അന്തര്ഭാവ "m3_bind.h"
#അന്തര്ഭാവ "m3_exception.h"
#അന്തര്ഭാവ "m3_info.h"


u8 ConvertTypeCharToTypeId (അക്ഷരം i_code)
{
    തിരഞ്ഞെടുപ്പ് (i_code) {
    അവസ്ഥ 'v': തിരികെ c_m3Type_none;
    അവസ്ഥ 'i': തിരികെ c_m3Type_i32;
    അവസ്ഥ 'I': തിരികെ c_m3Type_i64;
    അവസ്ഥ 'f': തിരികെ c_m3Type_f32;
    അവസ്ഥ 'F': തിരികെ c_m3Type_f64;
    അവസ്ഥ '*': തിരികെ c_m3Type_i32;
    // same letters c_waCompactTypes uses for the reference types
    അവസ്ഥ 'r': തിരികെ c_m3Type_funcref;
    അവസ്ഥ 'R': തിരികെ c_m3Type_externref;
    അവസ്ഥ 'c': തിരികെ c_m3Type_contref;
    }
    തിരികെ c_m3Type_unknown;
}


M3Result SignatureToFuncType (IM3FuncType* o_functionType, ccstr_t i_signature)
{
    IM3FuncType funcType = ശൂന്യ;

_try {
    എങ്കിൽ (not o_functionType) {
        _throw("null function type");
    }

    എങ്കിൽ (not i_signature) {
        _throw("null function signature");
    }

    cstr_t sig = i_signature;

    size_t max_അക്കം_Types = strlen(i_signature);

    // assume min signature is "()"
    _throwif(m3Err_malformedFunctionSignature, max_അക്കം_Types < 2);
    max_അക്കം_Types -= 2;

    _throwif(m3Err_tooManyArgsRets, max_അക്കം_Types > d_m3_Max_Sane_Function_Arg_Ret_എണ്ണം);

_   (AllocFuncType(&funcType, (u32)max_അക്കം_Types));

    m3type_t* typelist = funcType->types;

    bool parsingRets = true;
    ആകുമ്പോൾ (*sig) {
        അക്ഷരം typeChar = *sig++;

        എങ്കിൽ (typeChar == '(') {
            parsingRets = false;
            തുടരുക;
        } അല്ലെങ്കിൽ എങ്കിൽ (typeChar == ' ') {
            തുടരുക;
        } അല്ലെങ്കിൽ എങ്കിൽ (typeChar == ')') {
            നിർത്തുക;
        }

        u8 type = ConvertTypeCharToTypeId(typeChar);

        _throwif("unknown argument type char", c_m3Type_unknown == type);

        എങ്കിൽ (type == c_m3Type_none) {
            തുടരുക;
        }

        എങ്കിൽ (parsingRets) {
            _throwif("malformed signature; return count overflow", funcType->അക്കം_Rets >= max_അക്കം_Types);
            funcType->അക്കം_Rets++;
            *typelist++ = type;
        } അല്ലെങ്കിൽ {
            _throwif("malformed signature; arg count overflow", (u32)(funcType->അക്കം_Rets) + funcType->അക്കം_Args >= max_അക്കം_Types);
            funcType->അക്കം_Args++;
            *typelist++ = type;
        }
    }

} _catch:

    എങ്കിൽ (result) {
        m3_Free(funcType);
    }

    *o_functionType = funcType;

    തിരികെ result;
}


സ്ഥിതം
M3Result ValidateSignature (IM3Function i_function, ccstr_t i_linkingSignature)
{
    M3Result result = m3Err_none;

    IM3FuncType ftype = ശൂന്യ;
_   (SignatureToFuncType(&ftype, i_linkingSignature));

    എങ്കിൽ (not AreFuncTypesEqual(ftype, i_function->funcType)) {
        m3log(module, "expected: %s", SPrintFuncTypeSignature(ftype));
        m3log(module, "   found: %s", SPrintFuncTypeSignature(i_function->funcType));

        _throw(m3Err_functionSignatureMismatch);
    }

    _catch:

    m3_Free(ftype);

    തിരികെ result;
}


M3Result FindAndLinkFunction (IM3Module io_module,
                              ccstr_t   i_moduleName,
                              ccstr_t   i_functionName,
                              ccstr_t   i_signature,
                              voidptr_t i_function,
                              voidptr_t i_userdata)
{
_try {
    _throwif(m3Err_moduleNotLinked, !io_module->runtime);

    സ്ഥിരം bool wildcardModule = (strcmp(i_moduleName, "*") == 0);

    result = m3Err_functionLookupFailed;

    വേണ്ടി (u32 i = 0; i < io_module->അക്കം_Functions; ++i) {
        സ്ഥിരം IM3Function f = &io_module->functions[i];

        എങ്കിൽ (f->import.moduleUtf8 and f->import.fieldUtf8) {
            എങ്കിൽ (strcmp(f->import.fieldUtf8, i_functionName) == 0 and
                (wildcardModule or strcmp(f->import.moduleUtf8, i_moduleName) == 0)) {
                എങ്കിൽ (i_signature) {
_                   (ValidateSignature(f, i_signature));
                }
_               (CompileRawFunction(io_module, f, i_function, i_userdata));
            }
        }
    }
} _catch:
    തിരികെ result;
}

M3Result m3_LinkRawFunctionEx (IM3Module         io_module,
                               സ്ഥിരം അക്ഷരം* സ്ഥിരം i_moduleName,
                               സ്ഥിരം അക്ഷരം* സ്ഥിരം i_functionName,
                               സ്ഥിരം അക്ഷരം* സ്ഥിരം i_signature,
                               M3RawCall         i_function,
                               സ്ഥിരം ശൂന്യം*       i_userdata)
{
    തിരികെ FindAndLinkFunction(io_module, i_moduleName, i_functionName, i_signature, (voidptr_t)i_function, i_userdata);
}

M3Result m3_LinkRawFunction (IM3Module         io_module,
                             സ്ഥിരം അക്ഷരം* സ്ഥിരം i_moduleName,
                             സ്ഥിരം അക്ഷരം* സ്ഥിരം i_functionName,
                             സ്ഥിരം അക്ഷരം* സ്ഥിരം i_signature,
                             M3RawCall         i_function)
{
    തിരികെ FindAndLinkFunction(io_module, i_moduleName, i_functionName, i_signature, (voidptr_t)i_function, ശൂന്യ);
}
