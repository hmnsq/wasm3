//
//  m3_compile.c
//
//  Created by Steven Massey on 4/17/19.
//  Copyright © 2019 Steven Massey. All rights reserved.
//

// Allow using opcodes for compilation process
#define M3_COMPILE_OPCODES

#অন্তর্ভাব "m3_env.h"
#অন্তর্ভাব "m3_compile.h"
#অন্তর্ভাব "m3_exec.h"
#অন্তর্ভাব "m3_exception.h"
#অন্তর্ভাব "m3_info.h"
#অন্তর্ভাব "m3_validate.h"

// Resolves M3OpInfo.compiler, an index, to the function it names. Defined down
// with c_compilers [], which has to follow every compiler it points at.
স্থির
M3Compiler GetCompiler (IM3OpInfo i_opInfo);

//----- EMIT --------------------------------------------------------------------------------------------------------------

স্থির অন্তঃরেখ
pc_t GetPC (IM3Compilation o)
{
    ফেরত GetPagePC(o->page);
}

#যদি d_m3HasSnapshots

#  define d_m3GrowSnapshotArray(TYPE, ARRAY, গুনতি_2, CAPACITY)                    \
    যদি ((গুনতি_2) == (CAPACITY)) {                                                 \
        u32   newCapacity = (CAPACITY) ? (CAPACITY) * 2 : 8;                     \
        TYPE* grown = m3_ReallocArray(TYPE, (ARRAY), newCapacity, (CAPACITY));   \
        যদি (not grown) {                                                         \
            ফেরত m3Err_mallocFailed;                                           \
        }                                                                        \
        (ARRAY)    = grown;                                                      \
        (CAPACITY) = newCapacity;                                                \
    }

// Where the instruction being compiled starts, counted from the start of the
// function's body: the one position every build of every revision agrees on
স্থির অন্তঃরেখ
u32 GetWasmOffset (IM3Compilation o)
{
    ফেরত (u32)(o->lastOpcodeStart - o->bodyStart);
}

// Whether the operand stack entry at i_index is a value the frame holds. Every
// block keeps records of its parameter and result slots on the stack, between
// the height it was entered at and where its own values begin (see
// CompileBlock). Those are landing pads rather than values: nothing need have
// written them yet, and the slots they name may be in use for something else.
// The function's own scope has none - that stretch is its arguments and locals.
স্থির
bool Is_Live_স্তূপ_Entry (IM3Compilation o, u16 i_সূচক)
{
    জন্য (IM3CompilationScope scope = &o->block; scope->outer; scope = scope->outer) {
        যদি (i_সূচক >= scope->exit_স্তূপ_সূচক and i_সূচক < scope->block_স্তূপ_সূচক) {
            ফেরত false;
        }
    }

    ফেরত true;
}

স্থির
M3_ফলাফল Add_Safe_Point_মান (IM3Compilation o, M3SafePoint* io_point, u16 i_সূচক)
{
    M3SnapshotMap* map = o->snapshotMap;

    d_m3GrowSnapshotArray(M3_Slot_মান, map->মানসমূহ, map->অঙ্ক_মানসমূহ, o->মানসমূহ_Capacity);

    map->মানসমূহ[map->অঙ্ক_মানসমূহ].slot = o->wasm_স্তূপ[i_সূচক];
    map->মানসমূহ[map->অঙ্ক_মানসমূহ].type = o->type_স্তূপ[i_সূচক];
    map->অঙ্ক_মানসমূহ++;
    io_point->অঙ্ক_মানসমূহ++;

    ফেরত m3Err_none;
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
স্থির
M3_ফলাফল RecordSafePoint (IM3Compilation o, M3SafePointKind i_kind, u32 i_শব্দগুলি_Back, u16 i_অঙ্ক_Results,
                          u16 i_aux, IM3CompilationScope i_loop)
{
_try {
    M3SnapshotMap* map = o->snapshotMap;

    যদি (not map or not o->page) {
        ফেরত m3Err_none;
    }

    d_m3GrowSnapshotArray(M3SafePoint, map->safePoints, map->অঙ্ক_Safe_Points, o->safePointsCapacity);

    // grown before it is pointed at: adding values never moves the safepoints
    M3SafePoint* point     = &map->safePoints[map->অঙ্ক_Safe_Points];
    point->pc              = GetPC(o) - i_শব্দগুলি_Back;
    point->wasmOffset      = GetWasmOffset(o);
    point->first_মান      = map->অঙ্ক_মানসমূহ;
    point->first_Local_মান = UINT32_MAX;
    point->অঙ্ক_মানসমূহ       = 0;
    point->অঙ্ক_Results      = i_অঙ্ক_Results;
    point->aux             = i_aux;
    point->resumeType      = c_m3Type_none;
    point->kind            = (u8)i_kind;
    point->flags           = 0;

    // Non-defaultable locals can still be uninitialized at a safepoint. Keep
    // their heap types, but allow the null placeholder until local.set makes
    // them definitely initialized. Branches to a loop use its entry context.
    যদি (o->localInitDepth) {
        point->first_Local_মান = map->অঙ্ক_মানসমূহ;
        জন্য (u32 i = 0; i < map->অঙ্ক_Locals; ++i) {
            d_m3GrowSnapshotArray(M3_Slot_মান, map->মানসমূহ, map->অঙ্ক_মানসমূহ, o->মানসমূহ_Capacity);
            M3_Slot_মান মান = map->locals[i];
            যদি (o->localInitDepth[i] == UINT32_MAX or
                (i_loop and o->localInitDepth[i] >= (u32)i_loop->depth)) {
                মান.type &= ~d_m3Type_refNonNull;
            }
            map->মানসমূহ[map->অঙ্ক_মানসমূহ++] = মান;
        }
        point->first_মান = map->অঙ্ক_মানসমূহ;
    }

    u16 শীর্ষ = o->স্তূপ_সূচক;

    যদি (i_kind != safepoint_suspend) {
        শীর্ষ = (শীর্ষ > i_অঙ্ক_Results) ? (u16)(শীর্ষ - i_অঙ্ক_Results) : 0;
    }
    যদি (i_loop) {
        শীর্ষ = M3_MIN(শীর্ষ, i_loop->exit_স্তূপ_সূচক);
    }

    জন্য (u16 i = o->স্তূপ_First_Dynamic_সূচক; i < শীর্ষ; ++i) {
        যদি (Is_Live_স্তূপ_Entry(o, i)) {
_           (Add_Safe_Point_মান(o, point, i));
        }
    }

    যদি (i_loop) {
        u16 অঙ্ক_Params = Get_Func_Type_অঙ্ক_Params(i_loop->type);

        জন্য (u16 i = 0; i < অঙ্ক_Params; ++i) {
_           (Add_Safe_Point_মান(o, point, (u16)(i_loop->exit_স্তূপ_সূচক + i)));
        }
    }

    map->অঙ্ক_Safe_Points++;

} _catch: ফেরত ফলাফল;
}

// A back edge: op_ContinueLoop, or op_ContinueLoopIf when i_isConditional, and
// the loop it goes back to. The conditional one suspends before it has taken
// the branch, so it has to be resumed with the condition that took it.
স্থির
M3_ফলাফল RecordBackEdge (IM3Compilation o, IM3CompilationScope i_loop, bool i_isConditional)
{
    M3_ফলাফল ফলাফল = m3Err_none;

    // the loop is named by its record in the map, which has to fit the field
    _throwif("a function has too many loops for its snapshot map",
             o->snapshotMap and o->page and (i_loop->snapshotBlock == 0 or i_loop->snapshotBlock > UINT16_MAX + 1u));

_   (RecordSafePoint(o, safepoint_op, 2, 0, (u16)(i_loop->snapshotBlock - 1), i_loop));

    যদি (i_isConditional and o->snapshotMap and o->page) {
        o->snapshotMap->safePoints[o->snapshotMap->অঙ্ক_Safe_Points - 1].flags |= d_m3SafePointTakenBranch;
    }

    _catch: ফেরত ফলাফল;
}

// A call, waiting on its callee with i_numResults not yet written; the
// callee's frame starts i_calleeSlot slots above this one. A return_call that
// is compiled as a call is one too, since its frame stands while the callee
// runs, but it is marked: in Wasm the callee has replaced it, so a snapshot
// walks past it rather than writing it.
স্থির
M3_ফলাফল RecordCall (IM3Compilation o, u16 i_অঙ্ক_Results, u16 i_calleeSlot, bool i_isReturnCall)
{
    M3_ফলাফল ফলাফল = RecordSafePoint(o, safepoint_call, 0, i_অঙ্ক_Results, i_calleeSlot, ফাঁকা);

    যদি (not ফলাফল and i_isReturnCall and o->snapshotMap and o->page) {
        o->snapshotMap->safePoints[o->snapshotMap->অঙ্ক_Safe_Points - 1].flags |= d_m3_Safe_Point_লেজ_Call;
    }

    ফেরত ফলাফল;
}

// Where the function's arguments and locals are kept. Unlike the operand
// stack, that never changes, so it is recorded once rather than at every
// safepoint.
স্থির
M3_ফলাফল RecordLocals (IM3Compilation o)
{
    M3SnapshotMap* map = o->snapshotMap;

    যদি (map and o->স্তূপ_সূচক) {
        map->locals = m3_AllocArray(M3_Slot_মান, o->স্তূপ_সূচক);

        যদি (not map->locals) {
            ফেরত m3Err_mallocFailed;
        }

        জন্য (u16 i = 0; i < o->স্তূপ_সূচক; ++i) {
            map->locals[i].slot = o->wasm_স্তূপ[i];
            map->locals[i].type = o->type_স্তূপ[i];
        }

        map->অঙ্ক_Locals = o->স্তূপ_সূচক;
        জন্য (u32 i = Get_Function_অঙ্ক_Args(o->function); i < map->অঙ্ক_Locals; ++i) {
            যদি (IsRefType(map->locals[i].type) and not IsNullableRef(map->locals[i].type)) {
                o->localInitDepth = m3_AllocArray(u32, map->অঙ্ক_Locals);
                যদি (not o->localInitDepth) {
                    ফেরত m3Err_mallocFailed;
                }
                জন্য (u32 j = i; j < map->অঙ্ক_Locals; ++j) {
                    যদি (IsRefType(map->locals[j].type) and not IsNullableRef(map->locals[j].type)) {
                        o->localInitDepth[j] = UINT32_MAX;
                    }
                }
                বিরতি;
            }
        }
    }

    ফেরত m3Err_none;
}

// A loop or a try_table, whose native frame a snapshot names by the
// instruction's offset. i_pc is what that frame records. The scope being
// compiled keeps the record, so that RecordBlockEnd can close it.
স্থির
M3_ফলাফল RecordBlockStart (IM3Compilation o, m3opcode_t i_opcode, pc_t i_pc, u32 i_অঙ্ক_Clauses)
{
    M3SnapshotMap* map = o->snapshotMap;

    যদি (map and o->page) {
        d_m3GrowSnapshotArray(M3BlockStart, map->blocks, map->অঙ্ক_Blocks, o->blocksCapacity);

        M3BlockStart* block = &map->blocks[map->অঙ্ক_Blocks++];
        block->pc           = i_pc;
        block->wasmOffset   = GetWasmOffset(o);
        block->wasmEnd      = UINT32_MAX;
        block->অঙ্ক_Clauses   = (u16)i_অঙ্ক_Clauses;
        block->opcode       = (u8)i_opcode;

        o->block.snapshotBlock = map->অঙ্ক_Blocks;
    }

    ফেরত m3Err_none;
}

// Closes the scope's region, while the instruction just read is its end
স্থির
শূন্য RecordBlockEnd (IM3Compilation o)
{
    যদি (o->snapshotMap and o->block.snapshotBlock) {
        o->snapshotMap->blocks[o->block.snapshotBlock - 1].wasmEnd = GetWasmOffset(o);
    }
}

#  undef d_m3GrowSnapshotArray

#অন্যথা

স্থির অন্তঃরেখ
M3_ফলাফল RecordSafePoint (IM3Compilation o, M3SafePointKind i_kind, u32 i_শব্দগুলি_Back, u16 i_অঙ্ক_Results,
                          u16 i_aux, IM3CompilationScope i_loop)
{
    (শূন্য)o;
    (শূন্য)i_kind;
    (শূন্য)i_শব্দগুলি_Back;
    (শূন্য)i_অঙ্ক_Results;
    (শূন্য)i_aux;
    (শূন্য)i_loop;
    ফেরত m3Err_none;
}

স্থির অন্তঃরেখ
M3_ফলাফল RecordBackEdge (IM3Compilation o, IM3CompilationScope i_loop, bool i_isConditional)
{
    (শূন্য)o;
    (শূন্য)i_loop;
    (শূন্য)i_isConditional;
    ফেরত m3Err_none;
}

স্থির অন্তঃরেখ
M3_ফলাফল RecordCall (IM3Compilation o, u16 i_অঙ্ক_Results, u16 i_calleeSlot, bool i_isReturnCall)
{
    (শূন্য)o;
    (শূন্য)i_অঙ্ক_Results;
    (শূন্য)i_calleeSlot;
    (শূন্য)i_isReturnCall;
    ফেরত m3Err_none;
}

#endif // d_m3HasSnapshots

স্থির M3_NOINLINE
M3_ফলাফল Ensure_Code_Page_অঙ্ক_সারিগুলি (IM3Compilation o, u32 i_অঙ্ক_সারিগুলি)
{
    M3_ফলাফল ফলাফল = m3Err_none;

    i_অঙ্ক_সারিগুলি += 2; // room for Bridge

    যদি (M3_UNLIKELY(অঙ্ক_Free_সারিগুলি(o->page) < i_অঙ্ক_সারিগুলি)) {
        IM3CodePage page = AcquireCodePageWithCapacity(o->runtime, i_অঙ্ক_সারিগুলি);

        যদি (page) {
            m3log(emit, "bridging new code page from: %d %p (free slots: %d) to: %d", o->page->info.sequence, GetPC(o), অঙ্ক_Free_সারিগুলি(o->page), page->info.sequence);
            d_m3Assert(অঙ্ক_Free_সারিগুলি(o->page) >= 2);

            EmitWord(o->page, op_Branch);
            EmitWord(o->page, GetPagePC(page));

            ReleaseCodePage(o->runtime, o->page);

            o->page = page;
        } অন্যথা {
            ফলাফল = m3Err_mallocFailedCodePage;
        }
    }

    ফেরত ফলাফল;
}

// invalidate the pending local.set fold candidate; called wherever code is emitted or a
// branch target captures the current pc (a fold appends an immediate, moving that position)
স্থির অন্তঃরেখ
শূন্য InvalidateFold (IM3Compilation o)
{
#যদি d_m3FoldSetLocal
    o->foldPatchPC = ফাঁকা;
#অন্যথা
    (শূন্য)o;
#endif
}

স্থির M3_NOINLINE
M3_ফলাফল EmitOp (IM3Compilation o, IM3Operation i_operation)
{
    M3_ফলাফল ফলাফল = m3Err_none;                                 d_m3Assert (i_operation or Is_স্তূপ_Polymorphic (o));

    InvalidateFold(o);

    // it's OK for page to be null; when compile-walking the bytecode without emitting
    যদি (o->page) {
#যদি d_m3EnableOpTracing
        যদি (i_operation != op_Dump_স্তূপ) {
            o->অঙ্ক_Emits++;
        }
#endif

        // have execution jump to a new page if slots are critically low
        ফলাফল = Ensure_Code_Page_অঙ্ক_সারিগুলি(o, d_m3_Code_Page_Free_সারিগুলি_Threshold);

        যদি (M3_LIKELY(not ফলাফল)) {
            যদি (d_m3LogEmit) {
                log_emit(o, i_operation);
            }
#যদি d_m3RecordBacktraces
            EmitMappingEntry(o->page, (u32)(o->lastOpcodeStart - o->module->wasmStart));
#endif // d_m3RecordBacktraces
            EmitWord(o->page, i_operation);
        }
    }

    ফেরত ফলাফল;
}

// Push an immediate constant into the M3 codestream
স্থির M3_NOINLINE
শূন্য EmitConstant32 (IM3Compilation o, ধ্রুবক u32 i_immediate)
{
    যদি (o->page) {                                                          m3log (emit, "const32: %ud", i_immediate);
        EmitWord32(o->page, i_immediate);
    }
}

#যদি d_m3HasMemory64
// Takes two lines of the code page where a pointer is 32 bits, so whatever
// reads it back has to step _pc by the same amount - see op_Const64.
স্থির M3_NOINLINE
শূন্য EmitConstant64 (IM3Compilation o, ধ্রুবক u64 i_immediate)
{
    যদি (o->page) {
        EmitWord64(o->page, i_immediate);
    }
}
#endif

স্থির M3_NOINLINE
শূন্য EmitSlotOffset (IM3Compilation o, ধ্রুবক i32 i_offset)
{
    যদি (o->page) {                                                          m3log (emit, "slot: [%d]", i_offset);
        EmitWord32(o->page, i_offset);
    }
}

স্থির M3_NOINLINE
pc_t EmitPointer (IM3Compilation o, ধ্রুবক শূন্য* ধ্রুবক i_pointer)
{
    pc_t ptr = GetPagePC(o->page);

    যদি (o->page) {                                                          m3log (emit, "ptr: %p", i_pointer);
        EmitWord(o->page, i_pointer);
    }

    ফেরত ptr;
}

স্থির M3_NOINLINE
শূন্য* ReservePointer (IM3Compilation o)
{
    pc_t ptr = GetPagePC(o->page);
    EmitPointer(o, ফাঁকা);
    ফেরত (শূন্য*)ptr;
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

#যদি d_m3HasFloat
#  define FPOP(x) x
#অন্যথা
#  define FPOP(x) ফাঁকা
#endif

// These are indexed by M3ValueType, so every type up to c_m3Type_externref needs
// an entry. A reference is one pointer-sized word, so it moves with the integer
// operation of that width; v128 has no operations at all.
#যদি M3_SIZEOF_PTR == 8
#  define REFOP(NAME)  op_##NAME##_i64
#অন্যথা
#  define REFOP(NAME)  op_##NAME##_i32
#endif

// clang-format off

স্থির ধ্রুবক IM3Operation c_preserveSetSlot[] = { ফাঁকা, op_PreserveSetSlot_i32,       op_PreserveSetSlot_i64,
                                                    FPOP(op_PreserveSetSlot_f32), FPOP(op_PreserveSetSlot_f64),
                                                    ফাঁকা, REFOP(PreserveSetSlot),  REFOP(PreserveSetSlot),
                                                    REFOP(PreserveSetSlot),       REFOP(PreserveSetSlot) };
স্থির ধ্রুবক IM3Operation c_setSetOps[] =       { ফাঁকা, op_SetSlot_i32,               op_SetSlot_i64,
                                                    FPOP(op_SetSlot_f32),         FPOP(op_SetSlot_f64),
                                                    ফাঁকা, REFOP(SetSlot),         REFOP(SetSlot),
                                                    REFOP(SetSlot),               REFOP(SetSlot) };
স্থির ধ্রুবক IM3Operation c_setGlobalOps[] =    { ফাঁকা, op_SetGlobal_i32,             op_SetGlobal_i64,
                                                    FPOP(op_SetGlobal_f32),       FPOP(op_SetGlobal_f64),
                                                    ফাঁকা, REFOP(SetGlobal),       REFOP(SetGlobal),
                                                    REFOP(SetGlobal),             REFOP(SetGlobal) };
স্থির ধ্রুবক IM3Operation c_setRegisterOps[] =  { ফাঁকা, op_SetRegister_i32,           op_SetRegister_i64,
                                                    FPOP(op_SetRegister_f32),     FPOP(op_SetRegister_f64),
                                                    ফাঁকা, REFOP(SetRegister),     REFOP(SetRegister),
                                                    REFOP(SetRegister),           REFOP(SetRegister) };

// A table shorter than the enum reads out of bounds, so tie the two together at
// build time: adding a value type must not silently outgrow these.
#define d_m3CheckTypeTable(TABLE) \
    M3_STATIC_ASSERT (আকার (TABLE) / আকার (*(TABLE)) == c_m3_Type_গুনতি, TABLE##_needs_one_entry_per_type)

d_m3CheckTypeTable(c_preserveSetSlot);
d_m3CheckTypeTable(c_setSetOps);
d_m3CheckTypeTable(c_setGlobalOps);
d_m3CheckTypeTable(c_setRegisterOps);

#যদি d_m3FoldSetLocal

// destination-folded variants of the hot binops, indexed by the operand-form index
// recorded at emission: [0]=_rs [1]=_sr [2]=_ss [3]=(fp _rr, never folded)
#define d_foldOpList(TYPE, NAME)            { op_##TYPE##_##NAME##_rs_f, op_##TYPE##_##NAME##_sr_f, op_##TYPE##_##NAME##_ss_f, ফাঁকা }
#define d_foldCommutativeOpList(TYPE, NAME) { op_##TYPE##_##NAME##_rs_f, ফাঁকা,                      op_##TYPE##_##NAME##_ss_f, ফাঁকা }

স্থির ধ্রুবক IM3Operation c_fold_i32_Add[]         = d_foldCommutativeOpList (i32, Add);
স্থির ধ্রুবক IM3Operation c_fold_i32_Subtract[]    = d_foldOpList (i32, Subtract);
স্থির ধ্রুবক IM3Operation c_fold_i32_Multiply[]    = d_foldCommutativeOpList (i32, Multiply);
স্থির ধ্রুবক IM3Operation c_fold_u32_And[]         = d_foldCommutativeOpList (u32, And);
স্থির ধ্রুবক IM3Operation c_fold_u32_Or[]          = d_foldCommutativeOpList (u32, Or);
স্থির ধ্রুবক IM3Operation c_fold_u32_Xor[]         = d_foldCommutativeOpList (u32, Xor);
স্থির ধ্রুবক IM3Operation c_fold_u32_Shift_বাম[]   = d_foldOpList (u32, Shift_বাম);
স্থির ধ্রুবক IM3Operation c_fold_i32_Shift_ডান[]  = d_foldOpList (i32, Shift_ডান);
স্থির ধ্রুবক IM3Operation c_fold_u32_Shift_ডান[]  = d_foldOpList (u32, Shift_ডান);

স্থির ধ্রুবক IM3Operation c_fold_i64_Add[]         = d_foldCommutativeOpList (i64, Add);
স্থির ধ্রুবক IM3Operation c_fold_i64_Subtract[]    = d_foldOpList (i64, Subtract);
স্থির ধ্রুবক IM3Operation c_fold_i64_Multiply[]    = d_foldCommutativeOpList (i64, Multiply);
স্থির ধ্রুবক IM3Operation c_fold_u64_And[]         = d_foldCommutativeOpList (u64, And);
স্থির ধ্রুবক IM3Operation c_fold_u64_Or[]          = d_foldCommutativeOpList (u64, Or);
স্থির ধ্রুবক IM3Operation c_fold_u64_Xor[]         = d_foldCommutativeOpList (u64, Xor);
স্থির ধ্রুবক IM3Operation c_fold_u64_Shift_বাম[]   = d_foldOpList (u64, Shift_বাম);
স্থির ধ্রুবক IM3Operation c_fold_i64_Shift_ডান[]  = d_foldOpList (i64, Shift_ডান);
স্থির ধ্রুবক IM3Operation c_fold_u64_Shift_ডান[]  = d_foldOpList (u64, Shift_ডান);

#  যদি d_m3HasFloat
স্থির ধ্রুবক IM3Operation c_fold_f32_Add[]         = d_foldCommutativeOpList (f32, Add);
স্থির ধ্রুবক IM3Operation c_fold_f32_Subtract[]    = d_foldOpList (f32, Subtract);
স্থির ধ্রুবক IM3Operation c_fold_f32_Multiply[]    = d_foldCommutativeOpList (f32, Multiply);
স্থির ধ্রুবক IM3Operation c_fold_f32_Divide[]      = d_foldOpList (f32, Divide);
স্থির ধ্রুবক IM3Operation c_fold_f64_Add[]         = d_foldCommutativeOpList (f64, Add);
স্থির ধ্রুবক IM3Operation c_fold_f64_Subtract[]    = d_foldOpList (f64, Subtract);
স্থির ধ্রুবক IM3Operation c_fold_f64_Multiply[]    = d_foldCommutativeOpList (f64, Multiply);
স্থির ধ্রুবক IM3Operation c_fold_f64_Divide[]      = d_foldOpList (f64, Divide);
#  endif

// loads are unary: [0]= address in _r0, [1]= address in a slot
#define d_foldLoadList(DEST, SRC)       { op_##DEST##_Load_##SRC##_r_f, op_##DEST##_Load_##SRC##_s_f, ফাঁকা, ফাঁকা }

স্থির ধ্রুবক IM3Operation c_fold_i32_Load_i32[] = d_foldLoadList(i32, i32);
স্থির ধ্রুবক IM3Operation c_fold_i32_Load_i8[] = d_foldLoadList(i32, i8);
স্থির ধ্রুবক IM3Operation c_fold_i32_Load_u8[] = d_foldLoadList(i32, u8);
স্থির ধ্রুবক IM3Operation c_fold_i32_Load_i16[] = d_foldLoadList(i32, i16);
স্থির ধ্রুবক IM3Operation c_fold_i32_Load_u16[] = d_foldLoadList(i32, u16);

স্থির ধ্রুবক IM3Operation c_fold_i64_Load_i64[] = d_foldLoadList(i64, i64);
স্থির ধ্রুবক IM3Operation c_fold_i64_Load_i8[] = d_foldLoadList(i64, i8);
স্থির ধ্রুবক IM3Operation c_fold_i64_Load_u8[] = d_foldLoadList(i64, u8);
স্থির ধ্রুবক IM3Operation c_fold_i64_Load_i16[] = d_foldLoadList(i64, i16);
স্থির ধ্রুবক IM3Operation c_fold_i64_Load_u16[] = d_foldLoadList(i64, u16);
স্থির ধ্রুবক IM3Operation c_fold_i64_Load_i32[] = d_foldLoadList(i64, i32);
স্থির ধ্রুবক IM3Operation c_fold_i64_Load_u32[] = d_foldLoadList(i64, u32);
#  যদি d_m3HasFloat
স্থির ধ্রুবক IM3Operation c_fold_f32_Load_f32[] = d_foldLoadList(f32, f32);
স্থির ধ্রুবক IM3Operation c_fold_f64_Load_f64[] = d_foldLoadList(f64, f64);
#  endif

স্থির
IM3Operation GetFoldOp (m3opcode_t i_opcode, u8 i_form)
{
    যদি (i_form >= 4) {
        ফেরত ফাঁকা;
    }

    নির্বাচন (i_opcode) {
    ক্ষেত্র 0x6a:  ফেরত c_fold_i32_Add        [i_form];      // i32.add
    ক্ষেত্র 0x6b:  ফেরত c_fold_i32_Subtract   [i_form];      // i32.sub
    ক্ষেত্র 0x6c:  ফেরত c_fold_i32_Multiply   [i_form];      // i32.mul
    ক্ষেত্র 0x71:  ফেরত c_fold_u32_And        [i_form];      // i32.and
    ক্ষেত্র 0x72:  ফেরত c_fold_u32_Or         [i_form];      // i32.or
    ক্ষেত্র 0x73:  ফেরত c_fold_u32_Xor        [i_form];      // i32.xor
    ক্ষেত্র 0x74:  ফেরত c_fold_u32_Shift_বাম  [i_form];      // i32.shl
    ক্ষেত্র 0x75:  ফেরত c_fold_i32_Shift_ডান [i_form];      // i32.shr_s
    ক্ষেত্র 0x76:  ফেরত c_fold_u32_Shift_ডান [i_form];      // i32.shr_u

    ক্ষেত্র 0x7c:  ফেরত c_fold_i64_Add        [i_form];      // i64.add
    ক্ষেত্র 0x7d:  ফেরত c_fold_i64_Subtract   [i_form];      // i64.sub
    ক্ষেত্র 0x7e:  ফেরত c_fold_i64_Multiply   [i_form];      // i64.mul
    ক্ষেত্র 0x83:  ফেরত c_fold_u64_And        [i_form];      // i64.and
    ক্ষেত্র 0x84:  ফেরত c_fold_u64_Or         [i_form];      // i64.or
    ক্ষেত্র 0x85:  ফেরত c_fold_u64_Xor        [i_form];      // i64.xor
    ক্ষেত্র 0x86:  ফেরত c_fold_u64_Shift_বাম  [i_form];      // i64.shl
    ক্ষেত্র 0x87:  ফেরত c_fold_i64_Shift_ডান [i_form];      // i64.shr_s
    ক্ষেত্র 0x88:  ফেরত c_fold_u64_Shift_ডান [i_form];      // i64.shr_u

#  যদি d_m3HasFloat
    ক্ষেত্র 0x92:  ফেরত c_fold_f32_Add        [i_form];      // f32.add
    ক্ষেত্র 0x93:  ফেরত c_fold_f32_Subtract   [i_form];      // f32.sub
    ক্ষেত্র 0x94:  ফেরত c_fold_f32_Multiply   [i_form];      // f32.mul
    ক্ষেত্র 0x95:  ফেরত c_fold_f32_Divide     [i_form];      // f32.div
    ক্ষেত্র 0xa0:  ফেরত c_fold_f64_Add        [i_form];      // f64.add
    ক্ষেত্র 0xa1:  ফেরত c_fold_f64_Subtract   [i_form];      // f64.sub
    ক্ষেত্র 0xa2:  ফেরত c_fold_f64_Multiply   [i_form];      // f64.mul
    ক্ষেত্র 0xa3:  ফেরত c_fold_f64_Divide     [i_form];      // f64.div
#  endif

    ক্ষেত্র 0x28:  ফেরত c_fold_i32_Load_i32   [i_form];      // i32.load
    ক্ষেত্র 0x2c:  ফেরত c_fold_i32_Load_i8    [i_form];      // i32.load8_s
    ক্ষেত্র 0x2d:  ফেরত c_fold_i32_Load_u8    [i_form];      // i32.load8_u
    ক্ষেত্র 0x2e:  ফেরত c_fold_i32_Load_i16   [i_form];      // i32.load16_s
    ক্ষেত্র 0x2f:  ফেরত c_fold_i32_Load_u16   [i_form];      // i32.load16_u

    ক্ষেত্র 0x29:  ফেরত c_fold_i64_Load_i64   [i_form];      // i64.load
    ক্ষেত্র 0x30:  ফেরত c_fold_i64_Load_i8    [i_form];      // i64.load8_s
    ক্ষেত্র 0x31:  ফেরত c_fold_i64_Load_u8    [i_form];      // i64.load8_u
    ক্ষেত্র 0x32:  ফেরত c_fold_i64_Load_i16   [i_form];      // i64.load16_s
    ক্ষেত্র 0x33:  ফেরত c_fold_i64_Load_u16   [i_form];      // i64.load16_u
    ক্ষেত্র 0x34:  ফেরত c_fold_i64_Load_i32   [i_form];      // i64.load32_s
    ক্ষেত্র 0x35:  ফেরত c_fold_i64_Load_u32   [i_form];      // i64.load32_u

#  যদি d_m3HasFloat
    ক্ষেত্র 0x2a:  ফেরত c_fold_f32_Load_f32   [i_form];      // f32.load
    ক্ষেত্র 0x2b:  ফেরত c_fold_f64_Load_f64   [i_form];      // f64.load
#  endif

    পূর্বনির্ধারিত: ফেরত ফাঁকা;
    }
}

#endif // d_m3FoldSetLocal

// clang-format on

#যদি d_m3FuseBranch

// fused compare+branch/if variants, indexed like the fold lists: [0]=_rs [1]=_sr [2]=_ss [3]=unused.
// the [1] entries are NULL for commutative compares: the compiler never records the _sr form for them
#  define d_fuseCmpList(TYPE, NAME, KIND)             { op_##TYPE##_##KIND##_##NAME##_rs, op_##TYPE##_##KIND##_##NAME##_sr, op_##TYPE##_##KIND##_##NAME##_ss, ফাঁকা }
#  define d_fuseCommutativeCmpList(TYPE, NAME, KIND)  { op_##TYPE##_##KIND##_##NAME##_rs, ফাঁকা,                             op_##TYPE##_##KIND##_##NAME##_ss, ফাঁকা }

প্রকারলেখ কাঠামো M3FusedCmpOps {
    IM3Operation branchIf[4];
    IM3Operation ifOp[4];
} M3FusedCmpOps;

#  define d_fuseCmp(TYPE, NAME)               { d_fuseCmpList (TYPE, NAME, BranchIf),            d_fuseCmpList (TYPE, NAME, If) }
#  define d_fuseCommutativeCmp(TYPE, NAME)    { d_fuseCommutativeCmpList (TYPE, NAME, BranchIf), d_fuseCommutativeCmpList (TYPE, NAME, If) }

// clang-format off
স্থির ধ্রুবক M3FusedCmpOps c_fuse_i32_Equal              = d_fuseCommutativeCmp (i32, Equal);
স্থির ধ্রুবক M3FusedCmpOps c_fuse_i32_NotEqual           = d_fuseCommutativeCmp (i32, NotEqual);
স্থির ধ্রুবক M3FusedCmpOps c_fuse_i32_LessThan           = d_fuseCmp (i32, LessThan);
স্থির ধ্রুবক M3FusedCmpOps c_fuse_u32_LessThan           = d_fuseCmp (u32, LessThan);
স্থির ধ্রুবক M3FusedCmpOps c_fuse_i32_GreaterThan        = d_fuseCmp (i32, GreaterThan);
স্থির ধ্রুবক M3FusedCmpOps c_fuse_u32_GreaterThan        = d_fuseCmp (u32, GreaterThan);
স্থির ধ্রুবক M3FusedCmpOps c_fuse_i32_LessThanOrEqual    = d_fuseCmp (i32, LessThanOrEqual);
স্থির ধ্রুবক M3FusedCmpOps c_fuse_u32_LessThanOrEqual    = d_fuseCmp (u32, LessThanOrEqual);
স্থির ধ্রুবক M3FusedCmpOps c_fuse_i32_GreaterThanOrEqual = d_fuseCmp (i32, GreaterThanOrEqual);
স্থির ধ্রুবক M3FusedCmpOps c_fuse_u32_GreaterThanOrEqual = d_fuseCmp (u32, GreaterThanOrEqual);
// clang-format on

// i32.eqz is unary: [0]= operand in _r0, [1]= operand in a slot
স্থির ধ্রুবক M3FusedCmpOps c_fuse_i32_Eqz = {
    { op_i32_BranchIfEqz_r, op_i32_BranchIfEqz_s, ফাঁকা, ফাঁকা },
    { op_i32_IfEqz_r,       op_i32_IfEqz_s,       ফাঁকা, ফাঁকা }
};

স্থির
ধ্রুবক M3FusedCmpOps* GetFusedCmpOps (m3opcode_t i_opcode)
{
    নির্বাচন (i_opcode) {
    ক্ষেত্র 0x45: ফেরত &c_fuse_i32_Eqz;                    // i32.eqz
    ক্ষেত্র 0x46: ফেরত &c_fuse_i32_Equal;                  // i32.eq
    ক্ষেত্র 0x47: ফেরত &c_fuse_i32_NotEqual;               // i32.ne
    ক্ষেত্র 0x48: ফেরত &c_fuse_i32_LessThan;               // i32.lt_s
    ক্ষেত্র 0x49: ফেরত &c_fuse_u32_LessThan;               // i32.lt_u
    ক্ষেত্র 0x4a: ফেরত &c_fuse_i32_GreaterThan;            // i32.gt_s
    ক্ষেত্র 0x4b: ফেরত &c_fuse_u32_GreaterThan;            // i32.gt_u
    ক্ষেত্র 0x4c: ফেরত &c_fuse_i32_LessThanOrEqual;        // i32.le_s
    ক্ষেত্র 0x4d: ফেরত &c_fuse_u32_LessThanOrEqual;        // i32.le_u
    ক্ষেত্র 0x4e: ফেরত &c_fuse_i32_GreaterThanOrEqual;     // i32.ge_s
    ক্ষেত্র 0x4f: ফেরত &c_fuse_u32_GreaterThanOrEqual;     // i32.ge_u
    পূর্বনির্ধারিত: ফেরত ফাঁকা;
    }
}

#endif // d_m3FuseBranch

স্থির ধ্রুবক IM3Operation c_intSelectOps[2][4] = {
    { op_Select_i32_rss, op_Select_i32_srs, op_Select_i32_ssr, op_Select_i32_sss },
    { op_Select_i64_rss, op_Select_i64_srs, op_Select_i64_ssr, op_Select_i64_sss }
};
#যদি d_m3HasFloat
স্থির ধ্রুবক IM3Operation c_fpSelectOps[2][2][3] = {
    { { op_Select_f32_sss, op_Select_f32_srs, op_Select_f32_ssr }, // selector in slot
      { op_Select_f32_rss, op_Select_f32_rrs, op_Select_f32_rsr } }, // selector in reg
    { { op_Select_f64_sss, op_Select_f64_srs, op_Select_f64_ssr }, // selector in slot
      { op_Select_f64_rss, op_Select_f64_rrs, op_Select_f64_rsr } }
};    // selector in reg
#endif

// all args & returns are 64-bit aligned, so use 2 slots for a d_m3Use32BitSlots=1 build

স্থির
M3_ফলাফল AcquireCompilationCodePage (IM3Compilation o, IM3CodePage* o_codePage)
{
    M3_ফলাফল ফলাফল = m3Err_none;

    IM3CodePage page = AcquireCodePage(o->runtime);

    যদি (page) {
#যদি (d_m3EnableCodePageRefCounting)
        {
            যদি (o->function) {
                IM3Function func = o->function;
                page->info.usage_গুনতি++;

                u32 index = func->অঙ্ক_Code_Page_Refs++;
_               (m3ReallocArray(&func->codePageRefs, IM3CodePage, func->অঙ্ক_Code_Page_Refs, index));
                func->codePageRefs[index] = page;
            }
        }
#endif
    } অন্যথা {
        _throw(m3Err_mallocFailedCodePage);
    }

    _catch:

    *o_codePage = page;

    ফেরত ফলাফল;
}

স্থির অন্তঃরেখ
শূন্য ReleaseCompilationCodePage (IM3Compilation o)
{
    ReleaseCodePage(o->runtime, o->page);
}

স্থির অন্তঃরেখ
u16 Get_Type_অঙ্ক_Slots (m3type_t i_type)
{
    i_type = BaseTypeOf(i_type);

    // v128 is 16 bytes - 4 slots in 32-bit-slot mode, 2 in 64-bit.
    // (Slot-allocator only; no v128 ops execute.)
    যদি (i_type == c_m3Type_v128)
#যদি d_m3Use32BitSlots
        ফেরত 4;
#অন্যথা
        ফেরত 2;
#endif
#যদি d_m3Use32BitSlots
    ফেরত Is64BitType(i_type) ? 2 : 1;
#অন্যথা
    ফেরত 1;
#endif
}

স্থির অন্তঃরেখ
শূন্য AlignSlotToType (u16* io_slot, m3type_t i_type)
{
    // align 64-bit words to even slots (if d_m3Use32BitSlots)
    u16 অঙ্ক_Slots = Get_Type_অঙ্ক_Slots(i_type);

    u16 mask = অঙ্ক_Slots - 1;
    *io_slot = (*io_slot + mask) & ~mask;
}

// Deliberately unchecked. Validation rejects an empty operand stack before compilation.
স্থির অন্তঃরেখ
i16 Get_স্তূপ_শীর্ষ_সূচক (IM3Compilation o)
{
    ফেরত o->স্তূপ_সূচক - 1;
}

স্থির অন্তঃরেখ
M3_ফলাফল Get_স্তূপ_শীর্ষ_সূচক_Throws (IM3Compilation o, i16* o_স্তূপ_সূচক)
{
    *o_স্তূপ_সূচক = o->স্তূপ_সূচক - 1;

    যদি (M3_LIKELY(o->স্তূপ_সূচক > o->স্তূপ_First_Dynamic_সূচক or Is_স্তূপ_Polymorphic(o))) {
        ফেরত m3Err_none;
    } অন্যথা {
        ফেরত m3_Err_function_স্তূপ_Underrun;
    }
}


// Items in the static portion of the stack (args/locals) are hidden from GetStackTypeFromTop ()
// In other words, only "real" Wasm stack items can be inspected.  This is important when
// returning values, etc. and you need an accurate wasm-view of the stack.
স্থির
m3type_t Get_স্তূপ_Type_From_শীর্ষ (IM3Compilation o, u16 i_offset)
{
    m3type_t type = c_m3Type_none;

    ++i_offset;
    যদি (o->স্তূপ_সূচক >= i_offset) {
        u16 index = o->স্তূপ_সূচক - i_offset;

        যদি (index >= o->স্তূপ_First_Dynamic_সূচক) {
            type = o->type_স্তূপ[index];
        }
    }

    ফেরত type;
}

স্থির অন্তঃরেখ
m3type_t Get_স্তূপ_শীর্ষ_Type (IM3Compilation o)
{
    ফেরত Get_স্তূপ_Type_From_শীর্ষ(o, 0);
}

স্থির অন্তঃরেখ
m3type_t Get_স্তূপ_Type_From_Bottom (IM3Compilation o, u16 i_offset)
{
    m3type_t type = c_m3Type_none;

    যদি (i_offset < o->স্তূপ_সূচক) {
        type = o->type_স্তূপ[i_offset];
    }

    ফেরত type;
}


স্থির অন্তঃরেখ
bool IsConstantSlot (IM3Compilation o, u16 i_slot)
{
    ফেরত (i_slot >= o->slot_First_Const_সূচক and i_slot < o->slot_Max_Const_সূচক);
}
স্থির অন্তঃরেখ
bool IsSlotAllocated (IM3Compilation o, u16 i_slot)
{
    ফেরত o->m3Slots[i_slot];
}

স্থির অন্তঃরেখ
bool Is_স্তূপ_সূচক_In_Register (IM3Compilation o, i32 i_স্তূপ_সূচক)
{                                                                           d_m3Assert (i_স্তূপ_সূচক < o->স্তূপ_সূচক or Is_স্তূপ_Polymorphic (o));
    যদি (i_স্তূপ_সূচক >= 0 and i_স্তূপ_সূচক < o->স্তূপ_সূচক) {
        ফেরত (o->wasm_স্তূপ[i_স্তূপ_সূচক] >= d_m3Reg0SlotAlias);
    } অন্যথা {
        ফেরত false;
    }
}

স্থির অন্তঃরেখ
u16 Get_অঙ্ক_Block_মানসমূহ_On_স্তূপ (IM3Compilation o)
{
    ফেরত o->স্তূপ_সূচক - o->block.block_স্তূপ_সূচক;
}

স্থির অন্তঃরেখ
bool Is_স্তূপ_শীর্ষ_In_Register (IM3Compilation o)
{
    ফেরত Is_স্তূপ_সূচক_In_Register(o, (i32)Get_স্তূপ_শীর্ষ_সূচক(o));
}
স্থির অন্তঃরেখ
bool Is_স্তূপ_শীর্ষ_Minus1_In_Register (IM3Compilation o)
{
    ফেরত Is_স্তূপ_সূচক_In_Register(o, (i32)Get_স্তূপ_শীর্ষ_সূচক(o) - 1);
}
স্থির অন্তঃরেখ
bool Is_স্তূপ_শীর্ষ_Minus2_In_Register (IM3Compilation o)
{
    ফেরত Is_স্তূপ_সূচক_In_Register(o, (i32)Get_স্তূপ_শীর্ষ_সূচক(o) - 2);
}

স্থির অন্তঃরেখ
bool Is_স্তূপ_শীর্ষ_In_Slot (IM3Compilation o)
{
    ফেরত not Is_স্তূপ_শীর্ষ_In_Register(o);
}

স্থির অন্তঃরেখ
bool IsValidSlot (u16 i_slot)
{
    ফেরত (i_slot < d_m3MaxFunctionSlots);
}

স্থির অন্তঃরেখ
u16 Get_স্তূপ_শীর্ষ_Slot_সংখ্যা (IM3Compilation o)
{
    i16 i = Get_স্তূপ_শীর্ষ_সূচক(o);

    u16 slot = c_slotUnused;

    যদি (i >= 0) {
        slot = o->wasm_স্তূপ[i];
    }

    ফেরত slot;
}


// from bottom
স্থির অন্তঃরেখ
u16 Get_Slot_For_স্তূপ_সূচক (IM3Compilation o, u16 i_স্তূপ_সূচক)
{                                                                   d_m3Assert (i_স্তূপ_সূচক < o->স্তূপ_সূচক or Is_স্তূপ_Polymorphic (o));
    u16 slot = c_slotUnused;

    যদি (i_স্তূপ_সূচক < o->স্তূপ_সূচক) {
        slot = o->wasm_স্তূপ[i_স্তূপ_সূচক];
    }

    ফেরত slot;
}

স্থির অন্তঃরেখ
u16 Get_Extra_Slot_For_স্তূপ_সূচক (IM3Compilation o, u16 i_স্তূপ_সূচক)
{
    u16 baseSlot = Get_Slot_For_স্তূপ_সূচক(o, i_স্তূপ_সূচক);

    যদি (baseSlot != c_slotUnused) {
        u16 extraSlot = Get_Type_অঙ্ক_Slots(Get_স্তূপ_Type_From_Bottom(o, i_স্তূপ_সূচক)) - 1;
        baseSlot += extraSlot;
    }

    ফেরত baseSlot;
}


স্থির অন্তঃরেখ
শূন্য TouchSlot (IM3Compilation o, u16 i_slot)
{
    // op_Entry uses this value to track and detect stack overflow
    o->max_স্তূপ_Slots = M3_MAX(o->max_স্তূপ_Slots, i_slot + 1);
}

স্থির অন্তঃরেখ
শূন্য MarkSlotAllocated (IM3Compilation o, u16 i_slot)
{                                                                   d_m3Assert (o->m3Slots [i_slot] == 0); // shouldn't be already allocated
    o->m3Slots[i_slot] = 1;

    o->slot_Max_Allocated_সূচক_Plus_One = M3_MAX(o->slot_Max_Allocated_সূচক_Plus_One, i_slot + 1);

    TouchSlot(o, i_slot);
}

স্থির অন্তঃরেখ
M3_ফলাফল MarkSlotsAllocated (IM3Compilation o, u16 i_slot, u16 i_অঙ্ক_Slots)
{
    যদি (M3_UNLIKELY(i_slot + i_অঙ্ক_Slots > d_m3MaxFunctionSlots)) {
        ফেরত m3_Err_function_স্তূপ_Overflow;
    }

    যখন (i_অঙ্ক_Slots--) {
        MarkSlotAllocated(o, i_slot++);
    }

    ফেরত m3Err_none;
}

স্থির অন্তঃরেখ
M3_ফলাফল MarkSlotsAllocatedByType (IM3Compilation o, u16 i_slot, m3type_t i_type)
{
    u16 অঙ্ক_Slots = Get_Type_অঙ্ক_Slots(i_type);
    ফেরত MarkSlotsAllocated(o, i_slot, অঙ্ক_Slots);
}


স্থির
M3_ফলাফল AllocateSlotsWithinRange (IM3Compilation o, u16* o_slot, m3type_t i_type, u16 i_startSlot, u16 i_endSlot)
{
    M3_ফলাফল ফলাফল = m3_Err_function_স্তূপ_Overflow;

    u16 অঙ্ক_Slots     = Get_Type_অঙ্ক_Slots(i_type);
    u16 অনুসন্ধান_Offset = অঙ্ক_Slots - 1;

    AlignSlotToType(&i_startSlot, i_type);

    // search for 1 or 2 consecutive slots in the execution stack
    u16 i = i_startSlot;
    যখন (i + অনুসন্ধান_Offset < i_endSlot) {
        যদি (i + অনুসন্ধান_Offset < d_m3MaxFunctionSlots and o->m3Slots[i] == 0 and o->m3Slots[i + অনুসন্ধান_Offset] == 0) {
            MarkSlotsAllocated(o, i, অঙ্ক_Slots);

            *o_slot = i;
            ফলাফল  = m3Err_none;
            বিরতি;
        }

        // keep 2-slot allocations even-aligned
        i += অঙ্ক_Slots;
    }

    ফেরত ফলাফল;
}

স্থির অন্তঃরেখ
M3_ফলাফল AllocateSlots (IM3Compilation o, u16* o_slot, m3type_t i_type)
{
    ফেরত AllocateSlotsWithinRange(o, o_slot, i_type, o->slot_First_Dynamic_সূচক, d_m3MaxFunctionSlots);
}

স্থির অন্তঃরেখ
M3_ফলাফল AllocateConstantSlots (IM3Compilation o, u16* o_slot, m3type_t i_type)
{
    u16 max_Table_সূচক = o->slot_First_Const_সূচক + d_m3_Max_Constant_Table_আকার;
    ফেরত AllocateSlotsWithinRange(o, o_slot, i_type, o->slot_First_Const_সূচক, M3_MIN(o->slot_First_Dynamic_সূচক, max_Table_সূচক));
}


// TOQUE: this usage count system could be eliminated. real world code doesn't frequently trigger it.  just copy to multiple
// unique slots.
স্থির অন্তঃরেখ
M3_ফলাফল Increment_Slot_Usage_গুনতি (IM3Compilation o, u16 i_slot)
{                                                                                       d_m3Assert (i_slot < d_m3MaxFunctionSlots);
    M3_ফলাফল ফলাফল = m3Err_none;                                                       d_m3Assert (o->m3Slots [i_slot] > 0);

    // OPTZ (memory): 'm3Slots' could still be fused with 'typeStack' if 4 bits were used to indicate: [0,1,2,many]. The many-case
    // would scan 'wasmStack' to determine the actual usage count
    যদি (M3_LIKELY(o->m3Slots[i_slot] < 0xFF)) {
        o->m3Slots[i_slot]++;
    } অন্যথা {
        ফলাফল = "slot usage count overflow";
    }

    ফেরত ফলাফল;
}

স্থির অন্তঃরেখ
শূন্য DeallocateSlot (IM3Compilation o, i16 i_slot, m3type_t i_type)
{                                                                                       d_m3Assert (i_slot >= o->slot_First_Dynamic_সূচক);
                                                                                        d_m3Assert (i_slot < o->slot_Max_Allocated_সূচক_Plus_One);
    জন্য (u16 i = 0; i < Get_Type_অঙ্ক_Slots(i_type); ++i, ++i_slot) {                       d_m3Assert (o->m3Slots [i_slot]);
        --o->m3Slots[i_slot];
    }
}


স্থির অন্তঃরেখ
bool IsRegisterTypeAllocated (IM3Compilation o, u8 i_type)
{
    ফেরত IsRegisterAllocated(o, IsFpType(i_type));
}

স্থির অন্তঃরেখ
শূন্য AllocateRegister (IM3Compilation o, u32 i_register, u16 i_স্তূপ_সূচক)
{                                                                                       d_m3Assert (not IsRegisterAllocated (o, i_register));
    o->reg_স্তূপ_সূচক_Plus_One[i_register] = i_স্তূপ_সূচক + 1;
}

স্থির অন্তঃরেখ
শূন্য DeallocateRegister (IM3Compilation o, u32 i_register)
{                                                                                       d_m3Assert (IsRegisterAllocated (o, i_register));
    o->reg_স্তূপ_সূচক_Plus_One[i_register] = c_m3RegisterUnallocated;
}

স্থির অন্তঃরেখ
u16 Get_Register_স্তূপ_সূচক (IM3Compilation o, u32 i_register)
{                                                                                       d_m3Assert (IsRegisterAllocated (o, i_register));
    ফেরত o->reg_স্তূপ_সূচক_Plus_One[i_register] - 1;
}

u16 GetMaxUsedSlotPlusOne (IM3Compilation o)
{
    যখন (o->slot_Max_Allocated_সূচক_Plus_One > o->slot_First_Dynamic_সূচক) {
        যদি (IsSlotAllocated(o, o->slot_Max_Allocated_সূচক_Plus_One - 1)) {
            বিরতি;
        }

        o->slot_Max_Allocated_সূচক_Plus_One--;
    }

#ifdef DEBUG
    u16 maxSlot = o->slot_Max_Allocated_সূচক_Plus_One;
    যখন (maxSlot < d_m3MaxFunctionSlots) {
        d_m3Assert(o->m3Slots[maxSlot] == 0);
        maxSlot++;
    }
#endif

    ফেরত o->slot_Max_Allocated_সূচক_Plus_One;
}

স্থির
M3_ফলাফল PreserveRegisterIfOccupied (IM3Compilation o, m3type_t i_registerType)
{
    M3_ফলাফল ফলাফল = m3Err_none;

    u32 regSelect = IsFpType(i_registerType);

    যদি (IsRegisterAllocated(o, regSelect)) {
        u16 স্তূপ_সূচক = Get_Register_স্তূপ_সূচক(o, regSelect);
        DeallocateRegister(o, regSelect);

        m3type_t type = Get_স্তূপ_Type_From_Bottom(o, স্তূপ_সূচক);

        // and point to a exec slot
        u16 slot = c_slotUnused;
_       (AllocateSlots(o, &slot, type));
        o->wasm_স্তূপ[স্তূপ_সূচক] = slot;

        // Ensure type is within the valid range
        যদি (M3_LIKELY(BaseTypeOf(type) < c_m3_Type_গুনতি)) {
_           (EmitOp(o, c_setSetOps[BaseTypeOf(type)]));
        } অন্যথা {
            _throw(m3Err_unknownType);
        }

        EmitSlotOffset(o, slot);
    }

    _catch: ফেরত ফলাফল;
}


// all values must be in slots before entering loop, if, and else blocks
// otherwise they'd end up preserve-copied in the block to probably different locations (if/else)
স্থির অন্তঃরেখ
M3_ফলাফল PreserveRegisters (IM3Compilation o)
{
    M3_ফলাফল ফলাফল;

_   (PreserveRegisterIfOccupied(o, c_m3Type_f64));
_   (PreserveRegisterIfOccupied(o, c_m3Type_i64));

    _catch: ফেরত ফলাফল;
}

স্থির
M3_ফলাফল Preserve_Non_শীর্ষ_Registers (IM3Compilation o)
{
    M3_ফলাফল ফলাফল = m3Err_none;

    i16 স্তূপ_শীর্ষ = Get_স্তূপ_শীর্ষ_সূচক(o);

    যদি (স্তূপ_শীর্ষ >= 0) {
        যদি (IsRegisterAllocated(o, 0))     // r0
        {
            যদি (Get_Register_স্তূপ_সূচক(o, 0) != স্তূপ_শীর্ষ) {
_               (PreserveRegisterIfOccupied(o, c_m3Type_i64));
            }
        }

        যদি (IsRegisterAllocated(o, 1))     // fp0
        {
            যদি (Get_Register_স্তূপ_সূচক(o, 1) != স্তূপ_শীর্ষ) {
_               (PreserveRegisterIfOccupied(o, c_m3Type_f64));
            }
        }
    }

    _catch: ফেরত ফলাফল;
}


//----------------------------------------------------------------------------------------------------------------------

স্থির
M3_ফলাফল ঠেলো (IM3Compilation o, m3type_t i_type, u16 i_slot)
{
    M3_ফলাফল ফলাফল = m3Err_none;

#যদি !d_m3HasFloat
    যদি (i_type == c_m3Type_f32 || i_type == c_m3Type_f64) {
        ফেরত m3Err_unknownOpcode;
    }
#endif

    যদি (M3_UNLIKELY(o->স্তূপ_সূচক >= d_m3_Max_Function_স্তূপ_Height)) {
        ফেরত m3_Err_function_স্তূপ_Overflow;
    }

    u16 স্তূপ_সূচক = o->স্তূপ_সূচক++;                                       // printf ("push: %d\n", (i32) i);

    o->wasm_স্তূপ[স্তূপ_সূচক] = i_slot;
    o->type_স্তূপ[স্তূপ_সূচক] = i_type;

    যদি (IsRegisterSlotAlias(i_slot)) {
        u32 regSelect = IsFpRegisterSlotAlias(i_slot);
        AllocateRegister(o, regSelect, স্তূপ_সূচক);
    }

    যদি (d_m3_Log_Wasm_স্তূপ) {
        dump_type_স্তূপ(o);
    }

    ফেরত ফলাফল;
}

স্থির অন্তঃরেখ
M3_ফলাফল ঠেলো_Register (IM3Compilation o, m3type_t i_type)
{
    M3_ফলাফল ফলাফল = m3Err_none;                                                       d_m3Assert ((u16) d_m3Reg0SlotAlias > (u16) d_m3MaxFunctionSlots);
    u16      slot   = IsFpType(i_type) ? d_m3Fp0SlotAlias : d_m3Reg0SlotAlias;          d_m3Assert (i_type or Is_স্তূপ_Polymorphic (o));

_   (ঠেলো(o, i_type, slot));

    _catch: ফেরত ফলাফল;
}

স্থির
M3_ফলাফল বের_করো (IM3Compilation o)
{
    M3_ফলাফল ফলাফল = m3Err_none;

    যদি (M3_LIKELY(o->স্তূপ_সূচক > o->block.block_স্তূপ_সূচক)) {
        o->স্তূপ_সূচক--;                                                //  printf ("pop: %d\n", (i32) o->stackIndex);

        u16      slot = o->wasm_স্তূপ[o->স্তূপ_সূচক];
        m3type_t type = o->type_স্তূপ[o->স্তূপ_সূচক];

        যদি (IsRegisterSlotAlias(slot)) {
            u32 regSelect = IsFpRegisterSlotAlias(slot);
            DeallocateRegister(o, regSelect);
        } অন্যথা যদি (M3_UNLIKELY(slot >= o->slot_Max_Allocated_সূচক_Plus_One)) {
            ফেরত m3_Err_function_স্তূপ_Underrun; // Return error for invalid slot indices
        } অন্যথা যদি (slot >= o->slot_First_Dynamic_সূচক) {
            DeallocateSlot(o, slot, type);
        }
    } অন্যথা যদি (not Is_স্তূপ_Polymorphic(o)) {
        ফলাফল = m3_Err_function_স্তূপ_Underrun;
    }

    ফেরত ফলাফল;
}

স্থির
M3_ফলাফল বের_করো_Type (IM3Compilation o, m3type_t i_type)
{
    M3_ফলাফল ফলাফল = m3Err_none;

    m3type_t শীর্ষ_Type = Get_স্তূপ_শীর্ষ_Type(o);

    যদি (M3_LIKELY(IsSubTypeOf(শীর্ষ_Type, i_type) or o->block.isPolymorphic)) {
_       (বের_করো(o));
    } অন্যথা {
        _throw(m3Err_typeMismatch);
    }

    _catch:
    ফেরত ফলাফল;
}

স্থির
M3_ফলাফল ঠেলো_Allocated_Slot_And_Emit (IM3Compilation o, m3type_t i_type, bool i_doEmit)
{
    M3_ফলাফল ফলাফল = m3Err_none;

    u16 slot = c_slotUnused;

_   (AllocateSlots(o, &slot, i_type));
_   (ঠেলো(o, i_type, slot));

    যদি (i_doEmit) {
        EmitSlotOffset(o, slot);
    }

    //    printf ("push: %d\n", (u32) slot);

    _catch: ফেরত ফলাফল;
}

স্থির অন্তঃরেখ
M3_ফলাফল ঠেলো_Allocated_Slot_And_Emit_2 (IM3Compilation o, m3type_t i_type)
{
    ফেরত ঠেলো_Allocated_Slot_And_Emit(o, i_type, true);
}

স্থির অন্তঃরেখ
M3_ফলাফল ঠেলো_Allocated_Slot (IM3Compilation o, m3type_t i_type)
{
    ফেরত ঠেলো_Allocated_Slot_And_Emit(o, i_type, false);
}

স্থির
M3_ফলাফল ঠেলো_Const (IM3Compilation o, u64 i_word, m3type_t i_type)
{
    M3_ফলাফল ফলাফল = m3Err_none;

    // When compile-walking a constant expression without emitting there is no
    // constant table to place the value in, but the type still has to land on
    // the stack: extended-const arithmetic downstream consumes it.
    যদি (M3_UNLIKELY(!o->page)) {
        ফেরত ঠেলো_Allocated_Slot(o, i_type);
    }

    bool matchFound  = false;
    bool is64BitType = Is64BitType(i_type);

    u16 অঙ্ক_Required_Slots  = Get_Type_অঙ্ক_Slots(i_type);
    u16 অঙ্ক_Used_Const_Slots = o->slot_Max_Const_সূচক - o->slot_First_Const_সূচক;

    // search for duplicate matching constant slot to reuse
    যদি (অঙ্ক_Required_Slots == 2 and অঙ্ক_Used_Const_Slots >= 2) {
        u16 firstConstSlot = o->slot_First_Const_সূচক;
        AlignSlotToType(&firstConstSlot, c_m3Type_i64);

        জন্য (u16 slot = firstConstSlot; slot < o->slot_Max_Const_সূচক - 1; slot += 2) {
            যদি (IsSlotAllocated(o, slot) and IsSlotAllocated(o, slot + 1)) {
                u64 constant;
                memcpy(&constant, &o->constants[slot - o->slot_First_Const_সূচক], আকার(constant));

                যদি (constant == i_word) {
                    matchFound = true;
_                   (ঠেলো(o, i_type, slot));
                    বিরতি;
                }
            }
        }
    } অন্যথা যদি (অঙ্ক_Required_Slots == 1) {
        জন্য (u16 i = 0; i < অঙ্ক_Used_Const_Slots; ++i) {
            u16 slot = o->slot_First_Const_সূচক + i;

            যদি (IsSlotAllocated(o, slot)) {
                bool matches;
                যদি (is64BitType) {
                    u64 constant;
                    memcpy(&constant, &o->constants[i], আকার(constant));
                    matches = (constant == i_word);
                } অন্যথা {
                    u32 constant;
                    memcpy(&constant, &o->constants[i], আকার(constant));
                    matches = (constant == i_word);
                }
                যদি (matches) {
                    matchFound = true;
_                   (ঠেলো(o, i_type, slot));
                    বিরতি;
                }
            }
        }
    }

    যদি (not matchFound) {
        u16 slot = c_slotUnused;
        ফলাফল   = AllocateConstantSlots(o, &slot, i_type);

        যদি (ফলাফল || slot == c_slotUnused) // no more constant table space; use inline constants
        {
            ফলাফল = m3Err_none;

            যদি (is64BitType) {
_               (EmitOp(o, op_Const64));
                EmitWord64(o->page, i_word);
            } অন্যথা {
_               (EmitOp(o, op_Const32));
                EmitWord32(o->page, (u32)i_word);
            }

_           (ঠেলো_Allocated_Slot_And_Emit_2(o, i_type));
        } অন্যথা {
            u16 const_Table_সূচক = slot - o->slot_First_Const_সূচক;

            d_m3Assert(const_Table_সূচক < d_m3_Max_Constant_Table_আকার);

            যদি (is64BitType) {
                memcpy(&o->constants[const_Table_সূচক], &i_word, আকার(i_word));
            } অন্যথা {
                u32 word32 = (u32)i_word;
                memcpy(&o->constants[const_Table_সূচক], &word32, আকার(word32));
            }

_           (ঠেলো(o, i_type, slot));

            o->slot_Max_Const_সূচক = M3_MAX(slot + অঙ্ক_Required_Slots, o->slot_Max_Const_সূচক);
        }
    }

    _catch: ফেরত ফলাফল;
}

স্থির অন্তঃরেখ
M3_ফলাফল Emit_Slot_অঙ্ক_Of_স্তূপ_শীর্ষ_And_বের_করো (IM3Compilation o)
{
    // no emit if value is in register
    যদি (Is_স্তূপ_শীর্ষ_In_Slot(o)) {
        EmitSlotOffset(o, Get_স্তূপ_শীর্ষ_Slot_সংখ্যা(o));
    }

    ফেরত বের_করো(o);
}


// Or, maybe: EmitTrappingOp
M3_ফলাফল AddTrapRecord (IM3Compilation o)
{
    M3_ফলাফল ফলাফল = m3Err_none;

    যদি (o->function) {
    }

    ফেরত ফলাফল;
}

স্থির
M3_ফলাফল Unwind_Block_স্তূপ (IM3Compilation o)
{
    M3_ফলাফল ফলাফল = m3Err_none;

    u32 বের_করো_গুনতি = 0;
    যখন (o->স্তূপ_সূচক > o->block.block_স্তূপ_সূচক) {
_       (বের_করো(o));
        ++বের_করো_গুনতি;
    }

    যদি (বের_করো_গুনতি) {
        m3log(compile, "unwound stack top: %d", বের_করো_গুনতি);
    }

    _catch: ফেরত ফলাফল;
}

স্থির অন্তঃরেখ
M3_ফলাফল Set_স্তূপ_Polymorphic (IM3Compilation o)
{
    o->block.isPolymorphic = true;                              m3log (compile, "stack set polymorphic");
    ফেরত Unwind_Block_স্তূপ(o);
}

স্থির
শূন্য PatchBranches (IM3Compilation o)
{
    InvalidateFold(o);         // patched branches land at the current pc

    pc_t pc = GetPC(o);

    pc_t patches     = o->block.patches;
    o->block.patches = ফাঁকা;

    যখন (patches) {                                           m3log (compile, "patching location: %p to pc: %p", patches, pc);
        pc_t পরবর্তী       = *(pc_t*)patches;
        *(pc_t*)patches = pc;
        patches         = পরবর্তী;
    }
}

//-------------------------------------------------------------------------------------------------------------------------

স্থির
M3_ফলাফল Copy_স্তূপ_সূচক_To_Slot (IM3Compilation o, u16 i_destSlot, u16 i_স্তূপ_সূচক)  // NoPushPop
{
    M3_ফলাফল ফলাফল = m3Err_none;

    IM3Operation op;

    m3type_t type       = Get_স্তূপ_Type_From_Bottom(o, i_স্তূপ_সূচক);
    bool     inRegister = Is_স্তূপ_সূচক_In_Register(o, i_স্তূপ_সূচক);

    যদি (inRegister) {
        op = c_setSetOps[BaseTypeOf(type)];
    } অন্যথা {
        op = Is64BitType(type) ? op_CopySlot_64 : op_CopySlot_32;
    }

_   (EmitOp(o, op));
    EmitSlotOffset(o, i_destSlot);

    যদি (not inRegister) {
        u16 srcSlot = Get_Slot_For_স্তূপ_সূচক(o, i_স্তূপ_সূচক);
        EmitSlotOffset(o, srcSlot);
    }

    _catch: ফেরত ফলাফল;
}

স্থির
M3_ফলাফল Copy_স্তূপ_শীর্ষ_To_Slot (IM3Compilation o, u16 i_destSlot)  // NoPushPop
{
    M3_ফলাফল ফলাফল;

    i16 স্তূপ_শীর্ষ;
_   (Get_স্তূপ_শীর্ষ_সূচক_Throws(o, &স্তূপ_শীর্ষ));
_   (Copy_স্তূপ_সূচক_To_Slot(o, i_destSlot, (u16)স্তূপ_শীর্ষ));

    _catch: ফেরত ফলাফল;
}


// a copy-on-write strategy is used with locals. when a get local occurs, it's not copied anywhere. the stack
// entry just has a index pointer to that local memory slot.
// then, when a previously referenced local is set, the current value needs to be preserved for those references

// TODO: consider getting rid of these specialized operations: PreserveSetSlot & PreserveCopySlot.
// They likely just take up space (which seems to reduce performance) without improving performance.
// => unconfirmed in preliminary testing.
স্থির
M3_ফলাফল Preserved_Copy_শীর্ষ_Slot (IM3Compilation o, u16 i_destSlot, u16 i_preserveSlot)
{
    M3_ফলাফল ফলাফল = m3Err_none;             d_m3Assert (i_destSlot != i_preserveSlot);

    IM3Operation op;

    m3type_t type = Get_স্তূপ_শীর্ষ_Type(o);

    যদি (Is_স্তূপ_শীর্ষ_In_Register(o)) {
        op = c_preserveSetSlot[BaseTypeOf(type)];
    } অন্যথা {
        op = Is64BitType(type) ? op_PreserveCopySlot_64 : op_PreserveCopySlot_32;
    }

_   (EmitOp(o, op));
    EmitSlotOffset(o, i_destSlot);

    যদি (Is_স্তূপ_শীর্ষ_In_Slot(o)) {
        EmitSlotOffset(o, Get_স্তূপ_শীর্ষ_Slot_সংখ্যা(o));
    }

    EmitSlotOffset(o, i_preserveSlot);

    _catch: ফেরত ফলাফল;
}

স্থির
M3_ফলাফল Copy_স্তূপ_শীর্ষ_To_Register (IM3Compilation o, bool i_update_স্তূপ)
{
    M3_ফলাফল ফলাফল = m3Err_none;

    যদি (Is_স্তূপ_শীর্ষ_In_Slot(o)) {
        m3type_t type = Get_স্তূপ_শীর্ষ_Type(o);

_       (PreserveRegisterIfOccupied(o, type));

        IM3Operation op = c_setRegisterOps[BaseTypeOf(type)];

_       (EmitOp(o, op));
        EmitSlotOffset(o, Get_স্তূপ_শীর্ষ_Slot_সংখ্যা(o));

        যদি (i_update_স্তূপ) {
_           (বের_করো_Type(o, type));
_           (ঠেলো_Register(o, type));
        }
    }

    _catch: ফেরত ফলাফল;
}


// if local is unreferenced, o_preservedSlotNumber will be equal to localIndex on return
স্থির
M3_ফলাফল FindReferencedLocalWithinCurrentBlock (IM3Compilation o, u16* o_preserved_Slot_সংখ্যা, u32 i_localSlot)
{
    M3_ফলাফল ফলাফল = m3Err_none;

    IM3CompilationScope scope = &o->block;

    u16 start_সূচক = scope->block_স্তূপ_সূচক;

    যখন (scope->opcode == c_waOp_block) {
        scope = scope->outer;
        যদি (not scope) {
            বিরতি;
        }

        start_সূচক = scope->block_স্তূপ_সূচক;
    }

    *o_preserved_Slot_সংখ্যা = (u16)i_localSlot;

    জন্য (u32 i = start_সূচক; i < o->স্তূপ_সূচক; ++i) {
        যদি (o->wasm_স্তূপ[i] == i_localSlot) {
            m3type_t type = Get_স্তূপ_Type_From_Bottom(o, i);                         d_m3Assert (type != c_m3Type_none)

            যদি (*o_preserved_Slot_সংখ্যা == i_localSlot) {
_               (AllocateSlots(o, o_preserved_Slot_সংখ্যা, type));
            } অন্যথা {
                // A 64-bit value spans two slots, and allocating or releasing it
                // counts both, so a second reference has to count both as well.
                জন্য (u16 s = 0; s < Get_Type_অঙ্ক_Slots(type); ++s) {
_                   (Increment_Slot_Usage_গুনতি(o, (u16)(*o_preserved_Slot_সংখ্যা + s)));
                }
            }

            o->wasm_স্তূপ[i] = *o_preserved_Slot_সংখ্যা;
        }
    }

    _catch: ফেরত ফলাফল;
}

স্থির
M3_ফলাফল GetBlockScope (IM3Compilation o, IM3CompilationScope* o_scope, u32 i_depth)
{
    M3_ফলাফল ফলাফল = m3Err_none;

    IM3CompilationScope scope = &o->block;

    যখন (i_depth--) {
        scope = scope->outer;
        _throwif("invalid block depth", not scope);
    }

    *o_scope = scope;

    _catch:
    ফেরত ফলাফল;
}

স্থির
M3_ফলাফল Copy_স্তূপ_Slots_R (IM3Compilation o, u16 i_target_Slot_স্তূপ_সূচক, u16 i_স্তূপ_সূচক, u16 i_end_স্তূপ_সূচক, u16 i_অস্থায়ী_Slot)
{
    M3_ফলাফল ফলাফল = m3Err_none;

    যদি (i_স্তূপ_সূচক < i_end_স্তূপ_সূচক) {
        u16 srcSlot = Get_Slot_For_স্তূপ_সূচক(o, i_স্তূপ_সূচক);

        m3type_t type      = Get_স্তূপ_Type_From_Bottom(o, i_স্তূপ_সূচক);
        u16      অঙ্ক_Slots  = Get_Type_অঙ্ক_Slots(type);
        u16      extraSlot = অঙ্ক_Slots - 1;

        u16 targetSlot = Get_Slot_For_স্তূপ_সূচক(o, i_target_Slot_স্তূপ_সূচক);

        u16 preserve_সূচক = i_স্তূপ_সূচক;
        u16 collisionSlot = srcSlot;

        যদি (targetSlot != srcSlot) {
            // search for collisions
            u16 check_সূচক = i_স্তূপ_সূচক + 1;
            যখন (check_সূচক < i_end_স্তূপ_সূচক) {
                u16 otherSlot1 = Get_Slot_For_স্তূপ_সূচক(o, check_সূচক);
                u16 otherSlot2 = Get_Extra_Slot_For_স্তূপ_সূচক(o, check_সূচক);

                যদি (targetSlot == otherSlot1 or
                    targetSlot == otherSlot2 or
                    targetSlot + extraSlot == otherSlot1) {
                    _throwif(m3_Err_function_স্তূপ_Overflow, i_অস্থায়ী_Slot >= d_m3MaxFunctionSlots);

_                   (Copy_স্তূপ_সূচক_To_Slot(o, i_অস্থায়ী_Slot, check_সূচক));
                    o->wasm_স্তূপ[check_সূচক] = i_অস্থায়ী_Slot;
                    i_অস্থায়ী_Slot += Get_Type_অঙ্ক_Slots(c_m3Type_i64);
                    TouchSlot(o, i_অস্থায়ী_Slot - 1);

                    // restore this on the way back down
                    preserve_সূচক = check_সূচক;
                    collisionSlot = otherSlot1;

                    বিরতি;
                }

                ++check_সূচক;
            }

_           (Copy_স্তূপ_সূচক_To_Slot(o, targetSlot, i_স্তূপ_সূচক));                                                m3log (compile, " copying slot: %d to slot: %d", srcSlot, targetSlot);
            o->wasm_স্তূপ[i_স্তূপ_সূচক] = targetSlot;
        }

_       (Copy_স্তূপ_Slots_R(o, i_target_Slot_স্তূপ_সূচক + 1, i_স্তূপ_সূচক + 1, i_end_স্তূপ_সূচক, i_অস্থায়ী_Slot));

        // restore the stack state
        o->wasm_স্তূপ[i_স্তূপ_সূচক]  = srcSlot;
        o->wasm_স্তূপ[preserve_সূচক] = collisionSlot;
    }

    _catch:
    ফেরত ফলাফল;
}

স্থির
M3_ফলাফল ResolveBlockResults (IM3Compilation o, IM3CompilationScope i_targetBlock, bool i_isBranch)
{
    M3_ফলাফল ফলাফল = m3Err_none;
    যদি (d_m3_Log_Wasm_স্তূপ) {
        dump_type_স্তূপ(o);
    }

    bool isLoop = (i_targetBlock->opcode == c_waOp_loop and i_isBranch);

    u16 অঙ্ক_Params  = Get_Func_Type_অঙ্ক_Params(i_targetBlock->type);
    u16 অঙ্ক_Results = Get_Func_Type_অঙ্ক_Results(i_targetBlock->type);

    u16 slotRecords = i_targetBlock->exit_স্তূপ_সূচক;

    u16 অঙ্ক_মানসমূহ;

    যদি (not isLoop) {
        অঙ্ক_মানসমূহ = অঙ্ক_Results;
        slotRecords += অঙ্ক_Params;
    } অন্যথা {
        অঙ্ক_মানসমূহ = অঙ্ক_Params;
    }

    u16 blockHeight = Get_অঙ্ক_Block_মানসমূহ_On_স্তূপ(o);

    _throwif(m3_Err_type_গুনতি_Mismatch, i_isBranch ? (blockHeight < অঙ্ক_মানসমূহ) : (blockHeight != অঙ্ক_মানসমূহ));

    যদি (অঙ্ক_মানসমূহ) {
        u16 end_সূচক     = Get_স্তূপ_শীর্ষ_সূচক(o) + 1;
        u16 অঙ্ক_Rem_মানসমূহ = অঙ্ক_মানসমূহ;

        // The last result is taken from _fp0. See PushBlockResults.
        যদি (not isLoop and IsFpType(Get_স্তূপ_শীর্ষ_Type(o))) {
_           (Copy_স্তূপ_শীর্ষ_To_Register(o, false));
            --end_সূচক;
            --অঙ্ক_Rem_মানসমূহ;
        }

        // tempslot affects maxStackSlots, so can grow unnecess each time.
        // in practice, this is hardly ever hit.
        u16 অস্থায়ী_Slot = o->max_স্তূপ_Slots;// GetMaxUsedSlotPlusOne (o); doesn't work cause can collide with slotRecords
        AlignSlotToType(&অস্থায়ী_Slot, c_m3Type_i64);

_       (Copy_স্তূপ_Slots_R(o, slotRecords, end_সূচক - অঙ্ক_Rem_মানসমূহ, end_সূচক, অস্থায়ী_Slot));

        যদি (d_m3_Log_Wasm_স্তূপ) {
            dump_type_স্তূপ(o);
        }
    }

    _catch: ফেরত ফলাফল;
}


স্থির
M3_ফলাফল Return_মানসমূহ (IM3Compilation o, IM3CompilationScope i_functionBlock, bool i_isBranch)
{
    M3_ফলাফল ফলাফল = m3Err_none;
    যদি (d_m3_Log_Wasm_স্তূপ) {
        dump_type_স্তূপ(o);
    }

    u16 অঙ্ক_Returns  = Get_Func_Type_অঙ্ক_Results(i_functionBlock->type);     // could just o->function too...
    u16 blockHeight = Get_অঙ্ক_Block_মানসমূহ_On_স্তূপ(o);

    যদি (not Is_স্তূপ_Polymorphic(o)) {
        _throwif(m3_Err_type_গুনতি_Mismatch, i_isBranch ? (blockHeight < অঙ্ক_Returns) : (blockHeight != অঙ্ক_Returns));
    }

    যদি (অঙ্ক_Returns) {
        // return slots like args are 64-bit aligned
        u16 returnSlot = অঙ্ক_Returns * c_io_Slot_গুনতি;
        u16 স্তূপ_শীর্ষ   = Get_স্তূপ_শীর্ষ_সূচক(o);

        জন্য (u16 i = 0; i < অঙ্ক_Returns; ++i) {
            m3type_t returnType = Get_Func_Type_ফলাফল_Type(i_functionBlock->type, অঙ্ক_Returns - 1 - i);

            m3type_t স্তূপ_Type = Get_স্তূপ_Type_From_শীর্ষ(o, i);  // using FromTop so that only dynamic items are checked

            যদি (Is_স্তূপ_Polymorphic(o) and স্তূপ_Type == c_m3Type_none) {
                স্তূপ_Type = returnType;
            }

            _throwif(m3Err_typeMismatch, not IsSubTypeOf(স্তূপ_Type, returnType));

            যদি (not Is_স্তূপ_Polymorphic(o)) {
                returnSlot -= c_io_Slot_গুনতি;
_               (Copy_স্তূপ_সূচক_To_Slot(o, returnSlot, স্তূপ_শীর্ষ--));
            }
        }

        যদি (not i_isBranch) {
            যখন (অঙ্ক_Returns--) {
_               (বের_করো(o));
            }
        }
    }

    _catch: ফেরত ফলাফল;
}


//-------------------------------------------------------------------------------------------------------------------------

স্থির
M3_ফলাফল Compile_Const_i32 (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_ফলাফল ফলাফল;

    i32 মান;
_   (ReadLEB_i32(&মান, &o->wasm, o->wasmEnd));
_   (ঠেলো_Const(o, মান, c_m3Type_i32));                        m3log (compile, d_indent " (const i32 = %" PRIi32 ")", get_indention_string (o), মান);
    _catch: ফেরত ফলাফল;
}

স্থির
M3_ফলাফল Compile_Const_i64 (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_ফলাফল ফলাফল;

    i64 মান;
_   (ReadLEB_i64(&মান, &o->wasm, o->wasmEnd));
_   (ঠেলো_Const(o, মান, c_m3Type_i64));                        m3log (compile, d_indent " (const i64 = %" PRIi64 ")", get_indention_string (o), মান);
    _catch: ফেরত ফলাফল;
}


#যদি d_m3ImplementFloat
স্থির
M3_ফলাফল Compile_Const_f32 (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_ফলাফল ফলাফল;

    সংঘ {
        u32 u;
        f32 f;
    } মান = { 0 };

_   (Read_f32(&মান.f, &o->wasm, o->wasmEnd));                 m3log (compile, d_indent " (const f32 = %" PRIf32 ")", get_indention_string (o), মান.f);
_   (ঠেলো_Const(o, মান.u, c_m3Type_f32));

    _catch: ফেরত ফলাফল;
}

স্থির
M3_ফলাফল Compile_Const_f64 (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_ফলাফল ফলাফল;

    সংঘ {
        u64 u;
        f64 f;
    } মান = { 0 };

_   (Read_f64(&মান.f, &o->wasm, o->wasmEnd));                 m3log (compile, d_indent " (const f64 = %" PRIf64 ")", get_indention_string (o), মান.f);
_   (ঠেলো_Const(o, মান.u, c_m3Type_f64));

    _catch: ফেরত ফলাফল;
}
#endif

স্থির
M3_ফলাফল Compile_ExtendedOpcode (IM3Compilation o, m3opcode_t i_opcode)
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

    } _catch: ফেরত ফলাফল;
}

স্থির
M3_ফলাফল Compile_Return (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_ফলাফল ফলাফল = m3Err_none;

    যদি (not Is_স্তূপ_Polymorphic(o)) {
        IM3CompilationScope functionScope;
_       (GetBlockScope(o, &functionScope, o->block.depth));

_       (Return_মানসমূহ(o, functionScope, true));

_       (EmitOp(o, op_Return));

_       (Set_স্তূপ_Polymorphic(o));
    }

    _catch: ফেরত ফলাফল;
}

স্থির
M3_ফলাফল ValidateBlockEnd (IM3Compilation o)
{
    M3_ফলাফল ফলাফল = m3Err_none;

    u16 অঙ্ক_Results  = Get_Func_Type_অঙ্ক_Results(o->block.type);
    u16 blockHeight = Get_অঙ্ক_Block_মানসমূহ_On_স্তূপ(o);

    যদি (not Is_স্তূপ_Polymorphic(o)) {
        // Spec: at block end, stack height must match the number of results
        _throwif(m3_Err_type_গুনতি_Mismatch, blockHeight != অঙ্ক_Results);

        // Spec: result types must match expected types
        জন্য (u16 i = 0; i < অঙ্ক_Results; ++i) {
            m3type_t expectedType = Get_Func_Type_ফলাফল_Type(o->block.type, অঙ্ক_Results - 1 - i);
            m3type_t actualType   = Get_স্তূপ_Type_From_শীর্ষ(o, i);
            _throwif(m3Err_typeMismatch, not IsSubTypeOf(actualType, expectedType));
        }
    }

    _catch: ফেরত ফলাফল;
}

স্থির
M3_ফলাফল Compile_End (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_ফলাফল ফলাফল = m3Err_none;                   //dump_type_stack (o);

    // function end:
    যদি (o->block.depth == 0) {
        ValidateBlockEnd(o);

        //      if (not IsStackPolymorphic (o))
        {
            // A constant expression has no function frame, but its value still
            // has to be copied down to slot 0, where EvaluateExpression reads it
            // back. The non-emitting init-expr walk is skipped: it has no block
            // type to return against.
            যদি (o->function or o->page) {
_               (Return_মানসমূহ(o, &o->block, false));
            }

_           (EmitOp(o, op_Return));
        }
    }

    _catch: ফেরত ফলাফল;
}


স্থির
M3_ফলাফল Compile_SetLocal (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_ফলাফল ফলাফল;

    u32 local_সূচক;
_   (ReadLEB_u32(&local_সূচক, &o->wasm, o->wasmEnd));                           m3log (compile, d_indent " (index = %u)", get_indention_string (o), local_সূচক);

    যদি (local_সূচক < Get_Function_অঙ্ক_Args_And_Locals(o->function)) {
        // Spec: value type must match local type
        যদি (not Is_স্তূপ_Polymorphic(o)) {
            m3type_t localType    = Get_স্তূপ_Type_From_Bottom(o, local_সূচক);
            m3type_t স্তূপ_শীর্ষ_Type = Get_স্তূপ_শীর্ষ_Type(o);
            _throwif(m3Err_typeMismatch, স্তূপ_শীর্ষ_Type != c_m3Type_none and localType != c_m3Type_none and
                                           not IsSubTypeOf(স্তূপ_শীর্ষ_Type, localType));
#যদি d_m3HasSnapshots
            যদি (o->localInitDepth and o->localInitDepth[local_সূচক] == UINT32_MAX) {
                o->localInitDepth[local_সূচক] = (u32)o->block.depth;
            }
#endif
        }

        u16 localSlot = Get_Slot_For_স্তূপ_সূচক(o, local_সূচক);

        u16 preserveSlot;
_       (FindReferencedLocalWithinCurrentBlock(o, &preserveSlot, localSlot));  // preserve will be different than local, if referenced

        bool folded = false;

#যদি d_m3FoldSetLocal
        // destination folding: when the value was produced by the immediately preceding op (nothing
        // emitted since, result on top in _r0, no copy-on-write preservation needed), retro-patch the
        // producer to a variant that writes straight to the local's slot and skip the SetSlot op
        যদি (o->foldPatchPC and preserveSlot == localSlot and Is_স্তূপ_শীর্ষ_In_Register(o) and Get_স্তূপ_শীর্ষ_সূচক(o) == (i16)o->fold_স্তূপ_সূচক) {
            IM3Operation foldOp = GetFoldOp(o->foldOpcode, o->foldForm);

            যদি (foldOp) {
                *(IM3Operation*)o->foldPatchPC = foldOp;     // retro-patch the producer
                EmitSlotOffset(o, localSlot);                  // append its destination
                InvalidateFold(o);
                folded = true;
            }
        }
#endif

        যদি (folded) {
            m3type_t type = Get_স্তূপ_শীর্ষ_Type(o);
_           (বের_করো(o));                              // the value now lives in the local's slot

            // A folded op writes its result through to the register as well, so a tee
            // can keep it there. Pushing the slot instead would make the next use that
            // wants a register reload the slot this op just stored to.
            যদি (i_opcode == c_waOp_teeLocal) {
_               (ঠেলো_Register(o, type));
            }
        } অন্যথা {
            যদি (preserveSlot == localSlot) {
_               (Copy_স্তূপ_শীর্ষ_To_Slot(o, localSlot))
            } অন্যথা {
_               (Preserved_Copy_শীর্ষ_Slot(o, localSlot, preserveSlot))
            }

            যদি (i_opcode != c_waOp_teeLocal) {
_               (বের_করো(o));
            }
        }
    } অন্যথা {
        _throw("local index out of bounds");
    }

    _catch: ফেরত ফলাফল;
}

স্থির
M3_ফলাফল Compile_GetLocal (IM3Compilation o, m3opcode_t i_opcode)
{
_try {

    u32 local_সূচক;
_   (ReadLEB_u32(&local_সূচক, &o->wasm, o->wasmEnd));                           m3log (compile, d_indent " (index = %u)", get_indention_string (o), local_সূচক);

    যদি (local_সূচক >= Get_Function_অঙ্ক_Args_And_Locals(o->function)) {
        _throw("local index out of bounds");
    }

    m3type_t type = Get_স্তূপ_Type_From_Bottom(o, local_সূচক);
    u16      slot = Get_Slot_For_স্তূপ_সূচক(o, local_সূচক);

_   (ঠেলো(o, type, slot));

    } _catch: ফেরত ফলাফল;
}

স্থির
M3_ফলাফল Compile_GetGlobal (IM3Compilation o, M3Global* i_global)
{
    M3_ফলাফল ফলাফল;

    IM3Operation op = Is64BitType(i_global->type) ? op_GetGlobal_s64 : op_GetGlobal_s32;
_   (EmitOp(o, op));
    EmitPointer(o, &i_global->i64_মান);
_   (ঠেলো_Allocated_Slot_And_Emit_2(o, i_global->type));

    _catch: ফেরত ফলাফল;
}

স্থির
M3_ফলাফল Compile_SetGlobal (IM3Compilation o, M3Global* i_global)
{
    M3_ফলাফল ফলাফল = m3Err_none;

    যদি (i_global->isMutable) {
        // Spec: value type must match global type
        যদি (not Is_স্তূপ_Polymorphic(o)) {
            m3type_t স্তূপ_শীর্ষ_Type = Get_স্তূপ_শীর্ষ_Type(o);
            _throwif(m3Err_typeMismatch, স্তূপ_শীর্ষ_Type != c_m3Type_none and
                                           not IsSubTypeOf(স্তূপ_শীর্ষ_Type, i_global->type));
        }

        IM3Operation op;
        m3type_t     type = Get_স্তূপ_শীর্ষ_Type(o);

        যদি (Is_স্তূপ_শীর্ষ_In_Register(o)) {
            op = c_setGlobalOps[BaseTypeOf(type)];
        } অন্যথা {
            op = Is64BitType(type) ? op_SetGlobal_s64 : op_SetGlobal_s32;
        }

_       (EmitOp(o, op));
        EmitPointer(o, &i_global->i64_মান);

        যদি (Is_স্তূপ_শীর্ষ_In_Slot(o)) {
            EmitSlotOffset(o, Get_স্তূপ_শীর্ষ_Slot_সংখ্যা(o));
        }

_       (বের_করো(o));
    } অন্যথা {
        _throw(m3Err_settingImmutableGlobal);
    }

    _catch: ফেরত ফলাফল;
}

স্থির
M3_ফলাফল Compile_GetSetGlobal (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_ফলাফল ফলাফল = m3Err_none;

    u32 global_সূচক;
_   (ReadLEB_u32(&global_সূচক, &o->wasm, o->wasmEnd));

    যদি (global_সূচক < o->module->অঙ্ক_Globals) {
        যদি (o->module->globals) {
            M3Global* global = &o->module->globals[global_সূচক];

            // Spec: a constant expression may only read an imported immutable
            // global. The module's own globals are counted before their
            // initializer is walked, so a bare index check would let one
            // reference itself. These describe how the module declared the
            // global, so they are checked before following any link.
            _throwif(m3_Err_globa_সূচক_Out_Of_Bounds, o->is_Init_রাশিমালা and not global->imported);
            _throwif(m3Err_wasmMalformed, o->is_Init_রাশিমালা and global->isMutable);

            // an import linked to another module reads and writes that module's
            // cell, not the placeholder standing in for it here
            যদি (global->resolved) {
                global = global->resolved;
            }

_           ((i_opcode == c_waOp_getGlobal) ? Compile_GetGlobal(o, global) : Compile_SetGlobal(o, global));
        } অন্যথা {
            _throw(ErrorCompile(m3Err_globalMemoryNotAllocated, o, "module '%s' is missing global memory", o->module->name));
        }
    } অন্যথা {
        _throw(m3_Err_globa_সূচক_Out_Of_Bounds);
    }

    _catch: ফেরত ফলাফল;
}

স্থির
শূন্য EmitPatchingBranchPointer (IM3Compilation o, IM3CompilationScope i_scope)
{
    pc_t patch       = EmitPointer(o, i_scope->patches);                m3log (compile, "branch patch required at: %p", patch);
    i_scope->patches = patch;
}

স্থির
M3_ফলাফল EmitPatchingBranch (IM3Compilation o, IM3CompilationScope i_scope)
{
    M3_ফলাফল ফলাফল = m3Err_none;

_   (EmitOp(o, op_Branch));
    EmitPatchingBranchPointer(o, i_scope);

    _catch: ফেরত ফলাফল;
}

#যদি d_m3HasExceptionHandling

// How many try_table handler records an exit from i_from out to i_target takes
// down with it. Walking outward from the block the branch sits in, every
// try_table scope up to and including the target is left behind - the target
// too, because landing on a block's label means that block is finished.
//
// A loop label is the exception, but a loop is never a try_table, so the count
// is the same either way: the loop itself is not on the list.
স্থির
u32 গুনতি_Try_Frames_To_Exit (IM3CompilationScope i_from, IM3CompilationScope i_target)
{
    u32 গুনতি = 0;

    জন্য (IM3CompilationScope scope = i_from; scope; scope = scope->outer) {
        যদি (scope->opcode == c_waOp_tryTable) {
            ++গুনতি;
        }

        যদি (scope == i_target) {
            বিরতি;
        }
    }

    ফেরত গুনতি;
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
স্থির
u32 অঙ্ক_Try_Frames_To_বের_করো (IM3CompilationScope i_from, IM3CompilationScope i_target)
{
    যদি (i_target->depth == 0) {
        ফেরত 0;
    }

    // the walk only ever runs inward-to-outward; a target that is already
    // outside i_from has nothing between them
    যদি (not i_from or i_from->depth < i_target->depth) {
        ফেরত 0;
    }

    ফেরত গুনতি_Try_Frames_To_Exit(i_from, i_target);
}


স্থির
M3_ফলাফল Emit_বের_করো_Try_Frames (IM3Compilation o, u32 i_অঙ্ক_Frames)
{
    M3_ফলাফল ফলাফল = m3Err_none;

    যদি (i_অঙ্ক_Frames) {
_       (EmitOp(o, op_বের_করো_Handlers));
        EmitConstant32(o, i_অঙ্ক_Frames);
    }

    _catch: ফেরত ফলাফল;
}


// A return_call leaves the enclosing function before the callee runs, so no
// try_table in this function may catch what the callee throws. Neither shape of
// the instruction unwinds the native stack in time to arrange that on its own:
// op_ReturnCall tail-jumps with op_TryTable's frames still standing, and the
// fallback shape is an ordinary call sitting inside them. Either way the
// handler records have to come off first.
স্থির
M3_ফলাফল Emit_বের_করো_Try_Frames_For_Return_Call (IM3Compilation o)
{
    u32 গুনতি = 0;

    জন্য (IM3CompilationScope scope = &o->block; scope; scope = scope->outer) {
        যদি (scope->opcode == c_waOp_tryTable) {
            ++গুনতি;
        }
    }

    ফেরত Emit_বের_করো_Try_Frames(o, গুনতি);
}

#অন্যথা

স্থির
M3_ফলাফল Emit_বের_করো_Try_Frames (IM3Compilation o, u32 i_অঙ্ক_Frames)
{
    (শূন্য)o;
    (শূন্য)i_অঙ্ক_Frames;
    ফেরত m3Err_none;
}

#endif // d_m3HasExceptionHandling

#যদি d_m3FuseBranch
// when the pending fold candidate is a compare whose result sits on top in _r0, return the fused
// variant that absorbs an immediately following br_if (i_isIf false) or if (i_isIf true)
স্থির
IM3Operation GetBranchFusionOp (IM3Compilation o, bool i_isIf)
{
    যদি (o->foldPatchPC and o->foldForm < 4 and Is_স্তূপ_শীর্ষ_In_Register(o) and Get_স্তূপ_শীর্ষ_সূচক(o) == (i16)o->fold_স্তূপ_সূচক) {
        ধ্রুবক M3FusedCmpOps* ops = GetFusedCmpOps(o->foldOpcode);

        যদি (ops) {
            ফেরত i_isIf ? ops->ifOp[o->foldForm] : ops->branchIf[o->foldForm];
        }
    }

    ফেরত ফাঁকা;
}
#endif // d_m3FuseBranch

স্থির অন্তঃরেখ
IM3Operation Op_ContinueLoop (IM3Compilation o)
{
#যদি d_m3_Has_স্তূপ_Switching
    যদি (M3_UNLIKELY(o->module->runtime and o->module->runtime->isSuspendable)) {
        ফেরত op_ContinueLoop_Suspendable;
    }
#endif
    ফেরত op_ContinueLoop;
}

স্থির অন্তঃরেখ
IM3Operation Op_ContinueLoopIf (IM3Compilation o)
{
#যদি d_m3_Has_স্তূপ_Switching
    যদি (M3_UNLIKELY(o->module->runtime and o->module->runtime->isSuspendable)) {
        ফেরত op_ContinueLoopIf_Suspendable;
    }
#endif
    ফেরত op_ContinueLoopIf;
}


স্থির
M3_ফলাফল Compile_Branch (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_ফলাফল ফলাফল;
    u32      অঙ্ক_Try_Frames = 0;

    u32 depth;
_   (ReadLEB_u32(&depth, &o->wasm, o->wasmEnd));

    // Spec: br_if condition must be i32
    যদি (i_opcode == c_waOp_branchIf and not Is_স্তূপ_Polymorphic(o)) {
        m3type_t condType = Get_স্তূপ_শীর্ষ_Type(o);
        _throwif(m3Err_typeMismatch, condType != c_m3Type_none and condType != c_m3Type_i32);
    }

    IM3CompilationScope scope;
_   (GetBlockScope(o, &scope, depth));

#যদি d_m3HasExceptionHandling
    অঙ্ক_Try_Frames = অঙ্ক_Try_Frames_To_বের_করো(&o->block, scope);
#endif

    // branch target is a loop (continue)
    যদি (scope->opcode == c_waOp_loop) {
        যদি (i_opcode == c_waOp_branchIf) {
            // a handler pop belongs on the taken path only, so it forces the
            // jump-over form even where the target takes no parameters
            যদি (Get_Func_Type_অঙ্ক_Params(scope->type) or অঙ্ক_Try_Frames) {
                IM3Operation op = Is_স্তূপ_শীর্ষ_In_Register(o) ? op_BranchIfPrologue_r : op_BranchIfPrologue_s;

_               (EmitOp(o, op));
_               (Emit_Slot_অঙ্ক_Of_স্তূপ_শীর্ষ_And_বের_করো(o));

                pc_t* jumpTo = (pc_t*)ReservePointer(o);

_               (Emit_বের_করো_Try_Frames(o, অঙ্ক_Try_Frames));

                যদি (not Is_স্তূপ_Polymorphic(o)) {
_                   (ResolveBlockResults(o, scope, /* isBranch: */ true));
                }

_               (EmitOp(o, Op_ContinueLoop(o)));
                EmitPointer(o, scope->pc);
_               (RecordBackEdge(o, scope, false));

                *jumpTo = GetPC(o);
            } অন্যথা {
                // move the condition to a register
_               (Copy_স্তূপ_শীর্ষ_To_Register(o, false));
_               (বের_করো_Type(o, c_m3Type_i32));

_               (EmitOp(o, Op_ContinueLoopIf(o)));
                EmitPointer(o, scope->pc);
_               (RecordBackEdge(o, scope, true));
            }

            //          dump_type_stack(o);
        } অন্যথা // is c_waOp_branch
        {
_           (Emit_বের_করো_Try_Frames(o, অঙ্ক_Try_Frames));

            // the branch operands become the loop's parameters on the next iteration
            যদি (not Is_স্তূপ_Polymorphic(o)) {
_               (ResolveBlockResults(o, scope, /* isBranch: */ true));
            }

_           (EmitOp(o, Op_ContinueLoop(o)));
            EmitPointer(o, scope->pc);
_           (RecordBackEdge(o, scope, false));
            o->block.isPolymorphic = true;
        }
    } অন্যথা // forward branch
    {
        pc_t* jumpTo = ফাঁকা;

        bool isReturn         = (scope->depth == 0);
        bool targetHasResults = Get_Func_Type_অঙ্ক_Results(scope->type);

        যদি (i_opcode == c_waOp_branchIf) {
            যদি (targetHasResults or isReturn or অঙ্ক_Try_Frames) {
                IM3Operation op = Is_স্তূপ_শীর্ষ_In_Register(o) ? op_BranchIfPrologue_r : op_BranchIfPrologue_s;

_               (EmitOp(o, op));
_               (Emit_Slot_অঙ্ক_Of_স্তূপ_শীর্ষ_And_বের_করো(o)); // condition

                // this is continuation point, if the branch isn't taken
                jumpTo = (pc_t*)ReservePointer(o);
            } অন্যথা {
#যদি d_m3FuseBranch
                IM3Operation fuseOp = GetBranchFusionOp(o, false);

                যদি (fuseOp) {
                    // absorb the branch into the compare that produced the condition
                    *(IM3Operation*)o->foldPatchPC = fuseOp;
                    InvalidateFold(o);

_                   (বের_করো(o));                      // the condition is consumed inside the fused op

                    EmitPatchingBranchPointer(o, scope);
                    যাও _catch;
                }
#endif
                IM3Operation op = Is_স্তূপ_শীর্ষ_In_Register(o) ? op_BranchIf_r : op_BranchIf_s;

_               (EmitOp(o, op));
_               (Emit_Slot_অঙ্ক_Of_স্তূপ_শীর্ষ_And_বের_করো(o)); // condition

                EmitPatchingBranchPointer(o, scope);
                যাও _catch;
            }
        }

        যদি (not Is_স্তূপ_Polymorphic(o)) {
            যদি (isReturn) {
_               (Return_মানসমূহ(o, scope, true));
_               (EmitOp(o, op_Return));
            } অন্যথা {
_               (Emit_বের_করো_Try_Frames(o, অঙ্ক_Try_Frames));

_               (ResolveBlockResults(o, scope, true));
_               (EmitPatchingBranch(o, scope));
            }
        }

        যদি (jumpTo) {
            *jumpTo = GetPC(o);
        }

        যদি (i_opcode == c_waOp_branch) {
_           (Set_স্তূপ_Polymorphic(o));
        }
    }

    _catch: ফেরত ফলাফল;
}

স্থির
M3_ফলাফল Compile_BranchTable (IM3Compilation o, m3opcode_t i_opcode)
{
    // the page a continue-op page is standing in for; non-NULL only while that
    // page is installed in o->page, so the catch below knows to put it back
    IM3CodePage displacedPage = ফাঁকা;
    // indexed by label depth: the continuation stub already emitted for that target
    // within this table, or NULL if this table has not reached it yet
    pc_t* targetStubs = ফাঁকা;
_try {
    u32 target_গুনতি;
_   (ReadLEB_u32(&target_গুনতি, &o->wasm, o->wasmEnd));

    // Spec: validate that the branch index operand is i32
    যদি (not Is_স্তূপ_Polymorphic(o)) {
        m3type_t সূচক_Type = Get_স্তূপ_শীর্ষ_Type(o);
        _throwif(m3Err_typeMismatch, সূচক_Type != c_m3Type_none and সূচক_Type != c_m3Type_i32);
    }

_   (PreserveRegisterIfOccupied(o, c_m3Type_i64));         // move branch operand to a slot
    u16 slot = Get_স্তূপ_শীর্ষ_Slot_সংখ্যা(o);
_   (বের_করো(o));

    // OPTZ: according to spec: "forward branches that target a control instruction with a non-empty
    // result type consume matching operands first and push them back on the operand stack after unwinding"
    // So, this move-to-reg is only necessary if the target scopes have a type.

    u32 অঙ্ক_Code_সারিগুলি = target_গুনতি + 4; // 3 => IM3Operation + slot + target_count + default_target
_   (Ensure_Code_Page_অঙ্ক_সারিগুলি(o, অঙ্ক_Code_সারিগুলি));

_   (EmitOp(o, op_BranchTable));
    EmitSlotOffset(o, slot);
    EmitConstant32(o, target_গুনতি);

    IM3CodePage continueOpPage = ফাঁকা;

    // The label types are checked by ValidateFunction, which runs under the same
    // d_m3EnableValidation guard just before this compilation pass. Comparing each
    // target against the default here would be wrong anyway: the spec matches every
    // target against the operand stack, so in unreachable code the operands are the
    // bottom type and the targets may legitimately differ (unreached-valid.wast).

    ++target_গুনতি; // include default

    // Every entry of one br_table is resolved from the same compilation state - each
    // stub restores whatever it touched - so two entries naming the same label emit
    // byte-identical epilogues. Emit one and point both table entries at it. A label
    // deeper than the current block is rejected by GetBlockScope below, which is what
    // bounds this array.
    targetStubs = m3_AllocArray(pc_t, (size_t)o->block.depth + 1);
    _throwifnull(targetStubs);

    জন্য (u32 i = 0; i < target_গুনতি; ++i) {
        u32 target;
_       (ReadLEB_u32(&target, &o->wasm, o->wasmEnd));

        IM3CompilationScope scope;
_       (GetBlockScope(o, &scope, target));

        যদি (targetStubs[target]) {
            EmitPointer(o, targetStubs[target]);
            চালু;
        }

        u32 অঙ্ক_Try_Frames = 0;
#যদি d_m3HasExceptionHandling
        অঙ্ক_Try_Frames = অঙ্ক_Try_Frames_To_বের_করো(&o->block, scope);
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
        যদি (scope->opcode != c_waOp_loop and scope->depth > 0 and অঙ্ক_Try_Frames == 0 and
            (Is_স্তূপ_Polymorphic(o) or Get_Func_Type_অঙ্ক_Results(scope->type) == 0)) {
            EmitPatchingBranchPointer(o, scope);
            চালু;
        }

_       (AcquireCompilationCodePage(o, &continueOpPage));

        pc_t startPC  = GetPagePC(continueOpPage);
        displacedPage = o->page;
        o->page       = continueOpPage;

        যদি (scope->opcode == c_waOp_loop) {
_           (Emit_বের_করো_Try_Frames(o, অঙ্ক_Try_Frames));

            যদি (not Is_স্তূপ_Polymorphic(o)) {
_               (ResolveBlockResults(o, scope, true));
            }

_           (EmitOp(o, Op_ContinueLoop(o)));
            EmitPointer(o, scope->pc);
_           (RecordBackEdge(o, scope, false));
        } অন্যথা {
            যদি (not Is_স্তূপ_Polymorphic(o)) {
                যদি (scope->depth == 0) {
_                   (Return_মানসমূহ(o, scope, true));
_                   (EmitOp(o, op_Return));
                } অন্যথা {
_                   (Emit_বের_করো_Try_Frames(o, অঙ্ক_Try_Frames));

_                   (ResolveBlockResults(o, scope, true));

_                   (EmitPatchingBranch(o, scope));
                }
            }
        }

        IM3CodePage returnTo = displacedPage;
        displacedPage        = ফাঁকা;

        ReleaseCompilationCodePage(o);
        o->page = returnTo;

        EmitPointer(o, startPC);
        targetStubs[target] = startPC;
    }

_   (Set_স্তূপ_Polymorphic(o));
} _catch:

    // thrown out of the loop above with a continue-op page installed: release it
    // and restore the one it displaced, which the caller's catch then releases.
    // Otherwise the displaced page is on no list and simply leaks.
    যদি (displacedPage) {
        ReleaseCompilationCodePage(o);
        o->page = displacedPage;
    }

    m3_Free(targetStubs);

    ফেরত ফলাফল;
}

স্থির
M3_ফলাফল CompileCallArgsAndReturn (IM3Compilation o, u16* o_স্তূপ_Offset, IM3FuncType i_type, bool i_isIndirect)
{
_try {

    u16 শীর্ষ_Slot = GetMaxUsedSlotPlusOne(o);

    // force use of at least one stack slot; this is to help ensure
    // the m3 stack overflows (and traps) before the native stack can overflow.
    // e.g. see Wasm spec test 'runaway' in call.wast
    শীর্ষ_Slot = M3_MAX(1, শীর্ষ_Slot);

    // stack frame is 64-bit aligned
    AlignSlotToType(&শীর্ষ_Slot, c_m3Type_i64);

    *o_স্তূপ_Offset = শীর্ষ_Slot;

    // wait to pop this here so that topSlot search is correct
    যদি (i_isIndirect) {
_       (বের_করো(o));
    }

    u16 অঙ্ক_Args = Get_Func_Type_অঙ্ক_Params(i_type);
    u16 অঙ্ক_Rets = Get_Func_Type_অঙ্ক_Results(i_type);

    u16 arg_শীর্ষ = শীর্ষ_Slot + (অঙ্ক_Args + অঙ্ক_Rets) * c_io_Slot_গুনতি;

    TouchSlot(o, arg_শীর্ষ - 1);

    যখন (অঙ্ক_Args--) {
_       (Copy_স্তূপ_শীর্ষ_To_Slot(o, arg_শীর্ষ -= c_io_Slot_গুনতি));
_       (বের_করো(o));
    }

    u16 i = 0;
    যখন (অঙ্ক_Rets--) {
        m3type_t type = Get_Func_Type_ফলাফল_Type(i_type, i++);

_       (ঠেলো(o, type, শীর্ষ_Slot));
_       (MarkSlotsAllocatedByType(o, শীর্ষ_Slot, type));

        শীর্ষ_Slot += c_io_Slot_গুনতি;
    }

    } _catch: ফেরত ফলাফল;
}

// A tail call hands the current frame over to the callee instead of stacking a new one.
// The callee's results have to be the enclosing function's results, so both use the same
// return slots and only the arguments move: they're staged above the caller's stack here
// (the argument expressions can still be reading the args/locals they're about to land on)
// and slid down onto the frame base at runtime, by op_ReturnCall[Indirect].
স্থির
M3_ফলাফল Compile_লেজ_Call_Args (IM3Compilation o, u16* o_স্তূপ_Offset, u16* o_অঙ্ক_Arg_Slots, IM3FuncType i_type, bool i_isIndirect)
{
_try {
    IM3FuncType funcType = o->function->funcType;

    u16 অঙ্ক_Rets = Get_Func_Type_অঙ্ক_Results(i_type);
    _throwif(m3Err_typeMismatch, অঙ্ক_Rets != Get_Func_Type_অঙ্ক_Results(funcType));

    জন্য (u16 i = 0; i < অঙ্ক_Rets; ++i) {
        _throwif(m3Err_typeMismatch, Get_Func_Type_ফলাফল_Type(i_type, i) != Get_Func_Type_ফলাফল_Type(funcType, i));
    }

    u16 শীর্ষ_Slot = GetMaxUsedSlotPlusOne(o);

    শীর্ষ_Slot = M3_MAX(1, শীর্ষ_Slot);

    // stack frame is 64-bit aligned
    AlignSlotToType(&শীর্ষ_Slot, c_m3Type_i64);

    *o_স্তূপ_Offset = শীর্ষ_Slot;

    // wait to pop this here so that topSlot search is correct
    যদি (i_isIndirect) {
_       (বের_করো(o));
    }

    u16 অঙ্ক_Args     = Get_Func_Type_অঙ্ক_Params(i_type);
    u16 অঙ্ক_Arg_Slots = অঙ্ক_Args * c_io_Slot_গুনতি;

    *o_অঙ্ক_Arg_Slots = অঙ্ক_Arg_Slots;

    u16 arg_শীর্ষ = শীর্ষ_Slot + অঙ্ক_Arg_Slots;
    _throwif(m3_Err_function_স্তূপ_Overflow, arg_শীর্ষ > d_m3MaxFunctionSlots);

    // the staging area is written by the caller, so op_Entry has to account for it
    TouchSlot(o, arg_শীর্ষ - 1);

    যখন (অঙ্ক_Args--) {
_       (Copy_স্তূপ_শীর্ষ_To_Slot(o, arg_শীর্ষ -= c_io_Slot_গুনতি));
_       (বের_করো(o));
    }

    } _catch: ফেরত ফলাফল;
}


#যদি d_m3HasTypedRefs

// call_ref $t: the callee is the reference on top of the stack, so the operand
// order is the same as call_indirect's, minus the table.
স্থির
M3_ফলাফল Compile_CallRef (IM3Compilation o, m3opcode_t i_opcode)
{
_try {
    u32 type_সূচক;
_   (ReadLEB_u32(&type_সূচক, &o->wasm, o->wasmEnd));

    _throwif("function call type index out of range", type_সূচক >= o->module->অঙ্ক_Func_Types);

    IM3FuncType type = o->module->funcTypes[type_সূচক];

    যদি (not Is_স্তূপ_Polymorphic(o)) {
        // whatever shape the reference has, it must be a function of this type
        m3type_t refType = Get_স্তূপ_শীর্ষ_Type(o);
        _throwif(m3Err_typeMismatch, not IsSubTypeOf(refType, RefTypeOfFuncType(type, false)));
    }

    যদি (Is_স্তূপ_শীর্ষ_In_Register(o)) {
_       (PreserveRegisterIfOccupied(o, c_m3Type_funcref));
    }

    u16 functionSlot = Get_স্তূপ_শীর্ষ_Slot_সংখ্যা(o);

    bool isReturnCall = (i_opcode == c_waOp_returnCallRef);
    bool use_লেজ_Call  = isReturnCall and d_m3_Can_লেজ_Call;

    u16 exec_শীর্ষ, অঙ্ক_Arg_Slots = 0;

    যদি (use_লেজ_Call) {
_       (Compile_লেজ_Call_Args(o, &exec_শীর্ষ, &অঙ্ক_Arg_Slots, type, true));
    } অন্যথা {
_       (CompileCallArgsAndReturn(o, &exec_শীর্ষ, type, true));
    }

#  যদি d_m3HasExceptionHandling
    যদি (isReturnCall) {
_       (Emit_বের_করো_Try_Frames_For_Return_Call(o));
    }
#  endif

_   (EmitOp(o, use_লেজ_Call ? op_ReturnCallRef : op_CallRef));
    EmitSlotOffset(o, functionSlot);
    EmitPointer(o, type);
    EmitSlotOffset(o, exec_শীর্ষ);

    যদি (not use_লেজ_Call) {
_       (RecordCall(o, Get_Func_Type_অঙ্ক_Results(type), exec_শীর্ষ, isReturnCall));
    }

    যদি (use_লেজ_Call) {
        EmitSlotOffset(o, o->function->অঙ্ক_Ret_Slots);
        EmitConstant32(o, অঙ্ক_Arg_Slots);

_       (Set_স্তূপ_Polymorphic(o));
    } অন্যথা যদি (isReturnCall) {
_       (Compile_Return(o, i_opcode));
    }

} _catch:
    ফেরত ফলাফল;
}


// ref.as_non_null only sharpens the type; the value is unchanged, so the stack
// entry is re-pushed with the null stripped out of its type.
স্থির
M3_ফলাফল Compile_Ref_AsNonNull (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_ফলাফল ফলাফল = m3Err_none;

    যদি (not Is_স্তূপ_Polymorphic(o)) {
        m3type_t type = Get_স্তূপ_শীর্ষ_Type(o);
        _throwif(m3Err_typeMismatch, not IsRefType(type));

        // op_RefAsNonNull only knows how to read its operand out of a slot; a
        // value still sitting in the register has to be spilled first, the way
        // every other single-op reader of the stack top does
        যদি (Is_স্তূপ_শীর্ষ_In_Register(o)) {
_           (PreserveRegisterIfOccupied(o, type));
        }

        u16 slot = Get_স্তূপ_শীর্ষ_Slot_সংখ্যা(o);

_       (EmitOp(o, op_RefAsNonNull));
        EmitSlotOffset(o, slot);

        m3type_t nonNull = IsSpelledRefType(type)
                             ? (m3type_t)(type | d_m3Type_refNonNull)
                             : (m3type_t)(d_m3Type_ref | d_m3Type_refNonNull | d_m3Type_heapAbstract |
                                          ((BaseTypeOf(type) == c_m3Type_externref) ? d_m3Type_refExtern : 0));

        // the slot keeps the one reference it already had; a Pop()/Push() of
        // the same slot would drop that reference's count without anything
        // to put it back, so the type above it is corrected in place instead
        o->type_স্তূপ[Get_স্তূপ_শীর্ষ_সূচক(o)] = nonNull;
    }

    _catch: ফেরত ফলাফল;
}

#endif // d_m3HasTypedRefs


// How wide a table operand is read at comes from the table it names, so a slot
// holding a narrower type would be read past what was allocated for it. The
// validator refuses that already; this keeps a build without one from reaching
// the interpreter with it. c_m3Type_none means the operand is not on the
// dynamic stack to inspect, which is not this check's business.
স্থির
M3_ফলাফল CheckOperandType (IM3Compilation o, u16 i_depth_From_শীর্ষ, m3type_t i_type)
{
    m3type_t actual;

    যদি (Is_স্তূপ_Polymorphic(o)) {
        ফেরত m3Err_none;
    }

    actual = Get_স্তূপ_Type_From_শীর্ষ(o, i_depth_From_শীর্ষ);

    ফেরত (actual == c_m3Type_none or BaseTypeOf(actual) == i_type) ? m3Err_none
                                                                     : m3Err_typeMismatch;
}


// Swaps the _mem register onto a given memory. Only ever emitted in pairs --
// see op_SetMemory.
স্থির
M3_ফলাফল EmitSetMemoryPtr (IM3Compilation o, IM3Memory i_memory)
{
    M3_ফলাফল ফলাফল = m3Err_none;

_   (EmitOp(o, op_SetMemory));
    EmitPointer(o, i_memory);

    _catch: ফেরত ফলাফল;
}


// ...onto one of the compiling module's own memories, by index. Only reached
// with a non-zero index under multi-memory, but the index has already been
// bounds-checked by ReadMemoryIndex either way.
স্থির অন্তঃরেখ
M3_ফলাফল EmitSetMemory (IM3Compilation o, u32 i_memoryIdx)
{
#যদি d_m3HasMultiMemory
    ফেরত EmitSetMemoryPtr(o, o->module->memories[i_memoryIdx]);
#অন্যথা
    (শূন্য)o;
    (শূন্য)i_memoryIdx;
    ফেরত m3Err_unknownMemory;
#endif
}

স্থির
M3_ফলাফল Compile_Call (IM3Compilation o, m3opcode_t i_opcode)
{
_try {
    u32 function_সূচক;
_   (ReadLEB_u32(&function_সূচক, &o->wasm, o->wasmEnd));

    IM3Function function = Module_GetFunction(o->module, function_সূচক);

    যদি (function) {                                                     m3log (compile, d_indent " (func= [%d] '%s'; args= %d)",
                                                                                get_indention_string (o), function_সূচক, m3_GetFunctionName (function), function->funcType->অঙ্ক_Args);
        // an import linked to another module's export runs that module's body
        IM3Function target = Function_Implementation(function);

        যদি (target->module) {
            bool isReturnCall = (i_opcode == c_waOp_returnCall);

            // The callee's body runs against its own module's memory, so a call
            // that crosses modules is bracketed by a pair of op_SetMemory. A
            // tail call never returns to run the second one, so those compile
            // as a plain call followed by a return instead.
            bool crossModule = (target->module != o->module);
            bool use_লেজ_Call = isReturnCall and d_m3_Can_লেজ_Call and not crossModule;

            u16 slot_শীর্ষ, অঙ্ক_Arg_Slots = 0;

            যদি (use_লেজ_Call) {
_               (Compile_লেজ_Call_Args(o, &slot_শীর্ষ, &অঙ্ক_Arg_Slots, target->funcType, false));
            } অন্যথা {
_               (CompileCallArgsAndReturn(o, &slot_শীর্ষ, target->funcType, false));
            }

            IM3Operation op;
            ধ্রুবক শূন্য*  operand;

            যদি (target->compiled) {
                op      = use_লেজ_Call ? op_ReturnCall : op_Call;
                operand = target->compiled;
            } অন্যথা {
                op      = use_লেজ_Call ? op_CompileReturnCall : op_Compile;
                operand = target;
            }

#যদি d_m3HasExceptionHandling
            যদি (isReturnCall) {
_               (Emit_বের_করো_Try_Frames_For_Return_Call(o));
            }
#endif

            যদি (crossModule) {
_               (EmitSetMemoryPtr(o, Module_Memory0(target->module)));
            }

_           (EmitOp(o, op));
            EmitPointer(o, operand);
            EmitSlotOffset(o, slot_শীর্ষ);

            যদি (not use_লেজ_Call) {
                // a suspend inside the callee leaves this frame waiting here, its
                // results not yet written
_               (RecordCall(o, Get_Func_Type_অঙ্ক_Results(target->funcType), slot_শীর্ষ, isReturnCall));
            }

            যদি (use_লেজ_Call) {
                EmitSlotOffset(o, o->function->অঙ্ক_Ret_Slots);
                EmitConstant32(o, অঙ্ক_Arg_Slots);

_               (Set_স্তূপ_Polymorphic(o));
            } অন্যথা {
                যদি (crossModule) {
_                   (EmitSetMemoryPtr(o, Module_Memory0(o->module)));
                }

                যদি (isReturnCall) {
_                   (Compile_Return(o, i_opcode));
                }
            }
        } অন্যথা {
            _throw(ErrorCompile(m3Err_functionImportMissing, o, "'%s.%s'", GetFunctionImportModuleName(function), m3_GetFunctionName(function)));
        }
    } অন্যথা {
        _throw(ErrorCompile(m3Err_functionLookupFailed, o, "index: %d", function_সূচক));
    }

    } _catch: ফেরত ফলাফল;
}

স্থির
M3_ফলাফল Compile_CallIndirect (IM3Compilation o, m3opcode_t i_opcode)
{
_try {
    u32 type_সূচক;
_   (ReadLEB_u32(&type_সূচক, &o->wasm, o->wasmEnd));

    u32 table_সূচক;
_   (ReadLEB_u32(&table_সূচক, &o->wasm, o->wasmEnd));

    _throwif("function call type index out of range", type_সূচক >= o->module->অঙ্ক_Func_Types);
    _throwif("table index out of range", table_সূচক >= o->module->অঙ্ক_Tables);
    _throwif(m3Err_typeMismatch, BaseTypeOf(o->module->tables[table_সূচক]->type) != c_m3Type_funcref);

_   (CheckOperandType(o, 0, Table_AddrType(o->module->tables[table_সূচক])));

    যদি (Is_স্তূপ_শীর্ষ_In_Register(o)) {
_       (PreserveRegisterIfOccupied(o, c_m3Type_i32));
    }

    u16 table_সূচক_Slot = Get_স্তূপ_শীর্ষ_Slot_সংখ্যা(o);

    bool isReturnCall = (i_opcode == c_waOp_returnCallIndirect);
    bool use_লেজ_Call  = isReturnCall and d_m3_Can_লেজ_Call;

    u16         exec_শীর্ষ, অঙ্ক_Arg_Slots = 0;
    IM3FuncType type = o->module->funcTypes[type_সূচক];

    যদি (use_লেজ_Call) {
_       (Compile_লেজ_Call_Args(o, &exec_শীর্ষ, &অঙ্ক_Arg_Slots, type, true));
    } অন্যথা {
_       (CompileCallArgsAndReturn(o, &exec_শীর্ষ, type, true));
    }

#যদি d_m3HasExceptionHandling
    যদি (isReturnCall) {
_       (Emit_বের_করো_Try_Frames_For_Return_Call(o));
    }
#endif

    // the table comes first: how wide its indexes are is what says how much of
    // the slot below holds one
_   (EmitOp(o, use_লেজ_Call ? op_ReturnCallIndirect : op_CallIndirect));
    EmitPointer(o, o->module->tables[table_সূচক]);
    EmitSlotOffset(o, table_সূচক_Slot);
    EmitPointer(o, type);
    EmitSlotOffset(o, exec_শীর্ষ);

    যদি (not use_লেজ_Call) {
_       (RecordCall(o, Get_Func_Type_অঙ্ক_Results(type), exec_শীর্ষ, isReturnCall));
    }

    যদি (use_লেজ_Call) {
        EmitSlotOffset(o, o->function->অঙ্ক_Ret_Slots);
        EmitConstant32(o, অঙ্ক_Arg_Slots);

_       (Set_স্তূপ_Polymorphic(o));
    } অন্যথা যদি (isReturnCall) {
_       (Compile_Return(o, i_opcode));
    }

} _catch:
    ফেরত ফলাফল;
}

// Reads the memory index a memory instruction names, and checks it against the
// module's index space. Without multi-memory the immediate is a reserved byte
// that has to be zero, which is the same check.
স্থির
M3_ফলাফল Read_Memory_সূচক (IM3Compilation o, u32* o_memoryIdx)
{
    M3_ফলাফল ফলাফল;

_   (ReadLEB_u32(o_memoryIdx, &o->wasm, o->wasmEnd));

    _throwif(m3Err_unknownMemory, *o_memoryIdx >= o->module->অঙ্ক_Memories);

    _catch: ফেরত ফলাফল;
}

স্থির
M3_ফলাফল Compile_Memory_আকার (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_ফলাফল ফলাফল;

    u32 memoryIdx;

    // A page count is given in the memory's own address type. Declared up here
    // so the throws below don't jump over its initialization.
    m3type_t addrType = c_m3Type_i32;

_   (Read_Memory_সূচক(o, &memoryIdx));

    addrType = Memory_AddrType(o->module->memories[memoryIdx]);

_   (PreserveRegisterIfOccupied(o, addrType));

    যদি (memoryIdx) {
_       (EmitSetMemory(o, memoryIdx));
    }

    // op_MemSize writes the whole register either way; the slot it is pushed
    // to is what decides how much of it the module gets to see
_   (EmitOp(o, op_Mem_আকার));

    যদি (memoryIdx) {
_       (EmitSetMemory(o, 0));
    }

_   (ঠেলো_Register(o, addrType));

    _catch: ফেরত ফলাফল;
}

স্থির
M3_ফলাফল Compile_Memory_Grow (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_ফলাফল ফলাফল;

    u32 memoryIdx;

    // see Compile_Memory_Size
    m3type_t addrType = c_m3Type_i32;

_   (Read_Memory_সূচক(o, &memoryIdx));

    addrType = Memory_AddrType(o->module->memories[memoryIdx]);

_   (Copy_স্তূপ_শীর্ষ_To_Register(o, false));
_   (বের_করো_Type(o, addrType));

    যদি (memoryIdx) {
_       (EmitSetMemory(o, memoryIdx));
    }

#যদি d_m3HasMemory64
_   (EmitOp(o, (addrType == c_m3Type_i64) ? op_MemGrow64 : op_MemGrow));
#অন্যথা
_   (EmitOp(o, op_MemGrow));
#endif

    যদি (memoryIdx) {
_       (EmitSetMemory(o, 0));
    }

_   (ঠেলো_Register(o, addrType));

    _catch: ফেরত ফলাফল;
}

স্থির
M3_ফলাফল Compile_Memory_CopyFill (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_ফলাফল ফলাফল = m3Err_none;

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
    m3type_t দৈর্ঘ্য_Type = c_m3Type_i32;

    _throwif(m3Err_wasmMalformed, o->module->অঙ্ক_Memories == 0);

_   (Read_Memory_সূচক(o, &targetMemoryIdx));

    যদি (isCopy) {
_       (Read_Memory_সূচক(o, &sourceMemoryIdx));
    }

    targetType = Memory_AddrType(o->module->memories[targetMemoryIdx]);
    sourceType = isCopy ? Memory_AddrType(o->module->memories[sourceMemoryIdx])
                        : targetType;
    দৈর্ঘ্য_Type = (targetType == c_m3Type_i64 and sourceType == c_m3Type_i64)
                   ? c_m3Type_i64
                   : c_m3Type_i32;

_   (Copy_স্তূপ_শীর্ষ_To_Register(o, false));

#যদি d_m3HasMultiMemory
    যদি (isCopy and sourceMemoryIdx != targetMemoryIdx) {
        // two memories at once: _mem can only name one, so the op takes both
_       (EmitOp(o, op_MemCopy_x));
        EmitPointer(o, o->module->memories[targetMemoryIdx]);
        EmitPointer(o, o->module->memories[sourceMemoryIdx]);

        // ...and, since they need not be addressed alike, how wide to read
        // each of the operands: bit 0 the destination, bit 1 the source
        EmitConstant32(o, ((targetType == c_m3Type_i64) ? 0x1u : 0u) |
                            ((sourceType == c_m3Type_i64) ? 0x2u : 0u));
    } অন্যথা
#endif
    {
        IM3Operation op = isCopy ? op_MemCopy : op_MemFill;

#যদি d_m3HasMemory64
        যদি (targetType == c_m3Type_i64) {
            op = isCopy ? op_MemCopy64 : op_MemFill64;
        }
#endif

        যদি (targetMemoryIdx) {
_           (EmitSetMemory(o, targetMemoryIdx));
        }

_       (EmitOp(o, op));
    }

    // the length is in the register; the other two are named by slot, at
    // whatever width their stack entries were pushed with
_   (বের_করো_Type(o, দৈর্ঘ্য_Type));
_   (Emit_Slot_অঙ্ক_Of_স্তূপ_শীর্ষ_And_বের_করো(o));      // the source, or the byte to fill with
_   (Emit_Slot_অঙ্ক_Of_স্তূপ_শীর্ষ_And_বের_করো(o));      // the destination

    যদি (targetMemoryIdx and not (isCopy and sourceMemoryIdx != targetMemoryIdx)) {
_       (EmitSetMemory(o, 0));
    }

    _catch: ফেরত ফলাফল;
}


// memory.init and data.drop address a data segment by index. Both are only valid
// when a data count section declared the segments up front.
স্থির
M3_ফলাফল Read_তথ্য_Segment (IM3Compilation o, M3_তথ্য_Segment** o_segment)
{
    M3_ফলাফল ফলাফল = m3Err_none;

    u32 index;
_   (ReadLEB_u32(&index, &o->wasm, o->wasmEnd));

    _throwif("data count section required", not o->module->has_তথ্য_গুনতি);
    _throwif(m3Err_wasmMalformed, index >= o->module->অঙ্ক_তথ্য_Segments);

    *o_segment = &o->module->তথ্য_Segments[index];

    _catch: ফেরত ফলাফল;
}


M3_ফলাফল Compile_Memory_Init (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_ফলাফল ফলাফল = m3Err_none;

    M3_তথ্য_Segment* segment = ফাঁকা;
    u32            memoryIdx;

    _throwif(m3Err_wasmMalformed, o->module->অঙ্ক_Memories == 0);

_   (Read_তথ্য_Segment(o, &segment));
_   (Read_Memory_সূচক(o, &memoryIdx));

_   (Copy_স্তূপ_শীর্ষ_To_Register(o, false));

    যদি (memoryIdx) {
_       (EmitSetMemory(o, memoryIdx));
    }

#যদি d_m3HasMemory64
    // only the destination follows the memory's address type: a data segment
    // is indexed as an i32 however the memory it is copied into is addressed
_   (EmitOp(o, o->module->memories[memoryIdx]->isMemory64 ? op_MemInit64 : op_MemInit));
#অন্যথা
_   (EmitOp(o, op_MemInit));
#endif
    EmitPointer(o, segment);
_   (বের_করো_Type(o, c_m3Type_i32));
_   (Emit_Slot_অঙ্ক_Of_স্তূপ_শীর্ষ_And_বের_করো(o));
_   (Emit_Slot_অঙ্ক_Of_স্তূপ_শীর্ষ_And_বের_করো(o));

    যদি (memoryIdx) {
_       (EmitSetMemory(o, 0));
    }

    _catch: ফেরত ফলাফল;
}


#যদি d_m3HasRefTypes

স্থির
M3_ফলাফল Compile_Select (IM3Compilation o, m3opcode_t i_opcode);

// select with an explicit result type vector. The types only matter to the
// validator; the operands are laid out exactly as for the untyped select.
M3_ফলাফল Compile_Select_Typed (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_ফলাফল ফলাফল = m3Err_none;

    u32 অঙ্ক_Types;
_   (ReadLEB_u32(&অঙ্ক_Types, &o->wasm, o->wasmEnd));
    _throwif(m3Err_wasmMalformed, অঙ্ক_Types != 1);

    i8 waType;
    u8 type;
_   (ReadLEB_i7(&waType, &o->wasm, o->wasmEnd));
_   (NormalizeType(&type, waType));

_   (Compile_Select(o, i_opcode));

    _catch: ফেরত ফলাফল;
}


// A reference is one pointer-sized word and null is 0, so null-testing it is
// just an integer eqz of the matching width.
#  যদি M3_SIZEOF_PTR == 8
#    define d_m3RefIsNull_r   op_i64_EqualToZero_r
#    define d_m3RefIsNull_s   op_i64_EqualToZero_s
#  অন্যথা
#    define d_m3RefIsNull_r   op_i32_EqualToZero_r
#    define d_m3RefIsNull_s   op_i32_EqualToZero_s
#  endif

স্থির
M3_ফলাফল ReadRefType (IM3Compilation o, m3type_t* o_type)
{
    M3_ফলাফল ফলাফল = m3Err_none;

#  যদি d_m3HasTypedRefs
    // ref.null names a heap type: func, extern, or a function type index
    m3type_t heapBits;
_   (ParseHeapType(o->module, &heapBits, &o->wasm, o->wasmEnd));

    *o_type = d_m3Type_ref | heapBits;
#  অন্যথা
    i8 waType;
    u8 plainType;
_   (ReadLEB_i7(&waType, &o->wasm, o->wasmEnd));
_   (NormalizeType(&plainType, waType));
    *o_type = plainType;
    _throwif(m3Err_wasmMalformed, not IsRefType(*o_type));
#  endif

    _catch: ফেরত ফলাফল;
}


M3_ফলাফল Compile_Ref_Null (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_ফলাফল ফলাফল = m3Err_none;

    m3type_t type;
_   (ReadRefType(o, &type));
_   (ঠেলো_Const(o, 0, type));

    _catch: ফেরত ফলাফল;
}


M3_ফলাফল Compile_Ref_IsNull (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_ফলাফল ফলাফল = m3Err_none;

    IM3Operation op;

    যদি (not Is_স্তূপ_Polymorphic(o)) {
        _throwif(m3Err_typeMismatch, not IsRefType(Get_স্তূপ_শীর্ষ_Type(o)));
    }

    যদি (Is_স্তূপ_শীর্ষ_In_Register(o)) {
        op = d_m3RefIsNull_r;
    } অন্যথা {
_       (PreserveRegisterIfOccupied(o, c_m3Type_i32));
        op = d_m3RefIsNull_s;
    }

_   (EmitOp(o, op));
_   (Emit_Slot_অঙ্ক_Of_স্তূপ_শীর্ষ_And_বের_করো(o));
_   (ঠেলো_Register(o, c_m3Type_i32));

    _catch: ফেরত ফলাফল;
}


M3_ফলাফল Compile_Ref_Func (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_ফলাফল ফলাফল = m3Err_none;

    u32 func_সূচক;

    // declared before the throws below, which jump past this point to _catch
    m3type_t    refType   = c_m3Type_funcref;
    IM3Function reference = ফাঁকা;

_   (ReadLEB_u32(&func_সূচক, &o->wasm, o->wasmEnd));
    _throwif("function index out of range", func_সূচক >= o->module->অঙ্ক_Functions);

    // Inside a constant expression ref.func is itself a declaration; inside a
    // function body the function must already have been declared elsewhere.
    যদি (o->function) {
        _throwif("undeclared function reference", not Module_IsFunctionDeclared(o->module, func_সূচক));
    } অন্যথা {
_       (Module_DeclareFunction(o->module, func_সূচক));
    }

#  যদি d_m3HasTypedRefs
    refType = RefTypeOfFuncType(o->module->functions[func_সূচক].funcType, true);
#  endif

    // A reference denotes the function that actually runs, so a ref.func naming
    // an import linked to another module is the same reference that module's
    // own ref.func would produce - and carries the defining module with it.
    reference = Function_Implementation(&o->module->functions[func_সূচক]);

_   (ঠেলো_Const(o, (u64)(uintptr_t)reference, refType));

    _catch: ফেরত ফলাফল;
}


// table.get/set/size/grow/fill all name a table by index; the table struct goes
// into the codestream so the operation doesn't have to walk the module.
স্থির
M3_ফলাফল ReadTable (IM3Compilation o, M3Table** o_table)
{
    M3_ফলাফল ফলাফল = m3Err_none;

    u32 index;
_   (ReadLEB_u32(&index, &o->wasm, o->wasmEnd));
    _throwif("table index out of range", index >= o->module->অঙ্ক_Tables);

    *o_table = o->module->tables[index];

    _catch: ফেরত ফলাফল;
}


// The table operations take every operand from a slot, so spill the integer
// register first: after this each EmitSlotNumOfStackTopAndPop () emits a real
// slot offset, and the register is free for the result. Operands are emitted
// from the top of the stack downwards, which is the order the ops read them.
স্থির
M3_ফলাফল Compile_Table_Op (IM3Compilation o, IM3Operation i_op, M3Table* i_table, u32 i_অঙ্ক_Operands, m3type_t i_retType)
{
    M3_ফলাফল ফলাফল = m3Err_none;

_   (PreserveRegisterIfOccupied(o, c_m3Type_i64));

_   (EmitOp(o, i_op));
    EmitPointer(o, i_table);

    জন্য (u32 i = 0; i < i_অঙ্ক_Operands; ++i) {
_       (Emit_Slot_অঙ্ক_Of_স্তূপ_শীর্ষ_And_বের_করো(o));
    }

    যদি (i_retType != c_m3Type_none) {
_       (ঠেলো_Register(o, i_retType));
    }

    _catch: ফেরত ফলাফল;
}


M3_ফলাফল Compile_Table_GetSet (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_ফলাফল ফলাফল = m3Err_none;

    M3Table* table;
_   (ReadTable(o, &table));

    যদি (i_opcode == c_waOp_tableGet) {
_       (CheckOperandType(o, 0, Table_AddrType(table)));
_       (Compile_Table_Op(o, op_TableGet, table, 1, table->type));
    } অন্যথা {
_       (CheckOperandType(o, 1, Table_AddrType(table)));
_       (Compile_Table_Op(o, op_TableSet, table, 2, c_m3Type_none));
    }

    _catch: ফেরত ফলাফল;
}


M3_ফলাফল Compile_Table_Init (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_ফলাফল ফলাফল = m3Err_none;

    u32      elem_সূচক;
    M3Table* table;

_   (ReadLEB_u32(&elem_সূচক, &o->wasm, o->wasmEnd));
    _throwif("element segment index out of range", elem_সূচক >= o->module->অঙ্ক_Element_Segments);
_   (ReadTable(o, &table));

_   (CheckOperandType(o, 2, Table_AddrType(table)));

_   (PreserveRegisterIfOccupied(o, c_m3Type_i64));
_   (EmitOp(o, op_TableInit));
    EmitPointer(o, table);
    EmitPointer(o, &o->module->elementSegments[elem_সূচক]);

    জন্য (u32 i = 0; i < 3; ++i) {
_       (Emit_Slot_অঙ্ক_Of_স্তূপ_শীর্ষ_And_বের_করো(o));
    }

    _catch: ফেরত ফলাফল;
}


M3_ফলাফল Compile_Elem_Drop (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_ফলাফল ফলাফল = m3Err_none;

    u32 elem_সূচক;
_   (ReadLEB_u32(&elem_সূচক, &o->wasm, o->wasmEnd));
    _throwif("element segment index out of range", elem_সূচক >= o->module->অঙ্ক_Element_Segments);

_   (EmitOp(o, op_ElemDrop));
    EmitPointer(o, &o->module->elementSegments[elem_সূচক]);

    _catch: ফেরত ফলাফল;
}


M3_ফলাফল Compile_Table_Copy (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_ফলাফল ফলাফল = m3Err_none;

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

    জন্য (u32 i = 0; i < 3; ++i) {
_       (Emit_Slot_অঙ্ক_Of_স্তূপ_শীর্ষ_And_বের_করো(o));
    }

    _catch: ফেরত ফলাফল;
}


M3_ফলাফল Compile_Table_আকার (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_ফলাফল ফলাফল = m3Err_none;

    M3Table* table;
_   (ReadTable(o, &table));

    // a table size is given in the table's own index type
_   (Compile_Table_Op(o, op_Table_আকার, table, 0, Table_AddrType(table)));

    _catch: ফেরত ফলাফল;
}


M3_ফলাফল Compile_Table_GrowFill (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_ফলাফল ফলাফল = m3Err_none;

    M3Table* table;
_   (ReadTable(o, &table));

    যদি (i_opcode == c_waOp_tableGrow) {
_       (CheckOperandType(o, 0, Table_AddrType(table)));
_       (Compile_Table_Op(o, op_TableGrow, table, 2, Table_AddrType(table)));
    } অন্যথা {
_       (CheckOperandType(o, 0, Table_AddrType(table)));
_       (CheckOperandType(o, 2, Table_AddrType(table)));
_       (Compile_Table_Op(o, op_TableFill, table, 3, c_m3Type_none));
    }

    _catch: ফেরত ফলাফল;
}

#endif // d_m3HasRefTypes


M3_ফলাফল Compile_তথ্য_Drop (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_ফলাফল ফলাফল = m3Err_none;

    M3_তথ্য_Segment* segment = ফাঁকা;
_   (Read_তথ্য_Segment(o, &segment));

_   (EmitOp(o, op_তথ্য_Drop));
    EmitPointer(o, segment);

    _catch: ফেরত ফলাফল;
}


স্থির
M3_ফলাফল ReadBlockType (IM3Compilation o, IM3FuncType* o_blockType)
{
    M3_ফলাফল ফলাফল = m3Err_none;

#যদি d_m3HasTypedRefs
    // a spelled-out reference type is two bytes, so it cannot be told apart
    // from a type index by the s33 that block types otherwise use
    যদি (o->wasm < o->wasmEnd and (*o->wasm == d_waEncode_ref or *o->wasm == d_waEncode_refNull)) {
        m3type_t refType;
_       (Parse_মান_Type(o->module, &refType, &o->wasm, o->wasmEnd));
        IM3FuncType ftype = ফাঁকা;
_       (AllocFuncType(&ftype, 1));
        ftype->অঙ্ক_Rets  = 1;
        ftype->types[0] = refType;
_       (Environment_AddFuncType(o->module->environment, &ftype));
        *o_blockType = ftype;
        ফেরত ফলাফল;
    }
#endif

    i64 type;
_   (ReadLebSigned(&type, 33, &o->wasm, o->wasmEnd));

    যদি (type < 0) {
        _throwif(m3Err_invalidTypeId, type < -64);
        u8 মান_Type;
_       (NormalizeType(&মান_Type, (i8)type));                                 m3log (compile, d_indent " (type: %s)", get_indention_string (o), c_waTypes [মান_Type]);
        *o_blockType = o->module->environment->retFuncTypes[মান_Type];
    } অন্যথা {
        _throwif("func type out of bounds", type >= o->module->অঙ্ক_Func_Types);
        *o_blockType = o->module->funcTypes[type];                         m3log (compile, d_indent " (type: %s)", get_indention_string (o), SPrintFuncTypeSignature (*o_blockType));
    }
    _catch: ফেরত ফলাফল;
}

স্থির
M3_ফলাফল PreserveArgsAndLocals (IM3Compilation o)
{
    M3_ফলাফল ফলাফল = m3Err_none;

    যদি (o->স্তূপ_সূচক > o->স্তূপ_First_Dynamic_সূচক) {
        u32 অঙ্ক_Args_And_Locals = Get_Function_অঙ্ক_Args_And_Locals(o->function);

        জন্য (u32 i = 0; i < অঙ্ক_Args_And_Locals; ++i) {
            u16 slot = Get_Slot_For_স্তূপ_সূচক(o, i);

            u16 preserved_Slot_সংখ্যা;
_           (FindReferencedLocalWithinCurrentBlock(o, &preserved_Slot_সংখ্যা, slot));

            যদি (preserved_Slot_সংখ্যা != slot) {
                m3type_t     type = Get_স্তূপ_Type_From_Bottom(o, i);                 d_m3Assert (type != c_m3Type_none)
                IM3Operation op   = Is64BitType(type) ? op_CopySlot_64 : op_CopySlot_32;

                EmitOp(o, op);
                EmitSlotOffset(o, preserved_Slot_সংখ্যা);
                EmitSlotOffset(o, slot);
            }
        }
    }

    _catch:
    ফেরত ফলাফল;
}

স্থির
M3_ফলাফল Compile_LoopOrBlock (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_ফলাফল ফলাফল;

    IM3FuncType blockType = ফাঁকা;

_   (PreserveRegisters(o));
_   (PreserveArgsAndLocals(o));

_   (ReadBlockType(o, &blockType));

    যদি (i_opcode == c_waOp_loop) {
        u16 অঙ্ক_Params = Get_Func_Type_অঙ্ক_Params(blockType);
        যদি (অঙ্ক_Params) {
            // instantiate constants
            u16 অঙ্ক_মানসমূহ = Get_অঙ্ক_Block_মানসমূহ_On_স্তূপ(o);

            যদি (অঙ্ক_মানসমূহ >= অঙ্ক_Params) {
                u16 স্তূপ_শীর্ষ = Get_স্তূপ_শীর্ষ_সূচক(o) + 1;

                জন্য (u16 i = স্তূপ_শীর্ষ - অঙ্ক_Params; i < স্তূপ_শীর্ষ; ++i) {
                    u16      slot = Get_Slot_For_স্তূপ_সূচক(o, i);
                    m3type_t type = Get_স্তূপ_Type_From_Bottom(o, i);

                    যদি (IsConstantSlot(o, slot)) {
                        u16 newSlot = c_slotUnused;
_                       (AllocateSlots(o, &newSlot, type));
_                       (Copy_স্তূপ_সূচক_To_Slot(o, newSlot, i));
                        o->wasm_স্তূপ[i] = newSlot;
                    }
                }
            }
        }

_       (EmitOp(o, op_Loop));
    } অন্যথা {
    }

_   (CompileBlock(o, blockType, i_opcode));

    _catch: ফেরত ফলাফল;
}

#যদি d_m3HasExceptionHandling

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
স্থির
M3_ফলাফল EmitCatchStubs (IM3Compilation o)
{
    IM3CodePage displacedPage = ফাঁকা;
_try {
    M3CatchClause* clauses    = o->tryClauses;
    u32            অঙ্ক_Clauses = o->অঙ্ক_Try_Clauses;

    // the body must not see these: a nested try_table sets up its own
    o->tryClauses    = ফাঁকা;
    o->অঙ্ক_Try_Clauses = 0;

    জন্য (u32 i = 0; i < অঙ্ক_Clauses; ++i) {
        M3CatchClause* clause = &clauses[i];

        IM3CompilationScope scope;
_       (GetBlockScope(o, &scope, clause->labelDepth + 1));

        IM3FuncType payload = clause->tag ? clause->tag->type : ফাঁকা;
        u16         অঙ্ক_Args = Get_Func_Type_অঙ্ক_Params(payload);

        // What the stub hands the label - the payload, then the exnref for a
        // _ref clause - has to be what the label takes. ResolveBlockResults
        // only counts the values, and the validator compares base types, so a
        // typed reference is only checked here: (ref null $t) reaching a label
        // that takes (ref $t) has to be rejected.
        {
            bool isLoop         = (scope->opcode == c_waOp_loop);
            u16  অঙ্ক_Label_মানসমূহ = isLoop ? Get_Func_Type_অঙ্ক_Params(scope->type)
                                         : Get_Func_Type_অঙ্ক_Results(scope->type);

            _throwif(m3_Err_type_গুনতি_Mismatch, অঙ্ক_Label_মানসমূহ != অঙ্ক_Args + (clause->hasRef ? 1 : 0));

            জন্য (u16 a = 0; a < অঙ্ক_Label_মানসমূহ; ++a) {
                m3type_t given  = (a < অঙ্ক_Args) ? GetFuncTypeParamType(payload, a)
                                                : (m3type_t)c_m3Type_exnref;
                m3type_t wanted = isLoop ? GetFuncTypeParamType(scope->type, a)
                                         : Get_Func_Type_ফলাফল_Type(scope->type, a);

                _throwif(m3Err_typeMismatch, not IsSubTypeOf(given, wanted));
            }
        }

        IM3CodePage stubPage;
_       (AcquireCompilationCodePage(o, &stubPage));

        pc_t startPC  = GetPagePC(stubPage);
        displacedPage = o->page;
        o->page       = stubPage;

        u16 first_সূচক = o->স্তূপ_সূচক;

        জন্য (u16 a = 0; a < অঙ্ক_Args; ++a) {
_           (ঠেলো_Allocated_Slot(o, GetFuncTypeParamType(payload, a)));
        }

        যদি (clause->hasRef) {
_           (ঠেলো_Allocated_Slot(o, c_m3Type_exnref));
        }

        // emitted even with nothing to move: this is also where an exception
        // no clause will reify gets released
_       (Ensure_Code_Page_অঙ্ক_সারিগুলি(o, 2 * অঙ্ক_Args + 3 + d_m3_Code_Page_Free_সারিগুলি_Threshold));

_       (EmitOp(o, op_CatchPayload));
        EmitConstant32(o, অঙ্ক_Args);

        জন্য (u16 a = 0; a < অঙ্ক_Args; ++a) {
            u16 index = first_সূচক + a;
            EmitSlotOffset(o, Get_Slot_For_স্তূপ_সূচক(o, index));
            EmitConstant32(o, Is64BitType(Get_স্তূপ_Type_From_Bottom(o, index)));
        }

        EmitSlotOffset(o, clause->hasRef ? (i32)Get_Slot_For_স্তূপ_সূচক(o, first_সূচক + অঙ্ক_Args) : -1);

_       (Emit_বের_করো_Try_Frames(o, অঙ্ক_Try_Frames_To_বের_করো(o->block.outer, scope)));

        যদি (scope->opcode == c_waOp_loop) {
_           (ResolveBlockResults(o, scope, /* isBranch: */ true));

_           (EmitOp(o, Op_ContinueLoop(o)));
            EmitPointer(o, scope->pc);
_           (RecordBackEdge(o, scope, false));
        } অন্যথা যদি (scope->depth == 0) {
_           (Return_মানসমূহ(o, scope, /* isBranch: */ true));
_           (EmitOp(o, op_Return));
        } অন্যথা {
_           (ResolveBlockResults(o, scope, /* isBranch: */ true));
_           (EmitPatchingBranch(o, scope));
        }

        IM3CodePage returnTo = displacedPage;
        displacedPage        = ফাঁকা;

        ReleaseCompilationCodePage(o);
        o->page = returnTo;

        *(pc_t*)clause->stubSlot = startPC;

        যখন (o->স্তূপ_সূচক > first_সূচক) {
_           (বের_করো(o));
        }
    }

} _catch:

    // thrown with a stub page installed: release it and put back the one it
    // displaced, which the caller's catch then releases
    যদি (displacedPage) {
        ReleaseCompilationCodePage(o);
        o->page = displacedPage;
    }

    ফেরত ফলাফল;
}


স্থির
M3_ফলাফল Compile_TryTable (IM3Compilation o, m3opcode_t i_opcode)
{
    M3CatchClause* clauses = ফাঁকা;
_try {
    // a catch entry has to be able to assume the operand stack is entirely in
    // slots: an unwind leaves nothing in the registers
_   (PreserveRegisters(o));
_   (PreserveArgsAndLocals(o));

    IM3FuncType blockType = ফাঁকা;
_   (ReadBlockType(o, &blockType));

    u32 অঙ্ক_Clauses;
_   (ReadLEB_u32(&অঙ্ক_Clauses, &o->wasm, o->wasmEnd));

    _throwif("too many catch clauses", অঙ্ক_Clauses > d_m3_Max_Sane_Tags_গুনতি);

    যদি (অঙ্ক_Clauses) {
        clauses = m3_AllocArray(M3CatchClause, অঙ্ক_Clauses);
        _throwifnull(clauses);
    }

    জন্য (u32 i = 0; i < অঙ্ক_Clauses; ++i) {
        u8 kind;
_       (Read_u8(&kind, &o->wasm, o->wasmEnd));
        _throwif(m3Err_wasmMalformed, kind > 0x03);

        clauses[i].tag    = ফাঁকা;
        clauses[i].hasRef = (kind == 0x01 or kind == 0x03);

        যদি (kind == 0x00 or kind == 0x01) {
            u32 tag_সূচক;
_           (ReadLEB_u32(&tag_সূচক, &o->wasm, o->wasmEnd));
            _throwif(m3Err_unknownTag, tag_সূচক >= o->module->অঙ্ক_Tags);

            IM3Tag tag     = &o->module->tags[tag_সূচক];
            clauses[i].tag = tag->resolved ? tag->resolved : tag;
        }

_       (ReadLEB_u32(&clauses[i].labelDepth, &o->wasm, o->wasmEnd));
    }

    // op_TryTable, the clause count, and two words per clause, all of which have
    // to land on one page: the op reads the table by offset from its own pc
_   (Ensure_Code_Page_অঙ্ক_সারিগুলি(o, 2 * অঙ্ক_Clauses + 2 + d_m3_Code_Page_Free_সারিগুলি_Threshold));

_   (EmitOp(o, op_TryTable));
    EmitConstant32(o, অঙ্ক_Clauses);

    জন্য (u32 i = 0; i < অঙ্ক_Clauses; ++i) {
        EmitPointer(o, clauses[i].tag);
        clauses[i].stubSlot = ReservePointer(o);
    }

    o->tryClauses    = clauses;
    o->অঙ্ক_Try_Clauses = অঙ্ক_Clauses;

_   (CompileBlock(o, blockType, c_waOp_tryTable));

} _catch:

    o->tryClauses    = ফাঁকা;
    o->অঙ্ক_Try_Clauses = 0;

    m3_Free(clauses);

    ফেরত ফলাফল;
}


স্থির
M3_ফলাফল Compile_Throw (IM3Compilation o, m3opcode_t i_opcode)
{
_try {
    u32 tag_সূচক;
_   (ReadLEB_u32(&tag_সূচক, &o->wasm, o->wasmEnd));
    _throwif(m3Err_unknownTag, tag_সূচক >= o->module->অঙ্ক_Tags);

    IM3Tag tag = &o->module->tags[tag_সূচক];
    যদি (tag->resolved) {
        tag = tag->resolved;
    }

    u16 অঙ্ক_Args = Get_Func_Type_অঙ্ক_Params(tag->type);

    // the payload is read out of slots, so nothing may still be in a register
_   (PreserveRegisters(o));

    // check the payload before emitting anything, so a mistyped throw does not
    // leave half an operation behind
    bool isPolymorphic = Is_স্তূপ_Polymorphic(o);

    যদি (not isPolymorphic) {
        _throwif(m3_Err_type_গুনতি_Mismatch, Get_অঙ্ক_Block_মানসমূহ_On_স্তূপ(o) < অঙ্ক_Args);

        জন্য (u16 i = 0; i < অঙ্ক_Args; ++i) {
            u16 index = (u16)(o->স্তূপ_সূচক - অঙ্ক_Args + i);

            _throwif(m3Err_typeMismatch, not IsSubTypeOf(Get_স্তূপ_Type_From_Bottom(o, index),
                                                         GetFuncTypeParamType(tag->type, i)));
        }
    }

_   (Ensure_Code_Page_অঙ্ক_সারিগুলি(o, 2 * অঙ্ক_Args + 3 + d_m3_Code_Page_Free_সারিগুলি_Threshold));

_   (EmitOp(o, op_Throw));
    EmitPointer(o, tag);
    EmitConstant32(o, অঙ্ক_Args);

    // the payload is named in the order the tag declares it, so the deepest of
    // the arguments on the stack comes first. In unreachable code there is
    // nothing on the stack to name and nothing will run this, so slot 0 stands
    // in for an operand that isn't there.
    জন্য (u16 i = 0; i < অঙ্ক_Args; ++i) {
        m3type_t type = GetFuncTypeParamType(tag->type, i);

        EmitSlotOffset(o, isPolymorphic ? 0 : Get_Slot_For_স্তূপ_সূচক(o, (u16)(o->স্তূপ_সূচক - অঙ্ক_Args + i)));
        EmitConstant32(o, Is64BitType(type));
    }

    যদি (not isPolymorphic) {
        জন্য (u16 i = 0; i < অঙ্ক_Args; ++i) {
_           (বের_করো(o));
        }
    }

_   (Set_স্তূপ_Polymorphic(o));

} _catch: ফেরত ফলাফল;
}


স্থির
M3_ফলাফল Compile_ThrowRef (IM3Compilation o, m3opcode_t i_opcode)
{
_try {
    যদি (not Is_স্তূপ_Polymorphic(o)) {
_       (PreserveRegisterIfOccupied(o, c_m3Type_i64));

        _throwif(m3Err_typeMismatch, BaseTypeOf(Get_স্তূপ_শীর্ষ_Type(o)) != c_m3Type_exnref);

_       (EmitOp(o, op_ThrowRef));
        EmitSlotOffset(o, Get_স্তূপ_শীর্ষ_Slot_সংখ্যা(o));

_       (বের_করো(o));
    }

_   (Set_স্তূপ_Polymorphic(o));

} _catch: ফেরত ফলাফল;
}

#endif // d_m3HasExceptionHandling

#যদি d_m3_Has_স্তূপ_Switching

স্থির
M3_ফলাফল Compile_ContNew (IM3Compilation o, m3opcode_t i_opcode)
{
_try {
    u32 type_সূচক;
_   (ReadLEB_u32(&type_সূচক, &o->wasm, o->wasmEnd));
    _throwif(m3_Err_type_গুনতি_Mismatch, type_সূচক >= o->module->অঙ্ক_Func_Types);

    IM3FuncType funcType = o->module->funcTypes[type_সূচক];
    _throwif("type is not a continuation", not funcType->isContinuation);

    // the operands are read out of slots, so nothing may still be in a register
_   (PreserveRegisters(o));

    // cont.new $ct : [(ref null $ft)] -> [(ref $ct)], where $ct ~~ cont $ft:
    // the function has to be one this continuation type can run
    m3type_t শীর্ষ_Type = Get_স্তূপ_শীর্ষ_Type(o);
    _throwif(m3Err_typeMismatch,
             not IsSubTypeOf(শীর্ষ_Type, RefTypeOfFuncType(funcType->contFuncType, false)));

    u16 funcSlot = Get_স্তূপ_শীর্ষ_Slot_সংখ্যা(o);
_   (বের_করো(o));

    u16 dstSlot;
_   (ঠেলো_Allocated_Slot(o, RefTypeOfFuncType(funcType, true)));
    dstSlot = Get_স্তূপ_শীর্ষ_Slot_সংখ্যা(o);

_   (Ensure_Code_Page_অঙ্ক_সারিগুলি(o, 4 + d_m3_Code_Page_Free_সারিগুলি_Threshold));
_   (EmitOp(o, op_ContNew));
    EmitPointer(o, funcType);
    EmitSlotOffset(o, dstSlot);
    EmitSlotOffset(o, funcSlot);

} _catch: ফেরত ফলাফল;
}

স্থির
M3_ফলাফল Compile_ContBind (IM3Compilation o, m3opcode_t i_opcode)
{
_try {
    u32 typeIndex1, typeIndex2;
_   (ReadLEB_u32(&typeIndex1, &o->wasm, o->wasmEnd));
_   (ReadLEB_u32(&typeIndex2, &o->wasm, o->wasmEnd));
    _throwif(m3_Err_type_গুনতি_Mismatch, typeIndex1 >= o->module->অঙ্ক_Func_Types);
    _throwif(m3_Err_type_গুনতি_Mismatch, typeIndex2 >= o->module->অঙ্ক_Func_Types);

    IM3FuncType ct1 = o->module->funcTypes[typeIndex1];
    IM3FuncType ct2 = o->module->funcTypes[typeIndex2];
    _throwif("type is not a continuation", not ct1->isContinuation || not ct2->isContinuation);

    // the operands are read out of slots, so nothing may still be in a register
_   (PreserveRegisters(o));

    m3type_t contType = Get_স্তূপ_শীর্ষ_Type(o);
    _throwif(m3Err_typeMismatch, not IsSubTypeOf(contType, RefTypeOfFuncType(ct1, false)));
    u16 contSlot = Get_স্তূপ_শীর্ষ_Slot_সংখ্যা(o);
_   (বের_করো(o));

    IM3FuncType ft1 = ct1->contFuncType ? ct1->contFuncType : ct1;
    IM3FuncType ft2 = ct2->contFuncType ? ct2->contFuncType : ct2;

    u16 অঙ্ক_Params1 = Get_Func_Type_অঙ্ক_Params(ft1);
    u16 অঙ্ক_Params2 = Get_Func_Type_অঙ্ক_Params(ft2);
    _throwif(m3Err_typeMismatch, অঙ্ক_Params1 < অঙ্ক_Params2);
    u32 অঙ্ক_Bound = অঙ্ক_Params1 - অঙ্ক_Params2;

    u16 অঙ্ক_Results1 = Get_Func_Type_অঙ্ক_Results(ft1);
    u16 অঙ্ক_Results2 = Get_Func_Type_অঙ্ক_Results(ft2);
    _throwif(m3Err_typeMismatch, অঙ্ক_Results1 != অঙ্ক_Results2);
    জন্য (u16 i = 0; i < অঙ্ক_Results1; ++i) {
        _throwif(m3Err_typeMismatch, Get_Func_Type_ফলাফল_Type(ft1, i) != Get_Func_Type_ফলাফল_Type(ft2, i));
    }
    জন্য (u16 i = 0; i < অঙ্ক_Params2; ++i) {
        _throwif(m3Err_typeMismatch, GetFuncTypeParamType(ft1, অঙ্ক_Bound + i) != GetFuncTypeParamType(ft2, i));
    }

    u16 argSlots[d_m3MaxContinuationPayload];
    _throwif("too many bound args", অঙ্ক_Bound > d_m3MaxContinuationPayload);

    জন্য (u32 i = 0; i < অঙ্ক_Bound; ++i) {
        u32      paramIdx     = অঙ্ক_Bound - 1 - i;
        m3type_t expectedType = GetFuncTypeParamType(ft1, paramIdx);
        m3type_t actualType   = Get_স্তূপ_শীর্ষ_Type(o);
        _throwif(m3Err_typeMismatch, not IsSubTypeOf(actualType, expectedType));
        argSlots[paramIdx] = Get_স্তূপ_শীর্ষ_Slot_সংখ্যা(o);
_       (বের_করো(o));
    }

    u16 dstSlot;
_   (ঠেলো_Allocated_Slot(o, RefTypeOfFuncType(ct2, true)));
    dstSlot = Get_স্তূপ_শীর্ষ_Slot_সংখ্যা(o);

_   (Ensure_Code_Page_অঙ্ক_সারিগুলি(o, 6 + অঙ্ক_Bound * 2 + d_m3_Code_Page_Free_সারিগুলি_Threshold));
_   (EmitOp(o, op_ContBind));
    EmitPointer(o, ct1);
    EmitPointer(o, ct2);
    EmitSlotOffset(o, dstSlot);
    EmitSlotOffset(o, contSlot);
    EmitConstant32(o, অঙ্ক_Bound);
    জন্য (u32 i = 0; i < অঙ্ক_Bound; ++i) {
        EmitSlotOffset(o, argSlots[i]);
        EmitConstant32(o, Is64BitType(GetFuncTypeParamType(ft1, i)));
    }

} _catch: ফেরত ফলাফল;
}

স্থির
M3_ফলাফল Compile_Suspend (IM3Compilation o, m3opcode_t i_opcode)
{
_try {
    u32 tag_সূচক;
_   (ReadLEB_u32(&tag_সূচক, &o->wasm, o->wasmEnd));
    _throwif(m3Err_unknownTag, tag_সূচক >= o->module->অঙ্ক_Tags);

    IM3Tag tag = &o->module->tags[tag_সূচক];
    যদি (tag->resolved) {
        tag = tag->resolved;
    }
    u16 অঙ্ক_Params  = Get_Func_Type_অঙ্ক_Params(tag->type);
    u16 অঙ্ক_Results = Get_Func_Type_অঙ্ক_Results(tag->type);

_   (PreserveRegisters(o));

    u16 paramSlots[d_m3MaxContinuationPayload];
    _throwif("too many tag params", অঙ্ক_Params > d_m3MaxContinuationPayload);
    জন্য (u16 i = 0; i < অঙ্ক_Params; ++i) {
        u16      pIdx         = অঙ্ক_Params - 1 - i;
        m3type_t expectedType = GetFuncTypeParamType(tag->type, pIdx);
        m3type_t actualType   = Get_স্তূপ_শীর্ষ_Type(o);
        _throwif(m3Err_typeMismatch, not IsSubTypeOf(actualType, expectedType));
        paramSlots[pIdx] = Get_স্তূপ_শীর্ষ_Slot_সংখ্যা(o);
_       (বের_করো(o));
    }

    u16 ফলাফল_Slots[d_m3MaxContinuationPayload];
    _throwif("too many tag results", অঙ্ক_Results > d_m3MaxContinuationPayload);
    জন্য (u16 i = 0; i < অঙ্ক_Results; ++i) {
        m3type_t rtype = Get_Func_Type_ফলাফল_Type(tag->type, i);
_       (ঠেলো_Allocated_Slot(o, rtype));
        ফলাফল_Slots[i] = Get_স্তূপ_শীর্ষ_Slot_সংখ্যা(o);
    }

_   (Ensure_Code_Page_অঙ্ক_সারিগুলি(o, 4 + অঙ্ক_Params * 2 + অঙ্ক_Results * 2 + d_m3_Code_Page_Free_সারিগুলি_Threshold));
_   (EmitOp(o, op_Suspend));
    EmitPointer(o, tag);
    EmitConstant32(o, অঙ্ক_Params);
    জন্য (u16 i = 0; i < অঙ্ক_Params; ++i) {
        EmitSlotOffset(o, paramSlots[i]);
        EmitConstant32(o, Is64BitType(GetFuncTypeParamType(tag->type, i)));
    }
    EmitConstant32(o, অঙ্ক_Results);
    জন্য (u16 i = 0; i < অঙ্ক_Results; ++i) {
        EmitSlotOffset(o, ফলাফল_Slots[i]);
        EmitConstant32(o, Is64BitType(Get_Func_Type_ফলাফল_Type(tag->type, i)));
    }

    // the results are part of the frame here: op_Suspend clears them, and
    // whatever resumes this writes them before anything reads them
_   (RecordSafePoint(o, safepoint_suspend, 0, অঙ্ক_Results, 0, ফাঁকা));

} _catch: ফেরত ফলাফল;
}

// resume, resume_throw and resume_throw_ref. All three run a continuation
// under a handler table and take its results; they differ only in what they
// hand it to start with - arguments, a fresh exception, or one off the stack.
//
// Emitted as: the continuation type, the slot holding it, the operands that
// belong to this form, the handler table, then the result slots. Keeping the
// tail identical is what lets one runtime path finish all three.
স্থির
M3_ফলাফল Compile_Resume (IM3Compilation o, m3opcode_t i_opcode)
{
    M3ResumeHandler* handlers      = ফাঁকা;
    IM3CodePage      displacedPage = ফাঁকা;
_try {
    bool isThrow    = (i_opcode == c_waOp_resumeThrow);
    bool isThrowRef = (i_opcode == c_waOp_resumeThrowRef);

    u32 type_সূচক;
_   (ReadLEB_u32(&type_সূচক, &o->wasm, o->wasmEnd));
    _throwif(m3_Err_type_গুনতি_Mismatch, type_সূচক >= o->module->অঙ্ক_Func_Types);

    IM3FuncType contFuncType = o->module->funcTypes[type_সূচক];
    _throwif("type is not a continuation", not contFuncType->isContinuation);

    IM3FuncType innerType      = contFuncType->contFuncType;
    u16         অঙ্ক_Cont_Results = Get_Func_Type_অঙ্ক_Results(innerType);

    IM3Tag exnTag      = ফাঁকা;
    u16    অঙ্ক_Operands = 0;

    যদি (isThrow) {
        u32 tag_সূচক;
_       (ReadLEB_u32(&tag_সূচক, &o->wasm, o->wasmEnd));
        _throwif(m3Err_unknownTag, tag_সূচক >= o->module->অঙ্ক_Tags);

        exnTag = &o->module->tags[tag_সূচক];

        // it is an exception that gets raised, so the tag must be one throw
        // could have raised: no results
        _throwif(m3Err_typeMismatch, Get_Func_Type_অঙ্ক_Results(exnTag->type) != 0);

        অঙ্ক_Operands = Get_Func_Type_অঙ্ক_Params(exnTag->type);
    } অন্যথা যদি (not isThrowRef) {
        // a plain resume hands over the continuation's own parameters. An
        // aborted one never receives them, so they are unconstrained there.
        অঙ্ক_Operands = Get_Func_Type_অঙ্ক_Params(innerType);
    }

    u32 অঙ্ক_Clauses;
_   (ReadLEB_u32(&অঙ্ক_Clauses, &o->wasm, o->wasmEnd));

    u32 clauseLabelDepths[16];
    _throwif("too many resume clauses", অঙ্ক_Clauses > 16);

    যদি (অঙ্ক_Clauses) {
        handlers = m3_AllocArray(M3ResumeHandler, অঙ্ক_Clauses);
        _throwifnull(handlers);
    }

    জন্য (u32 i = 0; i < অঙ্ক_Clauses; ++i) {
        u8 kind;
_       (Read_u8(&kind, &o->wasm, o->wasmEnd));
        _throwif(m3Err_wasmMalformed, kind > 0x01);

        handlers[i].kind = kind;

        u32 tag_সূচক;
_       (ReadLEB_u32(&tag_সূচক, &o->wasm, o->wasmEnd));
        _throwif(m3Err_unknownTag, tag_সূচক >= o->module->অঙ্ক_Tags);

        IM3Tag tag         = &o->module->tags[tag_সূচক];
        handlers[i].tag    = tag->resolved ? tag->resolved : tag;
        handlers[i].stubPC = ফাঁকা;

        যদি (kind == 0x00) {
_           (ReadLEB_u32(&clauseLabelDepths[i], &o->wasm, o->wasmEnd));
        } অন্যথা {
            // (on $e switch): the tag carries no payload, and what the switch
            // site yields is what this resume yields
            _throwif(m3Err_typeMismatch, Get_Func_Type_অঙ্ক_Params(handlers[i].tag->type) != 0);
            clauseLabelDepths[i] = 0;
        }
    }

    // the operands are read out of slots, so nothing may still be in a register
_   (PreserveRegisters(o));

    m3type_t cont_শীর্ষ_Type = Get_স্তূপ_শীর্ষ_Type(o);
    _throwif(m3Err_typeMismatch, not IsSubTypeOf(cont_শীর্ষ_Type, RefTypeOfFuncType(contFuncType, false)));
    u16 contSlot = Get_স্তূপ_শীর্ষ_Slot_সংখ্যা(o);
_   (বের_করো(o));

    u16 exnSlot = 0;

    যদি (isThrowRef) {
        m3type_t exnType = Get_স্তূপ_শীর্ষ_Type(o);
        _throwif(m3Err_typeMismatch, BaseTypeOf(exnType) != c_m3Type_exnref);
        exnSlot = Get_স্তূপ_শীর্ষ_Slot_সংখ্যা(o);
_       (বের_করো(o));
    }

    u16 operandSlots[d_m3MaxContinuationPayload];
    _throwif("too many resume operands", অঙ্ক_Operands > d_m3MaxContinuationPayload);

    IM3FuncType operandType = isThrow ? exnTag->type : innerType;

    জন্য (u16 i = 0; i < অঙ্ক_Operands; ++i) {
        u16      pIdx         = (u16)(অঙ্ক_Operands - 1 - i);
        m3type_t expectedType = GetFuncTypeParamType(operandType, pIdx);
        m3type_t actualType   = Get_স্তূপ_শীর্ষ_Type(o);
        _throwif(m3Err_typeMismatch, not IsSubTypeOf(actualType, expectedType));
        operandSlots[pIdx] = Get_স্তূপ_শীর্ষ_Slot_সংখ্যা(o);
_       (বের_করো(o));
    }

    জন্য (u32 i = 0; i < অঙ্ক_Clauses; ++i) {
        যদি (handlers[i].kind != 0x00) {
            চালু;
        }

        IM3CompilationScope scope;
_       (GetBlockScope(o, &scope, clauseLabelDepths[i]));

        IM3Tag tag        = handlers[i].tag;
        u16    অঙ্ক_Payload = Get_Func_Type_অঙ্ক_Params(tag->type);

        m3type_t labelContType = (scope->opcode == c_waOp_loop)
                                   ? GetFuncTypeParamType(scope->type, অঙ্ক_Payload)
                                   : Get_Func_Type_ফলাফল_Type(scope->type, অঙ্ক_Payload);

        IM3CodePage stubPage;
_       (AcquireCompilationCodePage(o, &stubPage));
        pc_t startPC  = GetPagePC(stubPage);
        displacedPage = o->page;
        o->page       = stubPage;

        u16 first_সূচক = o->স্তূপ_সূচক;
        জন্য (u16 a = 0; a < অঙ্ক_Payload; ++a) {
_           (ঠেলো_Allocated_Slot(o, GetFuncTypeParamType(tag->type, a)));
        }

        // The handler is handed the suspended continuation.
_       (ঠেলো_Allocated_Slot(o, labelContType));

_       (Ensure_Code_Page_অঙ্ক_সারিগুলি(o, 2 * অঙ্ক_Payload + 5 + d_m3_Code_Page_Free_সারিগুলি_Threshold));
_       (EmitOp(o, op_ResumePayload));
        EmitPointer(o, Module_ContTypeOfRef(o->module, labelContType));
        EmitConstant32(o, অঙ্ক_Payload);
        জন্য (u16 a = 0; a < অঙ্ক_Payload; ++a) {
            u16 index = first_সূচক + a;
            EmitSlotOffset(o, Get_Slot_For_স্তূপ_সূচক(o, index));
            EmitConstant32(o, Is64BitType(Get_স্তূপ_Type_From_Bottom(o, index)));
        }
        EmitSlotOffset(o, Get_Slot_For_স্তূপ_সূচক(o, first_সূচক + অঙ্ক_Payload));

#  যদি d_m3HasExceptionHandling
        // the clause leaves every try region between the resume and its label,
        // and leaving one retires its handlers, the way a branch does
_       (Emit_বের_করো_Try_Frames(o, অঙ্ক_Try_Frames_To_বের_করো(&o->block, scope)));
#  endif

        যদি (scope->opcode == c_waOp_loop) {
_           (ResolveBlockResults(o, scope, true));
_           (EmitOp(o, Op_ContinueLoop(o)));
            EmitPointer(o, scope->pc);
_           (RecordBackEdge(o, scope, false));
        } অন্যথা যদি (scope->depth == 0) {
_           (Return_মানসমূহ(o, scope, true));
_           (EmitOp(o, op_Return));
        } অন্যথা {
_           (ResolveBlockResults(o, scope, true));
_           (EmitPatchingBranch(o, scope));
        }

        IM3CodePage returnTo = displacedPage;
        displacedPage        = ফাঁকা;

        ReleaseCompilationCodePage(o);
        o->page = returnTo;

        handlers[i].stubPC = startPC;

        যখন (o->স্তূপ_সূচক > first_সূচক) {
_           (বের_করো(o));
        }
    }

    u16 resSlots[d_m3MaxContinuationPayload];
    _throwif("too many cont results", অঙ্ক_Cont_Results > d_m3MaxContinuationPayload);
    জন্য (u16 i = 0; i < অঙ্ক_Cont_Results; ++i) {
        m3type_t rtype = Get_Func_Type_ফলাফল_Type(innerType, i);
_       (ঠেলো_Allocated_Slot(o, rtype));
        resSlots[i] = Get_স্তূপ_শীর্ষ_Slot_সংখ্যা(o);
    }

_   (Ensure_Code_Page_অঙ্ক_সারিগুলি(o, 8 + অঙ্ক_Operands * 2 + অঙ্ক_Clauses * 3 + অঙ্ক_Cont_Results * 2 +
                                  d_m3_Code_Page_Free_সারিগুলি_Threshold));

_   (EmitOp(o, op_Resume));
    EmitPointer(o, contFuncType);
    EmitSlotOffset(o, contSlot);
    EmitConstant32(o, isThrow ? d_m3ResumeThrow : (isThrowRef ? d_m3ResumeThrowRef : d_m3ResumeNormal));

    যদি (isThrowRef) {
        EmitSlotOffset(o, exnSlot);
    } অন্যথা {
        যদি (isThrow) {
            EmitPointer(o, exnTag);
        }

        EmitConstant32(o, অঙ্ক_Operands);
        জন্য (u16 i = 0; i < অঙ্ক_Operands; ++i) {
            EmitSlotOffset(o, operandSlots[i]);
            EmitConstant32(o, Is64BitType(GetFuncTypeParamType(operandType, i)));
        }
    }

    EmitConstant32(o, অঙ্ক_Clauses);
    জন্য (u32 i = 0; i < অঙ্ক_Clauses; ++i) {
        EmitConstant32(o, handlers[i].kind);
        EmitPointer(o, handlers[i].tag);
        EmitPointer(o, handlers[i].stubPC);
    }

    EmitConstant32(o, অঙ্ক_Cont_Results);
    জন্য (u16 i = 0; i < অঙ্ক_Cont_Results; ++i) {
        EmitSlotOffset(o, resSlots[i]);
        EmitConstant32(o, Is64BitType(Get_Func_Type_ফলাফল_Type(innerType, i)));
    }

    // a suspend passing through the resumed continuation leaves this frame
    // waiting here, with the continuation's results not yet taken
_   (RecordSafePoint(o, safepoint_resume, 0, অঙ্ক_Cont_Results, (u16)অঙ্ক_Clauses, ফাঁকা));
#  যদি d_m3HasSnapshots
    যদি (o->snapshotMap and o->page) {
        o->snapshotMap->safePoints[o->snapshotMap->অঙ্ক_Safe_Points - 1].resumeType = RefTypeOfFuncType(contFuncType, true);
    }
#  endif

} _catch:
    যদি (displacedPage) {
        ReleaseCompilationCodePage(o);
        o->page = displacedPage;
    }
    m3_Free(handlers);
    ফেরত ফলাফল;
}


স্থির
M3_ফলাফল Compile_Switch (IM3Compilation o, m3opcode_t i_opcode)
{
_try {
    u32 type_সূচক, tag_সূচক;
_   (ReadLEB_u32(&type_সূচক, &o->wasm, o->wasmEnd));
_   (ReadLEB_u32(&tag_সূচক, &o->wasm, o->wasmEnd));
    _throwif(m3_Err_type_গুনতি_Mismatch, type_সূচক >= o->module->অঙ্ক_Func_Types);
    _throwif(m3Err_unknownTag, tag_সূচক >= o->module->অঙ্ক_Tags);

    IM3FuncType targetContType = o->module->funcTypes[type_সূচক];
    IM3Tag      tag            = &o->module->tags[tag_সূচক];
    যদি (tag->resolved) {
        tag = tag->resolved;
    }

    _throwif("type is not a continuation", not targetContType->isContinuation);

    // a switch tag carries no payload of its own: the values travel as the
    // peer's arguments, and the tag's results are what the switch site yields
    _throwif(m3Err_typeMismatch, Get_Func_Type_অঙ্ক_Params(tag->type) != 0);

    // switch $ct1 : [t1* (ref null $ct1)] -> [t2*]
    //   where $ct1 = cont [t1* (ref null? $ct2)] -> [..]
    //   and   $ct2 = cont [t2*] -> [..]
    //
    // The trailing parameter of $ct1 is not an operand: the peer receives the
    // continuation this instruction suspends. And what this instruction yields
    // is not $ct1's results but $ct2's parameters - the values whoever comes
    // back this way will hand over.
    IM3FuncType targetType = targetContType->contFuncType;
    u16         অঙ্ক_Params  = Get_Func_Type_অঙ্ক_Params(targetType);

    _throwif("switch target takes no continuation", অঙ্ক_Params == 0);

    m3type_t    peerRefType  = GetFuncTypeParamType(targetType, (u16)(অঙ্ক_Params - 1));
    IM3FuncType selfContType = Module_ContTypeOfRef(o->module, peerRefType);

    _throwif(m3Err_typeMismatch, not selfContType);

    IM3FuncType selfType   = selfContType->contFuncType;
    u16         অঙ্ক_Args    = (u16)(অঙ্ক_Params - 1);
    u16         অঙ্ক_Results = Get_Func_Type_অঙ্ক_Params(selfType);

    // the operands are read out of slots, so nothing may still be in a register
_   (PreserveRegisters(o));

    m3type_t cont_শীর্ষ_Type = Get_স্তূপ_শীর্ষ_Type(o);
    _throwif(m3Err_typeMismatch, not IsSubTypeOf(cont_শীর্ষ_Type, RefTypeOfFuncType(targetContType, false)));
    u16 targetContSlot = Get_স্তূপ_শীর্ষ_Slot_সংখ্যা(o);
_   (বের_করো(o));

    u16 argSlots[d_m3MaxContinuationPayload];
    _throwif("too many switch params", অঙ্ক_Args > d_m3MaxContinuationPayload);

    জন্য (u16 i = 0; i < অঙ্ক_Args; ++i) {
        u16      pIdx         = (u16)(অঙ্ক_Args - 1 - i);
        m3type_t expectedType = GetFuncTypeParamType(targetType, pIdx);
        m3type_t actualType   = Get_স্তূপ_শীর্ষ_Type(o);
        _throwif(m3Err_typeMismatch, not IsSubTypeOf(actualType, expectedType));
        argSlots[pIdx] = Get_স্তূপ_শীর্ষ_Slot_সংখ্যা(o);
_       (বের_করো(o));
    }

    u16 ফলাফল_Slots[d_m3MaxContinuationPayload];
    _throwif("too many switch results", অঙ্ক_Results > d_m3MaxContinuationPayload);

    জন্য (u16 i = 0; i < অঙ্ক_Results; ++i) {
_       (ঠেলো_Allocated_Slot(o, GetFuncTypeParamType(selfType, i)));
        ফলাফল_Slots[i] = Get_স্তূপ_শীর্ষ_Slot_সংখ্যা(o);
    }

_   (Ensure_Code_Page_অঙ্ক_সারিগুলি(o, 6 + অঙ্ক_Args * 2 + অঙ্ক_Results * 2 + d_m3_Code_Page_Free_সারিগুলি_Threshold));
_   (EmitOp(o, op_Switch));
    EmitPointer(o, targetContType);
    EmitPointer(o, tag);
    EmitSlotOffset(o, targetContSlot);
    EmitConstant32(o, অঙ্ক_Args);
    জন্য (u16 i = 0; i < অঙ্ক_Args; ++i) {
        EmitSlotOffset(o, argSlots[i]);
        EmitConstant32(o, Is64BitType(GetFuncTypeParamType(targetType, i)));
    }
    EmitConstant32(o, অঙ্ক_Results);
    জন্য (u16 i = 0; i < অঙ্ক_Results; ++i) {
        EmitSlotOffset(o, ফলাফল_Slots[i]);
        EmitConstant32(o, Is64BitType(GetFuncTypeParamType(selfType, i)));
    }

    // as with suspend: op_Switch clears the results before it leaves
_   (RecordSafePoint(o, safepoint_suspend, 0, অঙ্ক_Results, 0, ফাঁকা));

} _catch: ফেরত ফলাফল;
}

#endif // d_m3HasStackSwitching


স্থির
M3_ফলাফল CompileElseBlock (IM3Compilation o, pc_t* o_startPC, IM3FuncType i_blockType)
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

    যদি (o->page != savedPage) {
        ReleaseCompilationCodePage(o);
    }

    o->page = savedPage;

    ফেরত ফলাফল;
}

স্থির
M3_ফলাফল Compile_If (IM3Compilation o, m3opcode_t i_opcode)
{
    /*      [   op_If   ]
            [ <else-pc> ]   ---->   [ ..else..  ]
            [  ..if..   ]           [ ..block.. ]
            [ ..block.. ]           [ op_Branch ]
            [    end    ]  <-----   [  <end-pc> ]       */

_try {

    // Spec: if condition must be i32
    যদি (not Is_স্তূপ_Polymorphic(o)) {
        m3type_t condType = Get_স্তূপ_শীর্ষ_Type(o);
        _throwif(m3Err_typeMismatch, condType != c_m3Type_none and condType != c_m3Type_i32);
    }

_   (Preserve_Non_শীর্ষ_Registers(o));
_   (PreserveArgsAndLocals(o));

#যদি d_m3FuseBranch
    IM3Operation fuseOp = GetBranchFusionOp(o, true);

    যদি (fuseOp) {
        // absorb the if into the compare that produced the condition
        *(IM3Operation*)o->foldPatchPC = fuseOp;
        InvalidateFold(o);

_       (বের_করো(o));                          // the condition is consumed inside the fused op
    } অন্যথা
#endif
    {
        IM3Operation op = Is_স্তূপ_শীর্ষ_In_Register(o) ? op_If_r : op_If_s;

_       (EmitOp(o, op));
_       (Emit_Slot_অঙ্ক_Of_স্তূপ_শীর্ষ_And_বের_করো(o));
    }

    pc_t* pc = (pc_t*)ReservePointer(o);

    IM3FuncType blockType = ফাঁকা;
_   (ReadBlockType(o, &blockType));     //  dump_type_stack (o);

    u16 স্তূপ_সূচক = o->স্তূপ_সূচক;

_   (CompileBlock(o, blockType, i_opcode));

    যদি (o->previousOpcode == c_waOp_else) {
        o->স্তূপ_সূচক = স্তূপ_সূচক;
_       (CompileElseBlock(o, pc, blockType));
    } অন্যথা {
        // if block produces values and there isn't a defined else
        // case, then we need to make one up so that the pass-through
        // results end up in the right place
        যদি (Get_Func_Type_অঙ্ক_Results(blockType)) {
            // rewind to the if's end to create a fake else block
            o->wasm--;
            o->স্তূপ_সূচক = স্তূপ_সূচক;     // dump_type_stack (o);

_           (CompileElseBlock(o, pc, blockType));
        } অন্যথা {
            *pc = GetPC(o);
        }
    }

    } _catch: ফেরত ফলাফল;
}

স্থির
M3_ফলাফল Compile_Select (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_ফলাফল ফলাফল = m3Err_none;

    u16 slots[3] = { c_slotUnused, c_slotUnused, c_slotUnused };

    IM3Operation op = ফাঁকা;

    m3type_t type = Get_স্তূপ_Type_From_শীর্ষ(o, 1); // get type of selection

    যদি (not Is_স্তূপ_Polymorphic(o)) {
        // Spec: the condition operand (top) must be i32
        m3type_t condType = Get_স্তূপ_Type_From_শীর্ষ(o, 0);
        _throwif(m3Err_typeMismatch, condType != c_m3Type_none and condType != c_m3Type_i32);

        // Spec: the two value operands (below condition) must have matching types
        m3type_t type2 = Get_স্তূপ_Type_From_শীর্ষ(o, 2);
        _throwif(m3Err_typeMismatch, type != c_m3Type_none and type2 != c_m3Type_none and
                                       not (IsSubTypeOf(type, type2) or IsSubTypeOf(type2, type)));
    }

    যদি (IsFpType(type)) {
#যদি d_m3HasFloat
        // not consuming a fp reg, so preserve
        যদি (not Is_স্তূপ_শীর্ষ_Minus1_In_Register(o) and
            not Is_স্তূপ_শীর্ষ_Minus2_In_Register(o)) {
_           (PreserveRegisterIfOccupied(o, type));
        }

        bool selectorInReg = Is_স্তূপ_শীর্ষ_In_Register(o);
        slots[0]           = Get_স্তূপ_শীর্ষ_Slot_সংখ্যা(o);
_       (বের_করো(o));

        u32 op_সূচক = 0;

        জন্য (u32 i = 1; i <= 2; ++i) {
            যদি (Is_স্তূপ_শীর্ষ_In_Register(o)) {
                op_সূচক = i;
            } অন্যথা {
                slots[i] = Get_স্তূপ_শীর্ষ_Slot_সংখ্যা(o);
            }

_           (বের_করো(o));
        }

        op = c_fpSelectOps[type - c_m3Type_f32][selectorInReg][op_সূচক];
#অন্যথা
        _throw(m3Err_unknownOpcode);
#endif
    } অন্যথা যদি (IsIntType(type) or IsRefType(type)) {
        // 'sss' operation doesn't consume a register, so might have to protected its contents
        যদি (not Is_স্তূপ_শীর্ষ_In_Register(o) and
            not Is_স্তূপ_শীর্ষ_Minus1_In_Register(o) and
            not Is_স্তূপ_শীর্ষ_Minus2_In_Register(o)) {
_           (PreserveRegisterIfOccupied(o, type));
        }

        u32 op_সূচক = 3;  // op_Select_*_sss

        জন্য (u32 i = 0; i < 3; ++i) {
            যদি (Is_স্তূপ_শীর্ষ_In_Register(o)) {
                op_সূচক = i;
            } অন্যথা {
                slots[i] = Get_স্তূপ_শীর্ষ_Slot_সংখ্যা(o);
            }

_           (বের_করো(o));
        }

        // a reference is a pointer-sized integer as far as select is concerned
        u32 type_সূচক = IsRefType(type) ? ((M3_SIZEOF_PTR == 8) ? 1 : 0)
                                        : (u32)(type - c_m3Type_i32);

        op = c_intSelectOps[type_সূচক][op_সূচক];
    } অন্যথা যদি (not Is_স্তূপ_Polymorphic(o)) {
        _throw(m3_Err_function_স্তূপ_Underrun);
    }

    EmitOp(o, op);
    জন্য (u32 i = 0; i < 3; i++) {
        যদি (IsValidSlot(slots[i])) {
            EmitSlotOffset(o, slots[i]);
        }
    }
_   (ঠেলো_Register(o, type));

    _catch: ফেরত ফলাফল;
}

স্থির
M3_ফলাফল Compile_Drop (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_ফলাফল ফলাফল = বের_করো(o);
    যদি (d_m3_Log_Wasm_স্তূপ) {
        dump_type_স্তূপ(o);
    }
    ফেরত ফলাফল;
}

স্থির
M3_ফলাফল Compile_Nop (IM3Compilation o, m3opcode_t i_opcode)
{
    ফেরত m3Err_none;
}

স্থির
M3_ফলাফল Compile_Unreachable (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_ফলাফল ফলাফল;

_   (AddTrapRecord(o));

_   (EmitOp(o, op_Unreachable));
_   (Set_স্তূপ_Polymorphic(o));

    _catch:
    ফেরত ফলাফল;
}


// OPTZ: currently all stack slot indices take up a full word, but
// dual stack source operands could be packed together
স্থির
M3_ফলাফল Compile_Operator (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_ফলাফল ফলাফল;

    // Declared before the first throw below, which jumps past this point to
    // _catch and so must not skip an initialization.
    IM3Operation op = ফাঁকা;

    IM3OpInfo opInfo = GetOpInfo(i_opcode);
    _throwif(m3Err_unknownOpcode, not opInfo);

    // Spec: validate operand types for load/store operations
    যদি (not Is_স্তূপ_Polymorphic(o)) {
        // For load ops (stackOffset == 0, unary), the operand is always i32 (address)
        যদি (i_opcode >= 0x28 and i_opcode <= 0x35) {
            m3type_t শীর্ষ_Type = Get_স্তূপ_শীর্ষ_Type(o);
            _throwif(m3Err_typeMismatch, শীর্ষ_Type != c_m3Type_none and শীর্ষ_Type != c_m3Type_i32);
        }

        // For store ops (stackOffset == -2), the address operand must be i32
        যদি (i_opcode >= 0x36 and i_opcode <= 0x3e) {
            m3type_t addrType = Get_স্তূপ_Type_From_শীর্ষ(o, 1);
            _throwif(m3Err_typeMismatch, addrType != c_m3Type_none and addrType != c_m3Type_i32);
        }
    }

    // This preserve is for for FP compare operations.
    // either need additional slot destination operations or the
    // easy fix, move _r0 out of the way.
    // moving out the way might be the optimal solution most often?
    // otherwise, the _r0 reg can get buried down in the stack
    // and be idle & wasted for a moment.
    যদি (M3_UNLIKELY(IsFpType(Get_স্তূপ_শীর্ষ_Type(o)) and IsIntType(opInfo->type))) {
_       (PreserveRegisterIfOccupied(o, opInfo->type));
    }

    যদি (opInfo->স্তূপ_Offset == 0) {
        যদি (Is_স্তূপ_শীর্ষ_In_Register(o)) {
            op = opInfo->operations[0]; // _s
        } অন্যথা {
_           (PreserveRegisterIfOccupied(o, opInfo->type));
            op = opInfo->operations[1]; // _r
        }
    } অন্যথা {
        যদি (Is_স্তূপ_শীর্ষ_In_Register(o)) {
            op = opInfo->operations[0];  // _rs

            যদি (Is_স্তূপ_শীর্ষ_Minus1_In_Register(o)) {    d_m3Assert (i_opcode == c_waOp_store_f32 or i_opcode == c_waOp_store_f64);
                op = opInfo->operations[3]; // _rr for fp.store
            }
        } অন্যথা যদি (Is_স্তূপ_শীর্ষ_Minus1_In_Register(o)) {
            op = opInfo->operations[1]; // _sr

            যদি (not op) {  // must be commutative, then
                op = opInfo->operations[0];
            }
        } অন্যথা {
_           (PreserveRegisterIfOccupied(o, opInfo->type));     // _ss
            op = opInfo->operations[2];
        }
    }

    যদি (M3_LIKELY(op)) {
_       (EmitOp(o, op));

#যদি d_m3FoldSetLocal
        // the op word just written (EmitOp may have bridged pages); NULL in the page-less compile-walk mode
        pc_t patchPC = o->page ? GetPC(o) - 1 : ফাঁকা;
#endif

_       (Emit_Slot_অঙ্ক_Of_স্তূপ_শীর্ষ_And_বের_করো(o));

        যদি (opInfo->স্তূপ_Offset < 0) {
_           (Emit_Slot_অঙ্ক_Of_স্তূপ_শীর্ষ_And_বের_করো(o));
        }

        যদি (opInfo->type != c_m3Type_none) {
_           (ঠেলো_Register(o, opInfo->type));

#যদি d_m3FoldSetLocal
            // record this op as a fold candidate for an immediately following local.set;
            // the operand-form index is recovered by matching against the variant table
            যদি (patchPC) {
                জন্য (u8 form = 0; form < 4; ++form) {
                    যদি (opInfo->operations[form] == op) {
                        o->foldPatchPC    = patchPC;
                        o->foldOpcode     = i_opcode;
                        o->foldForm       = form;
                        o->fold_স্তূপ_সূচক = (u16)Get_স্তূপ_শীর্ষ_সূচক(o);
                        বিরতি;
                    }
                }
            }
#endif
        }
    } অন্যথা {
#ifdef DEBUG
        ফলাফল = ErrorCompile("no operation found for opcode", o, "'%s'", opInfo->name);
#অন্যথা
        ফলাফল = ErrorCompile("no operation found for opcode", o, "%x", i_opcode);
#endif
        _throw(ফলাফল);
    }

    _catch: ফেরত ফলাফল;
}

স্থির
M3_ফলাফল Compile_Convert (IM3Compilation o, m3opcode_t i_opcode)
{
_try {
    IM3OpInfo opInfo = GetOpInfo(i_opcode);
    _throwif(m3Err_unknownOpcode, not opInfo);

    // Spec: validate source operand type for conversion instructions
    যদি (not Is_স্তূপ_Polymorphic(o)) {
        u8 sourceType = c_m3Type_none;
        নির্বাচন (i_opcode)
        // clang-format off
        {
        ক্ষেত্র 0xa7:              // i32.wrap/i64
            sourceType = c_m3Type_i64; বিরতি;
        ক্ষেত্র 0xa8: ক্ষেত্র 0xa9:   // i32.trunc_s/f32, i32.trunc_u/f32
        ক্ষেত্র 0xae: ক্ষেত্র 0xaf:   // i64.trunc_s/f32, i64.trunc_u/f32
        ক্ষেত্র 0xbb:              // f64.promote/f32
        ক্ষেত্র 0xbc:              // i32.reinterpret/f32
            sourceType = c_m3Type_f32; বিরতি;
        ক্ষেত্র 0xaa: ক্ষেত্র 0xab:   // i32.trunc_s/f64, i32.trunc_u/f64
        ক্ষেত্র 0xb0: ক্ষেত্র 0xb1:   // i64.trunc_s/f64, i64.trunc_u/f64
        ক্ষেত্র 0xb6:              // f32.demote/f64
        ক্ষেত্র 0xbd:              // i64.reinterpret/f64
            sourceType = c_m3Type_f64; বিরতি;
        ক্ষেত্র 0xac: ক্ষেত্র 0xad:   // i64.extend_s/i32, i64.extend_u/i32
        ক্ষেত্র 0xb2: ক্ষেত্র 0xb3:   // f32.convert_s/i32, f32.convert_u/i32
        ক্ষেত্র 0xb7: ক্ষেত্র 0xb8:   // f64.convert_s/i32, f64.convert_u/i32
        ক্ষেত্র 0xbe:              // f32.reinterpret/i32
            sourceType = c_m3Type_i32; বিরতি;
        ক্ষেত্র 0xb4: ক্ষেত্র 0xb5:   // f32.convert_s/i64, f32.convert_u/i64
        ক্ষেত্র 0xb9: ক্ষেত্র 0xba:   // f64.convert_s/i64, f64.convert_u/i64
        ক্ষেত্র 0xbf:              // f64.reinterpret/i64
            sourceType = c_m3Type_i64; বিরতি;
        পূর্বনির্ধারিত: বিরতি;
        }
        // clang-format on

        যদি (sourceType != c_m3Type_none) {
            m3type_t শীর্ষ_Type = Get_স্তূপ_শীর্ষ_Type(o);
            _throwif(m3Err_typeMismatch, শীর্ষ_Type != c_m3Type_none and not IsSubTypeOf(শীর্ষ_Type, sourceType));
        }
    }

    bool destInSlot   = IsRegisterTypeAllocated(o, opInfo->type);
    bool sourceInSlot = Is_স্তূপ_শীর্ষ_In_Slot(o);

    IM3Operation op = opInfo->operations[destInSlot * 2 + sourceInSlot];

_   (EmitOp(o, op));
_   (Emit_Slot_অঙ্ক_Of_স্তূপ_শীর্ষ_And_বের_করো(o));

    যদি (destInSlot) {
_       (ঠেলো_Allocated_Slot_And_Emit_2(o, opInfo->type))
    } অন্যথা {
_       (ঠেলো_Register(o, opInfo->type))
    }
}
    _catch: ফেরত ফলাফল;
}

#যদি d_m3HasWideArithmetic

// The wide arithmetic proposal: i64.add128 and i64.sub128 over two 128-bit values
// given as i64 halves, and i64.mul_wide_s/u giving the full product of two i64s.
// Each gives back a 128-bit result the same way, low half first. The operands are
// all read from slots, and of the two results the high half, which ends up on top,
// goes to _r0 and the low half to a newly allocated slot - so every operation has
// the one form, whatever shape the stack was in.
স্থির
M3_ফলাফল Compile_WideArithmetic (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_ফলাফল ফলাফল = m3Err_none;

    IM3Operation op          = op_i64_Add128;
    u32          অঙ্ক_Operands = 4;

    নির্বাচন (i_opcode)
    // clang-format off
    {
    ক্ষেত্র c_waOp_i64Sub128:   op = op_i64_Sub128;                          বিরতি;
    ক্ষেত্র c_waOp_i64MulWideS: op = op_i64_MultiplyWide; অঙ্ক_Operands = 2;   বিরতি;
    ক্ষেত্র c_waOp_i64MulWideU: op = op_u64_MultiplyWide; অঙ্ক_Operands = 2;   বিরতি;
    পূর্বনির্ধারিত: বিরতি;
    }
    // clang-format on

_   (PreserveRegisterIfOccupied(o, c_m3Type_i64));

_   (EmitOp(o, op));

    // the top of the stack first: the high half of the second operand, for the
    // 128-bit ones
    জন্য (u32 i = 0; i < অঙ্ক_Operands; ++i) {
_       (Emit_Slot_অঙ্ক_Of_স্তূপ_শীর্ষ_And_বের_করো(o));
    }

_   (ঠেলো_Allocated_Slot_And_Emit_2(o, c_m3Type_i64));      // the low half
_   (ঠেলো_Register(o, c_m3Type_i64));                  // the high half

    _catch: ফেরত ফলাফল;
}

#endif // d_m3HasWideArithmetic

#যদি d_m3HasMemory64

// Replaces the 64-bit address operand of a load or store with the checked
// 32-bit effective address op_CheckAddr64 leaves behind, so that the access
// itself compiles exactly as it would against a 32-bit memory: one slot to read
// the address from, and an offset of zero folded in already.
//
// Where the address sits under other operands - the value of a store, the
// operands of an atomic - it is rewritten where it stands rather than popped:
// its stack entry is repointed at the scratch slot and retyped, and the slot it
// used to occupy released. i_numOperands counts the address and all above it.
স্থির
M3_ফলাফল EmitCheckAddr64 (IM3Compilation o, u64 i_offset, u16 i_অঙ্ক_Operands)
{
    M3_ফলাফল ফলাফল = m3Err_none;

    // The address has to be in a slot for op_CheckAddr64 to read it. Spilling
    // r0 wholesale also keeps the value of a store out of it, which is what
    // leaves the address as the one operand still in a register to worry about.
_   (PreserveRegisterIfOccupied(o, c_m3Type_i64));

    // unreachable code: the operands the instruction names may not be there
    যদি (o->স্তূপ_সূচক < o->block.block_স্তূপ_সূচক + i_অঙ্ক_Operands) {
        ফেরত ফলাফল;
    }

    {
        u16      স্তূপ_সূচক = (u16)(o->স্তূপ_সূচক - i_অঙ্ক_Operands);
        u16      srcSlot    = o->wasm_স্তূপ[স্তূপ_সূচক];
        m3type_t srcType    = o->type_স্তূপ[স্তূপ_সূচক];

        u16 dstSlot = c_slotUnused;

        // Checked here rather than left to the validator, which a build may
        // have switched off: reading a slot as an i64 that only holds an i32
        // would reach past what was allocated for it.
        _throwif(m3Err_typeMismatch, BaseTypeOf(srcType) != c_m3Type_i64);

        // an address is an integer, so nothing should be left in a register
        // once r0 has been preserved
        _throwif(m3Err_typeMismatch, IsRegisterSlotAlias(srcSlot));
        _throwif(m3_Err_function_স্তূপ_Underrun, srcSlot >= o->slot_Max_Allocated_সূচক_Plus_One);

        // allocated before the source is released, so the two cannot be the
        // same slot - op_CheckAddr64 reads one and writes the other
_       (AllocateSlots(o, &dstSlot, c_m3Type_i32));

_       (EmitOp(o, op_CheckAddr64));
        EmitSlotOffset(o, srcSlot);
        EmitSlotOffset(o, dstSlot);
        EmitConstant64(o, i_offset);

        o->wasm_স্তূপ[স্তূপ_সূচক] = dstSlot;
        o->type_স্তূপ[স্তূপ_সূচক] = c_m3Type_i32;

        // locals and the constant table outlive the instruction; a dynamic
        // slot the address was computed into does not
        যদি (srcSlot >= o->slot_First_Dynamic_সূচক) {
            DeallocateSlot(o, srcSlot, srcType);
        }
    }

    _catch: ফেরত ফলাফল;
}

#endif // d_m3HasMemory64


স্থির
M3_ফলাফল Compile_Load_Store (IM3Compilation o, m3opcode_t i_opcode)
{
_try {
    u32 alignHint, memoryIdx;
    u64 memoryOffset;

    // alignHint is checked by the validator
_   (ReadMemoryArg(&alignHint, &memoryIdx, &memoryOffset, &o->wasm, o->wasmEnd));
                                                                        m3log (compile, d_indent " (memory = %d; offset = %llu)", get_indention_string (o), memoryIdx, (unsigned long long) memoryOffset);
    _throwif(m3Err_unknownMemory, memoryIdx >= o->module->অঙ্ক_Memories);

    IM3OpInfo opInfo = GetOpInfo(i_opcode);
    _throwif(m3Err_unknownOpcode, not opInfo);

    bool isMemory64 = o->module->memories[memoryIdx]->isMemory64;

    // Spec: the static offset has to be in range of the address type. Checked
    // here as well as in the validator, so that a build without one still
    // refuses the module rather than silently truncating the offset.
    _throwif(m3Err_wasmMalformed, not isMemory64 and memoryOffset > 0xFFFFFFFFull);

    যদি (IsFpType(opInfo->type)) {
_       (PreserveRegisterIfOccupied(o, c_m3Type_f64));
    }

    যদি (memoryIdx) {
_       (EmitSetMemory(o, memoryIdx));
    }

#যদি d_m3HasMemory64
    যদি (isMemory64) {
        // An offset this far out puts every address out of bounds, so rather
        // than carrying it, clamp it to the limit: op_CheckAddr64 then traps
        // on whatever address it is given, which is the outcome either way.
        যদি (memoryOffset > d_m3AddressLimit) {
            memoryOffset = d_m3AddressLimit;
        }

_       (EmitCheckAddr64(o, memoryOffset, opInfo->স্তূপ_Offset < 0 ? 2 : 1));

        // op_CheckAddr64 has folded the offset in already
        memoryOffset = 0;
    }
#endif

_   (Compile_Operator(o, i_opcode));

    EmitConstant32(o, (u32)memoryOffset);

    যদি (memoryIdx) {
_       (EmitSetMemory(o, 0));
    }
}
    _catch: ফেরত ফলাফল;
}


#যদি d_m3HasAtomics

// What an atomic instruction takes and gives back, from its sub-opcode. The
// accesses come in runs of seven, one per combination of value type and cell width:
// i32/32, i64/64, i32/8, i32/16, i64/8, i64/16, i64/32. Those runs are told apart by
// where they start.
//
// The operand count includes the address. The top operand is the one that ends up
// in a register, so its type is the one to check on the way out; a store has no
// result, which is c_m3Type_none.
স্থির
শূন্য GetAtomicShape (u32 i_sub, u16* o_অঙ্ক_Operands, m3type_t* o_শীর্ষ_Type, m3type_t* o_ফলাফল_Type)
{
    // clang-format off
    স্থির ধ্রুবক m3type_t c_মান_Types [7] = {
        c_m3Type_i32, c_m3Type_i64,
        c_m3Type_i32, c_m3Type_i32,
        c_m3Type_i64, c_m3Type_i64, c_m3Type_i64
    };
    // clang-format on

    যদি (i_sub == 0x00) {                    // memory.atomic.notify: address, count
        *o_অঙ্ক_Operands = 2;
        *o_শীর্ষ_Type     = c_m3Type_i32;
        *o_ফলাফল_Type  = c_m3Type_i32;
    } অন্যথা যদি (i_sub <= 0x02) {             // memory.atomic.wait32/64: address, expected, timeout
        *o_অঙ্ক_Operands = 3;
        *o_শীর্ষ_Type     = c_m3Type_i64;
        *o_ফলাফল_Type  = c_m3Type_i32;
    } অন্যথা যদি (i_sub <= 0x16) {             // loads: address
        *o_অঙ্ক_Operands = 1;
        *o_শীর্ষ_Type     = c_m3Type_i32;
        *o_ফলাফল_Type  = c_মান_Types[i_sub - 0x10];
    } অন্যথা যদি (i_sub <= 0x1d) {             // stores: address, value
        *o_অঙ্ক_Operands = 2;
        *o_শীর্ষ_Type     = c_মান_Types[i_sub - 0x17];
        *o_ফলাফল_Type  = c_m3Type_none;
    } অন্যথা যদি (i_sub <= 0x47) {             // read-modify-write: address, value
        *o_অঙ্ক_Operands = 2;
        *o_শীর্ষ_Type     = c_মান_Types[(i_sub - 0x1e) % 7];
        *o_ফলাফল_Type  = *o_শীর্ষ_Type;
    } অন্যথা {                                // cmpxchg: address, expected, replacement
        *o_অঙ্ক_Operands = 3;
        *o_শীর্ষ_Type     = c_মান_Types[i_sub - 0x48];
        *o_ফলাফল_Type  = *o_শীর্ষ_Type;
    }
}


// The description an access takes as its last immediate: see m3_exec_atomic.h
স্থির
u32 GetAtomicDesc (u32 i_sub)
{
    // per variant, the log2 of the width of the cell
    স্থির ধ্রুবক u8 c_log2Widths[7] = { 2, 3, 0, 1, 0, 1, 2 };

    u32 variant, kind = 0;

    যদি (i_sub <= 0x16) {
        variant = i_sub - 0x10;
    } অন্যথা যদি (i_sub <= 0x1d) {
        variant = i_sub - 0x17;
    } অন্যথা যদি (i_sub <= 0x47) {
        variant = (i_sub - 0x1e) % 7;
        kind    = (i_sub - 0x1e) / 7;         // add, sub, and, or, xor, xchg
    } অন্যথা {
        variant = i_sub - 0x48;
    }

    // variants 1 and 4..6 are the i64 ones
    u32 is64 = (variant == 1 or variant >= 4) ? 1u : 0u;

    ফেরত c_log2Widths[variant] | (is64 << 2) | (kind << 3);
}


// Every atomic access is one op, whose operands are the topmost in _r0 and the rest
// in slots, topmost first. The address is always the deepest, so it is the last slot
// (or, for a load, the register), followed by the memarg offset. The ops check the
// address themselves: for an atomic that includes its alignment, which only the
// effective address can be asked.
স্থির
M3_ফলাফল Compile_Atomic (IM3Compilation o, m3opcode_t i_opcode)
{
_try {
    u32 alignHint, memoryIdx;
    u64 memoryOffset;

    u16      অঙ্ক_Operands = 0;
    m3type_t শীর্ষ_Type     = c_m3Type_none;
    m3type_t ফলাফল_Type  = c_m3Type_none;

    যদি (i_opcode == c_waOp_atomicFence) {
        // the reserved byte, which the validator has checked to be zero. There
        // is nothing to order between accesses that one thread makes in sequence.
        u8 reserved;
_       (Read_u8(&reserved, &o->wasm, o->wasmEnd));
    } অন্যথা {
        // alignHint is checked by the validator
_       (ReadMemoryArg(&alignHint, &memoryIdx, &memoryOffset, &o->wasm, o->wasmEnd));
                                                                        m3log (compile, d_indent " (memory = %d; offset = %llu)", get_indention_string (o), memoryIdx, (unsigned long long) memoryOffset);
        _throwif(m3Err_unknownMemory, memoryIdx >= o->module->অঙ্ক_Memories);

        IM3OpInfo opInfo = GetOpInfo(i_opcode);
        _throwif(m3Err_unknownOpcode, not opInfo);

        bool isMemory64 = o->module->memories[memoryIdx]->isMemory64;

        // see Compile_Load_Store
        _throwif(m3Err_wasmMalformed, not isMemory64 and memoryOffset > 0xFFFFFFFFull);

        GetAtomicShape(i_opcode & 0xFF, &অঙ্ক_Operands, &শীর্ষ_Type, &ফলাফল_Type);

        যদি (memoryIdx) {
_           (EmitSetMemory(o, memoryIdx));
        }

#  যদি d_m3HasMemory64
        যদি (isMemory64) {
            // see Compile_Load_Store
            যদি (memoryOffset > d_m3AddressLimit) {
                memoryOffset = d_m3AddressLimit;
            }

_           (EmitCheckAddr64(o, memoryOffset, অঙ্ক_Operands));
            memoryOffset = 0;
        }
#  endif

_       (Copy_স্তূপ_শীর্ষ_To_Register(o, false));
_       (EmitOp(o, opInfo->operations[0]));
_       (বের_করো_Type(o, শীর্ষ_Type));

        জন্য (u16 i = 1; i < অঙ্ক_Operands; ++i) {
_           (Emit_Slot_অঙ্ক_Of_স্তূপ_শীর্ষ_And_বের_করো(o));
        }

        EmitConstant32(o, (u32)memoryOffset);
        যদি (i_opcode >= c_waOp_atomicFirstAccess) {
            EmitConstant32(o, GetAtomicDesc(i_opcode & 0xFF));
        }

        যদি (memoryIdx) {
_           (EmitSetMemory(o, 0));
        }

        যদি (ফলাফল_Type != c_m3Type_none) {
_           (ঠেলো_Register(o, ফলাফল_Type));
        }
    }
}
    _catch: ফেরত ফলাফল;
}

#endif // d_m3HasAtomics


M3_ফলাফল CompileRawFunction (IM3Module io_module, IM3Function io_function, ধ্রুবক শূন্য* i_function, ধ্রুবক শূন্য* i_userdata)
{
    d_m3Assert(io_module->runtime);

    IM3CodePage page = AcquireCodePageWithCapacity(io_module->runtime, 4);

    যদি (page) {
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
        ফেরত m3Err_none;
    } অন্যথা {
        ফেরত m3Err_mallocFailedCodePage;
    }
}


// A table entry names every field it sets, so the struct can order them for
// packing; the columns follow that order, since M3OP names them positionally.
//
// d_logOp, d_logOp2 macros aren't actually used by the compiler, just codepage decoding (d_m3LogCodePages = 1)
#define d_ops(...)                          .operations = { __VA_ARGS__ }

#define d_logOp(OP)                         d_ops( op_##OP,                 ফাঁকা,                       ফাঁকা,                       ফাঁকা )
#define d_logOp2(OP1,OP2)                   d_ops( op_##OP1,                op_##OP2,                   ফাঁকা,                       ফাঁকা )

#define d_emptyOpList                       d_ops( ফাঁকা,                    ফাঁকা,                       ফাঁকা,                       ফাঁকা )
#define d_unaryOpList(TYPE, NAME)           d_ops( op_##TYPE##_##NAME##_r,  op_##TYPE##_##NAME##_s,     ফাঁকা,                       ফাঁকা )
#define d_binOpList(TYPE, NAME)             d_ops( op_##TYPE##_##NAME##_rs, op_##TYPE##_##NAME##_sr,    op_##TYPE##_##NAME##_ss,    ফাঁকা )
#define d_storeFpOpList(TYPE, NAME)         d_ops( op_##TYPE##_##NAME##_rs, op_##TYPE##_##NAME##_sr,    op_##TYPE##_##NAME##_ss,    op_##TYPE##_##NAME##_rr )
#define d_commutativeBinOpList(TYPE, NAME)  d_ops( op_##TYPE##_##NAME##_rs, ফাঁকা,                       op_##TYPE##_##NAME##_ss,    ফাঁকা )

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
    _( Compile_Memory_আকার )            \
    _( Compile_Memory_Grow )            \
    _( Compile_Memory_Init )            \
    _( Compile_Memory_CopyFill )        \
    _( Compile_তথ্য_Drop )              \
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
    d_m3_Compiler_List_স্তূপ_Switching( _ )

#যদি d_m3ImplementFloat
#  define d_m3CompilerList_f(_)         \
    _( Compile_Const_f32 )              \
    _( Compile_Const_f64 )
#অন্যথা
#  define d_m3CompilerList_f(_)
#endif

#যদি d_m3HasRefTypes
#  define d_m3CompilerList_refTypes(_)  \
    _( Compile_Select_Typed )           \
    _( Compile_Ref_Null )               \
    _( Compile_Ref_IsNull )             \
    _( Compile_Ref_Func )               \
    _( Compile_Table_GetSet )           \
    _( Compile_Table_Init )             \
    _( Compile_Table_Copy )             \
    _( Compile_Table_আকার )             \
    _( Compile_Table_GrowFill )         \
    _( Compile_Elem_Drop )
#অন্যথা
#  define d_m3CompilerList_refTypes(_)
#endif

#যদি d_m3HasTypedRefs
#  define d_m3CompilerList_typedRefs(_) \
    _( Compile_CallRef )                \
    _( Compile_Ref_AsNonNull )
#অন্যথা
#  define d_m3CompilerList_typedRefs(_)
#endif

#যদি d_m3HasExceptionHandling
#  define d_m3CompilerList_eh(_)        \
    _( Compile_Throw )                  \
    _( Compile_ThrowRef )               \
    _( Compile_TryTable )
#অন্যথা
#  define d_m3CompilerList_eh(_)
#endif

#যদি d_m3HasAtomics
#  define d_m3CompilerList_atomics(_)   \
    _( Compile_Atomic )
#অন্যথা
#  define d_m3CompilerList_atomics(_)
#endif

#যদি d_m3HasWideArithmetic
#  define d_m3CompilerList_wide(_)      \
    _( Compile_WideArithmetic )
#অন্যথা
#  define d_m3CompilerList_wide(_)
#endif

#যদি d_m3_Has_স্তূপ_Switching
#  define d_m3_Compiler_List_স্তূপ_Switching(_) \
    _( Compile_ContNew )                     \
    _( Compile_ContBind )                    \
    _( Compile_Suspend )                     \
    _( Compile_Resume )                      \
    _( Compile_Switch )
#অন্যথা
#  define d_m3_Compiler_List_স্তূপ_Switching(_)
#endif

// Index 0 is "this opcode has no compiler", so that a zeroed entry - which is
// what M3OP_RESERVED and the reserved holes in the tables are - reads as one.
গণনা {
    c_cc_NULL = 0,
#define d_m3CompilerEnum(NAME)  c_cc_##NAME,
    d_m3CompilerList(d_m3CompilerEnum)
#undef d_m3CompilerEnum
};

স্থির ধ্রুবক M3Compiler c_compilers[] = {
    ফাঁকা,
#define d_m3CompilerPtr(NAME)   NAME,
    d_m3CompilerList(d_m3CompilerPtr)
#undef d_m3CompilerPtr
};

// Names a table entry's compiler by index. d_cc (NULL) is the no-compiler entry.
#define d_cc(NAME)                          .compiler = c_cc_##NAME

স্থির
M3Compiler GetCompiler (IM3OpInfo i_opInfo)
{
    d_m3Assert(i_opInfo->compiler < M3_গুনতি_OF(c_compilers));
    ফেরত c_compilers[i_opInfo->compiler];
}

// clang-format off
ধ্রুবক M3OpInfo c_operations[] =
{
    M3OP( "unreachable",           0,  none,   d_cc(Compile_Unreachable),  d_logOp (Unreachable) ),                   // 0x00
    M3OP( "nop",                   0,  none,   d_cc(Compile_Nop),          d_emptyOpList ),                           // 0x01 .
    M3OP( "block",                 0,  none,   d_cc(Compile_LoopOrBlock),  d_emptyOpList ),                           // 0x02
    M3OP( "loop",                  0,  none,   d_cc(Compile_LoopOrBlock),  d_logOp (Loop) ),                          // 0x03
    M3OP( "if",                    -1, none,   d_cc(Compile_If),           d_emptyOpList ),                           // 0x04
    M3OP( "else",                  0,  none,   d_cc(Compile_Nop),          d_emptyOpList ),                           // 0x05

    M3OP_RESERVED,  M3OP_RESERVED,                                                                                    // 0x06...0x07

#যদি d_m3HasExceptionHandling
    M3OP( "throw",                 0,  none,   d_cc(Compile_Throw),        d_logOp (Throw) ),                         // 0x08
    M3OP_RESERVED,                                                                                                    // 0x09
    M3OP( "throw_ref",             0,  none,   d_cc(Compile_ThrowRef),     d_logOp (ThrowRef) ),                      // 0x0a
#অন্যথা
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

#যদি d_m3HasTypedRefs
    M3OP( "call_ref",              0,  any,    d_cc(Compile_CallRef),      d_logOp (CallRef) ),                       // 0x14
    M3OP( "return_call_ref",       0,  any,    d_cc(Compile_CallRef),      d_logOp (ReturnCallRef) ),                 // 0x15
#অন্যথা
    M3OP_RESERVED,  M3OP_RESERVED,                                                                                    // 0x14...
#endif
    M3OP_RESERVED,  M3OP_RESERVED, M3OP_RESERVED, M3OP_RESERVED,                                                      // ...0x19

    M3OP( "drop",                  -1, none,   d_cc(Compile_Drop),         d_emptyOpList ),                           // 0x1a
    M3OP( "select",                -2, any,    d_cc(Compile_Select),       d_emptyOpList ),                           // 0x1b

#যদি d_m3HasRefTypes
    M3OP( "select.t",              -2, any,    d_cc(Compile_Select_Typed), d_emptyOpList ),                           // 0x1c
#অন্যথা
    M3OP_RESERVED,                                                                                                    // 0x1c
#endif
    M3OP_RESERVED,  M3OP_RESERVED,                                                                                    // 0x1d...0x1e

#যদি d_m3HasExceptionHandling
    M3OP( "try_table",             0,  none,   d_cc(Compile_TryTable),     d_logOp (TryTable) ),                      // 0x1f
#অন্যথা
    M3OP_RESERVED,                                                                                                    // 0x1f
#endif

    M3OP( "local.get",             1,  any,    d_cc(Compile_GetLocal),     d_emptyOpList ),                           // 0x20
    M3OP( "local.set",             1,  none,   d_cc(Compile_SetLocal),     d_emptyOpList ),                           // 0x21
    M3OP( "local.tee",             0,  any,    d_cc(Compile_SetLocal),     d_emptyOpList ),                           // 0x22
    M3OP( "global.get",            1,  none,   d_cc(Compile_GetSetGlobal), d_emptyOpList ),                           // 0x23
    M3OP( "global.set",            1,  none,   d_cc(Compile_GetSetGlobal), d_emptyOpList ),                           // 0x24

#যদি d_m3HasRefTypes
    M3OP( "table.get",             0,  any,    d_cc(Compile_Table_GetSet), d_emptyOpList ),                           // 0x25
    M3OP( "table.set",             -2, none,   d_cc(Compile_Table_GetSet), d_emptyOpList ),                           // 0x26
#অন্যথা
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

    M3OP( "memory.size",           1,  i_32,   d_cc(Compile_Memory_আকার),  d_logOp (Mem_আকার) ),                       // 0x3f
    M3OP( "memory.grow",           1,  i_32,   d_cc(Compile_Memory_Grow),  d_logOp (MemGrow) ),                       // 0x40

    M3OP( "i32.const",             1,  i_32,   d_cc(Compile_Const_i32),    d_logOp (Const32) ),                       // 0x41
    M3OP( "i64.const",             1,  i_64,   d_cc(Compile_Const_i64),    d_logOp (Const64) ),                       // 0x42
    M3OP_F( "f32.const",           1,  f_32,   d_cc(Compile_Const_f32),    d_emptyOpList ),                           // 0x43
    M3OP_F( "f64.const",           1,  f_64,   d_cc(Compile_Const_f64),    d_emptyOpList ),                           // 0x44

    M3OP( "i32.eqz",               0,  i_32,   d_cc(ফাঁকা),                 d_unaryOpList (i32, EqualToZero) ),        // 0x45
    M3OP( "i32.eq",                -1, i_32,   d_cc(ফাঁকা),                 d_commutativeBinOpList (i32, Equal) ),     // 0x46
    M3OP( "i32.ne",                -1, i_32,   d_cc(ফাঁকা),                 d_commutativeBinOpList (i32, NotEqual) ),  // 0x47
    M3OP( "i32.lt_s",              -1, i_32,   d_cc(ফাঁকা),                 d_binOpList (i32, LessThan) ),             // 0x48
    M3OP( "i32.lt_u",              -1, i_32,   d_cc(ফাঁকা),                 d_binOpList (u32, LessThan) ),             // 0x49
    M3OP( "i32.gt_s",              -1, i_32,   d_cc(ফাঁকা),                 d_binOpList (i32, GreaterThan) ),          // 0x4a
    M3OP( "i32.gt_u",              -1, i_32,   d_cc(ফাঁকা),                 d_binOpList (u32, GreaterThan) ),          // 0x4b
    M3OP( "i32.le_s",              -1, i_32,   d_cc(ফাঁকা),                 d_binOpList (i32, LessThanOrEqual) ),      // 0x4c
    M3OP( "i32.le_u",              -1, i_32,   d_cc(ফাঁকা),                 d_binOpList (u32, LessThanOrEqual) ),      // 0x4d
    M3OP( "i32.ge_s",              -1, i_32,   d_cc(ফাঁকা),                 d_binOpList (i32, GreaterThanOrEqual) ),   // 0x4e
    M3OP( "i32.ge_u",              -1, i_32,   d_cc(ফাঁকা),                 d_binOpList (u32, GreaterThanOrEqual) ),   // 0x4f

    M3OP( "i64.eqz",               0,  i_32,   d_cc(ফাঁকা),                 d_unaryOpList (i64, EqualToZero) ),        // 0x50
    M3OP( "i64.eq",                -1, i_32,   d_cc(ফাঁকা),                 d_commutativeBinOpList (i64, Equal) ),     // 0x51
    M3OP( "i64.ne",                -1, i_32,   d_cc(ফাঁকা),                 d_commutativeBinOpList (i64, NotEqual) ),  // 0x52
    M3OP( "i64.lt_s",              -1, i_32,   d_cc(ফাঁকা),                 d_binOpList (i64, LessThan) ),             // 0x53
    M3OP( "i64.lt_u",              -1, i_32,   d_cc(ফাঁকা),                 d_binOpList (u64, LessThan) ),             // 0x54
    M3OP( "i64.gt_s",              -1, i_32,   d_cc(ফাঁকা),                 d_binOpList (i64, GreaterThan) ),          // 0x55
    M3OP( "i64.gt_u",              -1, i_32,   d_cc(ফাঁকা),                 d_binOpList (u64, GreaterThan) ),          // 0x56
    M3OP( "i64.le_s",              -1, i_32,   d_cc(ফাঁকা),                 d_binOpList (i64, LessThanOrEqual) ),      // 0x57
    M3OP( "i64.le_u",              -1, i_32,   d_cc(ফাঁকা),                 d_binOpList (u64, LessThanOrEqual) ),      // 0x58
    M3OP( "i64.ge_s",              -1, i_32,   d_cc(ফাঁকা),                 d_binOpList (i64, GreaterThanOrEqual) ),   // 0x59
    M3OP( "i64.ge_u",              -1, i_32,   d_cc(ফাঁকা),                 d_binOpList (u64, GreaterThanOrEqual) ),   // 0x5a

    M3OP_F( "f32.eq",              -1, i_32,   d_cc(ফাঁকা),                 d_commutativeBinOpList (f32, Equal) ),     // 0x5b
    M3OP_F( "f32.ne",              -1, i_32,   d_cc(ফাঁকা),                 d_commutativeBinOpList (f32, NotEqual) ),  // 0x5c
    M3OP_F( "f32.lt",              -1, i_32,   d_cc(ফাঁকা),                 d_binOpList (f32, LessThan) ),             // 0x5d
    M3OP_F( "f32.gt",              -1, i_32,   d_cc(ফাঁকা),                 d_binOpList (f32, GreaterThan) ),          // 0x5e
    M3OP_F( "f32.le",              -1, i_32,   d_cc(ফাঁকা),                 d_binOpList (f32, LessThanOrEqual) ),      // 0x5f
    M3OP_F( "f32.ge",              -1, i_32,   d_cc(ফাঁকা),                 d_binOpList (f32, GreaterThanOrEqual) ),   // 0x60

    M3OP_F( "f64.eq",              -1, i_32,   d_cc(ফাঁকা),                 d_commutativeBinOpList (f64, Equal) ),     // 0x61
    M3OP_F( "f64.ne",              -1, i_32,   d_cc(ফাঁকা),                 d_commutativeBinOpList (f64, NotEqual) ),  // 0x62
    M3OP_F( "f64.lt",              -1, i_32,   d_cc(ফাঁকা),                 d_binOpList (f64, LessThan) ),             // 0x63
    M3OP_F( "f64.gt",              -1, i_32,   d_cc(ফাঁকা),                 d_binOpList (f64, GreaterThan) ),          // 0x64
    M3OP_F( "f64.le",              -1, i_32,   d_cc(ফাঁকা),                 d_binOpList (f64, LessThanOrEqual) ),      // 0x65
    M3OP_F( "f64.ge",              -1, i_32,   d_cc(ফাঁকা),                 d_binOpList (f64, GreaterThanOrEqual) ),   // 0x66

    M3OP( "i32.clz",               0,  i_32,   d_cc(ফাঁকা),                 d_unaryOpList (u32, Clz) ),                // 0x67
    M3OP( "i32.ctz",               0,  i_32,   d_cc(ফাঁকা),                 d_unaryOpList (u32, Ctz) ),                // 0x68
    M3OP( "i32.popcnt",            0,  i_32,   d_cc(ফাঁকা),                 d_unaryOpList (u32, Popcnt) ),             // 0x69

    M3OP( "i32.add",               -1, i_32,   d_cc(ফাঁকা),                 d_commutativeBinOpList (i32, Add) ),       // 0x6a
    M3OP( "i32.sub",               -1, i_32,   d_cc(ফাঁকা),                 d_binOpList (i32, Subtract) ),             // 0x6b
    M3OP( "i32.mul",               -1, i_32,   d_cc(ফাঁকা),                 d_commutativeBinOpList (i32, Multiply) ),  // 0x6c
    M3OP( "i32.div_s",             -1, i_32,   d_cc(ফাঁকা),                 d_binOpList (i32, Divide) ),               // 0x6d
    M3OP( "i32.div_u",             -1, i_32,   d_cc(ফাঁকা),                 d_binOpList (u32, Divide) ),               // 0x6e
    M3OP( "i32.rem_s",             -1, i_32,   d_cc(ফাঁকা),                 d_binOpList (i32, Remainder) ),            // 0x6f
    M3OP( "i32.rem_u",             -1, i_32,   d_cc(ফাঁকা),                 d_binOpList (u32, Remainder) ),            // 0x70
    M3OP( "i32.and",               -1, i_32,   d_cc(ফাঁকা),                 d_commutativeBinOpList (u32, And) ),       // 0x71
    M3OP( "i32.or",                -1, i_32,   d_cc(ফাঁকা),                 d_commutativeBinOpList (u32, Or) ),        // 0x72
    M3OP( "i32.xor",               -1, i_32,   d_cc(ফাঁকা),                 d_commutativeBinOpList (u32, Xor) ),       // 0x73
    M3OP( "i32.shl",               -1, i_32,   d_cc(ফাঁকা),                 d_binOpList (u32, Shift_বাম) ),            // 0x74
    M3OP( "i32.shr_s",             -1, i_32,   d_cc(ফাঁকা),                 d_binOpList (i32, Shift_ডান) ),           // 0x75
    M3OP( "i32.shr_u",             -1, i_32,   d_cc(ফাঁকা),                 d_binOpList (u32, Shift_ডান) ),           // 0x76
    M3OP( "i32.rotl",              -1, i_32,   d_cc(ফাঁকা),                 d_binOpList (u32, Rotl) ),                 // 0x77
    M3OP( "i32.rotr",              -1, i_32,   d_cc(ফাঁকা),                 d_binOpList (u32, Rotr) ),                 // 0x78

    M3OP( "i64.clz",               0,  i_64,   d_cc(ফাঁকা),                 d_unaryOpList (u64, Clz) ),                // 0x79
    M3OP( "i64.ctz",               0,  i_64,   d_cc(ফাঁকা),                 d_unaryOpList (u64, Ctz) ),                // 0x7a
    M3OP( "i64.popcnt",            0,  i_64,   d_cc(ফাঁকা),                 d_unaryOpList (u64, Popcnt) ),             // 0x7b

    M3OP( "i64.add",               -1, i_64,   d_cc(ফাঁকা),                 d_commutativeBinOpList (i64, Add) ),       // 0x7c
    M3OP( "i64.sub",               -1, i_64,   d_cc(ফাঁকা),                 d_binOpList (i64, Subtract) ),             // 0x7d
    M3OP( "i64.mul",               -1, i_64,   d_cc(ফাঁকা),                 d_commutativeBinOpList (i64, Multiply) ),  // 0x7e
    M3OP( "i64.div_s",             -1, i_64,   d_cc(ফাঁকা),                 d_binOpList (i64, Divide) ),               // 0x7f
    M3OP( "i64.div_u",             -1, i_64,   d_cc(ফাঁকা),                 d_binOpList (u64, Divide) ),               // 0x80
    M3OP( "i64.rem_s",             -1, i_64,   d_cc(ফাঁকা),                 d_binOpList (i64, Remainder) ),            // 0x81
    M3OP( "i64.rem_u",             -1, i_64,   d_cc(ফাঁকা),                 d_binOpList (u64, Remainder) ),            // 0x82
    M3OP( "i64.and",               -1, i_64,   d_cc(ফাঁকা),                 d_commutativeBinOpList (u64, And) ),       // 0x83
    M3OP( "i64.or",                -1, i_64,   d_cc(ফাঁকা),                 d_commutativeBinOpList (u64, Or) ),        // 0x84
    M3OP( "i64.xor",               -1, i_64,   d_cc(ফাঁকা),                 d_commutativeBinOpList (u64, Xor) ),       // 0x85
    M3OP( "i64.shl",               -1, i_64,   d_cc(ফাঁকা),                 d_binOpList (u64, Shift_বাম) ),            // 0x86
    M3OP( "i64.shr_s",             -1, i_64,   d_cc(ফাঁকা),                 d_binOpList (i64, Shift_ডান) ),           // 0x87
    M3OP( "i64.shr_u",             -1, i_64,   d_cc(ফাঁকা),                 d_binOpList (u64, Shift_ডান) ),           // 0x88
    M3OP( "i64.rotl",              -1, i_64,   d_cc(ফাঁকা),                 d_binOpList (u64, Rotl) ),                 // 0x89
    M3OP( "i64.rotr",              -1, i_64,   d_cc(ফাঁকা),                 d_binOpList (u64, Rotr) ),                 // 0x8a

    M3OP_F( "f32.abs",             0,  f_32,   d_cc(ফাঁকা),                 d_unaryOpList(f32, Abs) ),                 // 0x8b
    M3OP_F( "f32.neg",             0,  f_32,   d_cc(ফাঁকা),                 d_unaryOpList(f32, Negate) ),              // 0x8c
    M3OP_F( "f32.ceil",            0,  f_32,   d_cc(ফাঁকা),                 d_unaryOpList(f32, Ceil) ),                // 0x8d
    M3OP_F( "f32.floor",           0,  f_32,   d_cc(ফাঁকা),                 d_unaryOpList(f32, Floor) ),               // 0x8e
    M3OP_F( "f32.trunc",           0,  f_32,   d_cc(ফাঁকা),                 d_unaryOpList(f32, Trunc) ),               // 0x8f
    M3OP_F( "f32.nearest",         0,  f_32,   d_cc(ফাঁকা),                 d_unaryOpList(f32, Nearest) ),             // 0x90
    M3OP_F( "f32.sqrt",            0,  f_32,   d_cc(ফাঁকা),                 d_unaryOpList(f32, Sqrt) ),                // 0x91

    M3OP_F( "f32.add",             -1, f_32,   d_cc(ফাঁকা),                 d_commutativeBinOpList (f32, Add) ),       // 0x92
    M3OP_F( "f32.sub",             -1, f_32,   d_cc(ফাঁকা),                 d_binOpList (f32, Subtract) ),             // 0x93
    M3OP_F( "f32.mul",             -1, f_32,   d_cc(ফাঁকা),                 d_commutativeBinOpList (f32, Multiply) ),  // 0x94
    M3OP_F( "f32.div",             -1, f_32,   d_cc(ফাঁকা),                 d_binOpList (f32, Divide) ),               // 0x95
    M3OP_F( "f32.min",             -1, f_32,   d_cc(ফাঁকা),                 d_commutativeBinOpList (f32, Min) ),       // 0x96
    M3OP_F( "f32.max",             -1, f_32,   d_cc(ফাঁকা),                 d_commutativeBinOpList (f32, Max) ),       // 0x97
    M3OP_F( "f32.copysign",        -1, f_32,   d_cc(ফাঁকা),                 d_binOpList (f32, CopySign) ),             // 0x98

    M3OP_F( "f64.abs",             0,  f_64,   d_cc(ফাঁকা),                 d_unaryOpList(f64, Abs) ),                 // 0x99
    M3OP_F( "f64.neg",             0,  f_64,   d_cc(ফাঁকা),                 d_unaryOpList(f64, Negate) ),              // 0x9a
    M3OP_F( "f64.ceil",            0,  f_64,   d_cc(ফাঁকা),                 d_unaryOpList(f64, Ceil) ),                // 0x9b
    M3OP_F( "f64.floor",           0,  f_64,   d_cc(ফাঁকা),                 d_unaryOpList(f64, Floor) ),               // 0x9c
    M3OP_F( "f64.trunc",           0,  f_64,   d_cc(ফাঁকা),                 d_unaryOpList(f64, Trunc) ),               // 0x9d
    M3OP_F( "f64.nearest",         0,  f_64,   d_cc(ফাঁকা),                 d_unaryOpList(f64, Nearest) ),             // 0x9e
    M3OP_F( "f64.sqrt",            0,  f_64,   d_cc(ফাঁকা),                 d_unaryOpList(f64, Sqrt) ),                // 0x9f

    M3OP_F( "f64.add",             -1, f_64,   d_cc(ফাঁকা),                 d_commutativeBinOpList (f64, Add) ),       // 0xa0
    M3OP_F( "f64.sub",             -1, f_64,   d_cc(ফাঁকা),                 d_binOpList (f64, Subtract) ),             // 0xa1
    M3OP_F( "f64.mul",             -1, f_64,   d_cc(ফাঁকা),                 d_commutativeBinOpList (f64, Multiply) ),  // 0xa2
    M3OP_F( "f64.div",             -1, f_64,   d_cc(ফাঁকা),                 d_binOpList (f64, Divide) ),               // 0xa3
    M3OP_F( "f64.min",             -1, f_64,   d_cc(ফাঁকা),                 d_commutativeBinOpList (f64, Min) ),       // 0xa4
    M3OP_F( "f64.max",             -1, f_64,   d_cc(ফাঁকা),                 d_commutativeBinOpList (f64, Max) ),       // 0xa5
    M3OP_F( "f64.copysign",        -1, f_64,   d_cc(ফাঁকা),                 d_binOpList (f64, CopySign) ),             // 0xa6

    M3OP( "i32.wrap/i64",          0,  i_32,   d_cc(ফাঁকা),                 d_unaryOpList (i32, Wrap_i64) ),           // 0xa7
    M3OP_F( "i32.trunc_s/f32",     0,  i_32,   d_cc(Compile_Convert),      d_convertOpList (i32_Trunc_f32) ),         // 0xa8
    M3OP_F( "i32.trunc_u/f32",     0,  i_32,   d_cc(Compile_Convert),      d_convertOpList (u32_Trunc_f32) ),         // 0xa9
    M3OP_F( "i32.trunc_s/f64",     0,  i_32,   d_cc(Compile_Convert),      d_convertOpList (i32_Trunc_f64) ),         // 0xaa
    M3OP_F( "i32.trunc_u/f64",     0,  i_32,   d_cc(Compile_Convert),      d_convertOpList (u32_Trunc_f64) ),         // 0xab

    M3OP( "i64.extend_s/i32",      0,  i_64,   d_cc(ফাঁকা),                 d_unaryOpList (i64, Extend_i32) ),         // 0xac
    M3OP( "i64.extend_u/i32",      0,  i_64,   d_cc(ফাঁকা),                 d_unaryOpList (i64, Extend_u32) ),         // 0xad

    M3OP_F( "i64.trunc_s/f32",     0,  i_64,   d_cc(Compile_Convert),      d_convertOpList (i64_Trunc_f32) ),         // 0xae
    M3OP_F( "i64.trunc_u/f32",     0,  i_64,   d_cc(Compile_Convert),      d_convertOpList (u64_Trunc_f32) ),         // 0xaf
    M3OP_F( "i64.trunc_s/f64",     0,  i_64,   d_cc(Compile_Convert),      d_convertOpList (i64_Trunc_f64) ),         // 0xb0
    M3OP_F( "i64.trunc_u/f64",     0,  i_64,   d_cc(Compile_Convert),      d_convertOpList (u64_Trunc_f64) ),         // 0xb1

    M3OP_F( "f32.convert_s/i32",   0,  f_32,   d_cc(Compile_Convert),      d_convertOpList (f32_Convert_i32) ),       // 0xb2
    M3OP_F( "f32.convert_u/i32",   0,  f_32,   d_cc(Compile_Convert),      d_convertOpList (f32_Convert_u32) ),       // 0xb3
    M3OP_F( "f32.convert_s/i64",   0,  f_32,   d_cc(Compile_Convert),      d_convertOpList (f32_Convert_i64) ),       // 0xb4
    M3OP_F( "f32.convert_u/i64",   0,  f_32,   d_cc(Compile_Convert),      d_convertOpList (f32_Convert_u64) ),       // 0xb5

    M3OP_F( "f32.demote/f64",      0,  f_32,   d_cc(ফাঁকা),                 d_unaryOpList (f32, Demote_f64) ),         // 0xb6

    M3OP_F( "f64.convert_s/i32",   0,  f_64,   d_cc(Compile_Convert),      d_convertOpList (f64_Convert_i32) ),       // 0xb7
    M3OP_F( "f64.convert_u/i32",   0,  f_64,   d_cc(Compile_Convert),      d_convertOpList (f64_Convert_u32) ),       // 0xb8
    M3OP_F( "f64.convert_s/i64",   0,  f_64,   d_cc(Compile_Convert),      d_convertOpList (f64_Convert_i64) ),       // 0xb9
    M3OP_F( "f64.convert_u/i64",   0,  f_64,   d_cc(Compile_Convert),      d_convertOpList (f64_Convert_u64) ),       // 0xba

    M3OP_F( "f64.promote/f32",     0,  f_64,   d_cc(ফাঁকা),                 d_unaryOpList (f64, Promote_f32) ),        // 0xbb

    M3OP_F( "i32.reinterpret/f32", 0,  i_32,   d_cc(Compile_Convert),      d_convertOpList (i32_Reinterpret_f32) ),   // 0xbc
    M3OP_F( "i64.reinterpret/f64", 0,  i_64,   d_cc(Compile_Convert),      d_convertOpList (i64_Reinterpret_f64) ),   // 0xbd
    M3OP_F( "f32.reinterpret/i32", 0,  f_32,   d_cc(Compile_Convert),      d_convertOpList (f32_Reinterpret_i32) ),   // 0xbe
    M3OP_F( "f64.reinterpret/i64", 0,  f_64,   d_cc(Compile_Convert),      d_convertOpList (f64_Reinterpret_i64) ),   // 0xbf

    M3OP( "i32.extend8_s",         0,  i_32,   d_cc(ফাঁকা),                 d_unaryOpList (i32, Extend8_s) ),          // 0xc0
    M3OP( "i32.extend16_s",        0,  i_32,   d_cc(ফাঁকা),                 d_unaryOpList (i32, Extend16_s) ),         // 0xc1
    M3OP( "i64.extend8_s",         0,  i_64,   d_cc(ফাঁকা),                 d_unaryOpList (i64, Extend8_s) ),          // 0xc2
    M3OP( "i64.extend16_s",        0,  i_64,   d_cc(ফাঁকা),                 d_unaryOpList (i64, Extend16_s) ),         // 0xc3
    M3OP( "i64.extend32_s",        0,  i_64,   d_cc(ফাঁকা),                 d_unaryOpList (i64, Extend32_s) ),         // 0xc4

// Internal operations, for codepage logging only. They sit past c_waOp_lastCore,
// the last opcode this table covers, so GetOpInfo () can never reach them.
# ifdef DEBUG // for codepage logging. the order doesn't matter:
#define d_m3DebugOp(OP) M3OP (#OP, 0, none, d_ops (op_##OP))

# যদি d_m3HasFloat
#define d_m3DebugTypedOp(OP) M3OP (#OP, 0, none, d_ops (op_##OP##_i32, op_##OP##_i64, op_##OP##_f32, op_##OP##_f64))
# অন্যথা
#define d_m3DebugTypedOp(OP) M3OP (#OP, 0, none, d_ops (op_##OP##_i32, op_##OP##_i64))
# endif

    d_m3DebugOp (Compile),          d_m3DebugOp (Entry),            d_m3DebugOp (End),
    d_m3DebugOp (Unsupported),      d_m3DebugOp (CallRawFunction),

    d_m3DebugOp (GetGlobal_s32),    d_m3DebugOp (GetGlobal_s64),    d_m3DebugOp (ContinueLoop),     d_m3DebugOp (ContinueLoopIf),
# যদি d_m3_Has_স্তূপ_Switching
    d_m3DebugOp (ContinueLoop_Suspendable), d_m3DebugOp (ContinueLoopIf_Suspendable), d_m3DebugOp (EntryCheck),
#  যদি !d_m3EntryKeepsFrame
    d_m3DebugOp (Entry_Suspendable),
#  endif
# endif

    d_m3DebugOp (CopySlot_32),      d_m3DebugOp (PreserveCopySlot_32), d_m3DebugOp (If_s),          d_m3DebugOp (BranchIfPrologue_s),
    d_m3DebugOp (CopySlot_64),      d_m3DebugOp (PreserveCopySlot_64), d_m3DebugOp (If_r),          d_m3DebugOp (BranchIfPrologue_r),

    d_m3DebugOp (Select_i32_rss),   d_m3DebugOp (Select_i32_srs),   d_m3DebugOp (Select_i32_ssr),   d_m3DebugOp (Select_i32_sss),
    d_m3DebugOp (Select_i64_rss),   d_m3DebugOp (Select_i64_srs),   d_m3DebugOp (Select_i64_ssr),   d_m3DebugOp (Select_i64_sss),

# যদি d_m3HasFloat
    d_m3DebugOp (Select_f32_sss),   d_m3DebugOp (Select_f32_srs),   d_m3DebugOp (Select_f32_ssr),
    d_m3DebugOp (Select_f32_rss),   d_m3DebugOp (Select_f32_rrs),   d_m3DebugOp (Select_f32_rsr),

    d_m3DebugOp (Select_f64_sss),   d_m3DebugOp (Select_f64_srs),   d_m3DebugOp (Select_f64_ssr),
    d_m3DebugOp (Select_f64_rss),   d_m3DebugOp (Select_f64_rrs),   d_m3DebugOp (Select_f64_rsr),
# endif

    d_m3DebugOp (MemFill),          d_m3DebugOp (MemCopy),          d_m3DebugOp (MemInit),          d_m3DebugOp (তথ্য_Drop),

# যদি d_m3HasGasMetering
    d_m3DebugOp (UseGas),
# endif

# যদি d_m3HasRefTypes
    d_m3DebugOp (TableGet),         d_m3DebugOp (TableSet),         d_m3DebugOp (Table_আকার),
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

ধ্রুবক M3OpInfo c_operationsFC[] =
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
    M3OP( "data.drop",             0, none,   d_cc(Compile_তথ্য_Drop),       d_emptyOpList ),                       // 0x09

    M3OP( "memory.copy",           0, none,   d_cc(Compile_Memory_CopyFill), d_emptyOpList ),                       // 0x0a
    M3OP( "memory.fill",           0, none,   d_cc(Compile_Memory_CopyFill), d_emptyOpList ),                       // 0x0b

#যদি d_m3HasRefTypes
    M3OP( "table.init",            0, none,   d_cc(Compile_Table_Init),      d_emptyOpList ),                       // 0x0c
    M3OP( "elem.drop",             0, none,   d_cc(Compile_Elem_Drop),       d_emptyOpList ),                       // 0x0d
    M3OP( "table.copy",            0, none,   d_cc(Compile_Table_Copy),      d_emptyOpList ),                       // 0x0e
#অন্যথা
    M3OP_RESERVED, M3OP_RESERVED, M3OP_RESERVED,                                                                    // 0x0c...0x0e
#endif

#যদি d_m3HasRefTypes
    M3OP( "table.grow",            0, i_32,   d_cc(Compile_Table_GrowFill),  d_emptyOpList ),                       // 0x0f
    M3OP( "table.size",            1, i_32,   d_cc(Compile_Table_আকার),      d_emptyOpList ),                       // 0x10
    M3OP( "table.fill",            0, none,   d_cc(Compile_Table_GrowFill),  d_emptyOpList ),                       // 0x11
#অন্যথা
    M3OP_RESERVED, M3OP_RESERVED, M3OP_RESERVED,                                                                    // 0x0f...0x11
#endif

#যদি d_m3HasWideArithmetic
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

#যদি d_m3HasAtomics

// The 0xFE instructions share one entry per kind rather than each having its own: what
// tells them apart, the width of the cell and the operation, is worked out from the
// sub-opcode by Compile_Atomic. GetAtomicInfoIndex () maps a sub-opcode onto this table.
গণনা {
    c_opAtomic_notify,
    c_opAtomic_wait32,
    c_opAtomic_wait64,
    c_opAtomic_fence,
    c_opAtomic_load,
    c_opAtomic_store,
    c_opAtomic_rmw,
    c_opAtomic_cmpxchg
};

ধ্রুবক M3OpInfo c_operationsFE[] =
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

ধ্রুবক u32 c_অঙ্ক_Operations_FE = M3_গুনতি_OF(c_operationsFE);

// The entry a 0xFE sub-opcode uses, or -1 for one that names nothing: the spec leaves
// 0x04..0x0f and everything past cmpxchg unassigned. The accesses come in runs of seven,
// one per pairing of value type and cell width.
স্থির অন্তঃরেখ
i32 Get_Atomic_Info_সূচক (u32 i_sub)
{
    যদি (i_sub <= 0x03) ফেরত (i32)i_sub;
    যদি (i_sub < 0x10) ফেরত -1;
    যদি (i_sub <= 0x16) ফেরত c_opAtomic_load;
    যদি (i_sub <= 0x1d) ফেরত c_opAtomic_store;
    যদি (i_sub <= 0x47) ফেরত c_opAtomic_rmw;
    যদি (i_sub <= c_waOp_lastAtomic) ফেরত c_opAtomic_cmpxchg;
    ফেরত -1;
}

#endif // d_m3HasAtomics

// The opcodes above c_waOp_lastCore are sparse: the reference instructions at
// 0xd0..0xd2 and 0xd4, and the 0xfc extended-opcode prefix. Indexing c_operations out to
// 0xfc to reach them would leave ~50 empty entries there, so they get their own
// table, which GetHighOpIndex () maps the opcode onto.
গণনা {
#যদি d_m3HasRefTypes
    c_op_উপরের_ref_Null,
    c_op_উপরের_ref_Is_Null,
    c_op_উপরের_ref_Func,
#  যদি d_m3HasTypedRefs
    c_op_উপরের_ref_As_Non_Null,
#  endif
#endif
#যদি d_m3_Has_স্তূপ_Switching
    c_op_উপরের_cont_New,
    c_op_উপরের_cont_Bind,
    c_op_উপরের_suspend,
    c_op_উপরের_resume,
    c_op_উপরের_resume_Throw,
    c_op_উপরের_resume_Throw_Ref,
    c_op_উপরের_switch,
#endif
#যদি d_m3HasAtomics
    c_op_উপরের_atomic,
#endif
    c_op_উপরের_extended   // always present, so the enum is never empty
};

// Positional, so the table must stay in c_opHigh_* order: array designators
// are not valid C++, which the maintenance builds compile this as.
স্থির ধ্রুবক M3OpInfo c_operations_উপরের[] =
{
#যদি d_m3HasRefTypes
    M3OP( "ref.null",        1, any,               d_cc(Compile_Ref_Null),       d_emptyOpList ),  // 0xd0  c_opHigh_refNull
    M3OP( "ref.is_null",     0, i_32,              d_cc(Compile_Ref_IsNull),     d_emptyOpList ),  // 0xd1  c_opHigh_refIsNull
    M3OP( "ref.func",        1, any,               d_cc(Compile_Ref_Func),       d_emptyOpList ),  // 0xd2  c_opHigh_refFunc
#  যদি d_m3HasTypedRefs
    M3OP( "ref.as_non_null", 0, any,               d_cc(Compile_Ref_AsNonNull),  d_emptyOpList ),  // 0xd4  c_opHigh_refAsNonNull
#  endif
#endif
#যদি d_m3_Has_স্তূপ_Switching
    M3OP( "cont.new",          1, any,             d_cc(Compile_ContNew),        d_emptyOpList ),  // 0xe0  c_opHigh_contNew
    M3OP( "cont.bind",         2, any,             d_cc(Compile_ContBind),       d_emptyOpList ),  // 0xe1  c_opHigh_contBind
    M3OP( "suspend",           1, any,             d_cc(Compile_Suspend),        d_emptyOpList ),  // 0xe2  c_opHigh_suspend
    M3OP( "resume",            1, any,             d_cc(Compile_Resume),         d_emptyOpList ),  // 0xe3  c_opHigh_resume
    M3OP( "resume_throw",      2, any,             d_cc(Compile_Resume)   ,    d_emptyOpList ),  // 0xe4  c_opHigh_resumeThrow
    M3OP( "resume_throw_ref",  1, any,             d_cc(Compile_Resume)      , d_emptyOpList ),  // 0xe5  c_opHigh_resumeThrowRef
    M3OP( "switch",            2, any,             d_cc(Compile_Switch),         d_emptyOpList ),  // 0xe6  c_opHigh_switch
#endif

#যদি d_m3HasAtomics
    M3OP( "0xFE",            0, c_m3Type_unknown,  d_cc(Compile_ExtendedOpcode), d_emptyOpList ),  // 0xfe  c_opHigh_atomic
#endif
    M3OP( "0xFC",            0, c_m3Type_unknown,  d_cc(Compile_ExtendedOpcode), d_emptyOpList ),  // 0xfc  c_opHigh_extended
};
M3_STATIC_ASSERT (M3_গুনতি_OF (c_operations_উপরের) == c_op_উপরের_extended + 1, c_operations_উপরের_needs_one_entry_per_c_op_উপরের);

// clang-format on


// Opcodes the spec reserves leave zeroed holes in the tables above: no compiler
// and no operations. Every implemented op has at least one of the two.
স্থির অন্তঃরেখ
bool IsImplementedOp (IM3OpInfo i_info)
{
    ফেরত (i_info->compiler != c_cc_NULL or i_info->operations[0] != ফাঁকা);
}

ধ্রুবক u32 c_অঙ্ক_Operations   = M3_গুনতি_OF(c_operations);
ধ্রুবক u32 c_অঙ্ক_Operations_FC = M3_গুনতি_OF(c_operationsFC);

// Maps one of the sparse opcodes above c_waOp_lastCore onto c_operationsHigh,
// or -1 for anything else in that range.
স্থির অন্তঃরেখ
i32 Get_উপরের_Op_সূচক (m3opcode_t opcode)
{
    নির্বাচন (opcode) {
#যদি d_m3HasRefTypes
    ক্ষেত্র c_waOp_refNull: ফেরত c_op_উপরের_ref_Null;
    ক্ষেত্র c_waOp_refIsNull: ফেরত c_op_উপরের_ref_Is_Null;
    ক্ষেত্র c_waOp_refFunc: ফেরত c_op_উপরের_ref_Func;
#  যদি d_m3HasTypedRefs
    ক্ষেত্র c_waOp_refAsNonNull: ফেরত c_op_উপরের_ref_As_Non_Null;
#  endif
#endif
#যদি d_m3_Has_স্তূপ_Switching
    ক্ষেত্র c_waOp_contNew: ফেরত c_op_উপরের_cont_New;
    ক্ষেত্র c_waOp_contBind: ফেরত c_op_উপরের_cont_Bind;
    ক্ষেত্র c_waOp_suspend: ফেরত c_op_উপরের_suspend;
    ক্ষেত্র c_waOp_resume: ফেরত c_op_উপরের_resume;
    ক্ষেত্র c_waOp_resumeThrow: ফেরত c_op_উপরের_resume_Throw;
    ক্ষেত্র c_waOp_resumeThrowRef: ফেরত c_op_উপরের_resume_Throw_Ref;
    ক্ষেত্র c_waOp_switch: ফেরত c_op_উপরের_switch;
#endif
    ক্ষেত্র c_waOp_extended: ফেরত c_op_উপরের_extended;
#যদি d_m3HasAtomics
    ক্ষেত্র c_waOp_atomic: ফেরত c_op_উপরের_atomic;
#endif
    পূর্বনির্ধারিত: ফেরত -1;
    }
}

IM3OpInfo GetOpInfo (m3opcode_t opcode)
{
    IM3OpInfo info = ফাঁকা;

    নির্বাচন (opcode >> 8) {
    ক্ষেত্র 0x00:
        // c_operations is indexed by opcode only up to c_waOp_lastCore; past that
        // it holds internal operations (DEBUG builds), unreachable by opcode.
        যদি (M3_LIKELY(opcode <= c_waOp_lastCore)) {
            info = &c_operations[opcode];
        } অন্যথা {
            i32 উপরের = Get_উপরের_Op_সূচক(opcode);
            যদি (উপরের >= 0) {
                info = &c_operations_উপরের[উপরের];
            }
        }
        বিরতি;
    ক্ষেত্র c_waOp_extended:
        opcode &= 0xFF;
        যদি (M3_LIKELY(opcode <= c_waOp_lastExtended)) {
            info = &c_operationsFC[opcode];
        }
        বিরতি;
#যদি d_m3HasAtomics
    ক্ষেত্র c_waOp_atomic: {
        i32 index = Get_Atomic_Info_সূচক(opcode & 0xFF);
        যদি (index >= 0) {
            info = &c_operationsFE[index];
        }
    } বিরতি;
#endif
    }

    ফেরত (info and IsImplementedOp(info)) ? info : ফাঁকা;
}

#যদি d_m3HasGasMetering

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
গণনা {
    c_gasLocal   = 1,          // charged once per declared local, at function entry
    c_gasNominal = 1,          // nop, block, loop, if, const
    c_gasArith   = 45,
    c_gasShift   = 67,
    c_gasBranch  = 90,
    c_gas_Mem_আকার = 100,
    c_gasAccess  = 120,        // a local, global, table or memory access
    c_gasHeavy   = 10000,      // an indirect call, or anything that works over a whole region
    c_gasDivide  = 36000
};

// The most one op_UseGas can carry. A segment that reaches it is split into
// consecutive charges, which prepays exactly the same amount.
#  define d_m3MaxSegmentGas   0xFFFFFFFF

স্থির
u32 GetGasCost (m3opcode_t i_opcode)
{
    নির্বাচন (i_opcode) {
    ক্ষেত্র 0x00:                        // unreachable
    ক্ষেত্র 0x01:                        // nop
    ক্ষেত্র c_waOp_block:
    ক্ষেত্র c_waOp_loop:
    ক্ষেত্র c_waOp_if:
    ক্ষেত্র c_waOp_tryTable:             // a block that also installs handlers
    ক্ষেত্র c_waOp_refNull:              // a constant, by another name
    ক্ষেত্র c_waOp_refFunc:
        ফেরত c_gasNominal;

    ক্ষেত্র c_waOp_end:
        ফেরত 0;                     // the one control instruction the table prices at nothing

    ক্ষেত্র c_waOp_else:
    ক্ষেত্র c_waOp_branch:
    ক্ষেত্র c_waOp_branchIf:
    ক্ষেত্র c_waOp_return:
    ক্ষেত্র c_waOp_call:
    ক্ষেত্র c_waOp_returnCall:
    ক্ষেত্র c_waOp_throw:
    ক্ষেত্র c_waOp_throwRef:
        ফেরত c_gasBranch;

    ক্ষেত্র c_waOp_branchTable:
    ক্ষেত্র 0x1a:                        // drop
    ক্ষেত্র 0x1b:                        // select
    ক্ষেত্র c_waOp_selectTyped:
        ফেরত c_gasAccess;

    ক্ষেত্র c_waOp_callIndirect:
    ক্ষেত্র c_waOp_returnCallIndirect:
    ক্ষেত্র c_waOp_callRef:
    ক্ষেত্র c_waOp_returnCallRef:
    ক্ষেত্র c_waOp_memoryInit:
    ক্ষেত্র c_waOp_memoryCopy:
    ক্ষেত্র c_waOp_memoryFill:
    ক্ষেত্র c_waOp_tableInit:
    ক্ষেত্র c_waOp_tableCopy:
    ক্ষেত্র c_waOp_tableGrow:
    ক্ষেত্র c_waOp_tableFill:
    ক্ষেত্র 0x40:                        // memory.grow
        ফেরত c_gasHeavy;

    ক্ষেত্র c_wa_Op_তথ্য_Drop:
    ক্ষেত্র c_waOp_elemDrop:
    ক্ষেত্র c_wa_Op_table_আকার:
    ক্ষেত্র 0x3f:                        // memory.size
        ফেরত c_gas_Mem_আকার;

    ক্ষেত্র c_waOp_refIsNull:
    ক্ষেত্র c_waOp_refAsNonNull:
        ফেরত c_gasArith;

    পূর্বনির্ধারিত:
        বিরতি;
    }

    // The two integer arithmetic runs are priced by position, i32 at 0x6a and
    // i64 at 0x7c, the table giving both the same shape
    যদি ((i_opcode >= 0x6a and i_opcode <= 0x78) or (i_opcode >= 0x7c and i_opcode <= 0x8a)) {
        // clang-format off
        স্থির ধ্রুবক u16 c_arithmeticCosts [] = {
            c_gasArith,  c_gasArith,  c_gasArith,                   // add, sub, mul
            c_gasDivide, c_gasDivide, c_gasDivide, c_gasDivide,     // div_s, div_u, rem_s, rem_u
            c_gasArith,  c_gasArith,  c_gasArith,                   // and, or, xor
            c_gasShift,  c_gasShift,  c_gasShift,                   // shl, shr_s, shr_u
            c_gasBranch, c_gasBranch                                // rotl, rotr
        };
        // clang-format on

        ফেরত c_arithmeticCosts[i_opcode - (i_opcode <= 0x78 ? 0x6a : 0x7c)];
    }

    // the atomic accesses are memory accesses; what waits, wakes or orders is worth
    // as much as an operation over a whole region
    যদি (i_opcode >= (c_waOp_atomic << 8) and i_opcode <= ((c_waOp_atomic << 8) | c_waOp_lastAtomic)) {
        ফেরত (i_opcode <= c_waOp_atomicFence) ? c_gasHeavy : c_gasAccess;
    }
    যদি (i_opcode >= 0x20 and i_opcode <= 0x3e) {
        ফেরত c_gasAccess;           // locals, globals, table accesses, loads and stores
    }
    যদি (i_opcode >= 0x41 and i_opcode <= 0x44) {
        ফেরত c_gasNominal;          // i32/i64/f32/f64.const
    }
    যদি (i_opcode >= 0x8b and i_opcode <= 0xa6) {
        // the float run, priced as integer arithmetic except where it divides
        bool divides = (i_opcode == 0x91 or i_opcode == 0x95 or     // f32.sqrt, f32.div
                        i_opcode == 0x9f or i_opcode == 0xa3);      // f64.sqrt, f64.div

        ফেরত divides ? c_gasDivide : c_gasArith;
    }

    // comparisons and clz/ctz/popcnt (0x45..0x69, 0x79..0x7b), the conversions
    // and sign extensions (0xa7..0xc4), the saturating truncations, the wide
    // arithmetic, and anything else that reaches here: plain arithmetic
    ফেরত c_gasArith;
}

// The instructions ewasm's design segments a body at. A segment runs up to and
// including the next one of these, so every cycle a body can execute - a loop's
// back edge, a branch out of a block, falling off the end of a function - passes
// through at least one, and no path can run unmetered however it is entered.
স্থির
bool IsGasSegmentEnd (m3opcode_t i_opcode)
{
    নির্বাচন (i_opcode) {
    ক্ষেত্র c_waOp_loop:
    ক্ষেত্র c_waOp_if:
    ক্ষেত্র c_waOp_else:
    ক্ষেত্র c_waOp_end:
    ক্ষেত্র c_waOp_branch:
    ক্ষেত্র c_waOp_branchIf:
    ক্ষেত্র c_waOp_branchTable:
    ক্ষেত্র c_waOp_return:
    ক্ষেত্র c_waOp_returnCall:
    ক্ষেত্র c_waOp_returnCallIndirect:
    ক্ষেত্র c_waOp_returnCallRef:
        ফেরত true;
    পূর্বনির্ধারিত:
        ফেরত false;
    }
}

// Ends the open segment by writing what it accumulated into the immediate its
// op_UseGas is still waiting on. Emitting onto a later code page does not move
// an earlier one, so the reservation is still where it was left.
স্থির
শূন্য CloseGasSegment (IM3Compilation o)
{
    যদি (o->isGasSegmentOpen) {
        যদি (o->gasPatch) {
            u32 cost = o->gasCost;
            memcpy(o->gasPatch, &cost, আকার(cost));

            o->gasPatch = ফাঁকা;
        }
        o->gasCost          = 0;
        o->isGasSegmentOpen = false;
    }
}

// Whether this compilation emits the charges
স্থির অন্তঃরেখ
bool IsMetering (IM3Compilation o)
{
    ফেরত o->runtime->gasLimit != 0;
}

// Charges one instruction to the segment being compiled, opening a segment at
// this point in the code stream if none is open, and settling the segment when
// the instruction is one that ends it. Called before the instruction is
// compiled, which is what both halves need: the op_UseGas lands ahead of what it
// pays for, and the cost is final before a block-structured instruction - loop,
// if, else - compiles its body from inside its own compiler. The body then opens
// a segment of its own, inside the loop or the arm, where it is charged on every
// pass rather than once on the way in.
স্থির
M3_ফলাফল MeterOpcode (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_ফলাফল ফলাফল = m3Err_none;

    // constant expressions are not metered: they run once at instantiation,
    // before the module is anything a gas budget was handed out for
    যদি (o->function and o->page and IsMetering(o)) {
        // only the 0xFC or 0xFE prefix has been read so far; the instruction it names is
        // the LEB128 u32 the compiler is about to take. Peek it without consuming.
        // Anything the tables do not cover leaves the prefix priced on its own,
        // which is moot - the compile fails on it a moment later.
        যদি (i_opcode == c_waOp_extended or i_opcode == c_waOp_atomic) {
            bytes_t peek = o->wasm;
            u32     sub;
            u32     last = (i_opcode == c_waOp_atomic) ? c_waOp_lastAtomic : c_waOp_lastExtended;

            যদি (not ReadLEB_u32(&sub, &peek, o->wasmEnd) and sub <= last) {
                i_opcode = (m3opcode_t)((i_opcode << 8) | sub);
            }
        }

        u32 cost = GetGasCost(i_opcode);

        যদি (o->isGasSegmentOpen and o->gasCost > d_m3MaxSegmentGas - cost) {
            CloseGasSegment(o);
        }

        যদি (not o->isGasSegmentOpen) {
            o->isGasSegmentOpen = true;

_           (EmitOp(o, op_UseGas));
            o->gasPatch = (শূন্য*)GetPagePC(o->page);
            EmitConstant32(o, 0);
        }

        o->gasCost += cost;

        যদি (IsGasSegmentEnd(i_opcode)) {
            CloseGasSegment(o);
        }
    }

    _catch: ফেরত ফলাফল;
}

#endif // d_m3HasGasMetering


M3_ফলাফল CompileBlockStatements (IM3Compilation o)
{
    M3_ফলাফল ফলাফল   = m3Err_none;
    bool     validEnd = false;

    যখন (o->wasm < o->wasmEnd) {
#যদি d_m3EnableOpTracing
        যদি (o->অঙ্ক_Emits) {
            EmitOp(o, op_Dump_স্তূপ);
            EmitConstant32(o, o->অঙ্ক_Opcodes);
            EmitConstant32(o, GetMaxUsedSlotPlusOne(o));
            EmitPointer(o, o->function);

            o->অঙ্ক_Emits = 0;
        }
#endif

        m3opcode_t opcode;
        o->lastOpcodeStart = o->wasm;
_       (Read_opcode(&opcode, &o->wasm, o->wasmEnd));
        log_opcode(o, opcode);

        // Restrict opcodes when evaluating expressions
        যদি (M3_UNLIKELY(not o->function)) {
            // clang-format off
            নির্বাচন (opcode) {
            ক্ষেত্র c_waOp_i32_const: ক্ষেত্র c_waOp_i64_const:
            ক্ষেত্র c_waOp_f32_const: ক্ষেত্র c_waOp_f64_const:
            ক্ষেত্র c_waOp_getGlobal: ক্ষেত্র c_waOp_end:
#যদি d_m3HasRefTypes
            ক্ষেত্র c_waOp_refNull:   ক্ষেত্র c_waOp_refFunc:
#endif
#যদি d_m3HasExtendedConst
            ক্ষেত্র c_waOp_i32_add:   ক্ষেত্র c_waOp_i32_sub:   ক্ষেত্র c_waOp_i32_mul:
            ক্ষেত্র c_waOp_i64_add:   ক্ষেত্র c_waOp_i64_sub:   ক্ষেত্র c_waOp_i64_mul:
#endif
                বিরতি;
            পূর্বনির্ধারিত:
                _throw(m3Err_restrictedOpcode);
            }
            // clang-format on
        }

        IM3OpInfo opinfo = GetOpInfo(opcode);

        যদি (M3_UNLIKELY(opinfo == ফাঁকা)) {
            _throw(ErrorCompile(m3Err_unknownOpcode, o, "opcode '%x' not available", opcode));
        }

#যদি d_m3HasGasMetering
_       (MeterOpcode(o, opcode));
#endif

        যদি (opinfo->compiler != c_cc_NULL) {
_           ((*GetCompiler(opinfo))(o, opcode))
        } অন্যথা {
_           (Compile_Operator(o, opcode));
        }

        o->previousOpcode = opcode;

        যদি (opcode == c_waOp_else) {
            _throwif(m3Err_wasmMalformed, o->block.opcode != c_waOp_if);
            validEnd = true;
            বিরতি;
        } অন্যথা যদি (opcode == c_waOp_end) {
            validEnd = true;
            বিরতি;
        }
    }
    _throwif(m3Err_wasmMalformed, not validEnd);

_catch:
    ফেরত ফলাফল;
}

স্থির
M3_ফলাফল ঠেলো_Block_Results (IM3Compilation o)
{
    M3_ফলাফল ফলাফল = m3Err_none;

    u16 অঙ্ক_Results = Get_Func_Type_অঙ্ক_Results(o->block.type);

    জন্য (u16 i = 0; i < অঙ্ক_Results; ++i) {
        m3type_t type = Get_Func_Type_ফলাফল_Type(o->block.type, i);

        যদি (i == অঙ্ক_Results - 1 and IsFpType(type)) {
_           (ঠেলো_Register(o, type));
        } অন্যথা {
_           (ঠেলো_Allocated_Slot(o, type));
        }
    }

    _catch: ফেরত ফলাফল;
}


M3_ফলাফল CompileBlock (IM3Compilation o, IM3FuncType i_blockType, m3opcode_t i_blockOpcode)
{
                                                                                        d_m3Assert (not IsRegisterAllocated (o, 0));
                                                                                        d_m3Assert (not IsRegisterAllocated (o, 1));
    InvalidateFold(o);

    M3CompilationScope  outerScope = o->block;
    M3CompilationScope* block      = &o->block;

    block->outer   = &outerScope;
    block->pc      = GetPagePC(o->page);
    block->patches = ফাঁকা;
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
#যদি d_m3HasSnapshots
    // op_Loop's frame records the body, which is also the loop's identity;
    // op_TryTable's records its clause table, two words a clause back from it
    যদি (i_blockOpcode == c_waOp_loop) {
_       (RecordBlockStart(o, i_blockOpcode, block->pc, 0));
    }
#  যদি d_m3HasExceptionHandling
    অন্যথা যদি (i_blockOpcode == c_waOp_tryTable) {
_       (RecordBlockStart(o, i_blockOpcode, block->pc - 2 * o->অঙ্ক_Try_Clauses, o->অঙ্ক_Try_Clauses));
    }
#  endif
#endif

    // validate and dealloc params ----------------------------

    u16 স্তূপ_সূচক = o->স্তূপ_সূচক;

    u16 অঙ্ক_Params = Get_Func_Type_অঙ্ক_Params(i_blockType);

    যদি (i_blockOpcode != c_waOp_else) {
        জন্য (u16 i = 0; i < অঙ্ক_Params; ++i) {
            m3type_t type = GetFuncTypeParamType(i_blockType, অঙ্ক_Params - 1 - i);
_           (বের_করো_Type(o, type));
        }
    } অন্যথা {
        যদি (Is_স্তূপ_Polymorphic(o) && o->block.block_স্তূপ_সূচক + অঙ্ক_Params > o->স্তূপ_সূচক) {
            o->স্তূপ_সূচক = o->block.block_স্তূপ_সূচক;
        } অন্যথা {
            o->স্তূপ_সূচক -= অঙ্ক_Params;
        }
    }

    u16 param_সূচক        = o->স্তূপ_সূচক;
    block->exit_স্তূপ_সূচক = param_সূচক; // consume the params at block exit

    // keep copies of param slots in the stack
    o->স্তূপ_সূচক = স্তূপ_সূচক;

    // find slots for the results ----------------------------
_   (ঠেলো_Block_Results(o));

    স্তূপ_সূচক = o->স্তূপ_সূচক;

    // dealloc but keep record of the result slots in the stack
    u16 অঙ্ক_Results = Get_Func_Type_অঙ্ক_Results(i_blockType);
    যখন (অঙ্ক_Results--) {
        বের_করো(o);
    }

    block->block_স্তূপ_সূচক = o->স্তূপ_সূচক = স্তূপ_সূচক;

    // push the params back onto the stack -------------------
    জন্য (u16 i = 0; i < অঙ্ক_Params; ++i) {
        m3type_t type = GetFuncTypeParamType(i_blockType, i);

        u16 slot = Get_Slot_For_স্তূপ_সূচক(o, param_সূচক + i);
        ঠেলো(o, type, slot);

        যদি (slot >= o->slot_First_Dynamic_সূচক && slot != c_slotUnused) {
_           (MarkSlotsAllocatedByType(o, slot, type));
        }
    }

    //--------------------------------------------------------

#যদি d_m3HasExceptionHandling
    // with the scope pushed and the params back on, a catch label of 0 names
    // this block - which is what the proposal says it should
    যদি (i_blockOpcode == c_waOp_tryTable) {
_       (EmitCatchStubs(o));
    }
#endif

_   (CompileBlockStatements(o));

_   (ValidateBlockEnd(o));

    যদি (o->function)    // skip for expressions
    {
        যদি (not Is_স্তূপ_Polymorphic(o)) {
_           (ResolveBlockResults(o, &o->block, /* isBranch: */ false));
        }

#যদি d_m3HasExceptionHandling
        // Falling out of the end of a try region retires its handler. This sits
        // ahead of PatchBranches so branches into the block's exit land past it:
        // a branch out of the try popped its own handler at the branch site.
        যদি (i_blockOpcode == c_waOp_tryTable) {
_           (EmitOp(o, op_বের_করো_Handlers));
            EmitConstant32(o, 1);
        }
#endif

_       (Unwind_Block_স্তূপ(o))

        যদি (not ((i_blockOpcode == c_waOp_if and অঙ্ক_Results) or o->previousOpcode == c_waOp_else)) {
            o->স্তূপ_সূচক = o->block.exit_স্তূপ_সূচক;
_           (ঠেলো_Block_Results(o));
        }
    }

    PatchBranches(o);

#যদি d_m3HasSnapshots
    RecordBlockEnd(o);

    যদি (o->localInitDepth) {
        জন্য (u32 i = 0; i < o->snapshotMap->অঙ্ক_Locals; ++i) {
            যদি (o->localInitDepth[i] >= (u32)o->block.depth) {
                o->localInitDepth[i] = UINT32_MAX;
            }
        }
    }
#endif
    o->block = outerScope;

} _catch: ফেরত ফলাফল;
}

স্থির
M3_ফলাফল CompileLocals (IM3Compilation o)
{
    M3_ফলাফল ফলাফল;

    u32 অঙ্ক_Locals = 0;
    u32 অঙ্ক_Local_Blocks;
_   (ReadLEB_u32(&অঙ্ক_Local_Blocks, &o->wasm, o->wasmEnd));

    জন্য (u32 l = 0; l < অঙ্ক_Local_Blocks; ++l) {
        u32      var_গুনতি;
        m3type_t localType;

_       (ReadLEB_u32(&var_গুনতি, &o->wasm, o->wasmEnd));
_       (Parse_মান_Type(o->module, &localType, &o->wasm, o->wasmEnd));
        অঙ্ক_Locals += var_গুনতি;                                                          m3log (compile, "pushing locals. count: %d; type: %s", var_গুনতি, c_waTypes [BaseTypeOf(localType)]);
        যখন (var_গুনতি--) {
_           (ঠেলো_Allocated_Slot(o, localType));
        }
    }

    যদি (o->function) {
        o->function->অঙ্ক_Locals = অঙ্ক_Locals;
    }

    _catch: ফেরত ফলাফল;
}

স্থির
M3_ফলাফল ReserveConstants (IM3Compilation o)
{
    M3_ফলাফল ফলাফল = m3Err_none;

    // in the interest of speed, this blindly scans the Wasm code looking for any byte
    // that looks like an const opcode.
    u16 অঙ্ক_Constant_Slots = 0;

    bytes_t wa = o->wasm;
    যখন (wa < o->wasmEnd) {
        u8  code     = *wa++;
        u16 addSlots = 0;

        যদি (code == c_waOp_i32_const or code == c_waOp_f32_const) {
            addSlots = 1;
        } অন্যথা যদি (code == c_waOp_i64_const or code == c_waOp_f64_const) {
            addSlots = Get_Type_অঙ্ক_Slots(c_m3Type_i64);
        }

        যদি (অঙ্ক_Constant_Slots + addSlots >= d_m3_Max_Constant_Table_আকার) {
            বিরতি;
        }

        অঙ্ক_Constant_Slots += addSlots;
    }

    // if constants overflow their reserved stack space, the compiler simply emits op_Const
    // operations as needed. Compiled expressions (global inits) don't pass through this
    // ReserveConstants function and thus always produce inline constants.

    AlignSlotToType(&অঙ্ক_Constant_Slots, c_m3Type_i64);                                           m3log (compile, "reserved constant slots: %d", অঙ্ক_Constant_Slots);

    o->slot_First_Dynamic_সূচক = o->slot_First_Const_সূচক + অঙ্ক_Constant_Slots;

    যদি (o->slot_First_Dynamic_সূচক >= d_m3MaxFunctionSlots) {
        _throw(m3_Err_function_স্তূপ_Overflow);
    }

    _catch:
    ফেরত ফলাফল;
}


// A constant expression compiles like a miniature function root block: no args,
// no locals and a single result, which Compile_End copies down into slot 0 for
// EvaluateExpression to read back.
M3_ফলাফল CompileExpression (IM3Compilation o, IM3FuncType i_ফলাফল_Type)
{
    M3_ফলাফল ফলাফল = m3Err_none;

    o->block.type = i_ফলাফল_Type;

    u16 অঙ্ক_Ret_Slots = Get_Func_Type_অঙ্ক_Results(i_ফলাফল_Type) * c_io_Slot_গুনতি;

    জন্য (u16 i = 0; i < অঙ্ক_Ret_Slots; ++i) {
        MarkSlotAllocated(o, i);
    }

    o->max_স্তূপ_Slots = o->slot_Max_Allocated_সূচক_Plus_One = o->slot_First_Dynamic_সূচক = অঙ্ক_Ret_Slots;

_   (CompileBlockStatements(o));

    _catch: ফেরত ফলাফল;
}


M3_ফলাফল CompileFunction (IM3Function io_function)
{
    যদি (!io_function->wasm) {
        // An import. Either linking pointed it at another module's function -
        // in which case that one carries the body - or a host function was
        // bound to it, or it is simply unsatisfied.
        IM3Function impl = Function_Implementation(io_function);

        যদি (impl != io_function) {
            যদি (not impl->compiled) {
                M3_ফলাফল r = CompileFunction(impl);
                যদি (r) {
                    ফেরত r;
                }
            }

            // keep the placeholder callable too, for anything still holding it
            io_function->compiled = impl->compiled;
            ফেরত m3Err_none;
        }

        যদি (io_function->compiled) {
            ফেরত m3Err_none;
        }

        ফেরত ErrorModule(m3Err_functionImportMissing, io_function->module, "'%s.%s'",
                           GetFunctionImportModuleName(io_function),
                           m3_GetFunctionName(io_function));
    }

#যদি d_m3EnableValidation
    যদি (not io_function->module->runtime->skipValidation) {
        M3_ফলাফল vr = ValidateFunction(io_function);
        যদি (vr) {
            ফেরত vr;
        }
    }
#endif

    IM3FuncType funcType = io_function->funcType;                   m3log (compile, "compiling: [%d] %s %s; wasm-size: %d",
                                                                        io_function->index, m3_GetFunctionName (io_function), SPrintFuncTypeSignature (funcType), (u32) (io_function->wasmEnd - io_function->wasm));
    IM3Runtime runtime = io_function->module->runtime;

    IM3Compilation o = &runtime->compilation;                       d_m3Assert (d_m3MaxFunctionSlots >= d_m3_Max_Function_স্তূপ_Height * (d_m3Use32BitSlots + 1))  // need twice as many slots in 32-bit mode
    memset(o, 0x0, আকার(M3Compilation));

    o->runtime    = runtime;
    o->module     = io_function->module;
    o->function   = io_function;
    o->wasm       = io_function->wasm;
    o->wasmEnd    = io_function->wasmEnd;
    o->block.type = funcType;

_try {
    // skip over code size. the end was already calculated during parse phase
    u32 আকার_2;
_   (ReadLEB_u32(&আকার_2, &o->wasm, o->wasmEnd));                     d_m3Assert (আকার_2 == (o->wasmEnd - o->wasm))

#যদি d_m3HasSnapshots
    o->bodyStart = o->wasm;
#endif

#যদি d_m3HasSnapshots
    // only code that can be suspended can end up in a snapshot
    যদি (runtime->isSuspendable) {
        o->snapshotMap = m3_AllocStruct(M3SnapshotMap);
        _throwifnull(o->snapshotMap);
    }
#endif

_   (AcquireCompilationCodePage(o, &o->page));

    pc_t pc = GetPagePC(o->page);

    u16 অঙ্ক_Ret_Slots = Get_Function_অঙ্ক_Returns(o->function) * c_io_Slot_গুনতি;

    জন্য (u16 i = 0; i < অঙ্ক_Ret_Slots; ++i) {
        MarkSlotAllocated(o, i);
    }

    o->function->অঙ্ক_Ret_Slots = o->slot_First_Dynamic_সূচক = অঙ্ক_Ret_Slots;

    u16 অঙ্ক_Args = Get_Function_অঙ্ক_Args(o->function);

    // push the arg types to the type stack
    জন্য (u16 i = 0; i < অঙ্ক_Args; ++i) {
        m3type_t type = GetFunctionArgType(o->function, i);
_       (ঠেলো_Allocated_Slot(o, type));

        // prevent allocator fill-in
        o->slot_First_Dynamic_সূচক += c_io_Slot_গুনতি;
    }

    o->slot_Max_Allocated_সূচক_Plus_One = o->function->অঙ্ক_Ret_And_Arg_Slots = o->slot_First_Local_সূচক = o->slot_First_Dynamic_সূচক;

_   (CompileLocals(o));

#যদি d_m3HasSnapshots
_   (RecordLocals(o));
#endif

#যদি d_m3HasGasMetering
    // the table charges for a function's locals as well as for its instructions.
    // op_Entry is what sets them up, so the charge rides on the body's first
    // segment, which is emitted right after it
    o->gasCost = o->function->অঙ্ক_Locals * c_gasLocal;
#endif

    u16 maxSlot = GetMaxUsedSlotPlusOne(o);

    o->function->অঙ্ক_Local_Bytes = (maxSlot - o->slot_First_Local_সূচক) * আকার(m3slot_t);

    o->slot_First_Const_সূচক = o->slot_Max_Const_সূচক = maxSlot;

    // ReserveConstants initializes o->firstDynamicSlotNumber
_   (ReserveConstants(o));

    // start tracking the max stack used (Push() also updates this value) so that op_Entry can precisely detect stack overflow
    o->max_স্তূপ_Slots = o->slot_Max_Allocated_সূচক_Plus_One = o->slot_First_Dynamic_সূচক;

    o->block.block_স্তূপ_সূচক = o->স্তূপ_First_Dynamic_সূচক = o->স্তূপ_সূচক;                           m3log (compile, "start stack index: %u; max stack slots: %u",
                                                                                                           (u32) o->স্তূপ_First_Dynamic_সূচক, (u32) o->max_স্তূপ_Slots);
#যদি d_m3_Has_স্তূপ_Switching
    // A suspendable body opens by checking for a pause request, once its frame
    // is set up. The pause point's offset is the first instruction's, just past
    // the local declarations - the body's end, when it has no other.
    যদি (runtime->isSuspendable) {
        o->lastOpcodeStart = o->wasm;
#  যদি d_m3EntryKeepsFrame
_       (EmitOp(o, op_Entry));
        EmitPointer(o, io_function);
_       (EmitOp(o, op_EntryCheck));
_       (RecordSafePoint(o, safepoint_entry, 1, 0, 0, ফাঁকা));
#  অন্যথা
_       (EmitOp(o, op_Entry_Suspendable));
        EmitPointer(o, io_function);
_       (RecordSafePoint(o, safepoint_entry, 0, 0, 0, ফাঁকা));
#  endif
    } অন্যথা
#endif
    {
_       (EmitOp(o, op_Entry));
        EmitPointer(o, io_function);
    }

_   (CompileBlockStatements(o));

    _throwif(m3Err_wasmMalformed, o->previousOpcode != c_waOp_end);

    io_function->compiled      = pc;
    io_function->max_স্তূপ_Slots = o->max_স্তূপ_Slots;

    u16 অঙ্ক_Constant_Slots = o->slot_Max_Const_সূচক - o->slot_First_Const_সূচক;                           m3log (compile, "unique constant slots: %u; unused slots: %u",
                                                                                                           অঙ্ক_Constant_Slots, o->slot_First_Dynamic_সূচক - o->slot_Max_Const_সূচক);
    io_function->অঙ্ক_Constant_Bytes = অঙ্ক_Constant_Slots * আকার(m3slot_t);

    যদি (অঙ্ক_Constant_Slots) {
        io_function->constants = m3_CopyMem((cbytes_t)o->constants, io_function->অঙ্ক_Constant_Bytes);
        _throwifnull(io_function->constants);
    }

#যদি d_m3HasSnapshots
    io_function->snapshotMap = o->snapshotMap;
    o->snapshotMap           = ফাঁকা;
#endif

} _catch:

    ReleaseCompilationCodePage(o);

#যদি d_m3HasSnapshots
    SnapshotMap_Free(o->snapshotMap);
    o->snapshotMap = ফাঁকা;
    m3_Free(o->localInitDepth);
#endif

    ফেরত ফলাফল;
}

#যদি d_m3_Has_স্তূপ_Switching
m3ret_t ResumeContinuation (IM3Runtime i_runtime, IM3Continuation i_cont)
{
    যদি (M3_UNLIKELY(!i_cont || i_cont->state != cont_suspended)) {
        ফেরত m3Err_none;
    }

    M3MemoryHeader* _mem = i_cont->entryFunction ? Module_MemoryHeader(i_cont->entryFunction->module) : ফাঁকা;

    i_runtime->activeContinuation = i_cont;
    i_cont->state                 = cont_running;

    i32 outermost     = (i32)i_cont->অঙ্ক_Frames - 1;
    i_cont->অঙ্ক_Frames = 0;

    // with no frames standing, this lands straight on the suspension point
    m3ret_t r = ReplayFrames(i_cont, outermost, _mem);

    যদি (r == m3Err_continuationSuspended) {
        i_cont->state = cont_suspended;
    } অন্যথা {
        i_cont->state = cont_returned;
    }
    i_runtime->activeContinuation = ফাঁকা;

    ফেরত r;
}
#endif
