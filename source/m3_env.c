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

    जइ (env) {
        _try
        {
            // create FuncTypes for all simple block return ValueTypes.
            // v128 is skipped: it parses as a slot but has no operations.
            प्रति (u8 t = c_m3Type_none; t < c_m3_Type_गणनम्; t++) {
                जइ (t == c_m3Type_v128) {
                    अनुवर्त;
                }

                IM3FuncType ftype;
_               (AllocFuncType(&ftype, 1));

                ftype->अङ्कः_Args  = 0;
                ftype->अङ्कः_Rets  = (t == c_m3Type_none) ? 0 : 1;
                ftype->types[0] = t;

                Environment_AddFuncType(env, &ftype);

                env->retFuncTypes[t] = ftype;
            }
        }

        _catch:
        जइ (result) {
            m3_FreeEnvironment(env);
            env = रिक्त;
        }
    }

    प्रत्यागम env;
}


शून्य Environment_Release (IM3Environment i_environment)
{
    IM3FuncType ftype = i_environment->funcTypes;

    जाव (ftype) {
        IM3FuncType next = ftype->next;
        m3_Free(ftype);
        ftype = next;
    }

    m3log(runtime, "freeing %d pages from environment", गणनम्_Code_Pages(i_environment->pagesReleased));
    FreeCodePages(&i_environment->pagesReleased);
}


शून्य m3_FreeEnvironment (IM3Environment i_environment)
{
    जइ (i_environment) {
        Environment_Release(i_environment);
        m3_Free(i_environment);
    }
}


शून्य m3_SetCustomSectionHandler (IM3Environment i_environment, M3SectionHandler i_handler)
{
    जइ (i_environment) {
        i_environment->customSectionHandler = i_handler;
    }
}


// returns the same io_funcType or replaces it with an equivalent that's already in the type linked list
M3Result Environment_AddFuncType (IM3Environment i_environment, IM3FuncType* io_funcType)
{
    IM3FuncType addType = *io_funcType;
    IM3FuncType newType = i_environment->funcTypes;

    जाव (newType) {
        जइ (not newType->inRecGroup and AreFuncTypesEqual(newType, addType)) {
            m3_Free(addType);
            भङ्ग;
        }

        newType = newType->next;
    }

    जइ (newType == रिक्त) {
        // a type index has to fit in the heap type field of an m3type_t
        जइ (i_environment->अङ्कः_Func_Types >= d_m3_Max_Sane_Types_गणनम्) {
            m3_Free(addType);
            *io_funcType = रिक्त;
            प्रत्यागम "too many distinct function types";
        }

        newType                  = addType;
        newType->canonicalIndex  = i_environment->अङ्कः_Func_Types++;
        newType->next            = i_environment->funcTypes;
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
M3Result Environment_ReserveFuncTypes (IM3Environment i_environment, u32 i_गणनम्, u16* o_firstIndex)
{
    जइ ((u64)i_environment->अङ्कः_Func_Types + i_गणनम् > d_m3_Max_Sane_Types_गणनम्) {
        प्रत्यागम "too many distinct function types";
    }

    *o_firstIndex = i_environment->अङ्कः_Func_Types;
    i_environment->अङ्कः_Func_Types += (u16)i_गणनम्;

    प्रत्यागम m3Err_none;
}


शून्य Environment_AdoptFuncType (IM3Environment i_environment, IM3FuncType i_funcType, u16 i_canonicalIndex)
{
    i_funcType->inRecGroup     = true;
    i_funcType->canonicalIndex = i_canonicalIndex;
    i_funcType->next           = i_environment->funcTypes;
    i_environment->funcTypes   = i_funcType;
}


IM3CodePage RemoveCodePageOfCapacity (M3CodePage** io_list, u32 i_minimum_पङ्क्तिः_गणनम्)
{
    IM3CodePage prev = रिक्त;
    IM3CodePage page = *io_list;

    जाव (page) {
        जइ (अङ्कः_Free_पङ्क्तयः(page) >= i_minimum_पङ्क्तिः_गणनम्) {             d_m3Assert (page->info.usage_गणनम् == 0);
            IM3CodePage next = page->info.next;
            जइ (prev) {
                prev->info.next = next; // mid-list
            } अण्णहा {
                *io_list = next;       // front of list
            }

            भङ्ग;
        }

        prev = page;
        page = page->info.next;
    }

    प्रत्यागम page;
}


IM3CodePage Environment_AcquireCodePage (IM3Environment i_environment, u32 i_minimum_पङ्क्तिः_गणनम्)
{
    प्रत्यागम RemoveCodePageOfCapacity(&i_environment->pagesReleased, i_minimum_पङ्क्तिः_गणनम्);
}


शून्य Environment_ReleaseCodePages (IM3Environment i_environment, IM3CodePage i_codePageList)
{
    IM3CodePage end = i_codePageList;

    जाव (end) {
        end->info.पङ्क्तिः_Index = 0; // reset page
#जइ d_m3RecordBacktraces
        end->info.mapping->size = 0;
#endif // d_m3RecordBacktraces

        IM3CodePage next = end->info.next;
        जइ (not next) {
            भङ्ग;
        }

        end = next;
    }

    जइ (end) {
        // push list to front
        end->info.next               = i_environment->pagesReleased;
        i_environment->pagesReleased = i_codePageList;
    }
}


IM3Runtime m3_NewRuntime (IM3Environment i_environment, u32 i_राशिः_Size_In_Bytes, शून्य* i_userdata)
{
    IM3Runtime runtime = m3_AllocStruct(M3Runtime);

    जइ (runtime) {
        m3_ResetErrorInfo(runtime);

        runtime->environment = i_environment;
        runtime->userdata    = i_userdata;

        Deterministic_InitRuntime(runtime);

        runtime->origin_राशिः = m3_Malloc("Wasm Stack", i_राशिः_Size_In_Bytes + 4 * परिमाण(m3slot_t)); // TODO: more precise stack checks

        जइ (runtime->origin_राशिः) {
            runtime->राशिः         = runtime->origin_राशिः;
            runtime->अङ्कः_राशिः_Slots = i_राशिः_Size_In_Bytes / परिमाण(m3slot_t);          m3log (runtime, "new stack: %p, slots: %u", runtime->origin_राशिः, runtime->अङ्कः_राशिः_Slots);
        } अण्णहा {
            m3_Free(runtime);
        }
    }

    प्रत्यागम runtime;
}

शून्य m3_SetValidation (IM3Runtime i_runtime, bool i_enable)
{
#जइ d_m3EnableValidation
    जइ (i_runtime) {
        i_runtime->skipValidation = not i_enable;
    }
#अण्णहा
    (शून्य)i_runtime;
    (शून्य)i_enable;              // nothing to skip: the validator was compiled out
#endif
}

M3Result m3_SetResourceLimit (IM3Runtime i_runtime, M3ResourceLimit i_limit, uint64_t i_मानम्)
{
    जइ (not i_runtime) {
        प्रत्यागम m3Err_mallocFailed;
    }
    विकल्प (i_limit) {
    पक्ष c_m3Limit_MemoryBytes:
        जइ (i_मानम् and i_मानम् < i_runtime->memoryBytesUsed) {
            प्रत्यागम m3Err_resourceLimitBelowUsage;
        }
        i_runtime->memoryBytesLimit = i_मानम्;
        प्रत्यागम m3Err_none;
    पक्ष c_m3Limit_TableElements:
        जइ (i_मानम् and i_मानम् < i_runtime->tableElementsUsed) {
            प्रत्यागम m3Err_resourceLimitBelowUsage;
        }
        i_runtime->tableElementsLimit = i_मानम्;
        प्रत्यागम m3Err_none;
#जइ d_m3_Has_राशिः_Switching
    पक्ष c_m3Limit_Continuations:
        जइ (i_मानम् and i_मानम् < i_runtime->continuationsAllocated) {
            प्रत्यागम m3Err_resourceLimitBelowUsage;
        }
        i_runtime->continuationsLimit = (u32)M3_MIN(i_मानम्, (u64)UINT32_MAX);
        जाव (i_मानम् and i_runtime->val_राशिः_Pool_गणनम् > i_मानम् - i_runtime->continuationsAllocated) {
            m3_Free(i_runtime->val_राशिः_Pool[--i_runtime->val_राशिः_Pool_गणनम्]);
        }
        प्रत्यागम m3Err_none;
#अण्णहा
    पक्ष c_m3Limit_Continuations: प्रत्यागम m3Err_resourceLimitNotSupported;
#endif
    पक्ष c_m3Limit_GasUnits:
#जइ d_m3HasGasMetering
        i_runtime->gasLimit = i_runtime->gasRemaining = (i64)M3_MIN(i_मानम्, (u64)INT64_MAX);
        प्रत्यागम m3Err_none;
#अण्णहा
        प्रत्यागम m3Err_resourceLimitNotSupported;
#endif
    स्वभाव: प्रत्यागम m3Err_unknownResourceLimit;
    }
}

uint64_t m3_GetResourceLimit (IM3Runtime i_runtime, M3ResourceLimit i_limit)
{
    जइ (not i_runtime) {
        प्रत्यागम 0;
    }
    विकल्प (i_limit) {
    पक्ष c_m3Limit_MemoryBytes: प्रत्यागम i_runtime->memoryBytesLimit;
    पक्ष c_m3Limit_TableElements: प्रत्यागम i_runtime->tableElementsLimit;
#जइ d_m3_Has_राशिः_Switching
    पक्ष c_m3Limit_Continuations: प्रत्यागम i_runtime->continuationsLimit;
#endif
#जइ d_m3HasGasMetering
    पक्ष c_m3Limit_GasUnits: प्रत्यागम (u64)i_runtime->gasLimit;
#endif
    स्वभाव: प्रत्यागम 0;
    }
}

uint64_t m3_GetResourceUsage (IM3Runtime i_runtime, M3ResourceLimit i_limit)
{
    जइ (not i_runtime) {
        प्रत्यागम 0;
    }
    विकल्प (i_limit) {
    पक्ष c_m3Limit_MemoryBytes: प्रत्यागम i_runtime->memoryBytesUsed;
    पक्ष c_m3Limit_TableElements: प्रत्यागम i_runtime->tableElementsUsed;
#जइ d_m3_Has_राशिः_Switching
    पक्ष c_m3Limit_Continuations: प्रत्यागम i_runtime->continuationsAllocated;
#endif
#जइ d_m3HasGasMetering
    पक्ष c_m3Limit_GasUnits: प्रत्यागम (u64)i_runtime->gasLimit - (u64)i_runtime->gasRemaining;
#endif
    स्वभाव: प्रत्यागम 0;
    }
}

शून्य* m3_GetUserData (IM3Runtime i_runtime)
{
    प्रत्यागम i_runtime ? i_runtime->userdata : रिक्त;
}


शून्य* ForEachModule (IM3Runtime i_runtime, ModuleVisitor i_visitor, शून्य* i_info)
{
    शून्य* r = रिक्त;

    IM3Module module = i_runtime->modules;

    जाव (module) {
        IM3Module next = module->next;
        जइ ((r = i_visitor(module, i_info))) {
            भङ्ग;
        }

        module = next;
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
        IM3Memory memory = i_module->memories[i];

        जइ (memory and memory->owner != i_module) {
            i_module->memories[i] = रिक्त;
        }
    }

    प्रति (u32 i = 0; i < i_module->अङ्कः_Tables; ++i) {
        IM3Table table = i_module->tables[i];

        जइ (table and table->owner != i_module) {
            i_module->tables[i] = रिक्त;
        }
    }

    प्रत्यागम रिक्त;
}


शून्य Runtime_Release (IM3Runtime i_runtime)
{
    ForEachModule(i_runtime, _ReleaseBorrowedSlots, रिक्त);
    ForEachModule(i_runtime, _FreeModule, रिक्त);                    d_m3Assert (i_runtime->अङ्कः_Active_Code_Pages == 0);

    Environment_ReleaseCodePages(i_runtime->environment, i_runtime->pagesOpen);
    Environment_ReleaseCodePages(i_runtime->environment, i_runtime->pagesFull);

    m3_Free(i_runtime->origin_राशिः);

#जइ d_m3EnableValidation
    m3_Free(i_runtime->validator);
#endif

#जइ d_m3HasExceptionHandling
    // a runtime let go of while suspended still holds its exceptions
    FreeExceptions(i_runtime);
    m3_Free(i_runtime->uncaughtException);
#endif
#जइ d_m3HasExceptionHandling || d_m3_Has_राशिः_Switching
    जाव (i_runtime->hostTags) {
        M3HostTag* next = i_runtime->hostTags->next;
        m3_Free(i_runtime->hostTags);
        i_runtime->hostTags = next;
    }
#endif
#जइ d_m3_Has_राशिः_Switching
    Continuation_ReleaseAll(i_runtime);
#endif
}


शून्य m3_FreeRuntime (IM3Runtime i_runtime)
{
    जइ (i_runtime) {
        m3_PrintProfilerInfo();

        Runtime_Release(i_runtime);
        m3_Free(i_runtime);
    }
}


#जइ d_m3_Has_राशिः_Switching

IM3Continuation Continuation_New (IM3Runtime i_runtime, IM3FuncType i_type, IM3Function i_function)
{
    IM3Continuation cont = m3_AllocStruct(M3Continuation);
    जइ (!cont) {
        प्रत्यागम रिक्त;
    }

    cont->type          = i_type;
    cont->entryFunction = i_function;
    cont->state         = cont_allocated;

    cont->अङ्कः_राशिः_Slots = d_m3_Continuation_राशिः_Slots;
    // frames are recorded only when this continuation actually suspends, and
    // only as deep as it got, so the array starts out unallocated
    cont->frames    = रिक्त;
    cont->framesCap = 0;
    cont->अङ्कः_Frames = 0;

    cont->sp = cont->val_राशिः;
    cont->pc = i_function ? i_function->compiled : रिक्त;
    cont->r0 = 0;
#  जइ d_m3HasFloat
    cont->fp0 = 0;
#  endif
    cont->bound_Args_गणनम्    = 0;
    cont->अङ्कः_Suspend_Results = 0;
    cont->अङ्कः_Handlers       = 0;
    cont->handlersPC        = रिक्त;
    cont->parent            = रिक्त;

    cont->next               = i_runtime->continuations;
    i_runtime->continuations = cont;

    प्रत्यागम cont;
}

IM3Continuation Continuation_ForkSuspended (IM3Runtime i_runtime, IM3Continuation i_cont)
{
    IM3Continuation newCont = Continuation_New(i_runtime, i_cont->type, i_cont->entryFunction);
    जइ (M3_UNLIKELY(not newCont)) {
        प्रत्यागम रिक्त;
    }

    newCont->val_राशिः      = i_cont->val_राशिः;
    newCont->अङ्कः_राशिः_Slots = i_cont->अङ्कः_राशिः_Slots;
    newCont->sp            = i_cont->sp;
    newCont->pc            = i_cont->pc;
    newCont->suspendPoint  = i_cont->suspendPoint;
    newCont->r0            = i_cont->r0;
#  जइ d_m3HasFloat
    newCont->fp0 = i_cont->fp0;
#  endif
    newCont->frames    = i_cont->frames;
    newCont->framesCap = i_cont->framesCap;
    newCont->अङ्कः_Frames = i_cont->अङ्कः_Frames;

    newCont->bound_Args_गणनम् = 0;
#  जइ d_m3HasExceptionHandling
    newCont->resumeThrow = i_cont->resumeThrow;
    i_cont->resumeThrow  = रिक्त;
#  endif
    newCont->अङ्कः_Suspend_Results = i_cont->अङ्कः_Suspend_Results;
    प्रति (u32 i = 0; i < i_cont->अङ्कः_Suspend_Results; ++i) {
        newCont->suspendResultOffsets[i] = i_cont->suspendResultOffsets[i];
        newCont->suspendResultIs64[i]    = i_cont->suspendResultIs64[i];
    }

    newCont->अङ्कः_Handlers = i_cont->अङ्कः_Handlers;
    newCont->handlersPC  = i_cont->handlersPC;
    newCont->parent      = i_cont->parent;
    newCont->state       = cont_suspended;

    i_cont->val_राशिः  = रिक्त;
    i_cont->frames    = रिक्त;
    i_cont->framesCap = 0;
    i_cont->अङ्कः_Frames = 0;
    i_cont->state     = cont_consumed;

    प्रत्यागम newCont;
}

// Only cont.new and snapshot restoration acquire stacks. Binding and suspension
// transfer them between records without changing the active count.
M3Result Continuation_Acquire_राशिः (IM3Runtime runtime, IM3Continuation cont)
{
    जइ (not Continuation_Can_Acquire_राशिः(runtime)) {
        प्रत्यागम m3Err_continuationLimitExceeded;
    }
    जइ (runtime->val_राशिः_Pool_गणनम्) {
        cont->val_राशिः = runtime->val_राशिः_Pool[--runtime->val_राशिः_Pool_गणनम्];
        memset(cont->val_राशिः, 0, (cont->अङ्कः_राशिः_Slots + 4) * परिमाण(m3slot_t));
    } अण्णहा {
        cont->val_राशिः = m3_AllocArray(m3slot_t, cont->अङ्कः_राशिः_Slots + 4);
    }
    जइ (not cont->val_राशिः) {
        प्रत्यागम m3Err_mallocFailed;
    }
    cont->sp = cont->val_राशिः;
    runtime->continuationsAllocated++;
    प्रत्यागम m3Err_none;
}

शून्य Continuation_Recycle_राशिः (IM3Runtime runtime, IM3Continuation cont)
{
    जइ (not cont->val_राशिः or cont == runtime->मूलम्_Continuation) {
        प्रत्यागम;
    }
    जइ (not runtime->val_राशिः_Pool) {
        runtime->val_राशिः_Pool = m3_AllocArray(m3slot_t*, 16);
        जइ (runtime->val_राशिः_Pool) {
            runtime->val_राशिः_Pool_Cap = 16;
        }
    }
    जइ (runtime->val_राशिः_Pool_गणनम् < runtime->val_राशिः_Pool_Cap and
        (not runtime->continuationsLimit or runtime->val_राशिः_Pool_गणनम् <
                                              runtime->continuationsLimit - runtime->continuationsAllocated + 1)) {
        runtime->val_राशिः_Pool[runtime->val_राशिः_Pool_गणनम्++] = cont->val_राशिः;
        cont->val_राशिः                                      = रिक्त;
    } अण्णहा {
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

    जाव (cont) {
        IM3Continuation next = cont->next;

        m3_Free(cont->val_राशिः);
        m3_Free(cont->frames);
        m3_Free(cont);

        cont = next;
    }

    प्रति (u32 i = 0; i < io_runtime->val_राशिः_Pool_गणनम्; ++i) {
        m3_Free(io_runtime->val_राशिः_Pool[i]);
    }
    m3_Free(io_runtime->val_राशिः_Pool);
    io_runtime->val_राशिः_Pool_गणनम् = io_runtime->val_राशिः_Pool_Cap = 0;
    io_runtime->continuationsAllocated                          = 0;
    io_runtime->continuations                                   = रिक्त;
    io_runtime->activeContinuation                              = रिक्त;
    io_runtime->मूलम्_Continuation                                = रिक्त;
}


m3ret_t Continuation_RecordFrame (IM3Runtime i_runtime, ध्रुव M3Frame* i_frame)
{
    IM3Continuation cont = i_runtime->activeContinuation;

    // the marker only travels between a suspend and the resume that runs it,
    // and that whole stretch has an active continuation
    d_m3Assert(cont);

    जइ (cont->अङ्कः_Frames == cont->framesCap) {
        u32 newCap = cont->framesCap ? cont->framesCap * 2 : 8;

        जइ (newCap > d_m3ContinuationMaxFrames) {
            newCap = d_m3ContinuationMaxFrames;
        }

        जइ (newCap == cont->framesCap) {
            प्रत्यागम m3_Err_trap_राशिः_Overflow;
        }

        M3Frame* frames = m3_ReallocArray(M3Frame, cont->frames, newCap, cont->framesCap);

        जइ (not frames) {
            प्रत्यागम m3Err_mallocFailed;
        }

        cont->frames    = frames;
        cont->framesCap = newCap;
    }

    cont->frames[cont->अङ्कः_Frames++] = *i_frame;

    प्रत्यागम m3Err_continuationSuspended;
}

#endif // d_m3HasStackSwitching

M3Result EvaluateExpression (IM3Module i_module, शून्य* o_expressed, m3type_t i_type, bytes_t* io_bytes, cbytes_t i_end)
{
    M3Result result = m3Err_none;

    // OPTZ: use a simplified interpreter for expressions

    // create a temporary runtime context
#जइ defined(d_m3PreferStaticAlloc)
    स्थिर M3Runtime runtime;
#अण्णहा
    M3Runtime runtime;
#endif
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
    o->wasm            = *io_bytes;
    o->wasmEnd         = i_end;
    o->lastOpcodeStart = o->wasm;

    // Borrowed for the length of this expression only. Runtime_Release below hands it
    // to Environment_ReleaseCodePages, which resets lineIndex, so the page comes back
    // empty and the next expression reuses it rather than burning fresh lines.
    o->page = AcquireCodePage(&runtime);

    जइ (o->page) {
        IM3FuncType ftype = रिक्त;

#जइ d_m3HasTypedRefs
        // A reference type has to be checked as the type it is: the shared
        // return types are one per storage class, so borrowing one of those
        // would let any reference at all satisfy a particular one.
        जइ (IsSpelledRefType(i_type)) {
            result = AllocFuncType(&ftype, 1);

            जइ (not result) {
                ftype->अङ्कः_Rets  = 1;
                ftype->types[0] = i_type;

                result = Environment_AddFuncType(runtime.environment, &ftype);
            }
        } अण्णहा
#endif
        {
            ftype = runtime.environment->retFuncTypes[BaseTypeOf(i_type)];
        }

        pc_t m3code = GetPagePC(o->page);

        जइ (not result) {
            result = CompileExpression(o, ftype);
        }

        जइ (not result && o->max_राशिः_Slots >= runtime.अङ्कः_राशिः_Slots) {
            result = m3_Err_trap_राशिः_Overflow;
        }

        जइ (not result) {
#जइ (d_m3EnableOpProfiling || d_m3EnableOpTracing)
            m3ret_t r = RunCode(m3code, राशिः, रिक्त, d_m3OpDefaultArgs, d_m3BaseCstr);
#अण्णहा
            m3ret_t r = RunCode(m3code, राशिः, रिक्त, d_m3OpDefaultArgs);
#endif

            जइ (r == 0) {                                                                   m3log (runtime, "expression result: %s", SPrint_मानम् (राशिः, i_type));
                जइ (SizeOfType(BaseTypeOf(i_type)) == परिमाण(u32)) {
                    *(u32*)o_expressed = *((u32*)राशिः);
                } अण्णहा {
                    *(u64*)o_expressed = *((u64*)राशिः);
                }
            }
        }

        ReleaseCodePage(&runtime, o->page);
    } अण्णहा {
        result = m3Err_mallocFailedCodePage;
    }

    runtime.origin_राशिः = रिक्त;        // prevent free(stack) in ReleaseRuntime
    Runtime_Release(&runtime);
    i_module->runtime = savedRuntime;

    *io_bytes = o->wasm;

    प्रत्यागम result;
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
bool LimitsSatisfy (u64 i_exportedSize, bool i_exportedHasMax, u64 i_exportedMax,
                    u64 i_importMin, bool i_importHasMax, u64 i_importMax)
{
    जइ (i_exportedSize < i_importMin) {
        प्रत्यागम false;
    }

    जइ (i_importHasMax and (not i_exportedHasMax or i_exportedMax > i_importMax)) {
        प्रत्यागम false;
    }

    प्रत्यागम true;
}


// The export of this kind under this name, or NULL. The index it carries is
// resolved by the caller, against the module's index space as it stands then -
// after linking, a re-exported import's slot holds the entity it was linked to.
स्थिर
ध्रुव M3Export* Module_FindExport (IM3Module i_module, u8 i_kind, cstr_t i_name)
{
    प्रति (u32 i = 0; i < i_module->अङ्कः_Exports; ++i) {
        ध्रुव M3Export* e = &i_module->exports[i];

        जइ (e->kind == i_kind and strcmp(e->name, i_name) == 0) {
            प्रत्यागम e;
        }
    }

    प्रत्यागम रिक्त;
}


स्थिर
IM3Function Module_FindExportedFunction (IM3Module i_module, cstr_t i_name)
{
    ध्रुव M3Export* e = Module_FindExport(i_module, d_externalKind_function, i_name);

    // A module that re-exports an import names the placeholder here. Resolve to
    // the function that actually runs, or a host-side call would run it against
    // the importing module's memory.
    प्रत्यागम e ? Function_Implementation(&i_module->functions[e->index]) : रिक्त;
}


स्थिर
IM3Memory Module_FindExportedMemory (IM3Module i_module, cstr_t i_name)
{
    ध्रुव M3Export* e = Module_FindExport(i_module, d_externalKind_memory, i_name);

    प्रत्यागम e ? i_module->memories[e->index] : रिक्त;
}


स्थिर
IM3Table Module_FindExportedTable (IM3Module i_module, cstr_t i_name)
{
    ध्रुव M3Export* e = Module_FindExport(i_module, d_externalKind_table, i_name);

    प्रत्यागम e ? i_module->tables[e->index] : रिक्त;
}


स्थिर
IM3Global Module_FindExportedGlobal (IM3Module i_module, cstr_t i_name)
{
    ध्रुव M3Export* e = Module_FindExport(i_module, d_externalKind_global, i_name);

    प्रत्यागम e ? &i_module->globals[e->index] : रिक्त;
}


#जइ d_m3HasExceptionHandling || d_m3_Has_राशिः_Switching
स्थिर
IM3Tag Module_FindExportedTag (IM3Module i_module, cstr_t i_name)
{
    ध्रुव M3Export* e = Module_FindExport(i_module, d_externalKind_tag, i_name);

    जइ (not e) {
        प्रत्यागम रिक्त;
    }

    IM3Tag tag = &i_module->tags[e->index];

    प्रत्यागम tag->resolved ? tag->resolved : tag;
}
#endif


// Whether the module exports anything at all under this name. Export names are
// unique within a module, so a name one of the lookups above missed but this
// one finds is exported as something else - a kind the import cannot be
// satisfied by, rather than a name the module never exported.
स्थिर
bool Module_HasExport (IM3Module i_module, cstr_t i_name)
{
    प्रति (u32 i = 0; i < i_module->अङ्कः_Exports; ++i) {
        जइ (strcmp(i_module->exports[i].name, i_name) == 0) {
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
M3Result LinkImports (IM3Runtime io_runtime, IM3Module io_module)
{
    M3Result result = m3Err_none;

    प्रति (u32 i = 0; i < io_module->अङ्कः_Functions; ++i) {
        IM3Function f = &io_module->functions[i];

        जइ (f->wasm or not (f->import.moduleUtf8 and f->import.fieldUtf8)) {
            अनुवर्त;
        }

        IM3Module from = m3_FindModule(io_runtime, f->import.moduleUtf8);
        जइ (not from) {
            अनुवर्त;
        }

        IM3Function exported = Module_FindExportedFunction(from, f->import.fieldUtf8);
        जइ (not exported) {
            // the import names a module that is loaded, so its exports settle the
            // question: a name it exports as something else is a type mismatch, and
            // one it does not export at all is an unknown import
            _throwif(m3Err_incompatibleImportType, Module_HasExport(from, f->import.fieldUtf8));
            _throw(m3Err_unknownImport);
        }

        // func types are canonical within an environment, so this is the
        // structural equivalence the spec asks for
        _throwif(m3Err_incompatibleImportType, exported->funcType != f->funcType);

        f->resolved = Function_Implementation(exported);
    }

    प्रति (u32 i = 0; i < io_module->अङ्कः_Memories; ++i) {
        IM3Memory memory = io_module->memories[i];

        जइ (not memory->imported or memory->owner != io_module) {
            अनुवर्त;
        }

        IM3Module from = m3_FindModule(io_runtime, memory->import.moduleUtf8);
        जइ (not from) {
            अनुवर्त;
        }

        IM3Memory exported = Module_FindExportedMemory(from, memory->import.fieldUtf8);
        जइ (not exported) {
            _throwif(m3Err_incompatibleImportType, Module_HasExport(from, memory->import.fieldUtf8));
            _throw(m3Err_unknownImport);
        }

        // the address type is part of the memory type, so an i64 memory does
        // not satisfy an i32 import, or the other way round - and so is the page
        // size, which custom page sizes made a declared property of a memory
        _throwif(m3Err_incompatibleImportType, exported->isMemory64 != memory->isMemory64);
        _throwif(m3Err_incompatibleImportType, Memory_PageSize(exported) != Memory_PageSize(memory));

        // a shared memory is not a subtype of an unshared one, or the other way round
        _throwif(m3Err_incompatibleImportType, exported->isShared != memory->isShared);

        _throwif(m3Err_incompatibleImportType,
                 not LimitsSatisfy(exported->अङ्कः_Pages, exported->hasMax, exported->maxPages,
                                   memory->initPages, memory->hasMax, memory->maxPages));

        // hand the slot over to the exporter's memory, and drop the placeholder
        FreeMemoryBlock(memory);
        FreeImportInfo(&memory->import);
        m3_Free(memory);

        io_module->memories[i] = exported;
    }

    प्रति (u32 i = 0; i < io_module->अङ्कः_Tables; ++i) {
        IM3Table table = io_module->tables[i];

        जइ (not table->imported or table->owner != io_module) {
            अनुवर्त;
        }

        IM3Module from = m3_FindModule(io_runtime, table->import.moduleUtf8);
        जइ (not from) {
            अनुवर्त;
        }

        IM3Table exported = Module_FindExportedTable(from, table->import.fieldUtf8);
        जइ (not exported) {
            _throwif(m3Err_incompatibleImportType, Module_HasExport(from, table->import.fieldUtf8));
            _throw(m3Err_unknownImport);
        }

        _throwif(m3Err_incompatibleImportType, exported->type != table->type);

        // the index type is part of the table type, the same way it is for a memory
        _throwif(m3Err_incompatibleImportType, exported->isTable64 != table->isTable64);

        _throwif(m3Err_incompatibleImportType,
                 not LimitsSatisfy(exported->size, exported->hasMax, exported->maxSize,
                                   table->initSize, table->hasMax, table->maxSize));

        // hand the slot over to the exporter's table, and drop the placeholder
        m3_Free(table->elements);
        FreeImportInfo(&table->import);
        m3_Free(table);

        io_module->tables[i] = exported;
    }

    प्रति (u32 i = 0; i < io_module->अङ्कः_Globals; ++i) {
        IM3Global g = &io_module->globals[i];

        जइ (not g->imported or not (g->import.moduleUtf8 and g->import.fieldUtf8)) {
            अनुवर्त;
        }

        IM3Module from = m3_FindModule(io_runtime, g->import.moduleUtf8);
        जइ (not from) {
            अनुवर्त;
        }

        IM3Global exported = Module_FindExportedGlobal(from, g->import.fieldUtf8);
        जइ (not exported) {
            _throwif(m3Err_incompatibleImportType, Module_HasExport(from, g->import.fieldUtf8));
            _throw(m3Err_unknownImport);
        }

        _throwif(m3Err_incompatibleImportType, exported->type != g->type);
        _throwif(m3Err_incompatibleImportType, exported->isMutable != g->isMutable);

        g->resolved = exported->resolved ? exported->resolved : exported;
    }

#जइ d_m3HasExceptionHandling || d_m3_Has_राशिः_Switching
    प्रति (u32 i = 0; i < io_module->अङ्कः_Tags; ++i) {
        IM3Tag tag = &io_module->tags[i];

        जइ (not tag->imported or not (tag->import.moduleUtf8 and tag->import.fieldUtf8)) {
            अनुवर्त;
        }

        IM3Module from = m3_FindModule(io_runtime, tag->import.moduleUtf8);
        जइ (not from) {
            अनुवर्त;
        }

        IM3Tag exported = Module_FindExportedTag(from, tag->import.fieldUtf8);
        जइ (not exported) {
            _throwif(m3Err_incompatibleImportType, Module_HasExport(from, tag->import.fieldUtf8));
            _throw(m3Err_unknownImport);
        }

        _throwif(m3Err_incompatibleImportType, exported->type != tag->type);

        tag->resolved = exported->resolved ? exported->resolved : exported;
    }
#endif

    _catch: प्रत्यागम result;
}


// Backs each of the module's memories with pages. LinkImports has already
// pointed any import it could satisfy at the exporting module's memory, and
// those are skipped here. An import nothing satisfied is still backed locally
// from its own declared limits, so that the module remains loadable.
M3Result InitMemory (IM3Runtime io_runtime, IM3Module i_module)
{
    M3Result result = m3Err_none;

    // Fixed from here on: the index space stops changing after parse, and
    // linking has already repointed any slot it was going to.
    i_module->memory0 = i_module->अङ्कः_Memories ? i_module->memories[0]
                                              : &i_module->emptyMemory;

    जइ (i_module->अङ्कः_Memories == 0) {
        // nothing addressable, but _mem still has to point somewhere
        i_module->emptyMemory.owner    = i_module;
        i_module->emptyMemory.pageSize = d_m3DefaultMemPageSize;

_       (ResizeMemory(io_runtime, &i_module->emptyMemory, 0));
    }

    प्रति (u32 i = 0; i < i_module->अङ्कः_Memories; ++i) {
        IM3Memory memory = i_module->memories[i];

        // a slot that already points at another module's memory is that
        // module's to allocate
        जइ (memory->owner != i_module or memory->mallocated) {
            अनुवर्त;
        }

        u32 pageSize = Memory_PageSize(memory);

        memory->pageSize = pageSize;

        // Without a declared maximum a memory may grow to the spec limit of
        // 2^|addrtype|/pagesize pages, which is the usual 65536 at the default
        // page size, 2^48 for a 64-bit memory, and a whole address space of
        // them when a page is a single byte. A declared maximum of zero is a
        // real limit, not the absence of one.
        //
        // 2^64/pagesize overflows a u64 only when a page is a single byte, and
        // a power-of-two page size divides the address space exactly, so the
        // 64-bit division below is 2^64/pagesize written so that it fits.
        जइ (not memory->hasMax) {
            memory->maxPages = memory->isMemory64
                                 ? (pageSize > 1 ? (UINT64_MAX / pageSize) + 1 : UINT64_MAX)
                                 : (0x100000000ull / pageSize);
        }

_       (ResizeMemory(io_runtime, memory, memory->initPages));
    }

    _catch: प्रत्यागम result;
}


M3Result ResizeMemory (IM3Runtime io_runtime, IM3Memory memory, u64 i_अङ्कः_Pages)
{
    M3Result result = m3Err_none;

    u64 अङ्कः_Pages_To_Alloc = i_अङ्कः_Pages;

    जइ (अङ्कः_Pages_To_Alloc <= memory->maxPages) {
        // A 64-bit memory may ask for up to 2^48 pages, which overflows a u64
        // of bytes. Nothing that large can be backed, so refuse it up front
        // rather than multiplying into a wrapped size.
        _throwif("linear memory limitation exceeded",
                 अङ्कः_Pages_To_Alloc > d_m3AddressLimit / memory->pageSize);

        u64 अङ्कः_Page_Bytes = अङ्कः_Pages_To_Alloc * memory->pageSize;

#जइ d_m3MaxLinearMemoryPages > 0
        // the limit is a memory size, counted in default-sized pages; comparing
        // it against a raw page count would make it 65536 times stricter for a
        // module whose pages are one byte
        _throwif("linear memory limitation exceeded",
                 अङ्कः_Page_Bytes > (u64)d_m3MaxLinearMemoryPages * d_m3DefaultMemPageSize);
#endif

        // Limit the amount of memory that gets actually allocated
        जइ (io_runtime->memoryLimit) {
            अङ्कः_Page_Bytes = M3_MIN(अङ्कः_Page_Bytes, (u64)io_runtime->memoryLimit);
        }

        u64 oldLength = memory->mallocated ? memory->mallocated->length : 0;
        u64 otherUsed = io_runtime->memoryBytesUsed - oldLength;
        _throwif(m3Err_memoryLimitExceeded, io_runtime->memoryBytesLimit and
                                              (अङ्कः_Page_Bytes > io_runtime->memoryBytesLimit or
                                               otherUsed > io_runtime->memoryBytesLimit - अङ्कः_Page_Bytes));

        _throwif("linear memory limitation exceeded", अङ्कः_Page_Bytes > (u64)SIZE_MAX - परिमाण(M3MemoryHeader));

#जइ d_m3GuardedMemory

        // A slot holds every address a Wasm access can name, and nothing beyond
        // that: a memory larger than the data part of one would put addresses past
        // the guard, where a fault is nobody's to recognize
        _throwif("linear memory limitation exceeded", अङ्कः_Page_Bytes > d_m3GuardedDataBytes);

        // What makes an access past the end fault is the page after the memory not
        // being committed, so the end has to fall on one of the system's page
        // boundaries. A Wasm page divides evenly into them and always will; a memory
        // that declared a page size of its own may not, and then the bytes between
        // the end of the memory and the end of its last system page would answer
        // instead of trapping. Refusing to back such a memory is the honest way out:
        // the custom page sizes proposal exists for targets far too small to reserve
        // 8GiB per memory in the first place. See d_m3GuardedMemory.
        _throwif("guarded memory needs a page size the system's divides into",
                 (memory->pageSize % m3_HostPageSize()) != 0);

        जइ (memory->guardSlot == रिक्त) {
            memory->guardSlot = Guard_TakeSlot();
            _throwifnull(memory->guardSlot);
        }

        // Growing only ever commits more of the same slot, so the memory never
        // moves - which the interpreter does not rely on, but nothing minds
        _throwif(m3Err_mallocFailed, not Guard_CommitSlot(memory->guardSlot, (size_t)अङ्कः_Page_Bytes));

        memory->mallocated = Guard_SlotHeader(memory->guardSlot);

#अण्णहा

        size_t अङ्कः_Bytes = (size_t)अङ्कः_Page_Bytes + परिमाण(M3MemoryHeader);

        // What is really there, which is not numPages * pageSize: the limit above
        // caps the bytes without capping the page count, so a memory can be short
        // of a page - by design, since a whole 64 KiB page is often more than an
        // MCU has. Measuring the old block by the page count would overstate it,
        // and then realloc never sees the size it already has and copies the
        // whole limit again on every grow past it.
        size_t अङ्कः_Previous_Bytes = memory->mallocated ? memory->mallocated->length + परिमाण(M3MemoryHeader) : 0;

        शून्य* newMem = m3_Realloc("Wasm Linear Memory", memory->mallocated, अङ्कः_Bytes, अङ्कः_Previous_Bytes);
        _throwifnull(newMem);

        memory->mallocated = (M3MemoryHeader*)newMem;

#endif

#जइ d_m3LogRuntime
        M3MemoryHeader* oldMallocated = memory->mallocated;
#endif

        io_runtime->memoryBytesUsed = otherUsed + अङ्कः_Page_Bytes;
        memory->अङ्कः_Pages            = अङ्कः_Pages_To_Alloc;

        memory->mallocated->length  = (size_t)अङ्कः_Page_Bytes;
        memory->mallocated->runtime = io_runtime;
        memory->mallocated->memory  = memory;

        memory->mallocated->max_राशिः = (m3slot_t*)io_runtime->origin_राशिः + io_runtime->अङ्कः_राशिः_Slots;

        m3log(runtime, "resized old: %p; mem: %p; length: %zu; pages: %llu", oldMallocated, memory->mallocated, memory->mallocated->length, (unsigned long long)memory->अङ्कः_Pages);
    } अण्णहा {
        result = m3Err_wasmMemoryOverflow;
    }

    _catch: प्रत्यागम result;
}


शून्य FreeMemoryBlock (IM3Memory io_memory)
{
    जइ (io_memory->mallocated and io_memory->mallocated->runtime) {
        io_memory->mallocated->runtime->memoryBytesUsed -= io_memory->mallocated->length;
    }
#जइ d_m3GuardedMemory
    Guard_GiveSlot(io_memory->guardSlot);
    io_memory->guardSlot = रिक्त;

    io_memory->mallocated = रिक्त;
#अण्णहा
    m3_Free(io_memory->mallocated);
#endif
}


M3Result InitGlobals (IM3Module io_module)
{
    M3Result result = m3Err_none;

    जइ (io_module->अङ्कः_Globals) {
        // placing the globals in their structs isn't good for cache locality, but i don't really know what the global
        // access patterns typically look like yet.

        //          io_module->globalMemory = m3Alloc (m3reg_t, io_module->numGlobals);

        //          if (io_module->globalMemory)
        {
            प्रति (u32 i = 0; i < io_module->अङ्कः_Globals; ++i) {
                M3Global* g = &io_module->globals[i];                           m3log (runtime, "initializing global: %d", i);

                जइ (g->init_व्यञ्जकम्) {
                    bytes_t start = g->init_व्यञ्जकम्;

                    result = EvaluateExpression(io_module, &g->i64_मानम्, g->type, &start, g->init_व्यञ्जकम् + g->init_व्यञ्जकम्_Size);

                    जइ (not result) {
                        // io_module->globalMemory [i] = initValue;
                    } अण्णहा {
                        भङ्ग;
                    }
                } अण्णहा {                                                        m3log (runtime, "importing global");
                }
            }
        }
        //          else result = ErrorModule (m3Err_mallocFailed, io_module, "could allocate globals for module: '%s", io_module->name);
    }

    प्रत्यागम result;
}


M3Result InitDataSegments (IM3Module io_module)
{
    M3Result result = m3Err_none;

    प्रति (u32 i = 0; i < io_module->अङ्कः_Data_Segments; ++i) {
        M3DataSegment* segment = &io_module->dataSegments[i];

        // A passive segment stays available for memory.init until data.drop.
        // An active one is copied here and then counts as dropped.
        जइ (segment->isPassive) {
            अनुवर्त;
        }

        _throwif("data segment memory index out of range",
                 segment->memoryRegion >= io_module->अङ्कः_Memories);

        IM3Memory io_memory = io_module->memories[segment->memoryRegion];

        _throwif("unallocated linear memory", !(io_memory->mallocated));

        // The offset expression has the memory's address type, and is
        // unsigned: an i32 offset of -1 is 4294967295, way out of bounds
        // rather than negative.
        u64     segmentOffset = 0;
        bytes_t start         = segment->init_व्यञ्जकम्;

        जइ (io_memory->isMemory64) {
_           (EvaluateExpression(io_module, &segmentOffset, c_m3Type_i64, &start, segment->init_व्यञ्जकम् + segment->init_व्यञ्जकम्_Size));
        } अण्णहा {
            u32 offset32;
_           (EvaluateExpression(io_module, &offset32, c_m3Type_i32, &start, segment->init_व्यञ्जकम् + segment->init_व्यञ्जकम्_Size));
            segmentOffset = offset32;
        }

        m3log(runtime, "loading data segment: %d; size: %d; offset: %llu", i, segment->size, (unsigned long long)segmentOffset);

        जइ (segmentOffset <= io_memory->mallocated->length &&
            (u64)segment->size <= io_memory->mallocated->length - segmentOffset) {
            u8* dest = m3MemData(io_memory->mallocated) + segmentOffset;
            memcpy(dest, segment->data, segment->size);
        } अण्णहा {
            _throw("data segment out of bounds");
        }

        segment->dropped = true;
    }

    _catch: प्रत्यागम result;
}


// Turns a segment's elements into references. Element expressions are constant
// expressions restricted to ref.null/ref.func, so they're read directly rather
// than run through the compiler.
स्थिर
M3Result ResolveElements (IM3Module io_module, M3ElementSegment* i_segment, शून्य** o_elements)
{
    M3Result result = m3Err_none;

    bytes_t  pos = i_segment->elements;
    cbytes_t end = io_module->elementSectionEnd;

    प्रति (u32 e = 0; e < i_segment->अङ्कः_Elements; ++e) {
        u32   funcIndex;
        शून्य* ref = रिक्त;

        जइ (i_segment->is_व्यञ्जकम्) {
            m3opcode_t opcode;
_           (Read_opcode(&opcode, &pos, end));

            जइ (opcode == c_waOp_refFunc) {
_               (ReadLEB_u32(&funcIndex, &pos, end));
                _throwif("function index out of range", funcIndex >= io_module->अङ्कः_Functions);
                ref = Function_Implementation(&io_module->functions[funcIndex]);
            } अण्णहा जइ (opcode == c_waOp_refNull) {
                i8 waType;
                u8 nullType;
_               (ReadLEB_i7(&waType, &pos, end));
_               (NormalizeType(&nullType, waType));
                _throwif(m3Err_typeMismatch, nullType != i_segment->type);
            } अण्णहा जइ (opcode == c_waOp_getGlobal) {
                // wasm 2.0 lets an element expression read an imported
                // immutable global, which is how one module seeds another's
                // table with a reference it exported.
                u32 globalIndex;
_               (ReadLEB_u32(&globalIndex, &pos, end));
                _throwif(m3Err_globaIndexOutOfBounds, globalIndex >= io_module->अङ्कः_Globals);

                IM3Global global = &io_module->globals[globalIndex];

                _throwif(m3Err_globaIndexOutOfBounds, not global->imported);
                _throwif(m3Err_wasmMalformed, global->isMutable);
                _throwif(m3Err_typeMismatch, BaseTypeOf(global->type) != BaseTypeOf(i_segment->type));

                // read the cell the import was linked to, not the placeholder
                जइ (global->resolved) {
                    global = global->resolved;
                }

                ref = global->ref_मानम्;
            } अण्णहा {
                _throw("constant expression required");
            }

_           (Read_opcode(&opcode, &pos, end));
            _throwif(m3Err_wasmMalformed, opcode != c_waOp_end);
        } अण्णहा {
_           (ReadLEB_u32(&funcIndex, &pos, end));
            _throwif("function index out of range", funcIndex >= io_module->अङ्कः_Functions);
            ref = Function_Implementation(&io_module->functions[funcIndex]);
        }

        o_elements[e] = ref;
    }

    _catch: प्रत्यागम result;
}


M3Result InitTableAndElements (IM3Module io_module)
{
    M3Result result = m3Err_none;

    cbytes_t end = io_module->elementSectionEnd;
    M3Table* table;

    प्रति (u32 i = 0; i < io_module->अङ्कः_Tables; ++i) {
        table = io_module->tables[i];

        // a slot pointing at another module's table is that module's to fill
        जइ (table->owner != io_module) {
            अनुवर्त;
        }

        जइ (table->size) {
            IM3Runtime runtime = io_module->runtime;
            _throwif(m3Err_tableLimitExceeded, runtime->tableElementsLimit and
                                                 table->size > runtime->tableElementsLimit - runtime->tableElementsUsed);
            table->elements = m3_AllocArray(शून्य*, table->size);
            _throwifnull(table->elements);
            runtime->tableElementsUsed += table->size;

            जइ (table->init_व्यञ्जकम्) {
                शून्य*   मानम् = रिक्त;
                bytes_t start = table->init_व्यञ्जकम्;
_               (EvaluateExpression(io_module, &मानम्, BaseTypeOf(table->type),
                                     &start, table->init_व्यञ्जकम् + table->init_व्यञ्जकम्_Size));

                प्रति (u32 e = 0; e < table->size; ++e) {
                    table->elements[e] = मानम्;
                }
            }
        }
    }

    प्रति (u32 i = 0; i < io_module->अङ्कः_Element_Segments; ++i) {
        M3ElementSegment* segment = &io_module->elementSegments[i];

        // Declarative segments only make their functions referenceable, and
        // passive ones wait for table.init, so neither is written out here.
        जइ (segment->mode == c_m3Elem_declarative) {
            segment->dropped = true;
            अनुवर्त;
        }

        जइ (segment->mode == c_m3Elem_passive) {
            जइ (segment->अङ्कः_Elements) {
                segment->resolved = m3_AllocArray(शून्य*, segment->अङ्कः_Elements);
                _throwifnull(segment->resolved);
_               (ResolveElements(io_module, segment, segment->resolved));
            }
            अनुवर्त;
        }

        table = io_module->tables[segment->tableIndex];

        // The offset expression has the table's index type, and is unsigned:
        // an i32 offset of -1 is 4294967295, out of bounds rather than negative.
        u64     offset = 0;
        bytes_t व्यञ्जकम्   = segment->init_व्यञ्जकम्;

        जइ (table->isTable64) {
_           (EvaluateExpression(io_module, &offset, c_m3Type_i64, &व्यञ्जकम्, end));
        } अण्णहा {
            u32 offset32;
_           (EvaluateExpression(io_module, &offset32, c_m3Type_i32, &व्यञ्जकम्, end));
            offset = offset32;
        }

        _throwif("out of bounds table access",
                 offset > table->size or segment->अङ्कः_Elements > table->size - offset);

_       (ResolveElements(io_module, segment, table->elements + offset));

        segment->dropped = true;
    }

    _catch: प्रत्यागम result;
}

M3Result m3_CompileModule (IM3Module io_module)
{
    M3Result result = m3Err_none;

    प्रति (u32 i = 0; i < io_module->अङ्कः_Functions; ++i) {
        IM3Function f = &io_module->functions[i];
        जइ (f->wasm and not f->compiled) {
_           (CompileFunction(f));
        }
    }

    _catch: प्रत्यागम result;
}

#जइ d_m3HasExceptionHandling

M3Exception* NewException (IM3Runtime io_runtime, IM3Tag i_tag, u32 i_अङ्कः_Args)
{
    M3Exception* exception = (M3Exception*)m3_Malloc("M3Exception", परिमाण(M3Exception) + i_अङ्कः_Args * परिमाण(u64));

    जइ (exception) {
        exception->tag     = i_tag;
        exception->अङ्कः_Args = i_अङ्कः_Args;
        exception->reified = false;
        exception->prev    = रिक्त;
        exception->next    = io_runtime->exceptions;

        जइ (exception->next) {
            exception->next->prev = exception;
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
    जइ (i_exception->prev) {
        i_exception->prev->next = i_exception->next;
    } अण्णहा {
        io_runtime->exceptions = i_exception->next;
    }

    जइ (i_exception->next) {
        i_exception->next->prev = i_exception->prev;
    }

    जइ (io_runtime->pendingException == i_exception) {
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

    जाव (exception) {
        M3Exception* next = exception->next;
        m3_Free_Impl(exception);
        exception = next;
    }
}


// Keeps a copy of the exception a call just ended with, for m3_GetExceptionTag,
// or forgets the last one when the call ended any other way. A copy, because
// the original is freed with the rest before the outermost call returns - and
// one a host function's nested call ended with may still be named by an
// exnref further down the stack.
स्थिर
शून्य RecordUncaughtException (IM3Runtime io_runtime, M3Result i_result)
{
    जइ (io_runtime->uncaughtException) {
        m3_Free(io_runtime->uncaughtException);
    }

    M3Exception* exception = io_runtime->pendingException;

    जइ (M3_UNLIKELY(i_result == m3Err_trapUncaughtException) and exception) {
        M3Exception* copy = (M3Exception*)m3_CopyMem(exception, परिमाण(M3Exception) + exception->अङ्कः_Args * परिमाण(u64));

        जइ (copy) {
            copy->next = रिक्त;
            copy->prev = रिक्त;
        }

        io_runtime->uncaughtException = copy;
    }
}

#endif // d_m3HasExceptionHandling


#जइ d_m3HasExceptionHandling || d_m3_Has_राशिः_Switching

IM3Tag m3_FindTag (IM3Module i_module, ध्रुव वर्ण* ध्रुव i_tagName)
{
    जइ (not i_module or not i_tagName) {
        प्रत्यागम रिक्त;
    }

    प्रत्यागम Module_FindExportedTag(i_module, i_tagName);
}


M3Result m3_NewTag (IM3Runtime io_runtime, IM3Tag* o_tag, ध्रुव वर्ण* ध्रुव i_signature)
{
    IM3FuncType type    = रिक्त;
    M3HostTag*  hostTag = रिक्त;

_try {
    _throwif("null tag", not o_tag);
    *o_tag = रिक्त;
    _throwif(m3Err_mallocFailed, not io_runtime);

_   (SignatureToFuncType(&type, i_signature));
    // tags are the same type exactly when their types are the same object
_   (Environment_AddFuncType(io_runtime->environment, &type));

    hostTag = m3_AllocStruct(M3HostTag);
    _throwifnull(hostTag);

    hostTag->tag.type    = type;
    hostTag->next        = io_runtime->hostTags;
    io_runtime->hostTags = hostTag;

    *o_tag = &hostTag->tag;
} _catch:
    प्रत्यागम result;
}


M3Result m3_LinkTag (IM3Module         io_module,
                     ध्रुव वर्ण* ध्रुव i_moduleName,
                     ध्रुव वर्ण* ध्रुव i_tagName,
                     IM3Tag            i_tag)
{
    जइ (not io_module or not i_moduleName or not i_tagName or not i_tag) {
        प्रत्यागम m3Err_tagLookupFailed;
    }

    ध्रुव bool wildcardModule = (strcmp(i_moduleName, "*") == 0);
    IM3Tag     target         = i_tag->resolved ? i_tag->resolved : i_tag;

    M3Result result = m3Err_tagLookupFailed;

    प्रति (u32 i = 0; i < io_module->अङ्कः_Tags; ++i) {
        IM3Tag tag = &io_module->tags[i];

        जइ (tag->imported and tag->import.moduleUtf8 and tag->import.fieldUtf8 and
            strcmp(tag->import.fieldUtf8, i_tagName) == 0 and
            (wildcardModule or strcmp(tag->import.moduleUtf8, i_moduleName) == 0)) {
            जइ (tag->type != target->type) {
                प्रत्यागम m3Err_incompatibleImportType;
            }

            tag->resolved = target;
            result        = m3Err_none;
        }
    }

    प्रत्यागम result;
}


uint32_t m3_Get_Tag_Arg_गणनम् (IM3Tag i_tag)
{
    प्रत्यागम i_tag ? i_tag->type->अङ्कः_Args : 0;
}


M3_मानम्_Type m3_GetTagArgType (IM3Tag i_tag, uint32_t i_index)
{
    जइ (i_tag and i_index < i_tag->type->अङ्कः_Args) {
        प्रत्यागम (M3_मानम्_Type)BaseTypeOf(d_FuncArgType(i_tag->type, i_index));
    }

    प्रत्यागम c_m3Type_none;
}

#अण्णहा

IM3Tag m3_FindTag (IM3Module i_module, ध्रुव वर्ण* ध्रुव i_tagName)
{
    प्रत्यागम रिक्त;
}

M3Result m3_NewTag (IM3Runtime io_runtime, IM3Tag* o_tag, ध्रुव वर्ण* ध्रुव i_signature)
{
    जइ (o_tag) {
        *o_tag = रिक्त;
    }
    प्रत्यागम m3Err_tagLookupFailed;
}

M3Result m3_LinkTag (IM3Module io_module, ध्रुव वर्ण* ध्रुव i_moduleName, ध्रुव वर्ण* ध्रुव i_tagName, IM3Tag i_tag)
{
    प्रत्यागम m3Err_tagLookupFailed;
}

uint32_t m3_Get_Tag_Arg_गणनम् (IM3Tag i_tag)
{
    प्रत्यागम 0;
}

M3_मानम्_Type m3_GetTagArgType (IM3Tag i_tag, uint32_t i_index)
{
    प्रत्यागम c_m3Type_none;
}

#endif // d_m3HasExceptionHandling || d_m3HasStackSwitching


#जइ d_m3HasExceptionHandling

M3Result m3_ThrowException (IM3Runtime io_runtime, IM3Tag i_tag, uint32_t i_argc, ध्रुव शून्य* i_argptrs[])
{
    जइ (not io_runtime or not i_tag) {
        प्रत्यागम m3Err_tagLookupFailed;
    }

    IM3Tag      tag  = i_tag->resolved ? i_tag->resolved : i_tag;
    IM3FuncType type = tag->type;

    // a tag with results is a stack switching control tag, which is suspended
    // with rather than thrown
    जइ (type->अङ्कः_Rets) {
        प्रत्यागम m3Err_typeMismatch;
    }
    जइ (i_argc != type->अङ्कः_Args) {
        प्रत्यागम m3_Err_argument_गणनम्_Mismatch;
    }
    // with nothing on the stack there is nothing to unwind, and the exception
    // would sit on the runtime's list until some later call ended
    जइ (io_runtime->callNesting == 0) {
        प्रत्यागम "m3_ThrowException called outside a host function";
    }

    M3Exception* exception = NewException(io_runtime, tag, i_argc);
    जइ (not exception) {
        प्रत्यागम m3Err_mallocFailed;
    }

    // the layout op_Throw gives a payload: 32-bit values zero-extended, floats
    // as their bit pattern
    प्रति (u32 i = 0; i < i_argc; ++i) {
        ध्रुव शून्य* मानम् = i_argptrs[i];

        विकल्प (BaseTypeOf(d_FuncArgType(type, i))) {
        पक्ष c_m3Type_i32:
        पक्ष c_m3Type_f32: {
            u32 bits;
            memcpy(&bits, मानम्, परिमाण(bits));
            exception->args[i] = bits;
            भङ्ग;
        }
        पक्ष c_m3Type_i64:
        पक्ष c_m3Type_f64:
            memcpy(&exception->args[i], मानम्, परिमाण(u64));
            भङ्ग;
        पक्ष c_m3Type_funcref:
        पक्ष c_m3Type_externref:
        पक्ष c_m3Type_exnref:
        पक्ष c_m3Type_contref:
            exception->args[i] = (u64)(*(ध्रुव uintptr_t*)मानम्);
            भङ्ग;
        स्वभाव:
            FreeException(io_runtime, exception);
            प्रत्यागम "unknown argument type";
        }
    }

    io_runtime->pendingException = exception;

    प्रत्यागम m3Err_pendingException;
}


IM3Tag m3_GetExceptionTag (IM3Runtime i_runtime)
{
    जइ (i_runtime and i_runtime->uncaughtException) {
        प्रत्यागम i_runtime->uncaughtException->tag;
    }

    प्रत्यागम रिक्त;
}


M3Result m3_GetExceptionArgs (IM3Runtime i_runtime, uint32_t i_argc, ध्रुव शून्य* o_argptrs[])
{
    जइ (not i_runtime or not i_runtime->uncaughtException) {
        प्रत्यागम "no uncaught exception";
    }

    ध्रुव M3Exception* exception = i_runtime->uncaughtException;
    IM3FuncType        type      = exception->tag->type;

    जइ (i_argc != exception->अङ्कः_Args) {
        प्रत्यागम m3_Err_argument_गणनम्_Mismatch;
    }

    प्रति (u32 i = 0; i < i_argc; ++i) {
        शून्य* मानम् = (शून्य*)o_argptrs[i];
        u64   bits  = exception->args[i];

        विकल्प (BaseTypeOf(d_FuncArgType(type, i))) {
        पक्ष c_m3Type_i32:
        पक्ष c_m3Type_f32: {
            u32 नीचः = (u32)bits;
            memcpy(मानम्, &नीचः, परिमाण(नीचः));
            भङ्ग;
        }
        पक्ष c_m3Type_i64:
        पक्ष c_m3Type_f64:
            memcpy(मानम्, &bits, परिमाण(bits));
            भङ्ग;
        पक्ष c_m3Type_funcref:
        पक्ष c_m3Type_externref:
        पक्ष c_m3Type_exnref:
        पक्ष c_m3Type_contref:
            *(uintptr_t*)मानम् = (uintptr_t)bits;
            भङ्ग;
        स्वभाव:
            प्रत्यागम "unknown argument type";
        }
    }

    प्रत्यागम m3Err_none;
}

#अण्णहा

M3Result m3_ThrowException (IM3Runtime io_runtime, IM3Tag i_tag, uint32_t i_argc, ध्रुव शून्य* i_argptrs[])
{
    प्रत्यागम m3Err_tagLookupFailed;
}

IM3Tag m3_GetExceptionTag (IM3Runtime i_runtime)
{
    प्रत्यागम रिक्त;
}

M3Result m3_GetExceptionArgs (IM3Runtime i_runtime, uint32_t i_argc, ध्रुव शून्य* o_argptrs[])
{
    प्रत्यागम "no uncaught exception";
}

#endif // d_m3HasExceptionHandling


#जइ d_m3GuardedMemory

// What RunCodeProtected hands to the platform layer to run, and what comes back
प्रकारनिर्देश संरचना M3ProtectedRun {
    pc_t            pc;
    IM3Runtime      runtime;
    M3MemoryHeader* mem;
    M3Result        result;
} M3ProtectedRun;

स्थिर
शून्य RunCodeBody (शून्य* io_context)
{
    M3ProtectedRun* run = (M3ProtectedRun*)io_context;

#  जइ (d_m3EnableOpProfiling || d_m3EnableOpTracing)
    run->result = (M3Result)RunCode(run->pc, (m3stack_t)run->runtime->राशिः, run->mem,
                                    d_m3OpDefaultArgs, d_m3BaseCstr);
#  अण्णहा
    run->result = (M3Result)RunCode(run->pc, (m3stack_t)run->runtime->राशिः, run->mem,
                                    d_m3OpDefaultArgs);
#  endif
}

// The bounds check the load and store operations no longer do, done once here by the
// system instead: an address past the end of a memory reaches the uncommitted part
// of that memory's reservation, and the fault comes back as the trap the spec asks
// for. Nothing is left half-done by it - the Wasm stack and the memories are heap,
// and the caller unwinds the rest.
स्थिर
M3Result RunCodeProtected (IM3Runtime i_runtime, pc_t i_pc, M3MemoryHeader* i_mem)
{
    // Memory operations in this build carry no bounds check, so code only runs where a
    // fault past the end of a memory can still be turned into a trap
    जइ (M3_UNLIKELY(not m3_HostGuardsActive())) {
        प्रत्यागम ErrorRuntime(m3Err_memoryGuardsUnavailable, i_runtime,
                            "another part of the process holds the fault signal");
    }

    M3ProtectedRun run;
    run.pc      = i_pc;
    run.runtime = i_runtime;
    run.mem     = i_mem;
    run.result  = m3Err_none;

    शून्य*  arena;
    size_t arenaBytes;
    Guard_ArenaRange(&arena, &arenaBytes);

    जइ (not m3_HostProtectedCall(RunCodeBody, &run, arena, arenaBytes)) {
        प्रत्यागम m3Err_trapOutOfBoundsMemoryAccess;
    }

    प्रत्यागम run.result;
}

#endif // d_m3GuardedMemory


#जइ d_m3_Has_राशिः_Switching

// The function a suspended continuation stopped in: the callee of its innermost
// call frame, or the function it was entered with if it made no call. Frames are
// recorded innermost first, so that is the lowest-numbered call frame.
स्थिर
IM3Function Continuation_InnermostFunction (IM3Continuation i_cont)
{
    प्रति (u32 i = 0; i < i_cont->अङ्कः_Frames; ++i) {
        जइ (i_cont->frames[i].kind == frame_call && i_cont->frames[i].call.function) {
            प्रत्यागम i_cont->frames[i].call.function;
        }
    }

    प्रत्यागम i_cont->entryFunction;
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

    जइ (not मूलम् or मूलम्->state != cont_suspended) {
        प्रत्यागम;
    }

    m3stack_t   sp       = मूलम्->sp;
    IM3Function function = Continuation_InnermostFunction(मूलम्);

    जइ (मूलम्->अङ्कः_Frames and मूलम्->frames[0].kind == frame_resume) {
        sp = मूलम्->frames[0].sp;
    }

    m3slot_t* base = (m3slot_t*)io_runtime->origin_राशिः;
    m3slot_t* शिखरम्  = (m3slot_t*)sp + (function ? function->max_राशिः_Slots : 0);

    जइ (शिखरम् > base + io_runtime->अङ्कः_राशिः_Slots) {
        शिखरम् = base + io_runtime->अङ्कः_राशिः_Slots;
    }

    io_runtime->राशिः = शिखरम्;
}

#endif // d_m3HasStackSwitching


// Run compiled code on the runtime's stack, bounding native recursion for the
// duration of the call. The outermost invocation establishes the stack limit;
// nested ones (an imported function calling back into Wasm) inherit it.
स्थिर अन्तःस्थ
M3Result RunCodeChecked (IM3Runtime i_runtime, IM3Function i_function)
{
    pc_t i_pc = i_function->compiled;

    // execution runs against the memory of the module the entry point belongs
    // to, not against some runtime-wide one
    M3MemoryHeader* _mem = Module_MemoryHeader(i_function->module);

#जइ d_m3HasSnapshots
    जइ (M3_UNLIKELY(i_function->module->isUnusable)) {
        प्रत्यागम "a snapshot failed to restore into the module, so it cannot run";
    }
    i_function->module->hasRun = true;
#endif

    d_m3_राशिः_Limit_Enter(i_runtime);

    // a host function calling back into Wasm does not change which call a trap
    // ends: that is still the one the host made first
    जइ (i_runtime->callNesting++ == 0) {
        i_runtime->entered = i_function;
    }
#जइ d_m3HasExceptionHandling
    // handler stacks don't nest across a call boundary: a host function calling
    // back into Wasm cannot be caught by a try_table its own caller entered
    u32 savedTryDepth   = i_runtime->tryDepth;
    i_runtime->tryDepth = 0;
#endif
#जइ d_m3_Has_राशिः_Switching
    // nor do continuations: the native frames on the far side of a host call
    // are not ours to record, so a suspend in here has no handler to reach and
    // no resume to unwind to
    IM3Continuation savedContinuation = i_runtime->activeContinuation;

    // A suspended root continuation is a paused program waiting to be resumed,
    // not scratch space. A call the host makes before resuming it gets a
    // context of its own, the way any other re-entry does, rather than having
    // the frames of the suspension reset out from under it.
    bool claim_मूलम् = i_runtime->isSuspendable and not savedContinuation and
                     not (i_runtime->मूलम्_Continuation and
                          i_runtime->मूलम्_Continuation->state == cont_suspended);

    जइ (claim_मूलम् and not i_runtime->मूलम्_Continuation) {
        i_runtime->मूलम्_Continuation = Continuation_New(i_runtime, i_function->funcType, i_function);

        जइ (i_runtime->मूलम्_Continuation) {
            // The root borrows the runtime's stack and is not charged as cont.new.
            i_runtime->मूलम्_Continuation->अङ्कः_राशिः_Slots = i_runtime->अङ्कः_राशिः_Slots;
        } अण्णहा {
            claim_मूलम् = false;
        }
    }

    जइ (claim_मूलम्) {
        i_runtime->activeContinuation              = i_runtime->मूलम्_Continuation;
        i_runtime->मूलम्_Continuation->entryFunction = i_function;
        i_runtime->मूलम्_Continuation->state         = cont_running;
        i_runtime->मूलम्_Continuation->अङ्कः_Frames     = 0;
    } अण्णहा {
        i_runtime->activeContinuation = रिक्त;
    }
#endif
#जइ d_m3GuardedMemory
    M3Result result = RunCodeProtected(i_runtime, i_pc, _mem);
#नोचेद्यदि (d_m3EnableOpProfiling || d_m3EnableOpTracing)
    M3Result result = (M3Result)RunCode(i_pc, (m3stack_t)i_runtime->राशिः, _mem, d_m3OpDefaultArgs, d_m3BaseCstr);
#अण्णहा
    M3Result result = (M3Result)RunCode(i_pc, (m3stack_t)i_runtime->राशिः, _mem, d_m3OpDefaultArgs);
#endif
#जइ d_m3_Has_राशिः_Switching
    जइ (claim_मूलम्) {
        i_runtime->मूलम्_Continuation->state =
          (result == m3Err_continuationSuspended) ? cont_suspended : cont_returned;

        Runtime_Place_Call_राशिः(i_runtime);
    }
    i_runtime->activeContinuation = savedContinuation;
#endif
#जइ d_m3HasExceptionHandling
    i_runtime->tryDepth = savedTryDepth;

    // an exception that reached the bottom of the call stack found no handler
    जइ (M3_UNLIKELY(result == m3Err_pendingException)) {
        result = m3Err_trapUncaughtException;
    }
    RecordUncaughtException(i_runtime, result);

    // a paused invocation can still reach the exceptions it holds, so they wait
    // for it to finish
    जइ (i_runtime->callNesting == 1 and not m3_IsSuspended(i_runtime)) {
        FreeExceptions(i_runtime);
    }
#endif
    i_runtime->callNesting--;
    d_m3_राशिः_Limit_Leave(i_runtime);

    प्रत्यागम result;
}

M3Result m3_RunStart (IM3Module io_module)
{
#ifdef FUZZING_BUILD_MODE_UNSAFE_FOR_PRODUCTION
    // Execution disabled for fuzzing builds
    प्रत्यागम m3Err_none;
#endif

    M3Result result           = m3Err_none;
    i32      startFunctionTmp = -1;

    जइ (io_module and io_module->startFunction >= 0) {
        IM3Function function = &io_module->functions[io_module->startFunction];

        जइ (not function->compiled) {
_           (CompileFunction(function));
        }

        IM3FuncType ftype = function->funcType;
        जइ (ftype->अङ्कः_Args != 0 || ftype->अङ्कः_Rets != 0) {
            _throw(m3_Err_argument_गणनम्_Mismatch);
        }

        IM3Module  module  = function->module;
        IM3Runtime runtime = module->runtime;

        startFunctionTmp = io_module->startFunction;

        io_module->startFunction = -1;

        result = RunCodeChecked(runtime, function);

        // a start function that suspended is not to be started again: resuming
        // is what finishes it
        जइ (result and result != m3Err_continuationSuspended) {
            io_module->startFunction = startFunctionTmp;
            EXCEPTION_PRINT(result);
            गच्छ _catch;
        }
    }

    _catch: प्रत्यागम result;
}

// TODO: deal with main + side-modules loading efforcement
M3Result m3_LoadModule (IM3Runtime io_runtime, IM3Module io_module)
{
    M3Result result = m3Err_none;

    जइ (M3_UNLIKELY(io_module->runtime)) {
        प्रत्यागम m3Err_moduleAlreadyLinked;
    }

    io_module->runtime = io_runtime;

    // linking first: a memory import has to be resolved before InitMemory would
    // give it pages of its own, and a global import before an initializer reads it
_   (LinkImports(io_runtime, io_module));

_   (InitMemory(io_runtime, io_module));
_   (InitGlobals(io_module));
    // Spec order: element segments are applied before data segments. It matters
    // when one of them traps - whatever ran before the trap stays done.
_   (InitTableAndElements(io_module));
_   (InitDataSegments(io_module));

    // Start func might use imported functions, which are not liked here yet,
    // so it will be called before a function call is attempted (in m3_FindFunction)

#ifdef DEBUG
    Module_GenerateNames(io_module);
#endif

    io_module->next     = io_runtime->modules;
    io_runtime->modules = io_module;
    प्रत्यागम result; // ok

_catch:
    // The runtime owns the module either way. Instantiation may already have
    // written this module's functions into a table another module owns, and the
    // spec keeps whatever it managed to do before the trap, so those entries
    // stay callable - retaining the module is what stops them dangling.
    //
    // It goes on the tail of the list rather than the head: it is not a module
    // anyone should find by name, only one whose functions may still be
    // reachable through someone else's table.
    io_module->next = रिक्त;

    IM3Module* tail = &io_runtime->modules;
    जाव (*tail) {
        tail = &(*tail)->next;
    }
    *tail = io_module;

    प्रत्यागम result;
}

IM3Global m3_FindGlobal (IM3Module         io_module,
                         ध्रुव वर्ण* ध्रुव i_globalName)
{
    // Search exports
    IM3Global exported = Module_FindExportedGlobal(io_module, i_globalName);
    जइ (exported) {
        // a re-exported global is the one that was imported, so reads and
        // writes have to reach the cell that actually holds the value
        प्रत्यागम exported->resolved ? exported->resolved : exported;
    }

    // Search imports
    प्रति (u32 i = 0; i < io_module->अङ्कः_Globals; ++i) {
        IM3Global g = &io_module->globals[i];

        जइ (g->import.moduleUtf8 and g->import.fieldUtf8) {
            जइ (strcmp(g->import.fieldUtf8, i_globalName) == 0) {
                प्रत्यागम g->resolved ? g->resolved : g;
            }
        }
    }
    प्रत्यागम रिक्त;
}

M3Result m3_GetGlobal (IM3Global      i_global,
                       IM3_Tagged_मानम् o_मानम्)
{
    जइ (not i_global) {
        प्रत्यागम m3Err_globalLookupFailed;
    }

    विकल्प (i_global->type) {
    पक्ष c_m3Type_i32: o_मानम्->मानम्.i32 = i_global->i32_मानम्; भङ्ग;
    पक्ष c_m3Type_i64: o_मानम्->मानम्.i64 = i_global->i64_मानम्; भङ्ग;
#जइ d_m3HasFloat
    पक्ष c_m3Type_f32: o_मानम्->मानम्.f32 = i_global->f32_मानम्; भङ्ग;
    पक्ष c_m3Type_f64: o_मानम्->मानम्.f64 = i_global->f64_मानम्; भङ्ग;
#endif
    स्वभाव: प्रत्यागम m3Err_invalidTypeId;
    }

    o_मानम्->type = (M3_मानम्_Type)(i_global->type);
    प्रत्यागम m3Err_none;
}

M3Result m3_SetGlobal (IM3Global            i_global,
                       ध्रुव IM3_Tagged_मानम् i_मानम्)
{
    जइ (not i_global) {
        प्रत्यागम m3Err_globalLookupFailed;
    }
    जइ (not i_global->isMutable) {
        प्रत्यागम m3Err_globalNotMutable;
    }
    जइ (i_global->type != i_मानम्->type) {
        प्रत्यागम m3Err_globalTypeMismatch;
    }

    विकल्प (i_मानम्->type) {
    पक्ष c_m3Type_i32: i_global->i32_मानम् = i_मानम्->मानम्.i32; भङ्ग;
    पक्ष c_m3Type_i64: i_global->i64_मानम् = i_मानम्->मानम्.i64; भङ्ग;
#जइ d_m3HasFloat
    पक्ष c_m3Type_f32: i_global->f32_मानम् = i_मानम्->मानम्.f32; भङ्ग;
    पक्ष c_m3Type_f64: i_global->f64_मानम् = i_मानम्->मानम्.f64; भङ्ग;
#endif
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
    जइ (exported) {
        प्रत्यागम exported;
    }

    // Search internal functions
    प्रति (u32 i = 0; i < i_module->अङ्कः_Functions; ++i) {
        IM3Function f = &i_module->functions[i];

        bool isImported = f->import.moduleUtf8 or f->import.fieldUtf8;

        जइ (isImported) {
            अनुवर्त;
        }

        प्रति (पूर्णाङ्क j = 0; j < f->अङ्कः_Names; j++) {
            जइ (f->names[j] and strcmp(f->names[j], i_name) == 0) {
                प्रत्यागम f;
            }
        }
    }

    प्रत्यागम रिक्त;
}


// Shared tail of the two lookups: a function is only usable once it has code.
स्थिर
M3Result PrepareFoundFunction (IM3Function* o_function, IM3Function i_function)
{
    M3Result result = m3Err_none;

    जइ (not i_function->compiled) {
_       (CompileFunction(i_function))
    }

    _catch:
    *o_function = result ? रिक्त : i_function;

    प्रत्यागम result;
}


// Searches every module in the runtime, most recently loaded first. That is a
// guess once more than one module is loaded and two of them export the same
// name - m3_FindFunctionIn says which module is meant.
M3Result m3_FindFunction (IM3Function* o_function, IM3Runtime i_runtime, ध्रुव वर्ण* ध्रुव i_functionName)
{
                                                                d_m3Assert (o_function and i_runtime and i_functionName);
    IM3Function function = रिक्त;

    जइ (not i_runtime->modules) {
        *o_function = रिक्त;
        प्रत्यागम "no modules loaded";
    }

    function = (IM3Function)ForEachModule(i_runtime, v_FindFunction, (शून्य*)i_functionName);

    जइ (not function) {
        *o_function = रिक्त;
        प्रत्यागम ErrorModule(m3Err_functionLookupFailed, i_runtime->modules, "'%s'", i_functionName);
    }

    प्रत्यागम PrepareFoundFunction(o_function, function);
}


IM3Module m3_FindModule (IM3Runtime i_runtime, ध्रुव वर्ण* ध्रुव i_moduleName)
{
    जइ (not i_runtime or not i_moduleName) {
        प्रत्यागम रिक्त;
    }

    // the list is newest-first, so a name registered twice names the newer one
    प्रति (IM3Module m = i_runtime->modules; m; m = m->next) {
        जइ (m->name and strcmp(m->name, i_moduleName) == 0) {
            प्रत्यागम m;
        }
    }

    प्रत्यागम रिक्त;
}


// Searches one module's exports, which is what naming a module means.
M3Result m3_FindFunctionIn (IM3Function* o_function, IM3Module i_module, ध्रुव वर्ण* ध्रुव i_functionName)
{
                                                                d_m3Assert (o_function and i_functionName);
    जइ (not i_module) {
        *o_function = रिक्त;
        प्रत्यागम m3Err_functionLookupFailed;      // ErrorModule would deref it
    }

    IM3Function function = (IM3Function)v_FindFunction(i_module, (शून्य*)i_functionName);

    जइ (not function) {
        *o_function = रिक्त;
        प्रत्यागम ErrorModule(m3Err_functionLookupFailed, i_module, "'%s'", i_functionName);
    }

    प्रत्यागम PrepareFoundFunction(o_function, function);
}


M3Result m3_GetTableFunction (IM3Function* o_function, IM3Module i_module, uint32_t i_index)
{
_try {
    M3Table*    table;
    IM3Function function;

    _throwif("no table", i_module->अङ्कः_Tables == 0);

    table = i_module->tables[0];
    _throwif("function index out of range", i_index >= table->size);

    function = (IM3Function)table->elements[i_index];

    जइ (function) {
        जइ (not function->compiled) {
_           (CompileFunction(function))
        }
    }

    *o_function = function;
} _catch:
    प्रत्यागम result;
}


स्थिर
M3Result checkStartFunction (IM3Module i_module)
{
    M3Result result = m3Err_none;                               d_m3Assert(i_module);

    // Check if start function needs to be called
    जइ (i_module->startFunction >= 0) {
        result = m3_RunStart(i_module);
    }

    प्रत्यागम result;
}

uint32_t m3_Get_Arg_गणनम् (IM3Function i_function)
{
    जइ (i_function) {
        IM3FuncType ft = i_function->funcType;
        जइ (ft) {
            प्रत्यागम ft->अङ्कः_Args;
        }
    }
    प्रत्यागम 0;
}

uint32_t m3_Get_Ret_गणनम् (IM3Function i_function)
{
    जइ (i_function) {
        IM3FuncType ft = i_function->funcType;
        जइ (ft) {
            प्रत्यागम ft->अङ्कः_Rets;
        }
    }
    प्रत्यागम 0;
}


M3_मानम्_Type m3_GetArgType (IM3Function i_function, uint32_t index)
{
    जइ (i_function) {
        IM3FuncType ft = i_function->funcType;
        जइ (ft and index < ft->अङ्कः_Args) {
            प्रत्यागम (M3_मानम्_Type)BaseTypeOf(d_FuncArgType(ft, index));
        }
    }
    प्रत्यागम c_m3Type_none;
}

M3_मानम्_Type m3_GetRetType (IM3Function i_function, uint32_t index)
{
    जइ (i_function) {
        IM3FuncType ft = i_function->funcType;
        जइ (ft and index < ft->अङ्कः_Rets) {
            प्रत्यागम (M3_मानम्_Type)BaseTypeOf(d_FuncRetType(ft, index));
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
M3Result Get_राशिः_Pointer_For_Args (IM3Function i_function, u8** o_राशिः)
{
    IM3Runtime  runtime = i_function->module->runtime;
    IM3FuncType ftype   = i_function->funcType;

    // the stack a paused invocation holds is not available to this one
    size_t used      = (size_t)((u8*)runtime->राशिः - (u8*)runtime->origin_राशिः);
    size_t needed    = ((size_t)ftype->अङ्कः_Rets + ftype->अङ्कः_Args) * परिमाण(u64);
    size_t available = (size_t)runtime->अङ्कः_राशिः_Slots * परिमाण(m3slot_t) - used;

    जइ (needed > available) {
        प्रत्यागम m3_Err_trap_राशिः_Overflow;
    }

    u64* राशिः = (u64*)runtime->राशिः;

    राशिः += ftype->अङ्कः_Rets;

    *o_राशिः = (u8*)राशिः;

    प्रत्यागम m3Err_none;
}


M3Result m3_CallV (IM3Function i_function, ...)
{
    va_list ap;
    va_start(ap, i_function);
    M3Result r = m3_CallVL(i_function, ap);
    va_end(ap);
    प्रत्यागम r;
}

स्थिर
शून्य Report_Native_राशिः_Usage ()
{
#जइ d_m3_Log_Native_राशिः
    पूर्णाङ्क राशिः_Used = m3_राशिः_Get_Max();
    fprintf(stderr, "Native stack used: %d\n", राशिः_Used);
#endif
}


M3Result m3_CallVL (IM3Function i_function, va_list i_args)
{
    IM3Runtime  runtime = i_function->module->runtime;
    IM3FuncType ftype   = i_function->funcType;
    M3Result    result  = m3Err_none;
    u8*         s       = रिक्त;

    जइ (!i_function->compiled) {
        प्रत्यागम m3Err_missingCompiledCode;
    }

#जइ d_m3RecordBacktraces
    ClearBacktrace(runtime);
#endif

    m3_राशिः_Check_Init();

_   (checkStartFunction(i_function->module))

_   (Get_राशिः_Pointer_For_Args(i_function, &s))

    प्रति (u32 i = 0; i < ftype->अङ्कः_Args; ++i) {
        विकल्प (d_FuncArgType(ftype, i)) {
        पक्ष c_m3Type_i32:
            *(i32*)(s) = va_arg(i_args, i32);
            s += 8;
            भङ्ग;
        पक्ष c_m3Type_i64:
            *(i64*)(s) = va_arg(i_args, i64);
            s += 8;
            भङ्ग;
        पक्ष c_m3Type_funcref:
        पक्ष c_m3Type_externref:
        पक्ष c_m3Type_exnref:
            *(uintptr_t*)(s) = va_arg(i_args, uintptr_t);
            s += 8;
            भङ्ग;
#जइ d_m3HasFloat
        पक्ष c_m3Type_f32:
            *(f32*)(s) = (f32)va_arg(i_args, f64);
            s += 8;
            भङ्ग; // f32 is passed as f64
        पक्ष c_m3Type_f64:
            *(f64*)(s) = va_arg(i_args, f64);
            s += 8;
            भङ्ग;
#endif
        स्वभाव: प्रत्यागम "unknown argument type";
        }
    }

    result = RunCodeChecked(runtime, i_function);
    Report_Native_राशिः_Usage();

    runtime->lastCalled = result ? रिक्त : i_function;

    _catch: प्रत्यागम result;
}

M3Result m3_Call (IM3Function i_function, uint32_t i_argc, ध्रुव शून्य* i_argptrs[])
{
    IM3Runtime  runtime = i_function->module->runtime;
    IM3FuncType ftype   = i_function->funcType;
    M3Result    result  = m3Err_none;
    u8*         s       = रिक्त;

    जइ (i_argc != ftype->अङ्कः_Args) {
        प्रत्यागम m3_Err_argument_गणनम्_Mismatch;
    }
    जइ (!i_function->compiled) {
        प्रत्यागम m3Err_missingCompiledCode;
    }

#जइ d_m3RecordBacktraces
    ClearBacktrace(runtime);
#endif

    m3_राशिः_Check_Init();

_   (checkStartFunction(i_function->module))

_   (Get_राशिः_Pointer_For_Args(i_function, &s))

    प्रति (u32 i = 0; i < ftype->अङ्कः_Args; ++i) {
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
#जइ d_m3HasFloat
        पक्ष c_m3Type_f32:
            *(f32*)(s) = *(f32*)i_argptrs[i];
            s += 8;
            भङ्ग;
        पक्ष c_m3Type_f64:
            *(f64*)(s) = *(f64*)i_argptrs[i];
            s += 8;
            भङ्ग;
#endif
        स्वभाव: प्रत्यागम "unknown argument type";
        }
    }

    result = RunCodeChecked(runtime, i_function);

    Report_Native_राशिः_Usage();

    runtime->lastCalled = result ? रिक्त : i_function;

    _catch: प्रत्यागम result;
}

// Argument parsing for m3_CallArgv. Strict on purpose: the whole string has to
// be consumed, so "12abc" is rejected rather than read as 12, and an empty or
// unparsable argument is an error rather than the zero that strtoul with a
// NULL end pointer used to hand back. See wasm3/wasm3#367.
स्थिर
M3Result ParseArgInteger (ccstr_t i_arg, u32 i_अङ्कः_Bits, u64* o_मानम्)
{
    जइ (not i_arg or not *i_arg) {
        प्रत्यागम "empty argument";
    }

    // strtoull would skip leading space, but trailing space is rejected below;
    // accepting one and not the other would just be confusing
    जइ (isspace((unsigned वर्ण)*i_arg)) {
        प्रत्यागम "argument is not a number";
    }

    वर्ण* end = रिक्त;
    u64   मानम्;

    errno = 0;

    // an argument may be spelled signed or unsigned: -1 and 4294967295 name the
    // same i32
    जइ (*i_arg == '-') {
        i64 signed_मानम् = strtoll(i_arg, &end, 10);

        जइ (i_अङ्कः_Bits == 32 and (signed_मानम् < INT32_MIN or signed_मानम् > INT32_MAX)) {
            प्रत्यागम "argument out of range";
        }

        मानम् = (u64)signed_मानम्;
    } अण्णहा {
        मानम् = strtoull(i_arg, &end, 10);

        जइ (i_अङ्कः_Bits == 32 and मानम् > UINT32_MAX) {
            प्रत्यागम "argument out of range";
        }
    }

    जइ (errno == ERANGE) {
        प्रत्यागम "argument out of range";
    }

    जइ (end == i_arg or *end) {
        प्रत्यागम "argument is not a number";
    }

    *o_मानम् = मानम्;

    प्रत्यागम m3Err_none;
}


#जइ d_m3HasFloat
स्थिर
M3Result ParseArgFloat (ccstr_t i_arg, f64* o_मानम्)
{
    जइ (not i_arg or not *i_arg) {
        प्रत्यागम "empty argument";
    }

    जइ (isspace((unsigned वर्ण)*i_arg)) {
        प्रत्यागम "argument is not a number";
    }

    वर्ण* end = रिक्त;

    errno = 0;

    f64 मानम् = strtod(i_arg, &end);

    जइ (end == i_arg or *end) {
        प्रत्यागम "argument is not a number";
    }

    // strtod reports underflow through ERANGE as well, and a denormal result is
    // perfectly usable, so only an overflow to infinity is out of range
    जइ (errno == ERANGE and (मानम् > DBL_MAX or मानम् < -DBL_MAX)) {
        प्रत्यागम "argument out of range";
    }

    *o_मानम् = मानम्;

    प्रत्यागम m3Err_none;
}
#endif


// A reference argument is either the null reference or a host handle written as
// an integer.
स्थिर
M3Result ParseArgReference (ccstr_t i_arg, u64* o_मानम्)
{
    जइ (i_arg and strcmp(i_arg, "null") == 0) {
        *o_मानम् = 0;
        प्रत्यागम m3Err_none;
    }

    प्रत्यागम ParseArgInteger(i_arg, 64, o_मानम्);
}


M3Result m3_CallArgv (IM3Function i_function, uint32_t i_argc, ध्रुव वर्ण* i_argv[])
{
    IM3FuncType ftype   = i_function->funcType;
    IM3Runtime  runtime = i_function->module->runtime;
    M3Result    result  = m3Err_none;
    u8*         s       = रिक्त;

    जइ (i_argc != ftype->अङ्कः_Args) {
        प्रत्यागम m3_Err_argument_गणनम्_Mismatch;
    }
    जइ (!i_function->compiled) {
        प्रत्यागम m3Err_missingCompiledCode;
    }

#जइ d_m3RecordBacktraces
    ClearBacktrace(runtime);
#endif

    m3_राशिः_Check_Init();

_   (checkStartFunction(i_function->module))

_   (Get_राशिः_Pointer_For_Args(i_function, &s))

    प्रति (u32 i = 0; i < ftype->अङ्कः_Args; ++i) {
        u64 मानम् = 0;
#जइ d_m3HasFloat
        f64 fvalue = 0;
#endif
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
#जइ d_m3HasFloat
            // strtof would be less portable
        पक्ष c_m3Type_f32:
_           (ParseArgFloat(i_argv[i], &fvalue)) * (f32*)(s) = (f32)fvalue;
            s += 8;
            भङ्ग;
        पक्ष c_m3Type_f64:
_           (ParseArgFloat(i_argv[i], &fvalue)) * (f64*)(s) = fvalue;
            s += 8;
            भङ्ग;
#endif
        स्वभाव: _throw("unknown argument type");
        }
    }

    result = RunCodeChecked(runtime, i_function);

    Report_Native_राशिः_Usage();

    runtime->lastCalled = result ? रिक्त : i_function;

    _catch: प्रत्यागम result;
}


//u8 * AlignStackPointerTo64Bits (const u8 * i_stack)
//{
//    uintptr_t ptr = (uintptr_t) i_stack;
//    return (u8 *) ((ptr + 7) & ~7);
//}


M3Result m3_GetResults (IM3Function i_function, uint32_t i_retc, ध्रुव शून्य* o_retptrs[])
{
    IM3FuncType ftype   = i_function->funcType;
    IM3Runtime  runtime = i_function->module->runtime;

    जइ (i_retc != ftype->अङ्कः_Rets) {
        प्रत्यागम m3_Err_argument_गणनम्_Mismatch;
    }
    जइ (i_function != runtime->lastCalled) {
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
#जइ d_m3HasFloat
        पक्ष c_m3Type_f32:
            *(f32*)o_retptrs[i] = *(f32*)(s);
            s += 8;
            भङ्ग;
        पक्ष c_m3Type_f64:
            *(f64*)o_retptrs[i] = *(f64*)(s);
            s += 8;
            भङ्ग;
#endif
        स्वभाव: प्रत्यागम "unknown return type";
        }
    }
    प्रत्यागम m3Err_none;
}

M3Result m3_GetResultsV (IM3Function i_function, ...)
{
    va_list ap;
    va_start(ap, i_function);
    M3Result r = m3_GetResultsVL(i_function, ap);
    va_end(ap);
    प्रत्यागम r;
}

M3Result m3_GetResultsVL (IM3Function i_function, va_list o_rets)
{
    IM3Runtime  runtime = i_function->module->runtime;
    IM3FuncType ftype   = i_function->funcType;

    जइ (i_function != runtime->lastCalled) {
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
#जइ d_m3HasFloat
        पक्ष c_m3Type_f32:
            *va_arg(o_rets, f32*) = *(f32*)(s);
            s += 8;
            भङ्ग;
        पक्ष c_m3Type_f64:
            *va_arg(o_rets, f64*) = *(f64*)(s);
            s += 8;
            भङ्ग;
#endif
        स्वभाव: प्रत्यागम "unknown argument type";
        }
    }
    प्रत्यागम m3Err_none;
}

शून्य ReleaseCodePageNoTrack (IM3Runtime i_runtime, IM3CodePage i_codePage)
{
    जइ (i_codePage) {
        IM3CodePage* list;

        bool pageFull = (अङ्कः_Free_पङ्क्तयः(i_codePage) < d_m3_Code_Page_Free_पङ्क्तयः_Threshold);
        जइ (pageFull) {
            list = &i_runtime->pagesFull;
        } अण्णहा {
            list = &i_runtime->pagesOpen;
        }

        प्रक्षिप_Code_Page(list, i_codePage);                         m3log (emit, "release page: %d to queue: '%s'", i_codePage->info.sequence, pageFull ? "full" : "open")
    }
}


IM3CodePage AcquireCodePageWithCapacity (IM3Runtime i_runtime, u32 i_min_पङ्क्तिः_गणनम्)
{
    IM3CodePage page = RemoveCodePageOfCapacity(&i_runtime->pagesOpen, i_min_पङ्क्तिः_गणनम्);

    जइ (not page) {
        page = Environment_AcquireCodePage(i_runtime->environment, i_min_पङ्क्तिः_गणनम्);

        जइ (not page) {
            page = NewCodePage(i_runtime, i_min_पङ्क्तिः_गणनम्);
        }

        जइ (page) {
            i_runtime->अङ्कः_Code_Pages++;
        }
    }

    जइ (page) {                                                  m3log (emit, "acquire page: %d", page->info.sequence);
        i_runtime->अङ्कः_Active_Code_Pages++;
    }

    प्रत्यागम page;
}


IM3CodePage AcquireCodePage (IM3Runtime i_runtime)
{
    प्रत्यागम AcquireCodePageWithCapacity(i_runtime, d_m3_Code_Page_Free_पङ्क्तयः_Threshold);
}


शून्य ReleaseCodePage (IM3Runtime i_runtime, IM3CodePage i_codePage)
{
    जइ (i_codePage) {
        ReleaseCodePageNoTrack(i_runtime, i_codePage);
        i_runtime->अङ्कः_Active_Code_Pages--;

#जइ defined(DEBUG)
        u32 अङ्कः_Open = गणनम्_Code_Pages(i_runtime->pagesOpen);
        u32 अङ्कः_Full = गणनम्_Code_Pages(i_runtime->pagesFull);

        m3log(runtime, "runtime: %p; open-pages: %d; full-pages: %d; active: %d; total: %d", i_runtime, अङ्कः_Open, अङ्कः_Full, i_runtime->अङ्कः_Active_Code_Pages, i_runtime->अङ्कः_Code_Pages);

        d_m3Assert(अङ्कः_Open + अङ्कः_Full + i_runtime->अङ्कः_Active_Code_Pages == i_runtime->अङ्कः_Code_Pages);

#  जइ d_m3LogCodePages
        dump_code_page(i_codePage, /* startPC: */ रिक्त);
#  endif
#endif
    }
}


#जइ d_m3VerboseErrorMessages
M3Result m3Error (M3Result i_result, IM3Runtime i_runtime, IM3Module i_module, IM3Function i_function,
                  ध्रुव वर्ण* ध्रुव i_file, u32 i_पङ्क्तिः_अङ्कः, ध्रुव वर्ण* ध्रुव i_errorMessage, ...)
{
    जइ (i_runtime) {
        i_runtime->error         = (M3ErrorInfo){ .result = i_result, .runtime = i_runtime, .module = i_module, .function = i_function, .file = i_file, .पङ्क्तिः_2 = i_पङ्क्तिः_अङ्कः };
        i_runtime->error.message = i_runtime->error_message;

        va_list args;
        va_start(args, i_errorMessage);
        vsnprintf(i_runtime->error_message, परिमाण(i_runtime->error_message), i_errorMessage, args);
        va_end(args);
    }

    प्रत्यागम i_result;
}
#endif


शून्य m3_GetErrorInfo (IM3Runtime i_runtime, M3ErrorInfo* o_info)
{
    जइ (i_runtime) {
        *o_info = i_runtime->error;
        m3_ResetErrorInfo(i_runtime);
    }
}


शून्य m3_ResetErrorInfo (IM3Runtime i_runtime)
{
    जइ (i_runtime) {
        M3_INIT(i_runtime->error);
        i_runtime->error.message = "";
    }
}

uint8_t* m3_GetMemory (IM3Module i_module, size_t* o_memorySizeInBytes, uint32_t i_memoryIndex)
{
    uint8_t* memory = रिक्त;
    size_t   size   = 0;

    जइ (i_module and i_memoryIndex < i_module->अङ्कः_Memories) {
        IM3Memory mem = i_module->memories[i_memoryIndex];

        जइ (mem->mallocated) {
            size = mem->mallocated->length;

            जइ (size) {
                memory = m3MemData(mem->mallocated);
            }
        }
    }

    जइ (o_memorySizeInBytes) {
        *o_memorySizeInBytes = size;
    }

    प्रत्यागम memory;
}


size_t m3_GetMemorySize (IM3Module i_module, uint32_t i_memoryIndex)
{
    जइ (not i_module or i_memoryIndex >= i_module->अङ्कः_Memories) {
        प्रत्यागम 0;
    }

    IM3Memory mem = i_module->memories[i_memoryIndex];

    प्रत्यागम mem->mallocated ? mem->mallocated->length : 0;
}


size_t m3_GetMemorySizeAt (ध्रुव शून्य* i_memory)
{
    जइ (not i_memory) {
        प्रत्यागम 0;
    }

    // the header sits immediately before the data it describes
    ध्रुव M3MemoryHeader* header = ((ध्रुव M3MemoryHeader*)i_memory) - 1;

    प्रत्यागम header->length;
}


M3Result m3_FindExportedMemory (IM3Module i_module, cstr_t i_name, u32* o_memoryIndex)
{
    जइ (not i_module or not i_name or not o_memoryIndex) {
        प्रत्यागम m3Err_unknownMemory;
    }

    ध्रुव M3Export* e = Module_FindExport(i_module, d_externalKind_memory, i_name);

    जइ (not e) {
        प्रत्यागम m3Err_unknownMemory;
    }

    *o_memoryIndex = e->index;
    प्रत्यागम m3Err_none;
}


bool Module_HasLinkedHostImport (IM3Module i_module, cstr_t i_importModule)
{
    प्रति (u32 i = 0; i < i_module->अङ्कः_Functions; ++i) {
        IM3Function f = &i_module->functions[i];

        // hostMemory is set when a host function is bound to the import, so it
        // is also the mark of one - see CompileRawFunction
        जइ (not f->hostMemory or f->wasm or f->resolved) {
            अनुवर्त;
        }

        जइ (f->import.moduleUtf8 and strcmp(f->import.moduleUtf8, i_importModule) == 0) {
            प्रत्यागम true;
        }
    }

    प्रत्यागम false;
}


M3Result m3_BindImportMemory (IM3Module io_module, cstr_t i_importModule, u32 i_memoryIndex)
{
    जइ (not io_module or not i_importModule) {
        प्रत्यागम m3Err_moduleNotLinked;
    }

    जइ (i_memoryIndex >= io_module->अङ्कः_Memories) {
        प्रत्यागम m3Err_unknownMemory;
    }

    // the memory itself, not the index: linking may have pointed this slot at
    // another module's memory, and growing one reallocates behind it
    IM3Memory memory = io_module->memories[i_memoryIndex];

    ध्रुव bool wildcardModule = (strcmp(i_importModule, "*") == 0);

    प्रति (u32 i = 0; i < io_module->अङ्कः_Functions; ++i) {
        IM3Function f = &io_module->functions[i];

        // only an import that a host function was bound to has a memory to
        // pin: one with a body of its own reads memory through the interpreter,
        // and one resolved to another module's export runs that module's code
        जइ (not f->hostMemory or f->wasm or f->resolved) {
            अनुवर्त;
        }

        जइ (wildcardModule or (f->import.moduleUtf8 and strcmp(f->import.moduleUtf8, i_importModule) == 0)) {
            f->hostMemory = memory;
        }
    }

    प्रत्यागम m3Err_none;
}


M3BacktraceInfo* m3_GetBacktrace (IM3Runtime i_runtime)
{
#जइ d_m3RecordBacktraces
    प्रत्यागम &i_runtime->backtrace;
#अण्णहा
    प्रत्यागम रिक्त;
#endif
}


//-------------------------------------------------------------------------------------------------------------------------------
//  snapshots & suspendable execution
//-------------------------------------------------------------------------------------------------------------------------------

शून्य m3_SetSuspendable (IM3Runtime io_runtime, bool i_suspendable)
{
    जइ (io_runtime) {
        io_runtime->isSuspendable = i_suspendable;
    }
}

शून्य m3_RequestSuspend (IM3Runtime io_runtime)
{
    जइ (io_runtime) {
        io_runtime->suspendRequested = true;
    }
}

bool m3_IsSuspended (IM3Runtime i_runtime)
{
    जइ (!i_runtime) {
        प्रत्यागम false;
    }
#जइ d_m3_Has_राशिः_Switching
    जइ (i_runtime->मूलम्_Continuation && i_runtime->मूलम्_Continuation->state == cont_suspended) {
        प्रत्यागम true;
    }
#endif
    प्रत्यागम false;
}

M3Result m3_ResumeRuntime (IM3Runtime io_runtime)
{
    जइ (!io_runtime) {
        प्रत्यागम m3Err_mallocFailed;
    }
    जइ (!io_runtime->isSuspendable) {
        प्रत्यागम m3Err_none;
    }
#जइ d_m3HasSnapshots
    // a restore that failed leaves nothing to resume, and says so every time
    // rather than resuming nothing successfully
    जइ (io_runtime->मूलम्_Continuation and io_runtime->मूलम्_Continuation->entryFunction and
        io_runtime->मूलम्_Continuation->entryFunction->module->isUnusable) {
        प्रत्यागम "a snapshot failed to restore into the module, so it cannot run";
    }
#endif
#जइ d_m3_Has_राशिः_Switching
    जइ (!io_runtime->मूलम्_Continuation || io_runtime->मूलम्_Continuation->state != cont_suspended) {
        प्रत्यागम m3Err_none;
    }
    // what finishes returns into the bottom of the stack, where it was entered
    io_runtime->राशिः = io_runtime->origin_राशिः;

    // this is the outermost call again, as far as a host function calling back
    // into Wasm from inside it can tell
    io_runtime->entered = io_runtime->मूलम्_Continuation->entryFunction;
    io_runtime->callNesting++;

    d_m3_राशिः_Limit_Enter(io_runtime);
    io_runtime->resumePastCheck = true;
    M3Result result             = (M3Result)ResumeContinuation(io_runtime, io_runtime->मूलम्_Continuation);
    io_runtime->resumePastCheck = false;
    d_m3_राशिः_Limit_Leave(io_runtime);

#  जइ d_m3HasExceptionHandling
    जइ (M3_UNLIKELY(result == m3Err_pendingException)) {
        result = m3Err_trapUncaughtException;
    }
    RecordUncaughtException(io_runtime, result);
#  endif

    io_runtime->callNesting--;

#  जइ d_m3HasExceptionHandling
    जइ (result != m3Err_continuationSuspended and io_runtime->callNesting == 0) {
        FreeExceptions(io_runtime);
    }
#  endif

    // the results are where the paused call left room for them, so a finished
    // resume can be asked for them the way the call itself could have been
    io_runtime->lastCalled = (result == m3Err_none) ? io_runtime->मूलम्_Continuation->entryFunction : रिक्त;

    Runtime_Place_Call_राशिः(io_runtime);
    प्रत्यागम result;
#अण्णहा
    प्रत्यागम m3Err_none;
#endif
}
