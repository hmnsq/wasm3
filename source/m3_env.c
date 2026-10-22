//
//  m3_env.c
//
//  Created by Steven Massey on 4/19/19.
//  Copyright © 2019 Steven Massey. All rights reserved.
//

#ਅਨ੍ਤਰ੍ਭਾਵ <stdarg.h>
#ਅਨ੍ਤਰ੍ਭਾਵ <limits.h>
#ਅਨ੍ਤਰ੍ਭਾਵ <errno.h>
#ਅਨ੍ਤਰ੍ਭਾਵ <float.h>
#ਅਨ੍ਤਰ੍ਭਾਵ <ctype.h>

#ਅਨ੍ਤਰ੍ਭਾਵ "m3_env.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "m3_bind.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "m3_deterministic.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "m3_compile.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "m3_host.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "m3_exception.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "m3_info.h"


IM3Environment m3_NewEnvironment ()
{
    IM3Environment env = m3_AllocStruct(M3Environment);

    ਜੇ (env) {
        _try
        {
            // create FuncTypes for all simple block return ValueTypes.
            // v128 is skipped: it parses as a slot but has no operations.
            ਲਈ (u8 t = c_m3Type_none; t < c_m3_Type_ਗਿਣਤੀ; t++) {
                ਜੇ (t == c_m3Type_v128) {
                    ਜਾਰੀ;
                }

                IM3FuncType ftype;
_               (AllocFuncType(&ftype, 1));

                ftype->ਅੰਕ_ਦਲੀਲਾਂ  = 0;
                ftype->ਅੰਕ_Rets  = (t == c_m3Type_none) ? 0 : 1;
                ftype->types[0] = t;

                Environment_ਜੋੜੋ_Func_Type(env, &ftype);

                env->retFuncTypes[t] = ftype;
            }
        }

        _catch:
        ਜੇ (ਨਤੀਜਾ) {
            m3_FreeEnvironment(env);
            env = ਨਿਰਾ;
        }
    }

    ਵਾਪਸ env;
}


ਖਾਲੀ Environment_Release (IM3Environment i_environment)
{
    IM3FuncType ftype = i_environment->funcTypes;

    ਜਦੋਂ (ftype) {
        IM3FuncType ਅਗਲਾ = ftype->ਅਗਲਾ;
        m3_Free(ftype);
        ftype = ਅਗਲਾ;
    }

    m3log(runtime, "freeing %d pages from environment", ਗਿਣਤੀ_ਕੋਡ_ਪੰਨੇ(i_environment->ਪੰਨੇ_Released));
    Free_ਕੋਡ_ਪੰਨੇ(&i_environment->ਪੰਨੇ_Released);
}


ਖਾਲੀ m3_FreeEnvironment (IM3Environment i_environment)
{
    ਜੇ (i_environment) {
        Environment_Release(i_environment);
        m3_Free(i_environment);
    }
}


ਖਾਲੀ m3_ਸੈੱਟ_Custom_Section_Handler (IM3Environment i_environment, M3SectionHandler i_handler)
{
    ਜੇ (i_environment) {
        i_environment->customSectionHandler = i_handler;
    }
}


// returns the same io_funcType or replaces it with an equivalent that's already in the type linked list
M3_ਨਤੀਜਾ Environment_ਜੋੜੋ_Func_Type (IM3Environment i_environment, IM3FuncType* io_funcType)
{
    IM3FuncType ਜੋੜੋ_Type = *io_funcType;
    IM3FuncType newType = i_environment->funcTypes;

    ਜਦੋਂ (newType) {
        ਜੇ (not newType->ਵਿੱਚ_Rec_ਸਮੂਹ and AreFuncTypesEqual(newType, ਜੋੜੋ_Type)) {
            m3_Free(ਜੋੜੋ_Type);
            ਰੋਕੋ;
        }

        newType = newType->ਅਗਲਾ;
    }

    ਜੇ (newType == ਨਿਰਾ) {
        // a type index has to fit in the heap type field of an m3type_t
        ਜੇ (i_environment->ਅੰਕ_Func_Types >= d_m3_Max_Sane_Types_ਗਿਣਤੀ) {
            m3_Free(ਜੋੜੋ_Type);
            *io_funcType = ਨਿਰਾ;
            ਵਾਪਸ "too many distinct function types";
        }

        newType                  = ਜੋੜੋ_Type;
        newType->canonical_ਸੂਚਕਾਂਕ  = i_environment->ਅੰਕ_Func_Types++;
        newType->ਅਗਲਾ            = i_environment->funcTypes;
        i_environment->funcTypes = newType;
    }

    *io_funcType = newType;

    ਵਾਪਸ m3Err_none;
}


// Adds a type without looking for an equal one first, and hands back the index
// it was given.
//
// Members of a recursive group do not get to share an entry with a type that
// merely looks like them: the group is their identity, so two groups spelled
// the same way are still two groups, and a type inside one is never the type
// outside it that matches field for field. Reserving the indices up front is
// also what lets the members refer to each other while they are being read.
M3_ਨਤੀਜਾ Environment_ReserveFuncTypes (IM3Environment i_environment, u32 i_ਗਿਣਤੀ, u16* o_ਪਹਿਲਾ_ਸੂਚਕਾਂਕ)
{
    ਜੇ ((u64)i_environment->ਅੰਕ_Func_Types + i_ਗਿਣਤੀ > d_m3_Max_Sane_Types_ਗਿਣਤੀ) {
        ਵਾਪਸ "too many distinct function types";
    }

    *o_ਪਹਿਲਾ_ਸੂਚਕਾਂਕ = i_environment->ਅੰਕ_Func_Types;
    i_environment->ਅੰਕ_Func_Types += (u16)i_ਗਿਣਤੀ;

    ਵਾਪਸ m3Err_none;
}


ਖਾਲੀ Environment_AdoptFuncType (IM3Environment i_environment, IM3FuncType i_funcType, u16 i_canonical_ਸੂਚਕਾਂਕ)
{
    i_funcType->ਵਿੱਚ_Rec_ਸਮੂਹ     = true;
    i_funcType->canonical_ਸੂਚਕਾਂਕ = i_canonical_ਸੂਚਕਾਂਕ;
    i_funcType->ਅਗਲਾ           = i_environment->funcTypes;
    i_environment->funcTypes   = i_funcType;
}


IM3_ਕੋਡ_ਪੰਨਾ Remove_ਕੋਡ_ਪੰਨਾ_ਦਾ_ਸਮਰੱਥਾ (M3_ਕੋਡ_ਪੰਨਾ** io_list, u32 i_minimum_ਸਤਰ_ਗਿਣਤੀ)
{
    IM3_ਕੋਡ_ਪੰਨਾ prev = ਨਿਰਾ;
    IM3_ਕੋਡ_ਪੰਨਾ ਪੰਨਾ = *io_list;

    ਜਦੋਂ (ਪੰਨਾ) {
        ਜੇ (ਅੰਕ_Free_ਸਤਰਾਂ(ਪੰਨਾ) >= i_minimum_ਸਤਰ_ਗਿਣਤੀ) {             d_m3Assert (ਪੰਨਾ->info.usage_ਗਿਣਤੀ == 0);
            IM3_ਕੋਡ_ਪੰਨਾ ਅਗਲਾ = ਪੰਨਾ->info.ਅਗਲਾ;
            ਜੇ (prev) {
                prev->info.ਅਗਲਾ = ਅਗਲਾ; // mid-list
            } ਨਹੀਂਤਾਂ {
                *io_list = ਅਗਲਾ;       // front of list
            }

            ਰੋਕੋ;
        }

        prev = ਪੰਨਾ;
        ਪੰਨਾ = ਪੰਨਾ->info.ਅਗਲਾ;
    }

    ਵਾਪਸ ਪੰਨਾ;
}


IM3_ਕੋਡ_ਪੰਨਾ Environment_Acquire_ਕੋਡ_ਪੰਨਾ (IM3Environment i_environment, u32 i_minimum_ਸਤਰ_ਗਿਣਤੀ)
{
    ਵਾਪਸ Remove_ਕੋਡ_ਪੰਨਾ_ਦਾ_ਸਮਰੱਥਾ(&i_environment->ਪੰਨੇ_Released, i_minimum_ਸਤਰ_ਗਿਣਤੀ);
}


ਖਾਲੀ Environment_Release_ਕੋਡ_ਪੰਨੇ (IM3Environment i_environment, IM3_ਕੋਡ_ਪੰਨਾ i_ਕੋਡ_ਪੰਨਾ_List)
{
    IM3_ਕੋਡ_ਪੰਨਾ ਸਿਰਾ = i_ਕੋਡ_ਪੰਨਾ_List;

    ਜਦੋਂ (ਸਿਰਾ) {
        ਸਿਰਾ->info.ਸਤਰ_ਸੂਚਕਾਂਕ = 0; // reset page
#ਜੇ d_m3_ਰਿਕਾਰਡ_Backtraces
        ਸਿਰਾ->info.mapping->ਮਾਪ = 0;
#ਜੇ_ਅੰਤ // d_m3RecordBacktraces

        IM3_ਕੋਡ_ਪੰਨਾ ਅਗਲਾ = ਸਿਰਾ->info.ਅਗਲਾ;
        ਜੇ (not ਅਗਲਾ) {
            ਰੋਕੋ;
        }

        ਸਿਰਾ = ਅਗਲਾ;
    }

    ਜੇ (ਸਿਰਾ) {
        // push list to front
        ਸਿਰਾ->info.ਅਗਲਾ               = i_environment->ਪੰਨੇ_Released;
        i_environment->ਪੰਨੇ_Released = i_ਕੋਡ_ਪੰਨਾ_List;
    }
}


IM3Runtime m3_NewRuntime (IM3Environment i_environment, u32 i_ਢੇਰ_ਮਾਪ_ਵਿੱਚ_ਬਾਈਟਾਂ, ਖਾਲੀ* i_userdata)
{
    IM3Runtime runtime = m3_AllocStruct(M3Runtime);

    ਜੇ (runtime) {
        m3_ResetErrorInfo(runtime);

        runtime->environment = i_environment;
        runtime->userdata    = i_userdata;

        Deterministic_InitRuntime(runtime);

        runtime->origin_ਢੇਰ = m3_Malloc("Wasm Stack", i_ਢੇਰ_ਮਾਪ_ਵਿੱਚ_ਬਾਈਟਾਂ + 4 * ਆਕਾਰ(m3slot_t)); // TODO: more precise stack checks

        ਜੇ (runtime->origin_ਢੇਰ) {
            runtime->ਢੇਰ         = runtime->origin_ਢੇਰ;
            runtime->ਅੰਕ_ਢੇਰ_Slots = i_ਢੇਰ_ਮਾਪ_ਵਿੱਚ_ਬਾਈਟਾਂ / ਆਕਾਰ(m3slot_t);          m3log (runtime, "new stack: %p, slots: %u", runtime->origin_ਢੇਰ, runtime->ਅੰਕ_ਢੇਰ_Slots);
        } ਨਹੀਂਤਾਂ {
            m3_Free(runtime);
        }
    }

    ਵਾਪਸ runtime;
}

ਖਾਲੀ m3_ਸੈੱਟ_Validation (IM3Runtime i_runtime, bool i_enable)
{
#ਜੇ d_m3EnableValidation
    ਜੇ (i_runtime) {
        i_runtime->skipValidation = not i_enable;
    }
#ਨਹੀਂਤਾਂ
    (ਖਾਲੀ)i_runtime;
    (ਖਾਲੀ)i_enable;              // nothing to skip: the validator was compiled out
#ਜੇ_ਅੰਤ
}

M3_ਨਤੀਜਾ m3_ਸੈੱਟ_Resource_ਹੱਦਬੰਦੀ (IM3Runtime i_runtime, M3_Resource_ਹੱਦਬੰਦੀ i_ਹੱਦਬੰਦੀ, uint64_t i_ਮੁੱਲ)
{
    ਜੇ (not i_runtime) {
        ਵਾਪਸ m3Err_mallocFailed;
    }
    ਚੋਣ (i_ਹੱਦਬੰਦੀ) {
    ਹਾਲਤ c_m3_ਹੱਦਬੰਦੀ_ਸਮ੍ਰਿਤੀ_ਬਾਈਟਾਂ:
        ਜੇ (i_ਮੁੱਲ and i_ਮੁੱਲ < i_runtime->ਸਮ੍ਰਿਤੀ_ਬਾਈਟਾਂ_ਵਰਤਿਆ) {
            ਵਾਪਸ m3_Err_resource_ਹੱਦਬੰਦੀ_ਹੇਠਾਂ_Usage;
        }
        i_runtime->ਸਮ੍ਰਿਤੀ_ਬਾਈਟਾਂ_ਹੱਦਬੰਦੀ = i_ਮੁੱਲ;
        ਵਾਪਸ m3Err_none;
    ਹਾਲਤ c_m3_ਹੱਦਬੰਦੀ_ਸਾਰਣੀ_Elements:
        ਜੇ (i_ਮੁੱਲ and i_ਮੁੱਲ < i_runtime->ਸਾਰਣੀ_Elements_ਵਰਤਿਆ) {
            ਵਾਪਸ m3_Err_resource_ਹੱਦਬੰਦੀ_ਹੇਠਾਂ_Usage;
        }
        i_runtime->ਸਾਰਣੀ_Elements_ਹੱਦਬੰਦੀ = i_ਮੁੱਲ;
        ਵਾਪਸ m3Err_none;
#ਜੇ d_m3_Has_ਢੇਰ_Switching
    ਹਾਲਤ c_m3_ਹੱਦਬੰਦੀ_Continuations:
        ਜੇ (i_ਮੁੱਲ and i_ਮੁੱਲ < i_runtime->continuationsAllocated) {
            ਵਾਪਸ m3_Err_resource_ਹੱਦਬੰਦੀ_ਹੇਠਾਂ_Usage;
        }
        i_runtime->continuations_ਹੱਦਬੰਦੀ = (u32)M3_MIN(i_ਮੁੱਲ, (u64)UINT32_MAX);
        ਜਦੋਂ (i_ਮੁੱਲ and i_runtime->val_ਢੇਰ_Pool_ਗਿਣਤੀ > i_ਮੁੱਲ - i_runtime->continuationsAllocated) {
            m3_Free(i_runtime->val_ਢੇਰ_Pool[--i_runtime->val_ਢੇਰ_Pool_ਗਿਣਤੀ]);
        }
        ਵਾਪਸ m3Err_none;
#ਨਹੀਂਤਾਂ
    ਹਾਲਤ c_m3_ਹੱਦਬੰਦੀ_Continuations: ਵਾਪਸ m3_Err_resource_ਹੱਦਬੰਦੀ_Not_Supported;
#ਜੇ_ਅੰਤ
    ਹਾਲਤ c_m3_ਹੱਦਬੰਦੀ_Gas_Units:
#ਜੇ d_m3HasGasMetering
        i_runtime->gas_ਹੱਦਬੰਦੀ = i_runtime->gas_ਬਚਿਆ = (i64)M3_MIN(i_ਮੁੱਲ, (u64)INT64_MAX);
        ਵਾਪਸ m3Err_none;
#ਨਹੀਂਤਾਂ
        ਵਾਪਸ m3_Err_resource_ਹੱਦਬੰਦੀ_Not_Supported;
#ਜੇ_ਅੰਤ
    ਮੂਲ: ਵਾਪਸ m3_Err_unknown_Resource_ਹੱਦਬੰਦੀ;
    }
}

uint64_t m3_Get_Resource_ਹੱਦਬੰਦੀ (IM3Runtime i_runtime, M3_Resource_ਹੱਦਬੰਦੀ i_ਹੱਦਬੰਦੀ)
{
    ਜੇ (not i_runtime) {
        ਵਾਪਸ 0;
    }
    ਚੋਣ (i_ਹੱਦਬੰਦੀ) {
    ਹਾਲਤ c_m3_ਹੱਦਬੰਦੀ_ਸਮ੍ਰਿਤੀ_ਬਾਈਟਾਂ: ਵਾਪਸ i_runtime->ਸਮ੍ਰਿਤੀ_ਬਾਈਟਾਂ_ਹੱਦਬੰਦੀ;
    ਹਾਲਤ c_m3_ਹੱਦਬੰਦੀ_ਸਾਰਣੀ_Elements: ਵਾਪਸ i_runtime->ਸਾਰਣੀ_Elements_ਹੱਦਬੰਦੀ;
#ਜੇ d_m3_Has_ਢੇਰ_Switching
    ਹਾਲਤ c_m3_ਹੱਦਬੰਦੀ_Continuations: ਵਾਪਸ i_runtime->continuations_ਹੱਦਬੰਦੀ;
#ਜੇ_ਅੰਤ
#ਜੇ d_m3HasGasMetering
    ਹਾਲਤ c_m3_ਹੱਦਬੰਦੀ_Gas_Units: ਵਾਪਸ (u64)i_runtime->gas_ਹੱਦਬੰਦੀ;
#ਜੇ_ਅੰਤ
    ਮੂਲ: ਵਾਪਸ 0;
    }
}

uint64_t m3_GetResourceUsage (IM3Runtime i_runtime, M3_Resource_ਹੱਦਬੰਦੀ i_ਹੱਦਬੰਦੀ)
{
    ਜੇ (not i_runtime) {
        ਵਾਪਸ 0;
    }
    ਚੋਣ (i_ਹੱਦਬੰਦੀ) {
    ਹਾਲਤ c_m3_ਹੱਦਬੰਦੀ_ਸਮ੍ਰਿਤੀ_ਬਾਈਟਾਂ: ਵਾਪਸ i_runtime->ਸਮ੍ਰਿਤੀ_ਬਾਈਟਾਂ_ਵਰਤਿਆ;
    ਹਾਲਤ c_m3_ਹੱਦਬੰਦੀ_ਸਾਰਣੀ_Elements: ਵਾਪਸ i_runtime->ਸਾਰਣੀ_Elements_ਵਰਤਿਆ;
#ਜੇ d_m3_Has_ਢੇਰ_Switching
    ਹਾਲਤ c_m3_ਹੱਦਬੰਦੀ_Continuations: ਵਾਪਸ i_runtime->continuationsAllocated;
#ਜੇ_ਅੰਤ
#ਜੇ d_m3HasGasMetering
    ਹਾਲਤ c_m3_ਹੱਦਬੰਦੀ_Gas_Units: ਵਾਪਸ (u64)i_runtime->gas_ਹੱਦਬੰਦੀ - (u64)i_runtime->gas_ਬਚਿਆ;
#ਜੇ_ਅੰਤ
    ਮੂਲ: ਵਾਪਸ 0;
    }
}

ਖਾਲੀ* m3_Get_User_ਅੰਕੜੇ (IM3Runtime i_runtime)
{
    ਵਾਪਸ i_runtime ? i_runtime->userdata : ਨਿਰਾ;
}


ਖਾਲੀ* ForEachModule (IM3Runtime i_runtime, ModuleVisitor i_visitor, ਖਾਲੀ* i_info)
{
    ਖਾਲੀ* r = ਨਿਰਾ;

    IM3Module module = i_runtime->modules;

    ਜਦੋਂ (module) {
        IM3Module ਅਗਲਾ = module->ਅਗਲਾ;
        ਜੇ ((r = i_visitor(module, i_info))) {
            ਰੋਕੋ;
        }

        module = ਅਗਲਾ;
    }

    ਵਾਪਸ r;
}


ਖਾਲੀ* _FreeModule (IM3Module i_module, ਖਾਲੀ* i_info)
{
    m3_FreeModule(i_module);
    ਵਾਪਸ ਨਿਰਾ;
}


// A module's memory and table index spaces hold borrowed pointers: linking
// repoints an imported slot at the memory or table the exporting module owns.
// m3_FreeModule tells a borrowed slot from an owned one by reading that owner,
// which only holds while the object is still allocated - and the free walk
// below visits modules in load order, with no ordering between an owner and
// the modules borrowing from it. So clear the borrowed slots first, in a pass
// that frees nothing; the walk then skips them on the NULL check it already
// does, instead of reading a memory some earlier module has freed.
ਖਾਲੀ* _ReleaseBorrowedSlots (IM3Module i_module, ਖਾਲੀ* i_info)
{
    ਲਈ (u32 i = 0; i < i_module->ਅੰਕ_Memories; ++i) {
        IM3_ਸਮ੍ਰਿਤੀ ਸਮ੍ਰਿਤੀ = i_module->memories[i];

        ਜੇ (ਸਮ੍ਰਿਤੀ and ਸਮ੍ਰਿਤੀ->owner != i_module) {
            i_module->memories[i] = ਨਿਰਾ;
        }
    }

    ਲਈ (u32 i = 0; i < i_module->ਅੰਕ_Tables; ++i) {
        IM3_ਸਾਰਣੀ ਸਾਰਣੀ_2 = i_module->tables[i];

        ਜੇ (ਸਾਰਣੀ_2 and ਸਾਰਣੀ_2->owner != i_module) {
            i_module->tables[i] = ਨਿਰਾ;
        }
    }

    ਵਾਪਸ ਨਿਰਾ;
}


ਖਾਲੀ Runtime_Release (IM3Runtime i_runtime)
{
    ForEachModule(i_runtime, _ReleaseBorrowedSlots, ਨਿਰਾ);
    ForEachModule(i_runtime, _FreeModule, ਨਿਰਾ);                    d_m3Assert (i_runtime->ਅੰਕ_ਸਰਗਰਮ_ਕੋਡ_ਪੰਨੇ == 0);

    Environment_Release_ਕੋਡ_ਪੰਨੇ(i_runtime->environment, i_runtime->ਪੰਨੇ_Open);
    Environment_Release_ਕੋਡ_ਪੰਨੇ(i_runtime->environment, i_runtime->ਪੰਨੇ_ਭਰਿਆ);

    m3_Free(i_runtime->origin_ਢੇਰ);

#ਜੇ d_m3EnableValidation
    m3_Free(i_runtime->validator);
#ਜੇ_ਅੰਤ

#ਜੇ d_m3HasExceptionHandling
    // a runtime let go of while suspended still holds its exceptions
    FreeExceptions(i_runtime);
    m3_Free(i_runtime->uncaughtException);
#ਜੇ_ਅੰਤ
#ਜੇ d_m3HasExceptionHandling || d_m3_Has_ਢੇਰ_Switching
    ਜਦੋਂ (i_runtime->host_ਟੈਗ_ਸੂਚੀ) {
        M3_Host_ਟੈਗ* ਅਗਲਾ = i_runtime->host_ਟੈਗ_ਸੂਚੀ->ਅਗਲਾ;
        m3_Free(i_runtime->host_ਟੈਗ_ਸੂਚੀ);
        i_runtime->host_ਟੈਗ_ਸੂਚੀ = ਅਗਲਾ;
    }
#ਜੇ_ਅੰਤ
#ਜੇ d_m3_Has_ਢੇਰ_Switching
    Continuation_ReleaseAll(i_runtime);
#ਜੇ_ਅੰਤ
}


ਖਾਲੀ m3_FreeRuntime (IM3Runtime i_runtime)
{
    ਜੇ (i_runtime) {
        m3_PrintProfilerInfo();

        Runtime_Release(i_runtime);
        m3_Free(i_runtime);
    }
}


#ਜੇ d_m3_Has_ਢੇਰ_Switching

IM3Continuation Continuation_New (IM3Runtime i_runtime, IM3FuncType i_type, IM3Function i_function)
{
    IM3Continuation cont = m3_AllocStruct(M3Continuation);
    ਜੇ (!cont) {
        ਵਾਪਸ ਨਿਰਾ;
    }

    cont->type          = i_type;
    cont->ਇੰਦਰਾਜ_Function = i_function;
    cont->state         = cont_allocated;

    cont->ਅੰਕ_ਢੇਰ_Slots = d_m3_Continuation_ਢੇਰ_Slots;
    // frames are recorded only when this continuation actually suspends, and
    // only as deep as it got, so the array starts out unallocated
    cont->ਚੌਖਟੇ    = ਨਿਰਾ;
    cont->ਚੌਖਟੇ_Cap = 0;
    cont->ਅੰਕ_ਚੌਖਟੇ = 0;

    cont->sp = cont->val_ਢੇਰ;
    cont->pc = i_function ? i_function->compiled : ਨਿਰਾ;
    cont->r0 = 0;
#  ਜੇ d_m3HasFloat
    cont->fp0 = 0;
#  endif
    cont->bound_ਦਲੀਲਾਂ_ਗਿਣਤੀ    = 0;
    cont->ਅੰਕ_Suspend_Results = 0;
    cont->ਅੰਕ_Handlers       = 0;
    cont->handlersPC        = ਨਿਰਾ;
    cont->ਮਾਪਾ            = ਨਿਰਾ;

    cont->ਅਗਲਾ               = i_runtime->continuations;
    i_runtime->continuations = cont;

    ਵਾਪਸ cont;
}

IM3Continuation Continuation_ForkSuspended (IM3Runtime i_runtime, IM3Continuation i_cont)
{
    IM3Continuation newCont = Continuation_New(i_runtime, i_cont->type, i_cont->ਇੰਦਰਾਜ_Function);
    ਜੇ (M3_UNLIKELY(not newCont)) {
        ਵਾਪਸ ਨਿਰਾ;
    }

    newCont->val_ਢੇਰ      = i_cont->val_ਢੇਰ;
    newCont->ਅੰਕ_ਢੇਰ_Slots = i_cont->ਅੰਕ_ਢੇਰ_Slots;
    newCont->sp            = i_cont->sp;
    newCont->pc            = i_cont->pc;
    newCont->suspend_ਬਿੰਦੂ  = i_cont->suspend_ਬਿੰਦੂ;
    newCont->r0            = i_cont->r0;
#  ਜੇ d_m3HasFloat
    newCont->fp0 = i_cont->fp0;
#  endif
    newCont->ਚੌਖਟੇ    = i_cont->ਚੌਖਟੇ;
    newCont->ਚੌਖਟੇ_Cap = i_cont->ਚੌਖਟੇ_Cap;
    newCont->ਅੰਕ_ਚੌਖਟੇ = i_cont->ਅੰਕ_ਚੌਖਟੇ;

    newCont->bound_ਦਲੀਲਾਂ_ਗਿਣਤੀ = 0;
#  ਜੇ d_m3HasExceptionHandling
    newCont->resumeThrow = i_cont->resumeThrow;
    i_cont->resumeThrow  = ਨਿਰਾ;
#  endif
    newCont->ਅੰਕ_Suspend_Results = i_cont->ਅੰਕ_Suspend_Results;
    ਲਈ (u32 i = 0; i < i_cont->ਅੰਕ_Suspend_Results; ++i) {
        newCont->suspend_ਨਤੀਜਾ_Offsets[i] = i_cont->suspend_ਨਤੀਜਾ_Offsets[i];
        newCont->suspend_ਨਤੀਜਾ_Is64[i]    = i_cont->suspend_ਨਤੀਜਾ_Is64[i];
    }

    newCont->ਅੰਕ_Handlers = i_cont->ਅੰਕ_Handlers;
    newCont->handlersPC  = i_cont->handlersPC;
    newCont->ਮਾਪਾ      = i_cont->ਮਾਪਾ;
    newCont->state       = cont_suspended;

    i_cont->val_ਢੇਰ  = ਨਿਰਾ;
    i_cont->ਚੌਖਟੇ    = ਨਿਰਾ;
    i_cont->ਚੌਖਟੇ_Cap = 0;
    i_cont->ਅੰਕ_ਚੌਖਟੇ = 0;
    i_cont->state     = cont_consumed;

    ਵਾਪਸ newCont;
}

// Only cont.new and snapshot restoration acquire stacks. Binding and suspension
// transfer them between records without changing the active count.
M3_ਨਤੀਜਾ Continuation_Acquire_ਢੇਰ (IM3Runtime runtime, IM3Continuation cont)
{
    ਜੇ (not Continuation_Can_Acquire_ਢੇਰ(runtime)) {
        ਵਾਪਸ m3_Err_continuation_ਹੱਦਬੰਦੀ_Exceeded;
    }
    ਜੇ (runtime->val_ਢੇਰ_Pool_ਗਿਣਤੀ) {
        cont->val_ਢੇਰ = runtime->val_ਢੇਰ_Pool[--runtime->val_ਢੇਰ_Pool_ਗਿਣਤੀ];
        memset(cont->val_ਢੇਰ, 0, (cont->ਅੰਕ_ਢੇਰ_Slots + 4) * ਆਕਾਰ(m3slot_t));
    } ਨਹੀਂਤਾਂ {
        cont->val_ਢੇਰ = m3_AllocArray(m3slot_t, cont->ਅੰਕ_ਢੇਰ_Slots + 4);
    }
    ਜੇ (not cont->val_ਢੇਰ) {
        ਵਾਪਸ m3Err_mallocFailed;
    }
    cont->sp = cont->val_ਢੇਰ;
    runtime->continuationsAllocated++;
    ਵਾਪਸ m3Err_none;
}

ਖਾਲੀ Continuation_Recycle_ਢੇਰ (IM3Runtime runtime, IM3Continuation cont)
{
    ਜੇ (not cont->val_ਢੇਰ or cont == runtime->ਜੜ੍ਹ_Continuation) {
        ਵਾਪਸ;
    }
    ਜੇ (not runtime->val_ਢੇਰ_Pool) {
        runtime->val_ਢੇਰ_Pool = m3_AllocArray(m3slot_t*, 16);
        ਜੇ (runtime->val_ਢੇਰ_Pool) {
            runtime->val_ਢੇਰ_Pool_Cap = 16;
        }
    }
    ਜੇ (runtime->val_ਢੇਰ_Pool_ਗਿਣਤੀ < runtime->val_ਢੇਰ_Pool_Cap and
        (not runtime->continuations_ਹੱਦਬੰਦੀ or runtime->val_ਢੇਰ_Pool_ਗਿਣਤੀ <
                                              runtime->continuations_ਹੱਦਬੰਦੀ - runtime->continuationsAllocated + 1)) {
        runtime->val_ਢੇਰ_Pool[runtime->val_ਢੇਰ_Pool_ਗਿਣਤੀ++] = cont->val_ਢੇਰ;
        cont->val_ਢੇਰ                                      = ਨਿਰਾ;
    } ਨਹੀਂਤਾਂ {
        m3_Free(cont->val_ਢੇਰ);
    }
    cont->sp = ਨਿਰਾ;
    runtime->continuationsAllocated--;
}

ਖਾਲੀ Continuation_ReleaseAll (IM3Runtime io_runtime)
{
    // Records live until teardown: a consumed reference can still be in a table
    // or on a Wasm stack. Its value stack has already been recycled.
    IM3Continuation cont = io_runtime->continuations;

    ਜਦੋਂ (cont) {
        IM3Continuation ਅਗਲਾ = cont->ਅਗਲਾ;

        m3_Free(cont->val_ਢੇਰ);
        m3_Free(cont->ਚੌਖਟੇ);
        m3_Free(cont);

        cont = ਅਗਲਾ;
    }

    ਲਈ (u32 i = 0; i < io_runtime->val_ਢੇਰ_Pool_ਗਿਣਤੀ; ++i) {
        m3_Free(io_runtime->val_ਢੇਰ_Pool[i]);
    }
    m3_Free(io_runtime->val_ਢੇਰ_Pool);
    io_runtime->val_ਢੇਰ_Pool_ਗਿਣਤੀ = io_runtime->val_ਢੇਰ_Pool_Cap = 0;
    io_runtime->continuationsAllocated                          = 0;
    io_runtime->continuations                                   = ਨਿਰਾ;
    io_runtime->ਸਰਗਰਮ_Continuation                              = ਨਿਰਾ;
    io_runtime->ਜੜ੍ਹ_Continuation                                = ਨਿਰਾ;
}


m3ret_t Continuation_ਰਿਕਾਰਡ_ਚੌਖਟਾ (IM3Runtime i_runtime, ਅਚਲ M3_ਚੌਖਟਾ* i_ਚੌਖਟਾ)
{
    IM3Continuation cont = i_runtime->ਸਰਗਰਮ_Continuation;

    // the marker only travels between a suspend and the resume that runs it,
    // and that whole stretch has an active continuation
    d_m3Assert(cont);

    ਜੇ (cont->ਅੰਕ_ਚੌਖਟੇ == cont->ਚੌਖਟੇ_Cap) {
        u32 newCap = cont->ਚੌਖਟੇ_Cap ? cont->ਚੌਖਟੇ_Cap * 2 : 8;

        ਜੇ (newCap > d_m3_Continuation_Max_ਚੌਖਟੇ) {
            newCap = d_m3_Continuation_Max_ਚੌਖਟੇ;
        }

        ਜੇ (newCap == cont->ਚੌਖਟੇ_Cap) {
            ਵਾਪਸ m3_Err_trap_ਢੇਰ_ਉਛਾਲ;
        }

        M3_ਚੌਖਟਾ* ਚੌਖਟੇ = m3_ReallocArray(M3_ਚੌਖਟਾ, cont->ਚੌਖਟੇ, newCap, cont->ਚੌਖਟੇ_Cap);

        ਜੇ (not ਚੌਖਟੇ) {
            ਵਾਪਸ m3Err_mallocFailed;
        }

        cont->ਚੌਖਟੇ    = ਚੌਖਟੇ;
        cont->ਚੌਖਟੇ_Cap = newCap;
    }

    cont->ਚੌਖਟੇ[cont->ਅੰਕ_ਚੌਖਟੇ++] = *i_ਚੌਖਟਾ;

    ਵਾਪਸ m3Err_continuationSuspended;
}

#ਜੇ_ਅੰਤ // d_m3HasStackSwitching

M3_ਨਤੀਜਾ Evaluate_ਵਿਅੰਜਕ (IM3Module i_module, ਖਾਲੀ* o_expressed, m3type_t i_type, ਬਾਈਟਾਂ_t* io_ਬਾਈਟਾਂ, cbytes_t i_ਸਿਰਾ)
{
    M3_ਨਤੀਜਾ ਨਤੀਜਾ = m3Err_none;

    // OPTZ: use a simplified interpreter for expressions

    // create a temporary runtime context
#ਜੇ defined(d_m3PreferStaticAlloc)
    ਸਥਿਰ M3Runtime runtime;
#ਨਹੀਂਤਾਂ
    M3Runtime runtime;
#ਜੇ_ਅੰਤ
    M3_INIT(runtime);

    runtime.environment   = i_module->runtime->environment;
    runtime.ਅੰਕ_ਢੇਰ_Slots = i_module->runtime->ਅੰਕ_ਢੇਰ_Slots;
    runtime.ਢੇਰ         = i_module->runtime->ਢੇਰ;

    m3stack_t ਢੇਰ = (m3stack_t)runtime.ਢੇਰ;

    IM3Runtime savedRuntime = i_module->runtime;
    i_module->runtime       = &runtime;

    IM3Compilation o   = &runtime.compilation;
    o->runtime         = &runtime;
    o->module          = i_module;
    o->wasm            = *io_ਬਾਈਟਾਂ;
    o->wasm_ਸਿਰਾ         = i_ਸਿਰਾ;
    o->ਆਖਰੀ_Opcode_ਸ਼ੁਰੂ = o->wasm;

    // Borrowed for the length of this expression only. Runtime_Release below hands it
    // to Environment_ReleaseCodePages, which resets lineIndex, so the page comes back
    // empty and the next expression reuses it rather than burning fresh lines.
    o->ਪੰਨਾ = Acquire_ਕੋਡ_ਪੰਨਾ(&runtime);

    ਜੇ (o->ਪੰਨਾ) {
        IM3FuncType ftype = ਨਿਰਾ;

#ਜੇ d_m3HasTypedRefs
        // A reference type has to be checked as the type it is: the shared
        // return types are one per storage class, so borrowing one of those
        // would let any reference at all satisfy a particular one.
        ਜੇ (IsSpelledRefType(i_type)) {
            ਨਤੀਜਾ = AllocFuncType(&ftype, 1);

            ਜੇ (not ਨਤੀਜਾ) {
                ftype->ਅੰਕ_Rets  = 1;
                ftype->types[0] = i_type;

                ਨਤੀਜਾ = Environment_ਜੋੜੋ_Func_Type(runtime.environment, &ftype);
            }
        } ਨਹੀਂਤਾਂ
#ਜੇ_ਅੰਤ
        {
            ftype = runtime.environment->retFuncTypes[ਅਧਾਰ_Type_ਦਾ(i_type)];
        }

        pc_t m3code = Get_ਪੰਨਾ_PC(o->ਪੰਨਾ);

        ਜੇ (not ਨਤੀਜਾ) {
            ਨਤੀਜਾ = Compile_ਵਿਅੰਜਕ(o, ftype);
        }

        ਜੇ (not ਨਤੀਜਾ && o->max_ਢੇਰ_Slots >= runtime.ਅੰਕ_ਢੇਰ_Slots) {
            ਨਤੀਜਾ = m3_Err_trap_ਢੇਰ_ਉਛਾਲ;
        }

        ਜੇ (not ਨਤੀਜਾ) {
#ਜੇ (d_m3EnableOpProfiling || d_m3EnableOpTracing)
            m3ret_t r = ਲੜੀਵਾਰ_ਕੋਡ(m3code, ਢੇਰ, ਨਿਰਾ, d_m3_Op_Default_ਦਲੀਲਾਂ, d_m3_ਅਧਾਰ_Cstr);
#ਨਹੀਂਤਾਂ
            m3ret_t r = ਲੜੀਵਾਰ_ਕੋਡ(m3code, ਢੇਰ, ਨਿਰਾ, d_m3_Op_Default_ਦਲੀਲਾਂ);
#ਜੇ_ਅੰਤ

            ਜੇ (r == 0) {                                                                   m3log (runtime, "expression result: %s", SPrint_ਮੁੱਲ (ਢੇਰ, i_type));
                ਜੇ (ਮਾਪ_ਦਾ_Type(ਅਧਾਰ_Type_ਦਾ(i_type)) == ਆਕਾਰ(u32)) {
                    *(u32*)o_expressed = *((u32*)ਢੇਰ);
                } ਨਹੀਂਤਾਂ {
                    *(u64*)o_expressed = *((u64*)ਢੇਰ);
                }
            }
        }

        Release_ਕੋਡ_ਪੰਨਾ(&runtime, o->ਪੰਨਾ);
    } ਨਹੀਂਤਾਂ {
        ਨਤੀਜਾ = m3_Err_malloc_Failed_ਕੋਡ_ਪੰਨਾ;
    }

    runtime.origin_ਢੇਰ = ਨਿਰਾ;        // prevent free(stack) in ReleaseRuntime
    Runtime_Release(&runtime);
    i_module->runtime = savedRuntime;

    *io_ਬਾਈਟਾਂ = o->wasm;

    ਵਾਪਸ ਨਤੀਜਾ;
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
ਸਥਿਰ
bool LimitsSatisfy (u64 i_exported_ਮਾਪ, bool i_exportedHasMax, u64 i_exportedMax,
                    u64 i_importMin, bool i_importHasMax, u64 i_importMax)
{
    ਜੇ (i_exported_ਮਾਪ < i_importMin) {
        ਵਾਪਸ false;
    }

    ਜੇ (i_importHasMax and (not i_exportedHasMax or i_exportedMax > i_importMax)) {
        ਵਾਪਸ false;
    }

    ਵਾਪਸ true;
}


// The export of this kind under this name, or NULL. The index it carries is
// resolved by the caller, against the module's index space as it stands then -
// after linking, a re-exported import's slot holds the entity it was linked to.
ਸਥਿਰ
ਅਚਲ M3Export* Module_ਲੱਭੋ_Export (IM3Module i_module, u8 i_ਕਿਸਮ, cstr_t i_name)
{
    ਲਈ (u32 i = 0; i < i_module->ਅੰਕ_Exports; ++i) {
        ਅਚਲ M3Export* e = &i_module->exports[i];

        ਜੇ (e->ਕਿਸਮ == i_ਕਿਸਮ and strcmp(e->name, i_name) == 0) {
            ਵਾਪਸ e;
        }
    }

    ਵਾਪਸ ਨਿਰਾ;
}


ਸਥਿਰ
IM3Function Module_ਲੱਭੋ_Exported_Function (IM3Module i_module, cstr_t i_name)
{
    ਅਚਲ M3Export* e = Module_ਲੱਭੋ_Export(i_module, d_external_ਕਿਸਮ_function, i_name);

    // A module that re-exports an import names the placeholder here. Resolve to
    // the function that actually runs, or a host-side call would run it against
    // the importing module's memory.
    ਵਾਪਸ e ? Function_Implementation(&i_module->functions[e->index]) : ਨਿਰਾ;
}


ਸਥਿਰ
IM3_ਸਮ੍ਰਿਤੀ Module_ਲੱਭੋ_Exported_ਸਮ੍ਰਿਤੀ (IM3Module i_module, cstr_t i_name)
{
    ਅਚਲ M3Export* e = Module_ਲੱਭੋ_Export(i_module, d_external_ਕਿਸਮ_ਸਮ੍ਰਿਤੀ, i_name);

    ਵਾਪਸ e ? i_module->memories[e->index] : ਨਿਰਾ;
}


ਸਥਿਰ
IM3_ਸਾਰਣੀ Module_ਲੱਭੋ_Exported_ਸਾਰਣੀ (IM3Module i_module, cstr_t i_name)
{
    ਅਚਲ M3Export* e = Module_ਲੱਭੋ_Export(i_module, d_external_ਕਿਸਮ_ਸਾਰਣੀ, i_name);

    ਵਾਪਸ e ? i_module->tables[e->index] : ਨਿਰਾ;
}


ਸਥਿਰ
IM3Global Module_ਲੱਭੋ_Exported_Global (IM3Module i_module, cstr_t i_name)
{
    ਅਚਲ M3Export* e = Module_ਲੱਭੋ_Export(i_module, d_external_ਕਿਸਮ_global, i_name);

    ਵਾਪਸ e ? &i_module->globals[e->index] : ਨਿਰਾ;
}


#ਜੇ d_m3HasExceptionHandling || d_m3_Has_ਢੇਰ_Switching
ਸਥਿਰ
IM3_ਟੈਗ Module_ਲੱਭੋ_Exported_ਟੈਗ (IM3Module i_module, cstr_t i_name)
{
    ਅਚਲ M3Export* e = Module_ਲੱਭੋ_Export(i_module, d_external_ਕਿਸਮ_ਟੈਗ, i_name);

    ਜੇ (not e) {
        ਵਾਪਸ ਨਿਰਾ;
    }

    IM3_ਟੈਗ ਟੈਗ = &i_module->ਟੈਗ_ਸੂਚੀ[e->index];

    ਵਾਪਸ ਟੈਗ->resolved ? ਟੈਗ->resolved : ਟੈਗ;
}
#ਜੇ_ਅੰਤ


// Whether the module exports anything at all under this name. Export names are
// unique within a module, so a name one of the lookups above missed but this
// one finds is exported as something else - a kind the import cannot be
// satisfied by, rather than a name the module never exported.
ਸਥਿਰ
bool Module_HasExport (IM3Module i_module, cstr_t i_name)
{
    ਲਈ (u32 i = 0; i < i_module->ਅੰਕ_Exports; ++i) {
        ਜੇ (strcmp(i_module->exports[i].name, i_name) == 0) {
            ਵਾਪਸ true;
        }
    }

    ਵਾਪਸ false;
}


// Points each of the module's imports at whatever already-loaded module exports
// it. Runs before anything is allocated or initialized: a memory import has to
// be resolved before InitMemory would give it pages of its own, and a global
// import before InitGlobals runs an initializer that reads it.
ਸਥਿਰ
M3_ਨਤੀਜਾ LinkImports (IM3Runtime io_runtime, IM3Module io_module)
{
    M3_ਨਤੀਜਾ ਨਤੀਜਾ = m3Err_none;

    ਲਈ (u32 i = 0; i < io_module->ਅੰਕ_Functions; ++i) {
        IM3Function f = &io_module->functions[i];

        ਜੇ (f->wasm or not (f->import.moduleUtf8 and f->import.ਖੇਤਰ_Utf8)) {
            ਜਾਰੀ;
        }

        IM3Module from = m3_ਲੱਭੋ_Module(io_runtime, f->import.moduleUtf8);
        ਜੇ (not from) {
            ਜਾਰੀ;
        }

        IM3Function exported = Module_ਲੱਭੋ_Exported_Function(from, f->import.ਖੇਤਰ_Utf8);
        ਜੇ (not exported) {
            // the import names a module that is loaded, so its exports settle the
            // question: a name it exports as something else is a type mismatch, and
            // one it does not export at all is an unknown import
            _throwif(m3Err_incompatibleImportType, Module_HasExport(from, f->import.ਖੇਤਰ_Utf8));
            _throw(m3Err_unknownImport);
        }

        // func types are canonical within an environment, so this is the
        // structural equivalence the spec asks for
        _throwif(m3Err_incompatibleImportType, exported->funcType != f->funcType);

        f->resolved = Function_Implementation(exported);
    }

    ਲਈ (u32 i = 0; i < io_module->ਅੰਕ_Memories; ++i) {
        IM3_ਸਮ੍ਰਿਤੀ ਸਮ੍ਰਿਤੀ = io_module->memories[i];

        ਜੇ (not ਸਮ੍ਰਿਤੀ->imported or ਸਮ੍ਰਿਤੀ->owner != io_module) {
            ਜਾਰੀ;
        }

        IM3Module from = m3_ਲੱਭੋ_Module(io_runtime, ਸਮ੍ਰਿਤੀ->import.moduleUtf8);
        ਜੇ (not from) {
            ਜਾਰੀ;
        }

        IM3_ਸਮ੍ਰਿਤੀ exported = Module_ਲੱਭੋ_Exported_ਸਮ੍ਰਿਤੀ(from, ਸਮ੍ਰਿਤੀ->import.ਖੇਤਰ_Utf8);
        ਜੇ (not exported) {
            _throwif(m3Err_incompatibleImportType, Module_HasExport(from, ਸਮ੍ਰਿਤੀ->import.ਖੇਤਰ_Utf8));
            _throw(m3Err_unknownImport);
        }

        // the address type is part of the memory type, so an i64 memory does
        // not satisfy an i32 import, or the other way round - and so is the page
        // size, which custom page sizes made a declared property of a memory
        _throwif(m3Err_incompatibleImportType, exported->isMemory64 != ਸਮ੍ਰਿਤੀ->isMemory64);
        _throwif(m3Err_incompatibleImportType, ਸਮ੍ਰਿਤੀ_ਪੰਨਾ_ਮਾਪ(exported) != ਸਮ੍ਰਿਤੀ_ਪੰਨਾ_ਮਾਪ(ਸਮ੍ਰਿਤੀ));

        // a shared memory is not a subtype of an unshared one, or the other way round
        _throwif(m3Err_incompatibleImportType, exported->isShared != ਸਮ੍ਰਿਤੀ->isShared);

        _throwif(m3Err_incompatibleImportType,
                 not LimitsSatisfy(exported->ਅੰਕ_ਪੰਨੇ, exported->hasMax, exported->max_ਪੰਨੇ,
                                   ਸਮ੍ਰਿਤੀ->init_ਪੰਨੇ, ਸਮ੍ਰਿਤੀ->hasMax, ਸਮ੍ਰਿਤੀ->max_ਪੰਨੇ));

        // hand the slot over to the exporter's memory, and drop the placeholder
        Free_ਸਮ੍ਰਿਤੀ_ਖੰਡ(ਸਮ੍ਰਿਤੀ);
        FreeImportInfo(&ਸਮ੍ਰਿਤੀ->import);
        m3_Free(ਸਮ੍ਰਿਤੀ);

        io_module->memories[i] = exported;
    }

    ਲਈ (u32 i = 0; i < io_module->ਅੰਕ_Tables; ++i) {
        IM3_ਸਾਰਣੀ ਸਾਰਣੀ_2 = io_module->tables[i];

        ਜੇ (not ਸਾਰਣੀ_2->imported or ਸਾਰਣੀ_2->owner != io_module) {
            ਜਾਰੀ;
        }

        IM3Module from = m3_ਲੱਭੋ_Module(io_runtime, ਸਾਰਣੀ_2->import.moduleUtf8);
        ਜੇ (not from) {
            ਜਾਰੀ;
        }

        IM3_ਸਾਰਣੀ exported = Module_ਲੱਭੋ_Exported_ਸਾਰਣੀ(from, ਸਾਰਣੀ_2->import.ਖੇਤਰ_Utf8);
        ਜੇ (not exported) {
            _throwif(m3Err_incompatibleImportType, Module_HasExport(from, ਸਾਰਣੀ_2->import.ਖੇਤਰ_Utf8));
            _throw(m3Err_unknownImport);
        }

        _throwif(m3Err_incompatibleImportType, exported->type != ਸਾਰਣੀ_2->type);

        // the index type is part of the table type, the same way it is for a memory
        _throwif(m3Err_incompatibleImportType, exported->isTable64 != ਸਾਰਣੀ_2->isTable64);

        _throwif(m3Err_incompatibleImportType,
                 not LimitsSatisfy(exported->ਮਾਪ, exported->hasMax, exported->max_ਮਾਪ,
                                   ਸਾਰਣੀ_2->init_ਮਾਪ, ਸਾਰਣੀ_2->hasMax, ਸਾਰਣੀ_2->max_ਮਾਪ));

        // hand the slot over to the exporter's table, and drop the placeholder
        m3_Free(ਸਾਰਣੀ_2->elements);
        FreeImportInfo(&ਸਾਰਣੀ_2->import);
        m3_Free(ਸਾਰਣੀ_2);

        io_module->tables[i] = exported;
    }

    ਲਈ (u32 i = 0; i < io_module->ਅੰਕ_Globals; ++i) {
        IM3Global g = &io_module->globals[i];

        ਜੇ (not g->imported or not (g->import.moduleUtf8 and g->import.ਖੇਤਰ_Utf8)) {
            ਜਾਰੀ;
        }

        IM3Module from = m3_ਲੱਭੋ_Module(io_runtime, g->import.moduleUtf8);
        ਜੇ (not from) {
            ਜਾਰੀ;
        }

        IM3Global exported = Module_ਲੱਭੋ_Exported_Global(from, g->import.ਖੇਤਰ_Utf8);
        ਜੇ (not exported) {
            _throwif(m3Err_incompatibleImportType, Module_HasExport(from, g->import.ਖੇਤਰ_Utf8));
            _throw(m3Err_unknownImport);
        }

        _throwif(m3Err_incompatibleImportType, exported->type != g->type);
        _throwif(m3Err_incompatibleImportType, exported->isMutable != g->isMutable);

        g->resolved = exported->resolved ? exported->resolved : exported;
    }

#ਜੇ d_m3HasExceptionHandling || d_m3_Has_ਢੇਰ_Switching
    ਲਈ (u32 i = 0; i < io_module->ਅੰਕ_ਟੈਗ_ਸੂਚੀ; ++i) {
        IM3_ਟੈਗ ਟੈਗ = &io_module->ਟੈਗ_ਸੂਚੀ[i];

        ਜੇ (not ਟੈਗ->imported or not (ਟੈਗ->import.moduleUtf8 and ਟੈਗ->import.ਖੇਤਰ_Utf8)) {
            ਜਾਰੀ;
        }

        IM3Module from = m3_ਲੱਭੋ_Module(io_runtime, ਟੈਗ->import.moduleUtf8);
        ਜੇ (not from) {
            ਜਾਰੀ;
        }

        IM3_ਟੈਗ exported = Module_ਲੱਭੋ_Exported_ਟੈਗ(from, ਟੈਗ->import.ਖੇਤਰ_Utf8);
        ਜੇ (not exported) {
            _throwif(m3Err_incompatibleImportType, Module_HasExport(from, ਟੈਗ->import.ਖੇਤਰ_Utf8));
            _throw(m3Err_unknownImport);
        }

        _throwif(m3Err_incompatibleImportType, exported->type != ਟੈਗ->type);

        ਟੈਗ->resolved = exported->resolved ? exported->resolved : exported;
    }
#ਜੇ_ਅੰਤ

    _catch: ਵਾਪਸ ਨਤੀਜਾ;
}


// Backs each of the module's memories with pages. LinkImports has already
// pointed any import it could satisfy at the exporting module's memory, and
// those are skipped here. An import nothing satisfied is still backed locally
// from its own declared limits, so that the module remains loadable.
M3_ਨਤੀਜਾ Init_ਸਮ੍ਰਿਤੀ (IM3Runtime io_runtime, IM3Module i_module)
{
    M3_ਨਤੀਜਾ ਨਤੀਜਾ = m3Err_none;

    // Fixed from here on: the index space stops changing after parse, and
    // linking has already repointed any slot it was going to.
    i_module->memory0 = i_module->ਅੰਕ_Memories ? i_module->memories[0]
                                              : &i_module->ਖਾਲੀ_ਥਾਵਾਂ_ਸਮ੍ਰਿਤੀ;

    ਜੇ (i_module->ਅੰਕ_Memories == 0) {
        // nothing addressable, but _mem still has to point somewhere
        i_module->ਖਾਲੀ_ਥਾਵਾਂ_ਸਮ੍ਰਿਤੀ.owner    = i_module;
        i_module->ਖਾਲੀ_ਥਾਵਾਂ_ਸਮ੍ਰਿਤੀ.ਪੰਨਾ_ਮਾਪ = d_m3_Default_Mem_ਪੰਨਾ_ਮਾਪ;

_       (Resize_ਸਮ੍ਰਿਤੀ(io_runtime, &i_module->ਖਾਲੀ_ਥਾਵਾਂ_ਸਮ੍ਰਿਤੀ, 0));
    }

    ਲਈ (u32 i = 0; i < i_module->ਅੰਕ_Memories; ++i) {
        IM3_ਸਮ੍ਰਿਤੀ ਸਮ੍ਰਿਤੀ = i_module->memories[i];

        // a slot that already points at another module's memory is that
        // module's to allocate
        ਜੇ (ਸਮ੍ਰਿਤੀ->owner != i_module or ਸਮ੍ਰਿਤੀ->mallocated) {
            ਜਾਰੀ;
        }

        u32 ਪੰਨਾ_ਮਾਪ = ਸਮ੍ਰਿਤੀ_ਪੰਨਾ_ਮਾਪ(ਸਮ੍ਰਿਤੀ);

        ਸਮ੍ਰਿਤੀ->ਪੰਨਾ_ਮਾਪ = ਪੰਨਾ_ਮਾਪ;

        // Without a declared maximum a memory may grow to the spec limit of
        // 2^|addrtype|/pagesize pages, which is the usual 65536 at the default
        // page size, 2^48 for a 64-bit memory, and a whole address space of
        // them when a page is a single byte. A declared maximum of zero is a
        // real limit, not the absence of one.
        //
        // 2^64/pagesize overflows a u64 only when a page is a single byte, and
        // a power-of-two page size divides the address space exactly, so the
        // 64-bit division below is 2^64/pagesize written so that it fits.
        ਜੇ (not ਸਮ੍ਰਿਤੀ->hasMax) {
            ਸਮ੍ਰਿਤੀ->max_ਪੰਨੇ = ਸਮ੍ਰਿਤੀ->isMemory64
                                 ? (ਪੰਨਾ_ਮਾਪ > 1 ? (UINT64_MAX / ਪੰਨਾ_ਮਾਪ) + 1 : UINT64_MAX)
                                 : (0x100000000ull / ਪੰਨਾ_ਮਾਪ);
        }

_       (Resize_ਸਮ੍ਰਿਤੀ(io_runtime, ਸਮ੍ਰਿਤੀ, ਸਮ੍ਰਿਤੀ->init_ਪੰਨੇ));
    }

    _catch: ਵਾਪਸ ਨਤੀਜਾ;
}


M3_ਨਤੀਜਾ Resize_ਸਮ੍ਰਿਤੀ (IM3Runtime io_runtime, IM3_ਸਮ੍ਰਿਤੀ ਸਮ੍ਰਿਤੀ, u64 i_ਅੰਕ_ਪੰਨੇ)
{
    M3_ਨਤੀਜਾ ਨਤੀਜਾ = m3Err_none;

    u64 ਅੰਕ_ਪੰਨੇ_ਮੰਜ਼ਿਲ_Alloc = i_ਅੰਕ_ਪੰਨੇ;

    ਜੇ (ਅੰਕ_ਪੰਨੇ_ਮੰਜ਼ਿਲ_Alloc <= ਸਮ੍ਰਿਤੀ->max_ਪੰਨੇ) {
        // A 64-bit memory may ask for up to 2^48 pages, which overflows a u64
        // of bytes. Nothing that large can be backed, so refuse it up front
        // rather than multiplying into a wrapped size.
        _throwif("linear memory limitation exceeded",
                 ਅੰਕ_ਪੰਨੇ_ਮੰਜ਼ਿਲ_Alloc > d_m3_ਪਤਾ_ਹੱਦਬੰਦੀ / ਸਮ੍ਰਿਤੀ->ਪੰਨਾ_ਮਾਪ);

        u64 ਅੰਕ_ਪੰਨਾ_ਬਾਈਟਾਂ = ਅੰਕ_ਪੰਨੇ_ਮੰਜ਼ਿਲ_Alloc * ਸਮ੍ਰਿਤੀ->ਪੰਨਾ_ਮਾਪ;

#ਜੇ d_m3_Max_ਰੇਖਿਕ_ਸਮ੍ਰਿਤੀ_ਪੰਨੇ > 0
        // the limit is a memory size, counted in default-sized pages; comparing
        // it against a raw page count would make it 65536 times stricter for a
        // module whose pages are one byte
        _throwif("linear memory limitation exceeded",
                 ਅੰਕ_ਪੰਨਾ_ਬਾਈਟਾਂ > (u64)d_m3_Max_ਰੇਖਿਕ_ਸਮ੍ਰਿਤੀ_ਪੰਨੇ * d_m3_Default_Mem_ਪੰਨਾ_ਮਾਪ);
#ਜੇ_ਅੰਤ

        // Limit the amount of memory that gets actually allocated
        ਜੇ (io_runtime->ਸਮ੍ਰਿਤੀ_ਹੱਦਬੰਦੀ) {
            ਅੰਕ_ਪੰਨਾ_ਬਾਈਟਾਂ = M3_MIN(ਅੰਕ_ਪੰਨਾ_ਬਾਈਟਾਂ, (u64)io_runtime->ਸਮ੍ਰਿਤੀ_ਹੱਦਬੰਦੀ);
        }

        u64 old_ਲੰਮਾਈ = ਸਮ੍ਰਿਤੀ->mallocated ? ਸਮ੍ਰਿਤੀ->mallocated->ਲੰਮਾਈ : 0;
        u64 other_ਵਰਤਿਆ = io_runtime->ਸਮ੍ਰਿਤੀ_ਬਾਈਟਾਂ_ਵਰਤਿਆ - old_ਲੰਮਾਈ;
        _throwif(m3_Err_ਸਮ੍ਰਿਤੀ_ਹੱਦਬੰਦੀ_Exceeded, io_runtime->ਸਮ੍ਰਿਤੀ_ਬਾਈਟਾਂ_ਹੱਦਬੰਦੀ and
                                              (ਅੰਕ_ਪੰਨਾ_ਬਾਈਟਾਂ > io_runtime->ਸਮ੍ਰਿਤੀ_ਬਾਈਟਾਂ_ਹੱਦਬੰਦੀ or
                                               other_ਵਰਤਿਆ > io_runtime->ਸਮ੍ਰਿਤੀ_ਬਾਈਟਾਂ_ਹੱਦਬੰਦੀ - ਅੰਕ_ਪੰਨਾ_ਬਾਈਟਾਂ));

        _throwif("linear memory limitation exceeded", ਅੰਕ_ਪੰਨਾ_ਬਾਈਟਾਂ > (u64)SIZE_MAX - ਆਕਾਰ(M3_ਸਮ੍ਰਿਤੀ_ਸਿਰਲੇਖ));

#ਜੇ d_m3_Guarded_ਸਮ੍ਰਿਤੀ

        // A slot holds every address a Wasm access can name, and nothing beyond
        // that: a memory larger than the data part of one would put addresses past
        // the guard, where a fault is nobody's to recognize
        _throwif("linear memory limitation exceeded", ਅੰਕ_ਪੰਨਾ_ਬਾਈਟਾਂ > d_m3_Guarded_ਅੰਕੜੇ_ਬਾਈਟਾਂ);

        // What makes an access past the end fault is the page after the memory not
        // being committed, so the end has to fall on one of the system's page
        // boundaries. A Wasm page divides evenly into them and always will; a memory
        // that declared a page size of its own may not, and then the bytes between
        // the end of the memory and the end of its last system page would answer
        // instead of trapping. Refusing to back such a memory is the honest way out:
        // the custom page sizes proposal exists for targets far too small to reserve
        // 8GiB per memory in the first place. See d_m3GuardedMemory.
        _throwif("guarded memory needs a page size the system's divides into",
                 (ਸਮ੍ਰਿਤੀ->ਪੰਨਾ_ਮਾਪ % m3_Host_ਪੰਨਾ_ਮਾਪ()) != 0);

        ਜੇ (ਸਮ੍ਰਿਤੀ->guard_ਥਾਂ == ਨਿਰਾ) {
            ਸਮ੍ਰਿਤੀ->guard_ਥਾਂ = Guard_Take_ਥਾਂ();
            _throwifnull(ਸਮ੍ਰਿਤੀ->guard_ਥਾਂ);
        }

        // Growing only ever commits more of the same slot, so the memory never
        // moves - which the interpreter does not rely on, but nothing minds
        _throwif(m3Err_mallocFailed, not Guard_Commit_ਥਾਂ(ਸਮ੍ਰਿਤੀ->guard_ਥਾਂ, (size_t)ਅੰਕ_ਪੰਨਾ_ਬਾਈਟਾਂ));

        ਸਮ੍ਰਿਤੀ->mallocated = Guard_ਥਾਂ_ਸਿਰਲੇਖ(ਸਮ੍ਰਿਤੀ->guard_ਥਾਂ);

#ਨਹੀਂਤਾਂ

        size_t ਅੰਕ_ਬਾਈਟਾਂ = (size_t)ਅੰਕ_ਪੰਨਾ_ਬਾਈਟਾਂ + ਆਕਾਰ(M3_ਸਮ੍ਰਿਤੀ_ਸਿਰਲੇਖ);

        // What is really there, which is not numPages * pageSize: the limit above
        // caps the bytes without capping the page count, so a memory can be short
        // of a page - by design, since a whole 64 KiB page is often more than an
        // MCU has. Measuring the old block by the page count would overstate it,
        // and then realloc never sees the size it already has and copies the
        // whole limit again on every grow past it.
        size_t ਅੰਕ_ਪਿਛਲਾ_ਬਾਈਟਾਂ = ਸਮ੍ਰਿਤੀ->mallocated ? ਸਮ੍ਰਿਤੀ->mallocated->ਲੰਮਾਈ + ਆਕਾਰ(M3_ਸਮ੍ਰਿਤੀ_ਸਿਰਲੇਖ) : 0;

        ਖਾਲੀ* newMem = m3_Realloc("Wasm Linear Memory", ਸਮ੍ਰਿਤੀ->mallocated, ਅੰਕ_ਬਾਈਟਾਂ, ਅੰਕ_ਪਿਛਲਾ_ਬਾਈਟਾਂ);
        _throwifnull(newMem);

        ਸਮ੍ਰਿਤੀ->mallocated = (M3_ਸਮ੍ਰਿਤੀ_ਸਿਰਲੇਖ*)newMem;

#ਜੇ_ਅੰਤ

#ਜੇ d_m3LogRuntime
        M3_ਸਮ੍ਰਿਤੀ_ਸਿਰਲੇਖ* oldMallocated = ਸਮ੍ਰਿਤੀ->mallocated;
#ਜੇ_ਅੰਤ

        io_runtime->ਸਮ੍ਰਿਤੀ_ਬਾਈਟਾਂ_ਵਰਤਿਆ = other_ਵਰਤਿਆ + ਅੰਕ_ਪੰਨਾ_ਬਾਈਟਾਂ;
        ਸਮ੍ਰਿਤੀ->ਅੰਕ_ਪੰਨੇ            = ਅੰਕ_ਪੰਨੇ_ਮੰਜ਼ਿਲ_Alloc;

        ਸਮ੍ਰਿਤੀ->mallocated->ਲੰਮਾਈ  = (size_t)ਅੰਕ_ਪੰਨਾ_ਬਾਈਟਾਂ;
        ਸਮ੍ਰਿਤੀ->mallocated->runtime = io_runtime;
        ਸਮ੍ਰਿਤੀ->mallocated->ਸਮ੍ਰਿਤੀ  = ਸਮ੍ਰਿਤੀ;

        ਸਮ੍ਰਿਤੀ->mallocated->max_ਢੇਰ = (m3slot_t*)io_runtime->origin_ਢੇਰ + io_runtime->ਅੰਕ_ਢੇਰ_Slots;

        m3log(runtime, "resized old: %p; mem: %p; length: %zu; pages: %llu", oldMallocated, ਸਮ੍ਰਿਤੀ->mallocated, ਸਮ੍ਰਿਤੀ->mallocated->ਲੰਮਾਈ, (ਬਿਨਾਨਿਸ਼ਾਨੀ ਲੰਮਾ ਲੰਮਾ)ਸਮ੍ਰਿਤੀ->ਅੰਕ_ਪੰਨੇ);
    } ਨਹੀਂਤਾਂ {
        ਨਤੀਜਾ = m3_Err_wasm_ਸਮ੍ਰਿਤੀ_ਉਛਾਲ;
    }

    _catch: ਵਾਪਸ ਨਤੀਜਾ;
}


ਖਾਲੀ Free_ਸਮ੍ਰਿਤੀ_ਖੰਡ (IM3_ਸਮ੍ਰਿਤੀ io_ਸਮ੍ਰਿਤੀ)
{
    ਜੇ (io_ਸਮ੍ਰਿਤੀ->mallocated and io_ਸਮ੍ਰਿਤੀ->mallocated->runtime) {
        io_ਸਮ੍ਰਿਤੀ->mallocated->runtime->ਸਮ੍ਰਿਤੀ_ਬਾਈਟਾਂ_ਵਰਤਿਆ -= io_ਸਮ੍ਰਿਤੀ->mallocated->ਲੰਮਾਈ;
    }
#ਜੇ d_m3_Guarded_ਸਮ੍ਰਿਤੀ
    Guard_Give_ਥਾਂ(io_ਸਮ੍ਰਿਤੀ->guard_ਥਾਂ);
    io_ਸਮ੍ਰਿਤੀ->guard_ਥਾਂ = ਨਿਰਾ;

    io_ਸਮ੍ਰਿਤੀ->mallocated = ਨਿਰਾ;
#ਨਹੀਂਤਾਂ
    m3_Free(io_ਸਮ੍ਰਿਤੀ->mallocated);
#ਜੇ_ਅੰਤ
}


M3_ਨਤੀਜਾ InitGlobals (IM3Module io_module)
{
    M3_ਨਤੀਜਾ ਨਤੀਜਾ = m3Err_none;

    ਜੇ (io_module->ਅੰਕ_Globals) {
        // placing the globals in their structs isn't good for cache locality, but i don't really know what the global
        // access patterns typically look like yet.

        //          io_module->globalMemory = m3Alloc (m3reg_t, io_module->numGlobals);

        //          if (io_module->globalMemory)
        {
            ਲਈ (u32 i = 0; i < io_module->ਅੰਕ_Globals; ++i) {
                M3Global* g = &io_module->globals[i];                           m3log (runtime, "initializing global: %d", i);

                ਜੇ (g->init_ਵਿਅੰਜਕ) {
                    ਬਾਈਟਾਂ_t ਸ਼ੁਰੂ = g->init_ਵਿਅੰਜਕ;

                    ਨਤੀਜਾ = Evaluate_ਵਿਅੰਜਕ(io_module, &g->i64_ਮੁੱਲ, g->type, &ਸ਼ੁਰੂ, g->init_ਵਿਅੰਜਕ + g->init_ਵਿਅੰਜਕ_ਮਾਪ);

                    ਜੇ (not ਨਤੀਜਾ) {
                        // io_module->globalMemory [i] = initValue;
                    } ਨਹੀਂਤਾਂ {
                        ਰੋਕੋ;
                    }
                } ਨਹੀਂਤਾਂ {                                                        m3log (runtime, "importing global");
                }
            }
        }
        //          else result = ErrorModule (m3Err_mallocFailed, io_module, "could allocate globals for module: '%s", io_module->name);
    }

    ਵਾਪਸ ਨਤੀਜਾ;
}


M3_ਨਤੀਜਾ Init_ਅੰਕੜੇ_Segments (IM3Module io_module)
{
    M3_ਨਤੀਜਾ ਨਤੀਜਾ = m3Err_none;

    ਲਈ (u32 i = 0; i < io_module->ਅੰਕ_ਅੰਕੜੇ_Segments; ++i) {
        M3_ਅੰਕੜੇ_Segment* segment = &io_module->ਅੰਕੜੇ_Segments[i];

        // A passive segment stays available for memory.init until data.drop.
        // An active one is copied here and then counts as dropped.
        ਜੇ (segment->isPassive) {
            ਜਾਰੀ;
        }

        _throwif("data segment memory index out of range",
                 segment->ਸਮ੍ਰਿਤੀ_Region >= io_module->ਅੰਕ_Memories);

        IM3_ਸਮ੍ਰਿਤੀ io_ਸਮ੍ਰਿਤੀ = io_module->memories[segment->ਸਮ੍ਰਿਤੀ_Region];

        _throwif("unallocated linear memory", !(io_ਸਮ੍ਰਿਤੀ->mallocated));

        // The offset expression has the memory's address type, and is
        // unsigned: an i32 offset of -1 is 4294967295, way out of bounds
        // rather than negative.
        u64     segment_ਵਿੱਥ = 0;
        ਬਾਈਟਾਂ_t ਸ਼ੁਰੂ         = segment->init_ਵਿਅੰਜਕ;

        ਜੇ (io_ਸਮ੍ਰਿਤੀ->isMemory64) {
_           (Evaluate_ਵਿਅੰਜਕ(io_module, &segment_ਵਿੱਥ, c_m3Type_i64, &ਸ਼ੁਰੂ, segment->init_ਵਿਅੰਜਕ + segment->init_ਵਿਅੰਜਕ_ਮਾਪ));
        } ਨਹੀਂਤਾਂ {
            u32 offset32;
_           (Evaluate_ਵਿਅੰਜਕ(io_module, &offset32, c_m3Type_i32, &ਸ਼ੁਰੂ, segment->init_ਵਿਅੰਜਕ + segment->init_ਵਿਅੰਜਕ_ਮਾਪ));
            segment_ਵਿੱਥ = offset32;
        }

        m3log(runtime, "loading data segment: %d; size: %d; offset: %llu", i, segment->ਮਾਪ, (ਬਿਨਾਨਿਸ਼ਾਨੀ ਲੰਮਾ ਲੰਮਾ)segment_ਵਿੱਥ);

        ਜੇ (segment_ਵਿੱਥ <= io_ਸਮ੍ਰਿਤੀ->mallocated->ਲੰਮਾਈ &&
            (u64)segment->ਮਾਪ <= io_ਸਮ੍ਰਿਤੀ->mallocated->ਲੰਮਾਈ - segment_ਵਿੱਥ) {
            u8* dest = m3_Mem_ਅੰਕੜੇ(io_ਸਮ੍ਰਿਤੀ->mallocated) + segment_ਵਿੱਥ;
            ਸਮ੍ਰਿਤੀ_ਨਕਲ(dest, segment->ਅੰਕੜੇ, segment->ਮਾਪ);
        } ਨਹੀਂਤਾਂ {
            _throw("data segment out of bounds");
        }

        segment->dropped = true;
    }

    _catch: ਵਾਪਸ ਨਤੀਜਾ;
}


// Turns a segment's elements into references. Element expressions are constant
// expressions restricted to ref.null/ref.func, so they're read directly rather
// than run through the compiler.
ਸਥਿਰ
M3_ਨਤੀਜਾ ResolveElements (IM3Module io_module, M3ElementSegment* i_segment, ਖਾਲੀ** o_elements)
{
    M3_ਨਤੀਜਾ ਨਤੀਜਾ = m3Err_none;

    ਬਾਈਟਾਂ_t  pos = i_segment->elements;
    cbytes_t ਸਿਰਾ = io_module->element_Section_ਸਿਰਾ;

    ਲਈ (u32 e = 0; e < i_segment->ਅੰਕ_Elements; ++e) {
        u32   func_ਸੂਚਕਾਂਕ;
        ਖਾਲੀ* ref = ਨਿਰਾ;

        ਜੇ (i_segment->is_ਵਿਅੰਜਕ) {
            m3opcode_t opcode;
_           (Read_opcode(&opcode, &pos, ਸਿਰਾ));

            ਜੇ (opcode == c_waOp_refFunc) {
_               (ReadLEB_u32(&func_ਸੂਚਕਾਂਕ, &pos, ਸਿਰਾ));
                _throwif("function index out of range", func_ਸੂਚਕਾਂਕ >= io_module->ਅੰਕ_Functions);
                ref = Function_Implementation(&io_module->functions[func_ਸੂਚਕਾਂਕ]);
            } ਨਹੀਂਤਾਂ ਜੇ (opcode == c_waOp_refNull) {
                i8 waType;
                u8 nullType;
_               (ReadLEB_i7(&waType, &pos, ਸਿਰਾ));
_               (NormalizeType(&nullType, waType));
                _throwif(m3Err_typeMismatch, nullType != i_segment->type);
            } ਨਹੀਂਤਾਂ ਜੇ (opcode == c_waOp_getGlobal) {
                // wasm 2.0 lets an element expression read an imported
                // immutable global, which is how one module seeds another's
                // table with a reference it exported.
                u32 global_ਸੂਚਕਾਂਕ;
_               (ReadLEB_u32(&global_ਸੂਚਕਾਂਕ, &pos, ਸਿਰਾ));
                _throwif(m3_Err_globa_ਸੂਚਕਾਂਕ_ਬਾਹਰਲਾ_ਦਾ_Bounds, global_ਸੂਚਕਾਂਕ >= io_module->ਅੰਕ_Globals);

                IM3Global global = &io_module->globals[global_ਸੂਚਕਾਂਕ];

                _throwif(m3_Err_globa_ਸੂਚਕਾਂਕ_ਬਾਹਰਲਾ_ਦਾ_Bounds, not global->imported);
                _throwif(m3Err_wasmMalformed, global->isMutable);
                _throwif(m3Err_typeMismatch, ਅਧਾਰ_Type_ਦਾ(global->type) != ਅਧਾਰ_Type_ਦਾ(i_segment->type));

                // read the cell the import was linked to, not the placeholder
                ਜੇ (global->resolved) {
                    global = global->resolved;
                }

                ref = global->ref_ਮੁੱਲ;
            } ਨਹੀਂਤਾਂ {
                _throw("constant expression required");
            }

_           (Read_opcode(&opcode, &pos, ਸਿਰਾ));
            _throwif(m3Err_wasmMalformed, opcode != c_wa_Op_ਸਿਰਾ);
        } ਨਹੀਂਤਾਂ {
_           (ReadLEB_u32(&func_ਸੂਚਕਾਂਕ, &pos, ਸਿਰਾ));
            _throwif("function index out of range", func_ਸੂਚਕਾਂਕ >= io_module->ਅੰਕ_Functions);
            ref = Function_Implementation(&io_module->functions[func_ਸੂਚਕਾਂਕ]);
        }

        o_elements[e] = ref;
    }

    _catch: ਵਾਪਸ ਨਤੀਜਾ;
}


M3_ਨਤੀਜਾ Init_ਸਾਰਣੀ_And_Elements (IM3Module io_module)
{
    M3_ਨਤੀਜਾ ਨਤੀਜਾ = m3Err_none;

    cbytes_t ਸਿਰਾ = io_module->element_Section_ਸਿਰਾ;
    M3_ਸਾਰਣੀ* ਸਾਰਣੀ_2;

    ਲਈ (u32 i = 0; i < io_module->ਅੰਕ_Tables; ++i) {
        ਸਾਰਣੀ_2 = io_module->tables[i];

        // a slot pointing at another module's table is that module's to fill
        ਜੇ (ਸਾਰਣੀ_2->owner != io_module) {
            ਜਾਰੀ;
        }

        ਜੇ (ਸਾਰਣੀ_2->ਮਾਪ) {
            IM3Runtime runtime = io_module->runtime;
            _throwif(m3_Err_ਸਾਰਣੀ_ਹੱਦਬੰਦੀ_Exceeded, runtime->ਸਾਰਣੀ_Elements_ਹੱਦਬੰਦੀ and
                                                 ਸਾਰਣੀ_2->ਮਾਪ > runtime->ਸਾਰਣੀ_Elements_ਹੱਦਬੰਦੀ - runtime->ਸਾਰਣੀ_Elements_ਵਰਤਿਆ);
            ਸਾਰਣੀ_2->elements = m3_AllocArray(ਖਾਲੀ*, ਸਾਰਣੀ_2->ਮਾਪ);
            _throwifnull(ਸਾਰਣੀ_2->elements);
            runtime->ਸਾਰਣੀ_Elements_ਵਰਤਿਆ += ਸਾਰਣੀ_2->ਮਾਪ;

            ਜੇ (ਸਾਰਣੀ_2->init_ਵਿਅੰਜਕ) {
                ਖਾਲੀ*   ਮੁੱਲ = ਨਿਰਾ;
                ਬਾਈਟਾਂ_t ਸ਼ੁਰੂ = ਸਾਰਣੀ_2->init_ਵਿਅੰਜਕ;
_               (Evaluate_ਵਿਅੰਜਕ(io_module, &ਮੁੱਲ, ਅਧਾਰ_Type_ਦਾ(ਸਾਰਣੀ_2->type),
                                     &ਸ਼ੁਰੂ, ਸਾਰਣੀ_2->init_ਵਿਅੰਜਕ + ਸਾਰਣੀ_2->init_ਵਿਅੰਜਕ_ਮਾਪ));

                ਲਈ (u32 e = 0; e < ਸਾਰਣੀ_2->ਮਾਪ; ++e) {
                    ਸਾਰਣੀ_2->elements[e] = ਮੁੱਲ;
                }
            }
        }
    }

    ਲਈ (u32 i = 0; i < io_module->ਅੰਕ_Element_Segments; ++i) {
        M3ElementSegment* segment = &io_module->elementSegments[i];

        // Declarative segments only make their functions referenceable, and
        // passive ones wait for table.init, so neither is written out here.
        ਜੇ (segment->ਬਹੁਲਕ == c_m3Elem_declarative) {
            segment->dropped = true;
            ਜਾਰੀ;
        }

        ਜੇ (segment->ਬਹੁਲਕ == c_m3Elem_passive) {
            ਜੇ (segment->ਅੰਕ_Elements) {
                segment->resolved = m3_AllocArray(ਖਾਲੀ*, segment->ਅੰਕ_Elements);
                _throwifnull(segment->resolved);
_               (ResolveElements(io_module, segment, segment->resolved));
            }
            ਜਾਰੀ;
        }

        ਸਾਰਣੀ_2 = io_module->tables[segment->ਸਾਰਣੀ_ਸੂਚਕਾਂਕ];

        // The offset expression has the table's index type, and is unsigned:
        // an i32 offset of -1 is 4294967295, out of bounds rather than negative.
        u64     offset = 0;
        ਬਾਈਟਾਂ_t ਵਿਅੰਜਕ   = segment->init_ਵਿਅੰਜਕ;

        ਜੇ (ਸਾਰਣੀ_2->isTable64) {
_           (Evaluate_ਵਿਅੰਜਕ(io_module, &offset, c_m3Type_i64, &ਵਿਅੰਜਕ, ਸਿਰਾ));
        } ਨਹੀਂਤਾਂ {
            u32 offset32;
_           (Evaluate_ਵਿਅੰਜਕ(io_module, &offset32, c_m3Type_i32, &ਵਿਅੰਜਕ, ਸਿਰਾ));
            offset = offset32;
        }

        _throwif("out of bounds table access",
                 offset > ਸਾਰਣੀ_2->ਮਾਪ or segment->ਅੰਕ_Elements > ਸਾਰਣੀ_2->ਮਾਪ - offset);

_       (ResolveElements(io_module, segment, ਸਾਰਣੀ_2->elements + offset));

        segment->dropped = true;
    }

    _catch: ਵਾਪਸ ਨਤੀਜਾ;
}

M3_ਨਤੀਜਾ m3_CompileModule (IM3Module io_module)
{
    M3_ਨਤੀਜਾ ਨਤੀਜਾ = m3Err_none;

    ਲਈ (u32 i = 0; i < io_module->ਅੰਕ_Functions; ++i) {
        IM3Function f = &io_module->functions[i];
        ਜੇ (f->wasm and not f->compiled) {
_           (CompileFunction(f));
        }
    }

    _catch: ਵਾਪਸ ਨਤੀਜਾ;
}

#ਜੇ d_m3HasExceptionHandling

M3Exception* NewException (IM3Runtime io_runtime, IM3_ਟੈਗ i_ਟੈਗ, u32 i_ਅੰਕ_ਦਲੀਲਾਂ)
{
    M3Exception* exception = (M3Exception*)m3_Malloc("M3Exception", ਆਕਾਰ(M3Exception) + i_ਅੰਕ_ਦਲੀਲਾਂ * ਆਕਾਰ(u64));

    ਜੇ (exception) {
        exception->ਟੈਗ     = i_ਟੈਗ;
        exception->ਅੰਕ_ਦਲੀਲਾਂ = i_ਅੰਕ_ਦਲੀਲਾਂ;
        exception->reified = false;
        exception->prev    = ਨਿਰਾ;
        exception->ਅਗਲਾ    = io_runtime->exceptions;

        ਜੇ (exception->ਅਗਲਾ) {
            exception->ਅਗਲਾ->prev = exception;
        }

        io_runtime->exceptions = exception;
    }

    ਵਾਪਸ exception;
}


// Releases one exception ahead of the rest. The caller has to know nothing can
// still name it: no exnref was ever taken of it, and its payload has already
// been copied out.
ਖਾਲੀ FreeException (IM3Runtime io_runtime, M3Exception* i_exception)
{
    ਜੇ (i_exception->prev) {
        i_exception->prev->ਅਗਲਾ = i_exception->ਅਗਲਾ;
    } ਨਹੀਂਤਾਂ {
        io_runtime->exceptions = i_exception->ਅਗਲਾ;
    }

    ਜੇ (i_exception->ਅਗਲਾ) {
        i_exception->ਅਗਲਾ->prev = i_exception->prev;
    }

    ਜੇ (io_runtime->pendingException == i_exception) {
        io_runtime->pendingException = ਨਿਰਾ;
    }

    m3_Free_Impl(i_exception);
}


// Releases every exception the runtime still holds. Only safe once the Wasm
// stack is empty, which is why the outermost RunCodeChecked() is the one that
// calls it.
ਖਾਲੀ FreeExceptions (IM3Runtime io_runtime)
{
    M3Exception* exception = io_runtime->exceptions;

    io_runtime->exceptions       = ਨਿਰਾ;
    io_runtime->pendingException = ਨਿਰਾ;

    ਜਦੋਂ (exception) {
        M3Exception* ਅਗਲਾ = exception->ਅਗਲਾ;
        m3_Free_Impl(exception);
        exception = ਅਗਲਾ;
    }
}


// Keeps a copy of the exception a call just ended with, for m3_GetExceptionTag,
// or forgets the last one when the call ended any other way. A copy, because
// the original is freed with the rest before the outermost call returns - and
// one a host function's nested call ended with may still be named by an
// exnref further down the stack.
ਸਥਿਰ
ਖਾਲੀ ਰਿਕਾਰਡ_Uncaught_Exception (IM3Runtime io_runtime, M3_ਨਤੀਜਾ i_ਨਤੀਜਾ)
{
    ਜੇ (io_runtime->uncaughtException) {
        m3_Free(io_runtime->uncaughtException);
    }

    M3Exception* exception = io_runtime->pendingException;

    ਜੇ (M3_UNLIKELY(i_ਨਤੀਜਾ == m3Err_trapUncaughtException) and exception) {
        M3Exception* copy = (M3Exception*)m3_CopyMem(exception, ਆਕਾਰ(M3Exception) + exception->ਅੰਕ_ਦਲੀਲਾਂ * ਆਕਾਰ(u64));

        ਜੇ (copy) {
            copy->ਅਗਲਾ = ਨਿਰਾ;
            copy->prev = ਨਿਰਾ;
        }

        io_runtime->uncaughtException = copy;
    }
}

#ਜੇ_ਅੰਤ // d_m3HasExceptionHandling


#ਜੇ d_m3HasExceptionHandling || d_m3_Has_ਢੇਰ_Switching

IM3_ਟੈਗ m3_ਲੱਭੋ_ਟੈਗ (IM3Module i_module, ਅਚਲ ਅੱਖਰ* ਅਚਲ i_ਟੈਗ_Name)
{
    ਜੇ (not i_module or not i_ਟੈਗ_Name) {
        ਵਾਪਸ ਨਿਰਾ;
    }

    ਵਾਪਸ Module_ਲੱਭੋ_Exported_ਟੈਗ(i_module, i_ਟੈਗ_Name);
}


M3_ਨਤੀਜਾ m3_New_ਟੈਗ (IM3Runtime io_runtime, IM3_ਟੈਗ* o_ਟੈਗ, ਅਚਲ ਅੱਖਰ* ਅਚਲ i_signature)
{
    IM3FuncType type    = ਨਿਰਾ;
    M3_Host_ਟੈਗ*  host_ਟੈਗ = ਨਿਰਾ;

_try {
    _throwif("null tag", not o_ਟੈਗ);
    *o_ਟੈਗ = ਨਿਰਾ;
    _throwif(m3Err_mallocFailed, not io_runtime);

_   (Signature_ਮੰਜ਼ਿਲ_Func_Type(&type, i_signature));
    // tags are the same type exactly when their types are the same object
_   (Environment_ਜੋੜੋ_Func_Type(io_runtime->environment, &type));

    host_ਟੈਗ = m3_AllocStruct(M3_Host_ਟੈਗ);
    _throwifnull(host_ਟੈਗ);

    host_ਟੈਗ->ਟੈਗ.type    = type;
    host_ਟੈਗ->ਅਗਲਾ        = io_runtime->host_ਟੈਗ_ਸੂਚੀ;
    io_runtime->host_ਟੈਗ_ਸੂਚੀ = host_ਟੈਗ;

    *o_ਟੈਗ = &host_ਟੈਗ->ਟੈਗ;
} _catch:
    ਵਾਪਸ ਨਤੀਜਾ;
}


M3_ਨਤੀਜਾ m3_Link_ਟੈਗ (IM3Module         io_module,
                     ਅਚਲ ਅੱਖਰ* ਅਚਲ i_moduleName,
                     ਅਚਲ ਅੱਖਰ* ਅਚਲ i_ਟੈਗ_Name,
                     IM3_ਟੈਗ            i_ਟੈਗ)
{
    ਜੇ (not io_module or not i_moduleName or not i_ਟੈਗ_Name or not i_ਟੈਗ) {
        ਵਾਪਸ m3_Err_ਟੈਗ_Lookup_Failed;
    }

    ਅਚਲ bool wildcardModule = (strcmp(i_moduleName, "*") == 0);
    IM3_ਟੈਗ     ਨਿਸ਼ਾਨਾ         = i_ਟੈਗ->resolved ? i_ਟੈਗ->resolved : i_ਟੈਗ;

    M3_ਨਤੀਜਾ ਨਤੀਜਾ = m3_Err_ਟੈਗ_Lookup_Failed;

    ਲਈ (u32 i = 0; i < io_module->ਅੰਕ_ਟੈਗ_ਸੂਚੀ; ++i) {
        IM3_ਟੈਗ ਟੈਗ = &io_module->ਟੈਗ_ਸੂਚੀ[i];

        ਜੇ (ਟੈਗ->imported and ਟੈਗ->import.moduleUtf8 and ਟੈਗ->import.ਖੇਤਰ_Utf8 and
            strcmp(ਟੈਗ->import.ਖੇਤਰ_Utf8, i_ਟੈਗ_Name) == 0 and
            (wildcardModule or strcmp(ਟੈਗ->import.moduleUtf8, i_moduleName) == 0)) {
            ਜੇ (ਟੈਗ->type != ਨਿਸ਼ਾਨਾ->type) {
                ਵਾਪਸ m3Err_incompatibleImportType;
            }

            ਟੈਗ->resolved = ਨਿਸ਼ਾਨਾ;
            ਨਤੀਜਾ        = m3Err_none;
        }
    }

    ਵਾਪਸ ਨਤੀਜਾ;
}


uint32_t m3_Get_ਟੈਗ_Arg_ਗਿਣਤੀ (IM3_ਟੈਗ i_ਟੈਗ)
{
    ਵਾਪਸ i_ਟੈਗ ? i_ਟੈਗ->type->ਅੰਕ_ਦਲੀਲਾਂ : 0;
}


M3_ਮੁੱਲ_Type m3_Get_ਟੈਗ_Arg_Type (IM3_ਟੈਗ i_ਟੈਗ, uint32_t i_ਸੂਚਕਾਂਕ)
{
    ਜੇ (i_ਟੈਗ and i_ਸੂਚਕਾਂਕ < i_ਟੈਗ->type->ਅੰਕ_ਦਲੀਲਾਂ) {
        ਵਾਪਸ (M3_ਮੁੱਲ_Type)ਅਧਾਰ_Type_ਦਾ(d_FuncArgType(i_ਟੈਗ->type, i_ਸੂਚਕਾਂਕ));
    }

    ਵਾਪਸ c_m3Type_none;
}

#ਨਹੀਂਤਾਂ

IM3_ਟੈਗ m3_ਲੱਭੋ_ਟੈਗ (IM3Module i_module, ਅਚਲ ਅੱਖਰ* ਅਚਲ i_ਟੈਗ_Name)
{
    ਵਾਪਸ ਨਿਰਾ;
}

M3_ਨਤੀਜਾ m3_New_ਟੈਗ (IM3Runtime io_runtime, IM3_ਟੈਗ* o_ਟੈਗ, ਅਚਲ ਅੱਖਰ* ਅਚਲ i_signature)
{
    ਜੇ (o_ਟੈਗ) {
        *o_ਟੈਗ = ਨਿਰਾ;
    }
    ਵਾਪਸ m3_Err_ਟੈਗ_Lookup_Failed;
}

M3_ਨਤੀਜਾ m3_Link_ਟੈਗ (IM3Module io_module, ਅਚਲ ਅੱਖਰ* ਅਚਲ i_moduleName, ਅਚਲ ਅੱਖਰ* ਅਚਲ i_ਟੈਗ_Name, IM3_ਟੈਗ i_ਟੈਗ)
{
    ਵਾਪਸ m3_Err_ਟੈਗ_Lookup_Failed;
}

uint32_t m3_Get_ਟੈਗ_Arg_ਗਿਣਤੀ (IM3_ਟੈਗ i_ਟੈਗ)
{
    ਵਾਪਸ 0;
}

M3_ਮੁੱਲ_Type m3_Get_ਟੈਗ_Arg_Type (IM3_ਟੈਗ i_ਟੈਗ, uint32_t i_ਸੂਚਕਾਂਕ)
{
    ਵਾਪਸ c_m3Type_none;
}

#ਜੇ_ਅੰਤ // d_m3HasExceptionHandling || d_m3HasStackSwitching


#ਜੇ d_m3HasExceptionHandling

M3_ਨਤੀਜਾ m3_ThrowException (IM3Runtime io_runtime, IM3_ਟੈਗ i_ਟੈਗ, uint32_t i_argc, ਅਚਲ ਖਾਲੀ* i_argptrs[])
{
    ਜੇ (not io_runtime or not i_ਟੈਗ) {
        ਵਾਪਸ m3_Err_ਟੈਗ_Lookup_Failed;
    }

    IM3_ਟੈਗ      ਟੈਗ  = i_ਟੈਗ->resolved ? i_ਟੈਗ->resolved : i_ਟੈਗ;
    IM3FuncType type = ਟੈਗ->type;

    // a tag with results is a stack switching control tag, which is suspended
    // with rather than thrown
    ਜੇ (type->ਅੰਕ_Rets) {
        ਵਾਪਸ m3Err_typeMismatch;
    }
    ਜੇ (i_argc != type->ਅੰਕ_ਦਲੀਲਾਂ) {
        ਵਾਪਸ m3_Err_ਦਲੀਲ_ਗਿਣਤੀ_Mismatch;
    }
    // with nothing on the stack there is nothing to unwind, and the exception
    // would sit on the runtime's list until some later call ended
    ਜੇ (io_runtime->callNesting == 0) {
        ਵਾਪਸ "m3_ThrowException called outside a host function";
    }

    M3Exception* exception = NewException(io_runtime, ਟੈਗ, i_argc);
    ਜੇ (not exception) {
        ਵਾਪਸ m3Err_mallocFailed;
    }

    // the layout op_Throw gives a payload: 32-bit values zero-extended, floats
    // as their bit pattern
    ਲਈ (u32 i = 0; i < i_argc; ++i) {
        ਅਚਲ ਖਾਲੀ* ਮੁੱਲ = i_argptrs[i];

        ਚੋਣ (ਅਧਾਰ_Type_ਦਾ(d_FuncArgType(type, i))) {
        ਹਾਲਤ c_m3Type_i32:
        ਹਾਲਤ c_m3Type_f32: {
            u32 ਬਿੱਟ;
            ਸਮ੍ਰਿਤੀ_ਨਕਲ(&ਬਿੱਟ, ਮੁੱਲ, ਆਕਾਰ(ਬਿੱਟ));
            exception->ਦਲੀਲਾਂ[i] = ਬਿੱਟ;
            ਰੋਕੋ;
        }
        ਹਾਲਤ c_m3Type_i64:
        ਹਾਲਤ c_m3Type_f64:
            ਸਮ੍ਰਿਤੀ_ਨਕਲ(&exception->ਦਲੀਲਾਂ[i], ਮੁੱਲ, ਆਕਾਰ(u64));
            ਰੋਕੋ;
        ਹਾਲਤ c_m3Type_funcref:
        ਹਾਲਤ c_m3Type_externref:
        ਹਾਲਤ c_m3Type_exnref:
        ਹਾਲਤ c_m3Type_contref:
            exception->ਦਲੀਲਾਂ[i] = (u64)(*(ਅਚਲ uintptr_t*)ਮੁੱਲ);
            ਰੋਕੋ;
        ਮੂਲ:
            FreeException(io_runtime, exception);
            ਵਾਪਸ "unknown argument type";
        }
    }

    io_runtime->pendingException = exception;

    ਵਾਪਸ m3Err_pendingException;
}


IM3_ਟੈਗ m3_Get_Exception_ਟੈਗ (IM3Runtime i_runtime)
{
    ਜੇ (i_runtime and i_runtime->uncaughtException) {
        ਵਾਪਸ i_runtime->uncaughtException->ਟੈਗ;
    }

    ਵਾਪਸ ਨਿਰਾ;
}


M3_ਨਤੀਜਾ m3_Get_Exception_ਦਲੀਲਾਂ (IM3Runtime i_runtime, uint32_t i_argc, ਅਚਲ ਖਾਲੀ* o_argptrs[])
{
    ਜੇ (not i_runtime or not i_runtime->uncaughtException) {
        ਵਾਪਸ "no uncaught exception";
    }

    ਅਚਲ M3Exception* exception = i_runtime->uncaughtException;
    IM3FuncType        type      = exception->ਟੈਗ->type;

    ਜੇ (i_argc != exception->ਅੰਕ_ਦਲੀਲਾਂ) {
        ਵਾਪਸ m3_Err_ਦਲੀਲ_ਗਿਣਤੀ_Mismatch;
    }

    ਲਈ (u32 i = 0; i < i_argc; ++i) {
        ਖਾਲੀ* ਮੁੱਲ = (ਖਾਲੀ*)o_argptrs[i];
        u64   ਬਿੱਟ  = exception->ਦਲੀਲਾਂ[i];

        ਚੋਣ (ਅਧਾਰ_Type_ਦਾ(d_FuncArgType(type, i))) {
        ਹਾਲਤ c_m3Type_i32:
        ਹਾਲਤ c_m3Type_f32: {
            u32 ਨੀਵਾਂ = (u32)ਬਿੱਟ;
            ਸਮ੍ਰਿਤੀ_ਨਕਲ(ਮੁੱਲ, &ਨੀਵਾਂ, ਆਕਾਰ(ਨੀਵਾਂ));
            ਰੋਕੋ;
        }
        ਹਾਲਤ c_m3Type_i64:
        ਹਾਲਤ c_m3Type_f64:
            ਸਮ੍ਰਿਤੀ_ਨਕਲ(ਮੁੱਲ, &ਬਿੱਟ, ਆਕਾਰ(ਬਿੱਟ));
            ਰੋਕੋ;
        ਹਾਲਤ c_m3Type_funcref:
        ਹਾਲਤ c_m3Type_externref:
        ਹਾਲਤ c_m3Type_exnref:
        ਹਾਲਤ c_m3Type_contref:
            *(uintptr_t*)ਮੁੱਲ = (uintptr_t)ਬਿੱਟ;
            ਰੋਕੋ;
        ਮੂਲ:
            ਵਾਪਸ "unknown argument type";
        }
    }

    ਵਾਪਸ m3Err_none;
}

#ਨਹੀਂਤਾਂ

M3_ਨਤੀਜਾ m3_ThrowException (IM3Runtime io_runtime, IM3_ਟੈਗ i_ਟੈਗ, uint32_t i_argc, ਅਚਲ ਖਾਲੀ* i_argptrs[])
{
    ਵਾਪਸ m3_Err_ਟੈਗ_Lookup_Failed;
}

IM3_ਟੈਗ m3_Get_Exception_ਟੈਗ (IM3Runtime i_runtime)
{
    ਵਾਪਸ ਨਿਰਾ;
}

M3_ਨਤੀਜਾ m3_Get_Exception_ਦਲੀਲਾਂ (IM3Runtime i_runtime, uint32_t i_argc, ਅਚਲ ਖਾਲੀ* o_argptrs[])
{
    ਵਾਪਸ "no uncaught exception";
}

#ਜੇ_ਅੰਤ // d_m3HasExceptionHandling


#ਜੇ d_m3_Guarded_ਸਮ੍ਰਿਤੀ

// What RunCodeProtected hands to the platform layer to run, and what comes back
ਕਿਸਮਪਰਿਭਾਸ਼ਾ ਬਣਤਰ M3_Protected_ਲੜੀਵਾਰ {
    pc_t            pc;
    IM3Runtime      runtime;
    M3_ਸਮ੍ਰਿਤੀ_ਸਿਰਲੇਖ* mem;
    M3_ਨਤੀਜਾ        ਨਤੀਜਾ;
} M3_Protected_ਲੜੀਵਾਰ;

ਸਥਿਰ
ਖਾਲੀ ਲੜੀਵਾਰ_ਕੋਡ_Body (ਖਾਲੀ* io_context)
{
    M3_Protected_ਲੜੀਵਾਰ* ਲੜੀਵਾਰ = (M3_Protected_ਲੜੀਵਾਰ*)io_context;

#  ਜੇ (d_m3EnableOpProfiling || d_m3EnableOpTracing)
    ਲੜੀਵਾਰ->ਨਤੀਜਾ = (M3_ਨਤੀਜਾ)ਲੜੀਵਾਰ_ਕੋਡ(ਲੜੀਵਾਰ->pc, (m3stack_t)ਲੜੀਵਾਰ->runtime->ਢੇਰ, ਲੜੀਵਾਰ->mem,
                                    d_m3_Op_Default_ਦਲੀਲਾਂ, d_m3_ਅਧਾਰ_Cstr);
#  ਨਹੀਂਤਾਂ
    ਲੜੀਵਾਰ->ਨਤੀਜਾ = (M3_ਨਤੀਜਾ)ਲੜੀਵਾਰ_ਕੋਡ(ਲੜੀਵਾਰ->pc, (m3stack_t)ਲੜੀਵਾਰ->runtime->ਢੇਰ, ਲੜੀਵਾਰ->mem,
                                    d_m3_Op_Default_ਦਲੀਲਾਂ);
#  endif
}

// The bounds check the load and store operations no longer do, done once here by the
// system instead: an address past the end of a memory reaches the uncommitted part
// of that memory's reservation, and the fault comes back as the trap the spec asks
// for. Nothing is left half-done by it - the Wasm stack and the memories are heap,
// and the caller unwinds the rest.
ਸਥਿਰ
M3_ਨਤੀਜਾ ਲੜੀਵਾਰ_ਕੋਡ_Protected (IM3Runtime i_runtime, pc_t i_pc, M3_ਸਮ੍ਰਿਤੀ_ਸਿਰਲੇਖ* i_mem)
{
    // Memory operations in this build carry no bounds check, so code only runs where a
    // fault past the end of a memory can still be turned into a trap
    ਜੇ (M3_UNLIKELY(not m3_Host_Guards_ਸਰਗਰਮ())) {
        ਵਾਪਸ ErrorRuntime(m3_Err_ਸਮ੍ਰਿਤੀ_Guards_Unavailable, i_runtime,
                            "another part of the process holds the fault signal");
    }

    M3_Protected_ਲੜੀਵਾਰ ਲੜੀਵਾਰ;
    ਲੜੀਵਾਰ.pc      = i_pc;
    ਲੜੀਵਾਰ.runtime = i_runtime;
    ਲੜੀਵਾਰ.mem     = i_mem;
    ਲੜੀਵਾਰ.ਨਤੀਜਾ  = m3Err_none;

    ਖਾਲੀ*  arena;
    size_t arena_ਬਾਈਟਾਂ;
    Guard_ArenaRange(&arena, &arena_ਬਾਈਟਾਂ);

    ਜੇ (not m3_HostProtectedCall(ਲੜੀਵਾਰ_ਕੋਡ_Body, &ਲੜੀਵਾਰ, arena, arena_ਬਾਈਟਾਂ)) {
        ਵਾਪਸ m3_Err_trap_ਬਾਹਰਲਾ_ਦਾ_Bounds_ਸਮ੍ਰਿਤੀ_Access;
    }

    ਵਾਪਸ ਲੜੀਵਾਰ.ਨਤੀਜਾ;
}

#ਜੇ_ਅੰਤ // d_m3GuardedMemory


#ਜੇ d_m3_Has_ਢੇਰ_Switching

// The function a suspended continuation stopped in: the callee of its innermost
// call frame, or the function it was entered with if it made no call. Frames are
// recorded innermost first, so that is the lowest-numbered call frame.
ਸਥਿਰ
IM3Function Continuation_InnermostFunction (IM3Continuation i_cont)
{
    ਲਈ (u32 i = 0; i < i_cont->ਅੰਕ_ਚੌਖਟੇ; ++i) {
        ਜੇ (i_cont->ਚੌਖਟੇ[i].ਕਿਸਮ == ਚੌਖਟਾ_call && i_cont->ਚੌਖਟੇ[i].call.function) {
            ਵਾਪਸ i_cont->ਚੌਖਟੇ[i].call.function;
        }
    }

    ਵਾਪਸ i_cont->ਇੰਦਰਾਜ_Function;
}


// Where the host's next call puts its arguments and finds its results. A
// suspended root keeps its frames on the bottom of the runtime's stack, so a call
// made before it is resumed starts above the last of them rather than on top of
// them. The top is the innermost function's frame base plus everything that
// function can use. That base is the root's own sp, unless it stopped inside a
// resume: then the suspension point belongs to the resumed continuation, and the
// root's innermost frame is that resume.
ਖਾਲੀ Runtime_ਰੱਖੋ_Call_ਢੇਰ (IM3Runtime io_runtime)
{
    IM3Continuation ਜੜ੍ਹ = io_runtime->ਜੜ੍ਹ_Continuation;

    io_runtime->ਢੇਰ = io_runtime->origin_ਢੇਰ;

    ਜੇ (not ਜੜ੍ਹ or ਜੜ੍ਹ->state != cont_suspended) {
        ਵਾਪਸ;
    }

    m3stack_t   sp       = ਜੜ੍ਹ->sp;
    IM3Function function = Continuation_InnermostFunction(ਜੜ੍ਹ);

    ਜੇ (ਜੜ੍ਹ->ਅੰਕ_ਚੌਖਟੇ and ਜੜ੍ਹ->ਚੌਖਟੇ[0].ਕਿਸਮ == ਚੌਖਟਾ_resume) {
        sp = ਜੜ੍ਹ->ਚੌਖਟੇ[0].sp;
    }

    m3slot_t* ਅਧਾਰ = (m3slot_t*)io_runtime->origin_ਢੇਰ;
    m3slot_t* ਸਿਖਰ  = (m3slot_t*)sp + (function ? function->max_ਢੇਰ_Slots : 0);

    ਜੇ (ਸਿਖਰ > ਅਧਾਰ + io_runtime->ਅੰਕ_ਢੇਰ_Slots) {
        ਸਿਖਰ = ਅਧਾਰ + io_runtime->ਅੰਕ_ਢੇਰ_Slots;
    }

    io_runtime->ਢੇਰ = ਸਿਖਰ;
}

#ਜੇ_ਅੰਤ // d_m3HasStackSwitching


// Run compiled code on the runtime's stack, bounding native recursion for the
// duration of the call. The outermost invocation establishes the stack limit;
// nested ones (an imported function calling back into Wasm) inherit it.
ਸਥਿਰ ਸਤਰਅੰਦਰ
M3_ਨਤੀਜਾ ਲੜੀਵਾਰ_ਕੋਡ_Checked (IM3Runtime i_runtime, IM3Function i_function)
{
    pc_t i_pc = i_function->compiled;

    // execution runs against the memory of the module the entry point belongs
    // to, not against some runtime-wide one
    M3_ਸਮ੍ਰਿਤੀ_ਸਿਰਲੇਖ* _mem = Module_ਸਮ੍ਰਿਤੀ_ਸਿਰਲੇਖ(i_function->module);

#ਜੇ d_m3HasSnapshots
    ਜੇ (M3_UNLIKELY(i_function->module->isUnusable)) {
        ਵਾਪਸ "a snapshot failed to restore into the module, so it cannot run";
    }
    i_function->module->has_ਲੜੀਵਾਰ = true;
#ਜੇ_ਅੰਤ

    d_m3_ਢੇਰ_ਹੱਦਬੰਦੀ_Enter(i_runtime);

    // a host function calling back into Wasm does not change which call a trap
    // ends: that is still the one the host made first
    ਜੇ (i_runtime->callNesting++ == 0) {
        i_runtime->entered = i_function;
    }
#ਜੇ d_m3HasExceptionHandling
    // handler stacks don't nest across a call boundary: a host function calling
    // back into Wasm cannot be caught by a try_table its own caller entered
    u32 saved_Try_ਡੂੰਘਾਈ   = i_runtime->try_ਡੂੰਘਾਈ;
    i_runtime->try_ਡੂੰਘਾਈ = 0;
#ਜੇ_ਅੰਤ
#ਜੇ d_m3_Has_ਢੇਰ_Switching
    // nor do continuations: the native frames on the far side of a host call
    // are not ours to record, so a suspend in here has no handler to reach and
    // no resume to unwind to
    IM3Continuation savedContinuation = i_runtime->ਸਰਗਰਮ_Continuation;

    // A suspended root continuation is a paused program waiting to be resumed,
    // not scratch space. A call the host makes before resuming it gets a
    // context of its own, the way any other re-entry does, rather than having
    // the frames of the suspension reset out from under it.
    bool claim_ਜੜ੍ਹ = i_runtime->isSuspendable and not savedContinuation and
                     not (i_runtime->ਜੜ੍ਹ_Continuation and
                          i_runtime->ਜੜ੍ਹ_Continuation->state == cont_suspended);

    ਜੇ (claim_ਜੜ੍ਹ and not i_runtime->ਜੜ੍ਹ_Continuation) {
        i_runtime->ਜੜ੍ਹ_Continuation = Continuation_New(i_runtime, i_function->funcType, i_function);

        ਜੇ (i_runtime->ਜੜ੍ਹ_Continuation) {
            // The root borrows the runtime's stack and is not charged as cont.new.
            i_runtime->ਜੜ੍ਹ_Continuation->ਅੰਕ_ਢੇਰ_Slots = i_runtime->ਅੰਕ_ਢੇਰ_Slots;
        } ਨਹੀਂਤਾਂ {
            claim_ਜੜ੍ਹ = false;
        }
    }

    ਜੇ (claim_ਜੜ੍ਹ) {
        i_runtime->ਸਰਗਰਮ_Continuation              = i_runtime->ਜੜ੍ਹ_Continuation;
        i_runtime->ਜੜ੍ਹ_Continuation->ਇੰਦਰਾਜ_Function = i_function;
        i_runtime->ਜੜ੍ਹ_Continuation->state         = cont_running;
        i_runtime->ਜੜ੍ਹ_Continuation->ਅੰਕ_ਚੌਖਟੇ     = 0;
    } ਨਹੀਂਤਾਂ {
        i_runtime->ਸਰਗਰਮ_Continuation = ਨਿਰਾ;
    }
#ਜੇ_ਅੰਤ
#ਜੇ d_m3_Guarded_ਸਮ੍ਰਿਤੀ
    M3_ਨਤੀਜਾ ਨਤੀਜਾ = ਲੜੀਵਾਰ_ਕੋਡ_Protected(i_runtime, i_pc, _mem);
#ਜਾਂਜੇ (d_m3EnableOpProfiling || d_m3EnableOpTracing)
    M3_ਨਤੀਜਾ ਨਤੀਜਾ = (M3_ਨਤੀਜਾ)ਲੜੀਵਾਰ_ਕੋਡ(i_pc, (m3stack_t)i_runtime->ਢੇਰ, _mem, d_m3_Op_Default_ਦਲੀਲਾਂ, d_m3_ਅਧਾਰ_Cstr);
#ਨਹੀਂਤਾਂ
    M3_ਨਤੀਜਾ ਨਤੀਜਾ = (M3_ਨਤੀਜਾ)ਲੜੀਵਾਰ_ਕੋਡ(i_pc, (m3stack_t)i_runtime->ਢੇਰ, _mem, d_m3_Op_Default_ਦਲੀਲਾਂ);
#ਜੇ_ਅੰਤ
#ਜੇ d_m3_Has_ਢੇਰ_Switching
    ਜੇ (claim_ਜੜ੍ਹ) {
        i_runtime->ਜੜ੍ਹ_Continuation->state =
          (ਨਤੀਜਾ == m3Err_continuationSuspended) ? cont_suspended : cont_returned;

        Runtime_ਰੱਖੋ_Call_ਢੇਰ(i_runtime);
    }
    i_runtime->ਸਰਗਰਮ_Continuation = savedContinuation;
#ਜੇ_ਅੰਤ
#ਜੇ d_m3HasExceptionHandling
    i_runtime->try_ਡੂੰਘਾਈ = saved_Try_ਡੂੰਘਾਈ;

    // an exception that reached the bottom of the call stack found no handler
    ਜੇ (M3_UNLIKELY(ਨਤੀਜਾ == m3Err_pendingException)) {
        ਨਤੀਜਾ = m3Err_trapUncaughtException;
    }
    ਰਿਕਾਰਡ_Uncaught_Exception(i_runtime, ਨਤੀਜਾ);

    // a paused invocation can still reach the exceptions it holds, so they wait
    // for it to finish
    ਜੇ (i_runtime->callNesting == 1 and not m3_IsSuspended(i_runtime)) {
        FreeExceptions(i_runtime);
    }
#ਜੇ_ਅੰਤ
    i_runtime->callNesting--;
    d_m3_ਢੇਰ_ਹੱਦਬੰਦੀ_Leave(i_runtime);

    ਵਾਪਸ ਨਤੀਜਾ;
}

M3_ਨਤੀਜਾ m3_ਲੜੀਵਾਰ_ਸ਼ੁਰੂ (IM3Module io_module)
{
#ਜੇ_ਪਰਿਭਾਸ਼ਿਤ FUZZING_BUILD_ਬਹੁਲਕ_UNSAFE_FOR_PRODUCTION
    // Execution disabled for fuzzing builds
    ਵਾਪਸ m3Err_none;
#ਜੇ_ਅੰਤ

    M3_ਨਤੀਜਾ ਨਤੀਜਾ           = m3Err_none;
    i32      ਸ਼ੁਰੂ_Function_Tmp = -1;

    ਜੇ (io_module and io_module->ਸ਼ੁਰੂ_Function >= 0) {
        IM3Function function = &io_module->functions[io_module->ਸ਼ੁਰੂ_Function];

        ਜੇ (not function->compiled) {
_           (CompileFunction(function));
        }

        IM3FuncType ftype = function->funcType;
        ਜੇ (ftype->ਅੰਕ_ਦਲੀਲਾਂ != 0 || ftype->ਅੰਕ_Rets != 0) {
            _throw(m3_Err_ਦਲੀਲ_ਗਿਣਤੀ_Mismatch);
        }

        IM3Module  module  = function->module;
        IM3Runtime runtime = module->runtime;

        ਸ਼ੁਰੂ_Function_Tmp = io_module->ਸ਼ੁਰੂ_Function;

        io_module->ਸ਼ੁਰੂ_Function = -1;

        ਨਤੀਜਾ = ਲੜੀਵਾਰ_ਕੋਡ_Checked(runtime, function);

        // a start function that suspended is not to be started again: resuming
        // is what finishes it
        ਜੇ (ਨਤੀਜਾ and ਨਤੀਜਾ != m3Err_continuationSuspended) {
            io_module->ਸ਼ੁਰੂ_Function = ਸ਼ੁਰੂ_Function_Tmp;
            EXCEPTION_PRINT(ਨਤੀਜਾ);
            ਜਾਓ _catch;
        }
    }

    _catch: ਵਾਪਸ ਨਤੀਜਾ;
}

// TODO: deal with main + side-modules loading efforcement
M3_ਨਤੀਜਾ m3_ਲੱਦੋ_Module (IM3Runtime io_runtime, IM3Module io_module)
{
    M3_ਨਤੀਜਾ ਨਤੀਜਾ = m3Err_none;

    ਜੇ (M3_UNLIKELY(io_module->runtime)) {
        ਵਾਪਸ m3Err_moduleAlreadyLinked;
    }

    io_module->runtime = io_runtime;

    // linking first: a memory import has to be resolved before InitMemory would
    // give it pages of its own, and a global import before an initializer reads it
_   (LinkImports(io_runtime, io_module));

_   (Init_ਸਮ੍ਰਿਤੀ(io_runtime, io_module));
_   (InitGlobals(io_module));
    // Spec order: element segments are applied before data segments. It matters
    // when one of them traps - whatever ran before the trap stays done.
_   (Init_ਸਾਰਣੀ_And_Elements(io_module));
_   (Init_ਅੰਕੜੇ_Segments(io_module));

    // Start func might use imported functions, which are not liked here yet,
    // so it will be called before a function call is attempted (in m3_FindFunction)

#ਜੇ_ਪਰਿਭਾਸ਼ਿਤ DEBUG
    Module_Generate_ਨਾਂ(io_module);
#ਜੇ_ਅੰਤ

    io_module->ਅਗਲਾ     = io_runtime->modules;
    io_runtime->modules = io_module;
    ਵਾਪਸ ਨਤੀਜਾ; // ok

_catch:
    // The runtime owns the module either way. Instantiation may already have
    // written this module's functions into a table another module owns, and the
    // spec keeps whatever it managed to do before the trap, so those entries
    // stay callable - retaining the module is what stops them dangling.
    //
    // It goes on the tail of the list rather than the head: it is not a module
    // anyone should find by name, only one whose functions may still be
    // reachable through someone else's table.
    io_module->ਅਗਲਾ = ਨਿਰਾ;

    IM3Module* ਪੂਛ = &io_runtime->modules;
    ਜਦੋਂ (*ਪੂਛ) {
        ਪੂਛ = &(*ਪੂਛ)->ਅਗਲਾ;
    }
    *ਪੂਛ = io_module;

    ਵਾਪਸ ਨਤੀਜਾ;
}

IM3Global m3_ਲੱਭੋ_Global (IM3Module         io_module,
                         ਅਚਲ ਅੱਖਰ* ਅਚਲ i_globalName)
{
    // Search exports
    IM3Global exported = Module_ਲੱਭੋ_Exported_Global(io_module, i_globalName);
    ਜੇ (exported) {
        // a re-exported global is the one that was imported, so reads and
        // writes have to reach the cell that actually holds the value
        ਵਾਪਸ exported->resolved ? exported->resolved : exported;
    }

    // Search imports
    ਲਈ (u32 i = 0; i < io_module->ਅੰਕ_Globals; ++i) {
        IM3Global g = &io_module->globals[i];

        ਜੇ (g->import.moduleUtf8 and g->import.ਖੇਤਰ_Utf8) {
            ਜੇ (strcmp(g->import.ਖੇਤਰ_Utf8, i_globalName) == 0) {
                ਵਾਪਸ g->resolved ? g->resolved : g;
            }
        }
    }
    ਵਾਪਸ ਨਿਰਾ;
}

M3_ਨਤੀਜਾ m3_GetGlobal (IM3Global      i_global,
                       IM3_Tagged_ਮੁੱਲ o_ਮੁੱਲ)
{
    ਜੇ (not i_global) {
        ਵਾਪਸ m3Err_globalLookupFailed;
    }

    ਚੋਣ (i_global->type) {
    ਹਾਲਤ c_m3Type_i32: o_ਮੁੱਲ->ਮੁੱਲ.i32 = i_global->i32_ਮੁੱਲ; ਰੋਕੋ;
    ਹਾਲਤ c_m3Type_i64: o_ਮੁੱਲ->ਮੁੱਲ.i64 = i_global->i64_ਮੁੱਲ; ਰੋਕੋ;
#ਜੇ d_m3HasFloat
    ਹਾਲਤ c_m3Type_f32: o_ਮੁੱਲ->ਮੁੱਲ.f32 = i_global->f32_ਮੁੱਲ; ਰੋਕੋ;
    ਹਾਲਤ c_m3Type_f64: o_ਮੁੱਲ->ਮੁੱਲ.f64 = i_global->f64_ਮੁੱਲ; ਰੋਕੋ;
#ਜੇ_ਅੰਤ
    ਮੂਲ: ਵਾਪਸ m3Err_invalidTypeId;
    }

    o_ਮੁੱਲ->type = (M3_ਮੁੱਲ_Type)(i_global->type);
    ਵਾਪਸ m3Err_none;
}

M3_ਨਤੀਜਾ m3_ਸੈੱਟ_Global (IM3Global            i_global,
                       ਅਚਲ IM3_Tagged_ਮੁੱਲ i_ਮੁੱਲ)
{
    ਜੇ (not i_global) {
        ਵਾਪਸ m3Err_globalLookupFailed;
    }
    ਜੇ (not i_global->isMutable) {
        ਵਾਪਸ m3Err_globalNotMutable;
    }
    ਜੇ (i_global->type != i_ਮੁੱਲ->type) {
        ਵਾਪਸ m3Err_globalTypeMismatch;
    }

    ਚੋਣ (i_ਮੁੱਲ->type) {
    ਹਾਲਤ c_m3Type_i32: i_global->i32_ਮੁੱਲ = i_ਮੁੱਲ->ਮੁੱਲ.i32; ਰੋਕੋ;
    ਹਾਲਤ c_m3Type_i64: i_global->i64_ਮੁੱਲ = i_ਮੁੱਲ->ਮੁੱਲ.i64; ਰੋਕੋ;
#ਜੇ d_m3HasFloat
    ਹਾਲਤ c_m3Type_f32: i_global->f32_ਮੁੱਲ = i_ਮੁੱਲ->ਮੁੱਲ.f32; ਰੋਕੋ;
    ਹਾਲਤ c_m3Type_f64: i_global->f64_ਮੁੱਲ = i_ਮੁੱਲ->ਮੁੱਲ.f64; ਰੋਕੋ;
#ਜੇ_ਅੰਤ
    ਮੂਲ: ਵਾਪਸ m3Err_invalidTypeId;
    }

    ਵਾਪਸ m3Err_none;
}

M3_ਮੁੱਲ_Type m3_GetGlobalType (IM3Global i_global)
{
    ਵਾਪਸ (i_global) ? (M3_ਮੁੱਲ_Type)(i_global->type) : c_m3Type_none;
}


ਖਾਲੀ* v_ਲੱਭੋ_Function (IM3Module i_module, ਖਾਲੀ* i_info)
{
    ਅਚਲ ਅੱਖਰ* ਅਚਲ i_name = (ਅਚਲ ਅੱਖਰ*)i_info;

    // Prefer exported functions
    IM3Function exported = Module_ਲੱਭੋ_Exported_Function(i_module, i_name);
    ਜੇ (exported) {
        ਵਾਪਸ exported;
    }

    // Search internal functions
    ਲਈ (u32 i = 0; i < i_module->ਅੰਕ_Functions; ++i) {
        IM3Function f = &i_module->functions[i];

        bool isImported = f->import.moduleUtf8 or f->import.ਖੇਤਰ_Utf8;

        ਜੇ (isImported) {
            ਜਾਰੀ;
        }

        ਲਈ (ਪੂਰਨ j = 0; j < f->ਅੰਕ_ਨਾਂ; j++) {
            ਜੇ (f->ਨਾਂ[j] and strcmp(f->ਨਾਂ[j], i_name) == 0) {
                ਵਾਪਸ f;
            }
        }
    }

    ਵਾਪਸ ਨਿਰਾ;
}


// Shared tail of the two lookups: a function is only usable once it has code.
ਸਥਿਰ
M3_ਨਤੀਜਾ Prepare_ਮਿਲਿਆ_Function (IM3Function* o_function, IM3Function i_function)
{
    M3_ਨਤੀਜਾ ਨਤੀਜਾ = m3Err_none;

    ਜੇ (not i_function->compiled) {
_       (CompileFunction(i_function))
    }

    _catch:
    *o_function = ਨਤੀਜਾ ? ਨਿਰਾ : i_function;

    ਵਾਪਸ ਨਤੀਜਾ;
}


// Searches every module in the runtime, most recently loaded first. That is a
// guess once more than one module is loaded and two of them export the same
// name - m3_FindFunctionIn says which module is meant.
M3_ਨਤੀਜਾ m3_ਲੱਭੋ_Function (IM3Function* o_function, IM3Runtime i_runtime, ਅਚਲ ਅੱਖਰ* ਅਚਲ i_functionName)
{
                                                                d_m3Assert (o_function and i_runtime and i_functionName);
    IM3Function function = ਨਿਰਾ;

    ਜੇ (not i_runtime->modules) {
        *o_function = ਨਿਰਾ;
        ਵਾਪਸ "no modules loaded";
    }

    function = (IM3Function)ForEachModule(i_runtime, v_ਲੱਭੋ_Function, (ਖਾਲੀ*)i_functionName);

    ਜੇ (not function) {
        *o_function = ਨਿਰਾ;
        ਵਾਪਸ ErrorModule(m3Err_functionLookupFailed, i_runtime->modules, "'%s'", i_functionName);
    }

    ਵਾਪਸ Prepare_ਮਿਲਿਆ_Function(o_function, function);
}


IM3Module m3_ਲੱਭੋ_Module (IM3Runtime i_runtime, ਅਚਲ ਅੱਖਰ* ਅਚਲ i_moduleName)
{
    ਜੇ (not i_runtime or not i_moduleName) {
        ਵਾਪਸ ਨਿਰਾ;
    }

    // the list is newest-first, so a name registered twice names the newer one
    ਲਈ (IM3Module m = i_runtime->modules; m; m = m->ਅਗਲਾ) {
        ਜੇ (m->name and strcmp(m->name, i_moduleName) == 0) {
            ਵਾਪਸ m;
        }
    }

    ਵਾਪਸ ਨਿਰਾ;
}


// Searches one module's exports, which is what naming a module means.
M3_ਨਤੀਜਾ m3_ਲੱਭੋ_Function_ਵਿੱਚ (IM3Function* o_function, IM3Module i_module, ਅਚਲ ਅੱਖਰ* ਅਚਲ i_functionName)
{
                                                                d_m3Assert (o_function and i_functionName);
    ਜੇ (not i_module) {
        *o_function = ਨਿਰਾ;
        ਵਾਪਸ m3Err_functionLookupFailed;      // ErrorModule would deref it
    }

    IM3Function function = (IM3Function)v_ਲੱਭੋ_Function(i_module, (ਖਾਲੀ*)i_functionName);

    ਜੇ (not function) {
        *o_function = ਨਿਰਾ;
        ਵਾਪਸ ErrorModule(m3Err_functionLookupFailed, i_module, "'%s'", i_functionName);
    }

    ਵਾਪਸ Prepare_ਮਿਲਿਆ_Function(o_function, function);
}


M3_ਨਤੀਜਾ m3_Get_ਸਾਰਣੀ_Function (IM3Function* o_function, IM3Module i_module, uint32_t i_ਸੂਚਕਾਂਕ)
{
_try {
    M3_ਸਾਰਣੀ*    ਸਾਰਣੀ_2;
    IM3Function function;

    _throwif("no table", i_module->ਅੰਕ_Tables == 0);

    ਸਾਰਣੀ_2 = i_module->tables[0];
    _throwif("function index out of range", i_ਸੂਚਕਾਂਕ >= ਸਾਰਣੀ_2->ਮਾਪ);

    function = (IM3Function)ਸਾਰਣੀ_2->elements[i_ਸੂਚਕਾਂਕ];

    ਜੇ (function) {
        ਜੇ (not function->compiled) {
_           (CompileFunction(function))
        }
    }

    *o_function = function;
} _catch:
    ਵਾਪਸ ਨਤੀਜਾ;
}


ਸਥਿਰ
M3_ਨਤੀਜਾ check_ਸ਼ੁਰੂ_Function (IM3Module i_module)
{
    M3_ਨਤੀਜਾ ਨਤੀਜਾ = m3Err_none;                               d_m3Assert(i_module);

    // Check if start function needs to be called
    ਜੇ (i_module->ਸ਼ੁਰੂ_Function >= 0) {
        ਨਤੀਜਾ = m3_ਲੜੀਵਾਰ_ਸ਼ੁਰੂ(i_module);
    }

    ਵਾਪਸ ਨਤੀਜਾ;
}

uint32_t m3_Get_Arg_ਗਿਣਤੀ (IM3Function i_function)
{
    ਜੇ (i_function) {
        IM3FuncType ft = i_function->funcType;
        ਜੇ (ft) {
            ਵਾਪਸ ft->ਅੰਕ_ਦਲੀਲਾਂ;
        }
    }
    ਵਾਪਸ 0;
}

uint32_t m3_Get_Ret_ਗਿਣਤੀ (IM3Function i_function)
{
    ਜੇ (i_function) {
        IM3FuncType ft = i_function->funcType;
        ਜੇ (ft) {
            ਵਾਪਸ ft->ਅੰਕ_Rets;
        }
    }
    ਵਾਪਸ 0;
}


M3_ਮੁੱਲ_Type m3_GetArgType (IM3Function i_function, uint32_t index)
{
    ਜੇ (i_function) {
        IM3FuncType ft = i_function->funcType;
        ਜੇ (ft and index < ft->ਅੰਕ_ਦਲੀਲਾਂ) {
            ਵਾਪਸ (M3_ਮੁੱਲ_Type)ਅਧਾਰ_Type_ਦਾ(d_FuncArgType(ft, index));
        }
    }
    ਵਾਪਸ c_m3Type_none;
}

M3_ਮੁੱਲ_Type m3_GetRetType (IM3Function i_function, uint32_t index)
{
    ਜੇ (i_function) {
        IM3FuncType ft = i_function->funcType;
        ਜੇ (ft and index < ft->ਅੰਕ_Rets) {
            ਵਾਪਸ (M3_ਮੁੱਲ_Type)ਅਧਾਰ_Type_ਦਾ(d_FuncRetType(ft, index));
        }
    }
    ਵਾਪਸ c_m3Type_none;
}


// The call APIs marshal their arguments into the runtime stack before any
// compiled code runs, so op_Entry's overflow check comes too late to cover those
// writes - and for a host function or an import linked to another module there
// is no op_Entry of our own to reach at all. Bound the area about to be written
// here instead: results and arguments each take a u64 at this boundary, however
// wide the slot is inside. Every caller goes through this, so none of them can
// be the one that forgets.
ਸਥਿਰ
M3_ਨਤੀਜਾ Get_ਢੇਰ_ਸੂਚਕ_For_ਦਲੀਲਾਂ (IM3Function i_function, u8** o_ਢੇਰ)
{
    IM3Runtime  runtime = i_function->module->runtime;
    IM3FuncType ftype   = i_function->funcType;

    // the stack a paused invocation holds is not available to this one
    size_t ਵਰਤਿਆ      = (size_t)((u8*)runtime->ਢੇਰ - (u8*)runtime->origin_ਢੇਰ);
    size_t needed    = ((size_t)ftype->ਅੰਕ_Rets + ftype->ਅੰਕ_ਦਲੀਲਾਂ) * ਆਕਾਰ(u64);
    size_t ਉਪਲਬਧ = (size_t)runtime->ਅੰਕ_ਢੇਰ_Slots * ਆਕਾਰ(m3slot_t) - ਵਰਤਿਆ;

    ਜੇ (needed > ਉਪਲਬਧ) {
        ਵਾਪਸ m3_Err_trap_ਢੇਰ_ਉਛਾਲ;
    }

    u64* ਢੇਰ = (u64*)runtime->ਢੇਰ;

    ਢੇਰ += ftype->ਅੰਕ_Rets;

    *o_ਢੇਰ = (u8*)ਢੇਰ;

    ਵਾਪਸ m3Err_none;
}


M3_ਨਤੀਜਾ m3_CallV (IM3Function i_function, ...)
{
    va_list ap;
    va_start(ap, i_function);
    M3_ਨਤੀਜਾ r = m3_CallVL(i_function, ap);
    va_end(ap);
    ਵਾਪਸ r;
}

ਸਥਿਰ
ਖਾਲੀ Report_Native_ਢੇਰ_Usage ()
{
#ਜੇ d_m3_Log_Native_ਢੇਰ
    ਪੂਰਨ ਢੇਰ_ਵਰਤਿਆ = m3_ਢੇਰ_Get_Max();
    fprintf(stderr, "Native stack used: %d\n", ਢੇਰ_ਵਰਤਿਆ);
#ਜੇ_ਅੰਤ
}


M3_ਨਤੀਜਾ m3_CallVL (IM3Function i_function, va_list i_ਦਲੀਲਾਂ)
{
    IM3Runtime  runtime = i_function->module->runtime;
    IM3FuncType ftype   = i_function->funcType;
    M3_ਨਤੀਜਾ    ਨਤੀਜਾ  = m3Err_none;
    u8*         s       = ਨਿਰਾ;

    ਜੇ (!i_function->compiled) {
        ਵਾਪਸ m3_Err_missing_Compiled_ਕੋਡ;
    }

#ਜੇ d_m3_ਰਿਕਾਰਡ_Backtraces
    ClearBacktrace(runtime);
#ਜੇ_ਅੰਤ

    m3_ਢੇਰ_Check_Init();

_   (check_ਸ਼ੁਰੂ_Function(i_function->module))

_   (Get_ਢੇਰ_ਸੂਚਕ_For_ਦਲੀਲਾਂ(i_function, &s))

    ਲਈ (u32 i = 0; i < ftype->ਅੰਕ_ਦਲੀਲਾਂ; ++i) {
        ਚੋਣ (d_FuncArgType(ftype, i)) {
        ਹਾਲਤ c_m3Type_i32:
            *(i32*)(s) = va_arg(i_ਦਲੀਲਾਂ, i32);
            s += 8;
            ਰੋਕੋ;
        ਹਾਲਤ c_m3Type_i64:
            *(i64*)(s) = va_arg(i_ਦਲੀਲਾਂ, i64);
            s += 8;
            ਰੋਕੋ;
        ਹਾਲਤ c_m3Type_funcref:
        ਹਾਲਤ c_m3Type_externref:
        ਹਾਲਤ c_m3Type_exnref:
            *(uintptr_t*)(s) = va_arg(i_ਦਲੀਲਾਂ, uintptr_t);
            s += 8;
            ਰੋਕੋ;
#ਜੇ d_m3HasFloat
        ਹਾਲਤ c_m3Type_f32:
            *(f32*)(s) = (f32)va_arg(i_ਦਲੀਲਾਂ, f64);
            s += 8;
            ਰੋਕੋ; // f32 is passed as f64
        ਹਾਲਤ c_m3Type_f64:
            *(f64*)(s) = va_arg(i_ਦਲੀਲਾਂ, f64);
            s += 8;
            ਰੋਕੋ;
#ਜੇ_ਅੰਤ
        ਮੂਲ: ਵਾਪਸ "unknown argument type";
        }
    }

    ਨਤੀਜਾ = ਲੜੀਵਾਰ_ਕੋਡ_Checked(runtime, i_function);
    Report_Native_ਢੇਰ_Usage();

    runtime->ਆਖਰੀ_Called = ਨਤੀਜਾ ? ਨਿਰਾ : i_function;

    _catch: ਵਾਪਸ ਨਤੀਜਾ;
}

M3_ਨਤੀਜਾ m3_Call (IM3Function i_function, uint32_t i_argc, ਅਚਲ ਖਾਲੀ* i_argptrs[])
{
    IM3Runtime  runtime = i_function->module->runtime;
    IM3FuncType ftype   = i_function->funcType;
    M3_ਨਤੀਜਾ    ਨਤੀਜਾ  = m3Err_none;
    u8*         s       = ਨਿਰਾ;

    ਜੇ (i_argc != ftype->ਅੰਕ_ਦਲੀਲਾਂ) {
        ਵਾਪਸ m3_Err_ਦਲੀਲ_ਗਿਣਤੀ_Mismatch;
    }
    ਜੇ (!i_function->compiled) {
        ਵਾਪਸ m3_Err_missing_Compiled_ਕੋਡ;
    }

#ਜੇ d_m3_ਰਿਕਾਰਡ_Backtraces
    ClearBacktrace(runtime);
#ਜੇ_ਅੰਤ

    m3_ਢੇਰ_Check_Init();

_   (check_ਸ਼ੁਰੂ_Function(i_function->module))

_   (Get_ਢੇਰ_ਸੂਚਕ_For_ਦਲੀਲਾਂ(i_function, &s))

    ਲਈ (u32 i = 0; i < ftype->ਅੰਕ_ਦਲੀਲਾਂ; ++i) {
        ਚੋਣ (d_FuncArgType(ftype, i)) {
        ਹਾਲਤ c_m3Type_i32:
            *(i32*)(s) = *(i32*)i_argptrs[i];
            s += 8;
            ਰੋਕੋ;
        ਹਾਲਤ c_m3Type_i64:
            *(i64*)(s) = *(i64*)i_argptrs[i];
            s += 8;
            ਰੋਕੋ;
        ਹਾਲਤ c_m3Type_funcref:
        ਹਾਲਤ c_m3Type_externref:
        ਹਾਲਤ c_m3Type_exnref:
            *(uintptr_t*)(s) = *(uintptr_t*)i_argptrs[i];
            s += 8;
            ਰੋਕੋ;
#ਜੇ d_m3HasFloat
        ਹਾਲਤ c_m3Type_f32:
            *(f32*)(s) = *(f32*)i_argptrs[i];
            s += 8;
            ਰੋਕੋ;
        ਹਾਲਤ c_m3Type_f64:
            *(f64*)(s) = *(f64*)i_argptrs[i];
            s += 8;
            ਰੋਕੋ;
#ਜੇ_ਅੰਤ
        ਮੂਲ: ਵਾਪਸ "unknown argument type";
        }
    }

    ਨਤੀਜਾ = ਲੜੀਵਾਰ_ਕੋਡ_Checked(runtime, i_function);

    Report_Native_ਢੇਰ_Usage();

    runtime->ਆਖਰੀ_Called = ਨਤੀਜਾ ? ਨਿਰਾ : i_function;

    _catch: ਵਾਪਸ ਨਤੀਜਾ;
}

// Argument parsing for m3_CallArgv. Strict on purpose: the whole string has to
// be consumed, so "12abc" is rejected rather than read as 12, and an empty or
// unparsable argument is an error rather than the zero that strtoul with a
// NULL end pointer used to hand back. See wasm3/wasm3#367.
ਸਥਿਰ
M3_ਨਤੀਜਾ ParseArgInteger (ccstr_t i_arg, u32 i_ਅੰਕ_ਬਿੱਟ, u64* o_ਮੁੱਲ)
{
    ਜੇ (not i_arg or not *i_arg) {
        ਵਾਪਸ "empty argument";
    }

    // strtoull would skip leading space, but trailing space is rejected below;
    // accepting one and not the other would just be confusing
    ਜੇ (ਖਾਲੀ_ਥਾਂ_ਹੈ((ਬਿਨਾਨਿਸ਼ਾਨੀ ਅੱਖਰ)*i_arg)) {
        ਵਾਪਸ "argument is not a number";
    }

    ਅੱਖਰ* ਸਿਰਾ = ਨਿਰਾ;
    u64   ਮੁੱਲ;

    errno = 0;

    // an argument may be spelled signed or unsigned: -1 and 4294967295 name the
    // same i32
    ਜੇ (*i_arg == '-') {
        i64 signed_ਮੁੱਲ = strtoll(i_arg, &ਸਿਰਾ, 10);

        ਜੇ (i_ਅੰਕ_ਬਿੱਟ == 32 and (signed_ਮੁੱਲ < INT32_MIN or signed_ਮੁੱਲ > INT32_MAX)) {
            ਵਾਪਸ "argument out of range";
        }

        ਮੁੱਲ = (u64)signed_ਮੁੱਲ;
    } ਨਹੀਂਤਾਂ {
        ਮੁੱਲ = strtoull(i_arg, &ਸਿਰਾ, 10);

        ਜੇ (i_ਅੰਕ_ਬਿੱਟ == 32 and ਮੁੱਲ > UINT32_MAX) {
            ਵਾਪਸ "argument out of range";
        }
    }

    ਜੇ (errno == ERANGE) {
        ਵਾਪਸ "argument out of range";
    }

    ਜੇ (ਸਿਰਾ == i_arg or *ਸਿਰਾ) {
        ਵਾਪਸ "argument is not a number";
    }

    *o_ਮੁੱਲ = ਮੁੱਲ;

    ਵਾਪਸ m3Err_none;
}


#ਜੇ d_m3HasFloat
ਸਥਿਰ
M3_ਨਤੀਜਾ ParseArgFloat (ccstr_t i_arg, f64* o_ਮੁੱਲ)
{
    ਜੇ (not i_arg or not *i_arg) {
        ਵਾਪਸ "empty argument";
    }

    ਜੇ (ਖਾਲੀ_ਥਾਂ_ਹੈ((ਬਿਨਾਨਿਸ਼ਾਨੀ ਅੱਖਰ)*i_arg)) {
        ਵਾਪਸ "argument is not a number";
    }

    ਅੱਖਰ* ਸਿਰਾ = ਨਿਰਾ;

    errno = 0;

    f64 ਮੁੱਲ = strtod(i_arg, &ਸਿਰਾ);

    ਜੇ (ਸਿਰਾ == i_arg or *ਸਿਰਾ) {
        ਵਾਪਸ "argument is not a number";
    }

    // strtod reports underflow through ERANGE as well, and a denormal result is
    // perfectly usable, so only an overflow to infinity is out of range
    ਜੇ (errno == ERANGE and (ਮੁੱਲ > DBL_MAX or ਮੁੱਲ < -DBL_MAX)) {
        ਵਾਪਸ "argument out of range";
    }

    *o_ਮੁੱਲ = ਮੁੱਲ;

    ਵਾਪਸ m3Err_none;
}
#ਜੇ_ਅੰਤ


// A reference argument is either the null reference or a host handle written as
// an integer.
ਸਥਿਰ
M3_ਨਤੀਜਾ ParseArgReference (ccstr_t i_arg, u64* o_ਮੁੱਲ)
{
    ਜੇ (i_arg and strcmp(i_arg, "null") == 0) {
        *o_ਮੁੱਲ = 0;
        ਵਾਪਸ m3Err_none;
    }

    ਵਾਪਸ ParseArgInteger(i_arg, 64, o_ਮੁੱਲ);
}


M3_ਨਤੀਜਾ m3_CallArgv (IM3Function i_function, uint32_t i_argc, ਅਚਲ ਅੱਖਰ* i_argv[])
{
    IM3FuncType ftype   = i_function->funcType;
    IM3Runtime  runtime = i_function->module->runtime;
    M3_ਨਤੀਜਾ    ਨਤੀਜਾ  = m3Err_none;
    u8*         s       = ਨਿਰਾ;

    ਜੇ (i_argc != ftype->ਅੰਕ_ਦਲੀਲਾਂ) {
        ਵਾਪਸ m3_Err_ਦਲੀਲ_ਗਿਣਤੀ_Mismatch;
    }
    ਜੇ (!i_function->compiled) {
        ਵਾਪਸ m3_Err_missing_Compiled_ਕੋਡ;
    }

#ਜੇ d_m3_ਰਿਕਾਰਡ_Backtraces
    ClearBacktrace(runtime);
#ਜੇ_ਅੰਤ

    m3_ਢੇਰ_Check_Init();

_   (check_ਸ਼ੁਰੂ_Function(i_function->module))

_   (Get_ਢੇਰ_ਸੂਚਕ_For_ਦਲੀਲਾਂ(i_function, &s))

    ਲਈ (u32 i = 0; i < ftype->ਅੰਕ_ਦਲੀਲਾਂ; ++i) {
        u64 ਮੁੱਲ = 0;
#ਜੇ d_m3HasFloat
        f64 fvalue = 0;
#ਜੇ_ਅੰਤ
        ਚੋਣ (d_FuncArgType(ftype, i)) {
        ਹਾਲਤ c_m3Type_i32:
_           (ParseArgInteger(i_argv[i], 32, &ਮੁੱਲ)) * (i32*)(s) = (i32)ਮੁੱਲ;
            s += 8;
            ਰੋਕੋ;
        ਹਾਲਤ c_m3Type_i64:
_           (ParseArgInteger(i_argv[i], 64, &ਮੁੱਲ)) * (i64*)(s) = (i64)ਮੁੱਲ;
            s += 8;
            ਰੋਕੋ;
        ਹਾਲਤ c_m3Type_funcref:
        ਹਾਲਤ c_m3Type_externref:
        ਹਾਲਤ c_m3Type_exnref:
_           (ParseArgReference(i_argv[i], &ਮੁੱਲ)) * (uintptr_t*)(s) = (uintptr_t)ਮੁੱਲ;
            s += 8;
            ਰੋਕੋ;
#ਜੇ d_m3HasFloat
            // strtof would be less portable
        ਹਾਲਤ c_m3Type_f32:
_           (ParseArgFloat(i_argv[i], &fvalue)) * (f32*)(s) = (f32)fvalue;
            s += 8;
            ਰੋਕੋ;
        ਹਾਲਤ c_m3Type_f64:
_           (ParseArgFloat(i_argv[i], &fvalue)) * (f64*)(s) = fvalue;
            s += 8;
            ਰੋਕੋ;
#ਜੇ_ਅੰਤ
        ਮੂਲ: _throw("unknown argument type");
        }
    }

    ਨਤੀਜਾ = ਲੜੀਵਾਰ_ਕੋਡ_Checked(runtime, i_function);

    Report_Native_ਢੇਰ_Usage();

    runtime->ਆਖਰੀ_Called = ਨਤੀਜਾ ? ਨਿਰਾ : i_function;

    _catch: ਵਾਪਸ ਨਤੀਜਾ;
}


//u8 * AlignStackPointerTo64Bits (const u8 * i_stack)
//{
//    uintptr_t ptr = (uintptr_t) i_stack;
//    return (u8 *) ((ptr + 7) & ~7);
//}


M3_ਨਤੀਜਾ m3_GetResults (IM3Function i_function, uint32_t i_retc, ਅਚਲ ਖਾਲੀ* o_retptrs[])
{
    IM3FuncType ftype   = i_function->funcType;
    IM3Runtime  runtime = i_function->module->runtime;

    ਜੇ (i_retc != ftype->ਅੰਕ_Rets) {
        ਵਾਪਸ m3_Err_ਦਲੀਲ_ਗਿਣਤੀ_Mismatch;
    }
    ਜੇ (i_function != runtime->ਆਖਰੀ_Called) {
        ਵਾਪਸ "function not called";
    }

    u8* s = (u8*)runtime->ਢੇਰ;

    ਲਈ (u32 i = 0; i < ftype->ਅੰਕ_Rets; ++i) {
        ਚੋਣ (d_FuncRetType(ftype, i)) {
        ਹਾਲਤ c_m3Type_i32:
            *(i32*)o_retptrs[i] = *(i32*)(s);
            s += 8;
            ਰੋਕੋ;
        ਹਾਲਤ c_m3Type_i64:
            *(i64*)o_retptrs[i] = *(i64*)(s);
            s += 8;
            ਰੋਕੋ;
        ਹਾਲਤ c_m3Type_funcref:
        ਹਾਲਤ c_m3Type_externref:
        ਹਾਲਤ c_m3Type_exnref:
            *(uintptr_t*)o_retptrs[i] = *(uintptr_t*)(s);
            s += 8;
            ਰੋਕੋ;
#ਜੇ d_m3HasFloat
        ਹਾਲਤ c_m3Type_f32:
            *(f32*)o_retptrs[i] = *(f32*)(s);
            s += 8;
            ਰੋਕੋ;
        ਹਾਲਤ c_m3Type_f64:
            *(f64*)o_retptrs[i] = *(f64*)(s);
            s += 8;
            ਰੋਕੋ;
#ਜੇ_ਅੰਤ
        ਮੂਲ: ਵਾਪਸ "unknown return type";
        }
    }
    ਵਾਪਸ m3Err_none;
}

M3_ਨਤੀਜਾ m3_GetResultsV (IM3Function i_function, ...)
{
    va_list ap;
    va_start(ap, i_function);
    M3_ਨਤੀਜਾ r = m3_GetResultsVL(i_function, ap);
    va_end(ap);
    ਵਾਪਸ r;
}

M3_ਨਤੀਜਾ m3_GetResultsVL (IM3Function i_function, va_list o_rets)
{
    IM3Runtime  runtime = i_function->module->runtime;
    IM3FuncType ftype   = i_function->funcType;

    ਜੇ (i_function != runtime->ਆਖਰੀ_Called) {
        ਵਾਪਸ "function not called";
    }

    u8* s = (u8*)runtime->ਢੇਰ;
    ਲਈ (u32 i = 0; i < ftype->ਅੰਕ_Rets; ++i) {
        ਚੋਣ (d_FuncRetType(ftype, i)) {
        ਹਾਲਤ c_m3Type_i32:
            *va_arg(o_rets, i32*) = *(i32*)(s);
            s += 8;
            ਰੋਕੋ;
        ਹਾਲਤ c_m3Type_i64:
            *va_arg(o_rets, i64*) = *(i64*)(s);
            s += 8;
            ਰੋਕੋ;
        ਹਾਲਤ c_m3Type_funcref:
        ਹਾਲਤ c_m3Type_externref:
        ਹਾਲਤ c_m3Type_exnref:
            *va_arg(o_rets, uintptr_t*) = *(uintptr_t*)(s);
            s += 8;
            ਰੋਕੋ;
#ਜੇ d_m3HasFloat
        ਹਾਲਤ c_m3Type_f32:
            *va_arg(o_rets, f32*) = *(f32*)(s);
            s += 8;
            ਰੋਕੋ;
        ਹਾਲਤ c_m3Type_f64:
            *va_arg(o_rets, f64*) = *(f64*)(s);
            s += 8;
            ਰੋਕੋ;
#ਜੇ_ਅੰਤ
        ਮੂਲ: ਵਾਪਸ "unknown argument type";
        }
    }
    ਵਾਪਸ m3Err_none;
}

ਖਾਲੀ Release_ਕੋਡ_ਪੰਨਾ_No_Track (IM3Runtime i_runtime, IM3_ਕੋਡ_ਪੰਨਾ i_ਕੋਡ_ਪੰਨਾ)
{
    ਜੇ (i_ਕੋਡ_ਪੰਨਾ) {
        IM3_ਕੋਡ_ਪੰਨਾ* list;

        bool ਪੰਨਾ_ਭਰਿਆ = (ਅੰਕ_Free_ਸਤਰਾਂ(i_ਕੋਡ_ਪੰਨਾ) < d_m3_ਕੋਡ_ਪੰਨਾ_Free_ਸਤਰਾਂ_ਹੱਦ);
        ਜੇ (ਪੰਨਾ_ਭਰਿਆ) {
            list = &i_runtime->ਪੰਨੇ_ਭਰਿਆ;
        } ਨਹੀਂਤਾਂ {
            list = &i_runtime->ਪੰਨੇ_Open;
        }

        ਧੱਕੋ_ਕੋਡ_ਪੰਨਾ(list, i_ਕੋਡ_ਪੰਨਾ);                         m3log (emit, "release page: %d to queue: '%s'", i_ਕੋਡ_ਪੰਨਾ->info.sequence, ਪੰਨਾ_ਭਰਿਆ ? "full" : "open")
    }
}


IM3_ਕੋਡ_ਪੰਨਾ Acquire_ਕੋਡ_ਪੰਨਾ_With_ਸਮਰੱਥਾ (IM3Runtime i_runtime, u32 i_min_ਸਤਰ_ਗਿਣਤੀ)
{
    IM3_ਕੋਡ_ਪੰਨਾ ਪੰਨਾ = Remove_ਕੋਡ_ਪੰਨਾ_ਦਾ_ਸਮਰੱਥਾ(&i_runtime->ਪੰਨੇ_Open, i_min_ਸਤਰ_ਗਿਣਤੀ);

    ਜੇ (not ਪੰਨਾ) {
        ਪੰਨਾ = Environment_Acquire_ਕੋਡ_ਪੰਨਾ(i_runtime->environment, i_min_ਸਤਰ_ਗਿਣਤੀ);

        ਜੇ (not ਪੰਨਾ) {
            ਪੰਨਾ = New_ਕੋਡ_ਪੰਨਾ(i_runtime, i_min_ਸਤਰ_ਗਿਣਤੀ);
        }

        ਜੇ (ਪੰਨਾ) {
            i_runtime->ਅੰਕ_ਕੋਡ_ਪੰਨੇ++;
        }
    }

    ਜੇ (ਪੰਨਾ) {                                                  m3log (emit, "acquire page: %d", ਪੰਨਾ->info.sequence);
        i_runtime->ਅੰਕ_ਸਰਗਰਮ_ਕੋਡ_ਪੰਨੇ++;
    }

    ਵਾਪਸ ਪੰਨਾ;
}


IM3_ਕੋਡ_ਪੰਨਾ Acquire_ਕੋਡ_ਪੰਨਾ (IM3Runtime i_runtime)
{
    ਵਾਪਸ Acquire_ਕੋਡ_ਪੰਨਾ_With_ਸਮਰੱਥਾ(i_runtime, d_m3_ਕੋਡ_ਪੰਨਾ_Free_ਸਤਰਾਂ_ਹੱਦ);
}


ਖਾਲੀ Release_ਕੋਡ_ਪੰਨਾ (IM3Runtime i_runtime, IM3_ਕੋਡ_ਪੰਨਾ i_ਕੋਡ_ਪੰਨਾ)
{
    ਜੇ (i_ਕੋਡ_ਪੰਨਾ) {
        Release_ਕੋਡ_ਪੰਨਾ_No_Track(i_runtime, i_ਕੋਡ_ਪੰਨਾ);
        i_runtime->ਅੰਕ_ਸਰਗਰਮ_ਕੋਡ_ਪੰਨੇ--;

#ਜੇ defined(DEBUG)
        u32 ਅੰਕ_Open = ਗਿਣਤੀ_ਕੋਡ_ਪੰਨੇ(i_runtime->ਪੰਨੇ_Open);
        u32 ਅੰਕ_ਭਰਿਆ = ਗਿਣਤੀ_ਕੋਡ_ਪੰਨੇ(i_runtime->ਪੰਨੇ_ਭਰਿਆ);

        m3log(runtime, "runtime: %p; open-pages: %d; full-pages: %d; active: %d; total: %d", i_runtime, ਅੰਕ_Open, ਅੰਕ_ਭਰਿਆ, i_runtime->ਅੰਕ_ਸਰਗਰਮ_ਕੋਡ_ਪੰਨੇ, i_runtime->ਅੰਕ_ਕੋਡ_ਪੰਨੇ);

        d_m3Assert(ਅੰਕ_Open + ਅੰਕ_ਭਰਿਆ + i_runtime->ਅੰਕ_ਸਰਗਰਮ_ਕੋਡ_ਪੰਨੇ == i_runtime->ਅੰਕ_ਕੋਡ_ਪੰਨੇ);

#  ਜੇ d_m3_Log_ਕੋਡ_ਪੰਨੇ
        dump_ਕੋਡ_ਪੰਨਾ(i_ਕੋਡ_ਪੰਨਾ, /* startPC: */ ਨਿਰਾ);
#  endif
#ਜੇ_ਅੰਤ
    }
}


#ਜੇ d_m3VerboseErrorMessages
M3_ਨਤੀਜਾ m3Error (M3_ਨਤੀਜਾ i_ਨਤੀਜਾ, IM3Runtime i_runtime, IM3Module i_module, IM3Function i_function,
                  ਅਚਲ ਅੱਖਰ* ਅਚਲ i_file, u32 i_ਸਤਰ_ਅੰਕ, ਅਚਲ ਅੱਖਰ* ਅਚਲ i_error_ਸੁਨੇਹਾ, ...)
{
    ਜੇ (i_runtime) {
        i_runtime->error         = (M3ErrorInfo){ .ਨਤੀਜਾ = i_ਨਤੀਜਾ, .runtime = i_runtime, .module = i_module, .function = i_function, .file = i_file, .ਸਤਰ_2 = i_ਸਤਰ_ਅੰਕ };
        i_runtime->error.ਸੁਨੇਹਾ = i_runtime->error_ਸੁਨੇਹਾ;

        va_list ਦਲੀਲਾਂ;
        va_start(ਦਲੀਲਾਂ, i_error_ਸੁਨੇਹਾ);
        vsnprintf(i_runtime->error_ਸੁਨੇਹਾ, ਆਕਾਰ(i_runtime->error_ਸੁਨੇਹਾ), i_error_ਸੁਨੇਹਾ, ਦਲੀਲਾਂ);
        va_end(ਦਲੀਲਾਂ);
    }

    ਵਾਪਸ i_ਨਤੀਜਾ;
}
#ਜੇ_ਅੰਤ


ਖਾਲੀ m3_GetErrorInfo (IM3Runtime i_runtime, M3ErrorInfo* o_info)
{
    ਜੇ (i_runtime) {
        *o_info = i_runtime->error;
        m3_ResetErrorInfo(i_runtime);
    }
}


ਖਾਲੀ m3_ResetErrorInfo (IM3Runtime i_runtime)
{
    ਜੇ (i_runtime) {
        M3_INIT(i_runtime->error);
        i_runtime->error.ਸੁਨੇਹਾ = "";
    }
}

uint8_t* m3_Get_ਸਮ੍ਰਿਤੀ (IM3Module i_module, size_t* o_ਸਮ੍ਰਿਤੀ_ਮਾਪ_ਵਿੱਚ_ਬਾਈਟਾਂ, uint32_t i_ਸਮ੍ਰਿਤੀ_ਸੂਚਕਾਂਕ)
{
    uint8_t* ਸਮ੍ਰਿਤੀ = ਨਿਰਾ;
    size_t   ਮਾਪ   = 0;

    ਜੇ (i_module and i_ਸਮ੍ਰਿਤੀ_ਸੂਚਕਾਂਕ < i_module->ਅੰਕ_Memories) {
        IM3_ਸਮ੍ਰਿਤੀ mem = i_module->memories[i_ਸਮ੍ਰਿਤੀ_ਸੂਚਕਾਂਕ];

        ਜੇ (mem->mallocated) {
            ਮਾਪ = mem->mallocated->ਲੰਮਾਈ;

            ਜੇ (ਮਾਪ) {
                ਸਮ੍ਰਿਤੀ = m3_Mem_ਅੰਕੜੇ(mem->mallocated);
            }
        }
    }

    ਜੇ (o_ਸਮ੍ਰਿਤੀ_ਮਾਪ_ਵਿੱਚ_ਬਾਈਟਾਂ) {
        *o_ਸਮ੍ਰਿਤੀ_ਮਾਪ_ਵਿੱਚ_ਬਾਈਟਾਂ = ਮਾਪ;
    }

    ਵਾਪਸ ਸਮ੍ਰਿਤੀ;
}


size_t m3_Get_ਸਮ੍ਰਿਤੀ_ਮਾਪ (IM3Module i_module, uint32_t i_ਸਮ੍ਰਿਤੀ_ਸੂਚਕਾਂਕ)
{
    ਜੇ (not i_module or i_ਸਮ੍ਰਿਤੀ_ਸੂਚਕਾਂਕ >= i_module->ਅੰਕ_Memories) {
        ਵਾਪਸ 0;
    }

    IM3_ਸਮ੍ਰਿਤੀ mem = i_module->memories[i_ਸਮ੍ਰਿਤੀ_ਸੂਚਕਾਂਕ];

    ਵਾਪਸ mem->mallocated ? mem->mallocated->ਲੰਮਾਈ : 0;
}


size_t m3_Get_ਸਮ੍ਰਿਤੀ_ਮਾਪ_At (ਅਚਲ ਖਾਲੀ* i_ਸਮ੍ਰਿਤੀ)
{
    ਜੇ (not i_ਸਮ੍ਰਿਤੀ) {
        ਵਾਪਸ 0;
    }

    // the header sits immediately before the data it describes
    ਅਚਲ M3_ਸਮ੍ਰਿਤੀ_ਸਿਰਲੇਖ* ਸਿਰਲੇਖ = ((ਅਚਲ M3_ਸਮ੍ਰਿਤੀ_ਸਿਰਲੇਖ*)i_ਸਮ੍ਰਿਤੀ) - 1;

    ਵਾਪਸ ਸਿਰਲੇਖ->ਲੰਮਾਈ;
}


M3_ਨਤੀਜਾ m3_ਲੱਭੋ_Exported_ਸਮ੍ਰਿਤੀ (IM3Module i_module, cstr_t i_name, u32* o_ਸਮ੍ਰਿਤੀ_ਸੂਚਕਾਂਕ)
{
    ਜੇ (not i_module or not i_name or not o_ਸਮ੍ਰਿਤੀ_ਸੂਚਕਾਂਕ) {
        ਵਾਪਸ m3_Err_unknown_ਸਮ੍ਰਿਤੀ;
    }

    ਅਚਲ M3Export* e = Module_ਲੱਭੋ_Export(i_module, d_external_ਕਿਸਮ_ਸਮ੍ਰਿਤੀ, i_name);

    ਜੇ (not e) {
        ਵਾਪਸ m3_Err_unknown_ਸਮ੍ਰਿਤੀ;
    }

    *o_ਸਮ੍ਰਿਤੀ_ਸੂਚਕਾਂਕ = e->index;
    ਵਾਪਸ m3Err_none;
}


bool Module_HasLinkedHostImport (IM3Module i_module, cstr_t i_importModule)
{
    ਲਈ (u32 i = 0; i < i_module->ਅੰਕ_Functions; ++i) {
        IM3Function f = &i_module->functions[i];

        // hostMemory is set when a host function is bound to the import, so it
        // is also the mark of one - see CompileRawFunction
        ਜੇ (not f->host_ਸਮ੍ਰਿਤੀ or f->wasm or f->resolved) {
            ਜਾਰੀ;
        }

        ਜੇ (f->import.moduleUtf8 and strcmp(f->import.moduleUtf8, i_importModule) == 0) {
            ਵਾਪਸ true;
        }
    }

    ਵਾਪਸ false;
}


M3_ਨਤੀਜਾ m3_Bind_Import_ਸਮ੍ਰਿਤੀ (IM3Module io_module, cstr_t i_importModule, u32 i_ਸਮ੍ਰਿਤੀ_ਸੂਚਕਾਂਕ)
{
    ਜੇ (not io_module or not i_importModule) {
        ਵਾਪਸ m3Err_moduleNotLinked;
    }

    ਜੇ (i_ਸਮ੍ਰਿਤੀ_ਸੂਚਕਾਂਕ >= io_module->ਅੰਕ_Memories) {
        ਵਾਪਸ m3_Err_unknown_ਸਮ੍ਰਿਤੀ;
    }

    // the memory itself, not the index: linking may have pointed this slot at
    // another module's memory, and growing one reallocates behind it
    IM3_ਸਮ੍ਰਿਤੀ ਸਮ੍ਰਿਤੀ = io_module->memories[i_ਸਮ੍ਰਿਤੀ_ਸੂਚਕਾਂਕ];

    ਅਚਲ bool wildcardModule = (strcmp(i_importModule, "*") == 0);

    ਲਈ (u32 i = 0; i < io_module->ਅੰਕ_Functions; ++i) {
        IM3Function f = &io_module->functions[i];

        // only an import that a host function was bound to has a memory to
        // pin: one with a body of its own reads memory through the interpreter,
        // and one resolved to another module's export runs that module's code
        ਜੇ (not f->host_ਸਮ੍ਰਿਤੀ or f->wasm or f->resolved) {
            ਜਾਰੀ;
        }

        ਜੇ (wildcardModule or (f->import.moduleUtf8 and strcmp(f->import.moduleUtf8, i_importModule) == 0)) {
            f->host_ਸਮ੍ਰਿਤੀ = ਸਮ੍ਰਿਤੀ;
        }
    }

    ਵਾਪਸ m3Err_none;
}


M3BacktraceInfo* m3_GetBacktrace (IM3Runtime i_runtime)
{
#ਜੇ d_m3_ਰਿਕਾਰਡ_Backtraces
    ਵਾਪਸ &i_runtime->backtrace;
#ਨਹੀਂਤਾਂ
    ਵਾਪਸ ਨਿਰਾ;
#ਜੇ_ਅੰਤ
}


//-------------------------------------------------------------------------------------------------------------------------------
//  snapshots & suspendable execution
//-------------------------------------------------------------------------------------------------------------------------------

ਖਾਲੀ m3_ਸੈੱਟ_Suspendable (IM3Runtime io_runtime, bool i_suspendable)
{
    ਜੇ (io_runtime) {
        io_runtime->isSuspendable = i_suspendable;
    }
}

ਖਾਲੀ m3_RequestSuspend (IM3Runtime io_runtime)
{
    ਜੇ (io_runtime) {
        io_runtime->suspendRequested = true;
    }
}

bool m3_IsSuspended (IM3Runtime i_runtime)
{
    ਜੇ (!i_runtime) {
        ਵਾਪਸ false;
    }
#ਜੇ d_m3_Has_ਢੇਰ_Switching
    ਜੇ (i_runtime->ਜੜ੍ਹ_Continuation && i_runtime->ਜੜ੍ਹ_Continuation->state == cont_suspended) {
        ਵਾਪਸ true;
    }
#ਜੇ_ਅੰਤ
    ਵਾਪਸ false;
}

M3_ਨਤੀਜਾ m3_ResumeRuntime (IM3Runtime io_runtime)
{
    ਜੇ (!io_runtime) {
        ਵਾਪਸ m3Err_mallocFailed;
    }
    ਜੇ (!io_runtime->isSuspendable) {
        ਵਾਪਸ m3Err_none;
    }
#ਜੇ d_m3HasSnapshots
    // a restore that failed leaves nothing to resume, and says so every time
    // rather than resuming nothing successfully
    ਜੇ (io_runtime->ਜੜ੍ਹ_Continuation and io_runtime->ਜੜ੍ਹ_Continuation->ਇੰਦਰਾਜ_Function and
        io_runtime->ਜੜ੍ਹ_Continuation->ਇੰਦਰਾਜ_Function->module->isUnusable) {
        ਵਾਪਸ "a snapshot failed to restore into the module, so it cannot run";
    }
#ਜੇ_ਅੰਤ
#ਜੇ d_m3_Has_ਢੇਰ_Switching
    ਜੇ (!io_runtime->ਜੜ੍ਹ_Continuation || io_runtime->ਜੜ੍ਹ_Continuation->state != cont_suspended) {
        ਵਾਪਸ m3Err_none;
    }
    // what finishes returns into the bottom of the stack, where it was entered
    io_runtime->ਢੇਰ = io_runtime->origin_ਢੇਰ;

    // this is the outermost call again, as far as a host function calling back
    // into Wasm from inside it can tell
    io_runtime->entered = io_runtime->ਜੜ੍ਹ_Continuation->ਇੰਦਰਾਜ_Function;
    io_runtime->callNesting++;

    d_m3_ਢੇਰ_ਹੱਦਬੰਦੀ_Enter(io_runtime);
    io_runtime->resumePastCheck = true;
    M3_ਨਤੀਜਾ ਨਤੀਜਾ             = (M3_ਨਤੀਜਾ)ResumeContinuation(io_runtime, io_runtime->ਜੜ੍ਹ_Continuation);
    io_runtime->resumePastCheck = false;
    d_m3_ਢੇਰ_ਹੱਦਬੰਦੀ_Leave(io_runtime);

#  ਜੇ d_m3HasExceptionHandling
    ਜੇ (M3_UNLIKELY(ਨਤੀਜਾ == m3Err_pendingException)) {
        ਨਤੀਜਾ = m3Err_trapUncaughtException;
    }
    ਰਿਕਾਰਡ_Uncaught_Exception(io_runtime, ਨਤੀਜਾ);
#  endif

    io_runtime->callNesting--;

#  ਜੇ d_m3HasExceptionHandling
    ਜੇ (ਨਤੀਜਾ != m3Err_continuationSuspended and io_runtime->callNesting == 0) {
        FreeExceptions(io_runtime);
    }
#  endif

    // the results are where the paused call left room for them, so a finished
    // resume can be asked for them the way the call itself could have been
    io_runtime->ਆਖਰੀ_Called = (ਨਤੀਜਾ == m3Err_none) ? io_runtime->ਜੜ੍ਹ_Continuation->ਇੰਦਰਾਜ_Function : ਨਿਰਾ;

    Runtime_ਰੱਖੋ_Call_ਢੇਰ(io_runtime);
    ਵਾਪਸ ਨਤੀਜਾ;
#ਨਹੀਂਤਾਂ
    ਵਾਪਸ m3Err_none;
#ਜੇ_ਅੰਤ
}
