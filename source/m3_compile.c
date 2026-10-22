//
//  m3_compile.c
//
//  Created by Steven Massey on 4/17/19.
//  Copyright © 2019 Steven Massey. All rights reserved.
//

// Allow using opcodes for compilation process
#define M3_COMPILE_OPCODES

#അന്തര്ഭാവ "m3_env.h"
#അന്തര്ഭാവ "m3_compile.h"
#അന്തര്ഭാവ "m3_exec.h"
#അന്തര്ഭാവ "m3_exception.h"
#അന്തര്ഭാവ "m3_info.h"
#അന്തര്ഭാവ "m3_validate.h"

// Resolves M3OpInfo.compiler, an index, to the function it names. Defined down
// with c_compilers [], which has to follow every compiler it points at.
സ്ഥിതം
M3Compiler GetCompiler (IM3OpInfo i_opInfo);

//----- EMIT --------------------------------------------------------------------------------------------------------------

സ്ഥിതം വരിയിൽ
pc_t GetPC (IM3Compilation o)
{
    തിരികെ GetPagePC(o->page);
}

#എങ്കിൽ d_m3HasSnapshots

#  define d_m3GrowSnapshotArray(TYPE, ARRAY, എണ്ണം_2, CAPACITY)                    \
    എങ്കിൽ ((എണ്ണം_2) == (CAPACITY)) {                                                 \
        u32   newCapacity = (CAPACITY) ? (CAPACITY) * 2 : 8;                     \
        TYPE* grown = m3_ReallocArray(TYPE, (ARRAY), newCapacity, (CAPACITY));   \
        എങ്കിൽ (not grown) {                                                         \
            തിരികെ m3Err_mallocFailed;                                           \
        }                                                                        \
        (ARRAY)    = grown;                                                      \
        (CAPACITY) = newCapacity;                                                \
    }

// Where the instruction being compiled starts, counted from the start of the
// function's body: the one position every build of every revision agrees on
സ്ഥിതം വരിയിൽ
u32 GetWasmOffset (IM3Compilation o)
{
    തിരികെ (u32)(o->lastOpcodeStart - o->bodyStart);
}

// Whether the operand stack entry at i_index is a value the frame holds. Every
// block keeps records of its parameter and result slots on the stack, between
// the height it was entered at and where its own values begin (see
// CompileBlock). Those are landing pads rather than values: nothing need have
// written them yet, and the slots they name may be in use for something else.
// The function's own scope has none - that stretch is its arguments and locals.
സ്ഥിതം
bool Is_Live_അട്ടി_Entry (IM3Compilation o, u16 i_index)
{
    വേണ്ടി (IM3CompilationScope scope = &o->block; scope->outer; scope = scope->outer) {
        എങ്കിൽ (i_index >= scope->exit_അട്ടി_Index and i_index < scope->block_അട്ടി_Index) {
            തിരികെ false;
        }
    }

    തിരികെ true;
}

സ്ഥിതം
M3Result Add_Safe_Point_മൂല്യം (IM3Compilation o, M3SafePoint* io_point, u16 i_index)
{
    M3SnapshotMap* map = o->snapshotMap;

    d_m3GrowSnapshotArray(M3_Slot_മൂല്യം, map->മൂല്യങ്ങൾ, map->അക്കം_മൂല്യങ്ങൾ, o->മൂല്യങ്ങൾ_Capacity);

    map->മൂല്യങ്ങൾ[map->അക്കം_മൂല്യങ്ങൾ].slot = o->wasm_അട്ടി[i_index];
    map->മൂല്യങ്ങൾ[map->അക്കം_മൂല്യങ്ങൾ].type = o->type_അട്ടി[i_index];
    map->അക്കം_മൂല്യങ്ങൾ++;
    io_point->അക്കം_മൂല്യങ്ങൾ++;

    തിരികെ m3Err_none;
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
സ്ഥിതം
M3Result RecordSafePoint (IM3Compilation o, M3SafePointKind i_kind, u32 i_വാക്കുകൾ_Back, u16 i_അക്കം_Results,
                          u16 i_aux, IM3CompilationScope i_loop)
{
_try {
    M3SnapshotMap* map = o->snapshotMap;

    എങ്കിൽ (not map or not o->page) {
        തിരികെ m3Err_none;
    }

    d_m3GrowSnapshotArray(M3SafePoint, map->safePoints, map->അക്കം_Safe_Points, o->safePointsCapacity);

    // grown before it is pointed at: adding values never moves the safepoints
    M3SafePoint* point     = &map->safePoints[map->അക്കം_Safe_Points];
    point->pc              = GetPC(o) - i_വാക്കുകൾ_Back;
    point->wasmOffset      = GetWasmOffset(o);
    point->first_മൂല്യം      = map->അക്കം_മൂല്യങ്ങൾ;
    point->first_Local_മൂല്യം = UINT32_MAX;
    point->അക്കം_മൂല്യങ്ങൾ       = 0;
    point->അക്കം_Results      = i_അക്കം_Results;
    point->aux             = i_aux;
    point->resumeType      = c_m3Type_none;
    point->kind            = (u8)i_kind;
    point->flags           = 0;

    // Non-defaultable locals can still be uninitialized at a safepoint. Keep
    // their heap types, but allow the null placeholder until local.set makes
    // them definitely initialized. Branches to a loop use its entry context.
    എങ്കിൽ (o->localInitDepth) {
        point->first_Local_മൂല്യം = map->അക്കം_മൂല്യങ്ങൾ;
        വേണ്ടി (u32 i = 0; i < map->അക്കം_Locals; ++i) {
            d_m3GrowSnapshotArray(M3_Slot_മൂല്യം, map->മൂല്യങ്ങൾ, map->അക്കം_മൂല്യങ്ങൾ, o->മൂല്യങ്ങൾ_Capacity);
            M3_Slot_മൂല്യം മൂല്യം = map->locals[i];
            എങ്കിൽ (o->localInitDepth[i] == UINT32_MAX or
                (i_loop and o->localInitDepth[i] >= (u32)i_loop->depth)) {
                മൂല്യം.type &= ~d_m3Type_refNonNull;
            }
            map->മൂല്യങ്ങൾ[map->അക്കം_മൂല്യങ്ങൾ++] = മൂല്യം;
        }
        point->first_മൂല്യം = map->അക്കം_മൂല്യങ്ങൾ;
    }

    u16 മുകൾ = o->അട്ടി_Index;

    എങ്കിൽ (i_kind != safepoint_suspend) {
        മുകൾ = (മുകൾ > i_അക്കം_Results) ? (u16)(മുകൾ - i_അക്കം_Results) : 0;
    }
    എങ്കിൽ (i_loop) {
        മുകൾ = M3_MIN(മുകൾ, i_loop->exit_അട്ടി_Index);
    }

    വേണ്ടി (u16 i = o->അട്ടി_First_Dynamic_Index; i < മുകൾ; ++i) {
        എങ്കിൽ (Is_Live_അട്ടി_Entry(o, i)) {
_           (Add_Safe_Point_മൂല്യം(o, point, i));
        }
    }

    എങ്കിൽ (i_loop) {
        u16 അക്കം_Params = Get_Func_Type_അക്കം_Params(i_loop->type);

        വേണ്ടി (u16 i = 0; i < അക്കം_Params; ++i) {
_           (Add_Safe_Point_മൂല്യം(o, point, (u16)(i_loop->exit_അട്ടി_Index + i)));
        }
    }

    map->അക്കം_Safe_Points++;

} _catch: തിരികെ result;
}

// A back edge: op_ContinueLoop, or op_ContinueLoopIf when i_isConditional, and
// the loop it goes back to. The conditional one suspends before it has taken
// the branch, so it has to be resumed with the condition that took it.
സ്ഥിതം
M3Result RecordBackEdge (IM3Compilation o, IM3CompilationScope i_loop, bool i_isConditional)
{
    M3Result result = m3Err_none;

    // the loop is named by its record in the map, which has to fit the field
    _throwif("a function has too many loops for its snapshot map",
             o->snapshotMap and o->page and (i_loop->snapshotBlock == 0 or i_loop->snapshotBlock > UINT16_MAX + 1u));

_   (RecordSafePoint(o, safepoint_op, 2, 0, (u16)(i_loop->snapshotBlock - 1), i_loop));

    എങ്കിൽ (i_isConditional and o->snapshotMap and o->page) {
        o->snapshotMap->safePoints[o->snapshotMap->അക്കം_Safe_Points - 1].flags |= d_m3SafePointTakenBranch;
    }

    _catch: തിരികെ result;
}

// A call, waiting on its callee with i_numResults not yet written; the
// callee's frame starts i_calleeSlot slots above this one. A return_call that
// is compiled as a call is one too, since its frame stands while the callee
// runs, but it is marked: in Wasm the callee has replaced it, so a snapshot
// walks past it rather than writing it.
സ്ഥിതം
M3Result RecordCall (IM3Compilation o, u16 i_അക്കം_Results, u16 i_calleeSlot, bool i_isReturnCall)
{
    M3Result result = RecordSafePoint(o, safepoint_call, 0, i_അക്കം_Results, i_calleeSlot, ശൂന്യ);

    എങ്കിൽ (not result and i_isReturnCall and o->snapshotMap and o->page) {
        o->snapshotMap->safePoints[o->snapshotMap->അക്കം_Safe_Points - 1].flags |= d_m3SafePointTailCall;
    }

    തിരികെ result;
}

// Where the function's arguments and locals are kept. Unlike the operand
// stack, that never changes, so it is recorded once rather than at every
// safepoint.
സ്ഥിതം
M3Result RecordLocals (IM3Compilation o)
{
    M3SnapshotMap* map = o->snapshotMap;

    എങ്കിൽ (map and o->അട്ടി_Index) {
        map->locals = m3_AllocArray(M3_Slot_മൂല്യം, o->അട്ടി_Index);

        എങ്കിൽ (not map->locals) {
            തിരികെ m3Err_mallocFailed;
        }

        വേണ്ടി (u16 i = 0; i < o->അട്ടി_Index; ++i) {
            map->locals[i].slot = o->wasm_അട്ടി[i];
            map->locals[i].type = o->type_അട്ടി[i];
        }

        map->അക്കം_Locals = o->അട്ടി_Index;
        വേണ്ടി (u32 i = Get_Function_അക്കം_Args(o->function); i < map->അക്കം_Locals; ++i) {
            എങ്കിൽ (IsRefType(map->locals[i].type) and not IsNullableRef(map->locals[i].type)) {
                o->localInitDepth = m3_AllocArray(u32, map->അക്കം_Locals);
                എങ്കിൽ (not o->localInitDepth) {
                    തിരികെ m3Err_mallocFailed;
                }
                വേണ്ടി (u32 j = i; j < map->അക്കം_Locals; ++j) {
                    എങ്കിൽ (IsRefType(map->locals[j].type) and not IsNullableRef(map->locals[j].type)) {
                        o->localInitDepth[j] = UINT32_MAX;
                    }
                }
                നിർത്തുക;
            }
        }
    }

    തിരികെ m3Err_none;
}

// A loop or a try_table, whose native frame a snapshot names by the
// instruction's offset. i_pc is what that frame records. The scope being
// compiled keeps the record, so that RecordBlockEnd can close it.
സ്ഥിതം
M3Result RecordBlockStart (IM3Compilation o, m3opcode_t i_opcode, pc_t i_pc, u32 i_അക്കം_Clauses)
{
    M3SnapshotMap* map = o->snapshotMap;

    എങ്കിൽ (map and o->page) {
        d_m3GrowSnapshotArray(M3BlockStart, map->blocks, map->അക്കം_Blocks, o->blocksCapacity);

        M3BlockStart* block = &map->blocks[map->അക്കം_Blocks++];
        block->pc           = i_pc;
        block->wasmOffset   = GetWasmOffset(o);
        block->wasmEnd      = UINT32_MAX;
        block->അക്കം_Clauses   = (u16)i_അക്കം_Clauses;
        block->opcode       = (u8)i_opcode;

        o->block.snapshotBlock = map->അക്കം_Blocks;
    }

    തിരികെ m3Err_none;
}

// Closes the scope's region, while the instruction just read is its end
സ്ഥിതം
ശൂന്യം RecordBlockEnd (IM3Compilation o)
{
    എങ്കിൽ (o->snapshotMap and o->block.snapshotBlock) {
        o->snapshotMap->blocks[o->block.snapshotBlock - 1].wasmEnd = GetWasmOffset(o);
    }
}

#  undef d_m3GrowSnapshotArray

#അല്ലെങ്കിൽ

സ്ഥിതം വരിയിൽ
M3Result RecordSafePoint (IM3Compilation o, M3SafePointKind i_kind, u32 i_വാക്കുകൾ_Back, u16 i_അക്കം_Results,
                          u16 i_aux, IM3CompilationScope i_loop)
{
    (ശൂന്യം)o;
    (ശൂന്യം)i_kind;
    (ശൂന്യം)i_വാക്കുകൾ_Back;
    (ശൂന്യം)i_അക്കം_Results;
    (ശൂന്യം)i_aux;
    (ശൂന്യം)i_loop;
    തിരികെ m3Err_none;
}

സ്ഥിതം വരിയിൽ
M3Result RecordBackEdge (IM3Compilation o, IM3CompilationScope i_loop, bool i_isConditional)
{
    (ശൂന്യം)o;
    (ശൂന്യം)i_loop;
    (ശൂന്യം)i_isConditional;
    തിരികെ m3Err_none;
}

സ്ഥിതം വരിയിൽ
M3Result RecordCall (IM3Compilation o, u16 i_അക്കം_Results, u16 i_calleeSlot, bool i_isReturnCall)
{
    (ശൂന്യം)o;
    (ശൂന്യം)i_അക്കം_Results;
    (ശൂന്യം)i_calleeSlot;
    (ശൂന്യം)i_isReturnCall;
    തിരികെ m3Err_none;
}

#endif // d_m3HasSnapshots

സ്ഥിതം M3_NOINLINE
M3Result Ensure_Code_Page_അക്കം_വരികൾ (IM3Compilation o, u32 i_അക്കം_വരികൾ)
{
    M3Result result = m3Err_none;

    i_അക്കം_വരികൾ += 2; // room for Bridge

    എങ്കിൽ (M3_UNLIKELY(അക്കം_Free_വരികൾ(o->page) < i_അക്കം_വരികൾ)) {
        IM3CodePage page = AcquireCodePageWithCapacity(o->runtime, i_അക്കം_വരികൾ);

        എങ്കിൽ (page) {
            m3log(emit, "bridging new code page from: %d %p (free slots: %d) to: %d", o->page->info.sequence, GetPC(o), അക്കം_Free_വരികൾ(o->page), page->info.sequence);
            d_m3Assert(അക്കം_Free_വരികൾ(o->page) >= 2);

            EmitWord(o->page, op_Branch);
            EmitWord(o->page, GetPagePC(page));

            ReleaseCodePage(o->runtime, o->page);

            o->page = page;
        } അല്ലെങ്കിൽ {
            result = m3Err_mallocFailedCodePage;
        }
    }

    തിരികെ result;
}

// invalidate the pending local.set fold candidate; called wherever code is emitted or a
// branch target captures the current pc (a fold appends an immediate, moving that position)
സ്ഥിതം വരിയിൽ
ശൂന്യം InvalidateFold (IM3Compilation o)
{
#എങ്കിൽ d_m3FoldSetLocal
    o->foldPatchPC = ശൂന്യ;
#അല്ലെങ്കിൽ
    (ശൂന്യം)o;
#endif
}

സ്ഥിതം M3_NOINLINE
M3Result EmitOp (IM3Compilation o, IM3Operation i_operation)
{
    M3Result result = m3Err_none;                                 d_m3Assert (i_operation or Is_അട്ടി_Polymorphic (o));

    InvalidateFold(o);

    // it's OK for page to be null; when compile-walking the bytecode without emitting
    എങ്കിൽ (o->page) {
#എങ്കിൽ d_m3EnableOpTracing
        എങ്കിൽ (i_operation != op_Dump_അട്ടി) {
            o->അക്കം_Emits++;
        }
#endif

        // have execution jump to a new page if slots are critically low
        result = Ensure_Code_Page_അക്കം_വരികൾ(o, d_m3_Code_Page_Free_വരികൾ_Threshold);

        എങ്കിൽ (M3_LIKELY(not result)) {
            എങ്കിൽ (d_m3LogEmit) {
                log_emit(o, i_operation);
            }
#എങ്കിൽ d_m3RecordBacktraces
            EmitMappingEntry(o->page, (u32)(o->lastOpcodeStart - o->module->wasmStart));
#endif // d_m3RecordBacktraces
            EmitWord(o->page, i_operation);
        }
    }

    തിരികെ result;
}

// Push an immediate constant into the M3 codestream
സ്ഥിതം M3_NOINLINE
ശൂന്യം EmitConstant32 (IM3Compilation o, സ്ഥിരം u32 i_immediate)
{
    എങ്കിൽ (o->page) {                                                          m3log (emit, "const32: %ud", i_immediate);
        EmitWord32(o->page, i_immediate);
    }
}

#എങ്കിൽ d_m3HasMemory64
// Takes two lines of the code page where a pointer is 32 bits, so whatever
// reads it back has to step _pc by the same amount - see op_Const64.
സ്ഥിതം M3_NOINLINE
ശൂന്യം EmitConstant64 (IM3Compilation o, സ്ഥിരം u64 i_immediate)
{
    എങ്കിൽ (o->page) {
        EmitWord64(o->page, i_immediate);
    }
}
#endif

സ്ഥിതം M3_NOINLINE
ശൂന്യം EmitSlotOffset (IM3Compilation o, സ്ഥിരം i32 i_offset)
{
    എങ്കിൽ (o->page) {                                                          m3log (emit, "slot: [%d]", i_offset);
        EmitWord32(o->page, i_offset);
    }
}

സ്ഥിതം M3_NOINLINE
pc_t EmitPointer (IM3Compilation o, സ്ഥിരം ശൂന്യം* സ്ഥിരം i_pointer)
{
    pc_t ptr = GetPagePC(o->page);

    എങ്കിൽ (o->page) {                                                          m3log (emit, "ptr: %p", i_pointer);
        EmitWord(o->page, i_pointer);
    }

    തിരികെ ptr;
}

സ്ഥിതം M3_NOINLINE
ശൂന്യം* ReservePointer (IM3Compilation o)
{
    pc_t ptr = GetPagePC(o->page);
    EmitPointer(o, ശൂന്യ);
    തിരികെ (ശൂന്യം*)ptr;
}


//-------------------------------------------------------------------------------------------------------------------------

#define d_indent "     | %s"

// just want less letters and numbers to stare at down the way in the compiler table
#define i_32    c_m3Type_i32
#define i_64    c_m3Type_i64
#define f_32    c_m3Type_f32
#define f_64    c_m3Type_f64
#define none    c_m3Type_none
#define any     (u8)-1

#എങ്കിൽ d_m3HasFloat
#  define FPOP(x) x
#അല്ലെങ്കിൽ
#  define FPOP(x) ശൂന്യ
#endif

// These are indexed by M3ValueType, so every type up to c_m3Type_externref needs
// an entry. A reference is one pointer-sized word, so it moves with the integer
// operation of that width; v128 has no operations at all.
#എങ്കിൽ M3_SIZEOF_PTR == 8
#  define REFOP(NAME)  op_##NAME##_i64
#അല്ലെങ്കിൽ
#  define REFOP(NAME)  op_##NAME##_i32
#endif

// clang-format off

സ്ഥിതം സ്ഥിരം IM3Operation c_preserveSetSlot[] = { ശൂന്യ, op_PreserveSetSlot_i32,       op_PreserveSetSlot_i64,
                                                    FPOP(op_PreserveSetSlot_f32), FPOP(op_PreserveSetSlot_f64),
                                                    ശൂന്യ, REFOP(PreserveSetSlot),  REFOP(PreserveSetSlot),
                                                    REFOP(PreserveSetSlot),       REFOP(PreserveSetSlot) };
സ്ഥിതം സ്ഥിരം IM3Operation c_setSetOps[] =       { ശൂന്യ, op_SetSlot_i32,               op_SetSlot_i64,
                                                    FPOP(op_SetSlot_f32),         FPOP(op_SetSlot_f64),
                                                    ശൂന്യ, REFOP(SetSlot),         REFOP(SetSlot),
                                                    REFOP(SetSlot),               REFOP(SetSlot) };
സ്ഥിതം സ്ഥിരം IM3Operation c_setGlobalOps[] =    { ശൂന്യ, op_SetGlobal_i32,             op_SetGlobal_i64,
                                                    FPOP(op_SetGlobal_f32),       FPOP(op_SetGlobal_f64),
                                                    ശൂന്യ, REFOP(SetGlobal),       REFOP(SetGlobal),
                                                    REFOP(SetGlobal),             REFOP(SetGlobal) };
സ്ഥിതം സ്ഥിരം IM3Operation c_setRegisterOps[] =  { ശൂന്യ, op_SetRegister_i32,           op_SetRegister_i64,
                                                    FPOP(op_SetRegister_f32),     FPOP(op_SetRegister_f64),
                                                    ശൂന്യ, REFOP(SetRegister),     REFOP(SetRegister),
                                                    REFOP(SetRegister),           REFOP(SetRegister) };

// A table shorter than the enum reads out of bounds, so tie the two together at
// build time: adding a value type must not silently outgrow these.
#define d_m3CheckTypeTable(TABLE) \
    M3_STATIC_ASSERT (വലുപ്പം (TABLE) / വലുപ്പം (*(TABLE)) == c_m3_Type_എണ്ണം, TABLE##_needs_one_entry_per_type)

d_m3CheckTypeTable(c_preserveSetSlot);
d_m3CheckTypeTable(c_setSetOps);
d_m3CheckTypeTable(c_setGlobalOps);
d_m3CheckTypeTable(c_setRegisterOps);

#എങ്കിൽ d_m3FoldSetLocal

// destination-folded variants of the hot binops, indexed by the operand-form index
// recorded at emission: [0]=_rs [1]=_sr [2]=_ss [3]=(fp _rr, never folded)
#define d_foldOpList(TYPE, NAME)            { op_##TYPE##_##NAME##_rs_f, op_##TYPE##_##NAME##_sr_f, op_##TYPE##_##NAME##_ss_f, ശൂന്യ }
#define d_foldCommutativeOpList(TYPE, NAME) { op_##TYPE##_##NAME##_rs_f, ശൂന്യ,                      op_##TYPE##_##NAME##_ss_f, ശൂന്യ }

സ്ഥിതം സ്ഥിരം IM3Operation c_fold_i32_Add[]         = d_foldCommutativeOpList (i32, Add);
സ്ഥിതം സ്ഥിരം IM3Operation c_fold_i32_Subtract[]    = d_foldOpList (i32, Subtract);
സ്ഥിതം സ്ഥിരം IM3Operation c_fold_i32_Multiply[]    = d_foldCommutativeOpList (i32, Multiply);
സ്ഥിതം സ്ഥിരം IM3Operation c_fold_u32_And[]         = d_foldCommutativeOpList (u32, And);
സ്ഥിതം സ്ഥിരം IM3Operation c_fold_u32_Or[]          = d_foldCommutativeOpList (u32, Or);
സ്ഥിതം സ്ഥിരം IM3Operation c_fold_u32_Xor[]         = d_foldCommutativeOpList (u32, Xor);
സ്ഥിതം സ്ഥിരം IM3Operation c_fold_u32_Shift_ഇടത്[]   = d_foldOpList (u32, Shift_ഇടത്);
സ്ഥിതം സ്ഥിരം IM3Operation c_fold_i32_Shift_വലത്[]  = d_foldOpList (i32, Shift_വലത്);
സ്ഥിതം സ്ഥിരം IM3Operation c_fold_u32_Shift_വലത്[]  = d_foldOpList (u32, Shift_വലത്);

സ്ഥിതം സ്ഥിരം IM3Operation c_fold_i64_Add[]         = d_foldCommutativeOpList (i64, Add);
സ്ഥിതം സ്ഥിരം IM3Operation c_fold_i64_Subtract[]    = d_foldOpList (i64, Subtract);
സ്ഥിതം സ്ഥിരം IM3Operation c_fold_i64_Multiply[]    = d_foldCommutativeOpList (i64, Multiply);
സ്ഥിതം സ്ഥിരം IM3Operation c_fold_u64_And[]         = d_foldCommutativeOpList (u64, And);
സ്ഥിതം സ്ഥിരം IM3Operation c_fold_u64_Or[]          = d_foldCommutativeOpList (u64, Or);
സ്ഥിതം സ്ഥിരം IM3Operation c_fold_u64_Xor[]         = d_foldCommutativeOpList (u64, Xor);
സ്ഥിതം സ്ഥിരം IM3Operation c_fold_u64_Shift_ഇടത്[]   = d_foldOpList (u64, Shift_ഇടത്);
സ്ഥിതം സ്ഥിരം IM3Operation c_fold_i64_Shift_വലത്[]  = d_foldOpList (i64, Shift_വലത്);
സ്ഥിതം സ്ഥിരം IM3Operation c_fold_u64_Shift_വലത്[]  = d_foldOpList (u64, Shift_വലത്);

#  എങ്കിൽ d_m3HasFloat
സ്ഥിതം സ്ഥിരം IM3Operation c_fold_f32_Add[]         = d_foldCommutativeOpList (f32, Add);
സ്ഥിതം സ്ഥിരം IM3Operation c_fold_f32_Subtract[]    = d_foldOpList (f32, Subtract);
സ്ഥിതം സ്ഥിരം IM3Operation c_fold_f32_Multiply[]    = d_foldCommutativeOpList (f32, Multiply);
സ്ഥിതം സ്ഥിരം IM3Operation c_fold_f32_Divide[]      = d_foldOpList (f32, Divide);
സ്ഥിതം സ്ഥിരം IM3Operation c_fold_f64_Add[]         = d_foldCommutativeOpList (f64, Add);
സ്ഥിതം സ്ഥിരം IM3Operation c_fold_f64_Subtract[]    = d_foldOpList (f64, Subtract);
സ്ഥിതം സ്ഥിരം IM3Operation c_fold_f64_Multiply[]    = d_foldCommutativeOpList (f64, Multiply);
സ്ഥിതം സ്ഥിരം IM3Operation c_fold_f64_Divide[]      = d_foldOpList (f64, Divide);
#  endif

// loads are unary: [0]= address in _r0, [1]= address in a slot
#define d_foldLoadList(DEST, SRC)       { op_##DEST##_Load_##SRC##_r_f, op_##DEST##_Load_##SRC##_s_f, ശൂന്യ, ശൂന്യ }

സ്ഥിതം സ്ഥിരം IM3Operation c_fold_i32_Load_i32[] = d_foldLoadList(i32, i32);
സ്ഥിതം സ്ഥിരം IM3Operation c_fold_i32_Load_i8[] = d_foldLoadList(i32, i8);
സ്ഥിതം സ്ഥിരം IM3Operation c_fold_i32_Load_u8[] = d_foldLoadList(i32, u8);
സ്ഥിതം സ്ഥിരം IM3Operation c_fold_i32_Load_i16[] = d_foldLoadList(i32, i16);
സ്ഥിതം സ്ഥിരം IM3Operation c_fold_i32_Load_u16[] = d_foldLoadList(i32, u16);

സ്ഥിതം സ്ഥിരം IM3Operation c_fold_i64_Load_i64[] = d_foldLoadList(i64, i64);
സ്ഥിതം സ്ഥിരം IM3Operation c_fold_i64_Load_i8[] = d_foldLoadList(i64, i8);
സ്ഥിതം സ്ഥിരം IM3Operation c_fold_i64_Load_u8[] = d_foldLoadList(i64, u8);
സ്ഥിതം സ്ഥിരം IM3Operation c_fold_i64_Load_i16[] = d_foldLoadList(i64, i16);
സ്ഥിതം സ്ഥിരം IM3Operation c_fold_i64_Load_u16[] = d_foldLoadList(i64, u16);
സ്ഥിതം സ്ഥിരം IM3Operation c_fold_i64_Load_i32[] = d_foldLoadList(i64, i32);
സ്ഥിതം സ്ഥിരം IM3Operation c_fold_i64_Load_u32[] = d_foldLoadList(i64, u32);
#  എങ്കിൽ d_m3HasFloat
സ്ഥിതം സ്ഥിരം IM3Operation c_fold_f32_Load_f32[] = d_foldLoadList(f32, f32);
സ്ഥിതം സ്ഥിരം IM3Operation c_fold_f64_Load_f64[] = d_foldLoadList(f64, f64);
#  endif

സ്ഥിതം
IM3Operation GetFoldOp (m3opcode_t i_opcode, u8 i_form)
{
    എങ്കിൽ (i_form >= 4) {
        തിരികെ ശൂന്യ;
    }

    തിരഞ്ഞെടുപ്പ് (i_opcode) {
    അവസ്ഥ 0x6a:  തിരികെ c_fold_i32_Add        [i_form];      // i32.add
    അവസ്ഥ 0x6b:  തിരികെ c_fold_i32_Subtract   [i_form];      // i32.sub
    അവസ്ഥ 0x6c:  തിരികെ c_fold_i32_Multiply   [i_form];      // i32.mul
    അവസ്ഥ 0x71:  തിരികെ c_fold_u32_And        [i_form];      // i32.and
    അവസ്ഥ 0x72:  തിരികെ c_fold_u32_Or         [i_form];      // i32.or
    അവസ്ഥ 0x73:  തിരികെ c_fold_u32_Xor        [i_form];      // i32.xor
    അവസ്ഥ 0x74:  തിരികെ c_fold_u32_Shift_ഇടത്  [i_form];      // i32.shl
    അവസ്ഥ 0x75:  തിരികെ c_fold_i32_Shift_വലത് [i_form];      // i32.shr_s
    അവസ്ഥ 0x76:  തിരികെ c_fold_u32_Shift_വലത് [i_form];      // i32.shr_u

    അവസ്ഥ 0x7c:  തിരികെ c_fold_i64_Add        [i_form];      // i64.add
    അവസ്ഥ 0x7d:  തിരികെ c_fold_i64_Subtract   [i_form];      // i64.sub
    അവസ്ഥ 0x7e:  തിരികെ c_fold_i64_Multiply   [i_form];      // i64.mul
    അവസ്ഥ 0x83:  തിരികെ c_fold_u64_And        [i_form];      // i64.and
    അവസ്ഥ 0x84:  തിരികെ c_fold_u64_Or         [i_form];      // i64.or
    അവസ്ഥ 0x85:  തിരികെ c_fold_u64_Xor        [i_form];      // i64.xor
    അവസ്ഥ 0x86:  തിരികെ c_fold_u64_Shift_ഇടത്  [i_form];      // i64.shl
    അവസ്ഥ 0x87:  തിരികെ c_fold_i64_Shift_വലത് [i_form];      // i64.shr_s
    അവസ്ഥ 0x88:  തിരികെ c_fold_u64_Shift_വലത് [i_form];      // i64.shr_u

#  എങ്കിൽ d_m3HasFloat
    അവസ്ഥ 0x92:  തിരികെ c_fold_f32_Add        [i_form];      // f32.add
    അവസ്ഥ 0x93:  തിരികെ c_fold_f32_Subtract   [i_form];      // f32.sub
    അവസ്ഥ 0x94:  തിരികെ c_fold_f32_Multiply   [i_form];      // f32.mul
    അവസ്ഥ 0x95:  തിരികെ c_fold_f32_Divide     [i_form];      // f32.div
    അവസ്ഥ 0xa0:  തിരികെ c_fold_f64_Add        [i_form];      // f64.add
    അവസ്ഥ 0xa1:  തിരികെ c_fold_f64_Subtract   [i_form];      // f64.sub
    അവസ്ഥ 0xa2:  തിരികെ c_fold_f64_Multiply   [i_form];      // f64.mul
    അവസ്ഥ 0xa3:  തിരികെ c_fold_f64_Divide     [i_form];      // f64.div
#  endif

    അവസ്ഥ 0x28:  തിരികെ c_fold_i32_Load_i32   [i_form];      // i32.load
    അവസ്ഥ 0x2c:  തിരികെ c_fold_i32_Load_i8    [i_form];      // i32.load8_s
    അവസ്ഥ 0x2d:  തിരികെ c_fold_i32_Load_u8    [i_form];      // i32.load8_u
    അവസ്ഥ 0x2e:  തിരികെ c_fold_i32_Load_i16   [i_form];      // i32.load16_s
    അവസ്ഥ 0x2f:  തിരികെ c_fold_i32_Load_u16   [i_form];      // i32.load16_u

    അവസ്ഥ 0x29:  തിരികെ c_fold_i64_Load_i64   [i_form];      // i64.load
    അവസ്ഥ 0x30:  തിരികെ c_fold_i64_Load_i8    [i_form];      // i64.load8_s
    അവസ്ഥ 0x31:  തിരികെ c_fold_i64_Load_u8    [i_form];      // i64.load8_u
    അവസ്ഥ 0x32:  തിരികെ c_fold_i64_Load_i16   [i_form];      // i64.load16_s
    അവസ്ഥ 0x33:  തിരികെ c_fold_i64_Load_u16   [i_form];      // i64.load16_u
    അവസ്ഥ 0x34:  തിരികെ c_fold_i64_Load_i32   [i_form];      // i64.load32_s
    അവസ്ഥ 0x35:  തിരികെ c_fold_i64_Load_u32   [i_form];      // i64.load32_u

#  എങ്കിൽ d_m3HasFloat
    അവസ്ഥ 0x2a:  തിരികെ c_fold_f32_Load_f32   [i_form];      // f32.load
    അവസ്ഥ 0x2b:  തിരികെ c_fold_f64_Load_f64   [i_form];      // f64.load
#  endif

    സ്ഥിരസ്ഥിതി: തിരികെ ശൂന്യ;
    }
}

#endif // d_m3FoldSetLocal

// clang-format on

#എങ്കിൽ d_m3FuseBranch

// fused compare+branch/if variants, indexed like the fold lists: [0]=_rs [1]=_sr [2]=_ss [3]=unused.
// the [1] entries are NULL for commutative compares: the compiler never records the _sr form for them
#  define d_fuseCmpList(TYPE, NAME, KIND)             { op_##TYPE##_##KIND##_##NAME##_rs, op_##TYPE##_##KIND##_##NAME##_sr, op_##TYPE##_##KIND##_##NAME##_ss, ശൂന്യ }
#  define d_fuseCommutativeCmpList(TYPE, NAME, KIND)  { op_##TYPE##_##KIND##_##NAME##_rs, ശൂന്യ,                             op_##TYPE##_##KIND##_##NAME##_ss, ശൂന്യ }

തരനിർവചനം ഘടന M3FusedCmpOps {
    IM3Operation branchIf[4];
    IM3Operation ifOp[4];
} M3FusedCmpOps;

#  define d_fuseCmp(TYPE, NAME)               { d_fuseCmpList (TYPE, NAME, BranchIf),            d_fuseCmpList (TYPE, NAME, If) }
#  define d_fuseCommutativeCmp(TYPE, NAME)    { d_fuseCommutativeCmpList (TYPE, NAME, BranchIf), d_fuseCommutativeCmpList (TYPE, NAME, If) }

// clang-format off
സ്ഥിതം സ്ഥിരം M3FusedCmpOps c_fuse_i32_Equal              = d_fuseCommutativeCmp (i32, Equal);
സ്ഥിതം സ്ഥിരം M3FusedCmpOps c_fuse_i32_NotEqual           = d_fuseCommutativeCmp (i32, NotEqual);
സ്ഥിതം സ്ഥിരം M3FusedCmpOps c_fuse_i32_LessThan           = d_fuseCmp (i32, LessThan);
സ്ഥിതം സ്ഥിരം M3FusedCmpOps c_fuse_u32_LessThan           = d_fuseCmp (u32, LessThan);
സ്ഥിതം സ്ഥിരം M3FusedCmpOps c_fuse_i32_GreaterThan        = d_fuseCmp (i32, GreaterThan);
സ്ഥിതം സ്ഥിരം M3FusedCmpOps c_fuse_u32_GreaterThan        = d_fuseCmp (u32, GreaterThan);
സ്ഥിതം സ്ഥിരം M3FusedCmpOps c_fuse_i32_LessThanOrEqual    = d_fuseCmp (i32, LessThanOrEqual);
സ്ഥിതം സ്ഥിരം M3FusedCmpOps c_fuse_u32_LessThanOrEqual    = d_fuseCmp (u32, LessThanOrEqual);
സ്ഥിതം സ്ഥിരം M3FusedCmpOps c_fuse_i32_GreaterThanOrEqual = d_fuseCmp (i32, GreaterThanOrEqual);
സ്ഥിതം സ്ഥിരം M3FusedCmpOps c_fuse_u32_GreaterThanOrEqual = d_fuseCmp (u32, GreaterThanOrEqual);
// clang-format on

// i32.eqz is unary: [0]= operand in _r0, [1]= operand in a slot
സ്ഥിതം സ്ഥിരം M3FusedCmpOps c_fuse_i32_Eqz = {
    { op_i32_BranchIfEqz_r, op_i32_BranchIfEqz_s, ശൂന്യ, ശൂന്യ },
    { op_i32_IfEqz_r,       op_i32_IfEqz_s,       ശൂന്യ, ശൂന്യ }
};

സ്ഥിതം
സ്ഥിരം M3FusedCmpOps* GetFusedCmpOps (m3opcode_t i_opcode)
{
    തിരഞ്ഞെടുപ്പ് (i_opcode) {
    അവസ്ഥ 0x45: തിരികെ &c_fuse_i32_Eqz;                    // i32.eqz
    അവസ്ഥ 0x46: തിരികെ &c_fuse_i32_Equal;                  // i32.eq
    അവസ്ഥ 0x47: തിരികെ &c_fuse_i32_NotEqual;               // i32.ne
    അവസ്ഥ 0x48: തിരികെ &c_fuse_i32_LessThan;               // i32.lt_s
    അവസ്ഥ 0x49: തിരികെ &c_fuse_u32_LessThan;               // i32.lt_u
    അവസ്ഥ 0x4a: തിരികെ &c_fuse_i32_GreaterThan;            // i32.gt_s
    അവസ്ഥ 0x4b: തിരികെ &c_fuse_u32_GreaterThan;            // i32.gt_u
    അവസ്ഥ 0x4c: തിരികെ &c_fuse_i32_LessThanOrEqual;        // i32.le_s
    അവസ്ഥ 0x4d: തിരികെ &c_fuse_u32_LessThanOrEqual;        // i32.le_u
    അവസ്ഥ 0x4e: തിരികെ &c_fuse_i32_GreaterThanOrEqual;     // i32.ge_s
    അവസ്ഥ 0x4f: തിരികെ &c_fuse_u32_GreaterThanOrEqual;     // i32.ge_u
    സ്ഥിരസ്ഥിതി: തിരികെ ശൂന്യ;
    }
}

#endif // d_m3FuseBranch

സ്ഥിതം സ്ഥിരം IM3Operation c_intSelectOps[2][4] = {
    { op_Select_i32_rss, op_Select_i32_srs, op_Select_i32_ssr, op_Select_i32_sss },
    { op_Select_i64_rss, op_Select_i64_srs, op_Select_i64_ssr, op_Select_i64_sss }
};
#എങ്കിൽ d_m3HasFloat
സ്ഥിതം സ്ഥിരം IM3Operation c_fpSelectOps[2][2][3] = {
    { { op_Select_f32_sss, op_Select_f32_srs, op_Select_f32_ssr }, // selector in slot
      { op_Select_f32_rss, op_Select_f32_rrs, op_Select_f32_rsr } }, // selector in reg
    { { op_Select_f64_sss, op_Select_f64_srs, op_Select_f64_ssr }, // selector in slot
      { op_Select_f64_rss, op_Select_f64_rrs, op_Select_f64_rsr } }
};    // selector in reg
#endif

// all args & returns are 64-bit aligned, so use 2 slots for a d_m3Use32BitSlots=1 build

സ്ഥിതം
M3Result AcquireCompilationCodePage (IM3Compilation o, IM3CodePage* o_codePage)
{
    M3Result result = m3Err_none;

    IM3CodePage page = AcquireCodePage(o->runtime);

    എങ്കിൽ (page) {
#എങ്കിൽ (d_m3EnableCodePageRefCounting)
        {
            എങ്കിൽ (o->function) {
                IM3Function func = o->function;
                page->info.usage_എണ്ണം++;

                u32 index = func->അക്കം_Code_Page_Refs++;
_               (m3ReallocArray(&func->codePageRefs, IM3CodePage, func->അക്കം_Code_Page_Refs, index));
                func->codePageRefs[index] = page;
            }
        }
#endif
    } അല്ലെങ്കിൽ {
        _throw(m3Err_mallocFailedCodePage);
    }

    _catch:

    *o_codePage = page;

    തിരികെ result;
}

സ്ഥിതം വരിയിൽ
ശൂന്യം ReleaseCompilationCodePage (IM3Compilation o)
{
    ReleaseCodePage(o->runtime, o->page);
}

സ്ഥിതം വരിയിൽ
u16 Get_Type_അക്കം_Slots (m3type_t i_type)
{
    i_type = BaseTypeOf(i_type);

    // v128 is 16 bytes - 4 slots in 32-bit-slot mode, 2 in 64-bit.
    // (Slot-allocator only; no v128 ops execute.)
    എങ്കിൽ (i_type == c_m3Type_v128)
#എങ്കിൽ d_m3Use32BitSlots
        തിരികെ 4;
#അല്ലെങ്കിൽ
        തിരികെ 2;
#endif
#എങ്കിൽ d_m3Use32BitSlots
    തിരികെ Is64BitType(i_type) ? 2 : 1;
#അല്ലെങ്കിൽ
    തിരികെ 1;
#endif
}

സ്ഥിതം വരിയിൽ
ശൂന്യം AlignSlotToType (u16* io_slot, m3type_t i_type)
{
    // align 64-bit words to even slots (if d_m3Use32BitSlots)
    u16 അക്കം_Slots = Get_Type_അക്കം_Slots(i_type);

    u16 mask = അക്കം_Slots - 1;
    *io_slot = (*io_slot + mask) & ~mask;
}

// Deliberately unchecked. Validation rejects an empty operand stack before compilation.
സ്ഥിതം വരിയിൽ
i16 Get_അട്ടി_മുകൾ_Index (IM3Compilation o)
{
    തിരികെ o->അട്ടി_Index - 1;
}

സ്ഥിതം വരിയിൽ
M3Result Get_അട്ടി_മുകൾ_Index_Throws (IM3Compilation o, i16* o_അട്ടി_Index)
{
    *o_അട്ടി_Index = o->അട്ടി_Index - 1;

    എങ്കിൽ (M3_LIKELY(o->അട്ടി_Index > o->അട്ടി_First_Dynamic_Index or Is_അട്ടി_Polymorphic(o))) {
        തിരികെ m3Err_none;
    } അല്ലെങ്കിൽ {
        തിരികെ m3_Err_function_അട്ടി_Underrun;
    }
}


// Items in the static portion of the stack (args/locals) are hidden from GetStackTypeFromTop ()
// In other words, only "real" Wasm stack items can be inspected.  This is important when
// returning values, etc. and you need an accurate wasm-view of the stack.
സ്ഥിതം
m3type_t Get_അട്ടി_Type_From_മുകൾ (IM3Compilation o, u16 i_offset)
{
    m3type_t type = c_m3Type_none;

    ++i_offset;
    എങ്കിൽ (o->അട്ടി_Index >= i_offset) {
        u16 index = o->അട്ടി_Index - i_offset;

        എങ്കിൽ (index >= o->അട്ടി_First_Dynamic_Index) {
            type = o->type_അട്ടി[index];
        }
    }

    തിരികെ type;
}

സ്ഥിതം വരിയിൽ
m3type_t Get_അട്ടി_മുകൾ_Type (IM3Compilation o)
{
    തിരികെ Get_അട്ടി_Type_From_മുകൾ(o, 0);
}

സ്ഥിതം വരിയിൽ
m3type_t Get_അട്ടി_Type_From_Bottom (IM3Compilation o, u16 i_offset)
{
    m3type_t type = c_m3Type_none;

    എങ്കിൽ (i_offset < o->അട്ടി_Index) {
        type = o->type_അട്ടി[i_offset];
    }

    തിരികെ type;
}


സ്ഥിതം വരിയിൽ
bool IsConstantSlot (IM3Compilation o, u16 i_slot)
{
    തിരികെ (i_slot >= o->slotFirstConstIndex and i_slot < o->slotMaxConstIndex);
}
സ്ഥിതം വരിയിൽ
bool IsSlotAllocated (IM3Compilation o, u16 i_slot)
{
    തിരികെ o->m3Slots[i_slot];
}

സ്ഥിതം വരിയിൽ
bool Is_അട്ടി_Index_In_Register (IM3Compilation o, i32 i_അട്ടി_Index)
{                                                                           d_m3Assert (i_അട്ടി_Index < o->അട്ടി_Index or Is_അട്ടി_Polymorphic (o));
    എങ്കിൽ (i_അട്ടി_Index >= 0 and i_അട്ടി_Index < o->അട്ടി_Index) {
        തിരികെ (o->wasm_അട്ടി[i_അട്ടി_Index] >= d_m3Reg0SlotAlias);
    } അല്ലെങ്കിൽ {
        തിരികെ false;
    }
}

സ്ഥിതം വരിയിൽ
u16 Get_അക്കം_Block_മൂല്യങ്ങൾ_On_അട്ടി (IM3Compilation o)
{
    തിരികെ o->അട്ടി_Index - o->block.block_അട്ടി_Index;
}

സ്ഥിതം വരിയിൽ
bool Is_അട്ടി_മുകൾ_In_Register (IM3Compilation o)
{
    തിരികെ Is_അട്ടി_Index_In_Register(o, (i32)Get_അട്ടി_മുകൾ_Index(o));
}
സ്ഥിതം വരിയിൽ
bool Is_അട്ടി_മുകൾ_Minus1_In_Register (IM3Compilation o)
{
    തിരികെ Is_അട്ടി_Index_In_Register(o, (i32)Get_അട്ടി_മുകൾ_Index(o) - 1);
}
സ്ഥിതം വരിയിൽ
bool Is_അട്ടി_മുകൾ_Minus2_In_Register (IM3Compilation o)
{
    തിരികെ Is_അട്ടി_Index_In_Register(o, (i32)Get_അട്ടി_മുകൾ_Index(o) - 2);
}

സ്ഥിതം വരിയിൽ
bool Is_അട്ടി_മുകൾ_In_Slot (IM3Compilation o)
{
    തിരികെ not Is_അട്ടി_മുകൾ_In_Register(o);
}

സ്ഥിതം വരിയിൽ
bool IsValidSlot (u16 i_slot)
{
    തിരികെ (i_slot < d_m3MaxFunctionSlots);
}

സ്ഥിതം വരിയിൽ
u16 Get_അട്ടി_മുകൾ_Slot_സംഖ്യ (IM3Compilation o)
{
    i16 i = Get_അട്ടി_മുകൾ_Index(o);

    u16 slot = c_slotUnused;

    എങ്കിൽ (i >= 0) {
        slot = o->wasm_അട്ടി[i];
    }

    തിരികെ slot;
}


// from bottom
സ്ഥിതം വരിയിൽ
u16 Get_Slot_For_അട്ടി_Index (IM3Compilation o, u16 i_അട്ടി_Index)
{                                                                   d_m3Assert (i_അട്ടി_Index < o->അട്ടി_Index or Is_അട്ടി_Polymorphic (o));
    u16 slot = c_slotUnused;

    എങ്കിൽ (i_അട്ടി_Index < o->അട്ടി_Index) {
        slot = o->wasm_അട്ടി[i_അട്ടി_Index];
    }

    തിരികെ slot;
}

സ്ഥിതം വരിയിൽ
u16 Get_Extra_Slot_For_അട്ടി_Index (IM3Compilation o, u16 i_അട്ടി_Index)
{
    u16 baseSlot = Get_Slot_For_അട്ടി_Index(o, i_അട്ടി_Index);

    എങ്കിൽ (baseSlot != c_slotUnused) {
        u16 extraSlot = Get_Type_അക്കം_Slots(Get_അട്ടി_Type_From_Bottom(o, i_അട്ടി_Index)) - 1;
        baseSlot += extraSlot;
    }

    തിരികെ baseSlot;
}


സ്ഥിതം വരിയിൽ
ശൂന്യം TouchSlot (IM3Compilation o, u16 i_slot)
{
    // op_Entry uses this value to track and detect stack overflow
    o->max_അട്ടി_Slots = M3_MAX(o->max_അട്ടി_Slots, i_slot + 1);
}

സ്ഥിതം വരിയിൽ
ശൂന്യം MarkSlotAllocated (IM3Compilation o, u16 i_slot)
{                                                                   d_m3Assert (o->m3Slots [i_slot] == 0); // shouldn't be already allocated
    o->m3Slots[i_slot] = 1;

    o->slotMaxAllocatedIndexPlusOne = M3_MAX(o->slotMaxAllocatedIndexPlusOne, i_slot + 1);

    TouchSlot(o, i_slot);
}

സ്ഥിതം വരിയിൽ
M3Result MarkSlotsAllocated (IM3Compilation o, u16 i_slot, u16 i_അക്കം_Slots)
{
    എങ്കിൽ (M3_UNLIKELY(i_slot + i_അക്കം_Slots > d_m3MaxFunctionSlots)) {
        തിരികെ m3_Err_function_അട്ടി_Overflow;
    }

    ആകുമ്പോൾ (i_അക്കം_Slots--) {
        MarkSlotAllocated(o, i_slot++);
    }

    തിരികെ m3Err_none;
}

സ്ഥിതം വരിയിൽ
M3Result MarkSlotsAllocatedByType (IM3Compilation o, u16 i_slot, m3type_t i_type)
{
    u16 അക്കം_Slots = Get_Type_അക്കം_Slots(i_type);
    തിരികെ MarkSlotsAllocated(o, i_slot, അക്കം_Slots);
}


സ്ഥിതം
M3Result AllocateSlotsWithinRange (IM3Compilation o, u16* o_slot, m3type_t i_type, u16 i_startSlot, u16 i_endSlot)
{
    M3Result result = m3_Err_function_അട്ടി_Overflow;

    u16 അക്കം_Slots     = Get_Type_അക്കം_Slots(i_type);
    u16 തിരയൽ_Offset = അക്കം_Slots - 1;

    AlignSlotToType(&i_startSlot, i_type);

    // search for 1 or 2 consecutive slots in the execution stack
    u16 i = i_startSlot;
    ആകുമ്പോൾ (i + തിരയൽ_Offset < i_endSlot) {
        എങ്കിൽ (i + തിരയൽ_Offset < d_m3MaxFunctionSlots and o->m3Slots[i] == 0 and o->m3Slots[i + തിരയൽ_Offset] == 0) {
            MarkSlotsAllocated(o, i, അക്കം_Slots);

            *o_slot = i;
            result  = m3Err_none;
            നിർത്തുക;
        }

        // keep 2-slot allocations even-aligned
        i += അക്കം_Slots;
    }

    തിരികെ result;
}

സ്ഥിതം വരിയിൽ
M3Result AllocateSlots (IM3Compilation o, u16* o_slot, m3type_t i_type)
{
    തിരികെ AllocateSlotsWithinRange(o, o_slot, i_type, o->slotFirstDynamicIndex, d_m3MaxFunctionSlots);
}

സ്ഥിതം വരിയിൽ
M3Result AllocateConstantSlots (IM3Compilation o, u16* o_slot, m3type_t i_type)
{
    u16 maxTableIndex = o->slotFirstConstIndex + d_m3MaxConstantTableSize;
    തിരികെ AllocateSlotsWithinRange(o, o_slot, i_type, o->slotFirstConstIndex, M3_MIN(o->slotFirstDynamicIndex, maxTableIndex));
}


// TOQUE: this usage count system could be eliminated. real world code doesn't frequently trigger it.  just copy to multiple
// unique slots.
സ്ഥിതം വരിയിൽ
M3Result Increment_Slot_Usage_എണ്ണം (IM3Compilation o, u16 i_slot)
{                                                                                       d_m3Assert (i_slot < d_m3MaxFunctionSlots);
    M3Result result = m3Err_none;                                                       d_m3Assert (o->m3Slots [i_slot] > 0);

    // OPTZ (memory): 'm3Slots' could still be fused with 'typeStack' if 4 bits were used to indicate: [0,1,2,many]. The many-case
    // would scan 'wasmStack' to determine the actual usage count
    എങ്കിൽ (M3_LIKELY(o->m3Slots[i_slot] < 0xFF)) {
        o->m3Slots[i_slot]++;
    } അല്ലെങ്കിൽ {
        result = "slot usage count overflow";
    }

    തിരികെ result;
}

സ്ഥിതം വരിയിൽ
ശൂന്യം DeallocateSlot (IM3Compilation o, i16 i_slot, m3type_t i_type)
{                                                                                       d_m3Assert (i_slot >= o->slotFirstDynamicIndex);
                                                                                        d_m3Assert (i_slot < o->slotMaxAllocatedIndexPlusOne);
    വേണ്ടി (u16 i = 0; i < Get_Type_അക്കം_Slots(i_type); ++i, ++i_slot) {                       d_m3Assert (o->m3Slots [i_slot]);
        --o->m3Slots[i_slot];
    }
}


സ്ഥിതം വരിയിൽ
bool IsRegisterTypeAllocated (IM3Compilation o, u8 i_type)
{
    തിരികെ IsRegisterAllocated(o, IsFpType(i_type));
}

സ്ഥിതം വരിയിൽ
ശൂന്യം AllocateRegister (IM3Compilation o, u32 i_register, u16 i_അട്ടി_Index)
{                                                                                       d_m3Assert (not IsRegisterAllocated (o, i_register));
    o->reg_അട്ടി_Index_Plus_One[i_register] = i_അട്ടി_Index + 1;
}

സ്ഥിതം വരിയിൽ
ശൂന്യം DeallocateRegister (IM3Compilation o, u32 i_register)
{                                                                                       d_m3Assert (IsRegisterAllocated (o, i_register));
    o->reg_അട്ടി_Index_Plus_One[i_register] = c_m3RegisterUnallocated;
}

സ്ഥിതം വരിയിൽ
u16 Get_Register_അട്ടി_Index (IM3Compilation o, u32 i_register)
{                                                                                       d_m3Assert (IsRegisterAllocated (o, i_register));
    തിരികെ o->reg_അട്ടി_Index_Plus_One[i_register] - 1;
}

u16 GetMaxUsedSlotPlusOne (IM3Compilation o)
{
    ആകുമ്പോൾ (o->slotMaxAllocatedIndexPlusOne > o->slotFirstDynamicIndex) {
        എങ്കിൽ (IsSlotAllocated(o, o->slotMaxAllocatedIndexPlusOne - 1)) {
            നിർത്തുക;
        }

        o->slotMaxAllocatedIndexPlusOne--;
    }

#ifdef DEBUG
    u16 maxSlot = o->slotMaxAllocatedIndexPlusOne;
    ആകുമ്പോൾ (maxSlot < d_m3MaxFunctionSlots) {
        d_m3Assert(o->m3Slots[maxSlot] == 0);
        maxSlot++;
    }
#endif

    തിരികെ o->slotMaxAllocatedIndexPlusOne;
}

സ്ഥിതം
M3Result PreserveRegisterIfOccupied (IM3Compilation o, m3type_t i_registerType)
{
    M3Result result = m3Err_none;

    u32 regSelect = IsFpType(i_registerType);

    എങ്കിൽ (IsRegisterAllocated(o, regSelect)) {
        u16 അട്ടി_Index = Get_Register_അട്ടി_Index(o, regSelect);
        DeallocateRegister(o, regSelect);

        m3type_t type = Get_അട്ടി_Type_From_Bottom(o, അട്ടി_Index);

        // and point to a exec slot
        u16 slot = c_slotUnused;
_       (AllocateSlots(o, &slot, type));
        o->wasm_അട്ടി[അട്ടി_Index] = slot;

        // Ensure type is within the valid range
        എങ്കിൽ (M3_LIKELY(BaseTypeOf(type) < c_m3_Type_എണ്ണം)) {
_           (EmitOp(o, c_setSetOps[BaseTypeOf(type)]));
        } അല്ലെങ്കിൽ {
            _throw(m3Err_unknownType);
        }

        EmitSlotOffset(o, slot);
    }

    _catch: തിരികെ result;
}


// all values must be in slots before entering loop, if, and else blocks
// otherwise they'd end up preserve-copied in the block to probably different locations (if/else)
സ്ഥിതം വരിയിൽ
M3Result PreserveRegisters (IM3Compilation o)
{
    M3Result result;

_   (PreserveRegisterIfOccupied(o, c_m3Type_f64));
_   (PreserveRegisterIfOccupied(o, c_m3Type_i64));

    _catch: തിരികെ result;
}

സ്ഥിതം
M3Result Preserve_Non_മുകൾ_Registers (IM3Compilation o)
{
    M3Result result = m3Err_none;

    i16 അട്ടി_മുകൾ = Get_അട്ടി_മുകൾ_Index(o);

    എങ്കിൽ (അട്ടി_മുകൾ >= 0) {
        എങ്കിൽ (IsRegisterAllocated(o, 0))     // r0
        {
            എങ്കിൽ (Get_Register_അട്ടി_Index(o, 0) != അട്ടി_മുകൾ) {
_               (PreserveRegisterIfOccupied(o, c_m3Type_i64));
            }
        }

        എങ്കിൽ (IsRegisterAllocated(o, 1))     // fp0
        {
            എങ്കിൽ (Get_Register_അട്ടി_Index(o, 1) != അട്ടി_മുകൾ) {
_               (PreserveRegisterIfOccupied(o, c_m3Type_f64));
            }
        }
    }

    _catch: തിരികെ result;
}


//----------------------------------------------------------------------------------------------------------------------

സ്ഥിതം
M3Result തള്ളുക (IM3Compilation o, m3type_t i_type, u16 i_slot)
{
    M3Result result = m3Err_none;

#എങ്കിൽ !d_m3HasFloat
    എങ്കിൽ (i_type == c_m3Type_f32 || i_type == c_m3Type_f64) {
        തിരികെ m3Err_unknownOpcode;
    }
#endif

    എങ്കിൽ (M3_UNLIKELY(o->അട്ടി_Index >= d_m3_Max_Function_അട്ടി_Height)) {
        തിരികെ m3_Err_function_അട്ടി_Overflow;
    }

    u16 അട്ടി_Index = o->അട്ടി_Index++;                                       // printf ("push: %d\n", (i32) i);

    o->wasm_അട്ടി[അട്ടി_Index] = i_slot;
    o->type_അട്ടി[അട്ടി_Index] = i_type;

    എങ്കിൽ (IsRegisterSlotAlias(i_slot)) {
        u32 regSelect = IsFpRegisterSlotAlias(i_slot);
        AllocateRegister(o, regSelect, അട്ടി_Index);
    }

    എങ്കിൽ (d_m3_Log_Wasm_അട്ടി) {
        dump_type_അട്ടി(o);
    }

    തിരികെ result;
}

സ്ഥിതം വരിയിൽ
M3Result തള്ളുക_Register (IM3Compilation o, m3type_t i_type)
{
    M3Result result = m3Err_none;                                                       d_m3Assert ((u16) d_m3Reg0SlotAlias > (u16) d_m3MaxFunctionSlots);
    u16      slot   = IsFpType(i_type) ? d_m3Fp0SlotAlias : d_m3Reg0SlotAlias;          d_m3Assert (i_type or Is_അട്ടി_Polymorphic (o));

_   (തള്ളുക(o, i_type, slot));

    _catch: തിരികെ result;
}

സ്ഥിതം
M3Result എടുക്കുക (IM3Compilation o)
{
    M3Result result = m3Err_none;

    എങ്കിൽ (M3_LIKELY(o->അട്ടി_Index > o->block.block_അട്ടി_Index)) {
        o->അട്ടി_Index--;                                                //  printf ("pop: %d\n", (i32) o->stackIndex);

        u16      slot = o->wasm_അട്ടി[o->അട്ടി_Index];
        m3type_t type = o->type_അട്ടി[o->അട്ടി_Index];

        എങ്കിൽ (IsRegisterSlotAlias(slot)) {
            u32 regSelect = IsFpRegisterSlotAlias(slot);
            DeallocateRegister(o, regSelect);
        } അല്ലെങ്കിൽ എങ്കിൽ (M3_UNLIKELY(slot >= o->slotMaxAllocatedIndexPlusOne)) {
            തിരികെ m3_Err_function_അട്ടി_Underrun; // Return error for invalid slot indices
        } അല്ലെങ്കിൽ എങ്കിൽ (slot >= o->slotFirstDynamicIndex) {
            DeallocateSlot(o, slot, type);
        }
    } അല്ലെങ്കിൽ എങ്കിൽ (not Is_അട്ടി_Polymorphic(o)) {
        result = m3_Err_function_അട്ടി_Underrun;
    }

    തിരികെ result;
}

സ്ഥിതം
M3Result എടുക്കുക_Type (IM3Compilation o, m3type_t i_type)
{
    M3Result result = m3Err_none;

    m3type_t മുകൾ_Type = Get_അട്ടി_മുകൾ_Type(o);

    എങ്കിൽ (M3_LIKELY(IsSubTypeOf(മുകൾ_Type, i_type) or o->block.isPolymorphic)) {
_       (എടുക്കുക(o));
    } അല്ലെങ്കിൽ {
        _throw(m3Err_typeMismatch);
    }

    _catch:
    തിരികെ result;
}

സ്ഥിതം
M3Result തള്ളുക_Allocated_Slot_And_Emit (IM3Compilation o, m3type_t i_type, bool i_doEmit)
{
    M3Result result = m3Err_none;

    u16 slot = c_slotUnused;

_   (AllocateSlots(o, &slot, i_type));
_   (തള്ളുക(o, i_type, slot));

    എങ്കിൽ (i_doEmit) {
        EmitSlotOffset(o, slot);
    }

    //    printf ("push: %d\n", (u32) slot);

    _catch: തിരികെ result;
}

സ്ഥിതം വരിയിൽ
M3Result തള്ളുക_Allocated_Slot_And_Emit_2 (IM3Compilation o, m3type_t i_type)
{
    തിരികെ തള്ളുക_Allocated_Slot_And_Emit(o, i_type, true);
}

സ്ഥിതം വരിയിൽ
M3Result തള്ളുക_Allocated_Slot (IM3Compilation o, m3type_t i_type)
{
    തിരികെ തള്ളുക_Allocated_Slot_And_Emit(o, i_type, false);
}

സ്ഥിതം
M3Result തള്ളുക_Const (IM3Compilation o, u64 i_word, m3type_t i_type)
{
    M3Result result = m3Err_none;

    // When compile-walking a constant expression without emitting there is no
    // constant table to place the value in, but the type still has to land on
    // the stack: extended-const arithmetic downstream consumes it.
    എങ്കിൽ (M3_UNLIKELY(!o->page)) {
        തിരികെ തള്ളുക_Allocated_Slot(o, i_type);
    }

    bool matchFound  = false;
    bool is64BitType = Is64BitType(i_type);

    u16 അക്കം_Required_Slots  = Get_Type_അക്കം_Slots(i_type);
    u16 അക്കം_Used_Const_Slots = o->slotMaxConstIndex - o->slotFirstConstIndex;

    // search for duplicate matching constant slot to reuse
    എങ്കിൽ (അക്കം_Required_Slots == 2 and അക്കം_Used_Const_Slots >= 2) {
        u16 firstConstSlot = o->slotFirstConstIndex;
        AlignSlotToType(&firstConstSlot, c_m3Type_i64);

        വേണ്ടി (u16 slot = firstConstSlot; slot < o->slotMaxConstIndex - 1; slot += 2) {
            എങ്കിൽ (IsSlotAllocated(o, slot) and IsSlotAllocated(o, slot + 1)) {
                u64 constant;
                memcpy(&constant, &o->constants[slot - o->slotFirstConstIndex], വലുപ്പം(constant));

                എങ്കിൽ (constant == i_word) {
                    matchFound = true;
_                   (തള്ളുക(o, i_type, slot));
                    നിർത്തുക;
                }
            }
        }
    } അല്ലെങ്കിൽ എങ്കിൽ (അക്കം_Required_Slots == 1) {
        വേണ്ടി (u16 i = 0; i < അക്കം_Used_Const_Slots; ++i) {
            u16 slot = o->slotFirstConstIndex + i;

            എങ്കിൽ (IsSlotAllocated(o, slot)) {
                bool matches;
                എങ്കിൽ (is64BitType) {
                    u64 constant;
                    memcpy(&constant, &o->constants[i], വലുപ്പം(constant));
                    matches = (constant == i_word);
                } അല്ലെങ്കിൽ {
                    u32 constant;
                    memcpy(&constant, &o->constants[i], വലുപ്പം(constant));
                    matches = (constant == i_word);
                }
                എങ്കിൽ (matches) {
                    matchFound = true;
_                   (തള്ളുക(o, i_type, slot));
                    നിർത്തുക;
                }
            }
        }
    }

    എങ്കിൽ (not matchFound) {
        u16 slot = c_slotUnused;
        result   = AllocateConstantSlots(o, &slot, i_type);

        എങ്കിൽ (result || slot == c_slotUnused) // no more constant table space; use inline constants
        {
            result = m3Err_none;

            എങ്കിൽ (is64BitType) {
_               (EmitOp(o, op_Const64));
                EmitWord64(o->page, i_word);
            } അല്ലെങ്കിൽ {
_               (EmitOp(o, op_Const32));
                EmitWord32(o->page, (u32)i_word);
            }

_           (തള്ളുക_Allocated_Slot_And_Emit_2(o, i_type));
        } അല്ലെങ്കിൽ {
            u16 constTableIndex = slot - o->slotFirstConstIndex;

            d_m3Assert(constTableIndex < d_m3MaxConstantTableSize);

            എങ്കിൽ (is64BitType) {
                memcpy(&o->constants[constTableIndex], &i_word, വലുപ്പം(i_word));
            } അല്ലെങ്കിൽ {
                u32 word32 = (u32)i_word;
                memcpy(&o->constants[constTableIndex], &word32, വലുപ്പം(word32));
            }

_           (തള്ളുക(o, i_type, slot));

            o->slotMaxConstIndex = M3_MAX(slot + അക്കം_Required_Slots, o->slotMaxConstIndex);
        }
    }

    _catch: തിരികെ result;
}

സ്ഥിതം വരിയിൽ
M3Result Emit_Slot_അക്കം_Of_അട്ടി_മുകൾ_And_എടുക്കുക (IM3Compilation o)
{
    // no emit if value is in register
    എങ്കിൽ (Is_അട്ടി_മുകൾ_In_Slot(o)) {
        EmitSlotOffset(o, Get_അട്ടി_മുകൾ_Slot_സംഖ്യ(o));
    }

    തിരികെ എടുക്കുക(o);
}


// Or, maybe: EmitTrappingOp
M3Result AddTrapRecord (IM3Compilation o)
{
    M3Result result = m3Err_none;

    എങ്കിൽ (o->function) {
    }

    തിരികെ result;
}

സ്ഥിതം
M3Result Unwind_Block_അട്ടി (IM3Compilation o)
{
    M3Result result = m3Err_none;

    u32 എടുക്കുക_എണ്ണം = 0;
    ആകുമ്പോൾ (o->അട്ടി_Index > o->block.block_അട്ടി_Index) {
_       (എടുക്കുക(o));
        ++എടുക്കുക_എണ്ണം;
    }

    എങ്കിൽ (എടുക്കുക_എണ്ണം) {
        m3log(compile, "unwound stack top: %d", എടുക്കുക_എണ്ണം);
    }

    _catch: തിരികെ result;
}

സ്ഥിതം വരിയിൽ
M3Result Set_അട്ടി_Polymorphic (IM3Compilation o)
{
    o->block.isPolymorphic = true;                              m3log (compile, "stack set polymorphic");
    തിരികെ Unwind_Block_അട്ടി(o);
}

സ്ഥിതം
ശൂന്യം PatchBranches (IM3Compilation o)
{
    InvalidateFold(o);         // patched branches land at the current pc

    pc_t pc = GetPC(o);

    pc_t patches     = o->block.patches;
    o->block.patches = ശൂന്യ;

    ആകുമ്പോൾ (patches) {                                           m3log (compile, "patching location: %p to pc: %p", patches, pc);
        pc_t next       = *(pc_t*)patches;
        *(pc_t*)patches = pc;
        patches         = next;
    }
}

//-------------------------------------------------------------------------------------------------------------------------

സ്ഥിതം
M3Result Copy_അട്ടി_Index_To_Slot (IM3Compilation o, u16 i_destSlot, u16 i_അട്ടി_Index)  // NoPushPop
{
    M3Result result = m3Err_none;

    IM3Operation op;

    m3type_t type       = Get_അട്ടി_Type_From_Bottom(o, i_അട്ടി_Index);
    bool     inRegister = Is_അട്ടി_Index_In_Register(o, i_അട്ടി_Index);

    എങ്കിൽ (inRegister) {
        op = c_setSetOps[BaseTypeOf(type)];
    } അല്ലെങ്കിൽ {
        op = Is64BitType(type) ? op_CopySlot_64 : op_CopySlot_32;
    }

_   (EmitOp(o, op));
    EmitSlotOffset(o, i_destSlot);

    എങ്കിൽ (not inRegister) {
        u16 srcSlot = Get_Slot_For_അട്ടി_Index(o, i_അട്ടി_Index);
        EmitSlotOffset(o, srcSlot);
    }

    _catch: തിരികെ result;
}

സ്ഥിതം
M3Result Copy_അട്ടി_മുകൾ_To_Slot (IM3Compilation o, u16 i_destSlot)  // NoPushPop
{
    M3Result result;

    i16 അട്ടി_മുകൾ;
_   (Get_അട്ടി_മുകൾ_Index_Throws(o, &അട്ടി_മുകൾ));
_   (Copy_അട്ടി_Index_To_Slot(o, i_destSlot, (u16)അട്ടി_മുകൾ));

    _catch: തിരികെ result;
}


// a copy-on-write strategy is used with locals. when a get local occurs, it's not copied anywhere. the stack
// entry just has a index pointer to that local memory slot.
// then, when a previously referenced local is set, the current value needs to be preserved for those references

// TODO: consider getting rid of these specialized operations: PreserveSetSlot & PreserveCopySlot.
// They likely just take up space (which seems to reduce performance) without improving performance.
// => unconfirmed in preliminary testing.
സ്ഥിതം
M3Result Preserved_Copy_മുകൾ_Slot (IM3Compilation o, u16 i_destSlot, u16 i_preserveSlot)
{
    M3Result result = m3Err_none;             d_m3Assert (i_destSlot != i_preserveSlot);

    IM3Operation op;

    m3type_t type = Get_അട്ടി_മുകൾ_Type(o);

    എങ്കിൽ (Is_അട്ടി_മുകൾ_In_Register(o)) {
        op = c_preserveSetSlot[BaseTypeOf(type)];
    } അല്ലെങ്കിൽ {
        op = Is64BitType(type) ? op_PreserveCopySlot_64 : op_PreserveCopySlot_32;
    }

_   (EmitOp(o, op));
    EmitSlotOffset(o, i_destSlot);

    എങ്കിൽ (Is_അട്ടി_മുകൾ_In_Slot(o)) {
        EmitSlotOffset(o, Get_അട്ടി_മുകൾ_Slot_സംഖ്യ(o));
    }

    EmitSlotOffset(o, i_preserveSlot);

    _catch: തിരികെ result;
}

സ്ഥിതം
M3Result Copy_അട്ടി_മുകൾ_To_Register (IM3Compilation o, bool i_update_അട്ടി)
{
    M3Result result = m3Err_none;

    എങ്കിൽ (Is_അട്ടി_മുകൾ_In_Slot(o)) {
        m3type_t type = Get_അട്ടി_മുകൾ_Type(o);

_       (PreserveRegisterIfOccupied(o, type));

        IM3Operation op = c_setRegisterOps[BaseTypeOf(type)];

_       (EmitOp(o, op));
        EmitSlotOffset(o, Get_അട്ടി_മുകൾ_Slot_സംഖ്യ(o));

        എങ്കിൽ (i_update_അട്ടി) {
_           (എടുക്കുക_Type(o, type));
_           (തള്ളുക_Register(o, type));
        }
    }

    _catch: തിരികെ result;
}


// if local is unreferenced, o_preservedSlotNumber will be equal to localIndex on return
സ്ഥിതം
M3Result FindReferencedLocalWithinCurrentBlock (IM3Compilation o, u16* o_preserved_Slot_സംഖ്യ, u32 i_localSlot)
{
    M3Result result = m3Err_none;

    IM3CompilationScope scope = &o->block;

    u16 startIndex = scope->block_അട്ടി_Index;

    ആകുമ്പോൾ (scope->opcode == c_waOp_block) {
        scope = scope->outer;
        എങ്കിൽ (not scope) {
            നിർത്തുക;
        }

        startIndex = scope->block_അട്ടി_Index;
    }

    *o_preserved_Slot_സംഖ്യ = (u16)i_localSlot;

    വേണ്ടി (u32 i = startIndex; i < o->അട്ടി_Index; ++i) {
        എങ്കിൽ (o->wasm_അട്ടി[i] == i_localSlot) {
            m3type_t type = Get_അട്ടി_Type_From_Bottom(o, i);                         d_m3Assert (type != c_m3Type_none)

            എങ്കിൽ (*o_preserved_Slot_സംഖ്യ == i_localSlot) {
_               (AllocateSlots(o, o_preserved_Slot_സംഖ്യ, type));
            } അല്ലെങ്കിൽ {
                // A 64-bit value spans two slots, and allocating or releasing it
                // counts both, so a second reference has to count both as well.
                വേണ്ടി (u16 s = 0; s < Get_Type_അക്കം_Slots(type); ++s) {
_                   (Increment_Slot_Usage_എണ്ണം(o, (u16)(*o_preserved_Slot_സംഖ്യ + s)));
                }
            }

            o->wasm_അട്ടി[i] = *o_preserved_Slot_സംഖ്യ;
        }
    }

    _catch: തിരികെ result;
}

സ്ഥിതം
M3Result GetBlockScope (IM3Compilation o, IM3CompilationScope* o_scope, u32 i_depth)
{
    M3Result result = m3Err_none;

    IM3CompilationScope scope = &o->block;

    ആകുമ്പോൾ (i_depth--) {
        scope = scope->outer;
        _throwif("invalid block depth", not scope);
    }

    *o_scope = scope;

    _catch:
    തിരികെ result;
}

സ്ഥിതം
M3Result Copy_അട്ടി_Slots_R (IM3Compilation o, u16 i_target_Slot_അട്ടി_Index, u16 i_അട്ടി_Index, u16 i_end_അട്ടി_Index, u16 i_tempSlot)
{
    M3Result result = m3Err_none;

    എങ്കിൽ (i_അട്ടി_Index < i_end_അട്ടി_Index) {
        u16 srcSlot = Get_Slot_For_അട്ടി_Index(o, i_അട്ടി_Index);

        m3type_t type      = Get_അട്ടി_Type_From_Bottom(o, i_അട്ടി_Index);
        u16      അക്കം_Slots  = Get_Type_അക്കം_Slots(type);
        u16      extraSlot = അക്കം_Slots - 1;

        u16 targetSlot = Get_Slot_For_അട്ടി_Index(o, i_target_Slot_അട്ടി_Index);

        u16 preserveIndex = i_അട്ടി_Index;
        u16 collisionSlot = srcSlot;

        എങ്കിൽ (targetSlot != srcSlot) {
            // search for collisions
            u16 checkIndex = i_അട്ടി_Index + 1;
            ആകുമ്പോൾ (checkIndex < i_end_അട്ടി_Index) {
                u16 otherSlot1 = Get_Slot_For_അട്ടി_Index(o, checkIndex);
                u16 otherSlot2 = Get_Extra_Slot_For_അട്ടി_Index(o, checkIndex);

                എങ്കിൽ (targetSlot == otherSlot1 or
                    targetSlot == otherSlot2 or
                    targetSlot + extraSlot == otherSlot1) {
                    _throwif(m3_Err_function_അട്ടി_Overflow, i_tempSlot >= d_m3MaxFunctionSlots);

_                   (Copy_അട്ടി_Index_To_Slot(o, i_tempSlot, checkIndex));
                    o->wasm_അട്ടി[checkIndex] = i_tempSlot;
                    i_tempSlot += Get_Type_അക്കം_Slots(c_m3Type_i64);
                    TouchSlot(o, i_tempSlot - 1);

                    // restore this on the way back down
                    preserveIndex = checkIndex;
                    collisionSlot = otherSlot1;

                    നിർത്തുക;
                }

                ++checkIndex;
            }

_           (Copy_അട്ടി_Index_To_Slot(o, targetSlot, i_അട്ടി_Index));                                                m3log (compile, " copying slot: %d to slot: %d", srcSlot, targetSlot);
            o->wasm_അട്ടി[i_അട്ടി_Index] = targetSlot;
        }

_       (Copy_അട്ടി_Slots_R(o, i_target_Slot_അട്ടി_Index + 1, i_അട്ടി_Index + 1, i_end_അട്ടി_Index, i_tempSlot));

        // restore the stack state
        o->wasm_അട്ടി[i_അട്ടി_Index]  = srcSlot;
        o->wasm_അട്ടി[preserveIndex] = collisionSlot;
    }

    _catch:
    തിരികെ result;
}

സ്ഥിതം
M3Result ResolveBlockResults (IM3Compilation o, IM3CompilationScope i_targetBlock, bool i_isBranch)
{
    M3Result result = m3Err_none;
    എങ്കിൽ (d_m3_Log_Wasm_അട്ടി) {
        dump_type_അട്ടി(o);
    }

    bool isLoop = (i_targetBlock->opcode == c_waOp_loop and i_isBranch);

    u16 അക്കം_Params  = Get_Func_Type_അക്കം_Params(i_targetBlock->type);
    u16 അക്കം_Results = Get_Func_Type_അക്കം_Results(i_targetBlock->type);

    u16 slotRecords = i_targetBlock->exit_അട്ടി_Index;

    u16 അക്കം_മൂല്യങ്ങൾ;

    എങ്കിൽ (not isLoop) {
        അക്കം_മൂല്യങ്ങൾ = അക്കം_Results;
        slotRecords += അക്കം_Params;
    } അല്ലെങ്കിൽ {
        അക്കം_മൂല്യങ്ങൾ = അക്കം_Params;
    }

    u16 blockHeight = Get_അക്കം_Block_മൂല്യങ്ങൾ_On_അട്ടി(o);

    _throwif(m3_Err_type_എണ്ണം_Mismatch, i_isBranch ? (blockHeight < അക്കം_മൂല്യങ്ങൾ) : (blockHeight != അക്കം_മൂല്യങ്ങൾ));

    എങ്കിൽ (അക്കം_മൂല്യങ്ങൾ) {
        u16 endIndex     = Get_അട്ടി_മുകൾ_Index(o) + 1;
        u16 അക്കം_Rem_മൂല്യങ്ങൾ = അക്കം_മൂല്യങ്ങൾ;

        // The last result is taken from _fp0. See PushBlockResults.
        എങ്കിൽ (not isLoop and IsFpType(Get_അട്ടി_മുകൾ_Type(o))) {
_           (Copy_അട്ടി_മുകൾ_To_Register(o, false));
            --endIndex;
            --അക്കം_Rem_മൂല്യങ്ങൾ;
        }

        // tempslot affects maxStackSlots, so can grow unnecess each time.
        // in practice, this is hardly ever hit.
        u16 tempSlot = o->max_അട്ടി_Slots;// GetMaxUsedSlotPlusOne (o); doesn't work cause can collide with slotRecords
        AlignSlotToType(&tempSlot, c_m3Type_i64);

_       (Copy_അട്ടി_Slots_R(o, slotRecords, endIndex - അക്കം_Rem_മൂല്യങ്ങൾ, endIndex, tempSlot));

        എങ്കിൽ (d_m3_Log_Wasm_അട്ടി) {
            dump_type_അട്ടി(o);
        }
    }

    _catch: തിരികെ result;
}


സ്ഥിതം
M3Result Return_മൂല്യങ്ങൾ (IM3Compilation o, IM3CompilationScope i_functionBlock, bool i_isBranch)
{
    M3Result result = m3Err_none;
    എങ്കിൽ (d_m3_Log_Wasm_അട്ടി) {
        dump_type_അട്ടി(o);
    }

    u16 അക്കം_Returns  = Get_Func_Type_അക്കം_Results(i_functionBlock->type);     // could just o->function too...
    u16 blockHeight = Get_അക്കം_Block_മൂല്യങ്ങൾ_On_അട്ടി(o);

    എങ്കിൽ (not Is_അട്ടി_Polymorphic(o)) {
        _throwif(m3_Err_type_എണ്ണം_Mismatch, i_isBranch ? (blockHeight < അക്കം_Returns) : (blockHeight != അക്കം_Returns));
    }

    എങ്കിൽ (അക്കം_Returns) {
        // return slots like args are 64-bit aligned
        u16 returnSlot = അക്കം_Returns * c_io_Slot_എണ്ണം;
        u16 അട്ടി_മുകൾ   = Get_അട്ടി_മുകൾ_Index(o);

        വേണ്ടി (u16 i = 0; i < അക്കം_Returns; ++i) {
            m3type_t returnType = GetFuncTypeResultType(i_functionBlock->type, അക്കം_Returns - 1 - i);

            m3type_t അട്ടി_Type = Get_അട്ടി_Type_From_മുകൾ(o, i);  // using FromTop so that only dynamic items are checked

            എങ്കിൽ (Is_അട്ടി_Polymorphic(o) and അട്ടി_Type == c_m3Type_none) {
                അട്ടി_Type = returnType;
            }

            _throwif(m3Err_typeMismatch, not IsSubTypeOf(അട്ടി_Type, returnType));

            എങ്കിൽ (not Is_അട്ടി_Polymorphic(o)) {
                returnSlot -= c_io_Slot_എണ്ണം;
_               (Copy_അട്ടി_Index_To_Slot(o, returnSlot, അട്ടി_മുകൾ--));
            }
        }

        എങ്കിൽ (not i_isBranch) {
            ആകുമ്പോൾ (അക്കം_Returns--) {
_               (എടുക്കുക(o));
            }
        }
    }

    _catch: തിരികെ result;
}


//-------------------------------------------------------------------------------------------------------------------------

സ്ഥിതം
M3Result Compile_Const_i32 (IM3Compilation o, m3opcode_t i_opcode)
{
    M3Result result;

    i32 മൂല്യം;
_   (ReadLEB_i32(&മൂല്യം, &o->wasm, o->wasmEnd));
_   (തള്ളുക_Const(o, മൂല്യം, c_m3Type_i32));                        m3log (compile, d_indent " (const i32 = %" PRIi32 ")", get_indention_string (o), മൂല്യം);
    _catch: തിരികെ result;
}

സ്ഥിതം
M3Result Compile_Const_i64 (IM3Compilation o, m3opcode_t i_opcode)
{
    M3Result result;

    i64 മൂല്യം;
_   (ReadLEB_i64(&മൂല്യം, &o->wasm, o->wasmEnd));
_   (തള്ളുക_Const(o, മൂല്യം, c_m3Type_i64));                        m3log (compile, d_indent " (const i64 = %" PRIi64 ")", get_indention_string (o), മൂല്യം);
    _catch: തിരികെ result;
}


#എങ്കിൽ d_m3ImplementFloat
സ്ഥിതം
M3Result Compile_Const_f32 (IM3Compilation o, m3opcode_t i_opcode)
{
    M3Result result;

    സംഘം {
        u32 u;
        f32 f;
    } മൂല്യം = { 0 };

_   (Read_f32(&മൂല്യം.f, &o->wasm, o->wasmEnd));                 m3log (compile, d_indent " (const f32 = %" PRIf32 ")", get_indention_string (o), മൂല്യം.f);
_   (തള്ളുക_Const(o, മൂല്യം.u, c_m3Type_f32));

    _catch: തിരികെ result;
}

സ്ഥിതം
M3Result Compile_Const_f64 (IM3Compilation o, m3opcode_t i_opcode)
{
    M3Result result;

    സംഘം {
        u64 u;
        f64 f;
    } മൂല്യം = { 0 };

_   (Read_f64(&മൂല്യം.f, &o->wasm, o->wasmEnd));                 m3log (compile, d_indent " (const f64 = %" PRIf64 ")", get_indention_string (o), മൂല്യം.f);
_   (തള്ളുക_Const(o, മൂല്യം.u, c_m3Type_f64));

    _catch: തിരികെ result;
}
#endif

സ്ഥിതം
M3Result Compile_ExtendedOpcode (IM3Compilation o, m3opcode_t i_opcode)
{
_try {
    u32 opcode;
_   (ReadLEB_u32(&opcode, &o->wasm, o->wasmEnd));               m3log (compile, d_indent " (%s: %" PRIu32 ")", get_indention_string (o), (i_opcode == c_waOp_atomic) ? "FE" : "FC", opcode);

    // m3opcode_t carries only the low byte, so a wider sub-opcode is unknown
    // here rather than something to truncate into range
    _throwif(m3Err_unknownOpcode, opcode > (u32)((i_opcode == c_waOp_atomic) ? c_waOp_lastAtomic : c_waOp_lastExtended));

    i_opcode = (m3opcode_t)((i_opcode << 8) | opcode);

    //printf("Extended opcode: 0x%x\n", i_opcode);

    IM3OpInfo opInfo = GetOpInfo(i_opcode);
    _throwif(m3Err_unknownOpcode, not opInfo);

    M3Compiler compiler = GetCompiler(opInfo);
    _throwif(m3Err_noCompiler, not compiler);

_   ((*compiler)(o, i_opcode));

    o->previousOpcode = i_opcode;

    } _catch: തിരികെ result;
}

സ്ഥിതം
M3Result Compile_Return (IM3Compilation o, m3opcode_t i_opcode)
{
    M3Result result = m3Err_none;

    എങ്കിൽ (not Is_അട്ടി_Polymorphic(o)) {
        IM3CompilationScope functionScope;
_       (GetBlockScope(o, &functionScope, o->block.depth));

_       (Return_മൂല്യങ്ങൾ(o, functionScope, true));

_       (EmitOp(o, op_Return));

_       (Set_അട്ടി_Polymorphic(o));
    }

    _catch: തിരികെ result;
}

സ്ഥിതം
M3Result ValidateBlockEnd (IM3Compilation o)
{
    M3Result result = m3Err_none;

    u16 അക്കം_Results  = Get_Func_Type_അക്കം_Results(o->block.type);
    u16 blockHeight = Get_അക്കം_Block_മൂല്യങ്ങൾ_On_അട്ടി(o);

    എങ്കിൽ (not Is_അട്ടി_Polymorphic(o)) {
        // Spec: at block end, stack height must match the number of results
        _throwif(m3_Err_type_എണ്ണം_Mismatch, blockHeight != അക്കം_Results);

        // Spec: result types must match expected types
        വേണ്ടി (u16 i = 0; i < അക്കം_Results; ++i) {
            m3type_t expectedType = GetFuncTypeResultType(o->block.type, അക്കം_Results - 1 - i);
            m3type_t actualType   = Get_അട്ടി_Type_From_മുകൾ(o, i);
            _throwif(m3Err_typeMismatch, not IsSubTypeOf(actualType, expectedType));
        }
    }

    _catch: തിരികെ result;
}

സ്ഥിതം
M3Result Compile_End (IM3Compilation o, m3opcode_t i_opcode)
{
    M3Result result = m3Err_none;                   //dump_type_stack (o);

    // function end:
    എങ്കിൽ (o->block.depth == 0) {
        ValidateBlockEnd(o);

        //      if (not IsStackPolymorphic (o))
        {
            // A constant expression has no function frame, but its value still
            // has to be copied down to slot 0, where EvaluateExpression reads it
            // back. The non-emitting init-expr walk is skipped: it has no block
            // type to return against.
            എങ്കിൽ (o->function or o->page) {
_               (Return_മൂല്യങ്ങൾ(o, &o->block, false));
            }

_           (EmitOp(o, op_Return));
        }
    }

    _catch: തിരികെ result;
}


സ്ഥിതം
M3Result Compile_SetLocal (IM3Compilation o, m3opcode_t i_opcode)
{
    M3Result result;

    u32 localIndex;
_   (ReadLEB_u32(&localIndex, &o->wasm, o->wasmEnd));                           m3log (compile, d_indent " (index = %u)", get_indention_string (o), localIndex);

    എങ്കിൽ (localIndex < Get_Function_അക്കം_Args_And_Locals(o->function)) {
        // Spec: value type must match local type
        എങ്കിൽ (not Is_അട്ടി_Polymorphic(o)) {
            m3type_t localType    = Get_അട്ടി_Type_From_Bottom(o, localIndex);
            m3type_t അട്ടി_മുകൾ_Type = Get_അട്ടി_മുകൾ_Type(o);
            _throwif(m3Err_typeMismatch, അട്ടി_മുകൾ_Type != c_m3Type_none and localType != c_m3Type_none and
                                           not IsSubTypeOf(അട്ടി_മുകൾ_Type, localType));
#എങ്കിൽ d_m3HasSnapshots
            എങ്കിൽ (o->localInitDepth and o->localInitDepth[localIndex] == UINT32_MAX) {
                o->localInitDepth[localIndex] = (u32)o->block.depth;
            }
#endif
        }

        u16 localSlot = Get_Slot_For_അട്ടി_Index(o, localIndex);

        u16 preserveSlot;
_       (FindReferencedLocalWithinCurrentBlock(o, &preserveSlot, localSlot));  // preserve will be different than local, if referenced

        bool folded = false;

#എങ്കിൽ d_m3FoldSetLocal
        // destination folding: when the value was produced by the immediately preceding op (nothing
        // emitted since, result on top in _r0, no copy-on-write preservation needed), retro-patch the
        // producer to a variant that writes straight to the local's slot and skip the SetSlot op
        എങ്കിൽ (o->foldPatchPC and preserveSlot == localSlot and Is_അട്ടി_മുകൾ_In_Register(o) and Get_അട്ടി_മുകൾ_Index(o) == (i16)o->fold_അട്ടി_Index) {
            IM3Operation foldOp = GetFoldOp(o->foldOpcode, o->foldForm);

            എങ്കിൽ (foldOp) {
                *(IM3Operation*)o->foldPatchPC = foldOp;     // retro-patch the producer
                EmitSlotOffset(o, localSlot);                  // append its destination
                InvalidateFold(o);
                folded = true;
            }
        }
#endif

        എങ്കിൽ (folded) {
            m3type_t type = Get_അട്ടി_മുകൾ_Type(o);
_           (എടുക്കുക(o));                              // the value now lives in the local's slot

            // A folded op writes its result through to the register as well, so a tee
            // can keep it there. Pushing the slot instead would make the next use that
            // wants a register reload the slot this op just stored to.
            എങ്കിൽ (i_opcode == c_waOp_teeLocal) {
_               (തള്ളുക_Register(o, type));
            }
        } അല്ലെങ്കിൽ {
            എങ്കിൽ (preserveSlot == localSlot) {
_               (Copy_അട്ടി_മുകൾ_To_Slot(o, localSlot))
            } അല്ലെങ്കിൽ {
_               (Preserved_Copy_മുകൾ_Slot(o, localSlot, preserveSlot))
            }

            എങ്കിൽ (i_opcode != c_waOp_teeLocal) {
_               (എടുക്കുക(o));
            }
        }
    } അല്ലെങ്കിൽ {
        _throw("local index out of bounds");
    }

    _catch: തിരികെ result;
}

സ്ഥിതം
M3Result Compile_GetLocal (IM3Compilation o, m3opcode_t i_opcode)
{
_try {

    u32 localIndex;
_   (ReadLEB_u32(&localIndex, &o->wasm, o->wasmEnd));                           m3log (compile, d_indent " (index = %u)", get_indention_string (o), localIndex);

    എങ്കിൽ (localIndex >= Get_Function_അക്കം_Args_And_Locals(o->function)) {
        _throw("local index out of bounds");
    }

    m3type_t type = Get_അട്ടി_Type_From_Bottom(o, localIndex);
    u16      slot = Get_Slot_For_അട്ടി_Index(o, localIndex);

_   (തള്ളുക(o, type, slot));

    } _catch: തിരികെ result;
}

സ്ഥിതം
M3Result Compile_GetGlobal (IM3Compilation o, M3Global* i_global)
{
    M3Result result;

    IM3Operation op = Is64BitType(i_global->type) ? op_GetGlobal_s64 : op_GetGlobal_s32;
_   (EmitOp(o, op));
    EmitPointer(o, &i_global->i64_മൂല്യം);
_   (തള്ളുക_Allocated_Slot_And_Emit_2(o, i_global->type));

    _catch: തിരികെ result;
}

സ്ഥിതം
M3Result Compile_SetGlobal (IM3Compilation o, M3Global* i_global)
{
    M3Result result = m3Err_none;

    എങ്കിൽ (i_global->isMutable) {
        // Spec: value type must match global type
        എങ്കിൽ (not Is_അട്ടി_Polymorphic(o)) {
            m3type_t അട്ടി_മുകൾ_Type = Get_അട്ടി_മുകൾ_Type(o);
            _throwif(m3Err_typeMismatch, അട്ടി_മുകൾ_Type != c_m3Type_none and
                                           not IsSubTypeOf(അട്ടി_മുകൾ_Type, i_global->type));
        }

        IM3Operation op;
        m3type_t     type = Get_അട്ടി_മുകൾ_Type(o);

        എങ്കിൽ (Is_അട്ടി_മുകൾ_In_Register(o)) {
            op = c_setGlobalOps[BaseTypeOf(type)];
        } അല്ലെങ്കിൽ {
            op = Is64BitType(type) ? op_SetGlobal_s64 : op_SetGlobal_s32;
        }

_       (EmitOp(o, op));
        EmitPointer(o, &i_global->i64_മൂല്യം);

        എങ്കിൽ (Is_അട്ടി_മുകൾ_In_Slot(o)) {
            EmitSlotOffset(o, Get_അട്ടി_മുകൾ_Slot_സംഖ്യ(o));
        }

_       (എടുക്കുക(o));
    } അല്ലെങ്കിൽ {
        _throw(m3Err_settingImmutableGlobal);
    }

    _catch: തിരികെ result;
}

സ്ഥിതം
M3Result Compile_GetSetGlobal (IM3Compilation o, m3opcode_t i_opcode)
{
    M3Result result = m3Err_none;

    u32 globalIndex;
_   (ReadLEB_u32(&globalIndex, &o->wasm, o->wasmEnd));

    എങ്കിൽ (globalIndex < o->module->അക്കം_Globals) {
        എങ്കിൽ (o->module->globals) {
            M3Global* global = &o->module->globals[globalIndex];

            // Spec: a constant expression may only read an imported immutable
            // global. The module's own globals are counted before their
            // initializer is walked, so a bare index check would let one
            // reference itself. These describe how the module declared the
            // global, so they are checked before following any link.
            _throwif(m3Err_globaIndexOutOfBounds, o->is_Init_വ്യഞ്ജകം and not global->imported);
            _throwif(m3Err_wasmMalformed, o->is_Init_വ്യഞ്ജകം and global->isMutable);

            // an import linked to another module reads and writes that module's
            // cell, not the placeholder standing in for it here
            എങ്കിൽ (global->resolved) {
                global = global->resolved;
            }

_           ((i_opcode == c_waOp_getGlobal) ? Compile_GetGlobal(o, global) : Compile_SetGlobal(o, global));
        } അല്ലെങ്കിൽ {
            _throw(ErrorCompile(m3Err_globalMemoryNotAllocated, o, "module '%s' is missing global memory", o->module->name));
        }
    } അല്ലെങ്കിൽ {
        _throw(m3Err_globaIndexOutOfBounds);
    }

    _catch: തിരികെ result;
}

സ്ഥിതം
ശൂന്യം EmitPatchingBranchPointer (IM3Compilation o, IM3CompilationScope i_scope)
{
    pc_t patch       = EmitPointer(o, i_scope->patches);                m3log (compile, "branch patch required at: %p", patch);
    i_scope->patches = patch;
}

സ്ഥിതം
M3Result EmitPatchingBranch (IM3Compilation o, IM3CompilationScope i_scope)
{
    M3Result result = m3Err_none;

_   (EmitOp(o, op_Branch));
    EmitPatchingBranchPointer(o, i_scope);

    _catch: തിരികെ result;
}

#എങ്കിൽ d_m3HasExceptionHandling

// How many try_table handler records an exit from i_from out to i_target takes
// down with it. Walking outward from the block the branch sits in, every
// try_table scope up to and including the target is left behind - the target
// too, because landing on a block's label means that block is finished.
//
// A loop label is the exception, but a loop is never a try_table, so the count
// is the same either way: the loop itself is not on the list.
സ്ഥിതം
u32 എണ്ണം_Try_Frames_To_Exit (IM3CompilationScope i_from, IM3CompilationScope i_target)
{
    u32 എണ്ണം = 0;

    വേണ്ടി (IM3CompilationScope scope = i_from; scope; scope = scope->outer) {
        എങ്കിൽ (scope->opcode == c_waOp_tryTable) {
            ++എണ്ണം;
        }

        എങ്കിൽ (scope == i_target) {
            നിർത്തുക;
        }
    }

    തിരികെ എണ്ണം;
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
സ്ഥിതം
u32 അക്കം_Try_Frames_To_എടുക്കുക (IM3CompilationScope i_from, IM3CompilationScope i_target)
{
    എങ്കിൽ (i_target->depth == 0) {
        തിരികെ 0;
    }

    // the walk only ever runs inward-to-outward; a target that is already
    // outside i_from has nothing between them
    എങ്കിൽ (not i_from or i_from->depth < i_target->depth) {
        തിരികെ 0;
    }

    തിരികെ എണ്ണം_Try_Frames_To_Exit(i_from, i_target);
}


സ്ഥിതം
M3Result Emit_എടുക്കുക_Try_Frames (IM3Compilation o, u32 i_അക്കം_Frames)
{
    M3Result result = m3Err_none;

    എങ്കിൽ (i_അക്കം_Frames) {
_       (EmitOp(o, op_എടുക്കുക_Handlers));
        EmitConstant32(o, i_അക്കം_Frames);
    }

    _catch: തിരികെ result;
}


// A return_call leaves the enclosing function before the callee runs, so no
// try_table in this function may catch what the callee throws. Neither shape of
// the instruction unwinds the native stack in time to arrange that on its own:
// op_ReturnCall tail-jumps with op_TryTable's frames still standing, and the
// fallback shape is an ordinary call sitting inside them. Either way the
// handler records have to come off first.
സ്ഥിതം
M3Result Emit_എടുക്കുക_Try_Frames_For_Return_Call (IM3Compilation o)
{
    u32 എണ്ണം = 0;

    വേണ്ടി (IM3CompilationScope scope = &o->block; scope; scope = scope->outer) {
        എങ്കിൽ (scope->opcode == c_waOp_tryTable) {
            ++എണ്ണം;
        }
    }

    തിരികെ Emit_എടുക്കുക_Try_Frames(o, എണ്ണം);
}

#അല്ലെങ്കിൽ

സ്ഥിതം
M3Result Emit_എടുക്കുക_Try_Frames (IM3Compilation o, u32 i_അക്കം_Frames)
{
    (ശൂന്യം)o;
    (ശൂന്യം)i_അക്കം_Frames;
    തിരികെ m3Err_none;
}

#endif // d_m3HasExceptionHandling

#എങ്കിൽ d_m3FuseBranch
// when the pending fold candidate is a compare whose result sits on top in _r0, return the fused
// variant that absorbs an immediately following br_if (i_isIf false) or if (i_isIf true)
സ്ഥിതം
IM3Operation GetBranchFusionOp (IM3Compilation o, bool i_isIf)
{
    എങ്കിൽ (o->foldPatchPC and o->foldForm < 4 and Is_അട്ടി_മുകൾ_In_Register(o) and Get_അട്ടി_മുകൾ_Index(o) == (i16)o->fold_അട്ടി_Index) {
        സ്ഥിരം M3FusedCmpOps* ops = GetFusedCmpOps(o->foldOpcode);

        എങ്കിൽ (ops) {
            തിരികെ i_isIf ? ops->ifOp[o->foldForm] : ops->branchIf[o->foldForm];
        }
    }

    തിരികെ ശൂന്യ;
}
#endif // d_m3FuseBranch

സ്ഥിതം വരിയിൽ
IM3Operation Op_ContinueLoop (IM3Compilation o)
{
#എങ്കിൽ d_m3_Has_അട്ടി_Switching
    എങ്കിൽ (M3_UNLIKELY(o->module->runtime and o->module->runtime->isSuspendable)) {
        തിരികെ op_ContinueLoop_Suspendable;
    }
#endif
    തിരികെ op_ContinueLoop;
}

സ്ഥിതം വരിയിൽ
IM3Operation Op_ContinueLoopIf (IM3Compilation o)
{
#എങ്കിൽ d_m3_Has_അട്ടി_Switching
    എങ്കിൽ (M3_UNLIKELY(o->module->runtime and o->module->runtime->isSuspendable)) {
        തിരികെ op_ContinueLoopIf_Suspendable;
    }
#endif
    തിരികെ op_ContinueLoopIf;
}


സ്ഥിതം
M3Result Compile_Branch (IM3Compilation o, m3opcode_t i_opcode)
{
    M3Result result;
    u32      അക്കം_Try_Frames = 0;

    u32 depth;
_   (ReadLEB_u32(&depth, &o->wasm, o->wasmEnd));

    // Spec: br_if condition must be i32
    എങ്കിൽ (i_opcode == c_waOp_branchIf and not Is_അട്ടി_Polymorphic(o)) {
        m3type_t condType = Get_അട്ടി_മുകൾ_Type(o);
        _throwif(m3Err_typeMismatch, condType != c_m3Type_none and condType != c_m3Type_i32);
    }

    IM3CompilationScope scope;
_   (GetBlockScope(o, &scope, depth));

#എങ്കിൽ d_m3HasExceptionHandling
    അക്കം_Try_Frames = അക്കം_Try_Frames_To_എടുക്കുക(&o->block, scope);
#endif

    // branch target is a loop (continue)
    എങ്കിൽ (scope->opcode == c_waOp_loop) {
        എങ്കിൽ (i_opcode == c_waOp_branchIf) {
            // a handler pop belongs on the taken path only, so it forces the
            // jump-over form even where the target takes no parameters
            എങ്കിൽ (Get_Func_Type_അക്കം_Params(scope->type) or അക്കം_Try_Frames) {
                IM3Operation op = Is_അട്ടി_മുകൾ_In_Register(o) ? op_BranchIfPrologue_r : op_BranchIfPrologue_s;

_               (EmitOp(o, op));
_               (Emit_Slot_അക്കം_Of_അട്ടി_മുകൾ_And_എടുക്കുക(o));

                pc_t* jumpTo = (pc_t*)ReservePointer(o);

_               (Emit_എടുക്കുക_Try_Frames(o, അക്കം_Try_Frames));

                എങ്കിൽ (not Is_അട്ടി_Polymorphic(o)) {
_                   (ResolveBlockResults(o, scope, /* isBranch: */ true));
                }

_               (EmitOp(o, Op_ContinueLoop(o)));
                EmitPointer(o, scope->pc);
_               (RecordBackEdge(o, scope, false));

                *jumpTo = GetPC(o);
            } അല്ലെങ്കിൽ {
                // move the condition to a register
_               (Copy_അട്ടി_മുകൾ_To_Register(o, false));
_               (എടുക്കുക_Type(o, c_m3Type_i32));

_               (EmitOp(o, Op_ContinueLoopIf(o)));
                EmitPointer(o, scope->pc);
_               (RecordBackEdge(o, scope, true));
            }

            //          dump_type_stack(o);
        } അല്ലെങ്കിൽ // is c_waOp_branch
        {
_           (Emit_എടുക്കുക_Try_Frames(o, അക്കം_Try_Frames));

            // the branch operands become the loop's parameters on the next iteration
            എങ്കിൽ (not Is_അട്ടി_Polymorphic(o)) {
_               (ResolveBlockResults(o, scope, /* isBranch: */ true));
            }

_           (EmitOp(o, Op_ContinueLoop(o)));
            EmitPointer(o, scope->pc);
_           (RecordBackEdge(o, scope, false));
            o->block.isPolymorphic = true;
        }
    } അല്ലെങ്കിൽ // forward branch
    {
        pc_t* jumpTo = ശൂന്യ;

        bool isReturn         = (scope->depth == 0);
        bool targetHasResults = Get_Func_Type_അക്കം_Results(scope->type);

        എങ്കിൽ (i_opcode == c_waOp_branchIf) {
            എങ്കിൽ (targetHasResults or isReturn or അക്കം_Try_Frames) {
                IM3Operation op = Is_അട്ടി_മുകൾ_In_Register(o) ? op_BranchIfPrologue_r : op_BranchIfPrologue_s;

_               (EmitOp(o, op));
_               (Emit_Slot_അക്കം_Of_അട്ടി_മുകൾ_And_എടുക്കുക(o)); // condition

                // this is continuation point, if the branch isn't taken
                jumpTo = (pc_t*)ReservePointer(o);
            } അല്ലെങ്കിൽ {
#എങ്കിൽ d_m3FuseBranch
                IM3Operation fuseOp = GetBranchFusionOp(o, false);

                എങ്കിൽ (fuseOp) {
                    // absorb the branch into the compare that produced the condition
                    *(IM3Operation*)o->foldPatchPC = fuseOp;
                    InvalidateFold(o);

_                   (എടുക്കുക(o));                      // the condition is consumed inside the fused op

                    EmitPatchingBranchPointer(o, scope);
                    പോകുക _catch;
                }
#endif
                IM3Operation op = Is_അട്ടി_മുകൾ_In_Register(o) ? op_BranchIf_r : op_BranchIf_s;

_               (EmitOp(o, op));
_               (Emit_Slot_അക്കം_Of_അട്ടി_മുകൾ_And_എടുക്കുക(o)); // condition

                EmitPatchingBranchPointer(o, scope);
                പോകുക _catch;
            }
        }

        എങ്കിൽ (not Is_അട്ടി_Polymorphic(o)) {
            എങ്കിൽ (isReturn) {
_               (Return_മൂല്യങ്ങൾ(o, scope, true));
_               (EmitOp(o, op_Return));
            } അല്ലെങ്കിൽ {
_               (Emit_എടുക്കുക_Try_Frames(o, അക്കം_Try_Frames));

_               (ResolveBlockResults(o, scope, true));
_               (EmitPatchingBranch(o, scope));
            }
        }

        എങ്കിൽ (jumpTo) {
            *jumpTo = GetPC(o);
        }

        എങ്കിൽ (i_opcode == c_waOp_branch) {
_           (Set_അട്ടി_Polymorphic(o));
        }
    }

    _catch: തിരികെ result;
}

സ്ഥിതം
M3Result Compile_BranchTable (IM3Compilation o, m3opcode_t i_opcode)
{
    // the page a continue-op page is standing in for; non-NULL only while that
    // page is installed in o->page, so the catch below knows to put it back
    IM3CodePage displacedPage = ശൂന്യ;
    // indexed by label depth: the continuation stub already emitted for that target
    // within this table, or NULL if this table has not reached it yet
    pc_t* targetStubs = ശൂന്യ;
_try {
    u32 target_എണ്ണം;
_   (ReadLEB_u32(&target_എണ്ണം, &o->wasm, o->wasmEnd));

    // Spec: validate that the branch index operand is i32
    എങ്കിൽ (not Is_അട്ടി_Polymorphic(o)) {
        m3type_t indexType = Get_അട്ടി_മുകൾ_Type(o);
        _throwif(m3Err_typeMismatch, indexType != c_m3Type_none and indexType != c_m3Type_i32);
    }

_   (PreserveRegisterIfOccupied(o, c_m3Type_i64));         // move branch operand to a slot
    u16 slot = Get_അട്ടി_മുകൾ_Slot_സംഖ്യ(o);
_   (എടുക്കുക(o));

    // OPTZ: according to spec: "forward branches that target a control instruction with a non-empty
    // result type consume matching operands first and push them back on the operand stack after unwinding"
    // So, this move-to-reg is only necessary if the target scopes have a type.

    u32 അക്കം_Code_വരികൾ = target_എണ്ണം + 4; // 3 => IM3Operation + slot + target_count + default_target
_   (Ensure_Code_Page_അക്കം_വരികൾ(o, അക്കം_Code_വരികൾ));

_   (EmitOp(o, op_BranchTable));
    EmitSlotOffset(o, slot);
    EmitConstant32(o, target_എണ്ണം);

    IM3CodePage continueOpPage = ശൂന്യ;

    // The label types are checked by ValidateFunction, which runs under the same
    // d_m3EnableValidation guard just before this compilation pass. Comparing each
    // target against the default here would be wrong anyway: the spec matches every
    // target against the operand stack, so in unreachable code the operands are the
    // bottom type and the targets may legitimately differ (unreached-valid.wast).

    ++target_എണ്ണം; // include default

    // Every entry of one br_table is resolved from the same compilation state - each
    // stub restores whatever it touched - so two entries naming the same label emit
    // byte-identical epilogues. Emit one and point both table entries at it. A label
    // deeper than the current block is rejected by GetBlockScope below, which is what
    // bounds this array.
    targetStubs = m3_AllocArray(pc_t, (size_t)o->block.depth + 1);
    _throwifnull(targetStubs);

    വേണ്ടി (u32 i = 0; i < target_എണ്ണം; ++i) {
        u32 target;
_       (ReadLEB_u32(&target, &o->wasm, o->wasmEnd));

        IM3CompilationScope scope;
_       (GetBlockScope(o, &scope, target));

        എങ്കിൽ (targetStubs[target]) {
            EmitPointer(o, targetStubs[target]);
            തുടരുക;
        }

        u32 അക്കം_Try_Frames = 0;
#എങ്കിൽ d_m3HasExceptionHandling
        അക്കം_Try_Frames = അക്കം_Try_Frames_To_എടുക്കുക(&o->block, scope);
#endif

        // A forward branch to a block that carries no results and leaves no try
        // frames behind has nothing to do on the way out but arrive. There is no
        // epilogue to hold, so the table entry becomes the patch site itself and the
        // block's address is written into it when the block ends - no stub, and no
        // code page to put one on. In unreachable code the same is true whatever the
        // block's type, since nothing is emitted for that case either way.
        //
        // Each such entry is patched on its own, so none is recorded in targetStubs:
        // there is no shared stub for a later entry naming this label to point at.
        എങ്കിൽ (scope->opcode != c_waOp_loop and scope->depth > 0 and അക്കം_Try_Frames == 0 and
            (Is_അട്ടി_Polymorphic(o) or Get_Func_Type_അക്കം_Results(scope->type) == 0)) {
            EmitPatchingBranchPointer(o, scope);
            തുടരുക;
        }

_       (AcquireCompilationCodePage(o, &continueOpPage));

        pc_t startPC  = GetPagePC(continueOpPage);
        displacedPage = o->page;
        o->page       = continueOpPage;

        എങ്കിൽ (scope->opcode == c_waOp_loop) {
_           (Emit_എടുക്കുക_Try_Frames(o, അക്കം_Try_Frames));

            എങ്കിൽ (not Is_അട്ടി_Polymorphic(o)) {
_               (ResolveBlockResults(o, scope, true));
            }

_           (EmitOp(o, Op_ContinueLoop(o)));
            EmitPointer(o, scope->pc);
_           (RecordBackEdge(o, scope, false));
        } അല്ലെങ്കിൽ {
            എങ്കിൽ (not Is_അട്ടി_Polymorphic(o)) {
                എങ്കിൽ (scope->depth == 0) {
_                   (Return_മൂല്യങ്ങൾ(o, scope, true));
_                   (EmitOp(o, op_Return));
                } അല്ലെങ്കിൽ {
_                   (Emit_എടുക്കുക_Try_Frames(o, അക്കം_Try_Frames));

_                   (ResolveBlockResults(o, scope, true));

_                   (EmitPatchingBranch(o, scope));
                }
            }
        }

        IM3CodePage returnTo = displacedPage;
        displacedPage        = ശൂന്യ;

        ReleaseCompilationCodePage(o);
        o->page = returnTo;

        EmitPointer(o, startPC);
        targetStubs[target] = startPC;
    }

_   (Set_അട്ടി_Polymorphic(o));
} _catch:

    // thrown out of the loop above with a continue-op page installed: release it
    // and restore the one it displaced, which the caller's catch then releases.
    // Otherwise the displaced page is on no list and simply leaks.
    എങ്കിൽ (displacedPage) {
        ReleaseCompilationCodePage(o);
        o->page = displacedPage;
    }

    m3_Free(targetStubs);

    തിരികെ result;
}

സ്ഥിതം
M3Result CompileCallArgsAndReturn (IM3Compilation o, u16* o_അട്ടി_Offset, IM3FuncType i_type, bool i_isIndirect)
{
_try {

    u16 മുകൾ_Slot = GetMaxUsedSlotPlusOne(o);

    // force use of at least one stack slot; this is to help ensure
    // the m3 stack overflows (and traps) before the native stack can overflow.
    // e.g. see Wasm spec test 'runaway' in call.wast
    മുകൾ_Slot = M3_MAX(1, മുകൾ_Slot);

    // stack frame is 64-bit aligned
    AlignSlotToType(&മുകൾ_Slot, c_m3Type_i64);

    *o_അട്ടി_Offset = മുകൾ_Slot;

    // wait to pop this here so that topSlot search is correct
    എങ്കിൽ (i_isIndirect) {
_       (എടുക്കുക(o));
    }

    u16 അക്കം_Args = Get_Func_Type_അക്കം_Params(i_type);
    u16 അക്കം_Rets = Get_Func_Type_അക്കം_Results(i_type);

    u16 arg_മുകൾ = മുകൾ_Slot + (അക്കം_Args + അക്കം_Rets) * c_io_Slot_എണ്ണം;

    TouchSlot(o, arg_മുകൾ - 1);

    ആകുമ്പോൾ (അക്കം_Args--) {
_       (Copy_അട്ടി_മുകൾ_To_Slot(o, arg_മുകൾ -= c_io_Slot_എണ്ണം));
_       (എടുക്കുക(o));
    }

    u16 i = 0;
    ആകുമ്പോൾ (അക്കം_Rets--) {
        m3type_t type = GetFuncTypeResultType(i_type, i++);

_       (തള്ളുക(o, type, മുകൾ_Slot));
_       (MarkSlotsAllocatedByType(o, മുകൾ_Slot, type));

        മുകൾ_Slot += c_io_Slot_എണ്ണം;
    }

    } _catch: തിരികെ result;
}

// A tail call hands the current frame over to the callee instead of stacking a new one.
// The callee's results have to be the enclosing function's results, so both use the same
// return slots and only the arguments move: they're staged above the caller's stack here
// (the argument expressions can still be reading the args/locals they're about to land on)
// and slid down onto the frame base at runtime, by op_ReturnCall[Indirect].
സ്ഥിതം
M3Result CompileTailCallArgs (IM3Compilation o, u16* o_അട്ടി_Offset, u16* o_അക്കം_Arg_Slots, IM3FuncType i_type, bool i_isIndirect)
{
_try {
    IM3FuncType funcType = o->function->funcType;

    u16 അക്കം_Rets = Get_Func_Type_അക്കം_Results(i_type);
    _throwif(m3Err_typeMismatch, അക്കം_Rets != Get_Func_Type_അക്കം_Results(funcType));

    വേണ്ടി (u16 i = 0; i < അക്കം_Rets; ++i) {
        _throwif(m3Err_typeMismatch, GetFuncTypeResultType(i_type, i) != GetFuncTypeResultType(funcType, i));
    }

    u16 മുകൾ_Slot = GetMaxUsedSlotPlusOne(o);

    മുകൾ_Slot = M3_MAX(1, മുകൾ_Slot);

    // stack frame is 64-bit aligned
    AlignSlotToType(&മുകൾ_Slot, c_m3Type_i64);

    *o_അട്ടി_Offset = മുകൾ_Slot;

    // wait to pop this here so that topSlot search is correct
    എങ്കിൽ (i_isIndirect) {
_       (എടുക്കുക(o));
    }

    u16 അക്കം_Args     = Get_Func_Type_അക്കം_Params(i_type);
    u16 അക്കം_Arg_Slots = അക്കം_Args * c_io_Slot_എണ്ണം;

    *o_അക്കം_Arg_Slots = അക്കം_Arg_Slots;

    u16 arg_മുകൾ = മുകൾ_Slot + അക്കം_Arg_Slots;
    _throwif(m3_Err_function_അട്ടി_Overflow, arg_മുകൾ > d_m3MaxFunctionSlots);

    // the staging area is written by the caller, so op_Entry has to account for it
    TouchSlot(o, arg_മുകൾ - 1);

    ആകുമ്പോൾ (അക്കം_Args--) {
_       (Copy_അട്ടി_മുകൾ_To_Slot(o, arg_മുകൾ -= c_io_Slot_എണ്ണം));
_       (എടുക്കുക(o));
    }

    } _catch: തിരികെ result;
}


#എങ്കിൽ d_m3HasTypedRefs

// call_ref $t: the callee is the reference on top of the stack, so the operand
// order is the same as call_indirect's, minus the table.
സ്ഥിതം
M3Result Compile_CallRef (IM3Compilation o, m3opcode_t i_opcode)
{
_try {
    u32 typeIndex;
_   (ReadLEB_u32(&typeIndex, &o->wasm, o->wasmEnd));

    _throwif("function call type index out of range", typeIndex >= o->module->അക്കം_Func_Types);

    IM3FuncType type = o->module->funcTypes[typeIndex];

    എങ്കിൽ (not Is_അട്ടി_Polymorphic(o)) {
        // whatever shape the reference has, it must be a function of this type
        m3type_t refType = Get_അട്ടി_മുകൾ_Type(o);
        _throwif(m3Err_typeMismatch, not IsSubTypeOf(refType, RefTypeOfFuncType(type, false)));
    }

    എങ്കിൽ (Is_അട്ടി_മുകൾ_In_Register(o)) {
_       (PreserveRegisterIfOccupied(o, c_m3Type_funcref));
    }

    u16 functionSlot = Get_അട്ടി_മുകൾ_Slot_സംഖ്യ(o);

    bool isReturnCall = (i_opcode == c_waOp_returnCallRef);
    bool useTailCall  = isReturnCall and d_m3CanTailCall;

    u16 exec_മുകൾ, അക്കം_Arg_Slots = 0;

    എങ്കിൽ (useTailCall) {
_       (CompileTailCallArgs(o, &exec_മുകൾ, &അക്കം_Arg_Slots, type, true));
    } അല്ലെങ്കിൽ {
_       (CompileCallArgsAndReturn(o, &exec_മുകൾ, type, true));
    }

#  എങ്കിൽ d_m3HasExceptionHandling
    എങ്കിൽ (isReturnCall) {
_       (Emit_എടുക്കുക_Try_Frames_For_Return_Call(o));
    }
#  endif

_   (EmitOp(o, useTailCall ? op_ReturnCallRef : op_CallRef));
    EmitSlotOffset(o, functionSlot);
    EmitPointer(o, type);
    EmitSlotOffset(o, exec_മുകൾ);

    എങ്കിൽ (not useTailCall) {
_       (RecordCall(o, Get_Func_Type_അക്കം_Results(type), exec_മുകൾ, isReturnCall));
    }

    എങ്കിൽ (useTailCall) {
        EmitSlotOffset(o, o->function->അക്കം_Ret_Slots);
        EmitConstant32(o, അക്കം_Arg_Slots);

_       (Set_അട്ടി_Polymorphic(o));
    } അല്ലെങ്കിൽ എങ്കിൽ (isReturnCall) {
_       (Compile_Return(o, i_opcode));
    }

} _catch:
    തിരികെ result;
}


// ref.as_non_null only sharpens the type; the value is unchanged, so the stack
// entry is re-pushed with the null stripped out of its type.
സ്ഥിതം
M3Result Compile_Ref_AsNonNull (IM3Compilation o, m3opcode_t i_opcode)
{
    M3Result result = m3Err_none;

    എങ്കിൽ (not Is_അട്ടി_Polymorphic(o)) {
        m3type_t type = Get_അട്ടി_മുകൾ_Type(o);
        _throwif(m3Err_typeMismatch, not IsRefType(type));

        // op_RefAsNonNull only knows how to read its operand out of a slot; a
        // value still sitting in the register has to be spilled first, the way
        // every other single-op reader of the stack top does
        എങ്കിൽ (Is_അട്ടി_മുകൾ_In_Register(o)) {
_           (PreserveRegisterIfOccupied(o, type));
        }

        u16 slot = Get_അട്ടി_മുകൾ_Slot_സംഖ്യ(o);

_       (EmitOp(o, op_RefAsNonNull));
        EmitSlotOffset(o, slot);

        m3type_t nonNull = IsSpelledRefType(type)
                             ? (m3type_t)(type | d_m3Type_refNonNull)
                             : (m3type_t)(d_m3Type_ref | d_m3Type_refNonNull | d_m3Type_heapAbstract |
                                          ((BaseTypeOf(type) == c_m3Type_externref) ? d_m3Type_refExtern : 0));

        // the slot keeps the one reference it already had; a Pop()/Push() of
        // the same slot would drop that reference's count without anything
        // to put it back, so the type above it is corrected in place instead
        o->type_അട്ടി[Get_അട്ടി_മുകൾ_Index(o)] = nonNull;
    }

    _catch: തിരികെ result;
}

#endif // d_m3HasTypedRefs


// How wide a table operand is read at comes from the table it names, so a slot
// holding a narrower type would be read past what was allocated for it. The
// validator refuses that already; this keeps a build without one from reaching
// the interpreter with it. c_m3Type_none means the operand is not on the
// dynamic stack to inspect, which is not this check's business.
സ്ഥിതം
M3Result CheckOperandType (IM3Compilation o, u16 i_depth_From_മുകൾ, m3type_t i_type)
{
    m3type_t actual;

    എങ്കിൽ (Is_അട്ടി_Polymorphic(o)) {
        തിരികെ m3Err_none;
    }

    actual = Get_അട്ടി_Type_From_മുകൾ(o, i_depth_From_മുകൾ);

    തിരികെ (actual == c_m3Type_none or BaseTypeOf(actual) == i_type) ? m3Err_none
                                                                     : m3Err_typeMismatch;
}


// Swaps the _mem register onto a given memory. Only ever emitted in pairs --
// see op_SetMemory.
സ്ഥിതം
M3Result EmitSetMemoryPtr (IM3Compilation o, IM3Memory i_memory)
{
    M3Result result = m3Err_none;

_   (EmitOp(o, op_SetMemory));
    EmitPointer(o, i_memory);

    _catch: തിരികെ result;
}


// ...onto one of the compiling module's own memories, by index. Only reached
// with a non-zero index under multi-memory, but the index has already been
// bounds-checked by ReadMemoryIndex either way.
സ്ഥിതം വരിയിൽ
M3Result EmitSetMemory (IM3Compilation o, u32 i_memoryIdx)
{
#എങ്കിൽ d_m3HasMultiMemory
    തിരികെ EmitSetMemoryPtr(o, o->module->memories[i_memoryIdx]);
#അല്ലെങ്കിൽ
    (ശൂന്യം)o;
    (ശൂന്യം)i_memoryIdx;
    തിരികെ m3Err_unknownMemory;
#endif
}

സ്ഥിതം
M3Result Compile_Call (IM3Compilation o, m3opcode_t i_opcode)
{
_try {
    u32 functionIndex;
_   (ReadLEB_u32(&functionIndex, &o->wasm, o->wasmEnd));

    IM3Function function = Module_GetFunction(o->module, functionIndex);

    എങ്കിൽ (function) {                                                     m3log (compile, d_indent " (func= [%d] '%s'; args= %d)",
                                                                                get_indention_string (o), functionIndex, m3_GetFunctionName (function), function->funcType->അക്കം_Args);
        // an import linked to another module's export runs that module's body
        IM3Function target = Function_Implementation(function);

        എങ്കിൽ (target->module) {
            bool isReturnCall = (i_opcode == c_waOp_returnCall);

            // The callee's body runs against its own module's memory, so a call
            // that crosses modules is bracketed by a pair of op_SetMemory. A
            // tail call never returns to run the second one, so those compile
            // as a plain call followed by a return instead.
            bool crossModule = (target->module != o->module);
            bool useTailCall = isReturnCall and d_m3CanTailCall and not crossModule;

            u16 slot_മുകൾ, അക്കം_Arg_Slots = 0;

            എങ്കിൽ (useTailCall) {
_               (CompileTailCallArgs(o, &slot_മുകൾ, &അക്കം_Arg_Slots, target->funcType, false));
            } അല്ലെങ്കിൽ {
_               (CompileCallArgsAndReturn(o, &slot_മുകൾ, target->funcType, false));
            }

            IM3Operation op;
            സ്ഥിരം ശൂന്യം*  operand;

            എങ്കിൽ (target->compiled) {
                op      = useTailCall ? op_ReturnCall : op_Call;
                operand = target->compiled;
            } അല്ലെങ്കിൽ {
                op      = useTailCall ? op_CompileReturnCall : op_Compile;
                operand = target;
            }

#എങ്കിൽ d_m3HasExceptionHandling
            എങ്കിൽ (isReturnCall) {
_               (Emit_എടുക്കുക_Try_Frames_For_Return_Call(o));
            }
#endif

            എങ്കിൽ (crossModule) {
_               (EmitSetMemoryPtr(o, Module_Memory0(target->module)));
            }

_           (EmitOp(o, op));
            EmitPointer(o, operand);
            EmitSlotOffset(o, slot_മുകൾ);

            എങ്കിൽ (not useTailCall) {
                // a suspend inside the callee leaves this frame waiting here, its
                // results not yet written
_               (RecordCall(o, Get_Func_Type_അക്കം_Results(target->funcType), slot_മുകൾ, isReturnCall));
            }

            എങ്കിൽ (useTailCall) {
                EmitSlotOffset(o, o->function->അക്കം_Ret_Slots);
                EmitConstant32(o, അക്കം_Arg_Slots);

_               (Set_അട്ടി_Polymorphic(o));
            } അല്ലെങ്കിൽ {
                എങ്കിൽ (crossModule) {
_                   (EmitSetMemoryPtr(o, Module_Memory0(o->module)));
                }

                എങ്കിൽ (isReturnCall) {
_                   (Compile_Return(o, i_opcode));
                }
            }
        } അല്ലെങ്കിൽ {
            _throw(ErrorCompile(m3Err_functionImportMissing, o, "'%s.%s'", GetFunctionImportModuleName(function), m3_GetFunctionName(function)));
        }
    } അല്ലെങ്കിൽ {
        _throw(ErrorCompile(m3Err_functionLookupFailed, o, "index: %d", functionIndex));
    }

    } _catch: തിരികെ result;
}

സ്ഥിതം
M3Result Compile_CallIndirect (IM3Compilation o, m3opcode_t i_opcode)
{
_try {
    u32 typeIndex;
_   (ReadLEB_u32(&typeIndex, &o->wasm, o->wasmEnd));

    u32 tableIndex;
_   (ReadLEB_u32(&tableIndex, &o->wasm, o->wasmEnd));

    _throwif("function call type index out of range", typeIndex >= o->module->അക്കം_Func_Types);
    _throwif("table index out of range", tableIndex >= o->module->അക്കം_Tables);
    _throwif(m3Err_typeMismatch, BaseTypeOf(o->module->tables[tableIndex]->type) != c_m3Type_funcref);

_   (CheckOperandType(o, 0, Table_AddrType(o->module->tables[tableIndex])));

    എങ്കിൽ (Is_അട്ടി_മുകൾ_In_Register(o)) {
_       (PreserveRegisterIfOccupied(o, c_m3Type_i32));
    }

    u16 tableIndexSlot = Get_അട്ടി_മുകൾ_Slot_സംഖ്യ(o);

    bool isReturnCall = (i_opcode == c_waOp_returnCallIndirect);
    bool useTailCall  = isReturnCall and d_m3CanTailCall;

    u16         exec_മുകൾ, അക്കം_Arg_Slots = 0;
    IM3FuncType type = o->module->funcTypes[typeIndex];

    എങ്കിൽ (useTailCall) {
_       (CompileTailCallArgs(o, &exec_മുകൾ, &അക്കം_Arg_Slots, type, true));
    } അല്ലെങ്കിൽ {
_       (CompileCallArgsAndReturn(o, &exec_മുകൾ, type, true));
    }

#എങ്കിൽ d_m3HasExceptionHandling
    എങ്കിൽ (isReturnCall) {
_       (Emit_എടുക്കുക_Try_Frames_For_Return_Call(o));
    }
#endif

    // the table comes first: how wide its indexes are is what says how much of
    // the slot below holds one
_   (EmitOp(o, useTailCall ? op_ReturnCallIndirect : op_CallIndirect));
    EmitPointer(o, o->module->tables[tableIndex]);
    EmitSlotOffset(o, tableIndexSlot);
    EmitPointer(o, type);
    EmitSlotOffset(o, exec_മുകൾ);

    എങ്കിൽ (not useTailCall) {
_       (RecordCall(o, Get_Func_Type_അക്കം_Results(type), exec_മുകൾ, isReturnCall));
    }

    എങ്കിൽ (useTailCall) {
        EmitSlotOffset(o, o->function->അക്കം_Ret_Slots);
        EmitConstant32(o, അക്കം_Arg_Slots);

_       (Set_അട്ടി_Polymorphic(o));
    } അല്ലെങ്കിൽ എങ്കിൽ (isReturnCall) {
_       (Compile_Return(o, i_opcode));
    }

} _catch:
    തിരികെ result;
}

// Reads the memory index a memory instruction names, and checks it against the
// module's index space. Without multi-memory the immediate is a reserved byte
// that has to be zero, which is the same check.
സ്ഥിതം
M3Result ReadMemoryIndex (IM3Compilation o, u32* o_memoryIdx)
{
    M3Result result;

_   (ReadLEB_u32(o_memoryIdx, &o->wasm, o->wasmEnd));

    _throwif(m3Err_unknownMemory, *o_memoryIdx >= o->module->അക്കം_Memories);

    _catch: തിരികെ result;
}

സ്ഥിതം
M3Result Compile_Memory_Size (IM3Compilation o, m3opcode_t i_opcode)
{
    M3Result result;

    u32 memoryIdx;

    // A page count is given in the memory's own address type. Declared up here
    // so the throws below don't jump over its initialization.
    m3type_t addrType = c_m3Type_i32;

_   (ReadMemoryIndex(o, &memoryIdx));

    addrType = Memory_AddrType(o->module->memories[memoryIdx]);

_   (PreserveRegisterIfOccupied(o, addrType));

    എങ്കിൽ (memoryIdx) {
_       (EmitSetMemory(o, memoryIdx));
    }

    // op_MemSize writes the whole register either way; the slot it is pushed
    // to is what decides how much of it the module gets to see
_   (EmitOp(o, op_MemSize));

    എങ്കിൽ (memoryIdx) {
_       (EmitSetMemory(o, 0));
    }

_   (തള്ളുക_Register(o, addrType));

    _catch: തിരികെ result;
}

സ്ഥിതം
M3Result Compile_Memory_Grow (IM3Compilation o, m3opcode_t i_opcode)
{
    M3Result result;

    u32 memoryIdx;

    // see Compile_Memory_Size
    m3type_t addrType = c_m3Type_i32;

_   (ReadMemoryIndex(o, &memoryIdx));

    addrType = Memory_AddrType(o->module->memories[memoryIdx]);

_   (Copy_അട്ടി_മുകൾ_To_Register(o, false));
_   (എടുക്കുക_Type(o, addrType));

    എങ്കിൽ (memoryIdx) {
_       (EmitSetMemory(o, memoryIdx));
    }

#എങ്കിൽ d_m3HasMemory64
_   (EmitOp(o, (addrType == c_m3Type_i64) ? op_MemGrow64 : op_MemGrow));
#അല്ലെങ്കിൽ
_   (EmitOp(o, op_MemGrow));
#endif

    എങ്കിൽ (memoryIdx) {
_       (EmitSetMemory(o, 0));
    }

_   (തള്ളുക_Register(o, addrType));

    _catch: തിരികെ result;
}

സ്ഥിതം
M3Result Compile_Memory_CopyFill (IM3Compilation o, m3opcode_t i_opcode)
{
    M3Result result = m3Err_none;

    // memory.copy names the destination first and then the source; memory.fill
    // names only the one it writes.
    u32  sourceMemoryIdx = 0, targetMemoryIdx = 0;
    bool isCopy = (i_opcode == c_waOp_memoryCopy);

    // Each address is typed by the memory it belongs to. The length follows
    // the narrower of the two, so it is 64-bit only when both are - which for
    // memory.fill, naming one memory twice over, means whenever that one is.
    // Declared up here so the throws below don't jump over them.
    m3type_t targetType = c_m3Type_i32;
    m3type_t sourceType = c_m3Type_i32;
    m3type_t lengthType = c_m3Type_i32;

    _throwif(m3Err_wasmMalformed, o->module->അക്കം_Memories == 0);

_   (ReadMemoryIndex(o, &targetMemoryIdx));

    എങ്കിൽ (isCopy) {
_       (ReadMemoryIndex(o, &sourceMemoryIdx));
    }

    targetType = Memory_AddrType(o->module->memories[targetMemoryIdx]);
    sourceType = isCopy ? Memory_AddrType(o->module->memories[sourceMemoryIdx])
                        : targetType;
    lengthType = (targetType == c_m3Type_i64 and sourceType == c_m3Type_i64)
                   ? c_m3Type_i64
                   : c_m3Type_i32;

_   (Copy_അട്ടി_മുകൾ_To_Register(o, false));

#എങ്കിൽ d_m3HasMultiMemory
    എങ്കിൽ (isCopy and sourceMemoryIdx != targetMemoryIdx) {
        // two memories at once: _mem can only name one, so the op takes both
_       (EmitOp(o, op_MemCopy_x));
        EmitPointer(o, o->module->memories[targetMemoryIdx]);
        EmitPointer(o, o->module->memories[sourceMemoryIdx]);

        // ...and, since they need not be addressed alike, how wide to read
        // each of the operands: bit 0 the destination, bit 1 the source
        EmitConstant32(o, ((targetType == c_m3Type_i64) ? 0x1u : 0u) |
                            ((sourceType == c_m3Type_i64) ? 0x2u : 0u));
    } അല്ലെങ്കിൽ
#endif
    {
        IM3Operation op = isCopy ? op_MemCopy : op_MemFill;

#എങ്കിൽ d_m3HasMemory64
        എങ്കിൽ (targetType == c_m3Type_i64) {
            op = isCopy ? op_MemCopy64 : op_MemFill64;
        }
#endif

        എങ്കിൽ (targetMemoryIdx) {
_           (EmitSetMemory(o, targetMemoryIdx));
        }

_       (EmitOp(o, op));
    }

    // the length is in the register; the other two are named by slot, at
    // whatever width their stack entries were pushed with
_   (എടുക്കുക_Type(o, lengthType));
_   (Emit_Slot_അക്കം_Of_അട്ടി_മുകൾ_And_എടുക്കുക(o));      // the source, or the byte to fill with
_   (Emit_Slot_അക്കം_Of_അട്ടി_മുകൾ_And_എടുക്കുക(o));      // the destination

    എങ്കിൽ (targetMemoryIdx and not (isCopy and sourceMemoryIdx != targetMemoryIdx)) {
_       (EmitSetMemory(o, 0));
    }

    _catch: തിരികെ result;
}


// memory.init and data.drop address a data segment by index. Both are only valid
// when a data count section declared the segments up front.
സ്ഥിതം
M3Result ReadDataSegment (IM3Compilation o, M3DataSegment** o_segment)
{
    M3Result result = m3Err_none;

    u32 index;
_   (ReadLEB_u32(&index, &o->wasm, o->wasmEnd));

    _throwif("data count section required", not o->module->has_Data_എണ്ണം);
    _throwif(m3Err_wasmMalformed, index >= o->module->അക്കം_Data_Segments);

    *o_segment = &o->module->dataSegments[index];

    _catch: തിരികെ result;
}


M3Result Compile_Memory_Init (IM3Compilation o, m3opcode_t i_opcode)
{
    M3Result result = m3Err_none;

    M3DataSegment* segment = ശൂന്യ;
    u32            memoryIdx;

    _throwif(m3Err_wasmMalformed, o->module->അക്കം_Memories == 0);

_   (ReadDataSegment(o, &segment));
_   (ReadMemoryIndex(o, &memoryIdx));

_   (Copy_അട്ടി_മുകൾ_To_Register(o, false));

    എങ്കിൽ (memoryIdx) {
_       (EmitSetMemory(o, memoryIdx));
    }

#എങ്കിൽ d_m3HasMemory64
    // only the destination follows the memory's address type: a data segment
    // is indexed as an i32 however the memory it is copied into is addressed
_   (EmitOp(o, o->module->memories[memoryIdx]->isMemory64 ? op_MemInit64 : op_MemInit));
#അല്ലെങ്കിൽ
_   (EmitOp(o, op_MemInit));
#endif
    EmitPointer(o, segment);
_   (എടുക്കുക_Type(o, c_m3Type_i32));
_   (Emit_Slot_അക്കം_Of_അട്ടി_മുകൾ_And_എടുക്കുക(o));
_   (Emit_Slot_അക്കം_Of_അട്ടി_മുകൾ_And_എടുക്കുക(o));

    എങ്കിൽ (memoryIdx) {
_       (EmitSetMemory(o, 0));
    }

    _catch: തിരികെ result;
}


#എങ്കിൽ d_m3HasRefTypes

സ്ഥിതം
M3Result Compile_Select (IM3Compilation o, m3opcode_t i_opcode);

// select with an explicit result type vector. The types only matter to the
// validator; the operands are laid out exactly as for the untyped select.
M3Result Compile_Select_Typed (IM3Compilation o, m3opcode_t i_opcode)
{
    M3Result result = m3Err_none;

    u32 അക്കം_Types;
_   (ReadLEB_u32(&അക്കം_Types, &o->wasm, o->wasmEnd));
    _throwif(m3Err_wasmMalformed, അക്കം_Types != 1);

    i8 waType;
    u8 type;
_   (ReadLEB_i7(&waType, &o->wasm, o->wasmEnd));
_   (NormalizeType(&type, waType));

_   (Compile_Select(o, i_opcode));

    _catch: തിരികെ result;
}


// A reference is one pointer-sized word and null is 0, so null-testing it is
// just an integer eqz of the matching width.
#  എങ്കിൽ M3_SIZEOF_PTR == 8
#    define d_m3RefIsNull_r   op_i64_EqualToZero_r
#    define d_m3RefIsNull_s   op_i64_EqualToZero_s
#  അല്ലെങ്കിൽ
#    define d_m3RefIsNull_r   op_i32_EqualToZero_r
#    define d_m3RefIsNull_s   op_i32_EqualToZero_s
#  endif

സ്ഥിതം
M3Result ReadRefType (IM3Compilation o, m3type_t* o_type)
{
    M3Result result = m3Err_none;

#  എങ്കിൽ d_m3HasTypedRefs
    // ref.null names a heap type: func, extern, or a function type index
    m3type_t heapBits;
_   (ParseHeapType(o->module, &heapBits, &o->wasm, o->wasmEnd));

    *o_type = d_m3Type_ref | heapBits;
#  അല്ലെങ്കിൽ
    i8 waType;
    u8 plainType;
_   (ReadLEB_i7(&waType, &o->wasm, o->wasmEnd));
_   (NormalizeType(&plainType, waType));
    *o_type = plainType;
    _throwif(m3Err_wasmMalformed, not IsRefType(*o_type));
#  endif

    _catch: തിരികെ result;
}


M3Result Compile_Ref_Null (IM3Compilation o, m3opcode_t i_opcode)
{
    M3Result result = m3Err_none;

    m3type_t type;
_   (ReadRefType(o, &type));
_   (തള്ളുക_Const(o, 0, type));

    _catch: തിരികെ result;
}


M3Result Compile_Ref_IsNull (IM3Compilation o, m3opcode_t i_opcode)
{
    M3Result result = m3Err_none;

    IM3Operation op;

    എങ്കിൽ (not Is_അട്ടി_Polymorphic(o)) {
        _throwif(m3Err_typeMismatch, not IsRefType(Get_അട്ടി_മുകൾ_Type(o)));
    }

    എങ്കിൽ (Is_അട്ടി_മുകൾ_In_Register(o)) {
        op = d_m3RefIsNull_r;
    } അല്ലെങ്കിൽ {
_       (PreserveRegisterIfOccupied(o, c_m3Type_i32));
        op = d_m3RefIsNull_s;
    }

_   (EmitOp(o, op));
_   (Emit_Slot_അക്കം_Of_അട്ടി_മുകൾ_And_എടുക്കുക(o));
_   (തള്ളുക_Register(o, c_m3Type_i32));

    _catch: തിരികെ result;
}


M3Result Compile_Ref_Func (IM3Compilation o, m3opcode_t i_opcode)
{
    M3Result result = m3Err_none;

    u32 funcIndex;

    // declared before the throws below, which jump past this point to _catch
    m3type_t    refType   = c_m3Type_funcref;
    IM3Function reference = ശൂന്യ;

_   (ReadLEB_u32(&funcIndex, &o->wasm, o->wasmEnd));
    _throwif("function index out of range", funcIndex >= o->module->അക്കം_Functions);

    // Inside a constant expression ref.func is itself a declaration; inside a
    // function body the function must already have been declared elsewhere.
    എങ്കിൽ (o->function) {
        _throwif("undeclared function reference", not Module_IsFunctionDeclared(o->module, funcIndex));
    } അല്ലെങ്കിൽ {
_       (Module_DeclareFunction(o->module, funcIndex));
    }

#  എങ്കിൽ d_m3HasTypedRefs
    refType = RefTypeOfFuncType(o->module->functions[funcIndex].funcType, true);
#  endif

    // A reference denotes the function that actually runs, so a ref.func naming
    // an import linked to another module is the same reference that module's
    // own ref.func would produce - and carries the defining module with it.
    reference = Function_Implementation(&o->module->functions[funcIndex]);

_   (തള്ളുക_Const(o, (u64)(uintptr_t)reference, refType));

    _catch: തിരികെ result;
}


// table.get/set/size/grow/fill all name a table by index; the table struct goes
// into the codestream so the operation doesn't have to walk the module.
സ്ഥിതം
M3Result ReadTable (IM3Compilation o, M3Table** o_table)
{
    M3Result result = m3Err_none;

    u32 index;
_   (ReadLEB_u32(&index, &o->wasm, o->wasmEnd));
    _throwif("table index out of range", index >= o->module->അക്കം_Tables);

    *o_table = o->module->tables[index];

    _catch: തിരികെ result;
}


// The table operations take every operand from a slot, so spill the integer
// register first: after this each EmitSlotNumOfStackTopAndPop () emits a real
// slot offset, and the register is free for the result. Operands are emitted
// from the top of the stack downwards, which is the order the ops read them.
സ്ഥിതം
M3Result Compile_Table_Op (IM3Compilation o, IM3Operation i_op, M3Table* i_table, u32 i_അക്കം_Operands, m3type_t i_retType)
{
    M3Result result = m3Err_none;

_   (PreserveRegisterIfOccupied(o, c_m3Type_i64));

_   (EmitOp(o, i_op));
    EmitPointer(o, i_table);

    വേണ്ടി (u32 i = 0; i < i_അക്കം_Operands; ++i) {
_       (Emit_Slot_അക്കം_Of_അട്ടി_മുകൾ_And_എടുക്കുക(o));
    }

    എങ്കിൽ (i_retType != c_m3Type_none) {
_       (തള്ളുക_Register(o, i_retType));
    }

    _catch: തിരികെ result;
}


M3Result Compile_Table_GetSet (IM3Compilation o, m3opcode_t i_opcode)
{
    M3Result result = m3Err_none;

    M3Table* table;
_   (ReadTable(o, &table));

    എങ്കിൽ (i_opcode == c_waOp_tableGet) {
_       (CheckOperandType(o, 0, Table_AddrType(table)));
_       (Compile_Table_Op(o, op_TableGet, table, 1, table->type));
    } അല്ലെങ്കിൽ {
_       (CheckOperandType(o, 1, Table_AddrType(table)));
_       (Compile_Table_Op(o, op_TableSet, table, 2, c_m3Type_none));
    }

    _catch: തിരികെ result;
}


M3Result Compile_Table_Init (IM3Compilation o, m3opcode_t i_opcode)
{
    M3Result result = m3Err_none;

    u32      elemIndex;
    M3Table* table;

_   (ReadLEB_u32(&elemIndex, &o->wasm, o->wasmEnd));
    _throwif("element segment index out of range", elemIndex >= o->module->അക്കം_Element_Segments);
_   (ReadTable(o, &table));

_   (CheckOperandType(o, 2, Table_AddrType(table)));

_   (PreserveRegisterIfOccupied(o, c_m3Type_i64));
_   (EmitOp(o, op_TableInit));
    EmitPointer(o, table);
    EmitPointer(o, &o->module->elementSegments[elemIndex]);

    വേണ്ടി (u32 i = 0; i < 3; ++i) {
_       (Emit_Slot_അക്കം_Of_അട്ടി_മുകൾ_And_എടുക്കുക(o));
    }

    _catch: തിരികെ result;
}


M3Result Compile_Elem_Drop (IM3Compilation o, m3opcode_t i_opcode)
{
    M3Result result = m3Err_none;

    u32 elemIndex;
_   (ReadLEB_u32(&elemIndex, &o->wasm, o->wasmEnd));
    _throwif("element segment index out of range", elemIndex >= o->module->അക്കം_Element_Segments);

_   (EmitOp(o, op_ElemDrop));
    EmitPointer(o, &o->module->elementSegments[elemIndex]);

    _catch: തിരികെ result;
}


M3Result Compile_Table_Copy (IM3Compilation o, m3opcode_t i_opcode)
{
    M3Result result = m3Err_none;

    M3Table* dst;
    M3Table* src;

_   (ReadTable(o, &dst));
_   (ReadTable(o, &src));
    _throwif(m3Err_typeMismatch, not IsSubTypeOf(src->type, dst->type));

_   (CheckOperandType(o, 2, Table_AddrType(dst)));
_   (CheckOperandType(o, 1, Table_AddrType(src)));
_   (CheckOperandType(o, 0, (dst->isTable64 and src->isTable64) ? c_m3Type_i64 : c_m3Type_i32));

_   (PreserveRegisterIfOccupied(o, c_m3Type_i64));
_   (EmitOp(o, op_TableCopy));
    EmitPointer(o, dst);
    EmitPointer(o, src);

    വേണ്ടി (u32 i = 0; i < 3; ++i) {
_       (Emit_Slot_അക്കം_Of_അട്ടി_മുകൾ_And_എടുക്കുക(o));
    }

    _catch: തിരികെ result;
}


M3Result Compile_Table_Size (IM3Compilation o, m3opcode_t i_opcode)
{
    M3Result result = m3Err_none;

    M3Table* table;
_   (ReadTable(o, &table));

    // a table size is given in the table's own index type
_   (Compile_Table_Op(o, op_TableSize, table, 0, Table_AddrType(table)));

    _catch: തിരികെ result;
}


M3Result Compile_Table_GrowFill (IM3Compilation o, m3opcode_t i_opcode)
{
    M3Result result = m3Err_none;

    M3Table* table;
_   (ReadTable(o, &table));

    എങ്കിൽ (i_opcode == c_waOp_tableGrow) {
_       (CheckOperandType(o, 0, Table_AddrType(table)));
_       (Compile_Table_Op(o, op_TableGrow, table, 2, Table_AddrType(table)));
    } അല്ലെങ്കിൽ {
_       (CheckOperandType(o, 0, Table_AddrType(table)));
_       (CheckOperandType(o, 2, Table_AddrType(table)));
_       (Compile_Table_Op(o, op_TableFill, table, 3, c_m3Type_none));
    }

    _catch: തിരികെ result;
}

#endif // d_m3HasRefTypes


M3Result Compile_Data_Drop (IM3Compilation o, m3opcode_t i_opcode)
{
    M3Result result = m3Err_none;

    M3DataSegment* segment = ശൂന്യ;
_   (ReadDataSegment(o, &segment));

_   (EmitOp(o, op_DataDrop));
    EmitPointer(o, segment);

    _catch: തിരികെ result;
}


സ്ഥിതം
M3Result ReadBlockType (IM3Compilation o, IM3FuncType* o_blockType)
{
    M3Result result = m3Err_none;

#എങ്കിൽ d_m3HasTypedRefs
    // a spelled-out reference type is two bytes, so it cannot be told apart
    // from a type index by the s33 that block types otherwise use
    എങ്കിൽ (o->wasm < o->wasmEnd and (*o->wasm == d_waEncode_ref or *o->wasm == d_waEncode_refNull)) {
        m3type_t refType;
_       (Parse_മൂല്യം_Type(o->module, &refType, &o->wasm, o->wasmEnd));
        IM3FuncType ftype = ശൂന്യ;
_       (AllocFuncType(&ftype, 1));
        ftype->അക്കം_Rets  = 1;
        ftype->types[0] = refType;
_       (Environment_AddFuncType(o->module->environment, &ftype));
        *o_blockType = ftype;
        തിരികെ result;
    }
#endif

    i64 type;
_   (ReadLebSigned(&type, 33, &o->wasm, o->wasmEnd));

    എങ്കിൽ (type < 0) {
        _throwif(m3Err_invalidTypeId, type < -64);
        u8 മൂല്യം_Type;
_       (NormalizeType(&മൂല്യം_Type, (i8)type));                                 m3log (compile, d_indent " (type: %s)", get_indention_string (o), c_waTypes [മൂല്യം_Type]);
        *o_blockType = o->module->environment->retFuncTypes[മൂല്യം_Type];
    } അല്ലെങ്കിൽ {
        _throwif("func type out of bounds", type >= o->module->അക്കം_Func_Types);
        *o_blockType = o->module->funcTypes[type];                         m3log (compile, d_indent " (type: %s)", get_indention_string (o), SPrintFuncTypeSignature (*o_blockType));
    }
    _catch: തിരികെ result;
}

സ്ഥിതം
M3Result PreserveArgsAndLocals (IM3Compilation o)
{
    M3Result result = m3Err_none;

    എങ്കിൽ (o->അട്ടി_Index > o->അട്ടി_First_Dynamic_Index) {
        u32 അക്കം_Args_And_Locals = Get_Function_അക്കം_Args_And_Locals(o->function);

        വേണ്ടി (u32 i = 0; i < അക്കം_Args_And_Locals; ++i) {
            u16 slot = Get_Slot_For_അട്ടി_Index(o, i);

            u16 preserved_Slot_സംഖ്യ;
_           (FindReferencedLocalWithinCurrentBlock(o, &preserved_Slot_സംഖ്യ, slot));

            എങ്കിൽ (preserved_Slot_സംഖ്യ != slot) {
                m3type_t     type = Get_അട്ടി_Type_From_Bottom(o, i);                 d_m3Assert (type != c_m3Type_none)
                IM3Operation op   = Is64BitType(type) ? op_CopySlot_64 : op_CopySlot_32;

                EmitOp(o, op);
                EmitSlotOffset(o, preserved_Slot_സംഖ്യ);
                EmitSlotOffset(o, slot);
            }
        }
    }

    _catch:
    തിരികെ result;
}

സ്ഥിതം
M3Result Compile_LoopOrBlock (IM3Compilation o, m3opcode_t i_opcode)
{
    M3Result result;

    IM3FuncType blockType = ശൂന്യ;

_   (PreserveRegisters(o));
_   (PreserveArgsAndLocals(o));

_   (ReadBlockType(o, &blockType));

    എങ്കിൽ (i_opcode == c_waOp_loop) {
        u16 അക്കം_Params = Get_Func_Type_അക്കം_Params(blockType);
        എങ്കിൽ (അക്കം_Params) {
            // instantiate constants
            u16 അക്കം_മൂല്യങ്ങൾ = Get_അക്കം_Block_മൂല്യങ്ങൾ_On_അട്ടി(o);

            എങ്കിൽ (അക്കം_മൂല്യങ്ങൾ >= അക്കം_Params) {
                u16 അട്ടി_മുകൾ = Get_അട്ടി_മുകൾ_Index(o) + 1;

                വേണ്ടി (u16 i = അട്ടി_മുകൾ - അക്കം_Params; i < അട്ടി_മുകൾ; ++i) {
                    u16      slot = Get_Slot_For_അട്ടി_Index(o, i);
                    m3type_t type = Get_അട്ടി_Type_From_Bottom(o, i);

                    എങ്കിൽ (IsConstantSlot(o, slot)) {
                        u16 newSlot = c_slotUnused;
_                       (AllocateSlots(o, &newSlot, type));
_                       (Copy_അട്ടി_Index_To_Slot(o, newSlot, i));
                        o->wasm_അട്ടി[i] = newSlot;
                    }
                }
            }
        }

_       (EmitOp(o, op_Loop));
    } അല്ലെങ്കിൽ {
    }

_   (CompileBlock(o, blockType, i_opcode));

    _catch: തിരികെ result;
}

#എങ്കിൽ d_m3HasExceptionHandling

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
സ്ഥിതം
M3Result EmitCatchStubs (IM3Compilation o)
{
    IM3CodePage displacedPage = ശൂന്യ;
_try {
    M3CatchClause* clauses    = o->tryClauses;
    u32            അക്കം_Clauses = o->അക്കം_Try_Clauses;

    // the body must not see these: a nested try_table sets up its own
    o->tryClauses    = ശൂന്യ;
    o->അക്കം_Try_Clauses = 0;

    വേണ്ടി (u32 i = 0; i < അക്കം_Clauses; ++i) {
        M3CatchClause* clause = &clauses[i];

        IM3CompilationScope scope;
_       (GetBlockScope(o, &scope, clause->labelDepth + 1));

        IM3FuncType payload = clause->tag ? clause->tag->type : ശൂന്യ;
        u16         അക്കം_Args = Get_Func_Type_അക്കം_Params(payload);

        // What the stub hands the label - the payload, then the exnref for a
        // _ref clause - has to be what the label takes. ResolveBlockResults
        // only counts the values, and the validator compares base types, so a
        // typed reference is only checked here: (ref null $t) reaching a label
        // that takes (ref $t) has to be rejected.
        {
            bool isLoop         = (scope->opcode == c_waOp_loop);
            u16  അക്കം_Label_മൂല്യങ്ങൾ = isLoop ? Get_Func_Type_അക്കം_Params(scope->type)
                                         : Get_Func_Type_അക്കം_Results(scope->type);

            _throwif(m3_Err_type_എണ്ണം_Mismatch, അക്കം_Label_മൂല്യങ്ങൾ != അക്കം_Args + (clause->hasRef ? 1 : 0));

            വേണ്ടി (u16 a = 0; a < അക്കം_Label_മൂല്യങ്ങൾ; ++a) {
                m3type_t given  = (a < അക്കം_Args) ? GetFuncTypeParamType(payload, a)
                                                : (m3type_t)c_m3Type_exnref;
                m3type_t wanted = isLoop ? GetFuncTypeParamType(scope->type, a)
                                         : GetFuncTypeResultType(scope->type, a);

                _throwif(m3Err_typeMismatch, not IsSubTypeOf(given, wanted));
            }
        }

        IM3CodePage stubPage;
_       (AcquireCompilationCodePage(o, &stubPage));

        pc_t startPC  = GetPagePC(stubPage);
        displacedPage = o->page;
        o->page       = stubPage;

        u16 firstIndex = o->അട്ടി_Index;

        വേണ്ടി (u16 a = 0; a < അക്കം_Args; ++a) {
_           (തള്ളുക_Allocated_Slot(o, GetFuncTypeParamType(payload, a)));
        }

        എങ്കിൽ (clause->hasRef) {
_           (തള്ളുക_Allocated_Slot(o, c_m3Type_exnref));
        }

        // emitted even with nothing to move: this is also where an exception
        // no clause will reify gets released
_       (Ensure_Code_Page_അക്കം_വരികൾ(o, 2 * അക്കം_Args + 3 + d_m3_Code_Page_Free_വരികൾ_Threshold));

_       (EmitOp(o, op_CatchPayload));
        EmitConstant32(o, അക്കം_Args);

        വേണ്ടി (u16 a = 0; a < അക്കം_Args; ++a) {
            u16 index = firstIndex + a;
            EmitSlotOffset(o, Get_Slot_For_അട്ടി_Index(o, index));
            EmitConstant32(o, Is64BitType(Get_അട്ടി_Type_From_Bottom(o, index)));
        }

        EmitSlotOffset(o, clause->hasRef ? (i32)Get_Slot_For_അട്ടി_Index(o, firstIndex + അക്കം_Args) : -1);

_       (Emit_എടുക്കുക_Try_Frames(o, അക്കം_Try_Frames_To_എടുക്കുക(o->block.outer, scope)));

        എങ്കിൽ (scope->opcode == c_waOp_loop) {
_           (ResolveBlockResults(o, scope, /* isBranch: */ true));

_           (EmitOp(o, Op_ContinueLoop(o)));
            EmitPointer(o, scope->pc);
_           (RecordBackEdge(o, scope, false));
        } അല്ലെങ്കിൽ എങ്കിൽ (scope->depth == 0) {
_           (Return_മൂല്യങ്ങൾ(o, scope, /* isBranch: */ true));
_           (EmitOp(o, op_Return));
        } അല്ലെങ്കിൽ {
_           (ResolveBlockResults(o, scope, /* isBranch: */ true));
_           (EmitPatchingBranch(o, scope));
        }

        IM3CodePage returnTo = displacedPage;
        displacedPage        = ശൂന്യ;

        ReleaseCompilationCodePage(o);
        o->page = returnTo;

        *(pc_t*)clause->stubSlot = startPC;

        ആകുമ്പോൾ (o->അട്ടി_Index > firstIndex) {
_           (എടുക്കുക(o));
        }
    }

} _catch:

    // thrown with a stub page installed: release it and put back the one it
    // displaced, which the caller's catch then releases
    എങ്കിൽ (displacedPage) {
        ReleaseCompilationCodePage(o);
        o->page = displacedPage;
    }

    തിരികെ result;
}


സ്ഥിതം
M3Result Compile_TryTable (IM3Compilation o, m3opcode_t i_opcode)
{
    M3CatchClause* clauses = ശൂന്യ;
_try {
    // a catch entry has to be able to assume the operand stack is entirely in
    // slots: an unwind leaves nothing in the registers
_   (PreserveRegisters(o));
_   (PreserveArgsAndLocals(o));

    IM3FuncType blockType = ശൂന്യ;
_   (ReadBlockType(o, &blockType));

    u32 അക്കം_Clauses;
_   (ReadLEB_u32(&അക്കം_Clauses, &o->wasm, o->wasmEnd));

    _throwif("too many catch clauses", അക്കം_Clauses > d_m3_Max_Sane_Tags_എണ്ണം);

    എങ്കിൽ (അക്കം_Clauses) {
        clauses = m3_AllocArray(M3CatchClause, അക്കം_Clauses);
        _throwifnull(clauses);
    }

    വേണ്ടി (u32 i = 0; i < അക്കം_Clauses; ++i) {
        u8 kind;
_       (Read_u8(&kind, &o->wasm, o->wasmEnd));
        _throwif(m3Err_wasmMalformed, kind > 0x03);

        clauses[i].tag    = ശൂന്യ;
        clauses[i].hasRef = (kind == 0x01 or kind == 0x03);

        എങ്കിൽ (kind == 0x00 or kind == 0x01) {
            u32 tagIndex;
_           (ReadLEB_u32(&tagIndex, &o->wasm, o->wasmEnd));
            _throwif(m3Err_unknownTag, tagIndex >= o->module->അക്കം_Tags);

            IM3Tag tag     = &o->module->tags[tagIndex];
            clauses[i].tag = tag->resolved ? tag->resolved : tag;
        }

_       (ReadLEB_u32(&clauses[i].labelDepth, &o->wasm, o->wasmEnd));
    }

    // op_TryTable, the clause count, and two words per clause, all of which have
    // to land on one page: the op reads the table by offset from its own pc
_   (Ensure_Code_Page_അക്കം_വരികൾ(o, 2 * അക്കം_Clauses + 2 + d_m3_Code_Page_Free_വരികൾ_Threshold));

_   (EmitOp(o, op_TryTable));
    EmitConstant32(o, അക്കം_Clauses);

    വേണ്ടി (u32 i = 0; i < അക്കം_Clauses; ++i) {
        EmitPointer(o, clauses[i].tag);
        clauses[i].stubSlot = ReservePointer(o);
    }

    o->tryClauses    = clauses;
    o->അക്കം_Try_Clauses = അക്കം_Clauses;

_   (CompileBlock(o, blockType, c_waOp_tryTable));

} _catch:

    o->tryClauses    = ശൂന്യ;
    o->അക്കം_Try_Clauses = 0;

    m3_Free(clauses);

    തിരികെ result;
}


സ്ഥിതം
M3Result Compile_Throw (IM3Compilation o, m3opcode_t i_opcode)
{
_try {
    u32 tagIndex;
_   (ReadLEB_u32(&tagIndex, &o->wasm, o->wasmEnd));
    _throwif(m3Err_unknownTag, tagIndex >= o->module->അക്കം_Tags);

    IM3Tag tag = &o->module->tags[tagIndex];
    എങ്കിൽ (tag->resolved) {
        tag = tag->resolved;
    }

    u16 അക്കം_Args = Get_Func_Type_അക്കം_Params(tag->type);

    // the payload is read out of slots, so nothing may still be in a register
_   (PreserveRegisters(o));

    // check the payload before emitting anything, so a mistyped throw does not
    // leave half an operation behind
    bool isPolymorphic = Is_അട്ടി_Polymorphic(o);

    എങ്കിൽ (not isPolymorphic) {
        _throwif(m3_Err_type_എണ്ണം_Mismatch, Get_അക്കം_Block_മൂല്യങ്ങൾ_On_അട്ടി(o) < അക്കം_Args);

        വേണ്ടി (u16 i = 0; i < അക്കം_Args; ++i) {
            u16 index = (u16)(o->അട്ടി_Index - അക്കം_Args + i);

            _throwif(m3Err_typeMismatch, not IsSubTypeOf(Get_അട്ടി_Type_From_Bottom(o, index),
                                                         GetFuncTypeParamType(tag->type, i)));
        }
    }

_   (Ensure_Code_Page_അക്കം_വരികൾ(o, 2 * അക്കം_Args + 3 + d_m3_Code_Page_Free_വരികൾ_Threshold));

_   (EmitOp(o, op_Throw));
    EmitPointer(o, tag);
    EmitConstant32(o, അക്കം_Args);

    // the payload is named in the order the tag declares it, so the deepest of
    // the arguments on the stack comes first. In unreachable code there is
    // nothing on the stack to name and nothing will run this, so slot 0 stands
    // in for an operand that isn't there.
    വേണ്ടി (u16 i = 0; i < അക്കം_Args; ++i) {
        m3type_t type = GetFuncTypeParamType(tag->type, i);

        EmitSlotOffset(o, isPolymorphic ? 0 : Get_Slot_For_അട്ടി_Index(o, (u16)(o->അട്ടി_Index - അക്കം_Args + i)));
        EmitConstant32(o, Is64BitType(type));
    }

    എങ്കിൽ (not isPolymorphic) {
        വേണ്ടി (u16 i = 0; i < അക്കം_Args; ++i) {
_           (എടുക്കുക(o));
        }
    }

_   (Set_അട്ടി_Polymorphic(o));

} _catch: തിരികെ result;
}


സ്ഥിതം
M3Result Compile_ThrowRef (IM3Compilation o, m3opcode_t i_opcode)
{
_try {
    എങ്കിൽ (not Is_അട്ടി_Polymorphic(o)) {
_       (PreserveRegisterIfOccupied(o, c_m3Type_i64));

        _throwif(m3Err_typeMismatch, BaseTypeOf(Get_അട്ടി_മുകൾ_Type(o)) != c_m3Type_exnref);

_       (EmitOp(o, op_ThrowRef));
        EmitSlotOffset(o, Get_അട്ടി_മുകൾ_Slot_സംഖ്യ(o));

_       (എടുക്കുക(o));
    }

_   (Set_അട്ടി_Polymorphic(o));

} _catch: തിരികെ result;
}

#endif // d_m3HasExceptionHandling

#എങ്കിൽ d_m3_Has_അട്ടി_Switching

സ്ഥിതം
M3Result Compile_ContNew (IM3Compilation o, m3opcode_t i_opcode)
{
_try {
    u32 typeIndex;
_   (ReadLEB_u32(&typeIndex, &o->wasm, o->wasmEnd));
    _throwif(m3_Err_type_എണ്ണം_Mismatch, typeIndex >= o->module->അക്കം_Func_Types);

    IM3FuncType funcType = o->module->funcTypes[typeIndex];
    _throwif("type is not a continuation", not funcType->isContinuation);

    // the operands are read out of slots, so nothing may still be in a register
_   (PreserveRegisters(o));

    // cont.new $ct : [(ref null $ft)] -> [(ref $ct)], where $ct ~~ cont $ft:
    // the function has to be one this continuation type can run
    m3type_t മുകൾ_Type = Get_അട്ടി_മുകൾ_Type(o);
    _throwif(m3Err_typeMismatch,
             not IsSubTypeOf(മുകൾ_Type, RefTypeOfFuncType(funcType->contFuncType, false)));

    u16 funcSlot = Get_അട്ടി_മുകൾ_Slot_സംഖ്യ(o);
_   (എടുക്കുക(o));

    u16 dstSlot;
_   (തള്ളുക_Allocated_Slot(o, RefTypeOfFuncType(funcType, true)));
    dstSlot = Get_അട്ടി_മുകൾ_Slot_സംഖ്യ(o);

_   (Ensure_Code_Page_അക്കം_വരികൾ(o, 4 + d_m3_Code_Page_Free_വരികൾ_Threshold));
_   (EmitOp(o, op_ContNew));
    EmitPointer(o, funcType);
    EmitSlotOffset(o, dstSlot);
    EmitSlotOffset(o, funcSlot);

} _catch: തിരികെ result;
}

സ്ഥിതം
M3Result Compile_ContBind (IM3Compilation o, m3opcode_t i_opcode)
{
_try {
    u32 typeIndex1, typeIndex2;
_   (ReadLEB_u32(&typeIndex1, &o->wasm, o->wasmEnd));
_   (ReadLEB_u32(&typeIndex2, &o->wasm, o->wasmEnd));
    _throwif(m3_Err_type_എണ്ണം_Mismatch, typeIndex1 >= o->module->അക്കം_Func_Types);
    _throwif(m3_Err_type_എണ്ണം_Mismatch, typeIndex2 >= o->module->അക്കം_Func_Types);

    IM3FuncType ct1 = o->module->funcTypes[typeIndex1];
    IM3FuncType ct2 = o->module->funcTypes[typeIndex2];
    _throwif("type is not a continuation", not ct1->isContinuation || not ct2->isContinuation);

    // the operands are read out of slots, so nothing may still be in a register
_   (PreserveRegisters(o));

    m3type_t contType = Get_അട്ടി_മുകൾ_Type(o);
    _throwif(m3Err_typeMismatch, not IsSubTypeOf(contType, RefTypeOfFuncType(ct1, false)));
    u16 contSlot = Get_അട്ടി_മുകൾ_Slot_സംഖ്യ(o);
_   (എടുക്കുക(o));

    IM3FuncType ft1 = ct1->contFuncType ? ct1->contFuncType : ct1;
    IM3FuncType ft2 = ct2->contFuncType ? ct2->contFuncType : ct2;

    u16 അക്കം_Params1 = Get_Func_Type_അക്കം_Params(ft1);
    u16 അക്കം_Params2 = Get_Func_Type_അക്കം_Params(ft2);
    _throwif(m3Err_typeMismatch, അക്കം_Params1 < അക്കം_Params2);
    u32 അക്കം_Bound = അക്കം_Params1 - അക്കം_Params2;

    u16 അക്കം_Results1 = Get_Func_Type_അക്കം_Results(ft1);
    u16 അക്കം_Results2 = Get_Func_Type_അക്കം_Results(ft2);
    _throwif(m3Err_typeMismatch, അക്കം_Results1 != അക്കം_Results2);
    വേണ്ടി (u16 i = 0; i < അക്കം_Results1; ++i) {
        _throwif(m3Err_typeMismatch, GetFuncTypeResultType(ft1, i) != GetFuncTypeResultType(ft2, i));
    }
    വേണ്ടി (u16 i = 0; i < അക്കം_Params2; ++i) {
        _throwif(m3Err_typeMismatch, GetFuncTypeParamType(ft1, അക്കം_Bound + i) != GetFuncTypeParamType(ft2, i));
    }

    u16 argSlots[d_m3MaxContinuationPayload];
    _throwif("too many bound args", അക്കം_Bound > d_m3MaxContinuationPayload);

    വേണ്ടി (u32 i = 0; i < അക്കം_Bound; ++i) {
        u32      paramIdx     = അക്കം_Bound - 1 - i;
        m3type_t expectedType = GetFuncTypeParamType(ft1, paramIdx);
        m3type_t actualType   = Get_അട്ടി_മുകൾ_Type(o);
        _throwif(m3Err_typeMismatch, not IsSubTypeOf(actualType, expectedType));
        argSlots[paramIdx] = Get_അട്ടി_മുകൾ_Slot_സംഖ്യ(o);
_       (എടുക്കുക(o));
    }

    u16 dstSlot;
_   (തള്ളുക_Allocated_Slot(o, RefTypeOfFuncType(ct2, true)));
    dstSlot = Get_അട്ടി_മുകൾ_Slot_സംഖ്യ(o);

_   (Ensure_Code_Page_അക്കം_വരികൾ(o, 6 + അക്കം_Bound * 2 + d_m3_Code_Page_Free_വരികൾ_Threshold));
_   (EmitOp(o, op_ContBind));
    EmitPointer(o, ct1);
    EmitPointer(o, ct2);
    EmitSlotOffset(o, dstSlot);
    EmitSlotOffset(o, contSlot);
    EmitConstant32(o, അക്കം_Bound);
    വേണ്ടി (u32 i = 0; i < അക്കം_Bound; ++i) {
        EmitSlotOffset(o, argSlots[i]);
        EmitConstant32(o, Is64BitType(GetFuncTypeParamType(ft1, i)));
    }

} _catch: തിരികെ result;
}

സ്ഥിതം
M3Result Compile_Suspend (IM3Compilation o, m3opcode_t i_opcode)
{
_try {
    u32 tagIndex;
_   (ReadLEB_u32(&tagIndex, &o->wasm, o->wasmEnd));
    _throwif(m3Err_unknownTag, tagIndex >= o->module->അക്കം_Tags);

    IM3Tag tag = &o->module->tags[tagIndex];
    എങ്കിൽ (tag->resolved) {
        tag = tag->resolved;
    }
    u16 അക്കം_Params  = Get_Func_Type_അക്കം_Params(tag->type);
    u16 അക്കം_Results = Get_Func_Type_അക്കം_Results(tag->type);

_   (PreserveRegisters(o));

    u16 paramSlots[d_m3MaxContinuationPayload];
    _throwif("too many tag params", അക്കം_Params > d_m3MaxContinuationPayload);
    വേണ്ടി (u16 i = 0; i < അക്കം_Params; ++i) {
        u16      pIdx         = അക്കം_Params - 1 - i;
        m3type_t expectedType = GetFuncTypeParamType(tag->type, pIdx);
        m3type_t actualType   = Get_അട്ടി_മുകൾ_Type(o);
        _throwif(m3Err_typeMismatch, not IsSubTypeOf(actualType, expectedType));
        paramSlots[pIdx] = Get_അട്ടി_മുകൾ_Slot_സംഖ്യ(o);
_       (എടുക്കുക(o));
    }

    u16 resultSlots[d_m3MaxContinuationPayload];
    _throwif("too many tag results", അക്കം_Results > d_m3MaxContinuationPayload);
    വേണ്ടി (u16 i = 0; i < അക്കം_Results; ++i) {
        m3type_t rtype = GetFuncTypeResultType(tag->type, i);
_       (തള്ളുക_Allocated_Slot(o, rtype));
        resultSlots[i] = Get_അട്ടി_മുകൾ_Slot_സംഖ്യ(o);
    }

_   (Ensure_Code_Page_അക്കം_വരികൾ(o, 4 + അക്കം_Params * 2 + അക്കം_Results * 2 + d_m3_Code_Page_Free_വരികൾ_Threshold));
_   (EmitOp(o, op_Suspend));
    EmitPointer(o, tag);
    EmitConstant32(o, അക്കം_Params);
    വേണ്ടി (u16 i = 0; i < അക്കം_Params; ++i) {
        EmitSlotOffset(o, paramSlots[i]);
        EmitConstant32(o, Is64BitType(GetFuncTypeParamType(tag->type, i)));
    }
    EmitConstant32(o, അക്കം_Results);
    വേണ്ടി (u16 i = 0; i < അക്കം_Results; ++i) {
        EmitSlotOffset(o, resultSlots[i]);
        EmitConstant32(o, Is64BitType(GetFuncTypeResultType(tag->type, i)));
    }

    // the results are part of the frame here: op_Suspend clears them, and
    // whatever resumes this writes them before anything reads them
_   (RecordSafePoint(o, safepoint_suspend, 0, അക്കം_Results, 0, ശൂന്യ));

} _catch: തിരികെ result;
}

// resume, resume_throw and resume_throw_ref. All three run a continuation
// under a handler table and take its results; they differ only in what they
// hand it to start with - arguments, a fresh exception, or one off the stack.
//
// Emitted as: the continuation type, the slot holding it, the operands that
// belong to this form, the handler table, then the result slots. Keeping the
// tail identical is what lets one runtime path finish all three.
സ്ഥിതം
M3Result Compile_Resume (IM3Compilation o, m3opcode_t i_opcode)
{
    M3ResumeHandler* handlers      = ശൂന്യ;
    IM3CodePage      displacedPage = ശൂന്യ;
_try {
    bool isThrow    = (i_opcode == c_waOp_resumeThrow);
    bool isThrowRef = (i_opcode == c_waOp_resumeThrowRef);

    u32 typeIndex;
_   (ReadLEB_u32(&typeIndex, &o->wasm, o->wasmEnd));
    _throwif(m3_Err_type_എണ്ണം_Mismatch, typeIndex >= o->module->അക്കം_Func_Types);

    IM3FuncType contFuncType = o->module->funcTypes[typeIndex];
    _throwif("type is not a continuation", not contFuncType->isContinuation);

    IM3FuncType innerType      = contFuncType->contFuncType;
    u16         അക്കം_Cont_Results = Get_Func_Type_അക്കം_Results(innerType);

    IM3Tag exnTag      = ശൂന്യ;
    u16    അക്കം_Operands = 0;

    എങ്കിൽ (isThrow) {
        u32 tagIndex;
_       (ReadLEB_u32(&tagIndex, &o->wasm, o->wasmEnd));
        _throwif(m3Err_unknownTag, tagIndex >= o->module->അക്കം_Tags);

        exnTag = &o->module->tags[tagIndex];

        // it is an exception that gets raised, so the tag must be one throw
        // could have raised: no results
        _throwif(m3Err_typeMismatch, Get_Func_Type_അക്കം_Results(exnTag->type) != 0);

        അക്കം_Operands = Get_Func_Type_അക്കം_Params(exnTag->type);
    } അല്ലെങ്കിൽ എങ്കിൽ (not isThrowRef) {
        // a plain resume hands over the continuation's own parameters. An
        // aborted one never receives them, so they are unconstrained there.
        അക്കം_Operands = Get_Func_Type_അക്കം_Params(innerType);
    }

    u32 അക്കം_Clauses;
_   (ReadLEB_u32(&അക്കം_Clauses, &o->wasm, o->wasmEnd));

    u32 clauseLabelDepths[16];
    _throwif("too many resume clauses", അക്കം_Clauses > 16);

    എങ്കിൽ (അക്കം_Clauses) {
        handlers = m3_AllocArray(M3ResumeHandler, അക്കം_Clauses);
        _throwifnull(handlers);
    }

    വേണ്ടി (u32 i = 0; i < അക്കം_Clauses; ++i) {
        u8 kind;
_       (Read_u8(&kind, &o->wasm, o->wasmEnd));
        _throwif(m3Err_wasmMalformed, kind > 0x01);

        handlers[i].kind = kind;

        u32 tagIndex;
_       (ReadLEB_u32(&tagIndex, &o->wasm, o->wasmEnd));
        _throwif(m3Err_unknownTag, tagIndex >= o->module->അക്കം_Tags);

        IM3Tag tag         = &o->module->tags[tagIndex];
        handlers[i].tag    = tag->resolved ? tag->resolved : tag;
        handlers[i].stubPC = ശൂന്യ;

        എങ്കിൽ (kind == 0x00) {
_           (ReadLEB_u32(&clauseLabelDepths[i], &o->wasm, o->wasmEnd));
        } അല്ലെങ്കിൽ {
            // (on $e switch): the tag carries no payload, and what the switch
            // site yields is what this resume yields
            _throwif(m3Err_typeMismatch, Get_Func_Type_അക്കം_Params(handlers[i].tag->type) != 0);
            clauseLabelDepths[i] = 0;
        }
    }

    // the operands are read out of slots, so nothing may still be in a register
_   (PreserveRegisters(o));

    m3type_t cont_മുകൾ_Type = Get_അട്ടി_മുകൾ_Type(o);
    _throwif(m3Err_typeMismatch, not IsSubTypeOf(cont_മുകൾ_Type, RefTypeOfFuncType(contFuncType, false)));
    u16 contSlot = Get_അട്ടി_മുകൾ_Slot_സംഖ്യ(o);
_   (എടുക്കുക(o));

    u16 exnSlot = 0;

    എങ്കിൽ (isThrowRef) {
        m3type_t exnType = Get_അട്ടി_മുകൾ_Type(o);
        _throwif(m3Err_typeMismatch, BaseTypeOf(exnType) != c_m3Type_exnref);
        exnSlot = Get_അട്ടി_മുകൾ_Slot_സംഖ്യ(o);
_       (എടുക്കുക(o));
    }

    u16 operandSlots[d_m3MaxContinuationPayload];
    _throwif("too many resume operands", അക്കം_Operands > d_m3MaxContinuationPayload);

    IM3FuncType operandType = isThrow ? exnTag->type : innerType;

    വേണ്ടി (u16 i = 0; i < അക്കം_Operands; ++i) {
        u16      pIdx         = (u16)(അക്കം_Operands - 1 - i);
        m3type_t expectedType = GetFuncTypeParamType(operandType, pIdx);
        m3type_t actualType   = Get_അട്ടി_മുകൾ_Type(o);
        _throwif(m3Err_typeMismatch, not IsSubTypeOf(actualType, expectedType));
        operandSlots[pIdx] = Get_അട്ടി_മുകൾ_Slot_സംഖ്യ(o);
_       (എടുക്കുക(o));
    }

    വേണ്ടി (u32 i = 0; i < അക്കം_Clauses; ++i) {
        എങ്കിൽ (handlers[i].kind != 0x00) {
            തുടരുക;
        }

        IM3CompilationScope scope;
_       (GetBlockScope(o, &scope, clauseLabelDepths[i]));

        IM3Tag tag        = handlers[i].tag;
        u16    അക്കം_Payload = Get_Func_Type_അക്കം_Params(tag->type);

        m3type_t labelContType = (scope->opcode == c_waOp_loop)
                                   ? GetFuncTypeParamType(scope->type, അക്കം_Payload)
                                   : GetFuncTypeResultType(scope->type, അക്കം_Payload);

        IM3CodePage stubPage;
_       (AcquireCompilationCodePage(o, &stubPage));
        pc_t startPC  = GetPagePC(stubPage);
        displacedPage = o->page;
        o->page       = stubPage;

        u16 firstIndex = o->അട്ടി_Index;
        വേണ്ടി (u16 a = 0; a < അക്കം_Payload; ++a) {
_           (തള്ളുക_Allocated_Slot(o, GetFuncTypeParamType(tag->type, a)));
        }

        // The handler is handed the suspended continuation.
_       (തള്ളുക_Allocated_Slot(o, labelContType));

_       (Ensure_Code_Page_അക്കം_വരികൾ(o, 2 * അക്കം_Payload + 5 + d_m3_Code_Page_Free_വരികൾ_Threshold));
_       (EmitOp(o, op_ResumePayload));
        EmitPointer(o, Module_ContTypeOfRef(o->module, labelContType));
        EmitConstant32(o, അക്കം_Payload);
        വേണ്ടി (u16 a = 0; a < അക്കം_Payload; ++a) {
            u16 index = firstIndex + a;
            EmitSlotOffset(o, Get_Slot_For_അട്ടി_Index(o, index));
            EmitConstant32(o, Is64BitType(Get_അട്ടി_Type_From_Bottom(o, index)));
        }
        EmitSlotOffset(o, Get_Slot_For_അട്ടി_Index(o, firstIndex + അക്കം_Payload));

#  എങ്കിൽ d_m3HasExceptionHandling
        // the clause leaves every try region between the resume and its label,
        // and leaving one retires its handlers, the way a branch does
_       (Emit_എടുക്കുക_Try_Frames(o, അക്കം_Try_Frames_To_എടുക്കുക(&o->block, scope)));
#  endif

        എങ്കിൽ (scope->opcode == c_waOp_loop) {
_           (ResolveBlockResults(o, scope, true));
_           (EmitOp(o, Op_ContinueLoop(o)));
            EmitPointer(o, scope->pc);
_           (RecordBackEdge(o, scope, false));
        } അല്ലെങ്കിൽ എങ്കിൽ (scope->depth == 0) {
_           (Return_മൂല്യങ്ങൾ(o, scope, true));
_           (EmitOp(o, op_Return));
        } അല്ലെങ്കിൽ {
_           (ResolveBlockResults(o, scope, true));
_           (EmitPatchingBranch(o, scope));
        }

        IM3CodePage returnTo = displacedPage;
        displacedPage        = ശൂന്യ;

        ReleaseCompilationCodePage(o);
        o->page = returnTo;

        handlers[i].stubPC = startPC;

        ആകുമ്പോൾ (o->അട്ടി_Index > firstIndex) {
_           (എടുക്കുക(o));
        }
    }

    u16 resSlots[d_m3MaxContinuationPayload];
    _throwif("too many cont results", അക്കം_Cont_Results > d_m3MaxContinuationPayload);
    വേണ്ടി (u16 i = 0; i < അക്കം_Cont_Results; ++i) {
        m3type_t rtype = GetFuncTypeResultType(innerType, i);
_       (തള്ളുക_Allocated_Slot(o, rtype));
        resSlots[i] = Get_അട്ടി_മുകൾ_Slot_സംഖ്യ(o);
    }

_   (Ensure_Code_Page_അക്കം_വരികൾ(o, 8 + അക്കം_Operands * 2 + അക്കം_Clauses * 3 + അക്കം_Cont_Results * 2 +
                                  d_m3_Code_Page_Free_വരികൾ_Threshold));

_   (EmitOp(o, op_Resume));
    EmitPointer(o, contFuncType);
    EmitSlotOffset(o, contSlot);
    EmitConstant32(o, isThrow ? d_m3ResumeThrow : (isThrowRef ? d_m3ResumeThrowRef : d_m3ResumeNormal));

    എങ്കിൽ (isThrowRef) {
        EmitSlotOffset(o, exnSlot);
    } അല്ലെങ്കിൽ {
        എങ്കിൽ (isThrow) {
            EmitPointer(o, exnTag);
        }

        EmitConstant32(o, അക്കം_Operands);
        വേണ്ടി (u16 i = 0; i < അക്കം_Operands; ++i) {
            EmitSlotOffset(o, operandSlots[i]);
            EmitConstant32(o, Is64BitType(GetFuncTypeParamType(operandType, i)));
        }
    }

    EmitConstant32(o, അക്കം_Clauses);
    വേണ്ടി (u32 i = 0; i < അക്കം_Clauses; ++i) {
        EmitConstant32(o, handlers[i].kind);
        EmitPointer(o, handlers[i].tag);
        EmitPointer(o, handlers[i].stubPC);
    }

    EmitConstant32(o, അക്കം_Cont_Results);
    വേണ്ടി (u16 i = 0; i < അക്കം_Cont_Results; ++i) {
        EmitSlotOffset(o, resSlots[i]);
        EmitConstant32(o, Is64BitType(GetFuncTypeResultType(innerType, i)));
    }

    // a suspend passing through the resumed continuation leaves this frame
    // waiting here, with the continuation's results not yet taken
_   (RecordSafePoint(o, safepoint_resume, 0, അക്കം_Cont_Results, (u16)അക്കം_Clauses, ശൂന്യ));
#  എങ്കിൽ d_m3HasSnapshots
    എങ്കിൽ (o->snapshotMap and o->page) {
        o->snapshotMap->safePoints[o->snapshotMap->അക്കം_Safe_Points - 1].resumeType = RefTypeOfFuncType(contFuncType, true);
    }
#  endif

} _catch:
    എങ്കിൽ (displacedPage) {
        ReleaseCompilationCodePage(o);
        o->page = displacedPage;
    }
    m3_Free(handlers);
    തിരികെ result;
}


സ്ഥിതം
M3Result Compile_Switch (IM3Compilation o, m3opcode_t i_opcode)
{
_try {
    u32 typeIndex, tagIndex;
_   (ReadLEB_u32(&typeIndex, &o->wasm, o->wasmEnd));
_   (ReadLEB_u32(&tagIndex, &o->wasm, o->wasmEnd));
    _throwif(m3_Err_type_എണ്ണം_Mismatch, typeIndex >= o->module->അക്കം_Func_Types);
    _throwif(m3Err_unknownTag, tagIndex >= o->module->അക്കം_Tags);

    IM3FuncType targetContType = o->module->funcTypes[typeIndex];
    IM3Tag      tag            = &o->module->tags[tagIndex];
    എങ്കിൽ (tag->resolved) {
        tag = tag->resolved;
    }

    _throwif("type is not a continuation", not targetContType->isContinuation);

    // a switch tag carries no payload of its own: the values travel as the
    // peer's arguments, and the tag's results are what the switch site yields
    _throwif(m3Err_typeMismatch, Get_Func_Type_അക്കം_Params(tag->type) != 0);

    // switch $ct1 : [t1* (ref null $ct1)] -> [t2*]
    //   where $ct1 = cont [t1* (ref null? $ct2)] -> [..]
    //   and   $ct2 = cont [t2*] -> [..]
    //
    // The trailing parameter of $ct1 is not an operand: the peer receives the
    // continuation this instruction suspends. And what this instruction yields
    // is not $ct1's results but $ct2's parameters - the values whoever comes
    // back this way will hand over.
    IM3FuncType targetType = targetContType->contFuncType;
    u16         അക്കം_Params  = Get_Func_Type_അക്കം_Params(targetType);

    _throwif("switch target takes no continuation", അക്കം_Params == 0);

    m3type_t    peerRefType  = GetFuncTypeParamType(targetType, (u16)(അക്കം_Params - 1));
    IM3FuncType selfContType = Module_ContTypeOfRef(o->module, peerRefType);

    _throwif(m3Err_typeMismatch, not selfContType);

    IM3FuncType selfType   = selfContType->contFuncType;
    u16         അക്കം_Args    = (u16)(അക്കം_Params - 1);
    u16         അക്കം_Results = Get_Func_Type_അക്കം_Params(selfType);

    // the operands are read out of slots, so nothing may still be in a register
_   (PreserveRegisters(o));

    m3type_t cont_മുകൾ_Type = Get_അട്ടി_മുകൾ_Type(o);
    _throwif(m3Err_typeMismatch, not IsSubTypeOf(cont_മുകൾ_Type, RefTypeOfFuncType(targetContType, false)));
    u16 targetContSlot = Get_അട്ടി_മുകൾ_Slot_സംഖ്യ(o);
_   (എടുക്കുക(o));

    u16 argSlots[d_m3MaxContinuationPayload];
    _throwif("too many switch params", അക്കം_Args > d_m3MaxContinuationPayload);

    വേണ്ടി (u16 i = 0; i < അക്കം_Args; ++i) {
        u16      pIdx         = (u16)(അക്കം_Args - 1 - i);
        m3type_t expectedType = GetFuncTypeParamType(targetType, pIdx);
        m3type_t actualType   = Get_അട്ടി_മുകൾ_Type(o);
        _throwif(m3Err_typeMismatch, not IsSubTypeOf(actualType, expectedType));
        argSlots[pIdx] = Get_അട്ടി_മുകൾ_Slot_സംഖ്യ(o);
_       (എടുക്കുക(o));
    }

    u16 resultSlots[d_m3MaxContinuationPayload];
    _throwif("too many switch results", അക്കം_Results > d_m3MaxContinuationPayload);

    വേണ്ടി (u16 i = 0; i < അക്കം_Results; ++i) {
_       (തള്ളുക_Allocated_Slot(o, GetFuncTypeParamType(selfType, i)));
        resultSlots[i] = Get_അട്ടി_മുകൾ_Slot_സംഖ്യ(o);
    }

_   (Ensure_Code_Page_അക്കം_വരികൾ(o, 6 + അക്കം_Args * 2 + അക്കം_Results * 2 + d_m3_Code_Page_Free_വരികൾ_Threshold));
_   (EmitOp(o, op_Switch));
    EmitPointer(o, targetContType);
    EmitPointer(o, tag);
    EmitSlotOffset(o, targetContSlot);
    EmitConstant32(o, അക്കം_Args);
    വേണ്ടി (u16 i = 0; i < അക്കം_Args; ++i) {
        EmitSlotOffset(o, argSlots[i]);
        EmitConstant32(o, Is64BitType(GetFuncTypeParamType(targetType, i)));
    }
    EmitConstant32(o, അക്കം_Results);
    വേണ്ടി (u16 i = 0; i < അക്കം_Results; ++i) {
        EmitSlotOffset(o, resultSlots[i]);
        EmitConstant32(o, Is64BitType(GetFuncTypeParamType(selfType, i)));
    }

    // as with suspend: op_Switch clears the results before it leaves
_   (RecordSafePoint(o, safepoint_suspend, 0, അക്കം_Results, 0, ശൂന്യ));

} _catch: തിരികെ result;
}

#endif // d_m3HasStackSwitching


സ്ഥിതം
M3Result CompileElseBlock (IM3Compilation o, pc_t* o_startPC, IM3FuncType i_blockType)
{
    IM3CodePage savedPage = o->page;
_try {

    IM3CodePage elsePage;
_   (AcquireCompilationCodePage(o, &elsePage));

    *o_startPC = GetPagePC(elsePage);

    o->page = elsePage;

_   (CompileBlock(o, i_blockType, c_waOp_else));

_   (EmitOp(o, op_Branch));
    EmitPointer(o, GetPagePC(savedPage));
} _catch:

    എങ്കിൽ (o->page != savedPage) {
        ReleaseCompilationCodePage(o);
    }

    o->page = savedPage;

    തിരികെ result;
}

സ്ഥിതം
M3Result Compile_If (IM3Compilation o, m3opcode_t i_opcode)
{
    /*      [   op_If   ]
            [ <else-pc> ]   ---->   [ ..else..  ]
            [  ..if..   ]           [ ..block.. ]
            [ ..block.. ]           [ op_Branch ]
            [    end    ]  <-----   [  <end-pc> ]       */

_try {

    // Spec: if condition must be i32
    എങ്കിൽ (not Is_അട്ടി_Polymorphic(o)) {
        m3type_t condType = Get_അട്ടി_മുകൾ_Type(o);
        _throwif(m3Err_typeMismatch, condType != c_m3Type_none and condType != c_m3Type_i32);
    }

_   (Preserve_Non_മുകൾ_Registers(o));
_   (PreserveArgsAndLocals(o));

#എങ്കിൽ d_m3FuseBranch
    IM3Operation fuseOp = GetBranchFusionOp(o, true);

    എങ്കിൽ (fuseOp) {
        // absorb the if into the compare that produced the condition
        *(IM3Operation*)o->foldPatchPC = fuseOp;
        InvalidateFold(o);

_       (എടുക്കുക(o));                          // the condition is consumed inside the fused op
    } അല്ലെങ്കിൽ
#endif
    {
        IM3Operation op = Is_അട്ടി_മുകൾ_In_Register(o) ? op_If_r : op_If_s;

_       (EmitOp(o, op));
_       (Emit_Slot_അക്കം_Of_അട്ടി_മുകൾ_And_എടുക്കുക(o));
    }

    pc_t* pc = (pc_t*)ReservePointer(o);

    IM3FuncType blockType = ശൂന്യ;
_   (ReadBlockType(o, &blockType));     //  dump_type_stack (o);

    u16 അട്ടി_Index = o->അട്ടി_Index;

_   (CompileBlock(o, blockType, i_opcode));

    എങ്കിൽ (o->previousOpcode == c_waOp_else) {
        o->അട്ടി_Index = അട്ടി_Index;
_       (CompileElseBlock(o, pc, blockType));
    } അല്ലെങ്കിൽ {
        // if block produces values and there isn't a defined else
        // case, then we need to make one up so that the pass-through
        // results end up in the right place
        എങ്കിൽ (Get_Func_Type_അക്കം_Results(blockType)) {
            // rewind to the if's end to create a fake else block
            o->wasm--;
            o->അട്ടി_Index = അട്ടി_Index;     // dump_type_stack (o);

_           (CompileElseBlock(o, pc, blockType));
        } അല്ലെങ്കിൽ {
            *pc = GetPC(o);
        }
    }

    } _catch: തിരികെ result;
}

സ്ഥിതം
M3Result Compile_Select (IM3Compilation o, m3opcode_t i_opcode)
{
    M3Result result = m3Err_none;

    u16 slots[3] = { c_slotUnused, c_slotUnused, c_slotUnused };

    IM3Operation op = ശൂന്യ;

    m3type_t type = Get_അട്ടി_Type_From_മുകൾ(o, 1); // get type of selection

    എങ്കിൽ (not Is_അട്ടി_Polymorphic(o)) {
        // Spec: the condition operand (top) must be i32
        m3type_t condType = Get_അട്ടി_Type_From_മുകൾ(o, 0);
        _throwif(m3Err_typeMismatch, condType != c_m3Type_none and condType != c_m3Type_i32);

        // Spec: the two value operands (below condition) must have matching types
        m3type_t type2 = Get_അട്ടി_Type_From_മുകൾ(o, 2);
        _throwif(m3Err_typeMismatch, type != c_m3Type_none and type2 != c_m3Type_none and
                                       not (IsSubTypeOf(type, type2) or IsSubTypeOf(type2, type)));
    }

    എങ്കിൽ (IsFpType(type)) {
#എങ്കിൽ d_m3HasFloat
        // not consuming a fp reg, so preserve
        എങ്കിൽ (not Is_അട്ടി_മുകൾ_Minus1_In_Register(o) and
            not Is_അട്ടി_മുകൾ_Minus2_In_Register(o)) {
_           (PreserveRegisterIfOccupied(o, type));
        }

        bool selectorInReg = Is_അട്ടി_മുകൾ_In_Register(o);
        slots[0]           = Get_അട്ടി_മുകൾ_Slot_സംഖ്യ(o);
_       (എടുക്കുക(o));

        u32 opIndex = 0;

        വേണ്ടി (u32 i = 1; i <= 2; ++i) {
            എങ്കിൽ (Is_അട്ടി_മുകൾ_In_Register(o)) {
                opIndex = i;
            } അല്ലെങ്കിൽ {
                slots[i] = Get_അട്ടി_മുകൾ_Slot_സംഖ്യ(o);
            }

_           (എടുക്കുക(o));
        }

        op = c_fpSelectOps[type - c_m3Type_f32][selectorInReg][opIndex];
#അല്ലെങ്കിൽ
        _throw(m3Err_unknownOpcode);
#endif
    } അല്ലെങ്കിൽ എങ്കിൽ (IsIntType(type) or IsRefType(type)) {
        // 'sss' operation doesn't consume a register, so might have to protected its contents
        എങ്കിൽ (not Is_അട്ടി_മുകൾ_In_Register(o) and
            not Is_അട്ടി_മുകൾ_Minus1_In_Register(o) and
            not Is_അട്ടി_മുകൾ_Minus2_In_Register(o)) {
_           (PreserveRegisterIfOccupied(o, type));
        }

        u32 opIndex = 3;  // op_Select_*_sss

        വേണ്ടി (u32 i = 0; i < 3; ++i) {
            എങ്കിൽ (Is_അട്ടി_മുകൾ_In_Register(o)) {
                opIndex = i;
            } അല്ലെങ്കിൽ {
                slots[i] = Get_അട്ടി_മുകൾ_Slot_സംഖ്യ(o);
            }

_           (എടുക്കുക(o));
        }

        // a reference is a pointer-sized integer as far as select is concerned
        u32 typeIndex = IsRefType(type) ? ((M3_SIZEOF_PTR == 8) ? 1 : 0)
                                        : (u32)(type - c_m3Type_i32);

        op = c_intSelectOps[typeIndex][opIndex];
    } അല്ലെങ്കിൽ എങ്കിൽ (not Is_അട്ടി_Polymorphic(o)) {
        _throw(m3_Err_function_അട്ടി_Underrun);
    }

    EmitOp(o, op);
    വേണ്ടി (u32 i = 0; i < 3; i++) {
        എങ്കിൽ (IsValidSlot(slots[i])) {
            EmitSlotOffset(o, slots[i]);
        }
    }
_   (തള്ളുക_Register(o, type));

    _catch: തിരികെ result;
}

സ്ഥിതം
M3Result Compile_Drop (IM3Compilation o, m3opcode_t i_opcode)
{
    M3Result result = എടുക്കുക(o);
    എങ്കിൽ (d_m3_Log_Wasm_അട്ടി) {
        dump_type_അട്ടി(o);
    }
    തിരികെ result;
}

സ്ഥിതം
M3Result Compile_Nop (IM3Compilation o, m3opcode_t i_opcode)
{
    തിരികെ m3Err_none;
}

സ്ഥിതം
M3Result Compile_Unreachable (IM3Compilation o, m3opcode_t i_opcode)
{
    M3Result result;

_   (AddTrapRecord(o));

_   (EmitOp(o, op_Unreachable));
_   (Set_അട്ടി_Polymorphic(o));

    _catch:
    തിരികെ result;
}


// OPTZ: currently all stack slot indices take up a full word, but
// dual stack source operands could be packed together
സ്ഥിതം
M3Result Compile_Operator (IM3Compilation o, m3opcode_t i_opcode)
{
    M3Result result;

    // Declared before the first throw below, which jumps past this point to
    // _catch and so must not skip an initialization.
    IM3Operation op = ശൂന്യ;

    IM3OpInfo opInfo = GetOpInfo(i_opcode);
    _throwif(m3Err_unknownOpcode, not opInfo);

    // Spec: validate operand types for load/store operations
    എങ്കിൽ (not Is_അട്ടി_Polymorphic(o)) {
        // For load ops (stackOffset == 0, unary), the operand is always i32 (address)
        എങ്കിൽ (i_opcode >= 0x28 and i_opcode <= 0x35) {
            m3type_t മുകൾ_Type = Get_അട്ടി_മുകൾ_Type(o);
            _throwif(m3Err_typeMismatch, മുകൾ_Type != c_m3Type_none and മുകൾ_Type != c_m3Type_i32);
        }

        // For store ops (stackOffset == -2), the address operand must be i32
        എങ്കിൽ (i_opcode >= 0x36 and i_opcode <= 0x3e) {
            m3type_t addrType = Get_അട്ടി_Type_From_മുകൾ(o, 1);
            _throwif(m3Err_typeMismatch, addrType != c_m3Type_none and addrType != c_m3Type_i32);
        }
    }

    // This preserve is for for FP compare operations.
    // either need additional slot destination operations or the
    // easy fix, move _r0 out of the way.
    // moving out the way might be the optimal solution most often?
    // otherwise, the _r0 reg can get buried down in the stack
    // and be idle & wasted for a moment.
    എങ്കിൽ (M3_UNLIKELY(IsFpType(Get_അട്ടി_മുകൾ_Type(o)) and IsIntType(opInfo->type))) {
_       (PreserveRegisterIfOccupied(o, opInfo->type));
    }

    എങ്കിൽ (opInfo->അട്ടി_Offset == 0) {
        എങ്കിൽ (Is_അട്ടി_മുകൾ_In_Register(o)) {
            op = opInfo->operations[0]; // _s
        } അല്ലെങ്കിൽ {
_           (PreserveRegisterIfOccupied(o, opInfo->type));
            op = opInfo->operations[1]; // _r
        }
    } അല്ലെങ്കിൽ {
        എങ്കിൽ (Is_അട്ടി_മുകൾ_In_Register(o)) {
            op = opInfo->operations[0];  // _rs

            എങ്കിൽ (Is_അട്ടി_മുകൾ_Minus1_In_Register(o)) {    d_m3Assert (i_opcode == c_waOp_store_f32 or i_opcode == c_waOp_store_f64);
                op = opInfo->operations[3]; // _rr for fp.store
            }
        } അല്ലെങ്കിൽ എങ്കിൽ (Is_അട്ടി_മുകൾ_Minus1_In_Register(o)) {
            op = opInfo->operations[1]; // _sr

            എങ്കിൽ (not op) {  // must be commutative, then
                op = opInfo->operations[0];
            }
        } അല്ലെങ്കിൽ {
_           (PreserveRegisterIfOccupied(o, opInfo->type));     // _ss
            op = opInfo->operations[2];
        }
    }

    എങ്കിൽ (M3_LIKELY(op)) {
_       (EmitOp(o, op));

#എങ്കിൽ d_m3FoldSetLocal
        // the op word just written (EmitOp may have bridged pages); NULL in the page-less compile-walk mode
        pc_t patchPC = o->page ? GetPC(o) - 1 : ശൂന്യ;
#endif

_       (Emit_Slot_അക്കം_Of_അട്ടി_മുകൾ_And_എടുക്കുക(o));

        എങ്കിൽ (opInfo->അട്ടി_Offset < 0) {
_           (Emit_Slot_അക്കം_Of_അട്ടി_മുകൾ_And_എടുക്കുക(o));
        }

        എങ്കിൽ (opInfo->type != c_m3Type_none) {
_           (തള്ളുക_Register(o, opInfo->type));

#എങ്കിൽ d_m3FoldSetLocal
            // record this op as a fold candidate for an immediately following local.set;
            // the operand-form index is recovered by matching against the variant table
            എങ്കിൽ (patchPC) {
                വേണ്ടി (u8 form = 0; form < 4; ++form) {
                    എങ്കിൽ (opInfo->operations[form] == op) {
                        o->foldPatchPC    = patchPC;
                        o->foldOpcode     = i_opcode;
                        o->foldForm       = form;
                        o->fold_അട്ടി_Index = (u16)Get_അട്ടി_മുകൾ_Index(o);
                        നിർത്തുക;
                    }
                }
            }
#endif
        }
    } അല്ലെങ്കിൽ {
#ifdef DEBUG
        result = ErrorCompile("no operation found for opcode", o, "'%s'", opInfo->name);
#അല്ലെങ്കിൽ
        result = ErrorCompile("no operation found for opcode", o, "%x", i_opcode);
#endif
        _throw(result);
    }

    _catch: തിരികെ result;
}

സ്ഥിതം
M3Result Compile_Convert (IM3Compilation o, m3opcode_t i_opcode)
{
_try {
    IM3OpInfo opInfo = GetOpInfo(i_opcode);
    _throwif(m3Err_unknownOpcode, not opInfo);

    // Spec: validate source operand type for conversion instructions
    എങ്കിൽ (not Is_അട്ടി_Polymorphic(o)) {
        u8 sourceType = c_m3Type_none;
        തിരഞ്ഞെടുപ്പ് (i_opcode)
        // clang-format off
        {
        അവസ്ഥ 0xa7:              // i32.wrap/i64
            sourceType = c_m3Type_i64; നിർത്തുക;
        അവസ്ഥ 0xa8: അവസ്ഥ 0xa9:   // i32.trunc_s/f32, i32.trunc_u/f32
        അവസ്ഥ 0xae: അവസ്ഥ 0xaf:   // i64.trunc_s/f32, i64.trunc_u/f32
        അവസ്ഥ 0xbb:              // f64.promote/f32
        അവസ്ഥ 0xbc:              // i32.reinterpret/f32
            sourceType = c_m3Type_f32; നിർത്തുക;
        അവസ്ഥ 0xaa: അവസ്ഥ 0xab:   // i32.trunc_s/f64, i32.trunc_u/f64
        അവസ്ഥ 0xb0: അവസ്ഥ 0xb1:   // i64.trunc_s/f64, i64.trunc_u/f64
        അവസ്ഥ 0xb6:              // f32.demote/f64
        അവസ്ഥ 0xbd:              // i64.reinterpret/f64
            sourceType = c_m3Type_f64; നിർത്തുക;
        അവസ്ഥ 0xac: അവസ്ഥ 0xad:   // i64.extend_s/i32, i64.extend_u/i32
        അവസ്ഥ 0xb2: അവസ്ഥ 0xb3:   // f32.convert_s/i32, f32.convert_u/i32
        അവസ്ഥ 0xb7: അവസ്ഥ 0xb8:   // f64.convert_s/i32, f64.convert_u/i32
        അവസ്ഥ 0xbe:              // f32.reinterpret/i32
            sourceType = c_m3Type_i32; നിർത്തുക;
        അവസ്ഥ 0xb4: അവസ്ഥ 0xb5:   // f32.convert_s/i64, f32.convert_u/i64
        അവസ്ഥ 0xb9: അവസ്ഥ 0xba:   // f64.convert_s/i64, f64.convert_u/i64
        അവസ്ഥ 0xbf:              // f64.reinterpret/i64
            sourceType = c_m3Type_i64; നിർത്തുക;
        സ്ഥിരസ്ഥിതി: നിർത്തുക;
        }
        // clang-format on

        എങ്കിൽ (sourceType != c_m3Type_none) {
            m3type_t മുകൾ_Type = Get_അട്ടി_മുകൾ_Type(o);
            _throwif(m3Err_typeMismatch, മുകൾ_Type != c_m3Type_none and not IsSubTypeOf(മുകൾ_Type, sourceType));
        }
    }

    bool destInSlot   = IsRegisterTypeAllocated(o, opInfo->type);
    bool sourceInSlot = Is_അട്ടി_മുകൾ_In_Slot(o);

    IM3Operation op = opInfo->operations[destInSlot * 2 + sourceInSlot];

_   (EmitOp(o, op));
_   (Emit_Slot_അക്കം_Of_അട്ടി_മുകൾ_And_എടുക്കുക(o));

    എങ്കിൽ (destInSlot) {
_       (തള്ളുക_Allocated_Slot_And_Emit_2(o, opInfo->type))
    } അല്ലെങ്കിൽ {
_       (തള്ളുക_Register(o, opInfo->type))
    }
}
    _catch: തിരികെ result;
}

#എങ്കിൽ d_m3HasWideArithmetic

// The wide arithmetic proposal: i64.add128 and i64.sub128 over two 128-bit values
// given as i64 halves, and i64.mul_wide_s/u giving the full product of two i64s.
// Each gives back a 128-bit result the same way, low half first. The operands are
// all read from slots, and of the two results the high half, which ends up on top,
// goes to _r0 and the low half to a newly allocated slot - so every operation has
// the one form, whatever shape the stack was in.
സ്ഥിതം
M3Result Compile_WideArithmetic (IM3Compilation o, m3opcode_t i_opcode)
{
    M3Result result = m3Err_none;

    IM3Operation op          = op_i64_Add128;
    u32          അക്കം_Operands = 4;

    തിരഞ്ഞെടുപ്പ് (i_opcode)
    // clang-format off
    {
    അവസ്ഥ c_waOp_i64Sub128:   op = op_i64_Sub128;                          നിർത്തുക;
    അവസ്ഥ c_waOp_i64MulWideS: op = op_i64_MultiplyWide; അക്കം_Operands = 2;   നിർത്തുക;
    അവസ്ഥ c_waOp_i64MulWideU: op = op_u64_MultiplyWide; അക്കം_Operands = 2;   നിർത്തുക;
    സ്ഥിരസ്ഥിതി: നിർത്തുക;
    }
    // clang-format on

_   (PreserveRegisterIfOccupied(o, c_m3Type_i64));

_   (EmitOp(o, op));

    // the top of the stack first: the high half of the second operand, for the
    // 128-bit ones
    വേണ്ടി (u32 i = 0; i < അക്കം_Operands; ++i) {
_       (Emit_Slot_അക്കം_Of_അട്ടി_മുകൾ_And_എടുക്കുക(o));
    }

_   (തള്ളുക_Allocated_Slot_And_Emit_2(o, c_m3Type_i64));      // the low half
_   (തള്ളുക_Register(o, c_m3Type_i64));                  // the high half

    _catch: തിരികെ result;
}

#endif // d_m3HasWideArithmetic

#എങ്കിൽ d_m3HasMemory64

// Replaces the 64-bit address operand of a load or store with the checked
// 32-bit effective address op_CheckAddr64 leaves behind, so that the access
// itself compiles exactly as it would against a 32-bit memory: one slot to read
// the address from, and an offset of zero folded in already.
//
// Where the address sits under other operands - the value of a store, the
// operands of an atomic - it is rewritten where it stands rather than popped:
// its stack entry is repointed at the scratch slot and retyped, and the slot it
// used to occupy released. i_numOperands counts the address and all above it.
സ്ഥിതം
M3Result EmitCheckAddr64 (IM3Compilation o, u64 i_offset, u16 i_അക്കം_Operands)
{
    M3Result result = m3Err_none;

    // The address has to be in a slot for op_CheckAddr64 to read it. Spilling
    // r0 wholesale also keeps the value of a store out of it, which is what
    // leaves the address as the one operand still in a register to worry about.
_   (PreserveRegisterIfOccupied(o, c_m3Type_i64));

    // unreachable code: the operands the instruction names may not be there
    എങ്കിൽ (o->അട്ടി_Index < o->block.block_അട്ടി_Index + i_അക്കം_Operands) {
        തിരികെ result;
    }

    {
        u16      അട്ടി_Index = (u16)(o->അട്ടി_Index - i_അക്കം_Operands);
        u16      srcSlot    = o->wasm_അട്ടി[അട്ടി_Index];
        m3type_t srcType    = o->type_അട്ടി[അട്ടി_Index];

        u16 dstSlot = c_slotUnused;

        // Checked here rather than left to the validator, which a build may
        // have switched off: reading a slot as an i64 that only holds an i32
        // would reach past what was allocated for it.
        _throwif(m3Err_typeMismatch, BaseTypeOf(srcType) != c_m3Type_i64);

        // an address is an integer, so nothing should be left in a register
        // once r0 has been preserved
        _throwif(m3Err_typeMismatch, IsRegisterSlotAlias(srcSlot));
        _throwif(m3_Err_function_അട്ടി_Underrun, srcSlot >= o->slotMaxAllocatedIndexPlusOne);

        // allocated before the source is released, so the two cannot be the
        // same slot - op_CheckAddr64 reads one and writes the other
_       (AllocateSlots(o, &dstSlot, c_m3Type_i32));

_       (EmitOp(o, op_CheckAddr64));
        EmitSlotOffset(o, srcSlot);
        EmitSlotOffset(o, dstSlot);
        EmitConstant64(o, i_offset);

        o->wasm_അട്ടി[അട്ടി_Index] = dstSlot;
        o->type_അട്ടി[അട്ടി_Index] = c_m3Type_i32;

        // locals and the constant table outlive the instruction; a dynamic
        // slot the address was computed into does not
        എങ്കിൽ (srcSlot >= o->slotFirstDynamicIndex) {
            DeallocateSlot(o, srcSlot, srcType);
        }
    }

    _catch: തിരികെ result;
}

#endif // d_m3HasMemory64


സ്ഥിതം
M3Result Compile_Load_Store (IM3Compilation o, m3opcode_t i_opcode)
{
_try {
    u32 alignHint, memoryIdx;
    u64 memoryOffset;

    // alignHint is checked by the validator
_   (ReadMemoryArg(&alignHint, &memoryIdx, &memoryOffset, &o->wasm, o->wasmEnd));
                                                                        m3log (compile, d_indent " (memory = %d; offset = %llu)", get_indention_string (o), memoryIdx, (unsigned long long) memoryOffset);
    _throwif(m3Err_unknownMemory, memoryIdx >= o->module->അക്കം_Memories);

    IM3OpInfo opInfo = GetOpInfo(i_opcode);
    _throwif(m3Err_unknownOpcode, not opInfo);

    bool isMemory64 = o->module->memories[memoryIdx]->isMemory64;

    // Spec: the static offset has to be in range of the address type. Checked
    // here as well as in the validator, so that a build without one still
    // refuses the module rather than silently truncating the offset.
    _throwif(m3Err_wasmMalformed, not isMemory64 and memoryOffset > 0xFFFFFFFFull);

    എങ്കിൽ (IsFpType(opInfo->type)) {
_       (PreserveRegisterIfOccupied(o, c_m3Type_f64));
    }

    എങ്കിൽ (memoryIdx) {
_       (EmitSetMemory(o, memoryIdx));
    }

#എങ്കിൽ d_m3HasMemory64
    എങ്കിൽ (isMemory64) {
        // An offset this far out puts every address out of bounds, so rather
        // than carrying it, clamp it to the limit: op_CheckAddr64 then traps
        // on whatever address it is given, which is the outcome either way.
        എങ്കിൽ (memoryOffset > d_m3AddressLimit) {
            memoryOffset = d_m3AddressLimit;
        }

_       (EmitCheckAddr64(o, memoryOffset, opInfo->അട്ടി_Offset < 0 ? 2 : 1));

        // op_CheckAddr64 has folded the offset in already
        memoryOffset = 0;
    }
#endif

_   (Compile_Operator(o, i_opcode));

    EmitConstant32(o, (u32)memoryOffset);

    എങ്കിൽ (memoryIdx) {
_       (EmitSetMemory(o, 0));
    }
}
    _catch: തിരികെ result;
}


#എങ്കിൽ d_m3HasAtomics

// What an atomic instruction takes and gives back, from its sub-opcode. The
// accesses come in runs of seven, one per combination of value type and cell width:
// i32/32, i64/64, i32/8, i32/16, i64/8, i64/16, i64/32. Those runs are told apart by
// where they start.
//
// The operand count includes the address. The top operand is the one that ends up
// in a register, so its type is the one to check on the way out; a store has no
// result, which is c_m3Type_none.
സ്ഥിതം
ശൂന്യം GetAtomicShape (u32 i_sub, u16* o_അക്കം_Operands, m3type_t* o_മുകൾ_Type, m3type_t* o_resultType)
{
    // clang-format off
    സ്ഥിതം സ്ഥിരം m3type_t c_മൂല്യം_Types [7] = {
        c_m3Type_i32, c_m3Type_i64,
        c_m3Type_i32, c_m3Type_i32,
        c_m3Type_i64, c_m3Type_i64, c_m3Type_i64
    };
    // clang-format on

    എങ്കിൽ (i_sub == 0x00) {                    // memory.atomic.notify: address, count
        *o_അക്കം_Operands = 2;
        *o_മുകൾ_Type     = c_m3Type_i32;
        *o_resultType  = c_m3Type_i32;
    } അല്ലെങ്കിൽ എങ്കിൽ (i_sub <= 0x02) {             // memory.atomic.wait32/64: address, expected, timeout
        *o_അക്കം_Operands = 3;
        *o_മുകൾ_Type     = c_m3Type_i64;
        *o_resultType  = c_m3Type_i32;
    } അല്ലെങ്കിൽ എങ്കിൽ (i_sub <= 0x16) {             // loads: address
        *o_അക്കം_Operands = 1;
        *o_മുകൾ_Type     = c_m3Type_i32;
        *o_resultType  = c_മൂല്യം_Types[i_sub - 0x10];
    } അല്ലെങ്കിൽ എങ്കിൽ (i_sub <= 0x1d) {             // stores: address, value
        *o_അക്കം_Operands = 2;
        *o_മുകൾ_Type     = c_മൂല്യം_Types[i_sub - 0x17];
        *o_resultType  = c_m3Type_none;
    } അല്ലെങ്കിൽ എങ്കിൽ (i_sub <= 0x47) {             // read-modify-write: address, value
        *o_അക്കം_Operands = 2;
        *o_മുകൾ_Type     = c_മൂല്യം_Types[(i_sub - 0x1e) % 7];
        *o_resultType  = *o_മുകൾ_Type;
    } അല്ലെങ്കിൽ {                                // cmpxchg: address, expected, replacement
        *o_അക്കം_Operands = 3;
        *o_മുകൾ_Type     = c_മൂല്യം_Types[i_sub - 0x48];
        *o_resultType  = *o_മുകൾ_Type;
    }
}


// The description an access takes as its last immediate: see m3_exec_atomic.h
സ്ഥിതം
u32 GetAtomicDesc (u32 i_sub)
{
    // per variant, the log2 of the width of the cell
    സ്ഥിതം സ്ഥിരം u8 c_log2Widths[7] = { 2, 3, 0, 1, 0, 1, 2 };

    u32 variant, kind = 0;

    എങ്കിൽ (i_sub <= 0x16) {
        variant = i_sub - 0x10;
    } അല്ലെങ്കിൽ എങ്കിൽ (i_sub <= 0x1d) {
        variant = i_sub - 0x17;
    } അല്ലെങ്കിൽ എങ്കിൽ (i_sub <= 0x47) {
        variant = (i_sub - 0x1e) % 7;
        kind    = (i_sub - 0x1e) / 7;         // add, sub, and, or, xor, xchg
    } അല്ലെങ്കിൽ {
        variant = i_sub - 0x48;
    }

    // variants 1 and 4..6 are the i64 ones
    u32 is64 = (variant == 1 or variant >= 4) ? 1u : 0u;

    തിരികെ c_log2Widths[variant] | (is64 << 2) | (kind << 3);
}


// Every atomic access is one op, whose operands are the topmost in _r0 and the rest
// in slots, topmost first. The address is always the deepest, so it is the last slot
// (or, for a load, the register), followed by the memarg offset. The ops check the
// address themselves: for an atomic that includes its alignment, which only the
// effective address can be asked.
സ്ഥിതം
M3Result Compile_Atomic (IM3Compilation o, m3opcode_t i_opcode)
{
_try {
    u32 alignHint, memoryIdx;
    u64 memoryOffset;

    u16      അക്കം_Operands = 0;
    m3type_t മുകൾ_Type     = c_m3Type_none;
    m3type_t resultType  = c_m3Type_none;

    എങ്കിൽ (i_opcode == c_waOp_atomicFence) {
        // the reserved byte, which the validator has checked to be zero. There
        // is nothing to order between accesses that one thread makes in sequence.
        u8 reserved;
_       (Read_u8(&reserved, &o->wasm, o->wasmEnd));
    } അല്ലെങ്കിൽ {
        // alignHint is checked by the validator
_       (ReadMemoryArg(&alignHint, &memoryIdx, &memoryOffset, &o->wasm, o->wasmEnd));
                                                                        m3log (compile, d_indent " (memory = %d; offset = %llu)", get_indention_string (o), memoryIdx, (unsigned long long) memoryOffset);
        _throwif(m3Err_unknownMemory, memoryIdx >= o->module->അക്കം_Memories);

        IM3OpInfo opInfo = GetOpInfo(i_opcode);
        _throwif(m3Err_unknownOpcode, not opInfo);

        bool isMemory64 = o->module->memories[memoryIdx]->isMemory64;

        // see Compile_Load_Store
        _throwif(m3Err_wasmMalformed, not isMemory64 and memoryOffset > 0xFFFFFFFFull);

        GetAtomicShape(i_opcode & 0xFF, &അക്കം_Operands, &മുകൾ_Type, &resultType);

        എങ്കിൽ (memoryIdx) {
_           (EmitSetMemory(o, memoryIdx));
        }

#  എങ്കിൽ d_m3HasMemory64
        എങ്കിൽ (isMemory64) {
            // see Compile_Load_Store
            എങ്കിൽ (memoryOffset > d_m3AddressLimit) {
                memoryOffset = d_m3AddressLimit;
            }

_           (EmitCheckAddr64(o, memoryOffset, അക്കം_Operands));
            memoryOffset = 0;
        }
#  endif

_       (Copy_അട്ടി_മുകൾ_To_Register(o, false));
_       (EmitOp(o, opInfo->operations[0]));
_       (എടുക്കുക_Type(o, മുകൾ_Type));

        വേണ്ടി (u16 i = 1; i < അക്കം_Operands; ++i) {
_           (Emit_Slot_അക്കം_Of_അട്ടി_മുകൾ_And_എടുക്കുക(o));
        }

        EmitConstant32(o, (u32)memoryOffset);
        എങ്കിൽ (i_opcode >= c_waOp_atomicFirstAccess) {
            EmitConstant32(o, GetAtomicDesc(i_opcode & 0xFF));
        }

        എങ്കിൽ (memoryIdx) {
_           (EmitSetMemory(o, 0));
        }

        എങ്കിൽ (resultType != c_m3Type_none) {
_           (തള്ളുക_Register(o, resultType));
        }
    }
}
    _catch: തിരികെ result;
}

#endif // d_m3HasAtomics


M3Result CompileRawFunction (IM3Module io_module, IM3Function io_function, സ്ഥിരം ശൂന്യം* i_function, സ്ഥിരം ശൂന്യം* i_userdata)
{
    d_m3Assert(io_module->runtime);

    IM3CodePage page = AcquireCodePageWithCapacity(io_module->runtime, 4);

    എങ്കിൽ (page) {
        io_function->compiled = GetPagePC(page);
        io_function->module   = io_module;

        // Unless a host module's ABI names a memory (m3_BindImportMemory), a
        // host function addresses memory 0 - what a bare i32 guest pointer means
        io_function->hostMemory = io_module->memory0;

        EmitWord(page, op_CallRawFunction);
        EmitWord(page, i_function);
        EmitWord(page, io_function);
        EmitWord(page, i_userdata);

        ReleaseCodePage(io_module->runtime, page);
        തിരികെ m3Err_none;
    } അല്ലെങ്കിൽ {
        തിരികെ m3Err_mallocFailedCodePage;
    }
}


// A table entry names every field it sets, so the struct can order them for
// packing; the columns follow that order, since M3OP names them positionally.
//
// d_logOp, d_logOp2 macros aren't actually used by the compiler, just codepage decoding (d_m3LogCodePages = 1)
#define d_ops(...)                          .operations = { __VA_ARGS__ }

#define d_logOp(OP)                         d_ops( op_##OP,                 ശൂന്യ,                       ശൂന്യ,                       ശൂന്യ )
#define d_logOp2(OP1,OP2)                   d_ops( op_##OP1,                op_##OP2,                   ശൂന്യ,                       ശൂന്യ )

#define d_emptyOpList                       d_ops( ശൂന്യ,                    ശൂന്യ,                       ശൂന്യ,                       ശൂന്യ )
#define d_unaryOpList(TYPE, NAME)           d_ops( op_##TYPE##_##NAME##_r,  op_##TYPE##_##NAME##_s,     ശൂന്യ,                       ശൂന്യ )
#define d_binOpList(TYPE, NAME)             d_ops( op_##TYPE##_##NAME##_rs, op_##TYPE##_##NAME##_sr,    op_##TYPE##_##NAME##_ss,    ശൂന്യ )
#define d_storeFpOpList(TYPE, NAME)         d_ops( op_##TYPE##_##NAME##_rs, op_##TYPE##_##NAME##_sr,    op_##TYPE##_##NAME##_ss,    op_##TYPE##_##NAME##_rr )
#define d_commutativeBinOpList(TYPE, NAME)  d_ops( op_##TYPE##_##NAME##_rs, ശൂന്യ,                       op_##TYPE##_##NAME##_ss,    ശൂന്യ )

#define d_convertOpList(OP)                 d_ops( op_##OP##_r_r,           op_##OP##_r_s,              op_##OP##_s_r,              op_##OP##_s_s )


// Every M3Compiler the tables below name, as one list, so that the index enum and
// the array of pointers can never drift apart. Each entry must carry the same
// config guard as the function's definition.
#define d_m3CompilerList(_)             \
    _( Compile_Unreachable )            \
    _( Compile_Nop )                    \
    _( Compile_LoopOrBlock )            \
    _( Compile_If )                     \
    _( Compile_End )                    \
    _( Compile_Branch )                 \
    _( Compile_BranchTable )            \
    _( Compile_Return )                 \
    _( Compile_Call )                   \
    _( Compile_CallIndirect )           \
    _( Compile_Drop )                   \
    _( Compile_Select )                 \
    _( Compile_GetLocal )               \
    _( Compile_SetLocal )               \
    _( Compile_GetSetGlobal )           \
    _( Compile_Load_Store )             \
    _( Compile_Memory_Size )            \
    _( Compile_Memory_Grow )            \
    _( Compile_Memory_Init )            \
    _( Compile_Memory_CopyFill )        \
    _( Compile_Data_Drop )              \
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
    d_m3_Compiler_List_അട്ടി_Switching( _ )

#എങ്കിൽ d_m3ImplementFloat
#  define d_m3CompilerList_f(_)         \
    _( Compile_Const_f32 )              \
    _( Compile_Const_f64 )
#അല്ലെങ്കിൽ
#  define d_m3CompilerList_f(_)
#endif

#എങ്കിൽ d_m3HasRefTypes
#  define d_m3CompilerList_refTypes(_)  \
    _( Compile_Select_Typed )           \
    _( Compile_Ref_Null )               \
    _( Compile_Ref_IsNull )             \
    _( Compile_Ref_Func )               \
    _( Compile_Table_GetSet )           \
    _( Compile_Table_Init )             \
    _( Compile_Table_Copy )             \
    _( Compile_Table_Size )             \
    _( Compile_Table_GrowFill )         \
    _( Compile_Elem_Drop )
#അല്ലെങ്കിൽ
#  define d_m3CompilerList_refTypes(_)
#endif

#എങ്കിൽ d_m3HasTypedRefs
#  define d_m3CompilerList_typedRefs(_) \
    _( Compile_CallRef )                \
    _( Compile_Ref_AsNonNull )
#അല്ലെങ്കിൽ
#  define d_m3CompilerList_typedRefs(_)
#endif

#എങ്കിൽ d_m3HasExceptionHandling
#  define d_m3CompilerList_eh(_)        \
    _( Compile_Throw )                  \
    _( Compile_ThrowRef )               \
    _( Compile_TryTable )
#അല്ലെങ്കിൽ
#  define d_m3CompilerList_eh(_)
#endif

#എങ്കിൽ d_m3HasAtomics
#  define d_m3CompilerList_atomics(_)   \
    _( Compile_Atomic )
#അല്ലെങ്കിൽ
#  define d_m3CompilerList_atomics(_)
#endif

#എങ്കിൽ d_m3HasWideArithmetic
#  define d_m3CompilerList_wide(_)      \
    _( Compile_WideArithmetic )
#അല്ലെങ്കിൽ
#  define d_m3CompilerList_wide(_)
#endif

#എങ്കിൽ d_m3_Has_അട്ടി_Switching
#  define d_m3_Compiler_List_അട്ടി_Switching(_) \
    _( Compile_ContNew )                     \
    _( Compile_ContBind )                    \
    _( Compile_Suspend )                     \
    _( Compile_Resume )                      \
    _( Compile_Switch )
#അല്ലെങ്കിൽ
#  define d_m3_Compiler_List_അട്ടി_Switching(_)
#endif

// Index 0 is "this opcode has no compiler", so that a zeroed entry - which is
// what M3OP_RESERVED and the reserved holes in the tables are - reads as one.
എണ്ണൽ {
    c_cc_NULL = 0,
#define d_m3CompilerEnum(NAME)  c_cc_##NAME,
    d_m3CompilerList(d_m3CompilerEnum)
#undef d_m3CompilerEnum
};

സ്ഥിതം സ്ഥിരം M3Compiler c_compilers[] = {
    ശൂന്യ,
#define d_m3CompilerPtr(NAME)   NAME,
    d_m3CompilerList(d_m3CompilerPtr)
#undef d_m3CompilerPtr
};

// Names a table entry's compiler by index. d_cc (NULL) is the no-compiler entry.
#define d_cc(NAME)                          .compiler = c_cc_##NAME

സ്ഥിതം
M3Compiler GetCompiler (IM3OpInfo i_opInfo)
{
    d_m3Assert(i_opInfo->compiler < M3_എണ്ണം_OF(c_compilers));
    തിരികെ c_compilers[i_opInfo->compiler];
}

// clang-format off
സ്ഥിരം M3OpInfo c_operations[] =
{
    M3OP( "unreachable",           0,  none,   d_cc(Compile_Unreachable),  d_logOp (Unreachable) ),                   // 0x00
    M3OP( "nop",                   0,  none,   d_cc(Compile_Nop),          d_emptyOpList ),                           // 0x01 .
    M3OP( "block",                 0,  none,   d_cc(Compile_LoopOrBlock),  d_emptyOpList ),                           // 0x02
    M3OP( "loop",                  0,  none,   d_cc(Compile_LoopOrBlock),  d_logOp (Loop) ),                          // 0x03
    M3OP( "if",                    -1, none,   d_cc(Compile_If),           d_emptyOpList ),                           // 0x04
    M3OP( "else",                  0,  none,   d_cc(Compile_Nop),          d_emptyOpList ),                           // 0x05

    M3OP_RESERVED,  M3OP_RESERVED,                                                                                    // 0x06...0x07

#എങ്കിൽ d_m3HasExceptionHandling
    M3OP( "throw",                 0,  none,   d_cc(Compile_Throw),        d_logOp (Throw) ),                         // 0x08
    M3OP_RESERVED,                                                                                                    // 0x09
    M3OP( "throw_ref",             0,  none,   d_cc(Compile_ThrowRef),     d_logOp (ThrowRef) ),                      // 0x0a
#അല്ലെങ്കിൽ
    M3OP_RESERVED,  M3OP_RESERVED, M3OP_RESERVED,                                                                     // 0x08...0x0a
#endif

    M3OP( "end",                   0,  none,   d_cc(Compile_End),          d_emptyOpList ),                           // 0x0b
    M3OP( "br",                    0,  none,   d_cc(Compile_Branch),       d_logOp (Branch) ),                        // 0x0c
    M3OP( "br_if",                 -1, none,   d_cc(Compile_Branch),       d_logOp2 (BranchIf_r, BranchIf_s) ),       // 0x0d
    M3OP( "br_table",              -1, none,   d_cc(Compile_BranchTable),  d_logOp (BranchTable) ),                   // 0x0e
    M3OP( "return",                0,  any,    d_cc(Compile_Return),       d_logOp (Return) ),                        // 0x0f
    M3OP( "call",                  0,  any,    d_cc(Compile_Call),         d_logOp (Call) ),                          // 0x10
    M3OP( "call_indirect",         0,  any,    d_cc(Compile_CallIndirect), d_logOp (CallIndirect) ),                  // 0x11
    M3OP( "return_call",           0,  any,    d_cc(Compile_Call),         d_logOp (ReturnCall) ),                    // 0x12
    M3OP( "return_call_indirect",  0,  any,    d_cc(Compile_CallIndirect), d_logOp (ReturnCallIndirect) ),            // 0x13

#എങ്കിൽ d_m3HasTypedRefs
    M3OP( "call_ref",              0,  any,    d_cc(Compile_CallRef),      d_logOp (CallRef) ),                       // 0x14
    M3OP( "return_call_ref",       0,  any,    d_cc(Compile_CallRef),      d_logOp (ReturnCallRef) ),                 // 0x15
#അല്ലെങ്കിൽ
    M3OP_RESERVED,  M3OP_RESERVED,                                                                                    // 0x14...
#endif
    M3OP_RESERVED,  M3OP_RESERVED, M3OP_RESERVED, M3OP_RESERVED,                                                      // ...0x19

    M3OP( "drop",                  -1, none,   d_cc(Compile_Drop),         d_emptyOpList ),                           // 0x1a
    M3OP( "select",                -2, any,    d_cc(Compile_Select),       d_emptyOpList ),                           // 0x1b

#എങ്കിൽ d_m3HasRefTypes
    M3OP( "select.t",              -2, any,    d_cc(Compile_Select_Typed), d_emptyOpList ),                           // 0x1c
#അല്ലെങ്കിൽ
    M3OP_RESERVED,                                                                                                    // 0x1c
#endif
    M3OP_RESERVED,  M3OP_RESERVED,                                                                                    // 0x1d...0x1e

#എങ്കിൽ d_m3HasExceptionHandling
    M3OP( "try_table",             0,  none,   d_cc(Compile_TryTable),     d_logOp (TryTable) ),                      // 0x1f
#അല്ലെങ്കിൽ
    M3OP_RESERVED,                                                                                                    // 0x1f
#endif

    M3OP( "local.get",             1,  any,    d_cc(Compile_GetLocal),     d_emptyOpList ),                           // 0x20
    M3OP( "local.set",             1,  none,   d_cc(Compile_SetLocal),     d_emptyOpList ),                           // 0x21
    M3OP( "local.tee",             0,  any,    d_cc(Compile_SetLocal),     d_emptyOpList ),                           // 0x22
    M3OP( "global.get",            1,  none,   d_cc(Compile_GetSetGlobal), d_emptyOpList ),                           // 0x23
    M3OP( "global.set",            1,  none,   d_cc(Compile_GetSetGlobal), d_emptyOpList ),                           // 0x24

#എങ്കിൽ d_m3HasRefTypes
    M3OP( "table.get",             0,  any,    d_cc(Compile_Table_GetSet), d_emptyOpList ),                           // 0x25
    M3OP( "table.set",             -2, none,   d_cc(Compile_Table_GetSet), d_emptyOpList ),                           // 0x26
#അല്ലെങ്കിൽ
    M3OP_RESERVED,  M3OP_RESERVED,                                                                                    // 0x25...0x26
#endif
    M3OP_RESERVED,                                                                                                    // 0x27

    M3OP( "i32.load",              0,  i_32,   d_cc(Compile_Load_Store),   d_unaryOpList (i32, Load_i32) ),           // 0x28
    M3OP( "i64.load",              0,  i_64,   d_cc(Compile_Load_Store),   d_unaryOpList (i64, Load_i64) ),           // 0x29
    M3OP_F( "f32.load",            0,  f_32,   d_cc(Compile_Load_Store),   d_unaryOpList (f32, Load_f32) ),           // 0x2a
    M3OP_F( "f64.load",            0,  f_64,   d_cc(Compile_Load_Store),   d_unaryOpList (f64, Load_f64) ),           // 0x2b

    M3OP( "i32.load8_s",           0,  i_32,   d_cc(Compile_Load_Store),   d_unaryOpList (i32, Load_i8) ),            // 0x2c
    M3OP( "i32.load8_u",           0,  i_32,   d_cc(Compile_Load_Store),   d_unaryOpList (i32, Load_u8) ),            // 0x2d
    M3OP( "i32.load16_s",          0,  i_32,   d_cc(Compile_Load_Store),   d_unaryOpList (i32, Load_i16) ),           // 0x2e
    M3OP( "i32.load16_u",          0,  i_32,   d_cc(Compile_Load_Store),   d_unaryOpList (i32, Load_u16) ),           // 0x2f

    M3OP( "i64.load8_s",           0,  i_64,   d_cc(Compile_Load_Store),   d_unaryOpList (i64, Load_i8) ),            // 0x30
    M3OP( "i64.load8_u",           0,  i_64,   d_cc(Compile_Load_Store),   d_unaryOpList (i64, Load_u8) ),            // 0x31
    M3OP( "i64.load16_s",          0,  i_64,   d_cc(Compile_Load_Store),   d_unaryOpList (i64, Load_i16) ),           // 0x32
    M3OP( "i64.load16_u",          0,  i_64,   d_cc(Compile_Load_Store),   d_unaryOpList (i64, Load_u16) ),           // 0x33
    M3OP( "i64.load32_s",          0,  i_64,   d_cc(Compile_Load_Store),   d_unaryOpList (i64, Load_i32) ),           // 0x34
    M3OP( "i64.load32_u",          0,  i_64,   d_cc(Compile_Load_Store),   d_unaryOpList (i64, Load_u32) ),           // 0x35

    M3OP( "i32.store",             -2, none,   d_cc(Compile_Load_Store),   d_binOpList (i32, Store_i32) ),            // 0x36
    M3OP( "i64.store",             -2, none,   d_cc(Compile_Load_Store),   d_binOpList (i64, Store_i64) ),            // 0x37
    M3OP_F( "f32.store",           -2, none,   d_cc(Compile_Load_Store),   d_storeFpOpList (f32, Store_f32) ),        // 0x38
    M3OP_F( "f64.store",           -2, none,   d_cc(Compile_Load_Store),   d_storeFpOpList (f64, Store_f64) ),        // 0x39

    M3OP( "i32.store8",            -2, none,   d_cc(Compile_Load_Store),   d_binOpList (i32, Store_u8) ),             // 0x3a
    M3OP( "i32.store16",           -2, none,   d_cc(Compile_Load_Store),   d_binOpList (i32, Store_i16) ),            // 0x3b

    M3OP( "i64.store8",            -2, none,   d_cc(Compile_Load_Store),   d_binOpList (i64, Store_u8) ),             // 0x3c
    M3OP( "i64.store16",           -2, none,   d_cc(Compile_Load_Store),   d_binOpList (i64, Store_i16) ),            // 0x3d
    M3OP( "i64.store32",           -2, none,   d_cc(Compile_Load_Store),   d_binOpList (i64, Store_i32) ),            // 0x3e

    M3OP( "memory.size",           1,  i_32,   d_cc(Compile_Memory_Size),  d_logOp (MemSize) ),                       // 0x3f
    M3OP( "memory.grow",           1,  i_32,   d_cc(Compile_Memory_Grow),  d_logOp (MemGrow) ),                       // 0x40

    M3OP( "i32.const",             1,  i_32,   d_cc(Compile_Const_i32),    d_logOp (Const32) ),                       // 0x41
    M3OP( "i64.const",             1,  i_64,   d_cc(Compile_Const_i64),    d_logOp (Const64) ),                       // 0x42
    M3OP_F( "f32.const",           1,  f_32,   d_cc(Compile_Const_f32),    d_emptyOpList ),                           // 0x43
    M3OP_F( "f64.const",           1,  f_64,   d_cc(Compile_Const_f64),    d_emptyOpList ),                           // 0x44

    M3OP( "i32.eqz",               0,  i_32,   d_cc(ശൂന്യ),                 d_unaryOpList (i32, EqualToZero) ),        // 0x45
    M3OP( "i32.eq",                -1, i_32,   d_cc(ശൂന്യ),                 d_commutativeBinOpList (i32, Equal) ),     // 0x46
    M3OP( "i32.ne",                -1, i_32,   d_cc(ശൂന്യ),                 d_commutativeBinOpList (i32, NotEqual) ),  // 0x47
    M3OP( "i32.lt_s",              -1, i_32,   d_cc(ശൂന്യ),                 d_binOpList (i32, LessThan) ),             // 0x48
    M3OP( "i32.lt_u",              -1, i_32,   d_cc(ശൂന്യ),                 d_binOpList (u32, LessThan) ),             // 0x49
    M3OP( "i32.gt_s",              -1, i_32,   d_cc(ശൂന്യ),                 d_binOpList (i32, GreaterThan) ),          // 0x4a
    M3OP( "i32.gt_u",              -1, i_32,   d_cc(ശൂന്യ),                 d_binOpList (u32, GreaterThan) ),          // 0x4b
    M3OP( "i32.le_s",              -1, i_32,   d_cc(ശൂന്യ),                 d_binOpList (i32, LessThanOrEqual) ),      // 0x4c
    M3OP( "i32.le_u",              -1, i_32,   d_cc(ശൂന്യ),                 d_binOpList (u32, LessThanOrEqual) ),      // 0x4d
    M3OP( "i32.ge_s",              -1, i_32,   d_cc(ശൂന്യ),                 d_binOpList (i32, GreaterThanOrEqual) ),   // 0x4e
    M3OP( "i32.ge_u",              -1, i_32,   d_cc(ശൂന്യ),                 d_binOpList (u32, GreaterThanOrEqual) ),   // 0x4f

    M3OP( "i64.eqz",               0,  i_32,   d_cc(ശൂന്യ),                 d_unaryOpList (i64, EqualToZero) ),        // 0x50
    M3OP( "i64.eq",                -1, i_32,   d_cc(ശൂന്യ),                 d_commutativeBinOpList (i64, Equal) ),     // 0x51
    M3OP( "i64.ne",                -1, i_32,   d_cc(ശൂന്യ),                 d_commutativeBinOpList (i64, NotEqual) ),  // 0x52
    M3OP( "i64.lt_s",              -1, i_32,   d_cc(ശൂന്യ),                 d_binOpList (i64, LessThan) ),             // 0x53
    M3OP( "i64.lt_u",              -1, i_32,   d_cc(ശൂന്യ),                 d_binOpList (u64, LessThan) ),             // 0x54
    M3OP( "i64.gt_s",              -1, i_32,   d_cc(ശൂന്യ),                 d_binOpList (i64, GreaterThan) ),          // 0x55
    M3OP( "i64.gt_u",              -1, i_32,   d_cc(ശൂന്യ),                 d_binOpList (u64, GreaterThan) ),          // 0x56
    M3OP( "i64.le_s",              -1, i_32,   d_cc(ശൂന്യ),                 d_binOpList (i64, LessThanOrEqual) ),      // 0x57
    M3OP( "i64.le_u",              -1, i_32,   d_cc(ശൂന്യ),                 d_binOpList (u64, LessThanOrEqual) ),      // 0x58
    M3OP( "i64.ge_s",              -1, i_32,   d_cc(ശൂന്യ),                 d_binOpList (i64, GreaterThanOrEqual) ),   // 0x59
    M3OP( "i64.ge_u",              -1, i_32,   d_cc(ശൂന്യ),                 d_binOpList (u64, GreaterThanOrEqual) ),   // 0x5a

    M3OP_F( "f32.eq",              -1, i_32,   d_cc(ശൂന്യ),                 d_commutativeBinOpList (f32, Equal) ),     // 0x5b
    M3OP_F( "f32.ne",              -1, i_32,   d_cc(ശൂന്യ),                 d_commutativeBinOpList (f32, NotEqual) ),  // 0x5c
    M3OP_F( "f32.lt",              -1, i_32,   d_cc(ശൂന്യ),                 d_binOpList (f32, LessThan) ),             // 0x5d
    M3OP_F( "f32.gt",              -1, i_32,   d_cc(ശൂന്യ),                 d_binOpList (f32, GreaterThan) ),          // 0x5e
    M3OP_F( "f32.le",              -1, i_32,   d_cc(ശൂന്യ),                 d_binOpList (f32, LessThanOrEqual) ),      // 0x5f
    M3OP_F( "f32.ge",              -1, i_32,   d_cc(ശൂന്യ),                 d_binOpList (f32, GreaterThanOrEqual) ),   // 0x60

    M3OP_F( "f64.eq",              -1, i_32,   d_cc(ശൂന്യ),                 d_commutativeBinOpList (f64, Equal) ),     // 0x61
    M3OP_F( "f64.ne",              -1, i_32,   d_cc(ശൂന്യ),                 d_commutativeBinOpList (f64, NotEqual) ),  // 0x62
    M3OP_F( "f64.lt",              -1, i_32,   d_cc(ശൂന്യ),                 d_binOpList (f64, LessThan) ),             // 0x63
    M3OP_F( "f64.gt",              -1, i_32,   d_cc(ശൂന്യ),                 d_binOpList (f64, GreaterThan) ),          // 0x64
    M3OP_F( "f64.le",              -1, i_32,   d_cc(ശൂന്യ),                 d_binOpList (f64, LessThanOrEqual) ),      // 0x65
    M3OP_F( "f64.ge",              -1, i_32,   d_cc(ശൂന്യ),                 d_binOpList (f64, GreaterThanOrEqual) ),   // 0x66

    M3OP( "i32.clz",               0,  i_32,   d_cc(ശൂന്യ),                 d_unaryOpList (u32, Clz) ),                // 0x67
    M3OP( "i32.ctz",               0,  i_32,   d_cc(ശൂന്യ),                 d_unaryOpList (u32, Ctz) ),                // 0x68
    M3OP( "i32.popcnt",            0,  i_32,   d_cc(ശൂന്യ),                 d_unaryOpList (u32, Popcnt) ),             // 0x69

    M3OP( "i32.add",               -1, i_32,   d_cc(ശൂന്യ),                 d_commutativeBinOpList (i32, Add) ),       // 0x6a
    M3OP( "i32.sub",               -1, i_32,   d_cc(ശൂന്യ),                 d_binOpList (i32, Subtract) ),             // 0x6b
    M3OP( "i32.mul",               -1, i_32,   d_cc(ശൂന്യ),                 d_commutativeBinOpList (i32, Multiply) ),  // 0x6c
    M3OP( "i32.div_s",             -1, i_32,   d_cc(ശൂന്യ),                 d_binOpList (i32, Divide) ),               // 0x6d
    M3OP( "i32.div_u",             -1, i_32,   d_cc(ശൂന്യ),                 d_binOpList (u32, Divide) ),               // 0x6e
    M3OP( "i32.rem_s",             -1, i_32,   d_cc(ശൂന്യ),                 d_binOpList (i32, Remainder) ),            // 0x6f
    M3OP( "i32.rem_u",             -1, i_32,   d_cc(ശൂന്യ),                 d_binOpList (u32, Remainder) ),            // 0x70
    M3OP( "i32.and",               -1, i_32,   d_cc(ശൂന്യ),                 d_commutativeBinOpList (u32, And) ),       // 0x71
    M3OP( "i32.or",                -1, i_32,   d_cc(ശൂന്യ),                 d_commutativeBinOpList (u32, Or) ),        // 0x72
    M3OP( "i32.xor",               -1, i_32,   d_cc(ശൂന്യ),                 d_commutativeBinOpList (u32, Xor) ),       // 0x73
    M3OP( "i32.shl",               -1, i_32,   d_cc(ശൂന്യ),                 d_binOpList (u32, Shift_ഇടത്) ),            // 0x74
    M3OP( "i32.shr_s",             -1, i_32,   d_cc(ശൂന്യ),                 d_binOpList (i32, Shift_വലത്) ),           // 0x75
    M3OP( "i32.shr_u",             -1, i_32,   d_cc(ശൂന്യ),                 d_binOpList (u32, Shift_വലത്) ),           // 0x76
    M3OP( "i32.rotl",              -1, i_32,   d_cc(ശൂന്യ),                 d_binOpList (u32, Rotl) ),                 // 0x77
    M3OP( "i32.rotr",              -1, i_32,   d_cc(ശൂന്യ),                 d_binOpList (u32, Rotr) ),                 // 0x78

    M3OP( "i64.clz",               0,  i_64,   d_cc(ശൂന്യ),                 d_unaryOpList (u64, Clz) ),                // 0x79
    M3OP( "i64.ctz",               0,  i_64,   d_cc(ശൂന്യ),                 d_unaryOpList (u64, Ctz) ),                // 0x7a
    M3OP( "i64.popcnt",            0,  i_64,   d_cc(ശൂന്യ),                 d_unaryOpList (u64, Popcnt) ),             // 0x7b

    M3OP( "i64.add",               -1, i_64,   d_cc(ശൂന്യ),                 d_commutativeBinOpList (i64, Add) ),       // 0x7c
    M3OP( "i64.sub",               -1, i_64,   d_cc(ശൂന്യ),                 d_binOpList (i64, Subtract) ),             // 0x7d
    M3OP( "i64.mul",               -1, i_64,   d_cc(ശൂന്യ),                 d_commutativeBinOpList (i64, Multiply) ),  // 0x7e
    M3OP( "i64.div_s",             -1, i_64,   d_cc(ശൂന്യ),                 d_binOpList (i64, Divide) ),               // 0x7f
    M3OP( "i64.div_u",             -1, i_64,   d_cc(ശൂന്യ),                 d_binOpList (u64, Divide) ),               // 0x80
    M3OP( "i64.rem_s",             -1, i_64,   d_cc(ശൂന്യ),                 d_binOpList (i64, Remainder) ),            // 0x81
    M3OP( "i64.rem_u",             -1, i_64,   d_cc(ശൂന്യ),                 d_binOpList (u64, Remainder) ),            // 0x82
    M3OP( "i64.and",               -1, i_64,   d_cc(ശൂന്യ),                 d_commutativeBinOpList (u64, And) ),       // 0x83
    M3OP( "i64.or",                -1, i_64,   d_cc(ശൂന്യ),                 d_commutativeBinOpList (u64, Or) ),        // 0x84
    M3OP( "i64.xor",               -1, i_64,   d_cc(ശൂന്യ),                 d_commutativeBinOpList (u64, Xor) ),       // 0x85
    M3OP( "i64.shl",               -1, i_64,   d_cc(ശൂന്യ),                 d_binOpList (u64, Shift_ഇടത്) ),            // 0x86
    M3OP( "i64.shr_s",             -1, i_64,   d_cc(ശൂന്യ),                 d_binOpList (i64, Shift_വലത്) ),           // 0x87
    M3OP( "i64.shr_u",             -1, i_64,   d_cc(ശൂന്യ),                 d_binOpList (u64, Shift_വലത്) ),           // 0x88
    M3OP( "i64.rotl",              -1, i_64,   d_cc(ശൂന്യ),                 d_binOpList (u64, Rotl) ),                 // 0x89
    M3OP( "i64.rotr",              -1, i_64,   d_cc(ശൂന്യ),                 d_binOpList (u64, Rotr) ),                 // 0x8a

    M3OP_F( "f32.abs",             0,  f_32,   d_cc(ശൂന്യ),                 d_unaryOpList(f32, Abs) ),                 // 0x8b
    M3OP_F( "f32.neg",             0,  f_32,   d_cc(ശൂന്യ),                 d_unaryOpList(f32, Negate) ),              // 0x8c
    M3OP_F( "f32.ceil",            0,  f_32,   d_cc(ശൂന്യ),                 d_unaryOpList(f32, Ceil) ),                // 0x8d
    M3OP_F( "f32.floor",           0,  f_32,   d_cc(ശൂന്യ),                 d_unaryOpList(f32, Floor) ),               // 0x8e
    M3OP_F( "f32.trunc",           0,  f_32,   d_cc(ശൂന്യ),                 d_unaryOpList(f32, Trunc) ),               // 0x8f
    M3OP_F( "f32.nearest",         0,  f_32,   d_cc(ശൂന്യ),                 d_unaryOpList(f32, Nearest) ),             // 0x90
    M3OP_F( "f32.sqrt",            0,  f_32,   d_cc(ശൂന്യ),                 d_unaryOpList(f32, Sqrt) ),                // 0x91

    M3OP_F( "f32.add",             -1, f_32,   d_cc(ശൂന്യ),                 d_commutativeBinOpList (f32, Add) ),       // 0x92
    M3OP_F( "f32.sub",             -1, f_32,   d_cc(ശൂന്യ),                 d_binOpList (f32, Subtract) ),             // 0x93
    M3OP_F( "f32.mul",             -1, f_32,   d_cc(ശൂന്യ),                 d_commutativeBinOpList (f32, Multiply) ),  // 0x94
    M3OP_F( "f32.div",             -1, f_32,   d_cc(ശൂന്യ),                 d_binOpList (f32, Divide) ),               // 0x95
    M3OP_F( "f32.min",             -1, f_32,   d_cc(ശൂന്യ),                 d_commutativeBinOpList (f32, Min) ),       // 0x96
    M3OP_F( "f32.max",             -1, f_32,   d_cc(ശൂന്യ),                 d_commutativeBinOpList (f32, Max) ),       // 0x97
    M3OP_F( "f32.copysign",        -1, f_32,   d_cc(ശൂന്യ),                 d_binOpList (f32, CopySign) ),             // 0x98

    M3OP_F( "f64.abs",             0,  f_64,   d_cc(ശൂന്യ),                 d_unaryOpList(f64, Abs) ),                 // 0x99
    M3OP_F( "f64.neg",             0,  f_64,   d_cc(ശൂന്യ),                 d_unaryOpList(f64, Negate) ),              // 0x9a
    M3OP_F( "f64.ceil",            0,  f_64,   d_cc(ശൂന്യ),                 d_unaryOpList(f64, Ceil) ),                // 0x9b
    M3OP_F( "f64.floor",           0,  f_64,   d_cc(ശൂന്യ),                 d_unaryOpList(f64, Floor) ),               // 0x9c
    M3OP_F( "f64.trunc",           0,  f_64,   d_cc(ശൂന്യ),                 d_unaryOpList(f64, Trunc) ),               // 0x9d
    M3OP_F( "f64.nearest",         0,  f_64,   d_cc(ശൂന്യ),                 d_unaryOpList(f64, Nearest) ),             // 0x9e
    M3OP_F( "f64.sqrt",            0,  f_64,   d_cc(ശൂന്യ),                 d_unaryOpList(f64, Sqrt) ),                // 0x9f

    M3OP_F( "f64.add",             -1, f_64,   d_cc(ശൂന്യ),                 d_commutativeBinOpList (f64, Add) ),       // 0xa0
    M3OP_F( "f64.sub",             -1, f_64,   d_cc(ശൂന്യ),                 d_binOpList (f64, Subtract) ),             // 0xa1
    M3OP_F( "f64.mul",             -1, f_64,   d_cc(ശൂന്യ),                 d_commutativeBinOpList (f64, Multiply) ),  // 0xa2
    M3OP_F( "f64.div",             -1, f_64,   d_cc(ശൂന്യ),                 d_binOpList (f64, Divide) ),               // 0xa3
    M3OP_F( "f64.min",             -1, f_64,   d_cc(ശൂന്യ),                 d_commutativeBinOpList (f64, Min) ),       // 0xa4
    M3OP_F( "f64.max",             -1, f_64,   d_cc(ശൂന്യ),                 d_commutativeBinOpList (f64, Max) ),       // 0xa5
    M3OP_F( "f64.copysign",        -1, f_64,   d_cc(ശൂന്യ),                 d_binOpList (f64, CopySign) ),             // 0xa6

    M3OP( "i32.wrap/i64",          0,  i_32,   d_cc(ശൂന്യ),                 d_unaryOpList (i32, Wrap_i64) ),           // 0xa7
    M3OP_F( "i32.trunc_s/f32",     0,  i_32,   d_cc(Compile_Convert),      d_convertOpList (i32_Trunc_f32) ),         // 0xa8
    M3OP_F( "i32.trunc_u/f32",     0,  i_32,   d_cc(Compile_Convert),      d_convertOpList (u32_Trunc_f32) ),         // 0xa9
    M3OP_F( "i32.trunc_s/f64",     0,  i_32,   d_cc(Compile_Convert),      d_convertOpList (i32_Trunc_f64) ),         // 0xaa
    M3OP_F( "i32.trunc_u/f64",     0,  i_32,   d_cc(Compile_Convert),      d_convertOpList (u32_Trunc_f64) ),         // 0xab

    M3OP( "i64.extend_s/i32",      0,  i_64,   d_cc(ശൂന്യ),                 d_unaryOpList (i64, Extend_i32) ),         // 0xac
    M3OP( "i64.extend_u/i32",      0,  i_64,   d_cc(ശൂന്യ),                 d_unaryOpList (i64, Extend_u32) ),         // 0xad

    M3OP_F( "i64.trunc_s/f32",     0,  i_64,   d_cc(Compile_Convert),      d_convertOpList (i64_Trunc_f32) ),         // 0xae
    M3OP_F( "i64.trunc_u/f32",     0,  i_64,   d_cc(Compile_Convert),      d_convertOpList (u64_Trunc_f32) ),         // 0xaf
    M3OP_F( "i64.trunc_s/f64",     0,  i_64,   d_cc(Compile_Convert),      d_convertOpList (i64_Trunc_f64) ),         // 0xb0
    M3OP_F( "i64.trunc_u/f64",     0,  i_64,   d_cc(Compile_Convert),      d_convertOpList (u64_Trunc_f64) ),         // 0xb1

    M3OP_F( "f32.convert_s/i32",   0,  f_32,   d_cc(Compile_Convert),      d_convertOpList (f32_Convert_i32) ),       // 0xb2
    M3OP_F( "f32.convert_u/i32",   0,  f_32,   d_cc(Compile_Convert),      d_convertOpList (f32_Convert_u32) ),       // 0xb3
    M3OP_F( "f32.convert_s/i64",   0,  f_32,   d_cc(Compile_Convert),      d_convertOpList (f32_Convert_i64) ),       // 0xb4
    M3OP_F( "f32.convert_u/i64",   0,  f_32,   d_cc(Compile_Convert),      d_convertOpList (f32_Convert_u64) ),       // 0xb5

    M3OP_F( "f32.demote/f64",      0,  f_32,   d_cc(ശൂന്യ),                 d_unaryOpList (f32, Demote_f64) ),         // 0xb6

    M3OP_F( "f64.convert_s/i32",   0,  f_64,   d_cc(Compile_Convert),      d_convertOpList (f64_Convert_i32) ),       // 0xb7
    M3OP_F( "f64.convert_u/i32",   0,  f_64,   d_cc(Compile_Convert),      d_convertOpList (f64_Convert_u32) ),       // 0xb8
    M3OP_F( "f64.convert_s/i64",   0,  f_64,   d_cc(Compile_Convert),      d_convertOpList (f64_Convert_i64) ),       // 0xb9
    M3OP_F( "f64.convert_u/i64",   0,  f_64,   d_cc(Compile_Convert),      d_convertOpList (f64_Convert_u64) ),       // 0xba

    M3OP_F( "f64.promote/f32",     0,  f_64,   d_cc(ശൂന്യ),                 d_unaryOpList (f64, Promote_f32) ),        // 0xbb

    M3OP_F( "i32.reinterpret/f32", 0,  i_32,   d_cc(Compile_Convert),      d_convertOpList (i32_Reinterpret_f32) ),   // 0xbc
    M3OP_F( "i64.reinterpret/f64", 0,  i_64,   d_cc(Compile_Convert),      d_convertOpList (i64_Reinterpret_f64) ),   // 0xbd
    M3OP_F( "f32.reinterpret/i32", 0,  f_32,   d_cc(Compile_Convert),      d_convertOpList (f32_Reinterpret_i32) ),   // 0xbe
    M3OP_F( "f64.reinterpret/i64", 0,  f_64,   d_cc(Compile_Convert),      d_convertOpList (f64_Reinterpret_i64) ),   // 0xbf

    M3OP( "i32.extend8_s",         0,  i_32,   d_cc(ശൂന്യ),                 d_unaryOpList (i32, Extend8_s) ),          // 0xc0
    M3OP( "i32.extend16_s",        0,  i_32,   d_cc(ശൂന്യ),                 d_unaryOpList (i32, Extend16_s) ),         // 0xc1
    M3OP( "i64.extend8_s",         0,  i_64,   d_cc(ശൂന്യ),                 d_unaryOpList (i64, Extend8_s) ),          // 0xc2
    M3OP( "i64.extend16_s",        0,  i_64,   d_cc(ശൂന്യ),                 d_unaryOpList (i64, Extend16_s) ),         // 0xc3
    M3OP( "i64.extend32_s",        0,  i_64,   d_cc(ശൂന്യ),                 d_unaryOpList (i64, Extend32_s) ),         // 0xc4

// Internal operations, for codepage logging only. They sit past c_waOp_lastCore,
// the last opcode this table covers, so GetOpInfo () can never reach them.
# ifdef DEBUG // for codepage logging. the order doesn't matter:
#define d_m3DebugOp(OP) M3OP (#OP, 0, none, d_ops (op_##OP))

# എങ്കിൽ d_m3HasFloat
#define d_m3DebugTypedOp(OP) M3OP (#OP, 0, none, d_ops (op_##OP##_i32, op_##OP##_i64, op_##OP##_f32, op_##OP##_f64))
# അല്ലെങ്കിൽ
#define d_m3DebugTypedOp(OP) M3OP (#OP, 0, none, d_ops (op_##OP##_i32, op_##OP##_i64))
# endif

    d_m3DebugOp (Compile),          d_m3DebugOp (Entry),            d_m3DebugOp (End),
    d_m3DebugOp (Unsupported),      d_m3DebugOp (CallRawFunction),

    d_m3DebugOp (GetGlobal_s32),    d_m3DebugOp (GetGlobal_s64),    d_m3DebugOp (ContinueLoop),     d_m3DebugOp (ContinueLoopIf),
# എങ്കിൽ d_m3_Has_അട്ടി_Switching
    d_m3DebugOp (ContinueLoop_Suspendable), d_m3DebugOp (ContinueLoopIf_Suspendable), d_m3DebugOp (EntryCheck),
#  എങ്കിൽ !d_m3EntryKeepsFrame
    d_m3DebugOp (Entry_Suspendable),
#  endif
# endif

    d_m3DebugOp (CopySlot_32),      d_m3DebugOp (PreserveCopySlot_32), d_m3DebugOp (If_s),          d_m3DebugOp (BranchIfPrologue_s),
    d_m3DebugOp (CopySlot_64),      d_m3DebugOp (PreserveCopySlot_64), d_m3DebugOp (If_r),          d_m3DebugOp (BranchIfPrologue_r),

    d_m3DebugOp (Select_i32_rss),   d_m3DebugOp (Select_i32_srs),   d_m3DebugOp (Select_i32_ssr),   d_m3DebugOp (Select_i32_sss),
    d_m3DebugOp (Select_i64_rss),   d_m3DebugOp (Select_i64_srs),   d_m3DebugOp (Select_i64_ssr),   d_m3DebugOp (Select_i64_sss),

# എങ്കിൽ d_m3HasFloat
    d_m3DebugOp (Select_f32_sss),   d_m3DebugOp (Select_f32_srs),   d_m3DebugOp (Select_f32_ssr),
    d_m3DebugOp (Select_f32_rss),   d_m3DebugOp (Select_f32_rrs),   d_m3DebugOp (Select_f32_rsr),

    d_m3DebugOp (Select_f64_sss),   d_m3DebugOp (Select_f64_srs),   d_m3DebugOp (Select_f64_ssr),
    d_m3DebugOp (Select_f64_rss),   d_m3DebugOp (Select_f64_rrs),   d_m3DebugOp (Select_f64_rsr),
# endif

    d_m3DebugOp (MemFill),          d_m3DebugOp (MemCopy),          d_m3DebugOp (MemInit),          d_m3DebugOp (DataDrop),

# എങ്കിൽ d_m3HasGasMetering
    d_m3DebugOp (UseGas),
# endif

# എങ്കിൽ d_m3HasRefTypes
    d_m3DebugOp (TableGet),         d_m3DebugOp (TableSet),         d_m3DebugOp (TableSize),
    d_m3DebugOp (TableGrow),        d_m3DebugOp (TableFill),        d_m3DebugOp (TableInit),
    d_m3DebugOp (ElemDrop),         d_m3DebugOp (TableCopy),
# endif

    d_m3DebugTypedOp (SetGlobal),   d_m3DebugOp (SetGlobal_s32),    d_m3DebugOp (SetGlobal_s64),

    d_m3DebugTypedOp (SetRegister), d_m3DebugTypedOp (SetSlot),     d_m3DebugTypedOp (PreserveSetSlot),
# endif

# ifdef DEBUG
    M3OP( "termination", 0, c_m3Type_unknown, d_emptyOpList ) // for find_operation_info
# endif
};

സ്ഥിരം M3OpInfo c_operationsFC[] =
{
    M3OP_F( "i32.trunc_s:sat/f32", 0, i_32,   d_cc(Compile_Convert),         d_convertOpList (i32_TruncSat_f32) ),  // 0x00
    M3OP_F( "i32.trunc_u:sat/f32", 0, i_32,   d_cc(Compile_Convert),         d_convertOpList (u32_TruncSat_f32) ),  // 0x01
    M3OP_F( "i32.trunc_s:sat/f64", 0, i_32,   d_cc(Compile_Convert),         d_convertOpList (i32_TruncSat_f64) ),  // 0x02
    M3OP_F( "i32.trunc_u:sat/f64", 0, i_32,   d_cc(Compile_Convert),         d_convertOpList (u32_TruncSat_f64) ),  // 0x03
    M3OP_F( "i64.trunc_s:sat/f32", 0, i_64,   d_cc(Compile_Convert),         d_convertOpList (i64_TruncSat_f32) ),  // 0x04
    M3OP_F( "i64.trunc_u:sat/f32", 0, i_64,   d_cc(Compile_Convert),         d_convertOpList (u64_TruncSat_f32) ),  // 0x05
    M3OP_F( "i64.trunc_s:sat/f64", 0, i_64,   d_cc(Compile_Convert),         d_convertOpList (i64_TruncSat_f64) ),  // 0x06
    M3OP_F( "i64.trunc_u:sat/f64", 0, i_64,   d_cc(Compile_Convert),         d_convertOpList (u64_TruncSat_f64) ),  // 0x07

    M3OP( "memory.init",           0, none,   d_cc(Compile_Memory_Init),     d_emptyOpList ),                       // 0x08
    M3OP( "data.drop",             0, none,   d_cc(Compile_Data_Drop),       d_emptyOpList ),                       // 0x09

    M3OP( "memory.copy",           0, none,   d_cc(Compile_Memory_CopyFill), d_emptyOpList ),                       // 0x0a
    M3OP( "memory.fill",           0, none,   d_cc(Compile_Memory_CopyFill), d_emptyOpList ),                       // 0x0b

#എങ്കിൽ d_m3HasRefTypes
    M3OP( "table.init",            0, none,   d_cc(Compile_Table_Init),      d_emptyOpList ),                       // 0x0c
    M3OP( "elem.drop",             0, none,   d_cc(Compile_Elem_Drop),       d_emptyOpList ),                       // 0x0d
    M3OP( "table.copy",            0, none,   d_cc(Compile_Table_Copy),      d_emptyOpList ),                       // 0x0e
#അല്ലെങ്കിൽ
    M3OP_RESERVED, M3OP_RESERVED, M3OP_RESERVED,                                                                    // 0x0c...0x0e
#endif

#എങ്കിൽ d_m3HasRefTypes
    M3OP( "table.grow",            0, i_32,   d_cc(Compile_Table_GrowFill),  d_emptyOpList ),                       // 0x0f
    M3OP( "table.size",            1, i_32,   d_cc(Compile_Table_Size),      d_emptyOpList ),                       // 0x10
    M3OP( "table.fill",            0, none,   d_cc(Compile_Table_GrowFill),  d_emptyOpList ),                       // 0x11
#അല്ലെങ്കിൽ
    M3OP_RESERVED, M3OP_RESERVED, M3OP_RESERVED,                                                                    // 0x0f...0x11
#endif

#എങ്കിൽ d_m3HasWideArithmetic
    M3OP_RESERVED,                                                                                                  // 0x12

    M3OP( "i64.add128",            -2, i_64,  d_cc(Compile_WideArithmetic), d_logOp (i64_Add128) ),              // 0x13
    M3OP( "i64.sub128",            -2, i_64,  d_cc(Compile_WideArithmetic), d_logOp (i64_Sub128) ),              // 0x14
    M3OP( "i64.mul_wide_s",        0,  i_64,  d_cc(Compile_WideArithmetic), d_logOp (i64_MultiplyWide) ),        // 0x15
    M3OP( "i64.mul_wide_u",        0,  i_64,  d_cc(Compile_WideArithmetic), d_logOp (u64_MultiplyWide) ),        // 0x16
#endif


# ifdef DEBUG
    M3OP( "termination", 0, c_m3Type_unknown, d_emptyOpList ) // for find_operation_info
# endif
};

#എങ്കിൽ d_m3HasAtomics

// The 0xFE instructions share one entry per kind rather than each having its own: what
// tells them apart, the width of the cell and the operation, is worked out from the
// sub-opcode by Compile_Atomic. GetAtomicInfoIndex () maps a sub-opcode onto this table.
എണ്ണൽ {
    c_opAtomic_notify,
    c_opAtomic_wait32,
    c_opAtomic_wait64,
    c_opAtomic_fence,
    c_opAtomic_load,
    c_opAtomic_store,
    c_opAtomic_rmw,
    c_opAtomic_cmpxchg
};

സ്ഥിരം M3OpInfo c_operationsFE[] =
{
    M3OP( "memory.atomic.notify",  -1, i_32,  d_cc(Compile_Atomic), d_logOp (AtomicNotify) ),    // c_opAtomic_notify
    M3OP( "memory.atomic.wait32",  -2, i_32,  d_cc(Compile_Atomic), d_logOp (AtomicWait32) ),    // c_opAtomic_wait32
    M3OP( "memory.atomic.wait64",  -2, i_32,  d_cc(Compile_Atomic), d_logOp (AtomicWait64) ),    // c_opAtomic_wait64
    M3OP( "atomic.fence",           0, none,  d_cc(Compile_Atomic), d_emptyOpList ),             // c_opAtomic_fence
    M3OP( "atomic.load",            0, any,   d_cc(Compile_Atomic), d_logOp (AtomicLoad) ),      // c_opAtomic_load
    M3OP( "atomic.store",          -2, none,  d_cc(Compile_Atomic), d_logOp (AtomicStore) ),     // c_opAtomic_store
    M3OP( "atomic.rmw",            -1, any,   d_cc(Compile_Atomic), d_logOp (AtomicRmw) ),       // c_opAtomic_rmw
    M3OP( "atomic.rmw.cmpxchg",    -2, any,   d_cc(Compile_Atomic), d_logOp (AtomicCmpxchg) ),   // c_opAtomic_cmpxchg
};

സ്ഥിരം u32 c_അക്കം_Operations_FE = M3_എണ്ണം_OF(c_operationsFE);

// The entry a 0xFE sub-opcode uses, or -1 for one that names nothing: the spec leaves
// 0x04..0x0f and everything past cmpxchg unassigned. The accesses come in runs of seven,
// one per pairing of value type and cell width.
സ്ഥിതം വരിയിൽ
i32 GetAtomicInfoIndex (u32 i_sub)
{
    എങ്കിൽ (i_sub <= 0x03) തിരികെ (i32)i_sub;
    എങ്കിൽ (i_sub < 0x10) തിരികെ -1;
    എങ്കിൽ (i_sub <= 0x16) തിരികെ c_opAtomic_load;
    എങ്കിൽ (i_sub <= 0x1d) തിരികെ c_opAtomic_store;
    എങ്കിൽ (i_sub <= 0x47) തിരികെ c_opAtomic_rmw;
    എങ്കിൽ (i_sub <= c_waOp_lastAtomic) തിരികെ c_opAtomic_cmpxchg;
    തിരികെ -1;
}

#endif // d_m3HasAtomics

// The opcodes above c_waOp_lastCore are sparse: the reference instructions at
// 0xd0..0xd2 and 0xd4, and the 0xfc extended-opcode prefix. Indexing c_operations out to
// 0xfc to reach them would leave ~50 empty entries there, so they get their own
// table, which GetHighOpIndex () maps the opcode onto.
എണ്ണൽ {
#എങ്കിൽ d_m3HasRefTypes
    c_op_മുകളിലെ_ref_Null,
    c_op_മുകളിലെ_ref_Is_Null,
    c_op_മുകളിലെ_ref_Func,
#  എങ്കിൽ d_m3HasTypedRefs
    c_op_മുകളിലെ_ref_As_Non_Null,
#  endif
#endif
#എങ്കിൽ d_m3_Has_അട്ടി_Switching
    c_op_മുകളിലെ_cont_New,
    c_op_മുകളിലെ_cont_Bind,
    c_op_മുകളിലെ_suspend,
    c_op_മുകളിലെ_resume,
    c_op_മുകളിലെ_resume_Throw,
    c_op_മുകളിലെ_resume_Throw_Ref,
    c_op_മുകളിലെ_switch,
#endif
#എങ്കിൽ d_m3HasAtomics
    c_op_മുകളിലെ_atomic,
#endif
    c_op_മുകളിലെ_extended   // always present, so the enum is never empty
};

// Positional, so the table must stay in c_opHigh_* order: array designators
// are not valid C++, which the maintenance builds compile this as.
സ്ഥിതം സ്ഥിരം M3OpInfo c_operations_മുകളിലെ[] =
{
#എങ്കിൽ d_m3HasRefTypes
    M3OP( "ref.null",        1, any,               d_cc(Compile_Ref_Null),       d_emptyOpList ),  // 0xd0  c_opHigh_refNull
    M3OP( "ref.is_null",     0, i_32,              d_cc(Compile_Ref_IsNull),     d_emptyOpList ),  // 0xd1  c_opHigh_refIsNull
    M3OP( "ref.func",        1, any,               d_cc(Compile_Ref_Func),       d_emptyOpList ),  // 0xd2  c_opHigh_refFunc
#  എങ്കിൽ d_m3HasTypedRefs
    M3OP( "ref.as_non_null", 0, any,               d_cc(Compile_Ref_AsNonNull),  d_emptyOpList ),  // 0xd4  c_opHigh_refAsNonNull
#  endif
#endif
#എങ്കിൽ d_m3_Has_അട്ടി_Switching
    M3OP( "cont.new",          1, any,             d_cc(Compile_ContNew),        d_emptyOpList ),  // 0xe0  c_opHigh_contNew
    M3OP( "cont.bind",         2, any,             d_cc(Compile_ContBind),       d_emptyOpList ),  // 0xe1  c_opHigh_contBind
    M3OP( "suspend",           1, any,             d_cc(Compile_Suspend),        d_emptyOpList ),  // 0xe2  c_opHigh_suspend
    M3OP( "resume",            1, any,             d_cc(Compile_Resume),         d_emptyOpList ),  // 0xe3  c_opHigh_resume
    M3OP( "resume_throw",      2, any,             d_cc(Compile_Resume)   ,    d_emptyOpList ),  // 0xe4  c_opHigh_resumeThrow
    M3OP( "resume_throw_ref",  1, any,             d_cc(Compile_Resume)      , d_emptyOpList ),  // 0xe5  c_opHigh_resumeThrowRef
    M3OP( "switch",            2, any,             d_cc(Compile_Switch),         d_emptyOpList ),  // 0xe6  c_opHigh_switch
#endif

#എങ്കിൽ d_m3HasAtomics
    M3OP( "0xFE",            0, c_m3Type_unknown,  d_cc(Compile_ExtendedOpcode), d_emptyOpList ),  // 0xfe  c_opHigh_atomic
#endif
    M3OP( "0xFC",            0, c_m3Type_unknown,  d_cc(Compile_ExtendedOpcode), d_emptyOpList ),  // 0xfc  c_opHigh_extended
};
M3_STATIC_ASSERT (M3_എണ്ണം_OF (c_operations_മുകളിലെ) == c_op_മുകളിലെ_extended + 1, c_operations_മുകളിലെ_needs_one_entry_per_c_op_മുകളിലെ);

// clang-format on


// Opcodes the spec reserves leave zeroed holes in the tables above: no compiler
// and no operations. Every implemented op has at least one of the two.
സ്ഥിതം വരിയിൽ
bool IsImplementedOp (IM3OpInfo i_info)
{
    തിരികെ (i_info->compiler != c_cc_NULL or i_info->operations[0] != ശൂന്യ);
}

സ്ഥിരം u32 c_അക്കം_Operations   = M3_എണ്ണം_OF(c_operations);
സ്ഥിരം u32 c_അക്കം_Operations_FC = M3_എണ്ണം_OF(c_operationsFC);

// Maps one of the sparse opcodes above c_waOp_lastCore onto c_operationsHigh,
// or -1 for anything else in that range.
സ്ഥിതം വരിയിൽ
i32 Get_മുകളിലെ_Op_Index (m3opcode_t opcode)
{
    തിരഞ്ഞെടുപ്പ് (opcode) {
#എങ്കിൽ d_m3HasRefTypes
    അവസ്ഥ c_waOp_refNull: തിരികെ c_op_മുകളിലെ_ref_Null;
    അവസ്ഥ c_waOp_refIsNull: തിരികെ c_op_മുകളിലെ_ref_Is_Null;
    അവസ്ഥ c_waOp_refFunc: തിരികെ c_op_മുകളിലെ_ref_Func;
#  എങ്കിൽ d_m3HasTypedRefs
    അവസ്ഥ c_waOp_refAsNonNull: തിരികെ c_op_മുകളിലെ_ref_As_Non_Null;
#  endif
#endif
#എങ്കിൽ d_m3_Has_അട്ടി_Switching
    അവസ്ഥ c_waOp_contNew: തിരികെ c_op_മുകളിലെ_cont_New;
    അവസ്ഥ c_waOp_contBind: തിരികെ c_op_മുകളിലെ_cont_Bind;
    അവസ്ഥ c_waOp_suspend: തിരികെ c_op_മുകളിലെ_suspend;
    അവസ്ഥ c_waOp_resume: തിരികെ c_op_മുകളിലെ_resume;
    അവസ്ഥ c_waOp_resumeThrow: തിരികെ c_op_മുകളിലെ_resume_Throw;
    അവസ്ഥ c_waOp_resumeThrowRef: തിരികെ c_op_മുകളിലെ_resume_Throw_Ref;
    അവസ്ഥ c_waOp_switch: തിരികെ c_op_മുകളിലെ_switch;
#endif
    അവസ്ഥ c_waOp_extended: തിരികെ c_op_മുകളിലെ_extended;
#എങ്കിൽ d_m3HasAtomics
    അവസ്ഥ c_waOp_atomic: തിരികെ c_op_മുകളിലെ_atomic;
#endif
    സ്ഥിരസ്ഥിതി: തിരികെ -1;
    }
}

IM3OpInfo GetOpInfo (m3opcode_t opcode)
{
    IM3OpInfo info = ശൂന്യ;

    തിരഞ്ഞെടുപ്പ് (opcode >> 8) {
    അവസ്ഥ 0x00:
        // c_operations is indexed by opcode only up to c_waOp_lastCore; past that
        // it holds internal operations (DEBUG builds), unreachable by opcode.
        എങ്കിൽ (M3_LIKELY(opcode <= c_waOp_lastCore)) {
            info = &c_operations[opcode];
        } അല്ലെങ്കിൽ {
            i32 മുകളിലെ = Get_മുകളിലെ_Op_Index(opcode);
            എങ്കിൽ (മുകളിലെ >= 0) {
                info = &c_operations_മുകളിലെ[മുകളിലെ];
            }
        }
        നിർത്തുക;
    അവസ്ഥ c_waOp_extended:
        opcode &= 0xFF;
        എങ്കിൽ (M3_LIKELY(opcode <= c_waOp_lastExtended)) {
            info = &c_operationsFC[opcode];
        }
        നിർത്തുക;
#എങ്കിൽ d_m3HasAtomics
    അവസ്ഥ c_waOp_atomic: {
        i32 index = GetAtomicInfoIndex(opcode & 0xFF);
        എങ്കിൽ (index >= 0) {
            info = &c_operationsFE[index];
        }
    } നിർത്തുക;
#endif
    }

    തിരികെ (info and IsImplementedOp(info)) ? info : ശൂന്യ;
}

#എങ്കിൽ d_m3HasGasMetering

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
എണ്ണൽ {
    c_gasLocal   = 1,          // charged once per declared local, at function entry
    c_gasNominal = 1,          // nop, block, loop, if, const
    c_gasArith   = 45,
    c_gasShift   = 67,
    c_gasBranch  = 90,
    c_gasMemSize = 100,
    c_gasAccess  = 120,        // a local, global, table or memory access
    c_gasHeavy   = 10000,      // an indirect call, or anything that works over a whole region
    c_gasDivide  = 36000
};

// The most one op_UseGas can carry. A segment that reaches it is split into
// consecutive charges, which prepays exactly the same amount.
#  define d_m3MaxSegmentGas   0xFFFFFFFF

സ്ഥിതം
u32 GetGasCost (m3opcode_t i_opcode)
{
    തിരഞ്ഞെടുപ്പ് (i_opcode) {
    അവസ്ഥ 0x00:                        // unreachable
    അവസ്ഥ 0x01:                        // nop
    അവസ്ഥ c_waOp_block:
    അവസ്ഥ c_waOp_loop:
    അവസ്ഥ c_waOp_if:
    അവസ്ഥ c_waOp_tryTable:             // a block that also installs handlers
    അവസ്ഥ c_waOp_refNull:              // a constant, by another name
    അവസ്ഥ c_waOp_refFunc:
        തിരികെ c_gasNominal;

    അവസ്ഥ c_waOp_end:
        തിരികെ 0;                     // the one control instruction the table prices at nothing

    അവസ്ഥ c_waOp_else:
    അവസ്ഥ c_waOp_branch:
    അവസ്ഥ c_waOp_branchIf:
    അവസ്ഥ c_waOp_return:
    അവസ്ഥ c_waOp_call:
    അവസ്ഥ c_waOp_returnCall:
    അവസ്ഥ c_waOp_throw:
    അവസ്ഥ c_waOp_throwRef:
        തിരികെ c_gasBranch;

    അവസ്ഥ c_waOp_branchTable:
    അവസ്ഥ 0x1a:                        // drop
    അവസ്ഥ 0x1b:                        // select
    അവസ്ഥ c_waOp_selectTyped:
        തിരികെ c_gasAccess;

    അവസ്ഥ c_waOp_callIndirect:
    അവസ്ഥ c_waOp_returnCallIndirect:
    അവസ്ഥ c_waOp_callRef:
    അവസ്ഥ c_waOp_returnCallRef:
    അവസ്ഥ c_waOp_memoryInit:
    അവസ്ഥ c_waOp_memoryCopy:
    അവസ്ഥ c_waOp_memoryFill:
    അവസ്ഥ c_waOp_tableInit:
    അവസ്ഥ c_waOp_tableCopy:
    അവസ്ഥ c_waOp_tableGrow:
    അവസ്ഥ c_waOp_tableFill:
    അവസ്ഥ 0x40:                        // memory.grow
        തിരികെ c_gasHeavy;

    അവസ്ഥ c_waOp_dataDrop:
    അവസ്ഥ c_waOp_elemDrop:
    അവസ്ഥ c_waOp_tableSize:
    അവസ്ഥ 0x3f:                        // memory.size
        തിരികെ c_gasMemSize;

    അവസ്ഥ c_waOp_refIsNull:
    അവസ്ഥ c_waOp_refAsNonNull:
        തിരികെ c_gasArith;

    സ്ഥിരസ്ഥിതി:
        നിർത്തുക;
    }

    // The two integer arithmetic runs are priced by position, i32 at 0x6a and
    // i64 at 0x7c, the table giving both the same shape
    എങ്കിൽ ((i_opcode >= 0x6a and i_opcode <= 0x78) or (i_opcode >= 0x7c and i_opcode <= 0x8a)) {
        // clang-format off
        സ്ഥിതം സ്ഥിരം u16 c_arithmeticCosts [] = {
            c_gasArith,  c_gasArith,  c_gasArith,                   // add, sub, mul
            c_gasDivide, c_gasDivide, c_gasDivide, c_gasDivide,     // div_s, div_u, rem_s, rem_u
            c_gasArith,  c_gasArith,  c_gasArith,                   // and, or, xor
            c_gasShift,  c_gasShift,  c_gasShift,                   // shl, shr_s, shr_u
            c_gasBranch, c_gasBranch                                // rotl, rotr
        };
        // clang-format on

        തിരികെ c_arithmeticCosts[i_opcode - (i_opcode <= 0x78 ? 0x6a : 0x7c)];
    }

    // the atomic accesses are memory accesses; what waits, wakes or orders is worth
    // as much as an operation over a whole region
    എങ്കിൽ (i_opcode >= (c_waOp_atomic << 8) and i_opcode <= ((c_waOp_atomic << 8) | c_waOp_lastAtomic)) {
        തിരികെ (i_opcode <= c_waOp_atomicFence) ? c_gasHeavy : c_gasAccess;
    }
    എങ്കിൽ (i_opcode >= 0x20 and i_opcode <= 0x3e) {
        തിരികെ c_gasAccess;           // locals, globals, table accesses, loads and stores
    }
    എങ്കിൽ (i_opcode >= 0x41 and i_opcode <= 0x44) {
        തിരികെ c_gasNominal;          // i32/i64/f32/f64.const
    }
    എങ്കിൽ (i_opcode >= 0x8b and i_opcode <= 0xa6) {
        // the float run, priced as integer arithmetic except where it divides
        bool divides = (i_opcode == 0x91 or i_opcode == 0x95 or     // f32.sqrt, f32.div
                        i_opcode == 0x9f or i_opcode == 0xa3);      // f64.sqrt, f64.div

        തിരികെ divides ? c_gasDivide : c_gasArith;
    }

    // comparisons and clz/ctz/popcnt (0x45..0x69, 0x79..0x7b), the conversions
    // and sign extensions (0xa7..0xc4), the saturating truncations, the wide
    // arithmetic, and anything else that reaches here: plain arithmetic
    തിരികെ c_gasArith;
}

// The instructions ewasm's design segments a body at. A segment runs up to and
// including the next one of these, so every cycle a body can execute - a loop's
// back edge, a branch out of a block, falling off the end of a function - passes
// through at least one, and no path can run unmetered however it is entered.
സ്ഥിതം
bool IsGasSegmentEnd (m3opcode_t i_opcode)
{
    തിരഞ്ഞെടുപ്പ് (i_opcode) {
    അവസ്ഥ c_waOp_loop:
    അവസ്ഥ c_waOp_if:
    അവസ്ഥ c_waOp_else:
    അവസ്ഥ c_waOp_end:
    അവസ്ഥ c_waOp_branch:
    അവസ്ഥ c_waOp_branchIf:
    അവസ്ഥ c_waOp_branchTable:
    അവസ്ഥ c_waOp_return:
    അവസ്ഥ c_waOp_returnCall:
    അവസ്ഥ c_waOp_returnCallIndirect:
    അവസ്ഥ c_waOp_returnCallRef:
        തിരികെ true;
    സ്ഥിരസ്ഥിതി:
        തിരികെ false;
    }
}

// Ends the open segment by writing what it accumulated into the immediate its
// op_UseGas is still waiting on. Emitting onto a later code page does not move
// an earlier one, so the reservation is still where it was left.
സ്ഥിതം
ശൂന്യം CloseGasSegment (IM3Compilation o)
{
    എങ്കിൽ (o->isGasSegmentOpen) {
        എങ്കിൽ (o->gasPatch) {
            u32 cost = o->gasCost;
            memcpy(o->gasPatch, &cost, വലുപ്പം(cost));

            o->gasPatch = ശൂന്യ;
        }
        o->gasCost          = 0;
        o->isGasSegmentOpen = false;
    }
}

// Whether this compilation emits the charges
സ്ഥിതം വരിയിൽ
bool IsMetering (IM3Compilation o)
{
    തിരികെ o->runtime->gasLimit != 0;
}

// Charges one instruction to the segment being compiled, opening a segment at
// this point in the code stream if none is open, and settling the segment when
// the instruction is one that ends it. Called before the instruction is
// compiled, which is what both halves need: the op_UseGas lands ahead of what it
// pays for, and the cost is final before a block-structured instruction - loop,
// if, else - compiles its body from inside its own compiler. The body then opens
// a segment of its own, inside the loop or the arm, where it is charged on every
// pass rather than once on the way in.
സ്ഥിതം
M3Result MeterOpcode (IM3Compilation o, m3opcode_t i_opcode)
{
    M3Result result = m3Err_none;

    // constant expressions are not metered: they run once at instantiation,
    // before the module is anything a gas budget was handed out for
    എങ്കിൽ (o->function and o->page and IsMetering(o)) {
        // only the 0xFC or 0xFE prefix has been read so far; the instruction it names is
        // the LEB128 u32 the compiler is about to take. Peek it without consuming.
        // Anything the tables do not cover leaves the prefix priced on its own,
        // which is moot - the compile fails on it a moment later.
        എങ്കിൽ (i_opcode == c_waOp_extended or i_opcode == c_waOp_atomic) {
            bytes_t peek = o->wasm;
            u32     sub;
            u32     last = (i_opcode == c_waOp_atomic) ? c_waOp_lastAtomic : c_waOp_lastExtended;

            എങ്കിൽ (not ReadLEB_u32(&sub, &peek, o->wasmEnd) and sub <= last) {
                i_opcode = (m3opcode_t)((i_opcode << 8) | sub);
            }
        }

        u32 cost = GetGasCost(i_opcode);

        എങ്കിൽ (o->isGasSegmentOpen and o->gasCost > d_m3MaxSegmentGas - cost) {
            CloseGasSegment(o);
        }

        എങ്കിൽ (not o->isGasSegmentOpen) {
            o->isGasSegmentOpen = true;

_           (EmitOp(o, op_UseGas));
            o->gasPatch = (ശൂന്യം*)GetPagePC(o->page);
            EmitConstant32(o, 0);
        }

        o->gasCost += cost;

        എങ്കിൽ (IsGasSegmentEnd(i_opcode)) {
            CloseGasSegment(o);
        }
    }

    _catch: തിരികെ result;
}

#endif // d_m3HasGasMetering


M3Result CompileBlockStatements (IM3Compilation o)
{
    M3Result result   = m3Err_none;
    bool     validEnd = false;

    ആകുമ്പോൾ (o->wasm < o->wasmEnd) {
#എങ്കിൽ d_m3EnableOpTracing
        എങ്കിൽ (o->അക്കം_Emits) {
            EmitOp(o, op_Dump_അട്ടി);
            EmitConstant32(o, o->അക്കം_Opcodes);
            EmitConstant32(o, GetMaxUsedSlotPlusOne(o));
            EmitPointer(o, o->function);

            o->അക്കം_Emits = 0;
        }
#endif

        m3opcode_t opcode;
        o->lastOpcodeStart = o->wasm;
_       (Read_opcode(&opcode, &o->wasm, o->wasmEnd));
        log_opcode(o, opcode);

        // Restrict opcodes when evaluating expressions
        എങ്കിൽ (M3_UNLIKELY(not o->function)) {
            // clang-format off
            തിരഞ്ഞെടുപ്പ് (opcode) {
            അവസ്ഥ c_waOp_i32_const: അവസ്ഥ c_waOp_i64_const:
            അവസ്ഥ c_waOp_f32_const: അവസ്ഥ c_waOp_f64_const:
            അവസ്ഥ c_waOp_getGlobal: അവസ്ഥ c_waOp_end:
#എങ്കിൽ d_m3HasRefTypes
            അവസ്ഥ c_waOp_refNull:   അവസ്ഥ c_waOp_refFunc:
#endif
#എങ്കിൽ d_m3HasExtendedConst
            അവസ്ഥ c_waOp_i32_add:   അവസ്ഥ c_waOp_i32_sub:   അവസ്ഥ c_waOp_i32_mul:
            അവസ്ഥ c_waOp_i64_add:   അവസ്ഥ c_waOp_i64_sub:   അവസ്ഥ c_waOp_i64_mul:
#endif
                നിർത്തുക;
            സ്ഥിരസ്ഥിതി:
                _throw(m3Err_restrictedOpcode);
            }
            // clang-format on
        }

        IM3OpInfo opinfo = GetOpInfo(opcode);

        എങ്കിൽ (M3_UNLIKELY(opinfo == ശൂന്യ)) {
            _throw(ErrorCompile(m3Err_unknownOpcode, o, "opcode '%x' not available", opcode));
        }

#എങ്കിൽ d_m3HasGasMetering
_       (MeterOpcode(o, opcode));
#endif

        എങ്കിൽ (opinfo->compiler != c_cc_NULL) {
_           ((*GetCompiler(opinfo))(o, opcode))
        } അല്ലെങ്കിൽ {
_           (Compile_Operator(o, opcode));
        }

        o->previousOpcode = opcode;

        എങ്കിൽ (opcode == c_waOp_else) {
            _throwif(m3Err_wasmMalformed, o->block.opcode != c_waOp_if);
            validEnd = true;
            നിർത്തുക;
        } അല്ലെങ്കിൽ എങ്കിൽ (opcode == c_waOp_end) {
            validEnd = true;
            നിർത്തുക;
        }
    }
    _throwif(m3Err_wasmMalformed, not validEnd);

_catch:
    തിരികെ result;
}

സ്ഥിതം
M3Result തള്ളുക_Block_Results (IM3Compilation o)
{
    M3Result result = m3Err_none;

    u16 അക്കം_Results = Get_Func_Type_അക്കം_Results(o->block.type);

    വേണ്ടി (u16 i = 0; i < അക്കം_Results; ++i) {
        m3type_t type = GetFuncTypeResultType(o->block.type, i);

        എങ്കിൽ (i == അക്കം_Results - 1 and IsFpType(type)) {
_           (തള്ളുക_Register(o, type));
        } അല്ലെങ്കിൽ {
_           (തള്ളുക_Allocated_Slot(o, type));
        }
    }

    _catch: തിരികെ result;
}


M3Result CompileBlock (IM3Compilation o, IM3FuncType i_blockType, m3opcode_t i_blockOpcode)
{
                                                                                        d_m3Assert (not IsRegisterAllocated (o, 0));
                                                                                        d_m3Assert (not IsRegisterAllocated (o, 1));
    InvalidateFold(o);

    M3CompilationScope  outerScope = o->block;
    M3CompilationScope* block      = &o->block;

    block->outer   = &outerScope;
    block->pc      = GetPagePC(o->page);
    block->patches = ശൂന്യ;
    block->type    = i_blockType;
    block->opcode  = i_blockOpcode;
    block->depth++;
    block->snapshotBlock = 0;

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
#എങ്കിൽ d_m3HasSnapshots
    // op_Loop's frame records the body, which is also the loop's identity;
    // op_TryTable's records its clause table, two words a clause back from it
    എങ്കിൽ (i_blockOpcode == c_waOp_loop) {
_       (RecordBlockStart(o, i_blockOpcode, block->pc, 0));
    }
#  എങ്കിൽ d_m3HasExceptionHandling
    അല്ലെങ്കിൽ എങ്കിൽ (i_blockOpcode == c_waOp_tryTable) {
_       (RecordBlockStart(o, i_blockOpcode, block->pc - 2 * o->അക്കം_Try_Clauses, o->അക്കം_Try_Clauses));
    }
#  endif
#endif

    // validate and dealloc params ----------------------------

    u16 അട്ടി_Index = o->അട്ടി_Index;

    u16 അക്കം_Params = Get_Func_Type_അക്കം_Params(i_blockType);

    എങ്കിൽ (i_blockOpcode != c_waOp_else) {
        വേണ്ടി (u16 i = 0; i < അക്കം_Params; ++i) {
            m3type_t type = GetFuncTypeParamType(i_blockType, അക്കം_Params - 1 - i);
_           (എടുക്കുക_Type(o, type));
        }
    } അല്ലെങ്കിൽ {
        എങ്കിൽ (Is_അട്ടി_Polymorphic(o) && o->block.block_അട്ടി_Index + അക്കം_Params > o->അട്ടി_Index) {
            o->അട്ടി_Index = o->block.block_അട്ടി_Index;
        } അല്ലെങ്കിൽ {
            o->അട്ടി_Index -= അക്കം_Params;
        }
    }

    u16 paramIndex        = o->അട്ടി_Index;
    block->exit_അട്ടി_Index = paramIndex; // consume the params at block exit

    // keep copies of param slots in the stack
    o->അട്ടി_Index = അട്ടി_Index;

    // find slots for the results ----------------------------
_   (തള്ളുക_Block_Results(o));

    അട്ടി_Index = o->അട്ടി_Index;

    // dealloc but keep record of the result slots in the stack
    u16 അക്കം_Results = Get_Func_Type_അക്കം_Results(i_blockType);
    ആകുമ്പോൾ (അക്കം_Results--) {
        എടുക്കുക(o);
    }

    block->block_അട്ടി_Index = o->അട്ടി_Index = അട്ടി_Index;

    // push the params back onto the stack -------------------
    വേണ്ടി (u16 i = 0; i < അക്കം_Params; ++i) {
        m3type_t type = GetFuncTypeParamType(i_blockType, i);

        u16 slot = Get_Slot_For_അട്ടി_Index(o, paramIndex + i);
        തള്ളുക(o, type, slot);

        എങ്കിൽ (slot >= o->slotFirstDynamicIndex && slot != c_slotUnused) {
_           (MarkSlotsAllocatedByType(o, slot, type));
        }
    }

    //--------------------------------------------------------

#എങ്കിൽ d_m3HasExceptionHandling
    // with the scope pushed and the params back on, a catch label of 0 names
    // this block - which is what the proposal says it should
    എങ്കിൽ (i_blockOpcode == c_waOp_tryTable) {
_       (EmitCatchStubs(o));
    }
#endif

_   (CompileBlockStatements(o));

_   (ValidateBlockEnd(o));

    എങ്കിൽ (o->function)    // skip for expressions
    {
        എങ്കിൽ (not Is_അട്ടി_Polymorphic(o)) {
_           (ResolveBlockResults(o, &o->block, /* isBranch: */ false));
        }

#എങ്കിൽ d_m3HasExceptionHandling
        // Falling out of the end of a try region retires its handler. This sits
        // ahead of PatchBranches so branches into the block's exit land past it:
        // a branch out of the try popped its own handler at the branch site.
        എങ്കിൽ (i_blockOpcode == c_waOp_tryTable) {
_           (EmitOp(o, op_എടുക്കുക_Handlers));
            EmitConstant32(o, 1);
        }
#endif

_       (Unwind_Block_അട്ടി(o))

        എങ്കിൽ (not ((i_blockOpcode == c_waOp_if and അക്കം_Results) or o->previousOpcode == c_waOp_else)) {
            o->അട്ടി_Index = o->block.exit_അട്ടി_Index;
_           (തള്ളുക_Block_Results(o));
        }
    }

    PatchBranches(o);

#എങ്കിൽ d_m3HasSnapshots
    RecordBlockEnd(o);

    എങ്കിൽ (o->localInitDepth) {
        വേണ്ടി (u32 i = 0; i < o->snapshotMap->അക്കം_Locals; ++i) {
            എങ്കിൽ (o->localInitDepth[i] >= (u32)o->block.depth) {
                o->localInitDepth[i] = UINT32_MAX;
            }
        }
    }
#endif
    o->block = outerScope;

} _catch: തിരികെ result;
}

സ്ഥിതം
M3Result CompileLocals (IM3Compilation o)
{
    M3Result result;

    u32 അക്കം_Locals = 0;
    u32 അക്കം_Local_Blocks;
_   (ReadLEB_u32(&അക്കം_Local_Blocks, &o->wasm, o->wasmEnd));

    വേണ്ടി (u32 l = 0; l < അക്കം_Local_Blocks; ++l) {
        u32      var_എണ്ണം;
        m3type_t localType;

_       (ReadLEB_u32(&var_എണ്ണം, &o->wasm, o->wasmEnd));
_       (Parse_മൂല്യം_Type(o->module, &localType, &o->wasm, o->wasmEnd));
        അക്കം_Locals += var_എണ്ണം;                                                          m3log (compile, "pushing locals. count: %d; type: %s", var_എണ്ണം, c_waTypes [BaseTypeOf(localType)]);
        ആകുമ്പോൾ (var_എണ്ണം--) {
_           (തള്ളുക_Allocated_Slot(o, localType));
        }
    }

    എങ്കിൽ (o->function) {
        o->function->അക്കം_Locals = അക്കം_Locals;
    }

    _catch: തിരികെ result;
}

സ്ഥിതം
M3Result ReserveConstants (IM3Compilation o)
{
    M3Result result = m3Err_none;

    // in the interest of speed, this blindly scans the Wasm code looking for any byte
    // that looks like an const opcode.
    u16 അക്കം_Constant_Slots = 0;

    bytes_t wa = o->wasm;
    ആകുമ്പോൾ (wa < o->wasmEnd) {
        u8  code     = *wa++;
        u16 addSlots = 0;

        എങ്കിൽ (code == c_waOp_i32_const or code == c_waOp_f32_const) {
            addSlots = 1;
        } അല്ലെങ്കിൽ എങ്കിൽ (code == c_waOp_i64_const or code == c_waOp_f64_const) {
            addSlots = Get_Type_അക്കം_Slots(c_m3Type_i64);
        }

        എങ്കിൽ (അക്കം_Constant_Slots + addSlots >= d_m3MaxConstantTableSize) {
            നിർത്തുക;
        }

        അക്കം_Constant_Slots += addSlots;
    }

    // if constants overflow their reserved stack space, the compiler simply emits op_Const
    // operations as needed. Compiled expressions (global inits) don't pass through this
    // ReserveConstants function and thus always produce inline constants.

    AlignSlotToType(&അക്കം_Constant_Slots, c_m3Type_i64);                                           m3log (compile, "reserved constant slots: %d", അക്കം_Constant_Slots);

    o->slotFirstDynamicIndex = o->slotFirstConstIndex + അക്കം_Constant_Slots;

    എങ്കിൽ (o->slotFirstDynamicIndex >= d_m3MaxFunctionSlots) {
        _throw(m3_Err_function_അട്ടി_Overflow);
    }

    _catch:
    തിരികെ result;
}


// A constant expression compiles like a miniature function root block: no args,
// no locals and a single result, which Compile_End copies down into slot 0 for
// EvaluateExpression to read back.
M3Result CompileExpression (IM3Compilation o, IM3FuncType i_resultType)
{
    M3Result result = m3Err_none;

    o->block.type = i_resultType;

    u16 അക്കം_Ret_Slots = Get_Func_Type_അക്കം_Results(i_resultType) * c_io_Slot_എണ്ണം;

    വേണ്ടി (u16 i = 0; i < അക്കം_Ret_Slots; ++i) {
        MarkSlotAllocated(o, i);
    }

    o->max_അട്ടി_Slots = o->slotMaxAllocatedIndexPlusOne = o->slotFirstDynamicIndex = അക്കം_Ret_Slots;

_   (CompileBlockStatements(o));

    _catch: തിരികെ result;
}


M3Result CompileFunction (IM3Function io_function)
{
    എങ്കിൽ (!io_function->wasm) {
        // An import. Either linking pointed it at another module's function -
        // in which case that one carries the body - or a host function was
        // bound to it, or it is simply unsatisfied.
        IM3Function impl = Function_Implementation(io_function);

        എങ്കിൽ (impl != io_function) {
            എങ്കിൽ (not impl->compiled) {
                M3Result r = CompileFunction(impl);
                എങ്കിൽ (r) {
                    തിരികെ r;
                }
            }

            // keep the placeholder callable too, for anything still holding it
            io_function->compiled = impl->compiled;
            തിരികെ m3Err_none;
        }

        എങ്കിൽ (io_function->compiled) {
            തിരികെ m3Err_none;
        }

        തിരികെ ErrorModule(m3Err_functionImportMissing, io_function->module, "'%s.%s'",
                           GetFunctionImportModuleName(io_function),
                           m3_GetFunctionName(io_function));
    }

#എങ്കിൽ d_m3EnableValidation
    എങ്കിൽ (not io_function->module->runtime->skipValidation) {
        M3Result vr = ValidateFunction(io_function);
        എങ്കിൽ (vr) {
            തിരികെ vr;
        }
    }
#endif

    IM3FuncType funcType = io_function->funcType;                   m3log (compile, "compiling: [%d] %s %s; wasm-size: %d",
                                                                        io_function->index, m3_GetFunctionName (io_function), SPrintFuncTypeSignature (funcType), (u32) (io_function->wasmEnd - io_function->wasm));
    IM3Runtime runtime = io_function->module->runtime;

    IM3Compilation o = &runtime->compilation;                       d_m3Assert (d_m3MaxFunctionSlots >= d_m3_Max_Function_അട്ടി_Height * (d_m3Use32BitSlots + 1))  // need twice as many slots in 32-bit mode
    memset(o, 0x0, വലുപ്പം(M3Compilation));

    o->runtime    = runtime;
    o->module     = io_function->module;
    o->function   = io_function;
    o->wasm       = io_function->wasm;
    o->wasmEnd    = io_function->wasmEnd;
    o->block.type = funcType;

_try {
    // skip over code size. the end was already calculated during parse phase
    u32 size;
_   (ReadLEB_u32(&size, &o->wasm, o->wasmEnd));                     d_m3Assert (size == (o->wasmEnd - o->wasm))

#എങ്കിൽ d_m3HasSnapshots
    o->bodyStart = o->wasm;
#endif

#എങ്കിൽ d_m3HasSnapshots
    // only code that can be suspended can end up in a snapshot
    എങ്കിൽ (runtime->isSuspendable) {
        o->snapshotMap = m3_AllocStruct(M3SnapshotMap);
        _throwifnull(o->snapshotMap);
    }
#endif

_   (AcquireCompilationCodePage(o, &o->page));

    pc_t pc = GetPagePC(o->page);

    u16 അക്കം_Ret_Slots = Get_Function_അക്കം_Returns(o->function) * c_io_Slot_എണ്ണം;

    വേണ്ടി (u16 i = 0; i < അക്കം_Ret_Slots; ++i) {
        MarkSlotAllocated(o, i);
    }

    o->function->അക്കം_Ret_Slots = o->slotFirstDynamicIndex = അക്കം_Ret_Slots;

    u16 അക്കം_Args = Get_Function_അക്കം_Args(o->function);

    // push the arg types to the type stack
    വേണ്ടി (u16 i = 0; i < അക്കം_Args; ++i) {
        m3type_t type = GetFunctionArgType(o->function, i);
_       (തള്ളുക_Allocated_Slot(o, type));

        // prevent allocator fill-in
        o->slotFirstDynamicIndex += c_io_Slot_എണ്ണം;
    }

    o->slotMaxAllocatedIndexPlusOne = o->function->അക്കം_Ret_And_Arg_Slots = o->slotFirstLocalIndex = o->slotFirstDynamicIndex;

_   (CompileLocals(o));

#എങ്കിൽ d_m3HasSnapshots
_   (RecordLocals(o));
#endif

#എങ്കിൽ d_m3HasGasMetering
    // the table charges for a function's locals as well as for its instructions.
    // op_Entry is what sets them up, so the charge rides on the body's first
    // segment, which is emitted right after it
    o->gasCost = o->function->അക്കം_Locals * c_gasLocal;
#endif

    u16 maxSlot = GetMaxUsedSlotPlusOne(o);

    o->function->അക്കം_Local_Bytes = (maxSlot - o->slotFirstLocalIndex) * വലുപ്പം(m3slot_t);

    o->slotFirstConstIndex = o->slotMaxConstIndex = maxSlot;

    // ReserveConstants initializes o->firstDynamicSlotNumber
_   (ReserveConstants(o));

    // start tracking the max stack used (Push() also updates this value) so that op_Entry can precisely detect stack overflow
    o->max_അട്ടി_Slots = o->slotMaxAllocatedIndexPlusOne = o->slotFirstDynamicIndex;

    o->block.block_അട്ടി_Index = o->അട്ടി_First_Dynamic_Index = o->അട്ടി_Index;                           m3log (compile, "start stack index: %u; max stack slots: %u",
                                                                                                           (u32) o->അട്ടി_First_Dynamic_Index, (u32) o->max_അട്ടി_Slots);
#എങ്കിൽ d_m3_Has_അട്ടി_Switching
    // A suspendable body opens by checking for a pause request, once its frame
    // is set up. The pause point's offset is the first instruction's, just past
    // the local declarations - the body's end, when it has no other.
    എങ്കിൽ (runtime->isSuspendable) {
        o->lastOpcodeStart = o->wasm;
#  എങ്കിൽ d_m3EntryKeepsFrame
_       (EmitOp(o, op_Entry));
        EmitPointer(o, io_function);
_       (EmitOp(o, op_EntryCheck));
_       (RecordSafePoint(o, safepoint_entry, 1, 0, 0, ശൂന്യ));
#  അല്ലെങ്കിൽ
_       (EmitOp(o, op_Entry_Suspendable));
        EmitPointer(o, io_function);
_       (RecordSafePoint(o, safepoint_entry, 0, 0, 0, ശൂന്യ));
#  endif
    } അല്ലെങ്കിൽ
#endif
    {
_       (EmitOp(o, op_Entry));
        EmitPointer(o, io_function);
    }

_   (CompileBlockStatements(o));

    _throwif(m3Err_wasmMalformed, o->previousOpcode != c_waOp_end);

    io_function->compiled      = pc;
    io_function->max_അട്ടി_Slots = o->max_അട്ടി_Slots;

    u16 അക്കം_Constant_Slots = o->slotMaxConstIndex - o->slotFirstConstIndex;                           m3log (compile, "unique constant slots: %u; unused slots: %u",
                                                                                                           അക്കം_Constant_Slots, o->slotFirstDynamicIndex - o->slotMaxConstIndex);
    io_function->അക്കം_Constant_Bytes = അക്കം_Constant_Slots * വലുപ്പം(m3slot_t);

    എങ്കിൽ (അക്കം_Constant_Slots) {
        io_function->constants = m3_CopyMem((cbytes_t)o->constants, io_function->അക്കം_Constant_Bytes);
        _throwifnull(io_function->constants);
    }

#എങ്കിൽ d_m3HasSnapshots
    io_function->snapshotMap = o->snapshotMap;
    o->snapshotMap           = ശൂന്യ;
#endif

} _catch:

    ReleaseCompilationCodePage(o);

#എങ്കിൽ d_m3HasSnapshots
    SnapshotMap_Free(o->snapshotMap);
    o->snapshotMap = ശൂന്യ;
    m3_Free(o->localInitDepth);
#endif

    തിരികെ result;
}

#എങ്കിൽ d_m3_Has_അട്ടി_Switching
m3ret_t ResumeContinuation (IM3Runtime i_runtime, IM3Continuation i_cont)
{
    എങ്കിൽ (M3_UNLIKELY(!i_cont || i_cont->state != cont_suspended)) {
        തിരികെ m3Err_none;
    }

    M3MemoryHeader* _mem = i_cont->entryFunction ? Module_MemoryHeader(i_cont->entryFunction->module) : ശൂന്യ;

    i_runtime->activeContinuation = i_cont;
    i_cont->state                 = cont_running;

    i32 outermost     = (i32)i_cont->അക്കം_Frames - 1;
    i_cont->അക്കം_Frames = 0;

    // with no frames standing, this lands straight on the suspension point
    m3ret_t r = ReplayFrames(i_cont, outermost, _mem);

    എങ്കിൽ (r == m3Err_continuationSuspended) {
        i_cont->state = cont_suspended;
    } അല്ലെങ്കിൽ {
        i_cont->state = cont_returned;
    }
    i_runtime->activeContinuation = ശൂന്യ;

    തിരികെ r;
}
#endif
