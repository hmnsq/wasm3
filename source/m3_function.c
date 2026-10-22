//
//  m3_function.c
//
//  Created by Steven Massey on 4/7/21.
//  Copyright © 2021 Steven Massey. All rights reserved.
//

#ଅନ୍ତର୍ଭାଵ "m3_function.h"
#ଅନ୍ତର୍ଭାଵ "m3_env.h"


M3Result AllocFuncType (IM3FuncType* o_functionType, u32 i_ଅଙ୍କ_Types)
{
    *o_functionType = (IM3FuncType)m3_Malloc("M3FuncType", ଆକାର(M3FuncType) + i_ଅଙ୍କ_Types * ଆକାର(m3type_t));
    ଯଦି (*o_functionType) {
        (*o_functionType)->isContinuation = false;
        (*o_functionType)->contFuncType   = ଖାଲି;
        (*o_functionType)->inRecGroup     = false;
        ଫେରନ୍ତୁ m3Err_none;
    }
    ଫେରନ୍ତୁ m3Err_mallocFailed;
}


#ଯଦି d_m3HasTypedRefs

// The type a (ref $t) / (ref null $t) naming this function type is spelled as
m3type_t RefTypeOfFuncType (ଅଚଳ IM3FuncType i_funcType, bool i_nonNull)
{
    m3type_t type = d_m3Type_ref | (i_funcType ? i_funcType->canonicalIndex : d_m3Type_heapAbstract);

    ଯଦି (i_funcType and i_funcType->isContinuation) {
        type |= d_m3Type_refCont;
    }

    ଯଦି (i_nonNull) {
        type |= d_m3Type_refNonNull;
    }

    ଫେରନ୍ତୁ type;
}

#endif // d_m3HasTypedRefs


bool AreFuncTypesEqual (ଅଚଳ IM3FuncType i_typeA, ଅଚଳ IM3FuncType i_typeB)
{
    ଯଦି (i_typeA->isContinuation != i_typeB->isContinuation) {
        ଫେରନ୍ତୁ false;
    }
    ଯଦି (i_typeA->isContinuation) {
        ଫେରନ୍ତୁ (i_typeA->contFuncType == i_typeB->contFuncType) or
               (i_typeA->contFuncType and i_typeB->contFuncType and
                AreFuncTypesEqual(i_typeA->contFuncType, i_typeB->contFuncType));
    }
    ଯଦି (i_typeA->ଅଙ୍କ_Rets == i_typeB->ଅଙ୍କ_Rets and i_typeA->ଅଙ୍କ_Args == i_typeB->ଅଙ୍କ_Args) {
        ଫେରନ୍ତୁ (memcmp(i_typeA->types, i_typeB->types,
                       (i_typeA->ଅଙ୍କ_Rets + i_typeA->ଅଙ୍କ_Args) * ଆକାର(m3type_t)) == 0);
    }

    ଫେରନ୍ତୁ false;
}

u16 Get_Func_Type_ଅଙ୍କ_Params (ଅଚଳ IM3FuncType i_funcType)
{
    ଫେରନ୍ତୁ i_funcType ? i_funcType->ଅଙ୍କ_Args : 0;
}


m3type_t GetFuncTypeParamType (ଅଚଳ IM3FuncType i_funcType, u16 i_index)
{
    m3type_t type = c_m3Type_unknown;

    ଯଦି (i_funcType) {
        ଯଦି (i_index < i_funcType->ଅଙ୍କ_Args) {
            type = i_funcType->types[i_funcType->ଅଙ୍କ_Rets + i_index];
        }
    }

    ଫେରନ୍ତୁ type;
}


u16 Get_Func_Type_ଅଙ୍କ_Results (ଅଚଳ IM3FuncType i_funcType)
{
    ଫେରନ୍ତୁ i_funcType ? i_funcType->ଅଙ୍କ_Rets : 0;
}


m3type_t GetFuncTypeResultType (ଅଚଳ IM3FuncType i_funcType, u16 i_index)
{
    m3type_t type = c_m3Type_unknown;

    ଯଦି (i_funcType) {
        ଯଦି (i_index < i_funcType->ଅଙ୍କ_Rets) {
            type = i_funcType->types[i_index];
        }
    }

    ଫେରନ୍ତୁ type;
}


//---------------------------------------------------------------------------------------------------------------


ଶୂନ୍ୟ FreeImportInfo (M3ImportInfo* i_info)
{
    m3_Free(i_info->moduleUtf8);
    m3_Free(i_info->fieldUtf8);
}


#ଯଦି d_m3HasSnapshots

ଶୂନ୍ୟ SnapshotMap_Free (M3SnapshotMap* i_map)
{
    ଯଦି (i_map) {
        m3_Free(i_map->locals);
        m3_Free(i_map->safePoints);
        m3_Free(i_map->ମୂଲ୍ୟଗୁଡ଼ିକ);
        m3_Free(i_map->blocks);
        m3_Free(i_map);
    }
}

#endif

ଶୂନ୍ୟ Function_Release (IM3Function i_function)
{
    m3_Free(i_function->constants);

#ଯଦି d_m3HasSnapshots
    SnapshotMap_Free(i_function->snapshotMap);
    i_function->snapshotMap = ଖାଲି;
#endif

    ପାଇଁ (ପୂର୍ଣ୍ଣସଂଖ୍ୟା i = 0; i < i_function->ଅଙ୍କ_Names; i++) {
        // name can be an alias of fieldUtf8
        ଯଦି (i_function->names[i] != i_function->import.fieldUtf8) {
            m3_Free(i_function->names[i]);
        }
    }

    FreeImportInfo(&i_function->import);

    ଯଦି (i_function->ownsWasmCode) {
        m3_Free(i_function->wasm);
    }

    // Function_FreeCompiledCode (func);

#ଯଦି (d_m3EnableCodePageRefCounting)
    {
        m3_Free(i_function->codePageRefs);
        i_function->ଅଙ୍କ_Code_Page_Refs = 0;
    }
#endif
}


ଶୂନ୍ୟ Function_FreeCompiledCode (IM3Function i_function)
{
#ଯଦି (d_m3EnableCodePageRefCounting)
    {
        i_function->compiled = ଖାଲି;

        ଯେତେବେଳେ (i_function->ଅଙ୍କ_Code_Page_Refs--) {
            IM3CodePage page = i_function->codePageRefs[i_function->ଅଙ୍କ_Code_Page_Refs];

            ଯଦି (--(page->info.usage_ଗଣନା) == 0) {
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
    u16     ଅଙ୍କ_Names = 0;
    cstr_t* names    = GetFunctionNames(i_function, &ଅଙ୍କ_Names);
    ଯଦି (ଅଙ୍କ_Names > 0) {
        ଫେରନ୍ତୁ names[0];
    } ଅନ୍ୟଥା {
        ଫେରନ୍ତୁ "<unnamed>";
    }
}


IM3Module m3_GetFunctionModule (IM3Function i_function)
{
    ଫେରନ୍ତୁ i_function ? i_function->module : ଖାଲି;
}


cstr_t* GetFunctionNames (IM3Function i_function, u16* o_ଅଙ୍କ_Names)
{
    ଯଦି (!i_function || !o_ଅଙ୍କ_Names) {
        ଫେରନ୍ତୁ ଖାଲି;
    }

    ଯଦି (i_function->import.fieldUtf8) {
        *o_ଅଙ୍କ_Names = 1;
        ଫେରନ୍ତୁ &i_function->import.fieldUtf8;
    } ଅନ୍ୟଥା {
        *o_ଅଙ୍କ_Names = i_function->ଅଙ୍କ_Names;
        ଫେରନ୍ତୁ i_function->names;
    }
}


cstr_t GetFunctionImportModuleName (IM3Function i_function)
{
    ଫେରନ୍ତୁ (i_function->import.moduleUtf8) ? i_function->import.moduleUtf8 : "";
}


u16 Get_Function_ଅଙ୍କ_Args (IM3Function i_function)
{
    u16 ଅଙ୍କ_Args = 0;

    ଯଦି (i_function) {
        ଯଦି (i_function->funcType) {
            ଅଙ୍କ_Args = i_function->funcType->ଅଙ୍କ_Args;
        }
    }

    ଫେରନ୍ତୁ ଅଙ୍କ_Args;
}

m3type_t GetFunctionArgType (IM3Function i_function, u32 i_index)
{
    m3type_t type = c_m3Type_none;

    ଯଦି (i_index < Get_Function_ଅଙ୍କ_Args(i_function)) {
        u32 ଅଙ୍କ_Returns = i_function->funcType->ଅଙ୍କ_Rets;

        type = i_function->funcType->types[ଅଙ୍କ_Returns + i_index];
    }

    ଫେରନ୍ତୁ type;
}


u16 Get_Function_ଅଙ୍କ_Returns (IM3Function i_function)
{
    u16 ଅଙ୍କ_Returns = 0;

    ଯଦି (i_function) {
        ଯଦି (i_function->funcType) {
            ଅଙ୍କ_Returns = i_function->funcType->ଅଙ୍କ_Rets;
        }
    }

    ଫେରନ୍ତୁ ଅଙ୍କ_Returns;
}


u8 GetFunctionReturnType (ଅଚଳ IM3Function i_function, u16 i_index)
{
    ଫେରନ୍ତୁ i_function ? GetFuncTypeResultType(i_function->funcType, i_index) : c_m3Type_unknown;
}


u32 Get_Function_ଅଙ୍କ_Args_And_Locals (IM3Function i_function)
{
    ଯଦି (i_function) {
        ଫେରନ୍ତୁ i_function->ଅଙ୍କ_Locals + Get_Function_ଅଙ୍କ_Args(i_function);
    } ଅନ୍ୟଥା {
        ଫେରନ୍ତୁ 0;
    }
}
