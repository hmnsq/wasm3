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
    אם (*o_functionType) {
        (*o_functionType)->isContinuation = false;
        (*o_functionType)->contFuncType   = عدم;
        (*o_functionType)->inRecGroup     = false;
        החזר m3Err_none;
    }
    החזר m3Err_mallocFailed;
}


#אם d_m3HasTypedRefs

// The type a (ref $t) / (ref null $t) naming this function type is spelled as
m3type_t RefTypeOfFuncType (ثابت IM3FuncType i_funcType, bool i_nonNull)
{
    m3type_t type = d_m3Type_ref | (i_funcType ? i_funcType->canonicalIndex : d_m3Type_heapAbstract);

    אם (i_funcType and i_funcType->isContinuation) {
        type |= d_m3Type_refCont;
    }

    אם (i_nonNull) {
        type |= d_m3Type_refNonNull;
    }

    החזר type;
}

#endif // d_m3HasTypedRefs


bool AreFuncTypesEqual (ثابت IM3FuncType i_typeA, ثابت IM3FuncType i_typeB)
{
    אם (i_typeA->isContinuation != i_typeB->isContinuation) {
        החזר false;
    }
    אם (i_typeA->isContinuation) {
        החזר (i_typeA->contFuncType == i_typeB->contFuncType) or
               (i_typeA->contFuncType and i_typeB->contFuncType and
                AreFuncTypesEqual(i_typeA->contFuncType, i_typeB->contFuncType));
    }
    אם (i_typeA->numRets == i_typeB->numRets and i_typeA->numArgs == i_typeB->numArgs) {
        החזר (memcmp(i_typeA->types, i_typeB->types,
                       (i_typeA->numRets + i_typeA->numArgs) * حجم(m3type_t)) == 0);
    }

    החזר false;
}

u16 GetFuncTypeNumParams (ثابت IM3FuncType i_funcType)
{
    החזר i_funcType ? i_funcType->numArgs : 0;
}


m3type_t GetFuncTypeParamType (ثابت IM3FuncType i_funcType, u16 i_index)
{
    m3type_t type = c_m3Type_unknown;

    אם (i_funcType) {
        אם (i_index < i_funcType->numArgs) {
            type = i_funcType->types[i_funcType->numRets + i_index];
        }
    }

    החזר type;
}


u16 GetFuncTypeNumResults (ثابت IM3FuncType i_funcType)
{
    החזר i_funcType ? i_funcType->numRets : 0;
}


m3type_t GetFuncTypeResultType (ثابت IM3FuncType i_funcType, u16 i_index)
{
    m3type_t type = c_m3Type_unknown;

    אם (i_funcType) {
        אם (i_index < i_funcType->numRets) {
            type = i_funcType->types[i_index];
        }
    }

    החזר type;
}


//---------------------------------------------------------------------------------------------------------------


فراغ FreeImportInfo (M3ImportInfo* i_info)
{
    m3_Free(i_info->moduleUtf8);
    m3_Free(i_info->fieldUtf8);
}


#אם d_m3HasSnapshots

فراغ SnapshotMap_Free (M3SnapshotMap* i_map)
{
    אם (i_map) {
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

#אם d_m3HasSnapshots
    SnapshotMap_Free(i_function->snapshotMap);
    i_function->snapshotMap = عدم;
#endif

    עבור (صحيح i = 0; i < i_function->numNames; i++) {
        // name can be an alias of fieldUtf8
        אם (i_function->names[i] != i_function->import.fieldUtf8) {
            m3_Free(i_function->names[i]);
        }
    }

    FreeImportInfo(&i_function->import);

    אם (i_function->ownsWasmCode) {
        m3_Free(i_function->wasm);
    }

    // Function_FreeCompiledCode (func);

#אם (d_m3EnableCodePageRefCounting)
    {
        m3_Free(i_function->codePageRefs);
        i_function->numCodePageRefs = 0;
    }
#endif
}


فراغ Function_FreeCompiledCode (IM3Function i_function)
{
#אם (d_m3EnableCodePageRefCounting)
    {
        i_function->compiled = عدم;

        כלעוד (i_function->numCodePageRefs--) {
            IM3CodePage page = i_function->codePageRefs[i_function->numCodePageRefs];

            אם (--(page->info.usageCount) == 0) {
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
    אם (numNames > 0) {
        החזר names[0];
    } אחרת {
        החזר "<unnamed>";
    }
}


IM3Module m3_GetFunctionModule (IM3Function i_function)
{
    החזר i_function ? i_function->module : عدم;
}


cstr_t* GetFunctionNames (IM3Function i_function, u16* o_numNames)
{
    אם (!i_function || !o_numNames) {
        החזר عدم;
    }

    אם (i_function->import.fieldUtf8) {
        *o_numNames = 1;
        החזר &i_function->import.fieldUtf8;
    } אחרת {
        *o_numNames = i_function->numNames;
        החזר i_function->names;
    }
}


cstr_t GetFunctionImportModuleName (IM3Function i_function)
{
    החזר (i_function->import.moduleUtf8) ? i_function->import.moduleUtf8 : "";
}


u16 GetFunctionNumArgs (IM3Function i_function)
{
    u16 numArgs = 0;

    אם (i_function) {
        אם (i_function->funcType) {
            numArgs = i_function->funcType->numArgs;
        }
    }

    החזר numArgs;
}

m3type_t GetFunctionArgType (IM3Function i_function, u32 i_index)
{
    m3type_t type = c_m3Type_none;

    אם (i_index < GetFunctionNumArgs(i_function)) {
        u32 numReturns = i_function->funcType->numRets;

        type = i_function->funcType->types[numReturns + i_index];
    }

    החזר type;
}


u16 GetFunctionNumReturns (IM3Function i_function)
{
    u16 numReturns = 0;

    אם (i_function) {
        אם (i_function->funcType) {
            numReturns = i_function->funcType->numRets;
        }
    }

    החזר numReturns;
}


u8 GetFunctionReturnType (ثابت IM3Function i_function, u16 i_index)
{
    החזר i_function ? GetFuncTypeResultType(i_function->funcType, i_index) : c_m3Type_unknown;
}


u32 GetFunctionNumArgsAndLocals (IM3Function i_function)
{
    אם (i_function) {
        החזר i_function->numLocals + GetFunctionNumArgs(i_function);
    } אחרת {
        החזר 0;
    }
}
