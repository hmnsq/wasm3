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
    *o_functionType = (IM3FuncType)m3_Malloc("M3FuncType", परिमाण(M3FuncType) + i_अंक_Types * परिमाण(m3type_t));
    यदि (*o_functionType) {
        (*o_functionType)->isContinuation = false;
        (*o_functionType)->contFuncType   = रिक्त;
        (*o_functionType)->inRecGroup     = false;
        प्रत्यागम m3Err_none;
    }
    प्रत्यागम m3Err_mallocFailed;
}


#यदि d_m3HasTypedRefs

// The type a (ref $t) / (ref null $t) naming this function type is spelled as
m3type_t RefTypeOfFuncType (ध्रुव IM3FuncType i_funcType, bool i_nonNull)
{
    m3type_t type = d_m3Type_ref | (i_funcType ? i_funcType->canonicalIndex : d_m3Type_heapAbstract);

    यदि (i_funcType and i_funcType->isContinuation) {
        type |= d_m3Type_refCont;
    }

    यदि (i_nonNull) {
        type |= d_m3Type_refNonNull;
    }

    प्रत्यागम type;
}

#endif // d_m3HasTypedRefs


bool AreFuncTypesEqual (ध्रुव IM3FuncType i_typeA, ध्रुव IM3FuncType i_typeB)
{
    यदि (i_typeA->isContinuation != i_typeB->isContinuation) {
        प्रत्यागम false;
    }
    यदि (i_typeA->isContinuation) {
        प्रत्यागम (i_typeA->contFuncType == i_typeB->contFuncType) or
               (i_typeA->contFuncType and i_typeB->contFuncType and
                AreFuncTypesEqual(i_typeA->contFuncType, i_typeB->contFuncType));
    }
    यदि (i_typeA->अंक_Rets == i_typeB->अंक_Rets and i_typeA->अंक_Args == i_typeB->अंक_Args) {
        प्रत्यागम (स_भेद(i_typeA->types, i_typeB->types,
                       (i_typeA->अंक_Rets + i_typeA->अंक_Args) * परिमाण(m3type_t)) == 0);
    }

    प्रत्यागम false;
}

u16 Get_Func_Type_अंक_Params (ध्रुव IM3FuncType i_funcType)
{
    प्रत्यागम i_funcType ? i_funcType->अंक_Args : 0;
}


m3type_t GetFuncTypeParamType (ध्रुव IM3FuncType i_funcType, u16 i_index)
{
    m3type_t type = c_m3Type_unknown;

    यदि (i_funcType) {
        यदि (i_index < i_funcType->अंक_Args) {
            type = i_funcType->types[i_funcType->अंक_Rets + i_index];
        }
    }

    प्रत्यागम type;
}


u16 Get_Func_Type_अंक_Results (ध्रुव IM3FuncType i_funcType)
{
    प्रत्यागम i_funcType ? i_funcType->अंक_Rets : 0;
}


m3type_t GetFuncTypeResultType (ध्रुव IM3FuncType i_funcType, u16 i_index)
{
    m3type_t type = c_m3Type_unknown;

    यदि (i_funcType) {
        यदि (i_index < i_funcType->अंक_Rets) {
            type = i_funcType->types[i_index];
        }
    }

    प्रत्यागम type;
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
        m3_Free(i_map->मूल्यावली);
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

    प्रति (पूर्णाङ्क i = 0; i < i_function->अंक_Names; i++) {
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
        i_function->अंक_Code_Page_Refs = 0;
    }
#endif
}


शून्य Function_FreeCompiledCode (IM3Function i_function)
{
#यदि (d_m3EnableCodePageRefCounting)
    {
        i_function->compiled = रिक्त;

        यावत् (i_function->अंक_Code_Page_Refs--) {
            IM3CodePage page = i_function->codePageRefs[i_function->अंक_Code_Page_Refs];

            यदि (--(page->info.usage_गिनती) == 0) {
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
    यदि (अंक_Names > 0) {
        प्रत्यागम names[0];
    } अन्यथा {
        प्रत्यागम "<unnamed>";
    }
}


IM3Module m3_GetFunctionModule (IM3Function i_function)
{
    प्रत्यागम i_function ? i_function->module : रिक्त;
}


cstr_t* GetFunctionNames (IM3Function i_function, u16* o_अंक_Names)
{
    यदि (!i_function || !o_अंक_Names) {
        प्रत्यागम रिक्त;
    }

    यदि (i_function->import.fieldUtf8) {
        *o_अंक_Names = 1;
        प्रत्यागम &i_function->import.fieldUtf8;
    } अन्यथा {
        *o_अंक_Names = i_function->अंक_Names;
        प्रत्यागम i_function->names;
    }
}


cstr_t GetFunctionImportModuleName (IM3Function i_function)
{
    प्रत्यागम (i_function->import.moduleUtf8) ? i_function->import.moduleUtf8 : "";
}


u16 Get_Function_अंक_Args (IM3Function i_function)
{
    u16 अंक_Args = 0;

    यदि (i_function) {
        यदि (i_function->funcType) {
            अंक_Args = i_function->funcType->अंक_Args;
        }
    }

    प्रत्यागम अंक_Args;
}

m3type_t GetFunctionArgType (IM3Function i_function, u32 i_index)
{
    m3type_t type = c_m3Type_none;

    यदि (i_index < Get_Function_अंक_Args(i_function)) {
        u32 अंक_Returns = i_function->funcType->अंक_Rets;

        type = i_function->funcType->types[अंक_Returns + i_index];
    }

    प्रत्यागम type;
}


u16 Get_Function_अंक_Returns (IM3Function i_function)
{
    u16 अंक_Returns = 0;

    यदि (i_function) {
        यदि (i_function->funcType) {
            अंक_Returns = i_function->funcType->अंक_Rets;
        }
    }

    प्रत्यागम अंक_Returns;
}


u8 GetFunctionReturnType (ध्रुव IM3Function i_function, u16 i_index)
{
    प्रत्यागम i_function ? GetFuncTypeResultType(i_function->funcType, i_index) : c_m3Type_unknown;
}


u32 Get_Function_अंक_Args_And_Locals (IM3Function i_function)
{
    यदि (i_function) {
        प्रत्यागम i_function->अंक_Locals + Get_Function_अंक_Args(i_function);
    } अन्यथा {
        प्रत्यागम 0;
    }
}
