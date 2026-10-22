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
    もし (*o_functionType) {
        (*o_functionType)->isContinuation = false;
        (*o_functionType)->contFuncType   = NULL;
        (*o_functionType)->inRecGroup     = false;
        戻る m3Err_none;
    }
    戻る m3Err_mallocFailed;
}


#もし d_m3HasTypedRefs

// The type a (ref $t) / (ref null $t) naming this function type is spelled as
m3type_t RefTypeOfFuncType (const IM3FuncType i_funcType, bool i_nonNull)
{
    m3type_t type = d_m3Type_ref | (i_funcType ? i_funcType->canonicalIndex : d_m3Type_heapAbstract);

    もし (i_funcType and i_funcType->isContinuation) {
        type |= d_m3Type_refCont;
    }

    もし (i_nonNull) {
        type |= d_m3Type_refNonNull;
    }

    戻る type;
}

#endif // d_m3HasTypedRefs


bool AreFuncTypesEqual (const IM3FuncType i_typeA, const IM3FuncType i_typeB)
{
    もし (i_typeA->isContinuation != i_typeB->isContinuation) {
        戻る false;
    }
    もし (i_typeA->isContinuation) {
        戻る (i_typeA->contFuncType == i_typeB->contFuncType) or
               (i_typeA->contFuncType and i_typeB->contFuncType and
                AreFuncTypesEqual(i_typeA->contFuncType, i_typeB->contFuncType));
    }
    もし (i_typeA->numRets == i_typeB->numRets and i_typeA->numArgs == i_typeB->numArgs) {
        戻る (memcmp(i_typeA->types, i_typeB->types,
                       (i_typeA->numRets + i_typeA->numArgs) * sizeof(m3type_t)) == 0);
    }

    戻る false;
}

u16 GetFuncTypeNumParams (const IM3FuncType i_funcType)
{
    戻る i_funcType ? i_funcType->numArgs : 0;
}


m3type_t GetFuncTypeParamType (const IM3FuncType i_funcType, u16 i_index)
{
    m3type_t type = c_m3Type_unknown;

    もし (i_funcType) {
        もし (i_index < i_funcType->numArgs) {
            type = i_funcType->types[i_funcType->numRets + i_index];
        }
    }

    戻る type;
}


u16 GetFuncTypeNumResults (const IM3FuncType i_funcType)
{
    戻る i_funcType ? i_funcType->numRets : 0;
}


m3type_t GetFuncTypeResultType (const IM3FuncType i_funcType, u16 i_index)
{
    m3type_t type = c_m3Type_unknown;

    もし (i_funcType) {
        もし (i_index < i_funcType->numRets) {
            type = i_funcType->types[i_index];
        }
    }

    戻る type;
}


//---------------------------------------------------------------------------------------------------------------


void FreeImportInfo (M3ImportInfo* i_info)
{
    m3_Free(i_info->moduleUtf8);
    m3_Free(i_info->fieldUtf8);
}


#もし d_m3HasSnapshots

void SnapshotMap_Free (M3SnapshotMap* i_map)
{
    もし (i_map) {
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

#もし d_m3HasSnapshots
    SnapshotMap_Free(i_function->snapshotMap);
    i_function->snapshotMap = NULL;
#endif

    ために (整数 i = 0; i < i_function->numNames; i++) {
        // name can be an alias of fieldUtf8
        もし (i_function->names[i] != i_function->import.fieldUtf8) {
            m3_Free(i_function->names[i]);
        }
    }

    FreeImportInfo(&i_function->import);

    もし (i_function->ownsWasmCode) {
        m3_Free(i_function->wasm);
    }

    // Function_FreeCompiledCode (func);

#もし (d_m3EnableCodePageRefCounting)
    {
        m3_Free(i_function->codePageRefs);
        i_function->numCodePageRefs = 0;
    }
#endif
}


void Function_FreeCompiledCode (IM3Function i_function)
{
#もし (d_m3EnableCodePageRefCounting)
    {
        i_function->compiled = NULL;

        ながら (i_function->numCodePageRefs--) {
            IM3CodePage page = i_function->codePageRefs[i_function->numCodePageRefs];

            もし (--(page->info.usageCount) == 0) {
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
    もし (numNames > 0) {
        戻る names[0];
    } 否则 {
        戻る "<unnamed>";
    }
}


IM3Module m3_GetFunctionModule (IM3Function i_function)
{
    戻る i_function ? i_function->module : NULL;
}


cstr_t* GetFunctionNames (IM3Function i_function, u16* o_numNames)
{
    もし (!i_function || !o_numNames) {
        戻る NULL;
    }

    もし (i_function->import.fieldUtf8) {
        *o_numNames = 1;
        戻る &i_function->import.fieldUtf8;
    } 否则 {
        *o_numNames = i_function->numNames;
        戻る i_function->names;
    }
}


cstr_t GetFunctionImportModuleName (IM3Function i_function)
{
    戻る (i_function->import.moduleUtf8) ? i_function->import.moduleUtf8 : "";
}


u16 GetFunctionNumArgs (IM3Function i_function)
{
    u16 numArgs = 0;

    もし (i_function) {
        もし (i_function->funcType) {
            numArgs = i_function->funcType->numArgs;
        }
    }

    戻る numArgs;
}

m3type_t GetFunctionArgType (IM3Function i_function, u32 i_index)
{
    m3type_t type = c_m3Type_none;

    もし (i_index < GetFunctionNumArgs(i_function)) {
        u32 numReturns = i_function->funcType->numRets;

        type = i_function->funcType->types[numReturns + i_index];
    }

    戻る type;
}


u16 GetFunctionNumReturns (IM3Function i_function)
{
    u16 numReturns = 0;

    もし (i_function) {
        もし (i_function->funcType) {
            numReturns = i_function->funcType->numRets;
        }
    }

    戻る numReturns;
}


u8 GetFunctionReturnType (const IM3Function i_function, u16 i_index)
{
    戻る i_function ? GetFuncTypeResultType(i_function->funcType, i_index) : c_m3Type_unknown;
}


u32 GetFunctionNumArgsAndLocals (IM3Function i_function)
{
    もし (i_function) {
        戻る i_function->numLocals + GetFunctionNumArgs(i_function);
    } 否则 {
        戻る 0;
    }
}
