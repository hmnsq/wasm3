//
//  m3_compile.c
//
//  Created by Steven Massey on 4/17/19.
//  Copyright © 2019 Steven Massey. All rights reserved.
//

// Allow using opcodes for compilation process
#परिभाषय M3_COMPILE_OPCODES

#अन्तर्भाव "m3_env.h"
#अन्तर्भाव "m3_compile.h"
#अन्तर्भाव "m3_exec.h"
#अन्तर्भाव "m3_exception.h"
#अन्तर्भाव "m3_info.h"
#अन्तर्भाव "m3_validate.h"

// Resolves M3OpInfo.compiler, an index, to the function it names. Defined down
// with c_compilers [], which has to follow every compiler it points at.
स्थिर
M3Compiler GetCompiler (IM3OpInfo i_opInfo);

//----- EMIT --------------------------------------------------------------------------------------------------------------

स्थिर अन्तःस्थ
pc_t GetPC (IM3Compilation o)
{
    प्रत्यागम GetPagePC(o->page);
}

#यदि d_m3HasSnapshots

#  define d_m3GrowSnapshotArray(TYPE, ARRAY, गणनम्_2, क्षमता_2)                    \
    यदि ((गणनम्_2) == (क्षमता_2)) {                                                 \
        u32   new_क्षमता = (क्षमता_2) ? (क्षमता_2) * 2 : 8;                     \
        TYPE* grown = m3_ReallocArray(TYPE, (ARRAY), new_क्षमता, (क्षमता_2));   \
        यदि (not grown) {                                                         \
            प्रत्यागम m3Err_mallocFailed;                                           \
        }                                                                        \
        (ARRAY)    = grown;                                                      \
        (क्षमता_2) = new_क्षमता;                                                \
    }

// Where the instruction being compiled starts, counted from the start of the
// function's body: the one position every build of every revision agrees on
स्थिर अन्तःस्थ
u32 Get_Wasm_विस्थापनम् (IM3Compilation o)
{
    प्रत्यागम (u32)(o->चरमः_Opcode_Start - o->bodyStart);
}

// Whether the operand stack entry at i_index is a value the frame holds. Every
// block keeps records of its parameter and result slots on the stack, between
// the height it was entered at and where its own values begin (see
// CompileBlock). Those are landing pads rather than values: nothing need have
// written them yet, and the slots they name may be in use for something else.
// The function's own scope has none - that stretch is its arguments and locals.
स्थिर
bool Is_जीवितम्_राशिः_प्रविष्टिः (IM3Compilation o, u16 i_सूचकांक)
{
    प्रति (IM3CompilationScope scope = &o->खण्डम्; scope->outer; scope = scope->outer) {
        यदि (i_सूचकांक >= scope->exit_राशिः_सूचकांक and i_सूचकांक < scope->खण्डम्_राशिः_सूचकांक) {
            प्रत्यागम false;
        }
    }

    प्रत्यागम true;
}

स्थिर
M3_परिणाम सङ्कलय_Safe_Point_मानम् (IM3Compilation o, M3SafePoint* io_point, u16 i_सूचकांक)
{
    M3SnapshotMap* map = o->snapshotMap;

    d_m3GrowSnapshotArray(M3_Slot_मानम्, map->मानानि, map->अङ्कः_मानानि, o->मानानि_क्षमता);

    map->मानानि[map->अङ्कः_मानानि].slot = o->wasm_राशिः[i_सूचकांक];
    map->मानानि[map->अङ्कः_मानानि].type = o->type_राशिः[i_सूचकांक];
    map->अङ्कः_मानानि++;
    io_point->अङ्कः_मानानि++;

    प्रत्यागम m3Err_none;
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
स्थिर
M3_परिणाम लेखः_Safe_Point (IM3Compilation o, M3_Safe_Point_विधा i_विधा, u32 i_शब्दाः_पृष्ठभागः, u16 i_अङ्कः_Results,
                          u16 i_aux, IM3CompilationScope i_loop)
{
_try {
    M3SnapshotMap* map = o->snapshotMap;

    यदि (not map or not o->page) {
        प्रत्यागम m3Err_none;
    }

    d_m3GrowSnapshotArray(M3SafePoint, map->safePoints, map->अङ्कः_Safe_Points, o->safe_Points_क्षमता);

    // grown before it is pointed at: adding values never moves the safepoints
    M3SafePoint* point     = &map->safePoints[map->अङ्कः_Safe_Points];
    point->pc              = GetPC(o) - i_शब्दाः_पृष्ठभागः;
    point->wasm_विस्थापनम्      = Get_Wasm_विस्थापनम्(o);
    point->first_मानम्      = map->अङ्कः_मानानि;
    point->first_Local_मानम् = UINT32_MAX;
    point->अङ्कः_मानानि       = 0;
    point->अङ्कः_Results      = i_अङ्कः_Results;
    point->aux             = i_aux;
    point->resumeType      = c_m3Type_none;
    point->विधा            = (u8)i_विधा;
    point->flags           = 0;

    // Non-defaultable locals can still be uninitialized at a safepoint. Keep
    // their heap types, but allow the null placeholder until local.set makes
    // them definitely initialized. Branches to a loop use its entry context.
    यदि (o->local_Init_गभीरता) {
        point->first_Local_मानम् = map->अङ्कः_मानानि;
        प्रति (u32 i = 0; i < map->अङ्कः_Locals; ++i) {
            d_m3GrowSnapshotArray(M3_Slot_मानम्, map->मानानि, map->अङ्कः_मानानि, o->मानानि_क्षमता);
            M3_Slot_मानम् मानम् = map->locals[i];
            यदि (o->local_Init_गभीरता[i] == UINT32_MAX or
                (i_loop and o->local_Init_गभीरता[i] >= (u32)i_loop->गभीरता)) {
                मानम्.type &= ~d_m3Type_refNonNull;
            }
            map->मानानि[map->अङ्कः_मानानि++] = मानम्;
        }
        point->first_मानम् = map->अङ्कः_मानानि;
    }

    u16 शिखरम् = o->राशिः_सूचकांक;

    यदि (i_विधा != safepoint_suspend) {
        शिखरम् = (शिखरम् > i_अङ्कः_Results) ? (u16)(शिखरम् - i_अङ्कः_Results) : 0;
    }
    यदि (i_loop) {
        शिखरम् = M3_MIN(शिखरम्, i_loop->exit_राशिः_सूचकांक);
    }

    प्रति (u16 i = o->राशिः_First_Dynamic_सूचकांक; i < शिखरम्; ++i) {
        यदि (Is_जीवितम्_राशिः_प्रविष्टिः(o, i)) {
_           (सङ्कलय_Safe_Point_मानम्(o, point, i));
        }
    }

    यदि (i_loop) {
        u16 अङ्कः_Params = Get_Func_Type_अङ्कः_Params(i_loop->type);

        प्रति (u16 i = 0; i < अङ्कः_Params; ++i) {
_           (सङ्कलय_Safe_Point_मानम्(o, point, (u16)(i_loop->exit_राशिः_सूचकांक + i)));
        }
    }

    map->अङ्कः_Safe_Points++;

} _catch: प्रत्यागम परिणाम;
}

// A back edge: op_ContinueLoop, or op_ContinueLoopIf when i_isConditional, and
// the loop it goes back to. The conditional one suspends before it has taken
// the branch, so it has to be resumed with the condition that took it.
स्थिर
M3_परिणाम लेखः_पृष्ठभागः_Edge (IM3Compilation o, IM3CompilationScope i_loop, bool i_isConditional)
{
    M3_परिणाम परिणाम = m3Err_none;

    // the loop is named by its record in the map, which has to fit the field
    _throwif("a function has too many loops for its snapshot map",
             o->snapshotMap and o->page and (i_loop->snapshot_खण्डम् == 0 or i_loop->snapshot_खण्डम् > UINT16_MAX + 1u));

_   (लेखः_Safe_Point(o, safepoint_op, 2, 0, (u16)(i_loop->snapshot_खण्डम् - 1), i_loop));

    यदि (i_isConditional and o->snapshotMap and o->page) {
        o->snapshotMap->safePoints[o->snapshotMap->अङ्कः_Safe_Points - 1].flags |= d_m3_Safe_Point_गृहीतम्_Branch;
    }

    _catch: प्रत्यागम परिणाम;
}

// A call, waiting on its callee with i_numResults not yet written; the
// callee's frame starts i_calleeSlot slots above this one. A return_call that
// is compiled as a call is one too, since its frame stands while the callee
// runs, but it is marked: in Wasm the callee has replaced it, so a snapshot
// walks past it rather than writing it.
स्थिर
M3_परिणाम लेखः_Call (IM3Compilation o, u16 i_अङ्कः_Results, u16 i_calleeSlot, bool i_isReturnCall)
{
    M3_परिणाम परिणाम = लेखः_Safe_Point(o, safepoint_call, 0, i_अङ्कः_Results, i_calleeSlot, रिक्त);

    यदि (not परिणाम and i_isReturnCall and o->snapshotMap and o->page) {
        o->snapshotMap->safePoints[o->snapshotMap->अङ्कः_Safe_Points - 1].flags |= d_m3_Safe_Point_पुच्छ_Call;
    }

    प्रत्यागम परिणाम;
}

// Where the function's arguments and locals are kept. Unlike the operand
// stack, that never changes, so it is recorded once rather than at every
// safepoint.
स्थिर
M3_परिणाम लेखः_Locals (IM3Compilation o)
{
    M3SnapshotMap* map = o->snapshotMap;

    यदि (map and o->राशिः_सूचकांक) {
        map->locals = m3_AllocArray(M3_Slot_मानम्, o->राशिः_सूचकांक);

        यदि (not map->locals) {
            प्रत्यागम m3Err_mallocFailed;
        }

        प्रति (u16 i = 0; i < o->राशिः_सूचकांक; ++i) {
            map->locals[i].slot = o->wasm_राशिः[i];
            map->locals[i].type = o->type_राशिः[i];
        }

        map->अङ्कः_Locals = o->राशिः_सूचकांक;
        प्रति (u32 i = Get_Function_अङ्कः_तर्काः(o->function); i < map->अङ्कः_Locals; ++i) {
            यदि (IsRefType(map->locals[i].type) and not IsNullableRef(map->locals[i].type)) {
                o->local_Init_गभीरता = m3_AllocArray(u32, map->अङ्कः_Locals);
                यदि (not o->local_Init_गभीरता) {
                    प्रत्यागम m3Err_mallocFailed;
                }
                प्रति (u32 j = i; j < map->अङ्कः_Locals; ++j) {
                    यदि (IsRefType(map->locals[j].type) and not IsNullableRef(map->locals[j].type)) {
                        o->local_Init_गभीरता[j] = UINT32_MAX;
                    }
                }
                भङ्ग;
            }
        }
    }

    प्रत्यागम m3Err_none;
}

// A loop or a try_table, whose native frame a snapshot names by the
// instruction's offset. i_pc is what that frame records. The scope being
// compiled keeps the record, so that RecordBlockEnd can close it.
स्थिर
M3_परिणाम लेखः_खण्डम्_Start (IM3Compilation o, m3opcode_t i_opcode, pc_t i_pc, u32 i_अङ्कः_Clauses)
{
    M3SnapshotMap* map = o->snapshotMap;

    यदि (map and o->page) {
        d_m3GrowSnapshotArray(M3_खण्डम्_Start, map->खण्डानि, map->अङ्कः_खण्डानि, o->खण्डानि_क्षमता);

        M3_खण्डम्_Start* खण्डम् = &map->खण्डानि[map->अङ्कः_खण्डानि++];
        खण्डम्->pc           = i_pc;
        खण्डम्->wasm_विस्थापनम्   = Get_Wasm_विस्थापनम्(o);
        खण्डम्->wasm_अन्तः      = UINT32_MAX;
        खण्डम्->अङ्कः_Clauses   = (u16)i_अङ्कः_Clauses;
        खण्डम्->opcode       = (u8)i_opcode;

        o->खण्डम्.snapshot_खण्डम् = map->अङ्कः_खण्डानि;
    }

    प्रत्यागम m3Err_none;
}

// Closes the scope's region, while the instruction just read is its end
स्थिर
शून्य लेखः_खण्डम्_अन्तः (IM3Compilation o)
{
    यदि (o->snapshotMap and o->खण्डम्.snapshot_खण्डम्) {
        o->snapshotMap->खण्डानि[o->खण्डम्.snapshot_खण्डम् - 1].wasm_अन्तः = Get_Wasm_विस्थापनम्(o);
    }
}

#  undef d_m3GrowSnapshotArray

#अन्यथा

स्थिर अन्तःस्थ
M3_परिणाम लेखः_Safe_Point (IM3Compilation o, M3_Safe_Point_विधा i_विधा, u32 i_शब्दाः_पृष्ठभागः, u16 i_अङ्कः_Results,
                          u16 i_aux, IM3CompilationScope i_loop)
{
    (शून्य)o;
    (शून्य)i_विधा;
    (शून्य)i_शब्दाः_पृष्ठभागः;
    (शून्य)i_अङ्कः_Results;
    (शून्य)i_aux;
    (शून्य)i_loop;
    प्रत्यागम m3Err_none;
}

स्थिर अन्तःस्थ
M3_परिणाम लेखः_पृष्ठभागः_Edge (IM3Compilation o, IM3CompilationScope i_loop, bool i_isConditional)
{
    (शून्य)o;
    (शून्य)i_loop;
    (शून्य)i_isConditional;
    प्रत्यागम m3Err_none;
}

स्थिर अन्तःस्थ
M3_परिणाम लेखः_Call (IM3Compilation o, u16 i_अङ्कः_Results, u16 i_calleeSlot, bool i_isReturnCall)
{
    (शून्य)o;
    (शून्य)i_अङ्कः_Results;
    (शून्य)i_calleeSlot;
    (शून्य)i_isReturnCall;
    प्रत्यागम m3Err_none;
}

#यदि_अन्तः // d_m3HasSnapshots

स्थिर M3_NOINLINE
M3_परिणाम Ensure_संहिता_Page_अङ्कः_पङ्क्तयः (IM3Compilation o, u32 i_अङ्कः_पङ्क्तयः)
{
    M3_परिणाम परिणाम = m3Err_none;

    i_अङ्कः_पङ्क्तयः += 2; // room for Bridge

    यदि (M3_UNLIKELY(अङ्कः_Free_पङ्क्तयः(o->page) < i_अङ्कः_पङ्क्तयः)) {
        IM3_संहिता_Page page = Acquire_संहिता_Page_With_क्षमता(o->runtime, i_अङ्कः_पङ्क्तयः);

        यदि (page) {
            m3log(emit, "bridging new code page from: %d %p (free slots: %d) to: %d", o->page->info.sequence, GetPC(o), अङ्कः_Free_पङ्क्तयः(o->page), page->info.sequence);
            d_m3Assert(अङ्कः_Free_पङ्क्तयः(o->page) >= 2);

            EmitWord(o->page, op_Branch);
            EmitWord(o->page, GetPagePC(page));

            Release_संहिता_Page(o->runtime, o->page);

            o->page = page;
        } अन्यथा {
            परिणाम = m3_Err_malloc_Failed_संहिता_Page;
        }
    }

    प्रत्यागम परिणाम;
}

// invalidate the pending local.set fold candidate; called wherever code is emitted or a
// branch target captures the current pc (a fold appends an immediate, moving that position)
स्थिर अन्तःस्थ
शून्य InvalidateFold (IM3Compilation o)
{
#यदि d_m3_Fold_समुच्चयः_Local
    o->foldPatchPC = रिक्त;
#अन्यथा
    (शून्य)o;
#यदि_अन्तः
}

स्थिर M3_NOINLINE
M3_परिणाम EmitOp (IM3Compilation o, IM3Operation i_operation)
{
    M3_परिणाम परिणाम = m3Err_none;                                 d_m3Assert (i_operation or Is_राशिः_Polymorphic (o));

    InvalidateFold(o);

    // it's OK for page to be null; when compile-walking the bytecode without emitting
    यदि (o->page) {
#यदि d_m3EnableOpTracing
        यदि (i_operation != op_Dump_राशिः) {
            o->अङ्कः_Emits++;
        }
#यदि_अन्तः

        // have execution jump to a new page if slots are critically low
        परिणाम = Ensure_संहिता_Page_अङ्कः_पङ्क्तयः(o, d_m3_संहिता_Page_Free_पङ्क्तयः_देहली);

        यदि (M3_LIKELY(not परिणाम)) {
            यदि (d_m3LogEmit) {
                log_emit(o, i_operation);
            }
#यदि d_m3_लेखः_Backtraces
            Emit_Mapping_प्रविष्टिः(o->page, (u32)(o->चरमः_Opcode_Start - o->module->wasmStart));
#यदि_अन्तः // d_m3RecordBacktraces
            EmitWord(o->page, i_operation);
        }
    }

    प्रत्यागम परिणाम;
}

// Push an immediate constant into the M3 codestream
स्थिर M3_NOINLINE
शून्य EmitConstant32 (IM3Compilation o, ध्रुव u32 i_तात्कालिकम्)
{
    यदि (o->page) {                                                          m3log (emit, "const32: %ud", i_तात्कालिकम्);
        EmitWord32(o->page, i_तात्कालिकम्);
    }
}

#यदि d_m3HasMemory64
// Takes two lines of the code page where a pointer is 32 bits, so whatever
// reads it back has to step _pc by the same amount - see op_Const64.
स्थिर M3_NOINLINE
शून्य EmitConstant64 (IM3Compilation o, ध्रुव u64 i_तात्कालिकम्)
{
    यदि (o->page) {
        EmitWord64(o->page, i_तात्कालिकम्);
    }
}
#यदि_अन्तः

स्थिर M3_NOINLINE
शून्य Emit_Slot_विस्थापनम् (IM3Compilation o, ध्रुव i32 i_विस्थापनम्)
{
    यदि (o->page) {                                                          m3log (emit, "slot: [%d]", i_विस्थापनम्);
        EmitWord32(o->page, i_विस्थापनम्);
    }
}

स्थिर M3_NOINLINE
pc_t EmitPointer (IM3Compilation o, ध्रुव शून्य* ध्रुव i_pointer)
{
    pc_t ptr = GetPagePC(o->page);

    यदि (o->page) {                                                          m3log (emit, "ptr: %p", i_pointer);
        EmitWord(o->page, i_pointer);
    }

    प्रत्यागम ptr;
}

स्थिर M3_NOINLINE
शून्य* ReservePointer (IM3Compilation o)
{
    pc_t ptr = GetPagePC(o->page);
    EmitPointer(o, रिक्त);
    प्रत्यागम (शून्य*)ptr;
}


//-------------------------------------------------------------------------------------------------------------------------

#परिभाषय d_indent "     | %s"

// just want less letters and numbers to stare at down the way in the compiler table
#परिभाषय i_32    c_m3Type_i32
#परिभाषय i_64    c_m3Type_i64
#परिभाषय f_32    c_m3Type_f32
#परिभाषय f_64    c_m3Type_f64
#परिभाषय none    c_m3Type_none
#परिभाषय any     (u8)-1

#यदि d_m3HasFloat
#  define FPOP(x) x
#अन्यथा
#  define FPOP(x) रिक्त
#यदि_अन्तः

// These are indexed by M3ValueType, so every type up to c_m3Type_externref needs
// an entry. A reference is one pointer-sized word, so it moves with the integer
// operation of that width; v128 has no operations at all.
#यदि M3_SIZEOF_PTR == 8
#  define REFOP(NAME)  op_##NAME##_i64
#अन्यथा
#  define REFOP(NAME)  op_##NAME##_i32
#यदि_अन्तः

// clang-format off

स्थिर ध्रुव IM3Operation c_preserve_समुच्चयः_Slot[] = { रिक्त, op_Preserve_समुच्चयः_Slot_i32,       op_Preserve_समुच्चयः_Slot_i64,
                                                    FPOP(op_Preserve_समुच्चयः_Slot_f32), FPOP(op_Preserve_समुच्चयः_Slot_f64),
                                                    रिक्त, REFOP(Preserve_समुच्चयः_Slot),  REFOP(Preserve_समुच्चयः_Slot),
                                                    REFOP(Preserve_समुच्चयः_Slot),       REFOP(Preserve_समुच्चयः_Slot) };
स्थिर ध्रुव IM3Operation c_समुच्चयः_समुच्चयः_Ops[] =       { रिक्त, op_समुच्चयः_Slot_i32,               op_समुच्चयः_Slot_i64,
                                                    FPOP(op_समुच्चयः_Slot_f32),         FPOP(op_समुच्चयः_Slot_f64),
                                                    रिक्त, REFOP(समुच्चयः_Slot),         REFOP(समुच्चयः_Slot),
                                                    REFOP(समुच्चयः_Slot),               REFOP(समुच्चयः_Slot) };
स्थिर ध्रुव IM3Operation c_समुच्चयः_Global_Ops[] =    { रिक्त, op_समुच्चयः_Global_i32,             op_समुच्चयः_Global_i64,
                                                    FPOP(op_समुच्चयः_Global_f32),       FPOP(op_समुच्चयः_Global_f64),
                                                    रिक्त, REFOP(समुच्चयः_Global),       REFOP(समुच्चयः_Global),
                                                    REFOP(समुच्चयः_Global),             REFOP(समुच्चयः_Global) };
स्थिर ध्रुव IM3Operation c_समुच्चयः_पञ्जिकाकोशः_Ops[] =  { रिक्त, op_समुच्चयः_पञ्जिकाकोशः_i32,           op_समुच्चयः_पञ्जिकाकोशः_i64,
                                                    FPOP(op_समुच्चयः_पञ्जिकाकोशः_f32),     FPOP(op_समुच्चयः_पञ्जिकाकोशः_f64),
                                                    रिक्त, REFOP(समुच्चयः_पञ्जिकाकोशः),     REFOP(समुच्चयः_पञ्जिकाकोशः),
                                                    REFOP(समुच्चयः_पञ्जिकाकोशः),           REFOP(समुच्चयः_पञ्जिकाकोशः) };

// A table shorter than the enum reads out of bounds, so tie the two together at
// build time: adding a value type must not silently outgrow these.
#परिभाषय d_m3_Check_Type_सारणी(सारणी) \
    M3_STATIC_ASSERT (परिमाण (सारणी) / परिमाण (*(सारणी)) == c_m3_Type_गणनम्, सारणी##_needs_one_entry_per_type)

d_m3_Check_Type_सारणी(c_preserve_समुच्चयः_Slot);
d_m3_Check_Type_सारणी(c_समुच्चयः_समुच्चयः_Ops);
d_m3_Check_Type_सारणी(c_समुच्चयः_Global_Ops);
d_m3_Check_Type_सारणी(c_समुच्चयः_पञ्जिकाकोशः_Ops);

#यदि d_m3_Fold_समुच्चयः_Local

// destination-folded variants of the hot binops, indexed by the operand-form index
// recorded at emission: [0]=_rs [1]=_sr [2]=_ss [3]=(fp _rr, never folded)
#परिभाषय d_foldOpList(TYPE, NAME)            { op_##TYPE##_##NAME##_rs_f, op_##TYPE##_##NAME##_sr_f, op_##TYPE##_##NAME##_ss_f, रिक्त }
#परिभाषय d_foldCommutativeOpList(TYPE, NAME) { op_##TYPE##_##NAME##_rs_f, रिक्त,                      op_##TYPE##_##NAME##_ss_f, रिक्त }

स्थिर ध्रुव IM3Operation c_fold_i32_सङ्कलय[]         = d_foldCommutativeOpList (i32, सङ्कलय);
स्थिर ध्रुव IM3Operation c_fold_i32_व्यवकलय[]    = d_foldOpList (i32, व्यवकलय);
स्थिर ध्रुव IM3Operation c_fold_i32_Multiply[]    = d_foldCommutativeOpList (i32, Multiply);
स्थिर ध्रुव IM3Operation c_fold_u32_And[]         = d_foldCommutativeOpList (u32, And);
स्थिर ध्रुव IM3Operation c_fold_u32_Or[]          = d_foldCommutativeOpList (u32, Or);
स्थिर ध्रुव IM3Operation c_fold_u32_Xor[]         = d_foldCommutativeOpList (u32, Xor);
स्थिर ध्रुव IM3Operation c_fold_u32_सरणम्_वामः[]   = d_foldOpList (u32, सरणम्_वामः);
स्थिर ध्रुव IM3Operation c_fold_i32_सरणम्_दक्षिणः[]  = d_foldOpList (i32, सरणम्_दक्षिणः);
स्थिर ध्रुव IM3Operation c_fold_u32_सरणम्_दक्षिणः[]  = d_foldOpList (u32, सरणम्_दक्षिणः);

स्थिर ध्रुव IM3Operation c_fold_i64_सङ्कलय[]         = d_foldCommutativeOpList (i64, सङ्कलय);
स्थिर ध्रुव IM3Operation c_fold_i64_व्यवकलय[]    = d_foldOpList (i64, व्यवकलय);
स्थिर ध्रुव IM3Operation c_fold_i64_Multiply[]    = d_foldCommutativeOpList (i64, Multiply);
स्थिर ध्रुव IM3Operation c_fold_u64_And[]         = d_foldCommutativeOpList (u64, And);
स्थिर ध्रुव IM3Operation c_fold_u64_Or[]          = d_foldCommutativeOpList (u64, Or);
स्थिर ध्रुव IM3Operation c_fold_u64_Xor[]         = d_foldCommutativeOpList (u64, Xor);
स्थिर ध्रुव IM3Operation c_fold_u64_सरणम्_वामः[]   = d_foldOpList (u64, सरणम्_वामः);
स्थिर ध्रुव IM3Operation c_fold_i64_सरणम्_दक्षिणः[]  = d_foldOpList (i64, सरणम्_दक्षिणः);
स्थिर ध्रुव IM3Operation c_fold_u64_सरणम्_दक्षिणः[]  = d_foldOpList (u64, सरणम्_दक्षिणः);

#  यदि d_m3HasFloat
स्थिर ध्रुव IM3Operation c_fold_f32_सङ्कलय[]         = d_foldCommutativeOpList (f32, सङ्कलय);
स्थिर ध्रुव IM3Operation c_fold_f32_व्यवकलय[]    = d_foldOpList (f32, व्यवकलय);
स्थिर ध्रुव IM3Operation c_fold_f32_Multiply[]    = d_foldCommutativeOpList (f32, Multiply);
स्थिर ध्रुव IM3Operation c_fold_f32_Divide[]      = d_foldOpList (f32, Divide);
स्थिर ध्रुव IM3Operation c_fold_f64_सङ्कलय[]         = d_foldCommutativeOpList (f64, सङ्कलय);
स्थिर ध्रुव IM3Operation c_fold_f64_व्यवकलय[]    = d_foldOpList (f64, व्यवकलय);
स्थिर ध्रुव IM3Operation c_fold_f64_Multiply[]    = d_foldCommutativeOpList (f64, Multiply);
स्थिर ध्रुव IM3Operation c_fold_f64_Divide[]      = d_foldOpList (f64, Divide);
#  endif

// loads are unary: [0]= address in _r0, [1]= address in a slot
#परिभाषय d_fold_आरोपय_List(DEST, SRC)       { op_##DEST##_Load_##SRC##_r_f, op_##DEST##_Load_##SRC##_s_f, रिक्त, रिक्त }

स्थिर ध्रुव IM3Operation c_fold_i32_आरोपय_i32[] = d_fold_आरोपय_List(i32, i32);
स्थिर ध्रुव IM3Operation c_fold_i32_आरोपय_i8[] = d_fold_आरोपय_List(i32, i8);
स्थिर ध्रुव IM3Operation c_fold_i32_आरोपय_u8[] = d_fold_आरोपय_List(i32, u8);
स्थिर ध्रुव IM3Operation c_fold_i32_आरोपय_i16[] = d_fold_आरोपय_List(i32, i16);
स्थिर ध्रुव IM3Operation c_fold_i32_आरोपय_u16[] = d_fold_आरोपय_List(i32, u16);

स्थिर ध्रुव IM3Operation c_fold_i64_आरोपय_i64[] = d_fold_आरोपय_List(i64, i64);
स्थिर ध्रुव IM3Operation c_fold_i64_आरोपय_i8[] = d_fold_आरोपय_List(i64, i8);
स्थिर ध्रुव IM3Operation c_fold_i64_आरोपय_u8[] = d_fold_आरोपय_List(i64, u8);
स्थिर ध्रुव IM3Operation c_fold_i64_आरोपय_i16[] = d_fold_आरोपय_List(i64, i16);
स्थिर ध्रुव IM3Operation c_fold_i64_आरोपय_u16[] = d_fold_आरोपय_List(i64, u16);
स्थिर ध्रुव IM3Operation c_fold_i64_आरोपय_i32[] = d_fold_आरोपय_List(i64, i32);
स्थिर ध्रुव IM3Operation c_fold_i64_आरोपय_u32[] = d_fold_आरोपय_List(i64, u32);
#  यदि d_m3HasFloat
स्थिर ध्रुव IM3Operation c_fold_f32_आरोपय_f32[] = d_fold_आरोपय_List(f32, f32);
स्थिर ध्रुव IM3Operation c_fold_f64_आरोपय_f64[] = d_fold_आरोपय_List(f64, f64);
#  endif

स्थिर
IM3Operation GetFoldOp (m3opcode_t i_opcode, u8 i_form)
{
    यदि (i_form >= 4) {
        प्रत्यागम रिक्त;
    }

    विकल्प (i_opcode) {
    पक्ष 0x6a:  प्रत्यागम c_fold_i32_सङ्कलय        [i_form];      // i32.add
    पक्ष 0x6b:  प्रत्यागम c_fold_i32_व्यवकलय   [i_form];      // i32.sub
    पक्ष 0x6c:  प्रत्यागम c_fold_i32_Multiply   [i_form];      // i32.mul
    पक्ष 0x71:  प्रत्यागम c_fold_u32_And        [i_form];      // i32.and
    पक्ष 0x72:  प्रत्यागम c_fold_u32_Or         [i_form];      // i32.or
    पक्ष 0x73:  प्रत्यागम c_fold_u32_Xor        [i_form];      // i32.xor
    पक्ष 0x74:  प्रत्यागम c_fold_u32_सरणम्_वामः  [i_form];      // i32.shl
    पक्ष 0x75:  प्रत्यागम c_fold_i32_सरणम्_दक्षिणः [i_form];      // i32.shr_s
    पक्ष 0x76:  प्रत्यागम c_fold_u32_सरणम्_दक्षिणः [i_form];      // i32.shr_u

    पक्ष 0x7c:  प्रत्यागम c_fold_i64_सङ्कलय        [i_form];      // i64.add
    पक्ष 0x7d:  प्रत्यागम c_fold_i64_व्यवकलय   [i_form];      // i64.sub
    पक्ष 0x7e:  प्रत्यागम c_fold_i64_Multiply   [i_form];      // i64.mul
    पक्ष 0x83:  प्रत्यागम c_fold_u64_And        [i_form];      // i64.and
    पक्ष 0x84:  प्रत्यागम c_fold_u64_Or         [i_form];      // i64.or
    पक्ष 0x85:  प्रत्यागम c_fold_u64_Xor        [i_form];      // i64.xor
    पक्ष 0x86:  प्रत्यागम c_fold_u64_सरणम्_वामः  [i_form];      // i64.shl
    पक्ष 0x87:  प्रत्यागम c_fold_i64_सरणम्_दक्षिणः [i_form];      // i64.shr_s
    पक्ष 0x88:  प्रत्यागम c_fold_u64_सरणम्_दक्षिणः [i_form];      // i64.shr_u

#  यदि d_m3HasFloat
    पक्ष 0x92:  प्रत्यागम c_fold_f32_सङ्कलय        [i_form];      // f32.add
    पक्ष 0x93:  प्रत्यागम c_fold_f32_व्यवकलय   [i_form];      // f32.sub
    पक्ष 0x94:  प्रत्यागम c_fold_f32_Multiply   [i_form];      // f32.mul
    पक्ष 0x95:  प्रत्यागम c_fold_f32_Divide     [i_form];      // f32.div
    पक्ष 0xa0:  प्रत्यागम c_fold_f64_सङ्कलय        [i_form];      // f64.add
    पक्ष 0xa1:  प्रत्यागम c_fold_f64_व्यवकलय   [i_form];      // f64.sub
    पक्ष 0xa2:  प्रत्यागम c_fold_f64_Multiply   [i_form];      // f64.mul
    पक्ष 0xa3:  प्रत्यागम c_fold_f64_Divide     [i_form];      // f64.div
#  endif

    पक्ष 0x28:  प्रत्यागम c_fold_i32_आरोपय_i32   [i_form];      // i32.load
    पक्ष 0x2c:  प्रत्यागम c_fold_i32_आरोपय_i8    [i_form];      // i32.load8_s
    पक्ष 0x2d:  प्रत्यागम c_fold_i32_आरोपय_u8    [i_form];      // i32.load8_u
    पक्ष 0x2e:  प्रत्यागम c_fold_i32_आरोपय_i16   [i_form];      // i32.load16_s
    पक्ष 0x2f:  प्रत्यागम c_fold_i32_आरोपय_u16   [i_form];      // i32.load16_u

    पक्ष 0x29:  प्रत्यागम c_fold_i64_आरोपय_i64   [i_form];      // i64.load
    पक्ष 0x30:  प्रत्यागम c_fold_i64_आरोपय_i8    [i_form];      // i64.load8_s
    पक्ष 0x31:  प्रत्यागम c_fold_i64_आरोपय_u8    [i_form];      // i64.load8_u
    पक्ष 0x32:  प्रत्यागम c_fold_i64_आरोपय_i16   [i_form];      // i64.load16_s
    पक्ष 0x33:  प्रत्यागम c_fold_i64_आरोपय_u16   [i_form];      // i64.load16_u
    पक्ष 0x34:  प्रत्यागम c_fold_i64_आरोपय_i32   [i_form];      // i64.load32_s
    पक्ष 0x35:  प्रत्यागम c_fold_i64_आरोपय_u32   [i_form];      // i64.load32_u

#  यदि d_m3HasFloat
    पक्ष 0x2a:  प्रत्यागम c_fold_f32_आरोपय_f32   [i_form];      // f32.load
    पक्ष 0x2b:  प्रत्यागम c_fold_f64_आरोपय_f64   [i_form];      // f64.load
#  endif

    स्वभाव: प्रत्यागम रिक्त;
    }
}

#यदि_अन्तः // d_m3FoldSetLocal

// clang-format on

#यदि d_m3FuseBranch

// fused compare+branch/if variants, indexed like the fold lists: [0]=_rs [1]=_sr [2]=_ss [3]=unused.
// the [1] entries are NULL for commutative compares: the compiler never records the _sr form for them
#  define d_fuseCmpList(TYPE, NAME, विधा_2)             { op_##TYPE##_##KIND##_##NAME##_rs, op_##TYPE##_##KIND##_##NAME##_sr, op_##TYPE##_##KIND##_##NAME##_ss, रिक्त }
#  define d_fuseCommutativeCmpList(TYPE, NAME, विधा_2)  { op_##TYPE##_##KIND##_##NAME##_rs, रिक्त,                             op_##TYPE##_##KIND##_##NAME##_ss, रिक्त }

प्रकारनिर्देश संरचना M3FusedCmpOps {
    IM3Operation branchIf[4];
    IM3Operation ifOp[4];
} M3FusedCmpOps;

#  define d_fuseCmp(TYPE, NAME)               { d_fuseCmpList (TYPE, NAME, BranchIf),            d_fuseCmpList (TYPE, NAME, If) }
#  define d_fuseCommutativeCmp(TYPE, NAME)    { d_fuseCommutativeCmpList (TYPE, NAME, BranchIf), d_fuseCommutativeCmpList (TYPE, NAME, If) }

// clang-format off
स्थिर ध्रुव M3FusedCmpOps c_fuse_i32_Equal              = d_fuseCommutativeCmp (i32, Equal);
स्थिर ध्रुव M3FusedCmpOps c_fuse_i32_NotEqual           = d_fuseCommutativeCmp (i32, NotEqual);
स्थिर ध्रुव M3FusedCmpOps c_fuse_i32_LessThan           = d_fuseCmp (i32, LessThan);
स्थिर ध्रुव M3FusedCmpOps c_fuse_u32_LessThan           = d_fuseCmp (u32, LessThan);
स्थिर ध्रुव M3FusedCmpOps c_fuse_i32_GreaterThan        = d_fuseCmp (i32, GreaterThan);
स्थिर ध्रुव M3FusedCmpOps c_fuse_u32_GreaterThan        = d_fuseCmp (u32, GreaterThan);
स्थिर ध्रुव M3FusedCmpOps c_fuse_i32_LessThanOrEqual    = d_fuseCmp (i32, LessThanOrEqual);
स्थिर ध्रुव M3FusedCmpOps c_fuse_u32_LessThanOrEqual    = d_fuseCmp (u32, LessThanOrEqual);
स्थिर ध्रुव M3FusedCmpOps c_fuse_i32_GreaterThanOrEqual = d_fuseCmp (i32, GreaterThanOrEqual);
स्थिर ध्रुव M3FusedCmpOps c_fuse_u32_GreaterThanOrEqual = d_fuseCmp (u32, GreaterThanOrEqual);
// clang-format on

// i32.eqz is unary: [0]= operand in _r0, [1]= operand in a slot
स्थिर ध्रुव M3FusedCmpOps c_fuse_i32_Eqz = {
    { op_i32_BranchIfEqz_r, op_i32_BranchIfEqz_s, रिक्त, रिक्त },
    { op_i32_IfEqz_r,       op_i32_IfEqz_s,       रिक्त, रिक्त }
};

स्थिर
ध्रुव M3FusedCmpOps* GetFusedCmpOps (m3opcode_t i_opcode)
{
    विकल्प (i_opcode) {
    पक्ष 0x45: प्रत्यागम &c_fuse_i32_Eqz;                    // i32.eqz
    पक्ष 0x46: प्रत्यागम &c_fuse_i32_Equal;                  // i32.eq
    पक्ष 0x47: प्रत्यागम &c_fuse_i32_NotEqual;               // i32.ne
    पक्ष 0x48: प्रत्यागम &c_fuse_i32_LessThan;               // i32.lt_s
    पक्ष 0x49: प्रत्यागम &c_fuse_u32_LessThan;               // i32.lt_u
    पक्ष 0x4a: प्रत्यागम &c_fuse_i32_GreaterThan;            // i32.gt_s
    पक्ष 0x4b: प्रत्यागम &c_fuse_u32_GreaterThan;            // i32.gt_u
    पक्ष 0x4c: प्रत्यागम &c_fuse_i32_LessThanOrEqual;        // i32.le_s
    पक्ष 0x4d: प्रत्यागम &c_fuse_u32_LessThanOrEqual;        // i32.le_u
    पक्ष 0x4e: प्रत्यागम &c_fuse_i32_GreaterThanOrEqual;     // i32.ge_s
    पक्ष 0x4f: प्रत्यागम &c_fuse_u32_GreaterThanOrEqual;     // i32.ge_u
    स्वभाव: प्रत्यागम रिक्त;
    }
}

#यदि_अन्तः // d_m3FuseBranch

स्थिर ध्रुव IM3Operation c_intSelectOps[2][4] = {
    { op_Select_i32_rss, op_Select_i32_srs, op_Select_i32_ssr, op_Select_i32_sss },
    { op_Select_i64_rss, op_Select_i64_srs, op_Select_i64_ssr, op_Select_i64_sss }
};
#यदि d_m3HasFloat
स्थिर ध्रुव IM3Operation c_fpSelectOps[2][2][3] = {
    { { op_Select_f32_sss, op_Select_f32_srs, op_Select_f32_ssr }, // selector in slot
      { op_Select_f32_rss, op_Select_f32_rrs, op_Select_f32_rsr } }, // selector in reg
    { { op_Select_f64_sss, op_Select_f64_srs, op_Select_f64_ssr }, // selector in slot
      { op_Select_f64_rss, op_Select_f64_rrs, op_Select_f64_rsr } }
};    // selector in reg
#यदि_अन्तः

// all args & returns are 64-bit aligned, so use 2 slots for a d_m3Use32BitSlots=1 build

स्थिर
M3_परिणाम Acquire_Compilation_संहिता_Page (IM3Compilation o, IM3_संहिता_Page* o_संहिता_Page)
{
    M3_परिणाम परिणाम = m3Err_none;

    IM3_संहिता_Page page = Acquire_संहिता_Page(o->runtime);

    यदि (page) {
#यदि (d_m3_Enable_संहिता_Page_Ref_Counting)
        {
            यदि (o->function) {
                IM3Function func = o->function;
                page->info.usage_गणनम्++;

                u32 index = func->अङ्कः_संहिता_Page_Refs++;
_               (m3ReallocArray(&func->संहिता_Page_Refs, IM3_संहिता_Page, func->अङ्कः_संहिता_Page_Refs, index));
                func->संहिता_Page_Refs[index] = page;
            }
        }
#यदि_अन्तः
    } अन्यथा {
        _throw(m3_Err_malloc_Failed_संहिता_Page);
    }

    _catch:

    *o_संहिता_Page = page;

    प्रत्यागम परिणाम;
}

स्थिर अन्तःस्थ
शून्य Release_Compilation_संहिता_Page (IM3Compilation o)
{
    Release_संहिता_Page(o->runtime, o->page);
}

स्थिर अन्तःस्थ
u16 Get_Type_अङ्कः_Slots (m3type_t i_type)
{
    i_type = आधारः_Type_सम्बन्धि(i_type);

    // v128 is 16 bytes - 4 slots in 32-bit-slot mode, 2 in 64-bit.
    // (Slot-allocator only; no v128 ops execute.)
    यदि (i_type == c_m3Type_v128)
#यदि d_m3Use32BitSlots
        प्रत्यागम 4;
#अन्यथा
        प्रत्यागम 2;
#यदि_अन्तः
#यदि d_m3Use32BitSlots
    प्रत्यागम Is64BitType(i_type) ? 2 : 1;
#अन्यथा
    प्रत्यागम 1;
#यदि_अन्तः
}

स्थिर अन्तःस्थ
शून्य Align_Slot_गन्तव्यम्_Type (u16* io_slot, m3type_t i_type)
{
    // align 64-bit words to even slots (if d_m3Use32BitSlots)
    u16 अङ्कः_Slots = Get_Type_अङ्कः_Slots(i_type);

    u16 आवरणम् = अङ्कः_Slots - 1;
    *io_slot = (*io_slot + आवरणम्) & ~आवरणम्;
}

// Deliberately unchecked. Validation rejects an empty operand stack before compilation.
स्थिर अन्तःस्थ
i16 Get_राशिः_शिखरम्_सूचकांक (IM3Compilation o)
{
    प्रत्यागम o->राशिः_सूचकांक - 1;
}

स्थिर अन्तःस्थ
M3_परिणाम Get_राशिः_शिखरम्_सूचकांक_Throws (IM3Compilation o, i16* o_राशिः_सूचकांक)
{
    *o_राशिः_सूचकांक = o->राशिः_सूचकांक - 1;

    यदि (M3_LIKELY(o->राशिः_सूचकांक > o->राशिः_First_Dynamic_सूचकांक or Is_राशिः_Polymorphic(o))) {
        प्रत्यागम m3Err_none;
    } अन्यथा {
        प्रत्यागम m3_Err_function_राशिः_Underrun;
    }
}


// Items in the static portion of the stack (args/locals) are hidden from GetStackTypeFromTop ()
// In other words, only "real" Wasm stack items can be inspected.  This is important when
// returning values, etc. and you need an accurate wasm-view of the stack.
स्थिर
m3type_t Get_राशिः_Type_From_शिखरम् (IM3Compilation o, u16 i_विस्थापनम्)
{
    m3type_t type = c_m3Type_none;

    ++i_विस्थापनम्;
    यदि (o->राशिः_सूचकांक >= i_विस्थापनम्) {
        u16 index = o->राशिः_सूचकांक - i_विस्थापनम्;

        यदि (index >= o->राशिः_First_Dynamic_सूचकांक) {
            type = o->type_राशिः[index];
        }
    }

    प्रत्यागम type;
}

स्थिर अन्तःस्थ
m3type_t Get_राशिः_शिखरम्_Type (IM3Compilation o)
{
    प्रत्यागम Get_राशिः_Type_From_शिखरम्(o, 0);
}

स्थिर अन्तःस्थ
m3type_t Get_राशिः_Type_From_Bottom (IM3Compilation o, u16 i_विस्थापनम्)
{
    m3type_t type = c_m3Type_none;

    यदि (i_विस्थापनम् < o->राशिः_सूचकांक) {
        type = o->type_राशिः[i_विस्थापनम्];
    }

    प्रत्यागम type;
}


स्थिर अन्तःस्थ
bool IsConstantSlot (IM3Compilation o, u16 i_slot)
{
    प्रत्यागम (i_slot >= o->slot_First_Const_सूचकांक and i_slot < o->slot_Max_Const_सूचकांक);
}
स्थिर अन्तःस्थ
bool IsSlotAllocated (IM3Compilation o, u16 i_slot)
{
    प्रत्यागम o->m3Slots[i_slot];
}

स्थिर अन्तःस्थ
bool Is_राशिः_सूचकांक_अन्तर्_पञ्जिकाकोशः (IM3Compilation o, i32 i_राशिः_सूचकांक)
{                                                                           d_m3Assert (i_राशिः_सूचकांक < o->राशिः_सूचकांक or Is_राशिः_Polymorphic (o));
    यदि (i_राशिः_सूचकांक >= 0 and i_राशिः_सूचकांक < o->राशिः_सूचकांक) {
        प्रत्यागम (o->wasm_राशिः[i_राशिः_सूचकांक] >= d_m3Reg0SlotAlias);
    } अन्यथा {
        प्रत्यागम false;
    }
}

स्थिर अन्तःस्थ
u16 Get_अङ्कः_खण्डम्_मानानि_On_राशिः (IM3Compilation o)
{
    प्रत्यागम o->राशिः_सूचकांक - o->खण्डम्.खण्डम्_राशिः_सूचकांक;
}

स्थिर अन्तःस्थ
bool Is_राशिः_शिखरम्_अन्तर्_पञ्जिकाकोशः (IM3Compilation o)
{
    प्रत्यागम Is_राशिः_सूचकांक_अन्तर्_पञ्जिकाकोशः(o, (i32)Get_राशिः_शिखरम्_सूचकांक(o));
}
स्थिर अन्तःस्थ
bool Is_राशिः_शिखरम्_Minus1_अन्तर्_पञ्जिकाकोशः (IM3Compilation o)
{
    प्रत्यागम Is_राशिः_सूचकांक_अन्तर्_पञ्जिकाकोशः(o, (i32)Get_राशिः_शिखरम्_सूचकांक(o) - 1);
}
स्थिर अन्तःस्थ
bool Is_राशिः_शिखरम्_Minus2_अन्तर्_पञ्जिकाकोशः (IM3Compilation o)
{
    प्रत्यागम Is_राशिः_सूचकांक_अन्तर्_पञ्जिकाकोशः(o, (i32)Get_राशिः_शिखरम्_सूचकांक(o) - 2);
}

स्थिर अन्तःस्थ
bool Is_राशिः_शिखरम्_अन्तर्_Slot (IM3Compilation o)
{
    प्रत्यागम not Is_राशिः_शिखरम्_अन्तर्_पञ्जिकाकोशः(o);
}

स्थिर अन्तःस्थ
bool IsValidSlot (u16 i_slot)
{
    प्रत्यागम (i_slot < d_m3MaxFunctionSlots);
}

स्थिर अन्तःस्थ
u16 Get_राशिः_शिखरम्_Slot_सङ्ख्या (IM3Compilation o)
{
    i16 i = Get_राशिः_शिखरम्_सूचकांक(o);

    u16 slot = c_slot_अप्रयुक्तम्;

    यदि (i >= 0) {
        slot = o->wasm_राशिः[i];
    }

    प्रत्यागम slot;
}


// from bottom
स्थिर अन्तःस्थ
u16 Get_Slot_For_राशिः_सूचकांक (IM3Compilation o, u16 i_राशिः_सूचकांक)
{                                                                   d_m3Assert (i_राशिः_सूचकांक < o->राशिः_सूचकांक or Is_राशिः_Polymorphic (o));
    u16 slot = c_slot_अप्रयुक्तम्;

    यदि (i_राशिः_सूचकांक < o->राशिः_सूचकांक) {
        slot = o->wasm_राशिः[i_राशिः_सूचकांक];
    }

    प्रत्यागम slot;
}

स्थिर अन्तःस्थ
u16 Get_Extra_Slot_For_राशिः_सूचकांक (IM3Compilation o, u16 i_राशिः_सूचकांक)
{
    u16 आधारः_Slot = Get_Slot_For_राशिः_सूचकांक(o, i_राशिः_सूचकांक);

    यदि (आधारः_Slot != c_slot_अप्रयुक्तम्) {
        u16 extraSlot = Get_Type_अङ्कः_Slots(Get_राशिः_Type_From_Bottom(o, i_राशिः_सूचकांक)) - 1;
        आधारः_Slot += extraSlot;
    }

    प्रत्यागम आधारः_Slot;
}


स्थिर अन्तःस्थ
शून्य TouchSlot (IM3Compilation o, u16 i_slot)
{
    // op_Entry uses this value to track and detect stack overflow
    o->max_राशिः_Slots = M3_MAX(o->max_राशिः_Slots, i_slot + 1);
}

स्थिर अन्तःस्थ
शून्य MarkSlotAllocated (IM3Compilation o, u16 i_slot)
{                                                                   d_m3Assert (o->m3Slots [i_slot] == 0); // shouldn't be already allocated
    o->m3Slots[i_slot] = 1;

    o->slot_Max_Allocated_सूचकांक_Plus_One = M3_MAX(o->slot_Max_Allocated_सूचकांक_Plus_One, i_slot + 1);

    TouchSlot(o, i_slot);
}

स्थिर अन्तःस्थ
M3_परिणाम MarkSlotsAllocated (IM3Compilation o, u16 i_slot, u16 i_अङ्कः_Slots)
{
    यदि (M3_UNLIKELY(i_slot + i_अङ्कः_Slots > d_m3MaxFunctionSlots)) {
        प्रत्यागम m3_Err_function_राशिः_अतिप्रवाहः;
    }

    यावत् (i_अङ्कः_Slots--) {
        MarkSlotAllocated(o, i_slot++);
    }

    प्रत्यागम m3Err_none;
}

स्थिर अन्तःस्थ
M3_परिणाम MarkSlotsAllocatedByType (IM3Compilation o, u16 i_slot, m3type_t i_type)
{
    u16 अङ्कः_Slots = Get_Type_अङ्कः_Slots(i_type);
    प्रत्यागम MarkSlotsAllocated(o, i_slot, अङ्कः_Slots);
}


स्थिर
M3_परिणाम AllocateSlotsWithinRange (IM3Compilation o, u16* o_slot, m3type_t i_type, u16 i_startSlot, u16 i_अन्तः_Slot)
{
    M3_परिणाम परिणाम = m3_Err_function_राशिः_अतिप्रवाहः;

    u16 अङ्कः_Slots     = Get_Type_अङ्कः_Slots(i_type);
    u16 अन्वेषणम्_विस्थापनम् = अङ्कः_Slots - 1;

    Align_Slot_गन्तव्यम्_Type(&i_startSlot, i_type);

    // search for 1 or 2 consecutive slots in the execution stack
    u16 i = i_startSlot;
    यावत् (i + अन्वेषणम्_विस्थापनम् < i_अन्तः_Slot) {
        यदि (i + अन्वेषणम्_विस्थापनम् < d_m3MaxFunctionSlots and o->m3Slots[i] == 0 and o->m3Slots[i + अन्वेषणम्_विस्थापनम्] == 0) {
            MarkSlotsAllocated(o, i, अङ्कः_Slots);

            *o_slot = i;
            परिणाम  = m3Err_none;
            भङ्ग;
        }

        // keep 2-slot allocations even-aligned
        i += अङ्कः_Slots;
    }

    प्रत्यागम परिणाम;
}

स्थिर अन्तःस्थ
M3_परिणाम AllocateSlots (IM3Compilation o, u16* o_slot, m3type_t i_type)
{
    प्रत्यागम AllocateSlotsWithinRange(o, o_slot, i_type, o->slot_First_Dynamic_सूचकांक, d_m3MaxFunctionSlots);
}

स्थिर अन्तःस्थ
M3_परिणाम AllocateConstantSlots (IM3Compilation o, u16* o_slot, m3type_t i_type)
{
    u16 max_सारणी_सूचकांक = o->slot_First_Const_सूचकांक + d_m3_Max_Constant_सारणी_आकार;
    प्रत्यागम AllocateSlotsWithinRange(o, o_slot, i_type, o->slot_First_Const_सूचकांक, M3_MIN(o->slot_First_Dynamic_सूचकांक, max_सारणी_सूचकांक));
}


// TOQUE: this usage count system could be eliminated. real world code doesn't frequently trigger it.  just copy to multiple
// unique slots.
स्थिर अन्तःस्थ
M3_परिणाम Increment_Slot_Usage_गणनम् (IM3Compilation o, u16 i_slot)
{                                                                                       d_m3Assert (i_slot < d_m3MaxFunctionSlots);
    M3_परिणाम परिणाम = m3Err_none;                                                       d_m3Assert (o->m3Slots [i_slot] > 0);

    // OPTZ (memory): 'm3Slots' could still be fused with 'typeStack' if 4 bits were used to indicate: [0,1,2,many]. The many-case
    // would scan 'wasmStack' to determine the actual usage count
    यदि (M3_LIKELY(o->m3Slots[i_slot] < 0xFF)) {
        o->m3Slots[i_slot]++;
    } अन्यथा {
        परिणाम = "slot usage count overflow";
    }

    प्रत्यागम परिणाम;
}

स्थिर अन्तःस्थ
शून्य DeallocateSlot (IM3Compilation o, i16 i_slot, m3type_t i_type)
{                                                                                       d_m3Assert (i_slot >= o->slot_First_Dynamic_सूचकांक);
                                                                                        d_m3Assert (i_slot < o->slot_Max_Allocated_सूचकांक_Plus_One);
    प्रति (u16 i = 0; i < Get_Type_अङ्कः_Slots(i_type); ++i, ++i_slot) {                       d_m3Assert (o->m3Slots [i_slot]);
        --o->m3Slots[i_slot];
    }
}


स्थिर अन्तःस्थ
bool Is_पञ्जिकाकोशः_Type_Allocated (IM3Compilation o, u8 i_type)
{
    प्रत्यागम Is_पञ्जिकाकोशः_Allocated(o, IsFpType(i_type));
}

स्थिर अन्तःस्थ
शून्य Allocate_पञ्जिकाकोशः (IM3Compilation o, u32 i_पञ्जिकाकोशः, u16 i_राशिः_सूचकांक)
{                                                                                       d_m3Assert (not Is_पञ्जिकाकोशः_Allocated (o, i_पञ्जिकाकोशः));
    o->reg_राशिः_सूचकांक_Plus_One[i_पञ्जिकाकोशः] = i_राशिः_सूचकांक + 1;
}

स्थिर अन्तःस्थ
शून्य Deallocate_पञ्जिकाकोशः (IM3Compilation o, u32 i_पञ्जिकाकोशः)
{                                                                                       d_m3Assert (Is_पञ्जिकाकोशः_Allocated (o, i_पञ्जिकाकोशः));
    o->reg_राशिः_सूचकांक_Plus_One[i_पञ्जिकाकोशः] = c_m3_पञ्जिकाकोशः_Unallocated;
}

स्थिर अन्तःस्थ
u16 Get_पञ्जिकाकोशः_राशिः_सूचकांक (IM3Compilation o, u32 i_पञ्जिकाकोशः)
{                                                                                       d_m3Assert (Is_पञ्जिकाकोशः_Allocated (o, i_पञ्जिकाकोशः));
    प्रत्यागम o->reg_राशिः_सूचकांक_Plus_One[i_पञ्जिकाकोशः] - 1;
}

u16 Get_Max_प्रयुक्तम्_Slot_Plus_One (IM3Compilation o)
{
    यावत् (o->slot_Max_Allocated_सूचकांक_Plus_One > o->slot_First_Dynamic_सूचकांक) {
        यदि (IsSlotAllocated(o, o->slot_Max_Allocated_सूचकांक_Plus_One - 1)) {
            भङ्ग;
        }

        o->slot_Max_Allocated_सूचकांक_Plus_One--;
    }

#यदि_परिभाषितम् DEBUG
    u16 maxSlot = o->slot_Max_Allocated_सूचकांक_Plus_One;
    यावत् (maxSlot < d_m3MaxFunctionSlots) {
        d_m3Assert(o->m3Slots[maxSlot] == 0);
        maxSlot++;
    }
#यदि_अन्तः

    प्रत्यागम o->slot_Max_Allocated_सूचकांक_Plus_One;
}

स्थिर
M3_परिणाम Preserve_पञ्जिकाकोशः_If_Occupied (IM3Compilation o, m3type_t i_पञ्जिकाकोशः_Type)
{
    M3_परिणाम परिणाम = m3Err_none;

    u32 regSelect = IsFpType(i_पञ्जिकाकोशः_Type);

    यदि (Is_पञ्जिकाकोशः_Allocated(o, regSelect)) {
        u16 राशिः_सूचकांक = Get_पञ्जिकाकोशः_राशिः_सूचकांक(o, regSelect);
        Deallocate_पञ्जिकाकोशः(o, regSelect);

        m3type_t type = Get_राशिः_Type_From_Bottom(o, राशिः_सूचकांक);

        // and point to a exec slot
        u16 slot = c_slot_अप्रयुक्तम्;
_       (AllocateSlots(o, &slot, type));
        o->wasm_राशिः[राशिः_सूचकांक] = slot;

        // Ensure type is within the valid range
        यदि (M3_LIKELY(आधारः_Type_सम्बन्धि(type) < c_m3_Type_गणनम्)) {
_           (EmitOp(o, c_समुच्चयः_समुच्चयः_Ops[आधारः_Type_सम्बन्धि(type)]));
        } अन्यथा {
            _throw(m3Err_unknownType);
        }

        Emit_Slot_विस्थापनम्(o, slot);
    }

    _catch: प्रत्यागम परिणाम;
}


// all values must be in slots before entering loop, if, and else blocks
// otherwise they'd end up preserve-copied in the block to probably different locations (if/else)
स्थिर अन्तःस्थ
M3_परिणाम PreserveRegisters (IM3Compilation o)
{
    M3_परिणाम परिणाम;

_   (Preserve_पञ्जिकाकोशः_If_Occupied(o, c_m3Type_f64));
_   (Preserve_पञ्जिकाकोशः_If_Occupied(o, c_m3Type_i64));

    _catch: प्रत्यागम परिणाम;
}

स्थिर
M3_परिणाम Preserve_Non_शिखरम्_Registers (IM3Compilation o)
{
    M3_परिणाम परिणाम = m3Err_none;

    i16 राशिः_शिखरम् = Get_राशिः_शिखरम्_सूचकांक(o);

    यदि (राशिः_शिखरम् >= 0) {
        यदि (Is_पञ्जिकाकोशः_Allocated(o, 0))     // r0
        {
            यदि (Get_पञ्जिकाकोशः_राशिः_सूचकांक(o, 0) != राशिः_शिखरम्) {
_               (Preserve_पञ्जिकाकोशः_If_Occupied(o, c_m3Type_i64));
            }
        }

        यदि (Is_पञ्जिकाकोशः_Allocated(o, 1))     // fp0
        {
            यदि (Get_पञ्जिकाकोशः_राशिः_सूचकांक(o, 1) != राशिः_शिखरम्) {
_               (Preserve_पञ्जिकाकोशः_If_Occupied(o, c_m3Type_f64));
            }
        }
    }

    _catch: प्रत्यागम परिणाम;
}


//----------------------------------------------------------------------------------------------------------------------

स्थिर
M3_परिणाम प्रक्षिप (IM3Compilation o, m3type_t i_type, u16 i_slot)
{
    M3_परिणाम परिणाम = m3Err_none;

#यदि !d_m3HasFloat
    यदि (i_type == c_m3Type_f32 || i_type == c_m3Type_f64) {
        प्रत्यागम m3Err_unknownOpcode;
    }
#यदि_अन्तः

    यदि (M3_UNLIKELY(o->राशिः_सूचकांक >= d_m3_Max_Function_राशिः_Height)) {
        प्रत्यागम m3_Err_function_राशिः_अतिप्रवाहः;
    }

    u16 राशिः_सूचकांक = o->राशिः_सूचकांक++;                                       // printf ("push: %d\n", (i32) i);

    o->wasm_राशिः[राशिः_सूचकांक] = i_slot;
    o->type_राशिः[राशिः_सूचकांक] = i_type;

    यदि (Is_पञ्जिकाकोशः_Slot_Alias(i_slot)) {
        u32 regSelect = Is_Fp_पञ्जिकाकोशः_Slot_Alias(i_slot);
        Allocate_पञ्जिकाकोशः(o, regSelect, राशिः_सूचकांक);
    }

    यदि (d_m3_Log_Wasm_राशिः) {
        dump_type_राशिः(o);
    }

    प्रत्यागम परिणाम;
}

स्थिर अन्तःस्थ
M3_परिणाम प्रक्षिप_पञ्जिकाकोशः (IM3Compilation o, m3type_t i_type)
{
    M3_परिणाम परिणाम = m3Err_none;                                                       d_m3Assert ((u16) d_m3Reg0SlotAlias > (u16) d_m3MaxFunctionSlots);
    u16      slot   = IsFpType(i_type) ? d_m3Fp0SlotAlias : d_m3Reg0SlotAlias;          d_m3Assert (i_type or Is_राशिः_Polymorphic (o));

_   (प्रक्षिप(o, i_type, slot));

    _catch: प्रत्यागम परिणाम;
}

स्थिर
M3_परिणाम निष्कासय (IM3Compilation o)
{
    M3_परिणाम परिणाम = m3Err_none;

    यदि (M3_LIKELY(o->राशिः_सूचकांक > o->खण्डम्.खण्डम्_राशिः_सूचकांक)) {
        o->राशिः_सूचकांक--;                                                //  printf ("pop: %d\n", (i32) o->stackIndex);

        u16      slot = o->wasm_राशिः[o->राशिः_सूचकांक];
        m3type_t type = o->type_राशिः[o->राशिः_सूचकांक];

        यदि (Is_पञ्जिकाकोशः_Slot_Alias(slot)) {
            u32 regSelect = Is_Fp_पञ्जिकाकोशः_Slot_Alias(slot);
            Deallocate_पञ्जिकाकोशः(o, regSelect);
        } अन्यथा यदि (M3_UNLIKELY(slot >= o->slot_Max_Allocated_सूचकांक_Plus_One)) {
            प्रत्यागम m3_Err_function_राशिः_Underrun; // Return error for invalid slot indices
        } अन्यथा यदि (slot >= o->slot_First_Dynamic_सूचकांक) {
            DeallocateSlot(o, slot, type);
        }
    } अन्यथा यदि (not Is_राशिः_Polymorphic(o)) {
        परिणाम = m3_Err_function_राशिः_Underrun;
    }

    प्रत्यागम परिणाम;
}

स्थिर
M3_परिणाम निष्कासय_Type (IM3Compilation o, m3type_t i_type)
{
    M3_परिणाम परिणाम = m3Err_none;

    m3type_t शिखरम्_Type = Get_राशिः_शिखरम्_Type(o);

    यदि (M3_LIKELY(Is_Sub_Type_सम्बन्धि(शिखरम्_Type, i_type) or o->खण्डम्.isPolymorphic)) {
_       (निष्कासय(o));
    } अन्यथा {
        _throw(m3Err_typeMismatch);
    }

    _catch:
    प्रत्यागम परिणाम;
}

स्थिर
M3_परिणाम प्रक्षिप_Allocated_Slot_And_Emit (IM3Compilation o, m3type_t i_type, bool i_doEmit)
{
    M3_परिणाम परिणाम = m3Err_none;

    u16 slot = c_slot_अप्रयुक्तम्;

_   (AllocateSlots(o, &slot, i_type));
_   (प्रक्षिप(o, i_type, slot));

    यदि (i_doEmit) {
        Emit_Slot_विस्थापनम्(o, slot);
    }

    //    printf ("push: %d\n", (u32) slot);

    _catch: प्रत्यागम परिणाम;
}

स्थिर अन्तःस्थ
M3_परिणाम प्रक्षिप_Allocated_Slot_And_Emit_2 (IM3Compilation o, m3type_t i_type)
{
    प्रत्यागम प्रक्षिप_Allocated_Slot_And_Emit(o, i_type, true);
}

स्थिर अन्तःस्थ
M3_परिणाम प्रक्षिप_Allocated_Slot (IM3Compilation o, m3type_t i_type)
{
    प्रत्यागम प्रक्षिप_Allocated_Slot_And_Emit(o, i_type, false);
}

स्थिर
M3_परिणाम प्रक्षिप_Const (IM3Compilation o, u64 i_word, m3type_t i_type)
{
    M3_परिणाम परिणाम = m3Err_none;

    // When compile-walking a constant expression without emitting there is no
    // constant table to place the value in, but the type still has to land on
    // the stack: extended-const arithmetic downstream consumes it.
    यदि (M3_UNLIKELY(!o->page)) {
        प्रत्यागम प्रक्षिप_Allocated_Slot(o, i_type);
    }

    bool मेलनम्_प्राप्तम्  = false;
    bool is64BitType = Is64BitType(i_type);

    u16 अङ्कः_Required_Slots  = Get_Type_अङ्कः_Slots(i_type);
    u16 अङ्कः_प्रयुक्तम्_Const_Slots = o->slot_Max_Const_सूचकांक - o->slot_First_Const_सूचकांक;

    // search for duplicate matching constant slot to reuse
    यदि (अङ्कः_Required_Slots == 2 and अङ्कः_प्रयुक्तम्_Const_Slots >= 2) {
        u16 firstConstSlot = o->slot_First_Const_सूचकांक;
        Align_Slot_गन्तव्यम्_Type(&firstConstSlot, c_m3Type_i64);

        प्रति (u16 slot = firstConstSlot; slot < o->slot_Max_Const_सूचकांक - 1; slot += 2) {
            यदि (IsSlotAllocated(o, slot) and IsSlotAllocated(o, slot + 1)) {
                u64 constant;
                स्मृतिं_प्रतिलिख(&constant, &o->constants[slot - o->slot_First_Const_सूचकांक], परिमाण(constant));

                यदि (constant == i_word) {
                    मेलनम्_प्राप्तम् = true;
_                   (प्रक्षिप(o, i_type, slot));
                    भङ्ग;
                }
            }
        }
    } अन्यथा यदि (अङ्कः_Required_Slots == 1) {
        प्रति (u16 i = 0; i < अङ्कः_प्रयुक्तम्_Const_Slots; ++i) {
            u16 slot = o->slot_First_Const_सूचकांक + i;

            यदि (IsSlotAllocated(o, slot)) {
                bool matches;
                यदि (is64BitType) {
                    u64 constant;
                    स्मृतिं_प्रतिलिख(&constant, &o->constants[i], परिमाण(constant));
                    matches = (constant == i_word);
                } अन्यथा {
                    u32 constant;
                    स्मृतिं_प्रतिलिख(&constant, &o->constants[i], परिमाण(constant));
                    matches = (constant == i_word);
                }
                यदि (matches) {
                    मेलनम्_प्राप्तम् = true;
_                   (प्रक्षिप(o, i_type, slot));
                    भङ्ग;
                }
            }
        }
    }

    यदि (not मेलनम्_प्राप्तम्) {
        u16 slot = c_slot_अप्रयुक्तम्;
        परिणाम   = AllocateConstantSlots(o, &slot, i_type);

        यदि (परिणाम || slot == c_slot_अप्रयुक्तम्) // no more constant table space; use inline constants
        {
            परिणाम = m3Err_none;

            यदि (is64BitType) {
_               (EmitOp(o, op_Const64));
                EmitWord64(o->page, i_word);
            } अन्यथा {
_               (EmitOp(o, op_Const32));
                EmitWord32(o->page, (u32)i_word);
            }

_           (प्रक्षिप_Allocated_Slot_And_Emit_2(o, i_type));
        } अन्यथा {
            u16 const_सारणी_सूचकांक = slot - o->slot_First_Const_सूचकांक;

            d_m3Assert(const_सारणी_सूचकांक < d_m3_Max_Constant_सारणी_आकार);

            यदि (is64BitType) {
                स्मृतिं_प्रतिलिख(&o->constants[const_सारणी_सूचकांक], &i_word, परिमाण(i_word));
            } अन्यथा {
                u32 word32 = (u32)i_word;
                स्मृतिं_प्रतिलिख(&o->constants[const_सारणी_सूचकांक], &word32, परिमाण(word32));
            }

_           (प्रक्षिप(o, i_type, slot));

            o->slot_Max_Const_सूचकांक = M3_MAX(slot + अङ्कः_Required_Slots, o->slot_Max_Const_सूचकांक);
        }
    }

    _catch: प्रत्यागम परिणाम;
}

स्थिर अन्तःस्थ
M3_परिणाम Emit_Slot_अङ्कः_सम्बन्धि_राशिः_शिखरम्_And_निष्कासय (IM3Compilation o)
{
    // no emit if value is in register
    यदि (Is_राशिः_शिखरम्_अन्तर्_Slot(o)) {
        Emit_Slot_विस्थापनम्(o, Get_राशिः_शिखरम्_Slot_सङ्ख्या(o));
    }

    प्रत्यागम निष्कासय(o);
}


// Or, maybe: EmitTrappingOp
M3_परिणाम सङ्कलय_Trap_लेखः (IM3Compilation o)
{
    M3_परिणाम परिणाम = m3Err_none;

    यदि (o->function) {
    }

    प्रत्यागम परिणाम;
}

स्थिर
M3_परिणाम Unwind_खण्डम्_राशिः (IM3Compilation o)
{
    M3_परिणाम परिणाम = m3Err_none;

    u32 निष्कासय_गणनम् = 0;
    यावत् (o->राशिः_सूचकांक > o->खण्डम्.खण्डम्_राशिः_सूचकांक) {
_       (निष्कासय(o));
        ++निष्कासय_गणनम्;
    }

    यदि (निष्कासय_गणनम्) {
        m3log(compile, "unwound stack top: %d", निष्कासय_गणनम्);
    }

    _catch: प्रत्यागम परिणाम;
}

स्थिर अन्तःस्थ
M3_परिणाम समुच्चयः_राशिः_Polymorphic (IM3Compilation o)
{
    o->खण्डम्.isPolymorphic = true;                              m3log (compile, "stack set polymorphic");
    प्रत्यागम Unwind_खण्डम्_राशिः(o);
}

स्थिर
शून्य PatchBranches (IM3Compilation o)
{
    InvalidateFold(o);         // patched branches land at the current pc

    pc_t pc = GetPC(o);

    pc_t patches     = o->खण्डम्.patches;
    o->खण्डम्.patches = रिक्त;

    यावत् (patches) {                                           m3log (compile, "patching location: %p to pc: %p", patches, pc);
        pc_t अग्र       = *(pc_t*)patches;
        *(pc_t*)patches = pc;
        patches         = अग्र;
    }
}

//-------------------------------------------------------------------------------------------------------------------------

स्थिर
M3_परिणाम Copy_राशिः_सूचकांक_गन्तव्यम्_Slot (IM3Compilation o, u16 i_destSlot, u16 i_राशिः_सूचकांक)  // NoPushPop
{
    M3_परिणाम परिणाम = m3Err_none;

    IM3Operation op;

    m3type_t type       = Get_राशिः_Type_From_Bottom(o, i_राशिः_सूचकांक);
    bool     अन्तर्_पञ्जिकाकोशः = Is_राशिः_सूचकांक_अन्तर्_पञ्जिकाकोशः(o, i_राशिः_सूचकांक);

    यदि (अन्तर्_पञ्जिकाकोशः) {
        op = c_समुच्चयः_समुच्चयः_Ops[आधारः_Type_सम्बन्धि(type)];
    } अन्यथा {
        op = Is64BitType(type) ? op_CopySlot_64 : op_CopySlot_32;
    }

_   (EmitOp(o, op));
    Emit_Slot_विस्थापनम्(o, i_destSlot);

    यदि (not अन्तर्_पञ्जिकाकोशः) {
        u16 srcSlot = Get_Slot_For_राशिः_सूचकांक(o, i_राशिः_सूचकांक);
        Emit_Slot_विस्थापनम्(o, srcSlot);
    }

    _catch: प्रत्यागम परिणाम;
}

स्थिर
M3_परिणाम Copy_राशिः_शिखरम्_गन्तव्यम्_Slot (IM3Compilation o, u16 i_destSlot)  // NoPushPop
{
    M3_परिणाम परिणाम;

    i16 राशिः_शिखरम्;
_   (Get_राशिः_शिखरम्_सूचकांक_Throws(o, &राशिः_शिखरम्));
_   (Copy_राशिः_सूचकांक_गन्तव्यम्_Slot(o, i_destSlot, (u16)राशिः_शिखरम्));

    _catch: प्रत्यागम परिणाम;
}


// a copy-on-write strategy is used with locals. when a get local occurs, it's not copied anywhere. the stack
// entry just has a index pointer to that local memory slot.
// then, when a previously referenced local is set, the current value needs to be preserved for those references

// TODO: consider getting rid of these specialized operations: PreserveSetSlot & PreserveCopySlot.
// They likely just take up space (which seems to reduce performance) without improving performance.
// => unconfirmed in preliminary testing.
स्थिर
M3_परिणाम Preserved_Copy_शिखरम्_Slot (IM3Compilation o, u16 i_destSlot, u16 i_preserveSlot)
{
    M3_परिणाम परिणाम = m3Err_none;             d_m3Assert (i_destSlot != i_preserveSlot);

    IM3Operation op;

    m3type_t type = Get_राशिः_शिखरम्_Type(o);

    यदि (Is_राशिः_शिखरम्_अन्तर्_पञ्जिकाकोशः(o)) {
        op = c_preserve_समुच्चयः_Slot[आधारः_Type_सम्बन्धि(type)];
    } अन्यथा {
        op = Is64BitType(type) ? op_PreserveCopySlot_64 : op_PreserveCopySlot_32;
    }

_   (EmitOp(o, op));
    Emit_Slot_विस्थापनम्(o, i_destSlot);

    यदि (Is_राशिः_शिखरम्_अन्तर्_Slot(o)) {
        Emit_Slot_विस्थापनम्(o, Get_राशिः_शिखरम्_Slot_सङ्ख्या(o));
    }

    Emit_Slot_विस्थापनम्(o, i_preserveSlot);

    _catch: प्रत्यागम परिणाम;
}

स्थिर
M3_परिणाम Copy_राशिः_शिखरम्_गन्तव्यम्_पञ्जिकाकोशः (IM3Compilation o, bool i_update_राशिः)
{
    M3_परिणाम परिणाम = m3Err_none;

    यदि (Is_राशिः_शिखरम्_अन्तर्_Slot(o)) {
        m3type_t type = Get_राशिः_शिखरम्_Type(o);

_       (Preserve_पञ्जिकाकोशः_If_Occupied(o, type));

        IM3Operation op = c_समुच्चयः_पञ्जिकाकोशः_Ops[आधारः_Type_सम्बन्धि(type)];

_       (EmitOp(o, op));
        Emit_Slot_विस्थापनम्(o, Get_राशिः_शिखरम्_Slot_सङ्ख्या(o));

        यदि (i_update_राशिः) {
_           (निष्कासय_Type(o, type));
_           (प्रक्षिप_पञ्जिकाकोशः(o, type));
        }
    }

    _catch: प्रत्यागम परिणाम;
}


// if local is unreferenced, o_preservedSlotNumber will be equal to localIndex on return
स्थिर
M3_परिणाम Find_Referenced_Local_Within_Current_खण्डम् (IM3Compilation o, u16* o_preserved_Slot_सङ्ख्या, u32 i_localSlot)
{
    M3_परिणाम परिणाम = m3Err_none;

    IM3CompilationScope scope = &o->खण्डम्;

    u16 start_सूचकांक = scope->खण्डम्_राशिः_सूचकांक;

    यावत् (scope->opcode == c_wa_Op_खण्डम्) {
        scope = scope->outer;
        यदि (not scope) {
            भङ्ग;
        }

        start_सूचकांक = scope->खण्डम्_राशिः_सूचकांक;
    }

    *o_preserved_Slot_सङ्ख्या = (u16)i_localSlot;

    प्रति (u32 i = start_सूचकांक; i < o->राशिः_सूचकांक; ++i) {
        यदि (o->wasm_राशिः[i] == i_localSlot) {
            m3type_t type = Get_राशिः_Type_From_Bottom(o, i);                         d_m3Assert (type != c_m3Type_none)

            यदि (*o_preserved_Slot_सङ्ख्या == i_localSlot) {
_               (AllocateSlots(o, o_preserved_Slot_सङ्ख्या, type));
            } अन्यथा {
                // A 64-bit value spans two slots, and allocating or releasing it
                // counts both, so a second reference has to count both as well.
                प्रति (u16 s = 0; s < Get_Type_अङ्कः_Slots(type); ++s) {
_                   (Increment_Slot_Usage_गणनम्(o, (u16)(*o_preserved_Slot_सङ्ख्या + s)));
                }
            }

            o->wasm_राशिः[i] = *o_preserved_Slot_सङ्ख्या;
        }
    }

    _catch: प्रत्यागम परिणाम;
}

स्थिर
M3_परिणाम Get_खण्डम्_Scope (IM3Compilation o, IM3CompilationScope* o_scope, u32 i_गभीरता)
{
    M3_परिणाम परिणाम = m3Err_none;

    IM3CompilationScope scope = &o->खण्डम्;

    यावत् (i_गभीरता--) {
        scope = scope->outer;
        _throwif("invalid block depth", not scope);
    }

    *o_scope = scope;

    _catch:
    प्रत्यागम परिणाम;
}

स्थिर
M3_परिणाम Copy_राशिः_Slots_R (IM3Compilation o, u16 i_target_Slot_राशिः_सूचकांक, u16 i_राशिः_सूचकांक, u16 i_अन्तः_राशिः_सूचकांक, u16 i_अस्थायी_Slot)
{
    M3_परिणाम परिणाम = m3Err_none;

    यदि (i_राशिः_सूचकांक < i_अन्तः_राशिः_सूचकांक) {
        u16 srcSlot = Get_Slot_For_राशिः_सूचकांक(o, i_राशिः_सूचकांक);

        m3type_t type      = Get_राशिः_Type_From_Bottom(o, i_राशिः_सूचकांक);
        u16      अङ्कः_Slots  = Get_Type_अङ्कः_Slots(type);
        u16      extraSlot = अङ्कः_Slots - 1;

        u16 targetSlot = Get_Slot_For_राशिः_सूचकांक(o, i_target_Slot_राशिः_सूचकांक);

        u16 preserve_सूचकांक = i_राशिः_सूचकांक;
        u16 collisionSlot = srcSlot;

        यदि (targetSlot != srcSlot) {
            // search for collisions
            u16 check_सूचकांक = i_राशिः_सूचकांक + 1;
            यावत् (check_सूचकांक < i_अन्तः_राशिः_सूचकांक) {
                u16 otherSlot1 = Get_Slot_For_राशिः_सूचकांक(o, check_सूचकांक);
                u16 otherSlot2 = Get_Extra_Slot_For_राशिः_सूचकांक(o, check_सूचकांक);

                यदि (targetSlot == otherSlot1 or
                    targetSlot == otherSlot2 or
                    targetSlot + extraSlot == otherSlot1) {
                    _throwif(m3_Err_function_राशिः_अतिप्रवाहः, i_अस्थायी_Slot >= d_m3MaxFunctionSlots);

_                   (Copy_राशिः_सूचकांक_गन्तव्यम्_Slot(o, i_अस्थायी_Slot, check_सूचकांक));
                    o->wasm_राशिः[check_सूचकांक] = i_अस्थायी_Slot;
                    i_अस्थायी_Slot += Get_Type_अङ्कः_Slots(c_m3Type_i64);
                    TouchSlot(o, i_अस्थायी_Slot - 1);

                    // restore this on the way back down
                    preserve_सूचकांक = check_सूचकांक;
                    collisionSlot = otherSlot1;

                    भङ्ग;
                }

                ++check_सूचकांक;
            }

_           (Copy_राशिः_सूचकांक_गन्तव्यम्_Slot(o, targetSlot, i_राशिः_सूचकांक));                                                m3log (compile, " copying slot: %d to slot: %d", srcSlot, targetSlot);
            o->wasm_राशिः[i_राशिः_सूचकांक] = targetSlot;
        }

_       (Copy_राशिः_Slots_R(o, i_target_Slot_राशिः_सूचकांक + 1, i_राशिः_सूचकांक + 1, i_अन्तः_राशिः_सूचकांक, i_अस्थायी_Slot));

        // restore the stack state
        o->wasm_राशिः[i_राशिः_सूचकांक]  = srcSlot;
        o->wasm_राशिः[preserve_सूचकांक] = collisionSlot;
    }

    _catch:
    प्रत्यागम परिणाम;
}

स्थिर
M3_परिणाम Resolve_खण्डम्_Results (IM3Compilation o, IM3CompilationScope i_target_खण्डम्, bool i_isBranch)
{
    M3_परिणाम परिणाम = m3Err_none;
    यदि (d_m3_Log_Wasm_राशिः) {
        dump_type_राशिः(o);
    }

    bool isLoop = (i_target_खण्डम्->opcode == c_waOp_loop and i_isBranch);

    u16 अङ्कः_Params  = Get_Func_Type_अङ्कः_Params(i_target_खण्डम्->type);
    u16 अङ्कः_Results = Get_Func_Type_अङ्कः_Results(i_target_खण्डम्->type);

    u16 slot_लेखाः = i_target_खण्डम्->exit_राशिः_सूचकांक;

    u16 अङ्कः_मानानि;

    यदि (not isLoop) {
        अङ्कः_मानानि = अङ्कः_Results;
        slot_लेखाः += अङ्कः_Params;
    } अन्यथा {
        अङ्कः_मानानि = अङ्कः_Params;
    }

    u16 खण्डम्_Height = Get_अङ्कः_खण्डम्_मानानि_On_राशिः(o);

    _throwif(m3_Err_type_गणनम्_Mismatch, i_isBranch ? (खण्डम्_Height < अङ्कः_मानानि) : (खण्डम्_Height != अङ्कः_मानानि));

    यदि (अङ्कः_मानानि) {
        u16 अन्तः_सूचकांक     = Get_राशिः_शिखरम्_सूचकांक(o) + 1;
        u16 अङ्कः_Rem_मानानि = अङ्कः_मानानि;

        // The last result is taken from _fp0. See PushBlockResults.
        यदि (not isLoop and IsFpType(Get_राशिः_शिखरम्_Type(o))) {
_           (Copy_राशिः_शिखरम्_गन्तव्यम्_पञ्जिकाकोशः(o, false));
            --अन्तः_सूचकांक;
            --अङ्कः_Rem_मानानि;
        }

        // tempslot affects maxStackSlots, so can grow unnecess each time.
        // in practice, this is hardly ever hit.
        u16 अस्थायी_Slot = o->max_राशिः_Slots;// GetMaxUsedSlotPlusOne (o); doesn't work cause can collide with slotRecords
        Align_Slot_गन्तव्यम्_Type(&अस्थायी_Slot, c_m3Type_i64);

_       (Copy_राशिः_Slots_R(o, slot_लेखाः, अन्तः_सूचकांक - अङ्कः_Rem_मानानि, अन्तः_सूचकांक, अस्थायी_Slot));

        यदि (d_m3_Log_Wasm_राशिः) {
            dump_type_राशिः(o);
        }
    }

    _catch: प्रत्यागम परिणाम;
}


स्थिर
M3_परिणाम Return_मानानि (IM3Compilation o, IM3CompilationScope i_function_खण्डम्, bool i_isBranch)
{
    M3_परिणाम परिणाम = m3Err_none;
    यदि (d_m3_Log_Wasm_राशिः) {
        dump_type_राशिः(o);
    }

    u16 अङ्कः_Returns  = Get_Func_Type_अङ्कः_Results(i_function_खण्डम्->type);     // could just o->function too...
    u16 खण्डम्_Height = Get_अङ्कः_खण्डम्_मानानि_On_राशिः(o);

    यदि (not Is_राशिः_Polymorphic(o)) {
        _throwif(m3_Err_type_गणनम्_Mismatch, i_isBranch ? (खण्डम्_Height < अङ्कः_Returns) : (खण्डम्_Height != अङ्कः_Returns));
    }

    यदि (अङ्कः_Returns) {
        // return slots like args are 64-bit aligned
        u16 returnSlot = अङ्कः_Returns * c_io_Slot_गणनम्;
        u16 राशिः_शिखरम्   = Get_राशिः_शिखरम्_सूचकांक(o);

        प्रति (u16 i = 0; i < अङ्कः_Returns; ++i) {
            m3type_t returnType = Get_Func_Type_परिणाम_Type(i_function_खण्डम्->type, अङ्कः_Returns - 1 - i);

            m3type_t राशिः_Type = Get_राशिः_Type_From_शिखरम्(o, i);  // using FromTop so that only dynamic items are checked

            यदि (Is_राशिः_Polymorphic(o) and राशिः_Type == c_m3Type_none) {
                राशिः_Type = returnType;
            }

            _throwif(m3Err_typeMismatch, not Is_Sub_Type_सम्बन्धि(राशिः_Type, returnType));

            यदि (not Is_राशिः_Polymorphic(o)) {
                returnSlot -= c_io_Slot_गणनम्;
_               (Copy_राशिः_सूचकांक_गन्तव्यम्_Slot(o, returnSlot, राशिः_शिखरम्--));
            }
        }

        यदि (not i_isBranch) {
            यावत् (अङ्कः_Returns--) {
_               (निष्कासय(o));
            }
        }
    }

    _catch: प्रत्यागम परिणाम;
}


//-------------------------------------------------------------------------------------------------------------------------

स्थिर
M3_परिणाम Compile_Const_i32 (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_परिणाम परिणाम;

    i32 मानम्;
_   (ReadLEB_i32(&मानम्, &o->wasm, o->wasm_अन्तः));
_   (प्रक्षिप_Const(o, मानम्, c_m3Type_i32));                        m3log (compile, d_indent " (const i32 = %" PRIi32 ")", get_indention_string (o), मानम्);
    _catch: प्रत्यागम परिणाम;
}

स्थिर
M3_परिणाम Compile_Const_i64 (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_परिणाम परिणाम;

    i64 मानम्;
_   (ReadLEB_i64(&मानम्, &o->wasm, o->wasm_अन्तः));
_   (प्रक्षिप_Const(o, मानम्, c_m3Type_i64));                        m3log (compile, d_indent " (const i64 = %" PRIi64 ")", get_indention_string (o), मानम्);
    _catch: प्रत्यागम परिणाम;
}


#यदि d_m3ImplementFloat
स्थिर
M3_परिणाम Compile_Const_f32 (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_परिणाम परिणाम;

    सङ्घ {
        u32 u;
        f32 f;
    } मानम् = { 0 };

_   (Read_f32(&मानम्.f, &o->wasm, o->wasm_अन्तः));                 m3log (compile, d_indent " (const f32 = %" PRIf32 ")", get_indention_string (o), मानम्.f);
_   (प्रक्षिप_Const(o, मानम्.u, c_m3Type_f32));

    _catch: प्रत्यागम परिणाम;
}

स्थिर
M3_परिणाम Compile_Const_f64 (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_परिणाम परिणाम;

    सङ्घ {
        u64 u;
        f64 f;
    } मानम् = { 0 };

_   (Read_f64(&मानम्.f, &o->wasm, o->wasm_अन्तः));                 m3log (compile, d_indent " (const f64 = %" PRIf64 ")", get_indention_string (o), मानम्.f);
_   (प्रक्षिप_Const(o, मानम्.u, c_m3Type_f64));

    _catch: प्रत्यागम परिणाम;
}
#यदि_अन्तः

स्थिर
M3_परिणाम Compile_ExtendedOpcode (IM3Compilation o, m3opcode_t i_opcode)
{
_try {
    u32 opcode;
_   (ReadLEB_u32(&opcode, &o->wasm, o->wasm_अन्तः));               m3log (compile, d_indent " (%s: %" PRIu32 ")", get_indention_string (o), (i_opcode == c_waOp_atomic) ? "FE" : "FC", opcode);

    // m3opcode_t carries only the low byte, so a wider sub-opcode is unknown
    // here rather than something to truncate into range
    _throwif(m3Err_unknownOpcode, opcode > (u32)((i_opcode == c_waOp_atomic) ? c_wa_Op_चरमः_Atomic : c_wa_Op_चरमः_Extended));

    i_opcode = (m3opcode_t)((i_opcode << 8) | opcode);

    //printf("Extended opcode: 0x%x\n", i_opcode);

    IM3OpInfo opInfo = GetOpInfo(i_opcode);
    _throwif(m3Err_unknownOpcode, not opInfo);

    M3Compiler compiler = GetCompiler(opInfo);
    _throwif(m3Err_noCompiler, not compiler);

_   ((*compiler)(o, i_opcode));

    o->previousOpcode = i_opcode;

    } _catch: प्रत्यागम परिणाम;
}

स्थिर
M3_परिणाम Compile_Return (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_परिणाम परिणाम = m3Err_none;

    यदि (not Is_राशिः_Polymorphic(o)) {
        IM3CompilationScope functionScope;
_       (Get_खण्डम्_Scope(o, &functionScope, o->खण्डम्.गभीरता));

_       (Return_मानानि(o, functionScope, true));

_       (EmitOp(o, op_Return));

_       (समुच्चयः_राशिः_Polymorphic(o));
    }

    _catch: प्रत्यागम परिणाम;
}

स्थिर
M3_परिणाम Validate_खण्डम्_अन्तः (IM3Compilation o)
{
    M3_परिणाम परिणाम = m3Err_none;

    u16 अङ्कः_Results  = Get_Func_Type_अङ्कः_Results(o->खण्डम्.type);
    u16 खण्डम्_Height = Get_अङ्कः_खण्डम्_मानानि_On_राशिः(o);

    यदि (not Is_राशिः_Polymorphic(o)) {
        // Spec: at block end, stack height must match the number of results
        _throwif(m3_Err_type_गणनम्_Mismatch, खण्डम्_Height != अङ्कः_Results);

        // Spec: result types must match expected types
        प्रति (u16 i = 0; i < अङ्कः_Results; ++i) {
            m3type_t expectedType = Get_Func_Type_परिणाम_Type(o->खण्डम्.type, अङ्कः_Results - 1 - i);
            m3type_t actualType   = Get_राशिः_Type_From_शिखरम्(o, i);
            _throwif(m3Err_typeMismatch, not Is_Sub_Type_सम्बन्धि(actualType, expectedType));
        }
    }

    _catch: प्रत्यागम परिणाम;
}

स्थिर
M3_परिणाम Compile_अन्तः (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_परिणाम परिणाम = m3Err_none;                   //dump_type_stack (o);

    // function end:
    यदि (o->खण्डम्.गभीरता == 0) {
        Validate_खण्डम्_अन्तः(o);

        //      if (not IsStackPolymorphic (o))
        {
            // A constant expression has no function frame, but its value still
            // has to be copied down to slot 0, where EvaluateExpression reads it
            // back. The non-emitting init-expr walk is skipped: it has no block
            // type to return against.
            यदि (o->function or o->page) {
_               (Return_मानानि(o, &o->खण्डम्, false));
            }

_           (EmitOp(o, op_Return));
        }
    }

    _catch: प्रत्यागम परिणाम;
}


स्थिर
M3_परिणाम Compile_समुच्चयः_Local (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_परिणाम परिणाम;

    u32 local_सूचकांक;
_   (ReadLEB_u32(&local_सूचकांक, &o->wasm, o->wasm_अन्तः));                           m3log (compile, d_indent " (index = %u)", get_indention_string (o), local_सूचकांक);

    यदि (local_सूचकांक < Get_Function_अङ्कः_तर्काः_And_Locals(o->function)) {
        // Spec: value type must match local type
        यदि (not Is_राशिः_Polymorphic(o)) {
            m3type_t localType    = Get_राशिः_Type_From_Bottom(o, local_सूचकांक);
            m3type_t राशिः_शिखरम्_Type = Get_राशिः_शिखरम्_Type(o);
            _throwif(m3Err_typeMismatch, राशिः_शिखरम्_Type != c_m3Type_none and localType != c_m3Type_none and
                                           not Is_Sub_Type_सम्बन्धि(राशिः_शिखरम्_Type, localType));
#यदि d_m3HasSnapshots
            यदि (o->local_Init_गभीरता and o->local_Init_गभीरता[local_सूचकांक] == UINT32_MAX) {
                o->local_Init_गभीरता[local_सूचकांक] = (u32)o->खण्डम्.गभीरता;
            }
#यदि_अन्तः
        }

        u16 localSlot = Get_Slot_For_राशिः_सूचकांक(o, local_सूचकांक);

        u16 preserveSlot;
_       (Find_Referenced_Local_Within_Current_खण्डम्(o, &preserveSlot, localSlot));  // preserve will be different than local, if referenced

        bool folded = false;

#यदि d_m3_Fold_समुच्चयः_Local
        // destination folding: when the value was produced by the immediately preceding op (nothing
        // emitted since, result on top in _r0, no copy-on-write preservation needed), retro-patch the
        // producer to a variant that writes straight to the local's slot and skip the SetSlot op
        यदि (o->foldPatchPC and preserveSlot == localSlot and Is_राशिः_शिखरम्_अन्तर्_पञ्जिकाकोशः(o) and Get_राशिः_शिखरम्_सूचकांक(o) == (i16)o->fold_राशिः_सूचकांक) {
            IM3Operation foldOp = GetFoldOp(o->foldOpcode, o->foldForm);

            यदि (foldOp) {
                *(IM3Operation*)o->foldPatchPC = foldOp;     // retro-patch the producer
                Emit_Slot_विस्थापनम्(o, localSlot);                  // append its destination
                InvalidateFold(o);
                folded = true;
            }
        }
#यदि_अन्तः

        यदि (folded) {
            m3type_t type = Get_राशिः_शिखरम्_Type(o);
_           (निष्कासय(o));                              // the value now lives in the local's slot

            // A folded op writes its result through to the register as well, so a tee
            // can keep it there. Pushing the slot instead would make the next use that
            // wants a register reload the slot this op just stored to.
            यदि (i_opcode == c_waOp_teeLocal) {
_               (प्रक्षिप_पञ्जिकाकोशः(o, type));
            }
        } अन्यथा {
            यदि (preserveSlot == localSlot) {
_               (Copy_राशिः_शिखरम्_गन्तव्यम्_Slot(o, localSlot))
            } अन्यथा {
_               (Preserved_Copy_शिखरम्_Slot(o, localSlot, preserveSlot))
            }

            यदि (i_opcode != c_waOp_teeLocal) {
_               (निष्कासय(o));
            }
        }
    } अन्यथा {
        _throw("local index out of bounds");
    }

    _catch: प्रत्यागम परिणाम;
}

स्थिर
M3_परिणाम Compile_GetLocal (IM3Compilation o, m3opcode_t i_opcode)
{
_try {

    u32 local_सूचकांक;
_   (ReadLEB_u32(&local_सूचकांक, &o->wasm, o->wasm_अन्तः));                           m3log (compile, d_indent " (index = %u)", get_indention_string (o), local_सूचकांक);

    यदि (local_सूचकांक >= Get_Function_अङ्कः_तर्काः_And_Locals(o->function)) {
        _throw("local index out of bounds");
    }

    m3type_t type = Get_राशिः_Type_From_Bottom(o, local_सूचकांक);
    u16      slot = Get_Slot_For_राशिः_सूचकांक(o, local_सूचकांक);

_   (प्रक्षिप(o, type, slot));

    } _catch: प्रत्यागम परिणाम;
}

स्थिर
M3_परिणाम Compile_GetGlobal (IM3Compilation o, M3Global* i_global)
{
    M3_परिणाम परिणाम;

    IM3Operation op = Is64BitType(i_global->type) ? op_GetGlobal_s64 : op_GetGlobal_s32;
_   (EmitOp(o, op));
    EmitPointer(o, &i_global->i64_मानम्);
_   (प्रक्षिप_Allocated_Slot_And_Emit_2(o, i_global->type));

    _catch: प्रत्यागम परिणाम;
}

स्थिर
M3_परिणाम Compile_समुच्चयः_Global (IM3Compilation o, M3Global* i_global)
{
    M3_परिणाम परिणाम = m3Err_none;

    यदि (i_global->isMutable) {
        // Spec: value type must match global type
        यदि (not Is_राशिः_Polymorphic(o)) {
            m3type_t राशिः_शिखरम्_Type = Get_राशिः_शिखरम्_Type(o);
            _throwif(m3Err_typeMismatch, राशिः_शिखरम्_Type != c_m3Type_none and
                                           not Is_Sub_Type_सम्बन्धि(राशिः_शिखरम्_Type, i_global->type));
        }

        IM3Operation op;
        m3type_t     type = Get_राशिः_शिखरम्_Type(o);

        यदि (Is_राशिः_शिखरम्_अन्तर्_पञ्जिकाकोशः(o)) {
            op = c_समुच्चयः_Global_Ops[आधारः_Type_सम्बन्धि(type)];
        } अन्यथा {
            op = Is64BitType(type) ? op_समुच्चयः_Global_s64 : op_समुच्चयः_Global_s32;
        }

_       (EmitOp(o, op));
        EmitPointer(o, &i_global->i64_मानम्);

        यदि (Is_राशिः_शिखरम्_अन्तर्_Slot(o)) {
            Emit_Slot_विस्थापनम्(o, Get_राशिः_शिखरम्_Slot_सङ्ख्या(o));
        }

_       (निष्कासय(o));
    } अन्यथा {
        _throw(m3Err_settingImmutableGlobal);
    }

    _catch: प्रत्यागम परिणाम;
}

स्थिर
M3_परिणाम Compile_Get_समुच्चयः_Global (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_परिणाम परिणाम = m3Err_none;

    u32 global_सूचकांक;
_   (ReadLEB_u32(&global_सूचकांक, &o->wasm, o->wasm_अन्तः));

    यदि (global_सूचकांक < o->module->अङ्कः_Globals) {
        यदि (o->module->globals) {
            M3Global* global = &o->module->globals[global_सूचकांक];

            // Spec: a constant expression may only read an imported immutable
            // global. The module's own globals are counted before their
            // initializer is walked, so a bare index check would let one
            // reference itself. These describe how the module declared the
            // global, so they are checked before following any link.
            _throwif(m3_Err_globa_सूचकांक_बहिर्गमनम्_सम्बन्धि_Bounds, o->is_Init_व्यञ्जकम् and not global->imported);
            _throwif(m3Err_wasmMalformed, o->is_Init_व्यञ्जकम् and global->isMutable);

            // an import linked to another module reads and writes that module's
            // cell, not the placeholder standing in for it here
            यदि (global->resolved) {
                global = global->resolved;
            }

_           ((i_opcode == c_waOp_getGlobal) ? Compile_GetGlobal(o, global) : Compile_समुच्चयः_Global(o, global));
        } अन्यथा {
            _throw(ErrorCompile(m3_Err_global_स्मृतिः_Not_Allocated, o, "module '%s' is missing global memory", o->module->name));
        }
    } अन्यथा {
        _throw(m3_Err_globa_सूचकांक_बहिर्गमनम्_सम्बन्धि_Bounds);
    }

    _catch: प्रत्यागम परिणाम;
}

स्थिर
शून्य EmitPatchingBranchPointer (IM3Compilation o, IM3CompilationScope i_scope)
{
    pc_t patch       = EmitPointer(o, i_scope->patches);                m3log (compile, "branch patch required at: %p", patch);
    i_scope->patches = patch;
}

स्थिर
M3_परिणाम EmitPatchingBranch (IM3Compilation o, IM3CompilationScope i_scope)
{
    M3_परिणाम परिणाम = m3Err_none;

_   (EmitOp(o, op_Branch));
    EmitPatchingBranchPointer(o, i_scope);

    _catch: प्रत्यागम परिणाम;
}

#यदि d_m3HasExceptionHandling

// How many try_table handler records an exit from i_from out to i_target takes
// down with it. Walking outward from the block the branch sits in, every
// try_table scope up to and including the target is left behind - the target
// too, because landing on a block's label means that block is finished.
//
// A loop label is the exception, but a loop is never a try_table, so the count
// is the same either way: the loop itself is not on the list.
स्थिर
u32 गणनम्_Try_चौकटाः_गन्तव्यम्_Exit (IM3CompilationScope i_from, IM3CompilationScope i_target)
{
    u32 गणनम् = 0;

    प्रति (IM3CompilationScope scope = i_from; scope; scope = scope->outer) {
        यदि (scope->opcode == c_wa_Op_try_सारणी) {
            ++गणनम्;
        }

        यदि (scope == i_target) {
            भङ्ग;
        }
    }

    प्रत्यागम गणनम्;
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
स्थिर
u32 अङ्कः_Try_चौकटाः_गन्तव्यम्_निष्कासय (IM3CompilationScope i_from, IM3CompilationScope i_target)
{
    यदि (i_target->गभीरता == 0) {
        प्रत्यागम 0;
    }

    // the walk only ever runs inward-to-outward; a target that is already
    // outside i_from has nothing between them
    यदि (not i_from or i_from->गभीरता < i_target->गभीरता) {
        प्रत्यागम 0;
    }

    प्रत्यागम गणनम्_Try_चौकटाः_गन्तव्यम्_Exit(i_from, i_target);
}


स्थिर
M3_परिणाम Emit_निष्कासय_Try_चौकटाः (IM3Compilation o, u32 i_अङ्कः_चौकटाः)
{
    M3_परिणाम परिणाम = m3Err_none;

    यदि (i_अङ्कः_चौकटाः) {
_       (EmitOp(o, op_निष्कासय_Handlers));
        EmitConstant32(o, i_अङ्कः_चौकटाः);
    }

    _catch: प्रत्यागम परिणाम;
}


// A return_call leaves the enclosing function before the callee runs, so no
// try_table in this function may catch what the callee throws. Neither shape of
// the instruction unwinds the native stack in time to arrange that on its own:
// op_ReturnCall tail-jumps with op_TryTable's frames still standing, and the
// fallback shape is an ordinary call sitting inside them. Either way the
// handler records have to come off first.
स्थिर
M3_परिणाम Emit_निष्कासय_Try_चौकटाः_For_Return_Call (IM3Compilation o)
{
    u32 गणनम् = 0;

    प्रति (IM3CompilationScope scope = &o->खण्डम्; scope; scope = scope->outer) {
        यदि (scope->opcode == c_wa_Op_try_सारणी) {
            ++गणनम्;
        }
    }

    प्रत्यागम Emit_निष्कासय_Try_चौकटाः(o, गणनम्);
}

#अन्यथा

स्थिर
M3_परिणाम Emit_निष्कासय_Try_चौकटाः (IM3Compilation o, u32 i_अङ्कः_चौकटाः)
{
    (शून्य)o;
    (शून्य)i_अङ्कः_चौकटाः;
    प्रत्यागम m3Err_none;
}

#यदि_अन्तः // d_m3HasExceptionHandling

#यदि d_m3FuseBranch
// when the pending fold candidate is a compare whose result sits on top in _r0, return the fused
// variant that absorbs an immediately following br_if (i_isIf false) or if (i_isIf true)
स्थिर
IM3Operation GetBranchFusionOp (IM3Compilation o, bool i_isIf)
{
    यदि (o->foldPatchPC and o->foldForm < 4 and Is_राशिः_शिखरम्_अन्तर्_पञ्जिकाकोशः(o) and Get_राशिः_शिखरम्_सूचकांक(o) == (i16)o->fold_राशिः_सूचकांक) {
        ध्रुव M3FusedCmpOps* ops = GetFusedCmpOps(o->foldOpcode);

        यदि (ops) {
            प्रत्यागम i_isIf ? ops->ifOp[o->foldForm] : ops->branchIf[o->foldForm];
        }
    }

    प्रत्यागम रिक्त;
}
#यदि_अन्तः // d_m3FuseBranch

स्थिर अन्तःस्थ
IM3Operation Op_ContinueLoop (IM3Compilation o)
{
#यदि d_m3_Has_राशिः_Switching
    यदि (M3_UNLIKELY(o->module->runtime and o->module->runtime->isSuspendable)) {
        प्रत्यागम op_ContinueLoop_Suspendable;
    }
#यदि_अन्तः
    प्रत्यागम op_ContinueLoop;
}

स्थिर अन्तःस्थ
IM3Operation Op_ContinueLoopIf (IM3Compilation o)
{
#यदि d_m3_Has_राशिः_Switching
    यदि (M3_UNLIKELY(o->module->runtime and o->module->runtime->isSuspendable)) {
        प्रत्यागम op_ContinueLoopIf_Suspendable;
    }
#यदि_अन्तः
    प्रत्यागम op_ContinueLoopIf;
}


स्थिर
M3_परिणाम Compile_Branch (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_परिणाम परिणाम;
    u32      अङ्कः_Try_चौकटाः = 0;

    u32 गभीरता;
_   (ReadLEB_u32(&गभीरता, &o->wasm, o->wasm_अन्तः));

    // Spec: br_if condition must be i32
    यदि (i_opcode == c_waOp_branchIf and not Is_राशिः_Polymorphic(o)) {
        m3type_t condType = Get_राशिः_शिखरम्_Type(o);
        _throwif(m3Err_typeMismatch, condType != c_m3Type_none and condType != c_m3Type_i32);
    }

    IM3CompilationScope scope;
_   (Get_खण्डम्_Scope(o, &scope, गभीरता));

#यदि d_m3HasExceptionHandling
    अङ्कः_Try_चौकटाः = अङ्कः_Try_चौकटाः_गन्तव्यम्_निष्कासय(&o->खण्डम्, scope);
#यदि_अन्तः

    // branch target is a loop (continue)
    यदि (scope->opcode == c_waOp_loop) {
        यदि (i_opcode == c_waOp_branchIf) {
            // a handler pop belongs on the taken path only, so it forces the
            // jump-over form even where the target takes no parameters
            यदि (Get_Func_Type_अङ्कः_Params(scope->type) or अङ्कः_Try_चौकटाः) {
                IM3Operation op = Is_राशिः_शिखरम्_अन्तर्_पञ्जिकाकोशः(o) ? op_BranchIfPrologue_r : op_BranchIfPrologue_s;

_               (EmitOp(o, op));
_               (Emit_Slot_अङ्कः_सम्बन्धि_राशिः_शिखरम्_And_निष्कासय(o));

                pc_t* jump_गन्तव्यम् = (pc_t*)ReservePointer(o);

_               (Emit_निष्कासय_Try_चौकटाः(o, अङ्कः_Try_चौकटाः));

                यदि (not Is_राशिः_Polymorphic(o)) {
_                   (Resolve_खण्डम्_Results(o, scope, /* isBranch: */ true));
                }

_               (EmitOp(o, Op_ContinueLoop(o)));
                EmitPointer(o, scope->pc);
_               (लेखः_पृष्ठभागः_Edge(o, scope, false));

                *jump_गन्तव्यम् = GetPC(o);
            } अन्यथा {
                // move the condition to a register
_               (Copy_राशिः_शिखरम्_गन्तव्यम्_पञ्जिकाकोशः(o, false));
_               (निष्कासय_Type(o, c_m3Type_i32));

_               (EmitOp(o, Op_ContinueLoopIf(o)));
                EmitPointer(o, scope->pc);
_               (लेखः_पृष्ठभागः_Edge(o, scope, true));
            }

            //          dump_type_stack(o);
        } अन्यथा // is c_waOp_branch
        {
_           (Emit_निष्कासय_Try_चौकटाः(o, अङ्कः_Try_चौकटाः));

            // the branch operands become the loop's parameters on the next iteration
            यदि (not Is_राशिः_Polymorphic(o)) {
_               (Resolve_खण्डम्_Results(o, scope, /* isBranch: */ true));
            }

_           (EmitOp(o, Op_ContinueLoop(o)));
            EmitPointer(o, scope->pc);
_           (लेखः_पृष्ठभागः_Edge(o, scope, false));
            o->खण्डम्.isPolymorphic = true;
        }
    } अन्यथा // forward branch
    {
        pc_t* jump_गन्तव्यम् = रिक्त;

        bool isReturn         = (scope->गभीरता == 0);
        bool targetHasResults = Get_Func_Type_अङ्कः_Results(scope->type);

        यदि (i_opcode == c_waOp_branchIf) {
            यदि (targetHasResults or isReturn or अङ्कः_Try_चौकटाः) {
                IM3Operation op = Is_राशिः_शिखरम्_अन्तर्_पञ्जिकाकोशः(o) ? op_BranchIfPrologue_r : op_BranchIfPrologue_s;

_               (EmitOp(o, op));
_               (Emit_Slot_अङ्कः_सम्बन्धि_राशिः_शिखरम्_And_निष्कासय(o)); // condition

                // this is continuation point, if the branch isn't taken
                jump_गन्तव्यम् = (pc_t*)ReservePointer(o);
            } अन्यथा {
#यदि d_m3FuseBranch
                IM3Operation fuseOp = GetBranchFusionOp(o, false);

                यदि (fuseOp) {
                    // absorb the branch into the compare that produced the condition
                    *(IM3Operation*)o->foldPatchPC = fuseOp;
                    InvalidateFold(o);

_                   (निष्कासय(o));                      // the condition is consumed inside the fused op

                    EmitPatchingBranchPointer(o, scope);
                    गच्छ _catch;
                }
#यदि_अन्तः
                IM3Operation op = Is_राशिः_शिखरम्_अन्तर्_पञ्जिकाकोशः(o) ? op_BranchIf_r : op_BranchIf_s;

_               (EmitOp(o, op));
_               (Emit_Slot_अङ्कः_सम्बन्धि_राशिः_शिखरम्_And_निष्कासय(o)); // condition

                EmitPatchingBranchPointer(o, scope);
                गच्छ _catch;
            }
        }

        यदि (not Is_राशिः_Polymorphic(o)) {
            यदि (isReturn) {
_               (Return_मानानि(o, scope, true));
_               (EmitOp(o, op_Return));
            } अन्यथा {
_               (Emit_निष्कासय_Try_चौकटाः(o, अङ्कः_Try_चौकटाः));

_               (Resolve_खण्डम्_Results(o, scope, true));
_               (EmitPatchingBranch(o, scope));
            }
        }

        यदि (jump_गन्तव्यम्) {
            *jump_गन्तव्यम् = GetPC(o);
        }

        यदि (i_opcode == c_waOp_branch) {
_           (समुच्चयः_राशिः_Polymorphic(o));
        }
    }

    _catch: प्रत्यागम परिणाम;
}

स्थिर
M3_परिणाम Compile_Branch_सारणी (IM3Compilation o, m3opcode_t i_opcode)
{
    // the page a continue-op page is standing in for; non-NULL only while that
    // page is installed in o->page, so the catch below knows to put it back
    IM3_संहिता_Page displacedPage = रिक्त;
    // indexed by label depth: the continuation stub already emitted for that target
    // within this table, or NULL if this table has not reached it yet
    pc_t* targetStubs = रिक्त;
_try {
    u32 target_गणनम्;
_   (ReadLEB_u32(&target_गणनम्, &o->wasm, o->wasm_अन्तः));

    // Spec: validate that the branch index operand is i32
    यदि (not Is_राशिः_Polymorphic(o)) {
        m3type_t सूचकांक_Type = Get_राशिः_शिखरम्_Type(o);
        _throwif(m3Err_typeMismatch, सूचकांक_Type != c_m3Type_none and सूचकांक_Type != c_m3Type_i32);
    }

_   (Preserve_पञ्जिकाकोशः_If_Occupied(o, c_m3Type_i64));         // move branch operand to a slot
    u16 slot = Get_राशिः_शिखरम्_Slot_सङ्ख्या(o);
_   (निष्कासय(o));

    // OPTZ: according to spec: "forward branches that target a control instruction with a non-empty
    // result type consume matching operands first and push them back on the operand stack after unwinding"
    // So, this move-to-reg is only necessary if the target scopes have a type.

    u32 अङ्कः_संहिता_पङ्क्तयः = target_गणनम् + 4; // 3 => IM3Operation + slot + target_count + default_target
_   (Ensure_संहिता_Page_अङ्कः_पङ्क्तयः(o, अङ्कः_संहिता_पङ्क्तयः));

_   (EmitOp(o, op_Branch_सारणी));
    Emit_Slot_विस्थापनम्(o, slot);
    EmitConstant32(o, target_गणनम्);

    IM3_संहिता_Page continueOpPage = रिक्त;

    // The label types are checked by ValidateFunction, which runs under the same
    // d_m3EnableValidation guard just before this compilation pass. Comparing each
    // target against the default here would be wrong anyway: the spec matches every
    // target against the operand stack, so in unreachable code the operands are the
    // bottom type and the targets may legitimately differ (unreached-valid.wast).

    ++target_गणनम्; // include default

    // Every entry of one br_table is resolved from the same compilation state - each
    // stub restores whatever it touched - so two entries naming the same label emit
    // byte-identical epilogues. Emit one and point both table entries at it. A label
    // deeper than the current block is rejected by GetBlockScope below, which is what
    // bounds this array.
    targetStubs = m3_AllocArray(pc_t, (size_t)o->खण्डम्.गभीरता + 1);
    _throwifnull(targetStubs);

    प्रति (u32 i = 0; i < target_गणनम्; ++i) {
        u32 target;
_       (ReadLEB_u32(&target, &o->wasm, o->wasm_अन्तः));

        IM3CompilationScope scope;
_       (Get_खण्डम्_Scope(o, &scope, target));

        यदि (targetStubs[target]) {
            EmitPointer(o, targetStubs[target]);
            अनुवर्त;
        }

        u32 अङ्कः_Try_चौकटाः = 0;
#यदि d_m3HasExceptionHandling
        अङ्कः_Try_चौकटाः = अङ्कः_Try_चौकटाः_गन्तव्यम्_निष्कासय(&o->खण्डम्, scope);
#यदि_अन्तः

        // A forward branch to a block that carries no results and leaves no try
        // frames behind has nothing to do on the way out but arrive. There is no
        // epilogue to hold, so the table entry becomes the patch site itself and the
        // block's address is written into it when the block ends - no stub, and no
        // code page to put one on. In unreachable code the same is true whatever the
        // block's type, since nothing is emitted for that case either way.
        //
        // Each such entry is patched on its own, so none is recorded in targetStubs:
        // there is no shared stub for a later entry naming this label to point at.
        यदि (scope->opcode != c_waOp_loop and scope->गभीरता > 0 and अङ्कः_Try_चौकटाः == 0 and
            (Is_राशिः_Polymorphic(o) or Get_Func_Type_अङ्कः_Results(scope->type) == 0)) {
            EmitPatchingBranchPointer(o, scope);
            अनुवर्त;
        }

_       (Acquire_Compilation_संहिता_Page(o, &continueOpPage));

        pc_t startPC  = GetPagePC(continueOpPage);
        displacedPage = o->page;
        o->page       = continueOpPage;

        यदि (scope->opcode == c_waOp_loop) {
_           (Emit_निष्कासय_Try_चौकटाः(o, अङ्कः_Try_चौकटाः));

            यदि (not Is_राशिः_Polymorphic(o)) {
_               (Resolve_खण्डम्_Results(o, scope, true));
            }

_           (EmitOp(o, Op_ContinueLoop(o)));
            EmitPointer(o, scope->pc);
_           (लेखः_पृष्ठभागः_Edge(o, scope, false));
        } अन्यथा {
            यदि (not Is_राशिः_Polymorphic(o)) {
                यदि (scope->गभीरता == 0) {
_                   (Return_मानानि(o, scope, true));
_                   (EmitOp(o, op_Return));
                } अन्यथा {
_                   (Emit_निष्कासय_Try_चौकटाः(o, अङ्कः_Try_चौकटाः));

_                   (Resolve_खण्डम्_Results(o, scope, true));

_                   (EmitPatchingBranch(o, scope));
                }
            }
        }

        IM3_संहिता_Page return_गन्तव्यम् = displacedPage;
        displacedPage        = रिक्त;

        Release_Compilation_संहिता_Page(o);
        o->page = return_गन्तव्यम्;

        EmitPointer(o, startPC);
        targetStubs[target] = startPC;
    }

_   (समुच्चयः_राशिः_Polymorphic(o));
} _catch:

    // thrown out of the loop above with a continue-op page installed: release it
    // and restore the one it displaced, which the caller's catch then releases.
    // Otherwise the displaced page is on no list and simply leaks.
    यदि (displacedPage) {
        Release_Compilation_संहिता_Page(o);
        o->page = displacedPage;
    }

    m3_Free(targetStubs);

    प्रत्यागम परिणाम;
}

स्थिर
M3_परिणाम Compile_Call_तर्काः_And_Return (IM3Compilation o, u16* o_राशिः_विस्थापनम्, IM3FuncType i_type, bool i_is_परोक्षम्)
{
_try {

    u16 शिखरम्_Slot = Get_Max_प्रयुक्तम्_Slot_Plus_One(o);

    // force use of at least one stack slot; this is to help ensure
    // the m3 stack overflows (and traps) before the native stack can overflow.
    // e.g. see Wasm spec test 'runaway' in call.wast
    शिखरम्_Slot = M3_MAX(1, शिखरम्_Slot);

    // stack frame is 64-bit aligned
    Align_Slot_गन्तव्यम्_Type(&शिखरम्_Slot, c_m3Type_i64);

    *o_राशिः_विस्थापनम् = शिखरम्_Slot;

    // wait to pop this here so that topSlot search is correct
    यदि (i_is_परोक्षम्) {
_       (निष्कासय(o));
    }

    u16 अङ्कः_तर्काः = Get_Func_Type_अङ्कः_Params(i_type);
    u16 अङ्कः_Rets = Get_Func_Type_अङ्कः_Results(i_type);

    u16 arg_शिखरम् = शिखरम्_Slot + (अङ्कः_तर्काः + अङ्कः_Rets) * c_io_Slot_गणनम्;

    TouchSlot(o, arg_शिखरम् - 1);

    यावत् (अङ्कः_तर्काः--) {
_       (Copy_राशिः_शिखरम्_गन्तव्यम्_Slot(o, arg_शिखरम् -= c_io_Slot_गणनम्));
_       (निष्कासय(o));
    }

    u16 i = 0;
    यावत् (अङ्कः_Rets--) {
        m3type_t type = Get_Func_Type_परिणाम_Type(i_type, i++);

_       (प्रक्षिप(o, type, शिखरम्_Slot));
_       (MarkSlotsAllocatedByType(o, शिखरम्_Slot, type));

        शिखरम्_Slot += c_io_Slot_गणनम्;
    }

    } _catch: प्रत्यागम परिणाम;
}

// A tail call hands the current frame over to the callee instead of stacking a new one.
// The callee's results have to be the enclosing function's results, so both use the same
// return slots and only the arguments move: they're staged above the caller's stack here
// (the argument expressions can still be reading the args/locals they're about to land on)
// and slid down onto the frame base at runtime, by op_ReturnCall[Indirect].
स्थिर
M3_परिणाम Compile_पुच्छ_Call_तर्काः (IM3Compilation o, u16* o_राशिः_विस्थापनम्, u16* o_अङ्कः_Arg_Slots, IM3FuncType i_type, bool i_is_परोक्षम्)
{
_try {
    IM3FuncType funcType = o->function->funcType;

    u16 अङ्कः_Rets = Get_Func_Type_अङ्कः_Results(i_type);
    _throwif(m3Err_typeMismatch, अङ्कः_Rets != Get_Func_Type_अङ्कः_Results(funcType));

    प्रति (u16 i = 0; i < अङ्कः_Rets; ++i) {
        _throwif(m3Err_typeMismatch, Get_Func_Type_परिणाम_Type(i_type, i) != Get_Func_Type_परिणाम_Type(funcType, i));
    }

    u16 शिखरम्_Slot = Get_Max_प्रयुक्तम्_Slot_Plus_One(o);

    शिखरम्_Slot = M3_MAX(1, शिखरम्_Slot);

    // stack frame is 64-bit aligned
    Align_Slot_गन्तव्यम्_Type(&शिखरम्_Slot, c_m3Type_i64);

    *o_राशिः_विस्थापनम् = शिखरम्_Slot;

    // wait to pop this here so that topSlot search is correct
    यदि (i_is_परोक्षम्) {
_       (निष्कासय(o));
    }

    u16 अङ्कः_तर्काः     = Get_Func_Type_अङ्कः_Params(i_type);
    u16 अङ्कः_Arg_Slots = अङ्कः_तर्काः * c_io_Slot_गणनम्;

    *o_अङ्कः_Arg_Slots = अङ्कः_Arg_Slots;

    u16 arg_शिखरम् = शिखरम्_Slot + अङ्कः_Arg_Slots;
    _throwif(m3_Err_function_राशिः_अतिप्रवाहः, arg_शिखरम् > d_m3MaxFunctionSlots);

    // the staging area is written by the caller, so op_Entry has to account for it
    TouchSlot(o, arg_शिखरम् - 1);

    यावत् (अङ्कः_तर्काः--) {
_       (Copy_राशिः_शिखरम्_गन्तव्यम्_Slot(o, arg_शिखरम् -= c_io_Slot_गणनम्));
_       (निष्कासय(o));
    }

    } _catch: प्रत्यागम परिणाम;
}


#यदि d_m3HasTypedRefs

// call_ref $t: the callee is the reference on top of the stack, so the operand
// order is the same as call_indirect's, minus the table.
स्थिर
M3_परिणाम Compile_CallRef (IM3Compilation o, m3opcode_t i_opcode)
{
_try {
    u32 type_सूचकांक;
_   (ReadLEB_u32(&type_सूचकांक, &o->wasm, o->wasm_अन्तः));

    _throwif("function call type index out of range", type_सूचकांक >= o->module->अङ्कः_Func_Types);

    IM3FuncType type = o->module->funcTypes[type_सूचकांक];

    यदि (not Is_राशिः_Polymorphic(o)) {
        // whatever shape the reference has, it must be a function of this type
        m3type_t refType = Get_राशिः_शिखरम्_Type(o);
        _throwif(m3Err_typeMismatch, not Is_Sub_Type_सम्बन्धि(refType, Ref_Type_सम्बन्धि_Func_Type(type, false)));
    }

    यदि (Is_राशिः_शिखरम्_अन्तर्_पञ्जिकाकोशः(o)) {
_       (Preserve_पञ्जिकाकोशः_If_Occupied(o, c_m3Type_funcref));
    }

    u16 functionSlot = Get_राशिः_शिखरम्_Slot_सङ्ख्या(o);

    bool isReturnCall = (i_opcode == c_waOp_returnCallRef);
    bool use_पुच्छ_Call  = isReturnCall and d_m3_Can_पुच्छ_Call;

    u16 exec_शिखरम्, अङ्कः_Arg_Slots = 0;

    यदि (use_पुच्छ_Call) {
_       (Compile_पुच्छ_Call_तर्काः(o, &exec_शिखरम्, &अङ्कः_Arg_Slots, type, true));
    } अन्यथा {
_       (Compile_Call_तर्काः_And_Return(o, &exec_शिखरम्, type, true));
    }

#  यदि d_m3HasExceptionHandling
    यदि (isReturnCall) {
_       (Emit_निष्कासय_Try_चौकटाः_For_Return_Call(o));
    }
#  endif

_   (EmitOp(o, use_पुच्छ_Call ? op_ReturnCallRef : op_CallRef));
    Emit_Slot_विस्थापनम्(o, functionSlot);
    EmitPointer(o, type);
    Emit_Slot_विस्थापनम्(o, exec_शिखरम्);

    यदि (not use_पुच्छ_Call) {
_       (लेखः_Call(o, Get_Func_Type_अङ्कः_Results(type), exec_शिखरम्, isReturnCall));
    }

    यदि (use_पुच्छ_Call) {
        Emit_Slot_विस्थापनम्(o, o->function->अङ्कः_Ret_Slots);
        EmitConstant32(o, अङ्कः_Arg_Slots);

_       (समुच्चयः_राशिः_Polymorphic(o));
    } अन्यथा यदि (isReturnCall) {
_       (Compile_Return(o, i_opcode));
    }

} _catch:
    प्रत्यागम परिणाम;
}


// ref.as_non_null only sharpens the type; the value is unchanged, so the stack
// entry is re-pushed with the null stripped out of its type.
स्थिर
M3_परिणाम Compile_Ref_AsNonNull (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_परिणाम परिणाम = m3Err_none;

    यदि (not Is_राशिः_Polymorphic(o)) {
        m3type_t type = Get_राशिः_शिखरम्_Type(o);
        _throwif(m3Err_typeMismatch, not IsRefType(type));

        // op_RefAsNonNull only knows how to read its operand out of a slot; a
        // value still sitting in the register has to be spilled first, the way
        // every other single-op reader of the stack top does
        यदि (Is_राशिः_शिखरम्_अन्तर्_पञ्जिकाकोशः(o)) {
_           (Preserve_पञ्जिकाकोशः_If_Occupied(o, type));
        }

        u16 slot = Get_राशिः_शिखरम्_Slot_सङ्ख्या(o);

_       (EmitOp(o, op_RefAsNonNull));
        Emit_Slot_विस्थापनम्(o, slot);

        m3type_t nonNull = IsSpelledRefType(type)
                             ? (m3type_t)(type | d_m3Type_refNonNull)
                             : (m3type_t)(d_m3Type_ref | d_m3Type_refNonNull | d_m3Type_heapAbstract |
                                          ((आधारः_Type_सम्बन्धि(type) == c_m3Type_externref) ? d_m3Type_refExtern : 0));

        // the slot keeps the one reference it already had; a Pop()/Push() of
        // the same slot would drop that reference's count without anything
        // to put it back, so the type above it is corrected in place instead
        o->type_राशिः[Get_राशिः_शिखरम्_सूचकांक(o)] = nonNull;
    }

    _catch: प्रत्यागम परिणाम;
}

#यदि_अन्तः // d_m3HasTypedRefs


// How wide a table operand is read at comes from the table it names, so a slot
// holding a narrower type would be read past what was allocated for it. The
// validator refuses that already; this keeps a build without one from reaching
// the interpreter with it. c_m3Type_none means the operand is not on the
// dynamic stack to inspect, which is not this check's business.
स्थिर
M3_परिणाम Check_सङ्कार्यम्_Type (IM3Compilation o, u16 i_गभीरता_From_शिखरम्, m3type_t i_type)
{
    m3type_t actual;

    यदि (Is_राशिः_Polymorphic(o)) {
        प्रत्यागम m3Err_none;
    }

    actual = Get_राशिः_Type_From_शिखरम्(o, i_गभीरता_From_शिखरम्);

    प्रत्यागम (actual == c_m3Type_none or आधारः_Type_सम्बन्धि(actual) == i_type) ? m3Err_none
                                                                     : m3Err_typeMismatch;
}


// Swaps the _mem register onto a given memory. Only ever emitted in pairs --
// see op_SetMemory.
स्थिर
M3_परिणाम Emit_समुच्चयः_स्मृतिः_Ptr (IM3Compilation o, IM3_स्मृतिः i_स्मृतिः)
{
    M3_परिणाम परिणाम = m3Err_none;

_   (EmitOp(o, op_समुच्चयः_स्मृतिः));
    EmitPointer(o, i_स्मृतिः);

    _catch: प्रत्यागम परिणाम;
}


// ...onto one of the compiling module's own memories, by index. Only reached
// with a non-zero index under multi-memory, but the index has already been
// bounds-checked by ReadMemoryIndex either way.
स्थिर अन्तःस्थ
M3_परिणाम Emit_समुच्चयः_स्मृतिः (IM3Compilation o, u32 i_स्मृतिः_Idx)
{
#यदि d_m3_Has_Multi_स्मृतिः
    प्रत्यागम Emit_समुच्चयः_स्मृतिः_Ptr(o, o->module->memories[i_स्मृतिः_Idx]);
#अन्यथा
    (शून्य)o;
    (शून्य)i_स्मृतिः_Idx;
    प्रत्यागम m3_Err_unknown_स्मृतिः;
#यदि_अन्तः
}

स्थिर
M3_परिणाम Compile_Call (IM3Compilation o, m3opcode_t i_opcode)
{
_try {
    u32 function_सूचकांक;
_   (ReadLEB_u32(&function_सूचकांक, &o->wasm, o->wasm_अन्तः));

    IM3Function function = Module_GetFunction(o->module, function_सूचकांक);

    यदि (function) {                                                     m3log (compile, d_indent " (func= [%d] '%s'; args= %d)",
                                                                                get_indention_string (o), function_सूचकांक, m3_GetFunctionName (function), function->funcType->अङ्कः_तर्काः);
        // an import linked to another module's export runs that module's body
        IM3Function target = Function_Implementation(function);

        यदि (target->module) {
            bool isReturnCall = (i_opcode == c_waOp_returnCall);

            // The callee's body runs against its own module's memory, so a call
            // that crosses modules is bracketed by a pair of op_SetMemory. A
            // tail call never returns to run the second one, so those compile
            // as a plain call followed by a return instead.
            bool crossModule = (target->module != o->module);
            bool use_पुच्छ_Call = isReturnCall and d_m3_Can_पुच्छ_Call and not crossModule;

            u16 slot_शिखरम्, अङ्कः_Arg_Slots = 0;

            यदि (use_पुच्छ_Call) {
_               (Compile_पुच्छ_Call_तर्काः(o, &slot_शिखरम्, &अङ्कः_Arg_Slots, target->funcType, false));
            } अन्यथा {
_               (Compile_Call_तर्काः_And_Return(o, &slot_शिखरम्, target->funcType, false));
            }

            IM3Operation op;
            ध्रुव शून्य*  सङ्कार्यम्;

            यदि (target->compiled) {
                op      = use_पुच्छ_Call ? op_ReturnCall : op_Call;
                सङ्कार्यम् = target->compiled;
            } अन्यथा {
                op      = use_पुच्छ_Call ? op_CompileReturnCall : op_Compile;
                सङ्कार्यम् = target;
            }

#यदि d_m3HasExceptionHandling
            यदि (isReturnCall) {
_               (Emit_निष्कासय_Try_चौकटाः_For_Return_Call(o));
            }
#यदि_अन्तः

            यदि (crossModule) {
_               (Emit_समुच्चयः_स्मृतिः_Ptr(o, Module_Memory0(target->module)));
            }

_           (EmitOp(o, op));
            EmitPointer(o, सङ्कार्यम्);
            Emit_Slot_विस्थापनम्(o, slot_शिखरम्);

            यदि (not use_पुच्छ_Call) {
                // a suspend inside the callee leaves this frame waiting here, its
                // results not yet written
_               (लेखः_Call(o, Get_Func_Type_अङ्कः_Results(target->funcType), slot_शिखरम्, isReturnCall));
            }

            यदि (use_पुच्छ_Call) {
                Emit_Slot_विस्थापनम्(o, o->function->अङ्कः_Ret_Slots);
                EmitConstant32(o, अङ्कः_Arg_Slots);

_               (समुच्चयः_राशिः_Polymorphic(o));
            } अन्यथा {
                यदि (crossModule) {
_                   (Emit_समुच्चयः_स्मृतिः_Ptr(o, Module_Memory0(o->module)));
                }

                यदि (isReturnCall) {
_                   (Compile_Return(o, i_opcode));
                }
            }
        } अन्यथा {
            _throw(ErrorCompile(m3Err_functionImportMissing, o, "'%s.%s'", GetFunctionImportModuleName(function), m3_GetFunctionName(function)));
        }
    } अन्यथा {
        _throw(ErrorCompile(m3Err_functionLookupFailed, o, "index: %d", function_सूचकांक));
    }

    } _catch: प्रत्यागम परिणाम;
}

स्थिर
M3_परिणाम Compile_Call_परोक्षम् (IM3Compilation o, m3opcode_t i_opcode)
{
_try {
    u32 type_सूचकांक;
_   (ReadLEB_u32(&type_सूचकांक, &o->wasm, o->wasm_अन्तः));

    u32 सारणी_सूचकांक;
_   (ReadLEB_u32(&सारणी_सूचकांक, &o->wasm, o->wasm_अन्तः));

    _throwif("function call type index out of range", type_सूचकांक >= o->module->अङ्कः_Func_Types);
    _throwif("table index out of range", सारणी_सूचकांक >= o->module->अङ्कः_Tables);
    _throwif(m3Err_typeMismatch, आधारः_Type_सम्बन्धि(o->module->tables[सारणी_सूचकांक]->type) != c_m3Type_funcref);

_   (Check_सङ्कार्यम्_Type(o, 0, सारणी_Addr_Type(o->module->tables[सारणी_सूचकांक])));

    यदि (Is_राशिः_शिखरम्_अन्तर्_पञ्जिकाकोशः(o)) {
_       (Preserve_पञ्जिकाकोशः_If_Occupied(o, c_m3Type_i32));
    }

    u16 सारणी_सूचकांक_Slot = Get_राशिः_शिखरम्_Slot_सङ्ख्या(o);

    bool isReturnCall = (i_opcode == c_wa_Op_return_Call_परोक्षम्);
    bool use_पुच्छ_Call  = isReturnCall and d_m3_Can_पुच्छ_Call;

    u16         exec_शिखरम्, अङ्कः_Arg_Slots = 0;
    IM3FuncType type = o->module->funcTypes[type_सूचकांक];

    यदि (use_पुच्छ_Call) {
_       (Compile_पुच्छ_Call_तर्काः(o, &exec_शिखरम्, &अङ्कः_Arg_Slots, type, true));
    } अन्यथा {
_       (Compile_Call_तर्काः_And_Return(o, &exec_शिखरम्, type, true));
    }

#यदि d_m3HasExceptionHandling
    यदि (isReturnCall) {
_       (Emit_निष्कासय_Try_चौकटाः_For_Return_Call(o));
    }
#यदि_अन्तः

    // the table comes first: how wide its indexes are is what says how much of
    // the slot below holds one
_   (EmitOp(o, use_पुच्छ_Call ? op_Return_Call_परोक्षम् : op_Call_परोक्षम्));
    EmitPointer(o, o->module->tables[सारणी_सूचकांक]);
    Emit_Slot_विस्थापनम्(o, सारणी_सूचकांक_Slot);
    EmitPointer(o, type);
    Emit_Slot_विस्थापनम्(o, exec_शिखरम्);

    यदि (not use_पुच्छ_Call) {
_       (लेखः_Call(o, Get_Func_Type_अङ्कः_Results(type), exec_शिखरम्, isReturnCall));
    }

    यदि (use_पुच्छ_Call) {
        Emit_Slot_विस्थापनम्(o, o->function->अङ्कः_Ret_Slots);
        EmitConstant32(o, अङ्कः_Arg_Slots);

_       (समुच्चयः_राशिः_Polymorphic(o));
    } अन्यथा यदि (isReturnCall) {
_       (Compile_Return(o, i_opcode));
    }

} _catch:
    प्रत्यागम परिणाम;
}

// Reads the memory index a memory instruction names, and checks it against the
// module's index space. Without multi-memory the immediate is a reserved byte
// that has to be zero, which is the same check.
स्थिर
M3_परिणाम Read_स्मृतिः_सूचकांक (IM3Compilation o, u32* o_स्मृतिः_Idx)
{
    M3_परिणाम परिणाम;

_   (ReadLEB_u32(o_स्मृतिः_Idx, &o->wasm, o->wasm_अन्तः));

    _throwif(m3_Err_unknown_स्मृतिः, *o_स्मृतिः_Idx >= o->module->अङ्कः_Memories);

    _catch: प्रत्यागम परिणाम;
}

स्थिर
M3_परिणाम Compile_स्मृतिः_आकार (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_परिणाम परिणाम;

    u32 स्मृतिः_Idx;

    // A page count is given in the memory's own address type. Declared up here
    // so the throws below don't jump over its initialization.
    m3type_t addrType = c_m3Type_i32;

_   (Read_स्मृतिः_सूचकांक(o, &स्मृतिः_Idx));

    addrType = स्मृतिः_Addr_Type(o->module->memories[स्मृतिः_Idx]);

_   (Preserve_पञ्जिकाकोशः_If_Occupied(o, addrType));

    यदि (स्मृतिः_Idx) {
_       (Emit_समुच्चयः_स्मृतिः(o, स्मृतिः_Idx));
    }

    // op_MemSize writes the whole register either way; the slot it is pushed
    // to is what decides how much of it the module gets to see
_   (EmitOp(o, op_Mem_आकार));

    यदि (स्मृतिः_Idx) {
_       (Emit_समुच्चयः_स्मृतिः(o, 0));
    }

_   (प्रक्षिप_पञ्जिकाकोशः(o, addrType));

    _catch: प्रत्यागम परिणाम;
}

स्थिर
M3_परिणाम Compile_स्मृतिः_Grow (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_परिणाम परिणाम;

    u32 स्मृतिः_Idx;

    // see Compile_Memory_Size
    m3type_t addrType = c_m3Type_i32;

_   (Read_स्मृतिः_सूचकांक(o, &स्मृतिः_Idx));

    addrType = स्मृतिः_Addr_Type(o->module->memories[स्मृतिः_Idx]);

_   (Copy_राशिः_शिखरम्_गन्तव्यम्_पञ्जिकाकोशः(o, false));
_   (निष्कासय_Type(o, addrType));

    यदि (स्मृतिः_Idx) {
_       (Emit_समुच्चयः_स्मृतिः(o, स्मृतिः_Idx));
    }

#यदि d_m3HasMemory64
_   (EmitOp(o, (addrType == c_m3Type_i64) ? op_MemGrow64 : op_MemGrow));
#अन्यथा
_   (EmitOp(o, op_MemGrow));
#यदि_अन्तः

    यदि (स्मृतिः_Idx) {
_       (Emit_समुच्चयः_स्मृतिः(o, 0));
    }

_   (प्रक्षिप_पञ्जिकाकोशः(o, addrType));

    _catch: प्रत्यागम परिणाम;
}

स्थिर
M3_परिणाम Compile_स्मृतिः_Copy_Fill (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_परिणाम परिणाम = m3Err_none;

    // memory.copy names the destination first and then the source; memory.fill
    // names only the one it writes.
    u32  source_स्मृतिः_Idx = 0, target_स्मृतिः_Idx = 0;
    bool isCopy = (i_opcode == c_wa_Op_स्मृतिः_Copy);

    // Each address is typed by the memory it belongs to. The length follows
    // the narrower of the two, so it is 64-bit only when both are - which for
    // memory.fill, naming one memory twice over, means whenever that one is.
    // Declared up here so the throws below don't jump over them.
    m3type_t targetType = c_m3Type_i32;
    m3type_t sourceType = c_m3Type_i32;
    m3type_t दीर्घता_Type = c_m3Type_i32;

    _throwif(m3Err_wasmMalformed, o->module->अङ्कः_Memories == 0);

_   (Read_स्मृतिः_सूचकांक(o, &target_स्मृतिः_Idx));

    यदि (isCopy) {
_       (Read_स्मृतिः_सूचकांक(o, &source_स्मृतिः_Idx));
    }

    targetType = स्मृतिः_Addr_Type(o->module->memories[target_स्मृतिः_Idx]);
    sourceType = isCopy ? स्मृतिः_Addr_Type(o->module->memories[source_स्मृतिः_Idx])
                        : targetType;
    दीर्घता_Type = (targetType == c_m3Type_i64 and sourceType == c_m3Type_i64)
                   ? c_m3Type_i64
                   : c_m3Type_i32;

_   (Copy_राशिः_शिखरम्_गन्तव्यम्_पञ्जिकाकोशः(o, false));

#यदि d_m3_Has_Multi_स्मृतिः
    यदि (isCopy and source_स्मृतिः_Idx != target_स्मृतिः_Idx) {
        // two memories at once: _mem can only name one, so the op takes both
_       (EmitOp(o, op_MemCopy_x));
        EmitPointer(o, o->module->memories[target_स्मृतिः_Idx]);
        EmitPointer(o, o->module->memories[source_स्मृतिः_Idx]);

        // ...and, since they need not be addressed alike, how wide to read
        // each of the operands: bit 0 the destination, bit 1 the source
        EmitConstant32(o, ((targetType == c_m3Type_i64) ? 0x1u : 0u) |
                            ((sourceType == c_m3Type_i64) ? 0x2u : 0u));
    } अन्यथा
#यदि_अन्तः
    {
        IM3Operation op = isCopy ? op_MemCopy : op_MemFill;

#यदि d_m3HasMemory64
        यदि (targetType == c_m3Type_i64) {
            op = isCopy ? op_MemCopy64 : op_MemFill64;
        }
#यदि_अन्तः

        यदि (target_स्मृतिः_Idx) {
_           (Emit_समुच्चयः_स्मृतिः(o, target_स्मृतिः_Idx));
        }

_       (EmitOp(o, op));
    }

    // the length is in the register; the other two are named by slot, at
    // whatever width their stack entries were pushed with
_   (निष्कासय_Type(o, दीर्घता_Type));
_   (Emit_Slot_अङ्कः_सम्बन्धि_राशिः_शिखरम्_And_निष्कासय(o));      // the source, or the byte to fill with
_   (Emit_Slot_अङ्कः_सम्बन्धि_राशिः_शिखरम्_And_निष्कासय(o));      // the destination

    यदि (target_स्मृतिः_Idx and not (isCopy and source_स्मृतिः_Idx != target_स्मृतिः_Idx)) {
_       (Emit_समुच्चयः_स्मृतिः(o, 0));
    }

    _catch: प्रत्यागम परिणाम;
}


// memory.init and data.drop address a data segment by index. Both are only valid
// when a data count section declared the segments up front.
स्थिर
M3_परिणाम Read_दत्त_Segment (IM3Compilation o, M3_दत्त_Segment** o_segment)
{
    M3_परिणाम परिणाम = m3Err_none;

    u32 index;
_   (ReadLEB_u32(&index, &o->wasm, o->wasm_अन्तः));

    _throwif("data count section required", not o->module->has_दत्त_गणनम्);
    _throwif(m3Err_wasmMalformed, index >= o->module->अङ्कः_दत्त_Segments);

    *o_segment = &o->module->दत्त_Segments[index];

    _catch: प्रत्यागम परिणाम;
}


M3_परिणाम Compile_स्मृतिः_Init (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_परिणाम परिणाम = m3Err_none;

    M3_दत्त_Segment* segment = रिक्त;
    u32            स्मृतिः_Idx;

    _throwif(m3Err_wasmMalformed, o->module->अङ्कः_Memories == 0);

_   (Read_दत्त_Segment(o, &segment));
_   (Read_स्मृतिः_सूचकांक(o, &स्मृतिः_Idx));

_   (Copy_राशिः_शिखरम्_गन्तव्यम्_पञ्जिकाकोशः(o, false));

    यदि (स्मृतिः_Idx) {
_       (Emit_समुच्चयः_स्मृतिः(o, स्मृतिः_Idx));
    }

#यदि d_m3HasMemory64
    // only the destination follows the memory's address type: a data segment
    // is indexed as an i32 however the memory it is copied into is addressed
_   (EmitOp(o, o->module->memories[स्मृतिः_Idx]->isMemory64 ? op_MemInit64 : op_MemInit));
#अन्यथा
_   (EmitOp(o, op_MemInit));
#यदि_अन्तः
    EmitPointer(o, segment);
_   (निष्कासय_Type(o, c_m3Type_i32));
_   (Emit_Slot_अङ्कः_सम्बन्धि_राशिः_शिखरम्_And_निष्कासय(o));
_   (Emit_Slot_अङ्कः_सम्बन्धि_राशिः_शिखरम्_And_निष्कासय(o));

    यदि (स्मृतिः_Idx) {
_       (Emit_समुच्चयः_स्मृतिः(o, 0));
    }

    _catch: प्रत्यागम परिणाम;
}


#यदि d_m3HasRefTypes

स्थिर
M3_परिणाम Compile_Select (IM3Compilation o, m3opcode_t i_opcode);

// select with an explicit result type vector. The types only matter to the
// validator; the operands are laid out exactly as for the untyped select.
M3_परिणाम Compile_Select_Typed (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_परिणाम परिणाम = m3Err_none;

    u32 अङ्कः_Types;
_   (ReadLEB_u32(&अङ्कः_Types, &o->wasm, o->wasm_अन्तः));
    _throwif(m3Err_wasmMalformed, अङ्कः_Types != 1);

    i8 waType;
    u8 type;
_   (ReadLEB_i7(&waType, &o->wasm, o->wasm_अन्तः));
_   (NormalizeType(&type, waType));

_   (Compile_Select(o, i_opcode));

    _catch: प्रत्यागम परिणाम;
}


// A reference is one pointer-sized word and null is 0, so null-testing it is
// just an integer eqz of the matching width.
#  यदि M3_SIZEOF_PTR == 8
#    define d_m3RefIsNull_r   op_i64_Equal_गन्तव्यम्_Zero_r
#    define d_m3RefIsNull_s   op_i64_Equal_गन्तव्यम्_Zero_s
#  अन्यथा
#    define d_m3RefIsNull_r   op_i32_Equal_गन्तव्यम्_Zero_r
#    define d_m3RefIsNull_s   op_i32_Equal_गन्तव्यम्_Zero_s
#  endif

स्थिर
M3_परिणाम ReadRefType (IM3Compilation o, m3type_t* o_type)
{
    M3_परिणाम परिणाम = m3Err_none;

#  यदि d_m3HasTypedRefs
    // ref.null names a heap type: func, extern, or a function type index
    m3type_t heap_द्वयङ्काः;
_   (ParseHeapType(o->module, &heap_द्वयङ्काः, &o->wasm, o->wasm_अन्तः));

    *o_type = d_m3Type_ref | heap_द्वयङ्काः;
#  अन्यथा
    i8 waType;
    u8 plainType;
_   (ReadLEB_i7(&waType, &o->wasm, o->wasm_अन्तः));
_   (NormalizeType(&plainType, waType));
    *o_type = plainType;
    _throwif(m3Err_wasmMalformed, not IsRefType(*o_type));
#  endif

    _catch: प्रत्यागम परिणाम;
}


M3_परिणाम Compile_Ref_Null (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_परिणाम परिणाम = m3Err_none;

    m3type_t type;
_   (ReadRefType(o, &type));
_   (प्रक्षिप_Const(o, 0, type));

    _catch: प्रत्यागम परिणाम;
}


M3_परिणाम Compile_Ref_IsNull (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_परिणाम परिणाम = m3Err_none;

    IM3Operation op;

    यदि (not Is_राशिः_Polymorphic(o)) {
        _throwif(m3Err_typeMismatch, not IsRefType(Get_राशिः_शिखरम्_Type(o)));
    }

    यदि (Is_राशिः_शिखरम्_अन्तर्_पञ्जिकाकोशः(o)) {
        op = d_m3RefIsNull_r;
    } अन्यथा {
_       (Preserve_पञ्जिकाकोशः_If_Occupied(o, c_m3Type_i32));
        op = d_m3RefIsNull_s;
    }

_   (EmitOp(o, op));
_   (Emit_Slot_अङ्कः_सम्बन्धि_राशिः_शिखरम्_And_निष्कासय(o));
_   (प्रक्षिप_पञ्जिकाकोशः(o, c_m3Type_i32));

    _catch: प्रत्यागम परिणाम;
}


M3_परिणाम Compile_Ref_Func (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_परिणाम परिणाम = m3Err_none;

    u32 func_सूचकांक;

    // declared before the throws below, which jump past this point to _catch
    m3type_t    refType   = c_m3Type_funcref;
    IM3Function reference = रिक्त;

_   (ReadLEB_u32(&func_सूचकांक, &o->wasm, o->wasm_अन्तः));
    _throwif("function index out of range", func_सूचकांक >= o->module->अङ्कः_Functions);

    // Inside a constant expression ref.func is itself a declaration; inside a
    // function body the function must already have been declared elsewhere.
    यदि (o->function) {
        _throwif("undeclared function reference", not Module_IsFunctionDeclared(o->module, func_सूचकांक));
    } अन्यथा {
_       (Module_DeclareFunction(o->module, func_सूचकांक));
    }

#  यदि d_m3HasTypedRefs
    refType = Ref_Type_सम्बन्धि_Func_Type(o->module->functions[func_सूचकांक].funcType, true);
#  endif

    // A reference denotes the function that actually runs, so a ref.func naming
    // an import linked to another module is the same reference that module's
    // own ref.func would produce - and carries the defining module with it.
    reference = Function_Implementation(&o->module->functions[func_सूचकांक]);

_   (प्रक्षिप_Const(o, (u64)(uintptr_t)reference, refType));

    _catch: प्रत्यागम परिणाम;
}


// table.get/set/size/grow/fill all name a table by index; the table struct goes
// into the codestream so the operation doesn't have to walk the module.
स्थिर
M3_परिणाम Read_सारणी (IM3Compilation o, M3_सारणी** o_सारणी)
{
    M3_परिणाम परिणाम = m3Err_none;

    u32 index;
_   (ReadLEB_u32(&index, &o->wasm, o->wasm_अन्तः));
    _throwif("table index out of range", index >= o->module->अङ्कः_Tables);

    *o_सारणी = o->module->tables[index];

    _catch: प्रत्यागम परिणाम;
}


// The table operations take every operand from a slot, so spill the integer
// register first: after this each EmitSlotNumOfStackTopAndPop () emits a real
// slot offset, and the register is free for the result. Operands are emitted
// from the top of the stack downwards, which is the order the ops read them.
स्थिर
M3_परिणाम Compile_सारणी_Op (IM3Compilation o, IM3Operation i_op, M3_सारणी* i_सारणी, u32 i_अङ्कः_Operands, m3type_t i_retType)
{
    M3_परिणाम परिणाम = m3Err_none;

_   (Preserve_पञ्जिकाकोशः_If_Occupied(o, c_m3Type_i64));

_   (EmitOp(o, i_op));
    EmitPointer(o, i_सारणी);

    प्रति (u32 i = 0; i < i_अङ्कः_Operands; ++i) {
_       (Emit_Slot_अङ्कः_सम्बन्धि_राशिः_शिखरम्_And_निष्कासय(o));
    }

    यदि (i_retType != c_m3Type_none) {
_       (प्रक्षिप_पञ्जिकाकोशः(o, i_retType));
    }

    _catch: प्रत्यागम परिणाम;
}


M3_परिणाम Compile_सारणी_Get_समुच्चयः (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_परिणाम परिणाम = m3Err_none;

    M3_सारणी* सारणी_2;
_   (Read_सारणी(o, &सारणी_2));

    यदि (i_opcode == c_wa_Op_सारणी_Get) {
_       (Check_सङ्कार्यम्_Type(o, 0, सारणी_Addr_Type(सारणी_2)));
_       (Compile_सारणी_Op(o, op_सारणी_Get, सारणी_2, 1, सारणी_2->type));
    } अन्यथा {
_       (Check_सङ्कार्यम्_Type(o, 1, सारणी_Addr_Type(सारणी_2)));
_       (Compile_सारणी_Op(o, op_सारणी_समुच्चयः, सारणी_2, 2, c_m3Type_none));
    }

    _catch: प्रत्यागम परिणाम;
}


M3_परिणाम Compile_सारणी_Init (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_परिणाम परिणाम = m3Err_none;

    u32      elem_सूचकांक;
    M3_सारणी* सारणी_2;

_   (ReadLEB_u32(&elem_सूचकांक, &o->wasm, o->wasm_अन्तः));
    _throwif("element segment index out of range", elem_सूचकांक >= o->module->अङ्कः_Element_Segments);
_   (Read_सारणी(o, &सारणी_2));

_   (Check_सङ्कार्यम्_Type(o, 2, सारणी_Addr_Type(सारणी_2)));

_   (Preserve_पञ्जिकाकोशः_If_Occupied(o, c_m3Type_i64));
_   (EmitOp(o, op_सारणी_Init));
    EmitPointer(o, सारणी_2);
    EmitPointer(o, &o->module->elementSegments[elem_सूचकांक]);

    प्रति (u32 i = 0; i < 3; ++i) {
_       (Emit_Slot_अङ्कः_सम्बन्धि_राशिः_शिखरम्_And_निष्कासय(o));
    }

    _catch: प्रत्यागम परिणाम;
}


M3_परिणाम Compile_Elem_Drop (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_परिणाम परिणाम = m3Err_none;

    u32 elem_सूचकांक;
_   (ReadLEB_u32(&elem_सूचकांक, &o->wasm, o->wasm_अन्तः));
    _throwif("element segment index out of range", elem_सूचकांक >= o->module->अङ्कः_Element_Segments);

_   (EmitOp(o, op_ElemDrop));
    EmitPointer(o, &o->module->elementSegments[elem_सूचकांक]);

    _catch: प्रत्यागम परिणाम;
}


M3_परिणाम Compile_सारणी_Copy (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_परिणाम परिणाम = m3Err_none;

    M3_सारणी* dst;
    M3_सारणी* src;

_   (Read_सारणी(o, &dst));
_   (Read_सारणी(o, &src));
    _throwif(m3Err_typeMismatch, not Is_Sub_Type_सम्बन्धि(src->type, dst->type));

_   (Check_सङ्कार्यम्_Type(o, 2, सारणी_Addr_Type(dst)));
_   (Check_सङ्कार्यम्_Type(o, 1, सारणी_Addr_Type(src)));
_   (Check_सङ्कार्यम्_Type(o, 0, (dst->isTable64 and src->isTable64) ? c_m3Type_i64 : c_m3Type_i32));

_   (Preserve_पञ्जिकाकोशः_If_Occupied(o, c_m3Type_i64));
_   (EmitOp(o, op_सारणी_Copy));
    EmitPointer(o, dst);
    EmitPointer(o, src);

    प्रति (u32 i = 0; i < 3; ++i) {
_       (Emit_Slot_अङ्कः_सम्बन्धि_राशिः_शिखरम्_And_निष्कासय(o));
    }

    _catch: प्रत्यागम परिणाम;
}


M3_परिणाम Compile_सारणी_आकार (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_परिणाम परिणाम = m3Err_none;

    M3_सारणी* सारणी_2;
_   (Read_सारणी(o, &सारणी_2));

    // a table size is given in the table's own index type
_   (Compile_सारणी_Op(o, op_सारणी_आकार, सारणी_2, 0, सारणी_Addr_Type(सारणी_2)));

    _catch: प्रत्यागम परिणाम;
}


M3_परिणाम Compile_सारणी_Grow_Fill (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_परिणाम परिणाम = m3Err_none;

    M3_सारणी* सारणी_2;
_   (Read_सारणी(o, &सारणी_2));

    यदि (i_opcode == c_wa_Op_सारणी_Grow) {
_       (Check_सङ्कार्यम्_Type(o, 0, सारणी_Addr_Type(सारणी_2)));
_       (Compile_सारणी_Op(o, op_सारणी_Grow, सारणी_2, 2, सारणी_Addr_Type(सारणी_2)));
    } अन्यथा {
_       (Check_सङ्कार्यम्_Type(o, 0, सारणी_Addr_Type(सारणी_2)));
_       (Check_सङ्कार्यम्_Type(o, 2, सारणी_Addr_Type(सारणी_2)));
_       (Compile_सारणी_Op(o, op_सारणी_Fill, सारणी_2, 3, c_m3Type_none));
    }

    _catch: प्रत्यागम परिणाम;
}

#यदि_अन्तः // d_m3HasRefTypes


M3_परिणाम Compile_दत्त_Drop (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_परिणाम परिणाम = m3Err_none;

    M3_दत्त_Segment* segment = रिक्त;
_   (Read_दत्त_Segment(o, &segment));

_   (EmitOp(o, op_दत्त_Drop));
    EmitPointer(o, segment);

    _catch: प्रत्यागम परिणाम;
}


स्थिर
M3_परिणाम Read_खण्डम्_Type (IM3Compilation o, IM3FuncType* o_खण्डम्_Type)
{
    M3_परिणाम परिणाम = m3Err_none;

#यदि d_m3HasTypedRefs
    // a spelled-out reference type is two bytes, so it cannot be told apart
    // from a type index by the s33 that block types otherwise use
    यदि (o->wasm < o->wasm_अन्तः and (*o->wasm == d_waEncode_ref or *o->wasm == d_waEncode_refNull)) {
        m3type_t refType;
_       (Parse_मानम्_Type(o->module, &refType, &o->wasm, o->wasm_अन्तः));
        IM3FuncType ftype = रिक्त;
_       (AllocFuncType(&ftype, 1));
        ftype->अङ्कः_Rets  = 1;
        ftype->types[0] = refType;
_       (Environment_सङ्कलय_Func_Type(o->module->environment, &ftype));
        *o_खण्डम्_Type = ftype;
        प्रत्यागम परिणाम;
    }
#यदि_अन्तः

    i64 type;
_   (ReadLebSigned(&type, 33, &o->wasm, o->wasm_अन्तः));

    यदि (type < 0) {
        _throwif(m3Err_invalidTypeId, type < -64);
        u8 मानम्_Type;
_       (NormalizeType(&मानम्_Type, (i8)type));                                 m3log (compile, d_indent " (type: %s)", get_indention_string (o), c_waTypes [मानम्_Type]);
        *o_खण्डम्_Type = o->module->environment->retFuncTypes[मानम्_Type];
    } अन्यथा {
        _throwif("func type out of bounds", type >= o->module->अङ्कः_Func_Types);
        *o_खण्डम्_Type = o->module->funcTypes[type];                         m3log (compile, d_indent " (type: %s)", get_indention_string (o), SPrintFuncTypeSignature (*o_खण्डम्_Type));
    }
    _catch: प्रत्यागम परिणाम;
}

स्थिर
M3_परिणाम Preserve_तर्काः_And_Locals (IM3Compilation o)
{
    M3_परिणाम परिणाम = m3Err_none;

    यदि (o->राशिः_सूचकांक > o->राशिः_First_Dynamic_सूचकांक) {
        u32 अङ्कः_तर्काः_And_Locals = Get_Function_अङ्कः_तर्काः_And_Locals(o->function);

        प्रति (u32 i = 0; i < अङ्कः_तर्काः_And_Locals; ++i) {
            u16 slot = Get_Slot_For_राशिः_सूचकांक(o, i);

            u16 preserved_Slot_सङ्ख्या;
_           (Find_Referenced_Local_Within_Current_खण्डम्(o, &preserved_Slot_सङ्ख्या, slot));

            यदि (preserved_Slot_सङ्ख्या != slot) {
                m3type_t     type = Get_राशिः_Type_From_Bottom(o, i);                 d_m3Assert (type != c_m3Type_none)
                IM3Operation op   = Is64BitType(type) ? op_CopySlot_64 : op_CopySlot_32;

                EmitOp(o, op);
                Emit_Slot_विस्थापनम्(o, preserved_Slot_सङ्ख्या);
                Emit_Slot_विस्थापनम्(o, slot);
            }
        }
    }

    _catch:
    प्रत्यागम परिणाम;
}

स्थिर
M3_परिणाम Compile_Loop_Or_खण्डम् (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_परिणाम परिणाम;

    IM3FuncType खण्डम्_Type = रिक्त;

_   (PreserveRegisters(o));
_   (Preserve_तर्काः_And_Locals(o));

_   (Read_खण्डम्_Type(o, &खण्डम्_Type));

    यदि (i_opcode == c_waOp_loop) {
        u16 अङ्कः_Params = Get_Func_Type_अङ्कः_Params(खण्डम्_Type);
        यदि (अङ्कः_Params) {
            // instantiate constants
            u16 अङ्कः_मानानि = Get_अङ्कः_खण्डम्_मानानि_On_राशिः(o);

            यदि (अङ्कः_मानानि >= अङ्कः_Params) {
                u16 राशिः_शिखरम् = Get_राशिः_शिखरम्_सूचकांक(o) + 1;

                प्रति (u16 i = राशिः_शिखरम् - अङ्कः_Params; i < राशिः_शिखरम्; ++i) {
                    u16      slot = Get_Slot_For_राशिः_सूचकांक(o, i);
                    m3type_t type = Get_राशिः_Type_From_Bottom(o, i);

                    यदि (IsConstantSlot(o, slot)) {
                        u16 newSlot = c_slot_अप्रयुक्तम्;
_                       (AllocateSlots(o, &newSlot, type));
_                       (Copy_राशिः_सूचकांक_गन्तव्यम्_Slot(o, newSlot, i));
                        o->wasm_राशिः[i] = newSlot;
                    }
                }
            }
        }

_       (EmitOp(o, op_Loop));
    } अन्यथा {
    }

_   (Compile_खण्डम्(o, खण्डम्_Type, i_opcode));

    _catch: प्रत्यागम परिणाम;
}

#यदि d_m3HasExceptionHandling

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
स्थिर
M3_परिणाम EmitCatchStubs (IM3Compilation o)
{
    IM3_संहिता_Page displacedPage = रिक्त;
_try {
    M3CatchClause* clauses    = o->tryClauses;
    u32            अङ्कः_Clauses = o->अङ्कः_Try_Clauses;

    // the body must not see these: a nested try_table sets up its own
    o->tryClauses    = रिक्त;
    o->अङ्कः_Try_Clauses = 0;

    प्रति (u32 i = 0; i < अङ्कः_Clauses; ++i) {
        M3CatchClause* clause = &clauses[i];

        IM3CompilationScope scope;
_       (Get_खण्डम्_Scope(o, &scope, clause->label_गभीरता + 1));

        IM3FuncType payload = clause->चिह्नकम् ? clause->चिह्नकम्->type : रिक्त;
        u16         अङ्कः_तर्काः = Get_Func_Type_अङ्कः_Params(payload);

        // What the stub hands the label - the payload, then the exnref for a
        // _ref clause - has to be what the label takes. ResolveBlockResults
        // only counts the values, and the validator compares base types, so a
        // typed reference is only checked here: (ref null $t) reaching a label
        // that takes (ref $t) has to be rejected.
        {
            bool isLoop         = (scope->opcode == c_waOp_loop);
            u16  अङ्कः_Label_मानानि = isLoop ? Get_Func_Type_अङ्कः_Params(scope->type)
                                         : Get_Func_Type_अङ्कः_Results(scope->type);

            _throwif(m3_Err_type_गणनम्_Mismatch, अङ्कः_Label_मानानि != अङ्कः_तर्काः + (clause->hasRef ? 1 : 0));

            प्रति (u16 a = 0; a < अङ्कः_Label_मानानि; ++a) {
                m3type_t given  = (a < अङ्कः_तर्काः) ? GetFuncTypeParamType(payload, a)
                                                : (m3type_t)c_m3Type_exnref;
                m3type_t wanted = isLoop ? GetFuncTypeParamType(scope->type, a)
                                         : Get_Func_Type_परिणाम_Type(scope->type, a);

                _throwif(m3Err_typeMismatch, not Is_Sub_Type_सम्बन्धि(given, wanted));
            }
        }

        IM3_संहिता_Page stubPage;
_       (Acquire_Compilation_संहिता_Page(o, &stubPage));

        pc_t startPC  = GetPagePC(stubPage);
        displacedPage = o->page;
        o->page       = stubPage;

        u16 first_सूचकांक = o->राशिः_सूचकांक;

        प्रति (u16 a = 0; a < अङ्कः_तर्काः; ++a) {
_           (प्रक्षिप_Allocated_Slot(o, GetFuncTypeParamType(payload, a)));
        }

        यदि (clause->hasRef) {
_           (प्रक्षिप_Allocated_Slot(o, c_m3Type_exnref));
        }

        // emitted even with nothing to move: this is also where an exception
        // no clause will reify gets released
_       (Ensure_संहिता_Page_अङ्कः_पङ्क्तयः(o, 2 * अङ्कः_तर्काः + 3 + d_m3_संहिता_Page_Free_पङ्क्तयः_देहली));

_       (EmitOp(o, op_CatchPayload));
        EmitConstant32(o, अङ्कः_तर्काः);

        प्रति (u16 a = 0; a < अङ्कः_तर्काः; ++a) {
            u16 index = first_सूचकांक + a;
            Emit_Slot_विस्थापनम्(o, Get_Slot_For_राशिः_सूचकांक(o, index));
            EmitConstant32(o, Is64BitType(Get_राशिः_Type_From_Bottom(o, index)));
        }

        Emit_Slot_विस्थापनम्(o, clause->hasRef ? (i32)Get_Slot_For_राशिः_सूचकांक(o, first_सूचकांक + अङ्कः_तर्काः) : -1);

_       (Emit_निष्कासय_Try_चौकटाः(o, अङ्कः_Try_चौकटाः_गन्तव्यम्_निष्कासय(o->खण्डम्.outer, scope)));

        यदि (scope->opcode == c_waOp_loop) {
_           (Resolve_खण्डम्_Results(o, scope, /* isBranch: */ true));

_           (EmitOp(o, Op_ContinueLoop(o)));
            EmitPointer(o, scope->pc);
_           (लेखः_पृष्ठभागः_Edge(o, scope, false));
        } अन्यथा यदि (scope->गभीरता == 0) {
_           (Return_मानानि(o, scope, /* isBranch: */ true));
_           (EmitOp(o, op_Return));
        } अन्यथा {
_           (Resolve_खण्डम्_Results(o, scope, /* isBranch: */ true));
_           (EmitPatchingBranch(o, scope));
        }

        IM3_संहिता_Page return_गन्तव्यम् = displacedPage;
        displacedPage        = रिक्त;

        Release_Compilation_संहिता_Page(o);
        o->page = return_गन्तव्यम्;

        *(pc_t*)clause->stubSlot = startPC;

        यावत् (o->राशिः_सूचकांक > first_सूचकांक) {
_           (निष्कासय(o));
        }
    }

} _catch:

    // thrown with a stub page installed: release it and put back the one it
    // displaced, which the caller's catch then releases
    यदि (displacedPage) {
        Release_Compilation_संहिता_Page(o);
        o->page = displacedPage;
    }

    प्रत्यागम परिणाम;
}


स्थिर
M3_परिणाम Compile_Try_सारणी (IM3Compilation o, m3opcode_t i_opcode)
{
    M3CatchClause* clauses = रिक्त;
_try {
    // a catch entry has to be able to assume the operand stack is entirely in
    // slots: an unwind leaves nothing in the registers
_   (PreserveRegisters(o));
_   (Preserve_तर्काः_And_Locals(o));

    IM3FuncType खण्डम्_Type = रिक्त;
_   (Read_खण्डम्_Type(o, &खण्डम्_Type));

    u32 अङ्कः_Clauses;
_   (ReadLEB_u32(&अङ्कः_Clauses, &o->wasm, o->wasm_अन्तः));

    _throwif("too many catch clauses", अङ्कः_Clauses > d_m3_Max_Sane_चिह्नकानि_गणनम्);

    यदि (अङ्कः_Clauses) {
        clauses = m3_AllocArray(M3CatchClause, अङ्कः_Clauses);
        _throwifnull(clauses);
    }

    प्रति (u32 i = 0; i < अङ्कः_Clauses; ++i) {
        u8 विधा;
_       (Read_u8(&विधा, &o->wasm, o->wasm_अन्तः));
        _throwif(m3Err_wasmMalformed, विधा > 0x03);

        clauses[i].चिह्नकम्    = रिक्त;
        clauses[i].hasRef = (विधा == 0x01 or विधा == 0x03);

        यदि (विधा == 0x00 or विधा == 0x01) {
            u32 चिह्नकम्_सूचकांक;
_           (ReadLEB_u32(&चिह्नकम्_सूचकांक, &o->wasm, o->wasm_अन्तः));
            _throwif(m3_Err_unknown_चिह्नकम्, चिह्नकम्_सूचकांक >= o->module->अङ्कः_चिह्नकानि);

            IM3_चिह्नकम् चिह्नकम्     = &o->module->चिह्नकानि[चिह्नकम्_सूचकांक];
            clauses[i].चिह्नकम् = चिह्नकम्->resolved ? चिह्नकम्->resolved : चिह्नकम्;
        }

_       (ReadLEB_u32(&clauses[i].label_गभीरता, &o->wasm, o->wasm_अन्तः));
    }

    // op_TryTable, the clause count, and two words per clause, all of which have
    // to land on one page: the op reads the table by offset from its own pc
_   (Ensure_संहिता_Page_अङ्कः_पङ्क्तयः(o, 2 * अङ्कः_Clauses + 2 + d_m3_संहिता_Page_Free_पङ्क्तयः_देहली));

_   (EmitOp(o, op_Try_सारणी));
    EmitConstant32(o, अङ्कः_Clauses);

    प्रति (u32 i = 0; i < अङ्कः_Clauses; ++i) {
        EmitPointer(o, clauses[i].चिह्नकम्);
        clauses[i].stubSlot = ReservePointer(o);
    }

    o->tryClauses    = clauses;
    o->अङ्कः_Try_Clauses = अङ्कः_Clauses;

_   (Compile_खण्डम्(o, खण्डम्_Type, c_wa_Op_try_सारणी));

} _catch:

    o->tryClauses    = रिक्त;
    o->अङ्कः_Try_Clauses = 0;

    m3_Free(clauses);

    प्रत्यागम परिणाम;
}


स्थिर
M3_परिणाम Compile_Throw (IM3Compilation o, m3opcode_t i_opcode)
{
_try {
    u32 चिह्नकम्_सूचकांक;
_   (ReadLEB_u32(&चिह्नकम्_सूचकांक, &o->wasm, o->wasm_अन्तः));
    _throwif(m3_Err_unknown_चिह्नकम्, चिह्नकम्_सूचकांक >= o->module->अङ्कः_चिह्नकानि);

    IM3_चिह्नकम् चिह्नकम् = &o->module->चिह्नकानि[चिह्नकम्_सूचकांक];
    यदि (चिह्नकम्->resolved) {
        चिह्नकम् = चिह्नकम्->resolved;
    }

    u16 अङ्कः_तर्काः = Get_Func_Type_अङ्कः_Params(चिह्नकम्->type);

    // the payload is read out of slots, so nothing may still be in a register
_   (PreserveRegisters(o));

    // check the payload before emitting anything, so a mistyped throw does not
    // leave half an operation behind
    bool isPolymorphic = Is_राशिः_Polymorphic(o);

    यदि (not isPolymorphic) {
        _throwif(m3_Err_type_गणनम्_Mismatch, Get_अङ्कः_खण्डम्_मानानि_On_राशिः(o) < अङ्कः_तर्काः);

        प्रति (u16 i = 0; i < अङ्कः_तर्काः; ++i) {
            u16 index = (u16)(o->राशिः_सूचकांक - अङ्कः_तर्काः + i);

            _throwif(m3Err_typeMismatch, not Is_Sub_Type_सम्बन्धि(Get_राशिः_Type_From_Bottom(o, index),
                                                         GetFuncTypeParamType(चिह्नकम्->type, i)));
        }
    }

_   (Ensure_संहिता_Page_अङ्कः_पङ्क्तयः(o, 2 * अङ्कः_तर्काः + 3 + d_m3_संहिता_Page_Free_पङ्क्तयः_देहली));

_   (EmitOp(o, op_Throw));
    EmitPointer(o, चिह्नकम्);
    EmitConstant32(o, अङ्कः_तर्काः);

    // the payload is named in the order the tag declares it, so the deepest of
    // the arguments on the stack comes first. In unreachable code there is
    // nothing on the stack to name and nothing will run this, so slot 0 stands
    // in for an operand that isn't there.
    प्रति (u16 i = 0; i < अङ्कः_तर्काः; ++i) {
        m3type_t type = GetFuncTypeParamType(चिह्नकम्->type, i);

        Emit_Slot_विस्थापनम्(o, isPolymorphic ? 0 : Get_Slot_For_राशिः_सूचकांक(o, (u16)(o->राशिः_सूचकांक - अङ्कः_तर्काः + i)));
        EmitConstant32(o, Is64BitType(type));
    }

    यदि (not isPolymorphic) {
        प्रति (u16 i = 0; i < अङ्कः_तर्काः; ++i) {
_           (निष्कासय(o));
        }
    }

_   (समुच्चयः_राशिः_Polymorphic(o));

} _catch: प्रत्यागम परिणाम;
}


स्थिर
M3_परिणाम Compile_ThrowRef (IM3Compilation o, m3opcode_t i_opcode)
{
_try {
    यदि (not Is_राशिः_Polymorphic(o)) {
_       (Preserve_पञ्जिकाकोशः_If_Occupied(o, c_m3Type_i64));

        _throwif(m3Err_typeMismatch, आधारः_Type_सम्बन्धि(Get_राशिः_शिखरम्_Type(o)) != c_m3Type_exnref);

_       (EmitOp(o, op_ThrowRef));
        Emit_Slot_विस्थापनम्(o, Get_राशिः_शिखरम्_Slot_सङ्ख्या(o));

_       (निष्कासय(o));
    }

_   (समुच्चयः_राशिः_Polymorphic(o));

} _catch: प्रत्यागम परिणाम;
}

#यदि_अन्तः // d_m3HasExceptionHandling

#यदि d_m3_Has_राशिः_Switching

स्थिर
M3_परिणाम Compile_ContNew (IM3Compilation o, m3opcode_t i_opcode)
{
_try {
    u32 type_सूचकांक;
_   (ReadLEB_u32(&type_सूचकांक, &o->wasm, o->wasm_अन्तः));
    _throwif(m3_Err_type_गणनम्_Mismatch, type_सूचकांक >= o->module->अङ्कः_Func_Types);

    IM3FuncType funcType = o->module->funcTypes[type_सूचकांक];
    _throwif("type is not a continuation", not funcType->isContinuation);

    // the operands are read out of slots, so nothing may still be in a register
_   (PreserveRegisters(o));

    // cont.new $ct : [(ref null $ft)] -> [(ref $ct)], where $ct ~~ cont $ft:
    // the function has to be one this continuation type can run
    m3type_t शिखरम्_Type = Get_राशिः_शिखरम्_Type(o);
    _throwif(m3Err_typeMismatch,
             not Is_Sub_Type_सम्बन्धि(शिखरम्_Type, Ref_Type_सम्बन्धि_Func_Type(funcType->contFuncType, false)));

    u16 funcSlot = Get_राशिः_शिखरम्_Slot_सङ्ख्या(o);
_   (निष्कासय(o));

    u16 dstSlot;
_   (प्रक्षिप_Allocated_Slot(o, Ref_Type_सम्बन्धि_Func_Type(funcType, true)));
    dstSlot = Get_राशिः_शिखरम्_Slot_सङ्ख्या(o);

_   (Ensure_संहिता_Page_अङ्कः_पङ्क्तयः(o, 4 + d_m3_संहिता_Page_Free_पङ्क्तयः_देहली));
_   (EmitOp(o, op_ContNew));
    EmitPointer(o, funcType);
    Emit_Slot_विस्थापनम्(o, dstSlot);
    Emit_Slot_विस्थापनम्(o, funcSlot);

} _catch: प्रत्यागम परिणाम;
}

स्थिर
M3_परिणाम Compile_ContBind (IM3Compilation o, m3opcode_t i_opcode)
{
_try {
    u32 typeIndex1, typeIndex2;
_   (ReadLEB_u32(&typeIndex1, &o->wasm, o->wasm_अन्तः));
_   (ReadLEB_u32(&typeIndex2, &o->wasm, o->wasm_अन्तः));
    _throwif(m3_Err_type_गणनम्_Mismatch, typeIndex1 >= o->module->अङ्कः_Func_Types);
    _throwif(m3_Err_type_गणनम्_Mismatch, typeIndex2 >= o->module->अङ्कः_Func_Types);

    IM3FuncType ct1 = o->module->funcTypes[typeIndex1];
    IM3FuncType ct2 = o->module->funcTypes[typeIndex2];
    _throwif("type is not a continuation", not ct1->isContinuation || not ct2->isContinuation);

    // the operands are read out of slots, so nothing may still be in a register
_   (PreserveRegisters(o));

    m3type_t contType = Get_राशिः_शिखरम्_Type(o);
    _throwif(m3Err_typeMismatch, not Is_Sub_Type_सम्बन्धि(contType, Ref_Type_सम्बन्धि_Func_Type(ct1, false)));
    u16 contSlot = Get_राशिः_शिखरम्_Slot_सङ्ख्या(o);
_   (निष्कासय(o));

    IM3FuncType ft1 = ct1->contFuncType ? ct1->contFuncType : ct1;
    IM3FuncType ft2 = ct2->contFuncType ? ct2->contFuncType : ct2;

    u16 अङ्कः_Params1 = Get_Func_Type_अङ्कः_Params(ft1);
    u16 अङ्कः_Params2 = Get_Func_Type_अङ्कः_Params(ft2);
    _throwif(m3Err_typeMismatch, अङ्कः_Params1 < अङ्कः_Params2);
    u32 अङ्कः_Bound = अङ्कः_Params1 - अङ्कः_Params2;

    u16 अङ्कः_Results1 = Get_Func_Type_अङ्कः_Results(ft1);
    u16 अङ्कः_Results2 = Get_Func_Type_अङ्कः_Results(ft2);
    _throwif(m3Err_typeMismatch, अङ्कः_Results1 != अङ्कः_Results2);
    प्रति (u16 i = 0; i < अङ्कः_Results1; ++i) {
        _throwif(m3Err_typeMismatch, Get_Func_Type_परिणाम_Type(ft1, i) != Get_Func_Type_परिणाम_Type(ft2, i));
    }
    प्रति (u16 i = 0; i < अङ्कः_Params2; ++i) {
        _throwif(m3Err_typeMismatch, GetFuncTypeParamType(ft1, अङ्कः_Bound + i) != GetFuncTypeParamType(ft2, i));
    }

    u16 argSlots[d_m3MaxContinuationPayload];
    _throwif("too many bound args", अङ्कः_Bound > d_m3MaxContinuationPayload);

    प्रति (u32 i = 0; i < अङ्कः_Bound; ++i) {
        u32      paramIdx     = अङ्कः_Bound - 1 - i;
        m3type_t expectedType = GetFuncTypeParamType(ft1, paramIdx);
        m3type_t actualType   = Get_राशिः_शिखरम्_Type(o);
        _throwif(m3Err_typeMismatch, not Is_Sub_Type_सम्बन्धि(actualType, expectedType));
        argSlots[paramIdx] = Get_राशिः_शिखरम्_Slot_सङ्ख्या(o);
_       (निष्कासय(o));
    }

    u16 dstSlot;
_   (प्रक्षिप_Allocated_Slot(o, Ref_Type_सम्बन्धि_Func_Type(ct2, true)));
    dstSlot = Get_राशिः_शिखरम्_Slot_सङ्ख्या(o);

_   (Ensure_संहिता_Page_अङ्कः_पङ्क्तयः(o, 6 + अङ्कः_Bound * 2 + d_m3_संहिता_Page_Free_पङ्क्तयः_देहली));
_   (EmitOp(o, op_ContBind));
    EmitPointer(o, ct1);
    EmitPointer(o, ct2);
    Emit_Slot_विस्थापनम्(o, dstSlot);
    Emit_Slot_विस्थापनम्(o, contSlot);
    EmitConstant32(o, अङ्कः_Bound);
    प्रति (u32 i = 0; i < अङ्कः_Bound; ++i) {
        Emit_Slot_विस्थापनम्(o, argSlots[i]);
        EmitConstant32(o, Is64BitType(GetFuncTypeParamType(ft1, i)));
    }

} _catch: प्रत्यागम परिणाम;
}

स्थिर
M3_परिणाम Compile_Suspend (IM3Compilation o, m3opcode_t i_opcode)
{
_try {
    u32 चिह्नकम्_सूचकांक;
_   (ReadLEB_u32(&चिह्नकम्_सूचकांक, &o->wasm, o->wasm_अन्तः));
    _throwif(m3_Err_unknown_चिह्नकम्, चिह्नकम्_सूचकांक >= o->module->अङ्कः_चिह्नकानि);

    IM3_चिह्नकम् चिह्नकम् = &o->module->चिह्नकानि[चिह्नकम्_सूचकांक];
    यदि (चिह्नकम्->resolved) {
        चिह्नकम् = चिह्नकम्->resolved;
    }
    u16 अङ्कः_Params  = Get_Func_Type_अङ्कः_Params(चिह्नकम्->type);
    u16 अङ्कः_Results = Get_Func_Type_अङ्कः_Results(चिह्नकम्->type);

_   (PreserveRegisters(o));

    u16 paramSlots[d_m3MaxContinuationPayload];
    _throwif("too many tag params", अङ्कः_Params > d_m3MaxContinuationPayload);
    प्रति (u16 i = 0; i < अङ्कः_Params; ++i) {
        u16      pIdx         = अङ्कः_Params - 1 - i;
        m3type_t expectedType = GetFuncTypeParamType(चिह्नकम्->type, pIdx);
        m3type_t actualType   = Get_राशिः_शिखरम्_Type(o);
        _throwif(m3Err_typeMismatch, not Is_Sub_Type_सम्बन्धि(actualType, expectedType));
        paramSlots[pIdx] = Get_राशिः_शिखरम्_Slot_सङ्ख्या(o);
_       (निष्कासय(o));
    }

    u16 परिणाम_Slots[d_m3MaxContinuationPayload];
    _throwif("too many tag results", अङ्कः_Results > d_m3MaxContinuationPayload);
    प्रति (u16 i = 0; i < अङ्कः_Results; ++i) {
        m3type_t rtype = Get_Func_Type_परिणाम_Type(चिह्नकम्->type, i);
_       (प्रक्षिप_Allocated_Slot(o, rtype));
        परिणाम_Slots[i] = Get_राशिः_शिखरम्_Slot_सङ्ख्या(o);
    }

_   (Ensure_संहिता_Page_अङ्कः_पङ्क्तयः(o, 4 + अङ्कः_Params * 2 + अङ्कः_Results * 2 + d_m3_संहिता_Page_Free_पङ्क्तयः_देहली));
_   (EmitOp(o, op_Suspend));
    EmitPointer(o, चिह्नकम्);
    EmitConstant32(o, अङ्कः_Params);
    प्रति (u16 i = 0; i < अङ्कः_Params; ++i) {
        Emit_Slot_विस्थापनम्(o, paramSlots[i]);
        EmitConstant32(o, Is64BitType(GetFuncTypeParamType(चिह्नकम्->type, i)));
    }
    EmitConstant32(o, अङ्कः_Results);
    प्रति (u16 i = 0; i < अङ्कः_Results; ++i) {
        Emit_Slot_विस्थापनम्(o, परिणाम_Slots[i]);
        EmitConstant32(o, Is64BitType(Get_Func_Type_परिणाम_Type(चिह्नकम्->type, i)));
    }

    // the results are part of the frame here: op_Suspend clears them, and
    // whatever resumes this writes them before anything reads them
_   (लेखः_Safe_Point(o, safepoint_suspend, 0, अङ्कः_Results, 0, रिक्त));

} _catch: प्रत्यागम परिणाम;
}

// resume, resume_throw and resume_throw_ref. All three run a continuation
// under a handler table and take its results; they differ only in what they
// hand it to start with - arguments, a fresh exception, or one off the stack.
//
// Emitted as: the continuation type, the slot holding it, the operands that
// belong to this form, the handler table, then the result slots. Keeping the
// tail identical is what lets one runtime path finish all three.
स्थिर
M3_परिणाम Compile_Resume (IM3Compilation o, m3opcode_t i_opcode)
{
    M3ResumeHandler* handlers      = रिक्त;
    IM3_संहिता_Page      displacedPage = रिक्त;
_try {
    bool isThrow    = (i_opcode == c_waOp_resumeThrow);
    bool isThrowRef = (i_opcode == c_waOp_resumeThrowRef);

    u32 type_सूचकांक;
_   (ReadLEB_u32(&type_सूचकांक, &o->wasm, o->wasm_अन्तः));
    _throwif(m3_Err_type_गणनम्_Mismatch, type_सूचकांक >= o->module->अङ्कः_Func_Types);

    IM3FuncType contFuncType = o->module->funcTypes[type_सूचकांक];
    _throwif("type is not a continuation", not contFuncType->isContinuation);

    IM3FuncType innerType      = contFuncType->contFuncType;
    u16         अङ्कः_Cont_Results = Get_Func_Type_अङ्कः_Results(innerType);

    IM3_चिह्नकम् exn_चिह्नकम्      = रिक्त;
    u16    अङ्कः_Operands = 0;

    यदि (isThrow) {
        u32 चिह्नकम्_सूचकांक;
_       (ReadLEB_u32(&चिह्नकम्_सूचकांक, &o->wasm, o->wasm_अन्तः));
        _throwif(m3_Err_unknown_चिह्नकम्, चिह्नकम्_सूचकांक >= o->module->अङ्कः_चिह्नकानि);

        exn_चिह्नकम् = &o->module->चिह्नकानि[चिह्नकम्_सूचकांक];

        // it is an exception that gets raised, so the tag must be one throw
        // could have raised: no results
        _throwif(m3Err_typeMismatch, Get_Func_Type_अङ्कः_Results(exn_चिह्नकम्->type) != 0);

        अङ्कः_Operands = Get_Func_Type_अङ्कः_Params(exn_चिह्नकम्->type);
    } अन्यथा यदि (not isThrowRef) {
        // a plain resume hands over the continuation's own parameters. An
        // aborted one never receives them, so they are unconstrained there.
        अङ्कः_Operands = Get_Func_Type_अङ्कः_Params(innerType);
    }

    u32 अङ्कः_Clauses;
_   (ReadLEB_u32(&अङ्कः_Clauses, &o->wasm, o->wasm_अन्तः));

    u32 clauseLabelDepths[16];
    _throwif("too many resume clauses", अङ्कः_Clauses > 16);

    यदि (अङ्कः_Clauses) {
        handlers = m3_AllocArray(M3ResumeHandler, अङ्कः_Clauses);
        _throwifnull(handlers);
    }

    प्रति (u32 i = 0; i < अङ्कः_Clauses; ++i) {
        u8 विधा;
_       (Read_u8(&विधा, &o->wasm, o->wasm_अन्तः));
        _throwif(m3Err_wasmMalformed, विधा > 0x01);

        handlers[i].विधा = विधा;

        u32 चिह्नकम्_सूचकांक;
_       (ReadLEB_u32(&चिह्नकम्_सूचकांक, &o->wasm, o->wasm_अन्तः));
        _throwif(m3_Err_unknown_चिह्नकम्, चिह्नकम्_सूचकांक >= o->module->अङ्कः_चिह्नकानि);

        IM3_चिह्नकम् चिह्नकम्         = &o->module->चिह्नकानि[चिह्नकम्_सूचकांक];
        handlers[i].चिह्नकम्    = चिह्नकम्->resolved ? चिह्नकम्->resolved : चिह्नकम्;
        handlers[i].stubPC = रिक्त;

        यदि (विधा == 0x00) {
_           (ReadLEB_u32(&clauseLabelDepths[i], &o->wasm, o->wasm_अन्तः));
        } अन्यथा {
            // (on $e switch): the tag carries no payload, and what the switch
            // site yields is what this resume yields
            _throwif(m3Err_typeMismatch, Get_Func_Type_अङ्कः_Params(handlers[i].चिह्नकम्->type) != 0);
            clauseLabelDepths[i] = 0;
        }
    }

    // the operands are read out of slots, so nothing may still be in a register
_   (PreserveRegisters(o));

    m3type_t cont_शिखरम्_Type = Get_राशिः_शिखरम्_Type(o);
    _throwif(m3Err_typeMismatch, not Is_Sub_Type_सम्बन्धि(cont_शिखरम्_Type, Ref_Type_सम्बन्धि_Func_Type(contFuncType, false)));
    u16 contSlot = Get_राशिः_शिखरम्_Slot_सङ्ख्या(o);
_   (निष्कासय(o));

    u16 exnSlot = 0;

    यदि (isThrowRef) {
        m3type_t exnType = Get_राशिः_शिखरम्_Type(o);
        _throwif(m3Err_typeMismatch, आधारः_Type_सम्बन्धि(exnType) != c_m3Type_exnref);
        exnSlot = Get_राशिः_शिखरम्_Slot_सङ्ख्या(o);
_       (निष्कासय(o));
    }

    u16 सङ्कार्यम्_Slots[d_m3MaxContinuationPayload];
    _throwif("too many resume operands", अङ्कः_Operands > d_m3MaxContinuationPayload);

    IM3FuncType सङ्कार्यम्_Type = isThrow ? exn_चिह्नकम्->type : innerType;

    प्रति (u16 i = 0; i < अङ्कः_Operands; ++i) {
        u16      pIdx         = (u16)(अङ्कः_Operands - 1 - i);
        m3type_t expectedType = GetFuncTypeParamType(सङ्कार्यम्_Type, pIdx);
        m3type_t actualType   = Get_राशिः_शिखरम्_Type(o);
        _throwif(m3Err_typeMismatch, not Is_Sub_Type_सम्बन्धि(actualType, expectedType));
        सङ्कार्यम्_Slots[pIdx] = Get_राशिः_शिखरम्_Slot_सङ्ख्या(o);
_       (निष्कासय(o));
    }

    प्रति (u32 i = 0; i < अङ्कः_Clauses; ++i) {
        यदि (handlers[i].विधा != 0x00) {
            अनुवर्त;
        }

        IM3CompilationScope scope;
_       (Get_खण्डम्_Scope(o, &scope, clauseLabelDepths[i]));

        IM3_चिह्नकम् चिह्नकम्        = handlers[i].चिह्नकम्;
        u16    अङ्कः_Payload = Get_Func_Type_अङ्कः_Params(चिह्नकम्->type);

        m3type_t labelContType = (scope->opcode == c_waOp_loop)
                                   ? GetFuncTypeParamType(scope->type, अङ्कः_Payload)
                                   : Get_Func_Type_परिणाम_Type(scope->type, अङ्कः_Payload);

        IM3_संहिता_Page stubPage;
_       (Acquire_Compilation_संहिता_Page(o, &stubPage));
        pc_t startPC  = GetPagePC(stubPage);
        displacedPage = o->page;
        o->page       = stubPage;

        u16 first_सूचकांक = o->राशिः_सूचकांक;
        प्रति (u16 a = 0; a < अङ्कः_Payload; ++a) {
_           (प्रक्षिप_Allocated_Slot(o, GetFuncTypeParamType(चिह्नकम्->type, a)));
        }

        // The handler is handed the suspended continuation.
_       (प्रक्षिप_Allocated_Slot(o, labelContType));

_       (Ensure_संहिता_Page_अङ्कः_पङ्क्तयः(o, 2 * अङ्कः_Payload + 5 + d_m3_संहिता_Page_Free_पङ्क्तयः_देहली));
_       (EmitOp(o, op_ResumePayload));
        EmitPointer(o, Module_Cont_Type_सम्बन्धि_Ref(o->module, labelContType));
        EmitConstant32(o, अङ्कः_Payload);
        प्रति (u16 a = 0; a < अङ्कः_Payload; ++a) {
            u16 index = first_सूचकांक + a;
            Emit_Slot_विस्थापनम्(o, Get_Slot_For_राशिः_सूचकांक(o, index));
            EmitConstant32(o, Is64BitType(Get_राशिः_Type_From_Bottom(o, index)));
        }
        Emit_Slot_विस्थापनम्(o, Get_Slot_For_राशिः_सूचकांक(o, first_सूचकांक + अङ्कः_Payload));

#  यदि d_m3HasExceptionHandling
        // the clause leaves every try region between the resume and its label,
        // and leaving one retires its handlers, the way a branch does
_       (Emit_निष्कासय_Try_चौकटाः(o, अङ्कः_Try_चौकटाः_गन्तव्यम्_निष्कासय(&o->खण्डम्, scope)));
#  endif

        यदि (scope->opcode == c_waOp_loop) {
_           (Resolve_खण्डम्_Results(o, scope, true));
_           (EmitOp(o, Op_ContinueLoop(o)));
            EmitPointer(o, scope->pc);
_           (लेखः_पृष्ठभागः_Edge(o, scope, false));
        } अन्यथा यदि (scope->गभीरता == 0) {
_           (Return_मानानि(o, scope, true));
_           (EmitOp(o, op_Return));
        } अन्यथा {
_           (Resolve_खण्डम्_Results(o, scope, true));
_           (EmitPatchingBranch(o, scope));
        }

        IM3_संहिता_Page return_गन्तव्यम् = displacedPage;
        displacedPage        = रिक्त;

        Release_Compilation_संहिता_Page(o);
        o->page = return_गन्तव्यम्;

        handlers[i].stubPC = startPC;

        यावत् (o->राशिः_सूचकांक > first_सूचकांक) {
_           (निष्कासय(o));
        }
    }

    u16 resSlots[d_m3MaxContinuationPayload];
    _throwif("too many cont results", अङ्कः_Cont_Results > d_m3MaxContinuationPayload);
    प्रति (u16 i = 0; i < अङ्कः_Cont_Results; ++i) {
        m3type_t rtype = Get_Func_Type_परिणाम_Type(innerType, i);
_       (प्रक्षिप_Allocated_Slot(o, rtype));
        resSlots[i] = Get_राशिः_शिखरम्_Slot_सङ्ख्या(o);
    }

_   (Ensure_संहिता_Page_अङ्कः_पङ्क्तयः(o, 8 + अङ्कः_Operands * 2 + अङ्कः_Clauses * 3 + अङ्कः_Cont_Results * 2 +
                                  d_m3_संहिता_Page_Free_पङ्क्तयः_देहली));

_   (EmitOp(o, op_Resume));
    EmitPointer(o, contFuncType);
    Emit_Slot_विस्थापनम्(o, contSlot);
    EmitConstant32(o, isThrow ? d_m3ResumeThrow : (isThrowRef ? d_m3ResumeThrowRef : d_m3_Resume_प्रसामान्यम्));

    यदि (isThrowRef) {
        Emit_Slot_विस्थापनम्(o, exnSlot);
    } अन्यथा {
        यदि (isThrow) {
            EmitPointer(o, exn_चिह्नकम्);
        }

        EmitConstant32(o, अङ्कः_Operands);
        प्रति (u16 i = 0; i < अङ्कः_Operands; ++i) {
            Emit_Slot_विस्थापनम्(o, सङ्कार्यम्_Slots[i]);
            EmitConstant32(o, Is64BitType(GetFuncTypeParamType(सङ्कार्यम्_Type, i)));
        }
    }

    EmitConstant32(o, अङ्कः_Clauses);
    प्रति (u32 i = 0; i < अङ्कः_Clauses; ++i) {
        EmitConstant32(o, handlers[i].विधा);
        EmitPointer(o, handlers[i].चिह्नकम्);
        EmitPointer(o, handlers[i].stubPC);
    }

    EmitConstant32(o, अङ्कः_Cont_Results);
    प्रति (u16 i = 0; i < अङ्कः_Cont_Results; ++i) {
        Emit_Slot_विस्थापनम्(o, resSlots[i]);
        EmitConstant32(o, Is64BitType(Get_Func_Type_परिणाम_Type(innerType, i)));
    }

    // a suspend passing through the resumed continuation leaves this frame
    // waiting here, with the continuation's results not yet taken
_   (लेखः_Safe_Point(o, safepoint_resume, 0, अङ्कः_Cont_Results, (u16)अङ्कः_Clauses, रिक्त));
#  यदि d_m3HasSnapshots
    यदि (o->snapshotMap and o->page) {
        o->snapshotMap->safePoints[o->snapshotMap->अङ्कः_Safe_Points - 1].resumeType = Ref_Type_सम्बन्धि_Func_Type(contFuncType, true);
    }
#  endif

} _catch:
    यदि (displacedPage) {
        Release_Compilation_संहिता_Page(o);
        o->page = displacedPage;
    }
    m3_Free(handlers);
    प्रत्यागम परिणाम;
}


स्थिर
M3_परिणाम Compile_Switch (IM3Compilation o, m3opcode_t i_opcode)
{
_try {
    u32 type_सूचकांक, चिह्नकम्_सूचकांक;
_   (ReadLEB_u32(&type_सूचकांक, &o->wasm, o->wasm_अन्तः));
_   (ReadLEB_u32(&चिह्नकम्_सूचकांक, &o->wasm, o->wasm_अन्तः));
    _throwif(m3_Err_type_गणनम्_Mismatch, type_सूचकांक >= o->module->अङ्कः_Func_Types);
    _throwif(m3_Err_unknown_चिह्नकम्, चिह्नकम्_सूचकांक >= o->module->अङ्कः_चिह्नकानि);

    IM3FuncType targetContType = o->module->funcTypes[type_सूचकांक];
    IM3_चिह्नकम्      चिह्नकम्            = &o->module->चिह्नकानि[चिह्नकम्_सूचकांक];
    यदि (चिह्नकम्->resolved) {
        चिह्नकम् = चिह्नकम्->resolved;
    }

    _throwif("type is not a continuation", not targetContType->isContinuation);

    // a switch tag carries no payload of its own: the values travel as the
    // peer's arguments, and the tag's results are what the switch site yields
    _throwif(m3Err_typeMismatch, Get_Func_Type_अङ्कः_Params(चिह्नकम्->type) != 0);

    // switch $ct1 : [t1* (ref null $ct1)] -> [t2*]
    //   where $ct1 = cont [t1* (ref null? $ct2)] -> [..]
    //   and   $ct2 = cont [t2*] -> [..]
    //
    // The trailing parameter of $ct1 is not an operand: the peer receives the
    // continuation this instruction suspends. And what this instruction yields
    // is not $ct1's results but $ct2's parameters - the values whoever comes
    // back this way will hand over.
    IM3FuncType targetType = targetContType->contFuncType;
    u16         अङ्कः_Params  = Get_Func_Type_अङ्कः_Params(targetType);

    _throwif("switch target takes no continuation", अङ्कः_Params == 0);

    m3type_t    peerRefType  = GetFuncTypeParamType(targetType, (u16)(अङ्कः_Params - 1));
    IM3FuncType selfContType = Module_Cont_Type_सम्बन्धि_Ref(o->module, peerRefType);

    _throwif(m3Err_typeMismatch, not selfContType);

    IM3FuncType selfType   = selfContType->contFuncType;
    u16         अङ्कः_तर्काः    = (u16)(अङ्कः_Params - 1);
    u16         अङ्कः_Results = Get_Func_Type_अङ्कः_Params(selfType);

    // the operands are read out of slots, so nothing may still be in a register
_   (PreserveRegisters(o));

    m3type_t cont_शिखरम्_Type = Get_राशिः_शिखरम्_Type(o);
    _throwif(m3Err_typeMismatch, not Is_Sub_Type_सम्बन्धि(cont_शिखरम्_Type, Ref_Type_सम्बन्धि_Func_Type(targetContType, false)));
    u16 targetContSlot = Get_राशिः_शिखरम्_Slot_सङ्ख्या(o);
_   (निष्कासय(o));

    u16 argSlots[d_m3MaxContinuationPayload];
    _throwif("too many switch params", अङ्कः_तर्काः > d_m3MaxContinuationPayload);

    प्रति (u16 i = 0; i < अङ्कः_तर्काः; ++i) {
        u16      pIdx         = (u16)(अङ्कः_तर्काः - 1 - i);
        m3type_t expectedType = GetFuncTypeParamType(targetType, pIdx);
        m3type_t actualType   = Get_राशिः_शिखरम्_Type(o);
        _throwif(m3Err_typeMismatch, not Is_Sub_Type_सम्बन्धि(actualType, expectedType));
        argSlots[pIdx] = Get_राशिः_शिखरम्_Slot_सङ्ख्या(o);
_       (निष्कासय(o));
    }

    u16 परिणाम_Slots[d_m3MaxContinuationPayload];
    _throwif("too many switch results", अङ्कः_Results > d_m3MaxContinuationPayload);

    प्रति (u16 i = 0; i < अङ्कः_Results; ++i) {
_       (प्रक्षिप_Allocated_Slot(o, GetFuncTypeParamType(selfType, i)));
        परिणाम_Slots[i] = Get_राशिः_शिखरम्_Slot_सङ्ख्या(o);
    }

_   (Ensure_संहिता_Page_अङ्कः_पङ्क्तयः(o, 6 + अङ्कः_तर्काः * 2 + अङ्कः_Results * 2 + d_m3_संहिता_Page_Free_पङ्क्तयः_देहली));
_   (EmitOp(o, op_Switch));
    EmitPointer(o, targetContType);
    EmitPointer(o, चिह्नकम्);
    Emit_Slot_विस्थापनम्(o, targetContSlot);
    EmitConstant32(o, अङ्कः_तर्काः);
    प्रति (u16 i = 0; i < अङ्कः_तर्काः; ++i) {
        Emit_Slot_विस्थापनम्(o, argSlots[i]);
        EmitConstant32(o, Is64BitType(GetFuncTypeParamType(targetType, i)));
    }
    EmitConstant32(o, अङ्कः_Results);
    प्रति (u16 i = 0; i < अङ्कः_Results; ++i) {
        Emit_Slot_विस्थापनम्(o, परिणाम_Slots[i]);
        EmitConstant32(o, Is64BitType(GetFuncTypeParamType(selfType, i)));
    }

    // as with suspend: op_Switch clears the results before it leaves
_   (लेखः_Safe_Point(o, safepoint_suspend, 0, अङ्कः_Results, 0, रिक्त));

} _catch: प्रत्यागम परिणाम;
}

#यदि_अन्तः // d_m3HasStackSwitching


स्थिर
M3_परिणाम Compile_Else_खण्डम् (IM3Compilation o, pc_t* o_startPC, IM3FuncType i_खण्डम्_Type)
{
    IM3_संहिता_Page savedPage = o->page;
_try {

    IM3_संहिता_Page elsePage;
_   (Acquire_Compilation_संहिता_Page(o, &elsePage));

    *o_startPC = GetPagePC(elsePage);

    o->page = elsePage;

_   (Compile_खण्डम्(o, i_खण्डम्_Type, c_waOp_else));

_   (EmitOp(o, op_Branch));
    EmitPointer(o, GetPagePC(savedPage));
} _catch:

    यदि (o->page != savedPage) {
        Release_Compilation_संहिता_Page(o);
    }

    o->page = savedPage;

    प्रत्यागम परिणाम;
}

स्थिर
M3_परिणाम Compile_If (IM3Compilation o, m3opcode_t i_opcode)
{
    /*      [   op_If   ]
            [ <else-pc> ]   ---->   [ ..else..  ]
            [  ..if..   ]           [ ..block.. ]
            [ ..block.. ]           [ op_Branch ]
            [    end    ]  <-----   [  <end-pc> ]       */

_try {

    // Spec: if condition must be i32
    यदि (not Is_राशिः_Polymorphic(o)) {
        m3type_t condType = Get_राशिः_शिखरम्_Type(o);
        _throwif(m3Err_typeMismatch, condType != c_m3Type_none and condType != c_m3Type_i32);
    }

_   (Preserve_Non_शिखरम्_Registers(o));
_   (Preserve_तर्काः_And_Locals(o));

#यदि d_m3FuseBranch
    IM3Operation fuseOp = GetBranchFusionOp(o, true);

    यदि (fuseOp) {
        // absorb the if into the compare that produced the condition
        *(IM3Operation*)o->foldPatchPC = fuseOp;
        InvalidateFold(o);

_       (निष्कासय(o));                          // the condition is consumed inside the fused op
    } अन्यथा
#यदि_अन्तः
    {
        IM3Operation op = Is_राशिः_शिखरम्_अन्तर्_पञ्जिकाकोशः(o) ? op_If_r : op_If_s;

_       (EmitOp(o, op));
_       (Emit_Slot_अङ्कः_सम्बन्धि_राशिः_शिखरम्_And_निष्कासय(o));
    }

    pc_t* pc = (pc_t*)ReservePointer(o);

    IM3FuncType खण्डम्_Type = रिक्त;
_   (Read_खण्डम्_Type(o, &खण्डम्_Type));     //  dump_type_stack (o);

    u16 राशिः_सूचकांक = o->राशिः_सूचकांक;

_   (Compile_खण्डम्(o, खण्डम्_Type, i_opcode));

    यदि (o->previousOpcode == c_waOp_else) {
        o->राशिः_सूचकांक = राशिः_सूचकांक;
_       (Compile_Else_खण्डम्(o, pc, खण्डम्_Type));
    } अन्यथा {
        // if block produces values and there isn't a defined else
        // case, then we need to make one up so that the pass-through
        // results end up in the right place
        यदि (Get_Func_Type_अङ्कः_Results(खण्डम्_Type)) {
            // rewind to the if's end to create a fake else block
            o->wasm--;
            o->राशिः_सूचकांक = राशिः_सूचकांक;     // dump_type_stack (o);

_           (Compile_Else_खण्डम्(o, pc, खण्डम्_Type));
        } अन्यथा {
            *pc = GetPC(o);
        }
    }

    } _catch: प्रत्यागम परिणाम;
}

स्थिर
M3_परिणाम Compile_Select (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_परिणाम परिणाम = m3Err_none;

    u16 slots[3] = { c_slot_अप्रयुक्तम्, c_slot_अप्रयुक्तम्, c_slot_अप्रयुक्तम् };

    IM3Operation op = रिक्त;

    m3type_t type = Get_राशिः_Type_From_शिखरम्(o, 1); // get type of selection

    यदि (not Is_राशिः_Polymorphic(o)) {
        // Spec: the condition operand (top) must be i32
        m3type_t condType = Get_राशिः_Type_From_शिखरम्(o, 0);
        _throwif(m3Err_typeMismatch, condType != c_m3Type_none and condType != c_m3Type_i32);

        // Spec: the two value operands (below condition) must have matching types
        m3type_t type2 = Get_राशिः_Type_From_शिखरम्(o, 2);
        _throwif(m3Err_typeMismatch, type != c_m3Type_none and type2 != c_m3Type_none and
                                       not (Is_Sub_Type_सम्बन्धि(type, type2) or Is_Sub_Type_सम्बन्धि(type2, type)));
    }

    यदि (IsFpType(type)) {
#यदि d_m3HasFloat
        // not consuming a fp reg, so preserve
        यदि (not Is_राशिः_शिखरम्_Minus1_अन्तर्_पञ्जिकाकोशः(o) and
            not Is_राशिः_शिखरम्_Minus2_अन्तर्_पञ्जिकाकोशः(o)) {
_           (Preserve_पञ्जिकाकोशः_If_Occupied(o, type));
        }

        bool selector_अन्तर्_Reg = Is_राशिः_शिखरम्_अन्तर्_पञ्जिकाकोशः(o);
        slots[0]           = Get_राशिः_शिखरम्_Slot_सङ्ख्या(o);
_       (निष्कासय(o));

        u32 op_सूचकांक = 0;

        प्रति (u32 i = 1; i <= 2; ++i) {
            यदि (Is_राशिः_शिखरम्_अन्तर्_पञ्जिकाकोशः(o)) {
                op_सूचकांक = i;
            } अन्यथा {
                slots[i] = Get_राशिः_शिखरम्_Slot_सङ्ख्या(o);
            }

_           (निष्कासय(o));
        }

        op = c_fpSelectOps[type - c_m3Type_f32][selector_अन्तर्_Reg][op_सूचकांक];
#अन्यथा
        _throw(m3Err_unknownOpcode);
#यदि_अन्तः
    } अन्यथा यदि (IsIntType(type) or IsRefType(type)) {
        // 'sss' operation doesn't consume a register, so might have to protected its contents
        यदि (not Is_राशिः_शिखरम्_अन्तर्_पञ्जिकाकोशः(o) and
            not Is_राशिः_शिखरम्_Minus1_अन्तर्_पञ्जिकाकोशः(o) and
            not Is_राशिः_शिखरम्_Minus2_अन्तर्_पञ्जिकाकोशः(o)) {
_           (Preserve_पञ्जिकाकोशः_If_Occupied(o, type));
        }

        u32 op_सूचकांक = 3;  // op_Select_*_sss

        प्रति (u32 i = 0; i < 3; ++i) {
            यदि (Is_राशिः_शिखरम्_अन्तर्_पञ्जिकाकोशः(o)) {
                op_सूचकांक = i;
            } अन्यथा {
                slots[i] = Get_राशिः_शिखरम्_Slot_सङ्ख्या(o);
            }

_           (निष्कासय(o));
        }

        // a reference is a pointer-sized integer as far as select is concerned
        u32 type_सूचकांक = IsRefType(type) ? ((M3_SIZEOF_PTR == 8) ? 1 : 0)
                                        : (u32)(type - c_m3Type_i32);

        op = c_intSelectOps[type_सूचकांक][op_सूचकांक];
    } अन्यथा यदि (not Is_राशिः_Polymorphic(o)) {
        _throw(m3_Err_function_राशिः_Underrun);
    }

    EmitOp(o, op);
    प्रति (u32 i = 0; i < 3; i++) {
        यदि (IsValidSlot(slots[i])) {
            Emit_Slot_विस्थापनम्(o, slots[i]);
        }
    }
_   (प्रक्षिप_पञ्जिकाकोशः(o, type));

    _catch: प्रत्यागम परिणाम;
}

स्थिर
M3_परिणाम Compile_Drop (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_परिणाम परिणाम = निष्कासय(o);
    यदि (d_m3_Log_Wasm_राशिः) {
        dump_type_राशिः(o);
    }
    प्रत्यागम परिणाम;
}

स्थिर
M3_परिणाम Compile_Nop (IM3Compilation o, m3opcode_t i_opcode)
{
    प्रत्यागम m3Err_none;
}

स्थिर
M3_परिणाम Compile_Unreachable (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_परिणाम परिणाम;

_   (सङ्कलय_Trap_लेखः(o));

_   (EmitOp(o, op_Unreachable));
_   (समुच्चयः_राशिः_Polymorphic(o));

    _catch:
    प्रत्यागम परिणाम;
}


// OPTZ: currently all stack slot indices take up a full word, but
// dual stack source operands could be packed together
स्थिर
M3_परिणाम Compile_Operator (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_परिणाम परिणाम;

    // Declared before the first throw below, which jumps past this point to
    // _catch and so must not skip an initialization.
    IM3Operation op = रिक्त;

    IM3OpInfo opInfo = GetOpInfo(i_opcode);
    _throwif(m3Err_unknownOpcode, not opInfo);

    // Spec: validate operand types for load/store operations
    यदि (not Is_राशिः_Polymorphic(o)) {
        // For load ops (stackOffset == 0, unary), the operand is always i32 (address)
        यदि (i_opcode >= 0x28 and i_opcode <= 0x35) {
            m3type_t शिखरम्_Type = Get_राशिः_शिखरम्_Type(o);
            _throwif(m3Err_typeMismatch, शिखरम्_Type != c_m3Type_none and शिखरम्_Type != c_m3Type_i32);
        }

        // For store ops (stackOffset == -2), the address operand must be i32
        यदि (i_opcode >= 0x36 and i_opcode <= 0x3e) {
            m3type_t addrType = Get_राशिः_Type_From_शिखरम्(o, 1);
            _throwif(m3Err_typeMismatch, addrType != c_m3Type_none and addrType != c_m3Type_i32);
        }
    }

    // This preserve is for for FP compare operations.
    // either need additional slot destination operations or the
    // easy fix, move _r0 out of the way.
    // moving out the way might be the optimal solution most often?
    // otherwise, the _r0 reg can get buried down in the stack
    // and be idle & wasted for a moment.
    यदि (M3_UNLIKELY(IsFpType(Get_राशिः_शिखरम्_Type(o)) and IsIntType(opInfo->type))) {
_       (Preserve_पञ्जिकाकोशः_If_Occupied(o, opInfo->type));
    }

    यदि (opInfo->राशिः_विस्थापनम् == 0) {
        यदि (Is_राशिः_शिखरम्_अन्तर्_पञ्जिकाकोशः(o)) {
            op = opInfo->operations[0]; // _s
        } अन्यथा {
_           (Preserve_पञ्जिकाकोशः_If_Occupied(o, opInfo->type));
            op = opInfo->operations[1]; // _r
        }
    } अन्यथा {
        यदि (Is_राशिः_शिखरम्_अन्तर्_पञ्जिकाकोशः(o)) {
            op = opInfo->operations[0];  // _rs

            यदि (Is_राशिः_शिखरम्_Minus1_अन्तर्_पञ्जिकाकोशः(o)) {    d_m3Assert (i_opcode == c_waOp_store_f32 or i_opcode == c_waOp_store_f64);
                op = opInfo->operations[3]; // _rr for fp.store
            }
        } अन्यथा यदि (Is_राशिः_शिखरम्_Minus1_अन्तर्_पञ्जिकाकोशः(o)) {
            op = opInfo->operations[1]; // _sr

            यदि (not op) {  // must be commutative, then
                op = opInfo->operations[0];
            }
        } अन्यथा {
_           (Preserve_पञ्जिकाकोशः_If_Occupied(o, opInfo->type));     // _ss
            op = opInfo->operations[2];
        }
    }

    यदि (M3_LIKELY(op)) {
_       (EmitOp(o, op));

#यदि d_m3_Fold_समुच्चयः_Local
        // the op word just written (EmitOp may have bridged pages); NULL in the page-less compile-walk mode
        pc_t patchPC = o->page ? GetPC(o) - 1 : रिक्त;
#यदि_अन्तः

_       (Emit_Slot_अङ्कः_सम्बन्धि_राशिः_शिखरम्_And_निष्कासय(o));

        यदि (opInfo->राशिः_विस्थापनम् < 0) {
_           (Emit_Slot_अङ्कः_सम्बन्धि_राशिः_शिखरम्_And_निष्कासय(o));
        }

        यदि (opInfo->type != c_m3Type_none) {
_           (प्रक्षिप_पञ्जिकाकोशः(o, opInfo->type));

#यदि d_m3_Fold_समुच्चयः_Local
            // record this op as a fold candidate for an immediately following local.set;
            // the operand-form index is recovered by matching against the variant table
            यदि (patchPC) {
                प्रति (u8 form = 0; form < 4; ++form) {
                    यदि (opInfo->operations[form] == op) {
                        o->foldPatchPC    = patchPC;
                        o->foldOpcode     = i_opcode;
                        o->foldForm       = form;
                        o->fold_राशिः_सूचकांक = (u16)Get_राशिः_शिखरम्_सूचकांक(o);
                        भङ्ग;
                    }
                }
            }
#यदि_अन्तः
        }
    } अन्यथा {
#यदि_परिभाषितम् DEBUG
        परिणाम = ErrorCompile("no operation found for opcode", o, "'%s'", opInfo->name);
#अन्यथा
        परिणाम = ErrorCompile("no operation found for opcode", o, "%x", i_opcode);
#यदि_अन्तः
        _throw(परिणाम);
    }

    _catch: प्रत्यागम परिणाम;
}

स्थिर
M3_परिणाम Compile_Convert (IM3Compilation o, m3opcode_t i_opcode)
{
_try {
    IM3OpInfo opInfo = GetOpInfo(i_opcode);
    _throwif(m3Err_unknownOpcode, not opInfo);

    // Spec: validate source operand type for conversion instructions
    यदि (not Is_राशिः_Polymorphic(o)) {
        u8 sourceType = c_m3Type_none;
        विकल्प (i_opcode)
        // clang-format off
        {
        पक्ष 0xa7:              // i32.wrap/i64
            sourceType = c_m3Type_i64; भङ्ग;
        पक्ष 0xa8: पक्ष 0xa9:   // i32.trunc_s/f32, i32.trunc_u/f32
        पक्ष 0xae: पक्ष 0xaf:   // i64.trunc_s/f32, i64.trunc_u/f32
        पक्ष 0xbb:              // f64.promote/f32
        पक्ष 0xbc:              // i32.reinterpret/f32
            sourceType = c_m3Type_f32; भङ्ग;
        पक्ष 0xaa: पक्ष 0xab:   // i32.trunc_s/f64, i32.trunc_u/f64
        पक्ष 0xb0: पक्ष 0xb1:   // i64.trunc_s/f64, i64.trunc_u/f64
        पक्ष 0xb6:              // f32.demote/f64
        पक्ष 0xbd:              // i64.reinterpret/f64
            sourceType = c_m3Type_f64; भङ्ग;
        पक्ष 0xac: पक्ष 0xad:   // i64.extend_s/i32, i64.extend_u/i32
        पक्ष 0xb2: पक्ष 0xb3:   // f32.convert_s/i32, f32.convert_u/i32
        पक्ष 0xb7: पक्ष 0xb8:   // f64.convert_s/i32, f64.convert_u/i32
        पक्ष 0xbe:              // f32.reinterpret/i32
            sourceType = c_m3Type_i32; भङ्ग;
        पक्ष 0xb4: पक्ष 0xb5:   // f32.convert_s/i64, f32.convert_u/i64
        पक्ष 0xb9: पक्ष 0xba:   // f64.convert_s/i64, f64.convert_u/i64
        पक्ष 0xbf:              // f64.reinterpret/i64
            sourceType = c_m3Type_i64; भङ्ग;
        स्वभाव: भङ्ग;
        }
        // clang-format on

        यदि (sourceType != c_m3Type_none) {
            m3type_t शिखरम्_Type = Get_राशिः_शिखरम्_Type(o);
            _throwif(m3Err_typeMismatch, शिखरम्_Type != c_m3Type_none and not Is_Sub_Type_सम्बन्धि(शिखरम्_Type, sourceType));
        }
    }

    bool dest_अन्तर्_Slot   = Is_पञ्जिकाकोशः_Type_Allocated(o, opInfo->type);
    bool source_अन्तर्_Slot = Is_राशिः_शिखरम्_अन्तर्_Slot(o);

    IM3Operation op = opInfo->operations[dest_अन्तर्_Slot * 2 + source_अन्तर्_Slot];

_   (EmitOp(o, op));
_   (Emit_Slot_अङ्कः_सम्बन्धि_राशिः_शिखरम्_And_निष्कासय(o));

    यदि (dest_अन्तर्_Slot) {
_       (प्रक्षिप_Allocated_Slot_And_Emit_2(o, opInfo->type))
    } अन्यथा {
_       (प्रक्षिप_पञ्जिकाकोशः(o, opInfo->type))
    }
}
    _catch: प्रत्यागम परिणाम;
}

#यदि d_m3HasWideArithmetic

// The wide arithmetic proposal: i64.add128 and i64.sub128 over two 128-bit values
// given as i64 halves, and i64.mul_wide_s/u giving the full product of two i64s.
// Each gives back a 128-bit result the same way, low half first. The operands are
// all read from slots, and of the two results the high half, which ends up on top,
// goes to _r0 and the low half to a newly allocated slot - so every operation has
// the one form, whatever shape the stack was in.
स्थिर
M3_परिणाम Compile_WideArithmetic (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_परिणाम परिणाम = m3Err_none;

    IM3Operation op          = op_i64_Add128;
    u32          अङ्कः_Operands = 4;

    विकल्प (i_opcode)
    // clang-format off
    {
    पक्ष c_waOp_i64Sub128:   op = op_i64_Sub128;                          भङ्ग;
    पक्ष c_waOp_i64MulWideS: op = op_i64_MultiplyWide; अङ्कः_Operands = 2;   भङ्ग;
    पक्ष c_waOp_i64MulWideU: op = op_u64_MultiplyWide; अङ्कः_Operands = 2;   भङ्ग;
    स्वभाव: भङ्ग;
    }
    // clang-format on

_   (Preserve_पञ्जिकाकोशः_If_Occupied(o, c_m3Type_i64));

_   (EmitOp(o, op));

    // the top of the stack first: the high half of the second operand, for the
    // 128-bit ones
    प्रति (u32 i = 0; i < अङ्कः_Operands; ++i) {
_       (Emit_Slot_अङ्कः_सम्बन्धि_राशिः_शिखरम्_And_निष्कासय(o));
    }

_   (प्रक्षिप_Allocated_Slot_And_Emit_2(o, c_m3Type_i64));      // the low half
_   (प्रक्षिप_पञ्जिकाकोशः(o, c_m3Type_i64));                  // the high half

    _catch: प्रत्यागम परिणाम;
}

#यदि_अन्तः // d_m3HasWideArithmetic

#यदि d_m3HasMemory64

// Replaces the 64-bit address operand of a load or store with the checked
// 32-bit effective address op_CheckAddr64 leaves behind, so that the access
// itself compiles exactly as it would against a 32-bit memory: one slot to read
// the address from, and an offset of zero folded in already.
//
// Where the address sits under other operands - the value of a store, the
// operands of an atomic - it is rewritten where it stands rather than popped:
// its stack entry is repointed at the scratch slot and retyped, and the slot it
// used to occupy released. i_numOperands counts the address and all above it.
स्थिर
M3_परिणाम EmitCheckAddr64 (IM3Compilation o, u64 i_विस्थापनम्, u16 i_अङ्कः_Operands)
{
    M3_परिणाम परिणाम = m3Err_none;

    // The address has to be in a slot for op_CheckAddr64 to read it. Spilling
    // r0 wholesale also keeps the value of a store out of it, which is what
    // leaves the address as the one operand still in a register to worry about.
_   (Preserve_पञ्जिकाकोशः_If_Occupied(o, c_m3Type_i64));

    // unreachable code: the operands the instruction names may not be there
    यदि (o->राशिः_सूचकांक < o->खण्डम्.खण्डम्_राशिः_सूचकांक + i_अङ्कः_Operands) {
        प्रत्यागम परिणाम;
    }

    {
        u16      राशिः_सूचकांक = (u16)(o->राशिः_सूचकांक - i_अङ्कः_Operands);
        u16      srcSlot    = o->wasm_राशिः[राशिः_सूचकांक];
        m3type_t srcType    = o->type_राशिः[राशिः_सूचकांक];

        u16 dstSlot = c_slot_अप्रयुक्तम्;

        // Checked here rather than left to the validator, which a build may
        // have switched off: reading a slot as an i64 that only holds an i32
        // would reach past what was allocated for it.
        _throwif(m3Err_typeMismatch, आधारः_Type_सम्बन्धि(srcType) != c_m3Type_i64);

        // an address is an integer, so nothing should be left in a register
        // once r0 has been preserved
        _throwif(m3Err_typeMismatch, Is_पञ्जिकाकोशः_Slot_Alias(srcSlot));
        _throwif(m3_Err_function_राशिः_Underrun, srcSlot >= o->slot_Max_Allocated_सूचकांक_Plus_One);

        // allocated before the source is released, so the two cannot be the
        // same slot - op_CheckAddr64 reads one and writes the other
_       (AllocateSlots(o, &dstSlot, c_m3Type_i32));

_       (EmitOp(o, op_CheckAddr64));
        Emit_Slot_विस्थापनम्(o, srcSlot);
        Emit_Slot_विस्थापनम्(o, dstSlot);
        EmitConstant64(o, i_विस्थापनम्);

        o->wasm_राशिः[राशिः_सूचकांक] = dstSlot;
        o->type_राशिः[राशिः_सूचकांक] = c_m3Type_i32;

        // locals and the constant table outlive the instruction; a dynamic
        // slot the address was computed into does not
        यदि (srcSlot >= o->slot_First_Dynamic_सूचकांक) {
            DeallocateSlot(o, srcSlot, srcType);
        }
    }

    _catch: प्रत्यागम परिणाम;
}

#यदि_अन्तः // d_m3HasMemory64


स्थिर
M3_परिणाम Compile_आरोपय_Store (IM3Compilation o, m3opcode_t i_opcode)
{
_try {
    u32 alignHint, स्मृतिः_Idx;
    u64 स्मृतिः_विस्थापनम्;

    // alignHint is checked by the validator
_   (Read_स्मृतिः_Arg(&alignHint, &स्मृतिः_Idx, &स्मृतिः_विस्थापनम्, &o->wasm, o->wasm_अन्तः));
                                                                        m3log (compile, d_indent " (memory = %d; offset = %llu)", get_indention_string (o), स्मृतिः_Idx, (अचिह्नित दीर्घ दीर्घ) स्मृतिः_विस्थापनम्);
    _throwif(m3_Err_unknown_स्मृतिः, स्मृतिः_Idx >= o->module->अङ्कः_Memories);

    IM3OpInfo opInfo = GetOpInfo(i_opcode);
    _throwif(m3Err_unknownOpcode, not opInfo);

    bool isMemory64 = o->module->memories[स्मृतिः_Idx]->isMemory64;

    // Spec: the static offset has to be in range of the address type. Checked
    // here as well as in the validator, so that a build without one still
    // refuses the module rather than silently truncating the offset.
    _throwif(m3Err_wasmMalformed, not isMemory64 and स्मृतिः_विस्थापनम् > 0xFFFFFFFFull);

    यदि (IsFpType(opInfo->type)) {
_       (Preserve_पञ्जिकाकोशः_If_Occupied(o, c_m3Type_f64));
    }

    यदि (स्मृतिः_Idx) {
_       (Emit_समुच्चयः_स्मृतिः(o, स्मृतिः_Idx));
    }

#यदि d_m3HasMemory64
    यदि (isMemory64) {
        // An offset this far out puts every address out of bounds, so rather
        // than carrying it, clamp it to the limit: op_CheckAddr64 then traps
        // on whatever address it is given, which is the outcome either way.
        यदि (स्मृतिः_विस्थापनम् > d_m3_स्थानाङ्कः_सीमा) {
            स्मृतिः_विस्थापनम् = d_m3_स्थानाङ्कः_सीमा;
        }

_       (EmitCheckAddr64(o, स्मृतिः_विस्थापनम्, opInfo->राशिः_विस्थापनम् < 0 ? 2 : 1));

        // op_CheckAddr64 has folded the offset in already
        स्मृतिः_विस्थापनम् = 0;
    }
#यदि_अन्तः

_   (Compile_Operator(o, i_opcode));

    EmitConstant32(o, (u32)स्मृतिः_विस्थापनम्);

    यदि (स्मृतिः_Idx) {
_       (Emit_समुच्चयः_स्मृतिः(o, 0));
    }
}
    _catch: प्रत्यागम परिणाम;
}


#यदि d_m3HasAtomics

// What an atomic instruction takes and gives back, from its sub-opcode. The
// accesses come in runs of seven, one per combination of value type and cell width:
// i32/32, i64/64, i32/8, i32/16, i64/8, i64/16, i64/32. Those runs are told apart by
// where they start.
//
// The operand count includes the address. The top operand is the one that ends up
// in a register, so its type is the one to check on the way out; a store has no
// result, which is c_m3Type_none.
स्थिर
शून्य GetAtomicShape (u32 i_sub, u16* o_अङ्कः_Operands, m3type_t* o_शिखरम्_Type, m3type_t* o_परिणाम_Type)
{
    // clang-format off
    स्थिर ध्रुव m3type_t c_मानम्_Types [7] = {
        c_m3Type_i32, c_m3Type_i64,
        c_m3Type_i32, c_m3Type_i32,
        c_m3Type_i64, c_m3Type_i64, c_m3Type_i64
    };
    // clang-format on

    यदि (i_sub == 0x00) {                    // memory.atomic.notify: address, count
        *o_अङ्कः_Operands = 2;
        *o_शिखरम्_Type     = c_m3Type_i32;
        *o_परिणाम_Type  = c_m3Type_i32;
    } अन्यथा यदि (i_sub <= 0x02) {             // memory.atomic.wait32/64: address, expected, timeout
        *o_अङ्कः_Operands = 3;
        *o_शिखरम्_Type     = c_m3Type_i64;
        *o_परिणाम_Type  = c_m3Type_i32;
    } अन्यथा यदि (i_sub <= 0x16) {             // loads: address
        *o_अङ्कः_Operands = 1;
        *o_शिखरम्_Type     = c_m3Type_i32;
        *o_परिणाम_Type  = c_मानम्_Types[i_sub - 0x10];
    } अन्यथा यदि (i_sub <= 0x1d) {             // stores: address, value
        *o_अङ्कः_Operands = 2;
        *o_शिखरम्_Type     = c_मानम्_Types[i_sub - 0x17];
        *o_परिणाम_Type  = c_m3Type_none;
    } अन्यथा यदि (i_sub <= 0x47) {             // read-modify-write: address, value
        *o_अङ्कः_Operands = 2;
        *o_शिखरम्_Type     = c_मानम्_Types[(i_sub - 0x1e) % 7];
        *o_परिणाम_Type  = *o_शिखरम्_Type;
    } अन्यथा {                                // cmpxchg: address, expected, replacement
        *o_अङ्कः_Operands = 3;
        *o_शिखरम्_Type     = c_मानम्_Types[i_sub - 0x48];
        *o_परिणाम_Type  = *o_शिखरम्_Type;
    }
}


// The description an access takes as its last immediate: see m3_exec_atomic.h
स्थिर
u32 GetAtomicDesc (u32 i_sub)
{
    // per variant, the log2 of the width of the cell
    स्थिर ध्रुव u8 c_log2Widths[7] = { 2, 3, 0, 1, 0, 1, 2 };

    u32 variant, विधा = 0;

    यदि (i_sub <= 0x16) {
        variant = i_sub - 0x10;
    } अन्यथा यदि (i_sub <= 0x1d) {
        variant = i_sub - 0x17;
    } अन्यथा यदि (i_sub <= 0x47) {
        variant = (i_sub - 0x1e) % 7;
        विधा    = (i_sub - 0x1e) / 7;         // add, sub, and, or, xor, xchg
    } अन्यथा {
        variant = i_sub - 0x48;
    }

    // variants 1 and 4..6 are the i64 ones
    u32 is64 = (variant == 1 or variant >= 4) ? 1u : 0u;

    प्रत्यागम c_log2Widths[variant] | (is64 << 2) | (विधा << 3);
}


// Every atomic access is one op, whose operands are the topmost in _r0 and the rest
// in slots, topmost first. The address is always the deepest, so it is the last slot
// (or, for a load, the register), followed by the memarg offset. The ops check the
// address themselves: for an atomic that includes its alignment, which only the
// effective address can be asked.
स्थिर
M3_परिणाम Compile_Atomic (IM3Compilation o, m3opcode_t i_opcode)
{
_try {
    u32 alignHint, स्मृतिः_Idx;
    u64 स्मृतिः_विस्थापनम्;

    u16      अङ्कः_Operands = 0;
    m3type_t शिखरम्_Type     = c_m3Type_none;
    m3type_t परिणाम_Type  = c_m3Type_none;

    यदि (i_opcode == c_waOp_atomicFence) {
        // the reserved byte, which the validator has checked to be zero. There
        // is nothing to order between accesses that one thread makes in sequence.
        u8 reserved;
_       (Read_u8(&reserved, &o->wasm, o->wasm_अन्तः));
    } अन्यथा {
        // alignHint is checked by the validator
_       (Read_स्मृतिः_Arg(&alignHint, &स्मृतिः_Idx, &स्मृतिः_विस्थापनम्, &o->wasm, o->wasm_अन्तः));
                                                                        m3log (compile, d_indent " (memory = %d; offset = %llu)", get_indention_string (o), स्मृतिः_Idx, (अचिह्नित दीर्घ दीर्घ) स्मृतिः_विस्थापनम्);
        _throwif(m3_Err_unknown_स्मृतिः, स्मृतिः_Idx >= o->module->अङ्कः_Memories);

        IM3OpInfo opInfo = GetOpInfo(i_opcode);
        _throwif(m3Err_unknownOpcode, not opInfo);

        bool isMemory64 = o->module->memories[स्मृतिः_Idx]->isMemory64;

        // see Compile_Load_Store
        _throwif(m3Err_wasmMalformed, not isMemory64 and स्मृतिः_विस्थापनम् > 0xFFFFFFFFull);

        GetAtomicShape(i_opcode & 0xFF, &अङ्कः_Operands, &शिखरम्_Type, &परिणाम_Type);

        यदि (स्मृतिः_Idx) {
_           (Emit_समुच्चयः_स्मृतिः(o, स्मृतिः_Idx));
        }

#  यदि d_m3HasMemory64
        यदि (isMemory64) {
            // see Compile_Load_Store
            यदि (स्मृतिः_विस्थापनम् > d_m3_स्थानाङ्कः_सीमा) {
                स्मृतिः_विस्थापनम् = d_m3_स्थानाङ्कः_सीमा;
            }

_           (EmitCheckAddr64(o, स्मृतिः_विस्थापनम्, अङ्कः_Operands));
            स्मृतिः_विस्थापनम् = 0;
        }
#  endif

_       (Copy_राशिः_शिखरम्_गन्तव्यम्_पञ्जिकाकोशः(o, false));
_       (EmitOp(o, opInfo->operations[0]));
_       (निष्कासय_Type(o, शिखरम्_Type));

        प्रति (u16 i = 1; i < अङ्कः_Operands; ++i) {
_           (Emit_Slot_अङ्कः_सम्बन्धि_राशिः_शिखरम्_And_निष्कासय(o));
        }

        EmitConstant32(o, (u32)स्मृतिः_विस्थापनम्);
        यदि (i_opcode >= c_waOp_atomicFirstAccess) {
            EmitConstant32(o, GetAtomicDesc(i_opcode & 0xFF));
        }

        यदि (स्मृतिः_Idx) {
_           (Emit_समुच्चयः_स्मृतिः(o, 0));
        }

        यदि (परिणाम_Type != c_m3Type_none) {
_           (प्रक्षिप_पञ्जिकाकोशः(o, परिणाम_Type));
        }
    }
}
    _catch: प्रत्यागम परिणाम;
}

#यदि_अन्तः // d_m3HasAtomics


M3_परिणाम CompileRawFunction (IM3Module io_module, IM3Function io_function, ध्रुव शून्य* i_function, ध्रुव शून्य* i_userdata)
{
    d_m3Assert(io_module->runtime);

    IM3_संहिता_Page page = Acquire_संहिता_Page_With_क्षमता(io_module->runtime, 4);

    यदि (page) {
        io_function->compiled = GetPagePC(page);
        io_function->module   = io_module;

        // Unless a host module's ABI names a memory (m3_BindImportMemory), a
        // host function addresses memory 0 - what a bare i32 guest pointer means
        io_function->host_स्मृतिः = io_module->memory0;

        EmitWord(page, op_CallRawFunction);
        EmitWord(page, i_function);
        EmitWord(page, io_function);
        EmitWord(page, i_userdata);

        Release_संहिता_Page(io_module->runtime, page);
        प्रत्यागम m3Err_none;
    } अन्यथा {
        प्रत्यागम m3_Err_malloc_Failed_संहिता_Page;
    }
}


// A table entry names every field it sets, so the struct can order them for
// packing; the columns follow that order, since M3OP names them positionally.
//
// d_logOp, d_logOp2 macros aren't actually used by the compiler, just codepage decoding (d_m3LogCodePages = 1)
#परिभाषय d_ops(...)                          .operations = { __VA_ARGS__ }

#परिभाषय d_logOp(OP)                         d_ops( op_##OP,                 रिक्त,                       रिक्त,                       रिक्त )
#परिभाषय d_logOp2(OP1,OP2)                   d_ops( op_##OP1,                op_##OP2,                   रिक्त,                       रिक्त )

#परिभाषय d_रिक्तानि_Op_List                       d_ops( रिक्त,                    रिक्त,                       रिक्त,                       रिक्त )
#परिभाषय d_unaryOpList(TYPE, NAME)           d_ops( op_##TYPE##_##NAME##_r,  op_##TYPE##_##NAME##_s,     रिक्त,                       रिक्त )
#परिभाषय d_binOpList(TYPE, NAME)             d_ops( op_##TYPE##_##NAME##_rs, op_##TYPE##_##NAME##_sr,    op_##TYPE##_##NAME##_ss,    रिक्त )
#परिभाषय d_storeFpOpList(TYPE, NAME)         d_ops( op_##TYPE##_##NAME##_rs, op_##TYPE##_##NAME##_sr,    op_##TYPE##_##NAME##_ss,    op_##TYPE##_##NAME##_rr )
#परिभाषय d_commutativeBinOpList(TYPE, NAME)  d_ops( op_##TYPE##_##NAME##_rs, रिक्त,                       op_##TYPE##_##NAME##_ss,    रिक्त )

#परिभाषय d_convertOpList(OP)                 d_ops( op_##OP##_r_r,           op_##OP##_r_s,              op_##OP##_s_r,              op_##OP##_s_s )


// Every M3Compiler the tables below name, as one list, so that the index enum and
// the array of pointers can never drift apart. Each entry must carry the same
// config guard as the function's definition.
#परिभाषय d_m3CompilerList(_)             \
    _( Compile_Unreachable )            \
    _( Compile_Nop )                    \
    _( Compile_Loop_Or_खण्डम् )            \
    _( Compile_If )                     \
    _( Compile_अन्तः )                    \
    _( Compile_Branch )                 \
    _( Compile_Branch_सारणी )            \
    _( Compile_Return )                 \
    _( Compile_Call )                   \
    _( Compile_Call_परोक्षम् )           \
    _( Compile_Drop )                   \
    _( Compile_Select )                 \
    _( Compile_GetLocal )               \
    _( Compile_समुच्चयः_Local )               \
    _( Compile_Get_समुच्चयः_Global )           \
    _( Compile_आरोपय_Store )             \
    _( Compile_स्मृतिः_आकार )            \
    _( Compile_स्मृतिः_Grow )            \
    _( Compile_स्मृतिः_Init )            \
    _( Compile_स्मृतिः_Copy_Fill )        \
    _( Compile_दत्त_Drop )              \
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
    d_m3_Compiler_List_राशिः_Switching( _ )

#यदि d_m3ImplementFloat
#  define d_m3CompilerList_f(_)         \
    _( Compile_Const_f32 )              \
    _( Compile_Const_f64 )
#अन्यथा
#  define d_m3CompilerList_f(_)
#यदि_अन्तः

#यदि d_m3HasRefTypes
#  define d_m3CompilerList_refTypes(_)  \
    _( Compile_Select_Typed )           \
    _( Compile_Ref_Null )               \
    _( Compile_Ref_IsNull )             \
    _( Compile_Ref_Func )               \
    _( Compile_सारणी_Get_समुच्चयः )           \
    _( Compile_सारणी_Init )             \
    _( Compile_सारणी_Copy )             \
    _( Compile_सारणी_आकार )             \
    _( Compile_सारणी_Grow_Fill )         \
    _( Compile_Elem_Drop )
#अन्यथा
#  define d_m3CompilerList_refTypes(_)
#यदि_अन्तः

#यदि d_m3HasTypedRefs
#  define d_m3CompilerList_typedRefs(_) \
    _( Compile_CallRef )                \
    _( Compile_Ref_AsNonNull )
#अन्यथा
#  define d_m3CompilerList_typedRefs(_)
#यदि_अन्तः

#यदि d_m3HasExceptionHandling
#  define d_m3CompilerList_eh(_)        \
    _( Compile_Throw )                  \
    _( Compile_ThrowRef )               \
    _( Compile_Try_सारणी )
#अन्यथा
#  define d_m3CompilerList_eh(_)
#यदि_अन्तः

#यदि d_m3HasAtomics
#  define d_m3CompilerList_atomics(_)   \
    _( Compile_Atomic )
#अन्यथा
#  define d_m3CompilerList_atomics(_)
#यदि_अन्तः

#यदि d_m3HasWideArithmetic
#  define d_m3CompilerList_wide(_)      \
    _( Compile_WideArithmetic )
#अन्यथा
#  define d_m3CompilerList_wide(_)
#यदि_अन्तः

#यदि d_m3_Has_राशिः_Switching
#  define d_m3_Compiler_List_राशिः_Switching(_) \
    _( Compile_ContNew )                     \
    _( Compile_ContBind )                    \
    _( Compile_Suspend )                     \
    _( Compile_Resume )                      \
    _( Compile_Switch )
#अन्यथा
#  define d_m3_Compiler_List_राशिः_Switching(_)
#यदि_अन्तः

// Index 0 is "this opcode has no compiler", so that a zeroed entry - which is
// what M3OP_RESERVED and the reserved holes in the tables are - reads as one.
गणना {
    c_cc_NULL = 0,
#परिभाषय d_m3CompilerEnum(NAME)  c_cc_##NAME,
    d_m3CompilerList(d_m3CompilerEnum)
#अपरिभाषय d_m3CompilerEnum
};

स्थिर ध्रुव M3Compiler c_compilers[] = {
    रिक्त,
#परिभाषय d_m3CompilerPtr(NAME)   NAME,
    d_m3CompilerList(d_m3CompilerPtr)
#अपरिभाषय d_m3CompilerPtr
};

// Names a table entry's compiler by index. d_cc (NULL) is the no-compiler entry.
#परिभाषय d_cc(NAME)                          .compiler = c_cc_##NAME

स्थिर
M3Compiler GetCompiler (IM3OpInfo i_opInfo)
{
    d_m3Assert(i_opInfo->compiler < M3_गणनम्_सम्बन्धि(c_compilers));
    प्रत्यागम c_compilers[i_opInfo->compiler];
}

// clang-format off
ध्रुव M3OpInfo c_operations[] =
{
    M3OP( "unreachable",           0,  none,   d_cc(Compile_Unreachable),  d_logOp (Unreachable) ),                   // 0x00
    M3OP( "nop",                   0,  none,   d_cc(Compile_Nop),          d_रिक्तानि_Op_List ),                           // 0x01 .
    M3OP( "block",                 0,  none,   d_cc(Compile_Loop_Or_खण्डम्),  d_रिक्तानि_Op_List ),                           // 0x02
    M3OP( "loop",                  0,  none,   d_cc(Compile_Loop_Or_खण्डम्),  d_logOp (Loop) ),                          // 0x03
    M3OP( "if",                    -1, none,   d_cc(Compile_If),           d_रिक्तानि_Op_List ),                           // 0x04
    M3OP( "else",                  0,  none,   d_cc(Compile_Nop),          d_रिक्तानि_Op_List ),                           // 0x05

    M3OP_RESERVED,  M3OP_RESERVED,                                                                                    // 0x06...0x07

#यदि d_m3HasExceptionHandling
    M3OP( "throw",                 0,  none,   d_cc(Compile_Throw),        d_logOp (Throw) ),                         // 0x08
    M3OP_RESERVED,                                                                                                    // 0x09
    M3OP( "throw_ref",             0,  none,   d_cc(Compile_ThrowRef),     d_logOp (ThrowRef) ),                      // 0x0a
#अन्यथा
    M3OP_RESERVED,  M3OP_RESERVED, M3OP_RESERVED,                                                                     // 0x08...0x0a
#यदि_अन्तः

    M3OP( "end",                   0,  none,   d_cc(Compile_अन्तः),          d_रिक्तानि_Op_List ),                           // 0x0b
    M3OP( "br",                    0,  none,   d_cc(Compile_Branch),       d_logOp (Branch) ),                        // 0x0c
    M3OP( "br_if",                 -1, none,   d_cc(Compile_Branch),       d_logOp2 (BranchIf_r, BranchIf_s) ),       // 0x0d
    M3OP( "br_table",              -1, none,   d_cc(Compile_Branch_सारणी),  d_logOp (Branch_सारणी) ),                   // 0x0e
    M3OP( "return",                0,  any,    d_cc(Compile_Return),       d_logOp (Return) ),                        // 0x0f
    M3OP( "call",                  0,  any,    d_cc(Compile_Call),         d_logOp (Call) ),                          // 0x10
    M3OP( "call_indirect",         0,  any,    d_cc(Compile_Call_परोक्षम्), d_logOp (Call_परोक्षम्) ),                  // 0x11
    M3OP( "return_call",           0,  any,    d_cc(Compile_Call),         d_logOp (ReturnCall) ),                    // 0x12
    M3OP( "return_call_indirect",  0,  any,    d_cc(Compile_Call_परोक्षम्), d_logOp (Return_Call_परोक्षम्) ),            // 0x13

#यदि d_m3HasTypedRefs
    M3OP( "call_ref",              0,  any,    d_cc(Compile_CallRef),      d_logOp (CallRef) ),                       // 0x14
    M3OP( "return_call_ref",       0,  any,    d_cc(Compile_CallRef),      d_logOp (ReturnCallRef) ),                 // 0x15
#अन्यथा
    M3OP_RESERVED,  M3OP_RESERVED,                                                                                    // 0x14...
#यदि_अन्तः
    M3OP_RESERVED,  M3OP_RESERVED, M3OP_RESERVED, M3OP_RESERVED,                                                      // ...0x19

    M3OP( "drop",                  -1, none,   d_cc(Compile_Drop),         d_रिक्तानि_Op_List ),                           // 0x1a
    M3OP( "select",                -2, any,    d_cc(Compile_Select),       d_रिक्तानि_Op_List ),                           // 0x1b

#यदि d_m3HasRefTypes
    M3OP( "select.t",              -2, any,    d_cc(Compile_Select_Typed), d_रिक्तानि_Op_List ),                           // 0x1c
#अन्यथा
    M3OP_RESERVED,                                                                                                    // 0x1c
#यदि_अन्तः
    M3OP_RESERVED,  M3OP_RESERVED,                                                                                    // 0x1d...0x1e

#यदि d_m3HasExceptionHandling
    M3OP( "try_table",             0,  none,   d_cc(Compile_Try_सारणी),     d_logOp (Try_सारणी) ),                      // 0x1f
#अन्यथा
    M3OP_RESERVED,                                                                                                    // 0x1f
#यदि_अन्तः

    M3OP( "local.get",             1,  any,    d_cc(Compile_GetLocal),     d_रिक्तानि_Op_List ),                           // 0x20
    M3OP( "local.set",             1,  none,   d_cc(Compile_समुच्चयः_Local),     d_रिक्तानि_Op_List ),                           // 0x21
    M3OP( "local.tee",             0,  any,    d_cc(Compile_समुच्चयः_Local),     d_रिक्तानि_Op_List ),                           // 0x22
    M3OP( "global.get",            1,  none,   d_cc(Compile_Get_समुच्चयः_Global), d_रिक्तानि_Op_List ),                           // 0x23
    M3OP( "global.set",            1,  none,   d_cc(Compile_Get_समुच्चयः_Global), d_रिक्तानि_Op_List ),                           // 0x24

#यदि d_m3HasRefTypes
    M3OP( "table.get",             0,  any,    d_cc(Compile_सारणी_Get_समुच्चयः), d_रिक्तानि_Op_List ),                           // 0x25
    M3OP( "table.set",             -2, none,   d_cc(Compile_सारणी_Get_समुच्चयः), d_रिक्तानि_Op_List ),                           // 0x26
#अन्यथा
    M3OP_RESERVED,  M3OP_RESERVED,                                                                                    // 0x25...0x26
#यदि_अन्तः
    M3OP_RESERVED,                                                                                                    // 0x27

    M3OP( "i32.load",              0,  i_32,   d_cc(Compile_आरोपय_Store),   d_unaryOpList (i32, आरोपय_i32) ),           // 0x28
    M3OP( "i64.load",              0,  i_64,   d_cc(Compile_आरोपय_Store),   d_unaryOpList (i64, आरोपय_i64) ),           // 0x29
    M3OP_F( "f32.load",            0,  f_32,   d_cc(Compile_आरोपय_Store),   d_unaryOpList (f32, आरोपय_f32) ),           // 0x2a
    M3OP_F( "f64.load",            0,  f_64,   d_cc(Compile_आरोपय_Store),   d_unaryOpList (f64, आरोपय_f64) ),           // 0x2b

    M3OP( "i32.load8_s",           0,  i_32,   d_cc(Compile_आरोपय_Store),   d_unaryOpList (i32, आरोपय_i8) ),            // 0x2c
    M3OP( "i32.load8_u",           0,  i_32,   d_cc(Compile_आरोपय_Store),   d_unaryOpList (i32, आरोपय_u8) ),            // 0x2d
    M3OP( "i32.load16_s",          0,  i_32,   d_cc(Compile_आरोपय_Store),   d_unaryOpList (i32, आरोपय_i16) ),           // 0x2e
    M3OP( "i32.load16_u",          0,  i_32,   d_cc(Compile_आरोपय_Store),   d_unaryOpList (i32, आरोपय_u16) ),           // 0x2f

    M3OP( "i64.load8_s",           0,  i_64,   d_cc(Compile_आरोपय_Store),   d_unaryOpList (i64, आरोपय_i8) ),            // 0x30
    M3OP( "i64.load8_u",           0,  i_64,   d_cc(Compile_आरोपय_Store),   d_unaryOpList (i64, आरोपय_u8) ),            // 0x31
    M3OP( "i64.load16_s",          0,  i_64,   d_cc(Compile_आरोपय_Store),   d_unaryOpList (i64, आरोपय_i16) ),           // 0x32
    M3OP( "i64.load16_u",          0,  i_64,   d_cc(Compile_आरोपय_Store),   d_unaryOpList (i64, आरोपय_u16) ),           // 0x33
    M3OP( "i64.load32_s",          0,  i_64,   d_cc(Compile_आरोपय_Store),   d_unaryOpList (i64, आरोपय_i32) ),           // 0x34
    M3OP( "i64.load32_u",          0,  i_64,   d_cc(Compile_आरोपय_Store),   d_unaryOpList (i64, आरोपय_u32) ),           // 0x35

    M3OP( "i32.store",             -2, none,   d_cc(Compile_आरोपय_Store),   d_binOpList (i32, Store_i32) ),            // 0x36
    M3OP( "i64.store",             -2, none,   d_cc(Compile_आरोपय_Store),   d_binOpList (i64, Store_i64) ),            // 0x37
    M3OP_F( "f32.store",           -2, none,   d_cc(Compile_आरोपय_Store),   d_storeFpOpList (f32, Store_f32) ),        // 0x38
    M3OP_F( "f64.store",           -2, none,   d_cc(Compile_आरोपय_Store),   d_storeFpOpList (f64, Store_f64) ),        // 0x39

    M3OP( "i32.store8",            -2, none,   d_cc(Compile_आरोपय_Store),   d_binOpList (i32, Store_u8) ),             // 0x3a
    M3OP( "i32.store16",           -2, none,   d_cc(Compile_आरोपय_Store),   d_binOpList (i32, Store_i16) ),            // 0x3b

    M3OP( "i64.store8",            -2, none,   d_cc(Compile_आरोपय_Store),   d_binOpList (i64, Store_u8) ),             // 0x3c
    M3OP( "i64.store16",           -2, none,   d_cc(Compile_आरोपय_Store),   d_binOpList (i64, Store_i16) ),            // 0x3d
    M3OP( "i64.store32",           -2, none,   d_cc(Compile_आरोपय_Store),   d_binOpList (i64, Store_i32) ),            // 0x3e

    M3OP( "memory.size",           1,  i_32,   d_cc(Compile_स्मृतिः_आकार),  d_logOp (Mem_आकार) ),                       // 0x3f
    M3OP( "memory.grow",           1,  i_32,   d_cc(Compile_स्मृतिः_Grow),  d_logOp (MemGrow) ),                       // 0x40

    M3OP( "i32.const",             1,  i_32,   d_cc(Compile_Const_i32),    d_logOp (Const32) ),                       // 0x41
    M3OP( "i64.const",             1,  i_64,   d_cc(Compile_Const_i64),    d_logOp (Const64) ),                       // 0x42
    M3OP_F( "f32.const",           1,  f_32,   d_cc(Compile_Const_f32),    d_रिक्तानि_Op_List ),                           // 0x43
    M3OP_F( "f64.const",           1,  f_64,   d_cc(Compile_Const_f64),    d_रिक्तानि_Op_List ),                           // 0x44

    M3OP( "i32.eqz",               0,  i_32,   d_cc(रिक्त),                 d_unaryOpList (i32, Equal_गन्तव्यम्_Zero) ),        // 0x45
    M3OP( "i32.eq",                -1, i_32,   d_cc(रिक्त),                 d_commutativeBinOpList (i32, Equal) ),     // 0x46
    M3OP( "i32.ne",                -1, i_32,   d_cc(रिक्त),                 d_commutativeBinOpList (i32, NotEqual) ),  // 0x47
    M3OP( "i32.lt_s",              -1, i_32,   d_cc(रिक्त),                 d_binOpList (i32, LessThan) ),             // 0x48
    M3OP( "i32.lt_u",              -1, i_32,   d_cc(रिक्त),                 d_binOpList (u32, LessThan) ),             // 0x49
    M3OP( "i32.gt_s",              -1, i_32,   d_cc(रिक्त),                 d_binOpList (i32, GreaterThan) ),          // 0x4a
    M3OP( "i32.gt_u",              -1, i_32,   d_cc(रिक्त),                 d_binOpList (u32, GreaterThan) ),          // 0x4b
    M3OP( "i32.le_s",              -1, i_32,   d_cc(रिक्त),                 d_binOpList (i32, LessThanOrEqual) ),      // 0x4c
    M3OP( "i32.le_u",              -1, i_32,   d_cc(रिक्त),                 d_binOpList (u32, LessThanOrEqual) ),      // 0x4d
    M3OP( "i32.ge_s",              -1, i_32,   d_cc(रिक्त),                 d_binOpList (i32, GreaterThanOrEqual) ),   // 0x4e
    M3OP( "i32.ge_u",              -1, i_32,   d_cc(रिक्त),                 d_binOpList (u32, GreaterThanOrEqual) ),   // 0x4f

    M3OP( "i64.eqz",               0,  i_32,   d_cc(रिक्त),                 d_unaryOpList (i64, Equal_गन्तव्यम्_Zero) ),        // 0x50
    M3OP( "i64.eq",                -1, i_32,   d_cc(रिक्त),                 d_commutativeBinOpList (i64, Equal) ),     // 0x51
    M3OP( "i64.ne",                -1, i_32,   d_cc(रिक्त),                 d_commutativeBinOpList (i64, NotEqual) ),  // 0x52
    M3OP( "i64.lt_s",              -1, i_32,   d_cc(रिक्त),                 d_binOpList (i64, LessThan) ),             // 0x53
    M3OP( "i64.lt_u",              -1, i_32,   d_cc(रिक्त),                 d_binOpList (u64, LessThan) ),             // 0x54
    M3OP( "i64.gt_s",              -1, i_32,   d_cc(रिक्त),                 d_binOpList (i64, GreaterThan) ),          // 0x55
    M3OP( "i64.gt_u",              -1, i_32,   d_cc(रिक्त),                 d_binOpList (u64, GreaterThan) ),          // 0x56
    M3OP( "i64.le_s",              -1, i_32,   d_cc(रिक्त),                 d_binOpList (i64, LessThanOrEqual) ),      // 0x57
    M3OP( "i64.le_u",              -1, i_32,   d_cc(रिक्त),                 d_binOpList (u64, LessThanOrEqual) ),      // 0x58
    M3OP( "i64.ge_s",              -1, i_32,   d_cc(रिक्त),                 d_binOpList (i64, GreaterThanOrEqual) ),   // 0x59
    M3OP( "i64.ge_u",              -1, i_32,   d_cc(रिक्त),                 d_binOpList (u64, GreaterThanOrEqual) ),   // 0x5a

    M3OP_F( "f32.eq",              -1, i_32,   d_cc(रिक्त),                 d_commutativeBinOpList (f32, Equal) ),     // 0x5b
    M3OP_F( "f32.ne",              -1, i_32,   d_cc(रिक्त),                 d_commutativeBinOpList (f32, NotEqual) ),  // 0x5c
    M3OP_F( "f32.lt",              -1, i_32,   d_cc(रिक्त),                 d_binOpList (f32, LessThan) ),             // 0x5d
    M3OP_F( "f32.gt",              -1, i_32,   d_cc(रिक्त),                 d_binOpList (f32, GreaterThan) ),          // 0x5e
    M3OP_F( "f32.le",              -1, i_32,   d_cc(रिक्त),                 d_binOpList (f32, LessThanOrEqual) ),      // 0x5f
    M3OP_F( "f32.ge",              -1, i_32,   d_cc(रिक्त),                 d_binOpList (f32, GreaterThanOrEqual) ),   // 0x60

    M3OP_F( "f64.eq",              -1, i_32,   d_cc(रिक्त),                 d_commutativeBinOpList (f64, Equal) ),     // 0x61
    M3OP_F( "f64.ne",              -1, i_32,   d_cc(रिक्त),                 d_commutativeBinOpList (f64, NotEqual) ),  // 0x62
    M3OP_F( "f64.lt",              -1, i_32,   d_cc(रिक्त),                 d_binOpList (f64, LessThan) ),             // 0x63
    M3OP_F( "f64.gt",              -1, i_32,   d_cc(रिक्त),                 d_binOpList (f64, GreaterThan) ),          // 0x64
    M3OP_F( "f64.le",              -1, i_32,   d_cc(रिक्त),                 d_binOpList (f64, LessThanOrEqual) ),      // 0x65
    M3OP_F( "f64.ge",              -1, i_32,   d_cc(रिक्त),                 d_binOpList (f64, GreaterThanOrEqual) ),   // 0x66

    M3OP( "i32.clz",               0,  i_32,   d_cc(रिक्त),                 d_unaryOpList (u32, Clz) ),                // 0x67
    M3OP( "i32.ctz",               0,  i_32,   d_cc(रिक्त),                 d_unaryOpList (u32, Ctz) ),                // 0x68
    M3OP( "i32.popcnt",            0,  i_32,   d_cc(रिक्त),                 d_unaryOpList (u32, Popcnt) ),             // 0x69

    M3OP( "i32.add",               -1, i_32,   d_cc(रिक्त),                 d_commutativeBinOpList (i32, सङ्कलय) ),       // 0x6a
    M3OP( "i32.sub",               -1, i_32,   d_cc(रिक्त),                 d_binOpList (i32, व्यवकलय) ),             // 0x6b
    M3OP( "i32.mul",               -1, i_32,   d_cc(रिक्त),                 d_commutativeBinOpList (i32, Multiply) ),  // 0x6c
    M3OP( "i32.div_s",             -1, i_32,   d_cc(रिक्त),                 d_binOpList (i32, Divide) ),               // 0x6d
    M3OP( "i32.div_u",             -1, i_32,   d_cc(रिक्त),                 d_binOpList (u32, Divide) ),               // 0x6e
    M3OP( "i32.rem_s",             -1, i_32,   d_cc(रिक्त),                 d_binOpList (i32, Remainder) ),            // 0x6f
    M3OP( "i32.rem_u",             -1, i_32,   d_cc(रिक्त),                 d_binOpList (u32, Remainder) ),            // 0x70
    M3OP( "i32.and",               -1, i_32,   d_cc(रिक्त),                 d_commutativeBinOpList (u32, And) ),       // 0x71
    M3OP( "i32.or",                -1, i_32,   d_cc(रिक्त),                 d_commutativeBinOpList (u32, Or) ),        // 0x72
    M3OP( "i32.xor",               -1, i_32,   d_cc(रिक्त),                 d_commutativeBinOpList (u32, Xor) ),       // 0x73
    M3OP( "i32.shl",               -1, i_32,   d_cc(रिक्त),                 d_binOpList (u32, सरणम्_वामः) ),            // 0x74
    M3OP( "i32.shr_s",             -1, i_32,   d_cc(रिक्त),                 d_binOpList (i32, सरणम्_दक्षिणः) ),           // 0x75
    M3OP( "i32.shr_u",             -1, i_32,   d_cc(रिक्त),                 d_binOpList (u32, सरणम्_दक्षिणः) ),           // 0x76
    M3OP( "i32.rotl",              -1, i_32,   d_cc(रिक्त),                 d_binOpList (u32, Rotl) ),                 // 0x77
    M3OP( "i32.rotr",              -1, i_32,   d_cc(रिक्त),                 d_binOpList (u32, Rotr) ),                 // 0x78

    M3OP( "i64.clz",               0,  i_64,   d_cc(रिक्त),                 d_unaryOpList (u64, Clz) ),                // 0x79
    M3OP( "i64.ctz",               0,  i_64,   d_cc(रिक्त),                 d_unaryOpList (u64, Ctz) ),                // 0x7a
    M3OP( "i64.popcnt",            0,  i_64,   d_cc(रिक्त),                 d_unaryOpList (u64, Popcnt) ),             // 0x7b

    M3OP( "i64.add",               -1, i_64,   d_cc(रिक्त),                 d_commutativeBinOpList (i64, सङ्कलय) ),       // 0x7c
    M3OP( "i64.sub",               -1, i_64,   d_cc(रिक्त),                 d_binOpList (i64, व्यवकलय) ),             // 0x7d
    M3OP( "i64.mul",               -1, i_64,   d_cc(रिक्त),                 d_commutativeBinOpList (i64, Multiply) ),  // 0x7e
    M3OP( "i64.div_s",             -1, i_64,   d_cc(रिक्त),                 d_binOpList (i64, Divide) ),               // 0x7f
    M3OP( "i64.div_u",             -1, i_64,   d_cc(रिक्त),                 d_binOpList (u64, Divide) ),               // 0x80
    M3OP( "i64.rem_s",             -1, i_64,   d_cc(रिक्त),                 d_binOpList (i64, Remainder) ),            // 0x81
    M3OP( "i64.rem_u",             -1, i_64,   d_cc(रिक्त),                 d_binOpList (u64, Remainder) ),            // 0x82
    M3OP( "i64.and",               -1, i_64,   d_cc(रिक्त),                 d_commutativeBinOpList (u64, And) ),       // 0x83
    M3OP( "i64.or",                -1, i_64,   d_cc(रिक्त),                 d_commutativeBinOpList (u64, Or) ),        // 0x84
    M3OP( "i64.xor",               -1, i_64,   d_cc(रिक्त),                 d_commutativeBinOpList (u64, Xor) ),       // 0x85
    M3OP( "i64.shl",               -1, i_64,   d_cc(रिक्त),                 d_binOpList (u64, सरणम्_वामः) ),            // 0x86
    M3OP( "i64.shr_s",             -1, i_64,   d_cc(रिक्त),                 d_binOpList (i64, सरणम्_दक्षिणः) ),           // 0x87
    M3OP( "i64.shr_u",             -1, i_64,   d_cc(रिक्त),                 d_binOpList (u64, सरणम्_दक्षिणः) ),           // 0x88
    M3OP( "i64.rotl",              -1, i_64,   d_cc(रिक्त),                 d_binOpList (u64, Rotl) ),                 // 0x89
    M3OP( "i64.rotr",              -1, i_64,   d_cc(रिक्त),                 d_binOpList (u64, Rotr) ),                 // 0x8a

    M3OP_F( "f32.abs",             0,  f_32,   d_cc(रिक्त),                 d_unaryOpList(f32, Abs) ),                 // 0x8b
    M3OP_F( "f32.neg",             0,  f_32,   d_cc(रिक्त),                 d_unaryOpList(f32, Negate) ),              // 0x8c
    M3OP_F( "f32.ceil",            0,  f_32,   d_cc(रिक्त),                 d_unaryOpList(f32, Ceil) ),                // 0x8d
    M3OP_F( "f32.floor",           0,  f_32,   d_cc(रिक्त),                 d_unaryOpList(f32, Floor) ),               // 0x8e
    M3OP_F( "f32.trunc",           0,  f_32,   d_cc(रिक्त),                 d_unaryOpList(f32, Trunc) ),               // 0x8f
    M3OP_F( "f32.nearest",         0,  f_32,   d_cc(रिक्त),                 d_unaryOpList(f32, निकटतमः) ),             // 0x90
    M3OP_F( "f32.sqrt",            0,  f_32,   d_cc(रिक्त),                 d_unaryOpList(f32, Sqrt) ),                // 0x91

    M3OP_F( "f32.add",             -1, f_32,   d_cc(रिक्त),                 d_commutativeBinOpList (f32, सङ्कलय) ),       // 0x92
    M3OP_F( "f32.sub",             -1, f_32,   d_cc(रिक्त),                 d_binOpList (f32, व्यवकलय) ),             // 0x93
    M3OP_F( "f32.mul",             -1, f_32,   d_cc(रिक्त),                 d_commutativeBinOpList (f32, Multiply) ),  // 0x94
    M3OP_F( "f32.div",             -1, f_32,   d_cc(रिक्त),                 d_binOpList (f32, Divide) ),               // 0x95
    M3OP_F( "f32.min",             -1, f_32,   d_cc(रिक्त),                 d_commutativeBinOpList (f32, Min) ),       // 0x96
    M3OP_F( "f32.max",             -1, f_32,   d_cc(रिक्त),                 d_commutativeBinOpList (f32, Max) ),       // 0x97
    M3OP_F( "f32.copysign",        -1, f_32,   d_cc(रिक्त),                 d_binOpList (f32, CopySign) ),             // 0x98

    M3OP_F( "f64.abs",             0,  f_64,   d_cc(रिक्त),                 d_unaryOpList(f64, Abs) ),                 // 0x99
    M3OP_F( "f64.neg",             0,  f_64,   d_cc(रिक्त),                 d_unaryOpList(f64, Negate) ),              // 0x9a
    M3OP_F( "f64.ceil",            0,  f_64,   d_cc(रिक्त),                 d_unaryOpList(f64, Ceil) ),                // 0x9b
    M3OP_F( "f64.floor",           0,  f_64,   d_cc(रिक्त),                 d_unaryOpList(f64, Floor) ),               // 0x9c
    M3OP_F( "f64.trunc",           0,  f_64,   d_cc(रिक्त),                 d_unaryOpList(f64, Trunc) ),               // 0x9d
    M3OP_F( "f64.nearest",         0,  f_64,   d_cc(रिक्त),                 d_unaryOpList(f64, निकटतमः) ),             // 0x9e
    M3OP_F( "f64.sqrt",            0,  f_64,   d_cc(रिक्त),                 d_unaryOpList(f64, Sqrt) ),                // 0x9f

    M3OP_F( "f64.add",             -1, f_64,   d_cc(रिक्त),                 d_commutativeBinOpList (f64, सङ्कलय) ),       // 0xa0
    M3OP_F( "f64.sub",             -1, f_64,   d_cc(रिक्त),                 d_binOpList (f64, व्यवकलय) ),             // 0xa1
    M3OP_F( "f64.mul",             -1, f_64,   d_cc(रिक्त),                 d_commutativeBinOpList (f64, Multiply) ),  // 0xa2
    M3OP_F( "f64.div",             -1, f_64,   d_cc(रिक्त),                 d_binOpList (f64, Divide) ),               // 0xa3
    M3OP_F( "f64.min",             -1, f_64,   d_cc(रिक्त),                 d_commutativeBinOpList (f64, Min) ),       // 0xa4
    M3OP_F( "f64.max",             -1, f_64,   d_cc(रिक्त),                 d_commutativeBinOpList (f64, Max) ),       // 0xa5
    M3OP_F( "f64.copysign",        -1, f_64,   d_cc(रिक्त),                 d_binOpList (f64, CopySign) ),             // 0xa6

    M3OP( "i32.wrap/i64",          0,  i_32,   d_cc(रिक्त),                 d_unaryOpList (i32, Wrap_i64) ),           // 0xa7
    M3OP_F( "i32.trunc_s/f32",     0,  i_32,   d_cc(Compile_Convert),      d_convertOpList (i32_Trunc_f32) ),         // 0xa8
    M3OP_F( "i32.trunc_u/f32",     0,  i_32,   d_cc(Compile_Convert),      d_convertOpList (u32_Trunc_f32) ),         // 0xa9
    M3OP_F( "i32.trunc_s/f64",     0,  i_32,   d_cc(Compile_Convert),      d_convertOpList (i32_Trunc_f64) ),         // 0xaa
    M3OP_F( "i32.trunc_u/f64",     0,  i_32,   d_cc(Compile_Convert),      d_convertOpList (u32_Trunc_f64) ),         // 0xab

    M3OP( "i64.extend_s/i32",      0,  i_64,   d_cc(रिक्त),                 d_unaryOpList (i64, Extend_i32) ),         // 0xac
    M3OP( "i64.extend_u/i32",      0,  i_64,   d_cc(रिक्त),                 d_unaryOpList (i64, Extend_u32) ),         // 0xad

    M3OP_F( "i64.trunc_s/f32",     0,  i_64,   d_cc(Compile_Convert),      d_convertOpList (i64_Trunc_f32) ),         // 0xae
    M3OP_F( "i64.trunc_u/f32",     0,  i_64,   d_cc(Compile_Convert),      d_convertOpList (u64_Trunc_f32) ),         // 0xaf
    M3OP_F( "i64.trunc_s/f64",     0,  i_64,   d_cc(Compile_Convert),      d_convertOpList (i64_Trunc_f64) ),         // 0xb0
    M3OP_F( "i64.trunc_u/f64",     0,  i_64,   d_cc(Compile_Convert),      d_convertOpList (u64_Trunc_f64) ),         // 0xb1

    M3OP_F( "f32.convert_s/i32",   0,  f_32,   d_cc(Compile_Convert),      d_convertOpList (f32_Convert_i32) ),       // 0xb2
    M3OP_F( "f32.convert_u/i32",   0,  f_32,   d_cc(Compile_Convert),      d_convertOpList (f32_Convert_u32) ),       // 0xb3
    M3OP_F( "f32.convert_s/i64",   0,  f_32,   d_cc(Compile_Convert),      d_convertOpList (f32_Convert_i64) ),       // 0xb4
    M3OP_F( "f32.convert_u/i64",   0,  f_32,   d_cc(Compile_Convert),      d_convertOpList (f32_Convert_u64) ),       // 0xb5

    M3OP_F( "f32.demote/f64",      0,  f_32,   d_cc(रिक्त),                 d_unaryOpList (f32, Demote_f64) ),         // 0xb6

    M3OP_F( "f64.convert_s/i32",   0,  f_64,   d_cc(Compile_Convert),      d_convertOpList (f64_Convert_i32) ),       // 0xb7
    M3OP_F( "f64.convert_u/i32",   0,  f_64,   d_cc(Compile_Convert),      d_convertOpList (f64_Convert_u32) ),       // 0xb8
    M3OP_F( "f64.convert_s/i64",   0,  f_64,   d_cc(Compile_Convert),      d_convertOpList (f64_Convert_i64) ),       // 0xb9
    M3OP_F( "f64.convert_u/i64",   0,  f_64,   d_cc(Compile_Convert),      d_convertOpList (f64_Convert_u64) ),       // 0xba

    M3OP_F( "f64.promote/f32",     0,  f_64,   d_cc(रिक्त),                 d_unaryOpList (f64, Promote_f32) ),        // 0xbb

    M3OP_F( "i32.reinterpret/f32", 0,  i_32,   d_cc(Compile_Convert),      d_convertOpList (i32_Reinterpret_f32) ),   // 0xbc
    M3OP_F( "i64.reinterpret/f64", 0,  i_64,   d_cc(Compile_Convert),      d_convertOpList (i64_Reinterpret_f64) ),   // 0xbd
    M3OP_F( "f32.reinterpret/i32", 0,  f_32,   d_cc(Compile_Convert),      d_convertOpList (f32_Reinterpret_i32) ),   // 0xbe
    M3OP_F( "f64.reinterpret/i64", 0,  f_64,   d_cc(Compile_Convert),      d_convertOpList (f64_Reinterpret_i64) ),   // 0xbf

    M3OP( "i32.extend8_s",         0,  i_32,   d_cc(रिक्त),                 d_unaryOpList (i32, Extend8_s) ),          // 0xc0
    M3OP( "i32.extend16_s",        0,  i_32,   d_cc(रिक्त),                 d_unaryOpList (i32, Extend16_s) ),         // 0xc1
    M3OP( "i64.extend8_s",         0,  i_64,   d_cc(रिक्त),                 d_unaryOpList (i64, Extend8_s) ),          // 0xc2
    M3OP( "i64.extend16_s",        0,  i_64,   d_cc(रिक्त),                 d_unaryOpList (i64, Extend16_s) ),         // 0xc3
    M3OP( "i64.extend32_s",        0,  i_64,   d_cc(रिक्त),                 d_unaryOpList (i64, Extend32_s) ),         // 0xc4

// Internal operations, for codepage logging only. They sit past c_waOp_lastCore,
// the last opcode this table covers, so GetOpInfo () can never reach them.
# ifdef DEBUG // for codepage logging. the order doesn't matter:
#परिभाषय d_m3DebugOp(OP) M3OP (#OP, 0, none, d_ops (op_##OP))

# यदि d_m3HasFloat
#परिभाषय d_m3DebugTypedOp(OP) M3OP (#OP, 0, none, d_ops (op_##OP##_i32, op_##OP##_i64, op_##OP##_f32, op_##OP##_f64))
# अन्यथा
#परिभाषय d_m3DebugTypedOp(OP) M3OP (#OP, 0, none, d_ops (op_##OP##_i32, op_##OP##_i64))
# endif

    d_m3DebugOp (Compile),          d_m3DebugOp (प्रविष्टिः_2),            d_m3DebugOp (अन्तः_2),
    d_m3DebugOp (Unsupported),      d_m3DebugOp (CallRawFunction),

    d_m3DebugOp (GetGlobal_s32),    d_m3DebugOp (GetGlobal_s64),    d_m3DebugOp (ContinueLoop),     d_m3DebugOp (ContinueLoopIf),
# यदि d_m3_Has_राशिः_Switching
    d_m3DebugOp (ContinueLoop_Suspendable), d_m3DebugOp (ContinueLoopIf_Suspendable), d_m3DebugOp (प्रविष्टिः_Check),
#  यदि !d_m3_प्रविष्टिः_Keeps_Frame
    d_m3DebugOp (प्रविष्टिः_Suspendable),
#  endif
# endif

    d_m3DebugOp (CopySlot_32),      d_m3DebugOp (PreserveCopySlot_32), d_m3DebugOp (If_s),          d_m3DebugOp (BranchIfPrologue_s),
    d_m3DebugOp (CopySlot_64),      d_m3DebugOp (PreserveCopySlot_64), d_m3DebugOp (If_r),          d_m3DebugOp (BranchIfPrologue_r),

    d_m3DebugOp (Select_i32_rss),   d_m3DebugOp (Select_i32_srs),   d_m3DebugOp (Select_i32_ssr),   d_m3DebugOp (Select_i32_sss),
    d_m3DebugOp (Select_i64_rss),   d_m3DebugOp (Select_i64_srs),   d_m3DebugOp (Select_i64_ssr),   d_m3DebugOp (Select_i64_sss),

# यदि d_m3HasFloat
    d_m3DebugOp (Select_f32_sss),   d_m3DebugOp (Select_f32_srs),   d_m3DebugOp (Select_f32_ssr),
    d_m3DebugOp (Select_f32_rss),   d_m3DebugOp (Select_f32_rrs),   d_m3DebugOp (Select_f32_rsr),

    d_m3DebugOp (Select_f64_sss),   d_m3DebugOp (Select_f64_srs),   d_m3DebugOp (Select_f64_ssr),
    d_m3DebugOp (Select_f64_rss),   d_m3DebugOp (Select_f64_rrs),   d_m3DebugOp (Select_f64_rsr),
# endif

    d_m3DebugOp (MemFill),          d_m3DebugOp (MemCopy),          d_m3DebugOp (MemInit),          d_m3DebugOp (दत्त_Drop),

# यदि d_m3HasGasMetering
    d_m3DebugOp (UseGas),
# endif

# यदि d_m3HasRefTypes
    d_m3DebugOp (सारणी_Get),         d_m3DebugOp (सारणी_समुच्चयः),         d_m3DebugOp (सारणी_आकार),
    d_m3DebugOp (सारणी_Grow),        d_m3DebugOp (सारणी_Fill),        d_m3DebugOp (सारणी_Init),
    d_m3DebugOp (ElemDrop),         d_m3DebugOp (सारणी_Copy),
# endif

    d_m3DebugTypedOp (समुच्चयः_Global),   d_m3DebugOp (समुच्चयः_Global_s32),    d_m3DebugOp (समुच्चयः_Global_s64),

    d_m3DebugTypedOp (समुच्चयः_पञ्जिकाकोशः), d_m3DebugTypedOp (समुच्चयः_Slot),     d_m3DebugTypedOp (Preserve_समुच्चयः_Slot),
# endif

# ifdef DEBUG
    M3OP( "termination", 0, c_m3Type_unknown, d_रिक्तानि_Op_List ) // for find_operation_info
# endif
};

ध्रुव M3OpInfo c_operationsFC[] =
{
    M3OP_F( "i32.trunc_s:sat/f32", 0, i_32,   d_cc(Compile_Convert),         d_convertOpList (i32_TruncSat_f32) ),  // 0x00
    M3OP_F( "i32.trunc_u:sat/f32", 0, i_32,   d_cc(Compile_Convert),         d_convertOpList (u32_TruncSat_f32) ),  // 0x01
    M3OP_F( "i32.trunc_s:sat/f64", 0, i_32,   d_cc(Compile_Convert),         d_convertOpList (i32_TruncSat_f64) ),  // 0x02
    M3OP_F( "i32.trunc_u:sat/f64", 0, i_32,   d_cc(Compile_Convert),         d_convertOpList (u32_TruncSat_f64) ),  // 0x03
    M3OP_F( "i64.trunc_s:sat/f32", 0, i_64,   d_cc(Compile_Convert),         d_convertOpList (i64_TruncSat_f32) ),  // 0x04
    M3OP_F( "i64.trunc_u:sat/f32", 0, i_64,   d_cc(Compile_Convert),         d_convertOpList (u64_TruncSat_f32) ),  // 0x05
    M3OP_F( "i64.trunc_s:sat/f64", 0, i_64,   d_cc(Compile_Convert),         d_convertOpList (i64_TruncSat_f64) ),  // 0x06
    M3OP_F( "i64.trunc_u:sat/f64", 0, i_64,   d_cc(Compile_Convert),         d_convertOpList (u64_TruncSat_f64) ),  // 0x07

    M3OP( "memory.init",           0, none,   d_cc(Compile_स्मृतिः_Init),     d_रिक्तानि_Op_List ),                       // 0x08
    M3OP( "data.drop",             0, none,   d_cc(Compile_दत्त_Drop),       d_रिक्तानि_Op_List ),                       // 0x09

    M3OP( "memory.copy",           0, none,   d_cc(Compile_स्मृतिः_Copy_Fill), d_रिक्तानि_Op_List ),                       // 0x0a
    M3OP( "memory.fill",           0, none,   d_cc(Compile_स्मृतिः_Copy_Fill), d_रिक्तानि_Op_List ),                       // 0x0b

#यदि d_m3HasRefTypes
    M3OP( "table.init",            0, none,   d_cc(Compile_सारणी_Init),      d_रिक्तानि_Op_List ),                       // 0x0c
    M3OP( "elem.drop",             0, none,   d_cc(Compile_Elem_Drop),       d_रिक्तानि_Op_List ),                       // 0x0d
    M3OP( "table.copy",            0, none,   d_cc(Compile_सारणी_Copy),      d_रिक्तानि_Op_List ),                       // 0x0e
#अन्यथा
    M3OP_RESERVED, M3OP_RESERVED, M3OP_RESERVED,                                                                    // 0x0c...0x0e
#यदि_अन्तः

#यदि d_m3HasRefTypes
    M3OP( "table.grow",            0, i_32,   d_cc(Compile_सारणी_Grow_Fill),  d_रिक्तानि_Op_List ),                       // 0x0f
    M3OP( "table.size",            1, i_32,   d_cc(Compile_सारणी_आकार),      d_रिक्तानि_Op_List ),                       // 0x10
    M3OP( "table.fill",            0, none,   d_cc(Compile_सारणी_Grow_Fill),  d_रिक्तानि_Op_List ),                       // 0x11
#अन्यथा
    M3OP_RESERVED, M3OP_RESERVED, M3OP_RESERVED,                                                                    // 0x0f...0x11
#यदि_अन्तः

#यदि d_m3HasWideArithmetic
    M3OP_RESERVED,                                                                                                  // 0x12

    M3OP( "i64.add128",            -2, i_64,  d_cc(Compile_WideArithmetic), d_logOp (i64_Add128) ),              // 0x13
    M3OP( "i64.sub128",            -2, i_64,  d_cc(Compile_WideArithmetic), d_logOp (i64_Sub128) ),              // 0x14
    M3OP( "i64.mul_wide_s",        0,  i_64,  d_cc(Compile_WideArithmetic), d_logOp (i64_MultiplyWide) ),        // 0x15
    M3OP( "i64.mul_wide_u",        0,  i_64,  d_cc(Compile_WideArithmetic), d_logOp (u64_MultiplyWide) ),        // 0x16
#यदि_अन्तः


# ifdef DEBUG
    M3OP( "termination", 0, c_m3Type_unknown, d_रिक्तानि_Op_List ) // for find_operation_info
# endif
};

#यदि d_m3HasAtomics

// The 0xFE instructions share one entry per kind rather than each having its own: what
// tells them apart, the width of the cell and the operation, is worked out from the
// sub-opcode by Compile_Atomic. GetAtomicInfoIndex () maps a sub-opcode onto this table.
गणना {
    c_opAtomic_notify,
    c_opAtomic_wait32,
    c_opAtomic_wait64,
    c_opAtomic_fence,
    c_op_Atomic_आरोपय,
    c_opAtomic_store,
    c_opAtomic_rmw,
    c_opAtomic_cmpxchg
};

ध्रुव M3OpInfo c_operationsFE[] =
{
    M3OP( "memory.atomic.notify",  -1, i_32,  d_cc(Compile_Atomic), d_logOp (AtomicNotify) ),    // c_opAtomic_notify
    M3OP( "memory.atomic.wait32",  -2, i_32,  d_cc(Compile_Atomic), d_logOp (AtomicWait32) ),    // c_opAtomic_wait32
    M3OP( "memory.atomic.wait64",  -2, i_32,  d_cc(Compile_Atomic), d_logOp (AtomicWait64) ),    // c_opAtomic_wait64
    M3OP( "atomic.fence",           0, none,  d_cc(Compile_Atomic), d_रिक्तानि_Op_List ),             // c_opAtomic_fence
    M3OP( "atomic.load",            0, any,   d_cc(Compile_Atomic), d_logOp (Atomic_आरोपय) ),      // c_opAtomic_load
    M3OP( "atomic.store",          -2, none,  d_cc(Compile_Atomic), d_logOp (AtomicStore) ),     // c_opAtomic_store
    M3OP( "atomic.rmw",            -1, any,   d_cc(Compile_Atomic), d_logOp (AtomicRmw) ),       // c_opAtomic_rmw
    M3OP( "atomic.rmw.cmpxchg",    -2, any,   d_cc(Compile_Atomic), d_logOp (AtomicCmpxchg) ),   // c_opAtomic_cmpxchg
};

ध्रुव u32 c_अङ्कः_Operations_FE = M3_गणनम्_सम्बन्धि(c_operationsFE);

// The entry a 0xFE sub-opcode uses, or -1 for one that names nothing: the spec leaves
// 0x04..0x0f and everything past cmpxchg unassigned. The accesses come in runs of seven,
// one per pairing of value type and cell width.
स्थिर अन्तःस्थ
i32 Get_Atomic_Info_सूचकांक (u32 i_sub)
{
    यदि (i_sub <= 0x03) प्रत्यागम (i32)i_sub;
    यदि (i_sub < 0x10) प्रत्यागम -1;
    यदि (i_sub <= 0x16) प्रत्यागम c_op_Atomic_आरोपय;
    यदि (i_sub <= 0x1d) प्रत्यागम c_opAtomic_store;
    यदि (i_sub <= 0x47) प्रत्यागम c_opAtomic_rmw;
    यदि (i_sub <= c_wa_Op_चरमः_Atomic) प्रत्यागम c_opAtomic_cmpxchg;
    प्रत्यागम -1;
}

#यदि_अन्तः // d_m3HasAtomics

// The opcodes above c_waOp_lastCore are sparse: the reference instructions at
// 0xd0..0xd2 and 0xd4, and the 0xfc extended-opcode prefix. Indexing c_operations out to
// 0xfc to reach them would leave ~50 empty entries there, so they get their own
// table, which GetHighOpIndex () maps the opcode onto.
गणना {
#यदि d_m3HasRefTypes
    c_op_उच्चः_ref_Null,
    c_op_उच्चः_ref_Is_Null,
    c_op_उच्चः_ref_Func,
#  यदि d_m3HasTypedRefs
    c_op_उच्चः_ref_As_Non_Null,
#  endif
#यदि_अन्तः
#यदि d_m3_Has_राशिः_Switching
    c_op_उच्चः_cont_New,
    c_op_उच्चः_cont_Bind,
    c_op_उच्चः_suspend,
    c_op_उच्चः_resume,
    c_op_उच्चः_resume_Throw,
    c_op_उच्चः_resume_Throw_Ref,
    c_op_उच्चः_switch,
#यदि_अन्तः
#यदि d_m3HasAtomics
    c_op_उच्चः_atomic,
#यदि_अन्तः
    c_op_उच्चः_extended   // always present, so the enum is never empty
};

// Positional, so the table must stay in c_opHigh_* order: array designators
// are not valid C++, which the maintenance builds compile this as.
स्थिर ध्रुव M3OpInfo c_operations_उच्चः[] =
{
#यदि d_m3HasRefTypes
    M3OP( "ref.null",        1, any,               d_cc(Compile_Ref_Null),       d_रिक्तानि_Op_List ),  // 0xd0  c_opHigh_refNull
    M3OP( "ref.is_null",     0, i_32,              d_cc(Compile_Ref_IsNull),     d_रिक्तानि_Op_List ),  // 0xd1  c_opHigh_refIsNull
    M3OP( "ref.func",        1, any,               d_cc(Compile_Ref_Func),       d_रिक्तानि_Op_List ),  // 0xd2  c_opHigh_refFunc
#  यदि d_m3HasTypedRefs
    M3OP( "ref.as_non_null", 0, any,               d_cc(Compile_Ref_AsNonNull),  d_रिक्तानि_Op_List ),  // 0xd4  c_opHigh_refAsNonNull
#  endif
#यदि_अन्तः
#यदि d_m3_Has_राशिः_Switching
    M3OP( "cont.new",          1, any,             d_cc(Compile_ContNew),        d_रिक्तानि_Op_List ),  // 0xe0  c_opHigh_contNew
    M3OP( "cont.bind",         2, any,             d_cc(Compile_ContBind),       d_रिक्तानि_Op_List ),  // 0xe1  c_opHigh_contBind
    M3OP( "suspend",           1, any,             d_cc(Compile_Suspend),        d_रिक्तानि_Op_List ),  // 0xe2  c_opHigh_suspend
    M3OP( "resume",            1, any,             d_cc(Compile_Resume),         d_रिक्तानि_Op_List ),  // 0xe3  c_opHigh_resume
    M3OP( "resume_throw",      2, any,             d_cc(Compile_Resume)   ,    d_रिक्तानि_Op_List ),  // 0xe4  c_opHigh_resumeThrow
    M3OP( "resume_throw_ref",  1, any,             d_cc(Compile_Resume)      , d_रिक्तानि_Op_List ),  // 0xe5  c_opHigh_resumeThrowRef
    M3OP( "switch",            2, any,             d_cc(Compile_Switch),         d_रिक्तानि_Op_List ),  // 0xe6  c_opHigh_switch
#यदि_अन्तः

#यदि d_m3HasAtomics
    M3OP( "0xFE",            0, c_m3Type_unknown,  d_cc(Compile_ExtendedOpcode), d_रिक्तानि_Op_List ),  // 0xfe  c_opHigh_atomic
#यदि_अन्तः
    M3OP( "0xFC",            0, c_m3Type_unknown,  d_cc(Compile_ExtendedOpcode), d_रिक्तानि_Op_List ),  // 0xfc  c_opHigh_extended
};
M3_STATIC_ASSERT (M3_गणनम्_सम्बन्धि (c_operations_उच्चः) == c_op_उच्चः_extended + 1, c_operations_उच्चः_needs_one_प्रविष्टिः_प्रत्येकम्_c_op_उच्चः);

// clang-format on


// Opcodes the spec reserves leave zeroed holes in the tables above: no compiler
// and no operations. Every implemented op has at least one of the two.
स्थिर अन्तःस्थ
bool IsImplementedOp (IM3OpInfo i_info)
{
    प्रत्यागम (i_info->compiler != c_cc_NULL or i_info->operations[0] != रिक्त);
}

ध्रुव u32 c_अङ्कः_Operations   = M3_गणनम्_सम्बन्धि(c_operations);
ध्रुव u32 c_अङ्कः_Operations_FC = M3_गणनम्_सम्बन्धि(c_operationsFC);

// Maps one of the sparse opcodes above c_waOp_lastCore onto c_operationsHigh,
// or -1 for anything else in that range.
स्थिर अन्तःस्थ
i32 Get_उच्चः_Op_सूचकांक (m3opcode_t opcode)
{
    विकल्प (opcode) {
#यदि d_m3HasRefTypes
    पक्ष c_waOp_refNull: प्रत्यागम c_op_उच्चः_ref_Null;
    पक्ष c_waOp_refIsNull: प्रत्यागम c_op_उच्चः_ref_Is_Null;
    पक्ष c_waOp_refFunc: प्रत्यागम c_op_उच्चः_ref_Func;
#  यदि d_m3HasTypedRefs
    पक्ष c_waOp_refAsNonNull: प्रत्यागम c_op_उच्चः_ref_As_Non_Null;
#  endif
#यदि_अन्तः
#यदि d_m3_Has_राशिः_Switching
    पक्ष c_waOp_contNew: प्रत्यागम c_op_उच्चः_cont_New;
    पक्ष c_waOp_contBind: प्रत्यागम c_op_उच्चः_cont_Bind;
    पक्ष c_waOp_suspend: प्रत्यागम c_op_उच्चः_suspend;
    पक्ष c_waOp_resume: प्रत्यागम c_op_उच्चः_resume;
    पक्ष c_waOp_resumeThrow: प्रत्यागम c_op_उच्चः_resume_Throw;
    पक्ष c_waOp_resumeThrowRef: प्रत्यागम c_op_उच्चः_resume_Throw_Ref;
    पक्ष c_waOp_switch: प्रत्यागम c_op_उच्चः_switch;
#यदि_अन्तः
    पक्ष c_waOp_extended: प्रत्यागम c_op_उच्चः_extended;
#यदि d_m3HasAtomics
    पक्ष c_waOp_atomic: प्रत्यागम c_op_उच्चः_atomic;
#यदि_अन्तः
    स्वभाव: प्रत्यागम -1;
    }
}

IM3OpInfo GetOpInfo (m3opcode_t opcode)
{
    IM3OpInfo info = रिक्त;

    विकल्प (opcode >> 8) {
    पक्ष 0x00:
        // c_operations is indexed by opcode only up to c_waOp_lastCore; past that
        // it holds internal operations (DEBUG builds), unreachable by opcode.
        यदि (M3_LIKELY(opcode <= c_wa_Op_चरमः_Core)) {
            info = &c_operations[opcode];
        } अन्यथा {
            i32 उच्चः = Get_उच्चः_Op_सूचकांक(opcode);
            यदि (उच्चः >= 0) {
                info = &c_operations_उच्चः[उच्चः];
            }
        }
        भङ्ग;
    पक्ष c_waOp_extended:
        opcode &= 0xFF;
        यदि (M3_LIKELY(opcode <= c_wa_Op_चरमः_Extended)) {
            info = &c_operationsFC[opcode];
        }
        भङ्ग;
#यदि d_m3HasAtomics
    पक्ष c_waOp_atomic: {
        i32 index = Get_Atomic_Info_सूचकांक(opcode & 0xFF);
        यदि (index >= 0) {
            info = &c_operationsFE[index];
        }
    } भङ्ग;
#यदि_अन्तः
    }

    प्रत्यागम (info and IsImplementedOp(info)) ? info : रिक्त;
}

#यदि d_m3HasGasMetering

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
गणना {
    c_gasLocal   = 1,          // charged once per declared local, at function entry
    c_gasNominal = 1,          // nop, block, loop, if, const
    c_gasArith   = 45,
    c_gas_सरणम्   = 67,
    c_gasBranch  = 90,
    c_gas_Mem_आकार = 100,
    c_gasAccess  = 120,        // a local, global, table or memory access
    c_gasHeavy   = 10000,      // an indirect call, or anything that works over a whole region
    c_gasDivide  = 36000
};

// The most one op_UseGas can carry. A segment that reaches it is split into
// consecutive charges, which prepays exactly the same amount.
#  define d_m3MaxSegmentGas   0xFFFFFFFF

स्थिर
u32 GetGasCost (m3opcode_t i_opcode)
{
    विकल्प (i_opcode) {
    पक्ष 0x00:                        // unreachable
    पक्ष 0x01:                        // nop
    पक्ष c_wa_Op_खण्डम्:
    पक्ष c_waOp_loop:
    पक्ष c_waOp_if:
    पक्ष c_wa_Op_try_सारणी:             // a block that also installs handlers
    पक्ष c_waOp_refNull:              // a constant, by another name
    पक्ष c_waOp_refFunc:
        प्रत्यागम c_gasNominal;

    पक्ष c_wa_Op_अन्तः:
        प्रत्यागम 0;                     // the one control instruction the table prices at nothing

    पक्ष c_waOp_else:
    पक्ष c_waOp_branch:
    पक्ष c_waOp_branchIf:
    पक्ष c_waOp_return:
    पक्ष c_waOp_call:
    पक्ष c_waOp_returnCall:
    पक्ष c_waOp_throw:
    पक्ष c_waOp_throwRef:
        प्रत्यागम c_gasBranch;

    पक्ष c_wa_Op_branch_सारणी:
    पक्ष 0x1a:                        // drop
    पक्ष 0x1b:                        // select
    पक्ष c_waOp_selectTyped:
        प्रत्यागम c_gasAccess;

    पक्ष c_wa_Op_call_परोक्षम्:
    पक्ष c_wa_Op_return_Call_परोक्षम्:
    पक्ष c_waOp_callRef:
    पक्ष c_waOp_returnCallRef:
    पक्ष c_wa_Op_स्मृतिः_Init:
    पक्ष c_wa_Op_स्मृतिः_Copy:
    पक्ष c_wa_Op_स्मृतिः_Fill:
    पक्ष c_wa_Op_सारणी_Init:
    पक्ष c_wa_Op_सारणी_Copy:
    पक्ष c_wa_Op_सारणी_Grow:
    पक्ष c_wa_Op_सारणी_Fill:
    पक्ष 0x40:                        // memory.grow
        प्रत्यागम c_gasHeavy;

    पक्ष c_wa_Op_दत्त_Drop:
    पक्ष c_waOp_elemDrop:
    पक्ष c_wa_Op_सारणी_आकार:
    पक्ष 0x3f:                        // memory.size
        प्रत्यागम c_gas_Mem_आकार;

    पक्ष c_waOp_refIsNull:
    पक्ष c_waOp_refAsNonNull:
        प्रत्यागम c_gasArith;

    स्वभाव:
        भङ्ग;
    }

    // The two integer arithmetic runs are priced by position, i32 at 0x6a and
    // i64 at 0x7c, the table giving both the same shape
    यदि ((i_opcode >= 0x6a and i_opcode <= 0x78) or (i_opcode >= 0x7c and i_opcode <= 0x8a)) {
        // clang-format off
        स्थिर ध्रुव u16 c_arithmeticCosts [] = {
            c_gasArith,  c_gasArith,  c_gasArith,                   // add, sub, mul
            c_gasDivide, c_gasDivide, c_gasDivide, c_gasDivide,     // div_s, div_u, rem_s, rem_u
            c_gasArith,  c_gasArith,  c_gasArith,                   // and, or, xor
            c_gas_सरणम्,  c_gas_सरणम्,  c_gas_सरणम्,                   // shl, shr_s, shr_u
            c_gasBranch, c_gasBranch                                // rotl, rotr
        };
        // clang-format on

        प्रत्यागम c_arithmeticCosts[i_opcode - (i_opcode <= 0x78 ? 0x6a : 0x7c)];
    }

    // the atomic accesses are memory accesses; what waits, wakes or orders is worth
    // as much as an operation over a whole region
    यदि (i_opcode >= (c_waOp_atomic << 8) and i_opcode <= ((c_waOp_atomic << 8) | c_wa_Op_चरमः_Atomic)) {
        प्रत्यागम (i_opcode <= c_waOp_atomicFence) ? c_gasHeavy : c_gasAccess;
    }
    यदि (i_opcode >= 0x20 and i_opcode <= 0x3e) {
        प्रत्यागम c_gasAccess;           // locals, globals, table accesses, loads and stores
    }
    यदि (i_opcode >= 0x41 and i_opcode <= 0x44) {
        प्रत्यागम c_gasNominal;          // i32/i64/f32/f64.const
    }
    यदि (i_opcode >= 0x8b and i_opcode <= 0xa6) {
        // the float run, priced as integer arithmetic except where it divides
        bool divides = (i_opcode == 0x91 or i_opcode == 0x95 or     // f32.sqrt, f32.div
                        i_opcode == 0x9f or i_opcode == 0xa3);      // f64.sqrt, f64.div

        प्रत्यागम divides ? c_gasDivide : c_gasArith;
    }

    // comparisons and clz/ctz/popcnt (0x45..0x69, 0x79..0x7b), the conversions
    // and sign extensions (0xa7..0xc4), the saturating truncations, the wide
    // arithmetic, and anything else that reaches here: plain arithmetic
    प्रत्यागम c_gasArith;
}

// The instructions ewasm's design segments a body at. A segment runs up to and
// including the next one of these, so every cycle a body can execute - a loop's
// back edge, a branch out of a block, falling off the end of a function - passes
// through at least one, and no path can run unmetered however it is entered.
स्थिर
bool Is_Gas_Segment_अन्तः (m3opcode_t i_opcode)
{
    विकल्प (i_opcode) {
    पक्ष c_waOp_loop:
    पक्ष c_waOp_if:
    पक्ष c_waOp_else:
    पक्ष c_wa_Op_अन्तः:
    पक्ष c_waOp_branch:
    पक्ष c_waOp_branchIf:
    पक्ष c_wa_Op_branch_सारणी:
    पक्ष c_waOp_return:
    पक्ष c_waOp_returnCall:
    पक्ष c_wa_Op_return_Call_परोक्षम्:
    पक्ष c_waOp_returnCallRef:
        प्रत्यागम true;
    स्वभाव:
        प्रत्यागम false;
    }
}

// Ends the open segment by writing what it accumulated into the immediate its
// op_UseGas is still waiting on. Emitting onto a later code page does not move
// an earlier one, so the reservation is still where it was left.
स्थिर
शून्य CloseGasSegment (IM3Compilation o)
{
    यदि (o->isGasSegmentOpen) {
        यदि (o->gasPatch) {
            u32 cost = o->gasCost;
            स्मृतिं_प्रतिलिख(o->gasPatch, &cost, परिमाण(cost));

            o->gasPatch = रिक्त;
        }
        o->gasCost          = 0;
        o->isGasSegmentOpen = false;
    }
}

// Whether this compilation emits the charges
स्थिर अन्तःस्थ
bool IsMetering (IM3Compilation o)
{
    प्रत्यागम o->runtime->gas_सीमा != 0;
}

// Charges one instruction to the segment being compiled, opening a segment at
// this point in the code stream if none is open, and settling the segment when
// the instruction is one that ends it. Called before the instruction is
// compiled, which is what both halves need: the op_UseGas lands ahead of what it
// pays for, and the cost is final before a block-structured instruction - loop,
// if, else - compiles its body from inside its own compiler. The body then opens
// a segment of its own, inside the loop or the arm, where it is charged on every
// pass rather than once on the way in.
स्थिर
M3_परिणाम MeterOpcode (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_परिणाम परिणाम = m3Err_none;

    // constant expressions are not metered: they run once at instantiation,
    // before the module is anything a gas budget was handed out for
    यदि (o->function and o->page and IsMetering(o)) {
        // only the 0xFC or 0xFE prefix has been read so far; the instruction it names is
        // the LEB128 u32 the compiler is about to take. Peek it without consuming.
        // Anything the tables do not cover leaves the prefix priced on its own,
        // which is moot - the compile fails on it a moment later.
        यदि (i_opcode == c_waOp_extended or i_opcode == c_waOp_atomic) {
            अष्टकानि_t peek = o->wasm;
            u32     sub;
            u32     चरमः = (i_opcode == c_waOp_atomic) ? c_wa_Op_चरमः_Atomic : c_wa_Op_चरमः_Extended;

            यदि (not ReadLEB_u32(&sub, &peek, o->wasm_अन्तः) and sub <= चरमः) {
                i_opcode = (m3opcode_t)((i_opcode << 8) | sub);
            }
        }

        u32 cost = GetGasCost(i_opcode);

        यदि (o->isGasSegmentOpen and o->gasCost > d_m3MaxSegmentGas - cost) {
            CloseGasSegment(o);
        }

        यदि (not o->isGasSegmentOpen) {
            o->isGasSegmentOpen = true;

_           (EmitOp(o, op_UseGas));
            o->gasPatch = (शून्य*)GetPagePC(o->page);
            EmitConstant32(o, 0);
        }

        o->gasCost += cost;

        यदि (Is_Gas_Segment_अन्तः(i_opcode)) {
            CloseGasSegment(o);
        }
    }

    _catch: प्रत्यागम परिणाम;
}

#यदि_अन्तः // d_m3HasGasMetering


M3_परिणाम Compile_खण्डम्_Statements (IM3Compilation o)
{
    M3_परिणाम परिणाम   = m3Err_none;
    bool     valid_अन्तः = false;

    यावत् (o->wasm < o->wasm_अन्तः) {
#यदि d_m3EnableOpTracing
        यदि (o->अङ्कः_Emits) {
            EmitOp(o, op_Dump_राशिः);
            EmitConstant32(o, o->अङ्कः_Opcodes);
            EmitConstant32(o, Get_Max_प्रयुक्तम्_Slot_Plus_One(o));
            EmitPointer(o, o->function);

            o->अङ्कः_Emits = 0;
        }
#यदि_अन्तः

        m3opcode_t opcode;
        o->चरमः_Opcode_Start = o->wasm;
_       (Read_opcode(&opcode, &o->wasm, o->wasm_अन्तः));
        log_opcode(o, opcode);

        // Restrict opcodes when evaluating expressions
        यदि (M3_UNLIKELY(not o->function)) {
            // clang-format off
            विकल्प (opcode) {
            पक्ष c_waOp_i32_const: पक्ष c_waOp_i64_const:
            पक्ष c_waOp_f32_const: पक्ष c_waOp_f64_const:
            पक्ष c_waOp_getGlobal: पक्ष c_wa_Op_अन्तः:
#यदि d_m3HasRefTypes
            पक्ष c_waOp_refNull:   पक्ष c_waOp_refFunc:
#यदि_अन्तः
#यदि d_m3HasExtendedConst
            पक्ष c_wa_Op_i32_सङ्कलय:   पक्ष c_waOp_i32_sub:   पक्ष c_waOp_i32_mul:
            पक्ष c_wa_Op_i64_सङ्कलय:   पक्ष c_waOp_i64_sub:   पक्ष c_waOp_i64_mul:
#यदि_अन्तः
                भङ्ग;
            स्वभाव:
                _throw(m3Err_restrictedOpcode);
            }
            // clang-format on
        }

        IM3OpInfo opinfo = GetOpInfo(opcode);

        यदि (M3_UNLIKELY(opinfo == रिक्त)) {
            _throw(ErrorCompile(m3Err_unknownOpcode, o, "opcode '%x' not available", opcode));
        }

#यदि d_m3HasGasMetering
_       (MeterOpcode(o, opcode));
#यदि_अन्तः

        यदि (opinfo->compiler != c_cc_NULL) {
_           ((*GetCompiler(opinfo))(o, opcode))
        } अन्यथा {
_           (Compile_Operator(o, opcode));
        }

        o->previousOpcode = opcode;

        यदि (opcode == c_waOp_else) {
            _throwif(m3Err_wasmMalformed, o->खण्डम्.opcode != c_waOp_if);
            valid_अन्तः = true;
            भङ्ग;
        } अन्यथा यदि (opcode == c_wa_Op_अन्तः) {
            valid_अन्तः = true;
            भङ्ग;
        }
    }
    _throwif(m3Err_wasmMalformed, not valid_अन्तः);

_catch:
    प्रत्यागम परिणाम;
}

स्थिर
M3_परिणाम प्रक्षिप_खण्डम्_Results (IM3Compilation o)
{
    M3_परिणाम परिणाम = m3Err_none;

    u16 अङ्कः_Results = Get_Func_Type_अङ्कः_Results(o->खण्डम्.type);

    प्रति (u16 i = 0; i < अङ्कः_Results; ++i) {
        m3type_t type = Get_Func_Type_परिणाम_Type(o->खण्डम्.type, i);

        यदि (i == अङ्कः_Results - 1 and IsFpType(type)) {
_           (प्रक्षिप_पञ्जिकाकोशः(o, type));
        } अन्यथा {
_           (प्रक्षिप_Allocated_Slot(o, type));
        }
    }

    _catch: प्रत्यागम परिणाम;
}


M3_परिणाम Compile_खण्डम् (IM3Compilation o, IM3FuncType i_खण्डम्_Type, m3opcode_t i_खण्डम्_Opcode)
{
                                                                                        d_m3Assert (not Is_पञ्जिकाकोशः_Allocated (o, 0));
                                                                                        d_m3Assert (not Is_पञ्जिकाकोशः_Allocated (o, 1));
    InvalidateFold(o);

    M3CompilationScope  outerScope = o->खण्डम्;
    M3CompilationScope* खण्डम्      = &o->खण्डम्;

    खण्डम्->outer   = &outerScope;
    खण्डम्->pc      = GetPagePC(o->page);
    खण्डम्->patches = रिक्त;
    खण्डम्->type    = i_खण्डम्_Type;
    खण्डम्->opcode  = i_खण्डम्_Opcode;
    खण्डम्->गभीरता++;
    खण्डम्->snapshot_खण्डम् = 0;

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
#यदि d_m3HasSnapshots
    // op_Loop's frame records the body, which is also the loop's identity;
    // op_TryTable's records its clause table, two words a clause back from it
    यदि (i_खण्डम्_Opcode == c_waOp_loop) {
_       (लेखः_खण्डम्_Start(o, i_खण्डम्_Opcode, खण्डम्->pc, 0));
    }
#  यदि d_m3HasExceptionHandling
    अन्यथा यदि (i_खण्डम्_Opcode == c_wa_Op_try_सारणी) {
_       (लेखः_खण्डम्_Start(o, i_खण्डम्_Opcode, खण्डम्->pc - 2 * o->अङ्कः_Try_Clauses, o->अङ्कः_Try_Clauses));
    }
#  endif
#यदि_अन्तः

    // validate and dealloc params ----------------------------

    u16 राशिः_सूचकांक = o->राशिः_सूचकांक;

    u16 अङ्कः_Params = Get_Func_Type_अङ्कः_Params(i_खण्डम्_Type);

    यदि (i_खण्डम्_Opcode != c_waOp_else) {
        प्रति (u16 i = 0; i < अङ्कः_Params; ++i) {
            m3type_t type = GetFuncTypeParamType(i_खण्डम्_Type, अङ्कः_Params - 1 - i);
_           (निष्कासय_Type(o, type));
        }
    } अन्यथा {
        यदि (Is_राशिः_Polymorphic(o) && o->खण्डम्.खण्डम्_राशिः_सूचकांक + अङ्कः_Params > o->राशिः_सूचकांक) {
            o->राशिः_सूचकांक = o->खण्डम्.खण्डम्_राशिः_सूचकांक;
        } अन्यथा {
            o->राशिः_सूचकांक -= अङ्कः_Params;
        }
    }

    u16 param_सूचकांक        = o->राशिः_सूचकांक;
    खण्डम्->exit_राशिः_सूचकांक = param_सूचकांक; // consume the params at block exit

    // keep copies of param slots in the stack
    o->राशिः_सूचकांक = राशिः_सूचकांक;

    // find slots for the results ----------------------------
_   (प्रक्षिप_खण्डम्_Results(o));

    राशिः_सूचकांक = o->राशिः_सूचकांक;

    // dealloc but keep record of the result slots in the stack
    u16 अङ्कः_Results = Get_Func_Type_अङ्कः_Results(i_खण्डम्_Type);
    यावत् (अङ्कः_Results--) {
        निष्कासय(o);
    }

    खण्डम्->खण्डम्_राशिः_सूचकांक = o->राशिः_सूचकांक = राशिः_सूचकांक;

    // push the params back onto the stack -------------------
    प्रति (u16 i = 0; i < अङ्कः_Params; ++i) {
        m3type_t type = GetFuncTypeParamType(i_खण्डम्_Type, i);

        u16 slot = Get_Slot_For_राशिः_सूचकांक(o, param_सूचकांक + i);
        प्रक्षिप(o, type, slot);

        यदि (slot >= o->slot_First_Dynamic_सूचकांक && slot != c_slot_अप्रयुक्तम्) {
_           (MarkSlotsAllocatedByType(o, slot, type));
        }
    }

    //--------------------------------------------------------

#यदि d_m3HasExceptionHandling
    // with the scope pushed and the params back on, a catch label of 0 names
    // this block - which is what the proposal says it should
    यदि (i_खण्डम्_Opcode == c_wa_Op_try_सारणी) {
_       (EmitCatchStubs(o));
    }
#यदि_अन्तः

_   (Compile_खण्डम्_Statements(o));

_   (Validate_खण्डम्_अन्तः(o));

    यदि (o->function)    // skip for expressions
    {
        यदि (not Is_राशिः_Polymorphic(o)) {
_           (Resolve_खण्डम्_Results(o, &o->खण्डम्, /* isBranch: */ false));
        }

#यदि d_m3HasExceptionHandling
        // Falling out of the end of a try region retires its handler. This sits
        // ahead of PatchBranches so branches into the block's exit land past it:
        // a branch out of the try popped its own handler at the branch site.
        यदि (i_खण्डम्_Opcode == c_wa_Op_try_सारणी) {
_           (EmitOp(o, op_निष्कासय_Handlers));
            EmitConstant32(o, 1);
        }
#यदि_अन्तः

_       (Unwind_खण्डम्_राशिः(o))

        यदि (not ((i_खण्डम्_Opcode == c_waOp_if and अङ्कः_Results) or o->previousOpcode == c_waOp_else)) {
            o->राशिः_सूचकांक = o->खण्डम्.exit_राशिः_सूचकांक;
_           (प्रक्षिप_खण्डम्_Results(o));
        }
    }

    PatchBranches(o);

#यदि d_m3HasSnapshots
    लेखः_खण्डम्_अन्तः(o);

    यदि (o->local_Init_गभीरता) {
        प्रति (u32 i = 0; i < o->snapshotMap->अङ्कः_Locals; ++i) {
            यदि (o->local_Init_गभीरता[i] >= (u32)o->खण्डम्.गभीरता) {
                o->local_Init_गभीरता[i] = UINT32_MAX;
            }
        }
    }
#यदि_अन्तः
    o->खण्डम् = outerScope;

} _catch: प्रत्यागम परिणाम;
}

स्थिर
M3_परिणाम CompileLocals (IM3Compilation o)
{
    M3_परिणाम परिणाम;

    u32 अङ्कः_Locals = 0;
    u32 अङ्कः_Local_खण्डानि;
_   (ReadLEB_u32(&अङ्कः_Local_खण्डानि, &o->wasm, o->wasm_अन्तः));

    प्रति (u32 l = 0; l < अङ्कः_Local_खण्डानि; ++l) {
        u32      var_गणनम्;
        m3type_t localType;

_       (ReadLEB_u32(&var_गणनम्, &o->wasm, o->wasm_अन्तः));
_       (Parse_मानम्_Type(o->module, &localType, &o->wasm, o->wasm_अन्तः));
        अङ्कः_Locals += var_गणनम्;                                                          m3log (compile, "pushing locals. count: %d; type: %s", var_गणनम्, c_waTypes [आधारः_Type_सम्बन्धि(localType)]);
        यावत् (var_गणनम्--) {
_           (प्रक्षिप_Allocated_Slot(o, localType));
        }
    }

    यदि (o->function) {
        o->function->अङ्कः_Locals = अङ्कः_Locals;
    }

    _catch: प्रत्यागम परिणाम;
}

स्थिर
M3_परिणाम ReserveConstants (IM3Compilation o)
{
    M3_परिणाम परिणाम = m3Err_none;

    // in the interest of speed, this blindly scans the Wasm code looking for any byte
    // that looks like an const opcode.
    u16 अङ्कः_Constant_Slots = 0;

    अष्टकानि_t wa = o->wasm;
    यावत् (wa < o->wasm_अन्तः) {
        u8  संहिता     = *wa++;
        u16 सङ्कलय_Slots = 0;

        यदि (संहिता == c_waOp_i32_const or संहिता == c_waOp_f32_const) {
            सङ्कलय_Slots = 1;
        } अन्यथा यदि (संहिता == c_waOp_i64_const or संहिता == c_waOp_f64_const) {
            सङ्कलय_Slots = Get_Type_अङ्कः_Slots(c_m3Type_i64);
        }

        यदि (अङ्कः_Constant_Slots + सङ्कलय_Slots >= d_m3_Max_Constant_सारणी_आकार) {
            भङ्ग;
        }

        अङ्कः_Constant_Slots += सङ्कलय_Slots;
    }

    // if constants overflow their reserved stack space, the compiler simply emits op_Const
    // operations as needed. Compiled expressions (global inits) don't pass through this
    // ReserveConstants function and thus always produce inline constants.

    Align_Slot_गन्तव्यम्_Type(&अङ्कः_Constant_Slots, c_m3Type_i64);                                           m3log (compile, "reserved constant slots: %d", अङ्कः_Constant_Slots);

    o->slot_First_Dynamic_सूचकांक = o->slot_First_Const_सूचकांक + अङ्कः_Constant_Slots;

    यदि (o->slot_First_Dynamic_सूचकांक >= d_m3MaxFunctionSlots) {
        _throw(m3_Err_function_राशिः_अतिप्रवाहः);
    }

    _catch:
    प्रत्यागम परिणाम;
}


// A constant expression compiles like a miniature function root block: no args,
// no locals and a single result, which Compile_End copies down into slot 0 for
// EvaluateExpression to read back.
M3_परिणाम CompileExpression (IM3Compilation o, IM3FuncType i_परिणाम_Type)
{
    M3_परिणाम परिणाम = m3Err_none;

    o->खण्डम्.type = i_परिणाम_Type;

    u16 अङ्कः_Ret_Slots = Get_Func_Type_अङ्कः_Results(i_परिणाम_Type) * c_io_Slot_गणनम्;

    प्रति (u16 i = 0; i < अङ्कः_Ret_Slots; ++i) {
        MarkSlotAllocated(o, i);
    }

    o->max_राशिः_Slots = o->slot_Max_Allocated_सूचकांक_Plus_One = o->slot_First_Dynamic_सूचकांक = अङ्कः_Ret_Slots;

_   (Compile_खण्डम्_Statements(o));

    _catch: प्रत्यागम परिणाम;
}


M3_परिणाम CompileFunction (IM3Function io_function)
{
    यदि (!io_function->wasm) {
        // An import. Either linking pointed it at another module's function -
        // in which case that one carries the body - or a host function was
        // bound to it, or it is simply unsatisfied.
        IM3Function impl = Function_Implementation(io_function);

        यदि (impl != io_function) {
            यदि (not impl->compiled) {
                M3_परिणाम r = CompileFunction(impl);
                यदि (r) {
                    प्रत्यागम r;
                }
            }

            // keep the placeholder callable too, for anything still holding it
            io_function->compiled = impl->compiled;
            प्रत्यागम m3Err_none;
        }

        यदि (io_function->compiled) {
            प्रत्यागम m3Err_none;
        }

        प्रत्यागम ErrorModule(m3Err_functionImportMissing, io_function->module, "'%s.%s'",
                           GetFunctionImportModuleName(io_function),
                           m3_GetFunctionName(io_function));
    }

#यदि d_m3EnableValidation
    यदि (not io_function->module->runtime->skipValidation) {
        M3_परिणाम vr = ValidateFunction(io_function);
        यदि (vr) {
            प्रत्यागम vr;
        }
    }
#यदि_अन्तः

    IM3FuncType funcType = io_function->funcType;                   m3log (compile, "compiling: [%d] %s %s; wasm-size: %d",
                                                                        io_function->index, m3_GetFunctionName (io_function), SPrintFuncTypeSignature (funcType), (u32) (io_function->wasm_अन्तः - io_function->wasm));
    IM3Runtime runtime = io_function->module->runtime;

    IM3Compilation o = &runtime->compilation;                       d_m3Assert (d_m3MaxFunctionSlots >= d_m3_Max_Function_राशिः_Height * (d_m3Use32BitSlots + 1))  // need twice as many slots in 32-bit mode
    स्मृतिं_पूरय(o, 0x0, परिमाण(M3Compilation));

    o->runtime    = runtime;
    o->module     = io_function->module;
    o->function   = io_function;
    o->wasm       = io_function->wasm;
    o->wasm_अन्तः    = io_function->wasm_अन्तः;
    o->खण्डम्.type = funcType;

_try {
    // skip over code size. the end was already calculated during parse phase
    u32 आकार;
_   (ReadLEB_u32(&आकार, &o->wasm, o->wasm_अन्तः));                     d_m3Assert (आकार == (o->wasm_अन्तः - o->wasm))

#यदि d_m3HasSnapshots
    o->bodyStart = o->wasm;
#यदि_अन्तः

#यदि d_m3HasSnapshots
    // only code that can be suspended can end up in a snapshot
    यदि (runtime->isSuspendable) {
        o->snapshotMap = m3_AllocStruct(M3SnapshotMap);
        _throwifnull(o->snapshotMap);
    }
#यदि_अन्तः

_   (Acquire_Compilation_संहिता_Page(o, &o->page));

    pc_t pc = GetPagePC(o->page);

    u16 अङ्कः_Ret_Slots = Get_Function_अङ्कः_Returns(o->function) * c_io_Slot_गणनम्;

    प्रति (u16 i = 0; i < अङ्कः_Ret_Slots; ++i) {
        MarkSlotAllocated(o, i);
    }

    o->function->अङ्कः_Ret_Slots = o->slot_First_Dynamic_सूचकांक = अङ्कः_Ret_Slots;

    u16 अङ्कः_तर्काः = Get_Function_अङ्कः_तर्काः(o->function);

    // push the arg types to the type stack
    प्रति (u16 i = 0; i < अङ्कः_तर्काः; ++i) {
        m3type_t type = GetFunctionArgType(o->function, i);
_       (प्रक्षिप_Allocated_Slot(o, type));

        // prevent allocator fill-in
        o->slot_First_Dynamic_सूचकांक += c_io_Slot_गणनम्;
    }

    o->slot_Max_Allocated_सूचकांक_Plus_One = o->function->अङ्कः_Ret_And_Arg_Slots = o->slot_First_Local_सूचकांक = o->slot_First_Dynamic_सूचकांक;

_   (CompileLocals(o));

#यदि d_m3HasSnapshots
_   (लेखः_Locals(o));
#यदि_अन्तः

#यदि d_m3HasGasMetering
    // the table charges for a function's locals as well as for its instructions.
    // op_Entry is what sets them up, so the charge rides on the body's first
    // segment, which is emitted right after it
    o->gasCost = o->function->अङ्कः_Locals * c_gasLocal;
#यदि_अन्तः

    u16 maxSlot = Get_Max_प्रयुक्तम्_Slot_Plus_One(o);

    o->function->अङ्कः_Local_अष्टकानि = (maxSlot - o->slot_First_Local_सूचकांक) * परिमाण(m3slot_t);

    o->slot_First_Const_सूचकांक = o->slot_Max_Const_सूचकांक = maxSlot;

    // ReserveConstants initializes o->firstDynamicSlotNumber
_   (ReserveConstants(o));

    // start tracking the max stack used (Push() also updates this value) so that op_Entry can precisely detect stack overflow
    o->max_राशिः_Slots = o->slot_Max_Allocated_सूचकांक_Plus_One = o->slot_First_Dynamic_सूचकांक;

    o->खण्डम्.खण्डम्_राशिः_सूचकांक = o->राशिः_First_Dynamic_सूचकांक = o->राशिः_सूचकांक;                           m3log (compile, "start stack index: %u; max stack slots: %u",
                                                                                                           (u32) o->राशिः_First_Dynamic_सूचकांक, (u32) o->max_राशिः_Slots);
#यदि d_m3_Has_राशिः_Switching
    // A suspendable body opens by checking for a pause request, once its frame
    // is set up. The pause point's offset is the first instruction's, just past
    // the local declarations - the body's end, when it has no other.
    यदि (runtime->isSuspendable) {
        o->चरमः_Opcode_Start = o->wasm;
#  यदि d_m3_प्रविष्टिः_Keeps_Frame
_       (EmitOp(o, op_प्रविष्टिः));
        EmitPointer(o, io_function);
_       (EmitOp(o, op_प्रविष्टिः_Check));
_       (लेखः_Safe_Point(o, safepoint_प्रविष्टिः, 1, 0, 0, रिक्त));
#  अन्यथा
_       (EmitOp(o, op_प्रविष्टिः_Suspendable));
        EmitPointer(o, io_function);
_       (लेखः_Safe_Point(o, safepoint_प्रविष्टिः, 0, 0, 0, रिक्त));
#  endif
    } अन्यथा
#यदि_अन्तः
    {
_       (EmitOp(o, op_प्रविष्टिः));
        EmitPointer(o, io_function);
    }

_   (Compile_खण्डम्_Statements(o));

    _throwif(m3Err_wasmMalformed, o->previousOpcode != c_wa_Op_अन्तः);

    io_function->compiled      = pc;
    io_function->max_राशिः_Slots = o->max_राशिः_Slots;

    u16 अङ्कः_Constant_Slots = o->slot_Max_Const_सूचकांक - o->slot_First_Const_सूचकांक;                           m3log (compile, "unique constant slots: %u; unused slots: %u",
                                                                                                           अङ्कः_Constant_Slots, o->slot_First_Dynamic_सूचकांक - o->slot_Max_Const_सूचकांक);
    io_function->अङ्कः_Constant_अष्टकानि = अङ्कः_Constant_Slots * परिमाण(m3slot_t);

    यदि (अङ्कः_Constant_Slots) {
        io_function->constants = m3_CopyMem((cbytes_t)o->constants, io_function->अङ्कः_Constant_अष्टकानि);
        _throwifnull(io_function->constants);
    }

#यदि d_m3HasSnapshots
    io_function->snapshotMap = o->snapshotMap;
    o->snapshotMap           = रिक्त;
#यदि_अन्तः

} _catch:

    Release_Compilation_संहिता_Page(o);

#यदि d_m3HasSnapshots
    SnapshotMap_Free(o->snapshotMap);
    o->snapshotMap = रिक्त;
    m3_Free(o->local_Init_गभीरता);
#यदि_अन्तः

    प्रत्यागम परिणाम;
}

#यदि d_m3_Has_राशिः_Switching
m3ret_t ResumeContinuation (IM3Runtime i_runtime, IM3Continuation i_cont)
{
    यदि (M3_UNLIKELY(!i_cont || i_cont->state != cont_suspended)) {
        प्रत्यागम m3Err_none;
    }

    M3_स्मृतिः_शीर्षकम्* _mem = i_cont->प्रविष्टिः_Function ? Module_स्मृतिः_शीर्षकम्(i_cont->प्रविष्टिः_Function->module) : रिक्त;

    i_runtime->सक्रिय_Continuation = i_cont;
    i_cont->state                 = cont_running;

    i32 outermost     = (i32)i_cont->अङ्कः_चौकटाः - 1;
    i_cont->अङ्कः_चौकटाः = 0;

    // with no frames standing, this lands straight on the suspension point
    m3ret_t r = Replay_चौकटाः(i_cont, outermost, _mem);

    यदि (r == m3Err_continuationSuspended) {
        i_cont->state = cont_suspended;
    } अन्यथा {
        i_cont->state = cont_returned;
    }
    i_runtime->सक्रिय_Continuation = रिक्त;

    प्रत्यागम r;
}
#यदि_अन्तः
