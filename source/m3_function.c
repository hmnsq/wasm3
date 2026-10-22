//
//  m3_function.c
//
//  Created by Steven Massey on 4/7/21.
//  Copyright © 2021 Steven Massey. All rights reserved.
//

#ضمن "m3_function.h"
#ضمن "m3_env.h"


M3Result AllocFuncType (IM3FuncType* o_functionType, u32 i_عدد_Types)
{
    *o_functionType = (IM3FuncType)m3_Malloc("M3FuncType", حجم(M3FuncType) + i_عدد_Types * حجم(m3type_t));
    إذا (*o_functionType) {
        (*o_functionType)->isContinuation = false;
        (*o_functionType)->contFuncType   = عدم;
        (*o_functionType)->inRecGroup     = false;
        أرجع m3Err_none;
    }
    أرجع m3Err_mallocFailed;
}


#إذا d_m3HasTypedRefs

// The type a (ref $t) / (ref null $t) naming this function type is spelled as
m3type_t RefTypeOfFuncType (ثابت IM3FuncType i_funcType, bool i_nonNull)
{
    m3type_t type = d_m3Type_ref | (i_funcType ? i_funcType->canonicalIndex : d_m3Type_heapAbstract);

    إذا (i_funcType and i_funcType->isContinuation) {
        type |= d_m3Type_refCont;
    }

    إذا (i_nonNull) {
        type |= d_m3Type_refNonNull;
    }

    أرجع type;
}

#endif // d_m3HasTypedRefs


bool AreFuncTypesEqual (ثابت IM3FuncType i_typeA, ثابت IM3FuncType i_typeB)
{
    إذا (i_typeA->isContinuation != i_typeB->isContinuation) {
        أرجع false;
    }
    إذا (i_typeA->isContinuation) {
        أرجع (i_typeA->contFuncType == i_typeB->contFuncType) or
               (i_typeA->contFuncType and i_typeB->contFuncType and
                AreFuncTypesEqual(i_typeA->contFuncType, i_typeB->contFuncType));
    }
    إذا (i_typeA->عدد_Rets == i_typeB->عدد_Rets and i_typeA->عدد_Args == i_typeB->عدد_Args) {
        أرجع (memcmp(i_typeA->types, i_typeB->types,
                       (i_typeA->عدد_Rets + i_typeA->عدد_Args) * حجم(m3type_t)) == 0);
    }

    أرجع false;
}

u16 Get_Func_Type_عدد_Params (ثابت IM3FuncType i_funcType)
{
    أرجع i_funcType ? i_funcType->عدد_Args : 0;
}


m3type_t GetFuncTypeParamType (ثابت IM3FuncType i_funcType, u16 i_index)
{
    m3type_t type = c_m3Type_unknown;

    إذا (i_funcType) {
        إذا (i_index < i_funcType->عدد_Args) {
            type = i_funcType->types[i_funcType->عدد_Rets + i_index];
        }
    }

    أرجع type;
}


u16 Get_Func_Type_عدد_Results (ثابت IM3FuncType i_funcType)
{
    أرجع i_funcType ? i_funcType->عدد_Rets : 0;
}


m3type_t GetFuncTypeResultType (ثابت IM3FuncType i_funcType, u16 i_index)
{
    m3type_t type = c_m3Type_unknown;

    إذا (i_funcType) {
        إذا (i_index < i_funcType->عدد_Rets) {
            type = i_funcType->types[i_index];
        }
    }

    أرجع type;
}


//---------------------------------------------------------------------------------------------------------------


فراغ FreeImportInfo (M3ImportInfo* i_info)
{
    m3_Free(i_info->moduleUtf8);
    m3_Free(i_info->fieldUtf8);
}


#إذا d_m3HasSnapshots

فراغ SnapshotMap_Free (M3SnapshotMap* i_map)
{
    إذا (i_map) {
        m3_Free(i_map->locals);
        m3_Free(i_map->safePoints);
        m3_Free(i_map->اقدار);
        m3_Free(i_map->blocks);
        m3_Free(i_map);
    }
}

#endif

فراغ Function_Release (IM3Function i_function)
{
    m3_Free(i_function->constants);

#إذا d_m3HasSnapshots
    SnapshotMap_Free(i_function->snapshotMap);
    i_function->snapshotMap = عدم;
#endif

    لكل (صحيح i = 0; i < i_function->عدد_Names; i++) {
        // name can be an alias of fieldUtf8
        إذا (i_function->names[i] != i_function->import.fieldUtf8) {
            m3_Free(i_function->names[i]);
        }
    }

    FreeImportInfo(&i_function->import);

    إذا (i_function->ownsWasmCode) {
        m3_Free(i_function->wasm);
    }

    // Function_FreeCompiledCode (func);

#إذا (d_m3EnableCodePageRefCounting)
    {
        m3_Free(i_function->codePageRefs);
        i_function->عدد_Code_Page_Refs = 0;
    }
#endif
}


فراغ Function_FreeCompiledCode (IM3Function i_function)
{
#إذا (d_m3EnableCodePageRefCounting)
    {
        i_function->compiled = عدم;

        طالما (i_function->عدد_Code_Page_Refs--) {
            IM3CodePage page = i_function->codePageRefs[i_function->عدد_Code_Page_Refs];

            إذا (--(page->info.usage_گنتی) == 0) {
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
    u16     عدد_Names = 0;
    cstr_t* names    = GetFunctionNames(i_function, &عدد_Names);
    إذا (عدد_Names > 0) {
        أرجع names[0];
    } وإلا {
        أرجع "<unnamed>";
    }
}


IM3Module m3_GetFunctionModule (IM3Function i_function)
{
    أرجع i_function ? i_function->module : عدم;
}


cstr_t* GetFunctionNames (IM3Function i_function, u16* o_عدد_Names)
{
    إذا (!i_function || !o_عدد_Names) {
        أرجع عدم;
    }

    إذا (i_function->import.fieldUtf8) {
        *o_عدد_Names = 1;
        أرجع &i_function->import.fieldUtf8;
    } وإلا {
        *o_عدد_Names = i_function->عدد_Names;
        أرجع i_function->names;
    }
}


cstr_t GetFunctionImportModuleName (IM3Function i_function)
{
    أرجع (i_function->import.moduleUtf8) ? i_function->import.moduleUtf8 : "";
}


u16 Get_Function_عدد_Args (IM3Function i_function)
{
    u16 عدد_Args = 0;

    إذا (i_function) {
        إذا (i_function->funcType) {
            عدد_Args = i_function->funcType->عدد_Args;
        }
    }

    أرجع عدد_Args;
}

m3type_t GetFunctionArgType (IM3Function i_function, u32 i_index)
{
    m3type_t type = c_m3Type_none;

    إذا (i_index < Get_Function_عدد_Args(i_function)) {
        u32 عدد_Returns = i_function->funcType->عدد_Rets;

        type = i_function->funcType->types[عدد_Returns + i_index];
    }

    أرجع type;
}


u16 Get_Function_عدد_Returns (IM3Function i_function)
{
    u16 عدد_Returns = 0;

    إذا (i_function) {
        إذا (i_function->funcType) {
            عدد_Returns = i_function->funcType->عدد_Rets;
        }
    }

    أرجع عدد_Returns;
}


u8 GetFunctionReturnType (ثابت IM3Function i_function, u16 i_index)
{
    أرجع i_function ? GetFuncTypeResultType(i_function->funcType, i_index) : c_m3Type_unknown;
}


u32 Get_Function_عدد_Args_And_Locals (IM3Function i_function)
{
    إذا (i_function) {
        أرجع i_function->عدد_Locals + Get_Function_عدد_Args(i_function);
    } وإلا {
        أرجع 0;
    }
}
