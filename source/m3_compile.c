//
//  m3_compile.c
//
//  Created by Steven Massey on 4/17/19.
//  Copyright © 2019 Steven Massey. All rights reserved.
//

// Allow using opcodes for compilation process
#ವ್ಯಾಖ್ಯಾನಿಸು M3_COMPILE_OPCODES

#ಅನ್ತರ್ಭಾವ "m3_env.h"
#ಅನ್ತರ್ಭಾವ "m3_compile.h"
#ಅನ್ತರ್ಭಾವ "m3_exec.h"
#ಅನ್ತರ್ಭಾವ "m3_exception.h"
#ಅನ್ತರ್ಭಾವ "m3_info.h"
#ಅನ್ತರ್ಭಾವ "m3_validate.h"

// Resolves M3OpInfo.compiler, an index, to the function it names. Defined down
// with c_compilers [], which has to follow every compiler it points at.
ಸ್ಥಿರ
M3Compiler GetCompiler (IM3OpInfo i_opInfo);

//----- EMIT --------------------------------------------------------------------------------------------------------------

ಸ್ಥಿರ ಸಾಲೊಳಗೆ
pc_t GetPC (IM3Compilation o)
{
    ಹಿಂತಿರುಗಿ Get_ಪುಟ_PC(o->ಪುಟ);
}

#ಆದರೆ d_m3HasSnapshots

#  define d_m3GrowSnapshotArray(TYPE, ARRAY, ಎಣಿಕೆ_3, ಸಾಮರ್ಥ್ಯ_2)                    \
    ಆದರೆ ((ಎಣಿಕೆ_3) == (ಸಾಮರ್ಥ್ಯ_2)) {                                                 \
        u32   new_ಸಾಮರ್ಥ್ಯ = (ಸಾಮರ್ಥ್ಯ_2) ? (ಸಾಮರ್ಥ್ಯ_2) * 2 : 8;                     \
        TYPE* grown = m3_ReallocArray(TYPE, (ARRAY), new_ಸಾಮರ್ಥ್ಯ, (ಸಾಮರ್ಥ್ಯ_2));   \
        ಆದರೆ (not grown) {                                                         \
            ಹಿಂತಿರುಗಿ m3Err_mallocFailed;                                           \
        }                                                                        \
        (ARRAY)    = grown;                                                      \
        (ಸಾಮರ್ಥ್ಯ_2) = new_ಸಾಮರ್ಥ್ಯ;                                                \
    }

// Where the instruction being compiled starts, counted from the start of the
// function's body: the one position every build of every revision agrees on
ಸ್ಥಿರ ಸಾಲೊಳಗೆ
u32 Get_Wasm_ಸ್ಥಾನಾಂತರ (IM3Compilation o)
{
    ಹಿಂತಿರುಗಿ (u32)(o->ಕೊನೆಯ_Opcode_ಆರಂಭ - o->body_ಆರಂಭ);
}

// Whether the operand stack entry at i_index is a value the frame holds. Every
// block keeps records of its parameter and result slots on the stack, between
// the height it was entered at and where its own values begin (see
// CompileBlock). Those are landing pads rather than values: nothing need have
// written them yet, and the slots they name may be in use for something else.
// The function's own scope has none - that stretch is its arguments and locals.
ಸ್ಥಿರ
bool Is_ಜೀವಂತ_ರಾಶಿ_ನಮೂದು (IM3Compilation o, u16 i_ಸೂಚ್ಯಂಕ)
{
    ಗಾಗಿ (IM3CompilationScope scope = &o->ಖಂಡ; scope->outer; scope = scope->outer) {
        ಆದರೆ (i_ಸೂಚ್ಯಂಕ >= scope->exit_ರಾಶಿ_ಸೂಚ್ಯಂಕ and i_ಸೂಚ್ಯಂಕ < scope->ಖಂಡ_ರಾಶಿ_ಸೂಚ್ಯಂಕ) {
            ಹಿಂತಿರುಗಿ false;
        }
    }

    ಹಿಂತಿರುಗಿ true;
}

ಸ್ಥಿರ
M3_ಫಲಿತಾಂಶ ಕೂಡಿಸು_ಸುರಕ್ಷಿತ_ಬಿಂದು_ಮೌಲ್ಯ (IM3Compilation o, M3_ಸುರಕ್ಷಿತ_ಬಿಂದು* io_ಬಿಂದು, u16 i_ಸೂಚ್ಯಂಕ)
{
    M3SnapshotMap* map = o->snapshotMap;

    d_m3GrowSnapshotArray(M3_ಸ್ಥಾನ_ಮೌಲ್ಯ, map->ಮೌಲ್ಯಗಳು, map->ಅಂಕೆ_ಮೌಲ್ಯಗಳು, o->ಮೌಲ್ಯಗಳು_ಸಾಮರ್ಥ್ಯ);

    map->ಮೌಲ್ಯಗಳು[map->ಅಂಕೆ_ಮೌಲ್ಯಗಳು].ಸ್ಥಾನ = o->wasm_ರಾಶಿ[i_ಸೂಚ್ಯಂಕ];
    map->ಮೌಲ್ಯಗಳು[map->ಅಂಕೆ_ಮೌಲ್ಯಗಳು].type = o->type_ರಾಶಿ[i_ಸೂಚ್ಯಂಕ];
    map->ಅಂಕೆ_ಮೌಲ್ಯಗಳು++;
    io_ಬಿಂದು->ಅಂಕೆ_ಮೌಲ್ಯಗಳು++;

    ಹಿಂತಿರುಗಿ m3Err_none;
}

// Records the pc i_wordsBack words before the current one as a safepoint, with
// where every value on the live operand stack is kept there.
//
// i_numResults are the operation's results, on top of the stack. A call's or a
// resume's are not written until it returns, so they are no part of the frame;
// a suspend's are, since whatever resumes it writes them - and cont.bind can
// write them while it waits. i_aux is what the kind keeps there (see
// M3SafePoint).
//
// A back edge to i_loop leaves the rest of the loop's body behind: what the
// frame holds is what was live before the loop, and the loop's parameters,
// which the branch has just written into its landing pads for the next lap.
ಸ್ಥಿರ
M3_ಫಲಿತಾಂಶ ದಾಖಲೆ_ಸುರಕ್ಷಿತ_ಬಿಂದು (IM3Compilation o, M3_ಸುರಕ್ಷಿತ_ಬಿಂದು_ಬಗೆ i_ಬಗೆ, u32 i_ಪದಗಳು_ಹಿಂಭಾಗ, u16 i_ಅಂಕೆ_Results,
                          u16 i_aux, IM3CompilationScope i_loop)
{
_try {
    M3SnapshotMap* map = o->snapshotMap;

    ಆದರೆ (not map or not o->ಪುಟ) {
        ಹಿಂತಿರುಗಿ m3Err_none;
    }

    d_m3GrowSnapshotArray(M3_ಸುರಕ್ಷಿತ_ಬಿಂದು, map->ಸುರಕ್ಷಿತ_Points, map->ಅಂಕೆ_ಸುರಕ್ಷಿತ_Points, o->ಸುರಕ್ಷಿತ_Points_ಸಾಮರ್ಥ್ಯ);

    // grown before it is pointed at: adding values never moves the safepoints
    M3_ಸುರಕ್ಷಿತ_ಬಿಂದು* ಬಿಂದು     = &map->ಸುರಕ್ಷಿತ_Points[map->ಅಂಕೆ_ಸುರಕ್ಷಿತ_Points];
    ಬಿಂದು->pc              = GetPC(o) - i_ಪದಗಳು_ಹಿಂಭಾಗ;
    ಬಿಂದು->wasm_ಸ್ಥಾನಾಂತರ      = Get_Wasm_ಸ್ಥಾನಾಂತರ(o);
    ಬಿಂದು->ಮೊದಲ_ಮೌಲ್ಯ      = map->ಅಂಕೆ_ಮೌಲ್ಯಗಳು;
    ಬಿಂದು->ಮೊದಲ_Local_ಮೌಲ್ಯ = UINT32_MAX;
    ಬಿಂದು->ಅಂಕೆ_ಮೌಲ್ಯಗಳು       = 0;
    ಬಿಂದು->ಅಂಕೆ_Results      = i_ಅಂಕೆ_Results;
    ಬಿಂದು->aux             = i_aux;
    ಬಿಂದು->resumeType      = c_m3Type_none;
    ಬಿಂದು->ಬಗೆ            = (u8)i_ಬಗೆ;
    ಬಿಂದು->flags           = 0;

    // Non-defaultable locals can still be uninitialized at a safepoint. Keep
    // their heap types, but allow the null placeholder until local.set makes
    // them definitely initialized. Branches to a loop use its entry context.
    ಆದರೆ (o->local_Init_ಆಳ) {
        ಬಿಂದು->ಮೊದಲ_Local_ಮೌಲ್ಯ = map->ಅಂಕೆ_ಮೌಲ್ಯಗಳು;
        ಗಾಗಿ (u32 i = 0; i < map->ಅಂಕೆ_Locals; ++i) {
            d_m3GrowSnapshotArray(M3_ಸ್ಥಾನ_ಮೌಲ್ಯ, map->ಮೌಲ್ಯಗಳು, map->ಅಂಕೆ_ಮೌಲ್ಯಗಳು, o->ಮೌಲ್ಯಗಳು_ಸಾಮರ್ಥ್ಯ);
            M3_ಸ್ಥಾನ_ಮೌಲ್ಯ ಮೌಲ್ಯ = map->locals[i];
            ಆದರೆ (o->local_Init_ಆಳ[i] == UINT32_MAX or
                (i_loop and o->local_Init_ಆಳ[i] >= (u32)i_loop->ಆಳ)) {
                ಮೌಲ್ಯ.type &= ~d_m3Type_refNonNull;
            }
            map->ಮೌಲ್ಯಗಳು[map->ಅಂಕೆ_ಮೌಲ್ಯಗಳು++] = ಮೌಲ್ಯ;
        }
        ಬಿಂದು->ಮೊದಲ_ಮೌಲ್ಯ = map->ಅಂಕೆ_ಮೌಲ್ಯಗಳು;
    }

    u16 ತುದಿ = o->ರಾಶಿ_ಸೂಚ್ಯಂಕ;

    ಆದರೆ (i_ಬಗೆ != safepoint_suspend) {
        ತುದಿ = (ತುದಿ > i_ಅಂಕೆ_Results) ? (u16)(ತುದಿ - i_ಅಂಕೆ_Results) : 0;
    }
    ಆದರೆ (i_loop) {
        ತುದಿ = M3_MIN(ತುದಿ, i_loop->exit_ರಾಶಿ_ಸೂಚ್ಯಂಕ);
    }

    ಗಾಗಿ (u16 i = o->ರಾಶಿ_ಮೊದಲ_Dynamic_ಸೂಚ್ಯಂಕ; i < ತುದಿ; ++i) {
        ಆದರೆ (Is_ಜೀವಂತ_ರಾಶಿ_ನಮೂದು(o, i)) {
_           (ಕೂಡಿಸು_ಸುರಕ್ಷಿತ_ಬಿಂದು_ಮೌಲ್ಯ(o, ಬಿಂದು, i));
        }
    }

    ಆದರೆ (i_loop) {
        u16 ಅಂಕೆ_Params = Get_Func_Type_ಅಂಕೆ_Params(i_loop->type);

        ಗಾಗಿ (u16 i = 0; i < ಅಂಕೆ_Params; ++i) {
_           (ಕೂಡಿಸು_ಸುರಕ್ಷಿತ_ಬಿಂದು_ಮೌಲ್ಯ(o, ಬಿಂದು, (u16)(i_loop->exit_ರಾಶಿ_ಸೂಚ್ಯಂಕ + i)));
        }
    }

    map->ಅಂಕೆ_ಸುರಕ್ಷಿತ_Points++;

} _catch: ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}

// A back edge: op_ContinueLoop, or op_ContinueLoopIf when i_isConditional, and
// the loop it goes back to. The conditional one suspends before it has taken
// the branch, so it has to be resumed with the condition that took it.
ಸ್ಥಿರ
M3_ಫಲಿತಾಂಶ ದಾಖಲೆ_ಹಿಂಭಾಗ_ಅಂಚು (IM3Compilation o, IM3CompilationScope i_loop, bool i_isConditional)
{
    M3_ಫಲಿತಾಂಶ ಫಲಿತಾಂಶ = m3Err_none;

    // the loop is named by its record in the map, which has to fit the field
    _throwif("a function has too many loops for its snapshot map",
             o->snapshotMap and o->ಪುಟ and (i_loop->snapshot_ಖಂಡ == 0 or i_loop->snapshot_ಖಂಡ > UINT16_MAX + 1u));

_   (ದಾಖಲೆ_ಸುರಕ್ಷಿತ_ಬಿಂದು(o, safepoint_op, 2, 0, (u16)(i_loop->snapshot_ಖಂಡ - 1), i_loop));

    ಆದರೆ (i_isConditional and o->snapshotMap and o->ಪುಟ) {
        o->snapshotMap->ಸುರಕ್ಷಿತ_Points[o->snapshotMap->ಅಂಕೆ_ಸುರಕ್ಷಿತ_Points - 1].flags |= d_m3_ಸುರಕ್ಷಿತ_ಬಿಂದು_ತೆಗೆದುಕೊಂಡ_Branch;
    }

    _catch: ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}

// A call, waiting on its callee with i_numResults not yet written; the
// callee's frame starts i_calleeSlot slots above this one. A return_call that
// is compiled as a call is one too, since its frame stands while the callee
// runs, but it is marked: in Wasm the callee has replaced it, so a snapshot
// walks past it rather than writing it.
ಸ್ಥಿರ
M3_ಫಲಿತಾಂಶ ದಾಖಲೆ_Call (IM3Compilation o, u16 i_ಅಂಕೆ_Results, u16 i_callee_ಸ್ಥಾನ, bool i_isReturnCall)
{
    M3_ಫಲಿತಾಂಶ ಫಲಿತಾಂಶ = ದಾಖಲೆ_ಸುರಕ್ಷಿತ_ಬಿಂದು(o, safepoint_call, 0, i_ಅಂಕೆ_Results, i_callee_ಸ್ಥಾನ, ಖಾಲಿ);

    ಆದರೆ (not ಫಲಿತಾಂಶ and i_isReturnCall and o->snapshotMap and o->ಪುಟ) {
        o->snapshotMap->ಸುರಕ್ಷಿತ_Points[o->snapshotMap->ಅಂಕೆ_ಸುರಕ್ಷಿತ_Points - 1].flags |= d_m3_ಸುರಕ್ಷಿತ_ಬಿಂದು_ಬಾಲ_Call;
    }

    ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}

// Where the function's arguments and locals are kept. Unlike the operand
// stack, that never changes, so it is recorded once rather than at every
// safepoint.
ಸ್ಥಿರ
M3_ಫಲಿತಾಂಶ ದಾಖಲೆ_Locals (IM3Compilation o)
{
    M3SnapshotMap* map = o->snapshotMap;

    ಆದರೆ (map and o->ರಾಶಿ_ಸೂಚ್ಯಂಕ) {
        map->locals = m3_AllocArray(M3_ಸ್ಥಾನ_ಮೌಲ್ಯ, o->ರಾಶಿ_ಸೂಚ್ಯಂಕ);

        ಆದರೆ (not map->locals) {
            ಹಿಂತಿರುಗಿ m3Err_mallocFailed;
        }

        ಗಾಗಿ (u16 i = 0; i < o->ರಾಶಿ_ಸೂಚ್ಯಂಕ; ++i) {
            map->locals[i].ಸ್ಥಾನ = o->wasm_ರಾಶಿ[i];
            map->locals[i].type = o->type_ರಾಶಿ[i];
        }

        map->ಅಂಕೆ_Locals = o->ರಾಶಿ_ಸೂಚ್ಯಂಕ;
        ಗಾಗಿ (u32 i = Get_Function_ಅಂಕೆ_ವಾದಗಳು(o->function); i < map->ಅಂಕೆ_Locals; ++i) {
            ಆದರೆ (IsRefType(map->locals[i].type) and not IsNullableRef(map->locals[i].type)) {
                o->local_Init_ಆಳ = m3_AllocArray(u32, map->ಅಂಕೆ_Locals);
                ಆದರೆ (not o->local_Init_ಆಳ) {
                    ಹಿಂತಿರುಗಿ m3Err_mallocFailed;
                }
                ಗಾಗಿ (u32 j = i; j < map->ಅಂಕೆ_Locals; ++j) {
                    ಆದರೆ (IsRefType(map->locals[j].type) and not IsNullableRef(map->locals[j].type)) {
                        o->local_Init_ಆಳ[j] = UINT32_MAX;
                    }
                }
                ನಿಲ್ಲಿಸು;
            }
        }
    }

    ಹಿಂತಿರುಗಿ m3Err_none;
}

// A loop or a try_table, whose native frame a snapshot names by the
// instruction's offset. i_pc is what that frame records. The scope being
// compiled keeps the record, so that RecordBlockEnd can close it.
ಸ್ಥಿರ
M3_ಫಲಿತಾಂಶ ದಾಖಲೆ_ಖಂಡ_ಆರಂಭ (IM3Compilation o, m3opcode_t i_opcode, pc_t i_pc, u32 i_ಅಂಕೆ_Clauses)
{
    M3SnapshotMap* map = o->snapshotMap;

    ಆದರೆ (map and o->ಪುಟ) {
        d_m3GrowSnapshotArray(M3_ಖಂಡ_ಆರಂಭ, map->ಖಂಡಗಳು, map->ಅಂಕೆ_ಖಂಡಗಳು, o->ಖಂಡಗಳು_ಸಾಮರ್ಥ್ಯ);

        M3_ಖಂಡ_ಆರಂಭ* ಖಂಡ = &map->ಖಂಡಗಳು[map->ಅಂಕೆ_ಖಂಡಗಳು++];
        ಖಂಡ->pc           = i_pc;
        ಖಂಡ->wasm_ಸ್ಥಾನಾಂತರ   = Get_Wasm_ಸ್ಥಾನಾಂತರ(o);
        ಖಂಡ->wasm_ಕೊನೆ      = UINT32_MAX;
        ಖಂಡ->ಅಂಕೆ_Clauses   = (u16)i_ಅಂಕೆ_Clauses;
        ಖಂಡ->opcode       = (u8)i_opcode;

        o->ಖಂಡ.snapshot_ಖಂಡ = map->ಅಂಕೆ_ಖಂಡಗಳು;
    }

    ಹಿಂತಿರುಗಿ m3Err_none;
}

// Closes the scope's region, while the instruction just read is its end
ಸ್ಥಿರ
ಶೂನ್ಯ ದಾಖಲೆ_ಖಂಡ_ಕೊನೆ (IM3Compilation o)
{
    ಆದರೆ (o->snapshotMap and o->ಖಂಡ.snapshot_ಖಂಡ) {
        o->snapshotMap->ಖಂಡಗಳು[o->ಖಂಡ.snapshot_ಖಂಡ - 1].wasm_ಕೊನೆ = Get_Wasm_ಸ್ಥಾನಾಂತರ(o);
    }
}

#  undef d_m3GrowSnapshotArray

#ಇಲ್ಲದಿದ್ದರೆ

ಸ್ಥಿರ ಸಾಲೊಳಗೆ
M3_ಫಲಿತಾಂಶ ದಾಖಲೆ_ಸುರಕ್ಷಿತ_ಬಿಂದು (IM3Compilation o, M3_ಸುರಕ್ಷಿತ_ಬಿಂದು_ಬಗೆ i_ಬಗೆ, u32 i_ಪದಗಳು_ಹಿಂಭಾಗ, u16 i_ಅಂಕೆ_Results,
                          u16 i_aux, IM3CompilationScope i_loop)
{
    (ಶೂನ್ಯ)o;
    (ಶೂನ್ಯ)i_ಬಗೆ;
    (ಶೂನ್ಯ)i_ಪದಗಳು_ಹಿಂಭಾಗ;
    (ಶೂನ್ಯ)i_ಅಂಕೆ_Results;
    (ಶೂನ್ಯ)i_aux;
    (ಶೂನ್ಯ)i_loop;
    ಹಿಂತಿರುಗಿ m3Err_none;
}

ಸ್ಥಿರ ಸಾಲೊಳಗೆ
M3_ಫಲಿತಾಂಶ ದಾಖಲೆ_ಹಿಂಭಾಗ_ಅಂಚು (IM3Compilation o, IM3CompilationScope i_loop, bool i_isConditional)
{
    (ಶೂನ್ಯ)o;
    (ಶೂನ್ಯ)i_loop;
    (ಶೂನ್ಯ)i_isConditional;
    ಹಿಂತಿರುಗಿ m3Err_none;
}

ಸ್ಥಿರ ಸಾಲೊಳಗೆ
M3_ಫಲಿತಾಂಶ ದಾಖಲೆ_Call (IM3Compilation o, u16 i_ಅಂಕೆ_Results, u16 i_callee_ಸ್ಥಾನ, bool i_isReturnCall)
{
    (ಶೂನ್ಯ)o;
    (ಶೂನ್ಯ)i_ಅಂಕೆ_Results;
    (ಶೂನ್ಯ)i_callee_ಸ್ಥಾನ;
    (ಶೂನ್ಯ)i_isReturnCall;
    ಹಿಂತಿರುಗಿ m3Err_none;
}

#ಆದರೆ_ಅಂತ್ಯ // d_m3HasSnapshots

ಸ್ಥಿರ M3_NOINLINE
M3_ಫಲಿತಾಂಶ Ensure_ಸಂಕೇತ_ಪುಟ_ಅಂಕೆ_ಸಾಲುಗಳು (IM3Compilation o, u32 i_ಅಂಕೆ_ಸಾಲುಗಳು)
{
    M3_ಫಲಿತಾಂಶ ಫಲಿತಾಂಶ = m3Err_none;

    i_ಅಂಕೆ_ಸಾಲುಗಳು += 2; // room for Bridge

    ಆದರೆ (M3_UNLIKELY(ಅಂಕೆ_Free_ಸಾಲುಗಳು(o->ಪುಟ) < i_ಅಂಕೆ_ಸಾಲುಗಳು)) {
        IM3_ಸಂಕೇತ_ಪುಟ ಪುಟ = Acquire_ಸಂಕೇತ_ಪುಟ_With_ಸಾಮರ್ಥ್ಯ(o->runtime, i_ಅಂಕೆ_ಸಾಲುಗಳು);

        ಆದರೆ (ಪುಟ) {
            m3log(emit, "bridging new code page from: %d %p (free slots: %d) to: %d", o->ಪುಟ->info.sequence, GetPC(o), ಅಂಕೆ_Free_ಸಾಲುಗಳು(o->ಪುಟ), ಪುಟ->info.sequence);
            d_m3Assert(ಅಂಕೆ_Free_ಸಾಲುಗಳು(o->ಪುಟ) >= 2);

            Emit_ಶಬ್ದ(o->ಪುಟ, op_Branch);
            Emit_ಶಬ್ದ(o->ಪುಟ, Get_ಪುಟ_PC(ಪುಟ));

            Release_ಸಂಕೇತ_ಪುಟ(o->runtime, o->ಪುಟ);

            o->ಪುಟ = ಪುಟ;
        } ಇಲ್ಲದಿದ್ದರೆ {
            ಫಲಿತಾಂಶ = m3_Err_malloc_Failed_ಸಂಕೇತ_ಪುಟ;
        }
    }

    ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}

// invalidate the pending local.set fold candidate; called wherever code is emitted or a
// branch target captures the current pc (a fold appends an immediate, moving that position)
ಸ್ಥಿರ ಸಾಲೊಳಗೆ
ಶೂನ್ಯ InvalidateFold (IM3Compilation o)
{
#ಆದರೆ d_m3_Fold_ಗಣ_Local
    o->foldPatchPC = ಖಾಲಿ;
#ಇಲ್ಲದಿದ್ದರೆ
    (ಶೂನ್ಯ)o;
#ಆದರೆ_ಅಂತ್ಯ
}

ಸ್ಥಿರ M3_NOINLINE
M3_ಫಲಿತಾಂಶ EmitOp (IM3Compilation o, IM3Operation i_operation)
{
    M3_ಫಲಿತಾಂಶ ಫಲಿತಾಂಶ = m3Err_none;                                 d_m3Assert (i_operation or Is_ರಾಶಿ_Polymorphic (o));

    InvalidateFold(o);

    // it's OK for page to be null; when compile-walking the bytecode without emitting
    ಆದರೆ (o->ಪುಟ) {
#ಆದರೆ d_m3EnableOpTracing
        ಆದರೆ (i_operation != op_Dump_ರಾಶಿ) {
            o->ಅಂಕೆ_Emits++;
        }
#ಆದರೆ_ಅಂತ್ಯ

        // have execution jump to a new page if slots are critically low
        ಫಲಿತಾಂಶ = Ensure_ಸಂಕೇತ_ಪುಟ_ಅಂಕೆ_ಸಾಲುಗಳು(o, d_m3_ಸಂಕೇತ_ಪುಟ_Free_ಸಾಲುಗಳು_ಮಿತಿ);

        ಆದರೆ (M3_LIKELY(not ಫಲಿತಾಂಶ)) {
            ಆದರೆ (d_m3LogEmit) {
                log_emit(o, i_operation);
            }
#ಆದರೆ d_m3_ದಾಖಲೆ_Backtraces
            Emit_Mapping_ನಮೂದು(o->ಪುಟ, (u32)(o->ಕೊನೆಯ_Opcode_ಆರಂಭ - o->module->wasm_ಆರಂಭ));
#ಆದರೆ_ಅಂತ್ಯ // d_m3RecordBacktraces
            Emit_ಶಬ್ದ(o->ಪುಟ, i_operation);
        }
    }

    ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}

// Push an immediate constant into the M3 codestream
ಸ್ಥಿರ M3_NOINLINE
ಶೂನ್ಯ EmitConstant32 (IM3Compilation o, ಸ್ಥಿರಾಂಕ u32 i_ತಕ್ಷಣ)
{
    ಆದರೆ (o->ಪುಟ) {                                                          m3log (emit, "const32: %ud", i_ತಕ್ಷಣ);
        EmitWord32(o->ಪುಟ, i_ತಕ್ಷಣ);
    }
}

#ಆದರೆ d_m3HasMemory64
// Takes two lines of the code page where a pointer is 32 bits, so whatever
// reads it back has to step _pc by the same amount - see op_Const64.
ಸ್ಥಿರ M3_NOINLINE
ಶೂನ್ಯ EmitConstant64 (IM3Compilation o, ಸ್ಥಿರಾಂಕ u64 i_ತಕ್ಷಣ)
{
    ಆದರೆ (o->ಪುಟ) {
        EmitWord64(o->ಪುಟ, i_ತಕ್ಷಣ);
    }
}
#ಆದರೆ_ಅಂತ್ಯ

ಸ್ಥಿರ M3_NOINLINE
ಶೂನ್ಯ Emit_ಸ್ಥಾನ_ಸ್ಥಾನಾಂತರ (IM3Compilation o, ಸ್ಥಿರಾಂಕ i32 i_ಸ್ಥಾನಾಂತರ)
{
    ಆದರೆ (o->ಪುಟ) {                                                          m3log (emit, "slot: [%d]", i_ಸ್ಥಾನಾಂತರ);
        EmitWord32(o->ಪುಟ, i_ಸ್ಥಾನಾಂತರ);
    }
}

ಸ್ಥಿರ M3_NOINLINE
pc_t Emit_ಸೂಚಕ (IM3Compilation o, ಸ್ಥಿರಾಂಕ ಶೂನ್ಯ* ಸ್ಥಿರಾಂಕ i_ಸೂಚಕ)
{
    pc_t ptr = Get_ಪುಟ_PC(o->ಪುಟ);

    ಆದರೆ (o->ಪುಟ) {                                                          m3log (emit, "ptr: %p", i_ಸೂಚಕ);
        Emit_ಶಬ್ದ(o->ಪುಟ, i_ಸೂಚಕ);
    }

    ಹಿಂತಿರುಗಿ ptr;
}

ಸ್ಥಿರ M3_NOINLINE
ಶೂನ್ಯ* Reserve_ಸೂಚಕ (IM3Compilation o)
{
    pc_t ptr = Get_ಪುಟ_PC(o->ಪುಟ);
    Emit_ಸೂಚಕ(o, ಖಾಲಿ);
    ಹಿಂತಿರುಗಿ (ಶೂನ್ಯ*)ptr;
}


//-------------------------------------------------------------------------------------------------------------------------

#ವ್ಯಾಖ್ಯಾನಿಸು d_indent "     | %s"

// just want less letters and numbers to stare at down the way in the compiler table
#ವ್ಯಾಖ್ಯಾನಿಸು i_32    c_m3Type_i32
#ವ್ಯಾಖ್ಯಾನಿಸು i_64    c_m3Type_i64
#ವ್ಯಾಖ್ಯಾನಿಸು f_32    c_m3Type_f32
#ವ್ಯಾಖ್ಯಾನಿಸು f_64    c_m3Type_f64
#ವ್ಯಾಖ್ಯಾನಿಸು none    c_m3Type_none
#ವ್ಯಾಖ್ಯಾನಿಸು any     (u8)-1

#ಆದರೆ d_m3HasFloat
#  define FPOP(x) x
#ಇಲ್ಲದಿದ್ದರೆ
#  define FPOP(x) ಖಾಲಿ
#ಆದರೆ_ಅಂತ್ಯ

// These are indexed by M3ValueType, so every type up to c_m3Type_externref needs
// an entry. A reference is one pointer-sized word, so it moves with the integer
// operation of that width; v128 has no operations at all.
#ಆದರೆ M3_SIZEOF_PTR == 8
#  define REFOP(NAME)  op_##NAME##_i64
#ಇಲ್ಲದಿದ್ದರೆ
#  define REFOP(NAME)  op_##NAME##_i32
#ಆದರೆ_ಅಂತ್ಯ

// clang-format off

ಸ್ಥಿರ ಸ್ಥಿರಾಂಕ IM3Operation c_preserve_ಗಣ_ಸ್ಥಾನ[] = { ಖಾಲಿ, op_Preserve_ಗಣ_ಸ್ಥಾನ_i32,       op_Preserve_ಗಣ_ಸ್ಥಾನ_i64,
                                                    FPOP(op_Preserve_ಗಣ_ಸ್ಥಾನ_f32), FPOP(op_Preserve_ಗಣ_ಸ್ಥಾನ_f64),
                                                    ಖಾಲಿ, REFOP(Preserve_ಗಣ_ಸ್ಥಾನ),  REFOP(Preserve_ಗಣ_ಸ್ಥಾನ),
                                                    REFOP(Preserve_ಗಣ_ಸ್ಥಾನ),       REFOP(Preserve_ಗಣ_ಸ್ಥಾನ) };
ಸ್ಥಿರ ಸ್ಥಿರಾಂಕ IM3Operation c_ಗಣ_ಗಣ_Ops[] =       { ಖಾಲಿ, op_ಗಣ_ಸ್ಥಾನ_i32,               op_ಗಣ_ಸ್ಥಾನ_i64,
                                                    FPOP(op_ಗಣ_ಸ್ಥಾನ_f32),         FPOP(op_ಗಣ_ಸ್ಥಾನ_f64),
                                                    ಖಾಲಿ, REFOP(ಗಣ_ಸ್ಥಾನ),         REFOP(ಗಣ_ಸ್ಥಾನ),
                                                    REFOP(ಗಣ_ಸ್ಥಾನ),               REFOP(ಗಣ_ಸ್ಥಾನ) };
ಸ್ಥಿರ ಸ್ಥಿರಾಂಕ IM3Operation c_ಗಣ_Global_Ops[] =    { ಖಾಲಿ, op_ಗಣ_Global_i32,             op_ಗಣ_Global_i64,
                                                    FPOP(op_ಗಣ_Global_f32),       FPOP(op_ಗಣ_Global_f64),
                                                    ಖಾಲಿ, REFOP(ಗಣ_Global),       REFOP(ಗಣ_Global),
                                                    REFOP(ಗಣ_Global),             REFOP(ಗಣ_Global) };
ಸ್ಥಿರ ಸ್ಥಿರಾಂಕ IM3Operation c_ಗಣ_ರಿಜಿಸ್ಟರ್_Ops[] =  { ಖಾಲಿ, op_ಗಣ_ರಿಜಿಸ್ಟರ್_i32,           op_ಗಣ_ರಿಜಿಸ್ಟರ್_i64,
                                                    FPOP(op_ಗಣ_ರಿಜಿಸ್ಟರ್_f32),     FPOP(op_ಗಣ_ರಿಜಿಸ್ಟರ್_f64),
                                                    ಖಾಲಿ, REFOP(ಗಣ_ರಿಜಿಸ್ಟರ್),     REFOP(ಗಣ_ರಿಜಿಸ್ಟರ್),
                                                    REFOP(ಗಣ_ರಿಜಿಸ್ಟರ್),           REFOP(ಗಣ_ರಿಜಿಸ್ಟರ್) };

// A table shorter than the enum reads out of bounds, so tie the two together at
// build time: adding a value type must not silently outgrow these.
#ವ್ಯಾಖ್ಯಾನಿಸು d_m3_Check_Type_ಕೋಷ್ಟಕ(ಕೋಷ್ಟಕ) \
    M3_STATIC_ASSERT (ಗಾತ್ರ (ಕೋಷ್ಟಕ) / ಗಾತ್ರ (*(ಕೋಷ್ಟಕ)) == c_m3_Type_ಎಣಿಕೆ, ಕೋಷ್ಟಕ##_needs_one_entry_per_type)

d_m3_Check_Type_ಕೋಷ್ಟಕ(c_preserve_ಗಣ_ಸ್ಥಾನ);
d_m3_Check_Type_ಕೋಷ್ಟಕ(c_ಗಣ_ಗಣ_Ops);
d_m3_Check_Type_ಕೋಷ್ಟಕ(c_ಗಣ_Global_Ops);
d_m3_Check_Type_ಕೋಷ್ಟಕ(c_ಗಣ_ರಿಜಿಸ್ಟರ್_Ops);

#ಆದರೆ d_m3_Fold_ಗಣ_Local

// destination-folded variants of the hot binops, indexed by the operand-form index
// recorded at emission: [0]=_rs [1]=_sr [2]=_ss [3]=(fp _rr, never folded)
#ವ್ಯಾಖ್ಯಾನಿಸು d_foldOpList(TYPE, NAME)            { op_##TYPE##_##NAME##_rs_f, op_##TYPE##_##NAME##_sr_f, op_##TYPE##_##NAME##_ss_f, ಖಾಲಿ }
#ವ್ಯಾಖ್ಯಾನಿಸು d_foldCommutativeOpList(TYPE, NAME) { op_##TYPE##_##NAME##_rs_f, ಖಾಲಿ,                      op_##TYPE##_##NAME##_ss_f, ಖಾಲಿ }

ಸ್ಥಿರ ಸ್ಥಿರಾಂಕ IM3Operation c_fold_i32_ಕೂಡಿಸು[]         = d_foldCommutativeOpList (i32, ಕೂಡಿಸು);
ಸ್ಥಿರ ಸ್ಥಿರಾಂಕ IM3Operation c_fold_i32_ಕಳೆ[]    = d_foldOpList (i32, ಕಳೆ);
ಸ್ಥಿರ ಸ್ಥಿರಾಂಕ IM3Operation c_fold_i32_Multiply[]    = d_foldCommutativeOpList (i32, Multiply);
ಸ್ಥಿರ ಸ್ಥಿರಾಂಕ IM3Operation c_fold_u32_And[]         = d_foldCommutativeOpList (u32, And);
ಸ್ಥಿರ ಸ್ಥಿರಾಂಕ IM3Operation c_fold_u32_Or[]          = d_foldCommutativeOpList (u32, Or);
ಸ್ಥಿರ ಸ್ಥಿರಾಂಕ IM3Operation c_fold_u32_Xor[]         = d_foldCommutativeOpList (u32, Xor);
ಸ್ಥಿರ ಸ್ಥಿರಾಂಕ IM3Operation c_fold_u32_ಸರಿಸು_ಎಡ[]   = d_foldOpList (u32, ಸರಿಸು_ಎಡ);
ಸ್ಥಿರ ಸ್ಥಿರಾಂಕ IM3Operation c_fold_i32_ಸರಿಸು_ಬಲ[]  = d_foldOpList (i32, ಸರಿಸು_ಬಲ);
ಸ್ಥಿರ ಸ್ಥಿರಾಂಕ IM3Operation c_fold_u32_ಸರಿಸು_ಬಲ[]  = d_foldOpList (u32, ಸರಿಸು_ಬಲ);

ಸ್ಥಿರ ಸ್ಥಿರಾಂಕ IM3Operation c_fold_i64_ಕೂಡಿಸು[]         = d_foldCommutativeOpList (i64, ಕೂಡಿಸು);
ಸ್ಥಿರ ಸ್ಥಿರಾಂಕ IM3Operation c_fold_i64_ಕಳೆ[]    = d_foldOpList (i64, ಕಳೆ);
ಸ್ಥಿರ ಸ್ಥಿರಾಂಕ IM3Operation c_fold_i64_Multiply[]    = d_foldCommutativeOpList (i64, Multiply);
ಸ್ಥಿರ ಸ್ಥಿರಾಂಕ IM3Operation c_fold_u64_And[]         = d_foldCommutativeOpList (u64, And);
ಸ್ಥಿರ ಸ್ಥಿರಾಂಕ IM3Operation c_fold_u64_Or[]          = d_foldCommutativeOpList (u64, Or);
ಸ್ಥಿರ ಸ್ಥಿರಾಂಕ IM3Operation c_fold_u64_Xor[]         = d_foldCommutativeOpList (u64, Xor);
ಸ್ಥಿರ ಸ್ಥಿರಾಂಕ IM3Operation c_fold_u64_ಸರಿಸು_ಎಡ[]   = d_foldOpList (u64, ಸರಿಸು_ಎಡ);
ಸ್ಥಿರ ಸ್ಥಿರಾಂಕ IM3Operation c_fold_i64_ಸರಿಸು_ಬಲ[]  = d_foldOpList (i64, ಸರಿಸು_ಬಲ);
ಸ್ಥಿರ ಸ್ಥಿರಾಂಕ IM3Operation c_fold_u64_ಸರಿಸು_ಬಲ[]  = d_foldOpList (u64, ಸರಿಸು_ಬಲ);

#  ಆದರೆ d_m3HasFloat
ಸ್ಥಿರ ಸ್ಥಿರಾಂಕ IM3Operation c_fold_f32_ಕೂಡಿಸು[]         = d_foldCommutativeOpList (f32, ಕೂಡಿಸು);
ಸ್ಥಿರ ಸ್ಥಿರಾಂಕ IM3Operation c_fold_f32_ಕಳೆ[]    = d_foldOpList (f32, ಕಳೆ);
ಸ್ಥಿರ ಸ್ಥಿರಾಂಕ IM3Operation c_fold_f32_Multiply[]    = d_foldCommutativeOpList (f32, Multiply);
ಸ್ಥಿರ ಸ್ಥಿರಾಂಕ IM3Operation c_fold_f32_Divide[]      = d_foldOpList (f32, Divide);
ಸ್ಥಿರ ಸ್ಥಿರಾಂಕ IM3Operation c_fold_f64_ಕೂಡಿಸು[]         = d_foldCommutativeOpList (f64, ಕೂಡಿಸು);
ಸ್ಥಿರ ಸ್ಥಿರಾಂಕ IM3Operation c_fold_f64_ಕಳೆ[]    = d_foldOpList (f64, ಕಳೆ);
ಸ್ಥಿರ ಸ್ಥಿರಾಂಕ IM3Operation c_fold_f64_Multiply[]    = d_foldCommutativeOpList (f64, Multiply);
ಸ್ಥಿರ ಸ್ಥಿರಾಂಕ IM3Operation c_fold_f64_Divide[]      = d_foldOpList (f64, Divide);
#  endif

// loads are unary: [0]= address in _r0, [1]= address in a slot
#ವ್ಯಾಖ್ಯಾನಿಸು d_fold_ತುಂಬು_List(DEST, SRC)       { op_##DEST##_Load_##SRC##_r_f, op_##DEST##_Load_##SRC##_s_f, ಖಾಲಿ, ಖಾಲಿ }

ಸ್ಥಿರ ಸ್ಥಿರಾಂಕ IM3Operation c_fold_i32_ತುಂಬು_i32[] = d_fold_ತುಂಬು_List(i32, i32);
ಸ್ಥಿರ ಸ್ಥಿರಾಂಕ IM3Operation c_fold_i32_ತುಂಬು_i8[] = d_fold_ತುಂಬು_List(i32, i8);
ಸ್ಥಿರ ಸ್ಥಿರಾಂಕ IM3Operation c_fold_i32_ತುಂಬು_u8[] = d_fold_ತುಂಬು_List(i32, u8);
ಸ್ಥಿರ ಸ್ಥಿರಾಂಕ IM3Operation c_fold_i32_ತುಂಬು_i16[] = d_fold_ತುಂಬು_List(i32, i16);
ಸ್ಥಿರ ಸ್ಥಿರಾಂಕ IM3Operation c_fold_i32_ತುಂಬು_u16[] = d_fold_ತುಂಬು_List(i32, u16);

ಸ್ಥಿರ ಸ್ಥಿರಾಂಕ IM3Operation c_fold_i64_ತುಂಬು_i64[] = d_fold_ತುಂಬು_List(i64, i64);
ಸ್ಥಿರ ಸ್ಥಿರಾಂಕ IM3Operation c_fold_i64_ತುಂಬು_i8[] = d_fold_ತುಂಬು_List(i64, i8);
ಸ್ಥಿರ ಸ್ಥಿರಾಂಕ IM3Operation c_fold_i64_ತುಂಬು_u8[] = d_fold_ತುಂಬು_List(i64, u8);
ಸ್ಥಿರ ಸ್ಥಿರಾಂಕ IM3Operation c_fold_i64_ತುಂಬು_i16[] = d_fold_ತುಂಬು_List(i64, i16);
ಸ್ಥಿರ ಸ್ಥಿರಾಂಕ IM3Operation c_fold_i64_ತುಂಬು_u16[] = d_fold_ತುಂಬು_List(i64, u16);
ಸ್ಥಿರ ಸ್ಥಿರಾಂಕ IM3Operation c_fold_i64_ತುಂಬು_i32[] = d_fold_ತುಂಬು_List(i64, i32);
ಸ್ಥಿರ ಸ್ಥಿರಾಂಕ IM3Operation c_fold_i64_ತುಂಬು_u32[] = d_fold_ತುಂಬು_List(i64, u32);
#  ಆದರೆ d_m3HasFloat
ಸ್ಥಿರ ಸ್ಥಿರಾಂಕ IM3Operation c_fold_f32_ತುಂಬು_f32[] = d_fold_ತುಂಬು_List(f32, f32);
ಸ್ಥಿರ ಸ್ಥಿರಾಂಕ IM3Operation c_fold_f64_ತುಂಬು_f64[] = d_fold_ತುಂಬು_List(f64, f64);
#  endif

ಸ್ಥಿರ
IM3Operation GetFoldOp (m3opcode_t i_opcode, u8 i_form)
{
    ಆದರೆ (i_form >= 4) {
        ಹಿಂತಿರುಗಿ ಖಾಲಿ;
    }

    ಆಯ್ಕೆ (i_opcode) {
    ಸ್ಥಿತಿ 0x6a:  ಹಿಂತಿರುಗಿ c_fold_i32_ಕೂಡಿಸು        [i_form];      // i32.add
    ಸ್ಥಿತಿ 0x6b:  ಹಿಂತಿರುಗಿ c_fold_i32_ಕಳೆ   [i_form];      // i32.sub
    ಸ್ಥಿತಿ 0x6c:  ಹಿಂತಿರುಗಿ c_fold_i32_Multiply   [i_form];      // i32.mul
    ಸ್ಥಿತಿ 0x71:  ಹಿಂತಿರುಗಿ c_fold_u32_And        [i_form];      // i32.and
    ಸ್ಥಿತಿ 0x72:  ಹಿಂತಿರುಗಿ c_fold_u32_Or         [i_form];      // i32.or
    ಸ್ಥಿತಿ 0x73:  ಹಿಂತಿರುಗಿ c_fold_u32_Xor        [i_form];      // i32.xor
    ಸ್ಥಿತಿ 0x74:  ಹಿಂತಿರುಗಿ c_fold_u32_ಸರಿಸು_ಎಡ  [i_form];      // i32.shl
    ಸ್ಥಿತಿ 0x75:  ಹಿಂತಿರುಗಿ c_fold_i32_ಸರಿಸು_ಬಲ [i_form];      // i32.shr_s
    ಸ್ಥಿತಿ 0x76:  ಹಿಂತಿರುಗಿ c_fold_u32_ಸರಿಸು_ಬಲ [i_form];      // i32.shr_u

    ಸ್ಥಿತಿ 0x7c:  ಹಿಂತಿರುಗಿ c_fold_i64_ಕೂಡಿಸು        [i_form];      // i64.add
    ಸ್ಥಿತಿ 0x7d:  ಹಿಂತಿರುಗಿ c_fold_i64_ಕಳೆ   [i_form];      // i64.sub
    ಸ್ಥಿತಿ 0x7e:  ಹಿಂತಿರುಗಿ c_fold_i64_Multiply   [i_form];      // i64.mul
    ಸ್ಥಿತಿ 0x83:  ಹಿಂತಿರುಗಿ c_fold_u64_And        [i_form];      // i64.and
    ಸ್ಥಿತಿ 0x84:  ಹಿಂತಿರುಗಿ c_fold_u64_Or         [i_form];      // i64.or
    ಸ್ಥಿತಿ 0x85:  ಹಿಂತಿರುಗಿ c_fold_u64_Xor        [i_form];      // i64.xor
    ಸ್ಥಿತಿ 0x86:  ಹಿಂತಿರುಗಿ c_fold_u64_ಸರಿಸು_ಎಡ  [i_form];      // i64.shl
    ಸ್ಥಿತಿ 0x87:  ಹಿಂತಿರುಗಿ c_fold_i64_ಸರಿಸು_ಬಲ [i_form];      // i64.shr_s
    ಸ್ಥಿತಿ 0x88:  ಹಿಂತಿರುಗಿ c_fold_u64_ಸರಿಸು_ಬಲ [i_form];      // i64.shr_u

#  ಆದರೆ d_m3HasFloat
    ಸ್ಥಿತಿ 0x92:  ಹಿಂತಿರುಗಿ c_fold_f32_ಕೂಡಿಸು        [i_form];      // f32.add
    ಸ್ಥಿತಿ 0x93:  ಹಿಂತಿರುಗಿ c_fold_f32_ಕಳೆ   [i_form];      // f32.sub
    ಸ್ಥಿತಿ 0x94:  ಹಿಂತಿರುಗಿ c_fold_f32_Multiply   [i_form];      // f32.mul
    ಸ್ಥಿತಿ 0x95:  ಹಿಂತಿರುಗಿ c_fold_f32_Divide     [i_form];      // f32.div
    ಸ್ಥಿತಿ 0xa0:  ಹಿಂತಿರುಗಿ c_fold_f64_ಕೂಡಿಸು        [i_form];      // f64.add
    ಸ್ಥಿತಿ 0xa1:  ಹಿಂತಿರುಗಿ c_fold_f64_ಕಳೆ   [i_form];      // f64.sub
    ಸ್ಥಿತಿ 0xa2:  ಹಿಂತಿರುಗಿ c_fold_f64_Multiply   [i_form];      // f64.mul
    ಸ್ಥಿತಿ 0xa3:  ಹಿಂತಿರುಗಿ c_fold_f64_Divide     [i_form];      // f64.div
#  endif

    ಸ್ಥಿತಿ 0x28:  ಹಿಂತಿರುಗಿ c_fold_i32_ತುಂಬು_i32   [i_form];      // i32.load
    ಸ್ಥಿತಿ 0x2c:  ಹಿಂತಿರುಗಿ c_fold_i32_ತುಂಬು_i8    [i_form];      // i32.load8_s
    ಸ್ಥಿತಿ 0x2d:  ಹಿಂತಿರುಗಿ c_fold_i32_ತುಂಬು_u8    [i_form];      // i32.load8_u
    ಸ್ಥಿತಿ 0x2e:  ಹಿಂತಿರುಗಿ c_fold_i32_ತುಂಬು_i16   [i_form];      // i32.load16_s
    ಸ್ಥಿತಿ 0x2f:  ಹಿಂತಿರುಗಿ c_fold_i32_ತುಂಬು_u16   [i_form];      // i32.load16_u

    ಸ್ಥಿತಿ 0x29:  ಹಿಂತಿರುಗಿ c_fold_i64_ತುಂಬು_i64   [i_form];      // i64.load
    ಸ್ಥಿತಿ 0x30:  ಹಿಂತಿರುಗಿ c_fold_i64_ತುಂಬು_i8    [i_form];      // i64.load8_s
    ಸ್ಥಿತಿ 0x31:  ಹಿಂತಿರುಗಿ c_fold_i64_ತುಂಬು_u8    [i_form];      // i64.load8_u
    ಸ್ಥಿತಿ 0x32:  ಹಿಂತಿರುಗಿ c_fold_i64_ತುಂಬು_i16   [i_form];      // i64.load16_s
    ಸ್ಥಿತಿ 0x33:  ಹಿಂತಿರುಗಿ c_fold_i64_ತುಂಬು_u16   [i_form];      // i64.load16_u
    ಸ್ಥಿತಿ 0x34:  ಹಿಂತಿರುಗಿ c_fold_i64_ತುಂಬು_i32   [i_form];      // i64.load32_s
    ಸ್ಥಿತಿ 0x35:  ಹಿಂತಿರುಗಿ c_fold_i64_ತುಂಬು_u32   [i_form];      // i64.load32_u

#  ಆದರೆ d_m3HasFloat
    ಸ್ಥಿತಿ 0x2a:  ಹಿಂತಿರುಗಿ c_fold_f32_ತುಂಬು_f32   [i_form];      // f32.load
    ಸ್ಥಿತಿ 0x2b:  ಹಿಂತಿರುಗಿ c_fold_f64_ತುಂಬು_f64   [i_form];      // f64.load
#  endif

    ಪೂರ್ವನಿಯೋಜಿತ: ಹಿಂತಿರುಗಿ ಖಾಲಿ;
    }
}

#ಆದರೆ_ಅಂತ್ಯ // d_m3FoldSetLocal

// clang-format on

#ಆದರೆ d_m3FuseBranch

// fused compare+branch/if variants, indexed like the fold lists: [0]=_rs [1]=_sr [2]=_ss [3]=unused.
// the [1] entries are NULL for commutative compares: the compiler never records the _sr form for them
#  define d_fuseCmpList(TYPE, NAME, ಬಗೆ_2)             { op_##TYPE##_##KIND##_##NAME##_rs, op_##TYPE##_##KIND##_##NAME##_sr, op_##TYPE##_##KIND##_##NAME##_ss, ಖಾಲಿ }
#  define d_fuseCommutativeCmpList(TYPE, NAME, ಬಗೆ_2)  { op_##TYPE##_##KIND##_##NAME##_rs, ಖಾಲಿ,                             op_##TYPE##_##KIND##_##NAME##_ss, ಖಾಲಿ }

ಪ್ರಕಾರವ್ಯಾಖ್ಯೆ ರಚನೆ M3FusedCmpOps {
    IM3Operation branchIf[4];
    IM3Operation ifOp[4];
} M3FusedCmpOps;

#  define d_fuseCmp(TYPE, NAME)               { d_fuseCmpList (TYPE, NAME, BranchIf),            d_fuseCmpList (TYPE, NAME, If) }
#  define d_fuseCommutativeCmp(TYPE, NAME)    { d_fuseCommutativeCmpList (TYPE, NAME, BranchIf), d_fuseCommutativeCmpList (TYPE, NAME, If) }

// clang-format off
ಸ್ಥಿರ ಸ್ಥಿರಾಂಕ M3FusedCmpOps c_fuse_i32_Equal              = d_fuseCommutativeCmp (i32, Equal);
ಸ್ಥಿರ ಸ್ಥಿರಾಂಕ M3FusedCmpOps c_fuse_i32_NotEqual           = d_fuseCommutativeCmp (i32, NotEqual);
ಸ್ಥಿರ ಸ್ಥಿರಾಂಕ M3FusedCmpOps c_fuse_i32_LessThan           = d_fuseCmp (i32, LessThan);
ಸ್ಥಿರ ಸ್ಥಿರಾಂಕ M3FusedCmpOps c_fuse_u32_LessThan           = d_fuseCmp (u32, LessThan);
ಸ್ಥಿರ ಸ್ಥಿರಾಂಕ M3FusedCmpOps c_fuse_i32_GreaterThan        = d_fuseCmp (i32, GreaterThan);
ಸ್ಥಿರ ಸ್ಥಿರಾಂಕ M3FusedCmpOps c_fuse_u32_GreaterThan        = d_fuseCmp (u32, GreaterThan);
ಸ್ಥಿರ ಸ್ಥಿರಾಂಕ M3FusedCmpOps c_fuse_i32_LessThanOrEqual    = d_fuseCmp (i32, LessThanOrEqual);
ಸ್ಥಿರ ಸ್ಥಿರಾಂಕ M3FusedCmpOps c_fuse_u32_LessThanOrEqual    = d_fuseCmp (u32, LessThanOrEqual);
ಸ್ಥಿರ ಸ್ಥಿರಾಂಕ M3FusedCmpOps c_fuse_i32_GreaterThanOrEqual = d_fuseCmp (i32, GreaterThanOrEqual);
ಸ್ಥಿರ ಸ್ಥಿರಾಂಕ M3FusedCmpOps c_fuse_u32_GreaterThanOrEqual = d_fuseCmp (u32, GreaterThanOrEqual);
// clang-format on

// i32.eqz is unary: [0]= operand in _r0, [1]= operand in a slot
ಸ್ಥಿರ ಸ್ಥಿರಾಂಕ M3FusedCmpOps c_fuse_i32_Eqz = {
    { op_i32_BranchIfEqz_r, op_i32_BranchIfEqz_s, ಖಾಲಿ, ಖಾಲಿ },
    { op_i32_IfEqz_r,       op_i32_IfEqz_s,       ಖಾಲಿ, ಖಾಲಿ }
};

ಸ್ಥಿರ
ಸ್ಥಿರಾಂಕ M3FusedCmpOps* GetFusedCmpOps (m3opcode_t i_opcode)
{
    ಆಯ್ಕೆ (i_opcode) {
    ಸ್ಥಿತಿ 0x45: ಹಿಂತಿರುಗಿ &c_fuse_i32_Eqz;                    // i32.eqz
    ಸ್ಥಿತಿ 0x46: ಹಿಂತಿರುಗಿ &c_fuse_i32_Equal;                  // i32.eq
    ಸ್ಥಿತಿ 0x47: ಹಿಂತಿರುಗಿ &c_fuse_i32_NotEqual;               // i32.ne
    ಸ್ಥಿತಿ 0x48: ಹಿಂತಿರುಗಿ &c_fuse_i32_LessThan;               // i32.lt_s
    ಸ್ಥಿತಿ 0x49: ಹಿಂತಿರುಗಿ &c_fuse_u32_LessThan;               // i32.lt_u
    ಸ್ಥಿತಿ 0x4a: ಹಿಂತಿರುಗಿ &c_fuse_i32_GreaterThan;            // i32.gt_s
    ಸ್ಥಿತಿ 0x4b: ಹಿಂತಿರುಗಿ &c_fuse_u32_GreaterThan;            // i32.gt_u
    ಸ್ಥಿತಿ 0x4c: ಹಿಂತಿರುಗಿ &c_fuse_i32_LessThanOrEqual;        // i32.le_s
    ಸ್ಥಿತಿ 0x4d: ಹಿಂತಿರುಗಿ &c_fuse_u32_LessThanOrEqual;        // i32.le_u
    ಸ್ಥಿತಿ 0x4e: ಹಿಂತಿರುಗಿ &c_fuse_i32_GreaterThanOrEqual;     // i32.ge_s
    ಸ್ಥಿತಿ 0x4f: ಹಿಂತಿರುಗಿ &c_fuse_u32_GreaterThanOrEqual;     // i32.ge_u
    ಪೂರ್ವನಿಯೋಜಿತ: ಹಿಂತಿರುಗಿ ಖಾಲಿ;
    }
}

#ಆದರೆ_ಅಂತ್ಯ // d_m3FuseBranch

ಸ್ಥಿರ ಸ್ಥಿರಾಂಕ IM3Operation c_intSelectOps[2][4] = {
    { op_Select_i32_rss, op_Select_i32_srs, op_Select_i32_ssr, op_Select_i32_sss },
    { op_Select_i64_rss, op_Select_i64_srs, op_Select_i64_ssr, op_Select_i64_sss }
};
#ಆದರೆ d_m3HasFloat
ಸ್ಥಿರ ಸ್ಥಿರಾಂಕ IM3Operation c_fpSelectOps[2][2][3] = {
    { { op_Select_f32_sss, op_Select_f32_srs, op_Select_f32_ssr }, // selector in slot
      { op_Select_f32_rss, op_Select_f32_rrs, op_Select_f32_rsr } }, // selector in reg
    { { op_Select_f64_sss, op_Select_f64_srs, op_Select_f64_ssr }, // selector in slot
      { op_Select_f64_rss, op_Select_f64_rrs, op_Select_f64_rsr } }
};    // selector in reg
#ಆದರೆ_ಅಂತ್ಯ

// all args & returns are 64-bit aligned, so use 2 slots for a d_m3Use32BitSlots=1 build

ಸ್ಥಿರ
M3_ಫಲಿತಾಂಶ Acquire_Compilation_ಸಂಕೇತ_ಪುಟ (IM3Compilation o, IM3_ಸಂಕೇತ_ಪುಟ* o_ಸಂಕೇತ_ಪುಟ)
{
    M3_ಫಲಿತಾಂಶ ಫಲಿತಾಂಶ = m3Err_none;

    IM3_ಸಂಕೇತ_ಪುಟ ಪುಟ = Acquire_ಸಂಕೇತ_ಪುಟ(o->runtime);

    ಆದರೆ (ಪುಟ) {
#ಆದರೆ (d_m3_Enable_ಸಂಕೇತ_ಪುಟ_Ref_Counting)
        {
            ಆದರೆ (o->function) {
                IM3Function func = o->function;
                ಪುಟ->info.usage_ಎಣಿಕೆ++;

                u32 index = func->ಅಂಕೆ_ಸಂಕೇತ_ಪುಟ_Refs++;
_               (m3ReallocArray(&func->ಸಂಕೇತ_ಪುಟ_Refs, IM3_ಸಂಕೇತ_ಪುಟ, func->ಅಂಕೆ_ಸಂಕೇತ_ಪುಟ_Refs, index));
                func->ಸಂಕೇತ_ಪುಟ_Refs[index] = ಪುಟ;
            }
        }
#ಆದರೆ_ಅಂತ್ಯ
    } ಇಲ್ಲದಿದ್ದರೆ {
        _throw(m3_Err_malloc_Failed_ಸಂಕೇತ_ಪುಟ);
    }

    _catch:

    *o_ಸಂಕೇತ_ಪುಟ = ಪುಟ;

    ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}

ಸ್ಥಿರ ಸಾಲೊಳಗೆ
ಶೂನ್ಯ Release_Compilation_ಸಂಕೇತ_ಪುಟ (IM3Compilation o)
{
    Release_ಸಂಕೇತ_ಪುಟ(o->runtime, o->ಪುಟ);
}

ಸ್ಥಿರ ಸಾಲೊಳಗೆ
u16 Get_Type_ಅಂಕೆ_Slots (m3type_t i_type)
{
    i_type = ತಳ_Type_ದ(i_type);

    // v128 is 16 bytes - 4 slots in 32-bit-slot mode, 2 in 64-bit.
    // (Slot-allocator only; no v128 ops execute.)
    ಆದರೆ (i_type == c_m3Type_v128)
#ಆದರೆ d_m3_Use32_ಬಿಟ್_Slots
        ಹಿಂತಿರುಗಿ 4;
#ಇಲ್ಲದಿದ್ದರೆ
        ಹಿಂತಿರುಗಿ 2;
#ಆದರೆ_ಅಂತ್ಯ
#ಆದರೆ d_m3_Use32_ಬಿಟ್_Slots
    ಹಿಂತಿರುಗಿ Is64_ಬಿಟ್_Type(i_type) ? 2 : 1;
#ಇಲ್ಲದಿದ್ದರೆ
    ಹಿಂತಿರುಗಿ 1;
#ಆದರೆ_ಅಂತ್ಯ
}

ಸ್ಥಿರ ಸಾಲೊಳಗೆ
ಶೂನ್ಯ Align_ಸ್ಥಾನ_ಗಮ್ಯ_Type (u16* io_ಸ್ಥಾನ, m3type_t i_type)
{
    // align 64-bit words to even slots (if d_m3Use32BitSlots)
    u16 ಅಂಕೆ_Slots = Get_Type_ಅಂಕೆ_Slots(i_type);

    u16 ಮುಖವಾಡ = ಅಂಕೆ_Slots - 1;
    *io_ಸ್ಥಾನ = (*io_ಸ್ಥಾನ + ಮುಖವಾಡ) & ~ಮುಖವಾಡ;
}

// Deliberately unchecked. Validation rejects an empty operand stack before compilation.
ಸ್ಥಿರ ಸಾಲೊಳಗೆ
i16 Get_ರಾಶಿ_ತುದಿ_ಸೂಚ್ಯಂಕ (IM3Compilation o)
{
    ಹಿಂತಿರುಗಿ o->ರಾಶಿ_ಸೂಚ್ಯಂಕ - 1;
}

ಸ್ಥಿರ ಸಾಲೊಳಗೆ
M3_ಫಲಿತಾಂಶ Get_ರಾಶಿ_ತುದಿ_ಸೂಚ್ಯಂಕ_Throws (IM3Compilation o, i16* o_ರಾಶಿ_ಸೂಚ್ಯಂಕ)
{
    *o_ರಾಶಿ_ಸೂಚ್ಯಂಕ = o->ರಾಶಿ_ಸೂಚ್ಯಂಕ - 1;

    ಆದರೆ (M3_LIKELY(o->ರಾಶಿ_ಸೂಚ್ಯಂಕ > o->ರಾಶಿ_ಮೊದಲ_Dynamic_ಸೂಚ್ಯಂಕ or Is_ರಾಶಿ_Polymorphic(o))) {
        ಹಿಂತಿರುಗಿ m3Err_none;
    } ಇಲ್ಲದಿದ್ದರೆ {
        ಹಿಂತಿರುಗಿ m3_Err_function_ರಾಶಿ_Underrun;
    }
}


// Items in the static portion of the stack (args/locals) are hidden from GetStackTypeFromTop ()
// In other words, only "real" Wasm stack items can be inspected.  This is important when
// returning values, etc. and you need an accurate wasm-view of the stack.
ಸ್ಥಿರ
m3type_t Get_ರಾಶಿ_Type_From_ತುದಿ (IM3Compilation o, u16 i_ಸ್ಥಾನಾಂತರ)
{
    m3type_t type = c_m3Type_none;

    ++i_ಸ್ಥಾನಾಂತರ;
    ಆದರೆ (o->ರಾಶಿ_ಸೂಚ್ಯಂಕ >= i_ಸ್ಥಾನಾಂತರ) {
        u16 index = o->ರಾಶಿ_ಸೂಚ್ಯಂಕ - i_ಸ್ಥಾನಾಂತರ;

        ಆದರೆ (index >= o->ರಾಶಿ_ಮೊದಲ_Dynamic_ಸೂಚ್ಯಂಕ) {
            type = o->type_ರಾಶಿ[index];
        }
    }

    ಹಿಂತಿರುಗಿ type;
}

ಸ್ಥಿರ ಸಾಲೊಳಗೆ
m3type_t Get_ರಾಶಿ_ತುದಿ_Type (IM3Compilation o)
{
    ಹಿಂತಿರುಗಿ Get_ರಾಶಿ_Type_From_ತುದಿ(o, 0);
}

ಸ್ಥಿರ ಸಾಲೊಳಗೆ
m3type_t Get_ರಾಶಿ_Type_From_Bottom (IM3Compilation o, u16 i_ಸ್ಥಾನಾಂತರ)
{
    m3type_t type = c_m3Type_none;

    ಆದರೆ (i_ಸ್ಥಾನಾಂತರ < o->ರಾಶಿ_ಸೂಚ್ಯಂಕ) {
        type = o->type_ರಾಶಿ[i_ಸ್ಥಾನಾಂತರ];
    }

    ಹಿಂತಿರುಗಿ type;
}


ಸ್ಥಿರ ಸಾಲೊಳಗೆ
bool Is_Constant_ಸ್ಥಾನ (IM3Compilation o, u16 i_ಸ್ಥಾನ)
{
    ಹಿಂತಿರುಗಿ (i_ಸ್ಥಾನ >= o->ಸ್ಥಾನ_ಮೊದಲ_Const_ಸೂಚ್ಯಂಕ and i_ಸ್ಥಾನ < o->ಸ್ಥಾನ_Max_Const_ಸೂಚ್ಯಂಕ);
}
ಸ್ಥಿರ ಸಾಲೊಳಗೆ
bool Is_ಸ್ಥಾನ_Allocated (IM3Compilation o, u16 i_ಸ್ಥಾನ)
{
    ಹಿಂತಿರುಗಿ o->m3Slots[i_ಸ್ಥಾನ];
}

ಸ್ಥಿರ ಸಾಲೊಳಗೆ
bool Is_ರಾಶಿ_ಸೂಚ್ಯಂಕ_ಒಳ_ರಿಜಿಸ್ಟರ್ (IM3Compilation o, i32 i_ರಾಶಿ_ಸೂಚ್ಯಂಕ)
{                                                                           d_m3Assert (i_ರಾಶಿ_ಸೂಚ್ಯಂಕ < o->ರಾಶಿ_ಸೂಚ್ಯಂಕ or Is_ರಾಶಿ_Polymorphic (o));
    ಆದರೆ (i_ರಾಶಿ_ಸೂಚ್ಯಂಕ >= 0 and i_ರಾಶಿ_ಸೂಚ್ಯಂಕ < o->ರಾಶಿ_ಸೂಚ್ಯಂಕ) {
        ಹಿಂತಿರುಗಿ (o->wasm_ರಾಶಿ[i_ರಾಶಿ_ಸೂಚ್ಯಂಕ] >= d_m3_Reg0_ಸ್ಥಾನ_Alias);
    } ಇಲ್ಲದಿದ್ದರೆ {
        ಹಿಂತಿರುಗಿ false;
    }
}

ಸ್ಥಿರ ಸಾಲೊಳಗೆ
u16 Get_ಅಂಕೆ_ಖಂಡ_ಮೌಲ್ಯಗಳು_On_ರಾಶಿ (IM3Compilation o)
{
    ಹಿಂತಿರುಗಿ o->ರಾಶಿ_ಸೂಚ್ಯಂಕ - o->ಖಂಡ.ಖಂಡ_ರಾಶಿ_ಸೂಚ್ಯಂಕ;
}

ಸ್ಥಿರ ಸಾಲೊಳಗೆ
bool Is_ರಾಶಿ_ತುದಿ_ಒಳ_ರಿಜಿಸ್ಟರ್ (IM3Compilation o)
{
    ಹಿಂತಿರುಗಿ Is_ರಾಶಿ_ಸೂಚ್ಯಂಕ_ಒಳ_ರಿಜಿಸ್ಟರ್(o, (i32)Get_ರಾಶಿ_ತುದಿ_ಸೂಚ್ಯಂಕ(o));
}
ಸ್ಥಿರ ಸಾಲೊಳಗೆ
bool Is_ರಾಶಿ_ತುದಿ_Minus1_ಒಳ_ರಿಜಿಸ್ಟರ್ (IM3Compilation o)
{
    ಹಿಂತಿರುಗಿ Is_ರಾಶಿ_ಸೂಚ್ಯಂಕ_ಒಳ_ರಿಜಿಸ್ಟರ್(o, (i32)Get_ರಾಶಿ_ತುದಿ_ಸೂಚ್ಯಂಕ(o) - 1);
}
ಸ್ಥಿರ ಸಾಲೊಳಗೆ
bool Is_ರಾಶಿ_ತುದಿ_Minus2_ಒಳ_ರಿಜಿಸ್ಟರ್ (IM3Compilation o)
{
    ಹಿಂತಿರುಗಿ Is_ರಾಶಿ_ಸೂಚ್ಯಂಕ_ಒಳ_ರಿಜಿಸ್ಟರ್(o, (i32)Get_ರಾಶಿ_ತುದಿ_ಸೂಚ್ಯಂಕ(o) - 2);
}

ಸ್ಥಿರ ಸಾಲೊಳಗೆ
bool Is_ರಾಶಿ_ತುದಿ_ಒಳ_ಸ್ಥಾನ (IM3Compilation o)
{
    ಹಿಂತಿರುಗಿ not Is_ರಾಶಿ_ತುದಿ_ಒಳ_ರಿಜಿಸ್ಟರ್(o);
}

ಸ್ಥಿರ ಸಾಲೊಳಗೆ
bool Is_Valid_ಸ್ಥಾನ (u16 i_ಸ್ಥಾನ)
{
    ಹಿಂತಿರುಗಿ (i_ಸ್ಥಾನ < d_m3MaxFunctionSlots);
}

ಸ್ಥಿರ ಸಾಲೊಳಗೆ
u16 Get_ರಾಶಿ_ತುದಿ_ಸ್ಥಾನ_ಸಂಖ್ಯೆ (IM3Compilation o)
{
    i16 i = Get_ರಾಶಿ_ತುದಿ_ಸೂಚ್ಯಂಕ(o);

    u16 ಸ್ಥಾನ = c_ಸ್ಥಾನ_ಬಳಸದ;

    ಆದರೆ (i >= 0) {
        ಸ್ಥಾನ = o->wasm_ರಾಶಿ[i];
    }

    ಹಿಂತಿರುಗಿ ಸ್ಥಾನ;
}


// from bottom
ಸ್ಥಿರ ಸಾಲೊಳಗೆ
u16 Get_ಸ್ಥಾನ_For_ರಾಶಿ_ಸೂಚ್ಯಂಕ (IM3Compilation o, u16 i_ರಾಶಿ_ಸೂಚ್ಯಂಕ)
{                                                                   d_m3Assert (i_ರಾಶಿ_ಸೂಚ್ಯಂಕ < o->ರಾಶಿ_ಸೂಚ್ಯಂಕ or Is_ರಾಶಿ_Polymorphic (o));
    u16 ಸ್ಥಾನ = c_ಸ್ಥಾನ_ಬಳಸದ;

    ಆದರೆ (i_ರಾಶಿ_ಸೂಚ್ಯಂಕ < o->ರಾಶಿ_ಸೂಚ್ಯಂಕ) {
        ಸ್ಥಾನ = o->wasm_ರಾಶಿ[i_ರಾಶಿ_ಸೂಚ್ಯಂಕ];
    }

    ಹಿಂತಿರುಗಿ ಸ್ಥಾನ;
}

ಸ್ಥಿರ ಸಾಲೊಳಗೆ
u16 Get_Extra_ಸ್ಥಾನ_For_ರಾಶಿ_ಸೂಚ್ಯಂಕ (IM3Compilation o, u16 i_ರಾಶಿ_ಸೂಚ್ಯಂಕ)
{
    u16 ತಳ_ಸ್ಥಾನ = Get_ಸ್ಥಾನ_For_ರಾಶಿ_ಸೂಚ್ಯಂಕ(o, i_ರಾಶಿ_ಸೂಚ್ಯಂಕ);

    ಆದರೆ (ತಳ_ಸ್ಥಾನ != c_ಸ್ಥಾನ_ಬಳಸದ) {
        u16 extra_ಸ್ಥಾನ = Get_Type_ಅಂಕೆ_Slots(Get_ರಾಶಿ_Type_From_Bottom(o, i_ರಾಶಿ_ಸೂಚ್ಯಂಕ)) - 1;
        ತಳ_ಸ್ಥಾನ += extra_ಸ್ಥಾನ;
    }

    ಹಿಂತಿರುಗಿ ತಳ_ಸ್ಥಾನ;
}


ಸ್ಥಿರ ಸಾಲೊಳಗೆ
ಶೂನ್ಯ Touch_ಸ್ಥಾನ (IM3Compilation o, u16 i_ಸ್ಥಾನ)
{
    // op_Entry uses this value to track and detect stack overflow
    o->max_ರಾಶಿ_Slots = M3_MAX(o->max_ರಾಶಿ_Slots, i_ಸ್ಥಾನ + 1);
}

ಸ್ಥಿರ ಸಾಲೊಳಗೆ
ಶೂನ್ಯ Mark_ಸ್ಥಾನ_Allocated (IM3Compilation o, u16 i_ಸ್ಥಾನ)
{                                                                   d_m3Assert (o->m3Slots [i_ಸ್ಥಾನ] == 0); // shouldn't be already allocated
    o->m3Slots[i_ಸ್ಥಾನ] = 1;

    o->ಸ್ಥಾನ_Max_Allocated_ಸೂಚ್ಯಂಕ_Plus_One = M3_MAX(o->ಸ್ಥಾನ_Max_Allocated_ಸೂಚ್ಯಂಕ_Plus_One, i_ಸ್ಥಾನ + 1);

    Touch_ಸ್ಥಾನ(o, i_ಸ್ಥಾನ);
}

ಸ್ಥಿರ ಸಾಲೊಳಗೆ
M3_ಫಲಿತಾಂಶ MarkSlotsAllocated (IM3Compilation o, u16 i_ಸ್ಥಾನ, u16 i_ಅಂಕೆ_Slots)
{
    ಆದರೆ (M3_UNLIKELY(i_ಸ್ಥಾನ + i_ಅಂಕೆ_Slots > d_m3MaxFunctionSlots)) {
        ಹಿಂತಿರುಗಿ m3_Err_function_ರಾಶಿ_ಅತಿಪ್ರವಾಹ;
    }

    ಆಗ (i_ಅಂಕೆ_Slots--) {
        Mark_ಸ್ಥಾನ_Allocated(o, i_ಸ್ಥಾನ++);
    }

    ಹಿಂತಿರುಗಿ m3Err_none;
}

ಸ್ಥಿರ ಸಾಲೊಳಗೆ
M3_ಫಲಿತಾಂಶ MarkSlotsAllocatedByType (IM3Compilation o, u16 i_ಸ್ಥಾನ, m3type_t i_type)
{
    u16 ಅಂಕೆ_Slots = Get_Type_ಅಂಕೆ_Slots(i_type);
    ಹಿಂತಿರುಗಿ MarkSlotsAllocated(o, i_ಸ್ಥಾನ, ಅಂಕೆ_Slots);
}


ಸ್ಥಿರ
M3_ಫಲಿತಾಂಶ AllocateSlotsWithinRange (IM3Compilation o, u16* o_ಸ್ಥಾನ, m3type_t i_type, u16 i_ಆರಂಭ_ಸ್ಥಾನ, u16 i_ಕೊನೆ_ಸ್ಥಾನ)
{
    M3_ಫಲಿತಾಂಶ ಫಲಿತಾಂಶ = m3_Err_function_ರಾಶಿ_ಅತಿಪ್ರವಾಹ;

    u16 ಅಂಕೆ_Slots     = Get_Type_ಅಂಕೆ_Slots(i_type);
    u16 ಹುಡುಕು_ಸ್ಥಾನಾಂತರ = ಅಂಕೆ_Slots - 1;

    Align_ಸ್ಥಾನ_ಗಮ್ಯ_Type(&i_ಆರಂಭ_ಸ್ಥಾನ, i_type);

    // search for 1 or 2 consecutive slots in the execution stack
    u16 i = i_ಆರಂಭ_ಸ್ಥಾನ;
    ಆಗ (i + ಹುಡುಕು_ಸ್ಥಾನಾಂತರ < i_ಕೊನೆ_ಸ್ಥಾನ) {
        ಆದರೆ (i + ಹುಡುಕು_ಸ್ಥಾನಾಂತರ < d_m3MaxFunctionSlots and o->m3Slots[i] == 0 and o->m3Slots[i + ಹುಡುಕು_ಸ್ಥಾನಾಂತರ] == 0) {
            MarkSlotsAllocated(o, i, ಅಂಕೆ_Slots);

            *o_ಸ್ಥಾನ = i;
            ಫಲಿತಾಂಶ  = m3Err_none;
            ನಿಲ್ಲಿಸು;
        }

        // keep 2-slot allocations even-aligned
        i += ಅಂಕೆ_Slots;
    }

    ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}

ಸ್ಥಿರ ಸಾಲೊಳಗೆ
M3_ಫಲಿತಾಂಶ AllocateSlots (IM3Compilation o, u16* o_ಸ್ಥಾನ, m3type_t i_type)
{
    ಹಿಂತಿರುಗಿ AllocateSlotsWithinRange(o, o_ಸ್ಥಾನ, i_type, o->ಸ್ಥಾನ_ಮೊದಲ_Dynamic_ಸೂಚ್ಯಂಕ, d_m3MaxFunctionSlots);
}

ಸ್ಥಿರ ಸಾಲೊಳಗೆ
M3_ಫಲಿತಾಂಶ AllocateConstantSlots (IM3Compilation o, u16* o_ಸ್ಥಾನ, m3type_t i_type)
{
    u16 max_ಕೋಷ್ಟಕ_ಸೂಚ್ಯಂಕ = o->ಸ್ಥಾನ_ಮೊದಲ_Const_ಸೂಚ್ಯಂಕ + d_m3_Max_Constant_ಕೋಷ್ಟಕ_ಗಾತ್ರ;
    ಹಿಂತಿರುಗಿ AllocateSlotsWithinRange(o, o_ಸ್ಥಾನ, i_type, o->ಸ್ಥಾನ_ಮೊದಲ_Const_ಸೂಚ್ಯಂಕ, M3_MIN(o->ಸ್ಥಾನ_ಮೊದಲ_Dynamic_ಸೂಚ್ಯಂಕ, max_ಕೋಷ್ಟಕ_ಸೂಚ್ಯಂಕ));
}


// TOQUE: this usage count system could be eliminated. real world code doesn't frequently trigger it.  just copy to multiple
// unique slots.
ಸ್ಥಿರ ಸಾಲೊಳಗೆ
M3_ಫಲಿತಾಂಶ Increment_ಸ್ಥಾನ_Usage_ಎಣಿಕೆ (IM3Compilation o, u16 i_ಸ್ಥಾನ)
{                                                                                       d_m3Assert (i_ಸ್ಥಾನ < d_m3MaxFunctionSlots);
    M3_ಫಲಿತಾಂಶ ಫಲಿತಾಂಶ = m3Err_none;                                                       d_m3Assert (o->m3Slots [i_ಸ್ಥಾನ] > 0);

    // OPTZ (memory): 'm3Slots' could still be fused with 'typeStack' if 4 bits were used to indicate: [0,1,2,many]. The many-case
    // would scan 'wasmStack' to determine the actual usage count
    ಆದರೆ (M3_LIKELY(o->m3Slots[i_ಸ್ಥಾನ] < 0xFF)) {
        o->m3Slots[i_ಸ್ಥಾನ]++;
    } ಇಲ್ಲದಿದ್ದರೆ {
        ಫಲಿತಾಂಶ = "slot usage count overflow";
    }

    ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}

ಸ್ಥಿರ ಸಾಲೊಳಗೆ
ಶೂನ್ಯ Deallocate_ಸ್ಥಾನ (IM3Compilation o, i16 i_ಸ್ಥಾನ, m3type_t i_type)
{                                                                                       d_m3Assert (i_ಸ್ಥಾನ >= o->ಸ್ಥಾನ_ಮೊದಲ_Dynamic_ಸೂಚ್ಯಂಕ);
                                                                                        d_m3Assert (i_ಸ್ಥಾನ < o->ಸ್ಥಾನ_Max_Allocated_ಸೂಚ್ಯಂಕ_Plus_One);
    ಗಾಗಿ (u16 i = 0; i < Get_Type_ಅಂಕೆ_Slots(i_type); ++i, ++i_ಸ್ಥಾನ) {                       d_m3Assert (o->m3Slots [i_ಸ್ಥಾನ]);
        --o->m3Slots[i_ಸ್ಥಾನ];
    }
}


ಸ್ಥಿರ ಸಾಲೊಳಗೆ
bool Is_ರಿಜಿಸ್ಟರ್_Type_Allocated (IM3Compilation o, u8 i_type)
{
    ಹಿಂತಿರುಗಿ Is_ರಿಜಿಸ್ಟರ್_Allocated(o, IsFpType(i_type));
}

ಸ್ಥಿರ ಸಾಲೊಳಗೆ
ಶೂನ್ಯ Allocate_ರಿಜಿಸ್ಟರ್ (IM3Compilation o, u32 i_ರಿಜಿಸ್ಟರ್, u16 i_ರಾಶಿ_ಸೂಚ್ಯಂಕ)
{                                                                                       d_m3Assert (not Is_ರಿಜಿಸ್ಟರ್_Allocated (o, i_ರಿಜಿಸ್ಟರ್));
    o->reg_ರಾಶಿ_ಸೂಚ್ಯಂಕ_Plus_One[i_ರಿಜಿಸ್ಟರ್] = i_ರಾಶಿ_ಸೂಚ್ಯಂಕ + 1;
}

ಸ್ಥಿರ ಸಾಲೊಳಗೆ
ಶೂನ್ಯ Deallocate_ರಿಜಿಸ್ಟರ್ (IM3Compilation o, u32 i_ರಿಜಿಸ್ಟರ್)
{                                                                                       d_m3Assert (Is_ರಿಜಿಸ್ಟರ್_Allocated (o, i_ರಿಜಿಸ್ಟರ್));
    o->reg_ರಾಶಿ_ಸೂಚ್ಯಂಕ_Plus_One[i_ರಿಜಿಸ್ಟರ್] = c_m3_ರಿಜಿಸ್ಟರ್_Unallocated;
}

ಸ್ಥಿರ ಸಾಲೊಳಗೆ
u16 Get_ರಿಜಿಸ್ಟರ್_ರಾಶಿ_ಸೂಚ್ಯಂಕ (IM3Compilation o, u32 i_ರಿಜಿಸ್ಟರ್)
{                                                                                       d_m3Assert (Is_ರಿಜಿಸ್ಟರ್_Allocated (o, i_ರಿಜಿಸ್ಟರ್));
    ಹಿಂತಿರುಗಿ o->reg_ರಾಶಿ_ಸೂಚ್ಯಂಕ_Plus_One[i_ರಿಜಿಸ್ಟರ್] - 1;
}

u16 Get_Max_ಬಳಸಿದ_ಸ್ಥಾನ_Plus_One (IM3Compilation o)
{
    ಆಗ (o->ಸ್ಥಾನ_Max_Allocated_ಸೂಚ್ಯಂಕ_Plus_One > o->ಸ್ಥಾನ_ಮೊದಲ_Dynamic_ಸೂಚ್ಯಂಕ) {
        ಆದರೆ (Is_ಸ್ಥಾನ_Allocated(o, o->ಸ್ಥಾನ_Max_Allocated_ಸೂಚ್ಯಂಕ_Plus_One - 1)) {
            ನಿಲ್ಲಿಸು;
        }

        o->ಸ್ಥಾನ_Max_Allocated_ಸೂಚ್ಯಂಕ_Plus_One--;
    }

#ವ್ಯಾಖ್ಯಾನಿತವಾದರೆ DEBUG
    u16 max_ಸ್ಥಾನ = o->ಸ್ಥಾನ_Max_Allocated_ಸೂಚ್ಯಂಕ_Plus_One;
    ಆಗ (max_ಸ್ಥಾನ < d_m3MaxFunctionSlots) {
        d_m3Assert(o->m3Slots[max_ಸ್ಥಾನ] == 0);
        max_ಸ್ಥಾನ++;
    }
#ಆದರೆ_ಅಂತ್ಯ

    ಹಿಂತಿರುಗಿ o->ಸ್ಥಾನ_Max_Allocated_ಸೂಚ್ಯಂಕ_Plus_One;
}

ಸ್ಥಿರ
M3_ಫಲಿತಾಂಶ Preserve_ರಿಜಿಸ್ಟರ್_If_Occupied (IM3Compilation o, m3type_t i_ರಿಜಿಸ್ಟರ್_Type)
{
    M3_ಫಲಿತಾಂಶ ಫಲಿತಾಂಶ = m3Err_none;

    u32 regSelect = IsFpType(i_ರಿಜಿಸ್ಟರ್_Type);

    ಆದರೆ (Is_ರಿಜಿಸ್ಟರ್_Allocated(o, regSelect)) {
        u16 ರಾಶಿ_ಸೂಚ್ಯಂಕ = Get_ರಿಜಿಸ್ಟರ್_ರಾಶಿ_ಸೂಚ್ಯಂಕ(o, regSelect);
        Deallocate_ರಿಜಿಸ್ಟರ್(o, regSelect);

        m3type_t type = Get_ರಾಶಿ_Type_From_Bottom(o, ರಾಶಿ_ಸೂಚ್ಯಂಕ);

        // and point to a exec slot
        u16 ಸ್ಥಾನ = c_ಸ್ಥಾನ_ಬಳಸದ;
_       (AllocateSlots(o, &ಸ್ಥಾನ, type));
        o->wasm_ರಾಶಿ[ರಾಶಿ_ಸೂಚ್ಯಂಕ] = ಸ್ಥಾನ;

        // Ensure type is within the valid range
        ಆದರೆ (M3_LIKELY(ತಳ_Type_ದ(type) < c_m3_Type_ಎಣಿಕೆ)) {
_           (EmitOp(o, c_ಗಣ_ಗಣ_Ops[ತಳ_Type_ದ(type)]));
        } ಇಲ್ಲದಿದ್ದರೆ {
            _throw(m3Err_unknownType);
        }

        Emit_ಸ್ಥಾನ_ಸ್ಥಾನಾಂತರ(o, ಸ್ಥಾನ);
    }

    _catch: ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}


// all values must be in slots before entering loop, if, and else blocks
// otherwise they'd end up preserve-copied in the block to probably different locations (if/else)
ಸ್ಥಿರ ಸಾಲೊಳಗೆ
M3_ಫಲಿತಾಂಶ PreserveRegisters (IM3Compilation o)
{
    M3_ಫಲಿತಾಂಶ ಫಲಿತಾಂಶ;

_   (Preserve_ರಿಜಿಸ್ಟರ್_If_Occupied(o, c_m3Type_f64));
_   (Preserve_ರಿಜಿಸ್ಟರ್_If_Occupied(o, c_m3Type_i64));

    _catch: ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}

ಸ್ಥಿರ
M3_ಫಲಿತಾಂಶ Preserve_Non_ತುದಿ_Registers (IM3Compilation o)
{
    M3_ಫಲಿತಾಂಶ ಫಲಿತಾಂಶ = m3Err_none;

    i16 ರಾಶಿ_ತುದಿ = Get_ರಾಶಿ_ತುದಿ_ಸೂಚ್ಯಂಕ(o);

    ಆದರೆ (ರಾಶಿ_ತುದಿ >= 0) {
        ಆದರೆ (Is_ರಿಜಿಸ್ಟರ್_Allocated(o, 0))     // r0
        {
            ಆದರೆ (Get_ರಿಜಿಸ್ಟರ್_ರಾಶಿ_ಸೂಚ್ಯಂಕ(o, 0) != ರಾಶಿ_ತುದಿ) {
_               (Preserve_ರಿಜಿಸ್ಟರ್_If_Occupied(o, c_m3Type_i64));
            }
        }

        ಆದರೆ (Is_ರಿಜಿಸ್ಟರ್_Allocated(o, 1))     // fp0
        {
            ಆದರೆ (Get_ರಿಜಿಸ್ಟರ್_ರಾಶಿ_ಸೂಚ್ಯಂಕ(o, 1) != ರಾಶಿ_ತುದಿ) {
_               (Preserve_ರಿಜಿಸ್ಟರ್_If_Occupied(o, c_m3Type_f64));
            }
        }
    }

    _catch: ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}


//----------------------------------------------------------------------------------------------------------------------

ಸ್ಥಿರ
M3_ಫಲಿತಾಂಶ ತಳ್ಳು (IM3Compilation o, m3type_t i_type, u16 i_ಸ್ಥಾನ)
{
    M3_ಫಲಿತಾಂಶ ಫಲಿತಾಂಶ = m3Err_none;

#ಆದರೆ !d_m3HasFloat
    ಆದರೆ (i_type == c_m3Type_f32 || i_type == c_m3Type_f64) {
        ಹಿಂತಿರುಗಿ m3Err_unknownOpcode;
    }
#ಆದರೆ_ಅಂತ್ಯ

    ಆದರೆ (M3_UNLIKELY(o->ರಾಶಿ_ಸೂಚ್ಯಂಕ >= d_m3_Max_Function_ರಾಶಿ_ಎತ್ತರ)) {
        ಹಿಂತಿರುಗಿ m3_Err_function_ರಾಶಿ_ಅತಿಪ್ರವಾಹ;
    }

    u16 ರಾಶಿ_ಸೂಚ್ಯಂಕ = o->ರಾಶಿ_ಸೂಚ್ಯಂಕ++;                                       // printf ("push: %d\n", (i32) i);

    o->wasm_ರಾಶಿ[ರಾಶಿ_ಸೂಚ್ಯಂಕ] = i_ಸ್ಥಾನ;
    o->type_ರಾಶಿ[ರಾಶಿ_ಸೂಚ್ಯಂಕ] = i_type;

    ಆದರೆ (Is_ರಿಜಿಸ್ಟರ್_ಸ್ಥಾನ_Alias(i_ಸ್ಥಾನ)) {
        u32 regSelect = Is_Fp_ರಿಜಿಸ್ಟರ್_ಸ್ಥಾನ_Alias(i_ಸ್ಥಾನ);
        Allocate_ರಿಜಿಸ್ಟರ್(o, regSelect, ರಾಶಿ_ಸೂಚ್ಯಂಕ);
    }

    ಆದರೆ (d_m3_Log_Wasm_ರಾಶಿ) {
        dump_type_ರಾಶಿ(o);
    }

    ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}

ಸ್ಥಿರ ಸಾಲೊಳಗೆ
M3_ಫಲಿತಾಂಶ ತಳ್ಳು_ರಿಜಿಸ್ಟರ್ (IM3Compilation o, m3type_t i_type)
{
    M3_ಫಲಿತಾಂಶ ಫಲಿತಾಂಶ = m3Err_none;                                                       d_m3Assert ((u16) d_m3_Reg0_ಸ್ಥಾನ_Alias > (u16) d_m3MaxFunctionSlots);
    u16      ಸ್ಥಾನ   = IsFpType(i_type) ? d_m3_Fp0_ಸ್ಥಾನ_Alias : d_m3_Reg0_ಸ್ಥಾನ_Alias;          d_m3Assert (i_type or Is_ರಾಶಿ_Polymorphic (o));

_   (ತಳ್ಳು(o, i_type, ಸ್ಥಾನ));

    _catch: ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}

ಸ್ಥಿರ
M3_ಫಲಿತಾಂಶ ತೆಗೆ (IM3Compilation o)
{
    M3_ಫಲಿತಾಂಶ ಫಲಿತಾಂಶ = m3Err_none;

    ಆದರೆ (M3_LIKELY(o->ರಾಶಿ_ಸೂಚ್ಯಂಕ > o->ಖಂಡ.ಖಂಡ_ರಾಶಿ_ಸೂಚ್ಯಂಕ)) {
        o->ರಾಶಿ_ಸೂಚ್ಯಂಕ--;                                                //  printf ("pop: %d\n", (i32) o->stackIndex);

        u16      ಸ್ಥಾನ = o->wasm_ರಾಶಿ[o->ರಾಶಿ_ಸೂಚ್ಯಂಕ];
        m3type_t type = o->type_ರಾಶಿ[o->ರಾಶಿ_ಸೂಚ್ಯಂಕ];

        ಆದರೆ (Is_ರಿಜಿಸ್ಟರ್_ಸ್ಥಾನ_Alias(ಸ್ಥಾನ)) {
            u32 regSelect = Is_Fp_ರಿಜಿಸ್ಟರ್_ಸ್ಥಾನ_Alias(ಸ್ಥಾನ);
            Deallocate_ರಿಜಿಸ್ಟರ್(o, regSelect);
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (M3_UNLIKELY(ಸ್ಥಾನ >= o->ಸ್ಥಾನ_Max_Allocated_ಸೂಚ್ಯಂಕ_Plus_One)) {
            ಹಿಂತಿರುಗಿ m3_Err_function_ರಾಶಿ_Underrun; // Return error for invalid slot indices
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (ಸ್ಥಾನ >= o->ಸ್ಥಾನ_ಮೊದಲ_Dynamic_ಸೂಚ್ಯಂಕ) {
            Deallocate_ಸ್ಥಾನ(o, ಸ್ಥಾನ, type);
        }
    } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (not Is_ರಾಶಿ_Polymorphic(o)) {
        ಫಲಿತಾಂಶ = m3_Err_function_ರಾಶಿ_Underrun;
    }

    ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}

ಸ್ಥಿರ
M3_ಫಲಿತಾಂಶ ತೆಗೆ_Type (IM3Compilation o, m3type_t i_type)
{
    M3_ಫಲಿತಾಂಶ ಫಲಿತಾಂಶ = m3Err_none;

    m3type_t ತುದಿ_Type = Get_ರಾಶಿ_ತುದಿ_Type(o);

    ಆದರೆ (M3_LIKELY(Is_Sub_Type_ದ(ತುದಿ_Type, i_type) or o->ಖಂಡ.isPolymorphic)) {
_       (ತೆಗೆ(o));
    } ಇಲ್ಲದಿದ್ದರೆ {
        _throw(m3Err_typeMismatch);
    }

    _catch:
    ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}

ಸ್ಥಿರ
M3_ಫಲಿತಾಂಶ ತಳ್ಳು_Allocated_ಸ್ಥಾನ_And_Emit (IM3Compilation o, m3type_t i_type, bool i_doEmit)
{
    M3_ಫಲಿತಾಂಶ ಫಲಿತಾಂಶ = m3Err_none;

    u16 ಸ್ಥಾನ = c_ಸ್ಥಾನ_ಬಳಸದ;

_   (AllocateSlots(o, &ಸ್ಥಾನ, i_type));
_   (ತಳ್ಳು(o, i_type, ಸ್ಥಾನ));

    ಆದರೆ (i_doEmit) {
        Emit_ಸ್ಥಾನ_ಸ್ಥಾನಾಂತರ(o, ಸ್ಥಾನ);
    }

    //    printf ("push: %d\n", (u32) slot);

    _catch: ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}

ಸ್ಥಿರ ಸಾಲೊಳಗೆ
M3_ಫಲಿತಾಂಶ ತಳ್ಳು_Allocated_ಸ್ಥಾನ_And_Emit_2 (IM3Compilation o, m3type_t i_type)
{
    ಹಿಂತಿರುಗಿ ತಳ್ಳು_Allocated_ಸ್ಥಾನ_And_Emit(o, i_type, true);
}

ಸ್ಥಿರ ಸಾಲೊಳಗೆ
M3_ಫಲಿತಾಂಶ ತಳ್ಳು_Allocated_ಸ್ಥಾನ (IM3Compilation o, m3type_t i_type)
{
    ಹಿಂತಿರುಗಿ ತಳ್ಳು_Allocated_ಸ್ಥಾನ_And_Emit(o, i_type, false);
}

ಸ್ಥಿರ
M3_ಫಲಿತಾಂಶ ತಳ್ಳು_Const (IM3Compilation o, u64 i_ಶಬ್ದ, m3type_t i_type)
{
    M3_ಫಲಿತಾಂಶ ಫಲಿತಾಂಶ = m3Err_none;

    // When compile-walking a constant expression without emitting there is no
    // constant table to place the value in, but the type still has to land on
    // the stack: extended-const arithmetic downstream consumes it.
    ಆದರೆ (M3_UNLIKELY(!o->ಪುಟ)) {
        ಹಿಂತಿರುಗಿ ತಳ್ಳು_Allocated_ಸ್ಥಾನ(o, i_type);
    }

    bool ಹೊಂದಿಕೆ_ಸಿಕ್ಕಿತು  = false;
    bool is64_ಬಿಟ್_Type = Is64_ಬಿಟ್_Type(i_type);

    u16 ಅಂಕೆ_Required_Slots  = Get_Type_ಅಂಕೆ_Slots(i_type);
    u16 ಅಂಕೆ_ಬಳಸಿದ_Const_Slots = o->ಸ್ಥಾನ_Max_Const_ಸೂಚ್ಯಂಕ - o->ಸ್ಥಾನ_ಮೊದಲ_Const_ಸೂಚ್ಯಂಕ;

    // search for duplicate matching constant slot to reuse
    ಆದರೆ (ಅಂಕೆ_Required_Slots == 2 and ಅಂಕೆ_ಬಳಸಿದ_Const_Slots >= 2) {
        u16 ಮೊದಲ_Const_ಸ್ಥಾನ = o->ಸ್ಥಾನ_ಮೊದಲ_Const_ಸೂಚ್ಯಂಕ;
        Align_ಸ್ಥಾನ_ಗಮ್ಯ_Type(&ಮೊದಲ_Const_ಸ್ಥಾನ, c_m3Type_i64);

        ಗಾಗಿ (u16 ಸ್ಥಾನ = ಮೊದಲ_Const_ಸ್ಥಾನ; ಸ್ಥಾನ < o->ಸ್ಥಾನ_Max_Const_ಸೂಚ್ಯಂಕ - 1; ಸ್ಥಾನ += 2) {
            ಆದರೆ (Is_ಸ್ಥಾನ_Allocated(o, ಸ್ಥಾನ) and Is_ಸ್ಥಾನ_Allocated(o, ಸ್ಥಾನ + 1)) {
                u64 constant;
                ಸ್ಮರಣೆ_ನಕಲು(&constant, &o->constants[ಸ್ಥಾನ - o->ಸ್ಥಾನ_ಮೊದಲ_Const_ಸೂಚ್ಯಂಕ], ಗಾತ್ರ(constant));

                ಆದರೆ (constant == i_ಶಬ್ದ) {
                    ಹೊಂದಿಕೆ_ಸಿಕ್ಕಿತು = true;
_                   (ತಳ್ಳು(o, i_type, ಸ್ಥಾನ));
                    ನಿಲ್ಲಿಸು;
                }
            }
        }
    } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (ಅಂಕೆ_Required_Slots == 1) {
        ಗಾಗಿ (u16 i = 0; i < ಅಂಕೆ_ಬಳಸಿದ_Const_Slots; ++i) {
            u16 ಸ್ಥಾನ = o->ಸ್ಥಾನ_ಮೊದಲ_Const_ಸೂಚ್ಯಂಕ + i;

            ಆದರೆ (Is_ಸ್ಥಾನ_Allocated(o, ಸ್ಥಾನ)) {
                bool matches;
                ಆದರೆ (is64_ಬಿಟ್_Type) {
                    u64 constant;
                    ಸ್ಮರಣೆ_ನಕಲು(&constant, &o->constants[i], ಗಾತ್ರ(constant));
                    matches = (constant == i_ಶಬ್ದ);
                } ಇಲ್ಲದಿದ್ದರೆ {
                    u32 constant;
                    ಸ್ಮರಣೆ_ನಕಲು(&constant, &o->constants[i], ಗಾತ್ರ(constant));
                    matches = (constant == i_ಶಬ್ದ);
                }
                ಆದರೆ (matches) {
                    ಹೊಂದಿಕೆ_ಸಿಕ್ಕಿತು = true;
_                   (ತಳ್ಳು(o, i_type, ಸ್ಥಾನ));
                    ನಿಲ್ಲಿಸು;
                }
            }
        }
    }

    ಆದರೆ (not ಹೊಂದಿಕೆ_ಸಿಕ್ಕಿತು) {
        u16 ಸ್ಥಾನ = c_ಸ್ಥಾನ_ಬಳಸದ;
        ಫಲಿತಾಂಶ   = AllocateConstantSlots(o, &ಸ್ಥಾನ, i_type);

        ಆದರೆ (ಫಲಿತಾಂಶ || ಸ್ಥಾನ == c_ಸ್ಥಾನ_ಬಳಸದ) // no more constant table space; use inline constants
        {
            ಫಲಿತಾಂಶ = m3Err_none;

            ಆದರೆ (is64_ಬಿಟ್_Type) {
_               (EmitOp(o, op_Const64));
                EmitWord64(o->ಪುಟ, i_ಶಬ್ದ);
            } ಇಲ್ಲದಿದ್ದರೆ {
_               (EmitOp(o, op_Const32));
                EmitWord32(o->ಪುಟ, (u32)i_ಶಬ್ದ);
            }

_           (ತಳ್ಳು_Allocated_ಸ್ಥಾನ_And_Emit_2(o, i_type));
        } ಇಲ್ಲದಿದ್ದರೆ {
            u16 const_ಕೋಷ್ಟಕ_ಸೂಚ್ಯಂಕ = ಸ್ಥಾನ - o->ಸ್ಥಾನ_ಮೊದಲ_Const_ಸೂಚ್ಯಂಕ;

            d_m3Assert(const_ಕೋಷ್ಟಕ_ಸೂಚ್ಯಂಕ < d_m3_Max_Constant_ಕೋಷ್ಟಕ_ಗಾತ್ರ);

            ಆದರೆ (is64_ಬಿಟ್_Type) {
                ಸ್ಮರಣೆ_ನಕಲು(&o->constants[const_ಕೋಷ್ಟಕ_ಸೂಚ್ಯಂಕ], &i_ಶಬ್ದ, ಗಾತ್ರ(i_ಶಬ್ದ));
            } ಇಲ್ಲದಿದ್ದರೆ {
                u32 word32 = (u32)i_ಶಬ್ದ;
                ಸ್ಮರಣೆ_ನಕಲು(&o->constants[const_ಕೋಷ್ಟಕ_ಸೂಚ್ಯಂಕ], &word32, ಗಾತ್ರ(word32));
            }

_           (ತಳ್ಳು(o, i_type, ಸ್ಥಾನ));

            o->ಸ್ಥಾನ_Max_Const_ಸೂಚ್ಯಂಕ = M3_MAX(ಸ್ಥಾನ + ಅಂಕೆ_Required_Slots, o->ಸ್ಥಾನ_Max_Const_ಸೂಚ್ಯಂಕ);
        }
    }

    _catch: ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}

ಸ್ಥಿರ ಸಾಲೊಳಗೆ
M3_ಫಲಿತಾಂಶ Emit_ಸ್ಥಾನ_ಅಂಕೆ_ದ_ರಾಶಿ_ತುದಿ_And_ತೆಗೆ (IM3Compilation o)
{
    // no emit if value is in register
    ಆದರೆ (Is_ರಾಶಿ_ತುದಿ_ಒಳ_ಸ್ಥಾನ(o)) {
        Emit_ಸ್ಥಾನ_ಸ್ಥಾನಾಂತರ(o, Get_ರಾಶಿ_ತುದಿ_ಸ್ಥಾನ_ಸಂಖ್ಯೆ(o));
    }

    ಹಿಂತಿರುಗಿ ತೆಗೆ(o);
}


// Or, maybe: EmitTrappingOp
M3_ಫಲಿತಾಂಶ ಕೂಡಿಸು_Trap_ದಾಖಲೆ (IM3Compilation o)
{
    M3_ಫಲಿತಾಂಶ ಫಲಿತಾಂಶ = m3Err_none;

    ಆದರೆ (o->function) {
    }

    ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}

ಸ್ಥಿರ
M3_ಫಲಿತಾಂಶ Unwind_ಖಂಡ_ರಾಶಿ (IM3Compilation o)
{
    M3_ಫಲಿತಾಂಶ ಫಲಿತಾಂಶ = m3Err_none;

    u32 ತೆಗೆ_ಎಣಿಕೆ = 0;
    ಆಗ (o->ರಾಶಿ_ಸೂಚ್ಯಂಕ > o->ಖಂಡ.ಖಂಡ_ರಾಶಿ_ಸೂಚ್ಯಂಕ) {
_       (ತೆಗೆ(o));
        ++ತೆಗೆ_ಎಣಿಕೆ;
    }

    ಆದರೆ (ತೆಗೆ_ಎಣಿಕೆ) {
        m3log(compile, "unwound stack top: %d", ತೆಗೆ_ಎಣಿಕೆ);
    }

    _catch: ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}

ಸ್ಥಿರ ಸಾಲೊಳಗೆ
M3_ಫಲಿತಾಂಶ ಗಣ_ರಾಶಿ_Polymorphic (IM3Compilation o)
{
    o->ಖಂಡ.isPolymorphic = true;                              m3log (compile, "stack set polymorphic");
    ಹಿಂತಿರುಗಿ Unwind_ಖಂಡ_ರಾಶಿ(o);
}

ಸ್ಥಿರ
ಶೂನ್ಯ PatchBranches (IM3Compilation o)
{
    InvalidateFold(o);         // patched branches land at the current pc

    pc_t pc = GetPC(o);

    pc_t patches     = o->ಖಂಡ.patches;
    o->ಖಂಡ.patches = ಖಾಲಿ;

    ಆಗ (patches) {                                           m3log (compile, "patching location: %p to pc: %p", patches, pc);
        pc_t ಮುಂದಿನ       = *(pc_t*)patches;
        *(pc_t*)patches = pc;
        patches         = ಮುಂದಿನ;
    }
}

//-------------------------------------------------------------------------------------------------------------------------

ಸ್ಥಿರ
M3_ಫಲಿತಾಂಶ Copy_ರಾಶಿ_ಸೂಚ್ಯಂಕ_ಗಮ್ಯ_ಸ್ಥಾನ (IM3Compilation o, u16 i_dest_ಸ್ಥಾನ, u16 i_ರಾಶಿ_ಸೂಚ್ಯಂಕ)  // NoPushPop
{
    M3_ಫಲಿತಾಂಶ ಫಲಿತಾಂಶ = m3Err_none;

    IM3Operation op;

    m3type_t type       = Get_ರಾಶಿ_Type_From_Bottom(o, i_ರಾಶಿ_ಸೂಚ್ಯಂಕ);
    bool     ಒಳ_ರಿಜಿಸ್ಟರ್ = Is_ರಾಶಿ_ಸೂಚ್ಯಂಕ_ಒಳ_ರಿಜಿಸ್ಟರ್(o, i_ರಾಶಿ_ಸೂಚ್ಯಂಕ);

    ಆದರೆ (ಒಳ_ರಿಜಿಸ್ಟರ್) {
        op = c_ಗಣ_ಗಣ_Ops[ತಳ_Type_ದ(type)];
    } ಇಲ್ಲದಿದ್ದರೆ {
        op = Is64_ಬಿಟ್_Type(type) ? op_Copy_ಸ್ಥಾನ_64 : op_Copy_ಸ್ಥಾನ_32;
    }

_   (EmitOp(o, op));
    Emit_ಸ್ಥಾನ_ಸ್ಥಾನಾಂತರ(o, i_dest_ಸ್ಥಾನ);

    ಆದರೆ (not ಒಳ_ರಿಜಿಸ್ಟರ್) {
        u16 src_ಸ್ಥಾನ = Get_ಸ್ಥಾನ_For_ರಾಶಿ_ಸೂಚ್ಯಂಕ(o, i_ರಾಶಿ_ಸೂಚ್ಯಂಕ);
        Emit_ಸ್ಥಾನ_ಸ್ಥಾನಾಂತರ(o, src_ಸ್ಥಾನ);
    }

    _catch: ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}

ಸ್ಥಿರ
M3_ಫಲಿತಾಂಶ Copy_ರಾಶಿ_ತುದಿ_ಗಮ್ಯ_ಸ್ಥಾನ (IM3Compilation o, u16 i_dest_ಸ್ಥಾನ)  // NoPushPop
{
    M3_ಫಲಿತಾಂಶ ಫಲಿತಾಂಶ;

    i16 ರಾಶಿ_ತುದಿ;
_   (Get_ರಾಶಿ_ತುದಿ_ಸೂಚ್ಯಂಕ_Throws(o, &ರಾಶಿ_ತುದಿ));
_   (Copy_ರಾಶಿ_ಸೂಚ್ಯಂಕ_ಗಮ್ಯ_ಸ್ಥಾನ(o, i_dest_ಸ್ಥಾನ, (u16)ರಾಶಿ_ತುದಿ));

    _catch: ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}


// a copy-on-write strategy is used with locals. when a get local occurs, it's not copied anywhere. the stack
// entry just has a index pointer to that local memory slot.
// then, when a previously referenced local is set, the current value needs to be preserved for those references

// TODO: consider getting rid of these specialized operations: PreserveSetSlot & PreserveCopySlot.
// They likely just take up space (which seems to reduce performance) without improving performance.
// => unconfirmed in preliminary testing.
ಸ್ಥಿರ
M3_ಫಲಿತಾಂಶ Preserved_Copy_ತುದಿ_ಸ್ಥಾನ (IM3Compilation o, u16 i_dest_ಸ್ಥಾನ, u16 i_preserve_ಸ್ಥಾನ)
{
    M3_ಫಲಿತಾಂಶ ಫಲಿತಾಂಶ = m3Err_none;             d_m3Assert (i_dest_ಸ್ಥಾನ != i_preserve_ಸ್ಥಾನ);

    IM3Operation op;

    m3type_t type = Get_ರಾಶಿ_ತುದಿ_Type(o);

    ಆದರೆ (Is_ರಾಶಿ_ತುದಿ_ಒಳ_ರಿಜಿಸ್ಟರ್(o)) {
        op = c_preserve_ಗಣ_ಸ್ಥಾನ[ತಳ_Type_ದ(type)];
    } ಇಲ್ಲದಿದ್ದರೆ {
        op = Is64_ಬಿಟ್_Type(type) ? op_Preserve_Copy_ಸ್ಥಾನ_64 : op_Preserve_Copy_ಸ್ಥಾನ_32;
    }

_   (EmitOp(o, op));
    Emit_ಸ್ಥಾನ_ಸ್ಥಾನಾಂತರ(o, i_dest_ಸ್ಥಾನ);

    ಆದರೆ (Is_ರಾಶಿ_ತುದಿ_ಒಳ_ಸ್ಥಾನ(o)) {
        Emit_ಸ್ಥಾನ_ಸ್ಥಾನಾಂತರ(o, Get_ರಾಶಿ_ತುದಿ_ಸ್ಥಾನ_ಸಂಖ್ಯೆ(o));
    }

    Emit_ಸ್ಥಾನ_ಸ್ಥಾನಾಂತರ(o, i_preserve_ಸ್ಥಾನ);

    _catch: ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}

ಸ್ಥಿರ
M3_ಫಲಿತಾಂಶ Copy_ರಾಶಿ_ತುದಿ_ಗಮ್ಯ_ರಿಜಿಸ್ಟರ್ (IM3Compilation o, bool i_update_ರಾಶಿ)
{
    M3_ಫಲಿತಾಂಶ ಫಲಿತಾಂಶ = m3Err_none;

    ಆದರೆ (Is_ರಾಶಿ_ತುದಿ_ಒಳ_ಸ್ಥಾನ(o)) {
        m3type_t type = Get_ರಾಶಿ_ತುದಿ_Type(o);

_       (Preserve_ರಿಜಿಸ್ಟರ್_If_Occupied(o, type));

        IM3Operation op = c_ಗಣ_ರಿಜಿಸ್ಟರ್_Ops[ತಳ_Type_ದ(type)];

_       (EmitOp(o, op));
        Emit_ಸ್ಥಾನ_ಸ್ಥಾನಾಂತರ(o, Get_ರಾಶಿ_ತುದಿ_ಸ್ಥಾನ_ಸಂಖ್ಯೆ(o));

        ಆದರೆ (i_update_ರಾಶಿ) {
_           (ತೆಗೆ_Type(o, type));
_           (ತಳ್ಳು_ರಿಜಿಸ್ಟರ್(o, type));
        }
    }

    _catch: ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}


// if local is unreferenced, o_preservedSlotNumber will be equal to localIndex on return
ಸ್ಥಿರ
M3_ಫಲಿತಾಂಶ ಕಂಡುಹಿಡಿ_Referenced_Local_Within_Current_ಖಂಡ (IM3Compilation o, u16* o_preserved_ಸ್ಥಾನ_ಸಂಖ್ಯೆ, u32 i_local_ಸ್ಥಾನ)
{
    M3_ಫಲಿತಾಂಶ ಫಲಿತಾಂಶ = m3Err_none;

    IM3CompilationScope scope = &o->ಖಂಡ;

    u16 ಆರಂಭ_ಸೂಚ್ಯಂಕ = scope->ಖಂಡ_ರಾಶಿ_ಸೂಚ್ಯಂಕ;

    ಆಗ (scope->opcode == c_wa_Op_ಖಂಡ) {
        scope = scope->outer;
        ಆದರೆ (not scope) {
            ನಿಲ್ಲಿಸು;
        }

        ಆರಂಭ_ಸೂಚ್ಯಂಕ = scope->ಖಂಡ_ರಾಶಿ_ಸೂಚ್ಯಂಕ;
    }

    *o_preserved_ಸ್ಥಾನ_ಸಂಖ್ಯೆ = (u16)i_local_ಸ್ಥಾನ;

    ಗಾಗಿ (u32 i = ಆರಂಭ_ಸೂಚ್ಯಂಕ; i < o->ರಾಶಿ_ಸೂಚ್ಯಂಕ; ++i) {
        ಆದರೆ (o->wasm_ರಾಶಿ[i] == i_local_ಸ್ಥಾನ) {
            m3type_t type = Get_ರಾಶಿ_Type_From_Bottom(o, i);                         d_m3Assert (type != c_m3Type_none)

            ಆದರೆ (*o_preserved_ಸ್ಥಾನ_ಸಂಖ್ಯೆ == i_local_ಸ್ಥಾನ) {
_               (AllocateSlots(o, o_preserved_ಸ್ಥಾನ_ಸಂಖ್ಯೆ, type));
            } ಇಲ್ಲದಿದ್ದರೆ {
                // A 64-bit value spans two slots, and allocating or releasing it
                // counts both, so a second reference has to count both as well.
                ಗಾಗಿ (u16 s = 0; s < Get_Type_ಅಂಕೆ_Slots(type); ++s) {
_                   (Increment_ಸ್ಥಾನ_Usage_ಎಣಿಕೆ(o, (u16)(*o_preserved_ಸ್ಥಾನ_ಸಂಖ್ಯೆ + s)));
                }
            }

            o->wasm_ರಾಶಿ[i] = *o_preserved_ಸ್ಥಾನ_ಸಂಖ್ಯೆ;
        }
    }

    _catch: ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}

ಸ್ಥಿರ
M3_ಫಲಿತಾಂಶ Get_ಖಂಡ_Scope (IM3Compilation o, IM3CompilationScope* o_scope, u32 i_ಆಳ)
{
    M3_ಫಲಿತಾಂಶ ಫಲಿತಾಂಶ = m3Err_none;

    IM3CompilationScope scope = &o->ಖಂಡ;

    ಆಗ (i_ಆಳ--) {
        scope = scope->outer;
        _throwif("invalid block depth", not scope);
    }

    *o_scope = scope;

    _catch:
    ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}

ಸ್ಥಿರ
M3_ಫಲಿತಾಂಶ Copy_ರಾಶಿ_Slots_R (IM3Compilation o, u16 i_ಗುರಿ_ಸ್ಥಾನ_ರಾಶಿ_ಸೂಚ್ಯಂಕ, u16 i_ರಾಶಿ_ಸೂಚ್ಯಂಕ, u16 i_ಕೊನೆ_ರಾಶಿ_ಸೂಚ್ಯಂಕ, u16 i_ತಾತ್ಕಾಲಿಕ_ಸ್ಥಾನ)
{
    M3_ಫಲಿತಾಂಶ ಫಲಿತಾಂಶ = m3Err_none;

    ಆದರೆ (i_ರಾಶಿ_ಸೂಚ್ಯಂಕ < i_ಕೊನೆ_ರಾಶಿ_ಸೂಚ್ಯಂಕ) {
        u16 src_ಸ್ಥಾನ = Get_ಸ್ಥಾನ_For_ರಾಶಿ_ಸೂಚ್ಯಂಕ(o, i_ರಾಶಿ_ಸೂಚ್ಯಂಕ);

        m3type_t type      = Get_ರಾಶಿ_Type_From_Bottom(o, i_ರಾಶಿ_ಸೂಚ್ಯಂಕ);
        u16      ಅಂಕೆ_Slots  = Get_Type_ಅಂಕೆ_Slots(type);
        u16      extra_ಸ್ಥಾನ = ಅಂಕೆ_Slots - 1;

        u16 ಗುರಿ_ಸ್ಥಾನ = Get_ಸ್ಥಾನ_For_ರಾಶಿ_ಸೂಚ್ಯಂಕ(o, i_ಗುರಿ_ಸ್ಥಾನ_ರಾಶಿ_ಸೂಚ್ಯಂಕ);

        u16 preserve_ಸೂಚ್ಯಂಕ = i_ರಾಶಿ_ಸೂಚ್ಯಂಕ;
        u16 collision_ಸ್ಥಾನ = src_ಸ್ಥಾನ;

        ಆದರೆ (ಗುರಿ_ಸ್ಥಾನ != src_ಸ್ಥಾನ) {
            // search for collisions
            u16 check_ಸೂಚ್ಯಂಕ = i_ರಾಶಿ_ಸೂಚ್ಯಂಕ + 1;
            ಆಗ (check_ಸೂಚ್ಯಂಕ < i_ಕೊನೆ_ರಾಶಿ_ಸೂಚ್ಯಂಕ) {
                u16 otherSlot1 = Get_ಸ್ಥಾನ_For_ರಾಶಿ_ಸೂಚ್ಯಂಕ(o, check_ಸೂಚ್ಯಂಕ);
                u16 otherSlot2 = Get_Extra_ಸ್ಥಾನ_For_ರಾಶಿ_ಸೂಚ್ಯಂಕ(o, check_ಸೂಚ್ಯಂಕ);

                ಆದರೆ (ಗುರಿ_ಸ್ಥಾನ == otherSlot1 or
                    ಗುರಿ_ಸ್ಥಾನ == otherSlot2 or
                    ಗುರಿ_ಸ್ಥಾನ + extra_ಸ್ಥಾನ == otherSlot1) {
                    _throwif(m3_Err_function_ರಾಶಿ_ಅತಿಪ್ರವಾಹ, i_ತಾತ್ಕಾಲಿಕ_ಸ್ಥಾನ >= d_m3MaxFunctionSlots);

_                   (Copy_ರಾಶಿ_ಸೂಚ್ಯಂಕ_ಗಮ್ಯ_ಸ್ಥಾನ(o, i_ತಾತ್ಕಾಲಿಕ_ಸ್ಥಾನ, check_ಸೂಚ್ಯಂಕ));
                    o->wasm_ರಾಶಿ[check_ಸೂಚ್ಯಂಕ] = i_ತಾತ್ಕಾಲಿಕ_ಸ್ಥಾನ;
                    i_ತಾತ್ಕಾಲಿಕ_ಸ್ಥಾನ += Get_Type_ಅಂಕೆ_Slots(c_m3Type_i64);
                    Touch_ಸ್ಥಾನ(o, i_ತಾತ್ಕಾಲಿಕ_ಸ್ಥಾನ - 1);

                    // restore this on the way back down
                    preserve_ಸೂಚ್ಯಂಕ = check_ಸೂಚ್ಯಂಕ;
                    collision_ಸ್ಥಾನ = otherSlot1;

                    ನಿಲ್ಲಿಸು;
                }

                ++check_ಸೂಚ್ಯಂಕ;
            }

_           (Copy_ರಾಶಿ_ಸೂಚ್ಯಂಕ_ಗಮ್ಯ_ಸ್ಥಾನ(o, ಗುರಿ_ಸ್ಥಾನ, i_ರಾಶಿ_ಸೂಚ್ಯಂಕ));                                                m3log (compile, " copying slot: %d to slot: %d", src_ಸ್ಥಾನ, ಗುರಿ_ಸ್ಥಾನ);
            o->wasm_ರಾಶಿ[i_ರಾಶಿ_ಸೂಚ್ಯಂಕ] = ಗುರಿ_ಸ್ಥಾನ;
        }

_       (Copy_ರಾಶಿ_Slots_R(o, i_ಗುರಿ_ಸ್ಥಾನ_ರಾಶಿ_ಸೂಚ್ಯಂಕ + 1, i_ರಾಶಿ_ಸೂಚ್ಯಂಕ + 1, i_ಕೊನೆ_ರಾಶಿ_ಸೂಚ್ಯಂಕ, i_ತಾತ್ಕಾಲಿಕ_ಸ್ಥಾನ));

        // restore the stack state
        o->wasm_ರಾಶಿ[i_ರಾಶಿ_ಸೂಚ್ಯಂಕ]  = src_ಸ್ಥಾನ;
        o->wasm_ರಾಶಿ[preserve_ಸೂಚ್ಯಂಕ] = collision_ಸ್ಥಾನ;
    }

    _catch:
    ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}

ಸ್ಥಿರ
M3_ಫಲಿತಾಂಶ Resolve_ಖಂಡ_Results (IM3Compilation o, IM3CompilationScope i_ಗುರಿ_ಖಂಡ, bool i_isBranch)
{
    M3_ಫಲಿತಾಂಶ ಫಲಿತಾಂಶ = m3Err_none;
    ಆದರೆ (d_m3_Log_Wasm_ರಾಶಿ) {
        dump_type_ರಾಶಿ(o);
    }

    bool isLoop = (i_ಗುರಿ_ಖಂಡ->opcode == c_waOp_loop and i_isBranch);

    u16 ಅಂಕೆ_Params  = Get_Func_Type_ಅಂಕೆ_Params(i_ಗುರಿ_ಖಂಡ->type);
    u16 ಅಂಕೆ_Results = Get_Func_Type_ಅಂಕೆ_Results(i_ಗುರಿ_ಖಂಡ->type);

    u16 ಸ್ಥಾನ_ದಾಖಲೆಗಳು = i_ಗುರಿ_ಖಂಡ->exit_ರಾಶಿ_ಸೂಚ್ಯಂಕ;

    u16 ಅಂಕೆ_ಮೌಲ್ಯಗಳು;

    ಆದರೆ (not isLoop) {
        ಅಂಕೆ_ಮೌಲ್ಯಗಳು = ಅಂಕೆ_Results;
        ಸ್ಥಾನ_ದಾಖಲೆಗಳು += ಅಂಕೆ_Params;
    } ಇಲ್ಲದಿದ್ದರೆ {
        ಅಂಕೆ_ಮೌಲ್ಯಗಳು = ಅಂಕೆ_Params;
    }

    u16 ಖಂಡ_ಎತ್ತರ = Get_ಅಂಕೆ_ಖಂಡ_ಮೌಲ್ಯಗಳು_On_ರಾಶಿ(o);

    _throwif(m3_Err_type_ಎಣಿಕೆ_Mismatch, i_isBranch ? (ಖಂಡ_ಎತ್ತರ < ಅಂಕೆ_ಮೌಲ್ಯಗಳು) : (ಖಂಡ_ಎತ್ತರ != ಅಂಕೆ_ಮೌಲ್ಯಗಳು));

    ಆದರೆ (ಅಂಕೆ_ಮೌಲ್ಯಗಳು) {
        u16 ಕೊನೆ_ಸೂಚ್ಯಂಕ     = Get_ರಾಶಿ_ತುದಿ_ಸೂಚ್ಯಂಕ(o) + 1;
        u16 ಅಂಕೆ_Rem_ಮೌಲ್ಯಗಳು = ಅಂಕೆ_ಮೌಲ್ಯಗಳು;

        // The last result is taken from _fp0. See PushBlockResults.
        ಆದರೆ (not isLoop and IsFpType(Get_ರಾಶಿ_ತುದಿ_Type(o))) {
_           (Copy_ರಾಶಿ_ತುದಿ_ಗಮ್ಯ_ರಿಜಿಸ್ಟರ್(o, false));
            --ಕೊನೆ_ಸೂಚ್ಯಂಕ;
            --ಅಂಕೆ_Rem_ಮೌಲ್ಯಗಳು;
        }

        // tempslot affects maxStackSlots, so can grow unnecess each time.
        // in practice, this is hardly ever hit.
        u16 ತಾತ್ಕಾಲಿಕ_ಸ್ಥಾನ = o->max_ರಾಶಿ_Slots;// GetMaxUsedSlotPlusOne (o); doesn't work cause can collide with slotRecords
        Align_ಸ್ಥಾನ_ಗಮ್ಯ_Type(&ತಾತ್ಕಾಲಿಕ_ಸ್ಥಾನ, c_m3Type_i64);

_       (Copy_ರಾಶಿ_Slots_R(o, ಸ್ಥಾನ_ದಾಖಲೆಗಳು, ಕೊನೆ_ಸೂಚ್ಯಂಕ - ಅಂಕೆ_Rem_ಮೌಲ್ಯಗಳು, ಕೊನೆ_ಸೂಚ್ಯಂಕ, ತಾತ್ಕಾಲಿಕ_ಸ್ಥಾನ));

        ಆದರೆ (d_m3_Log_Wasm_ರಾಶಿ) {
            dump_type_ರಾಶಿ(o);
        }
    }

    _catch: ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}


ಸ್ಥಿರ
M3_ಫಲಿತಾಂಶ Return_ಮೌಲ್ಯಗಳು (IM3Compilation o, IM3CompilationScope i_function_ಖಂಡ, bool i_isBranch)
{
    M3_ಫಲಿತಾಂಶ ಫಲಿತಾಂಶ = m3Err_none;
    ಆದರೆ (d_m3_Log_Wasm_ರಾಶಿ) {
        dump_type_ರಾಶಿ(o);
    }

    u16 ಅಂಕೆ_Returns  = Get_Func_Type_ಅಂಕೆ_Results(i_function_ಖಂಡ->type);     // could just o->function too...
    u16 ಖಂಡ_ಎತ್ತರ = Get_ಅಂಕೆ_ಖಂಡ_ಮೌಲ್ಯಗಳು_On_ರಾಶಿ(o);

    ಆದರೆ (not Is_ರಾಶಿ_Polymorphic(o)) {
        _throwif(m3_Err_type_ಎಣಿಕೆ_Mismatch, i_isBranch ? (ಖಂಡ_ಎತ್ತರ < ಅಂಕೆ_Returns) : (ಖಂಡ_ಎತ್ತರ != ಅಂಕೆ_Returns));
    }

    ಆದರೆ (ಅಂಕೆ_Returns) {
        // return slots like args are 64-bit aligned
        u16 return_ಸ್ಥಾನ = ಅಂಕೆ_Returns * c_io_ಸ್ಥಾನ_ಎಣಿಕೆ;
        u16 ರಾಶಿ_ತುದಿ   = Get_ರಾಶಿ_ತುದಿ_ಸೂಚ್ಯಂಕ(o);

        ಗಾಗಿ (u16 i = 0; i < ಅಂಕೆ_Returns; ++i) {
            m3type_t returnType = Get_Func_Type_ಫಲಿತಾಂಶ_Type(i_function_ಖಂಡ->type, ಅಂಕೆ_Returns - 1 - i);

            m3type_t ರಾಶಿ_Type = Get_ರಾಶಿ_Type_From_ತುದಿ(o, i);  // using FromTop so that only dynamic items are checked

            ಆದರೆ (Is_ರಾಶಿ_Polymorphic(o) and ರಾಶಿ_Type == c_m3Type_none) {
                ರಾಶಿ_Type = returnType;
            }

            _throwif(m3Err_typeMismatch, not Is_Sub_Type_ದ(ರಾಶಿ_Type, returnType));

            ಆದರೆ (not Is_ರಾಶಿ_Polymorphic(o)) {
                return_ಸ್ಥಾನ -= c_io_ಸ್ಥಾನ_ಎಣಿಕೆ;
_               (Copy_ರಾಶಿ_ಸೂಚ್ಯಂಕ_ಗಮ್ಯ_ಸ್ಥಾನ(o, return_ಸ್ಥಾನ, ರಾಶಿ_ತುದಿ--));
            }
        }

        ಆದರೆ (not i_isBranch) {
            ಆಗ (ಅಂಕೆ_Returns--) {
_               (ತೆಗೆ(o));
            }
        }
    }

    _catch: ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}


//-------------------------------------------------------------------------------------------------------------------------

ಸ್ಥಿರ
M3_ಫಲಿತಾಂಶ Compile_Const_i32 (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_ಫಲಿತಾಂಶ ಫಲಿತಾಂಶ;

    i32 ಮೌಲ್ಯ;
_   (ReadLEB_i32(&ಮೌಲ್ಯ, &o->wasm, o->wasm_ಕೊನೆ));
_   (ತಳ್ಳು_Const(o, ಮೌಲ್ಯ, c_m3Type_i32));                        m3log (compile, d_indent " (const i32 = %" PRIi32 ")", get_indention_string (o), ಮೌಲ್ಯ);
    _catch: ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}

ಸ್ಥಿರ
M3_ಫಲಿತಾಂಶ Compile_Const_i64 (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_ಫಲಿತಾಂಶ ಫಲಿತಾಂಶ;

    i64 ಮೌಲ್ಯ;
_   (ReadLEB_i64(&ಮೌಲ್ಯ, &o->wasm, o->wasm_ಕೊನೆ));
_   (ತಳ್ಳು_Const(o, ಮೌಲ್ಯ, c_m3Type_i64));                        m3log (compile, d_indent " (const i64 = %" PRIi64 ")", get_indention_string (o), ಮೌಲ್ಯ);
    _catch: ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}


#ಆದರೆ d_m3ImplementFloat
ಸ್ಥಿರ
M3_ಫಲಿತಾಂಶ Compile_Const_f32 (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_ಫಲಿತಾಂಶ ಫಲಿತಾಂಶ;

    ಒಕ್ಕೂಟ {
        u32 u;
        f32 f;
    } ಮೌಲ್ಯ = { 0 };

_   (Read_f32(&ಮೌಲ್ಯ.f, &o->wasm, o->wasm_ಕೊನೆ));                 m3log (compile, d_indent " (const f32 = %" PRIf32 ")", get_indention_string (o), ಮೌಲ್ಯ.f);
_   (ತಳ್ಳು_Const(o, ಮೌಲ್ಯ.u, c_m3Type_f32));

    _catch: ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}

ಸ್ಥಿರ
M3_ಫಲಿತಾಂಶ Compile_Const_f64 (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_ಫಲಿತಾಂಶ ಫಲಿತಾಂಶ;

    ಒಕ್ಕೂಟ {
        u64 u;
        f64 f;
    } ಮೌಲ್ಯ = { 0 };

_   (Read_f64(&ಮೌಲ್ಯ.f, &o->wasm, o->wasm_ಕೊನೆ));                 m3log (compile, d_indent " (const f64 = %" PRIf64 ")", get_indention_string (o), ಮೌಲ್ಯ.f);
_   (ತಳ್ಳು_Const(o, ಮೌಲ್ಯ.u, c_m3Type_f64));

    _catch: ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}
#ಆದರೆ_ಅಂತ್ಯ

ಸ್ಥಿರ
M3_ಫಲಿತಾಂಶ Compile_ExtendedOpcode (IM3Compilation o, m3opcode_t i_opcode)
{
_try {
    u32 opcode;
_   (ReadLEB_u32(&opcode, &o->wasm, o->wasm_ಕೊನೆ));               m3log (compile, d_indent " (%s: %" PRIu32 ")", get_indention_string (o), (i_opcode == c_waOp_atomic) ? "FE" : "FC", opcode);

    // m3opcode_t carries only the low byte, so a wider sub-opcode is unknown
    // here rather than something to truncate into range
    _throwif(m3Err_unknownOpcode, opcode > (u32)((i_opcode == c_waOp_atomic) ? c_wa_Op_ಕೊನೆಯ_Atomic : c_wa_Op_ಕೊನೆಯ_Extended));

    i_opcode = (m3opcode_t)((i_opcode << 8) | opcode);

    //printf("Extended opcode: 0x%x\n", i_opcode);

    IM3OpInfo opInfo = GetOpInfo(i_opcode);
    _throwif(m3Err_unknownOpcode, not opInfo);

    M3Compiler compiler = GetCompiler(opInfo);
    _throwif(m3Err_noCompiler, not compiler);

_   ((*compiler)(o, i_opcode));

    o->ಹಿಂದಿನ_Opcode = i_opcode;

    } _catch: ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}

ಸ್ಥಿರ
M3_ಫಲಿತಾಂಶ Compile_Return (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_ಫಲಿತಾಂಶ ಫಲಿತಾಂಶ = m3Err_none;

    ಆದರೆ (not Is_ರಾಶಿ_Polymorphic(o)) {
        IM3CompilationScope functionScope;
_       (Get_ಖಂಡ_Scope(o, &functionScope, o->ಖಂಡ.ಆಳ));

_       (Return_ಮೌಲ್ಯಗಳು(o, functionScope, true));

_       (EmitOp(o, op_Return));

_       (ಗಣ_ರಾಶಿ_Polymorphic(o));
    }

    _catch: ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}

ಸ್ಥಿರ
M3_ಫಲಿತಾಂಶ Validate_ಖಂಡ_ಕೊನೆ (IM3Compilation o)
{
    M3_ಫಲಿತಾಂಶ ಫಲಿತಾಂಶ = m3Err_none;

    u16 ಅಂಕೆ_Results  = Get_Func_Type_ಅಂಕೆ_Results(o->ಖಂಡ.type);
    u16 ಖಂಡ_ಎತ್ತರ = Get_ಅಂಕೆ_ಖಂಡ_ಮೌಲ್ಯಗಳು_On_ರಾಶಿ(o);

    ಆದರೆ (not Is_ರಾಶಿ_Polymorphic(o)) {
        // Spec: at block end, stack height must match the number of results
        _throwif(m3_Err_type_ಎಣಿಕೆ_Mismatch, ಖಂಡ_ಎತ್ತರ != ಅಂಕೆ_Results);

        // Spec: result types must match expected types
        ಗಾಗಿ (u16 i = 0; i < ಅಂಕೆ_Results; ++i) {
            m3type_t expectedType = Get_Func_Type_ಫಲಿತಾಂಶ_Type(o->ಖಂಡ.type, ಅಂಕೆ_Results - 1 - i);
            m3type_t actualType   = Get_ರಾಶಿ_Type_From_ತುದಿ(o, i);
            _throwif(m3Err_typeMismatch, not Is_Sub_Type_ದ(actualType, expectedType));
        }
    }

    _catch: ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}

ಸ್ಥಿರ
M3_ಫಲಿತಾಂಶ Compile_ಕೊನೆ (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_ಫಲಿತಾಂಶ ಫಲಿತಾಂಶ = m3Err_none;                   //dump_type_stack (o);

    // function end:
    ಆದರೆ (o->ಖಂಡ.ಆಳ == 0) {
        Validate_ಖಂಡ_ಕೊನೆ(o);

        //      if (not IsStackPolymorphic (o))
        {
            // A constant expression has no function frame, but its value still
            // has to be copied down to slot 0, where EvaluateExpression reads it
            // back. The non-emitting init-expr walk is skipped: it has no block
            // type to return against.
            ಆದರೆ (o->function or o->ಪುಟ) {
_               (Return_ಮೌಲ್ಯಗಳು(o, &o->ಖಂಡ, false));
            }

_           (EmitOp(o, op_Return));
        }
    }

    _catch: ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}


ಸ್ಥಿರ
M3_ಫಲಿತಾಂಶ Compile_ಗಣ_Local (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_ಫಲಿತಾಂಶ ಫಲಿತಾಂಶ;

    u32 local_ಸೂಚ್ಯಂಕ;
_   (ReadLEB_u32(&local_ಸೂಚ್ಯಂಕ, &o->wasm, o->wasm_ಕೊನೆ));                           m3log (compile, d_indent " (index = %u)", get_indention_string (o), local_ಸೂಚ್ಯಂಕ);

    ಆದರೆ (local_ಸೂಚ್ಯಂಕ < Get_Function_ಅಂಕೆ_ವಾದಗಳು_And_Locals(o->function)) {
        // Spec: value type must match local type
        ಆದರೆ (not Is_ರಾಶಿ_Polymorphic(o)) {
            m3type_t localType    = Get_ರಾಶಿ_Type_From_Bottom(o, local_ಸೂಚ್ಯಂಕ);
            m3type_t ರಾಶಿ_ತುದಿ_Type = Get_ರಾಶಿ_ತುದಿ_Type(o);
            _throwif(m3Err_typeMismatch, ರಾಶಿ_ತುದಿ_Type != c_m3Type_none and localType != c_m3Type_none and
                                           not Is_Sub_Type_ದ(ರಾಶಿ_ತುದಿ_Type, localType));
#ಆದರೆ d_m3HasSnapshots
            ಆದರೆ (o->local_Init_ಆಳ and o->local_Init_ಆಳ[local_ಸೂಚ್ಯಂಕ] == UINT32_MAX) {
                o->local_Init_ಆಳ[local_ಸೂಚ್ಯಂಕ] = (u32)o->ಖಂಡ.ಆಳ;
            }
#ಆದರೆ_ಅಂತ್ಯ
        }

        u16 local_ಸ್ಥಾನ = Get_ಸ್ಥಾನ_For_ರಾಶಿ_ಸೂಚ್ಯಂಕ(o, local_ಸೂಚ್ಯಂಕ);

        u16 preserve_ಸ್ಥಾನ;
_       (ಕಂಡುಹಿಡಿ_Referenced_Local_Within_Current_ಖಂಡ(o, &preserve_ಸ್ಥಾನ, local_ಸ್ಥಾನ));  // preserve will be different than local, if referenced

        bool folded = false;

#ಆದರೆ d_m3_Fold_ಗಣ_Local
        // destination folding: when the value was produced by the immediately preceding op (nothing
        // emitted since, result on top in _r0, no copy-on-write preservation needed), retro-patch the
        // producer to a variant that writes straight to the local's slot and skip the SetSlot op
        ಆದರೆ (o->foldPatchPC and preserve_ಸ್ಥಾನ == local_ಸ್ಥಾನ and Is_ರಾಶಿ_ತುದಿ_ಒಳ_ರಿಜಿಸ್ಟರ್(o) and Get_ರಾಶಿ_ತುದಿ_ಸೂಚ್ಯಂಕ(o) == (i16)o->fold_ರಾಶಿ_ಸೂಚ್ಯಂಕ) {
            IM3Operation foldOp = GetFoldOp(o->foldOpcode, o->foldForm);

            ಆದರೆ (foldOp) {
                *(IM3Operation*)o->foldPatchPC = foldOp;     // retro-patch the producer
                Emit_ಸ್ಥಾನ_ಸ್ಥಾನಾಂತರ(o, local_ಸ್ಥಾನ);                  // append its destination
                InvalidateFold(o);
                folded = true;
            }
        }
#ಆದರೆ_ಅಂತ್ಯ

        ಆದರೆ (folded) {
            m3type_t type = Get_ರಾಶಿ_ತುದಿ_Type(o);
_           (ತೆಗೆ(o));                              // the value now lives in the local's slot

            // A folded op writes its result through to the register as well, so a tee
            // can keep it there. Pushing the slot instead would make the next use that
            // wants a register reload the slot this op just stored to.
            ಆದರೆ (i_opcode == c_waOp_teeLocal) {
_               (ತಳ್ಳು_ರಿಜಿಸ್ಟರ್(o, type));
            }
        } ಇಲ್ಲದಿದ್ದರೆ {
            ಆದರೆ (preserve_ಸ್ಥಾನ == local_ಸ್ಥಾನ) {
_               (Copy_ರಾಶಿ_ತುದಿ_ಗಮ್ಯ_ಸ್ಥಾನ(o, local_ಸ್ಥಾನ))
            } ಇಲ್ಲದಿದ್ದರೆ {
_               (Preserved_Copy_ತುದಿ_ಸ್ಥಾನ(o, local_ಸ್ಥಾನ, preserve_ಸ್ಥಾನ))
            }

            ಆದರೆ (i_opcode != c_waOp_teeLocal) {
_               (ತೆಗೆ(o));
            }
        }
    } ಇಲ್ಲದಿದ್ದರೆ {
        _throw("local index out of bounds");
    }

    _catch: ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}

ಸ್ಥಿರ
M3_ಫಲಿತಾಂಶ Compile_GetLocal (IM3Compilation o, m3opcode_t i_opcode)
{
_try {

    u32 local_ಸೂಚ್ಯಂಕ;
_   (ReadLEB_u32(&local_ಸೂಚ್ಯಂಕ, &o->wasm, o->wasm_ಕೊನೆ));                           m3log (compile, d_indent " (index = %u)", get_indention_string (o), local_ಸೂಚ್ಯಂಕ);

    ಆದರೆ (local_ಸೂಚ್ಯಂಕ >= Get_Function_ಅಂಕೆ_ವಾದಗಳು_And_Locals(o->function)) {
        _throw("local index out of bounds");
    }

    m3type_t type = Get_ರಾಶಿ_Type_From_Bottom(o, local_ಸೂಚ್ಯಂಕ);
    u16      ಸ್ಥಾನ = Get_ಸ್ಥಾನ_For_ರಾಶಿ_ಸೂಚ್ಯಂಕ(o, local_ಸೂಚ್ಯಂಕ);

_   (ತಳ್ಳು(o, type, ಸ್ಥಾನ));

    } _catch: ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}

ಸ್ಥಿರ
M3_ಫಲಿತಾಂಶ Compile_GetGlobal (IM3Compilation o, M3Global* i_global)
{
    M3_ಫಲಿತಾಂಶ ಫಲಿತಾಂಶ;

    IM3Operation op = Is64_ಬಿಟ್_Type(i_global->type) ? op_GetGlobal_s64 : op_GetGlobal_s32;
_   (EmitOp(o, op));
    Emit_ಸೂಚಕ(o, &i_global->i64_ಮೌಲ್ಯ);
_   (ತಳ್ಳು_Allocated_ಸ್ಥಾನ_And_Emit_2(o, i_global->type));

    _catch: ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}

ಸ್ಥಿರ
M3_ಫಲಿತಾಂಶ Compile_ಗಣ_Global (IM3Compilation o, M3Global* i_global)
{
    M3_ಫಲಿತಾಂಶ ಫಲಿತಾಂಶ = m3Err_none;

    ಆದರೆ (i_global->isMutable) {
        // Spec: value type must match global type
        ಆದರೆ (not Is_ರಾಶಿ_Polymorphic(o)) {
            m3type_t ರಾಶಿ_ತುದಿ_Type = Get_ರಾಶಿ_ತುದಿ_Type(o);
            _throwif(m3Err_typeMismatch, ರಾಶಿ_ತುದಿ_Type != c_m3Type_none and
                                           not Is_Sub_Type_ದ(ರಾಶಿ_ತುದಿ_Type, i_global->type));
        }

        IM3Operation op;
        m3type_t     type = Get_ರಾಶಿ_ತುದಿ_Type(o);

        ಆದರೆ (Is_ರಾಶಿ_ತುದಿ_ಒಳ_ರಿಜಿಸ್ಟರ್(o)) {
            op = c_ಗಣ_Global_Ops[ತಳ_Type_ದ(type)];
        } ಇಲ್ಲದಿದ್ದರೆ {
            op = Is64_ಬಿಟ್_Type(type) ? op_ಗಣ_Global_s64 : op_ಗಣ_Global_s32;
        }

_       (EmitOp(o, op));
        Emit_ಸೂಚಕ(o, &i_global->i64_ಮೌಲ್ಯ);

        ಆದರೆ (Is_ರಾಶಿ_ತುದಿ_ಒಳ_ಸ್ಥಾನ(o)) {
            Emit_ಸ್ಥಾನ_ಸ್ಥಾನಾಂತರ(o, Get_ರಾಶಿ_ತುದಿ_ಸ್ಥಾನ_ಸಂಖ್ಯೆ(o));
        }

_       (ತೆಗೆ(o));
    } ಇಲ್ಲದಿದ್ದರೆ {
        _throw(m3Err_settingImmutableGlobal);
    }

    _catch: ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}

ಸ್ಥಿರ
M3_ಫಲಿತಾಂಶ Compile_Get_ಗಣ_Global (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_ಫಲಿತಾಂಶ ಫಲಿತಾಂಶ = m3Err_none;

    u32 global_ಸೂಚ್ಯಂಕ;
_   (ReadLEB_u32(&global_ಸೂಚ್ಯಂಕ, &o->wasm, o->wasm_ಕೊನೆ));

    ಆದರೆ (global_ಸೂಚ್ಯಂಕ < o->module->ಅಂಕೆ_Globals) {
        ಆದರೆ (o->module->globals) {
            M3Global* global = &o->module->globals[global_ಸೂಚ್ಯಂಕ];

            // Spec: a constant expression may only read an imported immutable
            // global. The module's own globals are counted before their
            // initializer is walked, so a bare index check would let one
            // reference itself. These describe how the module declared the
            // global, so they are checked before following any link.
            _throwif(m3_Err_globa_ಸೂಚ್ಯಂಕ_ಹೊರಹೋಗು_ದ_Bounds, o->is_Init_ಅಭಿವ್ಯಕ್ತಿ and not global->imported);
            _throwif(m3Err_wasmMalformed, o->is_Init_ಅಭಿವ್ಯಕ್ತಿ and global->isMutable);

            // an import linked to another module reads and writes that module's
            // cell, not the placeholder standing in for it here
            ಆದರೆ (global->resolved) {
                global = global->resolved;
            }

_           ((i_opcode == c_waOp_getGlobal) ? Compile_GetGlobal(o, global) : Compile_ಗಣ_Global(o, global));
        } ಇಲ್ಲದಿದ್ದರೆ {
            _throw(ErrorCompile(m3_Err_global_ಸ್ಮರಣೆ_Not_Allocated, o, "module '%s' is missing global memory", o->module->name));
        }
    } ಇಲ್ಲದಿದ್ದರೆ {
        _throw(m3_Err_globa_ಸೂಚ್ಯಂಕ_ಹೊರಹೋಗು_ದ_Bounds);
    }

    _catch: ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}

ಸ್ಥಿರ
ಶೂನ್ಯ Emit_Patching_Branch_ಸೂಚಕ (IM3Compilation o, IM3CompilationScope i_scope)
{
    pc_t patch       = Emit_ಸೂಚಕ(o, i_scope->patches);                m3log (compile, "branch patch required at: %p", patch);
    i_scope->patches = patch;
}

ಸ್ಥಿರ
M3_ಫಲಿತಾಂಶ EmitPatchingBranch (IM3Compilation o, IM3CompilationScope i_scope)
{
    M3_ಫಲಿತಾಂಶ ಫಲಿತಾಂಶ = m3Err_none;

_   (EmitOp(o, op_Branch));
    Emit_Patching_Branch_ಸೂಚಕ(o, i_scope);

    _catch: ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}

#ಆದರೆ d_m3HasExceptionHandling

// How many try_table handler records an exit from i_from out to i_target takes
// down with it. Walking outward from the block the branch sits in, every
// try_table scope up to and including the target is left behind - the target
// too, because landing on a block's label means that block is finished.
//
// A loop label is the exception, but a loop is never a try_table, so the count
// is the same either way: the loop itself is not on the list.
ಸ್ಥಿರ
u32 ಎಣಿಕೆ_Try_ಚೌಕಟ್ಟುಗಳು_ಗಮ್ಯ_Exit (IM3CompilationScope i_from, IM3CompilationScope i_ಗುರಿ)
{
    u32 ಎಣಿಕೆ_2 = 0;

    ಗಾಗಿ (IM3CompilationScope scope = i_from; scope; scope = scope->outer) {
        ಆದರೆ (scope->opcode == c_wa_Op_try_ಕೋಷ್ಟಕ) {
            ++ಎಣಿಕೆ_2;
        }

        ಆದರೆ (scope == i_ಗುರಿ) {
            ನಿಲ್ಲಿಸು;
        }
    }

    ಹಿಂತಿರುಗಿ ಎಣಿಕೆ_2;
}


// The handler records a branch out of i_from to i_target has to drop.
//
// A branch to the function's own label compiles to op_Return, and returning
// unwinds op_TryTable's native frame, which drops that record on its own, so
// those need no pop at the branch site.
//
// i_from is where the walk starts, which is not always the block the branch is
// written in: a catch stub passes the scope outside its try, because
// op_TryTable takes its own record off before jumping to the stub.
ಸ್ಥಿರ
u32 ಅಂಕೆ_Try_ಚೌಕಟ್ಟುಗಳು_ಗಮ್ಯ_ತೆಗೆ (IM3CompilationScope i_from, IM3CompilationScope i_ಗುರಿ)
{
    ಆದರೆ (i_ಗುರಿ->ಆಳ == 0) {
        ಹಿಂತಿರುಗಿ 0;
    }

    // the walk only ever runs inward-to-outward; a target that is already
    // outside i_from has nothing between them
    ಆದರೆ (not i_from or i_from->ಆಳ < i_ಗುರಿ->ಆಳ) {
        ಹಿಂತಿರುಗಿ 0;
    }

    ಹಿಂತಿರುಗಿ ಎಣಿಕೆ_Try_ಚೌಕಟ್ಟುಗಳು_ಗಮ್ಯ_Exit(i_from, i_ಗುರಿ);
}


ಸ್ಥಿರ
M3_ಫಲಿತಾಂಶ Emit_ತೆಗೆ_Try_ಚೌಕಟ್ಟುಗಳು (IM3Compilation o, u32 i_ಅಂಕೆ_ಚೌಕಟ್ಟುಗಳು)
{
    M3_ಫಲಿತಾಂಶ ಫಲಿತಾಂಶ = m3Err_none;

    ಆದರೆ (i_ಅಂಕೆ_ಚೌಕಟ್ಟುಗಳು) {
_       (EmitOp(o, op_ತೆಗೆ_Handlers));
        EmitConstant32(o, i_ಅಂಕೆ_ಚೌಕಟ್ಟುಗಳು);
    }

    _catch: ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}


// A return_call leaves the enclosing function before the callee runs, so no
// try_table in this function may catch what the callee throws. Neither shape of
// the instruction unwinds the native stack in time to arrange that on its own:
// op_ReturnCall tail-jumps with op_TryTable's frames still standing, and the
// fallback shape is an ordinary call sitting inside them. Either way the
// handler records have to come off first.
ಸ್ಥಿರ
M3_ಫಲಿತಾಂಶ Emit_ತೆಗೆ_Try_ಚೌಕಟ್ಟುಗಳು_For_Return_Call (IM3Compilation o)
{
    u32 ಎಣಿಕೆ_2 = 0;

    ಗಾಗಿ (IM3CompilationScope scope = &o->ಖಂಡ; scope; scope = scope->outer) {
        ಆದರೆ (scope->opcode == c_wa_Op_try_ಕೋಷ್ಟಕ) {
            ++ಎಣಿಕೆ_2;
        }
    }

    ಹಿಂತಿರುಗಿ Emit_ತೆಗೆ_Try_ಚೌಕಟ್ಟುಗಳು(o, ಎಣಿಕೆ_2);
}

#ಇಲ್ಲದಿದ್ದರೆ

ಸ್ಥಿರ
M3_ಫಲಿತಾಂಶ Emit_ತೆಗೆ_Try_ಚೌಕಟ್ಟುಗಳು (IM3Compilation o, u32 i_ಅಂಕೆ_ಚೌಕಟ್ಟುಗಳು)
{
    (ಶೂನ್ಯ)o;
    (ಶೂನ್ಯ)i_ಅಂಕೆ_ಚೌಕಟ್ಟುಗಳು;
    ಹಿಂತಿರುಗಿ m3Err_none;
}

#ಆದರೆ_ಅಂತ್ಯ // d_m3HasExceptionHandling

#ಆದರೆ d_m3FuseBranch
// when the pending fold candidate is a compare whose result sits on top in _r0, return the fused
// variant that absorbs an immediately following br_if (i_isIf false) or if (i_isIf true)
ಸ್ಥಿರ
IM3Operation GetBranchFusionOp (IM3Compilation o, bool i_isIf)
{
    ಆದರೆ (o->foldPatchPC and o->foldForm < 4 and Is_ರಾಶಿ_ತುದಿ_ಒಳ_ರಿಜಿಸ್ಟರ್(o) and Get_ರಾಶಿ_ತುದಿ_ಸೂಚ್ಯಂಕ(o) == (i16)o->fold_ರಾಶಿ_ಸೂಚ್ಯಂಕ) {
        ಸ್ಥಿರಾಂಕ M3FusedCmpOps* ops = GetFusedCmpOps(o->foldOpcode);

        ಆದರೆ (ops) {
            ಹಿಂತಿರುಗಿ i_isIf ? ops->ifOp[o->foldForm] : ops->branchIf[o->foldForm];
        }
    }

    ಹಿಂತಿರುಗಿ ಖಾಲಿ;
}
#ಆದರೆ_ಅಂತ್ಯ // d_m3FuseBranch

ಸ್ಥಿರ ಸಾಲೊಳಗೆ
IM3Operation Op_ContinueLoop (IM3Compilation o)
{
#ಆದರೆ d_m3_Has_ರಾಶಿ_Switching
    ಆದರೆ (M3_UNLIKELY(o->module->runtime and o->module->runtime->isSuspendable)) {
        ಹಿಂತಿರುಗಿ op_ContinueLoop_Suspendable;
    }
#ಆದರೆ_ಅಂತ್ಯ
    ಹಿಂತಿರುಗಿ op_ContinueLoop;
}

ಸ್ಥಿರ ಸಾಲೊಳಗೆ
IM3Operation Op_ContinueLoopIf (IM3Compilation o)
{
#ಆದರೆ d_m3_Has_ರಾಶಿ_Switching
    ಆದರೆ (M3_UNLIKELY(o->module->runtime and o->module->runtime->isSuspendable)) {
        ಹಿಂತಿರುಗಿ op_ContinueLoopIf_Suspendable;
    }
#ಆದರೆ_ಅಂತ್ಯ
    ಹಿಂತಿರುಗಿ op_ContinueLoopIf;
}


ಸ್ಥಿರ
M3_ಫಲಿತಾಂಶ Compile_Branch (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_ಫಲಿತಾಂಶ ಫಲಿತಾಂಶ;
    u32      ಅಂಕೆ_Try_ಚೌಕಟ್ಟುಗಳು = 0;

    u32 ಆಳ;
_   (ReadLEB_u32(&ಆಳ, &o->wasm, o->wasm_ಕೊನೆ));

    // Spec: br_if condition must be i32
    ಆದರೆ (i_opcode == c_waOp_branchIf and not Is_ರಾಶಿ_Polymorphic(o)) {
        m3type_t condType = Get_ರಾಶಿ_ತುದಿ_Type(o);
        _throwif(m3Err_typeMismatch, condType != c_m3Type_none and condType != c_m3Type_i32);
    }

    IM3CompilationScope scope;
_   (Get_ಖಂಡ_Scope(o, &scope, ಆಳ));

#ಆದರೆ d_m3HasExceptionHandling
    ಅಂಕೆ_Try_ಚೌಕಟ್ಟುಗಳು = ಅಂಕೆ_Try_ಚೌಕಟ್ಟುಗಳು_ಗಮ್ಯ_ತೆಗೆ(&o->ಖಂಡ, scope);
#ಆದರೆ_ಅಂತ್ಯ

    // branch target is a loop (continue)
    ಆದರೆ (scope->opcode == c_waOp_loop) {
        ಆದರೆ (i_opcode == c_waOp_branchIf) {
            // a handler pop belongs on the taken path only, so it forces the
            // jump-over form even where the target takes no parameters
            ಆದರೆ (Get_Func_Type_ಅಂಕೆ_Params(scope->type) or ಅಂಕೆ_Try_ಚೌಕಟ್ಟುಗಳು) {
                IM3Operation op = Is_ರಾಶಿ_ತುದಿ_ಒಳ_ರಿಜಿಸ್ಟರ್(o) ? op_BranchIfPrologue_r : op_BranchIfPrologue_s;

_               (EmitOp(o, op));
_               (Emit_ಸ್ಥಾನ_ಅಂಕೆ_ದ_ರಾಶಿ_ತುದಿ_And_ತೆಗೆ(o));

                pc_t* jump_ಗಮ್ಯ = (pc_t*)Reserve_ಸೂಚಕ(o);

_               (Emit_ತೆಗೆ_Try_ಚೌಕಟ್ಟುಗಳು(o, ಅಂಕೆ_Try_ಚೌಕಟ್ಟುಗಳು));

                ಆದರೆ (not Is_ರಾಶಿ_Polymorphic(o)) {
_                   (Resolve_ಖಂಡ_Results(o, scope, /* isBranch: */ true));
                }

_               (EmitOp(o, Op_ContinueLoop(o)));
                Emit_ಸೂಚಕ(o, scope->pc);
_               (ದಾಖಲೆ_ಹಿಂಭಾಗ_ಅಂಚು(o, scope, false));

                *jump_ಗಮ್ಯ = GetPC(o);
            } ಇಲ್ಲದಿದ್ದರೆ {
                // move the condition to a register
_               (Copy_ರಾಶಿ_ತುದಿ_ಗಮ್ಯ_ರಿಜಿಸ್ಟರ್(o, false));
_               (ತೆಗೆ_Type(o, c_m3Type_i32));

_               (EmitOp(o, Op_ContinueLoopIf(o)));
                Emit_ಸೂಚಕ(o, scope->pc);
_               (ದಾಖಲೆ_ಹಿಂಭಾಗ_ಅಂಚು(o, scope, true));
            }

            //          dump_type_stack(o);
        } ಇಲ್ಲದಿದ್ದರೆ // is c_waOp_branch
        {
_           (Emit_ತೆಗೆ_Try_ಚೌಕಟ್ಟುಗಳು(o, ಅಂಕೆ_Try_ಚೌಕಟ್ಟುಗಳು));

            // the branch operands become the loop's parameters on the next iteration
            ಆದರೆ (not Is_ರಾಶಿ_Polymorphic(o)) {
_               (Resolve_ಖಂಡ_Results(o, scope, /* isBranch: */ true));
            }

_           (EmitOp(o, Op_ContinueLoop(o)));
            Emit_ಸೂಚಕ(o, scope->pc);
_           (ದಾಖಲೆ_ಹಿಂಭಾಗ_ಅಂಚು(o, scope, false));
            o->ಖಂಡ.isPolymorphic = true;
        }
    } ಇಲ್ಲದಿದ್ದರೆ // forward branch
    {
        pc_t* jump_ಗಮ್ಯ = ಖಾಲಿ;

        bool isReturn         = (scope->ಆಳ == 0);
        bool ಗುರಿ_Has_Results = Get_Func_Type_ಅಂಕೆ_Results(scope->type);

        ಆದರೆ (i_opcode == c_waOp_branchIf) {
            ಆದರೆ (ಗುರಿ_Has_Results or isReturn or ಅಂಕೆ_Try_ಚೌಕಟ್ಟುಗಳು) {
                IM3Operation op = Is_ರಾಶಿ_ತುದಿ_ಒಳ_ರಿಜಿಸ್ಟರ್(o) ? op_BranchIfPrologue_r : op_BranchIfPrologue_s;

_               (EmitOp(o, op));
_               (Emit_ಸ್ಥಾನ_ಅಂಕೆ_ದ_ರಾಶಿ_ತುದಿ_And_ತೆಗೆ(o)); // condition

                // this is continuation point, if the branch isn't taken
                jump_ಗಮ್ಯ = (pc_t*)Reserve_ಸೂಚಕ(o);
            } ಇಲ್ಲದಿದ್ದರೆ {
#ಆದರೆ d_m3FuseBranch
                IM3Operation fuseOp = GetBranchFusionOp(o, false);

                ಆದರೆ (fuseOp) {
                    // absorb the branch into the compare that produced the condition
                    *(IM3Operation*)o->foldPatchPC = fuseOp;
                    InvalidateFold(o);

_                   (ತೆಗೆ(o));                      // the condition is consumed inside the fused op

                    Emit_Patching_Branch_ಸೂಚಕ(o, scope);
                    ಹೋಗು _catch;
                }
#ಆದರೆ_ಅಂತ್ಯ
                IM3Operation op = Is_ರಾಶಿ_ತುದಿ_ಒಳ_ರಿಜಿಸ್ಟರ್(o) ? op_BranchIf_r : op_BranchIf_s;

_               (EmitOp(o, op));
_               (Emit_ಸ್ಥಾನ_ಅಂಕೆ_ದ_ರಾಶಿ_ತುದಿ_And_ತೆಗೆ(o)); // condition

                Emit_Patching_Branch_ಸೂಚಕ(o, scope);
                ಹೋಗು _catch;
            }
        }

        ಆದರೆ (not Is_ರಾಶಿ_Polymorphic(o)) {
            ಆದರೆ (isReturn) {
_               (Return_ಮೌಲ್ಯಗಳು(o, scope, true));
_               (EmitOp(o, op_Return));
            } ಇಲ್ಲದಿದ್ದರೆ {
_               (Emit_ತೆಗೆ_Try_ಚೌಕಟ್ಟುಗಳು(o, ಅಂಕೆ_Try_ಚೌಕಟ್ಟುಗಳು));

_               (Resolve_ಖಂಡ_Results(o, scope, true));
_               (EmitPatchingBranch(o, scope));
            }
        }

        ಆದರೆ (jump_ಗಮ್ಯ) {
            *jump_ಗಮ್ಯ = GetPC(o);
        }

        ಆದರೆ (i_opcode == c_waOp_branch) {
_           (ಗಣ_ರಾಶಿ_Polymorphic(o));
        }
    }

    _catch: ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}

ಸ್ಥಿರ
M3_ಫಲಿತಾಂಶ Compile_Branch_ಕೋಷ್ಟಕ (IM3Compilation o, m3opcode_t i_opcode)
{
    // the page a continue-op page is standing in for; non-NULL only while that
    // page is installed in o->page, so the catch below knows to put it back
    IM3_ಸಂಕೇತ_ಪುಟ displaced_ಪುಟ = ಖಾಲಿ;
    // indexed by label depth: the continuation stub already emitted for that target
    // within this table, or NULL if this table has not reached it yet
    pc_t* ಗುರಿ_Stubs = ಖಾಲಿ;
_try {
    u32 ಗುರಿ_ಎಣಿಕೆ;
_   (ReadLEB_u32(&ಗುರಿ_ಎಣಿಕೆ, &o->wasm, o->wasm_ಕೊನೆ));

    // Spec: validate that the branch index operand is i32
    ಆದರೆ (not Is_ರಾಶಿ_Polymorphic(o)) {
        m3type_t ಸೂಚ್ಯಂಕ_Type = Get_ರಾಶಿ_ತುದಿ_Type(o);
        _throwif(m3Err_typeMismatch, ಸೂಚ್ಯಂಕ_Type != c_m3Type_none and ಸೂಚ್ಯಂಕ_Type != c_m3Type_i32);
    }

_   (Preserve_ರಿಜಿಸ್ಟರ್_If_Occupied(o, c_m3Type_i64));         // move branch operand to a slot
    u16 ಸ್ಥಾನ = Get_ರಾಶಿ_ತುದಿ_ಸ್ಥಾನ_ಸಂಖ್ಯೆ(o);
_   (ತೆಗೆ(o));

    // OPTZ: according to spec: "forward branches that target a control instruction with a non-empty
    // result type consume matching operands first and push them back on the operand stack after unwinding"
    // So, this move-to-reg is only necessary if the target scopes have a type.

    u32 ಅಂಕೆ_ಸಂಕೇತ_ಸಾಲುಗಳು = ಗುರಿ_ಎಣಿಕೆ + 4; // 3 => IM3Operation + slot + target_count + default_target
_   (Ensure_ಸಂಕೇತ_ಪುಟ_ಅಂಕೆ_ಸಾಲುಗಳು(o, ಅಂಕೆ_ಸಂಕೇತ_ಸಾಲುಗಳು));

_   (EmitOp(o, op_Branch_ಕೋಷ್ಟಕ));
    Emit_ಸ್ಥಾನ_ಸ್ಥಾನಾಂತರ(o, ಸ್ಥಾನ);
    EmitConstant32(o, ಗುರಿ_ಎಣಿಕೆ);

    IM3_ಸಂಕೇತ_ಪುಟ continue_Op_ಪುಟ = ಖಾಲಿ;

    // The label types are checked by ValidateFunction, which runs under the same
    // d_m3EnableValidation guard just before this compilation pass. Comparing each
    // target against the default here would be wrong anyway: the spec matches every
    // target against the operand stack, so in unreachable code the operands are the
    // bottom type and the targets may legitimately differ (unreached-valid.wast).

    ++ಗುರಿ_ಎಣಿಕೆ; // include default

    // Every entry of one br_table is resolved from the same compilation state - each
    // stub restores whatever it touched - so two entries naming the same label emit
    // byte-identical epilogues. Emit one and point both table entries at it. A label
    // deeper than the current block is rejected by GetBlockScope below, which is what
    // bounds this array.
    ಗುರಿ_Stubs = m3_AllocArray(pc_t, (size_t)o->ಖಂಡ.ಆಳ + 1);
    _throwifnull(ಗುರಿ_Stubs);

    ಗಾಗಿ (u32 i = 0; i < ಗುರಿ_ಎಣಿಕೆ; ++i) {
        u32 ಗುರಿ;
_       (ReadLEB_u32(&ಗುರಿ, &o->wasm, o->wasm_ಕೊನೆ));

        IM3CompilationScope scope;
_       (Get_ಖಂಡ_Scope(o, &scope, ಗುರಿ));

        ಆದರೆ (ಗುರಿ_Stubs[ಗುರಿ]) {
            Emit_ಸೂಚಕ(o, ಗುರಿ_Stubs[ಗುರಿ]);
            ಮುಂದುವರಿ;
        }

        u32 ಅಂಕೆ_Try_ಚೌಕಟ್ಟುಗಳು = 0;
#ಆದರೆ d_m3HasExceptionHandling
        ಅಂಕೆ_Try_ಚೌಕಟ್ಟುಗಳು = ಅಂಕೆ_Try_ಚೌಕಟ್ಟುಗಳು_ಗಮ್ಯ_ತೆಗೆ(&o->ಖಂಡ, scope);
#ಆದರೆ_ಅಂತ್ಯ

        // A forward branch to a block that carries no results and leaves no try
        // frames behind has nothing to do on the way out but arrive. There is no
        // epilogue to hold, so the table entry becomes the patch site itself and the
        // block's address is written into it when the block ends - no stub, and no
        // code page to put one on. In unreachable code the same is true whatever the
        // block's type, since nothing is emitted for that case either way.
        //
        // Each such entry is patched on its own, so none is recorded in targetStubs:
        // there is no shared stub for a later entry naming this label to point at.
        ಆದರೆ (scope->opcode != c_waOp_loop and scope->ಆಳ > 0 and ಅಂಕೆ_Try_ಚೌಕಟ್ಟುಗಳು == 0 and
            (Is_ರಾಶಿ_Polymorphic(o) or Get_Func_Type_ಅಂಕೆ_Results(scope->type) == 0)) {
            Emit_Patching_Branch_ಸೂಚಕ(o, scope);
            ಮುಂದುವರಿ;
        }

_       (Acquire_Compilation_ಸಂಕೇತ_ಪುಟ(o, &continue_Op_ಪುಟ));

        pc_t ಆರಂಭ_PC  = Get_ಪುಟ_PC(continue_Op_ಪುಟ);
        displaced_ಪುಟ = o->ಪುಟ;
        o->ಪುಟ       = continue_Op_ಪುಟ;

        ಆದರೆ (scope->opcode == c_waOp_loop) {
_           (Emit_ತೆಗೆ_Try_ಚೌಕಟ್ಟುಗಳು(o, ಅಂಕೆ_Try_ಚೌಕಟ್ಟುಗಳು));

            ಆದರೆ (not Is_ರಾಶಿ_Polymorphic(o)) {
_               (Resolve_ಖಂಡ_Results(o, scope, true));
            }

_           (EmitOp(o, Op_ContinueLoop(o)));
            Emit_ಸೂಚಕ(o, scope->pc);
_           (ದಾಖಲೆ_ಹಿಂಭಾಗ_ಅಂಚು(o, scope, false));
        } ಇಲ್ಲದಿದ್ದರೆ {
            ಆದರೆ (not Is_ರಾಶಿ_Polymorphic(o)) {
                ಆದರೆ (scope->ಆಳ == 0) {
_                   (Return_ಮೌಲ್ಯಗಳು(o, scope, true));
_                   (EmitOp(o, op_Return));
                } ಇಲ್ಲದಿದ್ದರೆ {
_                   (Emit_ತೆಗೆ_Try_ಚೌಕಟ್ಟುಗಳು(o, ಅಂಕೆ_Try_ಚೌಕಟ್ಟುಗಳು));

_                   (Resolve_ಖಂಡ_Results(o, scope, true));

_                   (EmitPatchingBranch(o, scope));
                }
            }
        }

        IM3_ಸಂಕೇತ_ಪುಟ return_ಗಮ್ಯ = displaced_ಪುಟ;
        displaced_ಪುಟ        = ಖಾಲಿ;

        Release_Compilation_ಸಂಕೇತ_ಪುಟ(o);
        o->ಪುಟ = return_ಗಮ್ಯ;

        Emit_ಸೂಚಕ(o, ಆರಂಭ_PC);
        ಗುರಿ_Stubs[ಗುರಿ] = ಆರಂಭ_PC;
    }

_   (ಗಣ_ರಾಶಿ_Polymorphic(o));
} _catch:

    // thrown out of the loop above with a continue-op page installed: release it
    // and restore the one it displaced, which the caller's catch then releases.
    // Otherwise the displaced page is on no list and simply leaks.
    ಆದರೆ (displaced_ಪುಟ) {
        Release_Compilation_ಸಂಕೇತ_ಪುಟ(o);
        o->ಪುಟ = displaced_ಪುಟ;
    }

    m3_Free(ಗುರಿ_Stubs);

    ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}

ಸ್ಥಿರ
M3_ಫಲಿತಾಂಶ Compile_Call_ವಾದಗಳು_And_Return (IM3Compilation o, u16* o_ರಾಶಿ_ಸ್ಥಾನಾಂತರ, IM3FuncType i_type, bool i_is_ಪರೋಕ್ಷ)
{
_try {

    u16 ತುದಿ_ಸ್ಥಾನ = Get_Max_ಬಳಸಿದ_ಸ್ಥಾನ_Plus_One(o);

    // force use of at least one stack slot; this is to help ensure
    // the m3 stack overflows (and traps) before the native stack can overflow.
    // e.g. see Wasm spec test 'runaway' in call.wast
    ತುದಿ_ಸ್ಥಾನ = M3_MAX(1, ತುದಿ_ಸ್ಥಾನ);

    // stack frame is 64-bit aligned
    Align_ಸ್ಥಾನ_ಗಮ್ಯ_Type(&ತುದಿ_ಸ್ಥಾನ, c_m3Type_i64);

    *o_ರಾಶಿ_ಸ್ಥಾನಾಂತರ = ತುದಿ_ಸ್ಥಾನ;

    // wait to pop this here so that topSlot search is correct
    ಆದರೆ (i_is_ಪರೋಕ್ಷ) {
_       (ತೆಗೆ(o));
    }

    u16 ಅಂಕೆ_ವಾದಗಳು = Get_Func_Type_ಅಂಕೆ_Params(i_type);
    u16 ಅಂಕೆ_Rets = Get_Func_Type_ಅಂಕೆ_Results(i_type);

    u16 arg_ತುದಿ = ತುದಿ_ಸ್ಥಾನ + (ಅಂಕೆ_ವಾದಗಳು + ಅಂಕೆ_Rets) * c_io_ಸ್ಥಾನ_ಎಣಿಕೆ;

    Touch_ಸ್ಥಾನ(o, arg_ತುದಿ - 1);

    ಆಗ (ಅಂಕೆ_ವಾದಗಳು--) {
_       (Copy_ರಾಶಿ_ತುದಿ_ಗಮ್ಯ_ಸ್ಥಾನ(o, arg_ತುದಿ -= c_io_ಸ್ಥಾನ_ಎಣಿಕೆ));
_       (ತೆಗೆ(o));
    }

    u16 i = 0;
    ಆಗ (ಅಂಕೆ_Rets--) {
        m3type_t type = Get_Func_Type_ಫಲಿತಾಂಶ_Type(i_type, i++);

_       (ತಳ್ಳು(o, type, ತುದಿ_ಸ್ಥಾನ));
_       (MarkSlotsAllocatedByType(o, ತುದಿ_ಸ್ಥಾನ, type));

        ತುದಿ_ಸ್ಥಾನ += c_io_ಸ್ಥಾನ_ಎಣಿಕೆ;
    }

    } _catch: ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}

// A tail call hands the current frame over to the callee instead of stacking a new one.
// The callee's results have to be the enclosing function's results, so both use the same
// return slots and only the arguments move: they're staged above the caller's stack here
// (the argument expressions can still be reading the args/locals they're about to land on)
// and slid down onto the frame base at runtime, by op_ReturnCall[Indirect].
ಸ್ಥಿರ
M3_ಫಲಿತಾಂಶ Compile_ಬಾಲ_Call_ವಾದಗಳು (IM3Compilation o, u16* o_ರಾಶಿ_ಸ್ಥಾನಾಂತರ, u16* o_ಅಂಕೆ_Arg_Slots, IM3FuncType i_type, bool i_is_ಪರೋಕ್ಷ)
{
_try {
    IM3FuncType funcType = o->function->funcType;

    u16 ಅಂಕೆ_Rets = Get_Func_Type_ಅಂಕೆ_Results(i_type);
    _throwif(m3Err_typeMismatch, ಅಂಕೆ_Rets != Get_Func_Type_ಅಂಕೆ_Results(funcType));

    ಗಾಗಿ (u16 i = 0; i < ಅಂಕೆ_Rets; ++i) {
        _throwif(m3Err_typeMismatch, Get_Func_Type_ಫಲಿತಾಂಶ_Type(i_type, i) != Get_Func_Type_ಫಲಿತಾಂಶ_Type(funcType, i));
    }

    u16 ತುದಿ_ಸ್ಥಾನ = Get_Max_ಬಳಸಿದ_ಸ್ಥಾನ_Plus_One(o);

    ತುದಿ_ಸ್ಥಾನ = M3_MAX(1, ತುದಿ_ಸ್ಥಾನ);

    // stack frame is 64-bit aligned
    Align_ಸ್ಥಾನ_ಗಮ್ಯ_Type(&ತುದಿ_ಸ್ಥಾನ, c_m3Type_i64);

    *o_ರಾಶಿ_ಸ್ಥಾನಾಂತರ = ತುದಿ_ಸ್ಥಾನ;

    // wait to pop this here so that topSlot search is correct
    ಆದರೆ (i_is_ಪರೋಕ್ಷ) {
_       (ತೆಗೆ(o));
    }

    u16 ಅಂಕೆ_ವಾದಗಳು     = Get_Func_Type_ಅಂಕೆ_Params(i_type);
    u16 ಅಂಕೆ_Arg_Slots = ಅಂಕೆ_ವಾದಗಳು * c_io_ಸ್ಥಾನ_ಎಣಿಕೆ;

    *o_ಅಂಕೆ_Arg_Slots = ಅಂಕೆ_Arg_Slots;

    u16 arg_ತುದಿ = ತುದಿ_ಸ್ಥಾನ + ಅಂಕೆ_Arg_Slots;
    _throwif(m3_Err_function_ರಾಶಿ_ಅತಿಪ್ರವಾಹ, arg_ತುದಿ > d_m3MaxFunctionSlots);

    // the staging area is written by the caller, so op_Entry has to account for it
    Touch_ಸ್ಥಾನ(o, arg_ತುದಿ - 1);

    ಆಗ (ಅಂಕೆ_ವಾದಗಳು--) {
_       (Copy_ರಾಶಿ_ತುದಿ_ಗಮ್ಯ_ಸ್ಥಾನ(o, arg_ತುದಿ -= c_io_ಸ್ಥಾನ_ಎಣಿಕೆ));
_       (ತೆಗೆ(o));
    }

    } _catch: ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}


#ಆದರೆ d_m3HasTypedRefs

// call_ref $t: the callee is the reference on top of the stack, so the operand
// order is the same as call_indirect's, minus the table.
ಸ್ಥಿರ
M3_ಫಲಿತಾಂಶ Compile_CallRef (IM3Compilation o, m3opcode_t i_opcode)
{
_try {
    u32 type_ಸೂಚ್ಯಂಕ;
_   (ReadLEB_u32(&type_ಸೂಚ್ಯಂಕ, &o->wasm, o->wasm_ಕೊನೆ));

    _throwif("function call type index out of range", type_ಸೂಚ್ಯಂಕ >= o->module->ಅಂಕೆ_Func_Types);

    IM3FuncType type = o->module->funcTypes[type_ಸೂಚ್ಯಂಕ];

    ಆದರೆ (not Is_ರಾಶಿ_Polymorphic(o)) {
        // whatever shape the reference has, it must be a function of this type
        m3type_t refType = Get_ರಾಶಿ_ತುದಿ_Type(o);
        _throwif(m3Err_typeMismatch, not Is_Sub_Type_ದ(refType, Ref_Type_ದ_Func_Type(type, false)));
    }

    ಆದರೆ (Is_ರಾಶಿ_ತುದಿ_ಒಳ_ರಿಜಿಸ್ಟರ್(o)) {
_       (Preserve_ರಿಜಿಸ್ಟರ್_If_Occupied(o, c_m3Type_funcref));
    }

    u16 function_ಸ್ಥಾನ = Get_ರಾಶಿ_ತುದಿ_ಸ್ಥಾನ_ಸಂಖ್ಯೆ(o);

    bool isReturnCall = (i_opcode == c_waOp_returnCallRef);
    bool use_ಬಾಲ_Call  = isReturnCall and d_m3_Can_ಬಾಲ_Call;

    u16 exec_ತುದಿ, ಅಂಕೆ_Arg_Slots = 0;

    ಆದರೆ (use_ಬಾಲ_Call) {
_       (Compile_ಬಾಲ_Call_ವಾದಗಳು(o, &exec_ತುದಿ, &ಅಂಕೆ_Arg_Slots, type, true));
    } ಇಲ್ಲದಿದ್ದರೆ {
_       (Compile_Call_ವಾದಗಳು_And_Return(o, &exec_ತುದಿ, type, true));
    }

#  ಆದರೆ d_m3HasExceptionHandling
    ಆದರೆ (isReturnCall) {
_       (Emit_ತೆಗೆ_Try_ಚೌಕಟ್ಟುಗಳು_For_Return_Call(o));
    }
#  endif

_   (EmitOp(o, use_ಬಾಲ_Call ? op_ReturnCallRef : op_CallRef));
    Emit_ಸ್ಥಾನ_ಸ್ಥಾನಾಂತರ(o, function_ಸ್ಥಾನ);
    Emit_ಸೂಚಕ(o, type);
    Emit_ಸ್ಥಾನ_ಸ್ಥಾನಾಂತರ(o, exec_ತುದಿ);

    ಆದರೆ (not use_ಬಾಲ_Call) {
_       (ದಾಖಲೆ_Call(o, Get_Func_Type_ಅಂಕೆ_Results(type), exec_ತುದಿ, isReturnCall));
    }

    ಆದರೆ (use_ಬಾಲ_Call) {
        Emit_ಸ್ಥಾನ_ಸ್ಥಾನಾಂತರ(o, o->function->ಅಂಕೆ_Ret_Slots);
        EmitConstant32(o, ಅಂಕೆ_Arg_Slots);

_       (ಗಣ_ರಾಶಿ_Polymorphic(o));
    } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (isReturnCall) {
_       (Compile_Return(o, i_opcode));
    }

} _catch:
    ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}


// ref.as_non_null only sharpens the type; the value is unchanged, so the stack
// entry is re-pushed with the null stripped out of its type.
ಸ್ಥಿರ
M3_ಫಲಿತಾಂಶ Compile_Ref_AsNonNull (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_ಫಲಿತಾಂಶ ಫಲಿತಾಂಶ = m3Err_none;

    ಆದರೆ (not Is_ರಾಶಿ_Polymorphic(o)) {
        m3type_t type = Get_ರಾಶಿ_ತುದಿ_Type(o);
        _throwif(m3Err_typeMismatch, not IsRefType(type));

        // op_RefAsNonNull only knows how to read its operand out of a slot; a
        // value still sitting in the register has to be spilled first, the way
        // every other single-op reader of the stack top does
        ಆದರೆ (Is_ರಾಶಿ_ತುದಿ_ಒಳ_ರಿಜಿಸ್ಟರ್(o)) {
_           (Preserve_ರಿಜಿಸ್ಟರ್_If_Occupied(o, type));
        }

        u16 ಸ್ಥಾನ = Get_ರಾಶಿ_ತುದಿ_ಸ್ಥಾನ_ಸಂಖ್ಯೆ(o);

_       (EmitOp(o, op_RefAsNonNull));
        Emit_ಸ್ಥಾನ_ಸ್ಥಾನಾಂತರ(o, ಸ್ಥಾನ);

        m3type_t nonNull = IsSpelledRefType(type)
                             ? (m3type_t)(type | d_m3Type_refNonNull)
                             : (m3type_t)(d_m3Type_ref | d_m3Type_refNonNull | d_m3_Type_ರಾಶಿ_Abstract |
                                          ((ತಳ_Type_ದ(type) == c_m3Type_externref) ? d_m3Type_refExtern : 0));

        // the slot keeps the one reference it already had; a Pop()/Push() of
        // the same slot would drop that reference's count without anything
        // to put it back, so the type above it is corrected in place instead
        o->type_ರಾಶಿ[Get_ರಾಶಿ_ತುದಿ_ಸೂಚ್ಯಂಕ(o)] = nonNull;
    }

    _catch: ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}

#ಆದರೆ_ಅಂತ್ಯ // d_m3HasTypedRefs


// How wide a table operand is read at comes from the table it names, so a slot
// holding a narrower type would be read past what was allocated for it. The
// validator refuses that already; this keeps a build without one from reaching
// the interpreter with it. c_m3Type_none means the operand is not on the
// dynamic stack to inspect, which is not this check's business.
ಸ್ಥಿರ
M3_ಫಲಿತಾಂಶ Check_ಕಾರ್ಯಾಂಶ_Type (IM3Compilation o, u16 i_ಆಳ_From_ತುದಿ, m3type_t i_type)
{
    m3type_t actual;

    ಆದರೆ (Is_ರಾಶಿ_Polymorphic(o)) {
        ಹಿಂತಿರುಗಿ m3Err_none;
    }

    actual = Get_ರಾಶಿ_Type_From_ತುದಿ(o, i_ಆಳ_From_ತುದಿ);

    ಹಿಂತಿರುಗಿ (actual == c_m3Type_none or ತಳ_Type_ದ(actual) == i_type) ? m3Err_none
                                                                     : m3Err_typeMismatch;
}


// Swaps the _mem register onto a given memory. Only ever emitted in pairs --
// see op_SetMemory.
ಸ್ಥಿರ
M3_ಫಲಿತಾಂಶ Emit_ಗಣ_ಸ್ಮರಣೆ_Ptr (IM3Compilation o, IM3_ಸ್ಮರಣೆ i_ಸ್ಮರಣೆ)
{
    M3_ಫಲಿತಾಂಶ ಫಲಿತಾಂಶ = m3Err_none;

_   (EmitOp(o, op_ಗಣ_ಸ್ಮರಣೆ));
    Emit_ಸೂಚಕ(o, i_ಸ್ಮರಣೆ);

    _catch: ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}


// ...onto one of the compiling module's own memories, by index. Only reached
// with a non-zero index under multi-memory, but the index has already been
// bounds-checked by ReadMemoryIndex either way.
ಸ್ಥಿರ ಸಾಲೊಳಗೆ
M3_ಫಲಿತಾಂಶ Emit_ಗಣ_ಸ್ಮರಣೆ (IM3Compilation o, u32 i_ಸ್ಮರಣೆ_Idx)
{
#ಆದರೆ d_m3_Has_Multi_ಸ್ಮರಣೆ
    ಹಿಂತಿರುಗಿ Emit_ಗಣ_ಸ್ಮರಣೆ_Ptr(o, o->module->memories[i_ಸ್ಮರಣೆ_Idx]);
#ಇಲ್ಲದಿದ್ದರೆ
    (ಶೂನ್ಯ)o;
    (ಶೂನ್ಯ)i_ಸ್ಮರಣೆ_Idx;
    ಹಿಂತಿರುಗಿ m3_Err_unknown_ಸ್ಮರಣೆ;
#ಆದರೆ_ಅಂತ್ಯ
}

ಸ್ಥಿರ
M3_ಫಲಿತಾಂಶ Compile_Call (IM3Compilation o, m3opcode_t i_opcode)
{
_try {
    u32 function_ಸೂಚ್ಯಂಕ;
_   (ReadLEB_u32(&function_ಸೂಚ್ಯಂಕ, &o->wasm, o->wasm_ಕೊನೆ));

    IM3Function function = Module_GetFunction(o->module, function_ಸೂಚ್ಯಂಕ);

    ಆದರೆ (function) {                                                     m3log (compile, d_indent " (func= [%d] '%s'; args= %d)",
                                                                                get_indention_string (o), function_ಸೂಚ್ಯಂಕ, m3_GetFunctionName (function), function->funcType->ಅಂಕೆ_ವಾದಗಳು);
        // an import linked to another module's export runs that module's body
        IM3Function ಗುರಿ = Function_Implementation(function);

        ಆದರೆ (ಗುರಿ->module) {
            bool isReturnCall = (i_opcode == c_waOp_returnCall);

            // The callee's body runs against its own module's memory, so a call
            // that crosses modules is bracketed by a pair of op_SetMemory. A
            // tail call never returns to run the second one, so those compile
            // as a plain call followed by a return instead.
            bool crossModule = (ಗುರಿ->module != o->module);
            bool use_ಬಾಲ_Call = isReturnCall and d_m3_Can_ಬಾಲ_Call and not crossModule;

            u16 ಸ್ಥಾನ_ತುದಿ, ಅಂಕೆ_Arg_Slots = 0;

            ಆದರೆ (use_ಬಾಲ_Call) {
_               (Compile_ಬಾಲ_Call_ವಾದಗಳು(o, &ಸ್ಥಾನ_ತುದಿ, &ಅಂಕೆ_Arg_Slots, ಗುರಿ->funcType, false));
            } ಇಲ್ಲದಿದ್ದರೆ {
_               (Compile_Call_ವಾದಗಳು_And_Return(o, &ಸ್ಥಾನ_ತುದಿ, ಗುರಿ->funcType, false));
            }

            IM3Operation op;
            ಸ್ಥಿರಾಂಕ ಶೂನ್ಯ*  ಕಾರ್ಯಾಂಶ;

            ಆದರೆ (ಗುರಿ->compiled) {
                op      = use_ಬಾಲ_Call ? op_ReturnCall : op_Call;
                ಕಾರ್ಯಾಂಶ = ಗುರಿ->compiled;
            } ಇಲ್ಲದಿದ್ದರೆ {
                op      = use_ಬಾಲ_Call ? op_CompileReturnCall : op_Compile;
                ಕಾರ್ಯಾಂಶ = ಗುರಿ;
            }

#ಆದರೆ d_m3HasExceptionHandling
            ಆದರೆ (isReturnCall) {
_               (Emit_ತೆಗೆ_Try_ಚೌಕಟ್ಟುಗಳು_For_Return_Call(o));
            }
#ಆದರೆ_ಅಂತ್ಯ

            ಆದರೆ (crossModule) {
_               (Emit_ಗಣ_ಸ್ಮರಣೆ_Ptr(o, Module_Memory0(ಗುರಿ->module)));
            }

_           (EmitOp(o, op));
            Emit_ಸೂಚಕ(o, ಕಾರ್ಯಾಂಶ);
            Emit_ಸ್ಥಾನ_ಸ್ಥಾನಾಂತರ(o, ಸ್ಥಾನ_ತುದಿ);

            ಆದರೆ (not use_ಬಾಲ_Call) {
                // a suspend inside the callee leaves this frame waiting here, its
                // results not yet written
_               (ದಾಖಲೆ_Call(o, Get_Func_Type_ಅಂಕೆ_Results(ಗುರಿ->funcType), ಸ್ಥಾನ_ತುದಿ, isReturnCall));
            }

            ಆದರೆ (use_ಬಾಲ_Call) {
                Emit_ಸ್ಥಾನ_ಸ್ಥಾನಾಂತರ(o, o->function->ಅಂಕೆ_Ret_Slots);
                EmitConstant32(o, ಅಂಕೆ_Arg_Slots);

_               (ಗಣ_ರಾಶಿ_Polymorphic(o));
            } ಇಲ್ಲದಿದ್ದರೆ {
                ಆದರೆ (crossModule) {
_                   (Emit_ಗಣ_ಸ್ಮರಣೆ_Ptr(o, Module_Memory0(o->module)));
                }

                ಆದರೆ (isReturnCall) {
_                   (Compile_Return(o, i_opcode));
                }
            }
        } ಇಲ್ಲದಿದ್ದರೆ {
            _throw(ErrorCompile(m3Err_functionImportMissing, o, "'%s.%s'", GetFunctionImportModuleName(function), m3_GetFunctionName(function)));
        }
    } ಇಲ್ಲದಿದ್ದರೆ {
        _throw(ErrorCompile(m3Err_functionLookupFailed, o, "index: %d", function_ಸೂಚ್ಯಂಕ));
    }

    } _catch: ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}

ಸ್ಥಿರ
M3_ಫಲಿತಾಂಶ Compile_Call_ಪರೋಕ್ಷ (IM3Compilation o, m3opcode_t i_opcode)
{
_try {
    u32 type_ಸೂಚ್ಯಂಕ;
_   (ReadLEB_u32(&type_ಸೂಚ್ಯಂಕ, &o->wasm, o->wasm_ಕೊನೆ));

    u32 ಕೋಷ್ಟಕ_ಸೂಚ್ಯಂಕ;
_   (ReadLEB_u32(&ಕೋಷ್ಟಕ_ಸೂಚ್ಯಂಕ, &o->wasm, o->wasm_ಕೊನೆ));

    _throwif("function call type index out of range", type_ಸೂಚ್ಯಂಕ >= o->module->ಅಂಕೆ_Func_Types);
    _throwif("table index out of range", ಕೋಷ್ಟಕ_ಸೂಚ್ಯಂಕ >= o->module->ಅಂಕೆ_Tables);
    _throwif(m3Err_typeMismatch, ತಳ_Type_ದ(o->module->tables[ಕೋಷ್ಟಕ_ಸೂಚ್ಯಂಕ]->type) != c_m3Type_funcref);

_   (Check_ಕಾರ್ಯಾಂಶ_Type(o, 0, ಕೋಷ್ಟಕ_Addr_Type(o->module->tables[ಕೋಷ್ಟಕ_ಸೂಚ್ಯಂಕ])));

    ಆದರೆ (Is_ರಾಶಿ_ತುದಿ_ಒಳ_ರಿಜಿಸ್ಟರ್(o)) {
_       (Preserve_ರಿಜಿಸ್ಟರ್_If_Occupied(o, c_m3Type_i32));
    }

    u16 ಕೋಷ್ಟಕ_ಸೂಚ್ಯಂಕ_ಸ್ಥಾನ = Get_ರಾಶಿ_ತುದಿ_ಸ್ಥಾನ_ಸಂಖ್ಯೆ(o);

    bool isReturnCall = (i_opcode == c_wa_Op_return_Call_ಪರೋಕ್ಷ);
    bool use_ಬಾಲ_Call  = isReturnCall and d_m3_Can_ಬಾಲ_Call;

    u16         exec_ತುದಿ, ಅಂಕೆ_Arg_Slots = 0;
    IM3FuncType type = o->module->funcTypes[type_ಸೂಚ್ಯಂಕ];

    ಆದರೆ (use_ಬಾಲ_Call) {
_       (Compile_ಬಾಲ_Call_ವಾದಗಳು(o, &exec_ತುದಿ, &ಅಂಕೆ_Arg_Slots, type, true));
    } ಇಲ್ಲದಿದ್ದರೆ {
_       (Compile_Call_ವಾದಗಳು_And_Return(o, &exec_ತುದಿ, type, true));
    }

#ಆದರೆ d_m3HasExceptionHandling
    ಆದರೆ (isReturnCall) {
_       (Emit_ತೆಗೆ_Try_ಚೌಕಟ್ಟುಗಳು_For_Return_Call(o));
    }
#ಆದರೆ_ಅಂತ್ಯ

    // the table comes first: how wide its indexes are is what says how much of
    // the slot below holds one
_   (EmitOp(o, use_ಬಾಲ_Call ? op_Return_Call_ಪರೋಕ್ಷ : op_Call_ಪರೋಕ್ಷ));
    Emit_ಸೂಚಕ(o, o->module->tables[ಕೋಷ್ಟಕ_ಸೂಚ್ಯಂಕ]);
    Emit_ಸ್ಥಾನ_ಸ್ಥಾನಾಂತರ(o, ಕೋಷ್ಟಕ_ಸೂಚ್ಯಂಕ_ಸ್ಥಾನ);
    Emit_ಸೂಚಕ(o, type);
    Emit_ಸ್ಥಾನ_ಸ್ಥಾನಾಂತರ(o, exec_ತುದಿ);

    ಆದರೆ (not use_ಬಾಲ_Call) {
_       (ದಾಖಲೆ_Call(o, Get_Func_Type_ಅಂಕೆ_Results(type), exec_ತುದಿ, isReturnCall));
    }

    ಆದರೆ (use_ಬಾಲ_Call) {
        Emit_ಸ್ಥಾನ_ಸ್ಥಾನಾಂತರ(o, o->function->ಅಂಕೆ_Ret_Slots);
        EmitConstant32(o, ಅಂಕೆ_Arg_Slots);

_       (ಗಣ_ರಾಶಿ_Polymorphic(o));
    } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (isReturnCall) {
_       (Compile_Return(o, i_opcode));
    }

} _catch:
    ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}

// Reads the memory index a memory instruction names, and checks it against the
// module's index space. Without multi-memory the immediate is a reserved byte
// that has to be zero, which is the same check.
ಸ್ಥಿರ
M3_ಫಲಿತಾಂಶ Read_ಸ್ಮರಣೆ_ಸೂಚ್ಯಂಕ (IM3Compilation o, u32* o_ಸ್ಮರಣೆ_Idx)
{
    M3_ಫಲಿತಾಂಶ ಫಲಿತಾಂಶ;

_   (ReadLEB_u32(o_ಸ್ಮರಣೆ_Idx, &o->wasm, o->wasm_ಕೊನೆ));

    _throwif(m3_Err_unknown_ಸ್ಮರಣೆ, *o_ಸ್ಮರಣೆ_Idx >= o->module->ಅಂಕೆ_Memories);

    _catch: ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}

ಸ್ಥಿರ
M3_ಫಲಿತಾಂಶ Compile_ಸ್ಮರಣೆ_ಗಾತ್ರ (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_ಫಲಿತಾಂಶ ಫಲಿತಾಂಶ;

    u32 ಸ್ಮರಣೆ_Idx;

    // A page count is given in the memory's own address type. Declared up here
    // so the throws below don't jump over its initialization.
    m3type_t addrType = c_m3Type_i32;

_   (Read_ಸ್ಮರಣೆ_ಸೂಚ್ಯಂಕ(o, &ಸ್ಮರಣೆ_Idx));

    addrType = ಸ್ಮರಣೆ_Addr_Type(o->module->memories[ಸ್ಮರಣೆ_Idx]);

_   (Preserve_ರಿಜಿಸ್ಟರ್_If_Occupied(o, addrType));

    ಆದರೆ (ಸ್ಮರಣೆ_Idx) {
_       (Emit_ಗಣ_ಸ್ಮರಣೆ(o, ಸ್ಮರಣೆ_Idx));
    }

    // op_MemSize writes the whole register either way; the slot it is pushed
    // to is what decides how much of it the module gets to see
_   (EmitOp(o, op_Mem_ಗಾತ್ರ));

    ಆದರೆ (ಸ್ಮರಣೆ_Idx) {
_       (Emit_ಗಣ_ಸ್ಮರಣೆ(o, 0));
    }

_   (ತಳ್ಳು_ರಿಜಿಸ್ಟರ್(o, addrType));

    _catch: ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}

ಸ್ಥಿರ
M3_ಫಲಿತಾಂಶ Compile_ಸ್ಮರಣೆ_Grow (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_ಫಲಿತಾಂಶ ಫಲಿತಾಂಶ;

    u32 ಸ್ಮರಣೆ_Idx;

    // see Compile_Memory_Size
    m3type_t addrType = c_m3Type_i32;

_   (Read_ಸ್ಮರಣೆ_ಸೂಚ್ಯಂಕ(o, &ಸ್ಮರಣೆ_Idx));

    addrType = ಸ್ಮರಣೆ_Addr_Type(o->module->memories[ಸ್ಮರಣೆ_Idx]);

_   (Copy_ರಾಶಿ_ತುದಿ_ಗಮ್ಯ_ರಿಜಿಸ್ಟರ್(o, false));
_   (ತೆಗೆ_Type(o, addrType));

    ಆದರೆ (ಸ್ಮರಣೆ_Idx) {
_       (Emit_ಗಣ_ಸ್ಮರಣೆ(o, ಸ್ಮರಣೆ_Idx));
    }

#ಆದರೆ d_m3HasMemory64
_   (EmitOp(o, (addrType == c_m3Type_i64) ? op_MemGrow64 : op_MemGrow));
#ಇಲ್ಲದಿದ್ದರೆ
_   (EmitOp(o, op_MemGrow));
#ಆದರೆ_ಅಂತ್ಯ

    ಆದರೆ (ಸ್ಮರಣೆ_Idx) {
_       (Emit_ಗಣ_ಸ್ಮರಣೆ(o, 0));
    }

_   (ತಳ್ಳು_ರಿಜಿಸ್ಟರ್(o, addrType));

    _catch: ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}

ಸ್ಥಿರ
M3_ಫಲಿತಾಂಶ Compile_ಸ್ಮರಣೆ_Copy_Fill (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_ಫಲಿತಾಂಶ ಫಲಿತಾಂಶ = m3Err_none;

    // memory.copy names the destination first and then the source; memory.fill
    // names only the one it writes.
    u32  ಮೂಲ_ಸ್ಮರಣೆ_Idx = 0, ಗುರಿ_ಸ್ಮರಣೆ_Idx = 0;
    bool isCopy = (i_opcode == c_wa_Op_ಸ್ಮರಣೆ_Copy);

    // Each address is typed by the memory it belongs to. The length follows
    // the narrower of the two, so it is 64-bit only when both are - which for
    // memory.fill, naming one memory twice over, means whenever that one is.
    // Declared up here so the throws below don't jump over them.
    m3type_t ಗುರಿ_Type = c_m3Type_i32;
    m3type_t ಮೂಲ_Type = c_m3Type_i32;
    m3type_t ಉದ್ದಳತೆ_Type = c_m3Type_i32;

    _throwif(m3Err_wasmMalformed, o->module->ಅಂಕೆ_Memories == 0);

_   (Read_ಸ್ಮರಣೆ_ಸೂಚ್ಯಂಕ(o, &ಗುರಿ_ಸ್ಮರಣೆ_Idx));

    ಆದರೆ (isCopy) {
_       (Read_ಸ್ಮರಣೆ_ಸೂಚ್ಯಂಕ(o, &ಮೂಲ_ಸ್ಮರಣೆ_Idx));
    }

    ಗುರಿ_Type = ಸ್ಮರಣೆ_Addr_Type(o->module->memories[ಗುರಿ_ಸ್ಮರಣೆ_Idx]);
    ಮೂಲ_Type = isCopy ? ಸ್ಮರಣೆ_Addr_Type(o->module->memories[ಮೂಲ_ಸ್ಮರಣೆ_Idx])
                        : ಗುರಿ_Type;
    ಉದ್ದಳತೆ_Type = (ಗುರಿ_Type == c_m3Type_i64 and ಮೂಲ_Type == c_m3Type_i64)
                   ? c_m3Type_i64
                   : c_m3Type_i32;

_   (Copy_ರಾಶಿ_ತುದಿ_ಗಮ್ಯ_ರಿಜಿಸ್ಟರ್(o, false));

#ಆದರೆ d_m3_Has_Multi_ಸ್ಮರಣೆ
    ಆದರೆ (isCopy and ಮೂಲ_ಸ್ಮರಣೆ_Idx != ಗುರಿ_ಸ್ಮರಣೆ_Idx) {
        // two memories at once: _mem can only name one, so the op takes both
_       (EmitOp(o, op_MemCopy_x));
        Emit_ಸೂಚಕ(o, o->module->memories[ಗುರಿ_ಸ್ಮರಣೆ_Idx]);
        Emit_ಸೂಚಕ(o, o->module->memories[ಮೂಲ_ಸ್ಮರಣೆ_Idx]);

        // ...and, since they need not be addressed alike, how wide to read
        // each of the operands: bit 0 the destination, bit 1 the source
        EmitConstant32(o, ((ಗುರಿ_Type == c_m3Type_i64) ? 0x1u : 0u) |
                            ((ಮೂಲ_Type == c_m3Type_i64) ? 0x2u : 0u));
    } ಇಲ್ಲದಿದ್ದರೆ
#ಆದರೆ_ಅಂತ್ಯ
    {
        IM3Operation op = isCopy ? op_MemCopy : op_MemFill;

#ಆದರೆ d_m3HasMemory64
        ಆದರೆ (ಗುರಿ_Type == c_m3Type_i64) {
            op = isCopy ? op_MemCopy64 : op_MemFill64;
        }
#ಆದರೆ_ಅಂತ್ಯ

        ಆದರೆ (ಗುರಿ_ಸ್ಮರಣೆ_Idx) {
_           (Emit_ಗಣ_ಸ್ಮರಣೆ(o, ಗುರಿ_ಸ್ಮರಣೆ_Idx));
        }

_       (EmitOp(o, op));
    }

    // the length is in the register; the other two are named by slot, at
    // whatever width their stack entries were pushed with
_   (ತೆಗೆ_Type(o, ಉದ್ದಳತೆ_Type));
_   (Emit_ಸ್ಥಾನ_ಅಂಕೆ_ದ_ರಾಶಿ_ತುದಿ_And_ತೆಗೆ(o));      // the source, or the byte to fill with
_   (Emit_ಸ್ಥಾನ_ಅಂಕೆ_ದ_ರಾಶಿ_ತುದಿ_And_ತೆಗೆ(o));      // the destination

    ಆದರೆ (ಗುರಿ_ಸ್ಮರಣೆ_Idx and not (isCopy and ಮೂಲ_ಸ್ಮರಣೆ_Idx != ಗುರಿ_ಸ್ಮರಣೆ_Idx)) {
_       (Emit_ಗಣ_ಸ್ಮರಣೆ(o, 0));
    }

    _catch: ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}


// memory.init and data.drop address a data segment by index. Both are only valid
// when a data count section declared the segments up front.
ಸ್ಥಿರ
M3_ಫಲಿತಾಂಶ Read_ದತ್ತಾಂಶ_Segment (IM3Compilation o, M3_ದತ್ತಾಂಶ_Segment** o_segment)
{
    M3_ಫಲಿತಾಂಶ ಫಲಿತಾಂಶ = m3Err_none;

    u32 index;
_   (ReadLEB_u32(&index, &o->wasm, o->wasm_ಕೊನೆ));

    _throwif("data count section required", not o->module->has_ದತ್ತಾಂಶ_ಎಣಿಕೆ);
    _throwif(m3Err_wasmMalformed, index >= o->module->ಅಂಕೆ_ದತ್ತಾಂಶ_Segments);

    *o_segment = &o->module->ದತ್ತಾಂಶ_Segments[index];

    _catch: ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}


M3_ಫಲಿತಾಂಶ Compile_ಸ್ಮರಣೆ_Init (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_ಫಲಿತಾಂಶ ಫಲಿತಾಂಶ = m3Err_none;

    M3_ದತ್ತಾಂಶ_Segment* segment = ಖಾಲಿ;
    u32            ಸ್ಮರಣೆ_Idx;

    _throwif(m3Err_wasmMalformed, o->module->ಅಂಕೆ_Memories == 0);

_   (Read_ದತ್ತಾಂಶ_Segment(o, &segment));
_   (Read_ಸ್ಮರಣೆ_ಸೂಚ್ಯಂಕ(o, &ಸ್ಮರಣೆ_Idx));

_   (Copy_ರಾಶಿ_ತುದಿ_ಗಮ್ಯ_ರಿಜಿಸ್ಟರ್(o, false));

    ಆದರೆ (ಸ್ಮರಣೆ_Idx) {
_       (Emit_ಗಣ_ಸ್ಮರಣೆ(o, ಸ್ಮರಣೆ_Idx));
    }

#ಆದರೆ d_m3HasMemory64
    // only the destination follows the memory's address type: a data segment
    // is indexed as an i32 however the memory it is copied into is addressed
_   (EmitOp(o, o->module->memories[ಸ್ಮರಣೆ_Idx]->isMemory64 ? op_MemInit64 : op_MemInit));
#ಇಲ್ಲದಿದ್ದರೆ
_   (EmitOp(o, op_MemInit));
#ಆದರೆ_ಅಂತ್ಯ
    Emit_ಸೂಚಕ(o, segment);
_   (ತೆಗೆ_Type(o, c_m3Type_i32));
_   (Emit_ಸ್ಥಾನ_ಅಂಕೆ_ದ_ರಾಶಿ_ತುದಿ_And_ತೆಗೆ(o));
_   (Emit_ಸ್ಥಾನ_ಅಂಕೆ_ದ_ರಾಶಿ_ತುದಿ_And_ತೆಗೆ(o));

    ಆದರೆ (ಸ್ಮರಣೆ_Idx) {
_       (Emit_ಗಣ_ಸ್ಮರಣೆ(o, 0));
    }

    _catch: ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}


#ಆದರೆ d_m3HasRefTypes

ಸ್ಥಿರ
M3_ಫಲಿತಾಂಶ Compile_Select (IM3Compilation o, m3opcode_t i_opcode);

// select with an explicit result type vector. The types only matter to the
// validator; the operands are laid out exactly as for the untyped select.
M3_ಫಲಿತಾಂಶ Compile_Select_Typed (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_ಫಲಿತಾಂಶ ಫಲಿತಾಂಶ = m3Err_none;

    u32 ಅಂಕೆ_Types;
_   (ReadLEB_u32(&ಅಂಕೆ_Types, &o->wasm, o->wasm_ಕೊನೆ));
    _throwif(m3Err_wasmMalformed, ಅಂಕೆ_Types != 1);

    i8 waType;
    u8 type;
_   (ReadLEB_i7(&waType, &o->wasm, o->wasm_ಕೊನೆ));
_   (NormalizeType(&type, waType));

_   (Compile_Select(o, i_opcode));

    _catch: ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}


// A reference is one pointer-sized word and null is 0, so null-testing it is
// just an integer eqz of the matching width.
#  ಆದರೆ M3_SIZEOF_PTR == 8
#    define d_m3RefIsNull_r   op_i64_Equal_ಗಮ್ಯ_Zero_r
#    define d_m3RefIsNull_s   op_i64_Equal_ಗಮ್ಯ_Zero_s
#  ಇಲ್ಲದಿದ್ದರೆ
#    define d_m3RefIsNull_r   op_i32_Equal_ಗಮ್ಯ_Zero_r
#    define d_m3RefIsNull_s   op_i32_Equal_ಗಮ್ಯ_Zero_s
#  endif

ಸ್ಥಿರ
M3_ಫಲಿತಾಂಶ ReadRefType (IM3Compilation o, m3type_t* o_type)
{
    M3_ಫಲಿತಾಂಶ ಫಲಿತಾಂಶ = m3Err_none;

#  ಆದರೆ d_m3HasTypedRefs
    // ref.null names a heap type: func, extern, or a function type index
    m3type_t ರಾಶಿ_ಬಿಟ್ಟುಗಳು;
_   (Parse_ರಾಶಿ_Type(o->module, &ರಾಶಿ_ಬಿಟ್ಟುಗಳು, &o->wasm, o->wasm_ಕೊನೆ));

    *o_type = d_m3Type_ref | ರಾಶಿ_ಬಿಟ್ಟುಗಳು;
#  ಇಲ್ಲದಿದ್ದರೆ
    i8 waType;
    u8 plainType;
_   (ReadLEB_i7(&waType, &o->wasm, o->wasm_ಕೊನೆ));
_   (NormalizeType(&plainType, waType));
    *o_type = plainType;
    _throwif(m3Err_wasmMalformed, not IsRefType(*o_type));
#  endif

    _catch: ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}


M3_ಫಲಿತಾಂಶ Compile_Ref_Null (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_ಫಲಿತಾಂಶ ಫಲಿತಾಂಶ = m3Err_none;

    m3type_t type;
_   (ReadRefType(o, &type));
_   (ತಳ್ಳು_Const(o, 0, type));

    _catch: ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}


M3_ಫಲಿತಾಂಶ Compile_Ref_IsNull (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_ಫಲಿತಾಂಶ ಫಲಿತಾಂಶ = m3Err_none;

    IM3Operation op;

    ಆದರೆ (not Is_ರಾಶಿ_Polymorphic(o)) {
        _throwif(m3Err_typeMismatch, not IsRefType(Get_ರಾಶಿ_ತುದಿ_Type(o)));
    }

    ಆದರೆ (Is_ರಾಶಿ_ತುದಿ_ಒಳ_ರಿಜಿಸ್ಟರ್(o)) {
        op = d_m3RefIsNull_r;
    } ಇಲ್ಲದಿದ್ದರೆ {
_       (Preserve_ರಿಜಿಸ್ಟರ್_If_Occupied(o, c_m3Type_i32));
        op = d_m3RefIsNull_s;
    }

_   (EmitOp(o, op));
_   (Emit_ಸ್ಥಾನ_ಅಂಕೆ_ದ_ರಾಶಿ_ತುದಿ_And_ತೆಗೆ(o));
_   (ತಳ್ಳು_ರಿಜಿಸ್ಟರ್(o, c_m3Type_i32));

    _catch: ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}


M3_ಫಲಿತಾಂಶ Compile_Ref_Func (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_ಫಲಿತಾಂಶ ಫಲಿತಾಂಶ = m3Err_none;

    u32 func_ಸೂಚ್ಯಂಕ;

    // declared before the throws below, which jump past this point to _catch
    m3type_t    refType   = c_m3Type_funcref;
    IM3Function reference = ಖಾಲಿ;

_   (ReadLEB_u32(&func_ಸೂಚ್ಯಂಕ, &o->wasm, o->wasm_ಕೊನೆ));
    _throwif("function index out of range", func_ಸೂಚ್ಯಂಕ >= o->module->ಅಂಕೆ_Functions);

    // Inside a constant expression ref.func is itself a declaration; inside a
    // function body the function must already have been declared elsewhere.
    ಆದರೆ (o->function) {
        _throwif("undeclared function reference", not Module_IsFunctionDeclared(o->module, func_ಸೂಚ್ಯಂಕ));
    } ಇಲ್ಲದಿದ್ದರೆ {
_       (Module_DeclareFunction(o->module, func_ಸೂಚ್ಯಂಕ));
    }

#  ಆದರೆ d_m3HasTypedRefs
    refType = Ref_Type_ದ_Func_Type(o->module->functions[func_ಸೂಚ್ಯಂಕ].funcType, true);
#  endif

    // A reference denotes the function that actually runs, so a ref.func naming
    // an import linked to another module is the same reference that module's
    // own ref.func would produce - and carries the defining module with it.
    reference = Function_Implementation(&o->module->functions[func_ಸೂಚ್ಯಂಕ]);

_   (ತಳ್ಳು_Const(o, (u64)(uintptr_t)reference, refType));

    _catch: ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}


// table.get/set/size/grow/fill all name a table by index; the table struct goes
// into the codestream so the operation doesn't have to walk the module.
ಸ್ಥಿರ
M3_ಫಲಿತಾಂಶ Read_ಕೋಷ್ಟಕ (IM3Compilation o, M3_ಕೋಷ್ಟಕ** o_ಕೋಷ್ಟಕ)
{
    M3_ಫಲಿತಾಂಶ ಫಲಿತಾಂಶ = m3Err_none;

    u32 index;
_   (ReadLEB_u32(&index, &o->wasm, o->wasm_ಕೊನೆ));
    _throwif("table index out of range", index >= o->module->ಅಂಕೆ_Tables);

    *o_ಕೋಷ್ಟಕ = o->module->tables[index];

    _catch: ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}


// The table operations take every operand from a slot, so spill the integer
// register first: after this each EmitSlotNumOfStackTopAndPop () emits a real
// slot offset, and the register is free for the result. Operands are emitted
// from the top of the stack downwards, which is the order the ops read them.
ಸ್ಥಿರ
M3_ಫಲಿತಾಂಶ Compile_ಕೋಷ್ಟಕ_Op (IM3Compilation o, IM3Operation i_op, M3_ಕೋಷ್ಟಕ* i_ಕೋಷ್ಟಕ, u32 i_ಅಂಕೆ_Operands, m3type_t i_retType)
{
    M3_ಫಲಿತಾಂಶ ಫಲಿತಾಂಶ = m3Err_none;

_   (Preserve_ರಿಜಿಸ್ಟರ್_If_Occupied(o, c_m3Type_i64));

_   (EmitOp(o, i_op));
    Emit_ಸೂಚಕ(o, i_ಕೋಷ್ಟಕ);

    ಗಾಗಿ (u32 i = 0; i < i_ಅಂಕೆ_Operands; ++i) {
_       (Emit_ಸ್ಥಾನ_ಅಂಕೆ_ದ_ರಾಶಿ_ತುದಿ_And_ತೆಗೆ(o));
    }

    ಆದರೆ (i_retType != c_m3Type_none) {
_       (ತಳ್ಳು_ರಿಜಿಸ್ಟರ್(o, i_retType));
    }

    _catch: ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}


M3_ಫಲಿತಾಂಶ Compile_ಕೋಷ್ಟಕ_Get_ಗಣ (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_ಫಲಿತಾಂಶ ಫಲಿತಾಂಶ = m3Err_none;

    M3_ಕೋಷ್ಟಕ* ಕೋಷ್ಟಕ_2;
_   (Read_ಕೋಷ್ಟಕ(o, &ಕೋಷ್ಟಕ_2));

    ಆದರೆ (i_opcode == c_wa_Op_ಕೋಷ್ಟಕ_Get) {
_       (Check_ಕಾರ್ಯಾಂಶ_Type(o, 0, ಕೋಷ್ಟಕ_Addr_Type(ಕೋಷ್ಟಕ_2)));
_       (Compile_ಕೋಷ್ಟಕ_Op(o, op_ಕೋಷ್ಟಕ_Get, ಕೋಷ್ಟಕ_2, 1, ಕೋಷ್ಟಕ_2->type));
    } ಇಲ್ಲದಿದ್ದರೆ {
_       (Check_ಕಾರ್ಯಾಂಶ_Type(o, 1, ಕೋಷ್ಟಕ_Addr_Type(ಕೋಷ್ಟಕ_2)));
_       (Compile_ಕೋಷ್ಟಕ_Op(o, op_ಕೋಷ್ಟಕ_ಗಣ, ಕೋಷ್ಟಕ_2, 2, c_m3Type_none));
    }

    _catch: ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}


M3_ಫಲಿತಾಂಶ Compile_ಕೋಷ್ಟಕ_Init (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_ಫಲಿತಾಂಶ ಫಲಿತಾಂಶ = m3Err_none;

    u32      elem_ಸೂಚ್ಯಂಕ;
    M3_ಕೋಷ್ಟಕ* ಕೋಷ್ಟಕ_2;

_   (ReadLEB_u32(&elem_ಸೂಚ್ಯಂಕ, &o->wasm, o->wasm_ಕೊನೆ));
    _throwif("element segment index out of range", elem_ಸೂಚ್ಯಂಕ >= o->module->ಅಂಕೆ_Element_Segments);
_   (Read_ಕೋಷ್ಟಕ(o, &ಕೋಷ್ಟಕ_2));

_   (Check_ಕಾರ್ಯಾಂಶ_Type(o, 2, ಕೋಷ್ಟಕ_Addr_Type(ಕೋಷ್ಟಕ_2)));

_   (Preserve_ರಿಜಿಸ್ಟರ್_If_Occupied(o, c_m3Type_i64));
_   (EmitOp(o, op_ಕೋಷ್ಟಕ_Init));
    Emit_ಸೂಚಕ(o, ಕೋಷ್ಟಕ_2);
    Emit_ಸೂಚಕ(o, &o->module->elementSegments[elem_ಸೂಚ್ಯಂಕ]);

    ಗಾಗಿ (u32 i = 0; i < 3; ++i) {
_       (Emit_ಸ್ಥಾನ_ಅಂಕೆ_ದ_ರಾಶಿ_ತುದಿ_And_ತೆಗೆ(o));
    }

    _catch: ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}


M3_ಫಲಿತಾಂಶ Compile_Elem_Drop (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_ಫಲಿತಾಂಶ ಫಲಿತಾಂಶ = m3Err_none;

    u32 elem_ಸೂಚ್ಯಂಕ;
_   (ReadLEB_u32(&elem_ಸೂಚ್ಯಂಕ, &o->wasm, o->wasm_ಕೊನೆ));
    _throwif("element segment index out of range", elem_ಸೂಚ್ಯಂಕ >= o->module->ಅಂಕೆ_Element_Segments);

_   (EmitOp(o, op_ElemDrop));
    Emit_ಸೂಚಕ(o, &o->module->elementSegments[elem_ಸೂಚ್ಯಂಕ]);

    _catch: ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}


M3_ಫಲಿತಾಂಶ Compile_ಕೋಷ್ಟಕ_Copy (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_ಫಲಿತಾಂಶ ಫಲಿತಾಂಶ = m3Err_none;

    M3_ಕೋಷ್ಟಕ* dst;
    M3_ಕೋಷ್ಟಕ* src;

_   (Read_ಕೋಷ್ಟಕ(o, &dst));
_   (Read_ಕೋಷ್ಟಕ(o, &src));
    _throwif(m3Err_typeMismatch, not Is_Sub_Type_ದ(src->type, dst->type));

_   (Check_ಕಾರ್ಯಾಂಶ_Type(o, 2, ಕೋಷ್ಟಕ_Addr_Type(dst)));
_   (Check_ಕಾರ್ಯಾಂಶ_Type(o, 1, ಕೋಷ್ಟಕ_Addr_Type(src)));
_   (Check_ಕಾರ್ಯಾಂಶ_Type(o, 0, (dst->isTable64 and src->isTable64) ? c_m3Type_i64 : c_m3Type_i32));

_   (Preserve_ರಿಜಿಸ್ಟರ್_If_Occupied(o, c_m3Type_i64));
_   (EmitOp(o, op_ಕೋಷ್ಟಕ_Copy));
    Emit_ಸೂಚಕ(o, dst);
    Emit_ಸೂಚಕ(o, src);

    ಗಾಗಿ (u32 i = 0; i < 3; ++i) {
_       (Emit_ಸ್ಥಾನ_ಅಂಕೆ_ದ_ರಾಶಿ_ತುದಿ_And_ತೆಗೆ(o));
    }

    _catch: ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}


M3_ಫಲಿತಾಂಶ Compile_ಕೋಷ್ಟಕ_ಗಾತ್ರ (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_ಫಲಿತಾಂಶ ಫಲಿತಾಂಶ = m3Err_none;

    M3_ಕೋಷ್ಟಕ* ಕೋಷ್ಟಕ_2;
_   (Read_ಕೋಷ್ಟಕ(o, &ಕೋಷ್ಟಕ_2));

    // a table size is given in the table's own index type
_   (Compile_ಕೋಷ್ಟಕ_Op(o, op_ಕೋಷ್ಟಕ_ಗಾತ್ರ, ಕೋಷ್ಟಕ_2, 0, ಕೋಷ್ಟಕ_Addr_Type(ಕೋಷ್ಟಕ_2)));

    _catch: ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}


M3_ಫಲಿತಾಂಶ Compile_ಕೋಷ್ಟಕ_Grow_Fill (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_ಫಲಿತಾಂಶ ಫಲಿತಾಂಶ = m3Err_none;

    M3_ಕೋಷ್ಟಕ* ಕೋಷ್ಟಕ_2;
_   (Read_ಕೋಷ್ಟಕ(o, &ಕೋಷ್ಟಕ_2));

    ಆದರೆ (i_opcode == c_wa_Op_ಕೋಷ್ಟಕ_Grow) {
_       (Check_ಕಾರ್ಯಾಂಶ_Type(o, 0, ಕೋಷ್ಟಕ_Addr_Type(ಕೋಷ್ಟಕ_2)));
_       (Compile_ಕೋಷ್ಟಕ_Op(o, op_ಕೋಷ್ಟಕ_Grow, ಕೋಷ್ಟಕ_2, 2, ಕೋಷ್ಟಕ_Addr_Type(ಕೋಷ್ಟಕ_2)));
    } ಇಲ್ಲದಿದ್ದರೆ {
_       (Check_ಕಾರ್ಯಾಂಶ_Type(o, 0, ಕೋಷ್ಟಕ_Addr_Type(ಕೋಷ್ಟಕ_2)));
_       (Check_ಕಾರ್ಯಾಂಶ_Type(o, 2, ಕೋಷ್ಟಕ_Addr_Type(ಕೋಷ್ಟಕ_2)));
_       (Compile_ಕೋಷ್ಟಕ_Op(o, op_ಕೋಷ್ಟಕ_Fill, ಕೋಷ್ಟಕ_2, 3, c_m3Type_none));
    }

    _catch: ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}

#ಆದರೆ_ಅಂತ್ಯ // d_m3HasRefTypes


M3_ಫಲಿತಾಂಶ Compile_ದತ್ತಾಂಶ_Drop (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_ಫಲಿತಾಂಶ ಫಲಿತಾಂಶ = m3Err_none;

    M3_ದತ್ತಾಂಶ_Segment* segment = ಖಾಲಿ;
_   (Read_ದತ್ತಾಂಶ_Segment(o, &segment));

_   (EmitOp(o, op_ದತ್ತಾಂಶ_Drop));
    Emit_ಸೂಚಕ(o, segment);

    _catch: ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}


ಸ್ಥಿರ
M3_ಫಲಿತಾಂಶ Read_ಖಂಡ_Type (IM3Compilation o, IM3FuncType* o_ಖಂಡ_Type)
{
    M3_ಫಲಿತಾಂಶ ಫಲಿತಾಂಶ = m3Err_none;

#ಆದರೆ d_m3HasTypedRefs
    // a spelled-out reference type is two bytes, so it cannot be told apart
    // from a type index by the s33 that block types otherwise use
    ಆದರೆ (o->wasm < o->wasm_ಕೊನೆ and (*o->wasm == d_waEncode_ref or *o->wasm == d_waEncode_refNull)) {
        m3type_t refType;
_       (Parse_ಮೌಲ್ಯ_Type(o->module, &refType, &o->wasm, o->wasm_ಕೊನೆ));
        IM3FuncType ftype = ಖಾಲಿ;
_       (AllocFuncType(&ftype, 1));
        ftype->ಅಂಕೆ_Rets  = 1;
        ftype->types[0] = refType;
_       (Environment_ಕೂಡಿಸು_Func_Type(o->module->environment, &ftype));
        *o_ಖಂಡ_Type = ftype;
        ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
    }
#ಆದರೆ_ಅಂತ್ಯ

    i64 type;
_   (ReadLebSigned(&type, 33, &o->wasm, o->wasm_ಕೊನೆ));

    ಆದರೆ (type < 0) {
        _throwif(m3Err_invalidTypeId, type < -64);
        u8 ಮೌಲ್ಯ_Type;
_       (NormalizeType(&ಮೌಲ್ಯ_Type, (i8)type));                                 m3log (compile, d_indent " (type: %s)", get_indention_string (o), c_waTypes [ಮೌಲ್ಯ_Type]);
        *o_ಖಂಡ_Type = o->module->environment->retFuncTypes[ಮೌಲ್ಯ_Type];
    } ಇಲ್ಲದಿದ್ದರೆ {
        _throwif("func type out of bounds", type >= o->module->ಅಂಕೆ_Func_Types);
        *o_ಖಂಡ_Type = o->module->funcTypes[type];                         m3log (compile, d_indent " (type: %s)", get_indention_string (o), SPrintFuncTypeSignature (*o_ಖಂಡ_Type));
    }
    _catch: ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}

ಸ್ಥಿರ
M3_ಫಲಿತಾಂಶ Preserve_ವಾದಗಳು_And_Locals (IM3Compilation o)
{
    M3_ಫಲಿತಾಂಶ ಫಲಿತಾಂಶ = m3Err_none;

    ಆದರೆ (o->ರಾಶಿ_ಸೂಚ್ಯಂಕ > o->ರಾಶಿ_ಮೊದಲ_Dynamic_ಸೂಚ್ಯಂಕ) {
        u32 ಅಂಕೆ_ವಾದಗಳು_And_Locals = Get_Function_ಅಂಕೆ_ವಾದಗಳು_And_Locals(o->function);

        ಗಾಗಿ (u32 i = 0; i < ಅಂಕೆ_ವಾದಗಳು_And_Locals; ++i) {
            u16 ಸ್ಥಾನ = Get_ಸ್ಥಾನ_For_ರಾಶಿ_ಸೂಚ್ಯಂಕ(o, i);

            u16 preserved_ಸ್ಥಾನ_ಸಂಖ್ಯೆ;
_           (ಕಂಡುಹಿಡಿ_Referenced_Local_Within_Current_ಖಂಡ(o, &preserved_ಸ್ಥಾನ_ಸಂಖ್ಯೆ, ಸ್ಥಾನ));

            ಆದರೆ (preserved_ಸ್ಥಾನ_ಸಂಖ್ಯೆ != ಸ್ಥಾನ) {
                m3type_t     type = Get_ರಾಶಿ_Type_From_Bottom(o, i);                 d_m3Assert (type != c_m3Type_none)
                IM3Operation op   = Is64_ಬಿಟ್_Type(type) ? op_Copy_ಸ್ಥಾನ_64 : op_Copy_ಸ್ಥಾನ_32;

                EmitOp(o, op);
                Emit_ಸ್ಥಾನ_ಸ್ಥಾನಾಂತರ(o, preserved_ಸ್ಥಾನ_ಸಂಖ್ಯೆ);
                Emit_ಸ್ಥಾನ_ಸ್ಥಾನಾಂತರ(o, ಸ್ಥಾನ);
            }
        }
    }

    _catch:
    ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}

ಸ್ಥಿರ
M3_ಫಲಿತಾಂಶ Compile_Loop_Or_ಖಂಡ (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_ಫಲಿತಾಂಶ ಫಲಿತಾಂಶ;

    IM3FuncType ಖಂಡ_Type = ಖಾಲಿ;

_   (PreserveRegisters(o));
_   (Preserve_ವಾದಗಳು_And_Locals(o));

_   (Read_ಖಂಡ_Type(o, &ಖಂಡ_Type));

    ಆದರೆ (i_opcode == c_waOp_loop) {
        u16 ಅಂಕೆ_Params = Get_Func_Type_ಅಂಕೆ_Params(ಖಂಡ_Type);
        ಆದರೆ (ಅಂಕೆ_Params) {
            // instantiate constants
            u16 ಅಂಕೆ_ಮೌಲ್ಯಗಳು = Get_ಅಂಕೆ_ಖಂಡ_ಮೌಲ್ಯಗಳು_On_ರಾಶಿ(o);

            ಆದರೆ (ಅಂಕೆ_ಮೌಲ್ಯಗಳು >= ಅಂಕೆ_Params) {
                u16 ರಾಶಿ_ತುದಿ = Get_ರಾಶಿ_ತುದಿ_ಸೂಚ್ಯಂಕ(o) + 1;

                ಗಾಗಿ (u16 i = ರಾಶಿ_ತುದಿ - ಅಂಕೆ_Params; i < ರಾಶಿ_ತುದಿ; ++i) {
                    u16      ಸ್ಥಾನ = Get_ಸ್ಥಾನ_For_ರಾಶಿ_ಸೂಚ್ಯಂಕ(o, i);
                    m3type_t type = Get_ರಾಶಿ_Type_From_Bottom(o, i);

                    ಆದರೆ (Is_Constant_ಸ್ಥಾನ(o, ಸ್ಥಾನ)) {
                        u16 new_ಸ್ಥಾನ = c_ಸ್ಥಾನ_ಬಳಸದ;
_                       (AllocateSlots(o, &new_ಸ್ಥಾನ, type));
_                       (Copy_ರಾಶಿ_ಸೂಚ್ಯಂಕ_ಗಮ್ಯ_ಸ್ಥಾನ(o, new_ಸ್ಥಾನ, i));
                        o->wasm_ರಾಶಿ[i] = new_ಸ್ಥಾನ;
                    }
                }
            }
        }

_       (EmitOp(o, op_Loop));
    } ಇಲ್ಲದಿದ್ದರೆ {
    }

_   (Compile_ಖಂಡ(o, ಖಂಡ_Type, i_opcode));

    _catch: ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}

#ಆದರೆ d_m3HasExceptionHandling

// Emits one out-of-line stub per catch clause, and writes each stub's address
// into the slot op_TryTable reserved for it.
//
// Called from CompileBlock with the try block's scope already on the stack, so
// the clause labels - which count from outside the try - are one deeper here.
// The payload values are pushed on top of whatever the block entry left there:
// only their position relative to the stack top matters, since
// ResolveBlockResults copies the topmost values into the label's landing pads.
// Popping them again afterwards puts the compiler's stack back where the body
// expects to find it.
ಸ್ಥಿರ
M3_ಫಲಿತಾಂಶ EmitCatchStubs (IM3Compilation o)
{
    IM3_ಸಂಕೇತ_ಪುಟ displaced_ಪುಟ = ಖಾಲಿ;
_try {
    M3CatchClause* clauses    = o->tryClauses;
    u32            ಅಂಕೆ_Clauses = o->ಅಂಕೆ_Try_Clauses;

    // the body must not see these: a nested try_table sets up its own
    o->tryClauses    = ಖಾಲಿ;
    o->ಅಂಕೆ_Try_Clauses = 0;

    ಗಾಗಿ (u32 i = 0; i < ಅಂಕೆ_Clauses; ++i) {
        M3CatchClause* clause = &clauses[i];

        IM3CompilationScope scope;
_       (Get_ಖಂಡ_Scope(o, &scope, clause->label_ಆಳ + 1));

        IM3FuncType payload = clause->ಟ್ಯಾಗ್ ? clause->ಟ್ಯಾಗ್->type : ಖಾಲಿ;
        u16         ಅಂಕೆ_ವಾದಗಳು = Get_Func_Type_ಅಂಕೆ_Params(payload);

        // What the stub hands the label - the payload, then the exnref for a
        // _ref clause - has to be what the label takes. ResolveBlockResults
        // only counts the values, and the validator compares base types, so a
        // typed reference is only checked here: (ref null $t) reaching a label
        // that takes (ref $t) has to be rejected.
        {
            bool isLoop         = (scope->opcode == c_waOp_loop);
            u16  ಅಂಕೆ_Label_ಮೌಲ್ಯಗಳು = isLoop ? Get_Func_Type_ಅಂಕೆ_Params(scope->type)
                                         : Get_Func_Type_ಅಂಕೆ_Results(scope->type);

            _throwif(m3_Err_type_ಎಣಿಕೆ_Mismatch, ಅಂಕೆ_Label_ಮೌಲ್ಯಗಳು != ಅಂಕೆ_ವಾದಗಳು + (clause->hasRef ? 1 : 0));

            ಗಾಗಿ (u16 a = 0; a < ಅಂಕೆ_Label_ಮೌಲ್ಯಗಳು; ++a) {
                m3type_t given  = (a < ಅಂಕೆ_ವಾದಗಳು) ? GetFuncTypeParamType(payload, a)
                                                : (m3type_t)c_m3Type_exnref;
                m3type_t wanted = isLoop ? GetFuncTypeParamType(scope->type, a)
                                         : Get_Func_Type_ಫಲಿತಾಂಶ_Type(scope->type, a);

                _throwif(m3Err_typeMismatch, not Is_Sub_Type_ದ(given, wanted));
            }
        }

        IM3_ಸಂಕೇತ_ಪುಟ stub_ಪುಟ;
_       (Acquire_Compilation_ಸಂಕೇತ_ಪುಟ(o, &stub_ಪುಟ));

        pc_t ಆರಂಭ_PC  = Get_ಪುಟ_PC(stub_ಪುಟ);
        displaced_ಪುಟ = o->ಪುಟ;
        o->ಪುಟ       = stub_ಪುಟ;

        u16 ಮೊದಲ_ಸೂಚ್ಯಂಕ = o->ರಾಶಿ_ಸೂಚ್ಯಂಕ;

        ಗಾಗಿ (u16 a = 0; a < ಅಂಕೆ_ವಾದಗಳು; ++a) {
_           (ತಳ್ಳು_Allocated_ಸ್ಥಾನ(o, GetFuncTypeParamType(payload, a)));
        }

        ಆದರೆ (clause->hasRef) {
_           (ತಳ್ಳು_Allocated_ಸ್ಥಾನ(o, c_m3Type_exnref));
        }

        // emitted even with nothing to move: this is also where an exception
        // no clause will reify gets released
_       (Ensure_ಸಂಕೇತ_ಪುಟ_ಅಂಕೆ_ಸಾಲುಗಳು(o, 2 * ಅಂಕೆ_ವಾದಗಳು + 3 + d_m3_ಸಂಕೇತ_ಪುಟ_Free_ಸಾಲುಗಳು_ಮಿತಿ));

_       (EmitOp(o, op_CatchPayload));
        EmitConstant32(o, ಅಂಕೆ_ವಾದಗಳು);

        ಗಾಗಿ (u16 a = 0; a < ಅಂಕೆ_ವಾದಗಳು; ++a) {
            u16 index = ಮೊದಲ_ಸೂಚ್ಯಂಕ + a;
            Emit_ಸ್ಥಾನ_ಸ್ಥಾನಾಂತರ(o, Get_ಸ್ಥಾನ_For_ರಾಶಿ_ಸೂಚ್ಯಂಕ(o, index));
            EmitConstant32(o, Is64_ಬಿಟ್_Type(Get_ರಾಶಿ_Type_From_Bottom(o, index)));
        }

        Emit_ಸ್ಥಾನ_ಸ್ಥಾನಾಂತರ(o, clause->hasRef ? (i32)Get_ಸ್ಥಾನ_For_ರಾಶಿ_ಸೂಚ್ಯಂಕ(o, ಮೊದಲ_ಸೂಚ್ಯಂಕ + ಅಂಕೆ_ವಾದಗಳು) : -1);

_       (Emit_ತೆಗೆ_Try_ಚೌಕಟ್ಟುಗಳು(o, ಅಂಕೆ_Try_ಚೌಕಟ್ಟುಗಳು_ಗಮ್ಯ_ತೆಗೆ(o->ಖಂಡ.outer, scope)));

        ಆದರೆ (scope->opcode == c_waOp_loop) {
_           (Resolve_ಖಂಡ_Results(o, scope, /* isBranch: */ true));

_           (EmitOp(o, Op_ContinueLoop(o)));
            Emit_ಸೂಚಕ(o, scope->pc);
_           (ದಾಖಲೆ_ಹಿಂಭಾಗ_ಅಂಚು(o, scope, false));
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (scope->ಆಳ == 0) {
_           (Return_ಮೌಲ್ಯಗಳು(o, scope, /* isBranch: */ true));
_           (EmitOp(o, op_Return));
        } ಇಲ್ಲದಿದ್ದರೆ {
_           (Resolve_ಖಂಡ_Results(o, scope, /* isBranch: */ true));
_           (EmitPatchingBranch(o, scope));
        }

        IM3_ಸಂಕೇತ_ಪುಟ return_ಗಮ್ಯ = displaced_ಪುಟ;
        displaced_ಪುಟ        = ಖಾಲಿ;

        Release_Compilation_ಸಂಕೇತ_ಪುಟ(o);
        o->ಪುಟ = return_ಗಮ್ಯ;

        *(pc_t*)clause->stub_ಸ್ಥಾನ = ಆರಂಭ_PC;

        ಆಗ (o->ರಾಶಿ_ಸೂಚ್ಯಂಕ > ಮೊದಲ_ಸೂಚ್ಯಂಕ) {
_           (ತೆಗೆ(o));
        }
    }

} _catch:

    // thrown with a stub page installed: release it and put back the one it
    // displaced, which the caller's catch then releases
    ಆದರೆ (displaced_ಪುಟ) {
        Release_Compilation_ಸಂಕೇತ_ಪುಟ(o);
        o->ಪುಟ = displaced_ಪುಟ;
    }

    ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}


ಸ್ಥಿರ
M3_ಫಲಿತಾಂಶ Compile_Try_ಕೋಷ್ಟಕ (IM3Compilation o, m3opcode_t i_opcode)
{
    M3CatchClause* clauses = ಖಾಲಿ;
_try {
    // a catch entry has to be able to assume the operand stack is entirely in
    // slots: an unwind leaves nothing in the registers
_   (PreserveRegisters(o));
_   (Preserve_ವಾದಗಳು_And_Locals(o));

    IM3FuncType ಖಂಡ_Type = ಖಾಲಿ;
_   (Read_ಖಂಡ_Type(o, &ಖಂಡ_Type));

    u32 ಅಂಕೆ_Clauses;
_   (ReadLEB_u32(&ಅಂಕೆ_Clauses, &o->wasm, o->wasm_ಕೊನೆ));

    _throwif("too many catch clauses", ಅಂಕೆ_Clauses > d_m3_Max_Sane_ಟ್ಯಾಗುಗಳು_ಎಣಿಕೆ);

    ಆದರೆ (ಅಂಕೆ_Clauses) {
        clauses = m3_AllocArray(M3CatchClause, ಅಂಕೆ_Clauses);
        _throwifnull(clauses);
    }

    ಗಾಗಿ (u32 i = 0; i < ಅಂಕೆ_Clauses; ++i) {
        u8 ಬಗೆ;
_       (Read_u8(&ಬಗೆ, &o->wasm, o->wasm_ಕೊನೆ));
        _throwif(m3Err_wasmMalformed, ಬಗೆ > 0x03);

        clauses[i].ಟ್ಯಾಗ್    = ಖಾಲಿ;
        clauses[i].hasRef = (ಬಗೆ == 0x01 or ಬಗೆ == 0x03);

        ಆದರೆ (ಬಗೆ == 0x00 or ಬಗೆ == 0x01) {
            u32 ಟ್ಯಾಗ್_ಸೂಚ್ಯಂಕ;
_           (ReadLEB_u32(&ಟ್ಯಾಗ್_ಸೂಚ್ಯಂಕ, &o->wasm, o->wasm_ಕೊನೆ));
            _throwif(m3_Err_unknown_ಟ್ಯಾಗ್, ಟ್ಯಾಗ್_ಸೂಚ್ಯಂಕ >= o->module->ಅಂಕೆ_ಟ್ಯಾಗುಗಳು);

            IM3_ಟ್ಯಾಗ್ ಟ್ಯಾಗ್     = &o->module->ಟ್ಯಾಗುಗಳು[ಟ್ಯಾಗ್_ಸೂಚ್ಯಂಕ];
            clauses[i].ಟ್ಯಾಗ್ = ಟ್ಯಾಗ್->resolved ? ಟ್ಯಾಗ್->resolved : ಟ್ಯಾಗ್;
        }

_       (ReadLEB_u32(&clauses[i].label_ಆಳ, &o->wasm, o->wasm_ಕೊನೆ));
    }

    // op_TryTable, the clause count, and two words per clause, all of which have
    // to land on one page: the op reads the table by offset from its own pc
_   (Ensure_ಸಂಕೇತ_ಪುಟ_ಅಂಕೆ_ಸಾಲುಗಳು(o, 2 * ಅಂಕೆ_Clauses + 2 + d_m3_ಸಂಕೇತ_ಪುಟ_Free_ಸಾಲುಗಳು_ಮಿತಿ));

_   (EmitOp(o, op_Try_ಕೋಷ್ಟಕ));
    EmitConstant32(o, ಅಂಕೆ_Clauses);

    ಗಾಗಿ (u32 i = 0; i < ಅಂಕೆ_Clauses; ++i) {
        Emit_ಸೂಚಕ(o, clauses[i].ಟ್ಯಾಗ್);
        clauses[i].stub_ಸ್ಥಾನ = Reserve_ಸೂಚಕ(o);
    }

    o->tryClauses    = clauses;
    o->ಅಂಕೆ_Try_Clauses = ಅಂಕೆ_Clauses;

_   (Compile_ಖಂಡ(o, ಖಂಡ_Type, c_wa_Op_try_ಕೋಷ್ಟಕ));

} _catch:

    o->tryClauses    = ಖಾಲಿ;
    o->ಅಂಕೆ_Try_Clauses = 0;

    m3_Free(clauses);

    ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}


ಸ್ಥಿರ
M3_ಫಲಿತಾಂಶ Compile_Throw (IM3Compilation o, m3opcode_t i_opcode)
{
_try {
    u32 ಟ್ಯಾಗ್_ಸೂಚ್ಯಂಕ;
_   (ReadLEB_u32(&ಟ್ಯಾಗ್_ಸೂಚ್ಯಂಕ, &o->wasm, o->wasm_ಕೊನೆ));
    _throwif(m3_Err_unknown_ಟ್ಯಾಗ್, ಟ್ಯಾಗ್_ಸೂಚ್ಯಂಕ >= o->module->ಅಂಕೆ_ಟ್ಯಾಗುಗಳು);

    IM3_ಟ್ಯಾಗ್ ಟ್ಯಾಗ್ = &o->module->ಟ್ಯಾಗುಗಳು[ಟ್ಯಾಗ್_ಸೂಚ್ಯಂಕ];
    ಆದರೆ (ಟ್ಯಾಗ್->resolved) {
        ಟ್ಯಾಗ್ = ಟ್ಯಾಗ್->resolved;
    }

    u16 ಅಂಕೆ_ವಾದಗಳು = Get_Func_Type_ಅಂಕೆ_Params(ಟ್ಯಾಗ್->type);

    // the payload is read out of slots, so nothing may still be in a register
_   (PreserveRegisters(o));

    // check the payload before emitting anything, so a mistyped throw does not
    // leave half an operation behind
    bool isPolymorphic = Is_ರಾಶಿ_Polymorphic(o);

    ಆದರೆ (not isPolymorphic) {
        _throwif(m3_Err_type_ಎಣಿಕೆ_Mismatch, Get_ಅಂಕೆ_ಖಂಡ_ಮೌಲ್ಯಗಳು_On_ರಾಶಿ(o) < ಅಂಕೆ_ವಾದಗಳು);

        ಗಾಗಿ (u16 i = 0; i < ಅಂಕೆ_ವಾದಗಳು; ++i) {
            u16 index = (u16)(o->ರಾಶಿ_ಸೂಚ್ಯಂಕ - ಅಂಕೆ_ವಾದಗಳು + i);

            _throwif(m3Err_typeMismatch, not Is_Sub_Type_ದ(Get_ರಾಶಿ_Type_From_Bottom(o, index),
                                                         GetFuncTypeParamType(ಟ್ಯಾಗ್->type, i)));
        }
    }

_   (Ensure_ಸಂಕೇತ_ಪುಟ_ಅಂಕೆ_ಸಾಲುಗಳು(o, 2 * ಅಂಕೆ_ವಾದಗಳು + 3 + d_m3_ಸಂಕೇತ_ಪುಟ_Free_ಸಾಲುಗಳು_ಮಿತಿ));

_   (EmitOp(o, op_Throw));
    Emit_ಸೂಚಕ(o, ಟ್ಯಾಗ್);
    EmitConstant32(o, ಅಂಕೆ_ವಾದಗಳು);

    // the payload is named in the order the tag declares it, so the deepest of
    // the arguments on the stack comes first. In unreachable code there is
    // nothing on the stack to name and nothing will run this, so slot 0 stands
    // in for an operand that isn't there.
    ಗಾಗಿ (u16 i = 0; i < ಅಂಕೆ_ವಾದಗಳು; ++i) {
        m3type_t type = GetFuncTypeParamType(ಟ್ಯಾಗ್->type, i);

        Emit_ಸ್ಥಾನ_ಸ್ಥಾನಾಂತರ(o, isPolymorphic ? 0 : Get_ಸ್ಥಾನ_For_ರಾಶಿ_ಸೂಚ್ಯಂಕ(o, (u16)(o->ರಾಶಿ_ಸೂಚ್ಯಂಕ - ಅಂಕೆ_ವಾದಗಳು + i)));
        EmitConstant32(o, Is64_ಬಿಟ್_Type(type));
    }

    ಆದರೆ (not isPolymorphic) {
        ಗಾಗಿ (u16 i = 0; i < ಅಂಕೆ_ವಾದಗಳು; ++i) {
_           (ತೆಗೆ(o));
        }
    }

_   (ಗಣ_ರಾಶಿ_Polymorphic(o));

} _catch: ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}


ಸ್ಥಿರ
M3_ಫಲಿತಾಂಶ Compile_ThrowRef (IM3Compilation o, m3opcode_t i_opcode)
{
_try {
    ಆದರೆ (not Is_ರಾಶಿ_Polymorphic(o)) {
_       (Preserve_ರಿಜಿಸ್ಟರ್_If_Occupied(o, c_m3Type_i64));

        _throwif(m3Err_typeMismatch, ತಳ_Type_ದ(Get_ರಾಶಿ_ತುದಿ_Type(o)) != c_m3Type_exnref);

_       (EmitOp(o, op_ThrowRef));
        Emit_ಸ್ಥಾನ_ಸ್ಥಾನಾಂತರ(o, Get_ರಾಶಿ_ತುದಿ_ಸ್ಥಾನ_ಸಂಖ್ಯೆ(o));

_       (ತೆಗೆ(o));
    }

_   (ಗಣ_ರಾಶಿ_Polymorphic(o));

} _catch: ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}

#ಆದರೆ_ಅಂತ್ಯ // d_m3HasExceptionHandling

#ಆದರೆ d_m3_Has_ರಾಶಿ_Switching

ಸ್ಥಿರ
M3_ಫಲಿತಾಂಶ Compile_ContNew (IM3Compilation o, m3opcode_t i_opcode)
{
_try {
    u32 type_ಸೂಚ್ಯಂಕ;
_   (ReadLEB_u32(&type_ಸೂಚ್ಯಂಕ, &o->wasm, o->wasm_ಕೊನೆ));
    _throwif(m3_Err_type_ಎಣಿಕೆ_Mismatch, type_ಸೂಚ್ಯಂಕ >= o->module->ಅಂಕೆ_Func_Types);

    IM3FuncType funcType = o->module->funcTypes[type_ಸೂಚ್ಯಂಕ];
    _throwif("type is not a continuation", not funcType->isContinuation);

    // the operands are read out of slots, so nothing may still be in a register
_   (PreserveRegisters(o));

    // cont.new $ct : [(ref null $ft)] -> [(ref $ct)], where $ct ~~ cont $ft:
    // the function has to be one this continuation type can run
    m3type_t ತುದಿ_Type = Get_ರಾಶಿ_ತುದಿ_Type(o);
    _throwif(m3Err_typeMismatch,
             not Is_Sub_Type_ದ(ತುದಿ_Type, Ref_Type_ದ_Func_Type(funcType->contFuncType, false)));

    u16 func_ಸ್ಥಾನ = Get_ರಾಶಿ_ತುದಿ_ಸ್ಥಾನ_ಸಂಖ್ಯೆ(o);
_   (ತೆಗೆ(o));

    u16 dst_ಸ್ಥಾನ;
_   (ತಳ್ಳು_Allocated_ಸ್ಥಾನ(o, Ref_Type_ದ_Func_Type(funcType, true)));
    dst_ಸ್ಥಾನ = Get_ರಾಶಿ_ತುದಿ_ಸ್ಥಾನ_ಸಂಖ್ಯೆ(o);

_   (Ensure_ಸಂಕೇತ_ಪುಟ_ಅಂಕೆ_ಸಾಲುಗಳು(o, 4 + d_m3_ಸಂಕೇತ_ಪುಟ_Free_ಸಾಲುಗಳು_ಮಿತಿ));
_   (EmitOp(o, op_ContNew));
    Emit_ಸೂಚಕ(o, funcType);
    Emit_ಸ್ಥಾನ_ಸ್ಥಾನಾಂತರ(o, dst_ಸ್ಥಾನ);
    Emit_ಸ್ಥಾನ_ಸ್ಥಾನಾಂತರ(o, func_ಸ್ಥಾನ);

} _catch: ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}

ಸ್ಥಿರ
M3_ಫಲಿತಾಂಶ Compile_ContBind (IM3Compilation o, m3opcode_t i_opcode)
{
_try {
    u32 typeIndex1, typeIndex2;
_   (ReadLEB_u32(&typeIndex1, &o->wasm, o->wasm_ಕೊನೆ));
_   (ReadLEB_u32(&typeIndex2, &o->wasm, o->wasm_ಕೊನೆ));
    _throwif(m3_Err_type_ಎಣಿಕೆ_Mismatch, typeIndex1 >= o->module->ಅಂಕೆ_Func_Types);
    _throwif(m3_Err_type_ಎಣಿಕೆ_Mismatch, typeIndex2 >= o->module->ಅಂಕೆ_Func_Types);

    IM3FuncType ct1 = o->module->funcTypes[typeIndex1];
    IM3FuncType ct2 = o->module->funcTypes[typeIndex2];
    _throwif("type is not a continuation", not ct1->isContinuation || not ct2->isContinuation);

    // the operands are read out of slots, so nothing may still be in a register
_   (PreserveRegisters(o));

    m3type_t contType = Get_ರಾಶಿ_ತುದಿ_Type(o);
    _throwif(m3Err_typeMismatch, not Is_Sub_Type_ದ(contType, Ref_Type_ದ_Func_Type(ct1, false)));
    u16 cont_ಸ್ಥಾನ = Get_ರಾಶಿ_ತುದಿ_ಸ್ಥಾನ_ಸಂಖ್ಯೆ(o);
_   (ತೆಗೆ(o));

    IM3FuncType ft1 = ct1->contFuncType ? ct1->contFuncType : ct1;
    IM3FuncType ft2 = ct2->contFuncType ? ct2->contFuncType : ct2;

    u16 ಅಂಕೆ_Params1 = Get_Func_Type_ಅಂಕೆ_Params(ft1);
    u16 ಅಂಕೆ_Params2 = Get_Func_Type_ಅಂಕೆ_Params(ft2);
    _throwif(m3Err_typeMismatch, ಅಂಕೆ_Params1 < ಅಂಕೆ_Params2);
    u32 ಅಂಕೆ_Bound = ಅಂಕೆ_Params1 - ಅಂಕೆ_Params2;

    u16 ಅಂಕೆ_Results1 = Get_Func_Type_ಅಂಕೆ_Results(ft1);
    u16 ಅಂಕೆ_Results2 = Get_Func_Type_ಅಂಕೆ_Results(ft2);
    _throwif(m3Err_typeMismatch, ಅಂಕೆ_Results1 != ಅಂಕೆ_Results2);
    ಗಾಗಿ (u16 i = 0; i < ಅಂಕೆ_Results1; ++i) {
        _throwif(m3Err_typeMismatch, Get_Func_Type_ಫಲಿತಾಂಶ_Type(ft1, i) != Get_Func_Type_ಫಲಿತಾಂಶ_Type(ft2, i));
    }
    ಗಾಗಿ (u16 i = 0; i < ಅಂಕೆ_Params2; ++i) {
        _throwif(m3Err_typeMismatch, GetFuncTypeParamType(ft1, ಅಂಕೆ_Bound + i) != GetFuncTypeParamType(ft2, i));
    }

    u16 argSlots[d_m3MaxContinuationPayload];
    _throwif("too many bound args", ಅಂಕೆ_Bound > d_m3MaxContinuationPayload);

    ಗಾಗಿ (u32 i = 0; i < ಅಂಕೆ_Bound; ++i) {
        u32      paramIdx     = ಅಂಕೆ_Bound - 1 - i;
        m3type_t expectedType = GetFuncTypeParamType(ft1, paramIdx);
        m3type_t actualType   = Get_ರಾಶಿ_ತುದಿ_Type(o);
        _throwif(m3Err_typeMismatch, not Is_Sub_Type_ದ(actualType, expectedType));
        argSlots[paramIdx] = Get_ರಾಶಿ_ತುದಿ_ಸ್ಥಾನ_ಸಂಖ್ಯೆ(o);
_       (ತೆಗೆ(o));
    }

    u16 dst_ಸ್ಥಾನ;
_   (ತಳ್ಳು_Allocated_ಸ್ಥಾನ(o, Ref_Type_ದ_Func_Type(ct2, true)));
    dst_ಸ್ಥಾನ = Get_ರಾಶಿ_ತುದಿ_ಸ್ಥಾನ_ಸಂಖ್ಯೆ(o);

_   (Ensure_ಸಂಕೇತ_ಪುಟ_ಅಂಕೆ_ಸಾಲುಗಳು(o, 6 + ಅಂಕೆ_Bound * 2 + d_m3_ಸಂಕೇತ_ಪುಟ_Free_ಸಾಲುಗಳು_ಮಿತಿ));
_   (EmitOp(o, op_ContBind));
    Emit_ಸೂಚಕ(o, ct1);
    Emit_ಸೂಚಕ(o, ct2);
    Emit_ಸ್ಥಾನ_ಸ್ಥಾನಾಂತರ(o, dst_ಸ್ಥಾನ);
    Emit_ಸ್ಥಾನ_ಸ್ಥಾನಾಂತರ(o, cont_ಸ್ಥಾನ);
    EmitConstant32(o, ಅಂಕೆ_Bound);
    ಗಾಗಿ (u32 i = 0; i < ಅಂಕೆ_Bound; ++i) {
        Emit_ಸ್ಥಾನ_ಸ್ಥಾನಾಂತರ(o, argSlots[i]);
        EmitConstant32(o, Is64_ಬಿಟ್_Type(GetFuncTypeParamType(ft1, i)));
    }

} _catch: ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}

ಸ್ಥಿರ
M3_ಫಲಿತಾಂಶ Compile_Suspend (IM3Compilation o, m3opcode_t i_opcode)
{
_try {
    u32 ಟ್ಯಾಗ್_ಸೂಚ್ಯಂಕ;
_   (ReadLEB_u32(&ಟ್ಯಾಗ್_ಸೂಚ್ಯಂಕ, &o->wasm, o->wasm_ಕೊನೆ));
    _throwif(m3_Err_unknown_ಟ್ಯಾಗ್, ಟ್ಯಾಗ್_ಸೂಚ್ಯಂಕ >= o->module->ಅಂಕೆ_ಟ್ಯಾಗುಗಳು);

    IM3_ಟ್ಯಾಗ್ ಟ್ಯಾಗ್ = &o->module->ಟ್ಯಾಗುಗಳು[ಟ್ಯಾಗ್_ಸೂಚ್ಯಂಕ];
    ಆದರೆ (ಟ್ಯಾಗ್->resolved) {
        ಟ್ಯಾಗ್ = ಟ್ಯಾಗ್->resolved;
    }
    u16 ಅಂಕೆ_Params  = Get_Func_Type_ಅಂಕೆ_Params(ಟ್ಯಾಗ್->type);
    u16 ಅಂಕೆ_Results = Get_Func_Type_ಅಂಕೆ_Results(ಟ್ಯಾಗ್->type);

_   (PreserveRegisters(o));

    u16 paramSlots[d_m3MaxContinuationPayload];
    _throwif("too many tag params", ಅಂಕೆ_Params > d_m3MaxContinuationPayload);
    ಗಾಗಿ (u16 i = 0; i < ಅಂಕೆ_Params; ++i) {
        u16      pIdx         = ಅಂಕೆ_Params - 1 - i;
        m3type_t expectedType = GetFuncTypeParamType(ಟ್ಯಾಗ್->type, pIdx);
        m3type_t actualType   = Get_ರಾಶಿ_ತುದಿ_Type(o);
        _throwif(m3Err_typeMismatch, not Is_Sub_Type_ದ(actualType, expectedType));
        paramSlots[pIdx] = Get_ರಾಶಿ_ತುದಿ_ಸ್ಥಾನ_ಸಂಖ್ಯೆ(o);
_       (ತೆಗೆ(o));
    }

    u16 ಫಲಿತಾಂಶ_Slots[d_m3MaxContinuationPayload];
    _throwif("too many tag results", ಅಂಕೆ_Results > d_m3MaxContinuationPayload);
    ಗಾಗಿ (u16 i = 0; i < ಅಂಕೆ_Results; ++i) {
        m3type_t rtype = Get_Func_Type_ಫಲಿತಾಂಶ_Type(ಟ್ಯಾಗ್->type, i);
_       (ತಳ್ಳು_Allocated_ಸ್ಥಾನ(o, rtype));
        ಫಲಿತಾಂಶ_Slots[i] = Get_ರಾಶಿ_ತುದಿ_ಸ್ಥಾನ_ಸಂಖ್ಯೆ(o);
    }

_   (Ensure_ಸಂಕೇತ_ಪುಟ_ಅಂಕೆ_ಸಾಲುಗಳು(o, 4 + ಅಂಕೆ_Params * 2 + ಅಂಕೆ_Results * 2 + d_m3_ಸಂಕೇತ_ಪುಟ_Free_ಸಾಲುಗಳು_ಮಿತಿ));
_   (EmitOp(o, op_Suspend));
    Emit_ಸೂಚಕ(o, ಟ್ಯಾಗ್);
    EmitConstant32(o, ಅಂಕೆ_Params);
    ಗಾಗಿ (u16 i = 0; i < ಅಂಕೆ_Params; ++i) {
        Emit_ಸ್ಥಾನ_ಸ್ಥಾನಾಂತರ(o, paramSlots[i]);
        EmitConstant32(o, Is64_ಬಿಟ್_Type(GetFuncTypeParamType(ಟ್ಯಾಗ್->type, i)));
    }
    EmitConstant32(o, ಅಂಕೆ_Results);
    ಗಾಗಿ (u16 i = 0; i < ಅಂಕೆ_Results; ++i) {
        Emit_ಸ್ಥಾನ_ಸ್ಥಾನಾಂತರ(o, ಫಲಿತಾಂಶ_Slots[i]);
        EmitConstant32(o, Is64_ಬಿಟ್_Type(Get_Func_Type_ಫಲಿತಾಂಶ_Type(ಟ್ಯಾಗ್->type, i)));
    }

    // the results are part of the frame here: op_Suspend clears them, and
    // whatever resumes this writes them before anything reads them
_   (ದಾಖಲೆ_ಸುರಕ್ಷಿತ_ಬಿಂದು(o, safepoint_suspend, 0, ಅಂಕೆ_Results, 0, ಖಾಲಿ));

} _catch: ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}

// resume, resume_throw and resume_throw_ref. All three run a continuation
// under a handler table and take its results; they differ only in what they
// hand it to start with - arguments, a fresh exception, or one off the stack.
//
// Emitted as: the continuation type, the slot holding it, the operands that
// belong to this form, the handler table, then the result slots. Keeping the
// tail identical is what lets one runtime path finish all three.
ಸ್ಥಿರ
M3_ಫಲಿತಾಂಶ Compile_Resume (IM3Compilation o, m3opcode_t i_opcode)
{
    M3ResumeHandler* handlers      = ಖಾಲಿ;
    IM3_ಸಂಕೇತ_ಪುಟ      displaced_ಪುಟ = ಖಾಲಿ;
_try {
    bool isThrow    = (i_opcode == c_waOp_resumeThrow);
    bool isThrowRef = (i_opcode == c_waOp_resumeThrowRef);

    u32 type_ಸೂಚ್ಯಂಕ;
_   (ReadLEB_u32(&type_ಸೂಚ್ಯಂಕ, &o->wasm, o->wasm_ಕೊನೆ));
    _throwif(m3_Err_type_ಎಣಿಕೆ_Mismatch, type_ಸೂಚ್ಯಂಕ >= o->module->ಅಂಕೆ_Func_Types);

    IM3FuncType contFuncType = o->module->funcTypes[type_ಸೂಚ್ಯಂಕ];
    _throwif("type is not a continuation", not contFuncType->isContinuation);

    IM3FuncType innerType      = contFuncType->contFuncType;
    u16         ಅಂಕೆ_Cont_Results = Get_Func_Type_ಅಂಕೆ_Results(innerType);

    IM3_ಟ್ಯಾಗ್ exn_ಟ್ಯಾಗ್      = ಖಾಲಿ;
    u16    ಅಂಕೆ_Operands = 0;

    ಆದರೆ (isThrow) {
        u32 ಟ್ಯಾಗ್_ಸೂಚ್ಯಂಕ;
_       (ReadLEB_u32(&ಟ್ಯಾಗ್_ಸೂಚ್ಯಂಕ, &o->wasm, o->wasm_ಕೊನೆ));
        _throwif(m3_Err_unknown_ಟ್ಯಾಗ್, ಟ್ಯಾಗ್_ಸೂಚ್ಯಂಕ >= o->module->ಅಂಕೆ_ಟ್ಯಾಗುಗಳು);

        exn_ಟ್ಯಾಗ್ = &o->module->ಟ್ಯಾಗುಗಳು[ಟ್ಯಾಗ್_ಸೂಚ್ಯಂಕ];

        // it is an exception that gets raised, so the tag must be one throw
        // could have raised: no results
        _throwif(m3Err_typeMismatch, Get_Func_Type_ಅಂಕೆ_Results(exn_ಟ್ಯಾಗ್->type) != 0);

        ಅಂಕೆ_Operands = Get_Func_Type_ಅಂಕೆ_Params(exn_ಟ್ಯಾಗ್->type);
    } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (not isThrowRef) {
        // a plain resume hands over the continuation's own parameters. An
        // aborted one never receives them, so they are unconstrained there.
        ಅಂಕೆ_Operands = Get_Func_Type_ಅಂಕೆ_Params(innerType);
    }

    u32 ಅಂಕೆ_Clauses;
_   (ReadLEB_u32(&ಅಂಕೆ_Clauses, &o->wasm, o->wasm_ಕೊನೆ));

    u32 clauseLabelDepths[16];
    _throwif("too many resume clauses", ಅಂಕೆ_Clauses > 16);

    ಆದರೆ (ಅಂಕೆ_Clauses) {
        handlers = m3_AllocArray(M3ResumeHandler, ಅಂಕೆ_Clauses);
        _throwifnull(handlers);
    }

    ಗಾಗಿ (u32 i = 0; i < ಅಂಕೆ_Clauses; ++i) {
        u8 ಬಗೆ;
_       (Read_u8(&ಬಗೆ, &o->wasm, o->wasm_ಕೊನೆ));
        _throwif(m3Err_wasmMalformed, ಬಗೆ > 0x01);

        handlers[i].ಬಗೆ = ಬಗೆ;

        u32 ಟ್ಯಾಗ್_ಸೂಚ್ಯಂಕ;
_       (ReadLEB_u32(&ಟ್ಯಾಗ್_ಸೂಚ್ಯಂಕ, &o->wasm, o->wasm_ಕೊನೆ));
        _throwif(m3_Err_unknown_ಟ್ಯಾಗ್, ಟ್ಯಾಗ್_ಸೂಚ್ಯಂಕ >= o->module->ಅಂಕೆ_ಟ್ಯಾಗುಗಳು);

        IM3_ಟ್ಯಾಗ್ ಟ್ಯಾಗ್         = &o->module->ಟ್ಯಾಗುಗಳು[ಟ್ಯಾಗ್_ಸೂಚ್ಯಂಕ];
        handlers[i].ಟ್ಯಾಗ್    = ಟ್ಯಾಗ್->resolved ? ಟ್ಯಾಗ್->resolved : ಟ್ಯಾಗ್;
        handlers[i].stubPC = ಖಾಲಿ;

        ಆದರೆ (ಬಗೆ == 0x00) {
_           (ReadLEB_u32(&clauseLabelDepths[i], &o->wasm, o->wasm_ಕೊನೆ));
        } ಇಲ್ಲದಿದ್ದರೆ {
            // (on $e switch): the tag carries no payload, and what the switch
            // site yields is what this resume yields
            _throwif(m3Err_typeMismatch, Get_Func_Type_ಅಂಕೆ_Params(handlers[i].ಟ್ಯಾಗ್->type) != 0);
            clauseLabelDepths[i] = 0;
        }
    }

    // the operands are read out of slots, so nothing may still be in a register
_   (PreserveRegisters(o));

    m3type_t cont_ತುದಿ_Type = Get_ರಾಶಿ_ತುದಿ_Type(o);
    _throwif(m3Err_typeMismatch, not Is_Sub_Type_ದ(cont_ತುದಿ_Type, Ref_Type_ದ_Func_Type(contFuncType, false)));
    u16 cont_ಸ್ಥಾನ = Get_ರಾಶಿ_ತುದಿ_ಸ್ಥಾನ_ಸಂಖ್ಯೆ(o);
_   (ತೆಗೆ(o));

    u16 exn_ಸ್ಥಾನ = 0;

    ಆದರೆ (isThrowRef) {
        m3type_t exnType = Get_ರಾಶಿ_ತುದಿ_Type(o);
        _throwif(m3Err_typeMismatch, ತಳ_Type_ದ(exnType) != c_m3Type_exnref);
        exn_ಸ್ಥಾನ = Get_ರಾಶಿ_ತುದಿ_ಸ್ಥಾನ_ಸಂಖ್ಯೆ(o);
_       (ತೆಗೆ(o));
    }

    u16 ಕಾರ್ಯಾಂಶ_Slots[d_m3MaxContinuationPayload];
    _throwif("too many resume operands", ಅಂಕೆ_Operands > d_m3MaxContinuationPayload);

    IM3FuncType ಕಾರ್ಯಾಂಶ_Type = isThrow ? exn_ಟ್ಯಾಗ್->type : innerType;

    ಗಾಗಿ (u16 i = 0; i < ಅಂಕೆ_Operands; ++i) {
        u16      pIdx         = (u16)(ಅಂಕೆ_Operands - 1 - i);
        m3type_t expectedType = GetFuncTypeParamType(ಕಾರ್ಯಾಂಶ_Type, pIdx);
        m3type_t actualType   = Get_ರಾಶಿ_ತುದಿ_Type(o);
        _throwif(m3Err_typeMismatch, not Is_Sub_Type_ದ(actualType, expectedType));
        ಕಾರ್ಯಾಂಶ_Slots[pIdx] = Get_ರಾಶಿ_ತುದಿ_ಸ್ಥಾನ_ಸಂಖ್ಯೆ(o);
_       (ತೆಗೆ(o));
    }

    ಗಾಗಿ (u32 i = 0; i < ಅಂಕೆ_Clauses; ++i) {
        ಆದರೆ (handlers[i].ಬಗೆ != 0x00) {
            ಮುಂದುವರಿ;
        }

        IM3CompilationScope scope;
_       (Get_ಖಂಡ_Scope(o, &scope, clauseLabelDepths[i]));

        IM3_ಟ್ಯಾಗ್ ಟ್ಯಾಗ್        = handlers[i].ಟ್ಯಾಗ್;
        u16    ಅಂಕೆ_Payload = Get_Func_Type_ಅಂಕೆ_Params(ಟ್ಯಾಗ್->type);

        m3type_t labelContType = (scope->opcode == c_waOp_loop)
                                   ? GetFuncTypeParamType(scope->type, ಅಂಕೆ_Payload)
                                   : Get_Func_Type_ಫಲಿತಾಂಶ_Type(scope->type, ಅಂಕೆ_Payload);

        IM3_ಸಂಕೇತ_ಪುಟ stub_ಪುಟ;
_       (Acquire_Compilation_ಸಂಕೇತ_ಪುಟ(o, &stub_ಪುಟ));
        pc_t ಆರಂಭ_PC  = Get_ಪುಟ_PC(stub_ಪುಟ);
        displaced_ಪುಟ = o->ಪುಟ;
        o->ಪುಟ       = stub_ಪುಟ;

        u16 ಮೊದಲ_ಸೂಚ್ಯಂಕ = o->ರಾಶಿ_ಸೂಚ್ಯಂಕ;
        ಗಾಗಿ (u16 a = 0; a < ಅಂಕೆ_Payload; ++a) {
_           (ತಳ್ಳು_Allocated_ಸ್ಥಾನ(o, GetFuncTypeParamType(ಟ್ಯಾಗ್->type, a)));
        }

        // The handler is handed the suspended continuation.
_       (ತಳ್ಳು_Allocated_ಸ್ಥಾನ(o, labelContType));

_       (Ensure_ಸಂಕೇತ_ಪುಟ_ಅಂಕೆ_ಸಾಲುಗಳು(o, 2 * ಅಂಕೆ_Payload + 5 + d_m3_ಸಂಕೇತ_ಪುಟ_Free_ಸಾಲುಗಳು_ಮಿತಿ));
_       (EmitOp(o, op_ResumePayload));
        Emit_ಸೂಚಕ(o, Module_Cont_Type_ದ_Ref(o->module, labelContType));
        EmitConstant32(o, ಅಂಕೆ_Payload);
        ಗಾಗಿ (u16 a = 0; a < ಅಂಕೆ_Payload; ++a) {
            u16 index = ಮೊದಲ_ಸೂಚ್ಯಂಕ + a;
            Emit_ಸ್ಥಾನ_ಸ್ಥಾನಾಂತರ(o, Get_ಸ್ಥಾನ_For_ರಾಶಿ_ಸೂಚ್ಯಂಕ(o, index));
            EmitConstant32(o, Is64_ಬಿಟ್_Type(Get_ರಾಶಿ_Type_From_Bottom(o, index)));
        }
        Emit_ಸ್ಥಾನ_ಸ್ಥಾನಾಂತರ(o, Get_ಸ್ಥಾನ_For_ರಾಶಿ_ಸೂಚ್ಯಂಕ(o, ಮೊದಲ_ಸೂಚ್ಯಂಕ + ಅಂಕೆ_Payload));

#  ಆದರೆ d_m3HasExceptionHandling
        // the clause leaves every try region between the resume and its label,
        // and leaving one retires its handlers, the way a branch does
_       (Emit_ತೆಗೆ_Try_ಚೌಕಟ್ಟುಗಳು(o, ಅಂಕೆ_Try_ಚೌಕಟ್ಟುಗಳು_ಗಮ್ಯ_ತೆಗೆ(&o->ಖಂಡ, scope)));
#  endif

        ಆದರೆ (scope->opcode == c_waOp_loop) {
_           (Resolve_ಖಂಡ_Results(o, scope, true));
_           (EmitOp(o, Op_ContinueLoop(o)));
            Emit_ಸೂಚಕ(o, scope->pc);
_           (ದಾಖಲೆ_ಹಿಂಭಾಗ_ಅಂಚು(o, scope, false));
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (scope->ಆಳ == 0) {
_           (Return_ಮೌಲ್ಯಗಳು(o, scope, true));
_           (EmitOp(o, op_Return));
        } ಇಲ್ಲದಿದ್ದರೆ {
_           (Resolve_ಖಂಡ_Results(o, scope, true));
_           (EmitPatchingBranch(o, scope));
        }

        IM3_ಸಂಕೇತ_ಪುಟ return_ಗಮ್ಯ = displaced_ಪುಟ;
        displaced_ಪುಟ        = ಖಾಲಿ;

        Release_Compilation_ಸಂಕೇತ_ಪುಟ(o);
        o->ಪುಟ = return_ಗಮ್ಯ;

        handlers[i].stubPC = ಆರಂಭ_PC;

        ಆಗ (o->ರಾಶಿ_ಸೂಚ್ಯಂಕ > ಮೊದಲ_ಸೂಚ್ಯಂಕ) {
_           (ತೆಗೆ(o));
        }
    }

    u16 resSlots[d_m3MaxContinuationPayload];
    _throwif("too many cont results", ಅಂಕೆ_Cont_Results > d_m3MaxContinuationPayload);
    ಗಾಗಿ (u16 i = 0; i < ಅಂಕೆ_Cont_Results; ++i) {
        m3type_t rtype = Get_Func_Type_ಫಲಿತಾಂಶ_Type(innerType, i);
_       (ತಳ್ಳು_Allocated_ಸ್ಥಾನ(o, rtype));
        resSlots[i] = Get_ರಾಶಿ_ತುದಿ_ಸ್ಥಾನ_ಸಂಖ್ಯೆ(o);
    }

_   (Ensure_ಸಂಕೇತ_ಪುಟ_ಅಂಕೆ_ಸಾಲುಗಳು(o, 8 + ಅಂಕೆ_Operands * 2 + ಅಂಕೆ_Clauses * 3 + ಅಂಕೆ_Cont_Results * 2 +
                                  d_m3_ಸಂಕೇತ_ಪುಟ_Free_ಸಾಲುಗಳು_ಮಿತಿ));

_   (EmitOp(o, op_Resume));
    Emit_ಸೂಚಕ(o, contFuncType);
    Emit_ಸ್ಥಾನ_ಸ್ಥಾನಾಂತರ(o, cont_ಸ್ಥಾನ);
    EmitConstant32(o, isThrow ? d_m3ResumeThrow : (isThrowRef ? d_m3ResumeThrowRef : d_m3_Resume_ಸಾಮಾನ್ಯ));

    ಆದರೆ (isThrowRef) {
        Emit_ಸ್ಥಾನ_ಸ್ಥಾನಾಂತರ(o, exn_ಸ್ಥಾನ);
    } ಇಲ್ಲದಿದ್ದರೆ {
        ಆದರೆ (isThrow) {
            Emit_ಸೂಚಕ(o, exn_ಟ್ಯಾಗ್);
        }

        EmitConstant32(o, ಅಂಕೆ_Operands);
        ಗಾಗಿ (u16 i = 0; i < ಅಂಕೆ_Operands; ++i) {
            Emit_ಸ್ಥಾನ_ಸ್ಥಾನಾಂತರ(o, ಕಾರ್ಯಾಂಶ_Slots[i]);
            EmitConstant32(o, Is64_ಬಿಟ್_Type(GetFuncTypeParamType(ಕಾರ್ಯಾಂಶ_Type, i)));
        }
    }

    EmitConstant32(o, ಅಂಕೆ_Clauses);
    ಗಾಗಿ (u32 i = 0; i < ಅಂಕೆ_Clauses; ++i) {
        EmitConstant32(o, handlers[i].ಬಗೆ);
        Emit_ಸೂಚಕ(o, handlers[i].ಟ್ಯಾಗ್);
        Emit_ಸೂಚಕ(o, handlers[i].stubPC);
    }

    EmitConstant32(o, ಅಂಕೆ_Cont_Results);
    ಗಾಗಿ (u16 i = 0; i < ಅಂಕೆ_Cont_Results; ++i) {
        Emit_ಸ್ಥಾನ_ಸ್ಥಾನಾಂತರ(o, resSlots[i]);
        EmitConstant32(o, Is64_ಬಿಟ್_Type(Get_Func_Type_ಫಲಿತಾಂಶ_Type(innerType, i)));
    }

    // a suspend passing through the resumed continuation leaves this frame
    // waiting here, with the continuation's results not yet taken
_   (ದಾಖಲೆ_ಸುರಕ್ಷಿತ_ಬಿಂದು(o, safepoint_resume, 0, ಅಂಕೆ_Cont_Results, (u16)ಅಂಕೆ_Clauses, ಖಾಲಿ));
#  ಆದರೆ d_m3HasSnapshots
    ಆದರೆ (o->snapshotMap and o->ಪುಟ) {
        o->snapshotMap->ಸುರಕ್ಷಿತ_Points[o->snapshotMap->ಅಂಕೆ_ಸುರಕ್ಷಿತ_Points - 1].resumeType = Ref_Type_ದ_Func_Type(contFuncType, true);
    }
#  endif

} _catch:
    ಆದರೆ (displaced_ಪುಟ) {
        Release_Compilation_ಸಂಕೇತ_ಪುಟ(o);
        o->ಪುಟ = displaced_ಪುಟ;
    }
    m3_Free(handlers);
    ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}


ಸ್ಥಿರ
M3_ಫಲಿತಾಂಶ Compile_Switch (IM3Compilation o, m3opcode_t i_opcode)
{
_try {
    u32 type_ಸೂಚ್ಯಂಕ, ಟ್ಯಾಗ್_ಸೂಚ್ಯಂಕ;
_   (ReadLEB_u32(&type_ಸೂಚ್ಯಂಕ, &o->wasm, o->wasm_ಕೊನೆ));
_   (ReadLEB_u32(&ಟ್ಯಾಗ್_ಸೂಚ್ಯಂಕ, &o->wasm, o->wasm_ಕೊನೆ));
    _throwif(m3_Err_type_ಎಣಿಕೆ_Mismatch, type_ಸೂಚ್ಯಂಕ >= o->module->ಅಂಕೆ_Func_Types);
    _throwif(m3_Err_unknown_ಟ್ಯಾಗ್, ಟ್ಯಾಗ್_ಸೂಚ್ಯಂಕ >= o->module->ಅಂಕೆ_ಟ್ಯಾಗುಗಳು);

    IM3FuncType ಗುರಿ_Cont_Type = o->module->funcTypes[type_ಸೂಚ್ಯಂಕ];
    IM3_ಟ್ಯಾಗ್      ಟ್ಯಾಗ್            = &o->module->ಟ್ಯಾಗುಗಳು[ಟ್ಯಾಗ್_ಸೂಚ್ಯಂಕ];
    ಆದರೆ (ಟ್ಯಾಗ್->resolved) {
        ಟ್ಯಾಗ್ = ಟ್ಯಾಗ್->resolved;
    }

    _throwif("type is not a continuation", not ಗುರಿ_Cont_Type->isContinuation);

    // a switch tag carries no payload of its own: the values travel as the
    // peer's arguments, and the tag's results are what the switch site yields
    _throwif(m3Err_typeMismatch, Get_Func_Type_ಅಂಕೆ_Params(ಟ್ಯಾಗ್->type) != 0);

    // switch $ct1 : [t1* (ref null $ct1)] -> [t2*]
    //   where $ct1 = cont [t1* (ref null? $ct2)] -> [..]
    //   and   $ct2 = cont [t2*] -> [..]
    //
    // The trailing parameter of $ct1 is not an operand: the peer receives the
    // continuation this instruction suspends. And what this instruction yields
    // is not $ct1's results but $ct2's parameters - the values whoever comes
    // back this way will hand over.
    IM3FuncType ಗುರಿ_Type = ಗುರಿ_Cont_Type->contFuncType;
    u16         ಅಂಕೆ_Params  = Get_Func_Type_ಅಂಕೆ_Params(ಗುರಿ_Type);

    _throwif("switch target takes no continuation", ಅಂಕೆ_Params == 0);

    m3type_t    peerRefType  = GetFuncTypeParamType(ಗುರಿ_Type, (u16)(ಅಂಕೆ_Params - 1));
    IM3FuncType selfContType = Module_Cont_Type_ದ_Ref(o->module, peerRefType);

    _throwif(m3Err_typeMismatch, not selfContType);

    IM3FuncType selfType   = selfContType->contFuncType;
    u16         ಅಂಕೆ_ವಾದಗಳು    = (u16)(ಅಂಕೆ_Params - 1);
    u16         ಅಂಕೆ_Results = Get_Func_Type_ಅಂಕೆ_Params(selfType);

    // the operands are read out of slots, so nothing may still be in a register
_   (PreserveRegisters(o));

    m3type_t cont_ತುದಿ_Type = Get_ರಾಶಿ_ತುದಿ_Type(o);
    _throwif(m3Err_typeMismatch, not Is_Sub_Type_ದ(cont_ತುದಿ_Type, Ref_Type_ದ_Func_Type(ಗುರಿ_Cont_Type, false)));
    u16 ಗುರಿ_Cont_ಸ್ಥಾನ = Get_ರಾಶಿ_ತುದಿ_ಸ್ಥಾನ_ಸಂಖ್ಯೆ(o);
_   (ತೆಗೆ(o));

    u16 argSlots[d_m3MaxContinuationPayload];
    _throwif("too many switch params", ಅಂಕೆ_ವಾದಗಳು > d_m3MaxContinuationPayload);

    ಗಾಗಿ (u16 i = 0; i < ಅಂಕೆ_ವಾದಗಳು; ++i) {
        u16      pIdx         = (u16)(ಅಂಕೆ_ವಾದಗಳು - 1 - i);
        m3type_t expectedType = GetFuncTypeParamType(ಗುರಿ_Type, pIdx);
        m3type_t actualType   = Get_ರಾಶಿ_ತುದಿ_Type(o);
        _throwif(m3Err_typeMismatch, not Is_Sub_Type_ದ(actualType, expectedType));
        argSlots[pIdx] = Get_ರಾಶಿ_ತುದಿ_ಸ್ಥಾನ_ಸಂಖ್ಯೆ(o);
_       (ತೆಗೆ(o));
    }

    u16 ಫಲಿತಾಂಶ_Slots[d_m3MaxContinuationPayload];
    _throwif("too many switch results", ಅಂಕೆ_Results > d_m3MaxContinuationPayload);

    ಗಾಗಿ (u16 i = 0; i < ಅಂಕೆ_Results; ++i) {
_       (ತಳ್ಳು_Allocated_ಸ್ಥಾನ(o, GetFuncTypeParamType(selfType, i)));
        ಫಲಿತಾಂಶ_Slots[i] = Get_ರಾಶಿ_ತುದಿ_ಸ್ಥಾನ_ಸಂಖ್ಯೆ(o);
    }

_   (Ensure_ಸಂಕೇತ_ಪುಟ_ಅಂಕೆ_ಸಾಲುಗಳು(o, 6 + ಅಂಕೆ_ವಾದಗಳು * 2 + ಅಂಕೆ_Results * 2 + d_m3_ಸಂಕೇತ_ಪುಟ_Free_ಸಾಲುಗಳು_ಮಿತಿ));
_   (EmitOp(o, op_Switch));
    Emit_ಸೂಚಕ(o, ಗುರಿ_Cont_Type);
    Emit_ಸೂಚಕ(o, ಟ್ಯಾಗ್);
    Emit_ಸ್ಥಾನ_ಸ್ಥಾನಾಂತರ(o, ಗುರಿ_Cont_ಸ್ಥಾನ);
    EmitConstant32(o, ಅಂಕೆ_ವಾದಗಳು);
    ಗಾಗಿ (u16 i = 0; i < ಅಂಕೆ_ವಾದಗಳು; ++i) {
        Emit_ಸ್ಥಾನ_ಸ್ಥಾನಾಂತರ(o, argSlots[i]);
        EmitConstant32(o, Is64_ಬಿಟ್_Type(GetFuncTypeParamType(ಗುರಿ_Type, i)));
    }
    EmitConstant32(o, ಅಂಕೆ_Results);
    ಗಾಗಿ (u16 i = 0; i < ಅಂಕೆ_Results; ++i) {
        Emit_ಸ್ಥಾನ_ಸ್ಥಾನಾಂತರ(o, ಫಲಿತಾಂಶ_Slots[i]);
        EmitConstant32(o, Is64_ಬಿಟ್_Type(GetFuncTypeParamType(selfType, i)));
    }

    // as with suspend: op_Switch clears the results before it leaves
_   (ದಾಖಲೆ_ಸುರಕ್ಷಿತ_ಬಿಂದು(o, safepoint_suspend, 0, ಅಂಕೆ_Results, 0, ಖಾಲಿ));

} _catch: ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}

#ಆದರೆ_ಅಂತ್ಯ // d_m3HasStackSwitching


ಸ್ಥಿರ
M3_ಫಲಿತಾಂಶ Compile_Else_ಖಂಡ (IM3Compilation o, pc_t* o_ಆರಂಭ_PC, IM3FuncType i_ಖಂಡ_Type)
{
    IM3_ಸಂಕೇತ_ಪುಟ saved_ಪುಟ = o->ಪುಟ;
_try {

    IM3_ಸಂಕೇತ_ಪುಟ else_ಪುಟ;
_   (Acquire_Compilation_ಸಂಕೇತ_ಪುಟ(o, &else_ಪುಟ));

    *o_ಆರಂಭ_PC = Get_ಪುಟ_PC(else_ಪುಟ);

    o->ಪುಟ = else_ಪುಟ;

_   (Compile_ಖಂಡ(o, i_ಖಂಡ_Type, c_waOp_else));

_   (EmitOp(o, op_Branch));
    Emit_ಸೂಚಕ(o, Get_ಪುಟ_PC(saved_ಪುಟ));
} _catch:

    ಆದರೆ (o->ಪುಟ != saved_ಪುಟ) {
        Release_Compilation_ಸಂಕೇತ_ಪುಟ(o);
    }

    o->ಪುಟ = saved_ಪುಟ;

    ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}

ಸ್ಥಿರ
M3_ಫಲಿತಾಂಶ Compile_If (IM3Compilation o, m3opcode_t i_opcode)
{
    /*      [   op_If   ]
            [ <else-pc> ]   ---->   [ ..else..  ]
            [  ..if..   ]           [ ..block.. ]
            [ ..block.. ]           [ op_Branch ]
            [    end    ]  <-----   [  <end-pc> ]       */

_try {

    // Spec: if condition must be i32
    ಆದರೆ (not Is_ರಾಶಿ_Polymorphic(o)) {
        m3type_t condType = Get_ರಾಶಿ_ತುದಿ_Type(o);
        _throwif(m3Err_typeMismatch, condType != c_m3Type_none and condType != c_m3Type_i32);
    }

_   (Preserve_Non_ತುದಿ_Registers(o));
_   (Preserve_ವಾದಗಳು_And_Locals(o));

#ಆದರೆ d_m3FuseBranch
    IM3Operation fuseOp = GetBranchFusionOp(o, true);

    ಆದರೆ (fuseOp) {
        // absorb the if into the compare that produced the condition
        *(IM3Operation*)o->foldPatchPC = fuseOp;
        InvalidateFold(o);

_       (ತೆಗೆ(o));                          // the condition is consumed inside the fused op
    } ಇಲ್ಲದಿದ್ದರೆ
#ಆದರೆ_ಅಂತ್ಯ
    {
        IM3Operation op = Is_ರಾಶಿ_ತುದಿ_ಒಳ_ರಿಜಿಸ್ಟರ್(o) ? op_If_r : op_If_s;

_       (EmitOp(o, op));
_       (Emit_ಸ್ಥಾನ_ಅಂಕೆ_ದ_ರಾಶಿ_ತುದಿ_And_ತೆಗೆ(o));
    }

    pc_t* pc = (pc_t*)Reserve_ಸೂಚಕ(o);

    IM3FuncType ಖಂಡ_Type = ಖಾಲಿ;
_   (Read_ಖಂಡ_Type(o, &ಖಂಡ_Type));     //  dump_type_stack (o);

    u16 ರಾಶಿ_ಸೂಚ್ಯಂಕ = o->ರಾಶಿ_ಸೂಚ್ಯಂಕ;

_   (Compile_ಖಂಡ(o, ಖಂಡ_Type, i_opcode));

    ಆದರೆ (o->ಹಿಂದಿನ_Opcode == c_waOp_else) {
        o->ರಾಶಿ_ಸೂಚ್ಯಂಕ = ರಾಶಿ_ಸೂಚ್ಯಂಕ;
_       (Compile_Else_ಖಂಡ(o, pc, ಖಂಡ_Type));
    } ಇಲ್ಲದಿದ್ದರೆ {
        // if block produces values and there isn't a defined else
        // case, then we need to make one up so that the pass-through
        // results end up in the right place
        ಆದರೆ (Get_Func_Type_ಅಂಕೆ_Results(ಖಂಡ_Type)) {
            // rewind to the if's end to create a fake else block
            o->wasm--;
            o->ರಾಶಿ_ಸೂಚ್ಯಂಕ = ರಾಶಿ_ಸೂಚ್ಯಂಕ;     // dump_type_stack (o);

_           (Compile_Else_ಖಂಡ(o, pc, ಖಂಡ_Type));
        } ಇಲ್ಲದಿದ್ದರೆ {
            *pc = GetPC(o);
        }
    }

    } _catch: ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}

ಸ್ಥಿರ
M3_ಫಲಿತಾಂಶ Compile_Select (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_ಫಲಿತಾಂಶ ಫಲಿತಾಂಶ = m3Err_none;

    u16 slots[3] = { c_ಸ್ಥಾನ_ಬಳಸದ, c_ಸ್ಥಾನ_ಬಳಸದ, c_ಸ್ಥಾನ_ಬಳಸದ };

    IM3Operation op = ಖಾಲಿ;

    m3type_t type = Get_ರಾಶಿ_Type_From_ತುದಿ(o, 1); // get type of selection

    ಆದರೆ (not Is_ರಾಶಿ_Polymorphic(o)) {
        // Spec: the condition operand (top) must be i32
        m3type_t condType = Get_ರಾಶಿ_Type_From_ತುದಿ(o, 0);
        _throwif(m3Err_typeMismatch, condType != c_m3Type_none and condType != c_m3Type_i32);

        // Spec: the two value operands (below condition) must have matching types
        m3type_t type2 = Get_ರಾಶಿ_Type_From_ತುದಿ(o, 2);
        _throwif(m3Err_typeMismatch, type != c_m3Type_none and type2 != c_m3Type_none and
                                       not (Is_Sub_Type_ದ(type, type2) or Is_Sub_Type_ದ(type2, type)));
    }

    ಆದರೆ (IsFpType(type)) {
#ಆದರೆ d_m3HasFloat
        // not consuming a fp reg, so preserve
        ಆದರೆ (not Is_ರಾಶಿ_ತುದಿ_Minus1_ಒಳ_ರಿಜಿಸ್ಟರ್(o) and
            not Is_ರಾಶಿ_ತುದಿ_Minus2_ಒಳ_ರಿಜಿಸ್ಟರ್(o)) {
_           (Preserve_ರಿಜಿಸ್ಟರ್_If_Occupied(o, type));
        }

        bool selector_ಒಳ_Reg = Is_ರಾಶಿ_ತುದಿ_ಒಳ_ರಿಜಿಸ್ಟರ್(o);
        slots[0]           = Get_ರಾಶಿ_ತುದಿ_ಸ್ಥಾನ_ಸಂಖ್ಯೆ(o);
_       (ತೆಗೆ(o));

        u32 op_ಸೂಚ್ಯಂಕ = 0;

        ಗಾಗಿ (u32 i = 1; i <= 2; ++i) {
            ಆದರೆ (Is_ರಾಶಿ_ತುದಿ_ಒಳ_ರಿಜಿಸ್ಟರ್(o)) {
                op_ಸೂಚ್ಯಂಕ = i;
            } ಇಲ್ಲದಿದ್ದರೆ {
                slots[i] = Get_ರಾಶಿ_ತುದಿ_ಸ್ಥಾನ_ಸಂಖ್ಯೆ(o);
            }

_           (ತೆಗೆ(o));
        }

        op = c_fpSelectOps[type - c_m3Type_f32][selector_ಒಳ_Reg][op_ಸೂಚ್ಯಂಕ];
#ಇಲ್ಲದಿದ್ದರೆ
        _throw(m3Err_unknownOpcode);
#ಆದರೆ_ಅಂತ್ಯ
    } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (IsIntType(type) or IsRefType(type)) {
        // 'sss' operation doesn't consume a register, so might have to protected its contents
        ಆದರೆ (not Is_ರಾಶಿ_ತುದಿ_ಒಳ_ರಿಜಿಸ್ಟರ್(o) and
            not Is_ರಾಶಿ_ತುದಿ_Minus1_ಒಳ_ರಿಜಿಸ್ಟರ್(o) and
            not Is_ರಾಶಿ_ತುದಿ_Minus2_ಒಳ_ರಿಜಿಸ್ಟರ್(o)) {
_           (Preserve_ರಿಜಿಸ್ಟರ್_If_Occupied(o, type));
        }

        u32 op_ಸೂಚ್ಯಂಕ = 3;  // op_Select_*_sss

        ಗಾಗಿ (u32 i = 0; i < 3; ++i) {
            ಆದರೆ (Is_ರಾಶಿ_ತುದಿ_ಒಳ_ರಿಜಿಸ್ಟರ್(o)) {
                op_ಸೂಚ್ಯಂಕ = i;
            } ಇಲ್ಲದಿದ್ದರೆ {
                slots[i] = Get_ರಾಶಿ_ತುದಿ_ಸ್ಥಾನ_ಸಂಖ್ಯೆ(o);
            }

_           (ತೆಗೆ(o));
        }

        // a reference is a pointer-sized integer as far as select is concerned
        u32 type_ಸೂಚ್ಯಂಕ = IsRefType(type) ? ((M3_SIZEOF_PTR == 8) ? 1 : 0)
                                        : (u32)(type - c_m3Type_i32);

        op = c_intSelectOps[type_ಸೂಚ್ಯಂಕ][op_ಸೂಚ್ಯಂಕ];
    } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (not Is_ರಾಶಿ_Polymorphic(o)) {
        _throw(m3_Err_function_ರಾಶಿ_Underrun);
    }

    EmitOp(o, op);
    ಗಾಗಿ (u32 i = 0; i < 3; i++) {
        ಆದರೆ (Is_Valid_ಸ್ಥಾನ(slots[i])) {
            Emit_ಸ್ಥಾನ_ಸ್ಥಾನಾಂತರ(o, slots[i]);
        }
    }
_   (ತಳ್ಳು_ರಿಜಿಸ್ಟರ್(o, type));

    _catch: ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}

ಸ್ಥಿರ
M3_ಫಲಿತಾಂಶ Compile_Drop (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_ಫಲಿತಾಂಶ ಫಲಿತಾಂಶ = ತೆಗೆ(o);
    ಆದರೆ (d_m3_Log_Wasm_ರಾಶಿ) {
        dump_type_ರಾಶಿ(o);
    }
    ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}

ಸ್ಥಿರ
M3_ಫಲಿತಾಂಶ Compile_Nop (IM3Compilation o, m3opcode_t i_opcode)
{
    ಹಿಂತಿರುಗಿ m3Err_none;
}

ಸ್ಥಿರ
M3_ಫಲಿತಾಂಶ Compile_Unreachable (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_ಫಲಿತಾಂಶ ಫಲಿತಾಂಶ;

_   (ಕೂಡಿಸು_Trap_ದಾಖಲೆ(o));

_   (EmitOp(o, op_Unreachable));
_   (ಗಣ_ರಾಶಿ_Polymorphic(o));

    _catch:
    ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}


// OPTZ: currently all stack slot indices take up a full word, but
// dual stack source operands could be packed together
ಸ್ಥಿರ
M3_ಫಲಿತಾಂಶ Compile_Operator (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_ಫಲಿತಾಂಶ ಫಲಿತಾಂಶ;

    // Declared before the first throw below, which jumps past this point to
    // _catch and so must not skip an initialization.
    IM3Operation op = ಖಾಲಿ;

    IM3OpInfo opInfo = GetOpInfo(i_opcode);
    _throwif(m3Err_unknownOpcode, not opInfo);

    // Spec: validate operand types for load/store operations
    ಆದರೆ (not Is_ರಾಶಿ_Polymorphic(o)) {
        // For load ops (stackOffset == 0, unary), the operand is always i32 (address)
        ಆದರೆ (i_opcode >= 0x28 and i_opcode <= 0x35) {
            m3type_t ತುದಿ_Type = Get_ರಾಶಿ_ತುದಿ_Type(o);
            _throwif(m3Err_typeMismatch, ತುದಿ_Type != c_m3Type_none and ತುದಿ_Type != c_m3Type_i32);
        }

        // For store ops (stackOffset == -2), the address operand must be i32
        ಆದರೆ (i_opcode >= 0x36 and i_opcode <= 0x3e) {
            m3type_t addrType = Get_ರಾಶಿ_Type_From_ತುದಿ(o, 1);
            _throwif(m3Err_typeMismatch, addrType != c_m3Type_none and addrType != c_m3Type_i32);
        }
    }

    // This preserve is for for FP compare operations.
    // either need additional slot destination operations or the
    // easy fix, move _r0 out of the way.
    // moving out the way might be the optimal solution most often?
    // otherwise, the _r0 reg can get buried down in the stack
    // and be idle & wasted for a moment.
    ಆದರೆ (M3_UNLIKELY(IsFpType(Get_ರಾಶಿ_ತುದಿ_Type(o)) and IsIntType(opInfo->type))) {
_       (Preserve_ರಿಜಿಸ್ಟರ್_If_Occupied(o, opInfo->type));
    }

    ಆದರೆ (opInfo->ರಾಶಿ_ಸ್ಥಾನಾಂತರ == 0) {
        ಆದರೆ (Is_ರಾಶಿ_ತುದಿ_ಒಳ_ರಿಜಿಸ್ಟರ್(o)) {
            op = opInfo->operations[0]; // _s
        } ಇಲ್ಲದಿದ್ದರೆ {
_           (Preserve_ರಿಜಿಸ್ಟರ್_If_Occupied(o, opInfo->type));
            op = opInfo->operations[1]; // _r
        }
    } ಇಲ್ಲದಿದ್ದರೆ {
        ಆದರೆ (Is_ರಾಶಿ_ತುದಿ_ಒಳ_ರಿಜಿಸ್ಟರ್(o)) {
            op = opInfo->operations[0];  // _rs

            ಆದರೆ (Is_ರಾಶಿ_ತುದಿ_Minus1_ಒಳ_ರಿಜಿಸ್ಟರ್(o)) {    d_m3Assert (i_opcode == c_waOp_store_f32 or i_opcode == c_waOp_store_f64);
                op = opInfo->operations[3]; // _rr for fp.store
            }
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (Is_ರಾಶಿ_ತುದಿ_Minus1_ಒಳ_ರಿಜಿಸ್ಟರ್(o)) {
            op = opInfo->operations[1]; // _sr

            ಆದರೆ (not op) {  // must be commutative, then
                op = opInfo->operations[0];
            }
        } ಇಲ್ಲದಿದ್ದರೆ {
_           (Preserve_ರಿಜಿಸ್ಟರ್_If_Occupied(o, opInfo->type));     // _ss
            op = opInfo->operations[2];
        }
    }

    ಆದರೆ (M3_LIKELY(op)) {
_       (EmitOp(o, op));

#ಆದರೆ d_m3_Fold_ಗಣ_Local
        // the op word just written (EmitOp may have bridged pages); NULL in the page-less compile-walk mode
        pc_t patchPC = o->ಪುಟ ? GetPC(o) - 1 : ಖಾಲಿ;
#ಆದರೆ_ಅಂತ್ಯ

_       (Emit_ಸ್ಥಾನ_ಅಂಕೆ_ದ_ರಾಶಿ_ತುದಿ_And_ತೆಗೆ(o));

        ಆದರೆ (opInfo->ರಾಶಿ_ಸ್ಥಾನಾಂತರ < 0) {
_           (Emit_ಸ್ಥಾನ_ಅಂಕೆ_ದ_ರಾಶಿ_ತುದಿ_And_ತೆಗೆ(o));
        }

        ಆದರೆ (opInfo->type != c_m3Type_none) {
_           (ತಳ್ಳು_ರಿಜಿಸ್ಟರ್(o, opInfo->type));

#ಆದರೆ d_m3_Fold_ಗಣ_Local
            // record this op as a fold candidate for an immediately following local.set;
            // the operand-form index is recovered by matching against the variant table
            ಆದರೆ (patchPC) {
                ಗಾಗಿ (u8 form = 0; form < 4; ++form) {
                    ಆದರೆ (opInfo->operations[form] == op) {
                        o->foldPatchPC    = patchPC;
                        o->foldOpcode     = i_opcode;
                        o->foldForm       = form;
                        o->fold_ರಾಶಿ_ಸೂಚ್ಯಂಕ = (u16)Get_ರಾಶಿ_ತುದಿ_ಸೂಚ್ಯಂಕ(o);
                        ನಿಲ್ಲಿಸು;
                    }
                }
            }
#ಆದರೆ_ಅಂತ್ಯ
        }
    } ಇಲ್ಲದಿದ್ದರೆ {
#ವ್ಯಾಖ್ಯಾನಿತವಾದರೆ DEBUG
        ಫಲಿತಾಂಶ = ErrorCompile("no operation found for opcode", o, "'%s'", opInfo->name);
#ಇಲ್ಲದಿದ್ದರೆ
        ಫಲಿತಾಂಶ = ErrorCompile("no operation found for opcode", o, "%x", i_opcode);
#ಆದರೆ_ಅಂತ್ಯ
        _throw(ಫಲಿತಾಂಶ);
    }

    _catch: ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}

ಸ್ಥಿರ
M3_ಫಲಿತಾಂಶ Compile_Convert (IM3Compilation o, m3opcode_t i_opcode)
{
_try {
    IM3OpInfo opInfo = GetOpInfo(i_opcode);
    _throwif(m3Err_unknownOpcode, not opInfo);

    // Spec: validate source operand type for conversion instructions
    ಆದರೆ (not Is_ರಾಶಿ_Polymorphic(o)) {
        u8 ಮೂಲ_Type = c_m3Type_none;
        ಆಯ್ಕೆ (i_opcode)
        // clang-format off
        {
        ಸ್ಥಿತಿ 0xa7:              // i32.wrap/i64
            ಮೂಲ_Type = c_m3Type_i64; ನಿಲ್ಲಿಸು;
        ಸ್ಥಿತಿ 0xa8: ಸ್ಥಿತಿ 0xa9:   // i32.trunc_s/f32, i32.trunc_u/f32
        ಸ್ಥಿತಿ 0xae: ಸ್ಥಿತಿ 0xaf:   // i64.trunc_s/f32, i64.trunc_u/f32
        ಸ್ಥಿತಿ 0xbb:              // f64.promote/f32
        ಸ್ಥಿತಿ 0xbc:              // i32.reinterpret/f32
            ಮೂಲ_Type = c_m3Type_f32; ನಿಲ್ಲಿಸು;
        ಸ್ಥಿತಿ 0xaa: ಸ್ಥಿತಿ 0xab:   // i32.trunc_s/f64, i32.trunc_u/f64
        ಸ್ಥಿತಿ 0xb0: ಸ್ಥಿತಿ 0xb1:   // i64.trunc_s/f64, i64.trunc_u/f64
        ಸ್ಥಿತಿ 0xb6:              // f32.demote/f64
        ಸ್ಥಿತಿ 0xbd:              // i64.reinterpret/f64
            ಮೂಲ_Type = c_m3Type_f64; ನಿಲ್ಲಿಸು;
        ಸ್ಥಿತಿ 0xac: ಸ್ಥಿತಿ 0xad:   // i64.extend_s/i32, i64.extend_u/i32
        ಸ್ಥಿತಿ 0xb2: ಸ್ಥಿತಿ 0xb3:   // f32.convert_s/i32, f32.convert_u/i32
        ಸ್ಥಿತಿ 0xb7: ಸ್ಥಿತಿ 0xb8:   // f64.convert_s/i32, f64.convert_u/i32
        ಸ್ಥಿತಿ 0xbe:              // f32.reinterpret/i32
            ಮೂಲ_Type = c_m3Type_i32; ನಿಲ್ಲಿಸು;
        ಸ್ಥಿತಿ 0xb4: ಸ್ಥಿತಿ 0xb5:   // f32.convert_s/i64, f32.convert_u/i64
        ಸ್ಥಿತಿ 0xb9: ಸ್ಥಿತಿ 0xba:   // f64.convert_s/i64, f64.convert_u/i64
        ಸ್ಥಿತಿ 0xbf:              // f64.reinterpret/i64
            ಮೂಲ_Type = c_m3Type_i64; ನಿಲ್ಲಿಸು;
        ಪೂರ್ವನಿಯೋಜಿತ: ನಿಲ್ಲಿಸು;
        }
        // clang-format on

        ಆದರೆ (ಮೂಲ_Type != c_m3Type_none) {
            m3type_t ತುದಿ_Type = Get_ರಾಶಿ_ತುದಿ_Type(o);
            _throwif(m3Err_typeMismatch, ತುದಿ_Type != c_m3Type_none and not Is_Sub_Type_ದ(ತುದಿ_Type, ಮೂಲ_Type));
        }
    }

    bool dest_ಒಳ_ಸ್ಥಾನ   = Is_ರಿಜಿಸ್ಟರ್_Type_Allocated(o, opInfo->type);
    bool ಮೂಲ_ಒಳ_ಸ್ಥಾನ = Is_ರಾಶಿ_ತುದಿ_ಒಳ_ಸ್ಥಾನ(o);

    IM3Operation op = opInfo->operations[dest_ಒಳ_ಸ್ಥಾನ * 2 + ಮೂಲ_ಒಳ_ಸ್ಥಾನ];

_   (EmitOp(o, op));
_   (Emit_ಸ್ಥಾನ_ಅಂಕೆ_ದ_ರಾಶಿ_ತುದಿ_And_ತೆಗೆ(o));

    ಆದರೆ (dest_ಒಳ_ಸ್ಥಾನ) {
_       (ತಳ್ಳು_Allocated_ಸ್ಥಾನ_And_Emit_2(o, opInfo->type))
    } ಇಲ್ಲದಿದ್ದರೆ {
_       (ತಳ್ಳು_ರಿಜಿಸ್ಟರ್(o, opInfo->type))
    }
}
    _catch: ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}

#ಆದರೆ d_m3HasWideArithmetic

// The wide arithmetic proposal: i64.add128 and i64.sub128 over two 128-bit values
// given as i64 halves, and i64.mul_wide_s/u giving the full product of two i64s.
// Each gives back a 128-bit result the same way, low half first. The operands are
// all read from slots, and of the two results the high half, which ends up on top,
// goes to _r0 and the low half to a newly allocated slot - so every operation has
// the one form, whatever shape the stack was in.
ಸ್ಥಿರ
M3_ಫಲಿತಾಂಶ Compile_WideArithmetic (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_ಫಲಿತಾಂಶ ಫಲಿತಾಂಶ = m3Err_none;

    IM3Operation op          = op_i64_Add128;
    u32          ಅಂಕೆ_Operands = 4;

    ಆಯ್ಕೆ (i_opcode)
    // clang-format off
    {
    ಸ್ಥಿತಿ c_waOp_i64Sub128:   op = op_i64_Sub128;                          ನಿಲ್ಲಿಸು;
    ಸ್ಥಿತಿ c_waOp_i64MulWideS: op = op_i64_MultiplyWide; ಅಂಕೆ_Operands = 2;   ನಿಲ್ಲಿಸು;
    ಸ್ಥಿತಿ c_waOp_i64MulWideU: op = op_u64_MultiplyWide; ಅಂಕೆ_Operands = 2;   ನಿಲ್ಲಿಸು;
    ಪೂರ್ವನಿಯೋಜಿತ: ನಿಲ್ಲಿಸು;
    }
    // clang-format on

_   (Preserve_ರಿಜಿಸ್ಟರ್_If_Occupied(o, c_m3Type_i64));

_   (EmitOp(o, op));

    // the top of the stack first: the high half of the second operand, for the
    // 128-bit ones
    ಗಾಗಿ (u32 i = 0; i < ಅಂಕೆ_Operands; ++i) {
_       (Emit_ಸ್ಥಾನ_ಅಂಕೆ_ದ_ರಾಶಿ_ತುದಿ_And_ತೆಗೆ(o));
    }

_   (ತಳ್ಳು_Allocated_ಸ್ಥಾನ_And_Emit_2(o, c_m3Type_i64));      // the low half
_   (ತಳ್ಳು_ರಿಜಿಸ್ಟರ್(o, c_m3Type_i64));                  // the high half

    _catch: ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}

#ಆದರೆ_ಅಂತ್ಯ // d_m3HasWideArithmetic

#ಆದರೆ d_m3HasMemory64

// Replaces the 64-bit address operand of a load or store with the checked
// 32-bit effective address op_CheckAddr64 leaves behind, so that the access
// itself compiles exactly as it would against a 32-bit memory: one slot to read
// the address from, and an offset of zero folded in already.
//
// Where the address sits under other operands - the value of a store, the
// operands of an atomic - it is rewritten where it stands rather than popped:
// its stack entry is repointed at the scratch slot and retyped, and the slot it
// used to occupy released. i_numOperands counts the address and all above it.
ಸ್ಥಿರ
M3_ಫಲಿತಾಂಶ EmitCheckAddr64 (IM3Compilation o, u64 i_ಸ್ಥಾನಾಂತರ, u16 i_ಅಂಕೆ_Operands)
{
    M3_ಫಲಿತಾಂಶ ಫಲಿತಾಂಶ = m3Err_none;

    // The address has to be in a slot for op_CheckAddr64 to read it. Spilling
    // r0 wholesale also keeps the value of a store out of it, which is what
    // leaves the address as the one operand still in a register to worry about.
_   (Preserve_ರಿಜಿಸ್ಟರ್_If_Occupied(o, c_m3Type_i64));

    // unreachable code: the operands the instruction names may not be there
    ಆದರೆ (o->ರಾಶಿ_ಸೂಚ್ಯಂಕ < o->ಖಂಡ.ಖಂಡ_ರಾಶಿ_ಸೂಚ್ಯಂಕ + i_ಅಂಕೆ_Operands) {
        ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
    }

    {
        u16      ರಾಶಿ_ಸೂಚ್ಯಂಕ = (u16)(o->ರಾಶಿ_ಸೂಚ್ಯಂಕ - i_ಅಂಕೆ_Operands);
        u16      src_ಸ್ಥಾನ    = o->wasm_ರಾಶಿ[ರಾಶಿ_ಸೂಚ್ಯಂಕ];
        m3type_t srcType    = o->type_ರಾಶಿ[ರಾಶಿ_ಸೂಚ್ಯಂಕ];

        u16 dst_ಸ್ಥಾನ = c_ಸ್ಥಾನ_ಬಳಸದ;

        // Checked here rather than left to the validator, which a build may
        // have switched off: reading a slot as an i64 that only holds an i32
        // would reach past what was allocated for it.
        _throwif(m3Err_typeMismatch, ತಳ_Type_ದ(srcType) != c_m3Type_i64);

        // an address is an integer, so nothing should be left in a register
        // once r0 has been preserved
        _throwif(m3Err_typeMismatch, Is_ರಿಜಿಸ್ಟರ್_ಸ್ಥಾನ_Alias(src_ಸ್ಥಾನ));
        _throwif(m3_Err_function_ರಾಶಿ_Underrun, src_ಸ್ಥಾನ >= o->ಸ್ಥಾನ_Max_Allocated_ಸೂಚ್ಯಂಕ_Plus_One);

        // allocated before the source is released, so the two cannot be the
        // same slot - op_CheckAddr64 reads one and writes the other
_       (AllocateSlots(o, &dst_ಸ್ಥಾನ, c_m3Type_i32));

_       (EmitOp(o, op_CheckAddr64));
        Emit_ಸ್ಥಾನ_ಸ್ಥಾನಾಂತರ(o, src_ಸ್ಥಾನ);
        Emit_ಸ್ಥಾನ_ಸ್ಥಾನಾಂತರ(o, dst_ಸ್ಥಾನ);
        EmitConstant64(o, i_ಸ್ಥಾನಾಂತರ);

        o->wasm_ರಾಶಿ[ರಾಶಿ_ಸೂಚ್ಯಂಕ] = dst_ಸ್ಥಾನ;
        o->type_ರಾಶಿ[ರಾಶಿ_ಸೂಚ್ಯಂಕ] = c_m3Type_i32;

        // locals and the constant table outlive the instruction; a dynamic
        // slot the address was computed into does not
        ಆದರೆ (src_ಸ್ಥಾನ >= o->ಸ್ಥಾನ_ಮೊದಲ_Dynamic_ಸೂಚ್ಯಂಕ) {
            Deallocate_ಸ್ಥಾನ(o, src_ಸ್ಥಾನ, srcType);
        }
    }

    _catch: ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}

#ಆದರೆ_ಅಂತ್ಯ // d_m3HasMemory64


ಸ್ಥಿರ
M3_ಫಲಿತಾಂಶ Compile_ತುಂಬು_Store (IM3Compilation o, m3opcode_t i_opcode)
{
_try {
    u32 alignHint, ಸ್ಮರಣೆ_Idx;
    u64 ಸ್ಮರಣೆ_ಸ್ಥಾನಾಂತರ;

    // alignHint is checked by the validator
_   (Read_ಸ್ಮರಣೆ_Arg(&alignHint, &ಸ್ಮರಣೆ_Idx, &ಸ್ಮರಣೆ_ಸ್ಥಾನಾಂತರ, &o->wasm, o->wasm_ಕೊನೆ));
                                                                        m3log (compile, d_indent " (memory = %d; offset = %llu)", get_indention_string (o), ಸ್ಮರಣೆ_Idx, (ಚಿಹ್ನೆರಹಿತ ದೀರ್ಘ ದೀರ್ಘ) ಸ್ಮರಣೆ_ಸ್ಥಾನಾಂತರ);
    _throwif(m3_Err_unknown_ಸ್ಮರಣೆ, ಸ್ಮರಣೆ_Idx >= o->module->ಅಂಕೆ_Memories);

    IM3OpInfo opInfo = GetOpInfo(i_opcode);
    _throwif(m3Err_unknownOpcode, not opInfo);

    bool isMemory64 = o->module->memories[ಸ್ಮರಣೆ_Idx]->isMemory64;

    // Spec: the static offset has to be in range of the address type. Checked
    // here as well as in the validator, so that a build without one still
    // refuses the module rather than silently truncating the offset.
    _throwif(m3Err_wasmMalformed, not isMemory64 and ಸ್ಮರಣೆ_ಸ್ಥಾನಾಂತರ > 0xFFFFFFFFull);

    ಆದರೆ (IsFpType(opInfo->type)) {
_       (Preserve_ರಿಜಿಸ್ಟರ್_If_Occupied(o, c_m3Type_f64));
    }

    ಆದರೆ (ಸ್ಮರಣೆ_Idx) {
_       (Emit_ಗಣ_ಸ್ಮರಣೆ(o, ಸ್ಮರಣೆ_Idx));
    }

#ಆದರೆ d_m3HasMemory64
    ಆದರೆ (isMemory64) {
        // An offset this far out puts every address out of bounds, so rather
        // than carrying it, clamp it to the limit: op_CheckAddr64 then traps
        // on whatever address it is given, which is the outcome either way.
        ಆದರೆ (ಸ್ಮರಣೆ_ಸ್ಥಾನಾಂತರ > d_m3_ವಿಳಾಸ_ಎಲ್ಲೆ) {
            ಸ್ಮರಣೆ_ಸ್ಥಾನಾಂತರ = d_m3_ವಿಳಾಸ_ಎಲ್ಲೆ;
        }

_       (EmitCheckAddr64(o, ಸ್ಮರಣೆ_ಸ್ಥಾನಾಂತರ, opInfo->ರಾಶಿ_ಸ್ಥಾನಾಂತರ < 0 ? 2 : 1));

        // op_CheckAddr64 has folded the offset in already
        ಸ್ಮರಣೆ_ಸ್ಥಾನಾಂತರ = 0;
    }
#ಆದರೆ_ಅಂತ್ಯ

_   (Compile_Operator(o, i_opcode));

    EmitConstant32(o, (u32)ಸ್ಮರಣೆ_ಸ್ಥಾನಾಂತರ);

    ಆದರೆ (ಸ್ಮರಣೆ_Idx) {
_       (Emit_ಗಣ_ಸ್ಮರಣೆ(o, 0));
    }
}
    _catch: ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}


#ಆದರೆ d_m3HasAtomics

// What an atomic instruction takes and gives back, from its sub-opcode. The
// accesses come in runs of seven, one per combination of value type and cell width:
// i32/32, i64/64, i32/8, i32/16, i64/8, i64/16, i64/32. Those runs are told apart by
// where they start.
//
// The operand count includes the address. The top operand is the one that ends up
// in a register, so its type is the one to check on the way out; a store has no
// result, which is c_m3Type_none.
ಸ್ಥಿರ
ಶೂನ್ಯ GetAtomicShape (u32 i_sub, u16* o_ಅಂಕೆ_Operands, m3type_t* o_ತುದಿ_Type, m3type_t* o_ಫಲಿತಾಂಶ_Type)
{
    // clang-format off
    ಸ್ಥಿರ ಸ್ಥಿರಾಂಕ m3type_t c_ಮೌಲ್ಯ_Types [7] = {
        c_m3Type_i32, c_m3Type_i64,
        c_m3Type_i32, c_m3Type_i32,
        c_m3Type_i64, c_m3Type_i64, c_m3Type_i64
    };
    // clang-format on

    ಆದರೆ (i_sub == 0x00) {                    // memory.atomic.notify: address, count
        *o_ಅಂಕೆ_Operands = 2;
        *o_ತುದಿ_Type     = c_m3Type_i32;
        *o_ಫಲಿತಾಂಶ_Type  = c_m3Type_i32;
    } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (i_sub <= 0x02) {             // memory.atomic.wait32/64: address, expected, timeout
        *o_ಅಂಕೆ_Operands = 3;
        *o_ತುದಿ_Type     = c_m3Type_i64;
        *o_ಫಲಿತಾಂಶ_Type  = c_m3Type_i32;
    } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (i_sub <= 0x16) {             // loads: address
        *o_ಅಂಕೆ_Operands = 1;
        *o_ತುದಿ_Type     = c_m3Type_i32;
        *o_ಫಲಿತಾಂಶ_Type  = c_ಮೌಲ್ಯ_Types[i_sub - 0x10];
    } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (i_sub <= 0x1d) {             // stores: address, value
        *o_ಅಂಕೆ_Operands = 2;
        *o_ತುದಿ_Type     = c_ಮೌಲ್ಯ_Types[i_sub - 0x17];
        *o_ಫಲಿತಾಂಶ_Type  = c_m3Type_none;
    } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (i_sub <= 0x47) {             // read-modify-write: address, value
        *o_ಅಂಕೆ_Operands = 2;
        *o_ತುದಿ_Type     = c_ಮೌಲ್ಯ_Types[(i_sub - 0x1e) % 7];
        *o_ಫಲಿತಾಂಶ_Type  = *o_ತುದಿ_Type;
    } ಇಲ್ಲದಿದ್ದರೆ {                                // cmpxchg: address, expected, replacement
        *o_ಅಂಕೆ_Operands = 3;
        *o_ತುದಿ_Type     = c_ಮೌಲ್ಯ_Types[i_sub - 0x48];
        *o_ಫಲಿತಾಂಶ_Type  = *o_ತುದಿ_Type;
    }
}


// The description an access takes as its last immediate: see m3_exec_atomic.h
ಸ್ಥಿರ
u32 GetAtomicDesc (u32 i_sub)
{
    // per variant, the log2 of the width of the cell
    ಸ್ಥಿರ ಸ್ಥಿರಾಂಕ u8 c_log2Widths[7] = { 2, 3, 0, 1, 0, 1, 2 };

    u32 variant, ಬಗೆ = 0;

    ಆದರೆ (i_sub <= 0x16) {
        variant = i_sub - 0x10;
    } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (i_sub <= 0x1d) {
        variant = i_sub - 0x17;
    } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (i_sub <= 0x47) {
        variant = (i_sub - 0x1e) % 7;
        ಬಗೆ    = (i_sub - 0x1e) / 7;         // add, sub, and, or, xor, xchg
    } ಇಲ್ಲದಿದ್ದರೆ {
        variant = i_sub - 0x48;
    }

    // variants 1 and 4..6 are the i64 ones
    u32 is64 = (variant == 1 or variant >= 4) ? 1u : 0u;

    ಹಿಂತಿರುಗಿ c_log2Widths[variant] | (is64 << 2) | (ಬಗೆ << 3);
}


// Every atomic access is one op, whose operands are the topmost in _r0 and the rest
// in slots, topmost first. The address is always the deepest, so it is the last slot
// (or, for a load, the register), followed by the memarg offset. The ops check the
// address themselves: for an atomic that includes its alignment, which only the
// effective address can be asked.
ಸ್ಥಿರ
M3_ಫಲಿತಾಂಶ Compile_Atomic (IM3Compilation o, m3opcode_t i_opcode)
{
_try {
    u32 alignHint, ಸ್ಮರಣೆ_Idx;
    u64 ಸ್ಮರಣೆ_ಸ್ಥಾನಾಂತರ;

    u16      ಅಂಕೆ_Operands = 0;
    m3type_t ತುದಿ_Type     = c_m3Type_none;
    m3type_t ಫಲಿತಾಂಶ_Type  = c_m3Type_none;

    ಆದರೆ (i_opcode == c_waOp_atomicFence) {
        // the reserved byte, which the validator has checked to be zero. There
        // is nothing to order between accesses that one thread makes in sequence.
        u8 reserved;
_       (Read_u8(&reserved, &o->wasm, o->wasm_ಕೊನೆ));
    } ಇಲ್ಲದಿದ್ದರೆ {
        // alignHint is checked by the validator
_       (Read_ಸ್ಮರಣೆ_Arg(&alignHint, &ಸ್ಮರಣೆ_Idx, &ಸ್ಮರಣೆ_ಸ್ಥಾನಾಂತರ, &o->wasm, o->wasm_ಕೊನೆ));
                                                                        m3log (compile, d_indent " (memory = %d; offset = %llu)", get_indention_string (o), ಸ್ಮರಣೆ_Idx, (ಚಿಹ್ನೆರಹಿತ ದೀರ್ಘ ದೀರ್ಘ) ಸ್ಮರಣೆ_ಸ್ಥಾನಾಂತರ);
        _throwif(m3_Err_unknown_ಸ್ಮರಣೆ, ಸ್ಮರಣೆ_Idx >= o->module->ಅಂಕೆ_Memories);

        IM3OpInfo opInfo = GetOpInfo(i_opcode);
        _throwif(m3Err_unknownOpcode, not opInfo);

        bool isMemory64 = o->module->memories[ಸ್ಮರಣೆ_Idx]->isMemory64;

        // see Compile_Load_Store
        _throwif(m3Err_wasmMalformed, not isMemory64 and ಸ್ಮರಣೆ_ಸ್ಥಾನಾಂತರ > 0xFFFFFFFFull);

        GetAtomicShape(i_opcode & 0xFF, &ಅಂಕೆ_Operands, &ತುದಿ_Type, &ಫಲಿತಾಂಶ_Type);

        ಆದರೆ (ಸ್ಮರಣೆ_Idx) {
_           (Emit_ಗಣ_ಸ್ಮರಣೆ(o, ಸ್ಮರಣೆ_Idx));
        }

#  ಆದರೆ d_m3HasMemory64
        ಆದರೆ (isMemory64) {
            // see Compile_Load_Store
            ಆದರೆ (ಸ್ಮರಣೆ_ಸ್ಥಾನಾಂತರ > d_m3_ವಿಳಾಸ_ಎಲ್ಲೆ) {
                ಸ್ಮರಣೆ_ಸ್ಥಾನಾಂತರ = d_m3_ವಿಳಾಸ_ಎಲ್ಲೆ;
            }

_           (EmitCheckAddr64(o, ಸ್ಮರಣೆ_ಸ್ಥಾನಾಂತರ, ಅಂಕೆ_Operands));
            ಸ್ಮರಣೆ_ಸ್ಥಾನಾಂತರ = 0;
        }
#  endif

_       (Copy_ರಾಶಿ_ತುದಿ_ಗಮ್ಯ_ರಿಜಿಸ್ಟರ್(o, false));
_       (EmitOp(o, opInfo->operations[0]));
_       (ತೆಗೆ_Type(o, ತುದಿ_Type));

        ಗಾಗಿ (u16 i = 1; i < ಅಂಕೆ_Operands; ++i) {
_           (Emit_ಸ್ಥಾನ_ಅಂಕೆ_ದ_ರಾಶಿ_ತುದಿ_And_ತೆಗೆ(o));
        }

        EmitConstant32(o, (u32)ಸ್ಮರಣೆ_ಸ್ಥಾನಾಂತರ);
        ಆದರೆ (i_opcode >= c_wa_Op_atomic_ಮೊದಲ_Access) {
            EmitConstant32(o, GetAtomicDesc(i_opcode & 0xFF));
        }

        ಆದರೆ (ಸ್ಮರಣೆ_Idx) {
_           (Emit_ಗಣ_ಸ್ಮರಣೆ(o, 0));
        }

        ಆದರೆ (ಫಲಿತಾಂಶ_Type != c_m3Type_none) {
_           (ತಳ್ಳು_ರಿಜಿಸ್ಟರ್(o, ಫಲಿತಾಂಶ_Type));
        }
    }
}
    _catch: ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}

#ಆದರೆ_ಅಂತ್ಯ // d_m3HasAtomics


M3_ಫಲಿತಾಂಶ CompileRawFunction (IM3Module io_module, IM3Function io_function, ಸ್ಥಿರಾಂಕ ಶೂನ್ಯ* i_function, ಸ್ಥಿರಾಂಕ ಶೂನ್ಯ* i_userdata)
{
    d_m3Assert(io_module->runtime);

    IM3_ಸಂಕೇತ_ಪುಟ ಪುಟ = Acquire_ಸಂಕೇತ_ಪುಟ_With_ಸಾಮರ್ಥ್ಯ(io_module->runtime, 4);

    ಆದರೆ (ಪುಟ) {
        io_function->compiled = Get_ಪುಟ_PC(ಪುಟ);
        io_function->module   = io_module;

        // Unless a host module's ABI names a memory (m3_BindImportMemory), a
        // host function addresses memory 0 - what a bare i32 guest pointer means
        io_function->host_ಸ್ಮರಣೆ = io_module->memory0;

        Emit_ಶಬ್ದ(ಪುಟ, op_CallRawFunction);
        Emit_ಶಬ್ದ(ಪುಟ, i_function);
        Emit_ಶಬ್ದ(ಪುಟ, io_function);
        Emit_ಶಬ್ದ(ಪುಟ, i_userdata);

        Release_ಸಂಕೇತ_ಪುಟ(io_module->runtime, ಪುಟ);
        ಹಿಂತಿರುಗಿ m3Err_none;
    } ಇಲ್ಲದಿದ್ದರೆ {
        ಹಿಂತಿರುಗಿ m3_Err_malloc_Failed_ಸಂಕೇತ_ಪುಟ;
    }
}


// A table entry names every field it sets, so the struct can order them for
// packing; the columns follow that order, since M3OP names them positionally.
//
// d_logOp, d_logOp2 macros aren't actually used by the compiler, just codepage decoding (d_m3LogCodePages = 1)
#ವ್ಯಾಖ್ಯಾನಿಸು d_ops(...)                          .operations = { __VA_ARGS__ }

#ವ್ಯಾಖ್ಯಾನಿಸು d_logOp(OP)                         d_ops( op_##OP,                 ಖಾಲಿ,                       ಖಾಲಿ,                       ಖಾಲಿ )
#ವ್ಯಾಖ್ಯಾನಿಸು d_logOp2(OP1,OP2)                   d_ops( op_##OP1,                op_##OP2,                   ಖಾಲಿ,                       ಖಾಲಿ )

#ವ್ಯಾಖ್ಯಾನಿಸು d_ಖಾಲಿಗಳು_Op_List                       d_ops( ಖಾಲಿ,                    ಖಾಲಿ,                       ಖಾಲಿ,                       ಖಾಲಿ )
#ವ್ಯಾಖ್ಯಾನಿಸು d_unaryOpList(TYPE, NAME)           d_ops( op_##TYPE##_##NAME##_r,  op_##TYPE##_##NAME##_s,     ಖಾಲಿ,                       ಖಾಲಿ )
#ವ್ಯಾಖ್ಯಾನಿಸು d_binOpList(TYPE, NAME)             d_ops( op_##TYPE##_##NAME##_rs, op_##TYPE##_##NAME##_sr,    op_##TYPE##_##NAME##_ss,    ಖಾಲಿ )
#ವ್ಯಾಖ್ಯಾನಿಸು d_storeFpOpList(TYPE, NAME)         d_ops( op_##TYPE##_##NAME##_rs, op_##TYPE##_##NAME##_sr,    op_##TYPE##_##NAME##_ss,    op_##TYPE##_##NAME##_rr )
#ವ್ಯಾಖ್ಯಾನಿಸು d_commutativeBinOpList(TYPE, NAME)  d_ops( op_##TYPE##_##NAME##_rs, ಖಾಲಿ,                       op_##TYPE##_##NAME##_ss,    ಖಾಲಿ )

#ವ್ಯಾಖ್ಯಾನಿಸು d_convertOpList(OP)                 d_ops( op_##OP##_r_r,           op_##OP##_r_s,              op_##OP##_s_r,              op_##OP##_s_s )


// Every M3Compiler the tables below name, as one list, so that the index enum and
// the array of pointers can never drift apart. Each entry must carry the same
// config guard as the function's definition.
#ವ್ಯಾಖ್ಯಾನಿಸು d_m3CompilerList(_)             \
    _( Compile_Unreachable )            \
    _( Compile_Nop )                    \
    _( Compile_Loop_Or_ಖಂಡ )            \
    _( Compile_If )                     \
    _( Compile_ಕೊನೆ )                    \
    _( Compile_Branch )                 \
    _( Compile_Branch_ಕೋಷ್ಟಕ )            \
    _( Compile_Return )                 \
    _( Compile_Call )                   \
    _( Compile_Call_ಪರೋಕ್ಷ )           \
    _( Compile_Drop )                   \
    _( Compile_Select )                 \
    _( Compile_GetLocal )               \
    _( Compile_ಗಣ_Local )               \
    _( Compile_Get_ಗಣ_Global )           \
    _( Compile_ತುಂಬು_Store )             \
    _( Compile_ಸ್ಮರಣೆ_ಗಾತ್ರ )            \
    _( Compile_ಸ್ಮರಣೆ_Grow )            \
    _( Compile_ಸ್ಮರಣೆ_Init )            \
    _( Compile_ಸ್ಮರಣೆ_Copy_Fill )        \
    _( Compile_ದತ್ತಾಂಶ_Drop )              \
    _( Compile_Const_i32 )              \
    _( Compile_Const_i64 )              \
    _( Compile_Convert )                \
    _( Compile_ExtendedOpcode )         \
    d_m3CompilerList_atomics( _ )       \
    d_m3CompilerList_f( _ )             \
    d_m3CompilerList_refTypes( _ )      \
    d_m3CompilerList_typedRefs( _ )     \
    d_m3CompilerList_eh( _ )            \
    d_m3CompilerList_wide( _ )          \
    d_m3_Compiler_List_ರಾಶಿ_Switching( _ )

#ಆದರೆ d_m3ImplementFloat
#  define d_m3CompilerList_f(_)         \
    _( Compile_Const_f32 )              \
    _( Compile_Const_f64 )
#ಇಲ್ಲದಿದ್ದರೆ
#  define d_m3CompilerList_f(_)
#ಆದರೆ_ಅಂತ್ಯ

#ಆದರೆ d_m3HasRefTypes
#  define d_m3CompilerList_refTypes(_)  \
    _( Compile_Select_Typed )           \
    _( Compile_Ref_Null )               \
    _( Compile_Ref_IsNull )             \
    _( Compile_Ref_Func )               \
    _( Compile_ಕೋಷ್ಟಕ_Get_ಗಣ )           \
    _( Compile_ಕೋಷ್ಟಕ_Init )             \
    _( Compile_ಕೋಷ್ಟಕ_Copy )             \
    _( Compile_ಕೋಷ್ಟಕ_ಗಾತ್ರ )             \
    _( Compile_ಕೋಷ್ಟಕ_Grow_Fill )         \
    _( Compile_Elem_Drop )
#ಇಲ್ಲದಿದ್ದರೆ
#  define d_m3CompilerList_refTypes(_)
#ಆದರೆ_ಅಂತ್ಯ

#ಆದರೆ d_m3HasTypedRefs
#  define d_m3CompilerList_typedRefs(_) \
    _( Compile_CallRef )                \
    _( Compile_Ref_AsNonNull )
#ಇಲ್ಲದಿದ್ದರೆ
#  define d_m3CompilerList_typedRefs(_)
#ಆದರೆ_ಅಂತ್ಯ

#ಆದರೆ d_m3HasExceptionHandling
#  define d_m3CompilerList_eh(_)        \
    _( Compile_Throw )                  \
    _( Compile_ThrowRef )               \
    _( Compile_Try_ಕೋಷ್ಟಕ )
#ಇಲ್ಲದಿದ್ದರೆ
#  define d_m3CompilerList_eh(_)
#ಆದರೆ_ಅಂತ್ಯ

#ಆದರೆ d_m3HasAtomics
#  define d_m3CompilerList_atomics(_)   \
    _( Compile_Atomic )
#ಇಲ್ಲದಿದ್ದರೆ
#  define d_m3CompilerList_atomics(_)
#ಆದರೆ_ಅಂತ್ಯ

#ಆದರೆ d_m3HasWideArithmetic
#  define d_m3CompilerList_wide(_)      \
    _( Compile_WideArithmetic )
#ಇಲ್ಲದಿದ್ದರೆ
#  define d_m3CompilerList_wide(_)
#ಆದರೆ_ಅಂತ್ಯ

#ಆದರೆ d_m3_Has_ರಾಶಿ_Switching
#  define d_m3_Compiler_List_ರಾಶಿ_Switching(_) \
    _( Compile_ContNew )                     \
    _( Compile_ContBind )                    \
    _( Compile_Suspend )                     \
    _( Compile_Resume )                      \
    _( Compile_Switch )
#ಇಲ್ಲದಿದ್ದರೆ
#  define d_m3_Compiler_List_ರಾಶಿ_Switching(_)
#ಆದರೆ_ಅಂತ್ಯ

// Index 0 is "this opcode has no compiler", so that a zeroed entry - which is
// what M3OP_RESERVED and the reserved holes in the tables are - reads as one.
ಎಣಿಕೆ {
    c_cc_NULL = 0,
#ವ್ಯಾಖ್ಯಾನಿಸು d_m3CompilerEnum(NAME)  c_cc_##NAME,
    d_m3CompilerList(d_m3CompilerEnum)
#ವ್ಯಾಖ್ಯಾನ_ರದ್ದು d_m3CompilerEnum
};

ಸ್ಥಿರ ಸ್ಥಿರಾಂಕ M3Compiler c_compilers[] = {
    ಖಾಲಿ,
#ವ್ಯಾಖ್ಯಾನಿಸು d_m3CompilerPtr(NAME)   NAME,
    d_m3CompilerList(d_m3CompilerPtr)
#ವ್ಯಾಖ್ಯಾನ_ರದ್ದು d_m3CompilerPtr
};

// Names a table entry's compiler by index. d_cc (NULL) is the no-compiler entry.
#ವ್ಯಾಖ್ಯಾನಿಸು d_cc(NAME)                          .compiler = c_cc_##NAME

ಸ್ಥಿರ
M3Compiler GetCompiler (IM3OpInfo i_opInfo)
{
    d_m3Assert(i_opInfo->compiler < M3_ಎಣಿಕೆ_ದ(c_compilers));
    ಹಿಂತಿರುಗಿ c_compilers[i_opInfo->compiler];
}

// clang-format off
ಸ್ಥಿರಾಂಕ M3OpInfo c_operations[] =
{
    M3OP( "unreachable",           0,  none,   d_cc(Compile_Unreachable),  d_logOp (Unreachable) ),                   // 0x00
    M3OP( "nop",                   0,  none,   d_cc(Compile_Nop),          d_ಖಾಲಿಗಳು_Op_List ),                           // 0x01 .
    M3OP( "block",                 0,  none,   d_cc(Compile_Loop_Or_ಖಂಡ),  d_ಖಾಲಿಗಳು_Op_List ),                           // 0x02
    M3OP( "loop",                  0,  none,   d_cc(Compile_Loop_Or_ಖಂಡ),  d_logOp (Loop) ),                          // 0x03
    M3OP( "if",                    -1, none,   d_cc(Compile_If),           d_ಖಾಲಿಗಳು_Op_List ),                           // 0x04
    M3OP( "else",                  0,  none,   d_cc(Compile_Nop),          d_ಖಾಲಿಗಳು_Op_List ),                           // 0x05

    M3OP_RESERVED,  M3OP_RESERVED,                                                                                    // 0x06...0x07

#ಆದರೆ d_m3HasExceptionHandling
    M3OP( "throw",                 0,  none,   d_cc(Compile_Throw),        d_logOp (Throw) ),                         // 0x08
    M3OP_RESERVED,                                                                                                    // 0x09
    M3OP( "throw_ref",             0,  none,   d_cc(Compile_ThrowRef),     d_logOp (ThrowRef) ),                      // 0x0a
#ಇಲ್ಲದಿದ್ದರೆ
    M3OP_RESERVED,  M3OP_RESERVED, M3OP_RESERVED,                                                                     // 0x08...0x0a
#ಆದರೆ_ಅಂತ್ಯ

    M3OP( "end",                   0,  none,   d_cc(Compile_ಕೊನೆ),          d_ಖಾಲಿಗಳು_Op_List ),                           // 0x0b
    M3OP( "br",                    0,  none,   d_cc(Compile_Branch),       d_logOp (Branch) ),                        // 0x0c
    M3OP( "br_if",                 -1, none,   d_cc(Compile_Branch),       d_logOp2 (BranchIf_r, BranchIf_s) ),       // 0x0d
    M3OP( "br_table",              -1, none,   d_cc(Compile_Branch_ಕೋಷ್ಟಕ),  d_logOp (Branch_ಕೋಷ್ಟಕ) ),                   // 0x0e
    M3OP( "return",                0,  any,    d_cc(Compile_Return),       d_logOp (Return) ),                        // 0x0f
    M3OP( "call",                  0,  any,    d_cc(Compile_Call),         d_logOp (Call) ),                          // 0x10
    M3OP( "call_indirect",         0,  any,    d_cc(Compile_Call_ಪರೋಕ್ಷ), d_logOp (Call_ಪರೋಕ್ಷ) ),                  // 0x11
    M3OP( "return_call",           0,  any,    d_cc(Compile_Call),         d_logOp (ReturnCall) ),                    // 0x12
    M3OP( "return_call_indirect",  0,  any,    d_cc(Compile_Call_ಪರೋಕ್ಷ), d_logOp (Return_Call_ಪರೋಕ್ಷ) ),            // 0x13

#ಆದರೆ d_m3HasTypedRefs
    M3OP( "call_ref",              0,  any,    d_cc(Compile_CallRef),      d_logOp (CallRef) ),                       // 0x14
    M3OP( "return_call_ref",       0,  any,    d_cc(Compile_CallRef),      d_logOp (ReturnCallRef) ),                 // 0x15
#ಇಲ್ಲದಿದ್ದರೆ
    M3OP_RESERVED,  M3OP_RESERVED,                                                                                    // 0x14...
#ಆದರೆ_ಅಂತ್ಯ
    M3OP_RESERVED,  M3OP_RESERVED, M3OP_RESERVED, M3OP_RESERVED,                                                      // ...0x19

    M3OP( "drop",                  -1, none,   d_cc(Compile_Drop),         d_ಖಾಲಿಗಳು_Op_List ),                           // 0x1a
    M3OP( "select",                -2, any,    d_cc(Compile_Select),       d_ಖಾಲಿಗಳು_Op_List ),                           // 0x1b

#ಆದರೆ d_m3HasRefTypes
    M3OP( "select.t",              -2, any,    d_cc(Compile_Select_Typed), d_ಖಾಲಿಗಳು_Op_List ),                           // 0x1c
#ಇಲ್ಲದಿದ್ದರೆ
    M3OP_RESERVED,                                                                                                    // 0x1c
#ಆದರೆ_ಅಂತ್ಯ
    M3OP_RESERVED,  M3OP_RESERVED,                                                                                    // 0x1d...0x1e

#ಆದರೆ d_m3HasExceptionHandling
    M3OP( "try_table",             0,  none,   d_cc(Compile_Try_ಕೋಷ್ಟಕ),     d_logOp (Try_ಕೋಷ್ಟಕ) ),                      // 0x1f
#ಇಲ್ಲದಿದ್ದರೆ
    M3OP_RESERVED,                                                                                                    // 0x1f
#ಆದರೆ_ಅಂತ್ಯ

    M3OP( "local.get",             1,  any,    d_cc(Compile_GetLocal),     d_ಖಾಲಿಗಳು_Op_List ),                           // 0x20
    M3OP( "local.set",             1,  none,   d_cc(Compile_ಗಣ_Local),     d_ಖಾಲಿಗಳು_Op_List ),                           // 0x21
    M3OP( "local.tee",             0,  any,    d_cc(Compile_ಗಣ_Local),     d_ಖಾಲಿಗಳು_Op_List ),                           // 0x22
    M3OP( "global.get",            1,  none,   d_cc(Compile_Get_ಗಣ_Global), d_ಖಾಲಿಗಳು_Op_List ),                           // 0x23
    M3OP( "global.set",            1,  none,   d_cc(Compile_Get_ಗಣ_Global), d_ಖಾಲಿಗಳು_Op_List ),                           // 0x24

#ಆದರೆ d_m3HasRefTypes
    M3OP( "table.get",             0,  any,    d_cc(Compile_ಕೋಷ್ಟಕ_Get_ಗಣ), d_ಖಾಲಿಗಳು_Op_List ),                           // 0x25
    M3OP( "table.set",             -2, none,   d_cc(Compile_ಕೋಷ್ಟಕ_Get_ಗಣ), d_ಖಾಲಿಗಳು_Op_List ),                           // 0x26
#ಇಲ್ಲದಿದ್ದರೆ
    M3OP_RESERVED,  M3OP_RESERVED,                                                                                    // 0x25...0x26
#ಆದರೆ_ಅಂತ್ಯ
    M3OP_RESERVED,                                                                                                    // 0x27

    M3OP( "i32.load",              0,  i_32,   d_cc(Compile_ತುಂಬು_Store),   d_unaryOpList (i32, ತುಂಬು_i32) ),           // 0x28
    M3OP( "i64.load",              0,  i_64,   d_cc(Compile_ತುಂಬು_Store),   d_unaryOpList (i64, ತುಂಬು_i64) ),           // 0x29
    M3OP_F( "f32.load",            0,  f_32,   d_cc(Compile_ತುಂಬು_Store),   d_unaryOpList (f32, ತುಂಬು_f32) ),           // 0x2a
    M3OP_F( "f64.load",            0,  f_64,   d_cc(Compile_ತುಂಬು_Store),   d_unaryOpList (f64, ತುಂಬು_f64) ),           // 0x2b

    M3OP( "i32.load8_s",           0,  i_32,   d_cc(Compile_ತುಂಬು_Store),   d_unaryOpList (i32, ತುಂಬು_i8) ),            // 0x2c
    M3OP( "i32.load8_u",           0,  i_32,   d_cc(Compile_ತುಂಬು_Store),   d_unaryOpList (i32, ತುಂಬು_u8) ),            // 0x2d
    M3OP( "i32.load16_s",          0,  i_32,   d_cc(Compile_ತುಂಬು_Store),   d_unaryOpList (i32, ತುಂಬು_i16) ),           // 0x2e
    M3OP( "i32.load16_u",          0,  i_32,   d_cc(Compile_ತುಂಬು_Store),   d_unaryOpList (i32, ತುಂಬು_u16) ),           // 0x2f

    M3OP( "i64.load8_s",           0,  i_64,   d_cc(Compile_ತುಂಬು_Store),   d_unaryOpList (i64, ತುಂಬು_i8) ),            // 0x30
    M3OP( "i64.load8_u",           0,  i_64,   d_cc(Compile_ತುಂಬು_Store),   d_unaryOpList (i64, ತುಂಬು_u8) ),            // 0x31
    M3OP( "i64.load16_s",          0,  i_64,   d_cc(Compile_ತುಂಬು_Store),   d_unaryOpList (i64, ತುಂಬು_i16) ),           // 0x32
    M3OP( "i64.load16_u",          0,  i_64,   d_cc(Compile_ತುಂಬು_Store),   d_unaryOpList (i64, ತುಂಬು_u16) ),           // 0x33
    M3OP( "i64.load32_s",          0,  i_64,   d_cc(Compile_ತುಂಬು_Store),   d_unaryOpList (i64, ತುಂಬು_i32) ),           // 0x34
    M3OP( "i64.load32_u",          0,  i_64,   d_cc(Compile_ತುಂಬು_Store),   d_unaryOpList (i64, ತುಂಬು_u32) ),           // 0x35

    M3OP( "i32.store",             -2, none,   d_cc(Compile_ತುಂಬು_Store),   d_binOpList (i32, Store_i32) ),            // 0x36
    M3OP( "i64.store",             -2, none,   d_cc(Compile_ತುಂಬು_Store),   d_binOpList (i64, Store_i64) ),            // 0x37
    M3OP_F( "f32.store",           -2, none,   d_cc(Compile_ತುಂಬು_Store),   d_storeFpOpList (f32, Store_f32) ),        // 0x38
    M3OP_F( "f64.store",           -2, none,   d_cc(Compile_ತುಂಬು_Store),   d_storeFpOpList (f64, Store_f64) ),        // 0x39

    M3OP( "i32.store8",            -2, none,   d_cc(Compile_ತುಂಬು_Store),   d_binOpList (i32, Store_u8) ),             // 0x3a
    M3OP( "i32.store16",           -2, none,   d_cc(Compile_ತುಂಬು_Store),   d_binOpList (i32, Store_i16) ),            // 0x3b

    M3OP( "i64.store8",            -2, none,   d_cc(Compile_ತುಂಬು_Store),   d_binOpList (i64, Store_u8) ),             // 0x3c
    M3OP( "i64.store16",           -2, none,   d_cc(Compile_ತುಂಬು_Store),   d_binOpList (i64, Store_i16) ),            // 0x3d
    M3OP( "i64.store32",           -2, none,   d_cc(Compile_ತುಂಬು_Store),   d_binOpList (i64, Store_i32) ),            // 0x3e

    M3OP( "memory.size",           1,  i_32,   d_cc(Compile_ಸ್ಮರಣೆ_ಗಾತ್ರ),  d_logOp (Mem_ಗಾತ್ರ) ),                       // 0x3f
    M3OP( "memory.grow",           1,  i_32,   d_cc(Compile_ಸ್ಮರಣೆ_Grow),  d_logOp (MemGrow) ),                       // 0x40

    M3OP( "i32.const",             1,  i_32,   d_cc(Compile_Const_i32),    d_logOp (Const32) ),                       // 0x41
    M3OP( "i64.const",             1,  i_64,   d_cc(Compile_Const_i64),    d_logOp (Const64) ),                       // 0x42
    M3OP_F( "f32.const",           1,  f_32,   d_cc(Compile_Const_f32),    d_ಖಾಲಿಗಳು_Op_List ),                           // 0x43
    M3OP_F( "f64.const",           1,  f_64,   d_cc(Compile_Const_f64),    d_ಖಾಲಿಗಳು_Op_List ),                           // 0x44

    M3OP( "i32.eqz",               0,  i_32,   d_cc(ಖಾಲಿ),                 d_unaryOpList (i32, Equal_ಗಮ್ಯ_Zero) ),        // 0x45
    M3OP( "i32.eq",                -1, i_32,   d_cc(ಖಾಲಿ),                 d_commutativeBinOpList (i32, Equal) ),     // 0x46
    M3OP( "i32.ne",                -1, i_32,   d_cc(ಖಾಲಿ),                 d_commutativeBinOpList (i32, NotEqual) ),  // 0x47
    M3OP( "i32.lt_s",              -1, i_32,   d_cc(ಖಾಲಿ),                 d_binOpList (i32, LessThan) ),             // 0x48
    M3OP( "i32.lt_u",              -1, i_32,   d_cc(ಖಾಲಿ),                 d_binOpList (u32, LessThan) ),             // 0x49
    M3OP( "i32.gt_s",              -1, i_32,   d_cc(ಖಾಲಿ),                 d_binOpList (i32, GreaterThan) ),          // 0x4a
    M3OP( "i32.gt_u",              -1, i_32,   d_cc(ಖಾಲಿ),                 d_binOpList (u32, GreaterThan) ),          // 0x4b
    M3OP( "i32.le_s",              -1, i_32,   d_cc(ಖಾಲಿ),                 d_binOpList (i32, LessThanOrEqual) ),      // 0x4c
    M3OP( "i32.le_u",              -1, i_32,   d_cc(ಖಾಲಿ),                 d_binOpList (u32, LessThanOrEqual) ),      // 0x4d
    M3OP( "i32.ge_s",              -1, i_32,   d_cc(ಖಾಲಿ),                 d_binOpList (i32, GreaterThanOrEqual) ),   // 0x4e
    M3OP( "i32.ge_u",              -1, i_32,   d_cc(ಖಾಲಿ),                 d_binOpList (u32, GreaterThanOrEqual) ),   // 0x4f

    M3OP( "i64.eqz",               0,  i_32,   d_cc(ಖಾಲಿ),                 d_unaryOpList (i64, Equal_ಗಮ್ಯ_Zero) ),        // 0x50
    M3OP( "i64.eq",                -1, i_32,   d_cc(ಖಾಲಿ),                 d_commutativeBinOpList (i64, Equal) ),     // 0x51
    M3OP( "i64.ne",                -1, i_32,   d_cc(ಖಾಲಿ),                 d_commutativeBinOpList (i64, NotEqual) ),  // 0x52
    M3OP( "i64.lt_s",              -1, i_32,   d_cc(ಖಾಲಿ),                 d_binOpList (i64, LessThan) ),             // 0x53
    M3OP( "i64.lt_u",              -1, i_32,   d_cc(ಖಾಲಿ),                 d_binOpList (u64, LessThan) ),             // 0x54
    M3OP( "i64.gt_s",              -1, i_32,   d_cc(ಖಾಲಿ),                 d_binOpList (i64, GreaterThan) ),          // 0x55
    M3OP( "i64.gt_u",              -1, i_32,   d_cc(ಖಾಲಿ),                 d_binOpList (u64, GreaterThan) ),          // 0x56
    M3OP( "i64.le_s",              -1, i_32,   d_cc(ಖಾಲಿ),                 d_binOpList (i64, LessThanOrEqual) ),      // 0x57
    M3OP( "i64.le_u",              -1, i_32,   d_cc(ಖಾಲಿ),                 d_binOpList (u64, LessThanOrEqual) ),      // 0x58
    M3OP( "i64.ge_s",              -1, i_32,   d_cc(ಖಾಲಿ),                 d_binOpList (i64, GreaterThanOrEqual) ),   // 0x59
    M3OP( "i64.ge_u",              -1, i_32,   d_cc(ಖಾಲಿ),                 d_binOpList (u64, GreaterThanOrEqual) ),   // 0x5a

    M3OP_F( "f32.eq",              -1, i_32,   d_cc(ಖಾಲಿ),                 d_commutativeBinOpList (f32, Equal) ),     // 0x5b
    M3OP_F( "f32.ne",              -1, i_32,   d_cc(ಖಾಲಿ),                 d_commutativeBinOpList (f32, NotEqual) ),  // 0x5c
    M3OP_F( "f32.lt",              -1, i_32,   d_cc(ಖಾಲಿ),                 d_binOpList (f32, LessThan) ),             // 0x5d
    M3OP_F( "f32.gt",              -1, i_32,   d_cc(ಖಾಲಿ),                 d_binOpList (f32, GreaterThan) ),          // 0x5e
    M3OP_F( "f32.le",              -1, i_32,   d_cc(ಖಾಲಿ),                 d_binOpList (f32, LessThanOrEqual) ),      // 0x5f
    M3OP_F( "f32.ge",              -1, i_32,   d_cc(ಖಾಲಿ),                 d_binOpList (f32, GreaterThanOrEqual) ),   // 0x60

    M3OP_F( "f64.eq",              -1, i_32,   d_cc(ಖಾಲಿ),                 d_commutativeBinOpList (f64, Equal) ),     // 0x61
    M3OP_F( "f64.ne",              -1, i_32,   d_cc(ಖಾಲಿ),                 d_commutativeBinOpList (f64, NotEqual) ),  // 0x62
    M3OP_F( "f64.lt",              -1, i_32,   d_cc(ಖಾಲಿ),                 d_binOpList (f64, LessThan) ),             // 0x63
    M3OP_F( "f64.gt",              -1, i_32,   d_cc(ಖಾಲಿ),                 d_binOpList (f64, GreaterThan) ),          // 0x64
    M3OP_F( "f64.le",              -1, i_32,   d_cc(ಖಾಲಿ),                 d_binOpList (f64, LessThanOrEqual) ),      // 0x65
    M3OP_F( "f64.ge",              -1, i_32,   d_cc(ಖಾಲಿ),                 d_binOpList (f64, GreaterThanOrEqual) ),   // 0x66

    M3OP( "i32.clz",               0,  i_32,   d_cc(ಖಾಲಿ),                 d_unaryOpList (u32, Clz) ),                // 0x67
    M3OP( "i32.ctz",               0,  i_32,   d_cc(ಖಾಲಿ),                 d_unaryOpList (u32, Ctz) ),                // 0x68
    M3OP( "i32.popcnt",            0,  i_32,   d_cc(ಖಾಲಿ),                 d_unaryOpList (u32, Popcnt) ),             // 0x69

    M3OP( "i32.add",               -1, i_32,   d_cc(ಖಾಲಿ),                 d_commutativeBinOpList (i32, ಕೂಡಿಸು) ),       // 0x6a
    M3OP( "i32.sub",               -1, i_32,   d_cc(ಖಾಲಿ),                 d_binOpList (i32, ಕಳೆ) ),             // 0x6b
    M3OP( "i32.mul",               -1, i_32,   d_cc(ಖಾಲಿ),                 d_commutativeBinOpList (i32, Multiply) ),  // 0x6c
    M3OP( "i32.div_s",             -1, i_32,   d_cc(ಖಾಲಿ),                 d_binOpList (i32, Divide) ),               // 0x6d
    M3OP( "i32.div_u",             -1, i_32,   d_cc(ಖಾಲಿ),                 d_binOpList (u32, Divide) ),               // 0x6e
    M3OP( "i32.rem_s",             -1, i_32,   d_cc(ಖಾಲಿ),                 d_binOpList (i32, Remainder) ),            // 0x6f
    M3OP( "i32.rem_u",             -1, i_32,   d_cc(ಖಾಲಿ),                 d_binOpList (u32, Remainder) ),            // 0x70
    M3OP( "i32.and",               -1, i_32,   d_cc(ಖಾಲಿ),                 d_commutativeBinOpList (u32, And) ),       // 0x71
    M3OP( "i32.or",                -1, i_32,   d_cc(ಖಾಲಿ),                 d_commutativeBinOpList (u32, Or) ),        // 0x72
    M3OP( "i32.xor",               -1, i_32,   d_cc(ಖಾಲಿ),                 d_commutativeBinOpList (u32, Xor) ),       // 0x73
    M3OP( "i32.shl",               -1, i_32,   d_cc(ಖಾಲಿ),                 d_binOpList (u32, ಸರಿಸು_ಎಡ) ),            // 0x74
    M3OP( "i32.shr_s",             -1, i_32,   d_cc(ಖಾಲಿ),                 d_binOpList (i32, ಸರಿಸು_ಬಲ) ),           // 0x75
    M3OP( "i32.shr_u",             -1, i_32,   d_cc(ಖಾಲಿ),                 d_binOpList (u32, ಸರಿಸು_ಬಲ) ),           // 0x76
    M3OP( "i32.rotl",              -1, i_32,   d_cc(ಖಾಲಿ),                 d_binOpList (u32, Rotl) ),                 // 0x77
    M3OP( "i32.rotr",              -1, i_32,   d_cc(ಖಾಲಿ),                 d_binOpList (u32, Rotr) ),                 // 0x78

    M3OP( "i64.clz",               0,  i_64,   d_cc(ಖಾಲಿ),                 d_unaryOpList (u64, Clz) ),                // 0x79
    M3OP( "i64.ctz",               0,  i_64,   d_cc(ಖಾಲಿ),                 d_unaryOpList (u64, Ctz) ),                // 0x7a
    M3OP( "i64.popcnt",            0,  i_64,   d_cc(ಖಾಲಿ),                 d_unaryOpList (u64, Popcnt) ),             // 0x7b

    M3OP( "i64.add",               -1, i_64,   d_cc(ಖಾಲಿ),                 d_commutativeBinOpList (i64, ಕೂಡಿಸು) ),       // 0x7c
    M3OP( "i64.sub",               -1, i_64,   d_cc(ಖಾಲಿ),                 d_binOpList (i64, ಕಳೆ) ),             // 0x7d
    M3OP( "i64.mul",               -1, i_64,   d_cc(ಖಾಲಿ),                 d_commutativeBinOpList (i64, Multiply) ),  // 0x7e
    M3OP( "i64.div_s",             -1, i_64,   d_cc(ಖಾಲಿ),                 d_binOpList (i64, Divide) ),               // 0x7f
    M3OP( "i64.div_u",             -1, i_64,   d_cc(ಖಾಲಿ),                 d_binOpList (u64, Divide) ),               // 0x80
    M3OP( "i64.rem_s",             -1, i_64,   d_cc(ಖಾಲಿ),                 d_binOpList (i64, Remainder) ),            // 0x81
    M3OP( "i64.rem_u",             -1, i_64,   d_cc(ಖಾಲಿ),                 d_binOpList (u64, Remainder) ),            // 0x82
    M3OP( "i64.and",               -1, i_64,   d_cc(ಖಾಲಿ),                 d_commutativeBinOpList (u64, And) ),       // 0x83
    M3OP( "i64.or",                -1, i_64,   d_cc(ಖಾಲಿ),                 d_commutativeBinOpList (u64, Or) ),        // 0x84
    M3OP( "i64.xor",               -1, i_64,   d_cc(ಖಾಲಿ),                 d_commutativeBinOpList (u64, Xor) ),       // 0x85
    M3OP( "i64.shl",               -1, i_64,   d_cc(ಖಾಲಿ),                 d_binOpList (u64, ಸರಿಸು_ಎಡ) ),            // 0x86
    M3OP( "i64.shr_s",             -1, i_64,   d_cc(ಖಾಲಿ),                 d_binOpList (i64, ಸರಿಸು_ಬಲ) ),           // 0x87
    M3OP( "i64.shr_u",             -1, i_64,   d_cc(ಖಾಲಿ),                 d_binOpList (u64, ಸರಿಸು_ಬಲ) ),           // 0x88
    M3OP( "i64.rotl",              -1, i_64,   d_cc(ಖಾಲಿ),                 d_binOpList (u64, Rotl) ),                 // 0x89
    M3OP( "i64.rotr",              -1, i_64,   d_cc(ಖಾಲಿ),                 d_binOpList (u64, Rotr) ),                 // 0x8a

    M3OP_F( "f32.abs",             0,  f_32,   d_cc(ಖಾಲಿ),                 d_unaryOpList(f32, Abs) ),                 // 0x8b
    M3OP_F( "f32.neg",             0,  f_32,   d_cc(ಖಾಲಿ),                 d_unaryOpList(f32, Negate) ),              // 0x8c
    M3OP_F( "f32.ceil",            0,  f_32,   d_cc(ಖಾಲಿ),                 d_unaryOpList(f32, Ceil) ),                // 0x8d
    M3OP_F( "f32.floor",           0,  f_32,   d_cc(ಖಾಲಿ),                 d_unaryOpList(f32, Floor) ),               // 0x8e
    M3OP_F( "f32.trunc",           0,  f_32,   d_cc(ಖಾಲಿ),                 d_unaryOpList(f32, Trunc) ),               // 0x8f
    M3OP_F( "f32.nearest",         0,  f_32,   d_cc(ಖಾಲಿ),                 d_unaryOpList(f32, ಹತ್ತಿರದ) ),             // 0x90
    M3OP_F( "f32.sqrt",            0,  f_32,   d_cc(ಖಾಲಿ),                 d_unaryOpList(f32, Sqrt) ),                // 0x91

    M3OP_F( "f32.add",             -1, f_32,   d_cc(ಖಾಲಿ),                 d_commutativeBinOpList (f32, ಕೂಡಿಸು) ),       // 0x92
    M3OP_F( "f32.sub",             -1, f_32,   d_cc(ಖಾಲಿ),                 d_binOpList (f32, ಕಳೆ) ),             // 0x93
    M3OP_F( "f32.mul",             -1, f_32,   d_cc(ಖಾಲಿ),                 d_commutativeBinOpList (f32, Multiply) ),  // 0x94
    M3OP_F( "f32.div",             -1, f_32,   d_cc(ಖಾಲಿ),                 d_binOpList (f32, Divide) ),               // 0x95
    M3OP_F( "f32.min",             -1, f_32,   d_cc(ಖಾಲಿ),                 d_commutativeBinOpList (f32, Min) ),       // 0x96
    M3OP_F( "f32.max",             -1, f_32,   d_cc(ಖಾಲಿ),                 d_commutativeBinOpList (f32, Max) ),       // 0x97
    M3OP_F( "f32.copysign",        -1, f_32,   d_cc(ಖಾಲಿ),                 d_binOpList (f32, CopySign) ),             // 0x98

    M3OP_F( "f64.abs",             0,  f_64,   d_cc(ಖಾಲಿ),                 d_unaryOpList(f64, Abs) ),                 // 0x99
    M3OP_F( "f64.neg",             0,  f_64,   d_cc(ಖಾಲಿ),                 d_unaryOpList(f64, Negate) ),              // 0x9a
    M3OP_F( "f64.ceil",            0,  f_64,   d_cc(ಖಾಲಿ),                 d_unaryOpList(f64, Ceil) ),                // 0x9b
    M3OP_F( "f64.floor",           0,  f_64,   d_cc(ಖಾಲಿ),                 d_unaryOpList(f64, Floor) ),               // 0x9c
    M3OP_F( "f64.trunc",           0,  f_64,   d_cc(ಖಾಲಿ),                 d_unaryOpList(f64, Trunc) ),               // 0x9d
    M3OP_F( "f64.nearest",         0,  f_64,   d_cc(ಖಾಲಿ),                 d_unaryOpList(f64, ಹತ್ತಿರದ) ),             // 0x9e
    M3OP_F( "f64.sqrt",            0,  f_64,   d_cc(ಖಾಲಿ),                 d_unaryOpList(f64, Sqrt) ),                // 0x9f

    M3OP_F( "f64.add",             -1, f_64,   d_cc(ಖಾಲಿ),                 d_commutativeBinOpList (f64, ಕೂಡಿಸು) ),       // 0xa0
    M3OP_F( "f64.sub",             -1, f_64,   d_cc(ಖಾಲಿ),                 d_binOpList (f64, ಕಳೆ) ),             // 0xa1
    M3OP_F( "f64.mul",             -1, f_64,   d_cc(ಖಾಲಿ),                 d_commutativeBinOpList (f64, Multiply) ),  // 0xa2
    M3OP_F( "f64.div",             -1, f_64,   d_cc(ಖಾಲಿ),                 d_binOpList (f64, Divide) ),               // 0xa3
    M3OP_F( "f64.min",             -1, f_64,   d_cc(ಖಾಲಿ),                 d_commutativeBinOpList (f64, Min) ),       // 0xa4
    M3OP_F( "f64.max",             -1, f_64,   d_cc(ಖಾಲಿ),                 d_commutativeBinOpList (f64, Max) ),       // 0xa5
    M3OP_F( "f64.copysign",        -1, f_64,   d_cc(ಖಾಲಿ),                 d_binOpList (f64, CopySign) ),             // 0xa6

    M3OP( "i32.wrap/i64",          0,  i_32,   d_cc(ಖಾಲಿ),                 d_unaryOpList (i32, Wrap_i64) ),           // 0xa7
    M3OP_F( "i32.trunc_s/f32",     0,  i_32,   d_cc(Compile_Convert),      d_convertOpList (i32_Trunc_f32) ),         // 0xa8
    M3OP_F( "i32.trunc_u/f32",     0,  i_32,   d_cc(Compile_Convert),      d_convertOpList (u32_Trunc_f32) ),         // 0xa9
    M3OP_F( "i32.trunc_s/f64",     0,  i_32,   d_cc(Compile_Convert),      d_convertOpList (i32_Trunc_f64) ),         // 0xaa
    M3OP_F( "i32.trunc_u/f64",     0,  i_32,   d_cc(Compile_Convert),      d_convertOpList (u32_Trunc_f64) ),         // 0xab

    M3OP( "i64.extend_s/i32",      0,  i_64,   d_cc(ಖಾಲಿ),                 d_unaryOpList (i64, Extend_i32) ),         // 0xac
    M3OP( "i64.extend_u/i32",      0,  i_64,   d_cc(ಖಾಲಿ),                 d_unaryOpList (i64, Extend_u32) ),         // 0xad

    M3OP_F( "i64.trunc_s/f32",     0,  i_64,   d_cc(Compile_Convert),      d_convertOpList (i64_Trunc_f32) ),         // 0xae
    M3OP_F( "i64.trunc_u/f32",     0,  i_64,   d_cc(Compile_Convert),      d_convertOpList (u64_Trunc_f32) ),         // 0xaf
    M3OP_F( "i64.trunc_s/f64",     0,  i_64,   d_cc(Compile_Convert),      d_convertOpList (i64_Trunc_f64) ),         // 0xb0
    M3OP_F( "i64.trunc_u/f64",     0,  i_64,   d_cc(Compile_Convert),      d_convertOpList (u64_Trunc_f64) ),         // 0xb1

    M3OP_F( "f32.convert_s/i32",   0,  f_32,   d_cc(Compile_Convert),      d_convertOpList (f32_Convert_i32) ),       // 0xb2
    M3OP_F( "f32.convert_u/i32",   0,  f_32,   d_cc(Compile_Convert),      d_convertOpList (f32_Convert_u32) ),       // 0xb3
    M3OP_F( "f32.convert_s/i64",   0,  f_32,   d_cc(Compile_Convert),      d_convertOpList (f32_Convert_i64) ),       // 0xb4
    M3OP_F( "f32.convert_u/i64",   0,  f_32,   d_cc(Compile_Convert),      d_convertOpList (f32_Convert_u64) ),       // 0xb5

    M3OP_F( "f32.demote/f64",      0,  f_32,   d_cc(ಖಾಲಿ),                 d_unaryOpList (f32, Demote_f64) ),         // 0xb6

    M3OP_F( "f64.convert_s/i32",   0,  f_64,   d_cc(Compile_Convert),      d_convertOpList (f64_Convert_i32) ),       // 0xb7
    M3OP_F( "f64.convert_u/i32",   0,  f_64,   d_cc(Compile_Convert),      d_convertOpList (f64_Convert_u32) ),       // 0xb8
    M3OP_F( "f64.convert_s/i64",   0,  f_64,   d_cc(Compile_Convert),      d_convertOpList (f64_Convert_i64) ),       // 0xb9
    M3OP_F( "f64.convert_u/i64",   0,  f_64,   d_cc(Compile_Convert),      d_convertOpList (f64_Convert_u64) ),       // 0xba

    M3OP_F( "f64.promote/f32",     0,  f_64,   d_cc(ಖಾಲಿ),                 d_unaryOpList (f64, Promote_f32) ),        // 0xbb

    M3OP_F( "i32.reinterpret/f32", 0,  i_32,   d_cc(Compile_Convert),      d_convertOpList (i32_Reinterpret_f32) ),   // 0xbc
    M3OP_F( "i64.reinterpret/f64", 0,  i_64,   d_cc(Compile_Convert),      d_convertOpList (i64_Reinterpret_f64) ),   // 0xbd
    M3OP_F( "f32.reinterpret/i32", 0,  f_32,   d_cc(Compile_Convert),      d_convertOpList (f32_Reinterpret_i32) ),   // 0xbe
    M3OP_F( "f64.reinterpret/i64", 0,  f_64,   d_cc(Compile_Convert),      d_convertOpList (f64_Reinterpret_i64) ),   // 0xbf

    M3OP( "i32.extend8_s",         0,  i_32,   d_cc(ಖಾಲಿ),                 d_unaryOpList (i32, Extend8_s) ),          // 0xc0
    M3OP( "i32.extend16_s",        0,  i_32,   d_cc(ಖಾಲಿ),                 d_unaryOpList (i32, Extend16_s) ),         // 0xc1
    M3OP( "i64.extend8_s",         0,  i_64,   d_cc(ಖಾಲಿ),                 d_unaryOpList (i64, Extend8_s) ),          // 0xc2
    M3OP( "i64.extend16_s",        0,  i_64,   d_cc(ಖಾಲಿ),                 d_unaryOpList (i64, Extend16_s) ),         // 0xc3
    M3OP( "i64.extend32_s",        0,  i_64,   d_cc(ಖಾಲಿ),                 d_unaryOpList (i64, Extend32_s) ),         // 0xc4

// Internal operations, for codepage logging only. They sit past c_waOp_lastCore,
// the last opcode this table covers, so GetOpInfo () can never reach them.
# ifdef DEBUG // for codepage logging. the order doesn't matter:
#ವ್ಯಾಖ್ಯಾನಿಸು d_m3DebugOp(OP) M3OP (#OP, 0, none, d_ops (op_##OP))

# ಆದರೆ d_m3HasFloat
#ವ್ಯಾಖ್ಯಾನಿಸು d_m3DebugTypedOp(OP) M3OP (#OP, 0, none, d_ops (op_##OP##_i32, op_##OP##_i64, op_##OP##_f32, op_##OP##_f64))
# ಇಲ್ಲದಿದ್ದರೆ
#ವ್ಯಾಖ್ಯಾನಿಸು d_m3DebugTypedOp(OP) M3OP (#OP, 0, none, d_ops (op_##OP##_i32, op_##OP##_i64))
# endif

    d_m3DebugOp (Compile),          d_m3DebugOp (ನಮೂದು_2),            d_m3DebugOp (ಕೊನೆ_2),
    d_m3DebugOp (Unsupported),      d_m3DebugOp (CallRawFunction),

    d_m3DebugOp (GetGlobal_s32),    d_m3DebugOp (GetGlobal_s64),    d_m3DebugOp (ContinueLoop),     d_m3DebugOp (ContinueLoopIf),
# ಆದರೆ d_m3_Has_ರಾಶಿ_Switching
    d_m3DebugOp (ContinueLoop_Suspendable), d_m3DebugOp (ContinueLoopIf_Suspendable), d_m3DebugOp (ನಮೂದು_Check),
#  ಆದರೆ !d_m3_ನಮೂದು_Keeps_ಚೌಕಟ್ಟು
    d_m3DebugOp (ನಮೂದು_Suspendable),
#  endif
# endif

    d_m3DebugOp (Copy_ಸ್ಥಾನ_32),      d_m3DebugOp (Preserve_Copy_ಸ್ಥಾನ_32), d_m3DebugOp (If_s),          d_m3DebugOp (BranchIfPrologue_s),
    d_m3DebugOp (Copy_ಸ್ಥಾನ_64),      d_m3DebugOp (Preserve_Copy_ಸ್ಥಾನ_64), d_m3DebugOp (If_r),          d_m3DebugOp (BranchIfPrologue_r),

    d_m3DebugOp (Select_i32_rss),   d_m3DebugOp (Select_i32_srs),   d_m3DebugOp (Select_i32_ssr),   d_m3DebugOp (Select_i32_sss),
    d_m3DebugOp (Select_i64_rss),   d_m3DebugOp (Select_i64_srs),   d_m3DebugOp (Select_i64_ssr),   d_m3DebugOp (Select_i64_sss),

# ಆದರೆ d_m3HasFloat
    d_m3DebugOp (Select_f32_sss),   d_m3DebugOp (Select_f32_srs),   d_m3DebugOp (Select_f32_ssr),
    d_m3DebugOp (Select_f32_rss),   d_m3DebugOp (Select_f32_rrs),   d_m3DebugOp (Select_f32_rsr),

    d_m3DebugOp (Select_f64_sss),   d_m3DebugOp (Select_f64_srs),   d_m3DebugOp (Select_f64_ssr),
    d_m3DebugOp (Select_f64_rss),   d_m3DebugOp (Select_f64_rrs),   d_m3DebugOp (Select_f64_rsr),
# endif

    d_m3DebugOp (MemFill),          d_m3DebugOp (MemCopy),          d_m3DebugOp (MemInit),          d_m3DebugOp (ದತ್ತಾಂಶ_Drop),

# ಆದರೆ d_m3HasGasMetering
    d_m3DebugOp (UseGas),
# endif

# ಆದರೆ d_m3HasRefTypes
    d_m3DebugOp (ಕೋಷ್ಟಕ_Get),         d_m3DebugOp (ಕೋಷ್ಟಕ_ಗಣ),         d_m3DebugOp (ಕೋಷ್ಟಕ_ಗಾತ್ರ),
    d_m3DebugOp (ಕೋಷ್ಟಕ_Grow),        d_m3DebugOp (ಕೋಷ್ಟಕ_Fill),        d_m3DebugOp (ಕೋಷ್ಟಕ_Init),
    d_m3DebugOp (ElemDrop),         d_m3DebugOp (ಕೋಷ್ಟಕ_Copy),
# endif

    d_m3DebugTypedOp (ಗಣ_Global),   d_m3DebugOp (ಗಣ_Global_s32),    d_m3DebugOp (ಗಣ_Global_s64),

    d_m3DebugTypedOp (ಗಣ_ರಿಜಿಸ್ಟರ್), d_m3DebugTypedOp (ಗಣ_ಸ್ಥಾನ),     d_m3DebugTypedOp (Preserve_ಗಣ_ಸ್ಥಾನ),
# endif

# ifdef DEBUG
    M3OP( "termination", 0, c_m3Type_unknown, d_ಖಾಲಿಗಳು_Op_List ) // for find_operation_info
# endif
};

ಸ್ಥಿರಾಂಕ M3OpInfo c_operationsFC[] =
{
    M3OP_F( "i32.trunc_s:sat/f32", 0, i_32,   d_cc(Compile_Convert),         d_convertOpList (i32_TruncSat_f32) ),  // 0x00
    M3OP_F( "i32.trunc_u:sat/f32", 0, i_32,   d_cc(Compile_Convert),         d_convertOpList (u32_TruncSat_f32) ),  // 0x01
    M3OP_F( "i32.trunc_s:sat/f64", 0, i_32,   d_cc(Compile_Convert),         d_convertOpList (i32_TruncSat_f64) ),  // 0x02
    M3OP_F( "i32.trunc_u:sat/f64", 0, i_32,   d_cc(Compile_Convert),         d_convertOpList (u32_TruncSat_f64) ),  // 0x03
    M3OP_F( "i64.trunc_s:sat/f32", 0, i_64,   d_cc(Compile_Convert),         d_convertOpList (i64_TruncSat_f32) ),  // 0x04
    M3OP_F( "i64.trunc_u:sat/f32", 0, i_64,   d_cc(Compile_Convert),         d_convertOpList (u64_TruncSat_f32) ),  // 0x05
    M3OP_F( "i64.trunc_s:sat/f64", 0, i_64,   d_cc(Compile_Convert),         d_convertOpList (i64_TruncSat_f64) ),  // 0x06
    M3OP_F( "i64.trunc_u:sat/f64", 0, i_64,   d_cc(Compile_Convert),         d_convertOpList (u64_TruncSat_f64) ),  // 0x07

    M3OP( "memory.init",           0, none,   d_cc(Compile_ಸ್ಮರಣೆ_Init),     d_ಖಾಲಿಗಳು_Op_List ),                       // 0x08
    M3OP( "data.drop",             0, none,   d_cc(Compile_ದತ್ತಾಂಶ_Drop),       d_ಖಾಲಿಗಳು_Op_List ),                       // 0x09

    M3OP( "memory.copy",           0, none,   d_cc(Compile_ಸ್ಮರಣೆ_Copy_Fill), d_ಖಾಲಿಗಳು_Op_List ),                       // 0x0a
    M3OP( "memory.fill",           0, none,   d_cc(Compile_ಸ್ಮರಣೆ_Copy_Fill), d_ಖಾಲಿಗಳು_Op_List ),                       // 0x0b

#ಆದರೆ d_m3HasRefTypes
    M3OP( "table.init",            0, none,   d_cc(Compile_ಕೋಷ್ಟಕ_Init),      d_ಖಾಲಿಗಳು_Op_List ),                       // 0x0c
    M3OP( "elem.drop",             0, none,   d_cc(Compile_Elem_Drop),       d_ಖಾಲಿಗಳು_Op_List ),                       // 0x0d
    M3OP( "table.copy",            0, none,   d_cc(Compile_ಕೋಷ್ಟಕ_Copy),      d_ಖಾಲಿಗಳು_Op_List ),                       // 0x0e
#ಇಲ್ಲದಿದ್ದರೆ
    M3OP_RESERVED, M3OP_RESERVED, M3OP_RESERVED,                                                                    // 0x0c...0x0e
#ಆದರೆ_ಅಂತ್ಯ

#ಆದರೆ d_m3HasRefTypes
    M3OP( "table.grow",            0, i_32,   d_cc(Compile_ಕೋಷ್ಟಕ_Grow_Fill),  d_ಖಾಲಿಗಳು_Op_List ),                       // 0x0f
    M3OP( "table.size",            1, i_32,   d_cc(Compile_ಕೋಷ್ಟಕ_ಗಾತ್ರ),      d_ಖಾಲಿಗಳು_Op_List ),                       // 0x10
    M3OP( "table.fill",            0, none,   d_cc(Compile_ಕೋಷ್ಟಕ_Grow_Fill),  d_ಖಾಲಿಗಳು_Op_List ),                       // 0x11
#ಇಲ್ಲದಿದ್ದರೆ
    M3OP_RESERVED, M3OP_RESERVED, M3OP_RESERVED,                                                                    // 0x0f...0x11
#ಆದರೆ_ಅಂತ್ಯ

#ಆದರೆ d_m3HasWideArithmetic
    M3OP_RESERVED,                                                                                                  // 0x12

    M3OP( "i64.add128",            -2, i_64,  d_cc(Compile_WideArithmetic), d_logOp (i64_Add128) ),              // 0x13
    M3OP( "i64.sub128",            -2, i_64,  d_cc(Compile_WideArithmetic), d_logOp (i64_Sub128) ),              // 0x14
    M3OP( "i64.mul_wide_s",        0,  i_64,  d_cc(Compile_WideArithmetic), d_logOp (i64_MultiplyWide) ),        // 0x15
    M3OP( "i64.mul_wide_u",        0,  i_64,  d_cc(Compile_WideArithmetic), d_logOp (u64_MultiplyWide) ),        // 0x16
#ಆದರೆ_ಅಂತ್ಯ


# ifdef DEBUG
    M3OP( "termination", 0, c_m3Type_unknown, d_ಖಾಲಿಗಳು_Op_List ) // for find_operation_info
# endif
};

#ಆದರೆ d_m3HasAtomics

// The 0xFE instructions share one entry per kind rather than each having its own: what
// tells them apart, the width of the cell and the operation, is worked out from the
// sub-opcode by Compile_Atomic. GetAtomicInfoIndex () maps a sub-opcode onto this table.
ಎಣಿಕೆ {
    c_opAtomic_notify,
    c_opAtomic_wait32,
    c_opAtomic_wait64,
    c_opAtomic_fence,
    c_op_Atomic_ತುಂಬು,
    c_opAtomic_store,
    c_opAtomic_rmw,
    c_opAtomic_cmpxchg
};

ಸ್ಥಿರಾಂಕ M3OpInfo c_operationsFE[] =
{
    M3OP( "memory.atomic.notify",  -1, i_32,  d_cc(Compile_Atomic), d_logOp (AtomicNotify) ),    // c_opAtomic_notify
    M3OP( "memory.atomic.wait32",  -2, i_32,  d_cc(Compile_Atomic), d_logOp (AtomicWait32) ),    // c_opAtomic_wait32
    M3OP( "memory.atomic.wait64",  -2, i_32,  d_cc(Compile_Atomic), d_logOp (AtomicWait64) ),    // c_opAtomic_wait64
    M3OP( "atomic.fence",           0, none,  d_cc(Compile_Atomic), d_ಖಾಲಿಗಳು_Op_List ),             // c_opAtomic_fence
    M3OP( "atomic.load",            0, any,   d_cc(Compile_Atomic), d_logOp (Atomic_ತುಂಬು) ),      // c_opAtomic_load
    M3OP( "atomic.store",          -2, none,  d_cc(Compile_Atomic), d_logOp (AtomicStore) ),     // c_opAtomic_store
    M3OP( "atomic.rmw",            -1, any,   d_cc(Compile_Atomic), d_logOp (AtomicRmw) ),       // c_opAtomic_rmw
    M3OP( "atomic.rmw.cmpxchg",    -2, any,   d_cc(Compile_Atomic), d_logOp (AtomicCmpxchg) ),   // c_opAtomic_cmpxchg
};

ಸ್ಥಿರಾಂಕ u32 c_ಅಂಕೆ_Operations_FE = M3_ಎಣಿಕೆ_ದ(c_operationsFE);

// The entry a 0xFE sub-opcode uses, or -1 for one that names nothing: the spec leaves
// 0x04..0x0f and everything past cmpxchg unassigned. The accesses come in runs of seven,
// one per pairing of value type and cell width.
ಸ್ಥಿರ ಸಾಲೊಳಗೆ
i32 Get_Atomic_Info_ಸೂಚ್ಯಂಕ (u32 i_sub)
{
    ಆದರೆ (i_sub <= 0x03) ಹಿಂತಿರುಗಿ (i32)i_sub;
    ಆದರೆ (i_sub < 0x10) ಹಿಂತಿರುಗಿ -1;
    ಆದರೆ (i_sub <= 0x16) ಹಿಂತಿರುಗಿ c_op_Atomic_ತುಂಬು;
    ಆದರೆ (i_sub <= 0x1d) ಹಿಂತಿರುಗಿ c_opAtomic_store;
    ಆದರೆ (i_sub <= 0x47) ಹಿಂತಿರುಗಿ c_opAtomic_rmw;
    ಆದರೆ (i_sub <= c_wa_Op_ಕೊನೆಯ_Atomic) ಹಿಂತಿರುಗಿ c_opAtomic_cmpxchg;
    ಹಿಂತಿರುಗಿ -1;
}

#ಆದರೆ_ಅಂತ್ಯ // d_m3HasAtomics

// The opcodes above c_waOp_lastCore are sparse: the reference instructions at
// 0xd0..0xd2 and 0xd4, and the 0xfc extended-opcode prefix. Indexing c_operations out to
// 0xfc to reach them would leave ~50 empty entries there, so they get their own
// table, which GetHighOpIndex () maps the opcode onto.
ಎಣಿಕೆ {
#ಆದರೆ d_m3HasRefTypes
    c_op_ಮೇಲಿನ_ref_Null,
    c_op_ಮೇಲಿನ_ref_Is_Null,
    c_op_ಮೇಲಿನ_ref_Func,
#  ಆದರೆ d_m3HasTypedRefs
    c_op_ಮೇಲಿನ_ref_As_Non_Null,
#  endif
#ಆದರೆ_ಅಂತ್ಯ
#ಆದರೆ d_m3_Has_ರಾಶಿ_Switching
    c_op_ಮೇಲಿನ_cont_New,
    c_op_ಮೇಲಿನ_cont_Bind,
    c_op_ಮೇಲಿನ_suspend,
    c_op_ಮೇಲಿನ_resume,
    c_op_ಮೇಲಿನ_resume_Throw,
    c_op_ಮೇಲಿನ_resume_Throw_Ref,
    c_op_ಮೇಲಿನ_switch,
#ಆದರೆ_ಅಂತ್ಯ
#ಆದರೆ d_m3HasAtomics
    c_op_ಮೇಲಿನ_atomic,
#ಆದರೆ_ಅಂತ್ಯ
    c_op_ಮೇಲಿನ_extended   // always present, so the enum is never empty
};

// Positional, so the table must stay in c_opHigh_* order: array designators
// are not valid C++, which the maintenance builds compile this as.
ಸ್ಥಿರ ಸ್ಥಿರಾಂಕ M3OpInfo c_operations_ಮೇಲಿನ[] =
{
#ಆದರೆ d_m3HasRefTypes
    M3OP( "ref.null",        1, any,               d_cc(Compile_Ref_Null),       d_ಖಾಲಿಗಳು_Op_List ),  // 0xd0  c_opHigh_refNull
    M3OP( "ref.is_null",     0, i_32,              d_cc(Compile_Ref_IsNull),     d_ಖಾಲಿಗಳು_Op_List ),  // 0xd1  c_opHigh_refIsNull
    M3OP( "ref.func",        1, any,               d_cc(Compile_Ref_Func),       d_ಖಾಲಿಗಳು_Op_List ),  // 0xd2  c_opHigh_refFunc
#  ಆದರೆ d_m3HasTypedRefs
    M3OP( "ref.as_non_null", 0, any,               d_cc(Compile_Ref_AsNonNull),  d_ಖಾಲಿಗಳು_Op_List ),  // 0xd4  c_opHigh_refAsNonNull
#  endif
#ಆದರೆ_ಅಂತ್ಯ
#ಆದರೆ d_m3_Has_ರಾಶಿ_Switching
    M3OP( "cont.new",          1, any,             d_cc(Compile_ContNew),        d_ಖಾಲಿಗಳು_Op_List ),  // 0xe0  c_opHigh_contNew
    M3OP( "cont.bind",         2, any,             d_cc(Compile_ContBind),       d_ಖಾಲಿಗಳು_Op_List ),  // 0xe1  c_opHigh_contBind
    M3OP( "suspend",           1, any,             d_cc(Compile_Suspend),        d_ಖಾಲಿಗಳು_Op_List ),  // 0xe2  c_opHigh_suspend
    M3OP( "resume",            1, any,             d_cc(Compile_Resume),         d_ಖಾಲಿಗಳು_Op_List ),  // 0xe3  c_opHigh_resume
    M3OP( "resume_throw",      2, any,             d_cc(Compile_Resume)   ,    d_ಖಾಲಿಗಳು_Op_List ),  // 0xe4  c_opHigh_resumeThrow
    M3OP( "resume_throw_ref",  1, any,             d_cc(Compile_Resume)      , d_ಖಾಲಿಗಳು_Op_List ),  // 0xe5  c_opHigh_resumeThrowRef
    M3OP( "switch",            2, any,             d_cc(Compile_Switch),         d_ಖಾಲಿಗಳು_Op_List ),  // 0xe6  c_opHigh_switch
#ಆದರೆ_ಅಂತ್ಯ

#ಆದರೆ d_m3HasAtomics
    M3OP( "0xFE",            0, c_m3Type_unknown,  d_cc(Compile_ExtendedOpcode), d_ಖಾಲಿಗಳು_Op_List ),  // 0xfe  c_opHigh_atomic
#ಆದರೆ_ಅಂತ್ಯ
    M3OP( "0xFC",            0, c_m3Type_unknown,  d_cc(Compile_ExtendedOpcode), d_ಖಾಲಿಗಳು_Op_List ),  // 0xfc  c_opHigh_extended
};
M3_STATIC_ASSERT (M3_ಎಣಿಕೆ_ದ (c_operations_ಮೇಲಿನ) == c_op_ಮೇಲಿನ_extended + 1, c_operations_ಮೇಲಿನ_needs_one_ನಮೂದು_ಪ್ರತಿ_c_op_ಮೇಲಿನ);

// clang-format on


// Opcodes the spec reserves leave zeroed holes in the tables above: no compiler
// and no operations. Every implemented op has at least one of the two.
ಸ್ಥಿರ ಸಾಲೊಳಗೆ
bool IsImplementedOp (IM3OpInfo i_info)
{
    ಹಿಂತಿರುಗಿ (i_info->compiler != c_cc_NULL or i_info->operations[0] != ಖಾಲಿ);
}

ಸ್ಥಿರಾಂಕ u32 c_ಅಂಕೆ_Operations   = M3_ಎಣಿಕೆ_ದ(c_operations);
ಸ್ಥಿರಾಂಕ u32 c_ಅಂಕೆ_Operations_FC = M3_ಎಣಿಕೆ_ದ(c_operationsFC);

// Maps one of the sparse opcodes above c_waOp_lastCore onto c_operationsHigh,
// or -1 for anything else in that range.
ಸ್ಥಿರ ಸಾಲೊಳಗೆ
i32 Get_ಮೇಲಿನ_Op_ಸೂಚ್ಯಂಕ (m3opcode_t opcode)
{
    ಆಯ್ಕೆ (opcode) {
#ಆದರೆ d_m3HasRefTypes
    ಸ್ಥಿತಿ c_waOp_refNull: ಹಿಂತಿರುಗಿ c_op_ಮೇಲಿನ_ref_Null;
    ಸ್ಥಿತಿ c_waOp_refIsNull: ಹಿಂತಿರುಗಿ c_op_ಮೇಲಿನ_ref_Is_Null;
    ಸ್ಥಿತಿ c_waOp_refFunc: ಹಿಂತಿರುಗಿ c_op_ಮೇಲಿನ_ref_Func;
#  ಆದರೆ d_m3HasTypedRefs
    ಸ್ಥಿತಿ c_waOp_refAsNonNull: ಹಿಂತಿರುಗಿ c_op_ಮೇಲಿನ_ref_As_Non_Null;
#  endif
#ಆದರೆ_ಅಂತ್ಯ
#ಆದರೆ d_m3_Has_ರಾಶಿ_Switching
    ಸ್ಥಿತಿ c_waOp_contNew: ಹಿಂತಿರುಗಿ c_op_ಮೇಲಿನ_cont_New;
    ಸ್ಥಿತಿ c_waOp_contBind: ಹಿಂತಿರುಗಿ c_op_ಮೇಲಿನ_cont_Bind;
    ಸ್ಥಿತಿ c_waOp_suspend: ಹಿಂತಿರುಗಿ c_op_ಮೇಲಿನ_suspend;
    ಸ್ಥಿತಿ c_waOp_resume: ಹಿಂತಿರುಗಿ c_op_ಮೇಲಿನ_resume;
    ಸ್ಥಿತಿ c_waOp_resumeThrow: ಹಿಂತಿರುಗಿ c_op_ಮೇಲಿನ_resume_Throw;
    ಸ್ಥಿತಿ c_waOp_resumeThrowRef: ಹಿಂತಿರುಗಿ c_op_ಮೇಲಿನ_resume_Throw_Ref;
    ಸ್ಥಿತಿ c_waOp_switch: ಹಿಂತಿರುಗಿ c_op_ಮೇಲಿನ_switch;
#ಆದರೆ_ಅಂತ್ಯ
    ಸ್ಥಿತಿ c_waOp_extended: ಹಿಂತಿರುಗಿ c_op_ಮೇಲಿನ_extended;
#ಆದರೆ d_m3HasAtomics
    ಸ್ಥಿತಿ c_waOp_atomic: ಹಿಂತಿರುಗಿ c_op_ಮೇಲಿನ_atomic;
#ಆದರೆ_ಅಂತ್ಯ
    ಪೂರ್ವನಿಯೋಜಿತ: ಹಿಂತಿರುಗಿ -1;
    }
}

IM3OpInfo GetOpInfo (m3opcode_t opcode)
{
    IM3OpInfo info = ಖಾಲಿ;

    ಆಯ್ಕೆ (opcode >> 8) {
    ಸ್ಥಿತಿ 0x00:
        // c_operations is indexed by opcode only up to c_waOp_lastCore; past that
        // it holds internal operations (DEBUG builds), unreachable by opcode.
        ಆದರೆ (M3_LIKELY(opcode <= c_wa_Op_ಕೊನೆಯ_Core)) {
            info = &c_operations[opcode];
        } ಇಲ್ಲದಿದ್ದರೆ {
            i32 ಮೇಲಿನ = Get_ಮೇಲಿನ_Op_ಸೂಚ್ಯಂಕ(opcode);
            ಆದರೆ (ಮೇಲಿನ >= 0) {
                info = &c_operations_ಮೇಲಿನ[ಮೇಲಿನ];
            }
        }
        ನಿಲ್ಲಿಸು;
    ಸ್ಥಿತಿ c_waOp_extended:
        opcode &= 0xFF;
        ಆದರೆ (M3_LIKELY(opcode <= c_wa_Op_ಕೊನೆಯ_Extended)) {
            info = &c_operationsFC[opcode];
        }
        ನಿಲ್ಲಿಸು;
#ಆದರೆ d_m3HasAtomics
    ಸ್ಥಿತಿ c_waOp_atomic: {
        i32 index = Get_Atomic_Info_ಸೂಚ್ಯಂಕ(opcode & 0xFF);
        ಆದರೆ (index >= 0) {
            info = &c_operationsFE[index];
        }
    } ನಿಲ್ಲಿಸು;
#ಆದರೆ_ಅಂತ್ಯ
    }

    ಹಿಂತಿರುಗಿ (info and IsImplementedOp(info)) ? info : ಖಾಲಿ;
}

#ಆದರೆ d_m3HasGasMetering

//----- GAS METERING ------------------------------------------------------------------------------------------------------
//
// Instrumentation the engine applies to a function body as it compiles it,
// following ewasm's metering design, which does the same thing to the Wasm ahead
// of time: https://github.com/ewasm/design/blob/master/metering.md
//
// A suspendable runtime that is not metering still cuts the body into the same
// segments, and records a safepoint wherever a segment would be charged, without
// emitting the charge. A runtime that was metering can stop at any of those when
// its gas runs out, and a snapshot taken there has to be able to go on in one that
// is not metering, or in a build that cannot meter at all.
//
// The body is cut into segments at the instructions that can transfer control,
// and each segment is charged in full, before any of it runs, by an op_UseGas
// carrying the total as an immediate. Prepaying is what makes the accounting
// hold: a call, or a loop iteration that never comes back round, has already
// been paid for by the time it leaves.

// The cost table's unit is a ten-thousandth of a gas, which is why these numbers
// are as large as they are. Two runs of instructions it does not price had to be
// filled in. It has no floating point at all, ewasm having banned it, so float
// instructions are priced as their integer counterparts - division and square
// root as an integer divide, the rest as integer arithmetic. And it predates the
// reference, bulk memory and exception instructions, which are priced as the
// nearest thing it does list: a table access as a memory access, an operation
// over a whole region as memory.grow, a throw as a branch.
ಎಣಿಕೆ {
    c_gasLocal   = 1,          // charged once per declared local, at function entry
    c_gasNominal = 1,          // nop, block, loop, if, const
    c_gasArith   = 45,
    c_gas_ಸರಿಸು   = 67,
    c_gasBranch  = 90,
    c_gas_Mem_ಗಾತ್ರ = 100,
    c_gasAccess  = 120,        // a local, global, table or memory access
    c_gasHeavy   = 10000,      // an indirect call, or anything that works over a whole region
    c_gasDivide  = 36000
};

// The most one op_UseGas can carry. A segment that reaches it is split into
// consecutive charges, which prepays exactly the same amount.
#  define d_m3MaxSegmentGas   0xFFFFFFFF

ಸ್ಥಿರ
u32 Get_Gas_ವೆಚ್ಚ (m3opcode_t i_opcode)
{
    ಆಯ್ಕೆ (i_opcode) {
    ಸ್ಥಿತಿ 0x00:                        // unreachable
    ಸ್ಥಿತಿ 0x01:                        // nop
    ಸ್ಥಿತಿ c_wa_Op_ಖಂಡ:
    ಸ್ಥಿತಿ c_waOp_loop:
    ಸ್ಥಿತಿ c_waOp_if:
    ಸ್ಥಿತಿ c_wa_Op_try_ಕೋಷ್ಟಕ:             // a block that also installs handlers
    ಸ್ಥಿತಿ c_waOp_refNull:              // a constant, by another name
    ಸ್ಥಿತಿ c_waOp_refFunc:
        ಹಿಂತಿರುಗಿ c_gasNominal;

    ಸ್ಥಿತಿ c_wa_Op_ಕೊನೆ:
        ಹಿಂತಿರುಗಿ 0;                     // the one control instruction the table prices at nothing

    ಸ್ಥಿತಿ c_waOp_else:
    ಸ್ಥಿತಿ c_waOp_branch:
    ಸ್ಥಿತಿ c_waOp_branchIf:
    ಸ್ಥಿತಿ c_waOp_return:
    ಸ್ಥಿತಿ c_waOp_call:
    ಸ್ಥಿತಿ c_waOp_returnCall:
    ಸ್ಥಿತಿ c_waOp_throw:
    ಸ್ಥಿತಿ c_waOp_throwRef:
        ಹಿಂತಿರುಗಿ c_gasBranch;

    ಸ್ಥಿತಿ c_wa_Op_branch_ಕೋಷ್ಟಕ:
    ಸ್ಥಿತಿ 0x1a:                        // drop
    ಸ್ಥಿತಿ 0x1b:                        // select
    ಸ್ಥಿತಿ c_waOp_selectTyped:
        ಹಿಂತಿರುಗಿ c_gasAccess;

    ಸ್ಥಿತಿ c_wa_Op_call_ಪರೋಕ್ಷ:
    ಸ್ಥಿತಿ c_wa_Op_return_Call_ಪರೋಕ್ಷ:
    ಸ್ಥಿತಿ c_waOp_callRef:
    ಸ್ಥಿತಿ c_waOp_returnCallRef:
    ಸ್ಥಿತಿ c_wa_Op_ಸ್ಮರಣೆ_Init:
    ಸ್ಥಿತಿ c_wa_Op_ಸ್ಮರಣೆ_Copy:
    ಸ್ಥಿತಿ c_wa_Op_ಸ್ಮರಣೆ_Fill:
    ಸ್ಥಿತಿ c_wa_Op_ಕೋಷ್ಟಕ_Init:
    ಸ್ಥಿತಿ c_wa_Op_ಕೋಷ್ಟಕ_Copy:
    ಸ್ಥಿತಿ c_wa_Op_ಕೋಷ್ಟಕ_Grow:
    ಸ್ಥಿತಿ c_wa_Op_ಕೋಷ್ಟಕ_Fill:
    ಸ್ಥಿತಿ 0x40:                        // memory.grow
        ಹಿಂತಿರುಗಿ c_gasHeavy;

    ಸ್ಥಿತಿ c_wa_Op_ದತ್ತಾಂಶ_Drop:
    ಸ್ಥಿತಿ c_waOp_elemDrop:
    ಸ್ಥಿತಿ c_wa_Op_ಕೋಷ್ಟಕ_ಗಾತ್ರ:
    ಸ್ಥಿತಿ 0x3f:                        // memory.size
        ಹಿಂತಿರುಗಿ c_gas_Mem_ಗಾತ್ರ;

    ಸ್ಥಿತಿ c_waOp_refIsNull:
    ಸ್ಥಿತಿ c_waOp_refAsNonNull:
        ಹಿಂತಿರುಗಿ c_gasArith;

    ಪೂರ್ವನಿಯೋಜಿತ:
        ನಿಲ್ಲಿಸು;
    }

    // The two integer arithmetic runs are priced by position, i32 at 0x6a and
    // i64 at 0x7c, the table giving both the same shape
    ಆದರೆ ((i_opcode >= 0x6a and i_opcode <= 0x78) or (i_opcode >= 0x7c and i_opcode <= 0x8a)) {
        // clang-format off
        ಸ್ಥಿರ ಸ್ಥಿರಾಂಕ u16 c_arithmeticCosts [] = {
            c_gasArith,  c_gasArith,  c_gasArith,                   // add, sub, mul
            c_gasDivide, c_gasDivide, c_gasDivide, c_gasDivide,     // div_s, div_u, rem_s, rem_u
            c_gasArith,  c_gasArith,  c_gasArith,                   // and, or, xor
            c_gas_ಸರಿಸು,  c_gas_ಸರಿಸು,  c_gas_ಸರಿಸು,                   // shl, shr_s, shr_u
            c_gasBranch, c_gasBranch                                // rotl, rotr
        };
        // clang-format on

        ಹಿಂತಿರುಗಿ c_arithmeticCosts[i_opcode - (i_opcode <= 0x78 ? 0x6a : 0x7c)];
    }

    // the atomic accesses are memory accesses; what waits, wakes or orders is worth
    // as much as an operation over a whole region
    ಆದರೆ (i_opcode >= (c_waOp_atomic << 8) and i_opcode <= ((c_waOp_atomic << 8) | c_wa_Op_ಕೊನೆಯ_Atomic)) {
        ಹಿಂತಿರುಗಿ (i_opcode <= c_waOp_atomicFence) ? c_gasHeavy : c_gasAccess;
    }
    ಆದರೆ (i_opcode >= 0x20 and i_opcode <= 0x3e) {
        ಹಿಂತಿರುಗಿ c_gasAccess;           // locals, globals, table accesses, loads and stores
    }
    ಆದರೆ (i_opcode >= 0x41 and i_opcode <= 0x44) {
        ಹಿಂತಿರುಗಿ c_gasNominal;          // i32/i64/f32/f64.const
    }
    ಆದರೆ (i_opcode >= 0x8b and i_opcode <= 0xa6) {
        // the float run, priced as integer arithmetic except where it divides
        bool divides = (i_opcode == 0x91 or i_opcode == 0x95 or     // f32.sqrt, f32.div
                        i_opcode == 0x9f or i_opcode == 0xa3);      // f64.sqrt, f64.div

        ಹಿಂತಿರುಗಿ divides ? c_gasDivide : c_gasArith;
    }

    // comparisons and clz/ctz/popcnt (0x45..0x69, 0x79..0x7b), the conversions
    // and sign extensions (0xa7..0xc4), the saturating truncations, the wide
    // arithmetic, and anything else that reaches here: plain arithmetic
    ಹಿಂತಿರುಗಿ c_gasArith;
}

// The instructions ewasm's design segments a body at. A segment runs up to and
// including the next one of these, so every cycle a body can execute - a loop's
// back edge, a branch out of a block, falling off the end of a function - passes
// through at least one, and no path can run unmetered however it is entered.
ಸ್ಥಿರ
bool Is_Gas_Segment_ಕೊನೆ (m3opcode_t i_opcode)
{
    ಆಯ್ಕೆ (i_opcode) {
    ಸ್ಥಿತಿ c_waOp_loop:
    ಸ್ಥಿತಿ c_waOp_if:
    ಸ್ಥಿತಿ c_waOp_else:
    ಸ್ಥಿತಿ c_wa_Op_ಕೊನೆ:
    ಸ್ಥಿತಿ c_waOp_branch:
    ಸ್ಥಿತಿ c_waOp_branchIf:
    ಸ್ಥಿತಿ c_wa_Op_branch_ಕೋಷ್ಟಕ:
    ಸ್ಥಿತಿ c_waOp_return:
    ಸ್ಥಿತಿ c_waOp_returnCall:
    ಸ್ಥಿತಿ c_wa_Op_return_Call_ಪರೋಕ್ಷ:
    ಸ್ಥಿತಿ c_waOp_returnCallRef:
        ಹಿಂತಿರುಗಿ true;
    ಪೂರ್ವನಿಯೋಜಿತ:
        ಹಿಂತಿರುಗಿ false;
    }
}

// Ends the open segment by writing what it accumulated into the immediate its
// op_UseGas is still waiting on. Emitting onto a later code page does not move
// an earlier one, so the reservation is still where it was left.
ಸ್ಥಿರ
ಶೂನ್ಯ CloseGasSegment (IM3Compilation o)
{
    ಆದರೆ (o->isGasSegmentOpen) {
        ಆದರೆ (o->gasPatch) {
            u32 ವೆಚ್ಚ = o->gas_ವೆಚ್ಚ;
            ಸ್ಮರಣೆ_ನಕಲು(o->gasPatch, &ವೆಚ್ಚ, ಗಾತ್ರ(ವೆಚ್ಚ));

            o->gasPatch = ಖಾಲಿ;
        }
        o->gas_ವೆಚ್ಚ          = 0;
        o->isGasSegmentOpen = false;
    }
}

// Whether this compilation emits the charges
ಸ್ಥಿರ ಸಾಲೊಳಗೆ
bool IsMetering (IM3Compilation o)
{
    ಹಿಂತಿರುಗಿ o->runtime->gas_ಎಲ್ಲೆ != 0;
}

// Charges one instruction to the segment being compiled, opening a segment at
// this point in the code stream if none is open, and settling the segment when
// the instruction is one that ends it. Called before the instruction is
// compiled, which is what both halves need: the op_UseGas lands ahead of what it
// pays for, and the cost is final before a block-structured instruction - loop,
// if, else - compiles its body from inside its own compiler. The body then opens
// a segment of its own, inside the loop or the arm, where it is charged on every
// pass rather than once on the way in.
ಸ್ಥಿರ
M3_ಫಲಿತಾಂಶ MeterOpcode (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_ಫಲಿತಾಂಶ ಫಲಿತಾಂಶ = m3Err_none;

    // constant expressions are not metered: they run once at instantiation,
    // before the module is anything a gas budget was handed out for
    ಆದರೆ (o->function and o->ಪುಟ and IsMetering(o)) {
        // only the 0xFC or 0xFE prefix has been read so far; the instruction it names is
        // the LEB128 u32 the compiler is about to take. Peek it without consuming.
        // Anything the tables do not cover leaves the prefix priced on its own,
        // which is moot - the compile fails on it a moment later.
        ಆದರೆ (i_opcode == c_waOp_extended or i_opcode == c_waOp_atomic) {
            ಬೈಟುಗಳು_t peek = o->wasm;
            u32     sub;
            u32     ಕೊನೆಯ = (i_opcode == c_waOp_atomic) ? c_wa_Op_ಕೊನೆಯ_Atomic : c_wa_Op_ಕೊನೆಯ_Extended;

            ಆದರೆ (not ReadLEB_u32(&sub, &peek, o->wasm_ಕೊನೆ) and sub <= ಕೊನೆಯ) {
                i_opcode = (m3opcode_t)((i_opcode << 8) | sub);
            }
        }

        u32 ವೆಚ್ಚ = Get_Gas_ವೆಚ್ಚ(i_opcode);

        ಆದರೆ (o->isGasSegmentOpen and o->gas_ವೆಚ್ಚ > d_m3MaxSegmentGas - ವೆಚ್ಚ) {
            CloseGasSegment(o);
        }

        ಆದರೆ (not o->isGasSegmentOpen) {
            o->isGasSegmentOpen = true;

_           (EmitOp(o, op_UseGas));
            o->gasPatch = (ಶೂನ್ಯ*)Get_ಪುಟ_PC(o->ಪುಟ);
            EmitConstant32(o, 0);
        }

        o->gas_ವೆಚ್ಚ += ವೆಚ್ಚ;

        ಆದರೆ (Is_Gas_Segment_ಕೊನೆ(i_opcode)) {
            CloseGasSegment(o);
        }
    }

    _catch: ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}

#ಆದರೆ_ಅಂತ್ಯ // d_m3HasGasMetering


M3_ಫಲಿತಾಂಶ Compile_ಖಂಡ_Statements (IM3Compilation o)
{
    M3_ಫಲಿತಾಂಶ ಫಲಿತಾಂಶ   = m3Err_none;
    bool     valid_ಕೊನೆ = false;

    ಆಗ (o->wasm < o->wasm_ಕೊನೆ) {
#ಆದರೆ d_m3EnableOpTracing
        ಆದರೆ (o->ಅಂಕೆ_Emits) {
            EmitOp(o, op_Dump_ರಾಶಿ);
            EmitConstant32(o, o->ಅಂಕೆ_Opcodes);
            EmitConstant32(o, Get_Max_ಬಳಸಿದ_ಸ್ಥಾನ_Plus_One(o));
            Emit_ಸೂಚಕ(o, o->function);

            o->ಅಂಕೆ_Emits = 0;
        }
#ಆದರೆ_ಅಂತ್ಯ

        m3opcode_t opcode;
        o->ಕೊನೆಯ_Opcode_ಆರಂಭ = o->wasm;
_       (Read_opcode(&opcode, &o->wasm, o->wasm_ಕೊನೆ));
        log_opcode(o, opcode);

        // Restrict opcodes when evaluating expressions
        ಆದರೆ (M3_UNLIKELY(not o->function)) {
            // clang-format off
            ಆಯ್ಕೆ (opcode) {
            ಸ್ಥಿತಿ c_waOp_i32_const: ಸ್ಥಿತಿ c_waOp_i64_const:
            ಸ್ಥಿತಿ c_waOp_f32_const: ಸ್ಥಿತಿ c_waOp_f64_const:
            ಸ್ಥಿತಿ c_waOp_getGlobal: ಸ್ಥಿತಿ c_wa_Op_ಕೊನೆ:
#ಆದರೆ d_m3HasRefTypes
            ಸ್ಥಿತಿ c_waOp_refNull:   ಸ್ಥಿತಿ c_waOp_refFunc:
#ಆದರೆ_ಅಂತ್ಯ
#ಆದರೆ d_m3HasExtendedConst
            ಸ್ಥಿತಿ c_wa_Op_i32_ಕೂಡಿಸು:   ಸ್ಥಿತಿ c_waOp_i32_sub:   ಸ್ಥಿತಿ c_waOp_i32_mul:
            ಸ್ಥಿತಿ c_wa_Op_i64_ಕೂಡಿಸು:   ಸ್ಥಿತಿ c_waOp_i64_sub:   ಸ್ಥಿತಿ c_waOp_i64_mul:
#ಆದರೆ_ಅಂತ್ಯ
                ನಿಲ್ಲಿಸು;
            ಪೂರ್ವನಿಯೋಜಿತ:
                _throw(m3Err_restrictedOpcode);
            }
            // clang-format on
        }

        IM3OpInfo opinfo = GetOpInfo(opcode);

        ಆದರೆ (M3_UNLIKELY(opinfo == ಖಾಲಿ)) {
            _throw(ErrorCompile(m3Err_unknownOpcode, o, "opcode '%x' not available", opcode));
        }

#ಆದರೆ d_m3HasGasMetering
_       (MeterOpcode(o, opcode));
#ಆದರೆ_ಅಂತ್ಯ

        ಆದರೆ (opinfo->compiler != c_cc_NULL) {
_           ((*GetCompiler(opinfo))(o, opcode))
        } ಇಲ್ಲದಿದ್ದರೆ {
_           (Compile_Operator(o, opcode));
        }

        o->ಹಿಂದಿನ_Opcode = opcode;

        ಆದರೆ (opcode == c_waOp_else) {
            _throwif(m3Err_wasmMalformed, o->ಖಂಡ.opcode != c_waOp_if);
            valid_ಕೊನೆ = true;
            ನಿಲ್ಲಿಸು;
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (opcode == c_wa_Op_ಕೊನೆ) {
            valid_ಕೊನೆ = true;
            ನಿಲ್ಲಿಸು;
        }
    }
    _throwif(m3Err_wasmMalformed, not valid_ಕೊನೆ);

_catch:
    ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}

ಸ್ಥಿರ
M3_ಫಲಿತಾಂಶ ತಳ್ಳು_ಖಂಡ_Results (IM3Compilation o)
{
    M3_ಫಲಿತಾಂಶ ಫಲಿತಾಂಶ = m3Err_none;

    u16 ಅಂಕೆ_Results = Get_Func_Type_ಅಂಕೆ_Results(o->ಖಂಡ.type);

    ಗಾಗಿ (u16 i = 0; i < ಅಂಕೆ_Results; ++i) {
        m3type_t type = Get_Func_Type_ಫಲಿತಾಂಶ_Type(o->ಖಂಡ.type, i);

        ಆದರೆ (i == ಅಂಕೆ_Results - 1 and IsFpType(type)) {
_           (ತಳ್ಳು_ರಿಜಿಸ್ಟರ್(o, type));
        } ಇಲ್ಲದಿದ್ದರೆ {
_           (ತಳ್ಳು_Allocated_ಸ್ಥಾನ(o, type));
        }
    }

    _catch: ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}


M3_ಫಲಿತಾಂಶ Compile_ಖಂಡ (IM3Compilation o, IM3FuncType i_ಖಂಡ_Type, m3opcode_t i_ಖಂಡ_Opcode)
{
                                                                                        d_m3Assert (not Is_ರಿಜಿಸ್ಟರ್_Allocated (o, 0));
                                                                                        d_m3Assert (not Is_ರಿಜಿಸ್ಟರ್_Allocated (o, 1));
    InvalidateFold(o);

    M3CompilationScope  outerScope = o->ಖಂಡ;
    M3CompilationScope* ಖಂಡ      = &o->ಖಂಡ;

    ಖಂಡ->outer   = &outerScope;
    ಖಂಡ->pc      = Get_ಪುಟ_PC(o->ಪುಟ);
    ಖಂಡ->patches = ಖಾಲಿ;
    ಖಂಡ->type    = i_ಖಂಡ_Type;
    ಖಂಡ->opcode  = i_ಖಂಡ_Opcode;
    ಖಂಡ->ಆಳ++;
    ಖಂಡ->snapshot_ಖಂಡ = 0;

    /*
     The block stack frame is a little strange but for good reasons.  Because blocks need to be restarted to
     compile different pathways (if/else), the incoming params must be saved.  The parameters are popped
     and validated.  But, then the stack top is readjusted so they aren't subsequently overwritten.
     Next, the result are preallocated to find destination slots.  But again these are immediately popped
     (deallocated) and the stack top is readjusted to keep these records in pace. This allows branch instructions
     to find their result landing pads.  Finally, the params are copied from the "dead" records and pushed back
     onto the stack as active stack items for the CompileBlockStatements () call.

    [     block      ]
    [     params     ]
    ------------------
    [     result     ]  <---- blockStackIndex
    [      slots     ]
    ------------------
    [   saved param  ]
    [     records    ]
                        <----- exitStackIndex
    */

_try {
#ಆದರೆ d_m3HasSnapshots
    // op_Loop's frame records the body, which is also the loop's identity;
    // op_TryTable's records its clause table, two words a clause back from it
    ಆದರೆ (i_ಖಂಡ_Opcode == c_waOp_loop) {
_       (ದಾಖಲೆ_ಖಂಡ_ಆರಂಭ(o, i_ಖಂಡ_Opcode, ಖಂಡ->pc, 0));
    }
#  ಆದರೆ d_m3HasExceptionHandling
    ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (i_ಖಂಡ_Opcode == c_wa_Op_try_ಕೋಷ್ಟಕ) {
_       (ದಾಖಲೆ_ಖಂಡ_ಆರಂಭ(o, i_ಖಂಡ_Opcode, ಖಂಡ->pc - 2 * o->ಅಂಕೆ_Try_Clauses, o->ಅಂಕೆ_Try_Clauses));
    }
#  endif
#ಆದರೆ_ಅಂತ್ಯ

    // validate and dealloc params ----------------------------

    u16 ರಾಶಿ_ಸೂಚ್ಯಂಕ = o->ರಾಶಿ_ಸೂಚ್ಯಂಕ;

    u16 ಅಂಕೆ_Params = Get_Func_Type_ಅಂಕೆ_Params(i_ಖಂಡ_Type);

    ಆದರೆ (i_ಖಂಡ_Opcode != c_waOp_else) {
        ಗಾಗಿ (u16 i = 0; i < ಅಂಕೆ_Params; ++i) {
            m3type_t type = GetFuncTypeParamType(i_ಖಂಡ_Type, ಅಂಕೆ_Params - 1 - i);
_           (ತೆಗೆ_Type(o, type));
        }
    } ಇಲ್ಲದಿದ್ದರೆ {
        ಆದರೆ (Is_ರಾಶಿ_Polymorphic(o) && o->ಖಂಡ.ಖಂಡ_ರಾಶಿ_ಸೂಚ್ಯಂಕ + ಅಂಕೆ_Params > o->ರಾಶಿ_ಸೂಚ್ಯಂಕ) {
            o->ರಾಶಿ_ಸೂಚ್ಯಂಕ = o->ಖಂಡ.ಖಂಡ_ರಾಶಿ_ಸೂಚ್ಯಂಕ;
        } ಇಲ್ಲದಿದ್ದರೆ {
            o->ರಾಶಿ_ಸೂಚ್ಯಂಕ -= ಅಂಕೆ_Params;
        }
    }

    u16 param_ಸೂಚ್ಯಂಕ        = o->ರಾಶಿ_ಸೂಚ್ಯಂಕ;
    ಖಂಡ->exit_ರಾಶಿ_ಸೂಚ್ಯಂಕ = param_ಸೂಚ್ಯಂಕ; // consume the params at block exit

    // keep copies of param slots in the stack
    o->ರಾಶಿ_ಸೂಚ್ಯಂಕ = ರಾಶಿ_ಸೂಚ್ಯಂಕ;

    // find slots for the results ----------------------------
_   (ತಳ್ಳು_ಖಂಡ_Results(o));

    ರಾಶಿ_ಸೂಚ್ಯಂಕ = o->ರಾಶಿ_ಸೂಚ್ಯಂಕ;

    // dealloc but keep record of the result slots in the stack
    u16 ಅಂಕೆ_Results = Get_Func_Type_ಅಂಕೆ_Results(i_ಖಂಡ_Type);
    ಆಗ (ಅಂಕೆ_Results--) {
        ತೆಗೆ(o);
    }

    ಖಂಡ->ಖಂಡ_ರಾಶಿ_ಸೂಚ್ಯಂಕ = o->ರಾಶಿ_ಸೂಚ್ಯಂಕ = ರಾಶಿ_ಸೂಚ್ಯಂಕ;

    // push the params back onto the stack -------------------
    ಗಾಗಿ (u16 i = 0; i < ಅಂಕೆ_Params; ++i) {
        m3type_t type = GetFuncTypeParamType(i_ಖಂಡ_Type, i);

        u16 ಸ್ಥಾನ = Get_ಸ್ಥಾನ_For_ರಾಶಿ_ಸೂಚ್ಯಂಕ(o, param_ಸೂಚ್ಯಂಕ + i);
        ತಳ್ಳು(o, type, ಸ್ಥಾನ);

        ಆದರೆ (ಸ್ಥಾನ >= o->ಸ್ಥಾನ_ಮೊದಲ_Dynamic_ಸೂಚ್ಯಂಕ && ಸ್ಥಾನ != c_ಸ್ಥಾನ_ಬಳಸದ) {
_           (MarkSlotsAllocatedByType(o, ಸ್ಥಾನ, type));
        }
    }

    //--------------------------------------------------------

#ಆದರೆ d_m3HasExceptionHandling
    // with the scope pushed and the params back on, a catch label of 0 names
    // this block - which is what the proposal says it should
    ಆದರೆ (i_ಖಂಡ_Opcode == c_wa_Op_try_ಕೋಷ್ಟಕ) {
_       (EmitCatchStubs(o));
    }
#ಆದರೆ_ಅಂತ್ಯ

_   (Compile_ಖಂಡ_Statements(o));

_   (Validate_ಖಂಡ_ಕೊನೆ(o));

    ಆದರೆ (o->function)    // skip for expressions
    {
        ಆದರೆ (not Is_ರಾಶಿ_Polymorphic(o)) {
_           (Resolve_ಖಂಡ_Results(o, &o->ಖಂಡ, /* isBranch: */ false));
        }

#ಆದರೆ d_m3HasExceptionHandling
        // Falling out of the end of a try region retires its handler. This sits
        // ahead of PatchBranches so branches into the block's exit land past it:
        // a branch out of the try popped its own handler at the branch site.
        ಆದರೆ (i_ಖಂಡ_Opcode == c_wa_Op_try_ಕೋಷ್ಟಕ) {
_           (EmitOp(o, op_ತೆಗೆ_Handlers));
            EmitConstant32(o, 1);
        }
#ಆದರೆ_ಅಂತ್ಯ

_       (Unwind_ಖಂಡ_ರಾಶಿ(o))

        ಆದರೆ (not ((i_ಖಂಡ_Opcode == c_waOp_if and ಅಂಕೆ_Results) or o->ಹಿಂದಿನ_Opcode == c_waOp_else)) {
            o->ರಾಶಿ_ಸೂಚ್ಯಂಕ = o->ಖಂಡ.exit_ರಾಶಿ_ಸೂಚ್ಯಂಕ;
_           (ತಳ್ಳು_ಖಂಡ_Results(o));
        }
    }

    PatchBranches(o);

#ಆದರೆ d_m3HasSnapshots
    ದಾಖಲೆ_ಖಂಡ_ಕೊನೆ(o);

    ಆದರೆ (o->local_Init_ಆಳ) {
        ಗಾಗಿ (u32 i = 0; i < o->snapshotMap->ಅಂಕೆ_Locals; ++i) {
            ಆದರೆ (o->local_Init_ಆಳ[i] >= (u32)o->ಖಂಡ.ಆಳ) {
                o->local_Init_ಆಳ[i] = UINT32_MAX;
            }
        }
    }
#ಆದರೆ_ಅಂತ್ಯ
    o->ಖಂಡ = outerScope;

} _catch: ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}

ಸ್ಥಿರ
M3_ಫಲಿತಾಂಶ CompileLocals (IM3Compilation o)
{
    M3_ಫಲಿತಾಂಶ ಫಲಿತಾಂಶ;

    u32 ಅಂಕೆ_Locals = 0;
    u32 ಅಂಕೆ_Local_ಖಂಡಗಳು;
_   (ReadLEB_u32(&ಅಂಕೆ_Local_ಖಂಡಗಳು, &o->wasm, o->wasm_ಕೊನೆ));

    ಗಾಗಿ (u32 l = 0; l < ಅಂಕೆ_Local_ಖಂಡಗಳು; ++l) {
        u32      var_ಎಣಿಕೆ;
        m3type_t localType;

_       (ReadLEB_u32(&var_ಎಣಿಕೆ, &o->wasm, o->wasm_ಕೊನೆ));
_       (Parse_ಮೌಲ್ಯ_Type(o->module, &localType, &o->wasm, o->wasm_ಕೊನೆ));
        ಅಂಕೆ_Locals += var_ಎಣಿಕೆ;                                                          m3log (compile, "pushing locals. count: %d; type: %s", var_ಎಣಿಕೆ, c_waTypes [ತಳ_Type_ದ(localType)]);
        ಆಗ (var_ಎಣಿಕೆ--) {
_           (ತಳ್ಳು_Allocated_ಸ್ಥಾನ(o, localType));
        }
    }

    ಆದರೆ (o->function) {
        o->function->ಅಂಕೆ_Locals = ಅಂಕೆ_Locals;
    }

    _catch: ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}

ಸ್ಥಿರ
M3_ಫಲಿತಾಂಶ ReserveConstants (IM3Compilation o)
{
    M3_ಫಲಿತಾಂಶ ಫಲಿತಾಂಶ = m3Err_none;

    // in the interest of speed, this blindly scans the Wasm code looking for any byte
    // that looks like an const opcode.
    u16 ಅಂಕೆ_Constant_Slots = 0;

    ಬೈಟುಗಳು_t wa = o->wasm;
    ಆಗ (wa < o->wasm_ಕೊನೆ) {
        u8  ಸಂಕೇತ_2     = *wa++;
        u16 ಕೂಡಿಸು_Slots = 0;

        ಆದರೆ (ಸಂಕೇತ_2 == c_waOp_i32_const or ಸಂಕೇತ_2 == c_waOp_f32_const) {
            ಕೂಡಿಸು_Slots = 1;
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (ಸಂಕೇತ_2 == c_waOp_i64_const or ಸಂಕೇತ_2 == c_waOp_f64_const) {
            ಕೂಡಿಸು_Slots = Get_Type_ಅಂಕೆ_Slots(c_m3Type_i64);
        }

        ಆದರೆ (ಅಂಕೆ_Constant_Slots + ಕೂಡಿಸು_Slots >= d_m3_Max_Constant_ಕೋಷ್ಟಕ_ಗಾತ್ರ) {
            ನಿಲ್ಲಿಸು;
        }

        ಅಂಕೆ_Constant_Slots += ಕೂಡಿಸು_Slots;
    }

    // if constants overflow their reserved stack space, the compiler simply emits op_Const
    // operations as needed. Compiled expressions (global inits) don't pass through this
    // ReserveConstants function and thus always produce inline constants.

    Align_ಸ್ಥಾನ_ಗಮ್ಯ_Type(&ಅಂಕೆ_Constant_Slots, c_m3Type_i64);                                           m3log (compile, "reserved constant slots: %d", ಅಂಕೆ_Constant_Slots);

    o->ಸ್ಥಾನ_ಮೊದಲ_Dynamic_ಸೂಚ್ಯಂಕ = o->ಸ್ಥಾನ_ಮೊದಲ_Const_ಸೂಚ್ಯಂಕ + ಅಂಕೆ_Constant_Slots;

    ಆದರೆ (o->ಸ್ಥಾನ_ಮೊದಲ_Dynamic_ಸೂಚ್ಯಂಕ >= d_m3MaxFunctionSlots) {
        _throw(m3_Err_function_ರಾಶಿ_ಅತಿಪ್ರವಾಹ);
    }

    _catch:
    ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}


// A constant expression compiles like a miniature function root block: no args,
// no locals and a single result, which Compile_End copies down into slot 0 for
// EvaluateExpression to read back.
M3_ಫಲಿತಾಂಶ Compile_ಅಭಿವ್ಯಕ್ತಿ (IM3Compilation o, IM3FuncType i_ಫಲಿತಾಂಶ_Type)
{
    M3_ಫಲಿತಾಂಶ ಫಲಿತಾಂಶ = m3Err_none;

    o->ಖಂಡ.type = i_ಫಲಿತಾಂಶ_Type;

    u16 ಅಂಕೆ_Ret_Slots = Get_Func_Type_ಅಂಕೆ_Results(i_ಫಲಿತಾಂಶ_Type) * c_io_ಸ್ಥಾನ_ಎಣಿಕೆ;

    ಗಾಗಿ (u16 i = 0; i < ಅಂಕೆ_Ret_Slots; ++i) {
        Mark_ಸ್ಥಾನ_Allocated(o, i);
    }

    o->max_ರಾಶಿ_Slots = o->ಸ್ಥಾನ_Max_Allocated_ಸೂಚ್ಯಂಕ_Plus_One = o->ಸ್ಥಾನ_ಮೊದಲ_Dynamic_ಸೂಚ್ಯಂಕ = ಅಂಕೆ_Ret_Slots;

_   (Compile_ಖಂಡ_Statements(o));

    _catch: ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}


M3_ಫಲಿತಾಂಶ CompileFunction (IM3Function io_function)
{
    ಆದರೆ (!io_function->wasm) {
        // An import. Either linking pointed it at another module's function -
        // in which case that one carries the body - or a host function was
        // bound to it, or it is simply unsatisfied.
        IM3Function impl = Function_Implementation(io_function);

        ಆದರೆ (impl != io_function) {
            ಆದರೆ (not impl->compiled) {
                M3_ಫಲಿತಾಂಶ r = CompileFunction(impl);
                ಆದರೆ (r) {
                    ಹಿಂತಿರುಗಿ r;
                }
            }

            // keep the placeholder callable too, for anything still holding it
            io_function->compiled = impl->compiled;
            ಹಿಂತಿರುಗಿ m3Err_none;
        }

        ಆದರೆ (io_function->compiled) {
            ಹಿಂತಿರುಗಿ m3Err_none;
        }

        ಹಿಂತಿರುಗಿ ErrorModule(m3Err_functionImportMissing, io_function->module, "'%s.%s'",
                           GetFunctionImportModuleName(io_function),
                           m3_GetFunctionName(io_function));
    }

#ಆದರೆ d_m3EnableValidation
    ಆದರೆ (not io_function->module->runtime->skipValidation) {
        M3_ಫಲಿತಾಂಶ vr = ValidateFunction(io_function);
        ಆದರೆ (vr) {
            ಹಿಂತಿರುಗಿ vr;
        }
    }
#ಆದರೆ_ಅಂತ್ಯ

    IM3FuncType funcType = io_function->funcType;                   m3log (compile, "compiling: [%d] %s %s; wasm-size: %d",
                                                                        io_function->index, m3_GetFunctionName (io_function), SPrintFuncTypeSignature (funcType), (u32) (io_function->wasm_ಕೊನೆ - io_function->wasm));
    IM3Runtime runtime = io_function->module->runtime;

    IM3Compilation o = &runtime->compilation;                       d_m3Assert (d_m3MaxFunctionSlots >= d_m3_Max_Function_ರಾಶಿ_ಎತ್ತರ * (d_m3_Use32_ಬಿಟ್_Slots + 1))  // need twice as many slots in 32-bit mode
    memset(o, 0x0, ಗಾತ್ರ(M3Compilation));

    o->runtime    = runtime;
    o->module     = io_function->module;
    o->function   = io_function;
    o->wasm       = io_function->wasm;
    o->wasm_ಕೊನೆ    = io_function->wasm_ಕೊನೆ;
    o->ಖಂಡ.type = funcType;

_try {
    // skip over code size. the end was already calculated during parse phase
    u32 ಗಾತ್ರ_2;
_   (ReadLEB_u32(&ಗಾತ್ರ_2, &o->wasm, o->wasm_ಕೊನೆ));                     d_m3Assert (ಗಾತ್ರ_2 == (o->wasm_ಕೊನೆ - o->wasm))

#ಆದರೆ d_m3HasSnapshots
    o->body_ಆರಂಭ = o->wasm;
#ಆದರೆ_ಅಂತ್ಯ

#ಆದರೆ d_m3HasSnapshots
    // only code that can be suspended can end up in a snapshot
    ಆದರೆ (runtime->isSuspendable) {
        o->snapshotMap = m3_AllocStruct(M3SnapshotMap);
        _throwifnull(o->snapshotMap);
    }
#ಆದರೆ_ಅಂತ್ಯ

_   (Acquire_Compilation_ಸಂಕೇತ_ಪುಟ(o, &o->ಪುಟ));

    pc_t pc = Get_ಪುಟ_PC(o->ಪುಟ);

    u16 ಅಂಕೆ_Ret_Slots = Get_Function_ಅಂಕೆ_Returns(o->function) * c_io_ಸ್ಥಾನ_ಎಣಿಕೆ;

    ಗಾಗಿ (u16 i = 0; i < ಅಂಕೆ_Ret_Slots; ++i) {
        Mark_ಸ್ಥಾನ_Allocated(o, i);
    }

    o->function->ಅಂಕೆ_Ret_Slots = o->ಸ್ಥಾನ_ಮೊದಲ_Dynamic_ಸೂಚ್ಯಂಕ = ಅಂಕೆ_Ret_Slots;

    u16 ಅಂಕೆ_ವಾದಗಳು = Get_Function_ಅಂಕೆ_ವಾದಗಳು(o->function);

    // push the arg types to the type stack
    ಗಾಗಿ (u16 i = 0; i < ಅಂಕೆ_ವಾದಗಳು; ++i) {
        m3type_t type = GetFunctionArgType(o->function, i);
_       (ತಳ್ಳು_Allocated_ಸ್ಥಾನ(o, type));

        // prevent allocator fill-in
        o->ಸ್ಥಾನ_ಮೊದಲ_Dynamic_ಸೂಚ್ಯಂಕ += c_io_ಸ್ಥಾನ_ಎಣಿಕೆ;
    }

    o->ಸ್ಥಾನ_Max_Allocated_ಸೂಚ್ಯಂಕ_Plus_One = o->function->ಅಂಕೆ_Ret_And_Arg_Slots = o->ಸ್ಥಾನ_ಮೊದಲ_Local_ಸೂಚ್ಯಂಕ = o->ಸ್ಥಾನ_ಮೊದಲ_Dynamic_ಸೂಚ್ಯಂಕ;

_   (CompileLocals(o));

#ಆದರೆ d_m3HasSnapshots
_   (ದಾಖಲೆ_Locals(o));
#ಆದರೆ_ಅಂತ್ಯ

#ಆದರೆ d_m3HasGasMetering
    // the table charges for a function's locals as well as for its instructions.
    // op_Entry is what sets them up, so the charge rides on the body's first
    // segment, which is emitted right after it
    o->gas_ವೆಚ್ಚ = o->function->ಅಂಕೆ_Locals * c_gasLocal;
#ಆದರೆ_ಅಂತ್ಯ

    u16 max_ಸ್ಥಾನ = Get_Max_ಬಳಸಿದ_ಸ್ಥಾನ_Plus_One(o);

    o->function->ಅಂಕೆ_Local_ಬೈಟುಗಳು = (max_ಸ್ಥಾನ - o->ಸ್ಥಾನ_ಮೊದಲ_Local_ಸೂಚ್ಯಂಕ) * ಗಾತ್ರ(m3slot_t);

    o->ಸ್ಥಾನ_ಮೊದಲ_Const_ಸೂಚ್ಯಂಕ = o->ಸ್ಥಾನ_Max_Const_ಸೂಚ್ಯಂಕ = max_ಸ್ಥಾನ;

    // ReserveConstants initializes o->firstDynamicSlotNumber
_   (ReserveConstants(o));

    // start tracking the max stack used (Push() also updates this value) so that op_Entry can precisely detect stack overflow
    o->max_ರಾಶಿ_Slots = o->ಸ್ಥಾನ_Max_Allocated_ಸೂಚ್ಯಂಕ_Plus_One = o->ಸ್ಥಾನ_ಮೊದಲ_Dynamic_ಸೂಚ್ಯಂಕ;

    o->ಖಂಡ.ಖಂಡ_ರಾಶಿ_ಸೂಚ್ಯಂಕ = o->ರಾಶಿ_ಮೊದಲ_Dynamic_ಸೂಚ್ಯಂಕ = o->ರಾಶಿ_ಸೂಚ್ಯಂಕ;                           m3log (compile, "start stack index: %u; max stack slots: %u",
                                                                                                           (u32) o->ರಾಶಿ_ಮೊದಲ_Dynamic_ಸೂಚ್ಯಂಕ, (u32) o->max_ರಾಶಿ_Slots);
#ಆದರೆ d_m3_Has_ರಾಶಿ_Switching
    // A suspendable body opens by checking for a pause request, once its frame
    // is set up. The pause point's offset is the first instruction's, just past
    // the local declarations - the body's end, when it has no other.
    ಆದರೆ (runtime->isSuspendable) {
        o->ಕೊನೆಯ_Opcode_ಆರಂಭ = o->wasm;
#  ಆದರೆ d_m3_ನಮೂದು_Keeps_ಚೌಕಟ್ಟು
_       (EmitOp(o, op_ನಮೂದು));
        Emit_ಸೂಚಕ(o, io_function);
_       (EmitOp(o, op_ನಮೂದು_Check));
_       (ದಾಖಲೆ_ಸುರಕ್ಷಿತ_ಬಿಂದು(o, safepoint_ನಮೂದು, 1, 0, 0, ಖಾಲಿ));
#  ಇಲ್ಲದಿದ್ದರೆ
_       (EmitOp(o, op_ನಮೂದು_Suspendable));
        Emit_ಸೂಚಕ(o, io_function);
_       (ದಾಖಲೆ_ಸುರಕ್ಷಿತ_ಬಿಂದು(o, safepoint_ನಮೂದು, 0, 0, 0, ಖಾಲಿ));
#  endif
    } ಇಲ್ಲದಿದ್ದರೆ
#ಆದರೆ_ಅಂತ್ಯ
    {
_       (EmitOp(o, op_ನಮೂದು));
        Emit_ಸೂಚಕ(o, io_function);
    }

_   (Compile_ಖಂಡ_Statements(o));

    _throwif(m3Err_wasmMalformed, o->ಹಿಂದಿನ_Opcode != c_wa_Op_ಕೊನೆ);

    io_function->compiled      = pc;
    io_function->max_ರಾಶಿ_Slots = o->max_ರಾಶಿ_Slots;

    u16 ಅಂಕೆ_Constant_Slots = o->ಸ್ಥಾನ_Max_Const_ಸೂಚ್ಯಂಕ - o->ಸ್ಥಾನ_ಮೊದಲ_Const_ಸೂಚ್ಯಂಕ;                           m3log (compile, "unique constant slots: %u; unused slots: %u",
                                                                                                           ಅಂಕೆ_Constant_Slots, o->ಸ್ಥಾನ_ಮೊದಲ_Dynamic_ಸೂಚ್ಯಂಕ - o->ಸ್ಥಾನ_Max_Const_ಸೂಚ್ಯಂಕ);
    io_function->ಅಂಕೆ_Constant_ಬೈಟುಗಳು = ಅಂಕೆ_Constant_Slots * ಗಾತ್ರ(m3slot_t);

    ಆದರೆ (ಅಂಕೆ_Constant_Slots) {
        io_function->constants = m3_CopyMem((cbytes_t)o->constants, io_function->ಅಂಕೆ_Constant_ಬೈಟುಗಳು);
        _throwifnull(io_function->constants);
    }

#ಆದರೆ d_m3HasSnapshots
    io_function->snapshotMap = o->snapshotMap;
    o->snapshotMap           = ಖಾಲಿ;
#ಆದರೆ_ಅಂತ್ಯ

} _catch:

    Release_Compilation_ಸಂಕೇತ_ಪುಟ(o);

#ಆದರೆ d_m3HasSnapshots
    SnapshotMap_Free(o->snapshotMap);
    o->snapshotMap = ಖಾಲಿ;
    m3_Free(o->local_Init_ಆಳ);
#ಆದರೆ_ಅಂತ್ಯ

    ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}

#ಆದರೆ d_m3_Has_ರಾಶಿ_Switching
m3ret_t ResumeContinuation (IM3Runtime i_runtime, IM3Continuation i_cont)
{
    ಆದರೆ (M3_UNLIKELY(!i_cont || i_cont->state != cont_suspended)) {
        ಹಿಂತಿರುಗಿ m3Err_none;
    }

    M3_ಸ್ಮರಣೆ_ಶೀರ್ಷಿಕೆ* _mem = i_cont->ನಮೂದು_Function ? Module_ಸ್ಮರಣೆ_ಶೀರ್ಷಿಕೆ(i_cont->ನಮೂದು_Function->module) : ಖಾಲಿ;

    i_runtime->ಸಕ್ರಿಯ_Continuation = i_cont;
    i_cont->state                 = cont_running;

    i32 outermost     = (i32)i_cont->ಅಂಕೆ_ಚೌಕಟ್ಟುಗಳು - 1;
    i_cont->ಅಂಕೆ_ಚೌಕಟ್ಟುಗಳು = 0;

    // with no frames standing, this lands straight on the suspension point
    m3ret_t r = Replay_ಚೌಕಟ್ಟುಗಳು(i_cont, outermost, _mem);

    ಆದರೆ (r == m3Err_continuationSuspended) {
        i_cont->state = cont_suspended;
    } ಇಲ್ಲದಿದ್ದರೆ {
        i_cont->state = cont_returned;
    }
    i_runtime->ಸಕ್ರಿಯ_Continuation = ಖಾಲಿ;

    ಹಿಂತಿರುಗಿ r;
}
#ಆದರೆ_ಅಂತ್ಯ
