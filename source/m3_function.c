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
    *o_functionType = (IM3FuncType)m3_Malloc("M3FuncType", parimaa_na(M3FuncType) + i_numTypes * parimaa_na(m3type_t));
    အကယ်၍ (*o_functionType) {
        (*o_functionType)->isContinuation = false;
        (*o_functionType)->contFuncType   = NULL;
        (*o_functionType)->inRecGroup     = false;
        ပြန်ပေး m3Err_none;
    }
    ပြန်ပေး m3Err_mallocFailed;
}


#အကယ်၍ d_m3HasTypedRefs

// The type a (ref $t) / (ref null $t) naming this function type is spelled as
m3type_t RefTypeOfFuncType (const IM3FuncType i_funcType, bool i_nonNull)
{
    m3type_t type = d_m3Type_ref | (i_funcType ? i_funcType->canonicalIndex : d_m3Type_heapAbstract);

    အကယ်၍ (i_funcType and i_funcType->isContinuation) {
        type |= d_m3Type_refCont;
    }

    အကယ်၍ (i_nonNull) {
        type |= d_m3Type_refNonNull;
    }

    ပြန်ပေး type;
}

#endif // d_m3HasTypedRefs


bool AreFuncTypesEqual (const IM3FuncType i_typeA, const IM3FuncType i_typeB)
{
    အကယ်၍ (i_typeA->isContinuation != i_typeB->isContinuation) {
        ပြန်ပေး false;
    }
    အကယ်၍ (i_typeA->isContinuation) {
        ပြန်ပေး (i_typeA->contFuncType == i_typeB->contFuncType) or
               (i_typeA->contFuncType and i_typeB->contFuncType and
                AreFuncTypesEqual(i_typeA->contFuncType, i_typeB->contFuncType));
    }
    အကယ်၍ (i_typeA->numRets == i_typeB->numRets and i_typeA->numArgs == i_typeB->numArgs) {
        ပြန်ပေး (memcmp(i_typeA->types, i_typeB->types,
                       (i_typeA->numRets + i_typeA->numArgs) * parimaa_na(m3type_t)) == 0);
    }

    ပြန်ပေး false;
}

u16 GetFuncTypeNumParams (const IM3FuncType i_funcType)
{
    ပြန်ပေး i_funcType ? i_funcType->numArgs : 0;
}


m3type_t GetFuncTypeParamType (const IM3FuncType i_funcType, u16 i_index)
{
    m3type_t type = c_m3Type_unknown;

    အကယ်၍ (i_funcType) {
        အကယ်၍ (i_index < i_funcType->numArgs) {
            type = i_funcType->types[i_funcType->numRets + i_index];
        }
    }

    ပြန်ပေး type;
}


u16 GetFuncTypeNumResults (const IM3FuncType i_funcType)
{
    ပြန်ပေး i_funcType ? i_funcType->numRets : 0;
}


m3type_t GetFuncTypeResultType (const IM3FuncType i_funcType, u16 i_index)
{
    m3type_t type = c_m3Type_unknown;

    အကယ်၍ (i_funcType) {
        အကယ်၍ (i_index < i_funcType->numRets) {
            type = i_funcType->types[i_index];
        }
    }

    ပြန်ပေး type;
}


//---------------------------------------------------------------------------------------------------------------


void FreeImportInfo (M3ImportInfo* i_info)
{
    m3_Free(i_info->moduleUtf8);
    m3_Free(i_info->fieldUtf8);
}


#အကယ်၍ d_m3HasSnapshots

void SnapshotMap_Free (M3SnapshotMap* i_map)
{
    အကယ်၍ (i_map) {
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

#အကယ်၍ d_m3HasSnapshots
    SnapshotMap_Free(i_function->snapshotMap);
    i_function->snapshotMap = NULL;
#endif

    အတွက် (ကိန်းပြည့် i = 0; i < i_function->numNames; i++) {
        // name can be an alias of fieldUtf8
        အကယ်၍ (i_function->names[i] != i_function->import.fieldUtf8) {
            m3_Free(i_function->names[i]);
        }
    }

    FreeImportInfo(&i_function->import);

    အကယ်၍ (i_function->ownsWasmCode) {
        m3_Free(i_function->wasm);
    }

    // Function_FreeCompiledCode (func);

#အကယ်၍ (d_m3EnableCodePageRefCounting)
    {
        m3_Free(i_function->codePageRefs);
        i_function->numCodePageRefs = 0;
    }
#endif
}


void Function_FreeCompiledCode (IM3Function i_function)
{
#အကယ်၍ (d_m3EnableCodePageRefCounting)
    {
        i_function->compiled = NULL;

        နေစဉ် (i_function->numCodePageRefs--) {
            IM3CodePage page = i_function->codePageRefs[i_function->numCodePageRefs];

            အကယ်၍ (--(page->info.usageCount) == 0) {
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
    အကယ်၍ (numNames > 0) {
        ပြန်ပေး names[0];
    } မဟုတ်ပါက {
        ပြန်ပေး "<unnamed>";
    }
}


IM3Module m3_GetFunctionModule (IM3Function i_function)
{
    ပြန်ပေး i_function ? i_function->module : NULL;
}


cstr_t* GetFunctionNames (IM3Function i_function, u16* o_numNames)
{
    အကယ်၍ (!i_function || !o_numNames) {
        ပြန်ပေး NULL;
    }

    အကယ်၍ (i_function->import.fieldUtf8) {
        *o_numNames = 1;
        ပြန်ပေး &i_function->import.fieldUtf8;
    } မဟုတ်ပါက {
        *o_numNames = i_function->numNames;
        ပြန်ပေး i_function->names;
    }
}


cstr_t GetFunctionImportModuleName (IM3Function i_function)
{
    ပြန်ပေး (i_function->import.moduleUtf8) ? i_function->import.moduleUtf8 : "";
}


u16 GetFunctionNumArgs (IM3Function i_function)
{
    u16 numArgs = 0;

    အကယ်၍ (i_function) {
        အကယ်၍ (i_function->funcType) {
            numArgs = i_function->funcType->numArgs;
        }
    }

    ပြန်ပေး numArgs;
}

m3type_t GetFunctionArgType (IM3Function i_function, u32 i_index)
{
    m3type_t type = c_m3Type_none;

    အကယ်၍ (i_index < GetFunctionNumArgs(i_function)) {
        u32 numReturns = i_function->funcType->numRets;

        type = i_function->funcType->types[numReturns + i_index];
    }

    ပြန်ပေး type;
}


u16 GetFunctionNumReturns (IM3Function i_function)
{
    u16 numReturns = 0;

    အကယ်၍ (i_function) {
        အကယ်၍ (i_function->funcType) {
            numReturns = i_function->funcType->numRets;
        }
    }

    ပြန်ပေး numReturns;
}


u8 GetFunctionReturnType (const IM3Function i_function, u16 i_index)
{
    ပြန်ပေး i_function ? GetFuncTypeResultType(i_function->funcType, i_index) : c_m3Type_unknown;
}


u32 GetFunctionNumArgsAndLocals (IM3Function i_function)
{
    အကယ်၍ (i_function) {
        ပြန်ပေး i_function->numLocals + GetFunctionNumArgs(i_function);
    } မဟုတ်ပါက {
        ပြန်ပေး 0;
    }
}
