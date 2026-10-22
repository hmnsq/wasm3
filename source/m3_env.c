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
            لكل (u8 t = c_m3Type_none; t < c_m3_Type_شمار; t++) {
                إذا (t == c_m3Type_v128) {
                    استمر;
                }

                IM3FuncType ftype;
_               (AllocFuncType(&ftype, 1));

                ftype->رقم_آرگومانها  = 0;
                ftype->رقم_Rets  = (t == c_m3Type_none) ? 0 : 1;
                ftype->types[0] = t;

                Environment_جمع_کن_Func_Type(env, &ftype);

                env->retFuncTypes[t] = ftype;
            }
        }

        _catch:
        إذا (نتیجه) {
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
        IM3FuncType بعدی = ftype->بعدی;
        m3_Free(ftype);
        ftype = بعدی;
    }

    m3log(runtime, "freeing %d pages from environment", شمار_کد_صفحات(i_environment->صفحات_Released));
    Free_کد_صفحات(&i_environment->صفحات_Released);
}


فراغ m3_FreeEnvironment (IM3Environment i_environment)
{
    إذا (i_environment) {
        Environment_Release(i_environment);
        m3_Free(i_environment);
    }
}


فراغ m3_مجموعه_Custom_Section_Handler (IM3Environment i_environment, M3SectionHandler i_handler)
{
    إذا (i_environment) {
        i_environment->customSectionHandler = i_handler;
    }
}


// returns the same io_funcType or replaces it with an equivalent that's already in the type linked list
M3_نتیجه Environment_جمع_کن_Func_Type (IM3Environment i_environment, IM3FuncType* io_funcType)
{
    IM3FuncType جمع_کن_Type = *io_funcType;
    IM3FuncType newType = i_environment->funcTypes;

    طالما (newType) {
        إذا (not newType->درون_Rec_گروه and AreFuncTypesEqual(newType, جمع_کن_Type)) {
            m3_Free(جمع_کن_Type);
            اكسر;
        }

        newType = newType->بعدی;
    }

    إذا (newType == عدم) {
        // a type index has to fit in the heap type field of an m3type_t
        إذا (i_environment->رقم_Func_Types >= d_m3_Max_Sane_Types_شمار) {
            m3_Free(جمع_کن_Type);
            *io_funcType = عدم;
            أرجع "too many distinct function types";
        }

        newType                  = جمع_کن_Type;
        newType->canonical_شاخص  = i_environment->رقم_Func_Types++;
        newType->بعدی            = i_environment->funcTypes;
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
M3_نتیجه Environment_ReserveFuncTypes (IM3Environment i_environment, u32 i_شمار, u16* o_اولی_شاخص)
{
    إذا ((u64)i_environment->رقم_Func_Types + i_شمار > d_m3_Max_Sane_Types_شمار) {
        أرجع "too many distinct function types";
    }

    *o_اولی_شاخص = i_environment->رقم_Func_Types;
    i_environment->رقم_Func_Types += (u16)i_شمار;

    أرجع m3Err_none;
}


فراغ Environment_AdoptFuncType (IM3Environment i_environment, IM3FuncType i_funcType, u16 i_canonical_شاخص)
{
    i_funcType->درون_Rec_گروه     = true;
    i_funcType->canonical_شاخص = i_canonical_شاخص;
    i_funcType->بعدی           = i_environment->funcTypes;
    i_environment->funcTypes   = i_funcType;
}


IM3_کد_صفحه Remove_کد_صفحه_از_ظرفیت (M3_کد_صفحه** io_list, u32 i_minimum_سطر_شمار)
{
    IM3_کد_صفحه prev = عدم;
    IM3_کد_صفحه صفحه = *io_list;

    طالما (صفحه) {
        إذا (رقم_Free_سطرها(صفحه) >= i_minimum_سطر_شمار) {             d_m3Assert (صفحه->info.usage_شمار == 0);
            IM3_کد_صفحه بعدی = صفحه->info.بعدی;
            إذا (prev) {
                prev->info.بعدی = بعدی; // mid-list
            } وإلا {
                *io_list = بعدی;       // front of list
            }

            اكسر;
        }

        prev = صفحه;
        صفحه = صفحه->info.بعدی;
    }

    أرجع صفحه;
}


IM3_کد_صفحه Environment_Acquire_کد_صفحه (IM3Environment i_environment, u32 i_minimum_سطر_شمار)
{
    أرجع Remove_کد_صفحه_از_ظرفیت(&i_environment->صفحات_Released, i_minimum_سطر_شمار);
}


فراغ Environment_Release_کد_صفحات (IM3Environment i_environment, IM3_کد_صفحه i_کد_صفحه_List)
{
    IM3_کد_صفحه انتها = i_کد_صفحه_List;

    طالما (انتها) {
        انتها->info.سطر_شاخص = 0; // reset page
#إذا d_m3_رکورد_Backtraces
        انتها->info.mapping->اندازه = 0;
#پایان_اگر // d_m3RecordBacktraces

        IM3_کد_صفحه بعدی = انتها->info.بعدی;
        إذا (not بعدی) {
            اكسر;
        }

        انتها = بعدی;
    }

    إذا (انتها) {
        // push list to front
        انتها->info.بعدی               = i_environment->صفحات_Released;
        i_environment->صفحات_Released = i_کد_صفحه_List;
    }
}


IM3Runtime m3_NewRuntime (IM3Environment i_environment, u32 i_پشته_اندازه_درون_بایتها, فراغ* i_userdata)
{
    IM3Runtime runtime = m3_AllocStruct(M3Runtime);

    إذا (runtime) {
        m3_ResetErrorInfo(runtime);

        runtime->environment = i_environment;
        runtime->userdata    = i_userdata;

        Deterministic_InitRuntime(runtime);

        runtime->origin_پشته = m3_Malloc("Wasm Stack", i_پشته_اندازه_درون_بایتها + 4 * حجم(m3slot_t)); // TODO: more precise stack checks

        إذا (runtime->origin_پشته) {
            runtime->پشته         = runtime->origin_پشته;
            runtime->رقم_پشته_Slots = i_پشته_اندازه_درون_بایتها / حجم(m3slot_t);          m3log (runtime, "new stack: %p, slots: %u", runtime->origin_پشته, runtime->رقم_پشته_Slots);
        } وإلا {
            m3_Free(runtime);
        }
    }

    أرجع runtime;
}

فراغ m3_مجموعه_Validation (IM3Runtime i_runtime, bool i_enable)
{
#إذا d_m3EnableValidation
    إذا (i_runtime) {
        i_runtime->skipValidation = not i_enable;
    }
#وإلا
    (فراغ)i_runtime;
    (فراغ)i_enable;              // nothing to skip: the validator was compiled out
#پایان_اگر
}

M3_نتیجه m3_مجموعه_Resource_حد (IM3Runtime i_runtime, M3_Resource_حد i_حد, uint64_t i_مقدار)
{
    إذا (not i_runtime) {
        أرجع m3Err_mallocFailed;
    }
    بدل (i_حد) {
    حالة c_m3_حد_حافظه_بایتها:
        إذا (i_مقدار and i_مقدار < i_runtime->حافظه_بایتها_استفاده_شده) {
            أرجع m3_Err_resource_حد_زیر_Usage;
        }
        i_runtime->حافظه_بایتها_حد = i_مقدار;
        أرجع m3Err_none;
    حالة c_m3_حد_جدول_Elements:
        إذا (i_مقدار and i_مقدار < i_runtime->جدول_Elements_استفاده_شده) {
            أرجع m3_Err_resource_حد_زیر_Usage;
        }
        i_runtime->جدول_Elements_حد = i_مقدار;
        أرجع m3Err_none;
#إذا d_m3_Has_پشته_Switching
    حالة c_m3_حد_Continuations:
        إذا (i_مقدار and i_مقدار < i_runtime->continuationsAllocated) {
            أرجع m3_Err_resource_حد_زیر_Usage;
        }
        i_runtime->continuations_حد = (u32)M3_MIN(i_مقدار, (u64)UINT32_MAX);
        طالما (i_مقدار and i_runtime->val_پشته_Pool_شمار > i_مقدار - i_runtime->continuationsAllocated) {
            m3_Free(i_runtime->val_پشته_Pool[--i_runtime->val_پشته_Pool_شمار]);
        }
        أرجع m3Err_none;
#وإلا
    حالة c_m3_حد_Continuations: أرجع m3_Err_resource_حد_Not_Supported;
#پایان_اگر
    حالة c_m3_حد_Gas_Units:
#إذا d_m3HasGasMetering
        i_runtime->gas_حد = i_runtime->gas_مانده = (i64)M3_MIN(i_مقدار, (u64)INT64_MAX);
        أرجع m3Err_none;
#وإلا
        أرجع m3_Err_resource_حد_Not_Supported;
#پایان_اگر
    افتراضي: أرجع m3_Err_unknown_Resource_حد;
    }
}

uint64_t m3_Get_Resource_حد (IM3Runtime i_runtime, M3_Resource_حد i_حد)
{
    إذا (not i_runtime) {
        أرجع 0;
    }
    بدل (i_حد) {
    حالة c_m3_حد_حافظه_بایتها: أرجع i_runtime->حافظه_بایتها_حد;
    حالة c_m3_حد_جدول_Elements: أرجع i_runtime->جدول_Elements_حد;
#إذا d_m3_Has_پشته_Switching
    حالة c_m3_حد_Continuations: أرجع i_runtime->continuations_حد;
#پایان_اگر
#إذا d_m3HasGasMetering
    حالة c_m3_حد_Gas_Units: أرجع (u64)i_runtime->gas_حد;
#پایان_اگر
    افتراضي: أرجع 0;
    }
}

uint64_t m3_GetResourceUsage (IM3Runtime i_runtime, M3_Resource_حد i_حد)
{
    إذا (not i_runtime) {
        أرجع 0;
    }
    بدل (i_حد) {
    حالة c_m3_حد_حافظه_بایتها: أرجع i_runtime->حافظه_بایتها_استفاده_شده;
    حالة c_m3_حد_جدول_Elements: أرجع i_runtime->جدول_Elements_استفاده_شده;
#إذا d_m3_Has_پشته_Switching
    حالة c_m3_حد_Continuations: أرجع i_runtime->continuationsAllocated;
#پایان_اگر
#إذا d_m3HasGasMetering
    حالة c_m3_حد_Gas_Units: أرجع (u64)i_runtime->gas_حد - (u64)i_runtime->gas_مانده;
#پایان_اگر
    افتراضي: أرجع 0;
    }
}

فراغ* m3_Get_User_داده (IM3Runtime i_runtime)
{
    أرجع i_runtime ? i_runtime->userdata : عدم;
}


فراغ* ForEachModule (IM3Runtime i_runtime, ModuleVisitor i_visitor, فراغ* i_info)
{
    فراغ* r = عدم;

    IM3Module module = i_runtime->modules;

    طالما (module) {
        IM3Module بعدی = module->بعدی;
        إذا ((r = i_visitor(module, i_info))) {
            اكسر;
        }

        module = بعدی;
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
    لكل (u32 i = 0; i < i_module->رقم_Memories; ++i) {
        IM3_حافظه حافظه = i_module->memories[i];

        إذا (حافظه and حافظه->owner != i_module) {
            i_module->memories[i] = عدم;
        }
    }

    لكل (u32 i = 0; i < i_module->رقم_Tables; ++i) {
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
    ForEachModule(i_runtime, _FreeModule, عدم);                    d_m3Assert (i_runtime->رقم_فعال_کد_صفحات == 0);

    Environment_Release_کد_صفحات(i_runtime->environment, i_runtime->صفحات_Open);
    Environment_Release_کد_صفحات(i_runtime->environment, i_runtime->صفحات_پر);

    m3_Free(i_runtime->origin_پشته);

#إذا d_m3EnableValidation
    m3_Free(i_runtime->validator);
#پایان_اگر

#إذا d_m3HasExceptionHandling
    // a runtime let go of while suspended still holds its exceptions
    FreeExceptions(i_runtime);
    m3_Free(i_runtime->uncaughtException);
#پایان_اگر
#إذا d_m3HasExceptionHandling || d_m3_Has_پشته_Switching
    طالما (i_runtime->host_برچسبها) {
        M3_Host_برچسب* بعدی = i_runtime->host_برچسبها->بعدی;
        m3_Free(i_runtime->host_برچسبها);
        i_runtime->host_برچسبها = بعدی;
    }
#پایان_اگر
#إذا d_m3_Has_پشته_Switching
    Continuation_ReleaseAll(i_runtime);
#پایان_اگر
}


فراغ m3_FreeRuntime (IM3Runtime i_runtime)
{
    إذا (i_runtime) {
        m3_PrintProfilerInfo();

        Runtime_Release(i_runtime);
        m3_Free(i_runtime);
    }
}


#إذا d_m3_Has_پشته_Switching

IM3Continuation Continuation_New (IM3Runtime i_runtime, IM3FuncType i_type, IM3Function i_function)
{
    IM3Continuation cont = m3_AllocStruct(M3Continuation);
    إذا (!cont) {
        أرجع عدم;
    }

    cont->type          = i_type;
    cont->مدخل_Function = i_function;
    cont->state         = cont_allocated;

    cont->رقم_پشته_Slots = d_m3_Continuation_پشته_Slots;
    // frames are recorded only when this continuation actually suspends, and
    // only as deep as it got, so the array starts out unallocated
    cont->قابها    = عدم;
    cont->قابها_Cap = 0;
    cont->رقم_قابها = 0;

    cont->sp = cont->val_پشته;
    cont->pc = i_function ? i_function->compiled : عدم;
    cont->r0 = 0;
#  إذا d_m3HasFloat
    cont->fp0 = 0;
#  endif
    cont->bound_آرگومانها_شمار    = 0;
    cont->رقم_Suspend_Results = 0;
    cont->رقم_Handlers       = 0;
    cont->handlersPC        = عدم;
    cont->والد            = عدم;

    cont->بعدی               = i_runtime->continuations;
    i_runtime->continuations = cont;

    أرجع cont;
}

IM3Continuation Continuation_ForkSuspended (IM3Runtime i_runtime, IM3Continuation i_cont)
{
    IM3Continuation newCont = Continuation_New(i_runtime, i_cont->type, i_cont->مدخل_Function);
    إذا (M3_UNLIKELY(not newCont)) {
        أرجع عدم;
    }

    newCont->val_پشته      = i_cont->val_پشته;
    newCont->رقم_پشته_Slots = i_cont->رقم_پشته_Slots;
    newCont->sp            = i_cont->sp;
    newCont->pc            = i_cont->pc;
    newCont->suspend_نقطه  = i_cont->suspend_نقطه;
    newCont->r0            = i_cont->r0;
#  إذا d_m3HasFloat
    newCont->fp0 = i_cont->fp0;
#  endif
    newCont->قابها    = i_cont->قابها;
    newCont->قابها_Cap = i_cont->قابها_Cap;
    newCont->رقم_قابها = i_cont->رقم_قابها;

    newCont->bound_آرگومانها_شمار = 0;
#  إذا d_m3HasExceptionHandling
    newCont->resumeThrow = i_cont->resumeThrow;
    i_cont->resumeThrow  = عدم;
#  endif
    newCont->رقم_Suspend_Results = i_cont->رقم_Suspend_Results;
    لكل (u32 i = 0; i < i_cont->رقم_Suspend_Results; ++i) {
        newCont->suspend_نتیجه_Offsets[i] = i_cont->suspend_نتیجه_Offsets[i];
        newCont->suspend_نتیجه_Is64[i]    = i_cont->suspend_نتیجه_Is64[i];
    }

    newCont->رقم_Handlers = i_cont->رقم_Handlers;
    newCont->handlersPC  = i_cont->handlersPC;
    newCont->والد      = i_cont->والد;
    newCont->state       = cont_suspended;

    i_cont->val_پشته  = عدم;
    i_cont->قابها    = عدم;
    i_cont->قابها_Cap = 0;
    i_cont->رقم_قابها = 0;
    i_cont->state     = cont_consumed;

    أرجع newCont;
}

// Only cont.new and snapshot restoration acquire stacks. Binding and suspension
// transfer them between records without changing the active count.
M3_نتیجه Continuation_Acquire_پشته (IM3Runtime runtime, IM3Continuation cont)
{
    إذا (not Continuation_Can_Acquire_پشته(runtime)) {
        أرجع m3_Err_continuation_حد_Exceeded;
    }
    إذا (runtime->val_پشته_Pool_شمار) {
        cont->val_پشته = runtime->val_پشته_Pool[--runtime->val_پشته_Pool_شمار];
        memset(cont->val_پشته, 0, (cont->رقم_پشته_Slots + 4) * حجم(m3slot_t));
    } وإلا {
        cont->val_پشته = m3_AllocArray(m3slot_t, cont->رقم_پشته_Slots + 4);
    }
    إذا (not cont->val_پشته) {
        أرجع m3Err_mallocFailed;
    }
    cont->sp = cont->val_پشته;
    runtime->continuationsAllocated++;
    أرجع m3Err_none;
}

فراغ Continuation_Recycle_پشته (IM3Runtime runtime, IM3Continuation cont)
{
    إذا (not cont->val_پشته or cont == runtime->ریشه_Continuation) {
        أرجع;
    }
    إذا (not runtime->val_پشته_Pool) {
        runtime->val_پشته_Pool = m3_AllocArray(m3slot_t*, 16);
        إذا (runtime->val_پشته_Pool) {
            runtime->val_پشته_Pool_Cap = 16;
        }
    }
    إذا (runtime->val_پشته_Pool_شمار < runtime->val_پشته_Pool_Cap and
        (not runtime->continuations_حد or runtime->val_پشته_Pool_شمار <
                                              runtime->continuations_حد - runtime->continuationsAllocated + 1)) {
        runtime->val_پشته_Pool[runtime->val_پشته_Pool_شمار++] = cont->val_پشته;
        cont->val_پشته                                      = عدم;
    } وإلا {
        m3_Free(cont->val_پشته);
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
        IM3Continuation بعدی = cont->بعدی;

        m3_Free(cont->val_پشته);
        m3_Free(cont->قابها);
        m3_Free(cont);

        cont = بعدی;
    }

    لكل (u32 i = 0; i < io_runtime->val_پشته_Pool_شمار; ++i) {
        m3_Free(io_runtime->val_پشته_Pool[i]);
    }
    m3_Free(io_runtime->val_پشته_Pool);
    io_runtime->val_پشته_Pool_شمار = io_runtime->val_پشته_Pool_Cap = 0;
    io_runtime->continuationsAllocated                          = 0;
    io_runtime->continuations                                   = عدم;
    io_runtime->فعال_Continuation                              = عدم;
    io_runtime->ریشه_Continuation                                = عدم;
}


m3ret_t Continuation_رکورد_قاب (IM3Runtime i_runtime, ثابت M3_قاب* i_قاب)
{
    IM3Continuation cont = i_runtime->فعال_Continuation;

    // the marker only travels between a suspend and the resume that runs it,
    // and that whole stretch has an active continuation
    d_m3Assert(cont);

    إذا (cont->رقم_قابها == cont->قابها_Cap) {
        u32 newCap = cont->قابها_Cap ? cont->قابها_Cap * 2 : 8;

        إذا (newCap > d_m3_Continuation_Max_قابها) {
            newCap = d_m3_Continuation_Max_قابها;
        }

        إذا (newCap == cont->قابها_Cap) {
            أرجع m3_Err_trap_پشته_سرریز;
        }

        M3_قاب* قابها = m3_ReallocArray(M3_قاب, cont->قابها, newCap, cont->قابها_Cap);

        إذا (not قابها) {
            أرجع m3Err_mallocFailed;
        }

        cont->قابها    = قابها;
        cont->قابها_Cap = newCap;
    }

    cont->قابها[cont->رقم_قابها++] = *i_قاب;

    أرجع m3Err_continuationSuspended;
}

#پایان_اگر // d_m3HasStackSwitching

M3_نتیجه Evaluate_عبارت (IM3Module i_module, فراغ* o_expressed, m3type_t i_type, بایتها_t* io_بایتها, cbytes_t i_انتها)
{
    M3_نتیجه نتیجه = m3Err_none;

    // OPTZ: use a simplified interpreter for expressions

    // create a temporary runtime context
#إذا defined(d_m3PreferStaticAlloc)
    ساكن M3Runtime runtime;
#وإلا
    M3Runtime runtime;
#پایان_اگر
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
    o->wasm            = *io_بایتها;
    o->wasm_انتها         = i_انتها;
    o->آخرین_Opcode_شروع = o->wasm;

    // Borrowed for the length of this expression only. Runtime_Release below hands it
    // to Environment_ReleaseCodePages, which resets lineIndex, so the page comes back
    // empty and the next expression reuses it rather than burning fresh lines.
    o->صفحه = Acquire_کد_صفحه(&runtime);

    إذا (o->صفحه) {
        IM3FuncType ftype = عدم;

#إذا d_m3HasTypedRefs
        // A reference type has to be checked as the type it is: the shared
        // return types are one per storage class, so borrowing one of those
        // would let any reference at all satisfy a particular one.
        إذا (IsSpelledRefType(i_type)) {
            نتیجه = AllocFuncType(&ftype, 1);

            إذا (not نتیجه) {
                ftype->رقم_Rets  = 1;
                ftype->types[0] = i_type;

                نتیجه = Environment_جمع_کن_Func_Type(runtime.environment, &ftype);
            }
        } وإلا
#پایان_اگر
        {
            ftype = runtime.environment->retFuncTypes[پایه_Type_از(i_type)];
        }

        pc_t m3code = Get_صفحه_PC(o->صفحه);

        إذا (not نتیجه) {
            نتیجه = Compile_عبارت(o, ftype);
        }

        إذا (not نتیجه && o->max_پشته_Slots >= runtime.رقم_پشته_Slots) {
            نتیجه = m3_Err_trap_پشته_سرریز;
        }

        إذا (not نتیجه) {
#إذا (d_m3EnableOpProfiling || d_m3EnableOpTracing)
            m3ret_t r = پیاپی_کد(m3code, پشته, عدم, d_m3_Op_Default_آرگومانها, d_m3_پایه_Cstr);
#وإلا
            m3ret_t r = پیاپی_کد(m3code, پشته, عدم, d_m3_Op_Default_آرگومانها);
#پایان_اگر

            إذا (r == 0) {                                                                   m3log (runtime, "expression result: %s", SPrint_مقدار (پشته, i_type));
                إذا (اندازه_از_Type(پایه_Type_از(i_type)) == حجم(u32)) {
                    *(u32*)o_expressed = *((u32*)پشته);
                } وإلا {
                    *(u64*)o_expressed = *((u64*)پشته);
                }
            }
        }

        Release_کد_صفحه(&runtime, o->صفحه);
    } وإلا {
        نتیجه = m3_Err_malloc_Failed_کد_صفحه;
    }

    runtime.origin_پشته = عدم;        // prevent free(stack) in ReleaseRuntime
    Runtime_Release(&runtime);
    i_module->runtime = savedRuntime;

    *io_بایتها = o->wasm;

    أرجع نتیجه;
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
ثابت M3Export* Module_پیدا_کن_Export (IM3Module i_module, u8 i_نوع, cstr_t i_name)
{
    لكل (u32 i = 0; i < i_module->رقم_Exports; ++i) {
        ثابت M3Export* e = &i_module->exports[i];

        إذا (e->نوع == i_نوع and strcmp(e->name, i_name) == 0) {
            أرجع e;
        }
    }

    أرجع عدم;
}


ساكن
IM3Function Module_پیدا_کن_Exported_Function (IM3Module i_module, cstr_t i_name)
{
    ثابت M3Export* e = Module_پیدا_کن_Export(i_module, d_external_نوع_function, i_name);

    // A module that re-exports an import names the placeholder here. Resolve to
    // the function that actually runs, or a host-side call would run it against
    // the importing module's memory.
    أرجع e ? Function_Implementation(&i_module->functions[e->index]) : عدم;
}


ساكن
IM3_حافظه Module_پیدا_کن_Exported_حافظه (IM3Module i_module, cstr_t i_name)
{
    ثابت M3Export* e = Module_پیدا_کن_Export(i_module, d_external_نوع_حافظه, i_name);

    أرجع e ? i_module->memories[e->index] : عدم;
}


ساكن
IM3_جدول Module_پیدا_کن_Exported_جدول (IM3Module i_module, cstr_t i_name)
{
    ثابت M3Export* e = Module_پیدا_کن_Export(i_module, d_external_نوع_جدول, i_name);

    أرجع e ? i_module->tables[e->index] : عدم;
}


ساكن
IM3Global Module_پیدا_کن_Exported_Global (IM3Module i_module, cstr_t i_name)
{
    ثابت M3Export* e = Module_پیدا_کن_Export(i_module, d_external_نوع_global, i_name);

    أرجع e ? &i_module->globals[e->index] : عدم;
}


#إذا d_m3HasExceptionHandling || d_m3_Has_پشته_Switching
ساكن
IM3_برچسب Module_پیدا_کن_Exported_برچسب (IM3Module i_module, cstr_t i_name)
{
    ثابت M3Export* e = Module_پیدا_کن_Export(i_module, d_external_نوع_برچسب, i_name);

    إذا (not e) {
        أرجع عدم;
    }

    IM3_برچسب برچسب = &i_module->برچسبها[e->index];

    أرجع برچسب->resolved ? برچسب->resolved : برچسب;
}
#پایان_اگر


// Whether the module exports anything at all under this name. Export names are
// unique within a module, so a name one of the lookups above missed but this
// one finds is exported as something else - a kind the import cannot be
// satisfied by, rather than a name the module never exported.
ساكن
bool Module_HasExport (IM3Module i_module, cstr_t i_name)
{
    لكل (u32 i = 0; i < i_module->رقم_Exports; ++i) {
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
M3_نتیجه LinkImports (IM3Runtime io_runtime, IM3Module io_module)
{
    M3_نتیجه نتیجه = m3Err_none;

    لكل (u32 i = 0; i < io_module->رقم_Functions; ++i) {
        IM3Function f = &io_module->functions[i];

        إذا (f->wasm or not (f->import.moduleUtf8 and f->import.فیلد_Utf8)) {
            استمر;
        }

        IM3Module from = m3_پیدا_کن_Module(io_runtime, f->import.moduleUtf8);
        إذا (not from) {
            استمر;
        }

        IM3Function exported = Module_پیدا_کن_Exported_Function(from, f->import.فیلد_Utf8);
        إذا (not exported) {
            // the import names a module that is loaded, so its exports settle the
            // question: a name it exports as something else is a type mismatch, and
            // one it does not export at all is an unknown import
            _throwif(m3Err_incompatibleImportType, Module_HasExport(from, f->import.فیلد_Utf8));
            _throw(m3Err_unknownImport);
        }

        // func types are canonical within an environment, so this is the
        // structural equivalence the spec asks for
        _throwif(m3Err_incompatibleImportType, exported->funcType != f->funcType);

        f->resolved = Function_Implementation(exported);
    }

    لكل (u32 i = 0; i < io_module->رقم_Memories; ++i) {
        IM3_حافظه حافظه = io_module->memories[i];

        إذا (not حافظه->imported or حافظه->owner != io_module) {
            استمر;
        }

        IM3Module from = m3_پیدا_کن_Module(io_runtime, حافظه->import.moduleUtf8);
        إذا (not from) {
            استمر;
        }

        IM3_حافظه exported = Module_پیدا_کن_Exported_حافظه(from, حافظه->import.فیلد_Utf8);
        إذا (not exported) {
            _throwif(m3Err_incompatibleImportType, Module_HasExport(from, حافظه->import.فیلد_Utf8));
            _throw(m3Err_unknownImport);
        }

        // the address type is part of the memory type, so an i64 memory does
        // not satisfy an i32 import, or the other way round - and so is the page
        // size, which custom page sizes made a declared property of a memory
        _throwif(m3Err_incompatibleImportType, exported->isMemory64 != حافظه->isMemory64);
        _throwif(m3Err_incompatibleImportType, حافظه_صفحه_اندازه(exported) != حافظه_صفحه_اندازه(حافظه));

        // a shared memory is not a subtype of an unshared one, or the other way round
        _throwif(m3Err_incompatibleImportType, exported->isShared != حافظه->isShared);

        _throwif(m3Err_incompatibleImportType,
                 not LimitsSatisfy(exported->رقم_صفحات, exported->hasMax, exported->max_صفحات,
                                   حافظه->init_صفحات, حافظه->hasMax, حافظه->max_صفحات));

        // hand the slot over to the exporter's memory, and drop the placeholder
        Free_حافظه_بلوک(حافظه);
        FreeImportInfo(&حافظه->import);
        m3_Free(حافظه);

        io_module->memories[i] = exported;
    }

    لكل (u32 i = 0; i < io_module->رقم_Tables; ++i) {
        IM3_جدول جدول_2 = io_module->tables[i];

        إذا (not جدول_2->imported or جدول_2->owner != io_module) {
            استمر;
        }

        IM3Module from = m3_پیدا_کن_Module(io_runtime, جدول_2->import.moduleUtf8);
        إذا (not from) {
            استمر;
        }

        IM3_جدول exported = Module_پیدا_کن_Exported_جدول(from, جدول_2->import.فیلد_Utf8);
        إذا (not exported) {
            _throwif(m3Err_incompatibleImportType, Module_HasExport(from, جدول_2->import.فیلد_Utf8));
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

    لكل (u32 i = 0; i < io_module->رقم_Globals; ++i) {
        IM3Global g = &io_module->globals[i];

        إذا (not g->imported or not (g->import.moduleUtf8 and g->import.فیلد_Utf8)) {
            استمر;
        }

        IM3Module from = m3_پیدا_کن_Module(io_runtime, g->import.moduleUtf8);
        إذا (not from) {
            استمر;
        }

        IM3Global exported = Module_پیدا_کن_Exported_Global(from, g->import.فیلد_Utf8);
        إذا (not exported) {
            _throwif(m3Err_incompatibleImportType, Module_HasExport(from, g->import.فیلد_Utf8));
            _throw(m3Err_unknownImport);
        }

        _throwif(m3Err_incompatibleImportType, exported->type != g->type);
        _throwif(m3Err_incompatibleImportType, exported->isMutable != g->isMutable);

        g->resolved = exported->resolved ? exported->resolved : exported;
    }

#إذا d_m3HasExceptionHandling || d_m3_Has_پشته_Switching
    لكل (u32 i = 0; i < io_module->رقم_برچسبها; ++i) {
        IM3_برچسب برچسب = &io_module->برچسبها[i];

        إذا (not برچسب->imported or not (برچسب->import.moduleUtf8 and برچسب->import.فیلد_Utf8)) {
            استمر;
        }

        IM3Module from = m3_پیدا_کن_Module(io_runtime, برچسب->import.moduleUtf8);
        إذا (not from) {
            استمر;
        }

        IM3_برچسب exported = Module_پیدا_کن_Exported_برچسب(from, برچسب->import.فیلد_Utf8);
        إذا (not exported) {
            _throwif(m3Err_incompatibleImportType, Module_HasExport(from, برچسب->import.فیلد_Utf8));
            _throw(m3Err_unknownImport);
        }

        _throwif(m3Err_incompatibleImportType, exported->type != برچسب->type);

        برچسب->resolved = exported->resolved ? exported->resolved : exported;
    }
#پایان_اگر

    _catch: أرجع نتیجه;
}


// Backs each of the module's memories with pages. LinkImports has already
// pointed any import it could satisfy at the exporting module's memory, and
// those are skipped here. An import nothing satisfied is still backed locally
// from its own declared limits, so that the module remains loadable.
M3_نتیجه Init_حافظه (IM3Runtime io_runtime, IM3Module i_module)
{
    M3_نتیجه نتیجه = m3Err_none;

    // Fixed from here on: the index space stops changing after parse, and
    // linking has already repointed any slot it was going to.
    i_module->memory0 = i_module->رقم_Memories ? i_module->memories[0]
                                              : &i_module->خالی_حافظه;

    إذا (i_module->رقم_Memories == 0) {
        // nothing addressable, but _mem still has to point somewhere
        i_module->خالی_حافظه.owner    = i_module;
        i_module->خالی_حافظه.صفحه_اندازه = d_m3_Default_Mem_صفحه_اندازه;

_       (Resize_حافظه(io_runtime, &i_module->خالی_حافظه, 0));
    }

    لكل (u32 i = 0; i < i_module->رقم_Memories; ++i) {
        IM3_حافظه حافظه = i_module->memories[i];

        // a slot that already points at another module's memory is that
        // module's to allocate
        إذا (حافظه->owner != i_module or حافظه->mallocated) {
            استمر;
        }

        u32 صفحه_اندازه = حافظه_صفحه_اندازه(حافظه);

        حافظه->صفحه_اندازه = صفحه_اندازه;

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
            حافظه->max_صفحات = حافظه->isMemory64
                                 ? (صفحه_اندازه > 1 ? (UINT64_MAX / صفحه_اندازه) + 1 : UINT64_MAX)
                                 : (0x100000000ull / صفحه_اندازه);
        }

_       (Resize_حافظه(io_runtime, حافظه, حافظه->init_صفحات));
    }

    _catch: أرجع نتیجه;
}


M3_نتیجه Resize_حافظه (IM3Runtime io_runtime, IM3_حافظه حافظه, u64 i_رقم_صفحات)
{
    M3_نتیجه نتیجه = m3Err_none;

    u64 رقم_صفحات_مقصد_Alloc = i_رقم_صفحات;

    إذا (رقم_صفحات_مقصد_Alloc <= حافظه->max_صفحات) {
        // A 64-bit memory may ask for up to 2^48 pages, which overflows a u64
        // of bytes. Nothing that large can be backed, so refuse it up front
        // rather than multiplying into a wrapped size.
        _throwif("linear memory limitation exceeded",
                 رقم_صفحات_مقصد_Alloc > d_m3_آدرس_حد / حافظه->صفحه_اندازه);

        u64 رقم_صفحه_بایتها = رقم_صفحات_مقصد_Alloc * حافظه->صفحه_اندازه;

#إذا d_m3_Max_خطی_حافظه_صفحات > 0
        // the limit is a memory size, counted in default-sized pages; comparing
        // it against a raw page count would make it 65536 times stricter for a
        // module whose pages are one byte
        _throwif("linear memory limitation exceeded",
                 رقم_صفحه_بایتها > (u64)d_m3_Max_خطی_حافظه_صفحات * d_m3_Default_Mem_صفحه_اندازه);
#پایان_اگر

        // Limit the amount of memory that gets actually allocated
        إذا (io_runtime->حافظه_حد) {
            رقم_صفحه_بایتها = M3_MIN(رقم_صفحه_بایتها, (u64)io_runtime->حافظه_حد);
        }

        u64 old_درازی = حافظه->mallocated ? حافظه->mallocated->درازی : 0;
        u64 other_استفاده_شده = io_runtime->حافظه_بایتها_استفاده_شده - old_درازی;
        _throwif(m3_Err_حافظه_حد_Exceeded, io_runtime->حافظه_بایتها_حد and
                                              (رقم_صفحه_بایتها > io_runtime->حافظه_بایتها_حد or
                                               other_استفاده_شده > io_runtime->حافظه_بایتها_حد - رقم_صفحه_بایتها));

        _throwif("linear memory limitation exceeded", رقم_صفحه_بایتها > (u64)SIZE_MAX - حجم(M3_حافظه_سرآیند));

#إذا d_m3_Guarded_حافظه

        // A slot holds every address a Wasm access can name, and nothing beyond
        // that: a memory larger than the data part of one would put addresses past
        // the guard, where a fault is nobody's to recognize
        _throwif("linear memory limitation exceeded", رقم_صفحه_بایتها > d_m3_Guarded_داده_بایتها);

        // What makes an access past the end fault is the page after the memory not
        // being committed, so the end has to fall on one of the system's page
        // boundaries. A Wasm page divides evenly into them and always will; a memory
        // that declared a page size of its own may not, and then the bytes between
        // the end of the memory and the end of its last system page would answer
        // instead of trapping. Refusing to back such a memory is the honest way out:
        // the custom page sizes proposal exists for targets far too small to reserve
        // 8GiB per memory in the first place. See d_m3GuardedMemory.
        _throwif("guarded memory needs a page size the system's divides into",
                 (حافظه->صفحه_اندازه % m3_Host_صفحه_اندازه()) != 0);

        إذا (حافظه->guard_جایگاه == عدم) {
            حافظه->guard_جایگاه = Guard_Take_جایگاه();
            _throwifnull(حافظه->guard_جایگاه);
        }

        // Growing only ever commits more of the same slot, so the memory never
        // moves - which the interpreter does not rely on, but nothing minds
        _throwif(m3Err_mallocFailed, not Guard_Commit_جایگاه(حافظه->guard_جایگاه, (size_t)رقم_صفحه_بایتها));

        حافظه->mallocated = Guard_جایگاه_سرآیند(حافظه->guard_جایگاه);

#وإلا

        size_t رقم_بایتها = (size_t)رقم_صفحه_بایتها + حجم(M3_حافظه_سرآیند);

        // What is really there, which is not numPages * pageSize: the limit above
        // caps the bytes without capping the page count, so a memory can be short
        // of a page - by design, since a whole 64 KiB page is often more than an
        // MCU has. Measuring the old block by the page count would overstate it,
        // and then realloc never sees the size it already has and copies the
        // whole limit again on every grow past it.
        size_t رقم_قبلی_بایتها = حافظه->mallocated ? حافظه->mallocated->درازی + حجم(M3_حافظه_سرآیند) : 0;

        فراغ* newMem = m3_Realloc("Wasm Linear Memory", حافظه->mallocated, رقم_بایتها, رقم_قبلی_بایتها);
        _throwifnull(newMem);

        حافظه->mallocated = (M3_حافظه_سرآیند*)newMem;

#پایان_اگر

#إذا d_m3LogRuntime
        M3_حافظه_سرآیند* oldMallocated = حافظه->mallocated;
#پایان_اگر

        io_runtime->حافظه_بایتها_استفاده_شده = other_استفاده_شده + رقم_صفحه_بایتها;
        حافظه->رقم_صفحات            = رقم_صفحات_مقصد_Alloc;

        حافظه->mallocated->درازی  = (size_t)رقم_صفحه_بایتها;
        حافظه->mallocated->runtime = io_runtime;
        حافظه->mallocated->حافظه  = حافظه;

        حافظه->mallocated->max_پشته = (m3slot_t*)io_runtime->origin_پشته + io_runtime->رقم_پشته_Slots;

        m3log(runtime, "resized old: %p; mem: %p; length: %zu; pages: %llu", oldMallocated, حافظه->mallocated, حافظه->mallocated->درازی, (بیعلامت دراز دراز)حافظه->رقم_صفحات);
    } وإلا {
        نتیجه = m3_Err_wasm_حافظه_سرریز;
    }

    _catch: أرجع نتیجه;
}


فراغ Free_حافظه_بلوک (IM3_حافظه io_حافظه)
{
    إذا (io_حافظه->mallocated and io_حافظه->mallocated->runtime) {
        io_حافظه->mallocated->runtime->حافظه_بایتها_استفاده_شده -= io_حافظه->mallocated->درازی;
    }
#إذا d_m3_Guarded_حافظه
    Guard_Give_جایگاه(io_حافظه->guard_جایگاه);
    io_حافظه->guard_جایگاه = عدم;

    io_حافظه->mallocated = عدم;
#وإلا
    m3_Free(io_حافظه->mallocated);
#پایان_اگر
}


M3_نتیجه InitGlobals (IM3Module io_module)
{
    M3_نتیجه نتیجه = m3Err_none;

    إذا (io_module->رقم_Globals) {
        // placing the globals in their structs isn't good for cache locality, but i don't really know what the global
        // access patterns typically look like yet.

        //          io_module->globalMemory = m3Alloc (m3reg_t, io_module->numGlobals);

        //          if (io_module->globalMemory)
        {
            لكل (u32 i = 0; i < io_module->رقم_Globals; ++i) {
                M3Global* g = &io_module->globals[i];                           m3log (runtime, "initializing global: %d", i);

                إذا (g->init_عبارت) {
                    بایتها_t شروع = g->init_عبارت;

                    نتیجه = Evaluate_عبارت(io_module, &g->i64_مقدار, g->type, &شروع, g->init_عبارت + g->init_عبارت_اندازه);

                    إذا (not نتیجه) {
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

    أرجع نتیجه;
}


M3_نتیجه Init_داده_Segments (IM3Module io_module)
{
    M3_نتیجه نتیجه = m3Err_none;

    لكل (u32 i = 0; i < io_module->رقم_داده_Segments; ++i) {
        M3_داده_Segment* segment = &io_module->داده_Segments[i];

        // A passive segment stays available for memory.init until data.drop.
        // An active one is copied here and then counts as dropped.
        إذا (segment->isPassive) {
            استمر;
        }

        _throwif("data segment memory index out of range",
                 segment->حافظه_Region >= io_module->رقم_Memories);

        IM3_حافظه io_حافظه = io_module->memories[segment->حافظه_Region];

        _throwif("unallocated linear memory", !(io_حافظه->mallocated));

        // The offset expression has the memory's address type, and is
        // unsigned: an i32 offset of -1 is 4294967295, way out of bounds
        // rather than negative.
        u64     segment_جابه_جایی = 0;
        بایتها_t شروع         = segment->init_عبارت;

        إذا (io_حافظه->isMemory64) {
_           (Evaluate_عبارت(io_module, &segment_جابه_جایی, c_m3Type_i64, &شروع, segment->init_عبارت + segment->init_عبارت_اندازه));
        } وإلا {
            u32 offset32;
_           (Evaluate_عبارت(io_module, &offset32, c_m3Type_i32, &شروع, segment->init_عبارت + segment->init_عبارت_اندازه));
            segment_جابه_جایی = offset32;
        }

        m3log(runtime, "loading data segment: %d; size: %d; offset: %llu", i, segment->اندازه, (بیعلامت دراز دراز)segment_جابه_جایی);

        إذا (segment_جابه_جایی <= io_حافظه->mallocated->درازی &&
            (u64)segment->اندازه <= io_حافظه->mallocated->درازی - segment_جابه_جایی) {
            u8* dest = m3_Mem_داده(io_حافظه->mallocated) + segment_جابه_جایی;
            کپی_حافظه(dest, segment->داده, segment->اندازه);
        } وإلا {
            _throw("data segment out of bounds");
        }

        segment->dropped = true;
    }

    _catch: أرجع نتیجه;
}


// Turns a segment's elements into references. Element expressions are constant
// expressions restricted to ref.null/ref.func, so they're read directly rather
// than run through the compiler.
ساكن
M3_نتیجه ResolveElements (IM3Module io_module, M3ElementSegment* i_segment, فراغ** o_elements)
{
    M3_نتیجه نتیجه = m3Err_none;

    بایتها_t  pos = i_segment->elements;
    cbytes_t انتها = io_module->element_Section_انتها;

    لكل (u32 e = 0; e < i_segment->رقم_Elements; ++e) {
        u32   func_شاخص;
        فراغ* ref = عدم;

        إذا (i_segment->is_عبارت) {
            m3opcode_t opcode;
_           (Read_opcode(&opcode, &pos, انتها));

            إذا (opcode == c_waOp_refFunc) {
_               (ReadLEB_u32(&func_شاخص, &pos, انتها));
                _throwif("function index out of range", func_شاخص >= io_module->رقم_Functions);
                ref = Function_Implementation(&io_module->functions[func_شاخص]);
            } وإلا إذا (opcode == c_waOp_refNull) {
                i8 waType;
                u8 nullType;
_               (ReadLEB_i7(&waType, &pos, انتها));
_               (NormalizeType(&nullType, waType));
                _throwif(m3Err_typeMismatch, nullType != i_segment->type);
            } وإلا إذا (opcode == c_waOp_getGlobal) {
                // wasm 2.0 lets an element expression read an imported
                // immutable global, which is how one module seeds another's
                // table with a reference it exported.
                u32 global_شاخص;
_               (ReadLEB_u32(&global_شاخص, &pos, انتها));
                _throwif(m3_Err_globa_شاخص_بیرون_از_Bounds, global_شاخص >= io_module->رقم_Globals);

                IM3Global global = &io_module->globals[global_شاخص];

                _throwif(m3_Err_globa_شاخص_بیرون_از_Bounds, not global->imported);
                _throwif(m3Err_wasmMalformed, global->isMutable);
                _throwif(m3Err_typeMismatch, پایه_Type_از(global->type) != پایه_Type_از(i_segment->type));

                // read the cell the import was linked to, not the placeholder
                إذا (global->resolved) {
                    global = global->resolved;
                }

                ref = global->ref_مقدار;
            } وإلا {
                _throw("constant expression required");
            }

_           (Read_opcode(&opcode, &pos, انتها));
            _throwif(m3Err_wasmMalformed, opcode != c_wa_Op_انتها);
        } وإلا {
_           (ReadLEB_u32(&func_شاخص, &pos, انتها));
            _throwif("function index out of range", func_شاخص >= io_module->رقم_Functions);
            ref = Function_Implementation(&io_module->functions[func_شاخص]);
        }

        o_elements[e] = ref;
    }

    _catch: أرجع نتیجه;
}


M3_نتیجه Init_جدول_And_Elements (IM3Module io_module)
{
    M3_نتیجه نتیجه = m3Err_none;

    cbytes_t انتها = io_module->element_Section_انتها;
    M3_جدول* جدول_2;

    لكل (u32 i = 0; i < io_module->رقم_Tables; ++i) {
        جدول_2 = io_module->tables[i];

        // a slot pointing at another module's table is that module's to fill
        إذا (جدول_2->owner != io_module) {
            استمر;
        }

        إذا (جدول_2->اندازه) {
            IM3Runtime runtime = io_module->runtime;
            _throwif(m3_Err_جدول_حد_Exceeded, runtime->جدول_Elements_حد and
                                                 جدول_2->اندازه > runtime->جدول_Elements_حد - runtime->جدول_Elements_استفاده_شده);
            جدول_2->elements = m3_AllocArray(فراغ*, جدول_2->اندازه);
            _throwifnull(جدول_2->elements);
            runtime->جدول_Elements_استفاده_شده += جدول_2->اندازه;

            إذا (جدول_2->init_عبارت) {
                فراغ*   مقدار = عدم;
                بایتها_t شروع = جدول_2->init_عبارت;
_               (Evaluate_عبارت(io_module, &مقدار, پایه_Type_از(جدول_2->type),
                                     &شروع, جدول_2->init_عبارت + جدول_2->init_عبارت_اندازه));

                لكل (u32 e = 0; e < جدول_2->اندازه; ++e) {
                    جدول_2->elements[e] = مقدار;
                }
            }
        }
    }

    لكل (u32 i = 0; i < io_module->رقم_Element_Segments; ++i) {
        M3ElementSegment* segment = &io_module->elementSegments[i];

        // Declarative segments only make their functions referenceable, and
        // passive ones wait for table.init, so neither is written out here.
        إذا (segment->نما == c_m3Elem_declarative) {
            segment->dropped = true;
            استمر;
        }

        إذا (segment->نما == c_m3Elem_passive) {
            إذا (segment->رقم_Elements) {
                segment->resolved = m3_AllocArray(فراغ*, segment->رقم_Elements);
                _throwifnull(segment->resolved);
_               (ResolveElements(io_module, segment, segment->resolved));
            }
            استمر;
        }

        جدول_2 = io_module->tables[segment->جدول_شاخص];

        // The offset expression has the table's index type, and is unsigned:
        // an i32 offset of -1 is 4294967295, out of bounds rather than negative.
        u64     offset = 0;
        بایتها_t عبارت   = segment->init_عبارت;

        إذا (جدول_2->isTable64) {
_           (Evaluate_عبارت(io_module, &offset, c_m3Type_i64, &عبارت, انتها));
        } وإلا {
            u32 offset32;
_           (Evaluate_عبارت(io_module, &offset32, c_m3Type_i32, &عبارت, انتها));
            offset = offset32;
        }

        _throwif("out of bounds table access",
                 offset > جدول_2->اندازه or segment->رقم_Elements > جدول_2->اندازه - offset);

_       (ResolveElements(io_module, segment, جدول_2->elements + offset));

        segment->dropped = true;
    }

    _catch: أرجع نتیجه;
}

M3_نتیجه m3_CompileModule (IM3Module io_module)
{
    M3_نتیجه نتیجه = m3Err_none;

    لكل (u32 i = 0; i < io_module->رقم_Functions; ++i) {
        IM3Function f = &io_module->functions[i];
        إذا (f->wasm and not f->compiled) {
_           (CompileFunction(f));
        }
    }

    _catch: أرجع نتیجه;
}

#إذا d_m3HasExceptionHandling

M3Exception* NewException (IM3Runtime io_runtime, IM3_برچسب i_برچسب, u32 i_رقم_آرگومانها)
{
    M3Exception* exception = (M3Exception*)m3_Malloc("M3Exception", حجم(M3Exception) + i_رقم_آرگومانها * حجم(u64));

    إذا (exception) {
        exception->برچسب     = i_برچسب;
        exception->رقم_آرگومانها = i_رقم_آرگومانها;
        exception->reified = false;
        exception->prev    = عدم;
        exception->بعدی    = io_runtime->exceptions;

        إذا (exception->بعدی) {
            exception->بعدی->prev = exception;
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
        i_exception->prev->بعدی = i_exception->بعدی;
    } وإلا {
        io_runtime->exceptions = i_exception->بعدی;
    }

    إذا (i_exception->بعدی) {
        i_exception->بعدی->prev = i_exception->prev;
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
        M3Exception* بعدی = exception->بعدی;
        m3_Free_Impl(exception);
        exception = بعدی;
    }
}


// Keeps a copy of the exception a call just ended with, for m3_GetExceptionTag,
// or forgets the last one when the call ended any other way. A copy, because
// the original is freed with the rest before the outermost call returns - and
// one a host function's nested call ended with may still be named by an
// exnref further down the stack.
ساكن
فراغ رکورد_Uncaught_Exception (IM3Runtime io_runtime, M3_نتیجه i_نتیجه)
{
    إذا (io_runtime->uncaughtException) {
        m3_Free(io_runtime->uncaughtException);
    }

    M3Exception* exception = io_runtime->pendingException;

    إذا (M3_UNLIKELY(i_نتیجه == m3Err_trapUncaughtException) and exception) {
        M3Exception* copy = (M3Exception*)m3_CopyMem(exception, حجم(M3Exception) + exception->رقم_آرگومانها * حجم(u64));

        إذا (copy) {
            copy->بعدی = عدم;
            copy->prev = عدم;
        }

        io_runtime->uncaughtException = copy;
    }
}

#پایان_اگر // d_m3HasExceptionHandling


#إذا d_m3HasExceptionHandling || d_m3_Has_پشته_Switching

IM3_برچسب m3_پیدا_کن_برچسب (IM3Module i_module, ثابت حرف* ثابت i_برچسب_Name)
{
    إذا (not i_module or not i_برچسب_Name) {
        أرجع عدم;
    }

    أرجع Module_پیدا_کن_Exported_برچسب(i_module, i_برچسب_Name);
}


M3_نتیجه m3_New_برچسب (IM3Runtime io_runtime, IM3_برچسب* o_برچسب, ثابت حرف* ثابت i_signature)
{
    IM3FuncType type    = عدم;
    M3_Host_برچسب*  host_برچسب = عدم;

_try {
    _throwif("null tag", not o_برچسب);
    *o_برچسب = عدم;
    _throwif(m3Err_mallocFailed, not io_runtime);

_   (Signature_مقصد_Func_Type(&type, i_signature));
    // tags are the same type exactly when their types are the same object
_   (Environment_جمع_کن_Func_Type(io_runtime->environment, &type));

    host_برچسب = m3_AllocStruct(M3_Host_برچسب);
    _throwifnull(host_برچسب);

    host_برچسب->برچسب.type    = type;
    host_برچسب->بعدی        = io_runtime->host_برچسبها;
    io_runtime->host_برچسبها = host_برچسب;

    *o_برچسب = &host_برچسب->برچسب;
} _catch:
    أرجع نتیجه;
}


M3_نتیجه m3_Link_برچسب (IM3Module         io_module,
                     ثابت حرف* ثابت i_moduleName,
                     ثابت حرف* ثابت i_برچسب_Name,
                     IM3_برچسب            i_برچسب)
{
    إذا (not io_module or not i_moduleName or not i_برچسب_Name or not i_برچسب) {
        أرجع m3_Err_برچسب_Lookup_Failed;
    }

    ثابت bool wildcardModule = (strcmp(i_moduleName, "*") == 0);
    IM3_برچسب     هدف         = i_برچسب->resolved ? i_برچسب->resolved : i_برچسب;

    M3_نتیجه نتیجه = m3_Err_برچسب_Lookup_Failed;

    لكل (u32 i = 0; i < io_module->رقم_برچسبها; ++i) {
        IM3_برچسب برچسب = &io_module->برچسبها[i];

        إذا (برچسب->imported and برچسب->import.moduleUtf8 and برچسب->import.فیلد_Utf8 and
            strcmp(برچسب->import.فیلد_Utf8, i_برچسب_Name) == 0 and
            (wildcardModule or strcmp(برچسب->import.moduleUtf8, i_moduleName) == 0)) {
            إذا (برچسب->type != هدف->type) {
                أرجع m3Err_incompatibleImportType;
            }

            برچسب->resolved = هدف;
            نتیجه        = m3Err_none;
        }
    }

    أرجع نتیجه;
}


uint32_t m3_Get_برچسب_Arg_شمار (IM3_برچسب i_برچسب)
{
    أرجع i_برچسب ? i_برچسب->type->رقم_آرگومانها : 0;
}


M3_مقدار_Type m3_Get_برچسب_Arg_Type (IM3_برچسب i_برچسب, uint32_t i_شاخص)
{
    إذا (i_برچسب and i_شاخص < i_برچسب->type->رقم_آرگومانها) {
        أرجع (M3_مقدار_Type)پایه_Type_از(d_FuncArgType(i_برچسب->type, i_شاخص));
    }

    أرجع c_m3Type_none;
}

#وإلا

IM3_برچسب m3_پیدا_کن_برچسب (IM3Module i_module, ثابت حرف* ثابت i_برچسب_Name)
{
    أرجع عدم;
}

M3_نتیجه m3_New_برچسب (IM3Runtime io_runtime, IM3_برچسب* o_برچسب, ثابت حرف* ثابت i_signature)
{
    إذا (o_برچسب) {
        *o_برچسب = عدم;
    }
    أرجع m3_Err_برچسب_Lookup_Failed;
}

M3_نتیجه m3_Link_برچسب (IM3Module io_module, ثابت حرف* ثابت i_moduleName, ثابت حرف* ثابت i_برچسب_Name, IM3_برچسب i_برچسب)
{
    أرجع m3_Err_برچسب_Lookup_Failed;
}

uint32_t m3_Get_برچسب_Arg_شمار (IM3_برچسب i_برچسب)
{
    أرجع 0;
}

M3_مقدار_Type m3_Get_برچسب_Arg_Type (IM3_برچسب i_برچسب, uint32_t i_شاخص)
{
    أرجع c_m3Type_none;
}

#پایان_اگر // d_m3HasExceptionHandling || d_m3HasStackSwitching


#إذا d_m3HasExceptionHandling

M3_نتیجه m3_ThrowException (IM3Runtime io_runtime, IM3_برچسب i_برچسب, uint32_t i_argc, ثابت فراغ* i_argptrs[])
{
    إذا (not io_runtime or not i_برچسب) {
        أرجع m3_Err_برچسب_Lookup_Failed;
    }

    IM3_برچسب      برچسب  = i_برچسب->resolved ? i_برچسب->resolved : i_برچسب;
    IM3FuncType type = برچسب->type;

    // a tag with results is a stack switching control tag, which is suspended
    // with rather than thrown
    إذا (type->رقم_Rets) {
        أرجع m3Err_typeMismatch;
    }
    إذا (i_argc != type->رقم_آرگومانها) {
        أرجع m3_Err_آرگومان_شمار_Mismatch;
    }
    // with nothing on the stack there is nothing to unwind, and the exception
    // would sit on the runtime's list until some later call ended
    إذا (io_runtime->callNesting == 0) {
        أرجع "m3_ThrowException called outside a host function";
    }

    M3Exception* exception = NewException(io_runtime, برچسب, i_argc);
    إذا (not exception) {
        أرجع m3Err_mallocFailed;
    }

    // the layout op_Throw gives a payload: 32-bit values zero-extended, floats
    // as their bit pattern
    لكل (u32 i = 0; i < i_argc; ++i) {
        ثابت فراغ* مقدار = i_argptrs[i];

        بدل (پایه_Type_از(d_FuncArgType(type, i))) {
        حالة c_m3Type_i32:
        حالة c_m3Type_f32: {
            u32 بیتها;
            کپی_حافظه(&بیتها, مقدار, حجم(بیتها));
            exception->آرگومانها[i] = بیتها;
            اكسر;
        }
        حالة c_m3Type_i64:
        حالة c_m3Type_f64:
            کپی_حافظه(&exception->آرگومانها[i], مقدار, حجم(u64));
            اكسر;
        حالة c_m3Type_funcref:
        حالة c_m3Type_externref:
        حالة c_m3Type_exnref:
        حالة c_m3Type_contref:
            exception->آرگومانها[i] = (u64)(*(ثابت uintptr_t*)مقدار);
            اكسر;
        افتراضي:
            FreeException(io_runtime, exception);
            أرجع "unknown argument type";
        }
    }

    io_runtime->pendingException = exception;

    أرجع m3Err_pendingException;
}


IM3_برچسب m3_Get_Exception_برچسب (IM3Runtime i_runtime)
{
    إذا (i_runtime and i_runtime->uncaughtException) {
        أرجع i_runtime->uncaughtException->برچسب;
    }

    أرجع عدم;
}


M3_نتیجه m3_Get_Exception_آرگومانها (IM3Runtime i_runtime, uint32_t i_argc, ثابت فراغ* o_argptrs[])
{
    إذا (not i_runtime or not i_runtime->uncaughtException) {
        أرجع "no uncaught exception";
    }

    ثابت M3Exception* exception = i_runtime->uncaughtException;
    IM3FuncType        type      = exception->برچسب->type;

    إذا (i_argc != exception->رقم_آرگومانها) {
        أرجع m3_Err_آرگومان_شمار_Mismatch;
    }

    لكل (u32 i = 0; i < i_argc; ++i) {
        فراغ* مقدار = (فراغ*)o_argptrs[i];
        u64   بیتها  = exception->آرگومانها[i];

        بدل (پایه_Type_از(d_FuncArgType(type, i))) {
        حالة c_m3Type_i32:
        حالة c_m3Type_f32: {
            u32 کم = (u32)بیتها;
            کپی_حافظه(مقدار, &کم, حجم(کم));
            اكسر;
        }
        حالة c_m3Type_i64:
        حالة c_m3Type_f64:
            کپی_حافظه(مقدار, &بیتها, حجم(بیتها));
            اكسر;
        حالة c_m3Type_funcref:
        حالة c_m3Type_externref:
        حالة c_m3Type_exnref:
        حالة c_m3Type_contref:
            *(uintptr_t*)مقدار = (uintptr_t)بیتها;
            اكسر;
        افتراضي:
            أرجع "unknown argument type";
        }
    }

    أرجع m3Err_none;
}

#وإلا

M3_نتیجه m3_ThrowException (IM3Runtime io_runtime, IM3_برچسب i_برچسب, uint32_t i_argc, ثابت فراغ* i_argptrs[])
{
    أرجع m3_Err_برچسب_Lookup_Failed;
}

IM3_برچسب m3_Get_Exception_برچسب (IM3Runtime i_runtime)
{
    أرجع عدم;
}

M3_نتیجه m3_Get_Exception_آرگومانها (IM3Runtime i_runtime, uint32_t i_argc, ثابت فراغ* o_argptrs[])
{
    أرجع "no uncaught exception";
}

#پایان_اگر // d_m3HasExceptionHandling


#إذا d_m3_Guarded_حافظه

// What RunCodeProtected hands to the platform layer to run, and what comes back
تعريفنوع بنية M3_Protected_پیاپی {
    pc_t            pc;
    IM3Runtime      runtime;
    M3_حافظه_سرآیند* mem;
    M3_نتیجه        نتیجه;
} M3_Protected_پیاپی;

ساكن
فراغ پیاپی_کد_Body (فراغ* io_context)
{
    M3_Protected_پیاپی* پیاپی = (M3_Protected_پیاپی*)io_context;

#  إذا (d_m3EnableOpProfiling || d_m3EnableOpTracing)
    پیاپی->نتیجه = (M3_نتیجه)پیاپی_کد(پیاپی->pc, (m3stack_t)پیاپی->runtime->پشته, پیاپی->mem,
                                    d_m3_Op_Default_آرگومانها, d_m3_پایه_Cstr);
#  وإلا
    پیاپی->نتیجه = (M3_نتیجه)پیاپی_کد(پیاپی->pc, (m3stack_t)پیاپی->runtime->پشته, پیاپی->mem,
                                    d_m3_Op_Default_آرگومانها);
#  endif
}

// The bounds check the load and store operations no longer do, done once here by the
// system instead: an address past the end of a memory reaches the uncommitted part
// of that memory's reservation, and the fault comes back as the trap the spec asks
// for. Nothing is left half-done by it - the Wasm stack and the memories are heap,
// and the caller unwinds the rest.
ساكن
M3_نتیجه پیاپی_کد_Protected (IM3Runtime i_runtime, pc_t i_pc, M3_حافظه_سرآیند* i_mem)
{
    // Memory operations in this build carry no bounds check, so code only runs where a
    // fault past the end of a memory can still be turned into a trap
    إذا (M3_UNLIKELY(not m3_Host_Guards_فعال())) {
        أرجع ErrorRuntime(m3_Err_حافظه_Guards_Unavailable, i_runtime,
                            "another part of the process holds the fault signal");
    }

    M3_Protected_پیاپی پیاپی;
    پیاپی.pc      = i_pc;
    پیاپی.runtime = i_runtime;
    پیاپی.mem     = i_mem;
    پیاپی.نتیجه  = m3Err_none;

    فراغ*  arena;
    size_t arena_بایتها;
    Guard_ArenaRange(&arena, &arena_بایتها);

    إذا (not m3_HostProtectedCall(پیاپی_کد_Body, &پیاپی, arena, arena_بایتها)) {
        أرجع m3_Err_trap_بیرون_از_Bounds_حافظه_Access;
    }

    أرجع پیاپی.نتیجه;
}

#پایان_اگر // d_m3GuardedMemory


#إذا d_m3_Has_پشته_Switching

// The function a suspended continuation stopped in: the callee of its innermost
// call frame, or the function it was entered with if it made no call. Frames are
// recorded innermost first, so that is the lowest-numbered call frame.
ساكن
IM3Function Continuation_InnermostFunction (IM3Continuation i_cont)
{
    لكل (u32 i = 0; i < i_cont->رقم_قابها; ++i) {
        إذا (i_cont->قابها[i].نوع == قاب_call && i_cont->قابها[i].call.function) {
            أرجع i_cont->قابها[i].call.function;
        }
    }

    أرجع i_cont->مدخل_Function;
}


// Where the host's next call puts its arguments and finds its results. A
// suspended root keeps its frames on the bottom of the runtime's stack, so a call
// made before it is resumed starts above the last of them rather than on top of
// them. The top is the innermost function's frame base plus everything that
// function can use. That base is the root's own sp, unless it stopped inside a
// resume: then the suspension point belongs to the resumed continuation, and the
// root's innermost frame is that resume.
فراغ Runtime_قراربده_Call_پشته (IM3Runtime io_runtime)
{
    IM3Continuation ریشه = io_runtime->ریشه_Continuation;

    io_runtime->پشته = io_runtime->origin_پشته;

    إذا (not ریشه or ریشه->state != cont_suspended) {
        أرجع;
    }

    m3stack_t   sp       = ریشه->sp;
    IM3Function function = Continuation_InnermostFunction(ریشه);

    إذا (ریشه->رقم_قابها and ریشه->قابها[0].نوع == قاب_resume) {
        sp = ریشه->قابها[0].sp;
    }

    m3slot_t* پایه = (m3slot_t*)io_runtime->origin_پشته;
    m3slot_t* بالاترین  = (m3slot_t*)sp + (function ? function->max_پشته_Slots : 0);

    إذا (بالاترین > پایه + io_runtime->رقم_پشته_Slots) {
        بالاترین = پایه + io_runtime->رقم_پشته_Slots;
    }

    io_runtime->پشته = بالاترین;
}

#پایان_اگر // d_m3HasStackSwitching


// Run compiled code on the runtime's stack, bounding native recursion for the
// duration of the call. The outermost invocation establishes the stack limit;
// nested ones (an imported function calling back into Wasm) inherit it.
ساكن مضمن
M3_نتیجه پیاپی_کد_Checked (IM3Runtime i_runtime, IM3Function i_function)
{
    pc_t i_pc = i_function->compiled;

    // execution runs against the memory of the module the entry point belongs
    // to, not against some runtime-wide one
    M3_حافظه_سرآیند* _mem = Module_حافظه_سرآیند(i_function->module);

#إذا d_m3HasSnapshots
    إذا (M3_UNLIKELY(i_function->module->isUnusable)) {
        أرجع "a snapshot failed to restore into the module, so it cannot run";
    }
    i_function->module->has_پیاپی = true;
#پایان_اگر

    d_m3_پشته_حد_Enter(i_runtime);

    // a host function calling back into Wasm does not change which call a trap
    // ends: that is still the one the host made first
    إذا (i_runtime->callNesting++ == 0) {
        i_runtime->entered = i_function;
    }
#إذا d_m3HasExceptionHandling
    // handler stacks don't nest across a call boundary: a host function calling
    // back into Wasm cannot be caught by a try_table its own caller entered
    u32 saved_Try_عمق   = i_runtime->try_عمق;
    i_runtime->try_عمق = 0;
#پایان_اگر
#إذا d_m3_Has_پشته_Switching
    // nor do continuations: the native frames on the far side of a host call
    // are not ours to record, so a suspend in here has no handler to reach and
    // no resume to unwind to
    IM3Continuation savedContinuation = i_runtime->فعال_Continuation;

    // A suspended root continuation is a paused program waiting to be resumed,
    // not scratch space. A call the host makes before resuming it gets a
    // context of its own, the way any other re-entry does, rather than having
    // the frames of the suspension reset out from under it.
    bool claim_ریشه = i_runtime->isSuspendable and not savedContinuation and
                     not (i_runtime->ریشه_Continuation and
                          i_runtime->ریشه_Continuation->state == cont_suspended);

    إذا (claim_ریشه and not i_runtime->ریشه_Continuation) {
        i_runtime->ریشه_Continuation = Continuation_New(i_runtime, i_function->funcType, i_function);

        إذا (i_runtime->ریشه_Continuation) {
            // The root borrows the runtime's stack and is not charged as cont.new.
            i_runtime->ریشه_Continuation->رقم_پشته_Slots = i_runtime->رقم_پشته_Slots;
        } وإلا {
            claim_ریشه = false;
        }
    }

    إذا (claim_ریشه) {
        i_runtime->فعال_Continuation              = i_runtime->ریشه_Continuation;
        i_runtime->ریشه_Continuation->مدخل_Function = i_function;
        i_runtime->ریشه_Continuation->state         = cont_running;
        i_runtime->ریشه_Continuation->رقم_قابها     = 0;
    } وإلا {
        i_runtime->فعال_Continuation = عدم;
    }
#پایان_اگر
#إذا d_m3_Guarded_حافظه
    M3_نتیجه نتیجه = پیاپی_کد_Protected(i_runtime, i_pc, _mem);
#وإلاإذا (d_m3EnableOpProfiling || d_m3EnableOpTracing)
    M3_نتیجه نتیجه = (M3_نتیجه)پیاپی_کد(i_pc, (m3stack_t)i_runtime->پشته, _mem, d_m3_Op_Default_آرگومانها, d_m3_پایه_Cstr);
#وإلا
    M3_نتیجه نتیجه = (M3_نتیجه)پیاپی_کد(i_pc, (m3stack_t)i_runtime->پشته, _mem, d_m3_Op_Default_آرگومانها);
#پایان_اگر
#إذا d_m3_Has_پشته_Switching
    إذا (claim_ریشه) {
        i_runtime->ریشه_Continuation->state =
          (نتیجه == m3Err_continuationSuspended) ? cont_suspended : cont_returned;

        Runtime_قراربده_Call_پشته(i_runtime);
    }
    i_runtime->فعال_Continuation = savedContinuation;
#پایان_اگر
#إذا d_m3HasExceptionHandling
    i_runtime->try_عمق = saved_Try_عمق;

    // an exception that reached the bottom of the call stack found no handler
    إذا (M3_UNLIKELY(نتیجه == m3Err_pendingException)) {
        نتیجه = m3Err_trapUncaughtException;
    }
    رکورد_Uncaught_Exception(i_runtime, نتیجه);

    // a paused invocation can still reach the exceptions it holds, so they wait
    // for it to finish
    إذا (i_runtime->callNesting == 1 and not m3_IsSuspended(i_runtime)) {
        FreeExceptions(i_runtime);
    }
#پایان_اگر
    i_runtime->callNesting--;
    d_m3_پشته_حد_Leave(i_runtime);

    أرجع نتیجه;
}

M3_نتیجه m3_پیاپی_شروع (IM3Module io_module)
{
#اگر_تعریف_شده FUZZING_BUILD_نما_UNSAFE_FOR_PRODUCTION
    // Execution disabled for fuzzing builds
    أرجع m3Err_none;
#پایان_اگر

    M3_نتیجه نتیجه           = m3Err_none;
    i32      شروع_Function_Tmp = -1;

    إذا (io_module and io_module->شروع_Function >= 0) {
        IM3Function function = &io_module->functions[io_module->شروع_Function];

        إذا (not function->compiled) {
_           (CompileFunction(function));
        }

        IM3FuncType ftype = function->funcType;
        إذا (ftype->رقم_آرگومانها != 0 || ftype->رقم_Rets != 0) {
            _throw(m3_Err_آرگومان_شمار_Mismatch);
        }

        IM3Module  module  = function->module;
        IM3Runtime runtime = module->runtime;

        شروع_Function_Tmp = io_module->شروع_Function;

        io_module->شروع_Function = -1;

        نتیجه = پیاپی_کد_Checked(runtime, function);

        // a start function that suspended is not to be started again: resuming
        // is what finishes it
        إذا (نتیجه and نتیجه != m3Err_continuationSuspended) {
            io_module->شروع_Function = شروع_Function_Tmp;
            EXCEPTION_PRINT(نتیجه);
            اذهب _catch;
        }
    }

    _catch: أرجع نتیجه;
}

// TODO: deal with main + side-modules loading efforcement
M3_نتیجه m3_بارگیری_Module (IM3Runtime io_runtime, IM3Module io_module)
{
    M3_نتیجه نتیجه = m3Err_none;

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
_   (Init_داده_Segments(io_module));

    // Start func might use imported functions, which are not liked here yet,
    // so it will be called before a function call is attempted (in m3_FindFunction)

#اگر_تعریف_شده DEBUG
    Module_Generate_نامها(io_module);
#پایان_اگر

    io_module->بعدی     = io_runtime->modules;
    io_runtime->modules = io_module;
    أرجع نتیجه; // ok

_catch:
    // The runtime owns the module either way. Instantiation may already have
    // written this module's functions into a table another module owns, and the
    // spec keeps whatever it managed to do before the trap, so those entries
    // stay callable - retaining the module is what stops them dangling.
    //
    // It goes on the tail of the list rather than the head: it is not a module
    // anyone should find by name, only one whose functions may still be
    // reachable through someone else's table.
    io_module->بعدی = عدم;

    IM3Module* دم = &io_runtime->modules;
    طالما (*دم) {
        دم = &(*دم)->بعدی;
    }
    *دم = io_module;

    أرجع نتیجه;
}

IM3Global m3_پیدا_کن_Global (IM3Module         io_module,
                         ثابت حرف* ثابت i_globalName)
{
    // Search exports
    IM3Global exported = Module_پیدا_کن_Exported_Global(io_module, i_globalName);
    إذا (exported) {
        // a re-exported global is the one that was imported, so reads and
        // writes have to reach the cell that actually holds the value
        أرجع exported->resolved ? exported->resolved : exported;
    }

    // Search imports
    لكل (u32 i = 0; i < io_module->رقم_Globals; ++i) {
        IM3Global g = &io_module->globals[i];

        إذا (g->import.moduleUtf8 and g->import.فیلد_Utf8) {
            إذا (strcmp(g->import.فیلد_Utf8, i_globalName) == 0) {
                أرجع g->resolved ? g->resolved : g;
            }
        }
    }
    أرجع عدم;
}

M3_نتیجه m3_GetGlobal (IM3Global      i_global,
                       IM3_Tagged_مقدار o_مقدار)
{
    إذا (not i_global) {
        أرجع m3Err_globalLookupFailed;
    }

    بدل (i_global->type) {
    حالة c_m3Type_i32: o_مقدار->مقدار.i32 = i_global->i32_مقدار; اكسر;
    حالة c_m3Type_i64: o_مقدار->مقدار.i64 = i_global->i64_مقدار; اكسر;
#إذا d_m3HasFloat
    حالة c_m3Type_f32: o_مقدار->مقدار.f32 = i_global->f32_مقدار; اكسر;
    حالة c_m3Type_f64: o_مقدار->مقدار.f64 = i_global->f64_مقدار; اكسر;
#پایان_اگر
    افتراضي: أرجع m3Err_invalidTypeId;
    }

    o_مقدار->type = (M3_مقدار_Type)(i_global->type);
    أرجع m3Err_none;
}

M3_نتیجه m3_مجموعه_Global (IM3Global            i_global,
                       ثابت IM3_Tagged_مقدار i_مقدار)
{
    إذا (not i_global) {
        أرجع m3Err_globalLookupFailed;
    }
    إذا (not i_global->isMutable) {
        أرجع m3Err_globalNotMutable;
    }
    إذا (i_global->type != i_مقدار->type) {
        أرجع m3Err_globalTypeMismatch;
    }

    بدل (i_مقدار->type) {
    حالة c_m3Type_i32: i_global->i32_مقدار = i_مقدار->مقدار.i32; اكسر;
    حالة c_m3Type_i64: i_global->i64_مقدار = i_مقدار->مقدار.i64; اكسر;
#إذا d_m3HasFloat
    حالة c_m3Type_f32: i_global->f32_مقدار = i_مقدار->مقدار.f32; اكسر;
    حالة c_m3Type_f64: i_global->f64_مقدار = i_مقدار->مقدار.f64; اكسر;
#پایان_اگر
    افتراضي: أرجع m3Err_invalidTypeId;
    }

    أرجع m3Err_none;
}

M3_مقدار_Type m3_GetGlobalType (IM3Global i_global)
{
    أرجع (i_global) ? (M3_مقدار_Type)(i_global->type) : c_m3Type_none;
}


فراغ* v_پیدا_کن_Function (IM3Module i_module, فراغ* i_info)
{
    ثابت حرف* ثابت i_name = (ثابت حرف*)i_info;

    // Prefer exported functions
    IM3Function exported = Module_پیدا_کن_Exported_Function(i_module, i_name);
    إذا (exported) {
        أرجع exported;
    }

    // Search internal functions
    لكل (u32 i = 0; i < i_module->رقم_Functions; ++i) {
        IM3Function f = &i_module->functions[i];

        bool isImported = f->import.moduleUtf8 or f->import.فیلد_Utf8;

        إذا (isImported) {
            استمر;
        }

        لكل (صحيح j = 0; j < f->رقم_نامها; j++) {
            إذا (f->نامها[j] and strcmp(f->نامها[j], i_name) == 0) {
                أرجع f;
            }
        }
    }

    أرجع عدم;
}


// Shared tail of the two lookups: a function is only usable once it has code.
ساكن
M3_نتیجه Prepare_یافت_Function (IM3Function* o_function, IM3Function i_function)
{
    M3_نتیجه نتیجه = m3Err_none;

    إذا (not i_function->compiled) {
_       (CompileFunction(i_function))
    }

    _catch:
    *o_function = نتیجه ? عدم : i_function;

    أرجع نتیجه;
}


// Searches every module in the runtime, most recently loaded first. That is a
// guess once more than one module is loaded and two of them export the same
// name - m3_FindFunctionIn says which module is meant.
M3_نتیجه m3_پیدا_کن_Function (IM3Function* o_function, IM3Runtime i_runtime, ثابت حرف* ثابت i_functionName)
{
                                                                d_m3Assert (o_function and i_runtime and i_functionName);
    IM3Function function = عدم;

    إذا (not i_runtime->modules) {
        *o_function = عدم;
        أرجع "no modules loaded";
    }

    function = (IM3Function)ForEachModule(i_runtime, v_پیدا_کن_Function, (فراغ*)i_functionName);

    إذا (not function) {
        *o_function = عدم;
        أرجع ErrorModule(m3Err_functionLookupFailed, i_runtime->modules, "'%s'", i_functionName);
    }

    أرجع Prepare_یافت_Function(o_function, function);
}


IM3Module m3_پیدا_کن_Module (IM3Runtime i_runtime, ثابت حرف* ثابت i_moduleName)
{
    إذا (not i_runtime or not i_moduleName) {
        أرجع عدم;
    }

    // the list is newest-first, so a name registered twice names the newer one
    لكل (IM3Module m = i_runtime->modules; m; m = m->بعدی) {
        إذا (m->name and strcmp(m->name, i_moduleName) == 0) {
            أرجع m;
        }
    }

    أرجع عدم;
}


// Searches one module's exports, which is what naming a module means.
M3_نتیجه m3_پیدا_کن_Function_درون (IM3Function* o_function, IM3Module i_module, ثابت حرف* ثابت i_functionName)
{
                                                                d_m3Assert (o_function and i_functionName);
    إذا (not i_module) {
        *o_function = عدم;
        أرجع m3Err_functionLookupFailed;      // ErrorModule would deref it
    }

    IM3Function function = (IM3Function)v_پیدا_کن_Function(i_module, (فراغ*)i_functionName);

    إذا (not function) {
        *o_function = عدم;
        أرجع ErrorModule(m3Err_functionLookupFailed, i_module, "'%s'", i_functionName);
    }

    أرجع Prepare_یافت_Function(o_function, function);
}


M3_نتیجه m3_Get_جدول_Function (IM3Function* o_function, IM3Module i_module, uint32_t i_شاخص)
{
_try {
    M3_جدول*    جدول_2;
    IM3Function function;

    _throwif("no table", i_module->رقم_Tables == 0);

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
    أرجع نتیجه;
}


ساكن
M3_نتیجه check_شروع_Function (IM3Module i_module)
{
    M3_نتیجه نتیجه = m3Err_none;                               d_m3Assert(i_module);

    // Check if start function needs to be called
    إذا (i_module->شروع_Function >= 0) {
        نتیجه = m3_پیاپی_شروع(i_module);
    }

    أرجع نتیجه;
}

uint32_t m3_Get_Arg_شمار (IM3Function i_function)
{
    إذا (i_function) {
        IM3FuncType ft = i_function->funcType;
        إذا (ft) {
            أرجع ft->رقم_آرگومانها;
        }
    }
    أرجع 0;
}

uint32_t m3_Get_Ret_شمار (IM3Function i_function)
{
    إذا (i_function) {
        IM3FuncType ft = i_function->funcType;
        إذا (ft) {
            أرجع ft->رقم_Rets;
        }
    }
    أرجع 0;
}


M3_مقدار_Type m3_GetArgType (IM3Function i_function, uint32_t index)
{
    إذا (i_function) {
        IM3FuncType ft = i_function->funcType;
        إذا (ft and index < ft->رقم_آرگومانها) {
            أرجع (M3_مقدار_Type)پایه_Type_از(d_FuncArgType(ft, index));
        }
    }
    أرجع c_m3Type_none;
}

M3_مقدار_Type m3_GetRetType (IM3Function i_function, uint32_t index)
{
    إذا (i_function) {
        IM3FuncType ft = i_function->funcType;
        إذا (ft and index < ft->رقم_Rets) {
            أرجع (M3_مقدار_Type)پایه_Type_از(d_FuncRetType(ft, index));
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
M3_نتیجه Get_پشته_اشاره_گر_For_آرگومانها (IM3Function i_function, u8** o_پشته)
{
    IM3Runtime  runtime = i_function->module->runtime;
    IM3FuncType ftype   = i_function->funcType;

    // the stack a paused invocation holds is not available to this one
    size_t استفاده_شده      = (size_t)((u8*)runtime->پشته - (u8*)runtime->origin_پشته);
    size_t needed    = ((size_t)ftype->رقم_Rets + ftype->رقم_آرگومانها) * حجم(u64);
    size_t موجود = (size_t)runtime->رقم_پشته_Slots * حجم(m3slot_t) - استفاده_شده;

    إذا (needed > موجود) {
        أرجع m3_Err_trap_پشته_سرریز;
    }

    u64* پشته = (u64*)runtime->پشته;

    پشته += ftype->رقم_Rets;

    *o_پشته = (u8*)پشته;

    أرجع m3Err_none;
}


M3_نتیجه m3_CallV (IM3Function i_function, ...)
{
    va_list ap;
    va_start(ap, i_function);
    M3_نتیجه r = m3_CallVL(i_function, ap);
    va_end(ap);
    أرجع r;
}

ساكن
فراغ Report_Native_پشته_Usage ()
{
#إذا d_m3_Log_Native_پشته
    صحيح پشته_استفاده_شده = m3_پشته_Get_Max();
    fprintf(stderr, "Native stack used: %d\n", پشته_استفاده_شده);
#پایان_اگر
}


M3_نتیجه m3_CallVL (IM3Function i_function, va_list i_آرگومانها)
{
    IM3Runtime  runtime = i_function->module->runtime;
    IM3FuncType ftype   = i_function->funcType;
    M3_نتیجه    نتیجه  = m3Err_none;
    u8*         s       = عدم;

    إذا (!i_function->compiled) {
        أرجع m3_Err_missing_Compiled_کد;
    }

#إذا d_m3_رکورد_Backtraces
    ClearBacktrace(runtime);
#پایان_اگر

    m3_پشته_Check_Init();

_   (check_شروع_Function(i_function->module))

_   (Get_پشته_اشاره_گر_For_آرگومانها(i_function, &s))

    لكل (u32 i = 0; i < ftype->رقم_آرگومانها; ++i) {
        بدل (d_FuncArgType(ftype, i)) {
        حالة c_m3Type_i32:
            *(i32*)(s) = va_arg(i_آرگومانها, i32);
            s += 8;
            اكسر;
        حالة c_m3Type_i64:
            *(i64*)(s) = va_arg(i_آرگومانها, i64);
            s += 8;
            اكسر;
        حالة c_m3Type_funcref:
        حالة c_m3Type_externref:
        حالة c_m3Type_exnref:
            *(uintptr_t*)(s) = va_arg(i_آرگومانها, uintptr_t);
            s += 8;
            اكسر;
#إذا d_m3HasFloat
        حالة c_m3Type_f32:
            *(f32*)(s) = (f32)va_arg(i_آرگومانها, f64);
            s += 8;
            اكسر; // f32 is passed as f64
        حالة c_m3Type_f64:
            *(f64*)(s) = va_arg(i_آرگومانها, f64);
            s += 8;
            اكسر;
#پایان_اگر
        افتراضي: أرجع "unknown argument type";
        }
    }

    نتیجه = پیاپی_کد_Checked(runtime, i_function);
    Report_Native_پشته_Usage();

    runtime->آخرین_Called = نتیجه ? عدم : i_function;

    _catch: أرجع نتیجه;
}

M3_نتیجه m3_Call (IM3Function i_function, uint32_t i_argc, ثابت فراغ* i_argptrs[])
{
    IM3Runtime  runtime = i_function->module->runtime;
    IM3FuncType ftype   = i_function->funcType;
    M3_نتیجه    نتیجه  = m3Err_none;
    u8*         s       = عدم;

    إذا (i_argc != ftype->رقم_آرگومانها) {
        أرجع m3_Err_آرگومان_شمار_Mismatch;
    }
    إذا (!i_function->compiled) {
        أرجع m3_Err_missing_Compiled_کد;
    }

#إذا d_m3_رکورد_Backtraces
    ClearBacktrace(runtime);
#پایان_اگر

    m3_پشته_Check_Init();

_   (check_شروع_Function(i_function->module))

_   (Get_پشته_اشاره_گر_For_آرگومانها(i_function, &s))

    لكل (u32 i = 0; i < ftype->رقم_آرگومانها; ++i) {
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
#پایان_اگر
        افتراضي: أرجع "unknown argument type";
        }
    }

    نتیجه = پیاپی_کد_Checked(runtime, i_function);

    Report_Native_پشته_Usage();

    runtime->آخرین_Called = نتیجه ? عدم : i_function;

    _catch: أرجع نتیجه;
}

// Argument parsing for m3_CallArgv. Strict on purpose: the whole string has to
// be consumed, so "12abc" is rejected rather than read as 12, and an empty or
// unparsable argument is an error rather than the zero that strtoul with a
// NULL end pointer used to hand back. See wasm3/wasm3#367.
ساكن
M3_نتیجه ParseArgInteger (ccstr_t i_arg, u32 i_رقم_بیتها, u64* o_مقدار)
{
    إذا (not i_arg or not *i_arg) {
        أرجع "empty argument";
    }

    // strtoull would skip leading space, but trailing space is rejected below;
    // accepting one and not the other would just be confusing
    إذا (آیا_فاصله((بیعلامت حرف)*i_arg)) {
        أرجع "argument is not a number";
    }

    حرف* انتها = عدم;
    u64   مقدار;

    errno = 0;

    // an argument may be spelled signed or unsigned: -1 and 4294967295 name the
    // same i32
    إذا (*i_arg == '-') {
        i64 signed_مقدار = strtoll(i_arg, &انتها, 10);

        إذا (i_رقم_بیتها == 32 and (signed_مقدار < INT32_MIN or signed_مقدار > INT32_MAX)) {
            أرجع "argument out of range";
        }

        مقدار = (u64)signed_مقدار;
    } وإلا {
        مقدار = strtoull(i_arg, &انتها, 10);

        إذا (i_رقم_بیتها == 32 and مقدار > UINT32_MAX) {
            أرجع "argument out of range";
        }
    }

    إذا (errno == ERANGE) {
        أرجع "argument out of range";
    }

    إذا (انتها == i_arg or *انتها) {
        أرجع "argument is not a number";
    }

    *o_مقدار = مقدار;

    أرجع m3Err_none;
}


#إذا d_m3HasFloat
ساكن
M3_نتیجه ParseArgFloat (ccstr_t i_arg, f64* o_مقدار)
{
    إذا (not i_arg or not *i_arg) {
        أرجع "empty argument";
    }

    إذا (آیا_فاصله((بیعلامت حرف)*i_arg)) {
        أرجع "argument is not a number";
    }

    حرف* انتها = عدم;

    errno = 0;

    f64 مقدار = strtod(i_arg, &انتها);

    إذا (انتها == i_arg or *انتها) {
        أرجع "argument is not a number";
    }

    // strtod reports underflow through ERANGE as well, and a denormal result is
    // perfectly usable, so only an overflow to infinity is out of range
    إذا (errno == ERANGE and (مقدار > DBL_MAX or مقدار < -DBL_MAX)) {
        أرجع "argument out of range";
    }

    *o_مقدار = مقدار;

    أرجع m3Err_none;
}
#پایان_اگر


// A reference argument is either the null reference or a host handle written as
// an integer.
ساكن
M3_نتیجه ParseArgReference (ccstr_t i_arg, u64* o_مقدار)
{
    إذا (i_arg and strcmp(i_arg, "null") == 0) {
        *o_مقدار = 0;
        أرجع m3Err_none;
    }

    أرجع ParseArgInteger(i_arg, 64, o_مقدار);
}


M3_نتیجه m3_CallArgv (IM3Function i_function, uint32_t i_argc, ثابت حرف* i_argv[])
{
    IM3FuncType ftype   = i_function->funcType;
    IM3Runtime  runtime = i_function->module->runtime;
    M3_نتیجه    نتیجه  = m3Err_none;
    u8*         s       = عدم;

    إذا (i_argc != ftype->رقم_آرگومانها) {
        أرجع m3_Err_آرگومان_شمار_Mismatch;
    }
    إذا (!i_function->compiled) {
        أرجع m3_Err_missing_Compiled_کد;
    }

#إذا d_m3_رکورد_Backtraces
    ClearBacktrace(runtime);
#پایان_اگر

    m3_پشته_Check_Init();

_   (check_شروع_Function(i_function->module))

_   (Get_پشته_اشاره_گر_For_آرگومانها(i_function, &s))

    لكل (u32 i = 0; i < ftype->رقم_آرگومانها; ++i) {
        u64 مقدار = 0;
#إذا d_m3HasFloat
        f64 fvalue = 0;
#پایان_اگر
        بدل (d_FuncArgType(ftype, i)) {
        حالة c_m3Type_i32:
_           (ParseArgInteger(i_argv[i], 32, &مقدار)) * (i32*)(s) = (i32)مقدار;
            s += 8;
            اكسر;
        حالة c_m3Type_i64:
_           (ParseArgInteger(i_argv[i], 64, &مقدار)) * (i64*)(s) = (i64)مقدار;
            s += 8;
            اكسر;
        حالة c_m3Type_funcref:
        حالة c_m3Type_externref:
        حالة c_m3Type_exnref:
_           (ParseArgReference(i_argv[i], &مقدار)) * (uintptr_t*)(s) = (uintptr_t)مقدار;
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
#پایان_اگر
        افتراضي: _throw("unknown argument type");
        }
    }

    نتیجه = پیاپی_کد_Checked(runtime, i_function);

    Report_Native_پشته_Usage();

    runtime->آخرین_Called = نتیجه ? عدم : i_function;

    _catch: أرجع نتیجه;
}


//u8 * AlignStackPointerTo64Bits (const u8 * i_stack)
//{
//    uintptr_t ptr = (uintptr_t) i_stack;
//    return (u8 *) ((ptr + 7) & ~7);
//}


M3_نتیجه m3_GetResults (IM3Function i_function, uint32_t i_retc, ثابت فراغ* o_retptrs[])
{
    IM3FuncType ftype   = i_function->funcType;
    IM3Runtime  runtime = i_function->module->runtime;

    إذا (i_retc != ftype->رقم_Rets) {
        أرجع m3_Err_آرگومان_شمار_Mismatch;
    }
    إذا (i_function != runtime->آخرین_Called) {
        أرجع "function not called";
    }

    u8* s = (u8*)runtime->پشته;

    لكل (u32 i = 0; i < ftype->رقم_Rets; ++i) {
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
#پایان_اگر
        افتراضي: أرجع "unknown return type";
        }
    }
    أرجع m3Err_none;
}

M3_نتیجه m3_GetResultsV (IM3Function i_function, ...)
{
    va_list ap;
    va_start(ap, i_function);
    M3_نتیجه r = m3_GetResultsVL(i_function, ap);
    va_end(ap);
    أرجع r;
}

M3_نتیجه m3_GetResultsVL (IM3Function i_function, va_list o_rets)
{
    IM3Runtime  runtime = i_function->module->runtime;
    IM3FuncType ftype   = i_function->funcType;

    إذا (i_function != runtime->آخرین_Called) {
        أرجع "function not called";
    }

    u8* s = (u8*)runtime->پشته;
    لكل (u32 i = 0; i < ftype->رقم_Rets; ++i) {
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
#پایان_اگر
        افتراضي: أرجع "unknown argument type";
        }
    }
    أرجع m3Err_none;
}

فراغ Release_کد_صفحه_No_Track (IM3Runtime i_runtime, IM3_کد_صفحه i_کد_صفحه)
{
    إذا (i_کد_صفحه) {
        IM3_کد_صفحه* list;

        bool صفحه_پر = (رقم_Free_سطرها(i_کد_صفحه) < d_m3_کد_صفحه_Free_سطرها_آستانه);
        إذا (صفحه_پر) {
            list = &i_runtime->صفحات_پر;
        } وإلا {
            list = &i_runtime->صفحات_Open;
        }

        افزودن_کد_صفحه(list, i_کد_صفحه);                         m3log (emit, "release page: %d to queue: '%s'", i_کد_صفحه->info.sequence, صفحه_پر ? "full" : "open")
    }
}


IM3_کد_صفحه Acquire_کد_صفحه_With_ظرفیت (IM3Runtime i_runtime, u32 i_min_سطر_شمار)
{
    IM3_کد_صفحه صفحه = Remove_کد_صفحه_از_ظرفیت(&i_runtime->صفحات_Open, i_min_سطر_شمار);

    إذا (not صفحه) {
        صفحه = Environment_Acquire_کد_صفحه(i_runtime->environment, i_min_سطر_شمار);

        إذا (not صفحه) {
            صفحه = New_کد_صفحه(i_runtime, i_min_سطر_شمار);
        }

        إذا (صفحه) {
            i_runtime->رقم_کد_صفحات++;
        }
    }

    إذا (صفحه) {                                                  m3log (emit, "acquire page: %d", صفحه->info.sequence);
        i_runtime->رقم_فعال_کد_صفحات++;
    }

    أرجع صفحه;
}


IM3_کد_صفحه Acquire_کد_صفحه (IM3Runtime i_runtime)
{
    أرجع Acquire_کد_صفحه_With_ظرفیت(i_runtime, d_m3_کد_صفحه_Free_سطرها_آستانه);
}


فراغ Release_کد_صفحه (IM3Runtime i_runtime, IM3_کد_صفحه i_کد_صفحه)
{
    إذا (i_کد_صفحه) {
        Release_کد_صفحه_No_Track(i_runtime, i_کد_صفحه);
        i_runtime->رقم_فعال_کد_صفحات--;

#إذا defined(DEBUG)
        u32 رقم_Open = شمار_کد_صفحات(i_runtime->صفحات_Open);
        u32 رقم_پر = شمار_کد_صفحات(i_runtime->صفحات_پر);

        m3log(runtime, "runtime: %p; open-pages: %d; full-pages: %d; active: %d; total: %d", i_runtime, رقم_Open, رقم_پر, i_runtime->رقم_فعال_کد_صفحات, i_runtime->رقم_کد_صفحات);

        d_m3Assert(رقم_Open + رقم_پر + i_runtime->رقم_فعال_کد_صفحات == i_runtime->رقم_کد_صفحات);

#  إذا d_m3_Log_کد_صفحات
        dump_کد_صفحه(i_کد_صفحه, /* startPC: */ عدم);
#  endif
#پایان_اگر
    }
}


#إذا d_m3VerboseErrorMessages
M3_نتیجه m3Error (M3_نتیجه i_نتیجه, IM3Runtime i_runtime, IM3Module i_module, IM3Function i_function,
                  ثابت حرف* ثابت i_file, u32 i_سطر_رقم, ثابت حرف* ثابت i_error_پیام, ...)
{
    إذا (i_runtime) {
        i_runtime->error         = (M3ErrorInfo){ .نتیجه = i_نتیجه, .runtime = i_runtime, .module = i_module, .function = i_function, .file = i_file, .سطر_2 = i_سطر_رقم };
        i_runtime->error.پیام = i_runtime->error_پیام;

        va_list آرگومانها;
        va_start(آرگومانها, i_error_پیام);
        vsnprintf(i_runtime->error_پیام, حجم(i_runtime->error_پیام), i_error_پیام, آرگومانها);
        va_end(آرگومانها);
    }

    أرجع i_نتیجه;
}
#پایان_اگر


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
        i_runtime->error.پیام = "";
    }
}

uint8_t* m3_Get_حافظه (IM3Module i_module, size_t* o_حافظه_اندازه_درون_بایتها, uint32_t i_حافظه_شاخص)
{
    uint8_t* حافظه = عدم;
    size_t   اندازه   = 0;

    إذا (i_module and i_حافظه_شاخص < i_module->رقم_Memories) {
        IM3_حافظه mem = i_module->memories[i_حافظه_شاخص];

        إذا (mem->mallocated) {
            اندازه = mem->mallocated->درازی;

            إذا (اندازه) {
                حافظه = m3_Mem_داده(mem->mallocated);
            }
        }
    }

    إذا (o_حافظه_اندازه_درون_بایتها) {
        *o_حافظه_اندازه_درون_بایتها = اندازه;
    }

    أرجع حافظه;
}


size_t m3_Get_حافظه_اندازه (IM3Module i_module, uint32_t i_حافظه_شاخص)
{
    إذا (not i_module or i_حافظه_شاخص >= i_module->رقم_Memories) {
        أرجع 0;
    }

    IM3_حافظه mem = i_module->memories[i_حافظه_شاخص];

    أرجع mem->mallocated ? mem->mallocated->درازی : 0;
}


size_t m3_Get_حافظه_اندازه_At (ثابت فراغ* i_حافظه)
{
    إذا (not i_حافظه) {
        أرجع 0;
    }

    // the header sits immediately before the data it describes
    ثابت M3_حافظه_سرآیند* سرآیند = ((ثابت M3_حافظه_سرآیند*)i_حافظه) - 1;

    أرجع سرآیند->درازی;
}


M3_نتیجه m3_پیدا_کن_Exported_حافظه (IM3Module i_module, cstr_t i_name, u32* o_حافظه_شاخص)
{
    إذا (not i_module or not i_name or not o_حافظه_شاخص) {
        أرجع m3_Err_unknown_حافظه;
    }

    ثابت M3Export* e = Module_پیدا_کن_Export(i_module, d_external_نوع_حافظه, i_name);

    إذا (not e) {
        أرجع m3_Err_unknown_حافظه;
    }

    *o_حافظه_شاخص = e->index;
    أرجع m3Err_none;
}


bool Module_HasLinkedHostImport (IM3Module i_module, cstr_t i_importModule)
{
    لكل (u32 i = 0; i < i_module->رقم_Functions; ++i) {
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


M3_نتیجه m3_Bind_Import_حافظه (IM3Module io_module, cstr_t i_importModule, u32 i_حافظه_شاخص)
{
    إذا (not io_module or not i_importModule) {
        أرجع m3Err_moduleNotLinked;
    }

    إذا (i_حافظه_شاخص >= io_module->رقم_Memories) {
        أرجع m3_Err_unknown_حافظه;
    }

    // the memory itself, not the index: linking may have pointed this slot at
    // another module's memory, and growing one reallocates behind it
    IM3_حافظه حافظه = io_module->memories[i_حافظه_شاخص];

    ثابت bool wildcardModule = (strcmp(i_importModule, "*") == 0);

    لكل (u32 i = 0; i < io_module->رقم_Functions; ++i) {
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
#إذا d_m3_رکورد_Backtraces
    أرجع &i_runtime->backtrace;
#وإلا
    أرجع عدم;
#پایان_اگر
}


//-------------------------------------------------------------------------------------------------------------------------------
//  snapshots & suspendable execution
//-------------------------------------------------------------------------------------------------------------------------------

فراغ m3_مجموعه_Suspendable (IM3Runtime io_runtime, bool i_suspendable)
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
#إذا d_m3_Has_پشته_Switching
    إذا (i_runtime->ریشه_Continuation && i_runtime->ریشه_Continuation->state == cont_suspended) {
        أرجع true;
    }
#پایان_اگر
    أرجع false;
}

M3_نتیجه m3_ResumeRuntime (IM3Runtime io_runtime)
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
    إذا (io_runtime->ریشه_Continuation and io_runtime->ریشه_Continuation->مدخل_Function and
        io_runtime->ریشه_Continuation->مدخل_Function->module->isUnusable) {
        أرجع "a snapshot failed to restore into the module, so it cannot run";
    }
#پایان_اگر
#إذا d_m3_Has_پشته_Switching
    إذا (!io_runtime->ریشه_Continuation || io_runtime->ریشه_Continuation->state != cont_suspended) {
        أرجع m3Err_none;
    }
    // what finishes returns into the bottom of the stack, where it was entered
    io_runtime->پشته = io_runtime->origin_پشته;

    // this is the outermost call again, as far as a host function calling back
    // into Wasm from inside it can tell
    io_runtime->entered = io_runtime->ریشه_Continuation->مدخل_Function;
    io_runtime->callNesting++;

    d_m3_پشته_حد_Enter(io_runtime);
    io_runtime->resumePastCheck = true;
    M3_نتیجه نتیجه             = (M3_نتیجه)ResumeContinuation(io_runtime, io_runtime->ریشه_Continuation);
    io_runtime->resumePastCheck = false;
    d_m3_پشته_حد_Leave(io_runtime);

#  إذا d_m3HasExceptionHandling
    إذا (M3_UNLIKELY(نتیجه == m3Err_pendingException)) {
        نتیجه = m3Err_trapUncaughtException;
    }
    رکورد_Uncaught_Exception(io_runtime, نتیجه);
#  endif

    io_runtime->callNesting--;

#  إذا d_m3HasExceptionHandling
    إذا (نتیجه != m3Err_continuationSuspended and io_runtime->callNesting == 0) {
        FreeExceptions(io_runtime);
    }
#  endif

    // the results are where the paused call left room for them, so a finished
    // resume can be asked for them the way the call itself could have been
    io_runtime->آخرین_Called = (نتیجه == m3Err_none) ? io_runtime->ریشه_Continuation->مدخل_Function : عدم;

    Runtime_قراربده_Call_پشته(io_runtime);
    أرجع نتیجه;
#وإلا
    أرجع m3Err_none;
#پایان_اگر
}
