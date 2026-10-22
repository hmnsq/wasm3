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
            لكل (u8 t = c_m3Type_none; t < c_m3_Type_گنتی; t++) {
                إذا (t == c_m3Type_v128) {
                    استمر;
                }

                IM3FuncType ftype;
_               (AllocFuncType(&ftype, 1));

                ftype->عدد_دلیلاں  = 0;
                ftype->عدد_Rets  = (t == c_m3Type_none) ? 0 : 1;
                ftype->types[0] = t;

                Environment_جوڑو_Func_Type(env, &ftype);

                env->retFuncTypes[t] = ftype;
            }
        }

        _catch:
        إذا (نتیجہ) {
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
        IM3FuncType اگلا = ftype->اگلا;
        m3_Free(ftype);
        ftype = اگلا;
    }

    m3log(runtime, "freeing %d pages from environment", گنتی_کوڈ_ورقے(i_environment->ورقے_Released));
    Free_کوڈ_ورقے(&i_environment->ورقے_Released);
}


فراغ m3_FreeEnvironment (IM3Environment i_environment)
{
    إذا (i_environment) {
        Environment_Release(i_environment);
        m3_Free(i_environment);
    }
}


فراغ m3_سیٹ_Custom_Section_Handler (IM3Environment i_environment, M3SectionHandler i_handler)
{
    إذا (i_environment) {
        i_environment->customSectionHandler = i_handler;
    }
}


// returns the same io_funcType or replaces it with an equivalent that's already in the type linked list
M3_نتیجہ Environment_جوڑو_Func_Type (IM3Environment i_environment, IM3FuncType* io_funcType)
{
    IM3FuncType جوڑو_Type = *io_funcType;
    IM3FuncType newType = i_environment->funcTypes;

    طالما (newType) {
        إذا (not newType->وچ_Rec_گروہ and AreFuncTypesEqual(newType, جوڑو_Type)) {
            m3_Free(جوڑو_Type);
            اكسر;
        }

        newType = newType->اگلا;
    }

    إذا (newType == عدم) {
        // a type index has to fit in the heap type field of an m3type_t
        إذا (i_environment->عدد_Func_Types >= d_m3_Max_Sane_Types_گنتی) {
            m3_Free(جوڑو_Type);
            *io_funcType = عدم;
            أرجع "too many distinct function types";
        }

        newType                  = جوڑو_Type;
        newType->canonical_اشاریہ  = i_environment->عدد_Func_Types++;
        newType->اگلا            = i_environment->funcTypes;
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
M3_نتیجہ Environment_ReserveFuncTypes (IM3Environment i_environment, u32 i_گنتی, u16* o_پہلا_اشاریہ)
{
    إذا ((u64)i_environment->عدد_Func_Types + i_گنتی > d_m3_Max_Sane_Types_گنتی) {
        أرجع "too many distinct function types";
    }

    *o_پہلا_اشاریہ = i_environment->عدد_Func_Types;
    i_environment->عدد_Func_Types += (u16)i_گنتی;

    أرجع m3Err_none;
}


فراغ Environment_AdoptFuncType (IM3Environment i_environment, IM3FuncType i_funcType, u16 i_canonical_اشاریہ)
{
    i_funcType->وچ_Rec_گروہ     = true;
    i_funcType->canonical_اشاریہ = i_canonical_اشاریہ;
    i_funcType->اگلا           = i_environment->funcTypes;
    i_environment->funcTypes   = i_funcType;
}


IM3_کوڈ_ورقہ Remove_کوڈ_ورقہ_دا_سمرتھا (M3_کوڈ_ورقہ** io_list, u32 i_minimum_سطر_گنتی)
{
    IM3_کوڈ_ورقہ prev = عدم;
    IM3_کوڈ_ورقہ ورقہ = *io_list;

    طالما (ورقہ) {
        إذا (عدد_Free_سطریں(ورقہ) >= i_minimum_سطر_گنتی) {             d_m3Assert (ورقہ->info.usage_گنتی == 0);
            IM3_کوڈ_ورقہ اگلا = ورقہ->info.اگلا;
            إذا (prev) {
                prev->info.اگلا = اگلا; // mid-list
            } وإلا {
                *io_list = اگلا;       // front of list
            }

            اكسر;
        }

        prev = ورقہ;
        ورقہ = ورقہ->info.اگلا;
    }

    أرجع ورقہ;
}


IM3_کوڈ_ورقہ Environment_Acquire_کوڈ_ورقہ (IM3Environment i_environment, u32 i_minimum_سطر_گنتی)
{
    أرجع Remove_کوڈ_ورقہ_دا_سمرتھا(&i_environment->ورقے_Released, i_minimum_سطر_گنتی);
}


فراغ Environment_Release_کوڈ_ورقے (IM3Environment i_environment, IM3_کوڈ_ورقہ i_کوڈ_ورقہ_List)
{
    IM3_کوڈ_ورقہ سرا = i_کوڈ_ورقہ_List;

    طالما (سرا) {
        سرا->info.سطر_اشاریہ = 0; // reset page
#إذا d_m3_ریکارڈ_Backtraces
        سرا->info.mapping->ناپ = 0;
#جے_انت // d_m3RecordBacktraces

        IM3_کوڈ_ورقہ اگلا = سرا->info.اگلا;
        إذا (not اگلا) {
            اكسر;
        }

        سرا = اگلا;
    }

    إذا (سرا) {
        // push list to front
        سرا->info.اگلا               = i_environment->ورقے_Released;
        i_environment->ورقے_Released = i_کوڈ_ورقہ_List;
    }
}


IM3Runtime m3_NewRuntime (IM3Environment i_environment, u32 i_ڈھیر_ناپ_وچ_بائٹاں, فراغ* i_userdata)
{
    IM3Runtime runtime = m3_AllocStruct(M3Runtime);

    إذا (runtime) {
        m3_ResetErrorInfo(runtime);

        runtime->environment = i_environment;
        runtime->userdata    = i_userdata;

        Deterministic_InitRuntime(runtime);

        runtime->origin_ڈھیر = m3_Malloc("Wasm Stack", i_ڈھیر_ناپ_وچ_بائٹاں + 4 * حجم(m3slot_t)); // TODO: more precise stack checks

        إذا (runtime->origin_ڈھیر) {
            runtime->ڈھیر         = runtime->origin_ڈھیر;
            runtime->عدد_ڈھیر_Slots = i_ڈھیر_ناپ_وچ_بائٹاں / حجم(m3slot_t);          m3log (runtime, "new stack: %p, slots: %u", runtime->origin_ڈھیر, runtime->عدد_ڈھیر_Slots);
        } وإلا {
            m3_Free(runtime);
        }
    }

    أرجع runtime;
}

فراغ m3_سیٹ_Validation (IM3Runtime i_runtime, bool i_enable)
{
#إذا d_m3EnableValidation
    إذا (i_runtime) {
        i_runtime->skipValidation = not i_enable;
    }
#وإلا
    (فراغ)i_runtime;
    (فراغ)i_enable;              // nothing to skip: the validator was compiled out
#جے_انت
}

M3_نتیجہ m3_سیٹ_Resource_حد_بندی (IM3Runtime i_runtime, M3_Resource_حد_بندی i_حد_بندی, uint64_t i_قدر)
{
    إذا (not i_runtime) {
        أرجع m3Err_mallocFailed;
    }
    بدل (i_حد_بندی) {
    حالة c_m3_حد_بندی_حافظہ_بائٹاں:
        إذا (i_قدر and i_قدر < i_runtime->حافظہ_بائٹاں_ورتیا) {
            أرجع m3_Err_resource_حد_بندی_ہیٹھاں_Usage;
        }
        i_runtime->حافظہ_بائٹاں_حد_بندی = i_قدر;
        أرجع m3Err_none;
    حالة c_m3_حد_بندی_جدول_Elements:
        إذا (i_قدر and i_قدر < i_runtime->جدول_Elements_ورتیا) {
            أرجع m3_Err_resource_حد_بندی_ہیٹھاں_Usage;
        }
        i_runtime->جدول_Elements_حد_بندی = i_قدر;
        أرجع m3Err_none;
#إذا d_m3_Has_ڈھیر_Switching
    حالة c_m3_حد_بندی_Continuations:
        إذا (i_قدر and i_قدر < i_runtime->continuationsAllocated) {
            أرجع m3_Err_resource_حد_بندی_ہیٹھاں_Usage;
        }
        i_runtime->continuations_حد_بندی = (u32)M3_MIN(i_قدر, (u64)UINT32_MAX);
        طالما (i_قدر and i_runtime->val_ڈھیر_Pool_گنتی > i_قدر - i_runtime->continuationsAllocated) {
            m3_Free(i_runtime->val_ڈھیر_Pool[--i_runtime->val_ڈھیر_Pool_گنتی]);
        }
        أرجع m3Err_none;
#وإلا
    حالة c_m3_حد_بندی_Continuations: أرجع m3_Err_resource_حد_بندی_Not_Supported;
#جے_انت
    حالة c_m3_حد_بندی_Gas_Units:
#إذا d_m3HasGasMetering
        i_runtime->gas_حد_بندی = i_runtime->gas_بچیا = (i64)M3_MIN(i_قدر, (u64)INT64_MAX);
        أرجع m3Err_none;
#وإلا
        أرجع m3_Err_resource_حد_بندی_Not_Supported;
#جے_انت
    افتراضي: أرجع m3_Err_unknown_Resource_حد_بندی;
    }
}

uint64_t m3_Get_Resource_حد_بندی (IM3Runtime i_runtime, M3_Resource_حد_بندی i_حد_بندی)
{
    إذا (not i_runtime) {
        أرجع 0;
    }
    بدل (i_حد_بندی) {
    حالة c_m3_حد_بندی_حافظہ_بائٹاں: أرجع i_runtime->حافظہ_بائٹاں_حد_بندی;
    حالة c_m3_حد_بندی_جدول_Elements: أرجع i_runtime->جدول_Elements_حد_بندی;
#إذا d_m3_Has_ڈھیر_Switching
    حالة c_m3_حد_بندی_Continuations: أرجع i_runtime->continuations_حد_بندی;
#جے_انت
#إذا d_m3HasGasMetering
    حالة c_m3_حد_بندی_Gas_Units: أرجع (u64)i_runtime->gas_حد_بندی;
#جے_انت
    افتراضي: أرجع 0;
    }
}

uint64_t m3_GetResourceUsage (IM3Runtime i_runtime, M3_Resource_حد_بندی i_حد_بندی)
{
    إذا (not i_runtime) {
        أرجع 0;
    }
    بدل (i_حد_بندی) {
    حالة c_m3_حد_بندی_حافظہ_بائٹاں: أرجع i_runtime->حافظہ_بائٹاں_ورتیا;
    حالة c_m3_حد_بندی_جدول_Elements: أرجع i_runtime->جدول_Elements_ورتیا;
#إذا d_m3_Has_ڈھیر_Switching
    حالة c_m3_حد_بندی_Continuations: أرجع i_runtime->continuationsAllocated;
#جے_انت
#إذا d_m3HasGasMetering
    حالة c_m3_حد_بندی_Gas_Units: أرجع (u64)i_runtime->gas_حد_بندی - (u64)i_runtime->gas_بچیا;
#جے_انت
    افتراضي: أرجع 0;
    }
}

فراغ* m3_Get_User_انکڑے (IM3Runtime i_runtime)
{
    أرجع i_runtime ? i_runtime->userdata : عدم;
}


فراغ* ForEachModule (IM3Runtime i_runtime, ModuleVisitor i_visitor, فراغ* i_info)
{
    فراغ* r = عدم;

    IM3Module module = i_runtime->modules;

    طالما (module) {
        IM3Module اگلا = module->اگلا;
        إذا ((r = i_visitor(module, i_info))) {
            اكسر;
        }

        module = اگلا;
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
        IM3_حافظہ حافظہ = i_module->memories[i];

        إذا (حافظہ and حافظہ->owner != i_module) {
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
    ForEachModule(i_runtime, _FreeModule, عدم);                    d_m3Assert (i_runtime->عدد_سرگرم_کوڈ_ورقے == 0);

    Environment_Release_کوڈ_ورقے(i_runtime->environment, i_runtime->ورقے_Open);
    Environment_Release_کوڈ_ورقے(i_runtime->environment, i_runtime->ورقے_بھریا);

    m3_Free(i_runtime->origin_ڈھیر);

#إذا d_m3EnableValidation
    m3_Free(i_runtime->validator);
#جے_انت

#إذا d_m3HasExceptionHandling
    // a runtime let go of while suspended still holds its exceptions
    FreeExceptions(i_runtime);
    m3_Free(i_runtime->uncaughtException);
#جے_انت
#إذا d_m3HasExceptionHandling || d_m3_Has_ڈھیر_Switching
    طالما (i_runtime->host_ٹیگ_سوچی) {
        M3_Host_ٹیگ* اگلا = i_runtime->host_ٹیگ_سوچی->اگلا;
        m3_Free(i_runtime->host_ٹیگ_سوچی);
        i_runtime->host_ٹیگ_سوچی = اگلا;
    }
#جے_انت
#إذا d_m3_Has_ڈھیر_Switching
    Continuation_ReleaseAll(i_runtime);
#جے_انت
}


فراغ m3_FreeRuntime (IM3Runtime i_runtime)
{
    إذا (i_runtime) {
        m3_PrintProfilerInfo();

        Runtime_Release(i_runtime);
        m3_Free(i_runtime);
    }
}


#إذا d_m3_Has_ڈھیر_Switching

IM3Continuation Continuation_New (IM3Runtime i_runtime, IM3FuncType i_type, IM3Function i_function)
{
    IM3Continuation cont = m3_AllocStruct(M3Continuation);
    إذا (!cont) {
        أرجع عدم;
    }

    cont->type          = i_type;
    cont->اندراج_Function = i_function;
    cont->state         = cont_allocated;

    cont->عدد_ڈھیر_Slots = d_m3_Continuation_ڈھیر_Slots;
    // frames are recorded only when this continuation actually suspends, and
    // only as deep as it got, so the array starts out unallocated
    cont->چوکھٹے    = عدم;
    cont->چوکھٹے_Cap = 0;
    cont->عدد_چوکھٹے = 0;

    cont->sp = cont->val_ڈھیر;
    cont->pc = i_function ? i_function->compiled : عدم;
    cont->r0 = 0;
#  إذا d_m3HasFloat
    cont->fp0 = 0;
#  endif
    cont->bound_دلیلاں_گنتی    = 0;
    cont->عدد_Suspend_Results = 0;
    cont->عدد_Handlers       = 0;
    cont->handlersPC        = عدم;
    cont->ماپا            = عدم;

    cont->اگلا               = i_runtime->continuations;
    i_runtime->continuations = cont;

    أرجع cont;
}

IM3Continuation Continuation_ForkSuspended (IM3Runtime i_runtime, IM3Continuation i_cont)
{
    IM3Continuation newCont = Continuation_New(i_runtime, i_cont->type, i_cont->اندراج_Function);
    إذا (M3_UNLIKELY(not newCont)) {
        أرجع عدم;
    }

    newCont->val_ڈھیر      = i_cont->val_ڈھیر;
    newCont->عدد_ڈھیر_Slots = i_cont->عدد_ڈھیر_Slots;
    newCont->sp            = i_cont->sp;
    newCont->pc            = i_cont->pc;
    newCont->suspend_نقطہ  = i_cont->suspend_نقطہ;
    newCont->r0            = i_cont->r0;
#  إذا d_m3HasFloat
    newCont->fp0 = i_cont->fp0;
#  endif
    newCont->چوکھٹے    = i_cont->چوکھٹے;
    newCont->چوکھٹے_Cap = i_cont->چوکھٹے_Cap;
    newCont->عدد_چوکھٹے = i_cont->عدد_چوکھٹے;

    newCont->bound_دلیلاں_گنتی = 0;
#  إذا d_m3HasExceptionHandling
    newCont->resumeThrow = i_cont->resumeThrow;
    i_cont->resumeThrow  = عدم;
#  endif
    newCont->عدد_Suspend_Results = i_cont->عدد_Suspend_Results;
    لكل (u32 i = 0; i < i_cont->عدد_Suspend_Results; ++i) {
        newCont->suspend_نتیجہ_Offsets[i] = i_cont->suspend_نتیجہ_Offsets[i];
        newCont->suspend_نتیجہ_Is64[i]    = i_cont->suspend_نتیجہ_Is64[i];
    }

    newCont->عدد_Handlers = i_cont->عدد_Handlers;
    newCont->handlersPC  = i_cont->handlersPC;
    newCont->ماپا      = i_cont->ماپا;
    newCont->state       = cont_suspended;

    i_cont->val_ڈھیر  = عدم;
    i_cont->چوکھٹے    = عدم;
    i_cont->چوکھٹے_Cap = 0;
    i_cont->عدد_چوکھٹے = 0;
    i_cont->state     = cont_consumed;

    أرجع newCont;
}

// Only cont.new and snapshot restoration acquire stacks. Binding and suspension
// transfer them between records without changing the active count.
M3_نتیجہ Continuation_Acquire_ڈھیر (IM3Runtime runtime, IM3Continuation cont)
{
    إذا (not Continuation_Can_Acquire_ڈھیر(runtime)) {
        أرجع m3_Err_continuation_حد_بندی_Exceeded;
    }
    إذا (runtime->val_ڈھیر_Pool_گنتی) {
        cont->val_ڈھیر = runtime->val_ڈھیر_Pool[--runtime->val_ڈھیر_Pool_گنتی];
        memset(cont->val_ڈھیر, 0, (cont->عدد_ڈھیر_Slots + 4) * حجم(m3slot_t));
    } وإلا {
        cont->val_ڈھیر = m3_AllocArray(m3slot_t, cont->عدد_ڈھیر_Slots + 4);
    }
    إذا (not cont->val_ڈھیر) {
        أرجع m3Err_mallocFailed;
    }
    cont->sp = cont->val_ڈھیر;
    runtime->continuationsAllocated++;
    أرجع m3Err_none;
}

فراغ Continuation_Recycle_ڈھیر (IM3Runtime runtime, IM3Continuation cont)
{
    إذا (not cont->val_ڈھیر or cont == runtime->جڑ_Continuation) {
        أرجع;
    }
    إذا (not runtime->val_ڈھیر_Pool) {
        runtime->val_ڈھیر_Pool = m3_AllocArray(m3slot_t*, 16);
        إذا (runtime->val_ڈھیر_Pool) {
            runtime->val_ڈھیر_Pool_Cap = 16;
        }
    }
    إذا (runtime->val_ڈھیر_Pool_گنتی < runtime->val_ڈھیر_Pool_Cap and
        (not runtime->continuations_حد_بندی or runtime->val_ڈھیر_Pool_گنتی <
                                              runtime->continuations_حد_بندی - runtime->continuationsAllocated + 1)) {
        runtime->val_ڈھیر_Pool[runtime->val_ڈھیر_Pool_گنتی++] = cont->val_ڈھیر;
        cont->val_ڈھیر                                      = عدم;
    } وإلا {
        m3_Free(cont->val_ڈھیر);
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
        IM3Continuation اگلا = cont->اگلا;

        m3_Free(cont->val_ڈھیر);
        m3_Free(cont->چوکھٹے);
        m3_Free(cont);

        cont = اگلا;
    }

    لكل (u32 i = 0; i < io_runtime->val_ڈھیر_Pool_گنتی; ++i) {
        m3_Free(io_runtime->val_ڈھیر_Pool[i]);
    }
    m3_Free(io_runtime->val_ڈھیر_Pool);
    io_runtime->val_ڈھیر_Pool_گنتی = io_runtime->val_ڈھیر_Pool_Cap = 0;
    io_runtime->continuationsAllocated                          = 0;
    io_runtime->continuations                                   = عدم;
    io_runtime->سرگرم_Continuation                              = عدم;
    io_runtime->جڑ_Continuation                                = عدم;
}


m3ret_t Continuation_ریکارڈ_چوکھٹا (IM3Runtime i_runtime, ثابت M3_چوکھٹا* i_چوکھٹا)
{
    IM3Continuation cont = i_runtime->سرگرم_Continuation;

    // the marker only travels between a suspend and the resume that runs it,
    // and that whole stretch has an active continuation
    d_m3Assert(cont);

    إذا (cont->عدد_چوکھٹے == cont->چوکھٹے_Cap) {
        u32 newCap = cont->چوکھٹے_Cap ? cont->چوکھٹے_Cap * 2 : 8;

        إذا (newCap > d_m3_Continuation_Max_چوکھٹے) {
            newCap = d_m3_Continuation_Max_چوکھٹے;
        }

        إذا (newCap == cont->چوکھٹے_Cap) {
            أرجع m3_Err_trap_ڈھیر_حد_توں_ودھ;
        }

        M3_چوکھٹا* چوکھٹے = m3_ReallocArray(M3_چوکھٹا, cont->چوکھٹے, newCap, cont->چوکھٹے_Cap);

        إذا (not چوکھٹے) {
            أرجع m3Err_mallocFailed;
        }

        cont->چوکھٹے    = چوکھٹے;
        cont->چوکھٹے_Cap = newCap;
    }

    cont->چوکھٹے[cont->عدد_چوکھٹے++] = *i_چوکھٹا;

    أرجع m3Err_continuationSuspended;
}

#جے_انت // d_m3HasStackSwitching

M3_نتیجہ Evaluate_عبارت (IM3Module i_module, فراغ* o_expressed, m3type_t i_type, بائٹاں_t* io_بائٹاں, cbytes_t i_سرا)
{
    M3_نتیجہ نتیجہ = m3Err_none;

    // OPTZ: use a simplified interpreter for expressions

    // create a temporary runtime context
#إذا defined(d_m3PreferStaticAlloc)
    ساكن M3Runtime runtime;
#وإلا
    M3Runtime runtime;
#جے_انت
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
    o->wasm            = *io_بائٹاں;
    o->wasm_سرا         = i_سرا;
    o->آخری_Opcode_شروع = o->wasm;

    // Borrowed for the length of this expression only. Runtime_Release below hands it
    // to Environment_ReleaseCodePages, which resets lineIndex, so the page comes back
    // empty and the next expression reuses it rather than burning fresh lines.
    o->ورقہ = Acquire_کوڈ_ورقہ(&runtime);

    إذا (o->ورقہ) {
        IM3FuncType ftype = عدم;

#إذا d_m3HasTypedRefs
        // A reference type has to be checked as the type it is: the shared
        // return types are one per storage class, so borrowing one of those
        // would let any reference at all satisfy a particular one.
        إذا (IsSpelledRefType(i_type)) {
            نتیجہ = AllocFuncType(&ftype, 1);

            إذا (not نتیجہ) {
                ftype->عدد_Rets  = 1;
                ftype->types[0] = i_type;

                نتیجہ = Environment_جوڑو_Func_Type(runtime.environment, &ftype);
            }
        } وإلا
#جے_انت
        {
            ftype = runtime.environment->retFuncTypes[ادھار_Type_دا(i_type)];
        }

        pc_t m3code = Get_ورقہ_PC(o->ورقہ);

        إذا (not نتیجہ) {
            نتیجہ = Compile_عبارت(o, ftype);
        }

        إذا (not نتیجہ && o->max_ڈھیر_Slots >= runtime.عدد_ڈھیر_Slots) {
            نتیجہ = m3_Err_trap_ڈھیر_حد_توں_ودھ;
        }

        إذا (not نتیجہ) {
#إذا (d_m3EnableOpProfiling || d_m3EnableOpTracing)
            m3ret_t r = لگاتار_کوڈ(m3code, ڈھیر, عدم, d_m3_Op_Default_دلیلاں, d_m3_ادھار_Cstr);
#وإلا
            m3ret_t r = لگاتار_کوڈ(m3code, ڈھیر, عدم, d_m3_Op_Default_دلیلاں);
#جے_انت

            إذا (r == 0) {                                                                   m3log (runtime, "expression result: %s", SPrint_قدر (ڈھیر, i_type));
                إذا (ناپ_دا_Type(ادھار_Type_دا(i_type)) == حجم(u32)) {
                    *(u32*)o_expressed = *((u32*)ڈھیر);
                } وإلا {
                    *(u64*)o_expressed = *((u64*)ڈھیر);
                }
            }
        }

        Release_کوڈ_ورقہ(&runtime, o->ورقہ);
    } وإلا {
        نتیجہ = m3_Err_malloc_Failed_کوڈ_ورقہ;
    }

    runtime.origin_ڈھیر = عدم;        // prevent free(stack) in ReleaseRuntime
    Runtime_Release(&runtime);
    i_module->runtime = savedRuntime;

    *io_بائٹاں = o->wasm;

    أرجع نتیجہ;
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
bool LimitsSatisfy (u64 i_exported_ناپ, bool i_exportedHasMax, u64 i_exportedMax,
                    u64 i_importMin, bool i_importHasMax, u64 i_importMax)
{
    إذا (i_exported_ناپ < i_importMin) {
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
ثابت M3Export* Module_لبھو_Export (IM3Module i_module, u8 i_قسم, cstr_t i_name)
{
    لكل (u32 i = 0; i < i_module->عدد_Exports; ++i) {
        ثابت M3Export* e = &i_module->exports[i];

        إذا (e->قسم == i_قسم and strcmp(e->name, i_name) == 0) {
            أرجع e;
        }
    }

    أرجع عدم;
}


ساكن
IM3Function Module_لبھو_Exported_Function (IM3Module i_module, cstr_t i_name)
{
    ثابت M3Export* e = Module_لبھو_Export(i_module, d_external_قسم_function, i_name);

    // A module that re-exports an import names the placeholder here. Resolve to
    // the function that actually runs, or a host-side call would run it against
    // the importing module's memory.
    أرجع e ? Function_Implementation(&i_module->functions[e->index]) : عدم;
}


ساكن
IM3_حافظہ Module_لبھو_Exported_حافظہ (IM3Module i_module, cstr_t i_name)
{
    ثابت M3Export* e = Module_لبھو_Export(i_module, d_external_قسم_حافظہ, i_name);

    أرجع e ? i_module->memories[e->index] : عدم;
}


ساكن
IM3_جدول Module_لبھو_Exported_جدول (IM3Module i_module, cstr_t i_name)
{
    ثابت M3Export* e = Module_لبھو_Export(i_module, d_external_قسم_جدول, i_name);

    أرجع e ? i_module->tables[e->index] : عدم;
}


ساكن
IM3Global Module_لبھو_Exported_Global (IM3Module i_module, cstr_t i_name)
{
    ثابت M3Export* e = Module_لبھو_Export(i_module, d_external_قسم_global, i_name);

    أرجع e ? &i_module->globals[e->index] : عدم;
}


#إذا d_m3HasExceptionHandling || d_m3_Has_ڈھیر_Switching
ساكن
IM3_ٹیگ Module_لبھو_Exported_ٹیگ (IM3Module i_module, cstr_t i_name)
{
    ثابت M3Export* e = Module_لبھو_Export(i_module, d_external_قسم_ٹیگ, i_name);

    إذا (not e) {
        أرجع عدم;
    }

    IM3_ٹیگ ٹیگ = &i_module->ٹیگ_سوچی[e->index];

    أرجع ٹیگ->resolved ? ٹیگ->resolved : ٹیگ;
}
#جے_انت


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
M3_نتیجہ LinkImports (IM3Runtime io_runtime, IM3Module io_module)
{
    M3_نتیجہ نتیجہ = m3Err_none;

    لكل (u32 i = 0; i < io_module->عدد_Functions; ++i) {
        IM3Function f = &io_module->functions[i];

        إذا (f->wasm or not (f->import.moduleUtf8 and f->import.میدان_Utf8)) {
            استمر;
        }

        IM3Module from = m3_لبھو_Module(io_runtime, f->import.moduleUtf8);
        إذا (not from) {
            استمر;
        }

        IM3Function exported = Module_لبھو_Exported_Function(from, f->import.میدان_Utf8);
        إذا (not exported) {
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

    لكل (u32 i = 0; i < io_module->عدد_Memories; ++i) {
        IM3_حافظہ حافظہ = io_module->memories[i];

        إذا (not حافظہ->imported or حافظہ->owner != io_module) {
            استمر;
        }

        IM3Module from = m3_لبھو_Module(io_runtime, حافظہ->import.moduleUtf8);
        إذا (not from) {
            استمر;
        }

        IM3_حافظہ exported = Module_لبھو_Exported_حافظہ(from, حافظہ->import.میدان_Utf8);
        إذا (not exported) {
            _throwif(m3Err_incompatibleImportType, Module_HasExport(from, حافظہ->import.میدان_Utf8));
            _throw(m3Err_unknownImport);
        }

        // the address type is part of the memory type, so an i64 memory does
        // not satisfy an i32 import, or the other way round - and so is the page
        // size, which custom page sizes made a declared property of a memory
        _throwif(m3Err_incompatibleImportType, exported->isMemory64 != حافظہ->isMemory64);
        _throwif(m3Err_incompatibleImportType, حافظہ_ورقہ_ناپ(exported) != حافظہ_ورقہ_ناپ(حافظہ));

        // a shared memory is not a subtype of an unshared one, or the other way round
        _throwif(m3Err_incompatibleImportType, exported->isShared != حافظہ->isShared);

        _throwif(m3Err_incompatibleImportType,
                 not LimitsSatisfy(exported->عدد_ورقے, exported->hasMax, exported->max_ورقے,
                                   حافظہ->init_ورقے, حافظہ->hasMax, حافظہ->max_ورقے));

        // hand the slot over to the exporter's memory, and drop the placeholder
        Free_حافظہ_بلاک(حافظہ);
        FreeImportInfo(&حافظہ->import);
        m3_Free(حافظہ);

        io_module->memories[i] = exported;
    }

    لكل (u32 i = 0; i < io_module->عدد_Tables; ++i) {
        IM3_جدول جدول_2 = io_module->tables[i];

        إذا (not جدول_2->imported or جدول_2->owner != io_module) {
            استمر;
        }

        IM3Module from = m3_لبھو_Module(io_runtime, جدول_2->import.moduleUtf8);
        إذا (not from) {
            استمر;
        }

        IM3_جدول exported = Module_لبھو_Exported_جدول(from, جدول_2->import.میدان_Utf8);
        إذا (not exported) {
            _throwif(m3Err_incompatibleImportType, Module_HasExport(from, جدول_2->import.میدان_Utf8));
            _throw(m3Err_unknownImport);
        }

        _throwif(m3Err_incompatibleImportType, exported->type != جدول_2->type);

        // the index type is part of the table type, the same way it is for a memory
        _throwif(m3Err_incompatibleImportType, exported->isTable64 != جدول_2->isTable64);

        _throwif(m3Err_incompatibleImportType,
                 not LimitsSatisfy(exported->ناپ, exported->hasMax, exported->max_ناپ,
                                   جدول_2->init_ناپ, جدول_2->hasMax, جدول_2->max_ناپ));

        // hand the slot over to the exporter's table, and drop the placeholder
        m3_Free(جدول_2->elements);
        FreeImportInfo(&جدول_2->import);
        m3_Free(جدول_2);

        io_module->tables[i] = exported;
    }

    لكل (u32 i = 0; i < io_module->عدد_Globals; ++i) {
        IM3Global g = &io_module->globals[i];

        إذا (not g->imported or not (g->import.moduleUtf8 and g->import.میدان_Utf8)) {
            استمر;
        }

        IM3Module from = m3_لبھو_Module(io_runtime, g->import.moduleUtf8);
        إذا (not from) {
            استمر;
        }

        IM3Global exported = Module_لبھو_Exported_Global(from, g->import.میدان_Utf8);
        إذا (not exported) {
            _throwif(m3Err_incompatibleImportType, Module_HasExport(from, g->import.میدان_Utf8));
            _throw(m3Err_unknownImport);
        }

        _throwif(m3Err_incompatibleImportType, exported->type != g->type);
        _throwif(m3Err_incompatibleImportType, exported->isMutable != g->isMutable);

        g->resolved = exported->resolved ? exported->resolved : exported;
    }

#إذا d_m3HasExceptionHandling || d_m3_Has_ڈھیر_Switching
    لكل (u32 i = 0; i < io_module->عدد_ٹیگ_سوچی; ++i) {
        IM3_ٹیگ ٹیگ = &io_module->ٹیگ_سوچی[i];

        إذا (not ٹیگ->imported or not (ٹیگ->import.moduleUtf8 and ٹیگ->import.میدان_Utf8)) {
            استمر;
        }

        IM3Module from = m3_لبھو_Module(io_runtime, ٹیگ->import.moduleUtf8);
        إذا (not from) {
            استمر;
        }

        IM3_ٹیگ exported = Module_لبھو_Exported_ٹیگ(from, ٹیگ->import.میدان_Utf8);
        إذا (not exported) {
            _throwif(m3Err_incompatibleImportType, Module_HasExport(from, ٹیگ->import.میدان_Utf8));
            _throw(m3Err_unknownImport);
        }

        _throwif(m3Err_incompatibleImportType, exported->type != ٹیگ->type);

        ٹیگ->resolved = exported->resolved ? exported->resolved : exported;
    }
#جے_انت

    _catch: أرجع نتیجہ;
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
                                              : &i_module->خالی_تھاواں_حافظہ;

    إذا (i_module->عدد_Memories == 0) {
        // nothing addressable, but _mem still has to point somewhere
        i_module->خالی_تھاواں_حافظہ.owner    = i_module;
        i_module->خالی_تھاواں_حافظہ.ورقہ_ناپ = d_m3_Default_Mem_ورقہ_ناپ;

_       (Resize_حافظہ(io_runtime, &i_module->خالی_تھاواں_حافظہ, 0));
    }

    لكل (u32 i = 0; i < i_module->عدد_Memories; ++i) {
        IM3_حافظہ حافظہ = i_module->memories[i];

        // a slot that already points at another module's memory is that
        // module's to allocate
        إذا (حافظہ->owner != i_module or حافظہ->mallocated) {
            استمر;
        }

        u32 ورقہ_ناپ = حافظہ_ورقہ_ناپ(حافظہ);

        حافظہ->ورقہ_ناپ = ورقہ_ناپ;

        // Without a declared maximum a memory may grow to the spec limit of
        // 2^|addrtype|/pagesize pages, which is the usual 65536 at the default
        // page size, 2^48 for a 64-bit memory, and a whole address space of
        // them when a page is a single byte. A declared maximum of zero is a
        // real limit, not the absence of one.
        //
        // 2^64/pagesize overflows a u64 only when a page is a single byte, and
        // a power-of-two page size divides the address space exactly, so the
        // 64-bit division below is 2^64/pagesize written so that it fits.
        إذا (not حافظہ->hasMax) {
            حافظہ->max_ورقے = حافظہ->isMemory64
                                 ? (ورقہ_ناپ > 1 ? (UINT64_MAX / ورقہ_ناپ) + 1 : UINT64_MAX)
                                 : (0x100000000ull / ورقہ_ناپ);
        }

_       (Resize_حافظہ(io_runtime, حافظہ, حافظہ->init_ورقے));
    }

    _catch: أرجع نتیجہ;
}


M3_نتیجہ Resize_حافظہ (IM3Runtime io_runtime, IM3_حافظہ حافظہ, u64 i_عدد_ورقے)
{
    M3_نتیجہ نتیجہ = m3Err_none;

    u64 عدد_ورقے_منزل_Alloc = i_عدد_ورقے;

    إذا (عدد_ورقے_منزل_Alloc <= حافظہ->max_ورقے) {
        // A 64-bit memory may ask for up to 2^48 pages, which overflows a u64
        // of bytes. Nothing that large can be backed, so refuse it up front
        // rather than multiplying into a wrapped size.
        _throwif("linear memory limitation exceeded",
                 عدد_ورقے_منزل_Alloc > d_m3_پتہ_حد_بندی / حافظہ->ورقہ_ناپ);

        u64 عدد_ورقہ_بائٹاں = عدد_ورقے_منزل_Alloc * حافظہ->ورقہ_ناپ;

#إذا d_m3_Max_خطی_حافظہ_ورقے > 0
        // the limit is a memory size, counted in default-sized pages; comparing
        // it against a raw page count would make it 65536 times stricter for a
        // module whose pages are one byte
        _throwif("linear memory limitation exceeded",
                 عدد_ورقہ_بائٹاں > (u64)d_m3_Max_خطی_حافظہ_ورقے * d_m3_Default_Mem_ورقہ_ناپ);
#جے_انت

        // Limit the amount of memory that gets actually allocated
        إذا (io_runtime->حافظہ_حد_بندی) {
            عدد_ورقہ_بائٹاں = M3_MIN(عدد_ورقہ_بائٹاں, (u64)io_runtime->حافظہ_حد_بندی);
        }

        u64 old_لمائی = حافظہ->mallocated ? حافظہ->mallocated->لمائی : 0;
        u64 other_ورتیا = io_runtime->حافظہ_بائٹاں_ورتیا - old_لمائی;
        _throwif(m3_Err_حافظہ_حد_بندی_Exceeded, io_runtime->حافظہ_بائٹاں_حد_بندی and
                                              (عدد_ورقہ_بائٹاں > io_runtime->حافظہ_بائٹاں_حد_بندی or
                                               other_ورتیا > io_runtime->حافظہ_بائٹاں_حد_بندی - عدد_ورقہ_بائٹاں));

        _throwif("linear memory limitation exceeded", عدد_ورقہ_بائٹاں > (u64)SIZE_MAX - حجم(M3_حافظہ_سرخی));

#إذا d_m3_Guarded_حافظہ

        // A slot holds every address a Wasm access can name, and nothing beyond
        // that: a memory larger than the data part of one would put addresses past
        // the guard, where a fault is nobody's to recognize
        _throwif("linear memory limitation exceeded", عدد_ورقہ_بائٹاں > d_m3_Guarded_انکڑے_بائٹاں);

        // What makes an access past the end fault is the page after the memory not
        // being committed, so the end has to fall on one of the system's page
        // boundaries. A Wasm page divides evenly into them and always will; a memory
        // that declared a page size of its own may not, and then the bytes between
        // the end of the memory and the end of its last system page would answer
        // instead of trapping. Refusing to back such a memory is the honest way out:
        // the custom page sizes proposal exists for targets far too small to reserve
        // 8GiB per memory in the first place. See d_m3GuardedMemory.
        _throwif("guarded memory needs a page size the system's divides into",
                 (حافظہ->ورقہ_ناپ % m3_Host_ورقہ_ناپ()) != 0);

        إذا (حافظہ->guard_تھاں == عدم) {
            حافظہ->guard_تھاں = Guard_Take_تھاں();
            _throwifnull(حافظہ->guard_تھاں);
        }

        // Growing only ever commits more of the same slot, so the memory never
        // moves - which the interpreter does not rely on, but nothing minds
        _throwif(m3Err_mallocFailed, not Guard_Commit_تھاں(حافظہ->guard_تھاں, (size_t)عدد_ورقہ_بائٹاں));

        حافظہ->mallocated = Guard_تھاں_سرخی(حافظہ->guard_تھاں);

#وإلا

        size_t عدد_بائٹاں = (size_t)عدد_ورقہ_بائٹاں + حجم(M3_حافظہ_سرخی);

        // What is really there, which is not numPages * pageSize: the limit above
        // caps the bytes without capping the page count, so a memory can be short
        // of a page - by design, since a whole 64 KiB page is often more than an
        // MCU has. Measuring the old block by the page count would overstate it,
        // and then realloc never sees the size it already has and copies the
        // whole limit again on every grow past it.
        size_t عدد_پچھلا_بائٹاں = حافظہ->mallocated ? حافظہ->mallocated->لمائی + حجم(M3_حافظہ_سرخی) : 0;

        فراغ* newMem = m3_Realloc("Wasm Linear Memory", حافظہ->mallocated, عدد_بائٹاں, عدد_پچھلا_بائٹاں);
        _throwifnull(newMem);

        حافظہ->mallocated = (M3_حافظہ_سرخی*)newMem;

#جے_انت

#إذا d_m3LogRuntime
        M3_حافظہ_سرخی* oldMallocated = حافظہ->mallocated;
#جے_انت

        io_runtime->حافظہ_بائٹاں_ورتیا = other_ورتیا + عدد_ورقہ_بائٹاں;
        حافظہ->عدد_ورقے            = عدد_ورقے_منزل_Alloc;

        حافظہ->mallocated->لمائی  = (size_t)عدد_ورقہ_بائٹاں;
        حافظہ->mallocated->runtime = io_runtime;
        حافظہ->mallocated->حافظہ  = حافظہ;

        حافظہ->mallocated->max_ڈھیر = (m3slot_t*)io_runtime->origin_ڈھیر + io_runtime->عدد_ڈھیر_Slots;

        m3log(runtime, "resized old: %p; mem: %p; length: %zu; pages: %llu", oldMallocated, حافظہ->mallocated, حافظہ->mallocated->لمائی, (بنانشانی لما لما)حافظہ->عدد_ورقے);
    } وإلا {
        نتیجہ = m3_Err_wasm_حافظہ_حد_توں_ودھ;
    }

    _catch: أرجع نتیجہ;
}


فراغ Free_حافظہ_بلاک (IM3_حافظہ io_حافظہ)
{
    إذا (io_حافظہ->mallocated and io_حافظہ->mallocated->runtime) {
        io_حافظہ->mallocated->runtime->حافظہ_بائٹاں_ورتیا -= io_حافظہ->mallocated->لمائی;
    }
#إذا d_m3_Guarded_حافظہ
    Guard_Give_تھاں(io_حافظہ->guard_تھاں);
    io_حافظہ->guard_تھاں = عدم;

    io_حافظہ->mallocated = عدم;
#وإلا
    m3_Free(io_حافظہ->mallocated);
#جے_انت
}


M3_نتیجہ InitGlobals (IM3Module io_module)
{
    M3_نتیجہ نتیجہ = m3Err_none;

    إذا (io_module->عدد_Globals) {
        // placing the globals in their structs isn't good for cache locality, but i don't really know what the global
        // access patterns typically look like yet.

        //          io_module->globalMemory = m3Alloc (m3reg_t, io_module->numGlobals);

        //          if (io_module->globalMemory)
        {
            لكل (u32 i = 0; i < io_module->عدد_Globals; ++i) {
                M3Global* g = &io_module->globals[i];                           m3log (runtime, "initializing global: %d", i);

                إذا (g->init_اظہاریہ) {
                    بائٹاں_t شروع = g->init_اظہاریہ;

                    نتیجہ = Evaluate_عبارت(io_module, &g->i64_قدر, g->type, &شروع, g->init_اظہاریہ + g->init_اظہاریہ_ناپ);

                    إذا (not نتیجہ) {
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

    أرجع نتیجہ;
}


M3_نتیجہ Init_انکڑے_Segments (IM3Module io_module)
{
    M3_نتیجہ نتیجہ = m3Err_none;

    لكل (u32 i = 0; i < io_module->عدد_انکڑے_Segments; ++i) {
        M3_انکڑے_Segment* segment = &io_module->انکڑے_Segments[i];

        // A passive segment stays available for memory.init until data.drop.
        // An active one is copied here and then counts as dropped.
        إذا (segment->isPassive) {
            استمر;
        }

        _throwif("data segment memory index out of range",
                 segment->حافظہ_Region >= io_module->عدد_Memories);

        IM3_حافظہ io_حافظہ = io_module->memories[segment->حافظہ_Region];

        _throwif("unallocated linear memory", !(io_حافظہ->mallocated));

        // The offset expression has the memory's address type, and is
        // unsigned: an i32 offset of -1 is 4294967295, way out of bounds
        // rather than negative.
        u64     segment_وتھ = 0;
        بائٹاں_t شروع         = segment->init_اظہاریہ;

        إذا (io_حافظہ->isMemory64) {
_           (Evaluate_عبارت(io_module, &segment_وتھ, c_m3Type_i64, &شروع, segment->init_اظہاریہ + segment->init_اظہاریہ_ناپ));
        } وإلا {
            u32 offset32;
_           (Evaluate_عبارت(io_module, &offset32, c_m3Type_i32, &شروع, segment->init_اظہاریہ + segment->init_اظہاریہ_ناپ));
            segment_وتھ = offset32;
        }

        m3log(runtime, "loading data segment: %d; size: %d; offset: %llu", i, segment->ناپ, (بنانشانی لما لما)segment_وتھ);

        إذا (segment_وتھ <= io_حافظہ->mallocated->لمائی &&
            (u64)segment->ناپ <= io_حافظہ->mallocated->لمائی - segment_وتھ) {
            u8* dest = m3_Mem_انکڑے(io_حافظہ->mallocated) + segment_وتھ;
            حافظہ_نقل(dest, segment->انکڑے, segment->ناپ);
        } وإلا {
            _throw("data segment out of bounds");
        }

        segment->dropped = true;
    }

    _catch: أرجع نتیجہ;
}


// Turns a segment's elements into references. Element expressions are constant
// expressions restricted to ref.null/ref.func, so they're read directly rather
// than run through the compiler.
ساكن
M3_نتیجہ ResolveElements (IM3Module io_module, M3ElementSegment* i_segment, فراغ** o_elements)
{
    M3_نتیجہ نتیجہ = m3Err_none;

    بائٹاں_t  pos = i_segment->elements;
    cbytes_t سرا = io_module->element_Section_سرا;

    لكل (u32 e = 0; e < i_segment->عدد_Elements; ++e) {
        u32   func_اشاریہ;
        فراغ* ref = عدم;

        إذا (i_segment->is_اظہاریہ) {
            m3opcode_t opcode;
_           (Read_opcode(&opcode, &pos, سرا));

            إذا (opcode == c_waOp_refFunc) {
_               (ReadLEB_u32(&func_اشاریہ, &pos, سرا));
                _throwif("function index out of range", func_اشاریہ >= io_module->عدد_Functions);
                ref = Function_Implementation(&io_module->functions[func_اشاریہ]);
            } وإلا إذا (opcode == c_waOp_refNull) {
                i8 waType;
                u8 nullType;
_               (ReadLEB_i7(&waType, &pos, سرا));
_               (NormalizeType(&nullType, waType));
                _throwif(m3Err_typeMismatch, nullType != i_segment->type);
            } وإلا إذا (opcode == c_waOp_getGlobal) {
                // wasm 2.0 lets an element expression read an imported
                // immutable global, which is how one module seeds another's
                // table with a reference it exported.
                u32 global_اشاریہ;
_               (ReadLEB_u32(&global_اشاریہ, &pos, سرا));
                _throwif(m3_Err_globa_اشاریہ_باہرلا_دا_Bounds, global_اشاریہ >= io_module->عدد_Globals);

                IM3Global global = &io_module->globals[global_اشاریہ];

                _throwif(m3_Err_globa_اشاریہ_باہرلا_دا_Bounds, not global->imported);
                _throwif(m3Err_wasmMalformed, global->isMutable);
                _throwif(m3Err_typeMismatch, ادھار_Type_دا(global->type) != ادھار_Type_دا(i_segment->type));

                // read the cell the import was linked to, not the placeholder
                إذا (global->resolved) {
                    global = global->resolved;
                }

                ref = global->ref_قدر;
            } وإلا {
                _throw("constant expression required");
            }

_           (Read_opcode(&opcode, &pos, سرا));
            _throwif(m3Err_wasmMalformed, opcode != c_wa_Op_سرا);
        } وإلا {
_           (ReadLEB_u32(&func_اشاریہ, &pos, سرا));
            _throwif("function index out of range", func_اشاریہ >= io_module->عدد_Functions);
            ref = Function_Implementation(&io_module->functions[func_اشاریہ]);
        }

        o_elements[e] = ref;
    }

    _catch: أرجع نتیجہ;
}


M3_نتیجہ Init_جدول_And_Elements (IM3Module io_module)
{
    M3_نتیجہ نتیجہ = m3Err_none;

    cbytes_t سرا = io_module->element_Section_سرا;
    M3_جدول* جدول_2;

    لكل (u32 i = 0; i < io_module->عدد_Tables; ++i) {
        جدول_2 = io_module->tables[i];

        // a slot pointing at another module's table is that module's to fill
        إذا (جدول_2->owner != io_module) {
            استمر;
        }

        إذا (جدول_2->ناپ) {
            IM3Runtime runtime = io_module->runtime;
            _throwif(m3_Err_جدول_حد_بندی_Exceeded, runtime->جدول_Elements_حد_بندی and
                                                 جدول_2->ناپ > runtime->جدول_Elements_حد_بندی - runtime->جدول_Elements_ورتیا);
            جدول_2->elements = m3_AllocArray(فراغ*, جدول_2->ناپ);
            _throwifnull(جدول_2->elements);
            runtime->جدول_Elements_ورتیا += جدول_2->ناپ;

            إذا (جدول_2->init_اظہاریہ) {
                فراغ*   قدر = عدم;
                بائٹاں_t شروع = جدول_2->init_اظہاریہ;
_               (Evaluate_عبارت(io_module, &قدر, ادھار_Type_دا(جدول_2->type),
                                     &شروع, جدول_2->init_اظہاریہ + جدول_2->init_اظہاریہ_ناپ));

                لكل (u32 e = 0; e < جدول_2->ناپ; ++e) {
                    جدول_2->elements[e] = قدر;
                }
            }
        }
    }

    لكل (u32 i = 0; i < io_module->عدد_Element_Segments; ++i) {
        M3ElementSegment* segment = &io_module->elementSegments[i];

        // Declarative segments only make their functions referenceable, and
        // passive ones wait for table.init, so neither is written out here.
        إذا (segment->منوال == c_m3Elem_declarative) {
            segment->dropped = true;
            استمر;
        }

        إذا (segment->منوال == c_m3Elem_passive) {
            إذا (segment->عدد_Elements) {
                segment->resolved = m3_AllocArray(فراغ*, segment->عدد_Elements);
                _throwifnull(segment->resolved);
_               (ResolveElements(io_module, segment, segment->resolved));
            }
            استمر;
        }

        جدول_2 = io_module->tables[segment->جدول_اشاریہ];

        // The offset expression has the table's index type, and is unsigned:
        // an i32 offset of -1 is 4294967295, out of bounds rather than negative.
        u64     offset = 0;
        بائٹاں_t اظہاریہ   = segment->init_اظہاریہ;

        إذا (جدول_2->isTable64) {
_           (Evaluate_عبارت(io_module, &offset, c_m3Type_i64, &اظہاریہ, سرا));
        } وإلا {
            u32 offset32;
_           (Evaluate_عبارت(io_module, &offset32, c_m3Type_i32, &اظہاریہ, سرا));
            offset = offset32;
        }

        _throwif("out of bounds table access",
                 offset > جدول_2->ناپ or segment->عدد_Elements > جدول_2->ناپ - offset);

_       (ResolveElements(io_module, segment, جدول_2->elements + offset));

        segment->dropped = true;
    }

    _catch: أرجع نتیجہ;
}

M3_نتیجہ m3_CompileModule (IM3Module io_module)
{
    M3_نتیجہ نتیجہ = m3Err_none;

    لكل (u32 i = 0; i < io_module->عدد_Functions; ++i) {
        IM3Function f = &io_module->functions[i];
        إذا (f->wasm and not f->compiled) {
_           (CompileFunction(f));
        }
    }

    _catch: أرجع نتیجہ;
}

#إذا d_m3HasExceptionHandling

M3Exception* NewException (IM3Runtime io_runtime, IM3_ٹیگ i_ٹیگ, u32 i_عدد_دلیلاں)
{
    M3Exception* exception = (M3Exception*)m3_Malloc("M3Exception", حجم(M3Exception) + i_عدد_دلیلاں * حجم(u64));

    إذا (exception) {
        exception->ٹیگ     = i_ٹیگ;
        exception->عدد_دلیلاں = i_عدد_دلیلاں;
        exception->reified = false;
        exception->prev    = عدم;
        exception->اگلا    = io_runtime->exceptions;

        إذا (exception->اگلا) {
            exception->اگلا->prev = exception;
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
        i_exception->prev->اگلا = i_exception->اگلا;
    } وإلا {
        io_runtime->exceptions = i_exception->اگلا;
    }

    إذا (i_exception->اگلا) {
        i_exception->اگلا->prev = i_exception->prev;
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
ساكن
فراغ ریکارڈ_Uncaught_Exception (IM3Runtime io_runtime, M3_نتیجہ i_نتیجہ)
{
    إذا (io_runtime->uncaughtException) {
        m3_Free(io_runtime->uncaughtException);
    }

    M3Exception* exception = io_runtime->pendingException;

    إذا (M3_UNLIKELY(i_نتیجہ == m3Err_trapUncaughtException) and exception) {
        M3Exception* copy = (M3Exception*)m3_CopyMem(exception, حجم(M3Exception) + exception->عدد_دلیلاں * حجم(u64));

        إذا (copy) {
            copy->اگلا = عدم;
            copy->prev = عدم;
        }

        io_runtime->uncaughtException = copy;
    }
}

#جے_انت // d_m3HasExceptionHandling


#إذا d_m3HasExceptionHandling || d_m3_Has_ڈھیر_Switching

IM3_ٹیگ m3_لبھو_ٹیگ (IM3Module i_module, ثابت حرف* ثابت i_ٹیگ_Name)
{
    إذا (not i_module or not i_ٹیگ_Name) {
        أرجع عدم;
    }

    أرجع Module_لبھو_Exported_ٹیگ(i_module, i_ٹیگ_Name);
}


M3_نتیجہ m3_New_ٹیگ (IM3Runtime io_runtime, IM3_ٹیگ* o_ٹیگ, ثابت حرف* ثابت i_signature)
{
    IM3FuncType type    = عدم;
    M3_Host_ٹیگ*  host_ٹیگ = عدم;

_try {
    _throwif("null tag", not o_ٹیگ);
    *o_ٹیگ = عدم;
    _throwif(m3Err_mallocFailed, not io_runtime);

_   (Signature_منزل_Func_Type(&type, i_signature));
    // tags are the same type exactly when their types are the same object
_   (Environment_جوڑو_Func_Type(io_runtime->environment, &type));

    host_ٹیگ = m3_AllocStruct(M3_Host_ٹیگ);
    _throwifnull(host_ٹیگ);

    host_ٹیگ->ٹیگ.type    = type;
    host_ٹیگ->اگلا        = io_runtime->host_ٹیگ_سوچی;
    io_runtime->host_ٹیگ_سوچی = host_ٹیگ;

    *o_ٹیگ = &host_ٹیگ->ٹیگ;
} _catch:
    أرجع نتیجہ;
}


M3_نتیجہ m3_Link_ٹیگ (IM3Module         io_module,
                     ثابت حرف* ثابت i_moduleName,
                     ثابت حرف* ثابت i_ٹیگ_Name,
                     IM3_ٹیگ            i_ٹیگ)
{
    إذا (not io_module or not i_moduleName or not i_ٹیگ_Name or not i_ٹیگ) {
        أرجع m3_Err_ٹیگ_Lookup_Failed;
    }

    ثابت bool wildcardModule = (strcmp(i_moduleName, "*") == 0);
    IM3_ٹیگ     نشانہ         = i_ٹیگ->resolved ? i_ٹیگ->resolved : i_ٹیگ;

    M3_نتیجہ نتیجہ = m3_Err_ٹیگ_Lookup_Failed;

    لكل (u32 i = 0; i < io_module->عدد_ٹیگ_سوچی; ++i) {
        IM3_ٹیگ ٹیگ = &io_module->ٹیگ_سوچی[i];

        إذا (ٹیگ->imported and ٹیگ->import.moduleUtf8 and ٹیگ->import.میدان_Utf8 and
            strcmp(ٹیگ->import.میدان_Utf8, i_ٹیگ_Name) == 0 and
            (wildcardModule or strcmp(ٹیگ->import.moduleUtf8, i_moduleName) == 0)) {
            إذا (ٹیگ->type != نشانہ->type) {
                أرجع m3Err_incompatibleImportType;
            }

            ٹیگ->resolved = نشانہ;
            نتیجہ        = m3Err_none;
        }
    }

    أرجع نتیجہ;
}


uint32_t m3_Get_ٹیگ_Arg_گنتی (IM3_ٹیگ i_ٹیگ)
{
    أرجع i_ٹیگ ? i_ٹیگ->type->عدد_دلیلاں : 0;
}


M3_قدر_Type m3_Get_ٹیگ_Arg_Type (IM3_ٹیگ i_ٹیگ, uint32_t i_اشاریہ)
{
    إذا (i_ٹیگ and i_اشاریہ < i_ٹیگ->type->عدد_دلیلاں) {
        أرجع (M3_قدر_Type)ادھار_Type_دا(d_FuncArgType(i_ٹیگ->type, i_اشاریہ));
    }

    أرجع c_m3Type_none;
}

#وإلا

IM3_ٹیگ m3_لبھو_ٹیگ (IM3Module i_module, ثابت حرف* ثابت i_ٹیگ_Name)
{
    أرجع عدم;
}

M3_نتیجہ m3_New_ٹیگ (IM3Runtime io_runtime, IM3_ٹیگ* o_ٹیگ, ثابت حرف* ثابت i_signature)
{
    إذا (o_ٹیگ) {
        *o_ٹیگ = عدم;
    }
    أرجع m3_Err_ٹیگ_Lookup_Failed;
}

M3_نتیجہ m3_Link_ٹیگ (IM3Module io_module, ثابت حرف* ثابت i_moduleName, ثابت حرف* ثابت i_ٹیگ_Name, IM3_ٹیگ i_ٹیگ)
{
    أرجع m3_Err_ٹیگ_Lookup_Failed;
}

uint32_t m3_Get_ٹیگ_Arg_گنتی (IM3_ٹیگ i_ٹیگ)
{
    أرجع 0;
}

M3_قدر_Type m3_Get_ٹیگ_Arg_Type (IM3_ٹیگ i_ٹیگ, uint32_t i_اشاریہ)
{
    أرجع c_m3Type_none;
}

#جے_انت // d_m3HasExceptionHandling || d_m3HasStackSwitching


#إذا d_m3HasExceptionHandling

M3_نتیجہ m3_ThrowException (IM3Runtime io_runtime, IM3_ٹیگ i_ٹیگ, uint32_t i_argc, ثابت فراغ* i_argptrs[])
{
    إذا (not io_runtime or not i_ٹیگ) {
        أرجع m3_Err_ٹیگ_Lookup_Failed;
    }

    IM3_ٹیگ      ٹیگ  = i_ٹیگ->resolved ? i_ٹیگ->resolved : i_ٹیگ;
    IM3FuncType type = ٹیگ->type;

    // a tag with results is a stack switching control tag, which is suspended
    // with rather than thrown
    إذا (type->عدد_Rets) {
        أرجع m3Err_typeMismatch;
    }
    إذا (i_argc != type->عدد_دلیلاں) {
        أرجع m3_Err_دلیل_گنتی_Mismatch;
    }
    // with nothing on the stack there is nothing to unwind, and the exception
    // would sit on the runtime's list until some later call ended
    إذا (io_runtime->callNesting == 0) {
        أرجع "m3_ThrowException called outside a host function";
    }

    M3Exception* exception = NewException(io_runtime, ٹیگ, i_argc);
    إذا (not exception) {
        أرجع m3Err_mallocFailed;
    }

    // the layout op_Throw gives a payload: 32-bit values zero-extended, floats
    // as their bit pattern
    لكل (u32 i = 0; i < i_argc; ++i) {
        ثابت فراغ* قدر = i_argptrs[i];

        بدل (ادھار_Type_دا(d_FuncArgType(type, i))) {
        حالة c_m3Type_i32:
        حالة c_m3Type_f32: {
            u32 بٹ;
            حافظہ_نقل(&بٹ, قدر, حجم(بٹ));
            exception->دلیلاں[i] = بٹ;
            اكسر;
        }
        حالة c_m3Type_i64:
        حالة c_m3Type_f64:
            حافظہ_نقل(&exception->دلیلاں[i], قدر, حجم(u64));
            اكسر;
        حالة c_m3Type_funcref:
        حالة c_m3Type_externref:
        حالة c_m3Type_exnref:
        حالة c_m3Type_contref:
            exception->دلیلاں[i] = (u64)(*(ثابت uintptr_t*)قدر);
            اكسر;
        افتراضي:
            FreeException(io_runtime, exception);
            أرجع "unknown argument type";
        }
    }

    io_runtime->pendingException = exception;

    أرجع m3Err_pendingException;
}


IM3_ٹیگ m3_Get_Exception_ٹیگ (IM3Runtime i_runtime)
{
    إذا (i_runtime and i_runtime->uncaughtException) {
        أرجع i_runtime->uncaughtException->ٹیگ;
    }

    أرجع عدم;
}


M3_نتیجہ m3_Get_Exception_دلیلاں (IM3Runtime i_runtime, uint32_t i_argc, ثابت فراغ* o_argptrs[])
{
    إذا (not i_runtime or not i_runtime->uncaughtException) {
        أرجع "no uncaught exception";
    }

    ثابت M3Exception* exception = i_runtime->uncaughtException;
    IM3FuncType        type      = exception->ٹیگ->type;

    إذا (i_argc != exception->عدد_دلیلاں) {
        أرجع m3_Err_دلیل_گنتی_Mismatch;
    }

    لكل (u32 i = 0; i < i_argc; ++i) {
        فراغ* قدر = (فراغ*)o_argptrs[i];
        u64   بٹ  = exception->دلیلاں[i];

        بدل (ادھار_Type_دا(d_FuncArgType(type, i))) {
        حالة c_m3Type_i32:
        حالة c_m3Type_f32: {
            u32 کم = (u32)بٹ;
            حافظہ_نقل(قدر, &کم, حجم(کم));
            اكسر;
        }
        حالة c_m3Type_i64:
        حالة c_m3Type_f64:
            حافظہ_نقل(قدر, &بٹ, حجم(بٹ));
            اكسر;
        حالة c_m3Type_funcref:
        حالة c_m3Type_externref:
        حالة c_m3Type_exnref:
        حالة c_m3Type_contref:
            *(uintptr_t*)قدر = (uintptr_t)بٹ;
            اكسر;
        افتراضي:
            أرجع "unknown argument type";
        }
    }

    أرجع m3Err_none;
}

#وإلا

M3_نتیجہ m3_ThrowException (IM3Runtime io_runtime, IM3_ٹیگ i_ٹیگ, uint32_t i_argc, ثابت فراغ* i_argptrs[])
{
    أرجع m3_Err_ٹیگ_Lookup_Failed;
}

IM3_ٹیگ m3_Get_Exception_ٹیگ (IM3Runtime i_runtime)
{
    أرجع عدم;
}

M3_نتیجہ m3_Get_Exception_دلیلاں (IM3Runtime i_runtime, uint32_t i_argc, ثابت فراغ* o_argptrs[])
{
    أرجع "no uncaught exception";
}

#جے_انت // d_m3HasExceptionHandling


#إذا d_m3_Guarded_حافظہ

// What RunCodeProtected hands to the platform layer to run, and what comes back
تعريفنوع بنية M3_Protected_لگاتار {
    pc_t            pc;
    IM3Runtime      runtime;
    M3_حافظہ_سرخی* mem;
    M3_نتیجہ        نتیجہ;
} M3_Protected_لگاتار;

ساكن
فراغ لگاتار_کوڈ_Body (فراغ* io_context)
{
    M3_Protected_لگاتار* لگاتار = (M3_Protected_لگاتار*)io_context;

#  إذا (d_m3EnableOpProfiling || d_m3EnableOpTracing)
    لگاتار->نتیجہ = (M3_نتیجہ)لگاتار_کوڈ(لگاتار->pc, (m3stack_t)لگاتار->runtime->ڈھیر, لگاتار->mem,
                                    d_m3_Op_Default_دلیلاں, d_m3_ادھار_Cstr);
#  وإلا
    لگاتار->نتیجہ = (M3_نتیجہ)لگاتار_کوڈ(لگاتار->pc, (m3stack_t)لگاتار->runtime->ڈھیر, لگاتار->mem,
                                    d_m3_Op_Default_دلیلاں);
#  endif
}

// The bounds check the load and store operations no longer do, done once here by the
// system instead: an address past the end of a memory reaches the uncommitted part
// of that memory's reservation, and the fault comes back as the trap the spec asks
// for. Nothing is left half-done by it - the Wasm stack and the memories are heap,
// and the caller unwinds the rest.
ساكن
M3_نتیجہ لگاتار_کوڈ_Protected (IM3Runtime i_runtime, pc_t i_pc, M3_حافظہ_سرخی* i_mem)
{
    // Memory operations in this build carry no bounds check, so code only runs where a
    // fault past the end of a memory can still be turned into a trap
    إذا (M3_UNLIKELY(not m3_Host_Guards_سرگرم())) {
        أرجع ErrorRuntime(m3_Err_حافظہ_Guards_Unavailable, i_runtime,
                            "another part of the process holds the fault signal");
    }

    M3_Protected_لگاتار لگاتار;
    لگاتار.pc      = i_pc;
    لگاتار.runtime = i_runtime;
    لگاتار.mem     = i_mem;
    لگاتار.نتیجہ  = m3Err_none;

    فراغ*  arena;
    size_t arena_بائٹاں;
    Guard_ArenaRange(&arena, &arena_بائٹاں);

    إذا (not m3_HostProtectedCall(لگاتار_کوڈ_Body, &لگاتار, arena, arena_بائٹاں)) {
        أرجع m3_Err_trap_باہرلا_دا_Bounds_حافظہ_Access;
    }

    أرجع لگاتار.نتیجہ;
}

#جے_انت // d_m3GuardedMemory


#إذا d_m3_Has_ڈھیر_Switching

// The function a suspended continuation stopped in: the callee of its innermost
// call frame, or the function it was entered with if it made no call. Frames are
// recorded innermost first, so that is the lowest-numbered call frame.
ساكن
IM3Function Continuation_InnermostFunction (IM3Continuation i_cont)
{
    لكل (u32 i = 0; i < i_cont->عدد_چوکھٹے; ++i) {
        إذا (i_cont->چوکھٹے[i].قسم == چوکھٹا_call && i_cont->چوکھٹے[i].call.function) {
            أرجع i_cont->چوکھٹے[i].call.function;
        }
    }

    أرجع i_cont->اندراج_Function;
}


// Where the host's next call puts its arguments and finds its results. A
// suspended root keeps its frames on the bottom of the runtime's stack, so a call
// made before it is resumed starts above the last of them rather than on top of
// them. The top is the innermost function's frame base plus everything that
// function can use. That base is the root's own sp, unless it stopped inside a
// resume: then the suspension point belongs to the resumed continuation, and the
// root's innermost frame is that resume.
فراغ Runtime_رکھو_Call_ڈھیر (IM3Runtime io_runtime)
{
    IM3Continuation جڑ = io_runtime->جڑ_Continuation;

    io_runtime->ڈھیر = io_runtime->origin_ڈھیر;

    إذا (not جڑ or جڑ->state != cont_suspended) {
        أرجع;
    }

    m3stack_t   sp       = جڑ->sp;
    IM3Function function = Continuation_InnermostFunction(جڑ);

    إذا (جڑ->عدد_چوکھٹے and جڑ->چوکھٹے[0].قسم == چوکھٹا_resume) {
        sp = جڑ->چوکھٹے[0].sp;
    }

    m3slot_t* ادھار = (m3slot_t*)io_runtime->origin_ڈھیر;
    m3slot_t* چوٹی  = (m3slot_t*)sp + (function ? function->max_ڈھیر_Slots : 0);

    إذا (چوٹی > ادھار + io_runtime->عدد_ڈھیر_Slots) {
        چوٹی = ادھار + io_runtime->عدد_ڈھیر_Slots;
    }

    io_runtime->ڈھیر = چوٹی;
}

#جے_انت // d_m3HasStackSwitching


// Run compiled code on the runtime's stack, bounding native recursion for the
// duration of the call. The outermost invocation establishes the stack limit;
// nested ones (an imported function calling back into Wasm) inherit it.
ساكن مضمن
M3_نتیجہ لگاتار_کوڈ_Checked (IM3Runtime i_runtime, IM3Function i_function)
{
    pc_t i_pc = i_function->compiled;

    // execution runs against the memory of the module the entry point belongs
    // to, not against some runtime-wide one
    M3_حافظہ_سرخی* _mem = Module_حافظہ_سرخی(i_function->module);

#إذا d_m3HasSnapshots
    إذا (M3_UNLIKELY(i_function->module->isUnusable)) {
        أرجع "a snapshot failed to restore into the module, so it cannot run";
    }
    i_function->module->has_لگاتار = true;
#جے_انت

    d_m3_ڈھیر_حد_بندی_Enter(i_runtime);

    // a host function calling back into Wasm does not change which call a trap
    // ends: that is still the one the host made first
    إذا (i_runtime->callNesting++ == 0) {
        i_runtime->entered = i_function;
    }
#إذا d_m3HasExceptionHandling
    // handler stacks don't nest across a call boundary: a host function calling
    // back into Wasm cannot be caught by a try_table its own caller entered
    u32 saved_Try_ڈونگھائی   = i_runtime->try_ڈونگھائی;
    i_runtime->try_ڈونگھائی = 0;
#جے_انت
#إذا d_m3_Has_ڈھیر_Switching
    // nor do continuations: the native frames on the far side of a host call
    // are not ours to record, so a suspend in here has no handler to reach and
    // no resume to unwind to
    IM3Continuation savedContinuation = i_runtime->سرگرم_Continuation;

    // A suspended root continuation is a paused program waiting to be resumed,
    // not scratch space. A call the host makes before resuming it gets a
    // context of its own, the way any other re-entry does, rather than having
    // the frames of the suspension reset out from under it.
    bool claim_جڑ = i_runtime->isSuspendable and not savedContinuation and
                     not (i_runtime->جڑ_Continuation and
                          i_runtime->جڑ_Continuation->state == cont_suspended);

    إذا (claim_جڑ and not i_runtime->جڑ_Continuation) {
        i_runtime->جڑ_Continuation = Continuation_New(i_runtime, i_function->funcType, i_function);

        إذا (i_runtime->جڑ_Continuation) {
            // The root borrows the runtime's stack and is not charged as cont.new.
            i_runtime->جڑ_Continuation->عدد_ڈھیر_Slots = i_runtime->عدد_ڈھیر_Slots;
        } وإلا {
            claim_جڑ = false;
        }
    }

    إذا (claim_جڑ) {
        i_runtime->سرگرم_Continuation              = i_runtime->جڑ_Continuation;
        i_runtime->جڑ_Continuation->اندراج_Function = i_function;
        i_runtime->جڑ_Continuation->state         = cont_running;
        i_runtime->جڑ_Continuation->عدد_چوکھٹے     = 0;
    } وإلا {
        i_runtime->سرگرم_Continuation = عدم;
    }
#جے_انت
#إذا d_m3_Guarded_حافظہ
    M3_نتیجہ نتیجہ = لگاتار_کوڈ_Protected(i_runtime, i_pc, _mem);
#وإلاإذا (d_m3EnableOpProfiling || d_m3EnableOpTracing)
    M3_نتیجہ نتیجہ = (M3_نتیجہ)لگاتار_کوڈ(i_pc, (m3stack_t)i_runtime->ڈھیر, _mem, d_m3_Op_Default_دلیلاں, d_m3_ادھار_Cstr);
#وإلا
    M3_نتیجہ نتیجہ = (M3_نتیجہ)لگاتار_کوڈ(i_pc, (m3stack_t)i_runtime->ڈھیر, _mem, d_m3_Op_Default_دلیلاں);
#جے_انت
#إذا d_m3_Has_ڈھیر_Switching
    إذا (claim_جڑ) {
        i_runtime->جڑ_Continuation->state =
          (نتیجہ == m3Err_continuationSuspended) ? cont_suspended : cont_returned;

        Runtime_رکھو_Call_ڈھیر(i_runtime);
    }
    i_runtime->سرگرم_Continuation = savedContinuation;
#جے_انت
#إذا d_m3HasExceptionHandling
    i_runtime->try_ڈونگھائی = saved_Try_ڈونگھائی;

    // an exception that reached the bottom of the call stack found no handler
    إذا (M3_UNLIKELY(نتیجہ == m3Err_pendingException)) {
        نتیجہ = m3Err_trapUncaughtException;
    }
    ریکارڈ_Uncaught_Exception(i_runtime, نتیجہ);

    // a paused invocation can still reach the exceptions it holds, so they wait
    // for it to finish
    إذا (i_runtime->callNesting == 1 and not m3_IsSuspended(i_runtime)) {
        FreeExceptions(i_runtime);
    }
#جے_انت
    i_runtime->callNesting--;
    d_m3_ڈھیر_حد_بندی_Leave(i_runtime);

    أرجع نتیجہ;
}

M3_نتیجہ m3_لگاتار_شروع (IM3Module io_module)
{
#جے_تعریف_شدہ FUZZING_BUILD_منوال_UNSAFE_FOR_PRODUCTION
    // Execution disabled for fuzzing builds
    أرجع m3Err_none;
#جے_انت

    M3_نتیجہ نتیجہ           = m3Err_none;
    i32      شروع_Function_Tmp = -1;

    إذا (io_module and io_module->شروع_Function >= 0) {
        IM3Function function = &io_module->functions[io_module->شروع_Function];

        إذا (not function->compiled) {
_           (CompileFunction(function));
        }

        IM3FuncType ftype = function->funcType;
        إذا (ftype->عدد_دلیلاں != 0 || ftype->عدد_Rets != 0) {
            _throw(m3_Err_دلیل_گنتی_Mismatch);
        }

        IM3Module  module  = function->module;
        IM3Runtime runtime = module->runtime;

        شروع_Function_Tmp = io_module->شروع_Function;

        io_module->شروع_Function = -1;

        نتیجہ = لگاتار_کوڈ_Checked(runtime, function);

        // a start function that suspended is not to be started again: resuming
        // is what finishes it
        إذا (نتیجہ and نتیجہ != m3Err_continuationSuspended) {
            io_module->شروع_Function = شروع_Function_Tmp;
            EXCEPTION_PRINT(نتیجہ);
            اذهب _catch;
        }
    }

    _catch: أرجع نتیجہ;
}

// TODO: deal with main + side-modules loading efforcement
M3_نتیجہ m3_لدو_Module (IM3Runtime io_runtime, IM3Module io_module)
{
    M3_نتیجہ نتیجہ = m3Err_none;

    إذا (M3_UNLIKELY(io_module->runtime)) {
        أرجع m3Err_moduleAlreadyLinked;
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
_   (Init_انکڑے_Segments(io_module));

    // Start func might use imported functions, which are not liked here yet,
    // so it will be called before a function call is attempted (in m3_FindFunction)

#جے_تعریف_شدہ DEBUG
    Module_Generate_ناں(io_module);
#جے_انت

    io_module->اگلا     = io_runtime->modules;
    io_runtime->modules = io_module;
    أرجع نتیجہ; // ok

_catch:
    // The runtime owns the module either way. Instantiation may already have
    // written this module's functions into a table another module owns, and the
    // spec keeps whatever it managed to do before the trap, so those entries
    // stay callable - retaining the module is what stops them dangling.
    //
    // It goes on the tail of the list rather than the head: it is not a module
    // anyone should find by name, only one whose functions may still be
    // reachable through someone else's table.
    io_module->اگلا = عدم;

    IM3Module* پوچھ = &io_runtime->modules;
    طالما (*پوچھ) {
        پوچھ = &(*پوچھ)->اگلا;
    }
    *پوچھ = io_module;

    أرجع نتیجہ;
}

IM3Global m3_لبھو_Global (IM3Module         io_module,
                         ثابت حرف* ثابت i_globalName)
{
    // Search exports
    IM3Global exported = Module_لبھو_Exported_Global(io_module, i_globalName);
    إذا (exported) {
        // a re-exported global is the one that was imported, so reads and
        // writes have to reach the cell that actually holds the value
        أرجع exported->resolved ? exported->resolved : exported;
    }

    // Search imports
    لكل (u32 i = 0; i < io_module->عدد_Globals; ++i) {
        IM3Global g = &io_module->globals[i];

        إذا (g->import.moduleUtf8 and g->import.میدان_Utf8) {
            إذا (strcmp(g->import.میدان_Utf8, i_globalName) == 0) {
                أرجع g->resolved ? g->resolved : g;
            }
        }
    }
    أرجع عدم;
}

M3_نتیجہ m3_GetGlobal (IM3Global      i_global,
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
#جے_انت
    افتراضي: أرجع m3Err_invalidTypeId;
    }

    o_قدر->type = (M3_قدر_Type)(i_global->type);
    أرجع m3Err_none;
}

M3_نتیجہ m3_سیٹ_Global (IM3Global            i_global,
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
#جے_انت
    افتراضي: أرجع m3Err_invalidTypeId;
    }

    أرجع m3Err_none;
}

M3_قدر_Type m3_GetGlobalType (IM3Global i_global)
{
    أرجع (i_global) ? (M3_قدر_Type)(i_global->type) : c_m3Type_none;
}


فراغ* v_لبھو_Function (IM3Module i_module, فراغ* i_info)
{
    ثابت حرف* ثابت i_name = (ثابت حرف*)i_info;

    // Prefer exported functions
    IM3Function exported = Module_لبھو_Exported_Function(i_module, i_name);
    إذا (exported) {
        أرجع exported;
    }

    // Search internal functions
    لكل (u32 i = 0; i < i_module->عدد_Functions; ++i) {
        IM3Function f = &i_module->functions[i];

        bool isImported = f->import.moduleUtf8 or f->import.میدان_Utf8;

        إذا (isImported) {
            استمر;
        }

        لكل (صحيح j = 0; j < f->عدد_ناں; j++) {
            إذا (f->ناں[j] and strcmp(f->ناں[j], i_name) == 0) {
                أرجع f;
            }
        }
    }

    أرجع عدم;
}


// Shared tail of the two lookups: a function is only usable once it has code.
ساكن
M3_نتیجہ Prepare_لبھیا_Function (IM3Function* o_function, IM3Function i_function)
{
    M3_نتیجہ نتیجہ = m3Err_none;

    إذا (not i_function->compiled) {
_       (CompileFunction(i_function))
    }

    _catch:
    *o_function = نتیجہ ? عدم : i_function;

    أرجع نتیجہ;
}


// Searches every module in the runtime, most recently loaded first. That is a
// guess once more than one module is loaded and two of them export the same
// name - m3_FindFunctionIn says which module is meant.
M3_نتیجہ m3_لبھو_Function (IM3Function* o_function, IM3Runtime i_runtime, ثابت حرف* ثابت i_functionName)
{
                                                                d_m3Assert (o_function and i_runtime and i_functionName);
    IM3Function function = عدم;

    إذا (not i_runtime->modules) {
        *o_function = عدم;
        أرجع "no modules loaded";
    }

    function = (IM3Function)ForEachModule(i_runtime, v_لبھو_Function, (فراغ*)i_functionName);

    إذا (not function) {
        *o_function = عدم;
        أرجع ErrorModule(m3Err_functionLookupFailed, i_runtime->modules, "'%s'", i_functionName);
    }

    أرجع Prepare_لبھیا_Function(o_function, function);
}


IM3Module m3_لبھو_Module (IM3Runtime i_runtime, ثابت حرف* ثابت i_moduleName)
{
    إذا (not i_runtime or not i_moduleName) {
        أرجع عدم;
    }

    // the list is newest-first, so a name registered twice names the newer one
    لكل (IM3Module m = i_runtime->modules; m; m = m->اگلا) {
        إذا (m->name and strcmp(m->name, i_moduleName) == 0) {
            أرجع m;
        }
    }

    أرجع عدم;
}


// Searches one module's exports, which is what naming a module means.
M3_نتیجہ m3_لبھو_Function_وچ (IM3Function* o_function, IM3Module i_module, ثابت حرف* ثابت i_functionName)
{
                                                                d_m3Assert (o_function and i_functionName);
    إذا (not i_module) {
        *o_function = عدم;
        أرجع m3Err_functionLookupFailed;      // ErrorModule would deref it
    }

    IM3Function function = (IM3Function)v_لبھو_Function(i_module, (فراغ*)i_functionName);

    إذا (not function) {
        *o_function = عدم;
        أرجع ErrorModule(m3Err_functionLookupFailed, i_module, "'%s'", i_functionName);
    }

    أرجع Prepare_لبھیا_Function(o_function, function);
}


M3_نتیجہ m3_Get_جدول_Function (IM3Function* o_function, IM3Module i_module, uint32_t i_اشاریہ)
{
_try {
    M3_جدول*    جدول_2;
    IM3Function function;

    _throwif("no table", i_module->عدد_Tables == 0);

    جدول_2 = i_module->tables[0];
    _throwif("function index out of range", i_اشاریہ >= جدول_2->ناپ);

    function = (IM3Function)جدول_2->elements[i_اشاریہ];

    إذا (function) {
        إذا (not function->compiled) {
_           (CompileFunction(function))
        }
    }

    *o_function = function;
} _catch:
    أرجع نتیجہ;
}


ساكن
M3_نتیجہ check_شروع_Function (IM3Module i_module)
{
    M3_نتیجہ نتیجہ = m3Err_none;                               d_m3Assert(i_module);

    // Check if start function needs to be called
    إذا (i_module->شروع_Function >= 0) {
        نتیجہ = m3_لگاتار_شروع(i_module);
    }

    أرجع نتیجہ;
}

uint32_t m3_Get_Arg_گنتی (IM3Function i_function)
{
    إذا (i_function) {
        IM3FuncType ft = i_function->funcType;
        إذا (ft) {
            أرجع ft->عدد_دلیلاں;
        }
    }
    أرجع 0;
}

uint32_t m3_Get_Ret_گنتی (IM3Function i_function)
{
    إذا (i_function) {
        IM3FuncType ft = i_function->funcType;
        إذا (ft) {
            أرجع ft->عدد_Rets;
        }
    }
    أرجع 0;
}


M3_قدر_Type m3_GetArgType (IM3Function i_function, uint32_t index)
{
    إذا (i_function) {
        IM3FuncType ft = i_function->funcType;
        إذا (ft and index < ft->عدد_دلیلاں) {
            أرجع (M3_قدر_Type)ادھار_Type_دا(d_FuncArgType(ft, index));
        }
    }
    أرجع c_m3Type_none;
}

M3_قدر_Type m3_GetRetType (IM3Function i_function, uint32_t index)
{
    إذا (i_function) {
        IM3FuncType ft = i_function->funcType;
        إذا (ft and index < ft->عدد_Rets) {
            أرجع (M3_قدر_Type)ادھار_Type_دا(d_FuncRetType(ft, index));
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
M3_نتیجہ Get_ڈھیر_اشارہ_گر_For_دلیلاں (IM3Function i_function, u8** o_ڈھیر)
{
    IM3Runtime  runtime = i_function->module->runtime;
    IM3FuncType ftype   = i_function->funcType;

    // the stack a paused invocation holds is not available to this one
    size_t ورتیا      = (size_t)((u8*)runtime->ڈھیر - (u8*)runtime->origin_ڈھیر);
    size_t needed    = ((size_t)ftype->عدد_Rets + ftype->عدد_دلیلاں) * حجم(u64);
    size_t دستیاب = (size_t)runtime->عدد_ڈھیر_Slots * حجم(m3slot_t) - ورتیا;

    إذا (needed > دستیاب) {
        أرجع m3_Err_trap_ڈھیر_حد_توں_ودھ;
    }

    u64* ڈھیر = (u64*)runtime->ڈھیر;

    ڈھیر += ftype->عدد_Rets;

    *o_ڈھیر = (u8*)ڈھیر;

    أرجع m3Err_none;
}


M3_نتیجہ m3_CallV (IM3Function i_function, ...)
{
    va_list ap;
    va_start(ap, i_function);
    M3_نتیجہ r = m3_CallVL(i_function, ap);
    va_end(ap);
    أرجع r;
}

ساكن
فراغ Report_Native_ڈھیر_Usage ()
{
#إذا d_m3_Log_Native_ڈھیر
    صحيح ڈھیر_ورتیا = m3_ڈھیر_Get_Max();
    fprintf(stderr, "Native stack used: %d\n", ڈھیر_ورتیا);
#جے_انت
}


M3_نتیجہ m3_CallVL (IM3Function i_function, va_list i_دلیلاں)
{
    IM3Runtime  runtime = i_function->module->runtime;
    IM3FuncType ftype   = i_function->funcType;
    M3_نتیجہ    نتیجہ  = m3Err_none;
    u8*         s       = عدم;

    إذا (!i_function->compiled) {
        أرجع m3_Err_missing_Compiled_کوڈ;
    }

#إذا d_m3_ریکارڈ_Backtraces
    ClearBacktrace(runtime);
#جے_انت

    m3_ڈھیر_Check_Init();

_   (check_شروع_Function(i_function->module))

_   (Get_ڈھیر_اشارہ_گر_For_دلیلاں(i_function, &s))

    لكل (u32 i = 0; i < ftype->عدد_دلیلاں; ++i) {
        بدل (d_FuncArgType(ftype, i)) {
        حالة c_m3Type_i32:
            *(i32*)(s) = va_arg(i_دلیلاں, i32);
            s += 8;
            اكسر;
        حالة c_m3Type_i64:
            *(i64*)(s) = va_arg(i_دلیلاں, i64);
            s += 8;
            اكسر;
        حالة c_m3Type_funcref:
        حالة c_m3Type_externref:
        حالة c_m3Type_exnref:
            *(uintptr_t*)(s) = va_arg(i_دلیلاں, uintptr_t);
            s += 8;
            اكسر;
#إذا d_m3HasFloat
        حالة c_m3Type_f32:
            *(f32*)(s) = (f32)va_arg(i_دلیلاں, f64);
            s += 8;
            اكسر; // f32 is passed as f64
        حالة c_m3Type_f64:
            *(f64*)(s) = va_arg(i_دلیلاں, f64);
            s += 8;
            اكسر;
#جے_انت
        افتراضي: أرجع "unknown argument type";
        }
    }

    نتیجہ = لگاتار_کوڈ_Checked(runtime, i_function);
    Report_Native_ڈھیر_Usage();

    runtime->آخری_Called = نتیجہ ? عدم : i_function;

    _catch: أرجع نتیجہ;
}

M3_نتیجہ m3_Call (IM3Function i_function, uint32_t i_argc, ثابت فراغ* i_argptrs[])
{
    IM3Runtime  runtime = i_function->module->runtime;
    IM3FuncType ftype   = i_function->funcType;
    M3_نتیجہ    نتیجہ  = m3Err_none;
    u8*         s       = عدم;

    إذا (i_argc != ftype->عدد_دلیلاں) {
        أرجع m3_Err_دلیل_گنتی_Mismatch;
    }
    إذا (!i_function->compiled) {
        أرجع m3_Err_missing_Compiled_کوڈ;
    }

#إذا d_m3_ریکارڈ_Backtraces
    ClearBacktrace(runtime);
#جے_انت

    m3_ڈھیر_Check_Init();

_   (check_شروع_Function(i_function->module))

_   (Get_ڈھیر_اشارہ_گر_For_دلیلاں(i_function, &s))

    لكل (u32 i = 0; i < ftype->عدد_دلیلاں; ++i) {
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
#جے_انت
        افتراضي: أرجع "unknown argument type";
        }
    }

    نتیجہ = لگاتار_کوڈ_Checked(runtime, i_function);

    Report_Native_ڈھیر_Usage();

    runtime->آخری_Called = نتیجہ ? عدم : i_function;

    _catch: أرجع نتیجہ;
}

// Argument parsing for m3_CallArgv. Strict on purpose: the whole string has to
// be consumed, so "12abc" is rejected rather than read as 12, and an empty or
// unparsable argument is an error rather than the zero that strtoul with a
// NULL end pointer used to hand back. See wasm3/wasm3#367.
ساكن
M3_نتیجہ ParseArgInteger (ccstr_t i_arg, u32 i_عدد_بٹ, u64* o_قدر)
{
    إذا (not i_arg or not *i_arg) {
        أرجع "empty argument";
    }

    // strtoull would skip leading space, but trailing space is rejected below;
    // accepting one and not the other would just be confusing
    إذا (خلا_اے((بنانشانی حرف)*i_arg)) {
        أرجع "argument is not a number";
    }

    حرف* سرا = عدم;
    u64   قدر;

    errno = 0;

    // an argument may be spelled signed or unsigned: -1 and 4294967295 name the
    // same i32
    إذا (*i_arg == '-') {
        i64 signed_قدر = strtoll(i_arg, &سرا, 10);

        إذا (i_عدد_بٹ == 32 and (signed_قدر < INT32_MIN or signed_قدر > INT32_MAX)) {
            أرجع "argument out of range";
        }

        قدر = (u64)signed_قدر;
    } وإلا {
        قدر = strtoull(i_arg, &سرا, 10);

        إذا (i_عدد_بٹ == 32 and قدر > UINT32_MAX) {
            أرجع "argument out of range";
        }
    }

    إذا (errno == ERANGE) {
        أرجع "argument out of range";
    }

    إذا (سرا == i_arg or *سرا) {
        أرجع "argument is not a number";
    }

    *o_قدر = قدر;

    أرجع m3Err_none;
}


#إذا d_m3HasFloat
ساكن
M3_نتیجہ ParseArgFloat (ccstr_t i_arg, f64* o_قدر)
{
    إذا (not i_arg or not *i_arg) {
        أرجع "empty argument";
    }

    إذا (خلا_اے((بنانشانی حرف)*i_arg)) {
        أرجع "argument is not a number";
    }

    حرف* سرا = عدم;

    errno = 0;

    f64 قدر = strtod(i_arg, &سرا);

    إذا (سرا == i_arg or *سرا) {
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
#جے_انت


// A reference argument is either the null reference or a host handle written as
// an integer.
ساكن
M3_نتیجہ ParseArgReference (ccstr_t i_arg, u64* o_قدر)
{
    إذا (i_arg and strcmp(i_arg, "null") == 0) {
        *o_قدر = 0;
        أرجع m3Err_none;
    }

    أرجع ParseArgInteger(i_arg, 64, o_قدر);
}


M3_نتیجہ m3_CallArgv (IM3Function i_function, uint32_t i_argc, ثابت حرف* i_argv[])
{
    IM3FuncType ftype   = i_function->funcType;
    IM3Runtime  runtime = i_function->module->runtime;
    M3_نتیجہ    نتیجہ  = m3Err_none;
    u8*         s       = عدم;

    إذا (i_argc != ftype->عدد_دلیلاں) {
        أرجع m3_Err_دلیل_گنتی_Mismatch;
    }
    إذا (!i_function->compiled) {
        أرجع m3_Err_missing_Compiled_کوڈ;
    }

#إذا d_m3_ریکارڈ_Backtraces
    ClearBacktrace(runtime);
#جے_انت

    m3_ڈھیر_Check_Init();

_   (check_شروع_Function(i_function->module))

_   (Get_ڈھیر_اشارہ_گر_For_دلیلاں(i_function, &s))

    لكل (u32 i = 0; i < ftype->عدد_دلیلاں; ++i) {
        u64 قدر = 0;
#إذا d_m3HasFloat
        f64 fvalue = 0;
#جے_انت
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
#جے_انت
        افتراضي: _throw("unknown argument type");
        }
    }

    نتیجہ = لگاتار_کوڈ_Checked(runtime, i_function);

    Report_Native_ڈھیر_Usage();

    runtime->آخری_Called = نتیجہ ? عدم : i_function;

    _catch: أرجع نتیجہ;
}


//u8 * AlignStackPointerTo64Bits (const u8 * i_stack)
//{
//    uintptr_t ptr = (uintptr_t) i_stack;
//    return (u8 *) ((ptr + 7) & ~7);
//}


M3_نتیجہ m3_GetResults (IM3Function i_function, uint32_t i_retc, ثابت فراغ* o_retptrs[])
{
    IM3FuncType ftype   = i_function->funcType;
    IM3Runtime  runtime = i_function->module->runtime;

    إذا (i_retc != ftype->عدد_Rets) {
        أرجع m3_Err_دلیل_گنتی_Mismatch;
    }
    إذا (i_function != runtime->آخری_Called) {
        أرجع "function not called";
    }

    u8* s = (u8*)runtime->ڈھیر;

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
#جے_انت
        افتراضي: أرجع "unknown return type";
        }
    }
    أرجع m3Err_none;
}

M3_نتیجہ m3_GetResultsV (IM3Function i_function, ...)
{
    va_list ap;
    va_start(ap, i_function);
    M3_نتیجہ r = m3_GetResultsVL(i_function, ap);
    va_end(ap);
    أرجع r;
}

M3_نتیجہ m3_GetResultsVL (IM3Function i_function, va_list o_rets)
{
    IM3Runtime  runtime = i_function->module->runtime;
    IM3FuncType ftype   = i_function->funcType;

    إذا (i_function != runtime->آخری_Called) {
        أرجع "function not called";
    }

    u8* s = (u8*)runtime->ڈھیر;
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
#جے_انت
        افتراضي: أرجع "unknown argument type";
        }
    }
    أرجع m3Err_none;
}

فراغ Release_کوڈ_ورقہ_No_Track (IM3Runtime i_runtime, IM3_کوڈ_ورقہ i_کوڈ_ورقہ)
{
    إذا (i_کوڈ_ورقہ) {
        IM3_کوڈ_ورقہ* list;

        bool ورقہ_بھریا = (عدد_Free_سطریں(i_کوڈ_ورقہ) < d_m3_کوڈ_ورقہ_Free_سطریں_حد);
        إذا (ورقہ_بھریا) {
            list = &i_runtime->ورقے_بھریا;
        } وإلا {
            list = &i_runtime->ورقے_Open;
        }

        ڈالو_کوڈ_ورقہ(list, i_کوڈ_ورقہ);                         m3log (emit, "release page: %d to queue: '%s'", i_کوڈ_ورقہ->info.sequence, ورقہ_بھریا ? "full" : "open")
    }
}


IM3_کوڈ_ورقہ Acquire_کوڈ_ورقہ_With_سمرتھا (IM3Runtime i_runtime, u32 i_min_سطر_گنتی)
{
    IM3_کوڈ_ورقہ ورقہ = Remove_کوڈ_ورقہ_دا_سمرتھا(&i_runtime->ورقے_Open, i_min_سطر_گنتی);

    إذا (not ورقہ) {
        ورقہ = Environment_Acquire_کوڈ_ورقہ(i_runtime->environment, i_min_سطر_گنتی);

        إذا (not ورقہ) {
            ورقہ = New_کوڈ_ورقہ(i_runtime, i_min_سطر_گنتی);
        }

        إذا (ورقہ) {
            i_runtime->عدد_کوڈ_ورقے++;
        }
    }

    إذا (ورقہ) {                                                  m3log (emit, "acquire page: %d", ورقہ->info.sequence);
        i_runtime->عدد_سرگرم_کوڈ_ورقے++;
    }

    أرجع ورقہ;
}


IM3_کوڈ_ورقہ Acquire_کوڈ_ورقہ (IM3Runtime i_runtime)
{
    أرجع Acquire_کوڈ_ورقہ_With_سمرتھا(i_runtime, d_m3_کوڈ_ورقہ_Free_سطریں_حد);
}


فراغ Release_کوڈ_ورقہ (IM3Runtime i_runtime, IM3_کوڈ_ورقہ i_کوڈ_ورقہ)
{
    إذا (i_کوڈ_ورقہ) {
        Release_کوڈ_ورقہ_No_Track(i_runtime, i_کوڈ_ورقہ);
        i_runtime->عدد_سرگرم_کوڈ_ورقے--;

#إذا defined(DEBUG)
        u32 عدد_Open = گنتی_کوڈ_ورقے(i_runtime->ورقے_Open);
        u32 عدد_بھریا = گنتی_کوڈ_ورقے(i_runtime->ورقے_بھریا);

        m3log(runtime, "runtime: %p; open-pages: %d; full-pages: %d; active: %d; total: %d", i_runtime, عدد_Open, عدد_بھریا, i_runtime->عدد_سرگرم_کوڈ_ورقے, i_runtime->عدد_کوڈ_ورقے);

        d_m3Assert(عدد_Open + عدد_بھریا + i_runtime->عدد_سرگرم_کوڈ_ورقے == i_runtime->عدد_کوڈ_ورقے);

#  إذا d_m3_Log_کوڈ_ورقے
        dump_کوڈ_ورقہ(i_کوڈ_ورقہ, /* startPC: */ عدم);
#  endif
#جے_انت
    }
}


#إذا d_m3VerboseErrorMessages
M3_نتیجہ m3Error (M3_نتیجہ i_نتیجہ, IM3Runtime i_runtime, IM3Module i_module, IM3Function i_function,
                  ثابت حرف* ثابت i_file, u32 i_سطر_عدد, ثابت حرف* ثابت i_error_سنیہا, ...)
{
    إذا (i_runtime) {
        i_runtime->error         = (M3ErrorInfo){ .نتیجہ = i_نتیجہ, .runtime = i_runtime, .module = i_module, .function = i_function, .file = i_file, .سطر_2 = i_سطر_عدد };
        i_runtime->error.سنیہا = i_runtime->error_سنیہا;

        va_list دلیلاں;
        va_start(دلیلاں, i_error_سنیہا);
        vsnprintf(i_runtime->error_سنیہا, حجم(i_runtime->error_سنیہا), i_error_سنیہا, دلیلاں);
        va_end(دلیلاں);
    }

    أرجع i_نتیجہ;
}
#جے_انت


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
        i_runtime->error.سنیہا = "";
    }
}

uint8_t* m3_Get_حافظہ (IM3Module i_module, size_t* o_حافظہ_ناپ_وچ_بائٹاں, uint32_t i_حافظہ_اشاریہ)
{
    uint8_t* حافظہ = عدم;
    size_t   ناپ   = 0;

    إذا (i_module and i_حافظہ_اشاریہ < i_module->عدد_Memories) {
        IM3_حافظہ mem = i_module->memories[i_حافظہ_اشاریہ];

        إذا (mem->mallocated) {
            ناپ = mem->mallocated->لمائی;

            إذا (ناپ) {
                حافظہ = m3_Mem_انکڑے(mem->mallocated);
            }
        }
    }

    إذا (o_حافظہ_ناپ_وچ_بائٹاں) {
        *o_حافظہ_ناپ_وچ_بائٹاں = ناپ;
    }

    أرجع حافظہ;
}


size_t m3_Get_حافظہ_ناپ (IM3Module i_module, uint32_t i_حافظہ_اشاریہ)
{
    إذا (not i_module or i_حافظہ_اشاریہ >= i_module->عدد_Memories) {
        أرجع 0;
    }

    IM3_حافظہ mem = i_module->memories[i_حافظہ_اشاریہ];

    أرجع mem->mallocated ? mem->mallocated->لمائی : 0;
}


size_t m3_Get_حافظہ_ناپ_At (ثابت فراغ* i_حافظہ)
{
    إذا (not i_حافظہ) {
        أرجع 0;
    }

    // the header sits immediately before the data it describes
    ثابت M3_حافظہ_سرخی* سرخی = ((ثابت M3_حافظہ_سرخی*)i_حافظہ) - 1;

    أرجع سرخی->لمائی;
}


M3_نتیجہ m3_لبھو_Exported_حافظہ (IM3Module i_module, cstr_t i_name, u32* o_حافظہ_اشاریہ)
{
    إذا (not i_module or not i_name or not o_حافظہ_اشاریہ) {
        أرجع m3_Err_unknown_حافظہ;
    }

    ثابت M3Export* e = Module_لبھو_Export(i_module, d_external_قسم_حافظہ, i_name);

    إذا (not e) {
        أرجع m3_Err_unknown_حافظہ;
    }

    *o_حافظہ_اشاریہ = e->index;
    أرجع m3Err_none;
}


bool Module_HasLinkedHostImport (IM3Module i_module, cstr_t i_importModule)
{
    لكل (u32 i = 0; i < i_module->عدد_Functions; ++i) {
        IM3Function f = &i_module->functions[i];

        // hostMemory is set when a host function is bound to the import, so it
        // is also the mark of one - see CompileRawFunction
        إذا (not f->host_حافظہ or f->wasm or f->resolved) {
            استمر;
        }

        إذا (f->import.moduleUtf8 and strcmp(f->import.moduleUtf8, i_importModule) == 0) {
            أرجع true;
        }
    }

    أرجع false;
}


M3_نتیجہ m3_Bind_Import_حافظہ (IM3Module io_module, cstr_t i_importModule, u32 i_حافظہ_اشاریہ)
{
    إذا (not io_module or not i_importModule) {
        أرجع m3Err_moduleNotLinked;
    }

    إذا (i_حافظہ_اشاریہ >= io_module->عدد_Memories) {
        أرجع m3_Err_unknown_حافظہ;
    }

    // the memory itself, not the index: linking may have pointed this slot at
    // another module's memory, and growing one reallocates behind it
    IM3_حافظہ حافظہ = io_module->memories[i_حافظہ_اشاریہ];

    ثابت bool wildcardModule = (strcmp(i_importModule, "*") == 0);

    لكل (u32 i = 0; i < io_module->عدد_Functions; ++i) {
        IM3Function f = &io_module->functions[i];

        // only an import that a host function was bound to has a memory to
        // pin: one with a body of its own reads memory through the interpreter,
        // and one resolved to another module's export runs that module's code
        إذا (not f->host_حافظہ or f->wasm or f->resolved) {
            استمر;
        }

        إذا (wildcardModule or (f->import.moduleUtf8 and strcmp(f->import.moduleUtf8, i_importModule) == 0)) {
            f->host_حافظہ = حافظہ;
        }
    }

    أرجع m3Err_none;
}


M3BacktraceInfo* m3_GetBacktrace (IM3Runtime i_runtime)
{
#إذا d_m3_ریکارڈ_Backtraces
    أرجع &i_runtime->backtrace;
#وإلا
    أرجع عدم;
#جے_انت
}


//-------------------------------------------------------------------------------------------------------------------------------
//  snapshots & suspendable execution
//-------------------------------------------------------------------------------------------------------------------------------

فراغ m3_سیٹ_Suspendable (IM3Runtime io_runtime, bool i_suspendable)
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
#إذا d_m3_Has_ڈھیر_Switching
    إذا (i_runtime->جڑ_Continuation && i_runtime->جڑ_Continuation->state == cont_suspended) {
        أرجع true;
    }
#جے_انت
    أرجع false;
}

M3_نتیجہ m3_ResumeRuntime (IM3Runtime io_runtime)
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
    إذا (io_runtime->جڑ_Continuation and io_runtime->جڑ_Continuation->اندراج_Function and
        io_runtime->جڑ_Continuation->اندراج_Function->module->isUnusable) {
        أرجع "a snapshot failed to restore into the module, so it cannot run";
    }
#جے_انت
#إذا d_m3_Has_ڈھیر_Switching
    إذا (!io_runtime->جڑ_Continuation || io_runtime->جڑ_Continuation->state != cont_suspended) {
        أرجع m3Err_none;
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

#  إذا d_m3HasExceptionHandling
    إذا (M3_UNLIKELY(نتیجہ == m3Err_pendingException)) {
        نتیجہ = m3Err_trapUncaughtException;
    }
    ریکارڈ_Uncaught_Exception(io_runtime, نتیجہ);
#  endif

    io_runtime->callNesting--;

#  إذا d_m3HasExceptionHandling
    إذا (نتیجہ != m3Err_continuationSuspended and io_runtime->callNesting == 0) {
        FreeExceptions(io_runtime);
    }
#  endif

    // the results are where the paused call left room for them, so a finished
    // resume can be asked for them the way the call itself could have been
    io_runtime->آخری_Called = (نتیجہ == m3Err_none) ? io_runtime->جڑ_Continuation->اندراج_Function : عدم;

    Runtime_رکھو_Call_ڈھیر(io_runtime);
    أرجع نتیجہ;
#وإلا
    أرجع m3Err_none;
#جے_انت
}
