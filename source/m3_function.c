//
//  m3_function.c
//
//  Created by Steven Massey on 4/7/21.
//  Copyright © 2021 Steven Massey. All rights reserved.
//

#ضمن "m3_function.h"
#ضمن "m3_env.h"


M3Result AllocFuncType (IM3FuncType* o_functionType, u32 i_numTypes)
{
    *o_functionType = (IM3FuncType)m3_Malloc("M3FuncType", حجم(M3FuncType) + i_numTypes * حجم(m3type_t));
    ܐܢ (*o_functionType) {
        (*o_functionType)->isContinuation = false;
        (*o_functionType)->contFuncType   = عدم;
        (*o_functionType)->inRecGroup     = false;
        ܦܢܐ m3Err_none;
    }
    ܦܢܐ m3Err_mallocFailed;
}


#ܐܢ d_m3HasTypedRefs

// The type a (ref $t) / (ref null $t) naming this function type is spelled as
m3type_t RefTypeOfFuncType (ثابت IM3FuncType i_funcType, bool i_nonNull)
{
    m3type_t type = d_m3Type_ref | (i_funcType ? i_funcType->canonicalIndex : d_m3Type_heapAbstract);

    ܐܢ (i_funcType and i_funcType->isContinuation) {
        type |= d_m3Type_refCont;
    }

    ܐܢ (i_nonNull) {
        type |= d_m3Type_refNonNull;
    }

    ܦܢܐ type;
}

#endif // d_m3HasTypedRefs


bool AreFuncTypesEqual (ثابت IM3FuncType i_typeA, ثابت IM3FuncType i_typeB)
{
    ܐܢ (i_typeA->isContinuation != i_typeB->isContinuation) {
        ܦܢܐ false;
    }
    ܐܢ (i_typeA->isContinuation) {
        ܦܢܐ (i_typeA->contFuncType == i_typeB->contFuncType) or
               (i_typeA->contFuncType and i_typeB->contFuncType and
                AreFuncTypesEqual(i_typeA->contFuncType, i_typeB->contFuncType));
    }
    ܐܢ (i_typeA->numRets == i_typeB->numRets and i_typeA->numArgs == i_typeB->numArgs) {
        ܦܢܐ (memcmp(i_typeA->types, i_typeB->types,
                       (i_typeA->numRets + i_typeA->numArgs) * حجم(m3type_t)) == 0);
    }

    ܦܢܐ false;
}

u16 GetFuncTypeNumParams (ثابت IM3FuncType i_funcType)
{
    ܦܢܐ i_funcType ? i_funcType->numArgs : 0;
}


m3type_t GetFuncTypeParamType (ثابت IM3FuncType i_funcType, u16 i_index)
{
    m3type_t type = c_m3Type_unknown;

    ܐܢ (i_funcType) {
        ܐܢ (i_index < i_funcType->numArgs) {
            type = i_funcType->types[i_funcType->numRets + i_index];
        }
    }

    ܦܢܐ type;
}


u16 GetFuncTypeNumResults (ثابت IM3FuncType i_funcType)
{
    ܦܢܐ i_funcType ? i_funcType->numRets : 0;
}


m3type_t GetFuncTypeResultType (ثابت IM3FuncType i_funcType, u16 i_index)
{
    m3type_t type = c_m3Type_unknown;

    ܐܢ (i_funcType) {
        ܐܢ (i_index < i_funcType->numRets) {
            type = i_funcType->types[i_index];
        }
    }

    ܦܢܐ type;
}


//---------------------------------------------------------------------------------------------------------------


فراغ FreeImportInfo (M3ImportInfo* i_info)
{
    m3_Free(i_info->moduleUtf8);
    m3_Free(i_info->fieldUtf8);
}


#ܐܢ d_m3HasSnapshots

فراغ SnapshotMap_Free (M3SnapshotMap* i_map)
{
    ܐܢ (i_map) {
        m3_Free(i_map->locals);
        m3_Free(i_map->safePoints);
        m3_Free(i_map->values);
        m3_Free(i_map->blocks);
        m3_Free(i_map);
    }
}

#endif

فراغ Function_Release (IM3Function i_function)
{
    m3_Free(i_function->constants);

#ܐܢ d_m3HasSnapshots
    SnapshotMap_Free(i_function->snapshotMap);
    i_function->snapshotMap = عدم;
#endif

    لكل (صحيح i = 0; i < i_function->numNames; i++) {
        // name can be an alias of fieldUtf8
        ܐܢ (i_function->names[i] != i_function->import.fieldUtf8) {
            m3_Free(i_function->names[i]);
        }
    }

    FreeImportInfo(&i_function->import);

    ܐܢ (i_function->ownsWasmCode) {
        m3_Free(i_function->wasm);
    }

    // Function_FreeCompiledCode (func);

#ܐܢ (d_m3EnableCodePageRefCounting)
    {
        m3_Free(i_function->codePageRefs);
        i_function->numCodePageRefs = 0;
    }
#endif
}


فراغ Function_FreeCompiledCode (IM3Function i_function)
{
#ܐܢ (d_m3EnableCodePageRefCounting)
    {
        i_function->compiled = عدم;

        ܟܡܐ (i_function->numCodePageRefs--) {
            IM3CodePage page = i_function->codePageRefs[i_function->numCodePageRefs];

            ܐܢ (--(page->info.usageCount) == 0) {
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
    ܐܢ (numNames > 0) {
        ܦܢܐ names[0];
    } ܐܚܪܢܐ {
        ܦܢܐ "<unnamed>";
    }
}


IM3Module m3_GetFunctionModule (IM3Function i_function)
{
    ܦܢܐ i_function ? i_function->module : عدم;
}


cstr_t* GetFunctionNames (IM3Function i_function, u16* o_numNames)
{
    ܐܢ (!i_function || !o_numNames) {
        ܦܢܐ عدم;
    }

    ܐܢ (i_function->import.fieldUtf8) {
        *o_numNames = 1;
        ܦܢܐ &i_function->import.fieldUtf8;
    } ܐܚܪܢܐ {
        *o_numNames = i_function->numNames;
        ܦܢܐ i_function->names;
    }
}


cstr_t GetFunctionImportModuleName (IM3Function i_function)
{
    ܦܢܐ (i_function->import.moduleUtf8) ? i_function->import.moduleUtf8 : "";
}


u16 GetFunctionNumArgs (IM3Function i_function)
{
    u16 numArgs = 0;

    ܐܢ (i_function) {
        ܐܢ (i_function->funcType) {
            numArgs = i_function->funcType->numArgs;
        }
    }

    ܦܢܐ numArgs;
}

m3type_t GetFunctionArgType (IM3Function i_function, u32 i_index)
{
    m3type_t type = c_m3Type_none;

    ܐܢ (i_index < GetFunctionNumArgs(i_function)) {
        u32 numReturns = i_function->funcType->numRets;

        type = i_function->funcType->types[numReturns + i_index];
    }

    ܦܢܐ type;
}


u16 GetFunctionNumReturns (IM3Function i_function)
{
    u16 numReturns = 0;

    ܐܢ (i_function) {
        ܐܢ (i_function->funcType) {
            numReturns = i_function->funcType->numRets;
        }
    }

    ܦܢܐ numReturns;
}


u8 GetFunctionReturnType (ثابت IM3Function i_function, u16 i_index)
{
    ܦܢܐ i_function ? GetFuncTypeResultType(i_function->funcType, i_index) : c_m3Type_unknown;
}


u32 GetFunctionNumArgsAndLocals (IM3Function i_function)
{
    ܐܢ (i_function) {
        ܦܢܐ i_function->numLocals + GetFunctionNumArgs(i_function);
    } ܐܚܪܢܐ {
        ܦܢܐ 0;
    }
}
