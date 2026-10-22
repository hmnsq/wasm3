//
//  m3_function.c
//
//  Created by Steven Massey on 4/7/21.
//  Copyright © 2021 Steven Massey. All rights reserved.
//

#अन्तर्भाव "m3_function.h"
#अन्तर्भाव "m3_env.h"


M3Result AllocFuncType (IM3FuncType* o_functionType, u32 i_अंक_Types)
{
    *o_functionType = (IM3FuncType)m3_Malloc("M3FuncType", आकार(M3FuncType) + i_अंक_Types * आकार(m3type_t));
    जर (*o_functionType) {
        (*o_functionType)->isContinuation = false;
        (*o_functionType)->contFuncType   = रिक्त;
        (*o_functionType)->inRecGroup     = false;
        परत m3Err_none;
    }
    परत m3Err_mallocFailed;
}


#जर d_m3HasTypedRefs

// The type a (ref $t) / (ref null $t) naming this function type is spelled as
m3type_t RefTypeOfFuncType (स्थिरांक IM3FuncType i_funcType, bool i_nonNull)
{
    m3type_t type = d_m3Type_ref | (i_funcType ? i_funcType->canonicalIndex : d_m3Type_heapAbstract);

    जर (i_funcType and i_funcType->isContinuation) {
        type |= d_m3Type_refCont;
    }

    जर (i_nonNull) {
        type |= d_m3Type_refNonNull;
    }

    परत type;
}

#endif // d_m3HasTypedRefs


bool AreFuncTypesEqual (स्थिरांक IM3FuncType i_typeA, स्थिरांक IM3FuncType i_typeB)
{
    जर (i_typeA->isContinuation != i_typeB->isContinuation) {
        परत false;
    }
    जर (i_typeA->isContinuation) {
        परत (i_typeA->contFuncType == i_typeB->contFuncType) or
               (i_typeA->contFuncType and i_typeB->contFuncType and
                AreFuncTypesEqual(i_typeA->contFuncType, i_typeB->contFuncType));
    }
    जर (i_typeA->अंक_Rets == i_typeB->अंक_Rets and i_typeA->अंक_Args == i_typeB->अंक_Args) {
        परत (स_भेद(i_typeA->types, i_typeB->types,
                       (i_typeA->अंक_Rets + i_typeA->अंक_Args) * आकार(m3type_t)) == 0);
    }

    परत false;
}

u16 Get_Func_Type_अंक_Params (स्थिरांक IM3FuncType i_funcType)
{
    परत i_funcType ? i_funcType->अंक_Args : 0;
}


m3type_t GetFuncTypeParamType (स्थिरांक IM3FuncType i_funcType, u16 i_index)
{
    m3type_t type = c_m3Type_unknown;

    जर (i_funcType) {
        जर (i_index < i_funcType->अंक_Args) {
            type = i_funcType->types[i_funcType->अंक_Rets + i_index];
        }
    }

    परत type;
}


u16 Get_Func_Type_अंक_Results (स्थिरांक IM3FuncType i_funcType)
{
    परत i_funcType ? i_funcType->अंक_Rets : 0;
}


m3type_t GetFuncTypeResultType (स्थिरांक IM3FuncType i_funcType, u16 i_index)
{
    m3type_t type = c_m3Type_unknown;

    जर (i_funcType) {
        जर (i_index < i_funcType->अंक_Rets) {
            type = i_funcType->types[i_index];
        }
    }

    परत type;
}


//---------------------------------------------------------------------------------------------------------------


शून्य FreeImportInfo (M3ImportInfo* i_info)
{
    m3_Free(i_info->moduleUtf8);
    m3_Free(i_info->fieldUtf8);
}


#जर d_m3HasSnapshots

शून्य SnapshotMap_Free (M3SnapshotMap* i_map)
{
    जर (i_map) {
        m3_Free(i_map->locals);
        m3_Free(i_map->safePoints);
        m3_Free(i_map->मूल्ये);
        m3_Free(i_map->blocks);
        m3_Free(i_map);
    }
}

#endif

शून्य Function_Release (IM3Function i_function)
{
    m3_Free(i_function->constants);

#जर d_m3HasSnapshots
    SnapshotMap_Free(i_function->snapshotMap);
    i_function->snapshotMap = रिक्त;
#endif

    साठी (पूर्णांक i = 0; i < i_function->अंक_Names; i++) {
        // name can be an alias of fieldUtf8
        जर (i_function->names[i] != i_function->import.fieldUtf8) {
            m3_Free(i_function->names[i]);
        }
    }

    FreeImportInfo(&i_function->import);

    जर (i_function->ownsWasmCode) {
        m3_Free(i_function->wasm);
    }

    // Function_FreeCompiledCode (func);

#जर (d_m3EnableCodePageRefCounting)
    {
        m3_Free(i_function->codePageRefs);
        i_function->अंक_Code_Page_Refs = 0;
    }
#endif
}


शून्य Function_FreeCompiledCode (IM3Function i_function)
{
#जर (d_m3EnableCodePageRefCounting)
    {
        i_function->compiled = रिक्त;

        पर्यंत (i_function->अंक_Code_Page_Refs--) {
            IM3CodePage page = i_function->codePageRefs[i_function->अंक_Code_Page_Refs];

            जर (--(page->info.usage_मोजणी) == 0) {
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
    u16     अंक_Names = 0;
    cstr_t* names    = GetFunctionNames(i_function, &अंक_Names);
    जर (अंक_Names > 0) {
        परत names[0];
    } अन्यथा {
        परत "<unnamed>";
    }
}


IM3Module m3_GetFunctionModule (IM3Function i_function)
{
    परत i_function ? i_function->module : रिक्त;
}


cstr_t* GetFunctionNames (IM3Function i_function, u16* o_अंक_Names)
{
    जर (!i_function || !o_अंक_Names) {
        परत रिक्त;
    }

    जर (i_function->import.fieldUtf8) {
        *o_अंक_Names = 1;
        परत &i_function->import.fieldUtf8;
    } अन्यथा {
        *o_अंक_Names = i_function->अंक_Names;
        परत i_function->names;
    }
}


cstr_t GetFunctionImportModuleName (IM3Function i_function)
{
    परत (i_function->import.moduleUtf8) ? i_function->import.moduleUtf8 : "";
}


u16 Get_Function_अंक_Args (IM3Function i_function)
{
    u16 अंक_Args = 0;

    जर (i_function) {
        जर (i_function->funcType) {
            अंक_Args = i_function->funcType->अंक_Args;
        }
    }

    परत अंक_Args;
}

m3type_t GetFunctionArgType (IM3Function i_function, u32 i_index)
{
    m3type_t type = c_m3Type_none;

    जर (i_index < Get_Function_अंक_Args(i_function)) {
        u32 अंक_Returns = i_function->funcType->अंक_Rets;

        type = i_function->funcType->types[अंक_Returns + i_index];
    }

    परत type;
}


u16 Get_Function_अंक_Returns (IM3Function i_function)
{
    u16 अंक_Returns = 0;

    जर (i_function) {
        जर (i_function->funcType) {
            अंक_Returns = i_function->funcType->अंक_Rets;
        }
    }

    परत अंक_Returns;
}


u8 GetFunctionReturnType (स्थिरांक IM3Function i_function, u16 i_index)
{
    परत i_function ? GetFuncTypeResultType(i_function->funcType, i_index) : c_m3Type_unknown;
}


u32 Get_Function_अंक_Args_And_Locals (IM3Function i_function)
{
    जर (i_function) {
        परत i_function->अंक_Locals + Get_Function_अंक_Args(i_function);
    } अन्यथा {
        परत 0;
    }
}
