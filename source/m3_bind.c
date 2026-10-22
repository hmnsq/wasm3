//
//  m3_bind.c
//
//  Created by Steven Massey on 4/29/19.
//  Copyright © 2019 Steven Massey. All rights reserved.
//

#समावेश "m3_env.h"
#समावेश "m3_bind.h"
#समावेश "m3_exception.h"
#समावेश "m3_info.h"


u8 ConvertTypeCharToTypeId (अक्षर i_code)
{
    चयन (i_code) {
    हाल 'v': वापस c_m3Type_none;
    हाल 'i': वापस c_m3Type_i32;
    हाल 'I': वापस c_m3Type_i64;
    हाल 'f': वापस c_m3Type_f32;
    हाल 'F': वापस c_m3Type_f64;
    हाल '*': वापस c_m3Type_i32;
    // same letters c_waCompactTypes uses for the reference types
    हाल 'r': वापस c_m3Type_funcref;
    हाल 'R': वापस c_m3Type_externref;
    हाल 'c': वापस c_m3Type_contref;
    }
    वापस c_m3Type_unknown;
}


M3_परिणाम SignatureToFuncType (IM3FuncType* o_functionType, ccstr_t i_signature)
{
    IM3FuncType funcType = शून्य;

_try {
    अगर (not o_functionType) {
        _throw("null function type");
    }

    अगर (not i_signature) {
        _throw("null function signature");
    }

    cstr_t sig = i_signature;

    माप_प्रकार max_अंक_Types = म_माप(i_signature);

    // assume min signature is "()"
    _throwif(m3Err_malformedFunctionSignature, max_अंक_Types < 2);
    max_अंक_Types -= 2;

    _throwif(m3Err_tooManyArgsRets, max_अंक_Types > d_m3_Max_Sane_Function_Arg_Ret_गिनती);

_   (AllocFuncType(&funcType, (u32)max_अंक_Types));

    m3type_t* typelist = funcType->types;

    bool parsingRets = true;
    जबतक (*sig) {
        अक्षर typeChar = *sig++;

        अगर (typeChar == '(') {
            parsingRets = false;
            जारी;
        } अन्यथा अगर (typeChar == ' ') {
            जारी;
        } अन्यथा अगर (typeChar == ')') {
            अवरोध;
        }

        u8 type = ConvertTypeCharToTypeId(typeChar);

        _throwif("unknown argument type char", c_m3Type_unknown == type);

        अगर (type == c_m3Type_none) {
            जारी;
        }

        अगर (parsingRets) {
            _throwif("malformed signature; return count overflow", funcType->अंक_Rets >= max_अंक_Types);
            funcType->अंक_Rets++;
            *typelist++ = type;
        } अन्यथा {
            _throwif("malformed signature; arg count overflow", (u32)(funcType->अंक_Rets) + funcType->अंक_Args >= max_अंक_Types);
            funcType->अंक_Args++;
            *typelist++ = type;
        }
    }

} _catch:

    अगर (परिणाम) {
        m3_Free(funcType);
    }

    *o_functionType = funcType;

    वापस परिणाम;
}


अटल
M3_परिणाम ValidateSignature (IM3Function i_function, ccstr_t i_linkingSignature)
{
    M3_परिणाम परिणाम = m3Err_none;

    IM3FuncType ftype = शून्य;
_   (SignatureToFuncType(&ftype, i_linkingSignature));

    अगर (not AreFuncTypesEqual(ftype, i_function->funcType)) {
        m3log(module, "expected: %s", SPrintFuncTypeSignature(ftype));
        m3log(module, "   found: %s", SPrintFuncTypeSignature(i_function->funcType));

        _throw(m3Err_functionSignatureMismatch);
    }

    _catch:

    m3_Free(ftype);

    वापस परिणाम;
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

    स्थिर bool wildcardModule = (म_भेद(i_moduleName, "*") == 0);

    परिणाम = m3Err_functionLookupFailed;

    क्रम (u32 i = 0; i < io_module->अंक_Functions; ++i) {
        स्थिर IM3Function f = &io_module->functions[i];

        अगर (f->import.moduleUtf8 and f->import.fieldUtf8) {
            अगर (म_भेद(f->import.fieldUtf8, i_functionName) == 0 and
                (wildcardModule or म_भेद(f->import.moduleUtf8, i_moduleName) == 0)) {
                अगर (i_signature) {
_                   (ValidateSignature(f, i_signature));
                }
_               (CompileRawFunction(io_module, f, i_function, i_userdata));
            }
        }
    }
} _catch:
    वापस परिणाम;
}

M3_परिणाम m3_LinkRawFunctionEx (IM3Module         io_module,
                               स्थिर अक्षर* स्थिर i_moduleName,
                               स्थिर अक्षर* स्थिर i_functionName,
                               स्थिर अक्षर* स्थिर i_signature,
                               M3RawCall         i_function,
                               स्थिर व्योम*       i_userdata)
{
    वापस FindAndLinkFunction(io_module, i_moduleName, i_functionName, i_signature, (voidptr_t)i_function, i_userdata);
}

M3_परिणाम m3_LinkRawFunction (IM3Module         io_module,
                             स्थिर अक्षर* स्थिर i_moduleName,
                             स्थिर अक्षर* स्थिर i_functionName,
                             स्थिर अक्षर* स्थिर i_signature,
                             M3RawCall         i_function)
{
    वापस FindAndLinkFunction(io_module, i_moduleName, i_functionName, i_signature, (voidptr_t)i_function, शून्य);
}
