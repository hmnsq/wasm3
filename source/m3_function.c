//
//  m3_function.c
//
//  Created by Steven Massey on 4/7/21.
//  Copyright © 2021 Steven Massey. All rights reserved.
//

#అన్తర్భావ "m3_function.h"
#అన్తర్భావ "m3_env.h"


M3_ఫలితం AllocFuncType (IM3FuncType* o_functionType, u32 i_అంకె_Types)
{
    *o_functionType = (IM3FuncType)m3_Malloc("M3FuncType", పరిమాణం(M3FuncType) + i_అంకె_Types * పరిమాణం(m3type_t));
    ఒకవేళ (*o_functionType) {
        (*o_functionType)->isContinuation = false;
        (*o_functionType)->contFuncType   = ఖాళీ;
        (*o_functionType)->లోపల_Rec_సమూహం     = false;
        తిరిగి m3Err_none;
    }
    తిరిగి m3Err_mallocFailed;
}


#ఒకవేళ d_m3HasTypedRefs

// The type a (ref $t) / (ref null $t) naming this function type is spelled as
m3type_t Ref_Type_యొక్క_Func_Type (స్థిరరాశి IM3FuncType i_funcType, bool i_nonNull)
{
    m3type_t type = d_m3Type_ref | (i_funcType ? i_funcType->canonical_సూచీ : d_m3_Type_కుప్ప_Abstract);

    ఒకవేళ (i_funcType and i_funcType->isContinuation) {
        type |= d_m3Type_refCont;
    }

    ఒకవేళ (i_nonNull) {
        type |= d_m3Type_refNonNull;
    }

    తిరిగి type;
}

#అయితే_ముగింపు // d_m3HasTypedRefs


bool AreFuncTypesEqual (స్థిరరాశి IM3FuncType i_typeA, స్థిరరాశి IM3FuncType i_typeB)
{
    ఒకవేళ (i_typeA->isContinuation != i_typeB->isContinuation) {
        తిరిగి false;
    }
    ఒకవేళ (i_typeA->isContinuation) {
        తిరిగి (i_typeA->contFuncType == i_typeB->contFuncType) or
               (i_typeA->contFuncType and i_typeB->contFuncType and
                AreFuncTypesEqual(i_typeA->contFuncType, i_typeB->contFuncType));
    }
    ఒకవేళ (i_typeA->అంకె_Rets == i_typeB->అంకె_Rets and i_typeA->అంకె_వాదనలు == i_typeB->అంకె_వాదనలు) {
        తిరిగి (memcmp(i_typeA->types, i_typeB->types,
                       (i_typeA->అంకె_Rets + i_typeA->అంకె_వాదనలు) * పరిమాణం(m3type_t)) == 0);
    }

    తిరిగి false;
}

u16 Get_Func_Type_అంకె_Params (స్థిరరాశి IM3FuncType i_funcType)
{
    తిరిగి i_funcType ? i_funcType->అంకె_వాదనలు : 0;
}


m3type_t GetFuncTypeParamType (స్థిరరాశి IM3FuncType i_funcType, u16 i_సూచీ)
{
    m3type_t type = c_m3Type_unknown;

    ఒకవేళ (i_funcType) {
        ఒకవేళ (i_సూచీ < i_funcType->అంకె_వాదనలు) {
            type = i_funcType->types[i_funcType->అంకె_Rets + i_సూచీ];
        }
    }

    తిరిగి type;
}


u16 Get_Func_Type_అంకె_Results (స్థిరరాశి IM3FuncType i_funcType)
{
    తిరిగి i_funcType ? i_funcType->అంకె_Rets : 0;
}


m3type_t Get_Func_Type_ఫలితం_Type (స్థిరరాశి IM3FuncType i_funcType, u16 i_సూచీ)
{
    m3type_t type = c_m3Type_unknown;

    ఒకవేళ (i_funcType) {
        ఒకవేళ (i_సూచీ < i_funcType->అంకె_Rets) {
            type = i_funcType->types[i_సూచీ];
        }
    }

    తిరిగి type;
}


//---------------------------------------------------------------------------------------------------------------


శూన్య FreeImportInfo (M3ImportInfo* i_info)
{
    m3_Free(i_info->moduleUtf8);
    m3_Free(i_info->క్షేత్రం_Utf8);
}


#ఒకవేళ d_m3HasSnapshots

శూన్య SnapshotMap_Free (M3SnapshotMap* i_map)
{
    ఒకవేళ (i_map) {
        m3_Free(i_map->locals);
        m3_Free(i_map->సురక్షిత_Points);
        m3_Free(i_map->విలువలు);
        m3_Free(i_map->ఖండాలు);
        m3_Free(i_map);
    }
}

#అయితే_ముగింపు

శూన్య Function_Release (IM3Function i_function)
{
    m3_Free(i_function->constants);

#ఒకవేళ d_m3HasSnapshots
    SnapshotMap_Free(i_function->snapshotMap);
    i_function->snapshotMap = ఖాళీ;
#అయితే_ముగింపు

    కొరకు (పూర్ణాంకం i = 0; i < i_function->అంకె_పేర్లు; i++) {
        // name can be an alias of fieldUtf8
        ఒకవేళ (i_function->పేర్లు[i] != i_function->import.క్షేత్రం_Utf8) {
            m3_Free(i_function->పేర్లు[i]);
        }
    }

    FreeImportInfo(&i_function->import);

    ఒకవేళ (i_function->owns_Wasm_సంకేతం) {
        m3_Free(i_function->wasm);
    }

    // Function_FreeCompiledCode (func);

#ఒకవేళ (d_m3_Enable_సంకేతం_పుట_Ref_Counting)
    {
        m3_Free(i_function->సంకేతం_పుట_Refs);
        i_function->అంకె_సంకేతం_పుట_Refs = 0;
    }
#అయితే_ముగింపు
}


శూన్య Function_Free_Compiled_సంకేతం (IM3Function i_function)
{
#ఒకవేళ (d_m3_Enable_సంకేతం_పుట_Ref_Counting)
    {
        i_function->compiled = ఖాళీ;

        అయితే (i_function->అంకె_సంకేతం_పుట_Refs--) {
            IM3_సంకేతం_పుట పుట = i_function->సంకేతం_పుట_Refs[i_function->అంకె_సంకేతం_పుట_Refs];

            ఒకవేళ (--(పుట->info.usage_లెక్క) == 0) {
                //                printf ("free %p\n", page);
            }
        }

        m3_Free(i_function->సంకేతం_పుట_Refs);

        Runtime_Release_సంకేతం_పుటలు(i_function->module->runtime);
    }
#అయితే_ముగింపు
}


cstr_t m3_GetFunctionName (IM3Function i_function)
{
    u16     అంకె_పేర్లు = 0;
    cstr_t* పేర్లు    = Get_Function_పేర్లు(i_function, &అంకె_పేర్లు);
    ఒకవేళ (అంకె_పేర్లు > 0) {
        తిరిగి పేర్లు[0];
    } లేకపోతే {
        తిరిగి "<unnamed>";
    }
}


IM3Module m3_GetFunctionModule (IM3Function i_function)
{
    తిరిగి i_function ? i_function->module : ఖాళీ;
}


cstr_t* Get_Function_పేర్లు (IM3Function i_function, u16* o_అంకె_పేర్లు)
{
    ఒకవేళ (!i_function || !o_అంకె_పేర్లు) {
        తిరిగి ఖాళీ;
    }

    ఒకవేళ (i_function->import.క్షేత్రం_Utf8) {
        *o_అంకె_పేర్లు = 1;
        తిరిగి &i_function->import.క్షేత్రం_Utf8;
    } లేకపోతే {
        *o_అంకె_పేర్లు = i_function->అంకె_పేర్లు;
        తిరిగి i_function->పేర్లు;
    }
}


cstr_t GetFunctionImportModuleName (IM3Function i_function)
{
    తిరిగి (i_function->import.moduleUtf8) ? i_function->import.moduleUtf8 : "";
}


u16 Get_Function_అంకె_వాదనలు (IM3Function i_function)
{
    u16 అంకె_వాదనలు = 0;

    ఒకవేళ (i_function) {
        ఒకవేళ (i_function->funcType) {
            అంకె_వాదనలు = i_function->funcType->అంకె_వాదనలు;
        }
    }

    తిరిగి అంకె_వాదనలు;
}

m3type_t GetFunctionArgType (IM3Function i_function, u32 i_సూచీ)
{
    m3type_t type = c_m3Type_none;

    ఒకవేళ (i_సూచీ < Get_Function_అంకె_వాదనలు(i_function)) {
        u32 అంకె_Returns = i_function->funcType->అంకె_Rets;

        type = i_function->funcType->types[అంకె_Returns + i_సూచీ];
    }

    తిరిగి type;
}


u16 Get_Function_అంకె_Returns (IM3Function i_function)
{
    u16 అంకె_Returns = 0;

    ఒకవేళ (i_function) {
        ఒకవేళ (i_function->funcType) {
            అంకె_Returns = i_function->funcType->అంకె_Rets;
        }
    }

    తిరిగి అంకె_Returns;
}


u8 GetFunctionReturnType (స్థిరరాశి IM3Function i_function, u16 i_సూచీ)
{
    తిరిగి i_function ? Get_Func_Type_ఫలితం_Type(i_function->funcType, i_సూచీ) : c_m3Type_unknown;
}


u32 Get_Function_అంకె_వాదనలు_And_Locals (IM3Function i_function)
{
    ఒకవేళ (i_function) {
        తిరిగి i_function->అంకె_Locals + Get_Function_అంకె_వాదనలు(i_function);
    } లేకపోతే {
        తిరిగి 0;
    }
}
