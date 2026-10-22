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
    पक्ष 'v': फर्काऊ c_m3Type_none;
    पक्ष 'i': फर्काऊ c_m3Type_i32;
    पक्ष 'I': फर्काऊ c_m3Type_i64;
    पक्ष 'f': फर्काऊ c_m3Type_f32;
    पक्ष 'F': फर्काऊ c_m3Type_f64;
    पक्ष '*': फर्काऊ c_m3Type_i32;
    // same letters c_waCompactTypes uses for the reference types
    पक्ष 'r': फर्काऊ c_m3Type_funcref;
    पक्ष 'R': फर्काऊ c_m3Type_externref;
    पक्ष 'c': फर्काऊ c_m3Type_contref;
    }
    फर्काऊ c_m3Type_unknown;
}


M3Result SignatureToFuncType (IM3FuncType* o_functionType, ccstr_t i_signature)
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

    माप_प्रकार max_अङ्क_Types = म_माप(i_signature);

    // assume min signature is "()"
    _throwif(m3Err_malformedFunctionSignature, max_अङ्क_Types < 2);
    max_अङ्क_Types -= 2;

    _throwif(m3Err_tooManyArgsRets, max_अङ्क_Types > d_m3_Max_Sane_Function_Arg_Ret_गन्ती);

_   (AllocFuncType(&funcType, (u32)max_अङ्क_Types));

    m3type_t* typelist = funcType->types;

    bool parsingRets = true;
    जबसम्म (*sig) {
        वर्ण typeChar = *sig++;

        यदि (typeChar == '(') {
            parsingRets = false;
            जारी;
        } अन्यथा यदि (typeChar == ' ') {
            जारी;
        } अन्यथा यदि (typeChar == ')') {
            रोक;
        }

        u8 type = ConvertTypeCharToTypeId(typeChar);

        _throwif("unknown argument type char", c_m3Type_unknown == type);

        यदि (type == c_m3Type_none) {
            जारी;
        }

        यदि (parsingRets) {
            _throwif("malformed signature; return count overflow", funcType->अङ्क_Rets >= max_अङ्क_Types);
            funcType->अङ्क_Rets++;
            *typelist++ = type;
        } अन्यथा {
            _throwif("malformed signature; arg count overflow", (u32)(funcType->अङ्क_Rets) + funcType->अङ्क_Args >= max_अङ्क_Types);
            funcType->अङ्क_Args++;
            *typelist++ = type;
        }
    }

} _catch:

    यदि (result) {
        m3_Free(funcType);
    }

    *o_functionType = funcType;

    फर्काऊ result;
}


स्थिर
M3Result ValidateSignature (IM3Function i_function, ccstr_t i_linkingSignature)
{
    M3Result result = m3Err_none;

    IM3FuncType ftype = रिक्त;
_   (SignatureToFuncType(&ftype, i_linkingSignature));

    यदि (not AreFuncTypesEqual(ftype, i_function->funcType)) {
        m3log(module, "expected: %s", SPrintFuncTypeSignature(ftype));
        m3log(module, "   found: %s", SPrintFuncTypeSignature(i_function->funcType));

        _throw(m3Err_functionSignatureMismatch);
    }

    _catch:

    m3_Free(ftype);

    फर्काऊ result;
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

    अचर bool wildcardModule = (म_भेद(i_moduleName, "*") == 0);

    result = m3Err_functionLookupFailed;

    लागि (u32 i = 0; i < io_module->अङ्क_Functions; ++i) {
        अचर IM3Function f = &io_module->functions[i];

        यदि (f->import.moduleUtf8 and f->import.fieldUtf8) {
            यदि (म_भेद(f->import.fieldUtf8, i_functionName) == 0 and
                (wildcardModule or म_भेद(f->import.moduleUtf8, i_moduleName) == 0)) {
                यदि (i_signature) {
_                   (ValidateSignature(f, i_signature));
                }
_               (CompileRawFunction(io_module, f, i_function, i_userdata));
            }
        }
    }
} _catch:
    फर्काऊ result;
}

M3Result m3_LinkRawFunctionEx (IM3Module         io_module,
                               अचर वर्ण* अचर i_moduleName,
                               अचर वर्ण* अचर i_functionName,
                               अचर वर्ण* अचर i_signature,
                               M3RawCall         i_function,
                               अचर शून्य*       i_userdata)
{
    फर्काऊ FindAndLinkFunction(io_module, i_moduleName, i_functionName, i_signature, (voidptr_t)i_function, i_userdata);
}

M3Result m3_LinkRawFunction (IM3Module         io_module,
                             अचर वर्ण* अचर i_moduleName,
                             अचर वर्ण* अचर i_functionName,
                             अचर वर्ण* अचर i_signature,
                             M3RawCall         i_function)
{
    फर्काऊ FindAndLinkFunction(io_module, i_moduleName, i_functionName, i_signature, (voidptr_t)i_function, रिक्त);
}
