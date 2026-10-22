//
//  m3_function.c
//
//  Created by Steven Massey on 4/7/21.
//  Copyright © 2021 Steven Massey. All rights reserved.
//

#अन्तर्भाव "m3_function.h"
#अन्तर्भाव "m3_env.h"


M3Result AllocFuncType (IM3FuncType* o_functionType, u32 i_अङ्कः_Types)
{
    *o_functionType = (IM3FuncType)m3_Malloc("M3FuncType", परिमाण(M3FuncType) + i_अङ्कः_Types * परिमाण(m3type_t));
    सचे (*o_functionType) {
        (*o_functionType)->isContinuation = false;
        (*o_functionType)->contFuncType   = रिक्त;
        (*o_functionType)->inRecGroup     = false;
        पटिनिवत्त m3Err_none;
    }
    पटिनिवत्त m3Err_mallocFailed;
}


#सचे d_m3HasTypedRefs

// The type a (ref $t) / (ref null $t) naming this function type is spelled as
m3type_t RefTypeOfFuncType (ध्रुव IM3FuncType i_funcType, bool i_nonNull)
{
    m3type_t type = d_m3Type_ref | (i_funcType ? i_funcType->canonicalIndex : d_m3Type_heapAbstract);

    सचे (i_funcType and i_funcType->isContinuation) {
        type |= d_m3Type_refCont;
    }

    सचे (i_nonNull) {
        type |= d_m3Type_refNonNull;
    }

    पटिनिवत्त type;
}

#endif // d_m3HasTypedRefs


bool AreFuncTypesEqual (ध्रुव IM3FuncType i_typeA, ध्रुव IM3FuncType i_typeB)
{
    सचे (i_typeA->isContinuation != i_typeB->isContinuation) {
        पटिनिवत्त false;
    }
    सचे (i_typeA->isContinuation) {
        पटिनिवत्त (i_typeA->contFuncType == i_typeB->contFuncType) or
               (i_typeA->contFuncType and i_typeB->contFuncType and
                AreFuncTypesEqual(i_typeA->contFuncType, i_typeB->contFuncType));
    }
    सचे (i_typeA->अङ्कः_Rets == i_typeB->अङ्कः_Rets and i_typeA->अङ्कः_Args == i_typeB->अङ्कः_Args) {
        पटिनिवत्त (memcmp(i_typeA->types, i_typeB->types,
                       (i_typeA->अङ्कः_Rets + i_typeA->अङ्कः_Args) * परिमाण(m3type_t)) == 0);
    }

    पटिनिवत्त false;
}

u16 Get_Func_Type_अङ्कः_Params (ध्रुव IM3FuncType i_funcType)
{
    पटिनिवत्त i_funcType ? i_funcType->अङ्कः_Args : 0;
}


m3type_t GetFuncTypeParamType (ध्रुव IM3FuncType i_funcType, u16 i_index)
{
    m3type_t type = c_m3Type_unknown;

    सचे (i_funcType) {
        सचे (i_index < i_funcType->अङ्कः_Args) {
            type = i_funcType->types[i_funcType->अङ्कः_Rets + i_index];
        }
    }

    पटिनिवत्त type;
}


u16 Get_Func_Type_अङ्कः_Results (ध्रुव IM3FuncType i_funcType)
{
    पटिनिवत्त i_funcType ? i_funcType->अङ्कः_Rets : 0;
}


m3type_t GetFuncTypeResultType (ध्रुव IM3FuncType i_funcType, u16 i_index)
{
    m3type_t type = c_m3Type_unknown;

    सचे (i_funcType) {
        सचे (i_index < i_funcType->अङ्कः_Rets) {
            type = i_funcType->types[i_index];
        }
    }

    पटिनिवत्त type;
}


//---------------------------------------------------------------------------------------------------------------


शून्य FreeImportInfo (M3ImportInfo* i_info)
{
    m3_Free(i_info->moduleUtf8);
    m3_Free(i_info->fieldUtf8);
}


#सचे d_m3HasSnapshots

शून्य SnapshotMap_Free (M3SnapshotMap* i_map)
{
    सचे (i_map) {
        m3_Free(i_map->locals);
        m3_Free(i_map->safePoints);
        m3_Free(i_map->मानानि);
        m3_Free(i_map->blocks);
        m3_Free(i_map);
    }
}

#endif

शून्य Function_Release (IM3Function i_function)
{
    m3_Free(i_function->constants);

#सचे d_m3HasSnapshots
    SnapshotMap_Free(i_function->snapshotMap);
    i_function->snapshotMap = रिक्त;
#endif

    अत्थाय (पूर्णाङ्क i = 0; i < i_function->अङ्कः_Names; i++) {
        // name can be an alias of fieldUtf8
        सचे (i_function->names[i] != i_function->import.fieldUtf8) {
            m3_Free(i_function->names[i]);
        }
    }

    FreeImportInfo(&i_function->import);

    सचे (i_function->ownsWasmCode) {
        m3_Free(i_function->wasm);
    }

    // Function_FreeCompiledCode (func);

#सचे (d_m3EnableCodePageRefCounting)
    {
        m3_Free(i_function->codePageRefs);
        i_function->अङ्कः_Code_Page_Refs = 0;
    }
#endif
}


शून्य Function_FreeCompiledCode (IM3Function i_function)
{
#सचे (d_m3EnableCodePageRefCounting)
    {
        i_function->compiled = रिक्त;

        याव (i_function->अङ्कः_Code_Page_Refs--) {
            IM3CodePage page = i_function->codePageRefs[i_function->अङ्कः_Code_Page_Refs];

            सचे (--(page->info.usage_गणनम्) == 0) {
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
    u16     अङ्कः_Names = 0;
    cstr_t* names    = GetFunctionNames(i_function, &अङ्कः_Names);
    सचे (अङ्कः_Names > 0) {
        पटिनिवत्त names[0];
    } अञ्ञथा {
        पटिनिवत्त "<unnamed>";
    }
}


IM3Module m3_GetFunctionModule (IM3Function i_function)
{
    पटिनिवत्त i_function ? i_function->module : रिक्त;
}


cstr_t* GetFunctionNames (IM3Function i_function, u16* o_अङ्कः_Names)
{
    सचे (!i_function || !o_अङ्कः_Names) {
        पटिनिवत्त रिक्त;
    }

    सचे (i_function->import.fieldUtf8) {
        *o_अङ्कः_Names = 1;
        पटिनिवत्त &i_function->import.fieldUtf8;
    } अञ्ञथा {
        *o_अङ्कः_Names = i_function->अङ्कः_Names;
        पटिनिवत्त i_function->names;
    }
}


cstr_t GetFunctionImportModuleName (IM3Function i_function)
{
    पटिनिवत्त (i_function->import.moduleUtf8) ? i_function->import.moduleUtf8 : "";
}


u16 Get_Function_अङ्कः_Args (IM3Function i_function)
{
    u16 अङ्कः_Args = 0;

    सचे (i_function) {
        सचे (i_function->funcType) {
            अङ्कः_Args = i_function->funcType->अङ्कः_Args;
        }
    }

    पटिनिवत्त अङ्कः_Args;
}

m3type_t GetFunctionArgType (IM3Function i_function, u32 i_index)
{
    m3type_t type = c_m3Type_none;

    सचे (i_index < Get_Function_अङ्कः_Args(i_function)) {
        u32 अङ्कः_Returns = i_function->funcType->अङ्कः_Rets;

        type = i_function->funcType->types[अङ्कः_Returns + i_index];
    }

    पटिनिवत्त type;
}


u16 Get_Function_अङ्कः_Returns (IM3Function i_function)
{
    u16 अङ्कः_Returns = 0;

    सचे (i_function) {
        सचे (i_function->funcType) {
            अङ्कः_Returns = i_function->funcType->अङ्कः_Rets;
        }
    }

    पटिनिवत्त अङ्कः_Returns;
}


u8 GetFunctionReturnType (ध्रुव IM3Function i_function, u16 i_index)
{
    पटिनिवत्त i_function ? GetFuncTypeResultType(i_function->funcType, i_index) : c_m3Type_unknown;
}


u32 Get_Function_अङ्कः_Args_And_Locals (IM3Function i_function)
{
    सचे (i_function) {
        पटिनिवत्त i_function->अङ्कः_Locals + Get_Function_अङ्कः_Args(i_function);
    } अञ्ञथा {
        पटिनिवत्त 0;
    }
}
