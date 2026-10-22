//
//  m3_function.c
//
//  Created by Steven Massey on 4/7/21.
//  Copyright © 2021 Steven Massey. All rights reserved.
//

#ضمن "m3_function.h"
#ضمن "m3_env.h"


M3_نتیجه AllocFuncType (IM3FuncType* o_functionType, u32 i_رقم_Types)
{
    *o_functionType = (IM3FuncType)m3_Malloc("M3FuncType", حجم(M3FuncType) + i_رقم_Types * حجم(m3type_t));
    إذا (*o_functionType) {
        (*o_functionType)->isContinuation = false;
        (*o_functionType)->contFuncType   = عدم;
        (*o_functionType)->درون_Rec_گروه     = false;
        أرجع m3Err_none;
    }
    أرجع m3Err_mallocFailed;
}


#إذا d_m3HasTypedRefs

// The type a (ref $t) / (ref null $t) naming this function type is spelled as
m3type_t Ref_Type_از_Func_Type (ثابت IM3FuncType i_funcType, bool i_nonNull)
{
    m3type_t type = d_m3Type_ref | (i_funcType ? i_funcType->canonical_شاخص : d_m3_Type_انباشته_Abstract);

    إذا (i_funcType and i_funcType->isContinuation) {
        type |= d_m3Type_refCont;
    }

    إذا (i_nonNull) {
        type |= d_m3Type_refNonNull;
    }

    أرجع type;
}

#پایان_اگر // d_m3HasTypedRefs


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
    إذا (i_typeA->رقم_Rets == i_typeB->رقم_Rets and i_typeA->رقم_آرگومانها == i_typeB->رقم_آرگومانها) {
        أرجع (memcmp(i_typeA->types, i_typeB->types,
                       (i_typeA->رقم_Rets + i_typeA->رقم_آرگومانها) * حجم(m3type_t)) == 0);
    }

    أرجع false;
}

u16 Get_Func_Type_رقم_Params (ثابت IM3FuncType i_funcType)
{
    أرجع i_funcType ? i_funcType->رقم_آرگومانها : 0;
}


m3type_t GetFuncTypeParamType (ثابت IM3FuncType i_funcType, u16 i_شاخص)
{
    m3type_t type = c_m3Type_unknown;

    إذا (i_funcType) {
        إذا (i_شاخص < i_funcType->رقم_آرگومانها) {
            type = i_funcType->types[i_funcType->رقم_Rets + i_شاخص];
        }
    }

    أرجع type;
}


u16 Get_Func_Type_رقم_Results (ثابت IM3FuncType i_funcType)
{
    أرجع i_funcType ? i_funcType->رقم_Rets : 0;
}


m3type_t Get_Func_Type_نتیجه_Type (ثابت IM3FuncType i_funcType, u16 i_شاخص)
{
    m3type_t type = c_m3Type_unknown;

    إذا (i_funcType) {
        إذا (i_شاخص < i_funcType->رقم_Rets) {
            type = i_funcType->types[i_شاخص];
        }
    }

    أرجع type;
}


//---------------------------------------------------------------------------------------------------------------


فراغ FreeImportInfo (M3ImportInfo* i_info)
{
    m3_Free(i_info->moduleUtf8);
    m3_Free(i_info->فیلد_Utf8);
}


#إذا d_m3HasSnapshots

فراغ SnapshotMap_Free (M3SnapshotMap* i_map)
{
    إذا (i_map) {
        m3_Free(i_map->locals);
        m3_Free(i_map->امن_Points);
        m3_Free(i_map->مقادیر);
        m3_Free(i_map->بلوکها);
        m3_Free(i_map);
    }
}

#پایان_اگر

فراغ Function_Release (IM3Function i_function)
{
    m3_Free(i_function->constants);

#إذا d_m3HasSnapshots
    SnapshotMap_Free(i_function->snapshotMap);
    i_function->snapshotMap = عدم;
#پایان_اگر

    لكل (صحيح i = 0; i < i_function->رقم_نامها; i++) {
        // name can be an alias of fieldUtf8
        إذا (i_function->نامها[i] != i_function->import.فیلد_Utf8) {
            m3_Free(i_function->نامها[i]);
        }
    }

    FreeImportInfo(&i_function->import);

    إذا (i_function->owns_Wasm_کد) {
        m3_Free(i_function->wasm);
    }

    // Function_FreeCompiledCode (func);

#إذا (d_m3_Enable_کد_صفحه_Ref_Counting)
    {
        m3_Free(i_function->کد_صفحه_Refs);
        i_function->رقم_کد_صفحه_Refs = 0;
    }
#پایان_اگر
}


فراغ Function_Free_Compiled_کد (IM3Function i_function)
{
#إذا (d_m3_Enable_کد_صفحه_Ref_Counting)
    {
        i_function->compiled = عدم;

        طالما (i_function->رقم_کد_صفحه_Refs--) {
            IM3_کد_صفحه صفحه = i_function->کد_صفحه_Refs[i_function->رقم_کد_صفحه_Refs];

            إذا (--(صفحه->info.usage_شمار) == 0) {
                //                printf ("free %p\n", page);
            }
        }

        m3_Free(i_function->کد_صفحه_Refs);

        Runtime_Release_کد_صفحات(i_function->module->runtime);
    }
#پایان_اگر
}


cstr_t m3_GetFunctionName (IM3Function i_function)
{
    u16     رقم_نامها = 0;
    cstr_t* نامها    = Get_Function_نامها(i_function, &رقم_نامها);
    إذا (رقم_نامها > 0) {
        أرجع نامها[0];
    } وإلا {
        أرجع "<unnamed>";
    }
}


IM3Module m3_GetFunctionModule (IM3Function i_function)
{
    أرجع i_function ? i_function->module : عدم;
}


cstr_t* Get_Function_نامها (IM3Function i_function, u16* o_رقم_نامها)
{
    إذا (!i_function || !o_رقم_نامها) {
        أرجع عدم;
    }

    إذا (i_function->import.فیلد_Utf8) {
        *o_رقم_نامها = 1;
        أرجع &i_function->import.فیلد_Utf8;
    } وإلا {
        *o_رقم_نامها = i_function->رقم_نامها;
        أرجع i_function->نامها;
    }
}


cstr_t GetFunctionImportModuleName (IM3Function i_function)
{
    أرجع (i_function->import.moduleUtf8) ? i_function->import.moduleUtf8 : "";
}


u16 Get_Function_رقم_آرگومانها (IM3Function i_function)
{
    u16 رقم_آرگومانها = 0;

    إذا (i_function) {
        إذا (i_function->funcType) {
            رقم_آرگومانها = i_function->funcType->رقم_آرگومانها;
        }
    }

    أرجع رقم_آرگومانها;
}

m3type_t GetFunctionArgType (IM3Function i_function, u32 i_شاخص)
{
    m3type_t type = c_m3Type_none;

    إذا (i_شاخص < Get_Function_رقم_آرگومانها(i_function)) {
        u32 رقم_Returns = i_function->funcType->رقم_Rets;

        type = i_function->funcType->types[رقم_Returns + i_شاخص];
    }

    أرجع type;
}


u16 Get_Function_رقم_Returns (IM3Function i_function)
{
    u16 رقم_Returns = 0;

    إذا (i_function) {
        إذا (i_function->funcType) {
            رقم_Returns = i_function->funcType->رقم_Rets;
        }
    }

    أرجع رقم_Returns;
}


u8 GetFunctionReturnType (ثابت IM3Function i_function, u16 i_شاخص)
{
    أرجع i_function ? Get_Func_Type_نتیجه_Type(i_function->funcType, i_شاخص) : c_m3Type_unknown;
}


u32 Get_Function_رقم_آرگومانها_And_Locals (IM3Function i_function)
{
    إذا (i_function) {
        أرجع i_function->رقم_Locals + Get_Function_رقم_آرگومانها(i_function);
    } وإلا {
        أرجع 0;
    }
}
