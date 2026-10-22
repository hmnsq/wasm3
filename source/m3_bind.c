//
//  m3_bind.c
//
//  Created by Steven Massey on 4/29/19.
//  Copyright © 2019 Steven Massey. All rights reserved.
//

#ضمن "m3_env.h"
#ضمن "m3_bind.h"
#ضمن "m3_exception.h"
#ضمن "m3_info.h"


u8 Convert_Type_Char_منزل_Type_Id (حرف i_کوډ)
{
    بدل (i_کوډ) {
    حالة 'v': أرجع c_m3Type_none;
    حالة 'i': أرجع c_m3Type_i32;
    حالة 'I': أرجع c_m3Type_i64;
    حالة 'f': أرجع c_m3Type_f32;
    حالة 'F': أرجع c_m3Type_f64;
    حالة '*': أرجع c_m3Type_i32;
    // same letters c_waCompactTypes uses for the reference types
    حالة 'r': أرجع c_m3Type_funcref;
    حالة 'R': أرجع c_m3Type_externref;
    حالة 'c': أرجع c_m3Type_contref;
    }
    أرجع c_m3Type_unknown;
}


M3_پایله Signature_منزل_Func_Type (IM3FuncType* o_functionType, ccstr_t i_signature)
{
    IM3FuncType funcType = عدم;

_try {
    إذا (not o_functionType) {
        _throw("null function type");
    }

    إذا (not i_signature) {
        _throw("null function signature");
    }

    cstr_t sig = i_signature;

    size_t max_عدد_Types = مزي_اوږدوالی(i_signature);

    // assume min signature is "()"
    _throwif(m3Err_malformedFunctionSignature, max_عدد_Types < 2);
    max_عدد_Types -= 2;

    _throwif(m3_Err_too_Many_دلیلونه_Rets, max_عدد_Types > d_m3_Max_Sane_Function_Arg_Ret_شمېر);

_   (AllocFuncType(&funcType, (u32)max_عدد_Types));

    m3type_t* typelist = funcType->types;

    bool parsingRets = true;
    طالما (*sig) {
        حرف typeChar = *sig++;

        إذا (typeChar == '(') {
            parsingRets = false;
            استمر;
        } وإلا إذا (typeChar == ' ') {
            استمر;
        } وإلا إذا (typeChar == ')') {
            اكسر;
        }

        u8 type = Convert_Type_Char_منزل_Type_Id(typeChar);

        _throwif("unknown argument type char", c_m3Type_unknown == type);

        إذا (type == c_m3Type_none) {
            استمر;
        }

        إذا (parsingRets) {
            _throwif("malformed signature; return count overflow", funcType->عدد_Rets >= max_عدد_Types);
            funcType->عدد_Rets++;
            *typelist++ = type;
        } وإلا {
            _throwif("malformed signature; arg count overflow", (u32)(funcType->عدد_Rets) + funcType->عدد_دلیلونه >= max_عدد_Types);
            funcType->عدد_دلیلونه++;
            *typelist++ = type;
        }
    }

} _catch:

    إذا (پایله) {
        m3_Free(funcType);
    }

    *o_functionType = funcType;

    أرجع پایله;
}


ساكن
M3_پایله ValidateSignature (IM3Function i_function, ccstr_t i_linkingSignature)
{
    M3_پایله پایله = m3Err_none;

    IM3FuncType ftype = عدم;
_   (Signature_منزل_Func_Type(&ftype, i_linkingSignature));

    إذا (not AreFuncTypesEqual(ftype, i_function->funcType)) {
        m3log(module, "expected: %s", SPrintFuncTypeSignature(ftype));
        m3log(module, "   found: %s", SPrintFuncTypeSignature(i_function->funcType));

        _throw(m3Err_functionSignatureMismatch);
    }

    _catch:

    m3_Free(ftype);

    أرجع پایله;
}


M3_پایله پیدا_کړه_And_Link_Function (IM3Module io_module,
                              ccstr_t   i_moduleName,
                              ccstr_t   i_functionName,
                              ccstr_t   i_signature,
                              voidptr_t i_function,
                              voidptr_t i_userdata)
{
_try {
    _throwif(m3Err_moduleNotLinked, !io_module->runtime);

    ثابت bool wildcardModule = (strcmp(i_moduleName, "*") == 0);

    پایله = m3Err_functionLookupFailed;

    لكل (u32 i = 0; i < io_module->عدد_Functions; ++i) {
        ثابت IM3Function f = &io_module->functions[i];

        إذا (f->import.moduleUtf8 and f->import.ډګر_Utf8) {
            إذا (strcmp(f->import.ډګر_Utf8, i_functionName) == 0 and
                (wildcardModule or strcmp(f->import.moduleUtf8, i_moduleName) == 0)) {
                إذا (i_signature) {
_                   (ValidateSignature(f, i_signature));
                }
_               (CompileRawFunction(io_module, f, i_function, i_userdata));
            }
        }
    }
} _catch:
    أرجع پایله;
}

M3_پایله m3_LinkRawFunctionEx (IM3Module         io_module,
                               ثابت حرف* ثابت i_moduleName,
                               ثابت حرف* ثابت i_functionName,
                               ثابت حرف* ثابت i_signature,
                               M3RawCall         i_function,
                               ثابت فراغ*       i_userdata)
{
    أرجع پیدا_کړه_And_Link_Function(io_module, i_moduleName, i_functionName, i_signature, (voidptr_t)i_function, i_userdata);
}

M3_پایله m3_LinkRawFunction (IM3Module         io_module,
                             ثابت حرف* ثابت i_moduleName,
                             ثابت حرف* ثابت i_functionName,
                             ثابت حرف* ثابت i_signature,
                             M3RawCall         i_function)
{
    أرجع پیدا_کړه_And_Link_Function(io_module, i_moduleName, i_functionName, i_signature, (voidptr_t)i_function, عدم);
}
