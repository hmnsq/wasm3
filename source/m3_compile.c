//
//  m3_compile.c
//
//  Created by Steven Massey on 4/17/19.
//  Copyright © 2019 Steven Massey. All rights reserved.
//

// Allow using opcodes for compilation process
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ M3_COMPILE_OPCODES

#ਅਨ੍ਤਰ੍ਭਾਵ "m3_env.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "m3_compile.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "m3_exec.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "m3_exception.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "m3_info.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "m3_validate.h"

// Resolves M3OpInfo.compiler, an index, to the function it names. Defined down
// with c_compilers [], which has to follow every compiler it points at.
ਸਥਿਰ
M3Compiler GetCompiler (IM3OpInfo i_opInfo);

//----- EMIT --------------------------------------------------------------------------------------------------------------

ਸਥਿਰ ਸਤਰਅੰਦਰ
pc_t GetPC (IM3Compilation o)
{
    ਵਾਪਸ Get_ਪੰਨਾ_PC(o->ਪੰਨਾ);
}

#ਜੇ d_m3HasSnapshots

#  define d_m3GrowSnapshotArray(TYPE, ARRAY, ਗਿਣਤੀ_3, ਸਮਰੱਥਾ_2)                    \
    ਜੇ ((ਗਿਣਤੀ_3) == (ਸਮਰੱਥਾ_2)) {                                                 \
        u32   new_ਸਮਰੱਥਾ = (ਸਮਰੱਥਾ_2) ? (ਸਮਰੱਥਾ_2) * 2 : 8;                     \
        TYPE* grown = m3_ReallocArray(TYPE, (ARRAY), new_ਸਮਰੱਥਾ, (ਸਮਰੱਥਾ_2));   \
        ਜੇ (not grown) {                                                         \
            ਵਾਪਸ m3Err_mallocFailed;                                           \
        }                                                                        \
        (ARRAY)    = grown;                                                      \
        (ਸਮਰੱਥਾ_2) = new_ਸਮਰੱਥਾ;                                                \
    }

// Where the instruction being compiled starts, counted from the start of the
// function's body: the one position every build of every revision agrees on
ਸਥਿਰ ਸਤਰਅੰਦਰ
u32 Get_Wasm_ਵਿੱਥ (IM3Compilation o)
{
    ਵਾਪਸ (u32)(o->ਆਖਰੀ_Opcode_ਸ਼ੁਰੂ - o->body_ਸ਼ੁਰੂ);
}

// Whether the operand stack entry at i_index is a value the frame holds. Every
// block keeps records of its parameter and result slots on the stack, between
// the height it was entered at and where its own values begin (see
// CompileBlock). Those are landing pads rather than values: nothing need have
// written them yet, and the slots they name may be in use for something else.
// The function's own scope has none - that stretch is its arguments and locals.
ਸਥਿਰ
bool Is_ਜੀਵਤ_ਢੇਰ_ਇੰਦਰਾਜ (IM3Compilation o, u16 i_ਸੂਚਕਾਂਕ)
{
    ਲਈ (IM3CompilationScope scope = &o->ਖੰਡ; scope->outer; scope = scope->outer) {
        ਜੇ (i_ਸੂਚਕਾਂਕ >= scope->exit_ਢੇਰ_ਸੂਚਕਾਂਕ and i_ਸੂਚਕਾਂਕ < scope->ਖੰਡ_ਢੇਰ_ਸੂਚਕਾਂਕ) {
            ਵਾਪਸ false;
        }
    }

    ਵਾਪਸ true;
}

ਸਥਿਰ
M3_ਨਤੀਜਾ ਜੋੜੋ_ਸੁਰੱਖਿਅਤ_ਬਿੰਦੂ_ਮੁੱਲ (IM3Compilation o, M3_ਸੁਰੱਖਿਅਤ_ਬਿੰਦੂ* io_ਬਿੰਦੂ, u16 i_ਸੂਚਕਾਂਕ)
{
    M3SnapshotMap* map = o->snapshotMap;

    d_m3GrowSnapshotArray(M3_ਥਾਂ_ਮੁੱਲ, map->ਮੁੱਲਾਂ, map->ਅੰਕ_ਮੁੱਲਾਂ, o->ਮੁੱਲਾਂ_ਸਮਰੱਥਾ);

    map->ਮੁੱਲਾਂ[map->ਅੰਕ_ਮੁੱਲਾਂ].ਥਾਂ = o->wasm_ਢੇਰ[i_ਸੂਚਕਾਂਕ];
    map->ਮੁੱਲਾਂ[map->ਅੰਕ_ਮੁੱਲਾਂ].type = o->type_ਢੇਰ[i_ਸੂਚਕਾਂਕ];
    map->ਅੰਕ_ਮੁੱਲਾਂ++;
    io_ਬਿੰਦੂ->ਅੰਕ_ਮੁੱਲਾਂ++;

    ਵਾਪਸ m3Err_none;
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
ਸਥਿਰ
M3_ਨਤੀਜਾ ਰਿਕਾਰਡ_ਸੁਰੱਖਿਅਤ_ਬਿੰਦੂ (IM3Compilation o, M3_ਸੁਰੱਖਿਅਤ_ਬਿੰਦੂ_ਕਿਸਮ i_ਕਿਸਮ, u32 i_ਸ਼ਬਦ_ਪਿੱਠ, u16 i_ਅੰਕ_Results,
                          u16 i_aux, IM3CompilationScope i_loop)
{
_try {
    M3SnapshotMap* map = o->snapshotMap;

    ਜੇ (not map or not o->ਪੰਨਾ) {
        ਵਾਪਸ m3Err_none;
    }

    d_m3GrowSnapshotArray(M3_ਸੁਰੱਖਿਅਤ_ਬਿੰਦੂ, map->ਸੁਰੱਖਿਅਤ_Points, map->ਅੰਕ_ਸੁਰੱਖਿਅਤ_Points, o->ਸੁਰੱਖਿਅਤ_Points_ਸਮਰੱਥਾ);

    // grown before it is pointed at: adding values never moves the safepoints
    M3_ਸੁਰੱਖਿਅਤ_ਬਿੰਦੂ* ਬਿੰਦੂ     = &map->ਸੁਰੱਖਿਅਤ_Points[map->ਅੰਕ_ਸੁਰੱਖਿਅਤ_Points];
    ਬਿੰਦੂ->pc              = GetPC(o) - i_ਸ਼ਬਦ_ਪਿੱਠ;
    ਬਿੰਦੂ->wasm_ਵਿੱਥ      = Get_Wasm_ਵਿੱਥ(o);
    ਬਿੰਦੂ->ਪਹਿਲਾ_ਮੁੱਲ      = map->ਅੰਕ_ਮੁੱਲਾਂ;
    ਬਿੰਦੂ->ਪਹਿਲਾ_Local_ਮੁੱਲ = UINT32_MAX;
    ਬਿੰਦੂ->ਅੰਕ_ਮੁੱਲਾਂ       = 0;
    ਬਿੰਦੂ->ਅੰਕ_Results      = i_ਅੰਕ_Results;
    ਬਿੰਦੂ->aux             = i_aux;
    ਬਿੰਦੂ->resumeType      = c_m3Type_none;
    ਬਿੰਦੂ->ਕਿਸਮ            = (u8)i_ਕਿਸਮ;
    ਬਿੰਦੂ->flags           = 0;

    // Non-defaultable locals can still be uninitialized at a safepoint. Keep
    // their heap types, but allow the null placeholder until local.set makes
    // them definitely initialized. Branches to a loop use its entry context.
    ਜੇ (o->local_Init_ਡੂੰਘਾਈ) {
        ਬਿੰਦੂ->ਪਹਿਲਾ_Local_ਮੁੱਲ = map->ਅੰਕ_ਮੁੱਲਾਂ;
        ਲਈ (u32 i = 0; i < map->ਅੰਕ_Locals; ++i) {
            d_m3GrowSnapshotArray(M3_ਥਾਂ_ਮੁੱਲ, map->ਮੁੱਲਾਂ, map->ਅੰਕ_ਮੁੱਲਾਂ, o->ਮੁੱਲਾਂ_ਸਮਰੱਥਾ);
            M3_ਥਾਂ_ਮੁੱਲ ਮੁੱਲ = map->locals[i];
            ਜੇ (o->local_Init_ਡੂੰਘਾਈ[i] == UINT32_MAX or
                (i_loop and o->local_Init_ਡੂੰਘਾਈ[i] >= (u32)i_loop->ਡੂੰਘਾਈ)) {
                ਮੁੱਲ.type &= ~d_m3Type_refNonNull;
            }
            map->ਮੁੱਲਾਂ[map->ਅੰਕ_ਮੁੱਲਾਂ++] = ਮੁੱਲ;
        }
        ਬਿੰਦੂ->ਪਹਿਲਾ_ਮੁੱਲ = map->ਅੰਕ_ਮੁੱਲਾਂ;
    }

    u16 ਸਿਖਰ = o->ਢੇਰ_ਸੂਚਕਾਂਕ;

    ਜੇ (i_ਕਿਸਮ != safepoint_suspend) {
        ਸਿਖਰ = (ਸਿਖਰ > i_ਅੰਕ_Results) ? (u16)(ਸਿਖਰ - i_ਅੰਕ_Results) : 0;
    }
    ਜੇ (i_loop) {
        ਸਿਖਰ = M3_MIN(ਸਿਖਰ, i_loop->exit_ਢੇਰ_ਸੂਚਕਾਂਕ);
    }

    ਲਈ (u16 i = o->ਢੇਰ_ਪਹਿਲਾ_Dynamic_ਸੂਚਕਾਂਕ; i < ਸਿਖਰ; ++i) {
        ਜੇ (Is_ਜੀਵਤ_ਢੇਰ_ਇੰਦਰਾਜ(o, i)) {
_           (ਜੋੜੋ_ਸੁਰੱਖਿਅਤ_ਬਿੰਦੂ_ਮੁੱਲ(o, ਬਿੰਦੂ, i));
        }
    }

    ਜੇ (i_loop) {
        u16 ਅੰਕ_Params = Get_Func_Type_ਅੰਕ_Params(i_loop->type);

        ਲਈ (u16 i = 0; i < ਅੰਕ_Params; ++i) {
_           (ਜੋੜੋ_ਸੁਰੱਖਿਅਤ_ਬਿੰਦੂ_ਮੁੱਲ(o, ਬਿੰਦੂ, (u16)(i_loop->exit_ਢੇਰ_ਸੂਚਕਾਂਕ + i)));
        }
    }

    map->ਅੰਕ_ਸੁਰੱਖਿਅਤ_Points++;

} _catch: ਵਾਪਸ ਨਤੀਜਾ;
}

// A back edge: op_ContinueLoop, or op_ContinueLoopIf when i_isConditional, and
// the loop it goes back to. The conditional one suspends before it has taken
// the branch, so it has to be resumed with the condition that took it.
ਸਥਿਰ
M3_ਨਤੀਜਾ ਰਿਕਾਰਡ_ਪਿੱਠ_ਕਿਨਾਰਾ (IM3Compilation o, IM3CompilationScope i_loop, bool i_isConditional)
{
    M3_ਨਤੀਜਾ ਨਤੀਜਾ = m3Err_none;

    // the loop is named by its record in the map, which has to fit the field
    _throwif("a function has too many loops for its snapshot map",
             o->snapshotMap and o->ਪੰਨਾ and (i_loop->snapshot_ਖੰਡ == 0 or i_loop->snapshot_ਖੰਡ > UINT16_MAX + 1u));

_   (ਰਿਕਾਰਡ_ਸੁਰੱਖਿਅਤ_ਬਿੰਦੂ(o, safepoint_op, 2, 0, (u16)(i_loop->snapshot_ਖੰਡ - 1), i_loop));

    ਜੇ (i_isConditional and o->snapshotMap and o->ਪੰਨਾ) {
        o->snapshotMap->ਸੁਰੱਖਿਅਤ_Points[o->snapshotMap->ਅੰਕ_ਸੁਰੱਖਿਅਤ_Points - 1].flags |= d_m3_ਸੁਰੱਖਿਅਤ_ਬਿੰਦੂ_ਲਿਆ_Branch;
    }

    _catch: ਵਾਪਸ ਨਤੀਜਾ;
}

// A call, waiting on its callee with i_numResults not yet written; the
// callee's frame starts i_calleeSlot slots above this one. A return_call that
// is compiled as a call is one too, since its frame stands while the callee
// runs, but it is marked: in Wasm the callee has replaced it, so a snapshot
// walks past it rather than writing it.
ਸਥਿਰ
M3_ਨਤੀਜਾ ਰਿਕਾਰਡ_Call (IM3Compilation o, u16 i_ਅੰਕ_Results, u16 i_callee_ਥਾਂ, bool i_isReturnCall)
{
    M3_ਨਤੀਜਾ ਨਤੀਜਾ = ਰਿਕਾਰਡ_ਸੁਰੱਖਿਅਤ_ਬਿੰਦੂ(o, safepoint_call, 0, i_ਅੰਕ_Results, i_callee_ਥਾਂ, ਨਿਰਾ);

    ਜੇ (not ਨਤੀਜਾ and i_isReturnCall and o->snapshotMap and o->ਪੰਨਾ) {
        o->snapshotMap->ਸੁਰੱਖਿਅਤ_Points[o->snapshotMap->ਅੰਕ_ਸੁਰੱਖਿਅਤ_Points - 1].flags |= d_m3_ਸੁਰੱਖਿਅਤ_ਬਿੰਦੂ_ਪੂਛ_Call;
    }

    ਵਾਪਸ ਨਤੀਜਾ;
}

// Where the function's arguments and locals are kept. Unlike the operand
// stack, that never changes, so it is recorded once rather than at every
// safepoint.
ਸਥਿਰ
M3_ਨਤੀਜਾ ਰਿਕਾਰਡ_Locals (IM3Compilation o)
{
    M3SnapshotMap* map = o->snapshotMap;

    ਜੇ (map and o->ਢੇਰ_ਸੂਚਕਾਂਕ) {
        map->locals = m3_AllocArray(M3_ਥਾਂ_ਮੁੱਲ, o->ਢੇਰ_ਸੂਚਕਾਂਕ);

        ਜੇ (not map->locals) {
            ਵਾਪਸ m3Err_mallocFailed;
        }

        ਲਈ (u16 i = 0; i < o->ਢੇਰ_ਸੂਚਕਾਂਕ; ++i) {
            map->locals[i].ਥਾਂ = o->wasm_ਢੇਰ[i];
            map->locals[i].type = o->type_ਢੇਰ[i];
        }

        map->ਅੰਕ_Locals = o->ਢੇਰ_ਸੂਚਕਾਂਕ;
        ਲਈ (u32 i = Get_Function_ਅੰਕ_ਦਲੀਲਾਂ(o->function); i < map->ਅੰਕ_Locals; ++i) {
            ਜੇ (IsRefType(map->locals[i].type) and not IsNullableRef(map->locals[i].type)) {
                o->local_Init_ਡੂੰਘਾਈ = m3_AllocArray(u32, map->ਅੰਕ_Locals);
                ਜੇ (not o->local_Init_ਡੂੰਘਾਈ) {
                    ਵਾਪਸ m3Err_mallocFailed;
                }
                ਲਈ (u32 j = i; j < map->ਅੰਕ_Locals; ++j) {
                    ਜੇ (IsRefType(map->locals[j].type) and not IsNullableRef(map->locals[j].type)) {
                        o->local_Init_ਡੂੰਘਾਈ[j] = UINT32_MAX;
                    }
                }
                ਰੋਕੋ;
            }
        }
    }

    ਵਾਪਸ m3Err_none;
}

// A loop or a try_table, whose native frame a snapshot names by the
// instruction's offset. i_pc is what that frame records. The scope being
// compiled keeps the record, so that RecordBlockEnd can close it.
ਸਥਿਰ
M3_ਨਤੀਜਾ ਰਿਕਾਰਡ_ਖੰਡ_ਸ਼ੁਰੂ (IM3Compilation o, m3opcode_t i_opcode, pc_t i_pc, u32 i_ਅੰਕ_Clauses)
{
    M3SnapshotMap* map = o->snapshotMap;

    ਜੇ (map and o->ਪੰਨਾ) {
        d_m3GrowSnapshotArray(M3_ਖੰਡ_ਸ਼ੁਰੂ, map->ਖੰਡਾਂ, map->ਅੰਕ_ਖੰਡਾਂ, o->ਖੰਡਾਂ_ਸਮਰੱਥਾ);

        M3_ਖੰਡ_ਸ਼ੁਰੂ* ਖੰਡ = &map->ਖੰਡਾਂ[map->ਅੰਕ_ਖੰਡਾਂ++];
        ਖੰਡ->pc           = i_pc;
        ਖੰਡ->wasm_ਵਿੱਥ   = Get_Wasm_ਵਿੱਥ(o);
        ਖੰਡ->wasm_ਸਿਰਾ      = UINT32_MAX;
        ਖੰਡ->ਅੰਕ_Clauses   = (u16)i_ਅੰਕ_Clauses;
        ਖੰਡ->opcode       = (u8)i_opcode;

        o->ਖੰਡ.snapshot_ਖੰਡ = map->ਅੰਕ_ਖੰਡਾਂ;
    }

    ਵਾਪਸ m3Err_none;
}

// Closes the scope's region, while the instruction just read is its end
ਸਥਿਰ
ਖਾਲੀ ਰਿਕਾਰਡ_ਖੰਡ_ਸਿਰਾ (IM3Compilation o)
{
    ਜੇ (o->snapshotMap and o->ਖੰਡ.snapshot_ਖੰਡ) {
        o->snapshotMap->ਖੰਡਾਂ[o->ਖੰਡ.snapshot_ਖੰਡ - 1].wasm_ਸਿਰਾ = Get_Wasm_ਵਿੱਥ(o);
    }
}

#  undef d_m3GrowSnapshotArray

#ਨਹੀਂਤਾਂ

ਸਥਿਰ ਸਤਰਅੰਦਰ
M3_ਨਤੀਜਾ ਰਿਕਾਰਡ_ਸੁਰੱਖਿਅਤ_ਬਿੰਦੂ (IM3Compilation o, M3_ਸੁਰੱਖਿਅਤ_ਬਿੰਦੂ_ਕਿਸਮ i_ਕਿਸਮ, u32 i_ਸ਼ਬਦ_ਪਿੱਠ, u16 i_ਅੰਕ_Results,
                          u16 i_aux, IM3CompilationScope i_loop)
{
    (ਖਾਲੀ)o;
    (ਖਾਲੀ)i_ਕਿਸਮ;
    (ਖਾਲੀ)i_ਸ਼ਬਦ_ਪਿੱਠ;
    (ਖਾਲੀ)i_ਅੰਕ_Results;
    (ਖਾਲੀ)i_aux;
    (ਖਾਲੀ)i_loop;
    ਵਾਪਸ m3Err_none;
}

ਸਥਿਰ ਸਤਰਅੰਦਰ
M3_ਨਤੀਜਾ ਰਿਕਾਰਡ_ਪਿੱਠ_ਕਿਨਾਰਾ (IM3Compilation o, IM3CompilationScope i_loop, bool i_isConditional)
{
    (ਖਾਲੀ)o;
    (ਖਾਲੀ)i_loop;
    (ਖਾਲੀ)i_isConditional;
    ਵਾਪਸ m3Err_none;
}

ਸਥਿਰ ਸਤਰਅੰਦਰ
M3_ਨਤੀਜਾ ਰਿਕਾਰਡ_Call (IM3Compilation o, u16 i_ਅੰਕ_Results, u16 i_callee_ਥਾਂ, bool i_isReturnCall)
{
    (ਖਾਲੀ)o;
    (ਖਾਲੀ)i_ਅੰਕ_Results;
    (ਖਾਲੀ)i_callee_ਥਾਂ;
    (ਖਾਲੀ)i_isReturnCall;
    ਵਾਪਸ m3Err_none;
}

#ਜੇ_ਅੰਤ // d_m3HasSnapshots

ਸਥਿਰ M3_NOINLINE
M3_ਨਤੀਜਾ Ensure_ਕੋਡ_ਪੰਨਾ_ਅੰਕ_ਸਤਰਾਂ (IM3Compilation o, u32 i_ਅੰਕ_ਸਤਰਾਂ)
{
    M3_ਨਤੀਜਾ ਨਤੀਜਾ = m3Err_none;

    i_ਅੰਕ_ਸਤਰਾਂ += 2; // room for Bridge

    ਜੇ (M3_UNLIKELY(ਅੰਕ_Free_ਸਤਰਾਂ(o->ਪੰਨਾ) < i_ਅੰਕ_ਸਤਰਾਂ)) {
        IM3_ਕੋਡ_ਪੰਨਾ ਪੰਨਾ = Acquire_ਕੋਡ_ਪੰਨਾ_With_ਸਮਰੱਥਾ(o->runtime, i_ਅੰਕ_ਸਤਰਾਂ);

        ਜੇ (ਪੰਨਾ) {
            m3log(emit, "bridging new code page from: %d %p (free slots: %d) to: %d", o->ਪੰਨਾ->info.sequence, GetPC(o), ਅੰਕ_Free_ਸਤਰਾਂ(o->ਪੰਨਾ), ਪੰਨਾ->info.sequence);
            d_m3Assert(ਅੰਕ_Free_ਸਤਰਾਂ(o->ਪੰਨਾ) >= 2);

            Emit_ਸ਼ਬਦ(o->ਪੰਨਾ, op_Branch);
            Emit_ਸ਼ਬਦ(o->ਪੰਨਾ, Get_ਪੰਨਾ_PC(ਪੰਨਾ));

            Release_ਕੋਡ_ਪੰਨਾ(o->runtime, o->ਪੰਨਾ);

            o->ਪੰਨਾ = ਪੰਨਾ;
        } ਨਹੀਂਤਾਂ {
            ਨਤੀਜਾ = m3_Err_malloc_Failed_ਕੋਡ_ਪੰਨਾ;
        }
    }

    ਵਾਪਸ ਨਤੀਜਾ;
}

// invalidate the pending local.set fold candidate; called wherever code is emitted or a
// branch target captures the current pc (a fold appends an immediate, moving that position)
ਸਥਿਰ ਸਤਰਅੰਦਰ
ਖਾਲੀ InvalidateFold (IM3Compilation o)
{
#ਜੇ d_m3_Fold_ਸੈੱਟ_Local
    o->foldPatchPC = ਨਿਰਾ;
#ਨਹੀਂਤਾਂ
    (ਖਾਲੀ)o;
#ਜੇ_ਅੰਤ
}

ਸਥਿਰ M3_NOINLINE
M3_ਨਤੀਜਾ EmitOp (IM3Compilation o, IM3Operation i_operation)
{
    M3_ਨਤੀਜਾ ਨਤੀਜਾ = m3Err_none;                                 d_m3Assert (i_operation or Is_ਢੇਰ_Polymorphic (o));

    InvalidateFold(o);

    // it's OK for page to be null; when compile-walking the bytecode without emitting
    ਜੇ (o->ਪੰਨਾ) {
#ਜੇ d_m3EnableOpTracing
        ਜੇ (i_operation != op_Dump_ਢੇਰ) {
            o->ਅੰਕ_Emits++;
        }
#ਜੇ_ਅੰਤ

        // have execution jump to a new page if slots are critically low
        ਨਤੀਜਾ = Ensure_ਕੋਡ_ਪੰਨਾ_ਅੰਕ_ਸਤਰਾਂ(o, d_m3_ਕੋਡ_ਪੰਨਾ_Free_ਸਤਰਾਂ_ਹੱਦ);

        ਜੇ (M3_LIKELY(not ਨਤੀਜਾ)) {
            ਜੇ (d_m3LogEmit) {
                log_emit(o, i_operation);
            }
#ਜੇ d_m3_ਰਿਕਾਰਡ_Backtraces
            Emit_Mapping_ਇੰਦਰਾਜ(o->ਪੰਨਾ, (u32)(o->ਆਖਰੀ_Opcode_ਸ਼ੁਰੂ - o->module->wasm_ਸ਼ੁਰੂ));
#ਜੇ_ਅੰਤ // d_m3RecordBacktraces
            Emit_ਸ਼ਬਦ(o->ਪੰਨਾ, i_operation);
        }
    }

    ਵਾਪਸ ਨਤੀਜਾ;
}

// Push an immediate constant into the M3 codestream
ਸਥਿਰ M3_NOINLINE
ਖਾਲੀ EmitConstant32 (IM3Compilation o, ਅਚਲ u32 i_ਤੁਰੰਤ)
{
    ਜੇ (o->ਪੰਨਾ) {                                                          m3log (emit, "const32: %ud", i_ਤੁਰੰਤ);
        EmitWord32(o->ਪੰਨਾ, i_ਤੁਰੰਤ);
    }
}

#ਜੇ d_m3HasMemory64
// Takes two lines of the code page where a pointer is 32 bits, so whatever
// reads it back has to step _pc by the same amount - see op_Const64.
ਸਥਿਰ M3_NOINLINE
ਖਾਲੀ EmitConstant64 (IM3Compilation o, ਅਚਲ u64 i_ਤੁਰੰਤ)
{
    ਜੇ (o->ਪੰਨਾ) {
        EmitWord64(o->ਪੰਨਾ, i_ਤੁਰੰਤ);
    }
}
#ਜੇ_ਅੰਤ

ਸਥਿਰ M3_NOINLINE
ਖਾਲੀ Emit_ਥਾਂ_ਵਿੱਥ (IM3Compilation o, ਅਚਲ i32 i_ਵਿੱਥ)
{
    ਜੇ (o->ਪੰਨਾ) {                                                          m3log (emit, "slot: [%d]", i_ਵਿੱਥ);
        EmitWord32(o->ਪੰਨਾ, i_ਵਿੱਥ);
    }
}

ਸਥਿਰ M3_NOINLINE
pc_t Emit_ਸੂਚਕ (IM3Compilation o, ਅਚਲ ਖਾਲੀ* ਅਚਲ i_ਸੂਚਕ)
{
    pc_t ptr = Get_ਪੰਨਾ_PC(o->ਪੰਨਾ);

    ਜੇ (o->ਪੰਨਾ) {                                                          m3log (emit, "ptr: %p", i_ਸੂਚਕ);
        Emit_ਸ਼ਬਦ(o->ਪੰਨਾ, i_ਸੂਚਕ);
    }

    ਵਾਪਸ ptr;
}

ਸਥਿਰ M3_NOINLINE
ਖਾਲੀ* Reserve_ਸੂਚਕ (IM3Compilation o)
{
    pc_t ptr = Get_ਪੰਨਾ_PC(o->ਪੰਨਾ);
    Emit_ਸੂਚਕ(o, ਨਿਰਾ);
    ਵਾਪਸ (ਖਾਲੀ*)ptr;
}


//-------------------------------------------------------------------------------------------------------------------------

#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ d_indent "     | %s"

// just want less letters and numbers to stare at down the way in the compiler table
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ i_32    c_m3Type_i32
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ i_64    c_m3Type_i64
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ f_32    c_m3Type_f32
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ f_64    c_m3Type_f64
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ none    c_m3Type_none
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ any     (u8)-1

#ਜੇ d_m3HasFloat
#  define FPOP(x) x
#ਨਹੀਂਤਾਂ
#  define FPOP(x) ਨਿਰਾ
#ਜੇ_ਅੰਤ

// These are indexed by M3ValueType, so every type up to c_m3Type_externref needs
// an entry. A reference is one pointer-sized word, so it moves with the integer
// operation of that width; v128 has no operations at all.
#ਜੇ M3_SIZEOF_PTR == 8
#  define REFOP(NAME)  op_##NAME##_i64
#ਨਹੀਂਤਾਂ
#  define REFOP(NAME)  op_##NAME##_i32
#ਜੇ_ਅੰਤ

// clang-format off

ਸਥਿਰ ਅਚਲ IM3Operation c_preserve_ਸੈੱਟ_ਥਾਂ[] = { ਨਿਰਾ, op_Preserve_ਸੈੱਟ_ਥਾਂ_i32,       op_Preserve_ਸੈੱਟ_ਥਾਂ_i64,
                                                    FPOP(op_Preserve_ਸੈੱਟ_ਥਾਂ_f32), FPOP(op_Preserve_ਸੈੱਟ_ਥਾਂ_f64),
                                                    ਨਿਰਾ, REFOP(Preserve_ਸੈੱਟ_ਥਾਂ),  REFOP(Preserve_ਸੈੱਟ_ਥਾਂ),
                                                    REFOP(Preserve_ਸੈੱਟ_ਥਾਂ),       REFOP(Preserve_ਸੈੱਟ_ਥਾਂ) };
ਸਥਿਰ ਅਚਲ IM3Operation c_ਸੈੱਟ_ਸੈੱਟ_Ops[] =       { ਨਿਰਾ, op_ਸੈੱਟ_ਥਾਂ_i32,               op_ਸੈੱਟ_ਥਾਂ_i64,
                                                    FPOP(op_ਸੈੱਟ_ਥਾਂ_f32),         FPOP(op_ਸੈੱਟ_ਥਾਂ_f64),
                                                    ਨਿਰਾ, REFOP(ਸੈੱਟ_ਥਾਂ),         REFOP(ਸੈੱਟ_ਥਾਂ),
                                                    REFOP(ਸੈੱਟ_ਥਾਂ),               REFOP(ਸੈੱਟ_ਥਾਂ) };
ਸਥਿਰ ਅਚਲ IM3Operation c_ਸੈੱਟ_Global_Ops[] =    { ਨਿਰਾ, op_ਸੈੱਟ_Global_i32,             op_ਸੈੱਟ_Global_i64,
                                                    FPOP(op_ਸੈੱਟ_Global_f32),       FPOP(op_ਸੈੱਟ_Global_f64),
                                                    ਨਿਰਾ, REFOP(ਸੈੱਟ_Global),       REFOP(ਸੈੱਟ_Global),
                                                    REFOP(ਸੈੱਟ_Global),             REFOP(ਸੈੱਟ_Global) };
ਸਥਿਰ ਅਚਲ IM3Operation c_ਸੈੱਟ_ਰਜਿਸਟਰ_Ops[] =  { ਨਿਰਾ, op_ਸੈੱਟ_ਰਜਿਸਟਰ_i32,           op_ਸੈੱਟ_ਰਜਿਸਟਰ_i64,
                                                    FPOP(op_ਸੈੱਟ_ਰਜਿਸਟਰ_f32),     FPOP(op_ਸੈੱਟ_ਰਜਿਸਟਰ_f64),
                                                    ਨਿਰਾ, REFOP(ਸੈੱਟ_ਰਜਿਸਟਰ),     REFOP(ਸੈੱਟ_ਰਜਿਸਟਰ),
                                                    REFOP(ਸੈੱਟ_ਰਜਿਸਟਰ),           REFOP(ਸੈੱਟ_ਰਜਿਸਟਰ) };

// A table shorter than the enum reads out of bounds, so tie the two together at
// build time: adding a value type must not silently outgrow these.
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ d_m3_Check_Type_ਸਾਰਣੀ(ਸਾਰਣੀ) \
    M3_STATIC_ASSERT (ਆਕਾਰ (ਸਾਰਣੀ) / ਆਕਾਰ (*(ਸਾਰਣੀ)) == c_m3_Type_ਗਿਣਤੀ, ਸਾਰਣੀ##_needs_one_entry_per_type)

d_m3_Check_Type_ਸਾਰਣੀ(c_preserve_ਸੈੱਟ_ਥਾਂ);
d_m3_Check_Type_ਸਾਰਣੀ(c_ਸੈੱਟ_ਸੈੱਟ_Ops);
d_m3_Check_Type_ਸਾਰਣੀ(c_ਸੈੱਟ_Global_Ops);
d_m3_Check_Type_ਸਾਰਣੀ(c_ਸੈੱਟ_ਰਜਿਸਟਰ_Ops);

#ਜੇ d_m3_Fold_ਸੈੱਟ_Local

// destination-folded variants of the hot binops, indexed by the operand-form index
// recorded at emission: [0]=_rs [1]=_sr [2]=_ss [3]=(fp _rr, never folded)
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ d_foldOpList(TYPE, NAME)            { op_##TYPE##_##NAME##_rs_f, op_##TYPE##_##NAME##_sr_f, op_##TYPE##_##NAME##_ss_f, ਨਿਰਾ }
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ d_foldCommutativeOpList(TYPE, NAME) { op_##TYPE##_##NAME##_rs_f, ਨਿਰਾ,                      op_##TYPE##_##NAME##_ss_f, ਨਿਰਾ }

ਸਥਿਰ ਅਚਲ IM3Operation c_fold_i32_ਜੋੜੋ[]         = d_foldCommutativeOpList (i32, ਜੋੜੋ);
ਸਥਿਰ ਅਚਲ IM3Operation c_fold_i32_ਘਟਾਓ[]    = d_foldOpList (i32, ਘਟਾਓ);
ਸਥਿਰ ਅਚਲ IM3Operation c_fold_i32_Multiply[]    = d_foldCommutativeOpList (i32, Multiply);
ਸਥਿਰ ਅਚਲ IM3Operation c_fold_u32_And[]         = d_foldCommutativeOpList (u32, And);
ਸਥਿਰ ਅਚਲ IM3Operation c_fold_u32_Or[]          = d_foldCommutativeOpList (u32, Or);
ਸਥਿਰ ਅਚਲ IM3Operation c_fold_u32_Xor[]         = d_foldCommutativeOpList (u32, Xor);
ਸਥਿਰ ਅਚਲ IM3Operation c_fold_u32_ਖਿਸਕਾਓ_ਖੱਬਾ[]   = d_foldOpList (u32, ਖਿਸਕਾਓ_ਖੱਬਾ);
ਸਥਿਰ ਅਚਲ IM3Operation c_fold_i32_ਖਿਸਕਾਓ_ਸੱਜਾ[]  = d_foldOpList (i32, ਖਿਸਕਾਓ_ਸੱਜਾ);
ਸਥਿਰ ਅਚਲ IM3Operation c_fold_u32_ਖਿਸਕਾਓ_ਸੱਜਾ[]  = d_foldOpList (u32, ਖਿਸਕਾਓ_ਸੱਜਾ);

ਸਥਿਰ ਅਚਲ IM3Operation c_fold_i64_ਜੋੜੋ[]         = d_foldCommutativeOpList (i64, ਜੋੜੋ);
ਸਥਿਰ ਅਚਲ IM3Operation c_fold_i64_ਘਟਾਓ[]    = d_foldOpList (i64, ਘਟਾਓ);
ਸਥਿਰ ਅਚਲ IM3Operation c_fold_i64_Multiply[]    = d_foldCommutativeOpList (i64, Multiply);
ਸਥਿਰ ਅਚਲ IM3Operation c_fold_u64_And[]         = d_foldCommutativeOpList (u64, And);
ਸਥਿਰ ਅਚਲ IM3Operation c_fold_u64_Or[]          = d_foldCommutativeOpList (u64, Or);
ਸਥਿਰ ਅਚਲ IM3Operation c_fold_u64_Xor[]         = d_foldCommutativeOpList (u64, Xor);
ਸਥਿਰ ਅਚਲ IM3Operation c_fold_u64_ਖਿਸਕਾਓ_ਖੱਬਾ[]   = d_foldOpList (u64, ਖਿਸਕਾਓ_ਖੱਬਾ);
ਸਥਿਰ ਅਚਲ IM3Operation c_fold_i64_ਖਿਸਕਾਓ_ਸੱਜਾ[]  = d_foldOpList (i64, ਖਿਸਕਾਓ_ਸੱਜਾ);
ਸਥਿਰ ਅਚਲ IM3Operation c_fold_u64_ਖਿਸਕਾਓ_ਸੱਜਾ[]  = d_foldOpList (u64, ਖਿਸਕਾਓ_ਸੱਜਾ);

#  ਜੇ d_m3HasFloat
ਸਥਿਰ ਅਚਲ IM3Operation c_fold_f32_ਜੋੜੋ[]         = d_foldCommutativeOpList (f32, ਜੋੜੋ);
ਸਥਿਰ ਅਚਲ IM3Operation c_fold_f32_ਘਟਾਓ[]    = d_foldOpList (f32, ਘਟਾਓ);
ਸਥਿਰ ਅਚਲ IM3Operation c_fold_f32_Multiply[]    = d_foldCommutativeOpList (f32, Multiply);
ਸਥਿਰ ਅਚਲ IM3Operation c_fold_f32_Divide[]      = d_foldOpList (f32, Divide);
ਸਥਿਰ ਅਚਲ IM3Operation c_fold_f64_ਜੋੜੋ[]         = d_foldCommutativeOpList (f64, ਜੋੜੋ);
ਸਥਿਰ ਅਚਲ IM3Operation c_fold_f64_ਘਟਾਓ[]    = d_foldOpList (f64, ਘਟਾਓ);
ਸਥਿਰ ਅਚਲ IM3Operation c_fold_f64_Multiply[]    = d_foldCommutativeOpList (f64, Multiply);
ਸਥਿਰ ਅਚਲ IM3Operation c_fold_f64_Divide[]      = d_foldOpList (f64, Divide);
#  endif

// loads are unary: [0]= address in _r0, [1]= address in a slot
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ d_fold_ਲੱਦੋ_List(DEST, SRC)       { op_##DEST##_Load_##SRC##_r_f, op_##DEST##_Load_##SRC##_s_f, ਨਿਰਾ, ਨਿਰਾ }

ਸਥਿਰ ਅਚਲ IM3Operation c_fold_i32_ਲੱਦੋ_i32[] = d_fold_ਲੱਦੋ_List(i32, i32);
ਸਥਿਰ ਅਚਲ IM3Operation c_fold_i32_ਲੱਦੋ_i8[] = d_fold_ਲੱਦੋ_List(i32, i8);
ਸਥਿਰ ਅਚਲ IM3Operation c_fold_i32_ਲੱਦੋ_u8[] = d_fold_ਲੱਦੋ_List(i32, u8);
ਸਥਿਰ ਅਚਲ IM3Operation c_fold_i32_ਲੱਦੋ_i16[] = d_fold_ਲੱਦੋ_List(i32, i16);
ਸਥਿਰ ਅਚਲ IM3Operation c_fold_i32_ਲੱਦੋ_u16[] = d_fold_ਲੱਦੋ_List(i32, u16);

ਸਥਿਰ ਅਚਲ IM3Operation c_fold_i64_ਲੱਦੋ_i64[] = d_fold_ਲੱਦੋ_List(i64, i64);
ਸਥਿਰ ਅਚਲ IM3Operation c_fold_i64_ਲੱਦੋ_i8[] = d_fold_ਲੱਦੋ_List(i64, i8);
ਸਥਿਰ ਅਚਲ IM3Operation c_fold_i64_ਲੱਦੋ_u8[] = d_fold_ਲੱਦੋ_List(i64, u8);
ਸਥਿਰ ਅਚਲ IM3Operation c_fold_i64_ਲੱਦੋ_i16[] = d_fold_ਲੱਦੋ_List(i64, i16);
ਸਥਿਰ ਅਚਲ IM3Operation c_fold_i64_ਲੱਦੋ_u16[] = d_fold_ਲੱਦੋ_List(i64, u16);
ਸਥਿਰ ਅਚਲ IM3Operation c_fold_i64_ਲੱਦੋ_i32[] = d_fold_ਲੱਦੋ_List(i64, i32);
ਸਥਿਰ ਅਚਲ IM3Operation c_fold_i64_ਲੱਦੋ_u32[] = d_fold_ਲੱਦੋ_List(i64, u32);
#  ਜੇ d_m3HasFloat
ਸਥਿਰ ਅਚਲ IM3Operation c_fold_f32_ਲੱਦੋ_f32[] = d_fold_ਲੱਦੋ_List(f32, f32);
ਸਥਿਰ ਅਚਲ IM3Operation c_fold_f64_ਲੱਦੋ_f64[] = d_fold_ਲੱਦੋ_List(f64, f64);
#  endif

ਸਥਿਰ
IM3Operation GetFoldOp (m3opcode_t i_opcode, u8 i_form)
{
    ਜੇ (i_form >= 4) {
        ਵਾਪਸ ਨਿਰਾ;
    }

    ਚੋਣ (i_opcode) {
    ਹਾਲਤ 0x6a:  ਵਾਪਸ c_fold_i32_ਜੋੜੋ        [i_form];      // i32.add
    ਹਾਲਤ 0x6b:  ਵਾਪਸ c_fold_i32_ਘਟਾਓ   [i_form];      // i32.sub
    ਹਾਲਤ 0x6c:  ਵਾਪਸ c_fold_i32_Multiply   [i_form];      // i32.mul
    ਹਾਲਤ 0x71:  ਵਾਪਸ c_fold_u32_And        [i_form];      // i32.and
    ਹਾਲਤ 0x72:  ਵਾਪਸ c_fold_u32_Or         [i_form];      // i32.or
    ਹਾਲਤ 0x73:  ਵਾਪਸ c_fold_u32_Xor        [i_form];      // i32.xor
    ਹਾਲਤ 0x74:  ਵਾਪਸ c_fold_u32_ਖਿਸਕਾਓ_ਖੱਬਾ  [i_form];      // i32.shl
    ਹਾਲਤ 0x75:  ਵਾਪਸ c_fold_i32_ਖਿਸਕਾਓ_ਸੱਜਾ [i_form];      // i32.shr_s
    ਹਾਲਤ 0x76:  ਵਾਪਸ c_fold_u32_ਖਿਸਕਾਓ_ਸੱਜਾ [i_form];      // i32.shr_u

    ਹਾਲਤ 0x7c:  ਵਾਪਸ c_fold_i64_ਜੋੜੋ        [i_form];      // i64.add
    ਹਾਲਤ 0x7d:  ਵਾਪਸ c_fold_i64_ਘਟਾਓ   [i_form];      // i64.sub
    ਹਾਲਤ 0x7e:  ਵਾਪਸ c_fold_i64_Multiply   [i_form];      // i64.mul
    ਹਾਲਤ 0x83:  ਵਾਪਸ c_fold_u64_And        [i_form];      // i64.and
    ਹਾਲਤ 0x84:  ਵਾਪਸ c_fold_u64_Or         [i_form];      // i64.or
    ਹਾਲਤ 0x85:  ਵਾਪਸ c_fold_u64_Xor        [i_form];      // i64.xor
    ਹਾਲਤ 0x86:  ਵਾਪਸ c_fold_u64_ਖਿਸਕਾਓ_ਖੱਬਾ  [i_form];      // i64.shl
    ਹਾਲਤ 0x87:  ਵਾਪਸ c_fold_i64_ਖਿਸਕਾਓ_ਸੱਜਾ [i_form];      // i64.shr_s
    ਹਾਲਤ 0x88:  ਵਾਪਸ c_fold_u64_ਖਿਸਕਾਓ_ਸੱਜਾ [i_form];      // i64.shr_u

#  ਜੇ d_m3HasFloat
    ਹਾਲਤ 0x92:  ਵਾਪਸ c_fold_f32_ਜੋੜੋ        [i_form];      // f32.add
    ਹਾਲਤ 0x93:  ਵਾਪਸ c_fold_f32_ਘਟਾਓ   [i_form];      // f32.sub
    ਹਾਲਤ 0x94:  ਵਾਪਸ c_fold_f32_Multiply   [i_form];      // f32.mul
    ਹਾਲਤ 0x95:  ਵਾਪਸ c_fold_f32_Divide     [i_form];      // f32.div
    ਹਾਲਤ 0xa0:  ਵਾਪਸ c_fold_f64_ਜੋੜੋ        [i_form];      // f64.add
    ਹਾਲਤ 0xa1:  ਵਾਪਸ c_fold_f64_ਘਟਾਓ   [i_form];      // f64.sub
    ਹਾਲਤ 0xa2:  ਵਾਪਸ c_fold_f64_Multiply   [i_form];      // f64.mul
    ਹਾਲਤ 0xa3:  ਵਾਪਸ c_fold_f64_Divide     [i_form];      // f64.div
#  endif

    ਹਾਲਤ 0x28:  ਵਾਪਸ c_fold_i32_ਲੱਦੋ_i32   [i_form];      // i32.load
    ਹਾਲਤ 0x2c:  ਵਾਪਸ c_fold_i32_ਲੱਦੋ_i8    [i_form];      // i32.load8_s
    ਹਾਲਤ 0x2d:  ਵਾਪਸ c_fold_i32_ਲੱਦੋ_u8    [i_form];      // i32.load8_u
    ਹਾਲਤ 0x2e:  ਵਾਪਸ c_fold_i32_ਲੱਦੋ_i16   [i_form];      // i32.load16_s
    ਹਾਲਤ 0x2f:  ਵਾਪਸ c_fold_i32_ਲੱਦੋ_u16   [i_form];      // i32.load16_u

    ਹਾਲਤ 0x29:  ਵਾਪਸ c_fold_i64_ਲੱਦੋ_i64   [i_form];      // i64.load
    ਹਾਲਤ 0x30:  ਵਾਪਸ c_fold_i64_ਲੱਦੋ_i8    [i_form];      // i64.load8_s
    ਹਾਲਤ 0x31:  ਵਾਪਸ c_fold_i64_ਲੱਦੋ_u8    [i_form];      // i64.load8_u
    ਹਾਲਤ 0x32:  ਵਾਪਸ c_fold_i64_ਲੱਦੋ_i16   [i_form];      // i64.load16_s
    ਹਾਲਤ 0x33:  ਵਾਪਸ c_fold_i64_ਲੱਦੋ_u16   [i_form];      // i64.load16_u
    ਹਾਲਤ 0x34:  ਵਾਪਸ c_fold_i64_ਲੱਦੋ_i32   [i_form];      // i64.load32_s
    ਹਾਲਤ 0x35:  ਵਾਪਸ c_fold_i64_ਲੱਦੋ_u32   [i_form];      // i64.load32_u

#  ਜੇ d_m3HasFloat
    ਹਾਲਤ 0x2a:  ਵਾਪਸ c_fold_f32_ਲੱਦੋ_f32   [i_form];      // f32.load
    ਹਾਲਤ 0x2b:  ਵਾਪਸ c_fold_f64_ਲੱਦੋ_f64   [i_form];      // f64.load
#  endif

    ਮੂਲ: ਵਾਪਸ ਨਿਰਾ;
    }
}

#ਜੇ_ਅੰਤ // d_m3FoldSetLocal

// clang-format on

#ਜੇ d_m3FuseBranch

// fused compare+branch/if variants, indexed like the fold lists: [0]=_rs [1]=_sr [2]=_ss [3]=unused.
// the [1] entries are NULL for commutative compares: the compiler never records the _sr form for them
#  define d_fuseCmpList(TYPE, NAME, ਕਿਸਮ_2)             { op_##TYPE##_##KIND##_##NAME##_rs, op_##TYPE##_##KIND##_##NAME##_sr, op_##TYPE##_##KIND##_##NAME##_ss, ਨਿਰਾ }
#  define d_fuseCommutativeCmpList(TYPE, NAME, ਕਿਸਮ_2)  { op_##TYPE##_##KIND##_##NAME##_rs, ਨਿਰਾ,                             op_##TYPE##_##KIND##_##NAME##_ss, ਨਿਰਾ }

ਕਿਸਮਪਰਿਭਾਸ਼ਾ ਬਣਤਰ M3FusedCmpOps {
    IM3Operation branchIf[4];
    IM3Operation ifOp[4];
} M3FusedCmpOps;

#  define d_fuseCmp(TYPE, NAME)               { d_fuseCmpList (TYPE, NAME, BranchIf),            d_fuseCmpList (TYPE, NAME, If) }
#  define d_fuseCommutativeCmp(TYPE, NAME)    { d_fuseCommutativeCmpList (TYPE, NAME, BranchIf), d_fuseCommutativeCmpList (TYPE, NAME, If) }

// clang-format off
ਸਥਿਰ ਅਚਲ M3FusedCmpOps c_fuse_i32_Equal              = d_fuseCommutativeCmp (i32, Equal);
ਸਥਿਰ ਅਚਲ M3FusedCmpOps c_fuse_i32_NotEqual           = d_fuseCommutativeCmp (i32, NotEqual);
ਸਥਿਰ ਅਚਲ M3FusedCmpOps c_fuse_i32_LessThan           = d_fuseCmp (i32, LessThan);
ਸਥਿਰ ਅਚਲ M3FusedCmpOps c_fuse_u32_LessThan           = d_fuseCmp (u32, LessThan);
ਸਥਿਰ ਅਚਲ M3FusedCmpOps c_fuse_i32_GreaterThan        = d_fuseCmp (i32, GreaterThan);
ਸਥਿਰ ਅਚਲ M3FusedCmpOps c_fuse_u32_GreaterThan        = d_fuseCmp (u32, GreaterThan);
ਸਥਿਰ ਅਚਲ M3FusedCmpOps c_fuse_i32_LessThanOrEqual    = d_fuseCmp (i32, LessThanOrEqual);
ਸਥਿਰ ਅਚਲ M3FusedCmpOps c_fuse_u32_LessThanOrEqual    = d_fuseCmp (u32, LessThanOrEqual);
ਸਥਿਰ ਅਚਲ M3FusedCmpOps c_fuse_i32_GreaterThanOrEqual = d_fuseCmp (i32, GreaterThanOrEqual);
ਸਥਿਰ ਅਚਲ M3FusedCmpOps c_fuse_u32_GreaterThanOrEqual = d_fuseCmp (u32, GreaterThanOrEqual);
// clang-format on

// i32.eqz is unary: [0]= operand in _r0, [1]= operand in a slot
ਸਥਿਰ ਅਚਲ M3FusedCmpOps c_fuse_i32_Eqz = {
    { op_i32_BranchIfEqz_r, op_i32_BranchIfEqz_s, ਨਿਰਾ, ਨਿਰਾ },
    { op_i32_IfEqz_r,       op_i32_IfEqz_s,       ਨਿਰਾ, ਨਿਰਾ }
};

ਸਥਿਰ
ਅਚਲ M3FusedCmpOps* GetFusedCmpOps (m3opcode_t i_opcode)
{
    ਚੋਣ (i_opcode) {
    ਹਾਲਤ 0x45: ਵਾਪਸ &c_fuse_i32_Eqz;                    // i32.eqz
    ਹਾਲਤ 0x46: ਵਾਪਸ &c_fuse_i32_Equal;                  // i32.eq
    ਹਾਲਤ 0x47: ਵਾਪਸ &c_fuse_i32_NotEqual;               // i32.ne
    ਹਾਲਤ 0x48: ਵਾਪਸ &c_fuse_i32_LessThan;               // i32.lt_s
    ਹਾਲਤ 0x49: ਵਾਪਸ &c_fuse_u32_LessThan;               // i32.lt_u
    ਹਾਲਤ 0x4a: ਵਾਪਸ &c_fuse_i32_GreaterThan;            // i32.gt_s
    ਹਾਲਤ 0x4b: ਵਾਪਸ &c_fuse_u32_GreaterThan;            // i32.gt_u
    ਹਾਲਤ 0x4c: ਵਾਪਸ &c_fuse_i32_LessThanOrEqual;        // i32.le_s
    ਹਾਲਤ 0x4d: ਵਾਪਸ &c_fuse_u32_LessThanOrEqual;        // i32.le_u
    ਹਾਲਤ 0x4e: ਵਾਪਸ &c_fuse_i32_GreaterThanOrEqual;     // i32.ge_s
    ਹਾਲਤ 0x4f: ਵਾਪਸ &c_fuse_u32_GreaterThanOrEqual;     // i32.ge_u
    ਮੂਲ: ਵਾਪਸ ਨਿਰਾ;
    }
}

#ਜੇ_ਅੰਤ // d_m3FuseBranch

ਸਥਿਰ ਅਚਲ IM3Operation c_intSelectOps[2][4] = {
    { op_Select_i32_rss, op_Select_i32_srs, op_Select_i32_ssr, op_Select_i32_sss },
    { op_Select_i64_rss, op_Select_i64_srs, op_Select_i64_ssr, op_Select_i64_sss }
};
#ਜੇ d_m3HasFloat
ਸਥਿਰ ਅਚਲ IM3Operation c_fpSelectOps[2][2][3] = {
    { { op_Select_f32_sss, op_Select_f32_srs, op_Select_f32_ssr }, // selector in slot
      { op_Select_f32_rss, op_Select_f32_rrs, op_Select_f32_rsr } }, // selector in reg
    { { op_Select_f64_sss, op_Select_f64_srs, op_Select_f64_ssr }, // selector in slot
      { op_Select_f64_rss, op_Select_f64_rrs, op_Select_f64_rsr } }
};    // selector in reg
#ਜੇ_ਅੰਤ

// all args & returns are 64-bit aligned, so use 2 slots for a d_m3Use32BitSlots=1 build

ਸਥਿਰ
M3_ਨਤੀਜਾ Acquire_Compilation_ਕੋਡ_ਪੰਨਾ (IM3Compilation o, IM3_ਕੋਡ_ਪੰਨਾ* o_ਕੋਡ_ਪੰਨਾ)
{
    M3_ਨਤੀਜਾ ਨਤੀਜਾ = m3Err_none;

    IM3_ਕੋਡ_ਪੰਨਾ ਪੰਨਾ = Acquire_ਕੋਡ_ਪੰਨਾ(o->runtime);

    ਜੇ (ਪੰਨਾ) {
#ਜੇ (d_m3_Enable_ਕੋਡ_ਪੰਨਾ_Ref_Counting)
        {
            ਜੇ (o->function) {
                IM3Function func = o->function;
                ਪੰਨਾ->info.usage_ਗਿਣਤੀ++;

                u32 index = func->ਅੰਕ_ਕੋਡ_ਪੰਨਾ_Refs++;
_               (m3ReallocArray(&func->ਕੋਡ_ਪੰਨਾ_Refs, IM3_ਕੋਡ_ਪੰਨਾ, func->ਅੰਕ_ਕੋਡ_ਪੰਨਾ_Refs, index));
                func->ਕੋਡ_ਪੰਨਾ_Refs[index] = ਪੰਨਾ;
            }
        }
#ਜੇ_ਅੰਤ
    } ਨਹੀਂਤਾਂ {
        _throw(m3_Err_malloc_Failed_ਕੋਡ_ਪੰਨਾ);
    }

    _catch:

    *o_ਕੋਡ_ਪੰਨਾ = ਪੰਨਾ;

    ਵਾਪਸ ਨਤੀਜਾ;
}

ਸਥਿਰ ਸਤਰਅੰਦਰ
ਖਾਲੀ Release_Compilation_ਕੋਡ_ਪੰਨਾ (IM3Compilation o)
{
    Release_ਕੋਡ_ਪੰਨਾ(o->runtime, o->ਪੰਨਾ);
}

ਸਥਿਰ ਸਤਰਅੰਦਰ
u16 Get_Type_ਅੰਕ_Slots (m3type_t i_type)
{
    i_type = ਅਧਾਰ_Type_ਦਾ(i_type);

    // v128 is 16 bytes - 4 slots in 32-bit-slot mode, 2 in 64-bit.
    // (Slot-allocator only; no v128 ops execute.)
    ਜੇ (i_type == c_m3Type_v128)
#ਜੇ d_m3_Use32_ਦੋਆਧਾਰੀ_ਅੰਕ_Slots
        ਵਾਪਸ 4;
#ਨਹੀਂਤਾਂ
        ਵਾਪਸ 2;
#ਜੇ_ਅੰਤ
#ਜੇ d_m3_Use32_ਦੋਆਧਾਰੀ_ਅੰਕ_Slots
    ਵਾਪਸ Is64_ਦੋਆਧਾਰੀ_ਅੰਕ_Type(i_type) ? 2 : 1;
#ਨਹੀਂਤਾਂ
    ਵਾਪਸ 1;
#ਜੇ_ਅੰਤ
}

ਸਥਿਰ ਸਤਰਅੰਦਰ
ਖਾਲੀ Align_ਥਾਂ_ਮੰਜ਼ਿਲ_Type (u16* io_ਥਾਂ, m3type_t i_type)
{
    // align 64-bit words to even slots (if d_m3Use32BitSlots)
    u16 ਅੰਕ_Slots = Get_Type_ਅੰਕ_Slots(i_type);

    u16 ਨਕਾਬ = ਅੰਕ_Slots - 1;
    *io_ਥਾਂ = (*io_ਥਾਂ + ਨਕਾਬ) & ~ਨਕਾਬ;
}

// Deliberately unchecked. Validation rejects an empty operand stack before compilation.
ਸਥਿਰ ਸਤਰਅੰਦਰ
i16 Get_ਢੇਰ_ਸਿਖਰ_ਸੂਚਕਾਂਕ (IM3Compilation o)
{
    ਵਾਪਸ o->ਢੇਰ_ਸੂਚਕਾਂਕ - 1;
}

ਸਥਿਰ ਸਤਰਅੰਦਰ
M3_ਨਤੀਜਾ Get_ਢੇਰ_ਸਿਖਰ_ਸੂਚਕਾਂਕ_Throws (IM3Compilation o, i16* o_ਢੇਰ_ਸੂਚਕਾਂਕ)
{
    *o_ਢੇਰ_ਸੂਚਕਾਂਕ = o->ਢੇਰ_ਸੂਚਕਾਂਕ - 1;

    ਜੇ (M3_LIKELY(o->ਢੇਰ_ਸੂਚਕਾਂਕ > o->ਢੇਰ_ਪਹਿਲਾ_Dynamic_ਸੂਚਕਾਂਕ or Is_ਢੇਰ_Polymorphic(o))) {
        ਵਾਪਸ m3Err_none;
    } ਨਹੀਂਤਾਂ {
        ਵਾਪਸ m3_Err_function_ਢੇਰ_Underrun;
    }
}


// Items in the static portion of the stack (args/locals) are hidden from GetStackTypeFromTop ()
// In other words, only "real" Wasm stack items can be inspected.  This is important when
// returning values, etc. and you need an accurate wasm-view of the stack.
ਸਥਿਰ
m3type_t Get_ਢੇਰ_Type_From_ਸਿਖਰ (IM3Compilation o, u16 i_ਵਿੱਥ)
{
    m3type_t type = c_m3Type_none;

    ++i_ਵਿੱਥ;
    ਜੇ (o->ਢੇਰ_ਸੂਚਕਾਂਕ >= i_ਵਿੱਥ) {
        u16 index = o->ਢੇਰ_ਸੂਚਕਾਂਕ - i_ਵਿੱਥ;

        ਜੇ (index >= o->ਢੇਰ_ਪਹਿਲਾ_Dynamic_ਸੂਚਕਾਂਕ) {
            type = o->type_ਢੇਰ[index];
        }
    }

    ਵਾਪਸ type;
}

ਸਥਿਰ ਸਤਰਅੰਦਰ
m3type_t Get_ਢੇਰ_ਸਿਖਰ_Type (IM3Compilation o)
{
    ਵਾਪਸ Get_ਢੇਰ_Type_From_ਸਿਖਰ(o, 0);
}

ਸਥਿਰ ਸਤਰਅੰਦਰ
m3type_t Get_ਢੇਰ_Type_From_Bottom (IM3Compilation o, u16 i_ਵਿੱਥ)
{
    m3type_t type = c_m3Type_none;

    ਜੇ (i_ਵਿੱਥ < o->ਢੇਰ_ਸੂਚਕਾਂਕ) {
        type = o->type_ਢੇਰ[i_ਵਿੱਥ];
    }

    ਵਾਪਸ type;
}


ਸਥਿਰ ਸਤਰਅੰਦਰ
bool Is_Constant_ਥਾਂ (IM3Compilation o, u16 i_ਥਾਂ)
{
    ਵਾਪਸ (i_ਥਾਂ >= o->ਥਾਂ_ਪਹਿਲਾ_Const_ਸੂਚਕਾਂਕ and i_ਥਾਂ < o->ਥਾਂ_Max_Const_ਸੂਚਕਾਂਕ);
}
ਸਥਿਰ ਸਤਰਅੰਦਰ
bool Is_ਥਾਂ_Allocated (IM3Compilation o, u16 i_ਥਾਂ)
{
    ਵਾਪਸ o->m3Slots[i_ਥਾਂ];
}

ਸਥਿਰ ਸਤਰਅੰਦਰ
bool Is_ਢੇਰ_ਸੂਚਕਾਂਕ_ਵਿੱਚ_ਰਜਿਸਟਰ (IM3Compilation o, i32 i_ਢੇਰ_ਸੂਚਕਾਂਕ)
{                                                                           d_m3Assert (i_ਢੇਰ_ਸੂਚਕਾਂਕ < o->ਢੇਰ_ਸੂਚਕਾਂਕ or Is_ਢੇਰ_Polymorphic (o));
    ਜੇ (i_ਢੇਰ_ਸੂਚਕਾਂਕ >= 0 and i_ਢੇਰ_ਸੂਚਕਾਂਕ < o->ਢੇਰ_ਸੂਚਕਾਂਕ) {
        ਵਾਪਸ (o->wasm_ਢੇਰ[i_ਢੇਰ_ਸੂਚਕਾਂਕ] >= d_m3_Reg0_ਥਾਂ_Alias);
    } ਨਹੀਂਤਾਂ {
        ਵਾਪਸ false;
    }
}

ਸਥਿਰ ਸਤਰਅੰਦਰ
u16 Get_ਅੰਕ_ਖੰਡ_ਮੁੱਲਾਂ_On_ਢੇਰ (IM3Compilation o)
{
    ਵਾਪਸ o->ਢੇਰ_ਸੂਚਕਾਂਕ - o->ਖੰਡ.ਖੰਡ_ਢੇਰ_ਸੂਚਕਾਂਕ;
}

ਸਥਿਰ ਸਤਰਅੰਦਰ
bool Is_ਢੇਰ_ਸਿਖਰ_ਵਿੱਚ_ਰਜਿਸਟਰ (IM3Compilation o)
{
    ਵਾਪਸ Is_ਢੇਰ_ਸੂਚਕਾਂਕ_ਵਿੱਚ_ਰਜਿਸਟਰ(o, (i32)Get_ਢੇਰ_ਸਿਖਰ_ਸੂਚਕਾਂਕ(o));
}
ਸਥਿਰ ਸਤਰਅੰਦਰ
bool Is_ਢੇਰ_ਸਿਖਰ_Minus1_ਵਿੱਚ_ਰਜਿਸਟਰ (IM3Compilation o)
{
    ਵਾਪਸ Is_ਢੇਰ_ਸੂਚਕਾਂਕ_ਵਿੱਚ_ਰਜਿਸਟਰ(o, (i32)Get_ਢੇਰ_ਸਿਖਰ_ਸੂਚਕਾਂਕ(o) - 1);
}
ਸਥਿਰ ਸਤਰਅੰਦਰ
bool Is_ਢੇਰ_ਸਿਖਰ_Minus2_ਵਿੱਚ_ਰਜਿਸਟਰ (IM3Compilation o)
{
    ਵਾਪਸ Is_ਢੇਰ_ਸੂਚਕਾਂਕ_ਵਿੱਚ_ਰਜਿਸਟਰ(o, (i32)Get_ਢੇਰ_ਸਿਖਰ_ਸੂਚਕਾਂਕ(o) - 2);
}

ਸਥਿਰ ਸਤਰਅੰਦਰ
bool Is_ਢੇਰ_ਸਿਖਰ_ਵਿੱਚ_ਥਾਂ (IM3Compilation o)
{
    ਵਾਪਸ not Is_ਢੇਰ_ਸਿਖਰ_ਵਿੱਚ_ਰਜਿਸਟਰ(o);
}

ਸਥਿਰ ਸਤਰਅੰਦਰ
bool Is_Valid_ਥਾਂ (u16 i_ਥਾਂ)
{
    ਵਾਪਸ (i_ਥਾਂ < d_m3MaxFunctionSlots);
}

ਸਥਿਰ ਸਤਰਅੰਦਰ
u16 Get_ਢੇਰ_ਸਿਖਰ_ਥਾਂ_ਸੰਖਿਆ (IM3Compilation o)
{
    i16 i = Get_ਢੇਰ_ਸਿਖਰ_ਸੂਚਕਾਂਕ(o);

    u16 ਥਾਂ = c_ਥਾਂ_ਅਣਵਰਤਿਆ;

    ਜੇ (i >= 0) {
        ਥਾਂ = o->wasm_ਢੇਰ[i];
    }

    ਵਾਪਸ ਥਾਂ;
}


// from bottom
ਸਥਿਰ ਸਤਰਅੰਦਰ
u16 Get_ਥਾਂ_For_ਢੇਰ_ਸੂਚਕਾਂਕ (IM3Compilation o, u16 i_ਢੇਰ_ਸੂਚਕਾਂਕ)
{                                                                   d_m3Assert (i_ਢੇਰ_ਸੂਚਕਾਂਕ < o->ਢੇਰ_ਸੂਚਕਾਂਕ or Is_ਢੇਰ_Polymorphic (o));
    u16 ਥਾਂ = c_ਥਾਂ_ਅਣਵਰਤਿਆ;

    ਜੇ (i_ਢੇਰ_ਸੂਚਕਾਂਕ < o->ਢੇਰ_ਸੂਚਕਾਂਕ) {
        ਥਾਂ = o->wasm_ਢੇਰ[i_ਢੇਰ_ਸੂਚਕਾਂਕ];
    }

    ਵਾਪਸ ਥਾਂ;
}

ਸਥਿਰ ਸਤਰਅੰਦਰ
u16 Get_Extra_ਥਾਂ_For_ਢੇਰ_ਸੂਚਕਾਂਕ (IM3Compilation o, u16 i_ਢੇਰ_ਸੂਚਕਾਂਕ)
{
    u16 ਅਧਾਰ_ਥਾਂ = Get_ਥਾਂ_For_ਢੇਰ_ਸੂਚਕਾਂਕ(o, i_ਢੇਰ_ਸੂਚਕਾਂਕ);

    ਜੇ (ਅਧਾਰ_ਥਾਂ != c_ਥਾਂ_ਅਣਵਰਤਿਆ) {
        u16 extra_ਥਾਂ = Get_Type_ਅੰਕ_Slots(Get_ਢੇਰ_Type_From_Bottom(o, i_ਢੇਰ_ਸੂਚਕਾਂਕ)) - 1;
        ਅਧਾਰ_ਥਾਂ += extra_ਥਾਂ;
    }

    ਵਾਪਸ ਅਧਾਰ_ਥਾਂ;
}


ਸਥਿਰ ਸਤਰਅੰਦਰ
ਖਾਲੀ Touch_ਥਾਂ (IM3Compilation o, u16 i_ਥਾਂ)
{
    // op_Entry uses this value to track and detect stack overflow
    o->max_ਢੇਰ_Slots = M3_MAX(o->max_ਢੇਰ_Slots, i_ਥਾਂ + 1);
}

ਸਥਿਰ ਸਤਰਅੰਦਰ
ਖਾਲੀ Mark_ਥਾਂ_Allocated (IM3Compilation o, u16 i_ਥਾਂ)
{                                                                   d_m3Assert (o->m3Slots [i_ਥਾਂ] == 0); // shouldn't be already allocated
    o->m3Slots[i_ਥਾਂ] = 1;

    o->ਥਾਂ_Max_Allocated_ਸੂਚਕਾਂਕ_Plus_One = M3_MAX(o->ਥਾਂ_Max_Allocated_ਸੂਚਕਾਂਕ_Plus_One, i_ਥਾਂ + 1);

    Touch_ਥਾਂ(o, i_ਥਾਂ);
}

ਸਥਿਰ ਸਤਰਅੰਦਰ
M3_ਨਤੀਜਾ MarkSlotsAllocated (IM3Compilation o, u16 i_ਥਾਂ, u16 i_ਅੰਕ_Slots)
{
    ਜੇ (M3_UNLIKELY(i_ਥਾਂ + i_ਅੰਕ_Slots > d_m3MaxFunctionSlots)) {
        ਵਾਪਸ m3_Err_function_ਢੇਰ_ਉਛਾਲ;
    }

    ਜਦੋਂ (i_ਅੰਕ_Slots--) {
        Mark_ਥਾਂ_Allocated(o, i_ਥਾਂ++);
    }

    ਵਾਪਸ m3Err_none;
}

ਸਥਿਰ ਸਤਰਅੰਦਰ
M3_ਨਤੀਜਾ MarkSlotsAllocatedByType (IM3Compilation o, u16 i_ਥਾਂ, m3type_t i_type)
{
    u16 ਅੰਕ_Slots = Get_Type_ਅੰਕ_Slots(i_type);
    ਵਾਪਸ MarkSlotsAllocated(o, i_ਥਾਂ, ਅੰਕ_Slots);
}


ਸਥਿਰ
M3_ਨਤੀਜਾ AllocateSlotsWithinRange (IM3Compilation o, u16* o_ਥਾਂ, m3type_t i_type, u16 i_ਸ਼ੁਰੂ_ਥਾਂ, u16 i_ਸਿਰਾ_ਥਾਂ)
{
    M3_ਨਤੀਜਾ ਨਤੀਜਾ = m3_Err_function_ਢੇਰ_ਉਛਾਲ;

    u16 ਅੰਕ_Slots     = Get_Type_ਅੰਕ_Slots(i_type);
    u16 ਖੋਜ_ਵਿੱਥ = ਅੰਕ_Slots - 1;

    Align_ਥਾਂ_ਮੰਜ਼ਿਲ_Type(&i_ਸ਼ੁਰੂ_ਥਾਂ, i_type);

    // search for 1 or 2 consecutive slots in the execution stack
    u16 i = i_ਸ਼ੁਰੂ_ਥਾਂ;
    ਜਦੋਂ (i + ਖੋਜ_ਵਿੱਥ < i_ਸਿਰਾ_ਥਾਂ) {
        ਜੇ (i + ਖੋਜ_ਵਿੱਥ < d_m3MaxFunctionSlots and o->m3Slots[i] == 0 and o->m3Slots[i + ਖੋਜ_ਵਿੱਥ] == 0) {
            MarkSlotsAllocated(o, i, ਅੰਕ_Slots);

            *o_ਥਾਂ = i;
            ਨਤੀਜਾ  = m3Err_none;
            ਰੋਕੋ;
        }

        // keep 2-slot allocations even-aligned
        i += ਅੰਕ_Slots;
    }

    ਵਾਪਸ ਨਤੀਜਾ;
}

ਸਥਿਰ ਸਤਰਅੰਦਰ
M3_ਨਤੀਜਾ AllocateSlots (IM3Compilation o, u16* o_ਥਾਂ, m3type_t i_type)
{
    ਵਾਪਸ AllocateSlotsWithinRange(o, o_ਥਾਂ, i_type, o->ਥਾਂ_ਪਹਿਲਾ_Dynamic_ਸੂਚਕਾਂਕ, d_m3MaxFunctionSlots);
}

ਸਥਿਰ ਸਤਰਅੰਦਰ
M3_ਨਤੀਜਾ AllocateConstantSlots (IM3Compilation o, u16* o_ਥਾਂ, m3type_t i_type)
{
    u16 max_ਸਾਰਣੀ_ਸੂਚਕਾਂਕ = o->ਥਾਂ_ਪਹਿਲਾ_Const_ਸੂਚਕਾਂਕ + d_m3_Max_Constant_ਸਾਰਣੀ_ਮਾਪ;
    ਵਾਪਸ AllocateSlotsWithinRange(o, o_ਥਾਂ, i_type, o->ਥਾਂ_ਪਹਿਲਾ_Const_ਸੂਚਕਾਂਕ, M3_MIN(o->ਥਾਂ_ਪਹਿਲਾ_Dynamic_ਸੂਚਕਾਂਕ, max_ਸਾਰਣੀ_ਸੂਚਕਾਂਕ));
}


// TOQUE: this usage count system could be eliminated. real world code doesn't frequently trigger it.  just copy to multiple
// unique slots.
ਸਥਿਰ ਸਤਰਅੰਦਰ
M3_ਨਤੀਜਾ Increment_ਥਾਂ_Usage_ਗਿਣਤੀ (IM3Compilation o, u16 i_ਥਾਂ)
{                                                                                       d_m3Assert (i_ਥਾਂ < d_m3MaxFunctionSlots);
    M3_ਨਤੀਜਾ ਨਤੀਜਾ = m3Err_none;                                                       d_m3Assert (o->m3Slots [i_ਥਾਂ] > 0);

    // OPTZ (memory): 'm3Slots' could still be fused with 'typeStack' if 4 bits were used to indicate: [0,1,2,many]. The many-case
    // would scan 'wasmStack' to determine the actual usage count
    ਜੇ (M3_LIKELY(o->m3Slots[i_ਥਾਂ] < 0xFF)) {
        o->m3Slots[i_ਥਾਂ]++;
    } ਨਹੀਂਤਾਂ {
        ਨਤੀਜਾ = "slot usage count overflow";
    }

    ਵਾਪਸ ਨਤੀਜਾ;
}

ਸਥਿਰ ਸਤਰਅੰਦਰ
ਖਾਲੀ Deallocate_ਥਾਂ (IM3Compilation o, i16 i_ਥਾਂ, m3type_t i_type)
{                                                                                       d_m3Assert (i_ਥਾਂ >= o->ਥਾਂ_ਪਹਿਲਾ_Dynamic_ਸੂਚਕਾਂਕ);
                                                                                        d_m3Assert (i_ਥਾਂ < o->ਥਾਂ_Max_Allocated_ਸੂਚਕਾਂਕ_Plus_One);
    ਲਈ (u16 i = 0; i < Get_Type_ਅੰਕ_Slots(i_type); ++i, ++i_ਥਾਂ) {                       d_m3Assert (o->m3Slots [i_ਥਾਂ]);
        --o->m3Slots[i_ਥਾਂ];
    }
}


ਸਥਿਰ ਸਤਰਅੰਦਰ
bool Is_ਰਜਿਸਟਰ_Type_Allocated (IM3Compilation o, u8 i_type)
{
    ਵਾਪਸ Is_ਰਜਿਸਟਰ_Allocated(o, IsFpType(i_type));
}

ਸਥਿਰ ਸਤਰਅੰਦਰ
ਖਾਲੀ Allocate_ਰਜਿਸਟਰ (IM3Compilation o, u32 i_ਰਜਿਸਟਰ, u16 i_ਢੇਰ_ਸੂਚਕਾਂਕ)
{                                                                                       d_m3Assert (not Is_ਰਜਿਸਟਰ_Allocated (o, i_ਰਜਿਸਟਰ));
    o->reg_ਢੇਰ_ਸੂਚਕਾਂਕ_Plus_One[i_ਰਜਿਸਟਰ] = i_ਢੇਰ_ਸੂਚਕਾਂਕ + 1;
}

ਸਥਿਰ ਸਤਰਅੰਦਰ
ਖਾਲੀ Deallocate_ਰਜਿਸਟਰ (IM3Compilation o, u32 i_ਰਜਿਸਟਰ)
{                                                                                       d_m3Assert (Is_ਰਜਿਸਟਰ_Allocated (o, i_ਰਜਿਸਟਰ));
    o->reg_ਢੇਰ_ਸੂਚਕਾਂਕ_Plus_One[i_ਰਜਿਸਟਰ] = c_m3_ਰਜਿਸਟਰ_Unallocated;
}

ਸਥਿਰ ਸਤਰਅੰਦਰ
u16 Get_ਰਜਿਸਟਰ_ਢੇਰ_ਸੂਚਕਾਂਕ (IM3Compilation o, u32 i_ਰਜਿਸਟਰ)
{                                                                                       d_m3Assert (Is_ਰਜਿਸਟਰ_Allocated (o, i_ਰਜਿਸਟਰ));
    ਵਾਪਸ o->reg_ਢੇਰ_ਸੂਚਕਾਂਕ_Plus_One[i_ਰਜਿਸਟਰ] - 1;
}

u16 Get_Max_ਵਰਤਿਆ_ਥਾਂ_Plus_One (IM3Compilation o)
{
    ਜਦੋਂ (o->ਥਾਂ_Max_Allocated_ਸੂਚਕਾਂਕ_Plus_One > o->ਥਾਂ_ਪਹਿਲਾ_Dynamic_ਸੂਚਕਾਂਕ) {
        ਜੇ (Is_ਥਾਂ_Allocated(o, o->ਥਾਂ_Max_Allocated_ਸੂਚਕਾਂਕ_Plus_One - 1)) {
            ਰੋਕੋ;
        }

        o->ਥਾਂ_Max_Allocated_ਸੂਚਕਾਂਕ_Plus_One--;
    }

#ਜੇ_ਪਰਿਭਾਸ਼ਿਤ DEBUG
    u16 max_ਥਾਂ = o->ਥਾਂ_Max_Allocated_ਸੂਚਕਾਂਕ_Plus_One;
    ਜਦੋਂ (max_ਥਾਂ < d_m3MaxFunctionSlots) {
        d_m3Assert(o->m3Slots[max_ਥਾਂ] == 0);
        max_ਥਾਂ++;
    }
#ਜੇ_ਅੰਤ

    ਵਾਪਸ o->ਥਾਂ_Max_Allocated_ਸੂਚਕਾਂਕ_Plus_One;
}

ਸਥਿਰ
M3_ਨਤੀਜਾ Preserve_ਰਜਿਸਟਰ_If_Occupied (IM3Compilation o, m3type_t i_ਰਜਿਸਟਰ_Type)
{
    M3_ਨਤੀਜਾ ਨਤੀਜਾ = m3Err_none;

    u32 regSelect = IsFpType(i_ਰਜਿਸਟਰ_Type);

    ਜੇ (Is_ਰਜਿਸਟਰ_Allocated(o, regSelect)) {
        u16 ਢੇਰ_ਸੂਚਕਾਂਕ = Get_ਰਜਿਸਟਰ_ਢੇਰ_ਸੂਚਕਾਂਕ(o, regSelect);
        Deallocate_ਰਜਿਸਟਰ(o, regSelect);

        m3type_t type = Get_ਢੇਰ_Type_From_Bottom(o, ਢੇਰ_ਸੂਚਕਾਂਕ);

        // and point to a exec slot
        u16 ਥਾਂ = c_ਥਾਂ_ਅਣਵਰਤਿਆ;
_       (AllocateSlots(o, &ਥਾਂ, type));
        o->wasm_ਢੇਰ[ਢੇਰ_ਸੂਚਕਾਂਕ] = ਥਾਂ;

        // Ensure type is within the valid range
        ਜੇ (M3_LIKELY(ਅਧਾਰ_Type_ਦਾ(type) < c_m3_Type_ਗਿਣਤੀ)) {
_           (EmitOp(o, c_ਸੈੱਟ_ਸੈੱਟ_Ops[ਅਧਾਰ_Type_ਦਾ(type)]));
        } ਨਹੀਂਤਾਂ {
            _throw(m3Err_unknownType);
        }

        Emit_ਥਾਂ_ਵਿੱਥ(o, ਥਾਂ);
    }

    _catch: ਵਾਪਸ ਨਤੀਜਾ;
}


// all values must be in slots before entering loop, if, and else blocks
// otherwise they'd end up preserve-copied in the block to probably different locations (if/else)
ਸਥਿਰ ਸਤਰਅੰਦਰ
M3_ਨਤੀਜਾ PreserveRegisters (IM3Compilation o)
{
    M3_ਨਤੀਜਾ ਨਤੀਜਾ;

_   (Preserve_ਰਜਿਸਟਰ_If_Occupied(o, c_m3Type_f64));
_   (Preserve_ਰਜਿਸਟਰ_If_Occupied(o, c_m3Type_i64));

    _catch: ਵਾਪਸ ਨਤੀਜਾ;
}

ਸਥਿਰ
M3_ਨਤੀਜਾ Preserve_Non_ਸਿਖਰ_Registers (IM3Compilation o)
{
    M3_ਨਤੀਜਾ ਨਤੀਜਾ = m3Err_none;

    i16 ਢੇਰ_ਸਿਖਰ = Get_ਢੇਰ_ਸਿਖਰ_ਸੂਚਕਾਂਕ(o);

    ਜੇ (ਢੇਰ_ਸਿਖਰ >= 0) {
        ਜੇ (Is_ਰਜਿਸਟਰ_Allocated(o, 0))     // r0
        {
            ਜੇ (Get_ਰਜਿਸਟਰ_ਢੇਰ_ਸੂਚਕਾਂਕ(o, 0) != ਢੇਰ_ਸਿਖਰ) {
_               (Preserve_ਰਜਿਸਟਰ_If_Occupied(o, c_m3Type_i64));
            }
        }

        ਜੇ (Is_ਰਜਿਸਟਰ_Allocated(o, 1))     // fp0
        {
            ਜੇ (Get_ਰਜਿਸਟਰ_ਢੇਰ_ਸੂਚਕਾਂਕ(o, 1) != ਢੇਰ_ਸਿਖਰ) {
_               (Preserve_ਰਜਿਸਟਰ_If_Occupied(o, c_m3Type_f64));
            }
        }
    }

    _catch: ਵਾਪਸ ਨਤੀਜਾ;
}


//----------------------------------------------------------------------------------------------------------------------

ਸਥਿਰ
M3_ਨਤੀਜਾ ਧੱਕੋ (IM3Compilation o, m3type_t i_type, u16 i_ਥਾਂ)
{
    M3_ਨਤੀਜਾ ਨਤੀਜਾ = m3Err_none;

#ਜੇ !d_m3HasFloat
    ਜੇ (i_type == c_m3Type_f32 || i_type == c_m3Type_f64) {
        ਵਾਪਸ m3Err_unknownOpcode;
    }
#ਜੇ_ਅੰਤ

    ਜੇ (M3_UNLIKELY(o->ਢੇਰ_ਸੂਚਕਾਂਕ >= d_m3_Max_Function_ਢੇਰ_ਉਚਾਈ)) {
        ਵਾਪਸ m3_Err_function_ਢੇਰ_ਉਛਾਲ;
    }

    u16 ਢੇਰ_ਸੂਚਕਾਂਕ = o->ਢੇਰ_ਸੂਚਕਾਂਕ++;                                       // printf ("push: %d\n", (i32) i);

    o->wasm_ਢੇਰ[ਢੇਰ_ਸੂਚਕਾਂਕ] = i_ਥਾਂ;
    o->type_ਢੇਰ[ਢੇਰ_ਸੂਚਕਾਂਕ] = i_type;

    ਜੇ (Is_ਰਜਿਸਟਰ_ਥਾਂ_Alias(i_ਥਾਂ)) {
        u32 regSelect = Is_Fp_ਰਜਿਸਟਰ_ਥਾਂ_Alias(i_ਥਾਂ);
        Allocate_ਰਜਿਸਟਰ(o, regSelect, ਢੇਰ_ਸੂਚਕਾਂਕ);
    }

    ਜੇ (d_m3_Log_Wasm_ਢੇਰ) {
        dump_type_ਢੇਰ(o);
    }

    ਵਾਪਸ ਨਤੀਜਾ;
}

ਸਥਿਰ ਸਤਰਅੰਦਰ
M3_ਨਤੀਜਾ ਧੱਕੋ_ਰਜਿਸਟਰ (IM3Compilation o, m3type_t i_type)
{
    M3_ਨਤੀਜਾ ਨਤੀਜਾ = m3Err_none;                                                       d_m3Assert ((u16) d_m3_Reg0_ਥਾਂ_Alias > (u16) d_m3MaxFunctionSlots);
    u16      ਥਾਂ   = IsFpType(i_type) ? d_m3_Fp0_ਥਾਂ_Alias : d_m3_Reg0_ਥਾਂ_Alias;          d_m3Assert (i_type or Is_ਢੇਰ_Polymorphic (o));

_   (ਧੱਕੋ(o, i_type, ਥਾਂ));

    _catch: ਵਾਪਸ ਨਤੀਜਾ;
}

ਸਥਿਰ
M3_ਨਤੀਜਾ ਕੱਢੋ (IM3Compilation o)
{
    M3_ਨਤੀਜਾ ਨਤੀਜਾ = m3Err_none;

    ਜੇ (M3_LIKELY(o->ਢੇਰ_ਸੂਚਕਾਂਕ > o->ਖੰਡ.ਖੰਡ_ਢੇਰ_ਸੂਚਕਾਂਕ)) {
        o->ਢੇਰ_ਸੂਚਕਾਂਕ--;                                                //  printf ("pop: %d\n", (i32) o->stackIndex);

        u16      ਥਾਂ = o->wasm_ਢੇਰ[o->ਢੇਰ_ਸੂਚਕਾਂਕ];
        m3type_t type = o->type_ਢੇਰ[o->ਢੇਰ_ਸੂਚਕਾਂਕ];

        ਜੇ (Is_ਰਜਿਸਟਰ_ਥਾਂ_Alias(ਥਾਂ)) {
            u32 regSelect = Is_Fp_ਰਜਿਸਟਰ_ਥਾਂ_Alias(ਥਾਂ);
            Deallocate_ਰਜਿਸਟਰ(o, regSelect);
        } ਨਹੀਂਤਾਂ ਜੇ (M3_UNLIKELY(ਥਾਂ >= o->ਥਾਂ_Max_Allocated_ਸੂਚਕਾਂਕ_Plus_One)) {
            ਵਾਪਸ m3_Err_function_ਢੇਰ_Underrun; // Return error for invalid slot indices
        } ਨਹੀਂਤਾਂ ਜੇ (ਥਾਂ >= o->ਥਾਂ_ਪਹਿਲਾ_Dynamic_ਸੂਚਕਾਂਕ) {
            Deallocate_ਥਾਂ(o, ਥਾਂ, type);
        }
    } ਨਹੀਂਤਾਂ ਜੇ (not Is_ਢੇਰ_Polymorphic(o)) {
        ਨਤੀਜਾ = m3_Err_function_ਢੇਰ_Underrun;
    }

    ਵਾਪਸ ਨਤੀਜਾ;
}

ਸਥਿਰ
M3_ਨਤੀਜਾ ਕੱਢੋ_Type (IM3Compilation o, m3type_t i_type)
{
    M3_ਨਤੀਜਾ ਨਤੀਜਾ = m3Err_none;

    m3type_t ਸਿਖਰ_Type = Get_ਢੇਰ_ਸਿਖਰ_Type(o);

    ਜੇ (M3_LIKELY(Is_Sub_Type_ਦਾ(ਸਿਖਰ_Type, i_type) or o->ਖੰਡ.isPolymorphic)) {
_       (ਕੱਢੋ(o));
    } ਨਹੀਂਤਾਂ {
        _throw(m3Err_typeMismatch);
    }

    _catch:
    ਵਾਪਸ ਨਤੀਜਾ;
}

ਸਥਿਰ
M3_ਨਤੀਜਾ ਧੱਕੋ_Allocated_ਥਾਂ_And_Emit (IM3Compilation o, m3type_t i_type, bool i_doEmit)
{
    M3_ਨਤੀਜਾ ਨਤੀਜਾ = m3Err_none;

    u16 ਥਾਂ = c_ਥਾਂ_ਅਣਵਰਤਿਆ;

_   (AllocateSlots(o, &ਥਾਂ, i_type));
_   (ਧੱਕੋ(o, i_type, ਥਾਂ));

    ਜੇ (i_doEmit) {
        Emit_ਥਾਂ_ਵਿੱਥ(o, ਥਾਂ);
    }

    //    printf ("push: %d\n", (u32) slot);

    _catch: ਵਾਪਸ ਨਤੀਜਾ;
}

ਸਥਿਰ ਸਤਰਅੰਦਰ
M3_ਨਤੀਜਾ ਧੱਕੋ_Allocated_ਥਾਂ_And_Emit_2 (IM3Compilation o, m3type_t i_type)
{
    ਵਾਪਸ ਧੱਕੋ_Allocated_ਥਾਂ_And_Emit(o, i_type, true);
}

ਸਥਿਰ ਸਤਰਅੰਦਰ
M3_ਨਤੀਜਾ ਧੱਕੋ_Allocated_ਥਾਂ (IM3Compilation o, m3type_t i_type)
{
    ਵਾਪਸ ਧੱਕੋ_Allocated_ਥਾਂ_And_Emit(o, i_type, false);
}

ਸਥਿਰ
M3_ਨਤੀਜਾ ਧੱਕੋ_Const (IM3Compilation o, u64 i_ਸ਼ਬਦ, m3type_t i_type)
{
    M3_ਨਤੀਜਾ ਨਤੀਜਾ = m3Err_none;

    // When compile-walking a constant expression without emitting there is no
    // constant table to place the value in, but the type still has to land on
    // the stack: extended-const arithmetic downstream consumes it.
    ਜੇ (M3_UNLIKELY(!o->ਪੰਨਾ)) {
        ਵਾਪਸ ਧੱਕੋ_Allocated_ਥਾਂ(o, i_type);
    }

    bool ਮੇਲ_ਮਿਲਿਆ  = false;
    bool is64_ਦੋਆਧਾਰੀ_ਅੰਕ_Type = Is64_ਦੋਆਧਾਰੀ_ਅੰਕ_Type(i_type);

    u16 ਅੰਕ_Required_Slots  = Get_Type_ਅੰਕ_Slots(i_type);
    u16 ਅੰਕ_ਵਰਤਿਆ_Const_Slots = o->ਥਾਂ_Max_Const_ਸੂਚਕਾਂਕ - o->ਥਾਂ_ਪਹਿਲਾ_Const_ਸੂਚਕਾਂਕ;

    // search for duplicate matching constant slot to reuse
    ਜੇ (ਅੰਕ_Required_Slots == 2 and ਅੰਕ_ਵਰਤਿਆ_Const_Slots >= 2) {
        u16 ਪਹਿਲਾ_Const_ਥਾਂ = o->ਥਾਂ_ਪਹਿਲਾ_Const_ਸੂਚਕਾਂਕ;
        Align_ਥਾਂ_ਮੰਜ਼ਿਲ_Type(&ਪਹਿਲਾ_Const_ਥਾਂ, c_m3Type_i64);

        ਲਈ (u16 ਥਾਂ = ਪਹਿਲਾ_Const_ਥਾਂ; ਥਾਂ < o->ਥਾਂ_Max_Const_ਸੂਚਕਾਂਕ - 1; ਥਾਂ += 2) {
            ਜੇ (Is_ਥਾਂ_Allocated(o, ਥਾਂ) and Is_ਥਾਂ_Allocated(o, ਥਾਂ + 1)) {
                u64 constant;
                ਸਮ੍ਰਿਤੀ_ਨਕਲ(&constant, &o->constants[ਥਾਂ - o->ਥਾਂ_ਪਹਿਲਾ_Const_ਸੂਚਕਾਂਕ], ਆਕਾਰ(constant));

                ਜੇ (constant == i_ਸ਼ਬਦ) {
                    ਮੇਲ_ਮਿਲਿਆ = true;
_                   (ਧੱਕੋ(o, i_type, ਥਾਂ));
                    ਰੋਕੋ;
                }
            }
        }
    } ਨਹੀਂਤਾਂ ਜੇ (ਅੰਕ_Required_Slots == 1) {
        ਲਈ (u16 i = 0; i < ਅੰਕ_ਵਰਤਿਆ_Const_Slots; ++i) {
            u16 ਥਾਂ = o->ਥਾਂ_ਪਹਿਲਾ_Const_ਸੂਚਕਾਂਕ + i;

            ਜੇ (Is_ਥਾਂ_Allocated(o, ਥਾਂ)) {
                bool matches;
                ਜੇ (is64_ਦੋਆਧਾਰੀ_ਅੰਕ_Type) {
                    u64 constant;
                    ਸਮ੍ਰਿਤੀ_ਨਕਲ(&constant, &o->constants[i], ਆਕਾਰ(constant));
                    matches = (constant == i_ਸ਼ਬਦ);
                } ਨਹੀਂਤਾਂ {
                    u32 constant;
                    ਸਮ੍ਰਿਤੀ_ਨਕਲ(&constant, &o->constants[i], ਆਕਾਰ(constant));
                    matches = (constant == i_ਸ਼ਬਦ);
                }
                ਜੇ (matches) {
                    ਮੇਲ_ਮਿਲਿਆ = true;
_                   (ਧੱਕੋ(o, i_type, ਥਾਂ));
                    ਰੋਕੋ;
                }
            }
        }
    }

    ਜੇ (not ਮੇਲ_ਮਿਲਿਆ) {
        u16 ਥਾਂ = c_ਥਾਂ_ਅਣਵਰਤਿਆ;
        ਨਤੀਜਾ   = AllocateConstantSlots(o, &ਥਾਂ, i_type);

        ਜੇ (ਨਤੀਜਾ || ਥਾਂ == c_ਥਾਂ_ਅਣਵਰਤਿਆ) // no more constant table space; use inline constants
        {
            ਨਤੀਜਾ = m3Err_none;

            ਜੇ (is64_ਦੋਆਧਾਰੀ_ਅੰਕ_Type) {
_               (EmitOp(o, op_Const64));
                EmitWord64(o->ਪੰਨਾ, i_ਸ਼ਬਦ);
            } ਨਹੀਂਤਾਂ {
_               (EmitOp(o, op_Const32));
                EmitWord32(o->ਪੰਨਾ, (u32)i_ਸ਼ਬਦ);
            }

_           (ਧੱਕੋ_Allocated_ਥਾਂ_And_Emit_2(o, i_type));
        } ਨਹੀਂਤਾਂ {
            u16 const_ਸਾਰਣੀ_ਸੂਚਕਾਂਕ = ਥਾਂ - o->ਥਾਂ_ਪਹਿਲਾ_Const_ਸੂਚਕਾਂਕ;

            d_m3Assert(const_ਸਾਰਣੀ_ਸੂਚਕਾਂਕ < d_m3_Max_Constant_ਸਾਰਣੀ_ਮਾਪ);

            ਜੇ (is64_ਦੋਆਧਾਰੀ_ਅੰਕ_Type) {
                ਸਮ੍ਰਿਤੀ_ਨਕਲ(&o->constants[const_ਸਾਰਣੀ_ਸੂਚਕਾਂਕ], &i_ਸ਼ਬਦ, ਆਕਾਰ(i_ਸ਼ਬਦ));
            } ਨਹੀਂਤਾਂ {
                u32 word32 = (u32)i_ਸ਼ਬਦ;
                ਸਮ੍ਰਿਤੀ_ਨਕਲ(&o->constants[const_ਸਾਰਣੀ_ਸੂਚਕਾਂਕ], &word32, ਆਕਾਰ(word32));
            }

_           (ਧੱਕੋ(o, i_type, ਥਾਂ));

            o->ਥਾਂ_Max_Const_ਸੂਚਕਾਂਕ = M3_MAX(ਥਾਂ + ਅੰਕ_Required_Slots, o->ਥਾਂ_Max_Const_ਸੂਚਕਾਂਕ);
        }
    }

    _catch: ਵਾਪਸ ਨਤੀਜਾ;
}

ਸਥਿਰ ਸਤਰਅੰਦਰ
M3_ਨਤੀਜਾ Emit_ਥਾਂ_ਅੰਕ_ਦਾ_ਢੇਰ_ਸਿਖਰ_And_ਕੱਢੋ (IM3Compilation o)
{
    // no emit if value is in register
    ਜੇ (Is_ਢੇਰ_ਸਿਖਰ_ਵਿੱਚ_ਥਾਂ(o)) {
        Emit_ਥਾਂ_ਵਿੱਥ(o, Get_ਢੇਰ_ਸਿਖਰ_ਥਾਂ_ਸੰਖਿਆ(o));
    }

    ਵਾਪਸ ਕੱਢੋ(o);
}


// Or, maybe: EmitTrappingOp
M3_ਨਤੀਜਾ ਜੋੜੋ_Trap_ਰਿਕਾਰਡ (IM3Compilation o)
{
    M3_ਨਤੀਜਾ ਨਤੀਜਾ = m3Err_none;

    ਜੇ (o->function) {
    }

    ਵਾਪਸ ਨਤੀਜਾ;
}

ਸਥਿਰ
M3_ਨਤੀਜਾ Unwind_ਖੰਡ_ਢੇਰ (IM3Compilation o)
{
    M3_ਨਤੀਜਾ ਨਤੀਜਾ = m3Err_none;

    u32 ਕੱਢੋ_ਗਿਣਤੀ = 0;
    ਜਦੋਂ (o->ਢੇਰ_ਸੂਚਕਾਂਕ > o->ਖੰਡ.ਖੰਡ_ਢੇਰ_ਸੂਚਕਾਂਕ) {
_       (ਕੱਢੋ(o));
        ++ਕੱਢੋ_ਗਿਣਤੀ;
    }

    ਜੇ (ਕੱਢੋ_ਗਿਣਤੀ) {
        m3log(compile, "unwound stack top: %d", ਕੱਢੋ_ਗਿਣਤੀ);
    }

    _catch: ਵਾਪਸ ਨਤੀਜਾ;
}

ਸਥਿਰ ਸਤਰਅੰਦਰ
M3_ਨਤੀਜਾ ਸੈੱਟ_ਢੇਰ_Polymorphic (IM3Compilation o)
{
    o->ਖੰਡ.isPolymorphic = true;                              m3log (compile, "stack set polymorphic");
    ਵਾਪਸ Unwind_ਖੰਡ_ਢੇਰ(o);
}

ਸਥਿਰ
ਖਾਲੀ PatchBranches (IM3Compilation o)
{
    InvalidateFold(o);         // patched branches land at the current pc

    pc_t pc = GetPC(o);

    pc_t patches     = o->ਖੰਡ.patches;
    o->ਖੰਡ.patches = ਨਿਰਾ;

    ਜਦੋਂ (patches) {                                           m3log (compile, "patching location: %p to pc: %p", patches, pc);
        pc_t ਅਗਲਾ       = *(pc_t*)patches;
        *(pc_t*)patches = pc;
        patches         = ਅਗਲਾ;
    }
}

//-------------------------------------------------------------------------------------------------------------------------

ਸਥਿਰ
M3_ਨਤੀਜਾ Copy_ਢੇਰ_ਸੂਚਕਾਂਕ_ਮੰਜ਼ਿਲ_ਥਾਂ (IM3Compilation o, u16 i_dest_ਥਾਂ, u16 i_ਢੇਰ_ਸੂਚਕਾਂਕ)  // NoPushPop
{
    M3_ਨਤੀਜਾ ਨਤੀਜਾ = m3Err_none;

    IM3Operation op;

    m3type_t type       = Get_ਢੇਰ_Type_From_Bottom(o, i_ਢੇਰ_ਸੂਚਕਾਂਕ);
    bool     ਵਿੱਚ_ਰਜਿਸਟਰ = Is_ਢੇਰ_ਸੂਚਕਾਂਕ_ਵਿੱਚ_ਰਜਿਸਟਰ(o, i_ਢੇਰ_ਸੂਚਕਾਂਕ);

    ਜੇ (ਵਿੱਚ_ਰਜਿਸਟਰ) {
        op = c_ਸੈੱਟ_ਸੈੱਟ_Ops[ਅਧਾਰ_Type_ਦਾ(type)];
    } ਨਹੀਂਤਾਂ {
        op = Is64_ਦੋਆਧਾਰੀ_ਅੰਕ_Type(type) ? op_Copy_ਥਾਂ_64 : op_Copy_ਥਾਂ_32;
    }

_   (EmitOp(o, op));
    Emit_ਥਾਂ_ਵਿੱਥ(o, i_dest_ਥਾਂ);

    ਜੇ (not ਵਿੱਚ_ਰਜਿਸਟਰ) {
        u16 src_ਥਾਂ = Get_ਥਾਂ_For_ਢੇਰ_ਸੂਚਕਾਂਕ(o, i_ਢੇਰ_ਸੂਚਕਾਂਕ);
        Emit_ਥਾਂ_ਵਿੱਥ(o, src_ਥਾਂ);
    }

    _catch: ਵਾਪਸ ਨਤੀਜਾ;
}

ਸਥਿਰ
M3_ਨਤੀਜਾ Copy_ਢੇਰ_ਸਿਖਰ_ਮੰਜ਼ਿਲ_ਥਾਂ (IM3Compilation o, u16 i_dest_ਥਾਂ)  // NoPushPop
{
    M3_ਨਤੀਜਾ ਨਤੀਜਾ;

    i16 ਢੇਰ_ਸਿਖਰ;
_   (Get_ਢੇਰ_ਸਿਖਰ_ਸੂਚਕਾਂਕ_Throws(o, &ਢੇਰ_ਸਿਖਰ));
_   (Copy_ਢੇਰ_ਸੂਚਕਾਂਕ_ਮੰਜ਼ਿਲ_ਥਾਂ(o, i_dest_ਥਾਂ, (u16)ਢੇਰ_ਸਿਖਰ));

    _catch: ਵਾਪਸ ਨਤੀਜਾ;
}


// a copy-on-write strategy is used with locals. when a get local occurs, it's not copied anywhere. the stack
// entry just has a index pointer to that local memory slot.
// then, when a previously referenced local is set, the current value needs to be preserved for those references

// TODO: consider getting rid of these specialized operations: PreserveSetSlot & PreserveCopySlot.
// They likely just take up space (which seems to reduce performance) without improving performance.
// => unconfirmed in preliminary testing.
ਸਥਿਰ
M3_ਨਤੀਜਾ Preserved_Copy_ਸਿਖਰ_ਥਾਂ (IM3Compilation o, u16 i_dest_ਥਾਂ, u16 i_preserve_ਥਾਂ)
{
    M3_ਨਤੀਜਾ ਨਤੀਜਾ = m3Err_none;             d_m3Assert (i_dest_ਥਾਂ != i_preserve_ਥਾਂ);

    IM3Operation op;

    m3type_t type = Get_ਢੇਰ_ਸਿਖਰ_Type(o);

    ਜੇ (Is_ਢੇਰ_ਸਿਖਰ_ਵਿੱਚ_ਰਜਿਸਟਰ(o)) {
        op = c_preserve_ਸੈੱਟ_ਥਾਂ[ਅਧਾਰ_Type_ਦਾ(type)];
    } ਨਹੀਂਤਾਂ {
        op = Is64_ਦੋਆਧਾਰੀ_ਅੰਕ_Type(type) ? op_Preserve_Copy_ਥਾਂ_64 : op_Preserve_Copy_ਥਾਂ_32;
    }

_   (EmitOp(o, op));
    Emit_ਥਾਂ_ਵਿੱਥ(o, i_dest_ਥਾਂ);

    ਜੇ (Is_ਢੇਰ_ਸਿਖਰ_ਵਿੱਚ_ਥਾਂ(o)) {
        Emit_ਥਾਂ_ਵਿੱਥ(o, Get_ਢੇਰ_ਸਿਖਰ_ਥਾਂ_ਸੰਖਿਆ(o));
    }

    Emit_ਥਾਂ_ਵਿੱਥ(o, i_preserve_ਥਾਂ);

    _catch: ਵਾਪਸ ਨਤੀਜਾ;
}

ਸਥਿਰ
M3_ਨਤੀਜਾ Copy_ਢੇਰ_ਸਿਖਰ_ਮੰਜ਼ਿਲ_ਰਜਿਸਟਰ (IM3Compilation o, bool i_update_ਢੇਰ)
{
    M3_ਨਤੀਜਾ ਨਤੀਜਾ = m3Err_none;

    ਜੇ (Is_ਢੇਰ_ਸਿਖਰ_ਵਿੱਚ_ਥਾਂ(o)) {
        m3type_t type = Get_ਢੇਰ_ਸਿਖਰ_Type(o);

_       (Preserve_ਰਜਿਸਟਰ_If_Occupied(o, type));

        IM3Operation op = c_ਸੈੱਟ_ਰਜਿਸਟਰ_Ops[ਅਧਾਰ_Type_ਦਾ(type)];

_       (EmitOp(o, op));
        Emit_ਥਾਂ_ਵਿੱਥ(o, Get_ਢੇਰ_ਸਿਖਰ_ਥਾਂ_ਸੰਖਿਆ(o));

        ਜੇ (i_update_ਢੇਰ) {
_           (ਕੱਢੋ_Type(o, type));
_           (ਧੱਕੋ_ਰਜਿਸਟਰ(o, type));
        }
    }

    _catch: ਵਾਪਸ ਨਤੀਜਾ;
}


// if local is unreferenced, o_preservedSlotNumber will be equal to localIndex on return
ਸਥਿਰ
M3_ਨਤੀਜਾ ਲੱਭੋ_Referenced_Local_Within_Current_ਖੰਡ (IM3Compilation o, u16* o_preserved_ਥਾਂ_ਸੰਖਿਆ, u32 i_local_ਥਾਂ)
{
    M3_ਨਤੀਜਾ ਨਤੀਜਾ = m3Err_none;

    IM3CompilationScope scope = &o->ਖੰਡ;

    u16 ਸ਼ੁਰੂ_ਸੂਚਕਾਂਕ = scope->ਖੰਡ_ਢੇਰ_ਸੂਚਕਾਂਕ;

    ਜਦੋਂ (scope->opcode == c_wa_Op_ਖੰਡ) {
        scope = scope->outer;
        ਜੇ (not scope) {
            ਰੋਕੋ;
        }

        ਸ਼ੁਰੂ_ਸੂਚਕਾਂਕ = scope->ਖੰਡ_ਢੇਰ_ਸੂਚਕਾਂਕ;
    }

    *o_preserved_ਥਾਂ_ਸੰਖਿਆ = (u16)i_local_ਥਾਂ;

    ਲਈ (u32 i = ਸ਼ੁਰੂ_ਸੂਚਕਾਂਕ; i < o->ਢੇਰ_ਸੂਚਕਾਂਕ; ++i) {
        ਜੇ (o->wasm_ਢੇਰ[i] == i_local_ਥਾਂ) {
            m3type_t type = Get_ਢੇਰ_Type_From_Bottom(o, i);                         d_m3Assert (type != c_m3Type_none)

            ਜੇ (*o_preserved_ਥਾਂ_ਸੰਖਿਆ == i_local_ਥਾਂ) {
_               (AllocateSlots(o, o_preserved_ਥਾਂ_ਸੰਖਿਆ, type));
            } ਨਹੀਂਤਾਂ {
                // A 64-bit value spans two slots, and allocating or releasing it
                // counts both, so a second reference has to count both as well.
                ਲਈ (u16 s = 0; s < Get_Type_ਅੰਕ_Slots(type); ++s) {
_                   (Increment_ਥਾਂ_Usage_ਗਿਣਤੀ(o, (u16)(*o_preserved_ਥਾਂ_ਸੰਖਿਆ + s)));
                }
            }

            o->wasm_ਢੇਰ[i] = *o_preserved_ਥਾਂ_ਸੰਖਿਆ;
        }
    }

    _catch: ਵਾਪਸ ਨਤੀਜਾ;
}

ਸਥਿਰ
M3_ਨਤੀਜਾ Get_ਖੰਡ_Scope (IM3Compilation o, IM3CompilationScope* o_scope, u32 i_ਡੂੰਘਾਈ)
{
    M3_ਨਤੀਜਾ ਨਤੀਜਾ = m3Err_none;

    IM3CompilationScope scope = &o->ਖੰਡ;

    ਜਦੋਂ (i_ਡੂੰਘਾਈ--) {
        scope = scope->outer;
        _throwif("invalid block depth", not scope);
    }

    *o_scope = scope;

    _catch:
    ਵਾਪਸ ਨਤੀਜਾ;
}

ਸਥਿਰ
M3_ਨਤੀਜਾ Copy_ਢੇਰ_Slots_R (IM3Compilation o, u16 i_ਨਿਸ਼ਾਨਾ_ਥਾਂ_ਢੇਰ_ਸੂਚਕਾਂਕ, u16 i_ਢੇਰ_ਸੂਚਕਾਂਕ, u16 i_ਸਿਰਾ_ਢੇਰ_ਸੂਚਕਾਂਕ, u16 i_ਆਰਜ਼ੀ_ਥਾਂ)
{
    M3_ਨਤੀਜਾ ਨਤੀਜਾ = m3Err_none;

    ਜੇ (i_ਢੇਰ_ਸੂਚਕਾਂਕ < i_ਸਿਰਾ_ਢੇਰ_ਸੂਚਕਾਂਕ) {
        u16 src_ਥਾਂ = Get_ਥਾਂ_For_ਢੇਰ_ਸੂਚਕਾਂਕ(o, i_ਢੇਰ_ਸੂਚਕਾਂਕ);

        m3type_t type      = Get_ਢੇਰ_Type_From_Bottom(o, i_ਢੇਰ_ਸੂਚਕਾਂਕ);
        u16      ਅੰਕ_Slots  = Get_Type_ਅੰਕ_Slots(type);
        u16      extra_ਥਾਂ = ਅੰਕ_Slots - 1;

        u16 ਨਿਸ਼ਾਨਾ_ਥਾਂ = Get_ਥਾਂ_For_ਢੇਰ_ਸੂਚਕਾਂਕ(o, i_ਨਿਸ਼ਾਨਾ_ਥਾਂ_ਢੇਰ_ਸੂਚਕਾਂਕ);

        u16 preserve_ਸੂਚਕਾਂਕ = i_ਢੇਰ_ਸੂਚਕਾਂਕ;
        u16 collision_ਥਾਂ = src_ਥਾਂ;

        ਜੇ (ਨਿਸ਼ਾਨਾ_ਥਾਂ != src_ਥਾਂ) {
            // search for collisions
            u16 check_ਸੂਚਕਾਂਕ = i_ਢੇਰ_ਸੂਚਕਾਂਕ + 1;
            ਜਦੋਂ (check_ਸੂਚਕਾਂਕ < i_ਸਿਰਾ_ਢੇਰ_ਸੂਚਕਾਂਕ) {
                u16 otherSlot1 = Get_ਥਾਂ_For_ਢੇਰ_ਸੂਚਕਾਂਕ(o, check_ਸੂਚਕਾਂਕ);
                u16 otherSlot2 = Get_Extra_ਥਾਂ_For_ਢੇਰ_ਸੂਚਕਾਂਕ(o, check_ਸੂਚਕਾਂਕ);

                ਜੇ (ਨਿਸ਼ਾਨਾ_ਥਾਂ == otherSlot1 or
                    ਨਿਸ਼ਾਨਾ_ਥਾਂ == otherSlot2 or
                    ਨਿਸ਼ਾਨਾ_ਥਾਂ + extra_ਥਾਂ == otherSlot1) {
                    _throwif(m3_Err_function_ਢੇਰ_ਉਛਾਲ, i_ਆਰਜ਼ੀ_ਥਾਂ >= d_m3MaxFunctionSlots);

_                   (Copy_ਢੇਰ_ਸੂਚਕਾਂਕ_ਮੰਜ਼ਿਲ_ਥਾਂ(o, i_ਆਰਜ਼ੀ_ਥਾਂ, check_ਸੂਚਕਾਂਕ));
                    o->wasm_ਢੇਰ[check_ਸੂਚਕਾਂਕ] = i_ਆਰਜ਼ੀ_ਥਾਂ;
                    i_ਆਰਜ਼ੀ_ਥਾਂ += Get_Type_ਅੰਕ_Slots(c_m3Type_i64);
                    Touch_ਥਾਂ(o, i_ਆਰਜ਼ੀ_ਥਾਂ - 1);

                    // restore this on the way back down
                    preserve_ਸੂਚਕਾਂਕ = check_ਸੂਚਕਾਂਕ;
                    collision_ਥਾਂ = otherSlot1;

                    ਰੋਕੋ;
                }

                ++check_ਸੂਚਕਾਂਕ;
            }

_           (Copy_ਢੇਰ_ਸੂਚਕਾਂਕ_ਮੰਜ਼ਿਲ_ਥਾਂ(o, ਨਿਸ਼ਾਨਾ_ਥਾਂ, i_ਢੇਰ_ਸੂਚਕਾਂਕ));                                                m3log (compile, " copying slot: %d to slot: %d", src_ਥਾਂ, ਨਿਸ਼ਾਨਾ_ਥਾਂ);
            o->wasm_ਢੇਰ[i_ਢੇਰ_ਸੂਚਕਾਂਕ] = ਨਿਸ਼ਾਨਾ_ਥਾਂ;
        }

_       (Copy_ਢੇਰ_Slots_R(o, i_ਨਿਸ਼ਾਨਾ_ਥਾਂ_ਢੇਰ_ਸੂਚਕਾਂਕ + 1, i_ਢੇਰ_ਸੂਚਕਾਂਕ + 1, i_ਸਿਰਾ_ਢੇਰ_ਸੂਚਕਾਂਕ, i_ਆਰਜ਼ੀ_ਥਾਂ));

        // restore the stack state
        o->wasm_ਢੇਰ[i_ਢੇਰ_ਸੂਚਕਾਂਕ]  = src_ਥਾਂ;
        o->wasm_ਢੇਰ[preserve_ਸੂਚਕਾਂਕ] = collision_ਥਾਂ;
    }

    _catch:
    ਵਾਪਸ ਨਤੀਜਾ;
}

ਸਥਿਰ
M3_ਨਤੀਜਾ Resolve_ਖੰਡ_Results (IM3Compilation o, IM3CompilationScope i_ਨਿਸ਼ਾਨਾ_ਖੰਡ, bool i_isBranch)
{
    M3_ਨਤੀਜਾ ਨਤੀਜਾ = m3Err_none;
    ਜੇ (d_m3_Log_Wasm_ਢੇਰ) {
        dump_type_ਢੇਰ(o);
    }

    bool isLoop = (i_ਨਿਸ਼ਾਨਾ_ਖੰਡ->opcode == c_waOp_loop and i_isBranch);

    u16 ਅੰਕ_Params  = Get_Func_Type_ਅੰਕ_Params(i_ਨਿਸ਼ਾਨਾ_ਖੰਡ->type);
    u16 ਅੰਕ_Results = Get_Func_Type_ਅੰਕ_Results(i_ਨਿਸ਼ਾਨਾ_ਖੰਡ->type);

    u16 ਥਾਂ_ਰਿਕਾਰਡਾਂ = i_ਨਿਸ਼ਾਨਾ_ਖੰਡ->exit_ਢੇਰ_ਸੂਚਕਾਂਕ;

    u16 ਅੰਕ_ਮੁੱਲਾਂ;

    ਜੇ (not isLoop) {
        ਅੰਕ_ਮੁੱਲਾਂ = ਅੰਕ_Results;
        ਥਾਂ_ਰਿਕਾਰਡਾਂ += ਅੰਕ_Params;
    } ਨਹੀਂਤਾਂ {
        ਅੰਕ_ਮੁੱਲਾਂ = ਅੰਕ_Params;
    }

    u16 ਖੰਡ_ਉਚਾਈ = Get_ਅੰਕ_ਖੰਡ_ਮੁੱਲਾਂ_On_ਢੇਰ(o);

    _throwif(m3_Err_type_ਗਿਣਤੀ_Mismatch, i_isBranch ? (ਖੰਡ_ਉਚਾਈ < ਅੰਕ_ਮੁੱਲਾਂ) : (ਖੰਡ_ਉਚਾਈ != ਅੰਕ_ਮੁੱਲਾਂ));

    ਜੇ (ਅੰਕ_ਮੁੱਲਾਂ) {
        u16 ਸਿਰਾ_ਸੂਚਕਾਂਕ     = Get_ਢੇਰ_ਸਿਖਰ_ਸੂਚਕਾਂਕ(o) + 1;
        u16 ਅੰਕ_Rem_ਮੁੱਲਾਂ = ਅੰਕ_ਮੁੱਲਾਂ;

        // The last result is taken from _fp0. See PushBlockResults.
        ਜੇ (not isLoop and IsFpType(Get_ਢੇਰ_ਸਿਖਰ_Type(o))) {
_           (Copy_ਢੇਰ_ਸਿਖਰ_ਮੰਜ਼ਿਲ_ਰਜਿਸਟਰ(o, false));
            --ਸਿਰਾ_ਸੂਚਕਾਂਕ;
            --ਅੰਕ_Rem_ਮੁੱਲਾਂ;
        }

        // tempslot affects maxStackSlots, so can grow unnecess each time.
        // in practice, this is hardly ever hit.
        u16 ਆਰਜ਼ੀ_ਥਾਂ = o->max_ਢੇਰ_Slots;// GetMaxUsedSlotPlusOne (o); doesn't work cause can collide with slotRecords
        Align_ਥਾਂ_ਮੰਜ਼ਿਲ_Type(&ਆਰਜ਼ੀ_ਥਾਂ, c_m3Type_i64);

_       (Copy_ਢੇਰ_Slots_R(o, ਥਾਂ_ਰਿਕਾਰਡਾਂ, ਸਿਰਾ_ਸੂਚਕਾਂਕ - ਅੰਕ_Rem_ਮੁੱਲਾਂ, ਸਿਰਾ_ਸੂਚਕਾਂਕ, ਆਰਜ਼ੀ_ਥਾਂ));

        ਜੇ (d_m3_Log_Wasm_ਢੇਰ) {
            dump_type_ਢੇਰ(o);
        }
    }

    _catch: ਵਾਪਸ ਨਤੀਜਾ;
}


ਸਥਿਰ
M3_ਨਤੀਜਾ Return_ਮੁੱਲਾਂ (IM3Compilation o, IM3CompilationScope i_function_ਖੰਡ, bool i_isBranch)
{
    M3_ਨਤੀਜਾ ਨਤੀਜਾ = m3Err_none;
    ਜੇ (d_m3_Log_Wasm_ਢੇਰ) {
        dump_type_ਢੇਰ(o);
    }

    u16 ਅੰਕ_Returns  = Get_Func_Type_ਅੰਕ_Results(i_function_ਖੰਡ->type);     // could just o->function too...
    u16 ਖੰਡ_ਉਚਾਈ = Get_ਅੰਕ_ਖੰਡ_ਮੁੱਲਾਂ_On_ਢੇਰ(o);

    ਜੇ (not Is_ਢੇਰ_Polymorphic(o)) {
        _throwif(m3_Err_type_ਗਿਣਤੀ_Mismatch, i_isBranch ? (ਖੰਡ_ਉਚਾਈ < ਅੰਕ_Returns) : (ਖੰਡ_ਉਚਾਈ != ਅੰਕ_Returns));
    }

    ਜੇ (ਅੰਕ_Returns) {
        // return slots like args are 64-bit aligned
        u16 return_ਥਾਂ = ਅੰਕ_Returns * c_io_ਥਾਂ_ਗਿਣਤੀ;
        u16 ਢੇਰ_ਸਿਖਰ   = Get_ਢੇਰ_ਸਿਖਰ_ਸੂਚਕਾਂਕ(o);

        ਲਈ (u16 i = 0; i < ਅੰਕ_Returns; ++i) {
            m3type_t returnType = Get_Func_Type_ਨਤੀਜਾ_Type(i_function_ਖੰਡ->type, ਅੰਕ_Returns - 1 - i);

            m3type_t ਢੇਰ_Type = Get_ਢੇਰ_Type_From_ਸਿਖਰ(o, i);  // using FromTop so that only dynamic items are checked

            ਜੇ (Is_ਢੇਰ_Polymorphic(o) and ਢੇਰ_Type == c_m3Type_none) {
                ਢੇਰ_Type = returnType;
            }

            _throwif(m3Err_typeMismatch, not Is_Sub_Type_ਦਾ(ਢੇਰ_Type, returnType));

            ਜੇ (not Is_ਢੇਰ_Polymorphic(o)) {
                return_ਥਾਂ -= c_io_ਥਾਂ_ਗਿਣਤੀ;
_               (Copy_ਢੇਰ_ਸੂਚਕਾਂਕ_ਮੰਜ਼ਿਲ_ਥਾਂ(o, return_ਥਾਂ, ਢੇਰ_ਸਿਖਰ--));
            }
        }

        ਜੇ (not i_isBranch) {
            ਜਦੋਂ (ਅੰਕ_Returns--) {
_               (ਕੱਢੋ(o));
            }
        }
    }

    _catch: ਵਾਪਸ ਨਤੀਜਾ;
}


//-------------------------------------------------------------------------------------------------------------------------

ਸਥਿਰ
M3_ਨਤੀਜਾ Compile_Const_i32 (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_ਨਤੀਜਾ ਨਤੀਜਾ;

    i32 ਮੁੱਲ;
_   (ReadLEB_i32(&ਮੁੱਲ, &o->wasm, o->wasm_ਸਿਰਾ));
_   (ਧੱਕੋ_Const(o, ਮੁੱਲ, c_m3Type_i32));                        m3log (compile, d_indent " (const i32 = %" PRIi32 ")", get_indention_string (o), ਮੁੱਲ);
    _catch: ਵਾਪਸ ਨਤੀਜਾ;
}

ਸਥਿਰ
M3_ਨਤੀਜਾ Compile_Const_i64 (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_ਨਤੀਜਾ ਨਤੀਜਾ;

    i64 ਮੁੱਲ;
_   (ReadLEB_i64(&ਮੁੱਲ, &o->wasm, o->wasm_ਸਿਰਾ));
_   (ਧੱਕੋ_Const(o, ਮੁੱਲ, c_m3Type_i64));                        m3log (compile, d_indent " (const i64 = %" PRIi64 ")", get_indention_string (o), ਮੁੱਲ);
    _catch: ਵਾਪਸ ਨਤੀਜਾ;
}


#ਜੇ d_m3ImplementFloat
ਸਥਿਰ
M3_ਨਤੀਜਾ Compile_Const_f32 (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_ਨਤੀਜਾ ਨਤੀਜਾ;

    ਸੰਘ {
        u32 u;
        f32 f;
    } ਮੁੱਲ = { 0 };

_   (Read_f32(&ਮੁੱਲ.f, &o->wasm, o->wasm_ਸਿਰਾ));                 m3log (compile, d_indent " (const f32 = %" PRIf32 ")", get_indention_string (o), ਮੁੱਲ.f);
_   (ਧੱਕੋ_Const(o, ਮੁੱਲ.u, c_m3Type_f32));

    _catch: ਵਾਪਸ ਨਤੀਜਾ;
}

ਸਥਿਰ
M3_ਨਤੀਜਾ Compile_Const_f64 (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_ਨਤੀਜਾ ਨਤੀਜਾ;

    ਸੰਘ {
        u64 u;
        f64 f;
    } ਮੁੱਲ = { 0 };

_   (Read_f64(&ਮੁੱਲ.f, &o->wasm, o->wasm_ਸਿਰਾ));                 m3log (compile, d_indent " (const f64 = %" PRIf64 ")", get_indention_string (o), ਮੁੱਲ.f);
_   (ਧੱਕੋ_Const(o, ਮੁੱਲ.u, c_m3Type_f64));

    _catch: ਵਾਪਸ ਨਤੀਜਾ;
}
#ਜੇ_ਅੰਤ

ਸਥਿਰ
M3_ਨਤੀਜਾ Compile_ExtendedOpcode (IM3Compilation o, m3opcode_t i_opcode)
{
_try {
    u32 opcode;
_   (ReadLEB_u32(&opcode, &o->wasm, o->wasm_ਸਿਰਾ));               m3log (compile, d_indent " (%s: %" PRIu32 ")", get_indention_string (o), (i_opcode == c_waOp_atomic) ? "FE" : "FC", opcode);

    // m3opcode_t carries only the low byte, so a wider sub-opcode is unknown
    // here rather than something to truncate into range
    _throwif(m3Err_unknownOpcode, opcode > (u32)((i_opcode == c_waOp_atomic) ? c_wa_Op_ਆਖਰੀ_Atomic : c_wa_Op_ਆਖਰੀ_Extended));

    i_opcode = (m3opcode_t)((i_opcode << 8) | opcode);

    //printf("Extended opcode: 0x%x\n", i_opcode);

    IM3OpInfo opInfo = GetOpInfo(i_opcode);
    _throwif(m3Err_unknownOpcode, not opInfo);

    M3Compiler compiler = GetCompiler(opInfo);
    _throwif(m3Err_noCompiler, not compiler);

_   ((*compiler)(o, i_opcode));

    o->ਪਿਛਲਾ_Opcode = i_opcode;

    } _catch: ਵਾਪਸ ਨਤੀਜਾ;
}

ਸਥਿਰ
M3_ਨਤੀਜਾ Compile_Return (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_ਨਤੀਜਾ ਨਤੀਜਾ = m3Err_none;

    ਜੇ (not Is_ਢੇਰ_Polymorphic(o)) {
        IM3CompilationScope functionScope;
_       (Get_ਖੰਡ_Scope(o, &functionScope, o->ਖੰਡ.ਡੂੰਘਾਈ));

_       (Return_ਮੁੱਲਾਂ(o, functionScope, true));

_       (EmitOp(o, op_Return));

_       (ਸੈੱਟ_ਢੇਰ_Polymorphic(o));
    }

    _catch: ਵਾਪਸ ਨਤੀਜਾ;
}

ਸਥਿਰ
M3_ਨਤੀਜਾ Validate_ਖੰਡ_ਸਿਰਾ (IM3Compilation o)
{
    M3_ਨਤੀਜਾ ਨਤੀਜਾ = m3Err_none;

    u16 ਅੰਕ_Results  = Get_Func_Type_ਅੰਕ_Results(o->ਖੰਡ.type);
    u16 ਖੰਡ_ਉਚਾਈ = Get_ਅੰਕ_ਖੰਡ_ਮੁੱਲਾਂ_On_ਢੇਰ(o);

    ਜੇ (not Is_ਢੇਰ_Polymorphic(o)) {
        // Spec: at block end, stack height must match the number of results
        _throwif(m3_Err_type_ਗਿਣਤੀ_Mismatch, ਖੰਡ_ਉਚਾਈ != ਅੰਕ_Results);

        // Spec: result types must match expected types
        ਲਈ (u16 i = 0; i < ਅੰਕ_Results; ++i) {
            m3type_t expectedType = Get_Func_Type_ਨਤੀਜਾ_Type(o->ਖੰਡ.type, ਅੰਕ_Results - 1 - i);
            m3type_t actualType   = Get_ਢੇਰ_Type_From_ਸਿਖਰ(o, i);
            _throwif(m3Err_typeMismatch, not Is_Sub_Type_ਦਾ(actualType, expectedType));
        }
    }

    _catch: ਵਾਪਸ ਨਤੀਜਾ;
}

ਸਥਿਰ
M3_ਨਤੀਜਾ Compile_ਸਿਰਾ (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_ਨਤੀਜਾ ਨਤੀਜਾ = m3Err_none;                   //dump_type_stack (o);

    // function end:
    ਜੇ (o->ਖੰਡ.ਡੂੰਘਾਈ == 0) {
        Validate_ਖੰਡ_ਸਿਰਾ(o);

        //      if (not IsStackPolymorphic (o))
        {
            // A constant expression has no function frame, but its value still
            // has to be copied down to slot 0, where EvaluateExpression reads it
            // back. The non-emitting init-expr walk is skipped: it has no block
            // type to return against.
            ਜੇ (o->function or o->ਪੰਨਾ) {
_               (Return_ਮੁੱਲਾਂ(o, &o->ਖੰਡ, false));
            }

_           (EmitOp(o, op_Return));
        }
    }

    _catch: ਵਾਪਸ ਨਤੀਜਾ;
}


ਸਥਿਰ
M3_ਨਤੀਜਾ Compile_ਸੈੱਟ_Local (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_ਨਤੀਜਾ ਨਤੀਜਾ;

    u32 local_ਸੂਚਕਾਂਕ;
_   (ReadLEB_u32(&local_ਸੂਚਕਾਂਕ, &o->wasm, o->wasm_ਸਿਰਾ));                           m3log (compile, d_indent " (index = %u)", get_indention_string (o), local_ਸੂਚਕਾਂਕ);

    ਜੇ (local_ਸੂਚਕਾਂਕ < Get_Function_ਅੰਕ_ਦਲੀਲਾਂ_And_Locals(o->function)) {
        // Spec: value type must match local type
        ਜੇ (not Is_ਢੇਰ_Polymorphic(o)) {
            m3type_t localType    = Get_ਢੇਰ_Type_From_Bottom(o, local_ਸੂਚਕਾਂਕ);
            m3type_t ਢੇਰ_ਸਿਖਰ_Type = Get_ਢੇਰ_ਸਿਖਰ_Type(o);
            _throwif(m3Err_typeMismatch, ਢੇਰ_ਸਿਖਰ_Type != c_m3Type_none and localType != c_m3Type_none and
                                           not Is_Sub_Type_ਦਾ(ਢੇਰ_ਸਿਖਰ_Type, localType));
#ਜੇ d_m3HasSnapshots
            ਜੇ (o->local_Init_ਡੂੰਘਾਈ and o->local_Init_ਡੂੰਘਾਈ[local_ਸੂਚਕਾਂਕ] == UINT32_MAX) {
                o->local_Init_ਡੂੰਘਾਈ[local_ਸੂਚਕਾਂਕ] = (u32)o->ਖੰਡ.ਡੂੰਘਾਈ;
            }
#ਜੇ_ਅੰਤ
        }

        u16 local_ਥਾਂ = Get_ਥਾਂ_For_ਢੇਰ_ਸੂਚਕਾਂਕ(o, local_ਸੂਚਕਾਂਕ);

        u16 preserve_ਥਾਂ;
_       (ਲੱਭੋ_Referenced_Local_Within_Current_ਖੰਡ(o, &preserve_ਥਾਂ, local_ਥਾਂ));  // preserve will be different than local, if referenced

        bool folded = false;

#ਜੇ d_m3_Fold_ਸੈੱਟ_Local
        // destination folding: when the value was produced by the immediately preceding op (nothing
        // emitted since, result on top in _r0, no copy-on-write preservation needed), retro-patch the
        // producer to a variant that writes straight to the local's slot and skip the SetSlot op
        ਜੇ (o->foldPatchPC and preserve_ਥਾਂ == local_ਥਾਂ and Is_ਢੇਰ_ਸਿਖਰ_ਵਿੱਚ_ਰਜਿਸਟਰ(o) and Get_ਢੇਰ_ਸਿਖਰ_ਸੂਚਕਾਂਕ(o) == (i16)o->fold_ਢੇਰ_ਸੂਚਕਾਂਕ) {
            IM3Operation foldOp = GetFoldOp(o->foldOpcode, o->foldForm);

            ਜੇ (foldOp) {
                *(IM3Operation*)o->foldPatchPC = foldOp;     // retro-patch the producer
                Emit_ਥਾਂ_ਵਿੱਥ(o, local_ਥਾਂ);                  // append its destination
                InvalidateFold(o);
                folded = true;
            }
        }
#ਜੇ_ਅੰਤ

        ਜੇ (folded) {
            m3type_t type = Get_ਢੇਰ_ਸਿਖਰ_Type(o);
_           (ਕੱਢੋ(o));                              // the value now lives in the local's slot

            // A folded op writes its result through to the register as well, so a tee
            // can keep it there. Pushing the slot instead would make the next use that
            // wants a register reload the slot this op just stored to.
            ਜੇ (i_opcode == c_waOp_teeLocal) {
_               (ਧੱਕੋ_ਰਜਿਸਟਰ(o, type));
            }
        } ਨਹੀਂਤਾਂ {
            ਜੇ (preserve_ਥਾਂ == local_ਥਾਂ) {
_               (Copy_ਢੇਰ_ਸਿਖਰ_ਮੰਜ਼ਿਲ_ਥਾਂ(o, local_ਥਾਂ))
            } ਨਹੀਂਤਾਂ {
_               (Preserved_Copy_ਸਿਖਰ_ਥਾਂ(o, local_ਥਾਂ, preserve_ਥਾਂ))
            }

            ਜੇ (i_opcode != c_waOp_teeLocal) {
_               (ਕੱਢੋ(o));
            }
        }
    } ਨਹੀਂਤਾਂ {
        _throw("local index out of bounds");
    }

    _catch: ਵਾਪਸ ਨਤੀਜਾ;
}

ਸਥਿਰ
M3_ਨਤੀਜਾ Compile_GetLocal (IM3Compilation o, m3opcode_t i_opcode)
{
_try {

    u32 local_ਸੂਚਕਾਂਕ;
_   (ReadLEB_u32(&local_ਸੂਚਕਾਂਕ, &o->wasm, o->wasm_ਸਿਰਾ));                           m3log (compile, d_indent " (index = %u)", get_indention_string (o), local_ਸੂਚਕਾਂਕ);

    ਜੇ (local_ਸੂਚਕਾਂਕ >= Get_Function_ਅੰਕ_ਦਲੀਲਾਂ_And_Locals(o->function)) {
        _throw("local index out of bounds");
    }

    m3type_t type = Get_ਢੇਰ_Type_From_Bottom(o, local_ਸੂਚਕਾਂਕ);
    u16      ਥਾਂ = Get_ਥਾਂ_For_ਢੇਰ_ਸੂਚਕਾਂਕ(o, local_ਸੂਚਕਾਂਕ);

_   (ਧੱਕੋ(o, type, ਥਾਂ));

    } _catch: ਵਾਪਸ ਨਤੀਜਾ;
}

ਸਥਿਰ
M3_ਨਤੀਜਾ Compile_GetGlobal (IM3Compilation o, M3Global* i_global)
{
    M3_ਨਤੀਜਾ ਨਤੀਜਾ;

    IM3Operation op = Is64_ਦੋਆਧਾਰੀ_ਅੰਕ_Type(i_global->type) ? op_GetGlobal_s64 : op_GetGlobal_s32;
_   (EmitOp(o, op));
    Emit_ਸੂਚਕ(o, &i_global->i64_ਮੁੱਲ);
_   (ਧੱਕੋ_Allocated_ਥਾਂ_And_Emit_2(o, i_global->type));

    _catch: ਵਾਪਸ ਨਤੀਜਾ;
}

ਸਥਿਰ
M3_ਨਤੀਜਾ Compile_ਸੈੱਟ_Global (IM3Compilation o, M3Global* i_global)
{
    M3_ਨਤੀਜਾ ਨਤੀਜਾ = m3Err_none;

    ਜੇ (i_global->isMutable) {
        // Spec: value type must match global type
        ਜੇ (not Is_ਢੇਰ_Polymorphic(o)) {
            m3type_t ਢੇਰ_ਸਿਖਰ_Type = Get_ਢੇਰ_ਸਿਖਰ_Type(o);
            _throwif(m3Err_typeMismatch, ਢੇਰ_ਸਿਖਰ_Type != c_m3Type_none and
                                           not Is_Sub_Type_ਦਾ(ਢੇਰ_ਸਿਖਰ_Type, i_global->type));
        }

        IM3Operation op;
        m3type_t     type = Get_ਢੇਰ_ਸਿਖਰ_Type(o);

        ਜੇ (Is_ਢੇਰ_ਸਿਖਰ_ਵਿੱਚ_ਰਜਿਸਟਰ(o)) {
            op = c_ਸੈੱਟ_Global_Ops[ਅਧਾਰ_Type_ਦਾ(type)];
        } ਨਹੀਂਤਾਂ {
            op = Is64_ਦੋਆਧਾਰੀ_ਅੰਕ_Type(type) ? op_ਸੈੱਟ_Global_s64 : op_ਸੈੱਟ_Global_s32;
        }

_       (EmitOp(o, op));
        Emit_ਸੂਚਕ(o, &i_global->i64_ਮੁੱਲ);

        ਜੇ (Is_ਢੇਰ_ਸਿਖਰ_ਵਿੱਚ_ਥਾਂ(o)) {
            Emit_ਥਾਂ_ਵਿੱਥ(o, Get_ਢੇਰ_ਸਿਖਰ_ਥਾਂ_ਸੰਖਿਆ(o));
        }

_       (ਕੱਢੋ(o));
    } ਨਹੀਂਤਾਂ {
        _throw(m3Err_settingImmutableGlobal);
    }

    _catch: ਵਾਪਸ ਨਤੀਜਾ;
}

ਸਥਿਰ
M3_ਨਤੀਜਾ Compile_Get_ਸੈੱਟ_Global (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_ਨਤੀਜਾ ਨਤੀਜਾ = m3Err_none;

    u32 global_ਸੂਚਕਾਂਕ;
_   (ReadLEB_u32(&global_ਸੂਚਕਾਂਕ, &o->wasm, o->wasm_ਸਿਰਾ));

    ਜੇ (global_ਸੂਚਕਾਂਕ < o->module->ਅੰਕ_Globals) {
        ਜੇ (o->module->globals) {
            M3Global* global = &o->module->globals[global_ਸੂਚਕਾਂਕ];

            // Spec: a constant expression may only read an imported immutable
            // global. The module's own globals are counted before their
            // initializer is walked, so a bare index check would let one
            // reference itself. These describe how the module declared the
            // global, so they are checked before following any link.
            _throwif(m3_Err_globa_ਸੂਚਕਾਂਕ_ਬਾਹਰਲਾ_ਦਾ_Bounds, o->is_Init_ਵਿਅੰਜਕ and not global->imported);
            _throwif(m3Err_wasmMalformed, o->is_Init_ਵਿਅੰਜਕ and global->isMutable);

            // an import linked to another module reads and writes that module's
            // cell, not the placeholder standing in for it here
            ਜੇ (global->resolved) {
                global = global->resolved;
            }

_           ((i_opcode == c_waOp_getGlobal) ? Compile_GetGlobal(o, global) : Compile_ਸੈੱਟ_Global(o, global));
        } ਨਹੀਂਤਾਂ {
            _throw(ErrorCompile(m3_Err_global_ਸਮ੍ਰਿਤੀ_Not_Allocated, o, "module '%s' is missing global memory", o->module->name));
        }
    } ਨਹੀਂਤਾਂ {
        _throw(m3_Err_globa_ਸੂਚਕਾਂਕ_ਬਾਹਰਲਾ_ਦਾ_Bounds);
    }

    _catch: ਵਾਪਸ ਨਤੀਜਾ;
}

ਸਥਿਰ
ਖਾਲੀ Emit_Patching_Branch_ਸੂਚਕ (IM3Compilation o, IM3CompilationScope i_scope)
{
    pc_t patch       = Emit_ਸੂਚਕ(o, i_scope->patches);                m3log (compile, "branch patch required at: %p", patch);
    i_scope->patches = patch;
}

ਸਥਿਰ
M3_ਨਤੀਜਾ EmitPatchingBranch (IM3Compilation o, IM3CompilationScope i_scope)
{
    M3_ਨਤੀਜਾ ਨਤੀਜਾ = m3Err_none;

_   (EmitOp(o, op_Branch));
    Emit_Patching_Branch_ਸੂਚਕ(o, i_scope);

    _catch: ਵਾਪਸ ਨਤੀਜਾ;
}

#ਜੇ d_m3HasExceptionHandling

// How many try_table handler records an exit from i_from out to i_target takes
// down with it. Walking outward from the block the branch sits in, every
// try_table scope up to and including the target is left behind - the target
// too, because landing on a block's label means that block is finished.
//
// A loop label is the exception, but a loop is never a try_table, so the count
// is the same either way: the loop itself is not on the list.
ਸਥਿਰ
u32 ਗਿਣਤੀ_Try_ਚੌਖਟੇ_ਮੰਜ਼ਿਲ_Exit (IM3CompilationScope i_from, IM3CompilationScope i_ਨਿਸ਼ਾਨਾ)
{
    u32 ਗਿਣਤੀ_2 = 0;

    ਲਈ (IM3CompilationScope scope = i_from; scope; scope = scope->outer) {
        ਜੇ (scope->opcode == c_wa_Op_try_ਸਾਰਣੀ) {
            ++ਗਿਣਤੀ_2;
        }

        ਜੇ (scope == i_ਨਿਸ਼ਾਨਾ) {
            ਰੋਕੋ;
        }
    }

    ਵਾਪਸ ਗਿਣਤੀ_2;
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
ਸਥਿਰ
u32 ਅੰਕ_Try_ਚੌਖਟੇ_ਮੰਜ਼ਿਲ_ਕੱਢੋ (IM3CompilationScope i_from, IM3CompilationScope i_ਨਿਸ਼ਾਨਾ)
{
    ਜੇ (i_ਨਿਸ਼ਾਨਾ->ਡੂੰਘਾਈ == 0) {
        ਵਾਪਸ 0;
    }

    // the walk only ever runs inward-to-outward; a target that is already
    // outside i_from has nothing between them
    ਜੇ (not i_from or i_from->ਡੂੰਘਾਈ < i_ਨਿਸ਼ਾਨਾ->ਡੂੰਘਾਈ) {
        ਵਾਪਸ 0;
    }

    ਵਾਪਸ ਗਿਣਤੀ_Try_ਚੌਖਟੇ_ਮੰਜ਼ਿਲ_Exit(i_from, i_ਨਿਸ਼ਾਨਾ);
}


ਸਥਿਰ
M3_ਨਤੀਜਾ Emit_ਕੱਢੋ_Try_ਚੌਖਟੇ (IM3Compilation o, u32 i_ਅੰਕ_ਚੌਖਟੇ)
{
    M3_ਨਤੀਜਾ ਨਤੀਜਾ = m3Err_none;

    ਜੇ (i_ਅੰਕ_ਚੌਖਟੇ) {
_       (EmitOp(o, op_ਕੱਢੋ_Handlers));
        EmitConstant32(o, i_ਅੰਕ_ਚੌਖਟੇ);
    }

    _catch: ਵਾਪਸ ਨਤੀਜਾ;
}


// A return_call leaves the enclosing function before the callee runs, so no
// try_table in this function may catch what the callee throws. Neither shape of
// the instruction unwinds the native stack in time to arrange that on its own:
// op_ReturnCall tail-jumps with op_TryTable's frames still standing, and the
// fallback shape is an ordinary call sitting inside them. Either way the
// handler records have to come off first.
ਸਥਿਰ
M3_ਨਤੀਜਾ Emit_ਕੱਢੋ_Try_ਚੌਖਟੇ_For_Return_Call (IM3Compilation o)
{
    u32 ਗਿਣਤੀ_2 = 0;

    ਲਈ (IM3CompilationScope scope = &o->ਖੰਡ; scope; scope = scope->outer) {
        ਜੇ (scope->opcode == c_wa_Op_try_ਸਾਰਣੀ) {
            ++ਗਿਣਤੀ_2;
        }
    }

    ਵਾਪਸ Emit_ਕੱਢੋ_Try_ਚੌਖਟੇ(o, ਗਿਣਤੀ_2);
}

#ਨਹੀਂਤਾਂ

ਸਥਿਰ
M3_ਨਤੀਜਾ Emit_ਕੱਢੋ_Try_ਚੌਖਟੇ (IM3Compilation o, u32 i_ਅੰਕ_ਚੌਖਟੇ)
{
    (ਖਾਲੀ)o;
    (ਖਾਲੀ)i_ਅੰਕ_ਚੌਖਟੇ;
    ਵਾਪਸ m3Err_none;
}

#ਜੇ_ਅੰਤ // d_m3HasExceptionHandling

#ਜੇ d_m3FuseBranch
// when the pending fold candidate is a compare whose result sits on top in _r0, return the fused
// variant that absorbs an immediately following br_if (i_isIf false) or if (i_isIf true)
ਸਥਿਰ
IM3Operation GetBranchFusionOp (IM3Compilation o, bool i_isIf)
{
    ਜੇ (o->foldPatchPC and o->foldForm < 4 and Is_ਢੇਰ_ਸਿਖਰ_ਵਿੱਚ_ਰਜਿਸਟਰ(o) and Get_ਢੇਰ_ਸਿਖਰ_ਸੂਚਕਾਂਕ(o) == (i16)o->fold_ਢੇਰ_ਸੂਚਕਾਂਕ) {
        ਅਚਲ M3FusedCmpOps* ops = GetFusedCmpOps(o->foldOpcode);

        ਜੇ (ops) {
            ਵਾਪਸ i_isIf ? ops->ifOp[o->foldForm] : ops->branchIf[o->foldForm];
        }
    }

    ਵਾਪਸ ਨਿਰਾ;
}
#ਜੇ_ਅੰਤ // d_m3FuseBranch

ਸਥਿਰ ਸਤਰਅੰਦਰ
IM3Operation Op_ContinueLoop (IM3Compilation o)
{
#ਜੇ d_m3_Has_ਢੇਰ_Switching
    ਜੇ (M3_UNLIKELY(o->module->runtime and o->module->runtime->isSuspendable)) {
        ਵਾਪਸ op_ContinueLoop_Suspendable;
    }
#ਜੇ_ਅੰਤ
    ਵਾਪਸ op_ContinueLoop;
}

ਸਥਿਰ ਸਤਰਅੰਦਰ
IM3Operation Op_ContinueLoopIf (IM3Compilation o)
{
#ਜੇ d_m3_Has_ਢੇਰ_Switching
    ਜੇ (M3_UNLIKELY(o->module->runtime and o->module->runtime->isSuspendable)) {
        ਵਾਪਸ op_ContinueLoopIf_Suspendable;
    }
#ਜੇ_ਅੰਤ
    ਵਾਪਸ op_ContinueLoopIf;
}


ਸਥਿਰ
M3_ਨਤੀਜਾ Compile_Branch (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_ਨਤੀਜਾ ਨਤੀਜਾ;
    u32      ਅੰਕ_Try_ਚੌਖਟੇ = 0;

    u32 ਡੂੰਘਾਈ;
_   (ReadLEB_u32(&ਡੂੰਘਾਈ, &o->wasm, o->wasm_ਸਿਰਾ));

    // Spec: br_if condition must be i32
    ਜੇ (i_opcode == c_waOp_branchIf and not Is_ਢੇਰ_Polymorphic(o)) {
        m3type_t condType = Get_ਢੇਰ_ਸਿਖਰ_Type(o);
        _throwif(m3Err_typeMismatch, condType != c_m3Type_none and condType != c_m3Type_i32);
    }

    IM3CompilationScope scope;
_   (Get_ਖੰਡ_Scope(o, &scope, ਡੂੰਘਾਈ));

#ਜੇ d_m3HasExceptionHandling
    ਅੰਕ_Try_ਚੌਖਟੇ = ਅੰਕ_Try_ਚੌਖਟੇ_ਮੰਜ਼ਿਲ_ਕੱਢੋ(&o->ਖੰਡ, scope);
#ਜੇ_ਅੰਤ

    // branch target is a loop (continue)
    ਜੇ (scope->opcode == c_waOp_loop) {
        ਜੇ (i_opcode == c_waOp_branchIf) {
            // a handler pop belongs on the taken path only, so it forces the
            // jump-over form even where the target takes no parameters
            ਜੇ (Get_Func_Type_ਅੰਕ_Params(scope->type) or ਅੰਕ_Try_ਚੌਖਟੇ) {
                IM3Operation op = Is_ਢੇਰ_ਸਿਖਰ_ਵਿੱਚ_ਰਜਿਸਟਰ(o) ? op_BranchIfPrologue_r : op_BranchIfPrologue_s;

_               (EmitOp(o, op));
_               (Emit_ਥਾਂ_ਅੰਕ_ਦਾ_ਢੇਰ_ਸਿਖਰ_And_ਕੱਢੋ(o));

                pc_t* jump_ਮੰਜ਼ਿਲ = (pc_t*)Reserve_ਸੂਚਕ(o);

_               (Emit_ਕੱਢੋ_Try_ਚੌਖਟੇ(o, ਅੰਕ_Try_ਚੌਖਟੇ));

                ਜੇ (not Is_ਢੇਰ_Polymorphic(o)) {
_                   (Resolve_ਖੰਡ_Results(o, scope, /* isBranch: */ true));
                }

_               (EmitOp(o, Op_ContinueLoop(o)));
                Emit_ਸੂਚਕ(o, scope->pc);
_               (ਰਿਕਾਰਡ_ਪਿੱਠ_ਕਿਨਾਰਾ(o, scope, false));

                *jump_ਮੰਜ਼ਿਲ = GetPC(o);
            } ਨਹੀਂਤਾਂ {
                // move the condition to a register
_               (Copy_ਢੇਰ_ਸਿਖਰ_ਮੰਜ਼ਿਲ_ਰਜਿਸਟਰ(o, false));
_               (ਕੱਢੋ_Type(o, c_m3Type_i32));

_               (EmitOp(o, Op_ContinueLoopIf(o)));
                Emit_ਸੂਚਕ(o, scope->pc);
_               (ਰਿਕਾਰਡ_ਪਿੱਠ_ਕਿਨਾਰਾ(o, scope, true));
            }

            //          dump_type_stack(o);
        } ਨਹੀਂਤਾਂ // is c_waOp_branch
        {
_           (Emit_ਕੱਢੋ_Try_ਚੌਖਟੇ(o, ਅੰਕ_Try_ਚੌਖਟੇ));

            // the branch operands become the loop's parameters on the next iteration
            ਜੇ (not Is_ਢੇਰ_Polymorphic(o)) {
_               (Resolve_ਖੰਡ_Results(o, scope, /* isBranch: */ true));
            }

_           (EmitOp(o, Op_ContinueLoop(o)));
            Emit_ਸੂਚਕ(o, scope->pc);
_           (ਰਿਕਾਰਡ_ਪਿੱਠ_ਕਿਨਾਰਾ(o, scope, false));
            o->ਖੰਡ.isPolymorphic = true;
        }
    } ਨਹੀਂਤਾਂ // forward branch
    {
        pc_t* jump_ਮੰਜ਼ਿਲ = ਨਿਰਾ;

        bool isReturn         = (scope->ਡੂੰਘਾਈ == 0);
        bool ਨਿਸ਼ਾਨਾ_Has_Results = Get_Func_Type_ਅੰਕ_Results(scope->type);

        ਜੇ (i_opcode == c_waOp_branchIf) {
            ਜੇ (ਨਿਸ਼ਾਨਾ_Has_Results or isReturn or ਅੰਕ_Try_ਚੌਖਟੇ) {
                IM3Operation op = Is_ਢੇਰ_ਸਿਖਰ_ਵਿੱਚ_ਰਜਿਸਟਰ(o) ? op_BranchIfPrologue_r : op_BranchIfPrologue_s;

_               (EmitOp(o, op));
_               (Emit_ਥਾਂ_ਅੰਕ_ਦਾ_ਢੇਰ_ਸਿਖਰ_And_ਕੱਢੋ(o)); // condition

                // this is continuation point, if the branch isn't taken
                jump_ਮੰਜ਼ਿਲ = (pc_t*)Reserve_ਸੂਚਕ(o);
            } ਨਹੀਂਤਾਂ {
#ਜੇ d_m3FuseBranch
                IM3Operation fuseOp = GetBranchFusionOp(o, false);

                ਜੇ (fuseOp) {
                    // absorb the branch into the compare that produced the condition
                    *(IM3Operation*)o->foldPatchPC = fuseOp;
                    InvalidateFold(o);

_                   (ਕੱਢੋ(o));                      // the condition is consumed inside the fused op

                    Emit_Patching_Branch_ਸੂਚਕ(o, scope);
                    ਜਾਓ _catch;
                }
#ਜੇ_ਅੰਤ
                IM3Operation op = Is_ਢੇਰ_ਸਿਖਰ_ਵਿੱਚ_ਰਜਿਸਟਰ(o) ? op_BranchIf_r : op_BranchIf_s;

_               (EmitOp(o, op));
_               (Emit_ਥਾਂ_ਅੰਕ_ਦਾ_ਢੇਰ_ਸਿਖਰ_And_ਕੱਢੋ(o)); // condition

                Emit_Patching_Branch_ਸੂਚਕ(o, scope);
                ਜਾਓ _catch;
            }
        }

        ਜੇ (not Is_ਢੇਰ_Polymorphic(o)) {
            ਜੇ (isReturn) {
_               (Return_ਮੁੱਲਾਂ(o, scope, true));
_               (EmitOp(o, op_Return));
            } ਨਹੀਂਤਾਂ {
_               (Emit_ਕੱਢੋ_Try_ਚੌਖਟੇ(o, ਅੰਕ_Try_ਚੌਖਟੇ));

_               (Resolve_ਖੰਡ_Results(o, scope, true));
_               (EmitPatchingBranch(o, scope));
            }
        }

        ਜੇ (jump_ਮੰਜ਼ਿਲ) {
            *jump_ਮੰਜ਼ਿਲ = GetPC(o);
        }

        ਜੇ (i_opcode == c_waOp_branch) {
_           (ਸੈੱਟ_ਢੇਰ_Polymorphic(o));
        }
    }

    _catch: ਵਾਪਸ ਨਤੀਜਾ;
}

ਸਥਿਰ
M3_ਨਤੀਜਾ Compile_Branch_ਸਾਰਣੀ (IM3Compilation o, m3opcode_t i_opcode)
{
    // the page a continue-op page is standing in for; non-NULL only while that
    // page is installed in o->page, so the catch below knows to put it back
    IM3_ਕੋਡ_ਪੰਨਾ displaced_ਪੰਨਾ = ਨਿਰਾ;
    // indexed by label depth: the continuation stub already emitted for that target
    // within this table, or NULL if this table has not reached it yet
    pc_t* ਨਿਸ਼ਾਨਾ_Stubs = ਨਿਰਾ;
_try {
    u32 ਨਿਸ਼ਾਨਾ_ਗਿਣਤੀ;
_   (ReadLEB_u32(&ਨਿਸ਼ਾਨਾ_ਗਿਣਤੀ, &o->wasm, o->wasm_ਸਿਰਾ));

    // Spec: validate that the branch index operand is i32
    ਜੇ (not Is_ਢੇਰ_Polymorphic(o)) {
        m3type_t ਸੂਚਕਾਂਕ_Type = Get_ਢੇਰ_ਸਿਖਰ_Type(o);
        _throwif(m3Err_typeMismatch, ਸੂਚਕਾਂਕ_Type != c_m3Type_none and ਸੂਚਕਾਂਕ_Type != c_m3Type_i32);
    }

_   (Preserve_ਰਜਿਸਟਰ_If_Occupied(o, c_m3Type_i64));         // move branch operand to a slot
    u16 ਥਾਂ = Get_ਢੇਰ_ਸਿਖਰ_ਥਾਂ_ਸੰਖਿਆ(o);
_   (ਕੱਢੋ(o));

    // OPTZ: according to spec: "forward branches that target a control instruction with a non-empty
    // result type consume matching operands first and push them back on the operand stack after unwinding"
    // So, this move-to-reg is only necessary if the target scopes have a type.

    u32 ਅੰਕ_ਕੋਡ_ਸਤਰਾਂ = ਨਿਸ਼ਾਨਾ_ਗਿਣਤੀ + 4; // 3 => IM3Operation + slot + target_count + default_target
_   (Ensure_ਕੋਡ_ਪੰਨਾ_ਅੰਕ_ਸਤਰਾਂ(o, ਅੰਕ_ਕੋਡ_ਸਤਰਾਂ));

_   (EmitOp(o, op_Branch_ਸਾਰਣੀ));
    Emit_ਥਾਂ_ਵਿੱਥ(o, ਥਾਂ);
    EmitConstant32(o, ਨਿਸ਼ਾਨਾ_ਗਿਣਤੀ);

    IM3_ਕੋਡ_ਪੰਨਾ continue_Op_ਪੰਨਾ = ਨਿਰਾ;

    // The label types are checked by ValidateFunction, which runs under the same
    // d_m3EnableValidation guard just before this compilation pass. Comparing each
    // target against the default here would be wrong anyway: the spec matches every
    // target against the operand stack, so in unreachable code the operands are the
    // bottom type and the targets may legitimately differ (unreached-valid.wast).

    ++ਨਿਸ਼ਾਨਾ_ਗਿਣਤੀ; // include default

    // Every entry of one br_table is resolved from the same compilation state - each
    // stub restores whatever it touched - so two entries naming the same label emit
    // byte-identical epilogues. Emit one and point both table entries at it. A label
    // deeper than the current block is rejected by GetBlockScope below, which is what
    // bounds this array.
    ਨਿਸ਼ਾਨਾ_Stubs = m3_AllocArray(pc_t, (size_t)o->ਖੰਡ.ਡੂੰਘਾਈ + 1);
    _throwifnull(ਨਿਸ਼ਾਨਾ_Stubs);

    ਲਈ (u32 i = 0; i < ਨਿਸ਼ਾਨਾ_ਗਿਣਤੀ; ++i) {
        u32 ਨਿਸ਼ਾਨਾ;
_       (ReadLEB_u32(&ਨਿਸ਼ਾਨਾ, &o->wasm, o->wasm_ਸਿਰਾ));

        IM3CompilationScope scope;
_       (Get_ਖੰਡ_Scope(o, &scope, ਨਿਸ਼ਾਨਾ));

        ਜੇ (ਨਿਸ਼ਾਨਾ_Stubs[ਨਿਸ਼ਾਨਾ]) {
            Emit_ਸੂਚਕ(o, ਨਿਸ਼ਾਨਾ_Stubs[ਨਿਸ਼ਾਨਾ]);
            ਜਾਰੀ;
        }

        u32 ਅੰਕ_Try_ਚੌਖਟੇ = 0;
#ਜੇ d_m3HasExceptionHandling
        ਅੰਕ_Try_ਚੌਖਟੇ = ਅੰਕ_Try_ਚੌਖਟੇ_ਮੰਜ਼ਿਲ_ਕੱਢੋ(&o->ਖੰਡ, scope);
#ਜੇ_ਅੰਤ

        // A forward branch to a block that carries no results and leaves no try
        // frames behind has nothing to do on the way out but arrive. There is no
        // epilogue to hold, so the table entry becomes the patch site itself and the
        // block's address is written into it when the block ends - no stub, and no
        // code page to put one on. In unreachable code the same is true whatever the
        // block's type, since nothing is emitted for that case either way.
        //
        // Each such entry is patched on its own, so none is recorded in targetStubs:
        // there is no shared stub for a later entry naming this label to point at.
        ਜੇ (scope->opcode != c_waOp_loop and scope->ਡੂੰਘਾਈ > 0 and ਅੰਕ_Try_ਚੌਖਟੇ == 0 and
            (Is_ਢੇਰ_Polymorphic(o) or Get_Func_Type_ਅੰਕ_Results(scope->type) == 0)) {
            Emit_Patching_Branch_ਸੂਚਕ(o, scope);
            ਜਾਰੀ;
        }

_       (Acquire_Compilation_ਕੋਡ_ਪੰਨਾ(o, &continue_Op_ਪੰਨਾ));

        pc_t ਸ਼ੁਰੂ_PC  = Get_ਪੰਨਾ_PC(continue_Op_ਪੰਨਾ);
        displaced_ਪੰਨਾ = o->ਪੰਨਾ;
        o->ਪੰਨਾ       = continue_Op_ਪੰਨਾ;

        ਜੇ (scope->opcode == c_waOp_loop) {
_           (Emit_ਕੱਢੋ_Try_ਚੌਖਟੇ(o, ਅੰਕ_Try_ਚੌਖਟੇ));

            ਜੇ (not Is_ਢੇਰ_Polymorphic(o)) {
_               (Resolve_ਖੰਡ_Results(o, scope, true));
            }

_           (EmitOp(o, Op_ContinueLoop(o)));
            Emit_ਸੂਚਕ(o, scope->pc);
_           (ਰਿਕਾਰਡ_ਪਿੱਠ_ਕਿਨਾਰਾ(o, scope, false));
        } ਨਹੀਂਤਾਂ {
            ਜੇ (not Is_ਢੇਰ_Polymorphic(o)) {
                ਜੇ (scope->ਡੂੰਘਾਈ == 0) {
_                   (Return_ਮੁੱਲਾਂ(o, scope, true));
_                   (EmitOp(o, op_Return));
                } ਨਹੀਂਤਾਂ {
_                   (Emit_ਕੱਢੋ_Try_ਚੌਖਟੇ(o, ਅੰਕ_Try_ਚੌਖਟੇ));

_                   (Resolve_ਖੰਡ_Results(o, scope, true));

_                   (EmitPatchingBranch(o, scope));
                }
            }
        }

        IM3_ਕੋਡ_ਪੰਨਾ return_ਮੰਜ਼ਿਲ = displaced_ਪੰਨਾ;
        displaced_ਪੰਨਾ        = ਨਿਰਾ;

        Release_Compilation_ਕੋਡ_ਪੰਨਾ(o);
        o->ਪੰਨਾ = return_ਮੰਜ਼ਿਲ;

        Emit_ਸੂਚਕ(o, ਸ਼ੁਰੂ_PC);
        ਨਿਸ਼ਾਨਾ_Stubs[ਨਿਸ਼ਾਨਾ] = ਸ਼ੁਰੂ_PC;
    }

_   (ਸੈੱਟ_ਢੇਰ_Polymorphic(o));
} _catch:

    // thrown out of the loop above with a continue-op page installed: release it
    // and restore the one it displaced, which the caller's catch then releases.
    // Otherwise the displaced page is on no list and simply leaks.
    ਜੇ (displaced_ਪੰਨਾ) {
        Release_Compilation_ਕੋਡ_ਪੰਨਾ(o);
        o->ਪੰਨਾ = displaced_ਪੰਨਾ;
    }

    m3_Free(ਨਿਸ਼ਾਨਾ_Stubs);

    ਵਾਪਸ ਨਤੀਜਾ;
}

ਸਥਿਰ
M3_ਨਤੀਜਾ Compile_Call_ਦਲੀਲਾਂ_And_Return (IM3Compilation o, u16* o_ਢੇਰ_ਵਿੱਥ, IM3FuncType i_type, bool i_is_ਅਸਿੱਧਾ)
{
_try {

    u16 ਸਿਖਰ_ਥਾਂ = Get_Max_ਵਰਤਿਆ_ਥਾਂ_Plus_One(o);

    // force use of at least one stack slot; this is to help ensure
    // the m3 stack overflows (and traps) before the native stack can overflow.
    // e.g. see Wasm spec test 'runaway' in call.wast
    ਸਿਖਰ_ਥਾਂ = M3_MAX(1, ਸਿਖਰ_ਥਾਂ);

    // stack frame is 64-bit aligned
    Align_ਥਾਂ_ਮੰਜ਼ਿਲ_Type(&ਸਿਖਰ_ਥਾਂ, c_m3Type_i64);

    *o_ਢੇਰ_ਵਿੱਥ = ਸਿਖਰ_ਥਾਂ;

    // wait to pop this here so that topSlot search is correct
    ਜੇ (i_is_ਅਸਿੱਧਾ) {
_       (ਕੱਢੋ(o));
    }

    u16 ਅੰਕ_ਦਲੀਲਾਂ = Get_Func_Type_ਅੰਕ_Params(i_type);
    u16 ਅੰਕ_Rets = Get_Func_Type_ਅੰਕ_Results(i_type);

    u16 arg_ਸਿਖਰ = ਸਿਖਰ_ਥਾਂ + (ਅੰਕ_ਦਲੀਲਾਂ + ਅੰਕ_Rets) * c_io_ਥਾਂ_ਗਿਣਤੀ;

    Touch_ਥਾਂ(o, arg_ਸਿਖਰ - 1);

    ਜਦੋਂ (ਅੰਕ_ਦਲੀਲਾਂ--) {
_       (Copy_ਢੇਰ_ਸਿਖਰ_ਮੰਜ਼ਿਲ_ਥਾਂ(o, arg_ਸਿਖਰ -= c_io_ਥਾਂ_ਗਿਣਤੀ));
_       (ਕੱਢੋ(o));
    }

    u16 i = 0;
    ਜਦੋਂ (ਅੰਕ_Rets--) {
        m3type_t type = Get_Func_Type_ਨਤੀਜਾ_Type(i_type, i++);

_       (ਧੱਕੋ(o, type, ਸਿਖਰ_ਥਾਂ));
_       (MarkSlotsAllocatedByType(o, ਸਿਖਰ_ਥਾਂ, type));

        ਸਿਖਰ_ਥਾਂ += c_io_ਥਾਂ_ਗਿਣਤੀ;
    }

    } _catch: ਵਾਪਸ ਨਤੀਜਾ;
}

// A tail call hands the current frame over to the callee instead of stacking a new one.
// The callee's results have to be the enclosing function's results, so both use the same
// return slots and only the arguments move: they're staged above the caller's stack here
// (the argument expressions can still be reading the args/locals they're about to land on)
// and slid down onto the frame base at runtime, by op_ReturnCall[Indirect].
ਸਥਿਰ
M3_ਨਤੀਜਾ Compile_ਪੂਛ_Call_ਦਲੀਲਾਂ (IM3Compilation o, u16* o_ਢੇਰ_ਵਿੱਥ, u16* o_ਅੰਕ_Arg_Slots, IM3FuncType i_type, bool i_is_ਅਸਿੱਧਾ)
{
_try {
    IM3FuncType funcType = o->function->funcType;

    u16 ਅੰਕ_Rets = Get_Func_Type_ਅੰਕ_Results(i_type);
    _throwif(m3Err_typeMismatch, ਅੰਕ_Rets != Get_Func_Type_ਅੰਕ_Results(funcType));

    ਲਈ (u16 i = 0; i < ਅੰਕ_Rets; ++i) {
        _throwif(m3Err_typeMismatch, Get_Func_Type_ਨਤੀਜਾ_Type(i_type, i) != Get_Func_Type_ਨਤੀਜਾ_Type(funcType, i));
    }

    u16 ਸਿਖਰ_ਥਾਂ = Get_Max_ਵਰਤਿਆ_ਥਾਂ_Plus_One(o);

    ਸਿਖਰ_ਥਾਂ = M3_MAX(1, ਸਿਖਰ_ਥਾਂ);

    // stack frame is 64-bit aligned
    Align_ਥਾਂ_ਮੰਜ਼ਿਲ_Type(&ਸਿਖਰ_ਥਾਂ, c_m3Type_i64);

    *o_ਢੇਰ_ਵਿੱਥ = ਸਿਖਰ_ਥਾਂ;

    // wait to pop this here so that topSlot search is correct
    ਜੇ (i_is_ਅਸਿੱਧਾ) {
_       (ਕੱਢੋ(o));
    }

    u16 ਅੰਕ_ਦਲੀਲਾਂ     = Get_Func_Type_ਅੰਕ_Params(i_type);
    u16 ਅੰਕ_Arg_Slots = ਅੰਕ_ਦਲੀਲਾਂ * c_io_ਥਾਂ_ਗਿਣਤੀ;

    *o_ਅੰਕ_Arg_Slots = ਅੰਕ_Arg_Slots;

    u16 arg_ਸਿਖਰ = ਸਿਖਰ_ਥਾਂ + ਅੰਕ_Arg_Slots;
    _throwif(m3_Err_function_ਢੇਰ_ਉਛਾਲ, arg_ਸਿਖਰ > d_m3MaxFunctionSlots);

    // the staging area is written by the caller, so op_Entry has to account for it
    Touch_ਥਾਂ(o, arg_ਸਿਖਰ - 1);

    ਜਦੋਂ (ਅੰਕ_ਦਲੀਲਾਂ--) {
_       (Copy_ਢੇਰ_ਸਿਖਰ_ਮੰਜ਼ਿਲ_ਥਾਂ(o, arg_ਸਿਖਰ -= c_io_ਥਾਂ_ਗਿਣਤੀ));
_       (ਕੱਢੋ(o));
    }

    } _catch: ਵਾਪਸ ਨਤੀਜਾ;
}


#ਜੇ d_m3HasTypedRefs

// call_ref $t: the callee is the reference on top of the stack, so the operand
// order is the same as call_indirect's, minus the table.
ਸਥਿਰ
M3_ਨਤੀਜਾ Compile_CallRef (IM3Compilation o, m3opcode_t i_opcode)
{
_try {
    u32 type_ਸੂਚਕਾਂਕ;
_   (ReadLEB_u32(&type_ਸੂਚਕਾਂਕ, &o->wasm, o->wasm_ਸਿਰਾ));

    _throwif("function call type index out of range", type_ਸੂਚਕਾਂਕ >= o->module->ਅੰਕ_Func_Types);

    IM3FuncType type = o->module->funcTypes[type_ਸੂਚਕਾਂਕ];

    ਜੇ (not Is_ਢੇਰ_Polymorphic(o)) {
        // whatever shape the reference has, it must be a function of this type
        m3type_t refType = Get_ਢੇਰ_ਸਿਖਰ_Type(o);
        _throwif(m3Err_typeMismatch, not Is_Sub_Type_ਦਾ(refType, Ref_Type_ਦਾ_Func_Type(type, false)));
    }

    ਜੇ (Is_ਢੇਰ_ਸਿਖਰ_ਵਿੱਚ_ਰਜਿਸਟਰ(o)) {
_       (Preserve_ਰਜਿਸਟਰ_If_Occupied(o, c_m3Type_funcref));
    }

    u16 function_ਥਾਂ = Get_ਢੇਰ_ਸਿਖਰ_ਥਾਂ_ਸੰਖਿਆ(o);

    bool isReturnCall = (i_opcode == c_waOp_returnCallRef);
    bool use_ਪੂਛ_Call  = isReturnCall and d_m3_Can_ਪੂਛ_Call;

    u16 exec_ਸਿਖਰ, ਅੰਕ_Arg_Slots = 0;

    ਜੇ (use_ਪੂਛ_Call) {
_       (Compile_ਪੂਛ_Call_ਦਲੀਲਾਂ(o, &exec_ਸਿਖਰ, &ਅੰਕ_Arg_Slots, type, true));
    } ਨਹੀਂਤਾਂ {
_       (Compile_Call_ਦਲੀਲਾਂ_And_Return(o, &exec_ਸਿਖਰ, type, true));
    }

#  ਜੇ d_m3HasExceptionHandling
    ਜੇ (isReturnCall) {
_       (Emit_ਕੱਢੋ_Try_ਚੌਖਟੇ_For_Return_Call(o));
    }
#  endif

_   (EmitOp(o, use_ਪੂਛ_Call ? op_ReturnCallRef : op_CallRef));
    Emit_ਥਾਂ_ਵਿੱਥ(o, function_ਥਾਂ);
    Emit_ਸੂਚਕ(o, type);
    Emit_ਥਾਂ_ਵਿੱਥ(o, exec_ਸਿਖਰ);

    ਜੇ (not use_ਪੂਛ_Call) {
_       (ਰਿਕਾਰਡ_Call(o, Get_Func_Type_ਅੰਕ_Results(type), exec_ਸਿਖਰ, isReturnCall));
    }

    ਜੇ (use_ਪੂਛ_Call) {
        Emit_ਥਾਂ_ਵਿੱਥ(o, o->function->ਅੰਕ_Ret_Slots);
        EmitConstant32(o, ਅੰਕ_Arg_Slots);

_       (ਸੈੱਟ_ਢੇਰ_Polymorphic(o));
    } ਨਹੀਂਤਾਂ ਜੇ (isReturnCall) {
_       (Compile_Return(o, i_opcode));
    }

} _catch:
    ਵਾਪਸ ਨਤੀਜਾ;
}


// ref.as_non_null only sharpens the type; the value is unchanged, so the stack
// entry is re-pushed with the null stripped out of its type.
ਸਥਿਰ
M3_ਨਤੀਜਾ Compile_Ref_AsNonNull (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_ਨਤੀਜਾ ਨਤੀਜਾ = m3Err_none;

    ਜੇ (not Is_ਢੇਰ_Polymorphic(o)) {
        m3type_t type = Get_ਢੇਰ_ਸਿਖਰ_Type(o);
        _throwif(m3Err_typeMismatch, not IsRefType(type));

        // op_RefAsNonNull only knows how to read its operand out of a slot; a
        // value still sitting in the register has to be spilled first, the way
        // every other single-op reader of the stack top does
        ਜੇ (Is_ਢੇਰ_ਸਿਖਰ_ਵਿੱਚ_ਰਜਿਸਟਰ(o)) {
_           (Preserve_ਰਜਿਸਟਰ_If_Occupied(o, type));
        }

        u16 ਥਾਂ = Get_ਢੇਰ_ਸਿਖਰ_ਥਾਂ_ਸੰਖਿਆ(o);

_       (EmitOp(o, op_RefAsNonNull));
        Emit_ਥਾਂ_ਵਿੱਥ(o, ਥਾਂ);

        m3type_t nonNull = IsSpelledRefType(type)
                             ? (m3type_t)(type | d_m3Type_refNonNull)
                             : (m3type_t)(d_m3Type_ref | d_m3Type_refNonNull | d_m3_Type_ਢੇਰ_Abstract |
                                          ((ਅਧਾਰ_Type_ਦਾ(type) == c_m3Type_externref) ? d_m3Type_refExtern : 0));

        // the slot keeps the one reference it already had; a Pop()/Push() of
        // the same slot would drop that reference's count without anything
        // to put it back, so the type above it is corrected in place instead
        o->type_ਢੇਰ[Get_ਢੇਰ_ਸਿਖਰ_ਸੂਚਕਾਂਕ(o)] = nonNull;
    }

    _catch: ਵਾਪਸ ਨਤੀਜਾ;
}

#ਜੇ_ਅੰਤ // d_m3HasTypedRefs


// How wide a table operand is read at comes from the table it names, so a slot
// holding a narrower type would be read past what was allocated for it. The
// validator refuses that already; this keeps a build without one from reaching
// the interpreter with it. c_m3Type_none means the operand is not on the
// dynamic stack to inspect, which is not this check's business.
ਸਥਿਰ
M3_ਨਤੀਜਾ Check_ਸੰਕਾਰਜ_Type (IM3Compilation o, u16 i_ਡੂੰਘਾਈ_From_ਸਿਖਰ, m3type_t i_type)
{
    m3type_t actual;

    ਜੇ (Is_ਢੇਰ_Polymorphic(o)) {
        ਵਾਪਸ m3Err_none;
    }

    actual = Get_ਢੇਰ_Type_From_ਸਿਖਰ(o, i_ਡੂੰਘਾਈ_From_ਸਿਖਰ);

    ਵਾਪਸ (actual == c_m3Type_none or ਅਧਾਰ_Type_ਦਾ(actual) == i_type) ? m3Err_none
                                                                     : m3Err_typeMismatch;
}


// Swaps the _mem register onto a given memory. Only ever emitted in pairs --
// see op_SetMemory.
ਸਥਿਰ
M3_ਨਤੀਜਾ Emit_ਸੈੱਟ_ਸਮ੍ਰਿਤੀ_Ptr (IM3Compilation o, IM3_ਸਮ੍ਰਿਤੀ i_ਸਮ੍ਰਿਤੀ)
{
    M3_ਨਤੀਜਾ ਨਤੀਜਾ = m3Err_none;

_   (EmitOp(o, op_ਸੈੱਟ_ਸਮ੍ਰਿਤੀ));
    Emit_ਸੂਚਕ(o, i_ਸਮ੍ਰਿਤੀ);

    _catch: ਵਾਪਸ ਨਤੀਜਾ;
}


// ...onto one of the compiling module's own memories, by index. Only reached
// with a non-zero index under multi-memory, but the index has already been
// bounds-checked by ReadMemoryIndex either way.
ਸਥਿਰ ਸਤਰਅੰਦਰ
M3_ਨਤੀਜਾ Emit_ਸੈੱਟ_ਸਮ੍ਰਿਤੀ (IM3Compilation o, u32 i_ਸਮ੍ਰਿਤੀ_Idx)
{
#ਜੇ d_m3_Has_Multi_ਸਮ੍ਰਿਤੀ
    ਵਾਪਸ Emit_ਸੈੱਟ_ਸਮ੍ਰਿਤੀ_Ptr(o, o->module->memories[i_ਸਮ੍ਰਿਤੀ_Idx]);
#ਨਹੀਂਤਾਂ
    (ਖਾਲੀ)o;
    (ਖਾਲੀ)i_ਸਮ੍ਰਿਤੀ_Idx;
    ਵਾਪਸ m3_Err_unknown_ਸਮ੍ਰਿਤੀ;
#ਜੇ_ਅੰਤ
}

ਸਥਿਰ
M3_ਨਤੀਜਾ Compile_Call (IM3Compilation o, m3opcode_t i_opcode)
{
_try {
    u32 function_ਸੂਚਕਾਂਕ;
_   (ReadLEB_u32(&function_ਸੂਚਕਾਂਕ, &o->wasm, o->wasm_ਸਿਰਾ));

    IM3Function function = Module_GetFunction(o->module, function_ਸੂਚਕਾਂਕ);

    ਜੇ (function) {                                                     m3log (compile, d_indent " (func= [%d] '%s'; args= %d)",
                                                                                get_indention_string (o), function_ਸੂਚਕਾਂਕ, m3_GetFunctionName (function), function->funcType->ਅੰਕ_ਦਲੀਲਾਂ);
        // an import linked to another module's export runs that module's body
        IM3Function ਨਿਸ਼ਾਨਾ = Function_Implementation(function);

        ਜੇ (ਨਿਸ਼ਾਨਾ->module) {
            bool isReturnCall = (i_opcode == c_waOp_returnCall);

            // The callee's body runs against its own module's memory, so a call
            // that crosses modules is bracketed by a pair of op_SetMemory. A
            // tail call never returns to run the second one, so those compile
            // as a plain call followed by a return instead.
            bool crossModule = (ਨਿਸ਼ਾਨਾ->module != o->module);
            bool use_ਪੂਛ_Call = isReturnCall and d_m3_Can_ਪੂਛ_Call and not crossModule;

            u16 ਥਾਂ_ਸਿਖਰ, ਅੰਕ_Arg_Slots = 0;

            ਜੇ (use_ਪੂਛ_Call) {
_               (Compile_ਪੂਛ_Call_ਦਲੀਲਾਂ(o, &ਥਾਂ_ਸਿਖਰ, &ਅੰਕ_Arg_Slots, ਨਿਸ਼ਾਨਾ->funcType, false));
            } ਨਹੀਂਤਾਂ {
_               (Compile_Call_ਦਲੀਲਾਂ_And_Return(o, &ਥਾਂ_ਸਿਖਰ, ਨਿਸ਼ਾਨਾ->funcType, false));
            }

            IM3Operation op;
            ਅਚਲ ਖਾਲੀ*  ਸੰਕਾਰਜ;

            ਜੇ (ਨਿਸ਼ਾਨਾ->compiled) {
                op      = use_ਪੂਛ_Call ? op_ReturnCall : op_Call;
                ਸੰਕਾਰਜ = ਨਿਸ਼ਾਨਾ->compiled;
            } ਨਹੀਂਤਾਂ {
                op      = use_ਪੂਛ_Call ? op_CompileReturnCall : op_Compile;
                ਸੰਕਾਰਜ = ਨਿਸ਼ਾਨਾ;
            }

#ਜੇ d_m3HasExceptionHandling
            ਜੇ (isReturnCall) {
_               (Emit_ਕੱਢੋ_Try_ਚੌਖਟੇ_For_Return_Call(o));
            }
#ਜੇ_ਅੰਤ

            ਜੇ (crossModule) {
_               (Emit_ਸੈੱਟ_ਸਮ੍ਰਿਤੀ_Ptr(o, Module_Memory0(ਨਿਸ਼ਾਨਾ->module)));
            }

_           (EmitOp(o, op));
            Emit_ਸੂਚਕ(o, ਸੰਕਾਰਜ);
            Emit_ਥਾਂ_ਵਿੱਥ(o, ਥਾਂ_ਸਿਖਰ);

            ਜੇ (not use_ਪੂਛ_Call) {
                // a suspend inside the callee leaves this frame waiting here, its
                // results not yet written
_               (ਰਿਕਾਰਡ_Call(o, Get_Func_Type_ਅੰਕ_Results(ਨਿਸ਼ਾਨਾ->funcType), ਥਾਂ_ਸਿਖਰ, isReturnCall));
            }

            ਜੇ (use_ਪੂਛ_Call) {
                Emit_ਥਾਂ_ਵਿੱਥ(o, o->function->ਅੰਕ_Ret_Slots);
                EmitConstant32(o, ਅੰਕ_Arg_Slots);

_               (ਸੈੱਟ_ਢੇਰ_Polymorphic(o));
            } ਨਹੀਂਤਾਂ {
                ਜੇ (crossModule) {
_                   (Emit_ਸੈੱਟ_ਸਮ੍ਰਿਤੀ_Ptr(o, Module_Memory0(o->module)));
                }

                ਜੇ (isReturnCall) {
_                   (Compile_Return(o, i_opcode));
                }
            }
        } ਨਹੀਂਤਾਂ {
            _throw(ErrorCompile(m3Err_functionImportMissing, o, "'%s.%s'", GetFunctionImportModuleName(function), m3_GetFunctionName(function)));
        }
    } ਨਹੀਂਤਾਂ {
        _throw(ErrorCompile(m3Err_functionLookupFailed, o, "index: %d", function_ਸੂਚਕਾਂਕ));
    }

    } _catch: ਵਾਪਸ ਨਤੀਜਾ;
}

ਸਥਿਰ
M3_ਨਤੀਜਾ Compile_Call_ਅਸਿੱਧਾ (IM3Compilation o, m3opcode_t i_opcode)
{
_try {
    u32 type_ਸੂਚਕਾਂਕ;
_   (ReadLEB_u32(&type_ਸੂਚਕਾਂਕ, &o->wasm, o->wasm_ਸਿਰਾ));

    u32 ਸਾਰਣੀ_ਸੂਚਕਾਂਕ;
_   (ReadLEB_u32(&ਸਾਰਣੀ_ਸੂਚਕਾਂਕ, &o->wasm, o->wasm_ਸਿਰਾ));

    _throwif("function call type index out of range", type_ਸੂਚਕਾਂਕ >= o->module->ਅੰਕ_Func_Types);
    _throwif("table index out of range", ਸਾਰਣੀ_ਸੂਚਕਾਂਕ >= o->module->ਅੰਕ_Tables);
    _throwif(m3Err_typeMismatch, ਅਧਾਰ_Type_ਦਾ(o->module->tables[ਸਾਰਣੀ_ਸੂਚਕਾਂਕ]->type) != c_m3Type_funcref);

_   (Check_ਸੰਕਾਰਜ_Type(o, 0, ਸਾਰਣੀ_Addr_Type(o->module->tables[ਸਾਰਣੀ_ਸੂਚਕਾਂਕ])));

    ਜੇ (Is_ਢੇਰ_ਸਿਖਰ_ਵਿੱਚ_ਰਜਿਸਟਰ(o)) {
_       (Preserve_ਰਜਿਸਟਰ_If_Occupied(o, c_m3Type_i32));
    }

    u16 ਸਾਰਣੀ_ਸੂਚਕਾਂਕ_ਥਾਂ = Get_ਢੇਰ_ਸਿਖਰ_ਥਾਂ_ਸੰਖਿਆ(o);

    bool isReturnCall = (i_opcode == c_wa_Op_return_Call_ਅਸਿੱਧਾ);
    bool use_ਪੂਛ_Call  = isReturnCall and d_m3_Can_ਪੂਛ_Call;

    u16         exec_ਸਿਖਰ, ਅੰਕ_Arg_Slots = 0;
    IM3FuncType type = o->module->funcTypes[type_ਸੂਚਕਾਂਕ];

    ਜੇ (use_ਪੂਛ_Call) {
_       (Compile_ਪੂਛ_Call_ਦਲੀਲਾਂ(o, &exec_ਸਿਖਰ, &ਅੰਕ_Arg_Slots, type, true));
    } ਨਹੀਂਤਾਂ {
_       (Compile_Call_ਦਲੀਲਾਂ_And_Return(o, &exec_ਸਿਖਰ, type, true));
    }

#ਜੇ d_m3HasExceptionHandling
    ਜੇ (isReturnCall) {
_       (Emit_ਕੱਢੋ_Try_ਚੌਖਟੇ_For_Return_Call(o));
    }
#ਜੇ_ਅੰਤ

    // the table comes first: how wide its indexes are is what says how much of
    // the slot below holds one
_   (EmitOp(o, use_ਪੂਛ_Call ? op_Return_Call_ਅਸਿੱਧਾ : op_Call_ਅਸਿੱਧਾ));
    Emit_ਸੂਚਕ(o, o->module->tables[ਸਾਰਣੀ_ਸੂਚਕਾਂਕ]);
    Emit_ਥਾਂ_ਵਿੱਥ(o, ਸਾਰਣੀ_ਸੂਚਕਾਂਕ_ਥਾਂ);
    Emit_ਸੂਚਕ(o, type);
    Emit_ਥਾਂ_ਵਿੱਥ(o, exec_ਸਿਖਰ);

    ਜੇ (not use_ਪੂਛ_Call) {
_       (ਰਿਕਾਰਡ_Call(o, Get_Func_Type_ਅੰਕ_Results(type), exec_ਸਿਖਰ, isReturnCall));
    }

    ਜੇ (use_ਪੂਛ_Call) {
        Emit_ਥਾਂ_ਵਿੱਥ(o, o->function->ਅੰਕ_Ret_Slots);
        EmitConstant32(o, ਅੰਕ_Arg_Slots);

_       (ਸੈੱਟ_ਢੇਰ_Polymorphic(o));
    } ਨਹੀਂਤਾਂ ਜੇ (isReturnCall) {
_       (Compile_Return(o, i_opcode));
    }

} _catch:
    ਵਾਪਸ ਨਤੀਜਾ;
}

// Reads the memory index a memory instruction names, and checks it against the
// module's index space. Without multi-memory the immediate is a reserved byte
// that has to be zero, which is the same check.
ਸਥਿਰ
M3_ਨਤੀਜਾ Read_ਸਮ੍ਰਿਤੀ_ਸੂਚਕਾਂਕ (IM3Compilation o, u32* o_ਸਮ੍ਰਿਤੀ_Idx)
{
    M3_ਨਤੀਜਾ ਨਤੀਜਾ;

_   (ReadLEB_u32(o_ਸਮ੍ਰਿਤੀ_Idx, &o->wasm, o->wasm_ਸਿਰਾ));

    _throwif(m3_Err_unknown_ਸਮ੍ਰਿਤੀ, *o_ਸਮ੍ਰਿਤੀ_Idx >= o->module->ਅੰਕ_Memories);

    _catch: ਵਾਪਸ ਨਤੀਜਾ;
}

ਸਥਿਰ
M3_ਨਤੀਜਾ Compile_ਸਮ੍ਰਿਤੀ_ਮਾਪ (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_ਨਤੀਜਾ ਨਤੀਜਾ;

    u32 ਸਮ੍ਰਿਤੀ_Idx;

    // A page count is given in the memory's own address type. Declared up here
    // so the throws below don't jump over its initialization.
    m3type_t addrType = c_m3Type_i32;

_   (Read_ਸਮ੍ਰਿਤੀ_ਸੂਚਕਾਂਕ(o, &ਸਮ੍ਰਿਤੀ_Idx));

    addrType = ਸਮ੍ਰਿਤੀ_Addr_Type(o->module->memories[ਸਮ੍ਰਿਤੀ_Idx]);

_   (Preserve_ਰਜਿਸਟਰ_If_Occupied(o, addrType));

    ਜੇ (ਸਮ੍ਰਿਤੀ_Idx) {
_       (Emit_ਸੈੱਟ_ਸਮ੍ਰਿਤੀ(o, ਸਮ੍ਰਿਤੀ_Idx));
    }

    // op_MemSize writes the whole register either way; the slot it is pushed
    // to is what decides how much of it the module gets to see
_   (EmitOp(o, op_Mem_ਮਾਪ));

    ਜੇ (ਸਮ੍ਰਿਤੀ_Idx) {
_       (Emit_ਸੈੱਟ_ਸਮ੍ਰਿਤੀ(o, 0));
    }

_   (ਧੱਕੋ_ਰਜਿਸਟਰ(o, addrType));

    _catch: ਵਾਪਸ ਨਤੀਜਾ;
}

ਸਥਿਰ
M3_ਨਤੀਜਾ Compile_ਸਮ੍ਰਿਤੀ_Grow (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_ਨਤੀਜਾ ਨਤੀਜਾ;

    u32 ਸਮ੍ਰਿਤੀ_Idx;

    // see Compile_Memory_Size
    m3type_t addrType = c_m3Type_i32;

_   (Read_ਸਮ੍ਰਿਤੀ_ਸੂਚਕਾਂਕ(o, &ਸਮ੍ਰਿਤੀ_Idx));

    addrType = ਸਮ੍ਰਿਤੀ_Addr_Type(o->module->memories[ਸਮ੍ਰਿਤੀ_Idx]);

_   (Copy_ਢੇਰ_ਸਿਖਰ_ਮੰਜ਼ਿਲ_ਰਜਿਸਟਰ(o, false));
_   (ਕੱਢੋ_Type(o, addrType));

    ਜੇ (ਸਮ੍ਰਿਤੀ_Idx) {
_       (Emit_ਸੈੱਟ_ਸਮ੍ਰਿਤੀ(o, ਸਮ੍ਰਿਤੀ_Idx));
    }

#ਜੇ d_m3HasMemory64
_   (EmitOp(o, (addrType == c_m3Type_i64) ? op_MemGrow64 : op_MemGrow));
#ਨਹੀਂਤਾਂ
_   (EmitOp(o, op_MemGrow));
#ਜੇ_ਅੰਤ

    ਜੇ (ਸਮ੍ਰਿਤੀ_Idx) {
_       (Emit_ਸੈੱਟ_ਸਮ੍ਰਿਤੀ(o, 0));
    }

_   (ਧੱਕੋ_ਰਜਿਸਟਰ(o, addrType));

    _catch: ਵਾਪਸ ਨਤੀਜਾ;
}

ਸਥਿਰ
M3_ਨਤੀਜਾ Compile_ਸਮ੍ਰਿਤੀ_Copy_Fill (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_ਨਤੀਜਾ ਨਤੀਜਾ = m3Err_none;

    // memory.copy names the destination first and then the source; memory.fill
    // names only the one it writes.
    u32  ਸਰੋਤ_ਸਮ੍ਰਿਤੀ_Idx = 0, ਨਿਸ਼ਾਨਾ_ਸਮ੍ਰਿਤੀ_Idx = 0;
    bool isCopy = (i_opcode == c_wa_Op_ਸਮ੍ਰਿਤੀ_Copy);

    // Each address is typed by the memory it belongs to. The length follows
    // the narrower of the two, so it is 64-bit only when both are - which for
    // memory.fill, naming one memory twice over, means whenever that one is.
    // Declared up here so the throws below don't jump over them.
    m3type_t ਨਿਸ਼ਾਨਾ_Type = c_m3Type_i32;
    m3type_t ਸਰੋਤ_Type = c_m3Type_i32;
    m3type_t ਲੰਮਾਈ_Type = c_m3Type_i32;

    _throwif(m3Err_wasmMalformed, o->module->ਅੰਕ_Memories == 0);

_   (Read_ਸਮ੍ਰਿਤੀ_ਸੂਚਕਾਂਕ(o, &ਨਿਸ਼ਾਨਾ_ਸਮ੍ਰਿਤੀ_Idx));

    ਜੇ (isCopy) {
_       (Read_ਸਮ੍ਰਿਤੀ_ਸੂਚਕਾਂਕ(o, &ਸਰੋਤ_ਸਮ੍ਰਿਤੀ_Idx));
    }

    ਨਿਸ਼ਾਨਾ_Type = ਸਮ੍ਰਿਤੀ_Addr_Type(o->module->memories[ਨਿਸ਼ਾਨਾ_ਸਮ੍ਰਿਤੀ_Idx]);
    ਸਰੋਤ_Type = isCopy ? ਸਮ੍ਰਿਤੀ_Addr_Type(o->module->memories[ਸਰੋਤ_ਸਮ੍ਰਿਤੀ_Idx])
                        : ਨਿਸ਼ਾਨਾ_Type;
    ਲੰਮਾਈ_Type = (ਨਿਸ਼ਾਨਾ_Type == c_m3Type_i64 and ਸਰੋਤ_Type == c_m3Type_i64)
                   ? c_m3Type_i64
                   : c_m3Type_i32;

_   (Copy_ਢੇਰ_ਸਿਖਰ_ਮੰਜ਼ਿਲ_ਰਜਿਸਟਰ(o, false));

#ਜੇ d_m3_Has_Multi_ਸਮ੍ਰਿਤੀ
    ਜੇ (isCopy and ਸਰੋਤ_ਸਮ੍ਰਿਤੀ_Idx != ਨਿਸ਼ਾਨਾ_ਸਮ੍ਰਿਤੀ_Idx) {
        // two memories at once: _mem can only name one, so the op takes both
_       (EmitOp(o, op_MemCopy_x));
        Emit_ਸੂਚਕ(o, o->module->memories[ਨਿਸ਼ਾਨਾ_ਸਮ੍ਰਿਤੀ_Idx]);
        Emit_ਸੂਚਕ(o, o->module->memories[ਸਰੋਤ_ਸਮ੍ਰਿਤੀ_Idx]);

        // ...and, since they need not be addressed alike, how wide to read
        // each of the operands: bit 0 the destination, bit 1 the source
        EmitConstant32(o, ((ਨਿਸ਼ਾਨਾ_Type == c_m3Type_i64) ? 0x1u : 0u) |
                            ((ਸਰੋਤ_Type == c_m3Type_i64) ? 0x2u : 0u));
    } ਨਹੀਂਤਾਂ
#ਜੇ_ਅੰਤ
    {
        IM3Operation op = isCopy ? op_MemCopy : op_MemFill;

#ਜੇ d_m3HasMemory64
        ਜੇ (ਨਿਸ਼ਾਨਾ_Type == c_m3Type_i64) {
            op = isCopy ? op_MemCopy64 : op_MemFill64;
        }
#ਜੇ_ਅੰਤ

        ਜੇ (ਨਿਸ਼ਾਨਾ_ਸਮ੍ਰਿਤੀ_Idx) {
_           (Emit_ਸੈੱਟ_ਸਮ੍ਰਿਤੀ(o, ਨਿਸ਼ਾਨਾ_ਸਮ੍ਰਿਤੀ_Idx));
        }

_       (EmitOp(o, op));
    }

    // the length is in the register; the other two are named by slot, at
    // whatever width their stack entries were pushed with
_   (ਕੱਢੋ_Type(o, ਲੰਮਾਈ_Type));
_   (Emit_ਥਾਂ_ਅੰਕ_ਦਾ_ਢੇਰ_ਸਿਖਰ_And_ਕੱਢੋ(o));      // the source, or the byte to fill with
_   (Emit_ਥਾਂ_ਅੰਕ_ਦਾ_ਢੇਰ_ਸਿਖਰ_And_ਕੱਢੋ(o));      // the destination

    ਜੇ (ਨਿਸ਼ਾਨਾ_ਸਮ੍ਰਿਤੀ_Idx and not (isCopy and ਸਰੋਤ_ਸਮ੍ਰਿਤੀ_Idx != ਨਿਸ਼ਾਨਾ_ਸਮ੍ਰਿਤੀ_Idx)) {
_       (Emit_ਸੈੱਟ_ਸਮ੍ਰਿਤੀ(o, 0));
    }

    _catch: ਵਾਪਸ ਨਤੀਜਾ;
}


// memory.init and data.drop address a data segment by index. Both are only valid
// when a data count section declared the segments up front.
ਸਥਿਰ
M3_ਨਤੀਜਾ Read_ਅੰਕੜੇ_Segment (IM3Compilation o, M3_ਅੰਕੜੇ_Segment** o_segment)
{
    M3_ਨਤੀਜਾ ਨਤੀਜਾ = m3Err_none;

    u32 index;
_   (ReadLEB_u32(&index, &o->wasm, o->wasm_ਸਿਰਾ));

    _throwif("data count section required", not o->module->has_ਅੰਕੜੇ_ਗਿਣਤੀ);
    _throwif(m3Err_wasmMalformed, index >= o->module->ਅੰਕ_ਅੰਕੜੇ_Segments);

    *o_segment = &o->module->ਅੰਕੜੇ_Segments[index];

    _catch: ਵਾਪਸ ਨਤੀਜਾ;
}


M3_ਨਤੀਜਾ Compile_ਸਮ੍ਰਿਤੀ_Init (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_ਨਤੀਜਾ ਨਤੀਜਾ = m3Err_none;

    M3_ਅੰਕੜੇ_Segment* segment = ਨਿਰਾ;
    u32            ਸਮ੍ਰਿਤੀ_Idx;

    _throwif(m3Err_wasmMalformed, o->module->ਅੰਕ_Memories == 0);

_   (Read_ਅੰਕੜੇ_Segment(o, &segment));
_   (Read_ਸਮ੍ਰਿਤੀ_ਸੂਚਕਾਂਕ(o, &ਸਮ੍ਰਿਤੀ_Idx));

_   (Copy_ਢੇਰ_ਸਿਖਰ_ਮੰਜ਼ਿਲ_ਰਜਿਸਟਰ(o, false));

    ਜੇ (ਸਮ੍ਰਿਤੀ_Idx) {
_       (Emit_ਸੈੱਟ_ਸਮ੍ਰਿਤੀ(o, ਸਮ੍ਰਿਤੀ_Idx));
    }

#ਜੇ d_m3HasMemory64
    // only the destination follows the memory's address type: a data segment
    // is indexed as an i32 however the memory it is copied into is addressed
_   (EmitOp(o, o->module->memories[ਸਮ੍ਰਿਤੀ_Idx]->isMemory64 ? op_MemInit64 : op_MemInit));
#ਨਹੀਂਤਾਂ
_   (EmitOp(o, op_MemInit));
#ਜੇ_ਅੰਤ
    Emit_ਸੂਚਕ(o, segment);
_   (ਕੱਢੋ_Type(o, c_m3Type_i32));
_   (Emit_ਥਾਂ_ਅੰਕ_ਦਾ_ਢੇਰ_ਸਿਖਰ_And_ਕੱਢੋ(o));
_   (Emit_ਥਾਂ_ਅੰਕ_ਦਾ_ਢੇਰ_ਸਿਖਰ_And_ਕੱਢੋ(o));

    ਜੇ (ਸਮ੍ਰਿਤੀ_Idx) {
_       (Emit_ਸੈੱਟ_ਸਮ੍ਰਿਤੀ(o, 0));
    }

    _catch: ਵਾਪਸ ਨਤੀਜਾ;
}


#ਜੇ d_m3HasRefTypes

ਸਥਿਰ
M3_ਨਤੀਜਾ Compile_Select (IM3Compilation o, m3opcode_t i_opcode);

// select with an explicit result type vector. The types only matter to the
// validator; the operands are laid out exactly as for the untyped select.
M3_ਨਤੀਜਾ Compile_Select_Typed (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_ਨਤੀਜਾ ਨਤੀਜਾ = m3Err_none;

    u32 ਅੰਕ_Types;
_   (ReadLEB_u32(&ਅੰਕ_Types, &o->wasm, o->wasm_ਸਿਰਾ));
    _throwif(m3Err_wasmMalformed, ਅੰਕ_Types != 1);

    i8 waType;
    u8 type;
_   (ReadLEB_i7(&waType, &o->wasm, o->wasm_ਸਿਰਾ));
_   (NormalizeType(&type, waType));

_   (Compile_Select(o, i_opcode));

    _catch: ਵਾਪਸ ਨਤੀਜਾ;
}


// A reference is one pointer-sized word and null is 0, so null-testing it is
// just an integer eqz of the matching width.
#  ਜੇ M3_SIZEOF_PTR == 8
#    define d_m3RefIsNull_r   op_i64_Equal_ਮੰਜ਼ਿਲ_Zero_r
#    define d_m3RefIsNull_s   op_i64_Equal_ਮੰਜ਼ਿਲ_Zero_s
#  ਨਹੀਂਤਾਂ
#    define d_m3RefIsNull_r   op_i32_Equal_ਮੰਜ਼ਿਲ_Zero_r
#    define d_m3RefIsNull_s   op_i32_Equal_ਮੰਜ਼ਿਲ_Zero_s
#  endif

ਸਥਿਰ
M3_ਨਤੀਜਾ ReadRefType (IM3Compilation o, m3type_t* o_type)
{
    M3_ਨਤੀਜਾ ਨਤੀਜਾ = m3Err_none;

#  ਜੇ d_m3HasTypedRefs
    // ref.null names a heap type: func, extern, or a function type index
    m3type_t ਢੇਰ_ਬਿੱਟ;
_   (Parse_ਢੇਰ_Type(o->module, &ਢੇਰ_ਬਿੱਟ, &o->wasm, o->wasm_ਸਿਰਾ));

    *o_type = d_m3Type_ref | ਢੇਰ_ਬਿੱਟ;
#  ਨਹੀਂਤਾਂ
    i8 waType;
    u8 plainType;
_   (ReadLEB_i7(&waType, &o->wasm, o->wasm_ਸਿਰਾ));
_   (NormalizeType(&plainType, waType));
    *o_type = plainType;
    _throwif(m3Err_wasmMalformed, not IsRefType(*o_type));
#  endif

    _catch: ਵਾਪਸ ਨਤੀਜਾ;
}


M3_ਨਤੀਜਾ Compile_Ref_Null (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_ਨਤੀਜਾ ਨਤੀਜਾ = m3Err_none;

    m3type_t type;
_   (ReadRefType(o, &type));
_   (ਧੱਕੋ_Const(o, 0, type));

    _catch: ਵਾਪਸ ਨਤੀਜਾ;
}


M3_ਨਤੀਜਾ Compile_Ref_IsNull (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_ਨਤੀਜਾ ਨਤੀਜਾ = m3Err_none;

    IM3Operation op;

    ਜੇ (not Is_ਢੇਰ_Polymorphic(o)) {
        _throwif(m3Err_typeMismatch, not IsRefType(Get_ਢੇਰ_ਸਿਖਰ_Type(o)));
    }

    ਜੇ (Is_ਢੇਰ_ਸਿਖਰ_ਵਿੱਚ_ਰਜਿਸਟਰ(o)) {
        op = d_m3RefIsNull_r;
    } ਨਹੀਂਤਾਂ {
_       (Preserve_ਰਜਿਸਟਰ_If_Occupied(o, c_m3Type_i32));
        op = d_m3RefIsNull_s;
    }

_   (EmitOp(o, op));
_   (Emit_ਥਾਂ_ਅੰਕ_ਦਾ_ਢੇਰ_ਸਿਖਰ_And_ਕੱਢੋ(o));
_   (ਧੱਕੋ_ਰਜਿਸਟਰ(o, c_m3Type_i32));

    _catch: ਵਾਪਸ ਨਤੀਜਾ;
}


M3_ਨਤੀਜਾ Compile_Ref_Func (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_ਨਤੀਜਾ ਨਤੀਜਾ = m3Err_none;

    u32 func_ਸੂਚਕਾਂਕ;

    // declared before the throws below, which jump past this point to _catch
    m3type_t    refType   = c_m3Type_funcref;
    IM3Function reference = ਨਿਰਾ;

_   (ReadLEB_u32(&func_ਸੂਚਕਾਂਕ, &o->wasm, o->wasm_ਸਿਰਾ));
    _throwif("function index out of range", func_ਸੂਚਕਾਂਕ >= o->module->ਅੰਕ_Functions);

    // Inside a constant expression ref.func is itself a declaration; inside a
    // function body the function must already have been declared elsewhere.
    ਜੇ (o->function) {
        _throwif("undeclared function reference", not Module_IsFunctionDeclared(o->module, func_ਸੂਚਕਾਂਕ));
    } ਨਹੀਂਤਾਂ {
_       (Module_DeclareFunction(o->module, func_ਸੂਚਕਾਂਕ));
    }

#  ਜੇ d_m3HasTypedRefs
    refType = Ref_Type_ਦਾ_Func_Type(o->module->functions[func_ਸੂਚਕਾਂਕ].funcType, true);
#  endif

    // A reference denotes the function that actually runs, so a ref.func naming
    // an import linked to another module is the same reference that module's
    // own ref.func would produce - and carries the defining module with it.
    reference = Function_Implementation(&o->module->functions[func_ਸੂਚਕਾਂਕ]);

_   (ਧੱਕੋ_Const(o, (u64)(uintptr_t)reference, refType));

    _catch: ਵਾਪਸ ਨਤੀਜਾ;
}


// table.get/set/size/grow/fill all name a table by index; the table struct goes
// into the codestream so the operation doesn't have to walk the module.
ਸਥਿਰ
M3_ਨਤੀਜਾ Read_ਸਾਰਣੀ (IM3Compilation o, M3_ਸਾਰਣੀ** o_ਸਾਰਣੀ)
{
    M3_ਨਤੀਜਾ ਨਤੀਜਾ = m3Err_none;

    u32 index;
_   (ReadLEB_u32(&index, &o->wasm, o->wasm_ਸਿਰਾ));
    _throwif("table index out of range", index >= o->module->ਅੰਕ_Tables);

    *o_ਸਾਰਣੀ = o->module->tables[index];

    _catch: ਵਾਪਸ ਨਤੀਜਾ;
}


// The table operations take every operand from a slot, so spill the integer
// register first: after this each EmitSlotNumOfStackTopAndPop () emits a real
// slot offset, and the register is free for the result. Operands are emitted
// from the top of the stack downwards, which is the order the ops read them.
ਸਥਿਰ
M3_ਨਤੀਜਾ Compile_ਸਾਰਣੀ_Op (IM3Compilation o, IM3Operation i_op, M3_ਸਾਰਣੀ* i_ਸਾਰਣੀ, u32 i_ਅੰਕ_Operands, m3type_t i_retType)
{
    M3_ਨਤੀਜਾ ਨਤੀਜਾ = m3Err_none;

_   (Preserve_ਰਜਿਸਟਰ_If_Occupied(o, c_m3Type_i64));

_   (EmitOp(o, i_op));
    Emit_ਸੂਚਕ(o, i_ਸਾਰਣੀ);

    ਲਈ (u32 i = 0; i < i_ਅੰਕ_Operands; ++i) {
_       (Emit_ਥਾਂ_ਅੰਕ_ਦਾ_ਢੇਰ_ਸਿਖਰ_And_ਕੱਢੋ(o));
    }

    ਜੇ (i_retType != c_m3Type_none) {
_       (ਧੱਕੋ_ਰਜਿਸਟਰ(o, i_retType));
    }

    _catch: ਵਾਪਸ ਨਤੀਜਾ;
}


M3_ਨਤੀਜਾ Compile_ਸਾਰਣੀ_Get_ਸੈੱਟ (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_ਨਤੀਜਾ ਨਤੀਜਾ = m3Err_none;

    M3_ਸਾਰਣੀ* ਸਾਰਣੀ_2;
_   (Read_ਸਾਰਣੀ(o, &ਸਾਰਣੀ_2));

    ਜੇ (i_opcode == c_wa_Op_ਸਾਰਣੀ_Get) {
_       (Check_ਸੰਕਾਰਜ_Type(o, 0, ਸਾਰਣੀ_Addr_Type(ਸਾਰਣੀ_2)));
_       (Compile_ਸਾਰਣੀ_Op(o, op_ਸਾਰਣੀ_Get, ਸਾਰਣੀ_2, 1, ਸਾਰਣੀ_2->type));
    } ਨਹੀਂਤਾਂ {
_       (Check_ਸੰਕਾਰਜ_Type(o, 1, ਸਾਰਣੀ_Addr_Type(ਸਾਰਣੀ_2)));
_       (Compile_ਸਾਰਣੀ_Op(o, op_ਸਾਰਣੀ_ਸੈੱਟ, ਸਾਰਣੀ_2, 2, c_m3Type_none));
    }

    _catch: ਵਾਪਸ ਨਤੀਜਾ;
}


M3_ਨਤੀਜਾ Compile_ਸਾਰਣੀ_Init (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_ਨਤੀਜਾ ਨਤੀਜਾ = m3Err_none;

    u32      elem_ਸੂਚਕਾਂਕ;
    M3_ਸਾਰਣੀ* ਸਾਰਣੀ_2;

_   (ReadLEB_u32(&elem_ਸੂਚਕਾਂਕ, &o->wasm, o->wasm_ਸਿਰਾ));
    _throwif("element segment index out of range", elem_ਸੂਚਕਾਂਕ >= o->module->ਅੰਕ_Element_Segments);
_   (Read_ਸਾਰਣੀ(o, &ਸਾਰਣੀ_2));

_   (Check_ਸੰਕਾਰਜ_Type(o, 2, ਸਾਰਣੀ_Addr_Type(ਸਾਰਣੀ_2)));

_   (Preserve_ਰਜਿਸਟਰ_If_Occupied(o, c_m3Type_i64));
_   (EmitOp(o, op_ਸਾਰਣੀ_Init));
    Emit_ਸੂਚਕ(o, ਸਾਰਣੀ_2);
    Emit_ਸੂਚਕ(o, &o->module->elementSegments[elem_ਸੂਚਕਾਂਕ]);

    ਲਈ (u32 i = 0; i < 3; ++i) {
_       (Emit_ਥਾਂ_ਅੰਕ_ਦਾ_ਢੇਰ_ਸਿਖਰ_And_ਕੱਢੋ(o));
    }

    _catch: ਵਾਪਸ ਨਤੀਜਾ;
}


M3_ਨਤੀਜਾ Compile_Elem_Drop (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_ਨਤੀਜਾ ਨਤੀਜਾ = m3Err_none;

    u32 elem_ਸੂਚਕਾਂਕ;
_   (ReadLEB_u32(&elem_ਸੂਚਕਾਂਕ, &o->wasm, o->wasm_ਸਿਰਾ));
    _throwif("element segment index out of range", elem_ਸੂਚਕਾਂਕ >= o->module->ਅੰਕ_Element_Segments);

_   (EmitOp(o, op_ElemDrop));
    Emit_ਸੂਚਕ(o, &o->module->elementSegments[elem_ਸੂਚਕਾਂਕ]);

    _catch: ਵਾਪਸ ਨਤੀਜਾ;
}


M3_ਨਤੀਜਾ Compile_ਸਾਰਣੀ_Copy (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_ਨਤੀਜਾ ਨਤੀਜਾ = m3Err_none;

    M3_ਸਾਰਣੀ* dst;
    M3_ਸਾਰਣੀ* src;

_   (Read_ਸਾਰਣੀ(o, &dst));
_   (Read_ਸਾਰਣੀ(o, &src));
    _throwif(m3Err_typeMismatch, not Is_Sub_Type_ਦਾ(src->type, dst->type));

_   (Check_ਸੰਕਾਰਜ_Type(o, 2, ਸਾਰਣੀ_Addr_Type(dst)));
_   (Check_ਸੰਕਾਰਜ_Type(o, 1, ਸਾਰਣੀ_Addr_Type(src)));
_   (Check_ਸੰਕਾਰਜ_Type(o, 0, (dst->isTable64 and src->isTable64) ? c_m3Type_i64 : c_m3Type_i32));

_   (Preserve_ਰਜਿਸਟਰ_If_Occupied(o, c_m3Type_i64));
_   (EmitOp(o, op_ਸਾਰਣੀ_Copy));
    Emit_ਸੂਚਕ(o, dst);
    Emit_ਸੂਚਕ(o, src);

    ਲਈ (u32 i = 0; i < 3; ++i) {
_       (Emit_ਥਾਂ_ਅੰਕ_ਦਾ_ਢੇਰ_ਸਿਖਰ_And_ਕੱਢੋ(o));
    }

    _catch: ਵਾਪਸ ਨਤੀਜਾ;
}


M3_ਨਤੀਜਾ Compile_ਸਾਰਣੀ_ਮਾਪ (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_ਨਤੀਜਾ ਨਤੀਜਾ = m3Err_none;

    M3_ਸਾਰਣੀ* ਸਾਰਣੀ_2;
_   (Read_ਸਾਰਣੀ(o, &ਸਾਰਣੀ_2));

    // a table size is given in the table's own index type
_   (Compile_ਸਾਰਣੀ_Op(o, op_ਸਾਰਣੀ_ਮਾਪ, ਸਾਰਣੀ_2, 0, ਸਾਰਣੀ_Addr_Type(ਸਾਰਣੀ_2)));

    _catch: ਵਾਪਸ ਨਤੀਜਾ;
}


M3_ਨਤੀਜਾ Compile_ਸਾਰਣੀ_Grow_Fill (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_ਨਤੀਜਾ ਨਤੀਜਾ = m3Err_none;

    M3_ਸਾਰਣੀ* ਸਾਰਣੀ_2;
_   (Read_ਸਾਰਣੀ(o, &ਸਾਰਣੀ_2));

    ਜੇ (i_opcode == c_wa_Op_ਸਾਰਣੀ_Grow) {
_       (Check_ਸੰਕਾਰਜ_Type(o, 0, ਸਾਰਣੀ_Addr_Type(ਸਾਰਣੀ_2)));
_       (Compile_ਸਾਰਣੀ_Op(o, op_ਸਾਰਣੀ_Grow, ਸਾਰਣੀ_2, 2, ਸਾਰਣੀ_Addr_Type(ਸਾਰਣੀ_2)));
    } ਨਹੀਂਤਾਂ {
_       (Check_ਸੰਕਾਰਜ_Type(o, 0, ਸਾਰਣੀ_Addr_Type(ਸਾਰਣੀ_2)));
_       (Check_ਸੰਕਾਰਜ_Type(o, 2, ਸਾਰਣੀ_Addr_Type(ਸਾਰਣੀ_2)));
_       (Compile_ਸਾਰਣੀ_Op(o, op_ਸਾਰਣੀ_Fill, ਸਾਰਣੀ_2, 3, c_m3Type_none));
    }

    _catch: ਵਾਪਸ ਨਤੀਜਾ;
}

#ਜੇ_ਅੰਤ // d_m3HasRefTypes


M3_ਨਤੀਜਾ Compile_ਅੰਕੜੇ_Drop (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_ਨਤੀਜਾ ਨਤੀਜਾ = m3Err_none;

    M3_ਅੰਕੜੇ_Segment* segment = ਨਿਰਾ;
_   (Read_ਅੰਕੜੇ_Segment(o, &segment));

_   (EmitOp(o, op_ਅੰਕੜੇ_Drop));
    Emit_ਸੂਚਕ(o, segment);

    _catch: ਵਾਪਸ ਨਤੀਜਾ;
}


ਸਥਿਰ
M3_ਨਤੀਜਾ Read_ਖੰਡ_Type (IM3Compilation o, IM3FuncType* o_ਖੰਡ_Type)
{
    M3_ਨਤੀਜਾ ਨਤੀਜਾ = m3Err_none;

#ਜੇ d_m3HasTypedRefs
    // a spelled-out reference type is two bytes, so it cannot be told apart
    // from a type index by the s33 that block types otherwise use
    ਜੇ (o->wasm < o->wasm_ਸਿਰਾ and (*o->wasm == d_waEncode_ref or *o->wasm == d_waEncode_refNull)) {
        m3type_t refType;
_       (Parse_ਮੁੱਲ_Type(o->module, &refType, &o->wasm, o->wasm_ਸਿਰਾ));
        IM3FuncType ftype = ਨਿਰਾ;
_       (AllocFuncType(&ftype, 1));
        ftype->ਅੰਕ_Rets  = 1;
        ftype->types[0] = refType;
_       (Environment_ਜੋੜੋ_Func_Type(o->module->environment, &ftype));
        *o_ਖੰਡ_Type = ftype;
        ਵਾਪਸ ਨਤੀਜਾ;
    }
#ਜੇ_ਅੰਤ

    i64 type;
_   (ReadLebSigned(&type, 33, &o->wasm, o->wasm_ਸਿਰਾ));

    ਜੇ (type < 0) {
        _throwif(m3Err_invalidTypeId, type < -64);
        u8 ਮੁੱਲ_Type;
_       (NormalizeType(&ਮੁੱਲ_Type, (i8)type));                                 m3log (compile, d_indent " (type: %s)", get_indention_string (o), c_waTypes [ਮੁੱਲ_Type]);
        *o_ਖੰਡ_Type = o->module->environment->retFuncTypes[ਮੁੱਲ_Type];
    } ਨਹੀਂਤਾਂ {
        _throwif("func type out of bounds", type >= o->module->ਅੰਕ_Func_Types);
        *o_ਖੰਡ_Type = o->module->funcTypes[type];                         m3log (compile, d_indent " (type: %s)", get_indention_string (o), SPrintFuncTypeSignature (*o_ਖੰਡ_Type));
    }
    _catch: ਵਾਪਸ ਨਤੀਜਾ;
}

ਸਥਿਰ
M3_ਨਤੀਜਾ Preserve_ਦਲੀਲਾਂ_And_Locals (IM3Compilation o)
{
    M3_ਨਤੀਜਾ ਨਤੀਜਾ = m3Err_none;

    ਜੇ (o->ਢੇਰ_ਸੂਚਕਾਂਕ > o->ਢੇਰ_ਪਹਿਲਾ_Dynamic_ਸੂਚਕਾਂਕ) {
        u32 ਅੰਕ_ਦਲੀਲਾਂ_And_Locals = Get_Function_ਅੰਕ_ਦਲੀਲਾਂ_And_Locals(o->function);

        ਲਈ (u32 i = 0; i < ਅੰਕ_ਦਲੀਲਾਂ_And_Locals; ++i) {
            u16 ਥਾਂ = Get_ਥਾਂ_For_ਢੇਰ_ਸੂਚਕਾਂਕ(o, i);

            u16 preserved_ਥਾਂ_ਸੰਖਿਆ;
_           (ਲੱਭੋ_Referenced_Local_Within_Current_ਖੰਡ(o, &preserved_ਥਾਂ_ਸੰਖਿਆ, ਥਾਂ));

            ਜੇ (preserved_ਥਾਂ_ਸੰਖਿਆ != ਥਾਂ) {
                m3type_t     type = Get_ਢੇਰ_Type_From_Bottom(o, i);                 d_m3Assert (type != c_m3Type_none)
                IM3Operation op   = Is64_ਦੋਆਧਾਰੀ_ਅੰਕ_Type(type) ? op_Copy_ਥਾਂ_64 : op_Copy_ਥਾਂ_32;

                EmitOp(o, op);
                Emit_ਥਾਂ_ਵਿੱਥ(o, preserved_ਥਾਂ_ਸੰਖਿਆ);
                Emit_ਥਾਂ_ਵਿੱਥ(o, ਥਾਂ);
            }
        }
    }

    _catch:
    ਵਾਪਸ ਨਤੀਜਾ;
}

ਸਥਿਰ
M3_ਨਤੀਜਾ Compile_Loop_Or_ਖੰਡ (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_ਨਤੀਜਾ ਨਤੀਜਾ;

    IM3FuncType ਖੰਡ_Type = ਨਿਰਾ;

_   (PreserveRegisters(o));
_   (Preserve_ਦਲੀਲਾਂ_And_Locals(o));

_   (Read_ਖੰਡ_Type(o, &ਖੰਡ_Type));

    ਜੇ (i_opcode == c_waOp_loop) {
        u16 ਅੰਕ_Params = Get_Func_Type_ਅੰਕ_Params(ਖੰਡ_Type);
        ਜੇ (ਅੰਕ_Params) {
            // instantiate constants
            u16 ਅੰਕ_ਮੁੱਲਾਂ = Get_ਅੰਕ_ਖੰਡ_ਮੁੱਲਾਂ_On_ਢੇਰ(o);

            ਜੇ (ਅੰਕ_ਮੁੱਲਾਂ >= ਅੰਕ_Params) {
                u16 ਢੇਰ_ਸਿਖਰ = Get_ਢੇਰ_ਸਿਖਰ_ਸੂਚਕਾਂਕ(o) + 1;

                ਲਈ (u16 i = ਢੇਰ_ਸਿਖਰ - ਅੰਕ_Params; i < ਢੇਰ_ਸਿਖਰ; ++i) {
                    u16      ਥਾਂ = Get_ਥਾਂ_For_ਢੇਰ_ਸੂਚਕਾਂਕ(o, i);
                    m3type_t type = Get_ਢੇਰ_Type_From_Bottom(o, i);

                    ਜੇ (Is_Constant_ਥਾਂ(o, ਥਾਂ)) {
                        u16 new_ਥਾਂ = c_ਥਾਂ_ਅਣਵਰਤਿਆ;
_                       (AllocateSlots(o, &new_ਥਾਂ, type));
_                       (Copy_ਢੇਰ_ਸੂਚਕਾਂਕ_ਮੰਜ਼ਿਲ_ਥਾਂ(o, new_ਥਾਂ, i));
                        o->wasm_ਢੇਰ[i] = new_ਥਾਂ;
                    }
                }
            }
        }

_       (EmitOp(o, op_Loop));
    } ਨਹੀਂਤਾਂ {
    }

_   (Compile_ਖੰਡ(o, ਖੰਡ_Type, i_opcode));

    _catch: ਵਾਪਸ ਨਤੀਜਾ;
}

#ਜੇ d_m3HasExceptionHandling

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
ਸਥਿਰ
M3_ਨਤੀਜਾ EmitCatchStubs (IM3Compilation o)
{
    IM3_ਕੋਡ_ਪੰਨਾ displaced_ਪੰਨਾ = ਨਿਰਾ;
_try {
    M3CatchClause* clauses    = o->tryClauses;
    u32            ਅੰਕ_Clauses = o->ਅੰਕ_Try_Clauses;

    // the body must not see these: a nested try_table sets up its own
    o->tryClauses    = ਨਿਰਾ;
    o->ਅੰਕ_Try_Clauses = 0;

    ਲਈ (u32 i = 0; i < ਅੰਕ_Clauses; ++i) {
        M3CatchClause* clause = &clauses[i];

        IM3CompilationScope scope;
_       (Get_ਖੰਡ_Scope(o, &scope, clause->label_ਡੂੰਘਾਈ + 1));

        IM3FuncType payload = clause->ਟੈਗ ? clause->ਟੈਗ->type : ਨਿਰਾ;
        u16         ਅੰਕ_ਦਲੀਲਾਂ = Get_Func_Type_ਅੰਕ_Params(payload);

        // What the stub hands the label - the payload, then the exnref for a
        // _ref clause - has to be what the label takes. ResolveBlockResults
        // only counts the values, and the validator compares base types, so a
        // typed reference is only checked here: (ref null $t) reaching a label
        // that takes (ref $t) has to be rejected.
        {
            bool isLoop         = (scope->opcode == c_waOp_loop);
            u16  ਅੰਕ_Label_ਮੁੱਲਾਂ = isLoop ? Get_Func_Type_ਅੰਕ_Params(scope->type)
                                         : Get_Func_Type_ਅੰਕ_Results(scope->type);

            _throwif(m3_Err_type_ਗਿਣਤੀ_Mismatch, ਅੰਕ_Label_ਮੁੱਲਾਂ != ਅੰਕ_ਦਲੀਲਾਂ + (clause->hasRef ? 1 : 0));

            ਲਈ (u16 a = 0; a < ਅੰਕ_Label_ਮੁੱਲਾਂ; ++a) {
                m3type_t given  = (a < ਅੰਕ_ਦਲੀਲਾਂ) ? GetFuncTypeParamType(payload, a)
                                                : (m3type_t)c_m3Type_exnref;
                m3type_t wanted = isLoop ? GetFuncTypeParamType(scope->type, a)
                                         : Get_Func_Type_ਨਤੀਜਾ_Type(scope->type, a);

                _throwif(m3Err_typeMismatch, not Is_Sub_Type_ਦਾ(given, wanted));
            }
        }

        IM3_ਕੋਡ_ਪੰਨਾ stub_ਪੰਨਾ;
_       (Acquire_Compilation_ਕੋਡ_ਪੰਨਾ(o, &stub_ਪੰਨਾ));

        pc_t ਸ਼ੁਰੂ_PC  = Get_ਪੰਨਾ_PC(stub_ਪੰਨਾ);
        displaced_ਪੰਨਾ = o->ਪੰਨਾ;
        o->ਪੰਨਾ       = stub_ਪੰਨਾ;

        u16 ਪਹਿਲਾ_ਸੂਚਕਾਂਕ = o->ਢੇਰ_ਸੂਚਕਾਂਕ;

        ਲਈ (u16 a = 0; a < ਅੰਕ_ਦਲੀਲਾਂ; ++a) {
_           (ਧੱਕੋ_Allocated_ਥਾਂ(o, GetFuncTypeParamType(payload, a)));
        }

        ਜੇ (clause->hasRef) {
_           (ਧੱਕੋ_Allocated_ਥਾਂ(o, c_m3Type_exnref));
        }

        // emitted even with nothing to move: this is also where an exception
        // no clause will reify gets released
_       (Ensure_ਕੋਡ_ਪੰਨਾ_ਅੰਕ_ਸਤਰਾਂ(o, 2 * ਅੰਕ_ਦਲੀਲਾਂ + 3 + d_m3_ਕੋਡ_ਪੰਨਾ_Free_ਸਤਰਾਂ_ਹੱਦ));

_       (EmitOp(o, op_CatchPayload));
        EmitConstant32(o, ਅੰਕ_ਦਲੀਲਾਂ);

        ਲਈ (u16 a = 0; a < ਅੰਕ_ਦਲੀਲਾਂ; ++a) {
            u16 index = ਪਹਿਲਾ_ਸੂਚਕਾਂਕ + a;
            Emit_ਥਾਂ_ਵਿੱਥ(o, Get_ਥਾਂ_For_ਢੇਰ_ਸੂਚਕਾਂਕ(o, index));
            EmitConstant32(o, Is64_ਦੋਆਧਾਰੀ_ਅੰਕ_Type(Get_ਢੇਰ_Type_From_Bottom(o, index)));
        }

        Emit_ਥਾਂ_ਵਿੱਥ(o, clause->hasRef ? (i32)Get_ਥਾਂ_For_ਢੇਰ_ਸੂਚਕਾਂਕ(o, ਪਹਿਲਾ_ਸੂਚਕਾਂਕ + ਅੰਕ_ਦਲੀਲਾਂ) : -1);

_       (Emit_ਕੱਢੋ_Try_ਚੌਖਟੇ(o, ਅੰਕ_Try_ਚੌਖਟੇ_ਮੰਜ਼ਿਲ_ਕੱਢੋ(o->ਖੰਡ.outer, scope)));

        ਜੇ (scope->opcode == c_waOp_loop) {
_           (Resolve_ਖੰਡ_Results(o, scope, /* isBranch: */ true));

_           (EmitOp(o, Op_ContinueLoop(o)));
            Emit_ਸੂਚਕ(o, scope->pc);
_           (ਰਿਕਾਰਡ_ਪਿੱਠ_ਕਿਨਾਰਾ(o, scope, false));
        } ਨਹੀਂਤਾਂ ਜੇ (scope->ਡੂੰਘਾਈ == 0) {
_           (Return_ਮੁੱਲਾਂ(o, scope, /* isBranch: */ true));
_           (EmitOp(o, op_Return));
        } ਨਹੀਂਤਾਂ {
_           (Resolve_ਖੰਡ_Results(o, scope, /* isBranch: */ true));
_           (EmitPatchingBranch(o, scope));
        }

        IM3_ਕੋਡ_ਪੰਨਾ return_ਮੰਜ਼ਿਲ = displaced_ਪੰਨਾ;
        displaced_ਪੰਨਾ        = ਨਿਰਾ;

        Release_Compilation_ਕੋਡ_ਪੰਨਾ(o);
        o->ਪੰਨਾ = return_ਮੰਜ਼ਿਲ;

        *(pc_t*)clause->stub_ਥਾਂ = ਸ਼ੁਰੂ_PC;

        ਜਦੋਂ (o->ਢੇਰ_ਸੂਚਕਾਂਕ > ਪਹਿਲਾ_ਸੂਚਕਾਂਕ) {
_           (ਕੱਢੋ(o));
        }
    }

} _catch:

    // thrown with a stub page installed: release it and put back the one it
    // displaced, which the caller's catch then releases
    ਜੇ (displaced_ਪੰਨਾ) {
        Release_Compilation_ਕੋਡ_ਪੰਨਾ(o);
        o->ਪੰਨਾ = displaced_ਪੰਨਾ;
    }

    ਵਾਪਸ ਨਤੀਜਾ;
}


ਸਥਿਰ
M3_ਨਤੀਜਾ Compile_Try_ਸਾਰਣੀ (IM3Compilation o, m3opcode_t i_opcode)
{
    M3CatchClause* clauses = ਨਿਰਾ;
_try {
    // a catch entry has to be able to assume the operand stack is entirely in
    // slots: an unwind leaves nothing in the registers
_   (PreserveRegisters(o));
_   (Preserve_ਦਲੀਲਾਂ_And_Locals(o));

    IM3FuncType ਖੰਡ_Type = ਨਿਰਾ;
_   (Read_ਖੰਡ_Type(o, &ਖੰਡ_Type));

    u32 ਅੰਕ_Clauses;
_   (ReadLEB_u32(&ਅੰਕ_Clauses, &o->wasm, o->wasm_ਸਿਰਾ));

    _throwif("too many catch clauses", ਅੰਕ_Clauses > d_m3_Max_Sane_ਟੈਗ_ਸੂਚੀ_ਗਿਣਤੀ);

    ਜੇ (ਅੰਕ_Clauses) {
        clauses = m3_AllocArray(M3CatchClause, ਅੰਕ_Clauses);
        _throwifnull(clauses);
    }

    ਲਈ (u32 i = 0; i < ਅੰਕ_Clauses; ++i) {
        u8 ਕਿਸਮ;
_       (Read_u8(&ਕਿਸਮ, &o->wasm, o->wasm_ਸਿਰਾ));
        _throwif(m3Err_wasmMalformed, ਕਿਸਮ > 0x03);

        clauses[i].ਟੈਗ    = ਨਿਰਾ;
        clauses[i].hasRef = (ਕਿਸਮ == 0x01 or ਕਿਸਮ == 0x03);

        ਜੇ (ਕਿਸਮ == 0x00 or ਕਿਸਮ == 0x01) {
            u32 ਟੈਗ_ਸੂਚਕਾਂਕ;
_           (ReadLEB_u32(&ਟੈਗ_ਸੂਚਕਾਂਕ, &o->wasm, o->wasm_ਸਿਰਾ));
            _throwif(m3_Err_unknown_ਟੈਗ, ਟੈਗ_ਸੂਚਕਾਂਕ >= o->module->ਅੰਕ_ਟੈਗ_ਸੂਚੀ);

            IM3_ਟੈਗ ਟੈਗ     = &o->module->ਟੈਗ_ਸੂਚੀ[ਟੈਗ_ਸੂਚਕਾਂਕ];
            clauses[i].ਟੈਗ = ਟੈਗ->resolved ? ਟੈਗ->resolved : ਟੈਗ;
        }

_       (ReadLEB_u32(&clauses[i].label_ਡੂੰਘਾਈ, &o->wasm, o->wasm_ਸਿਰਾ));
    }

    // op_TryTable, the clause count, and two words per clause, all of which have
    // to land on one page: the op reads the table by offset from its own pc
_   (Ensure_ਕੋਡ_ਪੰਨਾ_ਅੰਕ_ਸਤਰਾਂ(o, 2 * ਅੰਕ_Clauses + 2 + d_m3_ਕੋਡ_ਪੰਨਾ_Free_ਸਤਰਾਂ_ਹੱਦ));

_   (EmitOp(o, op_Try_ਸਾਰਣੀ));
    EmitConstant32(o, ਅੰਕ_Clauses);

    ਲਈ (u32 i = 0; i < ਅੰਕ_Clauses; ++i) {
        Emit_ਸੂਚਕ(o, clauses[i].ਟੈਗ);
        clauses[i].stub_ਥਾਂ = Reserve_ਸੂਚਕ(o);
    }

    o->tryClauses    = clauses;
    o->ਅੰਕ_Try_Clauses = ਅੰਕ_Clauses;

_   (Compile_ਖੰਡ(o, ਖੰਡ_Type, c_wa_Op_try_ਸਾਰਣੀ));

} _catch:

    o->tryClauses    = ਨਿਰਾ;
    o->ਅੰਕ_Try_Clauses = 0;

    m3_Free(clauses);

    ਵਾਪਸ ਨਤੀਜਾ;
}


ਸਥਿਰ
M3_ਨਤੀਜਾ Compile_Throw (IM3Compilation o, m3opcode_t i_opcode)
{
_try {
    u32 ਟੈਗ_ਸੂਚਕਾਂਕ;
_   (ReadLEB_u32(&ਟੈਗ_ਸੂਚਕਾਂਕ, &o->wasm, o->wasm_ਸਿਰਾ));
    _throwif(m3_Err_unknown_ਟੈਗ, ਟੈਗ_ਸੂਚਕਾਂਕ >= o->module->ਅੰਕ_ਟੈਗ_ਸੂਚੀ);

    IM3_ਟੈਗ ਟੈਗ = &o->module->ਟੈਗ_ਸੂਚੀ[ਟੈਗ_ਸੂਚਕਾਂਕ];
    ਜੇ (ਟੈਗ->resolved) {
        ਟੈਗ = ਟੈਗ->resolved;
    }

    u16 ਅੰਕ_ਦਲੀਲਾਂ = Get_Func_Type_ਅੰਕ_Params(ਟੈਗ->type);

    // the payload is read out of slots, so nothing may still be in a register
_   (PreserveRegisters(o));

    // check the payload before emitting anything, so a mistyped throw does not
    // leave half an operation behind
    bool isPolymorphic = Is_ਢੇਰ_Polymorphic(o);

    ਜੇ (not isPolymorphic) {
        _throwif(m3_Err_type_ਗਿਣਤੀ_Mismatch, Get_ਅੰਕ_ਖੰਡ_ਮੁੱਲਾਂ_On_ਢੇਰ(o) < ਅੰਕ_ਦਲੀਲਾਂ);

        ਲਈ (u16 i = 0; i < ਅੰਕ_ਦਲੀਲਾਂ; ++i) {
            u16 index = (u16)(o->ਢੇਰ_ਸੂਚਕਾਂਕ - ਅੰਕ_ਦਲੀਲਾਂ + i);

            _throwif(m3Err_typeMismatch, not Is_Sub_Type_ਦਾ(Get_ਢੇਰ_Type_From_Bottom(o, index),
                                                         GetFuncTypeParamType(ਟੈਗ->type, i)));
        }
    }

_   (Ensure_ਕੋਡ_ਪੰਨਾ_ਅੰਕ_ਸਤਰਾਂ(o, 2 * ਅੰਕ_ਦਲੀਲਾਂ + 3 + d_m3_ਕੋਡ_ਪੰਨਾ_Free_ਸਤਰਾਂ_ਹੱਦ));

_   (EmitOp(o, op_Throw));
    Emit_ਸੂਚਕ(o, ਟੈਗ);
    EmitConstant32(o, ਅੰਕ_ਦਲੀਲਾਂ);

    // the payload is named in the order the tag declares it, so the deepest of
    // the arguments on the stack comes first. In unreachable code there is
    // nothing on the stack to name and nothing will run this, so slot 0 stands
    // in for an operand that isn't there.
    ਲਈ (u16 i = 0; i < ਅੰਕ_ਦਲੀਲਾਂ; ++i) {
        m3type_t type = GetFuncTypeParamType(ਟੈਗ->type, i);

        Emit_ਥਾਂ_ਵਿੱਥ(o, isPolymorphic ? 0 : Get_ਥਾਂ_For_ਢੇਰ_ਸੂਚਕਾਂਕ(o, (u16)(o->ਢੇਰ_ਸੂਚਕਾਂਕ - ਅੰਕ_ਦਲੀਲਾਂ + i)));
        EmitConstant32(o, Is64_ਦੋਆਧਾਰੀ_ਅੰਕ_Type(type));
    }

    ਜੇ (not isPolymorphic) {
        ਲਈ (u16 i = 0; i < ਅੰਕ_ਦਲੀਲਾਂ; ++i) {
_           (ਕੱਢੋ(o));
        }
    }

_   (ਸੈੱਟ_ਢੇਰ_Polymorphic(o));

} _catch: ਵਾਪਸ ਨਤੀਜਾ;
}


ਸਥਿਰ
M3_ਨਤੀਜਾ Compile_ThrowRef (IM3Compilation o, m3opcode_t i_opcode)
{
_try {
    ਜੇ (not Is_ਢੇਰ_Polymorphic(o)) {
_       (Preserve_ਰਜਿਸਟਰ_If_Occupied(o, c_m3Type_i64));

        _throwif(m3Err_typeMismatch, ਅਧਾਰ_Type_ਦਾ(Get_ਢੇਰ_ਸਿਖਰ_Type(o)) != c_m3Type_exnref);

_       (EmitOp(o, op_ThrowRef));
        Emit_ਥਾਂ_ਵਿੱਥ(o, Get_ਢੇਰ_ਸਿਖਰ_ਥਾਂ_ਸੰਖਿਆ(o));

_       (ਕੱਢੋ(o));
    }

_   (ਸੈੱਟ_ਢੇਰ_Polymorphic(o));

} _catch: ਵਾਪਸ ਨਤੀਜਾ;
}

#ਜੇ_ਅੰਤ // d_m3HasExceptionHandling

#ਜੇ d_m3_Has_ਢੇਰ_Switching

ਸਥਿਰ
M3_ਨਤੀਜਾ Compile_ContNew (IM3Compilation o, m3opcode_t i_opcode)
{
_try {
    u32 type_ਸੂਚਕਾਂਕ;
_   (ReadLEB_u32(&type_ਸੂਚਕਾਂਕ, &o->wasm, o->wasm_ਸਿਰਾ));
    _throwif(m3_Err_type_ਗਿਣਤੀ_Mismatch, type_ਸੂਚਕਾਂਕ >= o->module->ਅੰਕ_Func_Types);

    IM3FuncType funcType = o->module->funcTypes[type_ਸੂਚਕਾਂਕ];
    _throwif("type is not a continuation", not funcType->isContinuation);

    // the operands are read out of slots, so nothing may still be in a register
_   (PreserveRegisters(o));

    // cont.new $ct : [(ref null $ft)] -> [(ref $ct)], where $ct ~~ cont $ft:
    // the function has to be one this continuation type can run
    m3type_t ਸਿਖਰ_Type = Get_ਢੇਰ_ਸਿਖਰ_Type(o);
    _throwif(m3Err_typeMismatch,
             not Is_Sub_Type_ਦਾ(ਸਿਖਰ_Type, Ref_Type_ਦਾ_Func_Type(funcType->contFuncType, false)));

    u16 func_ਥਾਂ = Get_ਢੇਰ_ਸਿਖਰ_ਥਾਂ_ਸੰਖਿਆ(o);
_   (ਕੱਢੋ(o));

    u16 dst_ਥਾਂ;
_   (ਧੱਕੋ_Allocated_ਥਾਂ(o, Ref_Type_ਦਾ_Func_Type(funcType, true)));
    dst_ਥਾਂ = Get_ਢੇਰ_ਸਿਖਰ_ਥਾਂ_ਸੰਖਿਆ(o);

_   (Ensure_ਕੋਡ_ਪੰਨਾ_ਅੰਕ_ਸਤਰਾਂ(o, 4 + d_m3_ਕੋਡ_ਪੰਨਾ_Free_ਸਤਰਾਂ_ਹੱਦ));
_   (EmitOp(o, op_ContNew));
    Emit_ਸੂਚਕ(o, funcType);
    Emit_ਥਾਂ_ਵਿੱਥ(o, dst_ਥਾਂ);
    Emit_ਥਾਂ_ਵਿੱਥ(o, func_ਥਾਂ);

} _catch: ਵਾਪਸ ਨਤੀਜਾ;
}

ਸਥਿਰ
M3_ਨਤੀਜਾ Compile_ContBind (IM3Compilation o, m3opcode_t i_opcode)
{
_try {
    u32 typeIndex1, typeIndex2;
_   (ReadLEB_u32(&typeIndex1, &o->wasm, o->wasm_ਸਿਰਾ));
_   (ReadLEB_u32(&typeIndex2, &o->wasm, o->wasm_ਸਿਰਾ));
    _throwif(m3_Err_type_ਗਿਣਤੀ_Mismatch, typeIndex1 >= o->module->ਅੰਕ_Func_Types);
    _throwif(m3_Err_type_ਗਿਣਤੀ_Mismatch, typeIndex2 >= o->module->ਅੰਕ_Func_Types);

    IM3FuncType ct1 = o->module->funcTypes[typeIndex1];
    IM3FuncType ct2 = o->module->funcTypes[typeIndex2];
    _throwif("type is not a continuation", not ct1->isContinuation || not ct2->isContinuation);

    // the operands are read out of slots, so nothing may still be in a register
_   (PreserveRegisters(o));

    m3type_t contType = Get_ਢੇਰ_ਸਿਖਰ_Type(o);
    _throwif(m3Err_typeMismatch, not Is_Sub_Type_ਦਾ(contType, Ref_Type_ਦਾ_Func_Type(ct1, false)));
    u16 cont_ਥਾਂ = Get_ਢੇਰ_ਸਿਖਰ_ਥਾਂ_ਸੰਖਿਆ(o);
_   (ਕੱਢੋ(o));

    IM3FuncType ft1 = ct1->contFuncType ? ct1->contFuncType : ct1;
    IM3FuncType ft2 = ct2->contFuncType ? ct2->contFuncType : ct2;

    u16 ਅੰਕ_Params1 = Get_Func_Type_ਅੰਕ_Params(ft1);
    u16 ਅੰਕ_Params2 = Get_Func_Type_ਅੰਕ_Params(ft2);
    _throwif(m3Err_typeMismatch, ਅੰਕ_Params1 < ਅੰਕ_Params2);
    u32 ਅੰਕ_Bound = ਅੰਕ_Params1 - ਅੰਕ_Params2;

    u16 ਅੰਕ_Results1 = Get_Func_Type_ਅੰਕ_Results(ft1);
    u16 ਅੰਕ_Results2 = Get_Func_Type_ਅੰਕ_Results(ft2);
    _throwif(m3Err_typeMismatch, ਅੰਕ_Results1 != ਅੰਕ_Results2);
    ਲਈ (u16 i = 0; i < ਅੰਕ_Results1; ++i) {
        _throwif(m3Err_typeMismatch, Get_Func_Type_ਨਤੀਜਾ_Type(ft1, i) != Get_Func_Type_ਨਤੀਜਾ_Type(ft2, i));
    }
    ਲਈ (u16 i = 0; i < ਅੰਕ_Params2; ++i) {
        _throwif(m3Err_typeMismatch, GetFuncTypeParamType(ft1, ਅੰਕ_Bound + i) != GetFuncTypeParamType(ft2, i));
    }

    u16 argSlots[d_m3MaxContinuationPayload];
    _throwif("too many bound args", ਅੰਕ_Bound > d_m3MaxContinuationPayload);

    ਲਈ (u32 i = 0; i < ਅੰਕ_Bound; ++i) {
        u32      paramIdx     = ਅੰਕ_Bound - 1 - i;
        m3type_t expectedType = GetFuncTypeParamType(ft1, paramIdx);
        m3type_t actualType   = Get_ਢੇਰ_ਸਿਖਰ_Type(o);
        _throwif(m3Err_typeMismatch, not Is_Sub_Type_ਦਾ(actualType, expectedType));
        argSlots[paramIdx] = Get_ਢੇਰ_ਸਿਖਰ_ਥਾਂ_ਸੰਖਿਆ(o);
_       (ਕੱਢੋ(o));
    }

    u16 dst_ਥਾਂ;
_   (ਧੱਕੋ_Allocated_ਥਾਂ(o, Ref_Type_ਦਾ_Func_Type(ct2, true)));
    dst_ਥਾਂ = Get_ਢੇਰ_ਸਿਖਰ_ਥਾਂ_ਸੰਖਿਆ(o);

_   (Ensure_ਕੋਡ_ਪੰਨਾ_ਅੰਕ_ਸਤਰਾਂ(o, 6 + ਅੰਕ_Bound * 2 + d_m3_ਕੋਡ_ਪੰਨਾ_Free_ਸਤਰਾਂ_ਹੱਦ));
_   (EmitOp(o, op_ContBind));
    Emit_ਸੂਚਕ(o, ct1);
    Emit_ਸੂਚਕ(o, ct2);
    Emit_ਥਾਂ_ਵਿੱਥ(o, dst_ਥਾਂ);
    Emit_ਥਾਂ_ਵਿੱਥ(o, cont_ਥਾਂ);
    EmitConstant32(o, ਅੰਕ_Bound);
    ਲਈ (u32 i = 0; i < ਅੰਕ_Bound; ++i) {
        Emit_ਥਾਂ_ਵਿੱਥ(o, argSlots[i]);
        EmitConstant32(o, Is64_ਦੋਆਧਾਰੀ_ਅੰਕ_Type(GetFuncTypeParamType(ft1, i)));
    }

} _catch: ਵਾਪਸ ਨਤੀਜਾ;
}

ਸਥਿਰ
M3_ਨਤੀਜਾ Compile_Suspend (IM3Compilation o, m3opcode_t i_opcode)
{
_try {
    u32 ਟੈਗ_ਸੂਚਕਾਂਕ;
_   (ReadLEB_u32(&ਟੈਗ_ਸੂਚਕਾਂਕ, &o->wasm, o->wasm_ਸਿਰਾ));
    _throwif(m3_Err_unknown_ਟੈਗ, ਟੈਗ_ਸੂਚਕਾਂਕ >= o->module->ਅੰਕ_ਟੈਗ_ਸੂਚੀ);

    IM3_ਟੈਗ ਟੈਗ = &o->module->ਟੈਗ_ਸੂਚੀ[ਟੈਗ_ਸੂਚਕਾਂਕ];
    ਜੇ (ਟੈਗ->resolved) {
        ਟੈਗ = ਟੈਗ->resolved;
    }
    u16 ਅੰਕ_Params  = Get_Func_Type_ਅੰਕ_Params(ਟੈਗ->type);
    u16 ਅੰਕ_Results = Get_Func_Type_ਅੰਕ_Results(ਟੈਗ->type);

_   (PreserveRegisters(o));

    u16 paramSlots[d_m3MaxContinuationPayload];
    _throwif("too many tag params", ਅੰਕ_Params > d_m3MaxContinuationPayload);
    ਲਈ (u16 i = 0; i < ਅੰਕ_Params; ++i) {
        u16      pIdx         = ਅੰਕ_Params - 1 - i;
        m3type_t expectedType = GetFuncTypeParamType(ਟੈਗ->type, pIdx);
        m3type_t actualType   = Get_ਢੇਰ_ਸਿਖਰ_Type(o);
        _throwif(m3Err_typeMismatch, not Is_Sub_Type_ਦਾ(actualType, expectedType));
        paramSlots[pIdx] = Get_ਢੇਰ_ਸਿਖਰ_ਥਾਂ_ਸੰਖਿਆ(o);
_       (ਕੱਢੋ(o));
    }

    u16 ਨਤੀਜਾ_Slots[d_m3MaxContinuationPayload];
    _throwif("too many tag results", ਅੰਕ_Results > d_m3MaxContinuationPayload);
    ਲਈ (u16 i = 0; i < ਅੰਕ_Results; ++i) {
        m3type_t rtype = Get_Func_Type_ਨਤੀਜਾ_Type(ਟੈਗ->type, i);
_       (ਧੱਕੋ_Allocated_ਥਾਂ(o, rtype));
        ਨਤੀਜਾ_Slots[i] = Get_ਢੇਰ_ਸਿਖਰ_ਥਾਂ_ਸੰਖਿਆ(o);
    }

_   (Ensure_ਕੋਡ_ਪੰਨਾ_ਅੰਕ_ਸਤਰਾਂ(o, 4 + ਅੰਕ_Params * 2 + ਅੰਕ_Results * 2 + d_m3_ਕੋਡ_ਪੰਨਾ_Free_ਸਤਰਾਂ_ਹੱਦ));
_   (EmitOp(o, op_Suspend));
    Emit_ਸੂਚਕ(o, ਟੈਗ);
    EmitConstant32(o, ਅੰਕ_Params);
    ਲਈ (u16 i = 0; i < ਅੰਕ_Params; ++i) {
        Emit_ਥਾਂ_ਵਿੱਥ(o, paramSlots[i]);
        EmitConstant32(o, Is64_ਦੋਆਧਾਰੀ_ਅੰਕ_Type(GetFuncTypeParamType(ਟੈਗ->type, i)));
    }
    EmitConstant32(o, ਅੰਕ_Results);
    ਲਈ (u16 i = 0; i < ਅੰਕ_Results; ++i) {
        Emit_ਥਾਂ_ਵਿੱਥ(o, ਨਤੀਜਾ_Slots[i]);
        EmitConstant32(o, Is64_ਦੋਆਧਾਰੀ_ਅੰਕ_Type(Get_Func_Type_ਨਤੀਜਾ_Type(ਟੈਗ->type, i)));
    }

    // the results are part of the frame here: op_Suspend clears them, and
    // whatever resumes this writes them before anything reads them
_   (ਰਿਕਾਰਡ_ਸੁਰੱਖਿਅਤ_ਬਿੰਦੂ(o, safepoint_suspend, 0, ਅੰਕ_Results, 0, ਨਿਰਾ));

} _catch: ਵਾਪਸ ਨਤੀਜਾ;
}

// resume, resume_throw and resume_throw_ref. All three run a continuation
// under a handler table and take its results; they differ only in what they
// hand it to start with - arguments, a fresh exception, or one off the stack.
//
// Emitted as: the continuation type, the slot holding it, the operands that
// belong to this form, the handler table, then the result slots. Keeping the
// tail identical is what lets one runtime path finish all three.
ਸਥਿਰ
M3_ਨਤੀਜਾ Compile_Resume (IM3Compilation o, m3opcode_t i_opcode)
{
    M3ResumeHandler* handlers      = ਨਿਰਾ;
    IM3_ਕੋਡ_ਪੰਨਾ      displaced_ਪੰਨਾ = ਨਿਰਾ;
_try {
    bool isThrow    = (i_opcode == c_waOp_resumeThrow);
    bool isThrowRef = (i_opcode == c_waOp_resumeThrowRef);

    u32 type_ਸੂਚਕਾਂਕ;
_   (ReadLEB_u32(&type_ਸੂਚਕਾਂਕ, &o->wasm, o->wasm_ਸਿਰਾ));
    _throwif(m3_Err_type_ਗਿਣਤੀ_Mismatch, type_ਸੂਚਕਾਂਕ >= o->module->ਅੰਕ_Func_Types);

    IM3FuncType contFuncType = o->module->funcTypes[type_ਸੂਚਕਾਂਕ];
    _throwif("type is not a continuation", not contFuncType->isContinuation);

    IM3FuncType innerType      = contFuncType->contFuncType;
    u16         ਅੰਕ_Cont_Results = Get_Func_Type_ਅੰਕ_Results(innerType);

    IM3_ਟੈਗ exn_ਟੈਗ      = ਨਿਰਾ;
    u16    ਅੰਕ_Operands = 0;

    ਜੇ (isThrow) {
        u32 ਟੈਗ_ਸੂਚਕਾਂਕ;
_       (ReadLEB_u32(&ਟੈਗ_ਸੂਚਕਾਂਕ, &o->wasm, o->wasm_ਸਿਰਾ));
        _throwif(m3_Err_unknown_ਟੈਗ, ਟੈਗ_ਸੂਚਕਾਂਕ >= o->module->ਅੰਕ_ਟੈਗ_ਸੂਚੀ);

        exn_ਟੈਗ = &o->module->ਟੈਗ_ਸੂਚੀ[ਟੈਗ_ਸੂਚਕਾਂਕ];

        // it is an exception that gets raised, so the tag must be one throw
        // could have raised: no results
        _throwif(m3Err_typeMismatch, Get_Func_Type_ਅੰਕ_Results(exn_ਟੈਗ->type) != 0);

        ਅੰਕ_Operands = Get_Func_Type_ਅੰਕ_Params(exn_ਟੈਗ->type);
    } ਨਹੀਂਤਾਂ ਜੇ (not isThrowRef) {
        // a plain resume hands over the continuation's own parameters. An
        // aborted one never receives them, so they are unconstrained there.
        ਅੰਕ_Operands = Get_Func_Type_ਅੰਕ_Params(innerType);
    }

    u32 ਅੰਕ_Clauses;
_   (ReadLEB_u32(&ਅੰਕ_Clauses, &o->wasm, o->wasm_ਸਿਰਾ));

    u32 clauseLabelDepths[16];
    _throwif("too many resume clauses", ਅੰਕ_Clauses > 16);

    ਜੇ (ਅੰਕ_Clauses) {
        handlers = m3_AllocArray(M3ResumeHandler, ਅੰਕ_Clauses);
        _throwifnull(handlers);
    }

    ਲਈ (u32 i = 0; i < ਅੰਕ_Clauses; ++i) {
        u8 ਕਿਸਮ;
_       (Read_u8(&ਕਿਸਮ, &o->wasm, o->wasm_ਸਿਰਾ));
        _throwif(m3Err_wasmMalformed, ਕਿਸਮ > 0x01);

        handlers[i].ਕਿਸਮ = ਕਿਸਮ;

        u32 ਟੈਗ_ਸੂਚਕਾਂਕ;
_       (ReadLEB_u32(&ਟੈਗ_ਸੂਚਕਾਂਕ, &o->wasm, o->wasm_ਸਿਰਾ));
        _throwif(m3_Err_unknown_ਟੈਗ, ਟੈਗ_ਸੂਚਕਾਂਕ >= o->module->ਅੰਕ_ਟੈਗ_ਸੂਚੀ);

        IM3_ਟੈਗ ਟੈਗ         = &o->module->ਟੈਗ_ਸੂਚੀ[ਟੈਗ_ਸੂਚਕਾਂਕ];
        handlers[i].ਟੈਗ    = ਟੈਗ->resolved ? ਟੈਗ->resolved : ਟੈਗ;
        handlers[i].stubPC = ਨਿਰਾ;

        ਜੇ (ਕਿਸਮ == 0x00) {
_           (ReadLEB_u32(&clauseLabelDepths[i], &o->wasm, o->wasm_ਸਿਰਾ));
        } ਨਹੀਂਤਾਂ {
            // (on $e switch): the tag carries no payload, and what the switch
            // site yields is what this resume yields
            _throwif(m3Err_typeMismatch, Get_Func_Type_ਅੰਕ_Params(handlers[i].ਟੈਗ->type) != 0);
            clauseLabelDepths[i] = 0;
        }
    }

    // the operands are read out of slots, so nothing may still be in a register
_   (PreserveRegisters(o));

    m3type_t cont_ਸਿਖਰ_Type = Get_ਢੇਰ_ਸਿਖਰ_Type(o);
    _throwif(m3Err_typeMismatch, not Is_Sub_Type_ਦਾ(cont_ਸਿਖਰ_Type, Ref_Type_ਦਾ_Func_Type(contFuncType, false)));
    u16 cont_ਥਾਂ = Get_ਢੇਰ_ਸਿਖਰ_ਥਾਂ_ਸੰਖਿਆ(o);
_   (ਕੱਢੋ(o));

    u16 exn_ਥਾਂ = 0;

    ਜੇ (isThrowRef) {
        m3type_t exnType = Get_ਢੇਰ_ਸਿਖਰ_Type(o);
        _throwif(m3Err_typeMismatch, ਅਧਾਰ_Type_ਦਾ(exnType) != c_m3Type_exnref);
        exn_ਥਾਂ = Get_ਢੇਰ_ਸਿਖਰ_ਥਾਂ_ਸੰਖਿਆ(o);
_       (ਕੱਢੋ(o));
    }

    u16 ਸੰਕਾਰਜ_Slots[d_m3MaxContinuationPayload];
    _throwif("too many resume operands", ਅੰਕ_Operands > d_m3MaxContinuationPayload);

    IM3FuncType ਸੰਕਾਰਜ_Type = isThrow ? exn_ਟੈਗ->type : innerType;

    ਲਈ (u16 i = 0; i < ਅੰਕ_Operands; ++i) {
        u16      pIdx         = (u16)(ਅੰਕ_Operands - 1 - i);
        m3type_t expectedType = GetFuncTypeParamType(ਸੰਕਾਰਜ_Type, pIdx);
        m3type_t actualType   = Get_ਢੇਰ_ਸਿਖਰ_Type(o);
        _throwif(m3Err_typeMismatch, not Is_Sub_Type_ਦਾ(actualType, expectedType));
        ਸੰਕਾਰਜ_Slots[pIdx] = Get_ਢੇਰ_ਸਿਖਰ_ਥਾਂ_ਸੰਖਿਆ(o);
_       (ਕੱਢੋ(o));
    }

    ਲਈ (u32 i = 0; i < ਅੰਕ_Clauses; ++i) {
        ਜੇ (handlers[i].ਕਿਸਮ != 0x00) {
            ਜਾਰੀ;
        }

        IM3CompilationScope scope;
_       (Get_ਖੰਡ_Scope(o, &scope, clauseLabelDepths[i]));

        IM3_ਟੈਗ ਟੈਗ        = handlers[i].ਟੈਗ;
        u16    ਅੰਕ_Payload = Get_Func_Type_ਅੰਕ_Params(ਟੈਗ->type);

        m3type_t labelContType = (scope->opcode == c_waOp_loop)
                                   ? GetFuncTypeParamType(scope->type, ਅੰਕ_Payload)
                                   : Get_Func_Type_ਨਤੀਜਾ_Type(scope->type, ਅੰਕ_Payload);

        IM3_ਕੋਡ_ਪੰਨਾ stub_ਪੰਨਾ;
_       (Acquire_Compilation_ਕੋਡ_ਪੰਨਾ(o, &stub_ਪੰਨਾ));
        pc_t ਸ਼ੁਰੂ_PC  = Get_ਪੰਨਾ_PC(stub_ਪੰਨਾ);
        displaced_ਪੰਨਾ = o->ਪੰਨਾ;
        o->ਪੰਨਾ       = stub_ਪੰਨਾ;

        u16 ਪਹਿਲਾ_ਸੂਚਕਾਂਕ = o->ਢੇਰ_ਸੂਚਕਾਂਕ;
        ਲਈ (u16 a = 0; a < ਅੰਕ_Payload; ++a) {
_           (ਧੱਕੋ_Allocated_ਥਾਂ(o, GetFuncTypeParamType(ਟੈਗ->type, a)));
        }

        // The handler is handed the suspended continuation.
_       (ਧੱਕੋ_Allocated_ਥਾਂ(o, labelContType));

_       (Ensure_ਕੋਡ_ਪੰਨਾ_ਅੰਕ_ਸਤਰਾਂ(o, 2 * ਅੰਕ_Payload + 5 + d_m3_ਕੋਡ_ਪੰਨਾ_Free_ਸਤਰਾਂ_ਹੱਦ));
_       (EmitOp(o, op_ResumePayload));
        Emit_ਸੂਚਕ(o, Module_Cont_Type_ਦਾ_Ref(o->module, labelContType));
        EmitConstant32(o, ਅੰਕ_Payload);
        ਲਈ (u16 a = 0; a < ਅੰਕ_Payload; ++a) {
            u16 index = ਪਹਿਲਾ_ਸੂਚਕਾਂਕ + a;
            Emit_ਥਾਂ_ਵਿੱਥ(o, Get_ਥਾਂ_For_ਢੇਰ_ਸੂਚਕਾਂਕ(o, index));
            EmitConstant32(o, Is64_ਦੋਆਧਾਰੀ_ਅੰਕ_Type(Get_ਢੇਰ_Type_From_Bottom(o, index)));
        }
        Emit_ਥਾਂ_ਵਿੱਥ(o, Get_ਥਾਂ_For_ਢੇਰ_ਸੂਚਕਾਂਕ(o, ਪਹਿਲਾ_ਸੂਚਕਾਂਕ + ਅੰਕ_Payload));

#  ਜੇ d_m3HasExceptionHandling
        // the clause leaves every try region between the resume and its label,
        // and leaving one retires its handlers, the way a branch does
_       (Emit_ਕੱਢੋ_Try_ਚੌਖਟੇ(o, ਅੰਕ_Try_ਚੌਖਟੇ_ਮੰਜ਼ਿਲ_ਕੱਢੋ(&o->ਖੰਡ, scope)));
#  endif

        ਜੇ (scope->opcode == c_waOp_loop) {
_           (Resolve_ਖੰਡ_Results(o, scope, true));
_           (EmitOp(o, Op_ContinueLoop(o)));
            Emit_ਸੂਚਕ(o, scope->pc);
_           (ਰਿਕਾਰਡ_ਪਿੱਠ_ਕਿਨਾਰਾ(o, scope, false));
        } ਨਹੀਂਤਾਂ ਜੇ (scope->ਡੂੰਘਾਈ == 0) {
_           (Return_ਮੁੱਲਾਂ(o, scope, true));
_           (EmitOp(o, op_Return));
        } ਨਹੀਂਤਾਂ {
_           (Resolve_ਖੰਡ_Results(o, scope, true));
_           (EmitPatchingBranch(o, scope));
        }

        IM3_ਕੋਡ_ਪੰਨਾ return_ਮੰਜ਼ਿਲ = displaced_ਪੰਨਾ;
        displaced_ਪੰਨਾ        = ਨਿਰਾ;

        Release_Compilation_ਕੋਡ_ਪੰਨਾ(o);
        o->ਪੰਨਾ = return_ਮੰਜ਼ਿਲ;

        handlers[i].stubPC = ਸ਼ੁਰੂ_PC;

        ਜਦੋਂ (o->ਢੇਰ_ਸੂਚਕਾਂਕ > ਪਹਿਲਾ_ਸੂਚਕਾਂਕ) {
_           (ਕੱਢੋ(o));
        }
    }

    u16 resSlots[d_m3MaxContinuationPayload];
    _throwif("too many cont results", ਅੰਕ_Cont_Results > d_m3MaxContinuationPayload);
    ਲਈ (u16 i = 0; i < ਅੰਕ_Cont_Results; ++i) {
        m3type_t rtype = Get_Func_Type_ਨਤੀਜਾ_Type(innerType, i);
_       (ਧੱਕੋ_Allocated_ਥਾਂ(o, rtype));
        resSlots[i] = Get_ਢੇਰ_ਸਿਖਰ_ਥਾਂ_ਸੰਖਿਆ(o);
    }

_   (Ensure_ਕੋਡ_ਪੰਨਾ_ਅੰਕ_ਸਤਰਾਂ(o, 8 + ਅੰਕ_Operands * 2 + ਅੰਕ_Clauses * 3 + ਅੰਕ_Cont_Results * 2 +
                                  d_m3_ਕੋਡ_ਪੰਨਾ_Free_ਸਤਰਾਂ_ਹੱਦ));

_   (EmitOp(o, op_Resume));
    Emit_ਸੂਚਕ(o, contFuncType);
    Emit_ਥਾਂ_ਵਿੱਥ(o, cont_ਥਾਂ);
    EmitConstant32(o, isThrow ? d_m3ResumeThrow : (isThrowRef ? d_m3ResumeThrowRef : d_m3_Resume_ਸਾਧਾਰਨ));

    ਜੇ (isThrowRef) {
        Emit_ਥਾਂ_ਵਿੱਥ(o, exn_ਥਾਂ);
    } ਨਹੀਂਤਾਂ {
        ਜੇ (isThrow) {
            Emit_ਸੂਚਕ(o, exn_ਟੈਗ);
        }

        EmitConstant32(o, ਅੰਕ_Operands);
        ਲਈ (u16 i = 0; i < ਅੰਕ_Operands; ++i) {
            Emit_ਥਾਂ_ਵਿੱਥ(o, ਸੰਕਾਰਜ_Slots[i]);
            EmitConstant32(o, Is64_ਦੋਆਧਾਰੀ_ਅੰਕ_Type(GetFuncTypeParamType(ਸੰਕਾਰਜ_Type, i)));
        }
    }

    EmitConstant32(o, ਅੰਕ_Clauses);
    ਲਈ (u32 i = 0; i < ਅੰਕ_Clauses; ++i) {
        EmitConstant32(o, handlers[i].ਕਿਸਮ);
        Emit_ਸੂਚਕ(o, handlers[i].ਟੈਗ);
        Emit_ਸੂਚਕ(o, handlers[i].stubPC);
    }

    EmitConstant32(o, ਅੰਕ_Cont_Results);
    ਲਈ (u16 i = 0; i < ਅੰਕ_Cont_Results; ++i) {
        Emit_ਥਾਂ_ਵਿੱਥ(o, resSlots[i]);
        EmitConstant32(o, Is64_ਦੋਆਧਾਰੀ_ਅੰਕ_Type(Get_Func_Type_ਨਤੀਜਾ_Type(innerType, i)));
    }

    // a suspend passing through the resumed continuation leaves this frame
    // waiting here, with the continuation's results not yet taken
_   (ਰਿਕਾਰਡ_ਸੁਰੱਖਿਅਤ_ਬਿੰਦੂ(o, safepoint_resume, 0, ਅੰਕ_Cont_Results, (u16)ਅੰਕ_Clauses, ਨਿਰਾ));
#  ਜੇ d_m3HasSnapshots
    ਜੇ (o->snapshotMap and o->ਪੰਨਾ) {
        o->snapshotMap->ਸੁਰੱਖਿਅਤ_Points[o->snapshotMap->ਅੰਕ_ਸੁਰੱਖਿਅਤ_Points - 1].resumeType = Ref_Type_ਦਾ_Func_Type(contFuncType, true);
    }
#  endif

} _catch:
    ਜੇ (displaced_ਪੰਨਾ) {
        Release_Compilation_ਕੋਡ_ਪੰਨਾ(o);
        o->ਪੰਨਾ = displaced_ਪੰਨਾ;
    }
    m3_Free(handlers);
    ਵਾਪਸ ਨਤੀਜਾ;
}


ਸਥਿਰ
M3_ਨਤੀਜਾ Compile_Switch (IM3Compilation o, m3opcode_t i_opcode)
{
_try {
    u32 type_ਸੂਚਕਾਂਕ, ਟੈਗ_ਸੂਚਕਾਂਕ;
_   (ReadLEB_u32(&type_ਸੂਚਕਾਂਕ, &o->wasm, o->wasm_ਸਿਰਾ));
_   (ReadLEB_u32(&ਟੈਗ_ਸੂਚਕਾਂਕ, &o->wasm, o->wasm_ਸਿਰਾ));
    _throwif(m3_Err_type_ਗਿਣਤੀ_Mismatch, type_ਸੂਚਕਾਂਕ >= o->module->ਅੰਕ_Func_Types);
    _throwif(m3_Err_unknown_ਟੈਗ, ਟੈਗ_ਸੂਚਕਾਂਕ >= o->module->ਅੰਕ_ਟੈਗ_ਸੂਚੀ);

    IM3FuncType ਨਿਸ਼ਾਨਾ_Cont_Type = o->module->funcTypes[type_ਸੂਚਕਾਂਕ];
    IM3_ਟੈਗ      ਟੈਗ            = &o->module->ਟੈਗ_ਸੂਚੀ[ਟੈਗ_ਸੂਚਕਾਂਕ];
    ਜੇ (ਟੈਗ->resolved) {
        ਟੈਗ = ਟੈਗ->resolved;
    }

    _throwif("type is not a continuation", not ਨਿਸ਼ਾਨਾ_Cont_Type->isContinuation);

    // a switch tag carries no payload of its own: the values travel as the
    // peer's arguments, and the tag's results are what the switch site yields
    _throwif(m3Err_typeMismatch, Get_Func_Type_ਅੰਕ_Params(ਟੈਗ->type) != 0);

    // switch $ct1 : [t1* (ref null $ct1)] -> [t2*]
    //   where $ct1 = cont [t1* (ref null? $ct2)] -> [..]
    //   and   $ct2 = cont [t2*] -> [..]
    //
    // The trailing parameter of $ct1 is not an operand: the peer receives the
    // continuation this instruction suspends. And what this instruction yields
    // is not $ct1's results but $ct2's parameters - the values whoever comes
    // back this way will hand over.
    IM3FuncType ਨਿਸ਼ਾਨਾ_Type = ਨਿਸ਼ਾਨਾ_Cont_Type->contFuncType;
    u16         ਅੰਕ_Params  = Get_Func_Type_ਅੰਕ_Params(ਨਿਸ਼ਾਨਾ_Type);

    _throwif("switch target takes no continuation", ਅੰਕ_Params == 0);

    m3type_t    peerRefType  = GetFuncTypeParamType(ਨਿਸ਼ਾਨਾ_Type, (u16)(ਅੰਕ_Params - 1));
    IM3FuncType selfContType = Module_Cont_Type_ਦਾ_Ref(o->module, peerRefType);

    _throwif(m3Err_typeMismatch, not selfContType);

    IM3FuncType selfType   = selfContType->contFuncType;
    u16         ਅੰਕ_ਦਲੀਲਾਂ    = (u16)(ਅੰਕ_Params - 1);
    u16         ਅੰਕ_Results = Get_Func_Type_ਅੰਕ_Params(selfType);

    // the operands are read out of slots, so nothing may still be in a register
_   (PreserveRegisters(o));

    m3type_t cont_ਸਿਖਰ_Type = Get_ਢੇਰ_ਸਿਖਰ_Type(o);
    _throwif(m3Err_typeMismatch, not Is_Sub_Type_ਦਾ(cont_ਸਿਖਰ_Type, Ref_Type_ਦਾ_Func_Type(ਨਿਸ਼ਾਨਾ_Cont_Type, false)));
    u16 ਨਿਸ਼ਾਨਾ_Cont_ਥਾਂ = Get_ਢੇਰ_ਸਿਖਰ_ਥਾਂ_ਸੰਖਿਆ(o);
_   (ਕੱਢੋ(o));

    u16 argSlots[d_m3MaxContinuationPayload];
    _throwif("too many switch params", ਅੰਕ_ਦਲੀਲਾਂ > d_m3MaxContinuationPayload);

    ਲਈ (u16 i = 0; i < ਅੰਕ_ਦਲੀਲਾਂ; ++i) {
        u16      pIdx         = (u16)(ਅੰਕ_ਦਲੀਲਾਂ - 1 - i);
        m3type_t expectedType = GetFuncTypeParamType(ਨਿਸ਼ਾਨਾ_Type, pIdx);
        m3type_t actualType   = Get_ਢੇਰ_ਸਿਖਰ_Type(o);
        _throwif(m3Err_typeMismatch, not Is_Sub_Type_ਦਾ(actualType, expectedType));
        argSlots[pIdx] = Get_ਢੇਰ_ਸਿਖਰ_ਥਾਂ_ਸੰਖਿਆ(o);
_       (ਕੱਢੋ(o));
    }

    u16 ਨਤੀਜਾ_Slots[d_m3MaxContinuationPayload];
    _throwif("too many switch results", ਅੰਕ_Results > d_m3MaxContinuationPayload);

    ਲਈ (u16 i = 0; i < ਅੰਕ_Results; ++i) {
_       (ਧੱਕੋ_Allocated_ਥਾਂ(o, GetFuncTypeParamType(selfType, i)));
        ਨਤੀਜਾ_Slots[i] = Get_ਢੇਰ_ਸਿਖਰ_ਥਾਂ_ਸੰਖਿਆ(o);
    }

_   (Ensure_ਕੋਡ_ਪੰਨਾ_ਅੰਕ_ਸਤਰਾਂ(o, 6 + ਅੰਕ_ਦਲੀਲਾਂ * 2 + ਅੰਕ_Results * 2 + d_m3_ਕੋਡ_ਪੰਨਾ_Free_ਸਤਰਾਂ_ਹੱਦ));
_   (EmitOp(o, op_Switch));
    Emit_ਸੂਚਕ(o, ਨਿਸ਼ਾਨਾ_Cont_Type);
    Emit_ਸੂਚਕ(o, ਟੈਗ);
    Emit_ਥਾਂ_ਵਿੱਥ(o, ਨਿਸ਼ਾਨਾ_Cont_ਥਾਂ);
    EmitConstant32(o, ਅੰਕ_ਦਲੀਲਾਂ);
    ਲਈ (u16 i = 0; i < ਅੰਕ_ਦਲੀਲਾਂ; ++i) {
        Emit_ਥਾਂ_ਵਿੱਥ(o, argSlots[i]);
        EmitConstant32(o, Is64_ਦੋਆਧਾਰੀ_ਅੰਕ_Type(GetFuncTypeParamType(ਨਿਸ਼ਾਨਾ_Type, i)));
    }
    EmitConstant32(o, ਅੰਕ_Results);
    ਲਈ (u16 i = 0; i < ਅੰਕ_Results; ++i) {
        Emit_ਥਾਂ_ਵਿੱਥ(o, ਨਤੀਜਾ_Slots[i]);
        EmitConstant32(o, Is64_ਦੋਆਧਾਰੀ_ਅੰਕ_Type(GetFuncTypeParamType(selfType, i)));
    }

    // as with suspend: op_Switch clears the results before it leaves
_   (ਰਿਕਾਰਡ_ਸੁਰੱਖਿਅਤ_ਬਿੰਦੂ(o, safepoint_suspend, 0, ਅੰਕ_Results, 0, ਨਿਰਾ));

} _catch: ਵਾਪਸ ਨਤੀਜਾ;
}

#ਜੇ_ਅੰਤ // d_m3HasStackSwitching


ਸਥਿਰ
M3_ਨਤੀਜਾ Compile_Else_ਖੰਡ (IM3Compilation o, pc_t* o_ਸ਼ੁਰੂ_PC, IM3FuncType i_ਖੰਡ_Type)
{
    IM3_ਕੋਡ_ਪੰਨਾ saved_ਪੰਨਾ = o->ਪੰਨਾ;
_try {

    IM3_ਕੋਡ_ਪੰਨਾ else_ਪੰਨਾ;
_   (Acquire_Compilation_ਕੋਡ_ਪੰਨਾ(o, &else_ਪੰਨਾ));

    *o_ਸ਼ੁਰੂ_PC = Get_ਪੰਨਾ_PC(else_ਪੰਨਾ);

    o->ਪੰਨਾ = else_ਪੰਨਾ;

_   (Compile_ਖੰਡ(o, i_ਖੰਡ_Type, c_waOp_else));

_   (EmitOp(o, op_Branch));
    Emit_ਸੂਚਕ(o, Get_ਪੰਨਾ_PC(saved_ਪੰਨਾ));
} _catch:

    ਜੇ (o->ਪੰਨਾ != saved_ਪੰਨਾ) {
        Release_Compilation_ਕੋਡ_ਪੰਨਾ(o);
    }

    o->ਪੰਨਾ = saved_ਪੰਨਾ;

    ਵਾਪਸ ਨਤੀਜਾ;
}

ਸਥਿਰ
M3_ਨਤੀਜਾ Compile_If (IM3Compilation o, m3opcode_t i_opcode)
{
    /*      [   op_If   ]
            [ <else-pc> ]   ---->   [ ..else..  ]
            [  ..if..   ]           [ ..block.. ]
            [ ..block.. ]           [ op_Branch ]
            [    end    ]  <-----   [  <end-pc> ]       */

_try {

    // Spec: if condition must be i32
    ਜੇ (not Is_ਢੇਰ_Polymorphic(o)) {
        m3type_t condType = Get_ਢੇਰ_ਸਿਖਰ_Type(o);
        _throwif(m3Err_typeMismatch, condType != c_m3Type_none and condType != c_m3Type_i32);
    }

_   (Preserve_Non_ਸਿਖਰ_Registers(o));
_   (Preserve_ਦਲੀਲਾਂ_And_Locals(o));

#ਜੇ d_m3FuseBranch
    IM3Operation fuseOp = GetBranchFusionOp(o, true);

    ਜੇ (fuseOp) {
        // absorb the if into the compare that produced the condition
        *(IM3Operation*)o->foldPatchPC = fuseOp;
        InvalidateFold(o);

_       (ਕੱਢੋ(o));                          // the condition is consumed inside the fused op
    } ਨਹੀਂਤਾਂ
#ਜੇ_ਅੰਤ
    {
        IM3Operation op = Is_ਢੇਰ_ਸਿਖਰ_ਵਿੱਚ_ਰਜਿਸਟਰ(o) ? op_If_r : op_If_s;

_       (EmitOp(o, op));
_       (Emit_ਥਾਂ_ਅੰਕ_ਦਾ_ਢੇਰ_ਸਿਖਰ_And_ਕੱਢੋ(o));
    }

    pc_t* pc = (pc_t*)Reserve_ਸੂਚਕ(o);

    IM3FuncType ਖੰਡ_Type = ਨਿਰਾ;
_   (Read_ਖੰਡ_Type(o, &ਖੰਡ_Type));     //  dump_type_stack (o);

    u16 ਢੇਰ_ਸੂਚਕਾਂਕ = o->ਢੇਰ_ਸੂਚਕਾਂਕ;

_   (Compile_ਖੰਡ(o, ਖੰਡ_Type, i_opcode));

    ਜੇ (o->ਪਿਛਲਾ_Opcode == c_waOp_else) {
        o->ਢੇਰ_ਸੂਚਕਾਂਕ = ਢੇਰ_ਸੂਚਕਾਂਕ;
_       (Compile_Else_ਖੰਡ(o, pc, ਖੰਡ_Type));
    } ਨਹੀਂਤਾਂ {
        // if block produces values and there isn't a defined else
        // case, then we need to make one up so that the pass-through
        // results end up in the right place
        ਜੇ (Get_Func_Type_ਅੰਕ_Results(ਖੰਡ_Type)) {
            // rewind to the if's end to create a fake else block
            o->wasm--;
            o->ਢੇਰ_ਸੂਚਕਾਂਕ = ਢੇਰ_ਸੂਚਕਾਂਕ;     // dump_type_stack (o);

_           (Compile_Else_ਖੰਡ(o, pc, ਖੰਡ_Type));
        } ਨਹੀਂਤਾਂ {
            *pc = GetPC(o);
        }
    }

    } _catch: ਵਾਪਸ ਨਤੀਜਾ;
}

ਸਥਿਰ
M3_ਨਤੀਜਾ Compile_Select (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_ਨਤੀਜਾ ਨਤੀਜਾ = m3Err_none;

    u16 slots[3] = { c_ਥਾਂ_ਅਣਵਰਤਿਆ, c_ਥਾਂ_ਅਣਵਰਤਿਆ, c_ਥਾਂ_ਅਣਵਰਤਿਆ };

    IM3Operation op = ਨਿਰਾ;

    m3type_t type = Get_ਢੇਰ_Type_From_ਸਿਖਰ(o, 1); // get type of selection

    ਜੇ (not Is_ਢੇਰ_Polymorphic(o)) {
        // Spec: the condition operand (top) must be i32
        m3type_t condType = Get_ਢੇਰ_Type_From_ਸਿਖਰ(o, 0);
        _throwif(m3Err_typeMismatch, condType != c_m3Type_none and condType != c_m3Type_i32);

        // Spec: the two value operands (below condition) must have matching types
        m3type_t type2 = Get_ਢੇਰ_Type_From_ਸਿਖਰ(o, 2);
        _throwif(m3Err_typeMismatch, type != c_m3Type_none and type2 != c_m3Type_none and
                                       not (Is_Sub_Type_ਦਾ(type, type2) or Is_Sub_Type_ਦਾ(type2, type)));
    }

    ਜੇ (IsFpType(type)) {
#ਜੇ d_m3HasFloat
        // not consuming a fp reg, so preserve
        ਜੇ (not Is_ਢੇਰ_ਸਿਖਰ_Minus1_ਵਿੱਚ_ਰਜਿਸਟਰ(o) and
            not Is_ਢੇਰ_ਸਿਖਰ_Minus2_ਵਿੱਚ_ਰਜਿਸਟਰ(o)) {
_           (Preserve_ਰਜਿਸਟਰ_If_Occupied(o, type));
        }

        bool selector_ਵਿੱਚ_Reg = Is_ਢੇਰ_ਸਿਖਰ_ਵਿੱਚ_ਰਜਿਸਟਰ(o);
        slots[0]           = Get_ਢੇਰ_ਸਿਖਰ_ਥਾਂ_ਸੰਖਿਆ(o);
_       (ਕੱਢੋ(o));

        u32 op_ਸੂਚਕਾਂਕ = 0;

        ਲਈ (u32 i = 1; i <= 2; ++i) {
            ਜੇ (Is_ਢੇਰ_ਸਿਖਰ_ਵਿੱਚ_ਰਜਿਸਟਰ(o)) {
                op_ਸੂਚਕਾਂਕ = i;
            } ਨਹੀਂਤਾਂ {
                slots[i] = Get_ਢੇਰ_ਸਿਖਰ_ਥਾਂ_ਸੰਖਿਆ(o);
            }

_           (ਕੱਢੋ(o));
        }

        op = c_fpSelectOps[type - c_m3Type_f32][selector_ਵਿੱਚ_Reg][op_ਸੂਚਕਾਂਕ];
#ਨਹੀਂਤਾਂ
        _throw(m3Err_unknownOpcode);
#ਜੇ_ਅੰਤ
    } ਨਹੀਂਤਾਂ ਜੇ (IsIntType(type) or IsRefType(type)) {
        // 'sss' operation doesn't consume a register, so might have to protected its contents
        ਜੇ (not Is_ਢੇਰ_ਸਿਖਰ_ਵਿੱਚ_ਰਜਿਸਟਰ(o) and
            not Is_ਢੇਰ_ਸਿਖਰ_Minus1_ਵਿੱਚ_ਰਜਿਸਟਰ(o) and
            not Is_ਢੇਰ_ਸਿਖਰ_Minus2_ਵਿੱਚ_ਰਜਿਸਟਰ(o)) {
_           (Preserve_ਰਜਿਸਟਰ_If_Occupied(o, type));
        }

        u32 op_ਸੂਚਕਾਂਕ = 3;  // op_Select_*_sss

        ਲਈ (u32 i = 0; i < 3; ++i) {
            ਜੇ (Is_ਢੇਰ_ਸਿਖਰ_ਵਿੱਚ_ਰਜਿਸਟਰ(o)) {
                op_ਸੂਚਕਾਂਕ = i;
            } ਨਹੀਂਤਾਂ {
                slots[i] = Get_ਢੇਰ_ਸਿਖਰ_ਥਾਂ_ਸੰਖਿਆ(o);
            }

_           (ਕੱਢੋ(o));
        }

        // a reference is a pointer-sized integer as far as select is concerned
        u32 type_ਸੂਚਕਾਂਕ = IsRefType(type) ? ((M3_SIZEOF_PTR == 8) ? 1 : 0)
                                        : (u32)(type - c_m3Type_i32);

        op = c_intSelectOps[type_ਸੂਚਕਾਂਕ][op_ਸੂਚਕਾਂਕ];
    } ਨਹੀਂਤਾਂ ਜੇ (not Is_ਢੇਰ_Polymorphic(o)) {
        _throw(m3_Err_function_ਢੇਰ_Underrun);
    }

    EmitOp(o, op);
    ਲਈ (u32 i = 0; i < 3; i++) {
        ਜੇ (Is_Valid_ਥਾਂ(slots[i])) {
            Emit_ਥਾਂ_ਵਿੱਥ(o, slots[i]);
        }
    }
_   (ਧੱਕੋ_ਰਜਿਸਟਰ(o, type));

    _catch: ਵਾਪਸ ਨਤੀਜਾ;
}

ਸਥਿਰ
M3_ਨਤੀਜਾ Compile_Drop (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_ਨਤੀਜਾ ਨਤੀਜਾ = ਕੱਢੋ(o);
    ਜੇ (d_m3_Log_Wasm_ਢੇਰ) {
        dump_type_ਢੇਰ(o);
    }
    ਵਾਪਸ ਨਤੀਜਾ;
}

ਸਥਿਰ
M3_ਨਤੀਜਾ Compile_Nop (IM3Compilation o, m3opcode_t i_opcode)
{
    ਵਾਪਸ m3Err_none;
}

ਸਥਿਰ
M3_ਨਤੀਜਾ Compile_Unreachable (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_ਨਤੀਜਾ ਨਤੀਜਾ;

_   (ਜੋੜੋ_Trap_ਰਿਕਾਰਡ(o));

_   (EmitOp(o, op_Unreachable));
_   (ਸੈੱਟ_ਢੇਰ_Polymorphic(o));

    _catch:
    ਵਾਪਸ ਨਤੀਜਾ;
}


// OPTZ: currently all stack slot indices take up a full word, but
// dual stack source operands could be packed together
ਸਥਿਰ
M3_ਨਤੀਜਾ Compile_Operator (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_ਨਤੀਜਾ ਨਤੀਜਾ;

    // Declared before the first throw below, which jumps past this point to
    // _catch and so must not skip an initialization.
    IM3Operation op = ਨਿਰਾ;

    IM3OpInfo opInfo = GetOpInfo(i_opcode);
    _throwif(m3Err_unknownOpcode, not opInfo);

    // Spec: validate operand types for load/store operations
    ਜੇ (not Is_ਢੇਰ_Polymorphic(o)) {
        // For load ops (stackOffset == 0, unary), the operand is always i32 (address)
        ਜੇ (i_opcode >= 0x28 and i_opcode <= 0x35) {
            m3type_t ਸਿਖਰ_Type = Get_ਢੇਰ_ਸਿਖਰ_Type(o);
            _throwif(m3Err_typeMismatch, ਸਿਖਰ_Type != c_m3Type_none and ਸਿਖਰ_Type != c_m3Type_i32);
        }

        // For store ops (stackOffset == -2), the address operand must be i32
        ਜੇ (i_opcode >= 0x36 and i_opcode <= 0x3e) {
            m3type_t addrType = Get_ਢੇਰ_Type_From_ਸਿਖਰ(o, 1);
            _throwif(m3Err_typeMismatch, addrType != c_m3Type_none and addrType != c_m3Type_i32);
        }
    }

    // This preserve is for for FP compare operations.
    // either need additional slot destination operations or the
    // easy fix, move _r0 out of the way.
    // moving out the way might be the optimal solution most often?
    // otherwise, the _r0 reg can get buried down in the stack
    // and be idle & wasted for a moment.
    ਜੇ (M3_UNLIKELY(IsFpType(Get_ਢੇਰ_ਸਿਖਰ_Type(o)) and IsIntType(opInfo->type))) {
_       (Preserve_ਰਜਿਸਟਰ_If_Occupied(o, opInfo->type));
    }

    ਜੇ (opInfo->ਢੇਰ_ਵਿੱਥ == 0) {
        ਜੇ (Is_ਢੇਰ_ਸਿਖਰ_ਵਿੱਚ_ਰਜਿਸਟਰ(o)) {
            op = opInfo->operations[0]; // _s
        } ਨਹੀਂਤਾਂ {
_           (Preserve_ਰਜਿਸਟਰ_If_Occupied(o, opInfo->type));
            op = opInfo->operations[1]; // _r
        }
    } ਨਹੀਂਤਾਂ {
        ਜੇ (Is_ਢੇਰ_ਸਿਖਰ_ਵਿੱਚ_ਰਜਿਸਟਰ(o)) {
            op = opInfo->operations[0];  // _rs

            ਜੇ (Is_ਢੇਰ_ਸਿਖਰ_Minus1_ਵਿੱਚ_ਰਜਿਸਟਰ(o)) {    d_m3Assert (i_opcode == c_waOp_store_f32 or i_opcode == c_waOp_store_f64);
                op = opInfo->operations[3]; // _rr for fp.store
            }
        } ਨਹੀਂਤਾਂ ਜੇ (Is_ਢੇਰ_ਸਿਖਰ_Minus1_ਵਿੱਚ_ਰਜਿਸਟਰ(o)) {
            op = opInfo->operations[1]; // _sr

            ਜੇ (not op) {  // must be commutative, then
                op = opInfo->operations[0];
            }
        } ਨਹੀਂਤਾਂ {
_           (Preserve_ਰਜਿਸਟਰ_If_Occupied(o, opInfo->type));     // _ss
            op = opInfo->operations[2];
        }
    }

    ਜੇ (M3_LIKELY(op)) {
_       (EmitOp(o, op));

#ਜੇ d_m3_Fold_ਸੈੱਟ_Local
        // the op word just written (EmitOp may have bridged pages); NULL in the page-less compile-walk mode
        pc_t patchPC = o->ਪੰਨਾ ? GetPC(o) - 1 : ਨਿਰਾ;
#ਜੇ_ਅੰਤ

_       (Emit_ਥਾਂ_ਅੰਕ_ਦਾ_ਢੇਰ_ਸਿਖਰ_And_ਕੱਢੋ(o));

        ਜੇ (opInfo->ਢੇਰ_ਵਿੱਥ < 0) {
_           (Emit_ਥਾਂ_ਅੰਕ_ਦਾ_ਢੇਰ_ਸਿਖਰ_And_ਕੱਢੋ(o));
        }

        ਜੇ (opInfo->type != c_m3Type_none) {
_           (ਧੱਕੋ_ਰਜਿਸਟਰ(o, opInfo->type));

#ਜੇ d_m3_Fold_ਸੈੱਟ_Local
            // record this op as a fold candidate for an immediately following local.set;
            // the operand-form index is recovered by matching against the variant table
            ਜੇ (patchPC) {
                ਲਈ (u8 form = 0; form < 4; ++form) {
                    ਜੇ (opInfo->operations[form] == op) {
                        o->foldPatchPC    = patchPC;
                        o->foldOpcode     = i_opcode;
                        o->foldForm       = form;
                        o->fold_ਢੇਰ_ਸੂਚਕਾਂਕ = (u16)Get_ਢੇਰ_ਸਿਖਰ_ਸੂਚਕਾਂਕ(o);
                        ਰੋਕੋ;
                    }
                }
            }
#ਜੇ_ਅੰਤ
        }
    } ਨਹੀਂਤਾਂ {
#ਜੇ_ਪਰਿਭਾਸ਼ਿਤ DEBUG
        ਨਤੀਜਾ = ErrorCompile("no operation found for opcode", o, "'%s'", opInfo->name);
#ਨਹੀਂਤਾਂ
        ਨਤੀਜਾ = ErrorCompile("no operation found for opcode", o, "%x", i_opcode);
#ਜੇ_ਅੰਤ
        _throw(ਨਤੀਜਾ);
    }

    _catch: ਵਾਪਸ ਨਤੀਜਾ;
}

ਸਥਿਰ
M3_ਨਤੀਜਾ Compile_Convert (IM3Compilation o, m3opcode_t i_opcode)
{
_try {
    IM3OpInfo opInfo = GetOpInfo(i_opcode);
    _throwif(m3Err_unknownOpcode, not opInfo);

    // Spec: validate source operand type for conversion instructions
    ਜੇ (not Is_ਢੇਰ_Polymorphic(o)) {
        u8 ਸਰੋਤ_Type = c_m3Type_none;
        ਚੋਣ (i_opcode)
        // clang-format off
        {
        ਹਾਲਤ 0xa7:              // i32.wrap/i64
            ਸਰੋਤ_Type = c_m3Type_i64; ਰੋਕੋ;
        ਹਾਲਤ 0xa8: ਹਾਲਤ 0xa9:   // i32.trunc_s/f32, i32.trunc_u/f32
        ਹਾਲਤ 0xae: ਹਾਲਤ 0xaf:   // i64.trunc_s/f32, i64.trunc_u/f32
        ਹਾਲਤ 0xbb:              // f64.promote/f32
        ਹਾਲਤ 0xbc:              // i32.reinterpret/f32
            ਸਰੋਤ_Type = c_m3Type_f32; ਰੋਕੋ;
        ਹਾਲਤ 0xaa: ਹਾਲਤ 0xab:   // i32.trunc_s/f64, i32.trunc_u/f64
        ਹਾਲਤ 0xb0: ਹਾਲਤ 0xb1:   // i64.trunc_s/f64, i64.trunc_u/f64
        ਹਾਲਤ 0xb6:              // f32.demote/f64
        ਹਾਲਤ 0xbd:              // i64.reinterpret/f64
            ਸਰੋਤ_Type = c_m3Type_f64; ਰੋਕੋ;
        ਹਾਲਤ 0xac: ਹਾਲਤ 0xad:   // i64.extend_s/i32, i64.extend_u/i32
        ਹਾਲਤ 0xb2: ਹਾਲਤ 0xb3:   // f32.convert_s/i32, f32.convert_u/i32
        ਹਾਲਤ 0xb7: ਹਾਲਤ 0xb8:   // f64.convert_s/i32, f64.convert_u/i32
        ਹਾਲਤ 0xbe:              // f32.reinterpret/i32
            ਸਰੋਤ_Type = c_m3Type_i32; ਰੋਕੋ;
        ਹਾਲਤ 0xb4: ਹਾਲਤ 0xb5:   // f32.convert_s/i64, f32.convert_u/i64
        ਹਾਲਤ 0xb9: ਹਾਲਤ 0xba:   // f64.convert_s/i64, f64.convert_u/i64
        ਹਾਲਤ 0xbf:              // f64.reinterpret/i64
            ਸਰੋਤ_Type = c_m3Type_i64; ਰੋਕੋ;
        ਮੂਲ: ਰੋਕੋ;
        }
        // clang-format on

        ਜੇ (ਸਰੋਤ_Type != c_m3Type_none) {
            m3type_t ਸਿਖਰ_Type = Get_ਢੇਰ_ਸਿਖਰ_Type(o);
            _throwif(m3Err_typeMismatch, ਸਿਖਰ_Type != c_m3Type_none and not Is_Sub_Type_ਦਾ(ਸਿਖਰ_Type, ਸਰੋਤ_Type));
        }
    }

    bool dest_ਵਿੱਚ_ਥਾਂ   = Is_ਰਜਿਸਟਰ_Type_Allocated(o, opInfo->type);
    bool ਸਰੋਤ_ਵਿੱਚ_ਥਾਂ = Is_ਢੇਰ_ਸਿਖਰ_ਵਿੱਚ_ਥਾਂ(o);

    IM3Operation op = opInfo->operations[dest_ਵਿੱਚ_ਥਾਂ * 2 + ਸਰੋਤ_ਵਿੱਚ_ਥਾਂ];

_   (EmitOp(o, op));
_   (Emit_ਥਾਂ_ਅੰਕ_ਦਾ_ਢੇਰ_ਸਿਖਰ_And_ਕੱਢੋ(o));

    ਜੇ (dest_ਵਿੱਚ_ਥਾਂ) {
_       (ਧੱਕੋ_Allocated_ਥਾਂ_And_Emit_2(o, opInfo->type))
    } ਨਹੀਂਤਾਂ {
_       (ਧੱਕੋ_ਰਜਿਸਟਰ(o, opInfo->type))
    }
}
    _catch: ਵਾਪਸ ਨਤੀਜਾ;
}

#ਜੇ d_m3HasWideArithmetic

// The wide arithmetic proposal: i64.add128 and i64.sub128 over two 128-bit values
// given as i64 halves, and i64.mul_wide_s/u giving the full product of two i64s.
// Each gives back a 128-bit result the same way, low half first. The operands are
// all read from slots, and of the two results the high half, which ends up on top,
// goes to _r0 and the low half to a newly allocated slot - so every operation has
// the one form, whatever shape the stack was in.
ਸਥਿਰ
M3_ਨਤੀਜਾ Compile_WideArithmetic (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_ਨਤੀਜਾ ਨਤੀਜਾ = m3Err_none;

    IM3Operation op          = op_i64_Add128;
    u32          ਅੰਕ_Operands = 4;

    ਚੋਣ (i_opcode)
    // clang-format off
    {
    ਹਾਲਤ c_waOp_i64Sub128:   op = op_i64_Sub128;                          ਰੋਕੋ;
    ਹਾਲਤ c_waOp_i64MulWideS: op = op_i64_MultiplyWide; ਅੰਕ_Operands = 2;   ਰੋਕੋ;
    ਹਾਲਤ c_waOp_i64MulWideU: op = op_u64_MultiplyWide; ਅੰਕ_Operands = 2;   ਰੋਕੋ;
    ਮੂਲ: ਰੋਕੋ;
    }
    // clang-format on

_   (Preserve_ਰਜਿਸਟਰ_If_Occupied(o, c_m3Type_i64));

_   (EmitOp(o, op));

    // the top of the stack first: the high half of the second operand, for the
    // 128-bit ones
    ਲਈ (u32 i = 0; i < ਅੰਕ_Operands; ++i) {
_       (Emit_ਥਾਂ_ਅੰਕ_ਦਾ_ਢੇਰ_ਸਿਖਰ_And_ਕੱਢੋ(o));
    }

_   (ਧੱਕੋ_Allocated_ਥਾਂ_And_Emit_2(o, c_m3Type_i64));      // the low half
_   (ਧੱਕੋ_ਰਜਿਸਟਰ(o, c_m3Type_i64));                  // the high half

    _catch: ਵਾਪਸ ਨਤੀਜਾ;
}

#ਜੇ_ਅੰਤ // d_m3HasWideArithmetic

#ਜੇ d_m3HasMemory64

// Replaces the 64-bit address operand of a load or store with the checked
// 32-bit effective address op_CheckAddr64 leaves behind, so that the access
// itself compiles exactly as it would against a 32-bit memory: one slot to read
// the address from, and an offset of zero folded in already.
//
// Where the address sits under other operands - the value of a store, the
// operands of an atomic - it is rewritten where it stands rather than popped:
// its stack entry is repointed at the scratch slot and retyped, and the slot it
// used to occupy released. i_numOperands counts the address and all above it.
ਸਥਿਰ
M3_ਨਤੀਜਾ EmitCheckAddr64 (IM3Compilation o, u64 i_ਵਿੱਥ, u16 i_ਅੰਕ_Operands)
{
    M3_ਨਤੀਜਾ ਨਤੀਜਾ = m3Err_none;

    // The address has to be in a slot for op_CheckAddr64 to read it. Spilling
    // r0 wholesale also keeps the value of a store out of it, which is what
    // leaves the address as the one operand still in a register to worry about.
_   (Preserve_ਰਜਿਸਟਰ_If_Occupied(o, c_m3Type_i64));

    // unreachable code: the operands the instruction names may not be there
    ਜੇ (o->ਢੇਰ_ਸੂਚਕਾਂਕ < o->ਖੰਡ.ਖੰਡ_ਢੇਰ_ਸੂਚਕਾਂਕ + i_ਅੰਕ_Operands) {
        ਵਾਪਸ ਨਤੀਜਾ;
    }

    {
        u16      ਢੇਰ_ਸੂਚਕਾਂਕ = (u16)(o->ਢੇਰ_ਸੂਚਕਾਂਕ - i_ਅੰਕ_Operands);
        u16      src_ਥਾਂ    = o->wasm_ਢੇਰ[ਢੇਰ_ਸੂਚਕਾਂਕ];
        m3type_t srcType    = o->type_ਢੇਰ[ਢੇਰ_ਸੂਚਕਾਂਕ];

        u16 dst_ਥਾਂ = c_ਥਾਂ_ਅਣਵਰਤਿਆ;

        // Checked here rather than left to the validator, which a build may
        // have switched off: reading a slot as an i64 that only holds an i32
        // would reach past what was allocated for it.
        _throwif(m3Err_typeMismatch, ਅਧਾਰ_Type_ਦਾ(srcType) != c_m3Type_i64);

        // an address is an integer, so nothing should be left in a register
        // once r0 has been preserved
        _throwif(m3Err_typeMismatch, Is_ਰਜਿਸਟਰ_ਥਾਂ_Alias(src_ਥਾਂ));
        _throwif(m3_Err_function_ਢੇਰ_Underrun, src_ਥਾਂ >= o->ਥਾਂ_Max_Allocated_ਸੂਚਕਾਂਕ_Plus_One);

        // allocated before the source is released, so the two cannot be the
        // same slot - op_CheckAddr64 reads one and writes the other
_       (AllocateSlots(o, &dst_ਥਾਂ, c_m3Type_i32));

_       (EmitOp(o, op_CheckAddr64));
        Emit_ਥਾਂ_ਵਿੱਥ(o, src_ਥਾਂ);
        Emit_ਥਾਂ_ਵਿੱਥ(o, dst_ਥਾਂ);
        EmitConstant64(o, i_ਵਿੱਥ);

        o->wasm_ਢੇਰ[ਢੇਰ_ਸੂਚਕਾਂਕ] = dst_ਥਾਂ;
        o->type_ਢੇਰ[ਢੇਰ_ਸੂਚਕਾਂਕ] = c_m3Type_i32;

        // locals and the constant table outlive the instruction; a dynamic
        // slot the address was computed into does not
        ਜੇ (src_ਥਾਂ >= o->ਥਾਂ_ਪਹਿਲਾ_Dynamic_ਸੂਚਕਾਂਕ) {
            Deallocate_ਥਾਂ(o, src_ਥਾਂ, srcType);
        }
    }

    _catch: ਵਾਪਸ ਨਤੀਜਾ;
}

#ਜੇ_ਅੰਤ // d_m3HasMemory64


ਸਥਿਰ
M3_ਨਤੀਜਾ Compile_ਲੱਦੋ_Store (IM3Compilation o, m3opcode_t i_opcode)
{
_try {
    u32 alignHint, ਸਮ੍ਰਿਤੀ_Idx;
    u64 ਸਮ੍ਰਿਤੀ_ਵਿੱਥ;

    // alignHint is checked by the validator
_   (Read_ਸਮ੍ਰਿਤੀ_Arg(&alignHint, &ਸਮ੍ਰਿਤੀ_Idx, &ਸਮ੍ਰਿਤੀ_ਵਿੱਥ, &o->wasm, o->wasm_ਸਿਰਾ));
                                                                        m3log (compile, d_indent " (memory = %d; offset = %llu)", get_indention_string (o), ਸਮ੍ਰਿਤੀ_Idx, (ਬਿਨਾਨਿਸ਼ਾਨੀ ਲੰਮਾ ਲੰਮਾ) ਸਮ੍ਰਿਤੀ_ਵਿੱਥ);
    _throwif(m3_Err_unknown_ਸਮ੍ਰਿਤੀ, ਸਮ੍ਰਿਤੀ_Idx >= o->module->ਅੰਕ_Memories);

    IM3OpInfo opInfo = GetOpInfo(i_opcode);
    _throwif(m3Err_unknownOpcode, not opInfo);

    bool isMemory64 = o->module->memories[ਸਮ੍ਰਿਤੀ_Idx]->isMemory64;

    // Spec: the static offset has to be in range of the address type. Checked
    // here as well as in the validator, so that a build without one still
    // refuses the module rather than silently truncating the offset.
    _throwif(m3Err_wasmMalformed, not isMemory64 and ਸਮ੍ਰਿਤੀ_ਵਿੱਥ > 0xFFFFFFFFull);

    ਜੇ (IsFpType(opInfo->type)) {
_       (Preserve_ਰਜਿਸਟਰ_If_Occupied(o, c_m3Type_f64));
    }

    ਜੇ (ਸਮ੍ਰਿਤੀ_Idx) {
_       (Emit_ਸੈੱਟ_ਸਮ੍ਰਿਤੀ(o, ਸਮ੍ਰਿਤੀ_Idx));
    }

#ਜੇ d_m3HasMemory64
    ਜੇ (isMemory64) {
        // An offset this far out puts every address out of bounds, so rather
        // than carrying it, clamp it to the limit: op_CheckAddr64 then traps
        // on whatever address it is given, which is the outcome either way.
        ਜੇ (ਸਮ੍ਰਿਤੀ_ਵਿੱਥ > d_m3_ਪਤਾ_ਹੱਦਬੰਦੀ) {
            ਸਮ੍ਰਿਤੀ_ਵਿੱਥ = d_m3_ਪਤਾ_ਹੱਦਬੰਦੀ;
        }

_       (EmitCheckAddr64(o, ਸਮ੍ਰਿਤੀ_ਵਿੱਥ, opInfo->ਢੇਰ_ਵਿੱਥ < 0 ? 2 : 1));

        // op_CheckAddr64 has folded the offset in already
        ਸਮ੍ਰਿਤੀ_ਵਿੱਥ = 0;
    }
#ਜੇ_ਅੰਤ

_   (Compile_Operator(o, i_opcode));

    EmitConstant32(o, (u32)ਸਮ੍ਰਿਤੀ_ਵਿੱਥ);

    ਜੇ (ਸਮ੍ਰਿਤੀ_Idx) {
_       (Emit_ਸੈੱਟ_ਸਮ੍ਰਿਤੀ(o, 0));
    }
}
    _catch: ਵਾਪਸ ਨਤੀਜਾ;
}


#ਜੇ d_m3HasAtomics

// What an atomic instruction takes and gives back, from its sub-opcode. The
// accesses come in runs of seven, one per combination of value type and cell width:
// i32/32, i64/64, i32/8, i32/16, i64/8, i64/16, i64/32. Those runs are told apart by
// where they start.
//
// The operand count includes the address. The top operand is the one that ends up
// in a register, so its type is the one to check on the way out; a store has no
// result, which is c_m3Type_none.
ਸਥਿਰ
ਖਾਲੀ GetAtomicShape (u32 i_sub, u16* o_ਅੰਕ_Operands, m3type_t* o_ਸਿਖਰ_Type, m3type_t* o_ਨਤੀਜਾ_Type)
{
    // clang-format off
    ਸਥਿਰ ਅਚਲ m3type_t c_ਮੁੱਲ_Types [7] = {
        c_m3Type_i32, c_m3Type_i64,
        c_m3Type_i32, c_m3Type_i32,
        c_m3Type_i64, c_m3Type_i64, c_m3Type_i64
    };
    // clang-format on

    ਜੇ (i_sub == 0x00) {                    // memory.atomic.notify: address, count
        *o_ਅੰਕ_Operands = 2;
        *o_ਸਿਖਰ_Type     = c_m3Type_i32;
        *o_ਨਤੀਜਾ_Type  = c_m3Type_i32;
    } ਨਹੀਂਤਾਂ ਜੇ (i_sub <= 0x02) {             // memory.atomic.wait32/64: address, expected, timeout
        *o_ਅੰਕ_Operands = 3;
        *o_ਸਿਖਰ_Type     = c_m3Type_i64;
        *o_ਨਤੀਜਾ_Type  = c_m3Type_i32;
    } ਨਹੀਂਤਾਂ ਜੇ (i_sub <= 0x16) {             // loads: address
        *o_ਅੰਕ_Operands = 1;
        *o_ਸਿਖਰ_Type     = c_m3Type_i32;
        *o_ਨਤੀਜਾ_Type  = c_ਮੁੱਲ_Types[i_sub - 0x10];
    } ਨਹੀਂਤਾਂ ਜੇ (i_sub <= 0x1d) {             // stores: address, value
        *o_ਅੰਕ_Operands = 2;
        *o_ਸਿਖਰ_Type     = c_ਮੁੱਲ_Types[i_sub - 0x17];
        *o_ਨਤੀਜਾ_Type  = c_m3Type_none;
    } ਨਹੀਂਤਾਂ ਜੇ (i_sub <= 0x47) {             // read-modify-write: address, value
        *o_ਅੰਕ_Operands = 2;
        *o_ਸਿਖਰ_Type     = c_ਮੁੱਲ_Types[(i_sub - 0x1e) % 7];
        *o_ਨਤੀਜਾ_Type  = *o_ਸਿਖਰ_Type;
    } ਨਹੀਂਤਾਂ {                                // cmpxchg: address, expected, replacement
        *o_ਅੰਕ_Operands = 3;
        *o_ਸਿਖਰ_Type     = c_ਮੁੱਲ_Types[i_sub - 0x48];
        *o_ਨਤੀਜਾ_Type  = *o_ਸਿਖਰ_Type;
    }
}


// The description an access takes as its last immediate: see m3_exec_atomic.h
ਸਥਿਰ
u32 GetAtomicDesc (u32 i_sub)
{
    // per variant, the log2 of the width of the cell
    ਸਥਿਰ ਅਚਲ u8 c_log2Widths[7] = { 2, 3, 0, 1, 0, 1, 2 };

    u32 variant, ਕਿਸਮ = 0;

    ਜੇ (i_sub <= 0x16) {
        variant = i_sub - 0x10;
    } ਨਹੀਂਤਾਂ ਜੇ (i_sub <= 0x1d) {
        variant = i_sub - 0x17;
    } ਨਹੀਂਤਾਂ ਜੇ (i_sub <= 0x47) {
        variant = (i_sub - 0x1e) % 7;
        ਕਿਸਮ    = (i_sub - 0x1e) / 7;         // add, sub, and, or, xor, xchg
    } ਨਹੀਂਤਾਂ {
        variant = i_sub - 0x48;
    }

    // variants 1 and 4..6 are the i64 ones
    u32 is64 = (variant == 1 or variant >= 4) ? 1u : 0u;

    ਵਾਪਸ c_log2Widths[variant] | (is64 << 2) | (ਕਿਸਮ << 3);
}


// Every atomic access is one op, whose operands are the topmost in _r0 and the rest
// in slots, topmost first. The address is always the deepest, so it is the last slot
// (or, for a load, the register), followed by the memarg offset. The ops check the
// address themselves: for an atomic that includes its alignment, which only the
// effective address can be asked.
ਸਥਿਰ
M3_ਨਤੀਜਾ Compile_Atomic (IM3Compilation o, m3opcode_t i_opcode)
{
_try {
    u32 alignHint, ਸਮ੍ਰਿਤੀ_Idx;
    u64 ਸਮ੍ਰਿਤੀ_ਵਿੱਥ;

    u16      ਅੰਕ_Operands = 0;
    m3type_t ਸਿਖਰ_Type     = c_m3Type_none;
    m3type_t ਨਤੀਜਾ_Type  = c_m3Type_none;

    ਜੇ (i_opcode == c_waOp_atomicFence) {
        // the reserved byte, which the validator has checked to be zero. There
        // is nothing to order between accesses that one thread makes in sequence.
        u8 reserved;
_       (Read_u8(&reserved, &o->wasm, o->wasm_ਸਿਰਾ));
    } ਨਹੀਂਤਾਂ {
        // alignHint is checked by the validator
_       (Read_ਸਮ੍ਰਿਤੀ_Arg(&alignHint, &ਸਮ੍ਰਿਤੀ_Idx, &ਸਮ੍ਰਿਤੀ_ਵਿੱਥ, &o->wasm, o->wasm_ਸਿਰਾ));
                                                                        m3log (compile, d_indent " (memory = %d; offset = %llu)", get_indention_string (o), ਸਮ੍ਰਿਤੀ_Idx, (ਬਿਨਾਨਿਸ਼ਾਨੀ ਲੰਮਾ ਲੰਮਾ) ਸਮ੍ਰਿਤੀ_ਵਿੱਥ);
        _throwif(m3_Err_unknown_ਸਮ੍ਰਿਤੀ, ਸਮ੍ਰਿਤੀ_Idx >= o->module->ਅੰਕ_Memories);

        IM3OpInfo opInfo = GetOpInfo(i_opcode);
        _throwif(m3Err_unknownOpcode, not opInfo);

        bool isMemory64 = o->module->memories[ਸਮ੍ਰਿਤੀ_Idx]->isMemory64;

        // see Compile_Load_Store
        _throwif(m3Err_wasmMalformed, not isMemory64 and ਸਮ੍ਰਿਤੀ_ਵਿੱਥ > 0xFFFFFFFFull);

        GetAtomicShape(i_opcode & 0xFF, &ਅੰਕ_Operands, &ਸਿਖਰ_Type, &ਨਤੀਜਾ_Type);

        ਜੇ (ਸਮ੍ਰਿਤੀ_Idx) {
_           (Emit_ਸੈੱਟ_ਸਮ੍ਰਿਤੀ(o, ਸਮ੍ਰਿਤੀ_Idx));
        }

#  ਜੇ d_m3HasMemory64
        ਜੇ (isMemory64) {
            // see Compile_Load_Store
            ਜੇ (ਸਮ੍ਰਿਤੀ_ਵਿੱਥ > d_m3_ਪਤਾ_ਹੱਦਬੰਦੀ) {
                ਸਮ੍ਰਿਤੀ_ਵਿੱਥ = d_m3_ਪਤਾ_ਹੱਦਬੰਦੀ;
            }

_           (EmitCheckAddr64(o, ਸਮ੍ਰਿਤੀ_ਵਿੱਥ, ਅੰਕ_Operands));
            ਸਮ੍ਰਿਤੀ_ਵਿੱਥ = 0;
        }
#  endif

_       (Copy_ਢੇਰ_ਸਿਖਰ_ਮੰਜ਼ਿਲ_ਰਜਿਸਟਰ(o, false));
_       (EmitOp(o, opInfo->operations[0]));
_       (ਕੱਢੋ_Type(o, ਸਿਖਰ_Type));

        ਲਈ (u16 i = 1; i < ਅੰਕ_Operands; ++i) {
_           (Emit_ਥਾਂ_ਅੰਕ_ਦਾ_ਢੇਰ_ਸਿਖਰ_And_ਕੱਢੋ(o));
        }

        EmitConstant32(o, (u32)ਸਮ੍ਰਿਤੀ_ਵਿੱਥ);
        ਜੇ (i_opcode >= c_wa_Op_atomic_ਪਹਿਲਾ_Access) {
            EmitConstant32(o, GetAtomicDesc(i_opcode & 0xFF));
        }

        ਜੇ (ਸਮ੍ਰਿਤੀ_Idx) {
_           (Emit_ਸੈੱਟ_ਸਮ੍ਰਿਤੀ(o, 0));
        }

        ਜੇ (ਨਤੀਜਾ_Type != c_m3Type_none) {
_           (ਧੱਕੋ_ਰਜਿਸਟਰ(o, ਨਤੀਜਾ_Type));
        }
    }
}
    _catch: ਵਾਪਸ ਨਤੀਜਾ;
}

#ਜੇ_ਅੰਤ // d_m3HasAtomics


M3_ਨਤੀਜਾ CompileRawFunction (IM3Module io_module, IM3Function io_function, ਅਚਲ ਖਾਲੀ* i_function, ਅਚਲ ਖਾਲੀ* i_userdata)
{
    d_m3Assert(io_module->runtime);

    IM3_ਕੋਡ_ਪੰਨਾ ਪੰਨਾ = Acquire_ਕੋਡ_ਪੰਨਾ_With_ਸਮਰੱਥਾ(io_module->runtime, 4);

    ਜੇ (ਪੰਨਾ) {
        io_function->compiled = Get_ਪੰਨਾ_PC(ਪੰਨਾ);
        io_function->module   = io_module;

        // Unless a host module's ABI names a memory (m3_BindImportMemory), a
        // host function addresses memory 0 - what a bare i32 guest pointer means
        io_function->host_ਸਮ੍ਰਿਤੀ = io_module->memory0;

        Emit_ਸ਼ਬਦ(ਪੰਨਾ, op_CallRawFunction);
        Emit_ਸ਼ਬਦ(ਪੰਨਾ, i_function);
        Emit_ਸ਼ਬਦ(ਪੰਨਾ, io_function);
        Emit_ਸ਼ਬਦ(ਪੰਨਾ, i_userdata);

        Release_ਕੋਡ_ਪੰਨਾ(io_module->runtime, ਪੰਨਾ);
        ਵਾਪਸ m3Err_none;
    } ਨਹੀਂਤਾਂ {
        ਵਾਪਸ m3_Err_malloc_Failed_ਕੋਡ_ਪੰਨਾ;
    }
}


// A table entry names every field it sets, so the struct can order them for
// packing; the columns follow that order, since M3OP names them positionally.
//
// d_logOp, d_logOp2 macros aren't actually used by the compiler, just codepage decoding (d_m3LogCodePages = 1)
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ d_ops(...)                          .operations = { __VA_ARGS__ }

#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ d_logOp(OP)                         d_ops( op_##OP,                 ਨਿਰਾ,                       ਨਿਰਾ,                       ਨਿਰਾ )
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ d_logOp2(OP1,OP2)                   d_ops( op_##OP1,                op_##OP2,                   ਨਿਰਾ,                       ਨਿਰਾ )

#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ d_ਖਾਲੀ_ਥਾਵਾਂ_Op_List                       d_ops( ਨਿਰਾ,                    ਨਿਰਾ,                       ਨਿਰਾ,                       ਨਿਰਾ )
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ d_unaryOpList(TYPE, NAME)           d_ops( op_##TYPE##_##NAME##_r,  op_##TYPE##_##NAME##_s,     ਨਿਰਾ,                       ਨਿਰਾ )
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ d_binOpList(TYPE, NAME)             d_ops( op_##TYPE##_##NAME##_rs, op_##TYPE##_##NAME##_sr,    op_##TYPE##_##NAME##_ss,    ਨਿਰਾ )
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ d_storeFpOpList(TYPE, NAME)         d_ops( op_##TYPE##_##NAME##_rs, op_##TYPE##_##NAME##_sr,    op_##TYPE##_##NAME##_ss,    op_##TYPE##_##NAME##_rr )
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ d_commutativeBinOpList(TYPE, NAME)  d_ops( op_##TYPE##_##NAME##_rs, ਨਿਰਾ,                       op_##TYPE##_##NAME##_ss,    ਨਿਰਾ )

#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ d_convertOpList(OP)                 d_ops( op_##OP##_r_r,           op_##OP##_r_s,              op_##OP##_s_r,              op_##OP##_s_s )


// Every M3Compiler the tables below name, as one list, so that the index enum and
// the array of pointers can never drift apart. Each entry must carry the same
// config guard as the function's definition.
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ d_m3CompilerList(_)             \
    _( Compile_Unreachable )            \
    _( Compile_Nop )                    \
    _( Compile_Loop_Or_ਖੰਡ )            \
    _( Compile_If )                     \
    _( Compile_ਸਿਰਾ )                    \
    _( Compile_Branch )                 \
    _( Compile_Branch_ਸਾਰਣੀ )            \
    _( Compile_Return )                 \
    _( Compile_Call )                   \
    _( Compile_Call_ਅਸਿੱਧਾ )           \
    _( Compile_Drop )                   \
    _( Compile_Select )                 \
    _( Compile_GetLocal )               \
    _( Compile_ਸੈੱਟ_Local )               \
    _( Compile_Get_ਸੈੱਟ_Global )           \
    _( Compile_ਲੱਦੋ_Store )             \
    _( Compile_ਸਮ੍ਰਿਤੀ_ਮਾਪ )            \
    _( Compile_ਸਮ੍ਰਿਤੀ_Grow )            \
    _( Compile_ਸਮ੍ਰਿਤੀ_Init )            \
    _( Compile_ਸਮ੍ਰਿਤੀ_Copy_Fill )        \
    _( Compile_ਅੰਕੜੇ_Drop )              \
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
    d_m3_Compiler_List_ਢੇਰ_Switching( _ )

#ਜੇ d_m3ImplementFloat
#  define d_m3CompilerList_f(_)         \
    _( Compile_Const_f32 )              \
    _( Compile_Const_f64 )
#ਨਹੀਂਤਾਂ
#  define d_m3CompilerList_f(_)
#ਜੇ_ਅੰਤ

#ਜੇ d_m3HasRefTypes
#  define d_m3CompilerList_refTypes(_)  \
    _( Compile_Select_Typed )           \
    _( Compile_Ref_Null )               \
    _( Compile_Ref_IsNull )             \
    _( Compile_Ref_Func )               \
    _( Compile_ਸਾਰਣੀ_Get_ਸੈੱਟ )           \
    _( Compile_ਸਾਰਣੀ_Init )             \
    _( Compile_ਸਾਰਣੀ_Copy )             \
    _( Compile_ਸਾਰਣੀ_ਮਾਪ )             \
    _( Compile_ਸਾਰਣੀ_Grow_Fill )         \
    _( Compile_Elem_Drop )
#ਨਹੀਂਤਾਂ
#  define d_m3CompilerList_refTypes(_)
#ਜੇ_ਅੰਤ

#ਜੇ d_m3HasTypedRefs
#  define d_m3CompilerList_typedRefs(_) \
    _( Compile_CallRef )                \
    _( Compile_Ref_AsNonNull )
#ਨਹੀਂਤਾਂ
#  define d_m3CompilerList_typedRefs(_)
#ਜੇ_ਅੰਤ

#ਜੇ d_m3HasExceptionHandling
#  define d_m3CompilerList_eh(_)        \
    _( Compile_Throw )                  \
    _( Compile_ThrowRef )               \
    _( Compile_Try_ਸਾਰਣੀ )
#ਨਹੀਂਤਾਂ
#  define d_m3CompilerList_eh(_)
#ਜੇ_ਅੰਤ

#ਜੇ d_m3HasAtomics
#  define d_m3CompilerList_atomics(_)   \
    _( Compile_Atomic )
#ਨਹੀਂਤਾਂ
#  define d_m3CompilerList_atomics(_)
#ਜੇ_ਅੰਤ

#ਜੇ d_m3HasWideArithmetic
#  define d_m3CompilerList_wide(_)      \
    _( Compile_WideArithmetic )
#ਨਹੀਂਤਾਂ
#  define d_m3CompilerList_wide(_)
#ਜੇ_ਅੰਤ

#ਜੇ d_m3_Has_ਢੇਰ_Switching
#  define d_m3_Compiler_List_ਢੇਰ_Switching(_) \
    _( Compile_ContNew )                     \
    _( Compile_ContBind )                    \
    _( Compile_Suspend )                     \
    _( Compile_Resume )                      \
    _( Compile_Switch )
#ਨਹੀਂਤਾਂ
#  define d_m3_Compiler_List_ਢੇਰ_Switching(_)
#ਜੇ_ਅੰਤ

// Index 0 is "this opcode has no compiler", so that a zeroed entry - which is
// what M3OP_RESERVED and the reserved holes in the tables are - reads as one.
ਗਿਣਤੀ {
    c_cc_NULL = 0,
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ d_m3CompilerEnum(NAME)  c_cc_##NAME,
    d_m3CompilerList(d_m3CompilerEnum)
#ਪਰਿਭਾਸ਼ਾ_ਰੱਦ d_m3CompilerEnum
};

ਸਥਿਰ ਅਚਲ M3Compiler c_compilers[] = {
    ਨਿਰਾ,
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ d_m3CompilerPtr(NAME)   NAME,
    d_m3CompilerList(d_m3CompilerPtr)
#ਪਰਿਭਾਸ਼ਾ_ਰੱਦ d_m3CompilerPtr
};

// Names a table entry's compiler by index. d_cc (NULL) is the no-compiler entry.
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ d_cc(NAME)                          .compiler = c_cc_##NAME

ਸਥਿਰ
M3Compiler GetCompiler (IM3OpInfo i_opInfo)
{
    d_m3Assert(i_opInfo->compiler < M3_ਗਿਣਤੀ_ਦਾ(c_compilers));
    ਵਾਪਸ c_compilers[i_opInfo->compiler];
}

// clang-format off
ਅਚਲ M3OpInfo c_operations[] =
{
    M3OP( "unreachable",           0,  none,   d_cc(Compile_Unreachable),  d_logOp (Unreachable) ),                   // 0x00
    M3OP( "nop",                   0,  none,   d_cc(Compile_Nop),          d_ਖਾਲੀ_ਥਾਵਾਂ_Op_List ),                           // 0x01 .
    M3OP( "block",                 0,  none,   d_cc(Compile_Loop_Or_ਖੰਡ),  d_ਖਾਲੀ_ਥਾਵਾਂ_Op_List ),                           // 0x02
    M3OP( "loop",                  0,  none,   d_cc(Compile_Loop_Or_ਖੰਡ),  d_logOp (Loop) ),                          // 0x03
    M3OP( "if",                    -1, none,   d_cc(Compile_If),           d_ਖਾਲੀ_ਥਾਵਾਂ_Op_List ),                           // 0x04
    M3OP( "else",                  0,  none,   d_cc(Compile_Nop),          d_ਖਾਲੀ_ਥਾਵਾਂ_Op_List ),                           // 0x05

    M3OP_RESERVED,  M3OP_RESERVED,                                                                                    // 0x06...0x07

#ਜੇ d_m3HasExceptionHandling
    M3OP( "throw",                 0,  none,   d_cc(Compile_Throw),        d_logOp (Throw) ),                         // 0x08
    M3OP_RESERVED,                                                                                                    // 0x09
    M3OP( "throw_ref",             0,  none,   d_cc(Compile_ThrowRef),     d_logOp (ThrowRef) ),                      // 0x0a
#ਨਹੀਂਤਾਂ
    M3OP_RESERVED,  M3OP_RESERVED, M3OP_RESERVED,                                                                     // 0x08...0x0a
#ਜੇ_ਅੰਤ

    M3OP( "end",                   0,  none,   d_cc(Compile_ਸਿਰਾ),          d_ਖਾਲੀ_ਥਾਵਾਂ_Op_List ),                           // 0x0b
    M3OP( "br",                    0,  none,   d_cc(Compile_Branch),       d_logOp (Branch) ),                        // 0x0c
    M3OP( "br_if",                 -1, none,   d_cc(Compile_Branch),       d_logOp2 (BranchIf_r, BranchIf_s) ),       // 0x0d
    M3OP( "br_table",              -1, none,   d_cc(Compile_Branch_ਸਾਰਣੀ),  d_logOp (Branch_ਸਾਰਣੀ) ),                   // 0x0e
    M3OP( "return",                0,  any,    d_cc(Compile_Return),       d_logOp (Return) ),                        // 0x0f
    M3OP( "call",                  0,  any,    d_cc(Compile_Call),         d_logOp (Call) ),                          // 0x10
    M3OP( "call_indirect",         0,  any,    d_cc(Compile_Call_ਅਸਿੱਧਾ), d_logOp (Call_ਅਸਿੱਧਾ) ),                  // 0x11
    M3OP( "return_call",           0,  any,    d_cc(Compile_Call),         d_logOp (ReturnCall) ),                    // 0x12
    M3OP( "return_call_indirect",  0,  any,    d_cc(Compile_Call_ਅਸਿੱਧਾ), d_logOp (Return_Call_ਅਸਿੱਧਾ) ),            // 0x13

#ਜੇ d_m3HasTypedRefs
    M3OP( "call_ref",              0,  any,    d_cc(Compile_CallRef),      d_logOp (CallRef) ),                       // 0x14
    M3OP( "return_call_ref",       0,  any,    d_cc(Compile_CallRef),      d_logOp (ReturnCallRef) ),                 // 0x15
#ਨਹੀਂਤਾਂ
    M3OP_RESERVED,  M3OP_RESERVED,                                                                                    // 0x14...
#ਜੇ_ਅੰਤ
    M3OP_RESERVED,  M3OP_RESERVED, M3OP_RESERVED, M3OP_RESERVED,                                                      // ...0x19

    M3OP( "drop",                  -1, none,   d_cc(Compile_Drop),         d_ਖਾਲੀ_ਥਾਵਾਂ_Op_List ),                           // 0x1a
    M3OP( "select",                -2, any,    d_cc(Compile_Select),       d_ਖਾਲੀ_ਥਾਵਾਂ_Op_List ),                           // 0x1b

#ਜੇ d_m3HasRefTypes
    M3OP( "select.t",              -2, any,    d_cc(Compile_Select_Typed), d_ਖਾਲੀ_ਥਾਵਾਂ_Op_List ),                           // 0x1c
#ਨਹੀਂਤਾਂ
    M3OP_RESERVED,                                                                                                    // 0x1c
#ਜੇ_ਅੰਤ
    M3OP_RESERVED,  M3OP_RESERVED,                                                                                    // 0x1d...0x1e

#ਜੇ d_m3HasExceptionHandling
    M3OP( "try_table",             0,  none,   d_cc(Compile_Try_ਸਾਰਣੀ),     d_logOp (Try_ਸਾਰਣੀ) ),                      // 0x1f
#ਨਹੀਂਤਾਂ
    M3OP_RESERVED,                                                                                                    // 0x1f
#ਜੇ_ਅੰਤ

    M3OP( "local.get",             1,  any,    d_cc(Compile_GetLocal),     d_ਖਾਲੀ_ਥਾਵਾਂ_Op_List ),                           // 0x20
    M3OP( "local.set",             1,  none,   d_cc(Compile_ਸੈੱਟ_Local),     d_ਖਾਲੀ_ਥਾਵਾਂ_Op_List ),                           // 0x21
    M3OP( "local.tee",             0,  any,    d_cc(Compile_ਸੈੱਟ_Local),     d_ਖਾਲੀ_ਥਾਵਾਂ_Op_List ),                           // 0x22
    M3OP( "global.get",            1,  none,   d_cc(Compile_Get_ਸੈੱਟ_Global), d_ਖਾਲੀ_ਥਾਵਾਂ_Op_List ),                           // 0x23
    M3OP( "global.set",            1,  none,   d_cc(Compile_Get_ਸੈੱਟ_Global), d_ਖਾਲੀ_ਥਾਵਾਂ_Op_List ),                           // 0x24

#ਜੇ d_m3HasRefTypes
    M3OP( "table.get",             0,  any,    d_cc(Compile_ਸਾਰਣੀ_Get_ਸੈੱਟ), d_ਖਾਲੀ_ਥਾਵਾਂ_Op_List ),                           // 0x25
    M3OP( "table.set",             -2, none,   d_cc(Compile_ਸਾਰਣੀ_Get_ਸੈੱਟ), d_ਖਾਲੀ_ਥਾਵਾਂ_Op_List ),                           // 0x26
#ਨਹੀਂਤਾਂ
    M3OP_RESERVED,  M3OP_RESERVED,                                                                                    // 0x25...0x26
#ਜੇ_ਅੰਤ
    M3OP_RESERVED,                                                                                                    // 0x27

    M3OP( "i32.load",              0,  i_32,   d_cc(Compile_ਲੱਦੋ_Store),   d_unaryOpList (i32, ਲੱਦੋ_i32) ),           // 0x28
    M3OP( "i64.load",              0,  i_64,   d_cc(Compile_ਲੱਦੋ_Store),   d_unaryOpList (i64, ਲੱਦੋ_i64) ),           // 0x29
    M3OP_F( "f32.load",            0,  f_32,   d_cc(Compile_ਲੱਦੋ_Store),   d_unaryOpList (f32, ਲੱਦੋ_f32) ),           // 0x2a
    M3OP_F( "f64.load",            0,  f_64,   d_cc(Compile_ਲੱਦੋ_Store),   d_unaryOpList (f64, ਲੱਦੋ_f64) ),           // 0x2b

    M3OP( "i32.load8_s",           0,  i_32,   d_cc(Compile_ਲੱਦੋ_Store),   d_unaryOpList (i32, ਲੱਦੋ_i8) ),            // 0x2c
    M3OP( "i32.load8_u",           0,  i_32,   d_cc(Compile_ਲੱਦੋ_Store),   d_unaryOpList (i32, ਲੱਦੋ_u8) ),            // 0x2d
    M3OP( "i32.load16_s",          0,  i_32,   d_cc(Compile_ਲੱਦੋ_Store),   d_unaryOpList (i32, ਲੱਦੋ_i16) ),           // 0x2e
    M3OP( "i32.load16_u",          0,  i_32,   d_cc(Compile_ਲੱਦੋ_Store),   d_unaryOpList (i32, ਲੱਦੋ_u16) ),           // 0x2f

    M3OP( "i64.load8_s",           0,  i_64,   d_cc(Compile_ਲੱਦੋ_Store),   d_unaryOpList (i64, ਲੱਦੋ_i8) ),            // 0x30
    M3OP( "i64.load8_u",           0,  i_64,   d_cc(Compile_ਲੱਦੋ_Store),   d_unaryOpList (i64, ਲੱਦੋ_u8) ),            // 0x31
    M3OP( "i64.load16_s",          0,  i_64,   d_cc(Compile_ਲੱਦੋ_Store),   d_unaryOpList (i64, ਲੱਦੋ_i16) ),           // 0x32
    M3OP( "i64.load16_u",          0,  i_64,   d_cc(Compile_ਲੱਦੋ_Store),   d_unaryOpList (i64, ਲੱਦੋ_u16) ),           // 0x33
    M3OP( "i64.load32_s",          0,  i_64,   d_cc(Compile_ਲੱਦੋ_Store),   d_unaryOpList (i64, ਲੱਦੋ_i32) ),           // 0x34
    M3OP( "i64.load32_u",          0,  i_64,   d_cc(Compile_ਲੱਦੋ_Store),   d_unaryOpList (i64, ਲੱਦੋ_u32) ),           // 0x35

    M3OP( "i32.store",             -2, none,   d_cc(Compile_ਲੱਦੋ_Store),   d_binOpList (i32, Store_i32) ),            // 0x36
    M3OP( "i64.store",             -2, none,   d_cc(Compile_ਲੱਦੋ_Store),   d_binOpList (i64, Store_i64) ),            // 0x37
    M3OP_F( "f32.store",           -2, none,   d_cc(Compile_ਲੱਦੋ_Store),   d_storeFpOpList (f32, Store_f32) ),        // 0x38
    M3OP_F( "f64.store",           -2, none,   d_cc(Compile_ਲੱਦੋ_Store),   d_storeFpOpList (f64, Store_f64) ),        // 0x39

    M3OP( "i32.store8",            -2, none,   d_cc(Compile_ਲੱਦੋ_Store),   d_binOpList (i32, Store_u8) ),             // 0x3a
    M3OP( "i32.store16",           -2, none,   d_cc(Compile_ਲੱਦੋ_Store),   d_binOpList (i32, Store_i16) ),            // 0x3b

    M3OP( "i64.store8",            -2, none,   d_cc(Compile_ਲੱਦੋ_Store),   d_binOpList (i64, Store_u8) ),             // 0x3c
    M3OP( "i64.store16",           -2, none,   d_cc(Compile_ਲੱਦੋ_Store),   d_binOpList (i64, Store_i16) ),            // 0x3d
    M3OP( "i64.store32",           -2, none,   d_cc(Compile_ਲੱਦੋ_Store),   d_binOpList (i64, Store_i32) ),            // 0x3e

    M3OP( "memory.size",           1,  i_32,   d_cc(Compile_ਸਮ੍ਰਿਤੀ_ਮਾਪ),  d_logOp (Mem_ਮਾਪ) ),                       // 0x3f
    M3OP( "memory.grow",           1,  i_32,   d_cc(Compile_ਸਮ੍ਰਿਤੀ_Grow),  d_logOp (MemGrow) ),                       // 0x40

    M3OP( "i32.const",             1,  i_32,   d_cc(Compile_Const_i32),    d_logOp (Const32) ),                       // 0x41
    M3OP( "i64.const",             1,  i_64,   d_cc(Compile_Const_i64),    d_logOp (Const64) ),                       // 0x42
    M3OP_F( "f32.const",           1,  f_32,   d_cc(Compile_Const_f32),    d_ਖਾਲੀ_ਥਾਵਾਂ_Op_List ),                           // 0x43
    M3OP_F( "f64.const",           1,  f_64,   d_cc(Compile_Const_f64),    d_ਖਾਲੀ_ਥਾਵਾਂ_Op_List ),                           // 0x44

    M3OP( "i32.eqz",               0,  i_32,   d_cc(ਨਿਰਾ),                 d_unaryOpList (i32, Equal_ਮੰਜ਼ਿਲ_Zero) ),        // 0x45
    M3OP( "i32.eq",                -1, i_32,   d_cc(ਨਿਰਾ),                 d_commutativeBinOpList (i32, Equal) ),     // 0x46
    M3OP( "i32.ne",                -1, i_32,   d_cc(ਨਿਰਾ),                 d_commutativeBinOpList (i32, NotEqual) ),  // 0x47
    M3OP( "i32.lt_s",              -1, i_32,   d_cc(ਨਿਰਾ),                 d_binOpList (i32, LessThan) ),             // 0x48
    M3OP( "i32.lt_u",              -1, i_32,   d_cc(ਨਿਰਾ),                 d_binOpList (u32, LessThan) ),             // 0x49
    M3OP( "i32.gt_s",              -1, i_32,   d_cc(ਨਿਰਾ),                 d_binOpList (i32, GreaterThan) ),          // 0x4a
    M3OP( "i32.gt_u",              -1, i_32,   d_cc(ਨਿਰਾ),                 d_binOpList (u32, GreaterThan) ),          // 0x4b
    M3OP( "i32.le_s",              -1, i_32,   d_cc(ਨਿਰਾ),                 d_binOpList (i32, LessThanOrEqual) ),      // 0x4c
    M3OP( "i32.le_u",              -1, i_32,   d_cc(ਨਿਰਾ),                 d_binOpList (u32, LessThanOrEqual) ),      // 0x4d
    M3OP( "i32.ge_s",              -1, i_32,   d_cc(ਨਿਰਾ),                 d_binOpList (i32, GreaterThanOrEqual) ),   // 0x4e
    M3OP( "i32.ge_u",              -1, i_32,   d_cc(ਨਿਰਾ),                 d_binOpList (u32, GreaterThanOrEqual) ),   // 0x4f

    M3OP( "i64.eqz",               0,  i_32,   d_cc(ਨਿਰਾ),                 d_unaryOpList (i64, Equal_ਮੰਜ਼ਿਲ_Zero) ),        // 0x50
    M3OP( "i64.eq",                -1, i_32,   d_cc(ਨਿਰਾ),                 d_commutativeBinOpList (i64, Equal) ),     // 0x51
    M3OP( "i64.ne",                -1, i_32,   d_cc(ਨਿਰਾ),                 d_commutativeBinOpList (i64, NotEqual) ),  // 0x52
    M3OP( "i64.lt_s",              -1, i_32,   d_cc(ਨਿਰਾ),                 d_binOpList (i64, LessThan) ),             // 0x53
    M3OP( "i64.lt_u",              -1, i_32,   d_cc(ਨਿਰਾ),                 d_binOpList (u64, LessThan) ),             // 0x54
    M3OP( "i64.gt_s",              -1, i_32,   d_cc(ਨਿਰਾ),                 d_binOpList (i64, GreaterThan) ),          // 0x55
    M3OP( "i64.gt_u",              -1, i_32,   d_cc(ਨਿਰਾ),                 d_binOpList (u64, GreaterThan) ),          // 0x56
    M3OP( "i64.le_s",              -1, i_32,   d_cc(ਨਿਰਾ),                 d_binOpList (i64, LessThanOrEqual) ),      // 0x57
    M3OP( "i64.le_u",              -1, i_32,   d_cc(ਨਿਰਾ),                 d_binOpList (u64, LessThanOrEqual) ),      // 0x58
    M3OP( "i64.ge_s",              -1, i_32,   d_cc(ਨਿਰਾ),                 d_binOpList (i64, GreaterThanOrEqual) ),   // 0x59
    M3OP( "i64.ge_u",              -1, i_32,   d_cc(ਨਿਰਾ),                 d_binOpList (u64, GreaterThanOrEqual) ),   // 0x5a

    M3OP_F( "f32.eq",              -1, i_32,   d_cc(ਨਿਰਾ),                 d_commutativeBinOpList (f32, Equal) ),     // 0x5b
    M3OP_F( "f32.ne",              -1, i_32,   d_cc(ਨਿਰਾ),                 d_commutativeBinOpList (f32, NotEqual) ),  // 0x5c
    M3OP_F( "f32.lt",              -1, i_32,   d_cc(ਨਿਰਾ),                 d_binOpList (f32, LessThan) ),             // 0x5d
    M3OP_F( "f32.gt",              -1, i_32,   d_cc(ਨਿਰਾ),                 d_binOpList (f32, GreaterThan) ),          // 0x5e
    M3OP_F( "f32.le",              -1, i_32,   d_cc(ਨਿਰਾ),                 d_binOpList (f32, LessThanOrEqual) ),      // 0x5f
    M3OP_F( "f32.ge",              -1, i_32,   d_cc(ਨਿਰਾ),                 d_binOpList (f32, GreaterThanOrEqual) ),   // 0x60

    M3OP_F( "f64.eq",              -1, i_32,   d_cc(ਨਿਰਾ),                 d_commutativeBinOpList (f64, Equal) ),     // 0x61
    M3OP_F( "f64.ne",              -1, i_32,   d_cc(ਨਿਰਾ),                 d_commutativeBinOpList (f64, NotEqual) ),  // 0x62
    M3OP_F( "f64.lt",              -1, i_32,   d_cc(ਨਿਰਾ),                 d_binOpList (f64, LessThan) ),             // 0x63
    M3OP_F( "f64.gt",              -1, i_32,   d_cc(ਨਿਰਾ),                 d_binOpList (f64, GreaterThan) ),          // 0x64
    M3OP_F( "f64.le",              -1, i_32,   d_cc(ਨਿਰਾ),                 d_binOpList (f64, LessThanOrEqual) ),      // 0x65
    M3OP_F( "f64.ge",              -1, i_32,   d_cc(ਨਿਰਾ),                 d_binOpList (f64, GreaterThanOrEqual) ),   // 0x66

    M3OP( "i32.clz",               0,  i_32,   d_cc(ਨਿਰਾ),                 d_unaryOpList (u32, Clz) ),                // 0x67
    M3OP( "i32.ctz",               0,  i_32,   d_cc(ਨਿਰਾ),                 d_unaryOpList (u32, Ctz) ),                // 0x68
    M3OP( "i32.popcnt",            0,  i_32,   d_cc(ਨਿਰਾ),                 d_unaryOpList (u32, Popcnt) ),             // 0x69

    M3OP( "i32.add",               -1, i_32,   d_cc(ਨਿਰਾ),                 d_commutativeBinOpList (i32, ਜੋੜੋ) ),       // 0x6a
    M3OP( "i32.sub",               -1, i_32,   d_cc(ਨਿਰਾ),                 d_binOpList (i32, ਘਟਾਓ) ),             // 0x6b
    M3OP( "i32.mul",               -1, i_32,   d_cc(ਨਿਰਾ),                 d_commutativeBinOpList (i32, Multiply) ),  // 0x6c
    M3OP( "i32.div_s",             -1, i_32,   d_cc(ਨਿਰਾ),                 d_binOpList (i32, Divide) ),               // 0x6d
    M3OP( "i32.div_u",             -1, i_32,   d_cc(ਨਿਰਾ),                 d_binOpList (u32, Divide) ),               // 0x6e
    M3OP( "i32.rem_s",             -1, i_32,   d_cc(ਨਿਰਾ),                 d_binOpList (i32, Remainder) ),            // 0x6f
    M3OP( "i32.rem_u",             -1, i_32,   d_cc(ਨਿਰਾ),                 d_binOpList (u32, Remainder) ),            // 0x70
    M3OP( "i32.and",               -1, i_32,   d_cc(ਨਿਰਾ),                 d_commutativeBinOpList (u32, And) ),       // 0x71
    M3OP( "i32.or",                -1, i_32,   d_cc(ਨਿਰਾ),                 d_commutativeBinOpList (u32, Or) ),        // 0x72
    M3OP( "i32.xor",               -1, i_32,   d_cc(ਨਿਰਾ),                 d_commutativeBinOpList (u32, Xor) ),       // 0x73
    M3OP( "i32.shl",               -1, i_32,   d_cc(ਨਿਰਾ),                 d_binOpList (u32, ਖਿਸਕਾਓ_ਖੱਬਾ) ),            // 0x74
    M3OP( "i32.shr_s",             -1, i_32,   d_cc(ਨਿਰਾ),                 d_binOpList (i32, ਖਿਸਕਾਓ_ਸੱਜਾ) ),           // 0x75
    M3OP( "i32.shr_u",             -1, i_32,   d_cc(ਨਿਰਾ),                 d_binOpList (u32, ਖਿਸਕਾਓ_ਸੱਜਾ) ),           // 0x76
    M3OP( "i32.rotl",              -1, i_32,   d_cc(ਨਿਰਾ),                 d_binOpList (u32, Rotl) ),                 // 0x77
    M3OP( "i32.rotr",              -1, i_32,   d_cc(ਨਿਰਾ),                 d_binOpList (u32, Rotr) ),                 // 0x78

    M3OP( "i64.clz",               0,  i_64,   d_cc(ਨਿਰਾ),                 d_unaryOpList (u64, Clz) ),                // 0x79
    M3OP( "i64.ctz",               0,  i_64,   d_cc(ਨਿਰਾ),                 d_unaryOpList (u64, Ctz) ),                // 0x7a
    M3OP( "i64.popcnt",            0,  i_64,   d_cc(ਨਿਰਾ),                 d_unaryOpList (u64, Popcnt) ),             // 0x7b

    M3OP( "i64.add",               -1, i_64,   d_cc(ਨਿਰਾ),                 d_commutativeBinOpList (i64, ਜੋੜੋ) ),       // 0x7c
    M3OP( "i64.sub",               -1, i_64,   d_cc(ਨਿਰਾ),                 d_binOpList (i64, ਘਟਾਓ) ),             // 0x7d
    M3OP( "i64.mul",               -1, i_64,   d_cc(ਨਿਰਾ),                 d_commutativeBinOpList (i64, Multiply) ),  // 0x7e
    M3OP( "i64.div_s",             -1, i_64,   d_cc(ਨਿਰਾ),                 d_binOpList (i64, Divide) ),               // 0x7f
    M3OP( "i64.div_u",             -1, i_64,   d_cc(ਨਿਰਾ),                 d_binOpList (u64, Divide) ),               // 0x80
    M3OP( "i64.rem_s",             -1, i_64,   d_cc(ਨਿਰਾ),                 d_binOpList (i64, Remainder) ),            // 0x81
    M3OP( "i64.rem_u",             -1, i_64,   d_cc(ਨਿਰਾ),                 d_binOpList (u64, Remainder) ),            // 0x82
    M3OP( "i64.and",               -1, i_64,   d_cc(ਨਿਰਾ),                 d_commutativeBinOpList (u64, And) ),       // 0x83
    M3OP( "i64.or",                -1, i_64,   d_cc(ਨਿਰਾ),                 d_commutativeBinOpList (u64, Or) ),        // 0x84
    M3OP( "i64.xor",               -1, i_64,   d_cc(ਨਿਰਾ),                 d_commutativeBinOpList (u64, Xor) ),       // 0x85
    M3OP( "i64.shl",               -1, i_64,   d_cc(ਨਿਰਾ),                 d_binOpList (u64, ਖਿਸਕਾਓ_ਖੱਬਾ) ),            // 0x86
    M3OP( "i64.shr_s",             -1, i_64,   d_cc(ਨਿਰਾ),                 d_binOpList (i64, ਖਿਸਕਾਓ_ਸੱਜਾ) ),           // 0x87
    M3OP( "i64.shr_u",             -1, i_64,   d_cc(ਨਿਰਾ),                 d_binOpList (u64, ਖਿਸਕਾਓ_ਸੱਜਾ) ),           // 0x88
    M3OP( "i64.rotl",              -1, i_64,   d_cc(ਨਿਰਾ),                 d_binOpList (u64, Rotl) ),                 // 0x89
    M3OP( "i64.rotr",              -1, i_64,   d_cc(ਨਿਰਾ),                 d_binOpList (u64, Rotr) ),                 // 0x8a

    M3OP_F( "f32.abs",             0,  f_32,   d_cc(ਨਿਰਾ),                 d_unaryOpList(f32, Abs) ),                 // 0x8b
    M3OP_F( "f32.neg",             0,  f_32,   d_cc(ਨਿਰਾ),                 d_unaryOpList(f32, Negate) ),              // 0x8c
    M3OP_F( "f32.ceil",            0,  f_32,   d_cc(ਨਿਰਾ),                 d_unaryOpList(f32, Ceil) ),                // 0x8d
    M3OP_F( "f32.floor",           0,  f_32,   d_cc(ਨਿਰਾ),                 d_unaryOpList(f32, Floor) ),               // 0x8e
    M3OP_F( "f32.trunc",           0,  f_32,   d_cc(ਨਿਰਾ),                 d_unaryOpList(f32, Trunc) ),               // 0x8f
    M3OP_F( "f32.nearest",         0,  f_32,   d_cc(ਨਿਰਾ),                 d_unaryOpList(f32, ਸਭ_ਤੋਂ_ਨੇੜੇ) ),             // 0x90
    M3OP_F( "f32.sqrt",            0,  f_32,   d_cc(ਨਿਰਾ),                 d_unaryOpList(f32, Sqrt) ),                // 0x91

    M3OP_F( "f32.add",             -1, f_32,   d_cc(ਨਿਰਾ),                 d_commutativeBinOpList (f32, ਜੋੜੋ) ),       // 0x92
    M3OP_F( "f32.sub",             -1, f_32,   d_cc(ਨਿਰਾ),                 d_binOpList (f32, ਘਟਾਓ) ),             // 0x93
    M3OP_F( "f32.mul",             -1, f_32,   d_cc(ਨਿਰਾ),                 d_commutativeBinOpList (f32, Multiply) ),  // 0x94
    M3OP_F( "f32.div",             -1, f_32,   d_cc(ਨਿਰਾ),                 d_binOpList (f32, Divide) ),               // 0x95
    M3OP_F( "f32.min",             -1, f_32,   d_cc(ਨਿਰਾ),                 d_commutativeBinOpList (f32, Min) ),       // 0x96
    M3OP_F( "f32.max",             -1, f_32,   d_cc(ਨਿਰਾ),                 d_commutativeBinOpList (f32, Max) ),       // 0x97
    M3OP_F( "f32.copysign",        -1, f_32,   d_cc(ਨਿਰਾ),                 d_binOpList (f32, CopySign) ),             // 0x98

    M3OP_F( "f64.abs",             0,  f_64,   d_cc(ਨਿਰਾ),                 d_unaryOpList(f64, Abs) ),                 // 0x99
    M3OP_F( "f64.neg",             0,  f_64,   d_cc(ਨਿਰਾ),                 d_unaryOpList(f64, Negate) ),              // 0x9a
    M3OP_F( "f64.ceil",            0,  f_64,   d_cc(ਨਿਰਾ),                 d_unaryOpList(f64, Ceil) ),                // 0x9b
    M3OP_F( "f64.floor",           0,  f_64,   d_cc(ਨਿਰਾ),                 d_unaryOpList(f64, Floor) ),               // 0x9c
    M3OP_F( "f64.trunc",           0,  f_64,   d_cc(ਨਿਰਾ),                 d_unaryOpList(f64, Trunc) ),               // 0x9d
    M3OP_F( "f64.nearest",         0,  f_64,   d_cc(ਨਿਰਾ),                 d_unaryOpList(f64, ਸਭ_ਤੋਂ_ਨੇੜੇ) ),             // 0x9e
    M3OP_F( "f64.sqrt",            0,  f_64,   d_cc(ਨਿਰਾ),                 d_unaryOpList(f64, Sqrt) ),                // 0x9f

    M3OP_F( "f64.add",             -1, f_64,   d_cc(ਨਿਰਾ),                 d_commutativeBinOpList (f64, ਜੋੜੋ) ),       // 0xa0
    M3OP_F( "f64.sub",             -1, f_64,   d_cc(ਨਿਰਾ),                 d_binOpList (f64, ਘਟਾਓ) ),             // 0xa1
    M3OP_F( "f64.mul",             -1, f_64,   d_cc(ਨਿਰਾ),                 d_commutativeBinOpList (f64, Multiply) ),  // 0xa2
    M3OP_F( "f64.div",             -1, f_64,   d_cc(ਨਿਰਾ),                 d_binOpList (f64, Divide) ),               // 0xa3
    M3OP_F( "f64.min",             -1, f_64,   d_cc(ਨਿਰਾ),                 d_commutativeBinOpList (f64, Min) ),       // 0xa4
    M3OP_F( "f64.max",             -1, f_64,   d_cc(ਨਿਰਾ),                 d_commutativeBinOpList (f64, Max) ),       // 0xa5
    M3OP_F( "f64.copysign",        -1, f_64,   d_cc(ਨਿਰਾ),                 d_binOpList (f64, CopySign) ),             // 0xa6

    M3OP( "i32.wrap/i64",          0,  i_32,   d_cc(ਨਿਰਾ),                 d_unaryOpList (i32, Wrap_i64) ),           // 0xa7
    M3OP_F( "i32.trunc_s/f32",     0,  i_32,   d_cc(Compile_Convert),      d_convertOpList (i32_Trunc_f32) ),         // 0xa8
    M3OP_F( "i32.trunc_u/f32",     0,  i_32,   d_cc(Compile_Convert),      d_convertOpList (u32_Trunc_f32) ),         // 0xa9
    M3OP_F( "i32.trunc_s/f64",     0,  i_32,   d_cc(Compile_Convert),      d_convertOpList (i32_Trunc_f64) ),         // 0xaa
    M3OP_F( "i32.trunc_u/f64",     0,  i_32,   d_cc(Compile_Convert),      d_convertOpList (u32_Trunc_f64) ),         // 0xab

    M3OP( "i64.extend_s/i32",      0,  i_64,   d_cc(ਨਿਰਾ),                 d_unaryOpList (i64, Extend_i32) ),         // 0xac
    M3OP( "i64.extend_u/i32",      0,  i_64,   d_cc(ਨਿਰਾ),                 d_unaryOpList (i64, Extend_u32) ),         // 0xad

    M3OP_F( "i64.trunc_s/f32",     0,  i_64,   d_cc(Compile_Convert),      d_convertOpList (i64_Trunc_f32) ),         // 0xae
    M3OP_F( "i64.trunc_u/f32",     0,  i_64,   d_cc(Compile_Convert),      d_convertOpList (u64_Trunc_f32) ),         // 0xaf
    M3OP_F( "i64.trunc_s/f64",     0,  i_64,   d_cc(Compile_Convert),      d_convertOpList (i64_Trunc_f64) ),         // 0xb0
    M3OP_F( "i64.trunc_u/f64",     0,  i_64,   d_cc(Compile_Convert),      d_convertOpList (u64_Trunc_f64) ),         // 0xb1

    M3OP_F( "f32.convert_s/i32",   0,  f_32,   d_cc(Compile_Convert),      d_convertOpList (f32_Convert_i32) ),       // 0xb2
    M3OP_F( "f32.convert_u/i32",   0,  f_32,   d_cc(Compile_Convert),      d_convertOpList (f32_Convert_u32) ),       // 0xb3
    M3OP_F( "f32.convert_s/i64",   0,  f_32,   d_cc(Compile_Convert),      d_convertOpList (f32_Convert_i64) ),       // 0xb4
    M3OP_F( "f32.convert_u/i64",   0,  f_32,   d_cc(Compile_Convert),      d_convertOpList (f32_Convert_u64) ),       // 0xb5

    M3OP_F( "f32.demote/f64",      0,  f_32,   d_cc(ਨਿਰਾ),                 d_unaryOpList (f32, Demote_f64) ),         // 0xb6

    M3OP_F( "f64.convert_s/i32",   0,  f_64,   d_cc(Compile_Convert),      d_convertOpList (f64_Convert_i32) ),       // 0xb7
    M3OP_F( "f64.convert_u/i32",   0,  f_64,   d_cc(Compile_Convert),      d_convertOpList (f64_Convert_u32) ),       // 0xb8
    M3OP_F( "f64.convert_s/i64",   0,  f_64,   d_cc(Compile_Convert),      d_convertOpList (f64_Convert_i64) ),       // 0xb9
    M3OP_F( "f64.convert_u/i64",   0,  f_64,   d_cc(Compile_Convert),      d_convertOpList (f64_Convert_u64) ),       // 0xba

    M3OP_F( "f64.promote/f32",     0,  f_64,   d_cc(ਨਿਰਾ),                 d_unaryOpList (f64, Promote_f32) ),        // 0xbb

    M3OP_F( "i32.reinterpret/f32", 0,  i_32,   d_cc(Compile_Convert),      d_convertOpList (i32_Reinterpret_f32) ),   // 0xbc
    M3OP_F( "i64.reinterpret/f64", 0,  i_64,   d_cc(Compile_Convert),      d_convertOpList (i64_Reinterpret_f64) ),   // 0xbd
    M3OP_F( "f32.reinterpret/i32", 0,  f_32,   d_cc(Compile_Convert),      d_convertOpList (f32_Reinterpret_i32) ),   // 0xbe
    M3OP_F( "f64.reinterpret/i64", 0,  f_64,   d_cc(Compile_Convert),      d_convertOpList (f64_Reinterpret_i64) ),   // 0xbf

    M3OP( "i32.extend8_s",         0,  i_32,   d_cc(ਨਿਰਾ),                 d_unaryOpList (i32, Extend8_s) ),          // 0xc0
    M3OP( "i32.extend16_s",        0,  i_32,   d_cc(ਨਿਰਾ),                 d_unaryOpList (i32, Extend16_s) ),         // 0xc1
    M3OP( "i64.extend8_s",         0,  i_64,   d_cc(ਨਿਰਾ),                 d_unaryOpList (i64, Extend8_s) ),          // 0xc2
    M3OP( "i64.extend16_s",        0,  i_64,   d_cc(ਨਿਰਾ),                 d_unaryOpList (i64, Extend16_s) ),         // 0xc3
    M3OP( "i64.extend32_s",        0,  i_64,   d_cc(ਨਿਰਾ),                 d_unaryOpList (i64, Extend32_s) ),         // 0xc4

// Internal operations, for codepage logging only. They sit past c_waOp_lastCore,
// the last opcode this table covers, so GetOpInfo () can never reach them.
# ifdef DEBUG // for codepage logging. the order doesn't matter:
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ d_m3DebugOp(OP) M3OP (#OP, 0, none, d_ops (op_##OP))

# ਜੇ d_m3HasFloat
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ d_m3DebugTypedOp(OP) M3OP (#OP, 0, none, d_ops (op_##OP##_i32, op_##OP##_i64, op_##OP##_f32, op_##OP##_f64))
# ਨਹੀਂਤਾਂ
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ d_m3DebugTypedOp(OP) M3OP (#OP, 0, none, d_ops (op_##OP##_i32, op_##OP##_i64))
# endif

    d_m3DebugOp (Compile),          d_m3DebugOp (ਇੰਦਰਾਜ_2),            d_m3DebugOp (ਸਿਰਾ_2),
    d_m3DebugOp (Unsupported),      d_m3DebugOp (CallRawFunction),

    d_m3DebugOp (GetGlobal_s32),    d_m3DebugOp (GetGlobal_s64),    d_m3DebugOp (ContinueLoop),     d_m3DebugOp (ContinueLoopIf),
# ਜੇ d_m3_Has_ਢੇਰ_Switching
    d_m3DebugOp (ContinueLoop_Suspendable), d_m3DebugOp (ContinueLoopIf_Suspendable), d_m3DebugOp (ਇੰਦਰਾਜ_Check),
#  ਜੇ !d_m3_ਇੰਦਰਾਜ_Keeps_ਚੌਖਟਾ
    d_m3DebugOp (ਇੰਦਰਾਜ_Suspendable),
#  endif
# endif

    d_m3DebugOp (Copy_ਥਾਂ_32),      d_m3DebugOp (Preserve_Copy_ਥਾਂ_32), d_m3DebugOp (If_s),          d_m3DebugOp (BranchIfPrologue_s),
    d_m3DebugOp (Copy_ਥਾਂ_64),      d_m3DebugOp (Preserve_Copy_ਥਾਂ_64), d_m3DebugOp (If_r),          d_m3DebugOp (BranchIfPrologue_r),

    d_m3DebugOp (Select_i32_rss),   d_m3DebugOp (Select_i32_srs),   d_m3DebugOp (Select_i32_ssr),   d_m3DebugOp (Select_i32_sss),
    d_m3DebugOp (Select_i64_rss),   d_m3DebugOp (Select_i64_srs),   d_m3DebugOp (Select_i64_ssr),   d_m3DebugOp (Select_i64_sss),

# ਜੇ d_m3HasFloat
    d_m3DebugOp (Select_f32_sss),   d_m3DebugOp (Select_f32_srs),   d_m3DebugOp (Select_f32_ssr),
    d_m3DebugOp (Select_f32_rss),   d_m3DebugOp (Select_f32_rrs),   d_m3DebugOp (Select_f32_rsr),

    d_m3DebugOp (Select_f64_sss),   d_m3DebugOp (Select_f64_srs),   d_m3DebugOp (Select_f64_ssr),
    d_m3DebugOp (Select_f64_rss),   d_m3DebugOp (Select_f64_rrs),   d_m3DebugOp (Select_f64_rsr),
# endif

    d_m3DebugOp (MemFill),          d_m3DebugOp (MemCopy),          d_m3DebugOp (MemInit),          d_m3DebugOp (ਅੰਕੜੇ_Drop),

# ਜੇ d_m3HasGasMetering
    d_m3DebugOp (UseGas),
# endif

# ਜੇ d_m3HasRefTypes
    d_m3DebugOp (ਸਾਰਣੀ_Get),         d_m3DebugOp (ਸਾਰਣੀ_ਸੈੱਟ),         d_m3DebugOp (ਸਾਰਣੀ_ਮਾਪ),
    d_m3DebugOp (ਸਾਰਣੀ_Grow),        d_m3DebugOp (ਸਾਰਣੀ_Fill),        d_m3DebugOp (ਸਾਰਣੀ_Init),
    d_m3DebugOp (ElemDrop),         d_m3DebugOp (ਸਾਰਣੀ_Copy),
# endif

    d_m3DebugTypedOp (ਸੈੱਟ_Global),   d_m3DebugOp (ਸੈੱਟ_Global_s32),    d_m3DebugOp (ਸੈੱਟ_Global_s64),

    d_m3DebugTypedOp (ਸੈੱਟ_ਰਜਿਸਟਰ), d_m3DebugTypedOp (ਸੈੱਟ_ਥਾਂ),     d_m3DebugTypedOp (Preserve_ਸੈੱਟ_ਥਾਂ),
# endif

# ifdef DEBUG
    M3OP( "termination", 0, c_m3Type_unknown, d_ਖਾਲੀ_ਥਾਵਾਂ_Op_List ) // for find_operation_info
# endif
};

ਅਚਲ M3OpInfo c_operationsFC[] =
{
    M3OP_F( "i32.trunc_s:sat/f32", 0, i_32,   d_cc(Compile_Convert),         d_convertOpList (i32_TruncSat_f32) ),  // 0x00
    M3OP_F( "i32.trunc_u:sat/f32", 0, i_32,   d_cc(Compile_Convert),         d_convertOpList (u32_TruncSat_f32) ),  // 0x01
    M3OP_F( "i32.trunc_s:sat/f64", 0, i_32,   d_cc(Compile_Convert),         d_convertOpList (i32_TruncSat_f64) ),  // 0x02
    M3OP_F( "i32.trunc_u:sat/f64", 0, i_32,   d_cc(Compile_Convert),         d_convertOpList (u32_TruncSat_f64) ),  // 0x03
    M3OP_F( "i64.trunc_s:sat/f32", 0, i_64,   d_cc(Compile_Convert),         d_convertOpList (i64_TruncSat_f32) ),  // 0x04
    M3OP_F( "i64.trunc_u:sat/f32", 0, i_64,   d_cc(Compile_Convert),         d_convertOpList (u64_TruncSat_f32) ),  // 0x05
    M3OP_F( "i64.trunc_s:sat/f64", 0, i_64,   d_cc(Compile_Convert),         d_convertOpList (i64_TruncSat_f64) ),  // 0x06
    M3OP_F( "i64.trunc_u:sat/f64", 0, i_64,   d_cc(Compile_Convert),         d_convertOpList (u64_TruncSat_f64) ),  // 0x07

    M3OP( "memory.init",           0, none,   d_cc(Compile_ਸਮ੍ਰਿਤੀ_Init),     d_ਖਾਲੀ_ਥਾਵਾਂ_Op_List ),                       // 0x08
    M3OP( "data.drop",             0, none,   d_cc(Compile_ਅੰਕੜੇ_Drop),       d_ਖਾਲੀ_ਥਾਵਾਂ_Op_List ),                       // 0x09

    M3OP( "memory.copy",           0, none,   d_cc(Compile_ਸਮ੍ਰਿਤੀ_Copy_Fill), d_ਖਾਲੀ_ਥਾਵਾਂ_Op_List ),                       // 0x0a
    M3OP( "memory.fill",           0, none,   d_cc(Compile_ਸਮ੍ਰਿਤੀ_Copy_Fill), d_ਖਾਲੀ_ਥਾਵਾਂ_Op_List ),                       // 0x0b

#ਜੇ d_m3HasRefTypes
    M3OP( "table.init",            0, none,   d_cc(Compile_ਸਾਰਣੀ_Init),      d_ਖਾਲੀ_ਥਾਵਾਂ_Op_List ),                       // 0x0c
    M3OP( "elem.drop",             0, none,   d_cc(Compile_Elem_Drop),       d_ਖਾਲੀ_ਥਾਵਾਂ_Op_List ),                       // 0x0d
    M3OP( "table.copy",            0, none,   d_cc(Compile_ਸਾਰਣੀ_Copy),      d_ਖਾਲੀ_ਥਾਵਾਂ_Op_List ),                       // 0x0e
#ਨਹੀਂਤਾਂ
    M3OP_RESERVED, M3OP_RESERVED, M3OP_RESERVED,                                                                    // 0x0c...0x0e
#ਜੇ_ਅੰਤ

#ਜੇ d_m3HasRefTypes
    M3OP( "table.grow",            0, i_32,   d_cc(Compile_ਸਾਰਣੀ_Grow_Fill),  d_ਖਾਲੀ_ਥਾਵਾਂ_Op_List ),                       // 0x0f
    M3OP( "table.size",            1, i_32,   d_cc(Compile_ਸਾਰਣੀ_ਮਾਪ),      d_ਖਾਲੀ_ਥਾਵਾਂ_Op_List ),                       // 0x10
    M3OP( "table.fill",            0, none,   d_cc(Compile_ਸਾਰਣੀ_Grow_Fill),  d_ਖਾਲੀ_ਥਾਵਾਂ_Op_List ),                       // 0x11
#ਨਹੀਂਤਾਂ
    M3OP_RESERVED, M3OP_RESERVED, M3OP_RESERVED,                                                                    // 0x0f...0x11
#ਜੇ_ਅੰਤ

#ਜੇ d_m3HasWideArithmetic
    M3OP_RESERVED,                                                                                                  // 0x12

    M3OP( "i64.add128",            -2, i_64,  d_cc(Compile_WideArithmetic), d_logOp (i64_Add128) ),              // 0x13
    M3OP( "i64.sub128",            -2, i_64,  d_cc(Compile_WideArithmetic), d_logOp (i64_Sub128) ),              // 0x14
    M3OP( "i64.mul_wide_s",        0,  i_64,  d_cc(Compile_WideArithmetic), d_logOp (i64_MultiplyWide) ),        // 0x15
    M3OP( "i64.mul_wide_u",        0,  i_64,  d_cc(Compile_WideArithmetic), d_logOp (u64_MultiplyWide) ),        // 0x16
#ਜੇ_ਅੰਤ


# ifdef DEBUG
    M3OP( "termination", 0, c_m3Type_unknown, d_ਖਾਲੀ_ਥਾਵਾਂ_Op_List ) // for find_operation_info
# endif
};

#ਜੇ d_m3HasAtomics

// The 0xFE instructions share one entry per kind rather than each having its own: what
// tells them apart, the width of the cell and the operation, is worked out from the
// sub-opcode by Compile_Atomic. GetAtomicInfoIndex () maps a sub-opcode onto this table.
ਗਿਣਤੀ {
    c_opAtomic_notify,
    c_opAtomic_wait32,
    c_opAtomic_wait64,
    c_opAtomic_fence,
    c_op_Atomic_ਲੱਦੋ,
    c_opAtomic_store,
    c_opAtomic_rmw,
    c_opAtomic_cmpxchg
};

ਅਚਲ M3OpInfo c_operationsFE[] =
{
    M3OP( "memory.atomic.notify",  -1, i_32,  d_cc(Compile_Atomic), d_logOp (AtomicNotify) ),    // c_opAtomic_notify
    M3OP( "memory.atomic.wait32",  -2, i_32,  d_cc(Compile_Atomic), d_logOp (AtomicWait32) ),    // c_opAtomic_wait32
    M3OP( "memory.atomic.wait64",  -2, i_32,  d_cc(Compile_Atomic), d_logOp (AtomicWait64) ),    // c_opAtomic_wait64
    M3OP( "atomic.fence",           0, none,  d_cc(Compile_Atomic), d_ਖਾਲੀ_ਥਾਵਾਂ_Op_List ),             // c_opAtomic_fence
    M3OP( "atomic.load",            0, any,   d_cc(Compile_Atomic), d_logOp (Atomic_ਲੱਦੋ) ),      // c_opAtomic_load
    M3OP( "atomic.store",          -2, none,  d_cc(Compile_Atomic), d_logOp (AtomicStore) ),     // c_opAtomic_store
    M3OP( "atomic.rmw",            -1, any,   d_cc(Compile_Atomic), d_logOp (AtomicRmw) ),       // c_opAtomic_rmw
    M3OP( "atomic.rmw.cmpxchg",    -2, any,   d_cc(Compile_Atomic), d_logOp (AtomicCmpxchg) ),   // c_opAtomic_cmpxchg
};

ਅਚਲ u32 c_ਅੰਕ_Operations_FE = M3_ਗਿਣਤੀ_ਦਾ(c_operationsFE);

// The entry a 0xFE sub-opcode uses, or -1 for one that names nothing: the spec leaves
// 0x04..0x0f and everything past cmpxchg unassigned. The accesses come in runs of seven,
// one per pairing of value type and cell width.
ਸਥਿਰ ਸਤਰਅੰਦਰ
i32 Get_Atomic_Info_ਸੂਚਕਾਂਕ (u32 i_sub)
{
    ਜੇ (i_sub <= 0x03) ਵਾਪਸ (i32)i_sub;
    ਜੇ (i_sub < 0x10) ਵਾਪਸ -1;
    ਜੇ (i_sub <= 0x16) ਵਾਪਸ c_op_Atomic_ਲੱਦੋ;
    ਜੇ (i_sub <= 0x1d) ਵਾਪਸ c_opAtomic_store;
    ਜੇ (i_sub <= 0x47) ਵਾਪਸ c_opAtomic_rmw;
    ਜੇ (i_sub <= c_wa_Op_ਆਖਰੀ_Atomic) ਵਾਪਸ c_opAtomic_cmpxchg;
    ਵਾਪਸ -1;
}

#ਜੇ_ਅੰਤ // d_m3HasAtomics

// The opcodes above c_waOp_lastCore are sparse: the reference instructions at
// 0xd0..0xd2 and 0xd4, and the 0xfc extended-opcode prefix. Indexing c_operations out to
// 0xfc to reach them would leave ~50 empty entries there, so they get their own
// table, which GetHighOpIndex () maps the opcode onto.
ਗਿਣਤੀ {
#ਜੇ d_m3HasRefTypes
    c_op_ਉੱਚਾ_ref_Null,
    c_op_ਉੱਚਾ_ref_Is_Null,
    c_op_ਉੱਚਾ_ref_Func,
#  ਜੇ d_m3HasTypedRefs
    c_op_ਉੱਚਾ_ref_As_Non_Null,
#  endif
#ਜੇ_ਅੰਤ
#ਜੇ d_m3_Has_ਢੇਰ_Switching
    c_op_ਉੱਚਾ_cont_New,
    c_op_ਉੱਚਾ_cont_Bind,
    c_op_ਉੱਚਾ_suspend,
    c_op_ਉੱਚਾ_resume,
    c_op_ਉੱਚਾ_resume_Throw,
    c_op_ਉੱਚਾ_resume_Throw_Ref,
    c_op_ਉੱਚਾ_switch,
#ਜੇ_ਅੰਤ
#ਜੇ d_m3HasAtomics
    c_op_ਉੱਚਾ_atomic,
#ਜੇ_ਅੰਤ
    c_op_ਉੱਚਾ_extended   // always present, so the enum is never empty
};

// Positional, so the table must stay in c_opHigh_* order: array designators
// are not valid C++, which the maintenance builds compile this as.
ਸਥਿਰ ਅਚਲ M3OpInfo c_operations_ਉੱਚਾ[] =
{
#ਜੇ d_m3HasRefTypes
    M3OP( "ref.null",        1, any,               d_cc(Compile_Ref_Null),       d_ਖਾਲੀ_ਥਾਵਾਂ_Op_List ),  // 0xd0  c_opHigh_refNull
    M3OP( "ref.is_null",     0, i_32,              d_cc(Compile_Ref_IsNull),     d_ਖਾਲੀ_ਥਾਵਾਂ_Op_List ),  // 0xd1  c_opHigh_refIsNull
    M3OP( "ref.func",        1, any,               d_cc(Compile_Ref_Func),       d_ਖਾਲੀ_ਥਾਵਾਂ_Op_List ),  // 0xd2  c_opHigh_refFunc
#  ਜੇ d_m3HasTypedRefs
    M3OP( "ref.as_non_null", 0, any,               d_cc(Compile_Ref_AsNonNull),  d_ਖਾਲੀ_ਥਾਵਾਂ_Op_List ),  // 0xd4  c_opHigh_refAsNonNull
#  endif
#ਜੇ_ਅੰਤ
#ਜੇ d_m3_Has_ਢੇਰ_Switching
    M3OP( "cont.new",          1, any,             d_cc(Compile_ContNew),        d_ਖਾਲੀ_ਥਾਵਾਂ_Op_List ),  // 0xe0  c_opHigh_contNew
    M3OP( "cont.bind",         2, any,             d_cc(Compile_ContBind),       d_ਖਾਲੀ_ਥਾਵਾਂ_Op_List ),  // 0xe1  c_opHigh_contBind
    M3OP( "suspend",           1, any,             d_cc(Compile_Suspend),        d_ਖਾਲੀ_ਥਾਵਾਂ_Op_List ),  // 0xe2  c_opHigh_suspend
    M3OP( "resume",            1, any,             d_cc(Compile_Resume),         d_ਖਾਲੀ_ਥਾਵਾਂ_Op_List ),  // 0xe3  c_opHigh_resume
    M3OP( "resume_throw",      2, any,             d_cc(Compile_Resume)   ,    d_ਖਾਲੀ_ਥਾਵਾਂ_Op_List ),  // 0xe4  c_opHigh_resumeThrow
    M3OP( "resume_throw_ref",  1, any,             d_cc(Compile_Resume)      , d_ਖਾਲੀ_ਥਾਵਾਂ_Op_List ),  // 0xe5  c_opHigh_resumeThrowRef
    M3OP( "switch",            2, any,             d_cc(Compile_Switch),         d_ਖਾਲੀ_ਥਾਵਾਂ_Op_List ),  // 0xe6  c_opHigh_switch
#ਜੇ_ਅੰਤ

#ਜੇ d_m3HasAtomics
    M3OP( "0xFE",            0, c_m3Type_unknown,  d_cc(Compile_ExtendedOpcode), d_ਖਾਲੀ_ਥਾਵਾਂ_Op_List ),  // 0xfe  c_opHigh_atomic
#ਜੇ_ਅੰਤ
    M3OP( "0xFC",            0, c_m3Type_unknown,  d_cc(Compile_ExtendedOpcode), d_ਖਾਲੀ_ਥਾਵਾਂ_Op_List ),  // 0xfc  c_opHigh_extended
};
M3_STATIC_ASSERT (M3_ਗਿਣਤੀ_ਦਾ (c_operations_ਉੱਚਾ) == c_op_ਉੱਚਾ_extended + 1, c_operations_ਉੱਚਾ_needs_one_ਇੰਦਰਾਜ_ਪ੍ਰਤੀ_c_op_ਉੱਚਾ);

// clang-format on


// Opcodes the spec reserves leave zeroed holes in the tables above: no compiler
// and no operations. Every implemented op has at least one of the two.
ਸਥਿਰ ਸਤਰਅੰਦਰ
bool IsImplementedOp (IM3OpInfo i_info)
{
    ਵਾਪਸ (i_info->compiler != c_cc_NULL or i_info->operations[0] != ਨਿਰਾ);
}

ਅਚਲ u32 c_ਅੰਕ_Operations   = M3_ਗਿਣਤੀ_ਦਾ(c_operations);
ਅਚਲ u32 c_ਅੰਕ_Operations_FC = M3_ਗਿਣਤੀ_ਦਾ(c_operationsFC);

// Maps one of the sparse opcodes above c_waOp_lastCore onto c_operationsHigh,
// or -1 for anything else in that range.
ਸਥਿਰ ਸਤਰਅੰਦਰ
i32 Get_ਉੱਚਾ_Op_ਸੂਚਕਾਂਕ (m3opcode_t opcode)
{
    ਚੋਣ (opcode) {
#ਜੇ d_m3HasRefTypes
    ਹਾਲਤ c_waOp_refNull: ਵਾਪਸ c_op_ਉੱਚਾ_ref_Null;
    ਹਾਲਤ c_waOp_refIsNull: ਵਾਪਸ c_op_ਉੱਚਾ_ref_Is_Null;
    ਹਾਲਤ c_waOp_refFunc: ਵਾਪਸ c_op_ਉੱਚਾ_ref_Func;
#  ਜੇ d_m3HasTypedRefs
    ਹਾਲਤ c_waOp_refAsNonNull: ਵਾਪਸ c_op_ਉੱਚਾ_ref_As_Non_Null;
#  endif
#ਜੇ_ਅੰਤ
#ਜੇ d_m3_Has_ਢੇਰ_Switching
    ਹਾਲਤ c_waOp_contNew: ਵਾਪਸ c_op_ਉੱਚਾ_cont_New;
    ਹਾਲਤ c_waOp_contBind: ਵਾਪਸ c_op_ਉੱਚਾ_cont_Bind;
    ਹਾਲਤ c_waOp_suspend: ਵਾਪਸ c_op_ਉੱਚਾ_suspend;
    ਹਾਲਤ c_waOp_resume: ਵਾਪਸ c_op_ਉੱਚਾ_resume;
    ਹਾਲਤ c_waOp_resumeThrow: ਵਾਪਸ c_op_ਉੱਚਾ_resume_Throw;
    ਹਾਲਤ c_waOp_resumeThrowRef: ਵਾਪਸ c_op_ਉੱਚਾ_resume_Throw_Ref;
    ਹਾਲਤ c_waOp_switch: ਵਾਪਸ c_op_ਉੱਚਾ_switch;
#ਜੇ_ਅੰਤ
    ਹਾਲਤ c_waOp_extended: ਵਾਪਸ c_op_ਉੱਚਾ_extended;
#ਜੇ d_m3HasAtomics
    ਹਾਲਤ c_waOp_atomic: ਵਾਪਸ c_op_ਉੱਚਾ_atomic;
#ਜੇ_ਅੰਤ
    ਮੂਲ: ਵਾਪਸ -1;
    }
}

IM3OpInfo GetOpInfo (m3opcode_t opcode)
{
    IM3OpInfo info = ਨਿਰਾ;

    ਚੋਣ (opcode >> 8) {
    ਹਾਲਤ 0x00:
        // c_operations is indexed by opcode only up to c_waOp_lastCore; past that
        // it holds internal operations (DEBUG builds), unreachable by opcode.
        ਜੇ (M3_LIKELY(opcode <= c_wa_Op_ਆਖਰੀ_Core)) {
            info = &c_operations[opcode];
        } ਨਹੀਂਤਾਂ {
            i32 ਉੱਚਾ = Get_ਉੱਚਾ_Op_ਸੂਚਕਾਂਕ(opcode);
            ਜੇ (ਉੱਚਾ >= 0) {
                info = &c_operations_ਉੱਚਾ[ਉੱਚਾ];
            }
        }
        ਰੋਕੋ;
    ਹਾਲਤ c_waOp_extended:
        opcode &= 0xFF;
        ਜੇ (M3_LIKELY(opcode <= c_wa_Op_ਆਖਰੀ_Extended)) {
            info = &c_operationsFC[opcode];
        }
        ਰੋਕੋ;
#ਜੇ d_m3HasAtomics
    ਹਾਲਤ c_waOp_atomic: {
        i32 index = Get_Atomic_Info_ਸੂਚਕਾਂਕ(opcode & 0xFF);
        ਜੇ (index >= 0) {
            info = &c_operationsFE[index];
        }
    } ਰੋਕੋ;
#ਜੇ_ਅੰਤ
    }

    ਵਾਪਸ (info and IsImplementedOp(info)) ? info : ਨਿਰਾ;
}

#ਜੇ d_m3HasGasMetering

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
ਗਿਣਤੀ {
    c_gasLocal   = 1,          // charged once per declared local, at function entry
    c_gasNominal = 1,          // nop, block, loop, if, const
    c_gasArith   = 45,
    c_gas_ਖਿਸਕਾਓ   = 67,
    c_gasBranch  = 90,
    c_gas_Mem_ਮਾਪ = 100,
    c_gasAccess  = 120,        // a local, global, table or memory access
    c_gasHeavy   = 10000,      // an indirect call, or anything that works over a whole region
    c_gasDivide  = 36000
};

// The most one op_UseGas can carry. A segment that reaches it is split into
// consecutive charges, which prepays exactly the same amount.
#  define d_m3MaxSegmentGas   0xFFFFFFFF

ਸਥਿਰ
u32 Get_Gas_ਲਾਗਤ (m3opcode_t i_opcode)
{
    ਚੋਣ (i_opcode) {
    ਹਾਲਤ 0x00:                        // unreachable
    ਹਾਲਤ 0x01:                        // nop
    ਹਾਲਤ c_wa_Op_ਖੰਡ:
    ਹਾਲਤ c_waOp_loop:
    ਹਾਲਤ c_waOp_if:
    ਹਾਲਤ c_wa_Op_try_ਸਾਰਣੀ:             // a block that also installs handlers
    ਹਾਲਤ c_waOp_refNull:              // a constant, by another name
    ਹਾਲਤ c_waOp_refFunc:
        ਵਾਪਸ c_gasNominal;

    ਹਾਲਤ c_wa_Op_ਸਿਰਾ:
        ਵਾਪਸ 0;                     // the one control instruction the table prices at nothing

    ਹਾਲਤ c_waOp_else:
    ਹਾਲਤ c_waOp_branch:
    ਹਾਲਤ c_waOp_branchIf:
    ਹਾਲਤ c_waOp_return:
    ਹਾਲਤ c_waOp_call:
    ਹਾਲਤ c_waOp_returnCall:
    ਹਾਲਤ c_waOp_throw:
    ਹਾਲਤ c_waOp_throwRef:
        ਵਾਪਸ c_gasBranch;

    ਹਾਲਤ c_wa_Op_branch_ਸਾਰਣੀ:
    ਹਾਲਤ 0x1a:                        // drop
    ਹਾਲਤ 0x1b:                        // select
    ਹਾਲਤ c_waOp_selectTyped:
        ਵਾਪਸ c_gasAccess;

    ਹਾਲਤ c_wa_Op_call_ਅਸਿੱਧਾ:
    ਹਾਲਤ c_wa_Op_return_Call_ਅਸਿੱਧਾ:
    ਹਾਲਤ c_waOp_callRef:
    ਹਾਲਤ c_waOp_returnCallRef:
    ਹਾਲਤ c_wa_Op_ਸਮ੍ਰਿਤੀ_Init:
    ਹਾਲਤ c_wa_Op_ਸਮ੍ਰਿਤੀ_Copy:
    ਹਾਲਤ c_wa_Op_ਸਮ੍ਰਿਤੀ_Fill:
    ਹਾਲਤ c_wa_Op_ਸਾਰਣੀ_Init:
    ਹਾਲਤ c_wa_Op_ਸਾਰਣੀ_Copy:
    ਹਾਲਤ c_wa_Op_ਸਾਰਣੀ_Grow:
    ਹਾਲਤ c_wa_Op_ਸਾਰਣੀ_Fill:
    ਹਾਲਤ 0x40:                        // memory.grow
        ਵਾਪਸ c_gasHeavy;

    ਹਾਲਤ c_wa_Op_ਅੰਕੜੇ_Drop:
    ਹਾਲਤ c_waOp_elemDrop:
    ਹਾਲਤ c_wa_Op_ਸਾਰਣੀ_ਮਾਪ:
    ਹਾਲਤ 0x3f:                        // memory.size
        ਵਾਪਸ c_gas_Mem_ਮਾਪ;

    ਹਾਲਤ c_waOp_refIsNull:
    ਹਾਲਤ c_waOp_refAsNonNull:
        ਵਾਪਸ c_gasArith;

    ਮੂਲ:
        ਰੋਕੋ;
    }

    // The two integer arithmetic runs are priced by position, i32 at 0x6a and
    // i64 at 0x7c, the table giving both the same shape
    ਜੇ ((i_opcode >= 0x6a and i_opcode <= 0x78) or (i_opcode >= 0x7c and i_opcode <= 0x8a)) {
        // clang-format off
        ਸਥਿਰ ਅਚਲ u16 c_arithmeticCosts [] = {
            c_gasArith,  c_gasArith,  c_gasArith,                   // add, sub, mul
            c_gasDivide, c_gasDivide, c_gasDivide, c_gasDivide,     // div_s, div_u, rem_s, rem_u
            c_gasArith,  c_gasArith,  c_gasArith,                   // and, or, xor
            c_gas_ਖਿਸਕਾਓ,  c_gas_ਖਿਸਕਾਓ,  c_gas_ਖਿਸਕਾਓ,                   // shl, shr_s, shr_u
            c_gasBranch, c_gasBranch                                // rotl, rotr
        };
        // clang-format on

        ਵਾਪਸ c_arithmeticCosts[i_opcode - (i_opcode <= 0x78 ? 0x6a : 0x7c)];
    }

    // the atomic accesses are memory accesses; what waits, wakes or orders is worth
    // as much as an operation over a whole region
    ਜੇ (i_opcode >= (c_waOp_atomic << 8) and i_opcode <= ((c_waOp_atomic << 8) | c_wa_Op_ਆਖਰੀ_Atomic)) {
        ਵਾਪਸ (i_opcode <= c_waOp_atomicFence) ? c_gasHeavy : c_gasAccess;
    }
    ਜੇ (i_opcode >= 0x20 and i_opcode <= 0x3e) {
        ਵਾਪਸ c_gasAccess;           // locals, globals, table accesses, loads and stores
    }
    ਜੇ (i_opcode >= 0x41 and i_opcode <= 0x44) {
        ਵਾਪਸ c_gasNominal;          // i32/i64/f32/f64.const
    }
    ਜੇ (i_opcode >= 0x8b and i_opcode <= 0xa6) {
        // the float run, priced as integer arithmetic except where it divides
        bool divides = (i_opcode == 0x91 or i_opcode == 0x95 or     // f32.sqrt, f32.div
                        i_opcode == 0x9f or i_opcode == 0xa3);      // f64.sqrt, f64.div

        ਵਾਪਸ divides ? c_gasDivide : c_gasArith;
    }

    // comparisons and clz/ctz/popcnt (0x45..0x69, 0x79..0x7b), the conversions
    // and sign extensions (0xa7..0xc4), the saturating truncations, the wide
    // arithmetic, and anything else that reaches here: plain arithmetic
    ਵਾਪਸ c_gasArith;
}

// The instructions ewasm's design segments a body at. A segment runs up to and
// including the next one of these, so every cycle a body can execute - a loop's
// back edge, a branch out of a block, falling off the end of a function - passes
// through at least one, and no path can run unmetered however it is entered.
ਸਥਿਰ
bool Is_Gas_Segment_ਸਿਰਾ (m3opcode_t i_opcode)
{
    ਚੋਣ (i_opcode) {
    ਹਾਲਤ c_waOp_loop:
    ਹਾਲਤ c_waOp_if:
    ਹਾਲਤ c_waOp_else:
    ਹਾਲਤ c_wa_Op_ਸਿਰਾ:
    ਹਾਲਤ c_waOp_branch:
    ਹਾਲਤ c_waOp_branchIf:
    ਹਾਲਤ c_wa_Op_branch_ਸਾਰਣੀ:
    ਹਾਲਤ c_waOp_return:
    ਹਾਲਤ c_waOp_returnCall:
    ਹਾਲਤ c_wa_Op_return_Call_ਅਸਿੱਧਾ:
    ਹਾਲਤ c_waOp_returnCallRef:
        ਵਾਪਸ true;
    ਮੂਲ:
        ਵਾਪਸ false;
    }
}

// Ends the open segment by writing what it accumulated into the immediate its
// op_UseGas is still waiting on. Emitting onto a later code page does not move
// an earlier one, so the reservation is still where it was left.
ਸਥਿਰ
ਖਾਲੀ CloseGasSegment (IM3Compilation o)
{
    ਜੇ (o->isGasSegmentOpen) {
        ਜੇ (o->gasPatch) {
            u32 ਲਾਗਤ = o->gas_ਲਾਗਤ;
            ਸਮ੍ਰਿਤੀ_ਨਕਲ(o->gasPatch, &ਲਾਗਤ, ਆਕਾਰ(ਲਾਗਤ));

            o->gasPatch = ਨਿਰਾ;
        }
        o->gas_ਲਾਗਤ          = 0;
        o->isGasSegmentOpen = false;
    }
}

// Whether this compilation emits the charges
ਸਥਿਰ ਸਤਰਅੰਦਰ
bool IsMetering (IM3Compilation o)
{
    ਵਾਪਸ o->runtime->gas_ਹੱਦਬੰਦੀ != 0;
}

// Charges one instruction to the segment being compiled, opening a segment at
// this point in the code stream if none is open, and settling the segment when
// the instruction is one that ends it. Called before the instruction is
// compiled, which is what both halves need: the op_UseGas lands ahead of what it
// pays for, and the cost is final before a block-structured instruction - loop,
// if, else - compiles its body from inside its own compiler. The body then opens
// a segment of its own, inside the loop or the arm, where it is charged on every
// pass rather than once on the way in.
ਸਥਿਰ
M3_ਨਤੀਜਾ MeterOpcode (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_ਨਤੀਜਾ ਨਤੀਜਾ = m3Err_none;

    // constant expressions are not metered: they run once at instantiation,
    // before the module is anything a gas budget was handed out for
    ਜੇ (o->function and o->ਪੰਨਾ and IsMetering(o)) {
        // only the 0xFC or 0xFE prefix has been read so far; the instruction it names is
        // the LEB128 u32 the compiler is about to take. Peek it without consuming.
        // Anything the tables do not cover leaves the prefix priced on its own,
        // which is moot - the compile fails on it a moment later.
        ਜੇ (i_opcode == c_waOp_extended or i_opcode == c_waOp_atomic) {
            ਬਾਈਟਾਂ_t peek = o->wasm;
            u32     sub;
            u32     ਆਖਰੀ = (i_opcode == c_waOp_atomic) ? c_wa_Op_ਆਖਰੀ_Atomic : c_wa_Op_ਆਖਰੀ_Extended;

            ਜੇ (not ReadLEB_u32(&sub, &peek, o->wasm_ਸਿਰਾ) and sub <= ਆਖਰੀ) {
                i_opcode = (m3opcode_t)((i_opcode << 8) | sub);
            }
        }

        u32 ਲਾਗਤ = Get_Gas_ਲਾਗਤ(i_opcode);

        ਜੇ (o->isGasSegmentOpen and o->gas_ਲਾਗਤ > d_m3MaxSegmentGas - ਲਾਗਤ) {
            CloseGasSegment(o);
        }

        ਜੇ (not o->isGasSegmentOpen) {
            o->isGasSegmentOpen = true;

_           (EmitOp(o, op_UseGas));
            o->gasPatch = (ਖਾਲੀ*)Get_ਪੰਨਾ_PC(o->ਪੰਨਾ);
            EmitConstant32(o, 0);
        }

        o->gas_ਲਾਗਤ += ਲਾਗਤ;

        ਜੇ (Is_Gas_Segment_ਸਿਰਾ(i_opcode)) {
            CloseGasSegment(o);
        }
    }

    _catch: ਵਾਪਸ ਨਤੀਜਾ;
}

#ਜੇ_ਅੰਤ // d_m3HasGasMetering


M3_ਨਤੀਜਾ Compile_ਖੰਡ_Statements (IM3Compilation o)
{
    M3_ਨਤੀਜਾ ਨਤੀਜਾ   = m3Err_none;
    bool     valid_ਸਿਰਾ = false;

    ਜਦੋਂ (o->wasm < o->wasm_ਸਿਰਾ) {
#ਜੇ d_m3EnableOpTracing
        ਜੇ (o->ਅੰਕ_Emits) {
            EmitOp(o, op_Dump_ਢੇਰ);
            EmitConstant32(o, o->ਅੰਕ_Opcodes);
            EmitConstant32(o, Get_Max_ਵਰਤਿਆ_ਥਾਂ_Plus_One(o));
            Emit_ਸੂਚਕ(o, o->function);

            o->ਅੰਕ_Emits = 0;
        }
#ਜੇ_ਅੰਤ

        m3opcode_t opcode;
        o->ਆਖਰੀ_Opcode_ਸ਼ੁਰੂ = o->wasm;
_       (Read_opcode(&opcode, &o->wasm, o->wasm_ਸਿਰਾ));
        log_opcode(o, opcode);

        // Restrict opcodes when evaluating expressions
        ਜੇ (M3_UNLIKELY(not o->function)) {
            // clang-format off
            ਚੋਣ (opcode) {
            ਹਾਲਤ c_waOp_i32_const: ਹਾਲਤ c_waOp_i64_const:
            ਹਾਲਤ c_waOp_f32_const: ਹਾਲਤ c_waOp_f64_const:
            ਹਾਲਤ c_waOp_getGlobal: ਹਾਲਤ c_wa_Op_ਸਿਰਾ:
#ਜੇ d_m3HasRefTypes
            ਹਾਲਤ c_waOp_refNull:   ਹਾਲਤ c_waOp_refFunc:
#ਜੇ_ਅੰਤ
#ਜੇ d_m3HasExtendedConst
            ਹਾਲਤ c_wa_Op_i32_ਜੋੜੋ:   ਹਾਲਤ c_waOp_i32_sub:   ਹਾਲਤ c_waOp_i32_mul:
            ਹਾਲਤ c_wa_Op_i64_ਜੋੜੋ:   ਹਾਲਤ c_waOp_i64_sub:   ਹਾਲਤ c_waOp_i64_mul:
#ਜੇ_ਅੰਤ
                ਰੋਕੋ;
            ਮੂਲ:
                _throw(m3Err_restrictedOpcode);
            }
            // clang-format on
        }

        IM3OpInfo opinfo = GetOpInfo(opcode);

        ਜੇ (M3_UNLIKELY(opinfo == ਨਿਰਾ)) {
            _throw(ErrorCompile(m3Err_unknownOpcode, o, "opcode '%x' not available", opcode));
        }

#ਜੇ d_m3HasGasMetering
_       (MeterOpcode(o, opcode));
#ਜੇ_ਅੰਤ

        ਜੇ (opinfo->compiler != c_cc_NULL) {
_           ((*GetCompiler(opinfo))(o, opcode))
        } ਨਹੀਂਤਾਂ {
_           (Compile_Operator(o, opcode));
        }

        o->ਪਿਛਲਾ_Opcode = opcode;

        ਜੇ (opcode == c_waOp_else) {
            _throwif(m3Err_wasmMalformed, o->ਖੰਡ.opcode != c_waOp_if);
            valid_ਸਿਰਾ = true;
            ਰੋਕੋ;
        } ਨਹੀਂਤਾਂ ਜੇ (opcode == c_wa_Op_ਸਿਰਾ) {
            valid_ਸਿਰਾ = true;
            ਰੋਕੋ;
        }
    }
    _throwif(m3Err_wasmMalformed, not valid_ਸਿਰਾ);

_catch:
    ਵਾਪਸ ਨਤੀਜਾ;
}

ਸਥਿਰ
M3_ਨਤੀਜਾ ਧੱਕੋ_ਖੰਡ_Results (IM3Compilation o)
{
    M3_ਨਤੀਜਾ ਨਤੀਜਾ = m3Err_none;

    u16 ਅੰਕ_Results = Get_Func_Type_ਅੰਕ_Results(o->ਖੰਡ.type);

    ਲਈ (u16 i = 0; i < ਅੰਕ_Results; ++i) {
        m3type_t type = Get_Func_Type_ਨਤੀਜਾ_Type(o->ਖੰਡ.type, i);

        ਜੇ (i == ਅੰਕ_Results - 1 and IsFpType(type)) {
_           (ਧੱਕੋ_ਰਜਿਸਟਰ(o, type));
        } ਨਹੀਂਤਾਂ {
_           (ਧੱਕੋ_Allocated_ਥਾਂ(o, type));
        }
    }

    _catch: ਵਾਪਸ ਨਤੀਜਾ;
}


M3_ਨਤੀਜਾ Compile_ਖੰਡ (IM3Compilation o, IM3FuncType i_ਖੰਡ_Type, m3opcode_t i_ਖੰਡ_Opcode)
{
                                                                                        d_m3Assert (not Is_ਰਜਿਸਟਰ_Allocated (o, 0));
                                                                                        d_m3Assert (not Is_ਰਜਿਸਟਰ_Allocated (o, 1));
    InvalidateFold(o);

    M3CompilationScope  outerScope = o->ਖੰਡ;
    M3CompilationScope* ਖੰਡ      = &o->ਖੰਡ;

    ਖੰਡ->outer   = &outerScope;
    ਖੰਡ->pc      = Get_ਪੰਨਾ_PC(o->ਪੰਨਾ);
    ਖੰਡ->patches = ਨਿਰਾ;
    ਖੰਡ->type    = i_ਖੰਡ_Type;
    ਖੰਡ->opcode  = i_ਖੰਡ_Opcode;
    ਖੰਡ->ਡੂੰਘਾਈ++;
    ਖੰਡ->snapshot_ਖੰਡ = 0;

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
#ਜੇ d_m3HasSnapshots
    // op_Loop's frame records the body, which is also the loop's identity;
    // op_TryTable's records its clause table, two words a clause back from it
    ਜੇ (i_ਖੰਡ_Opcode == c_waOp_loop) {
_       (ਰਿਕਾਰਡ_ਖੰਡ_ਸ਼ੁਰੂ(o, i_ਖੰਡ_Opcode, ਖੰਡ->pc, 0));
    }
#  ਜੇ d_m3HasExceptionHandling
    ਨਹੀਂਤਾਂ ਜੇ (i_ਖੰਡ_Opcode == c_wa_Op_try_ਸਾਰਣੀ) {
_       (ਰਿਕਾਰਡ_ਖੰਡ_ਸ਼ੁਰੂ(o, i_ਖੰਡ_Opcode, ਖੰਡ->pc - 2 * o->ਅੰਕ_Try_Clauses, o->ਅੰਕ_Try_Clauses));
    }
#  endif
#ਜੇ_ਅੰਤ

    // validate and dealloc params ----------------------------

    u16 ਢੇਰ_ਸੂਚਕਾਂਕ = o->ਢੇਰ_ਸੂਚਕਾਂਕ;

    u16 ਅੰਕ_Params = Get_Func_Type_ਅੰਕ_Params(i_ਖੰਡ_Type);

    ਜੇ (i_ਖੰਡ_Opcode != c_waOp_else) {
        ਲਈ (u16 i = 0; i < ਅੰਕ_Params; ++i) {
            m3type_t type = GetFuncTypeParamType(i_ਖੰਡ_Type, ਅੰਕ_Params - 1 - i);
_           (ਕੱਢੋ_Type(o, type));
        }
    } ਨਹੀਂਤਾਂ {
        ਜੇ (Is_ਢੇਰ_Polymorphic(o) && o->ਖੰਡ.ਖੰਡ_ਢੇਰ_ਸੂਚਕਾਂਕ + ਅੰਕ_Params > o->ਢੇਰ_ਸੂਚਕਾਂਕ) {
            o->ਢੇਰ_ਸੂਚਕਾਂਕ = o->ਖੰਡ.ਖੰਡ_ਢੇਰ_ਸੂਚਕਾਂਕ;
        } ਨਹੀਂਤਾਂ {
            o->ਢੇਰ_ਸੂਚਕਾਂਕ -= ਅੰਕ_Params;
        }
    }

    u16 param_ਸੂਚਕਾਂਕ        = o->ਢੇਰ_ਸੂਚਕਾਂਕ;
    ਖੰਡ->exit_ਢੇਰ_ਸੂਚਕਾਂਕ = param_ਸੂਚਕਾਂਕ; // consume the params at block exit

    // keep copies of param slots in the stack
    o->ਢੇਰ_ਸੂਚਕਾਂਕ = ਢੇਰ_ਸੂਚਕਾਂਕ;

    // find slots for the results ----------------------------
_   (ਧੱਕੋ_ਖੰਡ_Results(o));

    ਢੇਰ_ਸੂਚਕਾਂਕ = o->ਢੇਰ_ਸੂਚਕਾਂਕ;

    // dealloc but keep record of the result slots in the stack
    u16 ਅੰਕ_Results = Get_Func_Type_ਅੰਕ_Results(i_ਖੰਡ_Type);
    ਜਦੋਂ (ਅੰਕ_Results--) {
        ਕੱਢੋ(o);
    }

    ਖੰਡ->ਖੰਡ_ਢੇਰ_ਸੂਚਕਾਂਕ = o->ਢੇਰ_ਸੂਚਕਾਂਕ = ਢੇਰ_ਸੂਚਕਾਂਕ;

    // push the params back onto the stack -------------------
    ਲਈ (u16 i = 0; i < ਅੰਕ_Params; ++i) {
        m3type_t type = GetFuncTypeParamType(i_ਖੰਡ_Type, i);

        u16 ਥਾਂ = Get_ਥਾਂ_For_ਢੇਰ_ਸੂਚਕਾਂਕ(o, param_ਸੂਚਕਾਂਕ + i);
        ਧੱਕੋ(o, type, ਥਾਂ);

        ਜੇ (ਥਾਂ >= o->ਥਾਂ_ਪਹਿਲਾ_Dynamic_ਸੂਚਕਾਂਕ && ਥਾਂ != c_ਥਾਂ_ਅਣਵਰਤਿਆ) {
_           (MarkSlotsAllocatedByType(o, ਥਾਂ, type));
        }
    }

    //--------------------------------------------------------

#ਜੇ d_m3HasExceptionHandling
    // with the scope pushed and the params back on, a catch label of 0 names
    // this block - which is what the proposal says it should
    ਜੇ (i_ਖੰਡ_Opcode == c_wa_Op_try_ਸਾਰਣੀ) {
_       (EmitCatchStubs(o));
    }
#ਜੇ_ਅੰਤ

_   (Compile_ਖੰਡ_Statements(o));

_   (Validate_ਖੰਡ_ਸਿਰਾ(o));

    ਜੇ (o->function)    // skip for expressions
    {
        ਜੇ (not Is_ਢੇਰ_Polymorphic(o)) {
_           (Resolve_ਖੰਡ_Results(o, &o->ਖੰਡ, /* isBranch: */ false));
        }

#ਜੇ d_m3HasExceptionHandling
        // Falling out of the end of a try region retires its handler. This sits
        // ahead of PatchBranches so branches into the block's exit land past it:
        // a branch out of the try popped its own handler at the branch site.
        ਜੇ (i_ਖੰਡ_Opcode == c_wa_Op_try_ਸਾਰਣੀ) {
_           (EmitOp(o, op_ਕੱਢੋ_Handlers));
            EmitConstant32(o, 1);
        }
#ਜੇ_ਅੰਤ

_       (Unwind_ਖੰਡ_ਢੇਰ(o))

        ਜੇ (not ((i_ਖੰਡ_Opcode == c_waOp_if and ਅੰਕ_Results) or o->ਪਿਛਲਾ_Opcode == c_waOp_else)) {
            o->ਢੇਰ_ਸੂਚਕਾਂਕ = o->ਖੰਡ.exit_ਢੇਰ_ਸੂਚਕਾਂਕ;
_           (ਧੱਕੋ_ਖੰਡ_Results(o));
        }
    }

    PatchBranches(o);

#ਜੇ d_m3HasSnapshots
    ਰਿਕਾਰਡ_ਖੰਡ_ਸਿਰਾ(o);

    ਜੇ (o->local_Init_ਡੂੰਘਾਈ) {
        ਲਈ (u32 i = 0; i < o->snapshotMap->ਅੰਕ_Locals; ++i) {
            ਜੇ (o->local_Init_ਡੂੰਘਾਈ[i] >= (u32)o->ਖੰਡ.ਡੂੰਘਾਈ) {
                o->local_Init_ਡੂੰਘਾਈ[i] = UINT32_MAX;
            }
        }
    }
#ਜੇ_ਅੰਤ
    o->ਖੰਡ = outerScope;

} _catch: ਵਾਪਸ ਨਤੀਜਾ;
}

ਸਥਿਰ
M3_ਨਤੀਜਾ CompileLocals (IM3Compilation o)
{
    M3_ਨਤੀਜਾ ਨਤੀਜਾ;

    u32 ਅੰਕ_Locals = 0;
    u32 ਅੰਕ_Local_ਖੰਡਾਂ;
_   (ReadLEB_u32(&ਅੰਕ_Local_ਖੰਡਾਂ, &o->wasm, o->wasm_ਸਿਰਾ));

    ਲਈ (u32 l = 0; l < ਅੰਕ_Local_ਖੰਡਾਂ; ++l) {
        u32      var_ਗਿਣਤੀ;
        m3type_t localType;

_       (ReadLEB_u32(&var_ਗਿਣਤੀ, &o->wasm, o->wasm_ਸਿਰਾ));
_       (Parse_ਮੁੱਲ_Type(o->module, &localType, &o->wasm, o->wasm_ਸਿਰਾ));
        ਅੰਕ_Locals += var_ਗਿਣਤੀ;                                                          m3log (compile, "pushing locals. count: %d; type: %s", var_ਗਿਣਤੀ, c_waTypes [ਅਧਾਰ_Type_ਦਾ(localType)]);
        ਜਦੋਂ (var_ਗਿਣਤੀ--) {
_           (ਧੱਕੋ_Allocated_ਥਾਂ(o, localType));
        }
    }

    ਜੇ (o->function) {
        o->function->ਅੰਕ_Locals = ਅੰਕ_Locals;
    }

    _catch: ਵਾਪਸ ਨਤੀਜਾ;
}

ਸਥਿਰ
M3_ਨਤੀਜਾ ReserveConstants (IM3Compilation o)
{
    M3_ਨਤੀਜਾ ਨਤੀਜਾ = m3Err_none;

    // in the interest of speed, this blindly scans the Wasm code looking for any byte
    // that looks like an const opcode.
    u16 ਅੰਕ_Constant_Slots = 0;

    ਬਾਈਟਾਂ_t wa = o->wasm;
    ਜਦੋਂ (wa < o->wasm_ਸਿਰਾ) {
        u8  ਕੋਡ     = *wa++;
        u16 ਜੋੜੋ_Slots = 0;

        ਜੇ (ਕੋਡ == c_waOp_i32_const or ਕੋਡ == c_waOp_f32_const) {
            ਜੋੜੋ_Slots = 1;
        } ਨਹੀਂਤਾਂ ਜੇ (ਕੋਡ == c_waOp_i64_const or ਕੋਡ == c_waOp_f64_const) {
            ਜੋੜੋ_Slots = Get_Type_ਅੰਕ_Slots(c_m3Type_i64);
        }

        ਜੇ (ਅੰਕ_Constant_Slots + ਜੋੜੋ_Slots >= d_m3_Max_Constant_ਸਾਰਣੀ_ਮਾਪ) {
            ਰੋਕੋ;
        }

        ਅੰਕ_Constant_Slots += ਜੋੜੋ_Slots;
    }

    // if constants overflow their reserved stack space, the compiler simply emits op_Const
    // operations as needed. Compiled expressions (global inits) don't pass through this
    // ReserveConstants function and thus always produce inline constants.

    Align_ਥਾਂ_ਮੰਜ਼ਿਲ_Type(&ਅੰਕ_Constant_Slots, c_m3Type_i64);                                           m3log (compile, "reserved constant slots: %d", ਅੰਕ_Constant_Slots);

    o->ਥਾਂ_ਪਹਿਲਾ_Dynamic_ਸੂਚਕਾਂਕ = o->ਥਾਂ_ਪਹਿਲਾ_Const_ਸੂਚਕਾਂਕ + ਅੰਕ_Constant_Slots;

    ਜੇ (o->ਥਾਂ_ਪਹਿਲਾ_Dynamic_ਸੂਚਕਾਂਕ >= d_m3MaxFunctionSlots) {
        _throw(m3_Err_function_ਢੇਰ_ਉਛਾਲ);
    }

    _catch:
    ਵਾਪਸ ਨਤੀਜਾ;
}


// A constant expression compiles like a miniature function root block: no args,
// no locals and a single result, which Compile_End copies down into slot 0 for
// EvaluateExpression to read back.
M3_ਨਤੀਜਾ Compile_ਵਿਅੰਜਕ (IM3Compilation o, IM3FuncType i_ਨਤੀਜਾ_Type)
{
    M3_ਨਤੀਜਾ ਨਤੀਜਾ = m3Err_none;

    o->ਖੰਡ.type = i_ਨਤੀਜਾ_Type;

    u16 ਅੰਕ_Ret_Slots = Get_Func_Type_ਅੰਕ_Results(i_ਨਤੀਜਾ_Type) * c_io_ਥਾਂ_ਗਿਣਤੀ;

    ਲਈ (u16 i = 0; i < ਅੰਕ_Ret_Slots; ++i) {
        Mark_ਥਾਂ_Allocated(o, i);
    }

    o->max_ਢੇਰ_Slots = o->ਥਾਂ_Max_Allocated_ਸੂਚਕਾਂਕ_Plus_One = o->ਥਾਂ_ਪਹਿਲਾ_Dynamic_ਸੂਚਕਾਂਕ = ਅੰਕ_Ret_Slots;

_   (Compile_ਖੰਡ_Statements(o));

    _catch: ਵਾਪਸ ਨਤੀਜਾ;
}


M3_ਨਤੀਜਾ CompileFunction (IM3Function io_function)
{
    ਜੇ (!io_function->wasm) {
        // An import. Either linking pointed it at another module's function -
        // in which case that one carries the body - or a host function was
        // bound to it, or it is simply unsatisfied.
        IM3Function impl = Function_Implementation(io_function);

        ਜੇ (impl != io_function) {
            ਜੇ (not impl->compiled) {
                M3_ਨਤੀਜਾ r = CompileFunction(impl);
                ਜੇ (r) {
                    ਵਾਪਸ r;
                }
            }

            // keep the placeholder callable too, for anything still holding it
            io_function->compiled = impl->compiled;
            ਵਾਪਸ m3Err_none;
        }

        ਜੇ (io_function->compiled) {
            ਵਾਪਸ m3Err_none;
        }

        ਵਾਪਸ ErrorModule(m3Err_functionImportMissing, io_function->module, "'%s.%s'",
                           GetFunctionImportModuleName(io_function),
                           m3_GetFunctionName(io_function));
    }

#ਜੇ d_m3EnableValidation
    ਜੇ (not io_function->module->runtime->skipValidation) {
        M3_ਨਤੀਜਾ vr = ValidateFunction(io_function);
        ਜੇ (vr) {
            ਵਾਪਸ vr;
        }
    }
#ਜੇ_ਅੰਤ

    IM3FuncType funcType = io_function->funcType;                   m3log (compile, "compiling: [%d] %s %s; wasm-size: %d",
                                                                        io_function->index, m3_GetFunctionName (io_function), SPrintFuncTypeSignature (funcType), (u32) (io_function->wasm_ਸਿਰਾ - io_function->wasm));
    IM3Runtime runtime = io_function->module->runtime;

    IM3Compilation o = &runtime->compilation;                       d_m3Assert (d_m3MaxFunctionSlots >= d_m3_Max_Function_ਢੇਰ_ਉਚਾਈ * (d_m3_Use32_ਦੋਆਧਾਰੀ_ਅੰਕ_Slots + 1))  // need twice as many slots in 32-bit mode
    memset(o, 0x0, ਆਕਾਰ(M3Compilation));

    o->runtime    = runtime;
    o->module     = io_function->module;
    o->function   = io_function;
    o->wasm       = io_function->wasm;
    o->wasm_ਸਿਰਾ    = io_function->wasm_ਸਿਰਾ;
    o->ਖੰਡ.type = funcType;

_try {
    // skip over code size. the end was already calculated during parse phase
    u32 ਮਾਪ;
_   (ReadLEB_u32(&ਮਾਪ, &o->wasm, o->wasm_ਸਿਰਾ));                     d_m3Assert (ਮਾਪ == (o->wasm_ਸਿਰਾ - o->wasm))

#ਜੇ d_m3HasSnapshots
    o->body_ਸ਼ੁਰੂ = o->wasm;
#ਜੇ_ਅੰਤ

#ਜੇ d_m3HasSnapshots
    // only code that can be suspended can end up in a snapshot
    ਜੇ (runtime->isSuspendable) {
        o->snapshotMap = m3_AllocStruct(M3SnapshotMap);
        _throwifnull(o->snapshotMap);
    }
#ਜੇ_ਅੰਤ

_   (Acquire_Compilation_ਕੋਡ_ਪੰਨਾ(o, &o->ਪੰਨਾ));

    pc_t pc = Get_ਪੰਨਾ_PC(o->ਪੰਨਾ);

    u16 ਅੰਕ_Ret_Slots = Get_Function_ਅੰਕ_Returns(o->function) * c_io_ਥਾਂ_ਗਿਣਤੀ;

    ਲਈ (u16 i = 0; i < ਅੰਕ_Ret_Slots; ++i) {
        Mark_ਥਾਂ_Allocated(o, i);
    }

    o->function->ਅੰਕ_Ret_Slots = o->ਥਾਂ_ਪਹਿਲਾ_Dynamic_ਸੂਚਕਾਂਕ = ਅੰਕ_Ret_Slots;

    u16 ਅੰਕ_ਦਲੀਲਾਂ = Get_Function_ਅੰਕ_ਦਲੀਲਾਂ(o->function);

    // push the arg types to the type stack
    ਲਈ (u16 i = 0; i < ਅੰਕ_ਦਲੀਲਾਂ; ++i) {
        m3type_t type = GetFunctionArgType(o->function, i);
_       (ਧੱਕੋ_Allocated_ਥਾਂ(o, type));

        // prevent allocator fill-in
        o->ਥਾਂ_ਪਹਿਲਾ_Dynamic_ਸੂਚਕਾਂਕ += c_io_ਥਾਂ_ਗਿਣਤੀ;
    }

    o->ਥਾਂ_Max_Allocated_ਸੂਚਕਾਂਕ_Plus_One = o->function->ਅੰਕ_Ret_And_Arg_Slots = o->ਥਾਂ_ਪਹਿਲਾ_Local_ਸੂਚਕਾਂਕ = o->ਥਾਂ_ਪਹਿਲਾ_Dynamic_ਸੂਚਕਾਂਕ;

_   (CompileLocals(o));

#ਜੇ d_m3HasSnapshots
_   (ਰਿਕਾਰਡ_Locals(o));
#ਜੇ_ਅੰਤ

#ਜੇ d_m3HasGasMetering
    // the table charges for a function's locals as well as for its instructions.
    // op_Entry is what sets them up, so the charge rides on the body's first
    // segment, which is emitted right after it
    o->gas_ਲਾਗਤ = o->function->ਅੰਕ_Locals * c_gasLocal;
#ਜੇ_ਅੰਤ

    u16 max_ਥਾਂ = Get_Max_ਵਰਤਿਆ_ਥਾਂ_Plus_One(o);

    o->function->ਅੰਕ_Local_ਬਾਈਟਾਂ = (max_ਥਾਂ - o->ਥਾਂ_ਪਹਿਲਾ_Local_ਸੂਚਕਾਂਕ) * ਆਕਾਰ(m3slot_t);

    o->ਥਾਂ_ਪਹਿਲਾ_Const_ਸੂਚਕਾਂਕ = o->ਥਾਂ_Max_Const_ਸੂਚਕਾਂਕ = max_ਥਾਂ;

    // ReserveConstants initializes o->firstDynamicSlotNumber
_   (ReserveConstants(o));

    // start tracking the max stack used (Push() also updates this value) so that op_Entry can precisely detect stack overflow
    o->max_ਢੇਰ_Slots = o->ਥਾਂ_Max_Allocated_ਸੂਚਕਾਂਕ_Plus_One = o->ਥਾਂ_ਪਹਿਲਾ_Dynamic_ਸੂਚਕਾਂਕ;

    o->ਖੰਡ.ਖੰਡ_ਢੇਰ_ਸੂਚਕਾਂਕ = o->ਢੇਰ_ਪਹਿਲਾ_Dynamic_ਸੂਚਕਾਂਕ = o->ਢੇਰ_ਸੂਚਕਾਂਕ;                           m3log (compile, "start stack index: %u; max stack slots: %u",
                                                                                                           (u32) o->ਢੇਰ_ਪਹਿਲਾ_Dynamic_ਸੂਚਕਾਂਕ, (u32) o->max_ਢੇਰ_Slots);
#ਜੇ d_m3_Has_ਢੇਰ_Switching
    // A suspendable body opens by checking for a pause request, once its frame
    // is set up. The pause point's offset is the first instruction's, just past
    // the local declarations - the body's end, when it has no other.
    ਜੇ (runtime->isSuspendable) {
        o->ਆਖਰੀ_Opcode_ਸ਼ੁਰੂ = o->wasm;
#  ਜੇ d_m3_ਇੰਦਰਾਜ_Keeps_ਚੌਖਟਾ
_       (EmitOp(o, op_ਇੰਦਰਾਜ));
        Emit_ਸੂਚਕ(o, io_function);
_       (EmitOp(o, op_ਇੰਦਰਾਜ_Check));
_       (ਰਿਕਾਰਡ_ਸੁਰੱਖਿਅਤ_ਬਿੰਦੂ(o, safepoint_ਇੰਦਰਾਜ, 1, 0, 0, ਨਿਰਾ));
#  ਨਹੀਂਤਾਂ
_       (EmitOp(o, op_ਇੰਦਰਾਜ_Suspendable));
        Emit_ਸੂਚਕ(o, io_function);
_       (ਰਿਕਾਰਡ_ਸੁਰੱਖਿਅਤ_ਬਿੰਦੂ(o, safepoint_ਇੰਦਰਾਜ, 0, 0, 0, ਨਿਰਾ));
#  endif
    } ਨਹੀਂਤਾਂ
#ਜੇ_ਅੰਤ
    {
_       (EmitOp(o, op_ਇੰਦਰਾਜ));
        Emit_ਸੂਚਕ(o, io_function);
    }

_   (Compile_ਖੰਡ_Statements(o));

    _throwif(m3Err_wasmMalformed, o->ਪਿਛਲਾ_Opcode != c_wa_Op_ਸਿਰਾ);

    io_function->compiled      = pc;
    io_function->max_ਢੇਰ_Slots = o->max_ਢੇਰ_Slots;

    u16 ਅੰਕ_Constant_Slots = o->ਥਾਂ_Max_Const_ਸੂਚਕਾਂਕ - o->ਥਾਂ_ਪਹਿਲਾ_Const_ਸੂਚਕਾਂਕ;                           m3log (compile, "unique constant slots: %u; unused slots: %u",
                                                                                                           ਅੰਕ_Constant_Slots, o->ਥਾਂ_ਪਹਿਲਾ_Dynamic_ਸੂਚਕਾਂਕ - o->ਥਾਂ_Max_Const_ਸੂਚਕਾਂਕ);
    io_function->ਅੰਕ_Constant_ਬਾਈਟਾਂ = ਅੰਕ_Constant_Slots * ਆਕਾਰ(m3slot_t);

    ਜੇ (ਅੰਕ_Constant_Slots) {
        io_function->constants = m3_CopyMem((cbytes_t)o->constants, io_function->ਅੰਕ_Constant_ਬਾਈਟਾਂ);
        _throwifnull(io_function->constants);
    }

#ਜੇ d_m3HasSnapshots
    io_function->snapshotMap = o->snapshotMap;
    o->snapshotMap           = ਨਿਰਾ;
#ਜੇ_ਅੰਤ

} _catch:

    Release_Compilation_ਕੋਡ_ਪੰਨਾ(o);

#ਜੇ d_m3HasSnapshots
    SnapshotMap_Free(o->snapshotMap);
    o->snapshotMap = ਨਿਰਾ;
    m3_Free(o->local_Init_ਡੂੰਘਾਈ);
#ਜੇ_ਅੰਤ

    ਵਾਪਸ ਨਤੀਜਾ;
}

#ਜੇ d_m3_Has_ਢੇਰ_Switching
m3ret_t ResumeContinuation (IM3Runtime i_runtime, IM3Continuation i_cont)
{
    ਜੇ (M3_UNLIKELY(!i_cont || i_cont->state != cont_suspended)) {
        ਵਾਪਸ m3Err_none;
    }

    M3_ਸਮ੍ਰਿਤੀ_ਸਿਰਲੇਖ* _mem = i_cont->ਇੰਦਰਾਜ_Function ? Module_ਸਮ੍ਰਿਤੀ_ਸਿਰਲੇਖ(i_cont->ਇੰਦਰਾਜ_Function->module) : ਨਿਰਾ;

    i_runtime->ਸਰਗਰਮ_Continuation = i_cont;
    i_cont->state                 = cont_running;

    i32 outermost     = (i32)i_cont->ਅੰਕ_ਚੌਖਟੇ - 1;
    i_cont->ਅੰਕ_ਚੌਖਟੇ = 0;

    // with no frames standing, this lands straight on the suspension point
    m3ret_t r = Replay_ਚੌਖਟੇ(i_cont, outermost, _mem);

    ਜੇ (r == m3Err_continuationSuspended) {
        i_cont->state = cont_suspended;
    } ਨਹੀਂਤਾਂ {
        i_cont->state = cont_returned;
    }
    i_runtime->ਸਰਗਰਮ_Continuation = ਨਿਰਾ;

    ਵਾਪਸ r;
}
#ਜੇ_ਅੰਤ
