//
//  m3_function.c
//
//  Created by Steven Massey on 4/7/21.
//  Copyright © 2021 Steven Massey. All rights reserved.
//

#அந்தர்பாவ "m3_function.h"
#அந்தர்பாவ "m3_env.h"


M3Result AllocFuncType (IM3FuncType* o_functionType, u32 i_இலக்கம்_Types)
{
    *o_functionType = (IM3FuncType)m3_Malloc("M3FuncType", அளவு(M3FuncType) + i_இலக்கம்_Types * அளவு(m3type_t));
    எனில் (*o_functionType) {
        (*o_functionType)->isContinuation = false;
        (*o_functionType)->contFuncType   = வெற்று;
        (*o_functionType)->inRecGroup     = false;
        திரும்பு m3Err_none;
    }
    திரும்பு m3Err_mallocFailed;
}


#எனில் d_m3HasTypedRefs

// The type a (ref $t) / (ref null $t) naming this function type is spelled as
m3type_t RefTypeOfFuncType (மாறிலி IM3FuncType i_funcType, bool i_nonNull)
{
    m3type_t type = d_m3Type_ref | (i_funcType ? i_funcType->canonicalIndex : d_m3Type_heapAbstract);

    எனில் (i_funcType and i_funcType->isContinuation) {
        type |= d_m3Type_refCont;
    }

    எனில் (i_nonNull) {
        type |= d_m3Type_refNonNull;
    }

    திரும்பு type;
}

#endif // d_m3HasTypedRefs


bool AreFuncTypesEqual (மாறிலி IM3FuncType i_typeA, மாறிலி IM3FuncType i_typeB)
{
    எனில் (i_typeA->isContinuation != i_typeB->isContinuation) {
        திரும்பு false;
    }
    எனில் (i_typeA->isContinuation) {
        திரும்பு (i_typeA->contFuncType == i_typeB->contFuncType) or
               (i_typeA->contFuncType and i_typeB->contFuncType and
                AreFuncTypesEqual(i_typeA->contFuncType, i_typeB->contFuncType));
    }
    எனில் (i_typeA->இலக்கம்_Rets == i_typeB->இலக்கம்_Rets and i_typeA->இலக்கம்_Args == i_typeB->இலக்கம்_Args) {
        திரும்பு (memcmp(i_typeA->types, i_typeB->types,
                       (i_typeA->இலக்கம்_Rets + i_typeA->இலக்கம்_Args) * அளவு(m3type_t)) == 0);
    }

    திரும்பு false;
}

u16 Get_Func_Type_இலக்கம்_Params (மாறிலி IM3FuncType i_funcType)
{
    திரும்பு i_funcType ? i_funcType->இலக்கம்_Args : 0;
}


m3type_t GetFuncTypeParamType (மாறிலி IM3FuncType i_funcType, u16 i_index)
{
    m3type_t type = c_m3Type_unknown;

    எனில் (i_funcType) {
        எனில் (i_index < i_funcType->இலக்கம்_Args) {
            type = i_funcType->types[i_funcType->இலக்கம்_Rets + i_index];
        }
    }

    திரும்பு type;
}


u16 Get_Func_Type_இலக்கம்_Results (மாறிலி IM3FuncType i_funcType)
{
    திரும்பு i_funcType ? i_funcType->இலக்கம்_Rets : 0;
}


m3type_t GetFuncTypeResultType (மாறிலி IM3FuncType i_funcType, u16 i_index)
{
    m3type_t type = c_m3Type_unknown;

    எனில் (i_funcType) {
        எனில் (i_index < i_funcType->இலக்கம்_Rets) {
            type = i_funcType->types[i_index];
        }
    }

    திரும்பு type;
}


//---------------------------------------------------------------------------------------------------------------


வெறுமை FreeImportInfo (M3ImportInfo* i_info)
{
    m3_Free(i_info->moduleUtf8);
    m3_Free(i_info->fieldUtf8);
}


#எனில் d_m3HasSnapshots

வெறுமை SnapshotMap_Free (M3SnapshotMap* i_map)
{
    எனில் (i_map) {
        m3_Free(i_map->locals);
        m3_Free(i_map->safePoints);
        m3_Free(i_map->மதிப்புகள்);
        m3_Free(i_map->blocks);
        m3_Free(i_map);
    }
}

#endif

வெறுமை Function_Release (IM3Function i_function)
{
    m3_Free(i_function->constants);

#எனில் d_m3HasSnapshots
    SnapshotMap_Free(i_function->snapshotMap);
    i_function->snapshotMap = வெற்று;
#endif

    க்கு (முழு i = 0; i < i_function->இலக்கம்_Names; i++) {
        // name can be an alias of fieldUtf8
        எனில் (i_function->names[i] != i_function->import.fieldUtf8) {
            m3_Free(i_function->names[i]);
        }
    }

    FreeImportInfo(&i_function->import);

    எனில் (i_function->ownsWasmCode) {
        m3_Free(i_function->wasm);
    }

    // Function_FreeCompiledCode (func);

#எனில் (d_m3EnableCodePageRefCounting)
    {
        m3_Free(i_function->codePageRefs);
        i_function->இலக்கம்_Code_Page_Refs = 0;
    }
#endif
}


வெறுமை Function_FreeCompiledCode (IM3Function i_function)
{
#எனில் (d_m3EnableCodePageRefCounting)
    {
        i_function->compiled = வெற்று;

        போது (i_function->இலக்கம்_Code_Page_Refs--) {
            IM3CodePage page = i_function->codePageRefs[i_function->இலக்கம்_Code_Page_Refs];

            எனில் (--(page->info.usage_மொத்தம்) == 0) {
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
    u16     இலக்கம்_Names = 0;
    cstr_t* names    = GetFunctionNames(i_function, &இலக்கம்_Names);
    எனில் (இலக்கம்_Names > 0) {
        திரும்பு names[0];
    } இல்லையெனில் {
        திரும்பு "<unnamed>";
    }
}


IM3Module m3_GetFunctionModule (IM3Function i_function)
{
    திரும்பு i_function ? i_function->module : வெற்று;
}


cstr_t* GetFunctionNames (IM3Function i_function, u16* o_இலக்கம்_Names)
{
    எனில் (!i_function || !o_இலக்கம்_Names) {
        திரும்பு வெற்று;
    }

    எனில் (i_function->import.fieldUtf8) {
        *o_இலக்கம்_Names = 1;
        திரும்பு &i_function->import.fieldUtf8;
    } இல்லையெனில் {
        *o_இலக்கம்_Names = i_function->இலக்கம்_Names;
        திரும்பு i_function->names;
    }
}


cstr_t GetFunctionImportModuleName (IM3Function i_function)
{
    திரும்பு (i_function->import.moduleUtf8) ? i_function->import.moduleUtf8 : "";
}


u16 Get_Function_இலக்கம்_Args (IM3Function i_function)
{
    u16 இலக்கம்_Args = 0;

    எனில் (i_function) {
        எனில் (i_function->funcType) {
            இலக்கம்_Args = i_function->funcType->இலக்கம்_Args;
        }
    }

    திரும்பு இலக்கம்_Args;
}

m3type_t GetFunctionArgType (IM3Function i_function, u32 i_index)
{
    m3type_t type = c_m3Type_none;

    எனில் (i_index < Get_Function_இலக்கம்_Args(i_function)) {
        u32 இலக்கம்_Returns = i_function->funcType->இலக்கம்_Rets;

        type = i_function->funcType->types[இலக்கம்_Returns + i_index];
    }

    திரும்பு type;
}


u16 Get_Function_இலக்கம்_Returns (IM3Function i_function)
{
    u16 இலக்கம்_Returns = 0;

    எனில் (i_function) {
        எனில் (i_function->funcType) {
            இலக்கம்_Returns = i_function->funcType->இலக்கம்_Rets;
        }
    }

    திரும்பு இலக்கம்_Returns;
}


u8 GetFunctionReturnType (மாறிலி IM3Function i_function, u16 i_index)
{
    திரும்பு i_function ? GetFuncTypeResultType(i_function->funcType, i_index) : c_m3Type_unknown;
}


u32 Get_Function_இலக்கம்_Args_And_Locals (IM3Function i_function)
{
    எனில் (i_function) {
        திரும்பு i_function->இலக்கம்_Locals + Get_Function_இலக்கம்_Args(i_function);
    } இல்லையெனில் {
        திரும்பு 0;
    }
}
