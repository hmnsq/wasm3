//
//  m3_env.c
//
//  Created by Steven Massey on 4/19/19.
//  Copyright © 2019 Steven Massey. All rights reserved.
//

#समावेश <stdarg.h>
#समावेश <limits.h>
#समावेश <errno.h>
#समावेश <float.h>
#समावेश <ctype.h>

#समावेश "m3_env.h"
#समावेश "m3_bind.h"
#समावेश "m3_deterministic.h"
#समावेश "m3_compile.h"
#समावेश "m3_host.h"
#समावेश "m3_exception.h"
#समावेश "m3_info.h"


IM3Environment m3_NewEnvironment ()
{
    IM3Environment env = m3_AllocStruct(M3Environment);

    अगर (env) {
        _try
        {
            // create FuncTypes for all simple block return ValueTypes.
            // v128 is skipped: it parses as a slot but has no operations.
            क्रम (u8 t = c_m3Type_none; t < c_m3_Type_गिनती; t++) {
                अगर (t == c_m3Type_v128) {
                    जारी;
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
        अगर (परिणाम) {
            m3_FreeEnvironment(env);
            env = शून्य;
        }
    }

    वापस env;
}


व्योम Environment_Release (IM3Environment i_environment)
{
    IM3FuncType ftype = i_environment->funcTypes;

    जबतक (ftype) {
        IM3FuncType अग्र = ftype->अग्र;
        m3_Free(ftype);
        ftype = अग्र;
    }

    m3log(runtime, "freeing %d pages from environment", गिनती_Code_Pages(i_environment->pagesReleased));
    FreeCodePages(&i_environment->pagesReleased);
}


व्योम m3_FreeEnvironment (IM3Environment i_environment)
{
    अगर (i_environment) {
        Environment_Release(i_environment);
        m3_Free(i_environment);
    }
}


व्योम m3_SetCustomSectionHandler (IM3Environment i_environment, M3SectionHandler i_handler)
{
    अगर (i_environment) {
        i_environment->customSectionHandler = i_handler;
    }
}


// returns the same io_funcType or replaces it with an equivalent that's already in the type linked list
M3_परिणाम Environment_AddFuncType (IM3Environment i_environment, IM3FuncType* io_funcType)
{
    IM3FuncType addType = *io_funcType;
    IM3FuncType newType = i_environment->funcTypes;

    जबतक (newType) {
        अगर (not newType->inRecGroup and AreFuncTypesEqual(newType, addType)) {
            m3_Free(addType);
            अवरोध;
        }

        newType = newType->अग्र;
    }

    अगर (newType == शून्य) {
        // a type index has to fit in the heap type field of an m3type_t
        अगर (i_environment->अंक_Func_Types >= d_m3_Max_Sane_Types_गिनती) {
            m3_Free(addType);
            *io_funcType = शून्य;
            वापस "too many distinct function types";
        }

        newType                  = addType;
        newType->canonical_सूचकांक  = i_environment->अंक_Func_Types++;
        newType->अग्र            = i_environment->funcTypes;
        i_environment->funcTypes = newType;
    }

    *io_funcType = newType;

    वापस m3Err_none;
}


// Adds a type without looking for an equal one first, and hands back the index
// it was given.
//
// Members of a recursive group do not get to share an entry with a type that
// merely looks like them: the group is their identity, so two groups spelled
// the same way are still two groups, and a type inside one is never the type
// outside it that matches field for field. Reserving the indices up front is
// also what lets the members refer to each other while they are being read.
M3_परिणाम Environment_ReserveFuncTypes (IM3Environment i_environment, u32 i_गिनती, u16* o_first_सूचकांक)
{
    अगर ((u64)i_environment->अंक_Func_Types + i_गिनती > d_m3_Max_Sane_Types_गिनती) {
        वापस "too many distinct function types";
    }

    *o_first_सूचकांक = i_environment->अंक_Func_Types;
    i_environment->अंक_Func_Types += (u16)i_गिनती;

    वापस m3Err_none;
}


व्योम Environment_AdoptFuncType (IM3Environment i_environment, IM3FuncType i_funcType, u16 i_canonical_सूचकांक)
{
    i_funcType->inRecGroup     = true;
    i_funcType->canonical_सूचकांक = i_canonical_सूचकांक;
    i_funcType->अग्र           = i_environment->funcTypes;
    i_environment->funcTypes   = i_funcType;
}


IM3CodePage RemoveCodePageOfCapacity (M3CodePage** io_list, u32 i_minimum_पंक्ति_गिनती)
{
    IM3CodePage पूर्व = शून्य;
    IM3CodePage page = *io_list;

    जबतक (page) {
        अगर (अंक_Free_पंक्तियाँ(page) >= i_minimum_पंक्ति_गिनती) {             d_m3Assert (page->info.usage_गिनती == 0);
            IM3CodePage अग्र = page->info.अग्र;
            अगर (पूर्व) {
                पूर्व->info.अग्र = अग्र; // mid-list
            } अन्यथा {
                *io_list = अग्र;       // front of list
            }

            अवरोध;
        }

        पूर्व = page;
        page = page->info.अग्र;
    }

    वापस page;
}


IM3CodePage Environment_AcquireCodePage (IM3Environment i_environment, u32 i_minimum_पंक्ति_गिनती)
{
    वापस RemoveCodePageOfCapacity(&i_environment->pagesReleased, i_minimum_पंक्ति_गिनती);
}


व्योम Environment_ReleaseCodePages (IM3Environment i_environment, IM3CodePage i_codePageList)
{
    IM3CodePage end = i_codePageList;

    जबतक (end) {
        end->info.पंक्ति_सूचकांक = 0; // reset page
#अगर d_m3RecordBacktraces
        end->info.mapping->आकार = 0;
#पूर्ण_अगर // d_m3RecordBacktraces

        IM3CodePage अग्र = end->info.अग्र;
        अगर (not अग्र) {
            अवरोध;
        }

        end = अग्र;
    }

    अगर (end) {
        // push list to front
        end->info.अग्र               = i_environment->pagesReleased;
        i_environment->pagesReleased = i_codePageList;
    }
}


IM3Runtime m3_NewRuntime (IM3Environment i_environment, u32 i_ढेर_आकार_In_Bytes, व्योम* i_userdata)
{
    IM3Runtime runtime = m3_AllocStruct(M3Runtime);

    अगर (runtime) {
        m3_ResetErrorInfo(runtime);

        runtime->environment = i_environment;
        runtime->userdata    = i_userdata;

        Deterministic_InitRuntime(runtime);

        runtime->origin_ढेर = m3_Malloc("Wasm Stack", i_ढेर_आकार_In_Bytes + 4 * माप(m3slot_t)); // TODO: more precise stack checks

        अगर (runtime->origin_ढेर) {
            runtime->ढेर         = runtime->origin_ढेर;
            runtime->अंक_ढेर_Slots = i_ढेर_आकार_In_Bytes / माप(m3slot_t);          m3log (runtime, "new stack: %p, slots: %u", runtime->origin_ढेर, runtime->अंक_ढेर_Slots);
        } अन्यथा {
            m3_Free(runtime);
        }
    }

    वापस runtime;
}

व्योम m3_SetValidation (IM3Runtime i_runtime, bool i_enable)
{
#अगर d_m3EnableValidation
    अगर (i_runtime) {
        i_runtime->skipValidation = not i_enable;
    }
#अन्यथा
    (व्योम)i_runtime;
    (व्योम)i_enable;              // nothing to skip: the validator was compiled out
#पूर्ण_अगर
}

M3_परिणाम m3_SetResourceLimit (IM3Runtime i_runtime, M3ResourceLimit i_limit, uint64_t i_मूल्य)
{
    अगर (not i_runtime) {
        वापस m3Err_mallocFailed;
    }
    चयन (i_limit) {
    हाल c_m3Limit_MemoryBytes:
        अगर (i_मूल्य and i_मूल्य < i_runtime->memoryBytesUsed) {
            वापस m3Err_resourceLimitBelowUsage;
        }
        i_runtime->memoryBytesLimit = i_मूल्य;
        वापस m3Err_none;
    हाल c_m3Limit_TableElements:
        अगर (i_मूल्य and i_मूल्य < i_runtime->tableElementsUsed) {
            वापस m3Err_resourceLimitBelowUsage;
        }
        i_runtime->tableElementsLimit = i_मूल्य;
        वापस m3Err_none;
#अगर d_m3_Has_ढेर_Switching
    हाल c_m3Limit_Continuations:
        अगर (i_मूल्य and i_मूल्य < i_runtime->continuationsAllocated) {
            वापस m3Err_resourceLimitBelowUsage;
        }
        i_runtime->continuationsLimit = (u32)M3_MIN(i_मूल्य, (u64)UINT32_MAX);
        जबतक (i_मूल्य and i_runtime->val_ढेर_Pool_गिनती > i_मूल्य - i_runtime->continuationsAllocated) {
            m3_Free(i_runtime->val_ढेर_Pool[--i_runtime->val_ढेर_Pool_गिनती]);
        }
        वापस m3Err_none;
#अन्यथा
    हाल c_m3Limit_Continuations: वापस m3Err_resourceLimitNotSupported;
#पूर्ण_अगर
    हाल c_m3Limit_GasUnits:
#अगर d_m3HasGasMetering
        i_runtime->gasLimit = i_runtime->gasRemaining = (i64)M3_MIN(i_मूल्य, (u64)INT64_MAX);
        वापस m3Err_none;
#अन्यथा
        वापस m3Err_resourceLimitNotSupported;
#पूर्ण_अगर
    शेष: वापस m3Err_unknownResourceLimit;
    }
}

uint64_t m3_GetResourceLimit (IM3Runtime i_runtime, M3ResourceLimit i_limit)
{
    अगर (not i_runtime) {
        वापस 0;
    }
    चयन (i_limit) {
    हाल c_m3Limit_MemoryBytes: वापस i_runtime->memoryBytesLimit;
    हाल c_m3Limit_TableElements: वापस i_runtime->tableElementsLimit;
#अगर d_m3_Has_ढेर_Switching
    हाल c_m3Limit_Continuations: वापस i_runtime->continuationsLimit;
#पूर्ण_अगर
#अगर d_m3HasGasMetering
    हाल c_m3Limit_GasUnits: वापस (u64)i_runtime->gasLimit;
#पूर्ण_अगर
    शेष: वापस 0;
    }
}

uint64_t m3_GetResourceUsage (IM3Runtime i_runtime, M3ResourceLimit i_limit)
{
    अगर (not i_runtime) {
        वापस 0;
    }
    चयन (i_limit) {
    हाल c_m3Limit_MemoryBytes: वापस i_runtime->memoryBytesUsed;
    हाल c_m3Limit_TableElements: वापस i_runtime->tableElementsUsed;
#अगर d_m3_Has_ढेर_Switching
    हाल c_m3Limit_Continuations: वापस i_runtime->continuationsAllocated;
#पूर्ण_अगर
#अगर d_m3HasGasMetering
    हाल c_m3Limit_GasUnits: वापस (u64)i_runtime->gasLimit - (u64)i_runtime->gasRemaining;
#पूर्ण_अगर
    शेष: वापस 0;
    }
}

व्योम* m3_Get_User_तथ्य (IM3Runtime i_runtime)
{
    वापस i_runtime ? i_runtime->userdata : शून्य;
}


व्योम* ForEachModule (IM3Runtime i_runtime, ModuleVisitor i_visitor, व्योम* i_info)
{
    व्योम* r = शून्य;

    IM3Module module = i_runtime->modules;

    जबतक (module) {
        IM3Module अग्र = module->अग्र;
        अगर ((r = i_visitor(module, i_info))) {
            अवरोध;
        }

        module = अग्र;
    }

    वापस r;
}


व्योम* _FreeModule (IM3Module i_module, व्योम* i_info)
{
    m3_FreeModule(i_module);
    वापस शून्य;
}


// A module's memory and table index spaces hold borrowed pointers: linking
// repoints an imported slot at the memory or table the exporting module owns.
// m3_FreeModule tells a borrowed slot from an owned one by reading that owner,
// which only holds while the object is still allocated - and the free walk
// below visits modules in load order, with no ordering between an owner and
// the modules borrowing from it. So clear the borrowed slots first, in a pass
// that frees nothing; the walk then skips them on the NULL check it already
// does, instead of reading a memory some earlier module has freed.
व्योम* _ReleaseBorrowedSlots (IM3Module i_module, व्योम* i_info)
{
    क्रम (u32 i = 0; i < i_module->अंक_Memories; ++i) {
        IM3Memory memory = i_module->memories[i];

        अगर (memory and memory->owner != i_module) {
            i_module->memories[i] = शून्य;
        }
    }

    क्रम (u32 i = 0; i < i_module->अंक_Tables; ++i) {
        IM3Table table = i_module->tables[i];

        अगर (table and table->owner != i_module) {
            i_module->tables[i] = शून्य;
        }
    }

    वापस शून्य;
}


व्योम Runtime_Release (IM3Runtime i_runtime)
{
    ForEachModule(i_runtime, _ReleaseBorrowedSlots, शून्य);
    ForEachModule(i_runtime, _FreeModule, शून्य);                    d_m3Assert (i_runtime->अंक_Active_Code_Pages == 0);

    Environment_ReleaseCodePages(i_runtime->environment, i_runtime->pagesOpen);
    Environment_ReleaseCodePages(i_runtime->environment, i_runtime->pagesFull);

    m3_Free(i_runtime->origin_ढेर);

#अगर d_m3EnableValidation
    m3_Free(i_runtime->validator);
#पूर्ण_अगर

#अगर d_m3HasExceptionHandling
    // a runtime let go of while suspended still holds its exceptions
    FreeExceptions(i_runtime);
    m3_Free(i_runtime->uncaughtException);
#पूर्ण_अगर
#अगर d_m3HasExceptionHandling || d_m3_Has_ढेर_Switching
    जबतक (i_runtime->hostTags) {
        M3HostTag* अग्र = i_runtime->hostTags->अग्र;
        m3_Free(i_runtime->hostTags);
        i_runtime->hostTags = अग्र;
    }
#पूर्ण_अगर
#अगर d_m3_Has_ढेर_Switching
    Continuation_ReleaseAll(i_runtime);
#पूर्ण_अगर
}


व्योम m3_FreeRuntime (IM3Runtime i_runtime)
{
    अगर (i_runtime) {
        m3_PrintProfilerInfo();

        Runtime_Release(i_runtime);
        m3_Free(i_runtime);
    }
}


#अगर d_m3_Has_ढेर_Switching

IM3Continuation Continuation_New (IM3Runtime i_runtime, IM3FuncType i_type, IM3Function i_function)
{
    IM3Continuation cont = m3_AllocStruct(M3Continuation);
    अगर (!cont) {
        वापस शून्य;
    }

    cont->type          = i_type;
    cont->entryFunction = i_function;
    cont->state         = cont_allocated;

    cont->अंक_ढेर_Slots = d_m3_Continuation_ढेर_Slots;
    // frames are recorded only when this continuation actually suspends, and
    // only as deep as it got, so the array starts out unallocated
    cont->frames    = शून्य;
    cont->framesCap = 0;
    cont->अंक_Frames = 0;

    cont->sp = cont->val_ढेर;
    cont->pc = i_function ? i_function->compiled : शून्य;
    cont->r0 = 0;
#  अगर d_m3HasFloat
    cont->fp0 = 0;
#  endif
    cont->bound_Args_गिनती    = 0;
    cont->अंक_Suspend_Results = 0;
    cont->अंक_Handlers       = 0;
    cont->handlersPC        = शून्य;
    cont->parent            = शून्य;

    cont->अग्र               = i_runtime->continuations;
    i_runtime->continuations = cont;

    वापस cont;
}

IM3Continuation Continuation_ForkSuspended (IM3Runtime i_runtime, IM3Continuation i_cont)
{
    IM3Continuation newCont = Continuation_New(i_runtime, i_cont->type, i_cont->entryFunction);
    अगर (M3_UNLIKELY(not newCont)) {
        वापस शून्य;
    }

    newCont->val_ढेर      = i_cont->val_ढेर;
    newCont->अंक_ढेर_Slots = i_cont->अंक_ढेर_Slots;
    newCont->sp            = i_cont->sp;
    newCont->pc            = i_cont->pc;
    newCont->suspendPoint  = i_cont->suspendPoint;
    newCont->r0            = i_cont->r0;
#  अगर d_m3HasFloat
    newCont->fp0 = i_cont->fp0;
#  endif
    newCont->frames    = i_cont->frames;
    newCont->framesCap = i_cont->framesCap;
    newCont->अंक_Frames = i_cont->अंक_Frames;

    newCont->bound_Args_गिनती = 0;
#  अगर d_m3HasExceptionHandling
    newCont->resumeThrow = i_cont->resumeThrow;
    i_cont->resumeThrow  = शून्य;
#  endif
    newCont->अंक_Suspend_Results = i_cont->अंक_Suspend_Results;
    क्रम (u32 i = 0; i < i_cont->अंक_Suspend_Results; ++i) {
        newCont->suspend_परिणाम_Offsets[i] = i_cont->suspend_परिणाम_Offsets[i];
        newCont->suspend_परिणाम_Is64[i]    = i_cont->suspend_परिणाम_Is64[i];
    }

    newCont->अंक_Handlers = i_cont->अंक_Handlers;
    newCont->handlersPC  = i_cont->handlersPC;
    newCont->parent      = i_cont->parent;
    newCont->state       = cont_suspended;

    i_cont->val_ढेर  = शून्य;
    i_cont->frames    = शून्य;
    i_cont->framesCap = 0;
    i_cont->अंक_Frames = 0;
    i_cont->state     = cont_consumed;

    वापस newCont;
}

// Only cont.new and snapshot restoration acquire stacks. Binding and suspension
// transfer them between records without changing the active count.
M3_परिणाम Continuation_Acquire_ढेर (IM3Runtime runtime, IM3Continuation cont)
{
    अगर (not Continuation_Can_Acquire_ढेर(runtime)) {
        वापस m3Err_continuationLimitExceeded;
    }
    अगर (runtime->val_ढेर_Pool_गिनती) {
        cont->val_ढेर = runtime->val_ढेर_Pool[--runtime->val_ढेर_Pool_गिनती];
        स_रखो(cont->val_ढेर, 0, (cont->अंक_ढेर_Slots + 4) * माप(m3slot_t));
    } अन्यथा {
        cont->val_ढेर = m3_AllocArray(m3slot_t, cont->अंक_ढेर_Slots + 4);
    }
    अगर (not cont->val_ढेर) {
        वापस m3Err_mallocFailed;
    }
    cont->sp = cont->val_ढेर;
    runtime->continuationsAllocated++;
    वापस m3Err_none;
}

व्योम Continuation_Recycle_ढेर (IM3Runtime runtime, IM3Continuation cont)
{
    अगर (not cont->val_ढेर or cont == runtime->मूल_Continuation) {
        वापस;
    }
    अगर (not runtime->val_ढेर_Pool) {
        runtime->val_ढेर_Pool = m3_AllocArray(m3slot_t*, 16);
        अगर (runtime->val_ढेर_Pool) {
            runtime->val_ढेर_Pool_Cap = 16;
        }
    }
    अगर (runtime->val_ढेर_Pool_गिनती < runtime->val_ढेर_Pool_Cap and
        (not runtime->continuationsLimit or runtime->val_ढेर_Pool_गिनती <
                                              runtime->continuationsLimit - runtime->continuationsAllocated + 1)) {
        runtime->val_ढेर_Pool[runtime->val_ढेर_Pool_गिनती++] = cont->val_ढेर;
        cont->val_ढेर                                      = शून्य;
    } अन्यथा {
        m3_Free(cont->val_ढेर);
    }
    cont->sp = शून्य;
    runtime->continuationsAllocated--;
}

व्योम Continuation_ReleaseAll (IM3Runtime io_runtime)
{
    // Records live until teardown: a consumed reference can still be in a table
    // or on a Wasm stack. Its value stack has already been recycled.
    IM3Continuation cont = io_runtime->continuations;

    जबतक (cont) {
        IM3Continuation अग्र = cont->अग्र;

        m3_Free(cont->val_ढेर);
        m3_Free(cont->frames);
        m3_Free(cont);

        cont = अग्र;
    }

    क्रम (u32 i = 0; i < io_runtime->val_ढेर_Pool_गिनती; ++i) {
        m3_Free(io_runtime->val_ढेर_Pool[i]);
    }
    m3_Free(io_runtime->val_ढेर_Pool);
    io_runtime->val_ढेर_Pool_गिनती = io_runtime->val_ढेर_Pool_Cap = 0;
    io_runtime->continuationsAllocated                          = 0;
    io_runtime->continuations                                   = शून्य;
    io_runtime->activeContinuation                              = शून्य;
    io_runtime->मूल_Continuation                                = शून्य;
}


m3ret_t Continuation_RecordFrame (IM3Runtime i_runtime, स्थिर M3Frame* i_frame)
{
    IM3Continuation cont = i_runtime->activeContinuation;

    // the marker only travels between a suspend and the resume that runs it,
    // and that whole stretch has an active continuation
    d_m3Assert(cont);

    अगर (cont->अंक_Frames == cont->framesCap) {
        u32 newCap = cont->framesCap ? cont->framesCap * 2 : 8;

        अगर (newCap > d_m3ContinuationMaxFrames) {
            newCap = d_m3ContinuationMaxFrames;
        }

        अगर (newCap == cont->framesCap) {
            वापस m3_Err_trap_ढेर_Overflow;
        }

        M3Frame* frames = m3_ReallocArray(M3Frame, cont->frames, newCap, cont->framesCap);

        अगर (not frames) {
            वापस m3Err_mallocFailed;
        }

        cont->frames    = frames;
        cont->framesCap = newCap;
    }

    cont->frames[cont->अंक_Frames++] = *i_frame;

    वापस m3Err_continuationSuspended;
}

#पूर्ण_अगर // d_m3HasStackSwitching

M3_परिणाम EvaluateExpression (IM3Module i_module, व्योम* o_expressed, m3type_t i_type, bytes_t* io_bytes, cbytes_t i_end)
{
    M3_परिणाम परिणाम = m3Err_none;

    // OPTZ: use a simplified interpreter for expressions

    // create a temporary runtime context
#अगर defined(d_m3PreferStaticAlloc)
    अटल M3Runtime runtime;
#अन्यथा
    M3Runtime runtime;
#पूर्ण_अगर
    M3_INIT(runtime);

    runtime.environment   = i_module->runtime->environment;
    runtime.अंक_ढेर_Slots = i_module->runtime->अंक_ढेर_Slots;
    runtime.ढेर         = i_module->runtime->ढेर;

    m3stack_t ढेर = (m3stack_t)runtime.ढेर;

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

    अगर (o->page) {
        IM3FuncType ftype = शून्य;

#अगर d_m3HasTypedRefs
        // A reference type has to be checked as the type it is: the shared
        // return types are one per storage class, so borrowing one of those
        // would let any reference at all satisfy a particular one.
        अगर (IsSpelledRefType(i_type)) {
            परिणाम = AllocFuncType(&ftype, 1);

            अगर (not परिणाम) {
                ftype->अंक_Rets  = 1;
                ftype->types[0] = i_type;

                परिणाम = Environment_AddFuncType(runtime.environment, &ftype);
            }
        } अन्यथा
#पूर्ण_अगर
        {
            ftype = runtime.environment->retFuncTypes[BaseTypeOf(i_type)];
        }

        pc_t m3code = GetPagePC(o->page);

        अगर (not परिणाम) {
            परिणाम = CompileExpression(o, ftype);
        }

        अगर (not परिणाम && o->max_ढेर_Slots >= runtime.अंक_ढेर_Slots) {
            परिणाम = m3_Err_trap_ढेर_Overflow;
        }

        अगर (not परिणाम) {
#अगर (d_m3EnableOpProfiling || d_m3EnableOpTracing)
            m3ret_t r = RunCode(m3code, ढेर, शून्य, d_m3OpDefaultArgs, d_m3BaseCstr);
#अन्यथा
            m3ret_t r = RunCode(m3code, ढेर, शून्य, d_m3OpDefaultArgs);
#पूर्ण_अगर

            अगर (r == 0) {                                                                   m3log (runtime, "expression result: %s", SPrint_मूल्य (ढेर, i_type));
                अगर (आकार_Of_Type(BaseTypeOf(i_type)) == माप(u32)) {
                    *(u32*)o_expressed = *((u32*)ढेर);
                } अन्यथा {
                    *(u64*)o_expressed = *((u64*)ढेर);
                }
            }
        }

        ReleaseCodePage(&runtime, o->page);
    } अन्यथा {
        परिणाम = m3Err_mallocFailedCodePage;
    }

    runtime.origin_ढेर = शून्य;        // prevent free(stack) in ReleaseRuntime
    Runtime_Release(&runtime);
    i_module->runtime = savedRuntime;

    *io_bytes = o->wasm;

    वापस परिणाम;
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
अटल
bool LimitsSatisfy (u64 i_exported_आकार, bool i_exportedHasMax, u64 i_exportedMax,
                    u64 i_importMin, bool i_importHasMax, u64 i_importMax)
{
    अगर (i_exported_आकार < i_importMin) {
        वापस false;
    }

    अगर (i_importHasMax and (not i_exportedHasMax or i_exportedMax > i_importMax)) {
        वापस false;
    }

    वापस true;
}


// The export of this kind under this name, or NULL. The index it carries is
// resolved by the caller, against the module's index space as it stands then -
// after linking, a re-exported import's slot holds the entity it was linked to.
अटल
स्थिर M3Export* Module_FindExport (IM3Module i_module, u8 i_kind, cstr_t i_name)
{
    क्रम (u32 i = 0; i < i_module->अंक_Exports; ++i) {
        स्थिर M3Export* e = &i_module->exports[i];

        अगर (e->kind == i_kind and म_भेद(e->name, i_name) == 0) {
            वापस e;
        }
    }

    वापस शून्य;
}


अटल
IM3Function Module_FindExportedFunction (IM3Module i_module, cstr_t i_name)
{
    स्थिर M3Export* e = Module_FindExport(i_module, d_externalKind_function, i_name);

    // A module that re-exports an import names the placeholder here. Resolve to
    // the function that actually runs, or a host-side call would run it against
    // the importing module's memory.
    वापस e ? Function_Implementation(&i_module->functions[e->index]) : शून्य;
}


अटल
IM3Memory Module_FindExportedMemory (IM3Module i_module, cstr_t i_name)
{
    स्थिर M3Export* e = Module_FindExport(i_module, d_externalKind_memory, i_name);

    वापस e ? i_module->memories[e->index] : शून्य;
}


अटल
IM3Table Module_FindExportedTable (IM3Module i_module, cstr_t i_name)
{
    स्थिर M3Export* e = Module_FindExport(i_module, d_externalKind_table, i_name);

    वापस e ? i_module->tables[e->index] : शून्य;
}


अटल
IM3Global Module_FindExportedGlobal (IM3Module i_module, cstr_t i_name)
{
    स्थिर M3Export* e = Module_FindExport(i_module, d_externalKind_global, i_name);

    वापस e ? &i_module->globals[e->index] : शून्य;
}


#अगर d_m3HasExceptionHandling || d_m3_Has_ढेर_Switching
अटल
IM3Tag Module_FindExportedTag (IM3Module i_module, cstr_t i_name)
{
    स्थिर M3Export* e = Module_FindExport(i_module, d_externalKind_tag, i_name);

    अगर (not e) {
        वापस शून्य;
    }

    IM3Tag tag = &i_module->tags[e->index];

    वापस tag->resolved ? tag->resolved : tag;
}
#पूर्ण_अगर


// Whether the module exports anything at all under this name. Export names are
// unique within a module, so a name one of the lookups above missed but this
// one finds is exported as something else - a kind the import cannot be
// satisfied by, rather than a name the module never exported.
अटल
bool Module_HasExport (IM3Module i_module, cstr_t i_name)
{
    क्रम (u32 i = 0; i < i_module->अंक_Exports; ++i) {
        अगर (म_भेद(i_module->exports[i].name, i_name) == 0) {
            वापस true;
        }
    }

    वापस false;
}


// Points each of the module's imports at whatever already-loaded module exports
// it. Runs before anything is allocated or initialized: a memory import has to
// be resolved before InitMemory would give it pages of its own, and a global
// import before InitGlobals runs an initializer that reads it.
अटल
M3_परिणाम LinkImports (IM3Runtime io_runtime, IM3Module io_module)
{
    M3_परिणाम परिणाम = m3Err_none;

    क्रम (u32 i = 0; i < io_module->अंक_Functions; ++i) {
        IM3Function f = &io_module->functions[i];

        अगर (f->wasm or not (f->import.moduleUtf8 and f->import.fieldUtf8)) {
            जारी;
        }

        IM3Module from = m3_FindModule(io_runtime, f->import.moduleUtf8);
        अगर (not from) {
            जारी;
        }

        IM3Function exported = Module_FindExportedFunction(from, f->import.fieldUtf8);
        अगर (not exported) {
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

    क्रम (u32 i = 0; i < io_module->अंक_Memories; ++i) {
        IM3Memory memory = io_module->memories[i];

        अगर (not memory->imported or memory->owner != io_module) {
            जारी;
        }

        IM3Module from = m3_FindModule(io_runtime, memory->import.moduleUtf8);
        अगर (not from) {
            जारी;
        }

        IM3Memory exported = Module_FindExportedMemory(from, memory->import.fieldUtf8);
        अगर (not exported) {
            _throwif(m3Err_incompatibleImportType, Module_HasExport(from, memory->import.fieldUtf8));
            _throw(m3Err_unknownImport);
        }

        // the address type is part of the memory type, so an i64 memory does
        // not satisfy an i32 import, or the other way round - and so is the page
        // size, which custom page sizes made a declared property of a memory
        _throwif(m3Err_incompatibleImportType, exported->isMemory64 != memory->isMemory64);
        _throwif(m3Err_incompatibleImportType, Memory_Page_आकार(exported) != Memory_Page_आकार(memory));

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

    क्रम (u32 i = 0; i < io_module->अंक_Tables; ++i) {
        IM3Table table = io_module->tables[i];

        अगर (not table->imported or table->owner != io_module) {
            जारी;
        }

        IM3Module from = m3_FindModule(io_runtime, table->import.moduleUtf8);
        अगर (not from) {
            जारी;
        }

        IM3Table exported = Module_FindExportedTable(from, table->import.fieldUtf8);
        अगर (not exported) {
            _throwif(m3Err_incompatibleImportType, Module_HasExport(from, table->import.fieldUtf8));
            _throw(m3Err_unknownImport);
        }

        _throwif(m3Err_incompatibleImportType, exported->type != table->type);

        // the index type is part of the table type, the same way it is for a memory
        _throwif(m3Err_incompatibleImportType, exported->isTable64 != table->isTable64);

        _throwif(m3Err_incompatibleImportType,
                 not LimitsSatisfy(exported->आकार, exported->hasMax, exported->max_आकार,
                                   table->init_आकार, table->hasMax, table->max_आकार));

        // hand the slot over to the exporter's table, and drop the placeholder
        m3_Free(table->elements);
        FreeImportInfo(&table->import);
        m3_Free(table);

        io_module->tables[i] = exported;
    }

    क्रम (u32 i = 0; i < io_module->अंक_Globals; ++i) {
        IM3Global g = &io_module->globals[i];

        अगर (not g->imported or not (g->import.moduleUtf8 and g->import.fieldUtf8)) {
            जारी;
        }

        IM3Module from = m3_FindModule(io_runtime, g->import.moduleUtf8);
        अगर (not from) {
            जारी;
        }

        IM3Global exported = Module_FindExportedGlobal(from, g->import.fieldUtf8);
        अगर (not exported) {
            _throwif(m3Err_incompatibleImportType, Module_HasExport(from, g->import.fieldUtf8));
            _throw(m3Err_unknownImport);
        }

        _throwif(m3Err_incompatibleImportType, exported->type != g->type);
        _throwif(m3Err_incompatibleImportType, exported->isMutable != g->isMutable);

        g->resolved = exported->resolved ? exported->resolved : exported;
    }

#अगर d_m3HasExceptionHandling || d_m3_Has_ढेर_Switching
    क्रम (u32 i = 0; i < io_module->अंक_Tags; ++i) {
        IM3Tag tag = &io_module->tags[i];

        अगर (not tag->imported or not (tag->import.moduleUtf8 and tag->import.fieldUtf8)) {
            जारी;
        }

        IM3Module from = m3_FindModule(io_runtime, tag->import.moduleUtf8);
        अगर (not from) {
            जारी;
        }

        IM3Tag exported = Module_FindExportedTag(from, tag->import.fieldUtf8);
        अगर (not exported) {
            _throwif(m3Err_incompatibleImportType, Module_HasExport(from, tag->import.fieldUtf8));
            _throw(m3Err_unknownImport);
        }

        _throwif(m3Err_incompatibleImportType, exported->type != tag->type);

        tag->resolved = exported->resolved ? exported->resolved : exported;
    }
#पूर्ण_अगर

    _catch: वापस परिणाम;
}


// Backs each of the module's memories with pages. LinkImports has already
// pointed any import it could satisfy at the exporting module's memory, and
// those are skipped here. An import nothing satisfied is still backed locally
// from its own declared limits, so that the module remains loadable.
M3_परिणाम InitMemory (IM3Runtime io_runtime, IM3Module i_module)
{
    M3_परिणाम परिणाम = m3Err_none;

    // Fixed from here on: the index space stops changing after parse, and
    // linking has already repointed any slot it was going to.
    i_module->memory0 = i_module->अंक_Memories ? i_module->memories[0]
                                              : &i_module->emptyMemory;

    अगर (i_module->अंक_Memories == 0) {
        // nothing addressable, but _mem still has to point somewhere
        i_module->emptyMemory.owner    = i_module;
        i_module->emptyMemory.page_आकार = d_m3_Default_Mem_Page_आकार;

_       (ResizeMemory(io_runtime, &i_module->emptyMemory, 0));
    }

    क्रम (u32 i = 0; i < i_module->अंक_Memories; ++i) {
        IM3Memory memory = i_module->memories[i];

        // a slot that already points at another module's memory is that
        // module's to allocate
        अगर (memory->owner != i_module or memory->mallocated) {
            जारी;
        }

        u32 page_आकार = Memory_Page_आकार(memory);

        memory->page_आकार = page_आकार;

        // Without a declared maximum a memory may grow to the spec limit of
        // 2^|addrtype|/pagesize pages, which is the usual 65536 at the default
        // page size, 2^48 for a 64-bit memory, and a whole address space of
        // them when a page is a single byte. A declared maximum of zero is a
        // real limit, not the absence of one.
        //
        // 2^64/pagesize overflows a u64 only when a page is a single byte, and
        // a power-of-two page size divides the address space exactly, so the
        // 64-bit division below is 2^64/pagesize written so that it fits.
        अगर (not memory->hasMax) {
            memory->maxPages = memory->isMemory64
                                 ? (page_आकार > 1 ? (UINT64_MAX / page_आकार) + 1 : UINT64_MAX)
                                 : (0x100000000ull / page_आकार);
        }

_       (ResizeMemory(io_runtime, memory, memory->initPages));
    }

    _catch: वापस परिणाम;
}


M3_परिणाम ResizeMemory (IM3Runtime io_runtime, IM3Memory memory, u64 i_अंक_Pages)
{
    M3_परिणाम परिणाम = m3Err_none;

    u64 अंक_Pages_To_Alloc = i_अंक_Pages;

    अगर (अंक_Pages_To_Alloc <= memory->maxPages) {
        // A 64-bit memory may ask for up to 2^48 pages, which overflows a u64
        // of bytes. Nothing that large can be backed, so refuse it up front
        // rather than multiplying into a wrapped size.
        _throwif("linear memory limitation exceeded",
                 अंक_Pages_To_Alloc > d_m3AddressLimit / memory->page_आकार);

        u64 अंक_Page_Bytes = अंक_Pages_To_Alloc * memory->page_आकार;

#अगर d_m3MaxLinearMemoryPages > 0
        // the limit is a memory size, counted in default-sized pages; comparing
        // it against a raw page count would make it 65536 times stricter for a
        // module whose pages are one byte
        _throwif("linear memory limitation exceeded",
                 अंक_Page_Bytes > (u64)d_m3MaxLinearMemoryPages * d_m3_Default_Mem_Page_आकार);
#पूर्ण_अगर

        // Limit the amount of memory that gets actually allocated
        अगर (io_runtime->memoryLimit) {
            अंक_Page_Bytes = M3_MIN(अंक_Page_Bytes, (u64)io_runtime->memoryLimit);
        }

        u64 old_लंबाई = memory->mallocated ? memory->mallocated->लंबाई : 0;
        u64 otherUsed = io_runtime->memoryBytesUsed - old_लंबाई;
        _throwif(m3Err_memoryLimitExceeded, io_runtime->memoryBytesLimit and
                                              (अंक_Page_Bytes > io_runtime->memoryBytesLimit or
                                               otherUsed > io_runtime->memoryBytesLimit - अंक_Page_Bytes));

        _throwif("linear memory limitation exceeded", अंक_Page_Bytes > (u64)SIZE_MAX - माप(M3MemoryHeader));

#अगर d_m3GuardedMemory

        // A slot holds every address a Wasm access can name, and nothing beyond
        // that: a memory larger than the data part of one would put addresses past
        // the guard, where a fault is nobody's to recognize
        _throwif("linear memory limitation exceeded", अंक_Page_Bytes > d_m3_Guarded_तथ्य_Bytes);

        // What makes an access past the end fault is the page after the memory not
        // being committed, so the end has to fall on one of the system's page
        // boundaries. A Wasm page divides evenly into them and always will; a memory
        // that declared a page size of its own may not, and then the bytes between
        // the end of the memory and the end of its last system page would answer
        // instead of trapping. Refusing to back such a memory is the honest way out:
        // the custom page sizes proposal exists for targets far too small to reserve
        // 8GiB per memory in the first place. See d_m3GuardedMemory.
        _throwif("guarded memory needs a page size the system's divides into",
                 (memory->page_आकार % m3_Host_Page_आकार()) != 0);

        अगर (memory->guardSlot == शून्य) {
            memory->guardSlot = Guard_TakeSlot();
            _throwifnull(memory->guardSlot);
        }

        // Growing only ever commits more of the same slot, so the memory never
        // moves - which the interpreter does not rely on, but nothing minds
        _throwif(m3Err_mallocFailed, not Guard_CommitSlot(memory->guardSlot, (माप_प्रकार)अंक_Page_Bytes));

        memory->mallocated = Guard_SlotHeader(memory->guardSlot);

#अन्यथा

        माप_प्रकार अंक_Bytes = (माप_प्रकार)अंक_Page_Bytes + माप(M3MemoryHeader);

        // What is really there, which is not numPages * pageSize: the limit above
        // caps the bytes without capping the page count, so a memory can be short
        // of a page - by design, since a whole 64 KiB page is often more than an
        // MCU has. Measuring the old block by the page count would overstate it,
        // and then realloc never sees the size it already has and copies the
        // whole limit again on every grow past it.
        माप_प्रकार अंक_Previous_Bytes = memory->mallocated ? memory->mallocated->लंबाई + माप(M3MemoryHeader) : 0;

        व्योम* newMem = m3_Realloc("Wasm Linear Memory", memory->mallocated, अंक_Bytes, अंक_Previous_Bytes);
        _throwifnull(newMem);

        memory->mallocated = (M3MemoryHeader*)newMem;

#पूर्ण_अगर

#अगर d_m3LogRuntime
        M3MemoryHeader* oldMallocated = memory->mallocated;
#पूर्ण_अगर

        io_runtime->memoryBytesUsed = otherUsed + अंक_Page_Bytes;
        memory->अंक_Pages            = अंक_Pages_To_Alloc;

        memory->mallocated->लंबाई  = (माप_प्रकार)अंक_Page_Bytes;
        memory->mallocated->runtime = io_runtime;
        memory->mallocated->memory  = memory;

        memory->mallocated->max_ढेर = (m3slot_t*)io_runtime->origin_ढेर + io_runtime->अंक_ढेर_Slots;

        m3log(runtime, "resized old: %p; mem: %p; length: %zu; pages: %llu", oldMallocated, memory->mallocated, memory->mallocated->लंबाई, (अचिन्हित दीर्घ दीर्घ)memory->अंक_Pages);
    } अन्यथा {
        परिणाम = m3Err_wasmMemoryOverflow;
    }

    _catch: वापस परिणाम;
}


व्योम FreeMemoryBlock (IM3Memory io_memory)
{
    अगर (io_memory->mallocated and io_memory->mallocated->runtime) {
        io_memory->mallocated->runtime->memoryBytesUsed -= io_memory->mallocated->लंबाई;
    }
#अगर d_m3GuardedMemory
    Guard_GiveSlot(io_memory->guardSlot);
    io_memory->guardSlot = शून्य;

    io_memory->mallocated = शून्य;
#अन्यथा
    m3_Free(io_memory->mallocated);
#पूर्ण_अगर
}


M3_परिणाम InitGlobals (IM3Module io_module)
{
    M3_परिणाम परिणाम = m3Err_none;

    अगर (io_module->अंक_Globals) {
        // placing the globals in their structs isn't good for cache locality, but i don't really know what the global
        // access patterns typically look like yet.

        //          io_module->globalMemory = m3Alloc (m3reg_t, io_module->numGlobals);

        //          if (io_module->globalMemory)
        {
            क्रम (u32 i = 0; i < io_module->अंक_Globals; ++i) {
                M3Global* g = &io_module->globals[i];                           m3log (runtime, "initializing global: %d", i);

                अगर (g->init_व्यंजक) {
                    bytes_t start = g->init_व्यंजक;

                    परिणाम = EvaluateExpression(io_module, &g->i64_मूल्य, g->type, &start, g->init_व्यंजक + g->init_व्यंजक_आकार);

                    अगर (not परिणाम) {
                        // io_module->globalMemory [i] = initValue;
                    } अन्यथा {
                        अवरोध;
                    }
                } अन्यथा {                                                        m3log (runtime, "importing global");
                }
            }
        }
        //          else result = ErrorModule (m3Err_mallocFailed, io_module, "could allocate globals for module: '%s", io_module->name);
    }

    वापस परिणाम;
}


M3_परिणाम Init_तथ्य_Segments (IM3Module io_module)
{
    M3_परिणाम परिणाम = m3Err_none;

    क्रम (u32 i = 0; i < io_module->अंक_तथ्य_Segments; ++i) {
        M3_तथ्य_Segment* segment = &io_module->तथ्य_Segments[i];

        // A passive segment stays available for memory.init until data.drop.
        // An active one is copied here and then counts as dropped.
        अगर (segment->isPassive) {
            जारी;
        }

        _throwif("data segment memory index out of range",
                 segment->memoryRegion >= io_module->अंक_Memories);

        IM3Memory io_memory = io_module->memories[segment->memoryRegion];

        _throwif("unallocated linear memory", !(io_memory->mallocated));

        // The offset expression has the memory's address type, and is
        // unsigned: an i32 offset of -1 is 4294967295, way out of bounds
        // rather than negative.
        u64     segmentOffset = 0;
        bytes_t start         = segment->init_व्यंजक;

        अगर (io_memory->isMemory64) {
_           (EvaluateExpression(io_module, &segmentOffset, c_m3Type_i64, &start, segment->init_व्यंजक + segment->init_व्यंजक_आकार));
        } अन्यथा {
            u32 offset32;
_           (EvaluateExpression(io_module, &offset32, c_m3Type_i32, &start, segment->init_व्यंजक + segment->init_व्यंजक_आकार));
            segmentOffset = offset32;
        }

        m3log(runtime, "loading data segment: %d; size: %d; offset: %llu", i, segment->आकार, (अचिन्हित दीर्घ दीर्घ)segmentOffset);

        अगर (segmentOffset <= io_memory->mallocated->लंबाई &&
            (u64)segment->आकार <= io_memory->mallocated->लंबाई - segmentOffset) {
            u8* dest = m3_Mem_तथ्य(io_memory->mallocated) + segmentOffset;
            स_नकल(dest, segment->तथ्य, segment->आकार);
        } अन्यथा {
            _throw("data segment out of bounds");
        }

        segment->dropped = true;
    }

    _catch: वापस परिणाम;
}


// Turns a segment's elements into references. Element expressions are constant
// expressions restricted to ref.null/ref.func, so they're read directly rather
// than run through the compiler.
अटल
M3_परिणाम ResolveElements (IM3Module io_module, M3ElementSegment* i_segment, व्योम** o_elements)
{
    M3_परिणाम परिणाम = m3Err_none;

    bytes_t  pos = i_segment->elements;
    cbytes_t end = io_module->elementSectionEnd;

    क्रम (u32 e = 0; e < i_segment->अंक_Elements; ++e) {
        u32   func_सूचकांक;
        व्योम* ref = शून्य;

        अगर (i_segment->is_व्यंजक) {
            m3opcode_t opcode;
_           (Read_opcode(&opcode, &pos, end));

            अगर (opcode == c_waOp_refFunc) {
_               (ReadLEB_u32(&func_सूचकांक, &pos, end));
                _throwif("function index out of range", func_सूचकांक >= io_module->अंक_Functions);
                ref = Function_Implementation(&io_module->functions[func_सूचकांक]);
            } अन्यथा अगर (opcode == c_waOp_refNull) {
                i8 waType;
                u8 nullType;
_               (ReadLEB_i7(&waType, &pos, end));
_               (NormalizeType(&nullType, waType));
                _throwif(m3Err_typeMismatch, nullType != i_segment->type);
            } अन्यथा अगर (opcode == c_waOp_getGlobal) {
                // wasm 2.0 lets an element expression read an imported
                // immutable global, which is how one module seeds another's
                // table with a reference it exported.
                u32 global_सूचकांक;
_               (ReadLEB_u32(&global_सूचकांक, &pos, end));
                _throwif(m3_Err_globa_सूचकांक_Out_Of_Bounds, global_सूचकांक >= io_module->अंक_Globals);

                IM3Global global = &io_module->globals[global_सूचकांक];

                _throwif(m3_Err_globa_सूचकांक_Out_Of_Bounds, not global->imported);
                _throwif(m3Err_wasmMalformed, global->isMutable);
                _throwif(m3Err_typeMismatch, BaseTypeOf(global->type) != BaseTypeOf(i_segment->type));

                // read the cell the import was linked to, not the placeholder
                अगर (global->resolved) {
                    global = global->resolved;
                }

                ref = global->ref_मूल्य;
            } अन्यथा {
                _throw("constant expression required");
            }

_           (Read_opcode(&opcode, &pos, end));
            _throwif(m3Err_wasmMalformed, opcode != c_waOp_end);
        } अन्यथा {
_           (ReadLEB_u32(&func_सूचकांक, &pos, end));
            _throwif("function index out of range", func_सूचकांक >= io_module->अंक_Functions);
            ref = Function_Implementation(&io_module->functions[func_सूचकांक]);
        }

        o_elements[e] = ref;
    }

    _catch: वापस परिणाम;
}


M3_परिणाम InitTableAndElements (IM3Module io_module)
{
    M3_परिणाम परिणाम = m3Err_none;

    cbytes_t end = io_module->elementSectionEnd;
    M3Table* table;

    क्रम (u32 i = 0; i < io_module->अंक_Tables; ++i) {
        table = io_module->tables[i];

        // a slot pointing at another module's table is that module's to fill
        अगर (table->owner != io_module) {
            जारी;
        }

        अगर (table->आकार) {
            IM3Runtime runtime = io_module->runtime;
            _throwif(m3Err_tableLimitExceeded, runtime->tableElementsLimit and
                                                 table->आकार > runtime->tableElementsLimit - runtime->tableElementsUsed);
            table->elements = m3_AllocArray(व्योम*, table->आकार);
            _throwifnull(table->elements);
            runtime->tableElementsUsed += table->आकार;

            अगर (table->init_व्यंजक) {
                व्योम*   मूल्य = शून्य;
                bytes_t start = table->init_व्यंजक;
_               (EvaluateExpression(io_module, &मूल्य, BaseTypeOf(table->type),
                                     &start, table->init_व्यंजक + table->init_व्यंजक_आकार));

                क्रम (u32 e = 0; e < table->आकार; ++e) {
                    table->elements[e] = मूल्य;
                }
            }
        }
    }

    क्रम (u32 i = 0; i < io_module->अंक_Element_Segments; ++i) {
        M3ElementSegment* segment = &io_module->elementSegments[i];

        // Declarative segments only make their functions referenceable, and
        // passive ones wait for table.init, so neither is written out here.
        अगर (segment->mode == c_m3Elem_declarative) {
            segment->dropped = true;
            जारी;
        }

        अगर (segment->mode == c_m3Elem_passive) {
            अगर (segment->अंक_Elements) {
                segment->resolved = m3_AllocArray(व्योम*, segment->अंक_Elements);
                _throwifnull(segment->resolved);
_               (ResolveElements(io_module, segment, segment->resolved));
            }
            जारी;
        }

        table = io_module->tables[segment->table_सूचकांक];

        // The offset expression has the table's index type, and is unsigned:
        // an i32 offset of -1 is 4294967295, out of bounds rather than negative.
        u64     offset = 0;
        bytes_t व्यंजक   = segment->init_व्यंजक;

        अगर (table->isTable64) {
_           (EvaluateExpression(io_module, &offset, c_m3Type_i64, &व्यंजक, end));
        } अन्यथा {
            u32 offset32;
_           (EvaluateExpression(io_module, &offset32, c_m3Type_i32, &व्यंजक, end));
            offset = offset32;
        }

        _throwif("out of bounds table access",
                 offset > table->आकार or segment->अंक_Elements > table->आकार - offset);

_       (ResolveElements(io_module, segment, table->elements + offset));

        segment->dropped = true;
    }

    _catch: वापस परिणाम;
}

M3_परिणाम m3_CompileModule (IM3Module io_module)
{
    M3_परिणाम परिणाम = m3Err_none;

    क्रम (u32 i = 0; i < io_module->अंक_Functions; ++i) {
        IM3Function f = &io_module->functions[i];
        अगर (f->wasm and not f->compiled) {
_           (CompileFunction(f));
        }
    }

    _catch: वापस परिणाम;
}

#अगर d_m3HasExceptionHandling

M3Exception* NewException (IM3Runtime io_runtime, IM3Tag i_tag, u32 i_अंक_Args)
{
    M3Exception* exception = (M3Exception*)m3_Malloc("M3Exception", माप(M3Exception) + i_अंक_Args * माप(u64));

    अगर (exception) {
        exception->tag     = i_tag;
        exception->अंक_Args = i_अंक_Args;
        exception->reified = false;
        exception->पूर्व    = शून्य;
        exception->अग्र    = io_runtime->exceptions;

        अगर (exception->अग्र) {
            exception->अग्र->पूर्व = exception;
        }

        io_runtime->exceptions = exception;
    }

    वापस exception;
}


// Releases one exception ahead of the rest. The caller has to know nothing can
// still name it: no exnref was ever taken of it, and its payload has already
// been copied out.
व्योम FreeException (IM3Runtime io_runtime, M3Exception* i_exception)
{
    अगर (i_exception->पूर्व) {
        i_exception->पूर्व->अग्र = i_exception->अग्र;
    } अन्यथा {
        io_runtime->exceptions = i_exception->अग्र;
    }

    अगर (i_exception->अग्र) {
        i_exception->अग्र->पूर्व = i_exception->पूर्व;
    }

    अगर (io_runtime->pendingException == i_exception) {
        io_runtime->pendingException = शून्य;
    }

    m3_Free_Impl(i_exception);
}


// Releases every exception the runtime still holds. Only safe once the Wasm
// stack is empty, which is why the outermost RunCodeChecked() is the one that
// calls it.
व्योम FreeExceptions (IM3Runtime io_runtime)
{
    M3Exception* exception = io_runtime->exceptions;

    io_runtime->exceptions       = शून्य;
    io_runtime->pendingException = शून्य;

    जबतक (exception) {
        M3Exception* अग्र = exception->अग्र;
        m3_Free_Impl(exception);
        exception = अग्र;
    }
}


// Keeps a copy of the exception a call just ended with, for m3_GetExceptionTag,
// or forgets the last one when the call ended any other way. A copy, because
// the original is freed with the rest before the outermost call returns - and
// one a host function's nested call ended with may still be named by an
// exnref further down the stack.
अटल
व्योम RecordUncaughtException (IM3Runtime io_runtime, M3_परिणाम i_परिणाम)
{
    अगर (io_runtime->uncaughtException) {
        m3_Free(io_runtime->uncaughtException);
    }

    M3Exception* exception = io_runtime->pendingException;

    अगर (M3_UNLIKELY(i_परिणाम == m3Err_trapUncaughtException) and exception) {
        M3Exception* copy = (M3Exception*)m3_CopyMem(exception, माप(M3Exception) + exception->अंक_Args * माप(u64));

        अगर (copy) {
            copy->अग्र = शून्य;
            copy->पूर्व = शून्य;
        }

        io_runtime->uncaughtException = copy;
    }
}

#पूर्ण_अगर // d_m3HasExceptionHandling


#अगर d_m3HasExceptionHandling || d_m3_Has_ढेर_Switching

IM3Tag m3_FindTag (IM3Module i_module, स्थिर अक्षर* स्थिर i_tagName)
{
    अगर (not i_module or not i_tagName) {
        वापस शून्य;
    }

    वापस Module_FindExportedTag(i_module, i_tagName);
}


M3_परिणाम m3_NewTag (IM3Runtime io_runtime, IM3Tag* o_tag, स्थिर अक्षर* स्थिर i_signature)
{
    IM3FuncType type    = शून्य;
    M3HostTag*  hostTag = शून्य;

_try {
    _throwif("null tag", not o_tag);
    *o_tag = शून्य;
    _throwif(m3Err_mallocFailed, not io_runtime);

_   (SignatureToFuncType(&type, i_signature));
    // tags are the same type exactly when their types are the same object
_   (Environment_AddFuncType(io_runtime->environment, &type));

    hostTag = m3_AllocStruct(M3HostTag);
    _throwifnull(hostTag);

    hostTag->tag.type    = type;
    hostTag->अग्र        = io_runtime->hostTags;
    io_runtime->hostTags = hostTag;

    *o_tag = &hostTag->tag;
} _catch:
    वापस परिणाम;
}


M3_परिणाम m3_LinkTag (IM3Module         io_module,
                     स्थिर अक्षर* स्थिर i_moduleName,
                     स्थिर अक्षर* स्थिर i_tagName,
                     IM3Tag            i_tag)
{
    अगर (not io_module or not i_moduleName or not i_tagName or not i_tag) {
        वापस m3Err_tagLookupFailed;
    }

    स्थिर bool wildcardModule = (म_भेद(i_moduleName, "*") == 0);
    IM3Tag     target         = i_tag->resolved ? i_tag->resolved : i_tag;

    M3_परिणाम परिणाम = m3Err_tagLookupFailed;

    क्रम (u32 i = 0; i < io_module->अंक_Tags; ++i) {
        IM3Tag tag = &io_module->tags[i];

        अगर (tag->imported and tag->import.moduleUtf8 and tag->import.fieldUtf8 and
            म_भेद(tag->import.fieldUtf8, i_tagName) == 0 and
            (wildcardModule or म_भेद(tag->import.moduleUtf8, i_moduleName) == 0)) {
            अगर (tag->type != target->type) {
                वापस m3Err_incompatibleImportType;
            }

            tag->resolved = target;
            परिणाम        = m3Err_none;
        }
    }

    वापस परिणाम;
}


uint32_t m3_Get_Tag_Arg_गिनती (IM3Tag i_tag)
{
    वापस i_tag ? i_tag->type->अंक_Args : 0;
}


M3_मूल्य_Type m3_GetTagArgType (IM3Tag i_tag, uint32_t i_सूचकांक)
{
    अगर (i_tag and i_सूचकांक < i_tag->type->अंक_Args) {
        वापस (M3_मूल्य_Type)BaseTypeOf(d_FuncArgType(i_tag->type, i_सूचकांक));
    }

    वापस c_m3Type_none;
}

#अन्यथा

IM3Tag m3_FindTag (IM3Module i_module, स्थिर अक्षर* स्थिर i_tagName)
{
    वापस शून्य;
}

M3_परिणाम m3_NewTag (IM3Runtime io_runtime, IM3Tag* o_tag, स्थिर अक्षर* स्थिर i_signature)
{
    अगर (o_tag) {
        *o_tag = शून्य;
    }
    वापस m3Err_tagLookupFailed;
}

M3_परिणाम m3_LinkTag (IM3Module io_module, स्थिर अक्षर* स्थिर i_moduleName, स्थिर अक्षर* स्थिर i_tagName, IM3Tag i_tag)
{
    वापस m3Err_tagLookupFailed;
}

uint32_t m3_Get_Tag_Arg_गिनती (IM3Tag i_tag)
{
    वापस 0;
}

M3_मूल्य_Type m3_GetTagArgType (IM3Tag i_tag, uint32_t i_सूचकांक)
{
    वापस c_m3Type_none;
}

#पूर्ण_अगर // d_m3HasExceptionHandling || d_m3HasStackSwitching


#अगर d_m3HasExceptionHandling

M3_परिणाम m3_ThrowException (IM3Runtime io_runtime, IM3Tag i_tag, uint32_t i_argc, स्थिर व्योम* i_argptrs[])
{
    अगर (not io_runtime or not i_tag) {
        वापस m3Err_tagLookupFailed;
    }

    IM3Tag      tag  = i_tag->resolved ? i_tag->resolved : i_tag;
    IM3FuncType type = tag->type;

    // a tag with results is a stack switching control tag, which is suspended
    // with rather than thrown
    अगर (type->अंक_Rets) {
        वापस m3Err_typeMismatch;
    }
    अगर (i_argc != type->अंक_Args) {
        वापस m3_Err_argument_गिनती_Mismatch;
    }
    // with nothing on the stack there is nothing to unwind, and the exception
    // would sit on the runtime's list until some later call ended
    अगर (io_runtime->callNesting == 0) {
        वापस "m3_ThrowException called outside a host function";
    }

    M3Exception* exception = NewException(io_runtime, tag, i_argc);
    अगर (not exception) {
        वापस m3Err_mallocFailed;
    }

    // the layout op_Throw gives a payload: 32-bit values zero-extended, floats
    // as their bit pattern
    क्रम (u32 i = 0; i < i_argc; ++i) {
        स्थिर व्योम* मूल्य = i_argptrs[i];

        चयन (BaseTypeOf(d_FuncArgType(type, i))) {
        हाल c_m3Type_i32:
        हाल c_m3Type_f32: {
            u32 bits;
            स_नकल(&bits, मूल्य, माप(bits));
            exception->args[i] = bits;
            अवरोध;
        }
        हाल c_m3Type_i64:
        हाल c_m3Type_f64:
            स_नकल(&exception->args[i], मूल्य, माप(u64));
            अवरोध;
        हाल c_m3Type_funcref:
        हाल c_m3Type_externref:
        हाल c_m3Type_exnref:
        हाल c_m3Type_contref:
            exception->args[i] = (u64)(*(स्थिर uintptr_t*)मूल्य);
            अवरोध;
        शेष:
            FreeException(io_runtime, exception);
            वापस "unknown argument type";
        }
    }

    io_runtime->pendingException = exception;

    वापस m3Err_pendingException;
}


IM3Tag m3_GetExceptionTag (IM3Runtime i_runtime)
{
    अगर (i_runtime and i_runtime->uncaughtException) {
        वापस i_runtime->uncaughtException->tag;
    }

    वापस शून्य;
}


M3_परिणाम m3_GetExceptionArgs (IM3Runtime i_runtime, uint32_t i_argc, स्थिर व्योम* o_argptrs[])
{
    अगर (not i_runtime or not i_runtime->uncaughtException) {
        वापस "no uncaught exception";
    }

    स्थिर M3Exception* exception = i_runtime->uncaughtException;
    IM3FuncType        type      = exception->tag->type;

    अगर (i_argc != exception->अंक_Args) {
        वापस m3_Err_argument_गिनती_Mismatch;
    }

    क्रम (u32 i = 0; i < i_argc; ++i) {
        व्योम* मूल्य = (व्योम*)o_argptrs[i];
        u64   bits  = exception->args[i];

        चयन (BaseTypeOf(d_FuncArgType(type, i))) {
        हाल c_m3Type_i32:
        हाल c_m3Type_f32: {
            u32 निचला = (u32)bits;
            स_नकल(मूल्य, &निचला, माप(निचला));
            अवरोध;
        }
        हाल c_m3Type_i64:
        हाल c_m3Type_f64:
            स_नकल(मूल्य, &bits, माप(bits));
            अवरोध;
        हाल c_m3Type_funcref:
        हाल c_m3Type_externref:
        हाल c_m3Type_exnref:
        हाल c_m3Type_contref:
            *(uintptr_t*)मूल्य = (uintptr_t)bits;
            अवरोध;
        शेष:
            वापस "unknown argument type";
        }
    }

    वापस m3Err_none;
}

#अन्यथा

M3_परिणाम m3_ThrowException (IM3Runtime io_runtime, IM3Tag i_tag, uint32_t i_argc, स्थिर व्योम* i_argptrs[])
{
    वापस m3Err_tagLookupFailed;
}

IM3Tag m3_GetExceptionTag (IM3Runtime i_runtime)
{
    वापस शून्य;
}

M3_परिणाम m3_GetExceptionArgs (IM3Runtime i_runtime, uint32_t i_argc, स्थिर व्योम* o_argptrs[])
{
    वापस "no uncaught exception";
}

#पूर्ण_अगर // d_m3HasExceptionHandling


#अगर d_m3GuardedMemory

// What RunCodeProtected hands to the platform layer to run, and what comes back
प्रकार काष्ठा M3ProtectedRun {
    pc_t            pc;
    IM3Runtime      runtime;
    M3MemoryHeader* mem;
    M3_परिणाम        परिणाम;
} M3ProtectedRun;

अटल
व्योम RunCodeBody (व्योम* io_context)
{
    M3ProtectedRun* run = (M3ProtectedRun*)io_context;

#  अगर (d_m3EnableOpProfiling || d_m3EnableOpTracing)
    run->परिणाम = (M3_परिणाम)RunCode(run->pc, (m3stack_t)run->runtime->ढेर, run->mem,
                                    d_m3OpDefaultArgs, d_m3BaseCstr);
#  अन्यथा
    run->परिणाम = (M3_परिणाम)RunCode(run->pc, (m3stack_t)run->runtime->ढेर, run->mem,
                                    d_m3OpDefaultArgs);
#  endif
}

// The bounds check the load and store operations no longer do, done once here by the
// system instead: an address past the end of a memory reaches the uncommitted part
// of that memory's reservation, and the fault comes back as the trap the spec asks
// for. Nothing is left half-done by it - the Wasm stack and the memories are heap,
// and the caller unwinds the rest.
अटल
M3_परिणाम RunCodeProtected (IM3Runtime i_runtime, pc_t i_pc, M3MemoryHeader* i_mem)
{
    // Memory operations in this build carry no bounds check, so code only runs where a
    // fault past the end of a memory can still be turned into a trap
    अगर (M3_UNLIKELY(not m3_HostGuardsActive())) {
        वापस ErrorRuntime(m3Err_memoryGuardsUnavailable, i_runtime,
                            "another part of the process holds the fault signal");
    }

    M3ProtectedRun run;
    run.pc      = i_pc;
    run.runtime = i_runtime;
    run.mem     = i_mem;
    run.परिणाम  = m3Err_none;

    व्योम*  arena;
    माप_प्रकार arenaBytes;
    Guard_ArenaRange(&arena, &arenaBytes);

    अगर (not m3_HostProtectedCall(RunCodeBody, &run, arena, arenaBytes)) {
        वापस m3Err_trapOutOfBoundsMemoryAccess;
    }

    वापस run.परिणाम;
}

#पूर्ण_अगर // d_m3GuardedMemory


#अगर d_m3_Has_ढेर_Switching

// The function a suspended continuation stopped in: the callee of its innermost
// call frame, or the function it was entered with if it made no call. Frames are
// recorded innermost first, so that is the lowest-numbered call frame.
अटल
IM3Function Continuation_InnermostFunction (IM3Continuation i_cont)
{
    क्रम (u32 i = 0; i < i_cont->अंक_Frames; ++i) {
        अगर (i_cont->frames[i].kind == frame_call && i_cont->frames[i].call.function) {
            वापस i_cont->frames[i].call.function;
        }
    }

    वापस i_cont->entryFunction;
}


// Where the host's next call puts its arguments and finds its results. A
// suspended root keeps its frames on the bottom of the runtime's stack, so a call
// made before it is resumed starts above the last of them rather than on top of
// them. The top is the innermost function's frame base plus everything that
// function can use. That base is the root's own sp, unless it stopped inside a
// resume: then the suspension point belongs to the resumed continuation, and the
// root's innermost frame is that resume.
व्योम Runtime_Place_Call_ढेर (IM3Runtime io_runtime)
{
    IM3Continuation मूल = io_runtime->मूल_Continuation;

    io_runtime->ढेर = io_runtime->origin_ढेर;

    अगर (not मूल or मूल->state != cont_suspended) {
        वापस;
    }

    m3stack_t   sp       = मूल->sp;
    IM3Function function = Continuation_InnermostFunction(मूल);

    अगर (मूल->अंक_Frames and मूल->frames[0].kind == frame_resume) {
        sp = मूल->frames[0].sp;
    }

    m3slot_t* base = (m3slot_t*)io_runtime->origin_ढेर;
    m3slot_t* शीर्ष  = (m3slot_t*)sp + (function ? function->max_ढेर_Slots : 0);

    अगर (शीर्ष > base + io_runtime->अंक_ढेर_Slots) {
        शीर्ष = base + io_runtime->अंक_ढेर_Slots;
    }

    io_runtime->ढेर = शीर्ष;
}

#पूर्ण_अगर // d_m3HasStackSwitching


// Run compiled code on the runtime's stack, bounding native recursion for the
// duration of the call. The outermost invocation establishes the stack limit;
// nested ones (an imported function calling back into Wasm) inherit it.
अटल अंतरभूत
M3_परिणाम RunCodeChecked (IM3Runtime i_runtime, IM3Function i_function)
{
    pc_t i_pc = i_function->compiled;

    // execution runs against the memory of the module the entry point belongs
    // to, not against some runtime-wide one
    M3MemoryHeader* _mem = Module_MemoryHeader(i_function->module);

#अगर d_m3HasSnapshots
    अगर (M3_UNLIKELY(i_function->module->isUnusable)) {
        वापस "a snapshot failed to restore into the module, so it cannot run";
    }
    i_function->module->hasRun = true;
#पूर्ण_अगर

    d_m3_ढेर_Limit_Enter(i_runtime);

    // a host function calling back into Wasm does not change which call a trap
    // ends: that is still the one the host made first
    अगर (i_runtime->callNesting++ == 0) {
        i_runtime->entered = i_function;
    }
#अगर d_m3HasExceptionHandling
    // handler stacks don't nest across a call boundary: a host function calling
    // back into Wasm cannot be caught by a try_table its own caller entered
    u32 savedTryDepth   = i_runtime->tryDepth;
    i_runtime->tryDepth = 0;
#पूर्ण_अगर
#अगर d_m3_Has_ढेर_Switching
    // nor do continuations: the native frames on the far side of a host call
    // are not ours to record, so a suspend in here has no handler to reach and
    // no resume to unwind to
    IM3Continuation savedContinuation = i_runtime->activeContinuation;

    // A suspended root continuation is a paused program waiting to be resumed,
    // not scratch space. A call the host makes before resuming it gets a
    // context of its own, the way any other re-entry does, rather than having
    // the frames of the suspension reset out from under it.
    bool claim_मूल = i_runtime->isSuspendable and not savedContinuation and
                     not (i_runtime->मूल_Continuation and
                          i_runtime->मूल_Continuation->state == cont_suspended);

    अगर (claim_मूल and not i_runtime->मूल_Continuation) {
        i_runtime->मूल_Continuation = Continuation_New(i_runtime, i_function->funcType, i_function);

        अगर (i_runtime->मूल_Continuation) {
            // The root borrows the runtime's stack and is not charged as cont.new.
            i_runtime->मूल_Continuation->अंक_ढेर_Slots = i_runtime->अंक_ढेर_Slots;
        } अन्यथा {
            claim_मूल = false;
        }
    }

    अगर (claim_मूल) {
        i_runtime->activeContinuation              = i_runtime->मूल_Continuation;
        i_runtime->मूल_Continuation->entryFunction = i_function;
        i_runtime->मूल_Continuation->state         = cont_running;
        i_runtime->मूल_Continuation->अंक_Frames     = 0;
    } अन्यथा {
        i_runtime->activeContinuation = शून्य;
    }
#पूर्ण_अगर
#अगर d_m3GuardedMemory
    M3_परिणाम परिणाम = RunCodeProtected(i_runtime, i_pc, _mem);
#या_अगर (d_m3EnableOpProfiling || d_m3EnableOpTracing)
    M3_परिणाम परिणाम = (M3_परिणाम)RunCode(i_pc, (m3stack_t)i_runtime->ढेर, _mem, d_m3OpDefaultArgs, d_m3BaseCstr);
#अन्यथा
    M3_परिणाम परिणाम = (M3_परिणाम)RunCode(i_pc, (m3stack_t)i_runtime->ढेर, _mem, d_m3OpDefaultArgs);
#पूर्ण_अगर
#अगर d_m3_Has_ढेर_Switching
    अगर (claim_मूल) {
        i_runtime->मूल_Continuation->state =
          (परिणाम == m3Err_continuationSuspended) ? cont_suspended : cont_returned;

        Runtime_Place_Call_ढेर(i_runtime);
    }
    i_runtime->activeContinuation = savedContinuation;
#पूर्ण_अगर
#अगर d_m3HasExceptionHandling
    i_runtime->tryDepth = savedTryDepth;

    // an exception that reached the bottom of the call stack found no handler
    अगर (M3_UNLIKELY(परिणाम == m3Err_pendingException)) {
        परिणाम = m3Err_trapUncaughtException;
    }
    RecordUncaughtException(i_runtime, परिणाम);

    // a paused invocation can still reach the exceptions it holds, so they wait
    // for it to finish
    अगर (i_runtime->callNesting == 1 and not m3_IsSuspended(i_runtime)) {
        FreeExceptions(i_runtime);
    }
#पूर्ण_अगर
    i_runtime->callNesting--;
    d_m3_ढेर_Limit_Leave(i_runtime);

    वापस परिणाम;
}

M3_परिणाम m3_RunStart (IM3Module io_module)
{
#अगर_घोषित FUZZING_BUILD_MODE_UNSAFE_FOR_PRODUCTION
    // Execution disabled for fuzzing builds
    वापस m3Err_none;
#पूर्ण_अगर

    M3_परिणाम परिणाम           = m3Err_none;
    i32      startFunctionTmp = -1;

    अगर (io_module and io_module->startFunction >= 0) {
        IM3Function function = &io_module->functions[io_module->startFunction];

        अगर (not function->compiled) {
_           (CompileFunction(function));
        }

        IM3FuncType ftype = function->funcType;
        अगर (ftype->अंक_Args != 0 || ftype->अंक_Rets != 0) {
            _throw(m3_Err_argument_गिनती_Mismatch);
        }

        IM3Module  module  = function->module;
        IM3Runtime runtime = module->runtime;

        startFunctionTmp = io_module->startFunction;

        io_module->startFunction = -1;

        परिणाम = RunCodeChecked(runtime, function);

        // a start function that suspended is not to be started again: resuming
        // is what finishes it
        अगर (परिणाम and परिणाम != m3Err_continuationSuspended) {
            io_module->startFunction = startFunctionTmp;
            EXCEPTION_PRINT(परिणाम);
            जाओ _catch;
        }
    }

    _catch: वापस परिणाम;
}

// TODO: deal with main + side-modules loading efforcement
M3_परिणाम m3_LoadModule (IM3Runtime io_runtime, IM3Module io_module)
{
    M3_परिणाम परिणाम = m3Err_none;

    अगर (M3_UNLIKELY(io_module->runtime)) {
        वापस m3Err_moduleAlreadyLinked;
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
_   (Init_तथ्य_Segments(io_module));

    // Start func might use imported functions, which are not liked here yet,
    // so it will be called before a function call is attempted (in m3_FindFunction)

#अगर_घोषित DEBUG
    Module_GenerateNames(io_module);
#पूर्ण_अगर

    io_module->अग्र     = io_runtime->modules;
    io_runtime->modules = io_module;
    वापस परिणाम; // ok

_catch:
    // The runtime owns the module either way. Instantiation may already have
    // written this module's functions into a table another module owns, and the
    // spec keeps whatever it managed to do before the trap, so those entries
    // stay callable - retaining the module is what stops them dangling.
    //
    // It goes on the tail of the list rather than the head: it is not a module
    // anyone should find by name, only one whose functions may still be
    // reachable through someone else's table.
    io_module->अग्र = शून्य;

    IM3Module* पुच्छ = &io_runtime->modules;
    जबतक (*पुच्छ) {
        पुच्छ = &(*पुच्छ)->अग्र;
    }
    *पुच्छ = io_module;

    वापस परिणाम;
}

IM3Global m3_FindGlobal (IM3Module         io_module,
                         स्थिर अक्षर* स्थिर i_globalName)
{
    // Search exports
    IM3Global exported = Module_FindExportedGlobal(io_module, i_globalName);
    अगर (exported) {
        // a re-exported global is the one that was imported, so reads and
        // writes have to reach the cell that actually holds the value
        वापस exported->resolved ? exported->resolved : exported;
    }

    // Search imports
    क्रम (u32 i = 0; i < io_module->अंक_Globals; ++i) {
        IM3Global g = &io_module->globals[i];

        अगर (g->import.moduleUtf8 and g->import.fieldUtf8) {
            अगर (म_भेद(g->import.fieldUtf8, i_globalName) == 0) {
                वापस g->resolved ? g->resolved : g;
            }
        }
    }
    वापस शून्य;
}

M3_परिणाम m3_GetGlobal (IM3Global      i_global,
                       IM3_Tagged_मूल्य o_मूल्य)
{
    अगर (not i_global) {
        वापस m3Err_globalLookupFailed;
    }

    चयन (i_global->type) {
    हाल c_m3Type_i32: o_मूल्य->मूल्य.i32 = i_global->i32_मूल्य; अवरोध;
    हाल c_m3Type_i64: o_मूल्य->मूल्य.i64 = i_global->i64_मूल्य; अवरोध;
#अगर d_m3HasFloat
    हाल c_m3Type_f32: o_मूल्य->मूल्य.f32 = i_global->f32_मूल्य; अवरोध;
    हाल c_m3Type_f64: o_मूल्य->मूल्य.f64 = i_global->f64_मूल्य; अवरोध;
#पूर्ण_अगर
    शेष: वापस m3Err_invalidTypeId;
    }

    o_मूल्य->type = (M3_मूल्य_Type)(i_global->type);
    वापस m3Err_none;
}

M3_परिणाम m3_SetGlobal (IM3Global            i_global,
                       स्थिर IM3_Tagged_मूल्य i_मूल्य)
{
    अगर (not i_global) {
        वापस m3Err_globalLookupFailed;
    }
    अगर (not i_global->isMutable) {
        वापस m3Err_globalNotMutable;
    }
    अगर (i_global->type != i_मूल्य->type) {
        वापस m3Err_globalTypeMismatch;
    }

    चयन (i_मूल्य->type) {
    हाल c_m3Type_i32: i_global->i32_मूल्य = i_मूल्य->मूल्य.i32; अवरोध;
    हाल c_m3Type_i64: i_global->i64_मूल्य = i_मूल्य->मूल्य.i64; अवरोध;
#अगर d_m3HasFloat
    हाल c_m3Type_f32: i_global->f32_मूल्य = i_मूल्य->मूल्य.f32; अवरोध;
    हाल c_m3Type_f64: i_global->f64_मूल्य = i_मूल्य->मूल्य.f64; अवरोध;
#पूर्ण_अगर
    शेष: वापस m3Err_invalidTypeId;
    }

    वापस m3Err_none;
}

M3_मूल्य_Type m3_GetGlobalType (IM3Global i_global)
{
    वापस (i_global) ? (M3_मूल्य_Type)(i_global->type) : c_m3Type_none;
}


व्योम* v_FindFunction (IM3Module i_module, व्योम* i_info)
{
    स्थिर अक्षर* स्थिर i_name = (स्थिर अक्षर*)i_info;

    // Prefer exported functions
    IM3Function exported = Module_FindExportedFunction(i_module, i_name);
    अगर (exported) {
        वापस exported;
    }

    // Search internal functions
    क्रम (u32 i = 0; i < i_module->अंक_Functions; ++i) {
        IM3Function f = &i_module->functions[i];

        bool isImported = f->import.moduleUtf8 or f->import.fieldUtf8;

        अगर (isImported) {
            जारी;
        }

        क्रम (पूर्णांक j = 0; j < f->अंक_Names; j++) {
            अगर (f->names[j] and म_भेद(f->names[j], i_name) == 0) {
                वापस f;
            }
        }
    }

    वापस शून्य;
}


// Shared tail of the two lookups: a function is only usable once it has code.
अटल
M3_परिणाम PrepareFoundFunction (IM3Function* o_function, IM3Function i_function)
{
    M3_परिणाम परिणाम = m3Err_none;

    अगर (not i_function->compiled) {
_       (CompileFunction(i_function))
    }

    _catch:
    *o_function = परिणाम ? शून्य : i_function;

    वापस परिणाम;
}


// Searches every module in the runtime, most recently loaded first. That is a
// guess once more than one module is loaded and two of them export the same
// name - m3_FindFunctionIn says which module is meant.
M3_परिणाम m3_FindFunction (IM3Function* o_function, IM3Runtime i_runtime, स्थिर अक्षर* स्थिर i_functionName)
{
                                                                d_m3Assert (o_function and i_runtime and i_functionName);
    IM3Function function = शून्य;

    अगर (not i_runtime->modules) {
        *o_function = शून्य;
        वापस "no modules loaded";
    }

    function = (IM3Function)ForEachModule(i_runtime, v_FindFunction, (व्योम*)i_functionName);

    अगर (not function) {
        *o_function = शून्य;
        वापस ErrorModule(m3Err_functionLookupFailed, i_runtime->modules, "'%s'", i_functionName);
    }

    वापस PrepareFoundFunction(o_function, function);
}


IM3Module m3_FindModule (IM3Runtime i_runtime, स्थिर अक्षर* स्थिर i_moduleName)
{
    अगर (not i_runtime or not i_moduleName) {
        वापस शून्य;
    }

    // the list is newest-first, so a name registered twice names the newer one
    क्रम (IM3Module m = i_runtime->modules; m; m = m->अग्र) {
        अगर (m->name and म_भेद(m->name, i_moduleName) == 0) {
            वापस m;
        }
    }

    वापस शून्य;
}


// Searches one module's exports, which is what naming a module means.
M3_परिणाम m3_FindFunctionIn (IM3Function* o_function, IM3Module i_module, स्थिर अक्षर* स्थिर i_functionName)
{
                                                                d_m3Assert (o_function and i_functionName);
    अगर (not i_module) {
        *o_function = शून्य;
        वापस m3Err_functionLookupFailed;      // ErrorModule would deref it
    }

    IM3Function function = (IM3Function)v_FindFunction(i_module, (व्योम*)i_functionName);

    अगर (not function) {
        *o_function = शून्य;
        वापस ErrorModule(m3Err_functionLookupFailed, i_module, "'%s'", i_functionName);
    }

    वापस PrepareFoundFunction(o_function, function);
}


M3_परिणाम m3_GetTableFunction (IM3Function* o_function, IM3Module i_module, uint32_t i_सूचकांक)
{
_try {
    M3Table*    table;
    IM3Function function;

    _throwif("no table", i_module->अंक_Tables == 0);

    table = i_module->tables[0];
    _throwif("function index out of range", i_सूचकांक >= table->आकार);

    function = (IM3Function)table->elements[i_सूचकांक];

    अगर (function) {
        अगर (not function->compiled) {
_           (CompileFunction(function))
        }
    }

    *o_function = function;
} _catch:
    वापस परिणाम;
}


अटल
M3_परिणाम checkStartFunction (IM3Module i_module)
{
    M3_परिणाम परिणाम = m3Err_none;                               d_m3Assert(i_module);

    // Check if start function needs to be called
    अगर (i_module->startFunction >= 0) {
        परिणाम = m3_RunStart(i_module);
    }

    वापस परिणाम;
}

uint32_t m3_Get_Arg_गिनती (IM3Function i_function)
{
    अगर (i_function) {
        IM3FuncType ft = i_function->funcType;
        अगर (ft) {
            वापस ft->अंक_Args;
        }
    }
    वापस 0;
}

uint32_t m3_Get_Ret_गिनती (IM3Function i_function)
{
    अगर (i_function) {
        IM3FuncType ft = i_function->funcType;
        अगर (ft) {
            वापस ft->अंक_Rets;
        }
    }
    वापस 0;
}


M3_मूल्य_Type m3_GetArgType (IM3Function i_function, uint32_t index)
{
    अगर (i_function) {
        IM3FuncType ft = i_function->funcType;
        अगर (ft and index < ft->अंक_Args) {
            वापस (M3_मूल्य_Type)BaseTypeOf(d_FuncArgType(ft, index));
        }
    }
    वापस c_m3Type_none;
}

M3_मूल्य_Type m3_GetRetType (IM3Function i_function, uint32_t index)
{
    अगर (i_function) {
        IM3FuncType ft = i_function->funcType;
        अगर (ft and index < ft->अंक_Rets) {
            वापस (M3_मूल्य_Type)BaseTypeOf(d_FuncRetType(ft, index));
        }
    }
    वापस c_m3Type_none;
}


// The call APIs marshal their arguments into the runtime stack before any
// compiled code runs, so op_Entry's overflow check comes too late to cover those
// writes - and for a host function or an import linked to another module there
// is no op_Entry of our own to reach at all. Bound the area about to be written
// here instead: results and arguments each take a u64 at this boundary, however
// wide the slot is inside. Every caller goes through this, so none of them can
// be the one that forgets.
अटल
M3_परिणाम Get_ढेर_Pointer_For_Args (IM3Function i_function, u8** o_ढेर)
{
    IM3Runtime  runtime = i_function->module->runtime;
    IM3FuncType ftype   = i_function->funcType;

    // the stack a paused invocation holds is not available to this one
    माप_प्रकार used      = (माप_प्रकार)((u8*)runtime->ढेर - (u8*)runtime->origin_ढेर);
    माप_प्रकार needed    = ((माप_प्रकार)ftype->अंक_Rets + ftype->अंक_Args) * माप(u64);
    माप_प्रकार available = (माप_प्रकार)runtime->अंक_ढेर_Slots * माप(m3slot_t) - used;

    अगर (needed > available) {
        वापस m3_Err_trap_ढेर_Overflow;
    }

    u64* ढेर = (u64*)runtime->ढेर;

    ढेर += ftype->अंक_Rets;

    *o_ढेर = (u8*)ढेर;

    वापस m3Err_none;
}


M3_परिणाम m3_CallV (IM3Function i_function, ...)
{
    बहु_सूची ap;
    बहु_शुरू(ap, i_function);
    M3_परिणाम r = m3_CallVL(i_function, ap);
    बहु_पूर्ण(ap);
    वापस r;
}

अटल
व्योम Report_Native_ढेर_Usage ()
{
#अगर d_m3_Log_Native_ढेर
    पूर्णांक ढेर_Used = m3_ढेर_Get_Max();
    ख_लिखो(मानक_त्रुटि, "Native stack used: %d\n", ढेर_Used);
#पूर्ण_अगर
}


M3_परिणाम m3_CallVL (IM3Function i_function, बहु_सूची i_args)
{
    IM3Runtime  runtime = i_function->module->runtime;
    IM3FuncType ftype   = i_function->funcType;
    M3_परिणाम    परिणाम  = m3Err_none;
    u8*         s       = शून्य;

    अगर (!i_function->compiled) {
        वापस m3Err_missingCompiledCode;
    }

#अगर d_m3RecordBacktraces
    ClearBacktrace(runtime);
#पूर्ण_अगर

    m3_ढेर_Check_Init();

_   (checkStartFunction(i_function->module))

_   (Get_ढेर_Pointer_For_Args(i_function, &s))

    क्रम (u32 i = 0; i < ftype->अंक_Args; ++i) {
        चयन (d_FuncArgType(ftype, i)) {
        हाल c_m3Type_i32:
            *(i32*)(s) = बहु_तर्क(i_args, i32);
            s += 8;
            अवरोध;
        हाल c_m3Type_i64:
            *(i64*)(s) = बहु_तर्क(i_args, i64);
            s += 8;
            अवरोध;
        हाल c_m3Type_funcref:
        हाल c_m3Type_externref:
        हाल c_m3Type_exnref:
            *(uintptr_t*)(s) = बहु_तर्क(i_args, uintptr_t);
            s += 8;
            अवरोध;
#अगर d_m3HasFloat
        हाल c_m3Type_f32:
            *(f32*)(s) = (f32)बहु_तर्क(i_args, f64);
            s += 8;
            अवरोध; // f32 is passed as f64
        हाल c_m3Type_f64:
            *(f64*)(s) = बहु_तर्क(i_args, f64);
            s += 8;
            अवरोध;
#पूर्ण_अगर
        शेष: वापस "unknown argument type";
        }
    }

    परिणाम = RunCodeChecked(runtime, i_function);
    Report_Native_ढेर_Usage();

    runtime->lastCalled = परिणाम ? शून्य : i_function;

    _catch: वापस परिणाम;
}

M3_परिणाम m3_Call (IM3Function i_function, uint32_t i_argc, स्थिर व्योम* i_argptrs[])
{
    IM3Runtime  runtime = i_function->module->runtime;
    IM3FuncType ftype   = i_function->funcType;
    M3_परिणाम    परिणाम  = m3Err_none;
    u8*         s       = शून्य;

    अगर (i_argc != ftype->अंक_Args) {
        वापस m3_Err_argument_गिनती_Mismatch;
    }
    अगर (!i_function->compiled) {
        वापस m3Err_missingCompiledCode;
    }

#अगर d_m3RecordBacktraces
    ClearBacktrace(runtime);
#पूर्ण_अगर

    m3_ढेर_Check_Init();

_   (checkStartFunction(i_function->module))

_   (Get_ढेर_Pointer_For_Args(i_function, &s))

    क्रम (u32 i = 0; i < ftype->अंक_Args; ++i) {
        चयन (d_FuncArgType(ftype, i)) {
        हाल c_m3Type_i32:
            *(i32*)(s) = *(i32*)i_argptrs[i];
            s += 8;
            अवरोध;
        हाल c_m3Type_i64:
            *(i64*)(s) = *(i64*)i_argptrs[i];
            s += 8;
            अवरोध;
        हाल c_m3Type_funcref:
        हाल c_m3Type_externref:
        हाल c_m3Type_exnref:
            *(uintptr_t*)(s) = *(uintptr_t*)i_argptrs[i];
            s += 8;
            अवरोध;
#अगर d_m3HasFloat
        हाल c_m3Type_f32:
            *(f32*)(s) = *(f32*)i_argptrs[i];
            s += 8;
            अवरोध;
        हाल c_m3Type_f64:
            *(f64*)(s) = *(f64*)i_argptrs[i];
            s += 8;
            अवरोध;
#पूर्ण_अगर
        शेष: वापस "unknown argument type";
        }
    }

    परिणाम = RunCodeChecked(runtime, i_function);

    Report_Native_ढेर_Usage();

    runtime->lastCalled = परिणाम ? शून्य : i_function;

    _catch: वापस परिणाम;
}

// Argument parsing for m3_CallArgv. Strict on purpose: the whole string has to
// be consumed, so "12abc" is rejected rather than read as 12, and an empty or
// unparsable argument is an error rather than the zero that strtoul with a
// NULL end pointer used to hand back. See wasm3/wasm3#367.
अटल
M3_परिणाम ParseArgInteger (ccstr_t i_arg, u32 i_अंक_Bits, u64* o_मूल्य)
{
    अगर (not i_arg or not *i_arg) {
        वापस "empty argument";
    }

    // strtoull would skip leading space, but trailing space is rejected below;
    // accepting one and not the other would just be confusing
    अगर (है_खाली((अचिन्हित अक्षर)*i_arg)) {
        वापस "argument is not a number";
    }

    अक्षर* end = शून्य;
    u64   मूल्य;

    त्रुटि_सं = 0;

    // an argument may be spelled signed or unsigned: -1 and 4294967295 name the
    // same i32
    अगर (*i_arg == '-') {
        i64 signed_मूल्य = strtoll(i_arg, &end, 10);

        अगर (i_अंक_Bits == 32 and (signed_मूल्य < INT32_MIN or signed_मूल्य > INT32_MAX)) {
            वापस "argument out of range";
        }

        मूल्य = (u64)signed_मूल्य;
    } अन्यथा {
        मूल्य = strtoull(i_arg, &end, 10);

        अगर (i_अंक_Bits == 32 and मूल्य > UINT32_MAX) {
            वापस "argument out of range";
        }
    }

    अगर (त्रुटि_सं == दुस्फल) {
        वापस "argument out of range";
    }

    अगर (end == i_arg or *end) {
        वापस "argument is not a number";
    }

    *o_मूल्य = मूल्य;

    वापस m3Err_none;
}


#अगर d_m3HasFloat
अटल
M3_परिणाम ParseArgFloat (ccstr_t i_arg, f64* o_मूल्य)
{
    अगर (not i_arg or not *i_arg) {
        वापस "empty argument";
    }

    अगर (है_खाली((अचिन्हित अक्षर)*i_arg)) {
        वापस "argument is not a number";
    }

    अक्षर* end = शून्य;

    त्रुटि_सं = 0;

    f64 मूल्य = म_से_भग्न(i_arg, &end);

    अगर (end == i_arg or *end) {
        वापस "argument is not a number";
    }

    // strtod reports underflow through ERANGE as well, and a denormal result is
    // perfectly usable, so only an overflow to infinity is out of range
    अगर (त्रुटि_सं == दुस्फल and (मूल्य > द्विग_उच्च or मूल्य < -द्विग_उच्च)) {
        वापस "argument out of range";
    }

    *o_मूल्य = मूल्य;

    वापस m3Err_none;
}
#पूर्ण_अगर


// A reference argument is either the null reference or a host handle written as
// an integer.
अटल
M3_परिणाम ParseArgReference (ccstr_t i_arg, u64* o_मूल्य)
{
    अगर (i_arg and म_भेद(i_arg, "null") == 0) {
        *o_मूल्य = 0;
        वापस m3Err_none;
    }

    वापस ParseArgInteger(i_arg, 64, o_मूल्य);
}


M3_परिणाम m3_CallArgv (IM3Function i_function, uint32_t i_argc, स्थिर अक्षर* i_argv[])
{
    IM3FuncType ftype   = i_function->funcType;
    IM3Runtime  runtime = i_function->module->runtime;
    M3_परिणाम    परिणाम  = m3Err_none;
    u8*         s       = शून्य;

    अगर (i_argc != ftype->अंक_Args) {
        वापस m3_Err_argument_गिनती_Mismatch;
    }
    अगर (!i_function->compiled) {
        वापस m3Err_missingCompiledCode;
    }

#अगर d_m3RecordBacktraces
    ClearBacktrace(runtime);
#पूर्ण_अगर

    m3_ढेर_Check_Init();

_   (checkStartFunction(i_function->module))

_   (Get_ढेर_Pointer_For_Args(i_function, &s))

    क्रम (u32 i = 0; i < ftype->अंक_Args; ++i) {
        u64 मूल्य = 0;
#अगर d_m3HasFloat
        f64 fvalue = 0;
#पूर्ण_अगर
        चयन (d_FuncArgType(ftype, i)) {
        हाल c_m3Type_i32:
_           (ParseArgInteger(i_argv[i], 32, &मूल्य)) * (i32*)(s) = (i32)मूल्य;
            s += 8;
            अवरोध;
        हाल c_m3Type_i64:
_           (ParseArgInteger(i_argv[i], 64, &मूल्य)) * (i64*)(s) = (i64)मूल्य;
            s += 8;
            अवरोध;
        हाल c_m3Type_funcref:
        हाल c_m3Type_externref:
        हाल c_m3Type_exnref:
_           (ParseArgReference(i_argv[i], &मूल्य)) * (uintptr_t*)(s) = (uintptr_t)मूल्य;
            s += 8;
            अवरोध;
#अगर d_m3HasFloat
            // strtof would be less portable
        हाल c_m3Type_f32:
_           (ParseArgFloat(i_argv[i], &fvalue)) * (f32*)(s) = (f32)fvalue;
            s += 8;
            अवरोध;
        हाल c_m3Type_f64:
_           (ParseArgFloat(i_argv[i], &fvalue)) * (f64*)(s) = fvalue;
            s += 8;
            अवरोध;
#पूर्ण_अगर
        शेष: _throw("unknown argument type");
        }
    }

    परिणाम = RunCodeChecked(runtime, i_function);

    Report_Native_ढेर_Usage();

    runtime->lastCalled = परिणाम ? शून्य : i_function;

    _catch: वापस परिणाम;
}


//u8 * AlignStackPointerTo64Bits (const u8 * i_stack)
//{
//    uintptr_t ptr = (uintptr_t) i_stack;
//    return (u8 *) ((ptr + 7) & ~7);
//}


M3_परिणाम m3_GetResults (IM3Function i_function, uint32_t i_retc, स्थिर व्योम* o_retptrs[])
{
    IM3FuncType ftype   = i_function->funcType;
    IM3Runtime  runtime = i_function->module->runtime;

    अगर (i_retc != ftype->अंक_Rets) {
        वापस m3_Err_argument_गिनती_Mismatch;
    }
    अगर (i_function != runtime->lastCalled) {
        वापस "function not called";
    }

    u8* s = (u8*)runtime->ढेर;

    क्रम (u32 i = 0; i < ftype->अंक_Rets; ++i) {
        चयन (d_FuncRetType(ftype, i)) {
        हाल c_m3Type_i32:
            *(i32*)o_retptrs[i] = *(i32*)(s);
            s += 8;
            अवरोध;
        हाल c_m3Type_i64:
            *(i64*)o_retptrs[i] = *(i64*)(s);
            s += 8;
            अवरोध;
        हाल c_m3Type_funcref:
        हाल c_m3Type_externref:
        हाल c_m3Type_exnref:
            *(uintptr_t*)o_retptrs[i] = *(uintptr_t*)(s);
            s += 8;
            अवरोध;
#अगर d_m3HasFloat
        हाल c_m3Type_f32:
            *(f32*)o_retptrs[i] = *(f32*)(s);
            s += 8;
            अवरोध;
        हाल c_m3Type_f64:
            *(f64*)o_retptrs[i] = *(f64*)(s);
            s += 8;
            अवरोध;
#पूर्ण_अगर
        शेष: वापस "unknown return type";
        }
    }
    वापस m3Err_none;
}

M3_परिणाम m3_GetResultsV (IM3Function i_function, ...)
{
    बहु_सूची ap;
    बहु_शुरू(ap, i_function);
    M3_परिणाम r = m3_GetResultsVL(i_function, ap);
    बहु_पूर्ण(ap);
    वापस r;
}

M3_परिणाम m3_GetResultsVL (IM3Function i_function, बहु_सूची o_rets)
{
    IM3Runtime  runtime = i_function->module->runtime;
    IM3FuncType ftype   = i_function->funcType;

    अगर (i_function != runtime->lastCalled) {
        वापस "function not called";
    }

    u8* s = (u8*)runtime->ढेर;
    क्रम (u32 i = 0; i < ftype->अंक_Rets; ++i) {
        चयन (d_FuncRetType(ftype, i)) {
        हाल c_m3Type_i32:
            *बहु_तर्क(o_rets, i32*) = *(i32*)(s);
            s += 8;
            अवरोध;
        हाल c_m3Type_i64:
            *बहु_तर्क(o_rets, i64*) = *(i64*)(s);
            s += 8;
            अवरोध;
        हाल c_m3Type_funcref:
        हाल c_m3Type_externref:
        हाल c_m3Type_exnref:
            *बहु_तर्क(o_rets, uintptr_t*) = *(uintptr_t*)(s);
            s += 8;
            अवरोध;
#अगर d_m3HasFloat
        हाल c_m3Type_f32:
            *बहु_तर्क(o_rets, f32*) = *(f32*)(s);
            s += 8;
            अवरोध;
        हाल c_m3Type_f64:
            *बहु_तर्क(o_rets, f64*) = *(f64*)(s);
            s += 8;
            अवरोध;
#पूर्ण_अगर
        शेष: वापस "unknown argument type";
        }
    }
    वापस m3Err_none;
}

व्योम ReleaseCodePageNoTrack (IM3Runtime i_runtime, IM3CodePage i_codePage)
{
    अगर (i_codePage) {
        IM3CodePage* list;

        bool pageFull = (अंक_Free_पंक्तियाँ(i_codePage) < d_m3_Code_Page_Free_पंक्तियाँ_Threshold);
        अगर (pageFull) {
            list = &i_runtime->pagesFull;
        } अन्यथा {
            list = &i_runtime->pagesOpen;
        }

        धकेलो_Code_Page(list, i_codePage);                         m3log (emit, "release page: %d to queue: '%s'", i_codePage->info.sequence, pageFull ? "full" : "open")
    }
}


IM3CodePage AcquireCodePageWithCapacity (IM3Runtime i_runtime, u32 i_min_पंक्ति_गिनती)
{
    IM3CodePage page = RemoveCodePageOfCapacity(&i_runtime->pagesOpen, i_min_पंक्ति_गिनती);

    अगर (not page) {
        page = Environment_AcquireCodePage(i_runtime->environment, i_min_पंक्ति_गिनती);

        अगर (not page) {
            page = NewCodePage(i_runtime, i_min_पंक्ति_गिनती);
        }

        अगर (page) {
            i_runtime->अंक_Code_Pages++;
        }
    }

    अगर (page) {                                                  m3log (emit, "acquire page: %d", page->info.sequence);
        i_runtime->अंक_Active_Code_Pages++;
    }

    वापस page;
}


IM3CodePage AcquireCodePage (IM3Runtime i_runtime)
{
    वापस AcquireCodePageWithCapacity(i_runtime, d_m3_Code_Page_Free_पंक्तियाँ_Threshold);
}


व्योम ReleaseCodePage (IM3Runtime i_runtime, IM3CodePage i_codePage)
{
    अगर (i_codePage) {
        ReleaseCodePageNoTrack(i_runtime, i_codePage);
        i_runtime->अंक_Active_Code_Pages--;

#अगर defined(DEBUG)
        u32 अंक_Open = गिनती_Code_Pages(i_runtime->pagesOpen);
        u32 अंक_Full = गिनती_Code_Pages(i_runtime->pagesFull);

        m3log(runtime, "runtime: %p; open-pages: %d; full-pages: %d; active: %d; total: %d", i_runtime, अंक_Open, अंक_Full, i_runtime->अंक_Active_Code_Pages, i_runtime->अंक_Code_Pages);

        d_m3Assert(अंक_Open + अंक_Full + i_runtime->अंक_Active_Code_Pages == i_runtime->अंक_Code_Pages);

#  अगर d_m3LogCodePages
        dump_code_page(i_codePage, /* startPC: */ शून्य);
#  endif
#पूर्ण_अगर
    }
}


#अगर d_m3VerboseErrorMessages
M3_परिणाम m3Error (M3_परिणाम i_परिणाम, IM3Runtime i_runtime, IM3Module i_module, IM3Function i_function,
                  स्थिर अक्षर* स्थिर i_file, u32 i_पंक्ति_अंक, स्थिर अक्षर* स्थिर i_errorMessage, ...)
{
    अगर (i_runtime) {
        i_runtime->error         = (M3ErrorInfo){ .परिणाम = i_परिणाम, .runtime = i_runtime, .module = i_module, .function = i_function, .file = i_file, .पंक्ति_2 = i_पंक्ति_अंक };
        i_runtime->error.message = i_runtime->error_message;

        बहु_सूची args;
        बहु_शुरू(args, i_errorMessage);
        vsnprintf(i_runtime->error_message, माप(i_runtime->error_message), i_errorMessage, args);
        बहु_पूर्ण(args);
    }

    वापस i_परिणाम;
}
#पूर्ण_अगर


व्योम m3_GetErrorInfo (IM3Runtime i_runtime, M3ErrorInfo* o_info)
{
    अगर (i_runtime) {
        *o_info = i_runtime->error;
        m3_ResetErrorInfo(i_runtime);
    }
}


व्योम m3_ResetErrorInfo (IM3Runtime i_runtime)
{
    अगर (i_runtime) {
        M3_INIT(i_runtime->error);
        i_runtime->error.message = "";
    }
}

uint8_t* m3_GetMemory (IM3Module i_module, माप_प्रकार* o_memory_आकार_In_Bytes, uint32_t i_memory_सूचकांक)
{
    uint8_t* memory = शून्य;
    माप_प्रकार   आकार   = 0;

    अगर (i_module and i_memory_सूचकांक < i_module->अंक_Memories) {
        IM3Memory mem = i_module->memories[i_memory_सूचकांक];

        अगर (mem->mallocated) {
            आकार = mem->mallocated->लंबाई;

            अगर (आकार) {
                memory = m3_Mem_तथ्य(mem->mallocated);
            }
        }
    }

    अगर (o_memory_आकार_In_Bytes) {
        *o_memory_आकार_In_Bytes = आकार;
    }

    वापस memory;
}


माप_प्रकार m3_Get_Memory_आकार (IM3Module i_module, uint32_t i_memory_सूचकांक)
{
    अगर (not i_module or i_memory_सूचकांक >= i_module->अंक_Memories) {
        वापस 0;
    }

    IM3Memory mem = i_module->memories[i_memory_सूचकांक];

    वापस mem->mallocated ? mem->mallocated->लंबाई : 0;
}


माप_प्रकार m3_Get_Memory_आकार_At (स्थिर व्योम* i_memory)
{
    अगर (not i_memory) {
        वापस 0;
    }

    // the header sits immediately before the data it describes
    स्थिर M3MemoryHeader* header = ((स्थिर M3MemoryHeader*)i_memory) - 1;

    वापस header->लंबाई;
}


M3_परिणाम m3_FindExportedMemory (IM3Module i_module, cstr_t i_name, u32* o_memory_सूचकांक)
{
    अगर (not i_module or not i_name or not o_memory_सूचकांक) {
        वापस m3Err_unknownMemory;
    }

    स्थिर M3Export* e = Module_FindExport(i_module, d_externalKind_memory, i_name);

    अगर (not e) {
        वापस m3Err_unknownMemory;
    }

    *o_memory_सूचकांक = e->index;
    वापस m3Err_none;
}


bool Module_HasLinkedHostImport (IM3Module i_module, cstr_t i_importModule)
{
    क्रम (u32 i = 0; i < i_module->अंक_Functions; ++i) {
        IM3Function f = &i_module->functions[i];

        // hostMemory is set when a host function is bound to the import, so it
        // is also the mark of one - see CompileRawFunction
        अगर (not f->hostMemory or f->wasm or f->resolved) {
            जारी;
        }

        अगर (f->import.moduleUtf8 and म_भेद(f->import.moduleUtf8, i_importModule) == 0) {
            वापस true;
        }
    }

    वापस false;
}


M3_परिणाम m3_BindImportMemory (IM3Module io_module, cstr_t i_importModule, u32 i_memory_सूचकांक)
{
    अगर (not io_module or not i_importModule) {
        वापस m3Err_moduleNotLinked;
    }

    अगर (i_memory_सूचकांक >= io_module->अंक_Memories) {
        वापस m3Err_unknownMemory;
    }

    // the memory itself, not the index: linking may have pointed this slot at
    // another module's memory, and growing one reallocates behind it
    IM3Memory memory = io_module->memories[i_memory_सूचकांक];

    स्थिर bool wildcardModule = (म_भेद(i_importModule, "*") == 0);

    क्रम (u32 i = 0; i < io_module->अंक_Functions; ++i) {
        IM3Function f = &io_module->functions[i];

        // only an import that a host function was bound to has a memory to
        // pin: one with a body of its own reads memory through the interpreter,
        // and one resolved to another module's export runs that module's code
        अगर (not f->hostMemory or f->wasm or f->resolved) {
            जारी;
        }

        अगर (wildcardModule or (f->import.moduleUtf8 and म_भेद(f->import.moduleUtf8, i_importModule) == 0)) {
            f->hostMemory = memory;
        }
    }

    वापस m3Err_none;
}


M3BacktraceInfo* m3_GetBacktrace (IM3Runtime i_runtime)
{
#अगर d_m3RecordBacktraces
    वापस &i_runtime->backtrace;
#अन्यथा
    वापस शून्य;
#पूर्ण_अगर
}


//-------------------------------------------------------------------------------------------------------------------------------
//  snapshots & suspendable execution
//-------------------------------------------------------------------------------------------------------------------------------

व्योम m3_SetSuspendable (IM3Runtime io_runtime, bool i_suspendable)
{
    अगर (io_runtime) {
        io_runtime->isSuspendable = i_suspendable;
    }
}

व्योम m3_RequestSuspend (IM3Runtime io_runtime)
{
    अगर (io_runtime) {
        io_runtime->suspendRequested = true;
    }
}

bool m3_IsSuspended (IM3Runtime i_runtime)
{
    अगर (!i_runtime) {
        वापस false;
    }
#अगर d_m3_Has_ढेर_Switching
    अगर (i_runtime->मूल_Continuation && i_runtime->मूल_Continuation->state == cont_suspended) {
        वापस true;
    }
#पूर्ण_अगर
    वापस false;
}

M3_परिणाम m3_ResumeRuntime (IM3Runtime io_runtime)
{
    अगर (!io_runtime) {
        वापस m3Err_mallocFailed;
    }
    अगर (!io_runtime->isSuspendable) {
        वापस m3Err_none;
    }
#अगर d_m3HasSnapshots
    // a restore that failed leaves nothing to resume, and says so every time
    // rather than resuming nothing successfully
    अगर (io_runtime->मूल_Continuation and io_runtime->मूल_Continuation->entryFunction and
        io_runtime->मूल_Continuation->entryFunction->module->isUnusable) {
        वापस "a snapshot failed to restore into the module, so it cannot run";
    }
#पूर्ण_अगर
#अगर d_m3_Has_ढेर_Switching
    अगर (!io_runtime->मूल_Continuation || io_runtime->मूल_Continuation->state != cont_suspended) {
        वापस m3Err_none;
    }
    // what finishes returns into the bottom of the stack, where it was entered
    io_runtime->ढेर = io_runtime->origin_ढेर;

    // this is the outermost call again, as far as a host function calling back
    // into Wasm from inside it can tell
    io_runtime->entered = io_runtime->मूल_Continuation->entryFunction;
    io_runtime->callNesting++;

    d_m3_ढेर_Limit_Enter(io_runtime);
    io_runtime->resumePastCheck = true;
    M3_परिणाम परिणाम             = (M3_परिणाम)ResumeContinuation(io_runtime, io_runtime->मूल_Continuation);
    io_runtime->resumePastCheck = false;
    d_m3_ढेर_Limit_Leave(io_runtime);

#  अगर d_m3HasExceptionHandling
    अगर (M3_UNLIKELY(परिणाम == m3Err_pendingException)) {
        परिणाम = m3Err_trapUncaughtException;
    }
    RecordUncaughtException(io_runtime, परिणाम);
#  endif

    io_runtime->callNesting--;

#  अगर d_m3HasExceptionHandling
    अगर (परिणाम != m3Err_continuationSuspended and io_runtime->callNesting == 0) {
        FreeExceptions(io_runtime);
    }
#  endif

    // the results are where the paused call left room for them, so a finished
    // resume can be asked for them the way the call itself could have been
    io_runtime->lastCalled = (परिणाम == m3Err_none) ? io_runtime->मूल_Continuation->entryFunction : शून्य;

    Runtime_Place_Call_ढेर(io_runtime);
    वापस परिणाम;
#अन्यथा
    वापस m3Err_none;
#पूर्ण_अगर
}
