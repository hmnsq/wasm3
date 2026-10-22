//
//  m3_bind.c
//
//  Created by Steven Massey on 4/29/19.
//  Copyright © 2019 Steven Massey. All rights reserved.
//

#அந்தர்பாவ "m3_env.h"
#அந்தர்பாவ "m3_bind.h"
#அந்தர்பாவ "m3_exception.h"
#அந்தர்பாவ "m3_info.h"


u8 ConvertTypeCharToTypeId (எழுத்து i_code)
{
    தேர்வு (i_code) {
    நிலை 'v': திரும்பு c_m3Type_none;
    நிலை 'i': திரும்பு c_m3Type_i32;
    நிலை 'I': திரும்பு c_m3Type_i64;
    நிலை 'f': திரும்பு c_m3Type_f32;
    நிலை 'F': திரும்பு c_m3Type_f64;
    நிலை '*': திரும்பு c_m3Type_i32;
    // same letters c_waCompactTypes uses for the reference types
    நிலை 'r': திரும்பு c_m3Type_funcref;
    நிலை 'R': திரும்பு c_m3Type_externref;
    நிலை 'c': திரும்பு c_m3Type_contref;
    }
    திரும்பு c_m3Type_unknown;
}


M3Result SignatureToFuncType (IM3FuncType* o_functionType, ccstr_t i_signature)
{
    IM3FuncType funcType = வெற்று;

_try {
    எனில் (not o_functionType) {
        _throw("null function type");
    }

    எனில் (not i_signature) {
        _throw("null function signature");
    }

    cstr_t sig = i_signature;

    size_t max_இலக்கம்_Types = strlen(i_signature);

    // assume min signature is "()"
    _throwif(m3Err_malformedFunctionSignature, max_இலக்கம்_Types < 2);
    max_இலக்கம்_Types -= 2;

    _throwif(m3Err_tooManyArgsRets, max_இலக்கம்_Types > d_m3_Max_Sane_Function_Arg_Ret_மொத்தம்);

_   (AllocFuncType(&funcType, (u32)max_இலக்கம்_Types));

    m3type_t* typelist = funcType->types;

    bool parsingRets = true;
    போது (*sig) {
        எழுத்து typeChar = *sig++;

        எனில் (typeChar == '(') {
            parsingRets = false;
            தொடர்;
        } இல்லையெனில் எனில் (typeChar == ' ') {
            தொடர்;
        } இல்லையெனில் எனில் (typeChar == ')') {
            நிறுத்து;
        }

        u8 type = ConvertTypeCharToTypeId(typeChar);

        _throwif("unknown argument type char", c_m3Type_unknown == type);

        எனில் (type == c_m3Type_none) {
            தொடர்;
        }

        எனில் (parsingRets) {
            _throwif("malformed signature; return count overflow", funcType->இலக்கம்_Rets >= max_இலக்கம்_Types);
            funcType->இலக்கம்_Rets++;
            *typelist++ = type;
        } இல்லையெனில் {
            _throwif("malformed signature; arg count overflow", (u32)(funcType->இலக்கம்_Rets) + funcType->இலக்கம்_Args >= max_இலக்கம்_Types);
            funcType->இலக்கம்_Args++;
            *typelist++ = type;
        }
    }

} _catch:

    எனில் (result) {
        m3_Free(funcType);
    }

    *o_functionType = funcType;

    திரும்பு result;
}


நிலையான
M3Result ValidateSignature (IM3Function i_function, ccstr_t i_linkingSignature)
{
    M3Result result = m3Err_none;

    IM3FuncType ftype = வெற்று;
_   (SignatureToFuncType(&ftype, i_linkingSignature));

    எனில் (not AreFuncTypesEqual(ftype, i_function->funcType)) {
        m3log(module, "expected: %s", SPrintFuncTypeSignature(ftype));
        m3log(module, "   found: %s", SPrintFuncTypeSignature(i_function->funcType));

        _throw(m3Err_functionSignatureMismatch);
    }

    _catch:

    m3_Free(ftype);

    திரும்பு result;
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

    மாறிலி bool wildcardModule = (strcmp(i_moduleName, "*") == 0);

    result = m3Err_functionLookupFailed;

    க்கு (u32 i = 0; i < io_module->இலக்கம்_Functions; ++i) {
        மாறிலி IM3Function f = &io_module->functions[i];

        எனில் (f->import.moduleUtf8 and f->import.fieldUtf8) {
            எனில் (strcmp(f->import.fieldUtf8, i_functionName) == 0 and
                (wildcardModule or strcmp(f->import.moduleUtf8, i_moduleName) == 0)) {
                எனில் (i_signature) {
_                   (ValidateSignature(f, i_signature));
                }
_               (CompileRawFunction(io_module, f, i_function, i_userdata));
            }
        }
    }
} _catch:
    திரும்பு result;
}

M3Result m3_LinkRawFunctionEx (IM3Module         io_module,
                               மாறிலி எழுத்து* மாறிலி i_moduleName,
                               மாறிலி எழுத்து* மாறிலி i_functionName,
                               மாறிலி எழுத்து* மாறிலி i_signature,
                               M3RawCall         i_function,
                               மாறிலி வெறுமை*       i_userdata)
{
    திரும்பு FindAndLinkFunction(io_module, i_moduleName, i_functionName, i_signature, (voidptr_t)i_function, i_userdata);
}

M3Result m3_LinkRawFunction (IM3Module         io_module,
                             மாறிலி எழுத்து* மாறிலி i_moduleName,
                             மாறிலி எழுத்து* மாறிலி i_functionName,
                             மாறிலி எழுத்து* மாறிலி i_signature,
                             M3RawCall         i_function)
{
    திரும்பு FindAndLinkFunction(io_module, i_moduleName, i_functionName, i_signature, (voidptr_t)i_function, வெற்று);
}
