//
//  m3_bind.c
//
//  Created by Steven Massey on 4/29/19.
//  Copyright © 2019 Steven Massey. All rights reserved.
//

#ಅನ್ತರ್ಭಾವ "m3_env.h"
#ಅನ್ತರ್ಭಾವ "m3_bind.h"
#ಅನ್ತರ್ಭಾವ "m3_exception.h"
#ಅನ್ತರ್ಭಾವ "m3_info.h"


u8 ConvertTypeCharToTypeId (ವರ್ಣ i_code)
{
    ವಿಕಲ್ಪ (i_code) {
    ಪಕ್ಷ 'v': ಪ್ರತ್ಯಾಗಮ c_m3Type_none;
    ಪಕ್ಷ 'i': ಪ್ರತ್ಯಾಗಮ c_m3Type_i32;
    ಪಕ್ಷ 'I': ಪ್ರತ್ಯಾಗಮ c_m3Type_i64;
    ಪಕ್ಷ 'f': ಪ್ರತ್ಯಾಗಮ c_m3Type_f32;
    ಪಕ್ಷ 'F': ಪ್ರತ್ಯಾಗಮ c_m3Type_f64;
    ಪಕ್ಷ '*': ಪ್ರತ್ಯಾಗಮ c_m3Type_i32;
    // same letters c_waCompactTypes uses for the reference types
    ಪಕ್ಷ 'r': ಪ್ರತ್ಯಾಗಮ c_m3Type_funcref;
    ಪಕ್ಷ 'R': ಪ್ರತ್ಯಾಗಮ c_m3Type_externref;
    ಪಕ್ಷ 'c': ಪ್ರತ್ಯಾಗಮ c_m3Type_contref;
    }
    ಪ್ರತ್ಯಾಗಮ c_m3Type_unknown;
}


M3Result SignatureToFuncType (IM3FuncType* o_functionType, ccstr_t i_signature)
{
    IM3FuncType funcType = ರಿಕ್ತ;

_try {
    ಯದಿ (not o_functionType) {
        _throw("null function type");
    }

    ಯದಿ (not i_signature) {
        _throw("null function signature");
    }

    cstr_t sig = i_signature;

    size_t max_ಅಂಕೆ_Types = strlen(i_signature);

    // assume min signature is "()"
    _throwif(m3Err_malformedFunctionSignature, max_ಅಂಕೆ_Types < 2);
    max_ಅಂಕೆ_Types -= 2;

    _throwif(m3Err_tooManyArgsRets, max_ಅಂಕೆ_Types > d_m3_Max_Sane_Function_Arg_Ret_ಎಣಿಕೆ);

_   (AllocFuncType(&funcType, (u32)max_ಅಂಕೆ_Types));

    m3type_t* typelist = funcType->types;

    bool parsingRets = true;
    ಯಾವತ್ (*sig) {
        ವರ್ಣ typeChar = *sig++;

        ಯದಿ (typeChar == '(') {
            parsingRets = false;
            ಅನುವರ್ತ;
        } ಅನ್ಯಥಾ ಯದಿ (typeChar == ' ') {
            ಅನುವರ್ತ;
        } ಅನ್ಯಥಾ ಯದಿ (typeChar == ')') {
            ಭಙ್ಗ;
        }

        u8 type = ConvertTypeCharToTypeId(typeChar);

        _throwif("unknown argument type char", c_m3Type_unknown == type);

        ಯದಿ (type == c_m3Type_none) {
            ಅನುವರ್ತ;
        }

        ಯದಿ (parsingRets) {
            _throwif("malformed signature; return count overflow", funcType->ಅಂಕೆ_Rets >= max_ಅಂಕೆ_Types);
            funcType->ಅಂಕೆ_Rets++;
            *typelist++ = type;
        } ಅನ್ಯಥಾ {
            _throwif("malformed signature; arg count overflow", (u32)(funcType->ಅಂಕೆ_Rets) + funcType->ಅಂಕೆ_Args >= max_ಅಂಕೆ_Types);
            funcType->ಅಂಕೆ_Args++;
            *typelist++ = type;
        }
    }

} _catch:

    ಯದಿ (result) {
        m3_Free(funcType);
    }

    *o_functionType = funcType;

    ಪ್ರತ್ಯಾಗಮ result;
}


ಸ್ಥಿರ
M3Result ValidateSignature (IM3Function i_function, ccstr_t i_linkingSignature)
{
    M3Result result = m3Err_none;

    IM3FuncType ftype = ರಿಕ್ತ;
_   (SignatureToFuncType(&ftype, i_linkingSignature));

    ಯದಿ (not AreFuncTypesEqual(ftype, i_function->funcType)) {
        m3log(module, "expected: %s", SPrintFuncTypeSignature(ftype));
        m3log(module, "   found: %s", SPrintFuncTypeSignature(i_function->funcType));

        _throw(m3Err_functionSignatureMismatch);
    }

    _catch:

    m3_Free(ftype);

    ಪ್ರತ್ಯಾಗಮ result;
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

    ಧ್ರುವ bool wildcardModule = (strcmp(i_moduleName, "*") == 0);

    result = m3Err_functionLookupFailed;

    ಪ್ರತಿ (u32 i = 0; i < io_module->ಅಂಕೆ_Functions; ++i) {
        ಧ್ರುವ IM3Function f = &io_module->functions[i];

        ಯದಿ (f->import.moduleUtf8 and f->import.fieldUtf8) {
            ಯದಿ (strcmp(f->import.fieldUtf8, i_functionName) == 0 and
                (wildcardModule or strcmp(f->import.moduleUtf8, i_moduleName) == 0)) {
                ಯದಿ (i_signature) {
_                   (ValidateSignature(f, i_signature));
                }
_               (CompileRawFunction(io_module, f, i_function, i_userdata));
            }
        }
    }
} _catch:
    ಪ್ರತ್ಯಾಗಮ result;
}

M3Result m3_LinkRawFunctionEx (IM3Module         io_module,
                               ಧ್ರುವ ವರ್ಣ* ಧ್ರುವ i_moduleName,
                               ಧ್ರುವ ವರ್ಣ* ಧ್ರುವ i_functionName,
                               ಧ್ರುವ ವರ್ಣ* ಧ್ರುವ i_signature,
                               M3RawCall         i_function,
                               ಧ್ರುವ ಶೂನ್ಯ*       i_userdata)
{
    ಪ್ರತ್ಯಾಗಮ FindAndLinkFunction(io_module, i_moduleName, i_functionName, i_signature, (voidptr_t)i_function, i_userdata);
}

M3Result m3_LinkRawFunction (IM3Module         io_module,
                             ಧ್ರುವ ವರ್ಣ* ಧ್ರುವ i_moduleName,
                             ಧ್ರುವ ವರ್ಣ* ಧ್ರುವ i_functionName,
                             ಧ್ರುವ ವರ್ಣ* ಧ್ರುವ i_signature,
                             M3RawCall         i_function)
{
    ಪ್ರತ್ಯಾಗಮ FindAndLinkFunction(io_module, i_moduleName, i_functionName, i_signature, (voidptr_t)i_function, ರಿಕ್ತ);
}
