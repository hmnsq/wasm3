//
//  m3_function.c
//
//  Created by Steven Massey on 4/7/21.
//  Copyright © 2021 Steven Massey. All rights reserved.
//

#شامل "m3_function.h"
#شامل "m3_env.h"


M3Result AllocFuncType (IM3FuncType* o_functionType, u32 i_رقم_Types)
{
    *o_functionType = (IM3FuncType)m3_Malloc("M3FuncType", اندازه(M3FuncType) + i_رقم_Types * اندازه(m3type_t));
    اگر (*o_functionType) {
        (*o_functionType)->isContinuation = false;
        (*o_functionType)->contFuncType   = پوچ;
        (*o_functionType)->inRecGroup     = false;
        بازگشت m3Err_none;
    }
    بازگشت m3Err_mallocFailed;
}


#اگر d_m3HasTypedRefs

// The type a (ref $t) / (ref null $t) naming this function type is spelled as
m3type_t RefTypeOfFuncType (ثابت IM3FuncType i_funcType, bool i_nonNull)
{
    m3type_t type = d_m3Type_ref | (i_funcType ? i_funcType->canonicalIndex : d_m3Type_heapAbstract);

    اگر (i_funcType and i_funcType->isContinuation) {
        type |= d_m3Type_refCont;
    }

    اگر (i_nonNull) {
        type |= d_m3Type_refNonNull;
    }

    بازگشت type;
}

#endif // d_m3HasTypedRefs


bool AreFuncTypesEqual (ثابت IM3FuncType i_typeA, ثابت IM3FuncType i_typeB)
{
    اگر (i_typeA->isContinuation != i_typeB->isContinuation) {
        بازگشت false;
    }
    اگر (i_typeA->isContinuation) {
        بازگشت (i_typeA->contFuncType == i_typeB->contFuncType) or
               (i_typeA->contFuncType and i_typeB->contFuncType and
                AreFuncTypesEqual(i_typeA->contFuncType, i_typeB->contFuncType));
    }
    اگر (i_typeA->رقم_Rets == i_typeB->رقم_Rets and i_typeA->رقم_Args == i_typeB->رقم_Args) {
        بازگشت (memcmp(i_typeA->types, i_typeB->types,
                       (i_typeA->رقم_Rets + i_typeA->رقم_Args) * اندازه(m3type_t)) == 0);
    }

    بازگشت false;
}

u16 Get_Func_Type_رقم_Params (ثابت IM3FuncType i_funcType)
{
    بازگشت i_funcType ? i_funcType->رقم_Args : 0;
}


m3type_t GetFuncTypeParamType (ثابت IM3FuncType i_funcType, u16 i_index)
{
    m3type_t type = c_m3Type_unknown;

    اگر (i_funcType) {
        اگر (i_index < i_funcType->رقم_Args) {
            type = i_funcType->types[i_funcType->رقم_Rets + i_index];
        }
    }

    بازگشت type;
}


u16 Get_Func_Type_رقم_Results (ثابت IM3FuncType i_funcType)
{
    بازگشت i_funcType ? i_funcType->رقم_Rets : 0;
}


m3type_t GetFuncTypeResultType (ثابت IM3FuncType i_funcType, u16 i_index)
{
    m3type_t type = c_m3Type_unknown;

    اگر (i_funcType) {
        اگر (i_index < i_funcType->رقم_Rets) {
            type = i_funcType->types[i_index];
        }
    }

    بازگشت type;
}


//---------------------------------------------------------------------------------------------------------------


تهی FreeImportInfo (M3ImportInfo* i_info)
{
    m3_Free(i_info->moduleUtf8);
    m3_Free(i_info->fieldUtf8);
}


#اگر d_m3HasSnapshots

تهی SnapshotMap_Free (M3SnapshotMap* i_map)
{
    اگر (i_map) {
        m3_Free(i_map->locals);
        m3_Free(i_map->safePoints);
        m3_Free(i_map->مقادیر);
        m3_Free(i_map->blocks);
        m3_Free(i_map);
    }
}

#endif

تهی Function_Release (IM3Function i_function)
{
    m3_Free(i_function->constants);

#اگر d_m3HasSnapshots
    SnapshotMap_Free(i_function->snapshotMap);
    i_function->snapshotMap = پوچ;
#endif

    برای (صحیح i = 0; i < i_function->رقم_Names; i++) {
        // name can be an alias of fieldUtf8
        اگر (i_function->names[i] != i_function->import.fieldUtf8) {
            m3_Free(i_function->names[i]);
        }
    }

    FreeImportInfo(&i_function->import);

    اگر (i_function->ownsWasmCode) {
        m3_Free(i_function->wasm);
    }

    // Function_FreeCompiledCode (func);

#اگر (d_m3EnableCodePageRefCounting)
    {
        m3_Free(i_function->codePageRefs);
        i_function->رقم_Code_Page_Refs = 0;
    }
#endif
}


تهی Function_FreeCompiledCode (IM3Function i_function)
{
#اگر (d_m3EnableCodePageRefCounting)
    {
        i_function->compiled = پوچ;

        وقتی (i_function->رقم_Code_Page_Refs--) {
            IM3CodePage page = i_function->codePageRefs[i_function->رقم_Code_Page_Refs];

            اگر (--(page->info.usage_شمار) == 0) {
                //                printf ("free %p\n", page);
            }
        }

        m3_Free(i_function->codePageRefs);

        Runtime_ReleaseCodePages(i_function->module->runtime);
    }
#endif
}


cstr_t m3_GetFunctionName (IM3Function i_function)
{
    u16     رقم_Names = 0;
    cstr_t* names    = GetFunctionNames(i_function, &رقم_Names);
    اگر (رقم_Names > 0) {
        بازگشت names[0];
    } وگرنه {
        بازگشت "<unnamed>";
    }
}


IM3Module m3_GetFunctionModule (IM3Function i_function)
{
    بازگشت i_function ? i_function->module : پوچ;
}


cstr_t* GetFunctionNames (IM3Function i_function, u16* o_رقم_Names)
{
    اگر (!i_function || !o_رقم_Names) {
        بازگشت پوچ;
    }

    اگر (i_function->import.fieldUtf8) {
        *o_رقم_Names = 1;
        بازگشت &i_function->import.fieldUtf8;
    } وگرنه {
        *o_رقم_Names = i_function->رقم_Names;
        بازگشت i_function->names;
    }
}


cstr_t GetFunctionImportModuleName (IM3Function i_function)
{
    بازگشت (i_function->import.moduleUtf8) ? i_function->import.moduleUtf8 : "";
}


u16 Get_Function_رقم_Args (IM3Function i_function)
{
    u16 رقم_Args = 0;

    اگر (i_function) {
        اگر (i_function->funcType) {
            رقم_Args = i_function->funcType->رقم_Args;
        }
    }

    بازگشت رقم_Args;
}

m3type_t GetFunctionArgType (IM3Function i_function, u32 i_index)
{
    m3type_t type = c_m3Type_none;

    اگر (i_index < Get_Function_رقم_Args(i_function)) {
        u32 رقم_Returns = i_function->funcType->رقم_Rets;

        type = i_function->funcType->types[رقم_Returns + i_index];
    }

    بازگشت type;
}


u16 Get_Function_رقم_Returns (IM3Function i_function)
{
    u16 رقم_Returns = 0;

    اگر (i_function) {
        اگر (i_function->funcType) {
            رقم_Returns = i_function->funcType->رقم_Rets;
        }
    }

    بازگشت رقم_Returns;
}


u8 GetFunctionReturnType (ثابت IM3Function i_function, u16 i_index)
{
    بازگشت i_function ? GetFuncTypeResultType(i_function->funcType, i_index) : c_m3Type_unknown;
}


u32 Get_Function_رقم_Args_And_Locals (IM3Function i_function)
{
    اگر (i_function) {
        بازگشت i_function->رقم_Locals + Get_Function_رقم_Args(i_function);
    } وگرنه {
        بازگشت 0;
    }
}
