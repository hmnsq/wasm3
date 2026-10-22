//
//  m3_function.c
//
//  Created by Steven Massey on 4/7/21.
//  Copyright © 2021 Steven Massey. All rights reserved.
//

#अन्तर्भाव "m3_function.h"
#अन्तर्भाव "m3_env.h"


M3_परिणाम AllocFuncType (IM3FuncType* o_functionType, u32 i_अङ्कः_Types)
{
    *o_functionType = (IM3FuncType)m3_Malloc("M3FuncType", परिमाण(M3FuncType) + i_अङ्कः_Types * परिमाण(m3type_t));
    यदि (*o_functionType) {
        (*o_functionType)->isContinuation = false;
        (*o_functionType)->contFuncType   = रिक्त;
        (*o_functionType)->अन्तर्_Rec_गणः     = false;
        प्रत्यागम m3Err_none;
    }
    प्रत्यागम m3Err_mallocFailed;
}


#यदि d_m3HasTypedRefs

// The type a (ref $t) / (ref null $t) naming this function type is spelled as
m3type_t Ref_Type_सम्बन्धि_Func_Type (ध्रुव IM3FuncType i_funcType, bool i_nonNull)
{
    m3type_t type = d_m3Type_ref | (i_funcType ? i_funcType->canonical_सूचकांक : d_m3Type_heapAbstract);

    यदि (i_funcType and i_funcType->isContinuation) {
        type |= d_m3Type_refCont;
    }

    यदि (i_nonNull) {
        type |= d_m3Type_refNonNull;
    }

    प्रत्यागम type;
}

#यदि_अन्तः // d_m3HasTypedRefs


bool AreFuncTypesEqual (ध्रुव IM3FuncType i_typeA, ध्रुव IM3FuncType i_typeB)
{
    यदि (i_typeA->isContinuation != i_typeB->isContinuation) {
        प्रत्यागम false;
    }
    यदि (i_typeA->isContinuation) {
        प्रत्यागम (i_typeA->contFuncType == i_typeB->contFuncType) or
               (i_typeA->contFuncType and i_typeB->contFuncType and
                AreFuncTypesEqual(i_typeA->contFuncType, i_typeB->contFuncType));
    }
    यदि (i_typeA->अङ्कः_Rets == i_typeB->अङ्कः_Rets and i_typeA->अङ्कः_तर्काः == i_typeB->अङ्कः_तर्काः) {
        प्रत्यागम (memcmp(i_typeA->types, i_typeB->types,
                       (i_typeA->अङ्कः_Rets + i_typeA->अङ्कः_तर्काः) * परिमाण(m3type_t)) == 0);
    }

    प्रत्यागम false;
}

u16 Get_Func_Type_अङ्कः_Params (ध्रुव IM3FuncType i_funcType)
{
    प्रत्यागम i_funcType ? i_funcType->अङ्कः_तर्काः : 0;
}


m3type_t GetFuncTypeParamType (ध्रुव IM3FuncType i_funcType, u16 i_सूचकांक)
{
    m3type_t type = c_m3Type_unknown;

    यदि (i_funcType) {
        यदि (i_सूचकांक < i_funcType->अङ्कः_तर्काः) {
            type = i_funcType->types[i_funcType->अङ्कः_Rets + i_सूचकांक];
        }
    }

    प्रत्यागम type;
}


u16 Get_Func_Type_अङ्कः_Results (ध्रुव IM3FuncType i_funcType)
{
    प्रत्यागम i_funcType ? i_funcType->अङ्कः_Rets : 0;
}


m3type_t Get_Func_Type_परिणाम_Type (ध्रुव IM3FuncType i_funcType, u16 i_सूचकांक)
{
    m3type_t type = c_m3Type_unknown;

    यदि (i_funcType) {
        यदि (i_सूचकांक < i_funcType->अङ्कः_Rets) {
            type = i_funcType->types[i_सूचकांक];
        }
    }

    प्रत्यागम type;
}


//---------------------------------------------------------------------------------------------------------------


शून्य FreeImportInfo (M3ImportInfo* i_info)
{
    m3_Free(i_info->moduleUtf8);
    m3_Free(i_info->क्षेत्रम्_Utf8);
}


#यदि d_m3HasSnapshots

शून्य SnapshotMap_Free (M3SnapshotMap* i_map)
{
    यदि (i_map) {
        m3_Free(i_map->locals);
        m3_Free(i_map->safePoints);
        m3_Free(i_map->मानानि);
        m3_Free(i_map->खण्डानि);
        m3_Free(i_map);
    }
}

#यदि_अन्तः

शून्य Function_Release (IM3Function i_function)
{
    m3_Free(i_function->constants);

#यदि d_m3HasSnapshots
    SnapshotMap_Free(i_function->snapshotMap);
    i_function->snapshotMap = रिक्त;
#यदि_अन्तः

    प्रति (पूर्णाङ्क i = 0; i < i_function->अङ्कः_नामानि; i++) {
        // name can be an alias of fieldUtf8
        यदि (i_function->नामानि[i] != i_function->import.क्षेत्रम्_Utf8) {
            m3_Free(i_function->नामानि[i]);
        }
    }

    FreeImportInfo(&i_function->import);

    यदि (i_function->owns_Wasm_संहिता) {
        m3_Free(i_function->wasm);
    }

    // Function_FreeCompiledCode (func);

#यदि (d_m3_Enable_संहिता_Page_Ref_Counting)
    {
        m3_Free(i_function->संहिता_Page_Refs);
        i_function->अङ्कः_संहिता_Page_Refs = 0;
    }
#यदि_अन्तः
}


शून्य Function_Free_Compiled_संहिता (IM3Function i_function)
{
#यदि (d_m3_Enable_संहिता_Page_Ref_Counting)
    {
        i_function->compiled = रिक्त;

        यावत् (i_function->अङ्कः_संहिता_Page_Refs--) {
            IM3_संहिता_Page page = i_function->संहिता_Page_Refs[i_function->अङ्कः_संहिता_Page_Refs];

            यदि (--(page->info.usage_गणनम्) == 0) {
                //                printf ("free %p\n", page);
            }
        }

        m3_Free(i_function->संहिता_Page_Refs);

        Runtime_Release_संहिता_पृष्ठानि(i_function->module->runtime);
    }
#यदि_अन्तः
}


cstr_t m3_GetFunctionName (IM3Function i_function)
{
    u16     अङ्कः_नामानि = 0;
    cstr_t* नामानि    = Get_Function_नामानि(i_function, &अङ्कः_नामानि);
    यदि (अङ्कः_नामानि > 0) {
        प्रत्यागम नामानि[0];
    } अन्यथा {
        प्रत्यागम "<unnamed>";
    }
}


IM3Module m3_GetFunctionModule (IM3Function i_function)
{
    प्रत्यागम i_function ? i_function->module : रिक्त;
}


cstr_t* Get_Function_नामानि (IM3Function i_function, u16* o_अङ्कः_नामानि)
{
    यदि (!i_function || !o_अङ्कः_नामानि) {
        प्रत्यागम रिक्त;
    }

    यदि (i_function->import.क्षेत्रम्_Utf8) {
        *o_अङ्कः_नामानि = 1;
        प्रत्यागम &i_function->import.क्षेत्रम्_Utf8;
    } अन्यथा {
        *o_अङ्कः_नामानि = i_function->अङ्कः_नामानि;
        प्रत्यागम i_function->नामानि;
    }
}


cstr_t GetFunctionImportModuleName (IM3Function i_function)
{
    प्रत्यागम (i_function->import.moduleUtf8) ? i_function->import.moduleUtf8 : "";
}


u16 Get_Function_अङ्कः_तर्काः (IM3Function i_function)
{
    u16 अङ्कः_तर्काः = 0;

    यदि (i_function) {
        यदि (i_function->funcType) {
            अङ्कः_तर्काः = i_function->funcType->अङ्कः_तर्काः;
        }
    }

    प्रत्यागम अङ्कः_तर्काः;
}

m3type_t GetFunctionArgType (IM3Function i_function, u32 i_सूचकांक)
{
    m3type_t type = c_m3Type_none;

    यदि (i_सूचकांक < Get_Function_अङ्कः_तर्काः(i_function)) {
        u32 अङ्कः_Returns = i_function->funcType->अङ्कः_Rets;

        type = i_function->funcType->types[अङ्कः_Returns + i_सूचकांक];
    }

    प्रत्यागम type;
}


u16 Get_Function_अङ्कः_Returns (IM3Function i_function)
{
    u16 अङ्कः_Returns = 0;

    यदि (i_function) {
        यदि (i_function->funcType) {
            अङ्कः_Returns = i_function->funcType->अङ्कः_Rets;
        }
    }

    प्रत्यागम अङ्कः_Returns;
}


u8 GetFunctionReturnType (ध्रुव IM3Function i_function, u16 i_सूचकांक)
{
    प्रत्यागम i_function ? Get_Func_Type_परिणाम_Type(i_function->funcType, i_सूचकांक) : c_m3Type_unknown;
}


u32 Get_Function_अङ्कः_तर्काः_And_Locals (IM3Function i_function)
{
    यदि (i_function) {
        प्रत्यागम i_function->अङ्कः_Locals + Get_Function_अङ्कः_तर्काः(i_function);
    } अन्यथा {
        प्रत्यागम 0;
    }
}
