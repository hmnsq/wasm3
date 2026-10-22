//
//  m3_function.c
//
//  Created by Steven Massey on 4/7/21.
//  Copyright © 2021 Steven Massey. All rights reserved.
//

#ਅਨ੍ਤਰ੍ਭਾਵ "m3_function.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "m3_env.h"


M3_ਨਤੀਜਾ AllocFuncType (IM3FuncType* o_functionType, u32 i_ਅੰਕ_Types)
{
    *o_functionType = (IM3FuncType)m3_Malloc("M3FuncType", ਆਕਾਰ(M3FuncType) + i_ਅੰਕ_Types * ਆਕਾਰ(m3type_t));
    ਜੇ (*o_functionType) {
        (*o_functionType)->isContinuation = false;
        (*o_functionType)->contFuncType   = ਨਿਰਾ;
        (*o_functionType)->ਵਿੱਚ_Rec_ਸਮੂਹ     = false;
        ਵਾਪਸ m3Err_none;
    }
    ਵਾਪਸ m3Err_mallocFailed;
}


#ਜੇ d_m3HasTypedRefs

// The type a (ref $t) / (ref null $t) naming this function type is spelled as
m3type_t Ref_Type_ਦਾ_Func_Type (ਅਚਲ IM3FuncType i_funcType, bool i_nonNull)
{
    m3type_t type = d_m3Type_ref | (i_funcType ? i_funcType->canonical_ਸੂਚਕਾਂਕ : d_m3_Type_ਢੇਰ_Abstract);

    ਜੇ (i_funcType and i_funcType->isContinuation) {
        type |= d_m3Type_refCont;
    }

    ਜੇ (i_nonNull) {
        type |= d_m3Type_refNonNull;
    }

    ਵਾਪਸ type;
}

#ਜੇ_ਅੰਤ // d_m3HasTypedRefs


bool AreFuncTypesEqual (ਅਚਲ IM3FuncType i_typeA, ਅਚਲ IM3FuncType i_typeB)
{
    ਜੇ (i_typeA->isContinuation != i_typeB->isContinuation) {
        ਵਾਪਸ false;
    }
    ਜੇ (i_typeA->isContinuation) {
        ਵਾਪਸ (i_typeA->contFuncType == i_typeB->contFuncType) or
               (i_typeA->contFuncType and i_typeB->contFuncType and
                AreFuncTypesEqual(i_typeA->contFuncType, i_typeB->contFuncType));
    }
    ਜੇ (i_typeA->ਅੰਕ_Rets == i_typeB->ਅੰਕ_Rets and i_typeA->ਅੰਕ_ਦਲੀਲਾਂ == i_typeB->ਅੰਕ_ਦਲੀਲਾਂ) {
        ਵਾਪਸ (memcmp(i_typeA->types, i_typeB->types,
                       (i_typeA->ਅੰਕ_Rets + i_typeA->ਅੰਕ_ਦਲੀਲਾਂ) * ਆਕਾਰ(m3type_t)) == 0);
    }

    ਵਾਪਸ false;
}

u16 Get_Func_Type_ਅੰਕ_Params (ਅਚਲ IM3FuncType i_funcType)
{
    ਵਾਪਸ i_funcType ? i_funcType->ਅੰਕ_ਦਲੀਲਾਂ : 0;
}


m3type_t GetFuncTypeParamType (ਅਚਲ IM3FuncType i_funcType, u16 i_ਸੂਚਕਾਂਕ)
{
    m3type_t type = c_m3Type_unknown;

    ਜੇ (i_funcType) {
        ਜੇ (i_ਸੂਚਕਾਂਕ < i_funcType->ਅੰਕ_ਦਲੀਲਾਂ) {
            type = i_funcType->types[i_funcType->ਅੰਕ_Rets + i_ਸੂਚਕਾਂਕ];
        }
    }

    ਵਾਪਸ type;
}


u16 Get_Func_Type_ਅੰਕ_Results (ਅਚਲ IM3FuncType i_funcType)
{
    ਵਾਪਸ i_funcType ? i_funcType->ਅੰਕ_Rets : 0;
}


m3type_t Get_Func_Type_ਨਤੀਜਾ_Type (ਅਚਲ IM3FuncType i_funcType, u16 i_ਸੂਚਕਾਂਕ)
{
    m3type_t type = c_m3Type_unknown;

    ਜੇ (i_funcType) {
        ਜੇ (i_ਸੂਚਕਾਂਕ < i_funcType->ਅੰਕ_Rets) {
            type = i_funcType->types[i_ਸੂਚਕਾਂਕ];
        }
    }

    ਵਾਪਸ type;
}


//---------------------------------------------------------------------------------------------------------------


ਖਾਲੀ FreeImportInfo (M3ImportInfo* i_info)
{
    m3_Free(i_info->moduleUtf8);
    m3_Free(i_info->ਖੇਤਰ_Utf8);
}


#ਜੇ d_m3HasSnapshots

ਖਾਲੀ SnapshotMap_Free (M3SnapshotMap* i_map)
{
    ਜੇ (i_map) {
        m3_Free(i_map->locals);
        m3_Free(i_map->ਸੁਰੱਖਿਅਤ_Points);
        m3_Free(i_map->ਮੁੱਲਾਂ);
        m3_Free(i_map->ਖੰਡਾਂ);
        m3_Free(i_map);
    }
}

#ਜੇ_ਅੰਤ

ਖਾਲੀ Function_Release (IM3Function i_function)
{
    m3_Free(i_function->constants);

#ਜੇ d_m3HasSnapshots
    SnapshotMap_Free(i_function->snapshotMap);
    i_function->snapshotMap = ਨਿਰਾ;
#ਜੇ_ਅੰਤ

    ਲਈ (ਪੂਰਨ i = 0; i < i_function->ਅੰਕ_ਨਾਂ; i++) {
        // name can be an alias of fieldUtf8
        ਜੇ (i_function->ਨਾਂ[i] != i_function->import.ਖੇਤਰ_Utf8) {
            m3_Free(i_function->ਨਾਂ[i]);
        }
    }

    FreeImportInfo(&i_function->import);

    ਜੇ (i_function->owns_Wasm_ਕੋਡ) {
        m3_Free(i_function->wasm);
    }

    // Function_FreeCompiledCode (func);

#ਜੇ (d_m3_Enable_ਕੋਡ_ਪੰਨਾ_Ref_Counting)
    {
        m3_Free(i_function->ਕੋਡ_ਪੰਨਾ_Refs);
        i_function->ਅੰਕ_ਕੋਡ_ਪੰਨਾ_Refs = 0;
    }
#ਜੇ_ਅੰਤ
}


ਖਾਲੀ Function_Free_Compiled_ਕੋਡ (IM3Function i_function)
{
#ਜੇ (d_m3_Enable_ਕੋਡ_ਪੰਨਾ_Ref_Counting)
    {
        i_function->compiled = ਨਿਰਾ;

        ਜਦੋਂ (i_function->ਅੰਕ_ਕੋਡ_ਪੰਨਾ_Refs--) {
            IM3_ਕੋਡ_ਪੰਨਾ ਪੰਨਾ = i_function->ਕੋਡ_ਪੰਨਾ_Refs[i_function->ਅੰਕ_ਕੋਡ_ਪੰਨਾ_Refs];

            ਜੇ (--(ਪੰਨਾ->info.usage_ਗਿਣਤੀ) == 0) {
                //                printf ("free %p\n", page);
            }
        }

        m3_Free(i_function->ਕੋਡ_ਪੰਨਾ_Refs);

        Runtime_Release_ਕੋਡ_ਪੰਨੇ(i_function->module->runtime);
    }
#ਜੇ_ਅੰਤ
}


cstr_t m3_GetFunctionName (IM3Function i_function)
{
    u16     ਅੰਕ_ਨਾਂ = 0;
    cstr_t* ਨਾਂ    = Get_Function_ਨਾਂ(i_function, &ਅੰਕ_ਨਾਂ);
    ਜੇ (ਅੰਕ_ਨਾਂ > 0) {
        ਵਾਪਸ ਨਾਂ[0];
    } ਨਹੀਂਤਾਂ {
        ਵਾਪਸ "<unnamed>";
    }
}


IM3Module m3_GetFunctionModule (IM3Function i_function)
{
    ਵਾਪਸ i_function ? i_function->module : ਨਿਰਾ;
}


cstr_t* Get_Function_ਨਾਂ (IM3Function i_function, u16* o_ਅੰਕ_ਨਾਂ)
{
    ਜੇ (!i_function || !o_ਅੰਕ_ਨਾਂ) {
        ਵਾਪਸ ਨਿਰਾ;
    }

    ਜੇ (i_function->import.ਖੇਤਰ_Utf8) {
        *o_ਅੰਕ_ਨਾਂ = 1;
        ਵਾਪਸ &i_function->import.ਖੇਤਰ_Utf8;
    } ਨਹੀਂਤਾਂ {
        *o_ਅੰਕ_ਨਾਂ = i_function->ਅੰਕ_ਨਾਂ;
        ਵਾਪਸ i_function->ਨਾਂ;
    }
}


cstr_t GetFunctionImportModuleName (IM3Function i_function)
{
    ਵਾਪਸ (i_function->import.moduleUtf8) ? i_function->import.moduleUtf8 : "";
}


u16 Get_Function_ਅੰਕ_ਦਲੀਲਾਂ (IM3Function i_function)
{
    u16 ਅੰਕ_ਦਲੀਲਾਂ = 0;

    ਜੇ (i_function) {
        ਜੇ (i_function->funcType) {
            ਅੰਕ_ਦਲੀਲਾਂ = i_function->funcType->ਅੰਕ_ਦਲੀਲਾਂ;
        }
    }

    ਵਾਪਸ ਅੰਕ_ਦਲੀਲਾਂ;
}

m3type_t GetFunctionArgType (IM3Function i_function, u32 i_ਸੂਚਕਾਂਕ)
{
    m3type_t type = c_m3Type_none;

    ਜੇ (i_ਸੂਚਕਾਂਕ < Get_Function_ਅੰਕ_ਦਲੀਲਾਂ(i_function)) {
        u32 ਅੰਕ_Returns = i_function->funcType->ਅੰਕ_Rets;

        type = i_function->funcType->types[ਅੰਕ_Returns + i_ਸੂਚਕਾਂਕ];
    }

    ਵਾਪਸ type;
}


u16 Get_Function_ਅੰਕ_Returns (IM3Function i_function)
{
    u16 ਅੰਕ_Returns = 0;

    ਜੇ (i_function) {
        ਜੇ (i_function->funcType) {
            ਅੰਕ_Returns = i_function->funcType->ਅੰਕ_Rets;
        }
    }

    ਵਾਪਸ ਅੰਕ_Returns;
}


u8 GetFunctionReturnType (ਅਚਲ IM3Function i_function, u16 i_ਸੂਚਕਾਂਕ)
{
    ਵਾਪਸ i_function ? Get_Func_Type_ਨਤੀਜਾ_Type(i_function->funcType, i_ਸੂਚਕਾਂਕ) : c_m3Type_unknown;
}


u32 Get_Function_ਅੰਕ_ਦਲੀਲਾਂ_And_Locals (IM3Function i_function)
{
    ਜੇ (i_function) {
        ਵਾਪਸ i_function->ਅੰਕ_Locals + Get_Function_ਅੰਕ_ਦਲੀਲਾਂ(i_function);
    } ਨਹੀਂਤਾਂ {
        ਵਾਪਸ 0;
    }
}
