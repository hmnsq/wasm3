//
//  m3_bind.c
//
//  Created by Steven Massey on 4/29/19.
//  Copyright © 2019 Steven Massey. All rights reserved.
//

#अन्तर्भाव "m3_env.h"
#अन्तर्भाव "m3_bind.h"
#अन्तर्भाव "m3_exception.h"
#अन्तर्भाव "m3_info.h"


u8 ConvertTypeCharToTypeId (वर्ण i_code)
{
    विकल्प (i_code) {
    पक्ष 'v': पटिनिवत्त c_m3Type_none;
    पक्ष 'i': पटिनिवत्त c_m3Type_i32;
    पक्ष 'I': पटिनिवत्त c_m3Type_i64;
    पक्ष 'f': पटिनिवत्त c_m3Type_f32;
    पक्ष 'F': पटिनिवत्त c_m3Type_f64;
    पक्ष '*': पटिनिवत्त c_m3Type_i32;
    // same letters c_waCompactTypes uses for the reference types
    पक्ष 'r': पटिनिवत्त c_m3Type_funcref;
    पक्ष 'R': पटिनिवत्त c_m3Type_externref;
    पक्ष 'c': पटिनिवत्त c_m3Type_contref;
    }
    पटिनिवत्त c_m3Type_unknown;
}


M3Result SignatureToFuncType (IM3FuncType* o_functionType, ccstr_t i_signature)
{
    IM3FuncType funcType = रिक्त;

_try {
    सचे (not o_functionType) {
        _throw("null function type");
    }

    सचे (not i_signature) {
        _throw("null function signature");
    }

    cstr_t sig = i_signature;

    size_t max_अङ्कः_Types = strlen(i_signature);

    // assume min signature is "()"
    _throwif(m3Err_malformedFunctionSignature, max_अङ्कः_Types < 2);
    max_अङ्कः_Types -= 2;

    _throwif(m3Err_tooManyArgsRets, max_अङ्कः_Types > d_m3_Max_Sane_Function_Arg_Ret_गणनम्);

_   (AllocFuncType(&funcType, (u32)max_अङ्कः_Types));

    m3type_t* typelist = funcType->types;

    bool parsingRets = true;
    याव (*sig) {
        वर्ण typeChar = *sig++;

        सचे (typeChar == '(') {
            parsingRets = false;
            अनुवर्त;
        } अञ्ञथा सचे (typeChar == ' ') {
            अनुवर्त;
        } अञ्ञथा सचे (typeChar == ')') {
            भङ्ग;
        }

        u8 type = ConvertTypeCharToTypeId(typeChar);

        _throwif("unknown argument type char", c_m3Type_unknown == type);

        सचे (type == c_m3Type_none) {
            अनुवर्त;
        }

        सचे (parsingRets) {
            _throwif("malformed signature; return count overflow", funcType->अङ्कः_Rets >= max_अङ्कः_Types);
            funcType->अङ्कः_Rets++;
            *typelist++ = type;
        } अञ्ञथा {
            _throwif("malformed signature; arg count overflow", (u32)(funcType->अङ्कः_Rets) + funcType->अङ्कः_Args >= max_अङ्कः_Types);
            funcType->अङ्कः_Args++;
            *typelist++ = type;
        }
    }

} _catch:

    सचे (result) {
        m3_Free(funcType);
    }

    *o_functionType = funcType;

    पटिनिवत्त result;
}


स्थिर
M3Result ValidateSignature (IM3Function i_function, ccstr_t i_linkingSignature)
{
    M3Result result = m3Err_none;

    IM3FuncType ftype = रिक्त;
_   (SignatureToFuncType(&ftype, i_linkingSignature));

    सचे (not AreFuncTypesEqual(ftype, i_function->funcType)) {
        m3log(module, "expected: %s", SPrintFuncTypeSignature(ftype));
        m3log(module, "   found: %s", SPrintFuncTypeSignature(i_function->funcType));

        _throw(m3Err_functionSignatureMismatch);
    }

    _catch:

    m3_Free(ftype);

    पटिनिवत्त result;
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

    ध्रुव bool wildcardModule = (strcmp(i_moduleName, "*") == 0);

    result = m3Err_functionLookupFailed;

    अत्थाय (u32 i = 0; i < io_module->अङ्कः_Functions; ++i) {
        ध्रुव IM3Function f = &io_module->functions[i];

        सचे (f->import.moduleUtf8 and f->import.fieldUtf8) {
            सचे (strcmp(f->import.fieldUtf8, i_functionName) == 0 and
                (wildcardModule or strcmp(f->import.moduleUtf8, i_moduleName) == 0)) {
                सचे (i_signature) {
_                   (ValidateSignature(f, i_signature));
                }
_               (CompileRawFunction(io_module, f, i_function, i_userdata));
            }
        }
    }
} _catch:
    पटिनिवत्त result;
}

M3Result m3_LinkRawFunctionEx (IM3Module         io_module,
                               ध्रुव वर्ण* ध्रुव i_moduleName,
                               ध्रुव वर्ण* ध्रुव i_functionName,
                               ध्रुव वर्ण* ध्रुव i_signature,
                               M3RawCall         i_function,
                               ध्रुव शून्य*       i_userdata)
{
    पटिनिवत्त FindAndLinkFunction(io_module, i_moduleName, i_functionName, i_signature, (voidptr_t)i_function, i_userdata);
}

M3Result m3_LinkRawFunction (IM3Module         io_module,
                             ध्रुव वर्ण* ध्रुव i_moduleName,
                             ध्रुव वर्ण* ध्रुव i_functionName,
                             ध्रुव वर्ण* ध्रुव i_signature,
                             M3RawCall         i_function)
{
    पटिनिवत्त FindAndLinkFunction(io_module, i_moduleName, i_functionName, i_signature, (voidptr_t)i_function, रिक्त);
}
