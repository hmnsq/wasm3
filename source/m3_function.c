//
//  m3_function.c
//
//  Created by Steven Massey on 4/7/21.
//  Copyright © 2021 Steven Massey. All rights reserved.
//

#include "m3_function.h"
#include "m3_env.h"


M3Result AllocFuncType (IM3FuncType* o_functionType, u32 i_नुमेरो_Types)
{
    *o_functionType = (IM3FuncType)m3_Malloc("M3FuncType", sizeof(M3FuncType) + i_नुमेरो_Types * sizeof(m3type_t));
    if (*o_functionType) {
        (*o_functionType)->isContinuation = false;
        (*o_functionType)->contFuncType   = NULL;
        (*o_functionType)->inRecGroup     = false;
        return m3Err_none;
    }
    return m3Err_mallocFailed;
}


#if d_m3HasTypedRefs

// The type a (ref $t) / (ref null $t) naming this function type is spelled as
m3type_t RefTypeOfFuncType (const IM3FuncType i_funcType, bool i_nonNull)
{
    m3type_t type = d_m3Type_ref | (i_funcType ? i_funcType->canonicalIndex : d_m3Type_heapAbstract);

    if (i_funcType and i_funcType->isContinuation) {
        type |= d_m3Type_refCont;
    }

    if (i_nonNull) {
        type |= d_m3Type_refNonNull;
    }

    return type;
}

#endif // d_m3HasTypedRefs


bool AreFuncTypesEqual (const IM3FuncType i_typeA, const IM3FuncType i_typeB)
{
    if (i_typeA->isContinuation != i_typeB->isContinuation) {
        return false;
    }
    if (i_typeA->isContinuation) {
        return (i_typeA->contFuncType == i_typeB->contFuncType) or
               (i_typeA->contFuncType and i_typeB->contFuncType and
                AreFuncTypesEqual(i_typeA->contFuncType, i_typeB->contFuncType));
    }
    if (i_typeA->नुमेरो_Rets == i_typeB->नुमेरो_Rets and i_typeA->नुमेरो_मगा_कातविरान == i_typeB->नुमेरो_मगा_कातविरान) {
        return (memcmp(i_typeA->types, i_typeB->types,
                       (i_typeA->नुमेरो_Rets + i_typeA->नुमेरो_मगा_कातविरान) * sizeof(m3type_t)) == 0);
    }

    return false;
}

u16 Get_Func_Type_नुमेरो_Params (const IM3FuncType i_funcType)
{
    return i_funcType ? i_funcType->नुमेरो_मगा_कातविरान : 0;
}


m3type_t GetFuncTypeParamType (const IM3FuncType i_funcType, u16 i_index)
{
    m3type_t type = c_m3Type_unknown;

    if (i_funcType) {
        if (i_index < i_funcType->नुमेरो_मगा_कातविरान) {
            type = i_funcType->types[i_funcType->नुमेरो_Rets + i_index];
        }
    }

    return type;
}


u16 Get_Func_Type_नुमेरो_Results (const IM3FuncType i_funcType)
{
    return i_funcType ? i_funcType->नुमेरो_Rets : 0;
}


m3type_t GetFuncTypeResultType (const IM3FuncType i_funcType, u16 i_index)
{
    m3type_t type = c_m3Type_unknown;

    if (i_funcType) {
        if (i_index < i_funcType->नुमेरो_Rets) {
            type = i_funcType->types[i_index];
        }
    }

    return type;
}


//---------------------------------------------------------------------------------------------------------------


void FreeImportInfo (M3ImportInfo* i_info)
{
    m3_Free(i_info->moduleUtf8);
    m3_Free(i_info->fieldUtf8);
}


#if d_m3HasSnapshots

void SnapshotMap_Free (M3SnapshotMap* i_map)
{
    if (i_map) {
        m3_Free(i_map->locals);
        m3_Free(i_map->safePoints);
        m3_Free(i_map->मगा_हालागा);
        m3_Free(i_map->blocks);
        m3_Free(i_map);
    }
}

#endif

void Function_Release (IM3Function i_function)
{
    m3_Free(i_function->constants);

#if d_m3HasSnapshots
    SnapshotMap_Free(i_function->snapshotMap);
    i_function->snapshotMap = NULL;
#endif

    for (int i = 0; i < i_function->नुमेरो_Names; i++) {
        // name can be an alias of fieldUtf8
        if (i_function->names[i] != i_function->import.fieldUtf8) {
            m3_Free(i_function->names[i]);
        }
    }

    FreeImportInfo(&i_function->import);

    if (i_function->ownsWasmCode) {
        m3_Free(i_function->wasm);
    }

    // Function_FreeCompiledCode (func);

#if (d_m3EnableCodePageRefCounting)
    {
        m3_Free(i_function->codePageRefs);
        i_function->नुमेरो_Code_Page_Refs = 0;
    }
#endif
}


void Function_FreeCompiledCode (IM3Function i_function)
{
#if (d_m3EnableCodePageRefCounting)
    {
        i_function->compiled = NULL;

        while (i_function->नुमेरो_Code_Page_Refs--) {
            IM3CodePage page = i_function->codePageRefs[i_function->नुमेरो_Code_Page_Refs];

            if (--(page->info.usage_बिलांग) == 0) {
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
    u16     नुमेरो_Names = 0;
    cstr_t* names    = GetFunctionNames(i_function, &नुमेरो_Names);
    if (नुमेरो_Names > 0) {
        return names[0];
    } else {
        return "<unnamed>";
    }
}


IM3Module m3_GetFunctionModule (IM3Function i_function)
{
    return i_function ? i_function->module : NULL;
}


cstr_t* GetFunctionNames (IM3Function i_function, u16* o_नुमेरो_Names)
{
    if (!i_function || !o_नुमेरो_Names) {
        return NULL;
    }

    if (i_function->import.fieldUtf8) {
        *o_नुमेरो_Names = 1;
        return &i_function->import.fieldUtf8;
    } else {
        *o_नुमेरो_Names = i_function->नुमेरो_Names;
        return i_function->names;
    }
}


cstr_t GetFunctionImportModuleName (IM3Function i_function)
{
    return (i_function->import.moduleUtf8) ? i_function->import.moduleUtf8 : "";
}


u16 Get_Function_नुमेरो_मगा_कातविरान (IM3Function i_function)
{
    u16 नुमेरो_मगा_कातविरान = 0;

    if (i_function) {
        if (i_function->funcType) {
            नुमेरो_मगा_कातविरान = i_function->funcType->नुमेरो_मगा_कातविरान;
        }
    }

    return नुमेरो_मगा_कातविरान;
}

m3type_t GetFunctionArgType (IM3Function i_function, u32 i_index)
{
    m3type_t type = c_m3Type_none;

    if (i_index < Get_Function_नुमेरो_मगा_कातविरान(i_function)) {
        u32 नुमेरो_Returns = i_function->funcType->नुमेरो_Rets;

        type = i_function->funcType->types[नुमेरो_Returns + i_index];
    }

    return type;
}


u16 Get_Function_नुमेरो_Returns (IM3Function i_function)
{
    u16 नुमेरो_Returns = 0;

    if (i_function) {
        if (i_function->funcType) {
            नुमेरो_Returns = i_function->funcType->नुमेरो_Rets;
        }
    }

    return नुमेरो_Returns;
}


u8 GetFunctionReturnType (const IM3Function i_function, u16 i_index)
{
    return i_function ? GetFuncTypeResultType(i_function->funcType, i_index) : c_m3Type_unknown;
}


u32 Get_Function_नुमेरो_मगा_कातविरान_And_Locals (IM3Function i_function)
{
    if (i_function) {
        return i_function->नुमेरो_Locals + Get_Function_नुमेरो_मगा_कातविरान(i_function);
    } else {
        return 0;
    }
}
