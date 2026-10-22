//
//  m3_function.c
//
//  Created by Steven Massey on 4/7/21.
//  Copyright © 2021 Steven Massey. All rights reserved.
//

#ضمن "m3_function.h"
#ضمن "m3_env.h"


M3_نتيجو AllocFuncType (IM3FuncType* o_functionType, u32 i_انگ_Types)
{
    *o_functionType = (IM3FuncType)m3_Malloc("M3FuncType", حجم(M3FuncType) + i_انگ_Types * حجم(m3type_t));
    إذا (*o_functionType) {
        (*o_functionType)->isContinuation = false;
        (*o_functionType)->contFuncType   = عدم;
        (*o_functionType)->اندريون_Rec_گروهه     = false;
        أرجع m3Err_none;
    }
    أرجع m3Err_mallocFailed;
}


#إذا d_m3HasTypedRefs

// The type a (ref $t) / (ref null $t) naming this function type is spelled as
m3type_t Ref_Type_جو_Func_Type (ثابت IM3FuncType i_funcType, bool i_nonNull)
{
    m3type_t type = d_m3Type_ref | (i_funcType ? i_funcType->canonical_اشاريو : d_m3_Type_ڍير_Abstract);

    إذا (i_funcType and i_funcType->isContinuation) {
        type |= d_m3Type_refCont;
    }

    إذا (i_nonNull) {
        type |= d_m3Type_refNonNull;
    }

    أرجع type;
}

#جيڪڏهن_پورو // d_m3HasTypedRefs


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
    إذا (i_typeA->انگ_Rets == i_typeB->انگ_Rets and i_typeA->انگ_دليلون == i_typeB->انگ_دليلون) {
        أرجع (memcmp(i_typeA->types, i_typeB->types,
                       (i_typeA->انگ_Rets + i_typeA->انگ_دليلون) * حجم(m3type_t)) == 0);
    }

    أرجع false;
}

u16 Get_Func_Type_انگ_Params (ثابت IM3FuncType i_funcType)
{
    أرجع i_funcType ? i_funcType->انگ_دليلون : 0;
}


m3type_t GetFuncTypeParamType (ثابت IM3FuncType i_funcType, u16 i_اشاريو)
{
    m3type_t type = c_m3Type_unknown;

    إذا (i_funcType) {
        إذا (i_اشاريو < i_funcType->انگ_دليلون) {
            type = i_funcType->types[i_funcType->انگ_Rets + i_اشاريو];
        }
    }

    أرجع type;
}


u16 Get_Func_Type_انگ_Results (ثابت IM3FuncType i_funcType)
{
    أرجع i_funcType ? i_funcType->انگ_Rets : 0;
}


m3type_t Get_Func_Type_نتيجو_Type (ثابت IM3FuncType i_funcType, u16 i_اشاريو)
{
    m3type_t type = c_m3Type_unknown;

    إذا (i_funcType) {
        إذا (i_اشاريو < i_funcType->انگ_Rets) {
            type = i_funcType->types[i_اشاريو];
        }
    }

    أرجع type;
}


//---------------------------------------------------------------------------------------------------------------


فراغ FreeImportInfo (M3ImportInfo* i_info)
{
    m3_Free(i_info->moduleUtf8);
    m3_Free(i_info->ميدان_Utf8);
}


#إذا d_m3HasSnapshots

فراغ SnapshotMap_Free (M3SnapshotMap* i_map)
{
    إذا (i_map) {
        m3_Free(i_map->locals);
        m3_Free(i_map->محفوظ_Points);
        m3_Free(i_map->قدرون);
        m3_Free(i_map->بلاڪون);
        m3_Free(i_map);
    }
}

#جيڪڏهن_پورو

فراغ Function_Release (IM3Function i_function)
{
    m3_Free(i_function->constants);

#إذا d_m3HasSnapshots
    SnapshotMap_Free(i_function->snapshotMap);
    i_function->snapshotMap = عدم;
#جيڪڏهن_پورو

    لكل (صحيح i = 0; i < i_function->انگ_نالا; i++) {
        // name can be an alias of fieldUtf8
        إذا (i_function->نالا[i] != i_function->import.ميدان_Utf8) {
            m3_Free(i_function->نالا[i]);
        }
    }

    FreeImportInfo(&i_function->import);

    إذا (i_function->owns_Wasm_ڪوڊ) {
        m3_Free(i_function->wasm);
    }

    // Function_FreeCompiledCode (func);

#إذا (d_m3_Enable_ڪوڊ_صفحو_Ref_Counting)
    {
        m3_Free(i_function->ڪوڊ_صفحو_Refs);
        i_function->انگ_ڪوڊ_صفحو_Refs = 0;
    }
#جيڪڏهن_پورو
}


فراغ Function_Free_Compiled_ڪوڊ (IM3Function i_function)
{
#إذا (d_m3_Enable_ڪوڊ_صفحو_Ref_Counting)
    {
        i_function->compiled = عدم;

        طالما (i_function->انگ_ڪوڊ_صفحو_Refs--) {
            IM3_ڪوڊ_صفحو صفحو = i_function->ڪوڊ_صفحو_Refs[i_function->انگ_ڪوڊ_صفحو_Refs];

            إذا (--(صفحو->info.usage_ڳڻپ) == 0) {
                //                printf ("free %p\n", page);
            }
        }

        m3_Free(i_function->ڪوڊ_صفحو_Refs);

        Runtime_Release_ڪوڊ_صفحا(i_function->module->runtime);
    }
#جيڪڏهن_پورو
}


cstr_t m3_GetFunctionName (IM3Function i_function)
{
    u16     انگ_نالا = 0;
    cstr_t* نالا    = Get_Function_نالا(i_function, &انگ_نالا);
    إذا (انگ_نالا > 0) {
        أرجع نالا[0];
    } وإلا {
        أرجع "<unnamed>";
    }
}


IM3Module m3_GetFunctionModule (IM3Function i_function)
{
    أرجع i_function ? i_function->module : عدم;
}


cstr_t* Get_Function_نالا (IM3Function i_function, u16* o_انگ_نالا)
{
    إذا (!i_function || !o_انگ_نالا) {
        أرجع عدم;
    }

    إذا (i_function->import.ميدان_Utf8) {
        *o_انگ_نالا = 1;
        أرجع &i_function->import.ميدان_Utf8;
    } وإلا {
        *o_انگ_نالا = i_function->انگ_نالا;
        أرجع i_function->نالا;
    }
}


cstr_t GetFunctionImportModuleName (IM3Function i_function)
{
    أرجع (i_function->import.moduleUtf8) ? i_function->import.moduleUtf8 : "";
}


u16 Get_Function_انگ_دليلون (IM3Function i_function)
{
    u16 انگ_دليلون = 0;

    إذا (i_function) {
        إذا (i_function->funcType) {
            انگ_دليلون = i_function->funcType->انگ_دليلون;
        }
    }

    أرجع انگ_دليلون;
}

m3type_t GetFunctionArgType (IM3Function i_function, u32 i_اشاريو)
{
    m3type_t type = c_m3Type_none;

    إذا (i_اشاريو < Get_Function_انگ_دليلون(i_function)) {
        u32 انگ_Returns = i_function->funcType->انگ_Rets;

        type = i_function->funcType->types[انگ_Returns + i_اشاريو];
    }

    أرجع type;
}


u16 Get_Function_انگ_Returns (IM3Function i_function)
{
    u16 انگ_Returns = 0;

    إذا (i_function) {
        إذا (i_function->funcType) {
            انگ_Returns = i_function->funcType->انگ_Rets;
        }
    }

    أرجع انگ_Returns;
}


u8 GetFunctionReturnType (ثابت IM3Function i_function, u16 i_اشاريو)
{
    أرجع i_function ? Get_Func_Type_نتيجو_Type(i_function->funcType, i_اشاريو) : c_m3Type_unknown;
}


u32 Get_Function_انگ_دليلون_And_Locals (IM3Function i_function)
{
    إذا (i_function) {
        أرجع i_function->انگ_Locals + Get_Function_انگ_دليلون(i_function);
    } وإلا {
        أرجع 0;
    }
}
