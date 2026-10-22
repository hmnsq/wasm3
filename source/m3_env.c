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

    जर (env) {
        _try
        {
            // create FuncTypes for all simple block return ValueTypes.
            // v128 is skipped: it parses as a slot but has no operations.
            साठी (u8 t = c_m3Type_none; t < c_m3_Type_मोजणी; t++) {
                जर (t == c_m3Type_v128) {
                    सुरूठेवा;
                }

                IM3FuncType ftype;
_               (AllocFuncType(&ftype, 1));

                ftype->अंक_Args  = 0;
                ftype->अंक_Rets  = (t == c_m3Type_none) ? 0 : 1;
                ftype->types[0] = t;

                Environment_AddFuncType(env, &ftype);

                env->retFuncTypes[t] = ftype;
            }
        }

        _catch:
        जर (result) {
            m3_FreeEnvironment(env);
            env = रिक्त;
        }
    }

    परत env;
}


शून्य Environment_Release (IM3Environment i_environment)
{
    IM3FuncType ftype = i_environment->funcTypes;

    पर्यंत (ftype) {
        IM3FuncType next = ftype->next;
        m3_Free(ftype);
        ftype = next;
    }

    m3log(runtime, "freeing %d pages from environment", मोजणी_Code_Pages(i_environment->pagesReleased));
    FreeCodePages(&i_environment->pagesReleased);
}


शून्य m3_FreeEnvironment (IM3Environment i_environment)
{
    जर (i_environment) {
        Environment_Release(i_environment);
        m3_Free(i_environment);
    }
}


शून्य m3_SetCustomSectionHandler (IM3Environment i_environment, M3SectionHandler i_handler)
{
    जर (i_environment) {
        i_environment->customSectionHandler = i_handler;
    }
}


// returns the same io_funcType or replaces it with an equivalent that's already in the type linked list
M3Result Environment_AddFuncType (IM3Environment i_environment, IM3FuncType* io_funcType)
{
    IM3FuncType addType = *io_funcType;
    IM3FuncType newType = i_environment->funcTypes;

    पर्यंत (newType) {
        जर (not newType->inRecGroup and AreFuncTypesEqual(newType, addType)) {
            m3_Free(addType);
            थांबा;
        }

        newType = newType->next;
    }

    जर (newType == रिक्त) {
        // a type index has to fit in the heap type field of an m3type_t
        जर (i_environment->अंक_Func_Types >= d_m3_Max_Sane_Types_मोजणी) {
            m3_Free(addType);
            *io_funcType = रिक्त;
            परत "too many distinct function types";
        }

        newType                  = addType;
        newType->canonicalIndex  = i_environment->अंक_Func_Types++;
        newType->next            = i_environment->funcTypes;
        i_environment->funcTypes = newType;
    }

    *io_funcType = newType;

    परत m3Err_none;
}


// Adds a type without looking for an equal one first, and hands back the index
// it was given.
//
// Members of a recursive group do not get to share an entry with a type that
// merely looks like them: the group is their identity, so two groups spelled
// the same way are still two groups, and a type inside one is never the type
// outside it that matches field for field. Reserving the indices up front is
// also what lets the members refer to each other while they are being read.
M3Result Environment_ReserveFuncTypes (IM3Environment i_environment, u32 i_मोजणी, u16* o_firstIndex)
{
    जर ((u64)i_environment->अंक_Func_Types + i_मोजणी > d_m3_Max_Sane_Types_मोजणी) {
        परत "too many distinct function types";
    }

    *o_firstIndex = i_environment->अंक_Func_Types;
    i_environment->अंक_Func_Types += (u16)i_मोजणी;

    परत m3Err_none;
}


शून्य Environment_AdoptFuncType (IM3Environment i_environment, IM3FuncType i_funcType, u16 i_canonicalIndex)
{
    i_funcType->inRecGroup     = true;
    i_funcType->canonicalIndex = i_canonicalIndex;
    i_funcType->next           = i_environment->funcTypes;
    i_environment->funcTypes   = i_funcType;
}


IM3CodePage RemoveCodePageOfCapacity (M3CodePage** io_list, u32 i_minimum_ओळ_मोजणी)
{
    IM3CodePage prev = रिक्त;
    IM3CodePage page = *io_list;

    पर्यंत (page) {
        जर (अंक_Free_ओळी(page) >= i_minimum_ओळ_मोजणी) {             d_m3Assert (page->info.usage_मोजणी == 0);
            IM3CodePage next = page->info.next;
            जर (prev) {
                prev->info.next = next; // mid-list
            } अन्यथा {
                *io_list = next;       // front of list
            }

            थांबा;
        }

        prev = page;
        page = page->info.next;
    }

    परत page;
}


IM3CodePage Environment_AcquireCodePage (IM3Environment i_environment, u32 i_minimum_ओळ_मोजणी)
{
    परत RemoveCodePageOfCapacity(&i_environment->pagesReleased, i_minimum_ओळ_मोजणी);
}


शून्य Environment_ReleaseCodePages (IM3Environment i_environment, IM3CodePage i_codePageList)
{
    IM3CodePage end = i_codePageList;

    पर्यंत (end) {
        end->info.ओळ_Index = 0; // reset page
#जर d_m3RecordBacktraces
        end->info.mapping->size = 0;
#endif // d_m3RecordBacktraces

        IM3CodePage next = end->info.next;
        जर (not next) {
            थांबा;
        }

        end = next;
    }

    जर (end) {
        // push list to front
        end->info.next               = i_environment->pagesReleased;
        i_environment->pagesReleased = i_codePageList;
    }
}


IM3Runtime m3_NewRuntime (IM3Environment i_environment, u32 i_चळत_Size_In_Bytes, शून्य* i_userdata)
{
    IM3Runtime runtime = m3_AllocStruct(M3Runtime);

    जर (runtime) {
        m3_ResetErrorInfo(runtime);

        runtime->environment = i_environment;
        runtime->userdata    = i_userdata;

        Deterministic_InitRuntime(runtime);

        runtime->origin_चळत = m3_Malloc("Wasm Stack", i_चळत_Size_In_Bytes + 4 * आकार(m3slot_t)); // TODO: more precise stack checks

        जर (runtime->origin_चळत) {
            runtime->चळत         = runtime->origin_चळत;
            runtime->अंक_चळत_Slots = i_चळत_Size_In_Bytes / आकार(m3slot_t);          m3log (runtime, "new stack: %p, slots: %u", runtime->origin_चळत, runtime->अंक_चळत_Slots);
        } अन्यथा {
            m3_Free(runtime);
        }
    }

    परत runtime;
}

शून्य m3_SetValidation (IM3Runtime i_runtime, bool i_enable)
{
#जर d_m3EnableValidation
    जर (i_runtime) {
        i_runtime->skipValidation = not i_enable;
    }
#अन्यथा
    (शून्य)i_runtime;
    (शून्य)i_enable;              // nothing to skip: the validator was compiled out
#endif
}

M3Result m3_SetResourceLimit (IM3Runtime i_runtime, M3ResourceLimit i_limit, uint64_t i_मूल्य)
{
    जर (not i_runtime) {
        परत m3Err_mallocFailed;
    }
    निवड (i_limit) {
    स्थिती c_m3Limit_MemoryBytes:
        जर (i_मूल्य and i_मूल्य < i_runtime->memoryBytesUsed) {
            परत m3Err_resourceLimitBelowUsage;
        }
        i_runtime->memoryBytesLimit = i_मूल्य;
        परत m3Err_none;
    स्थिती c_m3Limit_TableElements:
        जर (i_मूल्य and i_मूल्य < i_runtime->tableElementsUsed) {
            परत m3Err_resourceLimitBelowUsage;
        }
        i_runtime->tableElementsLimit = i_मूल्य;
        परत m3Err_none;
#जर d_m3_Has_चळत_Switching
    स्थिती c_m3Limit_Continuations:
        जर (i_मूल्य and i_मूल्य < i_runtime->continuationsAllocated) {
            परत m3Err_resourceLimitBelowUsage;
        }
        i_runtime->continuationsLimit = (u32)M3_MIN(i_मूल्य, (u64)UINT32_MAX);
        पर्यंत (i_मूल्य and i_runtime->val_चळत_Pool_मोजणी > i_मूल्य - i_runtime->continuationsAllocated) {
            m3_Free(i_runtime->val_चळत_Pool[--i_runtime->val_चळत_Pool_मोजणी]);
        }
        परत m3Err_none;
#अन्यथा
    स्थिती c_m3Limit_Continuations: परत m3Err_resourceLimitNotSupported;
#endif
    स्थिती c_m3Limit_GasUnits:
#जर d_m3HasGasMetering
        i_runtime->gasLimit = i_runtime->gasRemaining = (i64)M3_MIN(i_मूल्य, (u64)INT64_MAX);
        परत m3Err_none;
#अन्यथा
        परत m3Err_resourceLimitNotSupported;
#endif
    पूर्वनिर्धारित: परत m3Err_unknownResourceLimit;
    }
}

uint64_t m3_GetResourceLimit (IM3Runtime i_runtime, M3ResourceLimit i_limit)
{
    जर (not i_runtime) {
        परत 0;
    }
    निवड (i_limit) {
    स्थिती c_m3Limit_MemoryBytes: परत i_runtime->memoryBytesLimit;
    स्थिती c_m3Limit_TableElements: परत i_runtime->tableElementsLimit;
#जर d_m3_Has_चळत_Switching
    स्थिती c_m3Limit_Continuations: परत i_runtime->continuationsLimit;
#endif
#जर d_m3HasGasMetering
    स्थिती c_m3Limit_GasUnits: परत (u64)i_runtime->gasLimit;
#endif
    पूर्वनिर्धारित: परत 0;
    }
}

uint64_t m3_GetResourceUsage (IM3Runtime i_runtime, M3ResourceLimit i_limit)
{
    जर (not i_runtime) {
        परत 0;
    }
    निवड (i_limit) {
    स्थिती c_m3Limit_MemoryBytes: परत i_runtime->memoryBytesUsed;
    स्थिती c_m3Limit_TableElements: परत i_runtime->tableElementsUsed;
#जर d_m3_Has_चळत_Switching
    स्थिती c_m3Limit_Continuations: परत i_runtime->continuationsAllocated;
#endif
#जर d_m3HasGasMetering
    स्थिती c_m3Limit_GasUnits: परत (u64)i_runtime->gasLimit - (u64)i_runtime->gasRemaining;
#endif
    पूर्वनिर्धारित: परत 0;
    }
}

शून्य* m3_GetUserData (IM3Runtime i_runtime)
{
    परत i_runtime ? i_runtime->userdata : रिक्त;
}


शून्य* ForEachModule (IM3Runtime i_runtime, ModuleVisitor i_visitor, शून्य* i_info)
{
    शून्य* r = रिक्त;

    IM3Module module = i_runtime->modules;

    पर्यंत (module) {
        IM3Module next = module->next;
        जर ((r = i_visitor(module, i_info))) {
            थांबा;
        }

        module = next;
    }

    परत r;
}


शून्य* _FreeModule (IM3Module i_module, शून्य* i_info)
{
    m3_FreeModule(i_module);
    परत रिक्त;
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
    साठी (u32 i = 0; i < i_module->अंक_Memories; ++i) {
        IM3Memory memory = i_module->memories[i];

        जर (memory and memory->owner != i_module) {
            i_module->memories[i] = रिक्त;
        }
    }

    साठी (u32 i = 0; i < i_module->अंक_Tables; ++i) {
        IM3Table table = i_module->tables[i];

        जर (table and table->owner != i_module) {
            i_module->tables[i] = रिक्त;
        }
    }

    परत रिक्त;
}


शून्य Runtime_Release (IM3Runtime i_runtime)
{
    ForEachModule(i_runtime, _ReleaseBorrowedSlots, रिक्त);
    ForEachModule(i_runtime, _FreeModule, रिक्त);                    d_m3Assert (i_runtime->अंक_Active_Code_Pages == 0);

    Environment_ReleaseCodePages(i_runtime->environment, i_runtime->pagesOpen);
    Environment_ReleaseCodePages(i_runtime->environment, i_runtime->pagesFull);

    m3_Free(i_runtime->origin_चळत);

#जर d_m3EnableValidation
    m3_Free(i_runtime->validator);
#endif

#जर d_m3HasExceptionHandling
    // a runtime let go of while suspended still holds its exceptions
    FreeExceptions(i_runtime);
    m3_Free(i_runtime->uncaughtException);
#endif
#जर d_m3HasExceptionHandling || d_m3_Has_चळत_Switching
    पर्यंत (i_runtime->hostTags) {
        M3HostTag* next = i_runtime->hostTags->next;
        m3_Free(i_runtime->hostTags);
        i_runtime->hostTags = next;
    }
#endif
#जर d_m3_Has_चळत_Switching
    Continuation_ReleaseAll(i_runtime);
#endif
}


शून्य m3_FreeRuntime (IM3Runtime i_runtime)
{
    जर (i_runtime) {
        m3_PrintProfilerInfo();

        Runtime_Release(i_runtime);
        m3_Free(i_runtime);
    }
}


#जर d_m3_Has_चळत_Switching

IM3Continuation Continuation_New (IM3Runtime i_runtime, IM3FuncType i_type, IM3Function i_function)
{
    IM3Continuation cont = m3_AllocStruct(M3Continuation);
    जर (!cont) {
        परत रिक्त;
    }

    cont->type          = i_type;
    cont->entryFunction = i_function;
    cont->state         = cont_allocated;

    cont->अंक_चळत_Slots = d_m3_Continuation_चळत_Slots;
    // frames are recorded only when this continuation actually suspends, and
    // only as deep as it got, so the array starts out unallocated
    cont->frames    = रिक्त;
    cont->framesCap = 0;
    cont->अंक_Frames = 0;

    cont->sp = cont->val_चळत;
    cont->pc = i_function ? i_function->compiled : रिक्त;
    cont->r0 = 0;
#  जर d_m3HasFloat
    cont->fp0 = 0;
#  endif
    cont->bound_Args_मोजणी    = 0;
    cont->अंक_Suspend_Results = 0;
    cont->अंक_Handlers       = 0;
    cont->handlersPC        = रिक्त;
    cont->parent            = रिक्त;

    cont->next               = i_runtime->continuations;
    i_runtime->continuations = cont;

    परत cont;
}

IM3Continuation Continuation_ForkSuspended (IM3Runtime i_runtime, IM3Continuation i_cont)
{
    IM3Continuation newCont = Continuation_New(i_runtime, i_cont->type, i_cont->entryFunction);
    जर (M3_UNLIKELY(not newCont)) {
        परत रिक्त;
    }

    newCont->val_चळत      = i_cont->val_चळत;
    newCont->अंक_चळत_Slots = i_cont->अंक_चळत_Slots;
    newCont->sp            = i_cont->sp;
    newCont->pc            = i_cont->pc;
    newCont->suspendPoint  = i_cont->suspendPoint;
    newCont->r0            = i_cont->r0;
#  जर d_m3HasFloat
    newCont->fp0 = i_cont->fp0;
#  endif
    newCont->frames    = i_cont->frames;
    newCont->framesCap = i_cont->framesCap;
    newCont->अंक_Frames = i_cont->अंक_Frames;

    newCont->bound_Args_मोजणी = 0;
#  जर d_m3HasExceptionHandling
    newCont->resumeThrow = i_cont->resumeThrow;
    i_cont->resumeThrow  = रिक्त;
#  endif
    newCont->अंक_Suspend_Results = i_cont->अंक_Suspend_Results;
    साठी (u32 i = 0; i < i_cont->अंक_Suspend_Results; ++i) {
        newCont->suspendResultOffsets[i] = i_cont->suspendResultOffsets[i];
        newCont->suspendResultIs64[i]    = i_cont->suspendResultIs64[i];
    }

    newCont->अंक_Handlers = i_cont->अंक_Handlers;
    newCont->handlersPC  = i_cont->handlersPC;
    newCont->parent      = i_cont->parent;
    newCont->state       = cont_suspended;

    i_cont->val_चळत  = रिक्त;
    i_cont->frames    = रिक्त;
    i_cont->framesCap = 0;
    i_cont->अंक_Frames = 0;
    i_cont->state     = cont_consumed;

    परत newCont;
}

// Only cont.new and snapshot restoration acquire stacks. Binding and suspension
// transfer them between records without changing the active count.
M3Result Continuation_Acquire_चळत (IM3Runtime runtime, IM3Continuation cont)
{
    जर (not Continuation_Can_Acquire_चळत(runtime)) {
        परत m3Err_continuationLimitExceeded;
    }
    जर (runtime->val_चळत_Pool_मोजणी) {
        cont->val_चळत = runtime->val_चळत_Pool[--runtime->val_चळत_Pool_मोजणी];
        स_रखो(cont->val_चळत, 0, (cont->अंक_चळत_Slots + 4) * आकार(m3slot_t));
    } अन्यथा {
        cont->val_चळत = m3_AllocArray(m3slot_t, cont->अंक_चळत_Slots + 4);
    }
    जर (not cont->val_चळत) {
        परत m3Err_mallocFailed;
    }
    cont->sp = cont->val_चळत;
    runtime->continuationsAllocated++;
    परत m3Err_none;
}

शून्य Continuation_Recycle_चळत (IM3Runtime runtime, IM3Continuation cont)
{
    जर (not cont->val_चळत or cont == runtime->मूळ_Continuation) {
        परत;
    }
    जर (not runtime->val_चळत_Pool) {
        runtime->val_चळत_Pool = m3_AllocArray(m3slot_t*, 16);
        जर (runtime->val_चळत_Pool) {
            runtime->val_चळत_Pool_Cap = 16;
        }
    }
    जर (runtime->val_चळत_Pool_मोजणी < runtime->val_चळत_Pool_Cap and
        (not runtime->continuationsLimit or runtime->val_चळत_Pool_मोजणी <
                                              runtime->continuationsLimit - runtime->continuationsAllocated + 1)) {
        runtime->val_चळत_Pool[runtime->val_चळत_Pool_मोजणी++] = cont->val_चळत;
        cont->val_चळत                                      = रिक्त;
    } अन्यथा {
        m3_Free(cont->val_चळत);
    }
    cont->sp = रिक्त;
    runtime->continuationsAllocated--;
}

शून्य Continuation_ReleaseAll (IM3Runtime io_runtime)
{
    // Records live until teardown: a consumed reference can still be in a table
    // or on a Wasm stack. Its value stack has already been recycled.
    IM3Continuation cont = io_runtime->continuations;

    पर्यंत (cont) {
        IM3Continuation next = cont->next;

        m3_Free(cont->val_चळत);
        m3_Free(cont->frames);
        m3_Free(cont);

        cont = next;
    }

    साठी (u32 i = 0; i < io_runtime->val_चळत_Pool_मोजणी; ++i) {
        m3_Free(io_runtime->val_चळत_Pool[i]);
    }
    m3_Free(io_runtime->val_चळत_Pool);
    io_runtime->val_चळत_Pool_मोजणी = io_runtime->val_चळत_Pool_Cap = 0;
    io_runtime->continuationsAllocated                          = 0;
    io_runtime->continuations                                   = रिक्त;
    io_runtime->activeContinuation                              = रिक्त;
    io_runtime->मूळ_Continuation                                = रिक्त;
}


m3ret_t Continuation_RecordFrame (IM3Runtime i_runtime, स्थिरांक M3Frame* i_frame)
{
    IM3Continuation cont = i_runtime->activeContinuation;

    // the marker only travels between a suspend and the resume that runs it,
    // and that whole stretch has an active continuation
    d_m3Assert(cont);

    जर (cont->अंक_Frames == cont->framesCap) {
        u32 newCap = cont->framesCap ? cont->framesCap * 2 : 8;

        जर (newCap > d_m3ContinuationMaxFrames) {
            newCap = d_m3ContinuationMaxFrames;
        }

        जर (newCap == cont->framesCap) {
            परत m3_Err_trap_चळत_Overflow;
        }

        M3Frame* frames = m3_ReallocArray(M3Frame, cont->frames, newCap, cont->framesCap);

        जर (not frames) {
            परत m3Err_mallocFailed;
        }

        cont->frames    = frames;
        cont->framesCap = newCap;
    }

    cont->frames[cont->अंक_Frames++] = *i_frame;

    परत m3Err_continuationSuspended;
}

#endif // d_m3HasStackSwitching

M3Result EvaluateExpression (IM3Module i_module, शून्य* o_expressed, m3type_t i_type, bytes_t* io_bytes, cbytes_t i_end)
{
    M3Result result = m3Err_none;

    // OPTZ: use a simplified interpreter for expressions

    // create a temporary runtime context
#जर defined(d_m3PreferStaticAlloc)
    स्थिर M3Runtime runtime;
#अन्यथा
    M3Runtime runtime;
#endif
    M3_INIT(runtime);

    runtime.environment   = i_module->runtime->environment;
    runtime.अंक_चळत_Slots = i_module->runtime->अंक_चळत_Slots;
    runtime.चळत         = i_module->runtime->चळत;

    m3stack_t चळत = (m3stack_t)runtime.चळत;

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

    जर (o->page) {
        IM3FuncType ftype = रिक्त;

#जर d_m3HasTypedRefs
        // A reference type has to be checked as the type it is: the shared
        // return types are one per storage class, so borrowing one of those
        // would let any reference at all satisfy a particular one.
        जर (IsSpelledRefType(i_type)) {
            result = AllocFuncType(&ftype, 1);

            जर (not result) {
                ftype->अंक_Rets  = 1;
                ftype->types[0] = i_type;

                result = Environment_AddFuncType(runtime.environment, &ftype);
            }
        } अन्यथा
#endif
        {
            ftype = runtime.environment->retFuncTypes[BaseTypeOf(i_type)];
        }

        pc_t m3code = GetPagePC(o->page);

        जर (not result) {
            result = CompileExpression(o, ftype);
        }

        जर (not result && o->max_चळत_Slots >= runtime.अंक_चळत_Slots) {
            result = m3_Err_trap_चळत_Overflow;
        }

        जर (not result) {
#जर (d_m3EnableOpProfiling || d_m3EnableOpTracing)
            m3ret_t r = RunCode(m3code, चळत, रिक्त, d_m3OpDefaultArgs, d_m3BaseCstr);
#अन्यथा
            m3ret_t r = RunCode(m3code, चळत, रिक्त, d_m3OpDefaultArgs);
#endif

            जर (r == 0) {                                                                   m3log (runtime, "expression result: %s", SPrint_मूल्य (चळत, i_type));
                जर (SizeOfType(BaseTypeOf(i_type)) == आकार(u32)) {
                    *(u32*)o_expressed = *((u32*)चळत);
                } अन्यथा {
                    *(u64*)o_expressed = *((u64*)चळत);
                }
            }
        }

        ReleaseCodePage(&runtime, o->page);
    } अन्यथा {
        result = m3Err_mallocFailedCodePage;
    }

    runtime.origin_चळत = रिक्त;        // prevent free(stack) in ReleaseRuntime
    Runtime_Release(&runtime);
    i_module->runtime = savedRuntime;

    *io_bytes = o->wasm;

    परत result;
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
    जर (i_exportedSize < i_importMin) {
        परत false;
    }

    जर (i_importHasMax and (not i_exportedHasMax or i_exportedMax > i_importMax)) {
        परत false;
    }

    परत true;
}


// The export of this kind under this name, or NULL. The index it carries is
// resolved by the caller, against the module's index space as it stands then -
// after linking, a re-exported import's slot holds the entity it was linked to.
स्थिर
स्थिरांक M3Export* Module_FindExport (IM3Module i_module, u8 i_kind, cstr_t i_name)
{
    साठी (u32 i = 0; i < i_module->अंक_Exports; ++i) {
        स्थिरांक M3Export* e = &i_module->exports[i];

        जर (e->kind == i_kind and म_भेद(e->name, i_name) == 0) {
            परत e;
        }
    }

    परत रिक्त;
}


स्थिर
IM3Function Module_FindExportedFunction (IM3Module i_module, cstr_t i_name)
{
    स्थिरांक M3Export* e = Module_FindExport(i_module, d_externalKind_function, i_name);

    // A module that re-exports an import names the placeholder here. Resolve to
    // the function that actually runs, or a host-side call would run it against
    // the importing module's memory.
    परत e ? Function_Implementation(&i_module->functions[e->index]) : रिक्त;
}


स्थिर
IM3Memory Module_FindExportedMemory (IM3Module i_module, cstr_t i_name)
{
    स्थिरांक M3Export* e = Module_FindExport(i_module, d_externalKind_memory, i_name);

    परत e ? i_module->memories[e->index] : रिक्त;
}


स्थिर
IM3Table Module_FindExportedTable (IM3Module i_module, cstr_t i_name)
{
    स्थिरांक M3Export* e = Module_FindExport(i_module, d_externalKind_table, i_name);

    परत e ? i_module->tables[e->index] : रिक्त;
}


स्थिर
IM3Global Module_FindExportedGlobal (IM3Module i_module, cstr_t i_name)
{
    स्थिरांक M3Export* e = Module_FindExport(i_module, d_externalKind_global, i_name);

    परत e ? &i_module->globals[e->index] : रिक्त;
}


#जर d_m3HasExceptionHandling || d_m3_Has_चळत_Switching
स्थिर
IM3Tag Module_FindExportedTag (IM3Module i_module, cstr_t i_name)
{
    स्थिरांक M3Export* e = Module_FindExport(i_module, d_externalKind_tag, i_name);

    जर (not e) {
        परत रिक्त;
    }

    IM3Tag tag = &i_module->tags[e->index];

    परत tag->resolved ? tag->resolved : tag;
}
#endif


// Whether the module exports anything at all under this name. Export names are
// unique within a module, so a name one of the lookups above missed but this
// one finds is exported as something else - a kind the import cannot be
// satisfied by, rather than a name the module never exported.
स्थिर
bool Module_HasExport (IM3Module i_module, cstr_t i_name)
{
    साठी (u32 i = 0; i < i_module->अंक_Exports; ++i) {
        जर (म_भेद(i_module->exports[i].name, i_name) == 0) {
            परत true;
        }
    }

    परत false;
}


// Points each of the module's imports at whatever already-loaded module exports
// it. Runs before anything is allocated or initialized: a memory import has to
// be resolved before InitMemory would give it pages of its own, and a global
// import before InitGlobals runs an initializer that reads it.
स्थिर
M3Result LinkImports (IM3Runtime io_runtime, IM3Module io_module)
{
    M3Result result = m3Err_none;

    साठी (u32 i = 0; i < io_module->अंक_Functions; ++i) {
        IM3Function f = &io_module->functions[i];

        जर (f->wasm or not (f->import.moduleUtf8 and f->import.fieldUtf8)) {
            सुरूठेवा;
        }

        IM3Module from = m3_FindModule(io_runtime, f->import.moduleUtf8);
        जर (not from) {
            सुरूठेवा;
        }

        IM3Function exported = Module_FindExportedFunction(from, f->import.fieldUtf8);
        जर (not exported) {
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

    साठी (u32 i = 0; i < io_module->अंक_Memories; ++i) {
        IM3Memory memory = io_module->memories[i];

        जर (not memory->imported or memory->owner != io_module) {
            सुरूठेवा;
        }

        IM3Module from = m3_FindModule(io_runtime, memory->import.moduleUtf8);
        जर (not from) {
            सुरूठेवा;
        }

        IM3Memory exported = Module_FindExportedMemory(from, memory->import.fieldUtf8);
        जर (not exported) {
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
                 not LimitsSatisfy(exported->अंक_Pages, exported->hasMax, exported->maxPages,
                                   memory->initPages, memory->hasMax, memory->maxPages));

        // hand the slot over to the exporter's memory, and drop the placeholder
        FreeMemoryBlock(memory);
        FreeImportInfo(&memory->import);
        m3_Free(memory);

        io_module->memories[i] = exported;
    }

    साठी (u32 i = 0; i < io_module->अंक_Tables; ++i) {
        IM3Table table = io_module->tables[i];

        जर (not table->imported or table->owner != io_module) {
            सुरूठेवा;
        }

        IM3Module from = m3_FindModule(io_runtime, table->import.moduleUtf8);
        जर (not from) {
            सुरूठेवा;
        }

        IM3Table exported = Module_FindExportedTable(from, table->import.fieldUtf8);
        जर (not exported) {
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

    साठी (u32 i = 0; i < io_module->अंक_Globals; ++i) {
        IM3Global g = &io_module->globals[i];

        जर (not g->imported or not (g->import.moduleUtf8 and g->import.fieldUtf8)) {
            सुरूठेवा;
        }

        IM3Module from = m3_FindModule(io_runtime, g->import.moduleUtf8);
        जर (not from) {
            सुरूठेवा;
        }

        IM3Global exported = Module_FindExportedGlobal(from, g->import.fieldUtf8);
        जर (not exported) {
            _throwif(m3Err_incompatibleImportType, Module_HasExport(from, g->import.fieldUtf8));
            _throw(m3Err_unknownImport);
        }

        _throwif(m3Err_incompatibleImportType, exported->type != g->type);
        _throwif(m3Err_incompatibleImportType, exported->isMutable != g->isMutable);

        g->resolved = exported->resolved ? exported->resolved : exported;
    }

#जर d_m3HasExceptionHandling || d_m3_Has_चळत_Switching
    साठी (u32 i = 0; i < io_module->अंक_Tags; ++i) {
        IM3Tag tag = &io_module->tags[i];

        जर (not tag->imported or not (tag->import.moduleUtf8 and tag->import.fieldUtf8)) {
            सुरूठेवा;
        }

        IM3Module from = m3_FindModule(io_runtime, tag->import.moduleUtf8);
        जर (not from) {
            सुरूठेवा;
        }

        IM3Tag exported = Module_FindExportedTag(from, tag->import.fieldUtf8);
        जर (not exported) {
            _throwif(m3Err_incompatibleImportType, Module_HasExport(from, tag->import.fieldUtf8));
            _throw(m3Err_unknownImport);
        }

        _throwif(m3Err_incompatibleImportType, exported->type != tag->type);

        tag->resolved = exported->resolved ? exported->resolved : exported;
    }
#endif

    _catch: परत result;
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
    i_module->memory0 = i_module->अंक_Memories ? i_module->memories[0]
                                              : &i_module->emptyMemory;

    जर (i_module->अंक_Memories == 0) {
        // nothing addressable, but _mem still has to point somewhere
        i_module->emptyMemory.owner    = i_module;
        i_module->emptyMemory.pageSize = d_m3DefaultMemPageSize;

_       (ResizeMemory(io_runtime, &i_module->emptyMemory, 0));
    }

    साठी (u32 i = 0; i < i_module->अंक_Memories; ++i) {
        IM3Memory memory = i_module->memories[i];

        // a slot that already points at another module's memory is that
        // module's to allocate
        जर (memory->owner != i_module or memory->mallocated) {
            सुरूठेवा;
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
        जर (not memory->hasMax) {
            memory->maxPages = memory->isMemory64
                                 ? (pageSize > 1 ? (UINT64_MAX / pageSize) + 1 : UINT64_MAX)
                                 : (0x100000000ull / pageSize);
        }

_       (ResizeMemory(io_runtime, memory, memory->initPages));
    }

    _catch: परत result;
}


M3Result ResizeMemory (IM3Runtime io_runtime, IM3Memory memory, u64 i_अंक_Pages)
{
    M3Result result = m3Err_none;

    u64 अंक_Pages_To_Alloc = i_अंक_Pages;

    जर (अंक_Pages_To_Alloc <= memory->maxPages) {
        // A 64-bit memory may ask for up to 2^48 pages, which overflows a u64
        // of bytes. Nothing that large can be backed, so refuse it up front
        // rather than multiplying into a wrapped size.
        _throwif("linear memory limitation exceeded",
                 अंक_Pages_To_Alloc > d_m3AddressLimit / memory->pageSize);

        u64 अंक_Page_Bytes = अंक_Pages_To_Alloc * memory->pageSize;

#जर d_m3MaxLinearMemoryPages > 0
        // the limit is a memory size, counted in default-sized pages; comparing
        // it against a raw page count would make it 65536 times stricter for a
        // module whose pages are one byte
        _throwif("linear memory limitation exceeded",
                 अंक_Page_Bytes > (u64)d_m3MaxLinearMemoryPages * d_m3DefaultMemPageSize);
#endif

        // Limit the amount of memory that gets actually allocated
        जर (io_runtime->memoryLimit) {
            अंक_Page_Bytes = M3_MIN(अंक_Page_Bytes, (u64)io_runtime->memoryLimit);
        }

        u64 oldLength = memory->mallocated ? memory->mallocated->length : 0;
        u64 otherUsed = io_runtime->memoryBytesUsed - oldLength;
        _throwif(m3Err_memoryLimitExceeded, io_runtime->memoryBytesLimit and
                                              (अंक_Page_Bytes > io_runtime->memoryBytesLimit or
                                               otherUsed > io_runtime->memoryBytesLimit - अंक_Page_Bytes));

        _throwif("linear memory limitation exceeded", अंक_Page_Bytes > (u64)SIZE_MAX - आकार(M3MemoryHeader));

#जर d_m3GuardedMemory

        // A slot holds every address a Wasm access can name, and nothing beyond
        // that: a memory larger than the data part of one would put addresses past
        // the guard, where a fault is nobody's to recognize
        _throwif("linear memory limitation exceeded", अंक_Page_Bytes > d_m3GuardedDataBytes);

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

        जर (memory->guardSlot == रिक्त) {
            memory->guardSlot = Guard_TakeSlot();
            _throwifnull(memory->guardSlot);
        }

        // Growing only ever commits more of the same slot, so the memory never
        // moves - which the interpreter does not rely on, but nothing minds
        _throwif(m3Err_mallocFailed, not Guard_CommitSlot(memory->guardSlot, (माप_प्रकार)अंक_Page_Bytes));

        memory->mallocated = Guard_SlotHeader(memory->guardSlot);

#अन्यथा

        माप_प्रकार अंक_Bytes = (माप_प्रकार)अंक_Page_Bytes + आकार(M3MemoryHeader);

        // What is really there, which is not numPages * pageSize: the limit above
        // caps the bytes without capping the page count, so a memory can be short
        // of a page - by design, since a whole 64 KiB page is often more than an
        // MCU has. Measuring the old block by the page count would overstate it,
        // and then realloc never sees the size it already has and copies the
        // whole limit again on every grow past it.
        माप_प्रकार अंक_Previous_Bytes = memory->mallocated ? memory->mallocated->length + आकार(M3MemoryHeader) : 0;

        शून्य* newMem = m3_Realloc("Wasm Linear Memory", memory->mallocated, अंक_Bytes, अंक_Previous_Bytes);
        _throwifnull(newMem);

        memory->mallocated = (M3MemoryHeader*)newMem;

#endif

#जर d_m3LogRuntime
        M3MemoryHeader* oldMallocated = memory->mallocated;
#endif

        io_runtime->memoryBytesUsed = otherUsed + अंक_Page_Bytes;
        memory->अंक_Pages            = अंक_Pages_To_Alloc;

        memory->mallocated->length  = (माप_प्रकार)अंक_Page_Bytes;
        memory->mallocated->runtime = io_runtime;
        memory->mallocated->memory  = memory;

        memory->mallocated->max_चळत = (m3slot_t*)io_runtime->origin_चळत + io_runtime->अंक_चळत_Slots;

        m3log(runtime, "resized old: %p; mem: %p; length: %zu; pages: %llu", oldMallocated, memory->mallocated, memory->mallocated->length, (अचिन्हित दीर्घ दीर्घ)memory->अंक_Pages);
    } अन्यथा {
        result = m3Err_wasmMemoryOverflow;
    }

    _catch: परत result;
}


शून्य FreeMemoryBlock (IM3Memory io_memory)
{
    जर (io_memory->mallocated and io_memory->mallocated->runtime) {
        io_memory->mallocated->runtime->memoryBytesUsed -= io_memory->mallocated->length;
    }
#जर d_m3GuardedMemory
    Guard_GiveSlot(io_memory->guardSlot);
    io_memory->guardSlot = रिक्त;

    io_memory->mallocated = रिक्त;
#अन्यथा
    m3_Free(io_memory->mallocated);
#endif
}


M3Result InitGlobals (IM3Module io_module)
{
    M3Result result = m3Err_none;

    जर (io_module->अंक_Globals) {
        // placing the globals in their structs isn't good for cache locality, but i don't really know what the global
        // access patterns typically look like yet.

        //          io_module->globalMemory = m3Alloc (m3reg_t, io_module->numGlobals);

        //          if (io_module->globalMemory)
        {
            साठी (u32 i = 0; i < io_module->अंक_Globals; ++i) {
                M3Global* g = &io_module->globals[i];                           m3log (runtime, "initializing global: %d", i);

                जर (g->init_अभिव्यक्ती) {
                    bytes_t start = g->init_अभिव्यक्ती;

                    result = EvaluateExpression(io_module, &g->i64_मूल्य, g->type, &start, g->init_अभिव्यक्ती + g->init_अभिव्यक्ती_Size);

                    जर (not result) {
                        // io_module->globalMemory [i] = initValue;
                    } अन्यथा {
                        थांबा;
                    }
                } अन्यथा {                                                        m3log (runtime, "importing global");
                }
            }
        }
        //          else result = ErrorModule (m3Err_mallocFailed, io_module, "could allocate globals for module: '%s", io_module->name);
    }

    परत result;
}


M3Result InitDataSegments (IM3Module io_module)
{
    M3Result result = m3Err_none;

    साठी (u32 i = 0; i < io_module->अंक_Data_Segments; ++i) {
        M3DataSegment* segment = &io_module->dataSegments[i];

        // A passive segment stays available for memory.init until data.drop.
        // An active one is copied here and then counts as dropped.
        जर (segment->isPassive) {
            सुरूठेवा;
        }

        _throwif("data segment memory index out of range",
                 segment->memoryRegion >= io_module->अंक_Memories);

        IM3Memory io_memory = io_module->memories[segment->memoryRegion];

        _throwif("unallocated linear memory", !(io_memory->mallocated));

        // The offset expression has the memory's address type, and is
        // unsigned: an i32 offset of -1 is 4294967295, way out of bounds
        // rather than negative.
        u64     segmentOffset = 0;
        bytes_t start         = segment->init_अभिव्यक्ती;

        जर (io_memory->isMemory64) {
_           (EvaluateExpression(io_module, &segmentOffset, c_m3Type_i64, &start, segment->init_अभिव्यक्ती + segment->init_अभिव्यक्ती_Size));
        } अन्यथा {
            u32 offset32;
_           (EvaluateExpression(io_module, &offset32, c_m3Type_i32, &start, segment->init_अभिव्यक्ती + segment->init_अभिव्यक्ती_Size));
            segmentOffset = offset32;
        }

        m3log(runtime, "loading data segment: %d; size: %d; offset: %llu", i, segment->size, (अचिन्हित दीर्घ दीर्घ)segmentOffset);

        जर (segmentOffset <= io_memory->mallocated->length &&
            (u64)segment->size <= io_memory->mallocated->length - segmentOffset) {
            u8* dest = m3MemData(io_memory->mallocated) + segmentOffset;
            स_नकल(dest, segment->data, segment->size);
        } अन्यथा {
            _throw("data segment out of bounds");
        }

        segment->dropped = true;
    }

    _catch: परत result;
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

    साठी (u32 e = 0; e < i_segment->अंक_Elements; ++e) {
        u32   funcIndex;
        शून्य* ref = रिक्त;

        जर (i_segment->is_अभिव्यक्ती) {
            m3opcode_t opcode;
_           (Read_opcode(&opcode, &pos, end));

            जर (opcode == c_waOp_refFunc) {
_               (ReadLEB_u32(&funcIndex, &pos, end));
                _throwif("function index out of range", funcIndex >= io_module->अंक_Functions);
                ref = Function_Implementation(&io_module->functions[funcIndex]);
            } अन्यथा जर (opcode == c_waOp_refNull) {
                i8 waType;
                u8 nullType;
_               (ReadLEB_i7(&waType, &pos, end));
_               (NormalizeType(&nullType, waType));
                _throwif(m3Err_typeMismatch, nullType != i_segment->type);
            } अन्यथा जर (opcode == c_waOp_getGlobal) {
                // wasm 2.0 lets an element expression read an imported
                // immutable global, which is how one module seeds another's
                // table with a reference it exported.
                u32 globalIndex;
_               (ReadLEB_u32(&globalIndex, &pos, end));
                _throwif(m3Err_globaIndexOutOfBounds, globalIndex >= io_module->अंक_Globals);

                IM3Global global = &io_module->globals[globalIndex];

                _throwif(m3Err_globaIndexOutOfBounds, not global->imported);
                _throwif(m3Err_wasmMalformed, global->isMutable);
                _throwif(m3Err_typeMismatch, BaseTypeOf(global->type) != BaseTypeOf(i_segment->type));

                // read the cell the import was linked to, not the placeholder
                जर (global->resolved) {
                    global = global->resolved;
                }

                ref = global->ref_मूल्य;
            } अन्यथा {
                _throw("constant expression required");
            }

_           (Read_opcode(&opcode, &pos, end));
            _throwif(m3Err_wasmMalformed, opcode != c_waOp_end);
        } अन्यथा {
_           (ReadLEB_u32(&funcIndex, &pos, end));
            _throwif("function index out of range", funcIndex >= io_module->अंक_Functions);
            ref = Function_Implementation(&io_module->functions[funcIndex]);
        }

        o_elements[e] = ref;
    }

    _catch: परत result;
}


M3Result InitTableAndElements (IM3Module io_module)
{
    M3Result result = m3Err_none;

    cbytes_t end = io_module->elementSectionEnd;
    M3Table* table;

    साठी (u32 i = 0; i < io_module->अंक_Tables; ++i) {
        table = io_module->tables[i];

        // a slot pointing at another module's table is that module's to fill
        जर (table->owner != io_module) {
            सुरूठेवा;
        }

        जर (table->size) {
            IM3Runtime runtime = io_module->runtime;
            _throwif(m3Err_tableLimitExceeded, runtime->tableElementsLimit and
                                                 table->size > runtime->tableElementsLimit - runtime->tableElementsUsed);
            table->elements = m3_AllocArray(शून्य*, table->size);
            _throwifnull(table->elements);
            runtime->tableElementsUsed += table->size;

            जर (table->init_अभिव्यक्ती) {
                शून्य*   मूल्य = रिक्त;
                bytes_t start = table->init_अभिव्यक्ती;
_               (EvaluateExpression(io_module, &मूल्य, BaseTypeOf(table->type),
                                     &start, table->init_अभिव्यक्ती + table->init_अभिव्यक्ती_Size));

                साठी (u32 e = 0; e < table->size; ++e) {
                    table->elements[e] = मूल्य;
                }
            }
        }
    }

    साठी (u32 i = 0; i < io_module->अंक_Element_Segments; ++i) {
        M3ElementSegment* segment = &io_module->elementSegments[i];

        // Declarative segments only make their functions referenceable, and
        // passive ones wait for table.init, so neither is written out here.
        जर (segment->mode == c_m3Elem_declarative) {
            segment->dropped = true;
            सुरूठेवा;
        }

        जर (segment->mode == c_m3Elem_passive) {
            जर (segment->अंक_Elements) {
                segment->resolved = m3_AllocArray(शून्य*, segment->अंक_Elements);
                _throwifnull(segment->resolved);
_               (ResolveElements(io_module, segment, segment->resolved));
            }
            सुरूठेवा;
        }

        table = io_module->tables[segment->tableIndex];

        // The offset expression has the table's index type, and is unsigned:
        // an i32 offset of -1 is 4294967295, out of bounds rather than negative.
        u64     offset = 0;
        bytes_t अभिव्यक्ती   = segment->init_अभिव्यक्ती;

        जर (table->isTable64) {
_           (EvaluateExpression(io_module, &offset, c_m3Type_i64, &अभिव्यक्ती, end));
        } अन्यथा {
            u32 offset32;
_           (EvaluateExpression(io_module, &offset32, c_m3Type_i32, &अभिव्यक्ती, end));
            offset = offset32;
        }

        _throwif("out of bounds table access",
                 offset > table->size or segment->अंक_Elements > table->size - offset);

_       (ResolveElements(io_module, segment, table->elements + offset));

        segment->dropped = true;
    }

    _catch: परत result;
}

M3Result m3_CompileModule (IM3Module io_module)
{
    M3Result result = m3Err_none;

    साठी (u32 i = 0; i < io_module->अंक_Functions; ++i) {
        IM3Function f = &io_module->functions[i];
        जर (f->wasm and not f->compiled) {
_           (CompileFunction(f));
        }
    }

    _catch: परत result;
}

#जर d_m3HasExceptionHandling

M3Exception* NewException (IM3Runtime io_runtime, IM3Tag i_tag, u32 i_अंक_Args)
{
    M3Exception* exception = (M3Exception*)m3_Malloc("M3Exception", आकार(M3Exception) + i_अंक_Args * आकार(u64));

    जर (exception) {
        exception->tag     = i_tag;
        exception->अंक_Args = i_अंक_Args;
        exception->reified = false;
        exception->prev    = रिक्त;
        exception->next    = io_runtime->exceptions;

        जर (exception->next) {
            exception->next->prev = exception;
        }

        io_runtime->exceptions = exception;
    }

    परत exception;
}


// Releases one exception ahead of the rest. The caller has to know nothing can
// still name it: no exnref was ever taken of it, and its payload has already
// been copied out.
शून्य FreeException (IM3Runtime io_runtime, M3Exception* i_exception)
{
    जर (i_exception->prev) {
        i_exception->prev->next = i_exception->next;
    } अन्यथा {
        io_runtime->exceptions = i_exception->next;
    }

    जर (i_exception->next) {
        i_exception->next->prev = i_exception->prev;
    }

    जर (io_runtime->pendingException == i_exception) {
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

    पर्यंत (exception) {
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
    जर (io_runtime->uncaughtException) {
        m3_Free(io_runtime->uncaughtException);
    }

    M3Exception* exception = io_runtime->pendingException;

    जर (M3_UNLIKELY(i_result == m3Err_trapUncaughtException) and exception) {
        M3Exception* copy = (M3Exception*)m3_CopyMem(exception, आकार(M3Exception) + exception->अंक_Args * आकार(u64));

        जर (copy) {
            copy->next = रिक्त;
            copy->prev = रिक्त;
        }

        io_runtime->uncaughtException = copy;
    }
}

#endif // d_m3HasExceptionHandling


#जर d_m3HasExceptionHandling || d_m3_Has_चळत_Switching

IM3Tag m3_FindTag (IM3Module i_module, स्थिरांक अक्षर* स्थिरांक i_tagName)
{
    जर (not i_module or not i_tagName) {
        परत रिक्त;
    }

    परत Module_FindExportedTag(i_module, i_tagName);
}


M3Result m3_NewTag (IM3Runtime io_runtime, IM3Tag* o_tag, स्थिरांक अक्षर* स्थिरांक i_signature)
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
    परत result;
}


M3Result m3_LinkTag (IM3Module         io_module,
                     स्थिरांक अक्षर* स्थिरांक i_moduleName,
                     स्थिरांक अक्षर* स्थिरांक i_tagName,
                     IM3Tag            i_tag)
{
    जर (not io_module or not i_moduleName or not i_tagName or not i_tag) {
        परत m3Err_tagLookupFailed;
    }

    स्थिरांक bool wildcardModule = (म_भेद(i_moduleName, "*") == 0);
    IM3Tag     target         = i_tag->resolved ? i_tag->resolved : i_tag;

    M3Result result = m3Err_tagLookupFailed;

    साठी (u32 i = 0; i < io_module->अंक_Tags; ++i) {
        IM3Tag tag = &io_module->tags[i];

        जर (tag->imported and tag->import.moduleUtf8 and tag->import.fieldUtf8 and
            म_भेद(tag->import.fieldUtf8, i_tagName) == 0 and
            (wildcardModule or म_भेद(tag->import.moduleUtf8, i_moduleName) == 0)) {
            जर (tag->type != target->type) {
                परत m3Err_incompatibleImportType;
            }

            tag->resolved = target;
            result        = m3Err_none;
        }
    }

    परत result;
}


uint32_t m3_Get_Tag_Arg_मोजणी (IM3Tag i_tag)
{
    परत i_tag ? i_tag->type->अंक_Args : 0;
}


M3_मूल्य_Type m3_GetTagArgType (IM3Tag i_tag, uint32_t i_index)
{
    जर (i_tag and i_index < i_tag->type->अंक_Args) {
        परत (M3_मूल्य_Type)BaseTypeOf(d_FuncArgType(i_tag->type, i_index));
    }

    परत c_m3Type_none;
}

#अन्यथा

IM3Tag m3_FindTag (IM3Module i_module, स्थिरांक अक्षर* स्थिरांक i_tagName)
{
    परत रिक्त;
}

M3Result m3_NewTag (IM3Runtime io_runtime, IM3Tag* o_tag, स्थिरांक अक्षर* स्थिरांक i_signature)
{
    जर (o_tag) {
        *o_tag = रिक्त;
    }
    परत m3Err_tagLookupFailed;
}

M3Result m3_LinkTag (IM3Module io_module, स्थिरांक अक्षर* स्थिरांक i_moduleName, स्थिरांक अक्षर* स्थिरांक i_tagName, IM3Tag i_tag)
{
    परत m3Err_tagLookupFailed;
}

uint32_t m3_Get_Tag_Arg_मोजणी (IM3Tag i_tag)
{
    परत 0;
}

M3_मूल्य_Type m3_GetTagArgType (IM3Tag i_tag, uint32_t i_index)
{
    परत c_m3Type_none;
}

#endif // d_m3HasExceptionHandling || d_m3HasStackSwitching


#जर d_m3HasExceptionHandling

M3Result m3_ThrowException (IM3Runtime io_runtime, IM3Tag i_tag, uint32_t i_argc, स्थिरांक शून्य* i_argptrs[])
{
    जर (not io_runtime or not i_tag) {
        परत m3Err_tagLookupFailed;
    }

    IM3Tag      tag  = i_tag->resolved ? i_tag->resolved : i_tag;
    IM3FuncType type = tag->type;

    // a tag with results is a stack switching control tag, which is suspended
    // with rather than thrown
    जर (type->अंक_Rets) {
        परत m3Err_typeMismatch;
    }
    जर (i_argc != type->अंक_Args) {
        परत m3_Err_argument_मोजणी_Mismatch;
    }
    // with nothing on the stack there is nothing to unwind, and the exception
    // would sit on the runtime's list until some later call ended
    जर (io_runtime->callNesting == 0) {
        परत "m3_ThrowException called outside a host function";
    }

    M3Exception* exception = NewException(io_runtime, tag, i_argc);
    जर (not exception) {
        परत m3Err_mallocFailed;
    }

    // the layout op_Throw gives a payload: 32-bit values zero-extended, floats
    // as their bit pattern
    साठी (u32 i = 0; i < i_argc; ++i) {
        स्थिरांक शून्य* मूल्य = i_argptrs[i];

        निवड (BaseTypeOf(d_FuncArgType(type, i))) {
        स्थिती c_m3Type_i32:
        स्थिती c_m3Type_f32: {
            u32 bits;
            स_नकल(&bits, मूल्य, आकार(bits));
            exception->args[i] = bits;
            थांबा;
        }
        स्थिती c_m3Type_i64:
        स्थिती c_m3Type_f64:
            स_नकल(&exception->args[i], मूल्य, आकार(u64));
            थांबा;
        स्थिती c_m3Type_funcref:
        स्थिती c_m3Type_externref:
        स्थिती c_m3Type_exnref:
        स्थिती c_m3Type_contref:
            exception->args[i] = (u64)(*(स्थिरांक uintptr_t*)मूल्य);
            थांबा;
        पूर्वनिर्धारित:
            FreeException(io_runtime, exception);
            परत "unknown argument type";
        }
    }

    io_runtime->pendingException = exception;

    परत m3Err_pendingException;
}


IM3Tag m3_GetExceptionTag (IM3Runtime i_runtime)
{
    जर (i_runtime and i_runtime->uncaughtException) {
        परत i_runtime->uncaughtException->tag;
    }

    परत रिक्त;
}


M3Result m3_GetExceptionArgs (IM3Runtime i_runtime, uint32_t i_argc, स्थिरांक शून्य* o_argptrs[])
{
    जर (not i_runtime or not i_runtime->uncaughtException) {
        परत "no uncaught exception";
    }

    स्थिरांक M3Exception* exception = i_runtime->uncaughtException;
    IM3FuncType        type      = exception->tag->type;

    जर (i_argc != exception->अंक_Args) {
        परत m3_Err_argument_मोजणी_Mismatch;
    }

    साठी (u32 i = 0; i < i_argc; ++i) {
        शून्य* मूल्य = (शून्य*)o_argptrs[i];
        u64   bits  = exception->args[i];

        निवड (BaseTypeOf(d_FuncArgType(type, i))) {
        स्थिती c_m3Type_i32:
        स्थिती c_m3Type_f32: {
            u32 खालचा = (u32)bits;
            स_नकल(मूल्य, &खालचा, आकार(खालचा));
            थांबा;
        }
        स्थिती c_m3Type_i64:
        स्थिती c_m3Type_f64:
            स_नकल(मूल्य, &bits, आकार(bits));
            थांबा;
        स्थिती c_m3Type_funcref:
        स्थिती c_m3Type_externref:
        स्थिती c_m3Type_exnref:
        स्थिती c_m3Type_contref:
            *(uintptr_t*)मूल्य = (uintptr_t)bits;
            थांबा;
        पूर्वनिर्धारित:
            परत "unknown argument type";
        }
    }

    परत m3Err_none;
}

#अन्यथा

M3Result m3_ThrowException (IM3Runtime io_runtime, IM3Tag i_tag, uint32_t i_argc, स्थिरांक शून्य* i_argptrs[])
{
    परत m3Err_tagLookupFailed;
}

IM3Tag m3_GetExceptionTag (IM3Runtime i_runtime)
{
    परत रिक्त;
}

M3Result m3_GetExceptionArgs (IM3Runtime i_runtime, uint32_t i_argc, स्थिरांक शून्य* o_argptrs[])
{
    परत "no uncaught exception";
}

#endif // d_m3HasExceptionHandling


#जर d_m3GuardedMemory

// What RunCodeProtected hands to the platform layer to run, and what comes back
प्रकारव्याख्या संरचना M3ProtectedRun {
    pc_t            pc;
    IM3Runtime      runtime;
    M3MemoryHeader* mem;
    M3Result        result;
} M3ProtectedRun;

स्थिर
शून्य RunCodeBody (शून्य* io_context)
{
    M3ProtectedRun* run = (M3ProtectedRun*)io_context;

#  जर (d_m3EnableOpProfiling || d_m3EnableOpTracing)
    run->result = (M3Result)RunCode(run->pc, (m3stack_t)run->runtime->चळत, run->mem,
                                    d_m3OpDefaultArgs, d_m3BaseCstr);
#  अन्यथा
    run->result = (M3Result)RunCode(run->pc, (m3stack_t)run->runtime->चळत, run->mem,
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
    जर (M3_UNLIKELY(not m3_HostGuardsActive())) {
        परत ErrorRuntime(m3Err_memoryGuardsUnavailable, i_runtime,
                            "another part of the process holds the fault signal");
    }

    M3ProtectedRun run;
    run.pc      = i_pc;
    run.runtime = i_runtime;
    run.mem     = i_mem;
    run.result  = m3Err_none;

    शून्य*  arena;
    माप_प्रकार arenaBytes;
    Guard_ArenaRange(&arena, &arenaBytes);

    जर (not m3_HostProtectedCall(RunCodeBody, &run, arena, arenaBytes)) {
        परत m3Err_trapOutOfBoundsMemoryAccess;
    }

    परत run.result;
}

#endif // d_m3GuardedMemory


#जर d_m3_Has_चळत_Switching

// The function a suspended continuation stopped in: the callee of its innermost
// call frame, or the function it was entered with if it made no call. Frames are
// recorded innermost first, so that is the lowest-numbered call frame.
स्थिर
IM3Function Continuation_InnermostFunction (IM3Continuation i_cont)
{
    साठी (u32 i = 0; i < i_cont->अंक_Frames; ++i) {
        जर (i_cont->frames[i].kind == frame_call && i_cont->frames[i].call.function) {
            परत i_cont->frames[i].call.function;
        }
    }

    परत i_cont->entryFunction;
}


// Where the host's next call puts its arguments and finds its results. A
// suspended root keeps its frames on the bottom of the runtime's stack, so a call
// made before it is resumed starts above the last of them rather than on top of
// them. The top is the innermost function's frame base plus everything that
// function can use. That base is the root's own sp, unless it stopped inside a
// resume: then the suspension point belongs to the resumed continuation, and the
// root's innermost frame is that resume.
शून्य Runtime_Place_Call_चळत (IM3Runtime io_runtime)
{
    IM3Continuation मूळ = io_runtime->मूळ_Continuation;

    io_runtime->चळत = io_runtime->origin_चळत;

    जर (not मूळ or मूळ->state != cont_suspended) {
        परत;
    }

    m3stack_t   sp       = मूळ->sp;
    IM3Function function = Continuation_InnermostFunction(मूळ);

    जर (मूळ->अंक_Frames and मूळ->frames[0].kind == frame_resume) {
        sp = मूळ->frames[0].sp;
    }

    m3slot_t* base = (m3slot_t*)io_runtime->origin_चळत;
    m3slot_t* शीर्ष  = (m3slot_t*)sp + (function ? function->max_चळत_Slots : 0);

    जर (शीर्ष > base + io_runtime->अंक_चळत_Slots) {
        शीर्ष = base + io_runtime->अंक_चळत_Slots;
    }

    io_runtime->चळत = शीर्ष;
}

#endif // d_m3HasStackSwitching


// Run compiled code on the runtime's stack, bounding native recursion for the
// duration of the call. The outermost invocation establishes the stack limit;
// nested ones (an imported function calling back into Wasm) inherit it.
स्थिर ओळीत
M3Result RunCodeChecked (IM3Runtime i_runtime, IM3Function i_function)
{
    pc_t i_pc = i_function->compiled;

    // execution runs against the memory of the module the entry point belongs
    // to, not against some runtime-wide one
    M3MemoryHeader* _mem = Module_MemoryHeader(i_function->module);

#जर d_m3HasSnapshots
    जर (M3_UNLIKELY(i_function->module->isUnusable)) {
        परत "a snapshot failed to restore into the module, so it cannot run";
    }
    i_function->module->hasRun = true;
#endif

    d_m3_चळत_Limit_Enter(i_runtime);

    // a host function calling back into Wasm does not change which call a trap
    // ends: that is still the one the host made first
    जर (i_runtime->callNesting++ == 0) {
        i_runtime->entered = i_function;
    }
#जर d_m3HasExceptionHandling
    // handler stacks don't nest across a call boundary: a host function calling
    // back into Wasm cannot be caught by a try_table its own caller entered
    u32 savedTryDepth   = i_runtime->tryDepth;
    i_runtime->tryDepth = 0;
#endif
#जर d_m3_Has_चळत_Switching
    // nor do continuations: the native frames on the far side of a host call
    // are not ours to record, so a suspend in here has no handler to reach and
    // no resume to unwind to
    IM3Continuation savedContinuation = i_runtime->activeContinuation;

    // A suspended root continuation is a paused program waiting to be resumed,
    // not scratch space. A call the host makes before resuming it gets a
    // context of its own, the way any other re-entry does, rather than having
    // the frames of the suspension reset out from under it.
    bool claim_मूळ = i_runtime->isSuspendable and not savedContinuation and
                     not (i_runtime->मूळ_Continuation and
                          i_runtime->मूळ_Continuation->state == cont_suspended);

    जर (claim_मूळ and not i_runtime->मूळ_Continuation) {
        i_runtime->मूळ_Continuation = Continuation_New(i_runtime, i_function->funcType, i_function);

        जर (i_runtime->मूळ_Continuation) {
            // The root borrows the runtime's stack and is not charged as cont.new.
            i_runtime->मूळ_Continuation->अंक_चळत_Slots = i_runtime->अंक_चळत_Slots;
        } अन्यथा {
            claim_मूळ = false;
        }
    }

    जर (claim_मूळ) {
        i_runtime->activeContinuation              = i_runtime->मूळ_Continuation;
        i_runtime->मूळ_Continuation->entryFunction = i_function;
        i_runtime->मूळ_Continuation->state         = cont_running;
        i_runtime->मूळ_Continuation->अंक_Frames     = 0;
    } अन्यथा {
        i_runtime->activeContinuation = रिक्त;
    }
#endif
#जर d_m3GuardedMemory
    M3Result result = RunCodeProtected(i_runtime, i_pc, _mem);
#किंवाजर (d_m3EnableOpProfiling || d_m3EnableOpTracing)
    M3Result result = (M3Result)RunCode(i_pc, (m3stack_t)i_runtime->चळत, _mem, d_m3OpDefaultArgs, d_m3BaseCstr);
#अन्यथा
    M3Result result = (M3Result)RunCode(i_pc, (m3stack_t)i_runtime->चळत, _mem, d_m3OpDefaultArgs);
#endif
#जर d_m3_Has_चळत_Switching
    जर (claim_मूळ) {
        i_runtime->मूळ_Continuation->state =
          (result == m3Err_continuationSuspended) ? cont_suspended : cont_returned;

        Runtime_Place_Call_चळत(i_runtime);
    }
    i_runtime->activeContinuation = savedContinuation;
#endif
#जर d_m3HasExceptionHandling
    i_runtime->tryDepth = savedTryDepth;

    // an exception that reached the bottom of the call stack found no handler
    जर (M3_UNLIKELY(result == m3Err_pendingException)) {
        result = m3Err_trapUncaughtException;
    }
    RecordUncaughtException(i_runtime, result);

    // a paused invocation can still reach the exceptions it holds, so they wait
    // for it to finish
    जर (i_runtime->callNesting == 1 and not m3_IsSuspended(i_runtime)) {
        FreeExceptions(i_runtime);
    }
#endif
    i_runtime->callNesting--;
    d_m3_चळत_Limit_Leave(i_runtime);

    परत result;
}

M3Result m3_RunStart (IM3Module io_module)
{
#ifdef FUZZING_BUILD_MODE_UNSAFE_FOR_PRODUCTION
    // Execution disabled for fuzzing builds
    परत m3Err_none;
#endif

    M3Result result           = m3Err_none;
    i32      startFunctionTmp = -1;

    जर (io_module and io_module->startFunction >= 0) {
        IM3Function function = &io_module->functions[io_module->startFunction];

        जर (not function->compiled) {
_           (CompileFunction(function));
        }

        IM3FuncType ftype = function->funcType;
        जर (ftype->अंक_Args != 0 || ftype->अंक_Rets != 0) {
            _throw(m3_Err_argument_मोजणी_Mismatch);
        }

        IM3Module  module  = function->module;
        IM3Runtime runtime = module->runtime;

        startFunctionTmp = io_module->startFunction;

        io_module->startFunction = -1;

        result = RunCodeChecked(runtime, function);

        // a start function that suspended is not to be started again: resuming
        // is what finishes it
        जर (result and result != m3Err_continuationSuspended) {
            io_module->startFunction = startFunctionTmp;
            EXCEPTION_PRINT(result);
            जा _catch;
        }
    }

    _catch: परत result;
}

// TODO: deal with main + side-modules loading efforcement
M3Result m3_LoadModule (IM3Runtime io_runtime, IM3Module io_module)
{
    M3Result result = m3Err_none;

    जर (M3_UNLIKELY(io_module->runtime)) {
        परत m3Err_moduleAlreadyLinked;
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
    परत result; // ok

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
    पर्यंत (*tail) {
        tail = &(*tail)->next;
    }
    *tail = io_module;

    परत result;
}

IM3Global m3_FindGlobal (IM3Module         io_module,
                         स्थिरांक अक्षर* स्थिरांक i_globalName)
{
    // Search exports
    IM3Global exported = Module_FindExportedGlobal(io_module, i_globalName);
    जर (exported) {
        // a re-exported global is the one that was imported, so reads and
        // writes have to reach the cell that actually holds the value
        परत exported->resolved ? exported->resolved : exported;
    }

    // Search imports
    साठी (u32 i = 0; i < io_module->अंक_Globals; ++i) {
        IM3Global g = &io_module->globals[i];

        जर (g->import.moduleUtf8 and g->import.fieldUtf8) {
            जर (म_भेद(g->import.fieldUtf8, i_globalName) == 0) {
                परत g->resolved ? g->resolved : g;
            }
        }
    }
    परत रिक्त;
}

M3Result m3_GetGlobal (IM3Global      i_global,
                       IM3_Tagged_मूल्य o_मूल्य)
{
    जर (not i_global) {
        परत m3Err_globalLookupFailed;
    }

    निवड (i_global->type) {
    स्थिती c_m3Type_i32: o_मूल्य->मूल्य.i32 = i_global->i32_मूल्य; थांबा;
    स्थिती c_m3Type_i64: o_मूल्य->मूल्य.i64 = i_global->i64_मूल्य; थांबा;
#जर d_m3HasFloat
    स्थिती c_m3Type_f32: o_मूल्य->मूल्य.f32 = i_global->f32_मूल्य; थांबा;
    स्थिती c_m3Type_f64: o_मूल्य->मूल्य.f64 = i_global->f64_मूल्य; थांबा;
#endif
    पूर्वनिर्धारित: परत m3Err_invalidTypeId;
    }

    o_मूल्य->type = (M3_मूल्य_Type)(i_global->type);
    परत m3Err_none;
}

M3Result m3_SetGlobal (IM3Global            i_global,
                       स्थिरांक IM3_Tagged_मूल्य i_मूल्य)
{
    जर (not i_global) {
        परत m3Err_globalLookupFailed;
    }
    जर (not i_global->isMutable) {
        परत m3Err_globalNotMutable;
    }
    जर (i_global->type != i_मूल्य->type) {
        परत m3Err_globalTypeMismatch;
    }

    निवड (i_मूल्य->type) {
    स्थिती c_m3Type_i32: i_global->i32_मूल्य = i_मूल्य->मूल्य.i32; थांबा;
    स्थिती c_m3Type_i64: i_global->i64_मूल्य = i_मूल्य->मूल्य.i64; थांबा;
#जर d_m3HasFloat
    स्थिती c_m3Type_f32: i_global->f32_मूल्य = i_मूल्य->मूल्य.f32; थांबा;
    स्थिती c_m3Type_f64: i_global->f64_मूल्य = i_मूल्य->मूल्य.f64; थांबा;
#endif
    पूर्वनिर्धारित: परत m3Err_invalidTypeId;
    }

    परत m3Err_none;
}

M3_मूल्य_Type m3_GetGlobalType (IM3Global i_global)
{
    परत (i_global) ? (M3_मूल्य_Type)(i_global->type) : c_m3Type_none;
}


शून्य* v_FindFunction (IM3Module i_module, शून्य* i_info)
{
    स्थिरांक अक्षर* स्थिरांक i_name = (स्थिरांक अक्षर*)i_info;

    // Prefer exported functions
    IM3Function exported = Module_FindExportedFunction(i_module, i_name);
    जर (exported) {
        परत exported;
    }

    // Search internal functions
    साठी (u32 i = 0; i < i_module->अंक_Functions; ++i) {
        IM3Function f = &i_module->functions[i];

        bool isImported = f->import.moduleUtf8 or f->import.fieldUtf8;

        जर (isImported) {
            सुरूठेवा;
        }

        साठी (पूर्णांक j = 0; j < f->अंक_Names; j++) {
            जर (f->names[j] and म_भेद(f->names[j], i_name) == 0) {
                परत f;
            }
        }
    }

    परत रिक्त;
}


// Shared tail of the two lookups: a function is only usable once it has code.
स्थिर
M3Result PrepareFoundFunction (IM3Function* o_function, IM3Function i_function)
{
    M3Result result = m3Err_none;

    जर (not i_function->compiled) {
_       (CompileFunction(i_function))
    }

    _catch:
    *o_function = result ? रिक्त : i_function;

    परत result;
}


// Searches every module in the runtime, most recently loaded first. That is a
// guess once more than one module is loaded and two of them export the same
// name - m3_FindFunctionIn says which module is meant.
M3Result m3_FindFunction (IM3Function* o_function, IM3Runtime i_runtime, स्थिरांक अक्षर* स्थिरांक i_functionName)
{
                                                                d_m3Assert (o_function and i_runtime and i_functionName);
    IM3Function function = रिक्त;

    जर (not i_runtime->modules) {
        *o_function = रिक्त;
        परत "no modules loaded";
    }

    function = (IM3Function)ForEachModule(i_runtime, v_FindFunction, (शून्य*)i_functionName);

    जर (not function) {
        *o_function = रिक्त;
        परत ErrorModule(m3Err_functionLookupFailed, i_runtime->modules, "'%s'", i_functionName);
    }

    परत PrepareFoundFunction(o_function, function);
}


IM3Module m3_FindModule (IM3Runtime i_runtime, स्थिरांक अक्षर* स्थिरांक i_moduleName)
{
    जर (not i_runtime or not i_moduleName) {
        परत रिक्त;
    }

    // the list is newest-first, so a name registered twice names the newer one
    साठी (IM3Module m = i_runtime->modules; m; m = m->next) {
        जर (m->name and म_भेद(m->name, i_moduleName) == 0) {
            परत m;
        }
    }

    परत रिक्त;
}


// Searches one module's exports, which is what naming a module means.
M3Result m3_FindFunctionIn (IM3Function* o_function, IM3Module i_module, स्थिरांक अक्षर* स्थिरांक i_functionName)
{
                                                                d_m3Assert (o_function and i_functionName);
    जर (not i_module) {
        *o_function = रिक्त;
        परत m3Err_functionLookupFailed;      // ErrorModule would deref it
    }

    IM3Function function = (IM3Function)v_FindFunction(i_module, (शून्य*)i_functionName);

    जर (not function) {
        *o_function = रिक्त;
        परत ErrorModule(m3Err_functionLookupFailed, i_module, "'%s'", i_functionName);
    }

    परत PrepareFoundFunction(o_function, function);
}


M3Result m3_GetTableFunction (IM3Function* o_function, IM3Module i_module, uint32_t i_index)
{
_try {
    M3Table*    table;
    IM3Function function;

    _throwif("no table", i_module->अंक_Tables == 0);

    table = i_module->tables[0];
    _throwif("function index out of range", i_index >= table->size);

    function = (IM3Function)table->elements[i_index];

    जर (function) {
        जर (not function->compiled) {
_           (CompileFunction(function))
        }
    }

    *o_function = function;
} _catch:
    परत result;
}


स्थिर
M3Result checkStartFunction (IM3Module i_module)
{
    M3Result result = m3Err_none;                               d_m3Assert(i_module);

    // Check if start function needs to be called
    जर (i_module->startFunction >= 0) {
        result = m3_RunStart(i_module);
    }

    परत result;
}

uint32_t m3_Get_Arg_मोजणी (IM3Function i_function)
{
    जर (i_function) {
        IM3FuncType ft = i_function->funcType;
        जर (ft) {
            परत ft->अंक_Args;
        }
    }
    परत 0;
}

uint32_t m3_Get_Ret_मोजणी (IM3Function i_function)
{
    जर (i_function) {
        IM3FuncType ft = i_function->funcType;
        जर (ft) {
            परत ft->अंक_Rets;
        }
    }
    परत 0;
}


M3_मूल्य_Type m3_GetArgType (IM3Function i_function, uint32_t index)
{
    जर (i_function) {
        IM3FuncType ft = i_function->funcType;
        जर (ft and index < ft->अंक_Args) {
            परत (M3_मूल्य_Type)BaseTypeOf(d_FuncArgType(ft, index));
        }
    }
    परत c_m3Type_none;
}

M3_मूल्य_Type m3_GetRetType (IM3Function i_function, uint32_t index)
{
    जर (i_function) {
        IM3FuncType ft = i_function->funcType;
        जर (ft and index < ft->अंक_Rets) {
            परत (M3_मूल्य_Type)BaseTypeOf(d_FuncRetType(ft, index));
        }
    }
    परत c_m3Type_none;
}


// The call APIs marshal their arguments into the runtime stack before any
// compiled code runs, so op_Entry's overflow check comes too late to cover those
// writes - and for a host function or an import linked to another module there
// is no op_Entry of our own to reach at all. Bound the area about to be written
// here instead: results and arguments each take a u64 at this boundary, however
// wide the slot is inside. Every caller goes through this, so none of them can
// be the one that forgets.
स्थिर
M3Result Get_चळत_Pointer_For_Args (IM3Function i_function, u8** o_चळत)
{
    IM3Runtime  runtime = i_function->module->runtime;
    IM3FuncType ftype   = i_function->funcType;

    // the stack a paused invocation holds is not available to this one
    माप_प्रकार used      = (माप_प्रकार)((u8*)runtime->चळत - (u8*)runtime->origin_चळत);
    माप_प्रकार needed    = ((माप_प्रकार)ftype->अंक_Rets + ftype->अंक_Args) * आकार(u64);
    माप_प्रकार available = (माप_प्रकार)runtime->अंक_चळत_Slots * आकार(m3slot_t) - used;

    जर (needed > available) {
        परत m3_Err_trap_चळत_Overflow;
    }

    u64* चळत = (u64*)runtime->चळत;

    चळत += ftype->अंक_Rets;

    *o_चळत = (u8*)चळत;

    परत m3Err_none;
}


M3Result m3_CallV (IM3Function i_function, ...)
{
    बहु_सूची ap;
    बहु_शुरू(ap, i_function);
    M3Result r = m3_CallVL(i_function, ap);
    बहु_पूर्ण(ap);
    परत r;
}

स्थिर
शून्य Report_Native_चळत_Usage ()
{
#जर d_m3_Log_Native_चळत
    पूर्णांक चळत_Used = m3_चळत_Get_Max();
    ख_लिखो(मानक_त्रुटि, "Native stack used: %d\n", चळत_Used);
#endif
}


M3Result m3_CallVL (IM3Function i_function, बहु_सूची i_args)
{
    IM3Runtime  runtime = i_function->module->runtime;
    IM3FuncType ftype   = i_function->funcType;
    M3Result    result  = m3Err_none;
    u8*         s       = रिक्त;

    जर (!i_function->compiled) {
        परत m3Err_missingCompiledCode;
    }

#जर d_m3RecordBacktraces
    ClearBacktrace(runtime);
#endif

    m3_चळत_Check_Init();

_   (checkStartFunction(i_function->module))

_   (Get_चळत_Pointer_For_Args(i_function, &s))

    साठी (u32 i = 0; i < ftype->अंक_Args; ++i) {
        निवड (d_FuncArgType(ftype, i)) {
        स्थिती c_m3Type_i32:
            *(i32*)(s) = बहु_तर्क(i_args, i32);
            s += 8;
            थांबा;
        स्थिती c_m3Type_i64:
            *(i64*)(s) = बहु_तर्क(i_args, i64);
            s += 8;
            थांबा;
        स्थिती c_m3Type_funcref:
        स्थिती c_m3Type_externref:
        स्थिती c_m3Type_exnref:
            *(uintptr_t*)(s) = बहु_तर्क(i_args, uintptr_t);
            s += 8;
            थांबा;
#जर d_m3HasFloat
        स्थिती c_m3Type_f32:
            *(f32*)(s) = (f32)बहु_तर्क(i_args, f64);
            s += 8;
            थांबा; // f32 is passed as f64
        स्थिती c_m3Type_f64:
            *(f64*)(s) = बहु_तर्क(i_args, f64);
            s += 8;
            थांबा;
#endif
        पूर्वनिर्धारित: परत "unknown argument type";
        }
    }

    result = RunCodeChecked(runtime, i_function);
    Report_Native_चळत_Usage();

    runtime->lastCalled = result ? रिक्त : i_function;

    _catch: परत result;
}

M3Result m3_Call (IM3Function i_function, uint32_t i_argc, स्थिरांक शून्य* i_argptrs[])
{
    IM3Runtime  runtime = i_function->module->runtime;
    IM3FuncType ftype   = i_function->funcType;
    M3Result    result  = m3Err_none;
    u8*         s       = रिक्त;

    जर (i_argc != ftype->अंक_Args) {
        परत m3_Err_argument_मोजणी_Mismatch;
    }
    जर (!i_function->compiled) {
        परत m3Err_missingCompiledCode;
    }

#जर d_m3RecordBacktraces
    ClearBacktrace(runtime);
#endif

    m3_चळत_Check_Init();

_   (checkStartFunction(i_function->module))

_   (Get_चळत_Pointer_For_Args(i_function, &s))

    साठी (u32 i = 0; i < ftype->अंक_Args; ++i) {
        निवड (d_FuncArgType(ftype, i)) {
        स्थिती c_m3Type_i32:
            *(i32*)(s) = *(i32*)i_argptrs[i];
            s += 8;
            थांबा;
        स्थिती c_m3Type_i64:
            *(i64*)(s) = *(i64*)i_argptrs[i];
            s += 8;
            थांबा;
        स्थिती c_m3Type_funcref:
        स्थिती c_m3Type_externref:
        स्थिती c_m3Type_exnref:
            *(uintptr_t*)(s) = *(uintptr_t*)i_argptrs[i];
            s += 8;
            थांबा;
#जर d_m3HasFloat
        स्थिती c_m3Type_f32:
            *(f32*)(s) = *(f32*)i_argptrs[i];
            s += 8;
            थांबा;
        स्थिती c_m3Type_f64:
            *(f64*)(s) = *(f64*)i_argptrs[i];
            s += 8;
            थांबा;
#endif
        पूर्वनिर्धारित: परत "unknown argument type";
        }
    }

    result = RunCodeChecked(runtime, i_function);

    Report_Native_चळत_Usage();

    runtime->lastCalled = result ? रिक्त : i_function;

    _catch: परत result;
}

// Argument parsing for m3_CallArgv. Strict on purpose: the whole string has to
// be consumed, so "12abc" is rejected rather than read as 12, and an empty or
// unparsable argument is an error rather than the zero that strtoul with a
// NULL end pointer used to hand back. See wasm3/wasm3#367.
स्थिर
M3Result ParseArgInteger (ccstr_t i_arg, u32 i_अंक_Bits, u64* o_मूल्य)
{
    जर (not i_arg or not *i_arg) {
        परत "empty argument";
    }

    // strtoull would skip leading space, but trailing space is rejected below;
    // accepting one and not the other would just be confusing
    जर (है_खाली((अचिन्हित अक्षर)*i_arg)) {
        परत "argument is not a number";
    }

    अक्षर* end = रिक्त;
    u64   मूल्य;

    त्रुटि_सं = 0;

    // an argument may be spelled signed or unsigned: -1 and 4294967295 name the
    // same i32
    जर (*i_arg == '-') {
        i64 signed_मूल्य = strtoll(i_arg, &end, 10);

        जर (i_अंक_Bits == 32 and (signed_मूल्य < INT32_MIN or signed_मूल्य > INT32_MAX)) {
            परत "argument out of range";
        }

        मूल्य = (u64)signed_मूल्य;
    } अन्यथा {
        मूल्य = strtoull(i_arg, &end, 10);

        जर (i_अंक_Bits == 32 and मूल्य > UINT32_MAX) {
            परत "argument out of range";
        }
    }

    जर (त्रुटि_सं == दुस्फल) {
        परत "argument out of range";
    }

    जर (end == i_arg or *end) {
        परत "argument is not a number";
    }

    *o_मूल्य = मूल्य;

    परत m3Err_none;
}


#जर d_m3HasFloat
स्थिर
M3Result ParseArgFloat (ccstr_t i_arg, f64* o_मूल्य)
{
    जर (not i_arg or not *i_arg) {
        परत "empty argument";
    }

    जर (है_खाली((अचिन्हित अक्षर)*i_arg)) {
        परत "argument is not a number";
    }

    अक्षर* end = रिक्त;

    त्रुटि_सं = 0;

    f64 मूल्य = म_से_भग्न(i_arg, &end);

    जर (end == i_arg or *end) {
        परत "argument is not a number";
    }

    // strtod reports underflow through ERANGE as well, and a denormal result is
    // perfectly usable, so only an overflow to infinity is out of range
    जर (त्रुटि_सं == दुस्फल and (मूल्य > द्विग_उच्च or मूल्य < -द्विग_उच्च)) {
        परत "argument out of range";
    }

    *o_मूल्य = मूल्य;

    परत m3Err_none;
}
#endif


// A reference argument is either the null reference or a host handle written as
// an integer.
स्थिर
M3Result ParseArgReference (ccstr_t i_arg, u64* o_मूल्य)
{
    जर (i_arg and म_भेद(i_arg, "null") == 0) {
        *o_मूल्य = 0;
        परत m3Err_none;
    }

    परत ParseArgInteger(i_arg, 64, o_मूल्य);
}


M3Result m3_CallArgv (IM3Function i_function, uint32_t i_argc, स्थिरांक अक्षर* i_argv[])
{
    IM3FuncType ftype   = i_function->funcType;
    IM3Runtime  runtime = i_function->module->runtime;
    M3Result    result  = m3Err_none;
    u8*         s       = रिक्त;

    जर (i_argc != ftype->अंक_Args) {
        परत m3_Err_argument_मोजणी_Mismatch;
    }
    जर (!i_function->compiled) {
        परत m3Err_missingCompiledCode;
    }

#जर d_m3RecordBacktraces
    ClearBacktrace(runtime);
#endif

    m3_चळत_Check_Init();

_   (checkStartFunction(i_function->module))

_   (Get_चळत_Pointer_For_Args(i_function, &s))

    साठी (u32 i = 0; i < ftype->अंक_Args; ++i) {
        u64 मूल्य = 0;
#जर d_m3HasFloat
        f64 fvalue = 0;
#endif
        निवड (d_FuncArgType(ftype, i)) {
        स्थिती c_m3Type_i32:
_           (ParseArgInteger(i_argv[i], 32, &मूल्य)) * (i32*)(s) = (i32)मूल्य;
            s += 8;
            थांबा;
        स्थिती c_m3Type_i64:
_           (ParseArgInteger(i_argv[i], 64, &मूल्य)) * (i64*)(s) = (i64)मूल्य;
            s += 8;
            थांबा;
        स्थिती c_m3Type_funcref:
        स्थिती c_m3Type_externref:
        स्थिती c_m3Type_exnref:
_           (ParseArgReference(i_argv[i], &मूल्य)) * (uintptr_t*)(s) = (uintptr_t)मूल्य;
            s += 8;
            थांबा;
#जर d_m3HasFloat
            // strtof would be less portable
        स्थिती c_m3Type_f32:
_           (ParseArgFloat(i_argv[i], &fvalue)) * (f32*)(s) = (f32)fvalue;
            s += 8;
            थांबा;
        स्थिती c_m3Type_f64:
_           (ParseArgFloat(i_argv[i], &fvalue)) * (f64*)(s) = fvalue;
            s += 8;
            थांबा;
#endif
        पूर्वनिर्धारित: _throw("unknown argument type");
        }
    }

    result = RunCodeChecked(runtime, i_function);

    Report_Native_चळत_Usage();

    runtime->lastCalled = result ? रिक्त : i_function;

    _catch: परत result;
}


//u8 * AlignStackPointerTo64Bits (const u8 * i_stack)
//{
//    uintptr_t ptr = (uintptr_t) i_stack;
//    return (u8 *) ((ptr + 7) & ~7);
//}


M3Result m3_GetResults (IM3Function i_function, uint32_t i_retc, स्थिरांक शून्य* o_retptrs[])
{
    IM3FuncType ftype   = i_function->funcType;
    IM3Runtime  runtime = i_function->module->runtime;

    जर (i_retc != ftype->अंक_Rets) {
        परत m3_Err_argument_मोजणी_Mismatch;
    }
    जर (i_function != runtime->lastCalled) {
        परत "function not called";
    }

    u8* s = (u8*)runtime->चळत;

    साठी (u32 i = 0; i < ftype->अंक_Rets; ++i) {
        निवड (d_FuncRetType(ftype, i)) {
        स्थिती c_m3Type_i32:
            *(i32*)o_retptrs[i] = *(i32*)(s);
            s += 8;
            थांबा;
        स्थिती c_m3Type_i64:
            *(i64*)o_retptrs[i] = *(i64*)(s);
            s += 8;
            थांबा;
        स्थिती c_m3Type_funcref:
        स्थिती c_m3Type_externref:
        स्थिती c_m3Type_exnref:
            *(uintptr_t*)o_retptrs[i] = *(uintptr_t*)(s);
            s += 8;
            थांबा;
#जर d_m3HasFloat
        स्थिती c_m3Type_f32:
            *(f32*)o_retptrs[i] = *(f32*)(s);
            s += 8;
            थांबा;
        स्थिती c_m3Type_f64:
            *(f64*)o_retptrs[i] = *(f64*)(s);
            s += 8;
            थांबा;
#endif
        पूर्वनिर्धारित: परत "unknown return type";
        }
    }
    परत m3Err_none;
}

M3Result m3_GetResultsV (IM3Function i_function, ...)
{
    बहु_सूची ap;
    बहु_शुरू(ap, i_function);
    M3Result r = m3_GetResultsVL(i_function, ap);
    बहु_पूर्ण(ap);
    परत r;
}

M3Result m3_GetResultsVL (IM3Function i_function, बहु_सूची o_rets)
{
    IM3Runtime  runtime = i_function->module->runtime;
    IM3FuncType ftype   = i_function->funcType;

    जर (i_function != runtime->lastCalled) {
        परत "function not called";
    }

    u8* s = (u8*)runtime->चळत;
    साठी (u32 i = 0; i < ftype->अंक_Rets; ++i) {
        निवड (d_FuncRetType(ftype, i)) {
        स्थिती c_m3Type_i32:
            *बहु_तर्क(o_rets, i32*) = *(i32*)(s);
            s += 8;
            थांबा;
        स्थिती c_m3Type_i64:
            *बहु_तर्क(o_rets, i64*) = *(i64*)(s);
            s += 8;
            थांबा;
        स्थिती c_m3Type_funcref:
        स्थिती c_m3Type_externref:
        स्थिती c_m3Type_exnref:
            *बहु_तर्क(o_rets, uintptr_t*) = *(uintptr_t*)(s);
            s += 8;
            थांबा;
#जर d_m3HasFloat
        स्थिती c_m3Type_f32:
            *बहु_तर्क(o_rets, f32*) = *(f32*)(s);
            s += 8;
            थांबा;
        स्थिती c_m3Type_f64:
            *बहु_तर्क(o_rets, f64*) = *(f64*)(s);
            s += 8;
            थांबा;
#endif
        पूर्वनिर्धारित: परत "unknown argument type";
        }
    }
    परत m3Err_none;
}

शून्य ReleaseCodePageNoTrack (IM3Runtime i_runtime, IM3CodePage i_codePage)
{
    जर (i_codePage) {
        IM3CodePage* list;

        bool pageFull = (अंक_Free_ओळी(i_codePage) < d_m3_Code_Page_Free_ओळी_Threshold);
        जर (pageFull) {
            list = &i_runtime->pagesFull;
        } अन्यथा {
            list = &i_runtime->pagesOpen;
        }

        ढकला_Code_Page(list, i_codePage);                         m3log (emit, "release page: %d to queue: '%s'", i_codePage->info.sequence, pageFull ? "full" : "open")
    }
}


IM3CodePage AcquireCodePageWithCapacity (IM3Runtime i_runtime, u32 i_min_ओळ_मोजणी)
{
    IM3CodePage page = RemoveCodePageOfCapacity(&i_runtime->pagesOpen, i_min_ओळ_मोजणी);

    जर (not page) {
        page = Environment_AcquireCodePage(i_runtime->environment, i_min_ओळ_मोजणी);

        जर (not page) {
            page = NewCodePage(i_runtime, i_min_ओळ_मोजणी);
        }

        जर (page) {
            i_runtime->अंक_Code_Pages++;
        }
    }

    जर (page) {                                                  m3log (emit, "acquire page: %d", page->info.sequence);
        i_runtime->अंक_Active_Code_Pages++;
    }

    परत page;
}


IM3CodePage AcquireCodePage (IM3Runtime i_runtime)
{
    परत AcquireCodePageWithCapacity(i_runtime, d_m3_Code_Page_Free_ओळी_Threshold);
}


शून्य ReleaseCodePage (IM3Runtime i_runtime, IM3CodePage i_codePage)
{
    जर (i_codePage) {
        ReleaseCodePageNoTrack(i_runtime, i_codePage);
        i_runtime->अंक_Active_Code_Pages--;

#जर defined(DEBUG)
        u32 अंक_Open = मोजणी_Code_Pages(i_runtime->pagesOpen);
        u32 अंक_Full = मोजणी_Code_Pages(i_runtime->pagesFull);

        m3log(runtime, "runtime: %p; open-pages: %d; full-pages: %d; active: %d; total: %d", i_runtime, अंक_Open, अंक_Full, i_runtime->अंक_Active_Code_Pages, i_runtime->अंक_Code_Pages);

        d_m3Assert(अंक_Open + अंक_Full + i_runtime->अंक_Active_Code_Pages == i_runtime->अंक_Code_Pages);

#  जर d_m3LogCodePages
        dump_code_page(i_codePage, /* startPC: */ रिक्त);
#  endif
#endif
    }
}


#जर d_m3VerboseErrorMessages
M3Result m3Error (M3Result i_result, IM3Runtime i_runtime, IM3Module i_module, IM3Function i_function,
                  स्थिरांक अक्षर* स्थिरांक i_file, u32 i_ओळ_अंक, स्थिरांक अक्षर* स्थिरांक i_errorMessage, ...)
{
    जर (i_runtime) {
        i_runtime->error         = (M3ErrorInfo){ .result = i_result, .runtime = i_runtime, .module = i_module, .function = i_function, .file = i_file, .ओळ_2 = i_ओळ_अंक };
        i_runtime->error.message = i_runtime->error_message;

        बहु_सूची args;
        बहु_शुरू(args, i_errorMessage);
        vsnprintf(i_runtime->error_message, आकार(i_runtime->error_message), i_errorMessage, args);
        बहु_पूर्ण(args);
    }

    परत i_result;
}
#endif


शून्य m3_GetErrorInfo (IM3Runtime i_runtime, M3ErrorInfo* o_info)
{
    जर (i_runtime) {
        *o_info = i_runtime->error;
        m3_ResetErrorInfo(i_runtime);
    }
}


शून्य m3_ResetErrorInfo (IM3Runtime i_runtime)
{
    जर (i_runtime) {
        M3_INIT(i_runtime->error);
        i_runtime->error.message = "";
    }
}

uint8_t* m3_GetMemory (IM3Module i_module, माप_प्रकार* o_memorySizeInBytes, uint32_t i_memoryIndex)
{
    uint8_t* memory = रिक्त;
    माप_प्रकार   size   = 0;

    जर (i_module and i_memoryIndex < i_module->अंक_Memories) {
        IM3Memory mem = i_module->memories[i_memoryIndex];

        जर (mem->mallocated) {
            size = mem->mallocated->length;

            जर (size) {
                memory = m3MemData(mem->mallocated);
            }
        }
    }

    जर (o_memorySizeInBytes) {
        *o_memorySizeInBytes = size;
    }

    परत memory;
}


माप_प्रकार m3_GetMemorySize (IM3Module i_module, uint32_t i_memoryIndex)
{
    जर (not i_module or i_memoryIndex >= i_module->अंक_Memories) {
        परत 0;
    }

    IM3Memory mem = i_module->memories[i_memoryIndex];

    परत mem->mallocated ? mem->mallocated->length : 0;
}


माप_प्रकार m3_GetMemorySizeAt (स्थिरांक शून्य* i_memory)
{
    जर (not i_memory) {
        परत 0;
    }

    // the header sits immediately before the data it describes
    स्थिरांक M3MemoryHeader* header = ((स्थिरांक M3MemoryHeader*)i_memory) - 1;

    परत header->length;
}


M3Result m3_FindExportedMemory (IM3Module i_module, cstr_t i_name, u32* o_memoryIndex)
{
    जर (not i_module or not i_name or not o_memoryIndex) {
        परत m3Err_unknownMemory;
    }

    स्थिरांक M3Export* e = Module_FindExport(i_module, d_externalKind_memory, i_name);

    जर (not e) {
        परत m3Err_unknownMemory;
    }

    *o_memoryIndex = e->index;
    परत m3Err_none;
}


bool Module_HasLinkedHostImport (IM3Module i_module, cstr_t i_importModule)
{
    साठी (u32 i = 0; i < i_module->अंक_Functions; ++i) {
        IM3Function f = &i_module->functions[i];

        // hostMemory is set when a host function is bound to the import, so it
        // is also the mark of one - see CompileRawFunction
        जर (not f->hostMemory or f->wasm or f->resolved) {
            सुरूठेवा;
        }

        जर (f->import.moduleUtf8 and म_भेद(f->import.moduleUtf8, i_importModule) == 0) {
            परत true;
        }
    }

    परत false;
}


M3Result m3_BindImportMemory (IM3Module io_module, cstr_t i_importModule, u32 i_memoryIndex)
{
    जर (not io_module or not i_importModule) {
        परत m3Err_moduleNotLinked;
    }

    जर (i_memoryIndex >= io_module->अंक_Memories) {
        परत m3Err_unknownMemory;
    }

    // the memory itself, not the index: linking may have pointed this slot at
    // another module's memory, and growing one reallocates behind it
    IM3Memory memory = io_module->memories[i_memoryIndex];

    स्थिरांक bool wildcardModule = (म_भेद(i_importModule, "*") == 0);

    साठी (u32 i = 0; i < io_module->अंक_Functions; ++i) {
        IM3Function f = &io_module->functions[i];

        // only an import that a host function was bound to has a memory to
        // pin: one with a body of its own reads memory through the interpreter,
        // and one resolved to another module's export runs that module's code
        जर (not f->hostMemory or f->wasm or f->resolved) {
            सुरूठेवा;
        }

        जर (wildcardModule or (f->import.moduleUtf8 and म_भेद(f->import.moduleUtf8, i_importModule) == 0)) {
            f->hostMemory = memory;
        }
    }

    परत m3Err_none;
}


M3BacktraceInfo* m3_GetBacktrace (IM3Runtime i_runtime)
{
#जर d_m3RecordBacktraces
    परत &i_runtime->backtrace;
#अन्यथा
    परत रिक्त;
#endif
}


//-------------------------------------------------------------------------------------------------------------------------------
//  snapshots & suspendable execution
//-------------------------------------------------------------------------------------------------------------------------------

शून्य m3_SetSuspendable (IM3Runtime io_runtime, bool i_suspendable)
{
    जर (io_runtime) {
        io_runtime->isSuspendable = i_suspendable;
    }
}

शून्य m3_RequestSuspend (IM3Runtime io_runtime)
{
    जर (io_runtime) {
        io_runtime->suspendRequested = true;
    }
}

bool m3_IsSuspended (IM3Runtime i_runtime)
{
    जर (!i_runtime) {
        परत false;
    }
#जर d_m3_Has_चळत_Switching
    जर (i_runtime->मूळ_Continuation && i_runtime->मूळ_Continuation->state == cont_suspended) {
        परत true;
    }
#endif
    परत false;
}

M3Result m3_ResumeRuntime (IM3Runtime io_runtime)
{
    जर (!io_runtime) {
        परत m3Err_mallocFailed;
    }
    जर (!io_runtime->isSuspendable) {
        परत m3Err_none;
    }
#जर d_m3HasSnapshots
    // a restore that failed leaves nothing to resume, and says so every time
    // rather than resuming nothing successfully
    जर (io_runtime->मूळ_Continuation and io_runtime->मूळ_Continuation->entryFunction and
        io_runtime->मूळ_Continuation->entryFunction->module->isUnusable) {
        परत "a snapshot failed to restore into the module, so it cannot run";
    }
#endif
#जर d_m3_Has_चळत_Switching
    जर (!io_runtime->मूळ_Continuation || io_runtime->मूळ_Continuation->state != cont_suspended) {
        परत m3Err_none;
    }
    // what finishes returns into the bottom of the stack, where it was entered
    io_runtime->चळत = io_runtime->origin_चळत;

    // this is the outermost call again, as far as a host function calling back
    // into Wasm from inside it can tell
    io_runtime->entered = io_runtime->मूळ_Continuation->entryFunction;
    io_runtime->callNesting++;

    d_m3_चळत_Limit_Enter(io_runtime);
    io_runtime->resumePastCheck = true;
    M3Result result             = (M3Result)ResumeContinuation(io_runtime, io_runtime->मूळ_Continuation);
    io_runtime->resumePastCheck = false;
    d_m3_चळत_Limit_Leave(io_runtime);

#  जर d_m3HasExceptionHandling
    जर (M3_UNLIKELY(result == m3Err_pendingException)) {
        result = m3Err_trapUncaughtException;
    }
    RecordUncaughtException(io_runtime, result);
#  endif

    io_runtime->callNesting--;

#  जर d_m3HasExceptionHandling
    जर (result != m3Err_continuationSuspended and io_runtime->callNesting == 0) {
        FreeExceptions(io_runtime);
    }
#  endif

    // the results are where the paused call left room for them, so a finished
    // resume can be asked for them the way the call itself could have been
    io_runtime->lastCalled = (result == m3Err_none) ? io_runtime->मूळ_Continuation->entryFunction : रिक्त;

    Runtime_Place_Call_चळत(io_runtime);
    परत result;
#अन्यथा
    परत m3Err_none;
#endif
}
