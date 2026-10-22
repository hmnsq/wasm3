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
            لكل (u8 t = c_m3Type_none; t < c_m3_Type_شمېر; t++) {
                إذا (t == c_m3Type_v128) {
                    استمر;
                }

                IM3FuncType ftype;
_               (AllocFuncType(&ftype, 1));

                ftype->عدد_دلیلونه  = 0;
                ftype->عدد_Rets  = (t == c_m3Type_none) ? 0 : 1;
                ftype->types[0] = t;

                Environment_جمع_کړه_Func_Type(env, &ftype);

                env->retFuncTypes[t] = ftype;
            }
        }

        _catch:
        إذا (پایله) {
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
        IM3FuncType بل = ftype->بل;
        m3_Free(ftype);
        ftype = بل;
    }

    m3log(runtime, "freeing %d pages from environment", شمېر_کوډ_پاڼې(i_environment->پاڼې_Released));
    Free_کوډ_پاڼې(&i_environment->پاڼې_Released);
}


فراغ m3_FreeEnvironment (IM3Environment i_environment)
{
    إذا (i_environment) {
        Environment_Release(i_environment);
        m3_Free(i_environment);
    }
}


فراغ m3_سېټ_Custom_Section_Handler (IM3Environment i_environment, M3SectionHandler i_handler)
{
    إذا (i_environment) {
        i_environment->customSectionHandler = i_handler;
    }
}


// returns the same io_funcType or replaces it with an equivalent that's already in the type linked list
M3_پایله Environment_جمع_کړه_Func_Type (IM3Environment i_environment, IM3FuncType* io_funcType)
{
    IM3FuncType جمع_کړه_Type = *io_funcType;
    IM3FuncType newType = i_environment->funcTypes;

    طالما (newType) {
        إذا (not newType->دننه_Rec_ډله and AreFuncTypesEqual(newType, جمع_کړه_Type)) {
            m3_Free(جمع_کړه_Type);
            اكسر;
        }

        newType = newType->بل;
    }

    إذا (newType == عدم) {
        // a type index has to fit in the heap type field of an m3type_t
        إذا (i_environment->عدد_Func_Types >= d_m3_Max_Sane_Types_شمېر) {
            m3_Free(جمع_کړه_Type);
            *io_funcType = عدم;
            أرجع "too many distinct function types";
        }

        newType                  = جمع_کړه_Type;
        newType->canonical_شاخص  = i_environment->عدد_Func_Types++;
        newType->بل            = i_environment->funcTypes;
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
M3_پایله Environment_ReserveFuncTypes (IM3Environment i_environment, u32 i_شمېر, u16* o_لومړی_شاخص)
{
    إذا ((u64)i_environment->عدد_Func_Types + i_شمېر > d_m3_Max_Sane_Types_شمېر) {
        أرجع "too many distinct function types";
    }

    *o_لومړی_شاخص = i_environment->عدد_Func_Types;
    i_environment->عدد_Func_Types += (u16)i_شمېر;

    أرجع m3Err_none;
}


فراغ Environment_AdoptFuncType (IM3Environment i_environment, IM3FuncType i_funcType, u16 i_canonical_شاخص)
{
    i_funcType->دننه_Rec_ډله     = true;
    i_funcType->canonical_شاخص = i_canonical_شاخص;
    i_funcType->بل           = i_environment->funcTypes;
    i_environment->funcTypes   = i_funcType;
}


IM3_کوډ_پاڼه Remove_کوډ_پاڼه_د_ظرفیت (M3_کوډ_پاڼه** io_list, u32 i_minimum_کرښه_شمېر)
{
    IM3_کوډ_پاڼه prev = عدم;
    IM3_کوډ_پاڼه پاڼه = *io_list;

    طالما (پاڼه) {
        إذا (عدد_Free_کرښې(پاڼه) >= i_minimum_کرښه_شمېر) {             d_m3Assert (پاڼه->info.usage_شمېر == 0);
            IM3_کوډ_پاڼه بل = پاڼه->info.بل;
            إذا (prev) {
                prev->info.بل = بل; // mid-list
            } وإلا {
                *io_list = بل;       // front of list
            }

            اكسر;
        }

        prev = پاڼه;
        پاڼه = پاڼه->info.بل;
    }

    أرجع پاڼه;
}


IM3_کوډ_پاڼه Environment_Acquire_کوډ_پاڼه (IM3Environment i_environment, u32 i_minimum_کرښه_شمېر)
{
    أرجع Remove_کوډ_پاڼه_د_ظرفیت(&i_environment->پاڼې_Released, i_minimum_کرښه_شمېر);
}


فراغ Environment_Release_کوډ_پاڼې (IM3Environment i_environment, IM3_کوډ_پاڼه i_کوډ_پاڼه_List)
{
    IM3_کوډ_پاڼه پای_ټکی = i_کوډ_پاڼه_List;

    طالما (پای_ټکی) {
        پای_ټکی->info.کرښه_شاخص = 0; // reset page
#إذا d_m3_ریکارډ_Backtraces
        پای_ټکی->info.mapping->اندازه = 0;
#د_که_پای // d_m3RecordBacktraces

        IM3_کوډ_پاڼه بل = پای_ټکی->info.بل;
        إذا (not بل) {
            اكسر;
        }

        پای_ټکی = بل;
    }

    إذا (پای_ټکی) {
        // push list to front
        پای_ټکی->info.بل               = i_environment->پاڼې_Released;
        i_environment->پاڼې_Released = i_کوډ_پاڼه_List;
    }
}


IM3Runtime m3_NewRuntime (IM3Environment i_environment, u32 i_ډېری_اندازه_دننه_بایټونه, فراغ* i_userdata)
{
    IM3Runtime runtime = m3_AllocStruct(M3Runtime);

    إذا (runtime) {
        m3_ResetErrorInfo(runtime);

        runtime->environment = i_environment;
        runtime->userdata    = i_userdata;

        Deterministic_InitRuntime(runtime);

        runtime->origin_ډېری = m3_Malloc("Wasm Stack", i_ډېری_اندازه_دننه_بایټونه + 4 * حجم(m3slot_t)); // TODO: more precise stack checks

        إذا (runtime->origin_ډېری) {
            runtime->ډېری         = runtime->origin_ډېری;
            runtime->عدد_ډېری_Slots = i_ډېری_اندازه_دننه_بایټونه / حجم(m3slot_t);          m3log (runtime, "new stack: %p, slots: %u", runtime->origin_ډېری, runtime->عدد_ډېری_Slots);
        } وإلا {
            m3_Free(runtime);
        }
    }

    أرجع runtime;
}

فراغ m3_سېټ_Validation (IM3Runtime i_runtime, bool i_enable)
{
#إذا d_m3EnableValidation
    إذا (i_runtime) {
        i_runtime->skipValidation = not i_enable;
    }
#وإلا
    (فراغ)i_runtime;
    (فراغ)i_enable;              // nothing to skip: the validator was compiled out
#د_که_پای
}

M3_پایله m3_سېټ_Resource_حد (IM3Runtime i_runtime, M3_Resource_حد i_حد, uint64_t i_ارزښت)
{
    إذا (not i_runtime) {
        أرجع m3Err_mallocFailed;
    }
    بدل (i_حد) {
    حالة c_m3_حد_حافظه_بایټونه:
        إذا (i_ارزښت and i_ارزښت < i_runtime->حافظه_بایټونه_کارول_شوی) {
            أرجع m3_Err_resource_حد_لاندې_Usage;
        }
        i_runtime->حافظه_بایټونه_حد = i_ارزښت;
        أرجع m3Err_none;
    حالة c_m3_حد_جدول_Elements:
        إذا (i_ارزښت and i_ارزښت < i_runtime->جدول_Elements_کارول_شوی) {
            أرجع m3_Err_resource_حد_لاندې_Usage;
        }
        i_runtime->جدول_Elements_حد = i_ارزښت;
        أرجع m3Err_none;
#إذا d_m3_Has_ډېری_Switching
    حالة c_m3_حد_Continuations:
        إذا (i_ارزښت and i_ارزښت < i_runtime->continuationsAllocated) {
            أرجع m3_Err_resource_حد_لاندې_Usage;
        }
        i_runtime->continuations_حد = (u32)M3_MIN(i_ارزښت, (u64)UINT32_MAX);
        طالما (i_ارزښت and i_runtime->val_ډېری_Pool_شمېر > i_ارزښت - i_runtime->continuationsAllocated) {
            m3_Free(i_runtime->val_ډېری_Pool[--i_runtime->val_ډېری_Pool_شمېر]);
        }
        أرجع m3Err_none;
#وإلا
    حالة c_m3_حد_Continuations: أرجع m3_Err_resource_حد_Not_Supported;
#د_که_پای
    حالة c_m3_حد_Gas_Units:
#إذا d_m3HasGasMetering
        i_runtime->gas_حد = i_runtime->gas_پاتې_شوی = (i64)M3_MIN(i_ارزښت, (u64)INT64_MAX);
        أرجع m3Err_none;
#وإلا
        أرجع m3_Err_resource_حد_Not_Supported;
#د_که_پای
    افتراضي: أرجع m3_Err_unknown_Resource_حد;
    }
}

uint64_t m3_Get_Resource_حد (IM3Runtime i_runtime, M3_Resource_حد i_حد)
{
    إذا (not i_runtime) {
        أرجع 0;
    }
    بدل (i_حد) {
    حالة c_m3_حد_حافظه_بایټونه: أرجع i_runtime->حافظه_بایټونه_حد;
    حالة c_m3_حد_جدول_Elements: أرجع i_runtime->جدول_Elements_حد;
#إذا d_m3_Has_ډېری_Switching
    حالة c_m3_حد_Continuations: أرجع i_runtime->continuations_حد;
#د_که_پای
#إذا d_m3HasGasMetering
    حالة c_m3_حد_Gas_Units: أرجع (u64)i_runtime->gas_حد;
#د_که_پای
    افتراضي: أرجع 0;
    }
}

uint64_t m3_GetResourceUsage (IM3Runtime i_runtime, M3_Resource_حد i_حد)
{
    إذا (not i_runtime) {
        أرجع 0;
    }
    بدل (i_حد) {
    حالة c_m3_حد_حافظه_بایټونه: أرجع i_runtime->حافظه_بایټونه_کارول_شوی;
    حالة c_m3_حد_جدول_Elements: أرجع i_runtime->جدول_Elements_کارول_شوی;
#إذا d_m3_Has_ډېری_Switching
    حالة c_m3_حد_Continuations: أرجع i_runtime->continuationsAllocated;
#د_که_پای
#إذا d_m3HasGasMetering
    حالة c_m3_حد_Gas_Units: أرجع (u64)i_runtime->gas_حد - (u64)i_runtime->gas_پاتې_شوی;
#د_که_پای
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
        IM3Module بل = module->بل;
        إذا ((r = i_visitor(module, i_info))) {
            اكسر;
        }

        module = بل;
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
    لكل (u32 i = 0; i < i_module->عدد_Memories; ++i) {
        IM3_حافظه حافظه = i_module->memories[i];

        إذا (حافظه and حافظه->owner != i_module) {
            i_module->memories[i] = عدم;
        }
    }

    لكل (u32 i = 0; i < i_module->عدد_Tables; ++i) {
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
    ForEachModule(i_runtime, _FreeModule, عدم);                    d_m3Assert (i_runtime->عدد_فعال_کوډ_پاڼې == 0);

    Environment_Release_کوډ_پاڼې(i_runtime->environment, i_runtime->پاڼې_Open);
    Environment_Release_کوډ_پاڼې(i_runtime->environment, i_runtime->پاڼې_ډک);

    m3_Free(i_runtime->origin_ډېری);

#إذا d_m3EnableValidation
    m3_Free(i_runtime->validator);
#د_که_پای

#إذا d_m3HasExceptionHandling
    // a runtime let go of while suspended still holds its exceptions
    FreeExceptions(i_runtime);
    m3_Free(i_runtime->uncaughtException);
#د_که_پای
#إذا d_m3HasExceptionHandling || d_m3_Has_ډېری_Switching
    طالما (i_runtime->host_نښانونه) {
        M3_Host_نښان* بل = i_runtime->host_نښانونه->بل;
        m3_Free(i_runtime->host_نښانونه);
        i_runtime->host_نښانونه = بل;
    }
#د_که_پای
#إذا d_m3_Has_ډېری_Switching
    Continuation_ReleaseAll(i_runtime);
#د_که_پای
}


فراغ m3_FreeRuntime (IM3Runtime i_runtime)
{
    إذا (i_runtime) {
        m3_PrintProfilerInfo();

        Runtime_Release(i_runtime);
        m3_Free(i_runtime);
    }
}


#إذا d_m3_Has_ډېری_Switching

IM3Continuation Continuation_New (IM3Runtime i_runtime, IM3FuncType i_type, IM3Function i_function)
{
    IM3Continuation cont = m3_AllocStruct(M3Continuation);
    إذا (!cont) {
        أرجع عدم;
    }

    cont->type          = i_type;
    cont->ثبت_Function = i_function;
    cont->state         = cont_allocated;

    cont->عدد_ډېری_Slots = d_m3_Continuation_ډېری_Slots;
    // frames are recorded only when this continuation actually suspends, and
    // only as deep as it got, so the array starts out unallocated
    cont->چوکاټونه    = عدم;
    cont->چوکاټونه_Cap = 0;
    cont->عدد_چوکاټونه = 0;

    cont->sp = cont->val_ډېری;
    cont->pc = i_function ? i_function->compiled : عدم;
    cont->r0 = 0;
#  إذا d_m3HasFloat
    cont->fp0 = 0;
#  endif
    cont->bound_دلیلونه_شمېر    = 0;
    cont->عدد_Suspend_Results = 0;
    cont->عدد_Handlers       = 0;
    cont->handlersPC        = عدم;
    cont->پلار            = عدم;

    cont->بل               = i_runtime->continuations;
    i_runtime->continuations = cont;

    أرجع cont;
}

IM3Continuation Continuation_ForkSuspended (IM3Runtime i_runtime, IM3Continuation i_cont)
{
    IM3Continuation newCont = Continuation_New(i_runtime, i_cont->type, i_cont->ثبت_Function);
    إذا (M3_UNLIKELY(not newCont)) {
        أرجع عدم;
    }

    newCont->val_ډېری      = i_cont->val_ډېری;
    newCont->عدد_ډېری_Slots = i_cont->عدد_ډېری_Slots;
    newCont->sp            = i_cont->sp;
    newCont->pc            = i_cont->pc;
    newCont->suspend_ټکی  = i_cont->suspend_ټکی;
    newCont->r0            = i_cont->r0;
#  إذا d_m3HasFloat
    newCont->fp0 = i_cont->fp0;
#  endif
    newCont->چوکاټونه    = i_cont->چوکاټونه;
    newCont->چوکاټونه_Cap = i_cont->چوکاټونه_Cap;
    newCont->عدد_چوکاټونه = i_cont->عدد_چوکاټونه;

    newCont->bound_دلیلونه_شمېر = 0;
#  إذا d_m3HasExceptionHandling
    newCont->resumeThrow = i_cont->resumeThrow;
    i_cont->resumeThrow  = عدم;
#  endif
    newCont->عدد_Suspend_Results = i_cont->عدد_Suspend_Results;
    لكل (u32 i = 0; i < i_cont->عدد_Suspend_Results; ++i) {
        newCont->suspend_پایله_Offsets[i] = i_cont->suspend_پایله_Offsets[i];
        newCont->suspend_پایله_Is64[i]    = i_cont->suspend_پایله_Is64[i];
    }

    newCont->عدد_Handlers = i_cont->عدد_Handlers;
    newCont->handlersPC  = i_cont->handlersPC;
    newCont->پلار      = i_cont->پلار;
    newCont->state       = cont_suspended;

    i_cont->val_ډېری  = عدم;
    i_cont->چوکاټونه    = عدم;
    i_cont->چوکاټونه_Cap = 0;
    i_cont->عدد_چوکاټونه = 0;
    i_cont->state     = cont_consumed;

    أرجع newCont;
}

// Only cont.new and snapshot restoration acquire stacks. Binding and suspension
// transfer them between records without changing the active count.
M3_پایله Continuation_Acquire_ډېری (IM3Runtime runtime, IM3Continuation cont)
{
    إذا (not Continuation_Can_Acquire_ډېری(runtime)) {
        أرجع m3_Err_continuation_حد_Exceeded;
    }
    إذا (runtime->val_ډېری_Pool_شمېر) {
        cont->val_ډېری = runtime->val_ډېری_Pool[--runtime->val_ډېری_Pool_شمېر];
        memset(cont->val_ډېری, 0, (cont->عدد_ډېری_Slots + 4) * حجم(m3slot_t));
    } وإلا {
        cont->val_ډېری = m3_AllocArray(m3slot_t, cont->عدد_ډېری_Slots + 4);
    }
    إذا (not cont->val_ډېری) {
        أرجع m3Err_mallocFailed;
    }
    cont->sp = cont->val_ډېری;
    runtime->continuationsAllocated++;
    أرجع m3Err_none;
}

فراغ Continuation_Recycle_ډېری (IM3Runtime runtime, IM3Continuation cont)
{
    إذا (not cont->val_ډېری or cont == runtime->ریښه_Continuation) {
        أرجع;
    }
    إذا (not runtime->val_ډېری_Pool) {
        runtime->val_ډېری_Pool = m3_AllocArray(m3slot_t*, 16);
        إذا (runtime->val_ډېری_Pool) {
            runtime->val_ډېری_Pool_Cap = 16;
        }
    }
    إذا (runtime->val_ډېری_Pool_شمېر < runtime->val_ډېری_Pool_Cap and
        (not runtime->continuations_حد or runtime->val_ډېری_Pool_شمېر <
                                              runtime->continuations_حد - runtime->continuationsAllocated + 1)) {
        runtime->val_ډېری_Pool[runtime->val_ډېری_Pool_شمېر++] = cont->val_ډېری;
        cont->val_ډېری                                      = عدم;
    } وإلا {
        m3_Free(cont->val_ډېری);
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
        IM3Continuation بل = cont->بل;

        m3_Free(cont->val_ډېری);
        m3_Free(cont->چوکاټونه);
        m3_Free(cont);

        cont = بل;
    }

    لكل (u32 i = 0; i < io_runtime->val_ډېری_Pool_شمېر; ++i) {
        m3_Free(io_runtime->val_ډېری_Pool[i]);
    }
    m3_Free(io_runtime->val_ډېری_Pool);
    io_runtime->val_ډېری_Pool_شمېر = io_runtime->val_ډېری_Pool_Cap = 0;
    io_runtime->continuationsAllocated                          = 0;
    io_runtime->continuations                                   = عدم;
    io_runtime->فعال_Continuation                              = عدم;
    io_runtime->ریښه_Continuation                                = عدم;
}


m3ret_t Continuation_ریکارډ_چوکاټ (IM3Runtime i_runtime, ثابت M3_چوکاټ* i_چوکاټ)
{
    IM3Continuation cont = i_runtime->فعال_Continuation;

    // the marker only travels between a suspend and the resume that runs it,
    // and that whole stretch has an active continuation
    d_m3Assert(cont);

    إذا (cont->عدد_چوکاټونه == cont->چوکاټونه_Cap) {
        u32 newCap = cont->چوکاټونه_Cap ? cont->چوکاټونه_Cap * 2 : 8;

        إذا (newCap > d_m3_Continuation_Max_چوکاټونه) {
            newCap = d_m3_Continuation_Max_چوکاټونه;
        }

        إذا (newCap == cont->چوکاټونه_Cap) {
            أرجع m3_Err_trap_ډېری_ډکېدنه;
        }

        M3_چوکاټ* چوکاټونه = m3_ReallocArray(M3_چوکاټ, cont->چوکاټونه, newCap, cont->چوکاټونه_Cap);

        إذا (not چوکاټونه) {
            أرجع m3Err_mallocFailed;
        }

        cont->چوکاټونه    = چوکاټونه;
        cont->چوکاټونه_Cap = newCap;
    }

    cont->چوکاټونه[cont->عدد_چوکاټونه++] = *i_چوکاټ;

    أرجع m3Err_continuationSuspended;
}

#د_که_پای // d_m3HasStackSwitching

M3_پایله Evaluate_افاده (IM3Module i_module, فراغ* o_expressed, m3type_t i_type, بایټونه_t* io_بایټونه, cbytes_t i_پای_ټکی)
{
    M3_پایله پایله = m3Err_none;

    // OPTZ: use a simplified interpreter for expressions

    // create a temporary runtime context
#إذا defined(d_m3PreferStaticAlloc)
    ساكن M3Runtime runtime;
#وإلا
    M3Runtime runtime;
#د_که_پای
    M3_INIT(runtime);

    runtime.environment   = i_module->runtime->environment;
    runtime.عدد_ډېری_Slots = i_module->runtime->عدد_ډېری_Slots;
    runtime.ډېری         = i_module->runtime->ډېری;

    m3stack_t ډېری = (m3stack_t)runtime.ډېری;

    IM3Runtime savedRuntime = i_module->runtime;
    i_module->runtime       = &runtime;

    IM3Compilation o   = &runtime.compilation;
    o->runtime         = &runtime;
    o->module          = i_module;
    o->wasm            = *io_بایټونه;
    o->wasm_پای_ټکی         = i_پای_ټکی;
    o->وروستی_Opcode_پیل = o->wasm;

    // Borrowed for the length of this expression only. Runtime_Release below hands it
    // to Environment_ReleaseCodePages, which resets lineIndex, so the page comes back
    // empty and the next expression reuses it rather than burning fresh lines.
    o->پاڼه = Acquire_کوډ_پاڼه(&runtime);

    إذا (o->پاڼه) {
        IM3FuncType ftype = عدم;

#إذا d_m3HasTypedRefs
        // A reference type has to be checked as the type it is: the shared
        // return types are one per storage class, so borrowing one of those
        // would let any reference at all satisfy a particular one.
        إذا (IsSpelledRefType(i_type)) {
            پایله = AllocFuncType(&ftype, 1);

            إذا (not پایله) {
                ftype->عدد_Rets  = 1;
                ftype->types[0] = i_type;

                پایله = Environment_جمع_کړه_Func_Type(runtime.environment, &ftype);
            }
        } وإلا
#د_که_پای
        {
            ftype = runtime.environment->retFuncTypes[بنسټ_Type_د(i_type)];
        }

        pc_t m3code = Get_پاڼه_PC(o->پاڼه);

        إذا (not پایله) {
            پایله = Compile_افاده(o, ftype);
        }

        إذا (not پایله && o->max_ډېری_Slots >= runtime.عدد_ډېری_Slots) {
            پایله = m3_Err_trap_ډېری_ډکېدنه;
        }

        إذا (not پایله) {
#إذا (d_m3EnableOpProfiling || d_m3EnableOpTracing)
            m3ret_t r = پرلپسې_کوډ(m3code, ډېری, عدم, d_m3_Op_Default_دلیلونه, d_m3_بنسټ_Cstr);
#وإلا
            m3ret_t r = پرلپسې_کوډ(m3code, ډېری, عدم, d_m3_Op_Default_دلیلونه);
#د_که_پای

            إذا (r == 0) {                                                                   m3log (runtime, "expression result: %s", SPrint_ارزښت (ډېری, i_type));
                إذا (اندازه_د_Type(بنسټ_Type_د(i_type)) == حجم(u32)) {
                    *(u32*)o_expressed = *((u32*)ډېری);
                } وإلا {
                    *(u64*)o_expressed = *((u64*)ډېری);
                }
            }
        }

        Release_کوډ_پاڼه(&runtime, o->پاڼه);
    } وإلا {
        پایله = m3_Err_malloc_Failed_کوډ_پاڼه;
    }

    runtime.origin_ډېری = عدم;        // prevent free(stack) in ReleaseRuntime
    Runtime_Release(&runtime);
    i_module->runtime = savedRuntime;

    *io_بایټونه = o->wasm;

    أرجع پایله;
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
bool LimitsSatisfy (u64 i_exported_اندازه, bool i_exportedHasMax, u64 i_exportedMax,
                    u64 i_importMin, bool i_importHasMax, u64 i_importMax)
{
    إذا (i_exported_اندازه < i_importMin) {
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
ثابت M3Export* Module_پیدا_کړه_Export (IM3Module i_module, u8 i_ډول, cstr_t i_name)
{
    لكل (u32 i = 0; i < i_module->عدد_Exports; ++i) {
        ثابت M3Export* e = &i_module->exports[i];

        إذا (e->ډول == i_ډول and strcmp(e->name, i_name) == 0) {
            أرجع e;
        }
    }

    أرجع عدم;
}


ساكن
IM3Function Module_پیدا_کړه_Exported_Function (IM3Module i_module, cstr_t i_name)
{
    ثابت M3Export* e = Module_پیدا_کړه_Export(i_module, d_external_ډول_function, i_name);

    // A module that re-exports an import names the placeholder here. Resolve to
    // the function that actually runs, or a host-side call would run it against
    // the importing module's memory.
    أرجع e ? Function_Implementation(&i_module->functions[e->index]) : عدم;
}


ساكن
IM3_حافظه Module_پیدا_کړه_Exported_حافظه (IM3Module i_module, cstr_t i_name)
{
    ثابت M3Export* e = Module_پیدا_کړه_Export(i_module, d_external_ډول_حافظه, i_name);

    أرجع e ? i_module->memories[e->index] : عدم;
}


ساكن
IM3_جدول Module_پیدا_کړه_Exported_جدول (IM3Module i_module, cstr_t i_name)
{
    ثابت M3Export* e = Module_پیدا_کړه_Export(i_module, d_external_ډول_جدول, i_name);

    أرجع e ? i_module->tables[e->index] : عدم;
}


ساكن
IM3Global Module_پیدا_کړه_Exported_Global (IM3Module i_module, cstr_t i_name)
{
    ثابت M3Export* e = Module_پیدا_کړه_Export(i_module, d_external_ډول_global, i_name);

    أرجع e ? &i_module->globals[e->index] : عدم;
}


#إذا d_m3HasExceptionHandling || d_m3_Has_ډېری_Switching
ساكن
IM3_نښان Module_پیدا_کړه_Exported_نښان (IM3Module i_module, cstr_t i_name)
{
    ثابت M3Export* e = Module_پیدا_کړه_Export(i_module, d_external_ډول_نښان, i_name);

    إذا (not e) {
        أرجع عدم;
    }

    IM3_نښان نښان = &i_module->نښانونه[e->index];

    أرجع نښان->resolved ? نښان->resolved : نښان;
}
#د_که_پای


// Whether the module exports anything at all under this name. Export names are
// unique within a module, so a name one of the lookups above missed but this
// one finds is exported as something else - a kind the import cannot be
// satisfied by, rather than a name the module never exported.
ساكن
bool Module_HasExport (IM3Module i_module, cstr_t i_name)
{
    لكل (u32 i = 0; i < i_module->عدد_Exports; ++i) {
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
M3_پایله LinkImports (IM3Runtime io_runtime, IM3Module io_module)
{
    M3_پایله پایله = m3Err_none;

    لكل (u32 i = 0; i < io_module->عدد_Functions; ++i) {
        IM3Function f = &io_module->functions[i];

        إذا (f->wasm or not (f->import.moduleUtf8 and f->import.ډګر_Utf8)) {
            استمر;
        }

        IM3Module from = m3_پیدا_کړه_Module(io_runtime, f->import.moduleUtf8);
        إذا (not from) {
            استمر;
        }

        IM3Function exported = Module_پیدا_کړه_Exported_Function(from, f->import.ډګر_Utf8);
        إذا (not exported) {
            // the import names a module that is loaded, so its exports settle the
            // question: a name it exports as something else is a type mismatch, and
            // one it does not export at all is an unknown import
            _throwif(m3Err_incompatibleImportType, Module_HasExport(from, f->import.ډګر_Utf8));
            _throw(m3Err_unknownImport);
        }

        // func types are canonical within an environment, so this is the
        // structural equivalence the spec asks for
        _throwif(m3Err_incompatibleImportType, exported->funcType != f->funcType);

        f->resolved = Function_Implementation(exported);
    }

    لكل (u32 i = 0; i < io_module->عدد_Memories; ++i) {
        IM3_حافظه حافظه = io_module->memories[i];

        إذا (not حافظه->imported or حافظه->owner != io_module) {
            استمر;
        }

        IM3Module from = m3_پیدا_کړه_Module(io_runtime, حافظه->import.moduleUtf8);
        إذا (not from) {
            استمر;
        }

        IM3_حافظه exported = Module_پیدا_کړه_Exported_حافظه(from, حافظه->import.ډګر_Utf8);
        إذا (not exported) {
            _throwif(m3Err_incompatibleImportType, Module_HasExport(from, حافظه->import.ډګر_Utf8));
            _throw(m3Err_unknownImport);
        }

        // the address type is part of the memory type, so an i64 memory does
        // not satisfy an i32 import, or the other way round - and so is the page
        // size, which custom page sizes made a declared property of a memory
        _throwif(m3Err_incompatibleImportType, exported->isMemory64 != حافظه->isMemory64);
        _throwif(m3Err_incompatibleImportType, حافظه_پاڼه_اندازه(exported) != حافظه_پاڼه_اندازه(حافظه));

        // a shared memory is not a subtype of an unshared one, or the other way round
        _throwif(m3Err_incompatibleImportType, exported->isShared != حافظه->isShared);

        _throwif(m3Err_incompatibleImportType,
                 not LimitsSatisfy(exported->عدد_پاڼې, exported->hasMax, exported->max_پاڼې,
                                   حافظه->init_پاڼې, حافظه->hasMax, حافظه->max_پاڼې));

        // hand the slot over to the exporter's memory, and drop the placeholder
        Free_حافظه_بلاک(حافظه);
        FreeImportInfo(&حافظه->import);
        m3_Free(حافظه);

        io_module->memories[i] = exported;
    }

    لكل (u32 i = 0; i < io_module->عدد_Tables; ++i) {
        IM3_جدول جدول_2 = io_module->tables[i];

        إذا (not جدول_2->imported or جدول_2->owner != io_module) {
            استمر;
        }

        IM3Module from = m3_پیدا_کړه_Module(io_runtime, جدول_2->import.moduleUtf8);
        إذا (not from) {
            استمر;
        }

        IM3_جدول exported = Module_پیدا_کړه_Exported_جدول(from, جدول_2->import.ډګر_Utf8);
        إذا (not exported) {
            _throwif(m3Err_incompatibleImportType, Module_HasExport(from, جدول_2->import.ډګر_Utf8));
            _throw(m3Err_unknownImport);
        }

        _throwif(m3Err_incompatibleImportType, exported->type != جدول_2->type);

        // the index type is part of the table type, the same way it is for a memory
        _throwif(m3Err_incompatibleImportType, exported->isTable64 != جدول_2->isTable64);

        _throwif(m3Err_incompatibleImportType,
                 not LimitsSatisfy(exported->اندازه, exported->hasMax, exported->max_اندازه,
                                   جدول_2->init_اندازه, جدول_2->hasMax, جدول_2->max_اندازه));

        // hand the slot over to the exporter's table, and drop the placeholder
        m3_Free(جدول_2->elements);
        FreeImportInfo(&جدول_2->import);
        m3_Free(جدول_2);

        io_module->tables[i] = exported;
    }

    لكل (u32 i = 0; i < io_module->عدد_Globals; ++i) {
        IM3Global g = &io_module->globals[i];

        إذا (not g->imported or not (g->import.moduleUtf8 and g->import.ډګر_Utf8)) {
            استمر;
        }

        IM3Module from = m3_پیدا_کړه_Module(io_runtime, g->import.moduleUtf8);
        إذا (not from) {
            استمر;
        }

        IM3Global exported = Module_پیدا_کړه_Exported_Global(from, g->import.ډګر_Utf8);
        إذا (not exported) {
            _throwif(m3Err_incompatibleImportType, Module_HasExport(from, g->import.ډګر_Utf8));
            _throw(m3Err_unknownImport);
        }

        _throwif(m3Err_incompatibleImportType, exported->type != g->type);
        _throwif(m3Err_incompatibleImportType, exported->isMutable != g->isMutable);

        g->resolved = exported->resolved ? exported->resolved : exported;
    }

#إذا d_m3HasExceptionHandling || d_m3_Has_ډېری_Switching
    لكل (u32 i = 0; i < io_module->عدد_نښانونه; ++i) {
        IM3_نښان نښان = &io_module->نښانونه[i];

        إذا (not نښان->imported or not (نښان->import.moduleUtf8 and نښان->import.ډګر_Utf8)) {
            استمر;
        }

        IM3Module from = m3_پیدا_کړه_Module(io_runtime, نښان->import.moduleUtf8);
        إذا (not from) {
            استمر;
        }

        IM3_نښان exported = Module_پیدا_کړه_Exported_نښان(from, نښان->import.ډګر_Utf8);
        إذا (not exported) {
            _throwif(m3Err_incompatibleImportType, Module_HasExport(from, نښان->import.ډګر_Utf8));
            _throw(m3Err_unknownImport);
        }

        _throwif(m3Err_incompatibleImportType, exported->type != نښان->type);

        نښان->resolved = exported->resolved ? exported->resolved : exported;
    }
#د_که_پای

    _catch: أرجع پایله;
}


// Backs each of the module's memories with pages. LinkImports has already
// pointed any import it could satisfy at the exporting module's memory, and
// those are skipped here. An import nothing satisfied is still backed locally
// from its own declared limits, so that the module remains loadable.
M3_پایله Init_حافظه (IM3Runtime io_runtime, IM3Module i_module)
{
    M3_پایله پایله = m3Err_none;

    // Fixed from here on: the index space stops changing after parse, and
    // linking has already repointed any slot it was going to.
    i_module->memory0 = i_module->عدد_Memories ? i_module->memories[0]
                                              : &i_module->تشه_حافظه;

    إذا (i_module->عدد_Memories == 0) {
        // nothing addressable, but _mem still has to point somewhere
        i_module->تشه_حافظه.owner    = i_module;
        i_module->تشه_حافظه.پاڼه_اندازه = d_m3_Default_Mem_پاڼه_اندازه;

_       (Resize_حافظه(io_runtime, &i_module->تشه_حافظه, 0));
    }

    لكل (u32 i = 0; i < i_module->عدد_Memories; ++i) {
        IM3_حافظه حافظه = i_module->memories[i];

        // a slot that already points at another module's memory is that
        // module's to allocate
        إذا (حافظه->owner != i_module or حافظه->mallocated) {
            استمر;
        }

        u32 پاڼه_اندازه = حافظه_پاڼه_اندازه(حافظه);

        حافظه->پاڼه_اندازه = پاڼه_اندازه;

        // Without a declared maximum a memory may grow to the spec limit of
        // 2^|addrtype|/pagesize pages, which is the usual 65536 at the default
        // page size, 2^48 for a 64-bit memory, and a whole address space of
        // them when a page is a single byte. A declared maximum of zero is a
        // real limit, not the absence of one.
        //
        // 2^64/pagesize overflows a u64 only when a page is a single byte, and
        // a power-of-two page size divides the address space exactly, so the
        // 64-bit division below is 2^64/pagesize written so that it fits.
        إذا (not حافظه->hasMax) {
            حافظه->max_پاڼې = حافظه->isMemory64
                                 ? (پاڼه_اندازه > 1 ? (UINT64_MAX / پاڼه_اندازه) + 1 : UINT64_MAX)
                                 : (0x100000000ull / پاڼه_اندازه);
        }

_       (Resize_حافظه(io_runtime, حافظه, حافظه->init_پاڼې));
    }

    _catch: أرجع پایله;
}


M3_پایله Resize_حافظه (IM3Runtime io_runtime, IM3_حافظه حافظه, u64 i_عدد_پاڼې)
{
    M3_پایله پایله = m3Err_none;

    u64 عدد_پاڼې_منزل_Alloc = i_عدد_پاڼې;

    إذا (عدد_پاڼې_منزل_Alloc <= حافظه->max_پاڼې) {
        // A 64-bit memory may ask for up to 2^48 pages, which overflows a u64
        // of bytes. Nothing that large can be backed, so refuse it up front
        // rather than multiplying into a wrapped size.
        _throwif("linear memory limitation exceeded",
                 عدد_پاڼې_منزل_Alloc > d_m3_پته_حد / حافظه->پاڼه_اندازه);

        u64 عدد_پاڼه_بایټونه = عدد_پاڼې_منزل_Alloc * حافظه->پاڼه_اندازه;

#إذا d_m3_Max_خطي_حافظه_پاڼې > 0
        // the limit is a memory size, counted in default-sized pages; comparing
        // it against a raw page count would make it 65536 times stricter for a
        // module whose pages are one byte
        _throwif("linear memory limitation exceeded",
                 عدد_پاڼه_بایټونه > (u64)d_m3_Max_خطي_حافظه_پاڼې * d_m3_Default_Mem_پاڼه_اندازه);
#د_که_پای

        // Limit the amount of memory that gets actually allocated
        إذا (io_runtime->حافظه_حد) {
            عدد_پاڼه_بایټونه = M3_MIN(عدد_پاڼه_بایټونه, (u64)io_runtime->حافظه_حد);
        }

        u64 old_اوږدتیا = حافظه->mallocated ? حافظه->mallocated->اوږدتیا : 0;
        u64 other_کارول_شوی = io_runtime->حافظه_بایټونه_کارول_شوی - old_اوږدتیا;
        _throwif(m3_Err_حافظه_حد_Exceeded, io_runtime->حافظه_بایټونه_حد and
                                              (عدد_پاڼه_بایټونه > io_runtime->حافظه_بایټونه_حد or
                                               other_کارول_شوی > io_runtime->حافظه_بایټونه_حد - عدد_پاڼه_بایټونه));

        _throwif("linear memory limitation exceeded", عدد_پاڼه_بایټونه > (u64)SIZE_MAX - حجم(M3_حافظه_سرلیک));

#إذا d_m3_Guarded_حافظه

        // A slot holds every address a Wasm access can name, and nothing beyond
        // that: a memory larger than the data part of one would put addresses past
        // the guard, where a fault is nobody's to recognize
        _throwif("linear memory limitation exceeded", عدد_پاڼه_بایټونه > d_m3_Guarded_معلومات_بایټونه);

        // What makes an access past the end fault is the page after the memory not
        // being committed, so the end has to fall on one of the system's page
        // boundaries. A Wasm page divides evenly into them and always will; a memory
        // that declared a page size of its own may not, and then the bytes between
        // the end of the memory and the end of its last system page would answer
        // instead of trapping. Refusing to back such a memory is the honest way out:
        // the custom page sizes proposal exists for targets far too small to reserve
        // 8GiB per memory in the first place. See d_m3GuardedMemory.
        _throwif("guarded memory needs a page size the system's divides into",
                 (حافظه->پاڼه_اندازه % m3_Host_پاڼه_اندازه()) != 0);

        إذا (حافظه->guard_خونه == عدم) {
            حافظه->guard_خونه = Guard_Take_خونه();
            _throwifnull(حافظه->guard_خونه);
        }

        // Growing only ever commits more of the same slot, so the memory never
        // moves - which the interpreter does not rely on, but nothing minds
        _throwif(m3Err_mallocFailed, not Guard_Commit_خونه(حافظه->guard_خونه, (size_t)عدد_پاڼه_بایټونه));

        حافظه->mallocated = Guard_خونه_سرلیک(حافظه->guard_خونه);

#وإلا

        size_t عدد_بایټونه = (size_t)عدد_پاڼه_بایټونه + حجم(M3_حافظه_سرلیک);

        // What is really there, which is not numPages * pageSize: the limit above
        // caps the bytes without capping the page count, so a memory can be short
        // of a page - by design, since a whole 64 KiB page is often more than an
        // MCU has. Measuring the old block by the page count would overstate it,
        // and then realloc never sees the size it already has and copies the
        // whole limit again on every grow past it.
        size_t عدد_پخوانی_بایټونه = حافظه->mallocated ? حافظه->mallocated->اوږدتیا + حجم(M3_حافظه_سرلیک) : 0;

        فراغ* newMem = m3_Realloc("Wasm Linear Memory", حافظه->mallocated, عدد_بایټونه, عدد_پخوانی_بایټونه);
        _throwifnull(newMem);

        حافظه->mallocated = (M3_حافظه_سرلیک*)newMem;

#د_که_پای

#إذا d_m3LogRuntime
        M3_حافظه_سرلیک* oldMallocated = حافظه->mallocated;
#د_که_پای

        io_runtime->حافظه_بایټونه_کارول_شوی = other_کارول_شوی + عدد_پاڼه_بایټونه;
        حافظه->عدد_پاڼې            = عدد_پاڼې_منزل_Alloc;

        حافظه->mallocated->اوږدتیا  = (size_t)عدد_پاڼه_بایټونه;
        حافظه->mallocated->runtime = io_runtime;
        حافظه->mallocated->حافظه  = حافظه;

        حافظه->mallocated->max_ډېری = (m3slot_t*)io_runtime->origin_ډېری + io_runtime->عدد_ډېری_Slots;

        m3log(runtime, "resized old: %p; mem: %p; length: %zu; pages: %llu", oldMallocated, حافظه->mallocated, حافظه->mallocated->اوږدتیا, (بینښه اوږد اوږد)حافظه->عدد_پاڼې);
    } وإلا {
        پایله = m3_Err_wasm_حافظه_ډکېدنه;
    }

    _catch: أرجع پایله;
}


فراغ Free_حافظه_بلاک (IM3_حافظه io_حافظه)
{
    إذا (io_حافظه->mallocated and io_حافظه->mallocated->runtime) {
        io_حافظه->mallocated->runtime->حافظه_بایټونه_کارول_شوی -= io_حافظه->mallocated->اوږدتیا;
    }
#إذا d_m3_Guarded_حافظه
    Guard_Give_خونه(io_حافظه->guard_خونه);
    io_حافظه->guard_خونه = عدم;

    io_حافظه->mallocated = عدم;
#وإلا
    m3_Free(io_حافظه->mallocated);
#د_که_پای
}


M3_پایله InitGlobals (IM3Module io_module)
{
    M3_پایله پایله = m3Err_none;

    إذا (io_module->عدد_Globals) {
        // placing the globals in their structs isn't good for cache locality, but i don't really know what the global
        // access patterns typically look like yet.

        //          io_module->globalMemory = m3Alloc (m3reg_t, io_module->numGlobals);

        //          if (io_module->globalMemory)
        {
            لكل (u32 i = 0; i < io_module->عدد_Globals; ++i) {
                M3Global* g = &io_module->globals[i];                           m3log (runtime, "initializing global: %d", i);

                إذا (g->init_بیان) {
                    بایټونه_t پیل = g->init_بیان;

                    پایله = Evaluate_افاده(io_module, &g->i64_ارزښت, g->type, &پیل, g->init_بیان + g->init_بیان_اندازه);

                    إذا (not پایله) {
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

    أرجع پایله;
}


M3_پایله Init_معلومات_Segments (IM3Module io_module)
{
    M3_پایله پایله = m3Err_none;

    لكل (u32 i = 0; i < io_module->عدد_معلومات_Segments; ++i) {
        M3_معلومات_Segment* segment = &io_module->معلومات_Segments[i];

        // A passive segment stays available for memory.init until data.drop.
        // An active one is copied here and then counts as dropped.
        إذا (segment->isPassive) {
            استمر;
        }

        _throwif("data segment memory index out of range",
                 segment->حافظه_Region >= io_module->عدد_Memories);

        IM3_حافظه io_حافظه = io_module->memories[segment->حافظه_Region];

        _throwif("unallocated linear memory", !(io_حافظه->mallocated));

        // The offset expression has the memory's address type, and is
        // unsigned: an i32 offset of -1 is 4294967295, way out of bounds
        // rather than negative.
        u64     segment_دننه_واټن = 0;
        بایټونه_t پیل         = segment->init_بیان;

        إذا (io_حافظه->isMemory64) {
_           (Evaluate_افاده(io_module, &segment_دننه_واټن, c_m3Type_i64, &پیل, segment->init_بیان + segment->init_بیان_اندازه));
        } وإلا {
            u32 offset32;
_           (Evaluate_افاده(io_module, &offset32, c_m3Type_i32, &پیل, segment->init_بیان + segment->init_بیان_اندازه));
            segment_دننه_واټن = offset32;
        }

        m3log(runtime, "loading data segment: %d; size: %d; offset: %llu", i, segment->اندازه, (بینښه اوږد اوږد)segment_دننه_واټن);

        إذا (segment_دننه_واټن <= io_حافظه->mallocated->اوږدتیا &&
            (u64)segment->اندازه <= io_حافظه->mallocated->اوږدتیا - segment_دننه_واټن) {
            u8* dest = m3_Mem_معلومات(io_حافظه->mallocated) + segment_دننه_واټن;
            حافظه_نقل(dest, segment->معلومات, segment->اندازه);
        } وإلا {
            _throw("data segment out of bounds");
        }

        segment->dropped = true;
    }

    _catch: أرجع پایله;
}


// Turns a segment's elements into references. Element expressions are constant
// expressions restricted to ref.null/ref.func, so they're read directly rather
// than run through the compiler.
ساكن
M3_پایله ResolveElements (IM3Module io_module, M3ElementSegment* i_segment, فراغ** o_elements)
{
    M3_پایله پایله = m3Err_none;

    بایټونه_t  pos = i_segment->elements;
    cbytes_t پای_ټکی = io_module->element_Section_پای_ټکی;

    لكل (u32 e = 0; e < i_segment->عدد_Elements; ++e) {
        u32   func_شاخص;
        فراغ* ref = عدم;

        إذا (i_segment->is_بیان) {
            m3opcode_t opcode;
_           (Read_opcode(&opcode, &pos, پای_ټکی));

            إذا (opcode == c_waOp_refFunc) {
_               (ReadLEB_u32(&func_شاخص, &pos, پای_ټکی));
                _throwif("function index out of range", func_شاخص >= io_module->عدد_Functions);
                ref = Function_Implementation(&io_module->functions[func_شاخص]);
            } وإلا إذا (opcode == c_waOp_refNull) {
                i8 waType;
                u8 nullType;
_               (ReadLEB_i7(&waType, &pos, پای_ټکی));
_               (NormalizeType(&nullType, waType));
                _throwif(m3Err_typeMismatch, nullType != i_segment->type);
            } وإلا إذا (opcode == c_waOp_getGlobal) {
                // wasm 2.0 lets an element expression read an imported
                // immutable global, which is how one module seeds another's
                // table with a reference it exported.
                u32 global_شاخص;
_               (ReadLEB_u32(&global_شاخص, &pos, پای_ټکی));
                _throwif(m3_Err_globa_شاخص_بهر_د_Bounds, global_شاخص >= io_module->عدد_Globals);

                IM3Global global = &io_module->globals[global_شاخص];

                _throwif(m3_Err_globa_شاخص_بهر_د_Bounds, not global->imported);
                _throwif(m3Err_wasmMalformed, global->isMutable);
                _throwif(m3Err_typeMismatch, بنسټ_Type_د(global->type) != بنسټ_Type_د(i_segment->type));

                // read the cell the import was linked to, not the placeholder
                إذا (global->resolved) {
                    global = global->resolved;
                }

                ref = global->ref_ارزښت;
            } وإلا {
                _throw("constant expression required");
            }

_           (Read_opcode(&opcode, &pos, پای_ټکی));
            _throwif(m3Err_wasmMalformed, opcode != c_wa_Op_پای_ټکی);
        } وإلا {
_           (ReadLEB_u32(&func_شاخص, &pos, پای_ټکی));
            _throwif("function index out of range", func_شاخص >= io_module->عدد_Functions);
            ref = Function_Implementation(&io_module->functions[func_شاخص]);
        }

        o_elements[e] = ref;
    }

    _catch: أرجع پایله;
}


M3_پایله Init_جدول_And_Elements (IM3Module io_module)
{
    M3_پایله پایله = m3Err_none;

    cbytes_t پای_ټکی = io_module->element_Section_پای_ټکی;
    M3_جدول* جدول_2;

    لكل (u32 i = 0; i < io_module->عدد_Tables; ++i) {
        جدول_2 = io_module->tables[i];

        // a slot pointing at another module's table is that module's to fill
        إذا (جدول_2->owner != io_module) {
            استمر;
        }

        إذا (جدول_2->اندازه) {
            IM3Runtime runtime = io_module->runtime;
            _throwif(m3_Err_جدول_حد_Exceeded, runtime->جدول_Elements_حد and
                                                 جدول_2->اندازه > runtime->جدول_Elements_حد - runtime->جدول_Elements_کارول_شوی);
            جدول_2->elements = m3_AllocArray(فراغ*, جدول_2->اندازه);
            _throwifnull(جدول_2->elements);
            runtime->جدول_Elements_کارول_شوی += جدول_2->اندازه;

            إذا (جدول_2->init_بیان) {
                فراغ*   ارزښت = عدم;
                بایټونه_t پیل = جدول_2->init_بیان;
_               (Evaluate_افاده(io_module, &ارزښت, بنسټ_Type_د(جدول_2->type),
                                     &پیل, جدول_2->init_بیان + جدول_2->init_بیان_اندازه));

                لكل (u32 e = 0; e < جدول_2->اندازه; ++e) {
                    جدول_2->elements[e] = ارزښت;
                }
            }
        }
    }

    لكل (u32 i = 0; i < io_module->عدد_Element_Segments; ++i) {
        M3ElementSegment* segment = &io_module->elementSegments[i];

        // Declarative segments only make their functions referenceable, and
        // passive ones wait for table.init, so neither is written out here.
        إذا (segment->ډېرپېښ == c_m3Elem_declarative) {
            segment->dropped = true;
            استمر;
        }

        إذا (segment->ډېرپېښ == c_m3Elem_passive) {
            إذا (segment->عدد_Elements) {
                segment->resolved = m3_AllocArray(فراغ*, segment->عدد_Elements);
                _throwifnull(segment->resolved);
_               (ResolveElements(io_module, segment, segment->resolved));
            }
            استمر;
        }

        جدول_2 = io_module->tables[segment->جدول_شاخص];

        // The offset expression has the table's index type, and is unsigned:
        // an i32 offset of -1 is 4294967295, out of bounds rather than negative.
        u64     offset = 0;
        بایټونه_t بیان   = segment->init_بیان;

        إذا (جدول_2->isTable64) {
_           (Evaluate_افاده(io_module, &offset, c_m3Type_i64, &بیان, پای_ټکی));
        } وإلا {
            u32 offset32;
_           (Evaluate_افاده(io_module, &offset32, c_m3Type_i32, &بیان, پای_ټکی));
            offset = offset32;
        }

        _throwif("out of bounds table access",
                 offset > جدول_2->اندازه or segment->عدد_Elements > جدول_2->اندازه - offset);

_       (ResolveElements(io_module, segment, جدول_2->elements + offset));

        segment->dropped = true;
    }

    _catch: أرجع پایله;
}

M3_پایله m3_CompileModule (IM3Module io_module)
{
    M3_پایله پایله = m3Err_none;

    لكل (u32 i = 0; i < io_module->عدد_Functions; ++i) {
        IM3Function f = &io_module->functions[i];
        إذا (f->wasm and not f->compiled) {
_           (CompileFunction(f));
        }
    }

    _catch: أرجع پایله;
}

#إذا d_m3HasExceptionHandling

M3Exception* NewException (IM3Runtime io_runtime, IM3_نښان i_نښان, u32 i_عدد_دلیلونه)
{
    M3Exception* exception = (M3Exception*)m3_Malloc("M3Exception", حجم(M3Exception) + i_عدد_دلیلونه * حجم(u64));

    إذا (exception) {
        exception->نښان     = i_نښان;
        exception->عدد_دلیلونه = i_عدد_دلیلونه;
        exception->reified = false;
        exception->prev    = عدم;
        exception->بل    = io_runtime->exceptions;

        إذا (exception->بل) {
            exception->بل->prev = exception;
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
        i_exception->prev->بل = i_exception->بل;
    } وإلا {
        io_runtime->exceptions = i_exception->بل;
    }

    إذا (i_exception->بل) {
        i_exception->بل->prev = i_exception->prev;
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
        M3Exception* بل = exception->بل;
        m3_Free_Impl(exception);
        exception = بل;
    }
}


// Keeps a copy of the exception a call just ended with, for m3_GetExceptionTag,
// or forgets the last one when the call ended any other way. A copy, because
// the original is freed with the rest before the outermost call returns - and
// one a host function's nested call ended with may still be named by an
// exnref further down the stack.
ساكن
فراغ ریکارډ_Uncaught_Exception (IM3Runtime io_runtime, M3_پایله i_پایله)
{
    إذا (io_runtime->uncaughtException) {
        m3_Free(io_runtime->uncaughtException);
    }

    M3Exception* exception = io_runtime->pendingException;

    إذا (M3_UNLIKELY(i_پایله == m3Err_trapUncaughtException) and exception) {
        M3Exception* copy = (M3Exception*)m3_CopyMem(exception, حجم(M3Exception) + exception->عدد_دلیلونه * حجم(u64));

        إذا (copy) {
            copy->بل = عدم;
            copy->prev = عدم;
        }

        io_runtime->uncaughtException = copy;
    }
}

#د_که_پای // d_m3HasExceptionHandling


#إذا d_m3HasExceptionHandling || d_m3_Has_ډېری_Switching

IM3_نښان m3_پیدا_کړه_نښان (IM3Module i_module, ثابت حرف* ثابت i_نښان_Name)
{
    إذا (not i_module or not i_نښان_Name) {
        أرجع عدم;
    }

    أرجع Module_پیدا_کړه_Exported_نښان(i_module, i_نښان_Name);
}


M3_پایله m3_New_نښان (IM3Runtime io_runtime, IM3_نښان* o_نښان, ثابت حرف* ثابت i_signature)
{
    IM3FuncType type    = عدم;
    M3_Host_نښان*  host_نښان = عدم;

_try {
    _throwif("null tag", not o_نښان);
    *o_نښان = عدم;
    _throwif(m3Err_mallocFailed, not io_runtime);

_   (Signature_منزل_Func_Type(&type, i_signature));
    // tags are the same type exactly when their types are the same object
_   (Environment_جمع_کړه_Func_Type(io_runtime->environment, &type));

    host_نښان = m3_AllocStruct(M3_Host_نښان);
    _throwifnull(host_نښان);

    host_نښان->نښان.type    = type;
    host_نښان->بل        = io_runtime->host_نښانونه;
    io_runtime->host_نښانونه = host_نښان;

    *o_نښان = &host_نښان->نښان;
} _catch:
    أرجع پایله;
}


M3_پایله m3_Link_نښان (IM3Module         io_module,
                     ثابت حرف* ثابت i_moduleName,
                     ثابت حرف* ثابت i_نښان_Name,
                     IM3_نښان            i_نښان)
{
    إذا (not io_module or not i_moduleName or not i_نښان_Name or not i_نښان) {
        أرجع m3_Err_نښان_Lookup_Failed;
    }

    ثابت bool wildcardModule = (strcmp(i_moduleName, "*") == 0);
    IM3_نښان     موخه         = i_نښان->resolved ? i_نښان->resolved : i_نښان;

    M3_پایله پایله = m3_Err_نښان_Lookup_Failed;

    لكل (u32 i = 0; i < io_module->عدد_نښانونه; ++i) {
        IM3_نښان نښان = &io_module->نښانونه[i];

        إذا (نښان->imported and نښان->import.moduleUtf8 and نښان->import.ډګر_Utf8 and
            strcmp(نښان->import.ډګر_Utf8, i_نښان_Name) == 0 and
            (wildcardModule or strcmp(نښان->import.moduleUtf8, i_moduleName) == 0)) {
            إذا (نښان->type != موخه->type) {
                أرجع m3Err_incompatibleImportType;
            }

            نښان->resolved = موخه;
            پایله        = m3Err_none;
        }
    }

    أرجع پایله;
}


uint32_t m3_Get_نښان_Arg_شمېر (IM3_نښان i_نښان)
{
    أرجع i_نښان ? i_نښان->type->عدد_دلیلونه : 0;
}


M3_ارزښت_Type m3_Get_نښان_Arg_Type (IM3_نښان i_نښان, uint32_t i_شاخص)
{
    إذا (i_نښان and i_شاخص < i_نښان->type->عدد_دلیلونه) {
        أرجع (M3_ارزښت_Type)بنسټ_Type_د(d_FuncArgType(i_نښان->type, i_شاخص));
    }

    أرجع c_m3Type_none;
}

#وإلا

IM3_نښان m3_پیدا_کړه_نښان (IM3Module i_module, ثابت حرف* ثابت i_نښان_Name)
{
    أرجع عدم;
}

M3_پایله m3_New_نښان (IM3Runtime io_runtime, IM3_نښان* o_نښان, ثابت حرف* ثابت i_signature)
{
    إذا (o_نښان) {
        *o_نښان = عدم;
    }
    أرجع m3_Err_نښان_Lookup_Failed;
}

M3_پایله m3_Link_نښان (IM3Module io_module, ثابت حرف* ثابت i_moduleName, ثابت حرف* ثابت i_نښان_Name, IM3_نښان i_نښان)
{
    أرجع m3_Err_نښان_Lookup_Failed;
}

uint32_t m3_Get_نښان_Arg_شمېر (IM3_نښان i_نښان)
{
    أرجع 0;
}

M3_ارزښت_Type m3_Get_نښان_Arg_Type (IM3_نښان i_نښان, uint32_t i_شاخص)
{
    أرجع c_m3Type_none;
}

#د_که_پای // d_m3HasExceptionHandling || d_m3HasStackSwitching


#إذا d_m3HasExceptionHandling

M3_پایله m3_ThrowException (IM3Runtime io_runtime, IM3_نښان i_نښان, uint32_t i_argc, ثابت فراغ* i_argptrs[])
{
    إذا (not io_runtime or not i_نښان) {
        أرجع m3_Err_نښان_Lookup_Failed;
    }

    IM3_نښان      نښان  = i_نښان->resolved ? i_نښان->resolved : i_نښان;
    IM3FuncType type = نښان->type;

    // a tag with results is a stack switching control tag, which is suspended
    // with rather than thrown
    إذا (type->عدد_Rets) {
        أرجع m3Err_typeMismatch;
    }
    إذا (i_argc != type->عدد_دلیلونه) {
        أرجع m3_Err_دلیل_شمېر_Mismatch;
    }
    // with nothing on the stack there is nothing to unwind, and the exception
    // would sit on the runtime's list until some later call ended
    إذا (io_runtime->callNesting == 0) {
        أرجع "m3_ThrowException called outside a host function";
    }

    M3Exception* exception = NewException(io_runtime, نښان, i_argc);
    إذا (not exception) {
        أرجع m3Err_mallocFailed;
    }

    // the layout op_Throw gives a payload: 32-bit values zero-extended, floats
    // as their bit pattern
    لكل (u32 i = 0; i < i_argc; ++i) {
        ثابت فراغ* ارزښت = i_argptrs[i];

        بدل (بنسټ_Type_د(d_FuncArgType(type, i))) {
        حالة c_m3Type_i32:
        حالة c_m3Type_f32: {
            u32 بټونه;
            حافظه_نقل(&بټونه, ارزښت, حجم(بټونه));
            exception->دلیلونه[i] = بټونه;
            اكسر;
        }
        حالة c_m3Type_i64:
        حالة c_m3Type_f64:
            حافظه_نقل(&exception->دلیلونه[i], ارزښت, حجم(u64));
            اكسر;
        حالة c_m3Type_funcref:
        حالة c_m3Type_externref:
        حالة c_m3Type_exnref:
        حالة c_m3Type_contref:
            exception->دلیلونه[i] = (u64)(*(ثابت uintptr_t*)ارزښت);
            اكسر;
        افتراضي:
            FreeException(io_runtime, exception);
            أرجع "unknown argument type";
        }
    }

    io_runtime->pendingException = exception;

    أرجع m3Err_pendingException;
}


IM3_نښان m3_Get_Exception_نښان (IM3Runtime i_runtime)
{
    إذا (i_runtime and i_runtime->uncaughtException) {
        أرجع i_runtime->uncaughtException->نښان;
    }

    أرجع عدم;
}


M3_پایله m3_Get_Exception_دلیلونه (IM3Runtime i_runtime, uint32_t i_argc, ثابت فراغ* o_argptrs[])
{
    إذا (not i_runtime or not i_runtime->uncaughtException) {
        أرجع "no uncaught exception";
    }

    ثابت M3Exception* exception = i_runtime->uncaughtException;
    IM3FuncType        type      = exception->نښان->type;

    إذا (i_argc != exception->عدد_دلیلونه) {
        أرجع m3_Err_دلیل_شمېر_Mismatch;
    }

    لكل (u32 i = 0; i < i_argc; ++i) {
        فراغ* ارزښت = (فراغ*)o_argptrs[i];
        u64   بټونه  = exception->دلیلونه[i];

        بدل (بنسټ_Type_د(d_FuncArgType(type, i))) {
        حالة c_m3Type_i32:
        حالة c_m3Type_f32: {
            u32 ټیټ = (u32)بټونه;
            حافظه_نقل(ارزښت, &ټیټ, حجم(ټیټ));
            اكسر;
        }
        حالة c_m3Type_i64:
        حالة c_m3Type_f64:
            حافظه_نقل(ارزښت, &بټونه, حجم(بټونه));
            اكسر;
        حالة c_m3Type_funcref:
        حالة c_m3Type_externref:
        حالة c_m3Type_exnref:
        حالة c_m3Type_contref:
            *(uintptr_t*)ارزښت = (uintptr_t)بټونه;
            اكسر;
        افتراضي:
            أرجع "unknown argument type";
        }
    }

    أرجع m3Err_none;
}

#وإلا

M3_پایله m3_ThrowException (IM3Runtime io_runtime, IM3_نښان i_نښان, uint32_t i_argc, ثابت فراغ* i_argptrs[])
{
    أرجع m3_Err_نښان_Lookup_Failed;
}

IM3_نښان m3_Get_Exception_نښان (IM3Runtime i_runtime)
{
    أرجع عدم;
}

M3_پایله m3_Get_Exception_دلیلونه (IM3Runtime i_runtime, uint32_t i_argc, ثابت فراغ* o_argptrs[])
{
    أرجع "no uncaught exception";
}

#د_که_پای // d_m3HasExceptionHandling


#إذا d_m3_Guarded_حافظه

// What RunCodeProtected hands to the platform layer to run, and what comes back
تعريفنوع بنية M3_Protected_پرلپسې {
    pc_t            pc;
    IM3Runtime      runtime;
    M3_حافظه_سرلیک* mem;
    M3_پایله        پایله;
} M3_Protected_پرلپسې;

ساكن
فراغ پرلپسې_کوډ_Body (فراغ* io_context)
{
    M3_Protected_پرلپسې* پرلپسې = (M3_Protected_پرلپسې*)io_context;

#  إذا (d_m3EnableOpProfiling || d_m3EnableOpTracing)
    پرلپسې->پایله = (M3_پایله)پرلپسې_کوډ(پرلپسې->pc, (m3stack_t)پرلپسې->runtime->ډېری, پرلپسې->mem,
                                    d_m3_Op_Default_دلیلونه, d_m3_بنسټ_Cstr);
#  وإلا
    پرلپسې->پایله = (M3_پایله)پرلپسې_کوډ(پرلپسې->pc, (m3stack_t)پرلپسې->runtime->ډېری, پرلپسې->mem,
                                    d_m3_Op_Default_دلیلونه);
#  endif
}

// The bounds check the load and store operations no longer do, done once here by the
// system instead: an address past the end of a memory reaches the uncommitted part
// of that memory's reservation, and the fault comes back as the trap the spec asks
// for. Nothing is left half-done by it - the Wasm stack and the memories are heap,
// and the caller unwinds the rest.
ساكن
M3_پایله پرلپسې_کوډ_Protected (IM3Runtime i_runtime, pc_t i_pc, M3_حافظه_سرلیک* i_mem)
{
    // Memory operations in this build carry no bounds check, so code only runs where a
    // fault past the end of a memory can still be turned into a trap
    إذا (M3_UNLIKELY(not m3_Host_Guards_فعال())) {
        أرجع ErrorRuntime(m3_Err_حافظه_Guards_Unavailable, i_runtime,
                            "another part of the process holds the fault signal");
    }

    M3_Protected_پرلپسې پرلپسې;
    پرلپسې.pc      = i_pc;
    پرلپسې.runtime = i_runtime;
    پرلپسې.mem     = i_mem;
    پرلپسې.پایله  = m3Err_none;

    فراغ*  arena;
    size_t arena_بایټونه;
    Guard_ArenaRange(&arena, &arena_بایټونه);

    إذا (not m3_HostProtectedCall(پرلپسې_کوډ_Body, &پرلپسې, arena, arena_بایټونه)) {
        أرجع m3_Err_trap_بهر_د_Bounds_حافظه_Access;
    }

    أرجع پرلپسې.پایله;
}

#د_که_پای // d_m3GuardedMemory


#إذا d_m3_Has_ډېری_Switching

// The function a suspended continuation stopped in: the callee of its innermost
// call frame, or the function it was entered with if it made no call. Frames are
// recorded innermost first, so that is the lowest-numbered call frame.
ساكن
IM3Function Continuation_InnermostFunction (IM3Continuation i_cont)
{
    لكل (u32 i = 0; i < i_cont->عدد_چوکاټونه; ++i) {
        إذا (i_cont->چوکاټونه[i].ډول == چوکاټ_call && i_cont->چوکاټونه[i].call.function) {
            أرجع i_cont->چوکاټونه[i].call.function;
        }
    }

    أرجع i_cont->ثبت_Function;
}


// Where the host's next call puts its arguments and finds its results. A
// suspended root keeps its frames on the bottom of the runtime's stack, so a call
// made before it is resumed starts above the last of them rather than on top of
// them. The top is the innermost function's frame base plus everything that
// function can use. That base is the root's own sp, unless it stopped inside a
// resume: then the suspension point belongs to the resumed continuation, and the
// root's innermost frame is that resume.
فراغ Runtime_ځای_پرځای_کړه_Call_ډېری (IM3Runtime io_runtime)
{
    IM3Continuation ریښه = io_runtime->ریښه_Continuation;

    io_runtime->ډېری = io_runtime->origin_ډېری;

    إذا (not ریښه or ریښه->state != cont_suspended) {
        أرجع;
    }

    m3stack_t   sp       = ریښه->sp;
    IM3Function function = Continuation_InnermostFunction(ریښه);

    إذا (ریښه->عدد_چوکاټونه and ریښه->چوکاټونه[0].ډول == چوکاټ_resume) {
        sp = ریښه->چوکاټونه[0].sp;
    }

    m3slot_t* بنسټ = (m3slot_t*)io_runtime->origin_ډېری;
    m3slot_t* سر  = (m3slot_t*)sp + (function ? function->max_ډېری_Slots : 0);

    إذا (سر > بنسټ + io_runtime->عدد_ډېری_Slots) {
        سر = بنسټ + io_runtime->عدد_ډېری_Slots;
    }

    io_runtime->ډېری = سر;
}

#د_که_پای // d_m3HasStackSwitching


// Run compiled code on the runtime's stack, bounding native recursion for the
// duration of the call. The outermost invocation establishes the stack limit;
// nested ones (an imported function calling back into Wasm) inherit it.
ساكن مضمن
M3_پایله پرلپسې_کوډ_Checked (IM3Runtime i_runtime, IM3Function i_function)
{
    pc_t i_pc = i_function->compiled;

    // execution runs against the memory of the module the entry point belongs
    // to, not against some runtime-wide one
    M3_حافظه_سرلیک* _mem = Module_حافظه_سرلیک(i_function->module);

#إذا d_m3HasSnapshots
    إذا (M3_UNLIKELY(i_function->module->isUnusable)) {
        أرجع "a snapshot failed to restore into the module, so it cannot run";
    }
    i_function->module->has_پرلپسې = true;
#د_که_پای

    d_m3_ډېری_حد_Enter(i_runtime);

    // a host function calling back into Wasm does not change which call a trap
    // ends: that is still the one the host made first
    إذا (i_runtime->callNesting++ == 0) {
        i_runtime->entered = i_function;
    }
#إذا d_m3HasExceptionHandling
    // handler stacks don't nest across a call boundary: a host function calling
    // back into Wasm cannot be caught by a try_table its own caller entered
    u32 saved_Try_ژوروالی   = i_runtime->try_ژوروالی;
    i_runtime->try_ژوروالی = 0;
#د_که_پای
#إذا d_m3_Has_ډېری_Switching
    // nor do continuations: the native frames on the far side of a host call
    // are not ours to record, so a suspend in here has no handler to reach and
    // no resume to unwind to
    IM3Continuation savedContinuation = i_runtime->فعال_Continuation;

    // A suspended root continuation is a paused program waiting to be resumed,
    // not scratch space. A call the host makes before resuming it gets a
    // context of its own, the way any other re-entry does, rather than having
    // the frames of the suspension reset out from under it.
    bool claim_ریښه = i_runtime->isSuspendable and not savedContinuation and
                     not (i_runtime->ریښه_Continuation and
                          i_runtime->ریښه_Continuation->state == cont_suspended);

    إذا (claim_ریښه and not i_runtime->ریښه_Continuation) {
        i_runtime->ریښه_Continuation = Continuation_New(i_runtime, i_function->funcType, i_function);

        إذا (i_runtime->ریښه_Continuation) {
            // The root borrows the runtime's stack and is not charged as cont.new.
            i_runtime->ریښه_Continuation->عدد_ډېری_Slots = i_runtime->عدد_ډېری_Slots;
        } وإلا {
            claim_ریښه = false;
        }
    }

    إذا (claim_ریښه) {
        i_runtime->فعال_Continuation              = i_runtime->ریښه_Continuation;
        i_runtime->ریښه_Continuation->ثبت_Function = i_function;
        i_runtime->ریښه_Continuation->state         = cont_running;
        i_runtime->ریښه_Continuation->عدد_چوکاټونه     = 0;
    } وإلا {
        i_runtime->فعال_Continuation = عدم;
    }
#د_که_پای
#إذا d_m3_Guarded_حافظه
    M3_پایله پایله = پرلپسې_کوډ_Protected(i_runtime, i_pc, _mem);
#وإلاإذا (d_m3EnableOpProfiling || d_m3EnableOpTracing)
    M3_پایله پایله = (M3_پایله)پرلپسې_کوډ(i_pc, (m3stack_t)i_runtime->ډېری, _mem, d_m3_Op_Default_دلیلونه, d_m3_بنسټ_Cstr);
#وإلا
    M3_پایله پایله = (M3_پایله)پرلپسې_کوډ(i_pc, (m3stack_t)i_runtime->ډېری, _mem, d_m3_Op_Default_دلیلونه);
#د_که_پای
#إذا d_m3_Has_ډېری_Switching
    إذا (claim_ریښه) {
        i_runtime->ریښه_Continuation->state =
          (پایله == m3Err_continuationSuspended) ? cont_suspended : cont_returned;

        Runtime_ځای_پرځای_کړه_Call_ډېری(i_runtime);
    }
    i_runtime->فعال_Continuation = savedContinuation;
#د_که_پای
#إذا d_m3HasExceptionHandling
    i_runtime->try_ژوروالی = saved_Try_ژوروالی;

    // an exception that reached the bottom of the call stack found no handler
    إذا (M3_UNLIKELY(پایله == m3Err_pendingException)) {
        پایله = m3Err_trapUncaughtException;
    }
    ریکارډ_Uncaught_Exception(i_runtime, پایله);

    // a paused invocation can still reach the exceptions it holds, so they wait
    // for it to finish
    إذا (i_runtime->callNesting == 1 and not m3_IsSuspended(i_runtime)) {
        FreeExceptions(i_runtime);
    }
#د_که_پای
    i_runtime->callNesting--;
    d_m3_ډېری_حد_Leave(i_runtime);

    أرجع پایله;
}

M3_پایله m3_پرلپسې_پیل (IM3Module io_module)
{
#که_تعریف_شوی FUZZING_BUILD_ډېرپېښ_UNSAFE_FOR_PRODUCTION
    // Execution disabled for fuzzing builds
    أرجع m3Err_none;
#د_که_پای

    M3_پایله پایله           = m3Err_none;
    i32      پیل_Function_Tmp = -1;

    إذا (io_module and io_module->پیل_Function >= 0) {
        IM3Function function = &io_module->functions[io_module->پیل_Function];

        إذا (not function->compiled) {
_           (CompileFunction(function));
        }

        IM3FuncType ftype = function->funcType;
        إذا (ftype->عدد_دلیلونه != 0 || ftype->عدد_Rets != 0) {
            _throw(m3_Err_دلیل_شمېر_Mismatch);
        }

        IM3Module  module  = function->module;
        IM3Runtime runtime = module->runtime;

        پیل_Function_Tmp = io_module->پیل_Function;

        io_module->پیل_Function = -1;

        پایله = پرلپسې_کوډ_Checked(runtime, function);

        // a start function that suspended is not to be started again: resuming
        // is what finishes it
        إذا (پایله and پایله != m3Err_continuationSuspended) {
            io_module->پیل_Function = پیل_Function_Tmp;
            EXCEPTION_PRINT(پایله);
            اذهب _catch;
        }
    }

    _catch: أرجع پایله;
}

// TODO: deal with main + side-modules loading efforcement
M3_پایله m3_بارول_Module (IM3Runtime io_runtime, IM3Module io_module)
{
    M3_پایله پایله = m3Err_none;

    إذا (M3_UNLIKELY(io_module->runtime)) {
        أرجع m3Err_moduleAlreadyLinked;
    }

    io_module->runtime = io_runtime;

    // linking first: a memory import has to be resolved before InitMemory would
    // give it pages of its own, and a global import before an initializer reads it
_   (LinkImports(io_runtime, io_module));

_   (Init_حافظه(io_runtime, io_module));
_   (InitGlobals(io_module));
    // Spec order: element segments are applied before data segments. It matters
    // when one of them traps - whatever ran before the trap stays done.
_   (Init_جدول_And_Elements(io_module));
_   (Init_معلومات_Segments(io_module));

    // Start func might use imported functions, which are not liked here yet,
    // so it will be called before a function call is attempted (in m3_FindFunction)

#که_تعریف_شوی DEBUG
    Module_Generate_نومونه(io_module);
#د_که_پای

    io_module->بل     = io_runtime->modules;
    io_runtime->modules = io_module;
    أرجع پایله; // ok

_catch:
    // The runtime owns the module either way. Instantiation may already have
    // written this module's functions into a table another module owns, and the
    // spec keeps whatever it managed to do before the trap, so those entries
    // stay callable - retaining the module is what stops them dangling.
    //
    // It goes on the tail of the list rather than the head: it is not a module
    // anyone should find by name, only one whose functions may still be
    // reachable through someone else's table.
    io_module->بل = عدم;

    IM3Module* لکۍ = &io_runtime->modules;
    طالما (*لکۍ) {
        لکۍ = &(*لکۍ)->بل;
    }
    *لکۍ = io_module;

    أرجع پایله;
}

IM3Global m3_پیدا_کړه_Global (IM3Module         io_module,
                         ثابت حرف* ثابت i_globalName)
{
    // Search exports
    IM3Global exported = Module_پیدا_کړه_Exported_Global(io_module, i_globalName);
    إذا (exported) {
        // a re-exported global is the one that was imported, so reads and
        // writes have to reach the cell that actually holds the value
        أرجع exported->resolved ? exported->resolved : exported;
    }

    // Search imports
    لكل (u32 i = 0; i < io_module->عدد_Globals; ++i) {
        IM3Global g = &io_module->globals[i];

        إذا (g->import.moduleUtf8 and g->import.ډګر_Utf8) {
            إذا (strcmp(g->import.ډګر_Utf8, i_globalName) == 0) {
                أرجع g->resolved ? g->resolved : g;
            }
        }
    }
    أرجع عدم;
}

M3_پایله m3_GetGlobal (IM3Global      i_global,
                       IM3_Tagged_ارزښت o_ارزښت)
{
    إذا (not i_global) {
        أرجع m3Err_globalLookupFailed;
    }

    بدل (i_global->type) {
    حالة c_m3Type_i32: o_ارزښت->ارزښت.i32 = i_global->i32_ارزښت; اكسر;
    حالة c_m3Type_i64: o_ارزښت->ارزښت.i64 = i_global->i64_ارزښت; اكسر;
#إذا d_m3HasFloat
    حالة c_m3Type_f32: o_ارزښت->ارزښت.f32 = i_global->f32_ارزښت; اكسر;
    حالة c_m3Type_f64: o_ارزښت->ارزښت.f64 = i_global->f64_ارزښت; اكسر;
#د_که_پای
    افتراضي: أرجع m3Err_invalidTypeId;
    }

    o_ارزښت->type = (M3_ارزښت_Type)(i_global->type);
    أرجع m3Err_none;
}

M3_پایله m3_سېټ_Global (IM3Global            i_global,
                       ثابت IM3_Tagged_ارزښت i_ارزښت)
{
    إذا (not i_global) {
        أرجع m3Err_globalLookupFailed;
    }
    إذا (not i_global->isMutable) {
        أرجع m3Err_globalNotMutable;
    }
    إذا (i_global->type != i_ارزښت->type) {
        أرجع m3Err_globalTypeMismatch;
    }

    بدل (i_ارزښت->type) {
    حالة c_m3Type_i32: i_global->i32_ارزښت = i_ارزښت->ارزښت.i32; اكسر;
    حالة c_m3Type_i64: i_global->i64_ارزښت = i_ارزښت->ارزښت.i64; اكسر;
#إذا d_m3HasFloat
    حالة c_m3Type_f32: i_global->f32_ارزښت = i_ارزښت->ارزښت.f32; اكسر;
    حالة c_m3Type_f64: i_global->f64_ارزښت = i_ارزښت->ارزښت.f64; اكسر;
#د_که_پای
    افتراضي: أرجع m3Err_invalidTypeId;
    }

    أرجع m3Err_none;
}

M3_ارزښت_Type m3_GetGlobalType (IM3Global i_global)
{
    أرجع (i_global) ? (M3_ارزښت_Type)(i_global->type) : c_m3Type_none;
}


فراغ* v_پیدا_کړه_Function (IM3Module i_module, فراغ* i_info)
{
    ثابت حرف* ثابت i_name = (ثابت حرف*)i_info;

    // Prefer exported functions
    IM3Function exported = Module_پیدا_کړه_Exported_Function(i_module, i_name);
    إذا (exported) {
        أرجع exported;
    }

    // Search internal functions
    لكل (u32 i = 0; i < i_module->عدد_Functions; ++i) {
        IM3Function f = &i_module->functions[i];

        bool isImported = f->import.moduleUtf8 or f->import.ډګر_Utf8;

        إذا (isImported) {
            استمر;
        }

        لكل (صحيح j = 0; j < f->عدد_نومونه; j++) {
            إذا (f->نومونه[j] and strcmp(f->نومونه[j], i_name) == 0) {
                أرجع f;
            }
        }
    }

    أرجع عدم;
}


// Shared tail of the two lookups: a function is only usable once it has code.
ساكن
M3_پایله Prepare_وموندل_Function (IM3Function* o_function, IM3Function i_function)
{
    M3_پایله پایله = m3Err_none;

    إذا (not i_function->compiled) {
_       (CompileFunction(i_function))
    }

    _catch:
    *o_function = پایله ? عدم : i_function;

    أرجع پایله;
}


// Searches every module in the runtime, most recently loaded first. That is a
// guess once more than one module is loaded and two of them export the same
// name - m3_FindFunctionIn says which module is meant.
M3_پایله m3_پیدا_کړه_Function (IM3Function* o_function, IM3Runtime i_runtime, ثابت حرف* ثابت i_functionName)
{
                                                                d_m3Assert (o_function and i_runtime and i_functionName);
    IM3Function function = عدم;

    إذا (not i_runtime->modules) {
        *o_function = عدم;
        أرجع "no modules loaded";
    }

    function = (IM3Function)ForEachModule(i_runtime, v_پیدا_کړه_Function, (فراغ*)i_functionName);

    إذا (not function) {
        *o_function = عدم;
        أرجع ErrorModule(m3Err_functionLookupFailed, i_runtime->modules, "'%s'", i_functionName);
    }

    أرجع Prepare_وموندل_Function(o_function, function);
}


IM3Module m3_پیدا_کړه_Module (IM3Runtime i_runtime, ثابت حرف* ثابت i_moduleName)
{
    إذا (not i_runtime or not i_moduleName) {
        أرجع عدم;
    }

    // the list is newest-first, so a name registered twice names the newer one
    لكل (IM3Module m = i_runtime->modules; m; m = m->بل) {
        إذا (m->name and strcmp(m->name, i_moduleName) == 0) {
            أرجع m;
        }
    }

    أرجع عدم;
}


// Searches one module's exports, which is what naming a module means.
M3_پایله m3_پیدا_کړه_Function_دننه (IM3Function* o_function, IM3Module i_module, ثابت حرف* ثابت i_functionName)
{
                                                                d_m3Assert (o_function and i_functionName);
    إذا (not i_module) {
        *o_function = عدم;
        أرجع m3Err_functionLookupFailed;      // ErrorModule would deref it
    }

    IM3Function function = (IM3Function)v_پیدا_کړه_Function(i_module, (فراغ*)i_functionName);

    إذا (not function) {
        *o_function = عدم;
        أرجع ErrorModule(m3Err_functionLookupFailed, i_module, "'%s'", i_functionName);
    }

    أرجع Prepare_وموندل_Function(o_function, function);
}


M3_پایله m3_Get_جدول_Function (IM3Function* o_function, IM3Module i_module, uint32_t i_شاخص)
{
_try {
    M3_جدول*    جدول_2;
    IM3Function function;

    _throwif("no table", i_module->عدد_Tables == 0);

    جدول_2 = i_module->tables[0];
    _throwif("function index out of range", i_شاخص >= جدول_2->اندازه);

    function = (IM3Function)جدول_2->elements[i_شاخص];

    إذا (function) {
        إذا (not function->compiled) {
_           (CompileFunction(function))
        }
    }

    *o_function = function;
} _catch:
    أرجع پایله;
}


ساكن
M3_پایله check_پیل_Function (IM3Module i_module)
{
    M3_پایله پایله = m3Err_none;                               d_m3Assert(i_module);

    // Check if start function needs to be called
    إذا (i_module->پیل_Function >= 0) {
        پایله = m3_پرلپسې_پیل(i_module);
    }

    أرجع پایله;
}

uint32_t m3_Get_Arg_شمېر (IM3Function i_function)
{
    إذا (i_function) {
        IM3FuncType ft = i_function->funcType;
        إذا (ft) {
            أرجع ft->عدد_دلیلونه;
        }
    }
    أرجع 0;
}

uint32_t m3_Get_Ret_شمېر (IM3Function i_function)
{
    إذا (i_function) {
        IM3FuncType ft = i_function->funcType;
        إذا (ft) {
            أرجع ft->عدد_Rets;
        }
    }
    أرجع 0;
}


M3_ارزښت_Type m3_GetArgType (IM3Function i_function, uint32_t index)
{
    إذا (i_function) {
        IM3FuncType ft = i_function->funcType;
        إذا (ft and index < ft->عدد_دلیلونه) {
            أرجع (M3_ارزښت_Type)بنسټ_Type_د(d_FuncArgType(ft, index));
        }
    }
    أرجع c_m3Type_none;
}

M3_ارزښت_Type m3_GetRetType (IM3Function i_function, uint32_t index)
{
    إذا (i_function) {
        IM3FuncType ft = i_function->funcType;
        إذا (ft and index < ft->عدد_Rets) {
            أرجع (M3_ارزښت_Type)بنسټ_Type_د(d_FuncRetType(ft, index));
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
M3_پایله Get_ډېری_اشاره_ګر_For_دلیلونه (IM3Function i_function, u8** o_ډېری)
{
    IM3Runtime  runtime = i_function->module->runtime;
    IM3FuncType ftype   = i_function->funcType;

    // the stack a paused invocation holds is not available to this one
    size_t کارول_شوی      = (size_t)((u8*)runtime->ډېری - (u8*)runtime->origin_ډېری);
    size_t needed    = ((size_t)ftype->عدد_Rets + ftype->عدد_دلیلونه) * حجم(u64);
    size_t شته = (size_t)runtime->عدد_ډېری_Slots * حجم(m3slot_t) - کارول_شوی;

    إذا (needed > شته) {
        أرجع m3_Err_trap_ډېری_ډکېدنه;
    }

    u64* ډېری = (u64*)runtime->ډېری;

    ډېری += ftype->عدد_Rets;

    *o_ډېری = (u8*)ډېری;

    أرجع m3Err_none;
}


M3_پایله m3_CallV (IM3Function i_function, ...)
{
    va_list ap;
    va_start(ap, i_function);
    M3_پایله r = m3_CallVL(i_function, ap);
    va_end(ap);
    أرجع r;
}

ساكن
فراغ Report_Native_ډېری_Usage ()
{
#إذا d_m3_Log_Native_ډېری
    صحيح ډېری_کارول_شوی = m3_ډېری_Get_Max();
    fprintf(stderr, "Native stack used: %d\n", ډېری_کارول_شوی);
#د_که_پای
}


M3_پایله m3_CallVL (IM3Function i_function, va_list i_دلیلونه)
{
    IM3Runtime  runtime = i_function->module->runtime;
    IM3FuncType ftype   = i_function->funcType;
    M3_پایله    پایله  = m3Err_none;
    u8*         s       = عدم;

    إذا (!i_function->compiled) {
        أرجع m3_Err_missing_Compiled_کوډ;
    }

#إذا d_m3_ریکارډ_Backtraces
    ClearBacktrace(runtime);
#د_که_پای

    m3_ډېری_Check_Init();

_   (check_پیل_Function(i_function->module))

_   (Get_ډېری_اشاره_ګر_For_دلیلونه(i_function, &s))

    لكل (u32 i = 0; i < ftype->عدد_دلیلونه; ++i) {
        بدل (d_FuncArgType(ftype, i)) {
        حالة c_m3Type_i32:
            *(i32*)(s) = va_arg(i_دلیلونه, i32);
            s += 8;
            اكسر;
        حالة c_m3Type_i64:
            *(i64*)(s) = va_arg(i_دلیلونه, i64);
            s += 8;
            اكسر;
        حالة c_m3Type_funcref:
        حالة c_m3Type_externref:
        حالة c_m3Type_exnref:
            *(uintptr_t*)(s) = va_arg(i_دلیلونه, uintptr_t);
            s += 8;
            اكسر;
#إذا d_m3HasFloat
        حالة c_m3Type_f32:
            *(f32*)(s) = (f32)va_arg(i_دلیلونه, f64);
            s += 8;
            اكسر; // f32 is passed as f64
        حالة c_m3Type_f64:
            *(f64*)(s) = va_arg(i_دلیلونه, f64);
            s += 8;
            اكسر;
#د_که_پای
        افتراضي: أرجع "unknown argument type";
        }
    }

    پایله = پرلپسې_کوډ_Checked(runtime, i_function);
    Report_Native_ډېری_Usage();

    runtime->وروستی_Called = پایله ? عدم : i_function;

    _catch: أرجع پایله;
}

M3_پایله m3_Call (IM3Function i_function, uint32_t i_argc, ثابت فراغ* i_argptrs[])
{
    IM3Runtime  runtime = i_function->module->runtime;
    IM3FuncType ftype   = i_function->funcType;
    M3_پایله    پایله  = m3Err_none;
    u8*         s       = عدم;

    إذا (i_argc != ftype->عدد_دلیلونه) {
        أرجع m3_Err_دلیل_شمېر_Mismatch;
    }
    إذا (!i_function->compiled) {
        أرجع m3_Err_missing_Compiled_کوډ;
    }

#إذا d_m3_ریکارډ_Backtraces
    ClearBacktrace(runtime);
#د_که_پای

    m3_ډېری_Check_Init();

_   (check_پیل_Function(i_function->module))

_   (Get_ډېری_اشاره_ګر_For_دلیلونه(i_function, &s))

    لكل (u32 i = 0; i < ftype->عدد_دلیلونه; ++i) {
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
#د_که_پای
        افتراضي: أرجع "unknown argument type";
        }
    }

    پایله = پرلپسې_کوډ_Checked(runtime, i_function);

    Report_Native_ډېری_Usage();

    runtime->وروستی_Called = پایله ? عدم : i_function;

    _catch: أرجع پایله;
}

// Argument parsing for m3_CallArgv. Strict on purpose: the whole string has to
// be consumed, so "12abc" is rejected rather than read as 12, and an empty or
// unparsable argument is an error rather than the zero that strtoul with a
// NULL end pointer used to hand back. See wasm3/wasm3#367.
ساكن
M3_پایله ParseArgInteger (ccstr_t i_arg, u32 i_عدد_بټونه, u64* o_ارزښت)
{
    إذا (not i_arg or not *i_arg) {
        أرجع "empty argument";
    }

    // strtoull would skip leading space, but trailing space is rejected below;
    // accepting one and not the other would just be confusing
    إذا (که_تشه((بینښه حرف)*i_arg)) {
        أرجع "argument is not a number";
    }

    حرف* پای_ټکی = عدم;
    u64   ارزښت;

    errno = 0;

    // an argument may be spelled signed or unsigned: -1 and 4294967295 name the
    // same i32
    إذا (*i_arg == '-') {
        i64 signed_ارزښت = strtoll(i_arg, &پای_ټکی, 10);

        إذا (i_عدد_بټونه == 32 and (signed_ارزښت < INT32_MIN or signed_ارزښت > INT32_MAX)) {
            أرجع "argument out of range";
        }

        ارزښت = (u64)signed_ارزښت;
    } وإلا {
        ارزښت = strtoull(i_arg, &پای_ټکی, 10);

        إذا (i_عدد_بټونه == 32 and ارزښت > UINT32_MAX) {
            أرجع "argument out of range";
        }
    }

    إذا (errno == ERANGE) {
        أرجع "argument out of range";
    }

    إذا (پای_ټکی == i_arg or *پای_ټکی) {
        أرجع "argument is not a number";
    }

    *o_ارزښت = ارزښت;

    أرجع m3Err_none;
}


#إذا d_m3HasFloat
ساكن
M3_پایله ParseArgFloat (ccstr_t i_arg, f64* o_ارزښت)
{
    إذا (not i_arg or not *i_arg) {
        أرجع "empty argument";
    }

    إذا (که_تشه((بینښه حرف)*i_arg)) {
        أرجع "argument is not a number";
    }

    حرف* پای_ټکی = عدم;

    errno = 0;

    f64 ارزښت = strtod(i_arg, &پای_ټکی);

    إذا (پای_ټکی == i_arg or *پای_ټکی) {
        أرجع "argument is not a number";
    }

    // strtod reports underflow through ERANGE as well, and a denormal result is
    // perfectly usable, so only an overflow to infinity is out of range
    إذا (errno == ERANGE and (ارزښت > DBL_MAX or ارزښت < -DBL_MAX)) {
        أرجع "argument out of range";
    }

    *o_ارزښت = ارزښت;

    أرجع m3Err_none;
}
#د_که_پای


// A reference argument is either the null reference or a host handle written as
// an integer.
ساكن
M3_پایله ParseArgReference (ccstr_t i_arg, u64* o_ارزښت)
{
    إذا (i_arg and strcmp(i_arg, "null") == 0) {
        *o_ارزښت = 0;
        أرجع m3Err_none;
    }

    أرجع ParseArgInteger(i_arg, 64, o_ارزښت);
}


M3_پایله m3_CallArgv (IM3Function i_function, uint32_t i_argc, ثابت حرف* i_argv[])
{
    IM3FuncType ftype   = i_function->funcType;
    IM3Runtime  runtime = i_function->module->runtime;
    M3_پایله    پایله  = m3Err_none;
    u8*         s       = عدم;

    إذا (i_argc != ftype->عدد_دلیلونه) {
        أرجع m3_Err_دلیل_شمېر_Mismatch;
    }
    إذا (!i_function->compiled) {
        أرجع m3_Err_missing_Compiled_کوډ;
    }

#إذا d_m3_ریکارډ_Backtraces
    ClearBacktrace(runtime);
#د_که_پای

    m3_ډېری_Check_Init();

_   (check_پیل_Function(i_function->module))

_   (Get_ډېری_اشاره_ګر_For_دلیلونه(i_function, &s))

    لكل (u32 i = 0; i < ftype->عدد_دلیلونه; ++i) {
        u64 ارزښت = 0;
#إذا d_m3HasFloat
        f64 fvalue = 0;
#د_که_پای
        بدل (d_FuncArgType(ftype, i)) {
        حالة c_m3Type_i32:
_           (ParseArgInteger(i_argv[i], 32, &ارزښت)) * (i32*)(s) = (i32)ارزښت;
            s += 8;
            اكسر;
        حالة c_m3Type_i64:
_           (ParseArgInteger(i_argv[i], 64, &ارزښت)) * (i64*)(s) = (i64)ارزښت;
            s += 8;
            اكسر;
        حالة c_m3Type_funcref:
        حالة c_m3Type_externref:
        حالة c_m3Type_exnref:
_           (ParseArgReference(i_argv[i], &ارزښت)) * (uintptr_t*)(s) = (uintptr_t)ارزښت;
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
#د_که_پای
        افتراضي: _throw("unknown argument type");
        }
    }

    پایله = پرلپسې_کوډ_Checked(runtime, i_function);

    Report_Native_ډېری_Usage();

    runtime->وروستی_Called = پایله ? عدم : i_function;

    _catch: أرجع پایله;
}


//u8 * AlignStackPointerTo64Bits (const u8 * i_stack)
//{
//    uintptr_t ptr = (uintptr_t) i_stack;
//    return (u8 *) ((ptr + 7) & ~7);
//}


M3_پایله m3_GetResults (IM3Function i_function, uint32_t i_retc, ثابت فراغ* o_retptrs[])
{
    IM3FuncType ftype   = i_function->funcType;
    IM3Runtime  runtime = i_function->module->runtime;

    إذا (i_retc != ftype->عدد_Rets) {
        أرجع m3_Err_دلیل_شمېر_Mismatch;
    }
    إذا (i_function != runtime->وروستی_Called) {
        أرجع "function not called";
    }

    u8* s = (u8*)runtime->ډېری;

    لكل (u32 i = 0; i < ftype->عدد_Rets; ++i) {
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
#د_که_پای
        افتراضي: أرجع "unknown return type";
        }
    }
    أرجع m3Err_none;
}

M3_پایله m3_GetResultsV (IM3Function i_function, ...)
{
    va_list ap;
    va_start(ap, i_function);
    M3_پایله r = m3_GetResultsVL(i_function, ap);
    va_end(ap);
    أرجع r;
}

M3_پایله m3_GetResultsVL (IM3Function i_function, va_list o_rets)
{
    IM3Runtime  runtime = i_function->module->runtime;
    IM3FuncType ftype   = i_function->funcType;

    إذا (i_function != runtime->وروستی_Called) {
        أرجع "function not called";
    }

    u8* s = (u8*)runtime->ډېری;
    لكل (u32 i = 0; i < ftype->عدد_Rets; ++i) {
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
#د_که_پای
        افتراضي: أرجع "unknown argument type";
        }
    }
    أرجع m3Err_none;
}

فراغ Release_کوډ_پاڼه_No_Track (IM3Runtime i_runtime, IM3_کوډ_پاڼه i_کوډ_پاڼه)
{
    إذا (i_کوډ_پاڼه) {
        IM3_کوډ_پاڼه* list;

        bool پاڼه_ډک = (عدد_Free_کرښې(i_کوډ_پاڼه) < d_m3_کوډ_پاڼه_Free_کرښې_پوله);
        إذا (پاڼه_ډک) {
            list = &i_runtime->پاڼې_ډک;
        } وإلا {
            list = &i_runtime->پاڼې_Open;
        }

        ټېل_کوډ_پاڼه(list, i_کوډ_پاڼه);                         m3log (emit, "release page: %d to queue: '%s'", i_کوډ_پاڼه->info.sequence, پاڼه_ډک ? "full" : "open")
    }
}


IM3_کوډ_پاڼه Acquire_کوډ_پاڼه_With_ظرفیت (IM3Runtime i_runtime, u32 i_min_کرښه_شمېر)
{
    IM3_کوډ_پاڼه پاڼه = Remove_کوډ_پاڼه_د_ظرفیت(&i_runtime->پاڼې_Open, i_min_کرښه_شمېر);

    إذا (not پاڼه) {
        پاڼه = Environment_Acquire_کوډ_پاڼه(i_runtime->environment, i_min_کرښه_شمېر);

        إذا (not پاڼه) {
            پاڼه = New_کوډ_پاڼه(i_runtime, i_min_کرښه_شمېر);
        }

        إذا (پاڼه) {
            i_runtime->عدد_کوډ_پاڼې++;
        }
    }

    إذا (پاڼه) {                                                  m3log (emit, "acquire page: %d", پاڼه->info.sequence);
        i_runtime->عدد_فعال_کوډ_پاڼې++;
    }

    أرجع پاڼه;
}


IM3_کوډ_پاڼه Acquire_کوډ_پاڼه (IM3Runtime i_runtime)
{
    أرجع Acquire_کوډ_پاڼه_With_ظرفیت(i_runtime, d_m3_کوډ_پاڼه_Free_کرښې_پوله);
}


فراغ Release_کوډ_پاڼه (IM3Runtime i_runtime, IM3_کوډ_پاڼه i_کوډ_پاڼه)
{
    إذا (i_کوډ_پاڼه) {
        Release_کوډ_پاڼه_No_Track(i_runtime, i_کوډ_پاڼه);
        i_runtime->عدد_فعال_کوډ_پاڼې--;

#إذا defined(DEBUG)
        u32 عدد_Open = شمېر_کوډ_پاڼې(i_runtime->پاڼې_Open);
        u32 عدد_ډک = شمېر_کوډ_پاڼې(i_runtime->پاڼې_ډک);

        m3log(runtime, "runtime: %p; open-pages: %d; full-pages: %d; active: %d; total: %d", i_runtime, عدد_Open, عدد_ډک, i_runtime->عدد_فعال_کوډ_پاڼې, i_runtime->عدد_کوډ_پاڼې);

        d_m3Assert(عدد_Open + عدد_ډک + i_runtime->عدد_فعال_کوډ_پاڼې == i_runtime->عدد_کوډ_پاڼې);

#  إذا d_m3_Log_کوډ_پاڼې
        dump_کوډ_پاڼه(i_کوډ_پاڼه, /* startPC: */ عدم);
#  endif
#د_که_پای
    }
}


#إذا d_m3VerboseErrorMessages
M3_پایله m3Error (M3_پایله i_پایله, IM3Runtime i_runtime, IM3Module i_module, IM3Function i_function,
                  ثابت حرف* ثابت i_file, u32 i_کرښه_عدد, ثابت حرف* ثابت i_error_پیغام, ...)
{
    إذا (i_runtime) {
        i_runtime->error         = (M3ErrorInfo){ .پایله = i_پایله, .runtime = i_runtime, .module = i_module, .function = i_function, .file = i_file, .کرښه_2 = i_کرښه_عدد };
        i_runtime->error.پیغام = i_runtime->error_پیغام;

        va_list دلیلونه;
        va_start(دلیلونه, i_error_پیغام);
        vsnprintf(i_runtime->error_پیغام, حجم(i_runtime->error_پیغام), i_error_پیغام, دلیلونه);
        va_end(دلیلونه);
    }

    أرجع i_پایله;
}
#د_که_پای


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
        i_runtime->error.پیغام = "";
    }
}

uint8_t* m3_Get_حافظه (IM3Module i_module, size_t* o_حافظه_اندازه_دننه_بایټونه, uint32_t i_حافظه_شاخص)
{
    uint8_t* حافظه = عدم;
    size_t   اندازه   = 0;

    إذا (i_module and i_حافظه_شاخص < i_module->عدد_Memories) {
        IM3_حافظه mem = i_module->memories[i_حافظه_شاخص];

        إذا (mem->mallocated) {
            اندازه = mem->mallocated->اوږدتیا;

            إذا (اندازه) {
                حافظه = m3_Mem_معلومات(mem->mallocated);
            }
        }
    }

    إذا (o_حافظه_اندازه_دننه_بایټونه) {
        *o_حافظه_اندازه_دننه_بایټونه = اندازه;
    }

    أرجع حافظه;
}


size_t m3_Get_حافظه_اندازه (IM3Module i_module, uint32_t i_حافظه_شاخص)
{
    إذا (not i_module or i_حافظه_شاخص >= i_module->عدد_Memories) {
        أرجع 0;
    }

    IM3_حافظه mem = i_module->memories[i_حافظه_شاخص];

    أرجع mem->mallocated ? mem->mallocated->اوږدتیا : 0;
}


size_t m3_Get_حافظه_اندازه_At (ثابت فراغ* i_حافظه)
{
    إذا (not i_حافظه) {
        أرجع 0;
    }

    // the header sits immediately before the data it describes
    ثابت M3_حافظه_سرلیک* سرلیک = ((ثابت M3_حافظه_سرلیک*)i_حافظه) - 1;

    أرجع سرلیک->اوږدتیا;
}


M3_پایله m3_پیدا_کړه_Exported_حافظه (IM3Module i_module, cstr_t i_name, u32* o_حافظه_شاخص)
{
    إذا (not i_module or not i_name or not o_حافظه_شاخص) {
        أرجع m3_Err_unknown_حافظه;
    }

    ثابت M3Export* e = Module_پیدا_کړه_Export(i_module, d_external_ډول_حافظه, i_name);

    إذا (not e) {
        أرجع m3_Err_unknown_حافظه;
    }

    *o_حافظه_شاخص = e->index;
    أرجع m3Err_none;
}


bool Module_HasLinkedHostImport (IM3Module i_module, cstr_t i_importModule)
{
    لكل (u32 i = 0; i < i_module->عدد_Functions; ++i) {
        IM3Function f = &i_module->functions[i];

        // hostMemory is set when a host function is bound to the import, so it
        // is also the mark of one - see CompileRawFunction
        إذا (not f->host_حافظه or f->wasm or f->resolved) {
            استمر;
        }

        إذا (f->import.moduleUtf8 and strcmp(f->import.moduleUtf8, i_importModule) == 0) {
            أرجع true;
        }
    }

    أرجع false;
}


M3_پایله m3_Bind_Import_حافظه (IM3Module io_module, cstr_t i_importModule, u32 i_حافظه_شاخص)
{
    إذا (not io_module or not i_importModule) {
        أرجع m3Err_moduleNotLinked;
    }

    إذا (i_حافظه_شاخص >= io_module->عدد_Memories) {
        أرجع m3_Err_unknown_حافظه;
    }

    // the memory itself, not the index: linking may have pointed this slot at
    // another module's memory, and growing one reallocates behind it
    IM3_حافظه حافظه = io_module->memories[i_حافظه_شاخص];

    ثابت bool wildcardModule = (strcmp(i_importModule, "*") == 0);

    لكل (u32 i = 0; i < io_module->عدد_Functions; ++i) {
        IM3Function f = &io_module->functions[i];

        // only an import that a host function was bound to has a memory to
        // pin: one with a body of its own reads memory through the interpreter,
        // and one resolved to another module's export runs that module's code
        إذا (not f->host_حافظه or f->wasm or f->resolved) {
            استمر;
        }

        إذا (wildcardModule or (f->import.moduleUtf8 and strcmp(f->import.moduleUtf8, i_importModule) == 0)) {
            f->host_حافظه = حافظه;
        }
    }

    أرجع m3Err_none;
}


M3BacktraceInfo* m3_GetBacktrace (IM3Runtime i_runtime)
{
#إذا d_m3_ریکارډ_Backtraces
    أرجع &i_runtime->backtrace;
#وإلا
    أرجع عدم;
#د_که_پای
}


//-------------------------------------------------------------------------------------------------------------------------------
//  snapshots & suspendable execution
//-------------------------------------------------------------------------------------------------------------------------------

فراغ m3_سېټ_Suspendable (IM3Runtime io_runtime, bool i_suspendable)
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
#إذا d_m3_Has_ډېری_Switching
    إذا (i_runtime->ریښه_Continuation && i_runtime->ریښه_Continuation->state == cont_suspended) {
        أرجع true;
    }
#د_که_پای
    أرجع false;
}

M3_پایله m3_ResumeRuntime (IM3Runtime io_runtime)
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
    إذا (io_runtime->ریښه_Continuation and io_runtime->ریښه_Continuation->ثبت_Function and
        io_runtime->ریښه_Continuation->ثبت_Function->module->isUnusable) {
        أرجع "a snapshot failed to restore into the module, so it cannot run";
    }
#د_که_پای
#إذا d_m3_Has_ډېری_Switching
    إذا (!io_runtime->ریښه_Continuation || io_runtime->ریښه_Continuation->state != cont_suspended) {
        أرجع m3Err_none;
    }
    // what finishes returns into the bottom of the stack, where it was entered
    io_runtime->ډېری = io_runtime->origin_ډېری;

    // this is the outermost call again, as far as a host function calling back
    // into Wasm from inside it can tell
    io_runtime->entered = io_runtime->ریښه_Continuation->ثبت_Function;
    io_runtime->callNesting++;

    d_m3_ډېری_حد_Enter(io_runtime);
    io_runtime->resumePastCheck = true;
    M3_پایله پایله             = (M3_پایله)ResumeContinuation(io_runtime, io_runtime->ریښه_Continuation);
    io_runtime->resumePastCheck = false;
    d_m3_ډېری_حد_Leave(io_runtime);

#  إذا d_m3HasExceptionHandling
    إذا (M3_UNLIKELY(پایله == m3Err_pendingException)) {
        پایله = m3Err_trapUncaughtException;
    }
    ریکارډ_Uncaught_Exception(io_runtime, پایله);
#  endif

    io_runtime->callNesting--;

#  إذا d_m3HasExceptionHandling
    إذا (پایله != m3Err_continuationSuspended and io_runtime->callNesting == 0) {
        FreeExceptions(io_runtime);
    }
#  endif

    // the results are where the paused call left room for them, so a finished
    // resume can be asked for them the way the call itself could have been
    io_runtime->وروستی_Called = (پایله == m3Err_none) ? io_runtime->ریښه_Continuation->ثبت_Function : عدم;

    Runtime_ځای_پرځای_کړه_Call_ډېری(io_runtime);
    أرجع پایله;
#وإلا
    أرجع m3Err_none;
#د_که_پای
}
