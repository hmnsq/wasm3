//
//  m3_bind.c
//
//  Created by Steven Massey on 4/29/19.
//  Copyright © 2019 Steven Massey. All rights reserved.
//

#અન્તર્ભાવ "m3_env.h"
#અન્તર્ભાવ "m3_bind.h"
#અન્તર્ભાવ "m3_exception.h"
#અન્તર્ભાવ "m3_info.h"


u8 ConvertTypeCharToTypeId (અક્ષર i_code)
{
    પસંદગી (i_code) {
    સ્થિતિ 'v': પાછા c_m3Type_none;
    સ્થિતિ 'i': પાછા c_m3Type_i32;
    સ્થિતિ 'I': પાછા c_m3Type_i64;
    સ્થિતિ 'f': પાછા c_m3Type_f32;
    સ્થિતિ 'F': પાછા c_m3Type_f64;
    સ્થિતિ '*': પાછા c_m3Type_i32;
    // same letters c_waCompactTypes uses for the reference types
    સ્થિતિ 'r': પાછા c_m3Type_funcref;
    સ્થિતિ 'R': પાછા c_m3Type_externref;
    સ્થિતિ 'c': પાછા c_m3Type_contref;
    }
    પાછા c_m3Type_unknown;
}


M3Result SignatureToFuncType (IM3FuncType* o_functionType, ccstr_t i_signature)
{
    IM3FuncType funcType = ખાલી;

_try {
    જો (not o_functionType) {
        _throw("null function type");
    }

    જો (not i_signature) {
        _throw("null function signature");
    }

    cstr_t sig = i_signature;

    size_t max_અંક_Types = strlen(i_signature);

    // assume min signature is "()"
    _throwif(m3Err_malformedFunctionSignature, max_અંક_Types < 2);
    max_અંક_Types -= 2;

    _throwif(m3Err_tooManyArgsRets, max_અંક_Types > d_m3_Max_Sane_Function_Arg_Ret_ગણતરી);

_   (AllocFuncType(&funcType, (u32)max_અંક_Types));

    m3type_t* typelist = funcType->types;

    bool parsingRets = true;
    જ્યારે (*sig) {
        અક્ષર typeChar = *sig++;

        જો (typeChar == '(') {
            parsingRets = false;
            ચાલુ;
        } નહીંતર જો (typeChar == ' ') {
            ચાલુ;
        } નહીંતર જો (typeChar == ')') {
            તોડો;
        }

        u8 type = ConvertTypeCharToTypeId(typeChar);

        _throwif("unknown argument type char", c_m3Type_unknown == type);

        જો (type == c_m3Type_none) {
            ચાલુ;
        }

        જો (parsingRets) {
            _throwif("malformed signature; return count overflow", funcType->અંક_Rets >= max_અંક_Types);
            funcType->અંક_Rets++;
            *typelist++ = type;
        } નહીંતર {
            _throwif("malformed signature; arg count overflow", (u32)(funcType->અંક_Rets) + funcType->અંક_Args >= max_અંક_Types);
            funcType->અંક_Args++;
            *typelist++ = type;
        }
    }

} _catch:

    જો (result) {
        m3_Free(funcType);
    }

    *o_functionType = funcType;

    પાછા result;
}


સ્થિર
M3Result ValidateSignature (IM3Function i_function, ccstr_t i_linkingSignature)
{
    M3Result result = m3Err_none;

    IM3FuncType ftype = ખાલી;
_   (SignatureToFuncType(&ftype, i_linkingSignature));

    જો (not AreFuncTypesEqual(ftype, i_function->funcType)) {
        m3log(module, "expected: %s", SPrintFuncTypeSignature(ftype));
        m3log(module, "   found: %s", SPrintFuncTypeSignature(i_function->funcType));

        _throw(m3Err_functionSignatureMismatch);
    }

    _catch:

    m3_Free(ftype);

    પાછા result;
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

    અચલ bool wildcardModule = (strcmp(i_moduleName, "*") == 0);

    result = m3Err_functionLookupFailed;

    માટે (u32 i = 0; i < io_module->અંક_Functions; ++i) {
        અચલ IM3Function f = &io_module->functions[i];

        જો (f->import.moduleUtf8 and f->import.fieldUtf8) {
            જો (strcmp(f->import.fieldUtf8, i_functionName) == 0 and
                (wildcardModule or strcmp(f->import.moduleUtf8, i_moduleName) == 0)) {
                જો (i_signature) {
_                   (ValidateSignature(f, i_signature));
                }
_               (CompileRawFunction(io_module, f, i_function, i_userdata));
            }
        }
    }
} _catch:
    પાછા result;
}

M3Result m3_LinkRawFunctionEx (IM3Module         io_module,
                               અચલ અક્ષર* અચલ i_moduleName,
                               અચલ અક્ષર* અચલ i_functionName,
                               અચલ અક્ષર* અચલ i_signature,
                               M3RawCall         i_function,
                               અચલ શૂન્ય*       i_userdata)
{
    પાછા FindAndLinkFunction(io_module, i_moduleName, i_functionName, i_signature, (voidptr_t)i_function, i_userdata);
}

M3Result m3_LinkRawFunction (IM3Module         io_module,
                             અચલ અક્ષર* અચલ i_moduleName,
                             અચલ અક્ષર* અચલ i_functionName,
                             અચલ અક્ષર* અચલ i_signature,
                             M3RawCall         i_function)
{
    પાછા FindAndLinkFunction(io_module, i_moduleName, i_functionName, i_signature, (voidptr_t)i_function, ખાલી);
}
