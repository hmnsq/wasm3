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
            لكل (u8 t = c_m3Type_none; t < c_m3_Type_ڳڻپ; t++) {
                إذا (t == c_m3Type_v128) {
                    استمر;
                }

                IM3FuncType ftype;
_               (AllocFuncType(&ftype, 1));

                ftype->انگ_دليلون  = 0;
                ftype->انگ_Rets  = (t == c_m3Type_none) ? 0 : 1;
                ftype->types[0] = t;

                Environment_جوڙيو_Func_Type(env, &ftype);

                env->retFuncTypes[t] = ftype;
            }
        }

        _catch:
        إذا (نتيجو) {
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
        IM3FuncType ايندڙ = ftype->ايندڙ;
        m3_Free(ftype);
        ftype = ايندڙ;
    }

    m3log(runtime, "freeing %d pages from environment", ڳڻپ_ڪوڊ_صفحا(i_environment->صفحا_Released));
    Free_ڪوڊ_صفحا(&i_environment->صفحا_Released);
}


فراغ m3_FreeEnvironment (IM3Environment i_environment)
{
    إذا (i_environment) {
        Environment_Release(i_environment);
        m3_Free(i_environment);
    }
}


فراغ m3_سيٽ_Custom_Section_Handler (IM3Environment i_environment, M3SectionHandler i_handler)
{
    إذا (i_environment) {
        i_environment->customSectionHandler = i_handler;
    }
}


// returns the same io_funcType or replaces it with an equivalent that's already in the type linked list
M3_نتيجو Environment_جوڙيو_Func_Type (IM3Environment i_environment, IM3FuncType* io_funcType)
{
    IM3FuncType جوڙيو_Type = *io_funcType;
    IM3FuncType newType = i_environment->funcTypes;

    طالما (newType) {
        إذا (not newType->اندريون_Rec_گروهه and AreFuncTypesEqual(newType, جوڙيو_Type)) {
            m3_Free(جوڙيو_Type);
            اكسر;
        }

        newType = newType->ايندڙ;
    }

    إذا (newType == عدم) {
        // a type index has to fit in the heap type field of an m3type_t
        إذا (i_environment->انگ_Func_Types >= d_m3_Max_Sane_Types_ڳڻپ) {
            m3_Free(جوڙيو_Type);
            *io_funcType = عدم;
            أرجع "too many distinct function types";
        }

        newType                  = جوڙيو_Type;
        newType->canonical_اشاريو  = i_environment->انگ_Func_Types++;
        newType->ايندڙ            = i_environment->funcTypes;
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
M3_نتيجو Environment_ReserveFuncTypes (IM3Environment i_environment, u32 i_ڳڻپ, u16* o_پهريون_اشاريو)
{
    إذا ((u64)i_environment->انگ_Func_Types + i_ڳڻپ > d_m3_Max_Sane_Types_ڳڻپ) {
        أرجع "too many distinct function types";
    }

    *o_پهريون_اشاريو = i_environment->انگ_Func_Types;
    i_environment->انگ_Func_Types += (u16)i_ڳڻپ;

    أرجع m3Err_none;
}


فراغ Environment_AdoptFuncType (IM3Environment i_environment, IM3FuncType i_funcType, u16 i_canonical_اشاريو)
{
    i_funcType->اندريون_Rec_گروهه     = true;
    i_funcType->canonical_اشاريو = i_canonical_اشاريو;
    i_funcType->ايندڙ           = i_environment->funcTypes;
    i_environment->funcTypes   = i_funcType;
}


IM3_ڪوڊ_صفحو Remove_ڪوڊ_صفحو_جو_صلاحيت (M3_ڪوڊ_صفحو** io_list, u32 i_minimum_سٽ_ڳڻپ)
{
    IM3_ڪوڊ_صفحو prev = عدم;
    IM3_ڪوڊ_صفحو صفحو = *io_list;

    طالما (صفحو) {
        إذا (انگ_Free_سٽون(صفحو) >= i_minimum_سٽ_ڳڻپ) {             d_m3Assert (صفحو->info.usage_ڳڻپ == 0);
            IM3_ڪوڊ_صفحو ايندڙ = صفحو->info.ايندڙ;
            إذا (prev) {
                prev->info.ايندڙ = ايندڙ; // mid-list
            } وإلا {
                *io_list = ايندڙ;       // front of list
            }

            اكسر;
        }

        prev = صفحو;
        صفحو = صفحو->info.ايندڙ;
    }

    أرجع صفحو;
}


IM3_ڪوڊ_صفحو Environment_Acquire_ڪوڊ_صفحو (IM3Environment i_environment, u32 i_minimum_سٽ_ڳڻپ)
{
    أرجع Remove_ڪوڊ_صفحو_جو_صلاحيت(&i_environment->صفحا_Released, i_minimum_سٽ_ڳڻپ);
}


فراغ Environment_Release_ڪوڊ_صفحا (IM3Environment i_environment, IM3_ڪوڊ_صفحو i_ڪوڊ_صفحو_List)
{
    IM3_ڪوڊ_صفحو پڇاڙي = i_ڪوڊ_صفحو_List;

    طالما (پڇاڙي) {
        پڇاڙي->info.سٽ_اشاريو = 0; // reset page
#إذا d_m3_رڪارڊ_Backtraces
        پڇاڙي->info.mapping->سائيز = 0;
#جيڪڏهن_پورو // d_m3RecordBacktraces

        IM3_ڪوڊ_صفحو ايندڙ = پڇاڙي->info.ايندڙ;
        إذا (not ايندڙ) {
            اكسر;
        }

        پڇاڙي = ايندڙ;
    }

    إذا (پڇاڙي) {
        // push list to front
        پڇاڙي->info.ايندڙ               = i_environment->صفحا_Released;
        i_environment->صفحا_Released = i_ڪوڊ_صفحو_List;
    }
}


IM3Runtime m3_NewRuntime (IM3Environment i_environment, u32 i_ڍير_سائيز_اندريون_بائيٽون, فراغ* i_userdata)
{
    IM3Runtime runtime = m3_AllocStruct(M3Runtime);

    إذا (runtime) {
        m3_ResetErrorInfo(runtime);

        runtime->environment = i_environment;
        runtime->userdata    = i_userdata;

        Deterministic_InitRuntime(runtime);

        runtime->origin_ڍير = m3_Malloc("Wasm Stack", i_ڍير_سائيز_اندريون_بائيٽون + 4 * حجم(m3slot_t)); // TODO: more precise stack checks

        إذا (runtime->origin_ڍير) {
            runtime->ڍير         = runtime->origin_ڍير;
            runtime->انگ_ڍير_Slots = i_ڍير_سائيز_اندريون_بائيٽون / حجم(m3slot_t);          m3log (runtime, "new stack: %p, slots: %u", runtime->origin_ڍير, runtime->انگ_ڍير_Slots);
        } وإلا {
            m3_Free(runtime);
        }
    }

    أرجع runtime;
}

فراغ m3_سيٽ_Validation (IM3Runtime i_runtime, bool i_enable)
{
#إذا d_m3EnableValidation
    إذا (i_runtime) {
        i_runtime->skipValidation = not i_enable;
    }
#وإلا
    (فراغ)i_runtime;
    (فراغ)i_enable;              // nothing to skip: the validator was compiled out
#جيڪڏهن_پورو
}

M3_نتيجو m3_سيٽ_Resource_حد_بندي (IM3Runtime i_runtime, M3_Resource_حد_بندي i_حد_بندي, uint64_t i_قدر)
{
    إذا (not i_runtime) {
        أرجع m3Err_mallocFailed;
    }
    بدل (i_حد_بندي) {
    حالة c_m3_حد_بندي_يادگيري_بائيٽون:
        إذا (i_قدر and i_قدر < i_runtime->يادگيري_بائيٽون_استعمال_ٿيل) {
            أرجع m3_Err_resource_حد_بندي_هيٺ_Usage;
        }
        i_runtime->يادگيري_بائيٽون_حد_بندي = i_قدر;
        أرجع m3Err_none;
    حالة c_m3_حد_بندي_جدول_Elements:
        إذا (i_قدر and i_قدر < i_runtime->جدول_Elements_استعمال_ٿيل) {
            أرجع m3_Err_resource_حد_بندي_هيٺ_Usage;
        }
        i_runtime->جدول_Elements_حد_بندي = i_قدر;
        أرجع m3Err_none;
#إذا d_m3_Has_ڍير_Switching
    حالة c_m3_حد_بندي_Continuations:
        إذا (i_قدر and i_قدر < i_runtime->continuationsAllocated) {
            أرجع m3_Err_resource_حد_بندي_هيٺ_Usage;
        }
        i_runtime->continuations_حد_بندي = (u32)M3_MIN(i_قدر, (u64)UINT32_MAX);
        طالما (i_قدر and i_runtime->val_ڍير_Pool_ڳڻپ > i_قدر - i_runtime->continuationsAllocated) {
            m3_Free(i_runtime->val_ڍير_Pool[--i_runtime->val_ڍير_Pool_ڳڻپ]);
        }
        أرجع m3Err_none;
#وإلا
    حالة c_m3_حد_بندي_Continuations: أرجع m3_Err_resource_حد_بندي_Not_Supported;
#جيڪڏهن_پورو
    حالة c_m3_حد_بندي_Gas_Units:
#إذا d_m3HasGasMetering
        i_runtime->gas_حد_بندي = i_runtime->gas_بچيل = (i64)M3_MIN(i_قدر, (u64)INT64_MAX);
        أرجع m3Err_none;
#وإلا
        أرجع m3_Err_resource_حد_بندي_Not_Supported;
#جيڪڏهن_پورو
    افتراضي: أرجع m3_Err_unknown_Resource_حد_بندي;
    }
}

uint64_t m3_Get_Resource_حد_بندي (IM3Runtime i_runtime, M3_Resource_حد_بندي i_حد_بندي)
{
    إذا (not i_runtime) {
        أرجع 0;
    }
    بدل (i_حد_بندي) {
    حالة c_m3_حد_بندي_يادگيري_بائيٽون: أرجع i_runtime->يادگيري_بائيٽون_حد_بندي;
    حالة c_m3_حد_بندي_جدول_Elements: أرجع i_runtime->جدول_Elements_حد_بندي;
#إذا d_m3_Has_ڍير_Switching
    حالة c_m3_حد_بندي_Continuations: أرجع i_runtime->continuations_حد_بندي;
#جيڪڏهن_پورو
#إذا d_m3HasGasMetering
    حالة c_m3_حد_بندي_Gas_Units: أرجع (u64)i_runtime->gas_حد_بندي;
#جيڪڏهن_پورو
    افتراضي: أرجع 0;
    }
}

uint64_t m3_GetResourceUsage (IM3Runtime i_runtime, M3_Resource_حد_بندي i_حد_بندي)
{
    إذا (not i_runtime) {
        أرجع 0;
    }
    بدل (i_حد_بندي) {
    حالة c_m3_حد_بندي_يادگيري_بائيٽون: أرجع i_runtime->يادگيري_بائيٽون_استعمال_ٿيل;
    حالة c_m3_حد_بندي_جدول_Elements: أرجع i_runtime->جدول_Elements_استعمال_ٿيل;
#إذا d_m3_Has_ڍير_Switching
    حالة c_m3_حد_بندي_Continuations: أرجع i_runtime->continuationsAllocated;
#جيڪڏهن_پورو
#إذا d_m3HasGasMetering
    حالة c_m3_حد_بندي_Gas_Units: أرجع (u64)i_runtime->gas_حد_بندي - (u64)i_runtime->gas_بچيل;
#جيڪڏهن_پورو
    افتراضي: أرجع 0;
    }
}

فراغ* m3_Get_User_معلومات (IM3Runtime i_runtime)
{
    أرجع i_runtime ? i_runtime->userdata : عدم;
}


فراغ* ForEachModule (IM3Runtime i_runtime, ModuleVisitor i_visitor, فراغ* i_info)
{
    فراغ* r = عدم;

    IM3Module module = i_runtime->modules;

    طالما (module) {
        IM3Module ايندڙ = module->ايندڙ;
        إذا ((r = i_visitor(module, i_info))) {
            اكسر;
        }

        module = ايندڙ;
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
    لكل (u32 i = 0; i < i_module->انگ_Memories; ++i) {
        IM3_يادگيري يادگيري = i_module->memories[i];

        إذا (يادگيري and يادگيري->owner != i_module) {
            i_module->memories[i] = عدم;
        }
    }

    لكل (u32 i = 0; i < i_module->انگ_Tables; ++i) {
        IM3_جدول جدول_2 = i_module->tables[i];

        إذا (جدول_2 and جدول_2->owner != i_module) {
            i_module->tables[i] = عدم;
        }
    }

    أرجع عدم;
}


فراغ Runtime_Release (IM3Runtime i_runtime)
{
    ForEachModule(i_runtime, _ReleaseBorrowedSlots, عدم);
    ForEachModule(i_runtime, _FreeModule, عدم);                    d_m3Assert (i_runtime->انگ_سرگرم_ڪوڊ_صفحا == 0);

    Environment_Release_ڪوڊ_صفحا(i_runtime->environment, i_runtime->صفحا_Open);
    Environment_Release_ڪوڊ_صفحا(i_runtime->environment, i_runtime->صفحا_ڀريل);

    m3_Free(i_runtime->origin_ڍير);

#إذا d_m3EnableValidation
    m3_Free(i_runtime->validator);
#جيڪڏهن_پورو

#إذا d_m3HasExceptionHandling
    // a runtime let go of while suspended still holds its exceptions
    FreeExceptions(i_runtime);
    m3_Free(i_runtime->uncaughtException);
#جيڪڏهن_پورو
#إذا d_m3HasExceptionHandling || d_m3_Has_ڍير_Switching
    طالما (i_runtime->host_ٽيگ_فهرست) {
        M3_Host_ٽيگ* ايندڙ = i_runtime->host_ٽيگ_فهرست->ايندڙ;
        m3_Free(i_runtime->host_ٽيگ_فهرست);
        i_runtime->host_ٽيگ_فهرست = ايندڙ;
    }
#جيڪڏهن_پورو
#إذا d_m3_Has_ڍير_Switching
    Continuation_ReleaseAll(i_runtime);
#جيڪڏهن_پورو
}


فراغ m3_FreeRuntime (IM3Runtime i_runtime)
{
    إذا (i_runtime) {
        m3_PrintProfilerInfo();

        Runtime_Release(i_runtime);
        m3_Free(i_runtime);
    }
}


#إذا d_m3_Has_ڍير_Switching

IM3Continuation Continuation_New (IM3Runtime i_runtime, IM3FuncType i_type, IM3Function i_function)
{
    IM3Continuation cont = m3_AllocStruct(M3Continuation);
    إذا (!cont) {
        أرجع عدم;
    }

    cont->type          = i_type;
    cont->داخلا_Function = i_function;
    cont->state         = cont_allocated;

    cont->انگ_ڍير_Slots = d_m3_Continuation_ڍير_Slots;
    // frames are recorded only when this continuation actually suspends, and
    // only as deep as it got, so the array starts out unallocated
    cont->چوکٺا    = عدم;
    cont->چوکٺا_Cap = 0;
    cont->انگ_چوکٺا = 0;

    cont->sp = cont->val_ڍير;
    cont->pc = i_function ? i_function->compiled : عدم;
    cont->r0 = 0;
#  إذا d_m3HasFloat
    cont->fp0 = 0;
#  endif
    cont->bound_دليلون_ڳڻپ    = 0;
    cont->انگ_Suspend_Results = 0;
    cont->انگ_Handlers       = 0;
    cont->handlersPC        = عدم;
    cont->والد            = عدم;

    cont->ايندڙ               = i_runtime->continuations;
    i_runtime->continuations = cont;

    أرجع cont;
}

IM3Continuation Continuation_ForkSuspended (IM3Runtime i_runtime, IM3Continuation i_cont)
{
    IM3Continuation newCont = Continuation_New(i_runtime, i_cont->type, i_cont->داخلا_Function);
    إذا (M3_UNLIKELY(not newCont)) {
        أرجع عدم;
    }

    newCont->val_ڍير      = i_cont->val_ڍير;
    newCont->انگ_ڍير_Slots = i_cont->انگ_ڍير_Slots;
    newCont->sp            = i_cont->sp;
    newCont->pc            = i_cont->pc;
    newCont->suspend_نقطو  = i_cont->suspend_نقطو;
    newCont->r0            = i_cont->r0;
#  إذا d_m3HasFloat
    newCont->fp0 = i_cont->fp0;
#  endif
    newCont->چوکٺا    = i_cont->چوکٺا;
    newCont->چوکٺا_Cap = i_cont->چوکٺا_Cap;
    newCont->انگ_چوکٺا = i_cont->انگ_چوکٺا;

    newCont->bound_دليلون_ڳڻپ = 0;
#  إذا d_m3HasExceptionHandling
    newCont->resumeThrow = i_cont->resumeThrow;
    i_cont->resumeThrow  = عدم;
#  endif
    newCont->انگ_Suspend_Results = i_cont->انگ_Suspend_Results;
    لكل (u32 i = 0; i < i_cont->انگ_Suspend_Results; ++i) {
        newCont->suspend_نتيجو_Offsets[i] = i_cont->suspend_نتيجو_Offsets[i];
        newCont->suspend_نتيجو_Is64[i]    = i_cont->suspend_نتيجو_Is64[i];
    }

    newCont->انگ_Handlers = i_cont->انگ_Handlers;
    newCont->handlersPC  = i_cont->handlersPC;
    newCont->والد      = i_cont->والد;
    newCont->state       = cont_suspended;

    i_cont->val_ڍير  = عدم;
    i_cont->چوکٺا    = عدم;
    i_cont->چوکٺا_Cap = 0;
    i_cont->انگ_چوکٺا = 0;
    i_cont->state     = cont_consumed;

    أرجع newCont;
}

// Only cont.new and snapshot restoration acquire stacks. Binding and suspension
// transfer them between records without changing the active count.
M3_نتيجو Continuation_Acquire_ڍير (IM3Runtime runtime, IM3Continuation cont)
{
    إذا (not Continuation_Can_Acquire_ڍير(runtime)) {
        أرجع m3_Err_continuation_حد_بندي_Exceeded;
    }
    إذا (runtime->val_ڍير_Pool_ڳڻپ) {
        cont->val_ڍير = runtime->val_ڍير_Pool[--runtime->val_ڍير_Pool_ڳڻپ];
        memset(cont->val_ڍير, 0, (cont->انگ_ڍير_Slots + 4) * حجم(m3slot_t));
    } وإلا {
        cont->val_ڍير = m3_AllocArray(m3slot_t, cont->انگ_ڍير_Slots + 4);
    }
    إذا (not cont->val_ڍير) {
        أرجع m3Err_mallocFailed;
    }
    cont->sp = cont->val_ڍير;
    runtime->continuationsAllocated++;
    أرجع m3Err_none;
}

فراغ Continuation_Recycle_ڍير (IM3Runtime runtime, IM3Continuation cont)
{
    إذا (not cont->val_ڍير or cont == runtime->پاڙ_Continuation) {
        أرجع;
    }
    إذا (not runtime->val_ڍير_Pool) {
        runtime->val_ڍير_Pool = m3_AllocArray(m3slot_t*, 16);
        إذا (runtime->val_ڍير_Pool) {
            runtime->val_ڍير_Pool_Cap = 16;
        }
    }
    إذا (runtime->val_ڍير_Pool_ڳڻپ < runtime->val_ڍير_Pool_Cap and
        (not runtime->continuations_حد_بندي or runtime->val_ڍير_Pool_ڳڻپ <
                                              runtime->continuations_حد_بندي - runtime->continuationsAllocated + 1)) {
        runtime->val_ڍير_Pool[runtime->val_ڍير_Pool_ڳڻپ++] = cont->val_ڍير;
        cont->val_ڍير                                      = عدم;
    } وإلا {
        m3_Free(cont->val_ڍير);
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
        IM3Continuation ايندڙ = cont->ايندڙ;

        m3_Free(cont->val_ڍير);
        m3_Free(cont->چوکٺا);
        m3_Free(cont);

        cont = ايندڙ;
    }

    لكل (u32 i = 0; i < io_runtime->val_ڍير_Pool_ڳڻپ; ++i) {
        m3_Free(io_runtime->val_ڍير_Pool[i]);
    }
    m3_Free(io_runtime->val_ڍير_Pool);
    io_runtime->val_ڍير_Pool_ڳڻپ = io_runtime->val_ڍير_Pool_Cap = 0;
    io_runtime->continuationsAllocated                          = 0;
    io_runtime->continuations                                   = عدم;
    io_runtime->سرگرم_Continuation                              = عدم;
    io_runtime->پاڙ_Continuation                                = عدم;
}


m3ret_t Continuation_رڪارڊ_چوکٺو (IM3Runtime i_runtime, ثابت M3_چوکٺو* i_چوکٺو)
{
    IM3Continuation cont = i_runtime->سرگرم_Continuation;

    // the marker only travels between a suspend and the resume that runs it,
    // and that whole stretch has an active continuation
    d_m3Assert(cont);

    إذا (cont->انگ_چوکٺا == cont->چوکٺا_Cap) {
        u32 newCap = cont->چوکٺا_Cap ? cont->چوکٺا_Cap * 2 : 8;

        إذا (newCap > d_m3_Continuation_Max_چوکٺا) {
            newCap = d_m3_Continuation_Max_چوکٺا;
        }

        إذا (newCap == cont->چوکٺا_Cap) {
            أرجع m3_Err_trap_ڍير_حد_کان_مٿي;
        }

        M3_چوکٺو* چوکٺا = m3_ReallocArray(M3_چوکٺو, cont->چوکٺا, newCap, cont->چوکٺا_Cap);

        إذا (not چوکٺا) {
            أرجع m3Err_mallocFailed;
        }

        cont->چوکٺا    = چوکٺا;
        cont->چوکٺا_Cap = newCap;
    }

    cont->چوکٺا[cont->انگ_چوکٺا++] = *i_چوکٺو;

    أرجع m3Err_continuationSuspended;
}

#جيڪڏهن_پورو // d_m3HasStackSwitching

M3_نتيجو Evaluate_اظهار (IM3Module i_module, فراغ* o_expressed, m3type_t i_type, بائيٽون_t* io_بائيٽون, cbytes_t i_پڇاڙي)
{
    M3_نتيجو نتيجو = m3Err_none;

    // OPTZ: use a simplified interpreter for expressions

    // create a temporary runtime context
#إذا defined(d_m3PreferStaticAlloc)
    ساكن M3Runtime runtime;
#وإلا
    M3Runtime runtime;
#جيڪڏهن_پورو
    M3_INIT(runtime);

    runtime.environment   = i_module->runtime->environment;
    runtime.انگ_ڍير_Slots = i_module->runtime->انگ_ڍير_Slots;
    runtime.ڍير         = i_module->runtime->ڍير;

    m3stack_t ڍير = (m3stack_t)runtime.ڍير;

    IM3Runtime savedRuntime = i_module->runtime;
    i_module->runtime       = &runtime;

    IM3Compilation o   = &runtime.compilation;
    o->runtime         = &runtime;
    o->module          = i_module;
    o->wasm            = *io_بائيٽون;
    o->wasm_پڇاڙي         = i_پڇاڙي;
    o->آخري_Opcode_شروعات = o->wasm;

    // Borrowed for the length of this expression only. Runtime_Release below hands it
    // to Environment_ReleaseCodePages, which resets lineIndex, so the page comes back
    // empty and the next expression reuses it rather than burning fresh lines.
    o->صفحو = Acquire_ڪوڊ_صفحو(&runtime);

    إذا (o->صفحو) {
        IM3FuncType ftype = عدم;

#إذا d_m3HasTypedRefs
        // A reference type has to be checked as the type it is: the shared
        // return types are one per storage class, so borrowing one of those
        // would let any reference at all satisfy a particular one.
        إذا (IsSpelledRefType(i_type)) {
            نتيجو = AllocFuncType(&ftype, 1);

            إذا (not نتيجو) {
                ftype->انگ_Rets  = 1;
                ftype->types[0] = i_type;

                نتيجو = Environment_جوڙيو_Func_Type(runtime.environment, &ftype);
            }
        } وإلا
#جيڪڏهن_پورو
        {
            ftype = runtime.environment->retFuncTypes[بنياد_Type_جو(i_type)];
        }

        pc_t m3code = Get_صفحو_PC(o->صفحو);

        إذا (not نتيجو) {
            نتيجو = Compile_اظهار(o, ftype);
        }

        إذا (not نتيجو && o->max_ڍير_Slots >= runtime.انگ_ڍير_Slots) {
            نتيجو = m3_Err_trap_ڍير_حد_کان_مٿي;
        }

        إذا (not نتيجو) {
#إذا (d_m3EnableOpProfiling || d_m3EnableOpTracing)
            m3ret_t r = لاڳيتو_ڪوڊ(m3code, ڍير, عدم, d_m3_Op_Default_دليلون, d_m3_بنياد_Cstr);
#وإلا
            m3ret_t r = لاڳيتو_ڪوڊ(m3code, ڍير, عدم, d_m3_Op_Default_دليلون);
#جيڪڏهن_پورو

            إذا (r == 0) {                                                                   m3log (runtime, "expression result: %s", SPrint_قدر (ڍير, i_type));
                إذا (سائيز_جو_Type(بنياد_Type_جو(i_type)) == حجم(u32)) {
                    *(u32*)o_expressed = *((u32*)ڍير);
                } وإلا {
                    *(u64*)o_expressed = *((u64*)ڍير);
                }
            }
        }

        Release_ڪوڊ_صفحو(&runtime, o->صفحو);
    } وإلا {
        نتيجو = m3_Err_malloc_Failed_ڪوڊ_صفحو;
    }

    runtime.origin_ڍير = عدم;        // prevent free(stack) in ReleaseRuntime
    Runtime_Release(&runtime);
    i_module->runtime = savedRuntime;

    *io_بائيٽون = o->wasm;

    أرجع نتيجو;
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
bool LimitsSatisfy (u64 i_exported_سائيز, bool i_exportedHasMax, u64 i_exportedMax,
                    u64 i_importMin, bool i_importHasMax, u64 i_importMax)
{
    إذا (i_exported_سائيز < i_importMin) {
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
ثابت M3Export* Module_ڳوليو_Export (IM3Module i_module, u8 i_قسم, cstr_t i_name)
{
    لكل (u32 i = 0; i < i_module->انگ_Exports; ++i) {
        ثابت M3Export* e = &i_module->exports[i];

        إذا (e->قسم == i_قسم and strcmp(e->name, i_name) == 0) {
            أرجع e;
        }
    }

    أرجع عدم;
}


ساكن
IM3Function Module_ڳوليو_Exported_Function (IM3Module i_module, cstr_t i_name)
{
    ثابت M3Export* e = Module_ڳوليو_Export(i_module, d_external_قسم_function, i_name);

    // A module that re-exports an import names the placeholder here. Resolve to
    // the function that actually runs, or a host-side call would run it against
    // the importing module's memory.
    أرجع e ? Function_Implementation(&i_module->functions[e->index]) : عدم;
}


ساكن
IM3_يادگيري Module_ڳوليو_Exported_يادگيري (IM3Module i_module, cstr_t i_name)
{
    ثابت M3Export* e = Module_ڳوليو_Export(i_module, d_external_قسم_يادگيري, i_name);

    أرجع e ? i_module->memories[e->index] : عدم;
}


ساكن
IM3_جدول Module_ڳوليو_Exported_جدول (IM3Module i_module, cstr_t i_name)
{
    ثابت M3Export* e = Module_ڳوليو_Export(i_module, d_external_قسم_جدول, i_name);

    أرجع e ? i_module->tables[e->index] : عدم;
}


ساكن
IM3Global Module_ڳوليو_Exported_Global (IM3Module i_module, cstr_t i_name)
{
    ثابت M3Export* e = Module_ڳوليو_Export(i_module, d_external_قسم_global, i_name);

    أرجع e ? &i_module->globals[e->index] : عدم;
}


#إذا d_m3HasExceptionHandling || d_m3_Has_ڍير_Switching
ساكن
IM3_ٽيگ Module_ڳوليو_Exported_ٽيگ (IM3Module i_module, cstr_t i_name)
{
    ثابت M3Export* e = Module_ڳوليو_Export(i_module, d_external_قسم_ٽيگ, i_name);

    إذا (not e) {
        أرجع عدم;
    }

    IM3_ٽيگ ٽيگ = &i_module->ٽيگ_فهرست[e->index];

    أرجع ٽيگ->resolved ? ٽيگ->resolved : ٽيگ;
}
#جيڪڏهن_پورو


// Whether the module exports anything at all under this name. Export names are
// unique within a module, so a name one of the lookups above missed but this
// one finds is exported as something else - a kind the import cannot be
// satisfied by, rather than a name the module never exported.
ساكن
bool Module_HasExport (IM3Module i_module, cstr_t i_name)
{
    لكل (u32 i = 0; i < i_module->انگ_Exports; ++i) {
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
M3_نتيجو LinkImports (IM3Runtime io_runtime, IM3Module io_module)
{
    M3_نتيجو نتيجو = m3Err_none;

    لكل (u32 i = 0; i < io_module->انگ_Functions; ++i) {
        IM3Function f = &io_module->functions[i];

        إذا (f->wasm or not (f->import.moduleUtf8 and f->import.ميدان_Utf8)) {
            استمر;
        }

        IM3Module from = m3_ڳوليو_Module(io_runtime, f->import.moduleUtf8);
        إذا (not from) {
            استمر;
        }

        IM3Function exported = Module_ڳوليو_Exported_Function(from, f->import.ميدان_Utf8);
        إذا (not exported) {
            // the import names a module that is loaded, so its exports settle the
            // question: a name it exports as something else is a type mismatch, and
            // one it does not export at all is an unknown import
            _throwif(m3Err_incompatibleImportType, Module_HasExport(from, f->import.ميدان_Utf8));
            _throw(m3Err_unknownImport);
        }

        // func types are canonical within an environment, so this is the
        // structural equivalence the spec asks for
        _throwif(m3Err_incompatibleImportType, exported->funcType != f->funcType);

        f->resolved = Function_Implementation(exported);
    }

    لكل (u32 i = 0; i < io_module->انگ_Memories; ++i) {
        IM3_يادگيري يادگيري = io_module->memories[i];

        إذا (not يادگيري->imported or يادگيري->owner != io_module) {
            استمر;
        }

        IM3Module from = m3_ڳوليو_Module(io_runtime, يادگيري->import.moduleUtf8);
        إذا (not from) {
            استمر;
        }

        IM3_يادگيري exported = Module_ڳوليو_Exported_يادگيري(from, يادگيري->import.ميدان_Utf8);
        إذا (not exported) {
            _throwif(m3Err_incompatibleImportType, Module_HasExport(from, يادگيري->import.ميدان_Utf8));
            _throw(m3Err_unknownImport);
        }

        // the address type is part of the memory type, so an i64 memory does
        // not satisfy an i32 import, or the other way round - and so is the page
        // size, which custom page sizes made a declared property of a memory
        _throwif(m3Err_incompatibleImportType, exported->isMemory64 != يادگيري->isMemory64);
        _throwif(m3Err_incompatibleImportType, يادگيري_صفحو_سائيز(exported) != يادگيري_صفحو_سائيز(يادگيري));

        // a shared memory is not a subtype of an unshared one, or the other way round
        _throwif(m3Err_incompatibleImportType, exported->isShared != يادگيري->isShared);

        _throwif(m3Err_incompatibleImportType,
                 not LimitsSatisfy(exported->انگ_صفحا, exported->hasMax, exported->max_صفحا,
                                   يادگيري->init_صفحا, يادگيري->hasMax, يادگيري->max_صفحا));

        // hand the slot over to the exporter's memory, and drop the placeholder
        Free_يادگيري_بلاڪ(يادگيري);
        FreeImportInfo(&يادگيري->import);
        m3_Free(يادگيري);

        io_module->memories[i] = exported;
    }

    لكل (u32 i = 0; i < io_module->انگ_Tables; ++i) {
        IM3_جدول جدول_2 = io_module->tables[i];

        إذا (not جدول_2->imported or جدول_2->owner != io_module) {
            استمر;
        }

        IM3Module from = m3_ڳوليو_Module(io_runtime, جدول_2->import.moduleUtf8);
        إذا (not from) {
            استمر;
        }

        IM3_جدول exported = Module_ڳوليو_Exported_جدول(from, جدول_2->import.ميدان_Utf8);
        إذا (not exported) {
            _throwif(m3Err_incompatibleImportType, Module_HasExport(from, جدول_2->import.ميدان_Utf8));
            _throw(m3Err_unknownImport);
        }

        _throwif(m3Err_incompatibleImportType, exported->type != جدول_2->type);

        // the index type is part of the table type, the same way it is for a memory
        _throwif(m3Err_incompatibleImportType, exported->isTable64 != جدول_2->isTable64);

        _throwif(m3Err_incompatibleImportType,
                 not LimitsSatisfy(exported->سائيز, exported->hasMax, exported->max_سائيز,
                                   جدول_2->init_سائيز, جدول_2->hasMax, جدول_2->max_سائيز));

        // hand the slot over to the exporter's table, and drop the placeholder
        m3_Free(جدول_2->elements);
        FreeImportInfo(&جدول_2->import);
        m3_Free(جدول_2);

        io_module->tables[i] = exported;
    }

    لكل (u32 i = 0; i < io_module->انگ_Globals; ++i) {
        IM3Global g = &io_module->globals[i];

        إذا (not g->imported or not (g->import.moduleUtf8 and g->import.ميدان_Utf8)) {
            استمر;
        }

        IM3Module from = m3_ڳوليو_Module(io_runtime, g->import.moduleUtf8);
        إذا (not from) {
            استمر;
        }

        IM3Global exported = Module_ڳوليو_Exported_Global(from, g->import.ميدان_Utf8);
        إذا (not exported) {
            _throwif(m3Err_incompatibleImportType, Module_HasExport(from, g->import.ميدان_Utf8));
            _throw(m3Err_unknownImport);
        }

        _throwif(m3Err_incompatibleImportType, exported->type != g->type);
        _throwif(m3Err_incompatibleImportType, exported->isMutable != g->isMutable);

        g->resolved = exported->resolved ? exported->resolved : exported;
    }

#إذا d_m3HasExceptionHandling || d_m3_Has_ڍير_Switching
    لكل (u32 i = 0; i < io_module->انگ_ٽيگ_فهرست; ++i) {
        IM3_ٽيگ ٽيگ = &io_module->ٽيگ_فهرست[i];

        إذا (not ٽيگ->imported or not (ٽيگ->import.moduleUtf8 and ٽيگ->import.ميدان_Utf8)) {
            استمر;
        }

        IM3Module from = m3_ڳوليو_Module(io_runtime, ٽيگ->import.moduleUtf8);
        إذا (not from) {
            استمر;
        }

        IM3_ٽيگ exported = Module_ڳوليو_Exported_ٽيگ(from, ٽيگ->import.ميدان_Utf8);
        إذا (not exported) {
            _throwif(m3Err_incompatibleImportType, Module_HasExport(from, ٽيگ->import.ميدان_Utf8));
            _throw(m3Err_unknownImport);
        }

        _throwif(m3Err_incompatibleImportType, exported->type != ٽيگ->type);

        ٽيگ->resolved = exported->resolved ? exported->resolved : exported;
    }
#جيڪڏهن_پورو

    _catch: أرجع نتيجو;
}


// Backs each of the module's memories with pages. LinkImports has already
// pointed any import it could satisfy at the exporting module's memory, and
// those are skipped here. An import nothing satisfied is still backed locally
// from its own declared limits, so that the module remains loadable.
M3_نتيجو Init_يادگيري (IM3Runtime io_runtime, IM3Module i_module)
{
    M3_نتيجو نتيجو = m3Err_none;

    // Fixed from here on: the index space stops changing after parse, and
    // linking has already repointed any slot it was going to.
    i_module->memory0 = i_module->انگ_Memories ? i_module->memories[0]
                                              : &i_module->خالي_جايون_يادگيري;

    إذا (i_module->انگ_Memories == 0) {
        // nothing addressable, but _mem still has to point somewhere
        i_module->خالي_جايون_يادگيري.owner    = i_module;
        i_module->خالي_جايون_يادگيري.صفحو_سائيز = d_m3_Default_Mem_صفحو_سائيز;

_       (Resize_يادگيري(io_runtime, &i_module->خالي_جايون_يادگيري, 0));
    }

    لكل (u32 i = 0; i < i_module->انگ_Memories; ++i) {
        IM3_يادگيري يادگيري = i_module->memories[i];

        // a slot that already points at another module's memory is that
        // module's to allocate
        إذا (يادگيري->owner != i_module or يادگيري->mallocated) {
            استمر;
        }

        u32 صفحو_سائيز = يادگيري_صفحو_سائيز(يادگيري);

        يادگيري->صفحو_سائيز = صفحو_سائيز;

        // Without a declared maximum a memory may grow to the spec limit of
        // 2^|addrtype|/pagesize pages, which is the usual 65536 at the default
        // page size, 2^48 for a 64-bit memory, and a whole address space of
        // them when a page is a single byte. A declared maximum of zero is a
        // real limit, not the absence of one.
        //
        // 2^64/pagesize overflows a u64 only when a page is a single byte, and
        // a power-of-two page size divides the address space exactly, so the
        // 64-bit division below is 2^64/pagesize written so that it fits.
        إذا (not يادگيري->hasMax) {
            يادگيري->max_صفحا = يادگيري->isMemory64
                                 ? (صفحو_سائيز > 1 ? (UINT64_MAX / صفحو_سائيز) + 1 : UINT64_MAX)
                                 : (0x100000000ull / صفحو_سائيز);
        }

_       (Resize_يادگيري(io_runtime, يادگيري, يادگيري->init_صفحا));
    }

    _catch: أرجع نتيجو;
}


M3_نتيجو Resize_يادگيري (IM3Runtime io_runtime, IM3_يادگيري يادگيري, u64 i_انگ_صفحا)
{
    M3_نتيجو نتيجو = m3Err_none;

    u64 انگ_صفحا_منزل_Alloc = i_انگ_صفحا;

    إذا (انگ_صفحا_منزل_Alloc <= يادگيري->max_صفحا) {
        // A 64-bit memory may ask for up to 2^48 pages, which overflows a u64
        // of bytes. Nothing that large can be backed, so refuse it up front
        // rather than multiplying into a wrapped size.
        _throwif("linear memory limitation exceeded",
                 انگ_صفحا_منزل_Alloc > d_m3_پتو_حد_بندي / يادگيري->صفحو_سائيز);

        u64 انگ_صفحو_بائيٽون = انگ_صفحا_منزل_Alloc * يادگيري->صفحو_سائيز;

#إذا d_m3_Max_لڪيري_يادگيري_صفحا > 0
        // the limit is a memory size, counted in default-sized pages; comparing
        // it against a raw page count would make it 65536 times stricter for a
        // module whose pages are one byte
        _throwif("linear memory limitation exceeded",
                 انگ_صفحو_بائيٽون > (u64)d_m3_Max_لڪيري_يادگيري_صفحا * d_m3_Default_Mem_صفحو_سائيز);
#جيڪڏهن_پورو

        // Limit the amount of memory that gets actually allocated
        إذا (io_runtime->يادگيري_حد_بندي) {
            انگ_صفحو_بائيٽون = M3_MIN(انگ_صفحو_بائيٽون, (u64)io_runtime->يادگيري_حد_بندي);
        }

        u64 old_ڊيگهائي = يادگيري->mallocated ? يادگيري->mallocated->ڊيگهائي : 0;
        u64 other_استعمال_ٿيل = io_runtime->يادگيري_بائيٽون_استعمال_ٿيل - old_ڊيگهائي;
        _throwif(m3_Err_يادگيري_حد_بندي_Exceeded, io_runtime->يادگيري_بائيٽون_حد_بندي and
                                              (انگ_صفحو_بائيٽون > io_runtime->يادگيري_بائيٽون_حد_بندي or
                                               other_استعمال_ٿيل > io_runtime->يادگيري_بائيٽون_حد_بندي - انگ_صفحو_بائيٽون));

        _throwif("linear memory limitation exceeded", انگ_صفحو_بائيٽون > (u64)SIZE_MAX - حجم(M3_يادگيري_سرو));

#إذا d_m3_Guarded_يادگيري

        // A slot holds every address a Wasm access can name, and nothing beyond
        // that: a memory larger than the data part of one would put addresses past
        // the guard, where a fault is nobody's to recognize
        _throwif("linear memory limitation exceeded", انگ_صفحو_بائيٽون > d_m3_Guarded_معلومات_بائيٽون);

        // What makes an access past the end fault is the page after the memory not
        // being committed, so the end has to fall on one of the system's page
        // boundaries. A Wasm page divides evenly into them and always will; a memory
        // that declared a page size of its own may not, and then the bytes between
        // the end of the memory and the end of its last system page would answer
        // instead of trapping. Refusing to back such a memory is the honest way out:
        // the custom page sizes proposal exists for targets far too small to reserve
        // 8GiB per memory in the first place. See d_m3GuardedMemory.
        _throwif("guarded memory needs a page size the system's divides into",
                 (يادگيري->صفحو_سائيز % m3_Host_صفحو_سائيز()) != 0);

        إذا (يادگيري->guard_جڳهه == عدم) {
            يادگيري->guard_جڳهه = Guard_Take_جڳهه();
            _throwifnull(يادگيري->guard_جڳهه);
        }

        // Growing only ever commits more of the same slot, so the memory never
        // moves - which the interpreter does not rely on, but nothing minds
        _throwif(m3Err_mallocFailed, not Guard_Commit_جڳهه(يادگيري->guard_جڳهه, (size_t)انگ_صفحو_بائيٽون));

        يادگيري->mallocated = Guard_جڳهه_سرو(يادگيري->guard_جڳهه);

#وإلا

        size_t انگ_بائيٽون = (size_t)انگ_صفحو_بائيٽون + حجم(M3_يادگيري_سرو);

        // What is really there, which is not numPages * pageSize: the limit above
        // caps the bytes without capping the page count, so a memory can be short
        // of a page - by design, since a whole 64 KiB page is often more than an
        // MCU has. Measuring the old block by the page count would overstate it,
        // and then realloc never sees the size it already has and copies the
        // whole limit again on every grow past it.
        size_t انگ_اڳيون_بائيٽون = يادگيري->mallocated ? يادگيري->mallocated->ڊيگهائي + حجم(M3_يادگيري_سرو) : 0;

        فراغ* newMem = m3_Realloc("Wasm Linear Memory", يادگيري->mallocated, انگ_بائيٽون, انگ_اڳيون_بائيٽون);
        _throwifnull(newMem);

        يادگيري->mallocated = (M3_يادگيري_سرو*)newMem;

#جيڪڏهن_پورو

#إذا d_m3LogRuntime
        M3_يادگيري_سرو* oldMallocated = يادگيري->mallocated;
#جيڪڏهن_پورو

        io_runtime->يادگيري_بائيٽون_استعمال_ٿيل = other_استعمال_ٿيل + انگ_صفحو_بائيٽون;
        يادگيري->انگ_صفحا            = انگ_صفحا_منزل_Alloc;

        يادگيري->mallocated->ڊيگهائي  = (size_t)انگ_صفحو_بائيٽون;
        يادگيري->mallocated->runtime = io_runtime;
        يادگيري->mallocated->يادگيري  = يادگيري;

        يادگيري->mallocated->max_ڍير = (m3slot_t*)io_runtime->origin_ڍير + io_runtime->انگ_ڍير_Slots;

        m3log(runtime, "resized old: %p; mem: %p; length: %zu; pages: %llu", oldMallocated, يادگيري->mallocated, يادگيري->mallocated->ڊيگهائي, (بنانشان ڊگهو ڊگهو)يادگيري->انگ_صفحا);
    } وإلا {
        نتيجو = m3_Err_wasm_يادگيري_حد_کان_مٿي;
    }

    _catch: أرجع نتيجو;
}


فراغ Free_يادگيري_بلاڪ (IM3_يادگيري io_يادگيري)
{
    إذا (io_يادگيري->mallocated and io_يادگيري->mallocated->runtime) {
        io_يادگيري->mallocated->runtime->يادگيري_بائيٽون_استعمال_ٿيل -= io_يادگيري->mallocated->ڊيگهائي;
    }
#إذا d_m3_Guarded_يادگيري
    Guard_Give_جڳهه(io_يادگيري->guard_جڳهه);
    io_يادگيري->guard_جڳهه = عدم;

    io_يادگيري->mallocated = عدم;
#وإلا
    m3_Free(io_يادگيري->mallocated);
#جيڪڏهن_پورو
}


M3_نتيجو InitGlobals (IM3Module io_module)
{
    M3_نتيجو نتيجو = m3Err_none;

    إذا (io_module->انگ_Globals) {
        // placing the globals in their structs isn't good for cache locality, but i don't really know what the global
        // access patterns typically look like yet.

        //          io_module->globalMemory = m3Alloc (m3reg_t, io_module->numGlobals);

        //          if (io_module->globalMemory)
        {
            لكل (u32 i = 0; i < io_module->انگ_Globals; ++i) {
                M3Global* g = &io_module->globals[i];                           m3log (runtime, "initializing global: %d", i);

                إذا (g->init_اظهار) {
                    بائيٽون_t شروعات = g->init_اظهار;

                    نتيجو = Evaluate_اظهار(io_module, &g->i64_قدر, g->type, &شروعات, g->init_اظهار + g->init_اظهار_سائيز);

                    إذا (not نتيجو) {
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

    أرجع نتيجو;
}


M3_نتيجو Init_معلومات_Segments (IM3Module io_module)
{
    M3_نتيجو نتيجو = m3Err_none;

    لكل (u32 i = 0; i < io_module->انگ_معلومات_Segments; ++i) {
        M3_معلومات_Segment* segment = &io_module->معلومات_Segments[i];

        // A passive segment stays available for memory.init until data.drop.
        // An active one is copied here and then counts as dropped.
        إذا (segment->isPassive) {
            استمر;
        }

        _throwif("data segment memory index out of range",
                 segment->يادگيري_Region >= io_module->انگ_Memories);

        IM3_يادگيري io_يادگيري = io_module->memories[segment->يادگيري_Region];

        _throwif("unallocated linear memory", !(io_يادگيري->mallocated));

        // The offset expression has the memory's address type, and is
        // unsigned: an i32 offset of -1 is 4294967295, way out of bounds
        // rather than negative.
        u64     segment_وٿي = 0;
        بائيٽون_t شروعات         = segment->init_اظهار;

        إذا (io_يادگيري->isMemory64) {
_           (Evaluate_اظهار(io_module, &segment_وٿي, c_m3Type_i64, &شروعات, segment->init_اظهار + segment->init_اظهار_سائيز));
        } وإلا {
            u32 offset32;
_           (Evaluate_اظهار(io_module, &offset32, c_m3Type_i32, &شروعات, segment->init_اظهار + segment->init_اظهار_سائيز));
            segment_وٿي = offset32;
        }

        m3log(runtime, "loading data segment: %d; size: %d; offset: %llu", i, segment->سائيز, (بنانشان ڊگهو ڊگهو)segment_وٿي);

        إذا (segment_وٿي <= io_يادگيري->mallocated->ڊيگهائي &&
            (u64)segment->سائيز <= io_يادگيري->mallocated->ڊيگهائي - segment_وٿي) {
            u8* dest = m3_Mem_معلومات(io_يادگيري->mallocated) + segment_وٿي;
            يادگيري_نقل(dest, segment->معلومات, segment->سائيز);
        } وإلا {
            _throw("data segment out of bounds");
        }

        segment->dropped = true;
    }

    _catch: أرجع نتيجو;
}


// Turns a segment's elements into references. Element expressions are constant
// expressions restricted to ref.null/ref.func, so they're read directly rather
// than run through the compiler.
ساكن
M3_نتيجو ResolveElements (IM3Module io_module, M3ElementSegment* i_segment, فراغ** o_elements)
{
    M3_نتيجو نتيجو = m3Err_none;

    بائيٽون_t  pos = i_segment->elements;
    cbytes_t پڇاڙي = io_module->element_Section_پڇاڙي;

    لكل (u32 e = 0; e < i_segment->انگ_Elements; ++e) {
        u32   func_اشاريو;
        فراغ* ref = عدم;

        إذا (i_segment->is_اظهار) {
            m3opcode_t opcode;
_           (Read_opcode(&opcode, &pos, پڇاڙي));

            إذا (opcode == c_waOp_refFunc) {
_               (ReadLEB_u32(&func_اشاريو, &pos, پڇاڙي));
                _throwif("function index out of range", func_اشاريو >= io_module->انگ_Functions);
                ref = Function_Implementation(&io_module->functions[func_اشاريو]);
            } وإلا إذا (opcode == c_waOp_refNull) {
                i8 waType;
                u8 nullType;
_               (ReadLEB_i7(&waType, &pos, پڇاڙي));
_               (NormalizeType(&nullType, waType));
                _throwif(m3Err_typeMismatch, nullType != i_segment->type);
            } وإلا إذا (opcode == c_waOp_getGlobal) {
                // wasm 2.0 lets an element expression read an imported
                // immutable global, which is how one module seeds another's
                // table with a reference it exported.
                u32 global_اشاريو;
_               (ReadLEB_u32(&global_اشاريو, &pos, پڇاڙي));
                _throwif(m3_Err_globa_اشاريو_ٻاهريون_جو_Bounds, global_اشاريو >= io_module->انگ_Globals);

                IM3Global global = &io_module->globals[global_اشاريو];

                _throwif(m3_Err_globa_اشاريو_ٻاهريون_جو_Bounds, not global->imported);
                _throwif(m3Err_wasmMalformed, global->isMutable);
                _throwif(m3Err_typeMismatch, بنياد_Type_جو(global->type) != بنياد_Type_جو(i_segment->type));

                // read the cell the import was linked to, not the placeholder
                إذا (global->resolved) {
                    global = global->resolved;
                }

                ref = global->ref_قدر;
            } وإلا {
                _throw("constant expression required");
            }

_           (Read_opcode(&opcode, &pos, پڇاڙي));
            _throwif(m3Err_wasmMalformed, opcode != c_wa_Op_پڇاڙي);
        } وإلا {
_           (ReadLEB_u32(&func_اشاريو, &pos, پڇاڙي));
            _throwif("function index out of range", func_اشاريو >= io_module->انگ_Functions);
            ref = Function_Implementation(&io_module->functions[func_اشاريو]);
        }

        o_elements[e] = ref;
    }

    _catch: أرجع نتيجو;
}


M3_نتيجو Init_جدول_And_Elements (IM3Module io_module)
{
    M3_نتيجو نتيجو = m3Err_none;

    cbytes_t پڇاڙي = io_module->element_Section_پڇاڙي;
    M3_جدول* جدول_2;

    لكل (u32 i = 0; i < io_module->انگ_Tables; ++i) {
        جدول_2 = io_module->tables[i];

        // a slot pointing at another module's table is that module's to fill
        إذا (جدول_2->owner != io_module) {
            استمر;
        }

        إذا (جدول_2->سائيز) {
            IM3Runtime runtime = io_module->runtime;
            _throwif(m3_Err_جدول_حد_بندي_Exceeded, runtime->جدول_Elements_حد_بندي and
                                                 جدول_2->سائيز > runtime->جدول_Elements_حد_بندي - runtime->جدول_Elements_استعمال_ٿيل);
            جدول_2->elements = m3_AllocArray(فراغ*, جدول_2->سائيز);
            _throwifnull(جدول_2->elements);
            runtime->جدول_Elements_استعمال_ٿيل += جدول_2->سائيز;

            إذا (جدول_2->init_اظهار) {
                فراغ*   قدر = عدم;
                بائيٽون_t شروعات = جدول_2->init_اظهار;
_               (Evaluate_اظهار(io_module, &قدر, بنياد_Type_جو(جدول_2->type),
                                     &شروعات, جدول_2->init_اظهار + جدول_2->init_اظهار_سائيز));

                لكل (u32 e = 0; e < جدول_2->سائيز; ++e) {
                    جدول_2->elements[e] = قدر;
                }
            }
        }
    }

    لكل (u32 i = 0; i < io_module->انگ_Element_Segments; ++i) {
        M3ElementSegment* segment = &io_module->elementSegments[i];

        // Declarative segments only make their functions referenceable, and
        // passive ones wait for table.init, so neither is written out here.
        إذا (segment->گهڻائي == c_m3Elem_declarative) {
            segment->dropped = true;
            استمر;
        }

        إذا (segment->گهڻائي == c_m3Elem_passive) {
            إذا (segment->انگ_Elements) {
                segment->resolved = m3_AllocArray(فراغ*, segment->انگ_Elements);
                _throwifnull(segment->resolved);
_               (ResolveElements(io_module, segment, segment->resolved));
            }
            استمر;
        }

        جدول_2 = io_module->tables[segment->جدول_اشاريو];

        // The offset expression has the table's index type, and is unsigned:
        // an i32 offset of -1 is 4294967295, out of bounds rather than negative.
        u64     offset = 0;
        بائيٽون_t اظهار   = segment->init_اظهار;

        إذا (جدول_2->isTable64) {
_           (Evaluate_اظهار(io_module, &offset, c_m3Type_i64, &اظهار, پڇاڙي));
        } وإلا {
            u32 offset32;
_           (Evaluate_اظهار(io_module, &offset32, c_m3Type_i32, &اظهار, پڇاڙي));
            offset = offset32;
        }

        _throwif("out of bounds table access",
                 offset > جدول_2->سائيز or segment->انگ_Elements > جدول_2->سائيز - offset);

_       (ResolveElements(io_module, segment, جدول_2->elements + offset));

        segment->dropped = true;
    }

    _catch: أرجع نتيجو;
}

M3_نتيجو m3_CompileModule (IM3Module io_module)
{
    M3_نتيجو نتيجو = m3Err_none;

    لكل (u32 i = 0; i < io_module->انگ_Functions; ++i) {
        IM3Function f = &io_module->functions[i];
        إذا (f->wasm and not f->compiled) {
_           (CompileFunction(f));
        }
    }

    _catch: أرجع نتيجو;
}

#إذا d_m3HasExceptionHandling

M3Exception* NewException (IM3Runtime io_runtime, IM3_ٽيگ i_ٽيگ, u32 i_انگ_دليلون)
{
    M3Exception* exception = (M3Exception*)m3_Malloc("M3Exception", حجم(M3Exception) + i_انگ_دليلون * حجم(u64));

    إذا (exception) {
        exception->ٽيگ     = i_ٽيگ;
        exception->انگ_دليلون = i_انگ_دليلون;
        exception->reified = false;
        exception->prev    = عدم;
        exception->ايندڙ    = io_runtime->exceptions;

        إذا (exception->ايندڙ) {
            exception->ايندڙ->prev = exception;
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
        i_exception->prev->ايندڙ = i_exception->ايندڙ;
    } وإلا {
        io_runtime->exceptions = i_exception->ايندڙ;
    }

    إذا (i_exception->ايندڙ) {
        i_exception->ايندڙ->prev = i_exception->prev;
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
        M3Exception* ايندڙ = exception->ايندڙ;
        m3_Free_Impl(exception);
        exception = ايندڙ;
    }
}


// Keeps a copy of the exception a call just ended with, for m3_GetExceptionTag,
// or forgets the last one when the call ended any other way. A copy, because
// the original is freed with the rest before the outermost call returns - and
// one a host function's nested call ended with may still be named by an
// exnref further down the stack.
ساكن
فراغ رڪارڊ_Uncaught_Exception (IM3Runtime io_runtime, M3_نتيجو i_نتيجو)
{
    إذا (io_runtime->uncaughtException) {
        m3_Free(io_runtime->uncaughtException);
    }

    M3Exception* exception = io_runtime->pendingException;

    إذا (M3_UNLIKELY(i_نتيجو == m3Err_trapUncaughtException) and exception) {
        M3Exception* copy = (M3Exception*)m3_CopyMem(exception, حجم(M3Exception) + exception->انگ_دليلون * حجم(u64));

        إذا (copy) {
            copy->ايندڙ = عدم;
            copy->prev = عدم;
        }

        io_runtime->uncaughtException = copy;
    }
}

#جيڪڏهن_پورو // d_m3HasExceptionHandling


#إذا d_m3HasExceptionHandling || d_m3_Has_ڍير_Switching

IM3_ٽيگ m3_ڳوليو_ٽيگ (IM3Module i_module, ثابت حرف* ثابت i_ٽيگ_Name)
{
    إذا (not i_module or not i_ٽيگ_Name) {
        أرجع عدم;
    }

    أرجع Module_ڳوليو_Exported_ٽيگ(i_module, i_ٽيگ_Name);
}


M3_نتيجو m3_New_ٽيگ (IM3Runtime io_runtime, IM3_ٽيگ* o_ٽيگ, ثابت حرف* ثابت i_signature)
{
    IM3FuncType type    = عدم;
    M3_Host_ٽيگ*  host_ٽيگ = عدم;

_try {
    _throwif("null tag", not o_ٽيگ);
    *o_ٽيگ = عدم;
    _throwif(m3Err_mallocFailed, not io_runtime);

_   (Signature_منزل_Func_Type(&type, i_signature));
    // tags are the same type exactly when their types are the same object
_   (Environment_جوڙيو_Func_Type(io_runtime->environment, &type));

    host_ٽيگ = m3_AllocStruct(M3_Host_ٽيگ);
    _throwifnull(host_ٽيگ);

    host_ٽيگ->ٽيگ.type    = type;
    host_ٽيگ->ايندڙ        = io_runtime->host_ٽيگ_فهرست;
    io_runtime->host_ٽيگ_فهرست = host_ٽيگ;

    *o_ٽيگ = &host_ٽيگ->ٽيگ;
} _catch:
    أرجع نتيجو;
}


M3_نتيجو m3_Link_ٽيگ (IM3Module         io_module,
                     ثابت حرف* ثابت i_moduleName,
                     ثابت حرف* ثابت i_ٽيگ_Name,
                     IM3_ٽيگ            i_ٽيگ)
{
    إذا (not io_module or not i_moduleName or not i_ٽيگ_Name or not i_ٽيگ) {
        أرجع m3_Err_ٽيگ_Lookup_Failed;
    }

    ثابت bool wildcardModule = (strcmp(i_moduleName, "*") == 0);
    IM3_ٽيگ     هدف         = i_ٽيگ->resolved ? i_ٽيگ->resolved : i_ٽيگ;

    M3_نتيجو نتيجو = m3_Err_ٽيگ_Lookup_Failed;

    لكل (u32 i = 0; i < io_module->انگ_ٽيگ_فهرست; ++i) {
        IM3_ٽيگ ٽيگ = &io_module->ٽيگ_فهرست[i];

        إذا (ٽيگ->imported and ٽيگ->import.moduleUtf8 and ٽيگ->import.ميدان_Utf8 and
            strcmp(ٽيگ->import.ميدان_Utf8, i_ٽيگ_Name) == 0 and
            (wildcardModule or strcmp(ٽيگ->import.moduleUtf8, i_moduleName) == 0)) {
            إذا (ٽيگ->type != هدف->type) {
                أرجع m3Err_incompatibleImportType;
            }

            ٽيگ->resolved = هدف;
            نتيجو        = m3Err_none;
        }
    }

    أرجع نتيجو;
}


uint32_t m3_Get_ٽيگ_Arg_ڳڻپ (IM3_ٽيگ i_ٽيگ)
{
    أرجع i_ٽيگ ? i_ٽيگ->type->انگ_دليلون : 0;
}


M3_قدر_Type m3_Get_ٽيگ_Arg_Type (IM3_ٽيگ i_ٽيگ, uint32_t i_اشاريو)
{
    إذا (i_ٽيگ and i_اشاريو < i_ٽيگ->type->انگ_دليلون) {
        أرجع (M3_قدر_Type)بنياد_Type_جو(d_FuncArgType(i_ٽيگ->type, i_اشاريو));
    }

    أرجع c_m3Type_none;
}

#وإلا

IM3_ٽيگ m3_ڳوليو_ٽيگ (IM3Module i_module, ثابت حرف* ثابت i_ٽيگ_Name)
{
    أرجع عدم;
}

M3_نتيجو m3_New_ٽيگ (IM3Runtime io_runtime, IM3_ٽيگ* o_ٽيگ, ثابت حرف* ثابت i_signature)
{
    إذا (o_ٽيگ) {
        *o_ٽيگ = عدم;
    }
    أرجع m3_Err_ٽيگ_Lookup_Failed;
}

M3_نتيجو m3_Link_ٽيگ (IM3Module io_module, ثابت حرف* ثابت i_moduleName, ثابت حرف* ثابت i_ٽيگ_Name, IM3_ٽيگ i_ٽيگ)
{
    أرجع m3_Err_ٽيگ_Lookup_Failed;
}

uint32_t m3_Get_ٽيگ_Arg_ڳڻپ (IM3_ٽيگ i_ٽيگ)
{
    أرجع 0;
}

M3_قدر_Type m3_Get_ٽيگ_Arg_Type (IM3_ٽيگ i_ٽيگ, uint32_t i_اشاريو)
{
    أرجع c_m3Type_none;
}

#جيڪڏهن_پورو // d_m3HasExceptionHandling || d_m3HasStackSwitching


#إذا d_m3HasExceptionHandling

M3_نتيجو m3_ThrowException (IM3Runtime io_runtime, IM3_ٽيگ i_ٽيگ, uint32_t i_argc, ثابت فراغ* i_argptrs[])
{
    إذا (not io_runtime or not i_ٽيگ) {
        أرجع m3_Err_ٽيگ_Lookup_Failed;
    }

    IM3_ٽيگ      ٽيگ  = i_ٽيگ->resolved ? i_ٽيگ->resolved : i_ٽيگ;
    IM3FuncType type = ٽيگ->type;

    // a tag with results is a stack switching control tag, which is suspended
    // with rather than thrown
    إذا (type->انگ_Rets) {
        أرجع m3Err_typeMismatch;
    }
    إذا (i_argc != type->انگ_دليلون) {
        أرجع m3_Err_دليل_ڳڻپ_Mismatch;
    }
    // with nothing on the stack there is nothing to unwind, and the exception
    // would sit on the runtime's list until some later call ended
    إذا (io_runtime->callNesting == 0) {
        أرجع "m3_ThrowException called outside a host function";
    }

    M3Exception* exception = NewException(io_runtime, ٽيگ, i_argc);
    إذا (not exception) {
        أرجع m3Err_mallocFailed;
    }

    // the layout op_Throw gives a payload: 32-bit values zero-extended, floats
    // as their bit pattern
    لكل (u32 i = 0; i < i_argc; ++i) {
        ثابت فراغ* قدر = i_argptrs[i];

        بدل (بنياد_Type_جو(d_FuncArgType(type, i))) {
        حالة c_m3Type_i32:
        حالة c_m3Type_f32: {
            u32 بٽ;
            يادگيري_نقل(&بٽ, قدر, حجم(بٽ));
            exception->دليلون[i] = بٽ;
            اكسر;
        }
        حالة c_m3Type_i64:
        حالة c_m3Type_f64:
            يادگيري_نقل(&exception->دليلون[i], قدر, حجم(u64));
            اكسر;
        حالة c_m3Type_funcref:
        حالة c_m3Type_externref:
        حالة c_m3Type_exnref:
        حالة c_m3Type_contref:
            exception->دليلون[i] = (u64)(*(ثابت uintptr_t*)قدر);
            اكسر;
        افتراضي:
            FreeException(io_runtime, exception);
            أرجع "unknown argument type";
        }
    }

    io_runtime->pendingException = exception;

    أرجع m3Err_pendingException;
}


IM3_ٽيگ m3_Get_Exception_ٽيگ (IM3Runtime i_runtime)
{
    إذا (i_runtime and i_runtime->uncaughtException) {
        أرجع i_runtime->uncaughtException->ٽيگ;
    }

    أرجع عدم;
}


M3_نتيجو m3_Get_Exception_دليلون (IM3Runtime i_runtime, uint32_t i_argc, ثابت فراغ* o_argptrs[])
{
    إذا (not i_runtime or not i_runtime->uncaughtException) {
        أرجع "no uncaught exception";
    }

    ثابت M3Exception* exception = i_runtime->uncaughtException;
    IM3FuncType        type      = exception->ٽيگ->type;

    إذا (i_argc != exception->انگ_دليلون) {
        أرجع m3_Err_دليل_ڳڻپ_Mismatch;
    }

    لكل (u32 i = 0; i < i_argc; ++i) {
        فراغ* قدر = (فراغ*)o_argptrs[i];
        u64   بٽ  = exception->دليلون[i];

        بدل (بنياد_Type_جو(d_FuncArgType(type, i))) {
        حالة c_m3Type_i32:
        حالة c_m3Type_f32: {
            u32 هيٺيون = (u32)بٽ;
            يادگيري_نقل(قدر, &هيٺيون, حجم(هيٺيون));
            اكسر;
        }
        حالة c_m3Type_i64:
        حالة c_m3Type_f64:
            يادگيري_نقل(قدر, &بٽ, حجم(بٽ));
            اكسر;
        حالة c_m3Type_funcref:
        حالة c_m3Type_externref:
        حالة c_m3Type_exnref:
        حالة c_m3Type_contref:
            *(uintptr_t*)قدر = (uintptr_t)بٽ;
            اكسر;
        افتراضي:
            أرجع "unknown argument type";
        }
    }

    أرجع m3Err_none;
}

#وإلا

M3_نتيجو m3_ThrowException (IM3Runtime io_runtime, IM3_ٽيگ i_ٽيگ, uint32_t i_argc, ثابت فراغ* i_argptrs[])
{
    أرجع m3_Err_ٽيگ_Lookup_Failed;
}

IM3_ٽيگ m3_Get_Exception_ٽيگ (IM3Runtime i_runtime)
{
    أرجع عدم;
}

M3_نتيجو m3_Get_Exception_دليلون (IM3Runtime i_runtime, uint32_t i_argc, ثابت فراغ* o_argptrs[])
{
    أرجع "no uncaught exception";
}

#جيڪڏهن_پورو // d_m3HasExceptionHandling


#إذا d_m3_Guarded_يادگيري

// What RunCodeProtected hands to the platform layer to run, and what comes back
تعريفنوع بنية M3_Protected_لاڳيتو {
    pc_t            pc;
    IM3Runtime      runtime;
    M3_يادگيري_سرو* mem;
    M3_نتيجو        نتيجو;
} M3_Protected_لاڳيتو;

ساكن
فراغ لاڳيتو_ڪوڊ_Body (فراغ* io_context)
{
    M3_Protected_لاڳيتو* لاڳيتو = (M3_Protected_لاڳيتو*)io_context;

#  إذا (d_m3EnableOpProfiling || d_m3EnableOpTracing)
    لاڳيتو->نتيجو = (M3_نتيجو)لاڳيتو_ڪوڊ(لاڳيتو->pc, (m3stack_t)لاڳيتو->runtime->ڍير, لاڳيتو->mem,
                                    d_m3_Op_Default_دليلون, d_m3_بنياد_Cstr);
#  وإلا
    لاڳيتو->نتيجو = (M3_نتيجو)لاڳيتو_ڪوڊ(لاڳيتو->pc, (m3stack_t)لاڳيتو->runtime->ڍير, لاڳيتو->mem,
                                    d_m3_Op_Default_دليلون);
#  endif
}

// The bounds check the load and store operations no longer do, done once here by the
// system instead: an address past the end of a memory reaches the uncommitted part
// of that memory's reservation, and the fault comes back as the trap the spec asks
// for. Nothing is left half-done by it - the Wasm stack and the memories are heap,
// and the caller unwinds the rest.
ساكن
M3_نتيجو لاڳيتو_ڪوڊ_Protected (IM3Runtime i_runtime, pc_t i_pc, M3_يادگيري_سرو* i_mem)
{
    // Memory operations in this build carry no bounds check, so code only runs where a
    // fault past the end of a memory can still be turned into a trap
    إذا (M3_UNLIKELY(not m3_Host_Guards_سرگرم())) {
        أرجع ErrorRuntime(m3_Err_يادگيري_Guards_Unavailable, i_runtime,
                            "another part of the process holds the fault signal");
    }

    M3_Protected_لاڳيتو لاڳيتو;
    لاڳيتو.pc      = i_pc;
    لاڳيتو.runtime = i_runtime;
    لاڳيتو.mem     = i_mem;
    لاڳيتو.نتيجو  = m3Err_none;

    فراغ*  arena;
    size_t arena_بائيٽون;
    Guard_ArenaRange(&arena, &arena_بائيٽون);

    إذا (not m3_HostProtectedCall(لاڳيتو_ڪوڊ_Body, &لاڳيتو, arena, arena_بائيٽون)) {
        أرجع m3_Err_trap_ٻاهريون_جو_Bounds_يادگيري_Access;
    }

    أرجع لاڳيتو.نتيجو;
}

#جيڪڏهن_پورو // d_m3GuardedMemory


#إذا d_m3_Has_ڍير_Switching

// The function a suspended continuation stopped in: the callee of its innermost
// call frame, or the function it was entered with if it made no call. Frames are
// recorded innermost first, so that is the lowest-numbered call frame.
ساكن
IM3Function Continuation_InnermostFunction (IM3Continuation i_cont)
{
    لكل (u32 i = 0; i < i_cont->انگ_چوکٺا; ++i) {
        إذا (i_cont->چوکٺا[i].قسم == چوکٺو_call && i_cont->چوکٺا[i].call.function) {
            أرجع i_cont->چوکٺا[i].call.function;
        }
    }

    أرجع i_cont->داخلا_Function;
}


// Where the host's next call puts its arguments and finds its results. A
// suspended root keeps its frames on the bottom of the runtime's stack, so a call
// made before it is resumed starts above the last of them rather than on top of
// them. The top is the innermost function's frame base plus everything that
// function can use. That base is the root's own sp, unless it stopped inside a
// resume: then the suspension point belongs to the resumed continuation, and the
// root's innermost frame is that resume.
فراغ Runtime_رکو_Call_ڍير (IM3Runtime io_runtime)
{
    IM3Continuation پاڙ = io_runtime->پاڙ_Continuation;

    io_runtime->ڍير = io_runtime->origin_ڍير;

    إذا (not پاڙ or پاڙ->state != cont_suspended) {
        أرجع;
    }

    m3stack_t   sp       = پاڙ->sp;
    IM3Function function = Continuation_InnermostFunction(پاڙ);

    إذا (پاڙ->انگ_چوکٺا and پاڙ->چوکٺا[0].قسم == چوکٺو_resume) {
        sp = پاڙ->چوکٺا[0].sp;
    }

    m3slot_t* بنياد = (m3slot_t*)io_runtime->origin_ڍير;
    m3slot_t* چوٽي  = (m3slot_t*)sp + (function ? function->max_ڍير_Slots : 0);

    إذا (چوٽي > بنياد + io_runtime->انگ_ڍير_Slots) {
        چوٽي = بنياد + io_runtime->انگ_ڍير_Slots;
    }

    io_runtime->ڍير = چوٽي;
}

#جيڪڏهن_پورو // d_m3HasStackSwitching


// Run compiled code on the runtime's stack, bounding native recursion for the
// duration of the call. The outermost invocation establishes the stack limit;
// nested ones (an imported function calling back into Wasm) inherit it.
ساكن مضمن
M3_نتيجو لاڳيتو_ڪوڊ_Checked (IM3Runtime i_runtime, IM3Function i_function)
{
    pc_t i_pc = i_function->compiled;

    // execution runs against the memory of the module the entry point belongs
    // to, not against some runtime-wide one
    M3_يادگيري_سرو* _mem = Module_يادگيري_سرو(i_function->module);

#إذا d_m3HasSnapshots
    إذا (M3_UNLIKELY(i_function->module->isUnusable)) {
        أرجع "a snapshot failed to restore into the module, so it cannot run";
    }
    i_function->module->has_لاڳيتو = true;
#جيڪڏهن_پورو

    d_m3_ڍير_حد_بندي_Enter(i_runtime);

    // a host function calling back into Wasm does not change which call a trap
    // ends: that is still the one the host made first
    إذا (i_runtime->callNesting++ == 0) {
        i_runtime->entered = i_function;
    }
#إذا d_m3HasExceptionHandling
    // handler stacks don't nest across a call boundary: a host function calling
    // back into Wasm cannot be caught by a try_table its own caller entered
    u32 saved_Try_اونهائي   = i_runtime->try_اونهائي;
    i_runtime->try_اونهائي = 0;
#جيڪڏهن_پورو
#إذا d_m3_Has_ڍير_Switching
    // nor do continuations: the native frames on the far side of a host call
    // are not ours to record, so a suspend in here has no handler to reach and
    // no resume to unwind to
    IM3Continuation savedContinuation = i_runtime->سرگرم_Continuation;

    // A suspended root continuation is a paused program waiting to be resumed,
    // not scratch space. A call the host makes before resuming it gets a
    // context of its own, the way any other re-entry does, rather than having
    // the frames of the suspension reset out from under it.
    bool claim_پاڙ = i_runtime->isSuspendable and not savedContinuation and
                     not (i_runtime->پاڙ_Continuation and
                          i_runtime->پاڙ_Continuation->state == cont_suspended);

    إذا (claim_پاڙ and not i_runtime->پاڙ_Continuation) {
        i_runtime->پاڙ_Continuation = Continuation_New(i_runtime, i_function->funcType, i_function);

        إذا (i_runtime->پاڙ_Continuation) {
            // The root borrows the runtime's stack and is not charged as cont.new.
            i_runtime->پاڙ_Continuation->انگ_ڍير_Slots = i_runtime->انگ_ڍير_Slots;
        } وإلا {
            claim_پاڙ = false;
        }
    }

    إذا (claim_پاڙ) {
        i_runtime->سرگرم_Continuation              = i_runtime->پاڙ_Continuation;
        i_runtime->پاڙ_Continuation->داخلا_Function = i_function;
        i_runtime->پاڙ_Continuation->state         = cont_running;
        i_runtime->پاڙ_Continuation->انگ_چوکٺا     = 0;
    } وإلا {
        i_runtime->سرگرم_Continuation = عدم;
    }
#جيڪڏهن_پورو
#إذا d_m3_Guarded_يادگيري
    M3_نتيجو نتيجو = لاڳيتو_ڪوڊ_Protected(i_runtime, i_pc, _mem);
#وإلاإذا (d_m3EnableOpProfiling || d_m3EnableOpTracing)
    M3_نتيجو نتيجو = (M3_نتيجو)لاڳيتو_ڪوڊ(i_pc, (m3stack_t)i_runtime->ڍير, _mem, d_m3_Op_Default_دليلون, d_m3_بنياد_Cstr);
#وإلا
    M3_نتيجو نتيجو = (M3_نتيجو)لاڳيتو_ڪوڊ(i_pc, (m3stack_t)i_runtime->ڍير, _mem, d_m3_Op_Default_دليلون);
#جيڪڏهن_پورو
#إذا d_m3_Has_ڍير_Switching
    إذا (claim_پاڙ) {
        i_runtime->پاڙ_Continuation->state =
          (نتيجو == m3Err_continuationSuspended) ? cont_suspended : cont_returned;

        Runtime_رکو_Call_ڍير(i_runtime);
    }
    i_runtime->سرگرم_Continuation = savedContinuation;
#جيڪڏهن_پورو
#إذا d_m3HasExceptionHandling
    i_runtime->try_اونهائي = saved_Try_اونهائي;

    // an exception that reached the bottom of the call stack found no handler
    إذا (M3_UNLIKELY(نتيجو == m3Err_pendingException)) {
        نتيجو = m3Err_trapUncaughtException;
    }
    رڪارڊ_Uncaught_Exception(i_runtime, نتيجو);

    // a paused invocation can still reach the exceptions it holds, so they wait
    // for it to finish
    إذا (i_runtime->callNesting == 1 and not m3_IsSuspended(i_runtime)) {
        FreeExceptions(i_runtime);
    }
#جيڪڏهن_پورو
    i_runtime->callNesting--;
    d_m3_ڍير_حد_بندي_Leave(i_runtime);

    أرجع نتيجو;
}

M3_نتيجو m3_لاڳيتو_شروعات (IM3Module io_module)
{
#جيڪڏهن_وصف_ٿيل FUZZING_BUILD_گهڻائي_UNSAFE_FOR_PRODUCTION
    // Execution disabled for fuzzing builds
    أرجع m3Err_none;
#جيڪڏهن_پورو

    M3_نتيجو نتيجو           = m3Err_none;
    i32      شروعات_Function_Tmp = -1;

    إذا (io_module and io_module->شروعات_Function >= 0) {
        IM3Function function = &io_module->functions[io_module->شروعات_Function];

        إذا (not function->compiled) {
_           (CompileFunction(function));
        }

        IM3FuncType ftype = function->funcType;
        إذا (ftype->انگ_دليلون != 0 || ftype->انگ_Rets != 0) {
            _throw(m3_Err_دليل_ڳڻپ_Mismatch);
        }

        IM3Module  module  = function->module;
        IM3Runtime runtime = module->runtime;

        شروعات_Function_Tmp = io_module->شروعات_Function;

        io_module->شروعات_Function = -1;

        نتيجو = لاڳيتو_ڪوڊ_Checked(runtime, function);

        // a start function that suspended is not to be started again: resuming
        // is what finishes it
        إذا (نتيجو and نتيجو != m3Err_continuationSuspended) {
            io_module->شروعات_Function = شروعات_Function_Tmp;
            EXCEPTION_PRINT(نتيجو);
            اذهب _catch;
        }
    }

    _catch: أرجع نتيجو;
}

// TODO: deal with main + side-modules loading efforcement
M3_نتيجو m3_ڀريو_Module (IM3Runtime io_runtime, IM3Module io_module)
{
    M3_نتيجو نتيجو = m3Err_none;

    إذا (M3_UNLIKELY(io_module->runtime)) {
        أرجع m3Err_moduleAlreadyLinked;
    }

    io_module->runtime = io_runtime;

    // linking first: a memory import has to be resolved before InitMemory would
    // give it pages of its own, and a global import before an initializer reads it
_   (LinkImports(io_runtime, io_module));

_   (Init_يادگيري(io_runtime, io_module));
_   (InitGlobals(io_module));
    // Spec order: element segments are applied before data segments. It matters
    // when one of them traps - whatever ran before the trap stays done.
_   (Init_جدول_And_Elements(io_module));
_   (Init_معلومات_Segments(io_module));

    // Start func might use imported functions, which are not liked here yet,
    // so it will be called before a function call is attempted (in m3_FindFunction)

#جيڪڏهن_وصف_ٿيل DEBUG
    Module_Generate_نالا(io_module);
#جيڪڏهن_پورو

    io_module->ايندڙ     = io_runtime->modules;
    io_runtime->modules = io_module;
    أرجع نتيجو; // ok

_catch:
    // The runtime owns the module either way. Instantiation may already have
    // written this module's functions into a table another module owns, and the
    // spec keeps whatever it managed to do before the trap, so those entries
    // stay callable - retaining the module is what stops them dangling.
    //
    // It goes on the tail of the list rather than the head: it is not a module
    // anyone should find by name, only one whose functions may still be
    // reachable through someone else's table.
    io_module->ايندڙ = عدم;

    IM3Module* پڇ = &io_runtime->modules;
    طالما (*پڇ) {
        پڇ = &(*پڇ)->ايندڙ;
    }
    *پڇ = io_module;

    أرجع نتيجو;
}

IM3Global m3_ڳوليو_Global (IM3Module         io_module,
                         ثابت حرف* ثابت i_globalName)
{
    // Search exports
    IM3Global exported = Module_ڳوليو_Exported_Global(io_module, i_globalName);
    إذا (exported) {
        // a re-exported global is the one that was imported, so reads and
        // writes have to reach the cell that actually holds the value
        أرجع exported->resolved ? exported->resolved : exported;
    }

    // Search imports
    لكل (u32 i = 0; i < io_module->انگ_Globals; ++i) {
        IM3Global g = &io_module->globals[i];

        إذا (g->import.moduleUtf8 and g->import.ميدان_Utf8) {
            إذا (strcmp(g->import.ميدان_Utf8, i_globalName) == 0) {
                أرجع g->resolved ? g->resolved : g;
            }
        }
    }
    أرجع عدم;
}

M3_نتيجو m3_GetGlobal (IM3Global      i_global,
                       IM3_Tagged_قدر o_قدر)
{
    إذا (not i_global) {
        أرجع m3Err_globalLookupFailed;
    }

    بدل (i_global->type) {
    حالة c_m3Type_i32: o_قدر->قدر.i32 = i_global->i32_قدر; اكسر;
    حالة c_m3Type_i64: o_قدر->قدر.i64 = i_global->i64_قدر; اكسر;
#إذا d_m3HasFloat
    حالة c_m3Type_f32: o_قدر->قدر.f32 = i_global->f32_قدر; اكسر;
    حالة c_m3Type_f64: o_قدر->قدر.f64 = i_global->f64_قدر; اكسر;
#جيڪڏهن_پورو
    افتراضي: أرجع m3Err_invalidTypeId;
    }

    o_قدر->type = (M3_قدر_Type)(i_global->type);
    أرجع m3Err_none;
}

M3_نتيجو m3_سيٽ_Global (IM3Global            i_global,
                       ثابت IM3_Tagged_قدر i_قدر)
{
    إذا (not i_global) {
        أرجع m3Err_globalLookupFailed;
    }
    إذا (not i_global->isMutable) {
        أرجع m3Err_globalNotMutable;
    }
    إذا (i_global->type != i_قدر->type) {
        أرجع m3Err_globalTypeMismatch;
    }

    بدل (i_قدر->type) {
    حالة c_m3Type_i32: i_global->i32_قدر = i_قدر->قدر.i32; اكسر;
    حالة c_m3Type_i64: i_global->i64_قدر = i_قدر->قدر.i64; اكسر;
#إذا d_m3HasFloat
    حالة c_m3Type_f32: i_global->f32_قدر = i_قدر->قدر.f32; اكسر;
    حالة c_m3Type_f64: i_global->f64_قدر = i_قدر->قدر.f64; اكسر;
#جيڪڏهن_پورو
    افتراضي: أرجع m3Err_invalidTypeId;
    }

    أرجع m3Err_none;
}

M3_قدر_Type m3_GetGlobalType (IM3Global i_global)
{
    أرجع (i_global) ? (M3_قدر_Type)(i_global->type) : c_m3Type_none;
}


فراغ* v_ڳوليو_Function (IM3Module i_module, فراغ* i_info)
{
    ثابت حرف* ثابت i_name = (ثابت حرف*)i_info;

    // Prefer exported functions
    IM3Function exported = Module_ڳوليو_Exported_Function(i_module, i_name);
    إذا (exported) {
        أرجع exported;
    }

    // Search internal functions
    لكل (u32 i = 0; i < i_module->انگ_Functions; ++i) {
        IM3Function f = &i_module->functions[i];

        bool isImported = f->import.moduleUtf8 or f->import.ميدان_Utf8;

        إذا (isImported) {
            استمر;
        }

        لكل (صحيح j = 0; j < f->انگ_نالا; j++) {
            إذا (f->نالا[j] and strcmp(f->نالا[j], i_name) == 0) {
                أرجع f;
            }
        }
    }

    أرجع عدم;
}


// Shared tail of the two lookups: a function is only usable once it has code.
ساكن
M3_نتيجو Prepare_مليو_Function (IM3Function* o_function, IM3Function i_function)
{
    M3_نتيجو نتيجو = m3Err_none;

    إذا (not i_function->compiled) {
_       (CompileFunction(i_function))
    }

    _catch:
    *o_function = نتيجو ? عدم : i_function;

    أرجع نتيجو;
}


// Searches every module in the runtime, most recently loaded first. That is a
// guess once more than one module is loaded and two of them export the same
// name - m3_FindFunctionIn says which module is meant.
M3_نتيجو m3_ڳوليو_Function (IM3Function* o_function, IM3Runtime i_runtime, ثابت حرف* ثابت i_functionName)
{
                                                                d_m3Assert (o_function and i_runtime and i_functionName);
    IM3Function function = عدم;

    إذا (not i_runtime->modules) {
        *o_function = عدم;
        أرجع "no modules loaded";
    }

    function = (IM3Function)ForEachModule(i_runtime, v_ڳوليو_Function, (فراغ*)i_functionName);

    إذا (not function) {
        *o_function = عدم;
        أرجع ErrorModule(m3Err_functionLookupFailed, i_runtime->modules, "'%s'", i_functionName);
    }

    أرجع Prepare_مليو_Function(o_function, function);
}


IM3Module m3_ڳوليو_Module (IM3Runtime i_runtime, ثابت حرف* ثابت i_moduleName)
{
    إذا (not i_runtime or not i_moduleName) {
        أرجع عدم;
    }

    // the list is newest-first, so a name registered twice names the newer one
    لكل (IM3Module m = i_runtime->modules; m; m = m->ايندڙ) {
        إذا (m->name and strcmp(m->name, i_moduleName) == 0) {
            أرجع m;
        }
    }

    أرجع عدم;
}


// Searches one module's exports, which is what naming a module means.
M3_نتيجو m3_ڳوليو_Function_اندريون (IM3Function* o_function, IM3Module i_module, ثابت حرف* ثابت i_functionName)
{
                                                                d_m3Assert (o_function and i_functionName);
    إذا (not i_module) {
        *o_function = عدم;
        أرجع m3Err_functionLookupFailed;      // ErrorModule would deref it
    }

    IM3Function function = (IM3Function)v_ڳوليو_Function(i_module, (فراغ*)i_functionName);

    إذا (not function) {
        *o_function = عدم;
        أرجع ErrorModule(m3Err_functionLookupFailed, i_module, "'%s'", i_functionName);
    }

    أرجع Prepare_مليو_Function(o_function, function);
}


M3_نتيجو m3_Get_جدول_Function (IM3Function* o_function, IM3Module i_module, uint32_t i_اشاريو)
{
_try {
    M3_جدول*    جدول_2;
    IM3Function function;

    _throwif("no table", i_module->انگ_Tables == 0);

    جدول_2 = i_module->tables[0];
    _throwif("function index out of range", i_اشاريو >= جدول_2->سائيز);

    function = (IM3Function)جدول_2->elements[i_اشاريو];

    إذا (function) {
        إذا (not function->compiled) {
_           (CompileFunction(function))
        }
    }

    *o_function = function;
} _catch:
    أرجع نتيجو;
}


ساكن
M3_نتيجو check_شروعات_Function (IM3Module i_module)
{
    M3_نتيجو نتيجو = m3Err_none;                               d_m3Assert(i_module);

    // Check if start function needs to be called
    إذا (i_module->شروعات_Function >= 0) {
        نتيجو = m3_لاڳيتو_شروعات(i_module);
    }

    أرجع نتيجو;
}

uint32_t m3_Get_Arg_ڳڻپ (IM3Function i_function)
{
    إذا (i_function) {
        IM3FuncType ft = i_function->funcType;
        إذا (ft) {
            أرجع ft->انگ_دليلون;
        }
    }
    أرجع 0;
}

uint32_t m3_Get_Ret_ڳڻپ (IM3Function i_function)
{
    إذا (i_function) {
        IM3FuncType ft = i_function->funcType;
        إذا (ft) {
            أرجع ft->انگ_Rets;
        }
    }
    أرجع 0;
}


M3_قدر_Type m3_GetArgType (IM3Function i_function, uint32_t index)
{
    إذا (i_function) {
        IM3FuncType ft = i_function->funcType;
        إذا (ft and index < ft->انگ_دليلون) {
            أرجع (M3_قدر_Type)بنياد_Type_جو(d_FuncArgType(ft, index));
        }
    }
    أرجع c_m3Type_none;
}

M3_قدر_Type m3_GetRetType (IM3Function i_function, uint32_t index)
{
    إذا (i_function) {
        IM3FuncType ft = i_function->funcType;
        إذا (ft and index < ft->انگ_Rets) {
            أرجع (M3_قدر_Type)بنياد_Type_جو(d_FuncRetType(ft, index));
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
M3_نتيجو Get_ڍير_اشارو_For_دليلون (IM3Function i_function, u8** o_ڍير)
{
    IM3Runtime  runtime = i_function->module->runtime;
    IM3FuncType ftype   = i_function->funcType;

    // the stack a paused invocation holds is not available to this one
    size_t استعمال_ٿيل      = (size_t)((u8*)runtime->ڍير - (u8*)runtime->origin_ڍير);
    size_t needed    = ((size_t)ftype->انگ_Rets + ftype->انگ_دليلون) * حجم(u64);
    size_t موجود = (size_t)runtime->انگ_ڍير_Slots * حجم(m3slot_t) - استعمال_ٿيل;

    إذا (needed > موجود) {
        أرجع m3_Err_trap_ڍير_حد_کان_مٿي;
    }

    u64* ڍير = (u64*)runtime->ڍير;

    ڍير += ftype->انگ_Rets;

    *o_ڍير = (u8*)ڍير;

    أرجع m3Err_none;
}


M3_نتيجو m3_CallV (IM3Function i_function, ...)
{
    va_list ap;
    va_start(ap, i_function);
    M3_نتيجو r = m3_CallVL(i_function, ap);
    va_end(ap);
    أرجع r;
}

ساكن
فراغ Report_Native_ڍير_Usage ()
{
#إذا d_m3_Log_Native_ڍير
    صحيح ڍير_استعمال_ٿيل = m3_ڍير_Get_Max();
    fprintf(stderr, "Native stack used: %d\n", ڍير_استعمال_ٿيل);
#جيڪڏهن_پورو
}


M3_نتيجو m3_CallVL (IM3Function i_function, va_list i_دليلون)
{
    IM3Runtime  runtime = i_function->module->runtime;
    IM3FuncType ftype   = i_function->funcType;
    M3_نتيجو    نتيجو  = m3Err_none;
    u8*         s       = عدم;

    إذا (!i_function->compiled) {
        أرجع m3_Err_missing_Compiled_ڪوڊ;
    }

#إذا d_m3_رڪارڊ_Backtraces
    ClearBacktrace(runtime);
#جيڪڏهن_پورو

    m3_ڍير_Check_Init();

_   (check_شروعات_Function(i_function->module))

_   (Get_ڍير_اشارو_For_دليلون(i_function, &s))

    لكل (u32 i = 0; i < ftype->انگ_دليلون; ++i) {
        بدل (d_FuncArgType(ftype, i)) {
        حالة c_m3Type_i32:
            *(i32*)(s) = va_arg(i_دليلون, i32);
            s += 8;
            اكسر;
        حالة c_m3Type_i64:
            *(i64*)(s) = va_arg(i_دليلون, i64);
            s += 8;
            اكسر;
        حالة c_m3Type_funcref:
        حالة c_m3Type_externref:
        حالة c_m3Type_exnref:
            *(uintptr_t*)(s) = va_arg(i_دليلون, uintptr_t);
            s += 8;
            اكسر;
#إذا d_m3HasFloat
        حالة c_m3Type_f32:
            *(f32*)(s) = (f32)va_arg(i_دليلون, f64);
            s += 8;
            اكسر; // f32 is passed as f64
        حالة c_m3Type_f64:
            *(f64*)(s) = va_arg(i_دليلون, f64);
            s += 8;
            اكسر;
#جيڪڏهن_پورو
        افتراضي: أرجع "unknown argument type";
        }
    }

    نتيجو = لاڳيتو_ڪوڊ_Checked(runtime, i_function);
    Report_Native_ڍير_Usage();

    runtime->آخري_Called = نتيجو ? عدم : i_function;

    _catch: أرجع نتيجو;
}

M3_نتيجو m3_Call (IM3Function i_function, uint32_t i_argc, ثابت فراغ* i_argptrs[])
{
    IM3Runtime  runtime = i_function->module->runtime;
    IM3FuncType ftype   = i_function->funcType;
    M3_نتيجو    نتيجو  = m3Err_none;
    u8*         s       = عدم;

    إذا (i_argc != ftype->انگ_دليلون) {
        أرجع m3_Err_دليل_ڳڻپ_Mismatch;
    }
    إذا (!i_function->compiled) {
        أرجع m3_Err_missing_Compiled_ڪوڊ;
    }

#إذا d_m3_رڪارڊ_Backtraces
    ClearBacktrace(runtime);
#جيڪڏهن_پورو

    m3_ڍير_Check_Init();

_   (check_شروعات_Function(i_function->module))

_   (Get_ڍير_اشارو_For_دليلون(i_function, &s))

    لكل (u32 i = 0; i < ftype->انگ_دليلون; ++i) {
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
#جيڪڏهن_پورو
        افتراضي: أرجع "unknown argument type";
        }
    }

    نتيجو = لاڳيتو_ڪوڊ_Checked(runtime, i_function);

    Report_Native_ڍير_Usage();

    runtime->آخري_Called = نتيجو ? عدم : i_function;

    _catch: أرجع نتيجو;
}

// Argument parsing for m3_CallArgv. Strict on purpose: the whole string has to
// be consumed, so "12abc" is rejected rather than read as 12, and an empty or
// unparsable argument is an error rather than the zero that strtoul with a
// NULL end pointer used to hand back. See wasm3/wasm3#367.
ساكن
M3_نتيجو ParseArgInteger (ccstr_t i_arg, u32 i_انگ_بٽ, u64* o_قدر)
{
    إذا (not i_arg or not *i_arg) {
        أرجع "empty argument";
    }

    // strtoull would skip leading space, but trailing space is rejected below;
    // accepting one and not the other would just be confusing
    إذا (خال_آهي((بنانشان حرف)*i_arg)) {
        أرجع "argument is not a number";
    }

    حرف* پڇاڙي = عدم;
    u64   قدر;

    errno = 0;

    // an argument may be spelled signed or unsigned: -1 and 4294967295 name the
    // same i32
    إذا (*i_arg == '-') {
        i64 signed_قدر = strtoll(i_arg, &پڇاڙي, 10);

        إذا (i_انگ_بٽ == 32 and (signed_قدر < INT32_MIN or signed_قدر > INT32_MAX)) {
            أرجع "argument out of range";
        }

        قدر = (u64)signed_قدر;
    } وإلا {
        قدر = strtoull(i_arg, &پڇاڙي, 10);

        إذا (i_انگ_بٽ == 32 and قدر > UINT32_MAX) {
            أرجع "argument out of range";
        }
    }

    إذا (errno == ERANGE) {
        أرجع "argument out of range";
    }

    إذا (پڇاڙي == i_arg or *پڇاڙي) {
        أرجع "argument is not a number";
    }

    *o_قدر = قدر;

    أرجع m3Err_none;
}


#إذا d_m3HasFloat
ساكن
M3_نتيجو ParseArgFloat (ccstr_t i_arg, f64* o_قدر)
{
    إذا (not i_arg or not *i_arg) {
        أرجع "empty argument";
    }

    إذا (خال_آهي((بنانشان حرف)*i_arg)) {
        أرجع "argument is not a number";
    }

    حرف* پڇاڙي = عدم;

    errno = 0;

    f64 قدر = strtod(i_arg, &پڇاڙي);

    إذا (پڇاڙي == i_arg or *پڇاڙي) {
        أرجع "argument is not a number";
    }

    // strtod reports underflow through ERANGE as well, and a denormal result is
    // perfectly usable, so only an overflow to infinity is out of range
    إذا (errno == ERANGE and (قدر > DBL_MAX or قدر < -DBL_MAX)) {
        أرجع "argument out of range";
    }

    *o_قدر = قدر;

    أرجع m3Err_none;
}
#جيڪڏهن_پورو


// A reference argument is either the null reference or a host handle written as
// an integer.
ساكن
M3_نتيجو ParseArgReference (ccstr_t i_arg, u64* o_قدر)
{
    إذا (i_arg and strcmp(i_arg, "null") == 0) {
        *o_قدر = 0;
        أرجع m3Err_none;
    }

    أرجع ParseArgInteger(i_arg, 64, o_قدر);
}


M3_نتيجو m3_CallArgv (IM3Function i_function, uint32_t i_argc, ثابت حرف* i_argv[])
{
    IM3FuncType ftype   = i_function->funcType;
    IM3Runtime  runtime = i_function->module->runtime;
    M3_نتيجو    نتيجو  = m3Err_none;
    u8*         s       = عدم;

    إذا (i_argc != ftype->انگ_دليلون) {
        أرجع m3_Err_دليل_ڳڻپ_Mismatch;
    }
    إذا (!i_function->compiled) {
        أرجع m3_Err_missing_Compiled_ڪوڊ;
    }

#إذا d_m3_رڪارڊ_Backtraces
    ClearBacktrace(runtime);
#جيڪڏهن_پورو

    m3_ڍير_Check_Init();

_   (check_شروعات_Function(i_function->module))

_   (Get_ڍير_اشارو_For_دليلون(i_function, &s))

    لكل (u32 i = 0; i < ftype->انگ_دليلون; ++i) {
        u64 قدر = 0;
#إذا d_m3HasFloat
        f64 fvalue = 0;
#جيڪڏهن_پورو
        بدل (d_FuncArgType(ftype, i)) {
        حالة c_m3Type_i32:
_           (ParseArgInteger(i_argv[i], 32, &قدر)) * (i32*)(s) = (i32)قدر;
            s += 8;
            اكسر;
        حالة c_m3Type_i64:
_           (ParseArgInteger(i_argv[i], 64, &قدر)) * (i64*)(s) = (i64)قدر;
            s += 8;
            اكسر;
        حالة c_m3Type_funcref:
        حالة c_m3Type_externref:
        حالة c_m3Type_exnref:
_           (ParseArgReference(i_argv[i], &قدر)) * (uintptr_t*)(s) = (uintptr_t)قدر;
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
#جيڪڏهن_پورو
        افتراضي: _throw("unknown argument type");
        }
    }

    نتيجو = لاڳيتو_ڪوڊ_Checked(runtime, i_function);

    Report_Native_ڍير_Usage();

    runtime->آخري_Called = نتيجو ? عدم : i_function;

    _catch: أرجع نتيجو;
}


//u8 * AlignStackPointerTo64Bits (const u8 * i_stack)
//{
//    uintptr_t ptr = (uintptr_t) i_stack;
//    return (u8 *) ((ptr + 7) & ~7);
//}


M3_نتيجو m3_GetResults (IM3Function i_function, uint32_t i_retc, ثابت فراغ* o_retptrs[])
{
    IM3FuncType ftype   = i_function->funcType;
    IM3Runtime  runtime = i_function->module->runtime;

    إذا (i_retc != ftype->انگ_Rets) {
        أرجع m3_Err_دليل_ڳڻپ_Mismatch;
    }
    إذا (i_function != runtime->آخري_Called) {
        أرجع "function not called";
    }

    u8* s = (u8*)runtime->ڍير;

    لكل (u32 i = 0; i < ftype->انگ_Rets; ++i) {
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
#جيڪڏهن_پورو
        افتراضي: أرجع "unknown return type";
        }
    }
    أرجع m3Err_none;
}

M3_نتيجو m3_GetResultsV (IM3Function i_function, ...)
{
    va_list ap;
    va_start(ap, i_function);
    M3_نتيجو r = m3_GetResultsVL(i_function, ap);
    va_end(ap);
    أرجع r;
}

M3_نتيجو m3_GetResultsVL (IM3Function i_function, va_list o_rets)
{
    IM3Runtime  runtime = i_function->module->runtime;
    IM3FuncType ftype   = i_function->funcType;

    إذا (i_function != runtime->آخري_Called) {
        أرجع "function not called";
    }

    u8* s = (u8*)runtime->ڍير;
    لكل (u32 i = 0; i < ftype->انگ_Rets; ++i) {
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
#جيڪڏهن_پورو
        افتراضي: أرجع "unknown argument type";
        }
    }
    أرجع m3Err_none;
}

فراغ Release_ڪوڊ_صفحو_No_Track (IM3Runtime i_runtime, IM3_ڪوڊ_صفحو i_ڪوڊ_صفحو)
{
    إذا (i_ڪوڊ_صفحو) {
        IM3_ڪوڊ_صفحو* list;

        bool صفحو_ڀريل = (انگ_Free_سٽون(i_ڪوڊ_صفحو) < d_m3_ڪوڊ_صفحو_Free_سٽون_حد);
        إذا (صفحو_ڀريل) {
            list = &i_runtime->صفحا_ڀريل;
        } وإلا {
            list = &i_runtime->صفحا_Open;
        }

        ڌڪو_ڪوڊ_صفحو(list, i_ڪوڊ_صفحو);                         m3log (emit, "release page: %d to queue: '%s'", i_ڪوڊ_صفحو->info.sequence, صفحو_ڀريل ? "full" : "open")
    }
}


IM3_ڪوڊ_صفحو Acquire_ڪوڊ_صفحو_With_صلاحيت (IM3Runtime i_runtime, u32 i_min_سٽ_ڳڻپ)
{
    IM3_ڪوڊ_صفحو صفحو = Remove_ڪوڊ_صفحو_جو_صلاحيت(&i_runtime->صفحا_Open, i_min_سٽ_ڳڻپ);

    إذا (not صفحو) {
        صفحو = Environment_Acquire_ڪوڊ_صفحو(i_runtime->environment, i_min_سٽ_ڳڻپ);

        إذا (not صفحو) {
            صفحو = New_ڪوڊ_صفحو(i_runtime, i_min_سٽ_ڳڻپ);
        }

        إذا (صفحو) {
            i_runtime->انگ_ڪوڊ_صفحا++;
        }
    }

    إذا (صفحو) {                                                  m3log (emit, "acquire page: %d", صفحو->info.sequence);
        i_runtime->انگ_سرگرم_ڪوڊ_صفحا++;
    }

    أرجع صفحو;
}


IM3_ڪوڊ_صفحو Acquire_ڪوڊ_صفحو (IM3Runtime i_runtime)
{
    أرجع Acquire_ڪوڊ_صفحو_With_صلاحيت(i_runtime, d_m3_ڪوڊ_صفحو_Free_سٽون_حد);
}


فراغ Release_ڪوڊ_صفحو (IM3Runtime i_runtime, IM3_ڪوڊ_صفحو i_ڪوڊ_صفحو)
{
    إذا (i_ڪوڊ_صفحو) {
        Release_ڪوڊ_صفحو_No_Track(i_runtime, i_ڪوڊ_صفحو);
        i_runtime->انگ_سرگرم_ڪوڊ_صفحا--;

#إذا defined(DEBUG)
        u32 انگ_Open = ڳڻپ_ڪوڊ_صفحا(i_runtime->صفحا_Open);
        u32 انگ_ڀريل = ڳڻپ_ڪوڊ_صفحا(i_runtime->صفحا_ڀريل);

        m3log(runtime, "runtime: %p; open-pages: %d; full-pages: %d; active: %d; total: %d", i_runtime, انگ_Open, انگ_ڀريل, i_runtime->انگ_سرگرم_ڪوڊ_صفحا, i_runtime->انگ_ڪوڊ_صفحا);

        d_m3Assert(انگ_Open + انگ_ڀريل + i_runtime->انگ_سرگرم_ڪوڊ_صفحا == i_runtime->انگ_ڪوڊ_صفحا);

#  إذا d_m3_Log_ڪوڊ_صفحا
        dump_ڪوڊ_صفحو(i_ڪوڊ_صفحو, /* startPC: */ عدم);
#  endif
#جيڪڏهن_پورو
    }
}


#إذا d_m3VerboseErrorMessages
M3_نتيجو m3Error (M3_نتيجو i_نتيجو, IM3Runtime i_runtime, IM3Module i_module, IM3Function i_function,
                  ثابت حرف* ثابت i_file, u32 i_سٽ_انگ, ثابت حرف* ثابت i_error_پيغام, ...)
{
    إذا (i_runtime) {
        i_runtime->error         = (M3ErrorInfo){ .نتيجو = i_نتيجو, .runtime = i_runtime, .module = i_module, .function = i_function, .file = i_file, .سٽ_2 = i_سٽ_انگ };
        i_runtime->error.پيغام = i_runtime->error_پيغام;

        va_list دليلون;
        va_start(دليلون, i_error_پيغام);
        vsnprintf(i_runtime->error_پيغام, حجم(i_runtime->error_پيغام), i_error_پيغام, دليلون);
        va_end(دليلون);
    }

    أرجع i_نتيجو;
}
#جيڪڏهن_پورو


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
        i_runtime->error.پيغام = "";
    }
}

uint8_t* m3_Get_يادگيري (IM3Module i_module, size_t* o_يادگيري_سائيز_اندريون_بائيٽون, uint32_t i_يادگيري_اشاريو)
{
    uint8_t* يادگيري = عدم;
    size_t   سائيز   = 0;

    إذا (i_module and i_يادگيري_اشاريو < i_module->انگ_Memories) {
        IM3_يادگيري mem = i_module->memories[i_يادگيري_اشاريو];

        إذا (mem->mallocated) {
            سائيز = mem->mallocated->ڊيگهائي;

            إذا (سائيز) {
                يادگيري = m3_Mem_معلومات(mem->mallocated);
            }
        }
    }

    إذا (o_يادگيري_سائيز_اندريون_بائيٽون) {
        *o_يادگيري_سائيز_اندريون_بائيٽون = سائيز;
    }

    أرجع يادگيري;
}


size_t m3_Get_يادگيري_سائيز (IM3Module i_module, uint32_t i_يادگيري_اشاريو)
{
    إذا (not i_module or i_يادگيري_اشاريو >= i_module->انگ_Memories) {
        أرجع 0;
    }

    IM3_يادگيري mem = i_module->memories[i_يادگيري_اشاريو];

    أرجع mem->mallocated ? mem->mallocated->ڊيگهائي : 0;
}


size_t m3_Get_يادگيري_سائيز_At (ثابت فراغ* i_يادگيري)
{
    إذا (not i_يادگيري) {
        أرجع 0;
    }

    // the header sits immediately before the data it describes
    ثابت M3_يادگيري_سرو* سرو = ((ثابت M3_يادگيري_سرو*)i_يادگيري) - 1;

    أرجع سرو->ڊيگهائي;
}


M3_نتيجو m3_ڳوليو_Exported_يادگيري (IM3Module i_module, cstr_t i_name, u32* o_يادگيري_اشاريو)
{
    إذا (not i_module or not i_name or not o_يادگيري_اشاريو) {
        أرجع m3_Err_unknown_يادگيري;
    }

    ثابت M3Export* e = Module_ڳوليو_Export(i_module, d_external_قسم_يادگيري, i_name);

    إذا (not e) {
        أرجع m3_Err_unknown_يادگيري;
    }

    *o_يادگيري_اشاريو = e->index;
    أرجع m3Err_none;
}


bool Module_HasLinkedHostImport (IM3Module i_module, cstr_t i_importModule)
{
    لكل (u32 i = 0; i < i_module->انگ_Functions; ++i) {
        IM3Function f = &i_module->functions[i];

        // hostMemory is set when a host function is bound to the import, so it
        // is also the mark of one - see CompileRawFunction
        إذا (not f->host_يادگيري or f->wasm or f->resolved) {
            استمر;
        }

        إذا (f->import.moduleUtf8 and strcmp(f->import.moduleUtf8, i_importModule) == 0) {
            أرجع true;
        }
    }

    أرجع false;
}


M3_نتيجو m3_Bind_Import_يادگيري (IM3Module io_module, cstr_t i_importModule, u32 i_يادگيري_اشاريو)
{
    إذا (not io_module or not i_importModule) {
        أرجع m3Err_moduleNotLinked;
    }

    إذا (i_يادگيري_اشاريو >= io_module->انگ_Memories) {
        أرجع m3_Err_unknown_يادگيري;
    }

    // the memory itself, not the index: linking may have pointed this slot at
    // another module's memory, and growing one reallocates behind it
    IM3_يادگيري يادگيري = io_module->memories[i_يادگيري_اشاريو];

    ثابت bool wildcardModule = (strcmp(i_importModule, "*") == 0);

    لكل (u32 i = 0; i < io_module->انگ_Functions; ++i) {
        IM3Function f = &io_module->functions[i];

        // only an import that a host function was bound to has a memory to
        // pin: one with a body of its own reads memory through the interpreter,
        // and one resolved to another module's export runs that module's code
        إذا (not f->host_يادگيري or f->wasm or f->resolved) {
            استمر;
        }

        إذا (wildcardModule or (f->import.moduleUtf8 and strcmp(f->import.moduleUtf8, i_importModule) == 0)) {
            f->host_يادگيري = يادگيري;
        }
    }

    أرجع m3Err_none;
}


M3BacktraceInfo* m3_GetBacktrace (IM3Runtime i_runtime)
{
#إذا d_m3_رڪارڊ_Backtraces
    أرجع &i_runtime->backtrace;
#وإلا
    أرجع عدم;
#جيڪڏهن_پورو
}


//-------------------------------------------------------------------------------------------------------------------------------
//  snapshots & suspendable execution
//-------------------------------------------------------------------------------------------------------------------------------

فراغ m3_سيٽ_Suspendable (IM3Runtime io_runtime, bool i_suspendable)
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
#إذا d_m3_Has_ڍير_Switching
    إذا (i_runtime->پاڙ_Continuation && i_runtime->پاڙ_Continuation->state == cont_suspended) {
        أرجع true;
    }
#جيڪڏهن_پورو
    أرجع false;
}

M3_نتيجو m3_ResumeRuntime (IM3Runtime io_runtime)
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
    إذا (io_runtime->پاڙ_Continuation and io_runtime->پاڙ_Continuation->داخلا_Function and
        io_runtime->پاڙ_Continuation->داخلا_Function->module->isUnusable) {
        أرجع "a snapshot failed to restore into the module, so it cannot run";
    }
#جيڪڏهن_پورو
#إذا d_m3_Has_ڍير_Switching
    إذا (!io_runtime->پاڙ_Continuation || io_runtime->پاڙ_Continuation->state != cont_suspended) {
        أرجع m3Err_none;
    }
    // what finishes returns into the bottom of the stack, where it was entered
    io_runtime->ڍير = io_runtime->origin_ڍير;

    // this is the outermost call again, as far as a host function calling back
    // into Wasm from inside it can tell
    io_runtime->entered = io_runtime->پاڙ_Continuation->داخلا_Function;
    io_runtime->callNesting++;

    d_m3_ڍير_حد_بندي_Enter(io_runtime);
    io_runtime->resumePastCheck = true;
    M3_نتيجو نتيجو             = (M3_نتيجو)ResumeContinuation(io_runtime, io_runtime->پاڙ_Continuation);
    io_runtime->resumePastCheck = false;
    d_m3_ڍير_حد_بندي_Leave(io_runtime);

#  إذا d_m3HasExceptionHandling
    إذا (M3_UNLIKELY(نتيجو == m3Err_pendingException)) {
        نتيجو = m3Err_trapUncaughtException;
    }
    رڪارڊ_Uncaught_Exception(io_runtime, نتيجو);
#  endif

    io_runtime->callNesting--;

#  إذا d_m3HasExceptionHandling
    إذا (نتيجو != m3Err_continuationSuspended and io_runtime->callNesting == 0) {
        FreeExceptions(io_runtime);
    }
#  endif

    // the results are where the paused call left room for them, so a finished
    // resume can be asked for them the way the call itself could have been
    io_runtime->آخري_Called = (نتيجو == m3Err_none) ? io_runtime->پاڙ_Continuation->داخلا_Function : عدم;

    Runtime_رکو_Call_ڍير(io_runtime);
    أرجع نتيجو;
#وإلا
    أرجع m3Err_none;
#جيڪڏهن_پورو
}
