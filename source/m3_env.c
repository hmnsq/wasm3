//
//  m3_env.c
//
//  Created by Steven Massey on 4/19/19.
//  Copyright © 2019 Steven Massey. All rights reserved.
//

#ଅନ୍ତର୍ଭାଵ <stdarg.h>
#ଅନ୍ତର୍ଭାଵ <limits.h>
#ଅନ୍ତର୍ଭାଵ <errno.h>
#ଅନ୍ତର୍ଭାଵ <float.h>
#ଅନ୍ତର୍ଭାଵ <ctype.h>

#ଅନ୍ତର୍ଭାଵ "m3_env.h"
#ଅନ୍ତର୍ଭାଵ "m3_bind.h"
#ଅନ୍ତର୍ଭାଵ "m3_deterministic.h"
#ଅନ୍ତର୍ଭାଵ "m3_compile.h"
#ଅନ୍ତର୍ଭାଵ "m3_host.h"
#ଅନ୍ତର୍ଭାଵ "m3_exception.h"
#ଅନ୍ତର୍ଭାଵ "m3_info.h"


IM3Environment m3_NewEnvironment ()
{
    IM3Environment env = m3_AllocStruct(M3Environment);

    ଯଦି (env) {
        _try
        {
            // create FuncTypes for all simple block return ValueTypes.
            // v128 is skipped: it parses as a slot but has no operations.
            ପାଇଁ (u8 t = c_m3Type_none; t < c_m3_Type_ଗଣନା; t++) {
                ଯଦି (t == c_m3Type_v128) {
                    ଜାରି;
                }

                IM3FuncType ftype;
_               (AllocFuncType(&ftype, 1));

                ftype->ଅଙ୍କ_Args  = 0;
                ftype->ଅଙ୍କ_Rets  = (t == c_m3Type_none) ? 0 : 1;
                ftype->types[0] = t;

                Environment_AddFuncType(env, &ftype);

                env->retFuncTypes[t] = ftype;
            }
        }

        _catch:
        ଯଦି (result) {
            m3_FreeEnvironment(env);
            env = ଖାଲି;
        }
    }

    ଫେରନ୍ତୁ env;
}


ଶୂନ୍ୟ Environment_Release (IM3Environment i_environment)
{
    IM3FuncType ftype = i_environment->funcTypes;

    ଯେତେବେଳେ (ftype) {
        IM3FuncType next = ftype->next;
        m3_Free(ftype);
        ftype = next;
    }

    m3log(runtime, "freeing %d pages from environment", ଗଣନା_Code_Pages(i_environment->pagesReleased));
    FreeCodePages(&i_environment->pagesReleased);
}


ଶୂନ୍ୟ m3_FreeEnvironment (IM3Environment i_environment)
{
    ଯଦି (i_environment) {
        Environment_Release(i_environment);
        m3_Free(i_environment);
    }
}


ଶୂନ୍ୟ m3_SetCustomSectionHandler (IM3Environment i_environment, M3SectionHandler i_handler)
{
    ଯଦି (i_environment) {
        i_environment->customSectionHandler = i_handler;
    }
}


// returns the same io_funcType or replaces it with an equivalent that's already in the type linked list
M3Result Environment_AddFuncType (IM3Environment i_environment, IM3FuncType* io_funcType)
{
    IM3FuncType addType = *io_funcType;
    IM3FuncType newType = i_environment->funcTypes;

    ଯେତେବେଳେ (newType) {
        ଯଦି (not newType->inRecGroup and AreFuncTypesEqual(newType, addType)) {
            m3_Free(addType);
            ଭାଙ୍ଗ;
        }

        newType = newType->next;
    }

    ଯଦି (newType == ଖାଲି) {
        // a type index has to fit in the heap type field of an m3type_t
        ଯଦି (i_environment->ଅଙ୍କ_Func_Types >= d_m3_Max_Sane_Types_ଗଣନା) {
            m3_Free(addType);
            *io_funcType = ଖାଲି;
            ଫେରନ୍ତୁ "too many distinct function types";
        }

        newType                  = addType;
        newType->canonicalIndex  = i_environment->ଅଙ୍କ_Func_Types++;
        newType->next            = i_environment->funcTypes;
        i_environment->funcTypes = newType;
    }

    *io_funcType = newType;

    ଫେରନ୍ତୁ m3Err_none;
}


// Adds a type without looking for an equal one first, and hands back the index
// it was given.
//
// Members of a recursive group do not get to share an entry with a type that
// merely looks like them: the group is their identity, so two groups spelled
// the same way are still two groups, and a type inside one is never the type
// outside it that matches field for field. Reserving the indices up front is
// also what lets the members refer to each other while they are being read.
M3Result Environment_ReserveFuncTypes (IM3Environment i_environment, u32 i_ଗଣନା, u16* o_firstIndex)
{
    ଯଦି ((u64)i_environment->ଅଙ୍କ_Func_Types + i_ଗଣନା > d_m3_Max_Sane_Types_ଗଣନା) {
        ଫେରନ୍ତୁ "too many distinct function types";
    }

    *o_firstIndex = i_environment->ଅଙ୍କ_Func_Types;
    i_environment->ଅଙ୍କ_Func_Types += (u16)i_ଗଣନା;

    ଫେରନ୍ତୁ m3Err_none;
}


ଶୂନ୍ୟ Environment_AdoptFuncType (IM3Environment i_environment, IM3FuncType i_funcType, u16 i_canonicalIndex)
{
    i_funcType->inRecGroup     = true;
    i_funcType->canonicalIndex = i_canonicalIndex;
    i_funcType->next           = i_environment->funcTypes;
    i_environment->funcTypes   = i_funcType;
}


IM3CodePage RemoveCodePageOfCapacity (M3CodePage** io_list, u32 i_minimum_ଧାଡ଼ି_ଗଣନା)
{
    IM3CodePage prev = ଖାଲି;
    IM3CodePage page = *io_list;

    ଯେତେବେଳେ (page) {
        ଯଦି (ଅଙ୍କ_Free_ଧାଡ଼ିଗୁଡ଼ିକ(page) >= i_minimum_ଧାଡ଼ି_ଗଣନା) {             d_m3Assert (page->info.usage_ଗଣନା == 0);
            IM3CodePage next = page->info.next;
            ଯଦି (prev) {
                prev->info.next = next; // mid-list
            } ଅନ୍ୟଥା {
                *io_list = next;       // front of list
            }

            ଭାଙ୍ଗ;
        }

        prev = page;
        page = page->info.next;
    }

    ଫେରନ୍ତୁ page;
}


IM3CodePage Environment_AcquireCodePage (IM3Environment i_environment, u32 i_minimum_ଧାଡ଼ି_ଗଣନା)
{
    ଫେରନ୍ତୁ RemoveCodePageOfCapacity(&i_environment->pagesReleased, i_minimum_ଧାଡ଼ି_ଗଣନା);
}


ଶୂନ୍ୟ Environment_ReleaseCodePages (IM3Environment i_environment, IM3CodePage i_codePageList)
{
    IM3CodePage end = i_codePageList;

    ଯେତେବେଳେ (end) {
        end->info.ଧାଡ଼ି_Index = 0; // reset page
#ଯଦି d_m3RecordBacktraces
        end->info.mapping->size = 0;
#endif // d_m3RecordBacktraces

        IM3CodePage next = end->info.next;
        ଯଦି (not next) {
            ଭାଙ୍ଗ;
        }

        end = next;
    }

    ଯଦି (end) {
        // push list to front
        end->info.next               = i_environment->pagesReleased;
        i_environment->pagesReleased = i_codePageList;
    }
}


IM3Runtime m3_NewRuntime (IM3Environment i_environment, u32 i_ଗଦା_Size_In_Bytes, ଶୂନ୍ୟ* i_userdata)
{
    IM3Runtime runtime = m3_AllocStruct(M3Runtime);

    ଯଦି (runtime) {
        m3_ResetErrorInfo(runtime);

        runtime->environment = i_environment;
        runtime->userdata    = i_userdata;

        Deterministic_InitRuntime(runtime);

        runtime->origin_ଗଦା = m3_Malloc("Wasm Stack", i_ଗଦା_Size_In_Bytes + 4 * ଆକାର(m3slot_t)); // TODO: more precise stack checks

        ଯଦି (runtime->origin_ଗଦା) {
            runtime->ଗଦା         = runtime->origin_ଗଦା;
            runtime->ଅଙ୍କ_ଗଦା_Slots = i_ଗଦା_Size_In_Bytes / ଆକାର(m3slot_t);          m3log (runtime, "new stack: %p, slots: %u", runtime->origin_ଗଦା, runtime->ଅଙ୍କ_ଗଦା_Slots);
        } ଅନ୍ୟଥା {
            m3_Free(runtime);
        }
    }

    ଫେରନ୍ତୁ runtime;
}

ଶୂନ୍ୟ m3_SetValidation (IM3Runtime i_runtime, bool i_enable)
{
#ଯଦି d_m3EnableValidation
    ଯଦି (i_runtime) {
        i_runtime->skipValidation = not i_enable;
    }
#ଅନ୍ୟଥା
    (ଶୂନ୍ୟ)i_runtime;
    (ଶୂନ୍ୟ)i_enable;              // nothing to skip: the validator was compiled out
#endif
}

M3Result m3_SetResourceLimit (IM3Runtime i_runtime, M3ResourceLimit i_limit, uint64_t i_ମୂଲ୍ୟ)
{
    ଯଦି (not i_runtime) {
        ଫେରନ୍ତୁ m3Err_mallocFailed;
    }
    ବାଛ (i_limit) {
    ସ୍ଥିତି c_m3Limit_MemoryBytes:
        ଯଦି (i_ମୂଲ୍ୟ and i_ମୂଲ୍ୟ < i_runtime->memoryBytesUsed) {
            ଫେରନ୍ତୁ m3Err_resourceLimitBelowUsage;
        }
        i_runtime->memoryBytesLimit = i_ମୂଲ୍ୟ;
        ଫେରନ୍ତୁ m3Err_none;
    ସ୍ଥିତି c_m3Limit_TableElements:
        ଯଦି (i_ମୂଲ୍ୟ and i_ମୂଲ୍ୟ < i_runtime->tableElementsUsed) {
            ଫେରନ୍ତୁ m3Err_resourceLimitBelowUsage;
        }
        i_runtime->tableElementsLimit = i_ମୂଲ୍ୟ;
        ଫେରନ୍ତୁ m3Err_none;
#ଯଦି d_m3_Has_ଗଦା_Switching
    ସ୍ଥିତି c_m3Limit_Continuations:
        ଯଦି (i_ମୂଲ୍ୟ and i_ମୂଲ୍ୟ < i_runtime->continuationsAllocated) {
            ଫେରନ୍ତୁ m3Err_resourceLimitBelowUsage;
        }
        i_runtime->continuationsLimit = (u32)M3_MIN(i_ମୂଲ୍ୟ, (u64)UINT32_MAX);
        ଯେତେବେଳେ (i_ମୂଲ୍ୟ and i_runtime->val_ଗଦା_Pool_ଗଣନା > i_ମୂଲ୍ୟ - i_runtime->continuationsAllocated) {
            m3_Free(i_runtime->val_ଗଦା_Pool[--i_runtime->val_ଗଦା_Pool_ଗଣନା]);
        }
        ଫେରନ୍ତୁ m3Err_none;
#ଅନ୍ୟଥା
    ସ୍ଥିତି c_m3Limit_Continuations: ଫେରନ୍ତୁ m3Err_resourceLimitNotSupported;
#endif
    ସ୍ଥିତି c_m3Limit_GasUnits:
#ଯଦି d_m3HasGasMetering
        i_runtime->gasLimit = i_runtime->gasRemaining = (i64)M3_MIN(i_ମୂଲ୍ୟ, (u64)INT64_MAX);
        ଫେରନ୍ତୁ m3Err_none;
#ଅନ୍ୟଥା
        ଫେରନ୍ତୁ m3Err_resourceLimitNotSupported;
#endif
    ପୂର୍ବନିର୍ଦ୍ଧାରିତ: ଫେରନ୍ତୁ m3Err_unknownResourceLimit;
    }
}

uint64_t m3_GetResourceLimit (IM3Runtime i_runtime, M3ResourceLimit i_limit)
{
    ଯଦି (not i_runtime) {
        ଫେରନ୍ତୁ 0;
    }
    ବାଛ (i_limit) {
    ସ୍ଥିତି c_m3Limit_MemoryBytes: ଫେରନ୍ତୁ i_runtime->memoryBytesLimit;
    ସ୍ଥିତି c_m3Limit_TableElements: ଫେରନ୍ତୁ i_runtime->tableElementsLimit;
#ଯଦି d_m3_Has_ଗଦା_Switching
    ସ୍ଥିତି c_m3Limit_Continuations: ଫେରନ୍ତୁ i_runtime->continuationsLimit;
#endif
#ଯଦି d_m3HasGasMetering
    ସ୍ଥିତି c_m3Limit_GasUnits: ଫେରନ୍ତୁ (u64)i_runtime->gasLimit;
#endif
    ପୂର୍ବନିର୍ଦ୍ଧାରିତ: ଫେରନ୍ତୁ 0;
    }
}

uint64_t m3_GetResourceUsage (IM3Runtime i_runtime, M3ResourceLimit i_limit)
{
    ଯଦି (not i_runtime) {
        ଫେରନ୍ତୁ 0;
    }
    ବାଛ (i_limit) {
    ସ୍ଥିତି c_m3Limit_MemoryBytes: ଫେରନ୍ତୁ i_runtime->memoryBytesUsed;
    ସ୍ଥିତି c_m3Limit_TableElements: ଫେରନ୍ତୁ i_runtime->tableElementsUsed;
#ଯଦି d_m3_Has_ଗଦା_Switching
    ସ୍ଥିତି c_m3Limit_Continuations: ଫେରନ୍ତୁ i_runtime->continuationsAllocated;
#endif
#ଯଦି d_m3HasGasMetering
    ସ୍ଥିତି c_m3Limit_GasUnits: ଫେରନ୍ତୁ (u64)i_runtime->gasLimit - (u64)i_runtime->gasRemaining;
#endif
    ପୂର୍ବନିର୍ଦ୍ଧାରିତ: ଫେରନ୍ତୁ 0;
    }
}

ଶୂନ୍ୟ* m3_GetUserData (IM3Runtime i_runtime)
{
    ଫେରନ୍ତୁ i_runtime ? i_runtime->userdata : ଖାଲି;
}


ଶୂନ୍ୟ* ForEachModule (IM3Runtime i_runtime, ModuleVisitor i_visitor, ଶୂନ୍ୟ* i_info)
{
    ଶୂନ୍ୟ* r = ଖାଲି;

    IM3Module module = i_runtime->modules;

    ଯେତେବେଳେ (module) {
        IM3Module next = module->next;
        ଯଦି ((r = i_visitor(module, i_info))) {
            ଭାଙ୍ଗ;
        }

        module = next;
    }

    ଫେରନ୍ତୁ r;
}


ଶୂନ୍ୟ* _FreeModule (IM3Module i_module, ଶୂନ୍ୟ* i_info)
{
    m3_FreeModule(i_module);
    ଫେରନ୍ତୁ ଖାଲି;
}


// A module's memory and table index spaces hold borrowed pointers: linking
// repoints an imported slot at the memory or table the exporting module owns.
// m3_FreeModule tells a borrowed slot from an owned one by reading that owner,
// which only holds while the object is still allocated - and the free walk
// below visits modules in load order, with no ordering between an owner and
// the modules borrowing from it. So clear the borrowed slots first, in a pass
// that frees nothing; the walk then skips them on the NULL check it already
// does, instead of reading a memory some earlier module has freed.
ଶୂନ୍ୟ* _ReleaseBorrowedSlots (IM3Module i_module, ଶୂନ୍ୟ* i_info)
{
    ପାଇଁ (u32 i = 0; i < i_module->ଅଙ୍କ_Memories; ++i) {
        IM3Memory memory = i_module->memories[i];

        ଯଦି (memory and memory->owner != i_module) {
            i_module->memories[i] = ଖାଲି;
        }
    }

    ପାଇଁ (u32 i = 0; i < i_module->ଅଙ୍କ_Tables; ++i) {
        IM3Table table = i_module->tables[i];

        ଯଦି (table and table->owner != i_module) {
            i_module->tables[i] = ଖାଲି;
        }
    }

    ଫେରନ୍ତୁ ଖାଲି;
}


ଶୂନ୍ୟ Runtime_Release (IM3Runtime i_runtime)
{
    ForEachModule(i_runtime, _ReleaseBorrowedSlots, ଖାଲି);
    ForEachModule(i_runtime, _FreeModule, ଖାଲି);                    d_m3Assert (i_runtime->ଅଙ୍କ_Active_Code_Pages == 0);

    Environment_ReleaseCodePages(i_runtime->environment, i_runtime->pagesOpen);
    Environment_ReleaseCodePages(i_runtime->environment, i_runtime->pagesFull);

    m3_Free(i_runtime->origin_ଗଦା);

#ଯଦି d_m3EnableValidation
    m3_Free(i_runtime->validator);
#endif

#ଯଦି d_m3HasExceptionHandling
    // a runtime let go of while suspended still holds its exceptions
    FreeExceptions(i_runtime);
    m3_Free(i_runtime->uncaughtException);
#endif
#ଯଦି d_m3HasExceptionHandling || d_m3_Has_ଗଦା_Switching
    ଯେତେବେଳେ (i_runtime->hostTags) {
        M3HostTag* next = i_runtime->hostTags->next;
        m3_Free(i_runtime->hostTags);
        i_runtime->hostTags = next;
    }
#endif
#ଯଦି d_m3_Has_ଗଦା_Switching
    Continuation_ReleaseAll(i_runtime);
#endif
}


ଶୂନ୍ୟ m3_FreeRuntime (IM3Runtime i_runtime)
{
    ଯଦି (i_runtime) {
        m3_PrintProfilerInfo();

        Runtime_Release(i_runtime);
        m3_Free(i_runtime);
    }
}


#ଯଦି d_m3_Has_ଗଦା_Switching

IM3Continuation Continuation_New (IM3Runtime i_runtime, IM3FuncType i_type, IM3Function i_function)
{
    IM3Continuation cont = m3_AllocStruct(M3Continuation);
    ଯଦି (!cont) {
        ଫେରନ୍ତୁ ଖାଲି;
    }

    cont->type          = i_type;
    cont->entryFunction = i_function;
    cont->state         = cont_allocated;

    cont->ଅଙ୍କ_ଗଦା_Slots = d_m3_Continuation_ଗଦା_Slots;
    // frames are recorded only when this continuation actually suspends, and
    // only as deep as it got, so the array starts out unallocated
    cont->frames    = ଖାଲି;
    cont->framesCap = 0;
    cont->ଅଙ୍କ_Frames = 0;

    cont->sp = cont->val_ଗଦା;
    cont->pc = i_function ? i_function->compiled : ଖାଲି;
    cont->r0 = 0;
#  ଯଦି d_m3HasFloat
    cont->fp0 = 0;
#  endif
    cont->bound_Args_ଗଣନା    = 0;
    cont->ଅଙ୍କ_Suspend_Results = 0;
    cont->ଅଙ୍କ_Handlers       = 0;
    cont->handlersPC        = ଖାଲି;
    cont->parent            = ଖାଲି;

    cont->next               = i_runtime->continuations;
    i_runtime->continuations = cont;

    ଫେରନ୍ତୁ cont;
}

IM3Continuation Continuation_ForkSuspended (IM3Runtime i_runtime, IM3Continuation i_cont)
{
    IM3Continuation newCont = Continuation_New(i_runtime, i_cont->type, i_cont->entryFunction);
    ଯଦି (M3_UNLIKELY(not newCont)) {
        ଫେରନ୍ତୁ ଖାଲି;
    }

    newCont->val_ଗଦା      = i_cont->val_ଗଦା;
    newCont->ଅଙ୍କ_ଗଦା_Slots = i_cont->ଅଙ୍କ_ଗଦା_Slots;
    newCont->sp            = i_cont->sp;
    newCont->pc            = i_cont->pc;
    newCont->suspendPoint  = i_cont->suspendPoint;
    newCont->r0            = i_cont->r0;
#  ଯଦି d_m3HasFloat
    newCont->fp0 = i_cont->fp0;
#  endif
    newCont->frames    = i_cont->frames;
    newCont->framesCap = i_cont->framesCap;
    newCont->ଅଙ୍କ_Frames = i_cont->ଅଙ୍କ_Frames;

    newCont->bound_Args_ଗଣନା = 0;
#  ଯଦି d_m3HasExceptionHandling
    newCont->resumeThrow = i_cont->resumeThrow;
    i_cont->resumeThrow  = ଖାଲି;
#  endif
    newCont->ଅଙ୍କ_Suspend_Results = i_cont->ଅଙ୍କ_Suspend_Results;
    ପାଇଁ (u32 i = 0; i < i_cont->ଅଙ୍କ_Suspend_Results; ++i) {
        newCont->suspendResultOffsets[i] = i_cont->suspendResultOffsets[i];
        newCont->suspendResultIs64[i]    = i_cont->suspendResultIs64[i];
    }

    newCont->ଅଙ୍କ_Handlers = i_cont->ଅଙ୍କ_Handlers;
    newCont->handlersPC  = i_cont->handlersPC;
    newCont->parent      = i_cont->parent;
    newCont->state       = cont_suspended;

    i_cont->val_ଗଦା  = ଖାଲି;
    i_cont->frames    = ଖାଲି;
    i_cont->framesCap = 0;
    i_cont->ଅଙ୍କ_Frames = 0;
    i_cont->state     = cont_consumed;

    ଫେରନ୍ତୁ newCont;
}

// Only cont.new and snapshot restoration acquire stacks. Binding and suspension
// transfer them between records without changing the active count.
M3Result Continuation_Acquire_ଗଦା (IM3Runtime runtime, IM3Continuation cont)
{
    ଯଦି (not Continuation_Can_Acquire_ଗଦା(runtime)) {
        ଫେରନ୍ତୁ m3Err_continuationLimitExceeded;
    }
    ଯଦି (runtime->val_ଗଦା_Pool_ଗଣନା) {
        cont->val_ଗଦା = runtime->val_ଗଦା_Pool[--runtime->val_ଗଦା_Pool_ଗଣନା];
        memset(cont->val_ଗଦା, 0, (cont->ଅଙ୍କ_ଗଦା_Slots + 4) * ଆକାର(m3slot_t));
    } ଅନ୍ୟଥା {
        cont->val_ଗଦା = m3_AllocArray(m3slot_t, cont->ଅଙ୍କ_ଗଦା_Slots + 4);
    }
    ଯଦି (not cont->val_ଗଦା) {
        ଫେରନ୍ତୁ m3Err_mallocFailed;
    }
    cont->sp = cont->val_ଗଦା;
    runtime->continuationsAllocated++;
    ଫେରନ୍ତୁ m3Err_none;
}

ଶୂନ୍ୟ Continuation_Recycle_ଗଦା (IM3Runtime runtime, IM3Continuation cont)
{
    ଯଦି (not cont->val_ଗଦା or cont == runtime->ମୂଳ_Continuation) {
        ଫେରନ୍ତୁ;
    }
    ଯଦି (not runtime->val_ଗଦା_Pool) {
        runtime->val_ଗଦା_Pool = m3_AllocArray(m3slot_t*, 16);
        ଯଦି (runtime->val_ଗଦା_Pool) {
            runtime->val_ଗଦା_Pool_Cap = 16;
        }
    }
    ଯଦି (runtime->val_ଗଦା_Pool_ଗଣନା < runtime->val_ଗଦା_Pool_Cap and
        (not runtime->continuationsLimit or runtime->val_ଗଦା_Pool_ଗଣନା <
                                              runtime->continuationsLimit - runtime->continuationsAllocated + 1)) {
        runtime->val_ଗଦା_Pool[runtime->val_ଗଦା_Pool_ଗଣନା++] = cont->val_ଗଦା;
        cont->val_ଗଦା                                      = ଖାଲି;
    } ଅନ୍ୟଥା {
        m3_Free(cont->val_ଗଦା);
    }
    cont->sp = ଖାଲି;
    runtime->continuationsAllocated--;
}

ଶୂନ୍ୟ Continuation_ReleaseAll (IM3Runtime io_runtime)
{
    // Records live until teardown: a consumed reference can still be in a table
    // or on a Wasm stack. Its value stack has already been recycled.
    IM3Continuation cont = io_runtime->continuations;

    ଯେତେବେଳେ (cont) {
        IM3Continuation next = cont->next;

        m3_Free(cont->val_ଗଦା);
        m3_Free(cont->frames);
        m3_Free(cont);

        cont = next;
    }

    ପାଇଁ (u32 i = 0; i < io_runtime->val_ଗଦା_Pool_ଗଣନା; ++i) {
        m3_Free(io_runtime->val_ଗଦା_Pool[i]);
    }
    m3_Free(io_runtime->val_ଗଦା_Pool);
    io_runtime->val_ଗଦା_Pool_ଗଣନା = io_runtime->val_ଗଦା_Pool_Cap = 0;
    io_runtime->continuationsAllocated                          = 0;
    io_runtime->continuations                                   = ଖାଲି;
    io_runtime->activeContinuation                              = ଖାଲି;
    io_runtime->ମୂଳ_Continuation                                = ଖାଲି;
}


m3ret_t Continuation_RecordFrame (IM3Runtime i_runtime, ଅଚଳ M3Frame* i_frame)
{
    IM3Continuation cont = i_runtime->activeContinuation;

    // the marker only travels between a suspend and the resume that runs it,
    // and that whole stretch has an active continuation
    d_m3Assert(cont);

    ଯଦି (cont->ଅଙ୍କ_Frames == cont->framesCap) {
        u32 newCap = cont->framesCap ? cont->framesCap * 2 : 8;

        ଯଦି (newCap > d_m3ContinuationMaxFrames) {
            newCap = d_m3ContinuationMaxFrames;
        }

        ଯଦି (newCap == cont->framesCap) {
            ଫେରନ୍ତୁ m3_Err_trap_ଗଦା_Overflow;
        }

        M3Frame* frames = m3_ReallocArray(M3Frame, cont->frames, newCap, cont->framesCap);

        ଯଦି (not frames) {
            ଫେରନ୍ତୁ m3Err_mallocFailed;
        }

        cont->frames    = frames;
        cont->framesCap = newCap;
    }

    cont->frames[cont->ଅଙ୍କ_Frames++] = *i_frame;

    ଫେରନ୍ତୁ m3Err_continuationSuspended;
}

#endif // d_m3HasStackSwitching

M3Result EvaluateExpression (IM3Module i_module, ଶୂନ୍ୟ* o_expressed, m3type_t i_type, bytes_t* io_bytes, cbytes_t i_end)
{
    M3Result result = m3Err_none;

    // OPTZ: use a simplified interpreter for expressions

    // create a temporary runtime context
#ଯଦି defined(d_m3PreferStaticAlloc)
    ସ୍ଥିର M3Runtime runtime;
#ଅନ୍ୟଥା
    M3Runtime runtime;
#endif
    M3_INIT(runtime);

    runtime.environment   = i_module->runtime->environment;
    runtime.ଅଙ୍କ_ଗଦା_Slots = i_module->runtime->ଅଙ୍କ_ଗଦା_Slots;
    runtime.ଗଦା         = i_module->runtime->ଗଦା;

    m3stack_t ଗଦା = (m3stack_t)runtime.ଗଦା;

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

    ଯଦି (o->page) {
        IM3FuncType ftype = ଖାଲି;

#ଯଦି d_m3HasTypedRefs
        // A reference type has to be checked as the type it is: the shared
        // return types are one per storage class, so borrowing one of those
        // would let any reference at all satisfy a particular one.
        ଯଦି (IsSpelledRefType(i_type)) {
            result = AllocFuncType(&ftype, 1);

            ଯଦି (not result) {
                ftype->ଅଙ୍କ_Rets  = 1;
                ftype->types[0] = i_type;

                result = Environment_AddFuncType(runtime.environment, &ftype);
            }
        } ଅନ୍ୟଥା
#endif
        {
            ftype = runtime.environment->retFuncTypes[BaseTypeOf(i_type)];
        }

        pc_t m3code = GetPagePC(o->page);

        ଯଦି (not result) {
            result = CompileExpression(o, ftype);
        }

        ଯଦି (not result && o->max_ଗଦା_Slots >= runtime.ଅଙ୍କ_ଗଦା_Slots) {
            result = m3_Err_trap_ଗଦା_Overflow;
        }

        ଯଦି (not result) {
#ଯଦି (d_m3EnableOpProfiling || d_m3EnableOpTracing)
            m3ret_t r = RunCode(m3code, ଗଦା, ଖାଲି, d_m3OpDefaultArgs, d_m3BaseCstr);
#ଅନ୍ୟଥା
            m3ret_t r = RunCode(m3code, ଗଦା, ଖାଲି, d_m3OpDefaultArgs);
#endif

            ଯଦି (r == 0) {                                                                   m3log (runtime, "expression result: %s", SPrint_ମୂଲ୍ୟ (ଗଦା, i_type));
                ଯଦି (SizeOfType(BaseTypeOf(i_type)) == ଆକାର(u32)) {
                    *(u32*)o_expressed = *((u32*)ଗଦା);
                } ଅନ୍ୟଥା {
                    *(u64*)o_expressed = *((u64*)ଗଦା);
                }
            }
        }

        ReleaseCodePage(&runtime, o->page);
    } ଅନ୍ୟଥା {
        result = m3Err_mallocFailedCodePage;
    }

    runtime.origin_ଗଦା = ଖାଲି;        // prevent free(stack) in ReleaseRuntime
    Runtime_Release(&runtime);
    i_module->runtime = savedRuntime;

    *io_bytes = o->wasm;

    ଫେରନ୍ତୁ result;
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
ସ୍ଥିର
bool LimitsSatisfy (u64 i_exportedSize, bool i_exportedHasMax, u64 i_exportedMax,
                    u64 i_importMin, bool i_importHasMax, u64 i_importMax)
{
    ଯଦି (i_exportedSize < i_importMin) {
        ଫେରନ୍ତୁ false;
    }

    ଯଦି (i_importHasMax and (not i_exportedHasMax or i_exportedMax > i_importMax)) {
        ଫେରନ୍ତୁ false;
    }

    ଫେରନ୍ତୁ true;
}


// The export of this kind under this name, or NULL. The index it carries is
// resolved by the caller, against the module's index space as it stands then -
// after linking, a re-exported import's slot holds the entity it was linked to.
ସ୍ଥିର
ଅଚଳ M3Export* Module_FindExport (IM3Module i_module, u8 i_kind, cstr_t i_name)
{
    ପାଇଁ (u32 i = 0; i < i_module->ଅଙ୍କ_Exports; ++i) {
        ଅଚଳ M3Export* e = &i_module->exports[i];

        ଯଦି (e->kind == i_kind and strcmp(e->name, i_name) == 0) {
            ଫେରନ୍ତୁ e;
        }
    }

    ଫେରନ୍ତୁ ଖାଲି;
}


ସ୍ଥିର
IM3Function Module_FindExportedFunction (IM3Module i_module, cstr_t i_name)
{
    ଅଚଳ M3Export* e = Module_FindExport(i_module, d_externalKind_function, i_name);

    // A module that re-exports an import names the placeholder here. Resolve to
    // the function that actually runs, or a host-side call would run it against
    // the importing module's memory.
    ଫେରନ୍ତୁ e ? Function_Implementation(&i_module->functions[e->index]) : ଖାଲି;
}


ସ୍ଥିର
IM3Memory Module_FindExportedMemory (IM3Module i_module, cstr_t i_name)
{
    ଅଚଳ M3Export* e = Module_FindExport(i_module, d_externalKind_memory, i_name);

    ଫେରନ୍ତୁ e ? i_module->memories[e->index] : ଖାଲି;
}


ସ୍ଥିର
IM3Table Module_FindExportedTable (IM3Module i_module, cstr_t i_name)
{
    ଅଚଳ M3Export* e = Module_FindExport(i_module, d_externalKind_table, i_name);

    ଫେରନ୍ତୁ e ? i_module->tables[e->index] : ଖାଲି;
}


ସ୍ଥିର
IM3Global Module_FindExportedGlobal (IM3Module i_module, cstr_t i_name)
{
    ଅଚଳ M3Export* e = Module_FindExport(i_module, d_externalKind_global, i_name);

    ଫେରନ୍ତୁ e ? &i_module->globals[e->index] : ଖାଲି;
}


#ଯଦି d_m3HasExceptionHandling || d_m3_Has_ଗଦା_Switching
ସ୍ଥିର
IM3Tag Module_FindExportedTag (IM3Module i_module, cstr_t i_name)
{
    ଅଚଳ M3Export* e = Module_FindExport(i_module, d_externalKind_tag, i_name);

    ଯଦି (not e) {
        ଫେରନ୍ତୁ ଖାଲି;
    }

    IM3Tag tag = &i_module->tags[e->index];

    ଫେରନ୍ତୁ tag->resolved ? tag->resolved : tag;
}
#endif


// Whether the module exports anything at all under this name. Export names are
// unique within a module, so a name one of the lookups above missed but this
// one finds is exported as something else - a kind the import cannot be
// satisfied by, rather than a name the module never exported.
ସ୍ଥିର
bool Module_HasExport (IM3Module i_module, cstr_t i_name)
{
    ପାଇଁ (u32 i = 0; i < i_module->ଅଙ୍କ_Exports; ++i) {
        ଯଦି (strcmp(i_module->exports[i].name, i_name) == 0) {
            ଫେରନ୍ତୁ true;
        }
    }

    ଫେରନ୍ତୁ false;
}


// Points each of the module's imports at whatever already-loaded module exports
// it. Runs before anything is allocated or initialized: a memory import has to
// be resolved before InitMemory would give it pages of its own, and a global
// import before InitGlobals runs an initializer that reads it.
ସ୍ଥିର
M3Result LinkImports (IM3Runtime io_runtime, IM3Module io_module)
{
    M3Result result = m3Err_none;

    ପାଇଁ (u32 i = 0; i < io_module->ଅଙ୍କ_Functions; ++i) {
        IM3Function f = &io_module->functions[i];

        ଯଦି (f->wasm or not (f->import.moduleUtf8 and f->import.fieldUtf8)) {
            ଜାରି;
        }

        IM3Module from = m3_FindModule(io_runtime, f->import.moduleUtf8);
        ଯଦି (not from) {
            ଜାରି;
        }

        IM3Function exported = Module_FindExportedFunction(from, f->import.fieldUtf8);
        ଯଦି (not exported) {
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

    ପାଇଁ (u32 i = 0; i < io_module->ଅଙ୍କ_Memories; ++i) {
        IM3Memory memory = io_module->memories[i];

        ଯଦି (not memory->imported or memory->owner != io_module) {
            ଜାରି;
        }

        IM3Module from = m3_FindModule(io_runtime, memory->import.moduleUtf8);
        ଯଦି (not from) {
            ଜାରି;
        }

        IM3Memory exported = Module_FindExportedMemory(from, memory->import.fieldUtf8);
        ଯଦି (not exported) {
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
                 not LimitsSatisfy(exported->ଅଙ୍କ_Pages, exported->hasMax, exported->maxPages,
                                   memory->initPages, memory->hasMax, memory->maxPages));

        // hand the slot over to the exporter's memory, and drop the placeholder
        FreeMemoryBlock(memory);
        FreeImportInfo(&memory->import);
        m3_Free(memory);

        io_module->memories[i] = exported;
    }

    ପାଇଁ (u32 i = 0; i < io_module->ଅଙ୍କ_Tables; ++i) {
        IM3Table table = io_module->tables[i];

        ଯଦି (not table->imported or table->owner != io_module) {
            ଜାରି;
        }

        IM3Module from = m3_FindModule(io_runtime, table->import.moduleUtf8);
        ଯଦି (not from) {
            ଜାରି;
        }

        IM3Table exported = Module_FindExportedTable(from, table->import.fieldUtf8);
        ଯଦି (not exported) {
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

    ପାଇଁ (u32 i = 0; i < io_module->ଅଙ୍କ_Globals; ++i) {
        IM3Global g = &io_module->globals[i];

        ଯଦି (not g->imported or not (g->import.moduleUtf8 and g->import.fieldUtf8)) {
            ଜାରି;
        }

        IM3Module from = m3_FindModule(io_runtime, g->import.moduleUtf8);
        ଯଦି (not from) {
            ଜାରି;
        }

        IM3Global exported = Module_FindExportedGlobal(from, g->import.fieldUtf8);
        ଯଦି (not exported) {
            _throwif(m3Err_incompatibleImportType, Module_HasExport(from, g->import.fieldUtf8));
            _throw(m3Err_unknownImport);
        }

        _throwif(m3Err_incompatibleImportType, exported->type != g->type);
        _throwif(m3Err_incompatibleImportType, exported->isMutable != g->isMutable);

        g->resolved = exported->resolved ? exported->resolved : exported;
    }

#ଯଦି d_m3HasExceptionHandling || d_m3_Has_ଗଦା_Switching
    ପାଇଁ (u32 i = 0; i < io_module->ଅଙ୍କ_Tags; ++i) {
        IM3Tag tag = &io_module->tags[i];

        ଯଦି (not tag->imported or not (tag->import.moduleUtf8 and tag->import.fieldUtf8)) {
            ଜାରି;
        }

        IM3Module from = m3_FindModule(io_runtime, tag->import.moduleUtf8);
        ଯଦି (not from) {
            ଜାରି;
        }

        IM3Tag exported = Module_FindExportedTag(from, tag->import.fieldUtf8);
        ଯଦି (not exported) {
            _throwif(m3Err_incompatibleImportType, Module_HasExport(from, tag->import.fieldUtf8));
            _throw(m3Err_unknownImport);
        }

        _throwif(m3Err_incompatibleImportType, exported->type != tag->type);

        tag->resolved = exported->resolved ? exported->resolved : exported;
    }
#endif

    _catch: ଫେରନ୍ତୁ result;
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
    i_module->memory0 = i_module->ଅଙ୍କ_Memories ? i_module->memories[0]
                                              : &i_module->emptyMemory;

    ଯଦି (i_module->ଅଙ୍କ_Memories == 0) {
        // nothing addressable, but _mem still has to point somewhere
        i_module->emptyMemory.owner    = i_module;
        i_module->emptyMemory.pageSize = d_m3DefaultMemPageSize;

_       (ResizeMemory(io_runtime, &i_module->emptyMemory, 0));
    }

    ପାଇଁ (u32 i = 0; i < i_module->ଅଙ୍କ_Memories; ++i) {
        IM3Memory memory = i_module->memories[i];

        // a slot that already points at another module's memory is that
        // module's to allocate
        ଯଦି (memory->owner != i_module or memory->mallocated) {
            ଜାରି;
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
        ଯଦି (not memory->hasMax) {
            memory->maxPages = memory->isMemory64
                                 ? (pageSize > 1 ? (UINT64_MAX / pageSize) + 1 : UINT64_MAX)
                                 : (0x100000000ull / pageSize);
        }

_       (ResizeMemory(io_runtime, memory, memory->initPages));
    }

    _catch: ଫେରନ୍ତୁ result;
}


M3Result ResizeMemory (IM3Runtime io_runtime, IM3Memory memory, u64 i_ଅଙ୍କ_Pages)
{
    M3Result result = m3Err_none;

    u64 ଅଙ୍କ_Pages_To_Alloc = i_ଅଙ୍କ_Pages;

    ଯଦି (ଅଙ୍କ_Pages_To_Alloc <= memory->maxPages) {
        // A 64-bit memory may ask for up to 2^48 pages, which overflows a u64
        // of bytes. Nothing that large can be backed, so refuse it up front
        // rather than multiplying into a wrapped size.
        _throwif("linear memory limitation exceeded",
                 ଅଙ୍କ_Pages_To_Alloc > d_m3AddressLimit / memory->pageSize);

        u64 ଅଙ୍କ_Page_Bytes = ଅଙ୍କ_Pages_To_Alloc * memory->pageSize;

#ଯଦି d_m3MaxLinearMemoryPages > 0
        // the limit is a memory size, counted in default-sized pages; comparing
        // it against a raw page count would make it 65536 times stricter for a
        // module whose pages are one byte
        _throwif("linear memory limitation exceeded",
                 ଅଙ୍କ_Page_Bytes > (u64)d_m3MaxLinearMemoryPages * d_m3DefaultMemPageSize);
#endif

        // Limit the amount of memory that gets actually allocated
        ଯଦି (io_runtime->memoryLimit) {
            ଅଙ୍କ_Page_Bytes = M3_MIN(ଅଙ୍କ_Page_Bytes, (u64)io_runtime->memoryLimit);
        }

        u64 oldLength = memory->mallocated ? memory->mallocated->length : 0;
        u64 otherUsed = io_runtime->memoryBytesUsed - oldLength;
        _throwif(m3Err_memoryLimitExceeded, io_runtime->memoryBytesLimit and
                                              (ଅଙ୍କ_Page_Bytes > io_runtime->memoryBytesLimit or
                                               otherUsed > io_runtime->memoryBytesLimit - ଅଙ୍କ_Page_Bytes));

        _throwif("linear memory limitation exceeded", ଅଙ୍କ_Page_Bytes > (u64)SIZE_MAX - ଆକାର(M3MemoryHeader));

#ଯଦି d_m3GuardedMemory

        // A slot holds every address a Wasm access can name, and nothing beyond
        // that: a memory larger than the data part of one would put addresses past
        // the guard, where a fault is nobody's to recognize
        _throwif("linear memory limitation exceeded", ଅଙ୍କ_Page_Bytes > d_m3GuardedDataBytes);

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

        ଯଦି (memory->guardSlot == ଖାଲି) {
            memory->guardSlot = Guard_TakeSlot();
            _throwifnull(memory->guardSlot);
        }

        // Growing only ever commits more of the same slot, so the memory never
        // moves - which the interpreter does not rely on, but nothing minds
        _throwif(m3Err_mallocFailed, not Guard_CommitSlot(memory->guardSlot, (size_t)ଅଙ୍କ_Page_Bytes));

        memory->mallocated = Guard_SlotHeader(memory->guardSlot);

#ଅନ୍ୟଥା

        size_t ଅଙ୍କ_Bytes = (size_t)ଅଙ୍କ_Page_Bytes + ଆକାର(M3MemoryHeader);

        // What is really there, which is not numPages * pageSize: the limit above
        // caps the bytes without capping the page count, so a memory can be short
        // of a page - by design, since a whole 64 KiB page is often more than an
        // MCU has. Measuring the old block by the page count would overstate it,
        // and then realloc never sees the size it already has and copies the
        // whole limit again on every grow past it.
        size_t ଅଙ୍କ_Previous_Bytes = memory->mallocated ? memory->mallocated->length + ଆକାର(M3MemoryHeader) : 0;

        ଶୂନ୍ୟ* newMem = m3_Realloc("Wasm Linear Memory", memory->mallocated, ଅଙ୍କ_Bytes, ଅଙ୍କ_Previous_Bytes);
        _throwifnull(newMem);

        memory->mallocated = (M3MemoryHeader*)newMem;

#endif

#ଯଦି d_m3LogRuntime
        M3MemoryHeader* oldMallocated = memory->mallocated;
#endif

        io_runtime->memoryBytesUsed = otherUsed + ଅଙ୍କ_Page_Bytes;
        memory->ଅଙ୍କ_Pages            = ଅଙ୍କ_Pages_To_Alloc;

        memory->mallocated->length  = (size_t)ଅଙ୍କ_Page_Bytes;
        memory->mallocated->runtime = io_runtime;
        memory->mallocated->memory  = memory;

        memory->mallocated->max_ଗଦା = (m3slot_t*)io_runtime->origin_ଗଦା + io_runtime->ଅଙ୍କ_ଗଦା_Slots;

        m3log(runtime, "resized old: %p; mem: %p; length: %zu; pages: %llu", oldMallocated, memory->mallocated, memory->mallocated->length, (unsigned long long)memory->ଅଙ୍କ_Pages);
    } ଅନ୍ୟଥା {
        result = m3Err_wasmMemoryOverflow;
    }

    _catch: ଫେରନ୍ତୁ result;
}


ଶୂନ୍ୟ FreeMemoryBlock (IM3Memory io_memory)
{
    ଯଦି (io_memory->mallocated and io_memory->mallocated->runtime) {
        io_memory->mallocated->runtime->memoryBytesUsed -= io_memory->mallocated->length;
    }
#ଯଦି d_m3GuardedMemory
    Guard_GiveSlot(io_memory->guardSlot);
    io_memory->guardSlot = ଖାଲି;

    io_memory->mallocated = ଖାଲି;
#ଅନ୍ୟଥା
    m3_Free(io_memory->mallocated);
#endif
}


M3Result InitGlobals (IM3Module io_module)
{
    M3Result result = m3Err_none;

    ଯଦି (io_module->ଅଙ୍କ_Globals) {
        // placing the globals in their structs isn't good for cache locality, but i don't really know what the global
        // access patterns typically look like yet.

        //          io_module->globalMemory = m3Alloc (m3reg_t, io_module->numGlobals);

        //          if (io_module->globalMemory)
        {
            ପାଇଁ (u32 i = 0; i < io_module->ଅଙ୍କ_Globals; ++i) {
                M3Global* g = &io_module->globals[i];                           m3log (runtime, "initializing global: %d", i);

                ଯଦି (g->init_ପଦସମଷ୍ଟି) {
                    bytes_t start = g->init_ପଦସମଷ୍ଟି;

                    result = EvaluateExpression(io_module, &g->i64_ମୂଲ୍ୟ, g->type, &start, g->init_ପଦସମଷ୍ଟି + g->init_ପଦସମଷ୍ଟି_Size);

                    ଯଦି (not result) {
                        // io_module->globalMemory [i] = initValue;
                    } ଅନ୍ୟଥା {
                        ଭାଙ୍ଗ;
                    }
                } ଅନ୍ୟଥା {                                                        m3log (runtime, "importing global");
                }
            }
        }
        //          else result = ErrorModule (m3Err_mallocFailed, io_module, "could allocate globals for module: '%s", io_module->name);
    }

    ଫେରନ୍ତୁ result;
}


M3Result InitDataSegments (IM3Module io_module)
{
    M3Result result = m3Err_none;

    ପାଇଁ (u32 i = 0; i < io_module->ଅଙ୍କ_Data_Segments; ++i) {
        M3DataSegment* segment = &io_module->dataSegments[i];

        // A passive segment stays available for memory.init until data.drop.
        // An active one is copied here and then counts as dropped.
        ଯଦି (segment->isPassive) {
            ଜାରି;
        }

        _throwif("data segment memory index out of range",
                 segment->memoryRegion >= io_module->ଅଙ୍କ_Memories);

        IM3Memory io_memory = io_module->memories[segment->memoryRegion];

        _throwif("unallocated linear memory", !(io_memory->mallocated));

        // The offset expression has the memory's address type, and is
        // unsigned: an i32 offset of -1 is 4294967295, way out of bounds
        // rather than negative.
        u64     segmentOffset = 0;
        bytes_t start         = segment->init_ପଦସମଷ୍ଟି;

        ଯଦି (io_memory->isMemory64) {
_           (EvaluateExpression(io_module, &segmentOffset, c_m3Type_i64, &start, segment->init_ପଦସମଷ୍ଟି + segment->init_ପଦସମଷ୍ଟି_Size));
        } ଅନ୍ୟଥା {
            u32 offset32;
_           (EvaluateExpression(io_module, &offset32, c_m3Type_i32, &start, segment->init_ପଦସମଷ୍ଟି + segment->init_ପଦସମଷ୍ଟି_Size));
            segmentOffset = offset32;
        }

        m3log(runtime, "loading data segment: %d; size: %d; offset: %llu", i, segment->size, (unsigned long long)segmentOffset);

        ଯଦି (segmentOffset <= io_memory->mallocated->length &&
            (u64)segment->size <= io_memory->mallocated->length - segmentOffset) {
            u8* dest = m3MemData(io_memory->mallocated) + segmentOffset;
            memcpy(dest, segment->data, segment->size);
        } ଅନ୍ୟଥା {
            _throw("data segment out of bounds");
        }

        segment->dropped = true;
    }

    _catch: ଫେରନ୍ତୁ result;
}


// Turns a segment's elements into references. Element expressions are constant
// expressions restricted to ref.null/ref.func, so they're read directly rather
// than run through the compiler.
ସ୍ଥିର
M3Result ResolveElements (IM3Module io_module, M3ElementSegment* i_segment, ଶୂନ୍ୟ** o_elements)
{
    M3Result result = m3Err_none;

    bytes_t  pos = i_segment->elements;
    cbytes_t end = io_module->elementSectionEnd;

    ପାଇଁ (u32 e = 0; e < i_segment->ଅଙ୍କ_Elements; ++e) {
        u32   funcIndex;
        ଶୂନ୍ୟ* ref = ଖାଲି;

        ଯଦି (i_segment->is_ପଦସମଷ୍ଟି) {
            m3opcode_t opcode;
_           (Read_opcode(&opcode, &pos, end));

            ଯଦି (opcode == c_waOp_refFunc) {
_               (ReadLEB_u32(&funcIndex, &pos, end));
                _throwif("function index out of range", funcIndex >= io_module->ଅଙ୍କ_Functions);
                ref = Function_Implementation(&io_module->functions[funcIndex]);
            } ଅନ୍ୟଥା ଯଦି (opcode == c_waOp_refNull) {
                i8 waType;
                u8 nullType;
_               (ReadLEB_i7(&waType, &pos, end));
_               (NormalizeType(&nullType, waType));
                _throwif(m3Err_typeMismatch, nullType != i_segment->type);
            } ଅନ୍ୟଥା ଯଦି (opcode == c_waOp_getGlobal) {
                // wasm 2.0 lets an element expression read an imported
                // immutable global, which is how one module seeds another's
                // table with a reference it exported.
                u32 globalIndex;
_               (ReadLEB_u32(&globalIndex, &pos, end));
                _throwif(m3Err_globaIndexOutOfBounds, globalIndex >= io_module->ଅଙ୍କ_Globals);

                IM3Global global = &io_module->globals[globalIndex];

                _throwif(m3Err_globaIndexOutOfBounds, not global->imported);
                _throwif(m3Err_wasmMalformed, global->isMutable);
                _throwif(m3Err_typeMismatch, BaseTypeOf(global->type) != BaseTypeOf(i_segment->type));

                // read the cell the import was linked to, not the placeholder
                ଯଦି (global->resolved) {
                    global = global->resolved;
                }

                ref = global->ref_ମୂଲ୍ୟ;
            } ଅନ୍ୟଥା {
                _throw("constant expression required");
            }

_           (Read_opcode(&opcode, &pos, end));
            _throwif(m3Err_wasmMalformed, opcode != c_waOp_end);
        } ଅନ୍ୟଥା {
_           (ReadLEB_u32(&funcIndex, &pos, end));
            _throwif("function index out of range", funcIndex >= io_module->ଅଙ୍କ_Functions);
            ref = Function_Implementation(&io_module->functions[funcIndex]);
        }

        o_elements[e] = ref;
    }

    _catch: ଫେରନ୍ତୁ result;
}


M3Result InitTableAndElements (IM3Module io_module)
{
    M3Result result = m3Err_none;

    cbytes_t end = io_module->elementSectionEnd;
    M3Table* table;

    ପାଇଁ (u32 i = 0; i < io_module->ଅଙ୍କ_Tables; ++i) {
        table = io_module->tables[i];

        // a slot pointing at another module's table is that module's to fill
        ଯଦି (table->owner != io_module) {
            ଜାରି;
        }

        ଯଦି (table->size) {
            IM3Runtime runtime = io_module->runtime;
            _throwif(m3Err_tableLimitExceeded, runtime->tableElementsLimit and
                                                 table->size > runtime->tableElementsLimit - runtime->tableElementsUsed);
            table->elements = m3_AllocArray(ଶୂନ୍ୟ*, table->size);
            _throwifnull(table->elements);
            runtime->tableElementsUsed += table->size;

            ଯଦି (table->init_ପଦସମଷ୍ଟି) {
                ଶୂନ୍ୟ*   ମୂଲ୍ୟ = ଖାଲି;
                bytes_t start = table->init_ପଦସମଷ୍ଟି;
_               (EvaluateExpression(io_module, &ମୂଲ୍ୟ, BaseTypeOf(table->type),
                                     &start, table->init_ପଦସମଷ୍ଟି + table->init_ପଦସମଷ୍ଟି_Size));

                ପାଇଁ (u32 e = 0; e < table->size; ++e) {
                    table->elements[e] = ମୂଲ୍ୟ;
                }
            }
        }
    }

    ପାଇଁ (u32 i = 0; i < io_module->ଅଙ୍କ_Element_Segments; ++i) {
        M3ElementSegment* segment = &io_module->elementSegments[i];

        // Declarative segments only make their functions referenceable, and
        // passive ones wait for table.init, so neither is written out here.
        ଯଦି (segment->mode == c_m3Elem_declarative) {
            segment->dropped = true;
            ଜାରି;
        }

        ଯଦି (segment->mode == c_m3Elem_passive) {
            ଯଦି (segment->ଅଙ୍କ_Elements) {
                segment->resolved = m3_AllocArray(ଶୂନ୍ୟ*, segment->ଅଙ୍କ_Elements);
                _throwifnull(segment->resolved);
_               (ResolveElements(io_module, segment, segment->resolved));
            }
            ଜାରି;
        }

        table = io_module->tables[segment->tableIndex];

        // The offset expression has the table's index type, and is unsigned:
        // an i32 offset of -1 is 4294967295, out of bounds rather than negative.
        u64     offset = 0;
        bytes_t ପଦସମଷ୍ଟି   = segment->init_ପଦସମଷ୍ଟି;

        ଯଦି (table->isTable64) {
_           (EvaluateExpression(io_module, &offset, c_m3Type_i64, &ପଦସମଷ୍ଟି, end));
        } ଅନ୍ୟଥା {
            u32 offset32;
_           (EvaluateExpression(io_module, &offset32, c_m3Type_i32, &ପଦସମଷ୍ଟି, end));
            offset = offset32;
        }

        _throwif("out of bounds table access",
                 offset > table->size or segment->ଅଙ୍କ_Elements > table->size - offset);

_       (ResolveElements(io_module, segment, table->elements + offset));

        segment->dropped = true;
    }

    _catch: ଫେରନ୍ତୁ result;
}

M3Result m3_CompileModule (IM3Module io_module)
{
    M3Result result = m3Err_none;

    ପାଇଁ (u32 i = 0; i < io_module->ଅଙ୍କ_Functions; ++i) {
        IM3Function f = &io_module->functions[i];
        ଯଦି (f->wasm and not f->compiled) {
_           (CompileFunction(f));
        }
    }

    _catch: ଫେରନ୍ତୁ result;
}

#ଯଦି d_m3HasExceptionHandling

M3Exception* NewException (IM3Runtime io_runtime, IM3Tag i_tag, u32 i_ଅଙ୍କ_Args)
{
    M3Exception* exception = (M3Exception*)m3_Malloc("M3Exception", ଆକାର(M3Exception) + i_ଅଙ୍କ_Args * ଆକାର(u64));

    ଯଦି (exception) {
        exception->tag     = i_tag;
        exception->ଅଙ୍କ_Args = i_ଅଙ୍କ_Args;
        exception->reified = false;
        exception->prev    = ଖାଲି;
        exception->next    = io_runtime->exceptions;

        ଯଦି (exception->next) {
            exception->next->prev = exception;
        }

        io_runtime->exceptions = exception;
    }

    ଫେରନ୍ତୁ exception;
}


// Releases one exception ahead of the rest. The caller has to know nothing can
// still name it: no exnref was ever taken of it, and its payload has already
// been copied out.
ଶୂନ୍ୟ FreeException (IM3Runtime io_runtime, M3Exception* i_exception)
{
    ଯଦି (i_exception->prev) {
        i_exception->prev->next = i_exception->next;
    } ଅନ୍ୟଥା {
        io_runtime->exceptions = i_exception->next;
    }

    ଯଦି (i_exception->next) {
        i_exception->next->prev = i_exception->prev;
    }

    ଯଦି (io_runtime->pendingException == i_exception) {
        io_runtime->pendingException = ଖାଲି;
    }

    m3_Free_Impl(i_exception);
}


// Releases every exception the runtime still holds. Only safe once the Wasm
// stack is empty, which is why the outermost RunCodeChecked() is the one that
// calls it.
ଶୂନ୍ୟ FreeExceptions (IM3Runtime io_runtime)
{
    M3Exception* exception = io_runtime->exceptions;

    io_runtime->exceptions       = ଖାଲି;
    io_runtime->pendingException = ଖାଲି;

    ଯେତେବେଳେ (exception) {
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
ସ୍ଥିର
ଶୂନ୍ୟ RecordUncaughtException (IM3Runtime io_runtime, M3Result i_result)
{
    ଯଦି (io_runtime->uncaughtException) {
        m3_Free(io_runtime->uncaughtException);
    }

    M3Exception* exception = io_runtime->pendingException;

    ଯଦି (M3_UNLIKELY(i_result == m3Err_trapUncaughtException) and exception) {
        M3Exception* copy = (M3Exception*)m3_CopyMem(exception, ଆକାର(M3Exception) + exception->ଅଙ୍କ_Args * ଆକାର(u64));

        ଯଦି (copy) {
            copy->next = ଖାଲି;
            copy->prev = ଖାଲି;
        }

        io_runtime->uncaughtException = copy;
    }
}

#endif // d_m3HasExceptionHandling


#ଯଦି d_m3HasExceptionHandling || d_m3_Has_ଗଦା_Switching

IM3Tag m3_FindTag (IM3Module i_module, ଅଚଳ ଅକ୍ଷର* ଅଚଳ i_tagName)
{
    ଯଦି (not i_module or not i_tagName) {
        ଫେରନ୍ତୁ ଖାଲି;
    }

    ଫେରନ୍ତୁ Module_FindExportedTag(i_module, i_tagName);
}


M3Result m3_NewTag (IM3Runtime io_runtime, IM3Tag* o_tag, ଅଚଳ ଅକ୍ଷର* ଅଚଳ i_signature)
{
    IM3FuncType type    = ଖାଲି;
    M3HostTag*  hostTag = ଖାଲି;

_try {
    _throwif("null tag", not o_tag);
    *o_tag = ଖାଲି;
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
    ଫେରନ୍ତୁ result;
}


M3Result m3_LinkTag (IM3Module         io_module,
                     ଅଚଳ ଅକ୍ଷର* ଅଚଳ i_moduleName,
                     ଅଚଳ ଅକ୍ଷର* ଅଚଳ i_tagName,
                     IM3Tag            i_tag)
{
    ଯଦି (not io_module or not i_moduleName or not i_tagName or not i_tag) {
        ଫେରନ୍ତୁ m3Err_tagLookupFailed;
    }

    ଅଚଳ bool wildcardModule = (strcmp(i_moduleName, "*") == 0);
    IM3Tag     target         = i_tag->resolved ? i_tag->resolved : i_tag;

    M3Result result = m3Err_tagLookupFailed;

    ପାଇଁ (u32 i = 0; i < io_module->ଅଙ୍କ_Tags; ++i) {
        IM3Tag tag = &io_module->tags[i];

        ଯଦି (tag->imported and tag->import.moduleUtf8 and tag->import.fieldUtf8 and
            strcmp(tag->import.fieldUtf8, i_tagName) == 0 and
            (wildcardModule or strcmp(tag->import.moduleUtf8, i_moduleName) == 0)) {
            ଯଦି (tag->type != target->type) {
                ଫେରନ୍ତୁ m3Err_incompatibleImportType;
            }

            tag->resolved = target;
            result        = m3Err_none;
        }
    }

    ଫେରନ୍ତୁ result;
}


uint32_t m3_Get_Tag_Arg_ଗଣନା (IM3Tag i_tag)
{
    ଫେରନ୍ତୁ i_tag ? i_tag->type->ଅଙ୍କ_Args : 0;
}


M3_ମୂଲ୍ୟ_Type m3_GetTagArgType (IM3Tag i_tag, uint32_t i_index)
{
    ଯଦି (i_tag and i_index < i_tag->type->ଅଙ୍କ_Args) {
        ଫେରନ୍ତୁ (M3_ମୂଲ୍ୟ_Type)BaseTypeOf(d_FuncArgType(i_tag->type, i_index));
    }

    ଫେରନ୍ତୁ c_m3Type_none;
}

#ଅନ୍ୟଥା

IM3Tag m3_FindTag (IM3Module i_module, ଅଚଳ ଅକ୍ଷର* ଅଚଳ i_tagName)
{
    ଫେରନ୍ତୁ ଖାଲି;
}

M3Result m3_NewTag (IM3Runtime io_runtime, IM3Tag* o_tag, ଅଚଳ ଅକ୍ଷର* ଅଚଳ i_signature)
{
    ଯଦି (o_tag) {
        *o_tag = ଖାଲି;
    }
    ଫେରନ୍ତୁ m3Err_tagLookupFailed;
}

M3Result m3_LinkTag (IM3Module io_module, ଅଚଳ ଅକ୍ଷର* ଅଚଳ i_moduleName, ଅଚଳ ଅକ୍ଷର* ଅଚଳ i_tagName, IM3Tag i_tag)
{
    ଫେରନ୍ତୁ m3Err_tagLookupFailed;
}

uint32_t m3_Get_Tag_Arg_ଗଣନା (IM3Tag i_tag)
{
    ଫେରନ୍ତୁ 0;
}

M3_ମୂଲ୍ୟ_Type m3_GetTagArgType (IM3Tag i_tag, uint32_t i_index)
{
    ଫେରନ୍ତୁ c_m3Type_none;
}

#endif // d_m3HasExceptionHandling || d_m3HasStackSwitching


#ଯଦି d_m3HasExceptionHandling

M3Result m3_ThrowException (IM3Runtime io_runtime, IM3Tag i_tag, uint32_t i_argc, ଅଚଳ ଶୂନ୍ୟ* i_argptrs[])
{
    ଯଦି (not io_runtime or not i_tag) {
        ଫେରନ୍ତୁ m3Err_tagLookupFailed;
    }

    IM3Tag      tag  = i_tag->resolved ? i_tag->resolved : i_tag;
    IM3FuncType type = tag->type;

    // a tag with results is a stack switching control tag, which is suspended
    // with rather than thrown
    ଯଦି (type->ଅଙ୍କ_Rets) {
        ଫେରନ୍ତୁ m3Err_typeMismatch;
    }
    ଯଦି (i_argc != type->ଅଙ୍କ_Args) {
        ଫେରନ୍ତୁ m3_Err_argument_ଗଣନା_Mismatch;
    }
    // with nothing on the stack there is nothing to unwind, and the exception
    // would sit on the runtime's list until some later call ended
    ଯଦି (io_runtime->callNesting == 0) {
        ଫେରନ୍ତୁ "m3_ThrowException called outside a host function";
    }

    M3Exception* exception = NewException(io_runtime, tag, i_argc);
    ଯଦି (not exception) {
        ଫେରନ୍ତୁ m3Err_mallocFailed;
    }

    // the layout op_Throw gives a payload: 32-bit values zero-extended, floats
    // as their bit pattern
    ପାଇଁ (u32 i = 0; i < i_argc; ++i) {
        ଅଚଳ ଶୂନ୍ୟ* ମୂଲ୍ୟ = i_argptrs[i];

        ବାଛ (BaseTypeOf(d_FuncArgType(type, i))) {
        ସ୍ଥିତି c_m3Type_i32:
        ସ୍ଥିତି c_m3Type_f32: {
            u32 bits;
            memcpy(&bits, ମୂଲ୍ୟ, ଆକାର(bits));
            exception->args[i] = bits;
            ଭାଙ୍ଗ;
        }
        ସ୍ଥିତି c_m3Type_i64:
        ସ୍ଥିତି c_m3Type_f64:
            memcpy(&exception->args[i], ମୂଲ୍ୟ, ଆକାର(u64));
            ଭାଙ୍ଗ;
        ସ୍ଥିତି c_m3Type_funcref:
        ସ୍ଥିତି c_m3Type_externref:
        ସ୍ଥିତି c_m3Type_exnref:
        ସ୍ଥିତି c_m3Type_contref:
            exception->args[i] = (u64)(*(ଅଚଳ uintptr_t*)ମୂଲ୍ୟ);
            ଭାଙ୍ଗ;
        ପୂର୍ବନିର୍ଦ୍ଧାରିତ:
            FreeException(io_runtime, exception);
            ଫେରନ୍ତୁ "unknown argument type";
        }
    }

    io_runtime->pendingException = exception;

    ଫେରନ୍ତୁ m3Err_pendingException;
}


IM3Tag m3_GetExceptionTag (IM3Runtime i_runtime)
{
    ଯଦି (i_runtime and i_runtime->uncaughtException) {
        ଫେରନ୍ତୁ i_runtime->uncaughtException->tag;
    }

    ଫେରନ୍ତୁ ଖାଲି;
}


M3Result m3_GetExceptionArgs (IM3Runtime i_runtime, uint32_t i_argc, ଅଚଳ ଶୂନ୍ୟ* o_argptrs[])
{
    ଯଦି (not i_runtime or not i_runtime->uncaughtException) {
        ଫେରନ୍ତୁ "no uncaught exception";
    }

    ଅଚଳ M3Exception* exception = i_runtime->uncaughtException;
    IM3FuncType        type      = exception->tag->type;

    ଯଦି (i_argc != exception->ଅଙ୍କ_Args) {
        ଫେରନ୍ତୁ m3_Err_argument_ଗଣନା_Mismatch;
    }

    ପାଇଁ (u32 i = 0; i < i_argc; ++i) {
        ଶୂନ୍ୟ* ମୂଲ୍ୟ = (ଶୂନ୍ୟ*)o_argptrs[i];
        u64   bits  = exception->args[i];

        ବାଛ (BaseTypeOf(d_FuncArgType(type, i))) {
        ସ୍ଥିତି c_m3Type_i32:
        ସ୍ଥିତି c_m3Type_f32: {
            u32 ତଳ = (u32)bits;
            memcpy(ମୂଲ୍ୟ, &ତଳ, ଆକାର(ତଳ));
            ଭାଙ୍ଗ;
        }
        ସ୍ଥିତି c_m3Type_i64:
        ସ୍ଥିତି c_m3Type_f64:
            memcpy(ମୂଲ୍ୟ, &bits, ଆକାର(bits));
            ଭାଙ୍ଗ;
        ସ୍ଥିତି c_m3Type_funcref:
        ସ୍ଥିତି c_m3Type_externref:
        ସ୍ଥିତି c_m3Type_exnref:
        ସ୍ଥିତି c_m3Type_contref:
            *(uintptr_t*)ମୂଲ୍ୟ = (uintptr_t)bits;
            ଭାଙ୍ଗ;
        ପୂର୍ବନିର୍ଦ୍ଧାରିତ:
            ଫେରନ୍ତୁ "unknown argument type";
        }
    }

    ଫେରନ୍ତୁ m3Err_none;
}

#ଅନ୍ୟଥା

M3Result m3_ThrowException (IM3Runtime io_runtime, IM3Tag i_tag, uint32_t i_argc, ଅଚଳ ଶୂନ୍ୟ* i_argptrs[])
{
    ଫେରନ୍ତୁ m3Err_tagLookupFailed;
}

IM3Tag m3_GetExceptionTag (IM3Runtime i_runtime)
{
    ଫେରନ୍ତୁ ଖାଲି;
}

M3Result m3_GetExceptionArgs (IM3Runtime i_runtime, uint32_t i_argc, ଅଚଳ ଶୂନ୍ୟ* o_argptrs[])
{
    ଫେରନ୍ତୁ "no uncaught exception";
}

#endif // d_m3HasExceptionHandling


#ଯଦି d_m3GuardedMemory

// What RunCodeProtected hands to the platform layer to run, and what comes back
ପ୍ରକାରବ୍ୟାଖ୍ୟା ସଂରଚନା M3ProtectedRun {
    pc_t            pc;
    IM3Runtime      runtime;
    M3MemoryHeader* mem;
    M3Result        result;
} M3ProtectedRun;

ସ୍ଥିର
ଶୂନ୍ୟ RunCodeBody (ଶୂନ୍ୟ* io_context)
{
    M3ProtectedRun* run = (M3ProtectedRun*)io_context;

#  ଯଦି (d_m3EnableOpProfiling || d_m3EnableOpTracing)
    run->result = (M3Result)RunCode(run->pc, (m3stack_t)run->runtime->ଗଦା, run->mem,
                                    d_m3OpDefaultArgs, d_m3BaseCstr);
#  ଅନ୍ୟଥା
    run->result = (M3Result)RunCode(run->pc, (m3stack_t)run->runtime->ଗଦା, run->mem,
                                    d_m3OpDefaultArgs);
#  endif
}

// The bounds check the load and store operations no longer do, done once here by the
// system instead: an address past the end of a memory reaches the uncommitted part
// of that memory's reservation, and the fault comes back as the trap the spec asks
// for. Nothing is left half-done by it - the Wasm stack and the memories are heap,
// and the caller unwinds the rest.
ସ୍ଥିର
M3Result RunCodeProtected (IM3Runtime i_runtime, pc_t i_pc, M3MemoryHeader* i_mem)
{
    // Memory operations in this build carry no bounds check, so code only runs where a
    // fault past the end of a memory can still be turned into a trap
    ଯଦି (M3_UNLIKELY(not m3_HostGuardsActive())) {
        ଫେରନ୍ତୁ ErrorRuntime(m3Err_memoryGuardsUnavailable, i_runtime,
                            "another part of the process holds the fault signal");
    }

    M3ProtectedRun run;
    run.pc      = i_pc;
    run.runtime = i_runtime;
    run.mem     = i_mem;
    run.result  = m3Err_none;

    ଶୂନ୍ୟ*  arena;
    size_t arenaBytes;
    Guard_ArenaRange(&arena, &arenaBytes);

    ଯଦି (not m3_HostProtectedCall(RunCodeBody, &run, arena, arenaBytes)) {
        ଫେରନ୍ତୁ m3Err_trapOutOfBoundsMemoryAccess;
    }

    ଫେରନ୍ତୁ run.result;
}

#endif // d_m3GuardedMemory


#ଯଦି d_m3_Has_ଗଦା_Switching

// The function a suspended continuation stopped in: the callee of its innermost
// call frame, or the function it was entered with if it made no call. Frames are
// recorded innermost first, so that is the lowest-numbered call frame.
ସ୍ଥିର
IM3Function Continuation_InnermostFunction (IM3Continuation i_cont)
{
    ପାଇଁ (u32 i = 0; i < i_cont->ଅଙ୍କ_Frames; ++i) {
        ଯଦି (i_cont->frames[i].kind == frame_call && i_cont->frames[i].call.function) {
            ଫେରନ୍ତୁ i_cont->frames[i].call.function;
        }
    }

    ଫେରନ୍ତୁ i_cont->entryFunction;
}


// Where the host's next call puts its arguments and finds its results. A
// suspended root keeps its frames on the bottom of the runtime's stack, so a call
// made before it is resumed starts above the last of them rather than on top of
// them. The top is the innermost function's frame base plus everything that
// function can use. That base is the root's own sp, unless it stopped inside a
// resume: then the suspension point belongs to the resumed continuation, and the
// root's innermost frame is that resume.
ଶୂନ୍ୟ Runtime_Place_Call_ଗଦା (IM3Runtime io_runtime)
{
    IM3Continuation ମୂଳ = io_runtime->ମୂଳ_Continuation;

    io_runtime->ଗଦା = io_runtime->origin_ଗଦା;

    ଯଦି (not ମୂଳ or ମୂଳ->state != cont_suspended) {
        ଫେରନ୍ତୁ;
    }

    m3stack_t   sp       = ମୂଳ->sp;
    IM3Function function = Continuation_InnermostFunction(ମୂଳ);

    ଯଦି (ମୂଳ->ଅଙ୍କ_Frames and ମୂଳ->frames[0].kind == frame_resume) {
        sp = ମୂଳ->frames[0].sp;
    }

    m3slot_t* base = (m3slot_t*)io_runtime->origin_ଗଦା;
    m3slot_t* ଶୀର୍ଷ  = (m3slot_t*)sp + (function ? function->max_ଗଦା_Slots : 0);

    ଯଦି (ଶୀର୍ଷ > base + io_runtime->ଅଙ୍କ_ଗଦା_Slots) {
        ଶୀର୍ଷ = base + io_runtime->ଅଙ୍କ_ଗଦା_Slots;
    }

    io_runtime->ଗଦା = ଶୀର୍ଷ;
}

#endif // d_m3HasStackSwitching


// Run compiled code on the runtime's stack, bounding native recursion for the
// duration of the call. The outermost invocation establishes the stack limit;
// nested ones (an imported function calling back into Wasm) inherit it.
ସ୍ଥିର ଧାଡ଼ିଭିତରେ
M3Result RunCodeChecked (IM3Runtime i_runtime, IM3Function i_function)
{
    pc_t i_pc = i_function->compiled;

    // execution runs against the memory of the module the entry point belongs
    // to, not against some runtime-wide one
    M3MemoryHeader* _mem = Module_MemoryHeader(i_function->module);

#ଯଦି d_m3HasSnapshots
    ଯଦି (M3_UNLIKELY(i_function->module->isUnusable)) {
        ଫେରନ୍ତୁ "a snapshot failed to restore into the module, so it cannot run";
    }
    i_function->module->hasRun = true;
#endif

    d_m3_ଗଦା_Limit_Enter(i_runtime);

    // a host function calling back into Wasm does not change which call a trap
    // ends: that is still the one the host made first
    ଯଦି (i_runtime->callNesting++ == 0) {
        i_runtime->entered = i_function;
    }
#ଯଦି d_m3HasExceptionHandling
    // handler stacks don't nest across a call boundary: a host function calling
    // back into Wasm cannot be caught by a try_table its own caller entered
    u32 savedTryDepth   = i_runtime->tryDepth;
    i_runtime->tryDepth = 0;
#endif
#ଯଦି d_m3_Has_ଗଦା_Switching
    // nor do continuations: the native frames on the far side of a host call
    // are not ours to record, so a suspend in here has no handler to reach and
    // no resume to unwind to
    IM3Continuation savedContinuation = i_runtime->activeContinuation;

    // A suspended root continuation is a paused program waiting to be resumed,
    // not scratch space. A call the host makes before resuming it gets a
    // context of its own, the way any other re-entry does, rather than having
    // the frames of the suspension reset out from under it.
    bool claim_ମୂଳ = i_runtime->isSuspendable and not savedContinuation and
                     not (i_runtime->ମୂଳ_Continuation and
                          i_runtime->ମୂଳ_Continuation->state == cont_suspended);

    ଯଦି (claim_ମୂଳ and not i_runtime->ମୂଳ_Continuation) {
        i_runtime->ମୂଳ_Continuation = Continuation_New(i_runtime, i_function->funcType, i_function);

        ଯଦି (i_runtime->ମୂଳ_Continuation) {
            // The root borrows the runtime's stack and is not charged as cont.new.
            i_runtime->ମୂଳ_Continuation->ଅଙ୍କ_ଗଦା_Slots = i_runtime->ଅଙ୍କ_ଗଦା_Slots;
        } ଅନ୍ୟଥା {
            claim_ମୂଳ = false;
        }
    }

    ଯଦି (claim_ମୂଳ) {
        i_runtime->activeContinuation              = i_runtime->ମୂଳ_Continuation;
        i_runtime->ମୂଳ_Continuation->entryFunction = i_function;
        i_runtime->ମୂଳ_Continuation->state         = cont_running;
        i_runtime->ମୂଳ_Continuation->ଅଙ୍କ_Frames     = 0;
    } ଅନ୍ୟଥା {
        i_runtime->activeContinuation = ଖାଲି;
    }
#endif
#ଯଦି d_m3GuardedMemory
    M3Result result = RunCodeProtected(i_runtime, i_pc, _mem);
#ନଚେତଯଦି (d_m3EnableOpProfiling || d_m3EnableOpTracing)
    M3Result result = (M3Result)RunCode(i_pc, (m3stack_t)i_runtime->ଗଦା, _mem, d_m3OpDefaultArgs, d_m3BaseCstr);
#ଅନ୍ୟଥା
    M3Result result = (M3Result)RunCode(i_pc, (m3stack_t)i_runtime->ଗଦା, _mem, d_m3OpDefaultArgs);
#endif
#ଯଦି d_m3_Has_ଗଦା_Switching
    ଯଦି (claim_ମୂଳ) {
        i_runtime->ମୂଳ_Continuation->state =
          (result == m3Err_continuationSuspended) ? cont_suspended : cont_returned;

        Runtime_Place_Call_ଗଦା(i_runtime);
    }
    i_runtime->activeContinuation = savedContinuation;
#endif
#ଯଦି d_m3HasExceptionHandling
    i_runtime->tryDepth = savedTryDepth;

    // an exception that reached the bottom of the call stack found no handler
    ଯଦି (M3_UNLIKELY(result == m3Err_pendingException)) {
        result = m3Err_trapUncaughtException;
    }
    RecordUncaughtException(i_runtime, result);

    // a paused invocation can still reach the exceptions it holds, so they wait
    // for it to finish
    ଯଦି (i_runtime->callNesting == 1 and not m3_IsSuspended(i_runtime)) {
        FreeExceptions(i_runtime);
    }
#endif
    i_runtime->callNesting--;
    d_m3_ଗଦା_Limit_Leave(i_runtime);

    ଫେରନ୍ତୁ result;
}

M3Result m3_RunStart (IM3Module io_module)
{
#ifdef FUZZING_BUILD_MODE_UNSAFE_FOR_PRODUCTION
    // Execution disabled for fuzzing builds
    ଫେରନ୍ତୁ m3Err_none;
#endif

    M3Result result           = m3Err_none;
    i32      startFunctionTmp = -1;

    ଯଦି (io_module and io_module->startFunction >= 0) {
        IM3Function function = &io_module->functions[io_module->startFunction];

        ଯଦି (not function->compiled) {
_           (CompileFunction(function));
        }

        IM3FuncType ftype = function->funcType;
        ଯଦି (ftype->ଅଙ୍କ_Args != 0 || ftype->ଅଙ୍କ_Rets != 0) {
            _throw(m3_Err_argument_ଗଣନା_Mismatch);
        }

        IM3Module  module  = function->module;
        IM3Runtime runtime = module->runtime;

        startFunctionTmp = io_module->startFunction;

        io_module->startFunction = -1;

        result = RunCodeChecked(runtime, function);

        // a start function that suspended is not to be started again: resuming
        // is what finishes it
        ଯଦି (result and result != m3Err_continuationSuspended) {
            io_module->startFunction = startFunctionTmp;
            EXCEPTION_PRINT(result);
            ଯାଅ _catch;
        }
    }

    _catch: ଫେରନ୍ତୁ result;
}

// TODO: deal with main + side-modules loading efforcement
M3Result m3_LoadModule (IM3Runtime io_runtime, IM3Module io_module)
{
    M3Result result = m3Err_none;

    ଯଦି (M3_UNLIKELY(io_module->runtime)) {
        ଫେରନ୍ତୁ m3Err_moduleAlreadyLinked;
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
    ଫେରନ୍ତୁ result; // ok

_catch:
    // The runtime owns the module either way. Instantiation may already have
    // written this module's functions into a table another module owns, and the
    // spec keeps whatever it managed to do before the trap, so those entries
    // stay callable - retaining the module is what stops them dangling.
    //
    // It goes on the tail of the list rather than the head: it is not a module
    // anyone should find by name, only one whose functions may still be
    // reachable through someone else's table.
    io_module->next = ଖାଲି;

    IM3Module* tail = &io_runtime->modules;
    ଯେତେବେଳେ (*tail) {
        tail = &(*tail)->next;
    }
    *tail = io_module;

    ଫେରନ୍ତୁ result;
}

IM3Global m3_FindGlobal (IM3Module         io_module,
                         ଅଚଳ ଅକ୍ଷର* ଅଚଳ i_globalName)
{
    // Search exports
    IM3Global exported = Module_FindExportedGlobal(io_module, i_globalName);
    ଯଦି (exported) {
        // a re-exported global is the one that was imported, so reads and
        // writes have to reach the cell that actually holds the value
        ଫେରନ୍ତୁ exported->resolved ? exported->resolved : exported;
    }

    // Search imports
    ପାଇଁ (u32 i = 0; i < io_module->ଅଙ୍କ_Globals; ++i) {
        IM3Global g = &io_module->globals[i];

        ଯଦି (g->import.moduleUtf8 and g->import.fieldUtf8) {
            ଯଦି (strcmp(g->import.fieldUtf8, i_globalName) == 0) {
                ଫେରନ୍ତୁ g->resolved ? g->resolved : g;
            }
        }
    }
    ଫେରନ୍ତୁ ଖାଲି;
}

M3Result m3_GetGlobal (IM3Global      i_global,
                       IM3_Tagged_ମୂଲ୍ୟ o_ମୂଲ୍ୟ)
{
    ଯଦି (not i_global) {
        ଫେରନ୍ତୁ m3Err_globalLookupFailed;
    }

    ବାଛ (i_global->type) {
    ସ୍ଥିତି c_m3Type_i32: o_ମୂଲ୍ୟ->ମୂଲ୍ୟ.i32 = i_global->i32_ମୂଲ୍ୟ; ଭାଙ୍ଗ;
    ସ୍ଥିତି c_m3Type_i64: o_ମୂଲ୍ୟ->ମୂଲ୍ୟ.i64 = i_global->i64_ମୂଲ୍ୟ; ଭାଙ୍ଗ;
#ଯଦି d_m3HasFloat
    ସ୍ଥିତି c_m3Type_f32: o_ମୂଲ୍ୟ->ମୂଲ୍ୟ.f32 = i_global->f32_ମୂଲ୍ୟ; ଭାଙ୍ଗ;
    ସ୍ଥିତି c_m3Type_f64: o_ମୂଲ୍ୟ->ମୂଲ୍ୟ.f64 = i_global->f64_ମୂଲ୍ୟ; ଭାଙ୍ଗ;
#endif
    ପୂର୍ବନିର୍ଦ୍ଧାରିତ: ଫେରନ୍ତୁ m3Err_invalidTypeId;
    }

    o_ମୂଲ୍ୟ->type = (M3_ମୂଲ୍ୟ_Type)(i_global->type);
    ଫେରନ୍ତୁ m3Err_none;
}

M3Result m3_SetGlobal (IM3Global            i_global,
                       ଅଚଳ IM3_Tagged_ମୂଲ୍ୟ i_ମୂଲ୍ୟ)
{
    ଯଦି (not i_global) {
        ଫେରନ୍ତୁ m3Err_globalLookupFailed;
    }
    ଯଦି (not i_global->isMutable) {
        ଫେରନ୍ତୁ m3Err_globalNotMutable;
    }
    ଯଦି (i_global->type != i_ମୂଲ୍ୟ->type) {
        ଫେରନ୍ତୁ m3Err_globalTypeMismatch;
    }

    ବାଛ (i_ମୂଲ୍ୟ->type) {
    ସ୍ଥିତି c_m3Type_i32: i_global->i32_ମୂଲ୍ୟ = i_ମୂଲ୍ୟ->ମୂଲ୍ୟ.i32; ଭାଙ୍ଗ;
    ସ୍ଥିତି c_m3Type_i64: i_global->i64_ମୂଲ୍ୟ = i_ମୂଲ୍ୟ->ମୂଲ୍ୟ.i64; ଭାଙ୍ଗ;
#ଯଦି d_m3HasFloat
    ସ୍ଥିତି c_m3Type_f32: i_global->f32_ମୂଲ୍ୟ = i_ମୂଲ୍ୟ->ମୂଲ୍ୟ.f32; ଭାଙ୍ଗ;
    ସ୍ଥିତି c_m3Type_f64: i_global->f64_ମୂଲ୍ୟ = i_ମୂଲ୍ୟ->ମୂଲ୍ୟ.f64; ଭାଙ୍ଗ;
#endif
    ପୂର୍ବନିର୍ଦ୍ଧାରିତ: ଫେରନ୍ତୁ m3Err_invalidTypeId;
    }

    ଫେରନ୍ତୁ m3Err_none;
}

M3_ମୂଲ୍ୟ_Type m3_GetGlobalType (IM3Global i_global)
{
    ଫେରନ୍ତୁ (i_global) ? (M3_ମୂଲ୍ୟ_Type)(i_global->type) : c_m3Type_none;
}


ଶୂନ୍ୟ* v_FindFunction (IM3Module i_module, ଶୂନ୍ୟ* i_info)
{
    ଅଚଳ ଅକ୍ଷର* ଅଚଳ i_name = (ଅଚଳ ଅକ୍ଷର*)i_info;

    // Prefer exported functions
    IM3Function exported = Module_FindExportedFunction(i_module, i_name);
    ଯଦି (exported) {
        ଫେରନ୍ତୁ exported;
    }

    // Search internal functions
    ପାଇଁ (u32 i = 0; i < i_module->ଅଙ୍କ_Functions; ++i) {
        IM3Function f = &i_module->functions[i];

        bool isImported = f->import.moduleUtf8 or f->import.fieldUtf8;

        ଯଦି (isImported) {
            ଜାରି;
        }

        ପାଇଁ (ପୂର୍ଣ୍ଣସଂଖ୍ୟା j = 0; j < f->ଅଙ୍କ_Names; j++) {
            ଯଦି (f->names[j] and strcmp(f->names[j], i_name) == 0) {
                ଫେରନ୍ତୁ f;
            }
        }
    }

    ଫେରନ୍ତୁ ଖାଲି;
}


// Shared tail of the two lookups: a function is only usable once it has code.
ସ୍ଥିର
M3Result PrepareFoundFunction (IM3Function* o_function, IM3Function i_function)
{
    M3Result result = m3Err_none;

    ଯଦି (not i_function->compiled) {
_       (CompileFunction(i_function))
    }

    _catch:
    *o_function = result ? ଖାଲି : i_function;

    ଫେରନ୍ତୁ result;
}


// Searches every module in the runtime, most recently loaded first. That is a
// guess once more than one module is loaded and two of them export the same
// name - m3_FindFunctionIn says which module is meant.
M3Result m3_FindFunction (IM3Function* o_function, IM3Runtime i_runtime, ଅଚଳ ଅକ୍ଷର* ଅଚଳ i_functionName)
{
                                                                d_m3Assert (o_function and i_runtime and i_functionName);
    IM3Function function = ଖାଲି;

    ଯଦି (not i_runtime->modules) {
        *o_function = ଖାଲି;
        ଫେରନ୍ତୁ "no modules loaded";
    }

    function = (IM3Function)ForEachModule(i_runtime, v_FindFunction, (ଶୂନ୍ୟ*)i_functionName);

    ଯଦି (not function) {
        *o_function = ଖାଲି;
        ଫେରନ୍ତୁ ErrorModule(m3Err_functionLookupFailed, i_runtime->modules, "'%s'", i_functionName);
    }

    ଫେରନ୍ତୁ PrepareFoundFunction(o_function, function);
}


IM3Module m3_FindModule (IM3Runtime i_runtime, ଅଚଳ ଅକ୍ଷର* ଅଚଳ i_moduleName)
{
    ଯଦି (not i_runtime or not i_moduleName) {
        ଫେରନ୍ତୁ ଖାଲି;
    }

    // the list is newest-first, so a name registered twice names the newer one
    ପାଇଁ (IM3Module m = i_runtime->modules; m; m = m->next) {
        ଯଦି (m->name and strcmp(m->name, i_moduleName) == 0) {
            ଫେରନ୍ତୁ m;
        }
    }

    ଫେରନ୍ତୁ ଖାଲି;
}


// Searches one module's exports, which is what naming a module means.
M3Result m3_FindFunctionIn (IM3Function* o_function, IM3Module i_module, ଅଚଳ ଅକ୍ଷର* ଅଚଳ i_functionName)
{
                                                                d_m3Assert (o_function and i_functionName);
    ଯଦି (not i_module) {
        *o_function = ଖାଲି;
        ଫେରନ୍ତୁ m3Err_functionLookupFailed;      // ErrorModule would deref it
    }

    IM3Function function = (IM3Function)v_FindFunction(i_module, (ଶୂନ୍ୟ*)i_functionName);

    ଯଦି (not function) {
        *o_function = ଖାଲି;
        ଫେରନ୍ତୁ ErrorModule(m3Err_functionLookupFailed, i_module, "'%s'", i_functionName);
    }

    ଫେରନ୍ତୁ PrepareFoundFunction(o_function, function);
}


M3Result m3_GetTableFunction (IM3Function* o_function, IM3Module i_module, uint32_t i_index)
{
_try {
    M3Table*    table;
    IM3Function function;

    _throwif("no table", i_module->ଅଙ୍କ_Tables == 0);

    table = i_module->tables[0];
    _throwif("function index out of range", i_index >= table->size);

    function = (IM3Function)table->elements[i_index];

    ଯଦି (function) {
        ଯଦି (not function->compiled) {
_           (CompileFunction(function))
        }
    }

    *o_function = function;
} _catch:
    ଫେରନ୍ତୁ result;
}


ସ୍ଥିର
M3Result checkStartFunction (IM3Module i_module)
{
    M3Result result = m3Err_none;                               d_m3Assert(i_module);

    // Check if start function needs to be called
    ଯଦି (i_module->startFunction >= 0) {
        result = m3_RunStart(i_module);
    }

    ଫେରନ୍ତୁ result;
}

uint32_t m3_Get_Arg_ଗଣନା (IM3Function i_function)
{
    ଯଦି (i_function) {
        IM3FuncType ft = i_function->funcType;
        ଯଦି (ft) {
            ଫେରନ୍ତୁ ft->ଅଙ୍କ_Args;
        }
    }
    ଫେରନ୍ତୁ 0;
}

uint32_t m3_Get_Ret_ଗଣନା (IM3Function i_function)
{
    ଯଦି (i_function) {
        IM3FuncType ft = i_function->funcType;
        ଯଦି (ft) {
            ଫେରନ୍ତୁ ft->ଅଙ୍କ_Rets;
        }
    }
    ଫେରନ୍ତୁ 0;
}


M3_ମୂଲ୍ୟ_Type m3_GetArgType (IM3Function i_function, uint32_t index)
{
    ଯଦି (i_function) {
        IM3FuncType ft = i_function->funcType;
        ଯଦି (ft and index < ft->ଅଙ୍କ_Args) {
            ଫେରନ୍ତୁ (M3_ମୂଲ୍ୟ_Type)BaseTypeOf(d_FuncArgType(ft, index));
        }
    }
    ଫେରନ୍ତୁ c_m3Type_none;
}

M3_ମୂଲ୍ୟ_Type m3_GetRetType (IM3Function i_function, uint32_t index)
{
    ଯଦି (i_function) {
        IM3FuncType ft = i_function->funcType;
        ଯଦି (ft and index < ft->ଅଙ୍କ_Rets) {
            ଫେରନ୍ତୁ (M3_ମୂଲ୍ୟ_Type)BaseTypeOf(d_FuncRetType(ft, index));
        }
    }
    ଫେରନ୍ତୁ c_m3Type_none;
}


// The call APIs marshal their arguments into the runtime stack before any
// compiled code runs, so op_Entry's overflow check comes too late to cover those
// writes - and for a host function or an import linked to another module there
// is no op_Entry of our own to reach at all. Bound the area about to be written
// here instead: results and arguments each take a u64 at this boundary, however
// wide the slot is inside. Every caller goes through this, so none of them can
// be the one that forgets.
ସ୍ଥିର
M3Result Get_ଗଦା_Pointer_For_Args (IM3Function i_function, u8** o_ଗଦା)
{
    IM3Runtime  runtime = i_function->module->runtime;
    IM3FuncType ftype   = i_function->funcType;

    // the stack a paused invocation holds is not available to this one
    size_t used      = (size_t)((u8*)runtime->ଗଦା - (u8*)runtime->origin_ଗଦା);
    size_t needed    = ((size_t)ftype->ଅଙ୍କ_Rets + ftype->ଅଙ୍କ_Args) * ଆକାର(u64);
    size_t available = (size_t)runtime->ଅଙ୍କ_ଗଦା_Slots * ଆକାର(m3slot_t) - used;

    ଯଦି (needed > available) {
        ଫେରନ୍ତୁ m3_Err_trap_ଗଦା_Overflow;
    }

    u64* ଗଦା = (u64*)runtime->ଗଦା;

    ଗଦା += ftype->ଅଙ୍କ_Rets;

    *o_ଗଦା = (u8*)ଗଦା;

    ଫେରନ୍ତୁ m3Err_none;
}


M3Result m3_CallV (IM3Function i_function, ...)
{
    va_list ap;
    va_start(ap, i_function);
    M3Result r = m3_CallVL(i_function, ap);
    va_end(ap);
    ଫେରନ୍ତୁ r;
}

ସ୍ଥିର
ଶୂନ୍ୟ Report_Native_ଗଦା_Usage ()
{
#ଯଦି d_m3_Log_Native_ଗଦା
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା ଗଦା_Used = m3_ଗଦା_Get_Max();
    fprintf(stderr, "Native stack used: %d\n", ଗଦା_Used);
#endif
}


M3Result m3_CallVL (IM3Function i_function, va_list i_args)
{
    IM3Runtime  runtime = i_function->module->runtime;
    IM3FuncType ftype   = i_function->funcType;
    M3Result    result  = m3Err_none;
    u8*         s       = ଖାଲି;

    ଯଦି (!i_function->compiled) {
        ଫେରନ୍ତୁ m3Err_missingCompiledCode;
    }

#ଯଦି d_m3RecordBacktraces
    ClearBacktrace(runtime);
#endif

    m3_ଗଦା_Check_Init();

_   (checkStartFunction(i_function->module))

_   (Get_ଗଦା_Pointer_For_Args(i_function, &s))

    ପାଇଁ (u32 i = 0; i < ftype->ଅଙ୍କ_Args; ++i) {
        ବାଛ (d_FuncArgType(ftype, i)) {
        ସ୍ଥିତି c_m3Type_i32:
            *(i32*)(s) = va_arg(i_args, i32);
            s += 8;
            ଭାଙ୍ଗ;
        ସ୍ଥିତି c_m3Type_i64:
            *(i64*)(s) = va_arg(i_args, i64);
            s += 8;
            ଭାଙ୍ଗ;
        ସ୍ଥିତି c_m3Type_funcref:
        ସ୍ଥିତି c_m3Type_externref:
        ସ୍ଥିତି c_m3Type_exnref:
            *(uintptr_t*)(s) = va_arg(i_args, uintptr_t);
            s += 8;
            ଭାଙ୍ଗ;
#ଯଦି d_m3HasFloat
        ସ୍ଥିତି c_m3Type_f32:
            *(f32*)(s) = (f32)va_arg(i_args, f64);
            s += 8;
            ଭାଙ୍ଗ; // f32 is passed as f64
        ସ୍ଥିତି c_m3Type_f64:
            *(f64*)(s) = va_arg(i_args, f64);
            s += 8;
            ଭାଙ୍ଗ;
#endif
        ପୂର୍ବନିର୍ଦ୍ଧାରିତ: ଫେରନ୍ତୁ "unknown argument type";
        }
    }

    result = RunCodeChecked(runtime, i_function);
    Report_Native_ଗଦା_Usage();

    runtime->lastCalled = result ? ଖାଲି : i_function;

    _catch: ଫେରନ୍ତୁ result;
}

M3Result m3_Call (IM3Function i_function, uint32_t i_argc, ଅଚଳ ଶୂନ୍ୟ* i_argptrs[])
{
    IM3Runtime  runtime = i_function->module->runtime;
    IM3FuncType ftype   = i_function->funcType;
    M3Result    result  = m3Err_none;
    u8*         s       = ଖାଲି;

    ଯଦି (i_argc != ftype->ଅଙ୍କ_Args) {
        ଫେରନ୍ତୁ m3_Err_argument_ଗଣନା_Mismatch;
    }
    ଯଦି (!i_function->compiled) {
        ଫେରନ୍ତୁ m3Err_missingCompiledCode;
    }

#ଯଦି d_m3RecordBacktraces
    ClearBacktrace(runtime);
#endif

    m3_ଗଦା_Check_Init();

_   (checkStartFunction(i_function->module))

_   (Get_ଗଦା_Pointer_For_Args(i_function, &s))

    ପାଇଁ (u32 i = 0; i < ftype->ଅଙ୍କ_Args; ++i) {
        ବାଛ (d_FuncArgType(ftype, i)) {
        ସ୍ଥିତି c_m3Type_i32:
            *(i32*)(s) = *(i32*)i_argptrs[i];
            s += 8;
            ଭାଙ୍ଗ;
        ସ୍ଥିତି c_m3Type_i64:
            *(i64*)(s) = *(i64*)i_argptrs[i];
            s += 8;
            ଭାଙ୍ଗ;
        ସ୍ଥିତି c_m3Type_funcref:
        ସ୍ଥିତି c_m3Type_externref:
        ସ୍ଥିତି c_m3Type_exnref:
            *(uintptr_t*)(s) = *(uintptr_t*)i_argptrs[i];
            s += 8;
            ଭାଙ୍ଗ;
#ଯଦି d_m3HasFloat
        ସ୍ଥିତି c_m3Type_f32:
            *(f32*)(s) = *(f32*)i_argptrs[i];
            s += 8;
            ଭାଙ୍ଗ;
        ସ୍ଥିତି c_m3Type_f64:
            *(f64*)(s) = *(f64*)i_argptrs[i];
            s += 8;
            ଭାଙ୍ଗ;
#endif
        ପୂର୍ବନିର୍ଦ୍ଧାରିତ: ଫେରନ୍ତୁ "unknown argument type";
        }
    }

    result = RunCodeChecked(runtime, i_function);

    Report_Native_ଗଦା_Usage();

    runtime->lastCalled = result ? ଖାଲି : i_function;

    _catch: ଫେରନ୍ତୁ result;
}

// Argument parsing for m3_CallArgv. Strict on purpose: the whole string has to
// be consumed, so "12abc" is rejected rather than read as 12, and an empty or
// unparsable argument is an error rather than the zero that strtoul with a
// NULL end pointer used to hand back. See wasm3/wasm3#367.
ସ୍ଥିର
M3Result ParseArgInteger (ccstr_t i_arg, u32 i_ଅଙ୍କ_Bits, u64* o_ମୂଲ୍ୟ)
{
    ଯଦି (not i_arg or not *i_arg) {
        ଫେରନ୍ତୁ "empty argument";
    }

    // strtoull would skip leading space, but trailing space is rejected below;
    // accepting one and not the other would just be confusing
    ଯଦି (isspace((unsigned ଅକ୍ଷର)*i_arg)) {
        ଫେରନ୍ତୁ "argument is not a number";
    }

    ଅକ୍ଷର* end = ଖାଲି;
    u64   ମୂଲ୍ୟ;

    errno = 0;

    // an argument may be spelled signed or unsigned: -1 and 4294967295 name the
    // same i32
    ଯଦି (*i_arg == '-') {
        i64 signed_ମୂଲ୍ୟ = strtoll(i_arg, &end, 10);

        ଯଦି (i_ଅଙ୍କ_Bits == 32 and (signed_ମୂଲ୍ୟ < INT32_MIN or signed_ମୂଲ୍ୟ > INT32_MAX)) {
            ଫେରନ୍ତୁ "argument out of range";
        }

        ମୂଲ୍ୟ = (u64)signed_ମୂଲ୍ୟ;
    } ଅନ୍ୟଥା {
        ମୂଲ୍ୟ = strtoull(i_arg, &end, 10);

        ଯଦି (i_ଅଙ୍କ_Bits == 32 and ମୂଲ୍ୟ > UINT32_MAX) {
            ଫେରନ୍ତୁ "argument out of range";
        }
    }

    ଯଦି (errno == ERANGE) {
        ଫେରନ୍ତୁ "argument out of range";
    }

    ଯଦି (end == i_arg or *end) {
        ଫେରନ୍ତୁ "argument is not a number";
    }

    *o_ମୂଲ୍ୟ = ମୂଲ୍ୟ;

    ଫେରନ୍ତୁ m3Err_none;
}


#ଯଦି d_m3HasFloat
ସ୍ଥିର
M3Result ParseArgFloat (ccstr_t i_arg, f64* o_ମୂଲ୍ୟ)
{
    ଯଦି (not i_arg or not *i_arg) {
        ଫେରନ୍ତୁ "empty argument";
    }

    ଯଦି (isspace((unsigned ଅକ୍ଷର)*i_arg)) {
        ଫେରନ୍ତୁ "argument is not a number";
    }

    ଅକ୍ଷର* end = ଖାଲି;

    errno = 0;

    f64 ମୂଲ୍ୟ = strtod(i_arg, &end);

    ଯଦି (end == i_arg or *end) {
        ଫେରନ୍ତୁ "argument is not a number";
    }

    // strtod reports underflow through ERANGE as well, and a denormal result is
    // perfectly usable, so only an overflow to infinity is out of range
    ଯଦି (errno == ERANGE and (ମୂଲ୍ୟ > DBL_MAX or ମୂଲ୍ୟ < -DBL_MAX)) {
        ଫେରନ୍ତୁ "argument out of range";
    }

    *o_ମୂଲ୍ୟ = ମୂଲ୍ୟ;

    ଫେରନ୍ତୁ m3Err_none;
}
#endif


// A reference argument is either the null reference or a host handle written as
// an integer.
ସ୍ଥିର
M3Result ParseArgReference (ccstr_t i_arg, u64* o_ମୂଲ୍ୟ)
{
    ଯଦି (i_arg and strcmp(i_arg, "null") == 0) {
        *o_ମୂଲ୍ୟ = 0;
        ଫେରନ୍ତୁ m3Err_none;
    }

    ଫେରନ୍ତୁ ParseArgInteger(i_arg, 64, o_ମୂଲ୍ୟ);
}


M3Result m3_CallArgv (IM3Function i_function, uint32_t i_argc, ଅଚଳ ଅକ୍ଷର* i_argv[])
{
    IM3FuncType ftype   = i_function->funcType;
    IM3Runtime  runtime = i_function->module->runtime;
    M3Result    result  = m3Err_none;
    u8*         s       = ଖାଲି;

    ଯଦି (i_argc != ftype->ଅଙ୍କ_Args) {
        ଫେରନ୍ତୁ m3_Err_argument_ଗଣନା_Mismatch;
    }
    ଯଦି (!i_function->compiled) {
        ଫେରନ୍ତୁ m3Err_missingCompiledCode;
    }

#ଯଦି d_m3RecordBacktraces
    ClearBacktrace(runtime);
#endif

    m3_ଗଦା_Check_Init();

_   (checkStartFunction(i_function->module))

_   (Get_ଗଦା_Pointer_For_Args(i_function, &s))

    ପାଇଁ (u32 i = 0; i < ftype->ଅଙ୍କ_Args; ++i) {
        u64 ମୂଲ୍ୟ = 0;
#ଯଦି d_m3HasFloat
        f64 fvalue = 0;
#endif
        ବାଛ (d_FuncArgType(ftype, i)) {
        ସ୍ଥିତି c_m3Type_i32:
_           (ParseArgInteger(i_argv[i], 32, &ମୂଲ୍ୟ)) * (i32*)(s) = (i32)ମୂଲ୍ୟ;
            s += 8;
            ଭାଙ୍ଗ;
        ସ୍ଥିତି c_m3Type_i64:
_           (ParseArgInteger(i_argv[i], 64, &ମୂଲ୍ୟ)) * (i64*)(s) = (i64)ମୂଲ୍ୟ;
            s += 8;
            ଭାଙ୍ଗ;
        ସ୍ଥିତି c_m3Type_funcref:
        ସ୍ଥିତି c_m3Type_externref:
        ସ୍ଥିତି c_m3Type_exnref:
_           (ParseArgReference(i_argv[i], &ମୂଲ୍ୟ)) * (uintptr_t*)(s) = (uintptr_t)ମୂଲ୍ୟ;
            s += 8;
            ଭାଙ୍ଗ;
#ଯଦି d_m3HasFloat
            // strtof would be less portable
        ସ୍ଥିତି c_m3Type_f32:
_           (ParseArgFloat(i_argv[i], &fvalue)) * (f32*)(s) = (f32)fvalue;
            s += 8;
            ଭାଙ୍ଗ;
        ସ୍ଥିତି c_m3Type_f64:
_           (ParseArgFloat(i_argv[i], &fvalue)) * (f64*)(s) = fvalue;
            s += 8;
            ଭାଙ୍ଗ;
#endif
        ପୂର୍ବନିର୍ଦ୍ଧାରିତ: _throw("unknown argument type");
        }
    }

    result = RunCodeChecked(runtime, i_function);

    Report_Native_ଗଦା_Usage();

    runtime->lastCalled = result ? ଖାଲି : i_function;

    _catch: ଫେରନ୍ତୁ result;
}


//u8 * AlignStackPointerTo64Bits (const u8 * i_stack)
//{
//    uintptr_t ptr = (uintptr_t) i_stack;
//    return (u8 *) ((ptr + 7) & ~7);
//}


M3Result m3_GetResults (IM3Function i_function, uint32_t i_retc, ଅଚଳ ଶୂନ୍ୟ* o_retptrs[])
{
    IM3FuncType ftype   = i_function->funcType;
    IM3Runtime  runtime = i_function->module->runtime;

    ଯଦି (i_retc != ftype->ଅଙ୍କ_Rets) {
        ଫେରନ୍ତୁ m3_Err_argument_ଗଣନା_Mismatch;
    }
    ଯଦି (i_function != runtime->lastCalled) {
        ଫେରନ୍ତୁ "function not called";
    }

    u8* s = (u8*)runtime->ଗଦା;

    ପାଇଁ (u32 i = 0; i < ftype->ଅଙ୍କ_Rets; ++i) {
        ବାଛ (d_FuncRetType(ftype, i)) {
        ସ୍ଥିତି c_m3Type_i32:
            *(i32*)o_retptrs[i] = *(i32*)(s);
            s += 8;
            ଭାଙ୍ଗ;
        ସ୍ଥିତି c_m3Type_i64:
            *(i64*)o_retptrs[i] = *(i64*)(s);
            s += 8;
            ଭାଙ୍ଗ;
        ସ୍ଥିତି c_m3Type_funcref:
        ସ୍ଥିତି c_m3Type_externref:
        ସ୍ଥିତି c_m3Type_exnref:
            *(uintptr_t*)o_retptrs[i] = *(uintptr_t*)(s);
            s += 8;
            ଭାଙ୍ଗ;
#ଯଦି d_m3HasFloat
        ସ୍ଥିତି c_m3Type_f32:
            *(f32*)o_retptrs[i] = *(f32*)(s);
            s += 8;
            ଭାଙ୍ଗ;
        ସ୍ଥିତି c_m3Type_f64:
            *(f64*)o_retptrs[i] = *(f64*)(s);
            s += 8;
            ଭାଙ୍ଗ;
#endif
        ପୂର୍ବନିର୍ଦ୍ଧାରିତ: ଫେରନ୍ତୁ "unknown return type";
        }
    }
    ଫେରନ୍ତୁ m3Err_none;
}

M3Result m3_GetResultsV (IM3Function i_function, ...)
{
    va_list ap;
    va_start(ap, i_function);
    M3Result r = m3_GetResultsVL(i_function, ap);
    va_end(ap);
    ଫେରନ୍ତୁ r;
}

M3Result m3_GetResultsVL (IM3Function i_function, va_list o_rets)
{
    IM3Runtime  runtime = i_function->module->runtime;
    IM3FuncType ftype   = i_function->funcType;

    ଯଦି (i_function != runtime->lastCalled) {
        ଫେରନ୍ତୁ "function not called";
    }

    u8* s = (u8*)runtime->ଗଦା;
    ପାଇଁ (u32 i = 0; i < ftype->ଅଙ୍କ_Rets; ++i) {
        ବାଛ (d_FuncRetType(ftype, i)) {
        ସ୍ଥିତି c_m3Type_i32:
            *va_arg(o_rets, i32*) = *(i32*)(s);
            s += 8;
            ଭାଙ୍ଗ;
        ସ୍ଥିତି c_m3Type_i64:
            *va_arg(o_rets, i64*) = *(i64*)(s);
            s += 8;
            ଭାଙ୍ଗ;
        ସ୍ଥିତି c_m3Type_funcref:
        ସ୍ଥିତି c_m3Type_externref:
        ସ୍ଥିତି c_m3Type_exnref:
            *va_arg(o_rets, uintptr_t*) = *(uintptr_t*)(s);
            s += 8;
            ଭାଙ୍ଗ;
#ଯଦି d_m3HasFloat
        ସ୍ଥିତି c_m3Type_f32:
            *va_arg(o_rets, f32*) = *(f32*)(s);
            s += 8;
            ଭାଙ୍ଗ;
        ସ୍ଥିତି c_m3Type_f64:
            *va_arg(o_rets, f64*) = *(f64*)(s);
            s += 8;
            ଭାଙ୍ଗ;
#endif
        ପୂର୍ବନିର୍ଦ୍ଧାରିତ: ଫେରନ୍ତୁ "unknown argument type";
        }
    }
    ଫେରନ୍ତୁ m3Err_none;
}

ଶୂନ୍ୟ ReleaseCodePageNoTrack (IM3Runtime i_runtime, IM3CodePage i_codePage)
{
    ଯଦି (i_codePage) {
        IM3CodePage* list;

        bool pageFull = (ଅଙ୍କ_Free_ଧାଡ଼ିଗୁଡ଼ିକ(i_codePage) < d_m3_Code_Page_Free_ଧାଡ଼ିଗୁଡ଼ିକ_Threshold);
        ଯଦି (pageFull) {
            list = &i_runtime->pagesFull;
        } ଅନ୍ୟଥା {
            list = &i_runtime->pagesOpen;
        }

        ଠେଲ_Code_Page(list, i_codePage);                         m3log (emit, "release page: %d to queue: '%s'", i_codePage->info.sequence, pageFull ? "full" : "open")
    }
}


IM3CodePage AcquireCodePageWithCapacity (IM3Runtime i_runtime, u32 i_min_ଧାଡ଼ି_ଗଣନା)
{
    IM3CodePage page = RemoveCodePageOfCapacity(&i_runtime->pagesOpen, i_min_ଧାଡ଼ି_ଗଣନା);

    ଯଦି (not page) {
        page = Environment_AcquireCodePage(i_runtime->environment, i_min_ଧାଡ଼ି_ଗଣନା);

        ଯଦି (not page) {
            page = NewCodePage(i_runtime, i_min_ଧାଡ଼ି_ଗଣନା);
        }

        ଯଦି (page) {
            i_runtime->ଅଙ୍କ_Code_Pages++;
        }
    }

    ଯଦି (page) {                                                  m3log (emit, "acquire page: %d", page->info.sequence);
        i_runtime->ଅଙ୍କ_Active_Code_Pages++;
    }

    ଫେରନ୍ତୁ page;
}


IM3CodePage AcquireCodePage (IM3Runtime i_runtime)
{
    ଫେରନ୍ତୁ AcquireCodePageWithCapacity(i_runtime, d_m3_Code_Page_Free_ଧାଡ଼ିଗୁଡ଼ିକ_Threshold);
}


ଶୂନ୍ୟ ReleaseCodePage (IM3Runtime i_runtime, IM3CodePage i_codePage)
{
    ଯଦି (i_codePage) {
        ReleaseCodePageNoTrack(i_runtime, i_codePage);
        i_runtime->ଅଙ୍କ_Active_Code_Pages--;

#ଯଦି defined(DEBUG)
        u32 ଅଙ୍କ_Open = ଗଣନା_Code_Pages(i_runtime->pagesOpen);
        u32 ଅଙ୍କ_Full = ଗଣନା_Code_Pages(i_runtime->pagesFull);

        m3log(runtime, "runtime: %p; open-pages: %d; full-pages: %d; active: %d; total: %d", i_runtime, ଅଙ୍କ_Open, ଅଙ୍କ_Full, i_runtime->ଅଙ୍କ_Active_Code_Pages, i_runtime->ଅଙ୍କ_Code_Pages);

        d_m3Assert(ଅଙ୍କ_Open + ଅଙ୍କ_Full + i_runtime->ଅଙ୍କ_Active_Code_Pages == i_runtime->ଅଙ୍କ_Code_Pages);

#  ଯଦି d_m3LogCodePages
        dump_code_page(i_codePage, /* startPC: */ ଖାଲି);
#  endif
#endif
    }
}


#ଯଦି d_m3VerboseErrorMessages
M3Result m3Error (M3Result i_result, IM3Runtime i_runtime, IM3Module i_module, IM3Function i_function,
                  ଅଚଳ ଅକ୍ଷର* ଅଚଳ i_file, u32 i_ଧାଡ଼ି_ଅଙ୍କ, ଅଚଳ ଅକ୍ଷର* ଅଚଳ i_errorMessage, ...)
{
    ଯଦି (i_runtime) {
        i_runtime->error         = (M3ErrorInfo){ .result = i_result, .runtime = i_runtime, .module = i_module, .function = i_function, .file = i_file, .ଧାଡ଼ି_2 = i_ଧାଡ଼ି_ଅଙ୍କ };
        i_runtime->error.message = i_runtime->error_message;

        va_list args;
        va_start(args, i_errorMessage);
        vsnprintf(i_runtime->error_message, ଆକାର(i_runtime->error_message), i_errorMessage, args);
        va_end(args);
    }

    ଫେରନ୍ତୁ i_result;
}
#endif


ଶୂନ୍ୟ m3_GetErrorInfo (IM3Runtime i_runtime, M3ErrorInfo* o_info)
{
    ଯଦି (i_runtime) {
        *o_info = i_runtime->error;
        m3_ResetErrorInfo(i_runtime);
    }
}


ଶୂନ୍ୟ m3_ResetErrorInfo (IM3Runtime i_runtime)
{
    ଯଦି (i_runtime) {
        M3_INIT(i_runtime->error);
        i_runtime->error.message = "";
    }
}

uint8_t* m3_GetMemory (IM3Module i_module, size_t* o_memorySizeInBytes, uint32_t i_memoryIndex)
{
    uint8_t* memory = ଖାଲି;
    size_t   size   = 0;

    ଯଦି (i_module and i_memoryIndex < i_module->ଅଙ୍କ_Memories) {
        IM3Memory mem = i_module->memories[i_memoryIndex];

        ଯଦି (mem->mallocated) {
            size = mem->mallocated->length;

            ଯଦି (size) {
                memory = m3MemData(mem->mallocated);
            }
        }
    }

    ଯଦି (o_memorySizeInBytes) {
        *o_memorySizeInBytes = size;
    }

    ଫେରନ୍ତୁ memory;
}


size_t m3_GetMemorySize (IM3Module i_module, uint32_t i_memoryIndex)
{
    ଯଦି (not i_module or i_memoryIndex >= i_module->ଅଙ୍କ_Memories) {
        ଫେରନ୍ତୁ 0;
    }

    IM3Memory mem = i_module->memories[i_memoryIndex];

    ଫେରନ୍ତୁ mem->mallocated ? mem->mallocated->length : 0;
}


size_t m3_GetMemorySizeAt (ଅଚଳ ଶୂନ୍ୟ* i_memory)
{
    ଯଦି (not i_memory) {
        ଫେରନ୍ତୁ 0;
    }

    // the header sits immediately before the data it describes
    ଅଚଳ M3MemoryHeader* header = ((ଅଚଳ M3MemoryHeader*)i_memory) - 1;

    ଫେରନ୍ତୁ header->length;
}


M3Result m3_FindExportedMemory (IM3Module i_module, cstr_t i_name, u32* o_memoryIndex)
{
    ଯଦି (not i_module or not i_name or not o_memoryIndex) {
        ଫେରନ୍ତୁ m3Err_unknownMemory;
    }

    ଅଚଳ M3Export* e = Module_FindExport(i_module, d_externalKind_memory, i_name);

    ଯଦି (not e) {
        ଫେରନ୍ତୁ m3Err_unknownMemory;
    }

    *o_memoryIndex = e->index;
    ଫେରନ୍ତୁ m3Err_none;
}


bool Module_HasLinkedHostImport (IM3Module i_module, cstr_t i_importModule)
{
    ପାଇଁ (u32 i = 0; i < i_module->ଅଙ୍କ_Functions; ++i) {
        IM3Function f = &i_module->functions[i];

        // hostMemory is set when a host function is bound to the import, so it
        // is also the mark of one - see CompileRawFunction
        ଯଦି (not f->hostMemory or f->wasm or f->resolved) {
            ଜାରି;
        }

        ଯଦି (f->import.moduleUtf8 and strcmp(f->import.moduleUtf8, i_importModule) == 0) {
            ଫେରନ୍ତୁ true;
        }
    }

    ଫେରନ୍ତୁ false;
}


M3Result m3_BindImportMemory (IM3Module io_module, cstr_t i_importModule, u32 i_memoryIndex)
{
    ଯଦି (not io_module or not i_importModule) {
        ଫେରନ୍ତୁ m3Err_moduleNotLinked;
    }

    ଯଦି (i_memoryIndex >= io_module->ଅଙ୍କ_Memories) {
        ଫେରନ୍ତୁ m3Err_unknownMemory;
    }

    // the memory itself, not the index: linking may have pointed this slot at
    // another module's memory, and growing one reallocates behind it
    IM3Memory memory = io_module->memories[i_memoryIndex];

    ଅଚଳ bool wildcardModule = (strcmp(i_importModule, "*") == 0);

    ପାଇଁ (u32 i = 0; i < io_module->ଅଙ୍କ_Functions; ++i) {
        IM3Function f = &io_module->functions[i];

        // only an import that a host function was bound to has a memory to
        // pin: one with a body of its own reads memory through the interpreter,
        // and one resolved to another module's export runs that module's code
        ଯଦି (not f->hostMemory or f->wasm or f->resolved) {
            ଜାରି;
        }

        ଯଦି (wildcardModule or (f->import.moduleUtf8 and strcmp(f->import.moduleUtf8, i_importModule) == 0)) {
            f->hostMemory = memory;
        }
    }

    ଫେରନ୍ତୁ m3Err_none;
}


M3BacktraceInfo* m3_GetBacktrace (IM3Runtime i_runtime)
{
#ଯଦି d_m3RecordBacktraces
    ଫେରନ୍ତୁ &i_runtime->backtrace;
#ଅନ୍ୟଥା
    ଫେରନ୍ତୁ ଖାଲି;
#endif
}


//-------------------------------------------------------------------------------------------------------------------------------
//  snapshots & suspendable execution
//-------------------------------------------------------------------------------------------------------------------------------

ଶୂନ୍ୟ m3_SetSuspendable (IM3Runtime io_runtime, bool i_suspendable)
{
    ଯଦି (io_runtime) {
        io_runtime->isSuspendable = i_suspendable;
    }
}

ଶୂନ୍ୟ m3_RequestSuspend (IM3Runtime io_runtime)
{
    ଯଦି (io_runtime) {
        io_runtime->suspendRequested = true;
    }
}

bool m3_IsSuspended (IM3Runtime i_runtime)
{
    ଯଦି (!i_runtime) {
        ଫେରନ୍ତୁ false;
    }
#ଯଦି d_m3_Has_ଗଦା_Switching
    ଯଦି (i_runtime->ମୂଳ_Continuation && i_runtime->ମୂଳ_Continuation->state == cont_suspended) {
        ଫେରନ୍ତୁ true;
    }
#endif
    ଫେରନ୍ତୁ false;
}

M3Result m3_ResumeRuntime (IM3Runtime io_runtime)
{
    ଯଦି (!io_runtime) {
        ଫେରନ୍ତୁ m3Err_mallocFailed;
    }
    ଯଦି (!io_runtime->isSuspendable) {
        ଫେରନ୍ତୁ m3Err_none;
    }
#ଯଦି d_m3HasSnapshots
    // a restore that failed leaves nothing to resume, and says so every time
    // rather than resuming nothing successfully
    ଯଦି (io_runtime->ମୂଳ_Continuation and io_runtime->ମୂଳ_Continuation->entryFunction and
        io_runtime->ମୂଳ_Continuation->entryFunction->module->isUnusable) {
        ଫେରନ୍ତୁ "a snapshot failed to restore into the module, so it cannot run";
    }
#endif
#ଯଦି d_m3_Has_ଗଦା_Switching
    ଯଦି (!io_runtime->ମୂଳ_Continuation || io_runtime->ମୂଳ_Continuation->state != cont_suspended) {
        ଫେରନ୍ତୁ m3Err_none;
    }
    // what finishes returns into the bottom of the stack, where it was entered
    io_runtime->ଗଦା = io_runtime->origin_ଗଦା;

    // this is the outermost call again, as far as a host function calling back
    // into Wasm from inside it can tell
    io_runtime->entered = io_runtime->ମୂଳ_Continuation->entryFunction;
    io_runtime->callNesting++;

    d_m3_ଗଦା_Limit_Enter(io_runtime);
    io_runtime->resumePastCheck = true;
    M3Result result             = (M3Result)ResumeContinuation(io_runtime, io_runtime->ମୂଳ_Continuation);
    io_runtime->resumePastCheck = false;
    d_m3_ଗଦା_Limit_Leave(io_runtime);

#  ଯଦି d_m3HasExceptionHandling
    ଯଦି (M3_UNLIKELY(result == m3Err_pendingException)) {
        result = m3Err_trapUncaughtException;
    }
    RecordUncaughtException(io_runtime, result);
#  endif

    io_runtime->callNesting--;

#  ଯଦି d_m3HasExceptionHandling
    ଯଦି (result != m3Err_continuationSuspended and io_runtime->callNesting == 0) {
        FreeExceptions(io_runtime);
    }
#  endif

    // the results are where the paused call left room for them, so a finished
    // resume can be asked for them the way the call itself could have been
    io_runtime->lastCalled = (result == m3Err_none) ? io_runtime->ମୂଳ_Continuation->entryFunction : ଖାଲି;

    Runtime_Place_Call_ଗଦା(io_runtime);
    ଫେରନ୍ତୁ result;
#ଅନ୍ୟଥା
    ଫେରନ୍ତୁ m3Err_none;
#endif
}
