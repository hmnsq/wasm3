//
//  m3_env.c
//
//  Created by Steven Massey on 4/19/19.
//  Copyright © 2019 Steven Massey. All rights reserved.
//

#شامل <stdarg.h>
#شامل <limits.h>
#شامل <errno.h>
#شامل <float.h>
#شامل <ctype.h>

#شامل "m3_env.h"
#شامل "m3_bind.h"
#شامل "m3_deterministic.h"
#شامل "m3_compile.h"
#شامل "m3_host.h"
#شامل "m3_exception.h"
#شامل "m3_info.h"


IM3Environment m3_NewEnvironment ()
{
    IM3Environment env = m3_AllocStruct(M3Environment);

    اگر (env) {
        _try
        {
            // create FuncTypes for all simple block return ValueTypes.
            // v128 is skipped: it parses as a slot but has no operations.
            برای (u8 t = c_m3Type_none; t < c_m3_Type_شمار; t++) {
                اگر (t == c_m3Type_v128) {
                    ادامه;
                }

                IM3FuncType ftype;
_               (AllocFuncType(&ftype, 1));

                ftype->رقم_Args  = 0;
                ftype->رقم_Rets  = (t == c_m3Type_none) ? 0 : 1;
                ftype->types[0] = t;

                Environment_AddFuncType(env, &ftype);

                env->retFuncTypes[t] = ftype;
            }
        }

        _catch:
        اگر (result) {
            m3_FreeEnvironment(env);
            env = پوچ;
        }
    }

    بازگشت env;
}


تهی Environment_Release (IM3Environment i_environment)
{
    IM3FuncType ftype = i_environment->funcTypes;

    وقتی (ftype) {
        IM3FuncType next = ftype->next;
        m3_Free(ftype);
        ftype = next;
    }

    m3log(runtime, "freeing %d pages from environment", شمار_Code_Pages(i_environment->pagesReleased));
    FreeCodePages(&i_environment->pagesReleased);
}


تهی m3_FreeEnvironment (IM3Environment i_environment)
{
    اگر (i_environment) {
        Environment_Release(i_environment);
        m3_Free(i_environment);
    }
}


تهی m3_SetCustomSectionHandler (IM3Environment i_environment, M3SectionHandler i_handler)
{
    اگر (i_environment) {
        i_environment->customSectionHandler = i_handler;
    }
}


// returns the same io_funcType or replaces it with an equivalent that's already in the type linked list
M3Result Environment_AddFuncType (IM3Environment i_environment, IM3FuncType* io_funcType)
{
    IM3FuncType addType = *io_funcType;
    IM3FuncType newType = i_environment->funcTypes;

    وقتی (newType) {
        اگر (not newType->inRecGroup and AreFuncTypesEqual(newType, addType)) {
            m3_Free(addType);
            بشکن;
        }

        newType = newType->next;
    }

    اگر (newType == پوچ) {
        // a type index has to fit in the heap type field of an m3type_t
        اگر (i_environment->رقم_Func_Types >= d_m3_Max_Sane_Types_شمار) {
            m3_Free(addType);
            *io_funcType = پوچ;
            بازگشت "too many distinct function types";
        }

        newType                  = addType;
        newType->canonicalIndex  = i_environment->رقم_Func_Types++;
        newType->next            = i_environment->funcTypes;
        i_environment->funcTypes = newType;
    }

    *io_funcType = newType;

    بازگشت m3Err_none;
}


// Adds a type without looking for an equal one first, and hands back the index
// it was given.
//
// Members of a recursive group do not get to share an entry with a type that
// merely looks like them: the group is their identity, so two groups spelled
// the same way are still two groups, and a type inside one is never the type
// outside it that matches field for field. Reserving the indices up front is
// also what lets the members refer to each other while they are being read.
M3Result Environment_ReserveFuncTypes (IM3Environment i_environment, u32 i_شمار, u16* o_firstIndex)
{
    اگر ((u64)i_environment->رقم_Func_Types + i_شمار > d_m3_Max_Sane_Types_شمار) {
        بازگشت "too many distinct function types";
    }

    *o_firstIndex = i_environment->رقم_Func_Types;
    i_environment->رقم_Func_Types += (u16)i_شمار;

    بازگشت m3Err_none;
}


تهی Environment_AdoptFuncType (IM3Environment i_environment, IM3FuncType i_funcType, u16 i_canonicalIndex)
{
    i_funcType->inRecGroup     = true;
    i_funcType->canonicalIndex = i_canonicalIndex;
    i_funcType->next           = i_environment->funcTypes;
    i_environment->funcTypes   = i_funcType;
}


IM3CodePage RemoveCodePageOfCapacity (M3CodePage** io_list, u32 i_minimum_سطر_شمار)
{
    IM3CodePage prev = پوچ;
    IM3CodePage page = *io_list;

    وقتی (page) {
        اگر (رقم_Free_سطرها(page) >= i_minimum_سطر_شمار) {             d_m3Assert (page->info.usage_شمار == 0);
            IM3CodePage next = page->info.next;
            اگر (prev) {
                prev->info.next = next; // mid-list
            } وگرنه {
                *io_list = next;       // front of list
            }

            بشکن;
        }

        prev = page;
        page = page->info.next;
    }

    بازگشت page;
}


IM3CodePage Environment_AcquireCodePage (IM3Environment i_environment, u32 i_minimum_سطر_شمار)
{
    بازگشت RemoveCodePageOfCapacity(&i_environment->pagesReleased, i_minimum_سطر_شمار);
}


تهی Environment_ReleaseCodePages (IM3Environment i_environment, IM3CodePage i_codePageList)
{
    IM3CodePage end = i_codePageList;

    وقتی (end) {
        end->info.سطر_Index = 0; // reset page
#اگر d_m3RecordBacktraces
        end->info.mapping->size = 0;
#endif // d_m3RecordBacktraces

        IM3CodePage next = end->info.next;
        اگر (not next) {
            بشکن;
        }

        end = next;
    }

    اگر (end) {
        // push list to front
        end->info.next               = i_environment->pagesReleased;
        i_environment->pagesReleased = i_codePageList;
    }
}


IM3Runtime m3_NewRuntime (IM3Environment i_environment, u32 i_پشته_Size_In_Bytes, تهی* i_userdata)
{
    IM3Runtime runtime = m3_AllocStruct(M3Runtime);

    اگر (runtime) {
        m3_ResetErrorInfo(runtime);

        runtime->environment = i_environment;
        runtime->userdata    = i_userdata;

        Deterministic_InitRuntime(runtime);

        runtime->origin_پشته = m3_Malloc("Wasm Stack", i_پشته_Size_In_Bytes + 4 * اندازه(m3slot_t)); // TODO: more precise stack checks

        اگر (runtime->origin_پشته) {
            runtime->پشته         = runtime->origin_پشته;
            runtime->رقم_پشته_Slots = i_پشته_Size_In_Bytes / اندازه(m3slot_t);          m3log (runtime, "new stack: %p, slots: %u", runtime->origin_پشته, runtime->رقم_پشته_Slots);
        } وگرنه {
            m3_Free(runtime);
        }
    }

    بازگشت runtime;
}

تهی m3_SetValidation (IM3Runtime i_runtime, bool i_enable)
{
#اگر d_m3EnableValidation
    اگر (i_runtime) {
        i_runtime->skipValidation = not i_enable;
    }
#وگرنه
    (تهی)i_runtime;
    (تهی)i_enable;              // nothing to skip: the validator was compiled out
#endif
}

M3Result m3_SetResourceLimit (IM3Runtime i_runtime, M3ResourceLimit i_limit, uint64_t i_مقدار)
{
    اگر (not i_runtime) {
        بازگشت m3Err_mallocFailed;
    }
    انتخاب (i_limit) {
    حالت c_m3Limit_MemoryBytes:
        اگر (i_مقدار and i_مقدار < i_runtime->memoryBytesUsed) {
            بازگشت m3Err_resourceLimitBelowUsage;
        }
        i_runtime->memoryBytesLimit = i_مقدار;
        بازگشت m3Err_none;
    حالت c_m3Limit_TableElements:
        اگر (i_مقدار and i_مقدار < i_runtime->tableElementsUsed) {
            بازگشت m3Err_resourceLimitBelowUsage;
        }
        i_runtime->tableElementsLimit = i_مقدار;
        بازگشت m3Err_none;
#اگر d_m3_Has_پشته_Switching
    حالت c_m3Limit_Continuations:
        اگر (i_مقدار and i_مقدار < i_runtime->continuationsAllocated) {
            بازگشت m3Err_resourceLimitBelowUsage;
        }
        i_runtime->continuationsLimit = (u32)M3_MIN(i_مقدار, (u64)UINT32_MAX);
        وقتی (i_مقدار and i_runtime->val_پشته_Pool_شمار > i_مقدار - i_runtime->continuationsAllocated) {
            m3_Free(i_runtime->val_پشته_Pool[--i_runtime->val_پشته_Pool_شمار]);
        }
        بازگشت m3Err_none;
#وگرنه
    حالت c_m3Limit_Continuations: بازگشت m3Err_resourceLimitNotSupported;
#endif
    حالت c_m3Limit_GasUnits:
#اگر d_m3HasGasMetering
        i_runtime->gasLimit = i_runtime->gasRemaining = (i64)M3_MIN(i_مقدار, (u64)INT64_MAX);
        بازگشت m3Err_none;
#وگرنه
        بازگشت m3Err_resourceLimitNotSupported;
#endif
    پیشفرض: بازگشت m3Err_unknownResourceLimit;
    }
}

uint64_t m3_GetResourceLimit (IM3Runtime i_runtime, M3ResourceLimit i_limit)
{
    اگر (not i_runtime) {
        بازگشت 0;
    }
    انتخاب (i_limit) {
    حالت c_m3Limit_MemoryBytes: بازگشت i_runtime->memoryBytesLimit;
    حالت c_m3Limit_TableElements: بازگشت i_runtime->tableElementsLimit;
#اگر d_m3_Has_پشته_Switching
    حالت c_m3Limit_Continuations: بازگشت i_runtime->continuationsLimit;
#endif
#اگر d_m3HasGasMetering
    حالت c_m3Limit_GasUnits: بازگشت (u64)i_runtime->gasLimit;
#endif
    پیشفرض: بازگشت 0;
    }
}

uint64_t m3_GetResourceUsage (IM3Runtime i_runtime, M3ResourceLimit i_limit)
{
    اگر (not i_runtime) {
        بازگشت 0;
    }
    انتخاب (i_limit) {
    حالت c_m3Limit_MemoryBytes: بازگشت i_runtime->memoryBytesUsed;
    حالت c_m3Limit_TableElements: بازگشت i_runtime->tableElementsUsed;
#اگر d_m3_Has_پشته_Switching
    حالت c_m3Limit_Continuations: بازگشت i_runtime->continuationsAllocated;
#endif
#اگر d_m3HasGasMetering
    حالت c_m3Limit_GasUnits: بازگشت (u64)i_runtime->gasLimit - (u64)i_runtime->gasRemaining;
#endif
    پیشفرض: بازگشت 0;
    }
}

تهی* m3_GetUserData (IM3Runtime i_runtime)
{
    بازگشت i_runtime ? i_runtime->userdata : پوچ;
}


تهی* ForEachModule (IM3Runtime i_runtime, ModuleVisitor i_visitor, تهی* i_info)
{
    تهی* r = پوچ;

    IM3Module module = i_runtime->modules;

    وقتی (module) {
        IM3Module next = module->next;
        اگر ((r = i_visitor(module, i_info))) {
            بشکن;
        }

        module = next;
    }

    بازگشت r;
}


تهی* _FreeModule (IM3Module i_module, تهی* i_info)
{
    m3_FreeModule(i_module);
    بازگشت پوچ;
}


// A module's memory and table index spaces hold borrowed pointers: linking
// repoints an imported slot at the memory or table the exporting module owns.
// m3_FreeModule tells a borrowed slot from an owned one by reading that owner,
// which only holds while the object is still allocated - and the free walk
// below visits modules in load order, with no ordering between an owner and
// the modules borrowing from it. So clear the borrowed slots first, in a pass
// that frees nothing; the walk then skips them on the NULL check it already
// does, instead of reading a memory some earlier module has freed.
تهی* _ReleaseBorrowedSlots (IM3Module i_module, تهی* i_info)
{
    برای (u32 i = 0; i < i_module->رقم_Memories; ++i) {
        IM3Memory memory = i_module->memories[i];

        اگر (memory and memory->owner != i_module) {
            i_module->memories[i] = پوچ;
        }
    }

    برای (u32 i = 0; i < i_module->رقم_Tables; ++i) {
        IM3Table table = i_module->tables[i];

        اگر (table and table->owner != i_module) {
            i_module->tables[i] = پوچ;
        }
    }

    بازگشت پوچ;
}


تهی Runtime_Release (IM3Runtime i_runtime)
{
    ForEachModule(i_runtime, _ReleaseBorrowedSlots, پوچ);
    ForEachModule(i_runtime, _FreeModule, پوچ);                    d_m3Assert (i_runtime->رقم_Active_Code_Pages == 0);

    Environment_ReleaseCodePages(i_runtime->environment, i_runtime->pagesOpen);
    Environment_ReleaseCodePages(i_runtime->environment, i_runtime->pagesFull);

    m3_Free(i_runtime->origin_پشته);

#اگر d_m3EnableValidation
    m3_Free(i_runtime->validator);
#endif

#اگر d_m3HasExceptionHandling
    // a runtime let go of while suspended still holds its exceptions
    FreeExceptions(i_runtime);
    m3_Free(i_runtime->uncaughtException);
#endif
#اگر d_m3HasExceptionHandling || d_m3_Has_پشته_Switching
    وقتی (i_runtime->hostTags) {
        M3HostTag* next = i_runtime->hostTags->next;
        m3_Free(i_runtime->hostTags);
        i_runtime->hostTags = next;
    }
#endif
#اگر d_m3_Has_پشته_Switching
    Continuation_ReleaseAll(i_runtime);
#endif
}


تهی m3_FreeRuntime (IM3Runtime i_runtime)
{
    اگر (i_runtime) {
        m3_PrintProfilerInfo();

        Runtime_Release(i_runtime);
        m3_Free(i_runtime);
    }
}


#اگر d_m3_Has_پشته_Switching

IM3Continuation Continuation_New (IM3Runtime i_runtime, IM3FuncType i_type, IM3Function i_function)
{
    IM3Continuation cont = m3_AllocStruct(M3Continuation);
    اگر (!cont) {
        بازگشت پوچ;
    }

    cont->type          = i_type;
    cont->entryFunction = i_function;
    cont->state         = cont_allocated;

    cont->رقم_پشته_Slots = d_m3_Continuation_پشته_Slots;
    // frames are recorded only when this continuation actually suspends, and
    // only as deep as it got, so the array starts out unallocated
    cont->frames    = پوچ;
    cont->framesCap = 0;
    cont->رقم_Frames = 0;

    cont->sp = cont->val_پشته;
    cont->pc = i_function ? i_function->compiled : پوچ;
    cont->r0 = 0;
#  اگر d_m3HasFloat
    cont->fp0 = 0;
#  endif
    cont->bound_Args_شمار    = 0;
    cont->رقم_Suspend_Results = 0;
    cont->رقم_Handlers       = 0;
    cont->handlersPC        = پوچ;
    cont->parent            = پوچ;

    cont->next               = i_runtime->continuations;
    i_runtime->continuations = cont;

    بازگشت cont;
}

IM3Continuation Continuation_ForkSuspended (IM3Runtime i_runtime, IM3Continuation i_cont)
{
    IM3Continuation newCont = Continuation_New(i_runtime, i_cont->type, i_cont->entryFunction);
    اگر (M3_UNLIKELY(not newCont)) {
        بازگشت پوچ;
    }

    newCont->val_پشته      = i_cont->val_پشته;
    newCont->رقم_پشته_Slots = i_cont->رقم_پشته_Slots;
    newCont->sp            = i_cont->sp;
    newCont->pc            = i_cont->pc;
    newCont->suspendPoint  = i_cont->suspendPoint;
    newCont->r0            = i_cont->r0;
#  اگر d_m3HasFloat
    newCont->fp0 = i_cont->fp0;
#  endif
    newCont->frames    = i_cont->frames;
    newCont->framesCap = i_cont->framesCap;
    newCont->رقم_Frames = i_cont->رقم_Frames;

    newCont->bound_Args_شمار = 0;
#  اگر d_m3HasExceptionHandling
    newCont->resumeThrow = i_cont->resumeThrow;
    i_cont->resumeThrow  = پوچ;
#  endif
    newCont->رقم_Suspend_Results = i_cont->رقم_Suspend_Results;
    برای (u32 i = 0; i < i_cont->رقم_Suspend_Results; ++i) {
        newCont->suspendResultOffsets[i] = i_cont->suspendResultOffsets[i];
        newCont->suspendResultIs64[i]    = i_cont->suspendResultIs64[i];
    }

    newCont->رقم_Handlers = i_cont->رقم_Handlers;
    newCont->handlersPC  = i_cont->handlersPC;
    newCont->parent      = i_cont->parent;
    newCont->state       = cont_suspended;

    i_cont->val_پشته  = پوچ;
    i_cont->frames    = پوچ;
    i_cont->framesCap = 0;
    i_cont->رقم_Frames = 0;
    i_cont->state     = cont_consumed;

    بازگشت newCont;
}

// Only cont.new and snapshot restoration acquire stacks. Binding and suspension
// transfer them between records without changing the active count.
M3Result Continuation_Acquire_پشته (IM3Runtime runtime, IM3Continuation cont)
{
    اگر (not Continuation_Can_Acquire_پشته(runtime)) {
        بازگشت m3Err_continuationLimitExceeded;
    }
    اگر (runtime->val_پشته_Pool_شمار) {
        cont->val_پشته = runtime->val_پشته_Pool[--runtime->val_پشته_Pool_شمار];
        memset(cont->val_پشته, 0, (cont->رقم_پشته_Slots + 4) * اندازه(m3slot_t));
    } وگرنه {
        cont->val_پشته = m3_AllocArray(m3slot_t, cont->رقم_پشته_Slots + 4);
    }
    اگر (not cont->val_پشته) {
        بازگشت m3Err_mallocFailed;
    }
    cont->sp = cont->val_پشته;
    runtime->continuationsAllocated++;
    بازگشت m3Err_none;
}

تهی Continuation_Recycle_پشته (IM3Runtime runtime, IM3Continuation cont)
{
    اگر (not cont->val_پشته or cont == runtime->ریشه_Continuation) {
        بازگشت;
    }
    اگر (not runtime->val_پشته_Pool) {
        runtime->val_پشته_Pool = m3_AllocArray(m3slot_t*, 16);
        اگر (runtime->val_پشته_Pool) {
            runtime->val_پشته_Pool_Cap = 16;
        }
    }
    اگر (runtime->val_پشته_Pool_شمار < runtime->val_پشته_Pool_Cap and
        (not runtime->continuationsLimit or runtime->val_پشته_Pool_شمار <
                                              runtime->continuationsLimit - runtime->continuationsAllocated + 1)) {
        runtime->val_پشته_Pool[runtime->val_پشته_Pool_شمار++] = cont->val_پشته;
        cont->val_پشته                                      = پوچ;
    } وگرنه {
        m3_Free(cont->val_پشته);
    }
    cont->sp = پوچ;
    runtime->continuationsAllocated--;
}

تهی Continuation_ReleaseAll (IM3Runtime io_runtime)
{
    // Records live until teardown: a consumed reference can still be in a table
    // or on a Wasm stack. Its value stack has already been recycled.
    IM3Continuation cont = io_runtime->continuations;

    وقتی (cont) {
        IM3Continuation next = cont->next;

        m3_Free(cont->val_پشته);
        m3_Free(cont->frames);
        m3_Free(cont);

        cont = next;
    }

    برای (u32 i = 0; i < io_runtime->val_پشته_Pool_شمار; ++i) {
        m3_Free(io_runtime->val_پشته_Pool[i]);
    }
    m3_Free(io_runtime->val_پشته_Pool);
    io_runtime->val_پشته_Pool_شمار = io_runtime->val_پشته_Pool_Cap = 0;
    io_runtime->continuationsAllocated                          = 0;
    io_runtime->continuations                                   = پوچ;
    io_runtime->activeContinuation                              = پوچ;
    io_runtime->ریشه_Continuation                                = پوچ;
}


m3ret_t Continuation_RecordFrame (IM3Runtime i_runtime, ثابت M3Frame* i_frame)
{
    IM3Continuation cont = i_runtime->activeContinuation;

    // the marker only travels between a suspend and the resume that runs it,
    // and that whole stretch has an active continuation
    d_m3Assert(cont);

    اگر (cont->رقم_Frames == cont->framesCap) {
        u32 newCap = cont->framesCap ? cont->framesCap * 2 : 8;

        اگر (newCap > d_m3ContinuationMaxFrames) {
            newCap = d_m3ContinuationMaxFrames;
        }

        اگر (newCap == cont->framesCap) {
            بازگشت m3_Err_trap_پشته_Overflow;
        }

        M3Frame* frames = m3_ReallocArray(M3Frame, cont->frames, newCap, cont->framesCap);

        اگر (not frames) {
            بازگشت m3Err_mallocFailed;
        }

        cont->frames    = frames;
        cont->framesCap = newCap;
    }

    cont->frames[cont->رقم_Frames++] = *i_frame;

    بازگشت m3Err_continuationSuspended;
}

#endif // d_m3HasStackSwitching

M3Result EvaluateExpression (IM3Module i_module, تهی* o_expressed, m3type_t i_type, bytes_t* io_bytes, cbytes_t i_end)
{
    M3Result result = m3Err_none;

    // OPTZ: use a simplified interpreter for expressions

    // create a temporary runtime context
#اگر defined(d_m3PreferStaticAlloc)
    ایستا M3Runtime runtime;
#وگرنه
    M3Runtime runtime;
#endif
    M3_INIT(runtime);

    runtime.environment   = i_module->runtime->environment;
    runtime.رقم_پشته_Slots = i_module->runtime->رقم_پشته_Slots;
    runtime.پشته         = i_module->runtime->پشته;

    m3stack_t پشته = (m3stack_t)runtime.پشته;

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

    اگر (o->page) {
        IM3FuncType ftype = پوچ;

#اگر d_m3HasTypedRefs
        // A reference type has to be checked as the type it is: the shared
        // return types are one per storage class, so borrowing one of those
        // would let any reference at all satisfy a particular one.
        اگر (IsSpelledRefType(i_type)) {
            result = AllocFuncType(&ftype, 1);

            اگر (not result) {
                ftype->رقم_Rets  = 1;
                ftype->types[0] = i_type;

                result = Environment_AddFuncType(runtime.environment, &ftype);
            }
        } وگرنه
#endif
        {
            ftype = runtime.environment->retFuncTypes[BaseTypeOf(i_type)];
        }

        pc_t m3code = GetPagePC(o->page);

        اگر (not result) {
            result = CompileExpression(o, ftype);
        }

        اگر (not result && o->max_پشته_Slots >= runtime.رقم_پشته_Slots) {
            result = m3_Err_trap_پشته_Overflow;
        }

        اگر (not result) {
#اگر (d_m3EnableOpProfiling || d_m3EnableOpTracing)
            m3ret_t r = RunCode(m3code, پشته, پوچ, d_m3OpDefaultArgs, d_m3BaseCstr);
#وگرنه
            m3ret_t r = RunCode(m3code, پشته, پوچ, d_m3OpDefaultArgs);
#endif

            اگر (r == 0) {                                                                   m3log (runtime, "expression result: %s", SPrint_مقدار (پشته, i_type));
                اگر (SizeOfType(BaseTypeOf(i_type)) == اندازه(u32)) {
                    *(u32*)o_expressed = *((u32*)پشته);
                } وگرنه {
                    *(u64*)o_expressed = *((u64*)پشته);
                }
            }
        }

        ReleaseCodePage(&runtime, o->page);
    } وگرنه {
        result = m3Err_mallocFailedCodePage;
    }

    runtime.origin_پشته = پوچ;        // prevent free(stack) in ReleaseRuntime
    Runtime_Release(&runtime);
    i_module->runtime = savedRuntime;

    *io_bytes = o->wasm;

    بازگشت result;
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
ایستا
bool LimitsSatisfy (u64 i_exportedSize, bool i_exportedHasMax, u64 i_exportedMax,
                    u64 i_importMin, bool i_importHasMax, u64 i_importMax)
{
    اگر (i_exportedSize < i_importMin) {
        بازگشت false;
    }

    اگر (i_importHasMax and (not i_exportedHasMax or i_exportedMax > i_importMax)) {
        بازگشت false;
    }

    بازگشت true;
}


// The export of this kind under this name, or NULL. The index it carries is
// resolved by the caller, against the module's index space as it stands then -
// after linking, a re-exported import's slot holds the entity it was linked to.
ایستا
ثابت M3Export* Module_FindExport (IM3Module i_module, u8 i_kind, cstr_t i_name)
{
    برای (u32 i = 0; i < i_module->رقم_Exports; ++i) {
        ثابت M3Export* e = &i_module->exports[i];

        اگر (e->kind == i_kind and strcmp(e->name, i_name) == 0) {
            بازگشت e;
        }
    }

    بازگشت پوچ;
}


ایستا
IM3Function Module_FindExportedFunction (IM3Module i_module, cstr_t i_name)
{
    ثابت M3Export* e = Module_FindExport(i_module, d_externalKind_function, i_name);

    // A module that re-exports an import names the placeholder here. Resolve to
    // the function that actually runs, or a host-side call would run it against
    // the importing module's memory.
    بازگشت e ? Function_Implementation(&i_module->functions[e->index]) : پوچ;
}


ایستا
IM3Memory Module_FindExportedMemory (IM3Module i_module, cstr_t i_name)
{
    ثابت M3Export* e = Module_FindExport(i_module, d_externalKind_memory, i_name);

    بازگشت e ? i_module->memories[e->index] : پوچ;
}


ایستا
IM3Table Module_FindExportedTable (IM3Module i_module, cstr_t i_name)
{
    ثابت M3Export* e = Module_FindExport(i_module, d_externalKind_table, i_name);

    بازگشت e ? i_module->tables[e->index] : پوچ;
}


ایستا
IM3Global Module_FindExportedGlobal (IM3Module i_module, cstr_t i_name)
{
    ثابت M3Export* e = Module_FindExport(i_module, d_externalKind_global, i_name);

    بازگشت e ? &i_module->globals[e->index] : پوچ;
}


#اگر d_m3HasExceptionHandling || d_m3_Has_پشته_Switching
ایستا
IM3Tag Module_FindExportedTag (IM3Module i_module, cstr_t i_name)
{
    ثابت M3Export* e = Module_FindExport(i_module, d_externalKind_tag, i_name);

    اگر (not e) {
        بازگشت پوچ;
    }

    IM3Tag tag = &i_module->tags[e->index];

    بازگشت tag->resolved ? tag->resolved : tag;
}
#endif


// Whether the module exports anything at all under this name. Export names are
// unique within a module, so a name one of the lookups above missed but this
// one finds is exported as something else - a kind the import cannot be
// satisfied by, rather than a name the module never exported.
ایستا
bool Module_HasExport (IM3Module i_module, cstr_t i_name)
{
    برای (u32 i = 0; i < i_module->رقم_Exports; ++i) {
        اگر (strcmp(i_module->exports[i].name, i_name) == 0) {
            بازگشت true;
        }
    }

    بازگشت false;
}


// Points each of the module's imports at whatever already-loaded module exports
// it. Runs before anything is allocated or initialized: a memory import has to
// be resolved before InitMemory would give it pages of its own, and a global
// import before InitGlobals runs an initializer that reads it.
ایستا
M3Result LinkImports (IM3Runtime io_runtime, IM3Module io_module)
{
    M3Result result = m3Err_none;

    برای (u32 i = 0; i < io_module->رقم_Functions; ++i) {
        IM3Function f = &io_module->functions[i];

        اگر (f->wasm or not (f->import.moduleUtf8 and f->import.fieldUtf8)) {
            ادامه;
        }

        IM3Module from = m3_FindModule(io_runtime, f->import.moduleUtf8);
        اگر (not from) {
            ادامه;
        }

        IM3Function exported = Module_FindExportedFunction(from, f->import.fieldUtf8);
        اگر (not exported) {
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

    برای (u32 i = 0; i < io_module->رقم_Memories; ++i) {
        IM3Memory memory = io_module->memories[i];

        اگر (not memory->imported or memory->owner != io_module) {
            ادامه;
        }

        IM3Module from = m3_FindModule(io_runtime, memory->import.moduleUtf8);
        اگر (not from) {
            ادامه;
        }

        IM3Memory exported = Module_FindExportedMemory(from, memory->import.fieldUtf8);
        اگر (not exported) {
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
                 not LimitsSatisfy(exported->رقم_Pages, exported->hasMax, exported->maxPages,
                                   memory->initPages, memory->hasMax, memory->maxPages));

        // hand the slot over to the exporter's memory, and drop the placeholder
        FreeMemoryBlock(memory);
        FreeImportInfo(&memory->import);
        m3_Free(memory);

        io_module->memories[i] = exported;
    }

    برای (u32 i = 0; i < io_module->رقم_Tables; ++i) {
        IM3Table table = io_module->tables[i];

        اگر (not table->imported or table->owner != io_module) {
            ادامه;
        }

        IM3Module from = m3_FindModule(io_runtime, table->import.moduleUtf8);
        اگر (not from) {
            ادامه;
        }

        IM3Table exported = Module_FindExportedTable(from, table->import.fieldUtf8);
        اگر (not exported) {
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

    برای (u32 i = 0; i < io_module->رقم_Globals; ++i) {
        IM3Global g = &io_module->globals[i];

        اگر (not g->imported or not (g->import.moduleUtf8 and g->import.fieldUtf8)) {
            ادامه;
        }

        IM3Module from = m3_FindModule(io_runtime, g->import.moduleUtf8);
        اگر (not from) {
            ادامه;
        }

        IM3Global exported = Module_FindExportedGlobal(from, g->import.fieldUtf8);
        اگر (not exported) {
            _throwif(m3Err_incompatibleImportType, Module_HasExport(from, g->import.fieldUtf8));
            _throw(m3Err_unknownImport);
        }

        _throwif(m3Err_incompatibleImportType, exported->type != g->type);
        _throwif(m3Err_incompatibleImportType, exported->isMutable != g->isMutable);

        g->resolved = exported->resolved ? exported->resolved : exported;
    }

#اگر d_m3HasExceptionHandling || d_m3_Has_پشته_Switching
    برای (u32 i = 0; i < io_module->رقم_Tags; ++i) {
        IM3Tag tag = &io_module->tags[i];

        اگر (not tag->imported or not (tag->import.moduleUtf8 and tag->import.fieldUtf8)) {
            ادامه;
        }

        IM3Module from = m3_FindModule(io_runtime, tag->import.moduleUtf8);
        اگر (not from) {
            ادامه;
        }

        IM3Tag exported = Module_FindExportedTag(from, tag->import.fieldUtf8);
        اگر (not exported) {
            _throwif(m3Err_incompatibleImportType, Module_HasExport(from, tag->import.fieldUtf8));
            _throw(m3Err_unknownImport);
        }

        _throwif(m3Err_incompatibleImportType, exported->type != tag->type);

        tag->resolved = exported->resolved ? exported->resolved : exported;
    }
#endif

    _catch: بازگشت result;
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
    i_module->memory0 = i_module->رقم_Memories ? i_module->memories[0]
                                              : &i_module->emptyMemory;

    اگر (i_module->رقم_Memories == 0) {
        // nothing addressable, but _mem still has to point somewhere
        i_module->emptyMemory.owner    = i_module;
        i_module->emptyMemory.pageSize = d_m3DefaultMemPageSize;

_       (ResizeMemory(io_runtime, &i_module->emptyMemory, 0));
    }

    برای (u32 i = 0; i < i_module->رقم_Memories; ++i) {
        IM3Memory memory = i_module->memories[i];

        // a slot that already points at another module's memory is that
        // module's to allocate
        اگر (memory->owner != i_module or memory->mallocated) {
            ادامه;
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
        اگر (not memory->hasMax) {
            memory->maxPages = memory->isMemory64
                                 ? (pageSize > 1 ? (UINT64_MAX / pageSize) + 1 : UINT64_MAX)
                                 : (0x100000000ull / pageSize);
        }

_       (ResizeMemory(io_runtime, memory, memory->initPages));
    }

    _catch: بازگشت result;
}


M3Result ResizeMemory (IM3Runtime io_runtime, IM3Memory memory, u64 i_رقم_Pages)
{
    M3Result result = m3Err_none;

    u64 رقم_Pages_To_Alloc = i_رقم_Pages;

    اگر (رقم_Pages_To_Alloc <= memory->maxPages) {
        // A 64-bit memory may ask for up to 2^48 pages, which overflows a u64
        // of bytes. Nothing that large can be backed, so refuse it up front
        // rather than multiplying into a wrapped size.
        _throwif("linear memory limitation exceeded",
                 رقم_Pages_To_Alloc > d_m3AddressLimit / memory->pageSize);

        u64 رقم_Page_Bytes = رقم_Pages_To_Alloc * memory->pageSize;

#اگر d_m3MaxLinearMemoryPages > 0
        // the limit is a memory size, counted in default-sized pages; comparing
        // it against a raw page count would make it 65536 times stricter for a
        // module whose pages are one byte
        _throwif("linear memory limitation exceeded",
                 رقم_Page_Bytes > (u64)d_m3MaxLinearMemoryPages * d_m3DefaultMemPageSize);
#endif

        // Limit the amount of memory that gets actually allocated
        اگر (io_runtime->memoryLimit) {
            رقم_Page_Bytes = M3_MIN(رقم_Page_Bytes, (u64)io_runtime->memoryLimit);
        }

        u64 oldLength = memory->mallocated ? memory->mallocated->length : 0;
        u64 otherUsed = io_runtime->memoryBytesUsed - oldLength;
        _throwif(m3Err_memoryLimitExceeded, io_runtime->memoryBytesLimit and
                                              (رقم_Page_Bytes > io_runtime->memoryBytesLimit or
                                               otherUsed > io_runtime->memoryBytesLimit - رقم_Page_Bytes));

        _throwif("linear memory limitation exceeded", رقم_Page_Bytes > (u64)SIZE_MAX - اندازه(M3MemoryHeader));

#اگر d_m3GuardedMemory

        // A slot holds every address a Wasm access can name, and nothing beyond
        // that: a memory larger than the data part of one would put addresses past
        // the guard, where a fault is nobody's to recognize
        _throwif("linear memory limitation exceeded", رقم_Page_Bytes > d_m3GuardedDataBytes);

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

        اگر (memory->guardSlot == پوچ) {
            memory->guardSlot = Guard_TakeSlot();
            _throwifnull(memory->guardSlot);
        }

        // Growing only ever commits more of the same slot, so the memory never
        // moves - which the interpreter does not rely on, but nothing minds
        _throwif(m3Err_mallocFailed, not Guard_CommitSlot(memory->guardSlot, (size_t)رقم_Page_Bytes));

        memory->mallocated = Guard_SlotHeader(memory->guardSlot);

#وگرنه

        size_t رقم_Bytes = (size_t)رقم_Page_Bytes + اندازه(M3MemoryHeader);

        // What is really there, which is not numPages * pageSize: the limit above
        // caps the bytes without capping the page count, so a memory can be short
        // of a page - by design, since a whole 64 KiB page is often more than an
        // MCU has. Measuring the old block by the page count would overstate it,
        // and then realloc never sees the size it already has and copies the
        // whole limit again on every grow past it.
        size_t رقم_Previous_Bytes = memory->mallocated ? memory->mallocated->length + اندازه(M3MemoryHeader) : 0;

        تهی* newMem = m3_Realloc("Wasm Linear Memory", memory->mallocated, رقم_Bytes, رقم_Previous_Bytes);
        _throwifnull(newMem);

        memory->mallocated = (M3MemoryHeader*)newMem;

#endif

#اگر d_m3LogRuntime
        M3MemoryHeader* oldMallocated = memory->mallocated;
#endif

        io_runtime->memoryBytesUsed = otherUsed + رقم_Page_Bytes;
        memory->رقم_Pages            = رقم_Pages_To_Alloc;

        memory->mallocated->length  = (size_t)رقم_Page_Bytes;
        memory->mallocated->runtime = io_runtime;
        memory->mallocated->memory  = memory;

        memory->mallocated->max_پشته = (m3slot_t*)io_runtime->origin_پشته + io_runtime->رقم_پشته_Slots;

        m3log(runtime, "resized old: %p; mem: %p; length: %zu; pages: %llu", oldMallocated, memory->mallocated, memory->mallocated->length, (unsigned long long)memory->رقم_Pages);
    } وگرنه {
        result = m3Err_wasmMemoryOverflow;
    }

    _catch: بازگشت result;
}


تهی FreeMemoryBlock (IM3Memory io_memory)
{
    اگر (io_memory->mallocated and io_memory->mallocated->runtime) {
        io_memory->mallocated->runtime->memoryBytesUsed -= io_memory->mallocated->length;
    }
#اگر d_m3GuardedMemory
    Guard_GiveSlot(io_memory->guardSlot);
    io_memory->guardSlot = پوچ;

    io_memory->mallocated = پوچ;
#وگرنه
    m3_Free(io_memory->mallocated);
#endif
}


M3Result InitGlobals (IM3Module io_module)
{
    M3Result result = m3Err_none;

    اگر (io_module->رقم_Globals) {
        // placing the globals in their structs isn't good for cache locality, but i don't really know what the global
        // access patterns typically look like yet.

        //          io_module->globalMemory = m3Alloc (m3reg_t, io_module->numGlobals);

        //          if (io_module->globalMemory)
        {
            برای (u32 i = 0; i < io_module->رقم_Globals; ++i) {
                M3Global* g = &io_module->globals[i];                           m3log (runtime, "initializing global: %d", i);

                اگر (g->init_عبارت) {
                    bytes_t start = g->init_عبارت;

                    result = EvaluateExpression(io_module, &g->i64_مقدار, g->type, &start, g->init_عبارت + g->init_عبارت_Size);

                    اگر (not result) {
                        // io_module->globalMemory [i] = initValue;
                    } وگرنه {
                        بشکن;
                    }
                } وگرنه {                                                        m3log (runtime, "importing global");
                }
            }
        }
        //          else result = ErrorModule (m3Err_mallocFailed, io_module, "could allocate globals for module: '%s", io_module->name);
    }

    بازگشت result;
}


M3Result InitDataSegments (IM3Module io_module)
{
    M3Result result = m3Err_none;

    برای (u32 i = 0; i < io_module->رقم_Data_Segments; ++i) {
        M3DataSegment* segment = &io_module->dataSegments[i];

        // A passive segment stays available for memory.init until data.drop.
        // An active one is copied here and then counts as dropped.
        اگر (segment->isPassive) {
            ادامه;
        }

        _throwif("data segment memory index out of range",
                 segment->memoryRegion >= io_module->رقم_Memories);

        IM3Memory io_memory = io_module->memories[segment->memoryRegion];

        _throwif("unallocated linear memory", !(io_memory->mallocated));

        // The offset expression has the memory's address type, and is
        // unsigned: an i32 offset of -1 is 4294967295, way out of bounds
        // rather than negative.
        u64     segmentOffset = 0;
        bytes_t start         = segment->init_عبارت;

        اگر (io_memory->isMemory64) {
_           (EvaluateExpression(io_module, &segmentOffset, c_m3Type_i64, &start, segment->init_عبارت + segment->init_عبارت_Size));
        } وگرنه {
            u32 offset32;
_           (EvaluateExpression(io_module, &offset32, c_m3Type_i32, &start, segment->init_عبارت + segment->init_عبارت_Size));
            segmentOffset = offset32;
        }

        m3log(runtime, "loading data segment: %d; size: %d; offset: %llu", i, segment->size, (unsigned long long)segmentOffset);

        اگر (segmentOffset <= io_memory->mallocated->length &&
            (u64)segment->size <= io_memory->mallocated->length - segmentOffset) {
            u8* dest = m3MemData(io_memory->mallocated) + segmentOffset;
            memcpy(dest, segment->data, segment->size);
        } وگرنه {
            _throw("data segment out of bounds");
        }

        segment->dropped = true;
    }

    _catch: بازگشت result;
}


// Turns a segment's elements into references. Element expressions are constant
// expressions restricted to ref.null/ref.func, so they're read directly rather
// than run through the compiler.
ایستا
M3Result ResolveElements (IM3Module io_module, M3ElementSegment* i_segment, تهی** o_elements)
{
    M3Result result = m3Err_none;

    bytes_t  pos = i_segment->elements;
    cbytes_t end = io_module->elementSectionEnd;

    برای (u32 e = 0; e < i_segment->رقم_Elements; ++e) {
        u32   funcIndex;
        تهی* ref = پوچ;

        اگر (i_segment->is_عبارت) {
            m3opcode_t opcode;
_           (Read_opcode(&opcode, &pos, end));

            اگر (opcode == c_waOp_refFunc) {
_               (ReadLEB_u32(&funcIndex, &pos, end));
                _throwif("function index out of range", funcIndex >= io_module->رقم_Functions);
                ref = Function_Implementation(&io_module->functions[funcIndex]);
            } وگرنه اگر (opcode == c_waOp_refNull) {
                i8 waType;
                u8 nullType;
_               (ReadLEB_i7(&waType, &pos, end));
_               (NormalizeType(&nullType, waType));
                _throwif(m3Err_typeMismatch, nullType != i_segment->type);
            } وگرنه اگر (opcode == c_waOp_getGlobal) {
                // wasm 2.0 lets an element expression read an imported
                // immutable global, which is how one module seeds another's
                // table with a reference it exported.
                u32 globalIndex;
_               (ReadLEB_u32(&globalIndex, &pos, end));
                _throwif(m3Err_globaIndexOutOfBounds, globalIndex >= io_module->رقم_Globals);

                IM3Global global = &io_module->globals[globalIndex];

                _throwif(m3Err_globaIndexOutOfBounds, not global->imported);
                _throwif(m3Err_wasmMalformed, global->isMutable);
                _throwif(m3Err_typeMismatch, BaseTypeOf(global->type) != BaseTypeOf(i_segment->type));

                // read the cell the import was linked to, not the placeholder
                اگر (global->resolved) {
                    global = global->resolved;
                }

                ref = global->ref_مقدار;
            } وگرنه {
                _throw("constant expression required");
            }

_           (Read_opcode(&opcode, &pos, end));
            _throwif(m3Err_wasmMalformed, opcode != c_waOp_end);
        } وگرنه {
_           (ReadLEB_u32(&funcIndex, &pos, end));
            _throwif("function index out of range", funcIndex >= io_module->رقم_Functions);
            ref = Function_Implementation(&io_module->functions[funcIndex]);
        }

        o_elements[e] = ref;
    }

    _catch: بازگشت result;
}


M3Result InitTableAndElements (IM3Module io_module)
{
    M3Result result = m3Err_none;

    cbytes_t end = io_module->elementSectionEnd;
    M3Table* table;

    برای (u32 i = 0; i < io_module->رقم_Tables; ++i) {
        table = io_module->tables[i];

        // a slot pointing at another module's table is that module's to fill
        اگر (table->owner != io_module) {
            ادامه;
        }

        اگر (table->size) {
            IM3Runtime runtime = io_module->runtime;
            _throwif(m3Err_tableLimitExceeded, runtime->tableElementsLimit and
                                                 table->size > runtime->tableElementsLimit - runtime->tableElementsUsed);
            table->elements = m3_AllocArray(تهی*, table->size);
            _throwifnull(table->elements);
            runtime->tableElementsUsed += table->size;

            اگر (table->init_عبارت) {
                تهی*   مقدار = پوچ;
                bytes_t start = table->init_عبارت;
_               (EvaluateExpression(io_module, &مقدار, BaseTypeOf(table->type),
                                     &start, table->init_عبارت + table->init_عبارت_Size));

                برای (u32 e = 0; e < table->size; ++e) {
                    table->elements[e] = مقدار;
                }
            }
        }
    }

    برای (u32 i = 0; i < io_module->رقم_Element_Segments; ++i) {
        M3ElementSegment* segment = &io_module->elementSegments[i];

        // Declarative segments only make their functions referenceable, and
        // passive ones wait for table.init, so neither is written out here.
        اگر (segment->mode == c_m3Elem_declarative) {
            segment->dropped = true;
            ادامه;
        }

        اگر (segment->mode == c_m3Elem_passive) {
            اگر (segment->رقم_Elements) {
                segment->resolved = m3_AllocArray(تهی*, segment->رقم_Elements);
                _throwifnull(segment->resolved);
_               (ResolveElements(io_module, segment, segment->resolved));
            }
            ادامه;
        }

        table = io_module->tables[segment->tableIndex];

        // The offset expression has the table's index type, and is unsigned:
        // an i32 offset of -1 is 4294967295, out of bounds rather than negative.
        u64     offset = 0;
        bytes_t عبارت   = segment->init_عبارت;

        اگر (table->isTable64) {
_           (EvaluateExpression(io_module, &offset, c_m3Type_i64, &عبارت, end));
        } وگرنه {
            u32 offset32;
_           (EvaluateExpression(io_module, &offset32, c_m3Type_i32, &عبارت, end));
            offset = offset32;
        }

        _throwif("out of bounds table access",
                 offset > table->size or segment->رقم_Elements > table->size - offset);

_       (ResolveElements(io_module, segment, table->elements + offset));

        segment->dropped = true;
    }

    _catch: بازگشت result;
}

M3Result m3_CompileModule (IM3Module io_module)
{
    M3Result result = m3Err_none;

    برای (u32 i = 0; i < io_module->رقم_Functions; ++i) {
        IM3Function f = &io_module->functions[i];
        اگر (f->wasm and not f->compiled) {
_           (CompileFunction(f));
        }
    }

    _catch: بازگشت result;
}

#اگر d_m3HasExceptionHandling

M3Exception* NewException (IM3Runtime io_runtime, IM3Tag i_tag, u32 i_رقم_Args)
{
    M3Exception* exception = (M3Exception*)m3_Malloc("M3Exception", اندازه(M3Exception) + i_رقم_Args * اندازه(u64));

    اگر (exception) {
        exception->tag     = i_tag;
        exception->رقم_Args = i_رقم_Args;
        exception->reified = false;
        exception->prev    = پوچ;
        exception->next    = io_runtime->exceptions;

        اگر (exception->next) {
            exception->next->prev = exception;
        }

        io_runtime->exceptions = exception;
    }

    بازگشت exception;
}


// Releases one exception ahead of the rest. The caller has to know nothing can
// still name it: no exnref was ever taken of it, and its payload has already
// been copied out.
تهی FreeException (IM3Runtime io_runtime, M3Exception* i_exception)
{
    اگر (i_exception->prev) {
        i_exception->prev->next = i_exception->next;
    } وگرنه {
        io_runtime->exceptions = i_exception->next;
    }

    اگر (i_exception->next) {
        i_exception->next->prev = i_exception->prev;
    }

    اگر (io_runtime->pendingException == i_exception) {
        io_runtime->pendingException = پوچ;
    }

    m3_Free_Impl(i_exception);
}


// Releases every exception the runtime still holds. Only safe once the Wasm
// stack is empty, which is why the outermost RunCodeChecked() is the one that
// calls it.
تهی FreeExceptions (IM3Runtime io_runtime)
{
    M3Exception* exception = io_runtime->exceptions;

    io_runtime->exceptions       = پوچ;
    io_runtime->pendingException = پوچ;

    وقتی (exception) {
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
ایستا
تهی RecordUncaughtException (IM3Runtime io_runtime, M3Result i_result)
{
    اگر (io_runtime->uncaughtException) {
        m3_Free(io_runtime->uncaughtException);
    }

    M3Exception* exception = io_runtime->pendingException;

    اگر (M3_UNLIKELY(i_result == m3Err_trapUncaughtException) and exception) {
        M3Exception* copy = (M3Exception*)m3_CopyMem(exception, اندازه(M3Exception) + exception->رقم_Args * اندازه(u64));

        اگر (copy) {
            copy->next = پوچ;
            copy->prev = پوچ;
        }

        io_runtime->uncaughtException = copy;
    }
}

#endif // d_m3HasExceptionHandling


#اگر d_m3HasExceptionHandling || d_m3_Has_پشته_Switching

IM3Tag m3_FindTag (IM3Module i_module, ثابت نویسه* ثابت i_tagName)
{
    اگر (not i_module or not i_tagName) {
        بازگشت پوچ;
    }

    بازگشت Module_FindExportedTag(i_module, i_tagName);
}


M3Result m3_NewTag (IM3Runtime io_runtime, IM3Tag* o_tag, ثابت نویسه* ثابت i_signature)
{
    IM3FuncType type    = پوچ;
    M3HostTag*  hostTag = پوچ;

_try {
    _throwif("null tag", not o_tag);
    *o_tag = پوچ;
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
    بازگشت result;
}


M3Result m3_LinkTag (IM3Module         io_module,
                     ثابت نویسه* ثابت i_moduleName,
                     ثابت نویسه* ثابت i_tagName,
                     IM3Tag            i_tag)
{
    اگر (not io_module or not i_moduleName or not i_tagName or not i_tag) {
        بازگشت m3Err_tagLookupFailed;
    }

    ثابت bool wildcardModule = (strcmp(i_moduleName, "*") == 0);
    IM3Tag     target         = i_tag->resolved ? i_tag->resolved : i_tag;

    M3Result result = m3Err_tagLookupFailed;

    برای (u32 i = 0; i < io_module->رقم_Tags; ++i) {
        IM3Tag tag = &io_module->tags[i];

        اگر (tag->imported and tag->import.moduleUtf8 and tag->import.fieldUtf8 and
            strcmp(tag->import.fieldUtf8, i_tagName) == 0 and
            (wildcardModule or strcmp(tag->import.moduleUtf8, i_moduleName) == 0)) {
            اگر (tag->type != target->type) {
                بازگشت m3Err_incompatibleImportType;
            }

            tag->resolved = target;
            result        = m3Err_none;
        }
    }

    بازگشت result;
}


uint32_t m3_Get_Tag_Arg_شمار (IM3Tag i_tag)
{
    بازگشت i_tag ? i_tag->type->رقم_Args : 0;
}


M3_مقدار_Type m3_GetTagArgType (IM3Tag i_tag, uint32_t i_index)
{
    اگر (i_tag and i_index < i_tag->type->رقم_Args) {
        بازگشت (M3_مقدار_Type)BaseTypeOf(d_FuncArgType(i_tag->type, i_index));
    }

    بازگشت c_m3Type_none;
}

#وگرنه

IM3Tag m3_FindTag (IM3Module i_module, ثابت نویسه* ثابت i_tagName)
{
    بازگشت پوچ;
}

M3Result m3_NewTag (IM3Runtime io_runtime, IM3Tag* o_tag, ثابت نویسه* ثابت i_signature)
{
    اگر (o_tag) {
        *o_tag = پوچ;
    }
    بازگشت m3Err_tagLookupFailed;
}

M3Result m3_LinkTag (IM3Module io_module, ثابت نویسه* ثابت i_moduleName, ثابت نویسه* ثابت i_tagName, IM3Tag i_tag)
{
    بازگشت m3Err_tagLookupFailed;
}

uint32_t m3_Get_Tag_Arg_شمار (IM3Tag i_tag)
{
    بازگشت 0;
}

M3_مقدار_Type m3_GetTagArgType (IM3Tag i_tag, uint32_t i_index)
{
    بازگشت c_m3Type_none;
}

#endif // d_m3HasExceptionHandling || d_m3HasStackSwitching


#اگر d_m3HasExceptionHandling

M3Result m3_ThrowException (IM3Runtime io_runtime, IM3Tag i_tag, uint32_t i_argc, ثابت تهی* i_argptrs[])
{
    اگر (not io_runtime or not i_tag) {
        بازگشت m3Err_tagLookupFailed;
    }

    IM3Tag      tag  = i_tag->resolved ? i_tag->resolved : i_tag;
    IM3FuncType type = tag->type;

    // a tag with results is a stack switching control tag, which is suspended
    // with rather than thrown
    اگر (type->رقم_Rets) {
        بازگشت m3Err_typeMismatch;
    }
    اگر (i_argc != type->رقم_Args) {
        بازگشت m3_Err_argument_شمار_Mismatch;
    }
    // with nothing on the stack there is nothing to unwind, and the exception
    // would sit on the runtime's list until some later call ended
    اگر (io_runtime->callNesting == 0) {
        بازگشت "m3_ThrowException called outside a host function";
    }

    M3Exception* exception = NewException(io_runtime, tag, i_argc);
    اگر (not exception) {
        بازگشت m3Err_mallocFailed;
    }

    // the layout op_Throw gives a payload: 32-bit values zero-extended, floats
    // as their bit pattern
    برای (u32 i = 0; i < i_argc; ++i) {
        ثابت تهی* مقدار = i_argptrs[i];

        انتخاب (BaseTypeOf(d_FuncArgType(type, i))) {
        حالت c_m3Type_i32:
        حالت c_m3Type_f32: {
            u32 bits;
            memcpy(&bits, مقدار, اندازه(bits));
            exception->args[i] = bits;
            بشکن;
        }
        حالت c_m3Type_i64:
        حالت c_m3Type_f64:
            memcpy(&exception->args[i], مقدار, اندازه(u64));
            بشکن;
        حالت c_m3Type_funcref:
        حالت c_m3Type_externref:
        حالت c_m3Type_exnref:
        حالت c_m3Type_contref:
            exception->args[i] = (u64)(*(ثابت uintptr_t*)مقدار);
            بشکن;
        پیشفرض:
            FreeException(io_runtime, exception);
            بازگشت "unknown argument type";
        }
    }

    io_runtime->pendingException = exception;

    بازگشت m3Err_pendingException;
}


IM3Tag m3_GetExceptionTag (IM3Runtime i_runtime)
{
    اگر (i_runtime and i_runtime->uncaughtException) {
        بازگشت i_runtime->uncaughtException->tag;
    }

    بازگشت پوچ;
}


M3Result m3_GetExceptionArgs (IM3Runtime i_runtime, uint32_t i_argc, ثابت تهی* o_argptrs[])
{
    اگر (not i_runtime or not i_runtime->uncaughtException) {
        بازگشت "no uncaught exception";
    }

    ثابت M3Exception* exception = i_runtime->uncaughtException;
    IM3FuncType        type      = exception->tag->type;

    اگر (i_argc != exception->رقم_Args) {
        بازگشت m3_Err_argument_شمار_Mismatch;
    }

    برای (u32 i = 0; i < i_argc; ++i) {
        تهی* مقدار = (تهی*)o_argptrs[i];
        u64   bits  = exception->args[i];

        انتخاب (BaseTypeOf(d_FuncArgType(type, i))) {
        حالت c_m3Type_i32:
        حالت c_m3Type_f32: {
            u32 کم = (u32)bits;
            memcpy(مقدار, &کم, اندازه(کم));
            بشکن;
        }
        حالت c_m3Type_i64:
        حالت c_m3Type_f64:
            memcpy(مقدار, &bits, اندازه(bits));
            بشکن;
        حالت c_m3Type_funcref:
        حالت c_m3Type_externref:
        حالت c_m3Type_exnref:
        حالت c_m3Type_contref:
            *(uintptr_t*)مقدار = (uintptr_t)bits;
            بشکن;
        پیشفرض:
            بازگشت "unknown argument type";
        }
    }

    بازگشت m3Err_none;
}

#وگرنه

M3Result m3_ThrowException (IM3Runtime io_runtime, IM3Tag i_tag, uint32_t i_argc, ثابت تهی* i_argptrs[])
{
    بازگشت m3Err_tagLookupFailed;
}

IM3Tag m3_GetExceptionTag (IM3Runtime i_runtime)
{
    بازگشت پوچ;
}

M3Result m3_GetExceptionArgs (IM3Runtime i_runtime, uint32_t i_argc, ثابت تهی* o_argptrs[])
{
    بازگشت "no uncaught exception";
}

#endif // d_m3HasExceptionHandling


#اگر d_m3GuardedMemory

// What RunCodeProtected hands to the platform layer to run, and what comes back
تعریفنوع ساختار M3ProtectedRun {
    pc_t            pc;
    IM3Runtime      runtime;
    M3MemoryHeader* mem;
    M3Result        result;
} M3ProtectedRun;

ایستا
تهی RunCodeBody (تهی* io_context)
{
    M3ProtectedRun* run = (M3ProtectedRun*)io_context;

#  اگر (d_m3EnableOpProfiling || d_m3EnableOpTracing)
    run->result = (M3Result)RunCode(run->pc, (m3stack_t)run->runtime->پشته, run->mem,
                                    d_m3OpDefaultArgs, d_m3BaseCstr);
#  وگرنه
    run->result = (M3Result)RunCode(run->pc, (m3stack_t)run->runtime->پشته, run->mem,
                                    d_m3OpDefaultArgs);
#  endif
}

// The bounds check the load and store operations no longer do, done once here by the
// system instead: an address past the end of a memory reaches the uncommitted part
// of that memory's reservation, and the fault comes back as the trap the spec asks
// for. Nothing is left half-done by it - the Wasm stack and the memories are heap,
// and the caller unwinds the rest.
ایستا
M3Result RunCodeProtected (IM3Runtime i_runtime, pc_t i_pc, M3MemoryHeader* i_mem)
{
    // Memory operations in this build carry no bounds check, so code only runs where a
    // fault past the end of a memory can still be turned into a trap
    اگر (M3_UNLIKELY(not m3_HostGuardsActive())) {
        بازگشت ErrorRuntime(m3Err_memoryGuardsUnavailable, i_runtime,
                            "another part of the process holds the fault signal");
    }

    M3ProtectedRun run;
    run.pc      = i_pc;
    run.runtime = i_runtime;
    run.mem     = i_mem;
    run.result  = m3Err_none;

    تهی*  arena;
    size_t arenaBytes;
    Guard_ArenaRange(&arena, &arenaBytes);

    اگر (not m3_HostProtectedCall(RunCodeBody, &run, arena, arenaBytes)) {
        بازگشت m3Err_trapOutOfBoundsMemoryAccess;
    }

    بازگشت run.result;
}

#endif // d_m3GuardedMemory


#اگر d_m3_Has_پشته_Switching

// The function a suspended continuation stopped in: the callee of its innermost
// call frame, or the function it was entered with if it made no call. Frames are
// recorded innermost first, so that is the lowest-numbered call frame.
ایستا
IM3Function Continuation_InnermostFunction (IM3Continuation i_cont)
{
    برای (u32 i = 0; i < i_cont->رقم_Frames; ++i) {
        اگر (i_cont->frames[i].kind == frame_call && i_cont->frames[i].call.function) {
            بازگشت i_cont->frames[i].call.function;
        }
    }

    بازگشت i_cont->entryFunction;
}


// Where the host's next call puts its arguments and finds its results. A
// suspended root keeps its frames on the bottom of the runtime's stack, so a call
// made before it is resumed starts above the last of them rather than on top of
// them. The top is the innermost function's frame base plus everything that
// function can use. That base is the root's own sp, unless it stopped inside a
// resume: then the suspension point belongs to the resumed continuation, and the
// root's innermost frame is that resume.
تهی Runtime_Place_Call_پشته (IM3Runtime io_runtime)
{
    IM3Continuation ریشه = io_runtime->ریشه_Continuation;

    io_runtime->پشته = io_runtime->origin_پشته;

    اگر (not ریشه or ریشه->state != cont_suspended) {
        بازگشت;
    }

    m3stack_t   sp       = ریشه->sp;
    IM3Function function = Continuation_InnermostFunction(ریشه);

    اگر (ریشه->رقم_Frames and ریشه->frames[0].kind == frame_resume) {
        sp = ریشه->frames[0].sp;
    }

    m3slot_t* base = (m3slot_t*)io_runtime->origin_پشته;
    m3slot_t* بالاترین  = (m3slot_t*)sp + (function ? function->max_پشته_Slots : 0);

    اگر (بالاترین > base + io_runtime->رقم_پشته_Slots) {
        بالاترین = base + io_runtime->رقم_پشته_Slots;
    }

    io_runtime->پشته = بالاترین;
}

#endif // d_m3HasStackSwitching


// Run compiled code on the runtime's stack, bounding native recursion for the
// duration of the call. The outermost invocation establishes the stack limit;
// nested ones (an imported function calling back into Wasm) inherit it.
ایستا درونخطی
M3Result RunCodeChecked (IM3Runtime i_runtime, IM3Function i_function)
{
    pc_t i_pc = i_function->compiled;

    // execution runs against the memory of the module the entry point belongs
    // to, not against some runtime-wide one
    M3MemoryHeader* _mem = Module_MemoryHeader(i_function->module);

#اگر d_m3HasSnapshots
    اگر (M3_UNLIKELY(i_function->module->isUnusable)) {
        بازگشت "a snapshot failed to restore into the module, so it cannot run";
    }
    i_function->module->hasRun = true;
#endif

    d_m3_پشته_Limit_Enter(i_runtime);

    // a host function calling back into Wasm does not change which call a trap
    // ends: that is still the one the host made first
    اگر (i_runtime->callNesting++ == 0) {
        i_runtime->entered = i_function;
    }
#اگر d_m3HasExceptionHandling
    // handler stacks don't nest across a call boundary: a host function calling
    // back into Wasm cannot be caught by a try_table its own caller entered
    u32 savedTryDepth   = i_runtime->tryDepth;
    i_runtime->tryDepth = 0;
#endif
#اگر d_m3_Has_پشته_Switching
    // nor do continuations: the native frames on the far side of a host call
    // are not ours to record, so a suspend in here has no handler to reach and
    // no resume to unwind to
    IM3Continuation savedContinuation = i_runtime->activeContinuation;

    // A suspended root continuation is a paused program waiting to be resumed,
    // not scratch space. A call the host makes before resuming it gets a
    // context of its own, the way any other re-entry does, rather than having
    // the frames of the suspension reset out from under it.
    bool claim_ریشه = i_runtime->isSuspendable and not savedContinuation and
                     not (i_runtime->ریشه_Continuation and
                          i_runtime->ریشه_Continuation->state == cont_suspended);

    اگر (claim_ریشه and not i_runtime->ریشه_Continuation) {
        i_runtime->ریشه_Continuation = Continuation_New(i_runtime, i_function->funcType, i_function);

        اگر (i_runtime->ریشه_Continuation) {
            // The root borrows the runtime's stack and is not charged as cont.new.
            i_runtime->ریشه_Continuation->رقم_پشته_Slots = i_runtime->رقم_پشته_Slots;
        } وگرنه {
            claim_ریشه = false;
        }
    }

    اگر (claim_ریشه) {
        i_runtime->activeContinuation              = i_runtime->ریشه_Continuation;
        i_runtime->ریشه_Continuation->entryFunction = i_function;
        i_runtime->ریشه_Continuation->state         = cont_running;
        i_runtime->ریشه_Continuation->رقم_Frames     = 0;
    } وگرنه {
        i_runtime->activeContinuation = پوچ;
    }
#endif
#اگر d_m3GuardedMemory
    M3Result result = RunCodeProtected(i_runtime, i_pc, _mem);
#وگرنهاگر (d_m3EnableOpProfiling || d_m3EnableOpTracing)
    M3Result result = (M3Result)RunCode(i_pc, (m3stack_t)i_runtime->پشته, _mem, d_m3OpDefaultArgs, d_m3BaseCstr);
#وگرنه
    M3Result result = (M3Result)RunCode(i_pc, (m3stack_t)i_runtime->پشته, _mem, d_m3OpDefaultArgs);
#endif
#اگر d_m3_Has_پشته_Switching
    اگر (claim_ریشه) {
        i_runtime->ریشه_Continuation->state =
          (result == m3Err_continuationSuspended) ? cont_suspended : cont_returned;

        Runtime_Place_Call_پشته(i_runtime);
    }
    i_runtime->activeContinuation = savedContinuation;
#endif
#اگر d_m3HasExceptionHandling
    i_runtime->tryDepth = savedTryDepth;

    // an exception that reached the bottom of the call stack found no handler
    اگر (M3_UNLIKELY(result == m3Err_pendingException)) {
        result = m3Err_trapUncaughtException;
    }
    RecordUncaughtException(i_runtime, result);

    // a paused invocation can still reach the exceptions it holds, so they wait
    // for it to finish
    اگر (i_runtime->callNesting == 1 and not m3_IsSuspended(i_runtime)) {
        FreeExceptions(i_runtime);
    }
#endif
    i_runtime->callNesting--;
    d_m3_پشته_Limit_Leave(i_runtime);

    بازگشت result;
}

M3Result m3_RunStart (IM3Module io_module)
{
#ifdef FUZZING_BUILD_MODE_UNSAFE_FOR_PRODUCTION
    // Execution disabled for fuzzing builds
    بازگشت m3Err_none;
#endif

    M3Result result           = m3Err_none;
    i32      startFunctionTmp = -1;

    اگر (io_module and io_module->startFunction >= 0) {
        IM3Function function = &io_module->functions[io_module->startFunction];

        اگر (not function->compiled) {
_           (CompileFunction(function));
        }

        IM3FuncType ftype = function->funcType;
        اگر (ftype->رقم_Args != 0 || ftype->رقم_Rets != 0) {
            _throw(m3_Err_argument_شمار_Mismatch);
        }

        IM3Module  module  = function->module;
        IM3Runtime runtime = module->runtime;

        startFunctionTmp = io_module->startFunction;

        io_module->startFunction = -1;

        result = RunCodeChecked(runtime, function);

        // a start function that suspended is not to be started again: resuming
        // is what finishes it
        اگر (result and result != m3Err_continuationSuspended) {
            io_module->startFunction = startFunctionTmp;
            EXCEPTION_PRINT(result);
            برو _catch;
        }
    }

    _catch: بازگشت result;
}

// TODO: deal with main + side-modules loading efforcement
M3Result m3_LoadModule (IM3Runtime io_runtime, IM3Module io_module)
{
    M3Result result = m3Err_none;

    اگر (M3_UNLIKELY(io_module->runtime)) {
        بازگشت m3Err_moduleAlreadyLinked;
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
    بازگشت result; // ok

_catch:
    // The runtime owns the module either way. Instantiation may already have
    // written this module's functions into a table another module owns, and the
    // spec keeps whatever it managed to do before the trap, so those entries
    // stay callable - retaining the module is what stops them dangling.
    //
    // It goes on the tail of the list rather than the head: it is not a module
    // anyone should find by name, only one whose functions may still be
    // reachable through someone else's table.
    io_module->next = پوچ;

    IM3Module* tail = &io_runtime->modules;
    وقتی (*tail) {
        tail = &(*tail)->next;
    }
    *tail = io_module;

    بازگشت result;
}

IM3Global m3_FindGlobal (IM3Module         io_module,
                         ثابت نویسه* ثابت i_globalName)
{
    // Search exports
    IM3Global exported = Module_FindExportedGlobal(io_module, i_globalName);
    اگر (exported) {
        // a re-exported global is the one that was imported, so reads and
        // writes have to reach the cell that actually holds the value
        بازگشت exported->resolved ? exported->resolved : exported;
    }

    // Search imports
    برای (u32 i = 0; i < io_module->رقم_Globals; ++i) {
        IM3Global g = &io_module->globals[i];

        اگر (g->import.moduleUtf8 and g->import.fieldUtf8) {
            اگر (strcmp(g->import.fieldUtf8, i_globalName) == 0) {
                بازگشت g->resolved ? g->resolved : g;
            }
        }
    }
    بازگشت پوچ;
}

M3Result m3_GetGlobal (IM3Global      i_global,
                       IM3_Tagged_مقدار o_مقدار)
{
    اگر (not i_global) {
        بازگشت m3Err_globalLookupFailed;
    }

    انتخاب (i_global->type) {
    حالت c_m3Type_i32: o_مقدار->مقدار.i32 = i_global->i32_مقدار; بشکن;
    حالت c_m3Type_i64: o_مقدار->مقدار.i64 = i_global->i64_مقدار; بشکن;
#اگر d_m3HasFloat
    حالت c_m3Type_f32: o_مقدار->مقدار.f32 = i_global->f32_مقدار; بشکن;
    حالت c_m3Type_f64: o_مقدار->مقدار.f64 = i_global->f64_مقدار; بشکن;
#endif
    پیشفرض: بازگشت m3Err_invalidTypeId;
    }

    o_مقدار->type = (M3_مقدار_Type)(i_global->type);
    بازگشت m3Err_none;
}

M3Result m3_SetGlobal (IM3Global            i_global,
                       ثابت IM3_Tagged_مقدار i_مقدار)
{
    اگر (not i_global) {
        بازگشت m3Err_globalLookupFailed;
    }
    اگر (not i_global->isMutable) {
        بازگشت m3Err_globalNotMutable;
    }
    اگر (i_global->type != i_مقدار->type) {
        بازگشت m3Err_globalTypeMismatch;
    }

    انتخاب (i_مقدار->type) {
    حالت c_m3Type_i32: i_global->i32_مقدار = i_مقدار->مقدار.i32; بشکن;
    حالت c_m3Type_i64: i_global->i64_مقدار = i_مقدار->مقدار.i64; بشکن;
#اگر d_m3HasFloat
    حالت c_m3Type_f32: i_global->f32_مقدار = i_مقدار->مقدار.f32; بشکن;
    حالت c_m3Type_f64: i_global->f64_مقدار = i_مقدار->مقدار.f64; بشکن;
#endif
    پیشفرض: بازگشت m3Err_invalidTypeId;
    }

    بازگشت m3Err_none;
}

M3_مقدار_Type m3_GetGlobalType (IM3Global i_global)
{
    بازگشت (i_global) ? (M3_مقدار_Type)(i_global->type) : c_m3Type_none;
}


تهی* v_FindFunction (IM3Module i_module, تهی* i_info)
{
    ثابت نویسه* ثابت i_name = (ثابت نویسه*)i_info;

    // Prefer exported functions
    IM3Function exported = Module_FindExportedFunction(i_module, i_name);
    اگر (exported) {
        بازگشت exported;
    }

    // Search internal functions
    برای (u32 i = 0; i < i_module->رقم_Functions; ++i) {
        IM3Function f = &i_module->functions[i];

        bool isImported = f->import.moduleUtf8 or f->import.fieldUtf8;

        اگر (isImported) {
            ادامه;
        }

        برای (صحیح j = 0; j < f->رقم_Names; j++) {
            اگر (f->names[j] and strcmp(f->names[j], i_name) == 0) {
                بازگشت f;
            }
        }
    }

    بازگشت پوچ;
}


// Shared tail of the two lookups: a function is only usable once it has code.
ایستا
M3Result PrepareFoundFunction (IM3Function* o_function, IM3Function i_function)
{
    M3Result result = m3Err_none;

    اگر (not i_function->compiled) {
_       (CompileFunction(i_function))
    }

    _catch:
    *o_function = result ? پوچ : i_function;

    بازگشت result;
}


// Searches every module in the runtime, most recently loaded first. That is a
// guess once more than one module is loaded and two of them export the same
// name - m3_FindFunctionIn says which module is meant.
M3Result m3_FindFunction (IM3Function* o_function, IM3Runtime i_runtime, ثابت نویسه* ثابت i_functionName)
{
                                                                d_m3Assert (o_function and i_runtime and i_functionName);
    IM3Function function = پوچ;

    اگر (not i_runtime->modules) {
        *o_function = پوچ;
        بازگشت "no modules loaded";
    }

    function = (IM3Function)ForEachModule(i_runtime, v_FindFunction, (تهی*)i_functionName);

    اگر (not function) {
        *o_function = پوچ;
        بازگشت ErrorModule(m3Err_functionLookupFailed, i_runtime->modules, "'%s'", i_functionName);
    }

    بازگشت PrepareFoundFunction(o_function, function);
}


IM3Module m3_FindModule (IM3Runtime i_runtime, ثابت نویسه* ثابت i_moduleName)
{
    اگر (not i_runtime or not i_moduleName) {
        بازگشت پوچ;
    }

    // the list is newest-first, so a name registered twice names the newer one
    برای (IM3Module m = i_runtime->modules; m; m = m->next) {
        اگر (m->name and strcmp(m->name, i_moduleName) == 0) {
            بازگشت m;
        }
    }

    بازگشت پوچ;
}


// Searches one module's exports, which is what naming a module means.
M3Result m3_FindFunctionIn (IM3Function* o_function, IM3Module i_module, ثابت نویسه* ثابت i_functionName)
{
                                                                d_m3Assert (o_function and i_functionName);
    اگر (not i_module) {
        *o_function = پوچ;
        بازگشت m3Err_functionLookupFailed;      // ErrorModule would deref it
    }

    IM3Function function = (IM3Function)v_FindFunction(i_module, (تهی*)i_functionName);

    اگر (not function) {
        *o_function = پوچ;
        بازگشت ErrorModule(m3Err_functionLookupFailed, i_module, "'%s'", i_functionName);
    }

    بازگشت PrepareFoundFunction(o_function, function);
}


M3Result m3_GetTableFunction (IM3Function* o_function, IM3Module i_module, uint32_t i_index)
{
_try {
    M3Table*    table;
    IM3Function function;

    _throwif("no table", i_module->رقم_Tables == 0);

    table = i_module->tables[0];
    _throwif("function index out of range", i_index >= table->size);

    function = (IM3Function)table->elements[i_index];

    اگر (function) {
        اگر (not function->compiled) {
_           (CompileFunction(function))
        }
    }

    *o_function = function;
} _catch:
    بازگشت result;
}


ایستا
M3Result checkStartFunction (IM3Module i_module)
{
    M3Result result = m3Err_none;                               d_m3Assert(i_module);

    // Check if start function needs to be called
    اگر (i_module->startFunction >= 0) {
        result = m3_RunStart(i_module);
    }

    بازگشت result;
}

uint32_t m3_Get_Arg_شمار (IM3Function i_function)
{
    اگر (i_function) {
        IM3FuncType ft = i_function->funcType;
        اگر (ft) {
            بازگشت ft->رقم_Args;
        }
    }
    بازگشت 0;
}

uint32_t m3_Get_Ret_شمار (IM3Function i_function)
{
    اگر (i_function) {
        IM3FuncType ft = i_function->funcType;
        اگر (ft) {
            بازگشت ft->رقم_Rets;
        }
    }
    بازگشت 0;
}


M3_مقدار_Type m3_GetArgType (IM3Function i_function, uint32_t index)
{
    اگر (i_function) {
        IM3FuncType ft = i_function->funcType;
        اگر (ft and index < ft->رقم_Args) {
            بازگشت (M3_مقدار_Type)BaseTypeOf(d_FuncArgType(ft, index));
        }
    }
    بازگشت c_m3Type_none;
}

M3_مقدار_Type m3_GetRetType (IM3Function i_function, uint32_t index)
{
    اگر (i_function) {
        IM3FuncType ft = i_function->funcType;
        اگر (ft and index < ft->رقم_Rets) {
            بازگشت (M3_مقدار_Type)BaseTypeOf(d_FuncRetType(ft, index));
        }
    }
    بازگشت c_m3Type_none;
}


// The call APIs marshal their arguments into the runtime stack before any
// compiled code runs, so op_Entry's overflow check comes too late to cover those
// writes - and for a host function or an import linked to another module there
// is no op_Entry of our own to reach at all. Bound the area about to be written
// here instead: results and arguments each take a u64 at this boundary, however
// wide the slot is inside. Every caller goes through this, so none of them can
// be the one that forgets.
ایستا
M3Result Get_پشته_Pointer_For_Args (IM3Function i_function, u8** o_پشته)
{
    IM3Runtime  runtime = i_function->module->runtime;
    IM3FuncType ftype   = i_function->funcType;

    // the stack a paused invocation holds is not available to this one
    size_t used      = (size_t)((u8*)runtime->پشته - (u8*)runtime->origin_پشته);
    size_t needed    = ((size_t)ftype->رقم_Rets + ftype->رقم_Args) * اندازه(u64);
    size_t available = (size_t)runtime->رقم_پشته_Slots * اندازه(m3slot_t) - used;

    اگر (needed > available) {
        بازگشت m3_Err_trap_پشته_Overflow;
    }

    u64* پشته = (u64*)runtime->پشته;

    پشته += ftype->رقم_Rets;

    *o_پشته = (u8*)پشته;

    بازگشت m3Err_none;
}


M3Result m3_CallV (IM3Function i_function, ...)
{
    va_list ap;
    va_start(ap, i_function);
    M3Result r = m3_CallVL(i_function, ap);
    va_end(ap);
    بازگشت r;
}

ایستا
تهی Report_Native_پشته_Usage ()
{
#اگر d_m3_Log_Native_پشته
    صحیح پشته_Used = m3_پشته_Get_Max();
    fprintf(stderr, "Native stack used: %d\n", پشته_Used);
#endif
}


M3Result m3_CallVL (IM3Function i_function, va_list i_args)
{
    IM3Runtime  runtime = i_function->module->runtime;
    IM3FuncType ftype   = i_function->funcType;
    M3Result    result  = m3Err_none;
    u8*         s       = پوچ;

    اگر (!i_function->compiled) {
        بازگشت m3Err_missingCompiledCode;
    }

#اگر d_m3RecordBacktraces
    ClearBacktrace(runtime);
#endif

    m3_پشته_Check_Init();

_   (checkStartFunction(i_function->module))

_   (Get_پشته_Pointer_For_Args(i_function, &s))

    برای (u32 i = 0; i < ftype->رقم_Args; ++i) {
        انتخاب (d_FuncArgType(ftype, i)) {
        حالت c_m3Type_i32:
            *(i32*)(s) = va_arg(i_args, i32);
            s += 8;
            بشکن;
        حالت c_m3Type_i64:
            *(i64*)(s) = va_arg(i_args, i64);
            s += 8;
            بشکن;
        حالت c_m3Type_funcref:
        حالت c_m3Type_externref:
        حالت c_m3Type_exnref:
            *(uintptr_t*)(s) = va_arg(i_args, uintptr_t);
            s += 8;
            بشکن;
#اگر d_m3HasFloat
        حالت c_m3Type_f32:
            *(f32*)(s) = (f32)va_arg(i_args, f64);
            s += 8;
            بشکن; // f32 is passed as f64
        حالت c_m3Type_f64:
            *(f64*)(s) = va_arg(i_args, f64);
            s += 8;
            بشکن;
#endif
        پیشفرض: بازگشت "unknown argument type";
        }
    }

    result = RunCodeChecked(runtime, i_function);
    Report_Native_پشته_Usage();

    runtime->lastCalled = result ? پوچ : i_function;

    _catch: بازگشت result;
}

M3Result m3_Call (IM3Function i_function, uint32_t i_argc, ثابت تهی* i_argptrs[])
{
    IM3Runtime  runtime = i_function->module->runtime;
    IM3FuncType ftype   = i_function->funcType;
    M3Result    result  = m3Err_none;
    u8*         s       = پوچ;

    اگر (i_argc != ftype->رقم_Args) {
        بازگشت m3_Err_argument_شمار_Mismatch;
    }
    اگر (!i_function->compiled) {
        بازگشت m3Err_missingCompiledCode;
    }

#اگر d_m3RecordBacktraces
    ClearBacktrace(runtime);
#endif

    m3_پشته_Check_Init();

_   (checkStartFunction(i_function->module))

_   (Get_پشته_Pointer_For_Args(i_function, &s))

    برای (u32 i = 0; i < ftype->رقم_Args; ++i) {
        انتخاب (d_FuncArgType(ftype, i)) {
        حالت c_m3Type_i32:
            *(i32*)(s) = *(i32*)i_argptrs[i];
            s += 8;
            بشکن;
        حالت c_m3Type_i64:
            *(i64*)(s) = *(i64*)i_argptrs[i];
            s += 8;
            بشکن;
        حالت c_m3Type_funcref:
        حالت c_m3Type_externref:
        حالت c_m3Type_exnref:
            *(uintptr_t*)(s) = *(uintptr_t*)i_argptrs[i];
            s += 8;
            بشکن;
#اگر d_m3HasFloat
        حالت c_m3Type_f32:
            *(f32*)(s) = *(f32*)i_argptrs[i];
            s += 8;
            بشکن;
        حالت c_m3Type_f64:
            *(f64*)(s) = *(f64*)i_argptrs[i];
            s += 8;
            بشکن;
#endif
        پیشفرض: بازگشت "unknown argument type";
        }
    }

    result = RunCodeChecked(runtime, i_function);

    Report_Native_پشته_Usage();

    runtime->lastCalled = result ? پوچ : i_function;

    _catch: بازگشت result;
}

// Argument parsing for m3_CallArgv. Strict on purpose: the whole string has to
// be consumed, so "12abc" is rejected rather than read as 12, and an empty or
// unparsable argument is an error rather than the zero that strtoul with a
// NULL end pointer used to hand back. See wasm3/wasm3#367.
ایستا
M3Result ParseArgInteger (ccstr_t i_arg, u32 i_رقم_Bits, u64* o_مقدار)
{
    اگر (not i_arg or not *i_arg) {
        بازگشت "empty argument";
    }

    // strtoull would skip leading space, but trailing space is rejected below;
    // accepting one and not the other would just be confusing
    اگر (isspace((unsigned نویسه)*i_arg)) {
        بازگشت "argument is not a number";
    }

    نویسه* end = پوچ;
    u64   مقدار;

    errno = 0;

    // an argument may be spelled signed or unsigned: -1 and 4294967295 name the
    // same i32
    اگر (*i_arg == '-') {
        i64 signed_مقدار = strtoll(i_arg, &end, 10);

        اگر (i_رقم_Bits == 32 and (signed_مقدار < INT32_MIN or signed_مقدار > INT32_MAX)) {
            بازگشت "argument out of range";
        }

        مقدار = (u64)signed_مقدار;
    } وگرنه {
        مقدار = strtoull(i_arg, &end, 10);

        اگر (i_رقم_Bits == 32 and مقدار > UINT32_MAX) {
            بازگشت "argument out of range";
        }
    }

    اگر (errno == ERANGE) {
        بازگشت "argument out of range";
    }

    اگر (end == i_arg or *end) {
        بازگشت "argument is not a number";
    }

    *o_مقدار = مقدار;

    بازگشت m3Err_none;
}


#اگر d_m3HasFloat
ایستا
M3Result ParseArgFloat (ccstr_t i_arg, f64* o_مقدار)
{
    اگر (not i_arg or not *i_arg) {
        بازگشت "empty argument";
    }

    اگر (isspace((unsigned نویسه)*i_arg)) {
        بازگشت "argument is not a number";
    }

    نویسه* end = پوچ;

    errno = 0;

    f64 مقدار = strtod(i_arg, &end);

    اگر (end == i_arg or *end) {
        بازگشت "argument is not a number";
    }

    // strtod reports underflow through ERANGE as well, and a denormal result is
    // perfectly usable, so only an overflow to infinity is out of range
    اگر (errno == ERANGE and (مقدار > DBL_MAX or مقدار < -DBL_MAX)) {
        بازگشت "argument out of range";
    }

    *o_مقدار = مقدار;

    بازگشت m3Err_none;
}
#endif


// A reference argument is either the null reference or a host handle written as
// an integer.
ایستا
M3Result ParseArgReference (ccstr_t i_arg, u64* o_مقدار)
{
    اگر (i_arg and strcmp(i_arg, "null") == 0) {
        *o_مقدار = 0;
        بازگشت m3Err_none;
    }

    بازگشت ParseArgInteger(i_arg, 64, o_مقدار);
}


M3Result m3_CallArgv (IM3Function i_function, uint32_t i_argc, ثابت نویسه* i_argv[])
{
    IM3FuncType ftype   = i_function->funcType;
    IM3Runtime  runtime = i_function->module->runtime;
    M3Result    result  = m3Err_none;
    u8*         s       = پوچ;

    اگر (i_argc != ftype->رقم_Args) {
        بازگشت m3_Err_argument_شمار_Mismatch;
    }
    اگر (!i_function->compiled) {
        بازگشت m3Err_missingCompiledCode;
    }

#اگر d_m3RecordBacktraces
    ClearBacktrace(runtime);
#endif

    m3_پشته_Check_Init();

_   (checkStartFunction(i_function->module))

_   (Get_پشته_Pointer_For_Args(i_function, &s))

    برای (u32 i = 0; i < ftype->رقم_Args; ++i) {
        u64 مقدار = 0;
#اگر d_m3HasFloat
        f64 fvalue = 0;
#endif
        انتخاب (d_FuncArgType(ftype, i)) {
        حالت c_m3Type_i32:
_           (ParseArgInteger(i_argv[i], 32, &مقدار)) * (i32*)(s) = (i32)مقدار;
            s += 8;
            بشکن;
        حالت c_m3Type_i64:
_           (ParseArgInteger(i_argv[i], 64, &مقدار)) * (i64*)(s) = (i64)مقدار;
            s += 8;
            بشکن;
        حالت c_m3Type_funcref:
        حالت c_m3Type_externref:
        حالت c_m3Type_exnref:
_           (ParseArgReference(i_argv[i], &مقدار)) * (uintptr_t*)(s) = (uintptr_t)مقدار;
            s += 8;
            بشکن;
#اگر d_m3HasFloat
            // strtof would be less portable
        حالت c_m3Type_f32:
_           (ParseArgFloat(i_argv[i], &fvalue)) * (f32*)(s) = (f32)fvalue;
            s += 8;
            بشکن;
        حالت c_m3Type_f64:
_           (ParseArgFloat(i_argv[i], &fvalue)) * (f64*)(s) = fvalue;
            s += 8;
            بشکن;
#endif
        پیشفرض: _throw("unknown argument type");
        }
    }

    result = RunCodeChecked(runtime, i_function);

    Report_Native_پشته_Usage();

    runtime->lastCalled = result ? پوچ : i_function;

    _catch: بازگشت result;
}


//u8 * AlignStackPointerTo64Bits (const u8 * i_stack)
//{
//    uintptr_t ptr = (uintptr_t) i_stack;
//    return (u8 *) ((ptr + 7) & ~7);
//}


M3Result m3_GetResults (IM3Function i_function, uint32_t i_retc, ثابت تهی* o_retptrs[])
{
    IM3FuncType ftype   = i_function->funcType;
    IM3Runtime  runtime = i_function->module->runtime;

    اگر (i_retc != ftype->رقم_Rets) {
        بازگشت m3_Err_argument_شمار_Mismatch;
    }
    اگر (i_function != runtime->lastCalled) {
        بازگشت "function not called";
    }

    u8* s = (u8*)runtime->پشته;

    برای (u32 i = 0; i < ftype->رقم_Rets; ++i) {
        انتخاب (d_FuncRetType(ftype, i)) {
        حالت c_m3Type_i32:
            *(i32*)o_retptrs[i] = *(i32*)(s);
            s += 8;
            بشکن;
        حالت c_m3Type_i64:
            *(i64*)o_retptrs[i] = *(i64*)(s);
            s += 8;
            بشکن;
        حالت c_m3Type_funcref:
        حالت c_m3Type_externref:
        حالت c_m3Type_exnref:
            *(uintptr_t*)o_retptrs[i] = *(uintptr_t*)(s);
            s += 8;
            بشکن;
#اگر d_m3HasFloat
        حالت c_m3Type_f32:
            *(f32*)o_retptrs[i] = *(f32*)(s);
            s += 8;
            بشکن;
        حالت c_m3Type_f64:
            *(f64*)o_retptrs[i] = *(f64*)(s);
            s += 8;
            بشکن;
#endif
        پیشفرض: بازگشت "unknown return type";
        }
    }
    بازگشت m3Err_none;
}

M3Result m3_GetResultsV (IM3Function i_function, ...)
{
    va_list ap;
    va_start(ap, i_function);
    M3Result r = m3_GetResultsVL(i_function, ap);
    va_end(ap);
    بازگشت r;
}

M3Result m3_GetResultsVL (IM3Function i_function, va_list o_rets)
{
    IM3Runtime  runtime = i_function->module->runtime;
    IM3FuncType ftype   = i_function->funcType;

    اگر (i_function != runtime->lastCalled) {
        بازگشت "function not called";
    }

    u8* s = (u8*)runtime->پشته;
    برای (u32 i = 0; i < ftype->رقم_Rets; ++i) {
        انتخاب (d_FuncRetType(ftype, i)) {
        حالت c_m3Type_i32:
            *va_arg(o_rets, i32*) = *(i32*)(s);
            s += 8;
            بشکن;
        حالت c_m3Type_i64:
            *va_arg(o_rets, i64*) = *(i64*)(s);
            s += 8;
            بشکن;
        حالت c_m3Type_funcref:
        حالت c_m3Type_externref:
        حالت c_m3Type_exnref:
            *va_arg(o_rets, uintptr_t*) = *(uintptr_t*)(s);
            s += 8;
            بشکن;
#اگر d_m3HasFloat
        حالت c_m3Type_f32:
            *va_arg(o_rets, f32*) = *(f32*)(s);
            s += 8;
            بشکن;
        حالت c_m3Type_f64:
            *va_arg(o_rets, f64*) = *(f64*)(s);
            s += 8;
            بشکن;
#endif
        پیشفرض: بازگشت "unknown argument type";
        }
    }
    بازگشت m3Err_none;
}

تهی ReleaseCodePageNoTrack (IM3Runtime i_runtime, IM3CodePage i_codePage)
{
    اگر (i_codePage) {
        IM3CodePage* list;

        bool pageFull = (رقم_Free_سطرها(i_codePage) < d_m3_Code_Page_Free_سطرها_Threshold);
        اگر (pageFull) {
            list = &i_runtime->pagesFull;
        } وگرنه {
            list = &i_runtime->pagesOpen;
        }

        افزودن_Code_Page(list, i_codePage);                         m3log (emit, "release page: %d to queue: '%s'", i_codePage->info.sequence, pageFull ? "full" : "open")
    }
}


IM3CodePage AcquireCodePageWithCapacity (IM3Runtime i_runtime, u32 i_min_سطر_شمار)
{
    IM3CodePage page = RemoveCodePageOfCapacity(&i_runtime->pagesOpen, i_min_سطر_شمار);

    اگر (not page) {
        page = Environment_AcquireCodePage(i_runtime->environment, i_min_سطر_شمار);

        اگر (not page) {
            page = NewCodePage(i_runtime, i_min_سطر_شمار);
        }

        اگر (page) {
            i_runtime->رقم_Code_Pages++;
        }
    }

    اگر (page) {                                                  m3log (emit, "acquire page: %d", page->info.sequence);
        i_runtime->رقم_Active_Code_Pages++;
    }

    بازگشت page;
}


IM3CodePage AcquireCodePage (IM3Runtime i_runtime)
{
    بازگشت AcquireCodePageWithCapacity(i_runtime, d_m3_Code_Page_Free_سطرها_Threshold);
}


تهی ReleaseCodePage (IM3Runtime i_runtime, IM3CodePage i_codePage)
{
    اگر (i_codePage) {
        ReleaseCodePageNoTrack(i_runtime, i_codePage);
        i_runtime->رقم_Active_Code_Pages--;

#اگر defined(DEBUG)
        u32 رقم_Open = شمار_Code_Pages(i_runtime->pagesOpen);
        u32 رقم_Full = شمار_Code_Pages(i_runtime->pagesFull);

        m3log(runtime, "runtime: %p; open-pages: %d; full-pages: %d; active: %d; total: %d", i_runtime, رقم_Open, رقم_Full, i_runtime->رقم_Active_Code_Pages, i_runtime->رقم_Code_Pages);

        d_m3Assert(رقم_Open + رقم_Full + i_runtime->رقم_Active_Code_Pages == i_runtime->رقم_Code_Pages);

#  اگر d_m3LogCodePages
        dump_code_page(i_codePage, /* startPC: */ پوچ);
#  endif
#endif
    }
}


#اگر d_m3VerboseErrorMessages
M3Result m3Error (M3Result i_result, IM3Runtime i_runtime, IM3Module i_module, IM3Function i_function,
                  ثابت نویسه* ثابت i_file, u32 i_سطر_رقم, ثابت نویسه* ثابت i_errorMessage, ...)
{
    اگر (i_runtime) {
        i_runtime->error         = (M3ErrorInfo){ .result = i_result, .runtime = i_runtime, .module = i_module, .function = i_function, .file = i_file, .سطر_2 = i_سطر_رقم };
        i_runtime->error.message = i_runtime->error_message;

        va_list args;
        va_start(args, i_errorMessage);
        vsnprintf(i_runtime->error_message, اندازه(i_runtime->error_message), i_errorMessage, args);
        va_end(args);
    }

    بازگشت i_result;
}
#endif


تهی m3_GetErrorInfo (IM3Runtime i_runtime, M3ErrorInfo* o_info)
{
    اگر (i_runtime) {
        *o_info = i_runtime->error;
        m3_ResetErrorInfo(i_runtime);
    }
}


تهی m3_ResetErrorInfo (IM3Runtime i_runtime)
{
    اگر (i_runtime) {
        M3_INIT(i_runtime->error);
        i_runtime->error.message = "";
    }
}

uint8_t* m3_GetMemory (IM3Module i_module, size_t* o_memorySizeInBytes, uint32_t i_memoryIndex)
{
    uint8_t* memory = پوچ;
    size_t   size   = 0;

    اگر (i_module and i_memoryIndex < i_module->رقم_Memories) {
        IM3Memory mem = i_module->memories[i_memoryIndex];

        اگر (mem->mallocated) {
            size = mem->mallocated->length;

            اگر (size) {
                memory = m3MemData(mem->mallocated);
            }
        }
    }

    اگر (o_memorySizeInBytes) {
        *o_memorySizeInBytes = size;
    }

    بازگشت memory;
}


size_t m3_GetMemorySize (IM3Module i_module, uint32_t i_memoryIndex)
{
    اگر (not i_module or i_memoryIndex >= i_module->رقم_Memories) {
        بازگشت 0;
    }

    IM3Memory mem = i_module->memories[i_memoryIndex];

    بازگشت mem->mallocated ? mem->mallocated->length : 0;
}


size_t m3_GetMemorySizeAt (ثابت تهی* i_memory)
{
    اگر (not i_memory) {
        بازگشت 0;
    }

    // the header sits immediately before the data it describes
    ثابت M3MemoryHeader* header = ((ثابت M3MemoryHeader*)i_memory) - 1;

    بازگشت header->length;
}


M3Result m3_FindExportedMemory (IM3Module i_module, cstr_t i_name, u32* o_memoryIndex)
{
    اگر (not i_module or not i_name or not o_memoryIndex) {
        بازگشت m3Err_unknownMemory;
    }

    ثابت M3Export* e = Module_FindExport(i_module, d_externalKind_memory, i_name);

    اگر (not e) {
        بازگشت m3Err_unknownMemory;
    }

    *o_memoryIndex = e->index;
    بازگشت m3Err_none;
}


bool Module_HasLinkedHostImport (IM3Module i_module, cstr_t i_importModule)
{
    برای (u32 i = 0; i < i_module->رقم_Functions; ++i) {
        IM3Function f = &i_module->functions[i];

        // hostMemory is set when a host function is bound to the import, so it
        // is also the mark of one - see CompileRawFunction
        اگر (not f->hostMemory or f->wasm or f->resolved) {
            ادامه;
        }

        اگر (f->import.moduleUtf8 and strcmp(f->import.moduleUtf8, i_importModule) == 0) {
            بازگشت true;
        }
    }

    بازگشت false;
}


M3Result m3_BindImportMemory (IM3Module io_module, cstr_t i_importModule, u32 i_memoryIndex)
{
    اگر (not io_module or not i_importModule) {
        بازگشت m3Err_moduleNotLinked;
    }

    اگر (i_memoryIndex >= io_module->رقم_Memories) {
        بازگشت m3Err_unknownMemory;
    }

    // the memory itself, not the index: linking may have pointed this slot at
    // another module's memory, and growing one reallocates behind it
    IM3Memory memory = io_module->memories[i_memoryIndex];

    ثابت bool wildcardModule = (strcmp(i_importModule, "*") == 0);

    برای (u32 i = 0; i < io_module->رقم_Functions; ++i) {
        IM3Function f = &io_module->functions[i];

        // only an import that a host function was bound to has a memory to
        // pin: one with a body of its own reads memory through the interpreter,
        // and one resolved to another module's export runs that module's code
        اگر (not f->hostMemory or f->wasm or f->resolved) {
            ادامه;
        }

        اگر (wildcardModule or (f->import.moduleUtf8 and strcmp(f->import.moduleUtf8, i_importModule) == 0)) {
            f->hostMemory = memory;
        }
    }

    بازگشت m3Err_none;
}


M3BacktraceInfo* m3_GetBacktrace (IM3Runtime i_runtime)
{
#اگر d_m3RecordBacktraces
    بازگشت &i_runtime->backtrace;
#وگرنه
    بازگشت پوچ;
#endif
}


//-------------------------------------------------------------------------------------------------------------------------------
//  snapshots & suspendable execution
//-------------------------------------------------------------------------------------------------------------------------------

تهی m3_SetSuspendable (IM3Runtime io_runtime, bool i_suspendable)
{
    اگر (io_runtime) {
        io_runtime->isSuspendable = i_suspendable;
    }
}

تهی m3_RequestSuspend (IM3Runtime io_runtime)
{
    اگر (io_runtime) {
        io_runtime->suspendRequested = true;
    }
}

bool m3_IsSuspended (IM3Runtime i_runtime)
{
    اگر (!i_runtime) {
        بازگشت false;
    }
#اگر d_m3_Has_پشته_Switching
    اگر (i_runtime->ریشه_Continuation && i_runtime->ریشه_Continuation->state == cont_suspended) {
        بازگشت true;
    }
#endif
    بازگشت false;
}

M3Result m3_ResumeRuntime (IM3Runtime io_runtime)
{
    اگر (!io_runtime) {
        بازگشت m3Err_mallocFailed;
    }
    اگر (!io_runtime->isSuspendable) {
        بازگشت m3Err_none;
    }
#اگر d_m3HasSnapshots
    // a restore that failed leaves nothing to resume, and says so every time
    // rather than resuming nothing successfully
    اگر (io_runtime->ریشه_Continuation and io_runtime->ریشه_Continuation->entryFunction and
        io_runtime->ریشه_Continuation->entryFunction->module->isUnusable) {
        بازگشت "a snapshot failed to restore into the module, so it cannot run";
    }
#endif
#اگر d_m3_Has_پشته_Switching
    اگر (!io_runtime->ریشه_Continuation || io_runtime->ریشه_Continuation->state != cont_suspended) {
        بازگشت m3Err_none;
    }
    // what finishes returns into the bottom of the stack, where it was entered
    io_runtime->پشته = io_runtime->origin_پشته;

    // this is the outermost call again, as far as a host function calling back
    // into Wasm from inside it can tell
    io_runtime->entered = io_runtime->ریشه_Continuation->entryFunction;
    io_runtime->callNesting++;

    d_m3_پشته_Limit_Enter(io_runtime);
    io_runtime->resumePastCheck = true;
    M3Result result             = (M3Result)ResumeContinuation(io_runtime, io_runtime->ریشه_Continuation);
    io_runtime->resumePastCheck = false;
    d_m3_پشته_Limit_Leave(io_runtime);

#  اگر d_m3HasExceptionHandling
    اگر (M3_UNLIKELY(result == m3Err_pendingException)) {
        result = m3Err_trapUncaughtException;
    }
    RecordUncaughtException(io_runtime, result);
#  endif

    io_runtime->callNesting--;

#  اگر d_m3HasExceptionHandling
    اگر (result != m3Err_continuationSuspended and io_runtime->callNesting == 0) {
        FreeExceptions(io_runtime);
    }
#  endif

    // the results are where the paused call left room for them, so a finished
    // resume can be asked for them the way the call itself could have been
    io_runtime->lastCalled = (result == m3Err_none) ? io_runtime->ریشه_Continuation->entryFunction : پوچ;

    Runtime_Place_Call_پشته(io_runtime);
    بازگشت result;
#وگرنه
    بازگشت m3Err_none;
#endif
}
