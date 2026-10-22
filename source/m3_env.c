//
//  m3_env.c
//
//  Created by Steven Massey on 4/19/19.
//  Copyright © 2019 Steven Massey. All rights reserved.
//

#अन्तर्भाव <stdarg.h>
#अन्तर्भाव <limits.h>
#अन्तर्भाव <errno.h>
#अन्तर्भाव <float.h>
#अन्तर्भाव <ctype.h>

#अन्तर्भाव "m3_env.h"
#अन्तर्भाव "m3_bind.h"
#अन्तर्भाव "m3_deterministic.h"
#अन्तर्भाव "m3_compile.h"
#अन्तर्भाव "m3_host.h"
#अन्तर्भाव "m3_exception.h"
#अन्तर्भाव "m3_info.h"


IM3Environment m3_NewEnvironment ()
{
    IM3Environment env = m3_AllocStruct(M3Environment);

    यदि (env) {
        _try
        {
            // create FuncTypes for all simple block return ValueTypes.
            // v128 is skipped: it parses as a slot but has no operations.
            प्रति (u8 t = c_m3Type_none; t < c_m3_Type_गणनम्; t++) {
                यदि (t == c_m3Type_v128) {
                    अनुवर्त;
                }

                IM3FuncType ftype;
_               (AllocFuncType(&ftype, 1));

                ftype->अङ्कः_तर्काः  = 0;
                ftype->अङ्कः_Rets  = (t == c_m3Type_none) ? 0 : 1;
                ftype->types[0] = t;

                Environment_सङ्कलय_Func_Type(env, &ftype);

                env->retFuncTypes[t] = ftype;
            }
        }

        _catch:
        यदि (परिणाम) {
            m3_FreeEnvironment(env);
            env = रिक्त;
        }
    }

    प्रत्यागम env;
}


शून्य Environment_Release (IM3Environment i_environment)
{
    IM3FuncType ftype = i_environment->funcTypes;

    यावत् (ftype) {
        IM3FuncType अग्र = ftype->अग्र;
        m3_Free(ftype);
        ftype = अग्र;
    }

    m3log(runtime, "freeing %d pages from environment", गणनम्_संहिता_पृष्ठानि(i_environment->पृष्ठानि_Released));
    Free_संहिता_पृष्ठानि(&i_environment->पृष्ठानि_Released);
}


शून्य m3_FreeEnvironment (IM3Environment i_environment)
{
    यदि (i_environment) {
        Environment_Release(i_environment);
        m3_Free(i_environment);
    }
}


शून्य m3_समुच्चयः_Custom_Section_Handler (IM3Environment i_environment, M3SectionHandler i_handler)
{
    यदि (i_environment) {
        i_environment->customSectionHandler = i_handler;
    }
}


// returns the same io_funcType or replaces it with an equivalent that's already in the type linked list
M3_परिणाम Environment_सङ्कलय_Func_Type (IM3Environment i_environment, IM3FuncType* io_funcType)
{
    IM3FuncType सङ्कलय_Type = *io_funcType;
    IM3FuncType newType = i_environment->funcTypes;

    यावत् (newType) {
        यदि (not newType->अन्तर्_Rec_गणः and AreFuncTypesEqual(newType, सङ्कलय_Type)) {
            m3_Free(सङ्कलय_Type);
            भङ्ग;
        }

        newType = newType->अग्र;
    }

    यदि (newType == रिक्त) {
        // a type index has to fit in the heap type field of an m3type_t
        यदि (i_environment->अङ्कः_Func_Types >= d_m3_Max_Sane_Types_गणनम्) {
            m3_Free(सङ्कलय_Type);
            *io_funcType = रिक्त;
            प्रत्यागम "too many distinct function types";
        }

        newType                  = सङ्कलय_Type;
        newType->canonical_सूचकांक  = i_environment->अङ्कः_Func_Types++;
        newType->अग्र            = i_environment->funcTypes;
        i_environment->funcTypes = newType;
    }

    *io_funcType = newType;

    प्रत्यागम m3Err_none;
}


// Adds a type without looking for an equal one first, and hands back the index
// it was given.
//
// Members of a recursive group do not get to share an entry with a type that
// merely looks like them: the group is their identity, so two groups spelled
// the same way are still two groups, and a type inside one is never the type
// outside it that matches field for field. Reserving the indices up front is
// also what lets the members refer to each other while they are being read.
M3_परिणाम Environment_ReserveFuncTypes (IM3Environment i_environment, u32 i_गणनम्, u16* o_first_सूचकांक)
{
    यदि ((u64)i_environment->अङ्कः_Func_Types + i_गणनम् > d_m3_Max_Sane_Types_गणनम्) {
        प्रत्यागम "too many distinct function types";
    }

    *o_first_सूचकांक = i_environment->अङ्कः_Func_Types;
    i_environment->अङ्कः_Func_Types += (u16)i_गणनम्;

    प्रत्यागम m3Err_none;
}


शून्य Environment_AdoptFuncType (IM3Environment i_environment, IM3FuncType i_funcType, u16 i_canonical_सूचकांक)
{
    i_funcType->अन्तर्_Rec_गणः     = true;
    i_funcType->canonical_सूचकांक = i_canonical_सूचकांक;
    i_funcType->अग्र           = i_environment->funcTypes;
    i_environment->funcTypes   = i_funcType;
}


IM3_संहिता_Page Remove_संहिता_Page_सम्बन्धि_क्षमता (M3_संहिता_Page** io_list, u32 i_minimum_पङ्क्तिः_गणनम्)
{
    IM3_संहिता_Page पूर्व = रिक्त;
    IM3_संहिता_Page page = *io_list;

    यावत् (page) {
        यदि (अङ्कः_Free_पङ्क्तयः(page) >= i_minimum_पङ्क्तिः_गणनम्) {             d_m3Assert (page->info.usage_गणनम् == 0);
            IM3_संहिता_Page अग्र = page->info.अग्र;
            यदि (पूर्व) {
                पूर्व->info.अग्र = अग्र; // mid-list
            } अन्यथा {
                *io_list = अग्र;       // front of list
            }

            भङ्ग;
        }

        पूर्व = page;
        page = page->info.अग्र;
    }

    प्रत्यागम page;
}


IM3_संहिता_Page Environment_Acquire_संहिता_Page (IM3Environment i_environment, u32 i_minimum_पङ्क्तिः_गणनम्)
{
    प्रत्यागम Remove_संहिता_Page_सम्बन्धि_क्षमता(&i_environment->पृष्ठानि_Released, i_minimum_पङ्क्तिः_गणनम्);
}


शून्य Environment_Release_संहिता_पृष्ठानि (IM3Environment i_environment, IM3_संहिता_Page i_संहिता_Page_List)
{
    IM3_संहिता_Page अन्तः = i_संहिता_Page_List;

    यावत् (अन्तः) {
        अन्तः->info.पङ्क्तिः_सूचकांक = 0; // reset page
#यदि d_m3_लेखः_Backtraces
        अन्तः->info.mapping->आकार = 0;
#यदि_अन्तः // d_m3RecordBacktraces

        IM3_संहिता_Page अग्र = अन्तः->info.अग्र;
        यदि (not अग्र) {
            भङ्ग;
        }

        अन्तः = अग्र;
    }

    यदि (अन्तः) {
        // push list to front
        अन्तः->info.अग्र               = i_environment->पृष्ठानि_Released;
        i_environment->पृष्ठानि_Released = i_संहिता_Page_List;
    }
}


IM3Runtime m3_NewRuntime (IM3Environment i_environment, u32 i_राशिः_आकार_अन्तर्_अष्टकानि, शून्य* i_userdata)
{
    IM3Runtime runtime = m3_AllocStruct(M3Runtime);

    यदि (runtime) {
        m3_ResetErrorInfo(runtime);

        runtime->environment = i_environment;
        runtime->userdata    = i_userdata;

        Deterministic_InitRuntime(runtime);

        runtime->origin_राशिः = m3_Malloc("Wasm Stack", i_राशिः_आकार_अन्तर्_अष्टकानि + 4 * परिमाण(m3slot_t)); // TODO: more precise stack checks

        यदि (runtime->origin_राशिः) {
            runtime->राशिः         = runtime->origin_राशिः;
            runtime->अङ्कः_राशिः_Slots = i_राशिः_आकार_अन्तर्_अष्टकानि / परिमाण(m3slot_t);          m3log (runtime, "new stack: %p, slots: %u", runtime->origin_राशिः, runtime->अङ्कः_राशिः_Slots);
        } अन्यथा {
            m3_Free(runtime);
        }
    }

    प्रत्यागम runtime;
}

शून्य m3_समुच्चयः_Validation (IM3Runtime i_runtime, bool i_enable)
{
#यदि d_m3EnableValidation
    यदि (i_runtime) {
        i_runtime->skipValidation = not i_enable;
    }
#अन्यथा
    (शून्य)i_runtime;
    (शून्य)i_enable;              // nothing to skip: the validator was compiled out
#यदि_अन्तः
}

M3_परिणाम m3_समुच्चयः_Resource_सीमा (IM3Runtime i_runtime, M3_Resource_सीमा i_सीमा, uint64_t i_मानम्)
{
    यदि (not i_runtime) {
        प्रत्यागम m3Err_mallocFailed;
    }
    विकल्प (i_सीमा) {
    पक्ष c_m3_सीमा_स्मृतिः_अष्टकानि:
        यदि (i_मानम् and i_मानम् < i_runtime->स्मृतिः_अष्टकानि_प्रयुक्तम्) {
            प्रत्यागम m3_Err_resource_सीमा_अधः_Usage;
        }
        i_runtime->स्मृतिः_अष्टकानि_सीमा = i_मानम्;
        प्रत्यागम m3Err_none;
    पक्ष c_m3_सीमा_सारणी_Elements:
        यदि (i_मानम् and i_मानम् < i_runtime->सारणी_Elements_प्रयुक्तम्) {
            प्रत्यागम m3_Err_resource_सीमा_अधः_Usage;
        }
        i_runtime->सारणी_Elements_सीमा = i_मानम्;
        प्रत्यागम m3Err_none;
#यदि d_m3_Has_राशिः_Switching
    पक्ष c_m3_सीमा_Continuations:
        यदि (i_मानम् and i_मानम् < i_runtime->continuationsAllocated) {
            प्रत्यागम m3_Err_resource_सीमा_अधः_Usage;
        }
        i_runtime->continuations_सीमा = (u32)M3_MIN(i_मानम्, (u64)UINT32_MAX);
        यावत् (i_मानम् and i_runtime->val_राशिः_Pool_गणनम् > i_मानम् - i_runtime->continuationsAllocated) {
            m3_Free(i_runtime->val_राशिः_Pool[--i_runtime->val_राशिः_Pool_गणनम्]);
        }
        प्रत्यागम m3Err_none;
#अन्यथा
    पक्ष c_m3_सीमा_Continuations: प्रत्यागम m3_Err_resource_सीमा_Not_Supported;
#यदि_अन्तः
    पक्ष c_m3_सीमा_Gas_Units:
#यदि d_m3HasGasMetering
        i_runtime->gas_सीमा = i_runtime->gasRemaining = (i64)M3_MIN(i_मानम्, (u64)INT64_MAX);
        प्रत्यागम m3Err_none;
#अन्यथा
        प्रत्यागम m3_Err_resource_सीमा_Not_Supported;
#यदि_अन्तः
    स्वभाव: प्रत्यागम m3_Err_unknown_Resource_सीमा;
    }
}

uint64_t m3_Get_Resource_सीमा (IM3Runtime i_runtime, M3_Resource_सीमा i_सीमा)
{
    यदि (not i_runtime) {
        प्रत्यागम 0;
    }
    विकल्प (i_सीमा) {
    पक्ष c_m3_सीमा_स्मृतिः_अष्टकानि: प्रत्यागम i_runtime->स्मृतिः_अष्टकानि_सीमा;
    पक्ष c_m3_सीमा_सारणी_Elements: प्रत्यागम i_runtime->सारणी_Elements_सीमा;
#यदि d_m3_Has_राशिः_Switching
    पक्ष c_m3_सीमा_Continuations: प्रत्यागम i_runtime->continuations_सीमा;
#यदि_अन्तः
#यदि d_m3HasGasMetering
    पक्ष c_m3_सीमा_Gas_Units: प्रत्यागम (u64)i_runtime->gas_सीमा;
#यदि_अन्तः
    स्वभाव: प्रत्यागम 0;
    }
}

uint64_t m3_GetResourceUsage (IM3Runtime i_runtime, M3_Resource_सीमा i_सीमा)
{
    यदि (not i_runtime) {
        प्रत्यागम 0;
    }
    विकल्प (i_सीमा) {
    पक्ष c_m3_सीमा_स्मृतिः_अष्टकानि: प्रत्यागम i_runtime->स्मृतिः_अष्टकानि_प्रयुक्तम्;
    पक्ष c_m3_सीमा_सारणी_Elements: प्रत्यागम i_runtime->सारणी_Elements_प्रयुक्तम्;
#यदि d_m3_Has_राशिः_Switching
    पक्ष c_m3_सीमा_Continuations: प्रत्यागम i_runtime->continuationsAllocated;
#यदि_अन्तः
#यदि d_m3HasGasMetering
    पक्ष c_m3_सीमा_Gas_Units: प्रत्यागम (u64)i_runtime->gas_सीमा - (u64)i_runtime->gasRemaining;
#यदि_अन्तः
    स्वभाव: प्रत्यागम 0;
    }
}

शून्य* m3_Get_User_दत्त (IM3Runtime i_runtime)
{
    प्रत्यागम i_runtime ? i_runtime->userdata : रिक्त;
}


शून्य* ForEachModule (IM3Runtime i_runtime, ModuleVisitor i_visitor, शून्य* i_info)
{
    शून्य* r = रिक्त;

    IM3Module module = i_runtime->modules;

    यावत् (module) {
        IM3Module अग्र = module->अग्र;
        यदि ((r = i_visitor(module, i_info))) {
            भङ्ग;
        }

        module = अग्र;
    }

    प्रत्यागम r;
}


शून्य* _FreeModule (IM3Module i_module, शून्य* i_info)
{
    m3_FreeModule(i_module);
    प्रत्यागम रिक्त;
}


// A module's memory and table index spaces hold borrowed pointers: linking
// repoints an imported slot at the memory or table the exporting module owns.
// m3_FreeModule tells a borrowed slot from an owned one by reading that owner,
// which only holds while the object is still allocated - and the free walk
// below visits modules in load order, with no ordering between an owner and
// the modules borrowing from it. So clear the borrowed slots first, in a pass
// that frees nothing; the walk then skips them on the NULL check it already
// does, instead of reading a memory some earlier module has freed.
शून्य* _ReleaseBorrowedSlots (IM3Module i_module, शून्य* i_info)
{
    प्रति (u32 i = 0; i < i_module->अङ्कः_Memories; ++i) {
        IM3_स्मृतिः स्मृतिः = i_module->memories[i];

        यदि (स्मृतिः and स्मृतिः->owner != i_module) {
            i_module->memories[i] = रिक्त;
        }
    }

    प्रति (u32 i = 0; i < i_module->अङ्कः_Tables; ++i) {
        IM3_सारणी सारणी_2 = i_module->tables[i];

        यदि (सारणी_2 and सारणी_2->owner != i_module) {
            i_module->tables[i] = रिक्त;
        }
    }

    प्रत्यागम रिक्त;
}


शून्य Runtime_Release (IM3Runtime i_runtime)
{
    ForEachModule(i_runtime, _ReleaseBorrowedSlots, रिक्त);
    ForEachModule(i_runtime, _FreeModule, रिक्त);                    d_m3Assert (i_runtime->अङ्कः_सक्रिय_संहिता_पृष्ठानि == 0);

    Environment_Release_संहिता_पृष्ठानि(i_runtime->environment, i_runtime->पृष्ठानि_Open);
    Environment_Release_संहिता_पृष्ठानि(i_runtime->environment, i_runtime->पृष्ठानि_पूर्णम्);

    m3_Free(i_runtime->origin_राशिः);

#यदि d_m3EnableValidation
    m3_Free(i_runtime->validator);
#यदि_अन्तः

#यदि d_m3HasExceptionHandling
    // a runtime let go of while suspended still holds its exceptions
    FreeExceptions(i_runtime);
    m3_Free(i_runtime->uncaughtException);
#यदि_अन्तः
#यदि d_m3HasExceptionHandling || d_m3_Has_राशिः_Switching
    यावत् (i_runtime->host_चिह्नकानि) {
        M3_Host_चिह्नकम्* अग्र = i_runtime->host_चिह्नकानि->अग्र;
        m3_Free(i_runtime->host_चिह्नकानि);
        i_runtime->host_चिह्नकानि = अग्र;
    }
#यदि_अन्तः
#यदि d_m3_Has_राशिः_Switching
    Continuation_ReleaseAll(i_runtime);
#यदि_अन्तः
}


शून्य m3_FreeRuntime (IM3Runtime i_runtime)
{
    यदि (i_runtime) {
        m3_PrintProfilerInfo();

        Runtime_Release(i_runtime);
        m3_Free(i_runtime);
    }
}


#यदि d_m3_Has_राशिः_Switching

IM3Continuation Continuation_New (IM3Runtime i_runtime, IM3FuncType i_type, IM3Function i_function)
{
    IM3Continuation cont = m3_AllocStruct(M3Continuation);
    यदि (!cont) {
        प्रत्यागम रिक्त;
    }

    cont->type          = i_type;
    cont->प्रविष्टिः_Function = i_function;
    cont->state         = cont_allocated;

    cont->अङ्कः_राशिः_Slots = d_m3_Continuation_राशिः_Slots;
    // frames are recorded only when this continuation actually suspends, and
    // only as deep as it got, so the array starts out unallocated
    cont->चौकटाः    = रिक्त;
    cont->चौकटाः_Cap = 0;
    cont->अङ्कः_चौकटाः = 0;

    cont->sp = cont->val_राशिः;
    cont->pc = i_function ? i_function->compiled : रिक्त;
    cont->r0 = 0;
#  यदि d_m3HasFloat
    cont->fp0 = 0;
#  endif
    cont->bound_तर्काः_गणनम्    = 0;
    cont->अङ्कः_Suspend_Results = 0;
    cont->अङ्कः_Handlers       = 0;
    cont->handlersPC        = रिक्त;
    cont->parent            = रिक्त;

    cont->अग्र               = i_runtime->continuations;
    i_runtime->continuations = cont;

    प्रत्यागम cont;
}

IM3Continuation Continuation_ForkSuspended (IM3Runtime i_runtime, IM3Continuation i_cont)
{
    IM3Continuation newCont = Continuation_New(i_runtime, i_cont->type, i_cont->प्रविष्टिः_Function);
    यदि (M3_UNLIKELY(not newCont)) {
        प्रत्यागम रिक्त;
    }

    newCont->val_राशिः      = i_cont->val_राशिः;
    newCont->अङ्कः_राशिः_Slots = i_cont->अङ्कः_राशिः_Slots;
    newCont->sp            = i_cont->sp;
    newCont->pc            = i_cont->pc;
    newCont->suspendPoint  = i_cont->suspendPoint;
    newCont->r0            = i_cont->r0;
#  यदि d_m3HasFloat
    newCont->fp0 = i_cont->fp0;
#  endif
    newCont->चौकटाः    = i_cont->चौकटाः;
    newCont->चौकटाः_Cap = i_cont->चौकटाः_Cap;
    newCont->अङ्कः_चौकटाः = i_cont->अङ्कः_चौकटाः;

    newCont->bound_तर्काः_गणनम् = 0;
#  यदि d_m3HasExceptionHandling
    newCont->resumeThrow = i_cont->resumeThrow;
    i_cont->resumeThrow  = रिक्त;
#  endif
    newCont->अङ्कः_Suspend_Results = i_cont->अङ्कः_Suspend_Results;
    प्रति (u32 i = 0; i < i_cont->अङ्कः_Suspend_Results; ++i) {
        newCont->suspend_परिणाम_Offsets[i] = i_cont->suspend_परिणाम_Offsets[i];
        newCont->suspend_परिणाम_Is64[i]    = i_cont->suspend_परिणाम_Is64[i];
    }

    newCont->अङ्कः_Handlers = i_cont->अङ्कः_Handlers;
    newCont->handlersPC  = i_cont->handlersPC;
    newCont->parent      = i_cont->parent;
    newCont->state       = cont_suspended;

    i_cont->val_राशिः  = रिक्त;
    i_cont->चौकटाः    = रिक्त;
    i_cont->चौकटाः_Cap = 0;
    i_cont->अङ्कः_चौकटाः = 0;
    i_cont->state     = cont_consumed;

    प्रत्यागम newCont;
}

// Only cont.new and snapshot restoration acquire stacks. Binding and suspension
// transfer them between records without changing the active count.
M3_परिणाम Continuation_Acquire_राशिः (IM3Runtime runtime, IM3Continuation cont)
{
    यदि (not Continuation_Can_Acquire_राशिः(runtime)) {
        प्रत्यागम m3_Err_continuation_सीमा_Exceeded;
    }
    यदि (runtime->val_राशिः_Pool_गणनम्) {
        cont->val_राशिः = runtime->val_राशिः_Pool[--runtime->val_राशिः_Pool_गणनम्];
        स्मृतिं_पूरय(cont->val_राशिः, 0, (cont->अङ्कः_राशिः_Slots + 4) * परिमाण(m3slot_t));
    } अन्यथा {
        cont->val_राशिः = m3_AllocArray(m3slot_t, cont->अङ्कः_राशिः_Slots + 4);
    }
    यदि (not cont->val_राशिः) {
        प्रत्यागम m3Err_mallocFailed;
    }
    cont->sp = cont->val_राशिः;
    runtime->continuationsAllocated++;
    प्रत्यागम m3Err_none;
}

शून्य Continuation_Recycle_राशिः (IM3Runtime runtime, IM3Continuation cont)
{
    यदि (not cont->val_राशिः or cont == runtime->मूलम्_Continuation) {
        प्रत्यागम;
    }
    यदि (not runtime->val_राशिः_Pool) {
        runtime->val_राशिः_Pool = m3_AllocArray(m3slot_t*, 16);
        यदि (runtime->val_राशिः_Pool) {
            runtime->val_राशिः_Pool_Cap = 16;
        }
    }
    यदि (runtime->val_राशिः_Pool_गणनम् < runtime->val_राशिः_Pool_Cap and
        (not runtime->continuations_सीमा or runtime->val_राशिः_Pool_गणनम् <
                                              runtime->continuations_सीमा - runtime->continuationsAllocated + 1)) {
        runtime->val_राशिः_Pool[runtime->val_राशिः_Pool_गणनम्++] = cont->val_राशिः;
        cont->val_राशिः                                      = रिक्त;
    } अन्यथा {
        m3_Free(cont->val_राशिः);
    }
    cont->sp = रिक्त;
    runtime->continuationsAllocated--;
}

शून्य Continuation_ReleaseAll (IM3Runtime io_runtime)
{
    // Records live until teardown: a consumed reference can still be in a table
    // or on a Wasm stack. Its value stack has already been recycled.
    IM3Continuation cont = io_runtime->continuations;

    यावत् (cont) {
        IM3Continuation अग्र = cont->अग्र;

        m3_Free(cont->val_राशिः);
        m3_Free(cont->चौकटाः);
        m3_Free(cont);

        cont = अग्र;
    }

    प्रति (u32 i = 0; i < io_runtime->val_राशिः_Pool_गणनम्; ++i) {
        m3_Free(io_runtime->val_राशिः_Pool[i]);
    }
    m3_Free(io_runtime->val_राशिः_Pool);
    io_runtime->val_राशिः_Pool_गणनम् = io_runtime->val_राशिः_Pool_Cap = 0;
    io_runtime->continuationsAllocated                          = 0;
    io_runtime->continuations                                   = रिक्त;
    io_runtime->सक्रिय_Continuation                              = रिक्त;
    io_runtime->मूलम्_Continuation                                = रिक्त;
}


m3ret_t Continuation_लेखः_Frame (IM3Runtime i_runtime, ध्रुव M3Frame* i_frame)
{
    IM3Continuation cont = i_runtime->सक्रिय_Continuation;

    // the marker only travels between a suspend and the resume that runs it,
    // and that whole stretch has an active continuation
    d_m3Assert(cont);

    यदि (cont->अङ्कः_चौकटाः == cont->चौकटाः_Cap) {
        u32 newCap = cont->चौकटाः_Cap ? cont->चौकटाः_Cap * 2 : 8;

        यदि (newCap > d_m3_Continuation_Max_चौकटाः) {
            newCap = d_m3_Continuation_Max_चौकटाः;
        }

        यदि (newCap == cont->चौकटाः_Cap) {
            प्रत्यागम m3_Err_trap_राशिः_अतिप्रवाहः;
        }

        M3Frame* चौकटाः = m3_ReallocArray(M3Frame, cont->चौकटाः, newCap, cont->चौकटाः_Cap);

        यदि (not चौकटाः) {
            प्रत्यागम m3Err_mallocFailed;
        }

        cont->चौकटाः    = चौकटाः;
        cont->चौकटाः_Cap = newCap;
    }

    cont->चौकटाः[cont->अङ्कः_चौकटाः++] = *i_frame;

    प्रत्यागम m3Err_continuationSuspended;
}

#यदि_अन्तः // d_m3HasStackSwitching

M3_परिणाम EvaluateExpression (IM3Module i_module, शून्य* o_expressed, m3type_t i_type, अष्टकानि_t* io_अष्टकानि, cbytes_t i_अन्तः)
{
    M3_परिणाम परिणाम = m3Err_none;

    // OPTZ: use a simplified interpreter for expressions

    // create a temporary runtime context
#यदि defined(d_m3PreferStaticAlloc)
    स्थिर M3Runtime runtime;
#अन्यथा
    M3Runtime runtime;
#यदि_अन्तः
    M3_INIT(runtime);

    runtime.environment   = i_module->runtime->environment;
    runtime.अङ्कः_राशिः_Slots = i_module->runtime->अङ्कः_राशिः_Slots;
    runtime.राशिः         = i_module->runtime->राशिः;

    m3stack_t राशिः = (m3stack_t)runtime.राशिः;

    IM3Runtime savedRuntime = i_module->runtime;
    i_module->runtime       = &runtime;

    IM3Compilation o   = &runtime.compilation;
    o->runtime         = &runtime;
    o->module          = i_module;
    o->wasm            = *io_अष्टकानि;
    o->wasm_अन्तः         = i_अन्तः;
    o->चरमः_Opcode_Start = o->wasm;

    // Borrowed for the length of this expression only. Runtime_Release below hands it
    // to Environment_ReleaseCodePages, which resets lineIndex, so the page comes back
    // empty and the next expression reuses it rather than burning fresh lines.
    o->page = Acquire_संहिता_Page(&runtime);

    यदि (o->page) {
        IM3FuncType ftype = रिक्त;

#यदि d_m3HasTypedRefs
        // A reference type has to be checked as the type it is: the shared
        // return types are one per storage class, so borrowing one of those
        // would let any reference at all satisfy a particular one.
        यदि (IsSpelledRefType(i_type)) {
            परिणाम = AllocFuncType(&ftype, 1);

            यदि (not परिणाम) {
                ftype->अङ्कः_Rets  = 1;
                ftype->types[0] = i_type;

                परिणाम = Environment_सङ्कलय_Func_Type(runtime.environment, &ftype);
            }
        } अन्यथा
#यदि_अन्तः
        {
            ftype = runtime.environment->retFuncTypes[आधारः_Type_सम्बन्धि(i_type)];
        }

        pc_t m3code = GetPagePC(o->page);

        यदि (not परिणाम) {
            परिणाम = CompileExpression(o, ftype);
        }

        यदि (not परिणाम && o->max_राशिः_Slots >= runtime.अङ्कः_राशिः_Slots) {
            परिणाम = m3_Err_trap_राशिः_अतिप्रवाहः;
        }

        यदि (not परिणाम) {
#यदि (d_m3EnableOpProfiling || d_m3EnableOpTracing)
            m3ret_t r = सततम्_संहिता(m3code, राशिः, रिक्त, d_m3_Op_Default_तर्काः, d_m3_आधारः_Cstr);
#अन्यथा
            m3ret_t r = सततम्_संहिता(m3code, राशिः, रिक्त, d_m3_Op_Default_तर्काः);
#यदि_अन्तः

            यदि (r == 0) {                                                                   m3log (runtime, "expression result: %s", SPrint_मानम् (राशिः, i_type));
                यदि (आकार_सम्बन्धि_Type(आधारः_Type_सम्बन्धि(i_type)) == परिमाण(u32)) {
                    *(u32*)o_expressed = *((u32*)राशिः);
                } अन्यथा {
                    *(u64*)o_expressed = *((u64*)राशिः);
                }
            }
        }

        Release_संहिता_Page(&runtime, o->page);
    } अन्यथा {
        परिणाम = m3_Err_malloc_Failed_संहिता_Page;
    }

    runtime.origin_राशिः = रिक्त;        // prevent free(stack) in ReleaseRuntime
    Runtime_Release(&runtime);
    i_module->runtime = savedRuntime;

    *io_अष्टकानि = o->wasm;

    प्रत्यागम परिणाम;
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
स्थिर
bool LimitsSatisfy (u64 i_exported_आकार, bool i_exportedHasMax, u64 i_exportedMax,
                    u64 i_importMin, bool i_importHasMax, u64 i_importMax)
{
    यदि (i_exported_आकार < i_importMin) {
        प्रत्यागम false;
    }

    यदि (i_importHasMax and (not i_exportedHasMax or i_exportedMax > i_importMax)) {
        प्रत्यागम false;
    }

    प्रत्यागम true;
}


// The export of this kind under this name, or NULL. The index it carries is
// resolved by the caller, against the module's index space as it stands then -
// after linking, a re-exported import's slot holds the entity it was linked to.
स्थिर
ध्रुव M3Export* Module_FindExport (IM3Module i_module, u8 i_विधा, cstr_t i_name)
{
    प्रति (u32 i = 0; i < i_module->अङ्कः_Exports; ++i) {
        ध्रुव M3Export* e = &i_module->exports[i];

        यदि (e->विधा == i_विधा and वाक्यं_तुलय(e->name, i_name) == 0) {
            प्रत्यागम e;
        }
    }

    प्रत्यागम रिक्त;
}


स्थिर
IM3Function Module_FindExportedFunction (IM3Module i_module, cstr_t i_name)
{
    ध्रुव M3Export* e = Module_FindExport(i_module, d_external_विधा_function, i_name);

    // A module that re-exports an import names the placeholder here. Resolve to
    // the function that actually runs, or a host-side call would run it against
    // the importing module's memory.
    प्रत्यागम e ? Function_Implementation(&i_module->functions[e->index]) : रिक्त;
}


स्थिर
IM3_स्मृतिः Module_Find_Exported_स्मृतिः (IM3Module i_module, cstr_t i_name)
{
    ध्रुव M3Export* e = Module_FindExport(i_module, d_external_विधा_स्मृतिः, i_name);

    प्रत्यागम e ? i_module->memories[e->index] : रिक्त;
}


स्थिर
IM3_सारणी Module_Find_Exported_सारणी (IM3Module i_module, cstr_t i_name)
{
    ध्रुव M3Export* e = Module_FindExport(i_module, d_external_विधा_सारणी, i_name);

    प्रत्यागम e ? i_module->tables[e->index] : रिक्त;
}


स्थिर
IM3Global Module_FindExportedGlobal (IM3Module i_module, cstr_t i_name)
{
    ध्रुव M3Export* e = Module_FindExport(i_module, d_external_विधा_global, i_name);

    प्रत्यागम e ? &i_module->globals[e->index] : रिक्त;
}


#यदि d_m3HasExceptionHandling || d_m3_Has_राशिः_Switching
स्थिर
IM3_चिह्नकम् Module_Find_Exported_चिह्नकम् (IM3Module i_module, cstr_t i_name)
{
    ध्रुव M3Export* e = Module_FindExport(i_module, d_external_विधा_चिह्नकम्, i_name);

    यदि (not e) {
        प्रत्यागम रिक्त;
    }

    IM3_चिह्नकम् चिह्नकम् = &i_module->चिह्नकानि[e->index];

    प्रत्यागम चिह्नकम्->resolved ? चिह्नकम्->resolved : चिह्नकम्;
}
#यदि_अन्तः


// Whether the module exports anything at all under this name. Export names are
// unique within a module, so a name one of the lookups above missed but this
// one finds is exported as something else - a kind the import cannot be
// satisfied by, rather than a name the module never exported.
स्थिर
bool Module_HasExport (IM3Module i_module, cstr_t i_name)
{
    प्रति (u32 i = 0; i < i_module->अङ्कः_Exports; ++i) {
        यदि (वाक्यं_तुलय(i_module->exports[i].name, i_name) == 0) {
            प्रत्यागम true;
        }
    }

    प्रत्यागम false;
}


// Points each of the module's imports at whatever already-loaded module exports
// it. Runs before anything is allocated or initialized: a memory import has to
// be resolved before InitMemory would give it pages of its own, and a global
// import before InitGlobals runs an initializer that reads it.
स्थिर
M3_परिणाम LinkImports (IM3Runtime io_runtime, IM3Module io_module)
{
    M3_परिणाम परिणाम = m3Err_none;

    प्रति (u32 i = 0; i < io_module->अङ्कः_Functions; ++i) {
        IM3Function f = &io_module->functions[i];

        यदि (f->wasm or not (f->import.moduleUtf8 and f->import.क्षेत्रम्_Utf8)) {
            अनुवर्त;
        }

        IM3Module from = m3_FindModule(io_runtime, f->import.moduleUtf8);
        यदि (not from) {
            अनुवर्त;
        }

        IM3Function exported = Module_FindExportedFunction(from, f->import.क्षेत्रम्_Utf8);
        यदि (not exported) {
            // the import names a module that is loaded, so its exports settle the
            // question: a name it exports as something else is a type mismatch, and
            // one it does not export at all is an unknown import
            _throwif(m3Err_incompatibleImportType, Module_HasExport(from, f->import.क्षेत्रम्_Utf8));
            _throw(m3Err_unknownImport);
        }

        // func types are canonical within an environment, so this is the
        // structural equivalence the spec asks for
        _throwif(m3Err_incompatibleImportType, exported->funcType != f->funcType);

        f->resolved = Function_Implementation(exported);
    }

    प्रति (u32 i = 0; i < io_module->अङ्कः_Memories; ++i) {
        IM3_स्मृतिः स्मृतिः = io_module->memories[i];

        यदि (not स्मृतिः->imported or स्मृतिः->owner != io_module) {
            अनुवर्त;
        }

        IM3Module from = m3_FindModule(io_runtime, स्मृतिः->import.moduleUtf8);
        यदि (not from) {
            अनुवर्त;
        }

        IM3_स्मृतिः exported = Module_Find_Exported_स्मृतिः(from, स्मृतिः->import.क्षेत्रम्_Utf8);
        यदि (not exported) {
            _throwif(m3Err_incompatibleImportType, Module_HasExport(from, स्मृतिः->import.क्षेत्रम्_Utf8));
            _throw(m3Err_unknownImport);
        }

        // the address type is part of the memory type, so an i64 memory does
        // not satisfy an i32 import, or the other way round - and so is the page
        // size, which custom page sizes made a declared property of a memory
        _throwif(m3Err_incompatibleImportType, exported->isMemory64 != स्मृतिः->isMemory64);
        _throwif(m3Err_incompatibleImportType, स्मृतिः_Page_आकार(exported) != स्मृतिः_Page_आकार(स्मृतिः));

        // a shared memory is not a subtype of an unshared one, or the other way round
        _throwif(m3Err_incompatibleImportType, exported->isShared != स्मृतिः->isShared);

        _throwif(m3Err_incompatibleImportType,
                 not LimitsSatisfy(exported->अङ्कः_पृष्ठानि, exported->hasMax, exported->max_पृष्ठानि,
                                   स्मृतिः->init_पृष्ठानि, स्मृतिः->hasMax, स्मृतिः->max_पृष्ठानि));

        // hand the slot over to the exporter's memory, and drop the placeholder
        Free_स्मृतिः_खण्डम्(स्मृतिः);
        FreeImportInfo(&स्मृतिः->import);
        m3_Free(स्मृतिः);

        io_module->memories[i] = exported;
    }

    प्रति (u32 i = 0; i < io_module->अङ्कः_Tables; ++i) {
        IM3_सारणी सारणी_2 = io_module->tables[i];

        यदि (not सारणी_2->imported or सारणी_2->owner != io_module) {
            अनुवर्त;
        }

        IM3Module from = m3_FindModule(io_runtime, सारणी_2->import.moduleUtf8);
        यदि (not from) {
            अनुवर्त;
        }

        IM3_सारणी exported = Module_Find_Exported_सारणी(from, सारणी_2->import.क्षेत्रम्_Utf8);
        यदि (not exported) {
            _throwif(m3Err_incompatibleImportType, Module_HasExport(from, सारणी_2->import.क्षेत्रम्_Utf8));
            _throw(m3Err_unknownImport);
        }

        _throwif(m3Err_incompatibleImportType, exported->type != सारणी_2->type);

        // the index type is part of the table type, the same way it is for a memory
        _throwif(m3Err_incompatibleImportType, exported->isTable64 != सारणी_2->isTable64);

        _throwif(m3Err_incompatibleImportType,
                 not LimitsSatisfy(exported->आकार, exported->hasMax, exported->max_आकार,
                                   सारणी_2->init_आकार, सारणी_2->hasMax, सारणी_2->max_आकार));

        // hand the slot over to the exporter's table, and drop the placeholder
        m3_Free(सारणी_2->elements);
        FreeImportInfo(&सारणी_2->import);
        m3_Free(सारणी_2);

        io_module->tables[i] = exported;
    }

    प्रति (u32 i = 0; i < io_module->अङ्कः_Globals; ++i) {
        IM3Global g = &io_module->globals[i];

        यदि (not g->imported or not (g->import.moduleUtf8 and g->import.क्षेत्रम्_Utf8)) {
            अनुवर्त;
        }

        IM3Module from = m3_FindModule(io_runtime, g->import.moduleUtf8);
        यदि (not from) {
            अनुवर्त;
        }

        IM3Global exported = Module_FindExportedGlobal(from, g->import.क्षेत्रम्_Utf8);
        यदि (not exported) {
            _throwif(m3Err_incompatibleImportType, Module_HasExport(from, g->import.क्षेत्रम्_Utf8));
            _throw(m3Err_unknownImport);
        }

        _throwif(m3Err_incompatibleImportType, exported->type != g->type);
        _throwif(m3Err_incompatibleImportType, exported->isMutable != g->isMutable);

        g->resolved = exported->resolved ? exported->resolved : exported;
    }

#यदि d_m3HasExceptionHandling || d_m3_Has_राशिः_Switching
    प्रति (u32 i = 0; i < io_module->अङ्कः_चिह्नकानि; ++i) {
        IM3_चिह्नकम् चिह्नकम् = &io_module->चिह्नकानि[i];

        यदि (not चिह्नकम्->imported or not (चिह्नकम्->import.moduleUtf8 and चिह्नकम्->import.क्षेत्रम्_Utf8)) {
            अनुवर्त;
        }

        IM3Module from = m3_FindModule(io_runtime, चिह्नकम्->import.moduleUtf8);
        यदि (not from) {
            अनुवर्त;
        }

        IM3_चिह्नकम् exported = Module_Find_Exported_चिह्नकम्(from, चिह्नकम्->import.क्षेत्रम्_Utf8);
        यदि (not exported) {
            _throwif(m3Err_incompatibleImportType, Module_HasExport(from, चिह्नकम्->import.क्षेत्रम्_Utf8));
            _throw(m3Err_unknownImport);
        }

        _throwif(m3Err_incompatibleImportType, exported->type != चिह्नकम्->type);

        चिह्नकम्->resolved = exported->resolved ? exported->resolved : exported;
    }
#यदि_अन्तः

    _catch: प्रत्यागम परिणाम;
}


// Backs each of the module's memories with pages. LinkImports has already
// pointed any import it could satisfy at the exporting module's memory, and
// those are skipped here. An import nothing satisfied is still backed locally
// from its own declared limits, so that the module remains loadable.
M3_परिणाम Init_स्मृतिः (IM3Runtime io_runtime, IM3Module i_module)
{
    M3_परिणाम परिणाम = m3Err_none;

    // Fixed from here on: the index space stops changing after parse, and
    // linking has already repointed any slot it was going to.
    i_module->memory0 = i_module->अङ्कः_Memories ? i_module->memories[0]
                                              : &i_module->रिक्तानि_स्मृतिः;

    यदि (i_module->अङ्कः_Memories == 0) {
        // nothing addressable, but _mem still has to point somewhere
        i_module->रिक्तानि_स्मृतिः.owner    = i_module;
        i_module->रिक्तानि_स्मृतिः.page_आकार = d_m3_Default_Mem_Page_आकार;

_       (Resize_स्मृतिः(io_runtime, &i_module->रिक्तानि_स्मृतिः, 0));
    }

    प्रति (u32 i = 0; i < i_module->अङ्कः_Memories; ++i) {
        IM3_स्मृतिः स्मृतिः = i_module->memories[i];

        // a slot that already points at another module's memory is that
        // module's to allocate
        यदि (स्मृतिः->owner != i_module or स्मृतिः->mallocated) {
            अनुवर्त;
        }

        u32 page_आकार = स्मृतिः_Page_आकार(स्मृतिः);

        स्मृतिः->page_आकार = page_आकार;

        // Without a declared maximum a memory may grow to the spec limit of
        // 2^|addrtype|/pagesize pages, which is the usual 65536 at the default
        // page size, 2^48 for a 64-bit memory, and a whole address space of
        // them when a page is a single byte. A declared maximum of zero is a
        // real limit, not the absence of one.
        //
        // 2^64/pagesize overflows a u64 only when a page is a single byte, and
        // a power-of-two page size divides the address space exactly, so the
        // 64-bit division below is 2^64/pagesize written so that it fits.
        यदि (not स्मृतिः->hasMax) {
            स्मृतिः->max_पृष्ठानि = स्मृतिः->isMemory64
                                 ? (page_आकार > 1 ? (UINT64_MAX / page_आकार) + 1 : UINT64_MAX)
                                 : (0x100000000ull / page_आकार);
        }

_       (Resize_स्मृतिः(io_runtime, स्मृतिः, स्मृतिः->init_पृष्ठानि));
    }

    _catch: प्रत्यागम परिणाम;
}


M3_परिणाम Resize_स्मृतिः (IM3Runtime io_runtime, IM3_स्मृतिः स्मृतिः, u64 i_अङ्कः_पृष्ठानि)
{
    M3_परिणाम परिणाम = m3Err_none;

    u64 अङ्कः_पृष्ठानि_गन्तव्यम्_Alloc = i_अङ्कः_पृष्ठानि;

    यदि (अङ्कः_पृष्ठानि_गन्तव्यम्_Alloc <= स्मृतिः->max_पृष्ठानि) {
        // A 64-bit memory may ask for up to 2^48 pages, which overflows a u64
        // of bytes. Nothing that large can be backed, so refuse it up front
        // rather than multiplying into a wrapped size.
        _throwif("linear memory limitation exceeded",
                 अङ्कः_पृष्ठानि_गन्तव्यम्_Alloc > d_m3_स्थानाङ्कः_सीमा / स्मृतिः->page_आकार);

        u64 अङ्कः_Page_अष्टकानि = अङ्कः_पृष्ठानि_गन्तव्यम्_Alloc * स्मृतिः->page_आकार;

#यदि d_m3_Max_रैखिकम्_स्मृतिः_पृष्ठानि > 0
        // the limit is a memory size, counted in default-sized pages; comparing
        // it against a raw page count would make it 65536 times stricter for a
        // module whose pages are one byte
        _throwif("linear memory limitation exceeded",
                 अङ्कः_Page_अष्टकानि > (u64)d_m3_Max_रैखिकम्_स्मृतिः_पृष्ठानि * d_m3_Default_Mem_Page_आकार);
#यदि_अन्तः

        // Limit the amount of memory that gets actually allocated
        यदि (io_runtime->स्मृतिः_सीमा) {
            अङ्कः_Page_अष्टकानि = M3_MIN(अङ्कः_Page_अष्टकानि, (u64)io_runtime->स्मृतिः_सीमा);
        }

        u64 old_दीर्घता = स्मृतिः->mallocated ? स्मृतिः->mallocated->दीर्घता : 0;
        u64 other_प्रयुक्तम् = io_runtime->स्मृतिः_अष्टकानि_प्रयुक्तम् - old_दीर्घता;
        _throwif(m3_Err_स्मृतिः_सीमा_Exceeded, io_runtime->स्मृतिः_अष्टकानि_सीमा and
                                              (अङ्कः_Page_अष्टकानि > io_runtime->स्मृतिः_अष्टकानि_सीमा or
                                               other_प्रयुक्तम् > io_runtime->स्मृतिः_अष्टकानि_सीमा - अङ्कः_Page_अष्टकानि));

        _throwif("linear memory limitation exceeded", अङ्कः_Page_अष्टकानि > (u64)SIZE_MAX - परिमाण(M3_स्मृतिः_शीर्षकम्));

#यदि d_m3_Guarded_स्मृतिः

        // A slot holds every address a Wasm access can name, and nothing beyond
        // that: a memory larger than the data part of one would put addresses past
        // the guard, where a fault is nobody's to recognize
        _throwif("linear memory limitation exceeded", अङ्कः_Page_अष्टकानि > d_m3_Guarded_दत्त_अष्टकानि);

        // What makes an access past the end fault is the page after the memory not
        // being committed, so the end has to fall on one of the system's page
        // boundaries. A Wasm page divides evenly into them and always will; a memory
        // that declared a page size of its own may not, and then the bytes between
        // the end of the memory and the end of its last system page would answer
        // instead of trapping. Refusing to back such a memory is the honest way out:
        // the custom page sizes proposal exists for targets far too small to reserve
        // 8GiB per memory in the first place. See d_m3GuardedMemory.
        _throwif("guarded memory needs a page size the system's divides into",
                 (स्मृतिः->page_आकार % m3_Host_Page_आकार()) != 0);

        यदि (स्मृतिः->guardSlot == रिक्त) {
            स्मृतिः->guardSlot = Guard_TakeSlot();
            _throwifnull(स्मृतिः->guardSlot);
        }

        // Growing only ever commits more of the same slot, so the memory never
        // moves - which the interpreter does not rely on, but nothing minds
        _throwif(m3Err_mallocFailed, not Guard_CommitSlot(स्मृतिः->guardSlot, (size_t)अङ्कः_Page_अष्टकानि));

        स्मृतिः->mallocated = Guard_Slot_शीर्षकम्(स्मृतिः->guardSlot);

#अन्यथा

        size_t अङ्कः_अष्टकानि = (size_t)अङ्कः_Page_अष्टकानि + परिमाण(M3_स्मृतिः_शीर्षकम्);

        // What is really there, which is not numPages * pageSize: the limit above
        // caps the bytes without capping the page count, so a memory can be short
        // of a page - by design, since a whole 64 KiB page is often more than an
        // MCU has. Measuring the old block by the page count would overstate it,
        // and then realloc never sees the size it already has and copies the
        // whole limit again on every grow past it.
        size_t अङ्कः_Previous_अष्टकानि = स्मृतिः->mallocated ? स्मृतिः->mallocated->दीर्घता + परिमाण(M3_स्मृतिः_शीर्षकम्) : 0;

        शून्य* newMem = m3_Realloc("Wasm Linear Memory", स्मृतिः->mallocated, अङ्कः_अष्टकानि, अङ्कः_Previous_अष्टकानि);
        _throwifnull(newMem);

        स्मृतिः->mallocated = (M3_स्मृतिः_शीर्षकम्*)newMem;

#यदि_अन्तः

#यदि d_m3LogRuntime
        M3_स्मृतिः_शीर्षकम्* oldMallocated = स्मृतिः->mallocated;
#यदि_अन्तः

        io_runtime->स्मृतिः_अष्टकानि_प्रयुक्तम् = other_प्रयुक्तम् + अङ्कः_Page_अष्टकानि;
        स्मृतिः->अङ्कः_पृष्ठानि            = अङ्कः_पृष्ठानि_गन्तव्यम्_Alloc;

        स्मृतिः->mallocated->दीर्घता  = (size_t)अङ्कः_Page_अष्टकानि;
        स्मृतिः->mallocated->runtime = io_runtime;
        स्मृतिः->mallocated->स्मृतिः  = स्मृतिः;

        स्मृतिः->mallocated->max_राशिः = (m3slot_t*)io_runtime->origin_राशिः + io_runtime->अङ्कः_राशिः_Slots;

        m3log(runtime, "resized old: %p; mem: %p; length: %zu; pages: %llu", oldMallocated, स्मृतिः->mallocated, स्मृतिः->mallocated->दीर्घता, (अचिह्नित दीर्घ दीर्घ)स्मृतिः->अङ्कः_पृष्ठानि);
    } अन्यथा {
        परिणाम = m3_Err_wasm_स्मृतिः_अतिप्रवाहः;
    }

    _catch: प्रत्यागम परिणाम;
}


शून्य Free_स्मृतिः_खण्डम् (IM3_स्मृतिः io_स्मृतिः)
{
    यदि (io_स्मृतिः->mallocated and io_स्मृतिः->mallocated->runtime) {
        io_स्मृतिः->mallocated->runtime->स्मृतिः_अष्टकानि_प्रयुक्तम् -= io_स्मृतिः->mallocated->दीर्घता;
    }
#यदि d_m3_Guarded_स्मृतिः
    Guard_GiveSlot(io_स्मृतिः->guardSlot);
    io_स्मृतिः->guardSlot = रिक्त;

    io_स्मृतिः->mallocated = रिक्त;
#अन्यथा
    m3_Free(io_स्मृतिः->mallocated);
#यदि_अन्तः
}


M3_परिणाम InitGlobals (IM3Module io_module)
{
    M3_परिणाम परिणाम = m3Err_none;

    यदि (io_module->अङ्कः_Globals) {
        // placing the globals in their structs isn't good for cache locality, but i don't really know what the global
        // access patterns typically look like yet.

        //          io_module->globalMemory = m3Alloc (m3reg_t, io_module->numGlobals);

        //          if (io_module->globalMemory)
        {
            प्रति (u32 i = 0; i < io_module->अङ्कः_Globals; ++i) {
                M3Global* g = &io_module->globals[i];                           m3log (runtime, "initializing global: %d", i);

                यदि (g->init_व्यञ्जकम्) {
                    अष्टकानि_t start = g->init_व्यञ्जकम्;

                    परिणाम = EvaluateExpression(io_module, &g->i64_मानम्, g->type, &start, g->init_व्यञ्जकम् + g->init_व्यञ्जकम्_आकार);

                    यदि (not परिणाम) {
                        // io_module->globalMemory [i] = initValue;
                    } अन्यथा {
                        भङ्ग;
                    }
                } अन्यथा {                                                        m3log (runtime, "importing global");
                }
            }
        }
        //          else result = ErrorModule (m3Err_mallocFailed, io_module, "could allocate globals for module: '%s", io_module->name);
    }

    प्रत्यागम परिणाम;
}


M3_परिणाम Init_दत्त_Segments (IM3Module io_module)
{
    M3_परिणाम परिणाम = m3Err_none;

    प्रति (u32 i = 0; i < io_module->अङ्कः_दत्त_Segments; ++i) {
        M3_दत्त_Segment* segment = &io_module->दत्त_Segments[i];

        // A passive segment stays available for memory.init until data.drop.
        // An active one is copied here and then counts as dropped.
        यदि (segment->isPassive) {
            अनुवर्त;
        }

        _throwif("data segment memory index out of range",
                 segment->स्मृतिः_Region >= io_module->अङ्कः_Memories);

        IM3_स्मृतिः io_स्मृतिः = io_module->memories[segment->स्मृतिः_Region];

        _throwif("unallocated linear memory", !(io_स्मृतिः->mallocated));

        // The offset expression has the memory's address type, and is
        // unsigned: an i32 offset of -1 is 4294967295, way out of bounds
        // rather than negative.
        u64     segment_विस्थापनम् = 0;
        अष्टकानि_t start         = segment->init_व्यञ्जकम्;

        यदि (io_स्मृतिः->isMemory64) {
_           (EvaluateExpression(io_module, &segment_विस्थापनम्, c_m3Type_i64, &start, segment->init_व्यञ्जकम् + segment->init_व्यञ्जकम्_आकार));
        } अन्यथा {
            u32 offset32;
_           (EvaluateExpression(io_module, &offset32, c_m3Type_i32, &start, segment->init_व्यञ्जकम् + segment->init_व्यञ्जकम्_आकार));
            segment_विस्थापनम् = offset32;
        }

        m3log(runtime, "loading data segment: %d; size: %d; offset: %llu", i, segment->आकार, (अचिह्नित दीर्घ दीर्घ)segment_विस्थापनम्);

        यदि (segment_विस्थापनम् <= io_स्मृतिः->mallocated->दीर्घता &&
            (u64)segment->आकार <= io_स्मृतिः->mallocated->दीर्घता - segment_विस्थापनम्) {
            u8* dest = m3_Mem_दत्त(io_स्मृतिः->mallocated) + segment_विस्थापनम्;
            स्मृतिं_प्रतिलिख(dest, segment->दत्त, segment->आकार);
        } अन्यथा {
            _throw("data segment out of bounds");
        }

        segment->dropped = true;
    }

    _catch: प्रत्यागम परिणाम;
}


// Turns a segment's elements into references. Element expressions are constant
// expressions restricted to ref.null/ref.func, so they're read directly rather
// than run through the compiler.
स्थिर
M3_परिणाम ResolveElements (IM3Module io_module, M3ElementSegment* i_segment, शून्य** o_elements)
{
    M3_परिणाम परिणाम = m3Err_none;

    अष्टकानि_t  pos = i_segment->elements;
    cbytes_t अन्तः = io_module->element_Section_अन्तः;

    प्रति (u32 e = 0; e < i_segment->अङ्कः_Elements; ++e) {
        u32   func_सूचकांक;
        शून्य* ref = रिक्त;

        यदि (i_segment->is_व्यञ्जकम्) {
            m3opcode_t opcode;
_           (Read_opcode(&opcode, &pos, अन्तः));

            यदि (opcode == c_waOp_refFunc) {
_               (ReadLEB_u32(&func_सूचकांक, &pos, अन्तः));
                _throwif("function index out of range", func_सूचकांक >= io_module->अङ्कः_Functions);
                ref = Function_Implementation(&io_module->functions[func_सूचकांक]);
            } अन्यथा यदि (opcode == c_waOp_refNull) {
                i8 waType;
                u8 nullType;
_               (ReadLEB_i7(&waType, &pos, अन्तः));
_               (NormalizeType(&nullType, waType));
                _throwif(m3Err_typeMismatch, nullType != i_segment->type);
            } अन्यथा यदि (opcode == c_waOp_getGlobal) {
                // wasm 2.0 lets an element expression read an imported
                // immutable global, which is how one module seeds another's
                // table with a reference it exported.
                u32 global_सूचकांक;
_               (ReadLEB_u32(&global_सूचकांक, &pos, अन्तः));
                _throwif(m3_Err_globa_सूचकांक_बहिर्गमनम्_सम्बन्धि_Bounds, global_सूचकांक >= io_module->अङ्कः_Globals);

                IM3Global global = &io_module->globals[global_सूचकांक];

                _throwif(m3_Err_globa_सूचकांक_बहिर्गमनम्_सम्बन्धि_Bounds, not global->imported);
                _throwif(m3Err_wasmMalformed, global->isMutable);
                _throwif(m3Err_typeMismatch, आधारः_Type_सम्बन्धि(global->type) != आधारः_Type_सम्बन्धि(i_segment->type));

                // read the cell the import was linked to, not the placeholder
                यदि (global->resolved) {
                    global = global->resolved;
                }

                ref = global->ref_मानम्;
            } अन्यथा {
                _throw("constant expression required");
            }

_           (Read_opcode(&opcode, &pos, अन्तः));
            _throwif(m3Err_wasmMalformed, opcode != c_wa_Op_अन्तः);
        } अन्यथा {
_           (ReadLEB_u32(&func_सूचकांक, &pos, अन्तः));
            _throwif("function index out of range", func_सूचकांक >= io_module->अङ्कः_Functions);
            ref = Function_Implementation(&io_module->functions[func_सूचकांक]);
        }

        o_elements[e] = ref;
    }

    _catch: प्रत्यागम परिणाम;
}


M3_परिणाम Init_सारणी_And_Elements (IM3Module io_module)
{
    M3_परिणाम परिणाम = m3Err_none;

    cbytes_t अन्तः = io_module->element_Section_अन्तः;
    M3_सारणी* सारणी_2;

    प्रति (u32 i = 0; i < io_module->अङ्कः_Tables; ++i) {
        सारणी_2 = io_module->tables[i];

        // a slot pointing at another module's table is that module's to fill
        यदि (सारणी_2->owner != io_module) {
            अनुवर्त;
        }

        यदि (सारणी_2->आकार) {
            IM3Runtime runtime = io_module->runtime;
            _throwif(m3_Err_सारणी_सीमा_Exceeded, runtime->सारणी_Elements_सीमा and
                                                 सारणी_2->आकार > runtime->सारणी_Elements_सीमा - runtime->सारणी_Elements_प्रयुक्तम्);
            सारणी_2->elements = m3_AllocArray(शून्य*, सारणी_2->आकार);
            _throwifnull(सारणी_2->elements);
            runtime->सारणी_Elements_प्रयुक्तम् += सारणी_2->आकार;

            यदि (सारणी_2->init_व्यञ्जकम्) {
                शून्य*   मानम् = रिक्त;
                अष्टकानि_t start = सारणी_2->init_व्यञ्जकम्;
_               (EvaluateExpression(io_module, &मानम्, आधारः_Type_सम्बन्धि(सारणी_2->type),
                                     &start, सारणी_2->init_व्यञ्जकम् + सारणी_2->init_व्यञ्जकम्_आकार));

                प्रति (u32 e = 0; e < सारणी_2->आकार; ++e) {
                    सारणी_2->elements[e] = मानम्;
                }
            }
        }
    }

    प्रति (u32 i = 0; i < io_module->अङ्कः_Element_Segments; ++i) {
        M3ElementSegment* segment = &io_module->elementSegments[i];

        // Declarative segments only make their functions referenceable, and
        // passive ones wait for table.init, so neither is written out here.
        यदि (segment->बहुलकम् == c_m3Elem_declarative) {
            segment->dropped = true;
            अनुवर्त;
        }

        यदि (segment->बहुलकम् == c_m3Elem_passive) {
            यदि (segment->अङ्कः_Elements) {
                segment->resolved = m3_AllocArray(शून्य*, segment->अङ्कः_Elements);
                _throwifnull(segment->resolved);
_               (ResolveElements(io_module, segment, segment->resolved));
            }
            अनुवर्त;
        }

        सारणी_2 = io_module->tables[segment->सारणी_सूचकांक];

        // The offset expression has the table's index type, and is unsigned:
        // an i32 offset of -1 is 4294967295, out of bounds rather than negative.
        u64     offset = 0;
        अष्टकानि_t व्यञ्जकम्   = segment->init_व्यञ्जकम्;

        यदि (सारणी_2->isTable64) {
_           (EvaluateExpression(io_module, &offset, c_m3Type_i64, &व्यञ्जकम्, अन्तः));
        } अन्यथा {
            u32 offset32;
_           (EvaluateExpression(io_module, &offset32, c_m3Type_i32, &व्यञ्जकम्, अन्तः));
            offset = offset32;
        }

        _throwif("out of bounds table access",
                 offset > सारणी_2->आकार or segment->अङ्कः_Elements > सारणी_2->आकार - offset);

_       (ResolveElements(io_module, segment, सारणी_2->elements + offset));

        segment->dropped = true;
    }

    _catch: प्रत्यागम परिणाम;
}

M3_परिणाम m3_CompileModule (IM3Module io_module)
{
    M3_परिणाम परिणाम = m3Err_none;

    प्रति (u32 i = 0; i < io_module->अङ्कः_Functions; ++i) {
        IM3Function f = &io_module->functions[i];
        यदि (f->wasm and not f->compiled) {
_           (CompileFunction(f));
        }
    }

    _catch: प्रत्यागम परिणाम;
}

#यदि d_m3HasExceptionHandling

M3Exception* NewException (IM3Runtime io_runtime, IM3_चिह्नकम् i_चिह्नकम्, u32 i_अङ्कः_तर्काः)
{
    M3Exception* exception = (M3Exception*)m3_Malloc("M3Exception", परिमाण(M3Exception) + i_अङ्कः_तर्काः * परिमाण(u64));

    यदि (exception) {
        exception->चिह्नकम्     = i_चिह्नकम्;
        exception->अङ्कः_तर्काः = i_अङ्कः_तर्काः;
        exception->reified = false;
        exception->पूर्व    = रिक्त;
        exception->अग्र    = io_runtime->exceptions;

        यदि (exception->अग्र) {
            exception->अग्र->पूर्व = exception;
        }

        io_runtime->exceptions = exception;
    }

    प्रत्यागम exception;
}


// Releases one exception ahead of the rest. The caller has to know nothing can
// still name it: no exnref was ever taken of it, and its payload has already
// been copied out.
शून्य FreeException (IM3Runtime io_runtime, M3Exception* i_exception)
{
    यदि (i_exception->पूर्व) {
        i_exception->पूर्व->अग्र = i_exception->अग्र;
    } अन्यथा {
        io_runtime->exceptions = i_exception->अग्र;
    }

    यदि (i_exception->अग्र) {
        i_exception->अग्र->पूर्व = i_exception->पूर्व;
    }

    यदि (io_runtime->pendingException == i_exception) {
        io_runtime->pendingException = रिक्त;
    }

    m3_Free_Impl(i_exception);
}


// Releases every exception the runtime still holds. Only safe once the Wasm
// stack is empty, which is why the outermost RunCodeChecked() is the one that
// calls it.
शून्य FreeExceptions (IM3Runtime io_runtime)
{
    M3Exception* exception = io_runtime->exceptions;

    io_runtime->exceptions       = रिक्त;
    io_runtime->pendingException = रिक्त;

    यावत् (exception) {
        M3Exception* अग्र = exception->अग्र;
        m3_Free_Impl(exception);
        exception = अग्र;
    }
}


// Keeps a copy of the exception a call just ended with, for m3_GetExceptionTag,
// or forgets the last one when the call ended any other way. A copy, because
// the original is freed with the rest before the outermost call returns - and
// one a host function's nested call ended with may still be named by an
// exnref further down the stack.
स्थिर
शून्य लेखः_Uncaught_Exception (IM3Runtime io_runtime, M3_परिणाम i_परिणाम)
{
    यदि (io_runtime->uncaughtException) {
        m3_Free(io_runtime->uncaughtException);
    }

    M3Exception* exception = io_runtime->pendingException;

    यदि (M3_UNLIKELY(i_परिणाम == m3Err_trapUncaughtException) and exception) {
        M3Exception* copy = (M3Exception*)m3_CopyMem(exception, परिमाण(M3Exception) + exception->अङ्कः_तर्काः * परिमाण(u64));

        यदि (copy) {
            copy->अग्र = रिक्त;
            copy->पूर्व = रिक्त;
        }

        io_runtime->uncaughtException = copy;
    }
}

#यदि_अन्तः // d_m3HasExceptionHandling


#यदि d_m3HasExceptionHandling || d_m3_Has_राशिः_Switching

IM3_चिह्नकम् m3_Find_चिह्नकम् (IM3Module i_module, ध्रुव वर्ण* ध्रुव i_चिह्नकम्_Name)
{
    यदि (not i_module or not i_चिह्नकम्_Name) {
        प्रत्यागम रिक्त;
    }

    प्रत्यागम Module_Find_Exported_चिह्नकम्(i_module, i_चिह्नकम्_Name);
}


M3_परिणाम m3_New_चिह्नकम् (IM3Runtime io_runtime, IM3_चिह्नकम्* o_चिह्नकम्, ध्रुव वर्ण* ध्रुव i_signature)
{
    IM3FuncType type    = रिक्त;
    M3_Host_चिह्नकम्*  host_चिह्नकम् = रिक्त;

_try {
    _throwif("null tag", not o_चिह्नकम्);
    *o_चिह्नकम् = रिक्त;
    _throwif(m3Err_mallocFailed, not io_runtime);

_   (Signature_गन्तव्यम्_Func_Type(&type, i_signature));
    // tags are the same type exactly when their types are the same object
_   (Environment_सङ्कलय_Func_Type(io_runtime->environment, &type));

    host_चिह्नकम् = m3_AllocStruct(M3_Host_चिह्नकम्);
    _throwifnull(host_चिह्नकम्);

    host_चिह्नकम्->चिह्नकम्.type    = type;
    host_चिह्नकम्->अग्र        = io_runtime->host_चिह्नकानि;
    io_runtime->host_चिह्नकानि = host_चिह्नकम्;

    *o_चिह्नकम् = &host_चिह्नकम्->चिह्नकम्;
} _catch:
    प्रत्यागम परिणाम;
}


M3_परिणाम m3_Link_चिह्नकम् (IM3Module         io_module,
                     ध्रुव वर्ण* ध्रुव i_moduleName,
                     ध्रुव वर्ण* ध्रुव i_चिह्नकम्_Name,
                     IM3_चिह्नकम्            i_चिह्नकम्)
{
    यदि (not io_module or not i_moduleName or not i_चिह्नकम्_Name or not i_चिह्नकम्) {
        प्रत्यागम m3_Err_चिह्नकम्_Lookup_Failed;
    }

    ध्रुव bool wildcardModule = (वाक्यं_तुलय(i_moduleName, "*") == 0);
    IM3_चिह्नकम्     target         = i_चिह्नकम्->resolved ? i_चिह्नकम्->resolved : i_चिह्नकम्;

    M3_परिणाम परिणाम = m3_Err_चिह्नकम्_Lookup_Failed;

    प्रति (u32 i = 0; i < io_module->अङ्कः_चिह्नकानि; ++i) {
        IM3_चिह्नकम् चिह्नकम् = &io_module->चिह्नकानि[i];

        यदि (चिह्नकम्->imported and चिह्नकम्->import.moduleUtf8 and चिह्नकम्->import.क्षेत्रम्_Utf8 and
            वाक्यं_तुलय(चिह्नकम्->import.क्षेत्रम्_Utf8, i_चिह्नकम्_Name) == 0 and
            (wildcardModule or वाक्यं_तुलय(चिह्नकम्->import.moduleUtf8, i_moduleName) == 0)) {
            यदि (चिह्नकम्->type != target->type) {
                प्रत्यागम m3Err_incompatibleImportType;
            }

            चिह्नकम्->resolved = target;
            परिणाम        = m3Err_none;
        }
    }

    प्रत्यागम परिणाम;
}


uint32_t m3_Get_चिह्नकम्_Arg_गणनम् (IM3_चिह्नकम् i_चिह्नकम्)
{
    प्रत्यागम i_चिह्नकम् ? i_चिह्नकम्->type->अङ्कः_तर्काः : 0;
}


M3_मानम्_Type m3_Get_चिह्नकम्_Arg_Type (IM3_चिह्नकम् i_चिह्नकम्, uint32_t i_सूचकांक)
{
    यदि (i_चिह्नकम् and i_सूचकांक < i_चिह्नकम्->type->अङ्कः_तर्काः) {
        प्रत्यागम (M3_मानम्_Type)आधारः_Type_सम्बन्धि(d_FuncArgType(i_चिह्नकम्->type, i_सूचकांक));
    }

    प्रत्यागम c_m3Type_none;
}

#अन्यथा

IM3_चिह्नकम् m3_Find_चिह्नकम् (IM3Module i_module, ध्रुव वर्ण* ध्रुव i_चिह्नकम्_Name)
{
    प्रत्यागम रिक्त;
}

M3_परिणाम m3_New_चिह्नकम् (IM3Runtime io_runtime, IM3_चिह्नकम्* o_चिह्नकम्, ध्रुव वर्ण* ध्रुव i_signature)
{
    यदि (o_चिह्नकम्) {
        *o_चिह्नकम् = रिक्त;
    }
    प्रत्यागम m3_Err_चिह्नकम्_Lookup_Failed;
}

M3_परिणाम m3_Link_चिह्नकम् (IM3Module io_module, ध्रुव वर्ण* ध्रुव i_moduleName, ध्रुव वर्ण* ध्रुव i_चिह्नकम्_Name, IM3_चिह्नकम् i_चिह्नकम्)
{
    प्रत्यागम m3_Err_चिह्नकम्_Lookup_Failed;
}

uint32_t m3_Get_चिह्नकम्_Arg_गणनम् (IM3_चिह्नकम् i_चिह्नकम्)
{
    प्रत्यागम 0;
}

M3_मानम्_Type m3_Get_चिह्नकम्_Arg_Type (IM3_चिह्नकम् i_चिह्नकम्, uint32_t i_सूचकांक)
{
    प्रत्यागम c_m3Type_none;
}

#यदि_अन्तः // d_m3HasExceptionHandling || d_m3HasStackSwitching


#यदि d_m3HasExceptionHandling

M3_परिणाम m3_ThrowException (IM3Runtime io_runtime, IM3_चिह्नकम् i_चिह्नकम्, uint32_t i_argc, ध्रुव शून्य* i_argptrs[])
{
    यदि (not io_runtime or not i_चिह्नकम्) {
        प्रत्यागम m3_Err_चिह्नकम्_Lookup_Failed;
    }

    IM3_चिह्नकम्      चिह्नकम्  = i_चिह्नकम्->resolved ? i_चिह्नकम्->resolved : i_चिह्नकम्;
    IM3FuncType type = चिह्नकम्->type;

    // a tag with results is a stack switching control tag, which is suspended
    // with rather than thrown
    यदि (type->अङ्कः_Rets) {
        प्रत्यागम m3Err_typeMismatch;
    }
    यदि (i_argc != type->अङ्कः_तर्काः) {
        प्रत्यागम m3_Err_तर्कः_गणनम्_Mismatch;
    }
    // with nothing on the stack there is nothing to unwind, and the exception
    // would sit on the runtime's list until some later call ended
    यदि (io_runtime->callNesting == 0) {
        प्रत्यागम "m3_ThrowException called outside a host function";
    }

    M3Exception* exception = NewException(io_runtime, चिह्नकम्, i_argc);
    यदि (not exception) {
        प्रत्यागम m3Err_mallocFailed;
    }

    // the layout op_Throw gives a payload: 32-bit values zero-extended, floats
    // as their bit pattern
    प्रति (u32 i = 0; i < i_argc; ++i) {
        ध्रुव शून्य* मानम् = i_argptrs[i];

        विकल्प (आधारः_Type_सम्बन्धि(d_FuncArgType(type, i))) {
        पक्ष c_m3Type_i32:
        पक्ष c_m3Type_f32: {
            u32 द्वयङ्काः;
            स्मृतिं_प्रतिलिख(&द्वयङ्काः, मानम्, परिमाण(द्वयङ्काः));
            exception->तर्काः[i] = द्वयङ्काः;
            भङ्ग;
        }
        पक्ष c_m3Type_i64:
        पक्ष c_m3Type_f64:
            स्मृतिं_प्रतिलिख(&exception->तर्काः[i], मानम्, परिमाण(u64));
            भङ्ग;
        पक्ष c_m3Type_funcref:
        पक्ष c_m3Type_externref:
        पक्ष c_m3Type_exnref:
        पक्ष c_m3Type_contref:
            exception->तर्काः[i] = (u64)(*(ध्रुव uintptr_t*)मानम्);
            भङ्ग;
        स्वभाव:
            FreeException(io_runtime, exception);
            प्रत्यागम "unknown argument type";
        }
    }

    io_runtime->pendingException = exception;

    प्रत्यागम m3Err_pendingException;
}


IM3_चिह्नकम् m3_Get_Exception_चिह्नकम् (IM3Runtime i_runtime)
{
    यदि (i_runtime and i_runtime->uncaughtException) {
        प्रत्यागम i_runtime->uncaughtException->चिह्नकम्;
    }

    प्रत्यागम रिक्त;
}


M3_परिणाम m3_Get_Exception_तर्काः (IM3Runtime i_runtime, uint32_t i_argc, ध्रुव शून्य* o_argptrs[])
{
    यदि (not i_runtime or not i_runtime->uncaughtException) {
        प्रत्यागम "no uncaught exception";
    }

    ध्रुव M3Exception* exception = i_runtime->uncaughtException;
    IM3FuncType        type      = exception->चिह्नकम्->type;

    यदि (i_argc != exception->अङ्कः_तर्काः) {
        प्रत्यागम m3_Err_तर्कः_गणनम्_Mismatch;
    }

    प्रति (u32 i = 0; i < i_argc; ++i) {
        शून्य* मानम् = (शून्य*)o_argptrs[i];
        u64   द्वयङ्काः  = exception->तर्काः[i];

        विकल्प (आधारः_Type_सम्बन्धि(d_FuncArgType(type, i))) {
        पक्ष c_m3Type_i32:
        पक्ष c_m3Type_f32: {
            u32 नीचः = (u32)द्वयङ्काः;
            स्मृतिं_प्रतिलिख(मानम्, &नीचः, परिमाण(नीचः));
            भङ्ग;
        }
        पक्ष c_m3Type_i64:
        पक्ष c_m3Type_f64:
            स्मृतिं_प्रतिलिख(मानम्, &द्वयङ्काः, परिमाण(द्वयङ्काः));
            भङ्ग;
        पक्ष c_m3Type_funcref:
        पक्ष c_m3Type_externref:
        पक्ष c_m3Type_exnref:
        पक्ष c_m3Type_contref:
            *(uintptr_t*)मानम् = (uintptr_t)द्वयङ्काः;
            भङ्ग;
        स्वभाव:
            प्रत्यागम "unknown argument type";
        }
    }

    प्रत्यागम m3Err_none;
}

#अन्यथा

M3_परिणाम m3_ThrowException (IM3Runtime io_runtime, IM3_चिह्नकम् i_चिह्नकम्, uint32_t i_argc, ध्रुव शून्य* i_argptrs[])
{
    प्रत्यागम m3_Err_चिह्नकम्_Lookup_Failed;
}

IM3_चिह्नकम् m3_Get_Exception_चिह्नकम् (IM3Runtime i_runtime)
{
    प्रत्यागम रिक्त;
}

M3_परिणाम m3_Get_Exception_तर्काः (IM3Runtime i_runtime, uint32_t i_argc, ध्रुव शून्य* o_argptrs[])
{
    प्रत्यागम "no uncaught exception";
}

#यदि_अन्तः // d_m3HasExceptionHandling


#यदि d_m3_Guarded_स्मृतिः

// What RunCodeProtected hands to the platform layer to run, and what comes back
प्रकारनिर्देश संरचना M3_Protected_सततम् {
    pc_t            pc;
    IM3Runtime      runtime;
    M3_स्मृतिः_शीर्षकम्* mem;
    M3_परिणाम        परिणाम;
} M3_Protected_सततम्;

स्थिर
शून्य सततम्_संहिता_Body (शून्य* io_context)
{
    M3_Protected_सततम्* सततम् = (M3_Protected_सततम्*)io_context;

#  यदि (d_m3EnableOpProfiling || d_m3EnableOpTracing)
    सततम्->परिणाम = (M3_परिणाम)सततम्_संहिता(सततम्->pc, (m3stack_t)सततम्->runtime->राशिः, सततम्->mem,
                                    d_m3_Op_Default_तर्काः, d_m3_आधारः_Cstr);
#  अन्यथा
    सततम्->परिणाम = (M3_परिणाम)सततम्_संहिता(सततम्->pc, (m3stack_t)सततम्->runtime->राशिः, सततम्->mem,
                                    d_m3_Op_Default_तर्काः);
#  endif
}

// The bounds check the load and store operations no longer do, done once here by the
// system instead: an address past the end of a memory reaches the uncommitted part
// of that memory's reservation, and the fault comes back as the trap the spec asks
// for. Nothing is left half-done by it - the Wasm stack and the memories are heap,
// and the caller unwinds the rest.
स्थिर
M3_परिणाम सततम्_संहिता_Protected (IM3Runtime i_runtime, pc_t i_pc, M3_स्मृतिः_शीर्षकम्* i_mem)
{
    // Memory operations in this build carry no bounds check, so code only runs where a
    // fault past the end of a memory can still be turned into a trap
    यदि (M3_UNLIKELY(not m3_Host_Guards_सक्रिय())) {
        प्रत्यागम ErrorRuntime(m3_Err_स्मृतिः_Guards_Unavailable, i_runtime,
                            "another part of the process holds the fault signal");
    }

    M3_Protected_सततम् सततम्;
    सततम्.pc      = i_pc;
    सततम्.runtime = i_runtime;
    सततम्.mem     = i_mem;
    सततम्.परिणाम  = m3Err_none;

    शून्य*  arena;
    size_t arena_अष्टकानि;
    Guard_ArenaRange(&arena, &arena_अष्टकानि);

    यदि (not m3_HostProtectedCall(सततम्_संहिता_Body, &सततम्, arena, arena_अष्टकानि)) {
        प्रत्यागम m3_Err_trap_बहिर्गमनम्_सम्बन्धि_Bounds_स्मृतिः_Access;
    }

    प्रत्यागम सततम्.परिणाम;
}

#यदि_अन्तः // d_m3GuardedMemory


#यदि d_m3_Has_राशिः_Switching

// The function a suspended continuation stopped in: the callee of its innermost
// call frame, or the function it was entered with if it made no call. Frames are
// recorded innermost first, so that is the lowest-numbered call frame.
स्थिर
IM3Function Continuation_InnermostFunction (IM3Continuation i_cont)
{
    प्रति (u32 i = 0; i < i_cont->अङ्कः_चौकटाः; ++i) {
        यदि (i_cont->चौकटाः[i].विधा == frame_call && i_cont->चौकटाः[i].call.function) {
            प्रत्यागम i_cont->चौकटाः[i].call.function;
        }
    }

    प्रत्यागम i_cont->प्रविष्टिः_Function;
}


// Where the host's next call puts its arguments and finds its results. A
// suspended root keeps its frames on the bottom of the runtime's stack, so a call
// made before it is resumed starts above the last of them rather than on top of
// them. The top is the innermost function's frame base plus everything that
// function can use. That base is the root's own sp, unless it stopped inside a
// resume: then the suspension point belongs to the resumed continuation, and the
// root's innermost frame is that resume.
शून्य Runtime_Place_Call_राशिः (IM3Runtime io_runtime)
{
    IM3Continuation मूलम् = io_runtime->मूलम्_Continuation;

    io_runtime->राशिः = io_runtime->origin_राशिः;

    यदि (not मूलम् or मूलम्->state != cont_suspended) {
        प्रत्यागम;
    }

    m3stack_t   sp       = मूलम्->sp;
    IM3Function function = Continuation_InnermostFunction(मूलम्);

    यदि (मूलम्->अङ्कः_चौकटाः and मूलम्->चौकटाः[0].विधा == frame_resume) {
        sp = मूलम्->चौकटाः[0].sp;
    }

    m3slot_t* आधारः = (m3slot_t*)io_runtime->origin_राशिः;
    m3slot_t* शिखरम्  = (m3slot_t*)sp + (function ? function->max_राशिः_Slots : 0);

    यदि (शिखरम् > आधारः + io_runtime->अङ्कः_राशिः_Slots) {
        शिखरम् = आधारः + io_runtime->अङ्कः_राशिः_Slots;
    }

    io_runtime->राशिः = शिखरम्;
}

#यदि_अन्तः // d_m3HasStackSwitching


// Run compiled code on the runtime's stack, bounding native recursion for the
// duration of the call. The outermost invocation establishes the stack limit;
// nested ones (an imported function calling back into Wasm) inherit it.
स्थिर अन्तःस्थ
M3_परिणाम सततम्_संहिता_Checked (IM3Runtime i_runtime, IM3Function i_function)
{
    pc_t i_pc = i_function->compiled;

    // execution runs against the memory of the module the entry point belongs
    // to, not against some runtime-wide one
    M3_स्मृतिः_शीर्षकम्* _mem = Module_स्मृतिः_शीर्षकम्(i_function->module);

#यदि d_m3HasSnapshots
    यदि (M3_UNLIKELY(i_function->module->isUnusable)) {
        प्रत्यागम "a snapshot failed to restore into the module, so it cannot run";
    }
    i_function->module->has_सततम् = true;
#यदि_अन्तः

    d_m3_राशिः_सीमा_Enter(i_runtime);

    // a host function calling back into Wasm does not change which call a trap
    // ends: that is still the one the host made first
    यदि (i_runtime->callNesting++ == 0) {
        i_runtime->entered = i_function;
    }
#यदि d_m3HasExceptionHandling
    // handler stacks don't nest across a call boundary: a host function calling
    // back into Wasm cannot be caught by a try_table its own caller entered
    u32 saved_Try_गभीरता   = i_runtime->try_गभीरता;
    i_runtime->try_गभीरता = 0;
#यदि_अन्तः
#यदि d_m3_Has_राशिः_Switching
    // nor do continuations: the native frames on the far side of a host call
    // are not ours to record, so a suspend in here has no handler to reach and
    // no resume to unwind to
    IM3Continuation savedContinuation = i_runtime->सक्रिय_Continuation;

    // A suspended root continuation is a paused program waiting to be resumed,
    // not scratch space. A call the host makes before resuming it gets a
    // context of its own, the way any other re-entry does, rather than having
    // the frames of the suspension reset out from under it.
    bool claim_मूलम् = i_runtime->isSuspendable and not savedContinuation and
                     not (i_runtime->मूलम्_Continuation and
                          i_runtime->मूलम्_Continuation->state == cont_suspended);

    यदि (claim_मूलम् and not i_runtime->मूलम्_Continuation) {
        i_runtime->मूलम्_Continuation = Continuation_New(i_runtime, i_function->funcType, i_function);

        यदि (i_runtime->मूलम्_Continuation) {
            // The root borrows the runtime's stack and is not charged as cont.new.
            i_runtime->मूलम्_Continuation->अङ्कः_राशिः_Slots = i_runtime->अङ्कः_राशिः_Slots;
        } अन्यथा {
            claim_मूलम् = false;
        }
    }

    यदि (claim_मूलम्) {
        i_runtime->सक्रिय_Continuation              = i_runtime->मूलम्_Continuation;
        i_runtime->मूलम्_Continuation->प्रविष्टिः_Function = i_function;
        i_runtime->मूलम्_Continuation->state         = cont_running;
        i_runtime->मूलम्_Continuation->अङ्कः_चौकटाः     = 0;
    } अन्यथा {
        i_runtime->सक्रिय_Continuation = रिक्त;
    }
#यदि_अन्तः
#यदि d_m3_Guarded_स्मृतिः
    M3_परिणाम परिणाम = सततम्_संहिता_Protected(i_runtime, i_pc, _mem);
#नोचेद्यदि (d_m3EnableOpProfiling || d_m3EnableOpTracing)
    M3_परिणाम परिणाम = (M3_परिणाम)सततम्_संहिता(i_pc, (m3stack_t)i_runtime->राशिः, _mem, d_m3_Op_Default_तर्काः, d_m3_आधारः_Cstr);
#अन्यथा
    M3_परिणाम परिणाम = (M3_परिणाम)सततम्_संहिता(i_pc, (m3stack_t)i_runtime->राशिः, _mem, d_m3_Op_Default_तर्काः);
#यदि_अन्तः
#यदि d_m3_Has_राशिः_Switching
    यदि (claim_मूलम्) {
        i_runtime->मूलम्_Continuation->state =
          (परिणाम == m3Err_continuationSuspended) ? cont_suspended : cont_returned;

        Runtime_Place_Call_राशिः(i_runtime);
    }
    i_runtime->सक्रिय_Continuation = savedContinuation;
#यदि_अन्तः
#यदि d_m3HasExceptionHandling
    i_runtime->try_गभीरता = saved_Try_गभीरता;

    // an exception that reached the bottom of the call stack found no handler
    यदि (M3_UNLIKELY(परिणाम == m3Err_pendingException)) {
        परिणाम = m3Err_trapUncaughtException;
    }
    लेखः_Uncaught_Exception(i_runtime, परिणाम);

    // a paused invocation can still reach the exceptions it holds, so they wait
    // for it to finish
    यदि (i_runtime->callNesting == 1 and not m3_IsSuspended(i_runtime)) {
        FreeExceptions(i_runtime);
    }
#यदि_अन्तः
    i_runtime->callNesting--;
    d_m3_राशिः_सीमा_Leave(i_runtime);

    प्रत्यागम परिणाम;
}

M3_परिणाम m3_सततम्_Start (IM3Module io_module)
{
#यदि_परिभाषितम् FUZZING_BUILD_बहुलकम्_UNSAFE_FOR_PRODUCTION
    // Execution disabled for fuzzing builds
    प्रत्यागम m3Err_none;
#यदि_अन्तः

    M3_परिणाम परिणाम           = m3Err_none;
    i32      startFunctionTmp = -1;

    यदि (io_module and io_module->startFunction >= 0) {
        IM3Function function = &io_module->functions[io_module->startFunction];

        यदि (not function->compiled) {
_           (CompileFunction(function));
        }

        IM3FuncType ftype = function->funcType;
        यदि (ftype->अङ्कः_तर्काः != 0 || ftype->अङ्कः_Rets != 0) {
            _throw(m3_Err_तर्कः_गणनम्_Mismatch);
        }

        IM3Module  module  = function->module;
        IM3Runtime runtime = module->runtime;

        startFunctionTmp = io_module->startFunction;

        io_module->startFunction = -1;

        परिणाम = सततम्_संहिता_Checked(runtime, function);

        // a start function that suspended is not to be started again: resuming
        // is what finishes it
        यदि (परिणाम and परिणाम != m3Err_continuationSuspended) {
            io_module->startFunction = startFunctionTmp;
            EXCEPTION_PRINT(परिणाम);
            गच्छ _catch;
        }
    }

    _catch: प्रत्यागम परिणाम;
}

// TODO: deal with main + side-modules loading efforcement
M3_परिणाम m3_आरोपय_Module (IM3Runtime io_runtime, IM3Module io_module)
{
    M3_परिणाम परिणाम = m3Err_none;

    यदि (M3_UNLIKELY(io_module->runtime)) {
        प्रत्यागम m3Err_moduleAlreadyLinked;
    }

    io_module->runtime = io_runtime;

    // linking first: a memory import has to be resolved before InitMemory would
    // give it pages of its own, and a global import before an initializer reads it
_   (LinkImports(io_runtime, io_module));

_   (Init_स्मृतिः(io_runtime, io_module));
_   (InitGlobals(io_module));
    // Spec order: element segments are applied before data segments. It matters
    // when one of them traps - whatever ran before the trap stays done.
_   (Init_सारणी_And_Elements(io_module));
_   (Init_दत्त_Segments(io_module));

    // Start func might use imported functions, which are not liked here yet,
    // so it will be called before a function call is attempted (in m3_FindFunction)

#यदि_परिभाषितम् DEBUG
    Module_Generate_नामानि(io_module);
#यदि_अन्तः

    io_module->अग्र     = io_runtime->modules;
    io_runtime->modules = io_module;
    प्रत्यागम परिणाम; // ok

_catch:
    // The runtime owns the module either way. Instantiation may already have
    // written this module's functions into a table another module owns, and the
    // spec keeps whatever it managed to do before the trap, so those entries
    // stay callable - retaining the module is what stops them dangling.
    //
    // It goes on the tail of the list rather than the head: it is not a module
    // anyone should find by name, only one whose functions may still be
    // reachable through someone else's table.
    io_module->अग्र = रिक्त;

    IM3Module* पुच्छ = &io_runtime->modules;
    यावत् (*पुच्छ) {
        पुच्छ = &(*पुच्छ)->अग्र;
    }
    *पुच्छ = io_module;

    प्रत्यागम परिणाम;
}

IM3Global m3_FindGlobal (IM3Module         io_module,
                         ध्रुव वर्ण* ध्रुव i_globalName)
{
    // Search exports
    IM3Global exported = Module_FindExportedGlobal(io_module, i_globalName);
    यदि (exported) {
        // a re-exported global is the one that was imported, so reads and
        // writes have to reach the cell that actually holds the value
        प्रत्यागम exported->resolved ? exported->resolved : exported;
    }

    // Search imports
    प्रति (u32 i = 0; i < io_module->अङ्कः_Globals; ++i) {
        IM3Global g = &io_module->globals[i];

        यदि (g->import.moduleUtf8 and g->import.क्षेत्रम्_Utf8) {
            यदि (वाक्यं_तुलय(g->import.क्षेत्रम्_Utf8, i_globalName) == 0) {
                प्रत्यागम g->resolved ? g->resolved : g;
            }
        }
    }
    प्रत्यागम रिक्त;
}

M3_परिणाम m3_GetGlobal (IM3Global      i_global,
                       IM3_Tagged_मानम् o_मानम्)
{
    यदि (not i_global) {
        प्रत्यागम m3Err_globalLookupFailed;
    }

    विकल्प (i_global->type) {
    पक्ष c_m3Type_i32: o_मानम्->मानम्.i32 = i_global->i32_मानम्; भङ्ग;
    पक्ष c_m3Type_i64: o_मानम्->मानम्.i64 = i_global->i64_मानम्; भङ्ग;
#यदि d_m3HasFloat
    पक्ष c_m3Type_f32: o_मानम्->मानम्.f32 = i_global->f32_मानम्; भङ्ग;
    पक्ष c_m3Type_f64: o_मानम्->मानम्.f64 = i_global->f64_मानम्; भङ्ग;
#यदि_अन्तः
    स्वभाव: प्रत्यागम m3Err_invalidTypeId;
    }

    o_मानम्->type = (M3_मानम्_Type)(i_global->type);
    प्रत्यागम m3Err_none;
}

M3_परिणाम m3_समुच्चयः_Global (IM3Global            i_global,
                       ध्रुव IM3_Tagged_मानम् i_मानम्)
{
    यदि (not i_global) {
        प्रत्यागम m3Err_globalLookupFailed;
    }
    यदि (not i_global->isMutable) {
        प्रत्यागम m3Err_globalNotMutable;
    }
    यदि (i_global->type != i_मानम्->type) {
        प्रत्यागम m3Err_globalTypeMismatch;
    }

    विकल्प (i_मानम्->type) {
    पक्ष c_m3Type_i32: i_global->i32_मानम् = i_मानम्->मानम्.i32; भङ्ग;
    पक्ष c_m3Type_i64: i_global->i64_मानम् = i_मानम्->मानम्.i64; भङ्ग;
#यदि d_m3HasFloat
    पक्ष c_m3Type_f32: i_global->f32_मानम् = i_मानम्->मानम्.f32; भङ्ग;
    पक्ष c_m3Type_f64: i_global->f64_मानम् = i_मानम्->मानम्.f64; भङ्ग;
#यदि_अन्तः
    स्वभाव: प्रत्यागम m3Err_invalidTypeId;
    }

    प्रत्यागम m3Err_none;
}

M3_मानम्_Type m3_GetGlobalType (IM3Global i_global)
{
    प्रत्यागम (i_global) ? (M3_मानम्_Type)(i_global->type) : c_m3Type_none;
}


शून्य* v_FindFunction (IM3Module i_module, शून्य* i_info)
{
    ध्रुव वर्ण* ध्रुव i_name = (ध्रुव वर्ण*)i_info;

    // Prefer exported functions
    IM3Function exported = Module_FindExportedFunction(i_module, i_name);
    यदि (exported) {
        प्रत्यागम exported;
    }

    // Search internal functions
    प्रति (u32 i = 0; i < i_module->अङ्कः_Functions; ++i) {
        IM3Function f = &i_module->functions[i];

        bool isImported = f->import.moduleUtf8 or f->import.क्षेत्रम्_Utf8;

        यदि (isImported) {
            अनुवर्त;
        }

        प्रति (पूर्णाङ्क j = 0; j < f->अङ्कः_नामानि; j++) {
            यदि (f->नामानि[j] and वाक्यं_तुलय(f->नामानि[j], i_name) == 0) {
                प्रत्यागम f;
            }
        }
    }

    प्रत्यागम रिक्त;
}


// Shared tail of the two lookups: a function is only usable once it has code.
स्थिर
M3_परिणाम Prepare_प्राप्तम्_Function (IM3Function* o_function, IM3Function i_function)
{
    M3_परिणाम परिणाम = m3Err_none;

    यदि (not i_function->compiled) {
_       (CompileFunction(i_function))
    }

    _catch:
    *o_function = परिणाम ? रिक्त : i_function;

    प्रत्यागम परिणाम;
}


// Searches every module in the runtime, most recently loaded first. That is a
// guess once more than one module is loaded and two of them export the same
// name - m3_FindFunctionIn says which module is meant.
M3_परिणाम m3_FindFunction (IM3Function* o_function, IM3Runtime i_runtime, ध्रुव वर्ण* ध्रुव i_functionName)
{
                                                                d_m3Assert (o_function and i_runtime and i_functionName);
    IM3Function function = रिक्त;

    यदि (not i_runtime->modules) {
        *o_function = रिक्त;
        प्रत्यागम "no modules loaded";
    }

    function = (IM3Function)ForEachModule(i_runtime, v_FindFunction, (शून्य*)i_functionName);

    यदि (not function) {
        *o_function = रिक्त;
        प्रत्यागम ErrorModule(m3Err_functionLookupFailed, i_runtime->modules, "'%s'", i_functionName);
    }

    प्रत्यागम Prepare_प्राप्तम्_Function(o_function, function);
}


IM3Module m3_FindModule (IM3Runtime i_runtime, ध्रुव वर्ण* ध्रुव i_moduleName)
{
    यदि (not i_runtime or not i_moduleName) {
        प्रत्यागम रिक्त;
    }

    // the list is newest-first, so a name registered twice names the newer one
    प्रति (IM3Module m = i_runtime->modules; m; m = m->अग्र) {
        यदि (m->name and वाक्यं_तुलय(m->name, i_moduleName) == 0) {
            प्रत्यागम m;
        }
    }

    प्रत्यागम रिक्त;
}


// Searches one module's exports, which is what naming a module means.
M3_परिणाम m3_Find_Function_अन्तर् (IM3Function* o_function, IM3Module i_module, ध्रुव वर्ण* ध्रुव i_functionName)
{
                                                                d_m3Assert (o_function and i_functionName);
    यदि (not i_module) {
        *o_function = रिक्त;
        प्रत्यागम m3Err_functionLookupFailed;      // ErrorModule would deref it
    }

    IM3Function function = (IM3Function)v_FindFunction(i_module, (शून्य*)i_functionName);

    यदि (not function) {
        *o_function = रिक्त;
        प्रत्यागम ErrorModule(m3Err_functionLookupFailed, i_module, "'%s'", i_functionName);
    }

    प्रत्यागम Prepare_प्राप्तम्_Function(o_function, function);
}


M3_परिणाम m3_Get_सारणी_Function (IM3Function* o_function, IM3Module i_module, uint32_t i_सूचकांक)
{
_try {
    M3_सारणी*    सारणी_2;
    IM3Function function;

    _throwif("no table", i_module->अङ्कः_Tables == 0);

    सारणी_2 = i_module->tables[0];
    _throwif("function index out of range", i_सूचकांक >= सारणी_2->आकार);

    function = (IM3Function)सारणी_2->elements[i_सूचकांक];

    यदि (function) {
        यदि (not function->compiled) {
_           (CompileFunction(function))
        }
    }

    *o_function = function;
} _catch:
    प्रत्यागम परिणाम;
}


स्थिर
M3_परिणाम checkStartFunction (IM3Module i_module)
{
    M3_परिणाम परिणाम = m3Err_none;                               d_m3Assert(i_module);

    // Check if start function needs to be called
    यदि (i_module->startFunction >= 0) {
        परिणाम = m3_सततम्_Start(i_module);
    }

    प्रत्यागम परिणाम;
}

uint32_t m3_Get_Arg_गणनम् (IM3Function i_function)
{
    यदि (i_function) {
        IM3FuncType ft = i_function->funcType;
        यदि (ft) {
            प्रत्यागम ft->अङ्कः_तर्काः;
        }
    }
    प्रत्यागम 0;
}

uint32_t m3_Get_Ret_गणनम् (IM3Function i_function)
{
    यदि (i_function) {
        IM3FuncType ft = i_function->funcType;
        यदि (ft) {
            प्रत्यागम ft->अङ्कः_Rets;
        }
    }
    प्रत्यागम 0;
}


M3_मानम्_Type m3_GetArgType (IM3Function i_function, uint32_t index)
{
    यदि (i_function) {
        IM3FuncType ft = i_function->funcType;
        यदि (ft and index < ft->अङ्कः_तर्काः) {
            प्रत्यागम (M3_मानम्_Type)आधारः_Type_सम्बन्धि(d_FuncArgType(ft, index));
        }
    }
    प्रत्यागम c_m3Type_none;
}

M3_मानम्_Type m3_GetRetType (IM3Function i_function, uint32_t index)
{
    यदि (i_function) {
        IM3FuncType ft = i_function->funcType;
        यदि (ft and index < ft->अङ्कः_Rets) {
            प्रत्यागम (M3_मानम्_Type)आधारः_Type_सम्बन्धि(d_FuncRetType(ft, index));
        }
    }
    प्रत्यागम c_m3Type_none;
}


// The call APIs marshal their arguments into the runtime stack before any
// compiled code runs, so op_Entry's overflow check comes too late to cover those
// writes - and for a host function or an import linked to another module there
// is no op_Entry of our own to reach at all. Bound the area about to be written
// here instead: results and arguments each take a u64 at this boundary, however
// wide the slot is inside. Every caller goes through this, so none of them can
// be the one that forgets.
स्थिर
M3_परिणाम Get_राशिः_Pointer_For_तर्काः (IM3Function i_function, u8** o_राशिः)
{
    IM3Runtime  runtime = i_function->module->runtime;
    IM3FuncType ftype   = i_function->funcType;

    // the stack a paused invocation holds is not available to this one
    size_t प्रयुक्तम्      = (size_t)((u8*)runtime->राशिः - (u8*)runtime->origin_राशिः);
    size_t needed    = ((size_t)ftype->अङ्कः_Rets + ftype->अङ्कः_तर्काः) * परिमाण(u64);
    size_t उपलब्धम् = (size_t)runtime->अङ्कः_राशिः_Slots * परिमाण(m3slot_t) - प्रयुक्तम्;

    यदि (needed > उपलब्धम्) {
        प्रत्यागम m3_Err_trap_राशिः_अतिप्रवाहः;
    }

    u64* राशिः = (u64*)runtime->राशिः;

    राशिः += ftype->अङ्कः_Rets;

    *o_राशिः = (u8*)राशिः;

    प्रत्यागम m3Err_none;
}


M3_परिणाम m3_CallV (IM3Function i_function, ...)
{
    va_list ap;
    va_start(ap, i_function);
    M3_परिणाम r = m3_CallVL(i_function, ap);
    va_end(ap);
    प्रत्यागम r;
}

स्थिर
शून्य Report_Native_राशिः_Usage ()
{
#यदि d_m3_Log_Native_राशिः
    पूर्णाङ्क राशिः_प्रयुक्तम् = m3_राशिः_Get_Max();
    fprintf(stderr, "Native stack used: %d\n", राशिः_प्रयुक्तम्);
#यदि_अन्तः
}


M3_परिणाम m3_CallVL (IM3Function i_function, va_list i_तर्काः)
{
    IM3Runtime  runtime = i_function->module->runtime;
    IM3FuncType ftype   = i_function->funcType;
    M3_परिणाम    परिणाम  = m3Err_none;
    u8*         s       = रिक्त;

    यदि (!i_function->compiled) {
        प्रत्यागम m3_Err_missing_Compiled_संहिता;
    }

#यदि d_m3_लेखः_Backtraces
    ClearBacktrace(runtime);
#यदि_अन्तः

    m3_राशिः_Check_Init();

_   (checkStartFunction(i_function->module))

_   (Get_राशिः_Pointer_For_तर्काः(i_function, &s))

    प्रति (u32 i = 0; i < ftype->अङ्कः_तर्काः; ++i) {
        विकल्प (d_FuncArgType(ftype, i)) {
        पक्ष c_m3Type_i32:
            *(i32*)(s) = va_arg(i_तर्काः, i32);
            s += 8;
            भङ्ग;
        पक्ष c_m3Type_i64:
            *(i64*)(s) = va_arg(i_तर्काः, i64);
            s += 8;
            भङ्ग;
        पक्ष c_m3Type_funcref:
        पक्ष c_m3Type_externref:
        पक्ष c_m3Type_exnref:
            *(uintptr_t*)(s) = va_arg(i_तर्काः, uintptr_t);
            s += 8;
            भङ्ग;
#यदि d_m3HasFloat
        पक्ष c_m3Type_f32:
            *(f32*)(s) = (f32)va_arg(i_तर्काः, f64);
            s += 8;
            भङ्ग; // f32 is passed as f64
        पक्ष c_m3Type_f64:
            *(f64*)(s) = va_arg(i_तर्काः, f64);
            s += 8;
            भङ्ग;
#यदि_अन्तः
        स्वभाव: प्रत्यागम "unknown argument type";
        }
    }

    परिणाम = सततम्_संहिता_Checked(runtime, i_function);
    Report_Native_राशिः_Usage();

    runtime->चरमः_Called = परिणाम ? रिक्त : i_function;

    _catch: प्रत्यागम परिणाम;
}

M3_परिणाम m3_Call (IM3Function i_function, uint32_t i_argc, ध्रुव शून्य* i_argptrs[])
{
    IM3Runtime  runtime = i_function->module->runtime;
    IM3FuncType ftype   = i_function->funcType;
    M3_परिणाम    परिणाम  = m3Err_none;
    u8*         s       = रिक्त;

    यदि (i_argc != ftype->अङ्कः_तर्काः) {
        प्रत्यागम m3_Err_तर्कः_गणनम्_Mismatch;
    }
    यदि (!i_function->compiled) {
        प्रत्यागम m3_Err_missing_Compiled_संहिता;
    }

#यदि d_m3_लेखः_Backtraces
    ClearBacktrace(runtime);
#यदि_अन्तः

    m3_राशिः_Check_Init();

_   (checkStartFunction(i_function->module))

_   (Get_राशिः_Pointer_For_तर्काः(i_function, &s))

    प्रति (u32 i = 0; i < ftype->अङ्कः_तर्काः; ++i) {
        विकल्प (d_FuncArgType(ftype, i)) {
        पक्ष c_m3Type_i32:
            *(i32*)(s) = *(i32*)i_argptrs[i];
            s += 8;
            भङ्ग;
        पक्ष c_m3Type_i64:
            *(i64*)(s) = *(i64*)i_argptrs[i];
            s += 8;
            भङ्ग;
        पक्ष c_m3Type_funcref:
        पक्ष c_m3Type_externref:
        पक्ष c_m3Type_exnref:
            *(uintptr_t*)(s) = *(uintptr_t*)i_argptrs[i];
            s += 8;
            भङ्ग;
#यदि d_m3HasFloat
        पक्ष c_m3Type_f32:
            *(f32*)(s) = *(f32*)i_argptrs[i];
            s += 8;
            भङ्ग;
        पक्ष c_m3Type_f64:
            *(f64*)(s) = *(f64*)i_argptrs[i];
            s += 8;
            भङ्ग;
#यदि_अन्तः
        स्वभाव: प्रत्यागम "unknown argument type";
        }
    }

    परिणाम = सततम्_संहिता_Checked(runtime, i_function);

    Report_Native_राशिः_Usage();

    runtime->चरमः_Called = परिणाम ? रिक्त : i_function;

    _catch: प्रत्यागम परिणाम;
}

// Argument parsing for m3_CallArgv. Strict on purpose: the whole string has to
// be consumed, so "12abc" is rejected rather than read as 12, and an empty or
// unparsable argument is an error rather than the zero that strtoul with a
// NULL end pointer used to hand back. See wasm3/wasm3#367.
स्थिर
M3_परिणाम ParseArgInteger (ccstr_t i_arg, u32 i_अङ्कः_द्वयङ्काः, u64* o_मानम्)
{
    यदि (not i_arg or not *i_arg) {
        प्रत्यागम "empty argument";
    }

    // strtoull would skip leading space, but trailing space is rejected below;
    // accepting one and not the other would just be confusing
    यदि (रिक्तम्_वा((अचिह्नित वर्ण)*i_arg)) {
        प्रत्यागम "argument is not a number";
    }

    वर्ण* अन्तः = रिक्त;
    u64   मानम्;

    errno = 0;

    // an argument may be spelled signed or unsigned: -1 and 4294967295 name the
    // same i32
    यदि (*i_arg == '-') {
        i64 signed_मानम् = strtoll(i_arg, &अन्तः, 10);

        यदि (i_अङ्कः_द्वयङ्काः == 32 and (signed_मानम् < INT32_MIN or signed_मानम् > INT32_MAX)) {
            प्रत्यागम "argument out of range";
        }

        मानम् = (u64)signed_मानम्;
    } अन्यथा {
        मानम् = strtoull(i_arg, &अन्तः, 10);

        यदि (i_अङ्कः_द्वयङ्काः == 32 and मानम् > UINT32_MAX) {
            प्रत्यागम "argument out of range";
        }
    }

    यदि (errno == ERANGE) {
        प्रत्यागम "argument out of range";
    }

    यदि (अन्तः == i_arg or *अन्तः) {
        प्रत्यागम "argument is not a number";
    }

    *o_मानम् = मानम्;

    प्रत्यागम m3Err_none;
}


#यदि d_m3HasFloat
स्थिर
M3_परिणाम ParseArgFloat (ccstr_t i_arg, f64* o_मानम्)
{
    यदि (not i_arg or not *i_arg) {
        प्रत्यागम "empty argument";
    }

    यदि (रिक्तम्_वा((अचिह्नित वर्ण)*i_arg)) {
        प्रत्यागम "argument is not a number";
    }

    वर्ण* अन्तः = रिक्त;

    errno = 0;

    f64 मानम् = strtod(i_arg, &अन्तः);

    यदि (अन्तः == i_arg or *अन्तः) {
        प्रत्यागम "argument is not a number";
    }

    // strtod reports underflow through ERANGE as well, and a denormal result is
    // perfectly usable, so only an overflow to infinity is out of range
    यदि (errno == ERANGE and (मानम् > DBL_MAX or मानम् < -DBL_MAX)) {
        प्रत्यागम "argument out of range";
    }

    *o_मानम् = मानम्;

    प्रत्यागम m3Err_none;
}
#यदि_अन्तः


// A reference argument is either the null reference or a host handle written as
// an integer.
स्थिर
M3_परिणाम ParseArgReference (ccstr_t i_arg, u64* o_मानम्)
{
    यदि (i_arg and वाक्यं_तुलय(i_arg, "null") == 0) {
        *o_मानम् = 0;
        प्रत्यागम m3Err_none;
    }

    प्रत्यागम ParseArgInteger(i_arg, 64, o_मानम्);
}


M3_परिणाम m3_CallArgv (IM3Function i_function, uint32_t i_argc, ध्रुव वर्ण* i_argv[])
{
    IM3FuncType ftype   = i_function->funcType;
    IM3Runtime  runtime = i_function->module->runtime;
    M3_परिणाम    परिणाम  = m3Err_none;
    u8*         s       = रिक्त;

    यदि (i_argc != ftype->अङ्कः_तर्काः) {
        प्रत्यागम m3_Err_तर्कः_गणनम्_Mismatch;
    }
    यदि (!i_function->compiled) {
        प्रत्यागम m3_Err_missing_Compiled_संहिता;
    }

#यदि d_m3_लेखः_Backtraces
    ClearBacktrace(runtime);
#यदि_अन्तः

    m3_राशिः_Check_Init();

_   (checkStartFunction(i_function->module))

_   (Get_राशिः_Pointer_For_तर्काः(i_function, &s))

    प्रति (u32 i = 0; i < ftype->अङ्कः_तर्काः; ++i) {
        u64 मानम् = 0;
#यदि d_m3HasFloat
        f64 fvalue = 0;
#यदि_अन्तः
        विकल्प (d_FuncArgType(ftype, i)) {
        पक्ष c_m3Type_i32:
_           (ParseArgInteger(i_argv[i], 32, &मानम्)) * (i32*)(s) = (i32)मानम्;
            s += 8;
            भङ्ग;
        पक्ष c_m3Type_i64:
_           (ParseArgInteger(i_argv[i], 64, &मानम्)) * (i64*)(s) = (i64)मानम्;
            s += 8;
            भङ्ग;
        पक्ष c_m3Type_funcref:
        पक्ष c_m3Type_externref:
        पक्ष c_m3Type_exnref:
_           (ParseArgReference(i_argv[i], &मानम्)) * (uintptr_t*)(s) = (uintptr_t)मानम्;
            s += 8;
            भङ्ग;
#यदि d_m3HasFloat
            // strtof would be less portable
        पक्ष c_m3Type_f32:
_           (ParseArgFloat(i_argv[i], &fvalue)) * (f32*)(s) = (f32)fvalue;
            s += 8;
            भङ्ग;
        पक्ष c_m3Type_f64:
_           (ParseArgFloat(i_argv[i], &fvalue)) * (f64*)(s) = fvalue;
            s += 8;
            भङ्ग;
#यदि_अन्तः
        स्वभाव: _throw("unknown argument type");
        }
    }

    परिणाम = सततम्_संहिता_Checked(runtime, i_function);

    Report_Native_राशिः_Usage();

    runtime->चरमः_Called = परिणाम ? रिक्त : i_function;

    _catch: प्रत्यागम परिणाम;
}


//u8 * AlignStackPointerTo64Bits (const u8 * i_stack)
//{
//    uintptr_t ptr = (uintptr_t) i_stack;
//    return (u8 *) ((ptr + 7) & ~7);
//}


M3_परिणाम m3_GetResults (IM3Function i_function, uint32_t i_retc, ध्रुव शून्य* o_retptrs[])
{
    IM3FuncType ftype   = i_function->funcType;
    IM3Runtime  runtime = i_function->module->runtime;

    यदि (i_retc != ftype->अङ्कः_Rets) {
        प्रत्यागम m3_Err_तर्कः_गणनम्_Mismatch;
    }
    यदि (i_function != runtime->चरमः_Called) {
        प्रत्यागम "function not called";
    }

    u8* s = (u8*)runtime->राशिः;

    प्रति (u32 i = 0; i < ftype->अङ्कः_Rets; ++i) {
        विकल्प (d_FuncRetType(ftype, i)) {
        पक्ष c_m3Type_i32:
            *(i32*)o_retptrs[i] = *(i32*)(s);
            s += 8;
            भङ्ग;
        पक्ष c_m3Type_i64:
            *(i64*)o_retptrs[i] = *(i64*)(s);
            s += 8;
            भङ्ग;
        पक्ष c_m3Type_funcref:
        पक्ष c_m3Type_externref:
        पक्ष c_m3Type_exnref:
            *(uintptr_t*)o_retptrs[i] = *(uintptr_t*)(s);
            s += 8;
            भङ्ग;
#यदि d_m3HasFloat
        पक्ष c_m3Type_f32:
            *(f32*)o_retptrs[i] = *(f32*)(s);
            s += 8;
            भङ्ग;
        पक्ष c_m3Type_f64:
            *(f64*)o_retptrs[i] = *(f64*)(s);
            s += 8;
            भङ्ग;
#यदि_अन्तः
        स्वभाव: प्रत्यागम "unknown return type";
        }
    }
    प्रत्यागम m3Err_none;
}

M3_परिणाम m3_GetResultsV (IM3Function i_function, ...)
{
    va_list ap;
    va_start(ap, i_function);
    M3_परिणाम r = m3_GetResultsVL(i_function, ap);
    va_end(ap);
    प्रत्यागम r;
}

M3_परिणाम m3_GetResultsVL (IM3Function i_function, va_list o_rets)
{
    IM3Runtime  runtime = i_function->module->runtime;
    IM3FuncType ftype   = i_function->funcType;

    यदि (i_function != runtime->चरमः_Called) {
        प्रत्यागम "function not called";
    }

    u8* s = (u8*)runtime->राशिः;
    प्रति (u32 i = 0; i < ftype->अङ्कः_Rets; ++i) {
        विकल्प (d_FuncRetType(ftype, i)) {
        पक्ष c_m3Type_i32:
            *va_arg(o_rets, i32*) = *(i32*)(s);
            s += 8;
            भङ्ग;
        पक्ष c_m3Type_i64:
            *va_arg(o_rets, i64*) = *(i64*)(s);
            s += 8;
            भङ्ग;
        पक्ष c_m3Type_funcref:
        पक्ष c_m3Type_externref:
        पक्ष c_m3Type_exnref:
            *va_arg(o_rets, uintptr_t*) = *(uintptr_t*)(s);
            s += 8;
            भङ्ग;
#यदि d_m3HasFloat
        पक्ष c_m3Type_f32:
            *va_arg(o_rets, f32*) = *(f32*)(s);
            s += 8;
            भङ्ग;
        पक्ष c_m3Type_f64:
            *va_arg(o_rets, f64*) = *(f64*)(s);
            s += 8;
            भङ्ग;
#यदि_अन्तः
        स्वभाव: प्रत्यागम "unknown argument type";
        }
    }
    प्रत्यागम m3Err_none;
}

शून्य Release_संहिता_Page_No_Track (IM3Runtime i_runtime, IM3_संहिता_Page i_संहिता_Page)
{
    यदि (i_संहिता_Page) {
        IM3_संहिता_Page* list;

        bool page_पूर्णम् = (अङ्कः_Free_पङ्क्तयः(i_संहिता_Page) < d_m3_संहिता_Page_Free_पङ्क्तयः_देहली);
        यदि (page_पूर्णम्) {
            list = &i_runtime->पृष्ठानि_पूर्णम्;
        } अन्यथा {
            list = &i_runtime->पृष्ठानि_Open;
        }

        प्रक्षिप_संहिता_Page(list, i_संहिता_Page);                         m3log (emit, "release page: %d to queue: '%s'", i_संहिता_Page->info.sequence, page_पूर्णम् ? "full" : "open")
    }
}


IM3_संहिता_Page Acquire_संहिता_Page_With_क्षमता (IM3Runtime i_runtime, u32 i_min_पङ्क्तिः_गणनम्)
{
    IM3_संहिता_Page page = Remove_संहिता_Page_सम्बन्धि_क्षमता(&i_runtime->पृष्ठानि_Open, i_min_पङ्क्तिः_गणनम्);

    यदि (not page) {
        page = Environment_Acquire_संहिता_Page(i_runtime->environment, i_min_पङ्क्तिः_गणनम्);

        यदि (not page) {
            page = New_संहिता_Page(i_runtime, i_min_पङ्क्तिः_गणनम्);
        }

        यदि (page) {
            i_runtime->अङ्कः_संहिता_पृष्ठानि++;
        }
    }

    यदि (page) {                                                  m3log (emit, "acquire page: %d", page->info.sequence);
        i_runtime->अङ्कः_सक्रिय_संहिता_पृष्ठानि++;
    }

    प्रत्यागम page;
}


IM3_संहिता_Page Acquire_संहिता_Page (IM3Runtime i_runtime)
{
    प्रत्यागम Acquire_संहिता_Page_With_क्षमता(i_runtime, d_m3_संहिता_Page_Free_पङ्क्तयः_देहली);
}


शून्य Release_संहिता_Page (IM3Runtime i_runtime, IM3_संहिता_Page i_संहिता_Page)
{
    यदि (i_संहिता_Page) {
        Release_संहिता_Page_No_Track(i_runtime, i_संहिता_Page);
        i_runtime->अङ्कः_सक्रिय_संहिता_पृष्ठानि--;

#यदि defined(DEBUG)
        u32 अङ्कः_Open = गणनम्_संहिता_पृष्ठानि(i_runtime->पृष्ठानि_Open);
        u32 अङ्कः_पूर्णम् = गणनम्_संहिता_पृष्ठानि(i_runtime->पृष्ठानि_पूर्णम्);

        m3log(runtime, "runtime: %p; open-pages: %d; full-pages: %d; active: %d; total: %d", i_runtime, अङ्कः_Open, अङ्कः_पूर्णम्, i_runtime->अङ्कः_सक्रिय_संहिता_पृष्ठानि, i_runtime->अङ्कः_संहिता_पृष्ठानि);

        d_m3Assert(अङ्कः_Open + अङ्कः_पूर्णम् + i_runtime->अङ्कः_सक्रिय_संहिता_पृष्ठानि == i_runtime->अङ्कः_संहिता_पृष्ठानि);

#  यदि d_m3_Log_संहिता_पृष्ठानि
        dump_संहिता_page(i_संहिता_Page, /* startPC: */ रिक्त);
#  endif
#यदि_अन्तः
    }
}


#यदि d_m3VerboseErrorMessages
M3_परिणाम m3Error (M3_परिणाम i_परिणाम, IM3Runtime i_runtime, IM3Module i_module, IM3Function i_function,
                  ध्रुव वर्ण* ध्रुव i_file, u32 i_पङ्क्तिः_अङ्कः, ध्रुव वर्ण* ध्रुव i_error_सन्देशः, ...)
{
    यदि (i_runtime) {
        i_runtime->error         = (M3ErrorInfo){ .परिणाम = i_परिणाम, .runtime = i_runtime, .module = i_module, .function = i_function, .file = i_file, .पङ्क्तिः_2 = i_पङ्क्तिः_अङ्कः };
        i_runtime->error.सन्देशः = i_runtime->error_सन्देशः;

        va_list तर्काः;
        va_start(तर्काः, i_error_सन्देशः);
        vsnprintf(i_runtime->error_सन्देशः, परिमाण(i_runtime->error_सन्देशः), i_error_सन्देशः, तर्काः);
        va_end(तर्काः);
    }

    प्रत्यागम i_परिणाम;
}
#यदि_अन्तः


शून्य m3_GetErrorInfo (IM3Runtime i_runtime, M3ErrorInfo* o_info)
{
    यदि (i_runtime) {
        *o_info = i_runtime->error;
        m3_ResetErrorInfo(i_runtime);
    }
}


शून्य m3_ResetErrorInfo (IM3Runtime i_runtime)
{
    यदि (i_runtime) {
        M3_INIT(i_runtime->error);
        i_runtime->error.सन्देशः = "";
    }
}

uint8_t* m3_Get_स्मृतिः (IM3Module i_module, size_t* o_स्मृतिः_आकार_अन्तर्_अष्टकानि, uint32_t i_स्मृतिः_सूचकांक)
{
    uint8_t* स्मृतिः = रिक्त;
    size_t   आकार   = 0;

    यदि (i_module and i_स्मृतिः_सूचकांक < i_module->अङ्कः_Memories) {
        IM3_स्मृतिः mem = i_module->memories[i_स्मृतिः_सूचकांक];

        यदि (mem->mallocated) {
            आकार = mem->mallocated->दीर्घता;

            यदि (आकार) {
                स्मृतिः = m3_Mem_दत्त(mem->mallocated);
            }
        }
    }

    यदि (o_स्मृतिः_आकार_अन्तर्_अष्टकानि) {
        *o_स्मृतिः_आकार_अन्तर्_अष्टकानि = आकार;
    }

    प्रत्यागम स्मृतिः;
}


size_t m3_Get_स्मृतिः_आकार (IM3Module i_module, uint32_t i_स्मृतिः_सूचकांक)
{
    यदि (not i_module or i_स्मृतिः_सूचकांक >= i_module->अङ्कः_Memories) {
        प्रत्यागम 0;
    }

    IM3_स्मृतिः mem = i_module->memories[i_स्मृतिः_सूचकांक];

    प्रत्यागम mem->mallocated ? mem->mallocated->दीर्घता : 0;
}


size_t m3_Get_स्मृतिः_आकार_At (ध्रुव शून्य* i_स्मृतिः)
{
    यदि (not i_स्मृतिः) {
        प्रत्यागम 0;
    }

    // the header sits immediately before the data it describes
    ध्रुव M3_स्मृतिः_शीर्षकम्* शीर्षकम् = ((ध्रुव M3_स्मृतिः_शीर्षकम्*)i_स्मृतिः) - 1;

    प्रत्यागम शीर्षकम्->दीर्घता;
}


M3_परिणाम m3_Find_Exported_स्मृतिः (IM3Module i_module, cstr_t i_name, u32* o_स्मृतिः_सूचकांक)
{
    यदि (not i_module or not i_name or not o_स्मृतिः_सूचकांक) {
        प्रत्यागम m3_Err_unknown_स्मृतिः;
    }

    ध्रुव M3Export* e = Module_FindExport(i_module, d_external_विधा_स्मृतिः, i_name);

    यदि (not e) {
        प्रत्यागम m3_Err_unknown_स्मृतिः;
    }

    *o_स्मृतिः_सूचकांक = e->index;
    प्रत्यागम m3Err_none;
}


bool Module_HasLinkedHostImport (IM3Module i_module, cstr_t i_importModule)
{
    प्रति (u32 i = 0; i < i_module->अङ्कः_Functions; ++i) {
        IM3Function f = &i_module->functions[i];

        // hostMemory is set when a host function is bound to the import, so it
        // is also the mark of one - see CompileRawFunction
        यदि (not f->host_स्मृतिः or f->wasm or f->resolved) {
            अनुवर्त;
        }

        यदि (f->import.moduleUtf8 and वाक्यं_तुलय(f->import.moduleUtf8, i_importModule) == 0) {
            प्रत्यागम true;
        }
    }

    प्रत्यागम false;
}


M3_परिणाम m3_Bind_Import_स्मृतिः (IM3Module io_module, cstr_t i_importModule, u32 i_स्मृतिः_सूचकांक)
{
    यदि (not io_module or not i_importModule) {
        प्रत्यागम m3Err_moduleNotLinked;
    }

    यदि (i_स्मृतिः_सूचकांक >= io_module->अङ्कः_Memories) {
        प्रत्यागम m3_Err_unknown_स्मृतिः;
    }

    // the memory itself, not the index: linking may have pointed this slot at
    // another module's memory, and growing one reallocates behind it
    IM3_स्मृतिः स्मृतिः = io_module->memories[i_स्मृतिः_सूचकांक];

    ध्रुव bool wildcardModule = (वाक्यं_तुलय(i_importModule, "*") == 0);

    प्रति (u32 i = 0; i < io_module->अङ्कः_Functions; ++i) {
        IM3Function f = &io_module->functions[i];

        // only an import that a host function was bound to has a memory to
        // pin: one with a body of its own reads memory through the interpreter,
        // and one resolved to another module's export runs that module's code
        यदि (not f->host_स्मृतिः or f->wasm or f->resolved) {
            अनुवर्त;
        }

        यदि (wildcardModule or (f->import.moduleUtf8 and वाक्यं_तुलय(f->import.moduleUtf8, i_importModule) == 0)) {
            f->host_स्मृतिः = स्मृतिः;
        }
    }

    प्रत्यागम m3Err_none;
}


M3BacktraceInfo* m3_GetBacktrace (IM3Runtime i_runtime)
{
#यदि d_m3_लेखः_Backtraces
    प्रत्यागम &i_runtime->backtrace;
#अन्यथा
    प्रत्यागम रिक्त;
#यदि_अन्तः
}


//-------------------------------------------------------------------------------------------------------------------------------
//  snapshots & suspendable execution
//-------------------------------------------------------------------------------------------------------------------------------

शून्य m3_समुच्चयः_Suspendable (IM3Runtime io_runtime, bool i_suspendable)
{
    यदि (io_runtime) {
        io_runtime->isSuspendable = i_suspendable;
    }
}

शून्य m3_RequestSuspend (IM3Runtime io_runtime)
{
    यदि (io_runtime) {
        io_runtime->suspendRequested = true;
    }
}

bool m3_IsSuspended (IM3Runtime i_runtime)
{
    यदि (!i_runtime) {
        प्रत्यागम false;
    }
#यदि d_m3_Has_राशिः_Switching
    यदि (i_runtime->मूलम्_Continuation && i_runtime->मूलम्_Continuation->state == cont_suspended) {
        प्रत्यागम true;
    }
#यदि_अन्तः
    प्रत्यागम false;
}

M3_परिणाम m3_ResumeRuntime (IM3Runtime io_runtime)
{
    यदि (!io_runtime) {
        प्रत्यागम m3Err_mallocFailed;
    }
    यदि (!io_runtime->isSuspendable) {
        प्रत्यागम m3Err_none;
    }
#यदि d_m3HasSnapshots
    // a restore that failed leaves nothing to resume, and says so every time
    // rather than resuming nothing successfully
    यदि (io_runtime->मूलम्_Continuation and io_runtime->मूलम्_Continuation->प्रविष्टिः_Function and
        io_runtime->मूलम्_Continuation->प्रविष्टिः_Function->module->isUnusable) {
        प्रत्यागम "a snapshot failed to restore into the module, so it cannot run";
    }
#यदि_अन्तः
#यदि d_m3_Has_राशिः_Switching
    यदि (!io_runtime->मूलम्_Continuation || io_runtime->मूलम्_Continuation->state != cont_suspended) {
        प्रत्यागम m3Err_none;
    }
    // what finishes returns into the bottom of the stack, where it was entered
    io_runtime->राशिः = io_runtime->origin_राशिः;

    // this is the outermost call again, as far as a host function calling back
    // into Wasm from inside it can tell
    io_runtime->entered = io_runtime->मूलम्_Continuation->प्रविष्टिः_Function;
    io_runtime->callNesting++;

    d_m3_राशिः_सीमा_Enter(io_runtime);
    io_runtime->resumePastCheck = true;
    M3_परिणाम परिणाम             = (M3_परिणाम)ResumeContinuation(io_runtime, io_runtime->मूलम्_Continuation);
    io_runtime->resumePastCheck = false;
    d_m3_राशिः_सीमा_Leave(io_runtime);

#  यदि d_m3HasExceptionHandling
    यदि (M3_UNLIKELY(परिणाम == m3Err_pendingException)) {
        परिणाम = m3Err_trapUncaughtException;
    }
    लेखः_Uncaught_Exception(io_runtime, परिणाम);
#  endif

    io_runtime->callNesting--;

#  यदि d_m3HasExceptionHandling
    यदि (परिणाम != m3Err_continuationSuspended and io_runtime->callNesting == 0) {
        FreeExceptions(io_runtime);
    }
#  endif

    // the results are where the paused call left room for them, so a finished
    // resume can be asked for them the way the call itself could have been
    io_runtime->चरमः_Called = (परिणाम == m3Err_none) ? io_runtime->मूलम्_Continuation->प्रविष्टिः_Function : रिक्त;

    Runtime_Place_Call_राशिः(io_runtime);
    प्रत्यागम परिणाम;
#अन्यथा
    प्रत्यागम m3Err_none;
#यदि_अन्तः
}
