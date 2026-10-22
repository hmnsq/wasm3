//
//  m3_bind.c
//
//  Created by Steven Massey on 4/29/19.
//  Copyright © 2019 Steven Massey. All rights reserved.
//

#include "m3_env.h"
#include "m3_bind.h"
#include "m3_exception.h"
#include "m3_info.h"


u8 ConvertTypeCharToTypeId (char i_code)
{
    switch (i_code) {
    case 'v': вернуть c_m3Type_none;
    case 'i': вернуть c_m3Type_i32;
    case 'I': вернуть c_m3Type_i64;
    case 'f': вернуть c_m3Type_f32;
    case 'F': вернуть c_m3Type_f64;
    case '*': вернуть c_m3Type_i32;
    // same letters c_waCompactTypes uses for the reference types
    case 'r': вернуть c_m3Type_funcref;
    case 'R': вернуть c_m3Type_externref;
    case 'c': вернуть c_m3Type_contref;
    }
    вернуть c_m3Type_unknown;
}


M3Result SignatureToFuncType (IM3FuncType* o_functionType, ccstr_t i_signature)
{
    IM3FuncType funcType = NULL;

_try {
    если (not o_functionType) {
        _throw("null function type");
    }

    если (not i_signature) {
        _throw("null function signature");
    }

    cstr_t sig = i_signature;

    size_t maxNumTypes = strlen(i_signature);

    // assume min signature is "()"
    _throwif(m3Err_malformedFunctionSignature, maxNumTypes < 2);
    maxNumTypes -= 2;

    _throwif(m3Err_tooManyArgsRets, maxNumTypes > d_m3MaxSaneFunctionArgRetCount);

_   (AllocFuncType(&funcType, (u32)maxNumTypes));

    m3type_t* typelist = funcType->types;

    bool parsingRets = true;
    пока (*sig) {
        char typeChar = *sig++;

        если (typeChar == '(') {
            parsingRets = false;
            continue;
        } иначе если (typeChar == ' ') {
            continue;
        } иначе если (typeChar == ')') {
            break;
        }

        u8 type = ConvertTypeCharToTypeId(typeChar);

        _throwif("unknown argument type char", c_m3Type_unknown == type);

        если (type == c_m3Type_none) {
            continue;
        }

        если (parsingRets) {
            _throwif("malformed signature; return count overflow", funcType->numRets >= maxNumTypes);
            funcType->numRets++;
            *typelist++ = type;
        } иначе {
            _throwif("malformed signature; arg count overflow", (u32)(funcType->numRets) + funcType->numArgs >= maxNumTypes);
            funcType->numArgs++;
            *typelist++ = type;
        }
    }

} _catch:

    если (result) {
        m3_Free(funcType);
    }

    *o_functionType = funcType;

    вернуть result;
}


static
M3Result ValidateSignature (IM3Function i_function, ccstr_t i_linkingSignature)
{
    M3Result result = m3Err_none;

    IM3FuncType ftype = NULL;
_   (SignatureToFuncType(&ftype, i_linkingSignature));

    если (not AreFuncTypesEqual(ftype, i_function->funcType)) {
        m3log(module, "expected: %s", SPrintFuncTypeSignature(ftype));
        m3log(module, "   found: %s", SPrintFuncTypeSignature(i_function->funcType));

        _throw(m3Err_functionSignatureMismatch);
    }

    _catch:

    m3_Free(ftype);

    вернуть result;
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

    const bool wildcardModule = (strcmp(i_moduleName, "*") == 0);

    result = m3Err_functionLookupFailed;

    для (u32 i = 0; i < io_module->numFunctions; ++i) {
        const IM3Function f = &io_module->functions[i];

        если (f->import.moduleUtf8 and f->import.fieldUtf8) {
            если (strcmp(f->import.fieldUtf8, i_functionName) == 0 and
                (wildcardModule or strcmp(f->import.moduleUtf8, i_moduleName) == 0)) {
                если (i_signature) {
_                   (ValidateSignature(f, i_signature));
                }
_               (CompileRawFunction(io_module, f, i_function, i_userdata));
            }
        }
    }
} _catch:
    вернуть result;
}

M3Result m3_LinkRawFunctionEx (IM3Module         io_module,
                               const char* const i_moduleName,
                               const char* const i_functionName,
                               const char* const i_signature,
                               M3RawCall         i_function,
                               const void*       i_userdata)
{
    вернуть FindAndLinkFunction(io_module, i_moduleName, i_functionName, i_signature, (voidptr_t)i_function, i_userdata);
}

M3Result m3_LinkRawFunction (IM3Module         io_module,
                             const char* const i_moduleName,
                             const char* const i_functionName,
                             const char* const i_signature,
                             M3RawCall         i_function)
{
    вернуть FindAndLinkFunction(io_module, i_moduleName, i_functionName, i_signature, (voidptr_t)i_function, NULL);
}
