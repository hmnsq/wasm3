//
//  m3_function.c
//
//  Created by Steven Massey on 4/7/21.
//  Copyright © 2021 Steven Massey. All rights reserved.
//

#include "m3_function.h"
#include "m3_env.h"


M3Result AllocFuncType (IM3FuncType* o_functionType, u32 i_numTypes)
{
    *o_functionType = (IM3FuncType)m3_Malloc("M3FuncType", sizeof(M3FuncType) + i_numTypes * sizeof(m3type_t));
    如果 (*o_functionType) {
        (*o_functionType)->isContinuation = false;
        (*o_functionType)->contFuncType   = NULL;
        (*o_functionType)->inRecGroup     = false;
        返回 m3Err_none;
    }
    返回 m3Err_mallocFailed;
}


#如果 d_m3HasTypedRefs

// The type a (ref $t) / (ref null $t) naming this function type is spelled as
m3type_t RefTypeOfFuncType (const IM3FuncType i_funcType, bool i_nonNull)
{
    m3type_t type = d_m3Type_ref | (i_funcType ? i_funcType->canonicalIndex : d_m3Type_heapAbstract);

    如果 (i_funcType and i_funcType->isContinuation) {
        type |= d_m3Type_refCont;
    }

    如果 (i_nonNull) {
        type |= d_m3Type_refNonNull;
    }

    返回 type;
}

#endif // d_m3HasTypedRefs


bool AreFuncTypesEqual (const IM3FuncType i_typeA, const IM3FuncType i_typeB)
{
    如果 (i_typeA->isContinuation != i_typeB->isContinuation) {
        返回 false;
    }
    如果 (i_typeA->isContinuation) {
        返回 (i_typeA->contFuncType == i_typeB->contFuncType) or
               (i_typeA->contFuncType and i_typeB->contFuncType and
                AreFuncTypesEqual(i_typeA->contFuncType, i_typeB->contFuncType));
    }
    如果 (i_typeA->numRets == i_typeB->numRets and i_typeA->numArgs == i_typeB->numArgs) {
        返回 (memcmp(i_typeA->types, i_typeB->types,
                       (i_typeA->numRets + i_typeA->numArgs) * sizeof(m3type_t)) == 0);
    }

    返回 false;
}

u16 GetFuncTypeNumParams (const IM3FuncType i_funcType)
{
    返回 i_funcType ? i_funcType->numArgs : 0;
}


m3type_t GetFuncTypeParamType (const IM3FuncType i_funcType, u16 i_index)
{
    m3type_t type = c_m3Type_unknown;

    如果 (i_funcType) {
        如果 (i_index < i_funcType->numArgs) {
            type = i_funcType->types[i_funcType->numRets + i_index];
        }
    }

    返回 type;
}


u16 GetFuncTypeNumResults (const IM3FuncType i_funcType)
{
    返回 i_funcType ? i_funcType->numRets : 0;
}


m3type_t GetFuncTypeResultType (const IM3FuncType i_funcType, u16 i_index)
{
    m3type_t type = c_m3Type_unknown;

    如果 (i_funcType) {
        如果 (i_index < i_funcType->numRets) {
            type = i_funcType->types[i_index];
        }
    }

    返回 type;
}


//---------------------------------------------------------------------------------------------------------------


void FreeImportInfo (M3ImportInfo* i_info)
{
    m3_Free(i_info->moduleUtf8);
    m3_Free(i_info->fieldUtf8);
}


#如果 d_m3HasSnapshots

void SnapshotMap_Free (M3SnapshotMap* i_map)
{
    如果 (i_map) {
        m3_Free(i_map->locals);
        m3_Free(i_map->safePoints);
        m3_Free(i_map->values);
        m3_Free(i_map->blocks);
        m3_Free(i_map);
    }
}

#endif

void Function_Release (IM3Function i_function)
{
    m3_Free(i_function->constants);

#如果 d_m3HasSnapshots
    SnapshotMap_Free(i_function->snapshotMap);
    i_function->snapshotMap = NULL;
#endif

    为 (整数 i = 0; i < i_function->numNames; i++) {
        // name can be an alias of fieldUtf8
        如果 (i_function->names[i] != i_function->import.fieldUtf8) {
            m3_Free(i_function->names[i]);
        }
    }

    FreeImportInfo(&i_function->import);

    如果 (i_function->ownsWasmCode) {
        m3_Free(i_function->wasm);
    }

    // Function_FreeCompiledCode (func);

#如果 (d_m3EnableCodePageRefCounting)
    {
        m3_Free(i_function->codePageRefs);
        i_function->numCodePageRefs = 0;
    }
#endif
}


void Function_FreeCompiledCode (IM3Function i_function)
{
#如果 (d_m3EnableCodePageRefCounting)
    {
        i_function->compiled = NULL;

        当 (i_function->numCodePageRefs--) {
            IM3CodePage page = i_function->codePageRefs[i_function->numCodePageRefs];

            如果 (--(page->info.usageCount) == 0) {
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
    u16     numNames = 0;
    cstr_t* names    = GetFunctionNames(i_function, &numNames);
    如果 (numNames > 0) {
        返回 names[0];
    } 否则 {
        返回 "<unnamed>";
    }
}


IM3Module m3_GetFunctionModule (IM3Function i_function)
{
    返回 i_function ? i_function->module : NULL;
}


cstr_t* GetFunctionNames (IM3Function i_function, u16* o_numNames)
{
    如果 (!i_function || !o_numNames) {
        返回 NULL;
    }

    如果 (i_function->import.fieldUtf8) {
        *o_numNames = 1;
        返回 &i_function->import.fieldUtf8;
    } 否则 {
        *o_numNames = i_function->numNames;
        返回 i_function->names;
    }
}


cstr_t GetFunctionImportModuleName (IM3Function i_function)
{
    返回 (i_function->import.moduleUtf8) ? i_function->import.moduleUtf8 : "";
}


u16 GetFunctionNumArgs (IM3Function i_function)
{
    u16 numArgs = 0;

    如果 (i_function) {
        如果 (i_function->funcType) {
            numArgs = i_function->funcType->numArgs;
        }
    }

    返回 numArgs;
}

m3type_t GetFunctionArgType (IM3Function i_function, u32 i_index)
{
    m3type_t type = c_m3Type_none;

    如果 (i_index < GetFunctionNumArgs(i_function)) {
        u32 numReturns = i_function->funcType->numRets;

        type = i_function->funcType->types[numReturns + i_index];
    }

    返回 type;
}


u16 GetFunctionNumReturns (IM3Function i_function)
{
    u16 numReturns = 0;

    如果 (i_function) {
        如果 (i_function->funcType) {
            numReturns = i_function->funcType->numRets;
        }
    }

    返回 numReturns;
}


u8 GetFunctionReturnType (const IM3Function i_function, u16 i_index)
{
    返回 i_function ? GetFuncTypeResultType(i_function->funcType, i_index) : c_m3Type_unknown;
}


u32 GetFunctionNumArgsAndLocals (IM3Function i_function)
{
    如果 (i_function) {
        返回 i_function->numLocals + GetFunctionNumArgs(i_function);
    } 否则 {
        返回 0;
    }
}
