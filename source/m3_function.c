//
//  m3_function.c
//
//  Created by Steven Massey on 4/7/21.
//  Copyright © 2021 Steven Massey. All rights reserved.
//

#અન્તર્ભાવ "m3_function.h"
#અન્તર્ભાવ "m3_env.h"


M3Result AllocFuncType (IM3FuncType* o_functionType, u32 i_અંક_Types)
{
    *o_functionType = (IM3FuncType)m3_Malloc("M3FuncType", કદ(M3FuncType) + i_અંક_Types * કદ(m3type_t));
    જો (*o_functionType) {
        (*o_functionType)->isContinuation = false;
        (*o_functionType)->contFuncType   = ખાલી;
        (*o_functionType)->inRecGroup     = false;
        પાછા m3Err_none;
    }
    પાછા m3Err_mallocFailed;
}


#જો d_m3HasTypedRefs

// The type a (ref $t) / (ref null $t) naming this function type is spelled as
m3type_t RefTypeOfFuncType (અચલ IM3FuncType i_funcType, bool i_nonNull)
{
    m3type_t type = d_m3Type_ref | (i_funcType ? i_funcType->canonicalIndex : d_m3Type_heapAbstract);

    જો (i_funcType and i_funcType->isContinuation) {
        type |= d_m3Type_refCont;
    }

    જો (i_nonNull) {
        type |= d_m3Type_refNonNull;
    }

    પાછા type;
}

#endif // d_m3HasTypedRefs


bool AreFuncTypesEqual (અચલ IM3FuncType i_typeA, અચલ IM3FuncType i_typeB)
{
    જો (i_typeA->isContinuation != i_typeB->isContinuation) {
        પાછા false;
    }
    જો (i_typeA->isContinuation) {
        પાછા (i_typeA->contFuncType == i_typeB->contFuncType) or
               (i_typeA->contFuncType and i_typeB->contFuncType and
                AreFuncTypesEqual(i_typeA->contFuncType, i_typeB->contFuncType));
    }
    જો (i_typeA->અંક_Rets == i_typeB->અંક_Rets and i_typeA->અંક_Args == i_typeB->અંક_Args) {
        પાછા (memcmp(i_typeA->types, i_typeB->types,
                       (i_typeA->અંક_Rets + i_typeA->અંક_Args) * કદ(m3type_t)) == 0);
    }

    પાછા false;
}

u16 Get_Func_Type_અંક_Params (અચલ IM3FuncType i_funcType)
{
    પાછા i_funcType ? i_funcType->અંક_Args : 0;
}


m3type_t GetFuncTypeParamType (અચલ IM3FuncType i_funcType, u16 i_index)
{
    m3type_t type = c_m3Type_unknown;

    જો (i_funcType) {
        જો (i_index < i_funcType->અંક_Args) {
            type = i_funcType->types[i_funcType->અંક_Rets + i_index];
        }
    }

    પાછા type;
}


u16 Get_Func_Type_અંક_Results (અચલ IM3FuncType i_funcType)
{
    પાછા i_funcType ? i_funcType->અંક_Rets : 0;
}


m3type_t GetFuncTypeResultType (અચલ IM3FuncType i_funcType, u16 i_index)
{
    m3type_t type = c_m3Type_unknown;

    જો (i_funcType) {
        જો (i_index < i_funcType->અંક_Rets) {
            type = i_funcType->types[i_index];
        }
    }

    પાછા type;
}


//---------------------------------------------------------------------------------------------------------------


શૂન્ય FreeImportInfo (M3ImportInfo* i_info)
{
    m3_Free(i_info->moduleUtf8);
    m3_Free(i_info->fieldUtf8);
}


#જો d_m3HasSnapshots

શૂન્ય SnapshotMap_Free (M3SnapshotMap* i_map)
{
    જો (i_map) {
        m3_Free(i_map->locals);
        m3_Free(i_map->safePoints);
        m3_Free(i_map->કિંમતો);
        m3_Free(i_map->blocks);
        m3_Free(i_map);
    }
}

#endif

શૂન્ય Function_Release (IM3Function i_function)
{
    m3_Free(i_function->constants);

#જો d_m3HasSnapshots
    SnapshotMap_Free(i_function->snapshotMap);
    i_function->snapshotMap = ખાલી;
#endif

    માટે (પૂર્ણાંક i = 0; i < i_function->અંક_Names; i++) {
        // name can be an alias of fieldUtf8
        જો (i_function->names[i] != i_function->import.fieldUtf8) {
            m3_Free(i_function->names[i]);
        }
    }

    FreeImportInfo(&i_function->import);

    જો (i_function->ownsWasmCode) {
        m3_Free(i_function->wasm);
    }

    // Function_FreeCompiledCode (func);

#જો (d_m3EnableCodePageRefCounting)
    {
        m3_Free(i_function->codePageRefs);
        i_function->અંક_Code_Page_Refs = 0;
    }
#endif
}


શૂન્ય Function_FreeCompiledCode (IM3Function i_function)
{
#જો (d_m3EnableCodePageRefCounting)
    {
        i_function->compiled = ખાલી;

        જ્યારે (i_function->અંક_Code_Page_Refs--) {
            IM3CodePage page = i_function->codePageRefs[i_function->અંક_Code_Page_Refs];

            જો (--(page->info.usage_ગણતરી) == 0) {
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
    u16     અંક_Names = 0;
    cstr_t* names    = GetFunctionNames(i_function, &અંક_Names);
    જો (અંક_Names > 0) {
        પાછા names[0];
    } નહીંતર {
        પાછા "<unnamed>";
    }
}


IM3Module m3_GetFunctionModule (IM3Function i_function)
{
    પાછા i_function ? i_function->module : ખાલી;
}


cstr_t* GetFunctionNames (IM3Function i_function, u16* o_અંક_Names)
{
    જો (!i_function || !o_અંક_Names) {
        પાછા ખાલી;
    }

    જો (i_function->import.fieldUtf8) {
        *o_અંક_Names = 1;
        પાછા &i_function->import.fieldUtf8;
    } નહીંતર {
        *o_અંક_Names = i_function->અંક_Names;
        પાછા i_function->names;
    }
}


cstr_t GetFunctionImportModuleName (IM3Function i_function)
{
    પાછા (i_function->import.moduleUtf8) ? i_function->import.moduleUtf8 : "";
}


u16 Get_Function_અંક_Args (IM3Function i_function)
{
    u16 અંક_Args = 0;

    જો (i_function) {
        જો (i_function->funcType) {
            અંક_Args = i_function->funcType->અંક_Args;
        }
    }

    પાછા અંક_Args;
}

m3type_t GetFunctionArgType (IM3Function i_function, u32 i_index)
{
    m3type_t type = c_m3Type_none;

    જો (i_index < Get_Function_અંક_Args(i_function)) {
        u32 અંક_Returns = i_function->funcType->અંક_Rets;

        type = i_function->funcType->types[અંક_Returns + i_index];
    }

    પાછા type;
}


u16 Get_Function_અંક_Returns (IM3Function i_function)
{
    u16 અંક_Returns = 0;

    જો (i_function) {
        જો (i_function->funcType) {
            અંક_Returns = i_function->funcType->અંક_Rets;
        }
    }

    પાછા અંક_Returns;
}


u8 GetFunctionReturnType (અચલ IM3Function i_function, u16 i_index)
{
    પાછા i_function ? GetFuncTypeResultType(i_function->funcType, i_index) : c_m3Type_unknown;
}


u32 Get_Function_અંક_Args_And_Locals (IM3Function i_function)
{
    જો (i_function) {
        પાછા i_function->અંક_Locals + Get_Function_અંક_Args(i_function);
    } નહીંતર {
        પાછા 0;
    }
}
