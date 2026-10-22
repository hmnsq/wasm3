//
//  m3_function.c
//
//  Created by Steven Massey on 4/7/21.
//  Copyright © 2021 Steven Massey. All rights reserved.
//

#അന്തര്ഭാവ "m3_function.h"
#അന്തര്ഭാവ "m3_env.h"


M3Result AllocFuncType (IM3FuncType* o_functionType, u32 i_അക്കം_Types)
{
    *o_functionType = (IM3FuncType)m3_Malloc("M3FuncType", വലുപ്പം(M3FuncType) + i_അക്കം_Types * വലുപ്പം(m3type_t));
    എങ്കിൽ (*o_functionType) {
        (*o_functionType)->isContinuation = false;
        (*o_functionType)->contFuncType   = ശൂന്യ;
        (*o_functionType)->inRecGroup     = false;
        തിരികെ m3Err_none;
    }
    തിരികെ m3Err_mallocFailed;
}


#എങ്കിൽ d_m3HasTypedRefs

// The type a (ref $t) / (ref null $t) naming this function type is spelled as
m3type_t RefTypeOfFuncType (സ്ഥിരം IM3FuncType i_funcType, bool i_nonNull)
{
    m3type_t type = d_m3Type_ref | (i_funcType ? i_funcType->canonicalIndex : d_m3Type_heapAbstract);

    എങ്കിൽ (i_funcType and i_funcType->isContinuation) {
        type |= d_m3Type_refCont;
    }

    എങ്കിൽ (i_nonNull) {
        type |= d_m3Type_refNonNull;
    }

    തിരികെ type;
}

#endif // d_m3HasTypedRefs


bool AreFuncTypesEqual (സ്ഥിരം IM3FuncType i_typeA, സ്ഥിരം IM3FuncType i_typeB)
{
    എങ്കിൽ (i_typeA->isContinuation != i_typeB->isContinuation) {
        തിരികെ false;
    }
    എങ്കിൽ (i_typeA->isContinuation) {
        തിരികെ (i_typeA->contFuncType == i_typeB->contFuncType) or
               (i_typeA->contFuncType and i_typeB->contFuncType and
                AreFuncTypesEqual(i_typeA->contFuncType, i_typeB->contFuncType));
    }
    എങ്കിൽ (i_typeA->അക്കം_Rets == i_typeB->അക്കം_Rets and i_typeA->അക്കം_Args == i_typeB->അക്കം_Args) {
        തിരികെ (memcmp(i_typeA->types, i_typeB->types,
                       (i_typeA->അക്കം_Rets + i_typeA->അക്കം_Args) * വലുപ്പം(m3type_t)) == 0);
    }

    തിരികെ false;
}

u16 Get_Func_Type_അക്കം_Params (സ്ഥിരം IM3FuncType i_funcType)
{
    തിരികെ i_funcType ? i_funcType->അക്കം_Args : 0;
}


m3type_t GetFuncTypeParamType (സ്ഥിരം IM3FuncType i_funcType, u16 i_index)
{
    m3type_t type = c_m3Type_unknown;

    എങ്കിൽ (i_funcType) {
        എങ്കിൽ (i_index < i_funcType->അക്കം_Args) {
            type = i_funcType->types[i_funcType->അക്കം_Rets + i_index];
        }
    }

    തിരികെ type;
}


u16 Get_Func_Type_അക്കം_Results (സ്ഥിരം IM3FuncType i_funcType)
{
    തിരികെ i_funcType ? i_funcType->അക്കം_Rets : 0;
}


m3type_t GetFuncTypeResultType (സ്ഥിരം IM3FuncType i_funcType, u16 i_index)
{
    m3type_t type = c_m3Type_unknown;

    എങ്കിൽ (i_funcType) {
        എങ്കിൽ (i_index < i_funcType->അക്കം_Rets) {
            type = i_funcType->types[i_index];
        }
    }

    തിരികെ type;
}


//---------------------------------------------------------------------------------------------------------------


ശൂന്യം FreeImportInfo (M3ImportInfo* i_info)
{
    m3_Free(i_info->moduleUtf8);
    m3_Free(i_info->fieldUtf8);
}


#എങ്കിൽ d_m3HasSnapshots

ശൂന്യം SnapshotMap_Free (M3SnapshotMap* i_map)
{
    എങ്കിൽ (i_map) {
        m3_Free(i_map->locals);
        m3_Free(i_map->safePoints);
        m3_Free(i_map->മൂല്യങ്ങൾ);
        m3_Free(i_map->blocks);
        m3_Free(i_map);
    }
}

#endif

ശൂന്യം Function_Release (IM3Function i_function)
{
    m3_Free(i_function->constants);

#എങ്കിൽ d_m3HasSnapshots
    SnapshotMap_Free(i_function->snapshotMap);
    i_function->snapshotMap = ശൂന്യ;
#endif

    വേണ്ടി (പൂർണ്ണസംഖ്യ i = 0; i < i_function->അക്കം_Names; i++) {
        // name can be an alias of fieldUtf8
        എങ്കിൽ (i_function->names[i] != i_function->import.fieldUtf8) {
            m3_Free(i_function->names[i]);
        }
    }

    FreeImportInfo(&i_function->import);

    എങ്കിൽ (i_function->ownsWasmCode) {
        m3_Free(i_function->wasm);
    }

    // Function_FreeCompiledCode (func);

#എങ്കിൽ (d_m3EnableCodePageRefCounting)
    {
        m3_Free(i_function->codePageRefs);
        i_function->അക്കം_Code_Page_Refs = 0;
    }
#endif
}


ശൂന്യം Function_FreeCompiledCode (IM3Function i_function)
{
#എങ്കിൽ (d_m3EnableCodePageRefCounting)
    {
        i_function->compiled = ശൂന്യ;

        ആകുമ്പോൾ (i_function->അക്കം_Code_Page_Refs--) {
            IM3CodePage page = i_function->codePageRefs[i_function->അക്കം_Code_Page_Refs];

            എങ്കിൽ (--(page->info.usage_എണ്ണം) == 0) {
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
    u16     അക്കം_Names = 0;
    cstr_t* names    = GetFunctionNames(i_function, &അക്കം_Names);
    എങ്കിൽ (അക്കം_Names > 0) {
        തിരികെ names[0];
    } അല്ലെങ്കിൽ {
        തിരികെ "<unnamed>";
    }
}


IM3Module m3_GetFunctionModule (IM3Function i_function)
{
    തിരികെ i_function ? i_function->module : ശൂന്യ;
}


cstr_t* GetFunctionNames (IM3Function i_function, u16* o_അക്കം_Names)
{
    എങ്കിൽ (!i_function || !o_അക്കം_Names) {
        തിരികെ ശൂന്യ;
    }

    എങ്കിൽ (i_function->import.fieldUtf8) {
        *o_അക്കം_Names = 1;
        തിരികെ &i_function->import.fieldUtf8;
    } അല്ലെങ്കിൽ {
        *o_അക്കം_Names = i_function->അക്കം_Names;
        തിരികെ i_function->names;
    }
}


cstr_t GetFunctionImportModuleName (IM3Function i_function)
{
    തിരികെ (i_function->import.moduleUtf8) ? i_function->import.moduleUtf8 : "";
}


u16 Get_Function_അക്കം_Args (IM3Function i_function)
{
    u16 അക്കം_Args = 0;

    എങ്കിൽ (i_function) {
        എങ്കിൽ (i_function->funcType) {
            അക്കം_Args = i_function->funcType->അക്കം_Args;
        }
    }

    തിരികെ അക്കം_Args;
}

m3type_t GetFunctionArgType (IM3Function i_function, u32 i_index)
{
    m3type_t type = c_m3Type_none;

    എങ്കിൽ (i_index < Get_Function_അക്കം_Args(i_function)) {
        u32 അക്കം_Returns = i_function->funcType->അക്കം_Rets;

        type = i_function->funcType->types[അക്കം_Returns + i_index];
    }

    തിരികെ type;
}


u16 Get_Function_അക്കം_Returns (IM3Function i_function)
{
    u16 അക്കം_Returns = 0;

    എങ്കിൽ (i_function) {
        എങ്കിൽ (i_function->funcType) {
            അക്കം_Returns = i_function->funcType->അക്കം_Rets;
        }
    }

    തിരികെ അക്കം_Returns;
}


u8 GetFunctionReturnType (സ്ഥിരം IM3Function i_function, u16 i_index)
{
    തിരികെ i_function ? GetFuncTypeResultType(i_function->funcType, i_index) : c_m3Type_unknown;
}


u32 Get_Function_അക്കം_Args_And_Locals (IM3Function i_function)
{
    എങ്കിൽ (i_function) {
        തിരികെ i_function->അക്കം_Locals + Get_Function_അക്കം_Args(i_function);
    } അല്ലെങ്കിൽ {
        തിരികെ 0;
    }
}
