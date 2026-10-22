//
//  m3_env.c
//
//  Created by Steven Massey on 4/19/19.
//  Copyright © 2019 Steven Massey. All rights reserved.
//

#ಅನ್ತರ್ಭಾವ <stdarg.h>
#ಅನ್ತರ್ಭಾವ <limits.h>
#ಅನ್ತರ್ಭಾವ <errno.h>
#ಅನ್ತರ್ಭಾವ <float.h>
#ಅನ್ತರ್ಭಾವ <ctype.h>

#ಅನ್ತರ್ಭಾವ "m3_env.h"
#ಅನ್ತರ್ಭಾವ "m3_bind.h"
#ಅನ್ತರ್ಭಾವ "m3_deterministic.h"
#ಅನ್ತರ್ಭಾವ "m3_compile.h"
#ಅನ್ತರ್ಭಾವ "m3_host.h"
#ಅನ್ತರ್ಭಾವ "m3_exception.h"
#ಅನ್ತರ್ಭಾವ "m3_info.h"


IM3Environment m3_NewEnvironment ()
{
    IM3Environment env = m3_AllocStruct(M3Environment);

    ಯದಿ (env) {
        _try
        {
            // create FuncTypes for all simple block return ValueTypes.
            // v128 is skipped: it parses as a slot but has no operations.
            ಪ್ರತಿ (u8 t = c_m3Type_none; t < c_m3_Type_ಎಣಿಕೆ; t++) {
                ಯದಿ (t == c_m3Type_v128) {
                    ಅನುವರ್ತ;
                }

                IM3FuncType ftype;
_               (AllocFuncType(&ftype, 1));

                ftype->ಅಂಕೆ_Args  = 0;
                ftype->ಅಂಕೆ_Rets  = (t == c_m3Type_none) ? 0 : 1;
                ftype->types[0] = t;

                Environment_AddFuncType(env, &ftype);

                env->retFuncTypes[t] = ftype;
            }
        }

        _catch:
        ಯದಿ (result) {
            m3_FreeEnvironment(env);
            env = ರಿಕ್ತ;
        }
    }

    ಪ್ರತ್ಯಾಗಮ env;
}


ಶೂನ್ಯ Environment_Release (IM3Environment i_environment)
{
    IM3FuncType ftype = i_environment->funcTypes;

    ಯಾವತ್ (ftype) {
        IM3FuncType next = ftype->next;
        m3_Free(ftype);
        ftype = next;
    }

    m3log(runtime, "freeing %d pages from environment", ಎಣಿಕೆ_Code_Pages(i_environment->pagesReleased));
    FreeCodePages(&i_environment->pagesReleased);
}


ಶೂನ್ಯ m3_FreeEnvironment (IM3Environment i_environment)
{
    ಯದಿ (i_environment) {
        Environment_Release(i_environment);
        m3_Free(i_environment);
    }
}


ಶೂನ್ಯ m3_SetCustomSectionHandler (IM3Environment i_environment, M3SectionHandler i_handler)
{
    ಯದಿ (i_environment) {
        i_environment->customSectionHandler = i_handler;
    }
}


// returns the same io_funcType or replaces it with an equivalent that's already in the type linked list
M3Result Environment_AddFuncType (IM3Environment i_environment, IM3FuncType* io_funcType)
{
    IM3FuncType addType = *io_funcType;
    IM3FuncType newType = i_environment->funcTypes;

    ಯಾವತ್ (newType) {
        ಯದಿ (not newType->inRecGroup and AreFuncTypesEqual(newType, addType)) {
            m3_Free(addType);
            ಭಙ್ಗ;
        }

        newType = newType->next;
    }

    ಯದಿ (newType == ರಿಕ್ತ) {
        // a type index has to fit in the heap type field of an m3type_t
        ಯದಿ (i_environment->ಅಂಕೆ_Func_Types >= d_m3_Max_Sane_Types_ಎಣಿಕೆ) {
            m3_Free(addType);
            *io_funcType = ರಿಕ್ತ;
            ಪ್ರತ್ಯಾಗಮ "too many distinct function types";
        }

        newType                  = addType;
        newType->canonicalIndex  = i_environment->ಅಂಕೆ_Func_Types++;
        newType->next            = i_environment->funcTypes;
        i_environment->funcTypes = newType;
    }

    *io_funcType = newType;

    ಪ್ರತ್ಯಾಗಮ m3Err_none;
}


// Adds a type without looking for an equal one first, and hands back the index
// it was given.
//
// Members of a recursive group do not get to share an entry with a type that
// merely looks like them: the group is their identity, so two groups spelled
// the same way are still two groups, and a type inside one is never the type
// outside it that matches field for field. Reserving the indices up front is
// also what lets the members refer to each other while they are being read.
M3Result Environment_ReserveFuncTypes (IM3Environment i_environment, u32 i_ಎಣಿಕೆ, u16* o_firstIndex)
{
    ಯದಿ ((u64)i_environment->ಅಂಕೆ_Func_Types + i_ಎಣಿಕೆ > d_m3_Max_Sane_Types_ಎಣಿಕೆ) {
        ಪ್ರತ್ಯಾಗಮ "too many distinct function types";
    }

    *o_firstIndex = i_environment->ಅಂಕೆ_Func_Types;
    i_environment->ಅಂಕೆ_Func_Types += (u16)i_ಎಣಿಕೆ;

    ಪ್ರತ್ಯಾಗಮ m3Err_none;
}


ಶೂನ್ಯ Environment_AdoptFuncType (IM3Environment i_environment, IM3FuncType i_funcType, u16 i_canonicalIndex)
{
    i_funcType->inRecGroup     = true;
    i_funcType->canonicalIndex = i_canonicalIndex;
    i_funcType->next           = i_environment->funcTypes;
    i_environment->funcTypes   = i_funcType;
}


IM3CodePage RemoveCodePageOfCapacity (M3CodePage** io_list, u32 i_minimum_ಸಾಲು_ಎಣಿಕೆ)
{
    IM3CodePage prev = ರಿಕ್ತ;
    IM3CodePage page = *io_list;

    ಯಾವತ್ (page) {
        ಯದಿ (ಅಂಕೆ_Free_ಸಾಲುಗಳು(page) >= i_minimum_ಸಾಲು_ಎಣಿಕೆ) {             d_m3Assert (page->info.usage_ಎಣಿಕೆ == 0);
            IM3CodePage next = page->info.next;
            ಯದಿ (prev) {
                prev->info.next = next; // mid-list
            } ಅನ್ಯಥಾ {
                *io_list = next;       // front of list
            }

            ಭಙ್ಗ;
        }

        prev = page;
        page = page->info.next;
    }

    ಪ್ರತ್ಯಾಗಮ page;
}


IM3CodePage Environment_AcquireCodePage (IM3Environment i_environment, u32 i_minimum_ಸಾಲು_ಎಣಿಕೆ)
{
    ಪ್ರತ್ಯಾಗಮ RemoveCodePageOfCapacity(&i_environment->pagesReleased, i_minimum_ಸಾಲು_ಎಣಿಕೆ);
}


ಶೂನ್ಯ Environment_ReleaseCodePages (IM3Environment i_environment, IM3CodePage i_codePageList)
{
    IM3CodePage end = i_codePageList;

    ಯಾವತ್ (end) {
        end->info.ಸಾಲು_Index = 0; // reset page
#ಯದಿ d_m3RecordBacktraces
        end->info.mapping->size = 0;
#endif // d_m3RecordBacktraces

        IM3CodePage next = end->info.next;
        ಯದಿ (not next) {
            ಭಙ್ಗ;
        }

        end = next;
    }

    ಯದಿ (end) {
        // push list to front
        end->info.next               = i_environment->pagesReleased;
        i_environment->pagesReleased = i_codePageList;
    }
}


IM3Runtime m3_NewRuntime (IM3Environment i_environment, u32 i_ರಾಶಿ_Size_In_Bytes, ಶೂನ್ಯ* i_userdata)
{
    IM3Runtime runtime = m3_AllocStruct(M3Runtime);

    ಯದಿ (runtime) {
        m3_ResetErrorInfo(runtime);

        runtime->environment = i_environment;
        runtime->userdata    = i_userdata;

        Deterministic_InitRuntime(runtime);

        runtime->origin_ರಾಶಿ = m3_Malloc("Wasm Stack", i_ರಾಶಿ_Size_In_Bytes + 4 * ಪರಿಮಾಣ(m3slot_t)); // TODO: more precise stack checks

        ಯದಿ (runtime->origin_ರಾಶಿ) {
            runtime->ರಾಶಿ         = runtime->origin_ರಾಶಿ;
            runtime->ಅಂಕೆ_ರಾಶಿ_Slots = i_ರಾಶಿ_Size_In_Bytes / ಪರಿಮಾಣ(m3slot_t);          m3log (runtime, "new stack: %p, slots: %u", runtime->origin_ರಾಶಿ, runtime->ಅಂಕೆ_ರಾಶಿ_Slots);
        } ಅನ್ಯಥಾ {
            m3_Free(runtime);
        }
    }

    ಪ್ರತ್ಯಾಗಮ runtime;
}

ಶೂನ್ಯ m3_SetValidation (IM3Runtime i_runtime, bool i_enable)
{
#ಯದಿ d_m3EnableValidation
    ಯದಿ (i_runtime) {
        i_runtime->skipValidation = not i_enable;
    }
#ಅನ್ಯಥಾ
    (ಶೂನ್ಯ)i_runtime;
    (ಶೂನ್ಯ)i_enable;              // nothing to skip: the validator was compiled out
#endif
}

M3Result m3_SetResourceLimit (IM3Runtime i_runtime, M3ResourceLimit i_limit, uint64_t i_ಮೌಲ್ಯ)
{
    ಯದಿ (not i_runtime) {
        ಪ್ರತ್ಯಾಗಮ m3Err_mallocFailed;
    }
    ವಿಕಲ್ಪ (i_limit) {
    ಪಕ್ಷ c_m3Limit_MemoryBytes:
        ಯದಿ (i_ಮೌಲ್ಯ and i_ಮೌಲ್ಯ < i_runtime->memoryBytesUsed) {
            ಪ್ರತ್ಯಾಗಮ m3Err_resourceLimitBelowUsage;
        }
        i_runtime->memoryBytesLimit = i_ಮೌಲ್ಯ;
        ಪ್ರತ್ಯಾಗಮ m3Err_none;
    ಪಕ್ಷ c_m3Limit_TableElements:
        ಯದಿ (i_ಮೌಲ್ಯ and i_ಮೌಲ್ಯ < i_runtime->tableElementsUsed) {
            ಪ್ರತ್ಯಾಗಮ m3Err_resourceLimitBelowUsage;
        }
        i_runtime->tableElementsLimit = i_ಮೌಲ್ಯ;
        ಪ್ರತ್ಯಾಗಮ m3Err_none;
#ಯದಿ d_m3_Has_ರಾಶಿ_Switching
    ಪಕ್ಷ c_m3Limit_Continuations:
        ಯದಿ (i_ಮೌಲ್ಯ and i_ಮೌಲ್ಯ < i_runtime->continuationsAllocated) {
            ಪ್ರತ್ಯಾಗಮ m3Err_resourceLimitBelowUsage;
        }
        i_runtime->continuationsLimit = (u32)M3_MIN(i_ಮೌಲ್ಯ, (u64)UINT32_MAX);
        ಯಾವತ್ (i_ಮೌಲ್ಯ and i_runtime->val_ರಾಶಿ_Pool_ಎಣಿಕೆ > i_ಮೌಲ್ಯ - i_runtime->continuationsAllocated) {
            m3_Free(i_runtime->val_ರಾಶಿ_Pool[--i_runtime->val_ರಾಶಿ_Pool_ಎಣಿಕೆ]);
        }
        ಪ್ರತ್ಯಾಗಮ m3Err_none;
#ಅನ್ಯಥಾ
    ಪಕ್ಷ c_m3Limit_Continuations: ಪ್ರತ್ಯಾಗಮ m3Err_resourceLimitNotSupported;
#endif
    ಪಕ್ಷ c_m3Limit_GasUnits:
#ಯದಿ d_m3HasGasMetering
        i_runtime->gasLimit = i_runtime->gasRemaining = (i64)M3_MIN(i_ಮೌಲ್ಯ, (u64)INT64_MAX);
        ಪ್ರತ್ಯಾಗಮ m3Err_none;
#ಅನ್ಯಥಾ
        ಪ್ರತ್ಯಾಗಮ m3Err_resourceLimitNotSupported;
#endif
    ಸ್ವಭಾವ: ಪ್ರತ್ಯಾಗಮ m3Err_unknownResourceLimit;
    }
}

uint64_t m3_GetResourceLimit (IM3Runtime i_runtime, M3ResourceLimit i_limit)
{
    ಯದಿ (not i_runtime) {
        ಪ್ರತ್ಯಾಗಮ 0;
    }
    ವಿಕಲ್ಪ (i_limit) {
    ಪಕ್ಷ c_m3Limit_MemoryBytes: ಪ್ರತ್ಯಾಗಮ i_runtime->memoryBytesLimit;
    ಪಕ್ಷ c_m3Limit_TableElements: ಪ್ರತ್ಯಾಗಮ i_runtime->tableElementsLimit;
#ಯದಿ d_m3_Has_ರಾಶಿ_Switching
    ಪಕ್ಷ c_m3Limit_Continuations: ಪ್ರತ್ಯಾಗಮ i_runtime->continuationsLimit;
#endif
#ಯದಿ d_m3HasGasMetering
    ಪಕ್ಷ c_m3Limit_GasUnits: ಪ್ರತ್ಯಾಗಮ (u64)i_runtime->gasLimit;
#endif
    ಸ್ವಭಾವ: ಪ್ರತ್ಯಾಗಮ 0;
    }
}

uint64_t m3_GetResourceUsage (IM3Runtime i_runtime, M3ResourceLimit i_limit)
{
    ಯದಿ (not i_runtime) {
        ಪ್ರತ್ಯಾಗಮ 0;
    }
    ವಿಕಲ್ಪ (i_limit) {
    ಪಕ್ಷ c_m3Limit_MemoryBytes: ಪ್ರತ್ಯಾಗಮ i_runtime->memoryBytesUsed;
    ಪಕ್ಷ c_m3Limit_TableElements: ಪ್ರತ್ಯಾಗಮ i_runtime->tableElementsUsed;
#ಯದಿ d_m3_Has_ರಾಶಿ_Switching
    ಪಕ್ಷ c_m3Limit_Continuations: ಪ್ರತ್ಯಾಗಮ i_runtime->continuationsAllocated;
#endif
#ಯದಿ d_m3HasGasMetering
    ಪಕ್ಷ c_m3Limit_GasUnits: ಪ್ರತ್ಯಾಗಮ (u64)i_runtime->gasLimit - (u64)i_runtime->gasRemaining;
#endif
    ಸ್ವಭಾವ: ಪ್ರತ್ಯಾಗಮ 0;
    }
}

ಶೂನ್ಯ* m3_GetUserData (IM3Runtime i_runtime)
{
    ಪ್ರತ್ಯಾಗಮ i_runtime ? i_runtime->userdata : ರಿಕ್ತ;
}


ಶೂನ್ಯ* ForEachModule (IM3Runtime i_runtime, ModuleVisitor i_visitor, ಶೂನ್ಯ* i_info)
{
    ಶೂನ್ಯ* r = ರಿಕ್ತ;

    IM3Module module = i_runtime->modules;

    ಯಾವತ್ (module) {
        IM3Module next = module->next;
        ಯದಿ ((r = i_visitor(module, i_info))) {
            ಭಙ್ಗ;
        }

        module = next;
    }

    ಪ್ರತ್ಯಾಗಮ r;
}


ಶೂನ್ಯ* _FreeModule (IM3Module i_module, ಶೂನ್ಯ* i_info)
{
    m3_FreeModule(i_module);
    ಪ್ರತ್ಯಾಗಮ ರಿಕ್ತ;
}


// A module's memory and table index spaces hold borrowed pointers: linking
// repoints an imported slot at the memory or table the exporting module owns.
// m3_FreeModule tells a borrowed slot from an owned one by reading that owner,
// which only holds while the object is still allocated - and the free walk
// below visits modules in load order, with no ordering between an owner and
// the modules borrowing from it. So clear the borrowed slots first, in a pass
// that frees nothing; the walk then skips them on the NULL check it already
// does, instead of reading a memory some earlier module has freed.
ಶೂನ್ಯ* _ReleaseBorrowedSlots (IM3Module i_module, ಶೂನ್ಯ* i_info)
{
    ಪ್ರತಿ (u32 i = 0; i < i_module->ಅಂಕೆ_Memories; ++i) {
        IM3Memory memory = i_module->memories[i];

        ಯದಿ (memory and memory->owner != i_module) {
            i_module->memories[i] = ರಿಕ್ತ;
        }
    }

    ಪ್ರತಿ (u32 i = 0; i < i_module->ಅಂಕೆ_Tables; ++i) {
        IM3Table table = i_module->tables[i];

        ಯದಿ (table and table->owner != i_module) {
            i_module->tables[i] = ರಿಕ್ತ;
        }
    }

    ಪ್ರತ್ಯಾಗಮ ರಿಕ್ತ;
}


ಶೂನ್ಯ Runtime_Release (IM3Runtime i_runtime)
{
    ForEachModule(i_runtime, _ReleaseBorrowedSlots, ರಿಕ್ತ);
    ForEachModule(i_runtime, _FreeModule, ರಿಕ್ತ);                    d_m3Assert (i_runtime->ಅಂಕೆ_Active_Code_Pages == 0);

    Environment_ReleaseCodePages(i_runtime->environment, i_runtime->pagesOpen);
    Environment_ReleaseCodePages(i_runtime->environment, i_runtime->pagesFull);

    m3_Free(i_runtime->origin_ರಾಶಿ);

#ಯದಿ d_m3EnableValidation
    m3_Free(i_runtime->validator);
#endif

#ಯದಿ d_m3HasExceptionHandling
    // a runtime let go of while suspended still holds its exceptions
    FreeExceptions(i_runtime);
    m3_Free(i_runtime->uncaughtException);
#endif
#ಯದಿ d_m3HasExceptionHandling || d_m3_Has_ರಾಶಿ_Switching
    ಯಾವತ್ (i_runtime->hostTags) {
        M3HostTag* next = i_runtime->hostTags->next;
        m3_Free(i_runtime->hostTags);
        i_runtime->hostTags = next;
    }
#endif
#ಯದಿ d_m3_Has_ರಾಶಿ_Switching
    Continuation_ReleaseAll(i_runtime);
#endif
}


ಶೂನ್ಯ m3_FreeRuntime (IM3Runtime i_runtime)
{
    ಯದಿ (i_runtime) {
        m3_PrintProfilerInfo();

        Runtime_Release(i_runtime);
        m3_Free(i_runtime);
    }
}


#ಯದಿ d_m3_Has_ರಾಶಿ_Switching

IM3Continuation Continuation_New (IM3Runtime i_runtime, IM3FuncType i_type, IM3Function i_function)
{
    IM3Continuation cont = m3_AllocStruct(M3Continuation);
    ಯದಿ (!cont) {
        ಪ್ರತ್ಯಾಗಮ ರಿಕ್ತ;
    }

    cont->type          = i_type;
    cont->entryFunction = i_function;
    cont->state         = cont_allocated;

    cont->ಅಂಕೆ_ರಾಶಿ_Slots = d_m3_Continuation_ರಾಶಿ_Slots;
    // frames are recorded only when this continuation actually suspends, and
    // only as deep as it got, so the array starts out unallocated
    cont->frames    = ರಿಕ್ತ;
    cont->framesCap = 0;
    cont->ಅಂಕೆ_Frames = 0;

    cont->sp = cont->val_ರಾಶಿ;
    cont->pc = i_function ? i_function->compiled : ರಿಕ್ತ;
    cont->r0 = 0;
#  ಯದಿ d_m3HasFloat
    cont->fp0 = 0;
#  endif
    cont->bound_Args_ಎಣಿಕೆ    = 0;
    cont->ಅಂಕೆ_Suspend_Results = 0;
    cont->ಅಂಕೆ_Handlers       = 0;
    cont->handlersPC        = ರಿಕ್ತ;
    cont->parent            = ರಿಕ್ತ;

    cont->next               = i_runtime->continuations;
    i_runtime->continuations = cont;

    ಪ್ರತ್ಯಾಗಮ cont;
}

IM3Continuation Continuation_ForkSuspended (IM3Runtime i_runtime, IM3Continuation i_cont)
{
    IM3Continuation newCont = Continuation_New(i_runtime, i_cont->type, i_cont->entryFunction);
    ಯದಿ (M3_UNLIKELY(not newCont)) {
        ಪ್ರತ್ಯಾಗಮ ರಿಕ್ತ;
    }

    newCont->val_ರಾಶಿ      = i_cont->val_ರಾಶಿ;
    newCont->ಅಂಕೆ_ರಾಶಿ_Slots = i_cont->ಅಂಕೆ_ರಾಶಿ_Slots;
    newCont->sp            = i_cont->sp;
    newCont->pc            = i_cont->pc;
    newCont->suspendPoint  = i_cont->suspendPoint;
    newCont->r0            = i_cont->r0;
#  ಯದಿ d_m3HasFloat
    newCont->fp0 = i_cont->fp0;
#  endif
    newCont->frames    = i_cont->frames;
    newCont->framesCap = i_cont->framesCap;
    newCont->ಅಂಕೆ_Frames = i_cont->ಅಂಕೆ_Frames;

    newCont->bound_Args_ಎಣಿಕೆ = 0;
#  ಯದಿ d_m3HasExceptionHandling
    newCont->resumeThrow = i_cont->resumeThrow;
    i_cont->resumeThrow  = ರಿಕ್ತ;
#  endif
    newCont->ಅಂಕೆ_Suspend_Results = i_cont->ಅಂಕೆ_Suspend_Results;
    ಪ್ರತಿ (u32 i = 0; i < i_cont->ಅಂಕೆ_Suspend_Results; ++i) {
        newCont->suspendResultOffsets[i] = i_cont->suspendResultOffsets[i];
        newCont->suspendResultIs64[i]    = i_cont->suspendResultIs64[i];
    }

    newCont->ಅಂಕೆ_Handlers = i_cont->ಅಂಕೆ_Handlers;
    newCont->handlersPC  = i_cont->handlersPC;
    newCont->parent      = i_cont->parent;
    newCont->state       = cont_suspended;

    i_cont->val_ರಾಶಿ  = ರಿಕ್ತ;
    i_cont->frames    = ರಿಕ್ತ;
    i_cont->framesCap = 0;
    i_cont->ಅಂಕೆ_Frames = 0;
    i_cont->state     = cont_consumed;

    ಪ್ರತ್ಯಾಗಮ newCont;
}

// Only cont.new and snapshot restoration acquire stacks. Binding and suspension
// transfer them between records without changing the active count.
M3Result Continuation_Acquire_ರಾಶಿ (IM3Runtime runtime, IM3Continuation cont)
{
    ಯದಿ (not Continuation_Can_Acquire_ರಾಶಿ(runtime)) {
        ಪ್ರತ್ಯಾಗಮ m3Err_continuationLimitExceeded;
    }
    ಯದಿ (runtime->val_ರಾಶಿ_Pool_ಎಣಿಕೆ) {
        cont->val_ರಾಶಿ = runtime->val_ರಾಶಿ_Pool[--runtime->val_ರಾಶಿ_Pool_ಎಣಿಕೆ];
        memset(cont->val_ರಾಶಿ, 0, (cont->ಅಂಕೆ_ರಾಶಿ_Slots + 4) * ಪರಿಮಾಣ(m3slot_t));
    } ಅನ್ಯಥಾ {
        cont->val_ರಾಶಿ = m3_AllocArray(m3slot_t, cont->ಅಂಕೆ_ರಾಶಿ_Slots + 4);
    }
    ಯದಿ (not cont->val_ರಾಶಿ) {
        ಪ್ರತ್ಯಾಗಮ m3Err_mallocFailed;
    }
    cont->sp = cont->val_ರಾಶಿ;
    runtime->continuationsAllocated++;
    ಪ್ರತ್ಯಾಗಮ m3Err_none;
}

ಶೂನ್ಯ Continuation_Recycle_ರಾಶಿ (IM3Runtime runtime, IM3Continuation cont)
{
    ಯದಿ (not cont->val_ರಾಶಿ or cont == runtime->ಬೇರು_Continuation) {
        ಪ್ರತ್ಯಾಗಮ;
    }
    ಯದಿ (not runtime->val_ರಾಶಿ_Pool) {
        runtime->val_ರಾಶಿ_Pool = m3_AllocArray(m3slot_t*, 16);
        ಯದಿ (runtime->val_ರಾಶಿ_Pool) {
            runtime->val_ರಾಶಿ_Pool_Cap = 16;
        }
    }
    ಯದಿ (runtime->val_ರಾಶಿ_Pool_ಎಣಿಕೆ < runtime->val_ರಾಶಿ_Pool_Cap and
        (not runtime->continuationsLimit or runtime->val_ರಾಶಿ_Pool_ಎಣಿಕೆ <
                                              runtime->continuationsLimit - runtime->continuationsAllocated + 1)) {
        runtime->val_ರಾಶಿ_Pool[runtime->val_ರಾಶಿ_Pool_ಎಣಿಕೆ++] = cont->val_ರಾಶಿ;
        cont->val_ರಾಶಿ                                      = ರಿಕ್ತ;
    } ಅನ್ಯಥಾ {
        m3_Free(cont->val_ರಾಶಿ);
    }
    cont->sp = ರಿಕ್ತ;
    runtime->continuationsAllocated--;
}

ಶೂನ್ಯ Continuation_ReleaseAll (IM3Runtime io_runtime)
{
    // Records live until teardown: a consumed reference can still be in a table
    // or on a Wasm stack. Its value stack has already been recycled.
    IM3Continuation cont = io_runtime->continuations;

    ಯಾವತ್ (cont) {
        IM3Continuation next = cont->next;

        m3_Free(cont->val_ರಾಶಿ);
        m3_Free(cont->frames);
        m3_Free(cont);

        cont = next;
    }

    ಪ್ರತಿ (u32 i = 0; i < io_runtime->val_ರಾಶಿ_Pool_ಎಣಿಕೆ; ++i) {
        m3_Free(io_runtime->val_ರಾಶಿ_Pool[i]);
    }
    m3_Free(io_runtime->val_ರಾಶಿ_Pool);
    io_runtime->val_ರಾಶಿ_Pool_ಎಣಿಕೆ = io_runtime->val_ರಾಶಿ_Pool_Cap = 0;
    io_runtime->continuationsAllocated                          = 0;
    io_runtime->continuations                                   = ರಿಕ್ತ;
    io_runtime->activeContinuation                              = ರಿಕ್ತ;
    io_runtime->ಬೇರು_Continuation                                = ರಿಕ್ತ;
}


m3ret_t Continuation_RecordFrame (IM3Runtime i_runtime, ಧ್ರುವ M3Frame* i_frame)
{
    IM3Continuation cont = i_runtime->activeContinuation;

    // the marker only travels between a suspend and the resume that runs it,
    // and that whole stretch has an active continuation
    d_m3Assert(cont);

    ಯದಿ (cont->ಅಂಕೆ_Frames == cont->framesCap) {
        u32 newCap = cont->framesCap ? cont->framesCap * 2 : 8;

        ಯದಿ (newCap > d_m3ContinuationMaxFrames) {
            newCap = d_m3ContinuationMaxFrames;
        }

        ಯದಿ (newCap == cont->framesCap) {
            ಪ್ರತ್ಯಾಗಮ m3_Err_trap_ರಾಶಿ_Overflow;
        }

        M3Frame* frames = m3_ReallocArray(M3Frame, cont->frames, newCap, cont->framesCap);

        ಯದಿ (not frames) {
            ಪ್ರತ್ಯಾಗಮ m3Err_mallocFailed;
        }

        cont->frames    = frames;
        cont->framesCap = newCap;
    }

    cont->frames[cont->ಅಂಕೆ_Frames++] = *i_frame;

    ಪ್ರತ್ಯಾಗಮ m3Err_continuationSuspended;
}

#endif // d_m3HasStackSwitching

M3Result EvaluateExpression (IM3Module i_module, ಶೂನ್ಯ* o_expressed, m3type_t i_type, bytes_t* io_bytes, cbytes_t i_end)
{
    M3Result result = m3Err_none;

    // OPTZ: use a simplified interpreter for expressions

    // create a temporary runtime context
#ಯದಿ defined(d_m3PreferStaticAlloc)
    ಸ್ಥಿರ M3Runtime runtime;
#ಅನ್ಯಥಾ
    M3Runtime runtime;
#endif
    M3_INIT(runtime);

    runtime.environment   = i_module->runtime->environment;
    runtime.ಅಂಕೆ_ರಾಶಿ_Slots = i_module->runtime->ಅಂಕೆ_ರಾಶಿ_Slots;
    runtime.ರಾಶಿ         = i_module->runtime->ರಾಶಿ;

    m3stack_t ರಾಶಿ = (m3stack_t)runtime.ರಾಶಿ;

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

    ಯದಿ (o->page) {
        IM3FuncType ftype = ರಿಕ್ತ;

#ಯದಿ d_m3HasTypedRefs
        // A reference type has to be checked as the type it is: the shared
        // return types are one per storage class, so borrowing one of those
        // would let any reference at all satisfy a particular one.
        ಯದಿ (IsSpelledRefType(i_type)) {
            result = AllocFuncType(&ftype, 1);

            ಯದಿ (not result) {
                ftype->ಅಂಕೆ_Rets  = 1;
                ftype->types[0] = i_type;

                result = Environment_AddFuncType(runtime.environment, &ftype);
            }
        } ಅನ್ಯಥಾ
#endif
        {
            ftype = runtime.environment->retFuncTypes[BaseTypeOf(i_type)];
        }

        pc_t m3code = GetPagePC(o->page);

        ಯದಿ (not result) {
            result = CompileExpression(o, ftype);
        }

        ಯದಿ (not result && o->max_ರಾಶಿ_Slots >= runtime.ಅಂಕೆ_ರಾಶಿ_Slots) {
            result = m3_Err_trap_ರಾಶಿ_Overflow;
        }

        ಯದಿ (not result) {
#ಯದಿ (d_m3EnableOpProfiling || d_m3EnableOpTracing)
            m3ret_t r = RunCode(m3code, ರಾಶಿ, ರಿಕ್ತ, d_m3OpDefaultArgs, d_m3BaseCstr);
#ಅನ್ಯಥಾ
            m3ret_t r = RunCode(m3code, ರಾಶಿ, ರಿಕ್ತ, d_m3OpDefaultArgs);
#endif

            ಯದಿ (r == 0) {                                                                   m3log (runtime, "expression result: %s", SPrint_ಮೌಲ್ಯ (ರಾಶಿ, i_type));
                ಯದಿ (SizeOfType(BaseTypeOf(i_type)) == ಪರಿಮಾಣ(u32)) {
                    *(u32*)o_expressed = *((u32*)ರಾಶಿ);
                } ಅನ್ಯಥಾ {
                    *(u64*)o_expressed = *((u64*)ರಾಶಿ);
                }
            }
        }

        ReleaseCodePage(&runtime, o->page);
    } ಅನ್ಯಥಾ {
        result = m3Err_mallocFailedCodePage;
    }

    runtime.origin_ರಾಶಿ = ರಿಕ್ತ;        // prevent free(stack) in ReleaseRuntime
    Runtime_Release(&runtime);
    i_module->runtime = savedRuntime;

    *io_bytes = o->wasm;

    ಪ್ರತ್ಯಾಗಮ result;
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
ಸ್ಥಿರ
bool LimitsSatisfy (u64 i_exportedSize, bool i_exportedHasMax, u64 i_exportedMax,
                    u64 i_importMin, bool i_importHasMax, u64 i_importMax)
{
    ಯದಿ (i_exportedSize < i_importMin) {
        ಪ್ರತ್ಯಾಗಮ false;
    }

    ಯದಿ (i_importHasMax and (not i_exportedHasMax or i_exportedMax > i_importMax)) {
        ಪ್ರತ್ಯಾಗಮ false;
    }

    ಪ್ರತ್ಯಾಗಮ true;
}


// The export of this kind under this name, or NULL. The index it carries is
// resolved by the caller, against the module's index space as it stands then -
// after linking, a re-exported import's slot holds the entity it was linked to.
ಸ್ಥಿರ
ಧ್ರುವ M3Export* Module_FindExport (IM3Module i_module, u8 i_kind, cstr_t i_name)
{
    ಪ್ರತಿ (u32 i = 0; i < i_module->ಅಂಕೆ_Exports; ++i) {
        ಧ್ರುವ M3Export* e = &i_module->exports[i];

        ಯದಿ (e->kind == i_kind and strcmp(e->name, i_name) == 0) {
            ಪ್ರತ್ಯಾಗಮ e;
        }
    }

    ಪ್ರತ್ಯಾಗಮ ರಿಕ್ತ;
}


ಸ್ಥಿರ
IM3Function Module_FindExportedFunction (IM3Module i_module, cstr_t i_name)
{
    ಧ್ರುವ M3Export* e = Module_FindExport(i_module, d_externalKind_function, i_name);

    // A module that re-exports an import names the placeholder here. Resolve to
    // the function that actually runs, or a host-side call would run it against
    // the importing module's memory.
    ಪ್ರತ್ಯಾಗಮ e ? Function_Implementation(&i_module->functions[e->index]) : ರಿಕ್ತ;
}


ಸ್ಥಿರ
IM3Memory Module_FindExportedMemory (IM3Module i_module, cstr_t i_name)
{
    ಧ್ರುವ M3Export* e = Module_FindExport(i_module, d_externalKind_memory, i_name);

    ಪ್ರತ್ಯಾಗಮ e ? i_module->memories[e->index] : ರಿಕ್ತ;
}


ಸ್ಥಿರ
IM3Table Module_FindExportedTable (IM3Module i_module, cstr_t i_name)
{
    ಧ್ರುವ M3Export* e = Module_FindExport(i_module, d_externalKind_table, i_name);

    ಪ್ರತ್ಯಾಗಮ e ? i_module->tables[e->index] : ರಿಕ್ತ;
}


ಸ್ಥಿರ
IM3Global Module_FindExportedGlobal (IM3Module i_module, cstr_t i_name)
{
    ಧ್ರುವ M3Export* e = Module_FindExport(i_module, d_externalKind_global, i_name);

    ಪ್ರತ್ಯಾಗಮ e ? &i_module->globals[e->index] : ರಿಕ್ತ;
}


#ಯದಿ d_m3HasExceptionHandling || d_m3_Has_ರಾಶಿ_Switching
ಸ್ಥಿರ
IM3Tag Module_FindExportedTag (IM3Module i_module, cstr_t i_name)
{
    ಧ್ರುವ M3Export* e = Module_FindExport(i_module, d_externalKind_tag, i_name);

    ಯದಿ (not e) {
        ಪ್ರತ್ಯಾಗಮ ರಿಕ್ತ;
    }

    IM3Tag tag = &i_module->tags[e->index];

    ಪ್ರತ್ಯಾಗಮ tag->resolved ? tag->resolved : tag;
}
#endif


// Whether the module exports anything at all under this name. Export names are
// unique within a module, so a name one of the lookups above missed but this
// one finds is exported as something else - a kind the import cannot be
// satisfied by, rather than a name the module never exported.
ಸ್ಥಿರ
bool Module_HasExport (IM3Module i_module, cstr_t i_name)
{
    ಪ್ರತಿ (u32 i = 0; i < i_module->ಅಂಕೆ_Exports; ++i) {
        ಯದಿ (strcmp(i_module->exports[i].name, i_name) == 0) {
            ಪ್ರತ್ಯಾಗಮ true;
        }
    }

    ಪ್ರತ್ಯಾಗಮ false;
}


// Points each of the module's imports at whatever already-loaded module exports
// it. Runs before anything is allocated or initialized: a memory import has to
// be resolved before InitMemory would give it pages of its own, and a global
// import before InitGlobals runs an initializer that reads it.
ಸ್ಥಿರ
M3Result LinkImports (IM3Runtime io_runtime, IM3Module io_module)
{
    M3Result result = m3Err_none;

    ಪ್ರತಿ (u32 i = 0; i < io_module->ಅಂಕೆ_Functions; ++i) {
        IM3Function f = &io_module->functions[i];

        ಯದಿ (f->wasm or not (f->import.moduleUtf8 and f->import.fieldUtf8)) {
            ಅನುವರ್ತ;
        }

        IM3Module from = m3_FindModule(io_runtime, f->import.moduleUtf8);
        ಯದಿ (not from) {
            ಅನುವರ್ತ;
        }

        IM3Function exported = Module_FindExportedFunction(from, f->import.fieldUtf8);
        ಯದಿ (not exported) {
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

    ಪ್ರತಿ (u32 i = 0; i < io_module->ಅಂಕೆ_Memories; ++i) {
        IM3Memory memory = io_module->memories[i];

        ಯದಿ (not memory->imported or memory->owner != io_module) {
            ಅನುವರ್ತ;
        }

        IM3Module from = m3_FindModule(io_runtime, memory->import.moduleUtf8);
        ಯದಿ (not from) {
            ಅನುವರ್ತ;
        }

        IM3Memory exported = Module_FindExportedMemory(from, memory->import.fieldUtf8);
        ಯದಿ (not exported) {
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
                 not LimitsSatisfy(exported->ಅಂಕೆ_Pages, exported->hasMax, exported->maxPages,
                                   memory->initPages, memory->hasMax, memory->maxPages));

        // hand the slot over to the exporter's memory, and drop the placeholder
        FreeMemoryBlock(memory);
        FreeImportInfo(&memory->import);
        m3_Free(memory);

        io_module->memories[i] = exported;
    }

    ಪ್ರತಿ (u32 i = 0; i < io_module->ಅಂಕೆ_Tables; ++i) {
        IM3Table table = io_module->tables[i];

        ಯದಿ (not table->imported or table->owner != io_module) {
            ಅನುವರ್ತ;
        }

        IM3Module from = m3_FindModule(io_runtime, table->import.moduleUtf8);
        ಯದಿ (not from) {
            ಅನುವರ್ತ;
        }

        IM3Table exported = Module_FindExportedTable(from, table->import.fieldUtf8);
        ಯದಿ (not exported) {
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

    ಪ್ರತಿ (u32 i = 0; i < io_module->ಅಂಕೆ_Globals; ++i) {
        IM3Global g = &io_module->globals[i];

        ಯದಿ (not g->imported or not (g->import.moduleUtf8 and g->import.fieldUtf8)) {
            ಅನುವರ್ತ;
        }

        IM3Module from = m3_FindModule(io_runtime, g->import.moduleUtf8);
        ಯದಿ (not from) {
            ಅನುವರ್ತ;
        }

        IM3Global exported = Module_FindExportedGlobal(from, g->import.fieldUtf8);
        ಯದಿ (not exported) {
            _throwif(m3Err_incompatibleImportType, Module_HasExport(from, g->import.fieldUtf8));
            _throw(m3Err_unknownImport);
        }

        _throwif(m3Err_incompatibleImportType, exported->type != g->type);
        _throwif(m3Err_incompatibleImportType, exported->isMutable != g->isMutable);

        g->resolved = exported->resolved ? exported->resolved : exported;
    }

#ಯದಿ d_m3HasExceptionHandling || d_m3_Has_ರಾಶಿ_Switching
    ಪ್ರತಿ (u32 i = 0; i < io_module->ಅಂಕೆ_Tags; ++i) {
        IM3Tag tag = &io_module->tags[i];

        ಯದಿ (not tag->imported or not (tag->import.moduleUtf8 and tag->import.fieldUtf8)) {
            ಅನುವರ್ತ;
        }

        IM3Module from = m3_FindModule(io_runtime, tag->import.moduleUtf8);
        ಯದಿ (not from) {
            ಅನುವರ್ತ;
        }

        IM3Tag exported = Module_FindExportedTag(from, tag->import.fieldUtf8);
        ಯದಿ (not exported) {
            _throwif(m3Err_incompatibleImportType, Module_HasExport(from, tag->import.fieldUtf8));
            _throw(m3Err_unknownImport);
        }

        _throwif(m3Err_incompatibleImportType, exported->type != tag->type);

        tag->resolved = exported->resolved ? exported->resolved : exported;
    }
#endif

    _catch: ಪ್ರತ್ಯಾಗಮ result;
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
    i_module->memory0 = i_module->ಅಂಕೆ_Memories ? i_module->memories[0]
                                              : &i_module->emptyMemory;

    ಯದಿ (i_module->ಅಂಕೆ_Memories == 0) {
        // nothing addressable, but _mem still has to point somewhere
        i_module->emptyMemory.owner    = i_module;
        i_module->emptyMemory.pageSize = d_m3DefaultMemPageSize;

_       (ResizeMemory(io_runtime, &i_module->emptyMemory, 0));
    }

    ಪ್ರತಿ (u32 i = 0; i < i_module->ಅಂಕೆ_Memories; ++i) {
        IM3Memory memory = i_module->memories[i];

        // a slot that already points at another module's memory is that
        // module's to allocate
        ಯದಿ (memory->owner != i_module or memory->mallocated) {
            ಅನುವರ್ತ;
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
        ಯದಿ (not memory->hasMax) {
            memory->maxPages = memory->isMemory64
                                 ? (pageSize > 1 ? (UINT64_MAX / pageSize) + 1 : UINT64_MAX)
                                 : (0x100000000ull / pageSize);
        }

_       (ResizeMemory(io_runtime, memory, memory->initPages));
    }

    _catch: ಪ್ರತ್ಯಾಗಮ result;
}


M3Result ResizeMemory (IM3Runtime io_runtime, IM3Memory memory, u64 i_ಅಂಕೆ_Pages)
{
    M3Result result = m3Err_none;

    u64 ಅಂಕೆ_Pages_To_Alloc = i_ಅಂಕೆ_Pages;

    ಯದಿ (ಅಂಕೆ_Pages_To_Alloc <= memory->maxPages) {
        // A 64-bit memory may ask for up to 2^48 pages, which overflows a u64
        // of bytes. Nothing that large can be backed, so refuse it up front
        // rather than multiplying into a wrapped size.
        _throwif("linear memory limitation exceeded",
                 ಅಂಕೆ_Pages_To_Alloc > d_m3AddressLimit / memory->pageSize);

        u64 ಅಂಕೆ_Page_Bytes = ಅಂಕೆ_Pages_To_Alloc * memory->pageSize;

#ಯದಿ d_m3MaxLinearMemoryPages > 0
        // the limit is a memory size, counted in default-sized pages; comparing
        // it against a raw page count would make it 65536 times stricter for a
        // module whose pages are one byte
        _throwif("linear memory limitation exceeded",
                 ಅಂಕೆ_Page_Bytes > (u64)d_m3MaxLinearMemoryPages * d_m3DefaultMemPageSize);
#endif

        // Limit the amount of memory that gets actually allocated
        ಯದಿ (io_runtime->memoryLimit) {
            ಅಂಕೆ_Page_Bytes = M3_MIN(ಅಂಕೆ_Page_Bytes, (u64)io_runtime->memoryLimit);
        }

        u64 oldLength = memory->mallocated ? memory->mallocated->length : 0;
        u64 otherUsed = io_runtime->memoryBytesUsed - oldLength;
        _throwif(m3Err_memoryLimitExceeded, io_runtime->memoryBytesLimit and
                                              (ಅಂಕೆ_Page_Bytes > io_runtime->memoryBytesLimit or
                                               otherUsed > io_runtime->memoryBytesLimit - ಅಂಕೆ_Page_Bytes));

        _throwif("linear memory limitation exceeded", ಅಂಕೆ_Page_Bytes > (u64)SIZE_MAX - ಪರಿಮಾಣ(M3MemoryHeader));

#ಯದಿ d_m3GuardedMemory

        // A slot holds every address a Wasm access can name, and nothing beyond
        // that: a memory larger than the data part of one would put addresses past
        // the guard, where a fault is nobody's to recognize
        _throwif("linear memory limitation exceeded", ಅಂಕೆ_Page_Bytes > d_m3GuardedDataBytes);

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

        ಯದಿ (memory->guardSlot == ರಿಕ್ತ) {
            memory->guardSlot = Guard_TakeSlot();
            _throwifnull(memory->guardSlot);
        }

        // Growing only ever commits more of the same slot, so the memory never
        // moves - which the interpreter does not rely on, but nothing minds
        _throwif(m3Err_mallocFailed, not Guard_CommitSlot(memory->guardSlot, (size_t)ಅಂಕೆ_Page_Bytes));

        memory->mallocated = Guard_SlotHeader(memory->guardSlot);

#ಅನ್ಯಥಾ

        size_t ಅಂಕೆ_Bytes = (size_t)ಅಂಕೆ_Page_Bytes + ಪರಿಮಾಣ(M3MemoryHeader);

        // What is really there, which is not numPages * pageSize: the limit above
        // caps the bytes without capping the page count, so a memory can be short
        // of a page - by design, since a whole 64 KiB page is often more than an
        // MCU has. Measuring the old block by the page count would overstate it,
        // and then realloc never sees the size it already has and copies the
        // whole limit again on every grow past it.
        size_t ಅಂಕೆ_Previous_Bytes = memory->mallocated ? memory->mallocated->length + ಪರಿಮಾಣ(M3MemoryHeader) : 0;

        ಶೂನ್ಯ* newMem = m3_Realloc("Wasm Linear Memory", memory->mallocated, ಅಂಕೆ_Bytes, ಅಂಕೆ_Previous_Bytes);
        _throwifnull(newMem);

        memory->mallocated = (M3MemoryHeader*)newMem;

#endif

#ಯದಿ d_m3LogRuntime
        M3MemoryHeader* oldMallocated = memory->mallocated;
#endif

        io_runtime->memoryBytesUsed = otherUsed + ಅಂಕೆ_Page_Bytes;
        memory->ಅಂಕೆ_Pages            = ಅಂಕೆ_Pages_To_Alloc;

        memory->mallocated->length  = (size_t)ಅಂಕೆ_Page_Bytes;
        memory->mallocated->runtime = io_runtime;
        memory->mallocated->memory  = memory;

        memory->mallocated->max_ರಾಶಿ = (m3slot_t*)io_runtime->origin_ರಾಶಿ + io_runtime->ಅಂಕೆ_ರಾಶಿ_Slots;

        m3log(runtime, "resized old: %p; mem: %p; length: %zu; pages: %llu", oldMallocated, memory->mallocated, memory->mallocated->length, (unsigned long long)memory->ಅಂಕೆ_Pages);
    } ಅನ್ಯಥಾ {
        result = m3Err_wasmMemoryOverflow;
    }

    _catch: ಪ್ರತ್ಯಾಗಮ result;
}


ಶೂನ್ಯ FreeMemoryBlock (IM3Memory io_memory)
{
    ಯದಿ (io_memory->mallocated and io_memory->mallocated->runtime) {
        io_memory->mallocated->runtime->memoryBytesUsed -= io_memory->mallocated->length;
    }
#ಯದಿ d_m3GuardedMemory
    Guard_GiveSlot(io_memory->guardSlot);
    io_memory->guardSlot = ರಿಕ್ತ;

    io_memory->mallocated = ರಿಕ್ತ;
#ಅನ್ಯಥಾ
    m3_Free(io_memory->mallocated);
#endif
}


M3Result InitGlobals (IM3Module io_module)
{
    M3Result result = m3Err_none;

    ಯದಿ (io_module->ಅಂಕೆ_Globals) {
        // placing the globals in their structs isn't good for cache locality, but i don't really know what the global
        // access patterns typically look like yet.

        //          io_module->globalMemory = m3Alloc (m3reg_t, io_module->numGlobals);

        //          if (io_module->globalMemory)
        {
            ಪ್ರತಿ (u32 i = 0; i < io_module->ಅಂಕೆ_Globals; ++i) {
                M3Global* g = &io_module->globals[i];                           m3log (runtime, "initializing global: %d", i);

                ಯದಿ (g->init_ಅಭಿವ್ಯಕ್ತಿ) {
                    bytes_t start = g->init_ಅಭಿವ್ಯಕ್ತಿ;

                    result = EvaluateExpression(io_module, &g->i64_ಮೌಲ್ಯ, g->type, &start, g->init_ಅಭಿವ್ಯಕ್ತಿ + g->init_ಅಭಿವ್ಯಕ್ತಿ_Size);

                    ಯದಿ (not result) {
                        // io_module->globalMemory [i] = initValue;
                    } ಅನ್ಯಥಾ {
                        ಭಙ್ಗ;
                    }
                } ಅನ್ಯಥಾ {                                                        m3log (runtime, "importing global");
                }
            }
        }
        //          else result = ErrorModule (m3Err_mallocFailed, io_module, "could allocate globals for module: '%s", io_module->name);
    }

    ಪ್ರತ್ಯಾಗಮ result;
}


M3Result InitDataSegments (IM3Module io_module)
{
    M3Result result = m3Err_none;

    ಪ್ರತಿ (u32 i = 0; i < io_module->ಅಂಕೆ_Data_Segments; ++i) {
        M3DataSegment* segment = &io_module->dataSegments[i];

        // A passive segment stays available for memory.init until data.drop.
        // An active one is copied here and then counts as dropped.
        ಯದಿ (segment->isPassive) {
            ಅನುವರ್ತ;
        }

        _throwif("data segment memory index out of range",
                 segment->memoryRegion >= io_module->ಅಂಕೆ_Memories);

        IM3Memory io_memory = io_module->memories[segment->memoryRegion];

        _throwif("unallocated linear memory", !(io_memory->mallocated));

        // The offset expression has the memory's address type, and is
        // unsigned: an i32 offset of -1 is 4294967295, way out of bounds
        // rather than negative.
        u64     segmentOffset = 0;
        bytes_t start         = segment->init_ಅಭಿವ್ಯಕ್ತಿ;

        ಯದಿ (io_memory->isMemory64) {
_           (EvaluateExpression(io_module, &segmentOffset, c_m3Type_i64, &start, segment->init_ಅಭಿವ್ಯಕ್ತಿ + segment->init_ಅಭಿವ್ಯಕ್ತಿ_Size));
        } ಅನ್ಯಥಾ {
            u32 offset32;
_           (EvaluateExpression(io_module, &offset32, c_m3Type_i32, &start, segment->init_ಅಭಿವ್ಯಕ್ತಿ + segment->init_ಅಭಿವ್ಯಕ್ತಿ_Size));
            segmentOffset = offset32;
        }

        m3log(runtime, "loading data segment: %d; size: %d; offset: %llu", i, segment->size, (unsigned long long)segmentOffset);

        ಯದಿ (segmentOffset <= io_memory->mallocated->length &&
            (u64)segment->size <= io_memory->mallocated->length - segmentOffset) {
            u8* dest = m3MemData(io_memory->mallocated) + segmentOffset;
            memcpy(dest, segment->data, segment->size);
        } ಅನ್ಯಥಾ {
            _throw("data segment out of bounds");
        }

        segment->dropped = true;
    }

    _catch: ಪ್ರತ್ಯಾಗಮ result;
}


// Turns a segment's elements into references. Element expressions are constant
// expressions restricted to ref.null/ref.func, so they're read directly rather
// than run through the compiler.
ಸ್ಥಿರ
M3Result ResolveElements (IM3Module io_module, M3ElementSegment* i_segment, ಶೂನ್ಯ** o_elements)
{
    M3Result result = m3Err_none;

    bytes_t  pos = i_segment->elements;
    cbytes_t end = io_module->elementSectionEnd;

    ಪ್ರತಿ (u32 e = 0; e < i_segment->ಅಂಕೆ_Elements; ++e) {
        u32   funcIndex;
        ಶೂನ್ಯ* ref = ರಿಕ್ತ;

        ಯದಿ (i_segment->is_ಅಭಿವ್ಯಕ್ತಿ) {
            m3opcode_t opcode;
_           (Read_opcode(&opcode, &pos, end));

            ಯದಿ (opcode == c_waOp_refFunc) {
_               (ReadLEB_u32(&funcIndex, &pos, end));
                _throwif("function index out of range", funcIndex >= io_module->ಅಂಕೆ_Functions);
                ref = Function_Implementation(&io_module->functions[funcIndex]);
            } ಅನ್ಯಥಾ ಯದಿ (opcode == c_waOp_refNull) {
                i8 waType;
                u8 nullType;
_               (ReadLEB_i7(&waType, &pos, end));
_               (NormalizeType(&nullType, waType));
                _throwif(m3Err_typeMismatch, nullType != i_segment->type);
            } ಅನ್ಯಥಾ ಯದಿ (opcode == c_waOp_getGlobal) {
                // wasm 2.0 lets an element expression read an imported
                // immutable global, which is how one module seeds another's
                // table with a reference it exported.
                u32 globalIndex;
_               (ReadLEB_u32(&globalIndex, &pos, end));
                _throwif(m3Err_globaIndexOutOfBounds, globalIndex >= io_module->ಅಂಕೆ_Globals);

                IM3Global global = &io_module->globals[globalIndex];

                _throwif(m3Err_globaIndexOutOfBounds, not global->imported);
                _throwif(m3Err_wasmMalformed, global->isMutable);
                _throwif(m3Err_typeMismatch, BaseTypeOf(global->type) != BaseTypeOf(i_segment->type));

                // read the cell the import was linked to, not the placeholder
                ಯದಿ (global->resolved) {
                    global = global->resolved;
                }

                ref = global->ref_ಮೌಲ್ಯ;
            } ಅನ್ಯಥಾ {
                _throw("constant expression required");
            }

_           (Read_opcode(&opcode, &pos, end));
            _throwif(m3Err_wasmMalformed, opcode != c_waOp_end);
        } ಅನ್ಯಥಾ {
_           (ReadLEB_u32(&funcIndex, &pos, end));
            _throwif("function index out of range", funcIndex >= io_module->ಅಂಕೆ_Functions);
            ref = Function_Implementation(&io_module->functions[funcIndex]);
        }

        o_elements[e] = ref;
    }

    _catch: ಪ್ರತ್ಯಾಗಮ result;
}


M3Result InitTableAndElements (IM3Module io_module)
{
    M3Result result = m3Err_none;

    cbytes_t end = io_module->elementSectionEnd;
    M3Table* table;

    ಪ್ರತಿ (u32 i = 0; i < io_module->ಅಂಕೆ_Tables; ++i) {
        table = io_module->tables[i];

        // a slot pointing at another module's table is that module's to fill
        ಯದಿ (table->owner != io_module) {
            ಅನುವರ್ತ;
        }

        ಯದಿ (table->size) {
            IM3Runtime runtime = io_module->runtime;
            _throwif(m3Err_tableLimitExceeded, runtime->tableElementsLimit and
                                                 table->size > runtime->tableElementsLimit - runtime->tableElementsUsed);
            table->elements = m3_AllocArray(ಶೂನ್ಯ*, table->size);
            _throwifnull(table->elements);
            runtime->tableElementsUsed += table->size;

            ಯದಿ (table->init_ಅಭಿವ್ಯಕ್ತಿ) {
                ಶೂನ್ಯ*   ಮೌಲ್ಯ = ರಿಕ್ತ;
                bytes_t start = table->init_ಅಭಿವ್ಯಕ್ತಿ;
_               (EvaluateExpression(io_module, &ಮೌಲ್ಯ, BaseTypeOf(table->type),
                                     &start, table->init_ಅಭಿವ್ಯಕ್ತಿ + table->init_ಅಭಿವ್ಯಕ್ತಿ_Size));

                ಪ್ರತಿ (u32 e = 0; e < table->size; ++e) {
                    table->elements[e] = ಮೌಲ್ಯ;
                }
            }
        }
    }

    ಪ್ರತಿ (u32 i = 0; i < io_module->ಅಂಕೆ_Element_Segments; ++i) {
        M3ElementSegment* segment = &io_module->elementSegments[i];

        // Declarative segments only make their functions referenceable, and
        // passive ones wait for table.init, so neither is written out here.
        ಯದಿ (segment->mode == c_m3Elem_declarative) {
            segment->dropped = true;
            ಅನುವರ್ತ;
        }

        ಯದಿ (segment->mode == c_m3Elem_passive) {
            ಯದಿ (segment->ಅಂಕೆ_Elements) {
                segment->resolved = m3_AllocArray(ಶೂನ್ಯ*, segment->ಅಂಕೆ_Elements);
                _throwifnull(segment->resolved);
_               (ResolveElements(io_module, segment, segment->resolved));
            }
            ಅನುವರ್ತ;
        }

        table = io_module->tables[segment->tableIndex];

        // The offset expression has the table's index type, and is unsigned:
        // an i32 offset of -1 is 4294967295, out of bounds rather than negative.
        u64     offset = 0;
        bytes_t ಅಭಿವ್ಯಕ್ತಿ   = segment->init_ಅಭಿವ್ಯಕ್ತಿ;

        ಯದಿ (table->isTable64) {
_           (EvaluateExpression(io_module, &offset, c_m3Type_i64, &ಅಭಿವ್ಯಕ್ತಿ, end));
        } ಅನ್ಯಥಾ {
            u32 offset32;
_           (EvaluateExpression(io_module, &offset32, c_m3Type_i32, &ಅಭಿವ್ಯಕ್ತಿ, end));
            offset = offset32;
        }

        _throwif("out of bounds table access",
                 offset > table->size or segment->ಅಂಕೆ_Elements > table->size - offset);

_       (ResolveElements(io_module, segment, table->elements + offset));

        segment->dropped = true;
    }

    _catch: ಪ್ರತ್ಯಾಗಮ result;
}

M3Result m3_CompileModule (IM3Module io_module)
{
    M3Result result = m3Err_none;

    ಪ್ರತಿ (u32 i = 0; i < io_module->ಅಂಕೆ_Functions; ++i) {
        IM3Function f = &io_module->functions[i];
        ಯದಿ (f->wasm and not f->compiled) {
_           (CompileFunction(f));
        }
    }

    _catch: ಪ್ರತ್ಯಾಗಮ result;
}

#ಯದಿ d_m3HasExceptionHandling

M3Exception* NewException (IM3Runtime io_runtime, IM3Tag i_tag, u32 i_ಅಂಕೆ_Args)
{
    M3Exception* exception = (M3Exception*)m3_Malloc("M3Exception", ಪರಿಮಾಣ(M3Exception) + i_ಅಂಕೆ_Args * ಪರಿಮಾಣ(u64));

    ಯದಿ (exception) {
        exception->tag     = i_tag;
        exception->ಅಂಕೆ_Args = i_ಅಂಕೆ_Args;
        exception->reified = false;
        exception->prev    = ರಿಕ್ತ;
        exception->next    = io_runtime->exceptions;

        ಯದಿ (exception->next) {
            exception->next->prev = exception;
        }

        io_runtime->exceptions = exception;
    }

    ಪ್ರತ್ಯಾಗಮ exception;
}


// Releases one exception ahead of the rest. The caller has to know nothing can
// still name it: no exnref was ever taken of it, and its payload has already
// been copied out.
ಶೂನ್ಯ FreeException (IM3Runtime io_runtime, M3Exception* i_exception)
{
    ಯದಿ (i_exception->prev) {
        i_exception->prev->next = i_exception->next;
    } ಅನ್ಯಥಾ {
        io_runtime->exceptions = i_exception->next;
    }

    ಯದಿ (i_exception->next) {
        i_exception->next->prev = i_exception->prev;
    }

    ಯದಿ (io_runtime->pendingException == i_exception) {
        io_runtime->pendingException = ರಿಕ್ತ;
    }

    m3_Free_Impl(i_exception);
}


// Releases every exception the runtime still holds. Only safe once the Wasm
// stack is empty, which is why the outermost RunCodeChecked() is the one that
// calls it.
ಶೂನ್ಯ FreeExceptions (IM3Runtime io_runtime)
{
    M3Exception* exception = io_runtime->exceptions;

    io_runtime->exceptions       = ರಿಕ್ತ;
    io_runtime->pendingException = ರಿಕ್ತ;

    ಯಾವತ್ (exception) {
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
ಸ್ಥಿರ
ಶೂನ್ಯ RecordUncaughtException (IM3Runtime io_runtime, M3Result i_result)
{
    ಯದಿ (io_runtime->uncaughtException) {
        m3_Free(io_runtime->uncaughtException);
    }

    M3Exception* exception = io_runtime->pendingException;

    ಯದಿ (M3_UNLIKELY(i_result == m3Err_trapUncaughtException) and exception) {
        M3Exception* copy = (M3Exception*)m3_CopyMem(exception, ಪರಿಮಾಣ(M3Exception) + exception->ಅಂಕೆ_Args * ಪರಿಮಾಣ(u64));

        ಯದಿ (copy) {
            copy->next = ರಿಕ್ತ;
            copy->prev = ರಿಕ್ತ;
        }

        io_runtime->uncaughtException = copy;
    }
}

#endif // d_m3HasExceptionHandling


#ಯದಿ d_m3HasExceptionHandling || d_m3_Has_ರಾಶಿ_Switching

IM3Tag m3_FindTag (IM3Module i_module, ಧ್ರುವ ವರ್ಣ* ಧ್ರುವ i_tagName)
{
    ಯದಿ (not i_module or not i_tagName) {
        ಪ್ರತ್ಯಾಗಮ ರಿಕ್ತ;
    }

    ಪ್ರತ್ಯಾಗಮ Module_FindExportedTag(i_module, i_tagName);
}


M3Result m3_NewTag (IM3Runtime io_runtime, IM3Tag* o_tag, ಧ್ರುವ ವರ್ಣ* ಧ್ರುವ i_signature)
{
    IM3FuncType type    = ರಿಕ್ತ;
    M3HostTag*  hostTag = ರಿಕ್ತ;

_try {
    _throwif("null tag", not o_tag);
    *o_tag = ರಿಕ್ತ;
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
    ಪ್ರತ್ಯಾಗಮ result;
}


M3Result m3_LinkTag (IM3Module         io_module,
                     ಧ್ರುವ ವರ್ಣ* ಧ್ರುವ i_moduleName,
                     ಧ್ರುವ ವರ್ಣ* ಧ್ರುವ i_tagName,
                     IM3Tag            i_tag)
{
    ಯದಿ (not io_module or not i_moduleName or not i_tagName or not i_tag) {
        ಪ್ರತ್ಯಾಗಮ m3Err_tagLookupFailed;
    }

    ಧ್ರುವ bool wildcardModule = (strcmp(i_moduleName, "*") == 0);
    IM3Tag     target         = i_tag->resolved ? i_tag->resolved : i_tag;

    M3Result result = m3Err_tagLookupFailed;

    ಪ್ರತಿ (u32 i = 0; i < io_module->ಅಂಕೆ_Tags; ++i) {
        IM3Tag tag = &io_module->tags[i];

        ಯದಿ (tag->imported and tag->import.moduleUtf8 and tag->import.fieldUtf8 and
            strcmp(tag->import.fieldUtf8, i_tagName) == 0 and
            (wildcardModule or strcmp(tag->import.moduleUtf8, i_moduleName) == 0)) {
            ಯದಿ (tag->type != target->type) {
                ಪ್ರತ್ಯಾಗಮ m3Err_incompatibleImportType;
            }

            tag->resolved = target;
            result        = m3Err_none;
        }
    }

    ಪ್ರತ್ಯಾಗಮ result;
}


uint32_t m3_Get_Tag_Arg_ಎಣಿಕೆ (IM3Tag i_tag)
{
    ಪ್ರತ್ಯಾಗಮ i_tag ? i_tag->type->ಅಂಕೆ_Args : 0;
}


M3_ಮೌಲ್ಯ_Type m3_GetTagArgType (IM3Tag i_tag, uint32_t i_index)
{
    ಯದಿ (i_tag and i_index < i_tag->type->ಅಂಕೆ_Args) {
        ಪ್ರತ್ಯಾಗಮ (M3_ಮೌಲ್ಯ_Type)BaseTypeOf(d_FuncArgType(i_tag->type, i_index));
    }

    ಪ್ರತ್ಯಾಗಮ c_m3Type_none;
}

#ಅನ್ಯಥಾ

IM3Tag m3_FindTag (IM3Module i_module, ಧ್ರುವ ವರ್ಣ* ಧ್ರುವ i_tagName)
{
    ಪ್ರತ್ಯಾಗಮ ರಿಕ್ತ;
}

M3Result m3_NewTag (IM3Runtime io_runtime, IM3Tag* o_tag, ಧ್ರುವ ವರ್ಣ* ಧ್ರುವ i_signature)
{
    ಯದಿ (o_tag) {
        *o_tag = ರಿಕ್ತ;
    }
    ಪ್ರತ್ಯಾಗಮ m3Err_tagLookupFailed;
}

M3Result m3_LinkTag (IM3Module io_module, ಧ್ರುವ ವರ್ಣ* ಧ್ರುವ i_moduleName, ಧ್ರುವ ವರ್ಣ* ಧ್ರುವ i_tagName, IM3Tag i_tag)
{
    ಪ್ರತ್ಯಾಗಮ m3Err_tagLookupFailed;
}

uint32_t m3_Get_Tag_Arg_ಎಣಿಕೆ (IM3Tag i_tag)
{
    ಪ್ರತ್ಯಾಗಮ 0;
}

M3_ಮೌಲ್ಯ_Type m3_GetTagArgType (IM3Tag i_tag, uint32_t i_index)
{
    ಪ್ರತ್ಯಾಗಮ c_m3Type_none;
}

#endif // d_m3HasExceptionHandling || d_m3HasStackSwitching


#ಯದಿ d_m3HasExceptionHandling

M3Result m3_ThrowException (IM3Runtime io_runtime, IM3Tag i_tag, uint32_t i_argc, ಧ್ರುವ ಶೂನ್ಯ* i_argptrs[])
{
    ಯದಿ (not io_runtime or not i_tag) {
        ಪ್ರತ್ಯಾಗಮ m3Err_tagLookupFailed;
    }

    IM3Tag      tag  = i_tag->resolved ? i_tag->resolved : i_tag;
    IM3FuncType type = tag->type;

    // a tag with results is a stack switching control tag, which is suspended
    // with rather than thrown
    ಯದಿ (type->ಅಂಕೆ_Rets) {
        ಪ್ರತ್ಯಾಗಮ m3Err_typeMismatch;
    }
    ಯದಿ (i_argc != type->ಅಂಕೆ_Args) {
        ಪ್ರತ್ಯಾಗಮ m3_Err_argument_ಎಣಿಕೆ_Mismatch;
    }
    // with nothing on the stack there is nothing to unwind, and the exception
    // would sit on the runtime's list until some later call ended
    ಯದಿ (io_runtime->callNesting == 0) {
        ಪ್ರತ್ಯಾಗಮ "m3_ThrowException called outside a host function";
    }

    M3Exception* exception = NewException(io_runtime, tag, i_argc);
    ಯದಿ (not exception) {
        ಪ್ರತ್ಯಾಗಮ m3Err_mallocFailed;
    }

    // the layout op_Throw gives a payload: 32-bit values zero-extended, floats
    // as their bit pattern
    ಪ್ರತಿ (u32 i = 0; i < i_argc; ++i) {
        ಧ್ರುವ ಶೂನ್ಯ* ಮೌಲ್ಯ = i_argptrs[i];

        ವಿಕಲ್ಪ (BaseTypeOf(d_FuncArgType(type, i))) {
        ಪಕ್ಷ c_m3Type_i32:
        ಪಕ್ಷ c_m3Type_f32: {
            u32 bits;
            memcpy(&bits, ಮೌಲ್ಯ, ಪರಿಮಾಣ(bits));
            exception->args[i] = bits;
            ಭಙ್ಗ;
        }
        ಪಕ್ಷ c_m3Type_i64:
        ಪಕ್ಷ c_m3Type_f64:
            memcpy(&exception->args[i], ಮೌಲ್ಯ, ಪರಿಮಾಣ(u64));
            ಭಙ್ಗ;
        ಪಕ್ಷ c_m3Type_funcref:
        ಪಕ್ಷ c_m3Type_externref:
        ಪಕ್ಷ c_m3Type_exnref:
        ಪಕ್ಷ c_m3Type_contref:
            exception->args[i] = (u64)(*(ಧ್ರುವ uintptr_t*)ಮೌಲ್ಯ);
            ಭಙ್ಗ;
        ಸ್ವಭಾವ:
            FreeException(io_runtime, exception);
            ಪ್ರತ್ಯಾಗಮ "unknown argument type";
        }
    }

    io_runtime->pendingException = exception;

    ಪ್ರತ್ಯಾಗಮ m3Err_pendingException;
}


IM3Tag m3_GetExceptionTag (IM3Runtime i_runtime)
{
    ಯದಿ (i_runtime and i_runtime->uncaughtException) {
        ಪ್ರತ್ಯಾಗಮ i_runtime->uncaughtException->tag;
    }

    ಪ್ರತ್ಯಾಗಮ ರಿಕ್ತ;
}


M3Result m3_GetExceptionArgs (IM3Runtime i_runtime, uint32_t i_argc, ಧ್ರುವ ಶೂನ್ಯ* o_argptrs[])
{
    ಯದಿ (not i_runtime or not i_runtime->uncaughtException) {
        ಪ್ರತ್ಯಾಗಮ "no uncaught exception";
    }

    ಧ್ರುವ M3Exception* exception = i_runtime->uncaughtException;
    IM3FuncType        type      = exception->tag->type;

    ಯದಿ (i_argc != exception->ಅಂಕೆ_Args) {
        ಪ್ರತ್ಯಾಗಮ m3_Err_argument_ಎಣಿಕೆ_Mismatch;
    }

    ಪ್ರತಿ (u32 i = 0; i < i_argc; ++i) {
        ಶೂನ್ಯ* ಮೌಲ್ಯ = (ಶೂನ್ಯ*)o_argptrs[i];
        u64   bits  = exception->args[i];

        ವಿಕಲ್ಪ (BaseTypeOf(d_FuncArgType(type, i))) {
        ಪಕ್ಷ c_m3Type_i32:
        ಪಕ್ಷ c_m3Type_f32: {
            u32 ಕೆಳಗಿನ = (u32)bits;
            memcpy(ಮೌಲ್ಯ, &ಕೆಳಗಿನ, ಪರಿಮಾಣ(ಕೆಳಗಿನ));
            ಭಙ್ಗ;
        }
        ಪಕ್ಷ c_m3Type_i64:
        ಪಕ್ಷ c_m3Type_f64:
            memcpy(ಮೌಲ್ಯ, &bits, ಪರಿಮಾಣ(bits));
            ಭಙ್ಗ;
        ಪಕ್ಷ c_m3Type_funcref:
        ಪಕ್ಷ c_m3Type_externref:
        ಪಕ್ಷ c_m3Type_exnref:
        ಪಕ್ಷ c_m3Type_contref:
            *(uintptr_t*)ಮೌಲ್ಯ = (uintptr_t)bits;
            ಭಙ್ಗ;
        ಸ್ವಭಾವ:
            ಪ್ರತ್ಯಾಗಮ "unknown argument type";
        }
    }

    ಪ್ರತ್ಯಾಗಮ m3Err_none;
}

#ಅನ್ಯಥಾ

M3Result m3_ThrowException (IM3Runtime io_runtime, IM3Tag i_tag, uint32_t i_argc, ಧ್ರುವ ಶೂನ್ಯ* i_argptrs[])
{
    ಪ್ರತ್ಯಾಗಮ m3Err_tagLookupFailed;
}

IM3Tag m3_GetExceptionTag (IM3Runtime i_runtime)
{
    ಪ್ರತ್ಯಾಗಮ ರಿಕ್ತ;
}

M3Result m3_GetExceptionArgs (IM3Runtime i_runtime, uint32_t i_argc, ಧ್ರುವ ಶೂನ್ಯ* o_argptrs[])
{
    ಪ್ರತ್ಯಾಗಮ "no uncaught exception";
}

#endif // d_m3HasExceptionHandling


#ಯದಿ d_m3GuardedMemory

// What RunCodeProtected hands to the platform layer to run, and what comes back
ಪ್ರಕಾರನಿರ್ದೇಶ ಸಂರಚನಾ M3ProtectedRun {
    pc_t            pc;
    IM3Runtime      runtime;
    M3MemoryHeader* mem;
    M3Result        result;
} M3ProtectedRun;

ಸ್ಥಿರ
ಶೂನ್ಯ RunCodeBody (ಶೂನ್ಯ* io_context)
{
    M3ProtectedRun* run = (M3ProtectedRun*)io_context;

#  ಯದಿ (d_m3EnableOpProfiling || d_m3EnableOpTracing)
    run->result = (M3Result)RunCode(run->pc, (m3stack_t)run->runtime->ರಾಶಿ, run->mem,
                                    d_m3OpDefaultArgs, d_m3BaseCstr);
#  ಅನ್ಯಥಾ
    run->result = (M3Result)RunCode(run->pc, (m3stack_t)run->runtime->ರಾಶಿ, run->mem,
                                    d_m3OpDefaultArgs);
#  endif
}

// The bounds check the load and store operations no longer do, done once here by the
// system instead: an address past the end of a memory reaches the uncommitted part
// of that memory's reservation, and the fault comes back as the trap the spec asks
// for. Nothing is left half-done by it - the Wasm stack and the memories are heap,
// and the caller unwinds the rest.
ಸ್ಥಿರ
M3Result RunCodeProtected (IM3Runtime i_runtime, pc_t i_pc, M3MemoryHeader* i_mem)
{
    // Memory operations in this build carry no bounds check, so code only runs where a
    // fault past the end of a memory can still be turned into a trap
    ಯದಿ (M3_UNLIKELY(not m3_HostGuardsActive())) {
        ಪ್ರತ್ಯಾಗಮ ErrorRuntime(m3Err_memoryGuardsUnavailable, i_runtime,
                            "another part of the process holds the fault signal");
    }

    M3ProtectedRun run;
    run.pc      = i_pc;
    run.runtime = i_runtime;
    run.mem     = i_mem;
    run.result  = m3Err_none;

    ಶೂನ್ಯ*  arena;
    size_t arenaBytes;
    Guard_ArenaRange(&arena, &arenaBytes);

    ಯದಿ (not m3_HostProtectedCall(RunCodeBody, &run, arena, arenaBytes)) {
        ಪ್ರತ್ಯಾಗಮ m3Err_trapOutOfBoundsMemoryAccess;
    }

    ಪ್ರತ್ಯಾಗಮ run.result;
}

#endif // d_m3GuardedMemory


#ಯದಿ d_m3_Has_ರಾಶಿ_Switching

// The function a suspended continuation stopped in: the callee of its innermost
// call frame, or the function it was entered with if it made no call. Frames are
// recorded innermost first, so that is the lowest-numbered call frame.
ಸ್ಥಿರ
IM3Function Continuation_InnermostFunction (IM3Continuation i_cont)
{
    ಪ್ರತಿ (u32 i = 0; i < i_cont->ಅಂಕೆ_Frames; ++i) {
        ಯದಿ (i_cont->frames[i].kind == frame_call && i_cont->frames[i].call.function) {
            ಪ್ರತ್ಯಾಗಮ i_cont->frames[i].call.function;
        }
    }

    ಪ್ರತ್ಯಾಗಮ i_cont->entryFunction;
}


// Where the host's next call puts its arguments and finds its results. A
// suspended root keeps its frames on the bottom of the runtime's stack, so a call
// made before it is resumed starts above the last of them rather than on top of
// them. The top is the innermost function's frame base plus everything that
// function can use. That base is the root's own sp, unless it stopped inside a
// resume: then the suspension point belongs to the resumed continuation, and the
// root's innermost frame is that resume.
ಶೂನ್ಯ Runtime_Place_Call_ರಾಶಿ (IM3Runtime io_runtime)
{
    IM3Continuation ಬೇರು = io_runtime->ಬೇರು_Continuation;

    io_runtime->ರಾಶಿ = io_runtime->origin_ರಾಶಿ;

    ಯದಿ (not ಬೇರು or ಬೇರು->state != cont_suspended) {
        ಪ್ರತ್ಯಾಗಮ;
    }

    m3stack_t   sp       = ಬೇರು->sp;
    IM3Function function = Continuation_InnermostFunction(ಬೇರು);

    ಯದಿ (ಬೇರು->ಅಂಕೆ_Frames and ಬೇರು->frames[0].kind == frame_resume) {
        sp = ಬೇರು->frames[0].sp;
    }

    m3slot_t* base = (m3slot_t*)io_runtime->origin_ರಾಶಿ;
    m3slot_t* ತುದಿ  = (m3slot_t*)sp + (function ? function->max_ರಾಶಿ_Slots : 0);

    ಯದಿ (ತುದಿ > base + io_runtime->ಅಂಕೆ_ರಾಶಿ_Slots) {
        ತುದಿ = base + io_runtime->ಅಂಕೆ_ರಾಶಿ_Slots;
    }

    io_runtime->ರಾಶಿ = ತುದಿ;
}

#endif // d_m3HasStackSwitching


// Run compiled code on the runtime's stack, bounding native recursion for the
// duration of the call. The outermost invocation establishes the stack limit;
// nested ones (an imported function calling back into Wasm) inherit it.
ಸ್ಥಿರ ಅನ್ತಃಸ್ಥ
M3Result RunCodeChecked (IM3Runtime i_runtime, IM3Function i_function)
{
    pc_t i_pc = i_function->compiled;

    // execution runs against the memory of the module the entry point belongs
    // to, not against some runtime-wide one
    M3MemoryHeader* _mem = Module_MemoryHeader(i_function->module);

#ಯದಿ d_m3HasSnapshots
    ಯದಿ (M3_UNLIKELY(i_function->module->isUnusable)) {
        ಪ್ರತ್ಯಾಗಮ "a snapshot failed to restore into the module, so it cannot run";
    }
    i_function->module->hasRun = true;
#endif

    d_m3_ರಾಶಿ_Limit_Enter(i_runtime);

    // a host function calling back into Wasm does not change which call a trap
    // ends: that is still the one the host made first
    ಯದಿ (i_runtime->callNesting++ == 0) {
        i_runtime->entered = i_function;
    }
#ಯದಿ d_m3HasExceptionHandling
    // handler stacks don't nest across a call boundary: a host function calling
    // back into Wasm cannot be caught by a try_table its own caller entered
    u32 savedTryDepth   = i_runtime->tryDepth;
    i_runtime->tryDepth = 0;
#endif
#ಯದಿ d_m3_Has_ರಾಶಿ_Switching
    // nor do continuations: the native frames on the far side of a host call
    // are not ours to record, so a suspend in here has no handler to reach and
    // no resume to unwind to
    IM3Continuation savedContinuation = i_runtime->activeContinuation;

    // A suspended root continuation is a paused program waiting to be resumed,
    // not scratch space. A call the host makes before resuming it gets a
    // context of its own, the way any other re-entry does, rather than having
    // the frames of the suspension reset out from under it.
    bool claim_ಬೇರು = i_runtime->isSuspendable and not savedContinuation and
                     not (i_runtime->ಬೇರು_Continuation and
                          i_runtime->ಬೇರು_Continuation->state == cont_suspended);

    ಯದಿ (claim_ಬೇರು and not i_runtime->ಬೇರು_Continuation) {
        i_runtime->ಬೇರು_Continuation = Continuation_New(i_runtime, i_function->funcType, i_function);

        ಯದಿ (i_runtime->ಬೇರು_Continuation) {
            // The root borrows the runtime's stack and is not charged as cont.new.
            i_runtime->ಬೇರು_Continuation->ಅಂಕೆ_ರಾಶಿ_Slots = i_runtime->ಅಂಕೆ_ರಾಶಿ_Slots;
        } ಅನ್ಯಥಾ {
            claim_ಬೇರು = false;
        }
    }

    ಯದಿ (claim_ಬೇರು) {
        i_runtime->activeContinuation              = i_runtime->ಬೇರು_Continuation;
        i_runtime->ಬೇರು_Continuation->entryFunction = i_function;
        i_runtime->ಬೇರು_Continuation->state         = cont_running;
        i_runtime->ಬೇರು_Continuation->ಅಂಕೆ_Frames     = 0;
    } ಅನ್ಯಥಾ {
        i_runtime->activeContinuation = ರಿಕ್ತ;
    }
#endif
#ಯದಿ d_m3GuardedMemory
    M3Result result = RunCodeProtected(i_runtime, i_pc, _mem);
#ನೋಚೇದ್ಯದಿ (d_m3EnableOpProfiling || d_m3EnableOpTracing)
    M3Result result = (M3Result)RunCode(i_pc, (m3stack_t)i_runtime->ರಾಶಿ, _mem, d_m3OpDefaultArgs, d_m3BaseCstr);
#ಅನ್ಯಥಾ
    M3Result result = (M3Result)RunCode(i_pc, (m3stack_t)i_runtime->ರಾಶಿ, _mem, d_m3OpDefaultArgs);
#endif
#ಯದಿ d_m3_Has_ರಾಶಿ_Switching
    ಯದಿ (claim_ಬೇರು) {
        i_runtime->ಬೇರು_Continuation->state =
          (result == m3Err_continuationSuspended) ? cont_suspended : cont_returned;

        Runtime_Place_Call_ರಾಶಿ(i_runtime);
    }
    i_runtime->activeContinuation = savedContinuation;
#endif
#ಯದಿ d_m3HasExceptionHandling
    i_runtime->tryDepth = savedTryDepth;

    // an exception that reached the bottom of the call stack found no handler
    ಯದಿ (M3_UNLIKELY(result == m3Err_pendingException)) {
        result = m3Err_trapUncaughtException;
    }
    RecordUncaughtException(i_runtime, result);

    // a paused invocation can still reach the exceptions it holds, so they wait
    // for it to finish
    ಯದಿ (i_runtime->callNesting == 1 and not m3_IsSuspended(i_runtime)) {
        FreeExceptions(i_runtime);
    }
#endif
    i_runtime->callNesting--;
    d_m3_ರಾಶಿ_Limit_Leave(i_runtime);

    ಪ್ರತ್ಯಾಗಮ result;
}

M3Result m3_RunStart (IM3Module io_module)
{
#ifdef FUZZING_BUILD_MODE_UNSAFE_FOR_PRODUCTION
    // Execution disabled for fuzzing builds
    ಪ್ರತ್ಯಾಗಮ m3Err_none;
#endif

    M3Result result           = m3Err_none;
    i32      startFunctionTmp = -1;

    ಯದಿ (io_module and io_module->startFunction >= 0) {
        IM3Function function = &io_module->functions[io_module->startFunction];

        ಯದಿ (not function->compiled) {
_           (CompileFunction(function));
        }

        IM3FuncType ftype = function->funcType;
        ಯದಿ (ftype->ಅಂಕೆ_Args != 0 || ftype->ಅಂಕೆ_Rets != 0) {
            _throw(m3_Err_argument_ಎಣಿಕೆ_Mismatch);
        }

        IM3Module  module  = function->module;
        IM3Runtime runtime = module->runtime;

        startFunctionTmp = io_module->startFunction;

        io_module->startFunction = -1;

        result = RunCodeChecked(runtime, function);

        // a start function that suspended is not to be started again: resuming
        // is what finishes it
        ಯದಿ (result and result != m3Err_continuationSuspended) {
            io_module->startFunction = startFunctionTmp;
            EXCEPTION_PRINT(result);
            ಗಚ್ಛ _catch;
        }
    }

    _catch: ಪ್ರತ್ಯಾಗಮ result;
}

// TODO: deal with main + side-modules loading efforcement
M3Result m3_LoadModule (IM3Runtime io_runtime, IM3Module io_module)
{
    M3Result result = m3Err_none;

    ಯದಿ (M3_UNLIKELY(io_module->runtime)) {
        ಪ್ರತ್ಯಾಗಮ m3Err_moduleAlreadyLinked;
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
    ಪ್ರತ್ಯಾಗಮ result; // ok

_catch:
    // The runtime owns the module either way. Instantiation may already have
    // written this module's functions into a table another module owns, and the
    // spec keeps whatever it managed to do before the trap, so those entries
    // stay callable - retaining the module is what stops them dangling.
    //
    // It goes on the tail of the list rather than the head: it is not a module
    // anyone should find by name, only one whose functions may still be
    // reachable through someone else's table.
    io_module->next = ರಿಕ್ತ;

    IM3Module* tail = &io_runtime->modules;
    ಯಾವತ್ (*tail) {
        tail = &(*tail)->next;
    }
    *tail = io_module;

    ಪ್ರತ್ಯಾಗಮ result;
}

IM3Global m3_FindGlobal (IM3Module         io_module,
                         ಧ್ರುವ ವರ್ಣ* ಧ್ರುವ i_globalName)
{
    // Search exports
    IM3Global exported = Module_FindExportedGlobal(io_module, i_globalName);
    ಯದಿ (exported) {
        // a re-exported global is the one that was imported, so reads and
        // writes have to reach the cell that actually holds the value
        ಪ್ರತ್ಯಾಗಮ exported->resolved ? exported->resolved : exported;
    }

    // Search imports
    ಪ್ರತಿ (u32 i = 0; i < io_module->ಅಂಕೆ_Globals; ++i) {
        IM3Global g = &io_module->globals[i];

        ಯದಿ (g->import.moduleUtf8 and g->import.fieldUtf8) {
            ಯದಿ (strcmp(g->import.fieldUtf8, i_globalName) == 0) {
                ಪ್ರತ್ಯಾಗಮ g->resolved ? g->resolved : g;
            }
        }
    }
    ಪ್ರತ್ಯಾಗಮ ರಿಕ್ತ;
}

M3Result m3_GetGlobal (IM3Global      i_global,
                       IM3_Tagged_ಮೌಲ್ಯ o_ಮೌಲ್ಯ)
{
    ಯದಿ (not i_global) {
        ಪ್ರತ್ಯಾಗಮ m3Err_globalLookupFailed;
    }

    ವಿಕಲ್ಪ (i_global->type) {
    ಪಕ್ಷ c_m3Type_i32: o_ಮೌಲ್ಯ->ಮೌಲ್ಯ.i32 = i_global->i32_ಮೌಲ್ಯ; ಭಙ್ಗ;
    ಪಕ್ಷ c_m3Type_i64: o_ಮೌಲ್ಯ->ಮೌಲ್ಯ.i64 = i_global->i64_ಮೌಲ್ಯ; ಭಙ್ಗ;
#ಯದಿ d_m3HasFloat
    ಪಕ್ಷ c_m3Type_f32: o_ಮೌಲ್ಯ->ಮೌಲ್ಯ.f32 = i_global->f32_ಮೌಲ್ಯ; ಭಙ್ಗ;
    ಪಕ್ಷ c_m3Type_f64: o_ಮೌಲ್ಯ->ಮೌಲ್ಯ.f64 = i_global->f64_ಮೌಲ್ಯ; ಭಙ್ಗ;
#endif
    ಸ್ವಭಾವ: ಪ್ರತ್ಯಾಗಮ m3Err_invalidTypeId;
    }

    o_ಮೌಲ್ಯ->type = (M3_ಮೌಲ್ಯ_Type)(i_global->type);
    ಪ್ರತ್ಯಾಗಮ m3Err_none;
}

M3Result m3_SetGlobal (IM3Global            i_global,
                       ಧ್ರುವ IM3_Tagged_ಮೌಲ್ಯ i_ಮೌಲ್ಯ)
{
    ಯದಿ (not i_global) {
        ಪ್ರತ್ಯಾಗಮ m3Err_globalLookupFailed;
    }
    ಯದಿ (not i_global->isMutable) {
        ಪ್ರತ್ಯಾಗಮ m3Err_globalNotMutable;
    }
    ಯದಿ (i_global->type != i_ಮೌಲ್ಯ->type) {
        ಪ್ರತ್ಯಾಗಮ m3Err_globalTypeMismatch;
    }

    ವಿಕಲ್ಪ (i_ಮೌಲ್ಯ->type) {
    ಪಕ್ಷ c_m3Type_i32: i_global->i32_ಮೌಲ್ಯ = i_ಮೌಲ್ಯ->ಮೌಲ್ಯ.i32; ಭಙ್ಗ;
    ಪಕ್ಷ c_m3Type_i64: i_global->i64_ಮೌಲ್ಯ = i_ಮೌಲ್ಯ->ಮೌಲ್ಯ.i64; ಭಙ್ಗ;
#ಯದಿ d_m3HasFloat
    ಪಕ್ಷ c_m3Type_f32: i_global->f32_ಮೌಲ್ಯ = i_ಮೌಲ್ಯ->ಮೌಲ್ಯ.f32; ಭಙ್ಗ;
    ಪಕ್ಷ c_m3Type_f64: i_global->f64_ಮೌಲ್ಯ = i_ಮೌಲ್ಯ->ಮೌಲ್ಯ.f64; ಭಙ್ಗ;
#endif
    ಸ್ವಭಾವ: ಪ್ರತ್ಯಾಗಮ m3Err_invalidTypeId;
    }

    ಪ್ರತ್ಯಾಗಮ m3Err_none;
}

M3_ಮೌಲ್ಯ_Type m3_GetGlobalType (IM3Global i_global)
{
    ಪ್ರತ್ಯಾಗಮ (i_global) ? (M3_ಮೌಲ್ಯ_Type)(i_global->type) : c_m3Type_none;
}


ಶೂನ್ಯ* v_FindFunction (IM3Module i_module, ಶೂನ್ಯ* i_info)
{
    ಧ್ರುವ ವರ್ಣ* ಧ್ರುವ i_name = (ಧ್ರುವ ವರ್ಣ*)i_info;

    // Prefer exported functions
    IM3Function exported = Module_FindExportedFunction(i_module, i_name);
    ಯದಿ (exported) {
        ಪ್ರತ್ಯಾಗಮ exported;
    }

    // Search internal functions
    ಪ್ರತಿ (u32 i = 0; i < i_module->ಅಂಕೆ_Functions; ++i) {
        IM3Function f = &i_module->functions[i];

        bool isImported = f->import.moduleUtf8 or f->import.fieldUtf8;

        ಯದಿ (isImported) {
            ಅನುವರ್ತ;
        }

        ಪ್ರತಿ (ಪೂರ್ಣಾಙ್ಕ j = 0; j < f->ಅಂಕೆ_Names; j++) {
            ಯದಿ (f->names[j] and strcmp(f->names[j], i_name) == 0) {
                ಪ್ರತ್ಯಾಗಮ f;
            }
        }
    }

    ಪ್ರತ್ಯಾಗಮ ರಿಕ್ತ;
}


// Shared tail of the two lookups: a function is only usable once it has code.
ಸ್ಥಿರ
M3Result PrepareFoundFunction (IM3Function* o_function, IM3Function i_function)
{
    M3Result result = m3Err_none;

    ಯದಿ (not i_function->compiled) {
_       (CompileFunction(i_function))
    }

    _catch:
    *o_function = result ? ರಿಕ್ತ : i_function;

    ಪ್ರತ್ಯಾಗಮ result;
}


// Searches every module in the runtime, most recently loaded first. That is a
// guess once more than one module is loaded and two of them export the same
// name - m3_FindFunctionIn says which module is meant.
M3Result m3_FindFunction (IM3Function* o_function, IM3Runtime i_runtime, ಧ್ರುವ ವರ್ಣ* ಧ್ರುವ i_functionName)
{
                                                                d_m3Assert (o_function and i_runtime and i_functionName);
    IM3Function function = ರಿಕ್ತ;

    ಯದಿ (not i_runtime->modules) {
        *o_function = ರಿಕ್ತ;
        ಪ್ರತ್ಯಾಗಮ "no modules loaded";
    }

    function = (IM3Function)ForEachModule(i_runtime, v_FindFunction, (ಶೂನ್ಯ*)i_functionName);

    ಯದಿ (not function) {
        *o_function = ರಿಕ್ತ;
        ಪ್ರತ್ಯಾಗಮ ErrorModule(m3Err_functionLookupFailed, i_runtime->modules, "'%s'", i_functionName);
    }

    ಪ್ರತ್ಯಾಗಮ PrepareFoundFunction(o_function, function);
}


IM3Module m3_FindModule (IM3Runtime i_runtime, ಧ್ರುವ ವರ್ಣ* ಧ್ರುವ i_moduleName)
{
    ಯದಿ (not i_runtime or not i_moduleName) {
        ಪ್ರತ್ಯಾಗಮ ರಿಕ್ತ;
    }

    // the list is newest-first, so a name registered twice names the newer one
    ಪ್ರತಿ (IM3Module m = i_runtime->modules; m; m = m->next) {
        ಯದಿ (m->name and strcmp(m->name, i_moduleName) == 0) {
            ಪ್ರತ್ಯಾಗಮ m;
        }
    }

    ಪ್ರತ್ಯಾಗಮ ರಿಕ್ತ;
}


// Searches one module's exports, which is what naming a module means.
M3Result m3_FindFunctionIn (IM3Function* o_function, IM3Module i_module, ಧ್ರುವ ವರ್ಣ* ಧ್ರುವ i_functionName)
{
                                                                d_m3Assert (o_function and i_functionName);
    ಯದಿ (not i_module) {
        *o_function = ರಿಕ್ತ;
        ಪ್ರತ್ಯಾಗಮ m3Err_functionLookupFailed;      // ErrorModule would deref it
    }

    IM3Function function = (IM3Function)v_FindFunction(i_module, (ಶೂನ್ಯ*)i_functionName);

    ಯದಿ (not function) {
        *o_function = ರಿಕ್ತ;
        ಪ್ರತ್ಯಾಗಮ ErrorModule(m3Err_functionLookupFailed, i_module, "'%s'", i_functionName);
    }

    ಪ್ರತ್ಯಾಗಮ PrepareFoundFunction(o_function, function);
}


M3Result m3_GetTableFunction (IM3Function* o_function, IM3Module i_module, uint32_t i_index)
{
_try {
    M3Table*    table;
    IM3Function function;

    _throwif("no table", i_module->ಅಂಕೆ_Tables == 0);

    table = i_module->tables[0];
    _throwif("function index out of range", i_index >= table->size);

    function = (IM3Function)table->elements[i_index];

    ಯದಿ (function) {
        ಯದಿ (not function->compiled) {
_           (CompileFunction(function))
        }
    }

    *o_function = function;
} _catch:
    ಪ್ರತ್ಯಾಗಮ result;
}


ಸ್ಥಿರ
M3Result checkStartFunction (IM3Module i_module)
{
    M3Result result = m3Err_none;                               d_m3Assert(i_module);

    // Check if start function needs to be called
    ಯದಿ (i_module->startFunction >= 0) {
        result = m3_RunStart(i_module);
    }

    ಪ್ರತ್ಯಾಗಮ result;
}

uint32_t m3_Get_Arg_ಎಣಿಕೆ (IM3Function i_function)
{
    ಯದಿ (i_function) {
        IM3FuncType ft = i_function->funcType;
        ಯದಿ (ft) {
            ಪ್ರತ್ಯಾಗಮ ft->ಅಂಕೆ_Args;
        }
    }
    ಪ್ರತ್ಯಾಗಮ 0;
}

uint32_t m3_Get_Ret_ಎಣಿಕೆ (IM3Function i_function)
{
    ಯದಿ (i_function) {
        IM3FuncType ft = i_function->funcType;
        ಯದಿ (ft) {
            ಪ್ರತ್ಯಾಗಮ ft->ಅಂಕೆ_Rets;
        }
    }
    ಪ್ರತ್ಯಾಗಮ 0;
}


M3_ಮೌಲ್ಯ_Type m3_GetArgType (IM3Function i_function, uint32_t index)
{
    ಯದಿ (i_function) {
        IM3FuncType ft = i_function->funcType;
        ಯದಿ (ft and index < ft->ಅಂಕೆ_Args) {
            ಪ್ರತ್ಯಾಗಮ (M3_ಮೌಲ್ಯ_Type)BaseTypeOf(d_FuncArgType(ft, index));
        }
    }
    ಪ್ರತ್ಯಾಗಮ c_m3Type_none;
}

M3_ಮೌಲ್ಯ_Type m3_GetRetType (IM3Function i_function, uint32_t index)
{
    ಯದಿ (i_function) {
        IM3FuncType ft = i_function->funcType;
        ಯದಿ (ft and index < ft->ಅಂಕೆ_Rets) {
            ಪ್ರತ್ಯಾಗಮ (M3_ಮೌಲ್ಯ_Type)BaseTypeOf(d_FuncRetType(ft, index));
        }
    }
    ಪ್ರತ್ಯಾಗಮ c_m3Type_none;
}


// The call APIs marshal their arguments into the runtime stack before any
// compiled code runs, so op_Entry's overflow check comes too late to cover those
// writes - and for a host function or an import linked to another module there
// is no op_Entry of our own to reach at all. Bound the area about to be written
// here instead: results and arguments each take a u64 at this boundary, however
// wide the slot is inside. Every caller goes through this, so none of them can
// be the one that forgets.
ಸ್ಥಿರ
M3Result Get_ರಾಶಿ_Pointer_For_Args (IM3Function i_function, u8** o_ರಾಶಿ)
{
    IM3Runtime  runtime = i_function->module->runtime;
    IM3FuncType ftype   = i_function->funcType;

    // the stack a paused invocation holds is not available to this one
    size_t used      = (size_t)((u8*)runtime->ರಾಶಿ - (u8*)runtime->origin_ರಾಶಿ);
    size_t needed    = ((size_t)ftype->ಅಂಕೆ_Rets + ftype->ಅಂಕೆ_Args) * ಪರಿಮಾಣ(u64);
    size_t available = (size_t)runtime->ಅಂಕೆ_ರಾಶಿ_Slots * ಪರಿಮಾಣ(m3slot_t) - used;

    ಯದಿ (needed > available) {
        ಪ್ರತ್ಯಾಗಮ m3_Err_trap_ರಾಶಿ_Overflow;
    }

    u64* ರಾಶಿ = (u64*)runtime->ರಾಶಿ;

    ರಾಶಿ += ftype->ಅಂಕೆ_Rets;

    *o_ರಾಶಿ = (u8*)ರಾಶಿ;

    ಪ್ರತ್ಯಾಗಮ m3Err_none;
}


M3Result m3_CallV (IM3Function i_function, ...)
{
    va_list ap;
    va_start(ap, i_function);
    M3Result r = m3_CallVL(i_function, ap);
    va_end(ap);
    ಪ್ರತ್ಯಾಗಮ r;
}

ಸ್ಥಿರ
ಶೂನ್ಯ Report_Native_ರಾಶಿ_Usage ()
{
#ಯದಿ d_m3_Log_Native_ರಾಶಿ
    ಪೂರ್ಣಾಙ್ಕ ರಾಶಿ_Used = m3_ರಾಶಿ_Get_Max();
    fprintf(stderr, "Native stack used: %d\n", ರಾಶಿ_Used);
#endif
}


M3Result m3_CallVL (IM3Function i_function, va_list i_args)
{
    IM3Runtime  runtime = i_function->module->runtime;
    IM3FuncType ftype   = i_function->funcType;
    M3Result    result  = m3Err_none;
    u8*         s       = ರಿಕ್ತ;

    ಯದಿ (!i_function->compiled) {
        ಪ್ರತ್ಯಾಗಮ m3Err_missingCompiledCode;
    }

#ಯದಿ d_m3RecordBacktraces
    ClearBacktrace(runtime);
#endif

    m3_ರಾಶಿ_Check_Init();

_   (checkStartFunction(i_function->module))

_   (Get_ರಾಶಿ_Pointer_For_Args(i_function, &s))

    ಪ್ರತಿ (u32 i = 0; i < ftype->ಅಂಕೆ_Args; ++i) {
        ವಿಕಲ್ಪ (d_FuncArgType(ftype, i)) {
        ಪಕ್ಷ c_m3Type_i32:
            *(i32*)(s) = va_arg(i_args, i32);
            s += 8;
            ಭಙ್ಗ;
        ಪಕ್ಷ c_m3Type_i64:
            *(i64*)(s) = va_arg(i_args, i64);
            s += 8;
            ಭಙ್ಗ;
        ಪಕ್ಷ c_m3Type_funcref:
        ಪಕ್ಷ c_m3Type_externref:
        ಪಕ್ಷ c_m3Type_exnref:
            *(uintptr_t*)(s) = va_arg(i_args, uintptr_t);
            s += 8;
            ಭಙ್ಗ;
#ಯದಿ d_m3HasFloat
        ಪಕ್ಷ c_m3Type_f32:
            *(f32*)(s) = (f32)va_arg(i_args, f64);
            s += 8;
            ಭಙ್ಗ; // f32 is passed as f64
        ಪಕ್ಷ c_m3Type_f64:
            *(f64*)(s) = va_arg(i_args, f64);
            s += 8;
            ಭಙ್ಗ;
#endif
        ಸ್ವಭಾವ: ಪ್ರತ್ಯಾಗಮ "unknown argument type";
        }
    }

    result = RunCodeChecked(runtime, i_function);
    Report_Native_ರಾಶಿ_Usage();

    runtime->lastCalled = result ? ರಿಕ್ತ : i_function;

    _catch: ಪ್ರತ್ಯಾಗಮ result;
}

M3Result m3_Call (IM3Function i_function, uint32_t i_argc, ಧ್ರುವ ಶೂನ್ಯ* i_argptrs[])
{
    IM3Runtime  runtime = i_function->module->runtime;
    IM3FuncType ftype   = i_function->funcType;
    M3Result    result  = m3Err_none;
    u8*         s       = ರಿಕ್ತ;

    ಯದಿ (i_argc != ftype->ಅಂಕೆ_Args) {
        ಪ್ರತ್ಯಾಗಮ m3_Err_argument_ಎಣಿಕೆ_Mismatch;
    }
    ಯದಿ (!i_function->compiled) {
        ಪ್ರತ್ಯಾಗಮ m3Err_missingCompiledCode;
    }

#ಯದಿ d_m3RecordBacktraces
    ClearBacktrace(runtime);
#endif

    m3_ರಾಶಿ_Check_Init();

_   (checkStartFunction(i_function->module))

_   (Get_ರಾಶಿ_Pointer_For_Args(i_function, &s))

    ಪ್ರತಿ (u32 i = 0; i < ftype->ಅಂಕೆ_Args; ++i) {
        ವಿಕಲ್ಪ (d_FuncArgType(ftype, i)) {
        ಪಕ್ಷ c_m3Type_i32:
            *(i32*)(s) = *(i32*)i_argptrs[i];
            s += 8;
            ಭಙ್ಗ;
        ಪಕ್ಷ c_m3Type_i64:
            *(i64*)(s) = *(i64*)i_argptrs[i];
            s += 8;
            ಭಙ್ಗ;
        ಪಕ್ಷ c_m3Type_funcref:
        ಪಕ್ಷ c_m3Type_externref:
        ಪಕ್ಷ c_m3Type_exnref:
            *(uintptr_t*)(s) = *(uintptr_t*)i_argptrs[i];
            s += 8;
            ಭಙ್ಗ;
#ಯದಿ d_m3HasFloat
        ಪಕ್ಷ c_m3Type_f32:
            *(f32*)(s) = *(f32*)i_argptrs[i];
            s += 8;
            ಭಙ್ಗ;
        ಪಕ್ಷ c_m3Type_f64:
            *(f64*)(s) = *(f64*)i_argptrs[i];
            s += 8;
            ಭಙ್ಗ;
#endif
        ಸ್ವಭಾವ: ಪ್ರತ್ಯಾಗಮ "unknown argument type";
        }
    }

    result = RunCodeChecked(runtime, i_function);

    Report_Native_ರಾಶಿ_Usage();

    runtime->lastCalled = result ? ರಿಕ್ತ : i_function;

    _catch: ಪ್ರತ್ಯಾಗಮ result;
}

// Argument parsing for m3_CallArgv. Strict on purpose: the whole string has to
// be consumed, so "12abc" is rejected rather than read as 12, and an empty or
// unparsable argument is an error rather than the zero that strtoul with a
// NULL end pointer used to hand back. See wasm3/wasm3#367.
ಸ್ಥಿರ
M3Result ParseArgInteger (ccstr_t i_arg, u32 i_ಅಂಕೆ_Bits, u64* o_ಮೌಲ್ಯ)
{
    ಯದಿ (not i_arg or not *i_arg) {
        ಪ್ರತ್ಯಾಗಮ "empty argument";
    }

    // strtoull would skip leading space, but trailing space is rejected below;
    // accepting one and not the other would just be confusing
    ಯದಿ (isspace((unsigned ವರ್ಣ)*i_arg)) {
        ಪ್ರತ್ಯಾಗಮ "argument is not a number";
    }

    ವರ್ಣ* end = ರಿಕ್ತ;
    u64   ಮೌಲ್ಯ;

    errno = 0;

    // an argument may be spelled signed or unsigned: -1 and 4294967295 name the
    // same i32
    ಯದಿ (*i_arg == '-') {
        i64 signed_ಮೌಲ್ಯ = strtoll(i_arg, &end, 10);

        ಯದಿ (i_ಅಂಕೆ_Bits == 32 and (signed_ಮೌಲ್ಯ < INT32_MIN or signed_ಮೌಲ್ಯ > INT32_MAX)) {
            ಪ್ರತ್ಯಾಗಮ "argument out of range";
        }

        ಮೌಲ್ಯ = (u64)signed_ಮೌಲ್ಯ;
    } ಅನ್ಯಥಾ {
        ಮೌಲ್ಯ = strtoull(i_arg, &end, 10);

        ಯದಿ (i_ಅಂಕೆ_Bits == 32 and ಮೌಲ್ಯ > UINT32_MAX) {
            ಪ್ರತ್ಯಾಗಮ "argument out of range";
        }
    }

    ಯದಿ (errno == ERANGE) {
        ಪ್ರತ್ಯಾಗಮ "argument out of range";
    }

    ಯದಿ (end == i_arg or *end) {
        ಪ್ರತ್ಯಾಗಮ "argument is not a number";
    }

    *o_ಮೌಲ್ಯ = ಮೌಲ್ಯ;

    ಪ್ರತ್ಯಾಗಮ m3Err_none;
}


#ಯದಿ d_m3HasFloat
ಸ್ಥಿರ
M3Result ParseArgFloat (ccstr_t i_arg, f64* o_ಮೌಲ್ಯ)
{
    ಯದಿ (not i_arg or not *i_arg) {
        ಪ್ರತ್ಯಾಗಮ "empty argument";
    }

    ಯದಿ (isspace((unsigned ವರ್ಣ)*i_arg)) {
        ಪ್ರತ್ಯಾಗಮ "argument is not a number";
    }

    ವರ್ಣ* end = ರಿಕ್ತ;

    errno = 0;

    f64 ಮೌಲ್ಯ = strtod(i_arg, &end);

    ಯದಿ (end == i_arg or *end) {
        ಪ್ರತ್ಯಾಗಮ "argument is not a number";
    }

    // strtod reports underflow through ERANGE as well, and a denormal result is
    // perfectly usable, so only an overflow to infinity is out of range
    ಯದಿ (errno == ERANGE and (ಮೌಲ್ಯ > DBL_MAX or ಮೌಲ್ಯ < -DBL_MAX)) {
        ಪ್ರತ್ಯಾಗಮ "argument out of range";
    }

    *o_ಮೌಲ್ಯ = ಮೌಲ್ಯ;

    ಪ್ರತ್ಯಾಗಮ m3Err_none;
}
#endif


// A reference argument is either the null reference or a host handle written as
// an integer.
ಸ್ಥಿರ
M3Result ParseArgReference (ccstr_t i_arg, u64* o_ಮೌಲ್ಯ)
{
    ಯದಿ (i_arg and strcmp(i_arg, "null") == 0) {
        *o_ಮೌಲ್ಯ = 0;
        ಪ್ರತ್ಯಾಗಮ m3Err_none;
    }

    ಪ್ರತ್ಯಾಗಮ ParseArgInteger(i_arg, 64, o_ಮೌಲ್ಯ);
}


M3Result m3_CallArgv (IM3Function i_function, uint32_t i_argc, ಧ್ರುವ ವರ್ಣ* i_argv[])
{
    IM3FuncType ftype   = i_function->funcType;
    IM3Runtime  runtime = i_function->module->runtime;
    M3Result    result  = m3Err_none;
    u8*         s       = ರಿಕ್ತ;

    ಯದಿ (i_argc != ftype->ಅಂಕೆ_Args) {
        ಪ್ರತ್ಯಾಗಮ m3_Err_argument_ಎಣಿಕೆ_Mismatch;
    }
    ಯದಿ (!i_function->compiled) {
        ಪ್ರತ್ಯಾಗಮ m3Err_missingCompiledCode;
    }

#ಯದಿ d_m3RecordBacktraces
    ClearBacktrace(runtime);
#endif

    m3_ರಾಶಿ_Check_Init();

_   (checkStartFunction(i_function->module))

_   (Get_ರಾಶಿ_Pointer_For_Args(i_function, &s))

    ಪ್ರತಿ (u32 i = 0; i < ftype->ಅಂಕೆ_Args; ++i) {
        u64 ಮೌಲ್ಯ = 0;
#ಯದಿ d_m3HasFloat
        f64 fvalue = 0;
#endif
        ವಿಕಲ್ಪ (d_FuncArgType(ftype, i)) {
        ಪಕ್ಷ c_m3Type_i32:
_           (ParseArgInteger(i_argv[i], 32, &ಮೌಲ್ಯ)) * (i32*)(s) = (i32)ಮೌಲ್ಯ;
            s += 8;
            ಭಙ್ಗ;
        ಪಕ್ಷ c_m3Type_i64:
_           (ParseArgInteger(i_argv[i], 64, &ಮೌಲ್ಯ)) * (i64*)(s) = (i64)ಮೌಲ್ಯ;
            s += 8;
            ಭಙ್ಗ;
        ಪಕ್ಷ c_m3Type_funcref:
        ಪಕ್ಷ c_m3Type_externref:
        ಪಕ್ಷ c_m3Type_exnref:
_           (ParseArgReference(i_argv[i], &ಮೌಲ್ಯ)) * (uintptr_t*)(s) = (uintptr_t)ಮೌಲ್ಯ;
            s += 8;
            ಭಙ್ಗ;
#ಯದಿ d_m3HasFloat
            // strtof would be less portable
        ಪಕ್ಷ c_m3Type_f32:
_           (ParseArgFloat(i_argv[i], &fvalue)) * (f32*)(s) = (f32)fvalue;
            s += 8;
            ಭಙ್ಗ;
        ಪಕ್ಷ c_m3Type_f64:
_           (ParseArgFloat(i_argv[i], &fvalue)) * (f64*)(s) = fvalue;
            s += 8;
            ಭಙ್ಗ;
#endif
        ಸ್ವಭಾವ: _throw("unknown argument type");
        }
    }

    result = RunCodeChecked(runtime, i_function);

    Report_Native_ರಾಶಿ_Usage();

    runtime->lastCalled = result ? ರಿಕ್ತ : i_function;

    _catch: ಪ್ರತ್ಯಾಗಮ result;
}


//u8 * AlignStackPointerTo64Bits (const u8 * i_stack)
//{
//    uintptr_t ptr = (uintptr_t) i_stack;
//    return (u8 *) ((ptr + 7) & ~7);
//}


M3Result m3_GetResults (IM3Function i_function, uint32_t i_retc, ಧ್ರುವ ಶೂನ್ಯ* o_retptrs[])
{
    IM3FuncType ftype   = i_function->funcType;
    IM3Runtime  runtime = i_function->module->runtime;

    ಯದಿ (i_retc != ftype->ಅಂಕೆ_Rets) {
        ಪ್ರತ್ಯಾಗಮ m3_Err_argument_ಎಣಿಕೆ_Mismatch;
    }
    ಯದಿ (i_function != runtime->lastCalled) {
        ಪ್ರತ್ಯಾಗಮ "function not called";
    }

    u8* s = (u8*)runtime->ರಾಶಿ;

    ಪ್ರತಿ (u32 i = 0; i < ftype->ಅಂಕೆ_Rets; ++i) {
        ವಿಕಲ್ಪ (d_FuncRetType(ftype, i)) {
        ಪಕ್ಷ c_m3Type_i32:
            *(i32*)o_retptrs[i] = *(i32*)(s);
            s += 8;
            ಭಙ್ಗ;
        ಪಕ್ಷ c_m3Type_i64:
            *(i64*)o_retptrs[i] = *(i64*)(s);
            s += 8;
            ಭಙ್ಗ;
        ಪಕ್ಷ c_m3Type_funcref:
        ಪಕ್ಷ c_m3Type_externref:
        ಪಕ್ಷ c_m3Type_exnref:
            *(uintptr_t*)o_retptrs[i] = *(uintptr_t*)(s);
            s += 8;
            ಭಙ್ಗ;
#ಯದಿ d_m3HasFloat
        ಪಕ್ಷ c_m3Type_f32:
            *(f32*)o_retptrs[i] = *(f32*)(s);
            s += 8;
            ಭಙ್ಗ;
        ಪಕ್ಷ c_m3Type_f64:
            *(f64*)o_retptrs[i] = *(f64*)(s);
            s += 8;
            ಭಙ್ಗ;
#endif
        ಸ್ವಭಾವ: ಪ್ರತ್ಯಾಗಮ "unknown return type";
        }
    }
    ಪ್ರತ್ಯಾಗಮ m3Err_none;
}

M3Result m3_GetResultsV (IM3Function i_function, ...)
{
    va_list ap;
    va_start(ap, i_function);
    M3Result r = m3_GetResultsVL(i_function, ap);
    va_end(ap);
    ಪ್ರತ್ಯಾಗಮ r;
}

M3Result m3_GetResultsVL (IM3Function i_function, va_list o_rets)
{
    IM3Runtime  runtime = i_function->module->runtime;
    IM3FuncType ftype   = i_function->funcType;

    ಯದಿ (i_function != runtime->lastCalled) {
        ಪ್ರತ್ಯಾಗಮ "function not called";
    }

    u8* s = (u8*)runtime->ರಾಶಿ;
    ಪ್ರತಿ (u32 i = 0; i < ftype->ಅಂಕೆ_Rets; ++i) {
        ವಿಕಲ್ಪ (d_FuncRetType(ftype, i)) {
        ಪಕ್ಷ c_m3Type_i32:
            *va_arg(o_rets, i32*) = *(i32*)(s);
            s += 8;
            ಭಙ್ಗ;
        ಪಕ್ಷ c_m3Type_i64:
            *va_arg(o_rets, i64*) = *(i64*)(s);
            s += 8;
            ಭಙ್ಗ;
        ಪಕ್ಷ c_m3Type_funcref:
        ಪಕ್ಷ c_m3Type_externref:
        ಪಕ್ಷ c_m3Type_exnref:
            *va_arg(o_rets, uintptr_t*) = *(uintptr_t*)(s);
            s += 8;
            ಭಙ್ಗ;
#ಯದಿ d_m3HasFloat
        ಪಕ್ಷ c_m3Type_f32:
            *va_arg(o_rets, f32*) = *(f32*)(s);
            s += 8;
            ಭಙ್ಗ;
        ಪಕ್ಷ c_m3Type_f64:
            *va_arg(o_rets, f64*) = *(f64*)(s);
            s += 8;
            ಭಙ್ಗ;
#endif
        ಸ್ವಭಾವ: ಪ್ರತ್ಯಾಗಮ "unknown argument type";
        }
    }
    ಪ್ರತ್ಯಾಗಮ m3Err_none;
}

ಶೂನ್ಯ ReleaseCodePageNoTrack (IM3Runtime i_runtime, IM3CodePage i_codePage)
{
    ಯದಿ (i_codePage) {
        IM3CodePage* list;

        bool pageFull = (ಅಂಕೆ_Free_ಸಾಲುಗಳು(i_codePage) < d_m3_Code_Page_Free_ಸಾಲುಗಳು_Threshold);
        ಯದಿ (pageFull) {
            list = &i_runtime->pagesFull;
        } ಅನ್ಯಥಾ {
            list = &i_runtime->pagesOpen;
        }

        ತಳ್ಳು_Code_Page(list, i_codePage);                         m3log (emit, "release page: %d to queue: '%s'", i_codePage->info.sequence, pageFull ? "full" : "open")
    }
}


IM3CodePage AcquireCodePageWithCapacity (IM3Runtime i_runtime, u32 i_min_ಸಾಲು_ಎಣಿಕೆ)
{
    IM3CodePage page = RemoveCodePageOfCapacity(&i_runtime->pagesOpen, i_min_ಸಾಲು_ಎಣಿಕೆ);

    ಯದಿ (not page) {
        page = Environment_AcquireCodePage(i_runtime->environment, i_min_ಸಾಲು_ಎಣಿಕೆ);

        ಯದಿ (not page) {
            page = NewCodePage(i_runtime, i_min_ಸಾಲು_ಎಣಿಕೆ);
        }

        ಯದಿ (page) {
            i_runtime->ಅಂಕೆ_Code_Pages++;
        }
    }

    ಯದಿ (page) {                                                  m3log (emit, "acquire page: %d", page->info.sequence);
        i_runtime->ಅಂಕೆ_Active_Code_Pages++;
    }

    ಪ್ರತ್ಯಾಗಮ page;
}


IM3CodePage AcquireCodePage (IM3Runtime i_runtime)
{
    ಪ್ರತ್ಯಾಗಮ AcquireCodePageWithCapacity(i_runtime, d_m3_Code_Page_Free_ಸಾಲುಗಳು_Threshold);
}


ಶೂನ್ಯ ReleaseCodePage (IM3Runtime i_runtime, IM3CodePage i_codePage)
{
    ಯದಿ (i_codePage) {
        ReleaseCodePageNoTrack(i_runtime, i_codePage);
        i_runtime->ಅಂಕೆ_Active_Code_Pages--;

#ಯದಿ defined(DEBUG)
        u32 ಅಂಕೆ_Open = ಎಣಿಕೆ_Code_Pages(i_runtime->pagesOpen);
        u32 ಅಂಕೆ_Full = ಎಣಿಕೆ_Code_Pages(i_runtime->pagesFull);

        m3log(runtime, "runtime: %p; open-pages: %d; full-pages: %d; active: %d; total: %d", i_runtime, ಅಂಕೆ_Open, ಅಂಕೆ_Full, i_runtime->ಅಂಕೆ_Active_Code_Pages, i_runtime->ಅಂಕೆ_Code_Pages);

        d_m3Assert(ಅಂಕೆ_Open + ಅಂಕೆ_Full + i_runtime->ಅಂಕೆ_Active_Code_Pages == i_runtime->ಅಂಕೆ_Code_Pages);

#  ಯದಿ d_m3LogCodePages
        dump_code_page(i_codePage, /* startPC: */ ರಿಕ್ತ);
#  endif
#endif
    }
}


#ಯದಿ d_m3VerboseErrorMessages
M3Result m3Error (M3Result i_result, IM3Runtime i_runtime, IM3Module i_module, IM3Function i_function,
                  ಧ್ರುವ ವರ್ಣ* ಧ್ರುವ i_file, u32 i_ಸಾಲು_ಅಂಕೆ, ಧ್ರುವ ವರ್ಣ* ಧ್ರುವ i_errorMessage, ...)
{
    ಯದಿ (i_runtime) {
        i_runtime->error         = (M3ErrorInfo){ .result = i_result, .runtime = i_runtime, .module = i_module, .function = i_function, .file = i_file, .ಸಾಲು_2 = i_ಸಾಲು_ಅಂಕೆ };
        i_runtime->error.message = i_runtime->error_message;

        va_list args;
        va_start(args, i_errorMessage);
        vsnprintf(i_runtime->error_message, ಪರಿಮಾಣ(i_runtime->error_message), i_errorMessage, args);
        va_end(args);
    }

    ಪ್ರತ್ಯಾಗಮ i_result;
}
#endif


ಶೂನ್ಯ m3_GetErrorInfo (IM3Runtime i_runtime, M3ErrorInfo* o_info)
{
    ಯದಿ (i_runtime) {
        *o_info = i_runtime->error;
        m3_ResetErrorInfo(i_runtime);
    }
}


ಶೂನ್ಯ m3_ResetErrorInfo (IM3Runtime i_runtime)
{
    ಯದಿ (i_runtime) {
        M3_INIT(i_runtime->error);
        i_runtime->error.message = "";
    }
}

uint8_t* m3_GetMemory (IM3Module i_module, size_t* o_memorySizeInBytes, uint32_t i_memoryIndex)
{
    uint8_t* memory = ರಿಕ್ತ;
    size_t   size   = 0;

    ಯದಿ (i_module and i_memoryIndex < i_module->ಅಂಕೆ_Memories) {
        IM3Memory mem = i_module->memories[i_memoryIndex];

        ಯದಿ (mem->mallocated) {
            size = mem->mallocated->length;

            ಯದಿ (size) {
                memory = m3MemData(mem->mallocated);
            }
        }
    }

    ಯದಿ (o_memorySizeInBytes) {
        *o_memorySizeInBytes = size;
    }

    ಪ್ರತ್ಯಾಗಮ memory;
}


size_t m3_GetMemorySize (IM3Module i_module, uint32_t i_memoryIndex)
{
    ಯದಿ (not i_module or i_memoryIndex >= i_module->ಅಂಕೆ_Memories) {
        ಪ್ರತ್ಯಾಗಮ 0;
    }

    IM3Memory mem = i_module->memories[i_memoryIndex];

    ಪ್ರತ್ಯಾಗಮ mem->mallocated ? mem->mallocated->length : 0;
}


size_t m3_GetMemorySizeAt (ಧ್ರುವ ಶೂನ್ಯ* i_memory)
{
    ಯದಿ (not i_memory) {
        ಪ್ರತ್ಯಾಗಮ 0;
    }

    // the header sits immediately before the data it describes
    ಧ್ರುವ M3MemoryHeader* header = ((ಧ್ರುವ M3MemoryHeader*)i_memory) - 1;

    ಪ್ರತ್ಯಾಗಮ header->length;
}


M3Result m3_FindExportedMemory (IM3Module i_module, cstr_t i_name, u32* o_memoryIndex)
{
    ಯದಿ (not i_module or not i_name or not o_memoryIndex) {
        ಪ್ರತ್ಯಾಗಮ m3Err_unknownMemory;
    }

    ಧ್ರುವ M3Export* e = Module_FindExport(i_module, d_externalKind_memory, i_name);

    ಯದಿ (not e) {
        ಪ್ರತ್ಯಾಗಮ m3Err_unknownMemory;
    }

    *o_memoryIndex = e->index;
    ಪ್ರತ್ಯಾಗಮ m3Err_none;
}


bool Module_HasLinkedHostImport (IM3Module i_module, cstr_t i_importModule)
{
    ಪ್ರತಿ (u32 i = 0; i < i_module->ಅಂಕೆ_Functions; ++i) {
        IM3Function f = &i_module->functions[i];

        // hostMemory is set when a host function is bound to the import, so it
        // is also the mark of one - see CompileRawFunction
        ಯದಿ (not f->hostMemory or f->wasm or f->resolved) {
            ಅನುವರ್ತ;
        }

        ಯದಿ (f->import.moduleUtf8 and strcmp(f->import.moduleUtf8, i_importModule) == 0) {
            ಪ್ರತ್ಯಾಗಮ true;
        }
    }

    ಪ್ರತ್ಯಾಗಮ false;
}


M3Result m3_BindImportMemory (IM3Module io_module, cstr_t i_importModule, u32 i_memoryIndex)
{
    ಯದಿ (not io_module or not i_importModule) {
        ಪ್ರತ್ಯಾಗಮ m3Err_moduleNotLinked;
    }

    ಯದಿ (i_memoryIndex >= io_module->ಅಂಕೆ_Memories) {
        ಪ್ರತ್ಯಾಗಮ m3Err_unknownMemory;
    }

    // the memory itself, not the index: linking may have pointed this slot at
    // another module's memory, and growing one reallocates behind it
    IM3Memory memory = io_module->memories[i_memoryIndex];

    ಧ್ರುವ bool wildcardModule = (strcmp(i_importModule, "*") == 0);

    ಪ್ರತಿ (u32 i = 0; i < io_module->ಅಂಕೆ_Functions; ++i) {
        IM3Function f = &io_module->functions[i];

        // only an import that a host function was bound to has a memory to
        // pin: one with a body of its own reads memory through the interpreter,
        // and one resolved to another module's export runs that module's code
        ಯದಿ (not f->hostMemory or f->wasm or f->resolved) {
            ಅನುವರ್ತ;
        }

        ಯದಿ (wildcardModule or (f->import.moduleUtf8 and strcmp(f->import.moduleUtf8, i_importModule) == 0)) {
            f->hostMemory = memory;
        }
    }

    ಪ್ರತ್ಯಾಗಮ m3Err_none;
}


M3BacktraceInfo* m3_GetBacktrace (IM3Runtime i_runtime)
{
#ಯದಿ d_m3RecordBacktraces
    ಪ್ರತ್ಯಾಗಮ &i_runtime->backtrace;
#ಅನ್ಯಥಾ
    ಪ್ರತ್ಯಾಗಮ ರಿಕ್ತ;
#endif
}


//-------------------------------------------------------------------------------------------------------------------------------
//  snapshots & suspendable execution
//-------------------------------------------------------------------------------------------------------------------------------

ಶೂನ್ಯ m3_SetSuspendable (IM3Runtime io_runtime, bool i_suspendable)
{
    ಯದಿ (io_runtime) {
        io_runtime->isSuspendable = i_suspendable;
    }
}

ಶೂನ್ಯ m3_RequestSuspend (IM3Runtime io_runtime)
{
    ಯದಿ (io_runtime) {
        io_runtime->suspendRequested = true;
    }
}

bool m3_IsSuspended (IM3Runtime i_runtime)
{
    ಯದಿ (!i_runtime) {
        ಪ್ರತ್ಯಾಗಮ false;
    }
#ಯದಿ d_m3_Has_ರಾಶಿ_Switching
    ಯದಿ (i_runtime->ಬೇರು_Continuation && i_runtime->ಬೇರು_Continuation->state == cont_suspended) {
        ಪ್ರತ್ಯಾಗಮ true;
    }
#endif
    ಪ್ರತ್ಯಾಗಮ false;
}

M3Result m3_ResumeRuntime (IM3Runtime io_runtime)
{
    ಯದಿ (!io_runtime) {
        ಪ್ರತ್ಯಾಗಮ m3Err_mallocFailed;
    }
    ಯದಿ (!io_runtime->isSuspendable) {
        ಪ್ರತ್ಯಾಗಮ m3Err_none;
    }
#ಯದಿ d_m3HasSnapshots
    // a restore that failed leaves nothing to resume, and says so every time
    // rather than resuming nothing successfully
    ಯದಿ (io_runtime->ಬೇರು_Continuation and io_runtime->ಬೇರು_Continuation->entryFunction and
        io_runtime->ಬೇರು_Continuation->entryFunction->module->isUnusable) {
        ಪ್ರತ್ಯಾಗಮ "a snapshot failed to restore into the module, so it cannot run";
    }
#endif
#ಯದಿ d_m3_Has_ರಾಶಿ_Switching
    ಯದಿ (!io_runtime->ಬೇರು_Continuation || io_runtime->ಬೇರು_Continuation->state != cont_suspended) {
        ಪ್ರತ್ಯಾಗಮ m3Err_none;
    }
    // what finishes returns into the bottom of the stack, where it was entered
    io_runtime->ರಾಶಿ = io_runtime->origin_ರಾಶಿ;

    // this is the outermost call again, as far as a host function calling back
    // into Wasm from inside it can tell
    io_runtime->entered = io_runtime->ಬೇರು_Continuation->entryFunction;
    io_runtime->callNesting++;

    d_m3_ರಾಶಿ_Limit_Enter(io_runtime);
    io_runtime->resumePastCheck = true;
    M3Result result             = (M3Result)ResumeContinuation(io_runtime, io_runtime->ಬೇರು_Continuation);
    io_runtime->resumePastCheck = false;
    d_m3_ರಾಶಿ_Limit_Leave(io_runtime);

#  ಯದಿ d_m3HasExceptionHandling
    ಯದಿ (M3_UNLIKELY(result == m3Err_pendingException)) {
        result = m3Err_trapUncaughtException;
    }
    RecordUncaughtException(io_runtime, result);
#  endif

    io_runtime->callNesting--;

#  ಯದಿ d_m3HasExceptionHandling
    ಯದಿ (result != m3Err_continuationSuspended and io_runtime->callNesting == 0) {
        FreeExceptions(io_runtime);
    }
#  endif

    // the results are where the paused call left room for them, so a finished
    // resume can be asked for them the way the call itself could have been
    io_runtime->lastCalled = (result == m3Err_none) ? io_runtime->ಬೇರು_Continuation->entryFunction : ರಿಕ್ತ;

    Runtime_Place_Call_ರಾಶಿ(io_runtime);
    ಪ್ರತ್ಯಾಗಮ result;
#ಅನ್ಯಥಾ
    ಪ್ರತ್ಯಾಗಮ m3Err_none;
#endif
}
