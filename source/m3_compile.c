//
//  m3_compile.c
//
//  Created by Steven Massey on 4/17/19.
//  Copyright © 2019 Steven Massey. All rights reserved.
//

// Allow using opcodes for compilation process
#నిర్వచించు M3_COMPILE_OPCODES

#అన్తర్భావ "m3_env.h"
#అన్తర్భావ "m3_compile.h"
#అన్తర్భావ "m3_exec.h"
#అన్తర్భావ "m3_exception.h"
#అన్తర్భావ "m3_info.h"
#అన్తర్భావ "m3_validate.h"

// Resolves M3OpInfo.compiler, an index, to the function it names. Defined down
// with c_compilers [], which has to follow every compiler it points at.
స్థిర
M3Compiler GetCompiler (IM3OpInfo i_opInfo);

//----- EMIT --------------------------------------------------------------------------------------------------------------

స్థిర అంతఃవరుస
pc_t GetPC (IM3Compilation o)
{
    తిరిగి Get_పుట_PC(o->పుట);
}

#ఒకవేళ d_m3HasSnapshots

#  define d_m3GrowSnapshotArray(TYPE, ARRAY, లెక్క_2, సామర్థ్యం_2)                    \
    ఒకవేళ ((లెక్క_2) == (సామర్థ్యం_2)) {                                                 \
        u32   new_సామర్థ్యం = (సామర్థ్యం_2) ? (సామర్థ్యం_2) * 2 : 8;                     \
        TYPE* grown = m3_ReallocArray(TYPE, (ARRAY), new_సామర్థ్యం, (సామర్థ్యం_2));   \
        ఒకవేళ (not grown) {                                                         \
            తిరిగి m3Err_mallocFailed;                                           \
        }                                                                        \
        (ARRAY)    = grown;                                                      \
        (సామర్థ్యం_2) = new_సామర్థ్యం;                                                \
    }

// Where the instruction being compiled starts, counted from the start of the
// function's body: the one position every build of every revision agrees on
స్థిర అంతఃవరుస
u32 Get_Wasm_స్థానభ్రంశం (IM3Compilation o)
{
    తిరిగి (u32)(o->చివరి_Opcode_ప్రారంభం - o->body_ప్రారంభం);
}

// Whether the operand stack entry at i_index is a value the frame holds. Every
// block keeps records of its parameter and result slots on the stack, between
// the height it was entered at and where its own values begin (see
// CompileBlock). Those are landing pads rather than values: nothing need have
// written them yet, and the slots they name may be in use for something else.
// The function's own scope has none - that stretch is its arguments and locals.
స్థిర
bool Is_సజీవ_దొంతర_నమోదు (IM3Compilation o, u16 i_సూచీ)
{
    కొరకు (IM3CompilationScope scope = &o->ఖండం; scope->outer; scope = scope->outer) {
        ఒకవేళ (i_సూచీ >= scope->exit_దొంతర_సూచీ and i_సూచీ < scope->ఖండం_దొంతర_సూచీ) {
            తిరిగి false;
        }
    }

    తిరిగి true;
}

స్థిర
M3_ఫలితం కూడు_సురక్షిత_బిందువు_విలువ (IM3Compilation o, M3_సురక్షిత_బిందువు* io_బిందువు, u16 i_సూచీ)
{
    M3SnapshotMap* map = o->snapshotMap;

    d_m3GrowSnapshotArray(M3_స్థానం_విలువ, map->విలువలు, map->అంకె_విలువలు, o->విలువలు_సామర్థ్యం);

    map->విలువలు[map->అంకె_విలువలు].స్థానం = o->wasm_దొంతర[i_సూచీ];
    map->విలువలు[map->అంకె_విలువలు].type = o->type_దొంతర[i_సూచీ];
    map->అంకె_విలువలు++;
    io_బిందువు->అంకె_విలువలు++;

    తిరిగి m3Err_none;
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
స్థిర
M3_ఫలితం రికార్డు_సురక్షిత_బిందువు (IM3Compilation o, M3_సురక్షిత_బిందువు_రకం i_రకం, u32 i_పదాలు_వెనుక, u16 i_అంకె_Results,
                          u16 i_aux, IM3CompilationScope i_loop)
{
_try {
    M3SnapshotMap* map = o->snapshotMap;

    ఒకవేళ (not map or not o->పుట) {
        తిరిగి m3Err_none;
    }

    d_m3GrowSnapshotArray(M3_సురక్షిత_బిందువు, map->సురక్షిత_Points, map->అంకె_సురక్షిత_Points, o->సురక్షిత_Points_సామర్థ్యం);

    // grown before it is pointed at: adding values never moves the safepoints
    M3_సురక్షిత_బిందువు* బిందువు     = &map->సురక్షిత_Points[map->అంకె_సురక్షిత_Points];
    బిందువు->pc              = GetPC(o) - i_పదాలు_వెనుక;
    బిందువు->wasm_స్థానభ్రంశం      = Get_Wasm_స్థానభ్రంశం(o);
    బిందువు->మొదటి_విలువ      = map->అంకె_విలువలు;
    బిందువు->మొదటి_Local_విలువ = UINT32_MAX;
    బిందువు->అంకె_విలువలు       = 0;
    బిందువు->అంకె_Results      = i_అంకె_Results;
    బిందువు->aux             = i_aux;
    బిందువు->resumeType      = c_m3Type_none;
    బిందువు->రకం            = (u8)i_రకం;
    బిందువు->flags           = 0;

    // Non-defaultable locals can still be uninitialized at a safepoint. Keep
    // their heap types, but allow the null placeholder until local.set makes
    // them definitely initialized. Branches to a loop use its entry context.
    ఒకవేళ (o->local_Init_లోతు) {
        బిందువు->మొదటి_Local_విలువ = map->అంకె_విలువలు;
        కొరకు (u32 i = 0; i < map->అంకె_Locals; ++i) {
            d_m3GrowSnapshotArray(M3_స్థానం_విలువ, map->విలువలు, map->అంకె_విలువలు, o->విలువలు_సామర్థ్యం);
            M3_స్థానం_విలువ విలువ = map->locals[i];
            ఒకవేళ (o->local_Init_లోతు[i] == UINT32_MAX or
                (i_loop and o->local_Init_లోతు[i] >= (u32)i_loop->లోతు)) {
                విలువ.type &= ~d_m3Type_refNonNull;
            }
            map->విలువలు[map->అంకె_విలువలు++] = విలువ;
        }
        బిందువు->మొదటి_విలువ = map->అంకె_విలువలు;
    }

    u16 శిఖరం = o->దొంతర_సూచీ;

    ఒకవేళ (i_రకం != safepoint_suspend) {
        శిఖరం = (శిఖరం > i_అంకె_Results) ? (u16)(శిఖరం - i_అంకె_Results) : 0;
    }
    ఒకవేళ (i_loop) {
        శిఖరం = M3_MIN(శిఖరం, i_loop->exit_దొంతర_సూచీ);
    }

    కొరకు (u16 i = o->దొంతర_మొదటి_Dynamic_సూచీ; i < శిఖరం; ++i) {
        ఒకవేళ (Is_సజీవ_దొంతర_నమోదు(o, i)) {
_           (కూడు_సురక్షిత_బిందువు_విలువ(o, బిందువు, i));
        }
    }

    ఒకవేళ (i_loop) {
        u16 అంకె_Params = Get_Func_Type_అంకె_Params(i_loop->type);

        కొరకు (u16 i = 0; i < అంకె_Params; ++i) {
_           (కూడు_సురక్షిత_బిందువు_విలువ(o, బిందువు, (u16)(i_loop->exit_దొంతర_సూచీ + i)));
        }
    }

    map->అంకె_సురక్షిత_Points++;

} _catch: తిరిగి ఫలితం;
}

// A back edge: op_ContinueLoop, or op_ContinueLoopIf when i_isConditional, and
// the loop it goes back to. The conditional one suspends before it has taken
// the branch, so it has to be resumed with the condition that took it.
స్థిర
M3_ఫలితం రికార్డు_వెనుక_అంచు (IM3Compilation o, IM3CompilationScope i_loop, bool i_isConditional)
{
    M3_ఫలితం ఫలితం = m3Err_none;

    // the loop is named by its record in the map, which has to fit the field
    _throwif("a function has too many loops for its snapshot map",
             o->snapshotMap and o->పుట and (i_loop->snapshot_ఖండం == 0 or i_loop->snapshot_ఖండం > UINT16_MAX + 1u));

_   (రికార్డు_సురక్షిత_బిందువు(o, safepoint_op, 2, 0, (u16)(i_loop->snapshot_ఖండం - 1), i_loop));

    ఒకవేళ (i_isConditional and o->snapshotMap and o->పుట) {
        o->snapshotMap->సురక్షిత_Points[o->snapshotMap->అంకె_సురక్షిత_Points - 1].flags |= d_m3_సురక్షిత_బిందువు_తీసుకున్న_Branch;
    }

    _catch: తిరిగి ఫలితం;
}

// A call, waiting on its callee with i_numResults not yet written; the
// callee's frame starts i_calleeSlot slots above this one. A return_call that
// is compiled as a call is one too, since its frame stands while the callee
// runs, but it is marked: in Wasm the callee has replaced it, so a snapshot
// walks past it rather than writing it.
స్థిర
M3_ఫలితం రికార్డు_Call (IM3Compilation o, u16 i_అంకె_Results, u16 i_callee_స్థానం, bool i_isReturnCall)
{
    M3_ఫలితం ఫలితం = రికార్డు_సురక్షిత_బిందువు(o, safepoint_call, 0, i_అంకె_Results, i_callee_స్థానం, ఖాళీ);

    ఒకవేళ (not ఫలితం and i_isReturnCall and o->snapshotMap and o->పుట) {
        o->snapshotMap->సురక్షిత_Points[o->snapshotMap->అంకె_సురక్షిత_Points - 1].flags |= d_m3_సురక్షిత_బిందువు_తోక_Call;
    }

    తిరిగి ఫలితం;
}

// Where the function's arguments and locals are kept. Unlike the operand
// stack, that never changes, so it is recorded once rather than at every
// safepoint.
స్థిర
M3_ఫలితం రికార్డు_Locals (IM3Compilation o)
{
    M3SnapshotMap* map = o->snapshotMap;

    ఒకవేళ (map and o->దొంతర_సూచీ) {
        map->locals = m3_AllocArray(M3_స్థానం_విలువ, o->దొంతర_సూచీ);

        ఒకవేళ (not map->locals) {
            తిరిగి m3Err_mallocFailed;
        }

        కొరకు (u16 i = 0; i < o->దొంతర_సూచీ; ++i) {
            map->locals[i].స్థానం = o->wasm_దొంతర[i];
            map->locals[i].type = o->type_దొంతర[i];
        }

        map->అంకె_Locals = o->దొంతర_సూచీ;
        కొరకు (u32 i = Get_Function_అంకె_వాదనలు(o->function); i < map->అంకె_Locals; ++i) {
            ఒకవేళ (IsRefType(map->locals[i].type) and not IsNullableRef(map->locals[i].type)) {
                o->local_Init_లోతు = m3_AllocArray(u32, map->అంకె_Locals);
                ఒకవేళ (not o->local_Init_లోతు) {
                    తిరిగి m3Err_mallocFailed;
                }
                కొరకు (u32 j = i; j < map->అంకె_Locals; ++j) {
                    ఒకవేళ (IsRefType(map->locals[j].type) and not IsNullableRef(map->locals[j].type)) {
                        o->local_Init_లోతు[j] = UINT32_MAX;
                    }
                }
                ఆపు;
            }
        }
    }

    తిరిగి m3Err_none;
}

// A loop or a try_table, whose native frame a snapshot names by the
// instruction's offset. i_pc is what that frame records. The scope being
// compiled keeps the record, so that RecordBlockEnd can close it.
స్థిర
M3_ఫలితం రికార్డు_ఖండం_ప్రారంభం (IM3Compilation o, m3opcode_t i_opcode, pc_t i_pc, u32 i_అంకె_Clauses)
{
    M3SnapshotMap* map = o->snapshotMap;

    ఒకవేళ (map and o->పుట) {
        d_m3GrowSnapshotArray(M3_ఖండం_ప్రారంభం, map->ఖండాలు, map->అంకె_ఖండాలు, o->ఖండాలు_సామర్థ్యం);

        M3_ఖండం_ప్రారంభం* ఖండం = &map->ఖండాలు[map->అంకె_ఖండాలు++];
        ఖండం->pc           = i_pc;
        ఖండం->wasm_స్థానభ్రంశం   = Get_Wasm_స్థానభ్రంశం(o);
        ఖండం->wasm_చివర      = UINT32_MAX;
        ఖండం->అంకె_Clauses   = (u16)i_అంకె_Clauses;
        ఖండం->opcode       = (u8)i_opcode;

        o->ఖండం.snapshot_ఖండం = map->అంకె_ఖండాలు;
    }

    తిరిగి m3Err_none;
}

// Closes the scope's region, while the instruction just read is its end
స్థిర
శూన్య రికార్డు_ఖండం_చివర (IM3Compilation o)
{
    ఒకవేళ (o->snapshotMap and o->ఖండం.snapshot_ఖండం) {
        o->snapshotMap->ఖండాలు[o->ఖండం.snapshot_ఖండం - 1].wasm_చివర = Get_Wasm_స్థానభ్రంశం(o);
    }
}

#  undef d_m3GrowSnapshotArray

#లేకపోతే

స్థిర అంతఃవరుస
M3_ఫలితం రికార్డు_సురక్షిత_బిందువు (IM3Compilation o, M3_సురక్షిత_బిందువు_రకం i_రకం, u32 i_పదాలు_వెనుక, u16 i_అంకె_Results,
                          u16 i_aux, IM3CompilationScope i_loop)
{
    (శూన్య)o;
    (శూన్య)i_రకం;
    (శూన్య)i_పదాలు_వెనుక;
    (శూన్య)i_అంకె_Results;
    (శూన్య)i_aux;
    (శూన్య)i_loop;
    తిరిగి m3Err_none;
}

స్థిర అంతఃవరుస
M3_ఫలితం రికార్డు_వెనుక_అంచు (IM3Compilation o, IM3CompilationScope i_loop, bool i_isConditional)
{
    (శూన్య)o;
    (శూన్య)i_loop;
    (శూన్య)i_isConditional;
    తిరిగి m3Err_none;
}

స్థిర అంతఃవరుస
M3_ఫలితం రికార్డు_Call (IM3Compilation o, u16 i_అంకె_Results, u16 i_callee_స్థానం, bool i_isReturnCall)
{
    (శూన్య)o;
    (శూన్య)i_అంకె_Results;
    (శూన్య)i_callee_స్థానం;
    (శూన్య)i_isReturnCall;
    తిరిగి m3Err_none;
}

#అయితే_ముగింపు // d_m3HasSnapshots

స్థిర M3_NOINLINE
M3_ఫలితం Ensure_సంకేతం_పుట_అంకె_పంక్తులు (IM3Compilation o, u32 i_అంకె_పంక్తులు)
{
    M3_ఫలితం ఫలితం = m3Err_none;

    i_అంకె_పంక్తులు += 2; // room for Bridge

    ఒకవేళ (M3_UNLIKELY(అంకె_Free_పంక్తులు(o->పుట) < i_అంకె_పంక్తులు)) {
        IM3_సంకేతం_పుట పుట = Acquire_సంకేతం_పుట_With_సామర్థ్యం(o->runtime, i_అంకె_పంక్తులు);

        ఒకవేళ (పుట) {
            m3log(emit, "bridging new code page from: %d %p (free slots: %d) to: %d", o->పుట->info.sequence, GetPC(o), అంకె_Free_పంక్తులు(o->పుట), పుట->info.sequence);
            d_m3Assert(అంకె_Free_పంక్తులు(o->పుట) >= 2);

            Emit_మాట(o->పుట, op_Branch);
            Emit_మాట(o->పుట, Get_పుట_PC(పుట));

            Release_సంకేతం_పుట(o->runtime, o->పుట);

            o->పుట = పుట;
        } లేకపోతే {
            ఫలితం = m3_Err_malloc_Failed_సంకేతం_పుట;
        }
    }

    తిరిగి ఫలితం;
}

// invalidate the pending local.set fold candidate; called wherever code is emitted or a
// branch target captures the current pc (a fold appends an immediate, moving that position)
స్థిర అంతఃవరుస
శూన్య InvalidateFold (IM3Compilation o)
{
#ఒకవేళ d_m3_Fold_సమితి_Local
    o->foldPatchPC = ఖాళీ;
#లేకపోతే
    (శూన్య)o;
#అయితే_ముగింపు
}

స్థిర M3_NOINLINE
M3_ఫలితం EmitOp (IM3Compilation o, IM3Operation i_operation)
{
    M3_ఫలితం ఫలితం = m3Err_none;                                 d_m3Assert (i_operation or Is_దొంతర_Polymorphic (o));

    InvalidateFold(o);

    // it's OK for page to be null; when compile-walking the bytecode without emitting
    ఒకవేళ (o->పుట) {
#ఒకవేళ d_m3EnableOpTracing
        ఒకవేళ (i_operation != op_Dump_దొంతర) {
            o->అంకె_Emits++;
        }
#అయితే_ముగింపు

        // have execution jump to a new page if slots are critically low
        ఫలితం = Ensure_సంకేతం_పుట_అంకె_పంక్తులు(o, d_m3_సంకేతం_పుట_Free_పంక్తులు_పరిమితి);

        ఒకవేళ (M3_LIKELY(not ఫలితం)) {
            ఒకవేళ (d_m3LogEmit) {
                log_emit(o, i_operation);
            }
#ఒకవేళ d_m3_రికార్డు_Backtraces
            Emit_Mapping_నమోదు(o->పుట, (u32)(o->చివరి_Opcode_ప్రారంభం - o->module->wasm_ప్రారంభం));
#అయితే_ముగింపు // d_m3RecordBacktraces
            Emit_మాట(o->పుట, i_operation);
        }
    }

    తిరిగి ఫలితం;
}

// Push an immediate constant into the M3 codestream
స్థిర M3_NOINLINE
శూన్య EmitConstant32 (IM3Compilation o, స్థిరరాశి u32 i_తక్షణ)
{
    ఒకవేళ (o->పుట) {                                                          m3log (emit, "const32: %ud", i_తక్షణ);
        EmitWord32(o->పుట, i_తక్షణ);
    }
}

#ఒకవేళ d_m3HasMemory64
// Takes two lines of the code page where a pointer is 32 bits, so whatever
// reads it back has to step _pc by the same amount - see op_Const64.
స్థిర M3_NOINLINE
శూన్య EmitConstant64 (IM3Compilation o, స్థిరరాశి u64 i_తక్షణ)
{
    ఒకవేళ (o->పుట) {
        EmitWord64(o->పుట, i_తక్షణ);
    }
}
#అయితే_ముగింపు

స్థిర M3_NOINLINE
శూన్య Emit_స్థానం_స్థానభ్రంశం (IM3Compilation o, స్థిరరాశి i32 i_స్థానభ్రంశం)
{
    ఒకవేళ (o->పుట) {                                                          m3log (emit, "slot: [%d]", i_స్థానభ్రంశం);
        EmitWord32(o->పుట, i_స్థానభ్రంశం);
    }
}

స్థిర M3_NOINLINE
pc_t Emit_సూచిక (IM3Compilation o, స్థిరరాశి శూన్య* స్థిరరాశి i_సూచిక)
{
    pc_t ptr = Get_పుట_PC(o->పుట);

    ఒకవేళ (o->పుట) {                                                          m3log (emit, "ptr: %p", i_సూచిక);
        Emit_మాట(o->పుట, i_సూచిక);
    }

    తిరిగి ptr;
}

స్థిర M3_NOINLINE
శూన్య* Reserve_సూచిక (IM3Compilation o)
{
    pc_t ptr = Get_పుట_PC(o->పుట);
    Emit_సూచిక(o, ఖాళీ);
    తిరిగి (శూన్య*)ptr;
}


//-------------------------------------------------------------------------------------------------------------------------

#నిర్వచించు d_indent "     | %s"

// just want less letters and numbers to stare at down the way in the compiler table
#నిర్వచించు i_32    c_m3Type_i32
#నిర్వచించు i_64    c_m3Type_i64
#నిర్వచించు f_32    c_m3Type_f32
#నిర్వచించు f_64    c_m3Type_f64
#నిర్వచించు none    c_m3Type_none
#నిర్వచించు any     (u8)-1

#ఒకవేళ d_m3HasFloat
#  define FPOP(x) x
#లేకపోతే
#  define FPOP(x) ఖాళీ
#అయితే_ముగింపు

// These are indexed by M3ValueType, so every type up to c_m3Type_externref needs
// an entry. A reference is one pointer-sized word, so it moves with the integer
// operation of that width; v128 has no operations at all.
#ఒకవేళ M3_SIZEOF_PTR == 8
#  define REFOP(NAME)  op_##NAME##_i64
#లేకపోతే
#  define REFOP(NAME)  op_##NAME##_i32
#అయితే_ముగింపు

// clang-format off

స్థిర స్థిరరాశి IM3Operation c_preserve_సమితి_స్థానం[] = { ఖాళీ, op_Preserve_సమితి_స్థానం_i32,       op_Preserve_సమితి_స్థానం_i64,
                                                    FPOP(op_Preserve_సమితి_స్థానం_f32), FPOP(op_Preserve_సమితి_స్థానం_f64),
                                                    ఖాళీ, REFOP(Preserve_సమితి_స్థానం),  REFOP(Preserve_సమితి_స్థానం),
                                                    REFOP(Preserve_సమితి_స్థానం),       REFOP(Preserve_సమితి_స్థానం) };
స్థిర స్థిరరాశి IM3Operation c_సమితి_సమితి_Ops[] =       { ఖాళీ, op_సమితి_స్థానం_i32,               op_సమితి_స్థానం_i64,
                                                    FPOP(op_సమితి_స్థానం_f32),         FPOP(op_సమితి_స్థానం_f64),
                                                    ఖాళీ, REFOP(సమితి_స్థానం),         REFOP(సమితి_స్థానం),
                                                    REFOP(సమితి_స్థానం),               REFOP(సమితి_స్థానం) };
స్థిర స్థిరరాశి IM3Operation c_సమితి_Global_Ops[] =    { ఖాళీ, op_సమితి_Global_i32,             op_సమితి_Global_i64,
                                                    FPOP(op_సమితి_Global_f32),       FPOP(op_సమితి_Global_f64),
                                                    ఖాళీ, REFOP(సమితి_Global),       REFOP(సమితి_Global),
                                                    REFOP(సమితి_Global),             REFOP(సమితి_Global) };
స్థిర స్థిరరాశి IM3Operation c_సమితి_రిజిస్టరు_Ops[] =  { ఖాళీ, op_సమితి_రిజిస్టరు_i32,           op_సమితి_రిజిస్టరు_i64,
                                                    FPOP(op_సమితి_రిజిస్టరు_f32),     FPOP(op_సమితి_రిజిస్టరు_f64),
                                                    ఖాళీ, REFOP(సమితి_రిజిస్టరు),     REFOP(సమితి_రిజిస్టరు),
                                                    REFOP(సమితి_రిజిస్టరు),           REFOP(సమితి_రిజిస్టరు) };

// A table shorter than the enum reads out of bounds, so tie the two together at
// build time: adding a value type must not silently outgrow these.
#నిర్వచించు d_m3_Check_Type_పట్టిక(పట్టిక) \
    M3_STATIC_ASSERT (పరిమాణం (పట్టిక) / పరిమాణం (*(పట్టిక)) == c_m3_Type_లెక్క, పట్టిక##_needs_one_entry_per_type)

d_m3_Check_Type_పట్టిక(c_preserve_సమితి_స్థానం);
d_m3_Check_Type_పట్టిక(c_సమితి_సమితి_Ops);
d_m3_Check_Type_పట్టిక(c_సమితి_Global_Ops);
d_m3_Check_Type_పట్టిక(c_సమితి_రిజిస్టరు_Ops);

#ఒకవేళ d_m3_Fold_సమితి_Local

// destination-folded variants of the hot binops, indexed by the operand-form index
// recorded at emission: [0]=_rs [1]=_sr [2]=_ss [3]=(fp _rr, never folded)
#నిర్వచించు d_foldOpList(TYPE, NAME)            { op_##TYPE##_##NAME##_rs_f, op_##TYPE##_##NAME##_sr_f, op_##TYPE##_##NAME##_ss_f, ఖాళీ }
#నిర్వచించు d_foldCommutativeOpList(TYPE, NAME) { op_##TYPE##_##NAME##_rs_f, ఖాళీ,                      op_##TYPE##_##NAME##_ss_f, ఖాళీ }

స్థిర స్థిరరాశి IM3Operation c_fold_i32_కూడు[]         = d_foldCommutativeOpList (i32, కూడు);
స్థిర స్థిరరాశి IM3Operation c_fold_i32_వ్యవకలనం[]    = d_foldOpList (i32, వ్యవకలనం);
స్థిర స్థిరరాశి IM3Operation c_fold_i32_Multiply[]    = d_foldCommutativeOpList (i32, Multiply);
స్థిర స్థిరరాశి IM3Operation c_fold_u32_And[]         = d_foldCommutativeOpList (u32, And);
స్థిర స్థిరరాశి IM3Operation c_fold_u32_Or[]          = d_foldCommutativeOpList (u32, Or);
స్థిర స్థిరరాశి IM3Operation c_fold_u32_Xor[]         = d_foldCommutativeOpList (u32, Xor);
స్థిర స్థిరరాశి IM3Operation c_fold_u32_జరుపు_ఎడమ[]   = d_foldOpList (u32, జరుపు_ఎడమ);
స్థిర స్థిరరాశి IM3Operation c_fold_i32_జరుపు_కుడి[]  = d_foldOpList (i32, జరుపు_కుడి);
స్థిర స్థిరరాశి IM3Operation c_fold_u32_జరుపు_కుడి[]  = d_foldOpList (u32, జరుపు_కుడి);

స్థిర స్థిరరాశి IM3Operation c_fold_i64_కూడు[]         = d_foldCommutativeOpList (i64, కూడు);
స్థిర స్థిరరాశి IM3Operation c_fold_i64_వ్యవకలనం[]    = d_foldOpList (i64, వ్యవకలనం);
స్థిర స్థిరరాశి IM3Operation c_fold_i64_Multiply[]    = d_foldCommutativeOpList (i64, Multiply);
స్థిర స్థిరరాశి IM3Operation c_fold_u64_And[]         = d_foldCommutativeOpList (u64, And);
స్థిర స్థిరరాశి IM3Operation c_fold_u64_Or[]          = d_foldCommutativeOpList (u64, Or);
స్థిర స్థిరరాశి IM3Operation c_fold_u64_Xor[]         = d_foldCommutativeOpList (u64, Xor);
స్థిర స్థిరరాశి IM3Operation c_fold_u64_జరుపు_ఎడమ[]   = d_foldOpList (u64, జరుపు_ఎడమ);
స్థిర స్థిరరాశి IM3Operation c_fold_i64_జరుపు_కుడి[]  = d_foldOpList (i64, జరుపు_కుడి);
స్థిర స్థిరరాశి IM3Operation c_fold_u64_జరుపు_కుడి[]  = d_foldOpList (u64, జరుపు_కుడి);

#  ఒకవేళ d_m3HasFloat
స్థిర స్థిరరాశి IM3Operation c_fold_f32_కూడు[]         = d_foldCommutativeOpList (f32, కూడు);
స్థిర స్థిరరాశి IM3Operation c_fold_f32_వ్యవకలనం[]    = d_foldOpList (f32, వ్యవకలనం);
స్థిర స్థిరరాశి IM3Operation c_fold_f32_Multiply[]    = d_foldCommutativeOpList (f32, Multiply);
స్థిర స్థిరరాశి IM3Operation c_fold_f32_Divide[]      = d_foldOpList (f32, Divide);
స్థిర స్థిరరాశి IM3Operation c_fold_f64_కూడు[]         = d_foldCommutativeOpList (f64, కూడు);
స్థిర స్థిరరాశి IM3Operation c_fold_f64_వ్యవకలనం[]    = d_foldOpList (f64, వ్యవకలనం);
స్థిర స్థిరరాశి IM3Operation c_fold_f64_Multiply[]    = d_foldCommutativeOpList (f64, Multiply);
స్థిర స్థిరరాశి IM3Operation c_fold_f64_Divide[]      = d_foldOpList (f64, Divide);
#  endif

// loads are unary: [0]= address in _r0, [1]= address in a slot
#నిర్వచించు d_fold_ఎక్కించు_List(DEST, SRC)       { op_##DEST##_Load_##SRC##_r_f, op_##DEST##_Load_##SRC##_s_f, ఖాళీ, ఖాళీ }

స్థిర స్థిరరాశి IM3Operation c_fold_i32_ఎక్కించు_i32[] = d_fold_ఎక్కించు_List(i32, i32);
స్థిర స్థిరరాశి IM3Operation c_fold_i32_ఎక్కించు_i8[] = d_fold_ఎక్కించు_List(i32, i8);
స్థిర స్థిరరాశి IM3Operation c_fold_i32_ఎక్కించు_u8[] = d_fold_ఎక్కించు_List(i32, u8);
స్థిర స్థిరరాశి IM3Operation c_fold_i32_ఎక్కించు_i16[] = d_fold_ఎక్కించు_List(i32, i16);
స్థిర స్థిరరాశి IM3Operation c_fold_i32_ఎక్కించు_u16[] = d_fold_ఎక్కించు_List(i32, u16);

స్థిర స్థిరరాశి IM3Operation c_fold_i64_ఎక్కించు_i64[] = d_fold_ఎక్కించు_List(i64, i64);
స్థిర స్థిరరాశి IM3Operation c_fold_i64_ఎక్కించు_i8[] = d_fold_ఎక్కించు_List(i64, i8);
స్థిర స్థిరరాశి IM3Operation c_fold_i64_ఎక్కించు_u8[] = d_fold_ఎక్కించు_List(i64, u8);
స్థిర స్థిరరాశి IM3Operation c_fold_i64_ఎక్కించు_i16[] = d_fold_ఎక్కించు_List(i64, i16);
స్థిర స్థిరరాశి IM3Operation c_fold_i64_ఎక్కించు_u16[] = d_fold_ఎక్కించు_List(i64, u16);
స్థిర స్థిరరాశి IM3Operation c_fold_i64_ఎక్కించు_i32[] = d_fold_ఎక్కించు_List(i64, i32);
స్థిర స్థిరరాశి IM3Operation c_fold_i64_ఎక్కించు_u32[] = d_fold_ఎక్కించు_List(i64, u32);
#  ఒకవేళ d_m3HasFloat
స్థిర స్థిరరాశి IM3Operation c_fold_f32_ఎక్కించు_f32[] = d_fold_ఎక్కించు_List(f32, f32);
స్థిర స్థిరరాశి IM3Operation c_fold_f64_ఎక్కించు_f64[] = d_fold_ఎక్కించు_List(f64, f64);
#  endif

స్థిర
IM3Operation GetFoldOp (m3opcode_t i_opcode, u8 i_form)
{
    ఒకవేళ (i_form >= 4) {
        తిరిగి ఖాళీ;
    }

    ఎంపిక (i_opcode) {
    స్థితి 0x6a:  తిరిగి c_fold_i32_కూడు        [i_form];      // i32.add
    స్థితి 0x6b:  తిరిగి c_fold_i32_వ్యవకలనం   [i_form];      // i32.sub
    స్థితి 0x6c:  తిరిగి c_fold_i32_Multiply   [i_form];      // i32.mul
    స్థితి 0x71:  తిరిగి c_fold_u32_And        [i_form];      // i32.and
    స్థితి 0x72:  తిరిగి c_fold_u32_Or         [i_form];      // i32.or
    స్థితి 0x73:  తిరిగి c_fold_u32_Xor        [i_form];      // i32.xor
    స్థితి 0x74:  తిరిగి c_fold_u32_జరుపు_ఎడమ  [i_form];      // i32.shl
    స్థితి 0x75:  తిరిగి c_fold_i32_జరుపు_కుడి [i_form];      // i32.shr_s
    స్థితి 0x76:  తిరిగి c_fold_u32_జరుపు_కుడి [i_form];      // i32.shr_u

    స్థితి 0x7c:  తిరిగి c_fold_i64_కూడు        [i_form];      // i64.add
    స్థితి 0x7d:  తిరిగి c_fold_i64_వ్యవకలనం   [i_form];      // i64.sub
    స్థితి 0x7e:  తిరిగి c_fold_i64_Multiply   [i_form];      // i64.mul
    స్థితి 0x83:  తిరిగి c_fold_u64_And        [i_form];      // i64.and
    స్థితి 0x84:  తిరిగి c_fold_u64_Or         [i_form];      // i64.or
    స్థితి 0x85:  తిరిగి c_fold_u64_Xor        [i_form];      // i64.xor
    స్థితి 0x86:  తిరిగి c_fold_u64_జరుపు_ఎడమ  [i_form];      // i64.shl
    స్థితి 0x87:  తిరిగి c_fold_i64_జరుపు_కుడి [i_form];      // i64.shr_s
    స్థితి 0x88:  తిరిగి c_fold_u64_జరుపు_కుడి [i_form];      // i64.shr_u

#  ఒకవేళ d_m3HasFloat
    స్థితి 0x92:  తిరిగి c_fold_f32_కూడు        [i_form];      // f32.add
    స్థితి 0x93:  తిరిగి c_fold_f32_వ్యవకలనం   [i_form];      // f32.sub
    స్థితి 0x94:  తిరిగి c_fold_f32_Multiply   [i_form];      // f32.mul
    స్థితి 0x95:  తిరిగి c_fold_f32_Divide     [i_form];      // f32.div
    స్థితి 0xa0:  తిరిగి c_fold_f64_కూడు        [i_form];      // f64.add
    స్థితి 0xa1:  తిరిగి c_fold_f64_వ్యవకలనం   [i_form];      // f64.sub
    స్థితి 0xa2:  తిరిగి c_fold_f64_Multiply   [i_form];      // f64.mul
    స్థితి 0xa3:  తిరిగి c_fold_f64_Divide     [i_form];      // f64.div
#  endif

    స్థితి 0x28:  తిరిగి c_fold_i32_ఎక్కించు_i32   [i_form];      // i32.load
    స్థితి 0x2c:  తిరిగి c_fold_i32_ఎక్కించు_i8    [i_form];      // i32.load8_s
    స్థితి 0x2d:  తిరిగి c_fold_i32_ఎక్కించు_u8    [i_form];      // i32.load8_u
    స్థితి 0x2e:  తిరిగి c_fold_i32_ఎక్కించు_i16   [i_form];      // i32.load16_s
    స్థితి 0x2f:  తిరిగి c_fold_i32_ఎక్కించు_u16   [i_form];      // i32.load16_u

    స్థితి 0x29:  తిరిగి c_fold_i64_ఎక్కించు_i64   [i_form];      // i64.load
    స్థితి 0x30:  తిరిగి c_fold_i64_ఎక్కించు_i8    [i_form];      // i64.load8_s
    స్థితి 0x31:  తిరిగి c_fold_i64_ఎక్కించు_u8    [i_form];      // i64.load8_u
    స్థితి 0x32:  తిరిగి c_fold_i64_ఎక్కించు_i16   [i_form];      // i64.load16_s
    స్థితి 0x33:  తిరిగి c_fold_i64_ఎక్కించు_u16   [i_form];      // i64.load16_u
    స్థితి 0x34:  తిరిగి c_fold_i64_ఎక్కించు_i32   [i_form];      // i64.load32_s
    స్థితి 0x35:  తిరిగి c_fold_i64_ఎక్కించు_u32   [i_form];      // i64.load32_u

#  ఒకవేళ d_m3HasFloat
    స్థితి 0x2a:  తిరిగి c_fold_f32_ఎక్కించు_f32   [i_form];      // f32.load
    స్థితి 0x2b:  తిరిగి c_fold_f64_ఎక్కించు_f64   [i_form];      // f64.load
#  endif

    అప్రమేయ: తిరిగి ఖాళీ;
    }
}

#అయితే_ముగింపు // d_m3FoldSetLocal

// clang-format on

#ఒకవేళ d_m3FuseBranch

// fused compare+branch/if variants, indexed like the fold lists: [0]=_rs [1]=_sr [2]=_ss [3]=unused.
// the [1] entries are NULL for commutative compares: the compiler never records the _sr form for them
#  define d_fuseCmpList(TYPE, NAME, రకం_2)             { op_##TYPE##_##KIND##_##NAME##_rs, op_##TYPE##_##KIND##_##NAME##_sr, op_##TYPE##_##KIND##_##NAME##_ss, ఖాళీ }
#  define d_fuseCommutativeCmpList(TYPE, NAME, రకం_2)  { op_##TYPE##_##KIND##_##NAME##_rs, ఖాళీ,                             op_##TYPE##_##KIND##_##NAME##_ss, ఖాళీ }

రకనిర్వచనం నిర్మాణం M3FusedCmpOps {
    IM3Operation branchIf[4];
    IM3Operation ifOp[4];
} M3FusedCmpOps;

#  define d_fuseCmp(TYPE, NAME)               { d_fuseCmpList (TYPE, NAME, BranchIf),            d_fuseCmpList (TYPE, NAME, If) }
#  define d_fuseCommutativeCmp(TYPE, NAME)    { d_fuseCommutativeCmpList (TYPE, NAME, BranchIf), d_fuseCommutativeCmpList (TYPE, NAME, If) }

// clang-format off
స్థిర స్థిరరాశి M3FusedCmpOps c_fuse_i32_Equal              = d_fuseCommutativeCmp (i32, Equal);
స్థిర స్థిరరాశి M3FusedCmpOps c_fuse_i32_NotEqual           = d_fuseCommutativeCmp (i32, NotEqual);
స్థిర స్థిరరాశి M3FusedCmpOps c_fuse_i32_LessThan           = d_fuseCmp (i32, LessThan);
స్థిర స్థిరరాశి M3FusedCmpOps c_fuse_u32_LessThan           = d_fuseCmp (u32, LessThan);
స్థిర స్థిరరాశి M3FusedCmpOps c_fuse_i32_GreaterThan        = d_fuseCmp (i32, GreaterThan);
స్థిర స్థిరరాశి M3FusedCmpOps c_fuse_u32_GreaterThan        = d_fuseCmp (u32, GreaterThan);
స్థిర స్థిరరాశి M3FusedCmpOps c_fuse_i32_LessThanOrEqual    = d_fuseCmp (i32, LessThanOrEqual);
స్థిర స్థిరరాశి M3FusedCmpOps c_fuse_u32_LessThanOrEqual    = d_fuseCmp (u32, LessThanOrEqual);
స్థిర స్థిరరాశి M3FusedCmpOps c_fuse_i32_GreaterThanOrEqual = d_fuseCmp (i32, GreaterThanOrEqual);
స్థిర స్థిరరాశి M3FusedCmpOps c_fuse_u32_GreaterThanOrEqual = d_fuseCmp (u32, GreaterThanOrEqual);
// clang-format on

// i32.eqz is unary: [0]= operand in _r0, [1]= operand in a slot
స్థిర స్థిరరాశి M3FusedCmpOps c_fuse_i32_Eqz = {
    { op_i32_BranchIfEqz_r, op_i32_BranchIfEqz_s, ఖాళీ, ఖాళీ },
    { op_i32_IfEqz_r,       op_i32_IfEqz_s,       ఖాళీ, ఖాళీ }
};

స్థిర
స్థిరరాశి M3FusedCmpOps* GetFusedCmpOps (m3opcode_t i_opcode)
{
    ఎంపిక (i_opcode) {
    స్థితి 0x45: తిరిగి &c_fuse_i32_Eqz;                    // i32.eqz
    స్థితి 0x46: తిరిగి &c_fuse_i32_Equal;                  // i32.eq
    స్థితి 0x47: తిరిగి &c_fuse_i32_NotEqual;               // i32.ne
    స్థితి 0x48: తిరిగి &c_fuse_i32_LessThan;               // i32.lt_s
    స్థితి 0x49: తిరిగి &c_fuse_u32_LessThan;               // i32.lt_u
    స్థితి 0x4a: తిరిగి &c_fuse_i32_GreaterThan;            // i32.gt_s
    స్థితి 0x4b: తిరిగి &c_fuse_u32_GreaterThan;            // i32.gt_u
    స్థితి 0x4c: తిరిగి &c_fuse_i32_LessThanOrEqual;        // i32.le_s
    స్థితి 0x4d: తిరిగి &c_fuse_u32_LessThanOrEqual;        // i32.le_u
    స్థితి 0x4e: తిరిగి &c_fuse_i32_GreaterThanOrEqual;     // i32.ge_s
    స్థితి 0x4f: తిరిగి &c_fuse_u32_GreaterThanOrEqual;     // i32.ge_u
    అప్రమేయ: తిరిగి ఖాళీ;
    }
}

#అయితే_ముగింపు // d_m3FuseBranch

స్థిర స్థిరరాశి IM3Operation c_intSelectOps[2][4] = {
    { op_Select_i32_rss, op_Select_i32_srs, op_Select_i32_ssr, op_Select_i32_sss },
    { op_Select_i64_rss, op_Select_i64_srs, op_Select_i64_ssr, op_Select_i64_sss }
};
#ఒకవేళ d_m3HasFloat
స్థిర స్థిరరాశి IM3Operation c_fpSelectOps[2][2][3] = {
    { { op_Select_f32_sss, op_Select_f32_srs, op_Select_f32_ssr }, // selector in slot
      { op_Select_f32_rss, op_Select_f32_rrs, op_Select_f32_rsr } }, // selector in reg
    { { op_Select_f64_sss, op_Select_f64_srs, op_Select_f64_ssr }, // selector in slot
      { op_Select_f64_rss, op_Select_f64_rrs, op_Select_f64_rsr } }
};    // selector in reg
#అయితే_ముగింపు

// all args & returns are 64-bit aligned, so use 2 slots for a d_m3Use32BitSlots=1 build

స్థిర
M3_ఫలితం Acquire_Compilation_సంకేతం_పుట (IM3Compilation o, IM3_సంకేతం_పుట* o_సంకేతం_పుట)
{
    M3_ఫలితం ఫలితం = m3Err_none;

    IM3_సంకేతం_పుట పుట = Acquire_సంకేతం_పుట(o->runtime);

    ఒకవేళ (పుట) {
#ఒకవేళ (d_m3_Enable_సంకేతం_పుట_Ref_Counting)
        {
            ఒకవేళ (o->function) {
                IM3Function func = o->function;
                పుట->info.usage_లెక్క++;

                u32 index = func->అంకె_సంకేతం_పుట_Refs++;
_               (m3ReallocArray(&func->సంకేతం_పుట_Refs, IM3_సంకేతం_పుట, func->అంకె_సంకేతం_పుట_Refs, index));
                func->సంకేతం_పుట_Refs[index] = పుట;
            }
        }
#అయితే_ముగింపు
    } లేకపోతే {
        _throw(m3_Err_malloc_Failed_సంకేతం_పుట);
    }

    _catch:

    *o_సంకేతం_పుట = పుట;

    తిరిగి ఫలితం;
}

స్థిర అంతఃవరుస
శూన్య Release_Compilation_సంకేతం_పుట (IM3Compilation o)
{
    Release_సంకేతం_పుట(o->runtime, o->పుట);
}

స్థిర అంతఃవరుస
u16 Get_Type_అంకె_Slots (m3type_t i_type)
{
    i_type = పునాది_Type_యొక్క(i_type);

    // v128 is 16 bytes - 4 slots in 32-bit-slot mode, 2 in 64-bit.
    // (Slot-allocator only; no v128 ops execute.)
    ఒకవేళ (i_type == c_m3Type_v128)
#ఒకవేళ d_m3_Use32_బిట్టు_Slots
        తిరిగి 4;
#లేకపోతే
        తిరిగి 2;
#అయితే_ముగింపు
#ఒకవేళ d_m3_Use32_బిట్టు_Slots
    తిరిగి Is64_బిట్టు_Type(i_type) ? 2 : 1;
#లేకపోతే
    తిరిగి 1;
#అయితే_ముగింపు
}

స్థిర అంతఃవరుస
శూన్య Align_స్థానం_గమ్యం_Type (u16* io_స్థానం, m3type_t i_type)
{
    // align 64-bit words to even slots (if d_m3Use32BitSlots)
    u16 అంకె_Slots = Get_Type_అంకె_Slots(i_type);

    u16 ముసుగు = అంకె_Slots - 1;
    *io_స్థానం = (*io_స్థానం + ముసుగు) & ~ముసుగు;
}

// Deliberately unchecked. Validation rejects an empty operand stack before compilation.
స్థిర అంతఃవరుస
i16 Get_దొంతర_శిఖరం_సూచీ (IM3Compilation o)
{
    తిరిగి o->దొంతర_సూచీ - 1;
}

స్థిర అంతఃవరుస
M3_ఫలితం Get_దొంతర_శిఖరం_సూచీ_Throws (IM3Compilation o, i16* o_దొంతర_సూచీ)
{
    *o_దొంతర_సూచీ = o->దొంతర_సూచీ - 1;

    ఒకవేళ (M3_LIKELY(o->దొంతర_సూచీ > o->దొంతర_మొదటి_Dynamic_సూచీ or Is_దొంతర_Polymorphic(o))) {
        తిరిగి m3Err_none;
    } లేకపోతే {
        తిరిగి m3_Err_function_దొంతర_Underrun;
    }
}


// Items in the static portion of the stack (args/locals) are hidden from GetStackTypeFromTop ()
// In other words, only "real" Wasm stack items can be inspected.  This is important when
// returning values, etc. and you need an accurate wasm-view of the stack.
స్థిర
m3type_t Get_దొంతర_Type_From_శిఖరం (IM3Compilation o, u16 i_స్థానభ్రంశం)
{
    m3type_t type = c_m3Type_none;

    ++i_స్థానభ్రంశం;
    ఒకవేళ (o->దొంతర_సూచీ >= i_స్థానభ్రంశం) {
        u16 index = o->దొంతర_సూచీ - i_స్థానభ్రంశం;

        ఒకవేళ (index >= o->దొంతర_మొదటి_Dynamic_సూచీ) {
            type = o->type_దొంతర[index];
        }
    }

    తిరిగి type;
}

స్థిర అంతఃవరుస
m3type_t Get_దొంతర_శిఖరం_Type (IM3Compilation o)
{
    తిరిగి Get_దొంతర_Type_From_శిఖరం(o, 0);
}

స్థిర అంతఃవరుస
m3type_t Get_దొంతర_Type_From_Bottom (IM3Compilation o, u16 i_స్థానభ్రంశం)
{
    m3type_t type = c_m3Type_none;

    ఒకవేళ (i_స్థానభ్రంశం < o->దొంతర_సూచీ) {
        type = o->type_దొంతర[i_స్థానభ్రంశం];
    }

    తిరిగి type;
}


స్థిర అంతఃవరుస
bool Is_Constant_స్థానం (IM3Compilation o, u16 i_స్థానం)
{
    తిరిగి (i_స్థానం >= o->స్థానం_మొదటి_Const_సూచీ and i_స్థానం < o->స్థానం_Max_Const_సూచీ);
}
స్థిర అంతఃవరుస
bool Is_స్థానం_Allocated (IM3Compilation o, u16 i_స్థానం)
{
    తిరిగి o->m3Slots[i_స్థానం];
}

స్థిర అంతఃవరుస
bool Is_దొంతర_సూచీ_లోపల_రిజిస్టరు (IM3Compilation o, i32 i_దొంతర_సూచీ)
{                                                                           d_m3Assert (i_దొంతర_సూచీ < o->దొంతర_సూచీ or Is_దొంతర_Polymorphic (o));
    ఒకవేళ (i_దొంతర_సూచీ >= 0 and i_దొంతర_సూచీ < o->దొంతర_సూచీ) {
        తిరిగి (o->wasm_దొంతర[i_దొంతర_సూచీ] >= d_m3_Reg0_స్థానం_Alias);
    } లేకపోతే {
        తిరిగి false;
    }
}

స్థిర అంతఃవరుస
u16 Get_అంకె_ఖండం_విలువలు_On_దొంతర (IM3Compilation o)
{
    తిరిగి o->దొంతర_సూచీ - o->ఖండం.ఖండం_దొంతర_సూచీ;
}

స్థిర అంతఃవరుస
bool Is_దొంతర_శిఖరం_లోపల_రిజిస్టరు (IM3Compilation o)
{
    తిరిగి Is_దొంతర_సూచీ_లోపల_రిజిస్టరు(o, (i32)Get_దొంతర_శిఖరం_సూచీ(o));
}
స్థిర అంతఃవరుస
bool Is_దొంతర_శిఖరం_Minus1_లోపల_రిజిస్టరు (IM3Compilation o)
{
    తిరిగి Is_దొంతర_సూచీ_లోపల_రిజిస్టరు(o, (i32)Get_దొంతర_శిఖరం_సూచీ(o) - 1);
}
స్థిర అంతఃవరుస
bool Is_దొంతర_శిఖరం_Minus2_లోపల_రిజిస్టరు (IM3Compilation o)
{
    తిరిగి Is_దొంతర_సూచీ_లోపల_రిజిస్టరు(o, (i32)Get_దొంతర_శిఖరం_సూచీ(o) - 2);
}

స్థిర అంతఃవరుస
bool Is_దొంతర_శిఖరం_లోపల_స్థానం (IM3Compilation o)
{
    తిరిగి not Is_దొంతర_శిఖరం_లోపల_రిజిస్టరు(o);
}

స్థిర అంతఃవరుస
bool Is_Valid_స్థానం (u16 i_స్థానం)
{
    తిరిగి (i_స్థానం < d_m3MaxFunctionSlots);
}

స్థిర అంతఃవరుస
u16 Get_దొంతర_శిఖరం_స్థానం_సంఖ్య (IM3Compilation o)
{
    i16 i = Get_దొంతర_శిఖరం_సూచీ(o);

    u16 స్థానం = c_స్థానం_వాడని;

    ఒకవేళ (i >= 0) {
        స్థానం = o->wasm_దొంతర[i];
    }

    తిరిగి స్థానం;
}


// from bottom
స్థిర అంతఃవరుస
u16 Get_స్థానం_For_దొంతర_సూచీ (IM3Compilation o, u16 i_దొంతర_సూచీ)
{                                                                   d_m3Assert (i_దొంతర_సూచీ < o->దొంతర_సూచీ or Is_దొంతర_Polymorphic (o));
    u16 స్థానం = c_స్థానం_వాడని;

    ఒకవేళ (i_దొంతర_సూచీ < o->దొంతర_సూచీ) {
        స్థానం = o->wasm_దొంతర[i_దొంతర_సూచీ];
    }

    తిరిగి స్థానం;
}

స్థిర అంతఃవరుస
u16 Get_Extra_స్థానం_For_దొంతర_సూచీ (IM3Compilation o, u16 i_దొంతర_సూచీ)
{
    u16 పునాది_స్థానం = Get_స్థానం_For_దొంతర_సూచీ(o, i_దొంతర_సూచీ);

    ఒకవేళ (పునాది_స్థానం != c_స్థానం_వాడని) {
        u16 extra_స్థానం = Get_Type_అంకె_Slots(Get_దొంతర_Type_From_Bottom(o, i_దొంతర_సూచీ)) - 1;
        పునాది_స్థానం += extra_స్థానం;
    }

    తిరిగి పునాది_స్థానం;
}


స్థిర అంతఃవరుస
శూన్య Touch_స్థానం (IM3Compilation o, u16 i_స్థానం)
{
    // op_Entry uses this value to track and detect stack overflow
    o->max_దొంతర_Slots = M3_MAX(o->max_దొంతర_Slots, i_స్థానం + 1);
}

స్థిర అంతఃవరుస
శూన్య Mark_స్థానం_Allocated (IM3Compilation o, u16 i_స్థానం)
{                                                                   d_m3Assert (o->m3Slots [i_స్థానం] == 0); // shouldn't be already allocated
    o->m3Slots[i_స్థానం] = 1;

    o->స్థానం_Max_Allocated_సూచీ_Plus_One = M3_MAX(o->స్థానం_Max_Allocated_సూచీ_Plus_One, i_స్థానం + 1);

    Touch_స్థానం(o, i_స్థానం);
}

స్థిర అంతఃవరుస
M3_ఫలితం MarkSlotsAllocated (IM3Compilation o, u16 i_స్థానం, u16 i_అంకె_Slots)
{
    ఒకవేళ (M3_UNLIKELY(i_స్థానం + i_అంకె_Slots > d_m3MaxFunctionSlots)) {
        తిరిగి m3_Err_function_దొంతర_అధికప్రవాహం;
    }

    అయితే (i_అంకె_Slots--) {
        Mark_స్థానం_Allocated(o, i_స్థానం++);
    }

    తిరిగి m3Err_none;
}

స్థిర అంతఃవరుస
M3_ఫలితం MarkSlotsAllocatedByType (IM3Compilation o, u16 i_స్థానం, m3type_t i_type)
{
    u16 అంకె_Slots = Get_Type_అంకె_Slots(i_type);
    తిరిగి MarkSlotsAllocated(o, i_స్థానం, అంకె_Slots);
}


స్థిర
M3_ఫలితం AllocateSlotsWithinRange (IM3Compilation o, u16* o_స్థానం, m3type_t i_type, u16 i_ప్రారంభం_స్థానం, u16 i_చివర_స్థానం)
{
    M3_ఫలితం ఫలితం = m3_Err_function_దొంతర_అధికప్రవాహం;

    u16 అంకె_Slots     = Get_Type_అంకె_Slots(i_type);
    u16 శోధన_స్థానభ్రంశం = అంకె_Slots - 1;

    Align_స్థానం_గమ్యం_Type(&i_ప్రారంభం_స్థానం, i_type);

    // search for 1 or 2 consecutive slots in the execution stack
    u16 i = i_ప్రారంభం_స్థానం;
    అయితే (i + శోధన_స్థానభ్రంశం < i_చివర_స్థానం) {
        ఒకవేళ (i + శోధన_స్థానభ్రంశం < d_m3MaxFunctionSlots and o->m3Slots[i] == 0 and o->m3Slots[i + శోధన_స్థానభ్రంశం] == 0) {
            MarkSlotsAllocated(o, i, అంకె_Slots);

            *o_స్థానం = i;
            ఫలితం  = m3Err_none;
            ఆపు;
        }

        // keep 2-slot allocations even-aligned
        i += అంకె_Slots;
    }

    తిరిగి ఫలితం;
}

స్థిర అంతఃవరుస
M3_ఫలితం AllocateSlots (IM3Compilation o, u16* o_స్థానం, m3type_t i_type)
{
    తిరిగి AllocateSlotsWithinRange(o, o_స్థానం, i_type, o->స్థానం_మొదటి_Dynamic_సూచీ, d_m3MaxFunctionSlots);
}

స్థిర అంతఃవరుస
M3_ఫలితం AllocateConstantSlots (IM3Compilation o, u16* o_స్థానం, m3type_t i_type)
{
    u16 max_పట్టిక_సూచీ = o->స్థానం_మొదటి_Const_సూచీ + d_m3_Max_Constant_పట్టిక_కొలత;
    తిరిగి AllocateSlotsWithinRange(o, o_స్థానం, i_type, o->స్థానం_మొదటి_Const_సూచీ, M3_MIN(o->స్థానం_మొదటి_Dynamic_సూచీ, max_పట్టిక_సూచీ));
}


// TOQUE: this usage count system could be eliminated. real world code doesn't frequently trigger it.  just copy to multiple
// unique slots.
స్థిర అంతఃవరుస
M3_ఫలితం Increment_స్థానం_Usage_లెక్క (IM3Compilation o, u16 i_స్థానం)
{                                                                                       d_m3Assert (i_స్థానం < d_m3MaxFunctionSlots);
    M3_ఫలితం ఫలితం = m3Err_none;                                                       d_m3Assert (o->m3Slots [i_స్థానం] > 0);

    // OPTZ (memory): 'm3Slots' could still be fused with 'typeStack' if 4 bits were used to indicate: [0,1,2,many]. The many-case
    // would scan 'wasmStack' to determine the actual usage count
    ఒకవేళ (M3_LIKELY(o->m3Slots[i_స్థానం] < 0xFF)) {
        o->m3Slots[i_స్థానం]++;
    } లేకపోతే {
        ఫలితం = "slot usage count overflow";
    }

    తిరిగి ఫలితం;
}

స్థిర అంతఃవరుస
శూన్య Deallocate_స్థానం (IM3Compilation o, i16 i_స్థానం, m3type_t i_type)
{                                                                                       d_m3Assert (i_స్థానం >= o->స్థానం_మొదటి_Dynamic_సూచీ);
                                                                                        d_m3Assert (i_స్థానం < o->స్థానం_Max_Allocated_సూచీ_Plus_One);
    కొరకు (u16 i = 0; i < Get_Type_అంకె_Slots(i_type); ++i, ++i_స్థానం) {                       d_m3Assert (o->m3Slots [i_స్థానం]);
        --o->m3Slots[i_స్థానం];
    }
}


స్థిర అంతఃవరుస
bool Is_రిజిస్టరు_Type_Allocated (IM3Compilation o, u8 i_type)
{
    తిరిగి Is_రిజిస్టరు_Allocated(o, IsFpType(i_type));
}

స్థిర అంతఃవరుస
శూన్య Allocate_రిజిస్టరు (IM3Compilation o, u32 i_రిజిస్టరు, u16 i_దొంతర_సూచీ)
{                                                                                       d_m3Assert (not Is_రిజిస్టరు_Allocated (o, i_రిజిస్టరు));
    o->reg_దొంతర_సూచీ_Plus_One[i_రిజిస్టరు] = i_దొంతర_సూచీ + 1;
}

స్థిర అంతఃవరుస
శూన్య Deallocate_రిజిస్టరు (IM3Compilation o, u32 i_రిజిస్టరు)
{                                                                                       d_m3Assert (Is_రిజిస్టరు_Allocated (o, i_రిజిస్టరు));
    o->reg_దొంతర_సూచీ_Plus_One[i_రిజిస్టరు] = c_m3_రిజిస్టరు_Unallocated;
}

స్థిర అంతఃవరుస
u16 Get_రిజిస్టరు_దొంతర_సూచీ (IM3Compilation o, u32 i_రిజిస్టరు)
{                                                                                       d_m3Assert (Is_రిజిస్టరు_Allocated (o, i_రిజిస్టరు));
    తిరిగి o->reg_దొంతర_సూచీ_Plus_One[i_రిజిస్టరు] - 1;
}

u16 Get_Max_వాడిన_స్థానం_Plus_One (IM3Compilation o)
{
    అయితే (o->స్థానం_Max_Allocated_సూచీ_Plus_One > o->స్థానం_మొదటి_Dynamic_సూచీ) {
        ఒకవేళ (Is_స్థానం_Allocated(o, o->స్థానం_Max_Allocated_సూచీ_Plus_One - 1)) {
            ఆపు;
        }

        o->స్థానం_Max_Allocated_సూచీ_Plus_One--;
    }

#నిర్వచితమైతే DEBUG
    u16 max_స్థానం = o->స్థానం_Max_Allocated_సూచీ_Plus_One;
    అయితే (max_స్థానం < d_m3MaxFunctionSlots) {
        d_m3Assert(o->m3Slots[max_స్థానం] == 0);
        max_స్థానం++;
    }
#అయితే_ముగింపు

    తిరిగి o->స్థానం_Max_Allocated_సూచీ_Plus_One;
}

స్థిర
M3_ఫలితం Preserve_రిజిస్టరు_If_Occupied (IM3Compilation o, m3type_t i_రిజిస్టరు_Type)
{
    M3_ఫలితం ఫలితం = m3Err_none;

    u32 regSelect = IsFpType(i_రిజిస్టరు_Type);

    ఒకవేళ (Is_రిజిస్టరు_Allocated(o, regSelect)) {
        u16 దొంతర_సూచీ = Get_రిజిస్టరు_దొంతర_సూచీ(o, regSelect);
        Deallocate_రిజిస్టరు(o, regSelect);

        m3type_t type = Get_దొంతర_Type_From_Bottom(o, దొంతర_సూచీ);

        // and point to a exec slot
        u16 స్థానం = c_స్థానం_వాడని;
_       (AllocateSlots(o, &స్థానం, type));
        o->wasm_దొంతర[దొంతర_సూచీ] = స్థానం;

        // Ensure type is within the valid range
        ఒకవేళ (M3_LIKELY(పునాది_Type_యొక్క(type) < c_m3_Type_లెక్క)) {
_           (EmitOp(o, c_సమితి_సమితి_Ops[పునాది_Type_యొక్క(type)]));
        } లేకపోతే {
            _throw(m3Err_unknownType);
        }

        Emit_స్థానం_స్థానభ్రంశం(o, స్థానం);
    }

    _catch: తిరిగి ఫలితం;
}


// all values must be in slots before entering loop, if, and else blocks
// otherwise they'd end up preserve-copied in the block to probably different locations (if/else)
స్థిర అంతఃవరుస
M3_ఫలితం PreserveRegisters (IM3Compilation o)
{
    M3_ఫలితం ఫలితం;

_   (Preserve_రిజిస్టరు_If_Occupied(o, c_m3Type_f64));
_   (Preserve_రిజిస్టరు_If_Occupied(o, c_m3Type_i64));

    _catch: తిరిగి ఫలితం;
}

స్థిర
M3_ఫలితం Preserve_Non_శిఖరం_Registers (IM3Compilation o)
{
    M3_ఫలితం ఫలితం = m3Err_none;

    i16 దొంతర_శిఖరం = Get_దొంతర_శిఖరం_సూచీ(o);

    ఒకవేళ (దొంతర_శిఖరం >= 0) {
        ఒకవేళ (Is_రిజిస్టరు_Allocated(o, 0))     // r0
        {
            ఒకవేళ (Get_రిజిస్టరు_దొంతర_సూచీ(o, 0) != దొంతర_శిఖరం) {
_               (Preserve_రిజిస్టరు_If_Occupied(o, c_m3Type_i64));
            }
        }

        ఒకవేళ (Is_రిజిస్టరు_Allocated(o, 1))     // fp0
        {
            ఒకవేళ (Get_రిజిస్టరు_దొంతర_సూచీ(o, 1) != దొంతర_శిఖరం) {
_               (Preserve_రిజిస్టరు_If_Occupied(o, c_m3Type_f64));
            }
        }
    }

    _catch: తిరిగి ఫలితం;
}


//----------------------------------------------------------------------------------------------------------------------

స్థిర
M3_ఫలితం నెట్టు (IM3Compilation o, m3type_t i_type, u16 i_స్థానం)
{
    M3_ఫలితం ఫలితం = m3Err_none;

#ఒకవేళ !d_m3HasFloat
    ఒకవేళ (i_type == c_m3Type_f32 || i_type == c_m3Type_f64) {
        తిరిగి m3Err_unknownOpcode;
    }
#అయితే_ముగింపు

    ఒకవేళ (M3_UNLIKELY(o->దొంతర_సూచీ >= d_m3_Max_Function_దొంతర_ఎత్తు)) {
        తిరిగి m3_Err_function_దొంతర_అధికప్రవాహం;
    }

    u16 దొంతర_సూచీ = o->దొంతర_సూచీ++;                                       // printf ("push: %d\n", (i32) i);

    o->wasm_దొంతర[దొంతర_సూచీ] = i_స్థానం;
    o->type_దొంతర[దొంతర_సూచీ] = i_type;

    ఒకవేళ (Is_రిజిస్టరు_స్థానం_Alias(i_స్థానం)) {
        u32 regSelect = Is_Fp_రిజిస్టరు_స్థానం_Alias(i_స్థానం);
        Allocate_రిజిస్టరు(o, regSelect, దొంతర_సూచీ);
    }

    ఒకవేళ (d_m3_Log_Wasm_దొంతర) {
        dump_type_దొంతర(o);
    }

    తిరిగి ఫలితం;
}

స్థిర అంతఃవరుస
M3_ఫలితం నెట్టు_రిజిస్టరు (IM3Compilation o, m3type_t i_type)
{
    M3_ఫలితం ఫలితం = m3Err_none;                                                       d_m3Assert ((u16) d_m3_Reg0_స్థానం_Alias > (u16) d_m3MaxFunctionSlots);
    u16      స్థానం   = IsFpType(i_type) ? d_m3_Fp0_స్థానం_Alias : d_m3_Reg0_స్థానం_Alias;          d_m3Assert (i_type or Is_దొంతర_Polymorphic (o));

_   (నెట్టు(o, i_type, స్థానం));

    _catch: తిరిగి ఫలితం;
}

స్థిర
M3_ఫలితం తీయి (IM3Compilation o)
{
    M3_ఫలితం ఫలితం = m3Err_none;

    ఒకవేళ (M3_LIKELY(o->దొంతర_సూచీ > o->ఖండం.ఖండం_దొంతర_సూచీ)) {
        o->దొంతర_సూచీ--;                                                //  printf ("pop: %d\n", (i32) o->stackIndex);

        u16      స్థానం = o->wasm_దొంతర[o->దొంతర_సూచీ];
        m3type_t type = o->type_దొంతర[o->దొంతర_సూచీ];

        ఒకవేళ (Is_రిజిస్టరు_స్థానం_Alias(స్థానం)) {
            u32 regSelect = Is_Fp_రిజిస్టరు_స్థానం_Alias(స్థానం);
            Deallocate_రిజిస్టరు(o, regSelect);
        } లేకపోతే ఒకవేళ (M3_UNLIKELY(స్థానం >= o->స్థానం_Max_Allocated_సూచీ_Plus_One)) {
            తిరిగి m3_Err_function_దొంతర_Underrun; // Return error for invalid slot indices
        } లేకపోతే ఒకవేళ (స్థానం >= o->స్థానం_మొదటి_Dynamic_సూచీ) {
            Deallocate_స్థానం(o, స్థానం, type);
        }
    } లేకపోతే ఒకవేళ (not Is_దొంతర_Polymorphic(o)) {
        ఫలితం = m3_Err_function_దొంతర_Underrun;
    }

    తిరిగి ఫలితం;
}

స్థిర
M3_ఫలితం తీయి_Type (IM3Compilation o, m3type_t i_type)
{
    M3_ఫలితం ఫలితం = m3Err_none;

    m3type_t శిఖరం_Type = Get_దొంతర_శిఖరం_Type(o);

    ఒకవేళ (M3_LIKELY(Is_Sub_Type_యొక్క(శిఖరం_Type, i_type) or o->ఖండం.isPolymorphic)) {
_       (తీయి(o));
    } లేకపోతే {
        _throw(m3Err_typeMismatch);
    }

    _catch:
    తిరిగి ఫలితం;
}

స్థిర
M3_ఫలితం నెట్టు_Allocated_స్థానం_And_Emit (IM3Compilation o, m3type_t i_type, bool i_doEmit)
{
    M3_ఫలితం ఫలితం = m3Err_none;

    u16 స్థానం = c_స్థానం_వాడని;

_   (AllocateSlots(o, &స్థానం, i_type));
_   (నెట్టు(o, i_type, స్థానం));

    ఒకవేళ (i_doEmit) {
        Emit_స్థానం_స్థానభ్రంశం(o, స్థానం);
    }

    //    printf ("push: %d\n", (u32) slot);

    _catch: తిరిగి ఫలితం;
}

స్థిర అంతఃవరుస
M3_ఫలితం నెట్టు_Allocated_స్థానం_And_Emit_2 (IM3Compilation o, m3type_t i_type)
{
    తిరిగి నెట్టు_Allocated_స్థానం_And_Emit(o, i_type, true);
}

స్థిర అంతఃవరుస
M3_ఫలితం నెట్టు_Allocated_స్థానం (IM3Compilation o, m3type_t i_type)
{
    తిరిగి నెట్టు_Allocated_స్థానం_And_Emit(o, i_type, false);
}

స్థిర
M3_ఫలితం నెట్టు_Const (IM3Compilation o, u64 i_మాట, m3type_t i_type)
{
    M3_ఫలితం ఫలితం = m3Err_none;

    // When compile-walking a constant expression without emitting there is no
    // constant table to place the value in, but the type still has to land on
    // the stack: extended-const arithmetic downstream consumes it.
    ఒకవేళ (M3_UNLIKELY(!o->పుట)) {
        తిరిగి నెట్టు_Allocated_స్థానం(o, i_type);
    }

    bool సరిపోలిక_దొరికింది  = false;
    bool is64_బిట్టు_Type = Is64_బిట్టు_Type(i_type);

    u16 అంకె_Required_Slots  = Get_Type_అంకె_Slots(i_type);
    u16 అంకె_వాడిన_Const_Slots = o->స్థానం_Max_Const_సూచీ - o->స్థానం_మొదటి_Const_సూచీ;

    // search for duplicate matching constant slot to reuse
    ఒకవేళ (అంకె_Required_Slots == 2 and అంకె_వాడిన_Const_Slots >= 2) {
        u16 మొదటి_Const_స్థానం = o->స్థానం_మొదటి_Const_సూచీ;
        Align_స్థానం_గమ్యం_Type(&మొదటి_Const_స్థానం, c_m3Type_i64);

        కొరకు (u16 స్థానం = మొదటి_Const_స్థానం; స్థానం < o->స్థానం_Max_Const_సూచీ - 1; స్థానం += 2) {
            ఒకవేళ (Is_స్థానం_Allocated(o, స్థానం) and Is_స్థానం_Allocated(o, స్థానం + 1)) {
                u64 constant;
                జ్ఞాపకం_నకలు(&constant, &o->constants[స్థానం - o->స్థానం_మొదటి_Const_సూచీ], పరిమాణం(constant));

                ఒకవేళ (constant == i_మాట) {
                    సరిపోలిక_దొరికింది = true;
_                   (నెట్టు(o, i_type, స్థానం));
                    ఆపు;
                }
            }
        }
    } లేకపోతే ఒకవేళ (అంకె_Required_Slots == 1) {
        కొరకు (u16 i = 0; i < అంకె_వాడిన_Const_Slots; ++i) {
            u16 స్థానం = o->స్థానం_మొదటి_Const_సూచీ + i;

            ఒకవేళ (Is_స్థానం_Allocated(o, స్థానం)) {
                bool matches;
                ఒకవేళ (is64_బిట్టు_Type) {
                    u64 constant;
                    జ్ఞాపకం_నకలు(&constant, &o->constants[i], పరిమాణం(constant));
                    matches = (constant == i_మాట);
                } లేకపోతే {
                    u32 constant;
                    జ్ఞాపకం_నకలు(&constant, &o->constants[i], పరిమాణం(constant));
                    matches = (constant == i_మాట);
                }
                ఒకవేళ (matches) {
                    సరిపోలిక_దొరికింది = true;
_                   (నెట్టు(o, i_type, స్థానం));
                    ఆపు;
                }
            }
        }
    }

    ఒకవేళ (not సరిపోలిక_దొరికింది) {
        u16 స్థానం = c_స్థానం_వాడని;
        ఫలితం   = AllocateConstantSlots(o, &స్థానం, i_type);

        ఒకవేళ (ఫలితం || స్థానం == c_స్థానం_వాడని) // no more constant table space; use inline constants
        {
            ఫలితం = m3Err_none;

            ఒకవేళ (is64_బిట్టు_Type) {
_               (EmitOp(o, op_Const64));
                EmitWord64(o->పుట, i_మాట);
            } లేకపోతే {
_               (EmitOp(o, op_Const32));
                EmitWord32(o->పుట, (u32)i_మాట);
            }

_           (నెట్టు_Allocated_స్థానం_And_Emit_2(o, i_type));
        } లేకపోతే {
            u16 const_పట్టిక_సూచీ = స్థానం - o->స్థానం_మొదటి_Const_సూచీ;

            d_m3Assert(const_పట్టిక_సూచీ < d_m3_Max_Constant_పట్టిక_కొలత);

            ఒకవేళ (is64_బిట్టు_Type) {
                జ్ఞాపకం_నకలు(&o->constants[const_పట్టిక_సూచీ], &i_మాట, పరిమాణం(i_మాట));
            } లేకపోతే {
                u32 word32 = (u32)i_మాట;
                జ్ఞాపకం_నకలు(&o->constants[const_పట్టిక_సూచీ], &word32, పరిమాణం(word32));
            }

_           (నెట్టు(o, i_type, స్థానం));

            o->స్థానం_Max_Const_సూచీ = M3_MAX(స్థానం + అంకె_Required_Slots, o->స్థానం_Max_Const_సూచీ);
        }
    }

    _catch: తిరిగి ఫలితం;
}

స్థిర అంతఃవరుస
M3_ఫలితం Emit_స్థానం_అంకె_యొక్క_దొంతర_శిఖరం_And_తీయి (IM3Compilation o)
{
    // no emit if value is in register
    ఒకవేళ (Is_దొంతర_శిఖరం_లోపల_స్థానం(o)) {
        Emit_స్థానం_స్థానభ్రంశం(o, Get_దొంతర_శిఖరం_స్థానం_సంఖ్య(o));
    }

    తిరిగి తీయి(o);
}


// Or, maybe: EmitTrappingOp
M3_ఫలితం కూడు_Trap_రికార్డు (IM3Compilation o)
{
    M3_ఫలితం ఫలితం = m3Err_none;

    ఒకవేళ (o->function) {
    }

    తిరిగి ఫలితం;
}

స్థిర
M3_ఫలితం Unwind_ఖండం_దొంతర (IM3Compilation o)
{
    M3_ఫలితం ఫలితం = m3Err_none;

    u32 తీయి_లెక్క = 0;
    అయితే (o->దొంతర_సూచీ > o->ఖండం.ఖండం_దొంతర_సూచీ) {
_       (తీయి(o));
        ++తీయి_లెక్క;
    }

    ఒకవేళ (తీయి_లెక్క) {
        m3log(compile, "unwound stack top: %d", తీయి_లెక్క);
    }

    _catch: తిరిగి ఫలితం;
}

స్థిర అంతఃవరుస
M3_ఫలితం సమితి_దొంతర_Polymorphic (IM3Compilation o)
{
    o->ఖండం.isPolymorphic = true;                              m3log (compile, "stack set polymorphic");
    తిరిగి Unwind_ఖండం_దొంతర(o);
}

స్థిర
శూన్య PatchBranches (IM3Compilation o)
{
    InvalidateFold(o);         // patched branches land at the current pc

    pc_t pc = GetPC(o);

    pc_t patches     = o->ఖండం.patches;
    o->ఖండం.patches = ఖాళీ;

    అయితే (patches) {                                           m3log (compile, "patching location: %p to pc: %p", patches, pc);
        pc_t తదుపరి       = *(pc_t*)patches;
        *(pc_t*)patches = pc;
        patches         = తదుపరి;
    }
}

//-------------------------------------------------------------------------------------------------------------------------

స్థిర
M3_ఫలితం Copy_దొంతర_సూచీ_గమ్యం_స్థానం (IM3Compilation o, u16 i_dest_స్థానం, u16 i_దొంతర_సూచీ)  // NoPushPop
{
    M3_ఫలితం ఫలితం = m3Err_none;

    IM3Operation op;

    m3type_t type       = Get_దొంతర_Type_From_Bottom(o, i_దొంతర_సూచీ);
    bool     లోపల_రిజిస్టరు = Is_దొంతర_సూచీ_లోపల_రిజిస్టరు(o, i_దొంతర_సూచీ);

    ఒకవేళ (లోపల_రిజిస్టరు) {
        op = c_సమితి_సమితి_Ops[పునాది_Type_యొక్క(type)];
    } లేకపోతే {
        op = Is64_బిట్టు_Type(type) ? op_Copy_స్థానం_64 : op_Copy_స్థానం_32;
    }

_   (EmitOp(o, op));
    Emit_స్థానం_స్థానభ్రంశం(o, i_dest_స్థానం);

    ఒకవేళ (not లోపల_రిజిస్టరు) {
        u16 src_స్థానం = Get_స్థానం_For_దొంతర_సూచీ(o, i_దొంతర_సూచీ);
        Emit_స్థానం_స్థానభ్రంశం(o, src_స్థానం);
    }

    _catch: తిరిగి ఫలితం;
}

స్థిర
M3_ఫలితం Copy_దొంతర_శిఖరం_గమ్యం_స్థానం (IM3Compilation o, u16 i_dest_స్థానం)  // NoPushPop
{
    M3_ఫలితం ఫలితం;

    i16 దొంతర_శిఖరం;
_   (Get_దొంతర_శిఖరం_సూచీ_Throws(o, &దొంతర_శిఖరం));
_   (Copy_దొంతర_సూచీ_గమ్యం_స్థానం(o, i_dest_స్థానం, (u16)దొంతర_శిఖరం));

    _catch: తిరిగి ఫలితం;
}


// a copy-on-write strategy is used with locals. when a get local occurs, it's not copied anywhere. the stack
// entry just has a index pointer to that local memory slot.
// then, when a previously referenced local is set, the current value needs to be preserved for those references

// TODO: consider getting rid of these specialized operations: PreserveSetSlot & PreserveCopySlot.
// They likely just take up space (which seems to reduce performance) without improving performance.
// => unconfirmed in preliminary testing.
స్థిర
M3_ఫలితం Preserved_Copy_శిఖరం_స్థానం (IM3Compilation o, u16 i_dest_స్థానం, u16 i_preserve_స్థానం)
{
    M3_ఫలితం ఫలితం = m3Err_none;             d_m3Assert (i_dest_స్థానం != i_preserve_స్థానం);

    IM3Operation op;

    m3type_t type = Get_దొంతర_శిఖరం_Type(o);

    ఒకవేళ (Is_దొంతర_శిఖరం_లోపల_రిజిస్టరు(o)) {
        op = c_preserve_సమితి_స్థానం[పునాది_Type_యొక్క(type)];
    } లేకపోతే {
        op = Is64_బిట్టు_Type(type) ? op_Preserve_Copy_స్థానం_64 : op_Preserve_Copy_స్థానం_32;
    }

_   (EmitOp(o, op));
    Emit_స్థానం_స్థానభ్రంశం(o, i_dest_స్థానం);

    ఒకవేళ (Is_దొంతర_శిఖరం_లోపల_స్థానం(o)) {
        Emit_స్థానం_స్థానభ్రంశం(o, Get_దొంతర_శిఖరం_స్థానం_సంఖ్య(o));
    }

    Emit_స్థానం_స్థానభ్రంశం(o, i_preserve_స్థానం);

    _catch: తిరిగి ఫలితం;
}

స్థిర
M3_ఫలితం Copy_దొంతర_శిఖరం_గమ్యం_రిజిస్టరు (IM3Compilation o, bool i_update_దొంతర)
{
    M3_ఫలితం ఫలితం = m3Err_none;

    ఒకవేళ (Is_దొంతర_శిఖరం_లోపల_స్థానం(o)) {
        m3type_t type = Get_దొంతర_శిఖరం_Type(o);

_       (Preserve_రిజిస్టరు_If_Occupied(o, type));

        IM3Operation op = c_సమితి_రిజిస్టరు_Ops[పునాది_Type_యొక్క(type)];

_       (EmitOp(o, op));
        Emit_స్థానం_స్థానభ్రంశం(o, Get_దొంతర_శిఖరం_స్థానం_సంఖ్య(o));

        ఒకవేళ (i_update_దొంతర) {
_           (తీయి_Type(o, type));
_           (నెట్టు_రిజిస్టరు(o, type));
        }
    }

    _catch: తిరిగి ఫలితం;
}


// if local is unreferenced, o_preservedSlotNumber will be equal to localIndex on return
స్థిర
M3_ఫలితం కనుగొను_Referenced_Local_Within_Current_ఖండం (IM3Compilation o, u16* o_preserved_స్థానం_సంఖ్య, u32 i_local_స్థానం)
{
    M3_ఫలితం ఫలితం = m3Err_none;

    IM3CompilationScope scope = &o->ఖండం;

    u16 ప్రారంభం_సూచీ = scope->ఖండం_దొంతర_సూచీ;

    అయితే (scope->opcode == c_wa_Op_ఖండం) {
        scope = scope->outer;
        ఒకవేళ (not scope) {
            ఆపు;
        }

        ప్రారంభం_సూచీ = scope->ఖండం_దొంతర_సూచీ;
    }

    *o_preserved_స్థానం_సంఖ్య = (u16)i_local_స్థానం;

    కొరకు (u32 i = ప్రారంభం_సూచీ; i < o->దొంతర_సూచీ; ++i) {
        ఒకవేళ (o->wasm_దొంతర[i] == i_local_స్థానం) {
            m3type_t type = Get_దొంతర_Type_From_Bottom(o, i);                         d_m3Assert (type != c_m3Type_none)

            ఒకవేళ (*o_preserved_స్థానం_సంఖ్య == i_local_స్థానం) {
_               (AllocateSlots(o, o_preserved_స్థానం_సంఖ్య, type));
            } లేకపోతే {
                // A 64-bit value spans two slots, and allocating or releasing it
                // counts both, so a second reference has to count both as well.
                కొరకు (u16 s = 0; s < Get_Type_అంకె_Slots(type); ++s) {
_                   (Increment_స్థానం_Usage_లెక్క(o, (u16)(*o_preserved_స్థానం_సంఖ్య + s)));
                }
            }

            o->wasm_దొంతర[i] = *o_preserved_స్థానం_సంఖ్య;
        }
    }

    _catch: తిరిగి ఫలితం;
}

స్థిర
M3_ఫలితం Get_ఖండం_Scope (IM3Compilation o, IM3CompilationScope* o_scope, u32 i_లోతు)
{
    M3_ఫలితం ఫలితం = m3Err_none;

    IM3CompilationScope scope = &o->ఖండం;

    అయితే (i_లోతు--) {
        scope = scope->outer;
        _throwif("invalid block depth", not scope);
    }

    *o_scope = scope;

    _catch:
    తిరిగి ఫలితం;
}

స్థిర
M3_ఫలితం Copy_దొంతర_Slots_R (IM3Compilation o, u16 i_లక్ష్యం_స్థానం_దొంతర_సూచీ, u16 i_దొంతర_సూచీ, u16 i_చివర_దొంతర_సూచీ, u16 i_తాత్కాలిక_స్థానం)
{
    M3_ఫలితం ఫలితం = m3Err_none;

    ఒకవేళ (i_దొంతర_సూచీ < i_చివర_దొంతర_సూచీ) {
        u16 src_స్థానం = Get_స్థానం_For_దొంతర_సూచీ(o, i_దొంతర_సూచీ);

        m3type_t type      = Get_దొంతర_Type_From_Bottom(o, i_దొంతర_సూచీ);
        u16      అంకె_Slots  = Get_Type_అంకె_Slots(type);
        u16      extra_స్థానం = అంకె_Slots - 1;

        u16 లక్ష్యం_స్థానం = Get_స్థానం_For_దొంతర_సూచీ(o, i_లక్ష్యం_స్థానం_దొంతర_సూచీ);

        u16 preserve_సూచీ = i_దొంతర_సూచీ;
        u16 collision_స్థానం = src_స్థానం;

        ఒకవేళ (లక్ష్యం_స్థానం != src_స్థానం) {
            // search for collisions
            u16 check_సూచీ = i_దొంతర_సూచీ + 1;
            అయితే (check_సూచీ < i_చివర_దొంతర_సూచీ) {
                u16 otherSlot1 = Get_స్థానం_For_దొంతర_సూచీ(o, check_సూచీ);
                u16 otherSlot2 = Get_Extra_స్థానం_For_దొంతర_సూచీ(o, check_సూచీ);

                ఒకవేళ (లక్ష్యం_స్థానం == otherSlot1 or
                    లక్ష్యం_స్థానం == otherSlot2 or
                    లక్ష్యం_స్థానం + extra_స్థానం == otherSlot1) {
                    _throwif(m3_Err_function_దొంతర_అధికప్రవాహం, i_తాత్కాలిక_స్థానం >= d_m3MaxFunctionSlots);

_                   (Copy_దొంతర_సూచీ_గమ్యం_స్థానం(o, i_తాత్కాలిక_స్థానం, check_సూచీ));
                    o->wasm_దొంతర[check_సూచీ] = i_తాత్కాలిక_స్థానం;
                    i_తాత్కాలిక_స్థానం += Get_Type_అంకె_Slots(c_m3Type_i64);
                    Touch_స్థానం(o, i_తాత్కాలిక_స్థానం - 1);

                    // restore this on the way back down
                    preserve_సూచీ = check_సూచీ;
                    collision_స్థానం = otherSlot1;

                    ఆపు;
                }

                ++check_సూచీ;
            }

_           (Copy_దొంతర_సూచీ_గమ్యం_స్థానం(o, లక్ష్యం_స్థానం, i_దొంతర_సూచీ));                                                m3log (compile, " copying slot: %d to slot: %d", src_స్థానం, లక్ష్యం_స్థానం);
            o->wasm_దొంతర[i_దొంతర_సూచీ] = లక్ష్యం_స్థానం;
        }

_       (Copy_దొంతర_Slots_R(o, i_లక్ష్యం_స్థానం_దొంతర_సూచీ + 1, i_దొంతర_సూచీ + 1, i_చివర_దొంతర_సూచీ, i_తాత్కాలిక_స్థానం));

        // restore the stack state
        o->wasm_దొంతర[i_దొంతర_సూచీ]  = src_స్థానం;
        o->wasm_దొంతర[preserve_సూచీ] = collision_స్థానం;
    }

    _catch:
    తిరిగి ఫలితం;
}

స్థిర
M3_ఫలితం Resolve_ఖండం_Results (IM3Compilation o, IM3CompilationScope i_లక్ష్యం_ఖండం, bool i_isBranch)
{
    M3_ఫలితం ఫలితం = m3Err_none;
    ఒకవేళ (d_m3_Log_Wasm_దొంతర) {
        dump_type_దొంతర(o);
    }

    bool isLoop = (i_లక్ష్యం_ఖండం->opcode == c_waOp_loop and i_isBranch);

    u16 అంకె_Params  = Get_Func_Type_అంకె_Params(i_లక్ష్యం_ఖండం->type);
    u16 అంకె_Results = Get_Func_Type_అంకె_Results(i_లక్ష్యం_ఖండం->type);

    u16 స్థానం_రికార్డులు = i_లక్ష్యం_ఖండం->exit_దొంతర_సూచీ;

    u16 అంకె_విలువలు;

    ఒకవేళ (not isLoop) {
        అంకె_విలువలు = అంకె_Results;
        స్థానం_రికార్డులు += అంకె_Params;
    } లేకపోతే {
        అంకె_విలువలు = అంకె_Params;
    }

    u16 ఖండం_ఎత్తు = Get_అంకె_ఖండం_విలువలు_On_దొంతర(o);

    _throwif(m3_Err_type_లెక్క_Mismatch, i_isBranch ? (ఖండం_ఎత్తు < అంకె_విలువలు) : (ఖండం_ఎత్తు != అంకె_విలువలు));

    ఒకవేళ (అంకె_విలువలు) {
        u16 చివర_సూచీ     = Get_దొంతర_శిఖరం_సూచీ(o) + 1;
        u16 అంకె_Rem_విలువలు = అంకె_విలువలు;

        // The last result is taken from _fp0. See PushBlockResults.
        ఒకవేళ (not isLoop and IsFpType(Get_దొంతర_శిఖరం_Type(o))) {
_           (Copy_దొంతర_శిఖరం_గమ్యం_రిజిస్టరు(o, false));
            --చివర_సూచీ;
            --అంకె_Rem_విలువలు;
        }

        // tempslot affects maxStackSlots, so can grow unnecess each time.
        // in practice, this is hardly ever hit.
        u16 తాత్కాలిక_స్థానం = o->max_దొంతర_Slots;// GetMaxUsedSlotPlusOne (o); doesn't work cause can collide with slotRecords
        Align_స్థానం_గమ్యం_Type(&తాత్కాలిక_స్థానం, c_m3Type_i64);

_       (Copy_దొంతర_Slots_R(o, స్థానం_రికార్డులు, చివర_సూచీ - అంకె_Rem_విలువలు, చివర_సూచీ, తాత్కాలిక_స్థానం));

        ఒకవేళ (d_m3_Log_Wasm_దొంతర) {
            dump_type_దొంతర(o);
        }
    }

    _catch: తిరిగి ఫలితం;
}


స్థిర
M3_ఫలితం Return_విలువలు (IM3Compilation o, IM3CompilationScope i_function_ఖండం, bool i_isBranch)
{
    M3_ఫలితం ఫలితం = m3Err_none;
    ఒకవేళ (d_m3_Log_Wasm_దొంతర) {
        dump_type_దొంతర(o);
    }

    u16 అంకె_Returns  = Get_Func_Type_అంకె_Results(i_function_ఖండం->type);     // could just o->function too...
    u16 ఖండం_ఎత్తు = Get_అంకె_ఖండం_విలువలు_On_దొంతర(o);

    ఒకవేళ (not Is_దొంతర_Polymorphic(o)) {
        _throwif(m3_Err_type_లెక్క_Mismatch, i_isBranch ? (ఖండం_ఎత్తు < అంకె_Returns) : (ఖండం_ఎత్తు != అంకె_Returns));
    }

    ఒకవేళ (అంకె_Returns) {
        // return slots like args are 64-bit aligned
        u16 return_స్థానం = అంకె_Returns * c_io_స్థానం_లెక్క;
        u16 దొంతర_శిఖరం   = Get_దొంతర_శిఖరం_సూచీ(o);

        కొరకు (u16 i = 0; i < అంకె_Returns; ++i) {
            m3type_t returnType = Get_Func_Type_ఫలితం_Type(i_function_ఖండం->type, అంకె_Returns - 1 - i);

            m3type_t దొంతర_Type = Get_దొంతర_Type_From_శిఖరం(o, i);  // using FromTop so that only dynamic items are checked

            ఒకవేళ (Is_దొంతర_Polymorphic(o) and దొంతర_Type == c_m3Type_none) {
                దొంతర_Type = returnType;
            }

            _throwif(m3Err_typeMismatch, not Is_Sub_Type_యొక్క(దొంతర_Type, returnType));

            ఒకవేళ (not Is_దొంతర_Polymorphic(o)) {
                return_స్థానం -= c_io_స్థానం_లెక్క;
_               (Copy_దొంతర_సూచీ_గమ్యం_స్థానం(o, return_స్థానం, దొంతర_శిఖరం--));
            }
        }

        ఒకవేళ (not i_isBranch) {
            అయితే (అంకె_Returns--) {
_               (తీయి(o));
            }
        }
    }

    _catch: తిరిగి ఫలితం;
}


//-------------------------------------------------------------------------------------------------------------------------

స్థిర
M3_ఫలితం Compile_Const_i32 (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_ఫలితం ఫలితం;

    i32 విలువ;
_   (ReadLEB_i32(&విలువ, &o->wasm, o->wasm_చివర));
_   (నెట్టు_Const(o, విలువ, c_m3Type_i32));                        m3log (compile, d_indent " (const i32 = %" PRIi32 ")", get_indention_string (o), విలువ);
    _catch: తిరిగి ఫలితం;
}

స్థిర
M3_ఫలితం Compile_Const_i64 (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_ఫలితం ఫలితం;

    i64 విలువ;
_   (ReadLEB_i64(&విలువ, &o->wasm, o->wasm_చివర));
_   (నెట్టు_Const(o, విలువ, c_m3Type_i64));                        m3log (compile, d_indent " (const i64 = %" PRIi64 ")", get_indention_string (o), విలువ);
    _catch: తిరిగి ఫలితం;
}


#ఒకవేళ d_m3ImplementFloat
స్థిర
M3_ఫలితం Compile_Const_f32 (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_ఫలితం ఫలితం;

    సంఘం {
        u32 u;
        f32 f;
    } విలువ = { 0 };

_   (Read_f32(&విలువ.f, &o->wasm, o->wasm_చివర));                 m3log (compile, d_indent " (const f32 = %" PRIf32 ")", get_indention_string (o), విలువ.f);
_   (నెట్టు_Const(o, విలువ.u, c_m3Type_f32));

    _catch: తిరిగి ఫలితం;
}

స్థిర
M3_ఫలితం Compile_Const_f64 (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_ఫలితం ఫలితం;

    సంఘం {
        u64 u;
        f64 f;
    } విలువ = { 0 };

_   (Read_f64(&విలువ.f, &o->wasm, o->wasm_చివర));                 m3log (compile, d_indent " (const f64 = %" PRIf64 ")", get_indention_string (o), విలువ.f);
_   (నెట్టు_Const(o, విలువ.u, c_m3Type_f64));

    _catch: తిరిగి ఫలితం;
}
#అయితే_ముగింపు

స్థిర
M3_ఫలితం Compile_ExtendedOpcode (IM3Compilation o, m3opcode_t i_opcode)
{
_try {
    u32 opcode;
_   (ReadLEB_u32(&opcode, &o->wasm, o->wasm_చివర));               m3log (compile, d_indent " (%s: %" PRIu32 ")", get_indention_string (o), (i_opcode == c_waOp_atomic) ? "FE" : "FC", opcode);

    // m3opcode_t carries only the low byte, so a wider sub-opcode is unknown
    // here rather than something to truncate into range
    _throwif(m3Err_unknownOpcode, opcode > (u32)((i_opcode == c_waOp_atomic) ? c_wa_Op_చివరి_Atomic : c_wa_Op_చివరి_Extended));

    i_opcode = (m3opcode_t)((i_opcode << 8) | opcode);

    //printf("Extended opcode: 0x%x\n", i_opcode);

    IM3OpInfo opInfo = GetOpInfo(i_opcode);
    _throwif(m3Err_unknownOpcode, not opInfo);

    M3Compiler compiler = GetCompiler(opInfo);
    _throwif(m3Err_noCompiler, not compiler);

_   ((*compiler)(o, i_opcode));

    o->మునుపటి_Opcode = i_opcode;

    } _catch: తిరిగి ఫలితం;
}

స్థిర
M3_ఫలితం Compile_Return (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_ఫలితం ఫలితం = m3Err_none;

    ఒకవేళ (not Is_దొంతర_Polymorphic(o)) {
        IM3CompilationScope functionScope;
_       (Get_ఖండం_Scope(o, &functionScope, o->ఖండం.లోతు));

_       (Return_విలువలు(o, functionScope, true));

_       (EmitOp(o, op_Return));

_       (సమితి_దొంతర_Polymorphic(o));
    }

    _catch: తిరిగి ఫలితం;
}

స్థిర
M3_ఫలితం Validate_ఖండం_చివర (IM3Compilation o)
{
    M3_ఫలితం ఫలితం = m3Err_none;

    u16 అంకె_Results  = Get_Func_Type_అంకె_Results(o->ఖండం.type);
    u16 ఖండం_ఎత్తు = Get_అంకె_ఖండం_విలువలు_On_దొంతర(o);

    ఒకవేళ (not Is_దొంతర_Polymorphic(o)) {
        // Spec: at block end, stack height must match the number of results
        _throwif(m3_Err_type_లెక్క_Mismatch, ఖండం_ఎత్తు != అంకె_Results);

        // Spec: result types must match expected types
        కొరకు (u16 i = 0; i < అంకె_Results; ++i) {
            m3type_t expectedType = Get_Func_Type_ఫలితం_Type(o->ఖండం.type, అంకె_Results - 1 - i);
            m3type_t actualType   = Get_దొంతర_Type_From_శిఖరం(o, i);
            _throwif(m3Err_typeMismatch, not Is_Sub_Type_యొక్క(actualType, expectedType));
        }
    }

    _catch: తిరిగి ఫలితం;
}

స్థిర
M3_ఫలితం Compile_చివర (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_ఫలితం ఫలితం = m3Err_none;                   //dump_type_stack (o);

    // function end:
    ఒకవేళ (o->ఖండం.లోతు == 0) {
        Validate_ఖండం_చివర(o);

        //      if (not IsStackPolymorphic (o))
        {
            // A constant expression has no function frame, but its value still
            // has to be copied down to slot 0, where EvaluateExpression reads it
            // back. The non-emitting init-expr walk is skipped: it has no block
            // type to return against.
            ఒకవేళ (o->function or o->పుట) {
_               (Return_విలువలు(o, &o->ఖండం, false));
            }

_           (EmitOp(o, op_Return));
        }
    }

    _catch: తిరిగి ఫలితం;
}


స్థిర
M3_ఫలితం Compile_సమితి_Local (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_ఫలితం ఫలితం;

    u32 local_సూచీ;
_   (ReadLEB_u32(&local_సూచీ, &o->wasm, o->wasm_చివర));                           m3log (compile, d_indent " (index = %u)", get_indention_string (o), local_సూచీ);

    ఒకవేళ (local_సూచీ < Get_Function_అంకె_వాదనలు_And_Locals(o->function)) {
        // Spec: value type must match local type
        ఒకవేళ (not Is_దొంతర_Polymorphic(o)) {
            m3type_t localType    = Get_దొంతర_Type_From_Bottom(o, local_సూచీ);
            m3type_t దొంతర_శిఖరం_Type = Get_దొంతర_శిఖరం_Type(o);
            _throwif(m3Err_typeMismatch, దొంతర_శిఖరం_Type != c_m3Type_none and localType != c_m3Type_none and
                                           not Is_Sub_Type_యొక్క(దొంతర_శిఖరం_Type, localType));
#ఒకవేళ d_m3HasSnapshots
            ఒకవేళ (o->local_Init_లోతు and o->local_Init_లోతు[local_సూచీ] == UINT32_MAX) {
                o->local_Init_లోతు[local_సూచీ] = (u32)o->ఖండం.లోతు;
            }
#అయితే_ముగింపు
        }

        u16 local_స్థానం = Get_స్థానం_For_దొంతర_సూచీ(o, local_సూచీ);

        u16 preserve_స్థానం;
_       (కనుగొను_Referenced_Local_Within_Current_ఖండం(o, &preserve_స్థానం, local_స్థానం));  // preserve will be different than local, if referenced

        bool folded = false;

#ఒకవేళ d_m3_Fold_సమితి_Local
        // destination folding: when the value was produced by the immediately preceding op (nothing
        // emitted since, result on top in _r0, no copy-on-write preservation needed), retro-patch the
        // producer to a variant that writes straight to the local's slot and skip the SetSlot op
        ఒకవేళ (o->foldPatchPC and preserve_స్థానం == local_స్థానం and Is_దొంతర_శిఖరం_లోపల_రిజిస్టరు(o) and Get_దొంతర_శిఖరం_సూచీ(o) == (i16)o->fold_దొంతర_సూచీ) {
            IM3Operation foldOp = GetFoldOp(o->foldOpcode, o->foldForm);

            ఒకవేళ (foldOp) {
                *(IM3Operation*)o->foldPatchPC = foldOp;     // retro-patch the producer
                Emit_స్థానం_స్థానభ్రంశం(o, local_స్థానం);                  // append its destination
                InvalidateFold(o);
                folded = true;
            }
        }
#అయితే_ముగింపు

        ఒకవేళ (folded) {
            m3type_t type = Get_దొంతర_శిఖరం_Type(o);
_           (తీయి(o));                              // the value now lives in the local's slot

            // A folded op writes its result through to the register as well, so a tee
            // can keep it there. Pushing the slot instead would make the next use that
            // wants a register reload the slot this op just stored to.
            ఒకవేళ (i_opcode == c_waOp_teeLocal) {
_               (నెట్టు_రిజిస్టరు(o, type));
            }
        } లేకపోతే {
            ఒకవేళ (preserve_స్థానం == local_స్థానం) {
_               (Copy_దొంతర_శిఖరం_గమ్యం_స్థానం(o, local_స్థానం))
            } లేకపోతే {
_               (Preserved_Copy_శిఖరం_స్థానం(o, local_స్థానం, preserve_స్థానం))
            }

            ఒకవేళ (i_opcode != c_waOp_teeLocal) {
_               (తీయి(o));
            }
        }
    } లేకపోతే {
        _throw("local index out of bounds");
    }

    _catch: తిరిగి ఫలితం;
}

స్థిర
M3_ఫలితం Compile_GetLocal (IM3Compilation o, m3opcode_t i_opcode)
{
_try {

    u32 local_సూచీ;
_   (ReadLEB_u32(&local_సూచీ, &o->wasm, o->wasm_చివర));                           m3log (compile, d_indent " (index = %u)", get_indention_string (o), local_సూచీ);

    ఒకవేళ (local_సూచీ >= Get_Function_అంకె_వాదనలు_And_Locals(o->function)) {
        _throw("local index out of bounds");
    }

    m3type_t type = Get_దొంతర_Type_From_Bottom(o, local_సూచీ);
    u16      స్థానం = Get_స్థానం_For_దొంతర_సూచీ(o, local_సూచీ);

_   (నెట్టు(o, type, స్థానం));

    } _catch: తిరిగి ఫలితం;
}

స్థిర
M3_ఫలితం Compile_GetGlobal (IM3Compilation o, M3Global* i_global)
{
    M3_ఫలితం ఫలితం;

    IM3Operation op = Is64_బిట్టు_Type(i_global->type) ? op_GetGlobal_s64 : op_GetGlobal_s32;
_   (EmitOp(o, op));
    Emit_సూచిక(o, &i_global->i64_విలువ);
_   (నెట్టు_Allocated_స్థానం_And_Emit_2(o, i_global->type));

    _catch: తిరిగి ఫలితం;
}

స్థిర
M3_ఫలితం Compile_సమితి_Global (IM3Compilation o, M3Global* i_global)
{
    M3_ఫలితం ఫలితం = m3Err_none;

    ఒకవేళ (i_global->isMutable) {
        // Spec: value type must match global type
        ఒకవేళ (not Is_దొంతర_Polymorphic(o)) {
            m3type_t దొంతర_శిఖరం_Type = Get_దొంతర_శిఖరం_Type(o);
            _throwif(m3Err_typeMismatch, దొంతర_శిఖరం_Type != c_m3Type_none and
                                           not Is_Sub_Type_యొక్క(దొంతర_శిఖరం_Type, i_global->type));
        }

        IM3Operation op;
        m3type_t     type = Get_దొంతర_శిఖరం_Type(o);

        ఒకవేళ (Is_దొంతర_శిఖరం_లోపల_రిజిస్టరు(o)) {
            op = c_సమితి_Global_Ops[పునాది_Type_యొక్క(type)];
        } లేకపోతే {
            op = Is64_బిట్టు_Type(type) ? op_సమితి_Global_s64 : op_సమితి_Global_s32;
        }

_       (EmitOp(o, op));
        Emit_సూచిక(o, &i_global->i64_విలువ);

        ఒకవేళ (Is_దొంతర_శిఖరం_లోపల_స్థానం(o)) {
            Emit_స్థానం_స్థానభ్రంశం(o, Get_దొంతర_శిఖరం_స్థానం_సంఖ్య(o));
        }

_       (తీయి(o));
    } లేకపోతే {
        _throw(m3Err_settingImmutableGlobal);
    }

    _catch: తిరిగి ఫలితం;
}

స్థిర
M3_ఫలితం Compile_Get_సమితి_Global (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_ఫలితం ఫలితం = m3Err_none;

    u32 global_సూచీ;
_   (ReadLEB_u32(&global_సూచీ, &o->wasm, o->wasm_చివర));

    ఒకవేళ (global_సూచీ < o->module->అంకె_Globals) {
        ఒకవేళ (o->module->globals) {
            M3Global* global = &o->module->globals[global_సూచీ];

            // Spec: a constant expression may only read an imported immutable
            // global. The module's own globals are counted before their
            // initializer is walked, so a bare index check would let one
            // reference itself. These describe how the module declared the
            // global, so they are checked before following any link.
            _throwif(m3_Err_globa_సూచీ_బహిర్గతం_యొక్క_Bounds, o->is_Init_సమాసం and not global->imported);
            _throwif(m3Err_wasmMalformed, o->is_Init_సమాసం and global->isMutable);

            // an import linked to another module reads and writes that module's
            // cell, not the placeholder standing in for it here
            ఒకవేళ (global->resolved) {
                global = global->resolved;
            }

_           ((i_opcode == c_waOp_getGlobal) ? Compile_GetGlobal(o, global) : Compile_సమితి_Global(o, global));
        } లేకపోతే {
            _throw(ErrorCompile(m3_Err_global_జ్ఞాపకం_Not_Allocated, o, "module '%s' is missing global memory", o->module->name));
        }
    } లేకపోతే {
        _throw(m3_Err_globa_సూచీ_బహిర్గతం_యొక్క_Bounds);
    }

    _catch: తిరిగి ఫలితం;
}

స్థిర
శూన్య Emit_Patching_Branch_సూచిక (IM3Compilation o, IM3CompilationScope i_scope)
{
    pc_t patch       = Emit_సూచిక(o, i_scope->patches);                m3log (compile, "branch patch required at: %p", patch);
    i_scope->patches = patch;
}

స్థిర
M3_ఫలితం EmitPatchingBranch (IM3Compilation o, IM3CompilationScope i_scope)
{
    M3_ఫలితం ఫలితం = m3Err_none;

_   (EmitOp(o, op_Branch));
    Emit_Patching_Branch_సూచిక(o, i_scope);

    _catch: తిరిగి ఫలితం;
}

#ఒకవేళ d_m3HasExceptionHandling

// How many try_table handler records an exit from i_from out to i_target takes
// down with it. Walking outward from the block the branch sits in, every
// try_table scope up to and including the target is left behind - the target
// too, because landing on a block's label means that block is finished.
//
// A loop label is the exception, but a loop is never a try_table, so the count
// is the same either way: the loop itself is not on the list.
స్థిర
u32 లెక్క_Try_చట్రాలు_గమ్యం_Exit (IM3CompilationScope i_from, IM3CompilationScope i_లక్ష్యం)
{
    u32 లెక్క = 0;

    కొరకు (IM3CompilationScope scope = i_from; scope; scope = scope->outer) {
        ఒకవేళ (scope->opcode == c_wa_Op_try_పట్టిక) {
            ++లెక్క;
        }

        ఒకవేళ (scope == i_లక్ష్యం) {
            ఆపు;
        }
    }

    తిరిగి లెక్క;
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
స్థిర
u32 అంకె_Try_చట్రాలు_గమ్యం_తీయి (IM3CompilationScope i_from, IM3CompilationScope i_లక్ష్యం)
{
    ఒకవేళ (i_లక్ష్యం->లోతు == 0) {
        తిరిగి 0;
    }

    // the walk only ever runs inward-to-outward; a target that is already
    // outside i_from has nothing between them
    ఒకవేళ (not i_from or i_from->లోతు < i_లక్ష్యం->లోతు) {
        తిరిగి 0;
    }

    తిరిగి లెక్క_Try_చట్రాలు_గమ్యం_Exit(i_from, i_లక్ష్యం);
}


స్థిర
M3_ఫలితం Emit_తీయి_Try_చట్రాలు (IM3Compilation o, u32 i_అంకె_చట్రాలు)
{
    M3_ఫలితం ఫలితం = m3Err_none;

    ఒకవేళ (i_అంకె_చట్రాలు) {
_       (EmitOp(o, op_తీయి_Handlers));
        EmitConstant32(o, i_అంకె_చట్రాలు);
    }

    _catch: తిరిగి ఫలితం;
}


// A return_call leaves the enclosing function before the callee runs, so no
// try_table in this function may catch what the callee throws. Neither shape of
// the instruction unwinds the native stack in time to arrange that on its own:
// op_ReturnCall tail-jumps with op_TryTable's frames still standing, and the
// fallback shape is an ordinary call sitting inside them. Either way the
// handler records have to come off first.
స్థిర
M3_ఫలితం Emit_తీయి_Try_చట్రాలు_For_Return_Call (IM3Compilation o)
{
    u32 లెక్క = 0;

    కొరకు (IM3CompilationScope scope = &o->ఖండం; scope; scope = scope->outer) {
        ఒకవేళ (scope->opcode == c_wa_Op_try_పట్టిక) {
            ++లెక్క;
        }
    }

    తిరిగి Emit_తీయి_Try_చట్రాలు(o, లెక్క);
}

#లేకపోతే

స్థిర
M3_ఫలితం Emit_తీయి_Try_చట్రాలు (IM3Compilation o, u32 i_అంకె_చట్రాలు)
{
    (శూన్య)o;
    (శూన్య)i_అంకె_చట్రాలు;
    తిరిగి m3Err_none;
}

#అయితే_ముగింపు // d_m3HasExceptionHandling

#ఒకవేళ d_m3FuseBranch
// when the pending fold candidate is a compare whose result sits on top in _r0, return the fused
// variant that absorbs an immediately following br_if (i_isIf false) or if (i_isIf true)
స్థిర
IM3Operation GetBranchFusionOp (IM3Compilation o, bool i_isIf)
{
    ఒకవేళ (o->foldPatchPC and o->foldForm < 4 and Is_దొంతర_శిఖరం_లోపల_రిజిస్టరు(o) and Get_దొంతర_శిఖరం_సూచీ(o) == (i16)o->fold_దొంతర_సూచీ) {
        స్థిరరాశి M3FusedCmpOps* ops = GetFusedCmpOps(o->foldOpcode);

        ఒకవేళ (ops) {
            తిరిగి i_isIf ? ops->ifOp[o->foldForm] : ops->branchIf[o->foldForm];
        }
    }

    తిరిగి ఖాళీ;
}
#అయితే_ముగింపు // d_m3FuseBranch

స్థిర అంతఃవరుస
IM3Operation Op_ContinueLoop (IM3Compilation o)
{
#ఒకవేళ d_m3_Has_దొంతర_Switching
    ఒకవేళ (M3_UNLIKELY(o->module->runtime and o->module->runtime->isSuspendable)) {
        తిరిగి op_ContinueLoop_Suspendable;
    }
#అయితే_ముగింపు
    తిరిగి op_ContinueLoop;
}

స్థిర అంతఃవరుస
IM3Operation Op_ContinueLoopIf (IM3Compilation o)
{
#ఒకవేళ d_m3_Has_దొంతర_Switching
    ఒకవేళ (M3_UNLIKELY(o->module->runtime and o->module->runtime->isSuspendable)) {
        తిరిగి op_ContinueLoopIf_Suspendable;
    }
#అయితే_ముగింపు
    తిరిగి op_ContinueLoopIf;
}


స్థిర
M3_ఫలితం Compile_Branch (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_ఫలితం ఫలితం;
    u32      అంకె_Try_చట్రాలు = 0;

    u32 లోతు;
_   (ReadLEB_u32(&లోతు, &o->wasm, o->wasm_చివర));

    // Spec: br_if condition must be i32
    ఒకవేళ (i_opcode == c_waOp_branchIf and not Is_దొంతర_Polymorphic(o)) {
        m3type_t condType = Get_దొంతర_శిఖరం_Type(o);
        _throwif(m3Err_typeMismatch, condType != c_m3Type_none and condType != c_m3Type_i32);
    }

    IM3CompilationScope scope;
_   (Get_ఖండం_Scope(o, &scope, లోతు));

#ఒకవేళ d_m3HasExceptionHandling
    అంకె_Try_చట్రాలు = అంకె_Try_చట్రాలు_గమ్యం_తీయి(&o->ఖండం, scope);
#అయితే_ముగింపు

    // branch target is a loop (continue)
    ఒకవేళ (scope->opcode == c_waOp_loop) {
        ఒకవేళ (i_opcode == c_waOp_branchIf) {
            // a handler pop belongs on the taken path only, so it forces the
            // jump-over form even where the target takes no parameters
            ఒకవేళ (Get_Func_Type_అంకె_Params(scope->type) or అంకె_Try_చట్రాలు) {
                IM3Operation op = Is_దొంతర_శిఖరం_లోపల_రిజిస్టరు(o) ? op_BranchIfPrologue_r : op_BranchIfPrologue_s;

_               (EmitOp(o, op));
_               (Emit_స్థానం_అంకె_యొక్క_దొంతర_శిఖరం_And_తీయి(o));

                pc_t* jump_గమ్యం = (pc_t*)Reserve_సూచిక(o);

_               (Emit_తీయి_Try_చట్రాలు(o, అంకె_Try_చట్రాలు));

                ఒకవేళ (not Is_దొంతర_Polymorphic(o)) {
_                   (Resolve_ఖండం_Results(o, scope, /* isBranch: */ true));
                }

_               (EmitOp(o, Op_ContinueLoop(o)));
                Emit_సూచిక(o, scope->pc);
_               (రికార్డు_వెనుక_అంచు(o, scope, false));

                *jump_గమ్యం = GetPC(o);
            } లేకపోతే {
                // move the condition to a register
_               (Copy_దొంతర_శిఖరం_గమ్యం_రిజిస్టరు(o, false));
_               (తీయి_Type(o, c_m3Type_i32));

_               (EmitOp(o, Op_ContinueLoopIf(o)));
                Emit_సూచిక(o, scope->pc);
_               (రికార్డు_వెనుక_అంచు(o, scope, true));
            }

            //          dump_type_stack(o);
        } లేకపోతే // is c_waOp_branch
        {
_           (Emit_తీయి_Try_చట్రాలు(o, అంకె_Try_చట్రాలు));

            // the branch operands become the loop's parameters on the next iteration
            ఒకవేళ (not Is_దొంతర_Polymorphic(o)) {
_               (Resolve_ఖండం_Results(o, scope, /* isBranch: */ true));
            }

_           (EmitOp(o, Op_ContinueLoop(o)));
            Emit_సూచిక(o, scope->pc);
_           (రికార్డు_వెనుక_అంచు(o, scope, false));
            o->ఖండం.isPolymorphic = true;
        }
    } లేకపోతే // forward branch
    {
        pc_t* jump_గమ్యం = ఖాళీ;

        bool isReturn         = (scope->లోతు == 0);
        bool లక్ష్యం_Has_Results = Get_Func_Type_అంకె_Results(scope->type);

        ఒకవేళ (i_opcode == c_waOp_branchIf) {
            ఒకవేళ (లక్ష్యం_Has_Results or isReturn or అంకె_Try_చట్రాలు) {
                IM3Operation op = Is_దొంతర_శిఖరం_లోపల_రిజిస్టరు(o) ? op_BranchIfPrologue_r : op_BranchIfPrologue_s;

_               (EmitOp(o, op));
_               (Emit_స్థానం_అంకె_యొక్క_దొంతర_శిఖరం_And_తీయి(o)); // condition

                // this is continuation point, if the branch isn't taken
                jump_గమ్యం = (pc_t*)Reserve_సూచిక(o);
            } లేకపోతే {
#ఒకవేళ d_m3FuseBranch
                IM3Operation fuseOp = GetBranchFusionOp(o, false);

                ఒకవేళ (fuseOp) {
                    // absorb the branch into the compare that produced the condition
                    *(IM3Operation*)o->foldPatchPC = fuseOp;
                    InvalidateFold(o);

_                   (తీయి(o));                      // the condition is consumed inside the fused op

                    Emit_Patching_Branch_సూచిక(o, scope);
                    వెళ్ళు _catch;
                }
#అయితే_ముగింపు
                IM3Operation op = Is_దొంతర_శిఖరం_లోపల_రిజిస్టరు(o) ? op_BranchIf_r : op_BranchIf_s;

_               (EmitOp(o, op));
_               (Emit_స్థానం_అంకె_యొక్క_దొంతర_శిఖరం_And_తీయి(o)); // condition

                Emit_Patching_Branch_సూచిక(o, scope);
                వెళ్ళు _catch;
            }
        }

        ఒకవేళ (not Is_దొంతర_Polymorphic(o)) {
            ఒకవేళ (isReturn) {
_               (Return_విలువలు(o, scope, true));
_               (EmitOp(o, op_Return));
            } లేకపోతే {
_               (Emit_తీయి_Try_చట్రాలు(o, అంకె_Try_చట్రాలు));

_               (Resolve_ఖండం_Results(o, scope, true));
_               (EmitPatchingBranch(o, scope));
            }
        }

        ఒకవేళ (jump_గమ్యం) {
            *jump_గమ్యం = GetPC(o);
        }

        ఒకవేళ (i_opcode == c_waOp_branch) {
_           (సమితి_దొంతర_Polymorphic(o));
        }
    }

    _catch: తిరిగి ఫలితం;
}

స్థిర
M3_ఫలితం Compile_Branch_పట్టిక (IM3Compilation o, m3opcode_t i_opcode)
{
    // the page a continue-op page is standing in for; non-NULL only while that
    // page is installed in o->page, so the catch below knows to put it back
    IM3_సంకేతం_పుట displaced_పుట = ఖాళీ;
    // indexed by label depth: the continuation stub already emitted for that target
    // within this table, or NULL if this table has not reached it yet
    pc_t* లక్ష్యం_Stubs = ఖాళీ;
_try {
    u32 లక్ష్యం_లెక్క;
_   (ReadLEB_u32(&లక్ష్యం_లెక్క, &o->wasm, o->wasm_చివర));

    // Spec: validate that the branch index operand is i32
    ఒకవేళ (not Is_దొంతర_Polymorphic(o)) {
        m3type_t సూచీ_Type = Get_దొంతర_శిఖరం_Type(o);
        _throwif(m3Err_typeMismatch, సూచీ_Type != c_m3Type_none and సూచీ_Type != c_m3Type_i32);
    }

_   (Preserve_రిజిస్టరు_If_Occupied(o, c_m3Type_i64));         // move branch operand to a slot
    u16 స్థానం = Get_దొంతర_శిఖరం_స్థానం_సంఖ్య(o);
_   (తీయి(o));

    // OPTZ: according to spec: "forward branches that target a control instruction with a non-empty
    // result type consume matching operands first and push them back on the operand stack after unwinding"
    // So, this move-to-reg is only necessary if the target scopes have a type.

    u32 అంకె_సంకేతం_పంక్తులు = లక్ష్యం_లెక్క + 4; // 3 => IM3Operation + slot + target_count + default_target
_   (Ensure_సంకేతం_పుట_అంకె_పంక్తులు(o, అంకె_సంకేతం_పంక్తులు));

_   (EmitOp(o, op_Branch_పట్టిక));
    Emit_స్థానం_స్థానభ్రంశం(o, స్థానం);
    EmitConstant32(o, లక్ష్యం_లెక్క);

    IM3_సంకేతం_పుట continue_Op_పుట = ఖాళీ;

    // The label types are checked by ValidateFunction, which runs under the same
    // d_m3EnableValidation guard just before this compilation pass. Comparing each
    // target against the default here would be wrong anyway: the spec matches every
    // target against the operand stack, so in unreachable code the operands are the
    // bottom type and the targets may legitimately differ (unreached-valid.wast).

    ++లక్ష్యం_లెక్క; // include default

    // Every entry of one br_table is resolved from the same compilation state - each
    // stub restores whatever it touched - so two entries naming the same label emit
    // byte-identical epilogues. Emit one and point both table entries at it. A label
    // deeper than the current block is rejected by GetBlockScope below, which is what
    // bounds this array.
    లక్ష్యం_Stubs = m3_AllocArray(pc_t, (size_t)o->ఖండం.లోతు + 1);
    _throwifnull(లక్ష్యం_Stubs);

    కొరకు (u32 i = 0; i < లక్ష్యం_లెక్క; ++i) {
        u32 లక్ష్యం;
_       (ReadLEB_u32(&లక్ష్యం, &o->wasm, o->wasm_చివర));

        IM3CompilationScope scope;
_       (Get_ఖండం_Scope(o, &scope, లక్ష్యం));

        ఒకవేళ (లక్ష్యం_Stubs[లక్ష్యం]) {
            Emit_సూచిక(o, లక్ష్యం_Stubs[లక్ష్యం]);
            కొనసాగు;
        }

        u32 అంకె_Try_చట్రాలు = 0;
#ఒకవేళ d_m3HasExceptionHandling
        అంకె_Try_చట్రాలు = అంకె_Try_చట్రాలు_గమ్యం_తీయి(&o->ఖండం, scope);
#అయితే_ముగింపు

        // A forward branch to a block that carries no results and leaves no try
        // frames behind has nothing to do on the way out but arrive. There is no
        // epilogue to hold, so the table entry becomes the patch site itself and the
        // block's address is written into it when the block ends - no stub, and no
        // code page to put one on. In unreachable code the same is true whatever the
        // block's type, since nothing is emitted for that case either way.
        //
        // Each such entry is patched on its own, so none is recorded in targetStubs:
        // there is no shared stub for a later entry naming this label to point at.
        ఒకవేళ (scope->opcode != c_waOp_loop and scope->లోతు > 0 and అంకె_Try_చట్రాలు == 0 and
            (Is_దొంతర_Polymorphic(o) or Get_Func_Type_అంకె_Results(scope->type) == 0)) {
            Emit_Patching_Branch_సూచిక(o, scope);
            కొనసాగు;
        }

_       (Acquire_Compilation_సంకేతం_పుట(o, &continue_Op_పుట));

        pc_t ప్రారంభం_PC  = Get_పుట_PC(continue_Op_పుట);
        displaced_పుట = o->పుట;
        o->పుట       = continue_Op_పుట;

        ఒకవేళ (scope->opcode == c_waOp_loop) {
_           (Emit_తీయి_Try_చట్రాలు(o, అంకె_Try_చట్రాలు));

            ఒకవేళ (not Is_దొంతర_Polymorphic(o)) {
_               (Resolve_ఖండం_Results(o, scope, true));
            }

_           (EmitOp(o, Op_ContinueLoop(o)));
            Emit_సూచిక(o, scope->pc);
_           (రికార్డు_వెనుక_అంచు(o, scope, false));
        } లేకపోతే {
            ఒకవేళ (not Is_దొంతర_Polymorphic(o)) {
                ఒకవేళ (scope->లోతు == 0) {
_                   (Return_విలువలు(o, scope, true));
_                   (EmitOp(o, op_Return));
                } లేకపోతే {
_                   (Emit_తీయి_Try_చట్రాలు(o, అంకె_Try_చట్రాలు));

_                   (Resolve_ఖండం_Results(o, scope, true));

_                   (EmitPatchingBranch(o, scope));
                }
            }
        }

        IM3_సంకేతం_పుట return_గమ్యం = displaced_పుట;
        displaced_పుట        = ఖాళీ;

        Release_Compilation_సంకేతం_పుట(o);
        o->పుట = return_గమ్యం;

        Emit_సూచిక(o, ప్రారంభం_PC);
        లక్ష్యం_Stubs[లక్ష్యం] = ప్రారంభం_PC;
    }

_   (సమితి_దొంతర_Polymorphic(o));
} _catch:

    // thrown out of the loop above with a continue-op page installed: release it
    // and restore the one it displaced, which the caller's catch then releases.
    // Otherwise the displaced page is on no list and simply leaks.
    ఒకవేళ (displaced_పుట) {
        Release_Compilation_సంకేతం_పుట(o);
        o->పుట = displaced_పుట;
    }

    m3_Free(లక్ష్యం_Stubs);

    తిరిగి ఫలితం;
}

స్థిర
M3_ఫలితం Compile_Call_వాదనలు_And_Return (IM3Compilation o, u16* o_దొంతర_స్థానభ్రంశం, IM3FuncType i_type, bool i_is_పరోక్ష)
{
_try {

    u16 శిఖరం_స్థానం = Get_Max_వాడిన_స్థానం_Plus_One(o);

    // force use of at least one stack slot; this is to help ensure
    // the m3 stack overflows (and traps) before the native stack can overflow.
    // e.g. see Wasm spec test 'runaway' in call.wast
    శిఖరం_స్థానం = M3_MAX(1, శిఖరం_స్థానం);

    // stack frame is 64-bit aligned
    Align_స్థానం_గమ్యం_Type(&శిఖరం_స్థానం, c_m3Type_i64);

    *o_దొంతర_స్థానభ్రంశం = శిఖరం_స్థానం;

    // wait to pop this here so that topSlot search is correct
    ఒకవేళ (i_is_పరోక్ష) {
_       (తీయి(o));
    }

    u16 అంకె_వాదనలు = Get_Func_Type_అంకె_Params(i_type);
    u16 అంకె_Rets = Get_Func_Type_అంకె_Results(i_type);

    u16 arg_శిఖరం = శిఖరం_స్థానం + (అంకె_వాదనలు + అంకె_Rets) * c_io_స్థానం_లెక్క;

    Touch_స్థానం(o, arg_శిఖరం - 1);

    అయితే (అంకె_వాదనలు--) {
_       (Copy_దొంతర_శిఖరం_గమ్యం_స్థానం(o, arg_శిఖరం -= c_io_స్థానం_లెక్క));
_       (తీయి(o));
    }

    u16 i = 0;
    అయితే (అంకె_Rets--) {
        m3type_t type = Get_Func_Type_ఫలితం_Type(i_type, i++);

_       (నెట్టు(o, type, శిఖరం_స్థానం));
_       (MarkSlotsAllocatedByType(o, శిఖరం_స్థానం, type));

        శిఖరం_స్థానం += c_io_స్థానం_లెక్క;
    }

    } _catch: తిరిగి ఫలితం;
}

// A tail call hands the current frame over to the callee instead of stacking a new one.
// The callee's results have to be the enclosing function's results, so both use the same
// return slots and only the arguments move: they're staged above the caller's stack here
// (the argument expressions can still be reading the args/locals they're about to land on)
// and slid down onto the frame base at runtime, by op_ReturnCall[Indirect].
స్థిర
M3_ఫలితం Compile_తోక_Call_వాదనలు (IM3Compilation o, u16* o_దొంతర_స్థానభ్రంశం, u16* o_అంకె_Arg_Slots, IM3FuncType i_type, bool i_is_పరోక్ష)
{
_try {
    IM3FuncType funcType = o->function->funcType;

    u16 అంకె_Rets = Get_Func_Type_అంకె_Results(i_type);
    _throwif(m3Err_typeMismatch, అంకె_Rets != Get_Func_Type_అంకె_Results(funcType));

    కొరకు (u16 i = 0; i < అంకె_Rets; ++i) {
        _throwif(m3Err_typeMismatch, Get_Func_Type_ఫలితం_Type(i_type, i) != Get_Func_Type_ఫలితం_Type(funcType, i));
    }

    u16 శిఖరం_స్థానం = Get_Max_వాడిన_స్థానం_Plus_One(o);

    శిఖరం_స్థానం = M3_MAX(1, శిఖరం_స్థానం);

    // stack frame is 64-bit aligned
    Align_స్థానం_గమ్యం_Type(&శిఖరం_స్థానం, c_m3Type_i64);

    *o_దొంతర_స్థానభ్రంశం = శిఖరం_స్థానం;

    // wait to pop this here so that topSlot search is correct
    ఒకవేళ (i_is_పరోక్ష) {
_       (తీయి(o));
    }

    u16 అంకె_వాదనలు     = Get_Func_Type_అంకె_Params(i_type);
    u16 అంకె_Arg_Slots = అంకె_వాదనలు * c_io_స్థానం_లెక్క;

    *o_అంకె_Arg_Slots = అంకె_Arg_Slots;

    u16 arg_శిఖరం = శిఖరం_స్థానం + అంకె_Arg_Slots;
    _throwif(m3_Err_function_దొంతర_అధికప్రవాహం, arg_శిఖరం > d_m3MaxFunctionSlots);

    // the staging area is written by the caller, so op_Entry has to account for it
    Touch_స్థానం(o, arg_శిఖరం - 1);

    అయితే (అంకె_వాదనలు--) {
_       (Copy_దొంతర_శిఖరం_గమ్యం_స్థానం(o, arg_శిఖరం -= c_io_స్థానం_లెక్క));
_       (తీయి(o));
    }

    } _catch: తిరిగి ఫలితం;
}


#ఒకవేళ d_m3HasTypedRefs

// call_ref $t: the callee is the reference on top of the stack, so the operand
// order is the same as call_indirect's, minus the table.
స్థిర
M3_ఫలితం Compile_CallRef (IM3Compilation o, m3opcode_t i_opcode)
{
_try {
    u32 type_సూచీ;
_   (ReadLEB_u32(&type_సూచీ, &o->wasm, o->wasm_చివర));

    _throwif("function call type index out of range", type_సూచీ >= o->module->అంకె_Func_Types);

    IM3FuncType type = o->module->funcTypes[type_సూచీ];

    ఒకవేళ (not Is_దొంతర_Polymorphic(o)) {
        // whatever shape the reference has, it must be a function of this type
        m3type_t refType = Get_దొంతర_శిఖరం_Type(o);
        _throwif(m3Err_typeMismatch, not Is_Sub_Type_యొక్క(refType, Ref_Type_యొక్క_Func_Type(type, false)));
    }

    ఒకవేళ (Is_దొంతర_శిఖరం_లోపల_రిజిస్టరు(o)) {
_       (Preserve_రిజిస్టరు_If_Occupied(o, c_m3Type_funcref));
    }

    u16 function_స్థానం = Get_దొంతర_శిఖరం_స్థానం_సంఖ్య(o);

    bool isReturnCall = (i_opcode == c_waOp_returnCallRef);
    bool use_తోక_Call  = isReturnCall and d_m3_Can_తోక_Call;

    u16 exec_శిఖరం, అంకె_Arg_Slots = 0;

    ఒకవేళ (use_తోక_Call) {
_       (Compile_తోక_Call_వాదనలు(o, &exec_శిఖరం, &అంకె_Arg_Slots, type, true));
    } లేకపోతే {
_       (Compile_Call_వాదనలు_And_Return(o, &exec_శిఖరం, type, true));
    }

#  ఒకవేళ d_m3HasExceptionHandling
    ఒకవేళ (isReturnCall) {
_       (Emit_తీయి_Try_చట్రాలు_For_Return_Call(o));
    }
#  endif

_   (EmitOp(o, use_తోక_Call ? op_ReturnCallRef : op_CallRef));
    Emit_స్థానం_స్థానభ్రంశం(o, function_స్థానం);
    Emit_సూచిక(o, type);
    Emit_స్థానం_స్థానభ్రంశం(o, exec_శిఖరం);

    ఒకవేళ (not use_తోక_Call) {
_       (రికార్డు_Call(o, Get_Func_Type_అంకె_Results(type), exec_శిఖరం, isReturnCall));
    }

    ఒకవేళ (use_తోక_Call) {
        Emit_స్థానం_స్థానభ్రంశం(o, o->function->అంకె_Ret_Slots);
        EmitConstant32(o, అంకె_Arg_Slots);

_       (సమితి_దొంతర_Polymorphic(o));
    } లేకపోతే ఒకవేళ (isReturnCall) {
_       (Compile_Return(o, i_opcode));
    }

} _catch:
    తిరిగి ఫలితం;
}


// ref.as_non_null only sharpens the type; the value is unchanged, so the stack
// entry is re-pushed with the null stripped out of its type.
స్థిర
M3_ఫలితం Compile_Ref_AsNonNull (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_ఫలితం ఫలితం = m3Err_none;

    ఒకవేళ (not Is_దొంతర_Polymorphic(o)) {
        m3type_t type = Get_దొంతర_శిఖరం_Type(o);
        _throwif(m3Err_typeMismatch, not IsRefType(type));

        // op_RefAsNonNull only knows how to read its operand out of a slot; a
        // value still sitting in the register has to be spilled first, the way
        // every other single-op reader of the stack top does
        ఒకవేళ (Is_దొంతర_శిఖరం_లోపల_రిజిస్టరు(o)) {
_           (Preserve_రిజిస్టరు_If_Occupied(o, type));
        }

        u16 స్థానం = Get_దొంతర_శిఖరం_స్థానం_సంఖ్య(o);

_       (EmitOp(o, op_RefAsNonNull));
        Emit_స్థానం_స్థానభ్రంశం(o, స్థానం);

        m3type_t nonNull = IsSpelledRefType(type)
                             ? (m3type_t)(type | d_m3Type_refNonNull)
                             : (m3type_t)(d_m3Type_ref | d_m3Type_refNonNull | d_m3_Type_కుప్ప_Abstract |
                                          ((పునాది_Type_యొక్క(type) == c_m3Type_externref) ? d_m3Type_refExtern : 0));

        // the slot keeps the one reference it already had; a Pop()/Push() of
        // the same slot would drop that reference's count without anything
        // to put it back, so the type above it is corrected in place instead
        o->type_దొంతర[Get_దొంతర_శిఖరం_సూచీ(o)] = nonNull;
    }

    _catch: తిరిగి ఫలితం;
}

#అయితే_ముగింపు // d_m3HasTypedRefs


// How wide a table operand is read at comes from the table it names, so a slot
// holding a narrower type would be read past what was allocated for it. The
// validator refuses that already; this keeps a build without one from reaching
// the interpreter with it. c_m3Type_none means the operand is not on the
// dynamic stack to inspect, which is not this check's business.
స్థిర
M3_ఫలితం Check_నిర్వాహ్యం_Type (IM3Compilation o, u16 i_లోతు_From_శిఖరం, m3type_t i_type)
{
    m3type_t actual;

    ఒకవేళ (Is_దొంతర_Polymorphic(o)) {
        తిరిగి m3Err_none;
    }

    actual = Get_దొంతర_Type_From_శిఖరం(o, i_లోతు_From_శిఖరం);

    తిరిగి (actual == c_m3Type_none or పునాది_Type_యొక్క(actual) == i_type) ? m3Err_none
                                                                     : m3Err_typeMismatch;
}


// Swaps the _mem register onto a given memory. Only ever emitted in pairs --
// see op_SetMemory.
స్థిర
M3_ఫలితం Emit_సమితి_జ్ఞాపకం_Ptr (IM3Compilation o, IM3_జ్ఞాపకం i_జ్ఞాపకం)
{
    M3_ఫలితం ఫలితం = m3Err_none;

_   (EmitOp(o, op_సమితి_జ్ఞాపకం));
    Emit_సూచిక(o, i_జ్ఞాపకం);

    _catch: తిరిగి ఫలితం;
}


// ...onto one of the compiling module's own memories, by index. Only reached
// with a non-zero index under multi-memory, but the index has already been
// bounds-checked by ReadMemoryIndex either way.
స్థిర అంతఃవరుస
M3_ఫలితం Emit_సమితి_జ్ఞాపకం (IM3Compilation o, u32 i_జ్ఞాపకం_Idx)
{
#ఒకవేళ d_m3_Has_Multi_జ్ఞాపకం
    తిరిగి Emit_సమితి_జ్ఞాపకం_Ptr(o, o->module->memories[i_జ్ఞాపకం_Idx]);
#లేకపోతే
    (శూన్య)o;
    (శూన్య)i_జ్ఞాపకం_Idx;
    తిరిగి m3_Err_unknown_జ్ఞాపకం;
#అయితే_ముగింపు
}

స్థిర
M3_ఫలితం Compile_Call (IM3Compilation o, m3opcode_t i_opcode)
{
_try {
    u32 function_సూచీ;
_   (ReadLEB_u32(&function_సూచీ, &o->wasm, o->wasm_చివర));

    IM3Function function = Module_GetFunction(o->module, function_సూచీ);

    ఒకవేళ (function) {                                                     m3log (compile, d_indent " (func= [%d] '%s'; args= %d)",
                                                                                get_indention_string (o), function_సూచీ, m3_GetFunctionName (function), function->funcType->అంకె_వాదనలు);
        // an import linked to another module's export runs that module's body
        IM3Function లక్ష్యం = Function_Implementation(function);

        ఒకవేళ (లక్ష్యం->module) {
            bool isReturnCall = (i_opcode == c_waOp_returnCall);

            // The callee's body runs against its own module's memory, so a call
            // that crosses modules is bracketed by a pair of op_SetMemory. A
            // tail call never returns to run the second one, so those compile
            // as a plain call followed by a return instead.
            bool crossModule = (లక్ష్యం->module != o->module);
            bool use_తోక_Call = isReturnCall and d_m3_Can_తోక_Call and not crossModule;

            u16 స్థానం_శిఖరం, అంకె_Arg_Slots = 0;

            ఒకవేళ (use_తోక_Call) {
_               (Compile_తోక_Call_వాదనలు(o, &స్థానం_శిఖరం, &అంకె_Arg_Slots, లక్ష్యం->funcType, false));
            } లేకపోతే {
_               (Compile_Call_వాదనలు_And_Return(o, &స్థానం_శిఖరం, లక్ష్యం->funcType, false));
            }

            IM3Operation op;
            స్థిరరాశి శూన్య*  నిర్వాహ్యం;

            ఒకవేళ (లక్ష్యం->compiled) {
                op      = use_తోక_Call ? op_ReturnCall : op_Call;
                నిర్వాహ్యం = లక్ష్యం->compiled;
            } లేకపోతే {
                op      = use_తోక_Call ? op_CompileReturnCall : op_Compile;
                నిర్వాహ్యం = లక్ష్యం;
            }

#ఒకవేళ d_m3HasExceptionHandling
            ఒకవేళ (isReturnCall) {
_               (Emit_తీయి_Try_చట్రాలు_For_Return_Call(o));
            }
#అయితే_ముగింపు

            ఒకవేళ (crossModule) {
_               (Emit_సమితి_జ్ఞాపకం_Ptr(o, Module_Memory0(లక్ష్యం->module)));
            }

_           (EmitOp(o, op));
            Emit_సూచిక(o, నిర్వాహ్యం);
            Emit_స్థానం_స్థానభ్రంశం(o, స్థానం_శిఖరం);

            ఒకవేళ (not use_తోక_Call) {
                // a suspend inside the callee leaves this frame waiting here, its
                // results not yet written
_               (రికార్డు_Call(o, Get_Func_Type_అంకె_Results(లక్ష్యం->funcType), స్థానం_శిఖరం, isReturnCall));
            }

            ఒకవేళ (use_తోక_Call) {
                Emit_స్థానం_స్థానభ్రంశం(o, o->function->అంకె_Ret_Slots);
                EmitConstant32(o, అంకె_Arg_Slots);

_               (సమితి_దొంతర_Polymorphic(o));
            } లేకపోతే {
                ఒకవేళ (crossModule) {
_                   (Emit_సమితి_జ్ఞాపకం_Ptr(o, Module_Memory0(o->module)));
                }

                ఒకవేళ (isReturnCall) {
_                   (Compile_Return(o, i_opcode));
                }
            }
        } లేకపోతే {
            _throw(ErrorCompile(m3Err_functionImportMissing, o, "'%s.%s'", GetFunctionImportModuleName(function), m3_GetFunctionName(function)));
        }
    } లేకపోతే {
        _throw(ErrorCompile(m3Err_functionLookupFailed, o, "index: %d", function_సూచీ));
    }

    } _catch: తిరిగి ఫలితం;
}

స్థిర
M3_ఫలితం Compile_Call_పరోక్ష (IM3Compilation o, m3opcode_t i_opcode)
{
_try {
    u32 type_సూచీ;
_   (ReadLEB_u32(&type_సూచీ, &o->wasm, o->wasm_చివర));

    u32 పట్టిక_సూచీ;
_   (ReadLEB_u32(&పట్టిక_సూచీ, &o->wasm, o->wasm_చివర));

    _throwif("function call type index out of range", type_సూచీ >= o->module->అంకె_Func_Types);
    _throwif("table index out of range", పట్టిక_సూచీ >= o->module->అంకె_Tables);
    _throwif(m3Err_typeMismatch, పునాది_Type_యొక్క(o->module->tables[పట్టిక_సూచీ]->type) != c_m3Type_funcref);

_   (Check_నిర్వాహ్యం_Type(o, 0, పట్టిక_Addr_Type(o->module->tables[పట్టిక_సూచీ])));

    ఒకవేళ (Is_దొంతర_శిఖరం_లోపల_రిజిస్టరు(o)) {
_       (Preserve_రిజిస్టరు_If_Occupied(o, c_m3Type_i32));
    }

    u16 పట్టిక_సూచీ_స్థానం = Get_దొంతర_శిఖరం_స్థానం_సంఖ్య(o);

    bool isReturnCall = (i_opcode == c_wa_Op_return_Call_పరోక్ష);
    bool use_తోక_Call  = isReturnCall and d_m3_Can_తోక_Call;

    u16         exec_శిఖరం, అంకె_Arg_Slots = 0;
    IM3FuncType type = o->module->funcTypes[type_సూచీ];

    ఒకవేళ (use_తోక_Call) {
_       (Compile_తోక_Call_వాదనలు(o, &exec_శిఖరం, &అంకె_Arg_Slots, type, true));
    } లేకపోతే {
_       (Compile_Call_వాదనలు_And_Return(o, &exec_శిఖరం, type, true));
    }

#ఒకవేళ d_m3HasExceptionHandling
    ఒకవేళ (isReturnCall) {
_       (Emit_తీయి_Try_చట్రాలు_For_Return_Call(o));
    }
#అయితే_ముగింపు

    // the table comes first: how wide its indexes are is what says how much of
    // the slot below holds one
_   (EmitOp(o, use_తోక_Call ? op_Return_Call_పరోక్ష : op_Call_పరోక్ష));
    Emit_సూచిక(o, o->module->tables[పట్టిక_సూచీ]);
    Emit_స్థానం_స్థానభ్రంశం(o, పట్టిక_సూచీ_స్థానం);
    Emit_సూచిక(o, type);
    Emit_స్థానం_స్థానభ్రంశం(o, exec_శిఖరం);

    ఒకవేళ (not use_తోక_Call) {
_       (రికార్డు_Call(o, Get_Func_Type_అంకె_Results(type), exec_శిఖరం, isReturnCall));
    }

    ఒకవేళ (use_తోక_Call) {
        Emit_స్థానం_స్థానభ్రంశం(o, o->function->అంకె_Ret_Slots);
        EmitConstant32(o, అంకె_Arg_Slots);

_       (సమితి_దొంతర_Polymorphic(o));
    } లేకపోతే ఒకవేళ (isReturnCall) {
_       (Compile_Return(o, i_opcode));
    }

} _catch:
    తిరిగి ఫలితం;
}

// Reads the memory index a memory instruction names, and checks it against the
// module's index space. Without multi-memory the immediate is a reserved byte
// that has to be zero, which is the same check.
స్థిర
M3_ఫలితం Read_జ్ఞాపకం_సూచీ (IM3Compilation o, u32* o_జ్ఞాపకం_Idx)
{
    M3_ఫలితం ఫలితం;

_   (ReadLEB_u32(o_జ్ఞాపకం_Idx, &o->wasm, o->wasm_చివర));

    _throwif(m3_Err_unknown_జ్ఞాపకం, *o_జ్ఞాపకం_Idx >= o->module->అంకె_Memories);

    _catch: తిరిగి ఫలితం;
}

స్థిర
M3_ఫలితం Compile_జ్ఞాపకం_కొలత (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_ఫలితం ఫలితం;

    u32 జ్ఞాపకం_Idx;

    // A page count is given in the memory's own address type. Declared up here
    // so the throws below don't jump over its initialization.
    m3type_t addrType = c_m3Type_i32;

_   (Read_జ్ఞాపకం_సూచీ(o, &జ్ఞాపకం_Idx));

    addrType = జ్ఞాపకం_Addr_Type(o->module->memories[జ్ఞాపకం_Idx]);

_   (Preserve_రిజిస్టరు_If_Occupied(o, addrType));

    ఒకవేళ (జ్ఞాపకం_Idx) {
_       (Emit_సమితి_జ్ఞాపకం(o, జ్ఞాపకం_Idx));
    }

    // op_MemSize writes the whole register either way; the slot it is pushed
    // to is what decides how much of it the module gets to see
_   (EmitOp(o, op_Mem_కొలత));

    ఒకవేళ (జ్ఞాపకం_Idx) {
_       (Emit_సమితి_జ్ఞాపకం(o, 0));
    }

_   (నెట్టు_రిజిస్టరు(o, addrType));

    _catch: తిరిగి ఫలితం;
}

స్థిర
M3_ఫలితం Compile_జ్ఞాపకం_Grow (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_ఫలితం ఫలితం;

    u32 జ్ఞాపకం_Idx;

    // see Compile_Memory_Size
    m3type_t addrType = c_m3Type_i32;

_   (Read_జ్ఞాపకం_సూచీ(o, &జ్ఞాపకం_Idx));

    addrType = జ్ఞాపకం_Addr_Type(o->module->memories[జ్ఞాపకం_Idx]);

_   (Copy_దొంతర_శిఖరం_గమ్యం_రిజిస్టరు(o, false));
_   (తీయి_Type(o, addrType));

    ఒకవేళ (జ్ఞాపకం_Idx) {
_       (Emit_సమితి_జ్ఞాపకం(o, జ్ఞాపకం_Idx));
    }

#ఒకవేళ d_m3HasMemory64
_   (EmitOp(o, (addrType == c_m3Type_i64) ? op_MemGrow64 : op_MemGrow));
#లేకపోతే
_   (EmitOp(o, op_MemGrow));
#అయితే_ముగింపు

    ఒకవేళ (జ్ఞాపకం_Idx) {
_       (Emit_సమితి_జ్ఞాపకం(o, 0));
    }

_   (నెట్టు_రిజిస్టరు(o, addrType));

    _catch: తిరిగి ఫలితం;
}

స్థిర
M3_ఫలితం Compile_జ్ఞాపకం_Copy_Fill (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_ఫలితం ఫలితం = m3Err_none;

    // memory.copy names the destination first and then the source; memory.fill
    // names only the one it writes.
    u32  ఆధారం_జ్ఞాపకం_Idx = 0, లక్ష్యం_జ్ఞాపకం_Idx = 0;
    bool isCopy = (i_opcode == c_wa_Op_జ్ఞాపకం_Copy);

    // Each address is typed by the memory it belongs to. The length follows
    // the narrower of the two, so it is 64-bit only when both are - which for
    // memory.fill, naming one memory twice over, means whenever that one is.
    // Declared up here so the throws below don't jump over them.
    m3type_t లక్ష్యం_Type = c_m3Type_i32;
    m3type_t ఆధారం_Type = c_m3Type_i32;
    m3type_t నిడివి_Type = c_m3Type_i32;

    _throwif(m3Err_wasmMalformed, o->module->అంకె_Memories == 0);

_   (Read_జ్ఞాపకం_సూచీ(o, &లక్ష్యం_జ్ఞాపకం_Idx));

    ఒకవేళ (isCopy) {
_       (Read_జ్ఞాపకం_సూచీ(o, &ఆధారం_జ్ఞాపకం_Idx));
    }

    లక్ష్యం_Type = జ్ఞాపకం_Addr_Type(o->module->memories[లక్ష్యం_జ్ఞాపకం_Idx]);
    ఆధారం_Type = isCopy ? జ్ఞాపకం_Addr_Type(o->module->memories[ఆధారం_జ్ఞాపకం_Idx])
                        : లక్ష్యం_Type;
    నిడివి_Type = (లక్ష్యం_Type == c_m3Type_i64 and ఆధారం_Type == c_m3Type_i64)
                   ? c_m3Type_i64
                   : c_m3Type_i32;

_   (Copy_దొంతర_శిఖరం_గమ్యం_రిజిస్టరు(o, false));

#ఒకవేళ d_m3_Has_Multi_జ్ఞాపకం
    ఒకవేళ (isCopy and ఆధారం_జ్ఞాపకం_Idx != లక్ష్యం_జ్ఞాపకం_Idx) {
        // two memories at once: _mem can only name one, so the op takes both
_       (EmitOp(o, op_MemCopy_x));
        Emit_సూచిక(o, o->module->memories[లక్ష్యం_జ్ఞాపకం_Idx]);
        Emit_సూచిక(o, o->module->memories[ఆధారం_జ్ఞాపకం_Idx]);

        // ...and, since they need not be addressed alike, how wide to read
        // each of the operands: bit 0 the destination, bit 1 the source
        EmitConstant32(o, ((లక్ష్యం_Type == c_m3Type_i64) ? 0x1u : 0u) |
                            ((ఆధారం_Type == c_m3Type_i64) ? 0x2u : 0u));
    } లేకపోతే
#అయితే_ముగింపు
    {
        IM3Operation op = isCopy ? op_MemCopy : op_MemFill;

#ఒకవేళ d_m3HasMemory64
        ఒకవేళ (లక్ష్యం_Type == c_m3Type_i64) {
            op = isCopy ? op_MemCopy64 : op_MemFill64;
        }
#అయితే_ముగింపు

        ఒకవేళ (లక్ష్యం_జ్ఞాపకం_Idx) {
_           (Emit_సమితి_జ్ఞాపకం(o, లక్ష్యం_జ్ఞాపకం_Idx));
        }

_       (EmitOp(o, op));
    }

    // the length is in the register; the other two are named by slot, at
    // whatever width their stack entries were pushed with
_   (తీయి_Type(o, నిడివి_Type));
_   (Emit_స్థానం_అంకె_యొక్క_దొంతర_శిఖరం_And_తీయి(o));      // the source, or the byte to fill with
_   (Emit_స్థానం_అంకె_యొక్క_దొంతర_శిఖరం_And_తీయి(o));      // the destination

    ఒకవేళ (లక్ష్యం_జ్ఞాపకం_Idx and not (isCopy and ఆధారం_జ్ఞాపకం_Idx != లక్ష్యం_జ్ఞాపకం_Idx)) {
_       (Emit_సమితి_జ్ఞాపకం(o, 0));
    }

    _catch: తిరిగి ఫలితం;
}


// memory.init and data.drop address a data segment by index. Both are only valid
// when a data count section declared the segments up front.
స్థిర
M3_ఫలితం Read_దత్తాంశం_Segment (IM3Compilation o, M3_దత్తాంశం_Segment** o_segment)
{
    M3_ఫలితం ఫలితం = m3Err_none;

    u32 index;
_   (ReadLEB_u32(&index, &o->wasm, o->wasm_చివర));

    _throwif("data count section required", not o->module->has_దత్తాంశం_లెక్క);
    _throwif(m3Err_wasmMalformed, index >= o->module->అంకె_దత్తాంశం_Segments);

    *o_segment = &o->module->దత్తాంశం_Segments[index];

    _catch: తిరిగి ఫలితం;
}


M3_ఫలితం Compile_జ్ఞాపకం_Init (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_ఫలితం ఫలితం = m3Err_none;

    M3_దత్తాంశం_Segment* segment = ఖాళీ;
    u32            జ్ఞాపకం_Idx;

    _throwif(m3Err_wasmMalformed, o->module->అంకె_Memories == 0);

_   (Read_దత్తాంశం_Segment(o, &segment));
_   (Read_జ్ఞాపకం_సూచీ(o, &జ్ఞాపకం_Idx));

_   (Copy_దొంతర_శిఖరం_గమ్యం_రిజిస్టరు(o, false));

    ఒకవేళ (జ్ఞాపకం_Idx) {
_       (Emit_సమితి_జ్ఞాపకం(o, జ్ఞాపకం_Idx));
    }

#ఒకవేళ d_m3HasMemory64
    // only the destination follows the memory's address type: a data segment
    // is indexed as an i32 however the memory it is copied into is addressed
_   (EmitOp(o, o->module->memories[జ్ఞాపకం_Idx]->isMemory64 ? op_MemInit64 : op_MemInit));
#లేకపోతే
_   (EmitOp(o, op_MemInit));
#అయితే_ముగింపు
    Emit_సూచిక(o, segment);
_   (తీయి_Type(o, c_m3Type_i32));
_   (Emit_స్థానం_అంకె_యొక్క_దొంతర_శిఖరం_And_తీయి(o));
_   (Emit_స్థానం_అంకె_యొక్క_దొంతర_శిఖరం_And_తీయి(o));

    ఒకవేళ (జ్ఞాపకం_Idx) {
_       (Emit_సమితి_జ్ఞాపకం(o, 0));
    }

    _catch: తిరిగి ఫలితం;
}


#ఒకవేళ d_m3HasRefTypes

స్థిర
M3_ఫలితం Compile_Select (IM3Compilation o, m3opcode_t i_opcode);

// select with an explicit result type vector. The types only matter to the
// validator; the operands are laid out exactly as for the untyped select.
M3_ఫలితం Compile_Select_Typed (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_ఫలితం ఫలితం = m3Err_none;

    u32 అంకె_Types;
_   (ReadLEB_u32(&అంకె_Types, &o->wasm, o->wasm_చివర));
    _throwif(m3Err_wasmMalformed, అంకె_Types != 1);

    i8 waType;
    u8 type;
_   (ReadLEB_i7(&waType, &o->wasm, o->wasm_చివర));
_   (NormalizeType(&type, waType));

_   (Compile_Select(o, i_opcode));

    _catch: తిరిగి ఫలితం;
}


// A reference is one pointer-sized word and null is 0, so null-testing it is
// just an integer eqz of the matching width.
#  ఒకవేళ M3_SIZEOF_PTR == 8
#    define d_m3RefIsNull_r   op_i64_Equal_గమ్యం_Zero_r
#    define d_m3RefIsNull_s   op_i64_Equal_గమ్యం_Zero_s
#  లేకపోతే
#    define d_m3RefIsNull_r   op_i32_Equal_గమ్యం_Zero_r
#    define d_m3RefIsNull_s   op_i32_Equal_గమ్యం_Zero_s
#  endif

స్థిర
M3_ఫలితం ReadRefType (IM3Compilation o, m3type_t* o_type)
{
    M3_ఫలితం ఫలితం = m3Err_none;

#  ఒకవేళ d_m3HasTypedRefs
    // ref.null names a heap type: func, extern, or a function type index
    m3type_t కుప్ప_బిట్లు;
_   (Parse_కుప్ప_Type(o->module, &కుప్ప_బిట్లు, &o->wasm, o->wasm_చివర));

    *o_type = d_m3Type_ref | కుప్ప_బిట్లు;
#  లేకపోతే
    i8 waType;
    u8 plainType;
_   (ReadLEB_i7(&waType, &o->wasm, o->wasm_చివర));
_   (NormalizeType(&plainType, waType));
    *o_type = plainType;
    _throwif(m3Err_wasmMalformed, not IsRefType(*o_type));
#  endif

    _catch: తిరిగి ఫలితం;
}


M3_ఫలితం Compile_Ref_Null (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_ఫలితం ఫలితం = m3Err_none;

    m3type_t type;
_   (ReadRefType(o, &type));
_   (నెట్టు_Const(o, 0, type));

    _catch: తిరిగి ఫలితం;
}


M3_ఫలితం Compile_Ref_IsNull (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_ఫలితం ఫలితం = m3Err_none;

    IM3Operation op;

    ఒకవేళ (not Is_దొంతర_Polymorphic(o)) {
        _throwif(m3Err_typeMismatch, not IsRefType(Get_దొంతర_శిఖరం_Type(o)));
    }

    ఒకవేళ (Is_దొంతర_శిఖరం_లోపల_రిజిస్టరు(o)) {
        op = d_m3RefIsNull_r;
    } లేకపోతే {
_       (Preserve_రిజిస్టరు_If_Occupied(o, c_m3Type_i32));
        op = d_m3RefIsNull_s;
    }

_   (EmitOp(o, op));
_   (Emit_స్థానం_అంకె_యొక్క_దొంతర_శిఖరం_And_తీయి(o));
_   (నెట్టు_రిజిస్టరు(o, c_m3Type_i32));

    _catch: తిరిగి ఫలితం;
}


M3_ఫలితం Compile_Ref_Func (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_ఫలితం ఫలితం = m3Err_none;

    u32 func_సూచీ;

    // declared before the throws below, which jump past this point to _catch
    m3type_t    refType   = c_m3Type_funcref;
    IM3Function reference = ఖాళీ;

_   (ReadLEB_u32(&func_సూచీ, &o->wasm, o->wasm_చివర));
    _throwif("function index out of range", func_సూచీ >= o->module->అంకె_Functions);

    // Inside a constant expression ref.func is itself a declaration; inside a
    // function body the function must already have been declared elsewhere.
    ఒకవేళ (o->function) {
        _throwif("undeclared function reference", not Module_IsFunctionDeclared(o->module, func_సూచీ));
    } లేకపోతే {
_       (Module_DeclareFunction(o->module, func_సూచీ));
    }

#  ఒకవేళ d_m3HasTypedRefs
    refType = Ref_Type_యొక్క_Func_Type(o->module->functions[func_సూచీ].funcType, true);
#  endif

    // A reference denotes the function that actually runs, so a ref.func naming
    // an import linked to another module is the same reference that module's
    // own ref.func would produce - and carries the defining module with it.
    reference = Function_Implementation(&o->module->functions[func_సూచీ]);

_   (నెట్టు_Const(o, (u64)(uintptr_t)reference, refType));

    _catch: తిరిగి ఫలితం;
}


// table.get/set/size/grow/fill all name a table by index; the table struct goes
// into the codestream so the operation doesn't have to walk the module.
స్థిర
M3_ఫలితం Read_పట్టిక (IM3Compilation o, M3_పట్టిక** o_పట్టిక)
{
    M3_ఫలితం ఫలితం = m3Err_none;

    u32 index;
_   (ReadLEB_u32(&index, &o->wasm, o->wasm_చివర));
    _throwif("table index out of range", index >= o->module->అంకె_Tables);

    *o_పట్టిక = o->module->tables[index];

    _catch: తిరిగి ఫలితం;
}


// The table operations take every operand from a slot, so spill the integer
// register first: after this each EmitSlotNumOfStackTopAndPop () emits a real
// slot offset, and the register is free for the result. Operands are emitted
// from the top of the stack downwards, which is the order the ops read them.
స్థిర
M3_ఫలితం Compile_పట్టిక_Op (IM3Compilation o, IM3Operation i_op, M3_పట్టిక* i_పట్టిక, u32 i_అంకె_Operands, m3type_t i_retType)
{
    M3_ఫలితం ఫలితం = m3Err_none;

_   (Preserve_రిజిస్టరు_If_Occupied(o, c_m3Type_i64));

_   (EmitOp(o, i_op));
    Emit_సూచిక(o, i_పట్టిక);

    కొరకు (u32 i = 0; i < i_అంకె_Operands; ++i) {
_       (Emit_స్థానం_అంకె_యొక్క_దొంతర_శిఖరం_And_తీయి(o));
    }

    ఒకవేళ (i_retType != c_m3Type_none) {
_       (నెట్టు_రిజిస్టరు(o, i_retType));
    }

    _catch: తిరిగి ఫలితం;
}


M3_ఫలితం Compile_పట్టిక_Get_సమితి (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_ఫలితం ఫలితం = m3Err_none;

    M3_పట్టిక* పట్టిక_2;
_   (Read_పట్టిక(o, &పట్టిక_2));

    ఒకవేళ (i_opcode == c_wa_Op_పట్టిక_Get) {
_       (Check_నిర్వాహ్యం_Type(o, 0, పట్టిక_Addr_Type(పట్టిక_2)));
_       (Compile_పట్టిక_Op(o, op_పట్టిక_Get, పట్టిక_2, 1, పట్టిక_2->type));
    } లేకపోతే {
_       (Check_నిర్వాహ్యం_Type(o, 1, పట్టిక_Addr_Type(పట్టిక_2)));
_       (Compile_పట్టిక_Op(o, op_పట్టిక_సమితి, పట్టిక_2, 2, c_m3Type_none));
    }

    _catch: తిరిగి ఫలితం;
}


M3_ఫలితం Compile_పట్టిక_Init (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_ఫలితం ఫలితం = m3Err_none;

    u32      elem_సూచీ;
    M3_పట్టిక* పట్టిక_2;

_   (ReadLEB_u32(&elem_సూచీ, &o->wasm, o->wasm_చివర));
    _throwif("element segment index out of range", elem_సూచీ >= o->module->అంకె_Element_Segments);
_   (Read_పట్టిక(o, &పట్టిక_2));

_   (Check_నిర్వాహ్యం_Type(o, 2, పట్టిక_Addr_Type(పట్టిక_2)));

_   (Preserve_రిజిస్టరు_If_Occupied(o, c_m3Type_i64));
_   (EmitOp(o, op_పట్టిక_Init));
    Emit_సూచిక(o, పట్టిక_2);
    Emit_సూచిక(o, &o->module->elementSegments[elem_సూచీ]);

    కొరకు (u32 i = 0; i < 3; ++i) {
_       (Emit_స్థానం_అంకె_యొక్క_దొంతర_శిఖరం_And_తీయి(o));
    }

    _catch: తిరిగి ఫలితం;
}


M3_ఫలితం Compile_Elem_Drop (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_ఫలితం ఫలితం = m3Err_none;

    u32 elem_సూచీ;
_   (ReadLEB_u32(&elem_సూచీ, &o->wasm, o->wasm_చివర));
    _throwif("element segment index out of range", elem_సూచీ >= o->module->అంకె_Element_Segments);

_   (EmitOp(o, op_ElemDrop));
    Emit_సూచిక(o, &o->module->elementSegments[elem_సూచీ]);

    _catch: తిరిగి ఫలితం;
}


M3_ఫలితం Compile_పట్టిక_Copy (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_ఫలితం ఫలితం = m3Err_none;

    M3_పట్టిక* dst;
    M3_పట్టిక* src;

_   (Read_పట్టిక(o, &dst));
_   (Read_పట్టిక(o, &src));
    _throwif(m3Err_typeMismatch, not Is_Sub_Type_యొక్క(src->type, dst->type));

_   (Check_నిర్వాహ్యం_Type(o, 2, పట్టిక_Addr_Type(dst)));
_   (Check_నిర్వాహ్యం_Type(o, 1, పట్టిక_Addr_Type(src)));
_   (Check_నిర్వాహ్యం_Type(o, 0, (dst->isTable64 and src->isTable64) ? c_m3Type_i64 : c_m3Type_i32));

_   (Preserve_రిజిస్టరు_If_Occupied(o, c_m3Type_i64));
_   (EmitOp(o, op_పట్టిక_Copy));
    Emit_సూచిక(o, dst);
    Emit_సూచిక(o, src);

    కొరకు (u32 i = 0; i < 3; ++i) {
_       (Emit_స్థానం_అంకె_యొక్క_దొంతర_శిఖరం_And_తీయి(o));
    }

    _catch: తిరిగి ఫలితం;
}


M3_ఫలితం Compile_పట్టిక_కొలత (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_ఫలితం ఫలితం = m3Err_none;

    M3_పట్టిక* పట్టిక_2;
_   (Read_పట్టిక(o, &పట్టిక_2));

    // a table size is given in the table's own index type
_   (Compile_పట్టిక_Op(o, op_పట్టిక_కొలత, పట్టిక_2, 0, పట్టిక_Addr_Type(పట్టిక_2)));

    _catch: తిరిగి ఫలితం;
}


M3_ఫలితం Compile_పట్టిక_Grow_Fill (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_ఫలితం ఫలితం = m3Err_none;

    M3_పట్టిక* పట్టిక_2;
_   (Read_పట్టిక(o, &పట్టిక_2));

    ఒకవేళ (i_opcode == c_wa_Op_పట్టిక_Grow) {
_       (Check_నిర్వాహ్యం_Type(o, 0, పట్టిక_Addr_Type(పట్టిక_2)));
_       (Compile_పట్టిక_Op(o, op_పట్టిక_Grow, పట్టిక_2, 2, పట్టిక_Addr_Type(పట్టిక_2)));
    } లేకపోతే {
_       (Check_నిర్వాహ్యం_Type(o, 0, పట్టిక_Addr_Type(పట్టిక_2)));
_       (Check_నిర్వాహ్యం_Type(o, 2, పట్టిక_Addr_Type(పట్టిక_2)));
_       (Compile_పట్టిక_Op(o, op_పట్టిక_Fill, పట్టిక_2, 3, c_m3Type_none));
    }

    _catch: తిరిగి ఫలితం;
}

#అయితే_ముగింపు // d_m3HasRefTypes


M3_ఫలితం Compile_దత్తాంశం_Drop (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_ఫలితం ఫలితం = m3Err_none;

    M3_దత్తాంశం_Segment* segment = ఖాళీ;
_   (Read_దత్తాంశం_Segment(o, &segment));

_   (EmitOp(o, op_దత్తాంశం_Drop));
    Emit_సూచిక(o, segment);

    _catch: తిరిగి ఫలితం;
}


స్థిర
M3_ఫలితం Read_ఖండం_Type (IM3Compilation o, IM3FuncType* o_ఖండం_Type)
{
    M3_ఫలితం ఫలితం = m3Err_none;

#ఒకవేళ d_m3HasTypedRefs
    // a spelled-out reference type is two bytes, so it cannot be told apart
    // from a type index by the s33 that block types otherwise use
    ఒకవేళ (o->wasm < o->wasm_చివర and (*o->wasm == d_waEncode_ref or *o->wasm == d_waEncode_refNull)) {
        m3type_t refType;
_       (Parse_విలువ_Type(o->module, &refType, &o->wasm, o->wasm_చివర));
        IM3FuncType ftype = ఖాళీ;
_       (AllocFuncType(&ftype, 1));
        ftype->అంకె_Rets  = 1;
        ftype->types[0] = refType;
_       (Environment_కూడు_Func_Type(o->module->environment, &ftype));
        *o_ఖండం_Type = ftype;
        తిరిగి ఫలితం;
    }
#అయితే_ముగింపు

    i64 type;
_   (ReadLebSigned(&type, 33, &o->wasm, o->wasm_చివర));

    ఒకవేళ (type < 0) {
        _throwif(m3Err_invalidTypeId, type < -64);
        u8 విలువ_Type;
_       (NormalizeType(&విలువ_Type, (i8)type));                                 m3log (compile, d_indent " (type: %s)", get_indention_string (o), c_waTypes [విలువ_Type]);
        *o_ఖండం_Type = o->module->environment->retFuncTypes[విలువ_Type];
    } లేకపోతే {
        _throwif("func type out of bounds", type >= o->module->అంకె_Func_Types);
        *o_ఖండం_Type = o->module->funcTypes[type];                         m3log (compile, d_indent " (type: %s)", get_indention_string (o), SPrintFuncTypeSignature (*o_ఖండం_Type));
    }
    _catch: తిరిగి ఫలితం;
}

స్థిర
M3_ఫలితం Preserve_వాదనలు_And_Locals (IM3Compilation o)
{
    M3_ఫలితం ఫలితం = m3Err_none;

    ఒకవేళ (o->దొంతర_సూచీ > o->దొంతర_మొదటి_Dynamic_సూచీ) {
        u32 అంకె_వాదనలు_And_Locals = Get_Function_అంకె_వాదనలు_And_Locals(o->function);

        కొరకు (u32 i = 0; i < అంకె_వాదనలు_And_Locals; ++i) {
            u16 స్థానం = Get_స్థానం_For_దొంతర_సూచీ(o, i);

            u16 preserved_స్థానం_సంఖ్య;
_           (కనుగొను_Referenced_Local_Within_Current_ఖండం(o, &preserved_స్థానం_సంఖ్య, స్థానం));

            ఒకవేళ (preserved_స్థానం_సంఖ్య != స్థానం) {
                m3type_t     type = Get_దొంతర_Type_From_Bottom(o, i);                 d_m3Assert (type != c_m3Type_none)
                IM3Operation op   = Is64_బిట్టు_Type(type) ? op_Copy_స్థానం_64 : op_Copy_స్థానం_32;

                EmitOp(o, op);
                Emit_స్థానం_స్థానభ్రంశం(o, preserved_స్థానం_సంఖ్య);
                Emit_స్థానం_స్థానభ్రంశం(o, స్థానం);
            }
        }
    }

    _catch:
    తిరిగి ఫలితం;
}

స్థిర
M3_ఫలితం Compile_Loop_Or_ఖండం (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_ఫలితం ఫలితం;

    IM3FuncType ఖండం_Type = ఖాళీ;

_   (PreserveRegisters(o));
_   (Preserve_వాదనలు_And_Locals(o));

_   (Read_ఖండం_Type(o, &ఖండం_Type));

    ఒకవేళ (i_opcode == c_waOp_loop) {
        u16 అంకె_Params = Get_Func_Type_అంకె_Params(ఖండం_Type);
        ఒకవేళ (అంకె_Params) {
            // instantiate constants
            u16 అంకె_విలువలు = Get_అంకె_ఖండం_విలువలు_On_దొంతర(o);

            ఒకవేళ (అంకె_విలువలు >= అంకె_Params) {
                u16 దొంతర_శిఖరం = Get_దొంతర_శిఖరం_సూచీ(o) + 1;

                కొరకు (u16 i = దొంతర_శిఖరం - అంకె_Params; i < దొంతర_శిఖరం; ++i) {
                    u16      స్థానం = Get_స్థానం_For_దొంతర_సూచీ(o, i);
                    m3type_t type = Get_దొంతర_Type_From_Bottom(o, i);

                    ఒకవేళ (Is_Constant_స్థానం(o, స్థానం)) {
                        u16 new_స్థానం = c_స్థానం_వాడని;
_                       (AllocateSlots(o, &new_స్థానం, type));
_                       (Copy_దొంతర_సూచీ_గమ్యం_స్థానం(o, new_స్థానం, i));
                        o->wasm_దొంతర[i] = new_స్థానం;
                    }
                }
            }
        }

_       (EmitOp(o, op_Loop));
    } లేకపోతే {
    }

_   (Compile_ఖండం(o, ఖండం_Type, i_opcode));

    _catch: తిరిగి ఫలితం;
}

#ఒకవేళ d_m3HasExceptionHandling

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
స్థిర
M3_ఫలితం EmitCatchStubs (IM3Compilation o)
{
    IM3_సంకేతం_పుట displaced_పుట = ఖాళీ;
_try {
    M3CatchClause* clauses    = o->tryClauses;
    u32            అంకె_Clauses = o->అంకె_Try_Clauses;

    // the body must not see these: a nested try_table sets up its own
    o->tryClauses    = ఖాళీ;
    o->అంకె_Try_Clauses = 0;

    కొరకు (u32 i = 0; i < అంకె_Clauses; ++i) {
        M3CatchClause* clause = &clauses[i];

        IM3CompilationScope scope;
_       (Get_ఖండం_Scope(o, &scope, clause->label_లోతు + 1));

        IM3FuncType payload = clause->ట్యాగ్ ? clause->ట్యాగ్->type : ఖాళీ;
        u16         అంకె_వాదనలు = Get_Func_Type_అంకె_Params(payload);

        // What the stub hands the label - the payload, then the exnref for a
        // _ref clause - has to be what the label takes. ResolveBlockResults
        // only counts the values, and the validator compares base types, so a
        // typed reference is only checked here: (ref null $t) reaching a label
        // that takes (ref $t) has to be rejected.
        {
            bool isLoop         = (scope->opcode == c_waOp_loop);
            u16  అంకె_Label_విలువలు = isLoop ? Get_Func_Type_అంకె_Params(scope->type)
                                         : Get_Func_Type_అంకె_Results(scope->type);

            _throwif(m3_Err_type_లెక్క_Mismatch, అంకె_Label_విలువలు != అంకె_వాదనలు + (clause->hasRef ? 1 : 0));

            కొరకు (u16 a = 0; a < అంకె_Label_విలువలు; ++a) {
                m3type_t given  = (a < అంకె_వాదనలు) ? GetFuncTypeParamType(payload, a)
                                                : (m3type_t)c_m3Type_exnref;
                m3type_t wanted = isLoop ? GetFuncTypeParamType(scope->type, a)
                                         : Get_Func_Type_ఫలితం_Type(scope->type, a);

                _throwif(m3Err_typeMismatch, not Is_Sub_Type_యొక్క(given, wanted));
            }
        }

        IM3_సంకేతం_పుట stub_పుట;
_       (Acquire_Compilation_సంకేతం_పుట(o, &stub_పుట));

        pc_t ప్రారంభం_PC  = Get_పుట_PC(stub_పుట);
        displaced_పుట = o->పుట;
        o->పుట       = stub_పుట;

        u16 మొదటి_సూచీ = o->దొంతర_సూచీ;

        కొరకు (u16 a = 0; a < అంకె_వాదనలు; ++a) {
_           (నెట్టు_Allocated_స్థానం(o, GetFuncTypeParamType(payload, a)));
        }

        ఒకవేళ (clause->hasRef) {
_           (నెట్టు_Allocated_స్థానం(o, c_m3Type_exnref));
        }

        // emitted even with nothing to move: this is also where an exception
        // no clause will reify gets released
_       (Ensure_సంకేతం_పుట_అంకె_పంక్తులు(o, 2 * అంకె_వాదనలు + 3 + d_m3_సంకేతం_పుట_Free_పంక్తులు_పరిమితి));

_       (EmitOp(o, op_CatchPayload));
        EmitConstant32(o, అంకె_వాదనలు);

        కొరకు (u16 a = 0; a < అంకె_వాదనలు; ++a) {
            u16 index = మొదటి_సూచీ + a;
            Emit_స్థానం_స్థానభ్రంశం(o, Get_స్థానం_For_దొంతర_సూచీ(o, index));
            EmitConstant32(o, Is64_బిట్టు_Type(Get_దొంతర_Type_From_Bottom(o, index)));
        }

        Emit_స్థానం_స్థానభ్రంశం(o, clause->hasRef ? (i32)Get_స్థానం_For_దొంతర_సూచీ(o, మొదటి_సూచీ + అంకె_వాదనలు) : -1);

_       (Emit_తీయి_Try_చట్రాలు(o, అంకె_Try_చట్రాలు_గమ్యం_తీయి(o->ఖండం.outer, scope)));

        ఒకవేళ (scope->opcode == c_waOp_loop) {
_           (Resolve_ఖండం_Results(o, scope, /* isBranch: */ true));

_           (EmitOp(o, Op_ContinueLoop(o)));
            Emit_సూచిక(o, scope->pc);
_           (రికార్డు_వెనుక_అంచు(o, scope, false));
        } లేకపోతే ఒకవేళ (scope->లోతు == 0) {
_           (Return_విలువలు(o, scope, /* isBranch: */ true));
_           (EmitOp(o, op_Return));
        } లేకపోతే {
_           (Resolve_ఖండం_Results(o, scope, /* isBranch: */ true));
_           (EmitPatchingBranch(o, scope));
        }

        IM3_సంకేతం_పుట return_గమ్యం = displaced_పుట;
        displaced_పుట        = ఖాళీ;

        Release_Compilation_సంకేతం_పుట(o);
        o->పుట = return_గమ్యం;

        *(pc_t*)clause->stub_స్థానం = ప్రారంభం_PC;

        అయితే (o->దొంతర_సూచీ > మొదటి_సూచీ) {
_           (తీయి(o));
        }
    }

} _catch:

    // thrown with a stub page installed: release it and put back the one it
    // displaced, which the caller's catch then releases
    ఒకవేళ (displaced_పుట) {
        Release_Compilation_సంకేతం_పుట(o);
        o->పుట = displaced_పుట;
    }

    తిరిగి ఫలితం;
}


స్థిర
M3_ఫలితం Compile_Try_పట్టిక (IM3Compilation o, m3opcode_t i_opcode)
{
    M3CatchClause* clauses = ఖాళీ;
_try {
    // a catch entry has to be able to assume the operand stack is entirely in
    // slots: an unwind leaves nothing in the registers
_   (PreserveRegisters(o));
_   (Preserve_వాదనలు_And_Locals(o));

    IM3FuncType ఖండం_Type = ఖాళీ;
_   (Read_ఖండం_Type(o, &ఖండం_Type));

    u32 అంకె_Clauses;
_   (ReadLEB_u32(&అంకె_Clauses, &o->wasm, o->wasm_చివర));

    _throwif("too many catch clauses", అంకె_Clauses > d_m3_Max_Sane_ట్యాగులు_లెక్క);

    ఒకవేళ (అంకె_Clauses) {
        clauses = m3_AllocArray(M3CatchClause, అంకె_Clauses);
        _throwifnull(clauses);
    }

    కొరకు (u32 i = 0; i < అంకె_Clauses; ++i) {
        u8 రకం;
_       (Read_u8(&రకం, &o->wasm, o->wasm_చివర));
        _throwif(m3Err_wasmMalformed, రకం > 0x03);

        clauses[i].ట్యాగ్    = ఖాళీ;
        clauses[i].hasRef = (రకం == 0x01 or రకం == 0x03);

        ఒకవేళ (రకం == 0x00 or రకం == 0x01) {
            u32 ట్యాగ్_సూచీ;
_           (ReadLEB_u32(&ట్యాగ్_సూచీ, &o->wasm, o->wasm_చివర));
            _throwif(m3_Err_unknown_ట్యాగ్, ట్యాగ్_సూచీ >= o->module->అంకె_ట్యాగులు);

            IM3_ట్యాగ్ ట్యాగ్     = &o->module->ట్యాగులు[ట్యాగ్_సూచీ];
            clauses[i].ట్యాగ్ = ట్యాగ్->resolved ? ట్యాగ్->resolved : ట్యాగ్;
        }

_       (ReadLEB_u32(&clauses[i].label_లోతు, &o->wasm, o->wasm_చివర));
    }

    // op_TryTable, the clause count, and two words per clause, all of which have
    // to land on one page: the op reads the table by offset from its own pc
_   (Ensure_సంకేతం_పుట_అంకె_పంక్తులు(o, 2 * అంకె_Clauses + 2 + d_m3_సంకేతం_పుట_Free_పంక్తులు_పరిమితి));

_   (EmitOp(o, op_Try_పట్టిక));
    EmitConstant32(o, అంకె_Clauses);

    కొరకు (u32 i = 0; i < అంకె_Clauses; ++i) {
        Emit_సూచిక(o, clauses[i].ట్యాగ్);
        clauses[i].stub_స్థానం = Reserve_సూచిక(o);
    }

    o->tryClauses    = clauses;
    o->అంకె_Try_Clauses = అంకె_Clauses;

_   (Compile_ఖండం(o, ఖండం_Type, c_wa_Op_try_పట్టిక));

} _catch:

    o->tryClauses    = ఖాళీ;
    o->అంకె_Try_Clauses = 0;

    m3_Free(clauses);

    తిరిగి ఫలితం;
}


స్థిర
M3_ఫలితం Compile_Throw (IM3Compilation o, m3opcode_t i_opcode)
{
_try {
    u32 ట్యాగ్_సూచీ;
_   (ReadLEB_u32(&ట్యాగ్_సూచీ, &o->wasm, o->wasm_చివర));
    _throwif(m3_Err_unknown_ట్యాగ్, ట్యాగ్_సూచీ >= o->module->అంకె_ట్యాగులు);

    IM3_ట్యాగ్ ట్యాగ్ = &o->module->ట్యాగులు[ట్యాగ్_సూచీ];
    ఒకవేళ (ట్యాగ్->resolved) {
        ట్యాగ్ = ట్యాగ్->resolved;
    }

    u16 అంకె_వాదనలు = Get_Func_Type_అంకె_Params(ట్యాగ్->type);

    // the payload is read out of slots, so nothing may still be in a register
_   (PreserveRegisters(o));

    // check the payload before emitting anything, so a mistyped throw does not
    // leave half an operation behind
    bool isPolymorphic = Is_దొంతర_Polymorphic(o);

    ఒకవేళ (not isPolymorphic) {
        _throwif(m3_Err_type_లెక్క_Mismatch, Get_అంకె_ఖండం_విలువలు_On_దొంతర(o) < అంకె_వాదనలు);

        కొరకు (u16 i = 0; i < అంకె_వాదనలు; ++i) {
            u16 index = (u16)(o->దొంతర_సూచీ - అంకె_వాదనలు + i);

            _throwif(m3Err_typeMismatch, not Is_Sub_Type_యొక్క(Get_దొంతర_Type_From_Bottom(o, index),
                                                         GetFuncTypeParamType(ట్యాగ్->type, i)));
        }
    }

_   (Ensure_సంకేతం_పుట_అంకె_పంక్తులు(o, 2 * అంకె_వాదనలు + 3 + d_m3_సంకేతం_పుట_Free_పంక్తులు_పరిమితి));

_   (EmitOp(o, op_Throw));
    Emit_సూచిక(o, ట్యాగ్);
    EmitConstant32(o, అంకె_వాదనలు);

    // the payload is named in the order the tag declares it, so the deepest of
    // the arguments on the stack comes first. In unreachable code there is
    // nothing on the stack to name and nothing will run this, so slot 0 stands
    // in for an operand that isn't there.
    కొరకు (u16 i = 0; i < అంకె_వాదనలు; ++i) {
        m3type_t type = GetFuncTypeParamType(ట్యాగ్->type, i);

        Emit_స్థానం_స్థానభ్రంశం(o, isPolymorphic ? 0 : Get_స్థానం_For_దొంతర_సూచీ(o, (u16)(o->దొంతర_సూచీ - అంకె_వాదనలు + i)));
        EmitConstant32(o, Is64_బిట్టు_Type(type));
    }

    ఒకవేళ (not isPolymorphic) {
        కొరకు (u16 i = 0; i < అంకె_వాదనలు; ++i) {
_           (తీయి(o));
        }
    }

_   (సమితి_దొంతర_Polymorphic(o));

} _catch: తిరిగి ఫలితం;
}


స్థిర
M3_ఫలితం Compile_ThrowRef (IM3Compilation o, m3opcode_t i_opcode)
{
_try {
    ఒకవేళ (not Is_దొంతర_Polymorphic(o)) {
_       (Preserve_రిజిస్టరు_If_Occupied(o, c_m3Type_i64));

        _throwif(m3Err_typeMismatch, పునాది_Type_యొక్క(Get_దొంతర_శిఖరం_Type(o)) != c_m3Type_exnref);

_       (EmitOp(o, op_ThrowRef));
        Emit_స్థానం_స్థానభ్రంశం(o, Get_దొంతర_శిఖరం_స్థానం_సంఖ్య(o));

_       (తీయి(o));
    }

_   (సమితి_దొంతర_Polymorphic(o));

} _catch: తిరిగి ఫలితం;
}

#అయితే_ముగింపు // d_m3HasExceptionHandling

#ఒకవేళ d_m3_Has_దొంతర_Switching

స్థిర
M3_ఫలితం Compile_ContNew (IM3Compilation o, m3opcode_t i_opcode)
{
_try {
    u32 type_సూచీ;
_   (ReadLEB_u32(&type_సూచీ, &o->wasm, o->wasm_చివర));
    _throwif(m3_Err_type_లెక్క_Mismatch, type_సూచీ >= o->module->అంకె_Func_Types);

    IM3FuncType funcType = o->module->funcTypes[type_సూచీ];
    _throwif("type is not a continuation", not funcType->isContinuation);

    // the operands are read out of slots, so nothing may still be in a register
_   (PreserveRegisters(o));

    // cont.new $ct : [(ref null $ft)] -> [(ref $ct)], where $ct ~~ cont $ft:
    // the function has to be one this continuation type can run
    m3type_t శిఖరం_Type = Get_దొంతర_శిఖరం_Type(o);
    _throwif(m3Err_typeMismatch,
             not Is_Sub_Type_యొక్క(శిఖరం_Type, Ref_Type_యొక్క_Func_Type(funcType->contFuncType, false)));

    u16 func_స్థానం = Get_దొంతర_శిఖరం_స్థానం_సంఖ్య(o);
_   (తీయి(o));

    u16 dst_స్థానం;
_   (నెట్టు_Allocated_స్థానం(o, Ref_Type_యొక్క_Func_Type(funcType, true)));
    dst_స్థానం = Get_దొంతర_శిఖరం_స్థానం_సంఖ్య(o);

_   (Ensure_సంకేతం_పుట_అంకె_పంక్తులు(o, 4 + d_m3_సంకేతం_పుట_Free_పంక్తులు_పరిమితి));
_   (EmitOp(o, op_ContNew));
    Emit_సూచిక(o, funcType);
    Emit_స్థానం_స్థానభ్రంశం(o, dst_స్థానం);
    Emit_స్థానం_స్థానభ్రంశం(o, func_స్థానం);

} _catch: తిరిగి ఫలితం;
}

స్థిర
M3_ఫలితం Compile_ContBind (IM3Compilation o, m3opcode_t i_opcode)
{
_try {
    u32 typeIndex1, typeIndex2;
_   (ReadLEB_u32(&typeIndex1, &o->wasm, o->wasm_చివర));
_   (ReadLEB_u32(&typeIndex2, &o->wasm, o->wasm_చివర));
    _throwif(m3_Err_type_లెక్క_Mismatch, typeIndex1 >= o->module->అంకె_Func_Types);
    _throwif(m3_Err_type_లెక్క_Mismatch, typeIndex2 >= o->module->అంకె_Func_Types);

    IM3FuncType ct1 = o->module->funcTypes[typeIndex1];
    IM3FuncType ct2 = o->module->funcTypes[typeIndex2];
    _throwif("type is not a continuation", not ct1->isContinuation || not ct2->isContinuation);

    // the operands are read out of slots, so nothing may still be in a register
_   (PreserveRegisters(o));

    m3type_t contType = Get_దొంతర_శిఖరం_Type(o);
    _throwif(m3Err_typeMismatch, not Is_Sub_Type_యొక్క(contType, Ref_Type_యొక్క_Func_Type(ct1, false)));
    u16 cont_స్థానం = Get_దొంతర_శిఖరం_స్థానం_సంఖ్య(o);
_   (తీయి(o));

    IM3FuncType ft1 = ct1->contFuncType ? ct1->contFuncType : ct1;
    IM3FuncType ft2 = ct2->contFuncType ? ct2->contFuncType : ct2;

    u16 అంకె_Params1 = Get_Func_Type_అంకె_Params(ft1);
    u16 అంకె_Params2 = Get_Func_Type_అంకె_Params(ft2);
    _throwif(m3Err_typeMismatch, అంకె_Params1 < అంకె_Params2);
    u32 అంకె_Bound = అంకె_Params1 - అంకె_Params2;

    u16 అంకె_Results1 = Get_Func_Type_అంకె_Results(ft1);
    u16 అంకె_Results2 = Get_Func_Type_అంకె_Results(ft2);
    _throwif(m3Err_typeMismatch, అంకె_Results1 != అంకె_Results2);
    కొరకు (u16 i = 0; i < అంకె_Results1; ++i) {
        _throwif(m3Err_typeMismatch, Get_Func_Type_ఫలితం_Type(ft1, i) != Get_Func_Type_ఫలితం_Type(ft2, i));
    }
    కొరకు (u16 i = 0; i < అంకె_Params2; ++i) {
        _throwif(m3Err_typeMismatch, GetFuncTypeParamType(ft1, అంకె_Bound + i) != GetFuncTypeParamType(ft2, i));
    }

    u16 argSlots[d_m3MaxContinuationPayload];
    _throwif("too many bound args", అంకె_Bound > d_m3MaxContinuationPayload);

    కొరకు (u32 i = 0; i < అంకె_Bound; ++i) {
        u32      paramIdx     = అంకె_Bound - 1 - i;
        m3type_t expectedType = GetFuncTypeParamType(ft1, paramIdx);
        m3type_t actualType   = Get_దొంతర_శిఖరం_Type(o);
        _throwif(m3Err_typeMismatch, not Is_Sub_Type_యొక్క(actualType, expectedType));
        argSlots[paramIdx] = Get_దొంతర_శిఖరం_స్థానం_సంఖ్య(o);
_       (తీయి(o));
    }

    u16 dst_స్థానం;
_   (నెట్టు_Allocated_స్థానం(o, Ref_Type_యొక్క_Func_Type(ct2, true)));
    dst_స్థానం = Get_దొంతర_శిఖరం_స్థానం_సంఖ్య(o);

_   (Ensure_సంకేతం_పుట_అంకె_పంక్తులు(o, 6 + అంకె_Bound * 2 + d_m3_సంకేతం_పుట_Free_పంక్తులు_పరిమితి));
_   (EmitOp(o, op_ContBind));
    Emit_సూచిక(o, ct1);
    Emit_సూచిక(o, ct2);
    Emit_స్థానం_స్థానభ్రంశం(o, dst_స్థానం);
    Emit_స్థానం_స్థానభ్రంశం(o, cont_స్థానం);
    EmitConstant32(o, అంకె_Bound);
    కొరకు (u32 i = 0; i < అంకె_Bound; ++i) {
        Emit_స్థానం_స్థానభ్రంశం(o, argSlots[i]);
        EmitConstant32(o, Is64_బిట్టు_Type(GetFuncTypeParamType(ft1, i)));
    }

} _catch: తిరిగి ఫలితం;
}

స్థిర
M3_ఫలితం Compile_Suspend (IM3Compilation o, m3opcode_t i_opcode)
{
_try {
    u32 ట్యాగ్_సూచీ;
_   (ReadLEB_u32(&ట్యాగ్_సూచీ, &o->wasm, o->wasm_చివర));
    _throwif(m3_Err_unknown_ట్యాగ్, ట్యాగ్_సూచీ >= o->module->అంకె_ట్యాగులు);

    IM3_ట్యాగ్ ట్యాగ్ = &o->module->ట్యాగులు[ట్యాగ్_సూచీ];
    ఒకవేళ (ట్యాగ్->resolved) {
        ట్యాగ్ = ట్యాగ్->resolved;
    }
    u16 అంకె_Params  = Get_Func_Type_అంకె_Params(ట్యాగ్->type);
    u16 అంకె_Results = Get_Func_Type_అంకె_Results(ట్యాగ్->type);

_   (PreserveRegisters(o));

    u16 paramSlots[d_m3MaxContinuationPayload];
    _throwif("too many tag params", అంకె_Params > d_m3MaxContinuationPayload);
    కొరకు (u16 i = 0; i < అంకె_Params; ++i) {
        u16      pIdx         = అంకె_Params - 1 - i;
        m3type_t expectedType = GetFuncTypeParamType(ట్యాగ్->type, pIdx);
        m3type_t actualType   = Get_దొంతర_శిఖరం_Type(o);
        _throwif(m3Err_typeMismatch, not Is_Sub_Type_యొక్క(actualType, expectedType));
        paramSlots[pIdx] = Get_దొంతర_శిఖరం_స్థానం_సంఖ్య(o);
_       (తీయి(o));
    }

    u16 ఫలితం_Slots[d_m3MaxContinuationPayload];
    _throwif("too many tag results", అంకె_Results > d_m3MaxContinuationPayload);
    కొరకు (u16 i = 0; i < అంకె_Results; ++i) {
        m3type_t rtype = Get_Func_Type_ఫలితం_Type(ట్యాగ్->type, i);
_       (నెట్టు_Allocated_స్థానం(o, rtype));
        ఫలితం_Slots[i] = Get_దొంతర_శిఖరం_స్థానం_సంఖ్య(o);
    }

_   (Ensure_సంకేతం_పుట_అంకె_పంక్తులు(o, 4 + అంకె_Params * 2 + అంకె_Results * 2 + d_m3_సంకేతం_పుట_Free_పంక్తులు_పరిమితి));
_   (EmitOp(o, op_Suspend));
    Emit_సూచిక(o, ట్యాగ్);
    EmitConstant32(o, అంకె_Params);
    కొరకు (u16 i = 0; i < అంకె_Params; ++i) {
        Emit_స్థానం_స్థానభ్రంశం(o, paramSlots[i]);
        EmitConstant32(o, Is64_బిట్టు_Type(GetFuncTypeParamType(ట్యాగ్->type, i)));
    }
    EmitConstant32(o, అంకె_Results);
    కొరకు (u16 i = 0; i < అంకె_Results; ++i) {
        Emit_స్థానం_స్థానభ్రంశం(o, ఫలితం_Slots[i]);
        EmitConstant32(o, Is64_బిట్టు_Type(Get_Func_Type_ఫలితం_Type(ట్యాగ్->type, i)));
    }

    // the results are part of the frame here: op_Suspend clears them, and
    // whatever resumes this writes them before anything reads them
_   (రికార్డు_సురక్షిత_బిందువు(o, safepoint_suspend, 0, అంకె_Results, 0, ఖాళీ));

} _catch: తిరిగి ఫలితం;
}

// resume, resume_throw and resume_throw_ref. All three run a continuation
// under a handler table and take its results; they differ only in what they
// hand it to start with - arguments, a fresh exception, or one off the stack.
//
// Emitted as: the continuation type, the slot holding it, the operands that
// belong to this form, the handler table, then the result slots. Keeping the
// tail identical is what lets one runtime path finish all three.
స్థిర
M3_ఫలితం Compile_Resume (IM3Compilation o, m3opcode_t i_opcode)
{
    M3ResumeHandler* handlers      = ఖాళీ;
    IM3_సంకేతం_పుట      displaced_పుట = ఖాళీ;
_try {
    bool isThrow    = (i_opcode == c_waOp_resumeThrow);
    bool isThrowRef = (i_opcode == c_waOp_resumeThrowRef);

    u32 type_సూచీ;
_   (ReadLEB_u32(&type_సూచీ, &o->wasm, o->wasm_చివర));
    _throwif(m3_Err_type_లెక్క_Mismatch, type_సూచీ >= o->module->అంకె_Func_Types);

    IM3FuncType contFuncType = o->module->funcTypes[type_సూచీ];
    _throwif("type is not a continuation", not contFuncType->isContinuation);

    IM3FuncType innerType      = contFuncType->contFuncType;
    u16         అంకె_Cont_Results = Get_Func_Type_అంకె_Results(innerType);

    IM3_ట్యాగ్ exn_ట్యాగ్      = ఖాళీ;
    u16    అంకె_Operands = 0;

    ఒకవేళ (isThrow) {
        u32 ట్యాగ్_సూచీ;
_       (ReadLEB_u32(&ట్యాగ్_సూచీ, &o->wasm, o->wasm_చివర));
        _throwif(m3_Err_unknown_ట్యాగ్, ట్యాగ్_సూచీ >= o->module->అంకె_ట్యాగులు);

        exn_ట్యాగ్ = &o->module->ట్యాగులు[ట్యాగ్_సూచీ];

        // it is an exception that gets raised, so the tag must be one throw
        // could have raised: no results
        _throwif(m3Err_typeMismatch, Get_Func_Type_అంకె_Results(exn_ట్యాగ్->type) != 0);

        అంకె_Operands = Get_Func_Type_అంకె_Params(exn_ట్యాగ్->type);
    } లేకపోతే ఒకవేళ (not isThrowRef) {
        // a plain resume hands over the continuation's own parameters. An
        // aborted one never receives them, so they are unconstrained there.
        అంకె_Operands = Get_Func_Type_అంకె_Params(innerType);
    }

    u32 అంకె_Clauses;
_   (ReadLEB_u32(&అంకె_Clauses, &o->wasm, o->wasm_చివర));

    u32 clauseLabelDepths[16];
    _throwif("too many resume clauses", అంకె_Clauses > 16);

    ఒకవేళ (అంకె_Clauses) {
        handlers = m3_AllocArray(M3ResumeHandler, అంకె_Clauses);
        _throwifnull(handlers);
    }

    కొరకు (u32 i = 0; i < అంకె_Clauses; ++i) {
        u8 రకం;
_       (Read_u8(&రకం, &o->wasm, o->wasm_చివర));
        _throwif(m3Err_wasmMalformed, రకం > 0x01);

        handlers[i].రకం = రకం;

        u32 ట్యాగ్_సూచీ;
_       (ReadLEB_u32(&ట్యాగ్_సూచీ, &o->wasm, o->wasm_చివర));
        _throwif(m3_Err_unknown_ట్యాగ్, ట్యాగ్_సూచీ >= o->module->అంకె_ట్యాగులు);

        IM3_ట్యాగ్ ట్యాగ్         = &o->module->ట్యాగులు[ట్యాగ్_సూచీ];
        handlers[i].ట్యాగ్    = ట్యాగ్->resolved ? ట్యాగ్->resolved : ట్యాగ్;
        handlers[i].stubPC = ఖాళీ;

        ఒకవేళ (రకం == 0x00) {
_           (ReadLEB_u32(&clauseLabelDepths[i], &o->wasm, o->wasm_చివర));
        } లేకపోతే {
            // (on $e switch): the tag carries no payload, and what the switch
            // site yields is what this resume yields
            _throwif(m3Err_typeMismatch, Get_Func_Type_అంకె_Params(handlers[i].ట్యాగ్->type) != 0);
            clauseLabelDepths[i] = 0;
        }
    }

    // the operands are read out of slots, so nothing may still be in a register
_   (PreserveRegisters(o));

    m3type_t cont_శిఖరం_Type = Get_దొంతర_శిఖరం_Type(o);
    _throwif(m3Err_typeMismatch, not Is_Sub_Type_యొక్క(cont_శిఖరం_Type, Ref_Type_యొక్క_Func_Type(contFuncType, false)));
    u16 cont_స్థానం = Get_దొంతర_శిఖరం_స్థానం_సంఖ్య(o);
_   (తీయి(o));

    u16 exn_స్థానం = 0;

    ఒకవేళ (isThrowRef) {
        m3type_t exnType = Get_దొంతర_శిఖరం_Type(o);
        _throwif(m3Err_typeMismatch, పునాది_Type_యొక్క(exnType) != c_m3Type_exnref);
        exn_స్థానం = Get_దొంతర_శిఖరం_స్థానం_సంఖ్య(o);
_       (తీయి(o));
    }

    u16 నిర్వాహ్యం_Slots[d_m3MaxContinuationPayload];
    _throwif("too many resume operands", అంకె_Operands > d_m3MaxContinuationPayload);

    IM3FuncType నిర్వాహ్యం_Type = isThrow ? exn_ట్యాగ్->type : innerType;

    కొరకు (u16 i = 0; i < అంకె_Operands; ++i) {
        u16      pIdx         = (u16)(అంకె_Operands - 1 - i);
        m3type_t expectedType = GetFuncTypeParamType(నిర్వాహ్యం_Type, pIdx);
        m3type_t actualType   = Get_దొంతర_శిఖరం_Type(o);
        _throwif(m3Err_typeMismatch, not Is_Sub_Type_యొక్క(actualType, expectedType));
        నిర్వాహ్యం_Slots[pIdx] = Get_దొంతర_శిఖరం_స్థానం_సంఖ్య(o);
_       (తీయి(o));
    }

    కొరకు (u32 i = 0; i < అంకె_Clauses; ++i) {
        ఒకవేళ (handlers[i].రకం != 0x00) {
            కొనసాగు;
        }

        IM3CompilationScope scope;
_       (Get_ఖండం_Scope(o, &scope, clauseLabelDepths[i]));

        IM3_ట్యాగ్ ట్యాగ్        = handlers[i].ట్యాగ్;
        u16    అంకె_Payload = Get_Func_Type_అంకె_Params(ట్యాగ్->type);

        m3type_t labelContType = (scope->opcode == c_waOp_loop)
                                   ? GetFuncTypeParamType(scope->type, అంకె_Payload)
                                   : Get_Func_Type_ఫలితం_Type(scope->type, అంకె_Payload);

        IM3_సంకేతం_పుట stub_పుట;
_       (Acquire_Compilation_సంకేతం_పుట(o, &stub_పుట));
        pc_t ప్రారంభం_PC  = Get_పుట_PC(stub_పుట);
        displaced_పుట = o->పుట;
        o->పుట       = stub_పుట;

        u16 మొదటి_సూచీ = o->దొంతర_సూచీ;
        కొరకు (u16 a = 0; a < అంకె_Payload; ++a) {
_           (నెట్టు_Allocated_స్థానం(o, GetFuncTypeParamType(ట్యాగ్->type, a)));
        }

        // The handler is handed the suspended continuation.
_       (నెట్టు_Allocated_స్థానం(o, labelContType));

_       (Ensure_సంకేతం_పుట_అంకె_పంక్తులు(o, 2 * అంకె_Payload + 5 + d_m3_సంకేతం_పుట_Free_పంక్తులు_పరిమితి));
_       (EmitOp(o, op_ResumePayload));
        Emit_సూచిక(o, Module_Cont_Type_యొక్క_Ref(o->module, labelContType));
        EmitConstant32(o, అంకె_Payload);
        కొరకు (u16 a = 0; a < అంకె_Payload; ++a) {
            u16 index = మొదటి_సూచీ + a;
            Emit_స్థానం_స్థానభ్రంశం(o, Get_స్థానం_For_దొంతర_సూచీ(o, index));
            EmitConstant32(o, Is64_బిట్టు_Type(Get_దొంతర_Type_From_Bottom(o, index)));
        }
        Emit_స్థానం_స్థానభ్రంశం(o, Get_స్థానం_For_దొంతర_సూచీ(o, మొదటి_సూచీ + అంకె_Payload));

#  ఒకవేళ d_m3HasExceptionHandling
        // the clause leaves every try region between the resume and its label,
        // and leaving one retires its handlers, the way a branch does
_       (Emit_తీయి_Try_చట్రాలు(o, అంకె_Try_చట్రాలు_గమ్యం_తీయి(&o->ఖండం, scope)));
#  endif

        ఒకవేళ (scope->opcode == c_waOp_loop) {
_           (Resolve_ఖండం_Results(o, scope, true));
_           (EmitOp(o, Op_ContinueLoop(o)));
            Emit_సూచిక(o, scope->pc);
_           (రికార్డు_వెనుక_అంచు(o, scope, false));
        } లేకపోతే ఒకవేళ (scope->లోతు == 0) {
_           (Return_విలువలు(o, scope, true));
_           (EmitOp(o, op_Return));
        } లేకపోతే {
_           (Resolve_ఖండం_Results(o, scope, true));
_           (EmitPatchingBranch(o, scope));
        }

        IM3_సంకేతం_పుట return_గమ్యం = displaced_పుట;
        displaced_పుట        = ఖాళీ;

        Release_Compilation_సంకేతం_పుట(o);
        o->పుట = return_గమ్యం;

        handlers[i].stubPC = ప్రారంభం_PC;

        అయితే (o->దొంతర_సూచీ > మొదటి_సూచీ) {
_           (తీయి(o));
        }
    }

    u16 resSlots[d_m3MaxContinuationPayload];
    _throwif("too many cont results", అంకె_Cont_Results > d_m3MaxContinuationPayload);
    కొరకు (u16 i = 0; i < అంకె_Cont_Results; ++i) {
        m3type_t rtype = Get_Func_Type_ఫలితం_Type(innerType, i);
_       (నెట్టు_Allocated_స్థానం(o, rtype));
        resSlots[i] = Get_దొంతర_శిఖరం_స్థానం_సంఖ్య(o);
    }

_   (Ensure_సంకేతం_పుట_అంకె_పంక్తులు(o, 8 + అంకె_Operands * 2 + అంకె_Clauses * 3 + అంకె_Cont_Results * 2 +
                                  d_m3_సంకేతం_పుట_Free_పంక్తులు_పరిమితి));

_   (EmitOp(o, op_Resume));
    Emit_సూచిక(o, contFuncType);
    Emit_స్థానం_స్థానభ్రంశం(o, cont_స్థానం);
    EmitConstant32(o, isThrow ? d_m3ResumeThrow : (isThrowRef ? d_m3ResumeThrowRef : d_m3_Resume_సాధారణ));

    ఒకవేళ (isThrowRef) {
        Emit_స్థానం_స్థానభ్రంశం(o, exn_స్థానం);
    } లేకపోతే {
        ఒకవేళ (isThrow) {
            Emit_సూచిక(o, exn_ట్యాగ్);
        }

        EmitConstant32(o, అంకె_Operands);
        కొరకు (u16 i = 0; i < అంకె_Operands; ++i) {
            Emit_స్థానం_స్థానభ్రంశం(o, నిర్వాహ్యం_Slots[i]);
            EmitConstant32(o, Is64_బిట్టు_Type(GetFuncTypeParamType(నిర్వాహ్యం_Type, i)));
        }
    }

    EmitConstant32(o, అంకె_Clauses);
    కొరకు (u32 i = 0; i < అంకె_Clauses; ++i) {
        EmitConstant32(o, handlers[i].రకం);
        Emit_సూచిక(o, handlers[i].ట్యాగ్);
        Emit_సూచిక(o, handlers[i].stubPC);
    }

    EmitConstant32(o, అంకె_Cont_Results);
    కొరకు (u16 i = 0; i < అంకె_Cont_Results; ++i) {
        Emit_స్థానం_స్థానభ్రంశం(o, resSlots[i]);
        EmitConstant32(o, Is64_బిట్టు_Type(Get_Func_Type_ఫలితం_Type(innerType, i)));
    }

    // a suspend passing through the resumed continuation leaves this frame
    // waiting here, with the continuation's results not yet taken
_   (రికార్డు_సురక్షిత_బిందువు(o, safepoint_resume, 0, అంకె_Cont_Results, (u16)అంకె_Clauses, ఖాళీ));
#  ఒకవేళ d_m3HasSnapshots
    ఒకవేళ (o->snapshotMap and o->పుట) {
        o->snapshotMap->సురక్షిత_Points[o->snapshotMap->అంకె_సురక్షిత_Points - 1].resumeType = Ref_Type_యొక్క_Func_Type(contFuncType, true);
    }
#  endif

} _catch:
    ఒకవేళ (displaced_పుట) {
        Release_Compilation_సంకేతం_పుట(o);
        o->పుట = displaced_పుట;
    }
    m3_Free(handlers);
    తిరిగి ఫలితం;
}


స్థిర
M3_ఫలితం Compile_Switch (IM3Compilation o, m3opcode_t i_opcode)
{
_try {
    u32 type_సూచీ, ట్యాగ్_సూచీ;
_   (ReadLEB_u32(&type_సూచీ, &o->wasm, o->wasm_చివర));
_   (ReadLEB_u32(&ట్యాగ్_సూచీ, &o->wasm, o->wasm_చివర));
    _throwif(m3_Err_type_లెక్క_Mismatch, type_సూచీ >= o->module->అంకె_Func_Types);
    _throwif(m3_Err_unknown_ట్యాగ్, ట్యాగ్_సూచీ >= o->module->అంకె_ట్యాగులు);

    IM3FuncType లక్ష్యం_Cont_Type = o->module->funcTypes[type_సూచీ];
    IM3_ట్యాగ్      ట్యాగ్            = &o->module->ట్యాగులు[ట్యాగ్_సూచీ];
    ఒకవేళ (ట్యాగ్->resolved) {
        ట్యాగ్ = ట్యాగ్->resolved;
    }

    _throwif("type is not a continuation", not లక్ష్యం_Cont_Type->isContinuation);

    // a switch tag carries no payload of its own: the values travel as the
    // peer's arguments, and the tag's results are what the switch site yields
    _throwif(m3Err_typeMismatch, Get_Func_Type_అంకె_Params(ట్యాగ్->type) != 0);

    // switch $ct1 : [t1* (ref null $ct1)] -> [t2*]
    //   where $ct1 = cont [t1* (ref null? $ct2)] -> [..]
    //   and   $ct2 = cont [t2*] -> [..]
    //
    // The trailing parameter of $ct1 is not an operand: the peer receives the
    // continuation this instruction suspends. And what this instruction yields
    // is not $ct1's results but $ct2's parameters - the values whoever comes
    // back this way will hand over.
    IM3FuncType లక్ష్యం_Type = లక్ష్యం_Cont_Type->contFuncType;
    u16         అంకె_Params  = Get_Func_Type_అంకె_Params(లక్ష్యం_Type);

    _throwif("switch target takes no continuation", అంకె_Params == 0);

    m3type_t    peerRefType  = GetFuncTypeParamType(లక్ష్యం_Type, (u16)(అంకె_Params - 1));
    IM3FuncType selfContType = Module_Cont_Type_యొక్క_Ref(o->module, peerRefType);

    _throwif(m3Err_typeMismatch, not selfContType);

    IM3FuncType selfType   = selfContType->contFuncType;
    u16         అంకె_వాదనలు    = (u16)(అంకె_Params - 1);
    u16         అంకె_Results = Get_Func_Type_అంకె_Params(selfType);

    // the operands are read out of slots, so nothing may still be in a register
_   (PreserveRegisters(o));

    m3type_t cont_శిఖరం_Type = Get_దొంతర_శిఖరం_Type(o);
    _throwif(m3Err_typeMismatch, not Is_Sub_Type_యొక్క(cont_శిఖరం_Type, Ref_Type_యొక్క_Func_Type(లక్ష్యం_Cont_Type, false)));
    u16 లక్ష్యం_Cont_స్థానం = Get_దొంతర_శిఖరం_స్థానం_సంఖ్య(o);
_   (తీయి(o));

    u16 argSlots[d_m3MaxContinuationPayload];
    _throwif("too many switch params", అంకె_వాదనలు > d_m3MaxContinuationPayload);

    కొరకు (u16 i = 0; i < అంకె_వాదనలు; ++i) {
        u16      pIdx         = (u16)(అంకె_వాదనలు - 1 - i);
        m3type_t expectedType = GetFuncTypeParamType(లక్ష్యం_Type, pIdx);
        m3type_t actualType   = Get_దొంతర_శిఖరం_Type(o);
        _throwif(m3Err_typeMismatch, not Is_Sub_Type_యొక్క(actualType, expectedType));
        argSlots[pIdx] = Get_దొంతర_శిఖరం_స్థానం_సంఖ్య(o);
_       (తీయి(o));
    }

    u16 ఫలితం_Slots[d_m3MaxContinuationPayload];
    _throwif("too many switch results", అంకె_Results > d_m3MaxContinuationPayload);

    కొరకు (u16 i = 0; i < అంకె_Results; ++i) {
_       (నెట్టు_Allocated_స్థానం(o, GetFuncTypeParamType(selfType, i)));
        ఫలితం_Slots[i] = Get_దొంతర_శిఖరం_స్థానం_సంఖ్య(o);
    }

_   (Ensure_సంకేతం_పుట_అంకె_పంక్తులు(o, 6 + అంకె_వాదనలు * 2 + అంకె_Results * 2 + d_m3_సంకేతం_పుట_Free_పంక్తులు_పరిమితి));
_   (EmitOp(o, op_Switch));
    Emit_సూచిక(o, లక్ష్యం_Cont_Type);
    Emit_సూచిక(o, ట్యాగ్);
    Emit_స్థానం_స్థానభ్రంశం(o, లక్ష్యం_Cont_స్థానం);
    EmitConstant32(o, అంకె_వాదనలు);
    కొరకు (u16 i = 0; i < అంకె_వాదనలు; ++i) {
        Emit_స్థానం_స్థానభ్రంశం(o, argSlots[i]);
        EmitConstant32(o, Is64_బిట్టు_Type(GetFuncTypeParamType(లక్ష్యం_Type, i)));
    }
    EmitConstant32(o, అంకె_Results);
    కొరకు (u16 i = 0; i < అంకె_Results; ++i) {
        Emit_స్థానం_స్థానభ్రంశం(o, ఫలితం_Slots[i]);
        EmitConstant32(o, Is64_బిట్టు_Type(GetFuncTypeParamType(selfType, i)));
    }

    // as with suspend: op_Switch clears the results before it leaves
_   (రికార్డు_సురక్షిత_బిందువు(o, safepoint_suspend, 0, అంకె_Results, 0, ఖాళీ));

} _catch: తిరిగి ఫలితం;
}

#అయితే_ముగింపు // d_m3HasStackSwitching


స్థిర
M3_ఫలితం Compile_Else_ఖండం (IM3Compilation o, pc_t* o_ప్రారంభం_PC, IM3FuncType i_ఖండం_Type)
{
    IM3_సంకేతం_పుట saved_పుట = o->పుట;
_try {

    IM3_సంకేతం_పుట else_పుట;
_   (Acquire_Compilation_సంకేతం_పుట(o, &else_పుట));

    *o_ప్రారంభం_PC = Get_పుట_PC(else_పుట);

    o->పుట = else_పుట;

_   (Compile_ఖండం(o, i_ఖండం_Type, c_waOp_else));

_   (EmitOp(o, op_Branch));
    Emit_సూచిక(o, Get_పుట_PC(saved_పుట));
} _catch:

    ఒకవేళ (o->పుట != saved_పుట) {
        Release_Compilation_సంకేతం_పుట(o);
    }

    o->పుట = saved_పుట;

    తిరిగి ఫలితం;
}

స్థిర
M3_ఫలితం Compile_If (IM3Compilation o, m3opcode_t i_opcode)
{
    /*      [   op_If   ]
            [ <else-pc> ]   ---->   [ ..else..  ]
            [  ..if..   ]           [ ..block.. ]
            [ ..block.. ]           [ op_Branch ]
            [    end    ]  <-----   [  <end-pc> ]       */

_try {

    // Spec: if condition must be i32
    ఒకవేళ (not Is_దొంతర_Polymorphic(o)) {
        m3type_t condType = Get_దొంతర_శిఖరం_Type(o);
        _throwif(m3Err_typeMismatch, condType != c_m3Type_none and condType != c_m3Type_i32);
    }

_   (Preserve_Non_శిఖరం_Registers(o));
_   (Preserve_వాదనలు_And_Locals(o));

#ఒకవేళ d_m3FuseBranch
    IM3Operation fuseOp = GetBranchFusionOp(o, true);

    ఒకవేళ (fuseOp) {
        // absorb the if into the compare that produced the condition
        *(IM3Operation*)o->foldPatchPC = fuseOp;
        InvalidateFold(o);

_       (తీయి(o));                          // the condition is consumed inside the fused op
    } లేకపోతే
#అయితే_ముగింపు
    {
        IM3Operation op = Is_దొంతర_శిఖరం_లోపల_రిజిస్టరు(o) ? op_If_r : op_If_s;

_       (EmitOp(o, op));
_       (Emit_స్థానం_అంకె_యొక్క_దొంతర_శిఖరం_And_తీయి(o));
    }

    pc_t* pc = (pc_t*)Reserve_సూచిక(o);

    IM3FuncType ఖండం_Type = ఖాళీ;
_   (Read_ఖండం_Type(o, &ఖండం_Type));     //  dump_type_stack (o);

    u16 దొంతర_సూచీ = o->దొంతర_సూచీ;

_   (Compile_ఖండం(o, ఖండం_Type, i_opcode));

    ఒకవేళ (o->మునుపటి_Opcode == c_waOp_else) {
        o->దొంతర_సూచీ = దొంతర_సూచీ;
_       (Compile_Else_ఖండం(o, pc, ఖండం_Type));
    } లేకపోతే {
        // if block produces values and there isn't a defined else
        // case, then we need to make one up so that the pass-through
        // results end up in the right place
        ఒకవేళ (Get_Func_Type_అంకె_Results(ఖండం_Type)) {
            // rewind to the if's end to create a fake else block
            o->wasm--;
            o->దొంతర_సూచీ = దొంతర_సూచీ;     // dump_type_stack (o);

_           (Compile_Else_ఖండం(o, pc, ఖండం_Type));
        } లేకపోతే {
            *pc = GetPC(o);
        }
    }

    } _catch: తిరిగి ఫలితం;
}

స్థిర
M3_ఫలితం Compile_Select (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_ఫలితం ఫలితం = m3Err_none;

    u16 slots[3] = { c_స్థానం_వాడని, c_స్థానం_వాడని, c_స్థానం_వాడని };

    IM3Operation op = ఖాళీ;

    m3type_t type = Get_దొంతర_Type_From_శిఖరం(o, 1); // get type of selection

    ఒకవేళ (not Is_దొంతర_Polymorphic(o)) {
        // Spec: the condition operand (top) must be i32
        m3type_t condType = Get_దొంతర_Type_From_శిఖరం(o, 0);
        _throwif(m3Err_typeMismatch, condType != c_m3Type_none and condType != c_m3Type_i32);

        // Spec: the two value operands (below condition) must have matching types
        m3type_t type2 = Get_దొంతర_Type_From_శిఖరం(o, 2);
        _throwif(m3Err_typeMismatch, type != c_m3Type_none and type2 != c_m3Type_none and
                                       not (Is_Sub_Type_యొక్క(type, type2) or Is_Sub_Type_యొక్క(type2, type)));
    }

    ఒకవేళ (IsFpType(type)) {
#ఒకవేళ d_m3HasFloat
        // not consuming a fp reg, so preserve
        ఒకవేళ (not Is_దొంతర_శిఖరం_Minus1_లోపల_రిజిస్టరు(o) and
            not Is_దొంతర_శిఖరం_Minus2_లోపల_రిజిస్టరు(o)) {
_           (Preserve_రిజిస్టరు_If_Occupied(o, type));
        }

        bool selector_లోపల_Reg = Is_దొంతర_శిఖరం_లోపల_రిజిస్టరు(o);
        slots[0]           = Get_దొంతర_శిఖరం_స్థానం_సంఖ్య(o);
_       (తీయి(o));

        u32 op_సూచీ = 0;

        కొరకు (u32 i = 1; i <= 2; ++i) {
            ఒకవేళ (Is_దొంతర_శిఖరం_లోపల_రిజిస్టరు(o)) {
                op_సూచీ = i;
            } లేకపోతే {
                slots[i] = Get_దొంతర_శిఖరం_స్థానం_సంఖ్య(o);
            }

_           (తీయి(o));
        }

        op = c_fpSelectOps[type - c_m3Type_f32][selector_లోపల_Reg][op_సూచీ];
#లేకపోతే
        _throw(m3Err_unknownOpcode);
#అయితే_ముగింపు
    } లేకపోతే ఒకవేళ (IsIntType(type) or IsRefType(type)) {
        // 'sss' operation doesn't consume a register, so might have to protected its contents
        ఒకవేళ (not Is_దొంతర_శిఖరం_లోపల_రిజిస్టరు(o) and
            not Is_దొంతర_శిఖరం_Minus1_లోపల_రిజిస్టరు(o) and
            not Is_దొంతర_శిఖరం_Minus2_లోపల_రిజిస్టరు(o)) {
_           (Preserve_రిజిస్టరు_If_Occupied(o, type));
        }

        u32 op_సూచీ = 3;  // op_Select_*_sss

        కొరకు (u32 i = 0; i < 3; ++i) {
            ఒకవేళ (Is_దొంతర_శిఖరం_లోపల_రిజిస్టరు(o)) {
                op_సూచీ = i;
            } లేకపోతే {
                slots[i] = Get_దొంతర_శిఖరం_స్థానం_సంఖ్య(o);
            }

_           (తీయి(o));
        }

        // a reference is a pointer-sized integer as far as select is concerned
        u32 type_సూచీ = IsRefType(type) ? ((M3_SIZEOF_PTR == 8) ? 1 : 0)
                                        : (u32)(type - c_m3Type_i32);

        op = c_intSelectOps[type_సూచీ][op_సూచీ];
    } లేకపోతే ఒకవేళ (not Is_దొంతర_Polymorphic(o)) {
        _throw(m3_Err_function_దొంతర_Underrun);
    }

    EmitOp(o, op);
    కొరకు (u32 i = 0; i < 3; i++) {
        ఒకవేళ (Is_Valid_స్థానం(slots[i])) {
            Emit_స్థానం_స్థానభ్రంశం(o, slots[i]);
        }
    }
_   (నెట్టు_రిజిస్టరు(o, type));

    _catch: తిరిగి ఫలితం;
}

స్థిర
M3_ఫలితం Compile_Drop (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_ఫలితం ఫలితం = తీయి(o);
    ఒకవేళ (d_m3_Log_Wasm_దొంతర) {
        dump_type_దొంతర(o);
    }
    తిరిగి ఫలితం;
}

స్థిర
M3_ఫలితం Compile_Nop (IM3Compilation o, m3opcode_t i_opcode)
{
    తిరిగి m3Err_none;
}

స్థిర
M3_ఫలితం Compile_Unreachable (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_ఫలితం ఫలితం;

_   (కూడు_Trap_రికార్డు(o));

_   (EmitOp(o, op_Unreachable));
_   (సమితి_దొంతర_Polymorphic(o));

    _catch:
    తిరిగి ఫలితం;
}


// OPTZ: currently all stack slot indices take up a full word, but
// dual stack source operands could be packed together
స్థిర
M3_ఫలితం Compile_Operator (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_ఫలితం ఫలితం;

    // Declared before the first throw below, which jumps past this point to
    // _catch and so must not skip an initialization.
    IM3Operation op = ఖాళీ;

    IM3OpInfo opInfo = GetOpInfo(i_opcode);
    _throwif(m3Err_unknownOpcode, not opInfo);

    // Spec: validate operand types for load/store operations
    ఒకవేళ (not Is_దొంతర_Polymorphic(o)) {
        // For load ops (stackOffset == 0, unary), the operand is always i32 (address)
        ఒకవేళ (i_opcode >= 0x28 and i_opcode <= 0x35) {
            m3type_t శిఖరం_Type = Get_దొంతర_శిఖరం_Type(o);
            _throwif(m3Err_typeMismatch, శిఖరం_Type != c_m3Type_none and శిఖరం_Type != c_m3Type_i32);
        }

        // For store ops (stackOffset == -2), the address operand must be i32
        ఒకవేళ (i_opcode >= 0x36 and i_opcode <= 0x3e) {
            m3type_t addrType = Get_దొంతర_Type_From_శిఖరం(o, 1);
            _throwif(m3Err_typeMismatch, addrType != c_m3Type_none and addrType != c_m3Type_i32);
        }
    }

    // This preserve is for for FP compare operations.
    // either need additional slot destination operations or the
    // easy fix, move _r0 out of the way.
    // moving out the way might be the optimal solution most often?
    // otherwise, the _r0 reg can get buried down in the stack
    // and be idle & wasted for a moment.
    ఒకవేళ (M3_UNLIKELY(IsFpType(Get_దొంతర_శిఖరం_Type(o)) and IsIntType(opInfo->type))) {
_       (Preserve_రిజిస్టరు_If_Occupied(o, opInfo->type));
    }

    ఒకవేళ (opInfo->దొంతర_స్థానభ్రంశం == 0) {
        ఒకవేళ (Is_దొంతర_శిఖరం_లోపల_రిజిస్టరు(o)) {
            op = opInfo->operations[0]; // _s
        } లేకపోతే {
_           (Preserve_రిజిస్టరు_If_Occupied(o, opInfo->type));
            op = opInfo->operations[1]; // _r
        }
    } లేకపోతే {
        ఒకవేళ (Is_దొంతర_శిఖరం_లోపల_రిజిస్టరు(o)) {
            op = opInfo->operations[0];  // _rs

            ఒకవేళ (Is_దొంతర_శిఖరం_Minus1_లోపల_రిజిస్టరు(o)) {    d_m3Assert (i_opcode == c_waOp_store_f32 or i_opcode == c_waOp_store_f64);
                op = opInfo->operations[3]; // _rr for fp.store
            }
        } లేకపోతే ఒకవేళ (Is_దొంతర_శిఖరం_Minus1_లోపల_రిజిస్టరు(o)) {
            op = opInfo->operations[1]; // _sr

            ఒకవేళ (not op) {  // must be commutative, then
                op = opInfo->operations[0];
            }
        } లేకపోతే {
_           (Preserve_రిజిస్టరు_If_Occupied(o, opInfo->type));     // _ss
            op = opInfo->operations[2];
        }
    }

    ఒకవేళ (M3_LIKELY(op)) {
_       (EmitOp(o, op));

#ఒకవేళ d_m3_Fold_సమితి_Local
        // the op word just written (EmitOp may have bridged pages); NULL in the page-less compile-walk mode
        pc_t patchPC = o->పుట ? GetPC(o) - 1 : ఖాళీ;
#అయితే_ముగింపు

_       (Emit_స్థానం_అంకె_యొక్క_దొంతర_శిఖరం_And_తీయి(o));

        ఒకవేళ (opInfo->దొంతర_స్థానభ్రంశం < 0) {
_           (Emit_స్థానం_అంకె_యొక్క_దొంతర_శిఖరం_And_తీయి(o));
        }

        ఒకవేళ (opInfo->type != c_m3Type_none) {
_           (నెట్టు_రిజిస్టరు(o, opInfo->type));

#ఒకవేళ d_m3_Fold_సమితి_Local
            // record this op as a fold candidate for an immediately following local.set;
            // the operand-form index is recovered by matching against the variant table
            ఒకవేళ (patchPC) {
                కొరకు (u8 form = 0; form < 4; ++form) {
                    ఒకవేళ (opInfo->operations[form] == op) {
                        o->foldPatchPC    = patchPC;
                        o->foldOpcode     = i_opcode;
                        o->foldForm       = form;
                        o->fold_దొంతర_సూచీ = (u16)Get_దొంతర_శిఖరం_సూచీ(o);
                        ఆపు;
                    }
                }
            }
#అయితే_ముగింపు
        }
    } లేకపోతే {
#నిర్వచితమైతే DEBUG
        ఫలితం = ErrorCompile("no operation found for opcode", o, "'%s'", opInfo->name);
#లేకపోతే
        ఫలితం = ErrorCompile("no operation found for opcode", o, "%x", i_opcode);
#అయితే_ముగింపు
        _throw(ఫలితం);
    }

    _catch: తిరిగి ఫలితం;
}

స్థిర
M3_ఫలితం Compile_Convert (IM3Compilation o, m3opcode_t i_opcode)
{
_try {
    IM3OpInfo opInfo = GetOpInfo(i_opcode);
    _throwif(m3Err_unknownOpcode, not opInfo);

    // Spec: validate source operand type for conversion instructions
    ఒకవేళ (not Is_దొంతర_Polymorphic(o)) {
        u8 ఆధారం_Type = c_m3Type_none;
        ఎంపిక (i_opcode)
        // clang-format off
        {
        స్థితి 0xa7:              // i32.wrap/i64
            ఆధారం_Type = c_m3Type_i64; ఆపు;
        స్థితి 0xa8: స్థితి 0xa9:   // i32.trunc_s/f32, i32.trunc_u/f32
        స్థితి 0xae: స్థితి 0xaf:   // i64.trunc_s/f32, i64.trunc_u/f32
        స్థితి 0xbb:              // f64.promote/f32
        స్థితి 0xbc:              // i32.reinterpret/f32
            ఆధారం_Type = c_m3Type_f32; ఆపు;
        స్థితి 0xaa: స్థితి 0xab:   // i32.trunc_s/f64, i32.trunc_u/f64
        స్థితి 0xb0: స్థితి 0xb1:   // i64.trunc_s/f64, i64.trunc_u/f64
        స్థితి 0xb6:              // f32.demote/f64
        స్థితి 0xbd:              // i64.reinterpret/f64
            ఆధారం_Type = c_m3Type_f64; ఆపు;
        స్థితి 0xac: స్థితి 0xad:   // i64.extend_s/i32, i64.extend_u/i32
        స్థితి 0xb2: స్థితి 0xb3:   // f32.convert_s/i32, f32.convert_u/i32
        స్థితి 0xb7: స్థితి 0xb8:   // f64.convert_s/i32, f64.convert_u/i32
        స్థితి 0xbe:              // f32.reinterpret/i32
            ఆధారం_Type = c_m3Type_i32; ఆపు;
        స్థితి 0xb4: స్థితి 0xb5:   // f32.convert_s/i64, f32.convert_u/i64
        స్థితి 0xb9: స్థితి 0xba:   // f64.convert_s/i64, f64.convert_u/i64
        స్థితి 0xbf:              // f64.reinterpret/i64
            ఆధారం_Type = c_m3Type_i64; ఆపు;
        అప్రమేయ: ఆపు;
        }
        // clang-format on

        ఒకవేళ (ఆధారం_Type != c_m3Type_none) {
            m3type_t శిఖరం_Type = Get_దొంతర_శిఖరం_Type(o);
            _throwif(m3Err_typeMismatch, శిఖరం_Type != c_m3Type_none and not Is_Sub_Type_యొక్క(శిఖరం_Type, ఆధారం_Type));
        }
    }

    bool dest_లోపల_స్థానం   = Is_రిజిస్టరు_Type_Allocated(o, opInfo->type);
    bool ఆధారం_లోపల_స్థానం = Is_దొంతర_శిఖరం_లోపల_స్థానం(o);

    IM3Operation op = opInfo->operations[dest_లోపల_స్థానం * 2 + ఆధారం_లోపల_స్థానం];

_   (EmitOp(o, op));
_   (Emit_స్థానం_అంకె_యొక్క_దొంతర_శిఖరం_And_తీయి(o));

    ఒకవేళ (dest_లోపల_స్థానం) {
_       (నెట్టు_Allocated_స్థానం_And_Emit_2(o, opInfo->type))
    } లేకపోతే {
_       (నెట్టు_రిజిస్టరు(o, opInfo->type))
    }
}
    _catch: తిరిగి ఫలితం;
}

#ఒకవేళ d_m3HasWideArithmetic

// The wide arithmetic proposal: i64.add128 and i64.sub128 over two 128-bit values
// given as i64 halves, and i64.mul_wide_s/u giving the full product of two i64s.
// Each gives back a 128-bit result the same way, low half first. The operands are
// all read from slots, and of the two results the high half, which ends up on top,
// goes to _r0 and the low half to a newly allocated slot - so every operation has
// the one form, whatever shape the stack was in.
స్థిర
M3_ఫలితం Compile_WideArithmetic (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_ఫలితం ఫలితం = m3Err_none;

    IM3Operation op          = op_i64_Add128;
    u32          అంకె_Operands = 4;

    ఎంపిక (i_opcode)
    // clang-format off
    {
    స్థితి c_waOp_i64Sub128:   op = op_i64_Sub128;                          ఆపు;
    స్థితి c_waOp_i64MulWideS: op = op_i64_MultiplyWide; అంకె_Operands = 2;   ఆపు;
    స్థితి c_waOp_i64MulWideU: op = op_u64_MultiplyWide; అంకె_Operands = 2;   ఆపు;
    అప్రమేయ: ఆపు;
    }
    // clang-format on

_   (Preserve_రిజిస్టరు_If_Occupied(o, c_m3Type_i64));

_   (EmitOp(o, op));

    // the top of the stack first: the high half of the second operand, for the
    // 128-bit ones
    కొరకు (u32 i = 0; i < అంకె_Operands; ++i) {
_       (Emit_స్థానం_అంకె_యొక్క_దొంతర_శిఖరం_And_తీయి(o));
    }

_   (నెట్టు_Allocated_స్థానం_And_Emit_2(o, c_m3Type_i64));      // the low half
_   (నెట్టు_రిజిస్టరు(o, c_m3Type_i64));                  // the high half

    _catch: తిరిగి ఫలితం;
}

#అయితే_ముగింపు // d_m3HasWideArithmetic

#ఒకవేళ d_m3HasMemory64

// Replaces the 64-bit address operand of a load or store with the checked
// 32-bit effective address op_CheckAddr64 leaves behind, so that the access
// itself compiles exactly as it would against a 32-bit memory: one slot to read
// the address from, and an offset of zero folded in already.
//
// Where the address sits under other operands - the value of a store, the
// operands of an atomic - it is rewritten where it stands rather than popped:
// its stack entry is repointed at the scratch slot and retyped, and the slot it
// used to occupy released. i_numOperands counts the address and all above it.
స్థిర
M3_ఫలితం EmitCheckAddr64 (IM3Compilation o, u64 i_స్థానభ్రంశం, u16 i_అంకె_Operands)
{
    M3_ఫలితం ఫలితం = m3Err_none;

    // The address has to be in a slot for op_CheckAddr64 to read it. Spilling
    // r0 wholesale also keeps the value of a store out of it, which is what
    // leaves the address as the one operand still in a register to worry about.
_   (Preserve_రిజిస్టరు_If_Occupied(o, c_m3Type_i64));

    // unreachable code: the operands the instruction names may not be there
    ఒకవేళ (o->దొంతర_సూచీ < o->ఖండం.ఖండం_దొంతర_సూచీ + i_అంకె_Operands) {
        తిరిగి ఫలితం;
    }

    {
        u16      దొంతర_సూచీ = (u16)(o->దొంతర_సూచీ - i_అంకె_Operands);
        u16      src_స్థానం    = o->wasm_దొంతర[దొంతర_సూచీ];
        m3type_t srcType    = o->type_దొంతర[దొంతర_సూచీ];

        u16 dst_స్థానం = c_స్థానం_వాడని;

        // Checked here rather than left to the validator, which a build may
        // have switched off: reading a slot as an i64 that only holds an i32
        // would reach past what was allocated for it.
        _throwif(m3Err_typeMismatch, పునాది_Type_యొక్క(srcType) != c_m3Type_i64);

        // an address is an integer, so nothing should be left in a register
        // once r0 has been preserved
        _throwif(m3Err_typeMismatch, Is_రిజిస్టరు_స్థానం_Alias(src_స్థానం));
        _throwif(m3_Err_function_దొంతర_Underrun, src_స్థానం >= o->స్థానం_Max_Allocated_సూచీ_Plus_One);

        // allocated before the source is released, so the two cannot be the
        // same slot - op_CheckAddr64 reads one and writes the other
_       (AllocateSlots(o, &dst_స్థానం, c_m3Type_i32));

_       (EmitOp(o, op_CheckAddr64));
        Emit_స్థానం_స్థానభ్రంశం(o, src_స్థానం);
        Emit_స్థానం_స్థానభ్రంశం(o, dst_స్థానం);
        EmitConstant64(o, i_స్థానభ్రంశం);

        o->wasm_దొంతర[దొంతర_సూచీ] = dst_స్థానం;
        o->type_దొంతర[దొంతర_సూచీ] = c_m3Type_i32;

        // locals and the constant table outlive the instruction; a dynamic
        // slot the address was computed into does not
        ఒకవేళ (src_స్థానం >= o->స్థానం_మొదటి_Dynamic_సూచీ) {
            Deallocate_స్థానం(o, src_స్థానం, srcType);
        }
    }

    _catch: తిరిగి ఫలితం;
}

#అయితే_ముగింపు // d_m3HasMemory64


స్థిర
M3_ఫలితం Compile_ఎక్కించు_Store (IM3Compilation o, m3opcode_t i_opcode)
{
_try {
    u32 alignHint, జ్ఞాపకం_Idx;
    u64 జ్ఞాపకం_స్థానభ్రంశం;

    // alignHint is checked by the validator
_   (Read_జ్ఞాపకం_Arg(&alignHint, &జ్ఞాపకం_Idx, &జ్ఞాపకం_స్థానభ్రంశం, &o->wasm, o->wasm_చివర));
                                                                        m3log (compile, d_indent " (memory = %d; offset = %llu)", get_indention_string (o), జ్ఞాపకం_Idx, (గుర్తులేని దీర్ఘ దీర్ఘ) జ్ఞాపకం_స్థానభ్రంశం);
    _throwif(m3_Err_unknown_జ్ఞాపకం, జ్ఞాపకం_Idx >= o->module->అంకె_Memories);

    IM3OpInfo opInfo = GetOpInfo(i_opcode);
    _throwif(m3Err_unknownOpcode, not opInfo);

    bool isMemory64 = o->module->memories[జ్ఞాపకం_Idx]->isMemory64;

    // Spec: the static offset has to be in range of the address type. Checked
    // here as well as in the validator, so that a build without one still
    // refuses the module rather than silently truncating the offset.
    _throwif(m3Err_wasmMalformed, not isMemory64 and జ్ఞాపకం_స్థానభ్రంశం > 0xFFFFFFFFull);

    ఒకవేళ (IsFpType(opInfo->type)) {
_       (Preserve_రిజిస్టరు_If_Occupied(o, c_m3Type_f64));
    }

    ఒకవేళ (జ్ఞాపకం_Idx) {
_       (Emit_సమితి_జ్ఞాపకం(o, జ్ఞాపకం_Idx));
    }

#ఒకవేళ d_m3HasMemory64
    ఒకవేళ (isMemory64) {
        // An offset this far out puts every address out of bounds, so rather
        // than carrying it, clamp it to the limit: op_CheckAddr64 then traps
        // on whatever address it is given, which is the outcome either way.
        ఒకవేళ (జ్ఞాపకం_స్థానభ్రంశం > d_m3_చిరునామా_హద్దు) {
            జ్ఞాపకం_స్థానభ్రంశం = d_m3_చిరునామా_హద్దు;
        }

_       (EmitCheckAddr64(o, జ్ఞాపకం_స్థానభ్రంశం, opInfo->దొంతర_స్థానభ్రంశం < 0 ? 2 : 1));

        // op_CheckAddr64 has folded the offset in already
        జ్ఞాపకం_స్థానభ్రంశం = 0;
    }
#అయితే_ముగింపు

_   (Compile_Operator(o, i_opcode));

    EmitConstant32(o, (u32)జ్ఞాపకం_స్థానభ్రంశం);

    ఒకవేళ (జ్ఞాపకం_Idx) {
_       (Emit_సమితి_జ్ఞాపకం(o, 0));
    }
}
    _catch: తిరిగి ఫలితం;
}


#ఒకవేళ d_m3HasAtomics

// What an atomic instruction takes and gives back, from its sub-opcode. The
// accesses come in runs of seven, one per combination of value type and cell width:
// i32/32, i64/64, i32/8, i32/16, i64/8, i64/16, i64/32. Those runs are told apart by
// where they start.
//
// The operand count includes the address. The top operand is the one that ends up
// in a register, so its type is the one to check on the way out; a store has no
// result, which is c_m3Type_none.
స్థిర
శూన్య GetAtomicShape (u32 i_sub, u16* o_అంకె_Operands, m3type_t* o_శిఖరం_Type, m3type_t* o_ఫలితం_Type)
{
    // clang-format off
    స్థిర స్థిరరాశి m3type_t c_విలువ_Types [7] = {
        c_m3Type_i32, c_m3Type_i64,
        c_m3Type_i32, c_m3Type_i32,
        c_m3Type_i64, c_m3Type_i64, c_m3Type_i64
    };
    // clang-format on

    ఒకవేళ (i_sub == 0x00) {                    // memory.atomic.notify: address, count
        *o_అంకె_Operands = 2;
        *o_శిఖరం_Type     = c_m3Type_i32;
        *o_ఫలితం_Type  = c_m3Type_i32;
    } లేకపోతే ఒకవేళ (i_sub <= 0x02) {             // memory.atomic.wait32/64: address, expected, timeout
        *o_అంకె_Operands = 3;
        *o_శిఖరం_Type     = c_m3Type_i64;
        *o_ఫలితం_Type  = c_m3Type_i32;
    } లేకపోతే ఒకవేళ (i_sub <= 0x16) {             // loads: address
        *o_అంకె_Operands = 1;
        *o_శిఖరం_Type     = c_m3Type_i32;
        *o_ఫలితం_Type  = c_విలువ_Types[i_sub - 0x10];
    } లేకపోతే ఒకవేళ (i_sub <= 0x1d) {             // stores: address, value
        *o_అంకె_Operands = 2;
        *o_శిఖరం_Type     = c_విలువ_Types[i_sub - 0x17];
        *o_ఫలితం_Type  = c_m3Type_none;
    } లేకపోతే ఒకవేళ (i_sub <= 0x47) {             // read-modify-write: address, value
        *o_అంకె_Operands = 2;
        *o_శిఖరం_Type     = c_విలువ_Types[(i_sub - 0x1e) % 7];
        *o_ఫలితం_Type  = *o_శిఖరం_Type;
    } లేకపోతే {                                // cmpxchg: address, expected, replacement
        *o_అంకె_Operands = 3;
        *o_శిఖరం_Type     = c_విలువ_Types[i_sub - 0x48];
        *o_ఫలితం_Type  = *o_శిఖరం_Type;
    }
}


// The description an access takes as its last immediate: see m3_exec_atomic.h
స్థిర
u32 GetAtomicDesc (u32 i_sub)
{
    // per variant, the log2 of the width of the cell
    స్థిర స్థిరరాశి u8 c_log2Widths[7] = { 2, 3, 0, 1, 0, 1, 2 };

    u32 variant, రకం = 0;

    ఒకవేళ (i_sub <= 0x16) {
        variant = i_sub - 0x10;
    } లేకపోతే ఒకవేళ (i_sub <= 0x1d) {
        variant = i_sub - 0x17;
    } లేకపోతే ఒకవేళ (i_sub <= 0x47) {
        variant = (i_sub - 0x1e) % 7;
        రకం    = (i_sub - 0x1e) / 7;         // add, sub, and, or, xor, xchg
    } లేకపోతే {
        variant = i_sub - 0x48;
    }

    // variants 1 and 4..6 are the i64 ones
    u32 is64 = (variant == 1 or variant >= 4) ? 1u : 0u;

    తిరిగి c_log2Widths[variant] | (is64 << 2) | (రకం << 3);
}


// Every atomic access is one op, whose operands are the topmost in _r0 and the rest
// in slots, topmost first. The address is always the deepest, so it is the last slot
// (or, for a load, the register), followed by the memarg offset. The ops check the
// address themselves: for an atomic that includes its alignment, which only the
// effective address can be asked.
స్థిర
M3_ఫలితం Compile_Atomic (IM3Compilation o, m3opcode_t i_opcode)
{
_try {
    u32 alignHint, జ్ఞాపకం_Idx;
    u64 జ్ఞాపకం_స్థానభ్రంశం;

    u16      అంకె_Operands = 0;
    m3type_t శిఖరం_Type     = c_m3Type_none;
    m3type_t ఫలితం_Type  = c_m3Type_none;

    ఒకవేళ (i_opcode == c_waOp_atomicFence) {
        // the reserved byte, which the validator has checked to be zero. There
        // is nothing to order between accesses that one thread makes in sequence.
        u8 reserved;
_       (Read_u8(&reserved, &o->wasm, o->wasm_చివర));
    } లేకపోతే {
        // alignHint is checked by the validator
_       (Read_జ్ఞాపకం_Arg(&alignHint, &జ్ఞాపకం_Idx, &జ్ఞాపకం_స్థానభ్రంశం, &o->wasm, o->wasm_చివర));
                                                                        m3log (compile, d_indent " (memory = %d; offset = %llu)", get_indention_string (o), జ్ఞాపకం_Idx, (గుర్తులేని దీర్ఘ దీర్ఘ) జ్ఞాపకం_స్థానభ్రంశం);
        _throwif(m3_Err_unknown_జ్ఞాపకం, జ్ఞాపకం_Idx >= o->module->అంకె_Memories);

        IM3OpInfo opInfo = GetOpInfo(i_opcode);
        _throwif(m3Err_unknownOpcode, not opInfo);

        bool isMemory64 = o->module->memories[జ్ఞాపకం_Idx]->isMemory64;

        // see Compile_Load_Store
        _throwif(m3Err_wasmMalformed, not isMemory64 and జ్ఞాపకం_స్థానభ్రంశం > 0xFFFFFFFFull);

        GetAtomicShape(i_opcode & 0xFF, &అంకె_Operands, &శిఖరం_Type, &ఫలితం_Type);

        ఒకవేళ (జ్ఞాపకం_Idx) {
_           (Emit_సమితి_జ్ఞాపకం(o, జ్ఞాపకం_Idx));
        }

#  ఒకవేళ d_m3HasMemory64
        ఒకవేళ (isMemory64) {
            // see Compile_Load_Store
            ఒకవేళ (జ్ఞాపకం_స్థానభ్రంశం > d_m3_చిరునామా_హద్దు) {
                జ్ఞాపకం_స్థానభ్రంశం = d_m3_చిరునామా_హద్దు;
            }

_           (EmitCheckAddr64(o, జ్ఞాపకం_స్థానభ్రంశం, అంకె_Operands));
            జ్ఞాపకం_స్థానభ్రంశం = 0;
        }
#  endif

_       (Copy_దొంతర_శిఖరం_గమ్యం_రిజిస్టరు(o, false));
_       (EmitOp(o, opInfo->operations[0]));
_       (తీయి_Type(o, శిఖరం_Type));

        కొరకు (u16 i = 1; i < అంకె_Operands; ++i) {
_           (Emit_స్థానం_అంకె_యొక్క_దొంతర_శిఖరం_And_తీయి(o));
        }

        EmitConstant32(o, (u32)జ్ఞాపకం_స్థానభ్రంశం);
        ఒకవేళ (i_opcode >= c_wa_Op_atomic_మొదటి_Access) {
            EmitConstant32(o, GetAtomicDesc(i_opcode & 0xFF));
        }

        ఒకవేళ (జ్ఞాపకం_Idx) {
_           (Emit_సమితి_జ్ఞాపకం(o, 0));
        }

        ఒకవేళ (ఫలితం_Type != c_m3Type_none) {
_           (నెట్టు_రిజిస్టరు(o, ఫలితం_Type));
        }
    }
}
    _catch: తిరిగి ఫలితం;
}

#అయితే_ముగింపు // d_m3HasAtomics


M3_ఫలితం CompileRawFunction (IM3Module io_module, IM3Function io_function, స్థిరరాశి శూన్య* i_function, స్థిరరాశి శూన్య* i_userdata)
{
    d_m3Assert(io_module->runtime);

    IM3_సంకేతం_పుట పుట = Acquire_సంకేతం_పుట_With_సామర్థ్యం(io_module->runtime, 4);

    ఒకవేళ (పుట) {
        io_function->compiled = Get_పుట_PC(పుట);
        io_function->module   = io_module;

        // Unless a host module's ABI names a memory (m3_BindImportMemory), a
        // host function addresses memory 0 - what a bare i32 guest pointer means
        io_function->host_జ్ఞాపకం = io_module->memory0;

        Emit_మాట(పుట, op_CallRawFunction);
        Emit_మాట(పుట, i_function);
        Emit_మాట(పుట, io_function);
        Emit_మాట(పుట, i_userdata);

        Release_సంకేతం_పుట(io_module->runtime, పుట);
        తిరిగి m3Err_none;
    } లేకపోతే {
        తిరిగి m3_Err_malloc_Failed_సంకేతం_పుట;
    }
}


// A table entry names every field it sets, so the struct can order them for
// packing; the columns follow that order, since M3OP names them positionally.
//
// d_logOp, d_logOp2 macros aren't actually used by the compiler, just codepage decoding (d_m3LogCodePages = 1)
#నిర్వచించు d_ops(...)                          .operations = { __VA_ARGS__ }

#నిర్వచించు d_logOp(OP)                         d_ops( op_##OP,                 ఖాళీ,                       ఖాళీ,                       ఖాళీ )
#నిర్వచించు d_logOp2(OP1,OP2)                   d_ops( op_##OP1,                op_##OP2,                   ఖాళీ,                       ఖాళీ )

#నిర్వచించు d_ఖాళీలు_Op_List                       d_ops( ఖాళీ,                    ఖాళీ,                       ఖాళీ,                       ఖాళీ )
#నిర్వచించు d_unaryOpList(TYPE, NAME)           d_ops( op_##TYPE##_##NAME##_r,  op_##TYPE##_##NAME##_s,     ఖాళీ,                       ఖాళీ )
#నిర్వచించు d_binOpList(TYPE, NAME)             d_ops( op_##TYPE##_##NAME##_rs, op_##TYPE##_##NAME##_sr,    op_##TYPE##_##NAME##_ss,    ఖాళీ )
#నిర్వచించు d_storeFpOpList(TYPE, NAME)         d_ops( op_##TYPE##_##NAME##_rs, op_##TYPE##_##NAME##_sr,    op_##TYPE##_##NAME##_ss,    op_##TYPE##_##NAME##_rr )
#నిర్వచించు d_commutativeBinOpList(TYPE, NAME)  d_ops( op_##TYPE##_##NAME##_rs, ఖాళీ,                       op_##TYPE##_##NAME##_ss,    ఖాళీ )

#నిర్వచించు d_convertOpList(OP)                 d_ops( op_##OP##_r_r,           op_##OP##_r_s,              op_##OP##_s_r,              op_##OP##_s_s )


// Every M3Compiler the tables below name, as one list, so that the index enum and
// the array of pointers can never drift apart. Each entry must carry the same
// config guard as the function's definition.
#నిర్వచించు d_m3CompilerList(_)             \
    _( Compile_Unreachable )            \
    _( Compile_Nop )                    \
    _( Compile_Loop_Or_ఖండం )            \
    _( Compile_If )                     \
    _( Compile_చివర )                    \
    _( Compile_Branch )                 \
    _( Compile_Branch_పట్టిక )            \
    _( Compile_Return )                 \
    _( Compile_Call )                   \
    _( Compile_Call_పరోక్ష )           \
    _( Compile_Drop )                   \
    _( Compile_Select )                 \
    _( Compile_GetLocal )               \
    _( Compile_సమితి_Local )               \
    _( Compile_Get_సమితి_Global )           \
    _( Compile_ఎక్కించు_Store )             \
    _( Compile_జ్ఞాపకం_కొలత )            \
    _( Compile_జ్ఞాపకం_Grow )            \
    _( Compile_జ్ఞాపకం_Init )            \
    _( Compile_జ్ఞాపకం_Copy_Fill )        \
    _( Compile_దత్తాంశం_Drop )              \
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
    d_m3_Compiler_List_దొంతర_Switching( _ )

#ఒకవేళ d_m3ImplementFloat
#  define d_m3CompilerList_f(_)         \
    _( Compile_Const_f32 )              \
    _( Compile_Const_f64 )
#లేకపోతే
#  define d_m3CompilerList_f(_)
#అయితే_ముగింపు

#ఒకవేళ d_m3HasRefTypes
#  define d_m3CompilerList_refTypes(_)  \
    _( Compile_Select_Typed )           \
    _( Compile_Ref_Null )               \
    _( Compile_Ref_IsNull )             \
    _( Compile_Ref_Func )               \
    _( Compile_పట్టిక_Get_సమితి )           \
    _( Compile_పట్టిక_Init )             \
    _( Compile_పట్టిక_Copy )             \
    _( Compile_పట్టిక_కొలత )             \
    _( Compile_పట్టిక_Grow_Fill )         \
    _( Compile_Elem_Drop )
#లేకపోతే
#  define d_m3CompilerList_refTypes(_)
#అయితే_ముగింపు

#ఒకవేళ d_m3HasTypedRefs
#  define d_m3CompilerList_typedRefs(_) \
    _( Compile_CallRef )                \
    _( Compile_Ref_AsNonNull )
#లేకపోతే
#  define d_m3CompilerList_typedRefs(_)
#అయితే_ముగింపు

#ఒకవేళ d_m3HasExceptionHandling
#  define d_m3CompilerList_eh(_)        \
    _( Compile_Throw )                  \
    _( Compile_ThrowRef )               \
    _( Compile_Try_పట్టిక )
#లేకపోతే
#  define d_m3CompilerList_eh(_)
#అయితే_ముగింపు

#ఒకవేళ d_m3HasAtomics
#  define d_m3CompilerList_atomics(_)   \
    _( Compile_Atomic )
#లేకపోతే
#  define d_m3CompilerList_atomics(_)
#అయితే_ముగింపు

#ఒకవేళ d_m3HasWideArithmetic
#  define d_m3CompilerList_wide(_)      \
    _( Compile_WideArithmetic )
#లేకపోతే
#  define d_m3CompilerList_wide(_)
#అయితే_ముగింపు

#ఒకవేళ d_m3_Has_దొంతర_Switching
#  define d_m3_Compiler_List_దొంతర_Switching(_) \
    _( Compile_ContNew )                     \
    _( Compile_ContBind )                    \
    _( Compile_Suspend )                     \
    _( Compile_Resume )                      \
    _( Compile_Switch )
#లేకపోతే
#  define d_m3_Compiler_List_దొంతర_Switching(_)
#అయితే_ముగింపు

// Index 0 is "this opcode has no compiler", so that a zeroed entry - which is
// what M3OP_RESERVED and the reserved holes in the tables are - reads as one.
గణన {
    c_cc_NULL = 0,
#నిర్వచించు d_m3CompilerEnum(NAME)  c_cc_##NAME,
    d_m3CompilerList(d_m3CompilerEnum)
#నిర్వచనం_రద్దు d_m3CompilerEnum
};

స్థిర స్థిరరాశి M3Compiler c_compilers[] = {
    ఖాళీ,
#నిర్వచించు d_m3CompilerPtr(NAME)   NAME,
    d_m3CompilerList(d_m3CompilerPtr)
#నిర్వచనం_రద్దు d_m3CompilerPtr
};

// Names a table entry's compiler by index. d_cc (NULL) is the no-compiler entry.
#నిర్వచించు d_cc(NAME)                          .compiler = c_cc_##NAME

స్థిర
M3Compiler GetCompiler (IM3OpInfo i_opInfo)
{
    d_m3Assert(i_opInfo->compiler < M3_లెక్క_యొక్క(c_compilers));
    తిరిగి c_compilers[i_opInfo->compiler];
}

// clang-format off
స్థిరరాశి M3OpInfo c_operations[] =
{
    M3OP( "unreachable",           0,  none,   d_cc(Compile_Unreachable),  d_logOp (Unreachable) ),                   // 0x00
    M3OP( "nop",                   0,  none,   d_cc(Compile_Nop),          d_ఖాళీలు_Op_List ),                           // 0x01 .
    M3OP( "block",                 0,  none,   d_cc(Compile_Loop_Or_ఖండం),  d_ఖాళీలు_Op_List ),                           // 0x02
    M3OP( "loop",                  0,  none,   d_cc(Compile_Loop_Or_ఖండం),  d_logOp (Loop) ),                          // 0x03
    M3OP( "if",                    -1, none,   d_cc(Compile_If),           d_ఖాళీలు_Op_List ),                           // 0x04
    M3OP( "else",                  0,  none,   d_cc(Compile_Nop),          d_ఖాళీలు_Op_List ),                           // 0x05

    M3OP_RESERVED,  M3OP_RESERVED,                                                                                    // 0x06...0x07

#ఒకవేళ d_m3HasExceptionHandling
    M3OP( "throw",                 0,  none,   d_cc(Compile_Throw),        d_logOp (Throw) ),                         // 0x08
    M3OP_RESERVED,                                                                                                    // 0x09
    M3OP( "throw_ref",             0,  none,   d_cc(Compile_ThrowRef),     d_logOp (ThrowRef) ),                      // 0x0a
#లేకపోతే
    M3OP_RESERVED,  M3OP_RESERVED, M3OP_RESERVED,                                                                     // 0x08...0x0a
#అయితే_ముగింపు

    M3OP( "end",                   0,  none,   d_cc(Compile_చివర),          d_ఖాళీలు_Op_List ),                           // 0x0b
    M3OP( "br",                    0,  none,   d_cc(Compile_Branch),       d_logOp (Branch) ),                        // 0x0c
    M3OP( "br_if",                 -1, none,   d_cc(Compile_Branch),       d_logOp2 (BranchIf_r, BranchIf_s) ),       // 0x0d
    M3OP( "br_table",              -1, none,   d_cc(Compile_Branch_పట్టిక),  d_logOp (Branch_పట్టిక) ),                   // 0x0e
    M3OP( "return",                0,  any,    d_cc(Compile_Return),       d_logOp (Return) ),                        // 0x0f
    M3OP( "call",                  0,  any,    d_cc(Compile_Call),         d_logOp (Call) ),                          // 0x10
    M3OP( "call_indirect",         0,  any,    d_cc(Compile_Call_పరోక్ష), d_logOp (Call_పరోక్ష) ),                  // 0x11
    M3OP( "return_call",           0,  any,    d_cc(Compile_Call),         d_logOp (ReturnCall) ),                    // 0x12
    M3OP( "return_call_indirect",  0,  any,    d_cc(Compile_Call_పరోక్ష), d_logOp (Return_Call_పరోక్ష) ),            // 0x13

#ఒకవేళ d_m3HasTypedRefs
    M3OP( "call_ref",              0,  any,    d_cc(Compile_CallRef),      d_logOp (CallRef) ),                       // 0x14
    M3OP( "return_call_ref",       0,  any,    d_cc(Compile_CallRef),      d_logOp (ReturnCallRef) ),                 // 0x15
#లేకపోతే
    M3OP_RESERVED,  M3OP_RESERVED,                                                                                    // 0x14...
#అయితే_ముగింపు
    M3OP_RESERVED,  M3OP_RESERVED, M3OP_RESERVED, M3OP_RESERVED,                                                      // ...0x19

    M3OP( "drop",                  -1, none,   d_cc(Compile_Drop),         d_ఖాళీలు_Op_List ),                           // 0x1a
    M3OP( "select",                -2, any,    d_cc(Compile_Select),       d_ఖాళీలు_Op_List ),                           // 0x1b

#ఒకవేళ d_m3HasRefTypes
    M3OP( "select.t",              -2, any,    d_cc(Compile_Select_Typed), d_ఖాళీలు_Op_List ),                           // 0x1c
#లేకపోతే
    M3OP_RESERVED,                                                                                                    // 0x1c
#అయితే_ముగింపు
    M3OP_RESERVED,  M3OP_RESERVED,                                                                                    // 0x1d...0x1e

#ఒకవేళ d_m3HasExceptionHandling
    M3OP( "try_table",             0,  none,   d_cc(Compile_Try_పట్టిక),     d_logOp (Try_పట్టిక) ),                      // 0x1f
#లేకపోతే
    M3OP_RESERVED,                                                                                                    // 0x1f
#అయితే_ముగింపు

    M3OP( "local.get",             1,  any,    d_cc(Compile_GetLocal),     d_ఖాళీలు_Op_List ),                           // 0x20
    M3OP( "local.set",             1,  none,   d_cc(Compile_సమితి_Local),     d_ఖాళీలు_Op_List ),                           // 0x21
    M3OP( "local.tee",             0,  any,    d_cc(Compile_సమితి_Local),     d_ఖాళీలు_Op_List ),                           // 0x22
    M3OP( "global.get",            1,  none,   d_cc(Compile_Get_సమితి_Global), d_ఖాళీలు_Op_List ),                           // 0x23
    M3OP( "global.set",            1,  none,   d_cc(Compile_Get_సమితి_Global), d_ఖాళీలు_Op_List ),                           // 0x24

#ఒకవేళ d_m3HasRefTypes
    M3OP( "table.get",             0,  any,    d_cc(Compile_పట్టిక_Get_సమితి), d_ఖాళీలు_Op_List ),                           // 0x25
    M3OP( "table.set",             -2, none,   d_cc(Compile_పట్టిక_Get_సమితి), d_ఖాళీలు_Op_List ),                           // 0x26
#లేకపోతే
    M3OP_RESERVED,  M3OP_RESERVED,                                                                                    // 0x25...0x26
#అయితే_ముగింపు
    M3OP_RESERVED,                                                                                                    // 0x27

    M3OP( "i32.load",              0,  i_32,   d_cc(Compile_ఎక్కించు_Store),   d_unaryOpList (i32, ఎక్కించు_i32) ),           // 0x28
    M3OP( "i64.load",              0,  i_64,   d_cc(Compile_ఎక్కించు_Store),   d_unaryOpList (i64, ఎక్కించు_i64) ),           // 0x29
    M3OP_F( "f32.load",            0,  f_32,   d_cc(Compile_ఎక్కించు_Store),   d_unaryOpList (f32, ఎక్కించు_f32) ),           // 0x2a
    M3OP_F( "f64.load",            0,  f_64,   d_cc(Compile_ఎక్కించు_Store),   d_unaryOpList (f64, ఎక్కించు_f64) ),           // 0x2b

    M3OP( "i32.load8_s",           0,  i_32,   d_cc(Compile_ఎక్కించు_Store),   d_unaryOpList (i32, ఎక్కించు_i8) ),            // 0x2c
    M3OP( "i32.load8_u",           0,  i_32,   d_cc(Compile_ఎక్కించు_Store),   d_unaryOpList (i32, ఎక్కించు_u8) ),            // 0x2d
    M3OP( "i32.load16_s",          0,  i_32,   d_cc(Compile_ఎక్కించు_Store),   d_unaryOpList (i32, ఎక్కించు_i16) ),           // 0x2e
    M3OP( "i32.load16_u",          0,  i_32,   d_cc(Compile_ఎక్కించు_Store),   d_unaryOpList (i32, ఎక్కించు_u16) ),           // 0x2f

    M3OP( "i64.load8_s",           0,  i_64,   d_cc(Compile_ఎక్కించు_Store),   d_unaryOpList (i64, ఎక్కించు_i8) ),            // 0x30
    M3OP( "i64.load8_u",           0,  i_64,   d_cc(Compile_ఎక్కించు_Store),   d_unaryOpList (i64, ఎక్కించు_u8) ),            // 0x31
    M3OP( "i64.load16_s",          0,  i_64,   d_cc(Compile_ఎక్కించు_Store),   d_unaryOpList (i64, ఎక్కించు_i16) ),           // 0x32
    M3OP( "i64.load16_u",          0,  i_64,   d_cc(Compile_ఎక్కించు_Store),   d_unaryOpList (i64, ఎక్కించు_u16) ),           // 0x33
    M3OP( "i64.load32_s",          0,  i_64,   d_cc(Compile_ఎక్కించు_Store),   d_unaryOpList (i64, ఎక్కించు_i32) ),           // 0x34
    M3OP( "i64.load32_u",          0,  i_64,   d_cc(Compile_ఎక్కించు_Store),   d_unaryOpList (i64, ఎక్కించు_u32) ),           // 0x35

    M3OP( "i32.store",             -2, none,   d_cc(Compile_ఎక్కించు_Store),   d_binOpList (i32, Store_i32) ),            // 0x36
    M3OP( "i64.store",             -2, none,   d_cc(Compile_ఎక్కించు_Store),   d_binOpList (i64, Store_i64) ),            // 0x37
    M3OP_F( "f32.store",           -2, none,   d_cc(Compile_ఎక్కించు_Store),   d_storeFpOpList (f32, Store_f32) ),        // 0x38
    M3OP_F( "f64.store",           -2, none,   d_cc(Compile_ఎక్కించు_Store),   d_storeFpOpList (f64, Store_f64) ),        // 0x39

    M3OP( "i32.store8",            -2, none,   d_cc(Compile_ఎక్కించు_Store),   d_binOpList (i32, Store_u8) ),             // 0x3a
    M3OP( "i32.store16",           -2, none,   d_cc(Compile_ఎక్కించు_Store),   d_binOpList (i32, Store_i16) ),            // 0x3b

    M3OP( "i64.store8",            -2, none,   d_cc(Compile_ఎక్కించు_Store),   d_binOpList (i64, Store_u8) ),             // 0x3c
    M3OP( "i64.store16",           -2, none,   d_cc(Compile_ఎక్కించు_Store),   d_binOpList (i64, Store_i16) ),            // 0x3d
    M3OP( "i64.store32",           -2, none,   d_cc(Compile_ఎక్కించు_Store),   d_binOpList (i64, Store_i32) ),            // 0x3e

    M3OP( "memory.size",           1,  i_32,   d_cc(Compile_జ్ఞాపకం_కొలత),  d_logOp (Mem_కొలత) ),                       // 0x3f
    M3OP( "memory.grow",           1,  i_32,   d_cc(Compile_జ్ఞాపకం_Grow),  d_logOp (MemGrow) ),                       // 0x40

    M3OP( "i32.const",             1,  i_32,   d_cc(Compile_Const_i32),    d_logOp (Const32) ),                       // 0x41
    M3OP( "i64.const",             1,  i_64,   d_cc(Compile_Const_i64),    d_logOp (Const64) ),                       // 0x42
    M3OP_F( "f32.const",           1,  f_32,   d_cc(Compile_Const_f32),    d_ఖాళీలు_Op_List ),                           // 0x43
    M3OP_F( "f64.const",           1,  f_64,   d_cc(Compile_Const_f64),    d_ఖాళీలు_Op_List ),                           // 0x44

    M3OP( "i32.eqz",               0,  i_32,   d_cc(ఖాళీ),                 d_unaryOpList (i32, Equal_గమ్యం_Zero) ),        // 0x45
    M3OP( "i32.eq",                -1, i_32,   d_cc(ఖాళీ),                 d_commutativeBinOpList (i32, Equal) ),     // 0x46
    M3OP( "i32.ne",                -1, i_32,   d_cc(ఖాళీ),                 d_commutativeBinOpList (i32, NotEqual) ),  // 0x47
    M3OP( "i32.lt_s",              -1, i_32,   d_cc(ఖాళీ),                 d_binOpList (i32, LessThan) ),             // 0x48
    M3OP( "i32.lt_u",              -1, i_32,   d_cc(ఖాళీ),                 d_binOpList (u32, LessThan) ),             // 0x49
    M3OP( "i32.gt_s",              -1, i_32,   d_cc(ఖాళీ),                 d_binOpList (i32, GreaterThan) ),          // 0x4a
    M3OP( "i32.gt_u",              -1, i_32,   d_cc(ఖాళీ),                 d_binOpList (u32, GreaterThan) ),          // 0x4b
    M3OP( "i32.le_s",              -1, i_32,   d_cc(ఖాళీ),                 d_binOpList (i32, LessThanOrEqual) ),      // 0x4c
    M3OP( "i32.le_u",              -1, i_32,   d_cc(ఖాళీ),                 d_binOpList (u32, LessThanOrEqual) ),      // 0x4d
    M3OP( "i32.ge_s",              -1, i_32,   d_cc(ఖాళీ),                 d_binOpList (i32, GreaterThanOrEqual) ),   // 0x4e
    M3OP( "i32.ge_u",              -1, i_32,   d_cc(ఖాళీ),                 d_binOpList (u32, GreaterThanOrEqual) ),   // 0x4f

    M3OP( "i64.eqz",               0,  i_32,   d_cc(ఖాళీ),                 d_unaryOpList (i64, Equal_గమ్యం_Zero) ),        // 0x50
    M3OP( "i64.eq",                -1, i_32,   d_cc(ఖాళీ),                 d_commutativeBinOpList (i64, Equal) ),     // 0x51
    M3OP( "i64.ne",                -1, i_32,   d_cc(ఖాళీ),                 d_commutativeBinOpList (i64, NotEqual) ),  // 0x52
    M3OP( "i64.lt_s",              -1, i_32,   d_cc(ఖాళీ),                 d_binOpList (i64, LessThan) ),             // 0x53
    M3OP( "i64.lt_u",              -1, i_32,   d_cc(ఖాళీ),                 d_binOpList (u64, LessThan) ),             // 0x54
    M3OP( "i64.gt_s",              -1, i_32,   d_cc(ఖాళీ),                 d_binOpList (i64, GreaterThan) ),          // 0x55
    M3OP( "i64.gt_u",              -1, i_32,   d_cc(ఖాళీ),                 d_binOpList (u64, GreaterThan) ),          // 0x56
    M3OP( "i64.le_s",              -1, i_32,   d_cc(ఖాళీ),                 d_binOpList (i64, LessThanOrEqual) ),      // 0x57
    M3OP( "i64.le_u",              -1, i_32,   d_cc(ఖాళీ),                 d_binOpList (u64, LessThanOrEqual) ),      // 0x58
    M3OP( "i64.ge_s",              -1, i_32,   d_cc(ఖాళీ),                 d_binOpList (i64, GreaterThanOrEqual) ),   // 0x59
    M3OP( "i64.ge_u",              -1, i_32,   d_cc(ఖాళీ),                 d_binOpList (u64, GreaterThanOrEqual) ),   // 0x5a

    M3OP_F( "f32.eq",              -1, i_32,   d_cc(ఖాళీ),                 d_commutativeBinOpList (f32, Equal) ),     // 0x5b
    M3OP_F( "f32.ne",              -1, i_32,   d_cc(ఖాళీ),                 d_commutativeBinOpList (f32, NotEqual) ),  // 0x5c
    M3OP_F( "f32.lt",              -1, i_32,   d_cc(ఖాళీ),                 d_binOpList (f32, LessThan) ),             // 0x5d
    M3OP_F( "f32.gt",              -1, i_32,   d_cc(ఖాళీ),                 d_binOpList (f32, GreaterThan) ),          // 0x5e
    M3OP_F( "f32.le",              -1, i_32,   d_cc(ఖాళీ),                 d_binOpList (f32, LessThanOrEqual) ),      // 0x5f
    M3OP_F( "f32.ge",              -1, i_32,   d_cc(ఖాళీ),                 d_binOpList (f32, GreaterThanOrEqual) ),   // 0x60

    M3OP_F( "f64.eq",              -1, i_32,   d_cc(ఖాళీ),                 d_commutativeBinOpList (f64, Equal) ),     // 0x61
    M3OP_F( "f64.ne",              -1, i_32,   d_cc(ఖాళీ),                 d_commutativeBinOpList (f64, NotEqual) ),  // 0x62
    M3OP_F( "f64.lt",              -1, i_32,   d_cc(ఖాళీ),                 d_binOpList (f64, LessThan) ),             // 0x63
    M3OP_F( "f64.gt",              -1, i_32,   d_cc(ఖాళీ),                 d_binOpList (f64, GreaterThan) ),          // 0x64
    M3OP_F( "f64.le",              -1, i_32,   d_cc(ఖాళీ),                 d_binOpList (f64, LessThanOrEqual) ),      // 0x65
    M3OP_F( "f64.ge",              -1, i_32,   d_cc(ఖాళీ),                 d_binOpList (f64, GreaterThanOrEqual) ),   // 0x66

    M3OP( "i32.clz",               0,  i_32,   d_cc(ఖాళీ),                 d_unaryOpList (u32, Clz) ),                // 0x67
    M3OP( "i32.ctz",               0,  i_32,   d_cc(ఖాళీ),                 d_unaryOpList (u32, Ctz) ),                // 0x68
    M3OP( "i32.popcnt",            0,  i_32,   d_cc(ఖాళీ),                 d_unaryOpList (u32, Popcnt) ),             // 0x69

    M3OP( "i32.add",               -1, i_32,   d_cc(ఖాళీ),                 d_commutativeBinOpList (i32, కూడు) ),       // 0x6a
    M3OP( "i32.sub",               -1, i_32,   d_cc(ఖాళీ),                 d_binOpList (i32, వ్యవకలనం) ),             // 0x6b
    M3OP( "i32.mul",               -1, i_32,   d_cc(ఖాళీ),                 d_commutativeBinOpList (i32, Multiply) ),  // 0x6c
    M3OP( "i32.div_s",             -1, i_32,   d_cc(ఖాళీ),                 d_binOpList (i32, Divide) ),               // 0x6d
    M3OP( "i32.div_u",             -1, i_32,   d_cc(ఖాళీ),                 d_binOpList (u32, Divide) ),               // 0x6e
    M3OP( "i32.rem_s",             -1, i_32,   d_cc(ఖాళీ),                 d_binOpList (i32, Remainder) ),            // 0x6f
    M3OP( "i32.rem_u",             -1, i_32,   d_cc(ఖాళీ),                 d_binOpList (u32, Remainder) ),            // 0x70
    M3OP( "i32.and",               -1, i_32,   d_cc(ఖాళీ),                 d_commutativeBinOpList (u32, And) ),       // 0x71
    M3OP( "i32.or",                -1, i_32,   d_cc(ఖాళీ),                 d_commutativeBinOpList (u32, Or) ),        // 0x72
    M3OP( "i32.xor",               -1, i_32,   d_cc(ఖాళీ),                 d_commutativeBinOpList (u32, Xor) ),       // 0x73
    M3OP( "i32.shl",               -1, i_32,   d_cc(ఖాళీ),                 d_binOpList (u32, జరుపు_ఎడమ) ),            // 0x74
    M3OP( "i32.shr_s",             -1, i_32,   d_cc(ఖాళీ),                 d_binOpList (i32, జరుపు_కుడి) ),           // 0x75
    M3OP( "i32.shr_u",             -1, i_32,   d_cc(ఖాళీ),                 d_binOpList (u32, జరుపు_కుడి) ),           // 0x76
    M3OP( "i32.rotl",              -1, i_32,   d_cc(ఖాళీ),                 d_binOpList (u32, Rotl) ),                 // 0x77
    M3OP( "i32.rotr",              -1, i_32,   d_cc(ఖాళీ),                 d_binOpList (u32, Rotr) ),                 // 0x78

    M3OP( "i64.clz",               0,  i_64,   d_cc(ఖాళీ),                 d_unaryOpList (u64, Clz) ),                // 0x79
    M3OP( "i64.ctz",               0,  i_64,   d_cc(ఖాళీ),                 d_unaryOpList (u64, Ctz) ),                // 0x7a
    M3OP( "i64.popcnt",            0,  i_64,   d_cc(ఖాళీ),                 d_unaryOpList (u64, Popcnt) ),             // 0x7b

    M3OP( "i64.add",               -1, i_64,   d_cc(ఖాళీ),                 d_commutativeBinOpList (i64, కూడు) ),       // 0x7c
    M3OP( "i64.sub",               -1, i_64,   d_cc(ఖాళీ),                 d_binOpList (i64, వ్యవకలనం) ),             // 0x7d
    M3OP( "i64.mul",               -1, i_64,   d_cc(ఖాళీ),                 d_commutativeBinOpList (i64, Multiply) ),  // 0x7e
    M3OP( "i64.div_s",             -1, i_64,   d_cc(ఖాళీ),                 d_binOpList (i64, Divide) ),               // 0x7f
    M3OP( "i64.div_u",             -1, i_64,   d_cc(ఖాళీ),                 d_binOpList (u64, Divide) ),               // 0x80
    M3OP( "i64.rem_s",             -1, i_64,   d_cc(ఖాళీ),                 d_binOpList (i64, Remainder) ),            // 0x81
    M3OP( "i64.rem_u",             -1, i_64,   d_cc(ఖాళీ),                 d_binOpList (u64, Remainder) ),            // 0x82
    M3OP( "i64.and",               -1, i_64,   d_cc(ఖాళీ),                 d_commutativeBinOpList (u64, And) ),       // 0x83
    M3OP( "i64.or",                -1, i_64,   d_cc(ఖాళీ),                 d_commutativeBinOpList (u64, Or) ),        // 0x84
    M3OP( "i64.xor",               -1, i_64,   d_cc(ఖాళీ),                 d_commutativeBinOpList (u64, Xor) ),       // 0x85
    M3OP( "i64.shl",               -1, i_64,   d_cc(ఖాళీ),                 d_binOpList (u64, జరుపు_ఎడమ) ),            // 0x86
    M3OP( "i64.shr_s",             -1, i_64,   d_cc(ఖాళీ),                 d_binOpList (i64, జరుపు_కుడి) ),           // 0x87
    M3OP( "i64.shr_u",             -1, i_64,   d_cc(ఖాళీ),                 d_binOpList (u64, జరుపు_కుడి) ),           // 0x88
    M3OP( "i64.rotl",              -1, i_64,   d_cc(ఖాళీ),                 d_binOpList (u64, Rotl) ),                 // 0x89
    M3OP( "i64.rotr",              -1, i_64,   d_cc(ఖాళీ),                 d_binOpList (u64, Rotr) ),                 // 0x8a

    M3OP_F( "f32.abs",             0,  f_32,   d_cc(ఖాళీ),                 d_unaryOpList(f32, Abs) ),                 // 0x8b
    M3OP_F( "f32.neg",             0,  f_32,   d_cc(ఖాళీ),                 d_unaryOpList(f32, Negate) ),              // 0x8c
    M3OP_F( "f32.ceil",            0,  f_32,   d_cc(ఖాళీ),                 d_unaryOpList(f32, Ceil) ),                // 0x8d
    M3OP_F( "f32.floor",           0,  f_32,   d_cc(ఖాళీ),                 d_unaryOpList(f32, Floor) ),               // 0x8e
    M3OP_F( "f32.trunc",           0,  f_32,   d_cc(ఖాళీ),                 d_unaryOpList(f32, Trunc) ),               // 0x8f
    M3OP_F( "f32.nearest",         0,  f_32,   d_cc(ఖాళీ),                 d_unaryOpList(f32, సమీప) ),             // 0x90
    M3OP_F( "f32.sqrt",            0,  f_32,   d_cc(ఖాళీ),                 d_unaryOpList(f32, Sqrt) ),                // 0x91

    M3OP_F( "f32.add",             -1, f_32,   d_cc(ఖాళీ),                 d_commutativeBinOpList (f32, కూడు) ),       // 0x92
    M3OP_F( "f32.sub",             -1, f_32,   d_cc(ఖాళీ),                 d_binOpList (f32, వ్యవకలనం) ),             // 0x93
    M3OP_F( "f32.mul",             -1, f_32,   d_cc(ఖాళీ),                 d_commutativeBinOpList (f32, Multiply) ),  // 0x94
    M3OP_F( "f32.div",             -1, f_32,   d_cc(ఖాళీ),                 d_binOpList (f32, Divide) ),               // 0x95
    M3OP_F( "f32.min",             -1, f_32,   d_cc(ఖాళీ),                 d_commutativeBinOpList (f32, Min) ),       // 0x96
    M3OP_F( "f32.max",             -1, f_32,   d_cc(ఖాళీ),                 d_commutativeBinOpList (f32, Max) ),       // 0x97
    M3OP_F( "f32.copysign",        -1, f_32,   d_cc(ఖాళీ),                 d_binOpList (f32, CopySign) ),             // 0x98

    M3OP_F( "f64.abs",             0,  f_64,   d_cc(ఖాళీ),                 d_unaryOpList(f64, Abs) ),                 // 0x99
    M3OP_F( "f64.neg",             0,  f_64,   d_cc(ఖాళీ),                 d_unaryOpList(f64, Negate) ),              // 0x9a
    M3OP_F( "f64.ceil",            0,  f_64,   d_cc(ఖాళీ),                 d_unaryOpList(f64, Ceil) ),                // 0x9b
    M3OP_F( "f64.floor",           0,  f_64,   d_cc(ఖాళీ),                 d_unaryOpList(f64, Floor) ),               // 0x9c
    M3OP_F( "f64.trunc",           0,  f_64,   d_cc(ఖాళీ),                 d_unaryOpList(f64, Trunc) ),               // 0x9d
    M3OP_F( "f64.nearest",         0,  f_64,   d_cc(ఖాళీ),                 d_unaryOpList(f64, సమీప) ),             // 0x9e
    M3OP_F( "f64.sqrt",            0,  f_64,   d_cc(ఖాళీ),                 d_unaryOpList(f64, Sqrt) ),                // 0x9f

    M3OP_F( "f64.add",             -1, f_64,   d_cc(ఖాళీ),                 d_commutativeBinOpList (f64, కూడు) ),       // 0xa0
    M3OP_F( "f64.sub",             -1, f_64,   d_cc(ఖాళీ),                 d_binOpList (f64, వ్యవకలనం) ),             // 0xa1
    M3OP_F( "f64.mul",             -1, f_64,   d_cc(ఖాళీ),                 d_commutativeBinOpList (f64, Multiply) ),  // 0xa2
    M3OP_F( "f64.div",             -1, f_64,   d_cc(ఖాళీ),                 d_binOpList (f64, Divide) ),               // 0xa3
    M3OP_F( "f64.min",             -1, f_64,   d_cc(ఖాళీ),                 d_commutativeBinOpList (f64, Min) ),       // 0xa4
    M3OP_F( "f64.max",             -1, f_64,   d_cc(ఖాళీ),                 d_commutativeBinOpList (f64, Max) ),       // 0xa5
    M3OP_F( "f64.copysign",        -1, f_64,   d_cc(ఖాళీ),                 d_binOpList (f64, CopySign) ),             // 0xa6

    M3OP( "i32.wrap/i64",          0,  i_32,   d_cc(ఖాళీ),                 d_unaryOpList (i32, Wrap_i64) ),           // 0xa7
    M3OP_F( "i32.trunc_s/f32",     0,  i_32,   d_cc(Compile_Convert),      d_convertOpList (i32_Trunc_f32) ),         // 0xa8
    M3OP_F( "i32.trunc_u/f32",     0,  i_32,   d_cc(Compile_Convert),      d_convertOpList (u32_Trunc_f32) ),         // 0xa9
    M3OP_F( "i32.trunc_s/f64",     0,  i_32,   d_cc(Compile_Convert),      d_convertOpList (i32_Trunc_f64) ),         // 0xaa
    M3OP_F( "i32.trunc_u/f64",     0,  i_32,   d_cc(Compile_Convert),      d_convertOpList (u32_Trunc_f64) ),         // 0xab

    M3OP( "i64.extend_s/i32",      0,  i_64,   d_cc(ఖాళీ),                 d_unaryOpList (i64, Extend_i32) ),         // 0xac
    M3OP( "i64.extend_u/i32",      0,  i_64,   d_cc(ఖాళీ),                 d_unaryOpList (i64, Extend_u32) ),         // 0xad

    M3OP_F( "i64.trunc_s/f32",     0,  i_64,   d_cc(Compile_Convert),      d_convertOpList (i64_Trunc_f32) ),         // 0xae
    M3OP_F( "i64.trunc_u/f32",     0,  i_64,   d_cc(Compile_Convert),      d_convertOpList (u64_Trunc_f32) ),         // 0xaf
    M3OP_F( "i64.trunc_s/f64",     0,  i_64,   d_cc(Compile_Convert),      d_convertOpList (i64_Trunc_f64) ),         // 0xb0
    M3OP_F( "i64.trunc_u/f64",     0,  i_64,   d_cc(Compile_Convert),      d_convertOpList (u64_Trunc_f64) ),         // 0xb1

    M3OP_F( "f32.convert_s/i32",   0,  f_32,   d_cc(Compile_Convert),      d_convertOpList (f32_Convert_i32) ),       // 0xb2
    M3OP_F( "f32.convert_u/i32",   0,  f_32,   d_cc(Compile_Convert),      d_convertOpList (f32_Convert_u32) ),       // 0xb3
    M3OP_F( "f32.convert_s/i64",   0,  f_32,   d_cc(Compile_Convert),      d_convertOpList (f32_Convert_i64) ),       // 0xb4
    M3OP_F( "f32.convert_u/i64",   0,  f_32,   d_cc(Compile_Convert),      d_convertOpList (f32_Convert_u64) ),       // 0xb5

    M3OP_F( "f32.demote/f64",      0,  f_32,   d_cc(ఖాళీ),                 d_unaryOpList (f32, Demote_f64) ),         // 0xb6

    M3OP_F( "f64.convert_s/i32",   0,  f_64,   d_cc(Compile_Convert),      d_convertOpList (f64_Convert_i32) ),       // 0xb7
    M3OP_F( "f64.convert_u/i32",   0,  f_64,   d_cc(Compile_Convert),      d_convertOpList (f64_Convert_u32) ),       // 0xb8
    M3OP_F( "f64.convert_s/i64",   0,  f_64,   d_cc(Compile_Convert),      d_convertOpList (f64_Convert_i64) ),       // 0xb9
    M3OP_F( "f64.convert_u/i64",   0,  f_64,   d_cc(Compile_Convert),      d_convertOpList (f64_Convert_u64) ),       // 0xba

    M3OP_F( "f64.promote/f32",     0,  f_64,   d_cc(ఖాళీ),                 d_unaryOpList (f64, Promote_f32) ),        // 0xbb

    M3OP_F( "i32.reinterpret/f32", 0,  i_32,   d_cc(Compile_Convert),      d_convertOpList (i32_Reinterpret_f32) ),   // 0xbc
    M3OP_F( "i64.reinterpret/f64", 0,  i_64,   d_cc(Compile_Convert),      d_convertOpList (i64_Reinterpret_f64) ),   // 0xbd
    M3OP_F( "f32.reinterpret/i32", 0,  f_32,   d_cc(Compile_Convert),      d_convertOpList (f32_Reinterpret_i32) ),   // 0xbe
    M3OP_F( "f64.reinterpret/i64", 0,  f_64,   d_cc(Compile_Convert),      d_convertOpList (f64_Reinterpret_i64) ),   // 0xbf

    M3OP( "i32.extend8_s",         0,  i_32,   d_cc(ఖాళీ),                 d_unaryOpList (i32, Extend8_s) ),          // 0xc0
    M3OP( "i32.extend16_s",        0,  i_32,   d_cc(ఖాళీ),                 d_unaryOpList (i32, Extend16_s) ),         // 0xc1
    M3OP( "i64.extend8_s",         0,  i_64,   d_cc(ఖాళీ),                 d_unaryOpList (i64, Extend8_s) ),          // 0xc2
    M3OP( "i64.extend16_s",        0,  i_64,   d_cc(ఖాళీ),                 d_unaryOpList (i64, Extend16_s) ),         // 0xc3
    M3OP( "i64.extend32_s",        0,  i_64,   d_cc(ఖాళీ),                 d_unaryOpList (i64, Extend32_s) ),         // 0xc4

// Internal operations, for codepage logging only. They sit past c_waOp_lastCore,
// the last opcode this table covers, so GetOpInfo () can never reach them.
# ifdef DEBUG // for codepage logging. the order doesn't matter:
#నిర్వచించు d_m3DebugOp(OP) M3OP (#OP, 0, none, d_ops (op_##OP))

# ఒకవేళ d_m3HasFloat
#నిర్వచించు d_m3DebugTypedOp(OP) M3OP (#OP, 0, none, d_ops (op_##OP##_i32, op_##OP##_i64, op_##OP##_f32, op_##OP##_f64))
# లేకపోతే
#నిర్వచించు d_m3DebugTypedOp(OP) M3OP (#OP, 0, none, d_ops (op_##OP##_i32, op_##OP##_i64))
# endif

    d_m3DebugOp (Compile),          d_m3DebugOp (నమోదు_3),            d_m3DebugOp (చివర_2),
    d_m3DebugOp (Unsupported),      d_m3DebugOp (CallRawFunction),

    d_m3DebugOp (GetGlobal_s32),    d_m3DebugOp (GetGlobal_s64),    d_m3DebugOp (ContinueLoop),     d_m3DebugOp (ContinueLoopIf),
# ఒకవేళ d_m3_Has_దొంతర_Switching
    d_m3DebugOp (ContinueLoop_Suspendable), d_m3DebugOp (ContinueLoopIf_Suspendable), d_m3DebugOp (నమోదు_Check),
#  ఒకవేళ !d_m3_నమోదు_Keeps_చట్రం
    d_m3DebugOp (నమోదు_Suspendable),
#  endif
# endif

    d_m3DebugOp (Copy_స్థానం_32),      d_m3DebugOp (Preserve_Copy_స్థానం_32), d_m3DebugOp (If_s),          d_m3DebugOp (BranchIfPrologue_s),
    d_m3DebugOp (Copy_స్థానం_64),      d_m3DebugOp (Preserve_Copy_స్థానం_64), d_m3DebugOp (If_r),          d_m3DebugOp (BranchIfPrologue_r),

    d_m3DebugOp (Select_i32_rss),   d_m3DebugOp (Select_i32_srs),   d_m3DebugOp (Select_i32_ssr),   d_m3DebugOp (Select_i32_sss),
    d_m3DebugOp (Select_i64_rss),   d_m3DebugOp (Select_i64_srs),   d_m3DebugOp (Select_i64_ssr),   d_m3DebugOp (Select_i64_sss),

# ఒకవేళ d_m3HasFloat
    d_m3DebugOp (Select_f32_sss),   d_m3DebugOp (Select_f32_srs),   d_m3DebugOp (Select_f32_ssr),
    d_m3DebugOp (Select_f32_rss),   d_m3DebugOp (Select_f32_rrs),   d_m3DebugOp (Select_f32_rsr),

    d_m3DebugOp (Select_f64_sss),   d_m3DebugOp (Select_f64_srs),   d_m3DebugOp (Select_f64_ssr),
    d_m3DebugOp (Select_f64_rss),   d_m3DebugOp (Select_f64_rrs),   d_m3DebugOp (Select_f64_rsr),
# endif

    d_m3DebugOp (MemFill),          d_m3DebugOp (MemCopy),          d_m3DebugOp (MemInit),          d_m3DebugOp (దత్తాంశం_Drop),

# ఒకవేళ d_m3HasGasMetering
    d_m3DebugOp (UseGas),
# endif

# ఒకవేళ d_m3HasRefTypes
    d_m3DebugOp (పట్టిక_Get),         d_m3DebugOp (పట్టిక_సమితి),         d_m3DebugOp (పట్టిక_కొలత),
    d_m3DebugOp (పట్టిక_Grow),        d_m3DebugOp (పట్టిక_Fill),        d_m3DebugOp (పట్టిక_Init),
    d_m3DebugOp (ElemDrop),         d_m3DebugOp (పట్టిక_Copy),
# endif

    d_m3DebugTypedOp (సమితి_Global),   d_m3DebugOp (సమితి_Global_s32),    d_m3DebugOp (సమితి_Global_s64),

    d_m3DebugTypedOp (సమితి_రిజిస్టరు), d_m3DebugTypedOp (సమితి_స్థానం),     d_m3DebugTypedOp (Preserve_సమితి_స్థానం),
# endif

# ifdef DEBUG
    M3OP( "termination", 0, c_m3Type_unknown, d_ఖాళీలు_Op_List ) // for find_operation_info
# endif
};

స్థిరరాశి M3OpInfo c_operationsFC[] =
{
    M3OP_F( "i32.trunc_s:sat/f32", 0, i_32,   d_cc(Compile_Convert),         d_convertOpList (i32_TruncSat_f32) ),  // 0x00
    M3OP_F( "i32.trunc_u:sat/f32", 0, i_32,   d_cc(Compile_Convert),         d_convertOpList (u32_TruncSat_f32) ),  // 0x01
    M3OP_F( "i32.trunc_s:sat/f64", 0, i_32,   d_cc(Compile_Convert),         d_convertOpList (i32_TruncSat_f64) ),  // 0x02
    M3OP_F( "i32.trunc_u:sat/f64", 0, i_32,   d_cc(Compile_Convert),         d_convertOpList (u32_TruncSat_f64) ),  // 0x03
    M3OP_F( "i64.trunc_s:sat/f32", 0, i_64,   d_cc(Compile_Convert),         d_convertOpList (i64_TruncSat_f32) ),  // 0x04
    M3OP_F( "i64.trunc_u:sat/f32", 0, i_64,   d_cc(Compile_Convert),         d_convertOpList (u64_TruncSat_f32) ),  // 0x05
    M3OP_F( "i64.trunc_s:sat/f64", 0, i_64,   d_cc(Compile_Convert),         d_convertOpList (i64_TruncSat_f64) ),  // 0x06
    M3OP_F( "i64.trunc_u:sat/f64", 0, i_64,   d_cc(Compile_Convert),         d_convertOpList (u64_TruncSat_f64) ),  // 0x07

    M3OP( "memory.init",           0, none,   d_cc(Compile_జ్ఞాపకం_Init),     d_ఖాళీలు_Op_List ),                       // 0x08
    M3OP( "data.drop",             0, none,   d_cc(Compile_దత్తాంశం_Drop),       d_ఖాళీలు_Op_List ),                       // 0x09

    M3OP( "memory.copy",           0, none,   d_cc(Compile_జ్ఞాపకం_Copy_Fill), d_ఖాళీలు_Op_List ),                       // 0x0a
    M3OP( "memory.fill",           0, none,   d_cc(Compile_జ్ఞాపకం_Copy_Fill), d_ఖాళీలు_Op_List ),                       // 0x0b

#ఒకవేళ d_m3HasRefTypes
    M3OP( "table.init",            0, none,   d_cc(Compile_పట్టిక_Init),      d_ఖాళీలు_Op_List ),                       // 0x0c
    M3OP( "elem.drop",             0, none,   d_cc(Compile_Elem_Drop),       d_ఖాళీలు_Op_List ),                       // 0x0d
    M3OP( "table.copy",            0, none,   d_cc(Compile_పట్టిక_Copy),      d_ఖాళీలు_Op_List ),                       // 0x0e
#లేకపోతే
    M3OP_RESERVED, M3OP_RESERVED, M3OP_RESERVED,                                                                    // 0x0c...0x0e
#అయితే_ముగింపు

#ఒకవేళ d_m3HasRefTypes
    M3OP( "table.grow",            0, i_32,   d_cc(Compile_పట్టిక_Grow_Fill),  d_ఖాళీలు_Op_List ),                       // 0x0f
    M3OP( "table.size",            1, i_32,   d_cc(Compile_పట్టిక_కొలత),      d_ఖాళీలు_Op_List ),                       // 0x10
    M3OP( "table.fill",            0, none,   d_cc(Compile_పట్టిక_Grow_Fill),  d_ఖాళీలు_Op_List ),                       // 0x11
#లేకపోతే
    M3OP_RESERVED, M3OP_RESERVED, M3OP_RESERVED,                                                                    // 0x0f...0x11
#అయితే_ముగింపు

#ఒకవేళ d_m3HasWideArithmetic
    M3OP_RESERVED,                                                                                                  // 0x12

    M3OP( "i64.add128",            -2, i_64,  d_cc(Compile_WideArithmetic), d_logOp (i64_Add128) ),              // 0x13
    M3OP( "i64.sub128",            -2, i_64,  d_cc(Compile_WideArithmetic), d_logOp (i64_Sub128) ),              // 0x14
    M3OP( "i64.mul_wide_s",        0,  i_64,  d_cc(Compile_WideArithmetic), d_logOp (i64_MultiplyWide) ),        // 0x15
    M3OP( "i64.mul_wide_u",        0,  i_64,  d_cc(Compile_WideArithmetic), d_logOp (u64_MultiplyWide) ),        // 0x16
#అయితే_ముగింపు


# ifdef DEBUG
    M3OP( "termination", 0, c_m3Type_unknown, d_ఖాళీలు_Op_List ) // for find_operation_info
# endif
};

#ఒకవేళ d_m3HasAtomics

// The 0xFE instructions share one entry per kind rather than each having its own: what
// tells them apart, the width of the cell and the operation, is worked out from the
// sub-opcode by Compile_Atomic. GetAtomicInfoIndex () maps a sub-opcode onto this table.
గణన {
    c_opAtomic_notify,
    c_opAtomic_wait32,
    c_opAtomic_wait64,
    c_opAtomic_fence,
    c_op_Atomic_ఎక్కించు,
    c_opAtomic_store,
    c_opAtomic_rmw,
    c_opAtomic_cmpxchg
};

స్థిరరాశి M3OpInfo c_operationsFE[] =
{
    M3OP( "memory.atomic.notify",  -1, i_32,  d_cc(Compile_Atomic), d_logOp (AtomicNotify) ),    // c_opAtomic_notify
    M3OP( "memory.atomic.wait32",  -2, i_32,  d_cc(Compile_Atomic), d_logOp (AtomicWait32) ),    // c_opAtomic_wait32
    M3OP( "memory.atomic.wait64",  -2, i_32,  d_cc(Compile_Atomic), d_logOp (AtomicWait64) ),    // c_opAtomic_wait64
    M3OP( "atomic.fence",           0, none,  d_cc(Compile_Atomic), d_ఖాళీలు_Op_List ),             // c_opAtomic_fence
    M3OP( "atomic.load",            0, any,   d_cc(Compile_Atomic), d_logOp (Atomic_ఎక్కించు) ),      // c_opAtomic_load
    M3OP( "atomic.store",          -2, none,  d_cc(Compile_Atomic), d_logOp (AtomicStore) ),     // c_opAtomic_store
    M3OP( "atomic.rmw",            -1, any,   d_cc(Compile_Atomic), d_logOp (AtomicRmw) ),       // c_opAtomic_rmw
    M3OP( "atomic.rmw.cmpxchg",    -2, any,   d_cc(Compile_Atomic), d_logOp (AtomicCmpxchg) ),   // c_opAtomic_cmpxchg
};

స్థిరరాశి u32 c_అంకె_Operations_FE = M3_లెక్క_యొక్క(c_operationsFE);

// The entry a 0xFE sub-opcode uses, or -1 for one that names nothing: the spec leaves
// 0x04..0x0f and everything past cmpxchg unassigned. The accesses come in runs of seven,
// one per pairing of value type and cell width.
స్థిర అంతఃవరుస
i32 Get_Atomic_Info_సూచీ (u32 i_sub)
{
    ఒకవేళ (i_sub <= 0x03) తిరిగి (i32)i_sub;
    ఒకవేళ (i_sub < 0x10) తిరిగి -1;
    ఒకవేళ (i_sub <= 0x16) తిరిగి c_op_Atomic_ఎక్కించు;
    ఒకవేళ (i_sub <= 0x1d) తిరిగి c_opAtomic_store;
    ఒకవేళ (i_sub <= 0x47) తిరిగి c_opAtomic_rmw;
    ఒకవేళ (i_sub <= c_wa_Op_చివరి_Atomic) తిరిగి c_opAtomic_cmpxchg;
    తిరిగి -1;
}

#అయితే_ముగింపు // d_m3HasAtomics

// The opcodes above c_waOp_lastCore are sparse: the reference instructions at
// 0xd0..0xd2 and 0xd4, and the 0xfc extended-opcode prefix. Indexing c_operations out to
// 0xfc to reach them would leave ~50 empty entries there, so they get their own
// table, which GetHighOpIndex () maps the opcode onto.
గణన {
#ఒకవేళ d_m3HasRefTypes
    c_op_ఎగువ_ref_Null,
    c_op_ఎగువ_ref_Is_Null,
    c_op_ఎగువ_ref_Func,
#  ఒకవేళ d_m3HasTypedRefs
    c_op_ఎగువ_ref_As_Non_Null,
#  endif
#అయితే_ముగింపు
#ఒకవేళ d_m3_Has_దొంతర_Switching
    c_op_ఎగువ_cont_New,
    c_op_ఎగువ_cont_Bind,
    c_op_ఎగువ_suspend,
    c_op_ఎగువ_resume,
    c_op_ఎగువ_resume_Throw,
    c_op_ఎగువ_resume_Throw_Ref,
    c_op_ఎగువ_switch,
#అయితే_ముగింపు
#ఒకవేళ d_m3HasAtomics
    c_op_ఎగువ_atomic,
#అయితే_ముగింపు
    c_op_ఎగువ_extended   // always present, so the enum is never empty
};

// Positional, so the table must stay in c_opHigh_* order: array designators
// are not valid C++, which the maintenance builds compile this as.
స్థిర స్థిరరాశి M3OpInfo c_operations_ఎగువ[] =
{
#ఒకవేళ d_m3HasRefTypes
    M3OP( "ref.null",        1, any,               d_cc(Compile_Ref_Null),       d_ఖాళీలు_Op_List ),  // 0xd0  c_opHigh_refNull
    M3OP( "ref.is_null",     0, i_32,              d_cc(Compile_Ref_IsNull),     d_ఖాళీలు_Op_List ),  // 0xd1  c_opHigh_refIsNull
    M3OP( "ref.func",        1, any,               d_cc(Compile_Ref_Func),       d_ఖాళీలు_Op_List ),  // 0xd2  c_opHigh_refFunc
#  ఒకవేళ d_m3HasTypedRefs
    M3OP( "ref.as_non_null", 0, any,               d_cc(Compile_Ref_AsNonNull),  d_ఖాళీలు_Op_List ),  // 0xd4  c_opHigh_refAsNonNull
#  endif
#అయితే_ముగింపు
#ఒకవేళ d_m3_Has_దొంతర_Switching
    M3OP( "cont.new",          1, any,             d_cc(Compile_ContNew),        d_ఖాళీలు_Op_List ),  // 0xe0  c_opHigh_contNew
    M3OP( "cont.bind",         2, any,             d_cc(Compile_ContBind),       d_ఖాళీలు_Op_List ),  // 0xe1  c_opHigh_contBind
    M3OP( "suspend",           1, any,             d_cc(Compile_Suspend),        d_ఖాళీలు_Op_List ),  // 0xe2  c_opHigh_suspend
    M3OP( "resume",            1, any,             d_cc(Compile_Resume),         d_ఖాళీలు_Op_List ),  // 0xe3  c_opHigh_resume
    M3OP( "resume_throw",      2, any,             d_cc(Compile_Resume)   ,    d_ఖాళీలు_Op_List ),  // 0xe4  c_opHigh_resumeThrow
    M3OP( "resume_throw_ref",  1, any,             d_cc(Compile_Resume)      , d_ఖాళీలు_Op_List ),  // 0xe5  c_opHigh_resumeThrowRef
    M3OP( "switch",            2, any,             d_cc(Compile_Switch),         d_ఖాళీలు_Op_List ),  // 0xe6  c_opHigh_switch
#అయితే_ముగింపు

#ఒకవేళ d_m3HasAtomics
    M3OP( "0xFE",            0, c_m3Type_unknown,  d_cc(Compile_ExtendedOpcode), d_ఖాళీలు_Op_List ),  // 0xfe  c_opHigh_atomic
#అయితే_ముగింపు
    M3OP( "0xFC",            0, c_m3Type_unknown,  d_cc(Compile_ExtendedOpcode), d_ఖాళీలు_Op_List ),  // 0xfc  c_opHigh_extended
};
M3_STATIC_ASSERT (M3_లెక్క_యొక్క (c_operations_ఎగువ) == c_op_ఎగువ_extended + 1, c_operations_ఎగువ_needs_one_నమోదు_చొప్పున_c_op_ఎగువ);

// clang-format on


// Opcodes the spec reserves leave zeroed holes in the tables above: no compiler
// and no operations. Every implemented op has at least one of the two.
స్థిర అంతఃవరుస
bool IsImplementedOp (IM3OpInfo i_info)
{
    తిరిగి (i_info->compiler != c_cc_NULL or i_info->operations[0] != ఖాళీ);
}

స్థిరరాశి u32 c_అంకె_Operations   = M3_లెక్క_యొక్క(c_operations);
స్థిరరాశి u32 c_అంకె_Operations_FC = M3_లెక్క_యొక్క(c_operationsFC);

// Maps one of the sparse opcodes above c_waOp_lastCore onto c_operationsHigh,
// or -1 for anything else in that range.
స్థిర అంతఃవరుస
i32 Get_ఎగువ_Op_సూచీ (m3opcode_t opcode)
{
    ఎంపిక (opcode) {
#ఒకవేళ d_m3HasRefTypes
    స్థితి c_waOp_refNull: తిరిగి c_op_ఎగువ_ref_Null;
    స్థితి c_waOp_refIsNull: తిరిగి c_op_ఎగువ_ref_Is_Null;
    స్థితి c_waOp_refFunc: తిరిగి c_op_ఎగువ_ref_Func;
#  ఒకవేళ d_m3HasTypedRefs
    స్థితి c_waOp_refAsNonNull: తిరిగి c_op_ఎగువ_ref_As_Non_Null;
#  endif
#అయితే_ముగింపు
#ఒకవేళ d_m3_Has_దొంతర_Switching
    స్థితి c_waOp_contNew: తిరిగి c_op_ఎగువ_cont_New;
    స్థితి c_waOp_contBind: తిరిగి c_op_ఎగువ_cont_Bind;
    స్థితి c_waOp_suspend: తిరిగి c_op_ఎగువ_suspend;
    స్థితి c_waOp_resume: తిరిగి c_op_ఎగువ_resume;
    స్థితి c_waOp_resumeThrow: తిరిగి c_op_ఎగువ_resume_Throw;
    స్థితి c_waOp_resumeThrowRef: తిరిగి c_op_ఎగువ_resume_Throw_Ref;
    స్థితి c_waOp_switch: తిరిగి c_op_ఎగువ_switch;
#అయితే_ముగింపు
    స్థితి c_waOp_extended: తిరిగి c_op_ఎగువ_extended;
#ఒకవేళ d_m3HasAtomics
    స్థితి c_waOp_atomic: తిరిగి c_op_ఎగువ_atomic;
#అయితే_ముగింపు
    అప్రమేయ: తిరిగి -1;
    }
}

IM3OpInfo GetOpInfo (m3opcode_t opcode)
{
    IM3OpInfo info = ఖాళీ;

    ఎంపిక (opcode >> 8) {
    స్థితి 0x00:
        // c_operations is indexed by opcode only up to c_waOp_lastCore; past that
        // it holds internal operations (DEBUG builds), unreachable by opcode.
        ఒకవేళ (M3_LIKELY(opcode <= c_wa_Op_చివరి_Core)) {
            info = &c_operations[opcode];
        } లేకపోతే {
            i32 ఎగువ = Get_ఎగువ_Op_సూచీ(opcode);
            ఒకవేళ (ఎగువ >= 0) {
                info = &c_operations_ఎగువ[ఎగువ];
            }
        }
        ఆపు;
    స్థితి c_waOp_extended:
        opcode &= 0xFF;
        ఒకవేళ (M3_LIKELY(opcode <= c_wa_Op_చివరి_Extended)) {
            info = &c_operationsFC[opcode];
        }
        ఆపు;
#ఒకవేళ d_m3HasAtomics
    స్థితి c_waOp_atomic: {
        i32 index = Get_Atomic_Info_సూచీ(opcode & 0xFF);
        ఒకవేళ (index >= 0) {
            info = &c_operationsFE[index];
        }
    } ఆపు;
#అయితే_ముగింపు
    }

    తిరిగి (info and IsImplementedOp(info)) ? info : ఖాళీ;
}

#ఒకవేళ d_m3HasGasMetering

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
గణన {
    c_gasLocal   = 1,          // charged once per declared local, at function entry
    c_gasNominal = 1,          // nop, block, loop, if, const
    c_gasArith   = 45,
    c_gas_జరుపు   = 67,
    c_gasBranch  = 90,
    c_gas_Mem_కొలత = 100,
    c_gasAccess  = 120,        // a local, global, table or memory access
    c_gasHeavy   = 10000,      // an indirect call, or anything that works over a whole region
    c_gasDivide  = 36000
};

// The most one op_UseGas can carry. A segment that reaches it is split into
// consecutive charges, which prepays exactly the same amount.
#  define d_m3MaxSegmentGas   0xFFFFFFFF

స్థిర
u32 Get_Gas_ఖర్చు (m3opcode_t i_opcode)
{
    ఎంపిక (i_opcode) {
    స్థితి 0x00:                        // unreachable
    స్థితి 0x01:                        // nop
    స్థితి c_wa_Op_ఖండం:
    స్థితి c_waOp_loop:
    స్థితి c_waOp_if:
    స్థితి c_wa_Op_try_పట్టిక:             // a block that also installs handlers
    స్థితి c_waOp_refNull:              // a constant, by another name
    స్థితి c_waOp_refFunc:
        తిరిగి c_gasNominal;

    స్థితి c_wa_Op_చివర:
        తిరిగి 0;                     // the one control instruction the table prices at nothing

    స్థితి c_waOp_else:
    స్థితి c_waOp_branch:
    స్థితి c_waOp_branchIf:
    స్థితి c_waOp_return:
    స్థితి c_waOp_call:
    స్థితి c_waOp_returnCall:
    స్థితి c_waOp_throw:
    స్థితి c_waOp_throwRef:
        తిరిగి c_gasBranch;

    స్థితి c_wa_Op_branch_పట్టిక:
    స్థితి 0x1a:                        // drop
    స్థితి 0x1b:                        // select
    స్థితి c_waOp_selectTyped:
        తిరిగి c_gasAccess;

    స్థితి c_wa_Op_call_పరోక్ష:
    స్థితి c_wa_Op_return_Call_పరోక్ష:
    స్థితి c_waOp_callRef:
    స్థితి c_waOp_returnCallRef:
    స్థితి c_wa_Op_జ్ఞాపకం_Init:
    స్థితి c_wa_Op_జ్ఞాపకం_Copy:
    స్థితి c_wa_Op_జ్ఞాపకం_Fill:
    స్థితి c_wa_Op_పట్టిక_Init:
    స్థితి c_wa_Op_పట్టిక_Copy:
    స్థితి c_wa_Op_పట్టిక_Grow:
    స్థితి c_wa_Op_పట్టిక_Fill:
    స్థితి 0x40:                        // memory.grow
        తిరిగి c_gasHeavy;

    స్థితి c_wa_Op_దత్తాంశం_Drop:
    స్థితి c_waOp_elemDrop:
    స్థితి c_wa_Op_పట్టిక_కొలత:
    స్థితి 0x3f:                        // memory.size
        తిరిగి c_gas_Mem_కొలత;

    స్థితి c_waOp_refIsNull:
    స్థితి c_waOp_refAsNonNull:
        తిరిగి c_gasArith;

    అప్రమేయ:
        ఆపు;
    }

    // The two integer arithmetic runs are priced by position, i32 at 0x6a and
    // i64 at 0x7c, the table giving both the same shape
    ఒకవేళ ((i_opcode >= 0x6a and i_opcode <= 0x78) or (i_opcode >= 0x7c and i_opcode <= 0x8a)) {
        // clang-format off
        స్థిర స్థిరరాశి u16 c_arithmeticCosts [] = {
            c_gasArith,  c_gasArith,  c_gasArith,                   // add, sub, mul
            c_gasDivide, c_gasDivide, c_gasDivide, c_gasDivide,     // div_s, div_u, rem_s, rem_u
            c_gasArith,  c_gasArith,  c_gasArith,                   // and, or, xor
            c_gas_జరుపు,  c_gas_జరుపు,  c_gas_జరుపు,                   // shl, shr_s, shr_u
            c_gasBranch, c_gasBranch                                // rotl, rotr
        };
        // clang-format on

        తిరిగి c_arithmeticCosts[i_opcode - (i_opcode <= 0x78 ? 0x6a : 0x7c)];
    }

    // the atomic accesses are memory accesses; what waits, wakes or orders is worth
    // as much as an operation over a whole region
    ఒకవేళ (i_opcode >= (c_waOp_atomic << 8) and i_opcode <= ((c_waOp_atomic << 8) | c_wa_Op_చివరి_Atomic)) {
        తిరిగి (i_opcode <= c_waOp_atomicFence) ? c_gasHeavy : c_gasAccess;
    }
    ఒకవేళ (i_opcode >= 0x20 and i_opcode <= 0x3e) {
        తిరిగి c_gasAccess;           // locals, globals, table accesses, loads and stores
    }
    ఒకవేళ (i_opcode >= 0x41 and i_opcode <= 0x44) {
        తిరిగి c_gasNominal;          // i32/i64/f32/f64.const
    }
    ఒకవేళ (i_opcode >= 0x8b and i_opcode <= 0xa6) {
        // the float run, priced as integer arithmetic except where it divides
        bool divides = (i_opcode == 0x91 or i_opcode == 0x95 or     // f32.sqrt, f32.div
                        i_opcode == 0x9f or i_opcode == 0xa3);      // f64.sqrt, f64.div

        తిరిగి divides ? c_gasDivide : c_gasArith;
    }

    // comparisons and clz/ctz/popcnt (0x45..0x69, 0x79..0x7b), the conversions
    // and sign extensions (0xa7..0xc4), the saturating truncations, the wide
    // arithmetic, and anything else that reaches here: plain arithmetic
    తిరిగి c_gasArith;
}

// The instructions ewasm's design segments a body at. A segment runs up to and
// including the next one of these, so every cycle a body can execute - a loop's
// back edge, a branch out of a block, falling off the end of a function - passes
// through at least one, and no path can run unmetered however it is entered.
స్థిర
bool Is_Gas_Segment_చివర (m3opcode_t i_opcode)
{
    ఎంపిక (i_opcode) {
    స్థితి c_waOp_loop:
    స్థితి c_waOp_if:
    స్థితి c_waOp_else:
    స్థితి c_wa_Op_చివర:
    స్థితి c_waOp_branch:
    స్థితి c_waOp_branchIf:
    స్థితి c_wa_Op_branch_పట్టిక:
    స్థితి c_waOp_return:
    స్థితి c_waOp_returnCall:
    స్థితి c_wa_Op_return_Call_పరోక్ష:
    స్థితి c_waOp_returnCallRef:
        తిరిగి true;
    అప్రమేయ:
        తిరిగి false;
    }
}

// Ends the open segment by writing what it accumulated into the immediate its
// op_UseGas is still waiting on. Emitting onto a later code page does not move
// an earlier one, so the reservation is still where it was left.
స్థిర
శూన్య CloseGasSegment (IM3Compilation o)
{
    ఒకవేళ (o->isGasSegmentOpen) {
        ఒకవేళ (o->gasPatch) {
            u32 ఖర్చు = o->gas_ఖర్చు;
            జ్ఞాపకం_నకలు(o->gasPatch, &ఖర్చు, పరిమాణం(ఖర్చు));

            o->gasPatch = ఖాళీ;
        }
        o->gas_ఖర్చు          = 0;
        o->isGasSegmentOpen = false;
    }
}

// Whether this compilation emits the charges
స్థిర అంతఃవరుస
bool IsMetering (IM3Compilation o)
{
    తిరిగి o->runtime->gas_హద్దు != 0;
}

// Charges one instruction to the segment being compiled, opening a segment at
// this point in the code stream if none is open, and settling the segment when
// the instruction is one that ends it. Called before the instruction is
// compiled, which is what both halves need: the op_UseGas lands ahead of what it
// pays for, and the cost is final before a block-structured instruction - loop,
// if, else - compiles its body from inside its own compiler. The body then opens
// a segment of its own, inside the loop or the arm, where it is charged on every
// pass rather than once on the way in.
స్థిర
M3_ఫలితం MeterOpcode (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_ఫలితం ఫలితం = m3Err_none;

    // constant expressions are not metered: they run once at instantiation,
    // before the module is anything a gas budget was handed out for
    ఒకవేళ (o->function and o->పుట and IsMetering(o)) {
        // only the 0xFC or 0xFE prefix has been read so far; the instruction it names is
        // the LEB128 u32 the compiler is about to take. Peek it without consuming.
        // Anything the tables do not cover leaves the prefix priced on its own,
        // which is moot - the compile fails on it a moment later.
        ఒకవేళ (i_opcode == c_waOp_extended or i_opcode == c_waOp_atomic) {
            బైట్లు_t peek = o->wasm;
            u32     sub;
            u32     చివరి = (i_opcode == c_waOp_atomic) ? c_wa_Op_చివరి_Atomic : c_wa_Op_చివరి_Extended;

            ఒకవేళ (not ReadLEB_u32(&sub, &peek, o->wasm_చివర) and sub <= చివరి) {
                i_opcode = (m3opcode_t)((i_opcode << 8) | sub);
            }
        }

        u32 ఖర్చు = Get_Gas_ఖర్చు(i_opcode);

        ఒకవేళ (o->isGasSegmentOpen and o->gas_ఖర్చు > d_m3MaxSegmentGas - ఖర్చు) {
            CloseGasSegment(o);
        }

        ఒకవేళ (not o->isGasSegmentOpen) {
            o->isGasSegmentOpen = true;

_           (EmitOp(o, op_UseGas));
            o->gasPatch = (శూన్య*)Get_పుట_PC(o->పుట);
            EmitConstant32(o, 0);
        }

        o->gas_ఖర్చు += ఖర్చు;

        ఒకవేళ (Is_Gas_Segment_చివర(i_opcode)) {
            CloseGasSegment(o);
        }
    }

    _catch: తిరిగి ఫలితం;
}

#అయితే_ముగింపు // d_m3HasGasMetering


M3_ఫలితం Compile_ఖండం_Statements (IM3Compilation o)
{
    M3_ఫలితం ఫలితం   = m3Err_none;
    bool     valid_చివర = false;

    అయితే (o->wasm < o->wasm_చివర) {
#ఒకవేళ d_m3EnableOpTracing
        ఒకవేళ (o->అంకె_Emits) {
            EmitOp(o, op_Dump_దొంతర);
            EmitConstant32(o, o->అంకె_Opcodes);
            EmitConstant32(o, Get_Max_వాడిన_స్థానం_Plus_One(o));
            Emit_సూచిక(o, o->function);

            o->అంకె_Emits = 0;
        }
#అయితే_ముగింపు

        m3opcode_t opcode;
        o->చివరి_Opcode_ప్రారంభం = o->wasm;
_       (Read_opcode(&opcode, &o->wasm, o->wasm_చివర));
        log_opcode(o, opcode);

        // Restrict opcodes when evaluating expressions
        ఒకవేళ (M3_UNLIKELY(not o->function)) {
            // clang-format off
            ఎంపిక (opcode) {
            స్థితి c_waOp_i32_const: స్థితి c_waOp_i64_const:
            స్థితి c_waOp_f32_const: స్థితి c_waOp_f64_const:
            స్థితి c_waOp_getGlobal: స్థితి c_wa_Op_చివర:
#ఒకవేళ d_m3HasRefTypes
            స్థితి c_waOp_refNull:   స్థితి c_waOp_refFunc:
#అయితే_ముగింపు
#ఒకవేళ d_m3HasExtendedConst
            స్థితి c_wa_Op_i32_కూడు:   స్థితి c_waOp_i32_sub:   స్థితి c_waOp_i32_mul:
            స్థితి c_wa_Op_i64_కూడు:   స్థితి c_waOp_i64_sub:   స్థితి c_waOp_i64_mul:
#అయితే_ముగింపు
                ఆపు;
            అప్రమేయ:
                _throw(m3Err_restrictedOpcode);
            }
            // clang-format on
        }

        IM3OpInfo opinfo = GetOpInfo(opcode);

        ఒకవేళ (M3_UNLIKELY(opinfo == ఖాళీ)) {
            _throw(ErrorCompile(m3Err_unknownOpcode, o, "opcode '%x' not available", opcode));
        }

#ఒకవేళ d_m3HasGasMetering
_       (MeterOpcode(o, opcode));
#అయితే_ముగింపు

        ఒకవేళ (opinfo->compiler != c_cc_NULL) {
_           ((*GetCompiler(opinfo))(o, opcode))
        } లేకపోతే {
_           (Compile_Operator(o, opcode));
        }

        o->మునుపటి_Opcode = opcode;

        ఒకవేళ (opcode == c_waOp_else) {
            _throwif(m3Err_wasmMalformed, o->ఖండం.opcode != c_waOp_if);
            valid_చివర = true;
            ఆపు;
        } లేకపోతే ఒకవేళ (opcode == c_wa_Op_చివర) {
            valid_చివర = true;
            ఆపు;
        }
    }
    _throwif(m3Err_wasmMalformed, not valid_చివర);

_catch:
    తిరిగి ఫలితం;
}

స్థిర
M3_ఫలితం నెట్టు_ఖండం_Results (IM3Compilation o)
{
    M3_ఫలితం ఫలితం = m3Err_none;

    u16 అంకె_Results = Get_Func_Type_అంకె_Results(o->ఖండం.type);

    కొరకు (u16 i = 0; i < అంకె_Results; ++i) {
        m3type_t type = Get_Func_Type_ఫలితం_Type(o->ఖండం.type, i);

        ఒకవేళ (i == అంకె_Results - 1 and IsFpType(type)) {
_           (నెట్టు_రిజిస్టరు(o, type));
        } లేకపోతే {
_           (నెట్టు_Allocated_స్థానం(o, type));
        }
    }

    _catch: తిరిగి ఫలితం;
}


M3_ఫలితం Compile_ఖండం (IM3Compilation o, IM3FuncType i_ఖండం_Type, m3opcode_t i_ఖండం_Opcode)
{
                                                                                        d_m3Assert (not Is_రిజిస్టరు_Allocated (o, 0));
                                                                                        d_m3Assert (not Is_రిజిస్టరు_Allocated (o, 1));
    InvalidateFold(o);

    M3CompilationScope  outerScope = o->ఖండం;
    M3CompilationScope* ఖండం      = &o->ఖండం;

    ఖండం->outer   = &outerScope;
    ఖండం->pc      = Get_పుట_PC(o->పుట);
    ఖండం->patches = ఖాళీ;
    ఖండం->type    = i_ఖండం_Type;
    ఖండం->opcode  = i_ఖండం_Opcode;
    ఖండం->లోతు++;
    ఖండం->snapshot_ఖండం = 0;

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
#ఒకవేళ d_m3HasSnapshots
    // op_Loop's frame records the body, which is also the loop's identity;
    // op_TryTable's records its clause table, two words a clause back from it
    ఒకవేళ (i_ఖండం_Opcode == c_waOp_loop) {
_       (రికార్డు_ఖండం_ప్రారంభం(o, i_ఖండం_Opcode, ఖండం->pc, 0));
    }
#  ఒకవేళ d_m3HasExceptionHandling
    లేకపోతే ఒకవేళ (i_ఖండం_Opcode == c_wa_Op_try_పట్టిక) {
_       (రికార్డు_ఖండం_ప్రారంభం(o, i_ఖండం_Opcode, ఖండం->pc - 2 * o->అంకె_Try_Clauses, o->అంకె_Try_Clauses));
    }
#  endif
#అయితే_ముగింపు

    // validate and dealloc params ----------------------------

    u16 దొంతర_సూచీ = o->దొంతర_సూచీ;

    u16 అంకె_Params = Get_Func_Type_అంకె_Params(i_ఖండం_Type);

    ఒకవేళ (i_ఖండం_Opcode != c_waOp_else) {
        కొరకు (u16 i = 0; i < అంకె_Params; ++i) {
            m3type_t type = GetFuncTypeParamType(i_ఖండం_Type, అంకె_Params - 1 - i);
_           (తీయి_Type(o, type));
        }
    } లేకపోతే {
        ఒకవేళ (Is_దొంతర_Polymorphic(o) && o->ఖండం.ఖండం_దొంతర_సూచీ + అంకె_Params > o->దొంతర_సూచీ) {
            o->దొంతర_సూచీ = o->ఖండం.ఖండం_దొంతర_సూచీ;
        } లేకపోతే {
            o->దొంతర_సూచీ -= అంకె_Params;
        }
    }

    u16 param_సూచీ        = o->దొంతర_సూచీ;
    ఖండం->exit_దొంతర_సూచీ = param_సూచీ; // consume the params at block exit

    // keep copies of param slots in the stack
    o->దొంతర_సూచీ = దొంతర_సూచీ;

    // find slots for the results ----------------------------
_   (నెట్టు_ఖండం_Results(o));

    దొంతర_సూచీ = o->దొంతర_సూచీ;

    // dealloc but keep record of the result slots in the stack
    u16 అంకె_Results = Get_Func_Type_అంకె_Results(i_ఖండం_Type);
    అయితే (అంకె_Results--) {
        తీయి(o);
    }

    ఖండం->ఖండం_దొంతర_సూచీ = o->దొంతర_సూచీ = దొంతర_సూచీ;

    // push the params back onto the stack -------------------
    కొరకు (u16 i = 0; i < అంకె_Params; ++i) {
        m3type_t type = GetFuncTypeParamType(i_ఖండం_Type, i);

        u16 స్థానం = Get_స్థానం_For_దొంతర_సూచీ(o, param_సూచీ + i);
        నెట్టు(o, type, స్థానం);

        ఒకవేళ (స్థానం >= o->స్థానం_మొదటి_Dynamic_సూచీ && స్థానం != c_స్థానం_వాడని) {
_           (MarkSlotsAllocatedByType(o, స్థానం, type));
        }
    }

    //--------------------------------------------------------

#ఒకవేళ d_m3HasExceptionHandling
    // with the scope pushed and the params back on, a catch label of 0 names
    // this block - which is what the proposal says it should
    ఒకవేళ (i_ఖండం_Opcode == c_wa_Op_try_పట్టిక) {
_       (EmitCatchStubs(o));
    }
#అయితే_ముగింపు

_   (Compile_ఖండం_Statements(o));

_   (Validate_ఖండం_చివర(o));

    ఒకవేళ (o->function)    // skip for expressions
    {
        ఒకవేళ (not Is_దొంతర_Polymorphic(o)) {
_           (Resolve_ఖండం_Results(o, &o->ఖండం, /* isBranch: */ false));
        }

#ఒకవేళ d_m3HasExceptionHandling
        // Falling out of the end of a try region retires its handler. This sits
        // ahead of PatchBranches so branches into the block's exit land past it:
        // a branch out of the try popped its own handler at the branch site.
        ఒకవేళ (i_ఖండం_Opcode == c_wa_Op_try_పట్టిక) {
_           (EmitOp(o, op_తీయి_Handlers));
            EmitConstant32(o, 1);
        }
#అయితే_ముగింపు

_       (Unwind_ఖండం_దొంతర(o))

        ఒకవేళ (not ((i_ఖండం_Opcode == c_waOp_if and అంకె_Results) or o->మునుపటి_Opcode == c_waOp_else)) {
            o->దొంతర_సూచీ = o->ఖండం.exit_దొంతర_సూచీ;
_           (నెట్టు_ఖండం_Results(o));
        }
    }

    PatchBranches(o);

#ఒకవేళ d_m3HasSnapshots
    రికార్డు_ఖండం_చివర(o);

    ఒకవేళ (o->local_Init_లోతు) {
        కొరకు (u32 i = 0; i < o->snapshotMap->అంకె_Locals; ++i) {
            ఒకవేళ (o->local_Init_లోతు[i] >= (u32)o->ఖండం.లోతు) {
                o->local_Init_లోతు[i] = UINT32_MAX;
            }
        }
    }
#అయితే_ముగింపు
    o->ఖండం = outerScope;

} _catch: తిరిగి ఫలితం;
}

స్థిర
M3_ఫలితం CompileLocals (IM3Compilation o)
{
    M3_ఫలితం ఫలితం;

    u32 అంకె_Locals = 0;
    u32 అంకె_Local_ఖండాలు;
_   (ReadLEB_u32(&అంకె_Local_ఖండాలు, &o->wasm, o->wasm_చివర));

    కొరకు (u32 l = 0; l < అంకె_Local_ఖండాలు; ++l) {
        u32      var_లెక్క;
        m3type_t localType;

_       (ReadLEB_u32(&var_లెక్క, &o->wasm, o->wasm_చివర));
_       (Parse_విలువ_Type(o->module, &localType, &o->wasm, o->wasm_చివర));
        అంకె_Locals += var_లెక్క;                                                          m3log (compile, "pushing locals. count: %d; type: %s", var_లెక్క, c_waTypes [పునాది_Type_యొక్క(localType)]);
        అయితే (var_లెక్క--) {
_           (నెట్టు_Allocated_స్థానం(o, localType));
        }
    }

    ఒకవేళ (o->function) {
        o->function->అంకె_Locals = అంకె_Locals;
    }

    _catch: తిరిగి ఫలితం;
}

స్థిర
M3_ఫలితం ReserveConstants (IM3Compilation o)
{
    M3_ఫలితం ఫలితం = m3Err_none;

    // in the interest of speed, this blindly scans the Wasm code looking for any byte
    // that looks like an const opcode.
    u16 అంకె_Constant_Slots = 0;

    బైట్లు_t wa = o->wasm;
    అయితే (wa < o->wasm_చివర) {
        u8  సంకేతం_2     = *wa++;
        u16 కూడు_Slots = 0;

        ఒకవేళ (సంకేతం_2 == c_waOp_i32_const or సంకేతం_2 == c_waOp_f32_const) {
            కూడు_Slots = 1;
        } లేకపోతే ఒకవేళ (సంకేతం_2 == c_waOp_i64_const or సంకేతం_2 == c_waOp_f64_const) {
            కూడు_Slots = Get_Type_అంకె_Slots(c_m3Type_i64);
        }

        ఒకవేళ (అంకె_Constant_Slots + కూడు_Slots >= d_m3_Max_Constant_పట్టిక_కొలత) {
            ఆపు;
        }

        అంకె_Constant_Slots += కూడు_Slots;
    }

    // if constants overflow their reserved stack space, the compiler simply emits op_Const
    // operations as needed. Compiled expressions (global inits) don't pass through this
    // ReserveConstants function and thus always produce inline constants.

    Align_స్థానం_గమ్యం_Type(&అంకె_Constant_Slots, c_m3Type_i64);                                           m3log (compile, "reserved constant slots: %d", అంకె_Constant_Slots);

    o->స్థానం_మొదటి_Dynamic_సూచీ = o->స్థానం_మొదటి_Const_సూచీ + అంకె_Constant_Slots;

    ఒకవేళ (o->స్థానం_మొదటి_Dynamic_సూచీ >= d_m3MaxFunctionSlots) {
        _throw(m3_Err_function_దొంతర_అధికప్రవాహం);
    }

    _catch:
    తిరిగి ఫలితం;
}


// A constant expression compiles like a miniature function root block: no args,
// no locals and a single result, which Compile_End copies down into slot 0 for
// EvaluateExpression to read back.
M3_ఫలితం Compile_వ్యక్తీకరణ (IM3Compilation o, IM3FuncType i_ఫలితం_Type)
{
    M3_ఫలితం ఫలితం = m3Err_none;

    o->ఖండం.type = i_ఫలితం_Type;

    u16 అంకె_Ret_Slots = Get_Func_Type_అంకె_Results(i_ఫలితం_Type) * c_io_స్థానం_లెక్క;

    కొరకు (u16 i = 0; i < అంకె_Ret_Slots; ++i) {
        Mark_స్థానం_Allocated(o, i);
    }

    o->max_దొంతర_Slots = o->స్థానం_Max_Allocated_సూచీ_Plus_One = o->స్థానం_మొదటి_Dynamic_సూచీ = అంకె_Ret_Slots;

_   (Compile_ఖండం_Statements(o));

    _catch: తిరిగి ఫలితం;
}


M3_ఫలితం CompileFunction (IM3Function io_function)
{
    ఒకవేళ (!io_function->wasm) {
        // An import. Either linking pointed it at another module's function -
        // in which case that one carries the body - or a host function was
        // bound to it, or it is simply unsatisfied.
        IM3Function impl = Function_Implementation(io_function);

        ఒకవేళ (impl != io_function) {
            ఒకవేళ (not impl->compiled) {
                M3_ఫలితం r = CompileFunction(impl);
                ఒకవేళ (r) {
                    తిరిగి r;
                }
            }

            // keep the placeholder callable too, for anything still holding it
            io_function->compiled = impl->compiled;
            తిరిగి m3Err_none;
        }

        ఒకవేళ (io_function->compiled) {
            తిరిగి m3Err_none;
        }

        తిరిగి ErrorModule(m3Err_functionImportMissing, io_function->module, "'%s.%s'",
                           GetFunctionImportModuleName(io_function),
                           m3_GetFunctionName(io_function));
    }

#ఒకవేళ d_m3EnableValidation
    ఒకవేళ (not io_function->module->runtime->skipValidation) {
        M3_ఫలితం vr = ValidateFunction(io_function);
        ఒకవేళ (vr) {
            తిరిగి vr;
        }
    }
#అయితే_ముగింపు

    IM3FuncType funcType = io_function->funcType;                   m3log (compile, "compiling: [%d] %s %s; wasm-size: %d",
                                                                        io_function->index, m3_GetFunctionName (io_function), SPrintFuncTypeSignature (funcType), (u32) (io_function->wasm_చివర - io_function->wasm));
    IM3Runtime runtime = io_function->module->runtime;

    IM3Compilation o = &runtime->compilation;                       d_m3Assert (d_m3MaxFunctionSlots >= d_m3_Max_Function_దొంతర_ఎత్తు * (d_m3_Use32_బిట్టు_Slots + 1))  // need twice as many slots in 32-bit mode
    memset(o, 0x0, పరిమాణం(M3Compilation));

    o->runtime    = runtime;
    o->module     = io_function->module;
    o->function   = io_function;
    o->wasm       = io_function->wasm;
    o->wasm_చివర    = io_function->wasm_చివర;
    o->ఖండం.type = funcType;

_try {
    // skip over code size. the end was already calculated during parse phase
    u32 కొలత;
_   (ReadLEB_u32(&కొలత, &o->wasm, o->wasm_చివర));                     d_m3Assert (కొలత == (o->wasm_చివర - o->wasm))

#ఒకవేళ d_m3HasSnapshots
    o->body_ప్రారంభం = o->wasm;
#అయితే_ముగింపు

#ఒకవేళ d_m3HasSnapshots
    // only code that can be suspended can end up in a snapshot
    ఒకవేళ (runtime->isSuspendable) {
        o->snapshotMap = m3_AllocStruct(M3SnapshotMap);
        _throwifnull(o->snapshotMap);
    }
#అయితే_ముగింపు

_   (Acquire_Compilation_సంకేతం_పుట(o, &o->పుట));

    pc_t pc = Get_పుట_PC(o->పుట);

    u16 అంకె_Ret_Slots = Get_Function_అంకె_Returns(o->function) * c_io_స్థానం_లెక్క;

    కొరకు (u16 i = 0; i < అంకె_Ret_Slots; ++i) {
        Mark_స్థానం_Allocated(o, i);
    }

    o->function->అంకె_Ret_Slots = o->స్థానం_మొదటి_Dynamic_సూచీ = అంకె_Ret_Slots;

    u16 అంకె_వాదనలు = Get_Function_అంకె_వాదనలు(o->function);

    // push the arg types to the type stack
    కొరకు (u16 i = 0; i < అంకె_వాదనలు; ++i) {
        m3type_t type = GetFunctionArgType(o->function, i);
_       (నెట్టు_Allocated_స్థానం(o, type));

        // prevent allocator fill-in
        o->స్థానం_మొదటి_Dynamic_సూచీ += c_io_స్థానం_లెక్క;
    }

    o->స్థానం_Max_Allocated_సూచీ_Plus_One = o->function->అంకె_Ret_And_Arg_Slots = o->స్థానం_మొదటి_Local_సూచీ = o->స్థానం_మొదటి_Dynamic_సూచీ;

_   (CompileLocals(o));

#ఒకవేళ d_m3HasSnapshots
_   (రికార్డు_Locals(o));
#అయితే_ముగింపు

#ఒకవేళ d_m3HasGasMetering
    // the table charges for a function's locals as well as for its instructions.
    // op_Entry is what sets them up, so the charge rides on the body's first
    // segment, which is emitted right after it
    o->gas_ఖర్చు = o->function->అంకె_Locals * c_gasLocal;
#అయితే_ముగింపు

    u16 max_స్థానం = Get_Max_వాడిన_స్థానం_Plus_One(o);

    o->function->అంకె_Local_బైట్లు = (max_స్థానం - o->స్థానం_మొదటి_Local_సూచీ) * పరిమాణం(m3slot_t);

    o->స్థానం_మొదటి_Const_సూచీ = o->స్థానం_Max_Const_సూచీ = max_స్థానం;

    // ReserveConstants initializes o->firstDynamicSlotNumber
_   (ReserveConstants(o));

    // start tracking the max stack used (Push() also updates this value) so that op_Entry can precisely detect stack overflow
    o->max_దొంతర_Slots = o->స్థానం_Max_Allocated_సూచీ_Plus_One = o->స్థానం_మొదటి_Dynamic_సూచీ;

    o->ఖండం.ఖండం_దొంతర_సూచీ = o->దొంతర_మొదటి_Dynamic_సూచీ = o->దొంతర_సూచీ;                           m3log (compile, "start stack index: %u; max stack slots: %u",
                                                                                                           (u32) o->దొంతర_మొదటి_Dynamic_సూచీ, (u32) o->max_దొంతర_Slots);
#ఒకవేళ d_m3_Has_దొంతర_Switching
    // A suspendable body opens by checking for a pause request, once its frame
    // is set up. The pause point's offset is the first instruction's, just past
    // the local declarations - the body's end, when it has no other.
    ఒకవేళ (runtime->isSuspendable) {
        o->చివరి_Opcode_ప్రారంభం = o->wasm;
#  ఒకవేళ d_m3_నమోదు_Keeps_చట్రం
_       (EmitOp(o, op_నమోదు));
        Emit_సూచిక(o, io_function);
_       (EmitOp(o, op_నమోదు_Check));
_       (రికార్డు_సురక్షిత_బిందువు(o, safepoint_నమోదు, 1, 0, 0, ఖాళీ));
#  లేకపోతే
_       (EmitOp(o, op_నమోదు_Suspendable));
        Emit_సూచిక(o, io_function);
_       (రికార్డు_సురక్షిత_బిందువు(o, safepoint_నమోదు, 0, 0, 0, ఖాళీ));
#  endif
    } లేకపోతే
#అయితే_ముగింపు
    {
_       (EmitOp(o, op_నమోదు));
        Emit_సూచిక(o, io_function);
    }

_   (Compile_ఖండం_Statements(o));

    _throwif(m3Err_wasmMalformed, o->మునుపటి_Opcode != c_wa_Op_చివర);

    io_function->compiled      = pc;
    io_function->max_దొంతర_Slots = o->max_దొంతర_Slots;

    u16 అంకె_Constant_Slots = o->స్థానం_Max_Const_సూచీ - o->స్థానం_మొదటి_Const_సూచీ;                           m3log (compile, "unique constant slots: %u; unused slots: %u",
                                                                                                           అంకె_Constant_Slots, o->స్థానం_మొదటి_Dynamic_సూచీ - o->స్థానం_Max_Const_సూచీ);
    io_function->అంకె_Constant_బైట్లు = అంకె_Constant_Slots * పరిమాణం(m3slot_t);

    ఒకవేళ (అంకె_Constant_Slots) {
        io_function->constants = m3_CopyMem((cbytes_t)o->constants, io_function->అంకె_Constant_బైట్లు);
        _throwifnull(io_function->constants);
    }

#ఒకవేళ d_m3HasSnapshots
    io_function->snapshotMap = o->snapshotMap;
    o->snapshotMap           = ఖాళీ;
#అయితే_ముగింపు

} _catch:

    Release_Compilation_సంకేతం_పుట(o);

#ఒకవేళ d_m3HasSnapshots
    SnapshotMap_Free(o->snapshotMap);
    o->snapshotMap = ఖాళీ;
    m3_Free(o->local_Init_లోతు);
#అయితే_ముగింపు

    తిరిగి ఫలితం;
}

#ఒకవేళ d_m3_Has_దొంతర_Switching
m3ret_t ResumeContinuation (IM3Runtime i_runtime, IM3Continuation i_cont)
{
    ఒకవేళ (M3_UNLIKELY(!i_cont || i_cont->state != cont_suspended)) {
        తిరిగి m3Err_none;
    }

    M3_జ్ఞాపకం_శీర్షిక* _mem = i_cont->నమోదు_Function ? Module_జ్ఞాపకం_శీర్షిక(i_cont->నమోదు_Function->module) : ఖాళీ;

    i_runtime->చురుకైన_Continuation = i_cont;
    i_cont->state                 = cont_running;

    i32 outermost     = (i32)i_cont->అంకె_చట్రాలు - 1;
    i_cont->అంకె_చట్రాలు = 0;

    // with no frames standing, this lands straight on the suspension point
    m3ret_t r = Replay_చట్రాలు(i_cont, outermost, _mem);

    ఒకవేళ (r == m3Err_continuationSuspended) {
        i_cont->state = cont_suspended;
    } లేకపోతే {
        i_cont->state = cont_returned;
    }
    i_runtime->చురుకైన_Continuation = ఖాళీ;

    తిరిగి r;
}
#అయితే_ముగింపు
