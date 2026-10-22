//
//  m3_bind.c
//
//  Created by Steven Massey on 4/29/19.
//  Copyright © 2019 Steven Massey. All rights reserved.
//

#ଅନ୍ତର୍ଭାଵ "m3_env.h"
#ଅନ୍ତର୍ଭାଵ "m3_bind.h"
#ଅନ୍ତର୍ଭାଵ "m3_exception.h"
#ଅନ୍ତର୍ଭାଵ "m3_info.h"


u8 ConvertTypeCharToTypeId (ଅକ୍ଷର i_code)
{
    ବାଛ (i_code) {
    ସ୍ଥିତି 'v': ଫେରନ୍ତୁ c_m3Type_none;
    ସ୍ଥିତି 'i': ଫେରନ୍ତୁ c_m3Type_i32;
    ସ୍ଥିତି 'I': ଫେରନ୍ତୁ c_m3Type_i64;
    ସ୍ଥିତି 'f': ଫେରନ୍ତୁ c_m3Type_f32;
    ସ୍ଥିତି 'F': ଫେରନ୍ତୁ c_m3Type_f64;
    ସ୍ଥିତି '*': ଫେରନ୍ତୁ c_m3Type_i32;
    // same letters c_waCompactTypes uses for the reference types
    ସ୍ଥିତି 'r': ଫେରନ୍ତୁ c_m3Type_funcref;
    ସ୍ଥିତି 'R': ଫେରନ୍ତୁ c_m3Type_externref;
    ସ୍ଥିତି 'c': ଫେରନ୍ତୁ c_m3Type_contref;
    }
    ଫେରନ୍ତୁ c_m3Type_unknown;
}


M3Result SignatureToFuncType (IM3FuncType* o_functionType, ccstr_t i_signature)
{
    IM3FuncType funcType = ଖାଲି;

_try {
    ଯଦି (not o_functionType) {
        _throw("null function type");
    }

    ଯଦି (not i_signature) {
        _throw("null function signature");
    }

    cstr_t sig = i_signature;

    size_t max_ଅଙ୍କ_Types = strlen(i_signature);

    // assume min signature is "()"
    _throwif(m3Err_malformedFunctionSignature, max_ଅଙ୍କ_Types < 2);
    max_ଅଙ୍କ_Types -= 2;

    _throwif(m3Err_tooManyArgsRets, max_ଅଙ୍କ_Types > d_m3_Max_Sane_Function_Arg_Ret_ଗଣନା);

_   (AllocFuncType(&funcType, (u32)max_ଅଙ୍କ_Types));

    m3type_t* typelist = funcType->types;

    bool parsingRets = true;
    ଯେତେବେଳେ (*sig) {
        ଅକ୍ଷର typeChar = *sig++;

        ଯଦି (typeChar == '(') {
            parsingRets = false;
            ଜାରି;
        } ଅନ୍ୟଥା ଯଦି (typeChar == ' ') {
            ଜାରି;
        } ଅନ୍ୟଥା ଯଦି (typeChar == ')') {
            ଭାଙ୍ଗ;
        }

        u8 type = ConvertTypeCharToTypeId(typeChar);

        _throwif("unknown argument type char", c_m3Type_unknown == type);

        ଯଦି (type == c_m3Type_none) {
            ଜାରି;
        }

        ଯଦି (parsingRets) {
            _throwif("malformed signature; return count overflow", funcType->ଅଙ୍କ_Rets >= max_ଅଙ୍କ_Types);
            funcType->ଅଙ୍କ_Rets++;
            *typelist++ = type;
        } ଅନ୍ୟଥା {
            _throwif("malformed signature; arg count overflow", (u32)(funcType->ଅଙ୍କ_Rets) + funcType->ଅଙ୍କ_Args >= max_ଅଙ୍କ_Types);
            funcType->ଅଙ୍କ_Args++;
            *typelist++ = type;
        }
    }

} _catch:

    ଯଦି (result) {
        m3_Free(funcType);
    }

    *o_functionType = funcType;

    ଫେରନ୍ତୁ result;
}


ସ୍ଥିର
M3Result ValidateSignature (IM3Function i_function, ccstr_t i_linkingSignature)
{
    M3Result result = m3Err_none;

    IM3FuncType ftype = ଖାଲି;
_   (SignatureToFuncType(&ftype, i_linkingSignature));

    ଯଦି (not AreFuncTypesEqual(ftype, i_function->funcType)) {
        m3log(module, "expected: %s", SPrintFuncTypeSignature(ftype));
        m3log(module, "   found: %s", SPrintFuncTypeSignature(i_function->funcType));

        _throw(m3Err_functionSignatureMismatch);
    }

    _catch:

    m3_Free(ftype);

    ଫେରନ୍ତୁ result;
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

    ଅଚଳ bool wildcardModule = (strcmp(i_moduleName, "*") == 0);

    result = m3Err_functionLookupFailed;

    ପାଇଁ (u32 i = 0; i < io_module->ଅଙ୍କ_Functions; ++i) {
        ଅଚଳ IM3Function f = &io_module->functions[i];

        ଯଦି (f->import.moduleUtf8 and f->import.fieldUtf8) {
            ଯଦି (strcmp(f->import.fieldUtf8, i_functionName) == 0 and
                (wildcardModule or strcmp(f->import.moduleUtf8, i_moduleName) == 0)) {
                ଯଦି (i_signature) {
_                   (ValidateSignature(f, i_signature));
                }
_               (CompileRawFunction(io_module, f, i_function, i_userdata));
            }
        }
    }
} _catch:
    ଫେରନ୍ତୁ result;
}

M3Result m3_LinkRawFunctionEx (IM3Module         io_module,
                               ଅଚଳ ଅକ୍ଷର* ଅଚଳ i_moduleName,
                               ଅଚଳ ଅକ୍ଷର* ଅଚଳ i_functionName,
                               ଅଚଳ ଅକ୍ଷର* ଅଚଳ i_signature,
                               M3RawCall         i_function,
                               ଅଚଳ ଶୂନ୍ୟ*       i_userdata)
{
    ଫେରନ୍ତୁ FindAndLinkFunction(io_module, i_moduleName, i_functionName, i_signature, (voidptr_t)i_function, i_userdata);
}

M3Result m3_LinkRawFunction (IM3Module         io_module,
                             ଅଚଳ ଅକ୍ଷର* ଅଚଳ i_moduleName,
                             ଅଚଳ ଅକ୍ଷର* ଅଚଳ i_functionName,
                             ଅଚଳ ଅକ୍ଷର* ଅଚଳ i_signature,
                             M3RawCall         i_function)
{
    ଫେରନ୍ତୁ FindAndLinkFunction(io_module, i_moduleName, i_functionName, i_signature, (voidptr_t)i_function, ଖାଲି);
}
