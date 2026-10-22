//
//  m3_bind.c
//
//  Created by Steven Massey on 4/29/19.
//  Copyright © 2019 Steven Massey. All rights reserved.
//

#ਅਨ੍ਤਰ੍ਭਾਵ "m3_env.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "m3_bind.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "m3_exception.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "m3_info.h"


u8 Convert_Type_Char_ਮੰਜ਼ਿਲ_Type_Id (ਅੱਖਰ i_ਕੋਡ)
{
    ਚੋਣ (i_ਕੋਡ) {
    ਹਾਲਤ 'v': ਵਾਪਸ c_m3Type_none;
    ਹਾਲਤ 'i': ਵਾਪਸ c_m3Type_i32;
    ਹਾਲਤ 'I': ਵਾਪਸ c_m3Type_i64;
    ਹਾਲਤ 'f': ਵਾਪਸ c_m3Type_f32;
    ਹਾਲਤ 'F': ਵਾਪਸ c_m3Type_f64;
    ਹਾਲਤ '*': ਵਾਪਸ c_m3Type_i32;
    // same letters c_waCompactTypes uses for the reference types
    ਹਾਲਤ 'r': ਵਾਪਸ c_m3Type_funcref;
    ਹਾਲਤ 'R': ਵਾਪਸ c_m3Type_externref;
    ਹਾਲਤ 'c': ਵਾਪਸ c_m3Type_contref;
    }
    ਵਾਪਸ c_m3Type_unknown;
}


M3_ਨਤੀਜਾ Signature_ਮੰਜ਼ਿਲ_Func_Type (IM3FuncType* o_functionType, ccstr_t i_signature)
{
    IM3FuncType funcType = ਨਿਰਾ;

_try {
    ਜੇ (not o_functionType) {
        _throw("null function type");
    }

    ਜੇ (not i_signature) {
        _throw("null function signature");
    }

    cstr_t sig = i_signature;

    size_t max_ਅੰਕ_Types = ਲੜੀ_ਲੰਬਾਈ(i_signature);

    // assume min signature is "()"
    _throwif(m3Err_malformedFunctionSignature, max_ਅੰਕ_Types < 2);
    max_ਅੰਕ_Types -= 2;

    _throwif(m3_Err_too_Many_ਦਲੀਲਾਂ_Rets, max_ਅੰਕ_Types > d_m3_Max_Sane_Function_Arg_Ret_ਗਿਣਤੀ);

_   (AllocFuncType(&funcType, (u32)max_ਅੰਕ_Types));

    m3type_t* typelist = funcType->types;

    bool parsingRets = true;
    ਜਦੋਂ (*sig) {
        ਅੱਖਰ typeChar = *sig++;

        ਜੇ (typeChar == '(') {
            parsingRets = false;
            ਜਾਰੀ;
        } ਨਹੀਂਤਾਂ ਜੇ (typeChar == ' ') {
            ਜਾਰੀ;
        } ਨਹੀਂਤਾਂ ਜੇ (typeChar == ')') {
            ਰੋਕੋ;
        }

        u8 type = Convert_Type_Char_ਮੰਜ਼ਿਲ_Type_Id(typeChar);

        _throwif("unknown argument type char", c_m3Type_unknown == type);

        ਜੇ (type == c_m3Type_none) {
            ਜਾਰੀ;
        }

        ਜੇ (parsingRets) {
            _throwif("malformed signature; return count overflow", funcType->ਅੰਕ_Rets >= max_ਅੰਕ_Types);
            funcType->ਅੰਕ_Rets++;
            *typelist++ = type;
        } ਨਹੀਂਤਾਂ {
            _throwif("malformed signature; arg count overflow", (u32)(funcType->ਅੰਕ_Rets) + funcType->ਅੰਕ_ਦਲੀਲਾਂ >= max_ਅੰਕ_Types);
            funcType->ਅੰਕ_ਦਲੀਲਾਂ++;
            *typelist++ = type;
        }
    }

} _catch:

    ਜੇ (ਨਤੀਜਾ) {
        m3_Free(funcType);
    }

    *o_functionType = funcType;

    ਵਾਪਸ ਨਤੀਜਾ;
}


ਸਥਿਰ
M3_ਨਤੀਜਾ ValidateSignature (IM3Function i_function, ccstr_t i_linkingSignature)
{
    M3_ਨਤੀਜਾ ਨਤੀਜਾ = m3Err_none;

    IM3FuncType ftype = ਨਿਰਾ;
_   (Signature_ਮੰਜ਼ਿਲ_Func_Type(&ftype, i_linkingSignature));

    ਜੇ (not AreFuncTypesEqual(ftype, i_function->funcType)) {
        m3log(module, "expected: %s", SPrintFuncTypeSignature(ftype));
        m3log(module, "   found: %s", SPrintFuncTypeSignature(i_function->funcType));

        _throw(m3Err_functionSignatureMismatch);
    }

    _catch:

    m3_Free(ftype);

    ਵਾਪਸ ਨਤੀਜਾ;
}


M3_ਨਤੀਜਾ ਲੱਭੋ_And_Link_Function (IM3Module io_module,
                              ccstr_t   i_moduleName,
                              ccstr_t   i_functionName,
                              ccstr_t   i_signature,
                              voidptr_t i_function,
                              voidptr_t i_userdata)
{
_try {
    _throwif(m3Err_moduleNotLinked, !io_module->runtime);

    ਅਚਲ bool wildcardModule = (strcmp(i_moduleName, "*") == 0);

    ਨਤੀਜਾ = m3Err_functionLookupFailed;

    ਲਈ (u32 i = 0; i < io_module->ਅੰਕ_Functions; ++i) {
        ਅਚਲ IM3Function f = &io_module->functions[i];

        ਜੇ (f->import.moduleUtf8 and f->import.ਖੇਤਰ_Utf8) {
            ਜੇ (strcmp(f->import.ਖੇਤਰ_Utf8, i_functionName) == 0 and
                (wildcardModule or strcmp(f->import.moduleUtf8, i_moduleName) == 0)) {
                ਜੇ (i_signature) {
_                   (ValidateSignature(f, i_signature));
                }
_               (CompileRawFunction(io_module, f, i_function, i_userdata));
            }
        }
    }
} _catch:
    ਵਾਪਸ ਨਤੀਜਾ;
}

M3_ਨਤੀਜਾ m3_LinkRawFunctionEx (IM3Module         io_module,
                               ਅਚਲ ਅੱਖਰ* ਅਚਲ i_moduleName,
                               ਅਚਲ ਅੱਖਰ* ਅਚਲ i_functionName,
                               ਅਚਲ ਅੱਖਰ* ਅਚਲ i_signature,
                               M3RawCall         i_function,
                               ਅਚਲ ਖਾਲੀ*       i_userdata)
{
    ਵਾਪਸ ਲੱਭੋ_And_Link_Function(io_module, i_moduleName, i_functionName, i_signature, (voidptr_t)i_function, i_userdata);
}

M3_ਨਤੀਜਾ m3_LinkRawFunction (IM3Module         io_module,
                             ਅਚਲ ਅੱਖਰ* ਅਚਲ i_moduleName,
                             ਅਚਲ ਅੱਖਰ* ਅਚਲ i_functionName,
                             ਅਚਲ ਅੱਖਰ* ਅਚਲ i_signature,
                             M3RawCall         i_function)
{
    ਵਾਪਸ ਲੱਭੋ_And_Link_Function(io_module, i_moduleName, i_functionName, i_signature, (voidptr_t)i_function, ਨਿਰਾ);
}
