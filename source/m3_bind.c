//
//  m3_bind.c
//
//  Created by Steven Massey on 4/29/19.
//  Copyright © 2019 Steven Massey. All rights reserved.
//

#অন্তর্ভাব "m3_env.h"
#অন্তর্ভাব "m3_bind.h"
#অন্তর্ভাব "m3_exception.h"
#অন্তর্ভাব "m3_info.h"


u8 ConvertTypeCharToTypeId (বর্ণ i_code)
{
    বিকল্প (i_code) {
    পক্ষ 'v': প্রত্যাগম c_m3Type_none;
    পক্ষ 'i': প্রত্যাগম c_m3Type_i32;
    পক্ষ 'I': প্রত্যাগম c_m3Type_i64;
    পক্ষ 'f': প্রত্যাগম c_m3Type_f32;
    পক্ষ 'F': প্রত্যাগম c_m3Type_f64;
    পক্ষ '*': প্রত্যাগম c_m3Type_i32;
    // same letters c_waCompactTypes uses for the reference types
    পক্ষ 'r': প্রত্যাগম c_m3Type_funcref;
    পক্ষ 'R': প্রত্যাগম c_m3Type_externref;
    পক্ষ 'c': প্রত্যাগম c_m3Type_contref;
    }
    প্রত্যাগম c_m3Type_unknown;
}


M3Result SignatureToFuncType (IM3FuncType* o_functionType, ccstr_t i_signature)
{
    IM3FuncType funcType = রিক্ত;

_try {
    যদি (not o_functionType) {
        _throw("null function type");
    }

    যদি (not i_signature) {
        _throw("null function signature");
    }

    cstr_t sig = i_signature;

    size_t max_অঙ্ক_Types = strlen(i_signature);

    // assume min signature is "()"
    _throwif(m3Err_malformedFunctionSignature, max_অঙ্ক_Types < 2);
    max_অঙ্ক_Types -= 2;

    _throwif(m3Err_tooManyArgsRets, max_অঙ্ক_Types > d_m3_Max_Sane_Function_Arg_Ret_গুনতি);

_   (AllocFuncType(&funcType, (u32)max_অঙ্ক_Types));

    m3type_t* typelist = funcType->types;

    bool parsingRets = true;
    যাবত্ (*sig) {
        বর্ণ typeChar = *sig++;

        যদি (typeChar == '(') {
            parsingRets = false;
            অনুবর্ত;
        } অন্যথা যদি (typeChar == ' ') {
            অনুবর্ত;
        } অন্যথা যদি (typeChar == ')') {
            ভঙ্গ;
        }

        u8 type = ConvertTypeCharToTypeId(typeChar);

        _throwif("unknown argument type char", c_m3Type_unknown == type);

        যদি (type == c_m3Type_none) {
            অনুবর্ত;
        }

        যদি (parsingRets) {
            _throwif("malformed signature; return count overflow", funcType->অঙ্ক_Rets >= max_অঙ্ক_Types);
            funcType->অঙ্ক_Rets++;
            *typelist++ = type;
        } অন্যথা {
            _throwif("malformed signature; arg count overflow", (u32)(funcType->অঙ্ক_Rets) + funcType->অঙ্ক_Args >= max_অঙ্ক_Types);
            funcType->অঙ্ক_Args++;
            *typelist++ = type;
        }
    }

} _catch:

    যদি (result) {
        m3_Free(funcType);
    }

    *o_functionType = funcType;

    প্রত্যাগম result;
}


স্থির
M3Result ValidateSignature (IM3Function i_function, ccstr_t i_linkingSignature)
{
    M3Result result = m3Err_none;

    IM3FuncType ftype = রিক্ত;
_   (SignatureToFuncType(&ftype, i_linkingSignature));

    যদি (not AreFuncTypesEqual(ftype, i_function->funcType)) {
        m3log(module, "expected: %s", SPrintFuncTypeSignature(ftype));
        m3log(module, "   found: %s", SPrintFuncTypeSignature(i_function->funcType));

        _throw(m3Err_functionSignatureMismatch);
    }

    _catch:

    m3_Free(ftype);

    প্রত্যাগম result;
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

    ধ্রুব bool wildcardModule = (strcmp(i_moduleName, "*") == 0);

    result = m3Err_functionLookupFailed;

    প্রতি (u32 i = 0; i < io_module->অঙ্ক_Functions; ++i) {
        ধ্রুব IM3Function f = &io_module->functions[i];

        যদি (f->import.moduleUtf8 and f->import.fieldUtf8) {
            যদি (strcmp(f->import.fieldUtf8, i_functionName) == 0 and
                (wildcardModule or strcmp(f->import.moduleUtf8, i_moduleName) == 0)) {
                যদি (i_signature) {
_                   (ValidateSignature(f, i_signature));
                }
_               (CompileRawFunction(io_module, f, i_function, i_userdata));
            }
        }
    }
} _catch:
    প্রত্যাগম result;
}

M3Result m3_LinkRawFunctionEx (IM3Module         io_module,
                               ধ্রুব বর্ণ* ধ্রুব i_moduleName,
                               ধ্রুব বর্ণ* ধ্রুব i_functionName,
                               ধ্রুব বর্ণ* ধ্রুব i_signature,
                               M3RawCall         i_function,
                               ধ্রুব শূন্য*       i_userdata)
{
    প্রত্যাগম FindAndLinkFunction(io_module, i_moduleName, i_functionName, i_signature, (voidptr_t)i_function, i_userdata);
}

M3Result m3_LinkRawFunction (IM3Module         io_module,
                             ধ্রুব বর্ণ* ধ্রুব i_moduleName,
                             ধ্রুব বর্ণ* ধ্রুব i_functionName,
                             ধ্রুব বর্ণ* ধ্রুব i_signature,
                             M3RawCall         i_function)
{
    প্রত্যাগম FindAndLinkFunction(io_module, i_moduleName, i_functionName, i_signature, (voidptr_t)i_function, রিক্ত);
}
