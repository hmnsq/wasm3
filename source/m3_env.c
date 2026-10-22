//
//  m3_env.c
//
//  Created by Steven Massey on 4/19/19.
//  Copyright © 2019 Steven Massey. All rights reserved.
//

#అన్తర్భావ <stdarg.h>
#అన్తర్భావ <limits.h>
#అన్తర్భావ <errno.h>
#అన్తర్భావ <float.h>
#అన్తర్భావ <ctype.h>

#అన్తర్భావ "m3_env.h"
#అన్తర్భావ "m3_bind.h"
#అన్తర్భావ "m3_deterministic.h"
#అన్తర్భావ "m3_compile.h"
#అన్తర్భావ "m3_host.h"
#అన్తర్భావ "m3_exception.h"
#అన్తర్భావ "m3_info.h"


IM3Environment m3_NewEnvironment ()
{
    IM3Environment env = m3_AllocStruct(M3Environment);

    ఒకవేళ (env) {
        _try
        {
            // create FuncTypes for all simple block return ValueTypes.
            // v128 is skipped: it parses as a slot but has no operations.
            కొరకు (u8 t = c_m3Type_none; t < c_m3_Type_లెక్క; t++) {
                ఒకవేళ (t == c_m3Type_v128) {
                    కొనసాగు;
                }

                IM3FuncType ftype;
_               (AllocFuncType(&ftype, 1));

                ftype->అంకె_వాదనలు  = 0;
                ftype->అంకె_Rets  = (t == c_m3Type_none) ? 0 : 1;
                ftype->types[0] = t;

                Environment_కూడు_Func_Type(env, &ftype);

                env->retFuncTypes[t] = ftype;
            }
        }

        _catch:
        ఒకవేళ (ఫలితం) {
            m3_FreeEnvironment(env);
            env = ఖాళీ;
        }
    }

    తిరిగి env;
}


శూన్య Environment_Release (IM3Environment i_environment)
{
    IM3FuncType ftype = i_environment->funcTypes;

    అయితే (ftype) {
        IM3FuncType తదుపరి = ftype->తదుపరి;
        m3_Free(ftype);
        ftype = తదుపరి;
    }

    m3log(runtime, "freeing %d pages from environment", లెక్క_సంకేతం_పుటలు(i_environment->పుటలు_Released));
    Free_సంకేతం_పుటలు(&i_environment->పుటలు_Released);
}


శూన్య m3_FreeEnvironment (IM3Environment i_environment)
{
    ఒకవేళ (i_environment) {
        Environment_Release(i_environment);
        m3_Free(i_environment);
    }
}


శూన్య m3_సమితి_Custom_Section_Handler (IM3Environment i_environment, M3SectionHandler i_handler)
{
    ఒకవేళ (i_environment) {
        i_environment->customSectionHandler = i_handler;
    }
}


// returns the same io_funcType or replaces it with an equivalent that's already in the type linked list
M3_ఫలితం Environment_కూడు_Func_Type (IM3Environment i_environment, IM3FuncType* io_funcType)
{
    IM3FuncType కూడు_Type = *io_funcType;
    IM3FuncType newType = i_environment->funcTypes;

    అయితే (newType) {
        ఒకవేళ (not newType->లోపల_Rec_సమూహం and AreFuncTypesEqual(newType, కూడు_Type)) {
            m3_Free(కూడు_Type);
            ఆపు;
        }

        newType = newType->తదుపరి;
    }

    ఒకవేళ (newType == ఖాళీ) {
        // a type index has to fit in the heap type field of an m3type_t
        ఒకవేళ (i_environment->అంకె_Func_Types >= d_m3_Max_Sane_Types_లెక్క) {
            m3_Free(కూడు_Type);
            *io_funcType = ఖాళీ;
            తిరిగి "too many distinct function types";
        }

        newType                  = కూడు_Type;
        newType->canonical_సూచీ  = i_environment->అంకె_Func_Types++;
        newType->తదుపరి            = i_environment->funcTypes;
        i_environment->funcTypes = newType;
    }

    *io_funcType = newType;

    తిరిగి m3Err_none;
}


// Adds a type without looking for an equal one first, and hands back the index
// it was given.
//
// Members of a recursive group do not get to share an entry with a type that
// merely looks like them: the group is their identity, so two groups spelled
// the same way are still two groups, and a type inside one is never the type
// outside it that matches field for field. Reserving the indices up front is
// also what lets the members refer to each other while they are being read.
M3_ఫలితం Environment_ReserveFuncTypes (IM3Environment i_environment, u32 i_లెక్క, u16* o_మొదటి_సూచీ)
{
    ఒకవేళ ((u64)i_environment->అంకె_Func_Types + i_లెక్క > d_m3_Max_Sane_Types_లెక్క) {
        తిరిగి "too many distinct function types";
    }

    *o_మొదటి_సూచీ = i_environment->అంకె_Func_Types;
    i_environment->అంకె_Func_Types += (u16)i_లెక్క;

    తిరిగి m3Err_none;
}


శూన్య Environment_AdoptFuncType (IM3Environment i_environment, IM3FuncType i_funcType, u16 i_canonical_సూచీ)
{
    i_funcType->లోపల_Rec_సమూహం     = true;
    i_funcType->canonical_సూచీ = i_canonical_సూచీ;
    i_funcType->తదుపరి           = i_environment->funcTypes;
    i_environment->funcTypes   = i_funcType;
}


IM3_సంకేతం_పుట Remove_సంకేతం_పుట_యొక్క_సామర్థ్యం (M3_సంకేతం_పుట** io_list, u32 i_minimum_పంక్తి_లెక్క)
{
    IM3_సంకేతం_పుట prev = ఖాళీ;
    IM3_సంకేతం_పుట పుట = *io_list;

    అయితే (పుట) {
        ఒకవేళ (అంకె_Free_పంక్తులు(పుట) >= i_minimum_పంక్తి_లెక్క) {             d_m3Assert (పుట->info.usage_లెక్క == 0);
            IM3_సంకేతం_పుట తదుపరి = పుట->info.తదుపరి;
            ఒకవేళ (prev) {
                prev->info.తదుపరి = తదుపరి; // mid-list
            } లేకపోతే {
                *io_list = తదుపరి;       // front of list
            }

            ఆపు;
        }

        prev = పుట;
        పుట = పుట->info.తదుపరి;
    }

    తిరిగి పుట;
}


IM3_సంకేతం_పుట Environment_Acquire_సంకేతం_పుట (IM3Environment i_environment, u32 i_minimum_పంక్తి_లెక్క)
{
    తిరిగి Remove_సంకేతం_పుట_యొక్క_సామర్థ్యం(&i_environment->పుటలు_Released, i_minimum_పంక్తి_లెక్క);
}


శూన్య Environment_Release_సంకేతం_పుటలు (IM3Environment i_environment, IM3_సంకేతం_పుట i_సంకేతం_పుట_List)
{
    IM3_సంకేతం_పుట చివర = i_సంకేతం_పుట_List;

    అయితే (చివర) {
        చివర->info.పంక్తి_సూచీ = 0; // reset page
#ఒకవేళ d_m3_రికార్డు_Backtraces
        చివర->info.mapping->కొలత = 0;
#అయితే_ముగింపు // d_m3RecordBacktraces

        IM3_సంకేతం_పుట తదుపరి = చివర->info.తదుపరి;
        ఒకవేళ (not తదుపరి) {
            ఆపు;
        }

        చివర = తదుపరి;
    }

    ఒకవేళ (చివర) {
        // push list to front
        చివర->info.తదుపరి               = i_environment->పుటలు_Released;
        i_environment->పుటలు_Released = i_సంకేతం_పుట_List;
    }
}


IM3Runtime m3_NewRuntime (IM3Environment i_environment, u32 i_దొంతర_కొలత_లోపల_బైట్లు, శూన్య* i_userdata)
{
    IM3Runtime runtime = m3_AllocStruct(M3Runtime);

    ఒకవేళ (runtime) {
        m3_ResetErrorInfo(runtime);

        runtime->environment = i_environment;
        runtime->userdata    = i_userdata;

        Deterministic_InitRuntime(runtime);

        runtime->origin_దొంతర = m3_Malloc("Wasm Stack", i_దొంతర_కొలత_లోపల_బైట్లు + 4 * పరిమాణం(m3slot_t)); // TODO: more precise stack checks

        ఒకవేళ (runtime->origin_దొంతర) {
            runtime->దొంతర         = runtime->origin_దొంతర;
            runtime->అంకె_దొంతర_Slots = i_దొంతర_కొలత_లోపల_బైట్లు / పరిమాణం(m3slot_t);          m3log (runtime, "new stack: %p, slots: %u", runtime->origin_దొంతర, runtime->అంకె_దొంతర_Slots);
        } లేకపోతే {
            m3_Free(runtime);
        }
    }

    తిరిగి runtime;
}

శూన్య m3_సమితి_Validation (IM3Runtime i_runtime, bool i_enable)
{
#ఒకవేళ d_m3EnableValidation
    ఒకవేళ (i_runtime) {
        i_runtime->skipValidation = not i_enable;
    }
#లేకపోతే
    (శూన్య)i_runtime;
    (శూన్య)i_enable;              // nothing to skip: the validator was compiled out
#అయితే_ముగింపు
}

M3_ఫలితం m3_సమితి_Resource_హద్దు (IM3Runtime i_runtime, M3_Resource_హద్దు i_హద్దు, uint64_t i_విలువ)
{
    ఒకవేళ (not i_runtime) {
        తిరిగి m3Err_mallocFailed;
    }
    ఎంపిక (i_హద్దు) {
    స్థితి c_m3_హద్దు_జ్ఞాపకం_బైట్లు:
        ఒకవేళ (i_విలువ and i_విలువ < i_runtime->జ్ఞాపకం_బైట్లు_వాడిన) {
            తిరిగి m3_Err_resource_హద్దు_క్రింద_Usage;
        }
        i_runtime->జ్ఞాపకం_బైట్లు_హద్దు = i_విలువ;
        తిరిగి m3Err_none;
    స్థితి c_m3_హద్దు_పట్టిక_Elements:
        ఒకవేళ (i_విలువ and i_విలువ < i_runtime->పట్టిక_Elements_వాడిన) {
            తిరిగి m3_Err_resource_హద్దు_క్రింద_Usage;
        }
        i_runtime->పట్టిక_Elements_హద్దు = i_విలువ;
        తిరిగి m3Err_none;
#ఒకవేళ d_m3_Has_దొంతర_Switching
    స్థితి c_m3_హద్దు_Continuations:
        ఒకవేళ (i_విలువ and i_విలువ < i_runtime->continuationsAllocated) {
            తిరిగి m3_Err_resource_హద్దు_క్రింద_Usage;
        }
        i_runtime->continuations_హద్దు = (u32)M3_MIN(i_విలువ, (u64)UINT32_MAX);
        అయితే (i_విలువ and i_runtime->val_దొంతర_Pool_లెక్క > i_విలువ - i_runtime->continuationsAllocated) {
            m3_Free(i_runtime->val_దొంతర_Pool[--i_runtime->val_దొంతర_Pool_లెక్క]);
        }
        తిరిగి m3Err_none;
#లేకపోతే
    స్థితి c_m3_హద్దు_Continuations: తిరిగి m3_Err_resource_హద్దు_Not_Supported;
#అయితే_ముగింపు
    స్థితి c_m3_హద్దు_Gas_Units:
#ఒకవేళ d_m3HasGasMetering
        i_runtime->gas_హద్దు = i_runtime->gas_మిగిలిన = (i64)M3_MIN(i_విలువ, (u64)INT64_MAX);
        తిరిగి m3Err_none;
#లేకపోతే
        తిరిగి m3_Err_resource_హద్దు_Not_Supported;
#అయితే_ముగింపు
    అప్రమేయ: తిరిగి m3_Err_unknown_Resource_హద్దు;
    }
}

uint64_t m3_Get_Resource_హద్దు (IM3Runtime i_runtime, M3_Resource_హద్దు i_హద్దు)
{
    ఒకవేళ (not i_runtime) {
        తిరిగి 0;
    }
    ఎంపిక (i_హద్దు) {
    స్థితి c_m3_హద్దు_జ్ఞాపకం_బైట్లు: తిరిగి i_runtime->జ్ఞాపకం_బైట్లు_హద్దు;
    స్థితి c_m3_హద్దు_పట్టిక_Elements: తిరిగి i_runtime->పట్టిక_Elements_హద్దు;
#ఒకవేళ d_m3_Has_దొంతర_Switching
    స్థితి c_m3_హద్దు_Continuations: తిరిగి i_runtime->continuations_హద్దు;
#అయితే_ముగింపు
#ఒకవేళ d_m3HasGasMetering
    స్థితి c_m3_హద్దు_Gas_Units: తిరిగి (u64)i_runtime->gas_హద్దు;
#అయితే_ముగింపు
    అప్రమేయ: తిరిగి 0;
    }
}

uint64_t m3_GetResourceUsage (IM3Runtime i_runtime, M3_Resource_హద్దు i_హద్దు)
{
    ఒకవేళ (not i_runtime) {
        తిరిగి 0;
    }
    ఎంపిక (i_హద్దు) {
    స్థితి c_m3_హద్దు_జ్ఞాపకం_బైట్లు: తిరిగి i_runtime->జ్ఞాపకం_బైట్లు_వాడిన;
    స్థితి c_m3_హద్దు_పట్టిక_Elements: తిరిగి i_runtime->పట్టిక_Elements_వాడిన;
#ఒకవేళ d_m3_Has_దొంతర_Switching
    స్థితి c_m3_హద్దు_Continuations: తిరిగి i_runtime->continuationsAllocated;
#అయితే_ముగింపు
#ఒకవేళ d_m3HasGasMetering
    స్థితి c_m3_హద్దు_Gas_Units: తిరిగి (u64)i_runtime->gas_హద్దు - (u64)i_runtime->gas_మిగిలిన;
#అయితే_ముగింపు
    అప్రమేయ: తిరిగి 0;
    }
}

శూన్య* m3_Get_User_దత్తాంశం (IM3Runtime i_runtime)
{
    తిరిగి i_runtime ? i_runtime->userdata : ఖాళీ;
}


శూన్య* ForEachModule (IM3Runtime i_runtime, ModuleVisitor i_visitor, శూన్య* i_info)
{
    శూన్య* r = ఖాళీ;

    IM3Module module = i_runtime->modules;

    అయితే (module) {
        IM3Module తదుపరి = module->తదుపరి;
        ఒకవేళ ((r = i_visitor(module, i_info))) {
            ఆపు;
        }

        module = తదుపరి;
    }

    తిరిగి r;
}


శూన్య* _FreeModule (IM3Module i_module, శూన్య* i_info)
{
    m3_FreeModule(i_module);
    తిరిగి ఖాళీ;
}


// A module's memory and table index spaces hold borrowed pointers: linking
// repoints an imported slot at the memory or table the exporting module owns.
// m3_FreeModule tells a borrowed slot from an owned one by reading that owner,
// which only holds while the object is still allocated - and the free walk
// below visits modules in load order, with no ordering between an owner and
// the modules borrowing from it. So clear the borrowed slots first, in a pass
// that frees nothing; the walk then skips them on the NULL check it already
// does, instead of reading a memory some earlier module has freed.
శూన్య* _ReleaseBorrowedSlots (IM3Module i_module, శూన్య* i_info)
{
    కొరకు (u32 i = 0; i < i_module->అంకె_Memories; ++i) {
        IM3_జ్ఞాపకం జ్ఞాపకం = i_module->memories[i];

        ఒకవేళ (జ్ఞాపకం and జ్ఞాపకం->owner != i_module) {
            i_module->memories[i] = ఖాళీ;
        }
    }

    కొరకు (u32 i = 0; i < i_module->అంకె_Tables; ++i) {
        IM3_పట్టిక పట్టిక_2 = i_module->tables[i];

        ఒకవేళ (పట్టిక_2 and పట్టిక_2->owner != i_module) {
            i_module->tables[i] = ఖాళీ;
        }
    }

    తిరిగి ఖాళీ;
}


శూన్య Runtime_Release (IM3Runtime i_runtime)
{
    ForEachModule(i_runtime, _ReleaseBorrowedSlots, ఖాళీ);
    ForEachModule(i_runtime, _FreeModule, ఖాళీ);                    d_m3Assert (i_runtime->అంకె_చురుకైన_సంకేతం_పుటలు == 0);

    Environment_Release_సంకేతం_పుటలు(i_runtime->environment, i_runtime->పుటలు_Open);
    Environment_Release_సంకేతం_పుటలు(i_runtime->environment, i_runtime->పుటలు_నిండిన);

    m3_Free(i_runtime->origin_దొంతర);

#ఒకవేళ d_m3EnableValidation
    m3_Free(i_runtime->validator);
#అయితే_ముగింపు

#ఒకవేళ d_m3HasExceptionHandling
    // a runtime let go of while suspended still holds its exceptions
    FreeExceptions(i_runtime);
    m3_Free(i_runtime->uncaughtException);
#అయితే_ముగింపు
#ఒకవేళ d_m3HasExceptionHandling || d_m3_Has_దొంతర_Switching
    అయితే (i_runtime->host_ట్యాగులు) {
        M3_Host_ట్యాగ్* తదుపరి = i_runtime->host_ట్యాగులు->తదుపరి;
        m3_Free(i_runtime->host_ట్యాగులు);
        i_runtime->host_ట్యాగులు = తదుపరి;
    }
#అయితే_ముగింపు
#ఒకవేళ d_m3_Has_దొంతర_Switching
    Continuation_ReleaseAll(i_runtime);
#అయితే_ముగింపు
}


శూన్య m3_FreeRuntime (IM3Runtime i_runtime)
{
    ఒకవేళ (i_runtime) {
        m3_PrintProfilerInfo();

        Runtime_Release(i_runtime);
        m3_Free(i_runtime);
    }
}


#ఒకవేళ d_m3_Has_దొంతర_Switching

IM3Continuation Continuation_New (IM3Runtime i_runtime, IM3FuncType i_type, IM3Function i_function)
{
    IM3Continuation cont = m3_AllocStruct(M3Continuation);
    ఒకవేళ (!cont) {
        తిరిగి ఖాళీ;
    }

    cont->type          = i_type;
    cont->నమోదు_Function = i_function;
    cont->state         = cont_allocated;

    cont->అంకె_దొంతర_Slots = d_m3_Continuation_దొంతర_Slots;
    // frames are recorded only when this continuation actually suspends, and
    // only as deep as it got, so the array starts out unallocated
    cont->చట్రాలు    = ఖాళీ;
    cont->చట్రాలు_Cap = 0;
    cont->అంకె_చట్రాలు = 0;

    cont->sp = cont->val_దొంతర;
    cont->pc = i_function ? i_function->compiled : ఖాళీ;
    cont->r0 = 0;
#  ఒకవేళ d_m3HasFloat
    cont->fp0 = 0;
#  endif
    cont->bound_వాదనలు_లెక్క    = 0;
    cont->అంకె_Suspend_Results = 0;
    cont->అంకె_Handlers       = 0;
    cont->handlersPC        = ఖాళీ;
    cont->మాతృ            = ఖాళీ;

    cont->తదుపరి               = i_runtime->continuations;
    i_runtime->continuations = cont;

    తిరిగి cont;
}

IM3Continuation Continuation_ForkSuspended (IM3Runtime i_runtime, IM3Continuation i_cont)
{
    IM3Continuation newCont = Continuation_New(i_runtime, i_cont->type, i_cont->నమోదు_Function);
    ఒకవేళ (M3_UNLIKELY(not newCont)) {
        తిరిగి ఖాళీ;
    }

    newCont->val_దొంతర      = i_cont->val_దొంతర;
    newCont->అంకె_దొంతర_Slots = i_cont->అంకె_దొంతర_Slots;
    newCont->sp            = i_cont->sp;
    newCont->pc            = i_cont->pc;
    newCont->suspend_బిందువు  = i_cont->suspend_బిందువు;
    newCont->r0            = i_cont->r0;
#  ఒకవేళ d_m3HasFloat
    newCont->fp0 = i_cont->fp0;
#  endif
    newCont->చట్రాలు    = i_cont->చట్రాలు;
    newCont->చట్రాలు_Cap = i_cont->చట్రాలు_Cap;
    newCont->అంకె_చట్రాలు = i_cont->అంకె_చట్రాలు;

    newCont->bound_వాదనలు_లెక్క = 0;
#  ఒకవేళ d_m3HasExceptionHandling
    newCont->resumeThrow = i_cont->resumeThrow;
    i_cont->resumeThrow  = ఖాళీ;
#  endif
    newCont->అంకె_Suspend_Results = i_cont->అంకె_Suspend_Results;
    కొరకు (u32 i = 0; i < i_cont->అంకె_Suspend_Results; ++i) {
        newCont->suspend_ఫలితం_Offsets[i] = i_cont->suspend_ఫలితం_Offsets[i];
        newCont->suspend_ఫలితం_Is64[i]    = i_cont->suspend_ఫలితం_Is64[i];
    }

    newCont->అంకె_Handlers = i_cont->అంకె_Handlers;
    newCont->handlersPC  = i_cont->handlersPC;
    newCont->మాతృ      = i_cont->మాతృ;
    newCont->state       = cont_suspended;

    i_cont->val_దొంతర  = ఖాళీ;
    i_cont->చట్రాలు    = ఖాళీ;
    i_cont->చట్రాలు_Cap = 0;
    i_cont->అంకె_చట్రాలు = 0;
    i_cont->state     = cont_consumed;

    తిరిగి newCont;
}

// Only cont.new and snapshot restoration acquire stacks. Binding and suspension
// transfer them between records without changing the active count.
M3_ఫలితం Continuation_Acquire_దొంతర (IM3Runtime runtime, IM3Continuation cont)
{
    ఒకవేళ (not Continuation_Can_Acquire_దొంతర(runtime)) {
        తిరిగి m3_Err_continuation_హద్దు_Exceeded;
    }
    ఒకవేళ (runtime->val_దొంతర_Pool_లెక్క) {
        cont->val_దొంతర = runtime->val_దొంతర_Pool[--runtime->val_దొంతర_Pool_లెక్క];
        memset(cont->val_దొంతర, 0, (cont->అంకె_దొంతర_Slots + 4) * పరిమాణం(m3slot_t));
    } లేకపోతే {
        cont->val_దొంతర = m3_AllocArray(m3slot_t, cont->అంకె_దొంతర_Slots + 4);
    }
    ఒకవేళ (not cont->val_దొంతర) {
        తిరిగి m3Err_mallocFailed;
    }
    cont->sp = cont->val_దొంతర;
    runtime->continuationsAllocated++;
    తిరిగి m3Err_none;
}

శూన్య Continuation_Recycle_దొంతర (IM3Runtime runtime, IM3Continuation cont)
{
    ఒకవేళ (not cont->val_దొంతర or cont == runtime->మూలం_Continuation) {
        తిరిగి;
    }
    ఒకవేళ (not runtime->val_దొంతర_Pool) {
        runtime->val_దొంతర_Pool = m3_AllocArray(m3slot_t*, 16);
        ఒకవేళ (runtime->val_దొంతర_Pool) {
            runtime->val_దొంతర_Pool_Cap = 16;
        }
    }
    ఒకవేళ (runtime->val_దొంతర_Pool_లెక్క < runtime->val_దొంతర_Pool_Cap and
        (not runtime->continuations_హద్దు or runtime->val_దొంతర_Pool_లెక్క <
                                              runtime->continuations_హద్దు - runtime->continuationsAllocated + 1)) {
        runtime->val_దొంతర_Pool[runtime->val_దొంతర_Pool_లెక్క++] = cont->val_దొంతర;
        cont->val_దొంతర                                      = ఖాళీ;
    } లేకపోతే {
        m3_Free(cont->val_దొంతర);
    }
    cont->sp = ఖాళీ;
    runtime->continuationsAllocated--;
}

శూన్య Continuation_ReleaseAll (IM3Runtime io_runtime)
{
    // Records live until teardown: a consumed reference can still be in a table
    // or on a Wasm stack. Its value stack has already been recycled.
    IM3Continuation cont = io_runtime->continuations;

    అయితే (cont) {
        IM3Continuation తదుపరి = cont->తదుపరి;

        m3_Free(cont->val_దొంతర);
        m3_Free(cont->చట్రాలు);
        m3_Free(cont);

        cont = తదుపరి;
    }

    కొరకు (u32 i = 0; i < io_runtime->val_దొంతర_Pool_లెక్క; ++i) {
        m3_Free(io_runtime->val_దొంతర_Pool[i]);
    }
    m3_Free(io_runtime->val_దొంతర_Pool);
    io_runtime->val_దొంతర_Pool_లెక్క = io_runtime->val_దొంతర_Pool_Cap = 0;
    io_runtime->continuationsAllocated                          = 0;
    io_runtime->continuations                                   = ఖాళీ;
    io_runtime->చురుకైన_Continuation                              = ఖాళీ;
    io_runtime->మూలం_Continuation                                = ఖాళీ;
}


m3ret_t Continuation_రికార్డు_చట్రం (IM3Runtime i_runtime, స్థిరరాశి M3_చట్రం* i_చట్రం)
{
    IM3Continuation cont = i_runtime->చురుకైన_Continuation;

    // the marker only travels between a suspend and the resume that runs it,
    // and that whole stretch has an active continuation
    d_m3Assert(cont);

    ఒకవేళ (cont->అంకె_చట్రాలు == cont->చట్రాలు_Cap) {
        u32 newCap = cont->చట్రాలు_Cap ? cont->చట్రాలు_Cap * 2 : 8;

        ఒకవేళ (newCap > d_m3_Continuation_Max_చట్రాలు) {
            newCap = d_m3_Continuation_Max_చట్రాలు;
        }

        ఒకవేళ (newCap == cont->చట్రాలు_Cap) {
            తిరిగి m3_Err_trap_దొంతర_అధికప్రవాహం;
        }

        M3_చట్రం* చట్రాలు = m3_ReallocArray(M3_చట్రం, cont->చట్రాలు, newCap, cont->చట్రాలు_Cap);

        ఒకవేళ (not చట్రాలు) {
            తిరిగి m3Err_mallocFailed;
        }

        cont->చట్రాలు    = చట్రాలు;
        cont->చట్రాలు_Cap = newCap;
    }

    cont->చట్రాలు[cont->అంకె_చట్రాలు++] = *i_చట్రం;

    తిరిగి m3Err_continuationSuspended;
}

#అయితే_ముగింపు // d_m3HasStackSwitching

M3_ఫలితం Evaluate_వ్యక్తీకరణ (IM3Module i_module, శూన్య* o_expressed, m3type_t i_type, బైట్లు_t* io_బైట్లు, cbytes_t i_చివర)
{
    M3_ఫలితం ఫలితం = m3Err_none;

    // OPTZ: use a simplified interpreter for expressions

    // create a temporary runtime context
#ఒకవేళ defined(d_m3PreferStaticAlloc)
    స్థిర M3Runtime runtime;
#లేకపోతే
    M3Runtime runtime;
#అయితే_ముగింపు
    M3_INIT(runtime);

    runtime.environment   = i_module->runtime->environment;
    runtime.అంకె_దొంతర_Slots = i_module->runtime->అంకె_దొంతర_Slots;
    runtime.దొంతర         = i_module->runtime->దొంతర;

    m3stack_t దొంతర = (m3stack_t)runtime.దొంతర;

    IM3Runtime savedRuntime = i_module->runtime;
    i_module->runtime       = &runtime;

    IM3Compilation o   = &runtime.compilation;
    o->runtime         = &runtime;
    o->module          = i_module;
    o->wasm            = *io_బైట్లు;
    o->wasm_చివర         = i_చివర;
    o->చివరి_Opcode_ప్రారంభం = o->wasm;

    // Borrowed for the length of this expression only. Runtime_Release below hands it
    // to Environment_ReleaseCodePages, which resets lineIndex, so the page comes back
    // empty and the next expression reuses it rather than burning fresh lines.
    o->పుట = Acquire_సంకేతం_పుట(&runtime);

    ఒకవేళ (o->పుట) {
        IM3FuncType ftype = ఖాళీ;

#ఒకవేళ d_m3HasTypedRefs
        // A reference type has to be checked as the type it is: the shared
        // return types are one per storage class, so borrowing one of those
        // would let any reference at all satisfy a particular one.
        ఒకవేళ (IsSpelledRefType(i_type)) {
            ఫలితం = AllocFuncType(&ftype, 1);

            ఒకవేళ (not ఫలితం) {
                ftype->అంకె_Rets  = 1;
                ftype->types[0] = i_type;

                ఫలితం = Environment_కూడు_Func_Type(runtime.environment, &ftype);
            }
        } లేకపోతే
#అయితే_ముగింపు
        {
            ftype = runtime.environment->retFuncTypes[పునాది_Type_యొక్క(i_type)];
        }

        pc_t m3code = Get_పుట_PC(o->పుట);

        ఒకవేళ (not ఫలితం) {
            ఫలితం = Compile_వ్యక్తీకరణ(o, ftype);
        }

        ఒకవేళ (not ఫలితం && o->max_దొంతర_Slots >= runtime.అంకె_దొంతర_Slots) {
            ఫలితం = m3_Err_trap_దొంతర_అధికప్రవాహం;
        }

        ఒకవేళ (not ఫలితం) {
#ఒకవేళ (d_m3EnableOpProfiling || d_m3EnableOpTracing)
            m3ret_t r = నిరంతరం_సంకేతం(m3code, దొంతర, ఖాళీ, d_m3_Op_Default_వాదనలు, d_m3_పునాది_Cstr);
#లేకపోతే
            m3ret_t r = నిరంతరం_సంకేతం(m3code, దొంతర, ఖాళీ, d_m3_Op_Default_వాదనలు);
#అయితే_ముగింపు

            ఒకవేళ (r == 0) {                                                                   m3log (runtime, "expression result: %s", SPrint_విలువ (దొంతర, i_type));
                ఒకవేళ (కొలత_యొక్క_Type(పునాది_Type_యొక్క(i_type)) == పరిమాణం(u32)) {
                    *(u32*)o_expressed = *((u32*)దొంతర);
                } లేకపోతే {
                    *(u64*)o_expressed = *((u64*)దొంతర);
                }
            }
        }

        Release_సంకేతం_పుట(&runtime, o->పుట);
    } లేకపోతే {
        ఫలితం = m3_Err_malloc_Failed_సంకేతం_పుట;
    }

    runtime.origin_దొంతర = ఖాళీ;        // prevent free(stack) in ReleaseRuntime
    Runtime_Release(&runtime);
    i_module->runtime = savedRuntime;

    *io_బైట్లు = o->wasm;

    తిరిగి ఫలితం;
}


//---------------------------------------------------------------------------------------------------------------------------------
//  Linking a module's imports against the exports of the modules already loaded
//  into the same runtime, matched on the name a module was registered under.
//
//  This is best-effort: an import nothing satisfies is left alone rather than
//  rejected, because a host function may still be bound to it after the module
//  is loaded (m3_LinkRawFunction needs the runtime, so it cannot run earlier),
//  and because an unsatisfiable memory or global still has to be backed by
//  something for the module to be loadable at all.
//---------------------------------------------------------------------------------------------------------------------------------

// Whether an exporting memory or table satisfies what an import asks for. The
// exporter's *current* size is its minimum - one that has been grown satisfies
// a larger import than its declaration would - and it may be no less bounded.
స్థిర
bool LimitsSatisfy (u64 i_exported_కొలత, bool i_exportedHasMax, u64 i_exportedMax,
                    u64 i_importMin, bool i_importHasMax, u64 i_importMax)
{
    ఒకవేళ (i_exported_కొలత < i_importMin) {
        తిరిగి false;
    }

    ఒకవేళ (i_importHasMax and (not i_exportedHasMax or i_exportedMax > i_importMax)) {
        తిరిగి false;
    }

    తిరిగి true;
}


// The export of this kind under this name, or NULL. The index it carries is
// resolved by the caller, against the module's index space as it stands then -
// after linking, a re-exported import's slot holds the entity it was linked to.
స్థిర
స్థిరరాశి M3Export* Module_కనుగొను_Export (IM3Module i_module, u8 i_రకం, cstr_t i_name)
{
    కొరకు (u32 i = 0; i < i_module->అంకె_Exports; ++i) {
        స్థిరరాశి M3Export* e = &i_module->exports[i];

        ఒకవేళ (e->రకం == i_రకం and strcmp(e->name, i_name) == 0) {
            తిరిగి e;
        }
    }

    తిరిగి ఖాళీ;
}


స్థిర
IM3Function Module_కనుగొను_Exported_Function (IM3Module i_module, cstr_t i_name)
{
    స్థిరరాశి M3Export* e = Module_కనుగొను_Export(i_module, d_external_రకం_function, i_name);

    // A module that re-exports an import names the placeholder here. Resolve to
    // the function that actually runs, or a host-side call would run it against
    // the importing module's memory.
    తిరిగి e ? Function_Implementation(&i_module->functions[e->index]) : ఖాళీ;
}


స్థిర
IM3_జ్ఞాపకం Module_కనుగొను_Exported_జ్ఞాపకం (IM3Module i_module, cstr_t i_name)
{
    స్థిరరాశి M3Export* e = Module_కనుగొను_Export(i_module, d_external_రకం_జ్ఞాపకం, i_name);

    తిరిగి e ? i_module->memories[e->index] : ఖాళీ;
}


స్థిర
IM3_పట్టిక Module_కనుగొను_Exported_పట్టిక (IM3Module i_module, cstr_t i_name)
{
    స్థిరరాశి M3Export* e = Module_కనుగొను_Export(i_module, d_external_రకం_పట్టిక, i_name);

    తిరిగి e ? i_module->tables[e->index] : ఖాళీ;
}


స్థిర
IM3Global Module_కనుగొను_Exported_Global (IM3Module i_module, cstr_t i_name)
{
    స్థిరరాశి M3Export* e = Module_కనుగొను_Export(i_module, d_external_రకం_global, i_name);

    తిరిగి e ? &i_module->globals[e->index] : ఖాళీ;
}


#ఒకవేళ d_m3HasExceptionHandling || d_m3_Has_దొంతర_Switching
స్థిర
IM3_ట్యాగ్ Module_కనుగొను_Exported_ట్యాగ్ (IM3Module i_module, cstr_t i_name)
{
    స్థిరరాశి M3Export* e = Module_కనుగొను_Export(i_module, d_external_రకం_ట్యాగ్, i_name);

    ఒకవేళ (not e) {
        తిరిగి ఖాళీ;
    }

    IM3_ట్యాగ్ ట్యాగ్ = &i_module->ట్యాగులు[e->index];

    తిరిగి ట్యాగ్->resolved ? ట్యాగ్->resolved : ట్యాగ్;
}
#అయితే_ముగింపు


// Whether the module exports anything at all under this name. Export names are
// unique within a module, so a name one of the lookups above missed but this
// one finds is exported as something else - a kind the import cannot be
// satisfied by, rather than a name the module never exported.
స్థిర
bool Module_HasExport (IM3Module i_module, cstr_t i_name)
{
    కొరకు (u32 i = 0; i < i_module->అంకె_Exports; ++i) {
        ఒకవేళ (strcmp(i_module->exports[i].name, i_name) == 0) {
            తిరిగి true;
        }
    }

    తిరిగి false;
}


// Points each of the module's imports at whatever already-loaded module exports
// it. Runs before anything is allocated or initialized: a memory import has to
// be resolved before InitMemory would give it pages of its own, and a global
// import before InitGlobals runs an initializer that reads it.
స్థిర
M3_ఫలితం LinkImports (IM3Runtime io_runtime, IM3Module io_module)
{
    M3_ఫలితం ఫలితం = m3Err_none;

    కొరకు (u32 i = 0; i < io_module->అంకె_Functions; ++i) {
        IM3Function f = &io_module->functions[i];

        ఒకవేళ (f->wasm or not (f->import.moduleUtf8 and f->import.క్షేత్రం_Utf8)) {
            కొనసాగు;
        }

        IM3Module from = m3_కనుగొను_Module(io_runtime, f->import.moduleUtf8);
        ఒకవేళ (not from) {
            కొనసాగు;
        }

        IM3Function exported = Module_కనుగొను_Exported_Function(from, f->import.క్షేత్రం_Utf8);
        ఒకవేళ (not exported) {
            // the import names a module that is loaded, so its exports settle the
            // question: a name it exports as something else is a type mismatch, and
            // one it does not export at all is an unknown import
            _throwif(m3Err_incompatibleImportType, Module_HasExport(from, f->import.క్షేత్రం_Utf8));
            _throw(m3Err_unknownImport);
        }

        // func types are canonical within an environment, so this is the
        // structural equivalence the spec asks for
        _throwif(m3Err_incompatibleImportType, exported->funcType != f->funcType);

        f->resolved = Function_Implementation(exported);
    }

    కొరకు (u32 i = 0; i < io_module->అంకె_Memories; ++i) {
        IM3_జ్ఞాపకం జ్ఞాపకం = io_module->memories[i];

        ఒకవేళ (not జ్ఞాపకం->imported or జ్ఞాపకం->owner != io_module) {
            కొనసాగు;
        }

        IM3Module from = m3_కనుగొను_Module(io_runtime, జ్ఞాపకం->import.moduleUtf8);
        ఒకవేళ (not from) {
            కొనసాగు;
        }

        IM3_జ్ఞాపకం exported = Module_కనుగొను_Exported_జ్ఞాపకం(from, జ్ఞాపకం->import.క్షేత్రం_Utf8);
        ఒకవేళ (not exported) {
            _throwif(m3Err_incompatibleImportType, Module_HasExport(from, జ్ఞాపకం->import.క్షేత్రం_Utf8));
            _throw(m3Err_unknownImport);
        }

        // the address type is part of the memory type, so an i64 memory does
        // not satisfy an i32 import, or the other way round - and so is the page
        // size, which custom page sizes made a declared property of a memory
        _throwif(m3Err_incompatibleImportType, exported->isMemory64 != జ్ఞాపకం->isMemory64);
        _throwif(m3Err_incompatibleImportType, జ్ఞాపకం_పుట_కొలత(exported) != జ్ఞాపకం_పుట_కొలత(జ్ఞాపకం));

        // a shared memory is not a subtype of an unshared one, or the other way round
        _throwif(m3Err_incompatibleImportType, exported->isShared != జ్ఞాపకం->isShared);

        _throwif(m3Err_incompatibleImportType,
                 not LimitsSatisfy(exported->అంకె_పుటలు, exported->hasMax, exported->max_పుటలు,
                                   జ్ఞాపకం->init_పుటలు, జ్ఞాపకం->hasMax, జ్ఞాపకం->max_పుటలు));

        // hand the slot over to the exporter's memory, and drop the placeholder
        Free_జ్ఞాపకం_ఖండం(జ్ఞాపకం);
        FreeImportInfo(&జ్ఞాపకం->import);
        m3_Free(జ్ఞాపకం);

        io_module->memories[i] = exported;
    }

    కొరకు (u32 i = 0; i < io_module->అంకె_Tables; ++i) {
        IM3_పట్టిక పట్టిక_2 = io_module->tables[i];

        ఒకవేళ (not పట్టిక_2->imported or పట్టిక_2->owner != io_module) {
            కొనసాగు;
        }

        IM3Module from = m3_కనుగొను_Module(io_runtime, పట్టిక_2->import.moduleUtf8);
        ఒకవేళ (not from) {
            కొనసాగు;
        }

        IM3_పట్టిక exported = Module_కనుగొను_Exported_పట్టిక(from, పట్టిక_2->import.క్షేత్రం_Utf8);
        ఒకవేళ (not exported) {
            _throwif(m3Err_incompatibleImportType, Module_HasExport(from, పట్టిక_2->import.క్షేత్రం_Utf8));
            _throw(m3Err_unknownImport);
        }

        _throwif(m3Err_incompatibleImportType, exported->type != పట్టిక_2->type);

        // the index type is part of the table type, the same way it is for a memory
        _throwif(m3Err_incompatibleImportType, exported->isTable64 != పట్టిక_2->isTable64);

        _throwif(m3Err_incompatibleImportType,
                 not LimitsSatisfy(exported->కొలత, exported->hasMax, exported->max_కొలత,
                                   పట్టిక_2->init_కొలత, పట్టిక_2->hasMax, పట్టిక_2->max_కొలత));

        // hand the slot over to the exporter's table, and drop the placeholder
        m3_Free(పట్టిక_2->elements);
        FreeImportInfo(&పట్టిక_2->import);
        m3_Free(పట్టిక_2);

        io_module->tables[i] = exported;
    }

    కొరకు (u32 i = 0; i < io_module->అంకె_Globals; ++i) {
        IM3Global g = &io_module->globals[i];

        ఒకవేళ (not g->imported or not (g->import.moduleUtf8 and g->import.క్షేత్రం_Utf8)) {
            కొనసాగు;
        }

        IM3Module from = m3_కనుగొను_Module(io_runtime, g->import.moduleUtf8);
        ఒకవేళ (not from) {
            కొనసాగు;
        }

        IM3Global exported = Module_కనుగొను_Exported_Global(from, g->import.క్షేత్రం_Utf8);
        ఒకవేళ (not exported) {
            _throwif(m3Err_incompatibleImportType, Module_HasExport(from, g->import.క్షేత్రం_Utf8));
            _throw(m3Err_unknownImport);
        }

        _throwif(m3Err_incompatibleImportType, exported->type != g->type);
        _throwif(m3Err_incompatibleImportType, exported->isMutable != g->isMutable);

        g->resolved = exported->resolved ? exported->resolved : exported;
    }

#ఒకవేళ d_m3HasExceptionHandling || d_m3_Has_దొంతర_Switching
    కొరకు (u32 i = 0; i < io_module->అంకె_ట్యాగులు; ++i) {
        IM3_ట్యాగ్ ట్యాగ్ = &io_module->ట్యాగులు[i];

        ఒకవేళ (not ట్యాగ్->imported or not (ట్యాగ్->import.moduleUtf8 and ట్యాగ్->import.క్షేత్రం_Utf8)) {
            కొనసాగు;
        }

        IM3Module from = m3_కనుగొను_Module(io_runtime, ట్యాగ్->import.moduleUtf8);
        ఒకవేళ (not from) {
            కొనసాగు;
        }

        IM3_ట్యాగ్ exported = Module_కనుగొను_Exported_ట్యాగ్(from, ట్యాగ్->import.క్షేత్రం_Utf8);
        ఒకవేళ (not exported) {
            _throwif(m3Err_incompatibleImportType, Module_HasExport(from, ట్యాగ్->import.క్షేత్రం_Utf8));
            _throw(m3Err_unknownImport);
        }

        _throwif(m3Err_incompatibleImportType, exported->type != ట్యాగ్->type);

        ట్యాగ్->resolved = exported->resolved ? exported->resolved : exported;
    }
#అయితే_ముగింపు

    _catch: తిరిగి ఫలితం;
}


// Backs each of the module's memories with pages. LinkImports has already
// pointed any import it could satisfy at the exporting module's memory, and
// those are skipped here. An import nothing satisfied is still backed locally
// from its own declared limits, so that the module remains loadable.
M3_ఫలితం Init_జ్ఞాపకం (IM3Runtime io_runtime, IM3Module i_module)
{
    M3_ఫలితం ఫలితం = m3Err_none;

    // Fixed from here on: the index space stops changing after parse, and
    // linking has already repointed any slot it was going to.
    i_module->memory0 = i_module->అంకె_Memories ? i_module->memories[0]
                                              : &i_module->ఖాళీలు_జ్ఞాపకం;

    ఒకవేళ (i_module->అంకె_Memories == 0) {
        // nothing addressable, but _mem still has to point somewhere
        i_module->ఖాళీలు_జ్ఞాపకం.owner    = i_module;
        i_module->ఖాళీలు_జ్ఞాపకం.పుట_కొలత = d_m3_Default_Mem_పుట_కొలత;

_       (Resize_జ్ఞాపకం(io_runtime, &i_module->ఖాళీలు_జ్ఞాపకం, 0));
    }

    కొరకు (u32 i = 0; i < i_module->అంకె_Memories; ++i) {
        IM3_జ్ఞాపకం జ్ఞాపకం = i_module->memories[i];

        // a slot that already points at another module's memory is that
        // module's to allocate
        ఒకవేళ (జ్ఞాపకం->owner != i_module or జ్ఞాపకం->mallocated) {
            కొనసాగు;
        }

        u32 పుట_కొలత = జ్ఞాపకం_పుట_కొలత(జ్ఞాపకం);

        జ్ఞాపకం->పుట_కొలత = పుట_కొలత;

        // Without a declared maximum a memory may grow to the spec limit of
        // 2^|addrtype|/pagesize pages, which is the usual 65536 at the default
        // page size, 2^48 for a 64-bit memory, and a whole address space of
        // them when a page is a single byte. A declared maximum of zero is a
        // real limit, not the absence of one.
        //
        // 2^64/pagesize overflows a u64 only when a page is a single byte, and
        // a power-of-two page size divides the address space exactly, so the
        // 64-bit division below is 2^64/pagesize written so that it fits.
        ఒకవేళ (not జ్ఞాపకం->hasMax) {
            జ్ఞాపకం->max_పుటలు = జ్ఞాపకం->isMemory64
                                 ? (పుట_కొలత > 1 ? (UINT64_MAX / పుట_కొలత) + 1 : UINT64_MAX)
                                 : (0x100000000ull / పుట_కొలత);
        }

_       (Resize_జ్ఞాపకం(io_runtime, జ్ఞాపకం, జ్ఞాపకం->init_పుటలు));
    }

    _catch: తిరిగి ఫలితం;
}


M3_ఫలితం Resize_జ్ఞాపకం (IM3Runtime io_runtime, IM3_జ్ఞాపకం జ్ఞాపకం, u64 i_అంకె_పుటలు)
{
    M3_ఫలితం ఫలితం = m3Err_none;

    u64 అంకె_పుటలు_గమ్యం_Alloc = i_అంకె_పుటలు;

    ఒకవేళ (అంకె_పుటలు_గమ్యం_Alloc <= జ్ఞాపకం->max_పుటలు) {
        // A 64-bit memory may ask for up to 2^48 pages, which overflows a u64
        // of bytes. Nothing that large can be backed, so refuse it up front
        // rather than multiplying into a wrapped size.
        _throwif("linear memory limitation exceeded",
                 అంకె_పుటలు_గమ్యం_Alloc > d_m3_చిరునామా_హద్దు / జ్ఞాపకం->పుట_కొలత);

        u64 అంకె_పుట_బైట్లు = అంకె_పుటలు_గమ్యం_Alloc * జ్ఞాపకం->పుట_కొలత;

#ఒకవేళ d_m3_Max_రేఖీయ_జ్ఞాపకం_పుటలు > 0
        // the limit is a memory size, counted in default-sized pages; comparing
        // it against a raw page count would make it 65536 times stricter for a
        // module whose pages are one byte
        _throwif("linear memory limitation exceeded",
                 అంకె_పుట_బైట్లు > (u64)d_m3_Max_రేఖీయ_జ్ఞాపకం_పుటలు * d_m3_Default_Mem_పుట_కొలత);
#అయితే_ముగింపు

        // Limit the amount of memory that gets actually allocated
        ఒకవేళ (io_runtime->జ్ఞాపకం_హద్దు) {
            అంకె_పుట_బైట్లు = M3_MIN(అంకె_పుట_బైట్లు, (u64)io_runtime->జ్ఞాపకం_హద్దు);
        }

        u64 old_నిడివి = జ్ఞాపకం->mallocated ? జ్ఞాపకం->mallocated->నిడివి : 0;
        u64 other_వాడిన = io_runtime->జ్ఞాపకం_బైట్లు_వాడిన - old_నిడివి;
        _throwif(m3_Err_జ్ఞాపకం_హద్దు_Exceeded, io_runtime->జ్ఞాపకం_బైట్లు_హద్దు and
                                              (అంకె_పుట_బైట్లు > io_runtime->జ్ఞాపకం_బైట్లు_హద్దు or
                                               other_వాడిన > io_runtime->జ్ఞాపకం_బైట్లు_హద్దు - అంకె_పుట_బైట్లు));

        _throwif("linear memory limitation exceeded", అంకె_పుట_బైట్లు > (u64)SIZE_MAX - పరిమాణం(M3_జ్ఞాపకం_శీర్షిక));

#ఒకవేళ d_m3_Guarded_జ్ఞాపకం

        // A slot holds every address a Wasm access can name, and nothing beyond
        // that: a memory larger than the data part of one would put addresses past
        // the guard, where a fault is nobody's to recognize
        _throwif("linear memory limitation exceeded", అంకె_పుట_బైట్లు > d_m3_Guarded_దత్తాంశం_బైట్లు);

        // What makes an access past the end fault is the page after the memory not
        // being committed, so the end has to fall on one of the system's page
        // boundaries. A Wasm page divides evenly into them and always will; a memory
        // that declared a page size of its own may not, and then the bytes between
        // the end of the memory and the end of its last system page would answer
        // instead of trapping. Refusing to back such a memory is the honest way out:
        // the custom page sizes proposal exists for targets far too small to reserve
        // 8GiB per memory in the first place. See d_m3GuardedMemory.
        _throwif("guarded memory needs a page size the system's divides into",
                 (జ్ఞాపకం->పుట_కొలత % m3_Host_పుట_కొలత()) != 0);

        ఒకవేళ (జ్ఞాపకం->guard_స్థానం == ఖాళీ) {
            జ్ఞాపకం->guard_స్థానం = Guard_Take_స్థానం();
            _throwifnull(జ్ఞాపకం->guard_స్థానం);
        }

        // Growing only ever commits more of the same slot, so the memory never
        // moves - which the interpreter does not rely on, but nothing minds
        _throwif(m3Err_mallocFailed, not Guard_Commit_స్థానం(జ్ఞాపకం->guard_స్థానం, (size_t)అంకె_పుట_బైట్లు));

        జ్ఞాపకం->mallocated = Guard_స్థానం_శీర్షిక(జ్ఞాపకం->guard_స్థానం);

#లేకపోతే

        size_t అంకె_బైట్లు = (size_t)అంకె_పుట_బైట్లు + పరిమాణం(M3_జ్ఞాపకం_శీర్షిక);

        // What is really there, which is not numPages * pageSize: the limit above
        // caps the bytes without capping the page count, so a memory can be short
        // of a page - by design, since a whole 64 KiB page is often more than an
        // MCU has. Measuring the old block by the page count would overstate it,
        // and then realloc never sees the size it already has and copies the
        // whole limit again on every grow past it.
        size_t అంకె_మునుపటి_బైట్లు = జ్ఞాపకం->mallocated ? జ్ఞాపకం->mallocated->నిడివి + పరిమాణం(M3_జ్ఞాపకం_శీర్షిక) : 0;

        శూన్య* newMem = m3_Realloc("Wasm Linear Memory", జ్ఞాపకం->mallocated, అంకె_బైట్లు, అంకె_మునుపటి_బైట్లు);
        _throwifnull(newMem);

        జ్ఞాపకం->mallocated = (M3_జ్ఞాపకం_శీర్షిక*)newMem;

#అయితే_ముగింపు

#ఒకవేళ d_m3LogRuntime
        M3_జ్ఞాపకం_శీర్షిక* oldMallocated = జ్ఞాపకం->mallocated;
#అయితే_ముగింపు

        io_runtime->జ్ఞాపకం_బైట్లు_వాడిన = other_వాడిన + అంకె_పుట_బైట్లు;
        జ్ఞాపకం->అంకె_పుటలు            = అంకె_పుటలు_గమ్యం_Alloc;

        జ్ఞాపకం->mallocated->నిడివి  = (size_t)అంకె_పుట_బైట్లు;
        జ్ఞాపకం->mallocated->runtime = io_runtime;
        జ్ఞాపకం->mallocated->జ్ఞాపకం  = జ్ఞాపకం;

        జ్ఞాపకం->mallocated->max_దొంతర = (m3slot_t*)io_runtime->origin_దొంతర + io_runtime->అంకె_దొంతర_Slots;

        m3log(runtime, "resized old: %p; mem: %p; length: %zu; pages: %llu", oldMallocated, జ్ఞాపకం->mallocated, జ్ఞాపకం->mallocated->నిడివి, (గుర్తులేని దీర్ఘ దీర్ఘ)జ్ఞాపకం->అంకె_పుటలు);
    } లేకపోతే {
        ఫలితం = m3_Err_wasm_జ్ఞాపకం_అధికప్రవాహం;
    }

    _catch: తిరిగి ఫలితం;
}


శూన్య Free_జ్ఞాపకం_ఖండం (IM3_జ్ఞాపకం io_జ్ఞాపకం)
{
    ఒకవేళ (io_జ్ఞాపకం->mallocated and io_జ్ఞాపకం->mallocated->runtime) {
        io_జ్ఞాపకం->mallocated->runtime->జ్ఞాపకం_బైట్లు_వాడిన -= io_జ్ఞాపకం->mallocated->నిడివి;
    }
#ఒకవేళ d_m3_Guarded_జ్ఞాపకం
    Guard_Give_స్థానం(io_జ్ఞాపకం->guard_స్థానం);
    io_జ్ఞాపకం->guard_స్థానం = ఖాళీ;

    io_జ్ఞాపకం->mallocated = ఖాళీ;
#లేకపోతే
    m3_Free(io_జ్ఞాపకం->mallocated);
#అయితే_ముగింపు
}


M3_ఫలితం InitGlobals (IM3Module io_module)
{
    M3_ఫలితం ఫలితం = m3Err_none;

    ఒకవేళ (io_module->అంకె_Globals) {
        // placing the globals in their structs isn't good for cache locality, but i don't really know what the global
        // access patterns typically look like yet.

        //          io_module->globalMemory = m3Alloc (m3reg_t, io_module->numGlobals);

        //          if (io_module->globalMemory)
        {
            కొరకు (u32 i = 0; i < io_module->అంకె_Globals; ++i) {
                M3Global* g = &io_module->globals[i];                           m3log (runtime, "initializing global: %d", i);

                ఒకవేళ (g->init_సమాసం) {
                    బైట్లు_t ప్రారంభం = g->init_సమాసం;

                    ఫలితం = Evaluate_వ్యక్తీకరణ(io_module, &g->i64_విలువ, g->type, &ప్రారంభం, g->init_సమాసం + g->init_సమాసం_కొలత);

                    ఒకవేళ (not ఫలితం) {
                        // io_module->globalMemory [i] = initValue;
                    } లేకపోతే {
                        ఆపు;
                    }
                } లేకపోతే {                                                        m3log (runtime, "importing global");
                }
            }
        }
        //          else result = ErrorModule (m3Err_mallocFailed, io_module, "could allocate globals for module: '%s", io_module->name);
    }

    తిరిగి ఫలితం;
}


M3_ఫలితం Init_దత్తాంశం_Segments (IM3Module io_module)
{
    M3_ఫలితం ఫలితం = m3Err_none;

    కొరకు (u32 i = 0; i < io_module->అంకె_దత్తాంశం_Segments; ++i) {
        M3_దత్తాంశం_Segment* segment = &io_module->దత్తాంశం_Segments[i];

        // A passive segment stays available for memory.init until data.drop.
        // An active one is copied here and then counts as dropped.
        ఒకవేళ (segment->isPassive) {
            కొనసాగు;
        }

        _throwif("data segment memory index out of range",
                 segment->జ్ఞాపకం_Region >= io_module->అంకె_Memories);

        IM3_జ్ఞాపకం io_జ్ఞాపకం = io_module->memories[segment->జ్ఞాపకం_Region];

        _throwif("unallocated linear memory", !(io_జ్ఞాపకం->mallocated));

        // The offset expression has the memory's address type, and is
        // unsigned: an i32 offset of -1 is 4294967295, way out of bounds
        // rather than negative.
        u64     segment_స్థానభ్రంశం = 0;
        బైట్లు_t ప్రారంభం         = segment->init_సమాసం;

        ఒకవేళ (io_జ్ఞాపకం->isMemory64) {
_           (Evaluate_వ్యక్తీకరణ(io_module, &segment_స్థానభ్రంశం, c_m3Type_i64, &ప్రారంభం, segment->init_సమాసం + segment->init_సమాసం_కొలత));
        } లేకపోతే {
            u32 offset32;
_           (Evaluate_వ్యక్తీకరణ(io_module, &offset32, c_m3Type_i32, &ప్రారంభం, segment->init_సమాసం + segment->init_సమాసం_కొలత));
            segment_స్థానభ్రంశం = offset32;
        }

        m3log(runtime, "loading data segment: %d; size: %d; offset: %llu", i, segment->కొలత, (గుర్తులేని దీర్ఘ దీర్ఘ)segment_స్థానభ్రంశం);

        ఒకవేళ (segment_స్థానభ్రంశం <= io_జ్ఞాపకం->mallocated->నిడివి &&
            (u64)segment->కొలత <= io_జ్ఞాపకం->mallocated->నిడివి - segment_స్థానభ్రంశం) {
            u8* dest = m3_Mem_దత్తాంశం(io_జ్ఞాపకం->mallocated) + segment_స్థానభ్రంశం;
            జ్ఞాపకం_నకలు(dest, segment->దత్తాంశం, segment->కొలత);
        } లేకపోతే {
            _throw("data segment out of bounds");
        }

        segment->dropped = true;
    }

    _catch: తిరిగి ఫలితం;
}


// Turns a segment's elements into references. Element expressions are constant
// expressions restricted to ref.null/ref.func, so they're read directly rather
// than run through the compiler.
స్థిర
M3_ఫలితం ResolveElements (IM3Module io_module, M3ElementSegment* i_segment, శూన్య** o_elements)
{
    M3_ఫలితం ఫలితం = m3Err_none;

    బైట్లు_t  pos = i_segment->elements;
    cbytes_t చివర = io_module->element_Section_చివర;

    కొరకు (u32 e = 0; e < i_segment->అంకె_Elements; ++e) {
        u32   func_సూచీ;
        శూన్య* ref = ఖాళీ;

        ఒకవేళ (i_segment->is_సమాసం) {
            m3opcode_t opcode;
_           (Read_opcode(&opcode, &pos, చివర));

            ఒకవేళ (opcode == c_waOp_refFunc) {
_               (ReadLEB_u32(&func_సూచీ, &pos, చివర));
                _throwif("function index out of range", func_సూచీ >= io_module->అంకె_Functions);
                ref = Function_Implementation(&io_module->functions[func_సూచీ]);
            } లేకపోతే ఒకవేళ (opcode == c_waOp_refNull) {
                i8 waType;
                u8 nullType;
_               (ReadLEB_i7(&waType, &pos, చివర));
_               (NormalizeType(&nullType, waType));
                _throwif(m3Err_typeMismatch, nullType != i_segment->type);
            } లేకపోతే ఒకవేళ (opcode == c_waOp_getGlobal) {
                // wasm 2.0 lets an element expression read an imported
                // immutable global, which is how one module seeds another's
                // table with a reference it exported.
                u32 global_సూచీ;
_               (ReadLEB_u32(&global_సూచీ, &pos, చివర));
                _throwif(m3_Err_globa_సూచీ_బహిర్గతం_యొక్క_Bounds, global_సూచీ >= io_module->అంకె_Globals);

                IM3Global global = &io_module->globals[global_సూచీ];

                _throwif(m3_Err_globa_సూచీ_బహిర్గతం_యొక్క_Bounds, not global->imported);
                _throwif(m3Err_wasmMalformed, global->isMutable);
                _throwif(m3Err_typeMismatch, పునాది_Type_యొక్క(global->type) != పునాది_Type_యొక్క(i_segment->type));

                // read the cell the import was linked to, not the placeholder
                ఒకవేళ (global->resolved) {
                    global = global->resolved;
                }

                ref = global->ref_విలువ;
            } లేకపోతే {
                _throw("constant expression required");
            }

_           (Read_opcode(&opcode, &pos, చివర));
            _throwif(m3Err_wasmMalformed, opcode != c_wa_Op_చివర);
        } లేకపోతే {
_           (ReadLEB_u32(&func_సూచీ, &pos, చివర));
            _throwif("function index out of range", func_సూచీ >= io_module->అంకె_Functions);
            ref = Function_Implementation(&io_module->functions[func_సూచీ]);
        }

        o_elements[e] = ref;
    }

    _catch: తిరిగి ఫలితం;
}


M3_ఫలితం Init_పట్టిక_And_Elements (IM3Module io_module)
{
    M3_ఫలితం ఫలితం = m3Err_none;

    cbytes_t చివర = io_module->element_Section_చివర;
    M3_పట్టిక* పట్టిక_2;

    కొరకు (u32 i = 0; i < io_module->అంకె_Tables; ++i) {
        పట్టిక_2 = io_module->tables[i];

        // a slot pointing at another module's table is that module's to fill
        ఒకవేళ (పట్టిక_2->owner != io_module) {
            కొనసాగు;
        }

        ఒకవేళ (పట్టిక_2->కొలత) {
            IM3Runtime runtime = io_module->runtime;
            _throwif(m3_Err_పట్టిక_హద్దు_Exceeded, runtime->పట్టిక_Elements_హద్దు and
                                                 పట్టిక_2->కొలత > runtime->పట్టిక_Elements_హద్దు - runtime->పట్టిక_Elements_వాడిన);
            పట్టిక_2->elements = m3_AllocArray(శూన్య*, పట్టిక_2->కొలత);
            _throwifnull(పట్టిక_2->elements);
            runtime->పట్టిక_Elements_వాడిన += పట్టిక_2->కొలత;

            ఒకవేళ (పట్టిక_2->init_సమాసం) {
                శూన్య*   విలువ = ఖాళీ;
                బైట్లు_t ప్రారంభం = పట్టిక_2->init_సమాసం;
_               (Evaluate_వ్యక్తీకరణ(io_module, &విలువ, పునాది_Type_యొక్క(పట్టిక_2->type),
                                     &ప్రారంభం, పట్టిక_2->init_సమాసం + పట్టిక_2->init_సమాసం_కొలత));

                కొరకు (u32 e = 0; e < పట్టిక_2->కొలత; ++e) {
                    పట్టిక_2->elements[e] = విలువ;
                }
            }
        }
    }

    కొరకు (u32 i = 0; i < io_module->అంకె_Element_Segments; ++i) {
        M3ElementSegment* segment = &io_module->elementSegments[i];

        // Declarative segments only make their functions referenceable, and
        // passive ones wait for table.init, so neither is written out here.
        ఒకవేళ (segment->బహుళకం == c_m3Elem_declarative) {
            segment->dropped = true;
            కొనసాగు;
        }

        ఒకవేళ (segment->బహుళకం == c_m3Elem_passive) {
            ఒకవేళ (segment->అంకె_Elements) {
                segment->resolved = m3_AllocArray(శూన్య*, segment->అంకె_Elements);
                _throwifnull(segment->resolved);
_               (ResolveElements(io_module, segment, segment->resolved));
            }
            కొనసాగు;
        }

        పట్టిక_2 = io_module->tables[segment->పట్టిక_సూచీ];

        // The offset expression has the table's index type, and is unsigned:
        // an i32 offset of -1 is 4294967295, out of bounds rather than negative.
        u64     offset = 0;
        బైట్లు_t సమాసం   = segment->init_సమాసం;

        ఒకవేళ (పట్టిక_2->isTable64) {
_           (Evaluate_వ్యక్తీకరణ(io_module, &offset, c_m3Type_i64, &సమాసం, చివర));
        } లేకపోతే {
            u32 offset32;
_           (Evaluate_వ్యక్తీకరణ(io_module, &offset32, c_m3Type_i32, &సమాసం, చివర));
            offset = offset32;
        }

        _throwif("out of bounds table access",
                 offset > పట్టిక_2->కొలత or segment->అంకె_Elements > పట్టిక_2->కొలత - offset);

_       (ResolveElements(io_module, segment, పట్టిక_2->elements + offset));

        segment->dropped = true;
    }

    _catch: తిరిగి ఫలితం;
}

M3_ఫలితం m3_CompileModule (IM3Module io_module)
{
    M3_ఫలితం ఫలితం = m3Err_none;

    కొరకు (u32 i = 0; i < io_module->అంకె_Functions; ++i) {
        IM3Function f = &io_module->functions[i];
        ఒకవేళ (f->wasm and not f->compiled) {
_           (CompileFunction(f));
        }
    }

    _catch: తిరిగి ఫలితం;
}

#ఒకవేళ d_m3HasExceptionHandling

M3Exception* NewException (IM3Runtime io_runtime, IM3_ట్యాగ్ i_ట్యాగ్, u32 i_అంకె_వాదనలు)
{
    M3Exception* exception = (M3Exception*)m3_Malloc("M3Exception", పరిమాణం(M3Exception) + i_అంకె_వాదనలు * పరిమాణం(u64));

    ఒకవేళ (exception) {
        exception->ట్యాగ్     = i_ట్యాగ్;
        exception->అంకె_వాదనలు = i_అంకె_వాదనలు;
        exception->reified = false;
        exception->prev    = ఖాళీ;
        exception->తదుపరి    = io_runtime->exceptions;

        ఒకవేళ (exception->తదుపరి) {
            exception->తదుపరి->prev = exception;
        }

        io_runtime->exceptions = exception;
    }

    తిరిగి exception;
}


// Releases one exception ahead of the rest. The caller has to know nothing can
// still name it: no exnref was ever taken of it, and its payload has already
// been copied out.
శూన్య FreeException (IM3Runtime io_runtime, M3Exception* i_exception)
{
    ఒకవేళ (i_exception->prev) {
        i_exception->prev->తదుపరి = i_exception->తదుపరి;
    } లేకపోతే {
        io_runtime->exceptions = i_exception->తదుపరి;
    }

    ఒకవేళ (i_exception->తదుపరి) {
        i_exception->తదుపరి->prev = i_exception->prev;
    }

    ఒకవేళ (io_runtime->pendingException == i_exception) {
        io_runtime->pendingException = ఖాళీ;
    }

    m3_Free_Impl(i_exception);
}


// Releases every exception the runtime still holds. Only safe once the Wasm
// stack is empty, which is why the outermost RunCodeChecked() is the one that
// calls it.
శూన్య FreeExceptions (IM3Runtime io_runtime)
{
    M3Exception* exception = io_runtime->exceptions;

    io_runtime->exceptions       = ఖాళీ;
    io_runtime->pendingException = ఖాళీ;

    అయితే (exception) {
        M3Exception* తదుపరి = exception->తదుపరి;
        m3_Free_Impl(exception);
        exception = తదుపరి;
    }
}


// Keeps a copy of the exception a call just ended with, for m3_GetExceptionTag,
// or forgets the last one when the call ended any other way. A copy, because
// the original is freed with the rest before the outermost call returns - and
// one a host function's nested call ended with may still be named by an
// exnref further down the stack.
స్థిర
శూన్య రికార్డు_Uncaught_Exception (IM3Runtime io_runtime, M3_ఫలితం i_ఫలితం)
{
    ఒకవేళ (io_runtime->uncaughtException) {
        m3_Free(io_runtime->uncaughtException);
    }

    M3Exception* exception = io_runtime->pendingException;

    ఒకవేళ (M3_UNLIKELY(i_ఫలితం == m3Err_trapUncaughtException) and exception) {
        M3Exception* copy = (M3Exception*)m3_CopyMem(exception, పరిమాణం(M3Exception) + exception->అంకె_వాదనలు * పరిమాణం(u64));

        ఒకవేళ (copy) {
            copy->తదుపరి = ఖాళీ;
            copy->prev = ఖాళీ;
        }

        io_runtime->uncaughtException = copy;
    }
}

#అయితే_ముగింపు // d_m3HasExceptionHandling


#ఒకవేళ d_m3HasExceptionHandling || d_m3_Has_దొంతర_Switching

IM3_ట్యాగ్ m3_కనుగొను_ట్యాగ్ (IM3Module i_module, స్థిరరాశి అక్షరం* స్థిరరాశి i_ట్యాగ్_Name)
{
    ఒకవేళ (not i_module or not i_ట్యాగ్_Name) {
        తిరిగి ఖాళీ;
    }

    తిరిగి Module_కనుగొను_Exported_ట్యాగ్(i_module, i_ట్యాగ్_Name);
}


M3_ఫలితం m3_New_ట్యాగ్ (IM3Runtime io_runtime, IM3_ట్యాగ్* o_ట్యాగ్, స్థిరరాశి అక్షరం* స్థిరరాశి i_signature)
{
    IM3FuncType type    = ఖాళీ;
    M3_Host_ట్యాగ్*  host_ట్యాగ్ = ఖాళీ;

_try {
    _throwif("null tag", not o_ట్యాగ్);
    *o_ట్యాగ్ = ఖాళీ;
    _throwif(m3Err_mallocFailed, not io_runtime);

_   (Signature_గమ్యం_Func_Type(&type, i_signature));
    // tags are the same type exactly when their types are the same object
_   (Environment_కూడు_Func_Type(io_runtime->environment, &type));

    host_ట్యాగ్ = m3_AllocStruct(M3_Host_ట్యాగ్);
    _throwifnull(host_ట్యాగ్);

    host_ట్యాగ్->ట్యాగ్.type    = type;
    host_ట్యాగ్->తదుపరి        = io_runtime->host_ట్యాగులు;
    io_runtime->host_ట్యాగులు = host_ట్యాగ్;

    *o_ట్యాగ్ = &host_ట్యాగ్->ట్యాగ్;
} _catch:
    తిరిగి ఫలితం;
}


M3_ఫలితం m3_Link_ట్యాగ్ (IM3Module         io_module,
                     స్థిరరాశి అక్షరం* స్థిరరాశి i_moduleName,
                     స్థిరరాశి అక్షరం* స్థిరరాశి i_ట్యాగ్_Name,
                     IM3_ట్యాగ్            i_ట్యాగ్)
{
    ఒకవేళ (not io_module or not i_moduleName or not i_ట్యాగ్_Name or not i_ట్యాగ్) {
        తిరిగి m3_Err_ట్యాగ్_Lookup_Failed;
    }

    స్థిరరాశి bool wildcardModule = (strcmp(i_moduleName, "*") == 0);
    IM3_ట్యాగ్     లక్ష్యం         = i_ట్యాగ్->resolved ? i_ట్యాగ్->resolved : i_ట్యాగ్;

    M3_ఫలితం ఫలితం = m3_Err_ట్యాగ్_Lookup_Failed;

    కొరకు (u32 i = 0; i < io_module->అంకె_ట్యాగులు; ++i) {
        IM3_ట్యాగ్ ట్యాగ్ = &io_module->ట్యాగులు[i];

        ఒకవేళ (ట్యాగ్->imported and ట్యాగ్->import.moduleUtf8 and ట్యాగ్->import.క్షేత్రం_Utf8 and
            strcmp(ట్యాగ్->import.క్షేత్రం_Utf8, i_ట్యాగ్_Name) == 0 and
            (wildcardModule or strcmp(ట్యాగ్->import.moduleUtf8, i_moduleName) == 0)) {
            ఒకవేళ (ట్యాగ్->type != లక్ష్యం->type) {
                తిరిగి m3Err_incompatibleImportType;
            }

            ట్యాగ్->resolved = లక్ష్యం;
            ఫలితం        = m3Err_none;
        }
    }

    తిరిగి ఫలితం;
}


uint32_t m3_Get_ట్యాగ్_Arg_లెక్క (IM3_ట్యాగ్ i_ట్యాగ్)
{
    తిరిగి i_ట్యాగ్ ? i_ట్యాగ్->type->అంకె_వాదనలు : 0;
}


M3_విలువ_Type m3_Get_ట్యాగ్_Arg_Type (IM3_ట్యాగ్ i_ట్యాగ్, uint32_t i_సూచీ)
{
    ఒకవేళ (i_ట్యాగ్ and i_సూచీ < i_ట్యాగ్->type->అంకె_వాదనలు) {
        తిరిగి (M3_విలువ_Type)పునాది_Type_యొక్క(d_FuncArgType(i_ట్యాగ్->type, i_సూచీ));
    }

    తిరిగి c_m3Type_none;
}

#లేకపోతే

IM3_ట్యాగ్ m3_కనుగొను_ట్యాగ్ (IM3Module i_module, స్థిరరాశి అక్షరం* స్థిరరాశి i_ట్యాగ్_Name)
{
    తిరిగి ఖాళీ;
}

M3_ఫలితం m3_New_ట్యాగ్ (IM3Runtime io_runtime, IM3_ట్యాగ్* o_ట్యాగ్, స్థిరరాశి అక్షరం* స్థిరరాశి i_signature)
{
    ఒకవేళ (o_ట్యాగ్) {
        *o_ట్యాగ్ = ఖాళీ;
    }
    తిరిగి m3_Err_ట్యాగ్_Lookup_Failed;
}

M3_ఫలితం m3_Link_ట్యాగ్ (IM3Module io_module, స్థిరరాశి అక్షరం* స్థిరరాశి i_moduleName, స్థిరరాశి అక్షరం* స్థిరరాశి i_ట్యాగ్_Name, IM3_ట్యాగ్ i_ట్యాగ్)
{
    తిరిగి m3_Err_ట్యాగ్_Lookup_Failed;
}

uint32_t m3_Get_ట్యాగ్_Arg_లెక్క (IM3_ట్యాగ్ i_ట్యాగ్)
{
    తిరిగి 0;
}

M3_విలువ_Type m3_Get_ట్యాగ్_Arg_Type (IM3_ట్యాగ్ i_ట్యాగ్, uint32_t i_సూచీ)
{
    తిరిగి c_m3Type_none;
}

#అయితే_ముగింపు // d_m3HasExceptionHandling || d_m3HasStackSwitching


#ఒకవేళ d_m3HasExceptionHandling

M3_ఫలితం m3_ThrowException (IM3Runtime io_runtime, IM3_ట్యాగ్ i_ట్యాగ్, uint32_t i_argc, స్థిరరాశి శూన్య* i_argptrs[])
{
    ఒకవేళ (not io_runtime or not i_ట్యాగ్) {
        తిరిగి m3_Err_ట్యాగ్_Lookup_Failed;
    }

    IM3_ట్యాగ్      ట్యాగ్  = i_ట్యాగ్->resolved ? i_ట్యాగ్->resolved : i_ట్యాగ్;
    IM3FuncType type = ట్యాగ్->type;

    // a tag with results is a stack switching control tag, which is suspended
    // with rather than thrown
    ఒకవేళ (type->అంకె_Rets) {
        తిరిగి m3Err_typeMismatch;
    }
    ఒకవేళ (i_argc != type->అంకె_వాదనలు) {
        తిరిగి m3_Err_వాదం_లెక్క_Mismatch;
    }
    // with nothing on the stack there is nothing to unwind, and the exception
    // would sit on the runtime's list until some later call ended
    ఒకవేళ (io_runtime->callNesting == 0) {
        తిరిగి "m3_ThrowException called outside a host function";
    }

    M3Exception* exception = NewException(io_runtime, ట్యాగ్, i_argc);
    ఒకవేళ (not exception) {
        తిరిగి m3Err_mallocFailed;
    }

    // the layout op_Throw gives a payload: 32-bit values zero-extended, floats
    // as their bit pattern
    కొరకు (u32 i = 0; i < i_argc; ++i) {
        స్థిరరాశి శూన్య* విలువ = i_argptrs[i];

        ఎంపిక (పునాది_Type_యొక్క(d_FuncArgType(type, i))) {
        స్థితి c_m3Type_i32:
        స్థితి c_m3Type_f32: {
            u32 బిట్లు;
            జ్ఞాపకం_నకలు(&బిట్లు, విలువ, పరిమాణం(బిట్లు));
            exception->వాదనలు[i] = బిట్లు;
            ఆపు;
        }
        స్థితి c_m3Type_i64:
        స్థితి c_m3Type_f64:
            జ్ఞాపకం_నకలు(&exception->వాదనలు[i], విలువ, పరిమాణం(u64));
            ఆపు;
        స్థితి c_m3Type_funcref:
        స్థితి c_m3Type_externref:
        స్థితి c_m3Type_exnref:
        స్థితి c_m3Type_contref:
            exception->వాదనలు[i] = (u64)(*(స్థిరరాశి uintptr_t*)విలువ);
            ఆపు;
        అప్రమేయ:
            FreeException(io_runtime, exception);
            తిరిగి "unknown argument type";
        }
    }

    io_runtime->pendingException = exception;

    తిరిగి m3Err_pendingException;
}


IM3_ట్యాగ్ m3_Get_Exception_ట్యాగ్ (IM3Runtime i_runtime)
{
    ఒకవేళ (i_runtime and i_runtime->uncaughtException) {
        తిరిగి i_runtime->uncaughtException->ట్యాగ్;
    }

    తిరిగి ఖాళీ;
}


M3_ఫలితం m3_Get_Exception_వాదనలు (IM3Runtime i_runtime, uint32_t i_argc, స్థిరరాశి శూన్య* o_argptrs[])
{
    ఒకవేళ (not i_runtime or not i_runtime->uncaughtException) {
        తిరిగి "no uncaught exception";
    }

    స్థిరరాశి M3Exception* exception = i_runtime->uncaughtException;
    IM3FuncType        type      = exception->ట్యాగ్->type;

    ఒకవేళ (i_argc != exception->అంకె_వాదనలు) {
        తిరిగి m3_Err_వాదం_లెక్క_Mismatch;
    }

    కొరకు (u32 i = 0; i < i_argc; ++i) {
        శూన్య* విలువ = (శూన్య*)o_argptrs[i];
        u64   బిట్లు  = exception->వాదనలు[i];

        ఎంపిక (పునాది_Type_యొక్క(d_FuncArgType(type, i))) {
        స్థితి c_m3Type_i32:
        స్థితి c_m3Type_f32: {
            u32 దిగువ = (u32)బిట్లు;
            జ్ఞాపకం_నకలు(విలువ, &దిగువ, పరిమాణం(దిగువ));
            ఆపు;
        }
        స్థితి c_m3Type_i64:
        స్థితి c_m3Type_f64:
            జ్ఞాపకం_నకలు(విలువ, &బిట్లు, పరిమాణం(బిట్లు));
            ఆపు;
        స్థితి c_m3Type_funcref:
        స్థితి c_m3Type_externref:
        స్థితి c_m3Type_exnref:
        స్థితి c_m3Type_contref:
            *(uintptr_t*)విలువ = (uintptr_t)బిట్లు;
            ఆపు;
        అప్రమేయ:
            తిరిగి "unknown argument type";
        }
    }

    తిరిగి m3Err_none;
}

#లేకపోతే

M3_ఫలితం m3_ThrowException (IM3Runtime io_runtime, IM3_ట్యాగ్ i_ట్యాగ్, uint32_t i_argc, స్థిరరాశి శూన్య* i_argptrs[])
{
    తిరిగి m3_Err_ట్యాగ్_Lookup_Failed;
}

IM3_ట్యాగ్ m3_Get_Exception_ట్యాగ్ (IM3Runtime i_runtime)
{
    తిరిగి ఖాళీ;
}

M3_ఫలితం m3_Get_Exception_వాదనలు (IM3Runtime i_runtime, uint32_t i_argc, స్థిరరాశి శూన్య* o_argptrs[])
{
    తిరిగి "no uncaught exception";
}

#అయితే_ముగింపు // d_m3HasExceptionHandling


#ఒకవేళ d_m3_Guarded_జ్ఞాపకం

// What RunCodeProtected hands to the platform layer to run, and what comes back
రకనిర్వచనం నిర్మాణం M3_Protected_నిరంతరం {
    pc_t            pc;
    IM3Runtime      runtime;
    M3_జ్ఞాపకం_శీర్షిక* mem;
    M3_ఫలితం        ఫలితం;
} M3_Protected_నిరంతరం;

స్థిర
శూన్య నిరంతరం_సంకేతం_Body (శూన్య* io_context)
{
    M3_Protected_నిరంతరం* నిరంతరం = (M3_Protected_నిరంతరం*)io_context;

#  ఒకవేళ (d_m3EnableOpProfiling || d_m3EnableOpTracing)
    నిరంతరం->ఫలితం = (M3_ఫలితం)నిరంతరం_సంకేతం(నిరంతరం->pc, (m3stack_t)నిరంతరం->runtime->దొంతర, నిరంతరం->mem,
                                    d_m3_Op_Default_వాదనలు, d_m3_పునాది_Cstr);
#  లేకపోతే
    నిరంతరం->ఫలితం = (M3_ఫలితం)నిరంతరం_సంకేతం(నిరంతరం->pc, (m3stack_t)నిరంతరం->runtime->దొంతర, నిరంతరం->mem,
                                    d_m3_Op_Default_వాదనలు);
#  endif
}

// The bounds check the load and store operations no longer do, done once here by the
// system instead: an address past the end of a memory reaches the uncommitted part
// of that memory's reservation, and the fault comes back as the trap the spec asks
// for. Nothing is left half-done by it - the Wasm stack and the memories are heap,
// and the caller unwinds the rest.
స్థిర
M3_ఫలితం నిరంతరం_సంకేతం_Protected (IM3Runtime i_runtime, pc_t i_pc, M3_జ్ఞాపకం_శీర్షిక* i_mem)
{
    // Memory operations in this build carry no bounds check, so code only runs where a
    // fault past the end of a memory can still be turned into a trap
    ఒకవేళ (M3_UNLIKELY(not m3_Host_Guards_చురుకైన())) {
        తిరిగి ErrorRuntime(m3_Err_జ్ఞాపకం_Guards_Unavailable, i_runtime,
                            "another part of the process holds the fault signal");
    }

    M3_Protected_నిరంతరం నిరంతరం;
    నిరంతరం.pc      = i_pc;
    నిరంతరం.runtime = i_runtime;
    నిరంతరం.mem     = i_mem;
    నిరంతరం.ఫలితం  = m3Err_none;

    శూన్య*  arena;
    size_t arena_బైట్లు;
    Guard_ArenaRange(&arena, &arena_బైట్లు);

    ఒకవేళ (not m3_HostProtectedCall(నిరంతరం_సంకేతం_Body, &నిరంతరం, arena, arena_బైట్లు)) {
        తిరిగి m3_Err_trap_బహిర్గతం_యొక్క_Bounds_జ్ఞాపకం_Access;
    }

    తిరిగి నిరంతరం.ఫలితం;
}

#అయితే_ముగింపు // d_m3GuardedMemory


#ఒకవేళ d_m3_Has_దొంతర_Switching

// The function a suspended continuation stopped in: the callee of its innermost
// call frame, or the function it was entered with if it made no call. Frames are
// recorded innermost first, so that is the lowest-numbered call frame.
స్థిర
IM3Function Continuation_InnermostFunction (IM3Continuation i_cont)
{
    కొరకు (u32 i = 0; i < i_cont->అంకె_చట్రాలు; ++i) {
        ఒకవేళ (i_cont->చట్రాలు[i].రకం == చట్రం_call && i_cont->చట్రాలు[i].call.function) {
            తిరిగి i_cont->చట్రాలు[i].call.function;
        }
    }

    తిరిగి i_cont->నమోదు_Function;
}


// Where the host's next call puts its arguments and finds its results. A
// suspended root keeps its frames on the bottom of the runtime's stack, so a call
// made before it is resumed starts above the last of them rather than on top of
// them. The top is the innermost function's frame base plus everything that
// function can use. That base is the root's own sp, unless it stopped inside a
// resume: then the suspension point belongs to the resumed continuation, and the
// root's innermost frame is that resume.
శూన్య Runtime_ఉంచు_Call_దొంతర (IM3Runtime io_runtime)
{
    IM3Continuation మూలం = io_runtime->మూలం_Continuation;

    io_runtime->దొంతర = io_runtime->origin_దొంతర;

    ఒకవేళ (not మూలం or మూలం->state != cont_suspended) {
        తిరిగి;
    }

    m3stack_t   sp       = మూలం->sp;
    IM3Function function = Continuation_InnermostFunction(మూలం);

    ఒకవేళ (మూలం->అంకె_చట్రాలు and మూలం->చట్రాలు[0].రకం == చట్రం_resume) {
        sp = మూలం->చట్రాలు[0].sp;
    }

    m3slot_t* పునాది = (m3slot_t*)io_runtime->origin_దొంతర;
    m3slot_t* శిఖరం  = (m3slot_t*)sp + (function ? function->max_దొంతర_Slots : 0);

    ఒకవేళ (శిఖరం > పునాది + io_runtime->అంకె_దొంతర_Slots) {
        శిఖరం = పునాది + io_runtime->అంకె_దొంతర_Slots;
    }

    io_runtime->దొంతర = శిఖరం;
}

#అయితే_ముగింపు // d_m3HasStackSwitching


// Run compiled code on the runtime's stack, bounding native recursion for the
// duration of the call. The outermost invocation establishes the stack limit;
// nested ones (an imported function calling back into Wasm) inherit it.
స్థిర అంతఃవరుస
M3_ఫలితం నిరంతరం_సంకేతం_Checked (IM3Runtime i_runtime, IM3Function i_function)
{
    pc_t i_pc = i_function->compiled;

    // execution runs against the memory of the module the entry point belongs
    // to, not against some runtime-wide one
    M3_జ్ఞాపకం_శీర్షిక* _mem = Module_జ్ఞాపకం_శీర్షిక(i_function->module);

#ఒకవేళ d_m3HasSnapshots
    ఒకవేళ (M3_UNLIKELY(i_function->module->isUnusable)) {
        తిరిగి "a snapshot failed to restore into the module, so it cannot run";
    }
    i_function->module->has_నిరంతరం = true;
#అయితే_ముగింపు

    d_m3_దొంతర_హద్దు_Enter(i_runtime);

    // a host function calling back into Wasm does not change which call a trap
    // ends: that is still the one the host made first
    ఒకవేళ (i_runtime->callNesting++ == 0) {
        i_runtime->entered = i_function;
    }
#ఒకవేళ d_m3HasExceptionHandling
    // handler stacks don't nest across a call boundary: a host function calling
    // back into Wasm cannot be caught by a try_table its own caller entered
    u32 saved_Try_లోతు   = i_runtime->try_లోతు;
    i_runtime->try_లోతు = 0;
#అయితే_ముగింపు
#ఒకవేళ d_m3_Has_దొంతర_Switching
    // nor do continuations: the native frames on the far side of a host call
    // are not ours to record, so a suspend in here has no handler to reach and
    // no resume to unwind to
    IM3Continuation savedContinuation = i_runtime->చురుకైన_Continuation;

    // A suspended root continuation is a paused program waiting to be resumed,
    // not scratch space. A call the host makes before resuming it gets a
    // context of its own, the way any other re-entry does, rather than having
    // the frames of the suspension reset out from under it.
    bool claim_మూలం = i_runtime->isSuspendable and not savedContinuation and
                     not (i_runtime->మూలం_Continuation and
                          i_runtime->మూలం_Continuation->state == cont_suspended);

    ఒకవేళ (claim_మూలం and not i_runtime->మూలం_Continuation) {
        i_runtime->మూలం_Continuation = Continuation_New(i_runtime, i_function->funcType, i_function);

        ఒకవేళ (i_runtime->మూలం_Continuation) {
            // The root borrows the runtime's stack and is not charged as cont.new.
            i_runtime->మూలం_Continuation->అంకె_దొంతర_Slots = i_runtime->అంకె_దొంతర_Slots;
        } లేకపోతే {
            claim_మూలం = false;
        }
    }

    ఒకవేళ (claim_మూలం) {
        i_runtime->చురుకైన_Continuation              = i_runtime->మూలం_Continuation;
        i_runtime->మూలం_Continuation->నమోదు_Function = i_function;
        i_runtime->మూలం_Continuation->state         = cont_running;
        i_runtime->మూలం_Continuation->అంకె_చట్రాలు     = 0;
    } లేకపోతే {
        i_runtime->చురుకైన_Continuation = ఖాళీ;
    }
#అయితే_ముగింపు
#ఒకవేళ d_m3_Guarded_జ్ఞాపకం
    M3_ఫలితం ఫలితం = నిరంతరం_సంకేతం_Protected(i_runtime, i_pc, _mem);
#లేదావేళ (d_m3EnableOpProfiling || d_m3EnableOpTracing)
    M3_ఫలితం ఫలితం = (M3_ఫలితం)నిరంతరం_సంకేతం(i_pc, (m3stack_t)i_runtime->దొంతర, _mem, d_m3_Op_Default_వాదనలు, d_m3_పునాది_Cstr);
#లేకపోతే
    M3_ఫలితం ఫలితం = (M3_ఫలితం)నిరంతరం_సంకేతం(i_pc, (m3stack_t)i_runtime->దొంతర, _mem, d_m3_Op_Default_వాదనలు);
#అయితే_ముగింపు
#ఒకవేళ d_m3_Has_దొంతర_Switching
    ఒకవేళ (claim_మూలం) {
        i_runtime->మూలం_Continuation->state =
          (ఫలితం == m3Err_continuationSuspended) ? cont_suspended : cont_returned;

        Runtime_ఉంచు_Call_దొంతర(i_runtime);
    }
    i_runtime->చురుకైన_Continuation = savedContinuation;
#అయితే_ముగింపు
#ఒకవేళ d_m3HasExceptionHandling
    i_runtime->try_లోతు = saved_Try_లోతు;

    // an exception that reached the bottom of the call stack found no handler
    ఒకవేళ (M3_UNLIKELY(ఫలితం == m3Err_pendingException)) {
        ఫలితం = m3Err_trapUncaughtException;
    }
    రికార్డు_Uncaught_Exception(i_runtime, ఫలితం);

    // a paused invocation can still reach the exceptions it holds, so they wait
    // for it to finish
    ఒకవేళ (i_runtime->callNesting == 1 and not m3_IsSuspended(i_runtime)) {
        FreeExceptions(i_runtime);
    }
#అయితే_ముగింపు
    i_runtime->callNesting--;
    d_m3_దొంతర_హద్దు_Leave(i_runtime);

    తిరిగి ఫలితం;
}

M3_ఫలితం m3_నిరంతరం_ప్రారంభం (IM3Module io_module)
{
#నిర్వచితమైతే FUZZING_BUILD_బహుళకం_UNSAFE_FOR_PRODUCTION
    // Execution disabled for fuzzing builds
    తిరిగి m3Err_none;
#అయితే_ముగింపు

    M3_ఫలితం ఫలితం           = m3Err_none;
    i32      ప్రారంభం_Function_Tmp = -1;

    ఒకవేళ (io_module and io_module->ప్రారంభం_Function >= 0) {
        IM3Function function = &io_module->functions[io_module->ప్రారంభం_Function];

        ఒకవేళ (not function->compiled) {
_           (CompileFunction(function));
        }

        IM3FuncType ftype = function->funcType;
        ఒకవేళ (ftype->అంకె_వాదనలు != 0 || ftype->అంకె_Rets != 0) {
            _throw(m3_Err_వాదం_లెక్క_Mismatch);
        }

        IM3Module  module  = function->module;
        IM3Runtime runtime = module->runtime;

        ప్రారంభం_Function_Tmp = io_module->ప్రారంభం_Function;

        io_module->ప్రారంభం_Function = -1;

        ఫలితం = నిరంతరం_సంకేతం_Checked(runtime, function);

        // a start function that suspended is not to be started again: resuming
        // is what finishes it
        ఒకవేళ (ఫలితం and ఫలితం != m3Err_continuationSuspended) {
            io_module->ప్రారంభం_Function = ప్రారంభం_Function_Tmp;
            EXCEPTION_PRINT(ఫలితం);
            వెళ్ళు _catch;
        }
    }

    _catch: తిరిగి ఫలితం;
}

// TODO: deal with main + side-modules loading efforcement
M3_ఫలితం m3_ఎక్కించు_Module (IM3Runtime io_runtime, IM3Module io_module)
{
    M3_ఫలితం ఫలితం = m3Err_none;

    ఒకవేళ (M3_UNLIKELY(io_module->runtime)) {
        తిరిగి m3Err_moduleAlreadyLinked;
    }

    io_module->runtime = io_runtime;

    // linking first: a memory import has to be resolved before InitMemory would
    // give it pages of its own, and a global import before an initializer reads it
_   (LinkImports(io_runtime, io_module));

_   (Init_జ్ఞాపకం(io_runtime, io_module));
_   (InitGlobals(io_module));
    // Spec order: element segments are applied before data segments. It matters
    // when one of them traps - whatever ran before the trap stays done.
_   (Init_పట్టిక_And_Elements(io_module));
_   (Init_దత్తాంశం_Segments(io_module));

    // Start func might use imported functions, which are not liked here yet,
    // so it will be called before a function call is attempted (in m3_FindFunction)

#నిర్వచితమైతే DEBUG
    Module_Generate_పేర్లు(io_module);
#అయితే_ముగింపు

    io_module->తదుపరి     = io_runtime->modules;
    io_runtime->modules = io_module;
    తిరిగి ఫలితం; // ok

_catch:
    // The runtime owns the module either way. Instantiation may already have
    // written this module's functions into a table another module owns, and the
    // spec keeps whatever it managed to do before the trap, so those entries
    // stay callable - retaining the module is what stops them dangling.
    //
    // It goes on the tail of the list rather than the head: it is not a module
    // anyone should find by name, only one whose functions may still be
    // reachable through someone else's table.
    io_module->తదుపరి = ఖాళీ;

    IM3Module* తోక = &io_runtime->modules;
    అయితే (*తోక) {
        తోక = &(*తోక)->తదుపరి;
    }
    *తోక = io_module;

    తిరిగి ఫలితం;
}

IM3Global m3_కనుగొను_Global (IM3Module         io_module,
                         స్థిరరాశి అక్షరం* స్థిరరాశి i_globalName)
{
    // Search exports
    IM3Global exported = Module_కనుగొను_Exported_Global(io_module, i_globalName);
    ఒకవేళ (exported) {
        // a re-exported global is the one that was imported, so reads and
        // writes have to reach the cell that actually holds the value
        తిరిగి exported->resolved ? exported->resolved : exported;
    }

    // Search imports
    కొరకు (u32 i = 0; i < io_module->అంకె_Globals; ++i) {
        IM3Global g = &io_module->globals[i];

        ఒకవేళ (g->import.moduleUtf8 and g->import.క్షేత్రం_Utf8) {
            ఒకవేళ (strcmp(g->import.క్షేత్రం_Utf8, i_globalName) == 0) {
                తిరిగి g->resolved ? g->resolved : g;
            }
        }
    }
    తిరిగి ఖాళీ;
}

M3_ఫలితం m3_GetGlobal (IM3Global      i_global,
                       IM3_Tagged_విలువ o_విలువ)
{
    ఒకవేళ (not i_global) {
        తిరిగి m3Err_globalLookupFailed;
    }

    ఎంపిక (i_global->type) {
    స్థితి c_m3Type_i32: o_విలువ->విలువ.i32 = i_global->i32_విలువ; ఆపు;
    స్థితి c_m3Type_i64: o_విలువ->విలువ.i64 = i_global->i64_విలువ; ఆపు;
#ఒకవేళ d_m3HasFloat
    స్థితి c_m3Type_f32: o_విలువ->విలువ.f32 = i_global->f32_విలువ; ఆపు;
    స్థితి c_m3Type_f64: o_విలువ->విలువ.f64 = i_global->f64_విలువ; ఆపు;
#అయితే_ముగింపు
    అప్రమేయ: తిరిగి m3Err_invalidTypeId;
    }

    o_విలువ->type = (M3_విలువ_Type)(i_global->type);
    తిరిగి m3Err_none;
}

M3_ఫలితం m3_సమితి_Global (IM3Global            i_global,
                       స్థిరరాశి IM3_Tagged_విలువ i_విలువ)
{
    ఒకవేళ (not i_global) {
        తిరిగి m3Err_globalLookupFailed;
    }
    ఒకవేళ (not i_global->isMutable) {
        తిరిగి m3Err_globalNotMutable;
    }
    ఒకవేళ (i_global->type != i_విలువ->type) {
        తిరిగి m3Err_globalTypeMismatch;
    }

    ఎంపిక (i_విలువ->type) {
    స్థితి c_m3Type_i32: i_global->i32_విలువ = i_విలువ->విలువ.i32; ఆపు;
    స్థితి c_m3Type_i64: i_global->i64_విలువ = i_విలువ->విలువ.i64; ఆపు;
#ఒకవేళ d_m3HasFloat
    స్థితి c_m3Type_f32: i_global->f32_విలువ = i_విలువ->విలువ.f32; ఆపు;
    స్థితి c_m3Type_f64: i_global->f64_విలువ = i_విలువ->విలువ.f64; ఆపు;
#అయితే_ముగింపు
    అప్రమేయ: తిరిగి m3Err_invalidTypeId;
    }

    తిరిగి m3Err_none;
}

M3_విలువ_Type m3_GetGlobalType (IM3Global i_global)
{
    తిరిగి (i_global) ? (M3_విలువ_Type)(i_global->type) : c_m3Type_none;
}


శూన్య* v_కనుగొను_Function (IM3Module i_module, శూన్య* i_info)
{
    స్థిరరాశి అక్షరం* స్థిరరాశి i_name = (స్థిరరాశి అక్షరం*)i_info;

    // Prefer exported functions
    IM3Function exported = Module_కనుగొను_Exported_Function(i_module, i_name);
    ఒకవేళ (exported) {
        తిరిగి exported;
    }

    // Search internal functions
    కొరకు (u32 i = 0; i < i_module->అంకె_Functions; ++i) {
        IM3Function f = &i_module->functions[i];

        bool isImported = f->import.moduleUtf8 or f->import.క్షేత్రం_Utf8;

        ఒకవేళ (isImported) {
            కొనసాగు;
        }

        కొరకు (పూర్ణాంకం j = 0; j < f->అంకె_పేర్లు; j++) {
            ఒకవేళ (f->పేర్లు[j] and strcmp(f->పేర్లు[j], i_name) == 0) {
                తిరిగి f;
            }
        }
    }

    తిరిగి ఖాళీ;
}


// Shared tail of the two lookups: a function is only usable once it has code.
స్థిర
M3_ఫలితం Prepare_దొరికింది_Function (IM3Function* o_function, IM3Function i_function)
{
    M3_ఫలితం ఫలితం = m3Err_none;

    ఒకవేళ (not i_function->compiled) {
_       (CompileFunction(i_function))
    }

    _catch:
    *o_function = ఫలితం ? ఖాళీ : i_function;

    తిరిగి ఫలితం;
}


// Searches every module in the runtime, most recently loaded first. That is a
// guess once more than one module is loaded and two of them export the same
// name - m3_FindFunctionIn says which module is meant.
M3_ఫలితం m3_కనుగొను_Function (IM3Function* o_function, IM3Runtime i_runtime, స్థిరరాశి అక్షరం* స్థిరరాశి i_functionName)
{
                                                                d_m3Assert (o_function and i_runtime and i_functionName);
    IM3Function function = ఖాళీ;

    ఒకవేళ (not i_runtime->modules) {
        *o_function = ఖాళీ;
        తిరిగి "no modules loaded";
    }

    function = (IM3Function)ForEachModule(i_runtime, v_కనుగొను_Function, (శూన్య*)i_functionName);

    ఒకవేళ (not function) {
        *o_function = ఖాళీ;
        తిరిగి ErrorModule(m3Err_functionLookupFailed, i_runtime->modules, "'%s'", i_functionName);
    }

    తిరిగి Prepare_దొరికింది_Function(o_function, function);
}


IM3Module m3_కనుగొను_Module (IM3Runtime i_runtime, స్థిరరాశి అక్షరం* స్థిరరాశి i_moduleName)
{
    ఒకవేళ (not i_runtime or not i_moduleName) {
        తిరిగి ఖాళీ;
    }

    // the list is newest-first, so a name registered twice names the newer one
    కొరకు (IM3Module m = i_runtime->modules; m; m = m->తదుపరి) {
        ఒకవేళ (m->name and strcmp(m->name, i_moduleName) == 0) {
            తిరిగి m;
        }
    }

    తిరిగి ఖాళీ;
}


// Searches one module's exports, which is what naming a module means.
M3_ఫలితం m3_కనుగొను_Function_లోపల (IM3Function* o_function, IM3Module i_module, స్థిరరాశి అక్షరం* స్థిరరాశి i_functionName)
{
                                                                d_m3Assert (o_function and i_functionName);
    ఒకవేళ (not i_module) {
        *o_function = ఖాళీ;
        తిరిగి m3Err_functionLookupFailed;      // ErrorModule would deref it
    }

    IM3Function function = (IM3Function)v_కనుగొను_Function(i_module, (శూన్య*)i_functionName);

    ఒకవేళ (not function) {
        *o_function = ఖాళీ;
        తిరిగి ErrorModule(m3Err_functionLookupFailed, i_module, "'%s'", i_functionName);
    }

    తిరిగి Prepare_దొరికింది_Function(o_function, function);
}


M3_ఫలితం m3_Get_పట్టిక_Function (IM3Function* o_function, IM3Module i_module, uint32_t i_సూచీ)
{
_try {
    M3_పట్టిక*    పట్టిక_2;
    IM3Function function;

    _throwif("no table", i_module->అంకె_Tables == 0);

    పట్టిక_2 = i_module->tables[0];
    _throwif("function index out of range", i_సూచీ >= పట్టిక_2->కొలత);

    function = (IM3Function)పట్టిక_2->elements[i_సూచీ];

    ఒకవేళ (function) {
        ఒకవేళ (not function->compiled) {
_           (CompileFunction(function))
        }
    }

    *o_function = function;
} _catch:
    తిరిగి ఫలితం;
}


స్థిర
M3_ఫలితం check_ప్రారంభం_Function (IM3Module i_module)
{
    M3_ఫలితం ఫలితం = m3Err_none;                               d_m3Assert(i_module);

    // Check if start function needs to be called
    ఒకవేళ (i_module->ప్రారంభం_Function >= 0) {
        ఫలితం = m3_నిరంతరం_ప్రారంభం(i_module);
    }

    తిరిగి ఫలితం;
}

uint32_t m3_Get_Arg_లెక్క (IM3Function i_function)
{
    ఒకవేళ (i_function) {
        IM3FuncType ft = i_function->funcType;
        ఒకవేళ (ft) {
            తిరిగి ft->అంకె_వాదనలు;
        }
    }
    తిరిగి 0;
}

uint32_t m3_Get_Ret_లెక్క (IM3Function i_function)
{
    ఒకవేళ (i_function) {
        IM3FuncType ft = i_function->funcType;
        ఒకవేళ (ft) {
            తిరిగి ft->అంకె_Rets;
        }
    }
    తిరిగి 0;
}


M3_విలువ_Type m3_GetArgType (IM3Function i_function, uint32_t index)
{
    ఒకవేళ (i_function) {
        IM3FuncType ft = i_function->funcType;
        ఒకవేళ (ft and index < ft->అంకె_వాదనలు) {
            తిరిగి (M3_విలువ_Type)పునాది_Type_యొక్క(d_FuncArgType(ft, index));
        }
    }
    తిరిగి c_m3Type_none;
}

M3_విలువ_Type m3_GetRetType (IM3Function i_function, uint32_t index)
{
    ఒకవేళ (i_function) {
        IM3FuncType ft = i_function->funcType;
        ఒకవేళ (ft and index < ft->అంకె_Rets) {
            తిరిగి (M3_విలువ_Type)పునాది_Type_యొక్క(d_FuncRetType(ft, index));
        }
    }
    తిరిగి c_m3Type_none;
}


// The call APIs marshal their arguments into the runtime stack before any
// compiled code runs, so op_Entry's overflow check comes too late to cover those
// writes - and for a host function or an import linked to another module there
// is no op_Entry of our own to reach at all. Bound the area about to be written
// here instead: results and arguments each take a u64 at this boundary, however
// wide the slot is inside. Every caller goes through this, so none of them can
// be the one that forgets.
స్థిర
M3_ఫలితం Get_దొంతర_సూచిక_For_వాదనలు (IM3Function i_function, u8** o_దొంతర)
{
    IM3Runtime  runtime = i_function->module->runtime;
    IM3FuncType ftype   = i_function->funcType;

    // the stack a paused invocation holds is not available to this one
    size_t వాడిన      = (size_t)((u8*)runtime->దొంతర - (u8*)runtime->origin_దొంతర);
    size_t needed    = ((size_t)ftype->అంకె_Rets + ftype->అంకె_వాదనలు) * పరిమాణం(u64);
    size_t అందుబాటు = (size_t)runtime->అంకె_దొంతర_Slots * పరిమాణం(m3slot_t) - వాడిన;

    ఒకవేళ (needed > అందుబాటు) {
        తిరిగి m3_Err_trap_దొంతర_అధికప్రవాహం;
    }

    u64* దొంతర = (u64*)runtime->దొంతర;

    దొంతర += ftype->అంకె_Rets;

    *o_దొంతర = (u8*)దొంతర;

    తిరిగి m3Err_none;
}


M3_ఫలితం m3_CallV (IM3Function i_function, ...)
{
    va_list ap;
    va_start(ap, i_function);
    M3_ఫలితం r = m3_CallVL(i_function, ap);
    va_end(ap);
    తిరిగి r;
}

స్థిర
శూన్య Report_Native_దొంతర_Usage ()
{
#ఒకవేళ d_m3_Log_Native_దొంతర
    పూర్ణాంకం దొంతర_వాడిన = m3_దొంతర_Get_Max();
    fprintf(stderr, "Native stack used: %d\n", దొంతర_వాడిన);
#అయితే_ముగింపు
}


M3_ఫలితం m3_CallVL (IM3Function i_function, va_list i_వాదనలు)
{
    IM3Runtime  runtime = i_function->module->runtime;
    IM3FuncType ftype   = i_function->funcType;
    M3_ఫలితం    ఫలితం  = m3Err_none;
    u8*         s       = ఖాళీ;

    ఒకవేళ (!i_function->compiled) {
        తిరిగి m3_Err_missing_Compiled_సంకేతం;
    }

#ఒకవేళ d_m3_రికార్డు_Backtraces
    ClearBacktrace(runtime);
#అయితే_ముగింపు

    m3_దొంతర_Check_Init();

_   (check_ప్రారంభం_Function(i_function->module))

_   (Get_దొంతర_సూచిక_For_వాదనలు(i_function, &s))

    కొరకు (u32 i = 0; i < ftype->అంకె_వాదనలు; ++i) {
        ఎంపిక (d_FuncArgType(ftype, i)) {
        స్థితి c_m3Type_i32:
            *(i32*)(s) = va_arg(i_వాదనలు, i32);
            s += 8;
            ఆపు;
        స్థితి c_m3Type_i64:
            *(i64*)(s) = va_arg(i_వాదనలు, i64);
            s += 8;
            ఆపు;
        స్థితి c_m3Type_funcref:
        స్థితి c_m3Type_externref:
        స్థితి c_m3Type_exnref:
            *(uintptr_t*)(s) = va_arg(i_వాదనలు, uintptr_t);
            s += 8;
            ఆపు;
#ఒకవేళ d_m3HasFloat
        స్థితి c_m3Type_f32:
            *(f32*)(s) = (f32)va_arg(i_వాదనలు, f64);
            s += 8;
            ఆపు; // f32 is passed as f64
        స్థితి c_m3Type_f64:
            *(f64*)(s) = va_arg(i_వాదనలు, f64);
            s += 8;
            ఆపు;
#అయితే_ముగింపు
        అప్రమేయ: తిరిగి "unknown argument type";
        }
    }

    ఫలితం = నిరంతరం_సంకేతం_Checked(runtime, i_function);
    Report_Native_దొంతర_Usage();

    runtime->చివరి_Called = ఫలితం ? ఖాళీ : i_function;

    _catch: తిరిగి ఫలితం;
}

M3_ఫలితం m3_Call (IM3Function i_function, uint32_t i_argc, స్థిరరాశి శూన్య* i_argptrs[])
{
    IM3Runtime  runtime = i_function->module->runtime;
    IM3FuncType ftype   = i_function->funcType;
    M3_ఫలితం    ఫలితం  = m3Err_none;
    u8*         s       = ఖాళీ;

    ఒకవేళ (i_argc != ftype->అంకె_వాదనలు) {
        తిరిగి m3_Err_వాదం_లెక్క_Mismatch;
    }
    ఒకవేళ (!i_function->compiled) {
        తిరిగి m3_Err_missing_Compiled_సంకేతం;
    }

#ఒకవేళ d_m3_రికార్డు_Backtraces
    ClearBacktrace(runtime);
#అయితే_ముగింపు

    m3_దొంతర_Check_Init();

_   (check_ప్రారంభం_Function(i_function->module))

_   (Get_దొంతర_సూచిక_For_వాదనలు(i_function, &s))

    కొరకు (u32 i = 0; i < ftype->అంకె_వాదనలు; ++i) {
        ఎంపిక (d_FuncArgType(ftype, i)) {
        స్థితి c_m3Type_i32:
            *(i32*)(s) = *(i32*)i_argptrs[i];
            s += 8;
            ఆపు;
        స్థితి c_m3Type_i64:
            *(i64*)(s) = *(i64*)i_argptrs[i];
            s += 8;
            ఆపు;
        స్థితి c_m3Type_funcref:
        స్థితి c_m3Type_externref:
        స్థితి c_m3Type_exnref:
            *(uintptr_t*)(s) = *(uintptr_t*)i_argptrs[i];
            s += 8;
            ఆపు;
#ఒకవేళ d_m3HasFloat
        స్థితి c_m3Type_f32:
            *(f32*)(s) = *(f32*)i_argptrs[i];
            s += 8;
            ఆపు;
        స్థితి c_m3Type_f64:
            *(f64*)(s) = *(f64*)i_argptrs[i];
            s += 8;
            ఆపు;
#అయితే_ముగింపు
        అప్రమేయ: తిరిగి "unknown argument type";
        }
    }

    ఫలితం = నిరంతరం_సంకేతం_Checked(runtime, i_function);

    Report_Native_దొంతర_Usage();

    runtime->చివరి_Called = ఫలితం ? ఖాళీ : i_function;

    _catch: తిరిగి ఫలితం;
}

// Argument parsing for m3_CallArgv. Strict on purpose: the whole string has to
// be consumed, so "12abc" is rejected rather than read as 12, and an empty or
// unparsable argument is an error rather than the zero that strtoul with a
// NULL end pointer used to hand back. See wasm3/wasm3#367.
స్థిర
M3_ఫలితం ParseArgInteger (ccstr_t i_arg, u32 i_అంకె_బిట్లు, u64* o_విలువ)
{
    ఒకవేళ (not i_arg or not *i_arg) {
        తిరిగి "empty argument";
    }

    // strtoull would skip leading space, but trailing space is rejected below;
    // accepting one and not the other would just be confusing
    ఒకవేళ (ఖాళీనా((గుర్తులేని అక్షరం)*i_arg)) {
        తిరిగి "argument is not a number";
    }

    అక్షరం* చివర = ఖాళీ;
    u64   విలువ;

    errno = 0;

    // an argument may be spelled signed or unsigned: -1 and 4294967295 name the
    // same i32
    ఒకవేళ (*i_arg == '-') {
        i64 signed_విలువ = strtoll(i_arg, &చివర, 10);

        ఒకవేళ (i_అంకె_బిట్లు == 32 and (signed_విలువ < INT32_MIN or signed_విలువ > INT32_MAX)) {
            తిరిగి "argument out of range";
        }

        విలువ = (u64)signed_విలువ;
    } లేకపోతే {
        విలువ = strtoull(i_arg, &చివర, 10);

        ఒకవేళ (i_అంకె_బిట్లు == 32 and విలువ > UINT32_MAX) {
            తిరిగి "argument out of range";
        }
    }

    ఒకవేళ (errno == ERANGE) {
        తిరిగి "argument out of range";
    }

    ఒకవేళ (చివర == i_arg or *చివర) {
        తిరిగి "argument is not a number";
    }

    *o_విలువ = విలువ;

    తిరిగి m3Err_none;
}


#ఒకవేళ d_m3HasFloat
స్థిర
M3_ఫలితం ParseArgFloat (ccstr_t i_arg, f64* o_విలువ)
{
    ఒకవేళ (not i_arg or not *i_arg) {
        తిరిగి "empty argument";
    }

    ఒకవేళ (ఖాళీనా((గుర్తులేని అక్షరం)*i_arg)) {
        తిరిగి "argument is not a number";
    }

    అక్షరం* చివర = ఖాళీ;

    errno = 0;

    f64 విలువ = strtod(i_arg, &చివర);

    ఒకవేళ (చివర == i_arg or *చివర) {
        తిరిగి "argument is not a number";
    }

    // strtod reports underflow through ERANGE as well, and a denormal result is
    // perfectly usable, so only an overflow to infinity is out of range
    ఒకవేళ (errno == ERANGE and (విలువ > DBL_MAX or విలువ < -DBL_MAX)) {
        తిరిగి "argument out of range";
    }

    *o_విలువ = విలువ;

    తిరిగి m3Err_none;
}
#అయితే_ముగింపు


// A reference argument is either the null reference or a host handle written as
// an integer.
స్థిర
M3_ఫలితం ParseArgReference (ccstr_t i_arg, u64* o_విలువ)
{
    ఒకవేళ (i_arg and strcmp(i_arg, "null") == 0) {
        *o_విలువ = 0;
        తిరిగి m3Err_none;
    }

    తిరిగి ParseArgInteger(i_arg, 64, o_విలువ);
}


M3_ఫలితం m3_CallArgv (IM3Function i_function, uint32_t i_argc, స్థిరరాశి అక్షరం* i_argv[])
{
    IM3FuncType ftype   = i_function->funcType;
    IM3Runtime  runtime = i_function->module->runtime;
    M3_ఫలితం    ఫలితం  = m3Err_none;
    u8*         s       = ఖాళీ;

    ఒకవేళ (i_argc != ftype->అంకె_వాదనలు) {
        తిరిగి m3_Err_వాదం_లెక్క_Mismatch;
    }
    ఒకవేళ (!i_function->compiled) {
        తిరిగి m3_Err_missing_Compiled_సంకేతం;
    }

#ఒకవేళ d_m3_రికార్డు_Backtraces
    ClearBacktrace(runtime);
#అయితే_ముగింపు

    m3_దొంతర_Check_Init();

_   (check_ప్రారంభం_Function(i_function->module))

_   (Get_దొంతర_సూచిక_For_వాదనలు(i_function, &s))

    కొరకు (u32 i = 0; i < ftype->అంకె_వాదనలు; ++i) {
        u64 విలువ = 0;
#ఒకవేళ d_m3HasFloat
        f64 fvalue = 0;
#అయితే_ముగింపు
        ఎంపిక (d_FuncArgType(ftype, i)) {
        స్థితి c_m3Type_i32:
_           (ParseArgInteger(i_argv[i], 32, &విలువ)) * (i32*)(s) = (i32)విలువ;
            s += 8;
            ఆపు;
        స్థితి c_m3Type_i64:
_           (ParseArgInteger(i_argv[i], 64, &విలువ)) * (i64*)(s) = (i64)విలువ;
            s += 8;
            ఆపు;
        స్థితి c_m3Type_funcref:
        స్థితి c_m3Type_externref:
        స్థితి c_m3Type_exnref:
_           (ParseArgReference(i_argv[i], &విలువ)) * (uintptr_t*)(s) = (uintptr_t)విలువ;
            s += 8;
            ఆపు;
#ఒకవేళ d_m3HasFloat
            // strtof would be less portable
        స్థితి c_m3Type_f32:
_           (ParseArgFloat(i_argv[i], &fvalue)) * (f32*)(s) = (f32)fvalue;
            s += 8;
            ఆపు;
        స్థితి c_m3Type_f64:
_           (ParseArgFloat(i_argv[i], &fvalue)) * (f64*)(s) = fvalue;
            s += 8;
            ఆపు;
#అయితే_ముగింపు
        అప్రమేయ: _throw("unknown argument type");
        }
    }

    ఫలితం = నిరంతరం_సంకేతం_Checked(runtime, i_function);

    Report_Native_దొంతర_Usage();

    runtime->చివరి_Called = ఫలితం ? ఖాళీ : i_function;

    _catch: తిరిగి ఫలితం;
}


//u8 * AlignStackPointerTo64Bits (const u8 * i_stack)
//{
//    uintptr_t ptr = (uintptr_t) i_stack;
//    return (u8 *) ((ptr + 7) & ~7);
//}


M3_ఫలితం m3_GetResults (IM3Function i_function, uint32_t i_retc, స్థిరరాశి శూన్య* o_retptrs[])
{
    IM3FuncType ftype   = i_function->funcType;
    IM3Runtime  runtime = i_function->module->runtime;

    ఒకవేళ (i_retc != ftype->అంకె_Rets) {
        తిరిగి m3_Err_వాదం_లెక్క_Mismatch;
    }
    ఒకవేళ (i_function != runtime->చివరి_Called) {
        తిరిగి "function not called";
    }

    u8* s = (u8*)runtime->దొంతర;

    కొరకు (u32 i = 0; i < ftype->అంకె_Rets; ++i) {
        ఎంపిక (d_FuncRetType(ftype, i)) {
        స్థితి c_m3Type_i32:
            *(i32*)o_retptrs[i] = *(i32*)(s);
            s += 8;
            ఆపు;
        స్థితి c_m3Type_i64:
            *(i64*)o_retptrs[i] = *(i64*)(s);
            s += 8;
            ఆపు;
        స్థితి c_m3Type_funcref:
        స్థితి c_m3Type_externref:
        స్థితి c_m3Type_exnref:
            *(uintptr_t*)o_retptrs[i] = *(uintptr_t*)(s);
            s += 8;
            ఆపు;
#ఒకవేళ d_m3HasFloat
        స్థితి c_m3Type_f32:
            *(f32*)o_retptrs[i] = *(f32*)(s);
            s += 8;
            ఆపు;
        స్థితి c_m3Type_f64:
            *(f64*)o_retptrs[i] = *(f64*)(s);
            s += 8;
            ఆపు;
#అయితే_ముగింపు
        అప్రమేయ: తిరిగి "unknown return type";
        }
    }
    తిరిగి m3Err_none;
}

M3_ఫలితం m3_GetResultsV (IM3Function i_function, ...)
{
    va_list ap;
    va_start(ap, i_function);
    M3_ఫలితం r = m3_GetResultsVL(i_function, ap);
    va_end(ap);
    తిరిగి r;
}

M3_ఫలితం m3_GetResultsVL (IM3Function i_function, va_list o_rets)
{
    IM3Runtime  runtime = i_function->module->runtime;
    IM3FuncType ftype   = i_function->funcType;

    ఒకవేళ (i_function != runtime->చివరి_Called) {
        తిరిగి "function not called";
    }

    u8* s = (u8*)runtime->దొంతర;
    కొరకు (u32 i = 0; i < ftype->అంకె_Rets; ++i) {
        ఎంపిక (d_FuncRetType(ftype, i)) {
        స్థితి c_m3Type_i32:
            *va_arg(o_rets, i32*) = *(i32*)(s);
            s += 8;
            ఆపు;
        స్థితి c_m3Type_i64:
            *va_arg(o_rets, i64*) = *(i64*)(s);
            s += 8;
            ఆపు;
        స్థితి c_m3Type_funcref:
        స్థితి c_m3Type_externref:
        స్థితి c_m3Type_exnref:
            *va_arg(o_rets, uintptr_t*) = *(uintptr_t*)(s);
            s += 8;
            ఆపు;
#ఒకవేళ d_m3HasFloat
        స్థితి c_m3Type_f32:
            *va_arg(o_rets, f32*) = *(f32*)(s);
            s += 8;
            ఆపు;
        స్థితి c_m3Type_f64:
            *va_arg(o_rets, f64*) = *(f64*)(s);
            s += 8;
            ఆపు;
#అయితే_ముగింపు
        అప్రమేయ: తిరిగి "unknown argument type";
        }
    }
    తిరిగి m3Err_none;
}

శూన్య Release_సంకేతం_పుట_No_Track (IM3Runtime i_runtime, IM3_సంకేతం_పుట i_సంకేతం_పుట)
{
    ఒకవేళ (i_సంకేతం_పుట) {
        IM3_సంకేతం_పుట* list;

        bool పుట_నిండిన = (అంకె_Free_పంక్తులు(i_సంకేతం_పుట) < d_m3_సంకేతం_పుట_Free_పంక్తులు_పరిమితి);
        ఒకవేళ (పుట_నిండిన) {
            list = &i_runtime->పుటలు_నిండిన;
        } లేకపోతే {
            list = &i_runtime->పుటలు_Open;
        }

        నెట్టు_సంకేతం_పుట(list, i_సంకేతం_పుట);                         m3log (emit, "release page: %d to queue: '%s'", i_సంకేతం_పుట->info.sequence, పుట_నిండిన ? "full" : "open")
    }
}


IM3_సంకేతం_పుట Acquire_సంకేతం_పుట_With_సామర్థ్యం (IM3Runtime i_runtime, u32 i_min_పంక్తి_లెక్క)
{
    IM3_సంకేతం_పుట పుట = Remove_సంకేతం_పుట_యొక్క_సామర్థ్యం(&i_runtime->పుటలు_Open, i_min_పంక్తి_లెక్క);

    ఒకవేళ (not పుట) {
        పుట = Environment_Acquire_సంకేతం_పుట(i_runtime->environment, i_min_పంక్తి_లెక్క);

        ఒకవేళ (not పుట) {
            పుట = New_సంకేతం_పుట(i_runtime, i_min_పంక్తి_లెక్క);
        }

        ఒకవేళ (పుట) {
            i_runtime->అంకె_సంకేతం_పుటలు++;
        }
    }

    ఒకవేళ (పుట) {                                                  m3log (emit, "acquire page: %d", పుట->info.sequence);
        i_runtime->అంకె_చురుకైన_సంకేతం_పుటలు++;
    }

    తిరిగి పుట;
}


IM3_సంకేతం_పుట Acquire_సంకేతం_పుట (IM3Runtime i_runtime)
{
    తిరిగి Acquire_సంకేతం_పుట_With_సామర్థ్యం(i_runtime, d_m3_సంకేతం_పుట_Free_పంక్తులు_పరిమితి);
}


శూన్య Release_సంకేతం_పుట (IM3Runtime i_runtime, IM3_సంకేతం_పుట i_సంకేతం_పుట)
{
    ఒకవేళ (i_సంకేతం_పుట) {
        Release_సంకేతం_పుట_No_Track(i_runtime, i_సంకేతం_పుట);
        i_runtime->అంకె_చురుకైన_సంకేతం_పుటలు--;

#ఒకవేళ defined(DEBUG)
        u32 అంకె_Open = లెక్క_సంకేతం_పుటలు(i_runtime->పుటలు_Open);
        u32 అంకె_నిండిన = లెక్క_సంకేతం_పుటలు(i_runtime->పుటలు_నిండిన);

        m3log(runtime, "runtime: %p; open-pages: %d; full-pages: %d; active: %d; total: %d", i_runtime, అంకె_Open, అంకె_నిండిన, i_runtime->అంకె_చురుకైన_సంకేతం_పుటలు, i_runtime->అంకె_సంకేతం_పుటలు);

        d_m3Assert(అంకె_Open + అంకె_నిండిన + i_runtime->అంకె_చురుకైన_సంకేతం_పుటలు == i_runtime->అంకె_సంకేతం_పుటలు);

#  ఒకవేళ d_m3_Log_సంకేతం_పుటలు
        dump_సంకేతం_పుట(i_సంకేతం_పుట, /* startPC: */ ఖాళీ);
#  endif
#అయితే_ముగింపు
    }
}


#ఒకవేళ d_m3VerboseErrorMessages
M3_ఫలితం m3Error (M3_ఫలితం i_ఫలితం, IM3Runtime i_runtime, IM3Module i_module, IM3Function i_function,
                  స్థిరరాశి అక్షరం* స్థిరరాశి i_file, u32 i_పంక్తి_అంకె, స్థిరరాశి అక్షరం* స్థిరరాశి i_error_సందేశం, ...)
{
    ఒకవేళ (i_runtime) {
        i_runtime->error         = (M3ErrorInfo){ .ఫలితం = i_ఫలితం, .runtime = i_runtime, .module = i_module, .function = i_function, .file = i_file, .పంక్తి_2 = i_పంక్తి_అంకె };
        i_runtime->error.సందేశం = i_runtime->error_సందేశం;

        va_list వాదనలు;
        va_start(వాదనలు, i_error_సందేశం);
        vsnprintf(i_runtime->error_సందేశం, పరిమాణం(i_runtime->error_సందేశం), i_error_సందేశం, వాదనలు);
        va_end(వాదనలు);
    }

    తిరిగి i_ఫలితం;
}
#అయితే_ముగింపు


శూన్య m3_GetErrorInfo (IM3Runtime i_runtime, M3ErrorInfo* o_info)
{
    ఒకవేళ (i_runtime) {
        *o_info = i_runtime->error;
        m3_ResetErrorInfo(i_runtime);
    }
}


శూన్య m3_ResetErrorInfo (IM3Runtime i_runtime)
{
    ఒకవేళ (i_runtime) {
        M3_INIT(i_runtime->error);
        i_runtime->error.సందేశం = "";
    }
}

uint8_t* m3_Get_జ్ఞాపకం (IM3Module i_module, size_t* o_జ్ఞాపకం_కొలత_లోపల_బైట్లు, uint32_t i_జ్ఞాపకం_సూచీ)
{
    uint8_t* జ్ఞాపకం = ఖాళీ;
    size_t   కొలత   = 0;

    ఒకవేళ (i_module and i_జ్ఞాపకం_సూచీ < i_module->అంకె_Memories) {
        IM3_జ్ఞాపకం mem = i_module->memories[i_జ్ఞాపకం_సూచీ];

        ఒకవేళ (mem->mallocated) {
            కొలత = mem->mallocated->నిడివి;

            ఒకవేళ (కొలత) {
                జ్ఞాపకం = m3_Mem_దత్తాంశం(mem->mallocated);
            }
        }
    }

    ఒకవేళ (o_జ్ఞాపకం_కొలత_లోపల_బైట్లు) {
        *o_జ్ఞాపకం_కొలత_లోపల_బైట్లు = కొలత;
    }

    తిరిగి జ్ఞాపకం;
}


size_t m3_Get_జ్ఞాపకం_కొలత (IM3Module i_module, uint32_t i_జ్ఞాపకం_సూచీ)
{
    ఒకవేళ (not i_module or i_జ్ఞాపకం_సూచీ >= i_module->అంకె_Memories) {
        తిరిగి 0;
    }

    IM3_జ్ఞాపకం mem = i_module->memories[i_జ్ఞాపకం_సూచీ];

    తిరిగి mem->mallocated ? mem->mallocated->నిడివి : 0;
}


size_t m3_Get_జ్ఞాపకం_కొలత_At (స్థిరరాశి శూన్య* i_జ్ఞాపకం)
{
    ఒకవేళ (not i_జ్ఞాపకం) {
        తిరిగి 0;
    }

    // the header sits immediately before the data it describes
    స్థిరరాశి M3_జ్ఞాపకం_శీర్షిక* శీర్షిక = ((స్థిరరాశి M3_జ్ఞాపకం_శీర్షిక*)i_జ్ఞాపకం) - 1;

    తిరిగి శీర్షిక->నిడివి;
}


M3_ఫలితం m3_కనుగొను_Exported_జ్ఞాపకం (IM3Module i_module, cstr_t i_name, u32* o_జ్ఞాపకం_సూచీ)
{
    ఒకవేళ (not i_module or not i_name or not o_జ్ఞాపకం_సూచీ) {
        తిరిగి m3_Err_unknown_జ్ఞాపకం;
    }

    స్థిరరాశి M3Export* e = Module_కనుగొను_Export(i_module, d_external_రకం_జ్ఞాపకం, i_name);

    ఒకవేళ (not e) {
        తిరిగి m3_Err_unknown_జ్ఞాపకం;
    }

    *o_జ్ఞాపకం_సూచీ = e->index;
    తిరిగి m3Err_none;
}


bool Module_HasLinkedHostImport (IM3Module i_module, cstr_t i_importModule)
{
    కొరకు (u32 i = 0; i < i_module->అంకె_Functions; ++i) {
        IM3Function f = &i_module->functions[i];

        // hostMemory is set when a host function is bound to the import, so it
        // is also the mark of one - see CompileRawFunction
        ఒకవేళ (not f->host_జ్ఞాపకం or f->wasm or f->resolved) {
            కొనసాగు;
        }

        ఒకవేళ (f->import.moduleUtf8 and strcmp(f->import.moduleUtf8, i_importModule) == 0) {
            తిరిగి true;
        }
    }

    తిరిగి false;
}


M3_ఫలితం m3_Bind_Import_జ్ఞాపకం (IM3Module io_module, cstr_t i_importModule, u32 i_జ్ఞాపకం_సూచీ)
{
    ఒకవేళ (not io_module or not i_importModule) {
        తిరిగి m3Err_moduleNotLinked;
    }

    ఒకవేళ (i_జ్ఞాపకం_సూచీ >= io_module->అంకె_Memories) {
        తిరిగి m3_Err_unknown_జ్ఞాపకం;
    }

    // the memory itself, not the index: linking may have pointed this slot at
    // another module's memory, and growing one reallocates behind it
    IM3_జ్ఞాపకం జ్ఞాపకం = io_module->memories[i_జ్ఞాపకం_సూచీ];

    స్థిరరాశి bool wildcardModule = (strcmp(i_importModule, "*") == 0);

    కొరకు (u32 i = 0; i < io_module->అంకె_Functions; ++i) {
        IM3Function f = &io_module->functions[i];

        // only an import that a host function was bound to has a memory to
        // pin: one with a body of its own reads memory through the interpreter,
        // and one resolved to another module's export runs that module's code
        ఒకవేళ (not f->host_జ్ఞాపకం or f->wasm or f->resolved) {
            కొనసాగు;
        }

        ఒకవేళ (wildcardModule or (f->import.moduleUtf8 and strcmp(f->import.moduleUtf8, i_importModule) == 0)) {
            f->host_జ్ఞాపకం = జ్ఞాపకం;
        }
    }

    తిరిగి m3Err_none;
}


M3BacktraceInfo* m3_GetBacktrace (IM3Runtime i_runtime)
{
#ఒకవేళ d_m3_రికార్డు_Backtraces
    తిరిగి &i_runtime->backtrace;
#లేకపోతే
    తిరిగి ఖాళీ;
#అయితే_ముగింపు
}


//-------------------------------------------------------------------------------------------------------------------------------
//  snapshots & suspendable execution
//-------------------------------------------------------------------------------------------------------------------------------

శూన్య m3_సమితి_Suspendable (IM3Runtime io_runtime, bool i_suspendable)
{
    ఒకవేళ (io_runtime) {
        io_runtime->isSuspendable = i_suspendable;
    }
}

శూన్య m3_RequestSuspend (IM3Runtime io_runtime)
{
    ఒకవేళ (io_runtime) {
        io_runtime->suspendRequested = true;
    }
}

bool m3_IsSuspended (IM3Runtime i_runtime)
{
    ఒకవేళ (!i_runtime) {
        తిరిగి false;
    }
#ఒకవేళ d_m3_Has_దొంతర_Switching
    ఒకవేళ (i_runtime->మూలం_Continuation && i_runtime->మూలం_Continuation->state == cont_suspended) {
        తిరిగి true;
    }
#అయితే_ముగింపు
    తిరిగి false;
}

M3_ఫలితం m3_ResumeRuntime (IM3Runtime io_runtime)
{
    ఒకవేళ (!io_runtime) {
        తిరిగి m3Err_mallocFailed;
    }
    ఒకవేళ (!io_runtime->isSuspendable) {
        తిరిగి m3Err_none;
    }
#ఒకవేళ d_m3HasSnapshots
    // a restore that failed leaves nothing to resume, and says so every time
    // rather than resuming nothing successfully
    ఒకవేళ (io_runtime->మూలం_Continuation and io_runtime->మూలం_Continuation->నమోదు_Function and
        io_runtime->మూలం_Continuation->నమోదు_Function->module->isUnusable) {
        తిరిగి "a snapshot failed to restore into the module, so it cannot run";
    }
#అయితే_ముగింపు
#ఒకవేళ d_m3_Has_దొంతర_Switching
    ఒకవేళ (!io_runtime->మూలం_Continuation || io_runtime->మూలం_Continuation->state != cont_suspended) {
        తిరిగి m3Err_none;
    }
    // what finishes returns into the bottom of the stack, where it was entered
    io_runtime->దొంతర = io_runtime->origin_దొంతర;

    // this is the outermost call again, as far as a host function calling back
    // into Wasm from inside it can tell
    io_runtime->entered = io_runtime->మూలం_Continuation->నమోదు_Function;
    io_runtime->callNesting++;

    d_m3_దొంతర_హద్దు_Enter(io_runtime);
    io_runtime->resumePastCheck = true;
    M3_ఫలితం ఫలితం             = (M3_ఫలితం)ResumeContinuation(io_runtime, io_runtime->మూలం_Continuation);
    io_runtime->resumePastCheck = false;
    d_m3_దొంతర_హద్దు_Leave(io_runtime);

#  ఒకవేళ d_m3HasExceptionHandling
    ఒకవేళ (M3_UNLIKELY(ఫలితం == m3Err_pendingException)) {
        ఫలితం = m3Err_trapUncaughtException;
    }
    రికార్డు_Uncaught_Exception(io_runtime, ఫలితం);
#  endif

    io_runtime->callNesting--;

#  ఒకవేళ d_m3HasExceptionHandling
    ఒకవేళ (ఫలితం != m3Err_continuationSuspended and io_runtime->callNesting == 0) {
        FreeExceptions(io_runtime);
    }
#  endif

    // the results are where the paused call left room for them, so a finished
    // resume can be asked for them the way the call itself could have been
    io_runtime->చివరి_Called = (ఫలితం == m3Err_none) ? io_runtime->మూలం_Continuation->నమోదు_Function : ఖాళీ;

    Runtime_ఉంచు_Call_దొంతర(io_runtime);
    తిరిగి ఫలితం;
#లేకపోతే
    తిరిగి m3Err_none;
#అయితే_ముగింపు
}
