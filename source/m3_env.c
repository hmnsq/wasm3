//
//  m3_env.c
//
//  Created by Steven Massey on 4/19/19.
//  Copyright © 2019 Steven Massey. All rights reserved.
//

#અન્તર્ભાવ <stdarg.h>
#અન્તર્ભાવ <limits.h>
#અન્તર્ભાવ <errno.h>
#અન્તર્ભાવ <float.h>
#અન્તર્ભાવ <ctype.h>

#અન્તર્ભાવ "m3_env.h"
#અન્તર્ભાવ "m3_bind.h"
#અન્તર્ભાવ "m3_deterministic.h"
#અન્તર્ભાવ "m3_compile.h"
#અન્તર્ભાવ "m3_host.h"
#અન્તર્ભાવ "m3_exception.h"
#અન્તર્ભાવ "m3_info.h"


IM3Environment m3_NewEnvironment ()
{
    IM3Environment env = m3_AllocStruct(M3Environment);

    જો (env) {
        _try
        {
            // create FuncTypes for all simple block return ValueTypes.
            // v128 is skipped: it parses as a slot but has no operations.
            માટે (u8 t = c_m3Type_none; t < c_m3_Type_ગણતરી; t++) {
                જો (t == c_m3Type_v128) {
                    ચાલુ;
                }

                IM3FuncType ftype;
_               (AllocFuncType(&ftype, 1));

                ftype->અંક_Args  = 0;
                ftype->અંક_Rets  = (t == c_m3Type_none) ? 0 : 1;
                ftype->types[0] = t;

                Environment_AddFuncType(env, &ftype);

                env->retFuncTypes[t] = ftype;
            }
        }

        _catch:
        જો (result) {
            m3_FreeEnvironment(env);
            env = ખાલી;
        }
    }

    પાછા env;
}


શૂન્ય Environment_Release (IM3Environment i_environment)
{
    IM3FuncType ftype = i_environment->funcTypes;

    જ્યારે (ftype) {
        IM3FuncType next = ftype->next;
        m3_Free(ftype);
        ftype = next;
    }

    m3log(runtime, "freeing %d pages from environment", ગણતરી_Code_Pages(i_environment->pagesReleased));
    FreeCodePages(&i_environment->pagesReleased);
}


શૂન્ય m3_FreeEnvironment (IM3Environment i_environment)
{
    જો (i_environment) {
        Environment_Release(i_environment);
        m3_Free(i_environment);
    }
}


શૂન્ય m3_SetCustomSectionHandler (IM3Environment i_environment, M3SectionHandler i_handler)
{
    જો (i_environment) {
        i_environment->customSectionHandler = i_handler;
    }
}


// returns the same io_funcType or replaces it with an equivalent that's already in the type linked list
M3Result Environment_AddFuncType (IM3Environment i_environment, IM3FuncType* io_funcType)
{
    IM3FuncType addType = *io_funcType;
    IM3FuncType newType = i_environment->funcTypes;

    જ્યારે (newType) {
        જો (not newType->inRecGroup and AreFuncTypesEqual(newType, addType)) {
            m3_Free(addType);
            તોડો;
        }

        newType = newType->next;
    }

    જો (newType == ખાલી) {
        // a type index has to fit in the heap type field of an m3type_t
        જો (i_environment->અંક_Func_Types >= d_m3_Max_Sane_Types_ગણતરી) {
            m3_Free(addType);
            *io_funcType = ખાલી;
            પાછા "too many distinct function types";
        }

        newType                  = addType;
        newType->canonicalIndex  = i_environment->અંક_Func_Types++;
        newType->next            = i_environment->funcTypes;
        i_environment->funcTypes = newType;
    }

    *io_funcType = newType;

    પાછા m3Err_none;
}


// Adds a type without looking for an equal one first, and hands back the index
// it was given.
//
// Members of a recursive group do not get to share an entry with a type that
// merely looks like them: the group is their identity, so two groups spelled
// the same way are still two groups, and a type inside one is never the type
// outside it that matches field for field. Reserving the indices up front is
// also what lets the members refer to each other while they are being read.
M3Result Environment_ReserveFuncTypes (IM3Environment i_environment, u32 i_ગણતરી, u16* o_firstIndex)
{
    જો ((u64)i_environment->અંક_Func_Types + i_ગણતરી > d_m3_Max_Sane_Types_ગણતરી) {
        પાછા "too many distinct function types";
    }

    *o_firstIndex = i_environment->અંક_Func_Types;
    i_environment->અંક_Func_Types += (u16)i_ગણતરી;

    પાછા m3Err_none;
}


શૂન્ય Environment_AdoptFuncType (IM3Environment i_environment, IM3FuncType i_funcType, u16 i_canonicalIndex)
{
    i_funcType->inRecGroup     = true;
    i_funcType->canonicalIndex = i_canonicalIndex;
    i_funcType->next           = i_environment->funcTypes;
    i_environment->funcTypes   = i_funcType;
}


IM3CodePage RemoveCodePageOfCapacity (M3CodePage** io_list, u32 i_minimum_લીટી_ગણતરી)
{
    IM3CodePage prev = ખાલી;
    IM3CodePage page = *io_list;

    જ્યારે (page) {
        જો (અંક_Free_લીટીઓ(page) >= i_minimum_લીટી_ગણતરી) {             d_m3Assert (page->info.usage_ગણતરી == 0);
            IM3CodePage next = page->info.next;
            જો (prev) {
                prev->info.next = next; // mid-list
            } નહીંતર {
                *io_list = next;       // front of list
            }

            તોડો;
        }

        prev = page;
        page = page->info.next;
    }

    પાછા page;
}


IM3CodePage Environment_AcquireCodePage (IM3Environment i_environment, u32 i_minimum_લીટી_ગણતરી)
{
    પાછા RemoveCodePageOfCapacity(&i_environment->pagesReleased, i_minimum_લીટી_ગણતરી);
}


શૂન્ય Environment_ReleaseCodePages (IM3Environment i_environment, IM3CodePage i_codePageList)
{
    IM3CodePage end = i_codePageList;

    જ્યારે (end) {
        end->info.લીટી_Index = 0; // reset page
#જો d_m3RecordBacktraces
        end->info.mapping->size = 0;
#endif // d_m3RecordBacktraces

        IM3CodePage next = end->info.next;
        જો (not next) {
            તોડો;
        }

        end = next;
    }

    જો (end) {
        // push list to front
        end->info.next               = i_environment->pagesReleased;
        i_environment->pagesReleased = i_codePageList;
    }
}


IM3Runtime m3_NewRuntime (IM3Environment i_environment, u32 i_ઢગલો_Size_In_Bytes, શૂન્ય* i_userdata)
{
    IM3Runtime runtime = m3_AllocStruct(M3Runtime);

    જો (runtime) {
        m3_ResetErrorInfo(runtime);

        runtime->environment = i_environment;
        runtime->userdata    = i_userdata;

        Deterministic_InitRuntime(runtime);

        runtime->origin_ઢગલો = m3_Malloc("Wasm Stack", i_ઢગલો_Size_In_Bytes + 4 * કદ(m3slot_t)); // TODO: more precise stack checks

        જો (runtime->origin_ઢગલો) {
            runtime->ઢગલો         = runtime->origin_ઢગલો;
            runtime->અંક_ઢગલો_Slots = i_ઢગલો_Size_In_Bytes / કદ(m3slot_t);          m3log (runtime, "new stack: %p, slots: %u", runtime->origin_ઢગલો, runtime->અંક_ઢગલો_Slots);
        } નહીંતર {
            m3_Free(runtime);
        }
    }

    પાછા runtime;
}

શૂન્ય m3_SetValidation (IM3Runtime i_runtime, bool i_enable)
{
#જો d_m3EnableValidation
    જો (i_runtime) {
        i_runtime->skipValidation = not i_enable;
    }
#નહીંતર
    (શૂન્ય)i_runtime;
    (શૂન્ય)i_enable;              // nothing to skip: the validator was compiled out
#endif
}

M3Result m3_SetResourceLimit (IM3Runtime i_runtime, M3ResourceLimit i_limit, uint64_t i_કિંમત)
{
    જો (not i_runtime) {
        પાછા m3Err_mallocFailed;
    }
    પસંદગી (i_limit) {
    સ્થિતિ c_m3Limit_MemoryBytes:
        જો (i_કિંમત and i_કિંમત < i_runtime->memoryBytesUsed) {
            પાછા m3Err_resourceLimitBelowUsage;
        }
        i_runtime->memoryBytesLimit = i_કિંમત;
        પાછા m3Err_none;
    સ્થિતિ c_m3Limit_TableElements:
        જો (i_કિંમત and i_કિંમત < i_runtime->tableElementsUsed) {
            પાછા m3Err_resourceLimitBelowUsage;
        }
        i_runtime->tableElementsLimit = i_કિંમત;
        પાછા m3Err_none;
#જો d_m3_Has_ઢગલો_Switching
    સ્થિતિ c_m3Limit_Continuations:
        જો (i_કિંમત and i_કિંમત < i_runtime->continuationsAllocated) {
            પાછા m3Err_resourceLimitBelowUsage;
        }
        i_runtime->continuationsLimit = (u32)M3_MIN(i_કિંમત, (u64)UINT32_MAX);
        જ્યારે (i_કિંમત and i_runtime->val_ઢગલો_Pool_ગણતરી > i_કિંમત - i_runtime->continuationsAllocated) {
            m3_Free(i_runtime->val_ઢગલો_Pool[--i_runtime->val_ઢગલો_Pool_ગણતરી]);
        }
        પાછા m3Err_none;
#નહીંતર
    સ્થિતિ c_m3Limit_Continuations: પાછા m3Err_resourceLimitNotSupported;
#endif
    સ્થિતિ c_m3Limit_GasUnits:
#જો d_m3HasGasMetering
        i_runtime->gasLimit = i_runtime->gasRemaining = (i64)M3_MIN(i_કિંમત, (u64)INT64_MAX);
        પાછા m3Err_none;
#નહીંતર
        પાછા m3Err_resourceLimitNotSupported;
#endif
    મૂળભૂત: પાછા m3Err_unknownResourceLimit;
    }
}

uint64_t m3_GetResourceLimit (IM3Runtime i_runtime, M3ResourceLimit i_limit)
{
    જો (not i_runtime) {
        પાછા 0;
    }
    પસંદગી (i_limit) {
    સ્થિતિ c_m3Limit_MemoryBytes: પાછા i_runtime->memoryBytesLimit;
    સ્થિતિ c_m3Limit_TableElements: પાછા i_runtime->tableElementsLimit;
#જો d_m3_Has_ઢગલો_Switching
    સ્થિતિ c_m3Limit_Continuations: પાછા i_runtime->continuationsLimit;
#endif
#જો d_m3HasGasMetering
    સ્થિતિ c_m3Limit_GasUnits: પાછા (u64)i_runtime->gasLimit;
#endif
    મૂળભૂત: પાછા 0;
    }
}

uint64_t m3_GetResourceUsage (IM3Runtime i_runtime, M3ResourceLimit i_limit)
{
    જો (not i_runtime) {
        પાછા 0;
    }
    પસંદગી (i_limit) {
    સ્થિતિ c_m3Limit_MemoryBytes: પાછા i_runtime->memoryBytesUsed;
    સ્થિતિ c_m3Limit_TableElements: પાછા i_runtime->tableElementsUsed;
#જો d_m3_Has_ઢગલો_Switching
    સ્થિતિ c_m3Limit_Continuations: પાછા i_runtime->continuationsAllocated;
#endif
#જો d_m3HasGasMetering
    સ્થિતિ c_m3Limit_GasUnits: પાછા (u64)i_runtime->gasLimit - (u64)i_runtime->gasRemaining;
#endif
    મૂળભૂત: પાછા 0;
    }
}

શૂન્ય* m3_GetUserData (IM3Runtime i_runtime)
{
    પાછા i_runtime ? i_runtime->userdata : ખાલી;
}


શૂન્ય* ForEachModule (IM3Runtime i_runtime, ModuleVisitor i_visitor, શૂન્ય* i_info)
{
    શૂન્ય* r = ખાલી;

    IM3Module module = i_runtime->modules;

    જ્યારે (module) {
        IM3Module next = module->next;
        જો ((r = i_visitor(module, i_info))) {
            તોડો;
        }

        module = next;
    }

    પાછા r;
}


શૂન્ય* _FreeModule (IM3Module i_module, શૂન્ય* i_info)
{
    m3_FreeModule(i_module);
    પાછા ખાલી;
}


// A module's memory and table index spaces hold borrowed pointers: linking
// repoints an imported slot at the memory or table the exporting module owns.
// m3_FreeModule tells a borrowed slot from an owned one by reading that owner,
// which only holds while the object is still allocated - and the free walk
// below visits modules in load order, with no ordering between an owner and
// the modules borrowing from it. So clear the borrowed slots first, in a pass
// that frees nothing; the walk then skips them on the NULL check it already
// does, instead of reading a memory some earlier module has freed.
શૂન્ય* _ReleaseBorrowedSlots (IM3Module i_module, શૂન્ય* i_info)
{
    માટે (u32 i = 0; i < i_module->અંક_Memories; ++i) {
        IM3Memory memory = i_module->memories[i];

        જો (memory and memory->owner != i_module) {
            i_module->memories[i] = ખાલી;
        }
    }

    માટે (u32 i = 0; i < i_module->અંક_Tables; ++i) {
        IM3Table table = i_module->tables[i];

        જો (table and table->owner != i_module) {
            i_module->tables[i] = ખાલી;
        }
    }

    પાછા ખાલી;
}


શૂન્ય Runtime_Release (IM3Runtime i_runtime)
{
    ForEachModule(i_runtime, _ReleaseBorrowedSlots, ખાલી);
    ForEachModule(i_runtime, _FreeModule, ખાલી);                    d_m3Assert (i_runtime->અંક_Active_Code_Pages == 0);

    Environment_ReleaseCodePages(i_runtime->environment, i_runtime->pagesOpen);
    Environment_ReleaseCodePages(i_runtime->environment, i_runtime->pagesFull);

    m3_Free(i_runtime->origin_ઢગલો);

#જો d_m3EnableValidation
    m3_Free(i_runtime->validator);
#endif

#જો d_m3HasExceptionHandling
    // a runtime let go of while suspended still holds its exceptions
    FreeExceptions(i_runtime);
    m3_Free(i_runtime->uncaughtException);
#endif
#જો d_m3HasExceptionHandling || d_m3_Has_ઢગલો_Switching
    જ્યારે (i_runtime->hostTags) {
        M3HostTag* next = i_runtime->hostTags->next;
        m3_Free(i_runtime->hostTags);
        i_runtime->hostTags = next;
    }
#endif
#જો d_m3_Has_ઢગલો_Switching
    Continuation_ReleaseAll(i_runtime);
#endif
}


શૂન્ય m3_FreeRuntime (IM3Runtime i_runtime)
{
    જો (i_runtime) {
        m3_PrintProfilerInfo();

        Runtime_Release(i_runtime);
        m3_Free(i_runtime);
    }
}


#જો d_m3_Has_ઢગલો_Switching

IM3Continuation Continuation_New (IM3Runtime i_runtime, IM3FuncType i_type, IM3Function i_function)
{
    IM3Continuation cont = m3_AllocStruct(M3Continuation);
    જો (!cont) {
        પાછા ખાલી;
    }

    cont->type          = i_type;
    cont->entryFunction = i_function;
    cont->state         = cont_allocated;

    cont->અંક_ઢગલો_Slots = d_m3_Continuation_ઢગલો_Slots;
    // frames are recorded only when this continuation actually suspends, and
    // only as deep as it got, so the array starts out unallocated
    cont->frames    = ખાલી;
    cont->framesCap = 0;
    cont->અંક_Frames = 0;

    cont->sp = cont->val_ઢગલો;
    cont->pc = i_function ? i_function->compiled : ખાલી;
    cont->r0 = 0;
#  જો d_m3HasFloat
    cont->fp0 = 0;
#  endif
    cont->bound_Args_ગણતરી    = 0;
    cont->અંક_Suspend_Results = 0;
    cont->અંક_Handlers       = 0;
    cont->handlersPC        = ખાલી;
    cont->parent            = ખાલી;

    cont->next               = i_runtime->continuations;
    i_runtime->continuations = cont;

    પાછા cont;
}

IM3Continuation Continuation_ForkSuspended (IM3Runtime i_runtime, IM3Continuation i_cont)
{
    IM3Continuation newCont = Continuation_New(i_runtime, i_cont->type, i_cont->entryFunction);
    જો (M3_UNLIKELY(not newCont)) {
        પાછા ખાલી;
    }

    newCont->val_ઢગલો      = i_cont->val_ઢગલો;
    newCont->અંક_ઢગલો_Slots = i_cont->અંક_ઢગલો_Slots;
    newCont->sp            = i_cont->sp;
    newCont->pc            = i_cont->pc;
    newCont->suspendPoint  = i_cont->suspendPoint;
    newCont->r0            = i_cont->r0;
#  જો d_m3HasFloat
    newCont->fp0 = i_cont->fp0;
#  endif
    newCont->frames    = i_cont->frames;
    newCont->framesCap = i_cont->framesCap;
    newCont->અંક_Frames = i_cont->અંક_Frames;

    newCont->bound_Args_ગણતરી = 0;
#  જો d_m3HasExceptionHandling
    newCont->resumeThrow = i_cont->resumeThrow;
    i_cont->resumeThrow  = ખાલી;
#  endif
    newCont->અંક_Suspend_Results = i_cont->અંક_Suspend_Results;
    માટે (u32 i = 0; i < i_cont->અંક_Suspend_Results; ++i) {
        newCont->suspendResultOffsets[i] = i_cont->suspendResultOffsets[i];
        newCont->suspendResultIs64[i]    = i_cont->suspendResultIs64[i];
    }

    newCont->અંક_Handlers = i_cont->અંક_Handlers;
    newCont->handlersPC  = i_cont->handlersPC;
    newCont->parent      = i_cont->parent;
    newCont->state       = cont_suspended;

    i_cont->val_ઢગલો  = ખાલી;
    i_cont->frames    = ખાલી;
    i_cont->framesCap = 0;
    i_cont->અંક_Frames = 0;
    i_cont->state     = cont_consumed;

    પાછા newCont;
}

// Only cont.new and snapshot restoration acquire stacks. Binding and suspension
// transfer them between records without changing the active count.
M3Result Continuation_Acquire_ઢગલો (IM3Runtime runtime, IM3Continuation cont)
{
    જો (not Continuation_Can_Acquire_ઢગલો(runtime)) {
        પાછા m3Err_continuationLimitExceeded;
    }
    જો (runtime->val_ઢગલો_Pool_ગણતરી) {
        cont->val_ઢગલો = runtime->val_ઢગલો_Pool[--runtime->val_ઢગલો_Pool_ગણતરી];
        memset(cont->val_ઢગલો, 0, (cont->અંક_ઢગલો_Slots + 4) * કદ(m3slot_t));
    } નહીંતર {
        cont->val_ઢગલો = m3_AllocArray(m3slot_t, cont->અંક_ઢગલો_Slots + 4);
    }
    જો (not cont->val_ઢગલો) {
        પાછા m3Err_mallocFailed;
    }
    cont->sp = cont->val_ઢગલો;
    runtime->continuationsAllocated++;
    પાછા m3Err_none;
}

શૂન્ય Continuation_Recycle_ઢગલો (IM3Runtime runtime, IM3Continuation cont)
{
    જો (not cont->val_ઢગલો or cont == runtime->મૂળ_Continuation) {
        પાછા;
    }
    જો (not runtime->val_ઢગલો_Pool) {
        runtime->val_ઢગલો_Pool = m3_AllocArray(m3slot_t*, 16);
        જો (runtime->val_ઢગલો_Pool) {
            runtime->val_ઢગલો_Pool_Cap = 16;
        }
    }
    જો (runtime->val_ઢગલો_Pool_ગણતરી < runtime->val_ઢગલો_Pool_Cap and
        (not runtime->continuationsLimit or runtime->val_ઢગલો_Pool_ગણતરી <
                                              runtime->continuationsLimit - runtime->continuationsAllocated + 1)) {
        runtime->val_ઢગલો_Pool[runtime->val_ઢગલો_Pool_ગણતરી++] = cont->val_ઢગલો;
        cont->val_ઢગલો                                      = ખાલી;
    } નહીંતર {
        m3_Free(cont->val_ઢગલો);
    }
    cont->sp = ખાલી;
    runtime->continuationsAllocated--;
}

શૂન્ય Continuation_ReleaseAll (IM3Runtime io_runtime)
{
    // Records live until teardown: a consumed reference can still be in a table
    // or on a Wasm stack. Its value stack has already been recycled.
    IM3Continuation cont = io_runtime->continuations;

    જ્યારે (cont) {
        IM3Continuation next = cont->next;

        m3_Free(cont->val_ઢગલો);
        m3_Free(cont->frames);
        m3_Free(cont);

        cont = next;
    }

    માટે (u32 i = 0; i < io_runtime->val_ઢગલો_Pool_ગણતરી; ++i) {
        m3_Free(io_runtime->val_ઢગલો_Pool[i]);
    }
    m3_Free(io_runtime->val_ઢગલો_Pool);
    io_runtime->val_ઢગલો_Pool_ગણતરી = io_runtime->val_ઢગલો_Pool_Cap = 0;
    io_runtime->continuationsAllocated                          = 0;
    io_runtime->continuations                                   = ખાલી;
    io_runtime->activeContinuation                              = ખાલી;
    io_runtime->મૂળ_Continuation                                = ખાલી;
}


m3ret_t Continuation_RecordFrame (IM3Runtime i_runtime, અચલ M3Frame* i_frame)
{
    IM3Continuation cont = i_runtime->activeContinuation;

    // the marker only travels between a suspend and the resume that runs it,
    // and that whole stretch has an active continuation
    d_m3Assert(cont);

    જો (cont->અંક_Frames == cont->framesCap) {
        u32 newCap = cont->framesCap ? cont->framesCap * 2 : 8;

        જો (newCap > d_m3ContinuationMaxFrames) {
            newCap = d_m3ContinuationMaxFrames;
        }

        જો (newCap == cont->framesCap) {
            પાછા m3_Err_trap_ઢગલો_Overflow;
        }

        M3Frame* frames = m3_ReallocArray(M3Frame, cont->frames, newCap, cont->framesCap);

        જો (not frames) {
            પાછા m3Err_mallocFailed;
        }

        cont->frames    = frames;
        cont->framesCap = newCap;
    }

    cont->frames[cont->અંક_Frames++] = *i_frame;

    પાછા m3Err_continuationSuspended;
}

#endif // d_m3HasStackSwitching

M3Result EvaluateExpression (IM3Module i_module, શૂન્ય* o_expressed, m3type_t i_type, bytes_t* io_bytes, cbytes_t i_end)
{
    M3Result result = m3Err_none;

    // OPTZ: use a simplified interpreter for expressions

    // create a temporary runtime context
#જો defined(d_m3PreferStaticAlloc)
    સ્થિર M3Runtime runtime;
#નહીંતર
    M3Runtime runtime;
#endif
    M3_INIT(runtime);

    runtime.environment   = i_module->runtime->environment;
    runtime.અંક_ઢગલો_Slots = i_module->runtime->અંક_ઢગલો_Slots;
    runtime.ઢગલો         = i_module->runtime->ઢગલો;

    m3stack_t ઢગલો = (m3stack_t)runtime.ઢગલો;

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

    જો (o->page) {
        IM3FuncType ftype = ખાલી;

#જો d_m3HasTypedRefs
        // A reference type has to be checked as the type it is: the shared
        // return types are one per storage class, so borrowing one of those
        // would let any reference at all satisfy a particular one.
        જો (IsSpelledRefType(i_type)) {
            result = AllocFuncType(&ftype, 1);

            જો (not result) {
                ftype->અંક_Rets  = 1;
                ftype->types[0] = i_type;

                result = Environment_AddFuncType(runtime.environment, &ftype);
            }
        } નહીંતર
#endif
        {
            ftype = runtime.environment->retFuncTypes[BaseTypeOf(i_type)];
        }

        pc_t m3code = GetPagePC(o->page);

        જો (not result) {
            result = CompileExpression(o, ftype);
        }

        જો (not result && o->max_ઢગલો_Slots >= runtime.અંક_ઢગલો_Slots) {
            result = m3_Err_trap_ઢગલો_Overflow;
        }

        જો (not result) {
#જો (d_m3EnableOpProfiling || d_m3EnableOpTracing)
            m3ret_t r = RunCode(m3code, ઢગલો, ખાલી, d_m3OpDefaultArgs, d_m3BaseCstr);
#નહીંતર
            m3ret_t r = RunCode(m3code, ઢગલો, ખાલી, d_m3OpDefaultArgs);
#endif

            જો (r == 0) {                                                                   m3log (runtime, "expression result: %s", SPrint_કિંમત (ઢગલો, i_type));
                જો (SizeOfType(BaseTypeOf(i_type)) == કદ(u32)) {
                    *(u32*)o_expressed = *((u32*)ઢગલો);
                } નહીંતર {
                    *(u64*)o_expressed = *((u64*)ઢગલો);
                }
            }
        }

        ReleaseCodePage(&runtime, o->page);
    } નહીંતર {
        result = m3Err_mallocFailedCodePage;
    }

    runtime.origin_ઢગલો = ખાલી;        // prevent free(stack) in ReleaseRuntime
    Runtime_Release(&runtime);
    i_module->runtime = savedRuntime;

    *io_bytes = o->wasm;

    પાછા result;
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
સ્થિર
bool LimitsSatisfy (u64 i_exportedSize, bool i_exportedHasMax, u64 i_exportedMax,
                    u64 i_importMin, bool i_importHasMax, u64 i_importMax)
{
    જો (i_exportedSize < i_importMin) {
        પાછા false;
    }

    જો (i_importHasMax and (not i_exportedHasMax or i_exportedMax > i_importMax)) {
        પાછા false;
    }

    પાછા true;
}


// The export of this kind under this name, or NULL. The index it carries is
// resolved by the caller, against the module's index space as it stands then -
// after linking, a re-exported import's slot holds the entity it was linked to.
સ્થિર
અચલ M3Export* Module_FindExport (IM3Module i_module, u8 i_kind, cstr_t i_name)
{
    માટે (u32 i = 0; i < i_module->અંક_Exports; ++i) {
        અચલ M3Export* e = &i_module->exports[i];

        જો (e->kind == i_kind and strcmp(e->name, i_name) == 0) {
            પાછા e;
        }
    }

    પાછા ખાલી;
}


સ્થિર
IM3Function Module_FindExportedFunction (IM3Module i_module, cstr_t i_name)
{
    અચલ M3Export* e = Module_FindExport(i_module, d_externalKind_function, i_name);

    // A module that re-exports an import names the placeholder here. Resolve to
    // the function that actually runs, or a host-side call would run it against
    // the importing module's memory.
    પાછા e ? Function_Implementation(&i_module->functions[e->index]) : ખાલી;
}


સ્થિર
IM3Memory Module_FindExportedMemory (IM3Module i_module, cstr_t i_name)
{
    અચલ M3Export* e = Module_FindExport(i_module, d_externalKind_memory, i_name);

    પાછા e ? i_module->memories[e->index] : ખાલી;
}


સ્થિર
IM3Table Module_FindExportedTable (IM3Module i_module, cstr_t i_name)
{
    અચલ M3Export* e = Module_FindExport(i_module, d_externalKind_table, i_name);

    પાછા e ? i_module->tables[e->index] : ખાલી;
}


સ્થિર
IM3Global Module_FindExportedGlobal (IM3Module i_module, cstr_t i_name)
{
    અચલ M3Export* e = Module_FindExport(i_module, d_externalKind_global, i_name);

    પાછા e ? &i_module->globals[e->index] : ખાલી;
}


#જો d_m3HasExceptionHandling || d_m3_Has_ઢગલો_Switching
સ્થિર
IM3Tag Module_FindExportedTag (IM3Module i_module, cstr_t i_name)
{
    અચલ M3Export* e = Module_FindExport(i_module, d_externalKind_tag, i_name);

    જો (not e) {
        પાછા ખાલી;
    }

    IM3Tag tag = &i_module->tags[e->index];

    પાછા tag->resolved ? tag->resolved : tag;
}
#endif


// Whether the module exports anything at all under this name. Export names are
// unique within a module, so a name one of the lookups above missed but this
// one finds is exported as something else - a kind the import cannot be
// satisfied by, rather than a name the module never exported.
સ્થિર
bool Module_HasExport (IM3Module i_module, cstr_t i_name)
{
    માટે (u32 i = 0; i < i_module->અંક_Exports; ++i) {
        જો (strcmp(i_module->exports[i].name, i_name) == 0) {
            પાછા true;
        }
    }

    પાછા false;
}


// Points each of the module's imports at whatever already-loaded module exports
// it. Runs before anything is allocated or initialized: a memory import has to
// be resolved before InitMemory would give it pages of its own, and a global
// import before InitGlobals runs an initializer that reads it.
સ્થિર
M3Result LinkImports (IM3Runtime io_runtime, IM3Module io_module)
{
    M3Result result = m3Err_none;

    માટે (u32 i = 0; i < io_module->અંક_Functions; ++i) {
        IM3Function f = &io_module->functions[i];

        જો (f->wasm or not (f->import.moduleUtf8 and f->import.fieldUtf8)) {
            ચાલુ;
        }

        IM3Module from = m3_FindModule(io_runtime, f->import.moduleUtf8);
        જો (not from) {
            ચાલુ;
        }

        IM3Function exported = Module_FindExportedFunction(from, f->import.fieldUtf8);
        જો (not exported) {
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

    માટે (u32 i = 0; i < io_module->અંક_Memories; ++i) {
        IM3Memory memory = io_module->memories[i];

        જો (not memory->imported or memory->owner != io_module) {
            ચાલુ;
        }

        IM3Module from = m3_FindModule(io_runtime, memory->import.moduleUtf8);
        જો (not from) {
            ચાલુ;
        }

        IM3Memory exported = Module_FindExportedMemory(from, memory->import.fieldUtf8);
        જો (not exported) {
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
                 not LimitsSatisfy(exported->અંક_Pages, exported->hasMax, exported->maxPages,
                                   memory->initPages, memory->hasMax, memory->maxPages));

        // hand the slot over to the exporter's memory, and drop the placeholder
        FreeMemoryBlock(memory);
        FreeImportInfo(&memory->import);
        m3_Free(memory);

        io_module->memories[i] = exported;
    }

    માટે (u32 i = 0; i < io_module->અંક_Tables; ++i) {
        IM3Table table = io_module->tables[i];

        જો (not table->imported or table->owner != io_module) {
            ચાલુ;
        }

        IM3Module from = m3_FindModule(io_runtime, table->import.moduleUtf8);
        જો (not from) {
            ચાલુ;
        }

        IM3Table exported = Module_FindExportedTable(from, table->import.fieldUtf8);
        જો (not exported) {
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

    માટે (u32 i = 0; i < io_module->અંક_Globals; ++i) {
        IM3Global g = &io_module->globals[i];

        જો (not g->imported or not (g->import.moduleUtf8 and g->import.fieldUtf8)) {
            ચાલુ;
        }

        IM3Module from = m3_FindModule(io_runtime, g->import.moduleUtf8);
        જો (not from) {
            ચાલુ;
        }

        IM3Global exported = Module_FindExportedGlobal(from, g->import.fieldUtf8);
        જો (not exported) {
            _throwif(m3Err_incompatibleImportType, Module_HasExport(from, g->import.fieldUtf8));
            _throw(m3Err_unknownImport);
        }

        _throwif(m3Err_incompatibleImportType, exported->type != g->type);
        _throwif(m3Err_incompatibleImportType, exported->isMutable != g->isMutable);

        g->resolved = exported->resolved ? exported->resolved : exported;
    }

#જો d_m3HasExceptionHandling || d_m3_Has_ઢગલો_Switching
    માટે (u32 i = 0; i < io_module->અંક_Tags; ++i) {
        IM3Tag tag = &io_module->tags[i];

        જો (not tag->imported or not (tag->import.moduleUtf8 and tag->import.fieldUtf8)) {
            ચાલુ;
        }

        IM3Module from = m3_FindModule(io_runtime, tag->import.moduleUtf8);
        જો (not from) {
            ચાલુ;
        }

        IM3Tag exported = Module_FindExportedTag(from, tag->import.fieldUtf8);
        જો (not exported) {
            _throwif(m3Err_incompatibleImportType, Module_HasExport(from, tag->import.fieldUtf8));
            _throw(m3Err_unknownImport);
        }

        _throwif(m3Err_incompatibleImportType, exported->type != tag->type);

        tag->resolved = exported->resolved ? exported->resolved : exported;
    }
#endif

    _catch: પાછા result;
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
    i_module->memory0 = i_module->અંક_Memories ? i_module->memories[0]
                                              : &i_module->emptyMemory;

    જો (i_module->અંક_Memories == 0) {
        // nothing addressable, but _mem still has to point somewhere
        i_module->emptyMemory.owner    = i_module;
        i_module->emptyMemory.pageSize = d_m3DefaultMemPageSize;

_       (ResizeMemory(io_runtime, &i_module->emptyMemory, 0));
    }

    માટે (u32 i = 0; i < i_module->અંક_Memories; ++i) {
        IM3Memory memory = i_module->memories[i];

        // a slot that already points at another module's memory is that
        // module's to allocate
        જો (memory->owner != i_module or memory->mallocated) {
            ચાલુ;
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
        જો (not memory->hasMax) {
            memory->maxPages = memory->isMemory64
                                 ? (pageSize > 1 ? (UINT64_MAX / pageSize) + 1 : UINT64_MAX)
                                 : (0x100000000ull / pageSize);
        }

_       (ResizeMemory(io_runtime, memory, memory->initPages));
    }

    _catch: પાછા result;
}


M3Result ResizeMemory (IM3Runtime io_runtime, IM3Memory memory, u64 i_અંક_Pages)
{
    M3Result result = m3Err_none;

    u64 અંક_Pages_To_Alloc = i_અંક_Pages;

    જો (અંક_Pages_To_Alloc <= memory->maxPages) {
        // A 64-bit memory may ask for up to 2^48 pages, which overflows a u64
        // of bytes. Nothing that large can be backed, so refuse it up front
        // rather than multiplying into a wrapped size.
        _throwif("linear memory limitation exceeded",
                 અંક_Pages_To_Alloc > d_m3AddressLimit / memory->pageSize);

        u64 અંક_Page_Bytes = અંક_Pages_To_Alloc * memory->pageSize;

#જો d_m3MaxLinearMemoryPages > 0
        // the limit is a memory size, counted in default-sized pages; comparing
        // it against a raw page count would make it 65536 times stricter for a
        // module whose pages are one byte
        _throwif("linear memory limitation exceeded",
                 અંક_Page_Bytes > (u64)d_m3MaxLinearMemoryPages * d_m3DefaultMemPageSize);
#endif

        // Limit the amount of memory that gets actually allocated
        જો (io_runtime->memoryLimit) {
            અંક_Page_Bytes = M3_MIN(અંક_Page_Bytes, (u64)io_runtime->memoryLimit);
        }

        u64 oldLength = memory->mallocated ? memory->mallocated->length : 0;
        u64 otherUsed = io_runtime->memoryBytesUsed - oldLength;
        _throwif(m3Err_memoryLimitExceeded, io_runtime->memoryBytesLimit and
                                              (અંક_Page_Bytes > io_runtime->memoryBytesLimit or
                                               otherUsed > io_runtime->memoryBytesLimit - અંક_Page_Bytes));

        _throwif("linear memory limitation exceeded", અંક_Page_Bytes > (u64)SIZE_MAX - કદ(M3MemoryHeader));

#જો d_m3GuardedMemory

        // A slot holds every address a Wasm access can name, and nothing beyond
        // that: a memory larger than the data part of one would put addresses past
        // the guard, where a fault is nobody's to recognize
        _throwif("linear memory limitation exceeded", અંક_Page_Bytes > d_m3GuardedDataBytes);

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

        જો (memory->guardSlot == ખાલી) {
            memory->guardSlot = Guard_TakeSlot();
            _throwifnull(memory->guardSlot);
        }

        // Growing only ever commits more of the same slot, so the memory never
        // moves - which the interpreter does not rely on, but nothing minds
        _throwif(m3Err_mallocFailed, not Guard_CommitSlot(memory->guardSlot, (size_t)અંક_Page_Bytes));

        memory->mallocated = Guard_SlotHeader(memory->guardSlot);

#નહીંતર

        size_t અંક_Bytes = (size_t)અંક_Page_Bytes + કદ(M3MemoryHeader);

        // What is really there, which is not numPages * pageSize: the limit above
        // caps the bytes without capping the page count, so a memory can be short
        // of a page - by design, since a whole 64 KiB page is often more than an
        // MCU has. Measuring the old block by the page count would overstate it,
        // and then realloc never sees the size it already has and copies the
        // whole limit again on every grow past it.
        size_t અંક_Previous_Bytes = memory->mallocated ? memory->mallocated->length + કદ(M3MemoryHeader) : 0;

        શૂન્ય* newMem = m3_Realloc("Wasm Linear Memory", memory->mallocated, અંક_Bytes, અંક_Previous_Bytes);
        _throwifnull(newMem);

        memory->mallocated = (M3MemoryHeader*)newMem;

#endif

#જો d_m3LogRuntime
        M3MemoryHeader* oldMallocated = memory->mallocated;
#endif

        io_runtime->memoryBytesUsed = otherUsed + અંક_Page_Bytes;
        memory->અંક_Pages            = અંક_Pages_To_Alloc;

        memory->mallocated->length  = (size_t)અંક_Page_Bytes;
        memory->mallocated->runtime = io_runtime;
        memory->mallocated->memory  = memory;

        memory->mallocated->max_ઢગલો = (m3slot_t*)io_runtime->origin_ઢગલો + io_runtime->અંક_ઢગલો_Slots;

        m3log(runtime, "resized old: %p; mem: %p; length: %zu; pages: %llu", oldMallocated, memory->mallocated, memory->mallocated->length, (unsigned long long)memory->અંક_Pages);
    } નહીંતર {
        result = m3Err_wasmMemoryOverflow;
    }

    _catch: પાછા result;
}


શૂન્ય FreeMemoryBlock (IM3Memory io_memory)
{
    જો (io_memory->mallocated and io_memory->mallocated->runtime) {
        io_memory->mallocated->runtime->memoryBytesUsed -= io_memory->mallocated->length;
    }
#જો d_m3GuardedMemory
    Guard_GiveSlot(io_memory->guardSlot);
    io_memory->guardSlot = ખાલી;

    io_memory->mallocated = ખાલી;
#નહીંતર
    m3_Free(io_memory->mallocated);
#endif
}


M3Result InitGlobals (IM3Module io_module)
{
    M3Result result = m3Err_none;

    જો (io_module->અંક_Globals) {
        // placing the globals in their structs isn't good for cache locality, but i don't really know what the global
        // access patterns typically look like yet.

        //          io_module->globalMemory = m3Alloc (m3reg_t, io_module->numGlobals);

        //          if (io_module->globalMemory)
        {
            માટે (u32 i = 0; i < io_module->અંક_Globals; ++i) {
                M3Global* g = &io_module->globals[i];                           m3log (runtime, "initializing global: %d", i);

                જો (g->init_પદાવલિ) {
                    bytes_t start = g->init_પદાવલિ;

                    result = EvaluateExpression(io_module, &g->i64_કિંમત, g->type, &start, g->init_પદાવલિ + g->init_પદાવલિ_Size);

                    જો (not result) {
                        // io_module->globalMemory [i] = initValue;
                    } નહીંતર {
                        તોડો;
                    }
                } નહીંતર {                                                        m3log (runtime, "importing global");
                }
            }
        }
        //          else result = ErrorModule (m3Err_mallocFailed, io_module, "could allocate globals for module: '%s", io_module->name);
    }

    પાછા result;
}


M3Result InitDataSegments (IM3Module io_module)
{
    M3Result result = m3Err_none;

    માટે (u32 i = 0; i < io_module->અંક_Data_Segments; ++i) {
        M3DataSegment* segment = &io_module->dataSegments[i];

        // A passive segment stays available for memory.init until data.drop.
        // An active one is copied here and then counts as dropped.
        જો (segment->isPassive) {
            ચાલુ;
        }

        _throwif("data segment memory index out of range",
                 segment->memoryRegion >= io_module->અંક_Memories);

        IM3Memory io_memory = io_module->memories[segment->memoryRegion];

        _throwif("unallocated linear memory", !(io_memory->mallocated));

        // The offset expression has the memory's address type, and is
        // unsigned: an i32 offset of -1 is 4294967295, way out of bounds
        // rather than negative.
        u64     segmentOffset = 0;
        bytes_t start         = segment->init_પદાવલિ;

        જો (io_memory->isMemory64) {
_           (EvaluateExpression(io_module, &segmentOffset, c_m3Type_i64, &start, segment->init_પદાવલિ + segment->init_પદાવલિ_Size));
        } નહીંતર {
            u32 offset32;
_           (EvaluateExpression(io_module, &offset32, c_m3Type_i32, &start, segment->init_પદાવલિ + segment->init_પદાવલિ_Size));
            segmentOffset = offset32;
        }

        m3log(runtime, "loading data segment: %d; size: %d; offset: %llu", i, segment->size, (unsigned long long)segmentOffset);

        જો (segmentOffset <= io_memory->mallocated->length &&
            (u64)segment->size <= io_memory->mallocated->length - segmentOffset) {
            u8* dest = m3MemData(io_memory->mallocated) + segmentOffset;
            memcpy(dest, segment->data, segment->size);
        } નહીંતર {
            _throw("data segment out of bounds");
        }

        segment->dropped = true;
    }

    _catch: પાછા result;
}


// Turns a segment's elements into references. Element expressions are constant
// expressions restricted to ref.null/ref.func, so they're read directly rather
// than run through the compiler.
સ્થિર
M3Result ResolveElements (IM3Module io_module, M3ElementSegment* i_segment, શૂન્ય** o_elements)
{
    M3Result result = m3Err_none;

    bytes_t  pos = i_segment->elements;
    cbytes_t end = io_module->elementSectionEnd;

    માટે (u32 e = 0; e < i_segment->અંક_Elements; ++e) {
        u32   funcIndex;
        શૂન્ય* ref = ખાલી;

        જો (i_segment->is_પદાવલિ) {
            m3opcode_t opcode;
_           (Read_opcode(&opcode, &pos, end));

            જો (opcode == c_waOp_refFunc) {
_               (ReadLEB_u32(&funcIndex, &pos, end));
                _throwif("function index out of range", funcIndex >= io_module->અંક_Functions);
                ref = Function_Implementation(&io_module->functions[funcIndex]);
            } નહીંતર જો (opcode == c_waOp_refNull) {
                i8 waType;
                u8 nullType;
_               (ReadLEB_i7(&waType, &pos, end));
_               (NormalizeType(&nullType, waType));
                _throwif(m3Err_typeMismatch, nullType != i_segment->type);
            } નહીંતર જો (opcode == c_waOp_getGlobal) {
                // wasm 2.0 lets an element expression read an imported
                // immutable global, which is how one module seeds another's
                // table with a reference it exported.
                u32 globalIndex;
_               (ReadLEB_u32(&globalIndex, &pos, end));
                _throwif(m3Err_globaIndexOutOfBounds, globalIndex >= io_module->અંક_Globals);

                IM3Global global = &io_module->globals[globalIndex];

                _throwif(m3Err_globaIndexOutOfBounds, not global->imported);
                _throwif(m3Err_wasmMalformed, global->isMutable);
                _throwif(m3Err_typeMismatch, BaseTypeOf(global->type) != BaseTypeOf(i_segment->type));

                // read the cell the import was linked to, not the placeholder
                જો (global->resolved) {
                    global = global->resolved;
                }

                ref = global->ref_કિંમત;
            } નહીંતર {
                _throw("constant expression required");
            }

_           (Read_opcode(&opcode, &pos, end));
            _throwif(m3Err_wasmMalformed, opcode != c_waOp_end);
        } નહીંતર {
_           (ReadLEB_u32(&funcIndex, &pos, end));
            _throwif("function index out of range", funcIndex >= io_module->અંક_Functions);
            ref = Function_Implementation(&io_module->functions[funcIndex]);
        }

        o_elements[e] = ref;
    }

    _catch: પાછા result;
}


M3Result InitTableAndElements (IM3Module io_module)
{
    M3Result result = m3Err_none;

    cbytes_t end = io_module->elementSectionEnd;
    M3Table* table;

    માટે (u32 i = 0; i < io_module->અંક_Tables; ++i) {
        table = io_module->tables[i];

        // a slot pointing at another module's table is that module's to fill
        જો (table->owner != io_module) {
            ચાલુ;
        }

        જો (table->size) {
            IM3Runtime runtime = io_module->runtime;
            _throwif(m3Err_tableLimitExceeded, runtime->tableElementsLimit and
                                                 table->size > runtime->tableElementsLimit - runtime->tableElementsUsed);
            table->elements = m3_AllocArray(શૂન્ય*, table->size);
            _throwifnull(table->elements);
            runtime->tableElementsUsed += table->size;

            જો (table->init_પદાવલિ) {
                શૂન્ય*   કિંમત = ખાલી;
                bytes_t start = table->init_પદાવલિ;
_               (EvaluateExpression(io_module, &કિંમત, BaseTypeOf(table->type),
                                     &start, table->init_પદાવલિ + table->init_પદાવલિ_Size));

                માટે (u32 e = 0; e < table->size; ++e) {
                    table->elements[e] = કિંમત;
                }
            }
        }
    }

    માટે (u32 i = 0; i < io_module->અંક_Element_Segments; ++i) {
        M3ElementSegment* segment = &io_module->elementSegments[i];

        // Declarative segments only make their functions referenceable, and
        // passive ones wait for table.init, so neither is written out here.
        જો (segment->mode == c_m3Elem_declarative) {
            segment->dropped = true;
            ચાલુ;
        }

        જો (segment->mode == c_m3Elem_passive) {
            જો (segment->અંક_Elements) {
                segment->resolved = m3_AllocArray(શૂન્ય*, segment->અંક_Elements);
                _throwifnull(segment->resolved);
_               (ResolveElements(io_module, segment, segment->resolved));
            }
            ચાલુ;
        }

        table = io_module->tables[segment->tableIndex];

        // The offset expression has the table's index type, and is unsigned:
        // an i32 offset of -1 is 4294967295, out of bounds rather than negative.
        u64     offset = 0;
        bytes_t પદાવલિ   = segment->init_પદાવલિ;

        જો (table->isTable64) {
_           (EvaluateExpression(io_module, &offset, c_m3Type_i64, &પદાવલિ, end));
        } નહીંતર {
            u32 offset32;
_           (EvaluateExpression(io_module, &offset32, c_m3Type_i32, &પદાવલિ, end));
            offset = offset32;
        }

        _throwif("out of bounds table access",
                 offset > table->size or segment->અંક_Elements > table->size - offset);

_       (ResolveElements(io_module, segment, table->elements + offset));

        segment->dropped = true;
    }

    _catch: પાછા result;
}

M3Result m3_CompileModule (IM3Module io_module)
{
    M3Result result = m3Err_none;

    માટે (u32 i = 0; i < io_module->અંક_Functions; ++i) {
        IM3Function f = &io_module->functions[i];
        જો (f->wasm and not f->compiled) {
_           (CompileFunction(f));
        }
    }

    _catch: પાછા result;
}

#જો d_m3HasExceptionHandling

M3Exception* NewException (IM3Runtime io_runtime, IM3Tag i_tag, u32 i_અંક_Args)
{
    M3Exception* exception = (M3Exception*)m3_Malloc("M3Exception", કદ(M3Exception) + i_અંક_Args * કદ(u64));

    જો (exception) {
        exception->tag     = i_tag;
        exception->અંક_Args = i_અંક_Args;
        exception->reified = false;
        exception->prev    = ખાલી;
        exception->next    = io_runtime->exceptions;

        જો (exception->next) {
            exception->next->prev = exception;
        }

        io_runtime->exceptions = exception;
    }

    પાછા exception;
}


// Releases one exception ahead of the rest. The caller has to know nothing can
// still name it: no exnref was ever taken of it, and its payload has already
// been copied out.
શૂન્ય FreeException (IM3Runtime io_runtime, M3Exception* i_exception)
{
    જો (i_exception->prev) {
        i_exception->prev->next = i_exception->next;
    } નહીંતર {
        io_runtime->exceptions = i_exception->next;
    }

    જો (i_exception->next) {
        i_exception->next->prev = i_exception->prev;
    }

    જો (io_runtime->pendingException == i_exception) {
        io_runtime->pendingException = ખાલી;
    }

    m3_Free_Impl(i_exception);
}


// Releases every exception the runtime still holds. Only safe once the Wasm
// stack is empty, which is why the outermost RunCodeChecked() is the one that
// calls it.
શૂન્ય FreeExceptions (IM3Runtime io_runtime)
{
    M3Exception* exception = io_runtime->exceptions;

    io_runtime->exceptions       = ખાલી;
    io_runtime->pendingException = ખાલી;

    જ્યારે (exception) {
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
સ્થિર
શૂન્ય RecordUncaughtException (IM3Runtime io_runtime, M3Result i_result)
{
    જો (io_runtime->uncaughtException) {
        m3_Free(io_runtime->uncaughtException);
    }

    M3Exception* exception = io_runtime->pendingException;

    જો (M3_UNLIKELY(i_result == m3Err_trapUncaughtException) and exception) {
        M3Exception* copy = (M3Exception*)m3_CopyMem(exception, કદ(M3Exception) + exception->અંક_Args * કદ(u64));

        જો (copy) {
            copy->next = ખાલી;
            copy->prev = ખાલી;
        }

        io_runtime->uncaughtException = copy;
    }
}

#endif // d_m3HasExceptionHandling


#જો d_m3HasExceptionHandling || d_m3_Has_ઢગલો_Switching

IM3Tag m3_FindTag (IM3Module i_module, અચલ અક્ષર* અચલ i_tagName)
{
    જો (not i_module or not i_tagName) {
        પાછા ખાલી;
    }

    પાછા Module_FindExportedTag(i_module, i_tagName);
}


M3Result m3_NewTag (IM3Runtime io_runtime, IM3Tag* o_tag, અચલ અક્ષર* અચલ i_signature)
{
    IM3FuncType type    = ખાલી;
    M3HostTag*  hostTag = ખાલી;

_try {
    _throwif("null tag", not o_tag);
    *o_tag = ખાલી;
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
    પાછા result;
}


M3Result m3_LinkTag (IM3Module         io_module,
                     અચલ અક્ષર* અચલ i_moduleName,
                     અચલ અક્ષર* અચલ i_tagName,
                     IM3Tag            i_tag)
{
    જો (not io_module or not i_moduleName or not i_tagName or not i_tag) {
        પાછા m3Err_tagLookupFailed;
    }

    અચલ bool wildcardModule = (strcmp(i_moduleName, "*") == 0);
    IM3Tag     target         = i_tag->resolved ? i_tag->resolved : i_tag;

    M3Result result = m3Err_tagLookupFailed;

    માટે (u32 i = 0; i < io_module->અંક_Tags; ++i) {
        IM3Tag tag = &io_module->tags[i];

        જો (tag->imported and tag->import.moduleUtf8 and tag->import.fieldUtf8 and
            strcmp(tag->import.fieldUtf8, i_tagName) == 0 and
            (wildcardModule or strcmp(tag->import.moduleUtf8, i_moduleName) == 0)) {
            જો (tag->type != target->type) {
                પાછા m3Err_incompatibleImportType;
            }

            tag->resolved = target;
            result        = m3Err_none;
        }
    }

    પાછા result;
}


uint32_t m3_Get_Tag_Arg_ગણતરી (IM3Tag i_tag)
{
    પાછા i_tag ? i_tag->type->અંક_Args : 0;
}


M3_કિંમત_Type m3_GetTagArgType (IM3Tag i_tag, uint32_t i_index)
{
    જો (i_tag and i_index < i_tag->type->અંક_Args) {
        પાછા (M3_કિંમત_Type)BaseTypeOf(d_FuncArgType(i_tag->type, i_index));
    }

    પાછા c_m3Type_none;
}

#નહીંતર

IM3Tag m3_FindTag (IM3Module i_module, અચલ અક્ષર* અચલ i_tagName)
{
    પાછા ખાલી;
}

M3Result m3_NewTag (IM3Runtime io_runtime, IM3Tag* o_tag, અચલ અક્ષર* અચલ i_signature)
{
    જો (o_tag) {
        *o_tag = ખાલી;
    }
    પાછા m3Err_tagLookupFailed;
}

M3Result m3_LinkTag (IM3Module io_module, અચલ અક્ષર* અચલ i_moduleName, અચલ અક્ષર* અચલ i_tagName, IM3Tag i_tag)
{
    પાછા m3Err_tagLookupFailed;
}

uint32_t m3_Get_Tag_Arg_ગણતરી (IM3Tag i_tag)
{
    પાછા 0;
}

M3_કિંમત_Type m3_GetTagArgType (IM3Tag i_tag, uint32_t i_index)
{
    પાછા c_m3Type_none;
}

#endif // d_m3HasExceptionHandling || d_m3HasStackSwitching


#જો d_m3HasExceptionHandling

M3Result m3_ThrowException (IM3Runtime io_runtime, IM3Tag i_tag, uint32_t i_argc, અચલ શૂન્ય* i_argptrs[])
{
    જો (not io_runtime or not i_tag) {
        પાછા m3Err_tagLookupFailed;
    }

    IM3Tag      tag  = i_tag->resolved ? i_tag->resolved : i_tag;
    IM3FuncType type = tag->type;

    // a tag with results is a stack switching control tag, which is suspended
    // with rather than thrown
    જો (type->અંક_Rets) {
        પાછા m3Err_typeMismatch;
    }
    જો (i_argc != type->અંક_Args) {
        પાછા m3_Err_argument_ગણતરી_Mismatch;
    }
    // with nothing on the stack there is nothing to unwind, and the exception
    // would sit on the runtime's list until some later call ended
    જો (io_runtime->callNesting == 0) {
        પાછા "m3_ThrowException called outside a host function";
    }

    M3Exception* exception = NewException(io_runtime, tag, i_argc);
    જો (not exception) {
        પાછા m3Err_mallocFailed;
    }

    // the layout op_Throw gives a payload: 32-bit values zero-extended, floats
    // as their bit pattern
    માટે (u32 i = 0; i < i_argc; ++i) {
        અચલ શૂન્ય* કિંમત = i_argptrs[i];

        પસંદગી (BaseTypeOf(d_FuncArgType(type, i))) {
        સ્થિતિ c_m3Type_i32:
        સ્થિતિ c_m3Type_f32: {
            u32 bits;
            memcpy(&bits, કિંમત, કદ(bits));
            exception->args[i] = bits;
            તોડો;
        }
        સ્થિતિ c_m3Type_i64:
        સ્થિતિ c_m3Type_f64:
            memcpy(&exception->args[i], કિંમત, કદ(u64));
            તોડો;
        સ્થિતિ c_m3Type_funcref:
        સ્થિતિ c_m3Type_externref:
        સ્થિતિ c_m3Type_exnref:
        સ્થિતિ c_m3Type_contref:
            exception->args[i] = (u64)(*(અચલ uintptr_t*)કિંમત);
            તોડો;
        મૂળભૂત:
            FreeException(io_runtime, exception);
            પાછા "unknown argument type";
        }
    }

    io_runtime->pendingException = exception;

    પાછા m3Err_pendingException;
}


IM3Tag m3_GetExceptionTag (IM3Runtime i_runtime)
{
    જો (i_runtime and i_runtime->uncaughtException) {
        પાછા i_runtime->uncaughtException->tag;
    }

    પાછા ખાલી;
}


M3Result m3_GetExceptionArgs (IM3Runtime i_runtime, uint32_t i_argc, અચલ શૂન્ય* o_argptrs[])
{
    જો (not i_runtime or not i_runtime->uncaughtException) {
        પાછા "no uncaught exception";
    }

    અચલ M3Exception* exception = i_runtime->uncaughtException;
    IM3FuncType        type      = exception->tag->type;

    જો (i_argc != exception->અંક_Args) {
        પાછા m3_Err_argument_ગણતરી_Mismatch;
    }

    માટે (u32 i = 0; i < i_argc; ++i) {
        શૂન્ય* કિંમત = (શૂન્ય*)o_argptrs[i];
        u64   bits  = exception->args[i];

        પસંદગી (BaseTypeOf(d_FuncArgType(type, i))) {
        સ્થિતિ c_m3Type_i32:
        સ્થિતિ c_m3Type_f32: {
            u32 નીચલું = (u32)bits;
            memcpy(કિંમત, &નીચલું, કદ(નીચલું));
            તોડો;
        }
        સ્થિતિ c_m3Type_i64:
        સ્થિતિ c_m3Type_f64:
            memcpy(કિંમત, &bits, કદ(bits));
            તોડો;
        સ્થિતિ c_m3Type_funcref:
        સ્થિતિ c_m3Type_externref:
        સ્થિતિ c_m3Type_exnref:
        સ્થિતિ c_m3Type_contref:
            *(uintptr_t*)કિંમત = (uintptr_t)bits;
            તોડો;
        મૂળભૂત:
            પાછા "unknown argument type";
        }
    }

    પાછા m3Err_none;
}

#નહીંતર

M3Result m3_ThrowException (IM3Runtime io_runtime, IM3Tag i_tag, uint32_t i_argc, અચલ શૂન્ય* i_argptrs[])
{
    પાછા m3Err_tagLookupFailed;
}

IM3Tag m3_GetExceptionTag (IM3Runtime i_runtime)
{
    પાછા ખાલી;
}

M3Result m3_GetExceptionArgs (IM3Runtime i_runtime, uint32_t i_argc, અચલ શૂન્ય* o_argptrs[])
{
    પાછા "no uncaught exception";
}

#endif // d_m3HasExceptionHandling


#જો d_m3GuardedMemory

// What RunCodeProtected hands to the platform layer to run, and what comes back
પ્રકારવ્યાખ્યા સંરચના M3ProtectedRun {
    pc_t            pc;
    IM3Runtime      runtime;
    M3MemoryHeader* mem;
    M3Result        result;
} M3ProtectedRun;

સ્થિર
શૂન્ય RunCodeBody (શૂન્ય* io_context)
{
    M3ProtectedRun* run = (M3ProtectedRun*)io_context;

#  જો (d_m3EnableOpProfiling || d_m3EnableOpTracing)
    run->result = (M3Result)RunCode(run->pc, (m3stack_t)run->runtime->ઢગલો, run->mem,
                                    d_m3OpDefaultArgs, d_m3BaseCstr);
#  નહીંતર
    run->result = (M3Result)RunCode(run->pc, (m3stack_t)run->runtime->ઢગલો, run->mem,
                                    d_m3OpDefaultArgs);
#  endif
}

// The bounds check the load and store operations no longer do, done once here by the
// system instead: an address past the end of a memory reaches the uncommitted part
// of that memory's reservation, and the fault comes back as the trap the spec asks
// for. Nothing is left half-done by it - the Wasm stack and the memories are heap,
// and the caller unwinds the rest.
સ્થિર
M3Result RunCodeProtected (IM3Runtime i_runtime, pc_t i_pc, M3MemoryHeader* i_mem)
{
    // Memory operations in this build carry no bounds check, so code only runs where a
    // fault past the end of a memory can still be turned into a trap
    જો (M3_UNLIKELY(not m3_HostGuardsActive())) {
        પાછા ErrorRuntime(m3Err_memoryGuardsUnavailable, i_runtime,
                            "another part of the process holds the fault signal");
    }

    M3ProtectedRun run;
    run.pc      = i_pc;
    run.runtime = i_runtime;
    run.mem     = i_mem;
    run.result  = m3Err_none;

    શૂન્ય*  arena;
    size_t arenaBytes;
    Guard_ArenaRange(&arena, &arenaBytes);

    જો (not m3_HostProtectedCall(RunCodeBody, &run, arena, arenaBytes)) {
        પાછા m3Err_trapOutOfBoundsMemoryAccess;
    }

    પાછા run.result;
}

#endif // d_m3GuardedMemory


#જો d_m3_Has_ઢગલો_Switching

// The function a suspended continuation stopped in: the callee of its innermost
// call frame, or the function it was entered with if it made no call. Frames are
// recorded innermost first, so that is the lowest-numbered call frame.
સ્થિર
IM3Function Continuation_InnermostFunction (IM3Continuation i_cont)
{
    માટે (u32 i = 0; i < i_cont->અંક_Frames; ++i) {
        જો (i_cont->frames[i].kind == frame_call && i_cont->frames[i].call.function) {
            પાછા i_cont->frames[i].call.function;
        }
    }

    પાછા i_cont->entryFunction;
}


// Where the host's next call puts its arguments and finds its results. A
// suspended root keeps its frames on the bottom of the runtime's stack, so a call
// made before it is resumed starts above the last of them rather than on top of
// them. The top is the innermost function's frame base plus everything that
// function can use. That base is the root's own sp, unless it stopped inside a
// resume: then the suspension point belongs to the resumed continuation, and the
// root's innermost frame is that resume.
શૂન્ય Runtime_Place_Call_ઢગલો (IM3Runtime io_runtime)
{
    IM3Continuation મૂળ = io_runtime->મૂળ_Continuation;

    io_runtime->ઢગલો = io_runtime->origin_ઢગલો;

    જો (not મૂળ or મૂળ->state != cont_suspended) {
        પાછા;
    }

    m3stack_t   sp       = મૂળ->sp;
    IM3Function function = Continuation_InnermostFunction(મૂળ);

    જો (મૂળ->અંક_Frames and મૂળ->frames[0].kind == frame_resume) {
        sp = મૂળ->frames[0].sp;
    }

    m3slot_t* base = (m3slot_t*)io_runtime->origin_ઢગલો;
    m3slot_t* ટોચ  = (m3slot_t*)sp + (function ? function->max_ઢગલો_Slots : 0);

    જો (ટોચ > base + io_runtime->અંક_ઢગલો_Slots) {
        ટોચ = base + io_runtime->અંક_ઢગલો_Slots;
    }

    io_runtime->ઢગલો = ટોચ;
}

#endif // d_m3HasStackSwitching


// Run compiled code on the runtime's stack, bounding native recursion for the
// duration of the call. The outermost invocation establishes the stack limit;
// nested ones (an imported function calling back into Wasm) inherit it.
સ્થિર અંતઃરેખ
M3Result RunCodeChecked (IM3Runtime i_runtime, IM3Function i_function)
{
    pc_t i_pc = i_function->compiled;

    // execution runs against the memory of the module the entry point belongs
    // to, not against some runtime-wide one
    M3MemoryHeader* _mem = Module_MemoryHeader(i_function->module);

#જો d_m3HasSnapshots
    જો (M3_UNLIKELY(i_function->module->isUnusable)) {
        પાછા "a snapshot failed to restore into the module, so it cannot run";
    }
    i_function->module->hasRun = true;
#endif

    d_m3_ઢગલો_Limit_Enter(i_runtime);

    // a host function calling back into Wasm does not change which call a trap
    // ends: that is still the one the host made first
    જો (i_runtime->callNesting++ == 0) {
        i_runtime->entered = i_function;
    }
#જો d_m3HasExceptionHandling
    // handler stacks don't nest across a call boundary: a host function calling
    // back into Wasm cannot be caught by a try_table its own caller entered
    u32 savedTryDepth   = i_runtime->tryDepth;
    i_runtime->tryDepth = 0;
#endif
#જો d_m3_Has_ઢગલો_Switching
    // nor do continuations: the native frames on the far side of a host call
    // are not ours to record, so a suspend in here has no handler to reach and
    // no resume to unwind to
    IM3Continuation savedContinuation = i_runtime->activeContinuation;

    // A suspended root continuation is a paused program waiting to be resumed,
    // not scratch space. A call the host makes before resuming it gets a
    // context of its own, the way any other re-entry does, rather than having
    // the frames of the suspension reset out from under it.
    bool claim_મૂળ = i_runtime->isSuspendable and not savedContinuation and
                     not (i_runtime->મૂળ_Continuation and
                          i_runtime->મૂળ_Continuation->state == cont_suspended);

    જો (claim_મૂળ and not i_runtime->મૂળ_Continuation) {
        i_runtime->મૂળ_Continuation = Continuation_New(i_runtime, i_function->funcType, i_function);

        જો (i_runtime->મૂળ_Continuation) {
            // The root borrows the runtime's stack and is not charged as cont.new.
            i_runtime->મૂળ_Continuation->અંક_ઢગલો_Slots = i_runtime->અંક_ઢગલો_Slots;
        } નહીંતર {
            claim_મૂળ = false;
        }
    }

    જો (claim_મૂળ) {
        i_runtime->activeContinuation              = i_runtime->મૂળ_Continuation;
        i_runtime->મૂળ_Continuation->entryFunction = i_function;
        i_runtime->મૂળ_Continuation->state         = cont_running;
        i_runtime->મૂળ_Continuation->અંક_Frames     = 0;
    } નહીંતર {
        i_runtime->activeContinuation = ખાલી;
    }
#endif
#જો d_m3GuardedMemory
    M3Result result = RunCodeProtected(i_runtime, i_pc, _mem);
#અથવાજો (d_m3EnableOpProfiling || d_m3EnableOpTracing)
    M3Result result = (M3Result)RunCode(i_pc, (m3stack_t)i_runtime->ઢગલો, _mem, d_m3OpDefaultArgs, d_m3BaseCstr);
#નહીંતર
    M3Result result = (M3Result)RunCode(i_pc, (m3stack_t)i_runtime->ઢગલો, _mem, d_m3OpDefaultArgs);
#endif
#જો d_m3_Has_ઢગલો_Switching
    જો (claim_મૂળ) {
        i_runtime->મૂળ_Continuation->state =
          (result == m3Err_continuationSuspended) ? cont_suspended : cont_returned;

        Runtime_Place_Call_ઢગલો(i_runtime);
    }
    i_runtime->activeContinuation = savedContinuation;
#endif
#જો d_m3HasExceptionHandling
    i_runtime->tryDepth = savedTryDepth;

    // an exception that reached the bottom of the call stack found no handler
    જો (M3_UNLIKELY(result == m3Err_pendingException)) {
        result = m3Err_trapUncaughtException;
    }
    RecordUncaughtException(i_runtime, result);

    // a paused invocation can still reach the exceptions it holds, so they wait
    // for it to finish
    જો (i_runtime->callNesting == 1 and not m3_IsSuspended(i_runtime)) {
        FreeExceptions(i_runtime);
    }
#endif
    i_runtime->callNesting--;
    d_m3_ઢગલો_Limit_Leave(i_runtime);

    પાછા result;
}

M3Result m3_RunStart (IM3Module io_module)
{
#ifdef FUZZING_BUILD_MODE_UNSAFE_FOR_PRODUCTION
    // Execution disabled for fuzzing builds
    પાછા m3Err_none;
#endif

    M3Result result           = m3Err_none;
    i32      startFunctionTmp = -1;

    જો (io_module and io_module->startFunction >= 0) {
        IM3Function function = &io_module->functions[io_module->startFunction];

        જો (not function->compiled) {
_           (CompileFunction(function));
        }

        IM3FuncType ftype = function->funcType;
        જો (ftype->અંક_Args != 0 || ftype->અંક_Rets != 0) {
            _throw(m3_Err_argument_ગણતરી_Mismatch);
        }

        IM3Module  module  = function->module;
        IM3Runtime runtime = module->runtime;

        startFunctionTmp = io_module->startFunction;

        io_module->startFunction = -1;

        result = RunCodeChecked(runtime, function);

        // a start function that suspended is not to be started again: resuming
        // is what finishes it
        જો (result and result != m3Err_continuationSuspended) {
            io_module->startFunction = startFunctionTmp;
            EXCEPTION_PRINT(result);
            જાઓ _catch;
        }
    }

    _catch: પાછા result;
}

// TODO: deal with main + side-modules loading efforcement
M3Result m3_LoadModule (IM3Runtime io_runtime, IM3Module io_module)
{
    M3Result result = m3Err_none;

    જો (M3_UNLIKELY(io_module->runtime)) {
        પાછા m3Err_moduleAlreadyLinked;
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
    પાછા result; // ok

_catch:
    // The runtime owns the module either way. Instantiation may already have
    // written this module's functions into a table another module owns, and the
    // spec keeps whatever it managed to do before the trap, so those entries
    // stay callable - retaining the module is what stops them dangling.
    //
    // It goes on the tail of the list rather than the head: it is not a module
    // anyone should find by name, only one whose functions may still be
    // reachable through someone else's table.
    io_module->next = ખાલી;

    IM3Module* tail = &io_runtime->modules;
    જ્યારે (*tail) {
        tail = &(*tail)->next;
    }
    *tail = io_module;

    પાછા result;
}

IM3Global m3_FindGlobal (IM3Module         io_module,
                         અચલ અક્ષર* અચલ i_globalName)
{
    // Search exports
    IM3Global exported = Module_FindExportedGlobal(io_module, i_globalName);
    જો (exported) {
        // a re-exported global is the one that was imported, so reads and
        // writes have to reach the cell that actually holds the value
        પાછા exported->resolved ? exported->resolved : exported;
    }

    // Search imports
    માટે (u32 i = 0; i < io_module->અંક_Globals; ++i) {
        IM3Global g = &io_module->globals[i];

        જો (g->import.moduleUtf8 and g->import.fieldUtf8) {
            જો (strcmp(g->import.fieldUtf8, i_globalName) == 0) {
                પાછા g->resolved ? g->resolved : g;
            }
        }
    }
    પાછા ખાલી;
}

M3Result m3_GetGlobal (IM3Global      i_global,
                       IM3_Tagged_કિંમત o_કિંમત)
{
    જો (not i_global) {
        પાછા m3Err_globalLookupFailed;
    }

    પસંદગી (i_global->type) {
    સ્થિતિ c_m3Type_i32: o_કિંમત->કિંમત.i32 = i_global->i32_કિંમત; તોડો;
    સ્થિતિ c_m3Type_i64: o_કિંમત->કિંમત.i64 = i_global->i64_કિંમત; તોડો;
#જો d_m3HasFloat
    સ્થિતિ c_m3Type_f32: o_કિંમત->કિંમત.f32 = i_global->f32_કિંમત; તોડો;
    સ્થિતિ c_m3Type_f64: o_કિંમત->કિંમત.f64 = i_global->f64_કિંમત; તોડો;
#endif
    મૂળભૂત: પાછા m3Err_invalidTypeId;
    }

    o_કિંમત->type = (M3_કિંમત_Type)(i_global->type);
    પાછા m3Err_none;
}

M3Result m3_SetGlobal (IM3Global            i_global,
                       અચલ IM3_Tagged_કિંમત i_કિંમત)
{
    જો (not i_global) {
        પાછા m3Err_globalLookupFailed;
    }
    જો (not i_global->isMutable) {
        પાછા m3Err_globalNotMutable;
    }
    જો (i_global->type != i_કિંમત->type) {
        પાછા m3Err_globalTypeMismatch;
    }

    પસંદગી (i_કિંમત->type) {
    સ્થિતિ c_m3Type_i32: i_global->i32_કિંમત = i_કિંમત->કિંમત.i32; તોડો;
    સ્થિતિ c_m3Type_i64: i_global->i64_કિંમત = i_કિંમત->કિંમત.i64; તોડો;
#જો d_m3HasFloat
    સ્થિતિ c_m3Type_f32: i_global->f32_કિંમત = i_કિંમત->કિંમત.f32; તોડો;
    સ્થિતિ c_m3Type_f64: i_global->f64_કિંમત = i_કિંમત->કિંમત.f64; તોડો;
#endif
    મૂળભૂત: પાછા m3Err_invalidTypeId;
    }

    પાછા m3Err_none;
}

M3_કિંમત_Type m3_GetGlobalType (IM3Global i_global)
{
    પાછા (i_global) ? (M3_કિંમત_Type)(i_global->type) : c_m3Type_none;
}


શૂન્ય* v_FindFunction (IM3Module i_module, શૂન્ય* i_info)
{
    અચલ અક્ષર* અચલ i_name = (અચલ અક્ષર*)i_info;

    // Prefer exported functions
    IM3Function exported = Module_FindExportedFunction(i_module, i_name);
    જો (exported) {
        પાછા exported;
    }

    // Search internal functions
    માટે (u32 i = 0; i < i_module->અંક_Functions; ++i) {
        IM3Function f = &i_module->functions[i];

        bool isImported = f->import.moduleUtf8 or f->import.fieldUtf8;

        જો (isImported) {
            ચાલુ;
        }

        માટે (પૂર્ણાંક j = 0; j < f->અંક_Names; j++) {
            જો (f->names[j] and strcmp(f->names[j], i_name) == 0) {
                પાછા f;
            }
        }
    }

    પાછા ખાલી;
}


// Shared tail of the two lookups: a function is only usable once it has code.
સ્થિર
M3Result PrepareFoundFunction (IM3Function* o_function, IM3Function i_function)
{
    M3Result result = m3Err_none;

    જો (not i_function->compiled) {
_       (CompileFunction(i_function))
    }

    _catch:
    *o_function = result ? ખાલી : i_function;

    પાછા result;
}


// Searches every module in the runtime, most recently loaded first. That is a
// guess once more than one module is loaded and two of them export the same
// name - m3_FindFunctionIn says which module is meant.
M3Result m3_FindFunction (IM3Function* o_function, IM3Runtime i_runtime, અચલ અક્ષર* અચલ i_functionName)
{
                                                                d_m3Assert (o_function and i_runtime and i_functionName);
    IM3Function function = ખાલી;

    જો (not i_runtime->modules) {
        *o_function = ખાલી;
        પાછા "no modules loaded";
    }

    function = (IM3Function)ForEachModule(i_runtime, v_FindFunction, (શૂન્ય*)i_functionName);

    જો (not function) {
        *o_function = ખાલી;
        પાછા ErrorModule(m3Err_functionLookupFailed, i_runtime->modules, "'%s'", i_functionName);
    }

    પાછા PrepareFoundFunction(o_function, function);
}


IM3Module m3_FindModule (IM3Runtime i_runtime, અચલ અક્ષર* અચલ i_moduleName)
{
    જો (not i_runtime or not i_moduleName) {
        પાછા ખાલી;
    }

    // the list is newest-first, so a name registered twice names the newer one
    માટે (IM3Module m = i_runtime->modules; m; m = m->next) {
        જો (m->name and strcmp(m->name, i_moduleName) == 0) {
            પાછા m;
        }
    }

    પાછા ખાલી;
}


// Searches one module's exports, which is what naming a module means.
M3Result m3_FindFunctionIn (IM3Function* o_function, IM3Module i_module, અચલ અક્ષર* અચલ i_functionName)
{
                                                                d_m3Assert (o_function and i_functionName);
    જો (not i_module) {
        *o_function = ખાલી;
        પાછા m3Err_functionLookupFailed;      // ErrorModule would deref it
    }

    IM3Function function = (IM3Function)v_FindFunction(i_module, (શૂન્ય*)i_functionName);

    જો (not function) {
        *o_function = ખાલી;
        પાછા ErrorModule(m3Err_functionLookupFailed, i_module, "'%s'", i_functionName);
    }

    પાછા PrepareFoundFunction(o_function, function);
}


M3Result m3_GetTableFunction (IM3Function* o_function, IM3Module i_module, uint32_t i_index)
{
_try {
    M3Table*    table;
    IM3Function function;

    _throwif("no table", i_module->અંક_Tables == 0);

    table = i_module->tables[0];
    _throwif("function index out of range", i_index >= table->size);

    function = (IM3Function)table->elements[i_index];

    જો (function) {
        જો (not function->compiled) {
_           (CompileFunction(function))
        }
    }

    *o_function = function;
} _catch:
    પાછા result;
}


સ્થિર
M3Result checkStartFunction (IM3Module i_module)
{
    M3Result result = m3Err_none;                               d_m3Assert(i_module);

    // Check if start function needs to be called
    જો (i_module->startFunction >= 0) {
        result = m3_RunStart(i_module);
    }

    પાછા result;
}

uint32_t m3_Get_Arg_ગણતરી (IM3Function i_function)
{
    જો (i_function) {
        IM3FuncType ft = i_function->funcType;
        જો (ft) {
            પાછા ft->અંક_Args;
        }
    }
    પાછા 0;
}

uint32_t m3_Get_Ret_ગણતરી (IM3Function i_function)
{
    જો (i_function) {
        IM3FuncType ft = i_function->funcType;
        જો (ft) {
            પાછા ft->અંક_Rets;
        }
    }
    પાછા 0;
}


M3_કિંમત_Type m3_GetArgType (IM3Function i_function, uint32_t index)
{
    જો (i_function) {
        IM3FuncType ft = i_function->funcType;
        જો (ft and index < ft->અંક_Args) {
            પાછા (M3_કિંમત_Type)BaseTypeOf(d_FuncArgType(ft, index));
        }
    }
    પાછા c_m3Type_none;
}

M3_કિંમત_Type m3_GetRetType (IM3Function i_function, uint32_t index)
{
    જો (i_function) {
        IM3FuncType ft = i_function->funcType;
        જો (ft and index < ft->અંક_Rets) {
            પાછા (M3_કિંમત_Type)BaseTypeOf(d_FuncRetType(ft, index));
        }
    }
    પાછા c_m3Type_none;
}


// The call APIs marshal their arguments into the runtime stack before any
// compiled code runs, so op_Entry's overflow check comes too late to cover those
// writes - and for a host function or an import linked to another module there
// is no op_Entry of our own to reach at all. Bound the area about to be written
// here instead: results and arguments each take a u64 at this boundary, however
// wide the slot is inside. Every caller goes through this, so none of them can
// be the one that forgets.
સ્થિર
M3Result Get_ઢગલો_Pointer_For_Args (IM3Function i_function, u8** o_ઢગલો)
{
    IM3Runtime  runtime = i_function->module->runtime;
    IM3FuncType ftype   = i_function->funcType;

    // the stack a paused invocation holds is not available to this one
    size_t used      = (size_t)((u8*)runtime->ઢગલો - (u8*)runtime->origin_ઢગલો);
    size_t needed    = ((size_t)ftype->અંક_Rets + ftype->અંક_Args) * કદ(u64);
    size_t available = (size_t)runtime->અંક_ઢગલો_Slots * કદ(m3slot_t) - used;

    જો (needed > available) {
        પાછા m3_Err_trap_ઢગલો_Overflow;
    }

    u64* ઢગલો = (u64*)runtime->ઢગલો;

    ઢગલો += ftype->અંક_Rets;

    *o_ઢગલો = (u8*)ઢગલો;

    પાછા m3Err_none;
}


M3Result m3_CallV (IM3Function i_function, ...)
{
    va_list ap;
    va_start(ap, i_function);
    M3Result r = m3_CallVL(i_function, ap);
    va_end(ap);
    પાછા r;
}

સ્થિર
શૂન્ય Report_Native_ઢગલો_Usage ()
{
#જો d_m3_Log_Native_ઢગલો
    પૂર્ણાંક ઢગલો_Used = m3_ઢગલો_Get_Max();
    fprintf(stderr, "Native stack used: %d\n", ઢગલો_Used);
#endif
}


M3Result m3_CallVL (IM3Function i_function, va_list i_args)
{
    IM3Runtime  runtime = i_function->module->runtime;
    IM3FuncType ftype   = i_function->funcType;
    M3Result    result  = m3Err_none;
    u8*         s       = ખાલી;

    જો (!i_function->compiled) {
        પાછા m3Err_missingCompiledCode;
    }

#જો d_m3RecordBacktraces
    ClearBacktrace(runtime);
#endif

    m3_ઢગલો_Check_Init();

_   (checkStartFunction(i_function->module))

_   (Get_ઢગલો_Pointer_For_Args(i_function, &s))

    માટે (u32 i = 0; i < ftype->અંક_Args; ++i) {
        પસંદગી (d_FuncArgType(ftype, i)) {
        સ્થિતિ c_m3Type_i32:
            *(i32*)(s) = va_arg(i_args, i32);
            s += 8;
            તોડો;
        સ્થિતિ c_m3Type_i64:
            *(i64*)(s) = va_arg(i_args, i64);
            s += 8;
            તોડો;
        સ્થિતિ c_m3Type_funcref:
        સ્થિતિ c_m3Type_externref:
        સ્થિતિ c_m3Type_exnref:
            *(uintptr_t*)(s) = va_arg(i_args, uintptr_t);
            s += 8;
            તોડો;
#જો d_m3HasFloat
        સ્થિતિ c_m3Type_f32:
            *(f32*)(s) = (f32)va_arg(i_args, f64);
            s += 8;
            તોડો; // f32 is passed as f64
        સ્થિતિ c_m3Type_f64:
            *(f64*)(s) = va_arg(i_args, f64);
            s += 8;
            તોડો;
#endif
        મૂળભૂત: પાછા "unknown argument type";
        }
    }

    result = RunCodeChecked(runtime, i_function);
    Report_Native_ઢગલો_Usage();

    runtime->lastCalled = result ? ખાલી : i_function;

    _catch: પાછા result;
}

M3Result m3_Call (IM3Function i_function, uint32_t i_argc, અચલ શૂન્ય* i_argptrs[])
{
    IM3Runtime  runtime = i_function->module->runtime;
    IM3FuncType ftype   = i_function->funcType;
    M3Result    result  = m3Err_none;
    u8*         s       = ખાલી;

    જો (i_argc != ftype->અંક_Args) {
        પાછા m3_Err_argument_ગણતરી_Mismatch;
    }
    જો (!i_function->compiled) {
        પાછા m3Err_missingCompiledCode;
    }

#જો d_m3RecordBacktraces
    ClearBacktrace(runtime);
#endif

    m3_ઢગલો_Check_Init();

_   (checkStartFunction(i_function->module))

_   (Get_ઢગલો_Pointer_For_Args(i_function, &s))

    માટે (u32 i = 0; i < ftype->અંક_Args; ++i) {
        પસંદગી (d_FuncArgType(ftype, i)) {
        સ્થિતિ c_m3Type_i32:
            *(i32*)(s) = *(i32*)i_argptrs[i];
            s += 8;
            તોડો;
        સ્થિતિ c_m3Type_i64:
            *(i64*)(s) = *(i64*)i_argptrs[i];
            s += 8;
            તોડો;
        સ્થિતિ c_m3Type_funcref:
        સ્થિતિ c_m3Type_externref:
        સ્થિતિ c_m3Type_exnref:
            *(uintptr_t*)(s) = *(uintptr_t*)i_argptrs[i];
            s += 8;
            તોડો;
#જો d_m3HasFloat
        સ્થિતિ c_m3Type_f32:
            *(f32*)(s) = *(f32*)i_argptrs[i];
            s += 8;
            તોડો;
        સ્થિતિ c_m3Type_f64:
            *(f64*)(s) = *(f64*)i_argptrs[i];
            s += 8;
            તોડો;
#endif
        મૂળભૂત: પાછા "unknown argument type";
        }
    }

    result = RunCodeChecked(runtime, i_function);

    Report_Native_ઢગલો_Usage();

    runtime->lastCalled = result ? ખાલી : i_function;

    _catch: પાછા result;
}

// Argument parsing for m3_CallArgv. Strict on purpose: the whole string has to
// be consumed, so "12abc" is rejected rather than read as 12, and an empty or
// unparsable argument is an error rather than the zero that strtoul with a
// NULL end pointer used to hand back. See wasm3/wasm3#367.
સ્થિર
M3Result ParseArgInteger (ccstr_t i_arg, u32 i_અંક_Bits, u64* o_કિંમત)
{
    જો (not i_arg or not *i_arg) {
        પાછા "empty argument";
    }

    // strtoull would skip leading space, but trailing space is rejected below;
    // accepting one and not the other would just be confusing
    જો (isspace((unsigned અક્ષર)*i_arg)) {
        પાછા "argument is not a number";
    }

    અક્ષર* end = ખાલી;
    u64   કિંમત;

    errno = 0;

    // an argument may be spelled signed or unsigned: -1 and 4294967295 name the
    // same i32
    જો (*i_arg == '-') {
        i64 signed_કિંમત = strtoll(i_arg, &end, 10);

        જો (i_અંક_Bits == 32 and (signed_કિંમત < INT32_MIN or signed_કિંમત > INT32_MAX)) {
            પાછા "argument out of range";
        }

        કિંમત = (u64)signed_કિંમત;
    } નહીંતર {
        કિંમત = strtoull(i_arg, &end, 10);

        જો (i_અંક_Bits == 32 and કિંમત > UINT32_MAX) {
            પાછા "argument out of range";
        }
    }

    જો (errno == ERANGE) {
        પાછા "argument out of range";
    }

    જો (end == i_arg or *end) {
        પાછા "argument is not a number";
    }

    *o_કિંમત = કિંમત;

    પાછા m3Err_none;
}


#જો d_m3HasFloat
સ્થિર
M3Result ParseArgFloat (ccstr_t i_arg, f64* o_કિંમત)
{
    જો (not i_arg or not *i_arg) {
        પાછા "empty argument";
    }

    જો (isspace((unsigned અક્ષર)*i_arg)) {
        પાછા "argument is not a number";
    }

    અક્ષર* end = ખાલી;

    errno = 0;

    f64 કિંમત = strtod(i_arg, &end);

    જો (end == i_arg or *end) {
        પાછા "argument is not a number";
    }

    // strtod reports underflow through ERANGE as well, and a denormal result is
    // perfectly usable, so only an overflow to infinity is out of range
    જો (errno == ERANGE and (કિંમત > DBL_MAX or કિંમત < -DBL_MAX)) {
        પાછા "argument out of range";
    }

    *o_કિંમત = કિંમત;

    પાછા m3Err_none;
}
#endif


// A reference argument is either the null reference or a host handle written as
// an integer.
સ્થિર
M3Result ParseArgReference (ccstr_t i_arg, u64* o_કિંમત)
{
    જો (i_arg and strcmp(i_arg, "null") == 0) {
        *o_કિંમત = 0;
        પાછા m3Err_none;
    }

    પાછા ParseArgInteger(i_arg, 64, o_કિંમત);
}


M3Result m3_CallArgv (IM3Function i_function, uint32_t i_argc, અચલ અક્ષર* i_argv[])
{
    IM3FuncType ftype   = i_function->funcType;
    IM3Runtime  runtime = i_function->module->runtime;
    M3Result    result  = m3Err_none;
    u8*         s       = ખાલી;

    જો (i_argc != ftype->અંક_Args) {
        પાછા m3_Err_argument_ગણતરી_Mismatch;
    }
    જો (!i_function->compiled) {
        પાછા m3Err_missingCompiledCode;
    }

#જો d_m3RecordBacktraces
    ClearBacktrace(runtime);
#endif

    m3_ઢગલો_Check_Init();

_   (checkStartFunction(i_function->module))

_   (Get_ઢગલો_Pointer_For_Args(i_function, &s))

    માટે (u32 i = 0; i < ftype->અંક_Args; ++i) {
        u64 કિંમત = 0;
#જો d_m3HasFloat
        f64 fvalue = 0;
#endif
        પસંદગી (d_FuncArgType(ftype, i)) {
        સ્થિતિ c_m3Type_i32:
_           (ParseArgInteger(i_argv[i], 32, &કિંમત)) * (i32*)(s) = (i32)કિંમત;
            s += 8;
            તોડો;
        સ્થિતિ c_m3Type_i64:
_           (ParseArgInteger(i_argv[i], 64, &કિંમત)) * (i64*)(s) = (i64)કિંમત;
            s += 8;
            તોડો;
        સ્થિતિ c_m3Type_funcref:
        સ્થિતિ c_m3Type_externref:
        સ્થિતિ c_m3Type_exnref:
_           (ParseArgReference(i_argv[i], &કિંમત)) * (uintptr_t*)(s) = (uintptr_t)કિંમત;
            s += 8;
            તોડો;
#જો d_m3HasFloat
            // strtof would be less portable
        સ્થિતિ c_m3Type_f32:
_           (ParseArgFloat(i_argv[i], &fvalue)) * (f32*)(s) = (f32)fvalue;
            s += 8;
            તોડો;
        સ્થિતિ c_m3Type_f64:
_           (ParseArgFloat(i_argv[i], &fvalue)) * (f64*)(s) = fvalue;
            s += 8;
            તોડો;
#endif
        મૂળભૂત: _throw("unknown argument type");
        }
    }

    result = RunCodeChecked(runtime, i_function);

    Report_Native_ઢગલો_Usage();

    runtime->lastCalled = result ? ખાલી : i_function;

    _catch: પાછા result;
}


//u8 * AlignStackPointerTo64Bits (const u8 * i_stack)
//{
//    uintptr_t ptr = (uintptr_t) i_stack;
//    return (u8 *) ((ptr + 7) & ~7);
//}


M3Result m3_GetResults (IM3Function i_function, uint32_t i_retc, અચલ શૂન્ય* o_retptrs[])
{
    IM3FuncType ftype   = i_function->funcType;
    IM3Runtime  runtime = i_function->module->runtime;

    જો (i_retc != ftype->અંક_Rets) {
        પાછા m3_Err_argument_ગણતરી_Mismatch;
    }
    જો (i_function != runtime->lastCalled) {
        પાછા "function not called";
    }

    u8* s = (u8*)runtime->ઢગલો;

    માટે (u32 i = 0; i < ftype->અંક_Rets; ++i) {
        પસંદગી (d_FuncRetType(ftype, i)) {
        સ્થિતિ c_m3Type_i32:
            *(i32*)o_retptrs[i] = *(i32*)(s);
            s += 8;
            તોડો;
        સ્થિતિ c_m3Type_i64:
            *(i64*)o_retptrs[i] = *(i64*)(s);
            s += 8;
            તોડો;
        સ્થિતિ c_m3Type_funcref:
        સ્થિતિ c_m3Type_externref:
        સ્થિતિ c_m3Type_exnref:
            *(uintptr_t*)o_retptrs[i] = *(uintptr_t*)(s);
            s += 8;
            તોડો;
#જો d_m3HasFloat
        સ્થિતિ c_m3Type_f32:
            *(f32*)o_retptrs[i] = *(f32*)(s);
            s += 8;
            તોડો;
        સ્થિતિ c_m3Type_f64:
            *(f64*)o_retptrs[i] = *(f64*)(s);
            s += 8;
            તોડો;
#endif
        મૂળભૂત: પાછા "unknown return type";
        }
    }
    પાછા m3Err_none;
}

M3Result m3_GetResultsV (IM3Function i_function, ...)
{
    va_list ap;
    va_start(ap, i_function);
    M3Result r = m3_GetResultsVL(i_function, ap);
    va_end(ap);
    પાછા r;
}

M3Result m3_GetResultsVL (IM3Function i_function, va_list o_rets)
{
    IM3Runtime  runtime = i_function->module->runtime;
    IM3FuncType ftype   = i_function->funcType;

    જો (i_function != runtime->lastCalled) {
        પાછા "function not called";
    }

    u8* s = (u8*)runtime->ઢગલો;
    માટે (u32 i = 0; i < ftype->અંક_Rets; ++i) {
        પસંદગી (d_FuncRetType(ftype, i)) {
        સ્થિતિ c_m3Type_i32:
            *va_arg(o_rets, i32*) = *(i32*)(s);
            s += 8;
            તોડો;
        સ્થિતિ c_m3Type_i64:
            *va_arg(o_rets, i64*) = *(i64*)(s);
            s += 8;
            તોડો;
        સ્થિતિ c_m3Type_funcref:
        સ્થિતિ c_m3Type_externref:
        સ્થિતિ c_m3Type_exnref:
            *va_arg(o_rets, uintptr_t*) = *(uintptr_t*)(s);
            s += 8;
            તોડો;
#જો d_m3HasFloat
        સ્થિતિ c_m3Type_f32:
            *va_arg(o_rets, f32*) = *(f32*)(s);
            s += 8;
            તોડો;
        સ્થિતિ c_m3Type_f64:
            *va_arg(o_rets, f64*) = *(f64*)(s);
            s += 8;
            તોડો;
#endif
        મૂળભૂત: પાછા "unknown argument type";
        }
    }
    પાછા m3Err_none;
}

શૂન્ય ReleaseCodePageNoTrack (IM3Runtime i_runtime, IM3CodePage i_codePage)
{
    જો (i_codePage) {
        IM3CodePage* list;

        bool pageFull = (અંક_Free_લીટીઓ(i_codePage) < d_m3_Code_Page_Free_લીટીઓ_Threshold);
        જો (pageFull) {
            list = &i_runtime->pagesFull;
        } નહીંતર {
            list = &i_runtime->pagesOpen;
        }

        ધકેલો_Code_Page(list, i_codePage);                         m3log (emit, "release page: %d to queue: '%s'", i_codePage->info.sequence, pageFull ? "full" : "open")
    }
}


IM3CodePage AcquireCodePageWithCapacity (IM3Runtime i_runtime, u32 i_min_લીટી_ગણતરી)
{
    IM3CodePage page = RemoveCodePageOfCapacity(&i_runtime->pagesOpen, i_min_લીટી_ગણતરી);

    જો (not page) {
        page = Environment_AcquireCodePage(i_runtime->environment, i_min_લીટી_ગણતરી);

        જો (not page) {
            page = NewCodePage(i_runtime, i_min_લીટી_ગણતરી);
        }

        જો (page) {
            i_runtime->અંક_Code_Pages++;
        }
    }

    જો (page) {                                                  m3log (emit, "acquire page: %d", page->info.sequence);
        i_runtime->અંક_Active_Code_Pages++;
    }

    પાછા page;
}


IM3CodePage AcquireCodePage (IM3Runtime i_runtime)
{
    પાછા AcquireCodePageWithCapacity(i_runtime, d_m3_Code_Page_Free_લીટીઓ_Threshold);
}


શૂન્ય ReleaseCodePage (IM3Runtime i_runtime, IM3CodePage i_codePage)
{
    જો (i_codePage) {
        ReleaseCodePageNoTrack(i_runtime, i_codePage);
        i_runtime->અંક_Active_Code_Pages--;

#જો defined(DEBUG)
        u32 અંક_Open = ગણતરી_Code_Pages(i_runtime->pagesOpen);
        u32 અંક_Full = ગણતરી_Code_Pages(i_runtime->pagesFull);

        m3log(runtime, "runtime: %p; open-pages: %d; full-pages: %d; active: %d; total: %d", i_runtime, અંક_Open, અંક_Full, i_runtime->અંક_Active_Code_Pages, i_runtime->અંક_Code_Pages);

        d_m3Assert(અંક_Open + અંક_Full + i_runtime->અંક_Active_Code_Pages == i_runtime->અંક_Code_Pages);

#  જો d_m3LogCodePages
        dump_code_page(i_codePage, /* startPC: */ ખાલી);
#  endif
#endif
    }
}


#જો d_m3VerboseErrorMessages
M3Result m3Error (M3Result i_result, IM3Runtime i_runtime, IM3Module i_module, IM3Function i_function,
                  અચલ અક્ષર* અચલ i_file, u32 i_લીટી_અંક, અચલ અક્ષર* અચલ i_errorMessage, ...)
{
    જો (i_runtime) {
        i_runtime->error         = (M3ErrorInfo){ .result = i_result, .runtime = i_runtime, .module = i_module, .function = i_function, .file = i_file, .લીટી_2 = i_લીટી_અંક };
        i_runtime->error.message = i_runtime->error_message;

        va_list args;
        va_start(args, i_errorMessage);
        vsnprintf(i_runtime->error_message, કદ(i_runtime->error_message), i_errorMessage, args);
        va_end(args);
    }

    પાછા i_result;
}
#endif


શૂન્ય m3_GetErrorInfo (IM3Runtime i_runtime, M3ErrorInfo* o_info)
{
    જો (i_runtime) {
        *o_info = i_runtime->error;
        m3_ResetErrorInfo(i_runtime);
    }
}


શૂન્ય m3_ResetErrorInfo (IM3Runtime i_runtime)
{
    જો (i_runtime) {
        M3_INIT(i_runtime->error);
        i_runtime->error.message = "";
    }
}

uint8_t* m3_GetMemory (IM3Module i_module, size_t* o_memorySizeInBytes, uint32_t i_memoryIndex)
{
    uint8_t* memory = ખાલી;
    size_t   size   = 0;

    જો (i_module and i_memoryIndex < i_module->અંક_Memories) {
        IM3Memory mem = i_module->memories[i_memoryIndex];

        જો (mem->mallocated) {
            size = mem->mallocated->length;

            જો (size) {
                memory = m3MemData(mem->mallocated);
            }
        }
    }

    જો (o_memorySizeInBytes) {
        *o_memorySizeInBytes = size;
    }

    પાછા memory;
}


size_t m3_GetMemorySize (IM3Module i_module, uint32_t i_memoryIndex)
{
    જો (not i_module or i_memoryIndex >= i_module->અંક_Memories) {
        પાછા 0;
    }

    IM3Memory mem = i_module->memories[i_memoryIndex];

    પાછા mem->mallocated ? mem->mallocated->length : 0;
}


size_t m3_GetMemorySizeAt (અચલ શૂન્ય* i_memory)
{
    જો (not i_memory) {
        પાછા 0;
    }

    // the header sits immediately before the data it describes
    અચલ M3MemoryHeader* header = ((અચલ M3MemoryHeader*)i_memory) - 1;

    પાછા header->length;
}


M3Result m3_FindExportedMemory (IM3Module i_module, cstr_t i_name, u32* o_memoryIndex)
{
    જો (not i_module or not i_name or not o_memoryIndex) {
        પાછા m3Err_unknownMemory;
    }

    અચલ M3Export* e = Module_FindExport(i_module, d_externalKind_memory, i_name);

    જો (not e) {
        પાછા m3Err_unknownMemory;
    }

    *o_memoryIndex = e->index;
    પાછા m3Err_none;
}


bool Module_HasLinkedHostImport (IM3Module i_module, cstr_t i_importModule)
{
    માટે (u32 i = 0; i < i_module->અંક_Functions; ++i) {
        IM3Function f = &i_module->functions[i];

        // hostMemory is set when a host function is bound to the import, so it
        // is also the mark of one - see CompileRawFunction
        જો (not f->hostMemory or f->wasm or f->resolved) {
            ચાલુ;
        }

        જો (f->import.moduleUtf8 and strcmp(f->import.moduleUtf8, i_importModule) == 0) {
            પાછા true;
        }
    }

    પાછા false;
}


M3Result m3_BindImportMemory (IM3Module io_module, cstr_t i_importModule, u32 i_memoryIndex)
{
    જો (not io_module or not i_importModule) {
        પાછા m3Err_moduleNotLinked;
    }

    જો (i_memoryIndex >= io_module->અંક_Memories) {
        પાછા m3Err_unknownMemory;
    }

    // the memory itself, not the index: linking may have pointed this slot at
    // another module's memory, and growing one reallocates behind it
    IM3Memory memory = io_module->memories[i_memoryIndex];

    અચલ bool wildcardModule = (strcmp(i_importModule, "*") == 0);

    માટે (u32 i = 0; i < io_module->અંક_Functions; ++i) {
        IM3Function f = &io_module->functions[i];

        // only an import that a host function was bound to has a memory to
        // pin: one with a body of its own reads memory through the interpreter,
        // and one resolved to another module's export runs that module's code
        જો (not f->hostMemory or f->wasm or f->resolved) {
            ચાલુ;
        }

        જો (wildcardModule or (f->import.moduleUtf8 and strcmp(f->import.moduleUtf8, i_importModule) == 0)) {
            f->hostMemory = memory;
        }
    }

    પાછા m3Err_none;
}


M3BacktraceInfo* m3_GetBacktrace (IM3Runtime i_runtime)
{
#જો d_m3RecordBacktraces
    પાછા &i_runtime->backtrace;
#નહીંતર
    પાછા ખાલી;
#endif
}


//-------------------------------------------------------------------------------------------------------------------------------
//  snapshots & suspendable execution
//-------------------------------------------------------------------------------------------------------------------------------

શૂન્ય m3_SetSuspendable (IM3Runtime io_runtime, bool i_suspendable)
{
    જો (io_runtime) {
        io_runtime->isSuspendable = i_suspendable;
    }
}

શૂન્ય m3_RequestSuspend (IM3Runtime io_runtime)
{
    જો (io_runtime) {
        io_runtime->suspendRequested = true;
    }
}

bool m3_IsSuspended (IM3Runtime i_runtime)
{
    જો (!i_runtime) {
        પાછા false;
    }
#જો d_m3_Has_ઢગલો_Switching
    જો (i_runtime->મૂળ_Continuation && i_runtime->મૂળ_Continuation->state == cont_suspended) {
        પાછા true;
    }
#endif
    પાછા false;
}

M3Result m3_ResumeRuntime (IM3Runtime io_runtime)
{
    જો (!io_runtime) {
        પાછા m3Err_mallocFailed;
    }
    જો (!io_runtime->isSuspendable) {
        પાછા m3Err_none;
    }
#જો d_m3HasSnapshots
    // a restore that failed leaves nothing to resume, and says so every time
    // rather than resuming nothing successfully
    જો (io_runtime->મૂળ_Continuation and io_runtime->મૂળ_Continuation->entryFunction and
        io_runtime->મૂળ_Continuation->entryFunction->module->isUnusable) {
        પાછા "a snapshot failed to restore into the module, so it cannot run";
    }
#endif
#જો d_m3_Has_ઢગલો_Switching
    જો (!io_runtime->મૂળ_Continuation || io_runtime->મૂળ_Continuation->state != cont_suspended) {
        પાછા m3Err_none;
    }
    // what finishes returns into the bottom of the stack, where it was entered
    io_runtime->ઢગલો = io_runtime->origin_ઢગલો;

    // this is the outermost call again, as far as a host function calling back
    // into Wasm from inside it can tell
    io_runtime->entered = io_runtime->મૂળ_Continuation->entryFunction;
    io_runtime->callNesting++;

    d_m3_ઢગલો_Limit_Enter(io_runtime);
    io_runtime->resumePastCheck = true;
    M3Result result             = (M3Result)ResumeContinuation(io_runtime, io_runtime->મૂળ_Continuation);
    io_runtime->resumePastCheck = false;
    d_m3_ઢગલો_Limit_Leave(io_runtime);

#  જો d_m3HasExceptionHandling
    જો (M3_UNLIKELY(result == m3Err_pendingException)) {
        result = m3Err_trapUncaughtException;
    }
    RecordUncaughtException(io_runtime, result);
#  endif

    io_runtime->callNesting--;

#  જો d_m3HasExceptionHandling
    જો (result != m3Err_continuationSuspended and io_runtime->callNesting == 0) {
        FreeExceptions(io_runtime);
    }
#  endif

    // the results are where the paused call left room for them, so a finished
    // resume can be asked for them the way the call itself could have been
    io_runtime->lastCalled = (result == m3Err_none) ? io_runtime->મૂળ_Continuation->entryFunction : ખાલી;

    Runtime_Place_Call_ઢગલો(io_runtime);
    પાછા result;
#નહીંતર
    પાછા m3Err_none;
#endif
}
