//
//  m3_env.c
//
//  Created by Steven Massey on 4/19/19.
//  Copyright © 2019 Steven Massey. All rights reserved.
//

#অন্তর্ভাব <stdarg.h>
#অন্তর্ভাব <limits.h>
#অন্তর্ভাব <errno.h>
#অন্তর্ভাব <float.h>
#অন্তর্ভাব <ctype.h>

#অন্তর্ভাব "m3_env.h"
#অন্তর্ভাব "m3_bind.h"
#অন্তর্ভাব "m3_deterministic.h"
#অন্তর্ভাব "m3_compile.h"
#অন্তর্ভাব "m3_host.h"
#অন্তর্ভাব "m3_exception.h"
#অন্তর্ভাব "m3_info.h"


IM3Environment m3_NewEnvironment ()
{
    IM3Environment env = m3_AllocStruct(M3Environment);

    যদি (env) {
        _try
        {
            // create FuncTypes for all simple block return ValueTypes.
            // v128 is skipped: it parses as a slot but has no operations.
            জন্য (u8 t = c_m3Type_none; t < c_m3_Type_গুনতি; t++) {
                যদি (t == c_m3Type_v128) {
                    চালু;
                }

                IM3FuncType ftype;
_               (AllocFuncType(&ftype, 1));

                ftype->অঙ্ক_Args  = 0;
                ftype->অঙ্ক_Rets  = (t == c_m3Type_none) ? 0 : 1;
                ftype->types[0] = t;

                Environment_AddFuncType(env, &ftype);

                env->retFuncTypes[t] = ftype;
            }
        }

        _catch:
        যদি (ফলাফল) {
            m3_FreeEnvironment(env);
            env = ফাঁকা;
        }
    }

    ফেরত env;
}


শূন্য Environment_Release (IM3Environment i_environment)
{
    IM3FuncType ftype = i_environment->funcTypes;

    যখন (ftype) {
        IM3FuncType পরবর্তী = ftype->পরবর্তী;
        m3_Free(ftype);
        ftype = পরবর্তী;
    }

    m3log(runtime, "freeing %d pages from environment", গুনতি_Code_Pages(i_environment->pagesReleased));
    FreeCodePages(&i_environment->pagesReleased);
}


শূন্য m3_FreeEnvironment (IM3Environment i_environment)
{
    যদি (i_environment) {
        Environment_Release(i_environment);
        m3_Free(i_environment);
    }
}


শূন্য m3_SetCustomSectionHandler (IM3Environment i_environment, M3SectionHandler i_handler)
{
    যদি (i_environment) {
        i_environment->customSectionHandler = i_handler;
    }
}


// returns the same io_funcType or replaces it with an equivalent that's already in the type linked list
M3_ফলাফল Environment_AddFuncType (IM3Environment i_environment, IM3FuncType* io_funcType)
{
    IM3FuncType addType = *io_funcType;
    IM3FuncType newType = i_environment->funcTypes;

    যখন (newType) {
        যদি (not newType->inRecGroup and AreFuncTypesEqual(newType, addType)) {
            m3_Free(addType);
            বিরতি;
        }

        newType = newType->পরবর্তী;
    }

    যদি (newType == ফাঁকা) {
        // a type index has to fit in the heap type field of an m3type_t
        যদি (i_environment->অঙ্ক_Func_Types >= d_m3_Max_Sane_Types_গুনতি) {
            m3_Free(addType);
            *io_funcType = ফাঁকা;
            ফেরত "too many distinct function types";
        }

        newType                  = addType;
        newType->canonical_সূচক  = i_environment->অঙ্ক_Func_Types++;
        newType->পরবর্তী            = i_environment->funcTypes;
        i_environment->funcTypes = newType;
    }

    *io_funcType = newType;

    ফেরত m3Err_none;
}


// Adds a type without looking for an equal one first, and hands back the index
// it was given.
//
// Members of a recursive group do not get to share an entry with a type that
// merely looks like them: the group is their identity, so two groups spelled
// the same way are still two groups, and a type inside one is never the type
// outside it that matches field for field. Reserving the indices up front is
// also what lets the members refer to each other while they are being read.
M3_ফলাফল Environment_ReserveFuncTypes (IM3Environment i_environment, u32 i_গুনতি, u16* o_first_সূচক)
{
    যদি ((u64)i_environment->অঙ্ক_Func_Types + i_গুনতি > d_m3_Max_Sane_Types_গুনতি) {
        ফেরত "too many distinct function types";
    }

    *o_first_সূচক = i_environment->অঙ্ক_Func_Types;
    i_environment->অঙ্ক_Func_Types += (u16)i_গুনতি;

    ফেরত m3Err_none;
}


শূন্য Environment_AdoptFuncType (IM3Environment i_environment, IM3FuncType i_funcType, u16 i_canonical_সূচক)
{
    i_funcType->inRecGroup     = true;
    i_funcType->canonical_সূচক = i_canonical_সূচক;
    i_funcType->পরবর্তী           = i_environment->funcTypes;
    i_environment->funcTypes   = i_funcType;
}


IM3CodePage RemoveCodePageOfCapacity (M3CodePage** io_list, u32 i_minimum_সারি_গুনতি)
{
    IM3CodePage পূর্ব = ফাঁকা;
    IM3CodePage page = *io_list;

    যখন (page) {
        যদি (অঙ্ক_Free_সারিগুলি(page) >= i_minimum_সারি_গুনতি) {             d_m3Assert (page->info.usage_গুনতি == 0);
            IM3CodePage পরবর্তী = page->info.পরবর্তী;
            যদি (পূর্ব) {
                পূর্ব->info.পরবর্তী = পরবর্তী; // mid-list
            } অন্যথা {
                *io_list = পরবর্তী;       // front of list
            }

            বিরতি;
        }

        পূর্ব = page;
        page = page->info.পরবর্তী;
    }

    ফেরত page;
}


IM3CodePage Environment_AcquireCodePage (IM3Environment i_environment, u32 i_minimum_সারি_গুনতি)
{
    ফেরত RemoveCodePageOfCapacity(&i_environment->pagesReleased, i_minimum_সারি_গুনতি);
}


শূন্য Environment_ReleaseCodePages (IM3Environment i_environment, IM3CodePage i_codePageList)
{
    IM3CodePage end = i_codePageList;

    যখন (end) {
        end->info.সারি_সূচক = 0; // reset page
#যদি d_m3RecordBacktraces
        end->info.mapping->আকার_2 = 0;
#endif // d_m3RecordBacktraces

        IM3CodePage পরবর্তী = end->info.পরবর্তী;
        যদি (not পরবর্তী) {
            বিরতি;
        }

        end = পরবর্তী;
    }

    যদি (end) {
        // push list to front
        end->info.পরবর্তী               = i_environment->pagesReleased;
        i_environment->pagesReleased = i_codePageList;
    }
}


IM3Runtime m3_NewRuntime (IM3Environment i_environment, u32 i_স্তূপ_আকার_In_Bytes, শূন্য* i_userdata)
{
    IM3Runtime runtime = m3_AllocStruct(M3Runtime);

    যদি (runtime) {
        m3_ResetErrorInfo(runtime);

        runtime->environment = i_environment;
        runtime->userdata    = i_userdata;

        Deterministic_InitRuntime(runtime);

        runtime->origin_স্তূপ = m3_Malloc("Wasm Stack", i_স্তূপ_আকার_In_Bytes + 4 * আকার(m3slot_t)); // TODO: more precise stack checks

        যদি (runtime->origin_স্তূপ) {
            runtime->স্তূপ         = runtime->origin_স্তূপ;
            runtime->অঙ্ক_স্তূপ_Slots = i_স্তূপ_আকার_In_Bytes / আকার(m3slot_t);          m3log (runtime, "new stack: %p, slots: %u", runtime->origin_স্তূপ, runtime->অঙ্ক_স্তূপ_Slots);
        } অন্যথা {
            m3_Free(runtime);
        }
    }

    ফেরত runtime;
}

শূন্য m3_SetValidation (IM3Runtime i_runtime, bool i_enable)
{
#যদি d_m3EnableValidation
    যদি (i_runtime) {
        i_runtime->skipValidation = not i_enable;
    }
#অন্যথা
    (শূন্য)i_runtime;
    (শূন্য)i_enable;              // nothing to skip: the validator was compiled out
#endif
}

M3_ফলাফল m3_SetResourceLimit (IM3Runtime i_runtime, M3ResourceLimit i_limit, uint64_t i_মান)
{
    যদি (not i_runtime) {
        ফেরত m3Err_mallocFailed;
    }
    নির্বাচন (i_limit) {
    ক্ষেত্র c_m3Limit_MemoryBytes:
        যদি (i_মান and i_মান < i_runtime->memoryBytesUsed) {
            ফেরত m3Err_resourceLimitBelowUsage;
        }
        i_runtime->memoryBytesLimit = i_মান;
        ফেরত m3Err_none;
    ক্ষেত্র c_m3Limit_TableElements:
        যদি (i_মান and i_মান < i_runtime->tableElementsUsed) {
            ফেরত m3Err_resourceLimitBelowUsage;
        }
        i_runtime->tableElementsLimit = i_মান;
        ফেরত m3Err_none;
#যদি d_m3_Has_স্তূপ_Switching
    ক্ষেত্র c_m3Limit_Continuations:
        যদি (i_মান and i_মান < i_runtime->continuationsAllocated) {
            ফেরত m3Err_resourceLimitBelowUsage;
        }
        i_runtime->continuationsLimit = (u32)M3_MIN(i_মান, (u64)UINT32_MAX);
        যখন (i_মান and i_runtime->val_স্তূপ_Pool_গুনতি > i_মান - i_runtime->continuationsAllocated) {
            m3_Free(i_runtime->val_স্তূপ_Pool[--i_runtime->val_স্তূপ_Pool_গুনতি]);
        }
        ফেরত m3Err_none;
#অন্যথা
    ক্ষেত্র c_m3Limit_Continuations: ফেরত m3Err_resourceLimitNotSupported;
#endif
    ক্ষেত্র c_m3Limit_GasUnits:
#যদি d_m3HasGasMetering
        i_runtime->gasLimit = i_runtime->gasRemaining = (i64)M3_MIN(i_মান, (u64)INT64_MAX);
        ফেরত m3Err_none;
#অন্যথা
        ফেরত m3Err_resourceLimitNotSupported;
#endif
    পূর্বনির্ধারিত: ফেরত m3Err_unknownResourceLimit;
    }
}

uint64_t m3_GetResourceLimit (IM3Runtime i_runtime, M3ResourceLimit i_limit)
{
    যদি (not i_runtime) {
        ফেরত 0;
    }
    নির্বাচন (i_limit) {
    ক্ষেত্র c_m3Limit_MemoryBytes: ফেরত i_runtime->memoryBytesLimit;
    ক্ষেত্র c_m3Limit_TableElements: ফেরত i_runtime->tableElementsLimit;
#যদি d_m3_Has_স্তূপ_Switching
    ক্ষেত্র c_m3Limit_Continuations: ফেরত i_runtime->continuationsLimit;
#endif
#যদি d_m3HasGasMetering
    ক্ষেত্র c_m3Limit_GasUnits: ফেরত (u64)i_runtime->gasLimit;
#endif
    পূর্বনির্ধারিত: ফেরত 0;
    }
}

uint64_t m3_GetResourceUsage (IM3Runtime i_runtime, M3ResourceLimit i_limit)
{
    যদি (not i_runtime) {
        ফেরত 0;
    }
    নির্বাচন (i_limit) {
    ক্ষেত্র c_m3Limit_MemoryBytes: ফেরত i_runtime->memoryBytesUsed;
    ক্ষেত্র c_m3Limit_TableElements: ফেরত i_runtime->tableElementsUsed;
#যদি d_m3_Has_স্তূপ_Switching
    ক্ষেত্র c_m3Limit_Continuations: ফেরত i_runtime->continuationsAllocated;
#endif
#যদি d_m3HasGasMetering
    ক্ষেত্র c_m3Limit_GasUnits: ফেরত (u64)i_runtime->gasLimit - (u64)i_runtime->gasRemaining;
#endif
    পূর্বনির্ধারিত: ফেরত 0;
    }
}

শূন্য* m3_Get_User_তথ্য (IM3Runtime i_runtime)
{
    ফেরত i_runtime ? i_runtime->userdata : ফাঁকা;
}


শূন্য* ForEachModule (IM3Runtime i_runtime, ModuleVisitor i_visitor, শূন্য* i_info)
{
    শূন্য* r = ফাঁকা;

    IM3Module module = i_runtime->modules;

    যখন (module) {
        IM3Module পরবর্তী = module->পরবর্তী;
        যদি ((r = i_visitor(module, i_info))) {
            বিরতি;
        }

        module = পরবর্তী;
    }

    ফেরত r;
}


শূন্য* _FreeModule (IM3Module i_module, শূন্য* i_info)
{
    m3_FreeModule(i_module);
    ফেরত ফাঁকা;
}


// A module's memory and table index spaces hold borrowed pointers: linking
// repoints an imported slot at the memory or table the exporting module owns.
// m3_FreeModule tells a borrowed slot from an owned one by reading that owner,
// which only holds while the object is still allocated - and the free walk
// below visits modules in load order, with no ordering between an owner and
// the modules borrowing from it. So clear the borrowed slots first, in a pass
// that frees nothing; the walk then skips them on the NULL check it already
// does, instead of reading a memory some earlier module has freed.
শূন্য* _ReleaseBorrowedSlots (IM3Module i_module, শূন্য* i_info)
{
    জন্য (u32 i = 0; i < i_module->অঙ্ক_Memories; ++i) {
        IM3Memory memory = i_module->memories[i];

        যদি (memory and memory->owner != i_module) {
            i_module->memories[i] = ফাঁকা;
        }
    }

    জন্য (u32 i = 0; i < i_module->অঙ্ক_Tables; ++i) {
        IM3Table table = i_module->tables[i];

        যদি (table and table->owner != i_module) {
            i_module->tables[i] = ফাঁকা;
        }
    }

    ফেরত ফাঁকা;
}


শূন্য Runtime_Release (IM3Runtime i_runtime)
{
    ForEachModule(i_runtime, _ReleaseBorrowedSlots, ফাঁকা);
    ForEachModule(i_runtime, _FreeModule, ফাঁকা);                    d_m3Assert (i_runtime->অঙ্ক_Active_Code_Pages == 0);

    Environment_ReleaseCodePages(i_runtime->environment, i_runtime->pagesOpen);
    Environment_ReleaseCodePages(i_runtime->environment, i_runtime->pagesFull);

    m3_Free(i_runtime->origin_স্তূপ);

#যদি d_m3EnableValidation
    m3_Free(i_runtime->validator);
#endif

#যদি d_m3HasExceptionHandling
    // a runtime let go of while suspended still holds its exceptions
    FreeExceptions(i_runtime);
    m3_Free(i_runtime->uncaughtException);
#endif
#যদি d_m3HasExceptionHandling || d_m3_Has_স্তূপ_Switching
    যখন (i_runtime->hostTags) {
        M3HostTag* পরবর্তী = i_runtime->hostTags->পরবর্তী;
        m3_Free(i_runtime->hostTags);
        i_runtime->hostTags = পরবর্তী;
    }
#endif
#যদি d_m3_Has_স্তূপ_Switching
    Continuation_ReleaseAll(i_runtime);
#endif
}


শূন্য m3_FreeRuntime (IM3Runtime i_runtime)
{
    যদি (i_runtime) {
        m3_PrintProfilerInfo();

        Runtime_Release(i_runtime);
        m3_Free(i_runtime);
    }
}


#যদি d_m3_Has_স্তূপ_Switching

IM3Continuation Continuation_New (IM3Runtime i_runtime, IM3FuncType i_type, IM3Function i_function)
{
    IM3Continuation cont = m3_AllocStruct(M3Continuation);
    যদি (!cont) {
        ফেরত ফাঁকা;
    }

    cont->type          = i_type;
    cont->entryFunction = i_function;
    cont->state         = cont_allocated;

    cont->অঙ্ক_স্তূপ_Slots = d_m3_Continuation_স্তূপ_Slots;
    // frames are recorded only when this continuation actually suspends, and
    // only as deep as it got, so the array starts out unallocated
    cont->frames    = ফাঁকা;
    cont->framesCap = 0;
    cont->অঙ্ক_Frames = 0;

    cont->sp = cont->val_স্তূপ;
    cont->pc = i_function ? i_function->compiled : ফাঁকা;
    cont->r0 = 0;
#  যদি d_m3HasFloat
    cont->fp0 = 0;
#  endif
    cont->bound_Args_গুনতি    = 0;
    cont->অঙ্ক_Suspend_Results = 0;
    cont->অঙ্ক_Handlers       = 0;
    cont->handlersPC        = ফাঁকা;
    cont->parent            = ফাঁকা;

    cont->পরবর্তী               = i_runtime->continuations;
    i_runtime->continuations = cont;

    ফেরত cont;
}

IM3Continuation Continuation_ForkSuspended (IM3Runtime i_runtime, IM3Continuation i_cont)
{
    IM3Continuation newCont = Continuation_New(i_runtime, i_cont->type, i_cont->entryFunction);
    যদি (M3_UNLIKELY(not newCont)) {
        ফেরত ফাঁকা;
    }

    newCont->val_স্তূপ      = i_cont->val_স্তূপ;
    newCont->অঙ্ক_স্তূপ_Slots = i_cont->অঙ্ক_স্তূপ_Slots;
    newCont->sp            = i_cont->sp;
    newCont->pc            = i_cont->pc;
    newCont->suspendPoint  = i_cont->suspendPoint;
    newCont->r0            = i_cont->r0;
#  যদি d_m3HasFloat
    newCont->fp0 = i_cont->fp0;
#  endif
    newCont->frames    = i_cont->frames;
    newCont->framesCap = i_cont->framesCap;
    newCont->অঙ্ক_Frames = i_cont->অঙ্ক_Frames;

    newCont->bound_Args_গুনতি = 0;
#  যদি d_m3HasExceptionHandling
    newCont->resumeThrow = i_cont->resumeThrow;
    i_cont->resumeThrow  = ফাঁকা;
#  endif
    newCont->অঙ্ক_Suspend_Results = i_cont->অঙ্ক_Suspend_Results;
    জন্য (u32 i = 0; i < i_cont->অঙ্ক_Suspend_Results; ++i) {
        newCont->suspend_ফলাফল_Offsets[i] = i_cont->suspend_ফলাফল_Offsets[i];
        newCont->suspend_ফলাফল_Is64[i]    = i_cont->suspend_ফলাফল_Is64[i];
    }

    newCont->অঙ্ক_Handlers = i_cont->অঙ্ক_Handlers;
    newCont->handlersPC  = i_cont->handlersPC;
    newCont->parent      = i_cont->parent;
    newCont->state       = cont_suspended;

    i_cont->val_স্তূপ  = ফাঁকা;
    i_cont->frames    = ফাঁকা;
    i_cont->framesCap = 0;
    i_cont->অঙ্ক_Frames = 0;
    i_cont->state     = cont_consumed;

    ফেরত newCont;
}

// Only cont.new and snapshot restoration acquire stacks. Binding and suspension
// transfer them between records without changing the active count.
M3_ফলাফল Continuation_Acquire_স্তূপ (IM3Runtime runtime, IM3Continuation cont)
{
    যদি (not Continuation_Can_Acquire_স্তূপ(runtime)) {
        ফেরত m3Err_continuationLimitExceeded;
    }
    যদি (runtime->val_স্তূপ_Pool_গুনতি) {
        cont->val_স্তূপ = runtime->val_স্তূপ_Pool[--runtime->val_স্তূপ_Pool_গুনতি];
        memset(cont->val_স্তূপ, 0, (cont->অঙ্ক_স্তূপ_Slots + 4) * আকার(m3slot_t));
    } অন্যথা {
        cont->val_স্তূপ = m3_AllocArray(m3slot_t, cont->অঙ্ক_স্তূপ_Slots + 4);
    }
    যদি (not cont->val_স্তূপ) {
        ফেরত m3Err_mallocFailed;
    }
    cont->sp = cont->val_স্তূপ;
    runtime->continuationsAllocated++;
    ফেরত m3Err_none;
}

শূন্য Continuation_Recycle_স্তূপ (IM3Runtime runtime, IM3Continuation cont)
{
    যদি (not cont->val_স্তূপ or cont == runtime->মূল_Continuation) {
        ফেরত;
    }
    যদি (not runtime->val_স্তূপ_Pool) {
        runtime->val_স্তূপ_Pool = m3_AllocArray(m3slot_t*, 16);
        যদি (runtime->val_স্তূপ_Pool) {
            runtime->val_স্তূপ_Pool_Cap = 16;
        }
    }
    যদি (runtime->val_স্তূপ_Pool_গুনতি < runtime->val_স্তূপ_Pool_Cap and
        (not runtime->continuationsLimit or runtime->val_স্তূপ_Pool_গুনতি <
                                              runtime->continuationsLimit - runtime->continuationsAllocated + 1)) {
        runtime->val_স্তূপ_Pool[runtime->val_স্তূপ_Pool_গুনতি++] = cont->val_স্তূপ;
        cont->val_স্তূপ                                      = ফাঁকা;
    } অন্যথা {
        m3_Free(cont->val_স্তূপ);
    }
    cont->sp = ফাঁকা;
    runtime->continuationsAllocated--;
}

শূন্য Continuation_ReleaseAll (IM3Runtime io_runtime)
{
    // Records live until teardown: a consumed reference can still be in a table
    // or on a Wasm stack. Its value stack has already been recycled.
    IM3Continuation cont = io_runtime->continuations;

    যখন (cont) {
        IM3Continuation পরবর্তী = cont->পরবর্তী;

        m3_Free(cont->val_স্তূপ);
        m3_Free(cont->frames);
        m3_Free(cont);

        cont = পরবর্তী;
    }

    জন্য (u32 i = 0; i < io_runtime->val_স্তূপ_Pool_গুনতি; ++i) {
        m3_Free(io_runtime->val_স্তূপ_Pool[i]);
    }
    m3_Free(io_runtime->val_স্তূপ_Pool);
    io_runtime->val_স্তূপ_Pool_গুনতি = io_runtime->val_স্তূপ_Pool_Cap = 0;
    io_runtime->continuationsAllocated                          = 0;
    io_runtime->continuations                                   = ফাঁকা;
    io_runtime->activeContinuation                              = ফাঁকা;
    io_runtime->মূল_Continuation                                = ফাঁকা;
}


m3ret_t Continuation_RecordFrame (IM3Runtime i_runtime, ধ্রুবক M3Frame* i_frame)
{
    IM3Continuation cont = i_runtime->activeContinuation;

    // the marker only travels between a suspend and the resume that runs it,
    // and that whole stretch has an active continuation
    d_m3Assert(cont);

    যদি (cont->অঙ্ক_Frames == cont->framesCap) {
        u32 newCap = cont->framesCap ? cont->framesCap * 2 : 8;

        যদি (newCap > d_m3ContinuationMaxFrames) {
            newCap = d_m3ContinuationMaxFrames;
        }

        যদি (newCap == cont->framesCap) {
            ফেরত m3_Err_trap_স্তূপ_Overflow;
        }

        M3Frame* frames = m3_ReallocArray(M3Frame, cont->frames, newCap, cont->framesCap);

        যদি (not frames) {
            ফেরত m3Err_mallocFailed;
        }

        cont->frames    = frames;
        cont->framesCap = newCap;
    }

    cont->frames[cont->অঙ্ক_Frames++] = *i_frame;

    ফেরত m3Err_continuationSuspended;
}

#endif // d_m3HasStackSwitching

M3_ফলাফল EvaluateExpression (IM3Module i_module, শূন্য* o_expressed, m3type_t i_type, bytes_t* io_bytes, cbytes_t i_end)
{
    M3_ফলাফল ফলাফল = m3Err_none;

    // OPTZ: use a simplified interpreter for expressions

    // create a temporary runtime context
#যদি defined(d_m3PreferStaticAlloc)
    স্থির M3Runtime runtime;
#অন্যথা
    M3Runtime runtime;
#endif
    M3_INIT(runtime);

    runtime.environment   = i_module->runtime->environment;
    runtime.অঙ্ক_স্তূপ_Slots = i_module->runtime->অঙ্ক_স্তূপ_Slots;
    runtime.স্তূপ         = i_module->runtime->স্তূপ;

    m3stack_t স্তূপ = (m3stack_t)runtime.স্তূপ;

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

    যদি (o->page) {
        IM3FuncType ftype = ফাঁকা;

#যদি d_m3HasTypedRefs
        // A reference type has to be checked as the type it is: the shared
        // return types are one per storage class, so borrowing one of those
        // would let any reference at all satisfy a particular one.
        যদি (IsSpelledRefType(i_type)) {
            ফলাফল = AllocFuncType(&ftype, 1);

            যদি (not ফলাফল) {
                ftype->অঙ্ক_Rets  = 1;
                ftype->types[0] = i_type;

                ফলাফল = Environment_AddFuncType(runtime.environment, &ftype);
            }
        } অন্যথা
#endif
        {
            ftype = runtime.environment->retFuncTypes[BaseTypeOf(i_type)];
        }

        pc_t m3code = GetPagePC(o->page);

        যদি (not ফলাফল) {
            ফলাফল = CompileExpression(o, ftype);
        }

        যদি (not ফলাফল && o->max_স্তূপ_Slots >= runtime.অঙ্ক_স্তূপ_Slots) {
            ফলাফল = m3_Err_trap_স্তূপ_Overflow;
        }

        যদি (not ফলাফল) {
#যদি (d_m3EnableOpProfiling || d_m3EnableOpTracing)
            m3ret_t r = RunCode(m3code, স্তূপ, ফাঁকা, d_m3OpDefaultArgs, d_m3BaseCstr);
#অন্যথা
            m3ret_t r = RunCode(m3code, স্তূপ, ফাঁকা, d_m3OpDefaultArgs);
#endif

            যদি (r == 0) {                                                                   m3log (runtime, "expression result: %s", SPrint_মান (স্তূপ, i_type));
                যদি (আকার_Of_Type(BaseTypeOf(i_type)) == আকার(u32)) {
                    *(u32*)o_expressed = *((u32*)স্তূপ);
                } অন্যথা {
                    *(u64*)o_expressed = *((u64*)স্তূপ);
                }
            }
        }

        ReleaseCodePage(&runtime, o->page);
    } অন্যথা {
        ফলাফল = m3Err_mallocFailedCodePage;
    }

    runtime.origin_স্তূপ = ফাঁকা;        // prevent free(stack) in ReleaseRuntime
    Runtime_Release(&runtime);
    i_module->runtime = savedRuntime;

    *io_bytes = o->wasm;

    ফেরত ফলাফল;
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
স্থির
bool LimitsSatisfy (u64 i_exported_আকার, bool i_exportedHasMax, u64 i_exportedMax,
                    u64 i_importMin, bool i_importHasMax, u64 i_importMax)
{
    যদি (i_exported_আকার < i_importMin) {
        ফেরত false;
    }

    যদি (i_importHasMax and (not i_exportedHasMax or i_exportedMax > i_importMax)) {
        ফেরত false;
    }

    ফেরত true;
}


// The export of this kind under this name, or NULL. The index it carries is
// resolved by the caller, against the module's index space as it stands then -
// after linking, a re-exported import's slot holds the entity it was linked to.
স্থির
ধ্রুবক M3Export* Module_FindExport (IM3Module i_module, u8 i_kind, cstr_t i_name)
{
    জন্য (u32 i = 0; i < i_module->অঙ্ক_Exports; ++i) {
        ধ্রুবক M3Export* e = &i_module->exports[i];

        যদি (e->kind == i_kind and strcmp(e->name, i_name) == 0) {
            ফেরত e;
        }
    }

    ফেরত ফাঁকা;
}


স্থির
IM3Function Module_FindExportedFunction (IM3Module i_module, cstr_t i_name)
{
    ধ্রুবক M3Export* e = Module_FindExport(i_module, d_externalKind_function, i_name);

    // A module that re-exports an import names the placeholder here. Resolve to
    // the function that actually runs, or a host-side call would run it against
    // the importing module's memory.
    ফেরত e ? Function_Implementation(&i_module->functions[e->index]) : ফাঁকা;
}


স্থির
IM3Memory Module_FindExportedMemory (IM3Module i_module, cstr_t i_name)
{
    ধ্রুবক M3Export* e = Module_FindExport(i_module, d_externalKind_memory, i_name);

    ফেরত e ? i_module->memories[e->index] : ফাঁকা;
}


স্থির
IM3Table Module_FindExportedTable (IM3Module i_module, cstr_t i_name)
{
    ধ্রুবক M3Export* e = Module_FindExport(i_module, d_externalKind_table, i_name);

    ফেরত e ? i_module->tables[e->index] : ফাঁকা;
}


স্থির
IM3Global Module_FindExportedGlobal (IM3Module i_module, cstr_t i_name)
{
    ধ্রুবক M3Export* e = Module_FindExport(i_module, d_externalKind_global, i_name);

    ফেরত e ? &i_module->globals[e->index] : ফাঁকা;
}


#যদি d_m3HasExceptionHandling || d_m3_Has_স্তূপ_Switching
স্থির
IM3Tag Module_FindExportedTag (IM3Module i_module, cstr_t i_name)
{
    ধ্রুবক M3Export* e = Module_FindExport(i_module, d_externalKind_tag, i_name);

    যদি (not e) {
        ফেরত ফাঁকা;
    }

    IM3Tag tag = &i_module->tags[e->index];

    ফেরত tag->resolved ? tag->resolved : tag;
}
#endif


// Whether the module exports anything at all under this name. Export names are
// unique within a module, so a name one of the lookups above missed but this
// one finds is exported as something else - a kind the import cannot be
// satisfied by, rather than a name the module never exported.
স্থির
bool Module_HasExport (IM3Module i_module, cstr_t i_name)
{
    জন্য (u32 i = 0; i < i_module->অঙ্ক_Exports; ++i) {
        যদি (strcmp(i_module->exports[i].name, i_name) == 0) {
            ফেরত true;
        }
    }

    ফেরত false;
}


// Points each of the module's imports at whatever already-loaded module exports
// it. Runs before anything is allocated or initialized: a memory import has to
// be resolved before InitMemory would give it pages of its own, and a global
// import before InitGlobals runs an initializer that reads it.
স্থির
M3_ফলাফল LinkImports (IM3Runtime io_runtime, IM3Module io_module)
{
    M3_ফলাফল ফলাফল = m3Err_none;

    জন্য (u32 i = 0; i < io_module->অঙ্ক_Functions; ++i) {
        IM3Function f = &io_module->functions[i];

        যদি (f->wasm or not (f->import.moduleUtf8 and f->import.fieldUtf8)) {
            চালু;
        }

        IM3Module from = m3_FindModule(io_runtime, f->import.moduleUtf8);
        যদি (not from) {
            চালু;
        }

        IM3Function exported = Module_FindExportedFunction(from, f->import.fieldUtf8);
        যদি (not exported) {
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

    জন্য (u32 i = 0; i < io_module->অঙ্ক_Memories; ++i) {
        IM3Memory memory = io_module->memories[i];

        যদি (not memory->imported or memory->owner != io_module) {
            চালু;
        }

        IM3Module from = m3_FindModule(io_runtime, memory->import.moduleUtf8);
        যদি (not from) {
            চালু;
        }

        IM3Memory exported = Module_FindExportedMemory(from, memory->import.fieldUtf8);
        যদি (not exported) {
            _throwif(m3Err_incompatibleImportType, Module_HasExport(from, memory->import.fieldUtf8));
            _throw(m3Err_unknownImport);
        }

        // the address type is part of the memory type, so an i64 memory does
        // not satisfy an i32 import, or the other way round - and so is the page
        // size, which custom page sizes made a declared property of a memory
        _throwif(m3Err_incompatibleImportType, exported->isMemory64 != memory->isMemory64);
        _throwif(m3Err_incompatibleImportType, Memory_Page_আকার(exported) != Memory_Page_আকার(memory));

        // a shared memory is not a subtype of an unshared one, or the other way round
        _throwif(m3Err_incompatibleImportType, exported->isShared != memory->isShared);

        _throwif(m3Err_incompatibleImportType,
                 not LimitsSatisfy(exported->অঙ্ক_Pages, exported->hasMax, exported->maxPages,
                                   memory->initPages, memory->hasMax, memory->maxPages));

        // hand the slot over to the exporter's memory, and drop the placeholder
        FreeMemoryBlock(memory);
        FreeImportInfo(&memory->import);
        m3_Free(memory);

        io_module->memories[i] = exported;
    }

    জন্য (u32 i = 0; i < io_module->অঙ্ক_Tables; ++i) {
        IM3Table table = io_module->tables[i];

        যদি (not table->imported or table->owner != io_module) {
            চালু;
        }

        IM3Module from = m3_FindModule(io_runtime, table->import.moduleUtf8);
        যদি (not from) {
            চালু;
        }

        IM3Table exported = Module_FindExportedTable(from, table->import.fieldUtf8);
        যদি (not exported) {
            _throwif(m3Err_incompatibleImportType, Module_HasExport(from, table->import.fieldUtf8));
            _throw(m3Err_unknownImport);
        }

        _throwif(m3Err_incompatibleImportType, exported->type != table->type);

        // the index type is part of the table type, the same way it is for a memory
        _throwif(m3Err_incompatibleImportType, exported->isTable64 != table->isTable64);

        _throwif(m3Err_incompatibleImportType,
                 not LimitsSatisfy(exported->আকার_2, exported->hasMax, exported->max_আকার,
                                   table->init_আকার, table->hasMax, table->max_আকার));

        // hand the slot over to the exporter's table, and drop the placeholder
        m3_Free(table->elements);
        FreeImportInfo(&table->import);
        m3_Free(table);

        io_module->tables[i] = exported;
    }

    জন্য (u32 i = 0; i < io_module->অঙ্ক_Globals; ++i) {
        IM3Global g = &io_module->globals[i];

        যদি (not g->imported or not (g->import.moduleUtf8 and g->import.fieldUtf8)) {
            চালু;
        }

        IM3Module from = m3_FindModule(io_runtime, g->import.moduleUtf8);
        যদি (not from) {
            চালু;
        }

        IM3Global exported = Module_FindExportedGlobal(from, g->import.fieldUtf8);
        যদি (not exported) {
            _throwif(m3Err_incompatibleImportType, Module_HasExport(from, g->import.fieldUtf8));
            _throw(m3Err_unknownImport);
        }

        _throwif(m3Err_incompatibleImportType, exported->type != g->type);
        _throwif(m3Err_incompatibleImportType, exported->isMutable != g->isMutable);

        g->resolved = exported->resolved ? exported->resolved : exported;
    }

#যদি d_m3HasExceptionHandling || d_m3_Has_স্তূপ_Switching
    জন্য (u32 i = 0; i < io_module->অঙ্ক_Tags; ++i) {
        IM3Tag tag = &io_module->tags[i];

        যদি (not tag->imported or not (tag->import.moduleUtf8 and tag->import.fieldUtf8)) {
            চালু;
        }

        IM3Module from = m3_FindModule(io_runtime, tag->import.moduleUtf8);
        যদি (not from) {
            চালু;
        }

        IM3Tag exported = Module_FindExportedTag(from, tag->import.fieldUtf8);
        যদি (not exported) {
            _throwif(m3Err_incompatibleImportType, Module_HasExport(from, tag->import.fieldUtf8));
            _throw(m3Err_unknownImport);
        }

        _throwif(m3Err_incompatibleImportType, exported->type != tag->type);

        tag->resolved = exported->resolved ? exported->resolved : exported;
    }
#endif

    _catch: ফেরত ফলাফল;
}


// Backs each of the module's memories with pages. LinkImports has already
// pointed any import it could satisfy at the exporting module's memory, and
// those are skipped here. An import nothing satisfied is still backed locally
// from its own declared limits, so that the module remains loadable.
M3_ফলাফল InitMemory (IM3Runtime io_runtime, IM3Module i_module)
{
    M3_ফলাফল ফলাফল = m3Err_none;

    // Fixed from here on: the index space stops changing after parse, and
    // linking has already repointed any slot it was going to.
    i_module->memory0 = i_module->অঙ্ক_Memories ? i_module->memories[0]
                                              : &i_module->emptyMemory;

    যদি (i_module->অঙ্ক_Memories == 0) {
        // nothing addressable, but _mem still has to point somewhere
        i_module->emptyMemory.owner    = i_module;
        i_module->emptyMemory.page_আকার = d_m3_Default_Mem_Page_আকার;

_       (ResizeMemory(io_runtime, &i_module->emptyMemory, 0));
    }

    জন্য (u32 i = 0; i < i_module->অঙ্ক_Memories; ++i) {
        IM3Memory memory = i_module->memories[i];

        // a slot that already points at another module's memory is that
        // module's to allocate
        যদি (memory->owner != i_module or memory->mallocated) {
            চালু;
        }

        u32 page_আকার = Memory_Page_আকার(memory);

        memory->page_আকার = page_আকার;

        // Without a declared maximum a memory may grow to the spec limit of
        // 2^|addrtype|/pagesize pages, which is the usual 65536 at the default
        // page size, 2^48 for a 64-bit memory, and a whole address space of
        // them when a page is a single byte. A declared maximum of zero is a
        // real limit, not the absence of one.
        //
        // 2^64/pagesize overflows a u64 only when a page is a single byte, and
        // a power-of-two page size divides the address space exactly, so the
        // 64-bit division below is 2^64/pagesize written so that it fits.
        যদি (not memory->hasMax) {
            memory->maxPages = memory->isMemory64
                                 ? (page_আকার > 1 ? (UINT64_MAX / page_আকার) + 1 : UINT64_MAX)
                                 : (0x100000000ull / page_আকার);
        }

_       (ResizeMemory(io_runtime, memory, memory->initPages));
    }

    _catch: ফেরত ফলাফল;
}


M3_ফলাফল ResizeMemory (IM3Runtime io_runtime, IM3Memory memory, u64 i_অঙ্ক_Pages)
{
    M3_ফলাফল ফলাফল = m3Err_none;

    u64 অঙ্ক_Pages_To_Alloc = i_অঙ্ক_Pages;

    যদি (অঙ্ক_Pages_To_Alloc <= memory->maxPages) {
        // A 64-bit memory may ask for up to 2^48 pages, which overflows a u64
        // of bytes. Nothing that large can be backed, so refuse it up front
        // rather than multiplying into a wrapped size.
        _throwif("linear memory limitation exceeded",
                 অঙ্ক_Pages_To_Alloc > d_m3AddressLimit / memory->page_আকার);

        u64 অঙ্ক_Page_Bytes = অঙ্ক_Pages_To_Alloc * memory->page_আকার;

#যদি d_m3MaxLinearMemoryPages > 0
        // the limit is a memory size, counted in default-sized pages; comparing
        // it against a raw page count would make it 65536 times stricter for a
        // module whose pages are one byte
        _throwif("linear memory limitation exceeded",
                 অঙ্ক_Page_Bytes > (u64)d_m3MaxLinearMemoryPages * d_m3_Default_Mem_Page_আকার);
#endif

        // Limit the amount of memory that gets actually allocated
        যদি (io_runtime->memoryLimit) {
            অঙ্ক_Page_Bytes = M3_MIN(অঙ্ক_Page_Bytes, (u64)io_runtime->memoryLimit);
        }

        u64 old_দৈর্ঘ্য = memory->mallocated ? memory->mallocated->দৈর্ঘ্য : 0;
        u64 otherUsed = io_runtime->memoryBytesUsed - old_দৈর্ঘ্য;
        _throwif(m3Err_memoryLimitExceeded, io_runtime->memoryBytesLimit and
                                              (অঙ্ক_Page_Bytes > io_runtime->memoryBytesLimit or
                                               otherUsed > io_runtime->memoryBytesLimit - অঙ্ক_Page_Bytes));

        _throwif("linear memory limitation exceeded", অঙ্ক_Page_Bytes > (u64)SIZE_MAX - আকার(M3MemoryHeader));

#যদি d_m3GuardedMemory

        // A slot holds every address a Wasm access can name, and nothing beyond
        // that: a memory larger than the data part of one would put addresses past
        // the guard, where a fault is nobody's to recognize
        _throwif("linear memory limitation exceeded", অঙ্ক_Page_Bytes > d_m3_Guarded_তথ্য_Bytes);

        // What makes an access past the end fault is the page after the memory not
        // being committed, so the end has to fall on one of the system's page
        // boundaries. A Wasm page divides evenly into them and always will; a memory
        // that declared a page size of its own may not, and then the bytes between
        // the end of the memory and the end of its last system page would answer
        // instead of trapping. Refusing to back such a memory is the honest way out:
        // the custom page sizes proposal exists for targets far too small to reserve
        // 8GiB per memory in the first place. See d_m3GuardedMemory.
        _throwif("guarded memory needs a page size the system's divides into",
                 (memory->page_আকার % m3_Host_Page_আকার()) != 0);

        যদি (memory->guardSlot == ফাঁকা) {
            memory->guardSlot = Guard_TakeSlot();
            _throwifnull(memory->guardSlot);
        }

        // Growing only ever commits more of the same slot, so the memory never
        // moves - which the interpreter does not rely on, but nothing minds
        _throwif(m3Err_mallocFailed, not Guard_CommitSlot(memory->guardSlot, (size_t)অঙ্ক_Page_Bytes));

        memory->mallocated = Guard_SlotHeader(memory->guardSlot);

#অন্যথা

        size_t অঙ্ক_Bytes = (size_t)অঙ্ক_Page_Bytes + আকার(M3MemoryHeader);

        // What is really there, which is not numPages * pageSize: the limit above
        // caps the bytes without capping the page count, so a memory can be short
        // of a page - by design, since a whole 64 KiB page is often more than an
        // MCU has. Measuring the old block by the page count would overstate it,
        // and then realloc never sees the size it already has and copies the
        // whole limit again on every grow past it.
        size_t অঙ্ক_Previous_Bytes = memory->mallocated ? memory->mallocated->দৈর্ঘ্য + আকার(M3MemoryHeader) : 0;

        শূন্য* newMem = m3_Realloc("Wasm Linear Memory", memory->mallocated, অঙ্ক_Bytes, অঙ্ক_Previous_Bytes);
        _throwifnull(newMem);

        memory->mallocated = (M3MemoryHeader*)newMem;

#endif

#যদি d_m3LogRuntime
        M3MemoryHeader* oldMallocated = memory->mallocated;
#endif

        io_runtime->memoryBytesUsed = otherUsed + অঙ্ক_Page_Bytes;
        memory->অঙ্ক_Pages            = অঙ্ক_Pages_To_Alloc;

        memory->mallocated->দৈর্ঘ্য  = (size_t)অঙ্ক_Page_Bytes;
        memory->mallocated->runtime = io_runtime;
        memory->mallocated->memory  = memory;

        memory->mallocated->max_স্তূপ = (m3slot_t*)io_runtime->origin_স্তূপ + io_runtime->অঙ্ক_স্তূপ_Slots;

        m3log(runtime, "resized old: %p; mem: %p; length: %zu; pages: %llu", oldMallocated, memory->mallocated, memory->mallocated->দৈর্ঘ্য, (unsigned long long)memory->অঙ্ক_Pages);
    } অন্যথা {
        ফলাফল = m3Err_wasmMemoryOverflow;
    }

    _catch: ফেরত ফলাফল;
}


শূন্য FreeMemoryBlock (IM3Memory io_memory)
{
    যদি (io_memory->mallocated and io_memory->mallocated->runtime) {
        io_memory->mallocated->runtime->memoryBytesUsed -= io_memory->mallocated->দৈর্ঘ্য;
    }
#যদি d_m3GuardedMemory
    Guard_GiveSlot(io_memory->guardSlot);
    io_memory->guardSlot = ফাঁকা;

    io_memory->mallocated = ফাঁকা;
#অন্যথা
    m3_Free(io_memory->mallocated);
#endif
}


M3_ফলাফল InitGlobals (IM3Module io_module)
{
    M3_ফলাফল ফলাফল = m3Err_none;

    যদি (io_module->অঙ্ক_Globals) {
        // placing the globals in their structs isn't good for cache locality, but i don't really know what the global
        // access patterns typically look like yet.

        //          io_module->globalMemory = m3Alloc (m3reg_t, io_module->numGlobals);

        //          if (io_module->globalMemory)
        {
            জন্য (u32 i = 0; i < io_module->অঙ্ক_Globals; ++i) {
                M3Global* g = &io_module->globals[i];                           m3log (runtime, "initializing global: %d", i);

                যদি (g->init_রাশিমালা) {
                    bytes_t start = g->init_রাশিমালা;

                    ফলাফল = EvaluateExpression(io_module, &g->i64_মান, g->type, &start, g->init_রাশিমালা + g->init_রাশিমালা_আকার);

                    যদি (not ফলাফল) {
                        // io_module->globalMemory [i] = initValue;
                    } অন্যথা {
                        বিরতি;
                    }
                } অন্যথা {                                                        m3log (runtime, "importing global");
                }
            }
        }
        //          else result = ErrorModule (m3Err_mallocFailed, io_module, "could allocate globals for module: '%s", io_module->name);
    }

    ফেরত ফলাফল;
}


M3_ফলাফল Init_তথ্য_Segments (IM3Module io_module)
{
    M3_ফলাফল ফলাফল = m3Err_none;

    জন্য (u32 i = 0; i < io_module->অঙ্ক_তথ্য_Segments; ++i) {
        M3_তথ্য_Segment* segment = &io_module->তথ্য_Segments[i];

        // A passive segment stays available for memory.init until data.drop.
        // An active one is copied here and then counts as dropped.
        যদি (segment->isPassive) {
            চালু;
        }

        _throwif("data segment memory index out of range",
                 segment->memoryRegion >= io_module->অঙ্ক_Memories);

        IM3Memory io_memory = io_module->memories[segment->memoryRegion];

        _throwif("unallocated linear memory", !(io_memory->mallocated));

        // The offset expression has the memory's address type, and is
        // unsigned: an i32 offset of -1 is 4294967295, way out of bounds
        // rather than negative.
        u64     segmentOffset = 0;
        bytes_t start         = segment->init_রাশিমালা;

        যদি (io_memory->isMemory64) {
_           (EvaluateExpression(io_module, &segmentOffset, c_m3Type_i64, &start, segment->init_রাশিমালা + segment->init_রাশিমালা_আকার));
        } অন্যথা {
            u32 offset32;
_           (EvaluateExpression(io_module, &offset32, c_m3Type_i32, &start, segment->init_রাশিমালা + segment->init_রাশিমালা_আকার));
            segmentOffset = offset32;
        }

        m3log(runtime, "loading data segment: %d; size: %d; offset: %llu", i, segment->আকার_2, (unsigned long long)segmentOffset);

        যদি (segmentOffset <= io_memory->mallocated->দৈর্ঘ্য &&
            (u64)segment->আকার_2 <= io_memory->mallocated->দৈর্ঘ্য - segmentOffset) {
            u8* dest = m3_Mem_তথ্য(io_memory->mallocated) + segmentOffset;
            memcpy(dest, segment->তথ্য, segment->আকার_2);
        } অন্যথা {
            _throw("data segment out of bounds");
        }

        segment->dropped = true;
    }

    _catch: ফেরত ফলাফল;
}


// Turns a segment's elements into references. Element expressions are constant
// expressions restricted to ref.null/ref.func, so they're read directly rather
// than run through the compiler.
স্থির
M3_ফলাফল ResolveElements (IM3Module io_module, M3ElementSegment* i_segment, শূন্য** o_elements)
{
    M3_ফলাফল ফলাফল = m3Err_none;

    bytes_t  pos = i_segment->elements;
    cbytes_t end = io_module->elementSectionEnd;

    জন্য (u32 e = 0; e < i_segment->অঙ্ক_Elements; ++e) {
        u32   func_সূচক;
        শূন্য* ref = ফাঁকা;

        যদি (i_segment->is_রাশিমালা) {
            m3opcode_t opcode;
_           (Read_opcode(&opcode, &pos, end));

            যদি (opcode == c_waOp_refFunc) {
_               (ReadLEB_u32(&func_সূচক, &pos, end));
                _throwif("function index out of range", func_সূচক >= io_module->অঙ্ক_Functions);
                ref = Function_Implementation(&io_module->functions[func_সূচক]);
            } অন্যথা যদি (opcode == c_waOp_refNull) {
                i8 waType;
                u8 nullType;
_               (ReadLEB_i7(&waType, &pos, end));
_               (NormalizeType(&nullType, waType));
                _throwif(m3Err_typeMismatch, nullType != i_segment->type);
            } অন্যথা যদি (opcode == c_waOp_getGlobal) {
                // wasm 2.0 lets an element expression read an imported
                // immutable global, which is how one module seeds another's
                // table with a reference it exported.
                u32 global_সূচক;
_               (ReadLEB_u32(&global_সূচক, &pos, end));
                _throwif(m3_Err_globa_সূচক_Out_Of_Bounds, global_সূচক >= io_module->অঙ্ক_Globals);

                IM3Global global = &io_module->globals[global_সূচক];

                _throwif(m3_Err_globa_সূচক_Out_Of_Bounds, not global->imported);
                _throwif(m3Err_wasmMalformed, global->isMutable);
                _throwif(m3Err_typeMismatch, BaseTypeOf(global->type) != BaseTypeOf(i_segment->type));

                // read the cell the import was linked to, not the placeholder
                যদি (global->resolved) {
                    global = global->resolved;
                }

                ref = global->ref_মান;
            } অন্যথা {
                _throw("constant expression required");
            }

_           (Read_opcode(&opcode, &pos, end));
            _throwif(m3Err_wasmMalformed, opcode != c_waOp_end);
        } অন্যথা {
_           (ReadLEB_u32(&func_সূচক, &pos, end));
            _throwif("function index out of range", func_সূচক >= io_module->অঙ্ক_Functions);
            ref = Function_Implementation(&io_module->functions[func_সূচক]);
        }

        o_elements[e] = ref;
    }

    _catch: ফেরত ফলাফল;
}


M3_ফলাফল InitTableAndElements (IM3Module io_module)
{
    M3_ফলাফল ফলাফল = m3Err_none;

    cbytes_t end = io_module->elementSectionEnd;
    M3Table* table;

    জন্য (u32 i = 0; i < io_module->অঙ্ক_Tables; ++i) {
        table = io_module->tables[i];

        // a slot pointing at another module's table is that module's to fill
        যদি (table->owner != io_module) {
            চালু;
        }

        যদি (table->আকার_2) {
            IM3Runtime runtime = io_module->runtime;
            _throwif(m3Err_tableLimitExceeded, runtime->tableElementsLimit and
                                                 table->আকার_2 > runtime->tableElementsLimit - runtime->tableElementsUsed);
            table->elements = m3_AllocArray(শূন্য*, table->আকার_2);
            _throwifnull(table->elements);
            runtime->tableElementsUsed += table->আকার_2;

            যদি (table->init_রাশিমালা) {
                শূন্য*   মান = ফাঁকা;
                bytes_t start = table->init_রাশিমালা;
_               (EvaluateExpression(io_module, &মান, BaseTypeOf(table->type),
                                     &start, table->init_রাশিমালা + table->init_রাশিমালা_আকার));

                জন্য (u32 e = 0; e < table->আকার_2; ++e) {
                    table->elements[e] = মান;
                }
            }
        }
    }

    জন্য (u32 i = 0; i < io_module->অঙ্ক_Element_Segments; ++i) {
        M3ElementSegment* segment = &io_module->elementSegments[i];

        // Declarative segments only make their functions referenceable, and
        // passive ones wait for table.init, so neither is written out here.
        যদি (segment->mode == c_m3Elem_declarative) {
            segment->dropped = true;
            চালু;
        }

        যদি (segment->mode == c_m3Elem_passive) {
            যদি (segment->অঙ্ক_Elements) {
                segment->resolved = m3_AllocArray(শূন্য*, segment->অঙ্ক_Elements);
                _throwifnull(segment->resolved);
_               (ResolveElements(io_module, segment, segment->resolved));
            }
            চালু;
        }

        table = io_module->tables[segment->table_সূচক];

        // The offset expression has the table's index type, and is unsigned:
        // an i32 offset of -1 is 4294967295, out of bounds rather than negative.
        u64     offset = 0;
        bytes_t রাশিমালা   = segment->init_রাশিমালা;

        যদি (table->isTable64) {
_           (EvaluateExpression(io_module, &offset, c_m3Type_i64, &রাশিমালা, end));
        } অন্যথা {
            u32 offset32;
_           (EvaluateExpression(io_module, &offset32, c_m3Type_i32, &রাশিমালা, end));
            offset = offset32;
        }

        _throwif("out of bounds table access",
                 offset > table->আকার_2 or segment->অঙ্ক_Elements > table->আকার_2 - offset);

_       (ResolveElements(io_module, segment, table->elements + offset));

        segment->dropped = true;
    }

    _catch: ফেরত ফলাফল;
}

M3_ফলাফল m3_CompileModule (IM3Module io_module)
{
    M3_ফলাফল ফলাফল = m3Err_none;

    জন্য (u32 i = 0; i < io_module->অঙ্ক_Functions; ++i) {
        IM3Function f = &io_module->functions[i];
        যদি (f->wasm and not f->compiled) {
_           (CompileFunction(f));
        }
    }

    _catch: ফেরত ফলাফল;
}

#যদি d_m3HasExceptionHandling

M3Exception* NewException (IM3Runtime io_runtime, IM3Tag i_tag, u32 i_অঙ্ক_Args)
{
    M3Exception* exception = (M3Exception*)m3_Malloc("M3Exception", আকার(M3Exception) + i_অঙ্ক_Args * আকার(u64));

    যদি (exception) {
        exception->tag     = i_tag;
        exception->অঙ্ক_Args = i_অঙ্ক_Args;
        exception->reified = false;
        exception->পূর্ব    = ফাঁকা;
        exception->পরবর্তী    = io_runtime->exceptions;

        যদি (exception->পরবর্তী) {
            exception->পরবর্তী->পূর্ব = exception;
        }

        io_runtime->exceptions = exception;
    }

    ফেরত exception;
}


// Releases one exception ahead of the rest. The caller has to know nothing can
// still name it: no exnref was ever taken of it, and its payload has already
// been copied out.
শূন্য FreeException (IM3Runtime io_runtime, M3Exception* i_exception)
{
    যদি (i_exception->পূর্ব) {
        i_exception->পূর্ব->পরবর্তী = i_exception->পরবর্তী;
    } অন্যথা {
        io_runtime->exceptions = i_exception->পরবর্তী;
    }

    যদি (i_exception->পরবর্তী) {
        i_exception->পরবর্তী->পূর্ব = i_exception->পূর্ব;
    }

    যদি (io_runtime->pendingException == i_exception) {
        io_runtime->pendingException = ফাঁকা;
    }

    m3_Free_Impl(i_exception);
}


// Releases every exception the runtime still holds. Only safe once the Wasm
// stack is empty, which is why the outermost RunCodeChecked() is the one that
// calls it.
শূন্য FreeExceptions (IM3Runtime io_runtime)
{
    M3Exception* exception = io_runtime->exceptions;

    io_runtime->exceptions       = ফাঁকা;
    io_runtime->pendingException = ফাঁকা;

    যখন (exception) {
        M3Exception* পরবর্তী = exception->পরবর্তী;
        m3_Free_Impl(exception);
        exception = পরবর্তী;
    }
}


// Keeps a copy of the exception a call just ended with, for m3_GetExceptionTag,
// or forgets the last one when the call ended any other way. A copy, because
// the original is freed with the rest before the outermost call returns - and
// one a host function's nested call ended with may still be named by an
// exnref further down the stack.
স্থির
শূন্য RecordUncaughtException (IM3Runtime io_runtime, M3_ফলাফল i_ফলাফল)
{
    যদি (io_runtime->uncaughtException) {
        m3_Free(io_runtime->uncaughtException);
    }

    M3Exception* exception = io_runtime->pendingException;

    যদি (M3_UNLIKELY(i_ফলাফল == m3Err_trapUncaughtException) and exception) {
        M3Exception* copy = (M3Exception*)m3_CopyMem(exception, আকার(M3Exception) + exception->অঙ্ক_Args * আকার(u64));

        যদি (copy) {
            copy->পরবর্তী = ফাঁকা;
            copy->পূর্ব = ফাঁকা;
        }

        io_runtime->uncaughtException = copy;
    }
}

#endif // d_m3HasExceptionHandling


#যদি d_m3HasExceptionHandling || d_m3_Has_স্তূপ_Switching

IM3Tag m3_FindTag (IM3Module i_module, ধ্রুবক অক্ষর* ধ্রুবক i_tagName)
{
    যদি (not i_module or not i_tagName) {
        ফেরত ফাঁকা;
    }

    ফেরত Module_FindExportedTag(i_module, i_tagName);
}


M3_ফলাফল m3_NewTag (IM3Runtime io_runtime, IM3Tag* o_tag, ধ্রুবক অক্ষর* ধ্রুবক i_signature)
{
    IM3FuncType type    = ফাঁকা;
    M3HostTag*  hostTag = ফাঁকা;

_try {
    _throwif("null tag", not o_tag);
    *o_tag = ফাঁকা;
    _throwif(m3Err_mallocFailed, not io_runtime);

_   (SignatureToFuncType(&type, i_signature));
    // tags are the same type exactly when their types are the same object
_   (Environment_AddFuncType(io_runtime->environment, &type));

    hostTag = m3_AllocStruct(M3HostTag);
    _throwifnull(hostTag);

    hostTag->tag.type    = type;
    hostTag->পরবর্তী        = io_runtime->hostTags;
    io_runtime->hostTags = hostTag;

    *o_tag = &hostTag->tag;
} _catch:
    ফেরত ফলাফল;
}


M3_ফলাফল m3_LinkTag (IM3Module         io_module,
                     ধ্রুবক অক্ষর* ধ্রুবক i_moduleName,
                     ধ্রুবক অক্ষর* ধ্রুবক i_tagName,
                     IM3Tag            i_tag)
{
    যদি (not io_module or not i_moduleName or not i_tagName or not i_tag) {
        ফেরত m3Err_tagLookupFailed;
    }

    ধ্রুবক bool wildcardModule = (strcmp(i_moduleName, "*") == 0);
    IM3Tag     target         = i_tag->resolved ? i_tag->resolved : i_tag;

    M3_ফলাফল ফলাফল = m3Err_tagLookupFailed;

    জন্য (u32 i = 0; i < io_module->অঙ্ক_Tags; ++i) {
        IM3Tag tag = &io_module->tags[i];

        যদি (tag->imported and tag->import.moduleUtf8 and tag->import.fieldUtf8 and
            strcmp(tag->import.fieldUtf8, i_tagName) == 0 and
            (wildcardModule or strcmp(tag->import.moduleUtf8, i_moduleName) == 0)) {
            যদি (tag->type != target->type) {
                ফেরত m3Err_incompatibleImportType;
            }

            tag->resolved = target;
            ফলাফল        = m3Err_none;
        }
    }

    ফেরত ফলাফল;
}


uint32_t m3_Get_Tag_Arg_গুনতি (IM3Tag i_tag)
{
    ফেরত i_tag ? i_tag->type->অঙ্ক_Args : 0;
}


M3_মান_Type m3_GetTagArgType (IM3Tag i_tag, uint32_t i_সূচক)
{
    যদি (i_tag and i_সূচক < i_tag->type->অঙ্ক_Args) {
        ফেরত (M3_মান_Type)BaseTypeOf(d_FuncArgType(i_tag->type, i_সূচক));
    }

    ফেরত c_m3Type_none;
}

#অন্যথা

IM3Tag m3_FindTag (IM3Module i_module, ধ্রুবক অক্ষর* ধ্রুবক i_tagName)
{
    ফেরত ফাঁকা;
}

M3_ফলাফল m3_NewTag (IM3Runtime io_runtime, IM3Tag* o_tag, ধ্রুবক অক্ষর* ধ্রুবক i_signature)
{
    যদি (o_tag) {
        *o_tag = ফাঁকা;
    }
    ফেরত m3Err_tagLookupFailed;
}

M3_ফলাফল m3_LinkTag (IM3Module io_module, ধ্রুবক অক্ষর* ধ্রুবক i_moduleName, ধ্রুবক অক্ষর* ধ্রুবক i_tagName, IM3Tag i_tag)
{
    ফেরত m3Err_tagLookupFailed;
}

uint32_t m3_Get_Tag_Arg_গুনতি (IM3Tag i_tag)
{
    ফেরত 0;
}

M3_মান_Type m3_GetTagArgType (IM3Tag i_tag, uint32_t i_সূচক)
{
    ফেরত c_m3Type_none;
}

#endif // d_m3HasExceptionHandling || d_m3HasStackSwitching


#যদি d_m3HasExceptionHandling

M3_ফলাফল m3_ThrowException (IM3Runtime io_runtime, IM3Tag i_tag, uint32_t i_argc, ধ্রুবক শূন্য* i_argptrs[])
{
    যদি (not io_runtime or not i_tag) {
        ফেরত m3Err_tagLookupFailed;
    }

    IM3Tag      tag  = i_tag->resolved ? i_tag->resolved : i_tag;
    IM3FuncType type = tag->type;

    // a tag with results is a stack switching control tag, which is suspended
    // with rather than thrown
    যদি (type->অঙ্ক_Rets) {
        ফেরত m3Err_typeMismatch;
    }
    যদি (i_argc != type->অঙ্ক_Args) {
        ফেরত m3_Err_argument_গুনতি_Mismatch;
    }
    // with nothing on the stack there is nothing to unwind, and the exception
    // would sit on the runtime's list until some later call ended
    যদি (io_runtime->callNesting == 0) {
        ফেরত "m3_ThrowException called outside a host function";
    }

    M3Exception* exception = NewException(io_runtime, tag, i_argc);
    যদি (not exception) {
        ফেরত m3Err_mallocFailed;
    }

    // the layout op_Throw gives a payload: 32-bit values zero-extended, floats
    // as their bit pattern
    জন্য (u32 i = 0; i < i_argc; ++i) {
        ধ্রুবক শূন্য* মান = i_argptrs[i];

        নির্বাচন (BaseTypeOf(d_FuncArgType(type, i))) {
        ক্ষেত্র c_m3Type_i32:
        ক্ষেত্র c_m3Type_f32: {
            u32 bits;
            memcpy(&bits, মান, আকার(bits));
            exception->args[i] = bits;
            বিরতি;
        }
        ক্ষেত্র c_m3Type_i64:
        ক্ষেত্র c_m3Type_f64:
            memcpy(&exception->args[i], মান, আকার(u64));
            বিরতি;
        ক্ষেত্র c_m3Type_funcref:
        ক্ষেত্র c_m3Type_externref:
        ক্ষেত্র c_m3Type_exnref:
        ক্ষেত্র c_m3Type_contref:
            exception->args[i] = (u64)(*(ধ্রুবক uintptr_t*)মান);
            বিরতি;
        পূর্বনির্ধারিত:
            FreeException(io_runtime, exception);
            ফেরত "unknown argument type";
        }
    }

    io_runtime->pendingException = exception;

    ফেরত m3Err_pendingException;
}


IM3Tag m3_GetExceptionTag (IM3Runtime i_runtime)
{
    যদি (i_runtime and i_runtime->uncaughtException) {
        ফেরত i_runtime->uncaughtException->tag;
    }

    ফেরত ফাঁকা;
}


M3_ফলাফল m3_GetExceptionArgs (IM3Runtime i_runtime, uint32_t i_argc, ধ্রুবক শূন্য* o_argptrs[])
{
    যদি (not i_runtime or not i_runtime->uncaughtException) {
        ফেরত "no uncaught exception";
    }

    ধ্রুবক M3Exception* exception = i_runtime->uncaughtException;
    IM3FuncType        type      = exception->tag->type;

    যদি (i_argc != exception->অঙ্ক_Args) {
        ফেরত m3_Err_argument_গুনতি_Mismatch;
    }

    জন্য (u32 i = 0; i < i_argc; ++i) {
        শূন্য* মান = (শূন্য*)o_argptrs[i];
        u64   bits  = exception->args[i];

        নির্বাচন (BaseTypeOf(d_FuncArgType(type, i))) {
        ক্ষেত্র c_m3Type_i32:
        ক্ষেত্র c_m3Type_f32: {
            u32 নিচের = (u32)bits;
            memcpy(মান, &নিচের, আকার(নিচের));
            বিরতি;
        }
        ক্ষেত্র c_m3Type_i64:
        ক্ষেত্র c_m3Type_f64:
            memcpy(মান, &bits, আকার(bits));
            বিরতি;
        ক্ষেত্র c_m3Type_funcref:
        ক্ষেত্র c_m3Type_externref:
        ক্ষেত্র c_m3Type_exnref:
        ক্ষেত্র c_m3Type_contref:
            *(uintptr_t*)মান = (uintptr_t)bits;
            বিরতি;
        পূর্বনির্ধারিত:
            ফেরত "unknown argument type";
        }
    }

    ফেরত m3Err_none;
}

#অন্যথা

M3_ফলাফল m3_ThrowException (IM3Runtime io_runtime, IM3Tag i_tag, uint32_t i_argc, ধ্রুবক শূন্য* i_argptrs[])
{
    ফেরত m3Err_tagLookupFailed;
}

IM3Tag m3_GetExceptionTag (IM3Runtime i_runtime)
{
    ফেরত ফাঁকা;
}

M3_ফলাফল m3_GetExceptionArgs (IM3Runtime i_runtime, uint32_t i_argc, ধ্রুবক শূন্য* o_argptrs[])
{
    ফেরত "no uncaught exception";
}

#endif // d_m3HasExceptionHandling


#যদি d_m3GuardedMemory

// What RunCodeProtected hands to the platform layer to run, and what comes back
প্রকারলেখ কাঠামো M3ProtectedRun {
    pc_t            pc;
    IM3Runtime      runtime;
    M3MemoryHeader* mem;
    M3_ফলাফল        ফলাফল;
} M3ProtectedRun;

স্থির
শূন্য RunCodeBody (শূন্য* io_context)
{
    M3ProtectedRun* run = (M3ProtectedRun*)io_context;

#  যদি (d_m3EnableOpProfiling || d_m3EnableOpTracing)
    run->ফলাফল = (M3_ফলাফল)RunCode(run->pc, (m3stack_t)run->runtime->স্তূপ, run->mem,
                                    d_m3OpDefaultArgs, d_m3BaseCstr);
#  অন্যথা
    run->ফলাফল = (M3_ফলাফল)RunCode(run->pc, (m3stack_t)run->runtime->স্তূপ, run->mem,
                                    d_m3OpDefaultArgs);
#  endif
}

// The bounds check the load and store operations no longer do, done once here by the
// system instead: an address past the end of a memory reaches the uncommitted part
// of that memory's reservation, and the fault comes back as the trap the spec asks
// for. Nothing is left half-done by it - the Wasm stack and the memories are heap,
// and the caller unwinds the rest.
স্থির
M3_ফলাফল RunCodeProtected (IM3Runtime i_runtime, pc_t i_pc, M3MemoryHeader* i_mem)
{
    // Memory operations in this build carry no bounds check, so code only runs where a
    // fault past the end of a memory can still be turned into a trap
    যদি (M3_UNLIKELY(not m3_HostGuardsActive())) {
        ফেরত ErrorRuntime(m3Err_memoryGuardsUnavailable, i_runtime,
                            "another part of the process holds the fault signal");
    }

    M3ProtectedRun run;
    run.pc      = i_pc;
    run.runtime = i_runtime;
    run.mem     = i_mem;
    run.ফলাফল  = m3Err_none;

    শূন্য*  arena;
    size_t arenaBytes;
    Guard_ArenaRange(&arena, &arenaBytes);

    যদি (not m3_HostProtectedCall(RunCodeBody, &run, arena, arenaBytes)) {
        ফেরত m3Err_trapOutOfBoundsMemoryAccess;
    }

    ফেরত run.ফলাফল;
}

#endif // d_m3GuardedMemory


#যদি d_m3_Has_স্তূপ_Switching

// The function a suspended continuation stopped in: the callee of its innermost
// call frame, or the function it was entered with if it made no call. Frames are
// recorded innermost first, so that is the lowest-numbered call frame.
স্থির
IM3Function Continuation_InnermostFunction (IM3Continuation i_cont)
{
    জন্য (u32 i = 0; i < i_cont->অঙ্ক_Frames; ++i) {
        যদি (i_cont->frames[i].kind == frame_call && i_cont->frames[i].call.function) {
            ফেরত i_cont->frames[i].call.function;
        }
    }

    ফেরত i_cont->entryFunction;
}


// Where the host's next call puts its arguments and finds its results. A
// suspended root keeps its frames on the bottom of the runtime's stack, so a call
// made before it is resumed starts above the last of them rather than on top of
// them. The top is the innermost function's frame base plus everything that
// function can use. That base is the root's own sp, unless it stopped inside a
// resume: then the suspension point belongs to the resumed continuation, and the
// root's innermost frame is that resume.
শূন্য Runtime_Place_Call_স্তূপ (IM3Runtime io_runtime)
{
    IM3Continuation মূল = io_runtime->মূল_Continuation;

    io_runtime->স্তূপ = io_runtime->origin_স্তূপ;

    যদি (not মূল or মূল->state != cont_suspended) {
        ফেরত;
    }

    m3stack_t   sp       = মূল->sp;
    IM3Function function = Continuation_InnermostFunction(মূল);

    যদি (মূল->অঙ্ক_Frames and মূল->frames[0].kind == frame_resume) {
        sp = মূল->frames[0].sp;
    }

    m3slot_t* base = (m3slot_t*)io_runtime->origin_স্তূপ;
    m3slot_t* শীর্ষ  = (m3slot_t*)sp + (function ? function->max_স্তূপ_Slots : 0);

    যদি (শীর্ষ > base + io_runtime->অঙ্ক_স্তূপ_Slots) {
        শীর্ষ = base + io_runtime->অঙ্ক_স্তূপ_Slots;
    }

    io_runtime->স্তূপ = শীর্ষ;
}

#endif // d_m3HasStackSwitching


// Run compiled code on the runtime's stack, bounding native recursion for the
// duration of the call. The outermost invocation establishes the stack limit;
// nested ones (an imported function calling back into Wasm) inherit it.
স্থির অন্তঃরেখ
M3_ফলাফল RunCodeChecked (IM3Runtime i_runtime, IM3Function i_function)
{
    pc_t i_pc = i_function->compiled;

    // execution runs against the memory of the module the entry point belongs
    // to, not against some runtime-wide one
    M3MemoryHeader* _mem = Module_MemoryHeader(i_function->module);

#যদি d_m3HasSnapshots
    যদি (M3_UNLIKELY(i_function->module->isUnusable)) {
        ফেরত "a snapshot failed to restore into the module, so it cannot run";
    }
    i_function->module->hasRun = true;
#endif

    d_m3_স্তূপ_Limit_Enter(i_runtime);

    // a host function calling back into Wasm does not change which call a trap
    // ends: that is still the one the host made first
    যদি (i_runtime->callNesting++ == 0) {
        i_runtime->entered = i_function;
    }
#যদি d_m3HasExceptionHandling
    // handler stacks don't nest across a call boundary: a host function calling
    // back into Wasm cannot be caught by a try_table its own caller entered
    u32 savedTryDepth   = i_runtime->tryDepth;
    i_runtime->tryDepth = 0;
#endif
#যদি d_m3_Has_স্তূপ_Switching
    // nor do continuations: the native frames on the far side of a host call
    // are not ours to record, so a suspend in here has no handler to reach and
    // no resume to unwind to
    IM3Continuation savedContinuation = i_runtime->activeContinuation;

    // A suspended root continuation is a paused program waiting to be resumed,
    // not scratch space. A call the host makes before resuming it gets a
    // context of its own, the way any other re-entry does, rather than having
    // the frames of the suspension reset out from under it.
    bool claim_মূল = i_runtime->isSuspendable and not savedContinuation and
                     not (i_runtime->মূল_Continuation and
                          i_runtime->মূল_Continuation->state == cont_suspended);

    যদি (claim_মূল and not i_runtime->মূল_Continuation) {
        i_runtime->মূল_Continuation = Continuation_New(i_runtime, i_function->funcType, i_function);

        যদি (i_runtime->মূল_Continuation) {
            // The root borrows the runtime's stack and is not charged as cont.new.
            i_runtime->মূল_Continuation->অঙ্ক_স্তূপ_Slots = i_runtime->অঙ্ক_স্তূপ_Slots;
        } অন্যথা {
            claim_মূল = false;
        }
    }

    যদি (claim_মূল) {
        i_runtime->activeContinuation              = i_runtime->মূল_Continuation;
        i_runtime->মূল_Continuation->entryFunction = i_function;
        i_runtime->মূল_Continuation->state         = cont_running;
        i_runtime->মূল_Continuation->অঙ্ক_Frames     = 0;
    } অন্যথা {
        i_runtime->activeContinuation = ফাঁকা;
    }
#endif
#যদি d_m3GuardedMemory
    M3_ফলাফল ফলাফল = RunCodeProtected(i_runtime, i_pc, _mem);
#নতুবা_যদি (d_m3EnableOpProfiling || d_m3EnableOpTracing)
    M3_ফলাফল ফলাফল = (M3_ফলাফল)RunCode(i_pc, (m3stack_t)i_runtime->স্তূপ, _mem, d_m3OpDefaultArgs, d_m3BaseCstr);
#অন্যথা
    M3_ফলাফল ফলাফল = (M3_ফলাফল)RunCode(i_pc, (m3stack_t)i_runtime->স্তূপ, _mem, d_m3OpDefaultArgs);
#endif
#যদি d_m3_Has_স্তূপ_Switching
    যদি (claim_মূল) {
        i_runtime->মূল_Continuation->state =
          (ফলাফল == m3Err_continuationSuspended) ? cont_suspended : cont_returned;

        Runtime_Place_Call_স্তূপ(i_runtime);
    }
    i_runtime->activeContinuation = savedContinuation;
#endif
#যদি d_m3HasExceptionHandling
    i_runtime->tryDepth = savedTryDepth;

    // an exception that reached the bottom of the call stack found no handler
    যদি (M3_UNLIKELY(ফলাফল == m3Err_pendingException)) {
        ফলাফল = m3Err_trapUncaughtException;
    }
    RecordUncaughtException(i_runtime, ফলাফল);

    // a paused invocation can still reach the exceptions it holds, so they wait
    // for it to finish
    যদি (i_runtime->callNesting == 1 and not m3_IsSuspended(i_runtime)) {
        FreeExceptions(i_runtime);
    }
#endif
    i_runtime->callNesting--;
    d_m3_স্তূপ_Limit_Leave(i_runtime);

    ফেরত ফলাফল;
}

M3_ফলাফল m3_RunStart (IM3Module io_module)
{
#ifdef FUZZING_BUILD_MODE_UNSAFE_FOR_PRODUCTION
    // Execution disabled for fuzzing builds
    ফেরত m3Err_none;
#endif

    M3_ফলাফল ফলাফল           = m3Err_none;
    i32      startFunctionTmp = -1;

    যদি (io_module and io_module->startFunction >= 0) {
        IM3Function function = &io_module->functions[io_module->startFunction];

        যদি (not function->compiled) {
_           (CompileFunction(function));
        }

        IM3FuncType ftype = function->funcType;
        যদি (ftype->অঙ্ক_Args != 0 || ftype->অঙ্ক_Rets != 0) {
            _throw(m3_Err_argument_গুনতি_Mismatch);
        }

        IM3Module  module  = function->module;
        IM3Runtime runtime = module->runtime;

        startFunctionTmp = io_module->startFunction;

        io_module->startFunction = -1;

        ফলাফল = RunCodeChecked(runtime, function);

        // a start function that suspended is not to be started again: resuming
        // is what finishes it
        যদি (ফলাফল and ফলাফল != m3Err_continuationSuspended) {
            io_module->startFunction = startFunctionTmp;
            EXCEPTION_PRINT(ফলাফল);
            যাও _catch;
        }
    }

    _catch: ফেরত ফলাফল;
}

// TODO: deal with main + side-modules loading efforcement
M3_ফলাফল m3_LoadModule (IM3Runtime io_runtime, IM3Module io_module)
{
    M3_ফলাফল ফলাফল = m3Err_none;

    যদি (M3_UNLIKELY(io_module->runtime)) {
        ফেরত m3Err_moduleAlreadyLinked;
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
_   (Init_তথ্য_Segments(io_module));

    // Start func might use imported functions, which are not liked here yet,
    // so it will be called before a function call is attempted (in m3_FindFunction)

#ifdef DEBUG
    Module_GenerateNames(io_module);
#endif

    io_module->পরবর্তী     = io_runtime->modules;
    io_runtime->modules = io_module;
    ফেরত ফলাফল; // ok

_catch:
    // The runtime owns the module either way. Instantiation may already have
    // written this module's functions into a table another module owns, and the
    // spec keeps whatever it managed to do before the trap, so those entries
    // stay callable - retaining the module is what stops them dangling.
    //
    // It goes on the tail of the list rather than the head: it is not a module
    // anyone should find by name, only one whose functions may still be
    // reachable through someone else's table.
    io_module->পরবর্তী = ফাঁকা;

    IM3Module* লেজ = &io_runtime->modules;
    যখন (*লেজ) {
        লেজ = &(*লেজ)->পরবর্তী;
    }
    *লেজ = io_module;

    ফেরত ফলাফল;
}

IM3Global m3_FindGlobal (IM3Module         io_module,
                         ধ্রুবক অক্ষর* ধ্রুবক i_globalName)
{
    // Search exports
    IM3Global exported = Module_FindExportedGlobal(io_module, i_globalName);
    যদি (exported) {
        // a re-exported global is the one that was imported, so reads and
        // writes have to reach the cell that actually holds the value
        ফেরত exported->resolved ? exported->resolved : exported;
    }

    // Search imports
    জন্য (u32 i = 0; i < io_module->অঙ্ক_Globals; ++i) {
        IM3Global g = &io_module->globals[i];

        যদি (g->import.moduleUtf8 and g->import.fieldUtf8) {
            যদি (strcmp(g->import.fieldUtf8, i_globalName) == 0) {
                ফেরত g->resolved ? g->resolved : g;
            }
        }
    }
    ফেরত ফাঁকা;
}

M3_ফলাফল m3_GetGlobal (IM3Global      i_global,
                       IM3_Tagged_মান o_মান)
{
    যদি (not i_global) {
        ফেরত m3Err_globalLookupFailed;
    }

    নির্বাচন (i_global->type) {
    ক্ষেত্র c_m3Type_i32: o_মান->মান.i32 = i_global->i32_মান; বিরতি;
    ক্ষেত্র c_m3Type_i64: o_মান->মান.i64 = i_global->i64_মান; বিরতি;
#যদি d_m3HasFloat
    ক্ষেত্র c_m3Type_f32: o_মান->মান.f32 = i_global->f32_মান; বিরতি;
    ক্ষেত্র c_m3Type_f64: o_মান->মান.f64 = i_global->f64_মান; বিরতি;
#endif
    পূর্বনির্ধারিত: ফেরত m3Err_invalidTypeId;
    }

    o_মান->type = (M3_মান_Type)(i_global->type);
    ফেরত m3Err_none;
}

M3_ফলাফল m3_SetGlobal (IM3Global            i_global,
                       ধ্রুবক IM3_Tagged_মান i_মান)
{
    যদি (not i_global) {
        ফেরত m3Err_globalLookupFailed;
    }
    যদি (not i_global->isMutable) {
        ফেরত m3Err_globalNotMutable;
    }
    যদি (i_global->type != i_মান->type) {
        ফেরত m3Err_globalTypeMismatch;
    }

    নির্বাচন (i_মান->type) {
    ক্ষেত্র c_m3Type_i32: i_global->i32_মান = i_মান->মান.i32; বিরতি;
    ক্ষেত্র c_m3Type_i64: i_global->i64_মান = i_মান->মান.i64; বিরতি;
#যদি d_m3HasFloat
    ক্ষেত্র c_m3Type_f32: i_global->f32_মান = i_মান->মান.f32; বিরতি;
    ক্ষেত্র c_m3Type_f64: i_global->f64_মান = i_মান->মান.f64; বিরতি;
#endif
    পূর্বনির্ধারিত: ফেরত m3Err_invalidTypeId;
    }

    ফেরত m3Err_none;
}

M3_মান_Type m3_GetGlobalType (IM3Global i_global)
{
    ফেরত (i_global) ? (M3_মান_Type)(i_global->type) : c_m3Type_none;
}


শূন্য* v_FindFunction (IM3Module i_module, শূন্য* i_info)
{
    ধ্রুবক অক্ষর* ধ্রুবক i_name = (ধ্রুবক অক্ষর*)i_info;

    // Prefer exported functions
    IM3Function exported = Module_FindExportedFunction(i_module, i_name);
    যদি (exported) {
        ফেরত exported;
    }

    // Search internal functions
    জন্য (u32 i = 0; i < i_module->অঙ্ক_Functions; ++i) {
        IM3Function f = &i_module->functions[i];

        bool isImported = f->import.moduleUtf8 or f->import.fieldUtf8;

        যদি (isImported) {
            চালু;
        }

        জন্য (পূর্ণসংখ্যা j = 0; j < f->অঙ্ক_Names; j++) {
            যদি (f->names[j] and strcmp(f->names[j], i_name) == 0) {
                ফেরত f;
            }
        }
    }

    ফেরত ফাঁকা;
}


// Shared tail of the two lookups: a function is only usable once it has code.
স্থির
M3_ফলাফল PrepareFoundFunction (IM3Function* o_function, IM3Function i_function)
{
    M3_ফলাফল ফলাফল = m3Err_none;

    যদি (not i_function->compiled) {
_       (CompileFunction(i_function))
    }

    _catch:
    *o_function = ফলাফল ? ফাঁকা : i_function;

    ফেরত ফলাফল;
}


// Searches every module in the runtime, most recently loaded first. That is a
// guess once more than one module is loaded and two of them export the same
// name - m3_FindFunctionIn says which module is meant.
M3_ফলাফল m3_FindFunction (IM3Function* o_function, IM3Runtime i_runtime, ধ্রুবক অক্ষর* ধ্রুবক i_functionName)
{
                                                                d_m3Assert (o_function and i_runtime and i_functionName);
    IM3Function function = ফাঁকা;

    যদি (not i_runtime->modules) {
        *o_function = ফাঁকা;
        ফেরত "no modules loaded";
    }

    function = (IM3Function)ForEachModule(i_runtime, v_FindFunction, (শূন্য*)i_functionName);

    যদি (not function) {
        *o_function = ফাঁকা;
        ফেরত ErrorModule(m3Err_functionLookupFailed, i_runtime->modules, "'%s'", i_functionName);
    }

    ফেরত PrepareFoundFunction(o_function, function);
}


IM3Module m3_FindModule (IM3Runtime i_runtime, ধ্রুবক অক্ষর* ধ্রুবক i_moduleName)
{
    যদি (not i_runtime or not i_moduleName) {
        ফেরত ফাঁকা;
    }

    // the list is newest-first, so a name registered twice names the newer one
    জন্য (IM3Module m = i_runtime->modules; m; m = m->পরবর্তী) {
        যদি (m->name and strcmp(m->name, i_moduleName) == 0) {
            ফেরত m;
        }
    }

    ফেরত ফাঁকা;
}


// Searches one module's exports, which is what naming a module means.
M3_ফলাফল m3_FindFunctionIn (IM3Function* o_function, IM3Module i_module, ধ্রুবক অক্ষর* ধ্রুবক i_functionName)
{
                                                                d_m3Assert (o_function and i_functionName);
    যদি (not i_module) {
        *o_function = ফাঁকা;
        ফেরত m3Err_functionLookupFailed;      // ErrorModule would deref it
    }

    IM3Function function = (IM3Function)v_FindFunction(i_module, (শূন্য*)i_functionName);

    যদি (not function) {
        *o_function = ফাঁকা;
        ফেরত ErrorModule(m3Err_functionLookupFailed, i_module, "'%s'", i_functionName);
    }

    ফেরত PrepareFoundFunction(o_function, function);
}


M3_ফলাফল m3_GetTableFunction (IM3Function* o_function, IM3Module i_module, uint32_t i_সূচক)
{
_try {
    M3Table*    table;
    IM3Function function;

    _throwif("no table", i_module->অঙ্ক_Tables == 0);

    table = i_module->tables[0];
    _throwif("function index out of range", i_সূচক >= table->আকার_2);

    function = (IM3Function)table->elements[i_সূচক];

    যদি (function) {
        যদি (not function->compiled) {
_           (CompileFunction(function))
        }
    }

    *o_function = function;
} _catch:
    ফেরত ফলাফল;
}


স্থির
M3_ফলাফল checkStartFunction (IM3Module i_module)
{
    M3_ফলাফল ফলাফল = m3Err_none;                               d_m3Assert(i_module);

    // Check if start function needs to be called
    যদি (i_module->startFunction >= 0) {
        ফলাফল = m3_RunStart(i_module);
    }

    ফেরত ফলাফল;
}

uint32_t m3_Get_Arg_গুনতি (IM3Function i_function)
{
    যদি (i_function) {
        IM3FuncType ft = i_function->funcType;
        যদি (ft) {
            ফেরত ft->অঙ্ক_Args;
        }
    }
    ফেরত 0;
}

uint32_t m3_Get_Ret_গুনতি (IM3Function i_function)
{
    যদি (i_function) {
        IM3FuncType ft = i_function->funcType;
        যদি (ft) {
            ফেরত ft->অঙ্ক_Rets;
        }
    }
    ফেরত 0;
}


M3_মান_Type m3_GetArgType (IM3Function i_function, uint32_t index)
{
    যদি (i_function) {
        IM3FuncType ft = i_function->funcType;
        যদি (ft and index < ft->অঙ্ক_Args) {
            ফেরত (M3_মান_Type)BaseTypeOf(d_FuncArgType(ft, index));
        }
    }
    ফেরত c_m3Type_none;
}

M3_মান_Type m3_GetRetType (IM3Function i_function, uint32_t index)
{
    যদি (i_function) {
        IM3FuncType ft = i_function->funcType;
        যদি (ft and index < ft->অঙ্ক_Rets) {
            ফেরত (M3_মান_Type)BaseTypeOf(d_FuncRetType(ft, index));
        }
    }
    ফেরত c_m3Type_none;
}


// The call APIs marshal their arguments into the runtime stack before any
// compiled code runs, so op_Entry's overflow check comes too late to cover those
// writes - and for a host function or an import linked to another module there
// is no op_Entry of our own to reach at all. Bound the area about to be written
// here instead: results and arguments each take a u64 at this boundary, however
// wide the slot is inside. Every caller goes through this, so none of them can
// be the one that forgets.
স্থির
M3_ফলাফল Get_স্তূপ_Pointer_For_Args (IM3Function i_function, u8** o_স্তূপ)
{
    IM3Runtime  runtime = i_function->module->runtime;
    IM3FuncType ftype   = i_function->funcType;

    // the stack a paused invocation holds is not available to this one
    size_t used      = (size_t)((u8*)runtime->স্তূপ - (u8*)runtime->origin_স্তূপ);
    size_t needed    = ((size_t)ftype->অঙ্ক_Rets + ftype->অঙ্ক_Args) * আকার(u64);
    size_t available = (size_t)runtime->অঙ্ক_স্তূপ_Slots * আকার(m3slot_t) - used;

    যদি (needed > available) {
        ফেরত m3_Err_trap_স্তূপ_Overflow;
    }

    u64* স্তূপ = (u64*)runtime->স্তূপ;

    স্তূপ += ftype->অঙ্ক_Rets;

    *o_স্তূপ = (u8*)স্তূপ;

    ফেরত m3Err_none;
}


M3_ফলাফল m3_CallV (IM3Function i_function, ...)
{
    va_list ap;
    va_start(ap, i_function);
    M3_ফলাফল r = m3_CallVL(i_function, ap);
    va_end(ap);
    ফেরত r;
}

স্থির
শূন্য Report_Native_স্তূপ_Usage ()
{
#যদি d_m3_Log_Native_স্তূপ
    পূর্ণসংখ্যা স্তূপ_Used = m3_স্তূপ_Get_Max();
    fprintf(stderr, "Native stack used: %d\n", স্তূপ_Used);
#endif
}


M3_ফলাফল m3_CallVL (IM3Function i_function, va_list i_args)
{
    IM3Runtime  runtime = i_function->module->runtime;
    IM3FuncType ftype   = i_function->funcType;
    M3_ফলাফল    ফলাফল  = m3Err_none;
    u8*         s       = ফাঁকা;

    যদি (!i_function->compiled) {
        ফেরত m3Err_missingCompiledCode;
    }

#যদি d_m3RecordBacktraces
    ClearBacktrace(runtime);
#endif

    m3_স্তূপ_Check_Init();

_   (checkStartFunction(i_function->module))

_   (Get_স্তূপ_Pointer_For_Args(i_function, &s))

    জন্য (u32 i = 0; i < ftype->অঙ্ক_Args; ++i) {
        নির্বাচন (d_FuncArgType(ftype, i)) {
        ক্ষেত্র c_m3Type_i32:
            *(i32*)(s) = va_arg(i_args, i32);
            s += 8;
            বিরতি;
        ক্ষেত্র c_m3Type_i64:
            *(i64*)(s) = va_arg(i_args, i64);
            s += 8;
            বিরতি;
        ক্ষেত্র c_m3Type_funcref:
        ক্ষেত্র c_m3Type_externref:
        ক্ষেত্র c_m3Type_exnref:
            *(uintptr_t*)(s) = va_arg(i_args, uintptr_t);
            s += 8;
            বিরতি;
#যদি d_m3HasFloat
        ক্ষেত্র c_m3Type_f32:
            *(f32*)(s) = (f32)va_arg(i_args, f64);
            s += 8;
            বিরতি; // f32 is passed as f64
        ক্ষেত্র c_m3Type_f64:
            *(f64*)(s) = va_arg(i_args, f64);
            s += 8;
            বিরতি;
#endif
        পূর্বনির্ধারিত: ফেরত "unknown argument type";
        }
    }

    ফলাফল = RunCodeChecked(runtime, i_function);
    Report_Native_স্তূপ_Usage();

    runtime->lastCalled = ফলাফল ? ফাঁকা : i_function;

    _catch: ফেরত ফলাফল;
}

M3_ফলাফল m3_Call (IM3Function i_function, uint32_t i_argc, ধ্রুবক শূন্য* i_argptrs[])
{
    IM3Runtime  runtime = i_function->module->runtime;
    IM3FuncType ftype   = i_function->funcType;
    M3_ফলাফল    ফলাফল  = m3Err_none;
    u8*         s       = ফাঁকা;

    যদি (i_argc != ftype->অঙ্ক_Args) {
        ফেরত m3_Err_argument_গুনতি_Mismatch;
    }
    যদি (!i_function->compiled) {
        ফেরত m3Err_missingCompiledCode;
    }

#যদি d_m3RecordBacktraces
    ClearBacktrace(runtime);
#endif

    m3_স্তূপ_Check_Init();

_   (checkStartFunction(i_function->module))

_   (Get_স্তূপ_Pointer_For_Args(i_function, &s))

    জন্য (u32 i = 0; i < ftype->অঙ্ক_Args; ++i) {
        নির্বাচন (d_FuncArgType(ftype, i)) {
        ক্ষেত্র c_m3Type_i32:
            *(i32*)(s) = *(i32*)i_argptrs[i];
            s += 8;
            বিরতি;
        ক্ষেত্র c_m3Type_i64:
            *(i64*)(s) = *(i64*)i_argptrs[i];
            s += 8;
            বিরতি;
        ক্ষেত্র c_m3Type_funcref:
        ক্ষেত্র c_m3Type_externref:
        ক্ষেত্র c_m3Type_exnref:
            *(uintptr_t*)(s) = *(uintptr_t*)i_argptrs[i];
            s += 8;
            বিরতি;
#যদি d_m3HasFloat
        ক্ষেত্র c_m3Type_f32:
            *(f32*)(s) = *(f32*)i_argptrs[i];
            s += 8;
            বিরতি;
        ক্ষেত্র c_m3Type_f64:
            *(f64*)(s) = *(f64*)i_argptrs[i];
            s += 8;
            বিরতি;
#endif
        পূর্বনির্ধারিত: ফেরত "unknown argument type";
        }
    }

    ফলাফল = RunCodeChecked(runtime, i_function);

    Report_Native_স্তূপ_Usage();

    runtime->lastCalled = ফলাফল ? ফাঁকা : i_function;

    _catch: ফেরত ফলাফল;
}

// Argument parsing for m3_CallArgv. Strict on purpose: the whole string has to
// be consumed, so "12abc" is rejected rather than read as 12, and an empty or
// unparsable argument is an error rather than the zero that strtoul with a
// NULL end pointer used to hand back. See wasm3/wasm3#367.
স্থির
M3_ফলাফল ParseArgInteger (ccstr_t i_arg, u32 i_অঙ্ক_Bits, u64* o_মান)
{
    যদি (not i_arg or not *i_arg) {
        ফেরত "empty argument";
    }

    // strtoull would skip leading space, but trailing space is rejected below;
    // accepting one and not the other would just be confusing
    যদি (isspace((unsigned অক্ষর)*i_arg)) {
        ফেরত "argument is not a number";
    }

    অক্ষর* end = ফাঁকা;
    u64   মান;

    errno = 0;

    // an argument may be spelled signed or unsigned: -1 and 4294967295 name the
    // same i32
    যদি (*i_arg == '-') {
        i64 signed_মান = strtoll(i_arg, &end, 10);

        যদি (i_অঙ্ক_Bits == 32 and (signed_মান < INT32_MIN or signed_মান > INT32_MAX)) {
            ফেরত "argument out of range";
        }

        মান = (u64)signed_মান;
    } অন্যথা {
        মান = strtoull(i_arg, &end, 10);

        যদি (i_অঙ্ক_Bits == 32 and মান > UINT32_MAX) {
            ফেরত "argument out of range";
        }
    }

    যদি (errno == ERANGE) {
        ফেরত "argument out of range";
    }

    যদি (end == i_arg or *end) {
        ফেরত "argument is not a number";
    }

    *o_মান = মান;

    ফেরত m3Err_none;
}


#যদি d_m3HasFloat
স্থির
M3_ফলাফল ParseArgFloat (ccstr_t i_arg, f64* o_মান)
{
    যদি (not i_arg or not *i_arg) {
        ফেরত "empty argument";
    }

    যদি (isspace((unsigned অক্ষর)*i_arg)) {
        ফেরত "argument is not a number";
    }

    অক্ষর* end = ফাঁকা;

    errno = 0;

    f64 মান = strtod(i_arg, &end);

    যদি (end == i_arg or *end) {
        ফেরত "argument is not a number";
    }

    // strtod reports underflow through ERANGE as well, and a denormal result is
    // perfectly usable, so only an overflow to infinity is out of range
    যদি (errno == ERANGE and (মান > DBL_MAX or মান < -DBL_MAX)) {
        ফেরত "argument out of range";
    }

    *o_মান = মান;

    ফেরত m3Err_none;
}
#endif


// A reference argument is either the null reference or a host handle written as
// an integer.
স্থির
M3_ফলাফল ParseArgReference (ccstr_t i_arg, u64* o_মান)
{
    যদি (i_arg and strcmp(i_arg, "null") == 0) {
        *o_মান = 0;
        ফেরত m3Err_none;
    }

    ফেরত ParseArgInteger(i_arg, 64, o_মান);
}


M3_ফলাফল m3_CallArgv (IM3Function i_function, uint32_t i_argc, ধ্রুবক অক্ষর* i_argv[])
{
    IM3FuncType ftype   = i_function->funcType;
    IM3Runtime  runtime = i_function->module->runtime;
    M3_ফলাফল    ফলাফল  = m3Err_none;
    u8*         s       = ফাঁকা;

    যদি (i_argc != ftype->অঙ্ক_Args) {
        ফেরত m3_Err_argument_গুনতি_Mismatch;
    }
    যদি (!i_function->compiled) {
        ফেরত m3Err_missingCompiledCode;
    }

#যদি d_m3RecordBacktraces
    ClearBacktrace(runtime);
#endif

    m3_স্তূপ_Check_Init();

_   (checkStartFunction(i_function->module))

_   (Get_স্তূপ_Pointer_For_Args(i_function, &s))

    জন্য (u32 i = 0; i < ftype->অঙ্ক_Args; ++i) {
        u64 মান = 0;
#যদি d_m3HasFloat
        f64 fvalue = 0;
#endif
        নির্বাচন (d_FuncArgType(ftype, i)) {
        ক্ষেত্র c_m3Type_i32:
_           (ParseArgInteger(i_argv[i], 32, &মান)) * (i32*)(s) = (i32)মান;
            s += 8;
            বিরতি;
        ক্ষেত্র c_m3Type_i64:
_           (ParseArgInteger(i_argv[i], 64, &মান)) * (i64*)(s) = (i64)মান;
            s += 8;
            বিরতি;
        ক্ষেত্র c_m3Type_funcref:
        ক্ষেত্র c_m3Type_externref:
        ক্ষেত্র c_m3Type_exnref:
_           (ParseArgReference(i_argv[i], &মান)) * (uintptr_t*)(s) = (uintptr_t)মান;
            s += 8;
            বিরতি;
#যদি d_m3HasFloat
            // strtof would be less portable
        ক্ষেত্র c_m3Type_f32:
_           (ParseArgFloat(i_argv[i], &fvalue)) * (f32*)(s) = (f32)fvalue;
            s += 8;
            বিরতি;
        ক্ষেত্র c_m3Type_f64:
_           (ParseArgFloat(i_argv[i], &fvalue)) * (f64*)(s) = fvalue;
            s += 8;
            বিরতি;
#endif
        পূর্বনির্ধারিত: _throw("unknown argument type");
        }
    }

    ফলাফল = RunCodeChecked(runtime, i_function);

    Report_Native_স্তূপ_Usage();

    runtime->lastCalled = ফলাফল ? ফাঁকা : i_function;

    _catch: ফেরত ফলাফল;
}


//u8 * AlignStackPointerTo64Bits (const u8 * i_stack)
//{
//    uintptr_t ptr = (uintptr_t) i_stack;
//    return (u8 *) ((ptr + 7) & ~7);
//}


M3_ফলাফল m3_GetResults (IM3Function i_function, uint32_t i_retc, ধ্রুবক শূন্য* o_retptrs[])
{
    IM3FuncType ftype   = i_function->funcType;
    IM3Runtime  runtime = i_function->module->runtime;

    যদি (i_retc != ftype->অঙ্ক_Rets) {
        ফেরত m3_Err_argument_গুনতি_Mismatch;
    }
    যদি (i_function != runtime->lastCalled) {
        ফেরত "function not called";
    }

    u8* s = (u8*)runtime->স্তূপ;

    জন্য (u32 i = 0; i < ftype->অঙ্ক_Rets; ++i) {
        নির্বাচন (d_FuncRetType(ftype, i)) {
        ক্ষেত্র c_m3Type_i32:
            *(i32*)o_retptrs[i] = *(i32*)(s);
            s += 8;
            বিরতি;
        ক্ষেত্র c_m3Type_i64:
            *(i64*)o_retptrs[i] = *(i64*)(s);
            s += 8;
            বিরতি;
        ক্ষেত্র c_m3Type_funcref:
        ক্ষেত্র c_m3Type_externref:
        ক্ষেত্র c_m3Type_exnref:
            *(uintptr_t*)o_retptrs[i] = *(uintptr_t*)(s);
            s += 8;
            বিরতি;
#যদি d_m3HasFloat
        ক্ষেত্র c_m3Type_f32:
            *(f32*)o_retptrs[i] = *(f32*)(s);
            s += 8;
            বিরতি;
        ক্ষেত্র c_m3Type_f64:
            *(f64*)o_retptrs[i] = *(f64*)(s);
            s += 8;
            বিরতি;
#endif
        পূর্বনির্ধারিত: ফেরত "unknown return type";
        }
    }
    ফেরত m3Err_none;
}

M3_ফলাফল m3_GetResultsV (IM3Function i_function, ...)
{
    va_list ap;
    va_start(ap, i_function);
    M3_ফলাফল r = m3_GetResultsVL(i_function, ap);
    va_end(ap);
    ফেরত r;
}

M3_ফলাফল m3_GetResultsVL (IM3Function i_function, va_list o_rets)
{
    IM3Runtime  runtime = i_function->module->runtime;
    IM3FuncType ftype   = i_function->funcType;

    যদি (i_function != runtime->lastCalled) {
        ফেরত "function not called";
    }

    u8* s = (u8*)runtime->স্তূপ;
    জন্য (u32 i = 0; i < ftype->অঙ্ক_Rets; ++i) {
        নির্বাচন (d_FuncRetType(ftype, i)) {
        ক্ষেত্র c_m3Type_i32:
            *va_arg(o_rets, i32*) = *(i32*)(s);
            s += 8;
            বিরতি;
        ক্ষেত্র c_m3Type_i64:
            *va_arg(o_rets, i64*) = *(i64*)(s);
            s += 8;
            বিরতি;
        ক্ষেত্র c_m3Type_funcref:
        ক্ষেত্র c_m3Type_externref:
        ক্ষেত্র c_m3Type_exnref:
            *va_arg(o_rets, uintptr_t*) = *(uintptr_t*)(s);
            s += 8;
            বিরতি;
#যদি d_m3HasFloat
        ক্ষেত্র c_m3Type_f32:
            *va_arg(o_rets, f32*) = *(f32*)(s);
            s += 8;
            বিরতি;
        ক্ষেত্র c_m3Type_f64:
            *va_arg(o_rets, f64*) = *(f64*)(s);
            s += 8;
            বিরতি;
#endif
        পূর্বনির্ধারিত: ফেরত "unknown argument type";
        }
    }
    ফেরত m3Err_none;
}

শূন্য ReleaseCodePageNoTrack (IM3Runtime i_runtime, IM3CodePage i_codePage)
{
    যদি (i_codePage) {
        IM3CodePage* list;

        bool pageFull = (অঙ্ক_Free_সারিগুলি(i_codePage) < d_m3_Code_Page_Free_সারিগুলি_Threshold);
        যদি (pageFull) {
            list = &i_runtime->pagesFull;
        } অন্যথা {
            list = &i_runtime->pagesOpen;
        }

        ঠেলো_Code_Page(list, i_codePage);                         m3log (emit, "release page: %d to queue: '%s'", i_codePage->info.sequence, pageFull ? "full" : "open")
    }
}


IM3CodePage AcquireCodePageWithCapacity (IM3Runtime i_runtime, u32 i_min_সারি_গুনতি)
{
    IM3CodePage page = RemoveCodePageOfCapacity(&i_runtime->pagesOpen, i_min_সারি_গুনতি);

    যদি (not page) {
        page = Environment_AcquireCodePage(i_runtime->environment, i_min_সারি_গুনতি);

        যদি (not page) {
            page = NewCodePage(i_runtime, i_min_সারি_গুনতি);
        }

        যদি (page) {
            i_runtime->অঙ্ক_Code_Pages++;
        }
    }

    যদি (page) {                                                  m3log (emit, "acquire page: %d", page->info.sequence);
        i_runtime->অঙ্ক_Active_Code_Pages++;
    }

    ফেরত page;
}


IM3CodePage AcquireCodePage (IM3Runtime i_runtime)
{
    ফেরত AcquireCodePageWithCapacity(i_runtime, d_m3_Code_Page_Free_সারিগুলি_Threshold);
}


শূন্য ReleaseCodePage (IM3Runtime i_runtime, IM3CodePage i_codePage)
{
    যদি (i_codePage) {
        ReleaseCodePageNoTrack(i_runtime, i_codePage);
        i_runtime->অঙ্ক_Active_Code_Pages--;

#যদি defined(DEBUG)
        u32 অঙ্ক_Open = গুনতি_Code_Pages(i_runtime->pagesOpen);
        u32 অঙ্ক_Full = গুনতি_Code_Pages(i_runtime->pagesFull);

        m3log(runtime, "runtime: %p; open-pages: %d; full-pages: %d; active: %d; total: %d", i_runtime, অঙ্ক_Open, অঙ্ক_Full, i_runtime->অঙ্ক_Active_Code_Pages, i_runtime->অঙ্ক_Code_Pages);

        d_m3Assert(অঙ্ক_Open + অঙ্ক_Full + i_runtime->অঙ্ক_Active_Code_Pages == i_runtime->অঙ্ক_Code_Pages);

#  যদি d_m3LogCodePages
        dump_code_page(i_codePage, /* startPC: */ ফাঁকা);
#  endif
#endif
    }
}


#যদি d_m3VerboseErrorMessages
M3_ফলাফল m3Error (M3_ফলাফল i_ফলাফল, IM3Runtime i_runtime, IM3Module i_module, IM3Function i_function,
                  ধ্রুবক অক্ষর* ধ্রুবক i_file, u32 i_সারি_অঙ্ক, ধ্রুবক অক্ষর* ধ্রুবক i_errorMessage, ...)
{
    যদি (i_runtime) {
        i_runtime->error         = (M3ErrorInfo){ .ফলাফল = i_ফলাফল, .runtime = i_runtime, .module = i_module, .function = i_function, .file = i_file, .সারি_2 = i_সারি_অঙ্ক };
        i_runtime->error.message = i_runtime->error_message;

        va_list args;
        va_start(args, i_errorMessage);
        vsnprintf(i_runtime->error_message, আকার(i_runtime->error_message), i_errorMessage, args);
        va_end(args);
    }

    ফেরত i_ফলাফল;
}
#endif


শূন্য m3_GetErrorInfo (IM3Runtime i_runtime, M3ErrorInfo* o_info)
{
    যদি (i_runtime) {
        *o_info = i_runtime->error;
        m3_ResetErrorInfo(i_runtime);
    }
}


শূন্য m3_ResetErrorInfo (IM3Runtime i_runtime)
{
    যদি (i_runtime) {
        M3_INIT(i_runtime->error);
        i_runtime->error.message = "";
    }
}

uint8_t* m3_GetMemory (IM3Module i_module, size_t* o_memory_আকার_In_Bytes, uint32_t i_memory_সূচক)
{
    uint8_t* memory = ফাঁকা;
    size_t   আকার_2   = 0;

    যদি (i_module and i_memory_সূচক < i_module->অঙ্ক_Memories) {
        IM3Memory mem = i_module->memories[i_memory_সূচক];

        যদি (mem->mallocated) {
            আকার_2 = mem->mallocated->দৈর্ঘ্য;

            যদি (আকার_2) {
                memory = m3_Mem_তথ্য(mem->mallocated);
            }
        }
    }

    যদি (o_memory_আকার_In_Bytes) {
        *o_memory_আকার_In_Bytes = আকার_2;
    }

    ফেরত memory;
}


size_t m3_Get_Memory_আকার (IM3Module i_module, uint32_t i_memory_সূচক)
{
    যদি (not i_module or i_memory_সূচক >= i_module->অঙ্ক_Memories) {
        ফেরত 0;
    }

    IM3Memory mem = i_module->memories[i_memory_সূচক];

    ফেরত mem->mallocated ? mem->mallocated->দৈর্ঘ্য : 0;
}


size_t m3_Get_Memory_আকার_At (ধ্রুবক শূন্য* i_memory)
{
    যদি (not i_memory) {
        ফেরত 0;
    }

    // the header sits immediately before the data it describes
    ধ্রুবক M3MemoryHeader* header = ((ধ্রুবক M3MemoryHeader*)i_memory) - 1;

    ফেরত header->দৈর্ঘ্য;
}


M3_ফলাফল m3_FindExportedMemory (IM3Module i_module, cstr_t i_name, u32* o_memory_সূচক)
{
    যদি (not i_module or not i_name or not o_memory_সূচক) {
        ফেরত m3Err_unknownMemory;
    }

    ধ্রুবক M3Export* e = Module_FindExport(i_module, d_externalKind_memory, i_name);

    যদি (not e) {
        ফেরত m3Err_unknownMemory;
    }

    *o_memory_সূচক = e->index;
    ফেরত m3Err_none;
}


bool Module_HasLinkedHostImport (IM3Module i_module, cstr_t i_importModule)
{
    জন্য (u32 i = 0; i < i_module->অঙ্ক_Functions; ++i) {
        IM3Function f = &i_module->functions[i];

        // hostMemory is set when a host function is bound to the import, so it
        // is also the mark of one - see CompileRawFunction
        যদি (not f->hostMemory or f->wasm or f->resolved) {
            চালু;
        }

        যদি (f->import.moduleUtf8 and strcmp(f->import.moduleUtf8, i_importModule) == 0) {
            ফেরত true;
        }
    }

    ফেরত false;
}


M3_ফলাফল m3_BindImportMemory (IM3Module io_module, cstr_t i_importModule, u32 i_memory_সূচক)
{
    যদি (not io_module or not i_importModule) {
        ফেরত m3Err_moduleNotLinked;
    }

    যদি (i_memory_সূচক >= io_module->অঙ্ক_Memories) {
        ফেরত m3Err_unknownMemory;
    }

    // the memory itself, not the index: linking may have pointed this slot at
    // another module's memory, and growing one reallocates behind it
    IM3Memory memory = io_module->memories[i_memory_সূচক];

    ধ্রুবক bool wildcardModule = (strcmp(i_importModule, "*") == 0);

    জন্য (u32 i = 0; i < io_module->অঙ্ক_Functions; ++i) {
        IM3Function f = &io_module->functions[i];

        // only an import that a host function was bound to has a memory to
        // pin: one with a body of its own reads memory through the interpreter,
        // and one resolved to another module's export runs that module's code
        যদি (not f->hostMemory or f->wasm or f->resolved) {
            চালু;
        }

        যদি (wildcardModule or (f->import.moduleUtf8 and strcmp(f->import.moduleUtf8, i_importModule) == 0)) {
            f->hostMemory = memory;
        }
    }

    ফেরত m3Err_none;
}


M3BacktraceInfo* m3_GetBacktrace (IM3Runtime i_runtime)
{
#যদি d_m3RecordBacktraces
    ফেরত &i_runtime->backtrace;
#অন্যথা
    ফেরত ফাঁকা;
#endif
}


//-------------------------------------------------------------------------------------------------------------------------------
//  snapshots & suspendable execution
//-------------------------------------------------------------------------------------------------------------------------------

শূন্য m3_SetSuspendable (IM3Runtime io_runtime, bool i_suspendable)
{
    যদি (io_runtime) {
        io_runtime->isSuspendable = i_suspendable;
    }
}

শূন্য m3_RequestSuspend (IM3Runtime io_runtime)
{
    যদি (io_runtime) {
        io_runtime->suspendRequested = true;
    }
}

bool m3_IsSuspended (IM3Runtime i_runtime)
{
    যদি (!i_runtime) {
        ফেরত false;
    }
#যদি d_m3_Has_স্তূপ_Switching
    যদি (i_runtime->মূল_Continuation && i_runtime->মূল_Continuation->state == cont_suspended) {
        ফেরত true;
    }
#endif
    ফেরত false;
}

M3_ফলাফল m3_ResumeRuntime (IM3Runtime io_runtime)
{
    যদি (!io_runtime) {
        ফেরত m3Err_mallocFailed;
    }
    যদি (!io_runtime->isSuspendable) {
        ফেরত m3Err_none;
    }
#যদি d_m3HasSnapshots
    // a restore that failed leaves nothing to resume, and says so every time
    // rather than resuming nothing successfully
    যদি (io_runtime->মূল_Continuation and io_runtime->মূল_Continuation->entryFunction and
        io_runtime->মূল_Continuation->entryFunction->module->isUnusable) {
        ফেরত "a snapshot failed to restore into the module, so it cannot run";
    }
#endif
#যদি d_m3_Has_স্তূপ_Switching
    যদি (!io_runtime->মূল_Continuation || io_runtime->মূল_Continuation->state != cont_suspended) {
        ফেরত m3Err_none;
    }
    // what finishes returns into the bottom of the stack, where it was entered
    io_runtime->স্তূপ = io_runtime->origin_স্তূপ;

    // this is the outermost call again, as far as a host function calling back
    // into Wasm from inside it can tell
    io_runtime->entered = io_runtime->মূল_Continuation->entryFunction;
    io_runtime->callNesting++;

    d_m3_স্তূপ_Limit_Enter(io_runtime);
    io_runtime->resumePastCheck = true;
    M3_ফলাফল ফলাফল             = (M3_ফলাফল)ResumeContinuation(io_runtime, io_runtime->মূল_Continuation);
    io_runtime->resumePastCheck = false;
    d_m3_স্তূপ_Limit_Leave(io_runtime);

#  যদি d_m3HasExceptionHandling
    যদি (M3_UNLIKELY(ফলাফল == m3Err_pendingException)) {
        ফলাফল = m3Err_trapUncaughtException;
    }
    RecordUncaughtException(io_runtime, ফলাফল);
#  endif

    io_runtime->callNesting--;

#  যদি d_m3HasExceptionHandling
    যদি (ফলাফল != m3Err_continuationSuspended and io_runtime->callNesting == 0) {
        FreeExceptions(io_runtime);
    }
#  endif

    // the results are where the paused call left room for them, so a finished
    // resume can be asked for them the way the call itself could have been
    io_runtime->lastCalled = (ফলাফল == m3Err_none) ? io_runtime->মূল_Continuation->entryFunction : ফাঁকা;

    Runtime_Place_Call_স্তূপ(io_runtime);
    ফেরত ফলাফল;
#অন্যথা
    ফেরত m3Err_none;
#endif
}
