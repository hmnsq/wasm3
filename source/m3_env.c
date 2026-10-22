//
//  m3_env.c
//
//  Created by Steven Massey on 4/19/19.
//  Copyright © 2019 Steven Massey. All rights reserved.
//

#include <stdarg.h>
#include <limits.h>
#include <errno.h>
#include <float.h>
#include <ctype.h>

#include "m3_env.h"
#include "m3_bind.h"
#include "m3_deterministic.h"
#include "m3_compile.h"
#include "m3_host.h"
#include "m3_exception.h"
#include "m3_info.h"


IM3Environment m3_NewEnvironment ()
{
    IM3Environment env = m3_AllocStruct(M3Environment);

    ᨀᨘ (env) {
        _try
        {
            // create FuncTypes for all simple block return ValueTypes.
            // v128 is skipped: it parses as a slot but has no operations.
            for (u8 t = c_m3Type_none; t < c_m3Type_count; t++) {
                ᨀᨘ (t == c_m3Type_v128) {
                    continue;
                }

                IM3FuncType ftype;
_               (AllocFuncType(&ftype, 1));

                ftype->numArgs  = 0;
                ftype->numRets  = (t == c_m3Type_none) ? 0 : 1;
                ftype->types[0] = t;

                Environment_AddFuncType(env, &ftype);

                env->retFuncTypes[t] = ftype;
            }
        }

        _catch:
        ᨀᨘ (result) {
            m3_FreeEnvironment(env);
            env = NULL;
        }
    }

    ᨀᨘᨒ env;
}


void Environment_Release (IM3Environment i_environment)
{
    IM3FuncType ftype = i_environment->funcTypes;

    ᨀᨛ (ftype) {
        IM3FuncType next = ftype->next;
        m3_Free(ftype);
        ftype = next;
    }

    m3log(runtime, "freeing %d pages from environment", CountCodePages(i_environment->pagesReleased));
    FreeCodePages(&i_environment->pagesReleased);
}


void m3_FreeEnvironment (IM3Environment i_environment)
{
    ᨀᨘ (i_environment) {
        Environment_Release(i_environment);
        m3_Free(i_environment);
    }
}


void m3_SetCustomSectionHandler (IM3Environment i_environment, M3SectionHandler i_handler)
{
    ᨀᨘ (i_environment) {
        i_environment->customSectionHandler = i_handler;
    }
}


// returns the same io_funcType or replaces it with an equivalent that's already in the type linked list
M3Result Environment_AddFuncType (IM3Environment i_environment, IM3FuncType* io_funcType)
{
    IM3FuncType addType = *io_funcType;
    IM3FuncType newType = i_environment->funcTypes;

    ᨀᨛ (newType) {
        ᨀᨘ (not newType->inRecGroup and AreFuncTypesEqual(newType, addType)) {
            m3_Free(addType);
            bha_nga;
        }

        newType = newType->next;
    }

    ᨀᨘ (newType == NULL) {
        // a type index has to fit in the heap type field of an m3type_t
        ᨀᨘ (i_environment->numFuncTypes >= d_m3MaxSaneTypesCount) {
            m3_Free(addType);
            *io_funcType = NULL;
            ᨀᨘᨒ "too many distinct function types";
        }

        newType                  = addType;
        newType->canonicalIndex  = i_environment->numFuncTypes++;
        newType->next            = i_environment->funcTypes;
        i_environment->funcTypes = newType;
    }

    *io_funcType = newType;

    ᨀᨘᨒ m3Err_none;
}


// Adds a type without looking for an equal one first, and hands back the index
// it was given.
//
// Members of a recursive group do not get to share an entry with a type that
// merely looks like them: the group is their identity, so two groups spelled
// the same way are still two groups, and a type inside one is never the type
// outside it that matches field for field. Reserving the indices up front is
// also what lets the members refer to each other while they are being read.
M3Result Environment_ReserveFuncTypes (IM3Environment i_environment, u32 i_count, u16* o_firstIndex)
{
    ᨀᨘ ((u64)i_environment->numFuncTypes + i_count > d_m3MaxSaneTypesCount) {
        ᨀᨘᨒ "too many distinct function types";
    }

    *o_firstIndex = i_environment->numFuncTypes;
    i_environment->numFuncTypes += (u16)i_count;

    ᨀᨘᨒ m3Err_none;
}


void Environment_AdoptFuncType (IM3Environment i_environment, IM3FuncType i_funcType, u16 i_canonicalIndex)
{
    i_funcType->inRecGroup     = true;
    i_funcType->canonicalIndex = i_canonicalIndex;
    i_funcType->next           = i_environment->funcTypes;
    i_environment->funcTypes   = i_funcType;
}


IM3CodePage RemoveCodePageOfCapacity (M3CodePage** io_list, u32 i_minimumLineCount)
{
    IM3CodePage prev = NULL;
    IM3CodePage page = *io_list;

    ᨀᨛ (page) {
        ᨀᨘ (NumFreeLines(page) >= i_minimumLineCount) {             d_m3Assert (page->info.usageCount == 0);
            IM3CodePage next = page->info.next;
            ᨀᨘ (prev) {
                prev->info.next = next; // mid-list
            } ᨀᨘᨁ {
                *io_list = next;       // front of list
            }

            bha_nga;
        }

        prev = page;
        page = page->info.next;
    }

    ᨀᨘᨒ page;
}


IM3CodePage Environment_AcquireCodePage (IM3Environment i_environment, u32 i_minimumLineCount)
{
    ᨀᨘᨒ RemoveCodePageOfCapacity(&i_environment->pagesReleased, i_minimumLineCount);
}


void Environment_ReleaseCodePages (IM3Environment i_environment, IM3CodePage i_codePageList)
{
    IM3CodePage end = i_codePageList;

    ᨀᨛ (end) {
        end->info.lineIndex = 0; // reset page
#ᨀᨘ d_m3RecordBacktraces
        end->info.mapping->size = 0;
#endif // d_m3RecordBacktraces

        IM3CodePage next = end->info.next;
        ᨀᨘ (not next) {
            bha_nga;
        }

        end = next;
    }

    ᨀᨘ (end) {
        // push list to front
        end->info.next               = i_environment->pagesReleased;
        i_environment->pagesReleased = i_codePageList;
    }
}


IM3Runtime m3_NewRuntime (IM3Environment i_environment, u32 i_stackSizeInBytes, void* i_userdata)
{
    IM3Runtime runtime = m3_AllocStruct(M3Runtime);

    ᨀᨘ (runtime) {
        m3_ResetErrorInfo(runtime);

        runtime->environment = i_environment;
        runtime->userdata    = i_userdata;

        Deterministic_InitRuntime(runtime);

        runtime->originStack = m3_Malloc("Wasm Stack", i_stackSizeInBytes + 4 * parimaa_na(m3slot_t)); // TODO: more precise stack checks

        ᨀᨘ (runtime->originStack) {
            runtime->stack         = runtime->originStack;
            runtime->numStackSlots = i_stackSizeInBytes / parimaa_na(m3slot_t);          m3log (runtime, "new stack: %p, slots: %u", runtime->originStack, runtime->numStackSlots);
        } ᨀᨘᨁ {
            m3_Free(runtime);
        }
    }

    ᨀᨘᨒ runtime;
}

void m3_SetValidation (IM3Runtime i_runtime, bool i_enable)
{
#ᨀᨘ d_m3EnableValidation
    ᨀᨘ (i_runtime) {
        i_runtime->skipValidation = not i_enable;
    }
#ᨀᨘᨁ
    (void)i_runtime;
    (void)i_enable;              // nothing to skip: the validator was compiled out
#endif
}

M3Result m3_SetResourceLimit (IM3Runtime i_runtime, M3ResourceLimit i_limit, uint64_t i_value)
{
    ᨀᨘ (not i_runtime) {
        ᨀᨘᨒ m3Err_mallocFailed;
    }
    switch (i_limit) {
    pak_sa c_m3Limit_MemoryBytes:
        ᨀᨘ (i_value and i_value < i_runtime->memoryBytesUsed) {
            ᨀᨘᨒ m3Err_resourceLimitBelowUsage;
        }
        i_runtime->memoryBytesLimit = i_value;
        ᨀᨘᨒ m3Err_none;
    pak_sa c_m3Limit_TableElements:
        ᨀᨘ (i_value and i_value < i_runtime->tableElementsUsed) {
            ᨀᨘᨒ m3Err_resourceLimitBelowUsage;
        }
        i_runtime->tableElementsLimit = i_value;
        ᨀᨘᨒ m3Err_none;
#ᨀᨘ d_m3HasStackSwitching
    pak_sa c_m3Limit_Continuations:
        ᨀᨘ (i_value and i_value < i_runtime->continuationsAllocated) {
            ᨀᨘᨒ m3Err_resourceLimitBelowUsage;
        }
        i_runtime->continuationsLimit = (u32)M3_MIN(i_value, (u64)UINT32_MAX);
        ᨀᨛ (i_value and i_runtime->valStackPoolCount > i_value - i_runtime->continuationsAllocated) {
            m3_Free(i_runtime->valStackPool[--i_runtime->valStackPoolCount]);
        }
        ᨀᨘᨒ m3Err_none;
#ᨀᨘᨁ
    pak_sa c_m3Limit_Continuations: ᨀᨘᨒ m3Err_resourceLimitNotSupported;
#endif
    pak_sa c_m3Limit_GasUnits:
#ᨀᨘ d_m3HasGasMetering
        i_runtime->gasLimit = i_runtime->gasRemaining = (i64)M3_MIN(i_value, (u64)INT64_MAX);
        ᨀᨘᨒ m3Err_none;
#ᨀᨘᨁ
        ᨀᨘᨒ m3Err_resourceLimitNotSupported;
#endif
    default: ᨀᨘᨒ m3Err_unknownResourceLimit;
    }
}

uint64_t m3_GetResourceLimit (IM3Runtime i_runtime, M3ResourceLimit i_limit)
{
    ᨀᨘ (not i_runtime) {
        ᨀᨘᨒ 0;
    }
    switch (i_limit) {
    pak_sa c_m3Limit_MemoryBytes: ᨀᨘᨒ i_runtime->memoryBytesLimit;
    pak_sa c_m3Limit_TableElements: ᨀᨘᨒ i_runtime->tableElementsLimit;
#ᨀᨘ d_m3HasStackSwitching
    pak_sa c_m3Limit_Continuations: ᨀᨘᨒ i_runtime->continuationsLimit;
#endif
#ᨀᨘ d_m3HasGasMetering
    pak_sa c_m3Limit_GasUnits: ᨀᨘᨒ (u64)i_runtime->gasLimit;
#endif
    default: ᨀᨘᨒ 0;
    }
}

uint64_t m3_GetResourceUsage (IM3Runtime i_runtime, M3ResourceLimit i_limit)
{
    ᨀᨘ (not i_runtime) {
        ᨀᨘᨒ 0;
    }
    switch (i_limit) {
    pak_sa c_m3Limit_MemoryBytes: ᨀᨘᨒ i_runtime->memoryBytesUsed;
    pak_sa c_m3Limit_TableElements: ᨀᨘᨒ i_runtime->tableElementsUsed;
#ᨀᨘ d_m3HasStackSwitching
    pak_sa c_m3Limit_Continuations: ᨀᨘᨒ i_runtime->continuationsAllocated;
#endif
#ᨀᨘ d_m3HasGasMetering
    pak_sa c_m3Limit_GasUnits: ᨀᨘᨒ (u64)i_runtime->gasLimit - (u64)i_runtime->gasRemaining;
#endif
    default: ᨀᨘᨒ 0;
    }
}

void* m3_GetUserData (IM3Runtime i_runtime)
{
    ᨀᨘᨒ i_runtime ? i_runtime->userdata : NULL;
}


void* ForEachModule (IM3Runtime i_runtime, ModuleVisitor i_visitor, void* i_info)
{
    void* r = NULL;

    IM3Module module = i_runtime->modules;

    ᨀᨛ (module) {
        IM3Module next = module->next;
        ᨀᨘ ((r = i_visitor(module, i_info))) {
            bha_nga;
        }

        module = next;
    }

    ᨀᨘᨒ r;
}


void* _FreeModule (IM3Module i_module, void* i_info)
{
    m3_FreeModule(i_module);
    ᨀᨘᨒ NULL;
}


// A module's memory and table index spaces hold borrowed pointers: linking
// repoints an imported slot at the memory or table the exporting module owns.
// m3_FreeModule tells a borrowed slot from an owned one by reading that owner,
// which only holds while the object is still allocated - and the free walk
// below visits modules in load order, with no ordering between an owner and
// the modules borrowing from it. So clear the borrowed slots first, in a pass
// that frees nothing; the walk then skips them on the NULL check it already
// does, instead of reading a memory some earlier module has freed.
void* _ReleaseBorrowedSlots (IM3Module i_module, void* i_info)
{
    for (u32 i = 0; i < i_module->numMemories; ++i) {
        IM3Memory memory = i_module->memories[i];

        ᨀᨘ (memory and memory->owner != i_module) {
            i_module->memories[i] = NULL;
        }
    }

    for (u32 i = 0; i < i_module->numTables; ++i) {
        IM3Table table = i_module->tables[i];

        ᨀᨘ (table and table->owner != i_module) {
            i_module->tables[i] = NULL;
        }
    }

    ᨀᨘᨒ NULL;
}


void Runtime_Release (IM3Runtime i_runtime)
{
    ForEachModule(i_runtime, _ReleaseBorrowedSlots, NULL);
    ForEachModule(i_runtime, _FreeModule, NULL);                    d_m3Assert (i_runtime->numActiveCodePages == 0);

    Environment_ReleaseCodePages(i_runtime->environment, i_runtime->pagesOpen);
    Environment_ReleaseCodePages(i_runtime->environment, i_runtime->pagesFull);

    m3_Free(i_runtime->originStack);

#ᨀᨘ d_m3EnableValidation
    m3_Free(i_runtime->validator);
#endif

#ᨀᨘ d_m3HasExceptionHandling
    // a runtime let go of while suspended still holds its exceptions
    FreeExceptions(i_runtime);
    m3_Free(i_runtime->uncaughtException);
#endif
#ᨀᨘ d_m3HasExceptionHandling || d_m3HasStackSwitching
    ᨀᨛ (i_runtime->hostTags) {
        M3HostTag* next = i_runtime->hostTags->next;
        m3_Free(i_runtime->hostTags);
        i_runtime->hostTags = next;
    }
#endif
#ᨀᨘ d_m3HasStackSwitching
    Continuation_ReleaseAll(i_runtime);
#endif
}


void m3_FreeRuntime (IM3Runtime i_runtime)
{
    ᨀᨘ (i_runtime) {
        m3_PrintProfilerInfo();

        Runtime_Release(i_runtime);
        m3_Free(i_runtime);
    }
}


#ᨀᨘ d_m3HasStackSwitching

IM3Continuation Continuation_New (IM3Runtime i_runtime, IM3FuncType i_type, IM3Function i_function)
{
    IM3Continuation cont = m3_AllocStruct(M3Continuation);
    ᨀᨘ (!cont) {
        ᨀᨘᨒ NULL;
    }

    cont->type          = i_type;
    cont->entryFunction = i_function;
    cont->state         = cont_allocated;

    cont->numStackSlots = d_m3ContinuationStackSlots;
    // frames are recorded only when this continuation actually suspends, and
    // only as deep as it got, so the array starts out unallocated
    cont->frames    = NULL;
    cont->framesCap = 0;
    cont->numFrames = 0;

    cont->sp = cont->valStack;
    cont->pc = i_function ? i_function->compiled : NULL;
    cont->r0 = 0;
#  ᨀᨘ d_m3HasFloat
    cont->fp0 = 0;
#  endif
    cont->boundArgsCount    = 0;
    cont->numSuspendResults = 0;
    cont->numHandlers       = 0;
    cont->handlersPC        = NULL;
    cont->parent            = NULL;

    cont->next               = i_runtime->continuations;
    i_runtime->continuations = cont;

    ᨀᨘᨒ cont;
}

IM3Continuation Continuation_ForkSuspended (IM3Runtime i_runtime, IM3Continuation i_cont)
{
    IM3Continuation newCont = Continuation_New(i_runtime, i_cont->type, i_cont->entryFunction);
    ᨀᨘ (M3_UNLIKELY(not newCont)) {
        ᨀᨘᨒ NULL;
    }

    newCont->valStack      = i_cont->valStack;
    newCont->numStackSlots = i_cont->numStackSlots;
    newCont->sp            = i_cont->sp;
    newCont->pc            = i_cont->pc;
    newCont->suspendPoint  = i_cont->suspendPoint;
    newCont->r0            = i_cont->r0;
#  ᨀᨘ d_m3HasFloat
    newCont->fp0 = i_cont->fp0;
#  endif
    newCont->frames    = i_cont->frames;
    newCont->framesCap = i_cont->framesCap;
    newCont->numFrames = i_cont->numFrames;

    newCont->boundArgsCount = 0;
#  ᨀᨘ d_m3HasExceptionHandling
    newCont->resumeThrow = i_cont->resumeThrow;
    i_cont->resumeThrow  = NULL;
#  endif
    newCont->numSuspendResults = i_cont->numSuspendResults;
    for (u32 i = 0; i < i_cont->numSuspendResults; ++i) {
        newCont->suspendResultOffsets[i] = i_cont->suspendResultOffsets[i];
        newCont->suspendResultIs64[i]    = i_cont->suspendResultIs64[i];
    }

    newCont->numHandlers = i_cont->numHandlers;
    newCont->handlersPC  = i_cont->handlersPC;
    newCont->parent      = i_cont->parent;
    newCont->state       = cont_suspended;

    i_cont->valStack  = NULL;
    i_cont->frames    = NULL;
    i_cont->framesCap = 0;
    i_cont->numFrames = 0;
    i_cont->state     = cont_consumed;

    ᨀᨘᨒ newCont;
}

// Only cont.new and snapshot restoration acquire stacks. Binding and suspension
// transfer them between records without changing the active count.
M3Result Continuation_AcquireStack (IM3Runtime runtime, IM3Continuation cont)
{
    ᨀᨘ (not Continuation_CanAcquireStack(runtime)) {
        ᨀᨘᨒ m3Err_continuationLimitExceeded;
    }
    ᨀᨘ (runtime->valStackPoolCount) {
        cont->valStack = runtime->valStackPool[--runtime->valStackPoolCount];
        memset(cont->valStack, 0, (cont->numStackSlots + 4) * parimaa_na(m3slot_t));
    } ᨀᨘᨁ {
        cont->valStack = m3_AllocArray(m3slot_t, cont->numStackSlots + 4);
    }
    ᨀᨘ (not cont->valStack) {
        ᨀᨘᨒ m3Err_mallocFailed;
    }
    cont->sp = cont->valStack;
    runtime->continuationsAllocated++;
    ᨀᨘᨒ m3Err_none;
}

void Continuation_RecycleStack (IM3Runtime runtime, IM3Continuation cont)
{
    ᨀᨘ (not cont->valStack or cont == runtime->rootContinuation) {
        ᨀᨘᨒ;
    }
    ᨀᨘ (not runtime->valStackPool) {
        runtime->valStackPool = m3_AllocArray(m3slot_t*, 16);
        ᨀᨘ (runtime->valStackPool) {
            runtime->valStackPoolCap = 16;
        }
    }
    ᨀᨘ (runtime->valStackPoolCount < runtime->valStackPoolCap and
        (not runtime->continuationsLimit or runtime->valStackPoolCount <
                                              runtime->continuationsLimit - runtime->continuationsAllocated + 1)) {
        runtime->valStackPool[runtime->valStackPoolCount++] = cont->valStack;
        cont->valStack                                      = NULL;
    } ᨀᨘᨁ {
        m3_Free(cont->valStack);
    }
    cont->sp = NULL;
    runtime->continuationsAllocated--;
}

void Continuation_ReleaseAll (IM3Runtime io_runtime)
{
    // Records live until teardown: a consumed reference can still be in a table
    // or on a Wasm stack. Its value stack has already been recycled.
    IM3Continuation cont = io_runtime->continuations;

    ᨀᨛ (cont) {
        IM3Continuation next = cont->next;

        m3_Free(cont->valStack);
        m3_Free(cont->frames);
        m3_Free(cont);

        cont = next;
    }

    for (u32 i = 0; i < io_runtime->valStackPoolCount; ++i) {
        m3_Free(io_runtime->valStackPool[i]);
    }
    m3_Free(io_runtime->valStackPool);
    io_runtime->valStackPoolCount = io_runtime->valStackPoolCap = 0;
    io_runtime->continuationsAllocated                          = 0;
    io_runtime->continuations                                   = NULL;
    io_runtime->activeContinuation                              = NULL;
    io_runtime->rootContinuation                                = NULL;
}


m3ret_t Continuation_RecordFrame (IM3Runtime i_runtime, const M3Frame* i_frame)
{
    IM3Continuation cont = i_runtime->activeContinuation;

    // the marker only travels between a suspend and the resume that runs it,
    // and that whole stretch has an active continuation
    d_m3Assert(cont);

    ᨀᨘ (cont->numFrames == cont->framesCap) {
        u32 newCap = cont->framesCap ? cont->framesCap * 2 : 8;

        ᨀᨘ (newCap > d_m3ContinuationMaxFrames) {
            newCap = d_m3ContinuationMaxFrames;
        }

        ᨀᨘ (newCap == cont->framesCap) {
            ᨀᨘᨒ m3Err_trapStackOverflow;
        }

        M3Frame* frames = m3_ReallocArray(M3Frame, cont->frames, newCap, cont->framesCap);

        ᨀᨘ (not frames) {
            ᨀᨘᨒ m3Err_mallocFailed;
        }

        cont->frames    = frames;
        cont->framesCap = newCap;
    }

    cont->frames[cont->numFrames++] = *i_frame;

    ᨀᨘᨒ m3Err_continuationSuspended;
}

#endif // d_m3HasStackSwitching

M3Result EvaluateExpression (IM3Module i_module, void* o_expressed, m3type_t i_type, bytes_t* io_bytes, cbytes_t i_end)
{
    M3Result result = m3Err_none;

    // OPTZ: use a simplified interpreter for expressions

    // create a temporary runtime context
#ᨀᨘ defined(d_m3PreferStaticAlloc)
    static M3Runtime runtime;
#ᨀᨘᨁ
    M3Runtime runtime;
#endif
    M3_INIT(runtime);

    runtime.environment   = i_module->runtime->environment;
    runtime.numStackSlots = i_module->runtime->numStackSlots;
    runtime.stack         = i_module->runtime->stack;

    m3stack_t stack = (m3stack_t)runtime.stack;

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

    ᨀᨘ (o->page) {
        IM3FuncType ftype = NULL;

#ᨀᨘ d_m3HasTypedRefs
        // A reference type has to be checked as the type it is: the shared
        // return types are one per storage class, so borrowing one of those
        // would let any reference at all satisfy a particular one.
        ᨀᨘ (IsSpelledRefType(i_type)) {
            result = AllocFuncType(&ftype, 1);

            ᨀᨘ (not result) {
                ftype->numRets  = 1;
                ftype->types[0] = i_type;

                result = Environment_AddFuncType(runtime.environment, &ftype);
            }
        } ᨀᨘᨁ
#endif
        {
            ftype = runtime.environment->retFuncTypes[BaseTypeOf(i_type)];
        }

        pc_t m3code = GetPagePC(o->page);

        ᨀᨘ (not result) {
            result = CompileExpression(o, ftype);
        }

        ᨀᨘ (not result && o->maxStackSlots >= runtime.numStackSlots) {
            result = m3Err_trapStackOverflow;
        }

        ᨀᨘ (not result) {
#ᨀᨘ (d_m3EnableOpProfiling || d_m3EnableOpTracing)
            m3ret_t r = RunCode(m3code, stack, NULL, d_m3OpDefaultArgs, d_m3BaseCstr);
#ᨀᨘᨁ
            m3ret_t r = RunCode(m3code, stack, NULL, d_m3OpDefaultArgs);
#endif

            ᨀᨘ (r == 0) {                                                                   m3log (runtime, "expression result: %s", SPrintValue (stack, i_type));
                ᨀᨘ (SizeOfType(BaseTypeOf(i_type)) == parimaa_na(u32)) {
                    *(u32*)o_expressed = *((u32*)stack);
                } ᨀᨘᨁ {
                    *(u64*)o_expressed = *((u64*)stack);
                }
            }
        }

        ReleaseCodePage(&runtime, o->page);
    } ᨀᨘᨁ {
        result = m3Err_mallocFailedCodePage;
    }

    runtime.originStack = NULL;        // prevent free(stack) in ReleaseRuntime
    Runtime_Release(&runtime);
    i_module->runtime = savedRuntime;

    *io_bytes = o->wasm;

    ᨀᨘᨒ result;
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
static
bool LimitsSatisfy (u64 i_exportedSize, bool i_exportedHasMax, u64 i_exportedMax,
                    u64 i_importMin, bool i_importHasMax, u64 i_importMax)
{
    ᨀᨘ (i_exportedSize < i_importMin) {
        ᨀᨘᨒ false;
    }

    ᨀᨘ (i_importHasMax and (not i_exportedHasMax or i_exportedMax > i_importMax)) {
        ᨀᨘᨒ false;
    }

    ᨀᨘᨒ true;
}


// The export of this kind under this name, or NULL. The index it carries is
// resolved by the caller, against the module's index space as it stands then -
// after linking, a re-exported import's slot holds the entity it was linked to.
static
const M3Export* Module_FindExport (IM3Module i_module, u8 i_kind, cstr_t i_name)
{
    for (u32 i = 0; i < i_module->numExports; ++i) {
        const M3Export* e = &i_module->exports[i];

        ᨀᨘ (e->kind == i_kind and strcmp(e->name, i_name) == 0) {
            ᨀᨘᨒ e;
        }
    }

    ᨀᨘᨒ NULL;
}


static
IM3Function Module_FindExportedFunction (IM3Module i_module, cstr_t i_name)
{
    const M3Export* e = Module_FindExport(i_module, d_externalKind_function, i_name);

    // A module that re-exports an import names the placeholder here. Resolve to
    // the function that actually runs, or a host-side call would run it against
    // the importing module's memory.
    ᨀᨘᨒ e ? Function_Implementation(&i_module->functions[e->index]) : NULL;
}


static
IM3Memory Module_FindExportedMemory (IM3Module i_module, cstr_t i_name)
{
    const M3Export* e = Module_FindExport(i_module, d_externalKind_memory, i_name);

    ᨀᨘᨒ e ? i_module->memories[e->index] : NULL;
}


static
IM3Table Module_FindExportedTable (IM3Module i_module, cstr_t i_name)
{
    const M3Export* e = Module_FindExport(i_module, d_externalKind_table, i_name);

    ᨀᨘᨒ e ? i_module->tables[e->index] : NULL;
}


static
IM3Global Module_FindExportedGlobal (IM3Module i_module, cstr_t i_name)
{
    const M3Export* e = Module_FindExport(i_module, d_externalKind_global, i_name);

    ᨀᨘᨒ e ? &i_module->globals[e->index] : NULL;
}


#ᨀᨘ d_m3HasExceptionHandling || d_m3HasStackSwitching
static
IM3Tag Module_FindExportedTag (IM3Module i_module, cstr_t i_name)
{
    const M3Export* e = Module_FindExport(i_module, d_externalKind_tag, i_name);

    ᨀᨘ (not e) {
        ᨀᨘᨒ NULL;
    }

    IM3Tag tag = &i_module->tags[e->index];

    ᨀᨘᨒ tag->resolved ? tag->resolved : tag;
}
#endif


// Whether the module exports anything at all under this name. Export names are
// unique within a module, so a name one of the lookups above missed but this
// one finds is exported as something else - a kind the import cannot be
// satisfied by, rather than a name the module never exported.
static
bool Module_HasExport (IM3Module i_module, cstr_t i_name)
{
    for (u32 i = 0; i < i_module->numExports; ++i) {
        ᨀᨘ (strcmp(i_module->exports[i].name, i_name) == 0) {
            ᨀᨘᨒ true;
        }
    }

    ᨀᨘᨒ false;
}


// Points each of the module's imports at whatever already-loaded module exports
// it. Runs before anything is allocated or initialized: a memory import has to
// be resolved before InitMemory would give it pages of its own, and a global
// import before InitGlobals runs an initializer that reads it.
static
M3Result LinkImports (IM3Runtime io_runtime, IM3Module io_module)
{
    M3Result result = m3Err_none;

    for (u32 i = 0; i < io_module->numFunctions; ++i) {
        IM3Function f = &io_module->functions[i];

        ᨀᨘ (f->wasm or not (f->import.moduleUtf8 and f->import.fieldUtf8)) {
            continue;
        }

        IM3Module from = m3_FindModule(io_runtime, f->import.moduleUtf8);
        ᨀᨘ (not from) {
            continue;
        }

        IM3Function exported = Module_FindExportedFunction(from, f->import.fieldUtf8);
        ᨀᨘ (not exported) {
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

    for (u32 i = 0; i < io_module->numMemories; ++i) {
        IM3Memory memory = io_module->memories[i];

        ᨀᨘ (not memory->imported or memory->owner != io_module) {
            continue;
        }

        IM3Module from = m3_FindModule(io_runtime, memory->import.moduleUtf8);
        ᨀᨘ (not from) {
            continue;
        }

        IM3Memory exported = Module_FindExportedMemory(from, memory->import.fieldUtf8);
        ᨀᨘ (not exported) {
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
                 not LimitsSatisfy(exported->numPages, exported->hasMax, exported->maxPages,
                                   memory->initPages, memory->hasMax, memory->maxPages));

        // hand the slot over to the exporter's memory, and drop the placeholder
        FreeMemoryBlock(memory);
        FreeImportInfo(&memory->import);
        m3_Free(memory);

        io_module->memories[i] = exported;
    }

    for (u32 i = 0; i < io_module->numTables; ++i) {
        IM3Table table = io_module->tables[i];

        ᨀᨘ (not table->imported or table->owner != io_module) {
            continue;
        }

        IM3Module from = m3_FindModule(io_runtime, table->import.moduleUtf8);
        ᨀᨘ (not from) {
            continue;
        }

        IM3Table exported = Module_FindExportedTable(from, table->import.fieldUtf8);
        ᨀᨘ (not exported) {
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

    for (u32 i = 0; i < io_module->numGlobals; ++i) {
        IM3Global g = &io_module->globals[i];

        ᨀᨘ (not g->imported or not (g->import.moduleUtf8 and g->import.fieldUtf8)) {
            continue;
        }

        IM3Module from = m3_FindModule(io_runtime, g->import.moduleUtf8);
        ᨀᨘ (not from) {
            continue;
        }

        IM3Global exported = Module_FindExportedGlobal(from, g->import.fieldUtf8);
        ᨀᨘ (not exported) {
            _throwif(m3Err_incompatibleImportType, Module_HasExport(from, g->import.fieldUtf8));
            _throw(m3Err_unknownImport);
        }

        _throwif(m3Err_incompatibleImportType, exported->type != g->type);
        _throwif(m3Err_incompatibleImportType, exported->isMutable != g->isMutable);

        g->resolved = exported->resolved ? exported->resolved : exported;
    }

#ᨀᨘ d_m3HasExceptionHandling || d_m3HasStackSwitching
    for (u32 i = 0; i < io_module->numTags; ++i) {
        IM3Tag tag = &io_module->tags[i];

        ᨀᨘ (not tag->imported or not (tag->import.moduleUtf8 and tag->import.fieldUtf8)) {
            continue;
        }

        IM3Module from = m3_FindModule(io_runtime, tag->import.moduleUtf8);
        ᨀᨘ (not from) {
            continue;
        }

        IM3Tag exported = Module_FindExportedTag(from, tag->import.fieldUtf8);
        ᨀᨘ (not exported) {
            _throwif(m3Err_incompatibleImportType, Module_HasExport(from, tag->import.fieldUtf8));
            _throw(m3Err_unknownImport);
        }

        _throwif(m3Err_incompatibleImportType, exported->type != tag->type);

        tag->resolved = exported->resolved ? exported->resolved : exported;
    }
#endif

    _catch: ᨀᨘᨒ result;
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
    i_module->memory0 = i_module->numMemories ? i_module->memories[0]
                                              : &i_module->emptyMemory;

    ᨀᨘ (i_module->numMemories == 0) {
        // nothing addressable, but _mem still has to point somewhere
        i_module->emptyMemory.owner    = i_module;
        i_module->emptyMemory.pageSize = d_m3DefaultMemPageSize;

_       (ResizeMemory(io_runtime, &i_module->emptyMemory, 0));
    }

    for (u32 i = 0; i < i_module->numMemories; ++i) {
        IM3Memory memory = i_module->memories[i];

        // a slot that already points at another module's memory is that
        // module's to allocate
        ᨀᨘ (memory->owner != i_module or memory->mallocated) {
            continue;
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
        ᨀᨘ (not memory->hasMax) {
            memory->maxPages = memory->isMemory64
                                 ? (pageSize > 1 ? (UINT64_MAX / pageSize) + 1 : UINT64_MAX)
                                 : (0x100000000ull / pageSize);
        }

_       (ResizeMemory(io_runtime, memory, memory->initPages));
    }

    _catch: ᨀᨘᨒ result;
}


M3Result ResizeMemory (IM3Runtime io_runtime, IM3Memory memory, u64 i_numPages)
{
    M3Result result = m3Err_none;

    u64 numPagesToAlloc = i_numPages;

    ᨀᨘ (numPagesToAlloc <= memory->maxPages) {
        // A 64-bit memory may ask for up to 2^48 pages, which overflows a u64
        // of bytes. Nothing that large can be backed, so refuse it up front
        // rather than multiplying into a wrapped size.
        _throwif("linear memory limitation exceeded",
                 numPagesToAlloc > d_m3AddressLimit / memory->pageSize);

        u64 numPageBytes = numPagesToAlloc * memory->pageSize;

#ᨀᨘ d_m3MaxLinearMemoryPages > 0
        // the limit is a memory size, counted in default-sized pages; comparing
        // it against a raw page count would make it 65536 times stricter for a
        // module whose pages are one byte
        _throwif("linear memory limitation exceeded",
                 numPageBytes > (u64)d_m3MaxLinearMemoryPages * d_m3DefaultMemPageSize);
#endif

        // Limit the amount of memory that gets actually allocated
        ᨀᨘ (io_runtime->memoryLimit) {
            numPageBytes = M3_MIN(numPageBytes, (u64)io_runtime->memoryLimit);
        }

        u64 oldLength = memory->mallocated ? memory->mallocated->length : 0;
        u64 otherUsed = io_runtime->memoryBytesUsed - oldLength;
        _throwif(m3Err_memoryLimitExceeded, io_runtime->memoryBytesLimit and
                                              (numPageBytes > io_runtime->memoryBytesLimit or
                                               otherUsed > io_runtime->memoryBytesLimit - numPageBytes));

        _throwif("linear memory limitation exceeded", numPageBytes > (u64)SIZE_MAX - parimaa_na(M3MemoryHeader));

#ᨀᨘ d_m3GuardedMemory

        // A slot holds every address a Wasm access can name, and nothing beyond
        // that: a memory larger than the data part of one would put addresses past
        // the guard, where a fault is nobody's to recognize
        _throwif("linear memory limitation exceeded", numPageBytes > d_m3GuardedDataBytes);

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

        ᨀᨘ (memory->guardSlot == NULL) {
            memory->guardSlot = Guard_TakeSlot();
            _throwifnull(memory->guardSlot);
        }

        // Growing only ever commits more of the same slot, so the memory never
        // moves - which the interpreter does not rely on, but nothing minds
        _throwif(m3Err_mallocFailed, not Guard_CommitSlot(memory->guardSlot, (size_t)numPageBytes));

        memory->mallocated = Guard_SlotHeader(memory->guardSlot);

#ᨀᨘᨁ

        size_t numBytes = (size_t)numPageBytes + parimaa_na(M3MemoryHeader);

        // What is really there, which is not numPages * pageSize: the limit above
        // caps the bytes without capping the page count, so a memory can be short
        // of a page - by design, since a whole 64 KiB page is often more than an
        // MCU has. Measuring the old block by the page count would overstate it,
        // and then realloc never sees the size it already has and copies the
        // whole limit again on every grow past it.
        size_t numPreviousBytes = memory->mallocated ? memory->mallocated->length + parimaa_na(M3MemoryHeader) : 0;

        void* newMem = m3_Realloc("Wasm Linear Memory", memory->mallocated, numBytes, numPreviousBytes);
        _throwifnull(newMem);

        memory->mallocated = (M3MemoryHeader*)newMem;

#endif

#ᨀᨘ d_m3LogRuntime
        M3MemoryHeader* oldMallocated = memory->mallocated;
#endif

        io_runtime->memoryBytesUsed = otherUsed + numPageBytes;
        memory->numPages            = numPagesToAlloc;

        memory->mallocated->length  = (size_t)numPageBytes;
        memory->mallocated->runtime = io_runtime;
        memory->mallocated->memory  = memory;

        memory->mallocated->maxStack = (m3slot_t*)io_runtime->originStack + io_runtime->numStackSlots;

        m3log(runtime, "resized old: %p; mem: %p; length: %zu; pages: %llu", oldMallocated, memory->mallocated, memory->mallocated->length, (unsigned long long)memory->numPages);
    } ᨀᨘᨁ {
        result = m3Err_wasmMemoryOverflow;
    }

    _catch: ᨀᨘᨒ result;
}


void FreeMemoryBlock (IM3Memory io_memory)
{
    ᨀᨘ (io_memory->mallocated and io_memory->mallocated->runtime) {
        io_memory->mallocated->runtime->memoryBytesUsed -= io_memory->mallocated->length;
    }
#ᨀᨘ d_m3GuardedMemory
    Guard_GiveSlot(io_memory->guardSlot);
    io_memory->guardSlot = NULL;

    io_memory->mallocated = NULL;
#ᨀᨘᨁ
    m3_Free(io_memory->mallocated);
#endif
}


M3Result InitGlobals (IM3Module io_module)
{
    M3Result result = m3Err_none;

    ᨀᨘ (io_module->numGlobals) {
        // placing the globals in their structs isn't good for cache locality, but i don't really know what the global
        // access patterns typically look like yet.

        //          io_module->globalMemory = m3Alloc (m3reg_t, io_module->numGlobals);

        //          if (io_module->globalMemory)
        {
            for (u32 i = 0; i < io_module->numGlobals; ++i) {
                M3Global* g = &io_module->globals[i];                           m3log (runtime, "initializing global: %d", i);

                ᨀᨘ (g->initExpr) {
                    bytes_t start = g->initExpr;

                    result = EvaluateExpression(io_module, &g->i64Value, g->type, &start, g->initExpr + g->initExprSize);

                    ᨀᨘ (not result) {
                        // io_module->globalMemory [i] = initValue;
                    } ᨀᨘᨁ {
                        bha_nga;
                    }
                } ᨀᨘᨁ {                                                        m3log (runtime, "importing global");
                }
            }
        }
        //          else result = ErrorModule (m3Err_mallocFailed, io_module, "could allocate globals for module: '%s", io_module->name);
    }

    ᨀᨘᨒ result;
}


M3Result InitDataSegments (IM3Module io_module)
{
    M3Result result = m3Err_none;

    for (u32 i = 0; i < io_module->numDataSegments; ++i) {
        M3DataSegment* segment = &io_module->dataSegments[i];

        // A passive segment stays available for memory.init until data.drop.
        // An active one is copied here and then counts as dropped.
        ᨀᨘ (segment->isPassive) {
            continue;
        }

        _throwif("data segment memory index out of range",
                 segment->memoryRegion >= io_module->numMemories);

        IM3Memory io_memory = io_module->memories[segment->memoryRegion];

        _throwif("unallocated linear memory", !(io_memory->mallocated));

        // The offset expression has the memory's address type, and is
        // unsigned: an i32 offset of -1 is 4294967295, way out of bounds
        // rather than negative.
        u64     segmentOffset = 0;
        bytes_t start         = segment->initExpr;

        ᨀᨘ (io_memory->isMemory64) {
_           (EvaluateExpression(io_module, &segmentOffset, c_m3Type_i64, &start, segment->initExpr + segment->initExprSize));
        } ᨀᨘᨁ {
            u32 offset32;
_           (EvaluateExpression(io_module, &offset32, c_m3Type_i32, &start, segment->initExpr + segment->initExprSize));
            segmentOffset = offset32;
        }

        m3log(runtime, "loading data segment: %d; size: %d; offset: %llu", i, segment->size, (unsigned long long)segmentOffset);

        ᨀᨘ (segmentOffset <= io_memory->mallocated->length &&
            (u64)segment->size <= io_memory->mallocated->length - segmentOffset) {
            u8* dest = m3MemData(io_memory->mallocated) + segmentOffset;
            memcpy(dest, segment->data, segment->size);
        } ᨀᨘᨁ {
            _throw("data segment out of bounds");
        }

        segment->dropped = true;
    }

    _catch: ᨀᨘᨒ result;
}


// Turns a segment's elements into references. Element expressions are constant
// expressions restricted to ref.null/ref.func, so they're read directly rather
// than run through the compiler.
static
M3Result ResolveElements (IM3Module io_module, M3ElementSegment* i_segment, void** o_elements)
{
    M3Result result = m3Err_none;

    bytes_t  pos = i_segment->elements;
    cbytes_t end = io_module->elementSectionEnd;

    for (u32 e = 0; e < i_segment->numElements; ++e) {
        u32   funcIndex;
        void* ref = NULL;

        ᨀᨘ (i_segment->isExpr) {
            m3opcode_t opcode;
_           (Read_opcode(&opcode, &pos, end));

            ᨀᨘ (opcode == c_waOp_refFunc) {
_               (ReadLEB_u32(&funcIndex, &pos, end));
                _throwif("function index out of range", funcIndex >= io_module->numFunctions);
                ref = Function_Implementation(&io_module->functions[funcIndex]);
            } ᨀᨘᨁ ᨀᨘ (opcode == c_waOp_refNull) {
                i8 waType;
                u8 nullType;
_               (ReadLEB_i7(&waType, &pos, end));
_               (NormalizeType(&nullType, waType));
                _throwif(m3Err_typeMismatch, nullType != i_segment->type);
            } ᨀᨘᨁ ᨀᨘ (opcode == c_waOp_getGlobal) {
                // wasm 2.0 lets an element expression read an imported
                // immutable global, which is how one module seeds another's
                // table with a reference it exported.
                u32 globalIndex;
_               (ReadLEB_u32(&globalIndex, &pos, end));
                _throwif(m3Err_globaIndexOutOfBounds, globalIndex >= io_module->numGlobals);

                IM3Global global = &io_module->globals[globalIndex];

                _throwif(m3Err_globaIndexOutOfBounds, not global->imported);
                _throwif(m3Err_wasmMalformed, global->isMutable);
                _throwif(m3Err_typeMismatch, BaseTypeOf(global->type) != BaseTypeOf(i_segment->type));

                // read the cell the import was linked to, not the placeholder
                ᨀᨘ (global->resolved) {
                    global = global->resolved;
                }

                ref = global->refValue;
            } ᨀᨘᨁ {
                _throw("constant expression required");
            }

_           (Read_opcode(&opcode, &pos, end));
            _throwif(m3Err_wasmMalformed, opcode != c_waOp_end);
        } ᨀᨘᨁ {
_           (ReadLEB_u32(&funcIndex, &pos, end));
            _throwif("function index out of range", funcIndex >= io_module->numFunctions);
            ref = Function_Implementation(&io_module->functions[funcIndex]);
        }

        o_elements[e] = ref;
    }

    _catch: ᨀᨘᨒ result;
}


M3Result InitTableAndElements (IM3Module io_module)
{
    M3Result result = m3Err_none;

    cbytes_t end = io_module->elementSectionEnd;
    M3Table* table;

    for (u32 i = 0; i < io_module->numTables; ++i) {
        table = io_module->tables[i];

        // a slot pointing at another module's table is that module's to fill
        ᨀᨘ (table->owner != io_module) {
            continue;
        }

        ᨀᨘ (table->size) {
            IM3Runtime runtime = io_module->runtime;
            _throwif(m3Err_tableLimitExceeded, runtime->tableElementsLimit and
                                                 table->size > runtime->tableElementsLimit - runtime->tableElementsUsed);
            table->elements = m3_AllocArray(void*, table->size);
            _throwifnull(table->elements);
            runtime->tableElementsUsed += table->size;

            ᨀᨘ (table->initExpr) {
                void*   value = NULL;
                bytes_t start = table->initExpr;
_               (EvaluateExpression(io_module, &value, BaseTypeOf(table->type),
                                     &start, table->initExpr + table->initExprSize));

                for (u32 e = 0; e < table->size; ++e) {
                    table->elements[e] = value;
                }
            }
        }
    }

    for (u32 i = 0; i < io_module->numElementSegments; ++i) {
        M3ElementSegment* segment = &io_module->elementSegments[i];

        // Declarative segments only make their functions referenceable, and
        // passive ones wait for table.init, so neither is written out here.
        ᨀᨘ (segment->mode == c_m3Elem_declarative) {
            segment->dropped = true;
            continue;
        }

        ᨀᨘ (segment->mode == c_m3Elem_passive) {
            ᨀᨘ (segment->numElements) {
                segment->resolved = m3_AllocArray(void*, segment->numElements);
                _throwifnull(segment->resolved);
_               (ResolveElements(io_module, segment, segment->resolved));
            }
            continue;
        }

        table = io_module->tables[segment->tableIndex];

        // The offset expression has the table's index type, and is unsigned:
        // an i32 offset of -1 is 4294967295, out of bounds rather than negative.
        u64     offset = 0;
        bytes_t expr   = segment->initExpr;

        ᨀᨘ (table->isTable64) {
_           (EvaluateExpression(io_module, &offset, c_m3Type_i64, &expr, end));
        } ᨀᨘᨁ {
            u32 offset32;
_           (EvaluateExpression(io_module, &offset32, c_m3Type_i32, &expr, end));
            offset = offset32;
        }

        _throwif("out of bounds table access",
                 offset > table->size or segment->numElements > table->size - offset);

_       (ResolveElements(io_module, segment, table->elements + offset));

        segment->dropped = true;
    }

    _catch: ᨀᨘᨒ result;
}

M3Result m3_CompileModule (IM3Module io_module)
{
    M3Result result = m3Err_none;

    for (u32 i = 0; i < io_module->numFunctions; ++i) {
        IM3Function f = &io_module->functions[i];
        ᨀᨘ (f->wasm and not f->compiled) {
_           (CompileFunction(f));
        }
    }

    _catch: ᨀᨘᨒ result;
}

#ᨀᨘ d_m3HasExceptionHandling

M3Exception* NewException (IM3Runtime io_runtime, IM3Tag i_tag, u32 i_numArgs)
{
    M3Exception* exception = (M3Exception*)m3_Malloc("M3Exception", parimaa_na(M3Exception) + i_numArgs * parimaa_na(u64));

    ᨀᨘ (exception) {
        exception->tag     = i_tag;
        exception->numArgs = i_numArgs;
        exception->reified = false;
        exception->prev    = NULL;
        exception->next    = io_runtime->exceptions;

        ᨀᨘ (exception->next) {
            exception->next->prev = exception;
        }

        io_runtime->exceptions = exception;
    }

    ᨀᨘᨒ exception;
}


// Releases one exception ahead of the rest. The caller has to know nothing can
// still name it: no exnref was ever taken of it, and its payload has already
// been copied out.
void FreeException (IM3Runtime io_runtime, M3Exception* i_exception)
{
    ᨀᨘ (i_exception->prev) {
        i_exception->prev->next = i_exception->next;
    } ᨀᨘᨁ {
        io_runtime->exceptions = i_exception->next;
    }

    ᨀᨘ (i_exception->next) {
        i_exception->next->prev = i_exception->prev;
    }

    ᨀᨘ (io_runtime->pendingException == i_exception) {
        io_runtime->pendingException = NULL;
    }

    m3_Free_Impl(i_exception);
}


// Releases every exception the runtime still holds. Only safe once the Wasm
// stack is empty, which is why the outermost RunCodeChecked() is the one that
// calls it.
void FreeExceptions (IM3Runtime io_runtime)
{
    M3Exception* exception = io_runtime->exceptions;

    io_runtime->exceptions       = NULL;
    io_runtime->pendingException = NULL;

    ᨀᨛ (exception) {
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
static
void RecordUncaughtException (IM3Runtime io_runtime, M3Result i_result)
{
    ᨀᨘ (io_runtime->uncaughtException) {
        m3_Free(io_runtime->uncaughtException);
    }

    M3Exception* exception = io_runtime->pendingException;

    ᨀᨘ (M3_UNLIKELY(i_result == m3Err_trapUncaughtException) and exception) {
        M3Exception* copy = (M3Exception*)m3_CopyMem(exception, parimaa_na(M3Exception) + exception->numArgs * parimaa_na(u64));

        ᨀᨘ (copy) {
            copy->next = NULL;
            copy->prev = NULL;
        }

        io_runtime->uncaughtException = copy;
    }
}

#endif // d_m3HasExceptionHandling


#ᨀᨘ d_m3HasExceptionHandling || d_m3HasStackSwitching

IM3Tag m3_FindTag (IM3Module i_module, const var_na* const i_tagName)
{
    ᨀᨘ (not i_module or not i_tagName) {
        ᨀᨘᨒ NULL;
    }

    ᨀᨘᨒ Module_FindExportedTag(i_module, i_tagName);
}


M3Result m3_NewTag (IM3Runtime io_runtime, IM3Tag* o_tag, const var_na* const i_signature)
{
    IM3FuncType type    = NULL;
    M3HostTag*  hostTag = NULL;

_try {
    _throwif("null tag", not o_tag);
    *o_tag = NULL;
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
    ᨀᨘᨒ result;
}


M3Result m3_LinkTag (IM3Module         io_module,
                     const var_na* const i_moduleName,
                     const var_na* const i_tagName,
                     IM3Tag            i_tag)
{
    ᨀᨘ (not io_module or not i_moduleName or not i_tagName or not i_tag) {
        ᨀᨘᨒ m3Err_tagLookupFailed;
    }

    const bool wildcardModule = (strcmp(i_moduleName, "*") == 0);
    IM3Tag     target         = i_tag->resolved ? i_tag->resolved : i_tag;

    M3Result result = m3Err_tagLookupFailed;

    for (u32 i = 0; i < io_module->numTags; ++i) {
        IM3Tag tag = &io_module->tags[i];

        ᨀᨘ (tag->imported and tag->import.moduleUtf8 and tag->import.fieldUtf8 and
            strcmp(tag->import.fieldUtf8, i_tagName) == 0 and
            (wildcardModule or strcmp(tag->import.moduleUtf8, i_moduleName) == 0)) {
            ᨀᨘ (tag->type != target->type) {
                ᨀᨘᨒ m3Err_incompatibleImportType;
            }

            tag->resolved = target;
            result        = m3Err_none;
        }
    }

    ᨀᨘᨒ result;
}


uint32_t m3_GetTagArgCount (IM3Tag i_tag)
{
    ᨀᨘᨒ i_tag ? i_tag->type->numArgs : 0;
}


M3ValueType m3_GetTagArgType (IM3Tag i_tag, uint32_t i_index)
{
    ᨀᨘ (i_tag and i_index < i_tag->type->numArgs) {
        ᨀᨘᨒ (M3ValueType)BaseTypeOf(d_FuncArgType(i_tag->type, i_index));
    }

    ᨀᨘᨒ c_m3Type_none;
}

#ᨀᨘᨁ

IM3Tag m3_FindTag (IM3Module i_module, const var_na* const i_tagName)
{
    ᨀᨘᨒ NULL;
}

M3Result m3_NewTag (IM3Runtime io_runtime, IM3Tag* o_tag, const var_na* const i_signature)
{
    ᨀᨘ (o_tag) {
        *o_tag = NULL;
    }
    ᨀᨘᨒ m3Err_tagLookupFailed;
}

M3Result m3_LinkTag (IM3Module io_module, const var_na* const i_moduleName, const var_na* const i_tagName, IM3Tag i_tag)
{
    ᨀᨘᨒ m3Err_tagLookupFailed;
}

uint32_t m3_GetTagArgCount (IM3Tag i_tag)
{
    ᨀᨘᨒ 0;
}

M3ValueType m3_GetTagArgType (IM3Tag i_tag, uint32_t i_index)
{
    ᨀᨘᨒ c_m3Type_none;
}

#endif // d_m3HasExceptionHandling || d_m3HasStackSwitching


#ᨀᨘ d_m3HasExceptionHandling

M3Result m3_ThrowException (IM3Runtime io_runtime, IM3Tag i_tag, uint32_t i_argc, const void* i_argptrs[])
{
    ᨀᨘ (not io_runtime or not i_tag) {
        ᨀᨘᨒ m3Err_tagLookupFailed;
    }

    IM3Tag      tag  = i_tag->resolved ? i_tag->resolved : i_tag;
    IM3FuncType type = tag->type;

    // a tag with results is a stack switching control tag, which is suspended
    // with rather than thrown
    ᨀᨘ (type->numRets) {
        ᨀᨘᨒ m3Err_typeMismatch;
    }
    ᨀᨘ (i_argc != type->numArgs) {
        ᨀᨘᨒ m3Err_argumentCountMismatch;
    }
    // with nothing on the stack there is nothing to unwind, and the exception
    // would sit on the runtime's list until some later call ended
    ᨀᨘ (io_runtime->callNesting == 0) {
        ᨀᨘᨒ "m3_ThrowException called outside a host function";
    }

    M3Exception* exception = NewException(io_runtime, tag, i_argc);
    ᨀᨘ (not exception) {
        ᨀᨘᨒ m3Err_mallocFailed;
    }

    // the layout op_Throw gives a payload: 32-bit values zero-extended, floats
    // as their bit pattern
    for (u32 i = 0; i < i_argc; ++i) {
        const void* value = i_argptrs[i];

        switch (BaseTypeOf(d_FuncArgType(type, i))) {
        pak_sa c_m3Type_i32:
        pak_sa c_m3Type_f32: {
            u32 bits;
            memcpy(&bits, value, parimaa_na(bits));
            exception->args[i] = bits;
            bha_nga;
        }
        pak_sa c_m3Type_i64:
        pak_sa c_m3Type_f64:
            memcpy(&exception->args[i], value, parimaa_na(u64));
            bha_nga;
        pak_sa c_m3Type_funcref:
        pak_sa c_m3Type_externref:
        pak_sa c_m3Type_exnref:
        pak_sa c_m3Type_contref:
            exception->args[i] = (u64)(*(const uintptr_t*)value);
            bha_nga;
        default:
            FreeException(io_runtime, exception);
            ᨀᨘᨒ "unknown argument type";
        }
    }

    io_runtime->pendingException = exception;

    ᨀᨘᨒ m3Err_pendingException;
}


IM3Tag m3_GetExceptionTag (IM3Runtime i_runtime)
{
    ᨀᨘ (i_runtime and i_runtime->uncaughtException) {
        ᨀᨘᨒ i_runtime->uncaughtException->tag;
    }

    ᨀᨘᨒ NULL;
}


M3Result m3_GetExceptionArgs (IM3Runtime i_runtime, uint32_t i_argc, const void* o_argptrs[])
{
    ᨀᨘ (not i_runtime or not i_runtime->uncaughtException) {
        ᨀᨘᨒ "no uncaught exception";
    }

    const M3Exception* exception = i_runtime->uncaughtException;
    IM3FuncType        type      = exception->tag->type;

    ᨀᨘ (i_argc != exception->numArgs) {
        ᨀᨘᨒ m3Err_argumentCountMismatch;
    }

    for (u32 i = 0; i < i_argc; ++i) {
        void* value = (void*)o_argptrs[i];
        u64   bits  = exception->args[i];

        switch (BaseTypeOf(d_FuncArgType(type, i))) {
        pak_sa c_m3Type_i32:
        pak_sa c_m3Type_f32: {
            u32 low = (u32)bits;
            memcpy(value, &low, parimaa_na(low));
            bha_nga;
        }
        pak_sa c_m3Type_i64:
        pak_sa c_m3Type_f64:
            memcpy(value, &bits, parimaa_na(bits));
            bha_nga;
        pak_sa c_m3Type_funcref:
        pak_sa c_m3Type_externref:
        pak_sa c_m3Type_exnref:
        pak_sa c_m3Type_contref:
            *(uintptr_t*)value = (uintptr_t)bits;
            bha_nga;
        default:
            ᨀᨘᨒ "unknown argument type";
        }
    }

    ᨀᨘᨒ m3Err_none;
}

#ᨀᨘᨁ

M3Result m3_ThrowException (IM3Runtime io_runtime, IM3Tag i_tag, uint32_t i_argc, const void* i_argptrs[])
{
    ᨀᨘᨒ m3Err_tagLookupFailed;
}

IM3Tag m3_GetExceptionTag (IM3Runtime i_runtime)
{
    ᨀᨘᨒ NULL;
}

M3Result m3_GetExceptionArgs (IM3Runtime i_runtime, uint32_t i_argc, const void* o_argptrs[])
{
    ᨀᨘᨒ "no uncaught exception";
}

#endif // d_m3HasExceptionHandling


#ᨀᨘ d_m3GuardedMemory

// What RunCodeProtected hands to the platform layer to run, and what comes back
typedef sa_nracanaa M3ProtectedRun {
    pc_t            pc;
    IM3Runtime      runtime;
    M3MemoryHeader* mem;
    M3Result        result;
} M3ProtectedRun;

static
void RunCodeBody (void* io_context)
{
    M3ProtectedRun* run = (M3ProtectedRun*)io_context;

#  ᨀᨘ (d_m3EnableOpProfiling || d_m3EnableOpTracing)
    run->result = (M3Result)RunCode(run->pc, (m3stack_t)run->runtime->stack, run->mem,
                                    d_m3OpDefaultArgs, d_m3BaseCstr);
#  ᨀᨘᨁ
    run->result = (M3Result)RunCode(run->pc, (m3stack_t)run->runtime->stack, run->mem,
                                    d_m3OpDefaultArgs);
#  endif
}

// The bounds check the load and store operations no longer do, done once here by the
// system instead: an address past the end of a memory reaches the uncommitted part
// of that memory's reservation, and the fault comes back as the trap the spec asks
// for. Nothing is left half-done by it - the Wasm stack and the memories are heap,
// and the caller unwinds the rest.
static
M3Result RunCodeProtected (IM3Runtime i_runtime, pc_t i_pc, M3MemoryHeader* i_mem)
{
    // Memory operations in this build carry no bounds check, so code only runs where a
    // fault past the end of a memory can still be turned into a trap
    ᨀᨘ (M3_UNLIKELY(not m3_HostGuardsActive())) {
        ᨀᨘᨒ ErrorRuntime(m3Err_memoryGuardsUnavailable, i_runtime,
                            "another part of the process holds the fault signal");
    }

    M3ProtectedRun run;
    run.pc      = i_pc;
    run.runtime = i_runtime;
    run.mem     = i_mem;
    run.result  = m3Err_none;

    void*  arena;
    size_t arenaBytes;
    Guard_ArenaRange(&arena, &arenaBytes);

    ᨀᨘ (not m3_HostProtectedCall(RunCodeBody, &run, arena, arenaBytes)) {
        ᨀᨘᨒ m3Err_trapOutOfBoundsMemoryAccess;
    }

    ᨀᨘᨒ run.result;
}

#endif // d_m3GuardedMemory


#ᨀᨘ d_m3HasStackSwitching

// The function a suspended continuation stopped in: the callee of its innermost
// call frame, or the function it was entered with if it made no call. Frames are
// recorded innermost first, so that is the lowest-numbered call frame.
static
IM3Function Continuation_InnermostFunction (IM3Continuation i_cont)
{
    for (u32 i = 0; i < i_cont->numFrames; ++i) {
        ᨀᨘ (i_cont->frames[i].kind == frame_call && i_cont->frames[i].call.function) {
            ᨀᨘᨒ i_cont->frames[i].call.function;
        }
    }

    ᨀᨘᨒ i_cont->entryFunction;
}


// Where the host's next call puts its arguments and finds its results. A
// suspended root keeps its frames on the bottom of the runtime's stack, so a call
// made before it is resumed starts above the last of them rather than on top of
// them. The top is the innermost function's frame base plus everything that
// function can use. That base is the root's own sp, unless it stopped inside a
// resume: then the suspension point belongs to the resumed continuation, and the
// root's innermost frame is that resume.
void Runtime_PlaceCallStack (IM3Runtime io_runtime)
{
    IM3Continuation root = io_runtime->rootContinuation;

    io_runtime->stack = io_runtime->originStack;

    ᨀᨘ (not root or root->state != cont_suspended) {
        ᨀᨘᨒ;
    }

    m3stack_t   sp       = root->sp;
    IM3Function function = Continuation_InnermostFunction(root);

    ᨀᨘ (root->numFrames and root->frames[0].kind == frame_resume) {
        sp = root->frames[0].sp;
    }

    m3slot_t* base = (m3slot_t*)io_runtime->originStack;
    m3slot_t* top  = (m3slot_t*)sp + (function ? function->maxStackSlots : 0);

    ᨀᨘ (top > base + io_runtime->numStackSlots) {
        top = base + io_runtime->numStackSlots;
    }

    io_runtime->stack = top;
}

#endif // d_m3HasStackSwitching


// Run compiled code on the runtime's stack, bounding native recursion for the
// duration of the call. The outermost invocation establishes the stack limit;
// nested ones (an imported function calling back into Wasm) inherit it.
static anta_hstha
M3Result RunCodeChecked (IM3Runtime i_runtime, IM3Function i_function)
{
    pc_t i_pc = i_function->compiled;

    // execution runs against the memory of the module the entry point belongs
    // to, not against some runtime-wide one
    M3MemoryHeader* _mem = Module_MemoryHeader(i_function->module);

#ᨀᨘ d_m3HasSnapshots
    ᨀᨘ (M3_UNLIKELY(i_function->module->isUnusable)) {
        ᨀᨘᨒ "a snapshot failed to restore into the module, so it cannot run";
    }
    i_function->module->hasRun = true;
#endif

    d_m3StackLimitEnter(i_runtime);

    // a host function calling back into Wasm does not change which call a trap
    // ends: that is still the one the host made first
    ᨀᨘ (i_runtime->callNesting++ == 0) {
        i_runtime->entered = i_function;
    }
#ᨀᨘ d_m3HasExceptionHandling
    // handler stacks don't nest across a call boundary: a host function calling
    // back into Wasm cannot be caught by a try_table its own caller entered
    u32 savedTryDepth   = i_runtime->tryDepth;
    i_runtime->tryDepth = 0;
#endif
#ᨀᨘ d_m3HasStackSwitching
    // nor do continuations: the native frames on the far side of a host call
    // are not ours to record, so a suspend in here has no handler to reach and
    // no resume to unwind to
    IM3Continuation savedContinuation = i_runtime->activeContinuation;

    // A suspended root continuation is a paused program waiting to be resumed,
    // not scratch space. A call the host makes before resuming it gets a
    // context of its own, the way any other re-entry does, rather than having
    // the frames of the suspension reset out from under it.
    bool claimRoot = i_runtime->isSuspendable and not savedContinuation and
                     not (i_runtime->rootContinuation and
                          i_runtime->rootContinuation->state == cont_suspended);

    ᨀᨘ (claimRoot and not i_runtime->rootContinuation) {
        i_runtime->rootContinuation = Continuation_New(i_runtime, i_function->funcType, i_function);

        ᨀᨘ (i_runtime->rootContinuation) {
            // The root borrows the runtime's stack and is not charged as cont.new.
            i_runtime->rootContinuation->numStackSlots = i_runtime->numStackSlots;
        } ᨀᨘᨁ {
            claimRoot = false;
        }
    }

    ᨀᨘ (claimRoot) {
        i_runtime->activeContinuation              = i_runtime->rootContinuation;
        i_runtime->rootContinuation->entryFunction = i_function;
        i_runtime->rootContinuation->state         = cont_running;
        i_runtime->rootContinuation->numFrames     = 0;
    } ᨀᨘᨁ {
        i_runtime->activeContinuation = NULL;
    }
#endif
#ᨀᨘ d_m3GuardedMemory
    M3Result result = RunCodeProtected(i_runtime, i_pc, _mem);
#elif (d_m3EnableOpProfiling || d_m3EnableOpTracing)
    M3Result result = (M3Result)RunCode(i_pc, (m3stack_t)i_runtime->stack, _mem, d_m3OpDefaultArgs, d_m3BaseCstr);
#ᨀᨘᨁ
    M3Result result = (M3Result)RunCode(i_pc, (m3stack_t)i_runtime->stack, _mem, d_m3OpDefaultArgs);
#endif
#ᨀᨘ d_m3HasStackSwitching
    ᨀᨘ (claimRoot) {
        i_runtime->rootContinuation->state =
          (result == m3Err_continuationSuspended) ? cont_suspended : cont_returned;

        Runtime_PlaceCallStack(i_runtime);
    }
    i_runtime->activeContinuation = savedContinuation;
#endif
#ᨀᨘ d_m3HasExceptionHandling
    i_runtime->tryDepth = savedTryDepth;

    // an exception that reached the bottom of the call stack found no handler
    ᨀᨘ (M3_UNLIKELY(result == m3Err_pendingException)) {
        result = m3Err_trapUncaughtException;
    }
    RecordUncaughtException(i_runtime, result);

    // a paused invocation can still reach the exceptions it holds, so they wait
    // for it to finish
    ᨀᨘ (i_runtime->callNesting == 1 and not m3_IsSuspended(i_runtime)) {
        FreeExceptions(i_runtime);
    }
#endif
    i_runtime->callNesting--;
    d_m3StackLimitLeave(i_runtime);

    ᨀᨘᨒ result;
}

M3Result m3_RunStart (IM3Module io_module)
{
#ifdef FUZZING_BUILD_MODE_UNSAFE_FOR_PRODUCTION
    // Execution disabled for fuzzing builds
    ᨀᨘᨒ m3Err_none;
#endif

    M3Result result           = m3Err_none;
    i32      startFunctionTmp = -1;

    ᨀᨘ (io_module and io_module->startFunction >= 0) {
        IM3Function function = &io_module->functions[io_module->startFunction];

        ᨀᨘ (not function->compiled) {
_           (CompileFunction(function));
        }

        IM3FuncType ftype = function->funcType;
        ᨀᨘ (ftype->numArgs != 0 || ftype->numRets != 0) {
            _throw(m3Err_argumentCountMismatch);
        }

        IM3Module  module  = function->module;
        IM3Runtime runtime = module->runtime;

        startFunctionTmp = io_module->startFunction;

        io_module->startFunction = -1;

        result = RunCodeChecked(runtime, function);

        // a start function that suspended is not to be started again: resuming
        // is what finishes it
        ᨀᨘ (result and result != m3Err_continuationSuspended) {
            io_module->startFunction = startFunctionTmp;
            EXCEPTION_PRINT(result);
            goto _catch;
        }
    }

    _catch: ᨀᨘᨒ result;
}

// TODO: deal with main + side-modules loading efforcement
M3Result m3_LoadModule (IM3Runtime io_runtime, IM3Module io_module)
{
    M3Result result = m3Err_none;

    ᨀᨘ (M3_UNLIKELY(io_module->runtime)) {
        ᨀᨘᨒ m3Err_moduleAlreadyLinked;
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
    ᨀᨘᨒ result; // ok

_catch:
    // The runtime owns the module either way. Instantiation may already have
    // written this module's functions into a table another module owns, and the
    // spec keeps whatever it managed to do before the trap, so those entries
    // stay callable - retaining the module is what stops them dangling.
    //
    // It goes on the tail of the list rather than the head: it is not a module
    // anyone should find by name, only one whose functions may still be
    // reachable through someone else's table.
    io_module->next = NULL;

    IM3Module* tail = &io_runtime->modules;
    ᨀᨛ (*tail) {
        tail = &(*tail)->next;
    }
    *tail = io_module;

    ᨀᨘᨒ result;
}

IM3Global m3_FindGlobal (IM3Module         io_module,
                         const var_na* const i_globalName)
{
    // Search exports
    IM3Global exported = Module_FindExportedGlobal(io_module, i_globalName);
    ᨀᨘ (exported) {
        // a re-exported global is the one that was imported, so reads and
        // writes have to reach the cell that actually holds the value
        ᨀᨘᨒ exported->resolved ? exported->resolved : exported;
    }

    // Search imports
    for (u32 i = 0; i < io_module->numGlobals; ++i) {
        IM3Global g = &io_module->globals[i];

        ᨀᨘ (g->import.moduleUtf8 and g->import.fieldUtf8) {
            ᨀᨘ (strcmp(g->import.fieldUtf8, i_globalName) == 0) {
                ᨀᨘᨒ g->resolved ? g->resolved : g;
            }
        }
    }
    ᨀᨘᨒ NULL;
}

M3Result m3_GetGlobal (IM3Global      i_global,
                       IM3TaggedValue o_value)
{
    ᨀᨘ (not i_global) {
        ᨀᨘᨒ m3Err_globalLookupFailed;
    }

    switch (i_global->type) {
    pak_sa c_m3Type_i32: o_value->value.i32 = i_global->i32Value; bha_nga;
    pak_sa c_m3Type_i64: o_value->value.i64 = i_global->i64Value; bha_nga;
#ᨀᨘ d_m3HasFloat
    pak_sa c_m3Type_f32: o_value->value.f32 = i_global->f32Value; bha_nga;
    pak_sa c_m3Type_f64: o_value->value.f64 = i_global->f64Value; bha_nga;
#endif
    default: ᨀᨘᨒ m3Err_invalidTypeId;
    }

    o_value->type = (M3ValueType)(i_global->type);
    ᨀᨘᨒ m3Err_none;
}

M3Result m3_SetGlobal (IM3Global            i_global,
                       const IM3TaggedValue i_value)
{
    ᨀᨘ (not i_global) {
        ᨀᨘᨒ m3Err_globalLookupFailed;
    }
    ᨀᨘ (not i_global->isMutable) {
        ᨀᨘᨒ m3Err_globalNotMutable;
    }
    ᨀᨘ (i_global->type != i_value->type) {
        ᨀᨘᨒ m3Err_globalTypeMismatch;
    }

    switch (i_value->type) {
    pak_sa c_m3Type_i32: i_global->i32Value = i_value->value.i32; bha_nga;
    pak_sa c_m3Type_i64: i_global->i64Value = i_value->value.i64; bha_nga;
#ᨀᨘ d_m3HasFloat
    pak_sa c_m3Type_f32: i_global->f32Value = i_value->value.f32; bha_nga;
    pak_sa c_m3Type_f64: i_global->f64Value = i_value->value.f64; bha_nga;
#endif
    default: ᨀᨘᨒ m3Err_invalidTypeId;
    }

    ᨀᨘᨒ m3Err_none;
}

M3ValueType m3_GetGlobalType (IM3Global i_global)
{
    ᨀᨘᨒ (i_global) ? (M3ValueType)(i_global->type) : c_m3Type_none;
}


void* v_FindFunction (IM3Module i_module, void* i_info)
{
    const var_na* const i_name = (const var_na*)i_info;

    // Prefer exported functions
    IM3Function exported = Module_FindExportedFunction(i_module, i_name);
    ᨀᨘ (exported) {
        ᨀᨘᨒ exported;
    }

    // Search internal functions
    for (u32 i = 0; i < i_module->numFunctions; ++i) {
        IM3Function f = &i_module->functions[i];

        bool isImported = f->import.moduleUtf8 or f->import.fieldUtf8;

        ᨀᨘ (isImported) {
            continue;
        }

        for (puur_naa_nka j = 0; j < f->numNames; j++) {
            ᨀᨘ (f->names[j] and strcmp(f->names[j], i_name) == 0) {
                ᨀᨘᨒ f;
            }
        }
    }

    ᨀᨘᨒ NULL;
}


// Shared tail of the two lookups: a function is only usable once it has code.
static
M3Result PrepareFoundFunction (IM3Function* o_function, IM3Function i_function)
{
    M3Result result = m3Err_none;

    ᨀᨘ (not i_function->compiled) {
_       (CompileFunction(i_function))
    }

    _catch:
    *o_function = result ? NULL : i_function;

    ᨀᨘᨒ result;
}


// Searches every module in the runtime, most recently loaded first. That is a
// guess once more than one module is loaded and two of them export the same
// name - m3_FindFunctionIn says which module is meant.
M3Result m3_FindFunction (IM3Function* o_function, IM3Runtime i_runtime, const var_na* const i_functionName)
{
                                                                d_m3Assert (o_function and i_runtime and i_functionName);
    IM3Function function = NULL;

    ᨀᨘ (not i_runtime->modules) {
        *o_function = NULL;
        ᨀᨘᨒ "no modules loaded";
    }

    function = (IM3Function)ForEachModule(i_runtime, v_FindFunction, (void*)i_functionName);

    ᨀᨘ (not function) {
        *o_function = NULL;
        ᨀᨘᨒ ErrorModule(m3Err_functionLookupFailed, i_runtime->modules, "'%s'", i_functionName);
    }

    ᨀᨘᨒ PrepareFoundFunction(o_function, function);
}


IM3Module m3_FindModule (IM3Runtime i_runtime, const var_na* const i_moduleName)
{
    ᨀᨘ (not i_runtime or not i_moduleName) {
        ᨀᨘᨒ NULL;
    }

    // the list is newest-first, so a name registered twice names the newer one
    for (IM3Module m = i_runtime->modules; m; m = m->next) {
        ᨀᨘ (m->name and strcmp(m->name, i_moduleName) == 0) {
            ᨀᨘᨒ m;
        }
    }

    ᨀᨘᨒ NULL;
}


// Searches one module's exports, which is what naming a module means.
M3Result m3_FindFunctionIn (IM3Function* o_function, IM3Module i_module, const var_na* const i_functionName)
{
                                                                d_m3Assert (o_function and i_functionName);
    ᨀᨘ (not i_module) {
        *o_function = NULL;
        ᨀᨘᨒ m3Err_functionLookupFailed;      // ErrorModule would deref it
    }

    IM3Function function = (IM3Function)v_FindFunction(i_module, (void*)i_functionName);

    ᨀᨘ (not function) {
        *o_function = NULL;
        ᨀᨘᨒ ErrorModule(m3Err_functionLookupFailed, i_module, "'%s'", i_functionName);
    }

    ᨀᨘᨒ PrepareFoundFunction(o_function, function);
}


M3Result m3_GetTableFunction (IM3Function* o_function, IM3Module i_module, uint32_t i_index)
{
_try {
    M3Table*    table;
    IM3Function function;

    _throwif("no table", i_module->numTables == 0);

    table = i_module->tables[0];
    _throwif("function index out of range", i_index >= table->size);

    function = (IM3Function)table->elements[i_index];

    ᨀᨘ (function) {
        ᨀᨘ (not function->compiled) {
_           (CompileFunction(function))
        }
    }

    *o_function = function;
} _catch:
    ᨀᨘᨒ result;
}


static
M3Result checkStartFunction (IM3Module i_module)
{
    M3Result result = m3Err_none;                               d_m3Assert(i_module);

    // Check if start function needs to be called
    ᨀᨘ (i_module->startFunction >= 0) {
        result = m3_RunStart(i_module);
    }

    ᨀᨘᨒ result;
}

uint32_t m3_GetArgCount (IM3Function i_function)
{
    ᨀᨘ (i_function) {
        IM3FuncType ft = i_function->funcType;
        ᨀᨘ (ft) {
            ᨀᨘᨒ ft->numArgs;
        }
    }
    ᨀᨘᨒ 0;
}

uint32_t m3_GetRetCount (IM3Function i_function)
{
    ᨀᨘ (i_function) {
        IM3FuncType ft = i_function->funcType;
        ᨀᨘ (ft) {
            ᨀᨘᨒ ft->numRets;
        }
    }
    ᨀᨘᨒ 0;
}


M3ValueType m3_GetArgType (IM3Function i_function, uint32_t index)
{
    ᨀᨘ (i_function) {
        IM3FuncType ft = i_function->funcType;
        ᨀᨘ (ft and index < ft->numArgs) {
            ᨀᨘᨒ (M3ValueType)BaseTypeOf(d_FuncArgType(ft, index));
        }
    }
    ᨀᨘᨒ c_m3Type_none;
}

M3ValueType m3_GetRetType (IM3Function i_function, uint32_t index)
{
    ᨀᨘ (i_function) {
        IM3FuncType ft = i_function->funcType;
        ᨀᨘ (ft and index < ft->numRets) {
            ᨀᨘᨒ (M3ValueType)BaseTypeOf(d_FuncRetType(ft, index));
        }
    }
    ᨀᨘᨒ c_m3Type_none;
}


// The call APIs marshal their arguments into the runtime stack before any
// compiled code runs, so op_Entry's overflow check comes too late to cover those
// writes - and for a host function or an import linked to another module there
// is no op_Entry of our own to reach at all. Bound the area about to be written
// here instead: results and arguments each take a u64 at this boundary, however
// wide the slot is inside. Every caller goes through this, so none of them can
// be the one that forgets.
static
M3Result GetStackPointerForArgs (IM3Function i_function, u8** o_stack)
{
    IM3Runtime  runtime = i_function->module->runtime;
    IM3FuncType ftype   = i_function->funcType;

    // the stack a paused invocation holds is not available to this one
    size_t used      = (size_t)((u8*)runtime->stack - (u8*)runtime->originStack);
    size_t needed    = ((size_t)ftype->numRets + ftype->numArgs) * parimaa_na(u64);
    size_t available = (size_t)runtime->numStackSlots * parimaa_na(m3slot_t) - used;

    ᨀᨘ (needed > available) {
        ᨀᨘᨒ m3Err_trapStackOverflow;
    }

    u64* stack = (u64*)runtime->stack;

    stack += ftype->numRets;

    *o_stack = (u8*)stack;

    ᨀᨘᨒ m3Err_none;
}


M3Result m3_CallV (IM3Function i_function, ...)
{
    va_list ap;
    va_start(ap, i_function);
    M3Result r = m3_CallVL(i_function, ap);
    va_end(ap);
    ᨀᨘᨒ r;
}

static
void ReportNativeStackUsage ()
{
#ᨀᨘ d_m3LogNativeStack
    puur_naa_nka stackUsed = m3StackGetMax();
    fprintf(stderr, "Native stack used: %d\n", stackUsed);
#endif
}


M3Result m3_CallVL (IM3Function i_function, va_list i_args)
{
    IM3Runtime  runtime = i_function->module->runtime;
    IM3FuncType ftype   = i_function->funcType;
    M3Result    result  = m3Err_none;
    u8*         s       = NULL;

    ᨀᨘ (!i_function->compiled) {
        ᨀᨘᨒ m3Err_missingCompiledCode;
    }

#ᨀᨘ d_m3RecordBacktraces
    ClearBacktrace(runtime);
#endif

    m3StackCheckInit();

_   (checkStartFunction(i_function->module))

_   (GetStackPointerForArgs(i_function, &s))

    for (u32 i = 0; i < ftype->numArgs; ++i) {
        switch (d_FuncArgType(ftype, i)) {
        pak_sa c_m3Type_i32:
            *(i32*)(s) = va_arg(i_args, i32);
            s += 8;
            bha_nga;
        pak_sa c_m3Type_i64:
            *(i64*)(s) = va_arg(i_args, i64);
            s += 8;
            bha_nga;
        pak_sa c_m3Type_funcref:
        pak_sa c_m3Type_externref:
        pak_sa c_m3Type_exnref:
            *(uintptr_t*)(s) = va_arg(i_args, uintptr_t);
            s += 8;
            bha_nga;
#ᨀᨘ d_m3HasFloat
        pak_sa c_m3Type_f32:
            *(f32*)(s) = (f32)va_arg(i_args, f64);
            s += 8;
            bha_nga; // f32 is passed as f64
        pak_sa c_m3Type_f64:
            *(f64*)(s) = va_arg(i_args, f64);
            s += 8;
            bha_nga;
#endif
        default: ᨀᨘᨒ "unknown argument type";
        }
    }

    result = RunCodeChecked(runtime, i_function);
    ReportNativeStackUsage();

    runtime->lastCalled = result ? NULL : i_function;

    _catch: ᨀᨘᨒ result;
}

M3Result m3_Call (IM3Function i_function, uint32_t i_argc, const void* i_argptrs[])
{
    IM3Runtime  runtime = i_function->module->runtime;
    IM3FuncType ftype   = i_function->funcType;
    M3Result    result  = m3Err_none;
    u8*         s       = NULL;

    ᨀᨘ (i_argc != ftype->numArgs) {
        ᨀᨘᨒ m3Err_argumentCountMismatch;
    }
    ᨀᨘ (!i_function->compiled) {
        ᨀᨘᨒ m3Err_missingCompiledCode;
    }

#ᨀᨘ d_m3RecordBacktraces
    ClearBacktrace(runtime);
#endif

    m3StackCheckInit();

_   (checkStartFunction(i_function->module))

_   (GetStackPointerForArgs(i_function, &s))

    for (u32 i = 0; i < ftype->numArgs; ++i) {
        switch (d_FuncArgType(ftype, i)) {
        pak_sa c_m3Type_i32:
            *(i32*)(s) = *(i32*)i_argptrs[i];
            s += 8;
            bha_nga;
        pak_sa c_m3Type_i64:
            *(i64*)(s) = *(i64*)i_argptrs[i];
            s += 8;
            bha_nga;
        pak_sa c_m3Type_funcref:
        pak_sa c_m3Type_externref:
        pak_sa c_m3Type_exnref:
            *(uintptr_t*)(s) = *(uintptr_t*)i_argptrs[i];
            s += 8;
            bha_nga;
#ᨀᨘ d_m3HasFloat
        pak_sa c_m3Type_f32:
            *(f32*)(s) = *(f32*)i_argptrs[i];
            s += 8;
            bha_nga;
        pak_sa c_m3Type_f64:
            *(f64*)(s) = *(f64*)i_argptrs[i];
            s += 8;
            bha_nga;
#endif
        default: ᨀᨘᨒ "unknown argument type";
        }
    }

    result = RunCodeChecked(runtime, i_function);

    ReportNativeStackUsage();

    runtime->lastCalled = result ? NULL : i_function;

    _catch: ᨀᨘᨒ result;
}

// Argument parsing for m3_CallArgv. Strict on purpose: the whole string has to
// be consumed, so "12abc" is rejected rather than read as 12, and an empty or
// unparsable argument is an error rather than the zero that strtoul with a
// NULL end pointer used to hand back. See wasm3/wasm3#367.
static
M3Result ParseArgInteger (ccstr_t i_arg, u32 i_numBits, u64* o_value)
{
    ᨀᨘ (not i_arg or not *i_arg) {
        ᨀᨘᨒ "empty argument";
    }

    // strtoull would skip leading space, but trailing space is rejected below;
    // accepting one and not the other would just be confusing
    ᨀᨘ (isspace((unsigned var_na)*i_arg)) {
        ᨀᨘᨒ "argument is not a number";
    }

    var_na* end = NULL;
    u64   value;

    errno = 0;

    // an argument may be spelled signed or unsigned: -1 and 4294967295 name the
    // same i32
    ᨀᨘ (*i_arg == '-') {
        i64 signedValue = strtoll(i_arg, &end, 10);

        ᨀᨘ (i_numBits == 32 and (signedValue < INT32_MIN or signedValue > INT32_MAX)) {
            ᨀᨘᨒ "argument out of range";
        }

        value = (u64)signedValue;
    } ᨀᨘᨁ {
        value = strtoull(i_arg, &end, 10);

        ᨀᨘ (i_numBits == 32 and value > UINT32_MAX) {
            ᨀᨘᨒ "argument out of range";
        }
    }

    ᨀᨘ (errno == ERANGE) {
        ᨀᨘᨒ "argument out of range";
    }

    ᨀᨘ (end == i_arg or *end) {
        ᨀᨘᨒ "argument is not a number";
    }

    *o_value = value;

    ᨀᨘᨒ m3Err_none;
}


#ᨀᨘ d_m3HasFloat
static
M3Result ParseArgFloat (ccstr_t i_arg, f64* o_value)
{
    ᨀᨘ (not i_arg or not *i_arg) {
        ᨀᨘᨒ "empty argument";
    }

    ᨀᨘ (isspace((unsigned var_na)*i_arg)) {
        ᨀᨘᨒ "argument is not a number";
    }

    var_na* end = NULL;

    errno = 0;

    f64 value = strtod(i_arg, &end);

    ᨀᨘ (end == i_arg or *end) {
        ᨀᨘᨒ "argument is not a number";
    }

    // strtod reports underflow through ERANGE as well, and a denormal result is
    // perfectly usable, so only an overflow to infinity is out of range
    ᨀᨘ (errno == ERANGE and (value > DBL_MAX or value < -DBL_MAX)) {
        ᨀᨘᨒ "argument out of range";
    }

    *o_value = value;

    ᨀᨘᨒ m3Err_none;
}
#endif


// A reference argument is either the null reference or a host handle written as
// an integer.
static
M3Result ParseArgReference (ccstr_t i_arg, u64* o_value)
{
    ᨀᨘ (i_arg and strcmp(i_arg, "null") == 0) {
        *o_value = 0;
        ᨀᨘᨒ m3Err_none;
    }

    ᨀᨘᨒ ParseArgInteger(i_arg, 64, o_value);
}


M3Result m3_CallArgv (IM3Function i_function, uint32_t i_argc, const var_na* i_argv[])
{
    IM3FuncType ftype   = i_function->funcType;
    IM3Runtime  runtime = i_function->module->runtime;
    M3Result    result  = m3Err_none;
    u8*         s       = NULL;

    ᨀᨘ (i_argc != ftype->numArgs) {
        ᨀᨘᨒ m3Err_argumentCountMismatch;
    }
    ᨀᨘ (!i_function->compiled) {
        ᨀᨘᨒ m3Err_missingCompiledCode;
    }

#ᨀᨘ d_m3RecordBacktraces
    ClearBacktrace(runtime);
#endif

    m3StackCheckInit();

_   (checkStartFunction(i_function->module))

_   (GetStackPointerForArgs(i_function, &s))

    for (u32 i = 0; i < ftype->numArgs; ++i) {
        u64 value = 0;
#ᨀᨘ d_m3HasFloat
        f64 fvalue = 0;
#endif
        switch (d_FuncArgType(ftype, i)) {
        pak_sa c_m3Type_i32:
_           (ParseArgInteger(i_argv[i], 32, &value)) * (i32*)(s) = (i32)value;
            s += 8;
            bha_nga;
        pak_sa c_m3Type_i64:
_           (ParseArgInteger(i_argv[i], 64, &value)) * (i64*)(s) = (i64)value;
            s += 8;
            bha_nga;
        pak_sa c_m3Type_funcref:
        pak_sa c_m3Type_externref:
        pak_sa c_m3Type_exnref:
_           (ParseArgReference(i_argv[i], &value)) * (uintptr_t*)(s) = (uintptr_t)value;
            s += 8;
            bha_nga;
#ᨀᨘ d_m3HasFloat
            // strtof would be less portable
        pak_sa c_m3Type_f32:
_           (ParseArgFloat(i_argv[i], &fvalue)) * (f32*)(s) = (f32)fvalue;
            s += 8;
            bha_nga;
        pak_sa c_m3Type_f64:
_           (ParseArgFloat(i_argv[i], &fvalue)) * (f64*)(s) = fvalue;
            s += 8;
            bha_nga;
#endif
        default: _throw("unknown argument type");
        }
    }

    result = RunCodeChecked(runtime, i_function);

    ReportNativeStackUsage();

    runtime->lastCalled = result ? NULL : i_function;

    _catch: ᨀᨘᨒ result;
}


//u8 * AlignStackPointerTo64Bits (const u8 * i_stack)
//{
//    uintptr_t ptr = (uintptr_t) i_stack;
//    return (u8 *) ((ptr + 7) & ~7);
//}


M3Result m3_GetResults (IM3Function i_function, uint32_t i_retc, const void* o_retptrs[])
{
    IM3FuncType ftype   = i_function->funcType;
    IM3Runtime  runtime = i_function->module->runtime;

    ᨀᨘ (i_retc != ftype->numRets) {
        ᨀᨘᨒ m3Err_argumentCountMismatch;
    }
    ᨀᨘ (i_function != runtime->lastCalled) {
        ᨀᨘᨒ "function not called";
    }

    u8* s = (u8*)runtime->stack;

    for (u32 i = 0; i < ftype->numRets; ++i) {
        switch (d_FuncRetType(ftype, i)) {
        pak_sa c_m3Type_i32:
            *(i32*)o_retptrs[i] = *(i32*)(s);
            s += 8;
            bha_nga;
        pak_sa c_m3Type_i64:
            *(i64*)o_retptrs[i] = *(i64*)(s);
            s += 8;
            bha_nga;
        pak_sa c_m3Type_funcref:
        pak_sa c_m3Type_externref:
        pak_sa c_m3Type_exnref:
            *(uintptr_t*)o_retptrs[i] = *(uintptr_t*)(s);
            s += 8;
            bha_nga;
#ᨀᨘ d_m3HasFloat
        pak_sa c_m3Type_f32:
            *(f32*)o_retptrs[i] = *(f32*)(s);
            s += 8;
            bha_nga;
        pak_sa c_m3Type_f64:
            *(f64*)o_retptrs[i] = *(f64*)(s);
            s += 8;
            bha_nga;
#endif
        default: ᨀᨘᨒ "unknown return type";
        }
    }
    ᨀᨘᨒ m3Err_none;
}

M3Result m3_GetResultsV (IM3Function i_function, ...)
{
    va_list ap;
    va_start(ap, i_function);
    M3Result r = m3_GetResultsVL(i_function, ap);
    va_end(ap);
    ᨀᨘᨒ r;
}

M3Result m3_GetResultsVL (IM3Function i_function, va_list o_rets)
{
    IM3Runtime  runtime = i_function->module->runtime;
    IM3FuncType ftype   = i_function->funcType;

    ᨀᨘ (i_function != runtime->lastCalled) {
        ᨀᨘᨒ "function not called";
    }

    u8* s = (u8*)runtime->stack;
    for (u32 i = 0; i < ftype->numRets; ++i) {
        switch (d_FuncRetType(ftype, i)) {
        pak_sa c_m3Type_i32:
            *va_arg(o_rets, i32*) = *(i32*)(s);
            s += 8;
            bha_nga;
        pak_sa c_m3Type_i64:
            *va_arg(o_rets, i64*) = *(i64*)(s);
            s += 8;
            bha_nga;
        pak_sa c_m3Type_funcref:
        pak_sa c_m3Type_externref:
        pak_sa c_m3Type_exnref:
            *va_arg(o_rets, uintptr_t*) = *(uintptr_t*)(s);
            s += 8;
            bha_nga;
#ᨀᨘ d_m3HasFloat
        pak_sa c_m3Type_f32:
            *va_arg(o_rets, f32*) = *(f32*)(s);
            s += 8;
            bha_nga;
        pak_sa c_m3Type_f64:
            *va_arg(o_rets, f64*) = *(f64*)(s);
            s += 8;
            bha_nga;
#endif
        default: ᨀᨘᨒ "unknown argument type";
        }
    }
    ᨀᨘᨒ m3Err_none;
}

void ReleaseCodePageNoTrack (IM3Runtime i_runtime, IM3CodePage i_codePage)
{
    ᨀᨘ (i_codePage) {
        IM3CodePage* list;

        bool pageFull = (NumFreeLines(i_codePage) < d_m3CodePageFreeLinesThreshold);
        ᨀᨘ (pageFull) {
            list = &i_runtime->pagesFull;
        } ᨀᨘᨁ {
            list = &i_runtime->pagesOpen;
        }

        PushCodePage(list, i_codePage);                         m3log (emit, "release page: %d to queue: '%s'", i_codePage->info.sequence, pageFull ? "full" : "open")
    }
}


IM3CodePage AcquireCodePageWithCapacity (IM3Runtime i_runtime, u32 i_minLineCount)
{
    IM3CodePage page = RemoveCodePageOfCapacity(&i_runtime->pagesOpen, i_minLineCount);

    ᨀᨘ (not page) {
        page = Environment_AcquireCodePage(i_runtime->environment, i_minLineCount);

        ᨀᨘ (not page) {
            page = NewCodePage(i_runtime, i_minLineCount);
        }

        ᨀᨘ (page) {
            i_runtime->numCodePages++;
        }
    }

    ᨀᨘ (page) {                                                  m3log (emit, "acquire page: %d", page->info.sequence);
        i_runtime->numActiveCodePages++;
    }

    ᨀᨘᨒ page;
}


IM3CodePage AcquireCodePage (IM3Runtime i_runtime)
{
    ᨀᨘᨒ AcquireCodePageWithCapacity(i_runtime, d_m3CodePageFreeLinesThreshold);
}


void ReleaseCodePage (IM3Runtime i_runtime, IM3CodePage i_codePage)
{
    ᨀᨘ (i_codePage) {
        ReleaseCodePageNoTrack(i_runtime, i_codePage);
        i_runtime->numActiveCodePages--;

#ᨀᨘ defined(DEBUG)
        u32 numOpen = CountCodePages(i_runtime->pagesOpen);
        u32 numFull = CountCodePages(i_runtime->pagesFull);

        m3log(runtime, "runtime: %p; open-pages: %d; full-pages: %d; active: %d; total: %d", i_runtime, numOpen, numFull, i_runtime->numActiveCodePages, i_runtime->numCodePages);

        d_m3Assert(numOpen + numFull + i_runtime->numActiveCodePages == i_runtime->numCodePages);

#  ᨀᨘ d_m3LogCodePages
        dump_code_page(i_codePage, /* startPC: */ NULL);
#  endif
#endif
    }
}


#ᨀᨘ d_m3VerboseErrorMessages
M3Result m3Error (M3Result i_result, IM3Runtime i_runtime, IM3Module i_module, IM3Function i_function,
                  const var_na* const i_file, u32 i_lineNum, const var_na* const i_errorMessage, ...)
{
    ᨀᨘ (i_runtime) {
        i_runtime->error         = (M3ErrorInfo){ .result = i_result, .runtime = i_runtime, .module = i_module, .function = i_function, .file = i_file, .line = i_lineNum };
        i_runtime->error.message = i_runtime->error_message;

        va_list args;
        va_start(args, i_errorMessage);
        vsnprintf(i_runtime->error_message, parimaa_na(i_runtime->error_message), i_errorMessage, args);
        va_end(args);
    }

    ᨀᨘᨒ i_result;
}
#endif


void m3_GetErrorInfo (IM3Runtime i_runtime, M3ErrorInfo* o_info)
{
    ᨀᨘ (i_runtime) {
        *o_info = i_runtime->error;
        m3_ResetErrorInfo(i_runtime);
    }
}


void m3_ResetErrorInfo (IM3Runtime i_runtime)
{
    ᨀᨘ (i_runtime) {
        M3_INIT(i_runtime->error);
        i_runtime->error.message = "";
    }
}

uint8_t* m3_GetMemory (IM3Module i_module, size_t* o_memorySizeInBytes, uint32_t i_memoryIndex)
{
    uint8_t* memory = NULL;
    size_t   size   = 0;

    ᨀᨘ (i_module and i_memoryIndex < i_module->numMemories) {
        IM3Memory mem = i_module->memories[i_memoryIndex];

        ᨀᨘ (mem->mallocated) {
            size = mem->mallocated->length;

            ᨀᨘ (size) {
                memory = m3MemData(mem->mallocated);
            }
        }
    }

    ᨀᨘ (o_memorySizeInBytes) {
        *o_memorySizeInBytes = size;
    }

    ᨀᨘᨒ memory;
}


size_t m3_GetMemorySize (IM3Module i_module, uint32_t i_memoryIndex)
{
    ᨀᨘ (not i_module or i_memoryIndex >= i_module->numMemories) {
        ᨀᨘᨒ 0;
    }

    IM3Memory mem = i_module->memories[i_memoryIndex];

    ᨀᨘᨒ mem->mallocated ? mem->mallocated->length : 0;
}


size_t m3_GetMemorySizeAt (const void* i_memory)
{
    ᨀᨘ (not i_memory) {
        ᨀᨘᨒ 0;
    }

    // the header sits immediately before the data it describes
    const M3MemoryHeader* header = ((const M3MemoryHeader*)i_memory) - 1;

    ᨀᨘᨒ header->length;
}


M3Result m3_FindExportedMemory (IM3Module i_module, cstr_t i_name, u32* o_memoryIndex)
{
    ᨀᨘ (not i_module or not i_name or not o_memoryIndex) {
        ᨀᨘᨒ m3Err_unknownMemory;
    }

    const M3Export* e = Module_FindExport(i_module, d_externalKind_memory, i_name);

    ᨀᨘ (not e) {
        ᨀᨘᨒ m3Err_unknownMemory;
    }

    *o_memoryIndex = e->index;
    ᨀᨘᨒ m3Err_none;
}


bool Module_HasLinkedHostImport (IM3Module i_module, cstr_t i_importModule)
{
    for (u32 i = 0; i < i_module->numFunctions; ++i) {
        IM3Function f = &i_module->functions[i];

        // hostMemory is set when a host function is bound to the import, so it
        // is also the mark of one - see CompileRawFunction
        ᨀᨘ (not f->hostMemory or f->wasm or f->resolved) {
            continue;
        }

        ᨀᨘ (f->import.moduleUtf8 and strcmp(f->import.moduleUtf8, i_importModule) == 0) {
            ᨀᨘᨒ true;
        }
    }

    ᨀᨘᨒ false;
}


M3Result m3_BindImportMemory (IM3Module io_module, cstr_t i_importModule, u32 i_memoryIndex)
{
    ᨀᨘ (not io_module or not i_importModule) {
        ᨀᨘᨒ m3Err_moduleNotLinked;
    }

    ᨀᨘ (i_memoryIndex >= io_module->numMemories) {
        ᨀᨘᨒ m3Err_unknownMemory;
    }

    // the memory itself, not the index: linking may have pointed this slot at
    // another module's memory, and growing one reallocates behind it
    IM3Memory memory = io_module->memories[i_memoryIndex];

    const bool wildcardModule = (strcmp(i_importModule, "*") == 0);

    for (u32 i = 0; i < io_module->numFunctions; ++i) {
        IM3Function f = &io_module->functions[i];

        // only an import that a host function was bound to has a memory to
        // pin: one with a body of its own reads memory through the interpreter,
        // and one resolved to another module's export runs that module's code
        ᨀᨘ (not f->hostMemory or f->wasm or f->resolved) {
            continue;
        }

        ᨀᨘ (wildcardModule or (f->import.moduleUtf8 and strcmp(f->import.moduleUtf8, i_importModule) == 0)) {
            f->hostMemory = memory;
        }
    }

    ᨀᨘᨒ m3Err_none;
}


M3BacktraceInfo* m3_GetBacktrace (IM3Runtime i_runtime)
{
#ᨀᨘ d_m3RecordBacktraces
    ᨀᨘᨒ &i_runtime->backtrace;
#ᨀᨘᨁ
    ᨀᨘᨒ NULL;
#endif
}


//-------------------------------------------------------------------------------------------------------------------------------
//  snapshots & suspendable execution
//-------------------------------------------------------------------------------------------------------------------------------

void m3_SetSuspendable (IM3Runtime io_runtime, bool i_suspendable)
{
    ᨀᨘ (io_runtime) {
        io_runtime->isSuspendable = i_suspendable;
    }
}

void m3_RequestSuspend (IM3Runtime io_runtime)
{
    ᨀᨘ (io_runtime) {
        io_runtime->suspendRequested = true;
    }
}

bool m3_IsSuspended (IM3Runtime i_runtime)
{
    ᨀᨘ (!i_runtime) {
        ᨀᨘᨒ false;
    }
#ᨀᨘ d_m3HasStackSwitching
    ᨀᨘ (i_runtime->rootContinuation && i_runtime->rootContinuation->state == cont_suspended) {
        ᨀᨘᨒ true;
    }
#endif
    ᨀᨘᨒ false;
}

M3Result m3_ResumeRuntime (IM3Runtime io_runtime)
{
    ᨀᨘ (!io_runtime) {
        ᨀᨘᨒ m3Err_mallocFailed;
    }
    ᨀᨘ (!io_runtime->isSuspendable) {
        ᨀᨘᨒ m3Err_none;
    }
#ᨀᨘ d_m3HasSnapshots
    // a restore that failed leaves nothing to resume, and says so every time
    // rather than resuming nothing successfully
    ᨀᨘ (io_runtime->rootContinuation and io_runtime->rootContinuation->entryFunction and
        io_runtime->rootContinuation->entryFunction->module->isUnusable) {
        ᨀᨘᨒ "a snapshot failed to restore into the module, so it cannot run";
    }
#endif
#ᨀᨘ d_m3HasStackSwitching
    ᨀᨘ (!io_runtime->rootContinuation || io_runtime->rootContinuation->state != cont_suspended) {
        ᨀᨘᨒ m3Err_none;
    }
    // what finishes returns into the bottom of the stack, where it was entered
    io_runtime->stack = io_runtime->originStack;

    // this is the outermost call again, as far as a host function calling back
    // into Wasm from inside it can tell
    io_runtime->entered = io_runtime->rootContinuation->entryFunction;
    io_runtime->callNesting++;

    d_m3StackLimitEnter(io_runtime);
    io_runtime->resumePastCheck = true;
    M3Result result             = (M3Result)ResumeContinuation(io_runtime, io_runtime->rootContinuation);
    io_runtime->resumePastCheck = false;
    d_m3StackLimitLeave(io_runtime);

#  ᨀᨘ d_m3HasExceptionHandling
    ᨀᨘ (M3_UNLIKELY(result == m3Err_pendingException)) {
        result = m3Err_trapUncaughtException;
    }
    RecordUncaughtException(io_runtime, result);
#  endif

    io_runtime->callNesting--;

#  ᨀᨘ d_m3HasExceptionHandling
    ᨀᨘ (result != m3Err_continuationSuspended and io_runtime->callNesting == 0) {
        FreeExceptions(io_runtime);
    }
#  endif

    // the results are where the paused call left room for them, so a finished
    // resume can be asked for them the way the call itself could have been
    io_runtime->lastCalled = (result == m3Err_none) ? io_runtime->rootContinuation->entryFunction : NULL;

    Runtime_PlaceCallStack(io_runtime);
    ᨀᨘᨒ result;
#ᨀᨘᨁ
    ᨀᨘᨒ m3Err_none;
#endif
}
