//
//  m3_function.c
//
//  Created by Steven Massey on 4/7/21.
//  Copyright © 2021 Steven Massey. All rights reserved.
//

#ضمن "m3_function.h"
#ضمن "m3_env.h"


M3_پایله AllocFuncType (IM3FuncType* o_functionType, u32 i_عدد_Types)
{
    *o_functionType = (IM3FuncType)m3_Malloc("M3FuncType", حجم(M3FuncType) + i_عدد_Types * حجم(m3type_t));
    إذا (*o_functionType) {
        (*o_functionType)->isContinuation = false;
        (*o_functionType)->contFuncType   = عدم;
        (*o_functionType)->دننه_Rec_ډله     = false;
        أرجع m3Err_none;
    }
    أرجع m3Err_mallocFailed;
}


#إذا d_m3HasTypedRefs

// The type a (ref $t) / (ref null $t) naming this function type is spelled as
m3type_t Ref_Type_د_Func_Type (ثابت IM3FuncType i_funcType, bool i_nonNull)
{
    m3type_t type = d_m3Type_ref | (i_funcType ? i_funcType->canonical_شاخص : d_m3_Type_ډېرۍ_Abstract);

    إذا (i_funcType and i_funcType->isContinuation) {
        type |= d_m3Type_refCont;
    }

    إذا (i_nonNull) {
        type |= d_m3Type_refNonNull;
    }

    أرجع type;
}

#د_که_پای // d_m3HasTypedRefs


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
    إذا (i_typeA->عدد_Rets == i_typeB->عدد_Rets and i_typeA->عدد_دلیلونه == i_typeB->عدد_دلیلونه) {
        أرجع (memcmp(i_typeA->types, i_typeB->types,
                       (i_typeA->عدد_Rets + i_typeA->عدد_دلیلونه) * حجم(m3type_t)) == 0);
    }

    أرجع false;
}

u16 Get_Func_Type_عدد_Params (ثابت IM3FuncType i_funcType)
{
    أرجع i_funcType ? i_funcType->عدد_دلیلونه : 0;
}


m3type_t GetFuncTypeParamType (ثابت IM3FuncType i_funcType, u16 i_شاخص)
{
    m3type_t type = c_m3Type_unknown;

    إذا (i_funcType) {
        إذا (i_شاخص < i_funcType->عدد_دلیلونه) {
            type = i_funcType->types[i_funcType->عدد_Rets + i_شاخص];
        }
    }

    أرجع type;
}


u16 Get_Func_Type_عدد_Results (ثابت IM3FuncType i_funcType)
{
    أرجع i_funcType ? i_funcType->عدد_Rets : 0;
}


m3type_t Get_Func_Type_پایله_Type (ثابت IM3FuncType i_funcType, u16 i_شاخص)
{
    m3type_t type = c_m3Type_unknown;

    إذا (i_funcType) {
        إذا (i_شاخص < i_funcType->عدد_Rets) {
            type = i_funcType->types[i_شاخص];
        }
    }

    أرجع type;
}


//---------------------------------------------------------------------------------------------------------------


فراغ FreeImportInfo (M3ImportInfo* i_info)
{
    m3_Free(i_info->moduleUtf8);
    m3_Free(i_info->ډګر_Utf8);
}


#إذا d_m3HasSnapshots

فراغ SnapshotMap_Free (M3SnapshotMap* i_map)
{
    إذا (i_map) {
        m3_Free(i_map->locals);
        m3_Free(i_map->خوندي_Points);
        m3_Free(i_map->ارزښتونه);
        m3_Free(i_map->بلاکونه);
        m3_Free(i_map);
    }
}

#د_که_پای

فراغ Function_Release (IM3Function i_function)
{
    m3_Free(i_function->constants);

#إذا d_m3HasSnapshots
    SnapshotMap_Free(i_function->snapshotMap);
    i_function->snapshotMap = عدم;
#د_که_پای

    لكل (صحيح i = 0; i < i_function->عدد_نومونه; i++) {
        // name can be an alias of fieldUtf8
        إذا (i_function->نومونه[i] != i_function->import.ډګر_Utf8) {
            m3_Free(i_function->نومونه[i]);
        }
    }

    FreeImportInfo(&i_function->import);

    إذا (i_function->owns_Wasm_کوډ) {
        m3_Free(i_function->wasm);
    }

    // Function_FreeCompiledCode (func);

#إذا (d_m3_Enable_کوډ_پاڼه_Ref_Counting)
    {
        m3_Free(i_function->کوډ_پاڼه_Refs);
        i_function->عدد_کوډ_پاڼه_Refs = 0;
    }
#د_که_پای
}


فراغ Function_Free_Compiled_کوډ (IM3Function i_function)
{
#إذا (d_m3_Enable_کوډ_پاڼه_Ref_Counting)
    {
        i_function->compiled = عدم;

        طالما (i_function->عدد_کوډ_پاڼه_Refs--) {
            IM3_کوډ_پاڼه پاڼه = i_function->کوډ_پاڼه_Refs[i_function->عدد_کوډ_پاڼه_Refs];

            إذا (--(پاڼه->info.usage_شمېر) == 0) {
                //                printf ("free %p\n", page);
            }
        }

        m3_Free(i_function->کوډ_پاڼه_Refs);

        Runtime_Release_کوډ_پاڼې(i_function->module->runtime);
    }
#د_که_پای
}


cstr_t m3_GetFunctionName (IM3Function i_function)
{
    u16     عدد_نومونه = 0;
    cstr_t* نومونه    = Get_Function_نومونه(i_function, &عدد_نومونه);
    إذا (عدد_نومونه > 0) {
        أرجع نومونه[0];
    } وإلا {
        أرجع "<unnamed>";
    }
}


IM3Module m3_GetFunctionModule (IM3Function i_function)
{
    أرجع i_function ? i_function->module : عدم;
}


cstr_t* Get_Function_نومونه (IM3Function i_function, u16* o_عدد_نومونه)
{
    إذا (!i_function || !o_عدد_نومونه) {
        أرجع عدم;
    }

    إذا (i_function->import.ډګر_Utf8) {
        *o_عدد_نومونه = 1;
        أرجع &i_function->import.ډګر_Utf8;
    } وإلا {
        *o_عدد_نومونه = i_function->عدد_نومونه;
        أرجع i_function->نومونه;
    }
}


cstr_t GetFunctionImportModuleName (IM3Function i_function)
{
    أرجع (i_function->import.moduleUtf8) ? i_function->import.moduleUtf8 : "";
}


u16 Get_Function_عدد_دلیلونه (IM3Function i_function)
{
    u16 عدد_دلیلونه = 0;

    إذا (i_function) {
        إذا (i_function->funcType) {
            عدد_دلیلونه = i_function->funcType->عدد_دلیلونه;
        }
    }

    أرجع عدد_دلیلونه;
}

m3type_t GetFunctionArgType (IM3Function i_function, u32 i_شاخص)
{
    m3type_t type = c_m3Type_none;

    إذا (i_شاخص < Get_Function_عدد_دلیلونه(i_function)) {
        u32 عدد_Returns = i_function->funcType->عدد_Rets;

        type = i_function->funcType->types[عدد_Returns + i_شاخص];
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


u8 GetFunctionReturnType (ثابت IM3Function i_function, u16 i_شاخص)
{
    أرجع i_function ? Get_Func_Type_پایله_Type(i_function->funcType, i_شاخص) : c_m3Type_unknown;
}


u32 Get_Function_عدد_دلیلونه_And_Locals (IM3Function i_function)
{
    إذا (i_function) {
        أرجع i_function->عدد_Locals + Get_Function_عدد_دلیلونه(i_function);
    } وإلا {
        أرجع 0;
    }
}
