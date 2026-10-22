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
            برائے (u8 t = c_m3Type_none; t < c_m3_Type_گنتی; t++) {
                اگر (t == c_m3Type_v128) {
                    جاری;
                }

                IM3FuncType ftype;
_               (AllocFuncType(&ftype, 1));

                ftype->عدد_دلائل  = 0;
                ftype->عدد_Rets  = (t == c_m3Type_none) ? 0 : 1;
                ftype->types[0] = t;

                Environment_جوڑو_Func_Type(env, &ftype);

                env->retFuncTypes[t] = ftype;
            }
        }

        _catch:
        اگر (نتیجہ) {
            m3_FreeEnvironment(env);
            env = تہی;
        }
    }

    واپس env;
}


خالی Environment_Release (IM3Environment i_environment)
{
    IM3FuncType ftype = i_environment->funcTypes;

    جب (ftype) {
        IM3FuncType اگلا = ftype->اگلا;
        m3_Free(ftype);
        ftype = اگلا;
    }

    m3log(runtime, "freeing %d pages from environment", گنتی_رمز_صفحات(i_environment->صفحات_Released));
    Free_رمز_صفحات(&i_environment->صفحات_Released);
}


خالی m3_FreeEnvironment (IM3Environment i_environment)
{
    اگر (i_environment) {
        Environment_Release(i_environment);
        m3_Free(i_environment);
    }
}


خالی m3_سیٹ_Custom_Section_Handler (IM3Environment i_environment, M3SectionHandler i_handler)
{
    اگر (i_environment) {
        i_environment->customSectionHandler = i_handler;
    }
}


// returns the same io_funcType or replaces it with an equivalent that's already in the type linked list
M3_نتیجہ Environment_جوڑو_Func_Type (IM3Environment i_environment, IM3FuncType* io_funcType)
{
    IM3FuncType جوڑو_Type = *io_funcType;
    IM3FuncType newType = i_environment->funcTypes;

    جب (newType) {
        اگر (not newType->اندر_Rec_گروہ and AreFuncTypesEqual(newType, جوڑو_Type)) {
            m3_Free(جوڑو_Type);
            توڑو;
        }

        newType = newType->اگلا;
    }

    اگر (newType == تہی) {
        // a type index has to fit in the heap type field of an m3type_t
        اگر (i_environment->عدد_Func_Types >= d_m3_Max_Sane_Types_گنتی) {
            m3_Free(جوڑو_Type);
            *io_funcType = تہی;
            واپس "too many distinct function types";
        }

        newType                  = جوڑو_Type;
        newType->canonical_اشاریہ  = i_environment->عدد_Func_Types++;
        newType->اگلا            = i_environment->funcTypes;
        i_environment->funcTypes = newType;
    }

    *io_funcType = newType;

    واپس m3Err_none;
}


// Adds a type without looking for an equal one first, and hands back the index
// it was given.
//
// Members of a recursive group do not get to share an entry with a type that
// merely looks like them: the group is their identity, so two groups spelled
// the same way are still two groups, and a type inside one is never the type
// outside it that matches field for field. Reserving the indices up front is
// also what lets the members refer to each other while they are being read.
M3_نتیجہ Environment_ReserveFuncTypes (IM3Environment i_environment, u32 i_گنتی, u16* o_پہلا_اشاریہ)
{
    اگر ((u64)i_environment->عدد_Func_Types + i_گنتی > d_m3_Max_Sane_Types_گنتی) {
        واپس "too many distinct function types";
    }

    *o_پہلا_اشاریہ = i_environment->عدد_Func_Types;
    i_environment->عدد_Func_Types += (u16)i_گنتی;

    واپس m3Err_none;
}


خالی Environment_AdoptFuncType (IM3Environment i_environment, IM3FuncType i_funcType, u16 i_canonical_اشاریہ)
{
    i_funcType->اندر_Rec_گروہ     = true;
    i_funcType->canonical_اشاریہ = i_canonical_اشاریہ;
    i_funcType->اگلا           = i_environment->funcTypes;
    i_environment->funcTypes   = i_funcType;
}


IM3_رمز_صفحہ Remove_رمز_صفحہ_کا_استعداد (M3_رمز_صفحہ** io_list, u32 i_minimum_سطر_گنتی)
{
    IM3_رمز_صفحہ پچھلا = تہی;
    IM3_رمز_صفحہ صفحہ = *io_list;

    جب (صفحہ) {
        اگر (عدد_Free_سطریں(صفحہ) >= i_minimum_سطر_گنتی) {             d_m3Assert (صفحہ->info.usage_گنتی == 0);
            IM3_رمز_صفحہ اگلا = صفحہ->info.اگلا;
            اگر (پچھلا) {
                پچھلا->info.اگلا = اگلا; // mid-list
            } ورنہ {
                *io_list = اگلا;       // front of list
            }

            توڑو;
        }

        پچھلا = صفحہ;
        صفحہ = صفحہ->info.اگلا;
    }

    واپس صفحہ;
}


IM3_رمز_صفحہ Environment_Acquire_رمز_صفحہ (IM3Environment i_environment, u32 i_minimum_سطر_گنتی)
{
    واپس Remove_رمز_صفحہ_کا_استعداد(&i_environment->صفحات_Released, i_minimum_سطر_گنتی);
}


خالی Environment_Release_رمز_صفحات (IM3Environment i_environment, IM3_رمز_صفحہ i_رمز_صفحہ_List)
{
    IM3_رمز_صفحہ انتہا = i_رمز_صفحہ_List;

    جب (انتہا) {
        انتہا->info.سطر_اشاریہ = 0; // reset page
#اگر d_m3_ریکارڈ_Backtraces
        انتہا->info.mapping->سائز = 0;
#اگر_ختم // d_m3RecordBacktraces

        IM3_رمز_صفحہ اگلا = انتہا->info.اگلا;
        اگر (not اگلا) {
            توڑو;
        }

        انتہا = اگلا;
    }

    اگر (انتہا) {
        // push list to front
        انتہا->info.اگلا               = i_environment->صفحات_Released;
        i_environment->صفحات_Released = i_رمز_صفحہ_List;
    }
}


IM3Runtime m3_NewRuntime (IM3Environment i_environment, u32 i_ڈھیر_سائز_اندر_بائٹیں, خالی* i_userdata)
{
    IM3Runtime runtime = m3_AllocStruct(M3Runtime);

    اگر (runtime) {
        m3_ResetErrorInfo(runtime);

        runtime->environment = i_environment;
        runtime->userdata    = i_userdata;

        Deterministic_InitRuntime(runtime);

        runtime->origin_ڈھیر = m3_Malloc("Wasm Stack", i_ڈھیر_سائز_اندر_بائٹیں + 4 * حجم(m3slot_t)); // TODO: more precise stack checks

        اگر (runtime->origin_ڈھیر) {
            runtime->ڈھیر         = runtime->origin_ڈھیر;
            runtime->عدد_ڈھیر_Slots = i_ڈھیر_سائز_اندر_بائٹیں / حجم(m3slot_t);          m3log (runtime, "new stack: %p, slots: %u", runtime->origin_ڈھیر, runtime->عدد_ڈھیر_Slots);
        } ورنہ {
            m3_Free(runtime);
        }
    }

    واپس runtime;
}

خالی m3_سیٹ_Validation (IM3Runtime i_runtime, bool i_enable)
{
#اگر d_m3EnableValidation
    اگر (i_runtime) {
        i_runtime->skipValidation = not i_enable;
    }
#ورنہ
    (خالی)i_runtime;
    (خالی)i_enable;              // nothing to skip: the validator was compiled out
#اگر_ختم
}

M3_نتیجہ m3_سیٹ_Resource_حد_بندی (IM3Runtime i_runtime, M3_Resource_حد_بندی i_حد_بندی, uint64_t i_قدر)
{
    اگر (not i_runtime) {
        واپس m3Err_mallocFailed;
    }
    انتخاب (i_حد_بندی) {
    حالت c_m3_حد_بندی_حافظہ_بائٹیں:
        اگر (i_قدر and i_قدر < i_runtime->حافظہ_بائٹیں_مستعمل) {
            واپس m3_Err_resource_حد_بندی_نیچے_Usage;
        }
        i_runtime->حافظہ_بائٹیں_حد_بندی = i_قدر;
        واپس m3Err_none;
    حالت c_m3_حد_بندی_جدول_Elements:
        اگر (i_قدر and i_قدر < i_runtime->جدول_Elements_مستعمل) {
            واپس m3_Err_resource_حد_بندی_نیچے_Usage;
        }
        i_runtime->جدول_Elements_حد_بندی = i_قدر;
        واپس m3Err_none;
#اگر d_m3_Has_ڈھیر_Switching
    حالت c_m3_حد_بندی_Continuations:
        اگر (i_قدر and i_قدر < i_runtime->continuationsAllocated) {
            واپس m3_Err_resource_حد_بندی_نیچے_Usage;
        }
        i_runtime->continuations_حد_بندی = (u32)M3_MIN(i_قدر, (u64)UINT32_MAX);
        جب (i_قدر and i_runtime->val_ڈھیر_Pool_گنتی > i_قدر - i_runtime->continuationsAllocated) {
            m3_Free(i_runtime->val_ڈھیر_Pool[--i_runtime->val_ڈھیر_Pool_گنتی]);
        }
        واپس m3Err_none;
#ورنہ
    حالت c_m3_حد_بندی_Continuations: واپس m3_Err_resource_حد_بندی_Not_Supported;
#اگر_ختم
    حالت c_m3_حد_بندی_Gas_Units:
#اگر d_m3HasGasMetering
        i_runtime->gas_حد_بندی = i_runtime->gas_بقایا = (i64)M3_MIN(i_قدر, (u64)INT64_MAX);
        واپس m3Err_none;
#ورنہ
        واپس m3_Err_resource_حد_بندی_Not_Supported;
#اگر_ختم
    طےشدہ: واپس m3_Err_unknown_Resource_حد_بندی;
    }
}

uint64_t m3_Get_Resource_حد_بندی (IM3Runtime i_runtime, M3_Resource_حد_بندی i_حد_بندی)
{
    اگر (not i_runtime) {
        واپس 0;
    }
    انتخاب (i_حد_بندی) {
    حالت c_m3_حد_بندی_حافظہ_بائٹیں: واپس i_runtime->حافظہ_بائٹیں_حد_بندی;
    حالت c_m3_حد_بندی_جدول_Elements: واپس i_runtime->جدول_Elements_حد_بندی;
#اگر d_m3_Has_ڈھیر_Switching
    حالت c_m3_حد_بندی_Continuations: واپس i_runtime->continuations_حد_بندی;
#اگر_ختم
#اگر d_m3HasGasMetering
    حالت c_m3_حد_بندی_Gas_Units: واپس (u64)i_runtime->gas_حد_بندی;
#اگر_ختم
    طےشدہ: واپس 0;
    }
}

uint64_t m3_GetResourceUsage (IM3Runtime i_runtime, M3_Resource_حد_بندی i_حد_بندی)
{
    اگر (not i_runtime) {
        واپس 0;
    }
    انتخاب (i_حد_بندی) {
    حالت c_m3_حد_بندی_حافظہ_بائٹیں: واپس i_runtime->حافظہ_بائٹیں_مستعمل;
    حالت c_m3_حد_بندی_جدول_Elements: واپس i_runtime->جدول_Elements_مستعمل;
#اگر d_m3_Has_ڈھیر_Switching
    حالت c_m3_حد_بندی_Continuations: واپس i_runtime->continuationsAllocated;
#اگر_ختم
#اگر d_m3HasGasMetering
    حالت c_m3_حد_بندی_Gas_Units: واپس (u64)i_runtime->gas_حد_بندی - (u64)i_runtime->gas_بقایا;
#اگر_ختم
    طےشدہ: واپس 0;
    }
}

خالی* m3_Get_User_ڈیٹا (IM3Runtime i_runtime)
{
    واپس i_runtime ? i_runtime->userdata : تہی;
}


خالی* ForEachModule (IM3Runtime i_runtime, ModuleVisitor i_visitor, خالی* i_info)
{
    خالی* r = تہی;

    IM3Module module = i_runtime->modules;

    جب (module) {
        IM3Module اگلا = module->اگلا;
        اگر ((r = i_visitor(module, i_info))) {
            توڑو;
        }

        module = اگلا;
    }

    واپس r;
}


خالی* _FreeModule (IM3Module i_module, خالی* i_info)
{
    m3_FreeModule(i_module);
    واپس تہی;
}


// A module's memory and table index spaces hold borrowed pointers: linking
// repoints an imported slot at the memory or table the exporting module owns.
// m3_FreeModule tells a borrowed slot from an owned one by reading that owner,
// which only holds while the object is still allocated - and the free walk
// below visits modules in load order, with no ordering between an owner and
// the modules borrowing from it. So clear the borrowed slots first, in a pass
// that frees nothing; the walk then skips them on the NULL check it already
// does, instead of reading a memory some earlier module has freed.
خالی* _ReleaseBorrowedSlots (IM3Module i_module, خالی* i_info)
{
    برائے (u32 i = 0; i < i_module->عدد_Memories; ++i) {
        IM3_حافظہ حافظہ = i_module->memories[i];

        اگر (حافظہ and حافظہ->owner != i_module) {
            i_module->memories[i] = تہی;
        }
    }

    برائے (u32 i = 0; i < i_module->عدد_Tables; ++i) {
        IM3_جدول جدول_2 = i_module->tables[i];

        اگر (جدول_2 and جدول_2->owner != i_module) {
            i_module->tables[i] = تہی;
        }
    }

    واپس تہی;
}


خالی Runtime_Release (IM3Runtime i_runtime)
{
    ForEachModule(i_runtime, _ReleaseBorrowedSlots, تہی);
    ForEachModule(i_runtime, _FreeModule, تہی);                    d_m3Assert (i_runtime->عدد_فعال_رمز_صفحات == 0);

    Environment_Release_رمز_صفحات(i_runtime->environment, i_runtime->صفحات_Open);
    Environment_Release_رمز_صفحات(i_runtime->environment, i_runtime->صفحات_بھرا);

    m3_Free(i_runtime->origin_ڈھیر);

#اگر d_m3EnableValidation
    m3_Free(i_runtime->validator);
#اگر_ختم

#اگر d_m3HasExceptionHandling
    // a runtime let go of while suspended still holds its exceptions
    FreeExceptions(i_runtime);
    m3_Free(i_runtime->uncaughtException);
#اگر_ختم
#اگر d_m3HasExceptionHandling || d_m3_Has_ڈھیر_Switching
    جب (i_runtime->host_لیبل_فہرست) {
        M3_Host_لیبل* اگلا = i_runtime->host_لیبل_فہرست->اگلا;
        m3_Free(i_runtime->host_لیبل_فہرست);
        i_runtime->host_لیبل_فہرست = اگلا;
    }
#اگر_ختم
#اگر d_m3_Has_ڈھیر_Switching
    Continuation_ReleaseAll(i_runtime);
#اگر_ختم
}


خالی m3_FreeRuntime (IM3Runtime i_runtime)
{
    اگر (i_runtime) {
        m3_PrintProfilerInfo();

        Runtime_Release(i_runtime);
        m3_Free(i_runtime);
    }
}


#اگر d_m3_Has_ڈھیر_Switching

IM3Continuation Continuation_New (IM3Runtime i_runtime, IM3FuncType i_type, IM3Function i_function)
{
    IM3Continuation cont = m3_AllocStruct(M3Continuation);
    اگر (!cont) {
        واپس تہی;
    }

    cont->type          = i_type;
    cont->اندراج_Function = i_function;
    cont->state         = cont_allocated;

    cont->عدد_ڈھیر_Slots = d_m3_Continuation_ڈھیر_Slots;
    // frames are recorded only when this continuation actually suspends, and
    // only as deep as it got, so the array starts out unallocated
    cont->چوکھٹے    = تہی;
    cont->چوکھٹے_Cap = 0;
    cont->عدد_چوکھٹے = 0;

    cont->sp = cont->val_ڈھیر;
    cont->pc = i_function ? i_function->compiled : تہی;
    cont->r0 = 0;
#  اگر d_m3HasFloat
    cont->fp0 = 0;
#  endif
    cont->bound_دلائل_گنتی    = 0;
    cont->عدد_Suspend_Results = 0;
    cont->عدد_Handlers       = 0;
    cont->handlersPC        = تہی;
    cont->والد            = تہی;

    cont->اگلا               = i_runtime->continuations;
    i_runtime->continuations = cont;

    واپس cont;
}

IM3Continuation Continuation_ForkSuspended (IM3Runtime i_runtime, IM3Continuation i_cont)
{
    IM3Continuation newCont = Continuation_New(i_runtime, i_cont->type, i_cont->اندراج_Function);
    اگر (M3_UNLIKELY(not newCont)) {
        واپس تہی;
    }

    newCont->val_ڈھیر      = i_cont->val_ڈھیر;
    newCont->عدد_ڈھیر_Slots = i_cont->عدد_ڈھیر_Slots;
    newCont->sp            = i_cont->sp;
    newCont->pc            = i_cont->pc;
    newCont->suspend_نقطہ  = i_cont->suspend_نقطہ;
    newCont->r0            = i_cont->r0;
#  اگر d_m3HasFloat
    newCont->fp0 = i_cont->fp0;
#  endif
    newCont->چوکھٹے    = i_cont->چوکھٹے;
    newCont->چوکھٹے_Cap = i_cont->چوکھٹے_Cap;
    newCont->عدد_چوکھٹے = i_cont->عدد_چوکھٹے;

    newCont->bound_دلائل_گنتی = 0;
#  اگر d_m3HasExceptionHandling
    newCont->resumeThrow = i_cont->resumeThrow;
    i_cont->resumeThrow  = تہی;
#  endif
    newCont->عدد_Suspend_Results = i_cont->عدد_Suspend_Results;
    برائے (u32 i = 0; i < i_cont->عدد_Suspend_Results; ++i) {
        newCont->suspend_نتیجہ_Offsets[i] = i_cont->suspend_نتیجہ_Offsets[i];
        newCont->suspend_نتیجہ_Is64[i]    = i_cont->suspend_نتیجہ_Is64[i];
    }

    newCont->عدد_Handlers = i_cont->عدد_Handlers;
    newCont->handlersPC  = i_cont->handlersPC;
    newCont->والد      = i_cont->والد;
    newCont->state       = cont_suspended;

    i_cont->val_ڈھیر  = تہی;
    i_cont->چوکھٹے    = تہی;
    i_cont->چوکھٹے_Cap = 0;
    i_cont->عدد_چوکھٹے = 0;
    i_cont->state     = cont_consumed;

    واپس newCont;
}

// Only cont.new and snapshot restoration acquire stacks. Binding and suspension
// transfer them between records without changing the active count.
M3_نتیجہ Continuation_Acquire_ڈھیر (IM3Runtime runtime, IM3Continuation cont)
{
    اگر (not Continuation_Can_Acquire_ڈھیر(runtime)) {
        واپس m3_Err_continuation_حد_بندی_Exceeded;
    }
    اگر (runtime->val_ڈھیر_Pool_گنتی) {
        cont->val_ڈھیر = runtime->val_ڈھیر_Pool[--runtime->val_ڈھیر_Pool_گنتی];
        memset(cont->val_ڈھیر, 0, (cont->عدد_ڈھیر_Slots + 4) * حجم(m3slot_t));
    } ورنہ {
        cont->val_ڈھیر = m3_AllocArray(m3slot_t, cont->عدد_ڈھیر_Slots + 4);
    }
    اگر (not cont->val_ڈھیر) {
        واپس m3Err_mallocFailed;
    }
    cont->sp = cont->val_ڈھیر;
    runtime->continuationsAllocated++;
    واپس m3Err_none;
}

خالی Continuation_Recycle_ڈھیر (IM3Runtime runtime, IM3Continuation cont)
{
    اگر (not cont->val_ڈھیر or cont == runtime->جڑ_Continuation) {
        واپس;
    }
    اگر (not runtime->val_ڈھیر_Pool) {
        runtime->val_ڈھیر_Pool = m3_AllocArray(m3slot_t*, 16);
        اگر (runtime->val_ڈھیر_Pool) {
            runtime->val_ڈھیر_Pool_Cap = 16;
        }
    }
    اگر (runtime->val_ڈھیر_Pool_گنتی < runtime->val_ڈھیر_Pool_Cap and
        (not runtime->continuations_حد_بندی or runtime->val_ڈھیر_Pool_گنتی <
                                              runtime->continuations_حد_بندی - runtime->continuationsAllocated + 1)) {
        runtime->val_ڈھیر_Pool[runtime->val_ڈھیر_Pool_گنتی++] = cont->val_ڈھیر;
        cont->val_ڈھیر                                      = تہی;
    } ورنہ {
        m3_Free(cont->val_ڈھیر);
    }
    cont->sp = تہی;
    runtime->continuationsAllocated--;
}

خالی Continuation_ReleaseAll (IM3Runtime io_runtime)
{
    // Records live until teardown: a consumed reference can still be in a table
    // or on a Wasm stack. Its value stack has already been recycled.
    IM3Continuation cont = io_runtime->continuations;

    جب (cont) {
        IM3Continuation اگلا = cont->اگلا;

        m3_Free(cont->val_ڈھیر);
        m3_Free(cont->چوکھٹے);
        m3_Free(cont);

        cont = اگلا;
    }

    برائے (u32 i = 0; i < io_runtime->val_ڈھیر_Pool_گنتی; ++i) {
        m3_Free(io_runtime->val_ڈھیر_Pool[i]);
    }
    m3_Free(io_runtime->val_ڈھیر_Pool);
    io_runtime->val_ڈھیر_Pool_گنتی = io_runtime->val_ڈھیر_Pool_Cap = 0;
    io_runtime->continuationsAllocated                          = 0;
    io_runtime->continuations                                   = تہی;
    io_runtime->فعال_Continuation                              = تہی;
    io_runtime->جڑ_Continuation                                = تہی;
}


m3ret_t Continuation_ریکارڈ_چوکھٹا (IM3Runtime i_runtime, ثابت M3_چوکھٹا* i_چوکھٹا)
{
    IM3Continuation cont = i_runtime->فعال_Continuation;

    // the marker only travels between a suspend and the resume that runs it,
    // and that whole stretch has an active continuation
    d_m3Assert(cont);

    اگر (cont->عدد_چوکھٹے == cont->چوکھٹے_Cap) {
        u32 newCap = cont->چوکھٹے_Cap ? cont->چوکھٹے_Cap * 2 : 8;

        اگر (newCap > d_m3_Continuation_Max_چوکھٹے) {
            newCap = d_m3_Continuation_Max_چوکھٹے;
        }

        اگر (newCap == cont->چوکھٹے_Cap) {
            واپس m3_Err_trap_ڈھیر_حد_سے_تجاوز;
        }

        M3_چوکھٹا* چوکھٹے = m3_ReallocArray(M3_چوکھٹا, cont->چوکھٹے, newCap, cont->چوکھٹے_Cap);

        اگر (not چوکھٹے) {
            واپس m3Err_mallocFailed;
        }

        cont->چوکھٹے    = چوکھٹے;
        cont->چوکھٹے_Cap = newCap;
    }

    cont->چوکھٹے[cont->عدد_چوکھٹے++] = *i_چوکھٹا;

    واپس m3Err_continuationSuspended;
}

#اگر_ختم // d_m3HasStackSwitching

M3_نتیجہ Evaluate_عبارت (IM3Module i_module, خالی* o_expressed, m3type_t i_type, بائٹیں_t* io_بائٹیں, cbytes_t i_انتہا)
{
    M3_نتیجہ نتیجہ = m3Err_none;

    // OPTZ: use a simplified interpreter for expressions

    // create a temporary runtime context
#اگر defined(d_m3PreferStaticAlloc)
    ساکن M3Runtime runtime;
#ورنہ
    M3Runtime runtime;
#اگر_ختم
    M3_INIT(runtime);

    runtime.environment   = i_module->runtime->environment;
    runtime.عدد_ڈھیر_Slots = i_module->runtime->عدد_ڈھیر_Slots;
    runtime.ڈھیر         = i_module->runtime->ڈھیر;

    m3stack_t ڈھیر = (m3stack_t)runtime.ڈھیر;

    IM3Runtime savedRuntime = i_module->runtime;
    i_module->runtime       = &runtime;

    IM3Compilation o   = &runtime.compilation;
    o->runtime         = &runtime;
    o->module          = i_module;
    o->wasm            = *io_بائٹیں;
    o->wasm_انتہا         = i_انتہا;
    o->آخری_Opcode_آغاز = o->wasm;

    // Borrowed for the length of this expression only. Runtime_Release below hands it
    // to Environment_ReleaseCodePages, which resets lineIndex, so the page comes back
    // empty and the next expression reuses it rather than burning fresh lines.
    o->صفحہ = Acquire_رمز_صفحہ(&runtime);

    اگر (o->صفحہ) {
        IM3FuncType ftype = تہی;

#اگر d_m3HasTypedRefs
        // A reference type has to be checked as the type it is: the shared
        // return types are one per storage class, so borrowing one of those
        // would let any reference at all satisfy a particular one.
        اگر (IsSpelledRefType(i_type)) {
            نتیجہ = AllocFuncType(&ftype, 1);

            اگر (not نتیجہ) {
                ftype->عدد_Rets  = 1;
                ftype->types[0] = i_type;

                نتیجہ = Environment_جوڑو_Func_Type(runtime.environment, &ftype);
            }
        } ورنہ
#اگر_ختم
        {
            ftype = runtime.environment->retFuncTypes[بنیاد_Type_کا(i_type)];
        }

        pc_t m3code = Get_صفحہ_PC(o->صفحہ);

        اگر (not نتیجہ) {
            نتیجہ = Compile_عبارت(o, ftype);
        }

        اگر (not نتیجہ && o->max_ڈھیر_Slots >= runtime.عدد_ڈھیر_Slots) {
            نتیجہ = m3_Err_trap_ڈھیر_حد_سے_تجاوز;
        }

        اگر (not نتیجہ) {
#اگر (d_m3EnableOpProfiling || d_m3EnableOpTracing)
            m3ret_t r = تسلسل_رمز(m3code, ڈھیر, تہی, d_m3_Op_Default_دلائل, d_m3_بنیاد_Cstr);
#ورنہ
            m3ret_t r = تسلسل_رمز(m3code, ڈھیر, تہی, d_m3_Op_Default_دلائل);
#اگر_ختم

            اگر (r == 0) {                                                                   m3log (runtime, "expression result: %s", SPrint_قدر (ڈھیر, i_type));
                اگر (سائز_کا_Type(بنیاد_Type_کا(i_type)) == حجم(u32)) {
                    *(u32*)o_expressed = *((u32*)ڈھیر);
                } ورنہ {
                    *(u64*)o_expressed = *((u64*)ڈھیر);
                }
            }
        }

        Release_رمز_صفحہ(&runtime, o->صفحہ);
    } ورنہ {
        نتیجہ = m3_Err_malloc_Failed_رمز_صفحہ;
    }

    runtime.origin_ڈھیر = تہی;        // prevent free(stack) in ReleaseRuntime
    Runtime_Release(&runtime);
    i_module->runtime = savedRuntime;

    *io_بائٹیں = o->wasm;

    واپس نتیجہ;
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
ساکن
bool LimitsSatisfy (u64 i_exported_سائز, bool i_exportedHasMax, u64 i_exportedMax,
                    u64 i_importMin, bool i_importHasMax, u64 i_importMax)
{
    اگر (i_exported_سائز < i_importMin) {
        واپس false;
    }

    اگر (i_importHasMax and (not i_exportedHasMax or i_exportedMax > i_importMax)) {
        واپس false;
    }

    واپس true;
}


// The export of this kind under this name, or NULL. The index it carries is
// resolved by the caller, against the module's index space as it stands then -
// after linking, a re-exported import's slot holds the entity it was linked to.
ساکن
ثابت M3Export* Module_ڈھونڈو_Export (IM3Module i_module, u8 i_قسم, cstr_t i_name)
{
    برائے (u32 i = 0; i < i_module->عدد_Exports; ++i) {
        ثابت M3Export* e = &i_module->exports[i];

        اگر (e->قسم == i_قسم and strcmp(e->name, i_name) == 0) {
            واپس e;
        }
    }

    واپس تہی;
}


ساکن
IM3Function Module_ڈھونڈو_Exported_Function (IM3Module i_module, cstr_t i_name)
{
    ثابت M3Export* e = Module_ڈھونڈو_Export(i_module, d_external_قسم_function, i_name);

    // A module that re-exports an import names the placeholder here. Resolve to
    // the function that actually runs, or a host-side call would run it against
    // the importing module's memory.
    واپس e ? Function_Implementation(&i_module->functions[e->index]) : تہی;
}


ساکن
IM3_حافظہ Module_ڈھونڈو_Exported_حافظہ (IM3Module i_module, cstr_t i_name)
{
    ثابت M3Export* e = Module_ڈھونڈو_Export(i_module, d_external_قسم_حافظہ, i_name);

    واپس e ? i_module->memories[e->index] : تہی;
}


ساکن
IM3_جدول Module_ڈھونڈو_Exported_جدول (IM3Module i_module, cstr_t i_name)
{
    ثابت M3Export* e = Module_ڈھونڈو_Export(i_module, d_external_قسم_جدول, i_name);

    واپس e ? i_module->tables[e->index] : تہی;
}


ساکن
IM3Global Module_ڈھونڈو_Exported_Global (IM3Module i_module, cstr_t i_name)
{
    ثابت M3Export* e = Module_ڈھونڈو_Export(i_module, d_external_قسم_global, i_name);

    واپس e ? &i_module->globals[e->index] : تہی;
}


#اگر d_m3HasExceptionHandling || d_m3_Has_ڈھیر_Switching
ساکن
IM3_لیبل Module_ڈھونڈو_Exported_لیبل (IM3Module i_module, cstr_t i_name)
{
    ثابت M3Export* e = Module_ڈھونڈو_Export(i_module, d_external_قسم_لیبل, i_name);

    اگر (not e) {
        واپس تہی;
    }

    IM3_لیبل لیبل = &i_module->لیبل_فہرست[e->index];

    واپس لیبل->resolved ? لیبل->resolved : لیبل;
}
#اگر_ختم


// Whether the module exports anything at all under this name. Export names are
// unique within a module, so a name one of the lookups above missed but this
// one finds is exported as something else - a kind the import cannot be
// satisfied by, rather than a name the module never exported.
ساکن
bool Module_HasExport (IM3Module i_module, cstr_t i_name)
{
    برائے (u32 i = 0; i < i_module->عدد_Exports; ++i) {
        اگر (strcmp(i_module->exports[i].name, i_name) == 0) {
            واپس true;
        }
    }

    واپس false;
}


// Points each of the module's imports at whatever already-loaded module exports
// it. Runs before anything is allocated or initialized: a memory import has to
// be resolved before InitMemory would give it pages of its own, and a global
// import before InitGlobals runs an initializer that reads it.
ساکن
M3_نتیجہ LinkImports (IM3Runtime io_runtime, IM3Module io_module)
{
    M3_نتیجہ نتیجہ = m3Err_none;

    برائے (u32 i = 0; i < io_module->عدد_Functions; ++i) {
        IM3Function f = &io_module->functions[i];

        اگر (f->wasm or not (f->import.moduleUtf8 and f->import.میدان_Utf8)) {
            جاری;
        }

        IM3Module from = m3_ڈھونڈو_Module(io_runtime, f->import.moduleUtf8);
        اگر (not from) {
            جاری;
        }

        IM3Function exported = Module_ڈھونڈو_Exported_Function(from, f->import.میدان_Utf8);
        اگر (not exported) {
            // the import names a module that is loaded, so its exports settle the
            // question: a name it exports as something else is a type mismatch, and
            // one it does not export at all is an unknown import
            _throwif(m3Err_incompatibleImportType, Module_HasExport(from, f->import.میدان_Utf8));
            _throw(m3Err_unknownImport);
        }

        // func types are canonical within an environment, so this is the
        // structural equivalence the spec asks for
        _throwif(m3Err_incompatibleImportType, exported->funcType != f->funcType);

        f->resolved = Function_Implementation(exported);
    }

    برائے (u32 i = 0; i < io_module->عدد_Memories; ++i) {
        IM3_حافظہ حافظہ = io_module->memories[i];

        اگر (not حافظہ->imported or حافظہ->owner != io_module) {
            جاری;
        }

        IM3Module from = m3_ڈھونڈو_Module(io_runtime, حافظہ->import.moduleUtf8);
        اگر (not from) {
            جاری;
        }

        IM3_حافظہ exported = Module_ڈھونڈو_Exported_حافظہ(from, حافظہ->import.میدان_Utf8);
        اگر (not exported) {
            _throwif(m3Err_incompatibleImportType, Module_HasExport(from, حافظہ->import.میدان_Utf8));
            _throw(m3Err_unknownImport);
        }

        // the address type is part of the memory type, so an i64 memory does
        // not satisfy an i32 import, or the other way round - and so is the page
        // size, which custom page sizes made a declared property of a memory
        _throwif(m3Err_incompatibleImportType, exported->isMemory64 != حافظہ->isMemory64);
        _throwif(m3Err_incompatibleImportType, حافظہ_صفحہ_سائز(exported) != حافظہ_صفحہ_سائز(حافظہ));

        // a shared memory is not a subtype of an unshared one, or the other way round
        _throwif(m3Err_incompatibleImportType, exported->isShared != حافظہ->isShared);

        _throwif(m3Err_incompatibleImportType,
                 not LimitsSatisfy(exported->عدد_صفحات, exported->hasMax, exported->max_صفحات,
                                   حافظہ->init_صفحات, حافظہ->hasMax, حافظہ->max_صفحات));

        // hand the slot over to the exporter's memory, and drop the placeholder
        Free_حافظہ_بلاک(حافظہ);
        FreeImportInfo(&حافظہ->import);
        m3_Free(حافظہ);

        io_module->memories[i] = exported;
    }

    برائے (u32 i = 0; i < io_module->عدد_Tables; ++i) {
        IM3_جدول جدول_2 = io_module->tables[i];

        اگر (not جدول_2->imported or جدول_2->owner != io_module) {
            جاری;
        }

        IM3Module from = m3_ڈھونڈو_Module(io_runtime, جدول_2->import.moduleUtf8);
        اگر (not from) {
            جاری;
        }

        IM3_جدول exported = Module_ڈھونڈو_Exported_جدول(from, جدول_2->import.میدان_Utf8);
        اگر (not exported) {
            _throwif(m3Err_incompatibleImportType, Module_HasExport(from, جدول_2->import.میدان_Utf8));
            _throw(m3Err_unknownImport);
        }

        _throwif(m3Err_incompatibleImportType, exported->type != جدول_2->type);

        // the index type is part of the table type, the same way it is for a memory
        _throwif(m3Err_incompatibleImportType, exported->isTable64 != جدول_2->isTable64);

        _throwif(m3Err_incompatibleImportType,
                 not LimitsSatisfy(exported->سائز, exported->hasMax, exported->max_سائز,
                                   جدول_2->init_سائز, جدول_2->hasMax, جدول_2->max_سائز));

        // hand the slot over to the exporter's table, and drop the placeholder
        m3_Free(جدول_2->elements);
        FreeImportInfo(&جدول_2->import);
        m3_Free(جدول_2);

        io_module->tables[i] = exported;
    }

    برائے (u32 i = 0; i < io_module->عدد_Globals; ++i) {
        IM3Global g = &io_module->globals[i];

        اگر (not g->imported or not (g->import.moduleUtf8 and g->import.میدان_Utf8)) {
            جاری;
        }

        IM3Module from = m3_ڈھونڈو_Module(io_runtime, g->import.moduleUtf8);
        اگر (not from) {
            جاری;
        }

        IM3Global exported = Module_ڈھونڈو_Exported_Global(from, g->import.میدان_Utf8);
        اگر (not exported) {
            _throwif(m3Err_incompatibleImportType, Module_HasExport(from, g->import.میدان_Utf8));
            _throw(m3Err_unknownImport);
        }

        _throwif(m3Err_incompatibleImportType, exported->type != g->type);
        _throwif(m3Err_incompatibleImportType, exported->isMutable != g->isMutable);

        g->resolved = exported->resolved ? exported->resolved : exported;
    }

#اگر d_m3HasExceptionHandling || d_m3_Has_ڈھیر_Switching
    برائے (u32 i = 0; i < io_module->عدد_لیبل_فہرست; ++i) {
        IM3_لیبل لیبل = &io_module->لیبل_فہرست[i];

        اگر (not لیبل->imported or not (لیبل->import.moduleUtf8 and لیبل->import.میدان_Utf8)) {
            جاری;
        }

        IM3Module from = m3_ڈھونڈو_Module(io_runtime, لیبل->import.moduleUtf8);
        اگر (not from) {
            جاری;
        }

        IM3_لیبل exported = Module_ڈھونڈو_Exported_لیبل(from, لیبل->import.میدان_Utf8);
        اگر (not exported) {
            _throwif(m3Err_incompatibleImportType, Module_HasExport(from, لیبل->import.میدان_Utf8));
            _throw(m3Err_unknownImport);
        }

        _throwif(m3Err_incompatibleImportType, exported->type != لیبل->type);

        لیبل->resolved = exported->resolved ? exported->resolved : exported;
    }
#اگر_ختم

    _catch: واپس نتیجہ;
}


// Backs each of the module's memories with pages. LinkImports has already
// pointed any import it could satisfy at the exporting module's memory, and
// those are skipped here. An import nothing satisfied is still backed locally
// from its own declared limits, so that the module remains loadable.
M3_نتیجہ Init_حافظہ (IM3Runtime io_runtime, IM3Module i_module)
{
    M3_نتیجہ نتیجہ = m3Err_none;

    // Fixed from here on: the index space stops changing after parse, and
    // linking has already repointed any slot it was going to.
    i_module->memory0 = i_module->عدد_Memories ? i_module->memories[0]
                                              : &i_module->خالی_خانے_حافظہ;

    اگر (i_module->عدد_Memories == 0) {
        // nothing addressable, but _mem still has to point somewhere
        i_module->خالی_خانے_حافظہ.owner    = i_module;
        i_module->خالی_خانے_حافظہ.صفحہ_سائز = d_m3_Default_Mem_صفحہ_سائز;

_       (Resize_حافظہ(io_runtime, &i_module->خالی_خانے_حافظہ, 0));
    }

    برائے (u32 i = 0; i < i_module->عدد_Memories; ++i) {
        IM3_حافظہ حافظہ = i_module->memories[i];

        // a slot that already points at another module's memory is that
        // module's to allocate
        اگر (حافظہ->owner != i_module or حافظہ->mallocated) {
            جاری;
        }

        u32 صفحہ_سائز = حافظہ_صفحہ_سائز(حافظہ);

        حافظہ->صفحہ_سائز = صفحہ_سائز;

        // Without a declared maximum a memory may grow to the spec limit of
        // 2^|addrtype|/pagesize pages, which is the usual 65536 at the default
        // page size, 2^48 for a 64-bit memory, and a whole address space of
        // them when a page is a single byte. A declared maximum of zero is a
        // real limit, not the absence of one.
        //
        // 2^64/pagesize overflows a u64 only when a page is a single byte, and
        // a power-of-two page size divides the address space exactly, so the
        // 64-bit division below is 2^64/pagesize written so that it fits.
        اگر (not حافظہ->hasMax) {
            حافظہ->max_صفحات = حافظہ->isMemory64
                                 ? (صفحہ_سائز > 1 ? (UINT64_MAX / صفحہ_سائز) + 1 : UINT64_MAX)
                                 : (0x100000000ull / صفحہ_سائز);
        }

_       (Resize_حافظہ(io_runtime, حافظہ, حافظہ->init_صفحات));
    }

    _catch: واپس نتیجہ;
}


M3_نتیجہ Resize_حافظہ (IM3Runtime io_runtime, IM3_حافظہ حافظہ, u64 i_عدد_صفحات)
{
    M3_نتیجہ نتیجہ = m3Err_none;

    u64 عدد_صفحات_مقصد_Alloc = i_عدد_صفحات;

    اگر (عدد_صفحات_مقصد_Alloc <= حافظہ->max_صفحات) {
        // A 64-bit memory may ask for up to 2^48 pages, which overflows a u64
        // of bytes. Nothing that large can be backed, so refuse it up front
        // rather than multiplying into a wrapped size.
        _throwif("linear memory limitation exceeded",
                 عدد_صفحات_مقصد_Alloc > d_m3_پتہ_حد_بندی / حافظہ->صفحہ_سائز);

        u64 عدد_صفحہ_بائٹیں = عدد_صفحات_مقصد_Alloc * حافظہ->صفحہ_سائز;

#اگر d_m3_Max_خطی_حافظہ_صفحات > 0
        // the limit is a memory size, counted in default-sized pages; comparing
        // it against a raw page count would make it 65536 times stricter for a
        // module whose pages are one byte
        _throwif("linear memory limitation exceeded",
                 عدد_صفحہ_بائٹیں > (u64)d_m3_Max_خطی_حافظہ_صفحات * d_m3_Default_Mem_صفحہ_سائز);
#اگر_ختم

        // Limit the amount of memory that gets actually allocated
        اگر (io_runtime->حافظہ_حد_بندی) {
            عدد_صفحہ_بائٹیں = M3_MIN(عدد_صفحہ_بائٹیں, (u64)io_runtime->حافظہ_حد_بندی);
        }

        u64 old_لمبائی = حافظہ->mallocated ? حافظہ->mallocated->لمبائی : 0;
        u64 other_مستعمل = io_runtime->حافظہ_بائٹیں_مستعمل - old_لمبائی;
        _throwif(m3_Err_حافظہ_حد_بندی_Exceeded, io_runtime->حافظہ_بائٹیں_حد_بندی and
                                              (عدد_صفحہ_بائٹیں > io_runtime->حافظہ_بائٹیں_حد_بندی or
                                               other_مستعمل > io_runtime->حافظہ_بائٹیں_حد_بندی - عدد_صفحہ_بائٹیں));

        _throwif("linear memory limitation exceeded", عدد_صفحہ_بائٹیں > (u64)SIZE_MAX - حجم(M3_حافظہ_سرخی));

#اگر d_m3_Guarded_حافظہ

        // A slot holds every address a Wasm access can name, and nothing beyond
        // that: a memory larger than the data part of one would put addresses past
        // the guard, where a fault is nobody's to recognize
        _throwif("linear memory limitation exceeded", عدد_صفحہ_بائٹیں > d_m3_Guarded_ڈیٹا_بائٹیں);

        // What makes an access past the end fault is the page after the memory not
        // being committed, so the end has to fall on one of the system's page
        // boundaries. A Wasm page divides evenly into them and always will; a memory
        // that declared a page size of its own may not, and then the bytes between
        // the end of the memory and the end of its last system page would answer
        // instead of trapping. Refusing to back such a memory is the honest way out:
        // the custom page sizes proposal exists for targets far too small to reserve
        // 8GiB per memory in the first place. See d_m3GuardedMemory.
        _throwif("guarded memory needs a page size the system's divides into",
                 (حافظہ->صفحہ_سائز % m3_Host_صفحہ_سائز()) != 0);

        اگر (حافظہ->guard_خانہ == تہی) {
            حافظہ->guard_خانہ = Guard_Take_خانہ();
            _throwifnull(حافظہ->guard_خانہ);
        }

        // Growing only ever commits more of the same slot, so the memory never
        // moves - which the interpreter does not rely on, but nothing minds
        _throwif(m3Err_mallocFailed, not Guard_Commit_خانہ(حافظہ->guard_خانہ, (size_t)عدد_صفحہ_بائٹیں));

        حافظہ->mallocated = Guard_خانہ_سرخی(حافظہ->guard_خانہ);

#ورنہ

        size_t عدد_بائٹیں = (size_t)عدد_صفحہ_بائٹیں + حجم(M3_حافظہ_سرخی);

        // What is really there, which is not numPages * pageSize: the limit above
        // caps the bytes without capping the page count, so a memory can be short
        // of a page - by design, since a whole 64 KiB page is often more than an
        // MCU has. Measuring the old block by the page count would overstate it,
        // and then realloc never sees the size it already has and copies the
        // whole limit again on every grow past it.
        size_t عدد_پچھلا_بائٹیں = حافظہ->mallocated ? حافظہ->mallocated->لمبائی + حجم(M3_حافظہ_سرخی) : 0;

        خالی* newMem = m3_Realloc("Wasm Linear Memory", حافظہ->mallocated, عدد_بائٹیں, عدد_پچھلا_بائٹیں);
        _throwifnull(newMem);

        حافظہ->mallocated = (M3_حافظہ_سرخی*)newMem;

#اگر_ختم

#اگر d_m3LogRuntime
        M3_حافظہ_سرخی* oldMallocated = حافظہ->mallocated;
#اگر_ختم

        io_runtime->حافظہ_بائٹیں_مستعمل = other_مستعمل + عدد_صفحہ_بائٹیں;
        حافظہ->عدد_صفحات            = عدد_صفحات_مقصد_Alloc;

        حافظہ->mallocated->لمبائی  = (size_t)عدد_صفحہ_بائٹیں;
        حافظہ->mallocated->runtime = io_runtime;
        حافظہ->mallocated->حافظہ  = حافظہ;

        حافظہ->mallocated->max_ڈھیر = (m3slot_t*)io_runtime->origin_ڈھیر + io_runtime->عدد_ڈھیر_Slots;

        m3log(runtime, "resized old: %p; mem: %p; length: %zu; pages: %llu", oldMallocated, حافظہ->mallocated, حافظہ->mallocated->لمبائی, (غیرعلامتی طویل طویل)حافظہ->عدد_صفحات);
    } ورنہ {
        نتیجہ = m3_Err_wasm_حافظہ_حد_سے_تجاوز;
    }

    _catch: واپس نتیجہ;
}


خالی Free_حافظہ_بلاک (IM3_حافظہ io_حافظہ)
{
    اگر (io_حافظہ->mallocated and io_حافظہ->mallocated->runtime) {
        io_حافظہ->mallocated->runtime->حافظہ_بائٹیں_مستعمل -= io_حافظہ->mallocated->لمبائی;
    }
#اگر d_m3_Guarded_حافظہ
    Guard_Give_خانہ(io_حافظہ->guard_خانہ);
    io_حافظہ->guard_خانہ = تہی;

    io_حافظہ->mallocated = تہی;
#ورنہ
    m3_Free(io_حافظہ->mallocated);
#اگر_ختم
}


M3_نتیجہ InitGlobals (IM3Module io_module)
{
    M3_نتیجہ نتیجہ = m3Err_none;

    اگر (io_module->عدد_Globals) {
        // placing the globals in their structs isn't good for cache locality, but i don't really know what the global
        // access patterns typically look like yet.

        //          io_module->globalMemory = m3Alloc (m3reg_t, io_module->numGlobals);

        //          if (io_module->globalMemory)
        {
            برائے (u32 i = 0; i < io_module->عدد_Globals; ++i) {
                M3Global* g = &io_module->globals[i];                           m3log (runtime, "initializing global: %d", i);

                اگر (g->init_اظہاریہ) {
                    بائٹیں_t آغاز = g->init_اظہاریہ;

                    نتیجہ = Evaluate_عبارت(io_module, &g->i64_قدر, g->type, &آغاز, g->init_اظہاریہ + g->init_اظہاریہ_سائز);

                    اگر (not نتیجہ) {
                        // io_module->globalMemory [i] = initValue;
                    } ورنہ {
                        توڑو;
                    }
                } ورنہ {                                                        m3log (runtime, "importing global");
                }
            }
        }
        //          else result = ErrorModule (m3Err_mallocFailed, io_module, "could allocate globals for module: '%s", io_module->name);
    }

    واپس نتیجہ;
}


M3_نتیجہ Init_ڈیٹا_Segments (IM3Module io_module)
{
    M3_نتیجہ نتیجہ = m3Err_none;

    برائے (u32 i = 0; i < io_module->عدد_ڈیٹا_Segments; ++i) {
        M3_ڈیٹا_Segment* segment = &io_module->ڈیٹا_Segments[i];

        // A passive segment stays available for memory.init until data.drop.
        // An active one is copied here and then counts as dropped.
        اگر (segment->isPassive) {
            جاری;
        }

        _throwif("data segment memory index out of range",
                 segment->حافظہ_Region >= io_module->عدد_Memories);

        IM3_حافظہ io_حافظہ = io_module->memories[segment->حافظہ_Region];

        _throwif("unallocated linear memory", !(io_حافظہ->mallocated));

        // The offset expression has the memory's address type, and is
        // unsigned: an i32 offset of -1 is 4294967295, way out of bounds
        // rather than negative.
        u64     segment_انحراف = 0;
        بائٹیں_t آغاز         = segment->init_اظہاریہ;

        اگر (io_حافظہ->isMemory64) {
_           (Evaluate_عبارت(io_module, &segment_انحراف, c_m3Type_i64, &آغاز, segment->init_اظہاریہ + segment->init_اظہاریہ_سائز));
        } ورنہ {
            u32 offset32;
_           (Evaluate_عبارت(io_module, &offset32, c_m3Type_i32, &آغاز, segment->init_اظہاریہ + segment->init_اظہاریہ_سائز));
            segment_انحراف = offset32;
        }

        m3log(runtime, "loading data segment: %d; size: %d; offset: %llu", i, segment->سائز, (غیرعلامتی طویل طویل)segment_انحراف);

        اگر (segment_انحراف <= io_حافظہ->mallocated->لمبائی &&
            (u64)segment->سائز <= io_حافظہ->mallocated->لمبائی - segment_انحراف) {
            u8* dest = m3_Mem_ڈیٹا(io_حافظہ->mallocated) + segment_انحراف;
            حافظہ_نقل(dest, segment->ڈیٹا, segment->سائز);
        } ورنہ {
            _throw("data segment out of bounds");
        }

        segment->dropped = true;
    }

    _catch: واپس نتیجہ;
}


// Turns a segment's elements into references. Element expressions are constant
// expressions restricted to ref.null/ref.func, so they're read directly rather
// than run through the compiler.
ساکن
M3_نتیجہ ResolveElements (IM3Module io_module, M3ElementSegment* i_segment, خالی** o_elements)
{
    M3_نتیجہ نتیجہ = m3Err_none;

    بائٹیں_t  pos = i_segment->elements;
    cbytes_t انتہا = io_module->element_Section_انتہا;

    برائے (u32 e = 0; e < i_segment->عدد_Elements; ++e) {
        u32   func_اشاریہ;
        خالی* ref = تہی;

        اگر (i_segment->is_اظہاریہ) {
            m3opcode_t opcode;
_           (Read_opcode(&opcode, &pos, انتہا));

            اگر (opcode == c_waOp_refFunc) {
_               (ReadLEB_u32(&func_اشاریہ, &pos, انتہا));
                _throwif("function index out of range", func_اشاریہ >= io_module->عدد_Functions);
                ref = Function_Implementation(&io_module->functions[func_اشاریہ]);
            } ورنہ اگر (opcode == c_waOp_refNull) {
                i8 waType;
                u8 nullType;
_               (ReadLEB_i7(&waType, &pos, انتہا));
_               (NormalizeType(&nullType, waType));
                _throwif(m3Err_typeMismatch, nullType != i_segment->type);
            } ورنہ اگر (opcode == c_waOp_getGlobal) {
                // wasm 2.0 lets an element expression read an imported
                // immutable global, which is how one module seeds another's
                // table with a reference it exported.
                u32 global_اشاریہ;
_               (ReadLEB_u32(&global_اشاریہ, &pos, انتہا));
                _throwif(m3_Err_globa_اشاریہ_برآمد_کا_Bounds, global_اشاریہ >= io_module->عدد_Globals);

                IM3Global global = &io_module->globals[global_اشاریہ];

                _throwif(m3_Err_globa_اشاریہ_برآمد_کا_Bounds, not global->imported);
                _throwif(m3Err_wasmMalformed, global->isMutable);
                _throwif(m3Err_typeMismatch, بنیاد_Type_کا(global->type) != بنیاد_Type_کا(i_segment->type));

                // read the cell the import was linked to, not the placeholder
                اگر (global->resolved) {
                    global = global->resolved;
                }

                ref = global->ref_قدر;
            } ورنہ {
                _throw("constant expression required");
            }

_           (Read_opcode(&opcode, &pos, انتہا));
            _throwif(m3Err_wasmMalformed, opcode != c_wa_Op_انتہا);
        } ورنہ {
_           (ReadLEB_u32(&func_اشاریہ, &pos, انتہا));
            _throwif("function index out of range", func_اشاریہ >= io_module->عدد_Functions);
            ref = Function_Implementation(&io_module->functions[func_اشاریہ]);
        }

        o_elements[e] = ref;
    }

    _catch: واپس نتیجہ;
}


M3_نتیجہ Init_جدول_And_Elements (IM3Module io_module)
{
    M3_نتیجہ نتیجہ = m3Err_none;

    cbytes_t انتہا = io_module->element_Section_انتہا;
    M3_جدول* جدول_2;

    برائے (u32 i = 0; i < io_module->عدد_Tables; ++i) {
        جدول_2 = io_module->tables[i];

        // a slot pointing at another module's table is that module's to fill
        اگر (جدول_2->owner != io_module) {
            جاری;
        }

        اگر (جدول_2->سائز) {
            IM3Runtime runtime = io_module->runtime;
            _throwif(m3_Err_جدول_حد_بندی_Exceeded, runtime->جدول_Elements_حد_بندی and
                                                 جدول_2->سائز > runtime->جدول_Elements_حد_بندی - runtime->جدول_Elements_مستعمل);
            جدول_2->elements = m3_AllocArray(خالی*, جدول_2->سائز);
            _throwifnull(جدول_2->elements);
            runtime->جدول_Elements_مستعمل += جدول_2->سائز;

            اگر (جدول_2->init_اظہاریہ) {
                خالی*   قدر = تہی;
                بائٹیں_t آغاز = جدول_2->init_اظہاریہ;
_               (Evaluate_عبارت(io_module, &قدر, بنیاد_Type_کا(جدول_2->type),
                                     &آغاز, جدول_2->init_اظہاریہ + جدول_2->init_اظہاریہ_سائز));

                برائے (u32 e = 0; e < جدول_2->سائز; ++e) {
                    جدول_2->elements[e] = قدر;
                }
            }
        }
    }

    برائے (u32 i = 0; i < io_module->عدد_Element_Segments; ++i) {
        M3ElementSegment* segment = &io_module->elementSegments[i];

        // Declarative segments only make their functions referenceable, and
        // passive ones wait for table.init, so neither is written out here.
        اگر (segment->منوال == c_m3Elem_declarative) {
            segment->dropped = true;
            جاری;
        }

        اگر (segment->منوال == c_m3Elem_passive) {
            اگر (segment->عدد_Elements) {
                segment->resolved = m3_AllocArray(خالی*, segment->عدد_Elements);
                _throwifnull(segment->resolved);
_               (ResolveElements(io_module, segment, segment->resolved));
            }
            جاری;
        }

        جدول_2 = io_module->tables[segment->جدول_اشاریہ];

        // The offset expression has the table's index type, and is unsigned:
        // an i32 offset of -1 is 4294967295, out of bounds rather than negative.
        u64     offset = 0;
        بائٹیں_t اظہاریہ   = segment->init_اظہاریہ;

        اگر (جدول_2->isTable64) {
_           (Evaluate_عبارت(io_module, &offset, c_m3Type_i64, &اظہاریہ, انتہا));
        } ورنہ {
            u32 offset32;
_           (Evaluate_عبارت(io_module, &offset32, c_m3Type_i32, &اظہاریہ, انتہا));
            offset = offset32;
        }

        _throwif("out of bounds table access",
                 offset > جدول_2->سائز or segment->عدد_Elements > جدول_2->سائز - offset);

_       (ResolveElements(io_module, segment, جدول_2->elements + offset));

        segment->dropped = true;
    }

    _catch: واپس نتیجہ;
}

M3_نتیجہ m3_CompileModule (IM3Module io_module)
{
    M3_نتیجہ نتیجہ = m3Err_none;

    برائے (u32 i = 0; i < io_module->عدد_Functions; ++i) {
        IM3Function f = &io_module->functions[i];
        اگر (f->wasm and not f->compiled) {
_           (CompileFunction(f));
        }
    }

    _catch: واپس نتیجہ;
}

#اگر d_m3HasExceptionHandling

M3Exception* NewException (IM3Runtime io_runtime, IM3_لیبل i_لیبل, u32 i_عدد_دلائل)
{
    M3Exception* exception = (M3Exception*)m3_Malloc("M3Exception", حجم(M3Exception) + i_عدد_دلائل * حجم(u64));

    اگر (exception) {
        exception->لیبل     = i_لیبل;
        exception->عدد_دلائل = i_عدد_دلائل;
        exception->reified = false;
        exception->پچھلا    = تہی;
        exception->اگلا    = io_runtime->exceptions;

        اگر (exception->اگلا) {
            exception->اگلا->پچھلا = exception;
        }

        io_runtime->exceptions = exception;
    }

    واپس exception;
}


// Releases one exception ahead of the rest. The caller has to know nothing can
// still name it: no exnref was ever taken of it, and its payload has already
// been copied out.
خالی FreeException (IM3Runtime io_runtime, M3Exception* i_exception)
{
    اگر (i_exception->پچھلا) {
        i_exception->پچھلا->اگلا = i_exception->اگلا;
    } ورنہ {
        io_runtime->exceptions = i_exception->اگلا;
    }

    اگر (i_exception->اگلا) {
        i_exception->اگلا->پچھلا = i_exception->پچھلا;
    }

    اگر (io_runtime->pendingException == i_exception) {
        io_runtime->pendingException = تہی;
    }

    m3_Free_Impl(i_exception);
}


// Releases every exception the runtime still holds. Only safe once the Wasm
// stack is empty, which is why the outermost RunCodeChecked() is the one that
// calls it.
خالی FreeExceptions (IM3Runtime io_runtime)
{
    M3Exception* exception = io_runtime->exceptions;

    io_runtime->exceptions       = تہی;
    io_runtime->pendingException = تہی;

    جب (exception) {
        M3Exception* اگلا = exception->اگلا;
        m3_Free_Impl(exception);
        exception = اگلا;
    }
}


// Keeps a copy of the exception a call just ended with, for m3_GetExceptionTag,
// or forgets the last one when the call ended any other way. A copy, because
// the original is freed with the rest before the outermost call returns - and
// one a host function's nested call ended with may still be named by an
// exnref further down the stack.
ساکن
خالی ریکارڈ_Uncaught_Exception (IM3Runtime io_runtime, M3_نتیجہ i_نتیجہ)
{
    اگر (io_runtime->uncaughtException) {
        m3_Free(io_runtime->uncaughtException);
    }

    M3Exception* exception = io_runtime->pendingException;

    اگر (M3_UNLIKELY(i_نتیجہ == m3Err_trapUncaughtException) and exception) {
        M3Exception* copy = (M3Exception*)m3_CopyMem(exception, حجم(M3Exception) + exception->عدد_دلائل * حجم(u64));

        اگر (copy) {
            copy->اگلا = تہی;
            copy->پچھلا = تہی;
        }

        io_runtime->uncaughtException = copy;
    }
}

#اگر_ختم // d_m3HasExceptionHandling


#اگر d_m3HasExceptionHandling || d_m3_Has_ڈھیر_Switching

IM3_لیبل m3_ڈھونڈو_لیبل (IM3Module i_module, ثابت حرف* ثابت i_لیبل_Name)
{
    اگر (not i_module or not i_لیبل_Name) {
        واپس تہی;
    }

    واپس Module_ڈھونڈو_Exported_لیبل(i_module, i_لیبل_Name);
}


M3_نتیجہ m3_New_لیبل (IM3Runtime io_runtime, IM3_لیبل* o_لیبل, ثابت حرف* ثابت i_signature)
{
    IM3FuncType type    = تہی;
    M3_Host_لیبل*  host_لیبل = تہی;

_try {
    _throwif("null tag", not o_لیبل);
    *o_لیبل = تہی;
    _throwif(m3Err_mallocFailed, not io_runtime);

_   (Signature_مقصد_Func_Type(&type, i_signature));
    // tags are the same type exactly when their types are the same object
_   (Environment_جوڑو_Func_Type(io_runtime->environment, &type));

    host_لیبل = m3_AllocStruct(M3_Host_لیبل);
    _throwifnull(host_لیبل);

    host_لیبل->لیبل.type    = type;
    host_لیبل->اگلا        = io_runtime->host_لیبل_فہرست;
    io_runtime->host_لیبل_فہرست = host_لیبل;

    *o_لیبل = &host_لیبل->لیبل;
} _catch:
    واپس نتیجہ;
}


M3_نتیجہ m3_Link_لیبل (IM3Module         io_module,
                     ثابت حرف* ثابت i_moduleName,
                     ثابت حرف* ثابت i_لیبل_Name,
                     IM3_لیبل            i_لیبل)
{
    اگر (not io_module or not i_moduleName or not i_لیبل_Name or not i_لیبل) {
        واپس m3_Err_لیبل_Lookup_Failed;
    }

    ثابت bool wildcardModule = (strcmp(i_moduleName, "*") == 0);
    IM3_لیبل     ہدف         = i_لیبل->resolved ? i_لیبل->resolved : i_لیبل;

    M3_نتیجہ نتیجہ = m3_Err_لیبل_Lookup_Failed;

    برائے (u32 i = 0; i < io_module->عدد_لیبل_فہرست; ++i) {
        IM3_لیبل لیبل = &io_module->لیبل_فہرست[i];

        اگر (لیبل->imported and لیبل->import.moduleUtf8 and لیبل->import.میدان_Utf8 and
            strcmp(لیبل->import.میدان_Utf8, i_لیبل_Name) == 0 and
            (wildcardModule or strcmp(لیبل->import.moduleUtf8, i_moduleName) == 0)) {
            اگر (لیبل->type != ہدف->type) {
                واپس m3Err_incompatibleImportType;
            }

            لیبل->resolved = ہدف;
            نتیجہ        = m3Err_none;
        }
    }

    واپس نتیجہ;
}


uint32_t m3_Get_لیبل_Arg_گنتی (IM3_لیبل i_لیبل)
{
    واپس i_لیبل ? i_لیبل->type->عدد_دلائل : 0;
}


M3_قدر_Type m3_Get_لیبل_Arg_Type (IM3_لیبل i_لیبل, uint32_t i_اشاریہ)
{
    اگر (i_لیبل and i_اشاریہ < i_لیبل->type->عدد_دلائل) {
        واپس (M3_قدر_Type)بنیاد_Type_کا(d_FuncArgType(i_لیبل->type, i_اشاریہ));
    }

    واپس c_m3Type_none;
}

#ورنہ

IM3_لیبل m3_ڈھونڈو_لیبل (IM3Module i_module, ثابت حرف* ثابت i_لیبل_Name)
{
    واپس تہی;
}

M3_نتیجہ m3_New_لیبل (IM3Runtime io_runtime, IM3_لیبل* o_لیبل, ثابت حرف* ثابت i_signature)
{
    اگر (o_لیبل) {
        *o_لیبل = تہی;
    }
    واپس m3_Err_لیبل_Lookup_Failed;
}

M3_نتیجہ m3_Link_لیبل (IM3Module io_module, ثابت حرف* ثابت i_moduleName, ثابت حرف* ثابت i_لیبل_Name, IM3_لیبل i_لیبل)
{
    واپس m3_Err_لیبل_Lookup_Failed;
}

uint32_t m3_Get_لیبل_Arg_گنتی (IM3_لیبل i_لیبل)
{
    واپس 0;
}

M3_قدر_Type m3_Get_لیبل_Arg_Type (IM3_لیبل i_لیبل, uint32_t i_اشاریہ)
{
    واپس c_m3Type_none;
}

#اگر_ختم // d_m3HasExceptionHandling || d_m3HasStackSwitching


#اگر d_m3HasExceptionHandling

M3_نتیجہ m3_ThrowException (IM3Runtime io_runtime, IM3_لیبل i_لیبل, uint32_t i_argc, ثابت خالی* i_argptrs[])
{
    اگر (not io_runtime or not i_لیبل) {
        واپس m3_Err_لیبل_Lookup_Failed;
    }

    IM3_لیبل      لیبل  = i_لیبل->resolved ? i_لیبل->resolved : i_لیبل;
    IM3FuncType type = لیبل->type;

    // a tag with results is a stack switching control tag, which is suspended
    // with rather than thrown
    اگر (type->عدد_Rets) {
        واپس m3Err_typeMismatch;
    }
    اگر (i_argc != type->عدد_دلائل) {
        واپس m3_Err_دلیل_گنتی_Mismatch;
    }
    // with nothing on the stack there is nothing to unwind, and the exception
    // would sit on the runtime's list until some later call ended
    اگر (io_runtime->callNesting == 0) {
        واپس "m3_ThrowException called outside a host function";
    }

    M3Exception* exception = NewException(io_runtime, لیبل, i_argc);
    اگر (not exception) {
        واپس m3Err_mallocFailed;
    }

    // the layout op_Throw gives a payload: 32-bit values zero-extended, floats
    // as their bit pattern
    برائے (u32 i = 0; i < i_argc; ++i) {
        ثابت خالی* قدر = i_argptrs[i];

        انتخاب (بنیاد_Type_کا(d_FuncArgType(type, i))) {
        حالت c_m3Type_i32:
        حالت c_m3Type_f32: {
            u32 بٹ;
            حافظہ_نقل(&بٹ, قدر, حجم(بٹ));
            exception->دلائل[i] = بٹ;
            توڑو;
        }
        حالت c_m3Type_i64:
        حالت c_m3Type_f64:
            حافظہ_نقل(&exception->دلائل[i], قدر, حجم(u64));
            توڑو;
        حالت c_m3Type_funcref:
        حالت c_m3Type_externref:
        حالت c_m3Type_exnref:
        حالت c_m3Type_contref:
            exception->دلائل[i] = (u64)(*(ثابت uintptr_t*)قدر);
            توڑو;
        طےشدہ:
            FreeException(io_runtime, exception);
            واپس "unknown argument type";
        }
    }

    io_runtime->pendingException = exception;

    واپس m3Err_pendingException;
}


IM3_لیبل m3_Get_Exception_لیبل (IM3Runtime i_runtime)
{
    اگر (i_runtime and i_runtime->uncaughtException) {
        واپس i_runtime->uncaughtException->لیبل;
    }

    واپس تہی;
}


M3_نتیجہ m3_Get_Exception_دلائل (IM3Runtime i_runtime, uint32_t i_argc, ثابت خالی* o_argptrs[])
{
    اگر (not i_runtime or not i_runtime->uncaughtException) {
        واپس "no uncaught exception";
    }

    ثابت M3Exception* exception = i_runtime->uncaughtException;
    IM3FuncType        type      = exception->لیبل->type;

    اگر (i_argc != exception->عدد_دلائل) {
        واپس m3_Err_دلیل_گنتی_Mismatch;
    }

    برائے (u32 i = 0; i < i_argc; ++i) {
        خالی* قدر = (خالی*)o_argptrs[i];
        u64   بٹ  = exception->دلائل[i];

        انتخاب (بنیاد_Type_کا(d_FuncArgType(type, i))) {
        حالت c_m3Type_i32:
        حالت c_m3Type_f32: {
            u32 کم = (u32)بٹ;
            حافظہ_نقل(قدر, &کم, حجم(کم));
            توڑو;
        }
        حالت c_m3Type_i64:
        حالت c_m3Type_f64:
            حافظہ_نقل(قدر, &بٹ, حجم(بٹ));
            توڑو;
        حالت c_m3Type_funcref:
        حالت c_m3Type_externref:
        حالت c_m3Type_exnref:
        حالت c_m3Type_contref:
            *(uintptr_t*)قدر = (uintptr_t)بٹ;
            توڑو;
        طےشدہ:
            واپس "unknown argument type";
        }
    }

    واپس m3Err_none;
}

#ورنہ

M3_نتیجہ m3_ThrowException (IM3Runtime io_runtime, IM3_لیبل i_لیبل, uint32_t i_argc, ثابت خالی* i_argptrs[])
{
    واپس m3_Err_لیبل_Lookup_Failed;
}

IM3_لیبل m3_Get_Exception_لیبل (IM3Runtime i_runtime)
{
    واپس تہی;
}

M3_نتیجہ m3_Get_Exception_دلائل (IM3Runtime i_runtime, uint32_t i_argc, ثابت خالی* o_argptrs[])
{
    واپس "no uncaught exception";
}

#اگر_ختم // d_m3HasExceptionHandling


#اگر d_m3_Guarded_حافظہ

// What RunCodeProtected hands to the platform layer to run, and what comes back
قسمتعریف ساخت M3_Protected_تسلسل {
    pc_t            pc;
    IM3Runtime      runtime;
    M3_حافظہ_سرخی* mem;
    M3_نتیجہ        نتیجہ;
} M3_Protected_تسلسل;

ساکن
خالی تسلسل_رمز_Body (خالی* io_context)
{
    M3_Protected_تسلسل* تسلسل = (M3_Protected_تسلسل*)io_context;

#  اگر (d_m3EnableOpProfiling || d_m3EnableOpTracing)
    تسلسل->نتیجہ = (M3_نتیجہ)تسلسل_رمز(تسلسل->pc, (m3stack_t)تسلسل->runtime->ڈھیر, تسلسل->mem,
                                    d_m3_Op_Default_دلائل, d_m3_بنیاد_Cstr);
#  ورنہ
    تسلسل->نتیجہ = (M3_نتیجہ)تسلسل_رمز(تسلسل->pc, (m3stack_t)تسلسل->runtime->ڈھیر, تسلسل->mem,
                                    d_m3_Op_Default_دلائل);
#  endif
}

// The bounds check the load and store operations no longer do, done once here by the
// system instead: an address past the end of a memory reaches the uncommitted part
// of that memory's reservation, and the fault comes back as the trap the spec asks
// for. Nothing is left half-done by it - the Wasm stack and the memories are heap,
// and the caller unwinds the rest.
ساکن
M3_نتیجہ تسلسل_رمز_Protected (IM3Runtime i_runtime, pc_t i_pc, M3_حافظہ_سرخی* i_mem)
{
    // Memory operations in this build carry no bounds check, so code only runs where a
    // fault past the end of a memory can still be turned into a trap
    اگر (M3_UNLIKELY(not m3_Host_Guards_فعال())) {
        واپس ErrorRuntime(m3_Err_حافظہ_Guards_Unavailable, i_runtime,
                            "another part of the process holds the fault signal");
    }

    M3_Protected_تسلسل تسلسل;
    تسلسل.pc      = i_pc;
    تسلسل.runtime = i_runtime;
    تسلسل.mem     = i_mem;
    تسلسل.نتیجہ  = m3Err_none;

    خالی*  arena;
    size_t arena_بائٹیں;
    Guard_ArenaRange(&arena, &arena_بائٹیں);

    اگر (not m3_HostProtectedCall(تسلسل_رمز_Body, &تسلسل, arena, arena_بائٹیں)) {
        واپس m3_Err_trap_برآمد_کا_Bounds_حافظہ_Access;
    }

    واپس تسلسل.نتیجہ;
}

#اگر_ختم // d_m3GuardedMemory


#اگر d_m3_Has_ڈھیر_Switching

// The function a suspended continuation stopped in: the callee of its innermost
// call frame, or the function it was entered with if it made no call. Frames are
// recorded innermost first, so that is the lowest-numbered call frame.
ساکن
IM3Function Continuation_InnermostFunction (IM3Continuation i_cont)
{
    برائے (u32 i = 0; i < i_cont->عدد_چوکھٹے; ++i) {
        اگر (i_cont->چوکھٹے[i].قسم == چوکھٹا_call && i_cont->چوکھٹے[i].call.function) {
            واپس i_cont->چوکھٹے[i].call.function;
        }
    }

    واپس i_cont->اندراج_Function;
}


// Where the host's next call puts its arguments and finds its results. A
// suspended root keeps its frames on the bottom of the runtime's stack, so a call
// made before it is resumed starts above the last of them rather than on top of
// them. The top is the innermost function's frame base plus everything that
// function can use. That base is the root's own sp, unless it stopped inside a
// resume: then the suspension point belongs to the resumed continuation, and the
// root's innermost frame is that resume.
خالی Runtime_رکھو_Call_ڈھیر (IM3Runtime io_runtime)
{
    IM3Continuation جڑ = io_runtime->جڑ_Continuation;

    io_runtime->ڈھیر = io_runtime->origin_ڈھیر;

    اگر (not جڑ or جڑ->state != cont_suspended) {
        واپس;
    }

    m3stack_t   sp       = جڑ->sp;
    IM3Function function = Continuation_InnermostFunction(جڑ);

    اگر (جڑ->عدد_چوکھٹے and جڑ->چوکھٹے[0].قسم == چوکھٹا_resume) {
        sp = جڑ->چوکھٹے[0].sp;
    }

    m3slot_t* بنیاد = (m3slot_t*)io_runtime->origin_ڈھیر;
    m3slot_t* چوٹی  = (m3slot_t*)sp + (function ? function->max_ڈھیر_Slots : 0);

    اگر (چوٹی > بنیاد + io_runtime->عدد_ڈھیر_Slots) {
        چوٹی = بنیاد + io_runtime->عدد_ڈھیر_Slots;
    }

    io_runtime->ڈھیر = چوٹی;
}

#اگر_ختم // d_m3HasStackSwitching


// Run compiled code on the runtime's stack, bounding native recursion for the
// duration of the call. The outermost invocation establishes the stack limit;
// nested ones (an imported function calling back into Wasm) inherit it.
ساکن اندرونی
M3_نتیجہ تسلسل_رمز_Checked (IM3Runtime i_runtime, IM3Function i_function)
{
    pc_t i_pc = i_function->compiled;

    // execution runs against the memory of the module the entry point belongs
    // to, not against some runtime-wide one
    M3_حافظہ_سرخی* _mem = Module_حافظہ_سرخی(i_function->module);

#اگر d_m3HasSnapshots
    اگر (M3_UNLIKELY(i_function->module->isUnusable)) {
        واپس "a snapshot failed to restore into the module, so it cannot run";
    }
    i_function->module->has_تسلسل = true;
#اگر_ختم

    d_m3_ڈھیر_حد_بندی_Enter(i_runtime);

    // a host function calling back into Wasm does not change which call a trap
    // ends: that is still the one the host made first
    اگر (i_runtime->callNesting++ == 0) {
        i_runtime->entered = i_function;
    }
#اگر d_m3HasExceptionHandling
    // handler stacks don't nest across a call boundary: a host function calling
    // back into Wasm cannot be caught by a try_table its own caller entered
    u32 saved_Try_گہرائی   = i_runtime->try_گہرائی;
    i_runtime->try_گہرائی = 0;
#اگر_ختم
#اگر d_m3_Has_ڈھیر_Switching
    // nor do continuations: the native frames on the far side of a host call
    // are not ours to record, so a suspend in here has no handler to reach and
    // no resume to unwind to
    IM3Continuation savedContinuation = i_runtime->فعال_Continuation;

    // A suspended root continuation is a paused program waiting to be resumed,
    // not scratch space. A call the host makes before resuming it gets a
    // context of its own, the way any other re-entry does, rather than having
    // the frames of the suspension reset out from under it.
    bool claim_جڑ = i_runtime->isSuspendable and not savedContinuation and
                     not (i_runtime->جڑ_Continuation and
                          i_runtime->جڑ_Continuation->state == cont_suspended);

    اگر (claim_جڑ and not i_runtime->جڑ_Continuation) {
        i_runtime->جڑ_Continuation = Continuation_New(i_runtime, i_function->funcType, i_function);

        اگر (i_runtime->جڑ_Continuation) {
            // The root borrows the runtime's stack and is not charged as cont.new.
            i_runtime->جڑ_Continuation->عدد_ڈھیر_Slots = i_runtime->عدد_ڈھیر_Slots;
        } ورنہ {
            claim_جڑ = false;
        }
    }

    اگر (claim_جڑ) {
        i_runtime->فعال_Continuation              = i_runtime->جڑ_Continuation;
        i_runtime->جڑ_Continuation->اندراج_Function = i_function;
        i_runtime->جڑ_Continuation->state         = cont_running;
        i_runtime->جڑ_Continuation->عدد_چوکھٹے     = 0;
    } ورنہ {
        i_runtime->فعال_Continuation = تہی;
    }
#اگر_ختم
#اگر d_m3_Guarded_حافظہ
    M3_نتیجہ نتیجہ = تسلسل_رمز_Protected(i_runtime, i_pc, _mem);
#وگرنہ_اگر (d_m3EnableOpProfiling || d_m3EnableOpTracing)
    M3_نتیجہ نتیجہ = (M3_نتیجہ)تسلسل_رمز(i_pc, (m3stack_t)i_runtime->ڈھیر, _mem, d_m3_Op_Default_دلائل, d_m3_بنیاد_Cstr);
#ورنہ
    M3_نتیجہ نتیجہ = (M3_نتیجہ)تسلسل_رمز(i_pc, (m3stack_t)i_runtime->ڈھیر, _mem, d_m3_Op_Default_دلائل);
#اگر_ختم
#اگر d_m3_Has_ڈھیر_Switching
    اگر (claim_جڑ) {
        i_runtime->جڑ_Continuation->state =
          (نتیجہ == m3Err_continuationSuspended) ? cont_suspended : cont_returned;

        Runtime_رکھو_Call_ڈھیر(i_runtime);
    }
    i_runtime->فعال_Continuation = savedContinuation;
#اگر_ختم
#اگر d_m3HasExceptionHandling
    i_runtime->try_گہرائی = saved_Try_گہرائی;

    // an exception that reached the bottom of the call stack found no handler
    اگر (M3_UNLIKELY(نتیجہ == m3Err_pendingException)) {
        نتیجہ = m3Err_trapUncaughtException;
    }
    ریکارڈ_Uncaught_Exception(i_runtime, نتیجہ);

    // a paused invocation can still reach the exceptions it holds, so they wait
    // for it to finish
    اگر (i_runtime->callNesting == 1 and not m3_IsSuspended(i_runtime)) {
        FreeExceptions(i_runtime);
    }
#اگر_ختم
    i_runtime->callNesting--;
    d_m3_ڈھیر_حد_بندی_Leave(i_runtime);

    واپس نتیجہ;
}

M3_نتیجہ m3_تسلسل_آغاز (IM3Module io_module)
{
#اگر_تعریف_شدہ FUZZING_BUILD_منوال_UNSAFE_FOR_PRODUCTION
    // Execution disabled for fuzzing builds
    واپس m3Err_none;
#اگر_ختم

    M3_نتیجہ نتیجہ           = m3Err_none;
    i32      آغاز_Function_Tmp = -1;

    اگر (io_module and io_module->آغاز_Function >= 0) {
        IM3Function function = &io_module->functions[io_module->آغاز_Function];

        اگر (not function->compiled) {
_           (CompileFunction(function));
        }

        IM3FuncType ftype = function->funcType;
        اگر (ftype->عدد_دلائل != 0 || ftype->عدد_Rets != 0) {
            _throw(m3_Err_دلیل_گنتی_Mismatch);
        }

        IM3Module  module  = function->module;
        IM3Runtime runtime = module->runtime;

        آغاز_Function_Tmp = io_module->آغاز_Function;

        io_module->آغاز_Function = -1;

        نتیجہ = تسلسل_رمز_Checked(runtime, function);

        // a start function that suspended is not to be started again: resuming
        // is what finishes it
        اگر (نتیجہ and نتیجہ != m3Err_continuationSuspended) {
            io_module->آغاز_Function = آغاز_Function_Tmp;
            EXCEPTION_PRINT(نتیجہ);
            جاؤ _catch;
        }
    }

    _catch: واپس نتیجہ;
}

// TODO: deal with main + side-modules loading efforcement
M3_نتیجہ m3_بھرو_Module (IM3Runtime io_runtime, IM3Module io_module)
{
    M3_نتیجہ نتیجہ = m3Err_none;

    اگر (M3_UNLIKELY(io_module->runtime)) {
        واپس m3Err_moduleAlreadyLinked;
    }

    io_module->runtime = io_runtime;

    // linking first: a memory import has to be resolved before InitMemory would
    // give it pages of its own, and a global import before an initializer reads it
_   (LinkImports(io_runtime, io_module));

_   (Init_حافظہ(io_runtime, io_module));
_   (InitGlobals(io_module));
    // Spec order: element segments are applied before data segments. It matters
    // when one of them traps - whatever ran before the trap stays done.
_   (Init_جدول_And_Elements(io_module));
_   (Init_ڈیٹا_Segments(io_module));

    // Start func might use imported functions, which are not liked here yet,
    // so it will be called before a function call is attempted (in m3_FindFunction)

#اگر_تعریف_شدہ DEBUG
    Module_Generate_نام(io_module);
#اگر_ختم

    io_module->اگلا     = io_runtime->modules;
    io_runtime->modules = io_module;
    واپس نتیجہ; // ok

_catch:
    // The runtime owns the module either way. Instantiation may already have
    // written this module's functions into a table another module owns, and the
    // spec keeps whatever it managed to do before the trap, so those entries
    // stay callable - retaining the module is what stops them dangling.
    //
    // It goes on the tail of the list rather than the head: it is not a module
    // anyone should find by name, only one whose functions may still be
    // reachable through someone else's table.
    io_module->اگلا = تہی;

    IM3Module* دم = &io_runtime->modules;
    جب (*دم) {
        دم = &(*دم)->اگلا;
    }
    *دم = io_module;

    واپس نتیجہ;
}

IM3Global m3_ڈھونڈو_Global (IM3Module         io_module,
                         ثابت حرف* ثابت i_globalName)
{
    // Search exports
    IM3Global exported = Module_ڈھونڈو_Exported_Global(io_module, i_globalName);
    اگر (exported) {
        // a re-exported global is the one that was imported, so reads and
        // writes have to reach the cell that actually holds the value
        واپس exported->resolved ? exported->resolved : exported;
    }

    // Search imports
    برائے (u32 i = 0; i < io_module->عدد_Globals; ++i) {
        IM3Global g = &io_module->globals[i];

        اگر (g->import.moduleUtf8 and g->import.میدان_Utf8) {
            اگر (strcmp(g->import.میدان_Utf8, i_globalName) == 0) {
                واپس g->resolved ? g->resolved : g;
            }
        }
    }
    واپس تہی;
}

M3_نتیجہ m3_GetGlobal (IM3Global      i_global,
                       IM3_Tagged_قدر o_قدر)
{
    اگر (not i_global) {
        واپس m3Err_globalLookupFailed;
    }

    انتخاب (i_global->type) {
    حالت c_m3Type_i32: o_قدر->قدر.i32 = i_global->i32_قدر; توڑو;
    حالت c_m3Type_i64: o_قدر->قدر.i64 = i_global->i64_قدر; توڑو;
#اگر d_m3HasFloat
    حالت c_m3Type_f32: o_قدر->قدر.f32 = i_global->f32_قدر; توڑو;
    حالت c_m3Type_f64: o_قدر->قدر.f64 = i_global->f64_قدر; توڑو;
#اگر_ختم
    طےشدہ: واپس m3Err_invalidTypeId;
    }

    o_قدر->type = (M3_قدر_Type)(i_global->type);
    واپس m3Err_none;
}

M3_نتیجہ m3_سیٹ_Global (IM3Global            i_global,
                       ثابت IM3_Tagged_قدر i_قدر)
{
    اگر (not i_global) {
        واپس m3Err_globalLookupFailed;
    }
    اگر (not i_global->isMutable) {
        واپس m3Err_globalNotMutable;
    }
    اگر (i_global->type != i_قدر->type) {
        واپس m3Err_globalTypeMismatch;
    }

    انتخاب (i_قدر->type) {
    حالت c_m3Type_i32: i_global->i32_قدر = i_قدر->قدر.i32; توڑو;
    حالت c_m3Type_i64: i_global->i64_قدر = i_قدر->قدر.i64; توڑو;
#اگر d_m3HasFloat
    حالت c_m3Type_f32: i_global->f32_قدر = i_قدر->قدر.f32; توڑو;
    حالت c_m3Type_f64: i_global->f64_قدر = i_قدر->قدر.f64; توڑو;
#اگر_ختم
    طےشدہ: واپس m3Err_invalidTypeId;
    }

    واپس m3Err_none;
}

M3_قدر_Type m3_GetGlobalType (IM3Global i_global)
{
    واپس (i_global) ? (M3_قدر_Type)(i_global->type) : c_m3Type_none;
}


خالی* v_ڈھونڈو_Function (IM3Module i_module, خالی* i_info)
{
    ثابت حرف* ثابت i_name = (ثابت حرف*)i_info;

    // Prefer exported functions
    IM3Function exported = Module_ڈھونڈو_Exported_Function(i_module, i_name);
    اگر (exported) {
        واپس exported;
    }

    // Search internal functions
    برائے (u32 i = 0; i < i_module->عدد_Functions; ++i) {
        IM3Function f = &i_module->functions[i];

        bool isImported = f->import.moduleUtf8 or f->import.میدان_Utf8;

        اگر (isImported) {
            جاری;
        }

        برائے (صحیح j = 0; j < f->عدد_نام; j++) {
            اگر (f->نام[j] and strcmp(f->نام[j], i_name) == 0) {
                واپس f;
            }
        }
    }

    واپس تہی;
}


// Shared tail of the two lookups: a function is only usable once it has code.
ساکن
M3_نتیجہ Prepare_ملا_Function (IM3Function* o_function, IM3Function i_function)
{
    M3_نتیجہ نتیجہ = m3Err_none;

    اگر (not i_function->compiled) {
_       (CompileFunction(i_function))
    }

    _catch:
    *o_function = نتیجہ ? تہی : i_function;

    واپس نتیجہ;
}


// Searches every module in the runtime, most recently loaded first. That is a
// guess once more than one module is loaded and two of them export the same
// name - m3_FindFunctionIn says which module is meant.
M3_نتیجہ m3_ڈھونڈو_Function (IM3Function* o_function, IM3Runtime i_runtime, ثابت حرف* ثابت i_functionName)
{
                                                                d_m3Assert (o_function and i_runtime and i_functionName);
    IM3Function function = تہی;

    اگر (not i_runtime->modules) {
        *o_function = تہی;
        واپس "no modules loaded";
    }

    function = (IM3Function)ForEachModule(i_runtime, v_ڈھونڈو_Function, (خالی*)i_functionName);

    اگر (not function) {
        *o_function = تہی;
        واپس ErrorModule(m3Err_functionLookupFailed, i_runtime->modules, "'%s'", i_functionName);
    }

    واپس Prepare_ملا_Function(o_function, function);
}


IM3Module m3_ڈھونڈو_Module (IM3Runtime i_runtime, ثابت حرف* ثابت i_moduleName)
{
    اگر (not i_runtime or not i_moduleName) {
        واپس تہی;
    }

    // the list is newest-first, so a name registered twice names the newer one
    برائے (IM3Module m = i_runtime->modules; m; m = m->اگلا) {
        اگر (m->name and strcmp(m->name, i_moduleName) == 0) {
            واپس m;
        }
    }

    واپس تہی;
}


// Searches one module's exports, which is what naming a module means.
M3_نتیجہ m3_ڈھونڈو_Function_اندر (IM3Function* o_function, IM3Module i_module, ثابت حرف* ثابت i_functionName)
{
                                                                d_m3Assert (o_function and i_functionName);
    اگر (not i_module) {
        *o_function = تہی;
        واپس m3Err_functionLookupFailed;      // ErrorModule would deref it
    }

    IM3Function function = (IM3Function)v_ڈھونڈو_Function(i_module, (خالی*)i_functionName);

    اگر (not function) {
        *o_function = تہی;
        واپس ErrorModule(m3Err_functionLookupFailed, i_module, "'%s'", i_functionName);
    }

    واپس Prepare_ملا_Function(o_function, function);
}


M3_نتیجہ m3_Get_جدول_Function (IM3Function* o_function, IM3Module i_module, uint32_t i_اشاریہ)
{
_try {
    M3_جدول*    جدول_2;
    IM3Function function;

    _throwif("no table", i_module->عدد_Tables == 0);

    جدول_2 = i_module->tables[0];
    _throwif("function index out of range", i_اشاریہ >= جدول_2->سائز);

    function = (IM3Function)جدول_2->elements[i_اشاریہ];

    اگر (function) {
        اگر (not function->compiled) {
_           (CompileFunction(function))
        }
    }

    *o_function = function;
} _catch:
    واپس نتیجہ;
}


ساکن
M3_نتیجہ check_آغاز_Function (IM3Module i_module)
{
    M3_نتیجہ نتیجہ = m3Err_none;                               d_m3Assert(i_module);

    // Check if start function needs to be called
    اگر (i_module->آغاز_Function >= 0) {
        نتیجہ = m3_تسلسل_آغاز(i_module);
    }

    واپس نتیجہ;
}

uint32_t m3_Get_Arg_گنتی (IM3Function i_function)
{
    اگر (i_function) {
        IM3FuncType ft = i_function->funcType;
        اگر (ft) {
            واپس ft->عدد_دلائل;
        }
    }
    واپس 0;
}

uint32_t m3_Get_Ret_گنتی (IM3Function i_function)
{
    اگر (i_function) {
        IM3FuncType ft = i_function->funcType;
        اگر (ft) {
            واپس ft->عدد_Rets;
        }
    }
    واپس 0;
}


M3_قدر_Type m3_GetArgType (IM3Function i_function, uint32_t index)
{
    اگر (i_function) {
        IM3FuncType ft = i_function->funcType;
        اگر (ft and index < ft->عدد_دلائل) {
            واپس (M3_قدر_Type)بنیاد_Type_کا(d_FuncArgType(ft, index));
        }
    }
    واپس c_m3Type_none;
}

M3_قدر_Type m3_GetRetType (IM3Function i_function, uint32_t index)
{
    اگر (i_function) {
        IM3FuncType ft = i_function->funcType;
        اگر (ft and index < ft->عدد_Rets) {
            واپس (M3_قدر_Type)بنیاد_Type_کا(d_FuncRetType(ft, index));
        }
    }
    واپس c_m3Type_none;
}


// The call APIs marshal their arguments into the runtime stack before any
// compiled code runs, so op_Entry's overflow check comes too late to cover those
// writes - and for a host function or an import linked to another module there
// is no op_Entry of our own to reach at all. Bound the area about to be written
// here instead: results and arguments each take a u64 at this boundary, however
// wide the slot is inside. Every caller goes through this, so none of them can
// be the one that forgets.
ساکن
M3_نتیجہ Get_ڈھیر_اشارہ_گر_For_دلائل (IM3Function i_function, u8** o_ڈھیر)
{
    IM3Runtime  runtime = i_function->module->runtime;
    IM3FuncType ftype   = i_function->funcType;

    // the stack a paused invocation holds is not available to this one
    size_t مستعمل      = (size_t)((u8*)runtime->ڈھیر - (u8*)runtime->origin_ڈھیر);
    size_t needed    = ((size_t)ftype->عدد_Rets + ftype->عدد_دلائل) * حجم(u64);
    size_t دستیاب = (size_t)runtime->عدد_ڈھیر_Slots * حجم(m3slot_t) - مستعمل;

    اگر (needed > دستیاب) {
        واپس m3_Err_trap_ڈھیر_حد_سے_تجاوز;
    }

    u64* ڈھیر = (u64*)runtime->ڈھیر;

    ڈھیر += ftype->عدد_Rets;

    *o_ڈھیر = (u8*)ڈھیر;

    واپس m3Err_none;
}


M3_نتیجہ m3_CallV (IM3Function i_function, ...)
{
    va_list ap;
    va_start(ap, i_function);
    M3_نتیجہ r = m3_CallVL(i_function, ap);
    va_end(ap);
    واپس r;
}

ساکن
خالی Report_Native_ڈھیر_Usage ()
{
#اگر d_m3_Log_Native_ڈھیر
    صحیح ڈھیر_مستعمل = m3_ڈھیر_Get_Max();
    fprintf(stderr, "Native stack used: %d\n", ڈھیر_مستعمل);
#اگر_ختم
}


M3_نتیجہ m3_CallVL (IM3Function i_function, va_list i_دلائل)
{
    IM3Runtime  runtime = i_function->module->runtime;
    IM3FuncType ftype   = i_function->funcType;
    M3_نتیجہ    نتیجہ  = m3Err_none;
    u8*         s       = تہی;

    اگر (!i_function->compiled) {
        واپس m3_Err_missing_Compiled_رمز;
    }

#اگر d_m3_ریکارڈ_Backtraces
    ClearBacktrace(runtime);
#اگر_ختم

    m3_ڈھیر_Check_Init();

_   (check_آغاز_Function(i_function->module))

_   (Get_ڈھیر_اشارہ_گر_For_دلائل(i_function, &s))

    برائے (u32 i = 0; i < ftype->عدد_دلائل; ++i) {
        انتخاب (d_FuncArgType(ftype, i)) {
        حالت c_m3Type_i32:
            *(i32*)(s) = va_arg(i_دلائل, i32);
            s += 8;
            توڑو;
        حالت c_m3Type_i64:
            *(i64*)(s) = va_arg(i_دلائل, i64);
            s += 8;
            توڑو;
        حالت c_m3Type_funcref:
        حالت c_m3Type_externref:
        حالت c_m3Type_exnref:
            *(uintptr_t*)(s) = va_arg(i_دلائل, uintptr_t);
            s += 8;
            توڑو;
#اگر d_m3HasFloat
        حالت c_m3Type_f32:
            *(f32*)(s) = (f32)va_arg(i_دلائل, f64);
            s += 8;
            توڑو; // f32 is passed as f64
        حالت c_m3Type_f64:
            *(f64*)(s) = va_arg(i_دلائل, f64);
            s += 8;
            توڑو;
#اگر_ختم
        طےشدہ: واپس "unknown argument type";
        }
    }

    نتیجہ = تسلسل_رمز_Checked(runtime, i_function);
    Report_Native_ڈھیر_Usage();

    runtime->آخری_Called = نتیجہ ? تہی : i_function;

    _catch: واپس نتیجہ;
}

M3_نتیجہ m3_Call (IM3Function i_function, uint32_t i_argc, ثابت خالی* i_argptrs[])
{
    IM3Runtime  runtime = i_function->module->runtime;
    IM3FuncType ftype   = i_function->funcType;
    M3_نتیجہ    نتیجہ  = m3Err_none;
    u8*         s       = تہی;

    اگر (i_argc != ftype->عدد_دلائل) {
        واپس m3_Err_دلیل_گنتی_Mismatch;
    }
    اگر (!i_function->compiled) {
        واپس m3_Err_missing_Compiled_رمز;
    }

#اگر d_m3_ریکارڈ_Backtraces
    ClearBacktrace(runtime);
#اگر_ختم

    m3_ڈھیر_Check_Init();

_   (check_آغاز_Function(i_function->module))

_   (Get_ڈھیر_اشارہ_گر_For_دلائل(i_function, &s))

    برائے (u32 i = 0; i < ftype->عدد_دلائل; ++i) {
        انتخاب (d_FuncArgType(ftype, i)) {
        حالت c_m3Type_i32:
            *(i32*)(s) = *(i32*)i_argptrs[i];
            s += 8;
            توڑو;
        حالت c_m3Type_i64:
            *(i64*)(s) = *(i64*)i_argptrs[i];
            s += 8;
            توڑو;
        حالت c_m3Type_funcref:
        حالت c_m3Type_externref:
        حالت c_m3Type_exnref:
            *(uintptr_t*)(s) = *(uintptr_t*)i_argptrs[i];
            s += 8;
            توڑو;
#اگر d_m3HasFloat
        حالت c_m3Type_f32:
            *(f32*)(s) = *(f32*)i_argptrs[i];
            s += 8;
            توڑو;
        حالت c_m3Type_f64:
            *(f64*)(s) = *(f64*)i_argptrs[i];
            s += 8;
            توڑو;
#اگر_ختم
        طےشدہ: واپس "unknown argument type";
        }
    }

    نتیجہ = تسلسل_رمز_Checked(runtime, i_function);

    Report_Native_ڈھیر_Usage();

    runtime->آخری_Called = نتیجہ ? تہی : i_function;

    _catch: واپس نتیجہ;
}

// Argument parsing for m3_CallArgv. Strict on purpose: the whole string has to
// be consumed, so "12abc" is rejected rather than read as 12, and an empty or
// unparsable argument is an error rather than the zero that strtoul with a
// NULL end pointer used to hand back. See wasm3/wasm3#367.
ساکن
M3_نتیجہ ParseArgInteger (ccstr_t i_arg, u32 i_عدد_بٹ, u64* o_قدر)
{
    اگر (not i_arg or not *i_arg) {
        واپس "empty argument";
    }

    // strtoull would skip leading space, but trailing space is rejected below;
    // accepting one and not the other would just be confusing
    اگر (خلا_ہے((غیرعلامتی حرف)*i_arg)) {
        واپس "argument is not a number";
    }

    حرف* انتہا = تہی;
    u64   قدر;

    errno = 0;

    // an argument may be spelled signed or unsigned: -1 and 4294967295 name the
    // same i32
    اگر (*i_arg == '-') {
        i64 signed_قدر = strtoll(i_arg, &انتہا, 10);

        اگر (i_عدد_بٹ == 32 and (signed_قدر < INT32_MIN or signed_قدر > INT32_MAX)) {
            واپس "argument out of range";
        }

        قدر = (u64)signed_قدر;
    } ورنہ {
        قدر = strtoull(i_arg, &انتہا, 10);

        اگر (i_عدد_بٹ == 32 and قدر > UINT32_MAX) {
            واپس "argument out of range";
        }
    }

    اگر (errno == ERANGE) {
        واپس "argument out of range";
    }

    اگر (انتہا == i_arg or *انتہا) {
        واپس "argument is not a number";
    }

    *o_قدر = قدر;

    واپس m3Err_none;
}


#اگر d_m3HasFloat
ساکن
M3_نتیجہ ParseArgFloat (ccstr_t i_arg, f64* o_قدر)
{
    اگر (not i_arg or not *i_arg) {
        واپس "empty argument";
    }

    اگر (خلا_ہے((غیرعلامتی حرف)*i_arg)) {
        واپس "argument is not a number";
    }

    حرف* انتہا = تہی;

    errno = 0;

    f64 قدر = strtod(i_arg, &انتہا);

    اگر (انتہا == i_arg or *انتہا) {
        واپس "argument is not a number";
    }

    // strtod reports underflow through ERANGE as well, and a denormal result is
    // perfectly usable, so only an overflow to infinity is out of range
    اگر (errno == ERANGE and (قدر > DBL_MAX or قدر < -DBL_MAX)) {
        واپس "argument out of range";
    }

    *o_قدر = قدر;

    واپس m3Err_none;
}
#اگر_ختم


// A reference argument is either the null reference or a host handle written as
// an integer.
ساکن
M3_نتیجہ ParseArgReference (ccstr_t i_arg, u64* o_قدر)
{
    اگر (i_arg and strcmp(i_arg, "null") == 0) {
        *o_قدر = 0;
        واپس m3Err_none;
    }

    واپس ParseArgInteger(i_arg, 64, o_قدر);
}


M3_نتیجہ m3_CallArgv (IM3Function i_function, uint32_t i_argc, ثابت حرف* i_argv[])
{
    IM3FuncType ftype   = i_function->funcType;
    IM3Runtime  runtime = i_function->module->runtime;
    M3_نتیجہ    نتیجہ  = m3Err_none;
    u8*         s       = تہی;

    اگر (i_argc != ftype->عدد_دلائل) {
        واپس m3_Err_دلیل_گنتی_Mismatch;
    }
    اگر (!i_function->compiled) {
        واپس m3_Err_missing_Compiled_رمز;
    }

#اگر d_m3_ریکارڈ_Backtraces
    ClearBacktrace(runtime);
#اگر_ختم

    m3_ڈھیر_Check_Init();

_   (check_آغاز_Function(i_function->module))

_   (Get_ڈھیر_اشارہ_گر_For_دلائل(i_function, &s))

    برائے (u32 i = 0; i < ftype->عدد_دلائل; ++i) {
        u64 قدر = 0;
#اگر d_m3HasFloat
        f64 fvalue = 0;
#اگر_ختم
        انتخاب (d_FuncArgType(ftype, i)) {
        حالت c_m3Type_i32:
_           (ParseArgInteger(i_argv[i], 32, &قدر)) * (i32*)(s) = (i32)قدر;
            s += 8;
            توڑو;
        حالت c_m3Type_i64:
_           (ParseArgInteger(i_argv[i], 64, &قدر)) * (i64*)(s) = (i64)قدر;
            s += 8;
            توڑو;
        حالت c_m3Type_funcref:
        حالت c_m3Type_externref:
        حالت c_m3Type_exnref:
_           (ParseArgReference(i_argv[i], &قدر)) * (uintptr_t*)(s) = (uintptr_t)قدر;
            s += 8;
            توڑو;
#اگر d_m3HasFloat
            // strtof would be less portable
        حالت c_m3Type_f32:
_           (ParseArgFloat(i_argv[i], &fvalue)) * (f32*)(s) = (f32)fvalue;
            s += 8;
            توڑو;
        حالت c_m3Type_f64:
_           (ParseArgFloat(i_argv[i], &fvalue)) * (f64*)(s) = fvalue;
            s += 8;
            توڑو;
#اگر_ختم
        طےشدہ: _throw("unknown argument type");
        }
    }

    نتیجہ = تسلسل_رمز_Checked(runtime, i_function);

    Report_Native_ڈھیر_Usage();

    runtime->آخری_Called = نتیجہ ? تہی : i_function;

    _catch: واپس نتیجہ;
}


//u8 * AlignStackPointerTo64Bits (const u8 * i_stack)
//{
//    uintptr_t ptr = (uintptr_t) i_stack;
//    return (u8 *) ((ptr + 7) & ~7);
//}


M3_نتیجہ m3_GetResults (IM3Function i_function, uint32_t i_retc, ثابت خالی* o_retptrs[])
{
    IM3FuncType ftype   = i_function->funcType;
    IM3Runtime  runtime = i_function->module->runtime;

    اگر (i_retc != ftype->عدد_Rets) {
        واپس m3_Err_دلیل_گنتی_Mismatch;
    }
    اگر (i_function != runtime->آخری_Called) {
        واپس "function not called";
    }

    u8* s = (u8*)runtime->ڈھیر;

    برائے (u32 i = 0; i < ftype->عدد_Rets; ++i) {
        انتخاب (d_FuncRetType(ftype, i)) {
        حالت c_m3Type_i32:
            *(i32*)o_retptrs[i] = *(i32*)(s);
            s += 8;
            توڑو;
        حالت c_m3Type_i64:
            *(i64*)o_retptrs[i] = *(i64*)(s);
            s += 8;
            توڑو;
        حالت c_m3Type_funcref:
        حالت c_m3Type_externref:
        حالت c_m3Type_exnref:
            *(uintptr_t*)o_retptrs[i] = *(uintptr_t*)(s);
            s += 8;
            توڑو;
#اگر d_m3HasFloat
        حالت c_m3Type_f32:
            *(f32*)o_retptrs[i] = *(f32*)(s);
            s += 8;
            توڑو;
        حالت c_m3Type_f64:
            *(f64*)o_retptrs[i] = *(f64*)(s);
            s += 8;
            توڑو;
#اگر_ختم
        طےشدہ: واپس "unknown return type";
        }
    }
    واپس m3Err_none;
}

M3_نتیجہ m3_GetResultsV (IM3Function i_function, ...)
{
    va_list ap;
    va_start(ap, i_function);
    M3_نتیجہ r = m3_GetResultsVL(i_function, ap);
    va_end(ap);
    واپس r;
}

M3_نتیجہ m3_GetResultsVL (IM3Function i_function, va_list o_rets)
{
    IM3Runtime  runtime = i_function->module->runtime;
    IM3FuncType ftype   = i_function->funcType;

    اگر (i_function != runtime->آخری_Called) {
        واپس "function not called";
    }

    u8* s = (u8*)runtime->ڈھیر;
    برائے (u32 i = 0; i < ftype->عدد_Rets; ++i) {
        انتخاب (d_FuncRetType(ftype, i)) {
        حالت c_m3Type_i32:
            *va_arg(o_rets, i32*) = *(i32*)(s);
            s += 8;
            توڑو;
        حالت c_m3Type_i64:
            *va_arg(o_rets, i64*) = *(i64*)(s);
            s += 8;
            توڑو;
        حالت c_m3Type_funcref:
        حالت c_m3Type_externref:
        حالت c_m3Type_exnref:
            *va_arg(o_rets, uintptr_t*) = *(uintptr_t*)(s);
            s += 8;
            توڑو;
#اگر d_m3HasFloat
        حالت c_m3Type_f32:
            *va_arg(o_rets, f32*) = *(f32*)(s);
            s += 8;
            توڑو;
        حالت c_m3Type_f64:
            *va_arg(o_rets, f64*) = *(f64*)(s);
            s += 8;
            توڑو;
#اگر_ختم
        طےشدہ: واپس "unknown argument type";
        }
    }
    واپس m3Err_none;
}

خالی Release_رمز_صفحہ_No_Track (IM3Runtime i_runtime, IM3_رمز_صفحہ i_رمز_صفحہ)
{
    اگر (i_رمز_صفحہ) {
        IM3_رمز_صفحہ* list;

        bool صفحہ_بھرا = (عدد_Free_سطریں(i_رمز_صفحہ) < d_m3_رمز_صفحہ_Free_سطریں_حد);
        اگر (صفحہ_بھرا) {
            list = &i_runtime->صفحات_بھرا;
        } ورنہ {
            list = &i_runtime->صفحات_Open;
        }

        ڈالو_رمز_صفحہ(list, i_رمز_صفحہ);                         m3log (emit, "release page: %d to queue: '%s'", i_رمز_صفحہ->info.sequence, صفحہ_بھرا ? "full" : "open")
    }
}


IM3_رمز_صفحہ Acquire_رمز_صفحہ_With_استعداد (IM3Runtime i_runtime, u32 i_min_سطر_گنتی)
{
    IM3_رمز_صفحہ صفحہ = Remove_رمز_صفحہ_کا_استعداد(&i_runtime->صفحات_Open, i_min_سطر_گنتی);

    اگر (not صفحہ) {
        صفحہ = Environment_Acquire_رمز_صفحہ(i_runtime->environment, i_min_سطر_گنتی);

        اگر (not صفحہ) {
            صفحہ = New_رمز_صفحہ(i_runtime, i_min_سطر_گنتی);
        }

        اگر (صفحہ) {
            i_runtime->عدد_رمز_صفحات++;
        }
    }

    اگر (صفحہ) {                                                  m3log (emit, "acquire page: %d", صفحہ->info.sequence);
        i_runtime->عدد_فعال_رمز_صفحات++;
    }

    واپس صفحہ;
}


IM3_رمز_صفحہ Acquire_رمز_صفحہ (IM3Runtime i_runtime)
{
    واپس Acquire_رمز_صفحہ_With_استعداد(i_runtime, d_m3_رمز_صفحہ_Free_سطریں_حد);
}


خالی Release_رمز_صفحہ (IM3Runtime i_runtime, IM3_رمز_صفحہ i_رمز_صفحہ)
{
    اگر (i_رمز_صفحہ) {
        Release_رمز_صفحہ_No_Track(i_runtime, i_رمز_صفحہ);
        i_runtime->عدد_فعال_رمز_صفحات--;

#اگر defined(DEBUG)
        u32 عدد_Open = گنتی_رمز_صفحات(i_runtime->صفحات_Open);
        u32 عدد_بھرا = گنتی_رمز_صفحات(i_runtime->صفحات_بھرا);

        m3log(runtime, "runtime: %p; open-pages: %d; full-pages: %d; active: %d; total: %d", i_runtime, عدد_Open, عدد_بھرا, i_runtime->عدد_فعال_رمز_صفحات, i_runtime->عدد_رمز_صفحات);

        d_m3Assert(عدد_Open + عدد_بھرا + i_runtime->عدد_فعال_رمز_صفحات == i_runtime->عدد_رمز_صفحات);

#  اگر d_m3_Log_رمز_صفحات
        dump_رمز_صفحہ(i_رمز_صفحہ, /* startPC: */ تہی);
#  endif
#اگر_ختم
    }
}


#اگر d_m3VerboseErrorMessages
M3_نتیجہ m3Error (M3_نتیجہ i_نتیجہ, IM3Runtime i_runtime, IM3Module i_module, IM3Function i_function,
                  ثابت حرف* ثابت i_file, u32 i_سطر_عدد, ثابت حرف* ثابت i_error_پیغام, ...)
{
    اگر (i_runtime) {
        i_runtime->error         = (M3ErrorInfo){ .نتیجہ = i_نتیجہ, .runtime = i_runtime, .module = i_module, .function = i_function, .file = i_file, .سطر_2 = i_سطر_عدد };
        i_runtime->error.پیغام = i_runtime->error_پیغام;

        va_list دلائل;
        va_start(دلائل, i_error_پیغام);
        vsnprintf(i_runtime->error_پیغام, حجم(i_runtime->error_پیغام), i_error_پیغام, دلائل);
        va_end(دلائل);
    }

    واپس i_نتیجہ;
}
#اگر_ختم


خالی m3_GetErrorInfo (IM3Runtime i_runtime, M3ErrorInfo* o_info)
{
    اگر (i_runtime) {
        *o_info = i_runtime->error;
        m3_ResetErrorInfo(i_runtime);
    }
}


خالی m3_ResetErrorInfo (IM3Runtime i_runtime)
{
    اگر (i_runtime) {
        M3_INIT(i_runtime->error);
        i_runtime->error.پیغام = "";
    }
}

uint8_t* m3_Get_حافظہ (IM3Module i_module, size_t* o_حافظہ_سائز_اندر_بائٹیں, uint32_t i_حافظہ_اشاریہ)
{
    uint8_t* حافظہ = تہی;
    size_t   سائز   = 0;

    اگر (i_module and i_حافظہ_اشاریہ < i_module->عدد_Memories) {
        IM3_حافظہ mem = i_module->memories[i_حافظہ_اشاریہ];

        اگر (mem->mallocated) {
            سائز = mem->mallocated->لمبائی;

            اگر (سائز) {
                حافظہ = m3_Mem_ڈیٹا(mem->mallocated);
            }
        }
    }

    اگر (o_حافظہ_سائز_اندر_بائٹیں) {
        *o_حافظہ_سائز_اندر_بائٹیں = سائز;
    }

    واپس حافظہ;
}


size_t m3_Get_حافظہ_سائز (IM3Module i_module, uint32_t i_حافظہ_اشاریہ)
{
    اگر (not i_module or i_حافظہ_اشاریہ >= i_module->عدد_Memories) {
        واپس 0;
    }

    IM3_حافظہ mem = i_module->memories[i_حافظہ_اشاریہ];

    واپس mem->mallocated ? mem->mallocated->لمبائی : 0;
}


size_t m3_Get_حافظہ_سائز_At (ثابت خالی* i_حافظہ)
{
    اگر (not i_حافظہ) {
        واپس 0;
    }

    // the header sits immediately before the data it describes
    ثابت M3_حافظہ_سرخی* سرخی = ((ثابت M3_حافظہ_سرخی*)i_حافظہ) - 1;

    واپس سرخی->لمبائی;
}


M3_نتیجہ m3_ڈھونڈو_Exported_حافظہ (IM3Module i_module, cstr_t i_name, u32* o_حافظہ_اشاریہ)
{
    اگر (not i_module or not i_name or not o_حافظہ_اشاریہ) {
        واپس m3_Err_unknown_حافظہ;
    }

    ثابت M3Export* e = Module_ڈھونڈو_Export(i_module, d_external_قسم_حافظہ, i_name);

    اگر (not e) {
        واپس m3_Err_unknown_حافظہ;
    }

    *o_حافظہ_اشاریہ = e->index;
    واپس m3Err_none;
}


bool Module_HasLinkedHostImport (IM3Module i_module, cstr_t i_importModule)
{
    برائے (u32 i = 0; i < i_module->عدد_Functions; ++i) {
        IM3Function f = &i_module->functions[i];

        // hostMemory is set when a host function is bound to the import, so it
        // is also the mark of one - see CompileRawFunction
        اگر (not f->host_حافظہ or f->wasm or f->resolved) {
            جاری;
        }

        اگر (f->import.moduleUtf8 and strcmp(f->import.moduleUtf8, i_importModule) == 0) {
            واپس true;
        }
    }

    واپس false;
}


M3_نتیجہ m3_Bind_Import_حافظہ (IM3Module io_module, cstr_t i_importModule, u32 i_حافظہ_اشاریہ)
{
    اگر (not io_module or not i_importModule) {
        واپس m3Err_moduleNotLinked;
    }

    اگر (i_حافظہ_اشاریہ >= io_module->عدد_Memories) {
        واپس m3_Err_unknown_حافظہ;
    }

    // the memory itself, not the index: linking may have pointed this slot at
    // another module's memory, and growing one reallocates behind it
    IM3_حافظہ حافظہ = io_module->memories[i_حافظہ_اشاریہ];

    ثابت bool wildcardModule = (strcmp(i_importModule, "*") == 0);

    برائے (u32 i = 0; i < io_module->عدد_Functions; ++i) {
        IM3Function f = &io_module->functions[i];

        // only an import that a host function was bound to has a memory to
        // pin: one with a body of its own reads memory through the interpreter,
        // and one resolved to another module's export runs that module's code
        اگر (not f->host_حافظہ or f->wasm or f->resolved) {
            جاری;
        }

        اگر (wildcardModule or (f->import.moduleUtf8 and strcmp(f->import.moduleUtf8, i_importModule) == 0)) {
            f->host_حافظہ = حافظہ;
        }
    }

    واپس m3Err_none;
}


M3BacktraceInfo* m3_GetBacktrace (IM3Runtime i_runtime)
{
#اگر d_m3_ریکارڈ_Backtraces
    واپس &i_runtime->backtrace;
#ورنہ
    واپس تہی;
#اگر_ختم
}


//-------------------------------------------------------------------------------------------------------------------------------
//  snapshots & suspendable execution
//-------------------------------------------------------------------------------------------------------------------------------

خالی m3_سیٹ_Suspendable (IM3Runtime io_runtime, bool i_suspendable)
{
    اگر (io_runtime) {
        io_runtime->isSuspendable = i_suspendable;
    }
}

خالی m3_RequestSuspend (IM3Runtime io_runtime)
{
    اگر (io_runtime) {
        io_runtime->suspendRequested = true;
    }
}

bool m3_IsSuspended (IM3Runtime i_runtime)
{
    اگر (!i_runtime) {
        واپس false;
    }
#اگر d_m3_Has_ڈھیر_Switching
    اگر (i_runtime->جڑ_Continuation && i_runtime->جڑ_Continuation->state == cont_suspended) {
        واپس true;
    }
#اگر_ختم
    واپس false;
}

M3_نتیجہ m3_ResumeRuntime (IM3Runtime io_runtime)
{
    اگر (!io_runtime) {
        واپس m3Err_mallocFailed;
    }
    اگر (!io_runtime->isSuspendable) {
        واپس m3Err_none;
    }
#اگر d_m3HasSnapshots
    // a restore that failed leaves nothing to resume, and says so every time
    // rather than resuming nothing successfully
    اگر (io_runtime->جڑ_Continuation and io_runtime->جڑ_Continuation->اندراج_Function and
        io_runtime->جڑ_Continuation->اندراج_Function->module->isUnusable) {
        واپس "a snapshot failed to restore into the module, so it cannot run";
    }
#اگر_ختم
#اگر d_m3_Has_ڈھیر_Switching
    اگر (!io_runtime->جڑ_Continuation || io_runtime->جڑ_Continuation->state != cont_suspended) {
        واپس m3Err_none;
    }
    // what finishes returns into the bottom of the stack, where it was entered
    io_runtime->ڈھیر = io_runtime->origin_ڈھیر;

    // this is the outermost call again, as far as a host function calling back
    // into Wasm from inside it can tell
    io_runtime->entered = io_runtime->جڑ_Continuation->اندراج_Function;
    io_runtime->callNesting++;

    d_m3_ڈھیر_حد_بندی_Enter(io_runtime);
    io_runtime->resumePastCheck = true;
    M3_نتیجہ نتیجہ             = (M3_نتیجہ)ResumeContinuation(io_runtime, io_runtime->جڑ_Continuation);
    io_runtime->resumePastCheck = false;
    d_m3_ڈھیر_حد_بندی_Leave(io_runtime);

#  اگر d_m3HasExceptionHandling
    اگر (M3_UNLIKELY(نتیجہ == m3Err_pendingException)) {
        نتیجہ = m3Err_trapUncaughtException;
    }
    ریکارڈ_Uncaught_Exception(io_runtime, نتیجہ);
#  endif

    io_runtime->callNesting--;

#  اگر d_m3HasExceptionHandling
    اگر (نتیجہ != m3Err_continuationSuspended and io_runtime->callNesting == 0) {
        FreeExceptions(io_runtime);
    }
#  endif

    // the results are where the paused call left room for them, so a finished
    // resume can be asked for them the way the call itself could have been
    io_runtime->آخری_Called = (نتیجہ == m3Err_none) ? io_runtime->جڑ_Continuation->اندراج_Function : تہی;

    Runtime_رکھو_Call_ڈھیر(io_runtime);
    واپس نتیجہ;
#ورنہ
    واپس m3Err_none;
#اگر_ختم
}
