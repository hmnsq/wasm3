//
//  m3_env.c
//
//  Created by Steven Massey on 4/19/19.
//  Copyright © 2019 Steven Massey. All rights reserved.
//

#അന്തര്ഭാവ <stdarg.h>
#അന്തര്ഭാവ <limits.h>
#അന്തര്ഭാവ <errno.h>
#അന്തര്ഭാവ <float.h>
#അന്തര്ഭാവ <ctype.h>

#അന്തര്ഭാവ "m3_env.h"
#അന്തര്ഭാവ "m3_bind.h"
#അന്തര്ഭാവ "m3_deterministic.h"
#അന്തര്ഭാവ "m3_compile.h"
#അന്തര്ഭാവ "m3_host.h"
#അന്തര്ഭാവ "m3_exception.h"
#അന്തര്ഭാവ "m3_info.h"


IM3Environment m3_NewEnvironment ()
{
    IM3Environment env = m3_AllocStruct(M3Environment);

    എങ്കിൽ (env) {
        _try
        {
            // create FuncTypes for all simple block return ValueTypes.
            // v128 is skipped: it parses as a slot but has no operations.
            വേണ്ടി (u8 t = c_m3Type_none; t < c_m3_Type_എണ്ണം; t++) {
                എങ്കിൽ (t == c_m3Type_v128) {
                    തുടരുക;
                }

                IM3FuncType ftype;
_               (AllocFuncType(&ftype, 1));

                ftype->അക്കം_Args  = 0;
                ftype->അക്കം_Rets  = (t == c_m3Type_none) ? 0 : 1;
                ftype->types[0] = t;

                Environment_AddFuncType(env, &ftype);

                env->retFuncTypes[t] = ftype;
            }
        }

        _catch:
        എങ്കിൽ (result) {
            m3_FreeEnvironment(env);
            env = ശൂന്യ;
        }
    }

    തിരികെ env;
}


ശൂന്യം Environment_Release (IM3Environment i_environment)
{
    IM3FuncType ftype = i_environment->funcTypes;

    ആകുമ്പോൾ (ftype) {
        IM3FuncType next = ftype->next;
        m3_Free(ftype);
        ftype = next;
    }

    m3log(runtime, "freeing %d pages from environment", എണ്ണം_Code_Pages(i_environment->pagesReleased));
    FreeCodePages(&i_environment->pagesReleased);
}


ശൂന്യം m3_FreeEnvironment (IM3Environment i_environment)
{
    എങ്കിൽ (i_environment) {
        Environment_Release(i_environment);
        m3_Free(i_environment);
    }
}


ശൂന്യം m3_SetCustomSectionHandler (IM3Environment i_environment, M3SectionHandler i_handler)
{
    എങ്കിൽ (i_environment) {
        i_environment->customSectionHandler = i_handler;
    }
}


// returns the same io_funcType or replaces it with an equivalent that's already in the type linked list
M3Result Environment_AddFuncType (IM3Environment i_environment, IM3FuncType* io_funcType)
{
    IM3FuncType addType = *io_funcType;
    IM3FuncType newType = i_environment->funcTypes;

    ആകുമ്പോൾ (newType) {
        എങ്കിൽ (not newType->inRecGroup and AreFuncTypesEqual(newType, addType)) {
            m3_Free(addType);
            നിർത്തുക;
        }

        newType = newType->next;
    }

    എങ്കിൽ (newType == ശൂന്യ) {
        // a type index has to fit in the heap type field of an m3type_t
        എങ്കിൽ (i_environment->അക്കം_Func_Types >= d_m3_Max_Sane_Types_എണ്ണം) {
            m3_Free(addType);
            *io_funcType = ശൂന്യ;
            തിരികെ "too many distinct function types";
        }

        newType                  = addType;
        newType->canonicalIndex  = i_environment->അക്കം_Func_Types++;
        newType->next            = i_environment->funcTypes;
        i_environment->funcTypes = newType;
    }

    *io_funcType = newType;

    തിരികെ m3Err_none;
}


// Adds a type without looking for an equal one first, and hands back the index
// it was given.
//
// Members of a recursive group do not get to share an entry with a type that
// merely looks like them: the group is their identity, so two groups spelled
// the same way are still two groups, and a type inside one is never the type
// outside it that matches field for field. Reserving the indices up front is
// also what lets the members refer to each other while they are being read.
M3Result Environment_ReserveFuncTypes (IM3Environment i_environment, u32 i_എണ്ണം, u16* o_firstIndex)
{
    എങ്കിൽ ((u64)i_environment->അക്കം_Func_Types + i_എണ്ണം > d_m3_Max_Sane_Types_എണ്ണം) {
        തിരികെ "too many distinct function types";
    }

    *o_firstIndex = i_environment->അക്കം_Func_Types;
    i_environment->അക്കം_Func_Types += (u16)i_എണ്ണം;

    തിരികെ m3Err_none;
}


ശൂന്യം Environment_AdoptFuncType (IM3Environment i_environment, IM3FuncType i_funcType, u16 i_canonicalIndex)
{
    i_funcType->inRecGroup     = true;
    i_funcType->canonicalIndex = i_canonicalIndex;
    i_funcType->next           = i_environment->funcTypes;
    i_environment->funcTypes   = i_funcType;
}


IM3CodePage RemoveCodePageOfCapacity (M3CodePage** io_list, u32 i_minimum_വരി_എണ്ണം)
{
    IM3CodePage prev = ശൂന്യ;
    IM3CodePage page = *io_list;

    ആകുമ്പോൾ (page) {
        എങ്കിൽ (അക്കം_Free_വരികൾ(page) >= i_minimum_വരി_എണ്ണം) {             d_m3Assert (page->info.usage_എണ്ണം == 0);
            IM3CodePage next = page->info.next;
            എങ്കിൽ (prev) {
                prev->info.next = next; // mid-list
            } അല്ലെങ്കിൽ {
                *io_list = next;       // front of list
            }

            നിർത്തുക;
        }

        prev = page;
        page = page->info.next;
    }

    തിരികെ page;
}


IM3CodePage Environment_AcquireCodePage (IM3Environment i_environment, u32 i_minimum_വരി_എണ്ണം)
{
    തിരികെ RemoveCodePageOfCapacity(&i_environment->pagesReleased, i_minimum_വരി_എണ്ണം);
}


ശൂന്യം Environment_ReleaseCodePages (IM3Environment i_environment, IM3CodePage i_codePageList)
{
    IM3CodePage end = i_codePageList;

    ആകുമ്പോൾ (end) {
        end->info.വരി_Index = 0; // reset page
#എങ്കിൽ d_m3RecordBacktraces
        end->info.mapping->size = 0;
#endif // d_m3RecordBacktraces

        IM3CodePage next = end->info.next;
        എങ്കിൽ (not next) {
            നിർത്തുക;
        }

        end = next;
    }

    എങ്കിൽ (end) {
        // push list to front
        end->info.next               = i_environment->pagesReleased;
        i_environment->pagesReleased = i_codePageList;
    }
}


IM3Runtime m3_NewRuntime (IM3Environment i_environment, u32 i_അട്ടി_Size_In_Bytes, ശൂന്യം* i_userdata)
{
    IM3Runtime runtime = m3_AllocStruct(M3Runtime);

    എങ്കിൽ (runtime) {
        m3_ResetErrorInfo(runtime);

        runtime->environment = i_environment;
        runtime->userdata    = i_userdata;

        Deterministic_InitRuntime(runtime);

        runtime->origin_അട്ടി = m3_Malloc("Wasm Stack", i_അട്ടി_Size_In_Bytes + 4 * വലുപ്പം(m3slot_t)); // TODO: more precise stack checks

        എങ്കിൽ (runtime->origin_അട്ടി) {
            runtime->അട്ടി         = runtime->origin_അട്ടി;
            runtime->അക്കം_അട്ടി_Slots = i_അട്ടി_Size_In_Bytes / വലുപ്പം(m3slot_t);          m3log (runtime, "new stack: %p, slots: %u", runtime->origin_അട്ടി, runtime->അക്കം_അട്ടി_Slots);
        } അല്ലെങ്കിൽ {
            m3_Free(runtime);
        }
    }

    തിരികെ runtime;
}

ശൂന്യം m3_SetValidation (IM3Runtime i_runtime, bool i_enable)
{
#എങ്കിൽ d_m3EnableValidation
    എങ്കിൽ (i_runtime) {
        i_runtime->skipValidation = not i_enable;
    }
#അല്ലെങ്കിൽ
    (ശൂന്യം)i_runtime;
    (ശൂന്യം)i_enable;              // nothing to skip: the validator was compiled out
#endif
}

M3Result m3_SetResourceLimit (IM3Runtime i_runtime, M3ResourceLimit i_limit, uint64_t i_മൂല്യം)
{
    എങ്കിൽ (not i_runtime) {
        തിരികെ m3Err_mallocFailed;
    }
    തിരഞ്ഞെടുപ്പ് (i_limit) {
    അവസ്ഥ c_m3Limit_MemoryBytes:
        എങ്കിൽ (i_മൂല്യം and i_മൂല്യം < i_runtime->memoryBytesUsed) {
            തിരികെ m3Err_resourceLimitBelowUsage;
        }
        i_runtime->memoryBytesLimit = i_മൂല്യം;
        തിരികെ m3Err_none;
    അവസ്ഥ c_m3Limit_TableElements:
        എങ്കിൽ (i_മൂല്യം and i_മൂല്യം < i_runtime->tableElementsUsed) {
            തിരികെ m3Err_resourceLimitBelowUsage;
        }
        i_runtime->tableElementsLimit = i_മൂല്യം;
        തിരികെ m3Err_none;
#എങ്കിൽ d_m3_Has_അട്ടി_Switching
    അവസ്ഥ c_m3Limit_Continuations:
        എങ്കിൽ (i_മൂല്യം and i_മൂല്യം < i_runtime->continuationsAllocated) {
            തിരികെ m3Err_resourceLimitBelowUsage;
        }
        i_runtime->continuationsLimit = (u32)M3_MIN(i_മൂല്യം, (u64)UINT32_MAX);
        ആകുമ്പോൾ (i_മൂല്യം and i_runtime->val_അട്ടി_Pool_എണ്ണം > i_മൂല്യം - i_runtime->continuationsAllocated) {
            m3_Free(i_runtime->val_അട്ടി_Pool[--i_runtime->val_അട്ടി_Pool_എണ്ണം]);
        }
        തിരികെ m3Err_none;
#അല്ലെങ്കിൽ
    അവസ്ഥ c_m3Limit_Continuations: തിരികെ m3Err_resourceLimitNotSupported;
#endif
    അവസ്ഥ c_m3Limit_GasUnits:
#എങ്കിൽ d_m3HasGasMetering
        i_runtime->gasLimit = i_runtime->gasRemaining = (i64)M3_MIN(i_മൂല്യം, (u64)INT64_MAX);
        തിരികെ m3Err_none;
#അല്ലെങ്കിൽ
        തിരികെ m3Err_resourceLimitNotSupported;
#endif
    സ്ഥിരസ്ഥിതി: തിരികെ m3Err_unknownResourceLimit;
    }
}

uint64_t m3_GetResourceLimit (IM3Runtime i_runtime, M3ResourceLimit i_limit)
{
    എങ്കിൽ (not i_runtime) {
        തിരികെ 0;
    }
    തിരഞ്ഞെടുപ്പ് (i_limit) {
    അവസ്ഥ c_m3Limit_MemoryBytes: തിരികെ i_runtime->memoryBytesLimit;
    അവസ്ഥ c_m3Limit_TableElements: തിരികെ i_runtime->tableElementsLimit;
#എങ്കിൽ d_m3_Has_അട്ടി_Switching
    അവസ്ഥ c_m3Limit_Continuations: തിരികെ i_runtime->continuationsLimit;
#endif
#എങ്കിൽ d_m3HasGasMetering
    അവസ്ഥ c_m3Limit_GasUnits: തിരികെ (u64)i_runtime->gasLimit;
#endif
    സ്ഥിരസ്ഥിതി: തിരികെ 0;
    }
}

uint64_t m3_GetResourceUsage (IM3Runtime i_runtime, M3ResourceLimit i_limit)
{
    എങ്കിൽ (not i_runtime) {
        തിരികെ 0;
    }
    തിരഞ്ഞെടുപ്പ് (i_limit) {
    അവസ്ഥ c_m3Limit_MemoryBytes: തിരികെ i_runtime->memoryBytesUsed;
    അവസ്ഥ c_m3Limit_TableElements: തിരികെ i_runtime->tableElementsUsed;
#എങ്കിൽ d_m3_Has_അട്ടി_Switching
    അവസ്ഥ c_m3Limit_Continuations: തിരികെ i_runtime->continuationsAllocated;
#endif
#എങ്കിൽ d_m3HasGasMetering
    അവസ്ഥ c_m3Limit_GasUnits: തിരികെ (u64)i_runtime->gasLimit - (u64)i_runtime->gasRemaining;
#endif
    സ്ഥിരസ്ഥിതി: തിരികെ 0;
    }
}

ശൂന്യം* m3_GetUserData (IM3Runtime i_runtime)
{
    തിരികെ i_runtime ? i_runtime->userdata : ശൂന്യ;
}


ശൂന്യം* ForEachModule (IM3Runtime i_runtime, ModuleVisitor i_visitor, ശൂന്യം* i_info)
{
    ശൂന്യം* r = ശൂന്യ;

    IM3Module module = i_runtime->modules;

    ആകുമ്പോൾ (module) {
        IM3Module next = module->next;
        എങ്കിൽ ((r = i_visitor(module, i_info))) {
            നിർത്തുക;
        }

        module = next;
    }

    തിരികെ r;
}


ശൂന്യം* _FreeModule (IM3Module i_module, ശൂന്യം* i_info)
{
    m3_FreeModule(i_module);
    തിരികെ ശൂന്യ;
}


// A module's memory and table index spaces hold borrowed pointers: linking
// repoints an imported slot at the memory or table the exporting module owns.
// m3_FreeModule tells a borrowed slot from an owned one by reading that owner,
// which only holds while the object is still allocated - and the free walk
// below visits modules in load order, with no ordering between an owner and
// the modules borrowing from it. So clear the borrowed slots first, in a pass
// that frees nothing; the walk then skips them on the NULL check it already
// does, instead of reading a memory some earlier module has freed.
ശൂന്യം* _ReleaseBorrowedSlots (IM3Module i_module, ശൂന്യം* i_info)
{
    വേണ്ടി (u32 i = 0; i < i_module->അക്കം_Memories; ++i) {
        IM3Memory memory = i_module->memories[i];

        എങ്കിൽ (memory and memory->owner != i_module) {
            i_module->memories[i] = ശൂന്യ;
        }
    }

    വേണ്ടി (u32 i = 0; i < i_module->അക്കം_Tables; ++i) {
        IM3Table table = i_module->tables[i];

        എങ്കിൽ (table and table->owner != i_module) {
            i_module->tables[i] = ശൂന്യ;
        }
    }

    തിരികെ ശൂന്യ;
}


ശൂന്യം Runtime_Release (IM3Runtime i_runtime)
{
    ForEachModule(i_runtime, _ReleaseBorrowedSlots, ശൂന്യ);
    ForEachModule(i_runtime, _FreeModule, ശൂന്യ);                    d_m3Assert (i_runtime->അക്കം_Active_Code_Pages == 0);

    Environment_ReleaseCodePages(i_runtime->environment, i_runtime->pagesOpen);
    Environment_ReleaseCodePages(i_runtime->environment, i_runtime->pagesFull);

    m3_Free(i_runtime->origin_അട്ടി);

#എങ്കിൽ d_m3EnableValidation
    m3_Free(i_runtime->validator);
#endif

#എങ്കിൽ d_m3HasExceptionHandling
    // a runtime let go of while suspended still holds its exceptions
    FreeExceptions(i_runtime);
    m3_Free(i_runtime->uncaughtException);
#endif
#എങ്കിൽ d_m3HasExceptionHandling || d_m3_Has_അട്ടി_Switching
    ആകുമ്പോൾ (i_runtime->hostTags) {
        M3HostTag* next = i_runtime->hostTags->next;
        m3_Free(i_runtime->hostTags);
        i_runtime->hostTags = next;
    }
#endif
#എങ്കിൽ d_m3_Has_അട്ടി_Switching
    Continuation_ReleaseAll(i_runtime);
#endif
}


ശൂന്യം m3_FreeRuntime (IM3Runtime i_runtime)
{
    എങ്കിൽ (i_runtime) {
        m3_PrintProfilerInfo();

        Runtime_Release(i_runtime);
        m3_Free(i_runtime);
    }
}


#എങ്കിൽ d_m3_Has_അട്ടി_Switching

IM3Continuation Continuation_New (IM3Runtime i_runtime, IM3FuncType i_type, IM3Function i_function)
{
    IM3Continuation cont = m3_AllocStruct(M3Continuation);
    എങ്കിൽ (!cont) {
        തിരികെ ശൂന്യ;
    }

    cont->type          = i_type;
    cont->entryFunction = i_function;
    cont->state         = cont_allocated;

    cont->അക്കം_അട്ടി_Slots = d_m3_Continuation_അട്ടി_Slots;
    // frames are recorded only when this continuation actually suspends, and
    // only as deep as it got, so the array starts out unallocated
    cont->frames    = ശൂന്യ;
    cont->framesCap = 0;
    cont->അക്കം_Frames = 0;

    cont->sp = cont->val_അട്ടി;
    cont->pc = i_function ? i_function->compiled : ശൂന്യ;
    cont->r0 = 0;
#  എങ്കിൽ d_m3HasFloat
    cont->fp0 = 0;
#  endif
    cont->bound_Args_എണ്ണം    = 0;
    cont->അക്കം_Suspend_Results = 0;
    cont->അക്കം_Handlers       = 0;
    cont->handlersPC        = ശൂന്യ;
    cont->parent            = ശൂന്യ;

    cont->next               = i_runtime->continuations;
    i_runtime->continuations = cont;

    തിരികെ cont;
}

IM3Continuation Continuation_ForkSuspended (IM3Runtime i_runtime, IM3Continuation i_cont)
{
    IM3Continuation newCont = Continuation_New(i_runtime, i_cont->type, i_cont->entryFunction);
    എങ്കിൽ (M3_UNLIKELY(not newCont)) {
        തിരികെ ശൂന്യ;
    }

    newCont->val_അട്ടി      = i_cont->val_അട്ടി;
    newCont->അക്കം_അട്ടി_Slots = i_cont->അക്കം_അട്ടി_Slots;
    newCont->sp            = i_cont->sp;
    newCont->pc            = i_cont->pc;
    newCont->suspendPoint  = i_cont->suspendPoint;
    newCont->r0            = i_cont->r0;
#  എങ്കിൽ d_m3HasFloat
    newCont->fp0 = i_cont->fp0;
#  endif
    newCont->frames    = i_cont->frames;
    newCont->framesCap = i_cont->framesCap;
    newCont->അക്കം_Frames = i_cont->അക്കം_Frames;

    newCont->bound_Args_എണ്ണം = 0;
#  എങ്കിൽ d_m3HasExceptionHandling
    newCont->resumeThrow = i_cont->resumeThrow;
    i_cont->resumeThrow  = ശൂന്യ;
#  endif
    newCont->അക്കം_Suspend_Results = i_cont->അക്കം_Suspend_Results;
    വേണ്ടി (u32 i = 0; i < i_cont->അക്കം_Suspend_Results; ++i) {
        newCont->suspendResultOffsets[i] = i_cont->suspendResultOffsets[i];
        newCont->suspendResultIs64[i]    = i_cont->suspendResultIs64[i];
    }

    newCont->അക്കം_Handlers = i_cont->അക്കം_Handlers;
    newCont->handlersPC  = i_cont->handlersPC;
    newCont->parent      = i_cont->parent;
    newCont->state       = cont_suspended;

    i_cont->val_അട്ടി  = ശൂന്യ;
    i_cont->frames    = ശൂന്യ;
    i_cont->framesCap = 0;
    i_cont->അക്കം_Frames = 0;
    i_cont->state     = cont_consumed;

    തിരികെ newCont;
}

// Only cont.new and snapshot restoration acquire stacks. Binding and suspension
// transfer them between records without changing the active count.
M3Result Continuation_Acquire_അട്ടി (IM3Runtime runtime, IM3Continuation cont)
{
    എങ്കിൽ (not Continuation_Can_Acquire_അട്ടി(runtime)) {
        തിരികെ m3Err_continuationLimitExceeded;
    }
    എങ്കിൽ (runtime->val_അട്ടി_Pool_എണ്ണം) {
        cont->val_അട്ടി = runtime->val_അട്ടി_Pool[--runtime->val_അട്ടി_Pool_എണ്ണം];
        memset(cont->val_അട്ടി, 0, (cont->അക്കം_അട്ടി_Slots + 4) * വലുപ്പം(m3slot_t));
    } അല്ലെങ്കിൽ {
        cont->val_അട്ടി = m3_AllocArray(m3slot_t, cont->അക്കം_അട്ടി_Slots + 4);
    }
    എങ്കിൽ (not cont->val_അട്ടി) {
        തിരികെ m3Err_mallocFailed;
    }
    cont->sp = cont->val_അട്ടി;
    runtime->continuationsAllocated++;
    തിരികെ m3Err_none;
}

ശൂന്യം Continuation_Recycle_അട്ടി (IM3Runtime runtime, IM3Continuation cont)
{
    എങ്കിൽ (not cont->val_അട്ടി or cont == runtime->വേര്_Continuation) {
        തിരികെ;
    }
    എങ്കിൽ (not runtime->val_അട്ടി_Pool) {
        runtime->val_അട്ടി_Pool = m3_AllocArray(m3slot_t*, 16);
        എങ്കിൽ (runtime->val_അട്ടി_Pool) {
            runtime->val_അട്ടി_Pool_Cap = 16;
        }
    }
    എങ്കിൽ (runtime->val_അട്ടി_Pool_എണ്ണം < runtime->val_അട്ടി_Pool_Cap and
        (not runtime->continuationsLimit or runtime->val_അട്ടി_Pool_എണ്ണം <
                                              runtime->continuationsLimit - runtime->continuationsAllocated + 1)) {
        runtime->val_അട്ടി_Pool[runtime->val_അട്ടി_Pool_എണ്ണം++] = cont->val_അട്ടി;
        cont->val_അട്ടി                                      = ശൂന്യ;
    } അല്ലെങ്കിൽ {
        m3_Free(cont->val_അട്ടി);
    }
    cont->sp = ശൂന്യ;
    runtime->continuationsAllocated--;
}

ശൂന്യം Continuation_ReleaseAll (IM3Runtime io_runtime)
{
    // Records live until teardown: a consumed reference can still be in a table
    // or on a Wasm stack. Its value stack has already been recycled.
    IM3Continuation cont = io_runtime->continuations;

    ആകുമ്പോൾ (cont) {
        IM3Continuation next = cont->next;

        m3_Free(cont->val_അട്ടി);
        m3_Free(cont->frames);
        m3_Free(cont);

        cont = next;
    }

    വേണ്ടി (u32 i = 0; i < io_runtime->val_അട്ടി_Pool_എണ്ണം; ++i) {
        m3_Free(io_runtime->val_അട്ടി_Pool[i]);
    }
    m3_Free(io_runtime->val_അട്ടി_Pool);
    io_runtime->val_അട്ടി_Pool_എണ്ണം = io_runtime->val_അട്ടി_Pool_Cap = 0;
    io_runtime->continuationsAllocated                          = 0;
    io_runtime->continuations                                   = ശൂന്യ;
    io_runtime->activeContinuation                              = ശൂന്യ;
    io_runtime->വേര്_Continuation                                = ശൂന്യ;
}


m3ret_t Continuation_RecordFrame (IM3Runtime i_runtime, സ്ഥിരം M3Frame* i_frame)
{
    IM3Continuation cont = i_runtime->activeContinuation;

    // the marker only travels between a suspend and the resume that runs it,
    // and that whole stretch has an active continuation
    d_m3Assert(cont);

    എങ്കിൽ (cont->അക്കം_Frames == cont->framesCap) {
        u32 newCap = cont->framesCap ? cont->framesCap * 2 : 8;

        എങ്കിൽ (newCap > d_m3ContinuationMaxFrames) {
            newCap = d_m3ContinuationMaxFrames;
        }

        എങ്കിൽ (newCap == cont->framesCap) {
            തിരികെ m3_Err_trap_അട്ടി_Overflow;
        }

        M3Frame* frames = m3_ReallocArray(M3Frame, cont->frames, newCap, cont->framesCap);

        എങ്കിൽ (not frames) {
            തിരികെ m3Err_mallocFailed;
        }

        cont->frames    = frames;
        cont->framesCap = newCap;
    }

    cont->frames[cont->അക്കം_Frames++] = *i_frame;

    തിരികെ m3Err_continuationSuspended;
}

#endif // d_m3HasStackSwitching

M3Result EvaluateExpression (IM3Module i_module, ശൂന്യം* o_expressed, m3type_t i_type, bytes_t* io_bytes, cbytes_t i_end)
{
    M3Result result = m3Err_none;

    // OPTZ: use a simplified interpreter for expressions

    // create a temporary runtime context
#എങ്കിൽ defined(d_m3PreferStaticAlloc)
    സ്ഥിതം M3Runtime runtime;
#അല്ലെങ്കിൽ
    M3Runtime runtime;
#endif
    M3_INIT(runtime);

    runtime.environment   = i_module->runtime->environment;
    runtime.അക്കം_അട്ടി_Slots = i_module->runtime->അക്കം_അട്ടി_Slots;
    runtime.അട്ടി         = i_module->runtime->അട്ടി;

    m3stack_t അട്ടി = (m3stack_t)runtime.അട്ടി;

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

    എങ്കിൽ (o->page) {
        IM3FuncType ftype = ശൂന്യ;

#എങ്കിൽ d_m3HasTypedRefs
        // A reference type has to be checked as the type it is: the shared
        // return types are one per storage class, so borrowing one of those
        // would let any reference at all satisfy a particular one.
        എങ്കിൽ (IsSpelledRefType(i_type)) {
            result = AllocFuncType(&ftype, 1);

            എങ്കിൽ (not result) {
                ftype->അക്കം_Rets  = 1;
                ftype->types[0] = i_type;

                result = Environment_AddFuncType(runtime.environment, &ftype);
            }
        } അല്ലെങ്കിൽ
#endif
        {
            ftype = runtime.environment->retFuncTypes[BaseTypeOf(i_type)];
        }

        pc_t m3code = GetPagePC(o->page);

        എങ്കിൽ (not result) {
            result = CompileExpression(o, ftype);
        }

        എങ്കിൽ (not result && o->max_അട്ടി_Slots >= runtime.അക്കം_അട്ടി_Slots) {
            result = m3_Err_trap_അട്ടി_Overflow;
        }

        എങ്കിൽ (not result) {
#എങ്കിൽ (d_m3EnableOpProfiling || d_m3EnableOpTracing)
            m3ret_t r = RunCode(m3code, അട്ടി, ശൂന്യ, d_m3OpDefaultArgs, d_m3BaseCstr);
#അല്ലെങ്കിൽ
            m3ret_t r = RunCode(m3code, അട്ടി, ശൂന്യ, d_m3OpDefaultArgs);
#endif

            എങ്കിൽ (r == 0) {                                                                   m3log (runtime, "expression result: %s", SPrint_മൂല്യം (അട്ടി, i_type));
                എങ്കിൽ (SizeOfType(BaseTypeOf(i_type)) == വലുപ്പം(u32)) {
                    *(u32*)o_expressed = *((u32*)അട്ടി);
                } അല്ലെങ്കിൽ {
                    *(u64*)o_expressed = *((u64*)അട്ടി);
                }
            }
        }

        ReleaseCodePage(&runtime, o->page);
    } അല്ലെങ്കിൽ {
        result = m3Err_mallocFailedCodePage;
    }

    runtime.origin_അട്ടി = ശൂന്യ;        // prevent free(stack) in ReleaseRuntime
    Runtime_Release(&runtime);
    i_module->runtime = savedRuntime;

    *io_bytes = o->wasm;

    തിരികെ result;
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
സ്ഥിതം
bool LimitsSatisfy (u64 i_exportedSize, bool i_exportedHasMax, u64 i_exportedMax,
                    u64 i_importMin, bool i_importHasMax, u64 i_importMax)
{
    എങ്കിൽ (i_exportedSize < i_importMin) {
        തിരികെ false;
    }

    എങ്കിൽ (i_importHasMax and (not i_exportedHasMax or i_exportedMax > i_importMax)) {
        തിരികെ false;
    }

    തിരികെ true;
}


// The export of this kind under this name, or NULL. The index it carries is
// resolved by the caller, against the module's index space as it stands then -
// after linking, a re-exported import's slot holds the entity it was linked to.
സ്ഥിതം
സ്ഥിരം M3Export* Module_FindExport (IM3Module i_module, u8 i_kind, cstr_t i_name)
{
    വേണ്ടി (u32 i = 0; i < i_module->അക്കം_Exports; ++i) {
        സ്ഥിരം M3Export* e = &i_module->exports[i];

        എങ്കിൽ (e->kind == i_kind and strcmp(e->name, i_name) == 0) {
            തിരികെ e;
        }
    }

    തിരികെ ശൂന്യ;
}


സ്ഥിതം
IM3Function Module_FindExportedFunction (IM3Module i_module, cstr_t i_name)
{
    സ്ഥിരം M3Export* e = Module_FindExport(i_module, d_externalKind_function, i_name);

    // A module that re-exports an import names the placeholder here. Resolve to
    // the function that actually runs, or a host-side call would run it against
    // the importing module's memory.
    തിരികെ e ? Function_Implementation(&i_module->functions[e->index]) : ശൂന്യ;
}


സ്ഥിതം
IM3Memory Module_FindExportedMemory (IM3Module i_module, cstr_t i_name)
{
    സ്ഥിരം M3Export* e = Module_FindExport(i_module, d_externalKind_memory, i_name);

    തിരികെ e ? i_module->memories[e->index] : ശൂന്യ;
}


സ്ഥിതം
IM3Table Module_FindExportedTable (IM3Module i_module, cstr_t i_name)
{
    സ്ഥിരം M3Export* e = Module_FindExport(i_module, d_externalKind_table, i_name);

    തിരികെ e ? i_module->tables[e->index] : ശൂന്യ;
}


സ്ഥിതം
IM3Global Module_FindExportedGlobal (IM3Module i_module, cstr_t i_name)
{
    സ്ഥിരം M3Export* e = Module_FindExport(i_module, d_externalKind_global, i_name);

    തിരികെ e ? &i_module->globals[e->index] : ശൂന്യ;
}


#എങ്കിൽ d_m3HasExceptionHandling || d_m3_Has_അട്ടി_Switching
സ്ഥിതം
IM3Tag Module_FindExportedTag (IM3Module i_module, cstr_t i_name)
{
    സ്ഥിരം M3Export* e = Module_FindExport(i_module, d_externalKind_tag, i_name);

    എങ്കിൽ (not e) {
        തിരികെ ശൂന്യ;
    }

    IM3Tag tag = &i_module->tags[e->index];

    തിരികെ tag->resolved ? tag->resolved : tag;
}
#endif


// Whether the module exports anything at all under this name. Export names are
// unique within a module, so a name one of the lookups above missed but this
// one finds is exported as something else - a kind the import cannot be
// satisfied by, rather than a name the module never exported.
സ്ഥിതം
bool Module_HasExport (IM3Module i_module, cstr_t i_name)
{
    വേണ്ടി (u32 i = 0; i < i_module->അക്കം_Exports; ++i) {
        എങ്കിൽ (strcmp(i_module->exports[i].name, i_name) == 0) {
            തിരികെ true;
        }
    }

    തിരികെ false;
}


// Points each of the module's imports at whatever already-loaded module exports
// it. Runs before anything is allocated or initialized: a memory import has to
// be resolved before InitMemory would give it pages of its own, and a global
// import before InitGlobals runs an initializer that reads it.
സ്ഥിതം
M3Result LinkImports (IM3Runtime io_runtime, IM3Module io_module)
{
    M3Result result = m3Err_none;

    വേണ്ടി (u32 i = 0; i < io_module->അക്കം_Functions; ++i) {
        IM3Function f = &io_module->functions[i];

        എങ്കിൽ (f->wasm or not (f->import.moduleUtf8 and f->import.fieldUtf8)) {
            തുടരുക;
        }

        IM3Module from = m3_FindModule(io_runtime, f->import.moduleUtf8);
        എങ്കിൽ (not from) {
            തുടരുക;
        }

        IM3Function exported = Module_FindExportedFunction(from, f->import.fieldUtf8);
        എങ്കിൽ (not exported) {
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

    വേണ്ടി (u32 i = 0; i < io_module->അക്കം_Memories; ++i) {
        IM3Memory memory = io_module->memories[i];

        എങ്കിൽ (not memory->imported or memory->owner != io_module) {
            തുടരുക;
        }

        IM3Module from = m3_FindModule(io_runtime, memory->import.moduleUtf8);
        എങ്കിൽ (not from) {
            തുടരുക;
        }

        IM3Memory exported = Module_FindExportedMemory(from, memory->import.fieldUtf8);
        എങ്കിൽ (not exported) {
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
                 not LimitsSatisfy(exported->അക്കം_Pages, exported->hasMax, exported->maxPages,
                                   memory->initPages, memory->hasMax, memory->maxPages));

        // hand the slot over to the exporter's memory, and drop the placeholder
        FreeMemoryBlock(memory);
        FreeImportInfo(&memory->import);
        m3_Free(memory);

        io_module->memories[i] = exported;
    }

    വേണ്ടി (u32 i = 0; i < io_module->അക്കം_Tables; ++i) {
        IM3Table table = io_module->tables[i];

        എങ്കിൽ (not table->imported or table->owner != io_module) {
            തുടരുക;
        }

        IM3Module from = m3_FindModule(io_runtime, table->import.moduleUtf8);
        എങ്കിൽ (not from) {
            തുടരുക;
        }

        IM3Table exported = Module_FindExportedTable(from, table->import.fieldUtf8);
        എങ്കിൽ (not exported) {
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

    വേണ്ടി (u32 i = 0; i < io_module->അക്കം_Globals; ++i) {
        IM3Global g = &io_module->globals[i];

        എങ്കിൽ (not g->imported or not (g->import.moduleUtf8 and g->import.fieldUtf8)) {
            തുടരുക;
        }

        IM3Module from = m3_FindModule(io_runtime, g->import.moduleUtf8);
        എങ്കിൽ (not from) {
            തുടരുക;
        }

        IM3Global exported = Module_FindExportedGlobal(from, g->import.fieldUtf8);
        എങ്കിൽ (not exported) {
            _throwif(m3Err_incompatibleImportType, Module_HasExport(from, g->import.fieldUtf8));
            _throw(m3Err_unknownImport);
        }

        _throwif(m3Err_incompatibleImportType, exported->type != g->type);
        _throwif(m3Err_incompatibleImportType, exported->isMutable != g->isMutable);

        g->resolved = exported->resolved ? exported->resolved : exported;
    }

#എങ്കിൽ d_m3HasExceptionHandling || d_m3_Has_അട്ടി_Switching
    വേണ്ടി (u32 i = 0; i < io_module->അക്കം_Tags; ++i) {
        IM3Tag tag = &io_module->tags[i];

        എങ്കിൽ (not tag->imported or not (tag->import.moduleUtf8 and tag->import.fieldUtf8)) {
            തുടരുക;
        }

        IM3Module from = m3_FindModule(io_runtime, tag->import.moduleUtf8);
        എങ്കിൽ (not from) {
            തുടരുക;
        }

        IM3Tag exported = Module_FindExportedTag(from, tag->import.fieldUtf8);
        എങ്കിൽ (not exported) {
            _throwif(m3Err_incompatibleImportType, Module_HasExport(from, tag->import.fieldUtf8));
            _throw(m3Err_unknownImport);
        }

        _throwif(m3Err_incompatibleImportType, exported->type != tag->type);

        tag->resolved = exported->resolved ? exported->resolved : exported;
    }
#endif

    _catch: തിരികെ result;
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
    i_module->memory0 = i_module->അക്കം_Memories ? i_module->memories[0]
                                              : &i_module->emptyMemory;

    എങ്കിൽ (i_module->അക്കം_Memories == 0) {
        // nothing addressable, but _mem still has to point somewhere
        i_module->emptyMemory.owner    = i_module;
        i_module->emptyMemory.pageSize = d_m3DefaultMemPageSize;

_       (ResizeMemory(io_runtime, &i_module->emptyMemory, 0));
    }

    വേണ്ടി (u32 i = 0; i < i_module->അക്കം_Memories; ++i) {
        IM3Memory memory = i_module->memories[i];

        // a slot that already points at another module's memory is that
        // module's to allocate
        എങ്കിൽ (memory->owner != i_module or memory->mallocated) {
            തുടരുക;
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
        എങ്കിൽ (not memory->hasMax) {
            memory->maxPages = memory->isMemory64
                                 ? (pageSize > 1 ? (UINT64_MAX / pageSize) + 1 : UINT64_MAX)
                                 : (0x100000000ull / pageSize);
        }

_       (ResizeMemory(io_runtime, memory, memory->initPages));
    }

    _catch: തിരികെ result;
}


M3Result ResizeMemory (IM3Runtime io_runtime, IM3Memory memory, u64 i_അക്കം_Pages)
{
    M3Result result = m3Err_none;

    u64 അക്കം_Pages_To_Alloc = i_അക്കം_Pages;

    എങ്കിൽ (അക്കം_Pages_To_Alloc <= memory->maxPages) {
        // A 64-bit memory may ask for up to 2^48 pages, which overflows a u64
        // of bytes. Nothing that large can be backed, so refuse it up front
        // rather than multiplying into a wrapped size.
        _throwif("linear memory limitation exceeded",
                 അക്കം_Pages_To_Alloc > d_m3AddressLimit / memory->pageSize);

        u64 അക്കം_Page_Bytes = അക്കം_Pages_To_Alloc * memory->pageSize;

#എങ്കിൽ d_m3MaxLinearMemoryPages > 0
        // the limit is a memory size, counted in default-sized pages; comparing
        // it against a raw page count would make it 65536 times stricter for a
        // module whose pages are one byte
        _throwif("linear memory limitation exceeded",
                 അക്കം_Page_Bytes > (u64)d_m3MaxLinearMemoryPages * d_m3DefaultMemPageSize);
#endif

        // Limit the amount of memory that gets actually allocated
        എങ്കിൽ (io_runtime->memoryLimit) {
            അക്കം_Page_Bytes = M3_MIN(അക്കം_Page_Bytes, (u64)io_runtime->memoryLimit);
        }

        u64 oldLength = memory->mallocated ? memory->mallocated->length : 0;
        u64 otherUsed = io_runtime->memoryBytesUsed - oldLength;
        _throwif(m3Err_memoryLimitExceeded, io_runtime->memoryBytesLimit and
                                              (അക്കം_Page_Bytes > io_runtime->memoryBytesLimit or
                                               otherUsed > io_runtime->memoryBytesLimit - അക്കം_Page_Bytes));

        _throwif("linear memory limitation exceeded", അക്കം_Page_Bytes > (u64)SIZE_MAX - വലുപ്പം(M3MemoryHeader));

#എങ്കിൽ d_m3GuardedMemory

        // A slot holds every address a Wasm access can name, and nothing beyond
        // that: a memory larger than the data part of one would put addresses past
        // the guard, where a fault is nobody's to recognize
        _throwif("linear memory limitation exceeded", അക്കം_Page_Bytes > d_m3GuardedDataBytes);

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

        എങ്കിൽ (memory->guardSlot == ശൂന്യ) {
            memory->guardSlot = Guard_TakeSlot();
            _throwifnull(memory->guardSlot);
        }

        // Growing only ever commits more of the same slot, so the memory never
        // moves - which the interpreter does not rely on, but nothing minds
        _throwif(m3Err_mallocFailed, not Guard_CommitSlot(memory->guardSlot, (size_t)അക്കം_Page_Bytes));

        memory->mallocated = Guard_SlotHeader(memory->guardSlot);

#അല്ലെങ്കിൽ

        size_t അക്കം_Bytes = (size_t)അക്കം_Page_Bytes + വലുപ്പം(M3MemoryHeader);

        // What is really there, which is not numPages * pageSize: the limit above
        // caps the bytes without capping the page count, so a memory can be short
        // of a page - by design, since a whole 64 KiB page is often more than an
        // MCU has. Measuring the old block by the page count would overstate it,
        // and then realloc never sees the size it already has and copies the
        // whole limit again on every grow past it.
        size_t അക്കം_Previous_Bytes = memory->mallocated ? memory->mallocated->length + വലുപ്പം(M3MemoryHeader) : 0;

        ശൂന്യം* newMem = m3_Realloc("Wasm Linear Memory", memory->mallocated, അക്കം_Bytes, അക്കം_Previous_Bytes);
        _throwifnull(newMem);

        memory->mallocated = (M3MemoryHeader*)newMem;

#endif

#എങ്കിൽ d_m3LogRuntime
        M3MemoryHeader* oldMallocated = memory->mallocated;
#endif

        io_runtime->memoryBytesUsed = otherUsed + അക്കം_Page_Bytes;
        memory->അക്കം_Pages            = അക്കം_Pages_To_Alloc;

        memory->mallocated->length  = (size_t)അക്കം_Page_Bytes;
        memory->mallocated->runtime = io_runtime;
        memory->mallocated->memory  = memory;

        memory->mallocated->max_അട്ടി = (m3slot_t*)io_runtime->origin_അട്ടി + io_runtime->അക്കം_അട്ടി_Slots;

        m3log(runtime, "resized old: %p; mem: %p; length: %zu; pages: %llu", oldMallocated, memory->mallocated, memory->mallocated->length, (unsigned long long)memory->അക്കം_Pages);
    } അല്ലെങ്കിൽ {
        result = m3Err_wasmMemoryOverflow;
    }

    _catch: തിരികെ result;
}


ശൂന്യം FreeMemoryBlock (IM3Memory io_memory)
{
    എങ്കിൽ (io_memory->mallocated and io_memory->mallocated->runtime) {
        io_memory->mallocated->runtime->memoryBytesUsed -= io_memory->mallocated->length;
    }
#എങ്കിൽ d_m3GuardedMemory
    Guard_GiveSlot(io_memory->guardSlot);
    io_memory->guardSlot = ശൂന്യ;

    io_memory->mallocated = ശൂന്യ;
#അല്ലെങ്കിൽ
    m3_Free(io_memory->mallocated);
#endif
}


M3Result InitGlobals (IM3Module io_module)
{
    M3Result result = m3Err_none;

    എങ്കിൽ (io_module->അക്കം_Globals) {
        // placing the globals in their structs isn't good for cache locality, but i don't really know what the global
        // access patterns typically look like yet.

        //          io_module->globalMemory = m3Alloc (m3reg_t, io_module->numGlobals);

        //          if (io_module->globalMemory)
        {
            വേണ്ടി (u32 i = 0; i < io_module->അക്കം_Globals; ++i) {
                M3Global* g = &io_module->globals[i];                           m3log (runtime, "initializing global: %d", i);

                എങ്കിൽ (g->init_വ്യഞ്ജകം) {
                    bytes_t start = g->init_വ്യഞ്ജകം;

                    result = EvaluateExpression(io_module, &g->i64_മൂല്യം, g->type, &start, g->init_വ്യഞ്ജകം + g->init_വ്യഞ്ജകം_Size);

                    എങ്കിൽ (not result) {
                        // io_module->globalMemory [i] = initValue;
                    } അല്ലെങ്കിൽ {
                        നിർത്തുക;
                    }
                } അല്ലെങ്കിൽ {                                                        m3log (runtime, "importing global");
                }
            }
        }
        //          else result = ErrorModule (m3Err_mallocFailed, io_module, "could allocate globals for module: '%s", io_module->name);
    }

    തിരികെ result;
}


M3Result InitDataSegments (IM3Module io_module)
{
    M3Result result = m3Err_none;

    വേണ്ടി (u32 i = 0; i < io_module->അക്കം_Data_Segments; ++i) {
        M3DataSegment* segment = &io_module->dataSegments[i];

        // A passive segment stays available for memory.init until data.drop.
        // An active one is copied here and then counts as dropped.
        എങ്കിൽ (segment->isPassive) {
            തുടരുക;
        }

        _throwif("data segment memory index out of range",
                 segment->memoryRegion >= io_module->അക്കം_Memories);

        IM3Memory io_memory = io_module->memories[segment->memoryRegion];

        _throwif("unallocated linear memory", !(io_memory->mallocated));

        // The offset expression has the memory's address type, and is
        // unsigned: an i32 offset of -1 is 4294967295, way out of bounds
        // rather than negative.
        u64     segmentOffset = 0;
        bytes_t start         = segment->init_വ്യഞ്ജകം;

        എങ്കിൽ (io_memory->isMemory64) {
_           (EvaluateExpression(io_module, &segmentOffset, c_m3Type_i64, &start, segment->init_വ്യഞ്ജകം + segment->init_വ്യഞ്ജകം_Size));
        } അല്ലെങ്കിൽ {
            u32 offset32;
_           (EvaluateExpression(io_module, &offset32, c_m3Type_i32, &start, segment->init_വ്യഞ്ജകം + segment->init_വ്യഞ്ജകം_Size));
            segmentOffset = offset32;
        }

        m3log(runtime, "loading data segment: %d; size: %d; offset: %llu", i, segment->size, (unsigned long long)segmentOffset);

        എങ്കിൽ (segmentOffset <= io_memory->mallocated->length &&
            (u64)segment->size <= io_memory->mallocated->length - segmentOffset) {
            u8* dest = m3MemData(io_memory->mallocated) + segmentOffset;
            memcpy(dest, segment->data, segment->size);
        } അല്ലെങ്കിൽ {
            _throw("data segment out of bounds");
        }

        segment->dropped = true;
    }

    _catch: തിരികെ result;
}


// Turns a segment's elements into references. Element expressions are constant
// expressions restricted to ref.null/ref.func, so they're read directly rather
// than run through the compiler.
സ്ഥിതം
M3Result ResolveElements (IM3Module io_module, M3ElementSegment* i_segment, ശൂന്യം** o_elements)
{
    M3Result result = m3Err_none;

    bytes_t  pos = i_segment->elements;
    cbytes_t end = io_module->elementSectionEnd;

    വേണ്ടി (u32 e = 0; e < i_segment->അക്കം_Elements; ++e) {
        u32   funcIndex;
        ശൂന്യം* ref = ശൂന്യ;

        എങ്കിൽ (i_segment->is_വ്യഞ്ജകം) {
            m3opcode_t opcode;
_           (Read_opcode(&opcode, &pos, end));

            എങ്കിൽ (opcode == c_waOp_refFunc) {
_               (ReadLEB_u32(&funcIndex, &pos, end));
                _throwif("function index out of range", funcIndex >= io_module->അക്കം_Functions);
                ref = Function_Implementation(&io_module->functions[funcIndex]);
            } അല്ലെങ്കിൽ എങ്കിൽ (opcode == c_waOp_refNull) {
                i8 waType;
                u8 nullType;
_               (ReadLEB_i7(&waType, &pos, end));
_               (NormalizeType(&nullType, waType));
                _throwif(m3Err_typeMismatch, nullType != i_segment->type);
            } അല്ലെങ്കിൽ എങ്കിൽ (opcode == c_waOp_getGlobal) {
                // wasm 2.0 lets an element expression read an imported
                // immutable global, which is how one module seeds another's
                // table with a reference it exported.
                u32 globalIndex;
_               (ReadLEB_u32(&globalIndex, &pos, end));
                _throwif(m3Err_globaIndexOutOfBounds, globalIndex >= io_module->അക്കം_Globals);

                IM3Global global = &io_module->globals[globalIndex];

                _throwif(m3Err_globaIndexOutOfBounds, not global->imported);
                _throwif(m3Err_wasmMalformed, global->isMutable);
                _throwif(m3Err_typeMismatch, BaseTypeOf(global->type) != BaseTypeOf(i_segment->type));

                // read the cell the import was linked to, not the placeholder
                എങ്കിൽ (global->resolved) {
                    global = global->resolved;
                }

                ref = global->ref_മൂല്യം;
            } അല്ലെങ്കിൽ {
                _throw("constant expression required");
            }

_           (Read_opcode(&opcode, &pos, end));
            _throwif(m3Err_wasmMalformed, opcode != c_waOp_end);
        } അല്ലെങ്കിൽ {
_           (ReadLEB_u32(&funcIndex, &pos, end));
            _throwif("function index out of range", funcIndex >= io_module->അക്കം_Functions);
            ref = Function_Implementation(&io_module->functions[funcIndex]);
        }

        o_elements[e] = ref;
    }

    _catch: തിരികെ result;
}


M3Result InitTableAndElements (IM3Module io_module)
{
    M3Result result = m3Err_none;

    cbytes_t end = io_module->elementSectionEnd;
    M3Table* table;

    വേണ്ടി (u32 i = 0; i < io_module->അക്കം_Tables; ++i) {
        table = io_module->tables[i];

        // a slot pointing at another module's table is that module's to fill
        എങ്കിൽ (table->owner != io_module) {
            തുടരുക;
        }

        എങ്കിൽ (table->size) {
            IM3Runtime runtime = io_module->runtime;
            _throwif(m3Err_tableLimitExceeded, runtime->tableElementsLimit and
                                                 table->size > runtime->tableElementsLimit - runtime->tableElementsUsed);
            table->elements = m3_AllocArray(ശൂന്യം*, table->size);
            _throwifnull(table->elements);
            runtime->tableElementsUsed += table->size;

            എങ്കിൽ (table->init_വ്യഞ്ജകം) {
                ശൂന്യം*   മൂല്യം = ശൂന്യ;
                bytes_t start = table->init_വ്യഞ്ജകം;
_               (EvaluateExpression(io_module, &മൂല്യം, BaseTypeOf(table->type),
                                     &start, table->init_വ്യഞ്ജകം + table->init_വ്യഞ്ജകം_Size));

                വേണ്ടി (u32 e = 0; e < table->size; ++e) {
                    table->elements[e] = മൂല്യം;
                }
            }
        }
    }

    വേണ്ടി (u32 i = 0; i < io_module->അക്കം_Element_Segments; ++i) {
        M3ElementSegment* segment = &io_module->elementSegments[i];

        // Declarative segments only make their functions referenceable, and
        // passive ones wait for table.init, so neither is written out here.
        എങ്കിൽ (segment->mode == c_m3Elem_declarative) {
            segment->dropped = true;
            തുടരുക;
        }

        എങ്കിൽ (segment->mode == c_m3Elem_passive) {
            എങ്കിൽ (segment->അക്കം_Elements) {
                segment->resolved = m3_AllocArray(ശൂന്യം*, segment->അക്കം_Elements);
                _throwifnull(segment->resolved);
_               (ResolveElements(io_module, segment, segment->resolved));
            }
            തുടരുക;
        }

        table = io_module->tables[segment->tableIndex];

        // The offset expression has the table's index type, and is unsigned:
        // an i32 offset of -1 is 4294967295, out of bounds rather than negative.
        u64     offset = 0;
        bytes_t വ്യഞ്ജകം   = segment->init_വ്യഞ്ജകം;

        എങ്കിൽ (table->isTable64) {
_           (EvaluateExpression(io_module, &offset, c_m3Type_i64, &വ്യഞ്ജകം, end));
        } അല്ലെങ്കിൽ {
            u32 offset32;
_           (EvaluateExpression(io_module, &offset32, c_m3Type_i32, &വ്യഞ്ജകം, end));
            offset = offset32;
        }

        _throwif("out of bounds table access",
                 offset > table->size or segment->അക്കം_Elements > table->size - offset);

_       (ResolveElements(io_module, segment, table->elements + offset));

        segment->dropped = true;
    }

    _catch: തിരികെ result;
}

M3Result m3_CompileModule (IM3Module io_module)
{
    M3Result result = m3Err_none;

    വേണ്ടി (u32 i = 0; i < io_module->അക്കം_Functions; ++i) {
        IM3Function f = &io_module->functions[i];
        എങ്കിൽ (f->wasm and not f->compiled) {
_           (CompileFunction(f));
        }
    }

    _catch: തിരികെ result;
}

#എങ്കിൽ d_m3HasExceptionHandling

M3Exception* NewException (IM3Runtime io_runtime, IM3Tag i_tag, u32 i_അക്കം_Args)
{
    M3Exception* exception = (M3Exception*)m3_Malloc("M3Exception", വലുപ്പം(M3Exception) + i_അക്കം_Args * വലുപ്പം(u64));

    എങ്കിൽ (exception) {
        exception->tag     = i_tag;
        exception->അക്കം_Args = i_അക്കം_Args;
        exception->reified = false;
        exception->prev    = ശൂന്യ;
        exception->next    = io_runtime->exceptions;

        എങ്കിൽ (exception->next) {
            exception->next->prev = exception;
        }

        io_runtime->exceptions = exception;
    }

    തിരികെ exception;
}


// Releases one exception ahead of the rest. The caller has to know nothing can
// still name it: no exnref was ever taken of it, and its payload has already
// been copied out.
ശൂന്യം FreeException (IM3Runtime io_runtime, M3Exception* i_exception)
{
    എങ്കിൽ (i_exception->prev) {
        i_exception->prev->next = i_exception->next;
    } അല്ലെങ്കിൽ {
        io_runtime->exceptions = i_exception->next;
    }

    എങ്കിൽ (i_exception->next) {
        i_exception->next->prev = i_exception->prev;
    }

    എങ്കിൽ (io_runtime->pendingException == i_exception) {
        io_runtime->pendingException = ശൂന്യ;
    }

    m3_Free_Impl(i_exception);
}


// Releases every exception the runtime still holds. Only safe once the Wasm
// stack is empty, which is why the outermost RunCodeChecked() is the one that
// calls it.
ശൂന്യം FreeExceptions (IM3Runtime io_runtime)
{
    M3Exception* exception = io_runtime->exceptions;

    io_runtime->exceptions       = ശൂന്യ;
    io_runtime->pendingException = ശൂന്യ;

    ആകുമ്പോൾ (exception) {
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
സ്ഥിതം
ശൂന്യം RecordUncaughtException (IM3Runtime io_runtime, M3Result i_result)
{
    എങ്കിൽ (io_runtime->uncaughtException) {
        m3_Free(io_runtime->uncaughtException);
    }

    M3Exception* exception = io_runtime->pendingException;

    എങ്കിൽ (M3_UNLIKELY(i_result == m3Err_trapUncaughtException) and exception) {
        M3Exception* copy = (M3Exception*)m3_CopyMem(exception, വലുപ്പം(M3Exception) + exception->അക്കം_Args * വലുപ്പം(u64));

        എങ്കിൽ (copy) {
            copy->next = ശൂന്യ;
            copy->prev = ശൂന്യ;
        }

        io_runtime->uncaughtException = copy;
    }
}

#endif // d_m3HasExceptionHandling


#എങ്കിൽ d_m3HasExceptionHandling || d_m3_Has_അട്ടി_Switching

IM3Tag m3_FindTag (IM3Module i_module, സ്ഥിരം അക്ഷരം* സ്ഥിരം i_tagName)
{
    എങ്കിൽ (not i_module or not i_tagName) {
        തിരികെ ശൂന്യ;
    }

    തിരികെ Module_FindExportedTag(i_module, i_tagName);
}


M3Result m3_NewTag (IM3Runtime io_runtime, IM3Tag* o_tag, സ്ഥിരം അക്ഷരം* സ്ഥിരം i_signature)
{
    IM3FuncType type    = ശൂന്യ;
    M3HostTag*  hostTag = ശൂന്യ;

_try {
    _throwif("null tag", not o_tag);
    *o_tag = ശൂന്യ;
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
    തിരികെ result;
}


M3Result m3_LinkTag (IM3Module         io_module,
                     സ്ഥിരം അക്ഷരം* സ്ഥിരം i_moduleName,
                     സ്ഥിരം അക്ഷരം* സ്ഥിരം i_tagName,
                     IM3Tag            i_tag)
{
    എങ്കിൽ (not io_module or not i_moduleName or not i_tagName or not i_tag) {
        തിരികെ m3Err_tagLookupFailed;
    }

    സ്ഥിരം bool wildcardModule = (strcmp(i_moduleName, "*") == 0);
    IM3Tag     target         = i_tag->resolved ? i_tag->resolved : i_tag;

    M3Result result = m3Err_tagLookupFailed;

    വേണ്ടി (u32 i = 0; i < io_module->അക്കം_Tags; ++i) {
        IM3Tag tag = &io_module->tags[i];

        എങ്കിൽ (tag->imported and tag->import.moduleUtf8 and tag->import.fieldUtf8 and
            strcmp(tag->import.fieldUtf8, i_tagName) == 0 and
            (wildcardModule or strcmp(tag->import.moduleUtf8, i_moduleName) == 0)) {
            എങ്കിൽ (tag->type != target->type) {
                തിരികെ m3Err_incompatibleImportType;
            }

            tag->resolved = target;
            result        = m3Err_none;
        }
    }

    തിരികെ result;
}


uint32_t m3_Get_Tag_Arg_എണ്ണം (IM3Tag i_tag)
{
    തിരികെ i_tag ? i_tag->type->അക്കം_Args : 0;
}


M3_മൂല്യം_Type m3_GetTagArgType (IM3Tag i_tag, uint32_t i_index)
{
    എങ്കിൽ (i_tag and i_index < i_tag->type->അക്കം_Args) {
        തിരികെ (M3_മൂല്യം_Type)BaseTypeOf(d_FuncArgType(i_tag->type, i_index));
    }

    തിരികെ c_m3Type_none;
}

#അല്ലെങ്കിൽ

IM3Tag m3_FindTag (IM3Module i_module, സ്ഥിരം അക്ഷരം* സ്ഥിരം i_tagName)
{
    തിരികെ ശൂന്യ;
}

M3Result m3_NewTag (IM3Runtime io_runtime, IM3Tag* o_tag, സ്ഥിരം അക്ഷരം* സ്ഥിരം i_signature)
{
    എങ്കിൽ (o_tag) {
        *o_tag = ശൂന്യ;
    }
    തിരികെ m3Err_tagLookupFailed;
}

M3Result m3_LinkTag (IM3Module io_module, സ്ഥിരം അക്ഷരം* സ്ഥിരം i_moduleName, സ്ഥിരം അക്ഷരം* സ്ഥിരം i_tagName, IM3Tag i_tag)
{
    തിരികെ m3Err_tagLookupFailed;
}

uint32_t m3_Get_Tag_Arg_എണ്ണം (IM3Tag i_tag)
{
    തിരികെ 0;
}

M3_മൂല്യം_Type m3_GetTagArgType (IM3Tag i_tag, uint32_t i_index)
{
    തിരികെ c_m3Type_none;
}

#endif // d_m3HasExceptionHandling || d_m3HasStackSwitching


#എങ്കിൽ d_m3HasExceptionHandling

M3Result m3_ThrowException (IM3Runtime io_runtime, IM3Tag i_tag, uint32_t i_argc, സ്ഥിരം ശൂന്യം* i_argptrs[])
{
    എങ്കിൽ (not io_runtime or not i_tag) {
        തിരികെ m3Err_tagLookupFailed;
    }

    IM3Tag      tag  = i_tag->resolved ? i_tag->resolved : i_tag;
    IM3FuncType type = tag->type;

    // a tag with results is a stack switching control tag, which is suspended
    // with rather than thrown
    എങ്കിൽ (type->അക്കം_Rets) {
        തിരികെ m3Err_typeMismatch;
    }
    എങ്കിൽ (i_argc != type->അക്കം_Args) {
        തിരികെ m3_Err_argument_എണ്ണം_Mismatch;
    }
    // with nothing on the stack there is nothing to unwind, and the exception
    // would sit on the runtime's list until some later call ended
    എങ്കിൽ (io_runtime->callNesting == 0) {
        തിരികെ "m3_ThrowException called outside a host function";
    }

    M3Exception* exception = NewException(io_runtime, tag, i_argc);
    എങ്കിൽ (not exception) {
        തിരികെ m3Err_mallocFailed;
    }

    // the layout op_Throw gives a payload: 32-bit values zero-extended, floats
    // as their bit pattern
    വേണ്ടി (u32 i = 0; i < i_argc; ++i) {
        സ്ഥിരം ശൂന്യം* മൂല്യം = i_argptrs[i];

        തിരഞ്ഞെടുപ്പ് (BaseTypeOf(d_FuncArgType(type, i))) {
        അവസ്ഥ c_m3Type_i32:
        അവസ്ഥ c_m3Type_f32: {
            u32 bits;
            memcpy(&bits, മൂല്യം, വലുപ്പം(bits));
            exception->args[i] = bits;
            നിർത്തുക;
        }
        അവസ്ഥ c_m3Type_i64:
        അവസ്ഥ c_m3Type_f64:
            memcpy(&exception->args[i], മൂല്യം, വലുപ്പം(u64));
            നിർത്തുക;
        അവസ്ഥ c_m3Type_funcref:
        അവസ്ഥ c_m3Type_externref:
        അവസ്ഥ c_m3Type_exnref:
        അവസ്ഥ c_m3Type_contref:
            exception->args[i] = (u64)(*(സ്ഥിരം uintptr_t*)മൂല്യം);
            നിർത്തുക;
        സ്ഥിരസ്ഥിതി:
            FreeException(io_runtime, exception);
            തിരികെ "unknown argument type";
        }
    }

    io_runtime->pendingException = exception;

    തിരികെ m3Err_pendingException;
}


IM3Tag m3_GetExceptionTag (IM3Runtime i_runtime)
{
    എങ്കിൽ (i_runtime and i_runtime->uncaughtException) {
        തിരികെ i_runtime->uncaughtException->tag;
    }

    തിരികെ ശൂന്യ;
}


M3Result m3_GetExceptionArgs (IM3Runtime i_runtime, uint32_t i_argc, സ്ഥിരം ശൂന്യം* o_argptrs[])
{
    എങ്കിൽ (not i_runtime or not i_runtime->uncaughtException) {
        തിരികെ "no uncaught exception";
    }

    സ്ഥിരം M3Exception* exception = i_runtime->uncaughtException;
    IM3FuncType        type      = exception->tag->type;

    എങ്കിൽ (i_argc != exception->അക്കം_Args) {
        തിരികെ m3_Err_argument_എണ്ണം_Mismatch;
    }

    വേണ്ടി (u32 i = 0; i < i_argc; ++i) {
        ശൂന്യം* മൂല്യം = (ശൂന്യം*)o_argptrs[i];
        u64   bits  = exception->args[i];

        തിരഞ്ഞെടുപ്പ് (BaseTypeOf(d_FuncArgType(type, i))) {
        അവസ്ഥ c_m3Type_i32:
        അവസ്ഥ c_m3Type_f32: {
            u32 താഴെയുള്ള = (u32)bits;
            memcpy(മൂല്യം, &താഴെയുള്ള, വലുപ്പം(താഴെയുള്ള));
            നിർത്തുക;
        }
        അവസ്ഥ c_m3Type_i64:
        അവസ്ഥ c_m3Type_f64:
            memcpy(മൂല്യം, &bits, വലുപ്പം(bits));
            നിർത്തുക;
        അവസ്ഥ c_m3Type_funcref:
        അവസ്ഥ c_m3Type_externref:
        അവസ്ഥ c_m3Type_exnref:
        അവസ്ഥ c_m3Type_contref:
            *(uintptr_t*)മൂല്യം = (uintptr_t)bits;
            നിർത്തുക;
        സ്ഥിരസ്ഥിതി:
            തിരികെ "unknown argument type";
        }
    }

    തിരികെ m3Err_none;
}

#അല്ലെങ്കിൽ

M3Result m3_ThrowException (IM3Runtime io_runtime, IM3Tag i_tag, uint32_t i_argc, സ്ഥിരം ശൂന്യം* i_argptrs[])
{
    തിരികെ m3Err_tagLookupFailed;
}

IM3Tag m3_GetExceptionTag (IM3Runtime i_runtime)
{
    തിരികെ ശൂന്യ;
}

M3Result m3_GetExceptionArgs (IM3Runtime i_runtime, uint32_t i_argc, സ്ഥിരം ശൂന്യം* o_argptrs[])
{
    തിരികെ "no uncaught exception";
}

#endif // d_m3HasExceptionHandling


#എങ്കിൽ d_m3GuardedMemory

// What RunCodeProtected hands to the platform layer to run, and what comes back
തരനിർവചനം ഘടന M3ProtectedRun {
    pc_t            pc;
    IM3Runtime      runtime;
    M3MemoryHeader* mem;
    M3Result        result;
} M3ProtectedRun;

സ്ഥിതം
ശൂന്യം RunCodeBody (ശൂന്യം* io_context)
{
    M3ProtectedRun* run = (M3ProtectedRun*)io_context;

#  എങ്കിൽ (d_m3EnableOpProfiling || d_m3EnableOpTracing)
    run->result = (M3Result)RunCode(run->pc, (m3stack_t)run->runtime->അട്ടി, run->mem,
                                    d_m3OpDefaultArgs, d_m3BaseCstr);
#  അല്ലെങ്കിൽ
    run->result = (M3Result)RunCode(run->pc, (m3stack_t)run->runtime->അട്ടി, run->mem,
                                    d_m3OpDefaultArgs);
#  endif
}

// The bounds check the load and store operations no longer do, done once here by the
// system instead: an address past the end of a memory reaches the uncommitted part
// of that memory's reservation, and the fault comes back as the trap the spec asks
// for. Nothing is left half-done by it - the Wasm stack and the memories are heap,
// and the caller unwinds the rest.
സ്ഥിതം
M3Result RunCodeProtected (IM3Runtime i_runtime, pc_t i_pc, M3MemoryHeader* i_mem)
{
    // Memory operations in this build carry no bounds check, so code only runs where a
    // fault past the end of a memory can still be turned into a trap
    എങ്കിൽ (M3_UNLIKELY(not m3_HostGuardsActive())) {
        തിരികെ ErrorRuntime(m3Err_memoryGuardsUnavailable, i_runtime,
                            "another part of the process holds the fault signal");
    }

    M3ProtectedRun run;
    run.pc      = i_pc;
    run.runtime = i_runtime;
    run.mem     = i_mem;
    run.result  = m3Err_none;

    ശൂന്യം*  arena;
    size_t arenaBytes;
    Guard_ArenaRange(&arena, &arenaBytes);

    എങ്കിൽ (not m3_HostProtectedCall(RunCodeBody, &run, arena, arenaBytes)) {
        തിരികെ m3Err_trapOutOfBoundsMemoryAccess;
    }

    തിരികെ run.result;
}

#endif // d_m3GuardedMemory


#എങ്കിൽ d_m3_Has_അട്ടി_Switching

// The function a suspended continuation stopped in: the callee of its innermost
// call frame, or the function it was entered with if it made no call. Frames are
// recorded innermost first, so that is the lowest-numbered call frame.
സ്ഥിതം
IM3Function Continuation_InnermostFunction (IM3Continuation i_cont)
{
    വേണ്ടി (u32 i = 0; i < i_cont->അക്കം_Frames; ++i) {
        എങ്കിൽ (i_cont->frames[i].kind == frame_call && i_cont->frames[i].call.function) {
            തിരികെ i_cont->frames[i].call.function;
        }
    }

    തിരികെ i_cont->entryFunction;
}


// Where the host's next call puts its arguments and finds its results. A
// suspended root keeps its frames on the bottom of the runtime's stack, so a call
// made before it is resumed starts above the last of them rather than on top of
// them. The top is the innermost function's frame base plus everything that
// function can use. That base is the root's own sp, unless it stopped inside a
// resume: then the suspension point belongs to the resumed continuation, and the
// root's innermost frame is that resume.
ശൂന്യം Runtime_Place_Call_അട്ടി (IM3Runtime io_runtime)
{
    IM3Continuation വേര് = io_runtime->വേര്_Continuation;

    io_runtime->അട്ടി = io_runtime->origin_അട്ടി;

    എങ്കിൽ (not വേര് or വേര്->state != cont_suspended) {
        തിരികെ;
    }

    m3stack_t   sp       = വേര്->sp;
    IM3Function function = Continuation_InnermostFunction(വേര്);

    എങ്കിൽ (വേര്->അക്കം_Frames and വേര്->frames[0].kind == frame_resume) {
        sp = വേര്->frames[0].sp;
    }

    m3slot_t* base = (m3slot_t*)io_runtime->origin_അട്ടി;
    m3slot_t* മുകൾ  = (m3slot_t*)sp + (function ? function->max_അട്ടി_Slots : 0);

    എങ്കിൽ (മുകൾ > base + io_runtime->അക്കം_അട്ടി_Slots) {
        മുകൾ = base + io_runtime->അക്കം_അട്ടി_Slots;
    }

    io_runtime->അട്ടി = മുകൾ;
}

#endif // d_m3HasStackSwitching


// Run compiled code on the runtime's stack, bounding native recursion for the
// duration of the call. The outermost invocation establishes the stack limit;
// nested ones (an imported function calling back into Wasm) inherit it.
സ്ഥിതം വരിയിൽ
M3Result RunCodeChecked (IM3Runtime i_runtime, IM3Function i_function)
{
    pc_t i_pc = i_function->compiled;

    // execution runs against the memory of the module the entry point belongs
    // to, not against some runtime-wide one
    M3MemoryHeader* _mem = Module_MemoryHeader(i_function->module);

#എങ്കിൽ d_m3HasSnapshots
    എങ്കിൽ (M3_UNLIKELY(i_function->module->isUnusable)) {
        തിരികെ "a snapshot failed to restore into the module, so it cannot run";
    }
    i_function->module->hasRun = true;
#endif

    d_m3_അട്ടി_Limit_Enter(i_runtime);

    // a host function calling back into Wasm does not change which call a trap
    // ends: that is still the one the host made first
    എങ്കിൽ (i_runtime->callNesting++ == 0) {
        i_runtime->entered = i_function;
    }
#എങ്കിൽ d_m3HasExceptionHandling
    // handler stacks don't nest across a call boundary: a host function calling
    // back into Wasm cannot be caught by a try_table its own caller entered
    u32 savedTryDepth   = i_runtime->tryDepth;
    i_runtime->tryDepth = 0;
#endif
#എങ്കിൽ d_m3_Has_അട്ടി_Switching
    // nor do continuations: the native frames on the far side of a host call
    // are not ours to record, so a suspend in here has no handler to reach and
    // no resume to unwind to
    IM3Continuation savedContinuation = i_runtime->activeContinuation;

    // A suspended root continuation is a paused program waiting to be resumed,
    // not scratch space. A call the host makes before resuming it gets a
    // context of its own, the way any other re-entry does, rather than having
    // the frames of the suspension reset out from under it.
    bool claim_വേര് = i_runtime->isSuspendable and not savedContinuation and
                     not (i_runtime->വേര്_Continuation and
                          i_runtime->വേര്_Continuation->state == cont_suspended);

    എങ്കിൽ (claim_വേര് and not i_runtime->വേര്_Continuation) {
        i_runtime->വേര്_Continuation = Continuation_New(i_runtime, i_function->funcType, i_function);

        എങ്കിൽ (i_runtime->വേര്_Continuation) {
            // The root borrows the runtime's stack and is not charged as cont.new.
            i_runtime->വേര്_Continuation->അക്കം_അട്ടി_Slots = i_runtime->അക്കം_അട്ടി_Slots;
        } അല്ലെങ്കിൽ {
            claim_വേര് = false;
        }
    }

    എങ്കിൽ (claim_വേര്) {
        i_runtime->activeContinuation              = i_runtime->വേര്_Continuation;
        i_runtime->വേര്_Continuation->entryFunction = i_function;
        i_runtime->വേര്_Continuation->state         = cont_running;
        i_runtime->വേര്_Continuation->അക്കം_Frames     = 0;
    } അല്ലെങ്കിൽ {
        i_runtime->activeContinuation = ശൂന്യ;
    }
#endif
#എങ്കിൽ d_m3GuardedMemory
    M3Result result = RunCodeProtected(i_runtime, i_pc, _mem);
#അല്ലെങ്കിലെങ്കിൽ (d_m3EnableOpProfiling || d_m3EnableOpTracing)
    M3Result result = (M3Result)RunCode(i_pc, (m3stack_t)i_runtime->അട്ടി, _mem, d_m3OpDefaultArgs, d_m3BaseCstr);
#അല്ലെങ്കിൽ
    M3Result result = (M3Result)RunCode(i_pc, (m3stack_t)i_runtime->അട്ടി, _mem, d_m3OpDefaultArgs);
#endif
#എങ്കിൽ d_m3_Has_അട്ടി_Switching
    എങ്കിൽ (claim_വേര്) {
        i_runtime->വേര്_Continuation->state =
          (result == m3Err_continuationSuspended) ? cont_suspended : cont_returned;

        Runtime_Place_Call_അട്ടി(i_runtime);
    }
    i_runtime->activeContinuation = savedContinuation;
#endif
#എങ്കിൽ d_m3HasExceptionHandling
    i_runtime->tryDepth = savedTryDepth;

    // an exception that reached the bottom of the call stack found no handler
    എങ്കിൽ (M3_UNLIKELY(result == m3Err_pendingException)) {
        result = m3Err_trapUncaughtException;
    }
    RecordUncaughtException(i_runtime, result);

    // a paused invocation can still reach the exceptions it holds, so they wait
    // for it to finish
    എങ്കിൽ (i_runtime->callNesting == 1 and not m3_IsSuspended(i_runtime)) {
        FreeExceptions(i_runtime);
    }
#endif
    i_runtime->callNesting--;
    d_m3_അട്ടി_Limit_Leave(i_runtime);

    തിരികെ result;
}

M3Result m3_RunStart (IM3Module io_module)
{
#ifdef FUZZING_BUILD_MODE_UNSAFE_FOR_PRODUCTION
    // Execution disabled for fuzzing builds
    തിരികെ m3Err_none;
#endif

    M3Result result           = m3Err_none;
    i32      startFunctionTmp = -1;

    എങ്കിൽ (io_module and io_module->startFunction >= 0) {
        IM3Function function = &io_module->functions[io_module->startFunction];

        എങ്കിൽ (not function->compiled) {
_           (CompileFunction(function));
        }

        IM3FuncType ftype = function->funcType;
        എങ്കിൽ (ftype->അക്കം_Args != 0 || ftype->അക്കം_Rets != 0) {
            _throw(m3_Err_argument_എണ്ണം_Mismatch);
        }

        IM3Module  module  = function->module;
        IM3Runtime runtime = module->runtime;

        startFunctionTmp = io_module->startFunction;

        io_module->startFunction = -1;

        result = RunCodeChecked(runtime, function);

        // a start function that suspended is not to be started again: resuming
        // is what finishes it
        എങ്കിൽ (result and result != m3Err_continuationSuspended) {
            io_module->startFunction = startFunctionTmp;
            EXCEPTION_PRINT(result);
            പോകുക _catch;
        }
    }

    _catch: തിരികെ result;
}

// TODO: deal with main + side-modules loading efforcement
M3Result m3_LoadModule (IM3Runtime io_runtime, IM3Module io_module)
{
    M3Result result = m3Err_none;

    എങ്കിൽ (M3_UNLIKELY(io_module->runtime)) {
        തിരികെ m3Err_moduleAlreadyLinked;
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
    തിരികെ result; // ok

_catch:
    // The runtime owns the module either way. Instantiation may already have
    // written this module's functions into a table another module owns, and the
    // spec keeps whatever it managed to do before the trap, so those entries
    // stay callable - retaining the module is what stops them dangling.
    //
    // It goes on the tail of the list rather than the head: it is not a module
    // anyone should find by name, only one whose functions may still be
    // reachable through someone else's table.
    io_module->next = ശൂന്യ;

    IM3Module* tail = &io_runtime->modules;
    ആകുമ്പോൾ (*tail) {
        tail = &(*tail)->next;
    }
    *tail = io_module;

    തിരികെ result;
}

IM3Global m3_FindGlobal (IM3Module         io_module,
                         സ്ഥിരം അക്ഷരം* സ്ഥിരം i_globalName)
{
    // Search exports
    IM3Global exported = Module_FindExportedGlobal(io_module, i_globalName);
    എങ്കിൽ (exported) {
        // a re-exported global is the one that was imported, so reads and
        // writes have to reach the cell that actually holds the value
        തിരികെ exported->resolved ? exported->resolved : exported;
    }

    // Search imports
    വേണ്ടി (u32 i = 0; i < io_module->അക്കം_Globals; ++i) {
        IM3Global g = &io_module->globals[i];

        എങ്കിൽ (g->import.moduleUtf8 and g->import.fieldUtf8) {
            എങ്കിൽ (strcmp(g->import.fieldUtf8, i_globalName) == 0) {
                തിരികെ g->resolved ? g->resolved : g;
            }
        }
    }
    തിരികെ ശൂന്യ;
}

M3Result m3_GetGlobal (IM3Global      i_global,
                       IM3_Tagged_മൂല്യം o_മൂല്യം)
{
    എങ്കിൽ (not i_global) {
        തിരികെ m3Err_globalLookupFailed;
    }

    തിരഞ്ഞെടുപ്പ് (i_global->type) {
    അവസ്ഥ c_m3Type_i32: o_മൂല്യം->മൂല്യം.i32 = i_global->i32_മൂല്യം; നിർത്തുക;
    അവസ്ഥ c_m3Type_i64: o_മൂല്യം->മൂല്യം.i64 = i_global->i64_മൂല്യം; നിർത്തുക;
#എങ്കിൽ d_m3HasFloat
    അവസ്ഥ c_m3Type_f32: o_മൂല്യം->മൂല്യം.f32 = i_global->f32_മൂല്യം; നിർത്തുക;
    അവസ്ഥ c_m3Type_f64: o_മൂല്യം->മൂല്യം.f64 = i_global->f64_മൂല്യം; നിർത്തുക;
#endif
    സ്ഥിരസ്ഥിതി: തിരികെ m3Err_invalidTypeId;
    }

    o_മൂല്യം->type = (M3_മൂല്യം_Type)(i_global->type);
    തിരികെ m3Err_none;
}

M3Result m3_SetGlobal (IM3Global            i_global,
                       സ്ഥിരം IM3_Tagged_മൂല്യം i_മൂല്യം)
{
    എങ്കിൽ (not i_global) {
        തിരികെ m3Err_globalLookupFailed;
    }
    എങ്കിൽ (not i_global->isMutable) {
        തിരികെ m3Err_globalNotMutable;
    }
    എങ്കിൽ (i_global->type != i_മൂല്യം->type) {
        തിരികെ m3Err_globalTypeMismatch;
    }

    തിരഞ്ഞെടുപ്പ് (i_മൂല്യം->type) {
    അവസ്ഥ c_m3Type_i32: i_global->i32_മൂല്യം = i_മൂല്യം->മൂല്യം.i32; നിർത്തുക;
    അവസ്ഥ c_m3Type_i64: i_global->i64_മൂല്യം = i_മൂല്യം->മൂല്യം.i64; നിർത്തുക;
#എങ്കിൽ d_m3HasFloat
    അവസ്ഥ c_m3Type_f32: i_global->f32_മൂല്യം = i_മൂല്യം->മൂല്യം.f32; നിർത്തുക;
    അവസ്ഥ c_m3Type_f64: i_global->f64_മൂല്യം = i_മൂല്യം->മൂല്യം.f64; നിർത്തുക;
#endif
    സ്ഥിരസ്ഥിതി: തിരികെ m3Err_invalidTypeId;
    }

    തിരികെ m3Err_none;
}

M3_മൂല്യം_Type m3_GetGlobalType (IM3Global i_global)
{
    തിരികെ (i_global) ? (M3_മൂല്യം_Type)(i_global->type) : c_m3Type_none;
}


ശൂന്യം* v_FindFunction (IM3Module i_module, ശൂന്യം* i_info)
{
    സ്ഥിരം അക്ഷരം* സ്ഥിരം i_name = (സ്ഥിരം അക്ഷരം*)i_info;

    // Prefer exported functions
    IM3Function exported = Module_FindExportedFunction(i_module, i_name);
    എങ്കിൽ (exported) {
        തിരികെ exported;
    }

    // Search internal functions
    വേണ്ടി (u32 i = 0; i < i_module->അക്കം_Functions; ++i) {
        IM3Function f = &i_module->functions[i];

        bool isImported = f->import.moduleUtf8 or f->import.fieldUtf8;

        എങ്കിൽ (isImported) {
            തുടരുക;
        }

        വേണ്ടി (പൂർണ്ണസംഖ്യ j = 0; j < f->അക്കം_Names; j++) {
            എങ്കിൽ (f->names[j] and strcmp(f->names[j], i_name) == 0) {
                തിരികെ f;
            }
        }
    }

    തിരികെ ശൂന്യ;
}


// Shared tail of the two lookups: a function is only usable once it has code.
സ്ഥിതം
M3Result PrepareFoundFunction (IM3Function* o_function, IM3Function i_function)
{
    M3Result result = m3Err_none;

    എങ്കിൽ (not i_function->compiled) {
_       (CompileFunction(i_function))
    }

    _catch:
    *o_function = result ? ശൂന്യ : i_function;

    തിരികെ result;
}


// Searches every module in the runtime, most recently loaded first. That is a
// guess once more than one module is loaded and two of them export the same
// name - m3_FindFunctionIn says which module is meant.
M3Result m3_FindFunction (IM3Function* o_function, IM3Runtime i_runtime, സ്ഥിരം അക്ഷരം* സ്ഥിരം i_functionName)
{
                                                                d_m3Assert (o_function and i_runtime and i_functionName);
    IM3Function function = ശൂന്യ;

    എങ്കിൽ (not i_runtime->modules) {
        *o_function = ശൂന്യ;
        തിരികെ "no modules loaded";
    }

    function = (IM3Function)ForEachModule(i_runtime, v_FindFunction, (ശൂന്യം*)i_functionName);

    എങ്കിൽ (not function) {
        *o_function = ശൂന്യ;
        തിരികെ ErrorModule(m3Err_functionLookupFailed, i_runtime->modules, "'%s'", i_functionName);
    }

    തിരികെ PrepareFoundFunction(o_function, function);
}


IM3Module m3_FindModule (IM3Runtime i_runtime, സ്ഥിരം അക്ഷരം* സ്ഥിരം i_moduleName)
{
    എങ്കിൽ (not i_runtime or not i_moduleName) {
        തിരികെ ശൂന്യ;
    }

    // the list is newest-first, so a name registered twice names the newer one
    വേണ്ടി (IM3Module m = i_runtime->modules; m; m = m->next) {
        എങ്കിൽ (m->name and strcmp(m->name, i_moduleName) == 0) {
            തിരികെ m;
        }
    }

    തിരികെ ശൂന്യ;
}


// Searches one module's exports, which is what naming a module means.
M3Result m3_FindFunctionIn (IM3Function* o_function, IM3Module i_module, സ്ഥിരം അക്ഷരം* സ്ഥിരം i_functionName)
{
                                                                d_m3Assert (o_function and i_functionName);
    എങ്കിൽ (not i_module) {
        *o_function = ശൂന്യ;
        തിരികെ m3Err_functionLookupFailed;      // ErrorModule would deref it
    }

    IM3Function function = (IM3Function)v_FindFunction(i_module, (ശൂന്യം*)i_functionName);

    എങ്കിൽ (not function) {
        *o_function = ശൂന്യ;
        തിരികെ ErrorModule(m3Err_functionLookupFailed, i_module, "'%s'", i_functionName);
    }

    തിരികെ PrepareFoundFunction(o_function, function);
}


M3Result m3_GetTableFunction (IM3Function* o_function, IM3Module i_module, uint32_t i_index)
{
_try {
    M3Table*    table;
    IM3Function function;

    _throwif("no table", i_module->അക്കം_Tables == 0);

    table = i_module->tables[0];
    _throwif("function index out of range", i_index >= table->size);

    function = (IM3Function)table->elements[i_index];

    എങ്കിൽ (function) {
        എങ്കിൽ (not function->compiled) {
_           (CompileFunction(function))
        }
    }

    *o_function = function;
} _catch:
    തിരികെ result;
}


സ്ഥിതം
M3Result checkStartFunction (IM3Module i_module)
{
    M3Result result = m3Err_none;                               d_m3Assert(i_module);

    // Check if start function needs to be called
    എങ്കിൽ (i_module->startFunction >= 0) {
        result = m3_RunStart(i_module);
    }

    തിരികെ result;
}

uint32_t m3_Get_Arg_എണ്ണം (IM3Function i_function)
{
    എങ്കിൽ (i_function) {
        IM3FuncType ft = i_function->funcType;
        എങ്കിൽ (ft) {
            തിരികെ ft->അക്കം_Args;
        }
    }
    തിരികെ 0;
}

uint32_t m3_Get_Ret_എണ്ണം (IM3Function i_function)
{
    എങ്കിൽ (i_function) {
        IM3FuncType ft = i_function->funcType;
        എങ്കിൽ (ft) {
            തിരികെ ft->അക്കം_Rets;
        }
    }
    തിരികെ 0;
}


M3_മൂല്യം_Type m3_GetArgType (IM3Function i_function, uint32_t index)
{
    എങ്കിൽ (i_function) {
        IM3FuncType ft = i_function->funcType;
        എങ്കിൽ (ft and index < ft->അക്കം_Args) {
            തിരികെ (M3_മൂല്യം_Type)BaseTypeOf(d_FuncArgType(ft, index));
        }
    }
    തിരികെ c_m3Type_none;
}

M3_മൂല്യം_Type m3_GetRetType (IM3Function i_function, uint32_t index)
{
    എങ്കിൽ (i_function) {
        IM3FuncType ft = i_function->funcType;
        എങ്കിൽ (ft and index < ft->അക്കം_Rets) {
            തിരികെ (M3_മൂല്യം_Type)BaseTypeOf(d_FuncRetType(ft, index));
        }
    }
    തിരികെ c_m3Type_none;
}


// The call APIs marshal their arguments into the runtime stack before any
// compiled code runs, so op_Entry's overflow check comes too late to cover those
// writes - and for a host function or an import linked to another module there
// is no op_Entry of our own to reach at all. Bound the area about to be written
// here instead: results and arguments each take a u64 at this boundary, however
// wide the slot is inside. Every caller goes through this, so none of them can
// be the one that forgets.
സ്ഥിതം
M3Result Get_അട്ടി_Pointer_For_Args (IM3Function i_function, u8** o_അട്ടി)
{
    IM3Runtime  runtime = i_function->module->runtime;
    IM3FuncType ftype   = i_function->funcType;

    // the stack a paused invocation holds is not available to this one
    size_t used      = (size_t)((u8*)runtime->അട്ടി - (u8*)runtime->origin_അട്ടി);
    size_t needed    = ((size_t)ftype->അക്കം_Rets + ftype->അക്കം_Args) * വലുപ്പം(u64);
    size_t available = (size_t)runtime->അക്കം_അട്ടി_Slots * വലുപ്പം(m3slot_t) - used;

    എങ്കിൽ (needed > available) {
        തിരികെ m3_Err_trap_അട്ടി_Overflow;
    }

    u64* അട്ടി = (u64*)runtime->അട്ടി;

    അട്ടി += ftype->അക്കം_Rets;

    *o_അട്ടി = (u8*)അട്ടി;

    തിരികെ m3Err_none;
}


M3Result m3_CallV (IM3Function i_function, ...)
{
    va_list ap;
    va_start(ap, i_function);
    M3Result r = m3_CallVL(i_function, ap);
    va_end(ap);
    തിരികെ r;
}

സ്ഥിതം
ശൂന്യം Report_Native_അട്ടി_Usage ()
{
#എങ്കിൽ d_m3_Log_Native_അട്ടി
    പൂർണ്ണസംഖ്യ അട്ടി_Used = m3_അട്ടി_Get_Max();
    fprintf(stderr, "Native stack used: %d\n", അട്ടി_Used);
#endif
}


M3Result m3_CallVL (IM3Function i_function, va_list i_args)
{
    IM3Runtime  runtime = i_function->module->runtime;
    IM3FuncType ftype   = i_function->funcType;
    M3Result    result  = m3Err_none;
    u8*         s       = ശൂന്യ;

    എങ്കിൽ (!i_function->compiled) {
        തിരികെ m3Err_missingCompiledCode;
    }

#എങ്കിൽ d_m3RecordBacktraces
    ClearBacktrace(runtime);
#endif

    m3_അട്ടി_Check_Init();

_   (checkStartFunction(i_function->module))

_   (Get_അട്ടി_Pointer_For_Args(i_function, &s))

    വേണ്ടി (u32 i = 0; i < ftype->അക്കം_Args; ++i) {
        തിരഞ്ഞെടുപ്പ് (d_FuncArgType(ftype, i)) {
        അവസ്ഥ c_m3Type_i32:
            *(i32*)(s) = va_arg(i_args, i32);
            s += 8;
            നിർത്തുക;
        അവസ്ഥ c_m3Type_i64:
            *(i64*)(s) = va_arg(i_args, i64);
            s += 8;
            നിർത്തുക;
        അവസ്ഥ c_m3Type_funcref:
        അവസ്ഥ c_m3Type_externref:
        അവസ്ഥ c_m3Type_exnref:
            *(uintptr_t*)(s) = va_arg(i_args, uintptr_t);
            s += 8;
            നിർത്തുക;
#എങ്കിൽ d_m3HasFloat
        അവസ്ഥ c_m3Type_f32:
            *(f32*)(s) = (f32)va_arg(i_args, f64);
            s += 8;
            നിർത്തുക; // f32 is passed as f64
        അവസ്ഥ c_m3Type_f64:
            *(f64*)(s) = va_arg(i_args, f64);
            s += 8;
            നിർത്തുക;
#endif
        സ്ഥിരസ്ഥിതി: തിരികെ "unknown argument type";
        }
    }

    result = RunCodeChecked(runtime, i_function);
    Report_Native_അട്ടി_Usage();

    runtime->lastCalled = result ? ശൂന്യ : i_function;

    _catch: തിരികെ result;
}

M3Result m3_Call (IM3Function i_function, uint32_t i_argc, സ്ഥിരം ശൂന്യം* i_argptrs[])
{
    IM3Runtime  runtime = i_function->module->runtime;
    IM3FuncType ftype   = i_function->funcType;
    M3Result    result  = m3Err_none;
    u8*         s       = ശൂന്യ;

    എങ്കിൽ (i_argc != ftype->അക്കം_Args) {
        തിരികെ m3_Err_argument_എണ്ണം_Mismatch;
    }
    എങ്കിൽ (!i_function->compiled) {
        തിരികെ m3Err_missingCompiledCode;
    }

#എങ്കിൽ d_m3RecordBacktraces
    ClearBacktrace(runtime);
#endif

    m3_അട്ടി_Check_Init();

_   (checkStartFunction(i_function->module))

_   (Get_അട്ടി_Pointer_For_Args(i_function, &s))

    വേണ്ടി (u32 i = 0; i < ftype->അക്കം_Args; ++i) {
        തിരഞ്ഞെടുപ്പ് (d_FuncArgType(ftype, i)) {
        അവസ്ഥ c_m3Type_i32:
            *(i32*)(s) = *(i32*)i_argptrs[i];
            s += 8;
            നിർത്തുക;
        അവസ്ഥ c_m3Type_i64:
            *(i64*)(s) = *(i64*)i_argptrs[i];
            s += 8;
            നിർത്തുക;
        അവസ്ഥ c_m3Type_funcref:
        അവസ്ഥ c_m3Type_externref:
        അവസ്ഥ c_m3Type_exnref:
            *(uintptr_t*)(s) = *(uintptr_t*)i_argptrs[i];
            s += 8;
            നിർത്തുക;
#എങ്കിൽ d_m3HasFloat
        അവസ്ഥ c_m3Type_f32:
            *(f32*)(s) = *(f32*)i_argptrs[i];
            s += 8;
            നിർത്തുക;
        അവസ്ഥ c_m3Type_f64:
            *(f64*)(s) = *(f64*)i_argptrs[i];
            s += 8;
            നിർത്തുക;
#endif
        സ്ഥിരസ്ഥിതി: തിരികെ "unknown argument type";
        }
    }

    result = RunCodeChecked(runtime, i_function);

    Report_Native_അട്ടി_Usage();

    runtime->lastCalled = result ? ശൂന്യ : i_function;

    _catch: തിരികെ result;
}

// Argument parsing for m3_CallArgv. Strict on purpose: the whole string has to
// be consumed, so "12abc" is rejected rather than read as 12, and an empty or
// unparsable argument is an error rather than the zero that strtoul with a
// NULL end pointer used to hand back. See wasm3/wasm3#367.
സ്ഥിതം
M3Result ParseArgInteger (ccstr_t i_arg, u32 i_അക്കം_Bits, u64* o_മൂല്യം)
{
    എങ്കിൽ (not i_arg or not *i_arg) {
        തിരികെ "empty argument";
    }

    // strtoull would skip leading space, but trailing space is rejected below;
    // accepting one and not the other would just be confusing
    എങ്കിൽ (isspace((unsigned അക്ഷരം)*i_arg)) {
        തിരികെ "argument is not a number";
    }

    അക്ഷരം* end = ശൂന്യ;
    u64   മൂല്യം;

    errno = 0;

    // an argument may be spelled signed or unsigned: -1 and 4294967295 name the
    // same i32
    എങ്കിൽ (*i_arg == '-') {
        i64 signed_മൂല്യം = strtoll(i_arg, &end, 10);

        എങ്കിൽ (i_അക്കം_Bits == 32 and (signed_മൂല്യം < INT32_MIN or signed_മൂല്യം > INT32_MAX)) {
            തിരികെ "argument out of range";
        }

        മൂല്യം = (u64)signed_മൂല്യം;
    } അല്ലെങ്കിൽ {
        മൂല്യം = strtoull(i_arg, &end, 10);

        എങ്കിൽ (i_അക്കം_Bits == 32 and മൂല്യം > UINT32_MAX) {
            തിരികെ "argument out of range";
        }
    }

    എങ്കിൽ (errno == ERANGE) {
        തിരികെ "argument out of range";
    }

    എങ്കിൽ (end == i_arg or *end) {
        തിരികെ "argument is not a number";
    }

    *o_മൂല്യം = മൂല്യം;

    തിരികെ m3Err_none;
}


#എങ്കിൽ d_m3HasFloat
സ്ഥിതം
M3Result ParseArgFloat (ccstr_t i_arg, f64* o_മൂല്യം)
{
    എങ്കിൽ (not i_arg or not *i_arg) {
        തിരികെ "empty argument";
    }

    എങ്കിൽ (isspace((unsigned അക്ഷരം)*i_arg)) {
        തിരികെ "argument is not a number";
    }

    അക്ഷരം* end = ശൂന്യ;

    errno = 0;

    f64 മൂല്യം = strtod(i_arg, &end);

    എങ്കിൽ (end == i_arg or *end) {
        തിരികെ "argument is not a number";
    }

    // strtod reports underflow through ERANGE as well, and a denormal result is
    // perfectly usable, so only an overflow to infinity is out of range
    എങ്കിൽ (errno == ERANGE and (മൂല്യം > DBL_MAX or മൂല്യം < -DBL_MAX)) {
        തിരികെ "argument out of range";
    }

    *o_മൂല്യം = മൂല്യം;

    തിരികെ m3Err_none;
}
#endif


// A reference argument is either the null reference or a host handle written as
// an integer.
സ്ഥിതം
M3Result ParseArgReference (ccstr_t i_arg, u64* o_മൂല്യം)
{
    എങ്കിൽ (i_arg and strcmp(i_arg, "null") == 0) {
        *o_മൂല്യം = 0;
        തിരികെ m3Err_none;
    }

    തിരികെ ParseArgInteger(i_arg, 64, o_മൂല്യം);
}


M3Result m3_CallArgv (IM3Function i_function, uint32_t i_argc, സ്ഥിരം അക്ഷരം* i_argv[])
{
    IM3FuncType ftype   = i_function->funcType;
    IM3Runtime  runtime = i_function->module->runtime;
    M3Result    result  = m3Err_none;
    u8*         s       = ശൂന്യ;

    എങ്കിൽ (i_argc != ftype->അക്കം_Args) {
        തിരികെ m3_Err_argument_എണ്ണം_Mismatch;
    }
    എങ്കിൽ (!i_function->compiled) {
        തിരികെ m3Err_missingCompiledCode;
    }

#എങ്കിൽ d_m3RecordBacktraces
    ClearBacktrace(runtime);
#endif

    m3_അട്ടി_Check_Init();

_   (checkStartFunction(i_function->module))

_   (Get_അട്ടി_Pointer_For_Args(i_function, &s))

    വേണ്ടി (u32 i = 0; i < ftype->അക്കം_Args; ++i) {
        u64 മൂല്യം = 0;
#എങ്കിൽ d_m3HasFloat
        f64 fvalue = 0;
#endif
        തിരഞ്ഞെടുപ്പ് (d_FuncArgType(ftype, i)) {
        അവസ്ഥ c_m3Type_i32:
_           (ParseArgInteger(i_argv[i], 32, &മൂല്യം)) * (i32*)(s) = (i32)മൂല്യം;
            s += 8;
            നിർത്തുക;
        അവസ്ഥ c_m3Type_i64:
_           (ParseArgInteger(i_argv[i], 64, &മൂല്യം)) * (i64*)(s) = (i64)മൂല്യം;
            s += 8;
            നിർത്തുക;
        അവസ്ഥ c_m3Type_funcref:
        അവസ്ഥ c_m3Type_externref:
        അവസ്ഥ c_m3Type_exnref:
_           (ParseArgReference(i_argv[i], &മൂല്യം)) * (uintptr_t*)(s) = (uintptr_t)മൂല്യം;
            s += 8;
            നിർത്തുക;
#എങ്കിൽ d_m3HasFloat
            // strtof would be less portable
        അവസ്ഥ c_m3Type_f32:
_           (ParseArgFloat(i_argv[i], &fvalue)) * (f32*)(s) = (f32)fvalue;
            s += 8;
            നിർത്തുക;
        അവസ്ഥ c_m3Type_f64:
_           (ParseArgFloat(i_argv[i], &fvalue)) * (f64*)(s) = fvalue;
            s += 8;
            നിർത്തുക;
#endif
        സ്ഥിരസ്ഥിതി: _throw("unknown argument type");
        }
    }

    result = RunCodeChecked(runtime, i_function);

    Report_Native_അട്ടി_Usage();

    runtime->lastCalled = result ? ശൂന്യ : i_function;

    _catch: തിരികെ result;
}


//u8 * AlignStackPointerTo64Bits (const u8 * i_stack)
//{
//    uintptr_t ptr = (uintptr_t) i_stack;
//    return (u8 *) ((ptr + 7) & ~7);
//}


M3Result m3_GetResults (IM3Function i_function, uint32_t i_retc, സ്ഥിരം ശൂന്യം* o_retptrs[])
{
    IM3FuncType ftype   = i_function->funcType;
    IM3Runtime  runtime = i_function->module->runtime;

    എങ്കിൽ (i_retc != ftype->അക്കം_Rets) {
        തിരികെ m3_Err_argument_എണ്ണം_Mismatch;
    }
    എങ്കിൽ (i_function != runtime->lastCalled) {
        തിരികെ "function not called";
    }

    u8* s = (u8*)runtime->അട്ടി;

    വേണ്ടി (u32 i = 0; i < ftype->അക്കം_Rets; ++i) {
        തിരഞ്ഞെടുപ്പ് (d_FuncRetType(ftype, i)) {
        അവസ്ഥ c_m3Type_i32:
            *(i32*)o_retptrs[i] = *(i32*)(s);
            s += 8;
            നിർത്തുക;
        അവസ്ഥ c_m3Type_i64:
            *(i64*)o_retptrs[i] = *(i64*)(s);
            s += 8;
            നിർത്തുക;
        അവസ്ഥ c_m3Type_funcref:
        അവസ്ഥ c_m3Type_externref:
        അവസ്ഥ c_m3Type_exnref:
            *(uintptr_t*)o_retptrs[i] = *(uintptr_t*)(s);
            s += 8;
            നിർത്തുക;
#എങ്കിൽ d_m3HasFloat
        അവസ്ഥ c_m3Type_f32:
            *(f32*)o_retptrs[i] = *(f32*)(s);
            s += 8;
            നിർത്തുക;
        അവസ്ഥ c_m3Type_f64:
            *(f64*)o_retptrs[i] = *(f64*)(s);
            s += 8;
            നിർത്തുക;
#endif
        സ്ഥിരസ്ഥിതി: തിരികെ "unknown return type";
        }
    }
    തിരികെ m3Err_none;
}

M3Result m3_GetResultsV (IM3Function i_function, ...)
{
    va_list ap;
    va_start(ap, i_function);
    M3Result r = m3_GetResultsVL(i_function, ap);
    va_end(ap);
    തിരികെ r;
}

M3Result m3_GetResultsVL (IM3Function i_function, va_list o_rets)
{
    IM3Runtime  runtime = i_function->module->runtime;
    IM3FuncType ftype   = i_function->funcType;

    എങ്കിൽ (i_function != runtime->lastCalled) {
        തിരികെ "function not called";
    }

    u8* s = (u8*)runtime->അട്ടി;
    വേണ്ടി (u32 i = 0; i < ftype->അക്കം_Rets; ++i) {
        തിരഞ്ഞെടുപ്പ് (d_FuncRetType(ftype, i)) {
        അവസ്ഥ c_m3Type_i32:
            *va_arg(o_rets, i32*) = *(i32*)(s);
            s += 8;
            നിർത്തുക;
        അവസ്ഥ c_m3Type_i64:
            *va_arg(o_rets, i64*) = *(i64*)(s);
            s += 8;
            നിർത്തുക;
        അവസ്ഥ c_m3Type_funcref:
        അവസ്ഥ c_m3Type_externref:
        അവസ്ഥ c_m3Type_exnref:
            *va_arg(o_rets, uintptr_t*) = *(uintptr_t*)(s);
            s += 8;
            നിർത്തുക;
#എങ്കിൽ d_m3HasFloat
        അവസ്ഥ c_m3Type_f32:
            *va_arg(o_rets, f32*) = *(f32*)(s);
            s += 8;
            നിർത്തുക;
        അവസ്ഥ c_m3Type_f64:
            *va_arg(o_rets, f64*) = *(f64*)(s);
            s += 8;
            നിർത്തുക;
#endif
        സ്ഥിരസ്ഥിതി: തിരികെ "unknown argument type";
        }
    }
    തിരികെ m3Err_none;
}

ശൂന്യം ReleaseCodePageNoTrack (IM3Runtime i_runtime, IM3CodePage i_codePage)
{
    എങ്കിൽ (i_codePage) {
        IM3CodePage* list;

        bool pageFull = (അക്കം_Free_വരികൾ(i_codePage) < d_m3_Code_Page_Free_വരികൾ_Threshold);
        എങ്കിൽ (pageFull) {
            list = &i_runtime->pagesFull;
        } അല്ലെങ്കിൽ {
            list = &i_runtime->pagesOpen;
        }

        തള്ളുക_Code_Page(list, i_codePage);                         m3log (emit, "release page: %d to queue: '%s'", i_codePage->info.sequence, pageFull ? "full" : "open")
    }
}


IM3CodePage AcquireCodePageWithCapacity (IM3Runtime i_runtime, u32 i_min_വരി_എണ്ണം)
{
    IM3CodePage page = RemoveCodePageOfCapacity(&i_runtime->pagesOpen, i_min_വരി_എണ്ണം);

    എങ്കിൽ (not page) {
        page = Environment_AcquireCodePage(i_runtime->environment, i_min_വരി_എണ്ണം);

        എങ്കിൽ (not page) {
            page = NewCodePage(i_runtime, i_min_വരി_എണ്ണം);
        }

        എങ്കിൽ (page) {
            i_runtime->അക്കം_Code_Pages++;
        }
    }

    എങ്കിൽ (page) {                                                  m3log (emit, "acquire page: %d", page->info.sequence);
        i_runtime->അക്കം_Active_Code_Pages++;
    }

    തിരികെ page;
}


IM3CodePage AcquireCodePage (IM3Runtime i_runtime)
{
    തിരികെ AcquireCodePageWithCapacity(i_runtime, d_m3_Code_Page_Free_വരികൾ_Threshold);
}


ശൂന്യം ReleaseCodePage (IM3Runtime i_runtime, IM3CodePage i_codePage)
{
    എങ്കിൽ (i_codePage) {
        ReleaseCodePageNoTrack(i_runtime, i_codePage);
        i_runtime->അക്കം_Active_Code_Pages--;

#എങ്കിൽ defined(DEBUG)
        u32 അക്കം_Open = എണ്ണം_Code_Pages(i_runtime->pagesOpen);
        u32 അക്കം_Full = എണ്ണം_Code_Pages(i_runtime->pagesFull);

        m3log(runtime, "runtime: %p; open-pages: %d; full-pages: %d; active: %d; total: %d", i_runtime, അക്കം_Open, അക്കം_Full, i_runtime->അക്കം_Active_Code_Pages, i_runtime->അക്കം_Code_Pages);

        d_m3Assert(അക്കം_Open + അക്കം_Full + i_runtime->അക്കം_Active_Code_Pages == i_runtime->അക്കം_Code_Pages);

#  എങ്കിൽ d_m3LogCodePages
        dump_code_page(i_codePage, /* startPC: */ ശൂന്യ);
#  endif
#endif
    }
}


#എങ്കിൽ d_m3VerboseErrorMessages
M3Result m3Error (M3Result i_result, IM3Runtime i_runtime, IM3Module i_module, IM3Function i_function,
                  സ്ഥിരം അക്ഷരം* സ്ഥിരം i_file, u32 i_വരി_അക്കം, സ്ഥിരം അക്ഷരം* സ്ഥിരം i_errorMessage, ...)
{
    എങ്കിൽ (i_runtime) {
        i_runtime->error         = (M3ErrorInfo){ .result = i_result, .runtime = i_runtime, .module = i_module, .function = i_function, .file = i_file, .വരി_2 = i_വരി_അക്കം };
        i_runtime->error.message = i_runtime->error_message;

        va_list args;
        va_start(args, i_errorMessage);
        vsnprintf(i_runtime->error_message, വലുപ്പം(i_runtime->error_message), i_errorMessage, args);
        va_end(args);
    }

    തിരികെ i_result;
}
#endif


ശൂന്യം m3_GetErrorInfo (IM3Runtime i_runtime, M3ErrorInfo* o_info)
{
    എങ്കിൽ (i_runtime) {
        *o_info = i_runtime->error;
        m3_ResetErrorInfo(i_runtime);
    }
}


ശൂന്യം m3_ResetErrorInfo (IM3Runtime i_runtime)
{
    എങ്കിൽ (i_runtime) {
        M3_INIT(i_runtime->error);
        i_runtime->error.message = "";
    }
}

uint8_t* m3_GetMemory (IM3Module i_module, size_t* o_memorySizeInBytes, uint32_t i_memoryIndex)
{
    uint8_t* memory = ശൂന്യ;
    size_t   size   = 0;

    എങ്കിൽ (i_module and i_memoryIndex < i_module->അക്കം_Memories) {
        IM3Memory mem = i_module->memories[i_memoryIndex];

        എങ്കിൽ (mem->mallocated) {
            size = mem->mallocated->length;

            എങ്കിൽ (size) {
                memory = m3MemData(mem->mallocated);
            }
        }
    }

    എങ്കിൽ (o_memorySizeInBytes) {
        *o_memorySizeInBytes = size;
    }

    തിരികെ memory;
}


size_t m3_GetMemorySize (IM3Module i_module, uint32_t i_memoryIndex)
{
    എങ്കിൽ (not i_module or i_memoryIndex >= i_module->അക്കം_Memories) {
        തിരികെ 0;
    }

    IM3Memory mem = i_module->memories[i_memoryIndex];

    തിരികെ mem->mallocated ? mem->mallocated->length : 0;
}


size_t m3_GetMemorySizeAt (സ്ഥിരം ശൂന്യം* i_memory)
{
    എങ്കിൽ (not i_memory) {
        തിരികെ 0;
    }

    // the header sits immediately before the data it describes
    സ്ഥിരം M3MemoryHeader* header = ((സ്ഥിരം M3MemoryHeader*)i_memory) - 1;

    തിരികെ header->length;
}


M3Result m3_FindExportedMemory (IM3Module i_module, cstr_t i_name, u32* o_memoryIndex)
{
    എങ്കിൽ (not i_module or not i_name or not o_memoryIndex) {
        തിരികെ m3Err_unknownMemory;
    }

    സ്ഥിരം M3Export* e = Module_FindExport(i_module, d_externalKind_memory, i_name);

    എങ്കിൽ (not e) {
        തിരികെ m3Err_unknownMemory;
    }

    *o_memoryIndex = e->index;
    തിരികെ m3Err_none;
}


bool Module_HasLinkedHostImport (IM3Module i_module, cstr_t i_importModule)
{
    വേണ്ടി (u32 i = 0; i < i_module->അക്കം_Functions; ++i) {
        IM3Function f = &i_module->functions[i];

        // hostMemory is set when a host function is bound to the import, so it
        // is also the mark of one - see CompileRawFunction
        എങ്കിൽ (not f->hostMemory or f->wasm or f->resolved) {
            തുടരുക;
        }

        എങ്കിൽ (f->import.moduleUtf8 and strcmp(f->import.moduleUtf8, i_importModule) == 0) {
            തിരികെ true;
        }
    }

    തിരികെ false;
}


M3Result m3_BindImportMemory (IM3Module io_module, cstr_t i_importModule, u32 i_memoryIndex)
{
    എങ്കിൽ (not io_module or not i_importModule) {
        തിരികെ m3Err_moduleNotLinked;
    }

    എങ്കിൽ (i_memoryIndex >= io_module->അക്കം_Memories) {
        തിരികെ m3Err_unknownMemory;
    }

    // the memory itself, not the index: linking may have pointed this slot at
    // another module's memory, and growing one reallocates behind it
    IM3Memory memory = io_module->memories[i_memoryIndex];

    സ്ഥിരം bool wildcardModule = (strcmp(i_importModule, "*") == 0);

    വേണ്ടി (u32 i = 0; i < io_module->അക്കം_Functions; ++i) {
        IM3Function f = &io_module->functions[i];

        // only an import that a host function was bound to has a memory to
        // pin: one with a body of its own reads memory through the interpreter,
        // and one resolved to another module's export runs that module's code
        എങ്കിൽ (not f->hostMemory or f->wasm or f->resolved) {
            തുടരുക;
        }

        എങ്കിൽ (wildcardModule or (f->import.moduleUtf8 and strcmp(f->import.moduleUtf8, i_importModule) == 0)) {
            f->hostMemory = memory;
        }
    }

    തിരികെ m3Err_none;
}


M3BacktraceInfo* m3_GetBacktrace (IM3Runtime i_runtime)
{
#എങ്കിൽ d_m3RecordBacktraces
    തിരികെ &i_runtime->backtrace;
#അല്ലെങ്കിൽ
    തിരികെ ശൂന്യ;
#endif
}


//-------------------------------------------------------------------------------------------------------------------------------
//  snapshots & suspendable execution
//-------------------------------------------------------------------------------------------------------------------------------

ശൂന്യം m3_SetSuspendable (IM3Runtime io_runtime, bool i_suspendable)
{
    എങ്കിൽ (io_runtime) {
        io_runtime->isSuspendable = i_suspendable;
    }
}

ശൂന്യം m3_RequestSuspend (IM3Runtime io_runtime)
{
    എങ്കിൽ (io_runtime) {
        io_runtime->suspendRequested = true;
    }
}

bool m3_IsSuspended (IM3Runtime i_runtime)
{
    എങ്കിൽ (!i_runtime) {
        തിരികെ false;
    }
#എങ്കിൽ d_m3_Has_അട്ടി_Switching
    എങ്കിൽ (i_runtime->വേര്_Continuation && i_runtime->വേര്_Continuation->state == cont_suspended) {
        തിരികെ true;
    }
#endif
    തിരികെ false;
}

M3Result m3_ResumeRuntime (IM3Runtime io_runtime)
{
    എങ്കിൽ (!io_runtime) {
        തിരികെ m3Err_mallocFailed;
    }
    എങ്കിൽ (!io_runtime->isSuspendable) {
        തിരികെ m3Err_none;
    }
#എങ്കിൽ d_m3HasSnapshots
    // a restore that failed leaves nothing to resume, and says so every time
    // rather than resuming nothing successfully
    എങ്കിൽ (io_runtime->വേര്_Continuation and io_runtime->വേര്_Continuation->entryFunction and
        io_runtime->വേര്_Continuation->entryFunction->module->isUnusable) {
        തിരികെ "a snapshot failed to restore into the module, so it cannot run";
    }
#endif
#എങ്കിൽ d_m3_Has_അട്ടി_Switching
    എങ്കിൽ (!io_runtime->വേര്_Continuation || io_runtime->വേര്_Continuation->state != cont_suspended) {
        തിരികെ m3Err_none;
    }
    // what finishes returns into the bottom of the stack, where it was entered
    io_runtime->അട്ടി = io_runtime->origin_അട്ടി;

    // this is the outermost call again, as far as a host function calling back
    // into Wasm from inside it can tell
    io_runtime->entered = io_runtime->വേര്_Continuation->entryFunction;
    io_runtime->callNesting++;

    d_m3_അട്ടി_Limit_Enter(io_runtime);
    io_runtime->resumePastCheck = true;
    M3Result result             = (M3Result)ResumeContinuation(io_runtime, io_runtime->വേര്_Continuation);
    io_runtime->resumePastCheck = false;
    d_m3_അട്ടി_Limit_Leave(io_runtime);

#  എങ്കിൽ d_m3HasExceptionHandling
    എങ്കിൽ (M3_UNLIKELY(result == m3Err_pendingException)) {
        result = m3Err_trapUncaughtException;
    }
    RecordUncaughtException(io_runtime, result);
#  endif

    io_runtime->callNesting--;

#  എങ്കിൽ d_m3HasExceptionHandling
    എങ്കിൽ (result != m3Err_continuationSuspended and io_runtime->callNesting == 0) {
        FreeExceptions(io_runtime);
    }
#  endif

    // the results are where the paused call left room for them, so a finished
    // resume can be asked for them the way the call itself could have been
    io_runtime->lastCalled = (result == m3Err_none) ? io_runtime->വേര്_Continuation->entryFunction : ശൂന്യ;

    Runtime_Place_Call_അട്ടി(io_runtime);
    തിരികെ result;
#അല്ലെങ്കിൽ
    തിരികെ m3Err_none;
#endif
}
