//
//  m3_env.c
//
//  Created by Steven Massey on 4/19/19.
//  Copyright © 2019 Steven Massey. All rights reserved.
//

#ضمن <stdarg.h>
#ضمن <limits.h>
#ضمن <errno.h>
#ضمن <float.h>
#ضمن <ctype.h>

#ضمن "m3_env.h"
#ضمن "m3_bind.h"
#ضمن "m3_deterministic.h"
#ضمن "m3_compile.h"
#ضمن "m3_host.h"
#ضمن "m3_exception.h"
#ضمن "m3_info.h"


IM3Environment m3_NewEnvironment ()
{
    IM3Environment env = m3_AllocStruct(M3Environment);

    إذا (env) {
        _try
        {
            // create FuncTypes for all simple block return ValueTypes.
            // v128 is skipped: it parses as a slot but has no operations.
            لكل (u8 t = c_m3Type_none; t < c_m3_Type_کيراءن; t++) {
                إذا (t == c_m3Type_v128) {
                    استمر;
                }

                IM3FuncType ftype;
_               (AllocFuncType(&ftype, 1));

                ftype->اڠک_Args  = 0;
                ftype->اڠک_Rets  = (t == c_m3Type_none) ? 0 : 1;
                ftype->types[0] = t;

                Environment_AddFuncType(env, &ftype);

                env->retFuncTypes[t] = ftype;
            }
        }

        _catch:
        إذا (result) {
            m3_FreeEnvironment(env);
            env = عدم;
        }
    }

    أرجع env;
}


فراغ Environment_Release (IM3Environment i_environment)
{
    IM3FuncType ftype = i_environment->funcTypes;

    طالما (ftype) {
        IM3FuncType next = ftype->next;
        m3_Free(ftype);
        ftype = next;
    }

    m3log(runtime, "freeing %d pages from environment", کيراءن_Code_Pages(i_environment->pagesReleased));
    FreeCodePages(&i_environment->pagesReleased);
}


فراغ m3_FreeEnvironment (IM3Environment i_environment)
{
    إذا (i_environment) {
        Environment_Release(i_environment);
        m3_Free(i_environment);
    }
}


فراغ m3_SetCustomSectionHandler (IM3Environment i_environment, M3SectionHandler i_handler)
{
    إذا (i_environment) {
        i_environment->customSectionHandler = i_handler;
    }
}


// returns the same io_funcType or replaces it with an equivalent that's already in the type linked list
M3Result Environment_AddFuncType (IM3Environment i_environment, IM3FuncType* io_funcType)
{
    IM3FuncType addType = *io_funcType;
    IM3FuncType newType = i_environment->funcTypes;

    طالما (newType) {
        إذا (not newType->inRecGroup and AreFuncTypesEqual(newType, addType)) {
            m3_Free(addType);
            اكسر;
        }

        newType = newType->next;
    }

    إذا (newType == عدم) {
        // a type index has to fit in the heap type field of an m3type_t
        إذا (i_environment->اڠک_Func_Types >= d_m3_Max_Sane_Types_کيراءن) {
            m3_Free(addType);
            *io_funcType = عدم;
            أرجع "too many distinct function types";
        }

        newType                  = addType;
        newType->canonicalIndex  = i_environment->اڠک_Func_Types++;
        newType->next            = i_environment->funcTypes;
        i_environment->funcTypes = newType;
    }

    *io_funcType = newType;

    أرجع m3Err_none;
}


// Adds a type without looking for an equal one first, and hands back the index
// it was given.
//
// Members of a recursive group do not get to share an entry with a type that
// merely looks like them: the group is their identity, so two groups spelled
// the same way are still two groups, and a type inside one is never the type
// outside it that matches field for field. Reserving the indices up front is
// also what lets the members refer to each other while they are being read.
M3Result Environment_ReserveFuncTypes (IM3Environment i_environment, u32 i_کيراءن, u16* o_firstIndex)
{
    إذا ((u64)i_environment->اڠک_Func_Types + i_کيراءن > d_m3_Max_Sane_Types_کيراءن) {
        أرجع "too many distinct function types";
    }

    *o_firstIndex = i_environment->اڠک_Func_Types;
    i_environment->اڠک_Func_Types += (u16)i_کيراءن;

    أرجع m3Err_none;
}


فراغ Environment_AdoptFuncType (IM3Environment i_environment, IM3FuncType i_funcType, u16 i_canonicalIndex)
{
    i_funcType->inRecGroup     = true;
    i_funcType->canonicalIndex = i_canonicalIndex;
    i_funcType->next           = i_environment->funcTypes;
    i_environment->funcTypes   = i_funcType;
}


IM3CodePage RemoveCodePageOfCapacity (M3CodePage** io_list, u32 i_minimum_باريس_کيراءن)
{
    IM3CodePage prev = عدم;
    IM3CodePage page = *io_list;

    طالما (page) {
        إذا (اڠک_Free_باريس_سموا(page) >= i_minimum_باريس_کيراءن) {             d_m3Assert (page->info.usage_کيراءن == 0);
            IM3CodePage next = page->info.next;
            إذا (prev) {
                prev->info.next = next; // mid-list
            } وإلا {
                *io_list = next;       // front of list
            }

            اكسر;
        }

        prev = page;
        page = page->info.next;
    }

    أرجع page;
}


IM3CodePage Environment_AcquireCodePage (IM3Environment i_environment, u32 i_minimum_باريس_کيراءن)
{
    أرجع RemoveCodePageOfCapacity(&i_environment->pagesReleased, i_minimum_باريس_کيراءن);
}


فراغ Environment_ReleaseCodePages (IM3Environment i_environment, IM3CodePage i_codePageList)
{
    IM3CodePage end = i_codePageList;

    طالما (end) {
        end->info.باريس_Index = 0; // reset page
#إذا d_m3RecordBacktraces
        end->info.mapping->size = 0;
#endif // d_m3RecordBacktraces

        IM3CodePage next = end->info.next;
        إذا (not next) {
            اكسر;
        }

        end = next;
    }

    إذا (end) {
        // push list to front
        end->info.next               = i_environment->pagesReleased;
        i_environment->pagesReleased = i_codePageList;
    }
}


IM3Runtime m3_NewRuntime (IM3Environment i_environment, u32 i_تيندنن_Size_In_Bytes, فراغ* i_userdata)
{
    IM3Runtime runtime = m3_AllocStruct(M3Runtime);

    إذا (runtime) {
        m3_ResetErrorInfo(runtime);

        runtime->environment = i_environment;
        runtime->userdata    = i_userdata;

        Deterministic_InitRuntime(runtime);

        runtime->origin_تيندنن = m3_Malloc("Wasm Stack", i_تيندنن_Size_In_Bytes + 4 * حجم(m3slot_t)); // TODO: more precise stack checks

        إذا (runtime->origin_تيندنن) {
            runtime->تيندنن         = runtime->origin_تيندنن;
            runtime->اڠک_تيندنن_Slots = i_تيندنن_Size_In_Bytes / حجم(m3slot_t);          m3log (runtime, "new stack: %p, slots: %u", runtime->origin_تيندنن, runtime->اڠک_تيندنن_Slots);
        } وإلا {
            m3_Free(runtime);
        }
    }

    أرجع runtime;
}

فراغ m3_SetValidation (IM3Runtime i_runtime, bool i_enable)
{
#إذا d_m3EnableValidation
    إذا (i_runtime) {
        i_runtime->skipValidation = not i_enable;
    }
#وإلا
    (فراغ)i_runtime;
    (فراغ)i_enable;              // nothing to skip: the validator was compiled out
#endif
}

M3Result m3_SetResourceLimit (IM3Runtime i_runtime, M3ResourceLimit i_limit, uint64_t i_نيلاي)
{
    إذا (not i_runtime) {
        أرجع m3Err_mallocFailed;
    }
    بدل (i_limit) {
    حالة c_m3Limit_MemoryBytes:
        إذا (i_نيلاي and i_نيلاي < i_runtime->memoryBytesUsed) {
            أرجع m3Err_resourceLimitBelowUsage;
        }
        i_runtime->memoryBytesLimit = i_نيلاي;
        أرجع m3Err_none;
    حالة c_m3Limit_TableElements:
        إذا (i_نيلاي and i_نيلاي < i_runtime->tableElementsUsed) {
            أرجع m3Err_resourceLimitBelowUsage;
        }
        i_runtime->tableElementsLimit = i_نيلاي;
        أرجع m3Err_none;
#إذا d_m3_Has_تيندنن_Switching
    حالة c_m3Limit_Continuations:
        إذا (i_نيلاي and i_نيلاي < i_runtime->continuationsAllocated) {
            أرجع m3Err_resourceLimitBelowUsage;
        }
        i_runtime->continuationsLimit = (u32)M3_MIN(i_نيلاي, (u64)UINT32_MAX);
        طالما (i_نيلاي and i_runtime->val_تيندنن_Pool_کيراءن > i_نيلاي - i_runtime->continuationsAllocated) {
            m3_Free(i_runtime->val_تيندنن_Pool[--i_runtime->val_تيندنن_Pool_کيراءن]);
        }
        أرجع m3Err_none;
#وإلا
    حالة c_m3Limit_Continuations: أرجع m3Err_resourceLimitNotSupported;
#endif
    حالة c_m3Limit_GasUnits:
#إذا d_m3HasGasMetering
        i_runtime->gasLimit = i_runtime->gasRemaining = (i64)M3_MIN(i_نيلاي, (u64)INT64_MAX);
        أرجع m3Err_none;
#وإلا
        أرجع m3Err_resourceLimitNotSupported;
#endif
    افتراضي: أرجع m3Err_unknownResourceLimit;
    }
}

uint64_t m3_GetResourceLimit (IM3Runtime i_runtime, M3ResourceLimit i_limit)
{
    إذا (not i_runtime) {
        أرجع 0;
    }
    بدل (i_limit) {
    حالة c_m3Limit_MemoryBytes: أرجع i_runtime->memoryBytesLimit;
    حالة c_m3Limit_TableElements: أرجع i_runtime->tableElementsLimit;
#إذا d_m3_Has_تيندنن_Switching
    حالة c_m3Limit_Continuations: أرجع i_runtime->continuationsLimit;
#endif
#إذا d_m3HasGasMetering
    حالة c_m3Limit_GasUnits: أرجع (u64)i_runtime->gasLimit;
#endif
    افتراضي: أرجع 0;
    }
}

uint64_t m3_GetResourceUsage (IM3Runtime i_runtime, M3ResourceLimit i_limit)
{
    إذا (not i_runtime) {
        أرجع 0;
    }
    بدل (i_limit) {
    حالة c_m3Limit_MemoryBytes: أرجع i_runtime->memoryBytesUsed;
    حالة c_m3Limit_TableElements: أرجع i_runtime->tableElementsUsed;
#إذا d_m3_Has_تيندنن_Switching
    حالة c_m3Limit_Continuations: أرجع i_runtime->continuationsAllocated;
#endif
#إذا d_m3HasGasMetering
    حالة c_m3Limit_GasUnits: أرجع (u64)i_runtime->gasLimit - (u64)i_runtime->gasRemaining;
#endif
    افتراضي: أرجع 0;
    }
}

فراغ* m3_GetUserData (IM3Runtime i_runtime)
{
    أرجع i_runtime ? i_runtime->userdata : عدم;
}


فراغ* ForEachModule (IM3Runtime i_runtime, ModuleVisitor i_visitor, فراغ* i_info)
{
    فراغ* r = عدم;

    IM3Module module = i_runtime->modules;

    طالما (module) {
        IM3Module next = module->next;
        إذا ((r = i_visitor(module, i_info))) {
            اكسر;
        }

        module = next;
    }

    أرجع r;
}


فراغ* _FreeModule (IM3Module i_module, فراغ* i_info)
{
    m3_FreeModule(i_module);
    أرجع عدم;
}


// A module's memory and table index spaces hold borrowed pointers: linking
// repoints an imported slot at the memory or table the exporting module owns.
// m3_FreeModule tells a borrowed slot from an owned one by reading that owner,
// which only holds while the object is still allocated - and the free walk
// below visits modules in load order, with no ordering between an owner and
// the modules borrowing from it. So clear the borrowed slots first, in a pass
// that frees nothing; the walk then skips them on the NULL check it already
// does, instead of reading a memory some earlier module has freed.
فراغ* _ReleaseBorrowedSlots (IM3Module i_module, فراغ* i_info)
{
    لكل (u32 i = 0; i < i_module->اڠک_Memories; ++i) {
        IM3Memory memory = i_module->memories[i];

        إذا (memory and memory->owner != i_module) {
            i_module->memories[i] = عدم;
        }
    }

    لكل (u32 i = 0; i < i_module->اڠک_Tables; ++i) {
        IM3Table table = i_module->tables[i];

        إذا (table and table->owner != i_module) {
            i_module->tables[i] = عدم;
        }
    }

    أرجع عدم;
}


فراغ Runtime_Release (IM3Runtime i_runtime)
{
    ForEachModule(i_runtime, _ReleaseBorrowedSlots, عدم);
    ForEachModule(i_runtime, _FreeModule, عدم);                    d_m3Assert (i_runtime->اڠک_Active_Code_Pages == 0);

    Environment_ReleaseCodePages(i_runtime->environment, i_runtime->pagesOpen);
    Environment_ReleaseCodePages(i_runtime->environment, i_runtime->pagesFull);

    m3_Free(i_runtime->origin_تيندنن);

#إذا d_m3EnableValidation
    m3_Free(i_runtime->validator);
#endif

#إذا d_m3HasExceptionHandling
    // a runtime let go of while suspended still holds its exceptions
    FreeExceptions(i_runtime);
    m3_Free(i_runtime->uncaughtException);
#endif
#إذا d_m3HasExceptionHandling || d_m3_Has_تيندنن_Switching
    طالما (i_runtime->hostTags) {
        M3HostTag* next = i_runtime->hostTags->next;
        m3_Free(i_runtime->hostTags);
        i_runtime->hostTags = next;
    }
#endif
#إذا d_m3_Has_تيندنن_Switching
    Continuation_ReleaseAll(i_runtime);
#endif
}


فراغ m3_FreeRuntime (IM3Runtime i_runtime)
{
    إذا (i_runtime) {
        m3_PrintProfilerInfo();

        Runtime_Release(i_runtime);
        m3_Free(i_runtime);
    }
}


#إذا d_m3_Has_تيندنن_Switching

IM3Continuation Continuation_New (IM3Runtime i_runtime, IM3FuncType i_type, IM3Function i_function)
{
    IM3Continuation cont = m3_AllocStruct(M3Continuation);
    إذا (!cont) {
        أرجع عدم;
    }

    cont->type          = i_type;
    cont->entryFunction = i_function;
    cont->state         = cont_allocated;

    cont->اڠک_تيندنن_Slots = d_m3_Continuation_تيندنن_Slots;
    // frames are recorded only when this continuation actually suspends, and
    // only as deep as it got, so the array starts out unallocated
    cont->frames    = عدم;
    cont->framesCap = 0;
    cont->اڠک_Frames = 0;

    cont->sp = cont->val_تيندنن;
    cont->pc = i_function ? i_function->compiled : عدم;
    cont->r0 = 0;
#  إذا d_m3HasFloat
    cont->fp0 = 0;
#  endif
    cont->bound_Args_کيراءن    = 0;
    cont->اڠک_Suspend_Results = 0;
    cont->اڠک_Handlers       = 0;
    cont->handlersPC        = عدم;
    cont->parent            = عدم;

    cont->next               = i_runtime->continuations;
    i_runtime->continuations = cont;

    أرجع cont;
}

IM3Continuation Continuation_ForkSuspended (IM3Runtime i_runtime, IM3Continuation i_cont)
{
    IM3Continuation newCont = Continuation_New(i_runtime, i_cont->type, i_cont->entryFunction);
    إذا (M3_UNLIKELY(not newCont)) {
        أرجع عدم;
    }

    newCont->val_تيندنن      = i_cont->val_تيندنن;
    newCont->اڠک_تيندنن_Slots = i_cont->اڠک_تيندنن_Slots;
    newCont->sp            = i_cont->sp;
    newCont->pc            = i_cont->pc;
    newCont->suspendPoint  = i_cont->suspendPoint;
    newCont->r0            = i_cont->r0;
#  إذا d_m3HasFloat
    newCont->fp0 = i_cont->fp0;
#  endif
    newCont->frames    = i_cont->frames;
    newCont->framesCap = i_cont->framesCap;
    newCont->اڠک_Frames = i_cont->اڠک_Frames;

    newCont->bound_Args_کيراءن = 0;
#  إذا d_m3HasExceptionHandling
    newCont->resumeThrow = i_cont->resumeThrow;
    i_cont->resumeThrow  = عدم;
#  endif
    newCont->اڠک_Suspend_Results = i_cont->اڠک_Suspend_Results;
    لكل (u32 i = 0; i < i_cont->اڠک_Suspend_Results; ++i) {
        newCont->suspendResultOffsets[i] = i_cont->suspendResultOffsets[i];
        newCont->suspendResultIs64[i]    = i_cont->suspendResultIs64[i];
    }

    newCont->اڠک_Handlers = i_cont->اڠک_Handlers;
    newCont->handlersPC  = i_cont->handlersPC;
    newCont->parent      = i_cont->parent;
    newCont->state       = cont_suspended;

    i_cont->val_تيندنن  = عدم;
    i_cont->frames    = عدم;
    i_cont->framesCap = 0;
    i_cont->اڠک_Frames = 0;
    i_cont->state     = cont_consumed;

    أرجع newCont;
}

// Only cont.new and snapshot restoration acquire stacks. Binding and suspension
// transfer them between records without changing the active count.
M3Result Continuation_Acquire_تيندنن (IM3Runtime runtime, IM3Continuation cont)
{
    إذا (not Continuation_Can_Acquire_تيندنن(runtime)) {
        أرجع m3Err_continuationLimitExceeded;
    }
    إذا (runtime->val_تيندنن_Pool_کيراءن) {
        cont->val_تيندنن = runtime->val_تيندنن_Pool[--runtime->val_تيندنن_Pool_کيراءن];
        memset(cont->val_تيندنن, 0, (cont->اڠک_تيندنن_Slots + 4) * حجم(m3slot_t));
    } وإلا {
        cont->val_تيندنن = m3_AllocArray(m3slot_t, cont->اڠک_تيندنن_Slots + 4);
    }
    إذا (not cont->val_تيندنن) {
        أرجع m3Err_mallocFailed;
    }
    cont->sp = cont->val_تيندنن;
    runtime->continuationsAllocated++;
    أرجع m3Err_none;
}

فراغ Continuation_Recycle_تيندنن (IM3Runtime runtime, IM3Continuation cont)
{
    إذا (not cont->val_تيندنن or cont == runtime->اکر_Continuation) {
        أرجع;
    }
    إذا (not runtime->val_تيندنن_Pool) {
        runtime->val_تيندنن_Pool = m3_AllocArray(m3slot_t*, 16);
        إذا (runtime->val_تيندنن_Pool) {
            runtime->val_تيندنن_Pool_Cap = 16;
        }
    }
    إذا (runtime->val_تيندنن_Pool_کيراءن < runtime->val_تيندنن_Pool_Cap and
        (not runtime->continuationsLimit or runtime->val_تيندنن_Pool_کيراءن <
                                              runtime->continuationsLimit - runtime->continuationsAllocated + 1)) {
        runtime->val_تيندنن_Pool[runtime->val_تيندنن_Pool_کيراءن++] = cont->val_تيندنن;
        cont->val_تيندنن                                      = عدم;
    } وإلا {
        m3_Free(cont->val_تيندنن);
    }
    cont->sp = عدم;
    runtime->continuationsAllocated--;
}

فراغ Continuation_ReleaseAll (IM3Runtime io_runtime)
{
    // Records live until teardown: a consumed reference can still be in a table
    // or on a Wasm stack. Its value stack has already been recycled.
    IM3Continuation cont = io_runtime->continuations;

    طالما (cont) {
        IM3Continuation next = cont->next;

        m3_Free(cont->val_تيندنن);
        m3_Free(cont->frames);
        m3_Free(cont);

        cont = next;
    }

    لكل (u32 i = 0; i < io_runtime->val_تيندنن_Pool_کيراءن; ++i) {
        m3_Free(io_runtime->val_تيندنن_Pool[i]);
    }
    m3_Free(io_runtime->val_تيندنن_Pool);
    io_runtime->val_تيندنن_Pool_کيراءن = io_runtime->val_تيندنن_Pool_Cap = 0;
    io_runtime->continuationsAllocated                          = 0;
    io_runtime->continuations                                   = عدم;
    io_runtime->activeContinuation                              = عدم;
    io_runtime->اکر_Continuation                                = عدم;
}


m3ret_t Continuation_RecordFrame (IM3Runtime i_runtime, ثابت M3Frame* i_frame)
{
    IM3Continuation cont = i_runtime->activeContinuation;

    // the marker only travels between a suspend and the resume that runs it,
    // and that whole stretch has an active continuation
    d_m3Assert(cont);

    إذا (cont->اڠک_Frames == cont->framesCap) {
        u32 newCap = cont->framesCap ? cont->framesCap * 2 : 8;

        إذا (newCap > d_m3ContinuationMaxFrames) {
            newCap = d_m3ContinuationMaxFrames;
        }

        إذا (newCap == cont->framesCap) {
            أرجع m3_Err_trap_تيندنن_Overflow;
        }

        M3Frame* frames = m3_ReallocArray(M3Frame, cont->frames, newCap, cont->framesCap);

        إذا (not frames) {
            أرجع m3Err_mallocFailed;
        }

        cont->frames    = frames;
        cont->framesCap = newCap;
    }

    cont->frames[cont->اڠک_Frames++] = *i_frame;

    أرجع m3Err_continuationSuspended;
}

#endif // d_m3HasStackSwitching

M3Result EvaluateExpression (IM3Module i_module, فراغ* o_expressed, m3type_t i_type, bytes_t* io_bytes, cbytes_t i_end)
{
    M3Result result = m3Err_none;

    // OPTZ: use a simplified interpreter for expressions

    // create a temporary runtime context
#إذا defined(d_m3PreferStaticAlloc)
    ساكن M3Runtime runtime;
#وإلا
    M3Runtime runtime;
#endif
    M3_INIT(runtime);

    runtime.environment   = i_module->runtime->environment;
    runtime.اڠک_تيندنن_Slots = i_module->runtime->اڠک_تيندنن_Slots;
    runtime.تيندنن         = i_module->runtime->تيندنن;

    m3stack_t تيندنن = (m3stack_t)runtime.تيندنن;

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

    إذا (o->page) {
        IM3FuncType ftype = عدم;

#إذا d_m3HasTypedRefs
        // A reference type has to be checked as the type it is: the shared
        // return types are one per storage class, so borrowing one of those
        // would let any reference at all satisfy a particular one.
        إذا (IsSpelledRefType(i_type)) {
            result = AllocFuncType(&ftype, 1);

            إذا (not result) {
                ftype->اڠک_Rets  = 1;
                ftype->types[0] = i_type;

                result = Environment_AddFuncType(runtime.environment, &ftype);
            }
        } وإلا
#endif
        {
            ftype = runtime.environment->retFuncTypes[BaseTypeOf(i_type)];
        }

        pc_t m3code = GetPagePC(o->page);

        إذا (not result) {
            result = CompileExpression(o, ftype);
        }

        إذا (not result && o->max_تيندنن_Slots >= runtime.اڠک_تيندنن_Slots) {
            result = m3_Err_trap_تيندنن_Overflow;
        }

        إذا (not result) {
#إذا (d_m3EnableOpProfiling || d_m3EnableOpTracing)
            m3ret_t r = RunCode(m3code, تيندنن, عدم, d_m3OpDefaultArgs, d_m3BaseCstr);
#وإلا
            m3ret_t r = RunCode(m3code, تيندنن, عدم, d_m3OpDefaultArgs);
#endif

            إذا (r == 0) {                                                                   m3log (runtime, "expression result: %s", SPrint_نيلاي (تيندنن, i_type));
                إذا (SizeOfType(BaseTypeOf(i_type)) == حجم(u32)) {
                    *(u32*)o_expressed = *((u32*)تيندنن);
                } وإلا {
                    *(u64*)o_expressed = *((u64*)تيندنن);
                }
            }
        }

        ReleaseCodePage(&runtime, o->page);
    } وإلا {
        result = m3Err_mallocFailedCodePage;
    }

    runtime.origin_تيندنن = عدم;        // prevent free(stack) in ReleaseRuntime
    Runtime_Release(&runtime);
    i_module->runtime = savedRuntime;

    *io_bytes = o->wasm;

    أرجع result;
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
ساكن
bool LimitsSatisfy (u64 i_exportedSize, bool i_exportedHasMax, u64 i_exportedMax,
                    u64 i_importMin, bool i_importHasMax, u64 i_importMax)
{
    إذا (i_exportedSize < i_importMin) {
        أرجع false;
    }

    إذا (i_importHasMax and (not i_exportedHasMax or i_exportedMax > i_importMax)) {
        أرجع false;
    }

    أرجع true;
}


// The export of this kind under this name, or NULL. The index it carries is
// resolved by the caller, against the module's index space as it stands then -
// after linking, a re-exported import's slot holds the entity it was linked to.
ساكن
ثابت M3Export* Module_FindExport (IM3Module i_module, u8 i_kind, cstr_t i_name)
{
    لكل (u32 i = 0; i < i_module->اڠک_Exports; ++i) {
        ثابت M3Export* e = &i_module->exports[i];

        إذا (e->kind == i_kind and strcmp(e->name, i_name) == 0) {
            أرجع e;
        }
    }

    أرجع عدم;
}


ساكن
IM3Function Module_FindExportedFunction (IM3Module i_module, cstr_t i_name)
{
    ثابت M3Export* e = Module_FindExport(i_module, d_externalKind_function, i_name);

    // A module that re-exports an import names the placeholder here. Resolve to
    // the function that actually runs, or a host-side call would run it against
    // the importing module's memory.
    أرجع e ? Function_Implementation(&i_module->functions[e->index]) : عدم;
}


ساكن
IM3Memory Module_FindExportedMemory (IM3Module i_module, cstr_t i_name)
{
    ثابت M3Export* e = Module_FindExport(i_module, d_externalKind_memory, i_name);

    أرجع e ? i_module->memories[e->index] : عدم;
}


ساكن
IM3Table Module_FindExportedTable (IM3Module i_module, cstr_t i_name)
{
    ثابت M3Export* e = Module_FindExport(i_module, d_externalKind_table, i_name);

    أرجع e ? i_module->tables[e->index] : عدم;
}


ساكن
IM3Global Module_FindExportedGlobal (IM3Module i_module, cstr_t i_name)
{
    ثابت M3Export* e = Module_FindExport(i_module, d_externalKind_global, i_name);

    أرجع e ? &i_module->globals[e->index] : عدم;
}


#إذا d_m3HasExceptionHandling || d_m3_Has_تيندنن_Switching
ساكن
IM3Tag Module_FindExportedTag (IM3Module i_module, cstr_t i_name)
{
    ثابت M3Export* e = Module_FindExport(i_module, d_externalKind_tag, i_name);

    إذا (not e) {
        أرجع عدم;
    }

    IM3Tag tag = &i_module->tags[e->index];

    أرجع tag->resolved ? tag->resolved : tag;
}
#endif


// Whether the module exports anything at all under this name. Export names are
// unique within a module, so a name one of the lookups above missed but this
// one finds is exported as something else - a kind the import cannot be
// satisfied by, rather than a name the module never exported.
ساكن
bool Module_HasExport (IM3Module i_module, cstr_t i_name)
{
    لكل (u32 i = 0; i < i_module->اڠک_Exports; ++i) {
        إذا (strcmp(i_module->exports[i].name, i_name) == 0) {
            أرجع true;
        }
    }

    أرجع false;
}


// Points each of the module's imports at whatever already-loaded module exports
// it. Runs before anything is allocated or initialized: a memory import has to
// be resolved before InitMemory would give it pages of its own, and a global
// import before InitGlobals runs an initializer that reads it.
ساكن
M3Result LinkImports (IM3Runtime io_runtime, IM3Module io_module)
{
    M3Result result = m3Err_none;

    لكل (u32 i = 0; i < io_module->اڠک_Functions; ++i) {
        IM3Function f = &io_module->functions[i];

        إذا (f->wasm or not (f->import.moduleUtf8 and f->import.fieldUtf8)) {
            استمر;
        }

        IM3Module from = m3_FindModule(io_runtime, f->import.moduleUtf8);
        إذا (not from) {
            استمر;
        }

        IM3Function exported = Module_FindExportedFunction(from, f->import.fieldUtf8);
        إذا (not exported) {
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

    لكل (u32 i = 0; i < io_module->اڠک_Memories; ++i) {
        IM3Memory memory = io_module->memories[i];

        إذا (not memory->imported or memory->owner != io_module) {
            استمر;
        }

        IM3Module from = m3_FindModule(io_runtime, memory->import.moduleUtf8);
        إذا (not from) {
            استمر;
        }

        IM3Memory exported = Module_FindExportedMemory(from, memory->import.fieldUtf8);
        إذا (not exported) {
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
                 not LimitsSatisfy(exported->اڠک_Pages, exported->hasMax, exported->maxPages,
                                   memory->initPages, memory->hasMax, memory->maxPages));

        // hand the slot over to the exporter's memory, and drop the placeholder
        FreeMemoryBlock(memory);
        FreeImportInfo(&memory->import);
        m3_Free(memory);

        io_module->memories[i] = exported;
    }

    لكل (u32 i = 0; i < io_module->اڠک_Tables; ++i) {
        IM3Table table = io_module->tables[i];

        إذا (not table->imported or table->owner != io_module) {
            استمر;
        }

        IM3Module from = m3_FindModule(io_runtime, table->import.moduleUtf8);
        إذا (not from) {
            استمر;
        }

        IM3Table exported = Module_FindExportedTable(from, table->import.fieldUtf8);
        إذا (not exported) {
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

    لكل (u32 i = 0; i < io_module->اڠک_Globals; ++i) {
        IM3Global g = &io_module->globals[i];

        إذا (not g->imported or not (g->import.moduleUtf8 and g->import.fieldUtf8)) {
            استمر;
        }

        IM3Module from = m3_FindModule(io_runtime, g->import.moduleUtf8);
        إذا (not from) {
            استمر;
        }

        IM3Global exported = Module_FindExportedGlobal(from, g->import.fieldUtf8);
        إذا (not exported) {
            _throwif(m3Err_incompatibleImportType, Module_HasExport(from, g->import.fieldUtf8));
            _throw(m3Err_unknownImport);
        }

        _throwif(m3Err_incompatibleImportType, exported->type != g->type);
        _throwif(m3Err_incompatibleImportType, exported->isMutable != g->isMutable);

        g->resolved = exported->resolved ? exported->resolved : exported;
    }

#إذا d_m3HasExceptionHandling || d_m3_Has_تيندنن_Switching
    لكل (u32 i = 0; i < io_module->اڠک_Tags; ++i) {
        IM3Tag tag = &io_module->tags[i];

        إذا (not tag->imported or not (tag->import.moduleUtf8 and tag->import.fieldUtf8)) {
            استمر;
        }

        IM3Module from = m3_FindModule(io_runtime, tag->import.moduleUtf8);
        إذا (not from) {
            استمر;
        }

        IM3Tag exported = Module_FindExportedTag(from, tag->import.fieldUtf8);
        إذا (not exported) {
            _throwif(m3Err_incompatibleImportType, Module_HasExport(from, tag->import.fieldUtf8));
            _throw(m3Err_unknownImport);
        }

        _throwif(m3Err_incompatibleImportType, exported->type != tag->type);

        tag->resolved = exported->resolved ? exported->resolved : exported;
    }
#endif

    _catch: أرجع result;
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
    i_module->memory0 = i_module->اڠک_Memories ? i_module->memories[0]
                                              : &i_module->emptyMemory;

    إذا (i_module->اڠک_Memories == 0) {
        // nothing addressable, but _mem still has to point somewhere
        i_module->emptyMemory.owner    = i_module;
        i_module->emptyMemory.pageSize = d_m3DefaultMemPageSize;

_       (ResizeMemory(io_runtime, &i_module->emptyMemory, 0));
    }

    لكل (u32 i = 0; i < i_module->اڠک_Memories; ++i) {
        IM3Memory memory = i_module->memories[i];

        // a slot that already points at another module's memory is that
        // module's to allocate
        إذا (memory->owner != i_module or memory->mallocated) {
            استمر;
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
        إذا (not memory->hasMax) {
            memory->maxPages = memory->isMemory64
                                 ? (pageSize > 1 ? (UINT64_MAX / pageSize) + 1 : UINT64_MAX)
                                 : (0x100000000ull / pageSize);
        }

_       (ResizeMemory(io_runtime, memory, memory->initPages));
    }

    _catch: أرجع result;
}


M3Result ResizeMemory (IM3Runtime io_runtime, IM3Memory memory, u64 i_اڠک_Pages)
{
    M3Result result = m3Err_none;

    u64 اڠک_Pages_To_Alloc = i_اڠک_Pages;

    إذا (اڠک_Pages_To_Alloc <= memory->maxPages) {
        // A 64-bit memory may ask for up to 2^48 pages, which overflows a u64
        // of bytes. Nothing that large can be backed, so refuse it up front
        // rather than multiplying into a wrapped size.
        _throwif("linear memory limitation exceeded",
                 اڠک_Pages_To_Alloc > d_m3AddressLimit / memory->pageSize);

        u64 اڠک_Page_Bytes = اڠک_Pages_To_Alloc * memory->pageSize;

#إذا d_m3MaxLinearMemoryPages > 0
        // the limit is a memory size, counted in default-sized pages; comparing
        // it against a raw page count would make it 65536 times stricter for a
        // module whose pages are one byte
        _throwif("linear memory limitation exceeded",
                 اڠک_Page_Bytes > (u64)d_m3MaxLinearMemoryPages * d_m3DefaultMemPageSize);
#endif

        // Limit the amount of memory that gets actually allocated
        إذا (io_runtime->memoryLimit) {
            اڠک_Page_Bytes = M3_MIN(اڠک_Page_Bytes, (u64)io_runtime->memoryLimit);
        }

        u64 oldLength = memory->mallocated ? memory->mallocated->length : 0;
        u64 otherUsed = io_runtime->memoryBytesUsed - oldLength;
        _throwif(m3Err_memoryLimitExceeded, io_runtime->memoryBytesLimit and
                                              (اڠک_Page_Bytes > io_runtime->memoryBytesLimit or
                                               otherUsed > io_runtime->memoryBytesLimit - اڠک_Page_Bytes));

        _throwif("linear memory limitation exceeded", اڠک_Page_Bytes > (u64)SIZE_MAX - حجم(M3MemoryHeader));

#إذا d_m3GuardedMemory

        // A slot holds every address a Wasm access can name, and nothing beyond
        // that: a memory larger than the data part of one would put addresses past
        // the guard, where a fault is nobody's to recognize
        _throwif("linear memory limitation exceeded", اڠک_Page_Bytes > d_m3GuardedDataBytes);

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

        إذا (memory->guardSlot == عدم) {
            memory->guardSlot = Guard_TakeSlot();
            _throwifnull(memory->guardSlot);
        }

        // Growing only ever commits more of the same slot, so the memory never
        // moves - which the interpreter does not rely on, but nothing minds
        _throwif(m3Err_mallocFailed, not Guard_CommitSlot(memory->guardSlot, (size_t)اڠک_Page_Bytes));

        memory->mallocated = Guard_SlotHeader(memory->guardSlot);

#وإلا

        size_t اڠک_Bytes = (size_t)اڠک_Page_Bytes + حجم(M3MemoryHeader);

        // What is really there, which is not numPages * pageSize: the limit above
        // caps the bytes without capping the page count, so a memory can be short
        // of a page - by design, since a whole 64 KiB page is often more than an
        // MCU has. Measuring the old block by the page count would overstate it,
        // and then realloc never sees the size it already has and copies the
        // whole limit again on every grow past it.
        size_t اڠک_Previous_Bytes = memory->mallocated ? memory->mallocated->length + حجم(M3MemoryHeader) : 0;

        فراغ* newMem = m3_Realloc("Wasm Linear Memory", memory->mallocated, اڠک_Bytes, اڠک_Previous_Bytes);
        _throwifnull(newMem);

        memory->mallocated = (M3MemoryHeader*)newMem;

#endif

#إذا d_m3LogRuntime
        M3MemoryHeader* oldMallocated = memory->mallocated;
#endif

        io_runtime->memoryBytesUsed = otherUsed + اڠک_Page_Bytes;
        memory->اڠک_Pages            = اڠک_Pages_To_Alloc;

        memory->mallocated->length  = (size_t)اڠک_Page_Bytes;
        memory->mallocated->runtime = io_runtime;
        memory->mallocated->memory  = memory;

        memory->mallocated->max_تيندنن = (m3slot_t*)io_runtime->origin_تيندنن + io_runtime->اڠک_تيندنن_Slots;

        m3log(runtime, "resized old: %p; mem: %p; length: %zu; pages: %llu", oldMallocated, memory->mallocated, memory->mallocated->length, (unsigned long long)memory->اڠک_Pages);
    } وإلا {
        result = m3Err_wasmMemoryOverflow;
    }

    _catch: أرجع result;
}


فراغ FreeMemoryBlock (IM3Memory io_memory)
{
    إذا (io_memory->mallocated and io_memory->mallocated->runtime) {
        io_memory->mallocated->runtime->memoryBytesUsed -= io_memory->mallocated->length;
    }
#إذا d_m3GuardedMemory
    Guard_GiveSlot(io_memory->guardSlot);
    io_memory->guardSlot = عدم;

    io_memory->mallocated = عدم;
#وإلا
    m3_Free(io_memory->mallocated);
#endif
}


M3Result InitGlobals (IM3Module io_module)
{
    M3Result result = m3Err_none;

    إذا (io_module->اڠک_Globals) {
        // placing the globals in their structs isn't good for cache locality, but i don't really know what the global
        // access patterns typically look like yet.

        //          io_module->globalMemory = m3Alloc (m3reg_t, io_module->numGlobals);

        //          if (io_module->globalMemory)
        {
            لكل (u32 i = 0; i < io_module->اڠک_Globals; ++i) {
                M3Global* g = &io_module->globals[i];                           m3log (runtime, "initializing global: %d", i);

                إذا (g->init_اوڠکاڤن) {
                    bytes_t start = g->init_اوڠکاڤن;

                    result = EvaluateExpression(io_module, &g->i64_نيلاي, g->type, &start, g->init_اوڠکاڤن + g->init_اوڠکاڤن_Size);

                    إذا (not result) {
                        // io_module->globalMemory [i] = initValue;
                    } وإلا {
                        اكسر;
                    }
                } وإلا {                                                        m3log (runtime, "importing global");
                }
            }
        }
        //          else result = ErrorModule (m3Err_mallocFailed, io_module, "could allocate globals for module: '%s", io_module->name);
    }

    أرجع result;
}


M3Result InitDataSegments (IM3Module io_module)
{
    M3Result result = m3Err_none;

    لكل (u32 i = 0; i < io_module->اڠک_Data_Segments; ++i) {
        M3DataSegment* segment = &io_module->dataSegments[i];

        // A passive segment stays available for memory.init until data.drop.
        // An active one is copied here and then counts as dropped.
        إذا (segment->isPassive) {
            استمر;
        }

        _throwif("data segment memory index out of range",
                 segment->memoryRegion >= io_module->اڠک_Memories);

        IM3Memory io_memory = io_module->memories[segment->memoryRegion];

        _throwif("unallocated linear memory", !(io_memory->mallocated));

        // The offset expression has the memory's address type, and is
        // unsigned: an i32 offset of -1 is 4294967295, way out of bounds
        // rather than negative.
        u64     segmentOffset = 0;
        bytes_t start         = segment->init_اوڠکاڤن;

        إذا (io_memory->isMemory64) {
_           (EvaluateExpression(io_module, &segmentOffset, c_m3Type_i64, &start, segment->init_اوڠکاڤن + segment->init_اوڠکاڤن_Size));
        } وإلا {
            u32 offset32;
_           (EvaluateExpression(io_module, &offset32, c_m3Type_i32, &start, segment->init_اوڠکاڤن + segment->init_اوڠکاڤن_Size));
            segmentOffset = offset32;
        }

        m3log(runtime, "loading data segment: %d; size: %d; offset: %llu", i, segment->size, (unsigned long long)segmentOffset);

        إذا (segmentOffset <= io_memory->mallocated->length &&
            (u64)segment->size <= io_memory->mallocated->length - segmentOffset) {
            u8* dest = m3MemData(io_memory->mallocated) + segmentOffset;
            memcpy(dest, segment->data, segment->size);
        } وإلا {
            _throw("data segment out of bounds");
        }

        segment->dropped = true;
    }

    _catch: أرجع result;
}


// Turns a segment's elements into references. Element expressions are constant
// expressions restricted to ref.null/ref.func, so they're read directly rather
// than run through the compiler.
ساكن
M3Result ResolveElements (IM3Module io_module, M3ElementSegment* i_segment, فراغ** o_elements)
{
    M3Result result = m3Err_none;

    bytes_t  pos = i_segment->elements;
    cbytes_t end = io_module->elementSectionEnd;

    لكل (u32 e = 0; e < i_segment->اڠک_Elements; ++e) {
        u32   funcIndex;
        فراغ* ref = عدم;

        إذا (i_segment->is_اوڠکاڤن) {
            m3opcode_t opcode;
_           (Read_opcode(&opcode, &pos, end));

            إذا (opcode == c_waOp_refFunc) {
_               (ReadLEB_u32(&funcIndex, &pos, end));
                _throwif("function index out of range", funcIndex >= io_module->اڠک_Functions);
                ref = Function_Implementation(&io_module->functions[funcIndex]);
            } وإلا إذا (opcode == c_waOp_refNull) {
                i8 waType;
                u8 nullType;
_               (ReadLEB_i7(&waType, &pos, end));
_               (NormalizeType(&nullType, waType));
                _throwif(m3Err_typeMismatch, nullType != i_segment->type);
            } وإلا إذا (opcode == c_waOp_getGlobal) {
                // wasm 2.0 lets an element expression read an imported
                // immutable global, which is how one module seeds another's
                // table with a reference it exported.
                u32 globalIndex;
_               (ReadLEB_u32(&globalIndex, &pos, end));
                _throwif(m3Err_globaIndexOutOfBounds, globalIndex >= io_module->اڠک_Globals);

                IM3Global global = &io_module->globals[globalIndex];

                _throwif(m3Err_globaIndexOutOfBounds, not global->imported);
                _throwif(m3Err_wasmMalformed, global->isMutable);
                _throwif(m3Err_typeMismatch, BaseTypeOf(global->type) != BaseTypeOf(i_segment->type));

                // read the cell the import was linked to, not the placeholder
                إذا (global->resolved) {
                    global = global->resolved;
                }

                ref = global->ref_نيلاي;
            } وإلا {
                _throw("constant expression required");
            }

_           (Read_opcode(&opcode, &pos, end));
            _throwif(m3Err_wasmMalformed, opcode != c_waOp_end);
        } وإلا {
_           (ReadLEB_u32(&funcIndex, &pos, end));
            _throwif("function index out of range", funcIndex >= io_module->اڠک_Functions);
            ref = Function_Implementation(&io_module->functions[funcIndex]);
        }

        o_elements[e] = ref;
    }

    _catch: أرجع result;
}


M3Result InitTableAndElements (IM3Module io_module)
{
    M3Result result = m3Err_none;

    cbytes_t end = io_module->elementSectionEnd;
    M3Table* table;

    لكل (u32 i = 0; i < io_module->اڠک_Tables; ++i) {
        table = io_module->tables[i];

        // a slot pointing at another module's table is that module's to fill
        إذا (table->owner != io_module) {
            استمر;
        }

        إذا (table->size) {
            IM3Runtime runtime = io_module->runtime;
            _throwif(m3Err_tableLimitExceeded, runtime->tableElementsLimit and
                                                 table->size > runtime->tableElementsLimit - runtime->tableElementsUsed);
            table->elements = m3_AllocArray(فراغ*, table->size);
            _throwifnull(table->elements);
            runtime->tableElementsUsed += table->size;

            إذا (table->init_اوڠکاڤن) {
                فراغ*   نيلاي = عدم;
                bytes_t start = table->init_اوڠکاڤن;
_               (EvaluateExpression(io_module, &نيلاي, BaseTypeOf(table->type),
                                     &start, table->init_اوڠکاڤن + table->init_اوڠکاڤن_Size));

                لكل (u32 e = 0; e < table->size; ++e) {
                    table->elements[e] = نيلاي;
                }
            }
        }
    }

    لكل (u32 i = 0; i < io_module->اڠک_Element_Segments; ++i) {
        M3ElementSegment* segment = &io_module->elementSegments[i];

        // Declarative segments only make their functions referenceable, and
        // passive ones wait for table.init, so neither is written out here.
        إذا (segment->mode == c_m3Elem_declarative) {
            segment->dropped = true;
            استمر;
        }

        إذا (segment->mode == c_m3Elem_passive) {
            إذا (segment->اڠک_Elements) {
                segment->resolved = m3_AllocArray(فراغ*, segment->اڠک_Elements);
                _throwifnull(segment->resolved);
_               (ResolveElements(io_module, segment, segment->resolved));
            }
            استمر;
        }

        table = io_module->tables[segment->tableIndex];

        // The offset expression has the table's index type, and is unsigned:
        // an i32 offset of -1 is 4294967295, out of bounds rather than negative.
        u64     offset = 0;
        bytes_t اوڠکاڤن   = segment->init_اوڠکاڤن;

        إذا (table->isTable64) {
_           (EvaluateExpression(io_module, &offset, c_m3Type_i64, &اوڠکاڤن, end));
        } وإلا {
            u32 offset32;
_           (EvaluateExpression(io_module, &offset32, c_m3Type_i32, &اوڠکاڤن, end));
            offset = offset32;
        }

        _throwif("out of bounds table access",
                 offset > table->size or segment->اڠک_Elements > table->size - offset);

_       (ResolveElements(io_module, segment, table->elements + offset));

        segment->dropped = true;
    }

    _catch: أرجع result;
}

M3Result m3_CompileModule (IM3Module io_module)
{
    M3Result result = m3Err_none;

    لكل (u32 i = 0; i < io_module->اڠک_Functions; ++i) {
        IM3Function f = &io_module->functions[i];
        إذا (f->wasm and not f->compiled) {
_           (CompileFunction(f));
        }
    }

    _catch: أرجع result;
}

#إذا d_m3HasExceptionHandling

M3Exception* NewException (IM3Runtime io_runtime, IM3Tag i_tag, u32 i_اڠک_Args)
{
    M3Exception* exception = (M3Exception*)m3_Malloc("M3Exception", حجم(M3Exception) + i_اڠک_Args * حجم(u64));

    إذا (exception) {
        exception->tag     = i_tag;
        exception->اڠک_Args = i_اڠک_Args;
        exception->reified = false;
        exception->prev    = عدم;
        exception->next    = io_runtime->exceptions;

        إذا (exception->next) {
            exception->next->prev = exception;
        }

        io_runtime->exceptions = exception;
    }

    أرجع exception;
}


// Releases one exception ahead of the rest. The caller has to know nothing can
// still name it: no exnref was ever taken of it, and its payload has already
// been copied out.
فراغ FreeException (IM3Runtime io_runtime, M3Exception* i_exception)
{
    إذا (i_exception->prev) {
        i_exception->prev->next = i_exception->next;
    } وإلا {
        io_runtime->exceptions = i_exception->next;
    }

    إذا (i_exception->next) {
        i_exception->next->prev = i_exception->prev;
    }

    إذا (io_runtime->pendingException == i_exception) {
        io_runtime->pendingException = عدم;
    }

    m3_Free_Impl(i_exception);
}


// Releases every exception the runtime still holds. Only safe once the Wasm
// stack is empty, which is why the outermost RunCodeChecked() is the one that
// calls it.
فراغ FreeExceptions (IM3Runtime io_runtime)
{
    M3Exception* exception = io_runtime->exceptions;

    io_runtime->exceptions       = عدم;
    io_runtime->pendingException = عدم;

    طالما (exception) {
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
ساكن
فراغ RecordUncaughtException (IM3Runtime io_runtime, M3Result i_result)
{
    إذا (io_runtime->uncaughtException) {
        m3_Free(io_runtime->uncaughtException);
    }

    M3Exception* exception = io_runtime->pendingException;

    إذا (M3_UNLIKELY(i_result == m3Err_trapUncaughtException) and exception) {
        M3Exception* copy = (M3Exception*)m3_CopyMem(exception, حجم(M3Exception) + exception->اڠک_Args * حجم(u64));

        إذا (copy) {
            copy->next = عدم;
            copy->prev = عدم;
        }

        io_runtime->uncaughtException = copy;
    }
}

#endif // d_m3HasExceptionHandling


#إذا d_m3HasExceptionHandling || d_m3_Has_تيندنن_Switching

IM3Tag m3_FindTag (IM3Module i_module, ثابت حرف* ثابت i_tagName)
{
    إذا (not i_module or not i_tagName) {
        أرجع عدم;
    }

    أرجع Module_FindExportedTag(i_module, i_tagName);
}


M3Result m3_NewTag (IM3Runtime io_runtime, IM3Tag* o_tag, ثابت حرف* ثابت i_signature)
{
    IM3FuncType type    = عدم;
    M3HostTag*  hostTag = عدم;

_try {
    _throwif("null tag", not o_tag);
    *o_tag = عدم;
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
    أرجع result;
}


M3Result m3_LinkTag (IM3Module         io_module,
                     ثابت حرف* ثابت i_moduleName,
                     ثابت حرف* ثابت i_tagName,
                     IM3Tag            i_tag)
{
    إذا (not io_module or not i_moduleName or not i_tagName or not i_tag) {
        أرجع m3Err_tagLookupFailed;
    }

    ثابت bool wildcardModule = (strcmp(i_moduleName, "*") == 0);
    IM3Tag     target         = i_tag->resolved ? i_tag->resolved : i_tag;

    M3Result result = m3Err_tagLookupFailed;

    لكل (u32 i = 0; i < io_module->اڠک_Tags; ++i) {
        IM3Tag tag = &io_module->tags[i];

        إذا (tag->imported and tag->import.moduleUtf8 and tag->import.fieldUtf8 and
            strcmp(tag->import.fieldUtf8, i_tagName) == 0 and
            (wildcardModule or strcmp(tag->import.moduleUtf8, i_moduleName) == 0)) {
            إذا (tag->type != target->type) {
                أرجع m3Err_incompatibleImportType;
            }

            tag->resolved = target;
            result        = m3Err_none;
        }
    }

    أرجع result;
}


uint32_t m3_Get_Tag_Arg_کيراءن (IM3Tag i_tag)
{
    أرجع i_tag ? i_tag->type->اڠک_Args : 0;
}


M3_نيلاي_Type m3_GetTagArgType (IM3Tag i_tag, uint32_t i_index)
{
    إذا (i_tag and i_index < i_tag->type->اڠک_Args) {
        أرجع (M3_نيلاي_Type)BaseTypeOf(d_FuncArgType(i_tag->type, i_index));
    }

    أرجع c_m3Type_none;
}

#وإلا

IM3Tag m3_FindTag (IM3Module i_module, ثابت حرف* ثابت i_tagName)
{
    أرجع عدم;
}

M3Result m3_NewTag (IM3Runtime io_runtime, IM3Tag* o_tag, ثابت حرف* ثابت i_signature)
{
    إذا (o_tag) {
        *o_tag = عدم;
    }
    أرجع m3Err_tagLookupFailed;
}

M3Result m3_LinkTag (IM3Module io_module, ثابت حرف* ثابت i_moduleName, ثابت حرف* ثابت i_tagName, IM3Tag i_tag)
{
    أرجع m3Err_tagLookupFailed;
}

uint32_t m3_Get_Tag_Arg_کيراءن (IM3Tag i_tag)
{
    أرجع 0;
}

M3_نيلاي_Type m3_GetTagArgType (IM3Tag i_tag, uint32_t i_index)
{
    أرجع c_m3Type_none;
}

#endif // d_m3HasExceptionHandling || d_m3HasStackSwitching


#إذا d_m3HasExceptionHandling

M3Result m3_ThrowException (IM3Runtime io_runtime, IM3Tag i_tag, uint32_t i_argc, ثابت فراغ* i_argptrs[])
{
    إذا (not io_runtime or not i_tag) {
        أرجع m3Err_tagLookupFailed;
    }

    IM3Tag      tag  = i_tag->resolved ? i_tag->resolved : i_tag;
    IM3FuncType type = tag->type;

    // a tag with results is a stack switching control tag, which is suspended
    // with rather than thrown
    إذا (type->اڠک_Rets) {
        أرجع m3Err_typeMismatch;
    }
    إذا (i_argc != type->اڠک_Args) {
        أرجع m3_Err_argument_کيراءن_Mismatch;
    }
    // with nothing on the stack there is nothing to unwind, and the exception
    // would sit on the runtime's list until some later call ended
    إذا (io_runtime->callNesting == 0) {
        أرجع "m3_ThrowException called outside a host function";
    }

    M3Exception* exception = NewException(io_runtime, tag, i_argc);
    إذا (not exception) {
        أرجع m3Err_mallocFailed;
    }

    // the layout op_Throw gives a payload: 32-bit values zero-extended, floats
    // as their bit pattern
    لكل (u32 i = 0; i < i_argc; ++i) {
        ثابت فراغ* نيلاي = i_argptrs[i];

        بدل (BaseTypeOf(d_FuncArgType(type, i))) {
        حالة c_m3Type_i32:
        حالة c_m3Type_f32: {
            u32 bits;
            memcpy(&bits, نيلاي, حجم(bits));
            exception->args[i] = bits;
            اكسر;
        }
        حالة c_m3Type_i64:
        حالة c_m3Type_f64:
            memcpy(&exception->args[i], نيلاي, حجم(u64));
            اكسر;
        حالة c_m3Type_funcref:
        حالة c_m3Type_externref:
        حالة c_m3Type_exnref:
        حالة c_m3Type_contref:
            exception->args[i] = (u64)(*(ثابت uintptr_t*)نيلاي);
            اكسر;
        افتراضي:
            FreeException(io_runtime, exception);
            أرجع "unknown argument type";
        }
    }

    io_runtime->pendingException = exception;

    أرجع m3Err_pendingException;
}


IM3Tag m3_GetExceptionTag (IM3Runtime i_runtime)
{
    إذا (i_runtime and i_runtime->uncaughtException) {
        أرجع i_runtime->uncaughtException->tag;
    }

    أرجع عدم;
}


M3Result m3_GetExceptionArgs (IM3Runtime i_runtime, uint32_t i_argc, ثابت فراغ* o_argptrs[])
{
    إذا (not i_runtime or not i_runtime->uncaughtException) {
        أرجع "no uncaught exception";
    }

    ثابت M3Exception* exception = i_runtime->uncaughtException;
    IM3FuncType        type      = exception->tag->type;

    إذا (i_argc != exception->اڠک_Args) {
        أرجع m3_Err_argument_کيراءن_Mismatch;
    }

    لكل (u32 i = 0; i < i_argc; ++i) {
        فراغ* نيلاي = (فراغ*)o_argptrs[i];
        u64   bits  = exception->args[i];

        بدل (BaseTypeOf(d_FuncArgType(type, i))) {
        حالة c_m3Type_i32:
        حالة c_m3Type_f32: {
            u32 رنده = (u32)bits;
            memcpy(نيلاي, &رنده, حجم(رنده));
            اكسر;
        }
        حالة c_m3Type_i64:
        حالة c_m3Type_f64:
            memcpy(نيلاي, &bits, حجم(bits));
            اكسر;
        حالة c_m3Type_funcref:
        حالة c_m3Type_externref:
        حالة c_m3Type_exnref:
        حالة c_m3Type_contref:
            *(uintptr_t*)نيلاي = (uintptr_t)bits;
            اكسر;
        افتراضي:
            أرجع "unknown argument type";
        }
    }

    أرجع m3Err_none;
}

#وإلا

M3Result m3_ThrowException (IM3Runtime io_runtime, IM3Tag i_tag, uint32_t i_argc, ثابت فراغ* i_argptrs[])
{
    أرجع m3Err_tagLookupFailed;
}

IM3Tag m3_GetExceptionTag (IM3Runtime i_runtime)
{
    أرجع عدم;
}

M3Result m3_GetExceptionArgs (IM3Runtime i_runtime, uint32_t i_argc, ثابت فراغ* o_argptrs[])
{
    أرجع "no uncaught exception";
}

#endif // d_m3HasExceptionHandling


#إذا d_m3GuardedMemory

// What RunCodeProtected hands to the platform layer to run, and what comes back
تعريفنوع بنية M3ProtectedRun {
    pc_t            pc;
    IM3Runtime      runtime;
    M3MemoryHeader* mem;
    M3Result        result;
} M3ProtectedRun;

ساكن
فراغ RunCodeBody (فراغ* io_context)
{
    M3ProtectedRun* run = (M3ProtectedRun*)io_context;

#  إذا (d_m3EnableOpProfiling || d_m3EnableOpTracing)
    run->result = (M3Result)RunCode(run->pc, (m3stack_t)run->runtime->تيندنن, run->mem,
                                    d_m3OpDefaultArgs, d_m3BaseCstr);
#  وإلا
    run->result = (M3Result)RunCode(run->pc, (m3stack_t)run->runtime->تيندنن, run->mem,
                                    d_m3OpDefaultArgs);
#  endif
}

// The bounds check the load and store operations no longer do, done once here by the
// system instead: an address past the end of a memory reaches the uncommitted part
// of that memory's reservation, and the fault comes back as the trap the spec asks
// for. Nothing is left half-done by it - the Wasm stack and the memories are heap,
// and the caller unwinds the rest.
ساكن
M3Result RunCodeProtected (IM3Runtime i_runtime, pc_t i_pc, M3MemoryHeader* i_mem)
{
    // Memory operations in this build carry no bounds check, so code only runs where a
    // fault past the end of a memory can still be turned into a trap
    إذا (M3_UNLIKELY(not m3_HostGuardsActive())) {
        أرجع ErrorRuntime(m3Err_memoryGuardsUnavailable, i_runtime,
                            "another part of the process holds the fault signal");
    }

    M3ProtectedRun run;
    run.pc      = i_pc;
    run.runtime = i_runtime;
    run.mem     = i_mem;
    run.result  = m3Err_none;

    فراغ*  arena;
    size_t arenaBytes;
    Guard_ArenaRange(&arena, &arenaBytes);

    إذا (not m3_HostProtectedCall(RunCodeBody, &run, arena, arenaBytes)) {
        أرجع m3Err_trapOutOfBoundsMemoryAccess;
    }

    أرجع run.result;
}

#endif // d_m3GuardedMemory


#إذا d_m3_Has_تيندنن_Switching

// The function a suspended continuation stopped in: the callee of its innermost
// call frame, or the function it was entered with if it made no call. Frames are
// recorded innermost first, so that is the lowest-numbered call frame.
ساكن
IM3Function Continuation_InnermostFunction (IM3Continuation i_cont)
{
    لكل (u32 i = 0; i < i_cont->اڠک_Frames; ++i) {
        إذا (i_cont->frames[i].kind == frame_call && i_cont->frames[i].call.function) {
            أرجع i_cont->frames[i].call.function;
        }
    }

    أرجع i_cont->entryFunction;
}


// Where the host's next call puts its arguments and finds its results. A
// suspended root keeps its frames on the bottom of the runtime's stack, so a call
// made before it is resumed starts above the last of them rather than on top of
// them. The top is the innermost function's frame base plus everything that
// function can use. That base is the root's own sp, unless it stopped inside a
// resume: then the suspension point belongs to the resumed continuation, and the
// root's innermost frame is that resume.
فراغ Runtime_Place_Call_تيندنن (IM3Runtime io_runtime)
{
    IM3Continuation اکر = io_runtime->اکر_Continuation;

    io_runtime->تيندنن = io_runtime->origin_تيندنن;

    إذا (not اکر or اکر->state != cont_suspended) {
        أرجع;
    }

    m3stack_t   sp       = اکر->sp;
    IM3Function function = Continuation_InnermostFunction(اکر);

    إذا (اکر->اڠک_Frames and اکر->frames[0].kind == frame_resume) {
        sp = اکر->frames[0].sp;
    }

    m3slot_t* base = (m3slot_t*)io_runtime->origin_تيندنن;
    m3slot_t* اتس  = (m3slot_t*)sp + (function ? function->max_تيندنن_Slots : 0);

    إذا (اتس > base + io_runtime->اڠک_تيندنن_Slots) {
        اتس = base + io_runtime->اڠک_تيندنن_Slots;
    }

    io_runtime->تيندنن = اتس;
}

#endif // d_m3HasStackSwitching


// Run compiled code on the runtime's stack, bounding native recursion for the
// duration of the call. The outermost invocation establishes the stack limit;
// nested ones (an imported function calling back into Wasm) inherit it.
ساكن مضمن
M3Result RunCodeChecked (IM3Runtime i_runtime, IM3Function i_function)
{
    pc_t i_pc = i_function->compiled;

    // execution runs against the memory of the module the entry point belongs
    // to, not against some runtime-wide one
    M3MemoryHeader* _mem = Module_MemoryHeader(i_function->module);

#إذا d_m3HasSnapshots
    إذا (M3_UNLIKELY(i_function->module->isUnusable)) {
        أرجع "a snapshot failed to restore into the module, so it cannot run";
    }
    i_function->module->hasRun = true;
#endif

    d_m3_تيندنن_Limit_Enter(i_runtime);

    // a host function calling back into Wasm does not change which call a trap
    // ends: that is still the one the host made first
    إذا (i_runtime->callNesting++ == 0) {
        i_runtime->entered = i_function;
    }
#إذا d_m3HasExceptionHandling
    // handler stacks don't nest across a call boundary: a host function calling
    // back into Wasm cannot be caught by a try_table its own caller entered
    u32 savedTryDepth   = i_runtime->tryDepth;
    i_runtime->tryDepth = 0;
#endif
#إذا d_m3_Has_تيندنن_Switching
    // nor do continuations: the native frames on the far side of a host call
    // are not ours to record, so a suspend in here has no handler to reach and
    // no resume to unwind to
    IM3Continuation savedContinuation = i_runtime->activeContinuation;

    // A suspended root continuation is a paused program waiting to be resumed,
    // not scratch space. A call the host makes before resuming it gets a
    // context of its own, the way any other re-entry does, rather than having
    // the frames of the suspension reset out from under it.
    bool claim_اکر = i_runtime->isSuspendable and not savedContinuation and
                     not (i_runtime->اکر_Continuation and
                          i_runtime->اکر_Continuation->state == cont_suspended);

    إذا (claim_اکر and not i_runtime->اکر_Continuation) {
        i_runtime->اکر_Continuation = Continuation_New(i_runtime, i_function->funcType, i_function);

        إذا (i_runtime->اکر_Continuation) {
            // The root borrows the runtime's stack and is not charged as cont.new.
            i_runtime->اکر_Continuation->اڠک_تيندنن_Slots = i_runtime->اڠک_تيندنن_Slots;
        } وإلا {
            claim_اکر = false;
        }
    }

    إذا (claim_اکر) {
        i_runtime->activeContinuation              = i_runtime->اکر_Continuation;
        i_runtime->اکر_Continuation->entryFunction = i_function;
        i_runtime->اکر_Continuation->state         = cont_running;
        i_runtime->اکر_Continuation->اڠک_Frames     = 0;
    } وإلا {
        i_runtime->activeContinuation = عدم;
    }
#endif
#إذا d_m3GuardedMemory
    M3Result result = RunCodeProtected(i_runtime, i_pc, _mem);
#وإلاإذا (d_m3EnableOpProfiling || d_m3EnableOpTracing)
    M3Result result = (M3Result)RunCode(i_pc, (m3stack_t)i_runtime->تيندنن, _mem, d_m3OpDefaultArgs, d_m3BaseCstr);
#وإلا
    M3Result result = (M3Result)RunCode(i_pc, (m3stack_t)i_runtime->تيندنن, _mem, d_m3OpDefaultArgs);
#endif
#إذا d_m3_Has_تيندنن_Switching
    إذا (claim_اکر) {
        i_runtime->اکر_Continuation->state =
          (result == m3Err_continuationSuspended) ? cont_suspended : cont_returned;

        Runtime_Place_Call_تيندنن(i_runtime);
    }
    i_runtime->activeContinuation = savedContinuation;
#endif
#إذا d_m3HasExceptionHandling
    i_runtime->tryDepth = savedTryDepth;

    // an exception that reached the bottom of the call stack found no handler
    إذا (M3_UNLIKELY(result == m3Err_pendingException)) {
        result = m3Err_trapUncaughtException;
    }
    RecordUncaughtException(i_runtime, result);

    // a paused invocation can still reach the exceptions it holds, so they wait
    // for it to finish
    إذا (i_runtime->callNesting == 1 and not m3_IsSuspended(i_runtime)) {
        FreeExceptions(i_runtime);
    }
#endif
    i_runtime->callNesting--;
    d_m3_تيندنن_Limit_Leave(i_runtime);

    أرجع result;
}

M3Result m3_RunStart (IM3Module io_module)
{
#ifdef FUZZING_BUILD_MODE_UNSAFE_FOR_PRODUCTION
    // Execution disabled for fuzzing builds
    أرجع m3Err_none;
#endif

    M3Result result           = m3Err_none;
    i32      startFunctionTmp = -1;

    إذا (io_module and io_module->startFunction >= 0) {
        IM3Function function = &io_module->functions[io_module->startFunction];

        إذا (not function->compiled) {
_           (CompileFunction(function));
        }

        IM3FuncType ftype = function->funcType;
        إذا (ftype->اڠک_Args != 0 || ftype->اڠک_Rets != 0) {
            _throw(m3_Err_argument_کيراءن_Mismatch);
        }

        IM3Module  module  = function->module;
        IM3Runtime runtime = module->runtime;

        startFunctionTmp = io_module->startFunction;

        io_module->startFunction = -1;

        result = RunCodeChecked(runtime, function);

        // a start function that suspended is not to be started again: resuming
        // is what finishes it
        إذا (result and result != m3Err_continuationSuspended) {
            io_module->startFunction = startFunctionTmp;
            EXCEPTION_PRINT(result);
            اذهب _catch;
        }
    }

    _catch: أرجع result;
}

// TODO: deal with main + side-modules loading efforcement
M3Result m3_LoadModule (IM3Runtime io_runtime, IM3Module io_module)
{
    M3Result result = m3Err_none;

    إذا (M3_UNLIKELY(io_module->runtime)) {
        أرجع m3Err_moduleAlreadyLinked;
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
    أرجع result; // ok

_catch:
    // The runtime owns the module either way. Instantiation may already have
    // written this module's functions into a table another module owns, and the
    // spec keeps whatever it managed to do before the trap, so those entries
    // stay callable - retaining the module is what stops them dangling.
    //
    // It goes on the tail of the list rather than the head: it is not a module
    // anyone should find by name, only one whose functions may still be
    // reachable through someone else's table.
    io_module->next = عدم;

    IM3Module* tail = &io_runtime->modules;
    طالما (*tail) {
        tail = &(*tail)->next;
    }
    *tail = io_module;

    أرجع result;
}

IM3Global m3_FindGlobal (IM3Module         io_module,
                         ثابت حرف* ثابت i_globalName)
{
    // Search exports
    IM3Global exported = Module_FindExportedGlobal(io_module, i_globalName);
    إذا (exported) {
        // a re-exported global is the one that was imported, so reads and
        // writes have to reach the cell that actually holds the value
        أرجع exported->resolved ? exported->resolved : exported;
    }

    // Search imports
    لكل (u32 i = 0; i < io_module->اڠک_Globals; ++i) {
        IM3Global g = &io_module->globals[i];

        إذا (g->import.moduleUtf8 and g->import.fieldUtf8) {
            إذا (strcmp(g->import.fieldUtf8, i_globalName) == 0) {
                أرجع g->resolved ? g->resolved : g;
            }
        }
    }
    أرجع عدم;
}

M3Result m3_GetGlobal (IM3Global      i_global,
                       IM3_Tagged_نيلاي o_نيلاي)
{
    إذا (not i_global) {
        أرجع m3Err_globalLookupFailed;
    }

    بدل (i_global->type) {
    حالة c_m3Type_i32: o_نيلاي->نيلاي.i32 = i_global->i32_نيلاي; اكسر;
    حالة c_m3Type_i64: o_نيلاي->نيلاي.i64 = i_global->i64_نيلاي; اكسر;
#إذا d_m3HasFloat
    حالة c_m3Type_f32: o_نيلاي->نيلاي.f32 = i_global->f32_نيلاي; اكسر;
    حالة c_m3Type_f64: o_نيلاي->نيلاي.f64 = i_global->f64_نيلاي; اكسر;
#endif
    افتراضي: أرجع m3Err_invalidTypeId;
    }

    o_نيلاي->type = (M3_نيلاي_Type)(i_global->type);
    أرجع m3Err_none;
}

M3Result m3_SetGlobal (IM3Global            i_global,
                       ثابت IM3_Tagged_نيلاي i_نيلاي)
{
    إذا (not i_global) {
        أرجع m3Err_globalLookupFailed;
    }
    إذا (not i_global->isMutable) {
        أرجع m3Err_globalNotMutable;
    }
    إذا (i_global->type != i_نيلاي->type) {
        أرجع m3Err_globalTypeMismatch;
    }

    بدل (i_نيلاي->type) {
    حالة c_m3Type_i32: i_global->i32_نيلاي = i_نيلاي->نيلاي.i32; اكسر;
    حالة c_m3Type_i64: i_global->i64_نيلاي = i_نيلاي->نيلاي.i64; اكسر;
#إذا d_m3HasFloat
    حالة c_m3Type_f32: i_global->f32_نيلاي = i_نيلاي->نيلاي.f32; اكسر;
    حالة c_m3Type_f64: i_global->f64_نيلاي = i_نيلاي->نيلاي.f64; اكسر;
#endif
    افتراضي: أرجع m3Err_invalidTypeId;
    }

    أرجع m3Err_none;
}

M3_نيلاي_Type m3_GetGlobalType (IM3Global i_global)
{
    أرجع (i_global) ? (M3_نيلاي_Type)(i_global->type) : c_m3Type_none;
}


فراغ* v_FindFunction (IM3Module i_module, فراغ* i_info)
{
    ثابت حرف* ثابت i_name = (ثابت حرف*)i_info;

    // Prefer exported functions
    IM3Function exported = Module_FindExportedFunction(i_module, i_name);
    إذا (exported) {
        أرجع exported;
    }

    // Search internal functions
    لكل (u32 i = 0; i < i_module->اڠک_Functions; ++i) {
        IM3Function f = &i_module->functions[i];

        bool isImported = f->import.moduleUtf8 or f->import.fieldUtf8;

        إذا (isImported) {
            استمر;
        }

        لكل (صحيح j = 0; j < f->اڠک_Names; j++) {
            إذا (f->names[j] and strcmp(f->names[j], i_name) == 0) {
                أرجع f;
            }
        }
    }

    أرجع عدم;
}


// Shared tail of the two lookups: a function is only usable once it has code.
ساكن
M3Result PrepareFoundFunction (IM3Function* o_function, IM3Function i_function)
{
    M3Result result = m3Err_none;

    إذا (not i_function->compiled) {
_       (CompileFunction(i_function))
    }

    _catch:
    *o_function = result ? عدم : i_function;

    أرجع result;
}


// Searches every module in the runtime, most recently loaded first. That is a
// guess once more than one module is loaded and two of them export the same
// name - m3_FindFunctionIn says which module is meant.
M3Result m3_FindFunction (IM3Function* o_function, IM3Runtime i_runtime, ثابت حرف* ثابت i_functionName)
{
                                                                d_m3Assert (o_function and i_runtime and i_functionName);
    IM3Function function = عدم;

    إذا (not i_runtime->modules) {
        *o_function = عدم;
        أرجع "no modules loaded";
    }

    function = (IM3Function)ForEachModule(i_runtime, v_FindFunction, (فراغ*)i_functionName);

    إذا (not function) {
        *o_function = عدم;
        أرجع ErrorModule(m3Err_functionLookupFailed, i_runtime->modules, "'%s'", i_functionName);
    }

    أرجع PrepareFoundFunction(o_function, function);
}


IM3Module m3_FindModule (IM3Runtime i_runtime, ثابت حرف* ثابت i_moduleName)
{
    إذا (not i_runtime or not i_moduleName) {
        أرجع عدم;
    }

    // the list is newest-first, so a name registered twice names the newer one
    لكل (IM3Module m = i_runtime->modules; m; m = m->next) {
        إذا (m->name and strcmp(m->name, i_moduleName) == 0) {
            أرجع m;
        }
    }

    أرجع عدم;
}


// Searches one module's exports, which is what naming a module means.
M3Result m3_FindFunctionIn (IM3Function* o_function, IM3Module i_module, ثابت حرف* ثابت i_functionName)
{
                                                                d_m3Assert (o_function and i_functionName);
    إذا (not i_module) {
        *o_function = عدم;
        أرجع m3Err_functionLookupFailed;      // ErrorModule would deref it
    }

    IM3Function function = (IM3Function)v_FindFunction(i_module, (فراغ*)i_functionName);

    إذا (not function) {
        *o_function = عدم;
        أرجع ErrorModule(m3Err_functionLookupFailed, i_module, "'%s'", i_functionName);
    }

    أرجع PrepareFoundFunction(o_function, function);
}


M3Result m3_GetTableFunction (IM3Function* o_function, IM3Module i_module, uint32_t i_index)
{
_try {
    M3Table*    table;
    IM3Function function;

    _throwif("no table", i_module->اڠک_Tables == 0);

    table = i_module->tables[0];
    _throwif("function index out of range", i_index >= table->size);

    function = (IM3Function)table->elements[i_index];

    إذا (function) {
        إذا (not function->compiled) {
_           (CompileFunction(function))
        }
    }

    *o_function = function;
} _catch:
    أرجع result;
}


ساكن
M3Result checkStartFunction (IM3Module i_module)
{
    M3Result result = m3Err_none;                               d_m3Assert(i_module);

    // Check if start function needs to be called
    إذا (i_module->startFunction >= 0) {
        result = m3_RunStart(i_module);
    }

    أرجع result;
}

uint32_t m3_Get_Arg_کيراءن (IM3Function i_function)
{
    إذا (i_function) {
        IM3FuncType ft = i_function->funcType;
        إذا (ft) {
            أرجع ft->اڠک_Args;
        }
    }
    أرجع 0;
}

uint32_t m3_Get_Ret_کيراءن (IM3Function i_function)
{
    إذا (i_function) {
        IM3FuncType ft = i_function->funcType;
        إذا (ft) {
            أرجع ft->اڠک_Rets;
        }
    }
    أرجع 0;
}


M3_نيلاي_Type m3_GetArgType (IM3Function i_function, uint32_t index)
{
    إذا (i_function) {
        IM3FuncType ft = i_function->funcType;
        إذا (ft and index < ft->اڠک_Args) {
            أرجع (M3_نيلاي_Type)BaseTypeOf(d_FuncArgType(ft, index));
        }
    }
    أرجع c_m3Type_none;
}

M3_نيلاي_Type m3_GetRetType (IM3Function i_function, uint32_t index)
{
    إذا (i_function) {
        IM3FuncType ft = i_function->funcType;
        إذا (ft and index < ft->اڠک_Rets) {
            أرجع (M3_نيلاي_Type)BaseTypeOf(d_FuncRetType(ft, index));
        }
    }
    أرجع c_m3Type_none;
}


// The call APIs marshal their arguments into the runtime stack before any
// compiled code runs, so op_Entry's overflow check comes too late to cover those
// writes - and for a host function or an import linked to another module there
// is no op_Entry of our own to reach at all. Bound the area about to be written
// here instead: results and arguments each take a u64 at this boundary, however
// wide the slot is inside. Every caller goes through this, so none of them can
// be the one that forgets.
ساكن
M3Result Get_تيندنن_Pointer_For_Args (IM3Function i_function, u8** o_تيندنن)
{
    IM3Runtime  runtime = i_function->module->runtime;
    IM3FuncType ftype   = i_function->funcType;

    // the stack a paused invocation holds is not available to this one
    size_t used      = (size_t)((u8*)runtime->تيندنن - (u8*)runtime->origin_تيندنن);
    size_t needed    = ((size_t)ftype->اڠک_Rets + ftype->اڠک_Args) * حجم(u64);
    size_t available = (size_t)runtime->اڠک_تيندنن_Slots * حجم(m3slot_t) - used;

    إذا (needed > available) {
        أرجع m3_Err_trap_تيندنن_Overflow;
    }

    u64* تيندنن = (u64*)runtime->تيندنن;

    تيندنن += ftype->اڠک_Rets;

    *o_تيندنن = (u8*)تيندنن;

    أرجع m3Err_none;
}


M3Result m3_CallV (IM3Function i_function, ...)
{
    va_list ap;
    va_start(ap, i_function);
    M3Result r = m3_CallVL(i_function, ap);
    va_end(ap);
    أرجع r;
}

ساكن
فراغ Report_Native_تيندنن_Usage ()
{
#إذا d_m3_Log_Native_تيندنن
    صحيح تيندنن_Used = m3_تيندنن_Get_Max();
    fprintf(stderr, "Native stack used: %d\n", تيندنن_Used);
#endif
}


M3Result m3_CallVL (IM3Function i_function, va_list i_args)
{
    IM3Runtime  runtime = i_function->module->runtime;
    IM3FuncType ftype   = i_function->funcType;
    M3Result    result  = m3Err_none;
    u8*         s       = عدم;

    إذا (!i_function->compiled) {
        أرجع m3Err_missingCompiledCode;
    }

#إذا d_m3RecordBacktraces
    ClearBacktrace(runtime);
#endif

    m3_تيندنن_Check_Init();

_   (checkStartFunction(i_function->module))

_   (Get_تيندنن_Pointer_For_Args(i_function, &s))

    لكل (u32 i = 0; i < ftype->اڠک_Args; ++i) {
        بدل (d_FuncArgType(ftype, i)) {
        حالة c_m3Type_i32:
            *(i32*)(s) = va_arg(i_args, i32);
            s += 8;
            اكسر;
        حالة c_m3Type_i64:
            *(i64*)(s) = va_arg(i_args, i64);
            s += 8;
            اكسر;
        حالة c_m3Type_funcref:
        حالة c_m3Type_externref:
        حالة c_m3Type_exnref:
            *(uintptr_t*)(s) = va_arg(i_args, uintptr_t);
            s += 8;
            اكسر;
#إذا d_m3HasFloat
        حالة c_m3Type_f32:
            *(f32*)(s) = (f32)va_arg(i_args, f64);
            s += 8;
            اكسر; // f32 is passed as f64
        حالة c_m3Type_f64:
            *(f64*)(s) = va_arg(i_args, f64);
            s += 8;
            اكسر;
#endif
        افتراضي: أرجع "unknown argument type";
        }
    }

    result = RunCodeChecked(runtime, i_function);
    Report_Native_تيندنن_Usage();

    runtime->lastCalled = result ? عدم : i_function;

    _catch: أرجع result;
}

M3Result m3_Call (IM3Function i_function, uint32_t i_argc, ثابت فراغ* i_argptrs[])
{
    IM3Runtime  runtime = i_function->module->runtime;
    IM3FuncType ftype   = i_function->funcType;
    M3Result    result  = m3Err_none;
    u8*         s       = عدم;

    إذا (i_argc != ftype->اڠک_Args) {
        أرجع m3_Err_argument_کيراءن_Mismatch;
    }
    إذا (!i_function->compiled) {
        أرجع m3Err_missingCompiledCode;
    }

#إذا d_m3RecordBacktraces
    ClearBacktrace(runtime);
#endif

    m3_تيندنن_Check_Init();

_   (checkStartFunction(i_function->module))

_   (Get_تيندنن_Pointer_For_Args(i_function, &s))

    لكل (u32 i = 0; i < ftype->اڠک_Args; ++i) {
        بدل (d_FuncArgType(ftype, i)) {
        حالة c_m3Type_i32:
            *(i32*)(s) = *(i32*)i_argptrs[i];
            s += 8;
            اكسر;
        حالة c_m3Type_i64:
            *(i64*)(s) = *(i64*)i_argptrs[i];
            s += 8;
            اكسر;
        حالة c_m3Type_funcref:
        حالة c_m3Type_externref:
        حالة c_m3Type_exnref:
            *(uintptr_t*)(s) = *(uintptr_t*)i_argptrs[i];
            s += 8;
            اكسر;
#إذا d_m3HasFloat
        حالة c_m3Type_f32:
            *(f32*)(s) = *(f32*)i_argptrs[i];
            s += 8;
            اكسر;
        حالة c_m3Type_f64:
            *(f64*)(s) = *(f64*)i_argptrs[i];
            s += 8;
            اكسر;
#endif
        افتراضي: أرجع "unknown argument type";
        }
    }

    result = RunCodeChecked(runtime, i_function);

    Report_Native_تيندنن_Usage();

    runtime->lastCalled = result ? عدم : i_function;

    _catch: أرجع result;
}

// Argument parsing for m3_CallArgv. Strict on purpose: the whole string has to
// be consumed, so "12abc" is rejected rather than read as 12, and an empty or
// unparsable argument is an error rather than the zero that strtoul with a
// NULL end pointer used to hand back. See wasm3/wasm3#367.
ساكن
M3Result ParseArgInteger (ccstr_t i_arg, u32 i_اڠک_Bits, u64* o_نيلاي)
{
    إذا (not i_arg or not *i_arg) {
        أرجع "empty argument";
    }

    // strtoull would skip leading space, but trailing space is rejected below;
    // accepting one and not the other would just be confusing
    إذا (isspace((unsigned حرف)*i_arg)) {
        أرجع "argument is not a number";
    }

    حرف* end = عدم;
    u64   نيلاي;

    errno = 0;

    // an argument may be spelled signed or unsigned: -1 and 4294967295 name the
    // same i32
    إذا (*i_arg == '-') {
        i64 signed_نيلاي = strtoll(i_arg, &end, 10);

        إذا (i_اڠک_Bits == 32 and (signed_نيلاي < INT32_MIN or signed_نيلاي > INT32_MAX)) {
            أرجع "argument out of range";
        }

        نيلاي = (u64)signed_نيلاي;
    } وإلا {
        نيلاي = strtoull(i_arg, &end, 10);

        إذا (i_اڠک_Bits == 32 and نيلاي > UINT32_MAX) {
            أرجع "argument out of range";
        }
    }

    إذا (errno == ERANGE) {
        أرجع "argument out of range";
    }

    إذا (end == i_arg or *end) {
        أرجع "argument is not a number";
    }

    *o_نيلاي = نيلاي;

    أرجع m3Err_none;
}


#إذا d_m3HasFloat
ساكن
M3Result ParseArgFloat (ccstr_t i_arg, f64* o_نيلاي)
{
    إذا (not i_arg or not *i_arg) {
        أرجع "empty argument";
    }

    إذا (isspace((unsigned حرف)*i_arg)) {
        أرجع "argument is not a number";
    }

    حرف* end = عدم;

    errno = 0;

    f64 نيلاي = strtod(i_arg, &end);

    إذا (end == i_arg or *end) {
        أرجع "argument is not a number";
    }

    // strtod reports underflow through ERANGE as well, and a denormal result is
    // perfectly usable, so only an overflow to infinity is out of range
    إذا (errno == ERANGE and (نيلاي > DBL_MAX or نيلاي < -DBL_MAX)) {
        أرجع "argument out of range";
    }

    *o_نيلاي = نيلاي;

    أرجع m3Err_none;
}
#endif


// A reference argument is either the null reference or a host handle written as
// an integer.
ساكن
M3Result ParseArgReference (ccstr_t i_arg, u64* o_نيلاي)
{
    إذا (i_arg and strcmp(i_arg, "null") == 0) {
        *o_نيلاي = 0;
        أرجع m3Err_none;
    }

    أرجع ParseArgInteger(i_arg, 64, o_نيلاي);
}


M3Result m3_CallArgv (IM3Function i_function, uint32_t i_argc, ثابت حرف* i_argv[])
{
    IM3FuncType ftype   = i_function->funcType;
    IM3Runtime  runtime = i_function->module->runtime;
    M3Result    result  = m3Err_none;
    u8*         s       = عدم;

    إذا (i_argc != ftype->اڠک_Args) {
        أرجع m3_Err_argument_کيراءن_Mismatch;
    }
    إذا (!i_function->compiled) {
        أرجع m3Err_missingCompiledCode;
    }

#إذا d_m3RecordBacktraces
    ClearBacktrace(runtime);
#endif

    m3_تيندنن_Check_Init();

_   (checkStartFunction(i_function->module))

_   (Get_تيندنن_Pointer_For_Args(i_function, &s))

    لكل (u32 i = 0; i < ftype->اڠک_Args; ++i) {
        u64 نيلاي = 0;
#إذا d_m3HasFloat
        f64 fvalue = 0;
#endif
        بدل (d_FuncArgType(ftype, i)) {
        حالة c_m3Type_i32:
_           (ParseArgInteger(i_argv[i], 32, &نيلاي)) * (i32*)(s) = (i32)نيلاي;
            s += 8;
            اكسر;
        حالة c_m3Type_i64:
_           (ParseArgInteger(i_argv[i], 64, &نيلاي)) * (i64*)(s) = (i64)نيلاي;
            s += 8;
            اكسر;
        حالة c_m3Type_funcref:
        حالة c_m3Type_externref:
        حالة c_m3Type_exnref:
_           (ParseArgReference(i_argv[i], &نيلاي)) * (uintptr_t*)(s) = (uintptr_t)نيلاي;
            s += 8;
            اكسر;
#إذا d_m3HasFloat
            // strtof would be less portable
        حالة c_m3Type_f32:
_           (ParseArgFloat(i_argv[i], &fvalue)) * (f32*)(s) = (f32)fvalue;
            s += 8;
            اكسر;
        حالة c_m3Type_f64:
_           (ParseArgFloat(i_argv[i], &fvalue)) * (f64*)(s) = fvalue;
            s += 8;
            اكسر;
#endif
        افتراضي: _throw("unknown argument type");
        }
    }

    result = RunCodeChecked(runtime, i_function);

    Report_Native_تيندنن_Usage();

    runtime->lastCalled = result ? عدم : i_function;

    _catch: أرجع result;
}


//u8 * AlignStackPointerTo64Bits (const u8 * i_stack)
//{
//    uintptr_t ptr = (uintptr_t) i_stack;
//    return (u8 *) ((ptr + 7) & ~7);
//}


M3Result m3_GetResults (IM3Function i_function, uint32_t i_retc, ثابت فراغ* o_retptrs[])
{
    IM3FuncType ftype   = i_function->funcType;
    IM3Runtime  runtime = i_function->module->runtime;

    إذا (i_retc != ftype->اڠک_Rets) {
        أرجع m3_Err_argument_کيراءن_Mismatch;
    }
    إذا (i_function != runtime->lastCalled) {
        أرجع "function not called";
    }

    u8* s = (u8*)runtime->تيندنن;

    لكل (u32 i = 0; i < ftype->اڠک_Rets; ++i) {
        بدل (d_FuncRetType(ftype, i)) {
        حالة c_m3Type_i32:
            *(i32*)o_retptrs[i] = *(i32*)(s);
            s += 8;
            اكسر;
        حالة c_m3Type_i64:
            *(i64*)o_retptrs[i] = *(i64*)(s);
            s += 8;
            اكسر;
        حالة c_m3Type_funcref:
        حالة c_m3Type_externref:
        حالة c_m3Type_exnref:
            *(uintptr_t*)o_retptrs[i] = *(uintptr_t*)(s);
            s += 8;
            اكسر;
#إذا d_m3HasFloat
        حالة c_m3Type_f32:
            *(f32*)o_retptrs[i] = *(f32*)(s);
            s += 8;
            اكسر;
        حالة c_m3Type_f64:
            *(f64*)o_retptrs[i] = *(f64*)(s);
            s += 8;
            اكسر;
#endif
        افتراضي: أرجع "unknown return type";
        }
    }
    أرجع m3Err_none;
}

M3Result m3_GetResultsV (IM3Function i_function, ...)
{
    va_list ap;
    va_start(ap, i_function);
    M3Result r = m3_GetResultsVL(i_function, ap);
    va_end(ap);
    أرجع r;
}

M3Result m3_GetResultsVL (IM3Function i_function, va_list o_rets)
{
    IM3Runtime  runtime = i_function->module->runtime;
    IM3FuncType ftype   = i_function->funcType;

    إذا (i_function != runtime->lastCalled) {
        أرجع "function not called";
    }

    u8* s = (u8*)runtime->تيندنن;
    لكل (u32 i = 0; i < ftype->اڠک_Rets; ++i) {
        بدل (d_FuncRetType(ftype, i)) {
        حالة c_m3Type_i32:
            *va_arg(o_rets, i32*) = *(i32*)(s);
            s += 8;
            اكسر;
        حالة c_m3Type_i64:
            *va_arg(o_rets, i64*) = *(i64*)(s);
            s += 8;
            اكسر;
        حالة c_m3Type_funcref:
        حالة c_m3Type_externref:
        حالة c_m3Type_exnref:
            *va_arg(o_rets, uintptr_t*) = *(uintptr_t*)(s);
            s += 8;
            اكسر;
#إذا d_m3HasFloat
        حالة c_m3Type_f32:
            *va_arg(o_rets, f32*) = *(f32*)(s);
            s += 8;
            اكسر;
        حالة c_m3Type_f64:
            *va_arg(o_rets, f64*) = *(f64*)(s);
            s += 8;
            اكسر;
#endif
        افتراضي: أرجع "unknown argument type";
        }
    }
    أرجع m3Err_none;
}

فراغ ReleaseCodePageNoTrack (IM3Runtime i_runtime, IM3CodePage i_codePage)
{
    إذا (i_codePage) {
        IM3CodePage* list;

        bool pageFull = (اڠک_Free_باريس_سموا(i_codePage) < d_m3_Code_Page_Free_باريس_سموا_Threshold);
        إذا (pageFull) {
            list = &i_runtime->pagesFull;
        } وإلا {
            list = &i_runtime->pagesOpen;
        }

        تولق_Code_Page(list, i_codePage);                         m3log (emit, "release page: %d to queue: '%s'", i_codePage->info.sequence, pageFull ? "full" : "open")
    }
}


IM3CodePage AcquireCodePageWithCapacity (IM3Runtime i_runtime, u32 i_min_باريس_کيراءن)
{
    IM3CodePage page = RemoveCodePageOfCapacity(&i_runtime->pagesOpen, i_min_باريس_کيراءن);

    إذا (not page) {
        page = Environment_AcquireCodePage(i_runtime->environment, i_min_باريس_کيراءن);

        إذا (not page) {
            page = NewCodePage(i_runtime, i_min_باريس_کيراءن);
        }

        إذا (page) {
            i_runtime->اڠک_Code_Pages++;
        }
    }

    إذا (page) {                                                  m3log (emit, "acquire page: %d", page->info.sequence);
        i_runtime->اڠک_Active_Code_Pages++;
    }

    أرجع page;
}


IM3CodePage AcquireCodePage (IM3Runtime i_runtime)
{
    أرجع AcquireCodePageWithCapacity(i_runtime, d_m3_Code_Page_Free_باريس_سموا_Threshold);
}


فراغ ReleaseCodePage (IM3Runtime i_runtime, IM3CodePage i_codePage)
{
    إذا (i_codePage) {
        ReleaseCodePageNoTrack(i_runtime, i_codePage);
        i_runtime->اڠک_Active_Code_Pages--;

#إذا defined(DEBUG)
        u32 اڠک_Open = کيراءن_Code_Pages(i_runtime->pagesOpen);
        u32 اڠک_Full = کيراءن_Code_Pages(i_runtime->pagesFull);

        m3log(runtime, "runtime: %p; open-pages: %d; full-pages: %d; active: %d; total: %d", i_runtime, اڠک_Open, اڠک_Full, i_runtime->اڠک_Active_Code_Pages, i_runtime->اڠک_Code_Pages);

        d_m3Assert(اڠک_Open + اڠک_Full + i_runtime->اڠک_Active_Code_Pages == i_runtime->اڠک_Code_Pages);

#  إذا d_m3LogCodePages
        dump_code_page(i_codePage, /* startPC: */ عدم);
#  endif
#endif
    }
}


#إذا d_m3VerboseErrorMessages
M3Result m3Error (M3Result i_result, IM3Runtime i_runtime, IM3Module i_module, IM3Function i_function,
                  ثابت حرف* ثابت i_file, u32 i_باريس_اڠک, ثابت حرف* ثابت i_errorMessage, ...)
{
    إذا (i_runtime) {
        i_runtime->error         = (M3ErrorInfo){ .result = i_result, .runtime = i_runtime, .module = i_module, .function = i_function, .file = i_file, .باريس_2 = i_باريس_اڠک };
        i_runtime->error.message = i_runtime->error_message;

        va_list args;
        va_start(args, i_errorMessage);
        vsnprintf(i_runtime->error_message, حجم(i_runtime->error_message), i_errorMessage, args);
        va_end(args);
    }

    أرجع i_result;
}
#endif


فراغ m3_GetErrorInfo (IM3Runtime i_runtime, M3ErrorInfo* o_info)
{
    إذا (i_runtime) {
        *o_info = i_runtime->error;
        m3_ResetErrorInfo(i_runtime);
    }
}


فراغ m3_ResetErrorInfo (IM3Runtime i_runtime)
{
    إذا (i_runtime) {
        M3_INIT(i_runtime->error);
        i_runtime->error.message = "";
    }
}

uint8_t* m3_GetMemory (IM3Module i_module, size_t* o_memorySizeInBytes, uint32_t i_memoryIndex)
{
    uint8_t* memory = عدم;
    size_t   size   = 0;

    إذا (i_module and i_memoryIndex < i_module->اڠک_Memories) {
        IM3Memory mem = i_module->memories[i_memoryIndex];

        إذا (mem->mallocated) {
            size = mem->mallocated->length;

            إذا (size) {
                memory = m3MemData(mem->mallocated);
            }
        }
    }

    إذا (o_memorySizeInBytes) {
        *o_memorySizeInBytes = size;
    }

    أرجع memory;
}


size_t m3_GetMemorySize (IM3Module i_module, uint32_t i_memoryIndex)
{
    إذا (not i_module or i_memoryIndex >= i_module->اڠک_Memories) {
        أرجع 0;
    }

    IM3Memory mem = i_module->memories[i_memoryIndex];

    أرجع mem->mallocated ? mem->mallocated->length : 0;
}


size_t m3_GetMemorySizeAt (ثابت فراغ* i_memory)
{
    إذا (not i_memory) {
        أرجع 0;
    }

    // the header sits immediately before the data it describes
    ثابت M3MemoryHeader* header = ((ثابت M3MemoryHeader*)i_memory) - 1;

    أرجع header->length;
}


M3Result m3_FindExportedMemory (IM3Module i_module, cstr_t i_name, u32* o_memoryIndex)
{
    إذا (not i_module or not i_name or not o_memoryIndex) {
        أرجع m3Err_unknownMemory;
    }

    ثابت M3Export* e = Module_FindExport(i_module, d_externalKind_memory, i_name);

    إذا (not e) {
        أرجع m3Err_unknownMemory;
    }

    *o_memoryIndex = e->index;
    أرجع m3Err_none;
}


bool Module_HasLinkedHostImport (IM3Module i_module, cstr_t i_importModule)
{
    لكل (u32 i = 0; i < i_module->اڠک_Functions; ++i) {
        IM3Function f = &i_module->functions[i];

        // hostMemory is set when a host function is bound to the import, so it
        // is also the mark of one - see CompileRawFunction
        إذا (not f->hostMemory or f->wasm or f->resolved) {
            استمر;
        }

        إذا (f->import.moduleUtf8 and strcmp(f->import.moduleUtf8, i_importModule) == 0) {
            أرجع true;
        }
    }

    أرجع false;
}


M3Result m3_BindImportMemory (IM3Module io_module, cstr_t i_importModule, u32 i_memoryIndex)
{
    إذا (not io_module or not i_importModule) {
        أرجع m3Err_moduleNotLinked;
    }

    إذا (i_memoryIndex >= io_module->اڠک_Memories) {
        أرجع m3Err_unknownMemory;
    }

    // the memory itself, not the index: linking may have pointed this slot at
    // another module's memory, and growing one reallocates behind it
    IM3Memory memory = io_module->memories[i_memoryIndex];

    ثابت bool wildcardModule = (strcmp(i_importModule, "*") == 0);

    لكل (u32 i = 0; i < io_module->اڠک_Functions; ++i) {
        IM3Function f = &io_module->functions[i];

        // only an import that a host function was bound to has a memory to
        // pin: one with a body of its own reads memory through the interpreter,
        // and one resolved to another module's export runs that module's code
        إذا (not f->hostMemory or f->wasm or f->resolved) {
            استمر;
        }

        إذا (wildcardModule or (f->import.moduleUtf8 and strcmp(f->import.moduleUtf8, i_importModule) == 0)) {
            f->hostMemory = memory;
        }
    }

    أرجع m3Err_none;
}


M3BacktraceInfo* m3_GetBacktrace (IM3Runtime i_runtime)
{
#إذا d_m3RecordBacktraces
    أرجع &i_runtime->backtrace;
#وإلا
    أرجع عدم;
#endif
}


//-------------------------------------------------------------------------------------------------------------------------------
//  snapshots & suspendable execution
//-------------------------------------------------------------------------------------------------------------------------------

فراغ m3_SetSuspendable (IM3Runtime io_runtime, bool i_suspendable)
{
    إذا (io_runtime) {
        io_runtime->isSuspendable = i_suspendable;
    }
}

فراغ m3_RequestSuspend (IM3Runtime io_runtime)
{
    إذا (io_runtime) {
        io_runtime->suspendRequested = true;
    }
}

bool m3_IsSuspended (IM3Runtime i_runtime)
{
    إذا (!i_runtime) {
        أرجع false;
    }
#إذا d_m3_Has_تيندنن_Switching
    إذا (i_runtime->اکر_Continuation && i_runtime->اکر_Continuation->state == cont_suspended) {
        أرجع true;
    }
#endif
    أرجع false;
}

M3Result m3_ResumeRuntime (IM3Runtime io_runtime)
{
    إذا (!io_runtime) {
        أرجع m3Err_mallocFailed;
    }
    إذا (!io_runtime->isSuspendable) {
        أرجع m3Err_none;
    }
#إذا d_m3HasSnapshots
    // a restore that failed leaves nothing to resume, and says so every time
    // rather than resuming nothing successfully
    إذا (io_runtime->اکر_Continuation and io_runtime->اکر_Continuation->entryFunction and
        io_runtime->اکر_Continuation->entryFunction->module->isUnusable) {
        أرجع "a snapshot failed to restore into the module, so it cannot run";
    }
#endif
#إذا d_m3_Has_تيندنن_Switching
    إذا (!io_runtime->اکر_Continuation || io_runtime->اکر_Continuation->state != cont_suspended) {
        أرجع m3Err_none;
    }
    // what finishes returns into the bottom of the stack, where it was entered
    io_runtime->تيندنن = io_runtime->origin_تيندنن;

    // this is the outermost call again, as far as a host function calling back
    // into Wasm from inside it can tell
    io_runtime->entered = io_runtime->اکر_Continuation->entryFunction;
    io_runtime->callNesting++;

    d_m3_تيندنن_Limit_Enter(io_runtime);
    io_runtime->resumePastCheck = true;
    M3Result result             = (M3Result)ResumeContinuation(io_runtime, io_runtime->اکر_Continuation);
    io_runtime->resumePastCheck = false;
    d_m3_تيندنن_Limit_Leave(io_runtime);

#  إذا d_m3HasExceptionHandling
    إذا (M3_UNLIKELY(result == m3Err_pendingException)) {
        result = m3Err_trapUncaughtException;
    }
    RecordUncaughtException(io_runtime, result);
#  endif

    io_runtime->callNesting--;

#  إذا d_m3HasExceptionHandling
    إذا (result != m3Err_continuationSuspended and io_runtime->callNesting == 0) {
        FreeExceptions(io_runtime);
    }
#  endif

    // the results are where the paused call left room for them, so a finished
    // resume can be asked for them the way the call itself could have been
    io_runtime->lastCalled = (result == m3Err_none) ? io_runtime->اکر_Continuation->entryFunction : عدم;

    Runtime_Place_Call_تيندنن(io_runtime);
    أرجع result;
#وإلا
    أرجع m3Err_none;
#endif
}
