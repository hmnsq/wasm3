//
//  m3_function.c
//
//  Created by Steven Massey on 4/7/21.
//  Copyright © 2021 Steven Massey. All rights reserved.
//

#ಅನ್ತರ್ಭಾವ "m3_function.h"
#ಅನ್ತರ್ಭಾವ "m3_env.h"


M3Result AllocFuncType (IM3FuncType* o_functionType, u32 i_ಅಂಕೆ_Types)
{
    *o_functionType = (IM3FuncType)m3_Malloc("M3FuncType", ಪರಿಮಾಣ(M3FuncType) + i_ಅಂಕೆ_Types * ಪರಿಮಾಣ(m3type_t));
    ಯದಿ (*o_functionType) {
        (*o_functionType)->isContinuation = false;
        (*o_functionType)->contFuncType   = ರಿಕ್ತ;
        (*o_functionType)->inRecGroup     = false;
        ಪ್ರತ್ಯಾಗಮ m3Err_none;
    }
    ಪ್ರತ್ಯಾಗಮ m3Err_mallocFailed;
}


#ಯದಿ d_m3HasTypedRefs

// The type a (ref $t) / (ref null $t) naming this function type is spelled as
m3type_t RefTypeOfFuncType (ಧ್ರುವ IM3FuncType i_funcType, bool i_nonNull)
{
    m3type_t type = d_m3Type_ref | (i_funcType ? i_funcType->canonicalIndex : d_m3Type_heapAbstract);

    ಯದಿ (i_funcType and i_funcType->isContinuation) {
        type |= d_m3Type_refCont;
    }

    ಯದಿ (i_nonNull) {
        type |= d_m3Type_refNonNull;
    }

    ಪ್ರತ್ಯಾಗಮ type;
}

#endif // d_m3HasTypedRefs


bool AreFuncTypesEqual (ಧ್ರುವ IM3FuncType i_typeA, ಧ್ರುವ IM3FuncType i_typeB)
{
    ಯದಿ (i_typeA->isContinuation != i_typeB->isContinuation) {
        ಪ್ರತ್ಯಾಗಮ false;
    }
    ಯದಿ (i_typeA->isContinuation) {
        ಪ್ರತ್ಯಾಗಮ (i_typeA->contFuncType == i_typeB->contFuncType) or
               (i_typeA->contFuncType and i_typeB->contFuncType and
                AreFuncTypesEqual(i_typeA->contFuncType, i_typeB->contFuncType));
    }
    ಯದಿ (i_typeA->ಅಂಕೆ_Rets == i_typeB->ಅಂಕೆ_Rets and i_typeA->ಅಂಕೆ_Args == i_typeB->ಅಂಕೆ_Args) {
        ಪ್ರತ್ಯಾಗಮ (memcmp(i_typeA->types, i_typeB->types,
                       (i_typeA->ಅಂಕೆ_Rets + i_typeA->ಅಂಕೆ_Args) * ಪರಿಮಾಣ(m3type_t)) == 0);
    }

    ಪ್ರತ್ಯಾಗಮ false;
}

u16 Get_Func_Type_ಅಂಕೆ_Params (ಧ್ರುವ IM3FuncType i_funcType)
{
    ಪ್ರತ್ಯಾಗಮ i_funcType ? i_funcType->ಅಂಕೆ_Args : 0;
}


m3type_t GetFuncTypeParamType (ಧ್ರುವ IM3FuncType i_funcType, u16 i_index)
{
    m3type_t type = c_m3Type_unknown;

    ಯದಿ (i_funcType) {
        ಯದಿ (i_index < i_funcType->ಅಂಕೆ_Args) {
            type = i_funcType->types[i_funcType->ಅಂಕೆ_Rets + i_index];
        }
    }

    ಪ್ರತ್ಯಾಗಮ type;
}


u16 Get_Func_Type_ಅಂಕೆ_Results (ಧ್ರುವ IM3FuncType i_funcType)
{
    ಪ್ರತ್ಯಾಗಮ i_funcType ? i_funcType->ಅಂಕೆ_Rets : 0;
}


m3type_t GetFuncTypeResultType (ಧ್ರುವ IM3FuncType i_funcType, u16 i_index)
{
    m3type_t type = c_m3Type_unknown;

    ಯದಿ (i_funcType) {
        ಯದಿ (i_index < i_funcType->ಅಂಕೆ_Rets) {
            type = i_funcType->types[i_index];
        }
    }

    ಪ್ರತ್ಯಾಗಮ type;
}


//---------------------------------------------------------------------------------------------------------------


ಶೂನ್ಯ FreeImportInfo (M3ImportInfo* i_info)
{
    m3_Free(i_info->moduleUtf8);
    m3_Free(i_info->fieldUtf8);
}


#ಯದಿ d_m3HasSnapshots

ಶೂನ್ಯ SnapshotMap_Free (M3SnapshotMap* i_map)
{
    ಯದಿ (i_map) {
        m3_Free(i_map->locals);
        m3_Free(i_map->safePoints);
        m3_Free(i_map->ಮೌಲ್ಯಗಳು);
        m3_Free(i_map->blocks);
        m3_Free(i_map);
    }
}

#endif

ಶೂನ್ಯ Function_Release (IM3Function i_function)
{
    m3_Free(i_function->constants);

#ಯದಿ d_m3HasSnapshots
    SnapshotMap_Free(i_function->snapshotMap);
    i_function->snapshotMap = ರಿಕ್ತ;
#endif

    ಪ್ರತಿ (ಪೂರ್ಣಾಙ್ಕ i = 0; i < i_function->ಅಂಕೆ_Names; i++) {
        // name can be an alias of fieldUtf8
        ಯದಿ (i_function->names[i] != i_function->import.fieldUtf8) {
            m3_Free(i_function->names[i]);
        }
    }

    FreeImportInfo(&i_function->import);

    ಯದಿ (i_function->ownsWasmCode) {
        m3_Free(i_function->wasm);
    }

    // Function_FreeCompiledCode (func);

#ಯದಿ (d_m3EnableCodePageRefCounting)
    {
        m3_Free(i_function->codePageRefs);
        i_function->ಅಂಕೆ_Code_Page_Refs = 0;
    }
#endif
}


ಶೂನ್ಯ Function_FreeCompiledCode (IM3Function i_function)
{
#ಯದಿ (d_m3EnableCodePageRefCounting)
    {
        i_function->compiled = ರಿಕ್ತ;

        ಯಾವತ್ (i_function->ಅಂಕೆ_Code_Page_Refs--) {
            IM3CodePage page = i_function->codePageRefs[i_function->ಅಂಕೆ_Code_Page_Refs];

            ಯದಿ (--(page->info.usage_ಎಣಿಕೆ) == 0) {
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
    u16     ಅಂಕೆ_Names = 0;
    cstr_t* names    = GetFunctionNames(i_function, &ಅಂಕೆ_Names);
    ಯದಿ (ಅಂಕೆ_Names > 0) {
        ಪ್ರತ್ಯಾಗಮ names[0];
    } ಅನ್ಯಥಾ {
        ಪ್ರತ್ಯಾಗಮ "<unnamed>";
    }
}


IM3Module m3_GetFunctionModule (IM3Function i_function)
{
    ಪ್ರತ್ಯಾಗಮ i_function ? i_function->module : ರಿಕ್ತ;
}


cstr_t* GetFunctionNames (IM3Function i_function, u16* o_ಅಂಕೆ_Names)
{
    ಯದಿ (!i_function || !o_ಅಂಕೆ_Names) {
        ಪ್ರತ್ಯಾಗಮ ರಿಕ್ತ;
    }

    ಯದಿ (i_function->import.fieldUtf8) {
        *o_ಅಂಕೆ_Names = 1;
        ಪ್ರತ್ಯಾಗಮ &i_function->import.fieldUtf8;
    } ಅನ್ಯಥಾ {
        *o_ಅಂಕೆ_Names = i_function->ಅಂಕೆ_Names;
        ಪ್ರತ್ಯಾಗಮ i_function->names;
    }
}


cstr_t GetFunctionImportModuleName (IM3Function i_function)
{
    ಪ್ರತ್ಯಾಗಮ (i_function->import.moduleUtf8) ? i_function->import.moduleUtf8 : "";
}


u16 Get_Function_ಅಂಕೆ_Args (IM3Function i_function)
{
    u16 ಅಂಕೆ_Args = 0;

    ಯದಿ (i_function) {
        ಯದಿ (i_function->funcType) {
            ಅಂಕೆ_Args = i_function->funcType->ಅಂಕೆ_Args;
        }
    }

    ಪ್ರತ್ಯಾಗಮ ಅಂಕೆ_Args;
}

m3type_t GetFunctionArgType (IM3Function i_function, u32 i_index)
{
    m3type_t type = c_m3Type_none;

    ಯದಿ (i_index < Get_Function_ಅಂಕೆ_Args(i_function)) {
        u32 ಅಂಕೆ_Returns = i_function->funcType->ಅಂಕೆ_Rets;

        type = i_function->funcType->types[ಅಂಕೆ_Returns + i_index];
    }

    ಪ್ರತ್ಯಾಗಮ type;
}


u16 Get_Function_ಅಂಕೆ_Returns (IM3Function i_function)
{
    u16 ಅಂಕೆ_Returns = 0;

    ಯದಿ (i_function) {
        ಯದಿ (i_function->funcType) {
            ಅಂಕೆ_Returns = i_function->funcType->ಅಂಕೆ_Rets;
        }
    }

    ಪ್ರತ್ಯಾಗಮ ಅಂಕೆ_Returns;
}


u8 GetFunctionReturnType (ಧ್ರುವ IM3Function i_function, u16 i_index)
{
    ಪ್ರತ್ಯಾಗಮ i_function ? GetFuncTypeResultType(i_function->funcType, i_index) : c_m3Type_unknown;
}


u32 Get_Function_ಅಂಕೆ_Args_And_Locals (IM3Function i_function)
{
    ಯದಿ (i_function) {
        ಪ್ರತ್ಯಾಗಮ i_function->ಅಂಕೆ_Locals + Get_Function_ಅಂಕೆ_Args(i_function);
    } ಅನ್ಯಥಾ {
        ಪ್ರತ್ಯಾಗಮ 0;
    }
}
