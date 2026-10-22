//
//  m3_bind.c
//
//  Created by Steven Massey on 4/29/19.
//  Copyright © 2019 Steven Massey. All rights reserved.
//

#شامل "m3_env.h"
#شامل "m3_bind.h"
#شامل "m3_exception.h"
#شامل "m3_info.h"


u8 ConvertTypeCharToTypeId (نویسه i_code)
{
    انتخاب (i_code) {
    حالت 'v': بازگشت c_m3Type_none;
    حالت 'i': بازگشت c_m3Type_i32;
    حالت 'I': بازگشت c_m3Type_i64;
    حالت 'f': بازگشت c_m3Type_f32;
    حالت 'F': بازگشت c_m3Type_f64;
    حالت '*': بازگشت c_m3Type_i32;
    // same letters c_waCompactTypes uses for the reference types
    حالت 'r': بازگشت c_m3Type_funcref;
    حالت 'R': بازگشت c_m3Type_externref;
    حالت 'c': بازگشت c_m3Type_contref;
    }
    بازگشت c_m3Type_unknown;
}


M3Result SignatureToFuncType (IM3FuncType* o_functionType, ccstr_t i_signature)
{
    IM3FuncType funcType = پوچ;

_try {
    اگر (not o_functionType) {
        _throw("null function type");
    }

    اگر (not i_signature) {
        _throw("null function signature");
    }

    cstr_t sig = i_signature;

    size_t max_رقم_Types = strlen(i_signature);

    // assume min signature is "()"
    _throwif(m3Err_malformedFunctionSignature, max_رقم_Types < 2);
    max_رقم_Types -= 2;

    _throwif(m3Err_tooManyArgsRets, max_رقم_Types > d_m3_Max_Sane_Function_Arg_Ret_شمار);

_   (AllocFuncType(&funcType, (u32)max_رقم_Types));

    m3type_t* typelist = funcType->types;

    bool parsingRets = true;
    وقتی (*sig) {
        نویسه typeChar = *sig++;

        اگر (typeChar == '(') {
            parsingRets = false;
            ادامه;
        } وگرنه اگر (typeChar == ' ') {
            ادامه;
        } وگرنه اگر (typeChar == ')') {
            بشکن;
        }

        u8 type = ConvertTypeCharToTypeId(typeChar);

        _throwif("unknown argument type char", c_m3Type_unknown == type);

        اگر (type == c_m3Type_none) {
            ادامه;
        }

        اگر (parsingRets) {
            _throwif("malformed signature; return count overflow", funcType->رقم_Rets >= max_رقم_Types);
            funcType->رقم_Rets++;
            *typelist++ = type;
        } وگرنه {
            _throwif("malformed signature; arg count overflow", (u32)(funcType->رقم_Rets) + funcType->رقم_Args >= max_رقم_Types);
            funcType->رقم_Args++;
            *typelist++ = type;
        }
    }

} _catch:

    اگر (result) {
        m3_Free(funcType);
    }

    *o_functionType = funcType;

    بازگشت result;
}


ایستا
M3Result ValidateSignature (IM3Function i_function, ccstr_t i_linkingSignature)
{
    M3Result result = m3Err_none;

    IM3FuncType ftype = پوچ;
_   (SignatureToFuncType(&ftype, i_linkingSignature));

    اگر (not AreFuncTypesEqual(ftype, i_function->funcType)) {
        m3log(module, "expected: %s", SPrintFuncTypeSignature(ftype));
        m3log(module, "   found: %s", SPrintFuncTypeSignature(i_function->funcType));

        _throw(m3Err_functionSignatureMismatch);
    }

    _catch:

    m3_Free(ftype);

    بازگشت result;
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

    ثابت bool wildcardModule = (strcmp(i_moduleName, "*") == 0);

    result = m3Err_functionLookupFailed;

    برای (u32 i = 0; i < io_module->رقم_Functions; ++i) {
        ثابت IM3Function f = &io_module->functions[i];

        اگر (f->import.moduleUtf8 and f->import.fieldUtf8) {
            اگر (strcmp(f->import.fieldUtf8, i_functionName) == 0 and
                (wildcardModule or strcmp(f->import.moduleUtf8, i_moduleName) == 0)) {
                اگر (i_signature) {
_                   (ValidateSignature(f, i_signature));
                }
_               (CompileRawFunction(io_module, f, i_function, i_userdata));
            }
        }
    }
} _catch:
    بازگشت result;
}

M3Result m3_LinkRawFunctionEx (IM3Module         io_module,
                               ثابت نویسه* ثابت i_moduleName,
                               ثابت نویسه* ثابت i_functionName,
                               ثابت نویسه* ثابت i_signature,
                               M3RawCall         i_function,
                               ثابت تهی*       i_userdata)
{
    بازگشت FindAndLinkFunction(io_module, i_moduleName, i_functionName, i_signature, (voidptr_t)i_function, i_userdata);
}

M3Result m3_LinkRawFunction (IM3Module         io_module,
                             ثابت نویسه* ثابت i_moduleName,
                             ثابت نویسه* ثابت i_functionName,
                             ثابت نویسه* ثابت i_signature,
                             M3RawCall         i_function)
{
    بازگشت FindAndLinkFunction(io_module, i_moduleName, i_functionName, i_signature, (voidptr_t)i_function, پوچ);
}
