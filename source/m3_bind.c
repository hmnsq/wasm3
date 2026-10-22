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


u8 ConvertTypeCharToTypeId (अक्षर i_code)
{
    निवड (i_code) {
    स्थिती 'v': परत c_m3Type_none;
    स्थिती 'i': परत c_m3Type_i32;
    स्थिती 'I': परत c_m3Type_i64;
    स्थिती 'f': परत c_m3Type_f32;
    स्थिती 'F': परत c_m3Type_f64;
    स्थिती '*': परत c_m3Type_i32;
    // same letters c_waCompactTypes uses for the reference types
    स्थिती 'r': परत c_m3Type_funcref;
    स्थिती 'R': परत c_m3Type_externref;
    स्थिती 'c': परत c_m3Type_contref;
    }
    परत c_m3Type_unknown;
}


M3Result SignatureToFuncType (IM3FuncType* o_functionType, ccstr_t i_signature)
{
    IM3FuncType funcType = रिक्त;

_try {
    जर (not o_functionType) {
        _throw("null function type");
    }

    जर (not i_signature) {
        _throw("null function signature");
    }

    cstr_t sig = i_signature;

    माप_प्रकार max_अंक_Types = म_माप(i_signature);

    // assume min signature is "()"
    _throwif(m3Err_malformedFunctionSignature, max_अंक_Types < 2);
    max_अंक_Types -= 2;

    _throwif(m3Err_tooManyArgsRets, max_अंक_Types > d_m3_Max_Sane_Function_Arg_Ret_मोजणी);

_   (AllocFuncType(&funcType, (u32)max_अंक_Types));

    m3type_t* typelist = funcType->types;

    bool parsingRets = true;
    पर्यंत (*sig) {
        अक्षर typeChar = *sig++;

        जर (typeChar == '(') {
            parsingRets = false;
            सुरूठेवा;
        } अन्यथा जर (typeChar == ' ') {
            सुरूठेवा;
        } अन्यथा जर (typeChar == ')') {
            थांबा;
        }

        u8 type = ConvertTypeCharToTypeId(typeChar);

        _throwif("unknown argument type char", c_m3Type_unknown == type);

        जर (type == c_m3Type_none) {
            सुरूठेवा;
        }

        जर (parsingRets) {
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

    जर (result) {
        m3_Free(funcType);
    }

    *o_functionType = funcType;

    परत result;
}


स्थिर
M3Result ValidateSignature (IM3Function i_function, ccstr_t i_linkingSignature)
{
    M3Result result = m3Err_none;

    IM3FuncType ftype = रिक्त;
_   (SignatureToFuncType(&ftype, i_linkingSignature));

    जर (not AreFuncTypesEqual(ftype, i_function->funcType)) {
        m3log(module, "expected: %s", SPrintFuncTypeSignature(ftype));
        m3log(module, "   found: %s", SPrintFuncTypeSignature(i_function->funcType));

        _throw(m3Err_functionSignatureMismatch);
    }

    _catch:

    m3_Free(ftype);

    परत result;
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

    स्थिरांक bool wildcardModule = (म_भेद(i_moduleName, "*") == 0);

    result = m3Err_functionLookupFailed;

    साठी (u32 i = 0; i < io_module->अंक_Functions; ++i) {
        स्थिरांक IM3Function f = &io_module->functions[i];

        जर (f->import.moduleUtf8 and f->import.fieldUtf8) {
            जर (म_भेद(f->import.fieldUtf8, i_functionName) == 0 and
                (wildcardModule or म_भेद(f->import.moduleUtf8, i_moduleName) == 0)) {
                जर (i_signature) {
_                   (ValidateSignature(f, i_signature));
                }
_               (CompileRawFunction(io_module, f, i_function, i_userdata));
            }
        }
    }
} _catch:
    परत result;
}

M3Result m3_LinkRawFunctionEx (IM3Module         io_module,
                               स्थिरांक अक्षर* स्थिरांक i_moduleName,
                               स्थिरांक अक्षर* स्थिरांक i_functionName,
                               स्थिरांक अक्षर* स्थिरांक i_signature,
                               M3RawCall         i_function,
                               स्थिरांक शून्य*       i_userdata)
{
    परत FindAndLinkFunction(io_module, i_moduleName, i_functionName, i_signature, (voidptr_t)i_function, i_userdata);
}

M3Result m3_LinkRawFunction (IM3Module         io_module,
                             स्थिरांक अक्षर* स्थिरांक i_moduleName,
                             स्थिरांक अक्षर* स्थिरांक i_functionName,
                             स्थिरांक अक्षर* स्थिरांक i_signature,
                             M3RawCall         i_function)
{
    परत FindAndLinkFunction(io_module, i_moduleName, i_functionName, i_signature, (voidptr_t)i_function, रिक्त);
}
