//
//  m3_env.c
//
//  Created by Steven Massey on 4/19/19.
//  Copyright © 2019 Steven Massey. All rights reserved.
//

#ಅನ್ತರ್ಭಾವ <stdarg.h>
#ಅನ್ತರ್ಭಾವ <limits.h>
#ಅನ್ತರ್ಭಾವ <errno.h>
#ಅನ್ತರ್ಭಾವ <float.h>
#ಅನ್ತರ್ಭಾವ <ctype.h>

#ಅನ್ತರ್ಭಾವ "m3_env.h"
#ಅನ್ತರ್ಭಾವ "m3_bind.h"
#ಅನ್ತರ್ಭಾವ "m3_deterministic.h"
#ಅನ್ತರ್ಭಾವ "m3_compile.h"
#ಅನ್ತರ್ಭಾವ "m3_host.h"
#ಅನ್ತರ್ಭಾವ "m3_exception.h"
#ಅನ್ತರ್ಭಾವ "m3_info.h"


IM3Environment m3_NewEnvironment ()
{
    IM3Environment env = m3_AllocStruct(M3Environment);

    ಆದರೆ (env) {
        _try
        {
            // create FuncTypes for all simple block return ValueTypes.
            // v128 is skipped: it parses as a slot but has no operations.
            ಗಾಗಿ (u8 t = c_m3Type_none; t < c_m3_Type_ಎಣಿಕೆ; t++) {
                ಆದರೆ (t == c_m3Type_v128) {
                    ಮುಂದುವರಿ;
                }

                IM3FuncType ftype;
_               (AllocFuncType(&ftype, 1));

                ftype->ಅಂಕೆ_ವಾದಗಳು  = 0;
                ftype->ಅಂಕೆ_Rets  = (t == c_m3Type_none) ? 0 : 1;
                ftype->types[0] = t;

                Environment_ಕೂಡಿಸು_Func_Type(env, &ftype);

                env->retFuncTypes[t] = ftype;
            }
        }

        _catch:
        ಆದರೆ (ಫಲಿತಾಂಶ) {
            m3_FreeEnvironment(env);
            env = ಖಾಲಿ;
        }
    }

    ಹಿಂತಿರುಗಿ env;
}


ಶೂನ್ಯ Environment_Release (IM3Environment i_environment)
{
    IM3FuncType ftype = i_environment->funcTypes;

    ಆಗ (ftype) {
        IM3FuncType ಮುಂದಿನ = ftype->ಮುಂದಿನ;
        m3_Free(ftype);
        ftype = ಮುಂದಿನ;
    }

    m3log(runtime, "freeing %d pages from environment", ಎಣಿಕೆ_ಸಂಕೇತ_ಪುಟಗಳು(i_environment->ಪುಟಗಳು_Released));
    Free_ಸಂಕೇತ_ಪುಟಗಳು(&i_environment->ಪುಟಗಳು_Released);
}


ಶೂನ್ಯ m3_FreeEnvironment (IM3Environment i_environment)
{
    ಆದರೆ (i_environment) {
        Environment_Release(i_environment);
        m3_Free(i_environment);
    }
}


ಶೂನ್ಯ m3_ಗಣ_Custom_Section_Handler (IM3Environment i_environment, M3SectionHandler i_handler)
{
    ಆದರೆ (i_environment) {
        i_environment->customSectionHandler = i_handler;
    }
}


// returns the same io_funcType or replaces it with an equivalent that's already in the type linked list
M3_ಫಲಿತಾಂಶ Environment_ಕೂಡಿಸು_Func_Type (IM3Environment i_environment, IM3FuncType* io_funcType)
{
    IM3FuncType ಕೂಡಿಸು_Type = *io_funcType;
    IM3FuncType newType = i_environment->funcTypes;

    ಆಗ (newType) {
        ಆದರೆ (not newType->ಒಳ_Rec_ಗುಂಪು and AreFuncTypesEqual(newType, ಕೂಡಿಸು_Type)) {
            m3_Free(ಕೂಡಿಸು_Type);
            ನಿಲ್ಲಿಸು;
        }

        newType = newType->ಮುಂದಿನ;
    }

    ಆದರೆ (newType == ಖಾಲಿ) {
        // a type index has to fit in the heap type field of an m3type_t
        ಆದರೆ (i_environment->ಅಂಕೆ_Func_Types >= d_m3_Max_Sane_Types_ಎಣಿಕೆ) {
            m3_Free(ಕೂಡಿಸು_Type);
            *io_funcType = ಖಾಲಿ;
            ಹಿಂತಿರುಗಿ "too many distinct function types";
        }

        newType                  = ಕೂಡಿಸು_Type;
        newType->canonical_ಸೂಚ್ಯಂಕ  = i_environment->ಅಂಕೆ_Func_Types++;
        newType->ಮುಂದಿನ            = i_environment->funcTypes;
        i_environment->funcTypes = newType;
    }

    *io_funcType = newType;

    ಹಿಂತಿರುಗಿ m3Err_none;
}


// Adds a type without looking for an equal one first, and hands back the index
// it was given.
//
// Members of a recursive group do not get to share an entry with a type that
// merely looks like them: the group is their identity, so two groups spelled
// the same way are still two groups, and a type inside one is never the type
// outside it that matches field for field. Reserving the indices up front is
// also what lets the members refer to each other while they are being read.
M3_ಫಲಿತಾಂಶ Environment_ReserveFuncTypes (IM3Environment i_environment, u32 i_ಎಣಿಕೆ, u16* o_ಮೊದಲ_ಸೂಚ್ಯಂಕ)
{
    ಆದರೆ ((u64)i_environment->ಅಂಕೆ_Func_Types + i_ಎಣಿಕೆ > d_m3_Max_Sane_Types_ಎಣಿಕೆ) {
        ಹಿಂತಿರುಗಿ "too many distinct function types";
    }

    *o_ಮೊದಲ_ಸೂಚ್ಯಂಕ = i_environment->ಅಂಕೆ_Func_Types;
    i_environment->ಅಂಕೆ_Func_Types += (u16)i_ಎಣಿಕೆ;

    ಹಿಂತಿರುಗಿ m3Err_none;
}


ಶೂನ್ಯ Environment_AdoptFuncType (IM3Environment i_environment, IM3FuncType i_funcType, u16 i_canonical_ಸೂಚ್ಯಂಕ)
{
    i_funcType->ಒಳ_Rec_ಗುಂಪು     = true;
    i_funcType->canonical_ಸೂಚ್ಯಂಕ = i_canonical_ಸೂಚ್ಯಂಕ;
    i_funcType->ಮುಂದಿನ           = i_environment->funcTypes;
    i_environment->funcTypes   = i_funcType;
}


IM3_ಸಂಕೇತ_ಪುಟ Remove_ಸಂಕೇತ_ಪುಟ_ದ_ಸಾಮರ್ಥ್ಯ (M3_ಸಂಕೇತ_ಪುಟ** io_list, u32 i_minimum_ಸಾಲು_ಎಣಿಕೆ)
{
    IM3_ಸಂಕೇತ_ಪುಟ prev = ಖಾಲಿ;
    IM3_ಸಂಕೇತ_ಪುಟ ಪುಟ = *io_list;

    ಆಗ (ಪುಟ) {
        ಆದರೆ (ಅಂಕೆ_Free_ಸಾಲುಗಳು(ಪುಟ) >= i_minimum_ಸಾಲು_ಎಣಿಕೆ) {             d_m3Assert (ಪುಟ->info.usage_ಎಣಿಕೆ == 0);
            IM3_ಸಂಕೇತ_ಪುಟ ಮುಂದಿನ = ಪುಟ->info.ಮುಂದಿನ;
            ಆದರೆ (prev) {
                prev->info.ಮುಂದಿನ = ಮುಂದಿನ; // mid-list
            } ಇಲ್ಲದಿದ್ದರೆ {
                *io_list = ಮುಂದಿನ;       // front of list
            }

            ನಿಲ್ಲಿಸು;
        }

        prev = ಪುಟ;
        ಪುಟ = ಪುಟ->info.ಮುಂದಿನ;
    }

    ಹಿಂತಿರುಗಿ ಪುಟ;
}


IM3_ಸಂಕೇತ_ಪುಟ Environment_Acquire_ಸಂಕೇತ_ಪುಟ (IM3Environment i_environment, u32 i_minimum_ಸಾಲು_ಎಣಿಕೆ)
{
    ಹಿಂತಿರುಗಿ Remove_ಸಂಕೇತ_ಪುಟ_ದ_ಸಾಮರ್ಥ್ಯ(&i_environment->ಪುಟಗಳು_Released, i_minimum_ಸಾಲು_ಎಣಿಕೆ);
}


ಶೂನ್ಯ Environment_Release_ಸಂಕೇತ_ಪುಟಗಳು (IM3Environment i_environment, IM3_ಸಂಕೇತ_ಪುಟ i_ಸಂಕೇತ_ಪುಟ_List)
{
    IM3_ಸಂಕೇತ_ಪುಟ ಕೊನೆ = i_ಸಂಕೇತ_ಪುಟ_List;

    ಆಗ (ಕೊನೆ) {
        ಕೊನೆ->info.ಸಾಲು_ಸೂಚ್ಯಂಕ = 0; // reset page
#ಆದರೆ d_m3_ದಾಖಲೆ_Backtraces
        ಕೊನೆ->info.mapping->ಗಾತ್ರ_2 = 0;
#ಆದರೆ_ಅಂತ್ಯ // d_m3RecordBacktraces

        IM3_ಸಂಕೇತ_ಪುಟ ಮುಂದಿನ = ಕೊನೆ->info.ಮುಂದಿನ;
        ಆದರೆ (not ಮುಂದಿನ) {
            ನಿಲ್ಲಿಸು;
        }

        ಕೊನೆ = ಮುಂದಿನ;
    }

    ಆದರೆ (ಕೊನೆ) {
        // push list to front
        ಕೊನೆ->info.ಮುಂದಿನ               = i_environment->ಪುಟಗಳು_Released;
        i_environment->ಪುಟಗಳು_Released = i_ಸಂಕೇತ_ಪುಟ_List;
    }
}


IM3Runtime m3_NewRuntime (IM3Environment i_environment, u32 i_ರಾಶಿ_ಗಾತ್ರ_ಒಳ_ಬೈಟುಗಳು, ಶೂನ್ಯ* i_userdata)
{
    IM3Runtime runtime = m3_AllocStruct(M3Runtime);

    ಆದರೆ (runtime) {
        m3_ResetErrorInfo(runtime);

        runtime->environment = i_environment;
        runtime->userdata    = i_userdata;

        Deterministic_InitRuntime(runtime);

        runtime->origin_ರಾಶಿ = m3_Malloc("Wasm Stack", i_ರಾಶಿ_ಗಾತ್ರ_ಒಳ_ಬೈಟುಗಳು + 4 * ಗಾತ್ರ(m3slot_t)); // TODO: more precise stack checks

        ಆದರೆ (runtime->origin_ರಾಶಿ) {
            runtime->ರಾಶಿ         = runtime->origin_ರಾಶಿ;
            runtime->ಅಂಕೆ_ರಾಶಿ_Slots = i_ರಾಶಿ_ಗಾತ್ರ_ಒಳ_ಬೈಟುಗಳು / ಗಾತ್ರ(m3slot_t);          m3log (runtime, "new stack: %p, slots: %u", runtime->origin_ರಾಶಿ, runtime->ಅಂಕೆ_ರಾಶಿ_Slots);
        } ಇಲ್ಲದಿದ್ದರೆ {
            m3_Free(runtime);
        }
    }

    ಹಿಂತಿರುಗಿ runtime;
}

ಶೂನ್ಯ m3_ಗಣ_Validation (IM3Runtime i_runtime, bool i_enable)
{
#ಆದರೆ d_m3EnableValidation
    ಆದರೆ (i_runtime) {
        i_runtime->skipValidation = not i_enable;
    }
#ಇಲ್ಲದಿದ್ದರೆ
    (ಶೂನ್ಯ)i_runtime;
    (ಶೂನ್ಯ)i_enable;              // nothing to skip: the validator was compiled out
#ಆದರೆ_ಅಂತ್ಯ
}

M3_ಫಲಿತಾಂಶ m3_ಗಣ_Resource_ಎಲ್ಲೆ (IM3Runtime i_runtime, M3_Resource_ಎಲ್ಲೆ i_ಎಲ್ಲೆ, uint64_t i_ಮೌಲ್ಯ)
{
    ಆದರೆ (not i_runtime) {
        ಹಿಂತಿರುಗಿ m3Err_mallocFailed;
    }
    ಆಯ್ಕೆ (i_ಎಲ್ಲೆ) {
    ಸ್ಥಿತಿ c_m3_ಎಲ್ಲೆ_ಸ್ಮರಣೆ_ಬೈಟುಗಳು:
        ಆದರೆ (i_ಮೌಲ್ಯ and i_ಮೌಲ್ಯ < i_runtime->ಸ್ಮರಣೆ_ಬೈಟುಗಳು_ಬಳಸಿದ) {
            ಹಿಂತಿರುಗಿ m3_Err_resource_ಎಲ್ಲೆ_ಕೆಳಗೆ_Usage;
        }
        i_runtime->ಸ್ಮರಣೆ_ಬೈಟುಗಳು_ಎಲ್ಲೆ = i_ಮೌಲ್ಯ;
        ಹಿಂತಿರುಗಿ m3Err_none;
    ಸ್ಥಿತಿ c_m3_ಎಲ್ಲೆ_ಕೋಷ್ಟಕ_Elements:
        ಆದರೆ (i_ಮೌಲ್ಯ and i_ಮೌಲ್ಯ < i_runtime->ಕೋಷ್ಟಕ_Elements_ಬಳಸಿದ) {
            ಹಿಂತಿರುಗಿ m3_Err_resource_ಎಲ್ಲೆ_ಕೆಳಗೆ_Usage;
        }
        i_runtime->ಕೋಷ್ಟಕ_Elements_ಎಲ್ಲೆ = i_ಮೌಲ್ಯ;
        ಹಿಂತಿರುಗಿ m3Err_none;
#ಆದರೆ d_m3_Has_ರಾಶಿ_Switching
    ಸ್ಥಿತಿ c_m3_ಎಲ್ಲೆ_Continuations:
        ಆದರೆ (i_ಮೌಲ್ಯ and i_ಮೌಲ್ಯ < i_runtime->continuationsAllocated) {
            ಹಿಂತಿರುಗಿ m3_Err_resource_ಎಲ್ಲೆ_ಕೆಳಗೆ_Usage;
        }
        i_runtime->continuations_ಎಲ್ಲೆ = (u32)M3_MIN(i_ಮೌಲ್ಯ, (u64)UINT32_MAX);
        ಆಗ (i_ಮೌಲ್ಯ and i_runtime->val_ರಾಶಿ_Pool_ಎಣಿಕೆ > i_ಮೌಲ್ಯ - i_runtime->continuationsAllocated) {
            m3_Free(i_runtime->val_ರಾಶಿ_Pool[--i_runtime->val_ರಾಶಿ_Pool_ಎಣಿಕೆ]);
        }
        ಹಿಂತಿರುಗಿ m3Err_none;
#ಇಲ್ಲದಿದ್ದರೆ
    ಸ್ಥಿತಿ c_m3_ಎಲ್ಲೆ_Continuations: ಹಿಂತಿರುಗಿ m3_Err_resource_ಎಲ್ಲೆ_Not_Supported;
#ಆದರೆ_ಅಂತ್ಯ
    ಸ್ಥಿತಿ c_m3_ಎಲ್ಲೆ_Gas_Units:
#ಆದರೆ d_m3HasGasMetering
        i_runtime->gas_ಎಲ್ಲೆ = i_runtime->gas_ಉಳಿದ = (i64)M3_MIN(i_ಮೌಲ್ಯ, (u64)INT64_MAX);
        ಹಿಂತಿರುಗಿ m3Err_none;
#ಇಲ್ಲದಿದ್ದರೆ
        ಹಿಂತಿರುಗಿ m3_Err_resource_ಎಲ್ಲೆ_Not_Supported;
#ಆದರೆ_ಅಂತ್ಯ
    ಪೂರ್ವನಿಯೋಜಿತ: ಹಿಂತಿರುಗಿ m3_Err_unknown_Resource_ಎಲ್ಲೆ;
    }
}

uint64_t m3_Get_Resource_ಎಲ್ಲೆ (IM3Runtime i_runtime, M3_Resource_ಎಲ್ಲೆ i_ಎಲ್ಲೆ)
{
    ಆದರೆ (not i_runtime) {
        ಹಿಂತಿರುಗಿ 0;
    }
    ಆಯ್ಕೆ (i_ಎಲ್ಲೆ) {
    ಸ್ಥಿತಿ c_m3_ಎಲ್ಲೆ_ಸ್ಮರಣೆ_ಬೈಟುಗಳು: ಹಿಂತಿರುಗಿ i_runtime->ಸ್ಮರಣೆ_ಬೈಟುಗಳು_ಎಲ್ಲೆ;
    ಸ್ಥಿತಿ c_m3_ಎಲ್ಲೆ_ಕೋಷ್ಟಕ_Elements: ಹಿಂತಿರುಗಿ i_runtime->ಕೋಷ್ಟಕ_Elements_ಎಲ್ಲೆ;
#ಆದರೆ d_m3_Has_ರಾಶಿ_Switching
    ಸ್ಥಿತಿ c_m3_ಎಲ್ಲೆ_Continuations: ಹಿಂತಿರುಗಿ i_runtime->continuations_ಎಲ್ಲೆ;
#ಆದರೆ_ಅಂತ್ಯ
#ಆದರೆ d_m3HasGasMetering
    ಸ್ಥಿತಿ c_m3_ಎಲ್ಲೆ_Gas_Units: ಹಿಂತಿರುಗಿ (u64)i_runtime->gas_ಎಲ್ಲೆ;
#ಆದರೆ_ಅಂತ್ಯ
    ಪೂರ್ವನಿಯೋಜಿತ: ಹಿಂತಿರುಗಿ 0;
    }
}

uint64_t m3_GetResourceUsage (IM3Runtime i_runtime, M3_Resource_ಎಲ್ಲೆ i_ಎಲ್ಲೆ)
{
    ಆದರೆ (not i_runtime) {
        ಹಿಂತಿರುಗಿ 0;
    }
    ಆಯ್ಕೆ (i_ಎಲ್ಲೆ) {
    ಸ್ಥಿತಿ c_m3_ಎಲ್ಲೆ_ಸ್ಮರಣೆ_ಬೈಟುಗಳು: ಹಿಂತಿರುಗಿ i_runtime->ಸ್ಮರಣೆ_ಬೈಟುಗಳು_ಬಳಸಿದ;
    ಸ್ಥಿತಿ c_m3_ಎಲ್ಲೆ_ಕೋಷ್ಟಕ_Elements: ಹಿಂತಿರುಗಿ i_runtime->ಕೋಷ್ಟಕ_Elements_ಬಳಸಿದ;
#ಆದರೆ d_m3_Has_ರಾಶಿ_Switching
    ಸ್ಥಿತಿ c_m3_ಎಲ್ಲೆ_Continuations: ಹಿಂತಿರುಗಿ i_runtime->continuationsAllocated;
#ಆದರೆ_ಅಂತ್ಯ
#ಆದರೆ d_m3HasGasMetering
    ಸ್ಥಿತಿ c_m3_ಎಲ್ಲೆ_Gas_Units: ಹಿಂತಿರುಗಿ (u64)i_runtime->gas_ಎಲ್ಲೆ - (u64)i_runtime->gas_ಉಳಿದ;
#ಆದರೆ_ಅಂತ್ಯ
    ಪೂರ್ವನಿಯೋಜಿತ: ಹಿಂತಿರುಗಿ 0;
    }
}

ಶೂನ್ಯ* m3_Get_User_ದತ್ತಾಂಶ (IM3Runtime i_runtime)
{
    ಹಿಂತಿರುಗಿ i_runtime ? i_runtime->userdata : ಖಾಲಿ;
}


ಶೂನ್ಯ* ForEachModule (IM3Runtime i_runtime, ModuleVisitor i_visitor, ಶೂನ್ಯ* i_info)
{
    ಶೂನ್ಯ* r = ಖಾಲಿ;

    IM3Module module = i_runtime->modules;

    ಆಗ (module) {
        IM3Module ಮುಂದಿನ = module->ಮುಂದಿನ;
        ಆದರೆ ((r = i_visitor(module, i_info))) {
            ನಿಲ್ಲಿಸು;
        }

        module = ಮುಂದಿನ;
    }

    ಹಿಂತಿರುಗಿ r;
}


ಶೂನ್ಯ* _FreeModule (IM3Module i_module, ಶೂನ್ಯ* i_info)
{
    m3_FreeModule(i_module);
    ಹಿಂತಿರುಗಿ ಖಾಲಿ;
}


// A module's memory and table index spaces hold borrowed pointers: linking
// repoints an imported slot at the memory or table the exporting module owns.
// m3_FreeModule tells a borrowed slot from an owned one by reading that owner,
// which only holds while the object is still allocated - and the free walk
// below visits modules in load order, with no ordering between an owner and
// the modules borrowing from it. So clear the borrowed slots first, in a pass
// that frees nothing; the walk then skips them on the NULL check it already
// does, instead of reading a memory some earlier module has freed.
ಶೂನ್ಯ* _ReleaseBorrowedSlots (IM3Module i_module, ಶೂನ್ಯ* i_info)
{
    ಗಾಗಿ (u32 i = 0; i < i_module->ಅಂಕೆ_Memories; ++i) {
        IM3_ಸ್ಮರಣೆ ಸ್ಮರಣೆ = i_module->memories[i];

        ಆದರೆ (ಸ್ಮರಣೆ and ಸ್ಮರಣೆ->owner != i_module) {
            i_module->memories[i] = ಖಾಲಿ;
        }
    }

    ಗಾಗಿ (u32 i = 0; i < i_module->ಅಂಕೆ_Tables; ++i) {
        IM3_ಕೋಷ್ಟಕ ಕೋಷ್ಟಕ_2 = i_module->tables[i];

        ಆದರೆ (ಕೋಷ್ಟಕ_2 and ಕೋಷ್ಟಕ_2->owner != i_module) {
            i_module->tables[i] = ಖಾಲಿ;
        }
    }

    ಹಿಂತಿರುಗಿ ಖಾಲಿ;
}


ಶೂನ್ಯ Runtime_Release (IM3Runtime i_runtime)
{
    ForEachModule(i_runtime, _ReleaseBorrowedSlots, ಖಾಲಿ);
    ForEachModule(i_runtime, _FreeModule, ಖಾಲಿ);                    d_m3Assert (i_runtime->ಅಂಕೆ_ಸಕ್ರಿಯ_ಸಂಕೇತ_ಪುಟಗಳು == 0);

    Environment_Release_ಸಂಕೇತ_ಪುಟಗಳು(i_runtime->environment, i_runtime->ಪುಟಗಳು_Open);
    Environment_Release_ಸಂಕೇತ_ಪುಟಗಳು(i_runtime->environment, i_runtime->ಪುಟಗಳು_ತುಂಬಿದ);

    m3_Free(i_runtime->origin_ರಾಶಿ);

#ಆದರೆ d_m3EnableValidation
    m3_Free(i_runtime->validator);
#ಆದರೆ_ಅಂತ್ಯ

#ಆದರೆ d_m3HasExceptionHandling
    // a runtime let go of while suspended still holds its exceptions
    FreeExceptions(i_runtime);
    m3_Free(i_runtime->uncaughtException);
#ಆದರೆ_ಅಂತ್ಯ
#ಆದರೆ d_m3HasExceptionHandling || d_m3_Has_ರಾಶಿ_Switching
    ಆಗ (i_runtime->host_ಟ್ಯಾಗುಗಳು) {
        M3_Host_ಟ್ಯಾಗ್* ಮುಂದಿನ = i_runtime->host_ಟ್ಯಾಗುಗಳು->ಮುಂದಿನ;
        m3_Free(i_runtime->host_ಟ್ಯಾಗುಗಳು);
        i_runtime->host_ಟ್ಯಾಗುಗಳು = ಮುಂದಿನ;
    }
#ಆದರೆ_ಅಂತ್ಯ
#ಆದರೆ d_m3_Has_ರಾಶಿ_Switching
    Continuation_ReleaseAll(i_runtime);
#ಆದರೆ_ಅಂತ್ಯ
}


ಶೂನ್ಯ m3_FreeRuntime (IM3Runtime i_runtime)
{
    ಆದರೆ (i_runtime) {
        m3_PrintProfilerInfo();

        Runtime_Release(i_runtime);
        m3_Free(i_runtime);
    }
}


#ಆದರೆ d_m3_Has_ರಾಶಿ_Switching

IM3Continuation Continuation_New (IM3Runtime i_runtime, IM3FuncType i_type, IM3Function i_function)
{
    IM3Continuation cont = m3_AllocStruct(M3Continuation);
    ಆದರೆ (!cont) {
        ಹಿಂತಿರುಗಿ ಖಾಲಿ;
    }

    cont->type          = i_type;
    cont->ನಮೂದು_Function = i_function;
    cont->state         = cont_allocated;

    cont->ಅಂಕೆ_ರಾಶಿ_Slots = d_m3_Continuation_ರಾಶಿ_Slots;
    // frames are recorded only when this continuation actually suspends, and
    // only as deep as it got, so the array starts out unallocated
    cont->ಚೌಕಟ್ಟುಗಳು    = ಖಾಲಿ;
    cont->ಚೌಕಟ್ಟುಗಳು_Cap = 0;
    cont->ಅಂಕೆ_ಚೌಕಟ್ಟುಗಳು = 0;

    cont->sp = cont->val_ರಾಶಿ;
    cont->pc = i_function ? i_function->compiled : ಖಾಲಿ;
    cont->r0 = 0;
#  ಆದರೆ d_m3HasFloat
    cont->fp0 = 0;
#  endif
    cont->bound_ವಾದಗಳು_ಎಣಿಕೆ    = 0;
    cont->ಅಂಕೆ_Suspend_Results = 0;
    cont->ಅಂಕೆ_Handlers       = 0;
    cont->handlersPC        = ಖಾಲಿ;
    cont->ಪೋಷಕ            = ಖಾಲಿ;

    cont->ಮುಂದಿನ               = i_runtime->continuations;
    i_runtime->continuations = cont;

    ಹಿಂತಿರುಗಿ cont;
}

IM3Continuation Continuation_ForkSuspended (IM3Runtime i_runtime, IM3Continuation i_cont)
{
    IM3Continuation newCont = Continuation_New(i_runtime, i_cont->type, i_cont->ನಮೂದು_Function);
    ಆದರೆ (M3_UNLIKELY(not newCont)) {
        ಹಿಂತಿರುಗಿ ಖಾಲಿ;
    }

    newCont->val_ರಾಶಿ      = i_cont->val_ರಾಶಿ;
    newCont->ಅಂಕೆ_ರಾಶಿ_Slots = i_cont->ಅಂಕೆ_ರಾಶಿ_Slots;
    newCont->sp            = i_cont->sp;
    newCont->pc            = i_cont->pc;
    newCont->suspend_ಬಿಂದು  = i_cont->suspend_ಬಿಂದು;
    newCont->r0            = i_cont->r0;
#  ಆದರೆ d_m3HasFloat
    newCont->fp0 = i_cont->fp0;
#  endif
    newCont->ಚೌಕಟ್ಟುಗಳು    = i_cont->ಚೌಕಟ್ಟುಗಳು;
    newCont->ಚೌಕಟ್ಟುಗಳು_Cap = i_cont->ಚೌಕಟ್ಟುಗಳು_Cap;
    newCont->ಅಂಕೆ_ಚೌಕಟ್ಟುಗಳು = i_cont->ಅಂಕೆ_ಚೌಕಟ್ಟುಗಳು;

    newCont->bound_ವಾದಗಳು_ಎಣಿಕೆ = 0;
#  ಆದರೆ d_m3HasExceptionHandling
    newCont->resumeThrow = i_cont->resumeThrow;
    i_cont->resumeThrow  = ಖಾಲಿ;
#  endif
    newCont->ಅಂಕೆ_Suspend_Results = i_cont->ಅಂಕೆ_Suspend_Results;
    ಗಾಗಿ (u32 i = 0; i < i_cont->ಅಂಕೆ_Suspend_Results; ++i) {
        newCont->suspend_ಫಲಿತಾಂಶ_Offsets[i] = i_cont->suspend_ಫಲಿತಾಂಶ_Offsets[i];
        newCont->suspend_ಫಲಿತಾಂಶ_Is64[i]    = i_cont->suspend_ಫಲಿತಾಂಶ_Is64[i];
    }

    newCont->ಅಂಕೆ_Handlers = i_cont->ಅಂಕೆ_Handlers;
    newCont->handlersPC  = i_cont->handlersPC;
    newCont->ಪೋಷಕ      = i_cont->ಪೋಷಕ;
    newCont->state       = cont_suspended;

    i_cont->val_ರಾಶಿ  = ಖಾಲಿ;
    i_cont->ಚೌಕಟ್ಟುಗಳು    = ಖಾಲಿ;
    i_cont->ಚೌಕಟ್ಟುಗಳು_Cap = 0;
    i_cont->ಅಂಕೆ_ಚೌಕಟ್ಟುಗಳು = 0;
    i_cont->state     = cont_consumed;

    ಹಿಂತಿರುಗಿ newCont;
}

// Only cont.new and snapshot restoration acquire stacks. Binding and suspension
// transfer them between records without changing the active count.
M3_ಫಲಿತಾಂಶ Continuation_Acquire_ರಾಶಿ (IM3Runtime runtime, IM3Continuation cont)
{
    ಆದರೆ (not Continuation_Can_Acquire_ರಾಶಿ(runtime)) {
        ಹಿಂತಿರುಗಿ m3_Err_continuation_ಎಲ್ಲೆ_Exceeded;
    }
    ಆದರೆ (runtime->val_ರಾಶಿ_Pool_ಎಣಿಕೆ) {
        cont->val_ರಾಶಿ = runtime->val_ರಾಶಿ_Pool[--runtime->val_ರಾಶಿ_Pool_ಎಣಿಕೆ];
        memset(cont->val_ರಾಶಿ, 0, (cont->ಅಂಕೆ_ರಾಶಿ_Slots + 4) * ಗಾತ್ರ(m3slot_t));
    } ಇಲ್ಲದಿದ್ದರೆ {
        cont->val_ರಾಶಿ = m3_AllocArray(m3slot_t, cont->ಅಂಕೆ_ರಾಶಿ_Slots + 4);
    }
    ಆದರೆ (not cont->val_ರಾಶಿ) {
        ಹಿಂತಿರುಗಿ m3Err_mallocFailed;
    }
    cont->sp = cont->val_ರಾಶಿ;
    runtime->continuationsAllocated++;
    ಹಿಂತಿರುಗಿ m3Err_none;
}

ಶೂನ್ಯ Continuation_Recycle_ರಾಶಿ (IM3Runtime runtime, IM3Continuation cont)
{
    ಆದರೆ (not cont->val_ರಾಶಿ or cont == runtime->ಬೇರು_Continuation) {
        ಹಿಂತಿರುಗಿ;
    }
    ಆದರೆ (not runtime->val_ರಾಶಿ_Pool) {
        runtime->val_ರಾಶಿ_Pool = m3_AllocArray(m3slot_t*, 16);
        ಆದರೆ (runtime->val_ರಾಶಿ_Pool) {
            runtime->val_ರಾಶಿ_Pool_Cap = 16;
        }
    }
    ಆದರೆ (runtime->val_ರಾಶಿ_Pool_ಎಣಿಕೆ < runtime->val_ರಾಶಿ_Pool_Cap and
        (not runtime->continuations_ಎಲ್ಲೆ or runtime->val_ರಾಶಿ_Pool_ಎಣಿಕೆ <
                                              runtime->continuations_ಎಲ್ಲೆ - runtime->continuationsAllocated + 1)) {
        runtime->val_ರಾಶಿ_Pool[runtime->val_ರಾಶಿ_Pool_ಎಣಿಕೆ++] = cont->val_ರಾಶಿ;
        cont->val_ರಾಶಿ                                      = ಖಾಲಿ;
    } ಇಲ್ಲದಿದ್ದರೆ {
        m3_Free(cont->val_ರಾಶಿ);
    }
    cont->sp = ಖಾಲಿ;
    runtime->continuationsAllocated--;
}

ಶೂನ್ಯ Continuation_ReleaseAll (IM3Runtime io_runtime)
{
    // Records live until teardown: a consumed reference can still be in a table
    // or on a Wasm stack. Its value stack has already been recycled.
    IM3Continuation cont = io_runtime->continuations;

    ಆಗ (cont) {
        IM3Continuation ಮುಂದಿನ = cont->ಮುಂದಿನ;

        m3_Free(cont->val_ರಾಶಿ);
        m3_Free(cont->ಚೌಕಟ್ಟುಗಳು);
        m3_Free(cont);

        cont = ಮುಂದಿನ;
    }

    ಗಾಗಿ (u32 i = 0; i < io_runtime->val_ರಾಶಿ_Pool_ಎಣಿಕೆ; ++i) {
        m3_Free(io_runtime->val_ರಾಶಿ_Pool[i]);
    }
    m3_Free(io_runtime->val_ರಾಶಿ_Pool);
    io_runtime->val_ರಾಶಿ_Pool_ಎಣಿಕೆ = io_runtime->val_ರಾಶಿ_Pool_Cap = 0;
    io_runtime->continuationsAllocated                          = 0;
    io_runtime->continuations                                   = ಖಾಲಿ;
    io_runtime->ಸಕ್ರಿಯ_Continuation                              = ಖಾಲಿ;
    io_runtime->ಬೇರು_Continuation                                = ಖಾಲಿ;
}


m3ret_t Continuation_ದಾಖಲೆ_ಚೌಕಟ್ಟು (IM3Runtime i_runtime, ಸ್ಥಿರಾಂಕ M3_ಚೌಕಟ್ಟು* i_ಚೌಕಟ್ಟು)
{
    IM3Continuation cont = i_runtime->ಸಕ್ರಿಯ_Continuation;

    // the marker only travels between a suspend and the resume that runs it,
    // and that whole stretch has an active continuation
    d_m3Assert(cont);

    ಆದರೆ (cont->ಅಂಕೆ_ಚೌಕಟ್ಟುಗಳು == cont->ಚೌಕಟ್ಟುಗಳು_Cap) {
        u32 newCap = cont->ಚೌಕಟ್ಟುಗಳು_Cap ? cont->ಚೌಕಟ್ಟುಗಳು_Cap * 2 : 8;

        ಆದರೆ (newCap > d_m3_Continuation_Max_ಚೌಕಟ್ಟುಗಳು) {
            newCap = d_m3_Continuation_Max_ಚೌಕಟ್ಟುಗಳು;
        }

        ಆದರೆ (newCap == cont->ಚೌಕಟ್ಟುಗಳು_Cap) {
            ಹಿಂತಿರುಗಿ m3_Err_trap_ರಾಶಿ_ಅತಿಪ್ರವಾಹ;
        }

        M3_ಚೌಕಟ್ಟು* ಚೌಕಟ್ಟುಗಳು = m3_ReallocArray(M3_ಚೌಕಟ್ಟು, cont->ಚೌಕಟ್ಟುಗಳು, newCap, cont->ಚೌಕಟ್ಟುಗಳು_Cap);

        ಆದರೆ (not ಚೌಕಟ್ಟುಗಳು) {
            ಹಿಂತಿರುಗಿ m3Err_mallocFailed;
        }

        cont->ಚೌಕಟ್ಟುಗಳು    = ಚೌಕಟ್ಟುಗಳು;
        cont->ಚೌಕಟ್ಟುಗಳು_Cap = newCap;
    }

    cont->ಚೌಕಟ್ಟುಗಳು[cont->ಅಂಕೆ_ಚೌಕಟ್ಟುಗಳು++] = *i_ಚೌಕಟ್ಟು;

    ಹಿಂತಿರುಗಿ m3Err_continuationSuspended;
}

#ಆದರೆ_ಅಂತ್ಯ // d_m3HasStackSwitching

M3_ಫಲಿತಾಂಶ Evaluate_ಅಭಿವ್ಯಕ್ತಿ (IM3Module i_module, ಶೂನ್ಯ* o_expressed, m3type_t i_type, ಬೈಟುಗಳು_t* io_ಬೈಟುಗಳು, cbytes_t i_ಕೊನೆ)
{
    M3_ಫಲಿತಾಂಶ ಫಲಿತಾಂಶ = m3Err_none;

    // OPTZ: use a simplified interpreter for expressions

    // create a temporary runtime context
#ಆದರೆ defined(d_m3PreferStaticAlloc)
    ಸ್ಥಿರ M3Runtime runtime;
#ಇಲ್ಲದಿದ್ದರೆ
    M3Runtime runtime;
#ಆದರೆ_ಅಂತ್ಯ
    M3_INIT(runtime);

    runtime.environment   = i_module->runtime->environment;
    runtime.ಅಂಕೆ_ರಾಶಿ_Slots = i_module->runtime->ಅಂಕೆ_ರಾಶಿ_Slots;
    runtime.ರಾಶಿ         = i_module->runtime->ರಾಶಿ;

    m3stack_t ರಾಶಿ = (m3stack_t)runtime.ರಾಶಿ;

    IM3Runtime savedRuntime = i_module->runtime;
    i_module->runtime       = &runtime;

    IM3Compilation o   = &runtime.compilation;
    o->runtime         = &runtime;
    o->module          = i_module;
    o->wasm            = *io_ಬೈಟುಗಳು;
    o->wasm_ಕೊನೆ         = i_ಕೊನೆ;
    o->ಕೊನೆಯ_Opcode_ಆರಂಭ = o->wasm;

    // Borrowed for the length of this expression only. Runtime_Release below hands it
    // to Environment_ReleaseCodePages, which resets lineIndex, so the page comes back
    // empty and the next expression reuses it rather than burning fresh lines.
    o->ಪುಟ = Acquire_ಸಂಕೇತ_ಪುಟ(&runtime);

    ಆದರೆ (o->ಪುಟ) {
        IM3FuncType ftype = ಖಾಲಿ;

#ಆದರೆ d_m3HasTypedRefs
        // A reference type has to be checked as the type it is: the shared
        // return types are one per storage class, so borrowing one of those
        // would let any reference at all satisfy a particular one.
        ಆದರೆ (IsSpelledRefType(i_type)) {
            ಫಲಿತಾಂಶ = AllocFuncType(&ftype, 1);

            ಆದರೆ (not ಫಲಿತಾಂಶ) {
                ftype->ಅಂಕೆ_Rets  = 1;
                ftype->types[0] = i_type;

                ಫಲಿತಾಂಶ = Environment_ಕೂಡಿಸು_Func_Type(runtime.environment, &ftype);
            }
        } ಇಲ್ಲದಿದ್ದರೆ
#ಆದರೆ_ಅಂತ್ಯ
        {
            ftype = runtime.environment->retFuncTypes[ತಳ_Type_ದ(i_type)];
        }

        pc_t m3code = Get_ಪುಟ_PC(o->ಪುಟ);

        ಆದರೆ (not ಫಲಿತಾಂಶ) {
            ಫಲಿತಾಂಶ = Compile_ಅಭಿವ್ಯಕ್ತಿ(o, ftype);
        }

        ಆದರೆ (not ಫಲಿತಾಂಶ && o->max_ರಾಶಿ_Slots >= runtime.ಅಂಕೆ_ರಾಶಿ_Slots) {
            ಫಲಿತಾಂಶ = m3_Err_trap_ರಾಶಿ_ಅತಿಪ್ರವಾಹ;
        }

        ಆದರೆ (not ಫಲಿತಾಂಶ) {
#ಆದರೆ (d_m3EnableOpProfiling || d_m3EnableOpTracing)
            m3ret_t r = ಸತತ_ಸಂಕೇತ(m3code, ರಾಶಿ, ಖಾಲಿ, d_m3_Op_Default_ವಾದಗಳು, d_m3_ತಳ_Cstr);
#ಇಲ್ಲದಿದ್ದರೆ
            m3ret_t r = ಸತತ_ಸಂಕೇತ(m3code, ರಾಶಿ, ಖಾಲಿ, d_m3_Op_Default_ವಾದಗಳು);
#ಆದರೆ_ಅಂತ್ಯ

            ಆದರೆ (r == 0) {                                                                   m3log (runtime, "expression result: %s", SPrint_ಮೌಲ್ಯ (ರಾಶಿ, i_type));
                ಆದರೆ (ಗಾತ್ರ_ದ_Type(ತಳ_Type_ದ(i_type)) == ಗಾತ್ರ(u32)) {
                    *(u32*)o_expressed = *((u32*)ರಾಶಿ);
                } ಇಲ್ಲದಿದ್ದರೆ {
                    *(u64*)o_expressed = *((u64*)ರಾಶಿ);
                }
            }
        }

        Release_ಸಂಕೇತ_ಪುಟ(&runtime, o->ಪುಟ);
    } ಇಲ್ಲದಿದ್ದರೆ {
        ಫಲಿತಾಂಶ = m3_Err_malloc_Failed_ಸಂಕೇತ_ಪುಟ;
    }

    runtime.origin_ರಾಶಿ = ಖಾಲಿ;        // prevent free(stack) in ReleaseRuntime
    Runtime_Release(&runtime);
    i_module->runtime = savedRuntime;

    *io_ಬೈಟುಗಳು = o->wasm;

    ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
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
ಸ್ಥಿರ
bool LimitsSatisfy (u64 i_exported_ಗಾತ್ರ, bool i_exportedHasMax, u64 i_exportedMax,
                    u64 i_importMin, bool i_importHasMax, u64 i_importMax)
{
    ಆದರೆ (i_exported_ಗಾತ್ರ < i_importMin) {
        ಹಿಂತಿರುಗಿ false;
    }

    ಆದರೆ (i_importHasMax and (not i_exportedHasMax or i_exportedMax > i_importMax)) {
        ಹಿಂತಿರುಗಿ false;
    }

    ಹಿಂತಿರುಗಿ true;
}


// The export of this kind under this name, or NULL. The index it carries is
// resolved by the caller, against the module's index space as it stands then -
// after linking, a re-exported import's slot holds the entity it was linked to.
ಸ್ಥಿರ
ಸ್ಥಿರಾಂಕ M3Export* Module_ಕಂಡುಹಿಡಿ_Export (IM3Module i_module, u8 i_ಬಗೆ, cstr_t i_name)
{
    ಗಾಗಿ (u32 i = 0; i < i_module->ಅಂಕೆ_Exports; ++i) {
        ಸ್ಥಿರಾಂಕ M3Export* e = &i_module->exports[i];

        ಆದರೆ (e->ಬಗೆ == i_ಬಗೆ and strcmp(e->name, i_name) == 0) {
            ಹಿಂತಿರುಗಿ e;
        }
    }

    ಹಿಂತಿರುಗಿ ಖಾಲಿ;
}


ಸ್ಥಿರ
IM3Function Module_ಕಂಡುಹಿಡಿ_Exported_Function (IM3Module i_module, cstr_t i_name)
{
    ಸ್ಥಿರಾಂಕ M3Export* e = Module_ಕಂಡುಹಿಡಿ_Export(i_module, d_external_ಬಗೆ_function, i_name);

    // A module that re-exports an import names the placeholder here. Resolve to
    // the function that actually runs, or a host-side call would run it against
    // the importing module's memory.
    ಹಿಂತಿರುಗಿ e ? Function_Implementation(&i_module->functions[e->index]) : ಖಾಲಿ;
}


ಸ್ಥಿರ
IM3_ಸ್ಮರಣೆ Module_ಕಂಡುಹಿಡಿ_Exported_ಸ್ಮರಣೆ (IM3Module i_module, cstr_t i_name)
{
    ಸ್ಥಿರಾಂಕ M3Export* e = Module_ಕಂಡುಹಿಡಿ_Export(i_module, d_external_ಬಗೆ_ಸ್ಮರಣೆ, i_name);

    ಹಿಂತಿರುಗಿ e ? i_module->memories[e->index] : ಖಾಲಿ;
}


ಸ್ಥಿರ
IM3_ಕೋಷ್ಟಕ Module_ಕಂಡುಹಿಡಿ_Exported_ಕೋಷ್ಟಕ (IM3Module i_module, cstr_t i_name)
{
    ಸ್ಥಿರಾಂಕ M3Export* e = Module_ಕಂಡುಹಿಡಿ_Export(i_module, d_external_ಬಗೆ_ಕೋಷ್ಟಕ, i_name);

    ಹಿಂತಿರುಗಿ e ? i_module->tables[e->index] : ಖಾಲಿ;
}


ಸ್ಥಿರ
IM3Global Module_ಕಂಡುಹಿಡಿ_Exported_Global (IM3Module i_module, cstr_t i_name)
{
    ಸ್ಥಿರಾಂಕ M3Export* e = Module_ಕಂಡುಹಿಡಿ_Export(i_module, d_external_ಬಗೆ_global, i_name);

    ಹಿಂತಿರುಗಿ e ? &i_module->globals[e->index] : ಖಾಲಿ;
}


#ಆದರೆ d_m3HasExceptionHandling || d_m3_Has_ರಾಶಿ_Switching
ಸ್ಥಿರ
IM3_ಟ್ಯಾಗ್ Module_ಕಂಡುಹಿಡಿ_Exported_ಟ್ಯಾಗ್ (IM3Module i_module, cstr_t i_name)
{
    ಸ್ಥಿರಾಂಕ M3Export* e = Module_ಕಂಡುಹಿಡಿ_Export(i_module, d_external_ಬಗೆ_ಟ್ಯಾಗ್, i_name);

    ಆದರೆ (not e) {
        ಹಿಂತಿರುಗಿ ಖಾಲಿ;
    }

    IM3_ಟ್ಯಾಗ್ ಟ್ಯಾಗ್ = &i_module->ಟ್ಯಾಗುಗಳು[e->index];

    ಹಿಂತಿರುಗಿ ಟ್ಯಾಗ್->resolved ? ಟ್ಯಾಗ್->resolved : ಟ್ಯಾಗ್;
}
#ಆದರೆ_ಅಂತ್ಯ


// Whether the module exports anything at all under this name. Export names are
// unique within a module, so a name one of the lookups above missed but this
// one finds is exported as something else - a kind the import cannot be
// satisfied by, rather than a name the module never exported.
ಸ್ಥಿರ
bool Module_HasExport (IM3Module i_module, cstr_t i_name)
{
    ಗಾಗಿ (u32 i = 0; i < i_module->ಅಂಕೆ_Exports; ++i) {
        ಆದರೆ (strcmp(i_module->exports[i].name, i_name) == 0) {
            ಹಿಂತಿರುಗಿ true;
        }
    }

    ಹಿಂತಿರುಗಿ false;
}


// Points each of the module's imports at whatever already-loaded module exports
// it. Runs before anything is allocated or initialized: a memory import has to
// be resolved before InitMemory would give it pages of its own, and a global
// import before InitGlobals runs an initializer that reads it.
ಸ್ಥಿರ
M3_ಫಲಿತಾಂಶ LinkImports (IM3Runtime io_runtime, IM3Module io_module)
{
    M3_ಫಲಿತಾಂಶ ಫಲಿತಾಂಶ = m3Err_none;

    ಗಾಗಿ (u32 i = 0; i < io_module->ಅಂಕೆ_Functions; ++i) {
        IM3Function f = &io_module->functions[i];

        ಆದರೆ (f->wasm or not (f->import.moduleUtf8 and f->import.ಕ್ಷೇತ್ರ_Utf8)) {
            ಮುಂದುವರಿ;
        }

        IM3Module from = m3_ಕಂಡುಹಿಡಿ_Module(io_runtime, f->import.moduleUtf8);
        ಆದರೆ (not from) {
            ಮುಂದುವರಿ;
        }

        IM3Function exported = Module_ಕಂಡುಹಿಡಿ_Exported_Function(from, f->import.ಕ್ಷೇತ್ರ_Utf8);
        ಆದರೆ (not exported) {
            // the import names a module that is loaded, so its exports settle the
            // question: a name it exports as something else is a type mismatch, and
            // one it does not export at all is an unknown import
            _throwif(m3Err_incompatibleImportType, Module_HasExport(from, f->import.ಕ್ಷೇತ್ರ_Utf8));
            _throw(m3Err_unknownImport);
        }

        // func types are canonical within an environment, so this is the
        // structural equivalence the spec asks for
        _throwif(m3Err_incompatibleImportType, exported->funcType != f->funcType);

        f->resolved = Function_Implementation(exported);
    }

    ಗಾಗಿ (u32 i = 0; i < io_module->ಅಂಕೆ_Memories; ++i) {
        IM3_ಸ್ಮರಣೆ ಸ್ಮರಣೆ = io_module->memories[i];

        ಆದರೆ (not ಸ್ಮರಣೆ->imported or ಸ್ಮರಣೆ->owner != io_module) {
            ಮುಂದುವರಿ;
        }

        IM3Module from = m3_ಕಂಡುಹಿಡಿ_Module(io_runtime, ಸ್ಮರಣೆ->import.moduleUtf8);
        ಆದರೆ (not from) {
            ಮುಂದುವರಿ;
        }

        IM3_ಸ್ಮರಣೆ exported = Module_ಕಂಡುಹಿಡಿ_Exported_ಸ್ಮರಣೆ(from, ಸ್ಮರಣೆ->import.ಕ್ಷೇತ್ರ_Utf8);
        ಆದರೆ (not exported) {
            _throwif(m3Err_incompatibleImportType, Module_HasExport(from, ಸ್ಮರಣೆ->import.ಕ್ಷೇತ್ರ_Utf8));
            _throw(m3Err_unknownImport);
        }

        // the address type is part of the memory type, so an i64 memory does
        // not satisfy an i32 import, or the other way round - and so is the page
        // size, which custom page sizes made a declared property of a memory
        _throwif(m3Err_incompatibleImportType, exported->isMemory64 != ಸ್ಮರಣೆ->isMemory64);
        _throwif(m3Err_incompatibleImportType, ಸ್ಮರಣೆ_ಪುಟ_ಗಾತ್ರ(exported) != ಸ್ಮರಣೆ_ಪುಟ_ಗಾತ್ರ(ಸ್ಮರಣೆ));

        // a shared memory is not a subtype of an unshared one, or the other way round
        _throwif(m3Err_incompatibleImportType, exported->isShared != ಸ್ಮರಣೆ->isShared);

        _throwif(m3Err_incompatibleImportType,
                 not LimitsSatisfy(exported->ಅಂಕೆ_ಪುಟಗಳು, exported->hasMax, exported->max_ಪುಟಗಳು,
                                   ಸ್ಮರಣೆ->init_ಪುಟಗಳು, ಸ್ಮರಣೆ->hasMax, ಸ್ಮರಣೆ->max_ಪುಟಗಳು));

        // hand the slot over to the exporter's memory, and drop the placeholder
        Free_ಸ್ಮರಣೆ_ಖಂಡ(ಸ್ಮರಣೆ);
        FreeImportInfo(&ಸ್ಮರಣೆ->import);
        m3_Free(ಸ್ಮರಣೆ);

        io_module->memories[i] = exported;
    }

    ಗಾಗಿ (u32 i = 0; i < io_module->ಅಂಕೆ_Tables; ++i) {
        IM3_ಕೋಷ್ಟಕ ಕೋಷ್ಟಕ_2 = io_module->tables[i];

        ಆದರೆ (not ಕೋಷ್ಟಕ_2->imported or ಕೋಷ್ಟಕ_2->owner != io_module) {
            ಮುಂದುವರಿ;
        }

        IM3Module from = m3_ಕಂಡುಹಿಡಿ_Module(io_runtime, ಕೋಷ್ಟಕ_2->import.moduleUtf8);
        ಆದರೆ (not from) {
            ಮುಂದುವರಿ;
        }

        IM3_ಕೋಷ್ಟಕ exported = Module_ಕಂಡುಹಿಡಿ_Exported_ಕೋಷ್ಟಕ(from, ಕೋಷ್ಟಕ_2->import.ಕ್ಷೇತ್ರ_Utf8);
        ಆದರೆ (not exported) {
            _throwif(m3Err_incompatibleImportType, Module_HasExport(from, ಕೋಷ್ಟಕ_2->import.ಕ್ಷೇತ್ರ_Utf8));
            _throw(m3Err_unknownImport);
        }

        _throwif(m3Err_incompatibleImportType, exported->type != ಕೋಷ್ಟಕ_2->type);

        // the index type is part of the table type, the same way it is for a memory
        _throwif(m3Err_incompatibleImportType, exported->isTable64 != ಕೋಷ್ಟಕ_2->isTable64);

        _throwif(m3Err_incompatibleImportType,
                 not LimitsSatisfy(exported->ಗಾತ್ರ_2, exported->hasMax, exported->max_ಗಾತ್ರ,
                                   ಕೋಷ್ಟಕ_2->init_ಗಾತ್ರ, ಕೋಷ್ಟಕ_2->hasMax, ಕೋಷ್ಟಕ_2->max_ಗಾತ್ರ));

        // hand the slot over to the exporter's table, and drop the placeholder
        m3_Free(ಕೋಷ್ಟಕ_2->elements);
        FreeImportInfo(&ಕೋಷ್ಟಕ_2->import);
        m3_Free(ಕೋಷ್ಟಕ_2);

        io_module->tables[i] = exported;
    }

    ಗಾಗಿ (u32 i = 0; i < io_module->ಅಂಕೆ_Globals; ++i) {
        IM3Global g = &io_module->globals[i];

        ಆದರೆ (not g->imported or not (g->import.moduleUtf8 and g->import.ಕ್ಷೇತ್ರ_Utf8)) {
            ಮುಂದುವರಿ;
        }

        IM3Module from = m3_ಕಂಡುಹಿಡಿ_Module(io_runtime, g->import.moduleUtf8);
        ಆದರೆ (not from) {
            ಮುಂದುವರಿ;
        }

        IM3Global exported = Module_ಕಂಡುಹಿಡಿ_Exported_Global(from, g->import.ಕ್ಷೇತ್ರ_Utf8);
        ಆದರೆ (not exported) {
            _throwif(m3Err_incompatibleImportType, Module_HasExport(from, g->import.ಕ್ಷೇತ್ರ_Utf8));
            _throw(m3Err_unknownImport);
        }

        _throwif(m3Err_incompatibleImportType, exported->type != g->type);
        _throwif(m3Err_incompatibleImportType, exported->isMutable != g->isMutable);

        g->resolved = exported->resolved ? exported->resolved : exported;
    }

#ಆದರೆ d_m3HasExceptionHandling || d_m3_Has_ರಾಶಿ_Switching
    ಗಾಗಿ (u32 i = 0; i < io_module->ಅಂಕೆ_ಟ್ಯಾಗುಗಳು; ++i) {
        IM3_ಟ್ಯಾಗ್ ಟ್ಯಾಗ್ = &io_module->ಟ್ಯಾಗುಗಳು[i];

        ಆದರೆ (not ಟ್ಯಾಗ್->imported or not (ಟ್ಯಾಗ್->import.moduleUtf8 and ಟ್ಯಾಗ್->import.ಕ್ಷೇತ್ರ_Utf8)) {
            ಮುಂದುವರಿ;
        }

        IM3Module from = m3_ಕಂಡುಹಿಡಿ_Module(io_runtime, ಟ್ಯಾಗ್->import.moduleUtf8);
        ಆದರೆ (not from) {
            ಮುಂದುವರಿ;
        }

        IM3_ಟ್ಯಾಗ್ exported = Module_ಕಂಡುಹಿಡಿ_Exported_ಟ್ಯಾಗ್(from, ಟ್ಯಾಗ್->import.ಕ್ಷೇತ್ರ_Utf8);
        ಆದರೆ (not exported) {
            _throwif(m3Err_incompatibleImportType, Module_HasExport(from, ಟ್ಯಾಗ್->import.ಕ್ಷೇತ್ರ_Utf8));
            _throw(m3Err_unknownImport);
        }

        _throwif(m3Err_incompatibleImportType, exported->type != ಟ್ಯಾಗ್->type);

        ಟ್ಯಾಗ್->resolved = exported->resolved ? exported->resolved : exported;
    }
#ಆದರೆ_ಅಂತ್ಯ

    _catch: ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}


// Backs each of the module's memories with pages. LinkImports has already
// pointed any import it could satisfy at the exporting module's memory, and
// those are skipped here. An import nothing satisfied is still backed locally
// from its own declared limits, so that the module remains loadable.
M3_ಫಲಿತಾಂಶ Init_ಸ್ಮರಣೆ (IM3Runtime io_runtime, IM3Module i_module)
{
    M3_ಫಲಿತಾಂಶ ಫಲಿತಾಂಶ = m3Err_none;

    // Fixed from here on: the index space stops changing after parse, and
    // linking has already repointed any slot it was going to.
    i_module->memory0 = i_module->ಅಂಕೆ_Memories ? i_module->memories[0]
                                              : &i_module->ಖಾಲಿಗಳು_ಸ್ಮರಣೆ;

    ಆದರೆ (i_module->ಅಂಕೆ_Memories == 0) {
        // nothing addressable, but _mem still has to point somewhere
        i_module->ಖಾಲಿಗಳು_ಸ್ಮರಣೆ.owner    = i_module;
        i_module->ಖಾಲಿಗಳು_ಸ್ಮರಣೆ.ಪುಟ_ಗಾತ್ರ = d_m3_Default_Mem_ಪುಟ_ಗಾತ್ರ;

_       (Resize_ಸ್ಮರಣೆ(io_runtime, &i_module->ಖಾಲಿಗಳು_ಸ್ಮರಣೆ, 0));
    }

    ಗಾಗಿ (u32 i = 0; i < i_module->ಅಂಕೆ_Memories; ++i) {
        IM3_ಸ್ಮರಣೆ ಸ್ಮರಣೆ = i_module->memories[i];

        // a slot that already points at another module's memory is that
        // module's to allocate
        ಆದರೆ (ಸ್ಮರಣೆ->owner != i_module or ಸ್ಮರಣೆ->mallocated) {
            ಮುಂದುವರಿ;
        }

        u32 ಪುಟ_ಗಾತ್ರ = ಸ್ಮರಣೆ_ಪುಟ_ಗಾತ್ರ(ಸ್ಮರಣೆ);

        ಸ್ಮರಣೆ->ಪುಟ_ಗಾತ್ರ = ಪುಟ_ಗಾತ್ರ;

        // Without a declared maximum a memory may grow to the spec limit of
        // 2^|addrtype|/pagesize pages, which is the usual 65536 at the default
        // page size, 2^48 for a 64-bit memory, and a whole address space of
        // them when a page is a single byte. A declared maximum of zero is a
        // real limit, not the absence of one.
        //
        // 2^64/pagesize overflows a u64 only when a page is a single byte, and
        // a power-of-two page size divides the address space exactly, so the
        // 64-bit division below is 2^64/pagesize written so that it fits.
        ಆದರೆ (not ಸ್ಮರಣೆ->hasMax) {
            ಸ್ಮರಣೆ->max_ಪುಟಗಳು = ಸ್ಮರಣೆ->isMemory64
                                 ? (ಪುಟ_ಗಾತ್ರ > 1 ? (UINT64_MAX / ಪುಟ_ಗಾತ್ರ) + 1 : UINT64_MAX)
                                 : (0x100000000ull / ಪುಟ_ಗಾತ್ರ);
        }

_       (Resize_ಸ್ಮರಣೆ(io_runtime, ಸ್ಮರಣೆ, ಸ್ಮರಣೆ->init_ಪುಟಗಳು));
    }

    _catch: ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}


M3_ಫಲಿತಾಂಶ Resize_ಸ್ಮರಣೆ (IM3Runtime io_runtime, IM3_ಸ್ಮರಣೆ ಸ್ಮರಣೆ, u64 i_ಅಂಕೆ_ಪುಟಗಳು)
{
    M3_ಫಲಿತಾಂಶ ಫಲಿತಾಂಶ = m3Err_none;

    u64 ಅಂಕೆ_ಪುಟಗಳು_ಗಮ್ಯ_Alloc = i_ಅಂಕೆ_ಪುಟಗಳು;

    ಆದರೆ (ಅಂಕೆ_ಪುಟಗಳು_ಗಮ್ಯ_Alloc <= ಸ್ಮರಣೆ->max_ಪುಟಗಳು) {
        // A 64-bit memory may ask for up to 2^48 pages, which overflows a u64
        // of bytes. Nothing that large can be backed, so refuse it up front
        // rather than multiplying into a wrapped size.
        _throwif("linear memory limitation exceeded",
                 ಅಂಕೆ_ಪುಟಗಳು_ಗಮ್ಯ_Alloc > d_m3_ವಿಳಾಸ_ಎಲ್ಲೆ / ಸ್ಮರಣೆ->ಪುಟ_ಗಾತ್ರ);

        u64 ಅಂಕೆ_ಪುಟ_ಬೈಟುಗಳು = ಅಂಕೆ_ಪುಟಗಳು_ಗಮ್ಯ_Alloc * ಸ್ಮರಣೆ->ಪುಟ_ಗಾತ್ರ;

#ಆದರೆ d_m3_Max_ರೇಖೀಯ_ಸ್ಮರಣೆ_ಪುಟಗಳು > 0
        // the limit is a memory size, counted in default-sized pages; comparing
        // it against a raw page count would make it 65536 times stricter for a
        // module whose pages are one byte
        _throwif("linear memory limitation exceeded",
                 ಅಂಕೆ_ಪುಟ_ಬೈಟುಗಳು > (u64)d_m3_Max_ರೇಖೀಯ_ಸ್ಮರಣೆ_ಪುಟಗಳು * d_m3_Default_Mem_ಪುಟ_ಗಾತ್ರ);
#ಆದರೆ_ಅಂತ್ಯ

        // Limit the amount of memory that gets actually allocated
        ಆದರೆ (io_runtime->ಸ್ಮರಣೆ_ಎಲ್ಲೆ) {
            ಅಂಕೆ_ಪುಟ_ಬೈಟುಗಳು = M3_MIN(ಅಂಕೆ_ಪುಟ_ಬೈಟುಗಳು, (u64)io_runtime->ಸ್ಮರಣೆ_ಎಲ್ಲೆ);
        }

        u64 old_ಉದ್ದಳತೆ = ಸ್ಮರಣೆ->mallocated ? ಸ್ಮರಣೆ->mallocated->ಉದ್ದಳತೆ : 0;
        u64 other_ಬಳಸಿದ = io_runtime->ಸ್ಮರಣೆ_ಬೈಟುಗಳು_ಬಳಸಿದ - old_ಉದ್ದಳತೆ;
        _throwif(m3_Err_ಸ್ಮರಣೆ_ಎಲ್ಲೆ_Exceeded, io_runtime->ಸ್ಮರಣೆ_ಬೈಟುಗಳು_ಎಲ್ಲೆ and
                                              (ಅಂಕೆ_ಪುಟ_ಬೈಟುಗಳು > io_runtime->ಸ್ಮರಣೆ_ಬೈಟುಗಳು_ಎಲ್ಲೆ or
                                               other_ಬಳಸಿದ > io_runtime->ಸ್ಮರಣೆ_ಬೈಟುಗಳು_ಎಲ್ಲೆ - ಅಂಕೆ_ಪುಟ_ಬೈಟುಗಳು));

        _throwif("linear memory limitation exceeded", ಅಂಕೆ_ಪುಟ_ಬೈಟುಗಳು > (u64)SIZE_MAX - ಗಾತ್ರ(M3_ಸ್ಮರಣೆ_ಶೀರ್ಷಿಕೆ));

#ಆದರೆ d_m3_Guarded_ಸ್ಮರಣೆ

        // A slot holds every address a Wasm access can name, and nothing beyond
        // that: a memory larger than the data part of one would put addresses past
        // the guard, where a fault is nobody's to recognize
        _throwif("linear memory limitation exceeded", ಅಂಕೆ_ಪುಟ_ಬೈಟುಗಳು > d_m3_Guarded_ದತ್ತಾಂಶ_ಬೈಟುಗಳು);

        // What makes an access past the end fault is the page after the memory not
        // being committed, so the end has to fall on one of the system's page
        // boundaries. A Wasm page divides evenly into them and always will; a memory
        // that declared a page size of its own may not, and then the bytes between
        // the end of the memory and the end of its last system page would answer
        // instead of trapping. Refusing to back such a memory is the honest way out:
        // the custom page sizes proposal exists for targets far too small to reserve
        // 8GiB per memory in the first place. See d_m3GuardedMemory.
        _throwif("guarded memory needs a page size the system's divides into",
                 (ಸ್ಮರಣೆ->ಪುಟ_ಗಾತ್ರ % m3_Host_ಪುಟ_ಗಾತ್ರ()) != 0);

        ಆದರೆ (ಸ್ಮರಣೆ->guard_ಸ್ಥಾನ == ಖಾಲಿ) {
            ಸ್ಮರಣೆ->guard_ಸ್ಥಾನ = Guard_Take_ಸ್ಥಾನ();
            _throwifnull(ಸ್ಮರಣೆ->guard_ಸ್ಥಾನ);
        }

        // Growing only ever commits more of the same slot, so the memory never
        // moves - which the interpreter does not rely on, but nothing minds
        _throwif(m3Err_mallocFailed, not Guard_Commit_ಸ್ಥಾನ(ಸ್ಮರಣೆ->guard_ಸ್ಥಾನ, (size_t)ಅಂಕೆ_ಪುಟ_ಬೈಟುಗಳು));

        ಸ್ಮರಣೆ->mallocated = Guard_ಸ್ಥಾನ_ಶೀರ್ಷಿಕೆ(ಸ್ಮರಣೆ->guard_ಸ್ಥಾನ);

#ಇಲ್ಲದಿದ್ದರೆ

        size_t ಅಂಕೆ_ಬೈಟುಗಳು = (size_t)ಅಂಕೆ_ಪುಟ_ಬೈಟುಗಳು + ಗಾತ್ರ(M3_ಸ್ಮರಣೆ_ಶೀರ್ಷಿಕೆ);

        // What is really there, which is not numPages * pageSize: the limit above
        // caps the bytes without capping the page count, so a memory can be short
        // of a page - by design, since a whole 64 KiB page is often more than an
        // MCU has. Measuring the old block by the page count would overstate it,
        // and then realloc never sees the size it already has and copies the
        // whole limit again on every grow past it.
        size_t ಅಂಕೆ_ಹಿಂದಿನ_ಬೈಟುಗಳು = ಸ್ಮರಣೆ->mallocated ? ಸ್ಮರಣೆ->mallocated->ಉದ್ದಳತೆ + ಗಾತ್ರ(M3_ಸ್ಮರಣೆ_ಶೀರ್ಷಿಕೆ) : 0;

        ಶೂನ್ಯ* newMem = m3_Realloc("Wasm Linear Memory", ಸ್ಮರಣೆ->mallocated, ಅಂಕೆ_ಬೈಟುಗಳು, ಅಂಕೆ_ಹಿಂದಿನ_ಬೈಟುಗಳು);
        _throwifnull(newMem);

        ಸ್ಮರಣೆ->mallocated = (M3_ಸ್ಮರಣೆ_ಶೀರ್ಷಿಕೆ*)newMem;

#ಆದರೆ_ಅಂತ್ಯ

#ಆದರೆ d_m3LogRuntime
        M3_ಸ್ಮರಣೆ_ಶೀರ್ಷಿಕೆ* oldMallocated = ಸ್ಮರಣೆ->mallocated;
#ಆದರೆ_ಅಂತ್ಯ

        io_runtime->ಸ್ಮರಣೆ_ಬೈಟುಗಳು_ಬಳಸಿದ = other_ಬಳಸಿದ + ಅಂಕೆ_ಪುಟ_ಬೈಟುಗಳು;
        ಸ್ಮರಣೆ->ಅಂಕೆ_ಪುಟಗಳು            = ಅಂಕೆ_ಪುಟಗಳು_ಗಮ್ಯ_Alloc;

        ಸ್ಮರಣೆ->mallocated->ಉದ್ದಳತೆ  = (size_t)ಅಂಕೆ_ಪುಟ_ಬೈಟುಗಳು;
        ಸ್ಮರಣೆ->mallocated->runtime = io_runtime;
        ಸ್ಮರಣೆ->mallocated->ಸ್ಮರಣೆ  = ಸ್ಮರಣೆ;

        ಸ್ಮರಣೆ->mallocated->max_ರಾಶಿ = (m3slot_t*)io_runtime->origin_ರಾಶಿ + io_runtime->ಅಂಕೆ_ರಾಶಿ_Slots;

        m3log(runtime, "resized old: %p; mem: %p; length: %zu; pages: %llu", oldMallocated, ಸ್ಮರಣೆ->mallocated, ಸ್ಮರಣೆ->mallocated->ಉದ್ದಳತೆ, (ಚಿಹ್ನೆರಹಿತ ದೀರ್ಘ ದೀರ್ಘ)ಸ್ಮರಣೆ->ಅಂಕೆ_ಪುಟಗಳು);
    } ಇಲ್ಲದಿದ್ದರೆ {
        ಫಲಿತಾಂಶ = m3_Err_wasm_ಸ್ಮರಣೆ_ಅತಿಪ್ರವಾಹ;
    }

    _catch: ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}


ಶೂನ್ಯ Free_ಸ್ಮರಣೆ_ಖಂಡ (IM3_ಸ್ಮರಣೆ io_ಸ್ಮರಣೆ)
{
    ಆದರೆ (io_ಸ್ಮರಣೆ->mallocated and io_ಸ್ಮರಣೆ->mallocated->runtime) {
        io_ಸ್ಮರಣೆ->mallocated->runtime->ಸ್ಮರಣೆ_ಬೈಟುಗಳು_ಬಳಸಿದ -= io_ಸ್ಮರಣೆ->mallocated->ಉದ್ದಳತೆ;
    }
#ಆದರೆ d_m3_Guarded_ಸ್ಮರಣೆ
    Guard_Give_ಸ್ಥಾನ(io_ಸ್ಮರಣೆ->guard_ಸ್ಥಾನ);
    io_ಸ್ಮರಣೆ->guard_ಸ್ಥಾನ = ಖಾಲಿ;

    io_ಸ್ಮರಣೆ->mallocated = ಖಾಲಿ;
#ಇಲ್ಲದಿದ್ದರೆ
    m3_Free(io_ಸ್ಮರಣೆ->mallocated);
#ಆದರೆ_ಅಂತ್ಯ
}


M3_ಫಲಿತಾಂಶ InitGlobals (IM3Module io_module)
{
    M3_ಫಲಿತಾಂಶ ಫಲಿತಾಂಶ = m3Err_none;

    ಆದರೆ (io_module->ಅಂಕೆ_Globals) {
        // placing the globals in their structs isn't good for cache locality, but i don't really know what the global
        // access patterns typically look like yet.

        //          io_module->globalMemory = m3Alloc (m3reg_t, io_module->numGlobals);

        //          if (io_module->globalMemory)
        {
            ಗಾಗಿ (u32 i = 0; i < io_module->ಅಂಕೆ_Globals; ++i) {
                M3Global* g = &io_module->globals[i];                           m3log (runtime, "initializing global: %d", i);

                ಆದರೆ (g->init_ಅಭಿವ್ಯಕ್ತಿ) {
                    ಬೈಟುಗಳು_t ಆರಂಭ = g->init_ಅಭಿವ್ಯಕ್ತಿ;

                    ಫಲಿತಾಂಶ = Evaluate_ಅಭಿವ್ಯಕ್ತಿ(io_module, &g->i64_ಮೌಲ್ಯ, g->type, &ಆರಂಭ, g->init_ಅಭಿವ್ಯಕ್ತಿ + g->init_ಅಭಿವ್ಯಕ್ತಿ_ಗಾತ್ರ);

                    ಆದರೆ (not ಫಲಿತಾಂಶ) {
                        // io_module->globalMemory [i] = initValue;
                    } ಇಲ್ಲದಿದ್ದರೆ {
                        ನಿಲ್ಲಿಸು;
                    }
                } ಇಲ್ಲದಿದ್ದರೆ {                                                        m3log (runtime, "importing global");
                }
            }
        }
        //          else result = ErrorModule (m3Err_mallocFailed, io_module, "could allocate globals for module: '%s", io_module->name);
    }

    ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}


M3_ಫಲಿತಾಂಶ Init_ದತ್ತಾಂಶ_Segments (IM3Module io_module)
{
    M3_ಫಲಿತಾಂಶ ಫಲಿತಾಂಶ = m3Err_none;

    ಗಾಗಿ (u32 i = 0; i < io_module->ಅಂಕೆ_ದತ್ತಾಂಶ_Segments; ++i) {
        M3_ದತ್ತಾಂಶ_Segment* segment = &io_module->ದತ್ತಾಂಶ_Segments[i];

        // A passive segment stays available for memory.init until data.drop.
        // An active one is copied here and then counts as dropped.
        ಆದರೆ (segment->isPassive) {
            ಮುಂದುವರಿ;
        }

        _throwif("data segment memory index out of range",
                 segment->ಸ್ಮರಣೆ_Region >= io_module->ಅಂಕೆ_Memories);

        IM3_ಸ್ಮರಣೆ io_ಸ್ಮರಣೆ = io_module->memories[segment->ಸ್ಮರಣೆ_Region];

        _throwif("unallocated linear memory", !(io_ಸ್ಮರಣೆ->mallocated));

        // The offset expression has the memory's address type, and is
        // unsigned: an i32 offset of -1 is 4294967295, way out of bounds
        // rather than negative.
        u64     segment_ಸ್ಥಾನಾಂತರ = 0;
        ಬೈಟುಗಳು_t ಆರಂಭ         = segment->init_ಅಭಿವ್ಯಕ್ತಿ;

        ಆದರೆ (io_ಸ್ಮರಣೆ->isMemory64) {
_           (Evaluate_ಅಭಿವ್ಯಕ್ತಿ(io_module, &segment_ಸ್ಥಾನಾಂತರ, c_m3Type_i64, &ಆರಂಭ, segment->init_ಅಭಿವ್ಯಕ್ತಿ + segment->init_ಅಭಿವ್ಯಕ್ತಿ_ಗಾತ್ರ));
        } ಇಲ್ಲದಿದ್ದರೆ {
            u32 offset32;
_           (Evaluate_ಅಭಿವ್ಯಕ್ತಿ(io_module, &offset32, c_m3Type_i32, &ಆರಂಭ, segment->init_ಅಭಿವ್ಯಕ್ತಿ + segment->init_ಅಭಿವ್ಯಕ್ತಿ_ಗಾತ್ರ));
            segment_ಸ್ಥಾನಾಂತರ = offset32;
        }

        m3log(runtime, "loading data segment: %d; size: %d; offset: %llu", i, segment->ಗಾತ್ರ_2, (ಚಿಹ್ನೆರಹಿತ ದೀರ್ಘ ದೀರ್ಘ)segment_ಸ್ಥಾನಾಂತರ);

        ಆದರೆ (segment_ಸ್ಥಾನಾಂತರ <= io_ಸ್ಮರಣೆ->mallocated->ಉದ್ದಳತೆ &&
            (u64)segment->ಗಾತ್ರ_2 <= io_ಸ್ಮರಣೆ->mallocated->ಉದ್ದಳತೆ - segment_ಸ್ಥಾನಾಂತರ) {
            u8* dest = m3_Mem_ದತ್ತಾಂಶ(io_ಸ್ಮರಣೆ->mallocated) + segment_ಸ್ಥಾನಾಂತರ;
            ಸ್ಮರಣೆ_ನಕಲು(dest, segment->ದತ್ತಾಂಶ, segment->ಗಾತ್ರ_2);
        } ಇಲ್ಲದಿದ್ದರೆ {
            _throw("data segment out of bounds");
        }

        segment->dropped = true;
    }

    _catch: ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}


// Turns a segment's elements into references. Element expressions are constant
// expressions restricted to ref.null/ref.func, so they're read directly rather
// than run through the compiler.
ಸ್ಥಿರ
M3_ಫಲಿತಾಂಶ ResolveElements (IM3Module io_module, M3ElementSegment* i_segment, ಶೂನ್ಯ** o_elements)
{
    M3_ಫಲಿತಾಂಶ ಫಲಿತಾಂಶ = m3Err_none;

    ಬೈಟುಗಳು_t  pos = i_segment->elements;
    cbytes_t ಕೊನೆ = io_module->element_Section_ಕೊನೆ;

    ಗಾಗಿ (u32 e = 0; e < i_segment->ಅಂಕೆ_Elements; ++e) {
        u32   func_ಸೂಚ್ಯಂಕ;
        ಶೂನ್ಯ* ref = ಖಾಲಿ;

        ಆದರೆ (i_segment->is_ಅಭಿವ್ಯಕ್ತಿ) {
            m3opcode_t opcode;
_           (Read_opcode(&opcode, &pos, ಕೊನೆ));

            ಆದರೆ (opcode == c_waOp_refFunc) {
_               (ReadLEB_u32(&func_ಸೂಚ್ಯಂಕ, &pos, ಕೊನೆ));
                _throwif("function index out of range", func_ಸೂಚ್ಯಂಕ >= io_module->ಅಂಕೆ_Functions);
                ref = Function_Implementation(&io_module->functions[func_ಸೂಚ್ಯಂಕ]);
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (opcode == c_waOp_refNull) {
                i8 waType;
                u8 nullType;
_               (ReadLEB_i7(&waType, &pos, ಕೊನೆ));
_               (NormalizeType(&nullType, waType));
                _throwif(m3Err_typeMismatch, nullType != i_segment->type);
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (opcode == c_waOp_getGlobal) {
                // wasm 2.0 lets an element expression read an imported
                // immutable global, which is how one module seeds another's
                // table with a reference it exported.
                u32 global_ಸೂಚ್ಯಂಕ;
_               (ReadLEB_u32(&global_ಸೂಚ್ಯಂಕ, &pos, ಕೊನೆ));
                _throwif(m3_Err_globa_ಸೂಚ್ಯಂಕ_ಹೊರಹೋಗು_ದ_Bounds, global_ಸೂಚ್ಯಂಕ >= io_module->ಅಂಕೆ_Globals);

                IM3Global global = &io_module->globals[global_ಸೂಚ್ಯಂಕ];

                _throwif(m3_Err_globa_ಸೂಚ್ಯಂಕ_ಹೊರಹೋಗು_ದ_Bounds, not global->imported);
                _throwif(m3Err_wasmMalformed, global->isMutable);
                _throwif(m3Err_typeMismatch, ತಳ_Type_ದ(global->type) != ತಳ_Type_ದ(i_segment->type));

                // read the cell the import was linked to, not the placeholder
                ಆದರೆ (global->resolved) {
                    global = global->resolved;
                }

                ref = global->ref_ಮೌಲ್ಯ;
            } ಇಲ್ಲದಿದ್ದರೆ {
                _throw("constant expression required");
            }

_           (Read_opcode(&opcode, &pos, ಕೊನೆ));
            _throwif(m3Err_wasmMalformed, opcode != c_wa_Op_ಕೊನೆ);
        } ಇಲ್ಲದಿದ್ದರೆ {
_           (ReadLEB_u32(&func_ಸೂಚ್ಯಂಕ, &pos, ಕೊನೆ));
            _throwif("function index out of range", func_ಸೂಚ್ಯಂಕ >= io_module->ಅಂಕೆ_Functions);
            ref = Function_Implementation(&io_module->functions[func_ಸೂಚ್ಯಂಕ]);
        }

        o_elements[e] = ref;
    }

    _catch: ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}


M3_ಫಲಿತಾಂಶ Init_ಕೋಷ್ಟಕ_And_Elements (IM3Module io_module)
{
    M3_ಫಲಿತಾಂಶ ಫಲಿತಾಂಶ = m3Err_none;

    cbytes_t ಕೊನೆ = io_module->element_Section_ಕೊನೆ;
    M3_ಕೋಷ್ಟಕ* ಕೋಷ್ಟಕ_2;

    ಗಾಗಿ (u32 i = 0; i < io_module->ಅಂಕೆ_Tables; ++i) {
        ಕೋಷ್ಟಕ_2 = io_module->tables[i];

        // a slot pointing at another module's table is that module's to fill
        ಆದರೆ (ಕೋಷ್ಟಕ_2->owner != io_module) {
            ಮುಂದುವರಿ;
        }

        ಆದರೆ (ಕೋಷ್ಟಕ_2->ಗಾತ್ರ_2) {
            IM3Runtime runtime = io_module->runtime;
            _throwif(m3_Err_ಕೋಷ್ಟಕ_ಎಲ್ಲೆ_Exceeded, runtime->ಕೋಷ್ಟಕ_Elements_ಎಲ್ಲೆ and
                                                 ಕೋಷ್ಟಕ_2->ಗಾತ್ರ_2 > runtime->ಕೋಷ್ಟಕ_Elements_ಎಲ್ಲೆ - runtime->ಕೋಷ್ಟಕ_Elements_ಬಳಸಿದ);
            ಕೋಷ್ಟಕ_2->elements = m3_AllocArray(ಶೂನ್ಯ*, ಕೋಷ್ಟಕ_2->ಗಾತ್ರ_2);
            _throwifnull(ಕೋಷ್ಟಕ_2->elements);
            runtime->ಕೋಷ್ಟಕ_Elements_ಬಳಸಿದ += ಕೋಷ್ಟಕ_2->ಗಾತ್ರ_2;

            ಆದರೆ (ಕೋಷ್ಟಕ_2->init_ಅಭಿವ್ಯಕ್ತಿ) {
                ಶೂನ್ಯ*   ಮೌಲ್ಯ = ಖಾಲಿ;
                ಬೈಟುಗಳು_t ಆರಂಭ = ಕೋಷ್ಟಕ_2->init_ಅಭಿವ್ಯಕ್ತಿ;
_               (Evaluate_ಅಭಿವ್ಯಕ್ತಿ(io_module, &ಮೌಲ್ಯ, ತಳ_Type_ದ(ಕೋಷ್ಟಕ_2->type),
                                     &ಆರಂಭ, ಕೋಷ್ಟಕ_2->init_ಅಭಿವ್ಯಕ್ತಿ + ಕೋಷ್ಟಕ_2->init_ಅಭಿವ್ಯಕ್ತಿ_ಗಾತ್ರ));

                ಗಾಗಿ (u32 e = 0; e < ಕೋಷ್ಟಕ_2->ಗಾತ್ರ_2; ++e) {
                    ಕೋಷ್ಟಕ_2->elements[e] = ಮೌಲ್ಯ;
                }
            }
        }
    }

    ಗಾಗಿ (u32 i = 0; i < io_module->ಅಂಕೆ_Element_Segments; ++i) {
        M3ElementSegment* segment = &io_module->elementSegments[i];

        // Declarative segments only make their functions referenceable, and
        // passive ones wait for table.init, so neither is written out here.
        ಆದರೆ (segment->ಬಹುಲಕ == c_m3Elem_declarative) {
            segment->dropped = true;
            ಮುಂದುವರಿ;
        }

        ಆದರೆ (segment->ಬಹುಲಕ == c_m3Elem_passive) {
            ಆದರೆ (segment->ಅಂಕೆ_Elements) {
                segment->resolved = m3_AllocArray(ಶೂನ್ಯ*, segment->ಅಂಕೆ_Elements);
                _throwifnull(segment->resolved);
_               (ResolveElements(io_module, segment, segment->resolved));
            }
            ಮುಂದುವರಿ;
        }

        ಕೋಷ್ಟಕ_2 = io_module->tables[segment->ಕೋಷ್ಟಕ_ಸೂಚ್ಯಂಕ];

        // The offset expression has the table's index type, and is unsigned:
        // an i32 offset of -1 is 4294967295, out of bounds rather than negative.
        u64     offset = 0;
        ಬೈಟುಗಳು_t ಅಭಿವ್ಯಕ್ತಿ   = segment->init_ಅಭಿವ್ಯಕ್ತಿ;

        ಆದರೆ (ಕೋಷ್ಟಕ_2->isTable64) {
_           (Evaluate_ಅಭಿವ್ಯಕ್ತಿ(io_module, &offset, c_m3Type_i64, &ಅಭಿವ್ಯಕ್ತಿ, ಕೊನೆ));
        } ಇಲ್ಲದಿದ್ದರೆ {
            u32 offset32;
_           (Evaluate_ಅಭಿವ್ಯಕ್ತಿ(io_module, &offset32, c_m3Type_i32, &ಅಭಿವ್ಯಕ್ತಿ, ಕೊನೆ));
            offset = offset32;
        }

        _throwif("out of bounds table access",
                 offset > ಕೋಷ್ಟಕ_2->ಗಾತ್ರ_2 or segment->ಅಂಕೆ_Elements > ಕೋಷ್ಟಕ_2->ಗಾತ್ರ_2 - offset);

_       (ResolveElements(io_module, segment, ಕೋಷ್ಟಕ_2->elements + offset));

        segment->dropped = true;
    }

    _catch: ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}

M3_ಫಲಿತಾಂಶ m3_CompileModule (IM3Module io_module)
{
    M3_ಫಲಿತಾಂಶ ಫಲಿತಾಂಶ = m3Err_none;

    ಗಾಗಿ (u32 i = 0; i < io_module->ಅಂಕೆ_Functions; ++i) {
        IM3Function f = &io_module->functions[i];
        ಆದರೆ (f->wasm and not f->compiled) {
_           (CompileFunction(f));
        }
    }

    _catch: ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}

#ಆದರೆ d_m3HasExceptionHandling

M3Exception* NewException (IM3Runtime io_runtime, IM3_ಟ್ಯಾಗ್ i_ಟ್ಯಾಗ್, u32 i_ಅಂಕೆ_ವಾದಗಳು)
{
    M3Exception* exception = (M3Exception*)m3_Malloc("M3Exception", ಗಾತ್ರ(M3Exception) + i_ಅಂಕೆ_ವಾದಗಳು * ಗಾತ್ರ(u64));

    ಆದರೆ (exception) {
        exception->ಟ್ಯಾಗ್     = i_ಟ್ಯಾಗ್;
        exception->ಅಂಕೆ_ವಾದಗಳು = i_ಅಂಕೆ_ವಾದಗಳು;
        exception->reified = false;
        exception->prev    = ಖಾಲಿ;
        exception->ಮುಂದಿನ    = io_runtime->exceptions;

        ಆದರೆ (exception->ಮುಂದಿನ) {
            exception->ಮುಂದಿನ->prev = exception;
        }

        io_runtime->exceptions = exception;
    }

    ಹಿಂತಿರುಗಿ exception;
}


// Releases one exception ahead of the rest. The caller has to know nothing can
// still name it: no exnref was ever taken of it, and its payload has already
// been copied out.
ಶೂನ್ಯ FreeException (IM3Runtime io_runtime, M3Exception* i_exception)
{
    ಆದರೆ (i_exception->prev) {
        i_exception->prev->ಮುಂದಿನ = i_exception->ಮುಂದಿನ;
    } ಇಲ್ಲದಿದ್ದರೆ {
        io_runtime->exceptions = i_exception->ಮುಂದಿನ;
    }

    ಆದರೆ (i_exception->ಮುಂದಿನ) {
        i_exception->ಮುಂದಿನ->prev = i_exception->prev;
    }

    ಆದರೆ (io_runtime->pendingException == i_exception) {
        io_runtime->pendingException = ಖಾಲಿ;
    }

    m3_Free_Impl(i_exception);
}


// Releases every exception the runtime still holds. Only safe once the Wasm
// stack is empty, which is why the outermost RunCodeChecked() is the one that
// calls it.
ಶೂನ್ಯ FreeExceptions (IM3Runtime io_runtime)
{
    M3Exception* exception = io_runtime->exceptions;

    io_runtime->exceptions       = ಖಾಲಿ;
    io_runtime->pendingException = ಖಾಲಿ;

    ಆಗ (exception) {
        M3Exception* ಮುಂದಿನ = exception->ಮುಂದಿನ;
        m3_Free_Impl(exception);
        exception = ಮುಂದಿನ;
    }
}


// Keeps a copy of the exception a call just ended with, for m3_GetExceptionTag,
// or forgets the last one when the call ended any other way. A copy, because
// the original is freed with the rest before the outermost call returns - and
// one a host function's nested call ended with may still be named by an
// exnref further down the stack.
ಸ್ಥಿರ
ಶೂನ್ಯ ದಾಖಲೆ_Uncaught_Exception (IM3Runtime io_runtime, M3_ಫಲಿತಾಂಶ i_ಫಲಿತಾಂಶ)
{
    ಆದರೆ (io_runtime->uncaughtException) {
        m3_Free(io_runtime->uncaughtException);
    }

    M3Exception* exception = io_runtime->pendingException;

    ಆದರೆ (M3_UNLIKELY(i_ಫಲಿತಾಂಶ == m3Err_trapUncaughtException) and exception) {
        M3Exception* copy = (M3Exception*)m3_CopyMem(exception, ಗಾತ್ರ(M3Exception) + exception->ಅಂಕೆ_ವಾದಗಳು * ಗಾತ್ರ(u64));

        ಆದರೆ (copy) {
            copy->ಮುಂದಿನ = ಖಾಲಿ;
            copy->prev = ಖಾಲಿ;
        }

        io_runtime->uncaughtException = copy;
    }
}

#ಆದರೆ_ಅಂತ್ಯ // d_m3HasExceptionHandling


#ಆದರೆ d_m3HasExceptionHandling || d_m3_Has_ರಾಶಿ_Switching

IM3_ಟ್ಯಾಗ್ m3_ಕಂಡುಹಿಡಿ_ಟ್ಯಾಗ್ (IM3Module i_module, ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* ಸ್ಥಿರಾಂಕ i_ಟ್ಯಾಗ್_Name)
{
    ಆದರೆ (not i_module or not i_ಟ್ಯಾಗ್_Name) {
        ಹಿಂತಿರುಗಿ ಖಾಲಿ;
    }

    ಹಿಂತಿರುಗಿ Module_ಕಂಡುಹಿಡಿ_Exported_ಟ್ಯಾಗ್(i_module, i_ಟ್ಯಾಗ್_Name);
}


M3_ಫಲಿತಾಂಶ m3_New_ಟ್ಯಾಗ್ (IM3Runtime io_runtime, IM3_ಟ್ಯಾಗ್* o_ಟ್ಯಾಗ್, ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* ಸ್ಥಿರಾಂಕ i_signature)
{
    IM3FuncType type    = ಖಾಲಿ;
    M3_Host_ಟ್ಯಾಗ್*  host_ಟ್ಯಾಗ್ = ಖಾಲಿ;

_try {
    _throwif("null tag", not o_ಟ್ಯಾಗ್);
    *o_ಟ್ಯಾಗ್ = ಖಾಲಿ;
    _throwif(m3Err_mallocFailed, not io_runtime);

_   (Signature_ಗಮ್ಯ_Func_Type(&type, i_signature));
    // tags are the same type exactly when their types are the same object
_   (Environment_ಕೂಡಿಸು_Func_Type(io_runtime->environment, &type));

    host_ಟ್ಯಾಗ್ = m3_AllocStruct(M3_Host_ಟ್ಯಾಗ್);
    _throwifnull(host_ಟ್ಯಾಗ್);

    host_ಟ್ಯಾಗ್->ಟ್ಯಾಗ್.type    = type;
    host_ಟ್ಯಾಗ್->ಮುಂದಿನ        = io_runtime->host_ಟ್ಯಾಗುಗಳು;
    io_runtime->host_ಟ್ಯಾಗುಗಳು = host_ಟ್ಯಾಗ್;

    *o_ಟ್ಯಾಗ್ = &host_ಟ್ಯಾಗ್->ಟ್ಯಾಗ್;
} _catch:
    ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}


M3_ಫಲಿತಾಂಶ m3_Link_ಟ್ಯಾಗ್ (IM3Module         io_module,
                     ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* ಸ್ಥಿರಾಂಕ i_moduleName,
                     ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* ಸ್ಥಿರಾಂಕ i_ಟ್ಯಾಗ್_Name,
                     IM3_ಟ್ಯಾಗ್            i_ಟ್ಯಾಗ್)
{
    ಆದರೆ (not io_module or not i_moduleName or not i_ಟ್ಯಾಗ್_Name or not i_ಟ್ಯಾಗ್) {
        ಹಿಂತಿರುಗಿ m3_Err_ಟ್ಯಾಗ್_Lookup_Failed;
    }

    ಸ್ಥಿರಾಂಕ bool wildcardModule = (strcmp(i_moduleName, "*") == 0);
    IM3_ಟ್ಯಾಗ್     ಗುರಿ         = i_ಟ್ಯಾಗ್->resolved ? i_ಟ್ಯಾಗ್->resolved : i_ಟ್ಯಾಗ್;

    M3_ಫಲಿತಾಂಶ ಫಲಿತಾಂಶ = m3_Err_ಟ್ಯಾಗ್_Lookup_Failed;

    ಗಾಗಿ (u32 i = 0; i < io_module->ಅಂಕೆ_ಟ್ಯಾಗುಗಳು; ++i) {
        IM3_ಟ್ಯಾಗ್ ಟ್ಯಾಗ್ = &io_module->ಟ್ಯಾಗುಗಳು[i];

        ಆದರೆ (ಟ್ಯಾಗ್->imported and ಟ್ಯಾಗ್->import.moduleUtf8 and ಟ್ಯಾಗ್->import.ಕ್ಷೇತ್ರ_Utf8 and
            strcmp(ಟ್ಯಾಗ್->import.ಕ್ಷೇತ್ರ_Utf8, i_ಟ್ಯಾಗ್_Name) == 0 and
            (wildcardModule or strcmp(ಟ್ಯಾಗ್->import.moduleUtf8, i_moduleName) == 0)) {
            ಆದರೆ (ಟ್ಯಾಗ್->type != ಗುರಿ->type) {
                ಹಿಂತಿರುಗಿ m3Err_incompatibleImportType;
            }

            ಟ್ಯಾಗ್->resolved = ಗುರಿ;
            ಫಲಿತಾಂಶ        = m3Err_none;
        }
    }

    ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}


uint32_t m3_Get_ಟ್ಯಾಗ್_Arg_ಎಣಿಕೆ (IM3_ಟ್ಯಾಗ್ i_ಟ್ಯಾಗ್)
{
    ಹಿಂತಿರುಗಿ i_ಟ್ಯಾಗ್ ? i_ಟ್ಯಾಗ್->type->ಅಂಕೆ_ವಾದಗಳು : 0;
}


M3_ಮೌಲ್ಯ_Type m3_Get_ಟ್ಯಾಗ್_Arg_Type (IM3_ಟ್ಯಾಗ್ i_ಟ್ಯಾಗ್, uint32_t i_ಸೂಚ್ಯಂಕ)
{
    ಆದರೆ (i_ಟ್ಯಾಗ್ and i_ಸೂಚ್ಯಂಕ < i_ಟ್ಯಾಗ್->type->ಅಂಕೆ_ವಾದಗಳು) {
        ಹಿಂತಿರುಗಿ (M3_ಮೌಲ್ಯ_Type)ತಳ_Type_ದ(d_FuncArgType(i_ಟ್ಯಾಗ್->type, i_ಸೂಚ್ಯಂಕ));
    }

    ಹಿಂತಿರುಗಿ c_m3Type_none;
}

#ಇಲ್ಲದಿದ್ದರೆ

IM3_ಟ್ಯಾಗ್ m3_ಕಂಡುಹಿಡಿ_ಟ್ಯಾಗ್ (IM3Module i_module, ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* ಸ್ಥಿರಾಂಕ i_ಟ್ಯಾಗ್_Name)
{
    ಹಿಂತಿರುಗಿ ಖಾಲಿ;
}

M3_ಫಲಿತಾಂಶ m3_New_ಟ್ಯಾಗ್ (IM3Runtime io_runtime, IM3_ಟ್ಯಾಗ್* o_ಟ್ಯಾಗ್, ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* ಸ್ಥಿರಾಂಕ i_signature)
{
    ಆದರೆ (o_ಟ್ಯಾಗ್) {
        *o_ಟ್ಯಾಗ್ = ಖಾಲಿ;
    }
    ಹಿಂತಿರುಗಿ m3_Err_ಟ್ಯಾಗ್_Lookup_Failed;
}

M3_ಫಲಿತಾಂಶ m3_Link_ಟ್ಯಾಗ್ (IM3Module io_module, ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* ಸ್ಥಿರಾಂಕ i_moduleName, ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* ಸ್ಥಿರಾಂಕ i_ಟ್ಯಾಗ್_Name, IM3_ಟ್ಯಾಗ್ i_ಟ್ಯಾಗ್)
{
    ಹಿಂತಿರುಗಿ m3_Err_ಟ್ಯಾಗ್_Lookup_Failed;
}

uint32_t m3_Get_ಟ್ಯಾಗ್_Arg_ಎಣಿಕೆ (IM3_ಟ್ಯಾಗ್ i_ಟ್ಯಾಗ್)
{
    ಹಿಂತಿರುಗಿ 0;
}

M3_ಮೌಲ್ಯ_Type m3_Get_ಟ್ಯಾಗ್_Arg_Type (IM3_ಟ್ಯಾಗ್ i_ಟ್ಯಾಗ್, uint32_t i_ಸೂಚ್ಯಂಕ)
{
    ಹಿಂತಿರುಗಿ c_m3Type_none;
}

#ಆದರೆ_ಅಂತ್ಯ // d_m3HasExceptionHandling || d_m3HasStackSwitching


#ಆದರೆ d_m3HasExceptionHandling

M3_ಫಲಿತಾಂಶ m3_ThrowException (IM3Runtime io_runtime, IM3_ಟ್ಯಾಗ್ i_ಟ್ಯಾಗ್, uint32_t i_argc, ಸ್ಥಿರಾಂಕ ಶೂನ್ಯ* i_argptrs[])
{
    ಆದರೆ (not io_runtime or not i_ಟ್ಯಾಗ್) {
        ಹಿಂತಿರುಗಿ m3_Err_ಟ್ಯಾಗ್_Lookup_Failed;
    }

    IM3_ಟ್ಯಾಗ್      ಟ್ಯಾಗ್  = i_ಟ್ಯಾಗ್->resolved ? i_ಟ್ಯಾಗ್->resolved : i_ಟ್ಯಾಗ್;
    IM3FuncType type = ಟ್ಯಾಗ್->type;

    // a tag with results is a stack switching control tag, which is suspended
    // with rather than thrown
    ಆದರೆ (type->ಅಂಕೆ_Rets) {
        ಹಿಂತಿರುಗಿ m3Err_typeMismatch;
    }
    ಆದರೆ (i_argc != type->ಅಂಕೆ_ವಾದಗಳು) {
        ಹಿಂತಿರುಗಿ m3_Err_ವಾದ_ಎಣಿಕೆ_Mismatch;
    }
    // with nothing on the stack there is nothing to unwind, and the exception
    // would sit on the runtime's list until some later call ended
    ಆದರೆ (io_runtime->callNesting == 0) {
        ಹಿಂತಿರುಗಿ "m3_ThrowException called outside a host function";
    }

    M3Exception* exception = NewException(io_runtime, ಟ್ಯಾಗ್, i_argc);
    ಆದರೆ (not exception) {
        ಹಿಂತಿರುಗಿ m3Err_mallocFailed;
    }

    // the layout op_Throw gives a payload: 32-bit values zero-extended, floats
    // as their bit pattern
    ಗಾಗಿ (u32 i = 0; i < i_argc; ++i) {
        ಸ್ಥಿರಾಂಕ ಶೂನ್ಯ* ಮೌಲ್ಯ = i_argptrs[i];

        ಆಯ್ಕೆ (ತಳ_Type_ದ(d_FuncArgType(type, i))) {
        ಸ್ಥಿತಿ c_m3Type_i32:
        ಸ್ಥಿತಿ c_m3Type_f32: {
            u32 ಬಿಟ್ಟುಗಳು;
            ಸ್ಮರಣೆ_ನಕಲು(&ಬಿಟ್ಟುಗಳು, ಮೌಲ್ಯ, ಗಾತ್ರ(ಬಿಟ್ಟುಗಳು));
            exception->ವಾದಗಳು[i] = ಬಿಟ್ಟುಗಳು;
            ನಿಲ್ಲಿಸು;
        }
        ಸ್ಥಿತಿ c_m3Type_i64:
        ಸ್ಥಿತಿ c_m3Type_f64:
            ಸ್ಮರಣೆ_ನಕಲು(&exception->ವಾದಗಳು[i], ಮೌಲ್ಯ, ಗಾತ್ರ(u64));
            ನಿಲ್ಲಿಸು;
        ಸ್ಥಿತಿ c_m3Type_funcref:
        ಸ್ಥಿತಿ c_m3Type_externref:
        ಸ್ಥಿತಿ c_m3Type_exnref:
        ಸ್ಥಿತಿ c_m3Type_contref:
            exception->ವಾದಗಳು[i] = (u64)(*(ಸ್ಥಿರಾಂಕ uintptr_t*)ಮೌಲ್ಯ);
            ನಿಲ್ಲಿಸು;
        ಪೂರ್ವನಿಯೋಜಿತ:
            FreeException(io_runtime, exception);
            ಹಿಂತಿರುಗಿ "unknown argument type";
        }
    }

    io_runtime->pendingException = exception;

    ಹಿಂತಿರುಗಿ m3Err_pendingException;
}


IM3_ಟ್ಯಾಗ್ m3_Get_Exception_ಟ್ಯಾಗ್ (IM3Runtime i_runtime)
{
    ಆದರೆ (i_runtime and i_runtime->uncaughtException) {
        ಹಿಂತಿರುಗಿ i_runtime->uncaughtException->ಟ್ಯಾಗ್;
    }

    ಹಿಂತಿರುಗಿ ಖಾಲಿ;
}


M3_ಫಲಿತಾಂಶ m3_Get_Exception_ವಾದಗಳು (IM3Runtime i_runtime, uint32_t i_argc, ಸ್ಥಿರಾಂಕ ಶೂನ್ಯ* o_argptrs[])
{
    ಆದರೆ (not i_runtime or not i_runtime->uncaughtException) {
        ಹಿಂತಿರುಗಿ "no uncaught exception";
    }

    ಸ್ಥಿರಾಂಕ M3Exception* exception = i_runtime->uncaughtException;
    IM3FuncType        type      = exception->ಟ್ಯಾಗ್->type;

    ಆದರೆ (i_argc != exception->ಅಂಕೆ_ವಾದಗಳು) {
        ಹಿಂತಿರುಗಿ m3_Err_ವಾದ_ಎಣಿಕೆ_Mismatch;
    }

    ಗಾಗಿ (u32 i = 0; i < i_argc; ++i) {
        ಶೂನ್ಯ* ಮೌಲ್ಯ = (ಶೂನ್ಯ*)o_argptrs[i];
        u64   ಬಿಟ್ಟುಗಳು  = exception->ವಾದಗಳು[i];

        ಆಯ್ಕೆ (ತಳ_Type_ದ(d_FuncArgType(type, i))) {
        ಸ್ಥಿತಿ c_m3Type_i32:
        ಸ್ಥಿತಿ c_m3Type_f32: {
            u32 ಕೆಳಗಿನ = (u32)ಬಿಟ್ಟುಗಳು;
            ಸ್ಮರಣೆ_ನಕಲು(ಮೌಲ್ಯ, &ಕೆಳಗಿನ, ಗಾತ್ರ(ಕೆಳಗಿನ));
            ನಿಲ್ಲಿಸು;
        }
        ಸ್ಥಿತಿ c_m3Type_i64:
        ಸ್ಥಿತಿ c_m3Type_f64:
            ಸ್ಮರಣೆ_ನಕಲು(ಮೌಲ್ಯ, &ಬಿಟ್ಟುಗಳು, ಗಾತ್ರ(ಬಿಟ್ಟುಗಳು));
            ನಿಲ್ಲಿಸು;
        ಸ್ಥಿತಿ c_m3Type_funcref:
        ಸ್ಥಿತಿ c_m3Type_externref:
        ಸ್ಥಿತಿ c_m3Type_exnref:
        ಸ್ಥಿತಿ c_m3Type_contref:
            *(uintptr_t*)ಮೌಲ್ಯ = (uintptr_t)ಬಿಟ್ಟುಗಳು;
            ನಿಲ್ಲಿಸು;
        ಪೂರ್ವನಿಯೋಜಿತ:
            ಹಿಂತಿರುಗಿ "unknown argument type";
        }
    }

    ಹಿಂತಿರುಗಿ m3Err_none;
}

#ಇಲ್ಲದಿದ್ದರೆ

M3_ಫಲಿತಾಂಶ m3_ThrowException (IM3Runtime io_runtime, IM3_ಟ್ಯಾಗ್ i_ಟ್ಯಾಗ್, uint32_t i_argc, ಸ್ಥಿರಾಂಕ ಶೂನ್ಯ* i_argptrs[])
{
    ಹಿಂತಿರುಗಿ m3_Err_ಟ್ಯಾಗ್_Lookup_Failed;
}

IM3_ಟ್ಯಾಗ್ m3_Get_Exception_ಟ್ಯಾಗ್ (IM3Runtime i_runtime)
{
    ಹಿಂತಿರುಗಿ ಖಾಲಿ;
}

M3_ಫಲಿತಾಂಶ m3_Get_Exception_ವಾದಗಳು (IM3Runtime i_runtime, uint32_t i_argc, ಸ್ಥಿರಾಂಕ ಶೂನ್ಯ* o_argptrs[])
{
    ಹಿಂತಿರುಗಿ "no uncaught exception";
}

#ಆದರೆ_ಅಂತ್ಯ // d_m3HasExceptionHandling


#ಆದರೆ d_m3_Guarded_ಸ್ಮರಣೆ

// What RunCodeProtected hands to the platform layer to run, and what comes back
ಪ್ರಕಾರವ್ಯಾಖ್ಯೆ ರಚನೆ M3_Protected_ಸತತ {
    pc_t            pc;
    IM3Runtime      runtime;
    M3_ಸ್ಮರಣೆ_ಶೀರ್ಷಿಕೆ* mem;
    M3_ಫಲಿತಾಂಶ        ಫಲಿತಾಂಶ;
} M3_Protected_ಸತತ;

ಸ್ಥಿರ
ಶೂನ್ಯ ಸತತ_ಸಂಕೇತ_Body (ಶೂನ್ಯ* io_context)
{
    M3_Protected_ಸತತ* ಸತತ = (M3_Protected_ಸತತ*)io_context;

#  ಆದರೆ (d_m3EnableOpProfiling || d_m3EnableOpTracing)
    ಸತತ->ಫಲಿತಾಂಶ = (M3_ಫಲಿತಾಂಶ)ಸತತ_ಸಂಕೇತ(ಸತತ->pc, (m3stack_t)ಸತತ->runtime->ರಾಶಿ, ಸತತ->mem,
                                    d_m3_Op_Default_ವಾದಗಳು, d_m3_ತಳ_Cstr);
#  ಇಲ್ಲದಿದ್ದರೆ
    ಸತತ->ಫಲಿತಾಂಶ = (M3_ಫಲಿತಾಂಶ)ಸತತ_ಸಂಕೇತ(ಸತತ->pc, (m3stack_t)ಸತತ->runtime->ರಾಶಿ, ಸತತ->mem,
                                    d_m3_Op_Default_ವಾದಗಳು);
#  endif
}

// The bounds check the load and store operations no longer do, done once here by the
// system instead: an address past the end of a memory reaches the uncommitted part
// of that memory's reservation, and the fault comes back as the trap the spec asks
// for. Nothing is left half-done by it - the Wasm stack and the memories are heap,
// and the caller unwinds the rest.
ಸ್ಥಿರ
M3_ಫಲಿತಾಂಶ ಸತತ_ಸಂಕೇತ_Protected (IM3Runtime i_runtime, pc_t i_pc, M3_ಸ್ಮರಣೆ_ಶೀರ್ಷಿಕೆ* i_mem)
{
    // Memory operations in this build carry no bounds check, so code only runs where a
    // fault past the end of a memory can still be turned into a trap
    ಆದರೆ (M3_UNLIKELY(not m3_Host_Guards_ಸಕ್ರಿಯ())) {
        ಹಿಂತಿರುಗಿ ErrorRuntime(m3_Err_ಸ್ಮರಣೆ_Guards_Unavailable, i_runtime,
                            "another part of the process holds the fault signal");
    }

    M3_Protected_ಸತತ ಸತತ;
    ಸತತ.pc      = i_pc;
    ಸತತ.runtime = i_runtime;
    ಸತತ.mem     = i_mem;
    ಸತತ.ಫಲಿತಾಂಶ  = m3Err_none;

    ಶೂನ್ಯ*  arena;
    size_t arena_ಬೈಟುಗಳು;
    Guard_ArenaRange(&arena, &arena_ಬೈಟುಗಳು);

    ಆದರೆ (not m3_HostProtectedCall(ಸತತ_ಸಂಕೇತ_Body, &ಸತತ, arena, arena_ಬೈಟುಗಳು)) {
        ಹಿಂತಿರುಗಿ m3_Err_trap_ಹೊರಹೋಗು_ದ_Bounds_ಸ್ಮರಣೆ_Access;
    }

    ಹಿಂತಿರುಗಿ ಸತತ.ಫಲಿತಾಂಶ;
}

#ಆದರೆ_ಅಂತ್ಯ // d_m3GuardedMemory


#ಆದರೆ d_m3_Has_ರಾಶಿ_Switching

// The function a suspended continuation stopped in: the callee of its innermost
// call frame, or the function it was entered with if it made no call. Frames are
// recorded innermost first, so that is the lowest-numbered call frame.
ಸ್ಥಿರ
IM3Function Continuation_InnermostFunction (IM3Continuation i_cont)
{
    ಗಾಗಿ (u32 i = 0; i < i_cont->ಅಂಕೆ_ಚೌಕಟ್ಟುಗಳು; ++i) {
        ಆದರೆ (i_cont->ಚೌಕಟ್ಟುಗಳು[i].ಬಗೆ == ಚೌಕಟ್ಟು_call && i_cont->ಚೌಕಟ್ಟುಗಳು[i].call.function) {
            ಹಿಂತಿರುಗಿ i_cont->ಚೌಕಟ್ಟುಗಳು[i].call.function;
        }
    }

    ಹಿಂತಿರುಗಿ i_cont->ನಮೂದು_Function;
}


// Where the host's next call puts its arguments and finds its results. A
// suspended root keeps its frames on the bottom of the runtime's stack, so a call
// made before it is resumed starts above the last of them rather than on top of
// them. The top is the innermost function's frame base plus everything that
// function can use. That base is the root's own sp, unless it stopped inside a
// resume: then the suspension point belongs to the resumed continuation, and the
// root's innermost frame is that resume.
ಶೂನ್ಯ Runtime_ಇರಿಸು_Call_ರಾಶಿ (IM3Runtime io_runtime)
{
    IM3Continuation ಬೇರು = io_runtime->ಬೇರು_Continuation;

    io_runtime->ರಾಶಿ = io_runtime->origin_ರಾಶಿ;

    ಆದರೆ (not ಬೇರು or ಬೇರು->state != cont_suspended) {
        ಹಿಂತಿರುಗಿ;
    }

    m3stack_t   sp       = ಬೇರು->sp;
    IM3Function function = Continuation_InnermostFunction(ಬೇರು);

    ಆದರೆ (ಬೇರು->ಅಂಕೆ_ಚೌಕಟ್ಟುಗಳು and ಬೇರು->ಚೌಕಟ್ಟುಗಳು[0].ಬಗೆ == ಚೌಕಟ್ಟು_resume) {
        sp = ಬೇರು->ಚೌಕಟ್ಟುಗಳು[0].sp;
    }

    m3slot_t* ತಳ = (m3slot_t*)io_runtime->origin_ರಾಶಿ;
    m3slot_t* ತುದಿ  = (m3slot_t*)sp + (function ? function->max_ರಾಶಿ_Slots : 0);

    ಆದರೆ (ತುದಿ > ತಳ + io_runtime->ಅಂಕೆ_ರಾಶಿ_Slots) {
        ತುದಿ = ತಳ + io_runtime->ಅಂಕೆ_ರಾಶಿ_Slots;
    }

    io_runtime->ರಾಶಿ = ತುದಿ;
}

#ಆದರೆ_ಅಂತ್ಯ // d_m3HasStackSwitching


// Run compiled code on the runtime's stack, bounding native recursion for the
// duration of the call. The outermost invocation establishes the stack limit;
// nested ones (an imported function calling back into Wasm) inherit it.
ಸ್ಥಿರ ಸಾಲೊಳಗೆ
M3_ಫಲಿತಾಂಶ ಸತತ_ಸಂಕೇತ_Checked (IM3Runtime i_runtime, IM3Function i_function)
{
    pc_t i_pc = i_function->compiled;

    // execution runs against the memory of the module the entry point belongs
    // to, not against some runtime-wide one
    M3_ಸ್ಮರಣೆ_ಶೀರ್ಷಿಕೆ* _mem = Module_ಸ್ಮರಣೆ_ಶೀರ್ಷಿಕೆ(i_function->module);

#ಆದರೆ d_m3HasSnapshots
    ಆದರೆ (M3_UNLIKELY(i_function->module->isUnusable)) {
        ಹಿಂತಿರುಗಿ "a snapshot failed to restore into the module, so it cannot run";
    }
    i_function->module->has_ಸತತ = true;
#ಆದರೆ_ಅಂತ್ಯ

    d_m3_ರಾಶಿ_ಎಲ್ಲೆ_Enter(i_runtime);

    // a host function calling back into Wasm does not change which call a trap
    // ends: that is still the one the host made first
    ಆದರೆ (i_runtime->callNesting++ == 0) {
        i_runtime->entered = i_function;
    }
#ಆದರೆ d_m3HasExceptionHandling
    // handler stacks don't nest across a call boundary: a host function calling
    // back into Wasm cannot be caught by a try_table its own caller entered
    u32 saved_Try_ಆಳ   = i_runtime->try_ಆಳ;
    i_runtime->try_ಆಳ = 0;
#ಆದರೆ_ಅಂತ್ಯ
#ಆದರೆ d_m3_Has_ರಾಶಿ_Switching
    // nor do continuations: the native frames on the far side of a host call
    // are not ours to record, so a suspend in here has no handler to reach and
    // no resume to unwind to
    IM3Continuation savedContinuation = i_runtime->ಸಕ್ರಿಯ_Continuation;

    // A suspended root continuation is a paused program waiting to be resumed,
    // not scratch space. A call the host makes before resuming it gets a
    // context of its own, the way any other re-entry does, rather than having
    // the frames of the suspension reset out from under it.
    bool claim_ಬೇರು = i_runtime->isSuspendable and not savedContinuation and
                     not (i_runtime->ಬೇರು_Continuation and
                          i_runtime->ಬೇರು_Continuation->state == cont_suspended);

    ಆದರೆ (claim_ಬೇರು and not i_runtime->ಬೇರು_Continuation) {
        i_runtime->ಬೇರು_Continuation = Continuation_New(i_runtime, i_function->funcType, i_function);

        ಆದರೆ (i_runtime->ಬೇರು_Continuation) {
            // The root borrows the runtime's stack and is not charged as cont.new.
            i_runtime->ಬೇರು_Continuation->ಅಂಕೆ_ರಾಶಿ_Slots = i_runtime->ಅಂಕೆ_ರಾಶಿ_Slots;
        } ಇಲ್ಲದಿದ್ದರೆ {
            claim_ಬೇರು = false;
        }
    }

    ಆದರೆ (claim_ಬೇರು) {
        i_runtime->ಸಕ್ರಿಯ_Continuation              = i_runtime->ಬೇರು_Continuation;
        i_runtime->ಬೇರು_Continuation->ನಮೂದು_Function = i_function;
        i_runtime->ಬೇರು_Continuation->state         = cont_running;
        i_runtime->ಬೇರು_Continuation->ಅಂಕೆ_ಚೌಕಟ್ಟುಗಳು     = 0;
    } ಇಲ್ಲದಿದ್ದರೆ {
        i_runtime->ಸಕ್ರಿಯ_Continuation = ಖಾಲಿ;
    }
#ಆದರೆ_ಅಂತ್ಯ
#ಆದರೆ d_m3_Guarded_ಸ್ಮರಣೆ
    M3_ಫಲಿತಾಂಶ ಫಲಿತಾಂಶ = ಸತತ_ಸಂಕೇತ_Protected(i_runtime, i_pc, _mem);
#ಅಥವಾಆದರೆ (d_m3EnableOpProfiling || d_m3EnableOpTracing)
    M3_ಫಲಿತಾಂಶ ಫಲಿತಾಂಶ = (M3_ಫಲಿತಾಂಶ)ಸತತ_ಸಂಕೇತ(i_pc, (m3stack_t)i_runtime->ರಾಶಿ, _mem, d_m3_Op_Default_ವಾದಗಳು, d_m3_ತಳ_Cstr);
#ಇಲ್ಲದಿದ್ದರೆ
    M3_ಫಲಿತಾಂಶ ಫಲಿತಾಂಶ = (M3_ಫಲಿತಾಂಶ)ಸತತ_ಸಂಕೇತ(i_pc, (m3stack_t)i_runtime->ರಾಶಿ, _mem, d_m3_Op_Default_ವಾದಗಳು);
#ಆದರೆ_ಅಂತ್ಯ
#ಆದರೆ d_m3_Has_ರಾಶಿ_Switching
    ಆದರೆ (claim_ಬೇರು) {
        i_runtime->ಬೇರು_Continuation->state =
          (ಫಲಿತಾಂಶ == m3Err_continuationSuspended) ? cont_suspended : cont_returned;

        Runtime_ಇರಿಸು_Call_ರಾಶಿ(i_runtime);
    }
    i_runtime->ಸಕ್ರಿಯ_Continuation = savedContinuation;
#ಆದರೆ_ಅಂತ್ಯ
#ಆದರೆ d_m3HasExceptionHandling
    i_runtime->try_ಆಳ = saved_Try_ಆಳ;

    // an exception that reached the bottom of the call stack found no handler
    ಆದರೆ (M3_UNLIKELY(ಫಲಿತಾಂಶ == m3Err_pendingException)) {
        ಫಲಿತಾಂಶ = m3Err_trapUncaughtException;
    }
    ದಾಖಲೆ_Uncaught_Exception(i_runtime, ಫಲಿತಾಂಶ);

    // a paused invocation can still reach the exceptions it holds, so they wait
    // for it to finish
    ಆದರೆ (i_runtime->callNesting == 1 and not m3_IsSuspended(i_runtime)) {
        FreeExceptions(i_runtime);
    }
#ಆದರೆ_ಅಂತ್ಯ
    i_runtime->callNesting--;
    d_m3_ರಾಶಿ_ಎಲ್ಲೆ_Leave(i_runtime);

    ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}

M3_ಫಲಿತಾಂಶ m3_ಸತತ_ಆರಂಭ (IM3Module io_module)
{
#ವ್ಯಾಖ್ಯಾನಿತವಾದರೆ FUZZING_BUILD_ಬಹುಲಕ_UNSAFE_FOR_PRODUCTION
    // Execution disabled for fuzzing builds
    ಹಿಂತಿರುಗಿ m3Err_none;
#ಆದರೆ_ಅಂತ್ಯ

    M3_ಫಲಿತಾಂಶ ಫಲಿತಾಂಶ           = m3Err_none;
    i32      ಆರಂಭ_Function_Tmp = -1;

    ಆದರೆ (io_module and io_module->ಆರಂಭ_Function >= 0) {
        IM3Function function = &io_module->functions[io_module->ಆರಂಭ_Function];

        ಆದರೆ (not function->compiled) {
_           (CompileFunction(function));
        }

        IM3FuncType ftype = function->funcType;
        ಆದರೆ (ftype->ಅಂಕೆ_ವಾದಗಳು != 0 || ftype->ಅಂಕೆ_Rets != 0) {
            _throw(m3_Err_ವಾದ_ಎಣಿಕೆ_Mismatch);
        }

        IM3Module  module  = function->module;
        IM3Runtime runtime = module->runtime;

        ಆರಂಭ_Function_Tmp = io_module->ಆರಂಭ_Function;

        io_module->ಆರಂಭ_Function = -1;

        ಫಲಿತಾಂಶ = ಸತತ_ಸಂಕೇತ_Checked(runtime, function);

        // a start function that suspended is not to be started again: resuming
        // is what finishes it
        ಆದರೆ (ಫಲಿತಾಂಶ and ಫಲಿತಾಂಶ != m3Err_continuationSuspended) {
            io_module->ಆರಂಭ_Function = ಆರಂಭ_Function_Tmp;
            EXCEPTION_PRINT(ಫಲಿತಾಂಶ);
            ಹೋಗು _catch;
        }
    }

    _catch: ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}

// TODO: deal with main + side-modules loading efforcement
M3_ಫಲಿತಾಂಶ m3_ತುಂಬು_Module (IM3Runtime io_runtime, IM3Module io_module)
{
    M3_ಫಲಿತಾಂಶ ಫಲಿತಾಂಶ = m3Err_none;

    ಆದರೆ (M3_UNLIKELY(io_module->runtime)) {
        ಹಿಂತಿರುಗಿ m3Err_moduleAlreadyLinked;
    }

    io_module->runtime = io_runtime;

    // linking first: a memory import has to be resolved before InitMemory would
    // give it pages of its own, and a global import before an initializer reads it
_   (LinkImports(io_runtime, io_module));

_   (Init_ಸ್ಮರಣೆ(io_runtime, io_module));
_   (InitGlobals(io_module));
    // Spec order: element segments are applied before data segments. It matters
    // when one of them traps - whatever ran before the trap stays done.
_   (Init_ಕೋಷ್ಟಕ_And_Elements(io_module));
_   (Init_ದತ್ತಾಂಶ_Segments(io_module));

    // Start func might use imported functions, which are not liked here yet,
    // so it will be called before a function call is attempted (in m3_FindFunction)

#ವ್ಯಾಖ್ಯಾನಿತವಾದರೆ DEBUG
    Module_Generate_ಹೆಸರುಗಳು(io_module);
#ಆದರೆ_ಅಂತ್ಯ

    io_module->ಮುಂದಿನ     = io_runtime->modules;
    io_runtime->modules = io_module;
    ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ; // ok

_catch:
    // The runtime owns the module either way. Instantiation may already have
    // written this module's functions into a table another module owns, and the
    // spec keeps whatever it managed to do before the trap, so those entries
    // stay callable - retaining the module is what stops them dangling.
    //
    // It goes on the tail of the list rather than the head: it is not a module
    // anyone should find by name, only one whose functions may still be
    // reachable through someone else's table.
    io_module->ಮುಂದಿನ = ಖಾಲಿ;

    IM3Module* ಬಾಲ = &io_runtime->modules;
    ಆಗ (*ಬಾಲ) {
        ಬಾಲ = &(*ಬಾಲ)->ಮುಂದಿನ;
    }
    *ಬಾಲ = io_module;

    ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}

IM3Global m3_ಕಂಡುಹಿಡಿ_Global (IM3Module         io_module,
                         ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* ಸ್ಥಿರಾಂಕ i_globalName)
{
    // Search exports
    IM3Global exported = Module_ಕಂಡುಹಿಡಿ_Exported_Global(io_module, i_globalName);
    ಆದರೆ (exported) {
        // a re-exported global is the one that was imported, so reads and
        // writes have to reach the cell that actually holds the value
        ಹಿಂತಿರುಗಿ exported->resolved ? exported->resolved : exported;
    }

    // Search imports
    ಗಾಗಿ (u32 i = 0; i < io_module->ಅಂಕೆ_Globals; ++i) {
        IM3Global g = &io_module->globals[i];

        ಆದರೆ (g->import.moduleUtf8 and g->import.ಕ್ಷೇತ್ರ_Utf8) {
            ಆದರೆ (strcmp(g->import.ಕ್ಷೇತ್ರ_Utf8, i_globalName) == 0) {
                ಹಿಂತಿರುಗಿ g->resolved ? g->resolved : g;
            }
        }
    }
    ಹಿಂತಿರುಗಿ ಖಾಲಿ;
}

M3_ಫಲಿತಾಂಶ m3_GetGlobal (IM3Global      i_global,
                       IM3_Tagged_ಮೌಲ್ಯ o_ಮೌಲ್ಯ)
{
    ಆದರೆ (not i_global) {
        ಹಿಂತಿರುಗಿ m3Err_globalLookupFailed;
    }

    ಆಯ್ಕೆ (i_global->type) {
    ಸ್ಥಿತಿ c_m3Type_i32: o_ಮೌಲ್ಯ->ಮೌಲ್ಯ.i32 = i_global->i32_ಮೌಲ್ಯ; ನಿಲ್ಲಿಸು;
    ಸ್ಥಿತಿ c_m3Type_i64: o_ಮೌಲ್ಯ->ಮೌಲ್ಯ.i64 = i_global->i64_ಮೌಲ್ಯ; ನಿಲ್ಲಿಸು;
#ಆದರೆ d_m3HasFloat
    ಸ್ಥಿತಿ c_m3Type_f32: o_ಮೌಲ್ಯ->ಮೌಲ್ಯ.f32 = i_global->f32_ಮೌಲ್ಯ; ನಿಲ್ಲಿಸು;
    ಸ್ಥಿತಿ c_m3Type_f64: o_ಮೌಲ್ಯ->ಮೌಲ್ಯ.f64 = i_global->f64_ಮೌಲ್ಯ; ನಿಲ್ಲಿಸು;
#ಆದರೆ_ಅಂತ್ಯ
    ಪೂರ್ವನಿಯೋಜಿತ: ಹಿಂತಿರುಗಿ m3Err_invalidTypeId;
    }

    o_ಮೌಲ್ಯ->type = (M3_ಮೌಲ್ಯ_Type)(i_global->type);
    ಹಿಂತಿರುಗಿ m3Err_none;
}

M3_ಫಲಿತಾಂಶ m3_ಗಣ_Global (IM3Global            i_global,
                       ಸ್ಥಿರಾಂಕ IM3_Tagged_ಮೌಲ್ಯ i_ಮೌಲ್ಯ)
{
    ಆದರೆ (not i_global) {
        ಹಿಂತಿರುಗಿ m3Err_globalLookupFailed;
    }
    ಆದರೆ (not i_global->isMutable) {
        ಹಿಂತಿರುಗಿ m3Err_globalNotMutable;
    }
    ಆದರೆ (i_global->type != i_ಮೌಲ್ಯ->type) {
        ಹಿಂತಿರುಗಿ m3Err_globalTypeMismatch;
    }

    ಆಯ್ಕೆ (i_ಮೌಲ್ಯ->type) {
    ಸ್ಥಿತಿ c_m3Type_i32: i_global->i32_ಮೌಲ್ಯ = i_ಮೌಲ್ಯ->ಮೌಲ್ಯ.i32; ನಿಲ್ಲಿಸು;
    ಸ್ಥಿತಿ c_m3Type_i64: i_global->i64_ಮೌಲ್ಯ = i_ಮೌಲ್ಯ->ಮೌಲ್ಯ.i64; ನಿಲ್ಲಿಸು;
#ಆದರೆ d_m3HasFloat
    ಸ್ಥಿತಿ c_m3Type_f32: i_global->f32_ಮೌಲ್ಯ = i_ಮೌಲ್ಯ->ಮೌಲ್ಯ.f32; ನಿಲ್ಲಿಸು;
    ಸ್ಥಿತಿ c_m3Type_f64: i_global->f64_ಮೌಲ್ಯ = i_ಮೌಲ್ಯ->ಮೌಲ್ಯ.f64; ನಿಲ್ಲಿಸು;
#ಆದರೆ_ಅಂತ್ಯ
    ಪೂರ್ವನಿಯೋಜಿತ: ಹಿಂತಿರುಗಿ m3Err_invalidTypeId;
    }

    ಹಿಂತಿರುಗಿ m3Err_none;
}

M3_ಮೌಲ್ಯ_Type m3_GetGlobalType (IM3Global i_global)
{
    ಹಿಂತಿರುಗಿ (i_global) ? (M3_ಮೌಲ್ಯ_Type)(i_global->type) : c_m3Type_none;
}


ಶೂನ್ಯ* v_ಕಂಡುಹಿಡಿ_Function (IM3Module i_module, ಶೂನ್ಯ* i_info)
{
    ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* ಸ್ಥಿರಾಂಕ i_name = (ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ*)i_info;

    // Prefer exported functions
    IM3Function exported = Module_ಕಂಡುಹಿಡಿ_Exported_Function(i_module, i_name);
    ಆದರೆ (exported) {
        ಹಿಂತಿರುಗಿ exported;
    }

    // Search internal functions
    ಗಾಗಿ (u32 i = 0; i < i_module->ಅಂಕೆ_Functions; ++i) {
        IM3Function f = &i_module->functions[i];

        bool isImported = f->import.moduleUtf8 or f->import.ಕ್ಷೇತ್ರ_Utf8;

        ಆದರೆ (isImported) {
            ಮುಂದುವರಿ;
        }

        ಗಾಗಿ (ಪೂರ್ಣಾಂಕ j = 0; j < f->ಅಂಕೆ_ಹೆಸರುಗಳು; j++) {
            ಆದರೆ (f->ಹೆಸರುಗಳು[j] and strcmp(f->ಹೆಸರುಗಳು[j], i_name) == 0) {
                ಹಿಂತಿರುಗಿ f;
            }
        }
    }

    ಹಿಂತಿರುಗಿ ಖಾಲಿ;
}


// Shared tail of the two lookups: a function is only usable once it has code.
ಸ್ಥಿರ
M3_ಫಲಿತಾಂಶ Prepare_ಸಿಕ್ಕಿತು_Function (IM3Function* o_function, IM3Function i_function)
{
    M3_ಫಲಿತಾಂಶ ಫಲಿತಾಂಶ = m3Err_none;

    ಆದರೆ (not i_function->compiled) {
_       (CompileFunction(i_function))
    }

    _catch:
    *o_function = ಫಲಿತಾಂಶ ? ಖಾಲಿ : i_function;

    ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}


// Searches every module in the runtime, most recently loaded first. That is a
// guess once more than one module is loaded and two of them export the same
// name - m3_FindFunctionIn says which module is meant.
M3_ಫಲಿತಾಂಶ m3_ಕಂಡುಹಿಡಿ_Function (IM3Function* o_function, IM3Runtime i_runtime, ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* ಸ್ಥಿರಾಂಕ i_functionName)
{
                                                                d_m3Assert (o_function and i_runtime and i_functionName);
    IM3Function function = ಖಾಲಿ;

    ಆದರೆ (not i_runtime->modules) {
        *o_function = ಖಾಲಿ;
        ಹಿಂತಿರುಗಿ "no modules loaded";
    }

    function = (IM3Function)ForEachModule(i_runtime, v_ಕಂಡುಹಿಡಿ_Function, (ಶೂನ್ಯ*)i_functionName);

    ಆದರೆ (not function) {
        *o_function = ಖಾಲಿ;
        ಹಿಂತಿರುಗಿ ErrorModule(m3Err_functionLookupFailed, i_runtime->modules, "'%s'", i_functionName);
    }

    ಹಿಂತಿರುಗಿ Prepare_ಸಿಕ್ಕಿತು_Function(o_function, function);
}


IM3Module m3_ಕಂಡುಹಿಡಿ_Module (IM3Runtime i_runtime, ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* ಸ್ಥಿರಾಂಕ i_moduleName)
{
    ಆದರೆ (not i_runtime or not i_moduleName) {
        ಹಿಂತಿರುಗಿ ಖಾಲಿ;
    }

    // the list is newest-first, so a name registered twice names the newer one
    ಗಾಗಿ (IM3Module m = i_runtime->modules; m; m = m->ಮುಂದಿನ) {
        ಆದರೆ (m->name and strcmp(m->name, i_moduleName) == 0) {
            ಹಿಂತಿರುಗಿ m;
        }
    }

    ಹಿಂತಿರುಗಿ ಖಾಲಿ;
}


// Searches one module's exports, which is what naming a module means.
M3_ಫಲಿತಾಂಶ m3_ಕಂಡುಹಿಡಿ_Function_ಒಳ (IM3Function* o_function, IM3Module i_module, ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* ಸ್ಥಿರಾಂಕ i_functionName)
{
                                                                d_m3Assert (o_function and i_functionName);
    ಆದರೆ (not i_module) {
        *o_function = ಖಾಲಿ;
        ಹಿಂತಿರುಗಿ m3Err_functionLookupFailed;      // ErrorModule would deref it
    }

    IM3Function function = (IM3Function)v_ಕಂಡುಹಿಡಿ_Function(i_module, (ಶೂನ್ಯ*)i_functionName);

    ಆದರೆ (not function) {
        *o_function = ಖಾಲಿ;
        ಹಿಂತಿರುಗಿ ErrorModule(m3Err_functionLookupFailed, i_module, "'%s'", i_functionName);
    }

    ಹಿಂತಿರುಗಿ Prepare_ಸಿಕ್ಕಿತು_Function(o_function, function);
}


M3_ಫಲಿತಾಂಶ m3_Get_ಕೋಷ್ಟಕ_Function (IM3Function* o_function, IM3Module i_module, uint32_t i_ಸೂಚ್ಯಂಕ)
{
_try {
    M3_ಕೋಷ್ಟಕ*    ಕೋಷ್ಟಕ_2;
    IM3Function function;

    _throwif("no table", i_module->ಅಂಕೆ_Tables == 0);

    ಕೋಷ್ಟಕ_2 = i_module->tables[0];
    _throwif("function index out of range", i_ಸೂಚ್ಯಂಕ >= ಕೋಷ್ಟಕ_2->ಗಾತ್ರ_2);

    function = (IM3Function)ಕೋಷ್ಟಕ_2->elements[i_ಸೂಚ್ಯಂಕ];

    ಆದರೆ (function) {
        ಆದರೆ (not function->compiled) {
_           (CompileFunction(function))
        }
    }

    *o_function = function;
} _catch:
    ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}


ಸ್ಥಿರ
M3_ಫಲಿತಾಂಶ check_ಆರಂಭ_Function (IM3Module i_module)
{
    M3_ಫಲಿತಾಂಶ ಫಲಿತಾಂಶ = m3Err_none;                               d_m3Assert(i_module);

    // Check if start function needs to be called
    ಆದರೆ (i_module->ಆರಂಭ_Function >= 0) {
        ಫಲಿತಾಂಶ = m3_ಸತತ_ಆರಂಭ(i_module);
    }

    ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}

uint32_t m3_Get_Arg_ಎಣಿಕೆ (IM3Function i_function)
{
    ಆದರೆ (i_function) {
        IM3FuncType ft = i_function->funcType;
        ಆದರೆ (ft) {
            ಹಿಂತಿರುಗಿ ft->ಅಂಕೆ_ವಾದಗಳು;
        }
    }
    ಹಿಂತಿರುಗಿ 0;
}

uint32_t m3_Get_Ret_ಎಣಿಕೆ (IM3Function i_function)
{
    ಆದರೆ (i_function) {
        IM3FuncType ft = i_function->funcType;
        ಆದರೆ (ft) {
            ಹಿಂತಿರುಗಿ ft->ಅಂಕೆ_Rets;
        }
    }
    ಹಿಂತಿರುಗಿ 0;
}


M3_ಮೌಲ್ಯ_Type m3_GetArgType (IM3Function i_function, uint32_t index)
{
    ಆದರೆ (i_function) {
        IM3FuncType ft = i_function->funcType;
        ಆದರೆ (ft and index < ft->ಅಂಕೆ_ವಾದಗಳು) {
            ಹಿಂತಿರುಗಿ (M3_ಮೌಲ್ಯ_Type)ತಳ_Type_ದ(d_FuncArgType(ft, index));
        }
    }
    ಹಿಂತಿರುಗಿ c_m3Type_none;
}

M3_ಮೌಲ್ಯ_Type m3_GetRetType (IM3Function i_function, uint32_t index)
{
    ಆದರೆ (i_function) {
        IM3FuncType ft = i_function->funcType;
        ಆದರೆ (ft and index < ft->ಅಂಕೆ_Rets) {
            ಹಿಂತಿರುಗಿ (M3_ಮೌಲ್ಯ_Type)ತಳ_Type_ದ(d_FuncRetType(ft, index));
        }
    }
    ಹಿಂತಿರುಗಿ c_m3Type_none;
}


// The call APIs marshal their arguments into the runtime stack before any
// compiled code runs, so op_Entry's overflow check comes too late to cover those
// writes - and for a host function or an import linked to another module there
// is no op_Entry of our own to reach at all. Bound the area about to be written
// here instead: results and arguments each take a u64 at this boundary, however
// wide the slot is inside. Every caller goes through this, so none of them can
// be the one that forgets.
ಸ್ಥಿರ
M3_ಫಲಿತಾಂಶ Get_ರಾಶಿ_ಸೂಚಕ_For_ವಾದಗಳು (IM3Function i_function, u8** o_ರಾಶಿ)
{
    IM3Runtime  runtime = i_function->module->runtime;
    IM3FuncType ftype   = i_function->funcType;

    // the stack a paused invocation holds is not available to this one
    size_t ಬಳಸಿದ      = (size_t)((u8*)runtime->ರಾಶಿ - (u8*)runtime->origin_ರಾಶಿ);
    size_t needed    = ((size_t)ftype->ಅಂಕೆ_Rets + ftype->ಅಂಕೆ_ವಾದಗಳು) * ಗಾತ್ರ(u64);
    size_t ಲಭ್ಯ = (size_t)runtime->ಅಂಕೆ_ರಾಶಿ_Slots * ಗಾತ್ರ(m3slot_t) - ಬಳಸಿದ;

    ಆದರೆ (needed > ಲಭ್ಯ) {
        ಹಿಂತಿರುಗಿ m3_Err_trap_ರಾಶಿ_ಅತಿಪ್ರವಾಹ;
    }

    u64* ರಾಶಿ = (u64*)runtime->ರಾಶಿ;

    ರಾಶಿ += ftype->ಅಂಕೆ_Rets;

    *o_ರಾಶಿ = (u8*)ರಾಶಿ;

    ಹಿಂತಿರುಗಿ m3Err_none;
}


M3_ಫಲಿತಾಂಶ m3_CallV (IM3Function i_function, ...)
{
    va_list ap;
    va_start(ap, i_function);
    M3_ಫಲಿತಾಂಶ r = m3_CallVL(i_function, ap);
    va_end(ap);
    ಹಿಂತಿರುಗಿ r;
}

ಸ್ಥಿರ
ಶೂನ್ಯ Report_Native_ರಾಶಿ_Usage ()
{
#ಆದರೆ d_m3_Log_Native_ರಾಶಿ
    ಪೂರ್ಣಾಂಕ ರಾಶಿ_ಬಳಸಿದ = m3_ರಾಶಿ_Get_Max();
    fprintf(stderr, "Native stack used: %d\n", ರಾಶಿ_ಬಳಸಿದ);
#ಆದರೆ_ಅಂತ್ಯ
}


M3_ಫಲಿತಾಂಶ m3_CallVL (IM3Function i_function, va_list i_ವಾದಗಳು)
{
    IM3Runtime  runtime = i_function->module->runtime;
    IM3FuncType ftype   = i_function->funcType;
    M3_ಫಲಿತಾಂಶ    ಫಲಿತಾಂಶ  = m3Err_none;
    u8*         s       = ಖಾಲಿ;

    ಆದರೆ (!i_function->compiled) {
        ಹಿಂತಿರುಗಿ m3_Err_missing_Compiled_ಸಂಕೇತ;
    }

#ಆದರೆ d_m3_ದಾಖಲೆ_Backtraces
    ClearBacktrace(runtime);
#ಆದರೆ_ಅಂತ್ಯ

    m3_ರಾಶಿ_Check_Init();

_   (check_ಆರಂಭ_Function(i_function->module))

_   (Get_ರಾಶಿ_ಸೂಚಕ_For_ವಾದಗಳು(i_function, &s))

    ಗಾಗಿ (u32 i = 0; i < ftype->ಅಂಕೆ_ವಾದಗಳು; ++i) {
        ಆಯ್ಕೆ (d_FuncArgType(ftype, i)) {
        ಸ್ಥಿತಿ c_m3Type_i32:
            *(i32*)(s) = va_arg(i_ವಾದಗಳು, i32);
            s += 8;
            ನಿಲ್ಲಿಸು;
        ಸ್ಥಿತಿ c_m3Type_i64:
            *(i64*)(s) = va_arg(i_ವಾದಗಳು, i64);
            s += 8;
            ನಿಲ್ಲಿಸು;
        ಸ್ಥಿತಿ c_m3Type_funcref:
        ಸ್ಥಿತಿ c_m3Type_externref:
        ಸ್ಥಿತಿ c_m3Type_exnref:
            *(uintptr_t*)(s) = va_arg(i_ವಾದಗಳು, uintptr_t);
            s += 8;
            ನಿಲ್ಲಿಸು;
#ಆದರೆ d_m3HasFloat
        ಸ್ಥಿತಿ c_m3Type_f32:
            *(f32*)(s) = (f32)va_arg(i_ವಾದಗಳು, f64);
            s += 8;
            ನಿಲ್ಲಿಸು; // f32 is passed as f64
        ಸ್ಥಿತಿ c_m3Type_f64:
            *(f64*)(s) = va_arg(i_ವಾದಗಳು, f64);
            s += 8;
            ನಿಲ್ಲಿಸು;
#ಆದರೆ_ಅಂತ್ಯ
        ಪೂರ್ವನಿಯೋಜಿತ: ಹಿಂತಿರುಗಿ "unknown argument type";
        }
    }

    ಫಲಿತಾಂಶ = ಸತತ_ಸಂಕೇತ_Checked(runtime, i_function);
    Report_Native_ರಾಶಿ_Usage();

    runtime->ಕೊನೆಯ_Called = ಫಲಿತಾಂಶ ? ಖಾಲಿ : i_function;

    _catch: ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}

M3_ಫಲಿತಾಂಶ m3_Call (IM3Function i_function, uint32_t i_argc, ಸ್ಥಿರಾಂಕ ಶೂನ್ಯ* i_argptrs[])
{
    IM3Runtime  runtime = i_function->module->runtime;
    IM3FuncType ftype   = i_function->funcType;
    M3_ಫಲಿತಾಂಶ    ಫಲಿತಾಂಶ  = m3Err_none;
    u8*         s       = ಖಾಲಿ;

    ಆದರೆ (i_argc != ftype->ಅಂಕೆ_ವಾದಗಳು) {
        ಹಿಂತಿರುಗಿ m3_Err_ವಾದ_ಎಣಿಕೆ_Mismatch;
    }
    ಆದರೆ (!i_function->compiled) {
        ಹಿಂತಿರುಗಿ m3_Err_missing_Compiled_ಸಂಕೇತ;
    }

#ಆದರೆ d_m3_ದಾಖಲೆ_Backtraces
    ClearBacktrace(runtime);
#ಆದರೆ_ಅಂತ್ಯ

    m3_ರಾಶಿ_Check_Init();

_   (check_ಆರಂಭ_Function(i_function->module))

_   (Get_ರಾಶಿ_ಸೂಚಕ_For_ವಾದಗಳು(i_function, &s))

    ಗಾಗಿ (u32 i = 0; i < ftype->ಅಂಕೆ_ವಾದಗಳು; ++i) {
        ಆಯ್ಕೆ (d_FuncArgType(ftype, i)) {
        ಸ್ಥಿತಿ c_m3Type_i32:
            *(i32*)(s) = *(i32*)i_argptrs[i];
            s += 8;
            ನಿಲ್ಲಿಸು;
        ಸ್ಥಿತಿ c_m3Type_i64:
            *(i64*)(s) = *(i64*)i_argptrs[i];
            s += 8;
            ನಿಲ್ಲಿಸು;
        ಸ್ಥಿತಿ c_m3Type_funcref:
        ಸ್ಥಿತಿ c_m3Type_externref:
        ಸ್ಥಿತಿ c_m3Type_exnref:
            *(uintptr_t*)(s) = *(uintptr_t*)i_argptrs[i];
            s += 8;
            ನಿಲ್ಲಿಸು;
#ಆದರೆ d_m3HasFloat
        ಸ್ಥಿತಿ c_m3Type_f32:
            *(f32*)(s) = *(f32*)i_argptrs[i];
            s += 8;
            ನಿಲ್ಲಿಸು;
        ಸ್ಥಿತಿ c_m3Type_f64:
            *(f64*)(s) = *(f64*)i_argptrs[i];
            s += 8;
            ನಿಲ್ಲಿಸು;
#ಆದರೆ_ಅಂತ್ಯ
        ಪೂರ್ವನಿಯೋಜಿತ: ಹಿಂತಿರುಗಿ "unknown argument type";
        }
    }

    ಫಲಿತಾಂಶ = ಸತತ_ಸಂಕೇತ_Checked(runtime, i_function);

    Report_Native_ರಾಶಿ_Usage();

    runtime->ಕೊನೆಯ_Called = ಫಲಿತಾಂಶ ? ಖಾಲಿ : i_function;

    _catch: ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}

// Argument parsing for m3_CallArgv. Strict on purpose: the whole string has to
// be consumed, so "12abc" is rejected rather than read as 12, and an empty or
// unparsable argument is an error rather than the zero that strtoul with a
// NULL end pointer used to hand back. See wasm3/wasm3#367.
ಸ್ಥಿರ
M3_ಫಲಿತಾಂಶ ParseArgInteger (ccstr_t i_arg, u32 i_ಅಂಕೆ_ಬಿಟ್ಟುಗಳು, u64* o_ಮೌಲ್ಯ)
{
    ಆದರೆ (not i_arg or not *i_arg) {
        ಹಿಂತಿರುಗಿ "empty argument";
    }

    // strtoull would skip leading space, but trailing space is rejected below;
    // accepting one and not the other would just be confusing
    ಆದರೆ (ಖಾಲಿಯೇ((ಚಿಹ್ನೆರಹಿತ ಅಕ್ಷರ)*i_arg)) {
        ಹಿಂತಿರುಗಿ "argument is not a number";
    }

    ಅಕ್ಷರ* ಕೊನೆ = ಖಾಲಿ;
    u64   ಮೌಲ್ಯ;

    errno = 0;

    // an argument may be spelled signed or unsigned: -1 and 4294967295 name the
    // same i32
    ಆದರೆ (*i_arg == '-') {
        i64 signed_ಮೌಲ್ಯ = strtoll(i_arg, &ಕೊನೆ, 10);

        ಆದರೆ (i_ಅಂಕೆ_ಬಿಟ್ಟುಗಳು == 32 and (signed_ಮೌಲ್ಯ < INT32_MIN or signed_ಮೌಲ್ಯ > INT32_MAX)) {
            ಹಿಂತಿರುಗಿ "argument out of range";
        }

        ಮೌಲ್ಯ = (u64)signed_ಮೌಲ್ಯ;
    } ಇಲ್ಲದಿದ್ದರೆ {
        ಮೌಲ್ಯ = strtoull(i_arg, &ಕೊನೆ, 10);

        ಆದರೆ (i_ಅಂಕೆ_ಬಿಟ್ಟುಗಳು == 32 and ಮೌಲ್ಯ > UINT32_MAX) {
            ಹಿಂತಿರುಗಿ "argument out of range";
        }
    }

    ಆದರೆ (errno == ERANGE) {
        ಹಿಂತಿರುಗಿ "argument out of range";
    }

    ಆದರೆ (ಕೊನೆ == i_arg or *ಕೊನೆ) {
        ಹಿಂತಿರುಗಿ "argument is not a number";
    }

    *o_ಮೌಲ್ಯ = ಮೌಲ್ಯ;

    ಹಿಂತಿರುಗಿ m3Err_none;
}


#ಆದರೆ d_m3HasFloat
ಸ್ಥಿರ
M3_ಫಲಿತಾಂಶ ParseArgFloat (ccstr_t i_arg, f64* o_ಮೌಲ್ಯ)
{
    ಆದರೆ (not i_arg or not *i_arg) {
        ಹಿಂತಿರುಗಿ "empty argument";
    }

    ಆದರೆ (ಖಾಲಿಯೇ((ಚಿಹ್ನೆರಹಿತ ಅಕ್ಷರ)*i_arg)) {
        ಹಿಂತಿರುಗಿ "argument is not a number";
    }

    ಅಕ್ಷರ* ಕೊನೆ = ಖಾಲಿ;

    errno = 0;

    f64 ಮೌಲ್ಯ = strtod(i_arg, &ಕೊನೆ);

    ಆದರೆ (ಕೊನೆ == i_arg or *ಕೊನೆ) {
        ಹಿಂತಿರುಗಿ "argument is not a number";
    }

    // strtod reports underflow through ERANGE as well, and a denormal result is
    // perfectly usable, so only an overflow to infinity is out of range
    ಆದರೆ (errno == ERANGE and (ಮೌಲ್ಯ > DBL_MAX or ಮೌಲ್ಯ < -DBL_MAX)) {
        ಹಿಂತಿರುಗಿ "argument out of range";
    }

    *o_ಮೌಲ್ಯ = ಮೌಲ್ಯ;

    ಹಿಂತಿರುಗಿ m3Err_none;
}
#ಆದರೆ_ಅಂತ್ಯ


// A reference argument is either the null reference or a host handle written as
// an integer.
ಸ್ಥಿರ
M3_ಫಲಿತಾಂಶ ParseArgReference (ccstr_t i_arg, u64* o_ಮೌಲ್ಯ)
{
    ಆದರೆ (i_arg and strcmp(i_arg, "null") == 0) {
        *o_ಮೌಲ್ಯ = 0;
        ಹಿಂತಿರುಗಿ m3Err_none;
    }

    ಹಿಂತಿರುಗಿ ParseArgInteger(i_arg, 64, o_ಮೌಲ್ಯ);
}


M3_ಫಲಿತಾಂಶ m3_CallArgv (IM3Function i_function, uint32_t i_argc, ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* i_argv[])
{
    IM3FuncType ftype   = i_function->funcType;
    IM3Runtime  runtime = i_function->module->runtime;
    M3_ಫಲಿತಾಂಶ    ಫಲಿತಾಂಶ  = m3Err_none;
    u8*         s       = ಖಾಲಿ;

    ಆದರೆ (i_argc != ftype->ಅಂಕೆ_ವಾದಗಳು) {
        ಹಿಂತಿರುಗಿ m3_Err_ವಾದ_ಎಣಿಕೆ_Mismatch;
    }
    ಆದರೆ (!i_function->compiled) {
        ಹಿಂತಿರುಗಿ m3_Err_missing_Compiled_ಸಂಕೇತ;
    }

#ಆದರೆ d_m3_ದಾಖಲೆ_Backtraces
    ClearBacktrace(runtime);
#ಆದರೆ_ಅಂತ್ಯ

    m3_ರಾಶಿ_Check_Init();

_   (check_ಆರಂಭ_Function(i_function->module))

_   (Get_ರಾಶಿ_ಸೂಚಕ_For_ವಾದಗಳು(i_function, &s))

    ಗಾಗಿ (u32 i = 0; i < ftype->ಅಂಕೆ_ವಾದಗಳು; ++i) {
        u64 ಮೌಲ್ಯ = 0;
#ಆದರೆ d_m3HasFloat
        f64 fvalue = 0;
#ಆದರೆ_ಅಂತ್ಯ
        ಆಯ್ಕೆ (d_FuncArgType(ftype, i)) {
        ಸ್ಥಿತಿ c_m3Type_i32:
_           (ParseArgInteger(i_argv[i], 32, &ಮೌಲ್ಯ)) * (i32*)(s) = (i32)ಮೌಲ್ಯ;
            s += 8;
            ನಿಲ್ಲಿಸು;
        ಸ್ಥಿತಿ c_m3Type_i64:
_           (ParseArgInteger(i_argv[i], 64, &ಮೌಲ್ಯ)) * (i64*)(s) = (i64)ಮೌಲ್ಯ;
            s += 8;
            ನಿಲ್ಲಿಸು;
        ಸ್ಥಿತಿ c_m3Type_funcref:
        ಸ್ಥಿತಿ c_m3Type_externref:
        ಸ್ಥಿತಿ c_m3Type_exnref:
_           (ParseArgReference(i_argv[i], &ಮೌಲ್ಯ)) * (uintptr_t*)(s) = (uintptr_t)ಮೌಲ್ಯ;
            s += 8;
            ನಿಲ್ಲಿಸು;
#ಆದರೆ d_m3HasFloat
            // strtof would be less portable
        ಸ್ಥಿತಿ c_m3Type_f32:
_           (ParseArgFloat(i_argv[i], &fvalue)) * (f32*)(s) = (f32)fvalue;
            s += 8;
            ನಿಲ್ಲಿಸು;
        ಸ್ಥಿತಿ c_m3Type_f64:
_           (ParseArgFloat(i_argv[i], &fvalue)) * (f64*)(s) = fvalue;
            s += 8;
            ನಿಲ್ಲಿಸು;
#ಆದರೆ_ಅಂತ್ಯ
        ಪೂರ್ವನಿಯೋಜಿತ: _throw("unknown argument type");
        }
    }

    ಫಲಿತಾಂಶ = ಸತತ_ಸಂಕೇತ_Checked(runtime, i_function);

    Report_Native_ರಾಶಿ_Usage();

    runtime->ಕೊನೆಯ_Called = ಫಲಿತಾಂಶ ? ಖಾಲಿ : i_function;

    _catch: ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}


//u8 * AlignStackPointerTo64Bits (const u8 * i_stack)
//{
//    uintptr_t ptr = (uintptr_t) i_stack;
//    return (u8 *) ((ptr + 7) & ~7);
//}


M3_ಫಲಿತಾಂಶ m3_GetResults (IM3Function i_function, uint32_t i_retc, ಸ್ಥಿರಾಂಕ ಶೂನ್ಯ* o_retptrs[])
{
    IM3FuncType ftype   = i_function->funcType;
    IM3Runtime  runtime = i_function->module->runtime;

    ಆದರೆ (i_retc != ftype->ಅಂಕೆ_Rets) {
        ಹಿಂತಿರುಗಿ m3_Err_ವಾದ_ಎಣಿಕೆ_Mismatch;
    }
    ಆದರೆ (i_function != runtime->ಕೊನೆಯ_Called) {
        ಹಿಂತಿರುಗಿ "function not called";
    }

    u8* s = (u8*)runtime->ರಾಶಿ;

    ಗಾಗಿ (u32 i = 0; i < ftype->ಅಂಕೆ_Rets; ++i) {
        ಆಯ್ಕೆ (d_FuncRetType(ftype, i)) {
        ಸ್ಥಿತಿ c_m3Type_i32:
            *(i32*)o_retptrs[i] = *(i32*)(s);
            s += 8;
            ನಿಲ್ಲಿಸು;
        ಸ್ಥಿತಿ c_m3Type_i64:
            *(i64*)o_retptrs[i] = *(i64*)(s);
            s += 8;
            ನಿಲ್ಲಿಸು;
        ಸ್ಥಿತಿ c_m3Type_funcref:
        ಸ್ಥಿತಿ c_m3Type_externref:
        ಸ್ಥಿತಿ c_m3Type_exnref:
            *(uintptr_t*)o_retptrs[i] = *(uintptr_t*)(s);
            s += 8;
            ನಿಲ್ಲಿಸು;
#ಆದರೆ d_m3HasFloat
        ಸ್ಥಿತಿ c_m3Type_f32:
            *(f32*)o_retptrs[i] = *(f32*)(s);
            s += 8;
            ನಿಲ್ಲಿಸು;
        ಸ್ಥಿತಿ c_m3Type_f64:
            *(f64*)o_retptrs[i] = *(f64*)(s);
            s += 8;
            ನಿಲ್ಲಿಸು;
#ಆದರೆ_ಅಂತ್ಯ
        ಪೂರ್ವನಿಯೋಜಿತ: ಹಿಂತಿರುಗಿ "unknown return type";
        }
    }
    ಹಿಂತಿರುಗಿ m3Err_none;
}

M3_ಫಲಿತಾಂಶ m3_GetResultsV (IM3Function i_function, ...)
{
    va_list ap;
    va_start(ap, i_function);
    M3_ಫಲಿತಾಂಶ r = m3_GetResultsVL(i_function, ap);
    va_end(ap);
    ಹಿಂತಿರುಗಿ r;
}

M3_ಫಲಿತಾಂಶ m3_GetResultsVL (IM3Function i_function, va_list o_rets)
{
    IM3Runtime  runtime = i_function->module->runtime;
    IM3FuncType ftype   = i_function->funcType;

    ಆದರೆ (i_function != runtime->ಕೊನೆಯ_Called) {
        ಹಿಂತಿರುಗಿ "function not called";
    }

    u8* s = (u8*)runtime->ರಾಶಿ;
    ಗಾಗಿ (u32 i = 0; i < ftype->ಅಂಕೆ_Rets; ++i) {
        ಆಯ್ಕೆ (d_FuncRetType(ftype, i)) {
        ಸ್ಥಿತಿ c_m3Type_i32:
            *va_arg(o_rets, i32*) = *(i32*)(s);
            s += 8;
            ನಿಲ್ಲಿಸು;
        ಸ್ಥಿತಿ c_m3Type_i64:
            *va_arg(o_rets, i64*) = *(i64*)(s);
            s += 8;
            ನಿಲ್ಲಿಸು;
        ಸ್ಥಿತಿ c_m3Type_funcref:
        ಸ್ಥಿತಿ c_m3Type_externref:
        ಸ್ಥಿತಿ c_m3Type_exnref:
            *va_arg(o_rets, uintptr_t*) = *(uintptr_t*)(s);
            s += 8;
            ನಿಲ್ಲಿಸು;
#ಆದರೆ d_m3HasFloat
        ಸ್ಥಿತಿ c_m3Type_f32:
            *va_arg(o_rets, f32*) = *(f32*)(s);
            s += 8;
            ನಿಲ್ಲಿಸು;
        ಸ್ಥಿತಿ c_m3Type_f64:
            *va_arg(o_rets, f64*) = *(f64*)(s);
            s += 8;
            ನಿಲ್ಲಿಸು;
#ಆದರೆ_ಅಂತ್ಯ
        ಪೂರ್ವನಿಯೋಜಿತ: ಹಿಂತಿರುಗಿ "unknown argument type";
        }
    }
    ಹಿಂತಿರುಗಿ m3Err_none;
}

ಶೂನ್ಯ Release_ಸಂಕೇತ_ಪುಟ_No_Track (IM3Runtime i_runtime, IM3_ಸಂಕೇತ_ಪುಟ i_ಸಂಕೇತ_ಪುಟ)
{
    ಆದರೆ (i_ಸಂಕೇತ_ಪುಟ) {
        IM3_ಸಂಕೇತ_ಪುಟ* list;

        bool ಪುಟ_ತುಂಬಿದ = (ಅಂಕೆ_Free_ಸಾಲುಗಳು(i_ಸಂಕೇತ_ಪುಟ) < d_m3_ಸಂಕೇತ_ಪುಟ_Free_ಸಾಲುಗಳು_ಮಿತಿ);
        ಆದರೆ (ಪುಟ_ತುಂಬಿದ) {
            list = &i_runtime->ಪುಟಗಳು_ತುಂಬಿದ;
        } ಇಲ್ಲದಿದ್ದರೆ {
            list = &i_runtime->ಪುಟಗಳು_Open;
        }

        ತಳ್ಳು_ಸಂಕೇತ_ಪುಟ(list, i_ಸಂಕೇತ_ಪುಟ);                         m3log (emit, "release page: %d to queue: '%s'", i_ಸಂಕೇತ_ಪುಟ->info.sequence, ಪುಟ_ತುಂಬಿದ ? "full" : "open")
    }
}


IM3_ಸಂಕೇತ_ಪುಟ Acquire_ಸಂಕೇತ_ಪುಟ_With_ಸಾಮರ್ಥ್ಯ (IM3Runtime i_runtime, u32 i_min_ಸಾಲು_ಎಣಿಕೆ)
{
    IM3_ಸಂಕೇತ_ಪುಟ ಪುಟ = Remove_ಸಂಕೇತ_ಪುಟ_ದ_ಸಾಮರ್ಥ್ಯ(&i_runtime->ಪುಟಗಳು_Open, i_min_ಸಾಲು_ಎಣಿಕೆ);

    ಆದರೆ (not ಪುಟ) {
        ಪುಟ = Environment_Acquire_ಸಂಕೇತ_ಪುಟ(i_runtime->environment, i_min_ಸಾಲು_ಎಣಿಕೆ);

        ಆದರೆ (not ಪುಟ) {
            ಪುಟ = New_ಸಂಕೇತ_ಪುಟ(i_runtime, i_min_ಸಾಲು_ಎಣಿಕೆ);
        }

        ಆದರೆ (ಪುಟ) {
            i_runtime->ಅಂಕೆ_ಸಂಕೇತ_ಪುಟಗಳು++;
        }
    }

    ಆದರೆ (ಪುಟ) {                                                  m3log (emit, "acquire page: %d", ಪುಟ->info.sequence);
        i_runtime->ಅಂಕೆ_ಸಕ್ರಿಯ_ಸಂಕೇತ_ಪುಟಗಳು++;
    }

    ಹಿಂತಿರುಗಿ ಪುಟ;
}


IM3_ಸಂಕೇತ_ಪುಟ Acquire_ಸಂಕೇತ_ಪುಟ (IM3Runtime i_runtime)
{
    ಹಿಂತಿರುಗಿ Acquire_ಸಂಕೇತ_ಪುಟ_With_ಸಾಮರ್ಥ್ಯ(i_runtime, d_m3_ಸಂಕೇತ_ಪುಟ_Free_ಸಾಲುಗಳು_ಮಿತಿ);
}


ಶೂನ್ಯ Release_ಸಂಕೇತ_ಪುಟ (IM3Runtime i_runtime, IM3_ಸಂಕೇತ_ಪುಟ i_ಸಂಕೇತ_ಪುಟ)
{
    ಆದರೆ (i_ಸಂಕೇತ_ಪುಟ) {
        Release_ಸಂಕೇತ_ಪುಟ_No_Track(i_runtime, i_ಸಂಕೇತ_ಪುಟ);
        i_runtime->ಅಂಕೆ_ಸಕ್ರಿಯ_ಸಂಕೇತ_ಪುಟಗಳು--;

#ಆದರೆ defined(DEBUG)
        u32 ಅಂಕೆ_Open = ಎಣಿಕೆ_ಸಂಕೇತ_ಪುಟಗಳು(i_runtime->ಪುಟಗಳು_Open);
        u32 ಅಂಕೆ_ತುಂಬಿದ = ಎಣಿಕೆ_ಸಂಕೇತ_ಪುಟಗಳು(i_runtime->ಪುಟಗಳು_ತುಂಬಿದ);

        m3log(runtime, "runtime: %p; open-pages: %d; full-pages: %d; active: %d; total: %d", i_runtime, ಅಂಕೆ_Open, ಅಂಕೆ_ತುಂಬಿದ, i_runtime->ಅಂಕೆ_ಸಕ್ರಿಯ_ಸಂಕೇತ_ಪುಟಗಳು, i_runtime->ಅಂಕೆ_ಸಂಕೇತ_ಪುಟಗಳು);

        d_m3Assert(ಅಂಕೆ_Open + ಅಂಕೆ_ತುಂಬಿದ + i_runtime->ಅಂಕೆ_ಸಕ್ರಿಯ_ಸಂಕೇತ_ಪುಟಗಳು == i_runtime->ಅಂಕೆ_ಸಂಕೇತ_ಪುಟಗಳು);

#  ಆದರೆ d_m3_Log_ಸಂಕೇತ_ಪುಟಗಳು
        dump_ಸಂಕೇತ_ಪುಟ(i_ಸಂಕೇತ_ಪುಟ, /* startPC: */ ಖಾಲಿ);
#  endif
#ಆದರೆ_ಅಂತ್ಯ
    }
}


#ಆದರೆ d_m3VerboseErrorMessages
M3_ಫಲಿತಾಂಶ m3Error (M3_ಫಲಿತಾಂಶ i_ಫಲಿತಾಂಶ, IM3Runtime i_runtime, IM3Module i_module, IM3Function i_function,
                  ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* ಸ್ಥಿರಾಂಕ i_file, u32 i_ಸಾಲು_ಅಂಕೆ, ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* ಸ್ಥಿರಾಂಕ i_error_ಸಂದೇಶ, ...)
{
    ಆದರೆ (i_runtime) {
        i_runtime->error         = (M3ErrorInfo){ .ಫಲಿತಾಂಶ = i_ಫಲಿತಾಂಶ, .runtime = i_runtime, .module = i_module, .function = i_function, .file = i_file, .ಸಾಲು_2 = i_ಸಾಲು_ಅಂಕೆ };
        i_runtime->error.ಸಂದೇಶ = i_runtime->error_ಸಂದೇಶ;

        va_list ವಾದಗಳು;
        va_start(ವಾದಗಳು, i_error_ಸಂದೇಶ);
        vsnprintf(i_runtime->error_ಸಂದೇಶ, ಗಾತ್ರ(i_runtime->error_ಸಂದೇಶ), i_error_ಸಂದೇಶ, ವಾದಗಳು);
        va_end(ವಾದಗಳು);
    }

    ಹಿಂತಿರುಗಿ i_ಫಲಿತಾಂಶ;
}
#ಆದರೆ_ಅಂತ್ಯ


ಶೂನ್ಯ m3_GetErrorInfo (IM3Runtime i_runtime, M3ErrorInfo* o_info)
{
    ಆದರೆ (i_runtime) {
        *o_info = i_runtime->error;
        m3_ResetErrorInfo(i_runtime);
    }
}


ಶೂನ್ಯ m3_ResetErrorInfo (IM3Runtime i_runtime)
{
    ಆದರೆ (i_runtime) {
        M3_INIT(i_runtime->error);
        i_runtime->error.ಸಂದೇಶ = "";
    }
}

uint8_t* m3_Get_ಸ್ಮರಣೆ (IM3Module i_module, size_t* o_ಸ್ಮರಣೆ_ಗಾತ್ರ_ಒಳ_ಬೈಟುಗಳು, uint32_t i_ಸ್ಮರಣೆ_ಸೂಚ್ಯಂಕ)
{
    uint8_t* ಸ್ಮರಣೆ = ಖಾಲಿ;
    size_t   ಗಾತ್ರ_2   = 0;

    ಆದರೆ (i_module and i_ಸ್ಮರಣೆ_ಸೂಚ್ಯಂಕ < i_module->ಅಂಕೆ_Memories) {
        IM3_ಸ್ಮರಣೆ mem = i_module->memories[i_ಸ್ಮರಣೆ_ಸೂಚ್ಯಂಕ];

        ಆದರೆ (mem->mallocated) {
            ಗಾತ್ರ_2 = mem->mallocated->ಉದ್ದಳತೆ;

            ಆದರೆ (ಗಾತ್ರ_2) {
                ಸ್ಮರಣೆ = m3_Mem_ದತ್ತಾಂಶ(mem->mallocated);
            }
        }
    }

    ಆದರೆ (o_ಸ್ಮರಣೆ_ಗಾತ್ರ_ಒಳ_ಬೈಟುಗಳು) {
        *o_ಸ್ಮರಣೆ_ಗಾತ್ರ_ಒಳ_ಬೈಟುಗಳು = ಗಾತ್ರ_2;
    }

    ಹಿಂತಿರುಗಿ ಸ್ಮರಣೆ;
}


size_t m3_Get_ಸ್ಮರಣೆ_ಗಾತ್ರ (IM3Module i_module, uint32_t i_ಸ್ಮರಣೆ_ಸೂಚ್ಯಂಕ)
{
    ಆದರೆ (not i_module or i_ಸ್ಮರಣೆ_ಸೂಚ್ಯಂಕ >= i_module->ಅಂಕೆ_Memories) {
        ಹಿಂತಿರುಗಿ 0;
    }

    IM3_ಸ್ಮರಣೆ mem = i_module->memories[i_ಸ್ಮರಣೆ_ಸೂಚ್ಯಂಕ];

    ಹಿಂತಿರುಗಿ mem->mallocated ? mem->mallocated->ಉದ್ದಳತೆ : 0;
}


size_t m3_Get_ಸ್ಮರಣೆ_ಗಾತ್ರ_At (ಸ್ಥಿರಾಂಕ ಶೂನ್ಯ* i_ಸ್ಮರಣೆ)
{
    ಆದರೆ (not i_ಸ್ಮರಣೆ) {
        ಹಿಂತಿರುಗಿ 0;
    }

    // the header sits immediately before the data it describes
    ಸ್ಥಿರಾಂಕ M3_ಸ್ಮರಣೆ_ಶೀರ್ಷಿಕೆ* ಶೀರ್ಷಿಕೆ = ((ಸ್ಥಿರಾಂಕ M3_ಸ್ಮರಣೆ_ಶೀರ್ಷಿಕೆ*)i_ಸ್ಮರಣೆ) - 1;

    ಹಿಂತಿರುಗಿ ಶೀರ್ಷಿಕೆ->ಉದ್ದಳತೆ;
}


M3_ಫಲಿತಾಂಶ m3_ಕಂಡುಹಿಡಿ_Exported_ಸ್ಮರಣೆ (IM3Module i_module, cstr_t i_name, u32* o_ಸ್ಮರಣೆ_ಸೂಚ್ಯಂಕ)
{
    ಆದರೆ (not i_module or not i_name or not o_ಸ್ಮರಣೆ_ಸೂಚ್ಯಂಕ) {
        ಹಿಂತಿರುಗಿ m3_Err_unknown_ಸ್ಮರಣೆ;
    }

    ಸ್ಥಿರಾಂಕ M3Export* e = Module_ಕಂಡುಹಿಡಿ_Export(i_module, d_external_ಬಗೆ_ಸ್ಮರಣೆ, i_name);

    ಆದರೆ (not e) {
        ಹಿಂತಿರುಗಿ m3_Err_unknown_ಸ್ಮರಣೆ;
    }

    *o_ಸ್ಮರಣೆ_ಸೂಚ್ಯಂಕ = e->index;
    ಹಿಂತಿರುಗಿ m3Err_none;
}


bool Module_HasLinkedHostImport (IM3Module i_module, cstr_t i_importModule)
{
    ಗಾಗಿ (u32 i = 0; i < i_module->ಅಂಕೆ_Functions; ++i) {
        IM3Function f = &i_module->functions[i];

        // hostMemory is set when a host function is bound to the import, so it
        // is also the mark of one - see CompileRawFunction
        ಆದರೆ (not f->host_ಸ್ಮರಣೆ or f->wasm or f->resolved) {
            ಮುಂದುವರಿ;
        }

        ಆದರೆ (f->import.moduleUtf8 and strcmp(f->import.moduleUtf8, i_importModule) == 0) {
            ಹಿಂತಿರುಗಿ true;
        }
    }

    ಹಿಂತಿರುಗಿ false;
}


M3_ಫಲಿತಾಂಶ m3_Bind_Import_ಸ್ಮರಣೆ (IM3Module io_module, cstr_t i_importModule, u32 i_ಸ್ಮರಣೆ_ಸೂಚ್ಯಂಕ)
{
    ಆದರೆ (not io_module or not i_importModule) {
        ಹಿಂತಿರುಗಿ m3Err_moduleNotLinked;
    }

    ಆದರೆ (i_ಸ್ಮರಣೆ_ಸೂಚ್ಯಂಕ >= io_module->ಅಂಕೆ_Memories) {
        ಹಿಂತಿರುಗಿ m3_Err_unknown_ಸ್ಮರಣೆ;
    }

    // the memory itself, not the index: linking may have pointed this slot at
    // another module's memory, and growing one reallocates behind it
    IM3_ಸ್ಮರಣೆ ಸ್ಮರಣೆ = io_module->memories[i_ಸ್ಮರಣೆ_ಸೂಚ್ಯಂಕ];

    ಸ್ಥಿರಾಂಕ bool wildcardModule = (strcmp(i_importModule, "*") == 0);

    ಗಾಗಿ (u32 i = 0; i < io_module->ಅಂಕೆ_Functions; ++i) {
        IM3Function f = &io_module->functions[i];

        // only an import that a host function was bound to has a memory to
        // pin: one with a body of its own reads memory through the interpreter,
        // and one resolved to another module's export runs that module's code
        ಆದರೆ (not f->host_ಸ್ಮರಣೆ or f->wasm or f->resolved) {
            ಮುಂದುವರಿ;
        }

        ಆದರೆ (wildcardModule or (f->import.moduleUtf8 and strcmp(f->import.moduleUtf8, i_importModule) == 0)) {
            f->host_ಸ್ಮರಣೆ = ಸ್ಮರಣೆ;
        }
    }

    ಹಿಂತಿರುಗಿ m3Err_none;
}


M3BacktraceInfo* m3_GetBacktrace (IM3Runtime i_runtime)
{
#ಆದರೆ d_m3_ದಾಖಲೆ_Backtraces
    ಹಿಂತಿರುಗಿ &i_runtime->backtrace;
#ಇಲ್ಲದಿದ್ದರೆ
    ಹಿಂತಿರುಗಿ ಖಾಲಿ;
#ಆದರೆ_ಅಂತ್ಯ
}


//-------------------------------------------------------------------------------------------------------------------------------
//  snapshots & suspendable execution
//-------------------------------------------------------------------------------------------------------------------------------

ಶೂನ್ಯ m3_ಗಣ_Suspendable (IM3Runtime io_runtime, bool i_suspendable)
{
    ಆದರೆ (io_runtime) {
        io_runtime->isSuspendable = i_suspendable;
    }
}

ಶೂನ್ಯ m3_RequestSuspend (IM3Runtime io_runtime)
{
    ಆದರೆ (io_runtime) {
        io_runtime->suspendRequested = true;
    }
}

bool m3_IsSuspended (IM3Runtime i_runtime)
{
    ಆದರೆ (!i_runtime) {
        ಹಿಂತಿರುಗಿ false;
    }
#ಆದರೆ d_m3_Has_ರಾಶಿ_Switching
    ಆದರೆ (i_runtime->ಬೇರು_Continuation && i_runtime->ಬೇರು_Continuation->state == cont_suspended) {
        ಹಿಂತಿರುಗಿ true;
    }
#ಆದರೆ_ಅಂತ್ಯ
    ಹಿಂತಿರುಗಿ false;
}

M3_ಫಲಿತಾಂಶ m3_ResumeRuntime (IM3Runtime io_runtime)
{
    ಆದರೆ (!io_runtime) {
        ಹಿಂತಿರುಗಿ m3Err_mallocFailed;
    }
    ಆದರೆ (!io_runtime->isSuspendable) {
        ಹಿಂತಿರುಗಿ m3Err_none;
    }
#ಆದರೆ d_m3HasSnapshots
    // a restore that failed leaves nothing to resume, and says so every time
    // rather than resuming nothing successfully
    ಆದರೆ (io_runtime->ಬೇರು_Continuation and io_runtime->ಬೇರು_Continuation->ನಮೂದು_Function and
        io_runtime->ಬೇರು_Continuation->ನಮೂದು_Function->module->isUnusable) {
        ಹಿಂತಿರುಗಿ "a snapshot failed to restore into the module, so it cannot run";
    }
#ಆದರೆ_ಅಂತ್ಯ
#ಆದರೆ d_m3_Has_ರಾಶಿ_Switching
    ಆದರೆ (!io_runtime->ಬೇರು_Continuation || io_runtime->ಬೇರು_Continuation->state != cont_suspended) {
        ಹಿಂತಿರುಗಿ m3Err_none;
    }
    // what finishes returns into the bottom of the stack, where it was entered
    io_runtime->ರಾಶಿ = io_runtime->origin_ರಾಶಿ;

    // this is the outermost call again, as far as a host function calling back
    // into Wasm from inside it can tell
    io_runtime->entered = io_runtime->ಬೇರು_Continuation->ನಮೂದು_Function;
    io_runtime->callNesting++;

    d_m3_ರಾಶಿ_ಎಲ್ಲೆ_Enter(io_runtime);
    io_runtime->resumePastCheck = true;
    M3_ಫಲಿತಾಂಶ ಫಲಿತಾಂಶ             = (M3_ಫಲಿತಾಂಶ)ResumeContinuation(io_runtime, io_runtime->ಬೇರು_Continuation);
    io_runtime->resumePastCheck = false;
    d_m3_ರಾಶಿ_ಎಲ್ಲೆ_Leave(io_runtime);

#  ಆದರೆ d_m3HasExceptionHandling
    ಆದರೆ (M3_UNLIKELY(ಫಲಿತಾಂಶ == m3Err_pendingException)) {
        ಫಲಿತಾಂಶ = m3Err_trapUncaughtException;
    }
    ದಾಖಲೆ_Uncaught_Exception(io_runtime, ಫಲಿತಾಂಶ);
#  endif

    io_runtime->callNesting--;

#  ಆದರೆ d_m3HasExceptionHandling
    ಆದರೆ (ಫಲಿತಾಂಶ != m3Err_continuationSuspended and io_runtime->callNesting == 0) {
        FreeExceptions(io_runtime);
    }
#  endif

    // the results are where the paused call left room for them, so a finished
    // resume can be asked for them the way the call itself could have been
    io_runtime->ಕೊನೆಯ_Called = (ಫಲಿತಾಂಶ == m3Err_none) ? io_runtime->ಬೇರು_Continuation->ನಮೂದು_Function : ಖಾಲಿ;

    Runtime_ಇರಿಸು_Call_ರಾಶಿ(io_runtime);
    ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
#ಇಲ್ಲದಿದ್ದರೆ
    ಹಿಂತಿರುಗಿ m3Err_none;
#ಆದರೆ_ಅಂತ್ಯ
}
