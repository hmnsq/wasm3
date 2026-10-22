//
//  m3_function.c
//
//  Created by Steven Massey on 4/7/21.
//  Copyright © 2021 Steven Massey. All rights reserved.
//

#अन्तर्भाव "m3_function.h"
#अन्तर्भाव "m3_env.h"


M3Result AllocFuncType (IM3FuncType* o_functionType, u32 i_अङ्क_Types)
{
    *o_functionType = (IM3FuncType)m3_Malloc("M3FuncType", परिमाण(M3FuncType) + i_अङ्क_Types * परिमाण(m3type_t));
    यदि (*o_functionType) {
        (*o_functionType)->isContinuation = false;
        (*o_functionType)->contFuncType   = रिक्त;
        (*o_functionType)->inRecGroup     = false;
        फर्काऊ m3Err_none;
    }
    फर्काऊ m3Err_mallocFailed;
}


#यदि d_m3HasTypedRefs

// The type a (ref $t) / (ref null $t) naming this function type is spelled as
m3type_t RefTypeOfFuncType (अचर IM3FuncType i_funcType, bool i_nonNull)
{
    m3type_t type = d_m3Type_ref | (i_funcType ? i_funcType->canonicalIndex : d_m3Type_heapAbstract);

    यदि (i_funcType and i_funcType->isContinuation) {
        type |= d_m3Type_refCont;
    }

    यदि (i_nonNull) {
        type |= d_m3Type_refNonNull;
    }

    फर्काऊ type;
}

#endif // d_m3HasTypedRefs


bool AreFuncTypesEqual (अचर IM3FuncType i_typeA, अचर IM3FuncType i_typeB)
{
    यदि (i_typeA->isContinuation != i_typeB->isContinuation) {
        फर्काऊ false;
    }
    यदि (i_typeA->isContinuation) {
        फर्काऊ (i_typeA->contFuncType == i_typeB->contFuncType) or
               (i_typeA->contFuncType and i_typeB->contFuncType and
                AreFuncTypesEqual(i_typeA->contFuncType, i_typeB->contFuncType));
    }
    यदि (i_typeA->अङ्क_Rets == i_typeB->अङ्क_Rets and i_typeA->अङ्क_Args == i_typeB->अङ्क_Args) {
        फर्काऊ (स_भेद(i_typeA->types, i_typeB->types,
                       (i_typeA->अङ्क_Rets + i_typeA->अङ्क_Args) * परिमाण(m3type_t)) == 0);
    }

    फर्काऊ false;
}

u16 Get_Func_Type_अङ्क_Params (अचर IM3FuncType i_funcType)
{
    फर्काऊ i_funcType ? i_funcType->अङ्क_Args : 0;
}


m3type_t GetFuncTypeParamType (अचर IM3FuncType i_funcType, u16 i_index)
{
    m3type_t type = c_m3Type_unknown;

    यदि (i_funcType) {
        यदि (i_index < i_funcType->अङ्क_Args) {
            type = i_funcType->types[i_funcType->अङ्क_Rets + i_index];
        }
    }

    फर्काऊ type;
}


u16 Get_Func_Type_अङ्क_Results (अचर IM3FuncType i_funcType)
{
    फर्काऊ i_funcType ? i_funcType->अङ्क_Rets : 0;
}


m3type_t GetFuncTypeResultType (अचर IM3FuncType i_funcType, u16 i_index)
{
    m3type_t type = c_m3Type_unknown;

    यदि (i_funcType) {
        यदि (i_index < i_funcType->अङ्क_Rets) {
            type = i_funcType->types[i_index];
        }
    }

    फर्काऊ type;
}


//---------------------------------------------------------------------------------------------------------------


शून्य FreeImportInfo (M3ImportInfo* i_info)
{
    m3_Free(i_info->moduleUtf8);
    m3_Free(i_info->fieldUtf8);
}


#यदि d_m3HasSnapshots

शून्य SnapshotMap_Free (M3SnapshotMap* i_map)
{
    यदि (i_map) {
        m3_Free(i_map->locals);
        m3_Free(i_map->safePoints);
        m3_Free(i_map->मानहरू);
        m3_Free(i_map->blocks);
        m3_Free(i_map);
    }
}

#endif

शून्य Function_Release (IM3Function i_function)
{
    m3_Free(i_function->constants);

#यदि d_m3HasSnapshots
    SnapshotMap_Free(i_function->snapshotMap);
    i_function->snapshotMap = रिक्त;
#endif

    लागि (पूर्णाङ्क i = 0; i < i_function->अङ्क_Names; i++) {
        // name can be an alias of fieldUtf8
        यदि (i_function->names[i] != i_function->import.fieldUtf8) {
            m3_Free(i_function->names[i]);
        }
    }

    FreeImportInfo(&i_function->import);

    यदि (i_function->ownsWasmCode) {
        m3_Free(i_function->wasm);
    }

    // Function_FreeCompiledCode (func);

#यदि (d_m3EnableCodePageRefCounting)
    {
        m3_Free(i_function->codePageRefs);
        i_function->अङ्क_Code_Page_Refs = 0;
    }
#endif
}


शून्य Function_FreeCompiledCode (IM3Function i_function)
{
#यदि (d_m3EnableCodePageRefCounting)
    {
        i_function->compiled = रिक्त;

        जबसम्म (i_function->अङ्क_Code_Page_Refs--) {
            IM3CodePage page = i_function->codePageRefs[i_function->अङ्क_Code_Page_Refs];

            यदि (--(page->info.usage_गन्ती) == 0) {
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
    u16     अङ्क_Names = 0;
    cstr_t* names    = GetFunctionNames(i_function, &अङ्क_Names);
    यदि (अङ्क_Names > 0) {
        फर्काऊ names[0];
    } अन्यथा {
        फर्काऊ "<unnamed>";
    }
}


IM3Module m3_GetFunctionModule (IM3Function i_function)
{
    फर्काऊ i_function ? i_function->module : रिक्त;
}


cstr_t* GetFunctionNames (IM3Function i_function, u16* o_अङ्क_Names)
{
    यदि (!i_function || !o_अङ्क_Names) {
        फर्काऊ रिक्त;
    }

    यदि (i_function->import.fieldUtf8) {
        *o_अङ्क_Names = 1;
        फर्काऊ &i_function->import.fieldUtf8;
    } अन्यथा {
        *o_अङ्क_Names = i_function->अङ्क_Names;
        फर्काऊ i_function->names;
    }
}


cstr_t GetFunctionImportModuleName (IM3Function i_function)
{
    फर्काऊ (i_function->import.moduleUtf8) ? i_function->import.moduleUtf8 : "";
}


u16 Get_Function_अङ्क_Args (IM3Function i_function)
{
    u16 अङ्क_Args = 0;

    यदि (i_function) {
        यदि (i_function->funcType) {
            अङ्क_Args = i_function->funcType->अङ्क_Args;
        }
    }

    फर्काऊ अङ्क_Args;
}

m3type_t GetFunctionArgType (IM3Function i_function, u32 i_index)
{
    m3type_t type = c_m3Type_none;

    यदि (i_index < Get_Function_अङ्क_Args(i_function)) {
        u32 अङ्क_Returns = i_function->funcType->अङ्क_Rets;

        type = i_function->funcType->types[अङ्क_Returns + i_index];
    }

    फर्काऊ type;
}


u16 Get_Function_अङ्क_Returns (IM3Function i_function)
{
    u16 अङ्क_Returns = 0;

    यदि (i_function) {
        यदि (i_function->funcType) {
            अङ्क_Returns = i_function->funcType->अङ्क_Rets;
        }
    }

    फर्काऊ अङ्क_Returns;
}


u8 GetFunctionReturnType (अचर IM3Function i_function, u16 i_index)
{
    फर्काऊ i_function ? GetFuncTypeResultType(i_function->funcType, i_index) : c_m3Type_unknown;
}


u32 Get_Function_अङ्क_Args_And_Locals (IM3Function i_function)
{
    यदि (i_function) {
        फर्काऊ i_function->अङ्क_Locals + Get_Function_अङ्क_Args(i_function);
    } अन्यथा {
        फर्काऊ 0;
    }
}
