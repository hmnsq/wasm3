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


u8 Convert_Type_Char_गन्तव्यम्_Type_Id (वर्ण i_संहिता)
{
    विकल्प (i_संहिता) {
    पक्ष 'v': प्रत्यागम c_m3Type_none;
    पक्ष 'i': प्रत्यागम c_m3Type_i32;
    पक्ष 'I': प्रत्यागम c_m3Type_i64;
    पक्ष 'f': प्रत्यागम c_m3Type_f32;
    पक्ष 'F': प्रत्यागम c_m3Type_f64;
    पक्ष '*': प्रत्यागम c_m3Type_i32;
    // same letters c_waCompactTypes uses for the reference types
    पक्ष 'r': प्रत्यागम c_m3Type_funcref;
    पक्ष 'R': प्रत्यागम c_m3Type_externref;
    पक्ष 'c': प्रत्यागम c_m3Type_contref;
    }
    प्रत्यागम c_m3Type_unknown;
}


M3_परिणाम Signature_गन्तव्यम्_Func_Type (IM3FuncType* o_functionType, ccstr_t i_signature)
{
    IM3FuncType funcType = रिक्त;

_try {
    यदि (not o_functionType) {
        _throw("null function type");
    }

    यदि (not i_signature) {
        _throw("null function signature");
    }

    cstr_t sig = i_signature;

    size_t max_अङ्कः_Types = वाक्यदैर्घ्यम्(i_signature);

    // assume min signature is "()"
    _throwif(m3Err_malformedFunctionSignature, max_अङ्कः_Types < 2);
    max_अङ्कः_Types -= 2;

    _throwif(m3_Err_too_Many_तर्काः_Rets, max_अङ्कः_Types > d_m3_Max_Sane_Function_Arg_Ret_गणनम्);

_   (AllocFuncType(&funcType, (u32)max_अङ्कः_Types));

    m3type_t* typelist = funcType->types;

    bool parsingRets = true;
    यावत् (*sig) {
        वर्ण typeChar = *sig++;

        यदि (typeChar == '(') {
            parsingRets = false;
            अनुवर्त;
        } अन्यथा यदि (typeChar == ' ') {
            अनुवर्त;
        } अन्यथा यदि (typeChar == ')') {
            भङ्ग;
        }

        u8 type = Convert_Type_Char_गन्तव्यम्_Type_Id(typeChar);

        _throwif("unknown argument type char", c_m3Type_unknown == type);

        यदि (type == c_m3Type_none) {
            अनुवर्त;
        }

        यदि (parsingRets) {
            _throwif("malformed signature; return count overflow", funcType->अङ्कः_Rets >= max_अङ्कः_Types);
            funcType->अङ्कः_Rets++;
            *typelist++ = type;
        } अन्यथा {
            _throwif("malformed signature; arg count overflow", (u32)(funcType->अङ्कः_Rets) + funcType->अङ्कः_तर्काः >= max_अङ्कः_Types);
            funcType->अङ्कः_तर्काः++;
            *typelist++ = type;
        }
    }

} _catch:

    यदि (परिणाम) {
        m3_Free(funcType);
    }

    *o_functionType = funcType;

    प्रत्यागम परिणाम;
}


स्थिर
M3_परिणाम ValidateSignature (IM3Function i_function, ccstr_t i_linkingSignature)
{
    M3_परिणाम परिणाम = m3Err_none;

    IM3FuncType ftype = रिक्त;
_   (Signature_गन्तव्यम्_Func_Type(&ftype, i_linkingSignature));

    यदि (not AreFuncTypesEqual(ftype, i_function->funcType)) {
        m3log(module, "expected: %s", SPrintFuncTypeSignature(ftype));
        m3log(module, "   found: %s", SPrintFuncTypeSignature(i_function->funcType));

        _throw(m3Err_functionSignatureMismatch);
    }

    _catch:

    m3_Free(ftype);

    प्रत्यागम परिणाम;
}


M3_परिणाम FindAndLinkFunction (IM3Module io_module,
                              ccstr_t   i_moduleName,
                              ccstr_t   i_functionName,
                              ccstr_t   i_signature,
                              voidptr_t i_function,
                              voidptr_t i_userdata)
{
_try {
    _throwif(m3Err_moduleNotLinked, !io_module->runtime);

    ध्रुव bool wildcardModule = (वाक्यं_तुलय(i_moduleName, "*") == 0);

    परिणाम = m3Err_functionLookupFailed;

    प्रति (u32 i = 0; i < io_module->अङ्कः_Functions; ++i) {
        ध्रुव IM3Function f = &io_module->functions[i];

        यदि (f->import.moduleUtf8 and f->import.क्षेत्रम्_Utf8) {
            यदि (वाक्यं_तुलय(f->import.क्षेत्रम्_Utf8, i_functionName) == 0 and
                (wildcardModule or वाक्यं_तुलय(f->import.moduleUtf8, i_moduleName) == 0)) {
                यदि (i_signature) {
_                   (ValidateSignature(f, i_signature));
                }
_               (CompileRawFunction(io_module, f, i_function, i_userdata));
            }
        }
    }
} _catch:
    प्रत्यागम परिणाम;
}

M3_परिणाम m3_LinkRawFunctionEx (IM3Module         io_module,
                               ध्रुव वर्ण* ध्रुव i_moduleName,
                               ध्रुव वर्ण* ध्रुव i_functionName,
                               ध्रुव वर्ण* ध्रुव i_signature,
                               M3RawCall         i_function,
                               ध्रुव शून्य*       i_userdata)
{
    प्रत्यागम FindAndLinkFunction(io_module, i_moduleName, i_functionName, i_signature, (voidptr_t)i_function, i_userdata);
}

M3_परिणाम m3_LinkRawFunction (IM3Module         io_module,
                             ध्रुव वर्ण* ध्रुव i_moduleName,
                             ध्रुव वर्ण* ध्रुव i_functionName,
                             ध्रुव वर्ण* ध्रुव i_signature,
                             M3RawCall         i_function)
{
    प्रत्यागम FindAndLinkFunction(io_module, i_moduleName, i_functionName, i_signature, (voidptr_t)i_function, रिक्त);
}
