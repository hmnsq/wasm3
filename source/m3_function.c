//
//  m3_function.c
//
//  Created by Steven Massey on 4/7/21.
//  Copyright © 2021 Steven Massey. All rights reserved.
//

#समावेश "m3_function.h"
#समावेश "m3_env.h"


M3_परिणाम AllocFuncType (IM3FuncType* o_functionType, u32 i_अंक_Types)
{
    *o_functionType = (IM3FuncType)m3_Malloc("M3FuncType", माप(M3FuncType) + i_अंक_Types * माप(m3type_t));
    अगर (*o_functionType) {
        (*o_functionType)->isContinuation = false;
        (*o_functionType)->contFuncType   = शून्य;
        (*o_functionType)->inRecGroup     = false;
        वापस m3Err_none;
    }
    वापस m3Err_mallocFailed;
}


#अगर d_m3HasTypedRefs

// The type a (ref $t) / (ref null $t) naming this function type is spelled as
m3type_t RefTypeOfFuncType (स्थिर IM3FuncType i_funcType, bool i_nonNull)
{
    m3type_t type = d_m3Type_ref | (i_funcType ? i_funcType->canonical_सूचकांक : d_m3Type_heapAbstract);

    अगर (i_funcType and i_funcType->isContinuation) {
        type |= d_m3Type_refCont;
    }

    अगर (i_nonNull) {
        type |= d_m3Type_refNonNull;
    }

    वापस type;
}

#पूर्ण_अगर // d_m3HasTypedRefs


bool AreFuncTypesEqual (स्थिर IM3FuncType i_typeA, स्थिर IM3FuncType i_typeB)
{
    अगर (i_typeA->isContinuation != i_typeB->isContinuation) {
        वापस false;
    }
    अगर (i_typeA->isContinuation) {
        वापस (i_typeA->contFuncType == i_typeB->contFuncType) or
               (i_typeA->contFuncType and i_typeB->contFuncType and
                AreFuncTypesEqual(i_typeA->contFuncType, i_typeB->contFuncType));
    }
    अगर (i_typeA->अंक_Rets == i_typeB->अंक_Rets and i_typeA->अंक_Args == i_typeB->अंक_Args) {
        वापस (स_भेद(i_typeA->types, i_typeB->types,
                       (i_typeA->अंक_Rets + i_typeA->अंक_Args) * माप(m3type_t)) == 0);
    }

    वापस false;
}

u16 Get_Func_Type_अंक_Params (स्थिर IM3FuncType i_funcType)
{
    वापस i_funcType ? i_funcType->अंक_Args : 0;
}


m3type_t GetFuncTypeParamType (स्थिर IM3FuncType i_funcType, u16 i_सूचकांक)
{
    m3type_t type = c_m3Type_unknown;

    अगर (i_funcType) {
        अगर (i_सूचकांक < i_funcType->अंक_Args) {
            type = i_funcType->types[i_funcType->अंक_Rets + i_सूचकांक];
        }
    }

    वापस type;
}


u16 Get_Func_Type_अंक_Results (स्थिर IM3FuncType i_funcType)
{
    वापस i_funcType ? i_funcType->अंक_Rets : 0;
}


m3type_t Get_Func_Type_परिणाम_Type (स्थिर IM3FuncType i_funcType, u16 i_सूचकांक)
{
    m3type_t type = c_m3Type_unknown;

    अगर (i_funcType) {
        अगर (i_सूचकांक < i_funcType->अंक_Rets) {
            type = i_funcType->types[i_सूचकांक];
        }
    }

    वापस type;
}


//---------------------------------------------------------------------------------------------------------------


व्योम FreeImportInfo (M3ImportInfo* i_info)
{
    m3_Free(i_info->moduleUtf8);
    m3_Free(i_info->fieldUtf8);
}


#अगर d_m3HasSnapshots

व्योम SnapshotMap_Free (M3SnapshotMap* i_map)
{
    अगर (i_map) {
        m3_Free(i_map->locals);
        m3_Free(i_map->safePoints);
        m3_Free(i_map->मूल्यावली);
        m3_Free(i_map->blocks);
        m3_Free(i_map);
    }
}

#पूर्ण_अगर

व्योम Function_Release (IM3Function i_function)
{
    m3_Free(i_function->constants);

#अगर d_m3HasSnapshots
    SnapshotMap_Free(i_function->snapshotMap);
    i_function->snapshotMap = शून्य;
#पूर्ण_अगर

    क्रम (पूर्णांक i = 0; i < i_function->अंक_Names; i++) {
        // name can be an alias of fieldUtf8
        अगर (i_function->names[i] != i_function->import.fieldUtf8) {
            m3_Free(i_function->names[i]);
        }
    }

    FreeImportInfo(&i_function->import);

    अगर (i_function->ownsWasmCode) {
        m3_Free(i_function->wasm);
    }

    // Function_FreeCompiledCode (func);

#अगर (d_m3EnableCodePageRefCounting)
    {
        m3_Free(i_function->codePageRefs);
        i_function->अंक_Code_Page_Refs = 0;
    }
#पूर्ण_अगर
}


व्योम Function_FreeCompiledCode (IM3Function i_function)
{
#अगर (d_m3EnableCodePageRefCounting)
    {
        i_function->compiled = शून्य;

        जबतक (i_function->अंक_Code_Page_Refs--) {
            IM3CodePage page = i_function->codePageRefs[i_function->अंक_Code_Page_Refs];

            अगर (--(page->info.usage_गिनती) == 0) {
                //                printf ("free %p\n", page);
            }
        }

        m3_Free(i_function->codePageRefs);

        Runtime_ReleaseCodePages(i_function->module->runtime);
    }
#पूर्ण_अगर
}


cstr_t m3_GetFunctionName (IM3Function i_function)
{
    u16     अंक_Names = 0;
    cstr_t* names    = GetFunctionNames(i_function, &अंक_Names);
    अगर (अंक_Names > 0) {
        वापस names[0];
    } अन्यथा {
        वापस "<unnamed>";
    }
}


IM3Module m3_GetFunctionModule (IM3Function i_function)
{
    वापस i_function ? i_function->module : शून्य;
}


cstr_t* GetFunctionNames (IM3Function i_function, u16* o_अंक_Names)
{
    अगर (!i_function || !o_अंक_Names) {
        वापस शून्य;
    }

    अगर (i_function->import.fieldUtf8) {
        *o_अंक_Names = 1;
        वापस &i_function->import.fieldUtf8;
    } अन्यथा {
        *o_अंक_Names = i_function->अंक_Names;
        वापस i_function->names;
    }
}


cstr_t GetFunctionImportModuleName (IM3Function i_function)
{
    वापस (i_function->import.moduleUtf8) ? i_function->import.moduleUtf8 : "";
}


u16 Get_Function_अंक_Args (IM3Function i_function)
{
    u16 अंक_Args = 0;

    अगर (i_function) {
        अगर (i_function->funcType) {
            अंक_Args = i_function->funcType->अंक_Args;
        }
    }

    वापस अंक_Args;
}

m3type_t GetFunctionArgType (IM3Function i_function, u32 i_सूचकांक)
{
    m3type_t type = c_m3Type_none;

    अगर (i_सूचकांक < Get_Function_अंक_Args(i_function)) {
        u32 अंक_Returns = i_function->funcType->अंक_Rets;

        type = i_function->funcType->types[अंक_Returns + i_सूचकांक];
    }

    वापस type;
}


u16 Get_Function_अंक_Returns (IM3Function i_function)
{
    u16 अंक_Returns = 0;

    अगर (i_function) {
        अगर (i_function->funcType) {
            अंक_Returns = i_function->funcType->अंक_Rets;
        }
    }

    वापस अंक_Returns;
}


u8 GetFunctionReturnType (स्थिर IM3Function i_function, u16 i_सूचकांक)
{
    वापस i_function ? Get_Func_Type_परिणाम_Type(i_function->funcType, i_सूचकांक) : c_m3Type_unknown;
}


u32 Get_Function_अंक_Args_And_Locals (IM3Function i_function)
{
    अगर (i_function) {
        वापस i_function->अंक_Locals + Get_Function_अंक_Args(i_function);
    } अन्यथा {
        वापस 0;
    }
}
