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


u8 Convert_Type_Char_مقصد_Type_Id (حرف i_رمز)
{
    انتخاب (i_رمز) {
    حالت 'v': واپس c_m3Type_none;
    حالت 'i': واپس c_m3Type_i32;
    حالت 'I': واپس c_m3Type_i64;
    حالت 'f': واپس c_m3Type_f32;
    حالت 'F': واپس c_m3Type_f64;
    حالت '*': واپس c_m3Type_i32;
    // same letters c_waCompactTypes uses for the reference types
    حالت 'r': واپس c_m3Type_funcref;
    حالت 'R': واپس c_m3Type_externref;
    حالت 'c': واپس c_m3Type_contref;
    }
    واپس c_m3Type_unknown;
}


M3_نتیجہ Signature_مقصد_Func_Type (IM3FuncType* o_functionType, ccstr_t i_signature)
{
    IM3FuncType funcType = تہی;

_try {
    اگر (not o_functionType) {
        _throw("null function type");
    }

    اگر (not i_signature) {
        _throw("null function signature");
    }

    cstr_t sig = i_signature;

    size_t max_عدد_Types = لڑی_لمبائی(i_signature);

    // assume min signature is "()"
    _throwif(m3Err_malformedFunctionSignature, max_عدد_Types < 2);
    max_عدد_Types -= 2;

    _throwif(m3_Err_too_Many_دلائل_Rets, max_عدد_Types > d_m3_Max_Sane_Function_Arg_Ret_گنتی);

_   (AllocFuncType(&funcType, (u32)max_عدد_Types));

    m3type_t* typelist = funcType->types;

    bool parsingRets = true;
    جب (*sig) {
        حرف typeChar = *sig++;

        اگر (typeChar == '(') {
            parsingRets = false;
            جاری;
        } ورنہ اگر (typeChar == ' ') {
            جاری;
        } ورنہ اگر (typeChar == ')') {
            توڑو;
        }

        u8 type = Convert_Type_Char_مقصد_Type_Id(typeChar);

        _throwif("unknown argument type char", c_m3Type_unknown == type);

        اگر (type == c_m3Type_none) {
            جاری;
        }

        اگر (parsingRets) {
            _throwif("malformed signature; return count overflow", funcType->عدد_Rets >= max_عدد_Types);
            funcType->عدد_Rets++;
            *typelist++ = type;
        } ورنہ {
            _throwif("malformed signature; arg count overflow", (u32)(funcType->عدد_Rets) + funcType->عدد_دلائل >= max_عدد_Types);
            funcType->عدد_دلائل++;
            *typelist++ = type;
        }
    }

} _catch:

    اگر (نتیجہ) {
        m3_Free(funcType);
    }

    *o_functionType = funcType;

    واپس نتیجہ;
}


ساکن
M3_نتیجہ ValidateSignature (IM3Function i_function, ccstr_t i_linkingSignature)
{
    M3_نتیجہ نتیجہ = m3Err_none;

    IM3FuncType ftype = تہی;
_   (Signature_مقصد_Func_Type(&ftype, i_linkingSignature));

    اگر (not AreFuncTypesEqual(ftype, i_function->funcType)) {
        m3log(module, "expected: %s", SPrintFuncTypeSignature(ftype));
        m3log(module, "   found: %s", SPrintFuncTypeSignature(i_function->funcType));

        _throw(m3Err_functionSignatureMismatch);
    }

    _catch:

    m3_Free(ftype);

    واپس نتیجہ;
}


M3_نتیجہ ڈھونڈو_And_Link_Function (IM3Module io_module,
                              ccstr_t   i_moduleName,
                              ccstr_t   i_functionName,
                              ccstr_t   i_signature,
                              voidptr_t i_function,
                              voidptr_t i_userdata)
{
_try {
    _throwif(m3Err_moduleNotLinked, !io_module->runtime);

    ثابت bool wildcardModule = (strcmp(i_moduleName, "*") == 0);

    نتیجہ = m3Err_functionLookupFailed;

    برائے (u32 i = 0; i < io_module->عدد_Functions; ++i) {
        ثابت IM3Function f = &io_module->functions[i];

        اگر (f->import.moduleUtf8 and f->import.میدان_Utf8) {
            اگر (strcmp(f->import.میدان_Utf8, i_functionName) == 0 and
                (wildcardModule or strcmp(f->import.moduleUtf8, i_moduleName) == 0)) {
                اگر (i_signature) {
_                   (ValidateSignature(f, i_signature));
                }
_               (CompileRawFunction(io_module, f, i_function, i_userdata));
            }
        }
    }
} _catch:
    واپس نتیجہ;
}

M3_نتیجہ m3_LinkRawFunctionEx (IM3Module         io_module,
                               ثابت حرف* ثابت i_moduleName,
                               ثابت حرف* ثابت i_functionName,
                               ثابت حرف* ثابت i_signature,
                               M3RawCall         i_function,
                               ثابت خالی*       i_userdata)
{
    واپس ڈھونڈو_And_Link_Function(io_module, i_moduleName, i_functionName, i_signature, (voidptr_t)i_function, i_userdata);
}

M3_نتیجہ m3_LinkRawFunction (IM3Module         io_module,
                             ثابت حرف* ثابت i_moduleName,
                             ثابت حرف* ثابت i_functionName,
                             ثابت حرف* ثابت i_signature,
                             M3RawCall         i_function)
{
    واپس ڈھونڈو_And_Link_Function(io_module, i_moduleName, i_functionName, i_signature, (voidptr_t)i_function, تہی);
}
