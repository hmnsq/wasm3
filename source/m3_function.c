//
//  m3_function.c
//
//  Created by Steven Massey on 4/7/21.
//  Copyright © 2021 Steven Massey. All rights reserved.
//

#অন্তর্ভাব "m3_function.h"
#অন্তর্ভাব "m3_env.h"


M3Result AllocFuncType (IM3FuncType* o_functionType, u32 i_অঙ্ক_Types)
{
    *o_functionType = (IM3FuncType)m3_Malloc("M3FuncType", পরিমাণ(M3FuncType) + i_অঙ্ক_Types * পরিমাণ(m3type_t));
    যদি (*o_functionType) {
        (*o_functionType)->isContinuation = false;
        (*o_functionType)->contFuncType   = রিক্ত;
        (*o_functionType)->inRecGroup     = false;
        প্রত্যাগম m3Err_none;
    }
    প্রত্যাগম m3Err_mallocFailed;
}


#যদি d_m3HasTypedRefs

// The type a (ref $t) / (ref null $t) naming this function type is spelled as
m3type_t RefTypeOfFuncType (ধ্রুব IM3FuncType i_funcType, bool i_nonNull)
{
    m3type_t type = d_m3Type_ref | (i_funcType ? i_funcType->canonicalIndex : d_m3Type_heapAbstract);

    যদি (i_funcType and i_funcType->isContinuation) {
        type |= d_m3Type_refCont;
    }

    যদি (i_nonNull) {
        type |= d_m3Type_refNonNull;
    }

    প্রত্যাগম type;
}

#endif // d_m3HasTypedRefs


bool AreFuncTypesEqual (ধ্রুব IM3FuncType i_typeA, ধ্রুব IM3FuncType i_typeB)
{
    যদি (i_typeA->isContinuation != i_typeB->isContinuation) {
        প্রত্যাগম false;
    }
    যদি (i_typeA->isContinuation) {
        প্রত্যাগম (i_typeA->contFuncType == i_typeB->contFuncType) or
               (i_typeA->contFuncType and i_typeB->contFuncType and
                AreFuncTypesEqual(i_typeA->contFuncType, i_typeB->contFuncType));
    }
    যদি (i_typeA->অঙ্ক_Rets == i_typeB->অঙ্ক_Rets and i_typeA->অঙ্ক_Args == i_typeB->অঙ্ক_Args) {
        প্রত্যাগম (memcmp(i_typeA->types, i_typeB->types,
                       (i_typeA->অঙ্ক_Rets + i_typeA->অঙ্ক_Args) * পরিমাণ(m3type_t)) == 0);
    }

    প্রত্যাগম false;
}

u16 Get_Func_Type_অঙ্ক_Params (ধ্রুব IM3FuncType i_funcType)
{
    প্রত্যাগম i_funcType ? i_funcType->অঙ্ক_Args : 0;
}


m3type_t GetFuncTypeParamType (ধ্রুব IM3FuncType i_funcType, u16 i_index)
{
    m3type_t type = c_m3Type_unknown;

    যদি (i_funcType) {
        যদি (i_index < i_funcType->অঙ্ক_Args) {
            type = i_funcType->types[i_funcType->অঙ্ক_Rets + i_index];
        }
    }

    প্রত্যাগম type;
}


u16 Get_Func_Type_অঙ্ক_Results (ধ্রুব IM3FuncType i_funcType)
{
    প্রত্যাগম i_funcType ? i_funcType->অঙ্ক_Rets : 0;
}


m3type_t GetFuncTypeResultType (ধ্রুব IM3FuncType i_funcType, u16 i_index)
{
    m3type_t type = c_m3Type_unknown;

    যদি (i_funcType) {
        যদি (i_index < i_funcType->অঙ্ক_Rets) {
            type = i_funcType->types[i_index];
        }
    }

    প্রত্যাগম type;
}


//---------------------------------------------------------------------------------------------------------------


শূন্য FreeImportInfo (M3ImportInfo* i_info)
{
    m3_Free(i_info->moduleUtf8);
    m3_Free(i_info->fieldUtf8);
}


#যদি d_m3HasSnapshots

শূন্য SnapshotMap_Free (M3SnapshotMap* i_map)
{
    যদি (i_map) {
        m3_Free(i_map->locals);
        m3_Free(i_map->safePoints);
        m3_Free(i_map->মানসমূহ);
        m3_Free(i_map->blocks);
        m3_Free(i_map);
    }
}

#endif

শূন্য Function_Release (IM3Function i_function)
{
    m3_Free(i_function->constants);

#যদি d_m3HasSnapshots
    SnapshotMap_Free(i_function->snapshotMap);
    i_function->snapshotMap = রিক্ত;
#endif

    প্রতি (পূর্ণাঙ্ক i = 0; i < i_function->অঙ্ক_Names; i++) {
        // name can be an alias of fieldUtf8
        যদি (i_function->names[i] != i_function->import.fieldUtf8) {
            m3_Free(i_function->names[i]);
        }
    }

    FreeImportInfo(&i_function->import);

    যদি (i_function->ownsWasmCode) {
        m3_Free(i_function->wasm);
    }

    // Function_FreeCompiledCode (func);

#যদি (d_m3EnableCodePageRefCounting)
    {
        m3_Free(i_function->codePageRefs);
        i_function->অঙ্ক_Code_Page_Refs = 0;
    }
#endif
}


শূন্য Function_FreeCompiledCode (IM3Function i_function)
{
#যদি (d_m3EnableCodePageRefCounting)
    {
        i_function->compiled = রিক্ত;

        যাবত্ (i_function->অঙ্ক_Code_Page_Refs--) {
            IM3CodePage page = i_function->codePageRefs[i_function->অঙ্ক_Code_Page_Refs];

            যদি (--(page->info.usage_গুনতি) == 0) {
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
    u16     অঙ্ক_Names = 0;
    cstr_t* names    = GetFunctionNames(i_function, &অঙ্ক_Names);
    যদি (অঙ্ক_Names > 0) {
        প্রত্যাগম names[0];
    } অন্যথা {
        প্রত্যাগম "<unnamed>";
    }
}


IM3Module m3_GetFunctionModule (IM3Function i_function)
{
    প্রত্যাগম i_function ? i_function->module : রিক্ত;
}


cstr_t* GetFunctionNames (IM3Function i_function, u16* o_অঙ্ক_Names)
{
    যদি (!i_function || !o_অঙ্ক_Names) {
        প্রত্যাগম রিক্ত;
    }

    যদি (i_function->import.fieldUtf8) {
        *o_অঙ্ক_Names = 1;
        প্রত্যাগম &i_function->import.fieldUtf8;
    } অন্যথা {
        *o_অঙ্ক_Names = i_function->অঙ্ক_Names;
        প্রত্যাগম i_function->names;
    }
}


cstr_t GetFunctionImportModuleName (IM3Function i_function)
{
    প্রত্যাগম (i_function->import.moduleUtf8) ? i_function->import.moduleUtf8 : "";
}


u16 Get_Function_অঙ্ক_Args (IM3Function i_function)
{
    u16 অঙ্ক_Args = 0;

    যদি (i_function) {
        যদি (i_function->funcType) {
            অঙ্ক_Args = i_function->funcType->অঙ্ক_Args;
        }
    }

    প্রত্যাগম অঙ্ক_Args;
}

m3type_t GetFunctionArgType (IM3Function i_function, u32 i_index)
{
    m3type_t type = c_m3Type_none;

    যদি (i_index < Get_Function_অঙ্ক_Args(i_function)) {
        u32 অঙ্ক_Returns = i_function->funcType->অঙ্ক_Rets;

        type = i_function->funcType->types[অঙ্ক_Returns + i_index];
    }

    প্রত্যাগম type;
}


u16 Get_Function_অঙ্ক_Returns (IM3Function i_function)
{
    u16 অঙ্ক_Returns = 0;

    যদি (i_function) {
        যদি (i_function->funcType) {
            অঙ্ক_Returns = i_function->funcType->অঙ্ক_Rets;
        }
    }

    প্রত্যাগম অঙ্ক_Returns;
}


u8 GetFunctionReturnType (ধ্রুব IM3Function i_function, u16 i_index)
{
    প্রত্যাগম i_function ? GetFuncTypeResultType(i_function->funcType, i_index) : c_m3Type_unknown;
}


u32 Get_Function_অঙ্ক_Args_And_Locals (IM3Function i_function)
{
    যদি (i_function) {
        প্রত্যাগম i_function->অঙ্ক_Locals + Get_Function_অঙ্ক_Args(i_function);
    } অন্যথা {
        প্রত্যাগম 0;
    }
}
