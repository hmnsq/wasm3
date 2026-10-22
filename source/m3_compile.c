//
//  m3_compile.c
//
//  Created by Steven Massey on 4/17/19.
//  Copyright © 2019 Steven Massey. All rights reserved.
//

// Allow using opcodes for compilation process
#घोषणा M3_COMPILE_OPCODES

#समावेश "m3_env.h"
#समावेश "m3_compile.h"
#समावेश "m3_exec.h"
#समावेश "m3_exception.h"
#समावेश "m3_info.h"
#समावेश "m3_validate.h"

// Resolves M3OpInfo.compiler, an index, to the function it names. Defined down
// with c_compilers [], which has to follow every compiler it points at.
अटल
M3Compiler GetCompiler (IM3OpInfo i_opInfo);

//----- EMIT --------------------------------------------------------------------------------------------------------------

अटल अंतरभूत
pc_t GetPC (IM3Compilation o)
{
    वापस GetPagePC(o->page);
}

#अगर d_m3HasSnapshots

#  define d_m3GrowSnapshotArray(TYPE, ARRAY, गिनती_2, CAPACITY)                    \
    अगर ((गिनती_2) == (CAPACITY)) {                                                 \
        u32   newCapacity = (CAPACITY) ? (CAPACITY) * 2 : 8;                     \
        TYPE* grown = m3_ReallocArray(TYPE, (ARRAY), newCapacity, (CAPACITY));   \
        अगर (not grown) {                                                         \
            वापस m3Err_mallocFailed;                                           \
        }                                                                        \
        (ARRAY)    = grown;                                                      \
        (CAPACITY) = newCapacity;                                                \
    }

// Where the instruction being compiled starts, counted from the start of the
// function's body: the one position every build of every revision agrees on
अटल अंतरभूत
u32 GetWasmOffset (IM3Compilation o)
{
    वापस (u32)(o->lastOpcodeStart - o->bodyStart);
}

// Whether the operand stack entry at i_index is a value the frame holds. Every
// block keeps records of its parameter and result slots on the stack, between
// the height it was entered at and where its own values begin (see
// CompileBlock). Those are landing pads rather than values: nothing need have
// written them yet, and the slots they name may be in use for something else.
// The function's own scope has none - that stretch is its arguments and locals.
अटल
bool Is_Live_ढेर_Entry (IM3Compilation o, u16 i_सूचकांक)
{
    क्रम (IM3CompilationScope scope = &o->block; scope->outer; scope = scope->outer) {
        अगर (i_सूचकांक >= scope->exit_ढेर_सूचकांक and i_सूचकांक < scope->block_ढेर_सूचकांक) {
            वापस false;
        }
    }

    वापस true;
}

अटल
M3_परिणाम Add_Safe_Point_मूल्य (IM3Compilation o, M3SafePoint* io_point, u16 i_सूचकांक)
{
    M3SnapshotMap* map = o->snapshotMap;

    d_m3GrowSnapshotArray(M3_Slot_मूल्य, map->मूल्यावली, map->अंक_मूल्यावली, o->मूल्यावली_Capacity);

    map->मूल्यावली[map->अंक_मूल्यावली].slot = o->wasm_ढेर[i_सूचकांक];
    map->मूल्यावली[map->अंक_मूल्यावली].type = o->type_ढेर[i_सूचकांक];
    map->अंक_मूल्यावली++;
    io_point->अंक_मूल्यावली++;

    वापस m3Err_none;
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
अटल
M3_परिणाम RecordSafePoint (IM3Compilation o, M3SafePointKind i_kind, u32 i_शब्दगण_Back, u16 i_अंक_Results,
                          u16 i_aux, IM3CompilationScope i_loop)
{
_try {
    M3SnapshotMap* map = o->snapshotMap;

    अगर (not map or not o->page) {
        वापस m3Err_none;
    }

    d_m3GrowSnapshotArray(M3SafePoint, map->safePoints, map->अंक_Safe_Points, o->safePointsCapacity);

    // grown before it is pointed at: adding values never moves the safepoints
    M3SafePoint* point     = &map->safePoints[map->अंक_Safe_Points];
    point->pc              = GetPC(o) - i_शब्दगण_Back;
    point->wasmOffset      = GetWasmOffset(o);
    point->first_मूल्य      = map->अंक_मूल्यावली;
    point->first_Local_मूल्य = UINT32_MAX;
    point->अंक_मूल्यावली       = 0;
    point->अंक_Results      = i_अंक_Results;
    point->aux             = i_aux;
    point->resumeType      = c_m3Type_none;
    point->kind            = (u8)i_kind;
    point->flags           = 0;

    // Non-defaultable locals can still be uninitialized at a safepoint. Keep
    // their heap types, but allow the null placeholder until local.set makes
    // them definitely initialized. Branches to a loop use its entry context.
    अगर (o->localInitDepth) {
        point->first_Local_मूल्य = map->अंक_मूल्यावली;
        क्रम (u32 i = 0; i < map->अंक_Locals; ++i) {
            d_m3GrowSnapshotArray(M3_Slot_मूल्य, map->मूल्यावली, map->अंक_मूल्यावली, o->मूल्यावली_Capacity);
            M3_Slot_मूल्य मूल्य = map->locals[i];
            अगर (o->localInitDepth[i] == UINT32_MAX or
                (i_loop and o->localInitDepth[i] >= (u32)i_loop->depth)) {
                मूल्य.type &= ~d_m3Type_refNonNull;
            }
            map->मूल्यावली[map->अंक_मूल्यावली++] = मूल्य;
        }
        point->first_मूल्य = map->अंक_मूल्यावली;
    }

    u16 शीर्ष = o->ढेर_सूचकांक;

    अगर (i_kind != safepoint_suspend) {
        शीर्ष = (शीर्ष > i_अंक_Results) ? (u16)(शीर्ष - i_अंक_Results) : 0;
    }
    अगर (i_loop) {
        शीर्ष = M3_MIN(शीर्ष, i_loop->exit_ढेर_सूचकांक);
    }

    क्रम (u16 i = o->ढेर_First_Dynamic_सूचकांक; i < शीर्ष; ++i) {
        अगर (Is_Live_ढेर_Entry(o, i)) {
_           (Add_Safe_Point_मूल्य(o, point, i));
        }
    }

    अगर (i_loop) {
        u16 अंक_Params = Get_Func_Type_अंक_Params(i_loop->type);

        क्रम (u16 i = 0; i < अंक_Params; ++i) {
_           (Add_Safe_Point_मूल्य(o, point, (u16)(i_loop->exit_ढेर_सूचकांक + i)));
        }
    }

    map->अंक_Safe_Points++;

} _catch: वापस परिणाम;
}

// A back edge: op_ContinueLoop, or op_ContinueLoopIf when i_isConditional, and
// the loop it goes back to. The conditional one suspends before it has taken
// the branch, so it has to be resumed with the condition that took it.
अटल
M3_परिणाम RecordBackEdge (IM3Compilation o, IM3CompilationScope i_loop, bool i_isConditional)
{
    M3_परिणाम परिणाम = m3Err_none;

    // the loop is named by its record in the map, which has to fit the field
    _throwif("a function has too many loops for its snapshot map",
             o->snapshotMap and o->page and (i_loop->snapshotBlock == 0 or i_loop->snapshotBlock > UINT16_MAX + 1u));

_   (RecordSafePoint(o, safepoint_op, 2, 0, (u16)(i_loop->snapshotBlock - 1), i_loop));

    अगर (i_isConditional and o->snapshotMap and o->page) {
        o->snapshotMap->safePoints[o->snapshotMap->अंक_Safe_Points - 1].flags |= d_m3SafePointTakenBranch;
    }

    _catch: वापस परिणाम;
}

// A call, waiting on its callee with i_numResults not yet written; the
// callee's frame starts i_calleeSlot slots above this one. A return_call that
// is compiled as a call is one too, since its frame stands while the callee
// runs, but it is marked: in Wasm the callee has replaced it, so a snapshot
// walks past it rather than writing it.
अटल
M3_परिणाम RecordCall (IM3Compilation o, u16 i_अंक_Results, u16 i_calleeSlot, bool i_isReturnCall)
{
    M3_परिणाम परिणाम = RecordSafePoint(o, safepoint_call, 0, i_अंक_Results, i_calleeSlot, शून्य);

    अगर (not परिणाम and i_isReturnCall and o->snapshotMap and o->page) {
        o->snapshotMap->safePoints[o->snapshotMap->अंक_Safe_Points - 1].flags |= d_m3_Safe_Point_पुच्छ_Call;
    }

    वापस परिणाम;
}

// Where the function's arguments and locals are kept. Unlike the operand
// stack, that never changes, so it is recorded once rather than at every
// safepoint.
अटल
M3_परिणाम RecordLocals (IM3Compilation o)
{
    M3SnapshotMap* map = o->snapshotMap;

    अगर (map and o->ढेर_सूचकांक) {
        map->locals = m3_AllocArray(M3_Slot_मूल्य, o->ढेर_सूचकांक);

        अगर (not map->locals) {
            वापस m3Err_mallocFailed;
        }

        क्रम (u16 i = 0; i < o->ढेर_सूचकांक; ++i) {
            map->locals[i].slot = o->wasm_ढेर[i];
            map->locals[i].type = o->type_ढेर[i];
        }

        map->अंक_Locals = o->ढेर_सूचकांक;
        क्रम (u32 i = Get_Function_अंक_Args(o->function); i < map->अंक_Locals; ++i) {
            अगर (IsRefType(map->locals[i].type) and not IsNullableRef(map->locals[i].type)) {
                o->localInitDepth = m3_AllocArray(u32, map->अंक_Locals);
                अगर (not o->localInitDepth) {
                    वापस m3Err_mallocFailed;
                }
                क्रम (u32 j = i; j < map->अंक_Locals; ++j) {
                    अगर (IsRefType(map->locals[j].type) and not IsNullableRef(map->locals[j].type)) {
                        o->localInitDepth[j] = UINT32_MAX;
                    }
                }
                अवरोध;
            }
        }
    }

    वापस m3Err_none;
}

// A loop or a try_table, whose native frame a snapshot names by the
// instruction's offset. i_pc is what that frame records. The scope being
// compiled keeps the record, so that RecordBlockEnd can close it.
अटल
M3_परिणाम RecordBlockStart (IM3Compilation o, m3opcode_t i_opcode, pc_t i_pc, u32 i_अंक_Clauses)
{
    M3SnapshotMap* map = o->snapshotMap;

    अगर (map and o->page) {
        d_m3GrowSnapshotArray(M3BlockStart, map->blocks, map->अंक_Blocks, o->blocksCapacity);

        M3BlockStart* block = &map->blocks[map->अंक_Blocks++];
        block->pc           = i_pc;
        block->wasmOffset   = GetWasmOffset(o);
        block->wasmEnd      = UINT32_MAX;
        block->अंक_Clauses   = (u16)i_अंक_Clauses;
        block->opcode       = (u8)i_opcode;

        o->block.snapshotBlock = map->अंक_Blocks;
    }

    वापस m3Err_none;
}

// Closes the scope's region, while the instruction just read is its end
अटल
व्योम RecordBlockEnd (IM3Compilation o)
{
    अगर (o->snapshotMap and o->block.snapshotBlock) {
        o->snapshotMap->blocks[o->block.snapshotBlock - 1].wasmEnd = GetWasmOffset(o);
    }
}

#  undef d_m3GrowSnapshotArray

#अन्यथा

अटल अंतरभूत
M3_परिणाम RecordSafePoint (IM3Compilation o, M3SafePointKind i_kind, u32 i_शब्दगण_Back, u16 i_अंक_Results,
                          u16 i_aux, IM3CompilationScope i_loop)
{
    (व्योम)o;
    (व्योम)i_kind;
    (व्योम)i_शब्दगण_Back;
    (व्योम)i_अंक_Results;
    (व्योम)i_aux;
    (व्योम)i_loop;
    वापस m3Err_none;
}

अटल अंतरभूत
M3_परिणाम RecordBackEdge (IM3Compilation o, IM3CompilationScope i_loop, bool i_isConditional)
{
    (व्योम)o;
    (व्योम)i_loop;
    (व्योम)i_isConditional;
    वापस m3Err_none;
}

अटल अंतरभूत
M3_परिणाम RecordCall (IM3Compilation o, u16 i_अंक_Results, u16 i_calleeSlot, bool i_isReturnCall)
{
    (व्योम)o;
    (व्योम)i_अंक_Results;
    (व्योम)i_calleeSlot;
    (व्योम)i_isReturnCall;
    वापस m3Err_none;
}

#पूर्ण_अगर // d_m3HasSnapshots

अटल M3_NOINLINE
M3_परिणाम Ensure_Code_Page_अंक_पंक्तियाँ (IM3Compilation o, u32 i_अंक_पंक्तियाँ)
{
    M3_परिणाम परिणाम = m3Err_none;

    i_अंक_पंक्तियाँ += 2; // room for Bridge

    अगर (M3_UNLIKELY(अंक_Free_पंक्तियाँ(o->page) < i_अंक_पंक्तियाँ)) {
        IM3CodePage page = AcquireCodePageWithCapacity(o->runtime, i_अंक_पंक्तियाँ);

        अगर (page) {
            m3log(emit, "bridging new code page from: %d %p (free slots: %d) to: %d", o->page->info.sequence, GetPC(o), अंक_Free_पंक्तियाँ(o->page), page->info.sequence);
            d_m3Assert(अंक_Free_पंक्तियाँ(o->page) >= 2);

            EmitWord(o->page, op_Branch);
            EmitWord(o->page, GetPagePC(page));

            ReleaseCodePage(o->runtime, o->page);

            o->page = page;
        } अन्यथा {
            परिणाम = m3Err_mallocFailedCodePage;
        }
    }

    वापस परिणाम;
}

// invalidate the pending local.set fold candidate; called wherever code is emitted or a
// branch target captures the current pc (a fold appends an immediate, moving that position)
अटल अंतरभूत
व्योम InvalidateFold (IM3Compilation o)
{
#अगर d_m3FoldSetLocal
    o->foldPatchPC = शून्य;
#अन्यथा
    (व्योम)o;
#पूर्ण_अगर
}

अटल M3_NOINLINE
M3_परिणाम EmitOp (IM3Compilation o, IM3Operation i_operation)
{
    M3_परिणाम परिणाम = m3Err_none;                                 d_m3Assert (i_operation or Is_ढेर_Polymorphic (o));

    InvalidateFold(o);

    // it's OK for page to be null; when compile-walking the bytecode without emitting
    अगर (o->page) {
#अगर d_m3EnableOpTracing
        अगर (i_operation != op_Dump_ढेर) {
            o->अंक_Emits++;
        }
#पूर्ण_अगर

        // have execution jump to a new page if slots are critically low
        परिणाम = Ensure_Code_Page_अंक_पंक्तियाँ(o, d_m3_Code_Page_Free_पंक्तियाँ_Threshold);

        अगर (M3_LIKELY(not परिणाम)) {
            अगर (d_m3LogEmit) {
                log_emit(o, i_operation);
            }
#अगर d_m3RecordBacktraces
            EmitMappingEntry(o->page, (u32)(o->lastOpcodeStart - o->module->wasmStart));
#पूर्ण_अगर // d_m3RecordBacktraces
            EmitWord(o->page, i_operation);
        }
    }

    वापस परिणाम;
}

// Push an immediate constant into the M3 codestream
अटल M3_NOINLINE
व्योम EmitConstant32 (IM3Compilation o, स्थिर u32 i_immediate)
{
    अगर (o->page) {                                                          m3log (emit, "const32: %ud", i_immediate);
        EmitWord32(o->page, i_immediate);
    }
}

#अगर d_m3HasMemory64
// Takes two lines of the code page where a pointer is 32 bits, so whatever
// reads it back has to step _pc by the same amount - see op_Const64.
अटल M3_NOINLINE
व्योम EmitConstant64 (IM3Compilation o, स्थिर u64 i_immediate)
{
    अगर (o->page) {
        EmitWord64(o->page, i_immediate);
    }
}
#पूर्ण_अगर

अटल M3_NOINLINE
व्योम EmitSlotOffset (IM3Compilation o, स्थिर i32 i_offset)
{
    अगर (o->page) {                                                          m3log (emit, "slot: [%d]", i_offset);
        EmitWord32(o->page, i_offset);
    }
}

अटल M3_NOINLINE
pc_t EmitPointer (IM3Compilation o, स्थिर व्योम* स्थिर i_pointer)
{
    pc_t ptr = GetPagePC(o->page);

    अगर (o->page) {                                                          m3log (emit, "ptr: %p", i_pointer);
        EmitWord(o->page, i_pointer);
    }

    वापस ptr;
}

अटल M3_NOINLINE
व्योम* ReservePointer (IM3Compilation o)
{
    pc_t ptr = GetPagePC(o->page);
    EmitPointer(o, शून्य);
    वापस (व्योम*)ptr;
}


//-------------------------------------------------------------------------------------------------------------------------

#घोषणा d_indent "     | %s"

// just want less letters and numbers to stare at down the way in the compiler table
#घोषणा i_32    c_m3Type_i32
#घोषणा i_64    c_m3Type_i64
#घोषणा f_32    c_m3Type_f32
#घोषणा f_64    c_m3Type_f64
#घोषणा none    c_m3Type_none
#घोषणा any     (u8)-1

#अगर d_m3HasFloat
#  define FPOP(x) x
#अन्यथा
#  define FPOP(x) शून्य
#पूर्ण_अगर

// These are indexed by M3ValueType, so every type up to c_m3Type_externref needs
// an entry. A reference is one pointer-sized word, so it moves with the integer
// operation of that width; v128 has no operations at all.
#अगर M3_SIZEOF_PTR == 8
#  define REFOP(NAME)  op_##NAME##_i64
#अन्यथा
#  define REFOP(NAME)  op_##NAME##_i32
#पूर्ण_अगर

// clang-format off

अटल स्थिर IM3Operation c_preserveSetSlot[] = { शून्य, op_PreserveSetSlot_i32,       op_PreserveSetSlot_i64,
                                                    FPOP(op_PreserveSetSlot_f32), FPOP(op_PreserveSetSlot_f64),
                                                    शून्य, REFOP(PreserveSetSlot),  REFOP(PreserveSetSlot),
                                                    REFOP(PreserveSetSlot),       REFOP(PreserveSetSlot) };
अटल स्थिर IM3Operation c_setSetOps[] =       { शून्य, op_SetSlot_i32,               op_SetSlot_i64,
                                                    FPOP(op_SetSlot_f32),         FPOP(op_SetSlot_f64),
                                                    शून्य, REFOP(SetSlot),         REFOP(SetSlot),
                                                    REFOP(SetSlot),               REFOP(SetSlot) };
अटल स्थिर IM3Operation c_setGlobalOps[] =    { शून्य, op_SetGlobal_i32,             op_SetGlobal_i64,
                                                    FPOP(op_SetGlobal_f32),       FPOP(op_SetGlobal_f64),
                                                    शून्य, REFOP(SetGlobal),       REFOP(SetGlobal),
                                                    REFOP(SetGlobal),             REFOP(SetGlobal) };
अटल स्थिर IM3Operation c_setRegisterOps[] =  { शून्य, op_SetRegister_i32,           op_SetRegister_i64,
                                                    FPOP(op_SetRegister_f32),     FPOP(op_SetRegister_f64),
                                                    शून्य, REFOP(SetRegister),     REFOP(SetRegister),
                                                    REFOP(SetRegister),           REFOP(SetRegister) };

// A table shorter than the enum reads out of bounds, so tie the two together at
// build time: adding a value type must not silently outgrow these.
#घोषणा d_m3CheckTypeTable(TABLE) \
    M3_STATIC_ASSERT (माप (TABLE) / माप (*(TABLE)) == c_m3_Type_गिनती, TABLE##_needs_one_entry_per_type)

d_m3CheckTypeTable(c_preserveSetSlot);
d_m3CheckTypeTable(c_setSetOps);
d_m3CheckTypeTable(c_setGlobalOps);
d_m3CheckTypeTable(c_setRegisterOps);

#अगर d_m3FoldSetLocal

// destination-folded variants of the hot binops, indexed by the operand-form index
// recorded at emission: [0]=_rs [1]=_sr [2]=_ss [3]=(fp _rr, never folded)
#घोषणा d_foldOpList(TYPE, NAME)            { op_##TYPE##_##NAME##_rs_f, op_##TYPE##_##NAME##_sr_f, op_##TYPE##_##NAME##_ss_f, शून्य }
#घोषणा d_foldCommutativeOpList(TYPE, NAME) { op_##TYPE##_##NAME##_rs_f, शून्य,                      op_##TYPE##_##NAME##_ss_f, शून्य }

अटल स्थिर IM3Operation c_fold_i32_Add[]         = d_foldCommutativeOpList (i32, Add);
अटल स्थिर IM3Operation c_fold_i32_Subtract[]    = d_foldOpList (i32, Subtract);
अटल स्थिर IM3Operation c_fold_i32_Multiply[]    = d_foldCommutativeOpList (i32, Multiply);
अटल स्थिर IM3Operation c_fold_u32_And[]         = d_foldCommutativeOpList (u32, And);
अटल स्थिर IM3Operation c_fold_u32_Or[]          = d_foldCommutativeOpList (u32, Or);
अटल स्थिर IM3Operation c_fold_u32_Xor[]         = d_foldCommutativeOpList (u32, Xor);
अटल स्थिर IM3Operation c_fold_u32_Shift_बायाँ[]   = d_foldOpList (u32, Shift_बायाँ);
अटल स्थिर IM3Operation c_fold_i32_Shift_दायाँ[]  = d_foldOpList (i32, Shift_दायाँ);
अटल स्थिर IM3Operation c_fold_u32_Shift_दायाँ[]  = d_foldOpList (u32, Shift_दायाँ);

अटल स्थिर IM3Operation c_fold_i64_Add[]         = d_foldCommutativeOpList (i64, Add);
अटल स्थिर IM3Operation c_fold_i64_Subtract[]    = d_foldOpList (i64, Subtract);
अटल स्थिर IM3Operation c_fold_i64_Multiply[]    = d_foldCommutativeOpList (i64, Multiply);
अटल स्थिर IM3Operation c_fold_u64_And[]         = d_foldCommutativeOpList (u64, And);
अटल स्थिर IM3Operation c_fold_u64_Or[]          = d_foldCommutativeOpList (u64, Or);
अटल स्थिर IM3Operation c_fold_u64_Xor[]         = d_foldCommutativeOpList (u64, Xor);
अटल स्थिर IM3Operation c_fold_u64_Shift_बायाँ[]   = d_foldOpList (u64, Shift_बायाँ);
अटल स्थिर IM3Operation c_fold_i64_Shift_दायाँ[]  = d_foldOpList (i64, Shift_दायाँ);
अटल स्थिर IM3Operation c_fold_u64_Shift_दायाँ[]  = d_foldOpList (u64, Shift_दायाँ);

#  अगर d_m3HasFloat
अटल स्थिर IM3Operation c_fold_f32_Add[]         = d_foldCommutativeOpList (f32, Add);
अटल स्थिर IM3Operation c_fold_f32_Subtract[]    = d_foldOpList (f32, Subtract);
अटल स्थिर IM3Operation c_fold_f32_Multiply[]    = d_foldCommutativeOpList (f32, Multiply);
अटल स्थिर IM3Operation c_fold_f32_Divide[]      = d_foldOpList (f32, Divide);
अटल स्थिर IM3Operation c_fold_f64_Add[]         = d_foldCommutativeOpList (f64, Add);
अटल स्थिर IM3Operation c_fold_f64_Subtract[]    = d_foldOpList (f64, Subtract);
अटल स्थिर IM3Operation c_fold_f64_Multiply[]    = d_foldCommutativeOpList (f64, Multiply);
अटल स्थिर IM3Operation c_fold_f64_Divide[]      = d_foldOpList (f64, Divide);
#  endif

// loads are unary: [0]= address in _r0, [1]= address in a slot
#घोषणा d_foldLoadList(DEST, SRC)       { op_##DEST##_Load_##SRC##_r_f, op_##DEST##_Load_##SRC##_s_f, शून्य, शून्य }

अटल स्थिर IM3Operation c_fold_i32_Load_i32[] = d_foldLoadList(i32, i32);
अटल स्थिर IM3Operation c_fold_i32_Load_i8[] = d_foldLoadList(i32, i8);
अटल स्थिर IM3Operation c_fold_i32_Load_u8[] = d_foldLoadList(i32, u8);
अटल स्थिर IM3Operation c_fold_i32_Load_i16[] = d_foldLoadList(i32, i16);
अटल स्थिर IM3Operation c_fold_i32_Load_u16[] = d_foldLoadList(i32, u16);

अटल स्थिर IM3Operation c_fold_i64_Load_i64[] = d_foldLoadList(i64, i64);
अटल स्थिर IM3Operation c_fold_i64_Load_i8[] = d_foldLoadList(i64, i8);
अटल स्थिर IM3Operation c_fold_i64_Load_u8[] = d_foldLoadList(i64, u8);
अटल स्थिर IM3Operation c_fold_i64_Load_i16[] = d_foldLoadList(i64, i16);
अटल स्थिर IM3Operation c_fold_i64_Load_u16[] = d_foldLoadList(i64, u16);
अटल स्थिर IM3Operation c_fold_i64_Load_i32[] = d_foldLoadList(i64, i32);
अटल स्थिर IM3Operation c_fold_i64_Load_u32[] = d_foldLoadList(i64, u32);
#  अगर d_m3HasFloat
अटल स्थिर IM3Operation c_fold_f32_Load_f32[] = d_foldLoadList(f32, f32);
अटल स्थिर IM3Operation c_fold_f64_Load_f64[] = d_foldLoadList(f64, f64);
#  endif

अटल
IM3Operation GetFoldOp (m3opcode_t i_opcode, u8 i_form)
{
    अगर (i_form >= 4) {
        वापस शून्य;
    }

    चयन (i_opcode) {
    हाल 0x6a:  वापस c_fold_i32_Add        [i_form];      // i32.add
    हाल 0x6b:  वापस c_fold_i32_Subtract   [i_form];      // i32.sub
    हाल 0x6c:  वापस c_fold_i32_Multiply   [i_form];      // i32.mul
    हाल 0x71:  वापस c_fold_u32_And        [i_form];      // i32.and
    हाल 0x72:  वापस c_fold_u32_Or         [i_form];      // i32.or
    हाल 0x73:  वापस c_fold_u32_Xor        [i_form];      // i32.xor
    हाल 0x74:  वापस c_fold_u32_Shift_बायाँ  [i_form];      // i32.shl
    हाल 0x75:  वापस c_fold_i32_Shift_दायाँ [i_form];      // i32.shr_s
    हाल 0x76:  वापस c_fold_u32_Shift_दायाँ [i_form];      // i32.shr_u

    हाल 0x7c:  वापस c_fold_i64_Add        [i_form];      // i64.add
    हाल 0x7d:  वापस c_fold_i64_Subtract   [i_form];      // i64.sub
    हाल 0x7e:  वापस c_fold_i64_Multiply   [i_form];      // i64.mul
    हाल 0x83:  वापस c_fold_u64_And        [i_form];      // i64.and
    हाल 0x84:  वापस c_fold_u64_Or         [i_form];      // i64.or
    हाल 0x85:  वापस c_fold_u64_Xor        [i_form];      // i64.xor
    हाल 0x86:  वापस c_fold_u64_Shift_बायाँ  [i_form];      // i64.shl
    हाल 0x87:  वापस c_fold_i64_Shift_दायाँ [i_form];      // i64.shr_s
    हाल 0x88:  वापस c_fold_u64_Shift_दायाँ [i_form];      // i64.shr_u

#  अगर d_m3HasFloat
    हाल 0x92:  वापस c_fold_f32_Add        [i_form];      // f32.add
    हाल 0x93:  वापस c_fold_f32_Subtract   [i_form];      // f32.sub
    हाल 0x94:  वापस c_fold_f32_Multiply   [i_form];      // f32.mul
    हाल 0x95:  वापस c_fold_f32_Divide     [i_form];      // f32.div
    हाल 0xa0:  वापस c_fold_f64_Add        [i_form];      // f64.add
    हाल 0xa1:  वापस c_fold_f64_Subtract   [i_form];      // f64.sub
    हाल 0xa2:  वापस c_fold_f64_Multiply   [i_form];      // f64.mul
    हाल 0xa3:  वापस c_fold_f64_Divide     [i_form];      // f64.div
#  endif

    हाल 0x28:  वापस c_fold_i32_Load_i32   [i_form];      // i32.load
    हाल 0x2c:  वापस c_fold_i32_Load_i8    [i_form];      // i32.load8_s
    हाल 0x2d:  वापस c_fold_i32_Load_u8    [i_form];      // i32.load8_u
    हाल 0x2e:  वापस c_fold_i32_Load_i16   [i_form];      // i32.load16_s
    हाल 0x2f:  वापस c_fold_i32_Load_u16   [i_form];      // i32.load16_u

    हाल 0x29:  वापस c_fold_i64_Load_i64   [i_form];      // i64.load
    हाल 0x30:  वापस c_fold_i64_Load_i8    [i_form];      // i64.load8_s
    हाल 0x31:  वापस c_fold_i64_Load_u8    [i_form];      // i64.load8_u
    हाल 0x32:  वापस c_fold_i64_Load_i16   [i_form];      // i64.load16_s
    हाल 0x33:  वापस c_fold_i64_Load_u16   [i_form];      // i64.load16_u
    हाल 0x34:  वापस c_fold_i64_Load_i32   [i_form];      // i64.load32_s
    हाल 0x35:  वापस c_fold_i64_Load_u32   [i_form];      // i64.load32_u

#  अगर d_m3HasFloat
    हाल 0x2a:  वापस c_fold_f32_Load_f32   [i_form];      // f32.load
    हाल 0x2b:  वापस c_fold_f64_Load_f64   [i_form];      // f64.load
#  endif

    शेष: वापस शून्य;
    }
}

#पूर्ण_अगर // d_m3FoldSetLocal

// clang-format on

#अगर d_m3FuseBranch

// fused compare+branch/if variants, indexed like the fold lists: [0]=_rs [1]=_sr [2]=_ss [3]=unused.
// the [1] entries are NULL for commutative compares: the compiler never records the _sr form for them
#  define d_fuseCmpList(TYPE, NAME, KIND)             { op_##TYPE##_##KIND##_##NAME##_rs, op_##TYPE##_##KIND##_##NAME##_sr, op_##TYPE##_##KIND##_##NAME##_ss, शून्य }
#  define d_fuseCommutativeCmpList(TYPE, NAME, KIND)  { op_##TYPE##_##KIND##_##NAME##_rs, शून्य,                             op_##TYPE##_##KIND##_##NAME##_ss, शून्य }

प्रकार काष्ठा M3FusedCmpOps {
    IM3Operation branchIf[4];
    IM3Operation ifOp[4];
} M3FusedCmpOps;

#  define d_fuseCmp(TYPE, NAME)               { d_fuseCmpList (TYPE, NAME, BranchIf),            d_fuseCmpList (TYPE, NAME, If) }
#  define d_fuseCommutativeCmp(TYPE, NAME)    { d_fuseCommutativeCmpList (TYPE, NAME, BranchIf), d_fuseCommutativeCmpList (TYPE, NAME, If) }

// clang-format off
अटल स्थिर M3FusedCmpOps c_fuse_i32_Equal              = d_fuseCommutativeCmp (i32, Equal);
अटल स्थिर M3FusedCmpOps c_fuse_i32_NotEqual           = d_fuseCommutativeCmp (i32, NotEqual);
अटल स्थिर M3FusedCmpOps c_fuse_i32_LessThan           = d_fuseCmp (i32, LessThan);
अटल स्थिर M3FusedCmpOps c_fuse_u32_LessThan           = d_fuseCmp (u32, LessThan);
अटल स्थिर M3FusedCmpOps c_fuse_i32_GreaterThan        = d_fuseCmp (i32, GreaterThan);
अटल स्थिर M3FusedCmpOps c_fuse_u32_GreaterThan        = d_fuseCmp (u32, GreaterThan);
अटल स्थिर M3FusedCmpOps c_fuse_i32_LessThanOrEqual    = d_fuseCmp (i32, LessThanOrEqual);
अटल स्थिर M3FusedCmpOps c_fuse_u32_LessThanOrEqual    = d_fuseCmp (u32, LessThanOrEqual);
अटल स्थिर M3FusedCmpOps c_fuse_i32_GreaterThanOrEqual = d_fuseCmp (i32, GreaterThanOrEqual);
अटल स्थिर M3FusedCmpOps c_fuse_u32_GreaterThanOrEqual = d_fuseCmp (u32, GreaterThanOrEqual);
// clang-format on

// i32.eqz is unary: [0]= operand in _r0, [1]= operand in a slot
अटल स्थिर M3FusedCmpOps c_fuse_i32_Eqz = {
    { op_i32_BranchIfEqz_r, op_i32_BranchIfEqz_s, शून्य, शून्य },
    { op_i32_IfEqz_r,       op_i32_IfEqz_s,       शून्य, शून्य }
};

अटल
स्थिर M3FusedCmpOps* GetFusedCmpOps (m3opcode_t i_opcode)
{
    चयन (i_opcode) {
    हाल 0x45: वापस &c_fuse_i32_Eqz;                    // i32.eqz
    हाल 0x46: वापस &c_fuse_i32_Equal;                  // i32.eq
    हाल 0x47: वापस &c_fuse_i32_NotEqual;               // i32.ne
    हाल 0x48: वापस &c_fuse_i32_LessThan;               // i32.lt_s
    हाल 0x49: वापस &c_fuse_u32_LessThan;               // i32.lt_u
    हाल 0x4a: वापस &c_fuse_i32_GreaterThan;            // i32.gt_s
    हाल 0x4b: वापस &c_fuse_u32_GreaterThan;            // i32.gt_u
    हाल 0x4c: वापस &c_fuse_i32_LessThanOrEqual;        // i32.le_s
    हाल 0x4d: वापस &c_fuse_u32_LessThanOrEqual;        // i32.le_u
    हाल 0x4e: वापस &c_fuse_i32_GreaterThanOrEqual;     // i32.ge_s
    हाल 0x4f: वापस &c_fuse_u32_GreaterThanOrEqual;     // i32.ge_u
    शेष: वापस शून्य;
    }
}

#पूर्ण_अगर // d_m3FuseBranch

अटल स्थिर IM3Operation c_intSelectOps[2][4] = {
    { op_Select_i32_rss, op_Select_i32_srs, op_Select_i32_ssr, op_Select_i32_sss },
    { op_Select_i64_rss, op_Select_i64_srs, op_Select_i64_ssr, op_Select_i64_sss }
};
#अगर d_m3HasFloat
अटल स्थिर IM3Operation c_fpSelectOps[2][2][3] = {
    { { op_Select_f32_sss, op_Select_f32_srs, op_Select_f32_ssr }, // selector in slot
      { op_Select_f32_rss, op_Select_f32_rrs, op_Select_f32_rsr } }, // selector in reg
    { { op_Select_f64_sss, op_Select_f64_srs, op_Select_f64_ssr }, // selector in slot
      { op_Select_f64_rss, op_Select_f64_rrs, op_Select_f64_rsr } }
};    // selector in reg
#पूर्ण_अगर

// all args & returns are 64-bit aligned, so use 2 slots for a d_m3Use32BitSlots=1 build

अटल
M3_परिणाम AcquireCompilationCodePage (IM3Compilation o, IM3CodePage* o_codePage)
{
    M3_परिणाम परिणाम = m3Err_none;

    IM3CodePage page = AcquireCodePage(o->runtime);

    अगर (page) {
#अगर (d_m3EnableCodePageRefCounting)
        {
            अगर (o->function) {
                IM3Function func = o->function;
                page->info.usage_गिनती++;

                u32 index = func->अंक_Code_Page_Refs++;
_               (m3ReallocArray(&func->codePageRefs, IM3CodePage, func->अंक_Code_Page_Refs, index));
                func->codePageRefs[index] = page;
            }
        }
#पूर्ण_अगर
    } अन्यथा {
        _throw(m3Err_mallocFailedCodePage);
    }

    _catch:

    *o_codePage = page;

    वापस परिणाम;
}

अटल अंतरभूत
व्योम ReleaseCompilationCodePage (IM3Compilation o)
{
    ReleaseCodePage(o->runtime, o->page);
}

अटल अंतरभूत
u16 Get_Type_अंक_Slots (m3type_t i_type)
{
    i_type = BaseTypeOf(i_type);

    // v128 is 16 bytes - 4 slots in 32-bit-slot mode, 2 in 64-bit.
    // (Slot-allocator only; no v128 ops execute.)
    अगर (i_type == c_m3Type_v128)
#अगर d_m3Use32BitSlots
        वापस 4;
#अन्यथा
        वापस 2;
#पूर्ण_अगर
#अगर d_m3Use32BitSlots
    वापस Is64BitType(i_type) ? 2 : 1;
#अन्यथा
    वापस 1;
#पूर्ण_अगर
}

अटल अंतरभूत
व्योम AlignSlotToType (u16* io_slot, m3type_t i_type)
{
    // align 64-bit words to even slots (if d_m3Use32BitSlots)
    u16 अंक_Slots = Get_Type_अंक_Slots(i_type);

    u16 mask = अंक_Slots - 1;
    *io_slot = (*io_slot + mask) & ~mask;
}

// Deliberately unchecked. Validation rejects an empty operand stack before compilation.
अटल अंतरभूत
i16 Get_ढेर_शीर्ष_सूचकांक (IM3Compilation o)
{
    वापस o->ढेर_सूचकांक - 1;
}

अटल अंतरभूत
M3_परिणाम Get_ढेर_शीर्ष_सूचकांक_Throws (IM3Compilation o, i16* o_ढेर_सूचकांक)
{
    *o_ढेर_सूचकांक = o->ढेर_सूचकांक - 1;

    अगर (M3_LIKELY(o->ढेर_सूचकांक > o->ढेर_First_Dynamic_सूचकांक or Is_ढेर_Polymorphic(o))) {
        वापस m3Err_none;
    } अन्यथा {
        वापस m3_Err_function_ढेर_Underrun;
    }
}


// Items in the static portion of the stack (args/locals) are hidden from GetStackTypeFromTop ()
// In other words, only "real" Wasm stack items can be inspected.  This is important when
// returning values, etc. and you need an accurate wasm-view of the stack.
अटल
m3type_t Get_ढेर_Type_From_शीर्ष (IM3Compilation o, u16 i_offset)
{
    m3type_t type = c_m3Type_none;

    ++i_offset;
    अगर (o->ढेर_सूचकांक >= i_offset) {
        u16 index = o->ढेर_सूचकांक - i_offset;

        अगर (index >= o->ढेर_First_Dynamic_सूचकांक) {
            type = o->type_ढेर[index];
        }
    }

    वापस type;
}

अटल अंतरभूत
m3type_t Get_ढेर_शीर्ष_Type (IM3Compilation o)
{
    वापस Get_ढेर_Type_From_शीर्ष(o, 0);
}

अटल अंतरभूत
m3type_t Get_ढेर_Type_From_Bottom (IM3Compilation o, u16 i_offset)
{
    m3type_t type = c_m3Type_none;

    अगर (i_offset < o->ढेर_सूचकांक) {
        type = o->type_ढेर[i_offset];
    }

    वापस type;
}


अटल अंतरभूत
bool IsConstantSlot (IM3Compilation o, u16 i_slot)
{
    वापस (i_slot >= o->slot_First_Const_सूचकांक and i_slot < o->slot_Max_Const_सूचकांक);
}
अटल अंतरभूत
bool IsSlotAllocated (IM3Compilation o, u16 i_slot)
{
    वापस o->m3Slots[i_slot];
}

अटल अंतरभूत
bool Is_ढेर_सूचकांक_In_Register (IM3Compilation o, i32 i_ढेर_सूचकांक)
{                                                                           d_m3Assert (i_ढेर_सूचकांक < o->ढेर_सूचकांक or Is_ढेर_Polymorphic (o));
    अगर (i_ढेर_सूचकांक >= 0 and i_ढेर_सूचकांक < o->ढेर_सूचकांक) {
        वापस (o->wasm_ढेर[i_ढेर_सूचकांक] >= d_m3Reg0SlotAlias);
    } अन्यथा {
        वापस false;
    }
}

अटल अंतरभूत
u16 Get_अंक_Block_मूल्यावली_On_ढेर (IM3Compilation o)
{
    वापस o->ढेर_सूचकांक - o->block.block_ढेर_सूचकांक;
}

अटल अंतरभूत
bool Is_ढेर_शीर्ष_In_Register (IM3Compilation o)
{
    वापस Is_ढेर_सूचकांक_In_Register(o, (i32)Get_ढेर_शीर्ष_सूचकांक(o));
}
अटल अंतरभूत
bool Is_ढेर_शीर्ष_Minus1_In_Register (IM3Compilation o)
{
    वापस Is_ढेर_सूचकांक_In_Register(o, (i32)Get_ढेर_शीर्ष_सूचकांक(o) - 1);
}
अटल अंतरभूत
bool Is_ढेर_शीर्ष_Minus2_In_Register (IM3Compilation o)
{
    वापस Is_ढेर_सूचकांक_In_Register(o, (i32)Get_ढेर_शीर्ष_सूचकांक(o) - 2);
}

अटल अंतरभूत
bool Is_ढेर_शीर्ष_In_Slot (IM3Compilation o)
{
    वापस not Is_ढेर_शीर्ष_In_Register(o);
}

अटल अंतरभूत
bool IsValidSlot (u16 i_slot)
{
    वापस (i_slot < d_m3MaxFunctionSlots);
}

अटल अंतरभूत
u16 Get_ढेर_शीर्ष_Slot_संख्या (IM3Compilation o)
{
    i16 i = Get_ढेर_शीर्ष_सूचकांक(o);

    u16 slot = c_slotUnused;

    अगर (i >= 0) {
        slot = o->wasm_ढेर[i];
    }

    वापस slot;
}


// from bottom
अटल अंतरभूत
u16 Get_Slot_For_ढेर_सूचकांक (IM3Compilation o, u16 i_ढेर_सूचकांक)
{                                                                   d_m3Assert (i_ढेर_सूचकांक < o->ढेर_सूचकांक or Is_ढेर_Polymorphic (o));
    u16 slot = c_slotUnused;

    अगर (i_ढेर_सूचकांक < o->ढेर_सूचकांक) {
        slot = o->wasm_ढेर[i_ढेर_सूचकांक];
    }

    वापस slot;
}

अटल अंतरभूत
u16 Get_Extra_Slot_For_ढेर_सूचकांक (IM3Compilation o, u16 i_ढेर_सूचकांक)
{
    u16 baseSlot = Get_Slot_For_ढेर_सूचकांक(o, i_ढेर_सूचकांक);

    अगर (baseSlot != c_slotUnused) {
        u16 extraSlot = Get_Type_अंक_Slots(Get_ढेर_Type_From_Bottom(o, i_ढेर_सूचकांक)) - 1;
        baseSlot += extraSlot;
    }

    वापस baseSlot;
}


अटल अंतरभूत
व्योम TouchSlot (IM3Compilation o, u16 i_slot)
{
    // op_Entry uses this value to track and detect stack overflow
    o->max_ढेर_Slots = M3_MAX(o->max_ढेर_Slots, i_slot + 1);
}

अटल अंतरभूत
व्योम MarkSlotAllocated (IM3Compilation o, u16 i_slot)
{                                                                   d_m3Assert (o->m3Slots [i_slot] == 0); // shouldn't be already allocated
    o->m3Slots[i_slot] = 1;

    o->slot_Max_Allocated_सूचकांक_Plus_One = M3_MAX(o->slot_Max_Allocated_सूचकांक_Plus_One, i_slot + 1);

    TouchSlot(o, i_slot);
}

अटल अंतरभूत
M3_परिणाम MarkSlotsAllocated (IM3Compilation o, u16 i_slot, u16 i_अंक_Slots)
{
    अगर (M3_UNLIKELY(i_slot + i_अंक_Slots > d_m3MaxFunctionSlots)) {
        वापस m3_Err_function_ढेर_Overflow;
    }

    जबतक (i_अंक_Slots--) {
        MarkSlotAllocated(o, i_slot++);
    }

    वापस m3Err_none;
}

अटल अंतरभूत
M3_परिणाम MarkSlotsAllocatedByType (IM3Compilation o, u16 i_slot, m3type_t i_type)
{
    u16 अंक_Slots = Get_Type_अंक_Slots(i_type);
    वापस MarkSlotsAllocated(o, i_slot, अंक_Slots);
}


अटल
M3_परिणाम AllocateSlotsWithinRange (IM3Compilation o, u16* o_slot, m3type_t i_type, u16 i_startSlot, u16 i_endSlot)
{
    M3_परिणाम परिणाम = m3_Err_function_ढेर_Overflow;

    u16 अंक_Slots     = Get_Type_अंक_Slots(i_type);
    u16 खोज_Offset = अंक_Slots - 1;

    AlignSlotToType(&i_startSlot, i_type);

    // search for 1 or 2 consecutive slots in the execution stack
    u16 i = i_startSlot;
    जबतक (i + खोज_Offset < i_endSlot) {
        अगर (i + खोज_Offset < d_m3MaxFunctionSlots and o->m3Slots[i] == 0 and o->m3Slots[i + खोज_Offset] == 0) {
            MarkSlotsAllocated(o, i, अंक_Slots);

            *o_slot = i;
            परिणाम  = m3Err_none;
            अवरोध;
        }

        // keep 2-slot allocations even-aligned
        i += अंक_Slots;
    }

    वापस परिणाम;
}

अटल अंतरभूत
M3_परिणाम AllocateSlots (IM3Compilation o, u16* o_slot, m3type_t i_type)
{
    वापस AllocateSlotsWithinRange(o, o_slot, i_type, o->slot_First_Dynamic_सूचकांक, d_m3MaxFunctionSlots);
}

अटल अंतरभूत
M3_परिणाम AllocateConstantSlots (IM3Compilation o, u16* o_slot, m3type_t i_type)
{
    u16 max_Table_सूचकांक = o->slot_First_Const_सूचकांक + d_m3_Max_Constant_Table_आकार;
    वापस AllocateSlotsWithinRange(o, o_slot, i_type, o->slot_First_Const_सूचकांक, M3_MIN(o->slot_First_Dynamic_सूचकांक, max_Table_सूचकांक));
}


// TOQUE: this usage count system could be eliminated. real world code doesn't frequently trigger it.  just copy to multiple
// unique slots.
अटल अंतरभूत
M3_परिणाम Increment_Slot_Usage_गिनती (IM3Compilation o, u16 i_slot)
{                                                                                       d_m3Assert (i_slot < d_m3MaxFunctionSlots);
    M3_परिणाम परिणाम = m3Err_none;                                                       d_m3Assert (o->m3Slots [i_slot] > 0);

    // OPTZ (memory): 'm3Slots' could still be fused with 'typeStack' if 4 bits were used to indicate: [0,1,2,many]. The many-case
    // would scan 'wasmStack' to determine the actual usage count
    अगर (M3_LIKELY(o->m3Slots[i_slot] < 0xFF)) {
        o->m3Slots[i_slot]++;
    } अन्यथा {
        परिणाम = "slot usage count overflow";
    }

    वापस परिणाम;
}

अटल अंतरभूत
व्योम DeallocateSlot (IM3Compilation o, i16 i_slot, m3type_t i_type)
{                                                                                       d_m3Assert (i_slot >= o->slot_First_Dynamic_सूचकांक);
                                                                                        d_m3Assert (i_slot < o->slot_Max_Allocated_सूचकांक_Plus_One);
    क्रम (u16 i = 0; i < Get_Type_अंक_Slots(i_type); ++i, ++i_slot) {                       d_m3Assert (o->m3Slots [i_slot]);
        --o->m3Slots[i_slot];
    }
}


अटल अंतरभूत
bool IsRegisterTypeAllocated (IM3Compilation o, u8 i_type)
{
    वापस IsRegisterAllocated(o, IsFpType(i_type));
}

अटल अंतरभूत
व्योम AllocateRegister (IM3Compilation o, u32 i_register, u16 i_ढेर_सूचकांक)
{                                                                                       d_m3Assert (not IsRegisterAllocated (o, i_register));
    o->reg_ढेर_सूचकांक_Plus_One[i_register] = i_ढेर_सूचकांक + 1;
}

अटल अंतरभूत
व्योम DeallocateRegister (IM3Compilation o, u32 i_register)
{                                                                                       d_m3Assert (IsRegisterAllocated (o, i_register));
    o->reg_ढेर_सूचकांक_Plus_One[i_register] = c_m3RegisterUnallocated;
}

अटल अंतरभूत
u16 Get_Register_ढेर_सूचकांक (IM3Compilation o, u32 i_register)
{                                                                                       d_m3Assert (IsRegisterAllocated (o, i_register));
    वापस o->reg_ढेर_सूचकांक_Plus_One[i_register] - 1;
}

u16 GetMaxUsedSlotPlusOne (IM3Compilation o)
{
    जबतक (o->slot_Max_Allocated_सूचकांक_Plus_One > o->slot_First_Dynamic_सूचकांक) {
        अगर (IsSlotAllocated(o, o->slot_Max_Allocated_सूचकांक_Plus_One - 1)) {
            अवरोध;
        }

        o->slot_Max_Allocated_सूचकांक_Plus_One--;
    }

#अगर_घोषित DEBUG
    u16 maxSlot = o->slot_Max_Allocated_सूचकांक_Plus_One;
    जबतक (maxSlot < d_m3MaxFunctionSlots) {
        d_m3Assert(o->m3Slots[maxSlot] == 0);
        maxSlot++;
    }
#पूर्ण_अगर

    वापस o->slot_Max_Allocated_सूचकांक_Plus_One;
}

अटल
M3_परिणाम PreserveRegisterIfOccupied (IM3Compilation o, m3type_t i_registerType)
{
    M3_परिणाम परिणाम = m3Err_none;

    u32 regSelect = IsFpType(i_registerType);

    अगर (IsRegisterAllocated(o, regSelect)) {
        u16 ढेर_सूचकांक = Get_Register_ढेर_सूचकांक(o, regSelect);
        DeallocateRegister(o, regSelect);

        m3type_t type = Get_ढेर_Type_From_Bottom(o, ढेर_सूचकांक);

        // and point to a exec slot
        u16 slot = c_slotUnused;
_       (AllocateSlots(o, &slot, type));
        o->wasm_ढेर[ढेर_सूचकांक] = slot;

        // Ensure type is within the valid range
        अगर (M3_LIKELY(BaseTypeOf(type) < c_m3_Type_गिनती)) {
_           (EmitOp(o, c_setSetOps[BaseTypeOf(type)]));
        } अन्यथा {
            _throw(m3Err_unknownType);
        }

        EmitSlotOffset(o, slot);
    }

    _catch: वापस परिणाम;
}


// all values must be in slots before entering loop, if, and else blocks
// otherwise they'd end up preserve-copied in the block to probably different locations (if/else)
अटल अंतरभूत
M3_परिणाम PreserveRegisters (IM3Compilation o)
{
    M3_परिणाम परिणाम;

_   (PreserveRegisterIfOccupied(o, c_m3Type_f64));
_   (PreserveRegisterIfOccupied(o, c_m3Type_i64));

    _catch: वापस परिणाम;
}

अटल
M3_परिणाम Preserve_Non_शीर्ष_Registers (IM3Compilation o)
{
    M3_परिणाम परिणाम = m3Err_none;

    i16 ढेर_शीर्ष = Get_ढेर_शीर्ष_सूचकांक(o);

    अगर (ढेर_शीर्ष >= 0) {
        अगर (IsRegisterAllocated(o, 0))     // r0
        {
            अगर (Get_Register_ढेर_सूचकांक(o, 0) != ढेर_शीर्ष) {
_               (PreserveRegisterIfOccupied(o, c_m3Type_i64));
            }
        }

        अगर (IsRegisterAllocated(o, 1))     // fp0
        {
            अगर (Get_Register_ढेर_सूचकांक(o, 1) != ढेर_शीर्ष) {
_               (PreserveRegisterIfOccupied(o, c_m3Type_f64));
            }
        }
    }

    _catch: वापस परिणाम;
}


//----------------------------------------------------------------------------------------------------------------------

अटल
M3_परिणाम धकेलो (IM3Compilation o, m3type_t i_type, u16 i_slot)
{
    M3_परिणाम परिणाम = m3Err_none;

#अगर !d_m3HasFloat
    अगर (i_type == c_m3Type_f32 || i_type == c_m3Type_f64) {
        वापस m3Err_unknownOpcode;
    }
#पूर्ण_अगर

    अगर (M3_UNLIKELY(o->ढेर_सूचकांक >= d_m3_Max_Function_ढेर_Height)) {
        वापस m3_Err_function_ढेर_Overflow;
    }

    u16 ढेर_सूचकांक = o->ढेर_सूचकांक++;                                       // printf ("push: %d\n", (i32) i);

    o->wasm_ढेर[ढेर_सूचकांक] = i_slot;
    o->type_ढेर[ढेर_सूचकांक] = i_type;

    अगर (IsRegisterSlotAlias(i_slot)) {
        u32 regSelect = IsFpRegisterSlotAlias(i_slot);
        AllocateRegister(o, regSelect, ढेर_सूचकांक);
    }

    अगर (d_m3_Log_Wasm_ढेर) {
        dump_type_ढेर(o);
    }

    वापस परिणाम;
}

अटल अंतरभूत
M3_परिणाम धकेलो_Register (IM3Compilation o, m3type_t i_type)
{
    M3_परिणाम परिणाम = m3Err_none;                                                       d_m3Assert ((u16) d_m3Reg0SlotAlias > (u16) d_m3MaxFunctionSlots);
    u16      slot   = IsFpType(i_type) ? d_m3Fp0SlotAlias : d_m3Reg0SlotAlias;          d_m3Assert (i_type or Is_ढेर_Polymorphic (o));

_   (धकेलो(o, i_type, slot));

    _catch: वापस परिणाम;
}

अटल
M3_परिणाम निकालो (IM3Compilation o)
{
    M3_परिणाम परिणाम = m3Err_none;

    अगर (M3_LIKELY(o->ढेर_सूचकांक > o->block.block_ढेर_सूचकांक)) {
        o->ढेर_सूचकांक--;                                                //  printf ("pop: %d\n", (i32) o->stackIndex);

        u16      slot = o->wasm_ढेर[o->ढेर_सूचकांक];
        m3type_t type = o->type_ढेर[o->ढेर_सूचकांक];

        अगर (IsRegisterSlotAlias(slot)) {
            u32 regSelect = IsFpRegisterSlotAlias(slot);
            DeallocateRegister(o, regSelect);
        } अन्यथा अगर (M3_UNLIKELY(slot >= o->slot_Max_Allocated_सूचकांक_Plus_One)) {
            वापस m3_Err_function_ढेर_Underrun; // Return error for invalid slot indices
        } अन्यथा अगर (slot >= o->slot_First_Dynamic_सूचकांक) {
            DeallocateSlot(o, slot, type);
        }
    } अन्यथा अगर (not Is_ढेर_Polymorphic(o)) {
        परिणाम = m3_Err_function_ढेर_Underrun;
    }

    वापस परिणाम;
}

अटल
M3_परिणाम निकालो_Type (IM3Compilation o, m3type_t i_type)
{
    M3_परिणाम परिणाम = m3Err_none;

    m3type_t शीर्ष_Type = Get_ढेर_शीर्ष_Type(o);

    अगर (M3_LIKELY(IsSubTypeOf(शीर्ष_Type, i_type) or o->block.isPolymorphic)) {
_       (निकालो(o));
    } अन्यथा {
        _throw(m3Err_typeMismatch);
    }

    _catch:
    वापस परिणाम;
}

अटल
M3_परिणाम धकेलो_Allocated_Slot_And_Emit (IM3Compilation o, m3type_t i_type, bool i_doEmit)
{
    M3_परिणाम परिणाम = m3Err_none;

    u16 slot = c_slotUnused;

_   (AllocateSlots(o, &slot, i_type));
_   (धकेलो(o, i_type, slot));

    अगर (i_doEmit) {
        EmitSlotOffset(o, slot);
    }

    //    printf ("push: %d\n", (u32) slot);

    _catch: वापस परिणाम;
}

अटल अंतरभूत
M3_परिणाम धकेलो_Allocated_Slot_And_Emit_2 (IM3Compilation o, m3type_t i_type)
{
    वापस धकेलो_Allocated_Slot_And_Emit(o, i_type, true);
}

अटल अंतरभूत
M3_परिणाम धकेलो_Allocated_Slot (IM3Compilation o, m3type_t i_type)
{
    वापस धकेलो_Allocated_Slot_And_Emit(o, i_type, false);
}

अटल
M3_परिणाम धकेलो_Const (IM3Compilation o, u64 i_word, m3type_t i_type)
{
    M3_परिणाम परिणाम = m3Err_none;

    // When compile-walking a constant expression without emitting there is no
    // constant table to place the value in, but the type still has to land on
    // the stack: extended-const arithmetic downstream consumes it.
    अगर (M3_UNLIKELY(!o->page)) {
        वापस धकेलो_Allocated_Slot(o, i_type);
    }

    bool matchFound  = false;
    bool is64BitType = Is64BitType(i_type);

    u16 अंक_Required_Slots  = Get_Type_अंक_Slots(i_type);
    u16 अंक_Used_Const_Slots = o->slot_Max_Const_सूचकांक - o->slot_First_Const_सूचकांक;

    // search for duplicate matching constant slot to reuse
    अगर (अंक_Required_Slots == 2 and अंक_Used_Const_Slots >= 2) {
        u16 firstConstSlot = o->slot_First_Const_सूचकांक;
        AlignSlotToType(&firstConstSlot, c_m3Type_i64);

        क्रम (u16 slot = firstConstSlot; slot < o->slot_Max_Const_सूचकांक - 1; slot += 2) {
            अगर (IsSlotAllocated(o, slot) and IsSlotAllocated(o, slot + 1)) {
                u64 constant;
                स_नकल(&constant, &o->constants[slot - o->slot_First_Const_सूचकांक], माप(constant));

                अगर (constant == i_word) {
                    matchFound = true;
_                   (धकेलो(o, i_type, slot));
                    अवरोध;
                }
            }
        }
    } अन्यथा अगर (अंक_Required_Slots == 1) {
        क्रम (u16 i = 0; i < अंक_Used_Const_Slots; ++i) {
            u16 slot = o->slot_First_Const_सूचकांक + i;

            अगर (IsSlotAllocated(o, slot)) {
                bool matches;
                अगर (is64BitType) {
                    u64 constant;
                    स_नकल(&constant, &o->constants[i], माप(constant));
                    matches = (constant == i_word);
                } अन्यथा {
                    u32 constant;
                    स_नकल(&constant, &o->constants[i], माप(constant));
                    matches = (constant == i_word);
                }
                अगर (matches) {
                    matchFound = true;
_                   (धकेलो(o, i_type, slot));
                    अवरोध;
                }
            }
        }
    }

    अगर (not matchFound) {
        u16 slot = c_slotUnused;
        परिणाम   = AllocateConstantSlots(o, &slot, i_type);

        अगर (परिणाम || slot == c_slotUnused) // no more constant table space; use inline constants
        {
            परिणाम = m3Err_none;

            अगर (is64BitType) {
_               (EmitOp(o, op_Const64));
                EmitWord64(o->page, i_word);
            } अन्यथा {
_               (EmitOp(o, op_Const32));
                EmitWord32(o->page, (u32)i_word);
            }

_           (धकेलो_Allocated_Slot_And_Emit_2(o, i_type));
        } अन्यथा {
            u16 const_Table_सूचकांक = slot - o->slot_First_Const_सूचकांक;

            d_m3Assert(const_Table_सूचकांक < d_m3_Max_Constant_Table_आकार);

            अगर (is64BitType) {
                स_नकल(&o->constants[const_Table_सूचकांक], &i_word, माप(i_word));
            } अन्यथा {
                u32 word32 = (u32)i_word;
                स_नकल(&o->constants[const_Table_सूचकांक], &word32, माप(word32));
            }

_           (धकेलो(o, i_type, slot));

            o->slot_Max_Const_सूचकांक = M3_MAX(slot + अंक_Required_Slots, o->slot_Max_Const_सूचकांक);
        }
    }

    _catch: वापस परिणाम;
}

अटल अंतरभूत
M3_परिणाम Emit_Slot_अंक_Of_ढेर_शीर्ष_And_निकालो (IM3Compilation o)
{
    // no emit if value is in register
    अगर (Is_ढेर_शीर्ष_In_Slot(o)) {
        EmitSlotOffset(o, Get_ढेर_शीर्ष_Slot_संख्या(o));
    }

    वापस निकालो(o);
}


// Or, maybe: EmitTrappingOp
M3_परिणाम AddTrapRecord (IM3Compilation o)
{
    M3_परिणाम परिणाम = m3Err_none;

    अगर (o->function) {
    }

    वापस परिणाम;
}

अटल
M3_परिणाम Unwind_Block_ढेर (IM3Compilation o)
{
    M3_परिणाम परिणाम = m3Err_none;

    u32 निकालो_गिनती = 0;
    जबतक (o->ढेर_सूचकांक > o->block.block_ढेर_सूचकांक) {
_       (निकालो(o));
        ++निकालो_गिनती;
    }

    अगर (निकालो_गिनती) {
        m3log(compile, "unwound stack top: %d", निकालो_गिनती);
    }

    _catch: वापस परिणाम;
}

अटल अंतरभूत
M3_परिणाम Set_ढेर_Polymorphic (IM3Compilation o)
{
    o->block.isPolymorphic = true;                              m3log (compile, "stack set polymorphic");
    वापस Unwind_Block_ढेर(o);
}

अटल
व्योम PatchBranches (IM3Compilation o)
{
    InvalidateFold(o);         // patched branches land at the current pc

    pc_t pc = GetPC(o);

    pc_t patches     = o->block.patches;
    o->block.patches = शून्य;

    जबतक (patches) {                                           m3log (compile, "patching location: %p to pc: %p", patches, pc);
        pc_t अग्र       = *(pc_t*)patches;
        *(pc_t*)patches = pc;
        patches         = अग्र;
    }
}

//-------------------------------------------------------------------------------------------------------------------------

अटल
M3_परिणाम Copy_ढेर_सूचकांक_To_Slot (IM3Compilation o, u16 i_destSlot, u16 i_ढेर_सूचकांक)  // NoPushPop
{
    M3_परिणाम परिणाम = m3Err_none;

    IM3Operation op;

    m3type_t type       = Get_ढेर_Type_From_Bottom(o, i_ढेर_सूचकांक);
    bool     inRegister = Is_ढेर_सूचकांक_In_Register(o, i_ढेर_सूचकांक);

    अगर (inRegister) {
        op = c_setSetOps[BaseTypeOf(type)];
    } अन्यथा {
        op = Is64BitType(type) ? op_CopySlot_64 : op_CopySlot_32;
    }

_   (EmitOp(o, op));
    EmitSlotOffset(o, i_destSlot);

    अगर (not inRegister) {
        u16 srcSlot = Get_Slot_For_ढेर_सूचकांक(o, i_ढेर_सूचकांक);
        EmitSlotOffset(o, srcSlot);
    }

    _catch: वापस परिणाम;
}

अटल
M3_परिणाम Copy_ढेर_शीर्ष_To_Slot (IM3Compilation o, u16 i_destSlot)  // NoPushPop
{
    M3_परिणाम परिणाम;

    i16 ढेर_शीर्ष;
_   (Get_ढेर_शीर्ष_सूचकांक_Throws(o, &ढेर_शीर्ष));
_   (Copy_ढेर_सूचकांक_To_Slot(o, i_destSlot, (u16)ढेर_शीर्ष));

    _catch: वापस परिणाम;
}


// a copy-on-write strategy is used with locals. when a get local occurs, it's not copied anywhere. the stack
// entry just has a index pointer to that local memory slot.
// then, when a previously referenced local is set, the current value needs to be preserved for those references

// TODO: consider getting rid of these specialized operations: PreserveSetSlot & PreserveCopySlot.
// They likely just take up space (which seems to reduce performance) without improving performance.
// => unconfirmed in preliminary testing.
अटल
M3_परिणाम Preserved_Copy_शीर्ष_Slot (IM3Compilation o, u16 i_destSlot, u16 i_preserveSlot)
{
    M3_परिणाम परिणाम = m3Err_none;             d_m3Assert (i_destSlot != i_preserveSlot);

    IM3Operation op;

    m3type_t type = Get_ढेर_शीर्ष_Type(o);

    अगर (Is_ढेर_शीर्ष_In_Register(o)) {
        op = c_preserveSetSlot[BaseTypeOf(type)];
    } अन्यथा {
        op = Is64BitType(type) ? op_PreserveCopySlot_64 : op_PreserveCopySlot_32;
    }

_   (EmitOp(o, op));
    EmitSlotOffset(o, i_destSlot);

    अगर (Is_ढेर_शीर्ष_In_Slot(o)) {
        EmitSlotOffset(o, Get_ढेर_शीर्ष_Slot_संख्या(o));
    }

    EmitSlotOffset(o, i_preserveSlot);

    _catch: वापस परिणाम;
}

अटल
M3_परिणाम Copy_ढेर_शीर्ष_To_Register (IM3Compilation o, bool i_update_ढेर)
{
    M3_परिणाम परिणाम = m3Err_none;

    अगर (Is_ढेर_शीर्ष_In_Slot(o)) {
        m3type_t type = Get_ढेर_शीर्ष_Type(o);

_       (PreserveRegisterIfOccupied(o, type));

        IM3Operation op = c_setRegisterOps[BaseTypeOf(type)];

_       (EmitOp(o, op));
        EmitSlotOffset(o, Get_ढेर_शीर्ष_Slot_संख्या(o));

        अगर (i_update_ढेर) {
_           (निकालो_Type(o, type));
_           (धकेलो_Register(o, type));
        }
    }

    _catch: वापस परिणाम;
}


// if local is unreferenced, o_preservedSlotNumber will be equal to localIndex on return
अटल
M3_परिणाम FindReferencedLocalWithinCurrentBlock (IM3Compilation o, u16* o_preserved_Slot_संख्या, u32 i_localSlot)
{
    M3_परिणाम परिणाम = m3Err_none;

    IM3CompilationScope scope = &o->block;

    u16 start_सूचकांक = scope->block_ढेर_सूचकांक;

    जबतक (scope->opcode == c_waOp_block) {
        scope = scope->outer;
        अगर (not scope) {
            अवरोध;
        }

        start_सूचकांक = scope->block_ढेर_सूचकांक;
    }

    *o_preserved_Slot_संख्या = (u16)i_localSlot;

    क्रम (u32 i = start_सूचकांक; i < o->ढेर_सूचकांक; ++i) {
        अगर (o->wasm_ढेर[i] == i_localSlot) {
            m3type_t type = Get_ढेर_Type_From_Bottom(o, i);                         d_m3Assert (type != c_m3Type_none)

            अगर (*o_preserved_Slot_संख्या == i_localSlot) {
_               (AllocateSlots(o, o_preserved_Slot_संख्या, type));
            } अन्यथा {
                // A 64-bit value spans two slots, and allocating or releasing it
                // counts both, so a second reference has to count both as well.
                क्रम (u16 s = 0; s < Get_Type_अंक_Slots(type); ++s) {
_                   (Increment_Slot_Usage_गिनती(o, (u16)(*o_preserved_Slot_संख्या + s)));
                }
            }

            o->wasm_ढेर[i] = *o_preserved_Slot_संख्या;
        }
    }

    _catch: वापस परिणाम;
}

अटल
M3_परिणाम GetBlockScope (IM3Compilation o, IM3CompilationScope* o_scope, u32 i_depth)
{
    M3_परिणाम परिणाम = m3Err_none;

    IM3CompilationScope scope = &o->block;

    जबतक (i_depth--) {
        scope = scope->outer;
        _throwif("invalid block depth", not scope);
    }

    *o_scope = scope;

    _catch:
    वापस परिणाम;
}

अटल
M3_परिणाम Copy_ढेर_Slots_R (IM3Compilation o, u16 i_target_Slot_ढेर_सूचकांक, u16 i_ढेर_सूचकांक, u16 i_end_ढेर_सूचकांक, u16 i_अस्थायी_Slot)
{
    M3_परिणाम परिणाम = m3Err_none;

    अगर (i_ढेर_सूचकांक < i_end_ढेर_सूचकांक) {
        u16 srcSlot = Get_Slot_For_ढेर_सूचकांक(o, i_ढेर_सूचकांक);

        m3type_t type      = Get_ढेर_Type_From_Bottom(o, i_ढेर_सूचकांक);
        u16      अंक_Slots  = Get_Type_अंक_Slots(type);
        u16      extraSlot = अंक_Slots - 1;

        u16 targetSlot = Get_Slot_For_ढेर_सूचकांक(o, i_target_Slot_ढेर_सूचकांक);

        u16 preserve_सूचकांक = i_ढेर_सूचकांक;
        u16 collisionSlot = srcSlot;

        अगर (targetSlot != srcSlot) {
            // search for collisions
            u16 check_सूचकांक = i_ढेर_सूचकांक + 1;
            जबतक (check_सूचकांक < i_end_ढेर_सूचकांक) {
                u16 otherSlot1 = Get_Slot_For_ढेर_सूचकांक(o, check_सूचकांक);
                u16 otherSlot2 = Get_Extra_Slot_For_ढेर_सूचकांक(o, check_सूचकांक);

                अगर (targetSlot == otherSlot1 or
                    targetSlot == otherSlot2 or
                    targetSlot + extraSlot == otherSlot1) {
                    _throwif(m3_Err_function_ढेर_Overflow, i_अस्थायी_Slot >= d_m3MaxFunctionSlots);

_                   (Copy_ढेर_सूचकांक_To_Slot(o, i_अस्थायी_Slot, check_सूचकांक));
                    o->wasm_ढेर[check_सूचकांक] = i_अस्थायी_Slot;
                    i_अस्थायी_Slot += Get_Type_अंक_Slots(c_m3Type_i64);
                    TouchSlot(o, i_अस्थायी_Slot - 1);

                    // restore this on the way back down
                    preserve_सूचकांक = check_सूचकांक;
                    collisionSlot = otherSlot1;

                    अवरोध;
                }

                ++check_सूचकांक;
            }

_           (Copy_ढेर_सूचकांक_To_Slot(o, targetSlot, i_ढेर_सूचकांक));                                                m3log (compile, " copying slot: %d to slot: %d", srcSlot, targetSlot);
            o->wasm_ढेर[i_ढेर_सूचकांक] = targetSlot;
        }

_       (Copy_ढेर_Slots_R(o, i_target_Slot_ढेर_सूचकांक + 1, i_ढेर_सूचकांक + 1, i_end_ढेर_सूचकांक, i_अस्थायी_Slot));

        // restore the stack state
        o->wasm_ढेर[i_ढेर_सूचकांक]  = srcSlot;
        o->wasm_ढेर[preserve_सूचकांक] = collisionSlot;
    }

    _catch:
    वापस परिणाम;
}

अटल
M3_परिणाम ResolveBlockResults (IM3Compilation o, IM3CompilationScope i_targetBlock, bool i_isBranch)
{
    M3_परिणाम परिणाम = m3Err_none;
    अगर (d_m3_Log_Wasm_ढेर) {
        dump_type_ढेर(o);
    }

    bool isLoop = (i_targetBlock->opcode == c_waOp_loop and i_isBranch);

    u16 अंक_Params  = Get_Func_Type_अंक_Params(i_targetBlock->type);
    u16 अंक_Results = Get_Func_Type_अंक_Results(i_targetBlock->type);

    u16 slotRecords = i_targetBlock->exit_ढेर_सूचकांक;

    u16 अंक_मूल्यावली;

    अगर (not isLoop) {
        अंक_मूल्यावली = अंक_Results;
        slotRecords += अंक_Params;
    } अन्यथा {
        अंक_मूल्यावली = अंक_Params;
    }

    u16 blockHeight = Get_अंक_Block_मूल्यावली_On_ढेर(o);

    _throwif(m3_Err_type_गिनती_Mismatch, i_isBranch ? (blockHeight < अंक_मूल्यावली) : (blockHeight != अंक_मूल्यावली));

    अगर (अंक_मूल्यावली) {
        u16 end_सूचकांक     = Get_ढेर_शीर्ष_सूचकांक(o) + 1;
        u16 अंक_Rem_मूल्यावली = अंक_मूल्यावली;

        // The last result is taken from _fp0. See PushBlockResults.
        अगर (not isLoop and IsFpType(Get_ढेर_शीर्ष_Type(o))) {
_           (Copy_ढेर_शीर्ष_To_Register(o, false));
            --end_सूचकांक;
            --अंक_Rem_मूल्यावली;
        }

        // tempslot affects maxStackSlots, so can grow unnecess each time.
        // in practice, this is hardly ever hit.
        u16 अस्थायी_Slot = o->max_ढेर_Slots;// GetMaxUsedSlotPlusOne (o); doesn't work cause can collide with slotRecords
        AlignSlotToType(&अस्थायी_Slot, c_m3Type_i64);

_       (Copy_ढेर_Slots_R(o, slotRecords, end_सूचकांक - अंक_Rem_मूल्यावली, end_सूचकांक, अस्थायी_Slot));

        अगर (d_m3_Log_Wasm_ढेर) {
            dump_type_ढेर(o);
        }
    }

    _catch: वापस परिणाम;
}


अटल
M3_परिणाम Return_मूल्यावली (IM3Compilation o, IM3CompilationScope i_functionBlock, bool i_isBranch)
{
    M3_परिणाम परिणाम = m3Err_none;
    अगर (d_m3_Log_Wasm_ढेर) {
        dump_type_ढेर(o);
    }

    u16 अंक_Returns  = Get_Func_Type_अंक_Results(i_functionBlock->type);     // could just o->function too...
    u16 blockHeight = Get_अंक_Block_मूल्यावली_On_ढेर(o);

    अगर (not Is_ढेर_Polymorphic(o)) {
        _throwif(m3_Err_type_गिनती_Mismatch, i_isBranch ? (blockHeight < अंक_Returns) : (blockHeight != अंक_Returns));
    }

    अगर (अंक_Returns) {
        // return slots like args are 64-bit aligned
        u16 returnSlot = अंक_Returns * c_io_Slot_गिनती;
        u16 ढेर_शीर्ष   = Get_ढेर_शीर्ष_सूचकांक(o);

        क्रम (u16 i = 0; i < अंक_Returns; ++i) {
            m3type_t returnType = Get_Func_Type_परिणाम_Type(i_functionBlock->type, अंक_Returns - 1 - i);

            m3type_t ढेर_Type = Get_ढेर_Type_From_शीर्ष(o, i);  // using FromTop so that only dynamic items are checked

            अगर (Is_ढेर_Polymorphic(o) and ढेर_Type == c_m3Type_none) {
                ढेर_Type = returnType;
            }

            _throwif(m3Err_typeMismatch, not IsSubTypeOf(ढेर_Type, returnType));

            अगर (not Is_ढेर_Polymorphic(o)) {
                returnSlot -= c_io_Slot_गिनती;
_               (Copy_ढेर_सूचकांक_To_Slot(o, returnSlot, ढेर_शीर्ष--));
            }
        }

        अगर (not i_isBranch) {
            जबतक (अंक_Returns--) {
_               (निकालो(o));
            }
        }
    }

    _catch: वापस परिणाम;
}


//-------------------------------------------------------------------------------------------------------------------------

अटल
M3_परिणाम Compile_Const_i32 (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_परिणाम परिणाम;

    i32 मूल्य;
_   (ReadLEB_i32(&मूल्य, &o->wasm, o->wasmEnd));
_   (धकेलो_Const(o, मूल्य, c_m3Type_i32));                        m3log (compile, d_indent " (const i32 = %" PRIi32 ")", get_indention_string (o), मूल्य);
    _catch: वापस परिणाम;
}

अटल
M3_परिणाम Compile_Const_i64 (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_परिणाम परिणाम;

    i64 मूल्य;
_   (ReadLEB_i64(&मूल्य, &o->wasm, o->wasmEnd));
_   (धकेलो_Const(o, मूल्य, c_m3Type_i64));                        m3log (compile, d_indent " (const i64 = %" PRIi64 ")", get_indention_string (o), मूल्य);
    _catch: वापस परिणाम;
}


#अगर d_m3ImplementFloat
अटल
M3_परिणाम Compile_Const_f32 (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_परिणाम परिणाम;

    जोड़ {
        u32 u;
        f32 f;
    } मूल्य = { 0 };

_   (Read_f32(&मूल्य.f, &o->wasm, o->wasmEnd));                 m3log (compile, d_indent " (const f32 = %" PRIf32 ")", get_indention_string (o), मूल्य.f);
_   (धकेलो_Const(o, मूल्य.u, c_m3Type_f32));

    _catch: वापस परिणाम;
}

अटल
M3_परिणाम Compile_Const_f64 (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_परिणाम परिणाम;

    जोड़ {
        u64 u;
        f64 f;
    } मूल्य = { 0 };

_   (Read_f64(&मूल्य.f, &o->wasm, o->wasmEnd));                 m3log (compile, d_indent " (const f64 = %" PRIf64 ")", get_indention_string (o), मूल्य.f);
_   (धकेलो_Const(o, मूल्य.u, c_m3Type_f64));

    _catch: वापस परिणाम;
}
#पूर्ण_अगर

अटल
M3_परिणाम Compile_ExtendedOpcode (IM3Compilation o, m3opcode_t i_opcode)
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

    } _catch: वापस परिणाम;
}

अटल
M3_परिणाम Compile_Return (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_परिणाम परिणाम = m3Err_none;

    अगर (not Is_ढेर_Polymorphic(o)) {
        IM3CompilationScope functionScope;
_       (GetBlockScope(o, &functionScope, o->block.depth));

_       (Return_मूल्यावली(o, functionScope, true));

_       (EmitOp(o, op_Return));

_       (Set_ढेर_Polymorphic(o));
    }

    _catch: वापस परिणाम;
}

अटल
M3_परिणाम ValidateBlockEnd (IM3Compilation o)
{
    M3_परिणाम परिणाम = m3Err_none;

    u16 अंक_Results  = Get_Func_Type_अंक_Results(o->block.type);
    u16 blockHeight = Get_अंक_Block_मूल्यावली_On_ढेर(o);

    अगर (not Is_ढेर_Polymorphic(o)) {
        // Spec: at block end, stack height must match the number of results
        _throwif(m3_Err_type_गिनती_Mismatch, blockHeight != अंक_Results);

        // Spec: result types must match expected types
        क्रम (u16 i = 0; i < अंक_Results; ++i) {
            m3type_t expectedType = Get_Func_Type_परिणाम_Type(o->block.type, अंक_Results - 1 - i);
            m3type_t actualType   = Get_ढेर_Type_From_शीर्ष(o, i);
            _throwif(m3Err_typeMismatch, not IsSubTypeOf(actualType, expectedType));
        }
    }

    _catch: वापस परिणाम;
}

अटल
M3_परिणाम Compile_End (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_परिणाम परिणाम = m3Err_none;                   //dump_type_stack (o);

    // function end:
    अगर (o->block.depth == 0) {
        ValidateBlockEnd(o);

        //      if (not IsStackPolymorphic (o))
        {
            // A constant expression has no function frame, but its value still
            // has to be copied down to slot 0, where EvaluateExpression reads it
            // back. The non-emitting init-expr walk is skipped: it has no block
            // type to return against.
            अगर (o->function or o->page) {
_               (Return_मूल्यावली(o, &o->block, false));
            }

_           (EmitOp(o, op_Return));
        }
    }

    _catch: वापस परिणाम;
}


अटल
M3_परिणाम Compile_SetLocal (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_परिणाम परिणाम;

    u32 local_सूचकांक;
_   (ReadLEB_u32(&local_सूचकांक, &o->wasm, o->wasmEnd));                           m3log (compile, d_indent " (index = %u)", get_indention_string (o), local_सूचकांक);

    अगर (local_सूचकांक < Get_Function_अंक_Args_And_Locals(o->function)) {
        // Spec: value type must match local type
        अगर (not Is_ढेर_Polymorphic(o)) {
            m3type_t localType    = Get_ढेर_Type_From_Bottom(o, local_सूचकांक);
            m3type_t ढेर_शीर्ष_Type = Get_ढेर_शीर्ष_Type(o);
            _throwif(m3Err_typeMismatch, ढेर_शीर्ष_Type != c_m3Type_none and localType != c_m3Type_none and
                                           not IsSubTypeOf(ढेर_शीर्ष_Type, localType));
#अगर d_m3HasSnapshots
            अगर (o->localInitDepth and o->localInitDepth[local_सूचकांक] == UINT32_MAX) {
                o->localInitDepth[local_सूचकांक] = (u32)o->block.depth;
            }
#पूर्ण_अगर
        }

        u16 localSlot = Get_Slot_For_ढेर_सूचकांक(o, local_सूचकांक);

        u16 preserveSlot;
_       (FindReferencedLocalWithinCurrentBlock(o, &preserveSlot, localSlot));  // preserve will be different than local, if referenced

        bool folded = false;

#अगर d_m3FoldSetLocal
        // destination folding: when the value was produced by the immediately preceding op (nothing
        // emitted since, result on top in _r0, no copy-on-write preservation needed), retro-patch the
        // producer to a variant that writes straight to the local's slot and skip the SetSlot op
        अगर (o->foldPatchPC and preserveSlot == localSlot and Is_ढेर_शीर्ष_In_Register(o) and Get_ढेर_शीर्ष_सूचकांक(o) == (i16)o->fold_ढेर_सूचकांक) {
            IM3Operation foldOp = GetFoldOp(o->foldOpcode, o->foldForm);

            अगर (foldOp) {
                *(IM3Operation*)o->foldPatchPC = foldOp;     // retro-patch the producer
                EmitSlotOffset(o, localSlot);                  // append its destination
                InvalidateFold(o);
                folded = true;
            }
        }
#पूर्ण_अगर

        अगर (folded) {
            m3type_t type = Get_ढेर_शीर्ष_Type(o);
_           (निकालो(o));                              // the value now lives in the local's slot

            // A folded op writes its result through to the register as well, so a tee
            // can keep it there. Pushing the slot instead would make the next use that
            // wants a register reload the slot this op just stored to.
            अगर (i_opcode == c_waOp_teeLocal) {
_               (धकेलो_Register(o, type));
            }
        } अन्यथा {
            अगर (preserveSlot == localSlot) {
_               (Copy_ढेर_शीर्ष_To_Slot(o, localSlot))
            } अन्यथा {
_               (Preserved_Copy_शीर्ष_Slot(o, localSlot, preserveSlot))
            }

            अगर (i_opcode != c_waOp_teeLocal) {
_               (निकालो(o));
            }
        }
    } अन्यथा {
        _throw("local index out of bounds");
    }

    _catch: वापस परिणाम;
}

अटल
M3_परिणाम Compile_GetLocal (IM3Compilation o, m3opcode_t i_opcode)
{
_try {

    u32 local_सूचकांक;
_   (ReadLEB_u32(&local_सूचकांक, &o->wasm, o->wasmEnd));                           m3log (compile, d_indent " (index = %u)", get_indention_string (o), local_सूचकांक);

    अगर (local_सूचकांक >= Get_Function_अंक_Args_And_Locals(o->function)) {
        _throw("local index out of bounds");
    }

    m3type_t type = Get_ढेर_Type_From_Bottom(o, local_सूचकांक);
    u16      slot = Get_Slot_For_ढेर_सूचकांक(o, local_सूचकांक);

_   (धकेलो(o, type, slot));

    } _catch: वापस परिणाम;
}

अटल
M3_परिणाम Compile_GetGlobal (IM3Compilation o, M3Global* i_global)
{
    M3_परिणाम परिणाम;

    IM3Operation op = Is64BitType(i_global->type) ? op_GetGlobal_s64 : op_GetGlobal_s32;
_   (EmitOp(o, op));
    EmitPointer(o, &i_global->i64_मूल्य);
_   (धकेलो_Allocated_Slot_And_Emit_2(o, i_global->type));

    _catch: वापस परिणाम;
}

अटल
M3_परिणाम Compile_SetGlobal (IM3Compilation o, M3Global* i_global)
{
    M3_परिणाम परिणाम = m3Err_none;

    अगर (i_global->isMutable) {
        // Spec: value type must match global type
        अगर (not Is_ढेर_Polymorphic(o)) {
            m3type_t ढेर_शीर्ष_Type = Get_ढेर_शीर्ष_Type(o);
            _throwif(m3Err_typeMismatch, ढेर_शीर्ष_Type != c_m3Type_none and
                                           not IsSubTypeOf(ढेर_शीर्ष_Type, i_global->type));
        }

        IM3Operation op;
        m3type_t     type = Get_ढेर_शीर्ष_Type(o);

        अगर (Is_ढेर_शीर्ष_In_Register(o)) {
            op = c_setGlobalOps[BaseTypeOf(type)];
        } अन्यथा {
            op = Is64BitType(type) ? op_SetGlobal_s64 : op_SetGlobal_s32;
        }

_       (EmitOp(o, op));
        EmitPointer(o, &i_global->i64_मूल्य);

        अगर (Is_ढेर_शीर्ष_In_Slot(o)) {
            EmitSlotOffset(o, Get_ढेर_शीर्ष_Slot_संख्या(o));
        }

_       (निकालो(o));
    } अन्यथा {
        _throw(m3Err_settingImmutableGlobal);
    }

    _catch: वापस परिणाम;
}

अटल
M3_परिणाम Compile_GetSetGlobal (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_परिणाम परिणाम = m3Err_none;

    u32 global_सूचकांक;
_   (ReadLEB_u32(&global_सूचकांक, &o->wasm, o->wasmEnd));

    अगर (global_सूचकांक < o->module->अंक_Globals) {
        अगर (o->module->globals) {
            M3Global* global = &o->module->globals[global_सूचकांक];

            // Spec: a constant expression may only read an imported immutable
            // global. The module's own globals are counted before their
            // initializer is walked, so a bare index check would let one
            // reference itself. These describe how the module declared the
            // global, so they are checked before following any link.
            _throwif(m3_Err_globa_सूचकांक_Out_Of_Bounds, o->is_Init_व्यंजक and not global->imported);
            _throwif(m3Err_wasmMalformed, o->is_Init_व्यंजक and global->isMutable);

            // an import linked to another module reads and writes that module's
            // cell, not the placeholder standing in for it here
            अगर (global->resolved) {
                global = global->resolved;
            }

_           ((i_opcode == c_waOp_getGlobal) ? Compile_GetGlobal(o, global) : Compile_SetGlobal(o, global));
        } अन्यथा {
            _throw(ErrorCompile(m3Err_globalMemoryNotAllocated, o, "module '%s' is missing global memory", o->module->name));
        }
    } अन्यथा {
        _throw(m3_Err_globa_सूचकांक_Out_Of_Bounds);
    }

    _catch: वापस परिणाम;
}

अटल
व्योम EmitPatchingBranchPointer (IM3Compilation o, IM3CompilationScope i_scope)
{
    pc_t patch       = EmitPointer(o, i_scope->patches);                m3log (compile, "branch patch required at: %p", patch);
    i_scope->patches = patch;
}

अटल
M3_परिणाम EmitPatchingBranch (IM3Compilation o, IM3CompilationScope i_scope)
{
    M3_परिणाम परिणाम = m3Err_none;

_   (EmitOp(o, op_Branch));
    EmitPatchingBranchPointer(o, i_scope);

    _catch: वापस परिणाम;
}

#अगर d_m3HasExceptionHandling

// How many try_table handler records an exit from i_from out to i_target takes
// down with it. Walking outward from the block the branch sits in, every
// try_table scope up to and including the target is left behind - the target
// too, because landing on a block's label means that block is finished.
//
// A loop label is the exception, but a loop is never a try_table, so the count
// is the same either way: the loop itself is not on the list.
अटल
u32 गिनती_Try_Frames_To_Exit (IM3CompilationScope i_from, IM3CompilationScope i_target)
{
    u32 गिनती = 0;

    क्रम (IM3CompilationScope scope = i_from; scope; scope = scope->outer) {
        अगर (scope->opcode == c_waOp_tryTable) {
            ++गिनती;
        }

        अगर (scope == i_target) {
            अवरोध;
        }
    }

    वापस गिनती;
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
अटल
u32 अंक_Try_Frames_To_निकालो (IM3CompilationScope i_from, IM3CompilationScope i_target)
{
    अगर (i_target->depth == 0) {
        वापस 0;
    }

    // the walk only ever runs inward-to-outward; a target that is already
    // outside i_from has nothing between them
    अगर (not i_from or i_from->depth < i_target->depth) {
        वापस 0;
    }

    वापस गिनती_Try_Frames_To_Exit(i_from, i_target);
}


अटल
M3_परिणाम Emit_निकालो_Try_Frames (IM3Compilation o, u32 i_अंक_Frames)
{
    M3_परिणाम परिणाम = m3Err_none;

    अगर (i_अंक_Frames) {
_       (EmitOp(o, op_निकालो_Handlers));
        EmitConstant32(o, i_अंक_Frames);
    }

    _catch: वापस परिणाम;
}


// A return_call leaves the enclosing function before the callee runs, so no
// try_table in this function may catch what the callee throws. Neither shape of
// the instruction unwinds the native stack in time to arrange that on its own:
// op_ReturnCall tail-jumps with op_TryTable's frames still standing, and the
// fallback shape is an ordinary call sitting inside them. Either way the
// handler records have to come off first.
अटल
M3_परिणाम Emit_निकालो_Try_Frames_For_Return_Call (IM3Compilation o)
{
    u32 गिनती = 0;

    क्रम (IM3CompilationScope scope = &o->block; scope; scope = scope->outer) {
        अगर (scope->opcode == c_waOp_tryTable) {
            ++गिनती;
        }
    }

    वापस Emit_निकालो_Try_Frames(o, गिनती);
}

#अन्यथा

अटल
M3_परिणाम Emit_निकालो_Try_Frames (IM3Compilation o, u32 i_अंक_Frames)
{
    (व्योम)o;
    (व्योम)i_अंक_Frames;
    वापस m3Err_none;
}

#पूर्ण_अगर // d_m3HasExceptionHandling

#अगर d_m3FuseBranch
// when the pending fold candidate is a compare whose result sits on top in _r0, return the fused
// variant that absorbs an immediately following br_if (i_isIf false) or if (i_isIf true)
अटल
IM3Operation GetBranchFusionOp (IM3Compilation o, bool i_isIf)
{
    अगर (o->foldPatchPC and o->foldForm < 4 and Is_ढेर_शीर्ष_In_Register(o) and Get_ढेर_शीर्ष_सूचकांक(o) == (i16)o->fold_ढेर_सूचकांक) {
        स्थिर M3FusedCmpOps* ops = GetFusedCmpOps(o->foldOpcode);

        अगर (ops) {
            वापस i_isIf ? ops->ifOp[o->foldForm] : ops->branchIf[o->foldForm];
        }
    }

    वापस शून्य;
}
#पूर्ण_अगर // d_m3FuseBranch

अटल अंतरभूत
IM3Operation Op_ContinueLoop (IM3Compilation o)
{
#अगर d_m3_Has_ढेर_Switching
    अगर (M3_UNLIKELY(o->module->runtime and o->module->runtime->isSuspendable)) {
        वापस op_ContinueLoop_Suspendable;
    }
#पूर्ण_अगर
    वापस op_ContinueLoop;
}

अटल अंतरभूत
IM3Operation Op_ContinueLoopIf (IM3Compilation o)
{
#अगर d_m3_Has_ढेर_Switching
    अगर (M3_UNLIKELY(o->module->runtime and o->module->runtime->isSuspendable)) {
        वापस op_ContinueLoopIf_Suspendable;
    }
#पूर्ण_अगर
    वापस op_ContinueLoopIf;
}


अटल
M3_परिणाम Compile_Branch (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_परिणाम परिणाम;
    u32      अंक_Try_Frames = 0;

    u32 depth;
_   (ReadLEB_u32(&depth, &o->wasm, o->wasmEnd));

    // Spec: br_if condition must be i32
    अगर (i_opcode == c_waOp_branchIf and not Is_ढेर_Polymorphic(o)) {
        m3type_t condType = Get_ढेर_शीर्ष_Type(o);
        _throwif(m3Err_typeMismatch, condType != c_m3Type_none and condType != c_m3Type_i32);
    }

    IM3CompilationScope scope;
_   (GetBlockScope(o, &scope, depth));

#अगर d_m3HasExceptionHandling
    अंक_Try_Frames = अंक_Try_Frames_To_निकालो(&o->block, scope);
#पूर्ण_अगर

    // branch target is a loop (continue)
    अगर (scope->opcode == c_waOp_loop) {
        अगर (i_opcode == c_waOp_branchIf) {
            // a handler pop belongs on the taken path only, so it forces the
            // jump-over form even where the target takes no parameters
            अगर (Get_Func_Type_अंक_Params(scope->type) or अंक_Try_Frames) {
                IM3Operation op = Is_ढेर_शीर्ष_In_Register(o) ? op_BranchIfPrologue_r : op_BranchIfPrologue_s;

_               (EmitOp(o, op));
_               (Emit_Slot_अंक_Of_ढेर_शीर्ष_And_निकालो(o));

                pc_t* jumpTo = (pc_t*)ReservePointer(o);

_               (Emit_निकालो_Try_Frames(o, अंक_Try_Frames));

                अगर (not Is_ढेर_Polymorphic(o)) {
_                   (ResolveBlockResults(o, scope, /* isBranch: */ true));
                }

_               (EmitOp(o, Op_ContinueLoop(o)));
                EmitPointer(o, scope->pc);
_               (RecordBackEdge(o, scope, false));

                *jumpTo = GetPC(o);
            } अन्यथा {
                // move the condition to a register
_               (Copy_ढेर_शीर्ष_To_Register(o, false));
_               (निकालो_Type(o, c_m3Type_i32));

_               (EmitOp(o, Op_ContinueLoopIf(o)));
                EmitPointer(o, scope->pc);
_               (RecordBackEdge(o, scope, true));
            }

            //          dump_type_stack(o);
        } अन्यथा // is c_waOp_branch
        {
_           (Emit_निकालो_Try_Frames(o, अंक_Try_Frames));

            // the branch operands become the loop's parameters on the next iteration
            अगर (not Is_ढेर_Polymorphic(o)) {
_               (ResolveBlockResults(o, scope, /* isBranch: */ true));
            }

_           (EmitOp(o, Op_ContinueLoop(o)));
            EmitPointer(o, scope->pc);
_           (RecordBackEdge(o, scope, false));
            o->block.isPolymorphic = true;
        }
    } अन्यथा // forward branch
    {
        pc_t* jumpTo = शून्य;

        bool isReturn         = (scope->depth == 0);
        bool targetHasResults = Get_Func_Type_अंक_Results(scope->type);

        अगर (i_opcode == c_waOp_branchIf) {
            अगर (targetHasResults or isReturn or अंक_Try_Frames) {
                IM3Operation op = Is_ढेर_शीर्ष_In_Register(o) ? op_BranchIfPrologue_r : op_BranchIfPrologue_s;

_               (EmitOp(o, op));
_               (Emit_Slot_अंक_Of_ढेर_शीर्ष_And_निकालो(o)); // condition

                // this is continuation point, if the branch isn't taken
                jumpTo = (pc_t*)ReservePointer(o);
            } अन्यथा {
#अगर d_m3FuseBranch
                IM3Operation fuseOp = GetBranchFusionOp(o, false);

                अगर (fuseOp) {
                    // absorb the branch into the compare that produced the condition
                    *(IM3Operation*)o->foldPatchPC = fuseOp;
                    InvalidateFold(o);

_                   (निकालो(o));                      // the condition is consumed inside the fused op

                    EmitPatchingBranchPointer(o, scope);
                    जाओ _catch;
                }
#पूर्ण_अगर
                IM3Operation op = Is_ढेर_शीर्ष_In_Register(o) ? op_BranchIf_r : op_BranchIf_s;

_               (EmitOp(o, op));
_               (Emit_Slot_अंक_Of_ढेर_शीर्ष_And_निकालो(o)); // condition

                EmitPatchingBranchPointer(o, scope);
                जाओ _catch;
            }
        }

        अगर (not Is_ढेर_Polymorphic(o)) {
            अगर (isReturn) {
_               (Return_मूल्यावली(o, scope, true));
_               (EmitOp(o, op_Return));
            } अन्यथा {
_               (Emit_निकालो_Try_Frames(o, अंक_Try_Frames));

_               (ResolveBlockResults(o, scope, true));
_               (EmitPatchingBranch(o, scope));
            }
        }

        अगर (jumpTo) {
            *jumpTo = GetPC(o);
        }

        अगर (i_opcode == c_waOp_branch) {
_           (Set_ढेर_Polymorphic(o));
        }
    }

    _catch: वापस परिणाम;
}

अटल
M3_परिणाम Compile_BranchTable (IM3Compilation o, m3opcode_t i_opcode)
{
    // the page a continue-op page is standing in for; non-NULL only while that
    // page is installed in o->page, so the catch below knows to put it back
    IM3CodePage displacedPage = शून्य;
    // indexed by label depth: the continuation stub already emitted for that target
    // within this table, or NULL if this table has not reached it yet
    pc_t* targetStubs = शून्य;
_try {
    u32 target_गिनती;
_   (ReadLEB_u32(&target_गिनती, &o->wasm, o->wasmEnd));

    // Spec: validate that the branch index operand is i32
    अगर (not Is_ढेर_Polymorphic(o)) {
        m3type_t सूचकांक_Type = Get_ढेर_शीर्ष_Type(o);
        _throwif(m3Err_typeMismatch, सूचकांक_Type != c_m3Type_none and सूचकांक_Type != c_m3Type_i32);
    }

_   (PreserveRegisterIfOccupied(o, c_m3Type_i64));         // move branch operand to a slot
    u16 slot = Get_ढेर_शीर्ष_Slot_संख्या(o);
_   (निकालो(o));

    // OPTZ: according to spec: "forward branches that target a control instruction with a non-empty
    // result type consume matching operands first and push them back on the operand stack after unwinding"
    // So, this move-to-reg is only necessary if the target scopes have a type.

    u32 अंक_Code_पंक्तियाँ = target_गिनती + 4; // 3 => IM3Operation + slot + target_count + default_target
_   (Ensure_Code_Page_अंक_पंक्तियाँ(o, अंक_Code_पंक्तियाँ));

_   (EmitOp(o, op_BranchTable));
    EmitSlotOffset(o, slot);
    EmitConstant32(o, target_गिनती);

    IM3CodePage continueOpPage = शून्य;

    // The label types are checked by ValidateFunction, which runs under the same
    // d_m3EnableValidation guard just before this compilation pass. Comparing each
    // target against the default here would be wrong anyway: the spec matches every
    // target against the operand stack, so in unreachable code the operands are the
    // bottom type and the targets may legitimately differ (unreached-valid.wast).

    ++target_गिनती; // include default

    // Every entry of one br_table is resolved from the same compilation state - each
    // stub restores whatever it touched - so two entries naming the same label emit
    // byte-identical epilogues. Emit one and point both table entries at it. A label
    // deeper than the current block is rejected by GetBlockScope below, which is what
    // bounds this array.
    targetStubs = m3_AllocArray(pc_t, (माप_प्रकार)o->block.depth + 1);
    _throwifnull(targetStubs);

    क्रम (u32 i = 0; i < target_गिनती; ++i) {
        u32 target;
_       (ReadLEB_u32(&target, &o->wasm, o->wasmEnd));

        IM3CompilationScope scope;
_       (GetBlockScope(o, &scope, target));

        अगर (targetStubs[target]) {
            EmitPointer(o, targetStubs[target]);
            जारी;
        }

        u32 अंक_Try_Frames = 0;
#अगर d_m3HasExceptionHandling
        अंक_Try_Frames = अंक_Try_Frames_To_निकालो(&o->block, scope);
#पूर्ण_अगर

        // A forward branch to a block that carries no results and leaves no try
        // frames behind has nothing to do on the way out but arrive. There is no
        // epilogue to hold, so the table entry becomes the patch site itself and the
        // block's address is written into it when the block ends - no stub, and no
        // code page to put one on. In unreachable code the same is true whatever the
        // block's type, since nothing is emitted for that case either way.
        //
        // Each such entry is patched on its own, so none is recorded in targetStubs:
        // there is no shared stub for a later entry naming this label to point at.
        अगर (scope->opcode != c_waOp_loop and scope->depth > 0 and अंक_Try_Frames == 0 and
            (Is_ढेर_Polymorphic(o) or Get_Func_Type_अंक_Results(scope->type) == 0)) {
            EmitPatchingBranchPointer(o, scope);
            जारी;
        }

_       (AcquireCompilationCodePage(o, &continueOpPage));

        pc_t startPC  = GetPagePC(continueOpPage);
        displacedPage = o->page;
        o->page       = continueOpPage;

        अगर (scope->opcode == c_waOp_loop) {
_           (Emit_निकालो_Try_Frames(o, अंक_Try_Frames));

            अगर (not Is_ढेर_Polymorphic(o)) {
_               (ResolveBlockResults(o, scope, true));
            }

_           (EmitOp(o, Op_ContinueLoop(o)));
            EmitPointer(o, scope->pc);
_           (RecordBackEdge(o, scope, false));
        } अन्यथा {
            अगर (not Is_ढेर_Polymorphic(o)) {
                अगर (scope->depth == 0) {
_                   (Return_मूल्यावली(o, scope, true));
_                   (EmitOp(o, op_Return));
                } अन्यथा {
_                   (Emit_निकालो_Try_Frames(o, अंक_Try_Frames));

_                   (ResolveBlockResults(o, scope, true));

_                   (EmitPatchingBranch(o, scope));
                }
            }
        }

        IM3CodePage returnTo = displacedPage;
        displacedPage        = शून्य;

        ReleaseCompilationCodePage(o);
        o->page = returnTo;

        EmitPointer(o, startPC);
        targetStubs[target] = startPC;
    }

_   (Set_ढेर_Polymorphic(o));
} _catch:

    // thrown out of the loop above with a continue-op page installed: release it
    // and restore the one it displaced, which the caller's catch then releases.
    // Otherwise the displaced page is on no list and simply leaks.
    अगर (displacedPage) {
        ReleaseCompilationCodePage(o);
        o->page = displacedPage;
    }

    m3_Free(targetStubs);

    वापस परिणाम;
}

अटल
M3_परिणाम CompileCallArgsAndReturn (IM3Compilation o, u16* o_ढेर_Offset, IM3FuncType i_type, bool i_isIndirect)
{
_try {

    u16 शीर्ष_Slot = GetMaxUsedSlotPlusOne(o);

    // force use of at least one stack slot; this is to help ensure
    // the m3 stack overflows (and traps) before the native stack can overflow.
    // e.g. see Wasm spec test 'runaway' in call.wast
    शीर्ष_Slot = M3_MAX(1, शीर्ष_Slot);

    // stack frame is 64-bit aligned
    AlignSlotToType(&शीर्ष_Slot, c_m3Type_i64);

    *o_ढेर_Offset = शीर्ष_Slot;

    // wait to pop this here so that topSlot search is correct
    अगर (i_isIndirect) {
_       (निकालो(o));
    }

    u16 अंक_Args = Get_Func_Type_अंक_Params(i_type);
    u16 अंक_Rets = Get_Func_Type_अंक_Results(i_type);

    u16 arg_शीर्ष = शीर्ष_Slot + (अंक_Args + अंक_Rets) * c_io_Slot_गिनती;

    TouchSlot(o, arg_शीर्ष - 1);

    जबतक (अंक_Args--) {
_       (Copy_ढेर_शीर्ष_To_Slot(o, arg_शीर्ष -= c_io_Slot_गिनती));
_       (निकालो(o));
    }

    u16 i = 0;
    जबतक (अंक_Rets--) {
        m3type_t type = Get_Func_Type_परिणाम_Type(i_type, i++);

_       (धकेलो(o, type, शीर्ष_Slot));
_       (MarkSlotsAllocatedByType(o, शीर्ष_Slot, type));

        शीर्ष_Slot += c_io_Slot_गिनती;
    }

    } _catch: वापस परिणाम;
}

// A tail call hands the current frame over to the callee instead of stacking a new one.
// The callee's results have to be the enclosing function's results, so both use the same
// return slots and only the arguments move: they're staged above the caller's stack here
// (the argument expressions can still be reading the args/locals they're about to land on)
// and slid down onto the frame base at runtime, by op_ReturnCall[Indirect].
अटल
M3_परिणाम Compile_पुच्छ_Call_Args (IM3Compilation o, u16* o_ढेर_Offset, u16* o_अंक_Arg_Slots, IM3FuncType i_type, bool i_isIndirect)
{
_try {
    IM3FuncType funcType = o->function->funcType;

    u16 अंक_Rets = Get_Func_Type_अंक_Results(i_type);
    _throwif(m3Err_typeMismatch, अंक_Rets != Get_Func_Type_अंक_Results(funcType));

    क्रम (u16 i = 0; i < अंक_Rets; ++i) {
        _throwif(m3Err_typeMismatch, Get_Func_Type_परिणाम_Type(i_type, i) != Get_Func_Type_परिणाम_Type(funcType, i));
    }

    u16 शीर्ष_Slot = GetMaxUsedSlotPlusOne(o);

    शीर्ष_Slot = M3_MAX(1, शीर्ष_Slot);

    // stack frame is 64-bit aligned
    AlignSlotToType(&शीर्ष_Slot, c_m3Type_i64);

    *o_ढेर_Offset = शीर्ष_Slot;

    // wait to pop this here so that topSlot search is correct
    अगर (i_isIndirect) {
_       (निकालो(o));
    }

    u16 अंक_Args     = Get_Func_Type_अंक_Params(i_type);
    u16 अंक_Arg_Slots = अंक_Args * c_io_Slot_गिनती;

    *o_अंक_Arg_Slots = अंक_Arg_Slots;

    u16 arg_शीर्ष = शीर्ष_Slot + अंक_Arg_Slots;
    _throwif(m3_Err_function_ढेर_Overflow, arg_शीर्ष > d_m3MaxFunctionSlots);

    // the staging area is written by the caller, so op_Entry has to account for it
    TouchSlot(o, arg_शीर्ष - 1);

    जबतक (अंक_Args--) {
_       (Copy_ढेर_शीर्ष_To_Slot(o, arg_शीर्ष -= c_io_Slot_गिनती));
_       (निकालो(o));
    }

    } _catch: वापस परिणाम;
}


#अगर d_m3HasTypedRefs

// call_ref $t: the callee is the reference on top of the stack, so the operand
// order is the same as call_indirect's, minus the table.
अटल
M3_परिणाम Compile_CallRef (IM3Compilation o, m3opcode_t i_opcode)
{
_try {
    u32 type_सूचकांक;
_   (ReadLEB_u32(&type_सूचकांक, &o->wasm, o->wasmEnd));

    _throwif("function call type index out of range", type_सूचकांक >= o->module->अंक_Func_Types);

    IM3FuncType type = o->module->funcTypes[type_सूचकांक];

    अगर (not Is_ढेर_Polymorphic(o)) {
        // whatever shape the reference has, it must be a function of this type
        m3type_t refType = Get_ढेर_शीर्ष_Type(o);
        _throwif(m3Err_typeMismatch, not IsSubTypeOf(refType, RefTypeOfFuncType(type, false)));
    }

    अगर (Is_ढेर_शीर्ष_In_Register(o)) {
_       (PreserveRegisterIfOccupied(o, c_m3Type_funcref));
    }

    u16 functionSlot = Get_ढेर_शीर्ष_Slot_संख्या(o);

    bool isReturnCall = (i_opcode == c_waOp_returnCallRef);
    bool use_पुच्छ_Call  = isReturnCall and d_m3_Can_पुच्छ_Call;

    u16 exec_शीर्ष, अंक_Arg_Slots = 0;

    अगर (use_पुच्छ_Call) {
_       (Compile_पुच्छ_Call_Args(o, &exec_शीर्ष, &अंक_Arg_Slots, type, true));
    } अन्यथा {
_       (CompileCallArgsAndReturn(o, &exec_शीर्ष, type, true));
    }

#  अगर d_m3HasExceptionHandling
    अगर (isReturnCall) {
_       (Emit_निकालो_Try_Frames_For_Return_Call(o));
    }
#  endif

_   (EmitOp(o, use_पुच्छ_Call ? op_ReturnCallRef : op_CallRef));
    EmitSlotOffset(o, functionSlot);
    EmitPointer(o, type);
    EmitSlotOffset(o, exec_शीर्ष);

    अगर (not use_पुच्छ_Call) {
_       (RecordCall(o, Get_Func_Type_अंक_Results(type), exec_शीर्ष, isReturnCall));
    }

    अगर (use_पुच्छ_Call) {
        EmitSlotOffset(o, o->function->अंक_Ret_Slots);
        EmitConstant32(o, अंक_Arg_Slots);

_       (Set_ढेर_Polymorphic(o));
    } अन्यथा अगर (isReturnCall) {
_       (Compile_Return(o, i_opcode));
    }

} _catch:
    वापस परिणाम;
}


// ref.as_non_null only sharpens the type; the value is unchanged, so the stack
// entry is re-pushed with the null stripped out of its type.
अटल
M3_परिणाम Compile_Ref_AsNonNull (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_परिणाम परिणाम = m3Err_none;

    अगर (not Is_ढेर_Polymorphic(o)) {
        m3type_t type = Get_ढेर_शीर्ष_Type(o);
        _throwif(m3Err_typeMismatch, not IsRefType(type));

        // op_RefAsNonNull only knows how to read its operand out of a slot; a
        // value still sitting in the register has to be spilled first, the way
        // every other single-op reader of the stack top does
        अगर (Is_ढेर_शीर्ष_In_Register(o)) {
_           (PreserveRegisterIfOccupied(o, type));
        }

        u16 slot = Get_ढेर_शीर्ष_Slot_संख्या(o);

_       (EmitOp(o, op_RefAsNonNull));
        EmitSlotOffset(o, slot);

        m3type_t nonNull = IsSpelledRefType(type)
                             ? (m3type_t)(type | d_m3Type_refNonNull)
                             : (m3type_t)(d_m3Type_ref | d_m3Type_refNonNull | d_m3Type_heapAbstract |
                                          ((BaseTypeOf(type) == c_m3Type_externref) ? d_m3Type_refExtern : 0));

        // the slot keeps the one reference it already had; a Pop()/Push() of
        // the same slot would drop that reference's count without anything
        // to put it back, so the type above it is corrected in place instead
        o->type_ढेर[Get_ढेर_शीर्ष_सूचकांक(o)] = nonNull;
    }

    _catch: वापस परिणाम;
}

#पूर्ण_अगर // d_m3HasTypedRefs


// How wide a table operand is read at comes from the table it names, so a slot
// holding a narrower type would be read past what was allocated for it. The
// validator refuses that already; this keeps a build without one from reaching
// the interpreter with it. c_m3Type_none means the operand is not on the
// dynamic stack to inspect, which is not this check's business.
अटल
M3_परिणाम CheckOperandType (IM3Compilation o, u16 i_depth_From_शीर्ष, m3type_t i_type)
{
    m3type_t actual;

    अगर (Is_ढेर_Polymorphic(o)) {
        वापस m3Err_none;
    }

    actual = Get_ढेर_Type_From_शीर्ष(o, i_depth_From_शीर्ष);

    वापस (actual == c_m3Type_none or BaseTypeOf(actual) == i_type) ? m3Err_none
                                                                     : m3Err_typeMismatch;
}


// Swaps the _mem register onto a given memory. Only ever emitted in pairs --
// see op_SetMemory.
अटल
M3_परिणाम EmitSetMemoryPtr (IM3Compilation o, IM3Memory i_memory)
{
    M3_परिणाम परिणाम = m3Err_none;

_   (EmitOp(o, op_SetMemory));
    EmitPointer(o, i_memory);

    _catch: वापस परिणाम;
}


// ...onto one of the compiling module's own memories, by index. Only reached
// with a non-zero index under multi-memory, but the index has already been
// bounds-checked by ReadMemoryIndex either way.
अटल अंतरभूत
M3_परिणाम EmitSetMemory (IM3Compilation o, u32 i_memoryIdx)
{
#अगर d_m3HasMultiMemory
    वापस EmitSetMemoryPtr(o, o->module->memories[i_memoryIdx]);
#अन्यथा
    (व्योम)o;
    (व्योम)i_memoryIdx;
    वापस m3Err_unknownMemory;
#पूर्ण_अगर
}

अटल
M3_परिणाम Compile_Call (IM3Compilation o, m3opcode_t i_opcode)
{
_try {
    u32 function_सूचकांक;
_   (ReadLEB_u32(&function_सूचकांक, &o->wasm, o->wasmEnd));

    IM3Function function = Module_GetFunction(o->module, function_सूचकांक);

    अगर (function) {                                                     m3log (compile, d_indent " (func= [%d] '%s'; args= %d)",
                                                                                get_indention_string (o), function_सूचकांक, m3_GetFunctionName (function), function->funcType->अंक_Args);
        // an import linked to another module's export runs that module's body
        IM3Function target = Function_Implementation(function);

        अगर (target->module) {
            bool isReturnCall = (i_opcode == c_waOp_returnCall);

            // The callee's body runs against its own module's memory, so a call
            // that crosses modules is bracketed by a pair of op_SetMemory. A
            // tail call never returns to run the second one, so those compile
            // as a plain call followed by a return instead.
            bool crossModule = (target->module != o->module);
            bool use_पुच्छ_Call = isReturnCall and d_m3_Can_पुच्छ_Call and not crossModule;

            u16 slot_शीर्ष, अंक_Arg_Slots = 0;

            अगर (use_पुच्छ_Call) {
_               (Compile_पुच्छ_Call_Args(o, &slot_शीर्ष, &अंक_Arg_Slots, target->funcType, false));
            } अन्यथा {
_               (CompileCallArgsAndReturn(o, &slot_शीर्ष, target->funcType, false));
            }

            IM3Operation op;
            स्थिर व्योम*  operand;

            अगर (target->compiled) {
                op      = use_पुच्छ_Call ? op_ReturnCall : op_Call;
                operand = target->compiled;
            } अन्यथा {
                op      = use_पुच्छ_Call ? op_CompileReturnCall : op_Compile;
                operand = target;
            }

#अगर d_m3HasExceptionHandling
            अगर (isReturnCall) {
_               (Emit_निकालो_Try_Frames_For_Return_Call(o));
            }
#पूर्ण_अगर

            अगर (crossModule) {
_               (EmitSetMemoryPtr(o, Module_Memory0(target->module)));
            }

_           (EmitOp(o, op));
            EmitPointer(o, operand);
            EmitSlotOffset(o, slot_शीर्ष);

            अगर (not use_पुच्छ_Call) {
                // a suspend inside the callee leaves this frame waiting here, its
                // results not yet written
_               (RecordCall(o, Get_Func_Type_अंक_Results(target->funcType), slot_शीर्ष, isReturnCall));
            }

            अगर (use_पुच्छ_Call) {
                EmitSlotOffset(o, o->function->अंक_Ret_Slots);
                EmitConstant32(o, अंक_Arg_Slots);

_               (Set_ढेर_Polymorphic(o));
            } अन्यथा {
                अगर (crossModule) {
_                   (EmitSetMemoryPtr(o, Module_Memory0(o->module)));
                }

                अगर (isReturnCall) {
_                   (Compile_Return(o, i_opcode));
                }
            }
        } अन्यथा {
            _throw(ErrorCompile(m3Err_functionImportMissing, o, "'%s.%s'", GetFunctionImportModuleName(function), m3_GetFunctionName(function)));
        }
    } अन्यथा {
        _throw(ErrorCompile(m3Err_functionLookupFailed, o, "index: %d", function_सूचकांक));
    }

    } _catch: वापस परिणाम;
}

अटल
M3_परिणाम Compile_CallIndirect (IM3Compilation o, m3opcode_t i_opcode)
{
_try {
    u32 type_सूचकांक;
_   (ReadLEB_u32(&type_सूचकांक, &o->wasm, o->wasmEnd));

    u32 table_सूचकांक;
_   (ReadLEB_u32(&table_सूचकांक, &o->wasm, o->wasmEnd));

    _throwif("function call type index out of range", type_सूचकांक >= o->module->अंक_Func_Types);
    _throwif("table index out of range", table_सूचकांक >= o->module->अंक_Tables);
    _throwif(m3Err_typeMismatch, BaseTypeOf(o->module->tables[table_सूचकांक]->type) != c_m3Type_funcref);

_   (CheckOperandType(o, 0, Table_AddrType(o->module->tables[table_सूचकांक])));

    अगर (Is_ढेर_शीर्ष_In_Register(o)) {
_       (PreserveRegisterIfOccupied(o, c_m3Type_i32));
    }

    u16 table_सूचकांक_Slot = Get_ढेर_शीर्ष_Slot_संख्या(o);

    bool isReturnCall = (i_opcode == c_waOp_returnCallIndirect);
    bool use_पुच्छ_Call  = isReturnCall and d_m3_Can_पुच्छ_Call;

    u16         exec_शीर्ष, अंक_Arg_Slots = 0;
    IM3FuncType type = o->module->funcTypes[type_सूचकांक];

    अगर (use_पुच्छ_Call) {
_       (Compile_पुच्छ_Call_Args(o, &exec_शीर्ष, &अंक_Arg_Slots, type, true));
    } अन्यथा {
_       (CompileCallArgsAndReturn(o, &exec_शीर्ष, type, true));
    }

#अगर d_m3HasExceptionHandling
    अगर (isReturnCall) {
_       (Emit_निकालो_Try_Frames_For_Return_Call(o));
    }
#पूर्ण_अगर

    // the table comes first: how wide its indexes are is what says how much of
    // the slot below holds one
_   (EmitOp(o, use_पुच्छ_Call ? op_ReturnCallIndirect : op_CallIndirect));
    EmitPointer(o, o->module->tables[table_सूचकांक]);
    EmitSlotOffset(o, table_सूचकांक_Slot);
    EmitPointer(o, type);
    EmitSlotOffset(o, exec_शीर्ष);

    अगर (not use_पुच्छ_Call) {
_       (RecordCall(o, Get_Func_Type_अंक_Results(type), exec_शीर्ष, isReturnCall));
    }

    अगर (use_पुच्छ_Call) {
        EmitSlotOffset(o, o->function->अंक_Ret_Slots);
        EmitConstant32(o, अंक_Arg_Slots);

_       (Set_ढेर_Polymorphic(o));
    } अन्यथा अगर (isReturnCall) {
_       (Compile_Return(o, i_opcode));
    }

} _catch:
    वापस परिणाम;
}

// Reads the memory index a memory instruction names, and checks it against the
// module's index space. Without multi-memory the immediate is a reserved byte
// that has to be zero, which is the same check.
अटल
M3_परिणाम Read_Memory_सूचकांक (IM3Compilation o, u32* o_memoryIdx)
{
    M3_परिणाम परिणाम;

_   (ReadLEB_u32(o_memoryIdx, &o->wasm, o->wasmEnd));

    _throwif(m3Err_unknownMemory, *o_memoryIdx >= o->module->अंक_Memories);

    _catch: वापस परिणाम;
}

अटल
M3_परिणाम Compile_Memory_आकार (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_परिणाम परिणाम;

    u32 memoryIdx;

    // A page count is given in the memory's own address type. Declared up here
    // so the throws below don't jump over its initialization.
    m3type_t addrType = c_m3Type_i32;

_   (Read_Memory_सूचकांक(o, &memoryIdx));

    addrType = Memory_AddrType(o->module->memories[memoryIdx]);

_   (PreserveRegisterIfOccupied(o, addrType));

    अगर (memoryIdx) {
_       (EmitSetMemory(o, memoryIdx));
    }

    // op_MemSize writes the whole register either way; the slot it is pushed
    // to is what decides how much of it the module gets to see
_   (EmitOp(o, op_Mem_आकार));

    अगर (memoryIdx) {
_       (EmitSetMemory(o, 0));
    }

_   (धकेलो_Register(o, addrType));

    _catch: वापस परिणाम;
}

अटल
M3_परिणाम Compile_Memory_Grow (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_परिणाम परिणाम;

    u32 memoryIdx;

    // see Compile_Memory_Size
    m3type_t addrType = c_m3Type_i32;

_   (Read_Memory_सूचकांक(o, &memoryIdx));

    addrType = Memory_AddrType(o->module->memories[memoryIdx]);

_   (Copy_ढेर_शीर्ष_To_Register(o, false));
_   (निकालो_Type(o, addrType));

    अगर (memoryIdx) {
_       (EmitSetMemory(o, memoryIdx));
    }

#अगर d_m3HasMemory64
_   (EmitOp(o, (addrType == c_m3Type_i64) ? op_MemGrow64 : op_MemGrow));
#अन्यथा
_   (EmitOp(o, op_MemGrow));
#पूर्ण_अगर

    अगर (memoryIdx) {
_       (EmitSetMemory(o, 0));
    }

_   (धकेलो_Register(o, addrType));

    _catch: वापस परिणाम;
}

अटल
M3_परिणाम Compile_Memory_CopyFill (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_परिणाम परिणाम = m3Err_none;

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
    m3type_t लंबाई_Type = c_m3Type_i32;

    _throwif(m3Err_wasmMalformed, o->module->अंक_Memories == 0);

_   (Read_Memory_सूचकांक(o, &targetMemoryIdx));

    अगर (isCopy) {
_       (Read_Memory_सूचकांक(o, &sourceMemoryIdx));
    }

    targetType = Memory_AddrType(o->module->memories[targetMemoryIdx]);
    sourceType = isCopy ? Memory_AddrType(o->module->memories[sourceMemoryIdx])
                        : targetType;
    लंबाई_Type = (targetType == c_m3Type_i64 and sourceType == c_m3Type_i64)
                   ? c_m3Type_i64
                   : c_m3Type_i32;

_   (Copy_ढेर_शीर्ष_To_Register(o, false));

#अगर d_m3HasMultiMemory
    अगर (isCopy and sourceMemoryIdx != targetMemoryIdx) {
        // two memories at once: _mem can only name one, so the op takes both
_       (EmitOp(o, op_MemCopy_x));
        EmitPointer(o, o->module->memories[targetMemoryIdx]);
        EmitPointer(o, o->module->memories[sourceMemoryIdx]);

        // ...and, since they need not be addressed alike, how wide to read
        // each of the operands: bit 0 the destination, bit 1 the source
        EmitConstant32(o, ((targetType == c_m3Type_i64) ? 0x1u : 0u) |
                            ((sourceType == c_m3Type_i64) ? 0x2u : 0u));
    } अन्यथा
#पूर्ण_अगर
    {
        IM3Operation op = isCopy ? op_MemCopy : op_MemFill;

#अगर d_m3HasMemory64
        अगर (targetType == c_m3Type_i64) {
            op = isCopy ? op_MemCopy64 : op_MemFill64;
        }
#पूर्ण_अगर

        अगर (targetMemoryIdx) {
_           (EmitSetMemory(o, targetMemoryIdx));
        }

_       (EmitOp(o, op));
    }

    // the length is in the register; the other two are named by slot, at
    // whatever width their stack entries were pushed with
_   (निकालो_Type(o, लंबाई_Type));
_   (Emit_Slot_अंक_Of_ढेर_शीर्ष_And_निकालो(o));      // the source, or the byte to fill with
_   (Emit_Slot_अंक_Of_ढेर_शीर्ष_And_निकालो(o));      // the destination

    अगर (targetMemoryIdx and not (isCopy and sourceMemoryIdx != targetMemoryIdx)) {
_       (EmitSetMemory(o, 0));
    }

    _catch: वापस परिणाम;
}


// memory.init and data.drop address a data segment by index. Both are only valid
// when a data count section declared the segments up front.
अटल
M3_परिणाम Read_तथ्य_Segment (IM3Compilation o, M3_तथ्य_Segment** o_segment)
{
    M3_परिणाम परिणाम = m3Err_none;

    u32 index;
_   (ReadLEB_u32(&index, &o->wasm, o->wasmEnd));

    _throwif("data count section required", not o->module->has_तथ्य_गिनती);
    _throwif(m3Err_wasmMalformed, index >= o->module->अंक_तथ्य_Segments);

    *o_segment = &o->module->तथ्य_Segments[index];

    _catch: वापस परिणाम;
}


M3_परिणाम Compile_Memory_Init (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_परिणाम परिणाम = m3Err_none;

    M3_तथ्य_Segment* segment = शून्य;
    u32            memoryIdx;

    _throwif(m3Err_wasmMalformed, o->module->अंक_Memories == 0);

_   (Read_तथ्य_Segment(o, &segment));
_   (Read_Memory_सूचकांक(o, &memoryIdx));

_   (Copy_ढेर_शीर्ष_To_Register(o, false));

    अगर (memoryIdx) {
_       (EmitSetMemory(o, memoryIdx));
    }

#अगर d_m3HasMemory64
    // only the destination follows the memory's address type: a data segment
    // is indexed as an i32 however the memory it is copied into is addressed
_   (EmitOp(o, o->module->memories[memoryIdx]->isMemory64 ? op_MemInit64 : op_MemInit));
#अन्यथा
_   (EmitOp(o, op_MemInit));
#पूर्ण_अगर
    EmitPointer(o, segment);
_   (निकालो_Type(o, c_m3Type_i32));
_   (Emit_Slot_अंक_Of_ढेर_शीर्ष_And_निकालो(o));
_   (Emit_Slot_अंक_Of_ढेर_शीर्ष_And_निकालो(o));

    अगर (memoryIdx) {
_       (EmitSetMemory(o, 0));
    }

    _catch: वापस परिणाम;
}


#अगर d_m3HasRefTypes

अटल
M3_परिणाम Compile_Select (IM3Compilation o, m3opcode_t i_opcode);

// select with an explicit result type vector. The types only matter to the
// validator; the operands are laid out exactly as for the untyped select.
M3_परिणाम Compile_Select_Typed (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_परिणाम परिणाम = m3Err_none;

    u32 अंक_Types;
_   (ReadLEB_u32(&अंक_Types, &o->wasm, o->wasmEnd));
    _throwif(m3Err_wasmMalformed, अंक_Types != 1);

    i8 waType;
    u8 type;
_   (ReadLEB_i7(&waType, &o->wasm, o->wasmEnd));
_   (NormalizeType(&type, waType));

_   (Compile_Select(o, i_opcode));

    _catch: वापस परिणाम;
}


// A reference is one pointer-sized word and null is 0, so null-testing it is
// just an integer eqz of the matching width.
#  अगर M3_SIZEOF_PTR == 8
#    define d_m3RefIsNull_r   op_i64_EqualToZero_r
#    define d_m3RefIsNull_s   op_i64_EqualToZero_s
#  अन्यथा
#    define d_m3RefIsNull_r   op_i32_EqualToZero_r
#    define d_m3RefIsNull_s   op_i32_EqualToZero_s
#  endif

अटल
M3_परिणाम ReadRefType (IM3Compilation o, m3type_t* o_type)
{
    M3_परिणाम परिणाम = m3Err_none;

#  अगर d_m3HasTypedRefs
    // ref.null names a heap type: func, extern, or a function type index
    m3type_t heapBits;
_   (ParseHeapType(o->module, &heapBits, &o->wasm, o->wasmEnd));

    *o_type = d_m3Type_ref | heapBits;
#  अन्यथा
    i8 waType;
    u8 plainType;
_   (ReadLEB_i7(&waType, &o->wasm, o->wasmEnd));
_   (NormalizeType(&plainType, waType));
    *o_type = plainType;
    _throwif(m3Err_wasmMalformed, not IsRefType(*o_type));
#  endif

    _catch: वापस परिणाम;
}


M3_परिणाम Compile_Ref_Null (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_परिणाम परिणाम = m3Err_none;

    m3type_t type;
_   (ReadRefType(o, &type));
_   (धकेलो_Const(o, 0, type));

    _catch: वापस परिणाम;
}


M3_परिणाम Compile_Ref_IsNull (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_परिणाम परिणाम = m3Err_none;

    IM3Operation op;

    अगर (not Is_ढेर_Polymorphic(o)) {
        _throwif(m3Err_typeMismatch, not IsRefType(Get_ढेर_शीर्ष_Type(o)));
    }

    अगर (Is_ढेर_शीर्ष_In_Register(o)) {
        op = d_m3RefIsNull_r;
    } अन्यथा {
_       (PreserveRegisterIfOccupied(o, c_m3Type_i32));
        op = d_m3RefIsNull_s;
    }

_   (EmitOp(o, op));
_   (Emit_Slot_अंक_Of_ढेर_शीर्ष_And_निकालो(o));
_   (धकेलो_Register(o, c_m3Type_i32));

    _catch: वापस परिणाम;
}


M3_परिणाम Compile_Ref_Func (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_परिणाम परिणाम = m3Err_none;

    u32 func_सूचकांक;

    // declared before the throws below, which jump past this point to _catch
    m3type_t    refType   = c_m3Type_funcref;
    IM3Function reference = शून्य;

_   (ReadLEB_u32(&func_सूचकांक, &o->wasm, o->wasmEnd));
    _throwif("function index out of range", func_सूचकांक >= o->module->अंक_Functions);

    // Inside a constant expression ref.func is itself a declaration; inside a
    // function body the function must already have been declared elsewhere.
    अगर (o->function) {
        _throwif("undeclared function reference", not Module_IsFunctionDeclared(o->module, func_सूचकांक));
    } अन्यथा {
_       (Module_DeclareFunction(o->module, func_सूचकांक));
    }

#  अगर d_m3HasTypedRefs
    refType = RefTypeOfFuncType(o->module->functions[func_सूचकांक].funcType, true);
#  endif

    // A reference denotes the function that actually runs, so a ref.func naming
    // an import linked to another module is the same reference that module's
    // own ref.func would produce - and carries the defining module with it.
    reference = Function_Implementation(&o->module->functions[func_सूचकांक]);

_   (धकेलो_Const(o, (u64)(uintptr_t)reference, refType));

    _catch: वापस परिणाम;
}


// table.get/set/size/grow/fill all name a table by index; the table struct goes
// into the codestream so the operation doesn't have to walk the module.
अटल
M3_परिणाम ReadTable (IM3Compilation o, M3Table** o_table)
{
    M3_परिणाम परिणाम = m3Err_none;

    u32 index;
_   (ReadLEB_u32(&index, &o->wasm, o->wasmEnd));
    _throwif("table index out of range", index >= o->module->अंक_Tables);

    *o_table = o->module->tables[index];

    _catch: वापस परिणाम;
}


// The table operations take every operand from a slot, so spill the integer
// register first: after this each EmitSlotNumOfStackTopAndPop () emits a real
// slot offset, and the register is free for the result. Operands are emitted
// from the top of the stack downwards, which is the order the ops read them.
अटल
M3_परिणाम Compile_Table_Op (IM3Compilation o, IM3Operation i_op, M3Table* i_table, u32 i_अंक_Operands, m3type_t i_retType)
{
    M3_परिणाम परिणाम = m3Err_none;

_   (PreserveRegisterIfOccupied(o, c_m3Type_i64));

_   (EmitOp(o, i_op));
    EmitPointer(o, i_table);

    क्रम (u32 i = 0; i < i_अंक_Operands; ++i) {
_       (Emit_Slot_अंक_Of_ढेर_शीर्ष_And_निकालो(o));
    }

    अगर (i_retType != c_m3Type_none) {
_       (धकेलो_Register(o, i_retType));
    }

    _catch: वापस परिणाम;
}


M3_परिणाम Compile_Table_GetSet (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_परिणाम परिणाम = m3Err_none;

    M3Table* table;
_   (ReadTable(o, &table));

    अगर (i_opcode == c_waOp_tableGet) {
_       (CheckOperandType(o, 0, Table_AddrType(table)));
_       (Compile_Table_Op(o, op_TableGet, table, 1, table->type));
    } अन्यथा {
_       (CheckOperandType(o, 1, Table_AddrType(table)));
_       (Compile_Table_Op(o, op_TableSet, table, 2, c_m3Type_none));
    }

    _catch: वापस परिणाम;
}


M3_परिणाम Compile_Table_Init (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_परिणाम परिणाम = m3Err_none;

    u32      elem_सूचकांक;
    M3Table* table;

_   (ReadLEB_u32(&elem_सूचकांक, &o->wasm, o->wasmEnd));
    _throwif("element segment index out of range", elem_सूचकांक >= o->module->अंक_Element_Segments);
_   (ReadTable(o, &table));

_   (CheckOperandType(o, 2, Table_AddrType(table)));

_   (PreserveRegisterIfOccupied(o, c_m3Type_i64));
_   (EmitOp(o, op_TableInit));
    EmitPointer(o, table);
    EmitPointer(o, &o->module->elementSegments[elem_सूचकांक]);

    क्रम (u32 i = 0; i < 3; ++i) {
_       (Emit_Slot_अंक_Of_ढेर_शीर्ष_And_निकालो(o));
    }

    _catch: वापस परिणाम;
}


M3_परिणाम Compile_Elem_Drop (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_परिणाम परिणाम = m3Err_none;

    u32 elem_सूचकांक;
_   (ReadLEB_u32(&elem_सूचकांक, &o->wasm, o->wasmEnd));
    _throwif("element segment index out of range", elem_सूचकांक >= o->module->अंक_Element_Segments);

_   (EmitOp(o, op_ElemDrop));
    EmitPointer(o, &o->module->elementSegments[elem_सूचकांक]);

    _catch: वापस परिणाम;
}


M3_परिणाम Compile_Table_Copy (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_परिणाम परिणाम = m3Err_none;

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

    क्रम (u32 i = 0; i < 3; ++i) {
_       (Emit_Slot_अंक_Of_ढेर_शीर्ष_And_निकालो(o));
    }

    _catch: वापस परिणाम;
}


M3_परिणाम Compile_Table_आकार (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_परिणाम परिणाम = m3Err_none;

    M3Table* table;
_   (ReadTable(o, &table));

    // a table size is given in the table's own index type
_   (Compile_Table_Op(o, op_Table_आकार, table, 0, Table_AddrType(table)));

    _catch: वापस परिणाम;
}


M3_परिणाम Compile_Table_GrowFill (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_परिणाम परिणाम = m3Err_none;

    M3Table* table;
_   (ReadTable(o, &table));

    अगर (i_opcode == c_waOp_tableGrow) {
_       (CheckOperandType(o, 0, Table_AddrType(table)));
_       (Compile_Table_Op(o, op_TableGrow, table, 2, Table_AddrType(table)));
    } अन्यथा {
_       (CheckOperandType(o, 0, Table_AddrType(table)));
_       (CheckOperandType(o, 2, Table_AddrType(table)));
_       (Compile_Table_Op(o, op_TableFill, table, 3, c_m3Type_none));
    }

    _catch: वापस परिणाम;
}

#पूर्ण_अगर // d_m3HasRefTypes


M3_परिणाम Compile_तथ्य_Drop (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_परिणाम परिणाम = m3Err_none;

    M3_तथ्य_Segment* segment = शून्य;
_   (Read_तथ्य_Segment(o, &segment));

_   (EmitOp(o, op_तथ्य_Drop));
    EmitPointer(o, segment);

    _catch: वापस परिणाम;
}


अटल
M3_परिणाम ReadBlockType (IM3Compilation o, IM3FuncType* o_blockType)
{
    M3_परिणाम परिणाम = m3Err_none;

#अगर d_m3HasTypedRefs
    // a spelled-out reference type is two bytes, so it cannot be told apart
    // from a type index by the s33 that block types otherwise use
    अगर (o->wasm < o->wasmEnd and (*o->wasm == d_waEncode_ref or *o->wasm == d_waEncode_refNull)) {
        m3type_t refType;
_       (Parse_मूल्य_Type(o->module, &refType, &o->wasm, o->wasmEnd));
        IM3FuncType ftype = शून्य;
_       (AllocFuncType(&ftype, 1));
        ftype->अंक_Rets  = 1;
        ftype->types[0] = refType;
_       (Environment_AddFuncType(o->module->environment, &ftype));
        *o_blockType = ftype;
        वापस परिणाम;
    }
#पूर्ण_अगर

    i64 type;
_   (ReadLebSigned(&type, 33, &o->wasm, o->wasmEnd));

    अगर (type < 0) {
        _throwif(m3Err_invalidTypeId, type < -64);
        u8 मूल्य_Type;
_       (NormalizeType(&मूल्य_Type, (i8)type));                                 m3log (compile, d_indent " (type: %s)", get_indention_string (o), c_waTypes [मूल्य_Type]);
        *o_blockType = o->module->environment->retFuncTypes[मूल्य_Type];
    } अन्यथा {
        _throwif("func type out of bounds", type >= o->module->अंक_Func_Types);
        *o_blockType = o->module->funcTypes[type];                         m3log (compile, d_indent " (type: %s)", get_indention_string (o), SPrintFuncTypeSignature (*o_blockType));
    }
    _catch: वापस परिणाम;
}

अटल
M3_परिणाम PreserveArgsAndLocals (IM3Compilation o)
{
    M3_परिणाम परिणाम = m3Err_none;

    अगर (o->ढेर_सूचकांक > o->ढेर_First_Dynamic_सूचकांक) {
        u32 अंक_Args_And_Locals = Get_Function_अंक_Args_And_Locals(o->function);

        क्रम (u32 i = 0; i < अंक_Args_And_Locals; ++i) {
            u16 slot = Get_Slot_For_ढेर_सूचकांक(o, i);

            u16 preserved_Slot_संख्या;
_           (FindReferencedLocalWithinCurrentBlock(o, &preserved_Slot_संख्या, slot));

            अगर (preserved_Slot_संख्या != slot) {
                m3type_t     type = Get_ढेर_Type_From_Bottom(o, i);                 d_m3Assert (type != c_m3Type_none)
                IM3Operation op   = Is64BitType(type) ? op_CopySlot_64 : op_CopySlot_32;

                EmitOp(o, op);
                EmitSlotOffset(o, preserved_Slot_संख्या);
                EmitSlotOffset(o, slot);
            }
        }
    }

    _catch:
    वापस परिणाम;
}

अटल
M3_परिणाम Compile_LoopOrBlock (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_परिणाम परिणाम;

    IM3FuncType blockType = शून्य;

_   (PreserveRegisters(o));
_   (PreserveArgsAndLocals(o));

_   (ReadBlockType(o, &blockType));

    अगर (i_opcode == c_waOp_loop) {
        u16 अंक_Params = Get_Func_Type_अंक_Params(blockType);
        अगर (अंक_Params) {
            // instantiate constants
            u16 अंक_मूल्यावली = Get_अंक_Block_मूल्यावली_On_ढेर(o);

            अगर (अंक_मूल्यावली >= अंक_Params) {
                u16 ढेर_शीर्ष = Get_ढेर_शीर्ष_सूचकांक(o) + 1;

                क्रम (u16 i = ढेर_शीर्ष - अंक_Params; i < ढेर_शीर्ष; ++i) {
                    u16      slot = Get_Slot_For_ढेर_सूचकांक(o, i);
                    m3type_t type = Get_ढेर_Type_From_Bottom(o, i);

                    अगर (IsConstantSlot(o, slot)) {
                        u16 newSlot = c_slotUnused;
_                       (AllocateSlots(o, &newSlot, type));
_                       (Copy_ढेर_सूचकांक_To_Slot(o, newSlot, i));
                        o->wasm_ढेर[i] = newSlot;
                    }
                }
            }
        }

_       (EmitOp(o, op_Loop));
    } अन्यथा {
    }

_   (CompileBlock(o, blockType, i_opcode));

    _catch: वापस परिणाम;
}

#अगर d_m3HasExceptionHandling

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
अटल
M3_परिणाम EmitCatchStubs (IM3Compilation o)
{
    IM3CodePage displacedPage = शून्य;
_try {
    M3CatchClause* clauses    = o->tryClauses;
    u32            अंक_Clauses = o->अंक_Try_Clauses;

    // the body must not see these: a nested try_table sets up its own
    o->tryClauses    = शून्य;
    o->अंक_Try_Clauses = 0;

    क्रम (u32 i = 0; i < अंक_Clauses; ++i) {
        M3CatchClause* clause = &clauses[i];

        IM3CompilationScope scope;
_       (GetBlockScope(o, &scope, clause->labelDepth + 1));

        IM3FuncType payload = clause->tag ? clause->tag->type : शून्य;
        u16         अंक_Args = Get_Func_Type_अंक_Params(payload);

        // What the stub hands the label - the payload, then the exnref for a
        // _ref clause - has to be what the label takes. ResolveBlockResults
        // only counts the values, and the validator compares base types, so a
        // typed reference is only checked here: (ref null $t) reaching a label
        // that takes (ref $t) has to be rejected.
        {
            bool isLoop         = (scope->opcode == c_waOp_loop);
            u16  अंक_Label_मूल्यावली = isLoop ? Get_Func_Type_अंक_Params(scope->type)
                                         : Get_Func_Type_अंक_Results(scope->type);

            _throwif(m3_Err_type_गिनती_Mismatch, अंक_Label_मूल्यावली != अंक_Args + (clause->hasRef ? 1 : 0));

            क्रम (u16 a = 0; a < अंक_Label_मूल्यावली; ++a) {
                m3type_t given  = (a < अंक_Args) ? GetFuncTypeParamType(payload, a)
                                                : (m3type_t)c_m3Type_exnref;
                m3type_t wanted = isLoop ? GetFuncTypeParamType(scope->type, a)
                                         : Get_Func_Type_परिणाम_Type(scope->type, a);

                _throwif(m3Err_typeMismatch, not IsSubTypeOf(given, wanted));
            }
        }

        IM3CodePage stubPage;
_       (AcquireCompilationCodePage(o, &stubPage));

        pc_t startPC  = GetPagePC(stubPage);
        displacedPage = o->page;
        o->page       = stubPage;

        u16 first_सूचकांक = o->ढेर_सूचकांक;

        क्रम (u16 a = 0; a < अंक_Args; ++a) {
_           (धकेलो_Allocated_Slot(o, GetFuncTypeParamType(payload, a)));
        }

        अगर (clause->hasRef) {
_           (धकेलो_Allocated_Slot(o, c_m3Type_exnref));
        }

        // emitted even with nothing to move: this is also where an exception
        // no clause will reify gets released
_       (Ensure_Code_Page_अंक_पंक्तियाँ(o, 2 * अंक_Args + 3 + d_m3_Code_Page_Free_पंक्तियाँ_Threshold));

_       (EmitOp(o, op_CatchPayload));
        EmitConstant32(o, अंक_Args);

        क्रम (u16 a = 0; a < अंक_Args; ++a) {
            u16 index = first_सूचकांक + a;
            EmitSlotOffset(o, Get_Slot_For_ढेर_सूचकांक(o, index));
            EmitConstant32(o, Is64BitType(Get_ढेर_Type_From_Bottom(o, index)));
        }

        EmitSlotOffset(o, clause->hasRef ? (i32)Get_Slot_For_ढेर_सूचकांक(o, first_सूचकांक + अंक_Args) : -1);

_       (Emit_निकालो_Try_Frames(o, अंक_Try_Frames_To_निकालो(o->block.outer, scope)));

        अगर (scope->opcode == c_waOp_loop) {
_           (ResolveBlockResults(o, scope, /* isBranch: */ true));

_           (EmitOp(o, Op_ContinueLoop(o)));
            EmitPointer(o, scope->pc);
_           (RecordBackEdge(o, scope, false));
        } अन्यथा अगर (scope->depth == 0) {
_           (Return_मूल्यावली(o, scope, /* isBranch: */ true));
_           (EmitOp(o, op_Return));
        } अन्यथा {
_           (ResolveBlockResults(o, scope, /* isBranch: */ true));
_           (EmitPatchingBranch(o, scope));
        }

        IM3CodePage returnTo = displacedPage;
        displacedPage        = शून्य;

        ReleaseCompilationCodePage(o);
        o->page = returnTo;

        *(pc_t*)clause->stubSlot = startPC;

        जबतक (o->ढेर_सूचकांक > first_सूचकांक) {
_           (निकालो(o));
        }
    }

} _catch:

    // thrown with a stub page installed: release it and put back the one it
    // displaced, which the caller's catch then releases
    अगर (displacedPage) {
        ReleaseCompilationCodePage(o);
        o->page = displacedPage;
    }

    वापस परिणाम;
}


अटल
M3_परिणाम Compile_TryTable (IM3Compilation o, m3opcode_t i_opcode)
{
    M3CatchClause* clauses = शून्य;
_try {
    // a catch entry has to be able to assume the operand stack is entirely in
    // slots: an unwind leaves nothing in the registers
_   (PreserveRegisters(o));
_   (PreserveArgsAndLocals(o));

    IM3FuncType blockType = शून्य;
_   (ReadBlockType(o, &blockType));

    u32 अंक_Clauses;
_   (ReadLEB_u32(&अंक_Clauses, &o->wasm, o->wasmEnd));

    _throwif("too many catch clauses", अंक_Clauses > d_m3_Max_Sane_Tags_गिनती);

    अगर (अंक_Clauses) {
        clauses = m3_AllocArray(M3CatchClause, अंक_Clauses);
        _throwifnull(clauses);
    }

    क्रम (u32 i = 0; i < अंक_Clauses; ++i) {
        u8 kind;
_       (Read_u8(&kind, &o->wasm, o->wasmEnd));
        _throwif(m3Err_wasmMalformed, kind > 0x03);

        clauses[i].tag    = शून्य;
        clauses[i].hasRef = (kind == 0x01 or kind == 0x03);

        अगर (kind == 0x00 or kind == 0x01) {
            u32 tag_सूचकांक;
_           (ReadLEB_u32(&tag_सूचकांक, &o->wasm, o->wasmEnd));
            _throwif(m3Err_unknownTag, tag_सूचकांक >= o->module->अंक_Tags);

            IM3Tag tag     = &o->module->tags[tag_सूचकांक];
            clauses[i].tag = tag->resolved ? tag->resolved : tag;
        }

_       (ReadLEB_u32(&clauses[i].labelDepth, &o->wasm, o->wasmEnd));
    }

    // op_TryTable, the clause count, and two words per clause, all of which have
    // to land on one page: the op reads the table by offset from its own pc
_   (Ensure_Code_Page_अंक_पंक्तियाँ(o, 2 * अंक_Clauses + 2 + d_m3_Code_Page_Free_पंक्तियाँ_Threshold));

_   (EmitOp(o, op_TryTable));
    EmitConstant32(o, अंक_Clauses);

    क्रम (u32 i = 0; i < अंक_Clauses; ++i) {
        EmitPointer(o, clauses[i].tag);
        clauses[i].stubSlot = ReservePointer(o);
    }

    o->tryClauses    = clauses;
    o->अंक_Try_Clauses = अंक_Clauses;

_   (CompileBlock(o, blockType, c_waOp_tryTable));

} _catch:

    o->tryClauses    = शून्य;
    o->अंक_Try_Clauses = 0;

    m3_Free(clauses);

    वापस परिणाम;
}


अटल
M3_परिणाम Compile_Throw (IM3Compilation o, m3opcode_t i_opcode)
{
_try {
    u32 tag_सूचकांक;
_   (ReadLEB_u32(&tag_सूचकांक, &o->wasm, o->wasmEnd));
    _throwif(m3Err_unknownTag, tag_सूचकांक >= o->module->अंक_Tags);

    IM3Tag tag = &o->module->tags[tag_सूचकांक];
    अगर (tag->resolved) {
        tag = tag->resolved;
    }

    u16 अंक_Args = Get_Func_Type_अंक_Params(tag->type);

    // the payload is read out of slots, so nothing may still be in a register
_   (PreserveRegisters(o));

    // check the payload before emitting anything, so a mistyped throw does not
    // leave half an operation behind
    bool isPolymorphic = Is_ढेर_Polymorphic(o);

    अगर (not isPolymorphic) {
        _throwif(m3_Err_type_गिनती_Mismatch, Get_अंक_Block_मूल्यावली_On_ढेर(o) < अंक_Args);

        क्रम (u16 i = 0; i < अंक_Args; ++i) {
            u16 index = (u16)(o->ढेर_सूचकांक - अंक_Args + i);

            _throwif(m3Err_typeMismatch, not IsSubTypeOf(Get_ढेर_Type_From_Bottom(o, index),
                                                         GetFuncTypeParamType(tag->type, i)));
        }
    }

_   (Ensure_Code_Page_अंक_पंक्तियाँ(o, 2 * अंक_Args + 3 + d_m3_Code_Page_Free_पंक्तियाँ_Threshold));

_   (EmitOp(o, op_Throw));
    EmitPointer(o, tag);
    EmitConstant32(o, अंक_Args);

    // the payload is named in the order the tag declares it, so the deepest of
    // the arguments on the stack comes first. In unreachable code there is
    // nothing on the stack to name and nothing will run this, so slot 0 stands
    // in for an operand that isn't there.
    क्रम (u16 i = 0; i < अंक_Args; ++i) {
        m3type_t type = GetFuncTypeParamType(tag->type, i);

        EmitSlotOffset(o, isPolymorphic ? 0 : Get_Slot_For_ढेर_सूचकांक(o, (u16)(o->ढेर_सूचकांक - अंक_Args + i)));
        EmitConstant32(o, Is64BitType(type));
    }

    अगर (not isPolymorphic) {
        क्रम (u16 i = 0; i < अंक_Args; ++i) {
_           (निकालो(o));
        }
    }

_   (Set_ढेर_Polymorphic(o));

} _catch: वापस परिणाम;
}


अटल
M3_परिणाम Compile_ThrowRef (IM3Compilation o, m3opcode_t i_opcode)
{
_try {
    अगर (not Is_ढेर_Polymorphic(o)) {
_       (PreserveRegisterIfOccupied(o, c_m3Type_i64));

        _throwif(m3Err_typeMismatch, BaseTypeOf(Get_ढेर_शीर्ष_Type(o)) != c_m3Type_exnref);

_       (EmitOp(o, op_ThrowRef));
        EmitSlotOffset(o, Get_ढेर_शीर्ष_Slot_संख्या(o));

_       (निकालो(o));
    }

_   (Set_ढेर_Polymorphic(o));

} _catch: वापस परिणाम;
}

#पूर्ण_अगर // d_m3HasExceptionHandling

#अगर d_m3_Has_ढेर_Switching

अटल
M3_परिणाम Compile_ContNew (IM3Compilation o, m3opcode_t i_opcode)
{
_try {
    u32 type_सूचकांक;
_   (ReadLEB_u32(&type_सूचकांक, &o->wasm, o->wasmEnd));
    _throwif(m3_Err_type_गिनती_Mismatch, type_सूचकांक >= o->module->अंक_Func_Types);

    IM3FuncType funcType = o->module->funcTypes[type_सूचकांक];
    _throwif("type is not a continuation", not funcType->isContinuation);

    // the operands are read out of slots, so nothing may still be in a register
_   (PreserveRegisters(o));

    // cont.new $ct : [(ref null $ft)] -> [(ref $ct)], where $ct ~~ cont $ft:
    // the function has to be one this continuation type can run
    m3type_t शीर्ष_Type = Get_ढेर_शीर्ष_Type(o);
    _throwif(m3Err_typeMismatch,
             not IsSubTypeOf(शीर्ष_Type, RefTypeOfFuncType(funcType->contFuncType, false)));

    u16 funcSlot = Get_ढेर_शीर्ष_Slot_संख्या(o);
_   (निकालो(o));

    u16 dstSlot;
_   (धकेलो_Allocated_Slot(o, RefTypeOfFuncType(funcType, true)));
    dstSlot = Get_ढेर_शीर्ष_Slot_संख्या(o);

_   (Ensure_Code_Page_अंक_पंक्तियाँ(o, 4 + d_m3_Code_Page_Free_पंक्तियाँ_Threshold));
_   (EmitOp(o, op_ContNew));
    EmitPointer(o, funcType);
    EmitSlotOffset(o, dstSlot);
    EmitSlotOffset(o, funcSlot);

} _catch: वापस परिणाम;
}

अटल
M3_परिणाम Compile_ContBind (IM3Compilation o, m3opcode_t i_opcode)
{
_try {
    u32 typeIndex1, typeIndex2;
_   (ReadLEB_u32(&typeIndex1, &o->wasm, o->wasmEnd));
_   (ReadLEB_u32(&typeIndex2, &o->wasm, o->wasmEnd));
    _throwif(m3_Err_type_गिनती_Mismatch, typeIndex1 >= o->module->अंक_Func_Types);
    _throwif(m3_Err_type_गिनती_Mismatch, typeIndex2 >= o->module->अंक_Func_Types);

    IM3FuncType ct1 = o->module->funcTypes[typeIndex1];
    IM3FuncType ct2 = o->module->funcTypes[typeIndex2];
    _throwif("type is not a continuation", not ct1->isContinuation || not ct2->isContinuation);

    // the operands are read out of slots, so nothing may still be in a register
_   (PreserveRegisters(o));

    m3type_t contType = Get_ढेर_शीर्ष_Type(o);
    _throwif(m3Err_typeMismatch, not IsSubTypeOf(contType, RefTypeOfFuncType(ct1, false)));
    u16 contSlot = Get_ढेर_शीर्ष_Slot_संख्या(o);
_   (निकालो(o));

    IM3FuncType ft1 = ct1->contFuncType ? ct1->contFuncType : ct1;
    IM3FuncType ft2 = ct2->contFuncType ? ct2->contFuncType : ct2;

    u16 अंक_Params1 = Get_Func_Type_अंक_Params(ft1);
    u16 अंक_Params2 = Get_Func_Type_अंक_Params(ft2);
    _throwif(m3Err_typeMismatch, अंक_Params1 < अंक_Params2);
    u32 अंक_Bound = अंक_Params1 - अंक_Params2;

    u16 अंक_Results1 = Get_Func_Type_अंक_Results(ft1);
    u16 अंक_Results2 = Get_Func_Type_अंक_Results(ft2);
    _throwif(m3Err_typeMismatch, अंक_Results1 != अंक_Results2);
    क्रम (u16 i = 0; i < अंक_Results1; ++i) {
        _throwif(m3Err_typeMismatch, Get_Func_Type_परिणाम_Type(ft1, i) != Get_Func_Type_परिणाम_Type(ft2, i));
    }
    क्रम (u16 i = 0; i < अंक_Params2; ++i) {
        _throwif(m3Err_typeMismatch, GetFuncTypeParamType(ft1, अंक_Bound + i) != GetFuncTypeParamType(ft2, i));
    }

    u16 argSlots[d_m3MaxContinuationPayload];
    _throwif("too many bound args", अंक_Bound > d_m3MaxContinuationPayload);

    क्रम (u32 i = 0; i < अंक_Bound; ++i) {
        u32      paramIdx     = अंक_Bound - 1 - i;
        m3type_t expectedType = GetFuncTypeParamType(ft1, paramIdx);
        m3type_t actualType   = Get_ढेर_शीर्ष_Type(o);
        _throwif(m3Err_typeMismatch, not IsSubTypeOf(actualType, expectedType));
        argSlots[paramIdx] = Get_ढेर_शीर्ष_Slot_संख्या(o);
_       (निकालो(o));
    }

    u16 dstSlot;
_   (धकेलो_Allocated_Slot(o, RefTypeOfFuncType(ct2, true)));
    dstSlot = Get_ढेर_शीर्ष_Slot_संख्या(o);

_   (Ensure_Code_Page_अंक_पंक्तियाँ(o, 6 + अंक_Bound * 2 + d_m3_Code_Page_Free_पंक्तियाँ_Threshold));
_   (EmitOp(o, op_ContBind));
    EmitPointer(o, ct1);
    EmitPointer(o, ct2);
    EmitSlotOffset(o, dstSlot);
    EmitSlotOffset(o, contSlot);
    EmitConstant32(o, अंक_Bound);
    क्रम (u32 i = 0; i < अंक_Bound; ++i) {
        EmitSlotOffset(o, argSlots[i]);
        EmitConstant32(o, Is64BitType(GetFuncTypeParamType(ft1, i)));
    }

} _catch: वापस परिणाम;
}

अटल
M3_परिणाम Compile_Suspend (IM3Compilation o, m3opcode_t i_opcode)
{
_try {
    u32 tag_सूचकांक;
_   (ReadLEB_u32(&tag_सूचकांक, &o->wasm, o->wasmEnd));
    _throwif(m3Err_unknownTag, tag_सूचकांक >= o->module->अंक_Tags);

    IM3Tag tag = &o->module->tags[tag_सूचकांक];
    अगर (tag->resolved) {
        tag = tag->resolved;
    }
    u16 अंक_Params  = Get_Func_Type_अंक_Params(tag->type);
    u16 अंक_Results = Get_Func_Type_अंक_Results(tag->type);

_   (PreserveRegisters(o));

    u16 paramSlots[d_m3MaxContinuationPayload];
    _throwif("too many tag params", अंक_Params > d_m3MaxContinuationPayload);
    क्रम (u16 i = 0; i < अंक_Params; ++i) {
        u16      pIdx         = अंक_Params - 1 - i;
        m3type_t expectedType = GetFuncTypeParamType(tag->type, pIdx);
        m3type_t actualType   = Get_ढेर_शीर्ष_Type(o);
        _throwif(m3Err_typeMismatch, not IsSubTypeOf(actualType, expectedType));
        paramSlots[pIdx] = Get_ढेर_शीर्ष_Slot_संख्या(o);
_       (निकालो(o));
    }

    u16 परिणाम_Slots[d_m3MaxContinuationPayload];
    _throwif("too many tag results", अंक_Results > d_m3MaxContinuationPayload);
    क्रम (u16 i = 0; i < अंक_Results; ++i) {
        m3type_t rtype = Get_Func_Type_परिणाम_Type(tag->type, i);
_       (धकेलो_Allocated_Slot(o, rtype));
        परिणाम_Slots[i] = Get_ढेर_शीर्ष_Slot_संख्या(o);
    }

_   (Ensure_Code_Page_अंक_पंक्तियाँ(o, 4 + अंक_Params * 2 + अंक_Results * 2 + d_m3_Code_Page_Free_पंक्तियाँ_Threshold));
_   (EmitOp(o, op_Suspend));
    EmitPointer(o, tag);
    EmitConstant32(o, अंक_Params);
    क्रम (u16 i = 0; i < अंक_Params; ++i) {
        EmitSlotOffset(o, paramSlots[i]);
        EmitConstant32(o, Is64BitType(GetFuncTypeParamType(tag->type, i)));
    }
    EmitConstant32(o, अंक_Results);
    क्रम (u16 i = 0; i < अंक_Results; ++i) {
        EmitSlotOffset(o, परिणाम_Slots[i]);
        EmitConstant32(o, Is64BitType(Get_Func_Type_परिणाम_Type(tag->type, i)));
    }

    // the results are part of the frame here: op_Suspend clears them, and
    // whatever resumes this writes them before anything reads them
_   (RecordSafePoint(o, safepoint_suspend, 0, अंक_Results, 0, शून्य));

} _catch: वापस परिणाम;
}

// resume, resume_throw and resume_throw_ref. All three run a continuation
// under a handler table and take its results; they differ only in what they
// hand it to start with - arguments, a fresh exception, or one off the stack.
//
// Emitted as: the continuation type, the slot holding it, the operands that
// belong to this form, the handler table, then the result slots. Keeping the
// tail identical is what lets one runtime path finish all three.
अटल
M3_परिणाम Compile_Resume (IM3Compilation o, m3opcode_t i_opcode)
{
    M3ResumeHandler* handlers      = शून्य;
    IM3CodePage      displacedPage = शून्य;
_try {
    bool isThrow    = (i_opcode == c_waOp_resumeThrow);
    bool isThrowRef = (i_opcode == c_waOp_resumeThrowRef);

    u32 type_सूचकांक;
_   (ReadLEB_u32(&type_सूचकांक, &o->wasm, o->wasmEnd));
    _throwif(m3_Err_type_गिनती_Mismatch, type_सूचकांक >= o->module->अंक_Func_Types);

    IM3FuncType contFuncType = o->module->funcTypes[type_सूचकांक];
    _throwif("type is not a continuation", not contFuncType->isContinuation);

    IM3FuncType innerType      = contFuncType->contFuncType;
    u16         अंक_Cont_Results = Get_Func_Type_अंक_Results(innerType);

    IM3Tag exnTag      = शून्य;
    u16    अंक_Operands = 0;

    अगर (isThrow) {
        u32 tag_सूचकांक;
_       (ReadLEB_u32(&tag_सूचकांक, &o->wasm, o->wasmEnd));
        _throwif(m3Err_unknownTag, tag_सूचकांक >= o->module->अंक_Tags);

        exnTag = &o->module->tags[tag_सूचकांक];

        // it is an exception that gets raised, so the tag must be one throw
        // could have raised: no results
        _throwif(m3Err_typeMismatch, Get_Func_Type_अंक_Results(exnTag->type) != 0);

        अंक_Operands = Get_Func_Type_अंक_Params(exnTag->type);
    } अन्यथा अगर (not isThrowRef) {
        // a plain resume hands over the continuation's own parameters. An
        // aborted one never receives them, so they are unconstrained there.
        अंक_Operands = Get_Func_Type_अंक_Params(innerType);
    }

    u32 अंक_Clauses;
_   (ReadLEB_u32(&अंक_Clauses, &o->wasm, o->wasmEnd));

    u32 clauseLabelDepths[16];
    _throwif("too many resume clauses", अंक_Clauses > 16);

    अगर (अंक_Clauses) {
        handlers = m3_AllocArray(M3ResumeHandler, अंक_Clauses);
        _throwifnull(handlers);
    }

    क्रम (u32 i = 0; i < अंक_Clauses; ++i) {
        u8 kind;
_       (Read_u8(&kind, &o->wasm, o->wasmEnd));
        _throwif(m3Err_wasmMalformed, kind > 0x01);

        handlers[i].kind = kind;

        u32 tag_सूचकांक;
_       (ReadLEB_u32(&tag_सूचकांक, &o->wasm, o->wasmEnd));
        _throwif(m3Err_unknownTag, tag_सूचकांक >= o->module->अंक_Tags);

        IM3Tag tag         = &o->module->tags[tag_सूचकांक];
        handlers[i].tag    = tag->resolved ? tag->resolved : tag;
        handlers[i].stubPC = शून्य;

        अगर (kind == 0x00) {
_           (ReadLEB_u32(&clauseLabelDepths[i], &o->wasm, o->wasmEnd));
        } अन्यथा {
            // (on $e switch): the tag carries no payload, and what the switch
            // site yields is what this resume yields
            _throwif(m3Err_typeMismatch, Get_Func_Type_अंक_Params(handlers[i].tag->type) != 0);
            clauseLabelDepths[i] = 0;
        }
    }

    // the operands are read out of slots, so nothing may still be in a register
_   (PreserveRegisters(o));

    m3type_t cont_शीर्ष_Type = Get_ढेर_शीर्ष_Type(o);
    _throwif(m3Err_typeMismatch, not IsSubTypeOf(cont_शीर्ष_Type, RefTypeOfFuncType(contFuncType, false)));
    u16 contSlot = Get_ढेर_शीर्ष_Slot_संख्या(o);
_   (निकालो(o));

    u16 exnSlot = 0;

    अगर (isThrowRef) {
        m3type_t exnType = Get_ढेर_शीर्ष_Type(o);
        _throwif(m3Err_typeMismatch, BaseTypeOf(exnType) != c_m3Type_exnref);
        exnSlot = Get_ढेर_शीर्ष_Slot_संख्या(o);
_       (निकालो(o));
    }

    u16 operandSlots[d_m3MaxContinuationPayload];
    _throwif("too many resume operands", अंक_Operands > d_m3MaxContinuationPayload);

    IM3FuncType operandType = isThrow ? exnTag->type : innerType;

    क्रम (u16 i = 0; i < अंक_Operands; ++i) {
        u16      pIdx         = (u16)(अंक_Operands - 1 - i);
        m3type_t expectedType = GetFuncTypeParamType(operandType, pIdx);
        m3type_t actualType   = Get_ढेर_शीर्ष_Type(o);
        _throwif(m3Err_typeMismatch, not IsSubTypeOf(actualType, expectedType));
        operandSlots[pIdx] = Get_ढेर_शीर्ष_Slot_संख्या(o);
_       (निकालो(o));
    }

    क्रम (u32 i = 0; i < अंक_Clauses; ++i) {
        अगर (handlers[i].kind != 0x00) {
            जारी;
        }

        IM3CompilationScope scope;
_       (GetBlockScope(o, &scope, clauseLabelDepths[i]));

        IM3Tag tag        = handlers[i].tag;
        u16    अंक_Payload = Get_Func_Type_अंक_Params(tag->type);

        m3type_t labelContType = (scope->opcode == c_waOp_loop)
                                   ? GetFuncTypeParamType(scope->type, अंक_Payload)
                                   : Get_Func_Type_परिणाम_Type(scope->type, अंक_Payload);

        IM3CodePage stubPage;
_       (AcquireCompilationCodePage(o, &stubPage));
        pc_t startPC  = GetPagePC(stubPage);
        displacedPage = o->page;
        o->page       = stubPage;

        u16 first_सूचकांक = o->ढेर_सूचकांक;
        क्रम (u16 a = 0; a < अंक_Payload; ++a) {
_           (धकेलो_Allocated_Slot(o, GetFuncTypeParamType(tag->type, a)));
        }

        // The handler is handed the suspended continuation.
_       (धकेलो_Allocated_Slot(o, labelContType));

_       (Ensure_Code_Page_अंक_पंक्तियाँ(o, 2 * अंक_Payload + 5 + d_m3_Code_Page_Free_पंक्तियाँ_Threshold));
_       (EmitOp(o, op_ResumePayload));
        EmitPointer(o, Module_ContTypeOfRef(o->module, labelContType));
        EmitConstant32(o, अंक_Payload);
        क्रम (u16 a = 0; a < अंक_Payload; ++a) {
            u16 index = first_सूचकांक + a;
            EmitSlotOffset(o, Get_Slot_For_ढेर_सूचकांक(o, index));
            EmitConstant32(o, Is64BitType(Get_ढेर_Type_From_Bottom(o, index)));
        }
        EmitSlotOffset(o, Get_Slot_For_ढेर_सूचकांक(o, first_सूचकांक + अंक_Payload));

#  अगर d_m3HasExceptionHandling
        // the clause leaves every try region between the resume and its label,
        // and leaving one retires its handlers, the way a branch does
_       (Emit_निकालो_Try_Frames(o, अंक_Try_Frames_To_निकालो(&o->block, scope)));
#  endif

        अगर (scope->opcode == c_waOp_loop) {
_           (ResolveBlockResults(o, scope, true));
_           (EmitOp(o, Op_ContinueLoop(o)));
            EmitPointer(o, scope->pc);
_           (RecordBackEdge(o, scope, false));
        } अन्यथा अगर (scope->depth == 0) {
_           (Return_मूल्यावली(o, scope, true));
_           (EmitOp(o, op_Return));
        } अन्यथा {
_           (ResolveBlockResults(o, scope, true));
_           (EmitPatchingBranch(o, scope));
        }

        IM3CodePage returnTo = displacedPage;
        displacedPage        = शून्य;

        ReleaseCompilationCodePage(o);
        o->page = returnTo;

        handlers[i].stubPC = startPC;

        जबतक (o->ढेर_सूचकांक > first_सूचकांक) {
_           (निकालो(o));
        }
    }

    u16 resSlots[d_m3MaxContinuationPayload];
    _throwif("too many cont results", अंक_Cont_Results > d_m3MaxContinuationPayload);
    क्रम (u16 i = 0; i < अंक_Cont_Results; ++i) {
        m3type_t rtype = Get_Func_Type_परिणाम_Type(innerType, i);
_       (धकेलो_Allocated_Slot(o, rtype));
        resSlots[i] = Get_ढेर_शीर्ष_Slot_संख्या(o);
    }

_   (Ensure_Code_Page_अंक_पंक्तियाँ(o, 8 + अंक_Operands * 2 + अंक_Clauses * 3 + अंक_Cont_Results * 2 +
                                  d_m3_Code_Page_Free_पंक्तियाँ_Threshold));

_   (EmitOp(o, op_Resume));
    EmitPointer(o, contFuncType);
    EmitSlotOffset(o, contSlot);
    EmitConstant32(o, isThrow ? d_m3ResumeThrow : (isThrowRef ? d_m3ResumeThrowRef : d_m3ResumeNormal));

    अगर (isThrowRef) {
        EmitSlotOffset(o, exnSlot);
    } अन्यथा {
        अगर (isThrow) {
            EmitPointer(o, exnTag);
        }

        EmitConstant32(o, अंक_Operands);
        क्रम (u16 i = 0; i < अंक_Operands; ++i) {
            EmitSlotOffset(o, operandSlots[i]);
            EmitConstant32(o, Is64BitType(GetFuncTypeParamType(operandType, i)));
        }
    }

    EmitConstant32(o, अंक_Clauses);
    क्रम (u32 i = 0; i < अंक_Clauses; ++i) {
        EmitConstant32(o, handlers[i].kind);
        EmitPointer(o, handlers[i].tag);
        EmitPointer(o, handlers[i].stubPC);
    }

    EmitConstant32(o, अंक_Cont_Results);
    क्रम (u16 i = 0; i < अंक_Cont_Results; ++i) {
        EmitSlotOffset(o, resSlots[i]);
        EmitConstant32(o, Is64BitType(Get_Func_Type_परिणाम_Type(innerType, i)));
    }

    // a suspend passing through the resumed continuation leaves this frame
    // waiting here, with the continuation's results not yet taken
_   (RecordSafePoint(o, safepoint_resume, 0, अंक_Cont_Results, (u16)अंक_Clauses, शून्य));
#  अगर d_m3HasSnapshots
    अगर (o->snapshotMap and o->page) {
        o->snapshotMap->safePoints[o->snapshotMap->अंक_Safe_Points - 1].resumeType = RefTypeOfFuncType(contFuncType, true);
    }
#  endif

} _catch:
    अगर (displacedPage) {
        ReleaseCompilationCodePage(o);
        o->page = displacedPage;
    }
    m3_Free(handlers);
    वापस परिणाम;
}


अटल
M3_परिणाम Compile_Switch (IM3Compilation o, m3opcode_t i_opcode)
{
_try {
    u32 type_सूचकांक, tag_सूचकांक;
_   (ReadLEB_u32(&type_सूचकांक, &o->wasm, o->wasmEnd));
_   (ReadLEB_u32(&tag_सूचकांक, &o->wasm, o->wasmEnd));
    _throwif(m3_Err_type_गिनती_Mismatch, type_सूचकांक >= o->module->अंक_Func_Types);
    _throwif(m3Err_unknownTag, tag_सूचकांक >= o->module->अंक_Tags);

    IM3FuncType targetContType = o->module->funcTypes[type_सूचकांक];
    IM3Tag      tag            = &o->module->tags[tag_सूचकांक];
    अगर (tag->resolved) {
        tag = tag->resolved;
    }

    _throwif("type is not a continuation", not targetContType->isContinuation);

    // a switch tag carries no payload of its own: the values travel as the
    // peer's arguments, and the tag's results are what the switch site yields
    _throwif(m3Err_typeMismatch, Get_Func_Type_अंक_Params(tag->type) != 0);

    // switch $ct1 : [t1* (ref null $ct1)] -> [t2*]
    //   where $ct1 = cont [t1* (ref null? $ct2)] -> [..]
    //   and   $ct2 = cont [t2*] -> [..]
    //
    // The trailing parameter of $ct1 is not an operand: the peer receives the
    // continuation this instruction suspends. And what this instruction yields
    // is not $ct1's results but $ct2's parameters - the values whoever comes
    // back this way will hand over.
    IM3FuncType targetType = targetContType->contFuncType;
    u16         अंक_Params  = Get_Func_Type_अंक_Params(targetType);

    _throwif("switch target takes no continuation", अंक_Params == 0);

    m3type_t    peerRefType  = GetFuncTypeParamType(targetType, (u16)(अंक_Params - 1));
    IM3FuncType selfContType = Module_ContTypeOfRef(o->module, peerRefType);

    _throwif(m3Err_typeMismatch, not selfContType);

    IM3FuncType selfType   = selfContType->contFuncType;
    u16         अंक_Args    = (u16)(अंक_Params - 1);
    u16         अंक_Results = Get_Func_Type_अंक_Params(selfType);

    // the operands are read out of slots, so nothing may still be in a register
_   (PreserveRegisters(o));

    m3type_t cont_शीर्ष_Type = Get_ढेर_शीर्ष_Type(o);
    _throwif(m3Err_typeMismatch, not IsSubTypeOf(cont_शीर्ष_Type, RefTypeOfFuncType(targetContType, false)));
    u16 targetContSlot = Get_ढेर_शीर्ष_Slot_संख्या(o);
_   (निकालो(o));

    u16 argSlots[d_m3MaxContinuationPayload];
    _throwif("too many switch params", अंक_Args > d_m3MaxContinuationPayload);

    क्रम (u16 i = 0; i < अंक_Args; ++i) {
        u16      pIdx         = (u16)(अंक_Args - 1 - i);
        m3type_t expectedType = GetFuncTypeParamType(targetType, pIdx);
        m3type_t actualType   = Get_ढेर_शीर्ष_Type(o);
        _throwif(m3Err_typeMismatch, not IsSubTypeOf(actualType, expectedType));
        argSlots[pIdx] = Get_ढेर_शीर्ष_Slot_संख्या(o);
_       (निकालो(o));
    }

    u16 परिणाम_Slots[d_m3MaxContinuationPayload];
    _throwif("too many switch results", अंक_Results > d_m3MaxContinuationPayload);

    क्रम (u16 i = 0; i < अंक_Results; ++i) {
_       (धकेलो_Allocated_Slot(o, GetFuncTypeParamType(selfType, i)));
        परिणाम_Slots[i] = Get_ढेर_शीर्ष_Slot_संख्या(o);
    }

_   (Ensure_Code_Page_अंक_पंक्तियाँ(o, 6 + अंक_Args * 2 + अंक_Results * 2 + d_m3_Code_Page_Free_पंक्तियाँ_Threshold));
_   (EmitOp(o, op_Switch));
    EmitPointer(o, targetContType);
    EmitPointer(o, tag);
    EmitSlotOffset(o, targetContSlot);
    EmitConstant32(o, अंक_Args);
    क्रम (u16 i = 0; i < अंक_Args; ++i) {
        EmitSlotOffset(o, argSlots[i]);
        EmitConstant32(o, Is64BitType(GetFuncTypeParamType(targetType, i)));
    }
    EmitConstant32(o, अंक_Results);
    क्रम (u16 i = 0; i < अंक_Results; ++i) {
        EmitSlotOffset(o, परिणाम_Slots[i]);
        EmitConstant32(o, Is64BitType(GetFuncTypeParamType(selfType, i)));
    }

    // as with suspend: op_Switch clears the results before it leaves
_   (RecordSafePoint(o, safepoint_suspend, 0, अंक_Results, 0, शून्य));

} _catch: वापस परिणाम;
}

#पूर्ण_अगर // d_m3HasStackSwitching


अटल
M3_परिणाम CompileElseBlock (IM3Compilation o, pc_t* o_startPC, IM3FuncType i_blockType)
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

    अगर (o->page != savedPage) {
        ReleaseCompilationCodePage(o);
    }

    o->page = savedPage;

    वापस परिणाम;
}

अटल
M3_परिणाम Compile_If (IM3Compilation o, m3opcode_t i_opcode)
{
    /*      [   op_If   ]
            [ <else-pc> ]   ---->   [ ..else..  ]
            [  ..if..   ]           [ ..block.. ]
            [ ..block.. ]           [ op_Branch ]
            [    end    ]  <-----   [  <end-pc> ]       */

_try {

    // Spec: if condition must be i32
    अगर (not Is_ढेर_Polymorphic(o)) {
        m3type_t condType = Get_ढेर_शीर्ष_Type(o);
        _throwif(m3Err_typeMismatch, condType != c_m3Type_none and condType != c_m3Type_i32);
    }

_   (Preserve_Non_शीर्ष_Registers(o));
_   (PreserveArgsAndLocals(o));

#अगर d_m3FuseBranch
    IM3Operation fuseOp = GetBranchFusionOp(o, true);

    अगर (fuseOp) {
        // absorb the if into the compare that produced the condition
        *(IM3Operation*)o->foldPatchPC = fuseOp;
        InvalidateFold(o);

_       (निकालो(o));                          // the condition is consumed inside the fused op
    } अन्यथा
#पूर्ण_अगर
    {
        IM3Operation op = Is_ढेर_शीर्ष_In_Register(o) ? op_If_r : op_If_s;

_       (EmitOp(o, op));
_       (Emit_Slot_अंक_Of_ढेर_शीर्ष_And_निकालो(o));
    }

    pc_t* pc = (pc_t*)ReservePointer(o);

    IM3FuncType blockType = शून्य;
_   (ReadBlockType(o, &blockType));     //  dump_type_stack (o);

    u16 ढेर_सूचकांक = o->ढेर_सूचकांक;

_   (CompileBlock(o, blockType, i_opcode));

    अगर (o->previousOpcode == c_waOp_else) {
        o->ढेर_सूचकांक = ढेर_सूचकांक;
_       (CompileElseBlock(o, pc, blockType));
    } अन्यथा {
        // if block produces values and there isn't a defined else
        // case, then we need to make one up so that the pass-through
        // results end up in the right place
        अगर (Get_Func_Type_अंक_Results(blockType)) {
            // rewind to the if's end to create a fake else block
            o->wasm--;
            o->ढेर_सूचकांक = ढेर_सूचकांक;     // dump_type_stack (o);

_           (CompileElseBlock(o, pc, blockType));
        } अन्यथा {
            *pc = GetPC(o);
        }
    }

    } _catch: वापस परिणाम;
}

अटल
M3_परिणाम Compile_Select (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_परिणाम परिणाम = m3Err_none;

    u16 slots[3] = { c_slotUnused, c_slotUnused, c_slotUnused };

    IM3Operation op = शून्य;

    m3type_t type = Get_ढेर_Type_From_शीर्ष(o, 1); // get type of selection

    अगर (not Is_ढेर_Polymorphic(o)) {
        // Spec: the condition operand (top) must be i32
        m3type_t condType = Get_ढेर_Type_From_शीर्ष(o, 0);
        _throwif(m3Err_typeMismatch, condType != c_m3Type_none and condType != c_m3Type_i32);

        // Spec: the two value operands (below condition) must have matching types
        m3type_t type2 = Get_ढेर_Type_From_शीर्ष(o, 2);
        _throwif(m3Err_typeMismatch, type != c_m3Type_none and type2 != c_m3Type_none and
                                       not (IsSubTypeOf(type, type2) or IsSubTypeOf(type2, type)));
    }

    अगर (IsFpType(type)) {
#अगर d_m3HasFloat
        // not consuming a fp reg, so preserve
        अगर (not Is_ढेर_शीर्ष_Minus1_In_Register(o) and
            not Is_ढेर_शीर्ष_Minus2_In_Register(o)) {
_           (PreserveRegisterIfOccupied(o, type));
        }

        bool selectorInReg = Is_ढेर_शीर्ष_In_Register(o);
        slots[0]           = Get_ढेर_शीर्ष_Slot_संख्या(o);
_       (निकालो(o));

        u32 op_सूचकांक = 0;

        क्रम (u32 i = 1; i <= 2; ++i) {
            अगर (Is_ढेर_शीर्ष_In_Register(o)) {
                op_सूचकांक = i;
            } अन्यथा {
                slots[i] = Get_ढेर_शीर्ष_Slot_संख्या(o);
            }

_           (निकालो(o));
        }

        op = c_fpSelectOps[type - c_m3Type_f32][selectorInReg][op_सूचकांक];
#अन्यथा
        _throw(m3Err_unknownOpcode);
#पूर्ण_अगर
    } अन्यथा अगर (IsIntType(type) or IsRefType(type)) {
        // 'sss' operation doesn't consume a register, so might have to protected its contents
        अगर (not Is_ढेर_शीर्ष_In_Register(o) and
            not Is_ढेर_शीर्ष_Minus1_In_Register(o) and
            not Is_ढेर_शीर्ष_Minus2_In_Register(o)) {
_           (PreserveRegisterIfOccupied(o, type));
        }

        u32 op_सूचकांक = 3;  // op_Select_*_sss

        क्रम (u32 i = 0; i < 3; ++i) {
            अगर (Is_ढेर_शीर्ष_In_Register(o)) {
                op_सूचकांक = i;
            } अन्यथा {
                slots[i] = Get_ढेर_शीर्ष_Slot_संख्या(o);
            }

_           (निकालो(o));
        }

        // a reference is a pointer-sized integer as far as select is concerned
        u32 type_सूचकांक = IsRefType(type) ? ((M3_SIZEOF_PTR == 8) ? 1 : 0)
                                        : (u32)(type - c_m3Type_i32);

        op = c_intSelectOps[type_सूचकांक][op_सूचकांक];
    } अन्यथा अगर (not Is_ढेर_Polymorphic(o)) {
        _throw(m3_Err_function_ढेर_Underrun);
    }

    EmitOp(o, op);
    क्रम (u32 i = 0; i < 3; i++) {
        अगर (IsValidSlot(slots[i])) {
            EmitSlotOffset(o, slots[i]);
        }
    }
_   (धकेलो_Register(o, type));

    _catch: वापस परिणाम;
}

अटल
M3_परिणाम Compile_Drop (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_परिणाम परिणाम = निकालो(o);
    अगर (d_m3_Log_Wasm_ढेर) {
        dump_type_ढेर(o);
    }
    वापस परिणाम;
}

अटल
M3_परिणाम Compile_Nop (IM3Compilation o, m3opcode_t i_opcode)
{
    वापस m3Err_none;
}

अटल
M3_परिणाम Compile_Unreachable (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_परिणाम परिणाम;

_   (AddTrapRecord(o));

_   (EmitOp(o, op_Unreachable));
_   (Set_ढेर_Polymorphic(o));

    _catch:
    वापस परिणाम;
}


// OPTZ: currently all stack slot indices take up a full word, but
// dual stack source operands could be packed together
अटल
M3_परिणाम Compile_Operator (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_परिणाम परिणाम;

    // Declared before the first throw below, which jumps past this point to
    // _catch and so must not skip an initialization.
    IM3Operation op = शून्य;

    IM3OpInfo opInfo = GetOpInfo(i_opcode);
    _throwif(m3Err_unknownOpcode, not opInfo);

    // Spec: validate operand types for load/store operations
    अगर (not Is_ढेर_Polymorphic(o)) {
        // For load ops (stackOffset == 0, unary), the operand is always i32 (address)
        अगर (i_opcode >= 0x28 and i_opcode <= 0x35) {
            m3type_t शीर्ष_Type = Get_ढेर_शीर्ष_Type(o);
            _throwif(m3Err_typeMismatch, शीर्ष_Type != c_m3Type_none and शीर्ष_Type != c_m3Type_i32);
        }

        // For store ops (stackOffset == -2), the address operand must be i32
        अगर (i_opcode >= 0x36 and i_opcode <= 0x3e) {
            m3type_t addrType = Get_ढेर_Type_From_शीर्ष(o, 1);
            _throwif(m3Err_typeMismatch, addrType != c_m3Type_none and addrType != c_m3Type_i32);
        }
    }

    // This preserve is for for FP compare operations.
    // either need additional slot destination operations or the
    // easy fix, move _r0 out of the way.
    // moving out the way might be the optimal solution most often?
    // otherwise, the _r0 reg can get buried down in the stack
    // and be idle & wasted for a moment.
    अगर (M3_UNLIKELY(IsFpType(Get_ढेर_शीर्ष_Type(o)) and IsIntType(opInfo->type))) {
_       (PreserveRegisterIfOccupied(o, opInfo->type));
    }

    अगर (opInfo->ढेर_Offset == 0) {
        अगर (Is_ढेर_शीर्ष_In_Register(o)) {
            op = opInfo->operations[0]; // _s
        } अन्यथा {
_           (PreserveRegisterIfOccupied(o, opInfo->type));
            op = opInfo->operations[1]; // _r
        }
    } अन्यथा {
        अगर (Is_ढेर_शीर्ष_In_Register(o)) {
            op = opInfo->operations[0];  // _rs

            अगर (Is_ढेर_शीर्ष_Minus1_In_Register(o)) {    d_m3Assert (i_opcode == c_waOp_store_f32 or i_opcode == c_waOp_store_f64);
                op = opInfo->operations[3]; // _rr for fp.store
            }
        } अन्यथा अगर (Is_ढेर_शीर्ष_Minus1_In_Register(o)) {
            op = opInfo->operations[1]; // _sr

            अगर (not op) {  // must be commutative, then
                op = opInfo->operations[0];
            }
        } अन्यथा {
_           (PreserveRegisterIfOccupied(o, opInfo->type));     // _ss
            op = opInfo->operations[2];
        }
    }

    अगर (M3_LIKELY(op)) {
_       (EmitOp(o, op));

#अगर d_m3FoldSetLocal
        // the op word just written (EmitOp may have bridged pages); NULL in the page-less compile-walk mode
        pc_t patchPC = o->page ? GetPC(o) - 1 : शून्य;
#पूर्ण_अगर

_       (Emit_Slot_अंक_Of_ढेर_शीर्ष_And_निकालो(o));

        अगर (opInfo->ढेर_Offset < 0) {
_           (Emit_Slot_अंक_Of_ढेर_शीर्ष_And_निकालो(o));
        }

        अगर (opInfo->type != c_m3Type_none) {
_           (धकेलो_Register(o, opInfo->type));

#अगर d_m3FoldSetLocal
            // record this op as a fold candidate for an immediately following local.set;
            // the operand-form index is recovered by matching against the variant table
            अगर (patchPC) {
                क्रम (u8 form = 0; form < 4; ++form) {
                    अगर (opInfo->operations[form] == op) {
                        o->foldPatchPC    = patchPC;
                        o->foldOpcode     = i_opcode;
                        o->foldForm       = form;
                        o->fold_ढेर_सूचकांक = (u16)Get_ढेर_शीर्ष_सूचकांक(o);
                        अवरोध;
                    }
                }
            }
#पूर्ण_अगर
        }
    } अन्यथा {
#अगर_घोषित DEBUG
        परिणाम = ErrorCompile("no operation found for opcode", o, "'%s'", opInfo->name);
#अन्यथा
        परिणाम = ErrorCompile("no operation found for opcode", o, "%x", i_opcode);
#पूर्ण_अगर
        _throw(परिणाम);
    }

    _catch: वापस परिणाम;
}

अटल
M3_परिणाम Compile_Convert (IM3Compilation o, m3opcode_t i_opcode)
{
_try {
    IM3OpInfo opInfo = GetOpInfo(i_opcode);
    _throwif(m3Err_unknownOpcode, not opInfo);

    // Spec: validate source operand type for conversion instructions
    अगर (not Is_ढेर_Polymorphic(o)) {
        u8 sourceType = c_m3Type_none;
        चयन (i_opcode)
        // clang-format off
        {
        हाल 0xa7:              // i32.wrap/i64
            sourceType = c_m3Type_i64; अवरोध;
        हाल 0xa8: हाल 0xa9:   // i32.trunc_s/f32, i32.trunc_u/f32
        हाल 0xae: हाल 0xaf:   // i64.trunc_s/f32, i64.trunc_u/f32
        हाल 0xbb:              // f64.promote/f32
        हाल 0xbc:              // i32.reinterpret/f32
            sourceType = c_m3Type_f32; अवरोध;
        हाल 0xaa: हाल 0xab:   // i32.trunc_s/f64, i32.trunc_u/f64
        हाल 0xb0: हाल 0xb1:   // i64.trunc_s/f64, i64.trunc_u/f64
        हाल 0xb6:              // f32.demote/f64
        हाल 0xbd:              // i64.reinterpret/f64
            sourceType = c_m3Type_f64; अवरोध;
        हाल 0xac: हाल 0xad:   // i64.extend_s/i32, i64.extend_u/i32
        हाल 0xb2: हाल 0xb3:   // f32.convert_s/i32, f32.convert_u/i32
        हाल 0xb7: हाल 0xb8:   // f64.convert_s/i32, f64.convert_u/i32
        हाल 0xbe:              // f32.reinterpret/i32
            sourceType = c_m3Type_i32; अवरोध;
        हाल 0xb4: हाल 0xb5:   // f32.convert_s/i64, f32.convert_u/i64
        हाल 0xb9: हाल 0xba:   // f64.convert_s/i64, f64.convert_u/i64
        हाल 0xbf:              // f64.reinterpret/i64
            sourceType = c_m3Type_i64; अवरोध;
        शेष: अवरोध;
        }
        // clang-format on

        अगर (sourceType != c_m3Type_none) {
            m3type_t शीर्ष_Type = Get_ढेर_शीर्ष_Type(o);
            _throwif(m3Err_typeMismatch, शीर्ष_Type != c_m3Type_none and not IsSubTypeOf(शीर्ष_Type, sourceType));
        }
    }

    bool destInSlot   = IsRegisterTypeAllocated(o, opInfo->type);
    bool sourceInSlot = Is_ढेर_शीर्ष_In_Slot(o);

    IM3Operation op = opInfo->operations[destInSlot * 2 + sourceInSlot];

_   (EmitOp(o, op));
_   (Emit_Slot_अंक_Of_ढेर_शीर्ष_And_निकालो(o));

    अगर (destInSlot) {
_       (धकेलो_Allocated_Slot_And_Emit_2(o, opInfo->type))
    } अन्यथा {
_       (धकेलो_Register(o, opInfo->type))
    }
}
    _catch: वापस परिणाम;
}

#अगर d_m3HasWideArithmetic

// The wide arithmetic proposal: i64.add128 and i64.sub128 over two 128-bit values
// given as i64 halves, and i64.mul_wide_s/u giving the full product of two i64s.
// Each gives back a 128-bit result the same way, low half first. The operands are
// all read from slots, and of the two results the high half, which ends up on top,
// goes to _r0 and the low half to a newly allocated slot - so every operation has
// the one form, whatever shape the stack was in.
अटल
M3_परिणाम Compile_WideArithmetic (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_परिणाम परिणाम = m3Err_none;

    IM3Operation op          = op_i64_Add128;
    u32          अंक_Operands = 4;

    चयन (i_opcode)
    // clang-format off
    {
    हाल c_waOp_i64Sub128:   op = op_i64_Sub128;                          अवरोध;
    हाल c_waOp_i64MulWideS: op = op_i64_MultiplyWide; अंक_Operands = 2;   अवरोध;
    हाल c_waOp_i64MulWideU: op = op_u64_MultiplyWide; अंक_Operands = 2;   अवरोध;
    शेष: अवरोध;
    }
    // clang-format on

_   (PreserveRegisterIfOccupied(o, c_m3Type_i64));

_   (EmitOp(o, op));

    // the top of the stack first: the high half of the second operand, for the
    // 128-bit ones
    क्रम (u32 i = 0; i < अंक_Operands; ++i) {
_       (Emit_Slot_अंक_Of_ढेर_शीर्ष_And_निकालो(o));
    }

_   (धकेलो_Allocated_Slot_And_Emit_2(o, c_m3Type_i64));      // the low half
_   (धकेलो_Register(o, c_m3Type_i64));                  // the high half

    _catch: वापस परिणाम;
}

#पूर्ण_अगर // d_m3HasWideArithmetic

#अगर d_m3HasMemory64

// Replaces the 64-bit address operand of a load or store with the checked
// 32-bit effective address op_CheckAddr64 leaves behind, so that the access
// itself compiles exactly as it would against a 32-bit memory: one slot to read
// the address from, and an offset of zero folded in already.
//
// Where the address sits under other operands - the value of a store, the
// operands of an atomic - it is rewritten where it stands rather than popped:
// its stack entry is repointed at the scratch slot and retyped, and the slot it
// used to occupy released. i_numOperands counts the address and all above it.
अटल
M3_परिणाम EmitCheckAddr64 (IM3Compilation o, u64 i_offset, u16 i_अंक_Operands)
{
    M3_परिणाम परिणाम = m3Err_none;

    // The address has to be in a slot for op_CheckAddr64 to read it. Spilling
    // r0 wholesale also keeps the value of a store out of it, which is what
    // leaves the address as the one operand still in a register to worry about.
_   (PreserveRegisterIfOccupied(o, c_m3Type_i64));

    // unreachable code: the operands the instruction names may not be there
    अगर (o->ढेर_सूचकांक < o->block.block_ढेर_सूचकांक + i_अंक_Operands) {
        वापस परिणाम;
    }

    {
        u16      ढेर_सूचकांक = (u16)(o->ढेर_सूचकांक - i_अंक_Operands);
        u16      srcSlot    = o->wasm_ढेर[ढेर_सूचकांक];
        m3type_t srcType    = o->type_ढेर[ढेर_सूचकांक];

        u16 dstSlot = c_slotUnused;

        // Checked here rather than left to the validator, which a build may
        // have switched off: reading a slot as an i64 that only holds an i32
        // would reach past what was allocated for it.
        _throwif(m3Err_typeMismatch, BaseTypeOf(srcType) != c_m3Type_i64);

        // an address is an integer, so nothing should be left in a register
        // once r0 has been preserved
        _throwif(m3Err_typeMismatch, IsRegisterSlotAlias(srcSlot));
        _throwif(m3_Err_function_ढेर_Underrun, srcSlot >= o->slot_Max_Allocated_सूचकांक_Plus_One);

        // allocated before the source is released, so the two cannot be the
        // same slot - op_CheckAddr64 reads one and writes the other
_       (AllocateSlots(o, &dstSlot, c_m3Type_i32));

_       (EmitOp(o, op_CheckAddr64));
        EmitSlotOffset(o, srcSlot);
        EmitSlotOffset(o, dstSlot);
        EmitConstant64(o, i_offset);

        o->wasm_ढेर[ढेर_सूचकांक] = dstSlot;
        o->type_ढेर[ढेर_सूचकांक] = c_m3Type_i32;

        // locals and the constant table outlive the instruction; a dynamic
        // slot the address was computed into does not
        अगर (srcSlot >= o->slot_First_Dynamic_सूचकांक) {
            DeallocateSlot(o, srcSlot, srcType);
        }
    }

    _catch: वापस परिणाम;
}

#पूर्ण_अगर // d_m3HasMemory64


अटल
M3_परिणाम Compile_Load_Store (IM3Compilation o, m3opcode_t i_opcode)
{
_try {
    u32 alignHint, memoryIdx;
    u64 memoryOffset;

    // alignHint is checked by the validator
_   (ReadMemoryArg(&alignHint, &memoryIdx, &memoryOffset, &o->wasm, o->wasmEnd));
                                                                        m3log (compile, d_indent " (memory = %d; offset = %llu)", get_indention_string (o), memoryIdx, (अचिन्हित दीर्घ दीर्घ) memoryOffset);
    _throwif(m3Err_unknownMemory, memoryIdx >= o->module->अंक_Memories);

    IM3OpInfo opInfo = GetOpInfo(i_opcode);
    _throwif(m3Err_unknownOpcode, not opInfo);

    bool isMemory64 = o->module->memories[memoryIdx]->isMemory64;

    // Spec: the static offset has to be in range of the address type. Checked
    // here as well as in the validator, so that a build without one still
    // refuses the module rather than silently truncating the offset.
    _throwif(m3Err_wasmMalformed, not isMemory64 and memoryOffset > 0xFFFFFFFFull);

    अगर (IsFpType(opInfo->type)) {
_       (PreserveRegisterIfOccupied(o, c_m3Type_f64));
    }

    अगर (memoryIdx) {
_       (EmitSetMemory(o, memoryIdx));
    }

#अगर d_m3HasMemory64
    अगर (isMemory64) {
        // An offset this far out puts every address out of bounds, so rather
        // than carrying it, clamp it to the limit: op_CheckAddr64 then traps
        // on whatever address it is given, which is the outcome either way.
        अगर (memoryOffset > d_m3AddressLimit) {
            memoryOffset = d_m3AddressLimit;
        }

_       (EmitCheckAddr64(o, memoryOffset, opInfo->ढेर_Offset < 0 ? 2 : 1));

        // op_CheckAddr64 has folded the offset in already
        memoryOffset = 0;
    }
#पूर्ण_अगर

_   (Compile_Operator(o, i_opcode));

    EmitConstant32(o, (u32)memoryOffset);

    अगर (memoryIdx) {
_       (EmitSetMemory(o, 0));
    }
}
    _catch: वापस परिणाम;
}


#अगर d_m3HasAtomics

// What an atomic instruction takes and gives back, from its sub-opcode. The
// accesses come in runs of seven, one per combination of value type and cell width:
// i32/32, i64/64, i32/8, i32/16, i64/8, i64/16, i64/32. Those runs are told apart by
// where they start.
//
// The operand count includes the address. The top operand is the one that ends up
// in a register, so its type is the one to check on the way out; a store has no
// result, which is c_m3Type_none.
अटल
व्योम GetAtomicShape (u32 i_sub, u16* o_अंक_Operands, m3type_t* o_शीर्ष_Type, m3type_t* o_परिणाम_Type)
{
    // clang-format off
    अटल स्थिर m3type_t c_मूल्य_Types [7] = {
        c_m3Type_i32, c_m3Type_i64,
        c_m3Type_i32, c_m3Type_i32,
        c_m3Type_i64, c_m3Type_i64, c_m3Type_i64
    };
    // clang-format on

    अगर (i_sub == 0x00) {                    // memory.atomic.notify: address, count
        *o_अंक_Operands = 2;
        *o_शीर्ष_Type     = c_m3Type_i32;
        *o_परिणाम_Type  = c_m3Type_i32;
    } अन्यथा अगर (i_sub <= 0x02) {             // memory.atomic.wait32/64: address, expected, timeout
        *o_अंक_Operands = 3;
        *o_शीर्ष_Type     = c_m3Type_i64;
        *o_परिणाम_Type  = c_m3Type_i32;
    } अन्यथा अगर (i_sub <= 0x16) {             // loads: address
        *o_अंक_Operands = 1;
        *o_शीर्ष_Type     = c_m3Type_i32;
        *o_परिणाम_Type  = c_मूल्य_Types[i_sub - 0x10];
    } अन्यथा अगर (i_sub <= 0x1d) {             // stores: address, value
        *o_अंक_Operands = 2;
        *o_शीर्ष_Type     = c_मूल्य_Types[i_sub - 0x17];
        *o_परिणाम_Type  = c_m3Type_none;
    } अन्यथा अगर (i_sub <= 0x47) {             // read-modify-write: address, value
        *o_अंक_Operands = 2;
        *o_शीर्ष_Type     = c_मूल्य_Types[(i_sub - 0x1e) % 7];
        *o_परिणाम_Type  = *o_शीर्ष_Type;
    } अन्यथा {                                // cmpxchg: address, expected, replacement
        *o_अंक_Operands = 3;
        *o_शीर्ष_Type     = c_मूल्य_Types[i_sub - 0x48];
        *o_परिणाम_Type  = *o_शीर्ष_Type;
    }
}


// The description an access takes as its last immediate: see m3_exec_atomic.h
अटल
u32 GetAtomicDesc (u32 i_sub)
{
    // per variant, the log2 of the width of the cell
    अटल स्थिर u8 c_log2Widths[7] = { 2, 3, 0, 1, 0, 1, 2 };

    u32 variant, kind = 0;

    अगर (i_sub <= 0x16) {
        variant = i_sub - 0x10;
    } अन्यथा अगर (i_sub <= 0x1d) {
        variant = i_sub - 0x17;
    } अन्यथा अगर (i_sub <= 0x47) {
        variant = (i_sub - 0x1e) % 7;
        kind    = (i_sub - 0x1e) / 7;         // add, sub, and, or, xor, xchg
    } अन्यथा {
        variant = i_sub - 0x48;
    }

    // variants 1 and 4..6 are the i64 ones
    u32 is64 = (variant == 1 or variant >= 4) ? 1u : 0u;

    वापस c_log2Widths[variant] | (is64 << 2) | (kind << 3);
}


// Every atomic access is one op, whose operands are the topmost in _r0 and the rest
// in slots, topmost first. The address is always the deepest, so it is the last slot
// (or, for a load, the register), followed by the memarg offset. The ops check the
// address themselves: for an atomic that includes its alignment, which only the
// effective address can be asked.
अटल
M3_परिणाम Compile_Atomic (IM3Compilation o, m3opcode_t i_opcode)
{
_try {
    u32 alignHint, memoryIdx;
    u64 memoryOffset;

    u16      अंक_Operands = 0;
    m3type_t शीर्ष_Type     = c_m3Type_none;
    m3type_t परिणाम_Type  = c_m3Type_none;

    अगर (i_opcode == c_waOp_atomicFence) {
        // the reserved byte, which the validator has checked to be zero. There
        // is nothing to order between accesses that one thread makes in sequence.
        u8 reserved;
_       (Read_u8(&reserved, &o->wasm, o->wasmEnd));
    } अन्यथा {
        // alignHint is checked by the validator
_       (ReadMemoryArg(&alignHint, &memoryIdx, &memoryOffset, &o->wasm, o->wasmEnd));
                                                                        m3log (compile, d_indent " (memory = %d; offset = %llu)", get_indention_string (o), memoryIdx, (अचिन्हित दीर्घ दीर्घ) memoryOffset);
        _throwif(m3Err_unknownMemory, memoryIdx >= o->module->अंक_Memories);

        IM3OpInfo opInfo = GetOpInfo(i_opcode);
        _throwif(m3Err_unknownOpcode, not opInfo);

        bool isMemory64 = o->module->memories[memoryIdx]->isMemory64;

        // see Compile_Load_Store
        _throwif(m3Err_wasmMalformed, not isMemory64 and memoryOffset > 0xFFFFFFFFull);

        GetAtomicShape(i_opcode & 0xFF, &अंक_Operands, &शीर्ष_Type, &परिणाम_Type);

        अगर (memoryIdx) {
_           (EmitSetMemory(o, memoryIdx));
        }

#  अगर d_m3HasMemory64
        अगर (isMemory64) {
            // see Compile_Load_Store
            अगर (memoryOffset > d_m3AddressLimit) {
                memoryOffset = d_m3AddressLimit;
            }

_           (EmitCheckAddr64(o, memoryOffset, अंक_Operands));
            memoryOffset = 0;
        }
#  endif

_       (Copy_ढेर_शीर्ष_To_Register(o, false));
_       (EmitOp(o, opInfo->operations[0]));
_       (निकालो_Type(o, शीर्ष_Type));

        क्रम (u16 i = 1; i < अंक_Operands; ++i) {
_           (Emit_Slot_अंक_Of_ढेर_शीर्ष_And_निकालो(o));
        }

        EmitConstant32(o, (u32)memoryOffset);
        अगर (i_opcode >= c_waOp_atomicFirstAccess) {
            EmitConstant32(o, GetAtomicDesc(i_opcode & 0xFF));
        }

        अगर (memoryIdx) {
_           (EmitSetMemory(o, 0));
        }

        अगर (परिणाम_Type != c_m3Type_none) {
_           (धकेलो_Register(o, परिणाम_Type));
        }
    }
}
    _catch: वापस परिणाम;
}

#पूर्ण_अगर // d_m3HasAtomics


M3_परिणाम CompileRawFunction (IM3Module io_module, IM3Function io_function, स्थिर व्योम* i_function, स्थिर व्योम* i_userdata)
{
    d_m3Assert(io_module->runtime);

    IM3CodePage page = AcquireCodePageWithCapacity(io_module->runtime, 4);

    अगर (page) {
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
        वापस m3Err_none;
    } अन्यथा {
        वापस m3Err_mallocFailedCodePage;
    }
}


// A table entry names every field it sets, so the struct can order them for
// packing; the columns follow that order, since M3OP names them positionally.
//
// d_logOp, d_logOp2 macros aren't actually used by the compiler, just codepage decoding (d_m3LogCodePages = 1)
#घोषणा d_ops(...)                          .operations = { __VA_ARGS__ }

#घोषणा d_logOp(OP)                         d_ops( op_##OP,                 शून्य,                       शून्य,                       शून्य )
#घोषणा d_logOp2(OP1,OP2)                   d_ops( op_##OP1,                op_##OP2,                   शून्य,                       शून्य )

#घोषणा d_emptyOpList                       d_ops( शून्य,                    शून्य,                       शून्य,                       शून्य )
#घोषणा d_unaryOpList(TYPE, NAME)           d_ops( op_##TYPE##_##NAME##_r,  op_##TYPE##_##NAME##_s,     शून्य,                       शून्य )
#घोषणा d_binOpList(TYPE, NAME)             d_ops( op_##TYPE##_##NAME##_rs, op_##TYPE##_##NAME##_sr,    op_##TYPE##_##NAME##_ss,    शून्य )
#घोषणा d_storeFpOpList(TYPE, NAME)         d_ops( op_##TYPE##_##NAME##_rs, op_##TYPE##_##NAME##_sr,    op_##TYPE##_##NAME##_ss,    op_##TYPE##_##NAME##_rr )
#घोषणा d_commutativeBinOpList(TYPE, NAME)  d_ops( op_##TYPE##_##NAME##_rs, शून्य,                       op_##TYPE##_##NAME##_ss,    शून्य )

#घोषणा d_convertOpList(OP)                 d_ops( op_##OP##_r_r,           op_##OP##_r_s,              op_##OP##_s_r,              op_##OP##_s_s )


// Every M3Compiler the tables below name, as one list, so that the index enum and
// the array of pointers can never drift apart. Each entry must carry the same
// config guard as the function's definition.
#घोषणा d_m3CompilerList(_)             \
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
    _( Compile_Memory_आकार )            \
    _( Compile_Memory_Grow )            \
    _( Compile_Memory_Init )            \
    _( Compile_Memory_CopyFill )        \
    _( Compile_तथ्य_Drop )              \
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
    d_m3_Compiler_List_ढेर_Switching( _ )

#अगर d_m3ImplementFloat
#  define d_m3CompilerList_f(_)         \
    _( Compile_Const_f32 )              \
    _( Compile_Const_f64 )
#अन्यथा
#  define d_m3CompilerList_f(_)
#पूर्ण_अगर

#अगर d_m3HasRefTypes
#  define d_m3CompilerList_refTypes(_)  \
    _( Compile_Select_Typed )           \
    _( Compile_Ref_Null )               \
    _( Compile_Ref_IsNull )             \
    _( Compile_Ref_Func )               \
    _( Compile_Table_GetSet )           \
    _( Compile_Table_Init )             \
    _( Compile_Table_Copy )             \
    _( Compile_Table_आकार )             \
    _( Compile_Table_GrowFill )         \
    _( Compile_Elem_Drop )
#अन्यथा
#  define d_m3CompilerList_refTypes(_)
#पूर्ण_अगर

#अगर d_m3HasTypedRefs
#  define d_m3CompilerList_typedRefs(_) \
    _( Compile_CallRef )                \
    _( Compile_Ref_AsNonNull )
#अन्यथा
#  define d_m3CompilerList_typedRefs(_)
#पूर्ण_अगर

#अगर d_m3HasExceptionHandling
#  define d_m3CompilerList_eh(_)        \
    _( Compile_Throw )                  \
    _( Compile_ThrowRef )               \
    _( Compile_TryTable )
#अन्यथा
#  define d_m3CompilerList_eh(_)
#पूर्ण_अगर

#अगर d_m3HasAtomics
#  define d_m3CompilerList_atomics(_)   \
    _( Compile_Atomic )
#अन्यथा
#  define d_m3CompilerList_atomics(_)
#पूर्ण_अगर

#अगर d_m3HasWideArithmetic
#  define d_m3CompilerList_wide(_)      \
    _( Compile_WideArithmetic )
#अन्यथा
#  define d_m3CompilerList_wide(_)
#पूर्ण_अगर

#अगर d_m3_Has_ढेर_Switching
#  define d_m3_Compiler_List_ढेर_Switching(_) \
    _( Compile_ContNew )                     \
    _( Compile_ContBind )                    \
    _( Compile_Suspend )                     \
    _( Compile_Resume )                      \
    _( Compile_Switch )
#अन्यथा
#  define d_m3_Compiler_List_ढेर_Switching(_)
#पूर्ण_अगर

// Index 0 is "this opcode has no compiler", so that a zeroed entry - which is
// what M3OP_RESERVED and the reserved holes in the tables are - reads as one.
क्रमागत {
    c_cc_NULL = 0,
#घोषणा d_m3CompilerEnum(NAME)  c_cc_##NAME,
    d_m3CompilerList(d_m3CompilerEnum)
#अघोषित d_m3CompilerEnum
};

अटल स्थिर M3Compiler c_compilers[] = {
    शून्य,
#घोषणा d_m3CompilerPtr(NAME)   NAME,
    d_m3CompilerList(d_m3CompilerPtr)
#अघोषित d_m3CompilerPtr
};

// Names a table entry's compiler by index. d_cc (NULL) is the no-compiler entry.
#घोषणा d_cc(NAME)                          .compiler = c_cc_##NAME

अटल
M3Compiler GetCompiler (IM3OpInfo i_opInfo)
{
    d_m3Assert(i_opInfo->compiler < M3_गिनती_OF(c_compilers));
    वापस c_compilers[i_opInfo->compiler];
}

// clang-format off
स्थिर M3OpInfo c_operations[] =
{
    M3OP( "unreachable",           0,  none,   d_cc(Compile_Unreachable),  d_logOp (Unreachable) ),                   // 0x00
    M3OP( "nop",                   0,  none,   d_cc(Compile_Nop),          d_emptyOpList ),                           // 0x01 .
    M3OP( "block",                 0,  none,   d_cc(Compile_LoopOrBlock),  d_emptyOpList ),                           // 0x02
    M3OP( "loop",                  0,  none,   d_cc(Compile_LoopOrBlock),  d_logOp (Loop) ),                          // 0x03
    M3OP( "if",                    -1, none,   d_cc(Compile_If),           d_emptyOpList ),                           // 0x04
    M3OP( "else",                  0,  none,   d_cc(Compile_Nop),          d_emptyOpList ),                           // 0x05

    M3OP_RESERVED,  M3OP_RESERVED,                                                                                    // 0x06...0x07

#अगर d_m3HasExceptionHandling
    M3OP( "throw",                 0,  none,   d_cc(Compile_Throw),        d_logOp (Throw) ),                         // 0x08
    M3OP_RESERVED,                                                                                                    // 0x09
    M3OP( "throw_ref",             0,  none,   d_cc(Compile_ThrowRef),     d_logOp (ThrowRef) ),                      // 0x0a
#अन्यथा
    M3OP_RESERVED,  M3OP_RESERVED, M3OP_RESERVED,                                                                     // 0x08...0x0a
#पूर्ण_अगर

    M3OP( "end",                   0,  none,   d_cc(Compile_End),          d_emptyOpList ),                           // 0x0b
    M3OP( "br",                    0,  none,   d_cc(Compile_Branch),       d_logOp (Branch) ),                        // 0x0c
    M3OP( "br_if",                 -1, none,   d_cc(Compile_Branch),       d_logOp2 (BranchIf_r, BranchIf_s) ),       // 0x0d
    M3OP( "br_table",              -1, none,   d_cc(Compile_BranchTable),  d_logOp (BranchTable) ),                   // 0x0e
    M3OP( "return",                0,  any,    d_cc(Compile_Return),       d_logOp (Return) ),                        // 0x0f
    M3OP( "call",                  0,  any,    d_cc(Compile_Call),         d_logOp (Call) ),                          // 0x10
    M3OP( "call_indirect",         0,  any,    d_cc(Compile_CallIndirect), d_logOp (CallIndirect) ),                  // 0x11
    M3OP( "return_call",           0,  any,    d_cc(Compile_Call),         d_logOp (ReturnCall) ),                    // 0x12
    M3OP( "return_call_indirect",  0,  any,    d_cc(Compile_CallIndirect), d_logOp (ReturnCallIndirect) ),            // 0x13

#अगर d_m3HasTypedRefs
    M3OP( "call_ref",              0,  any,    d_cc(Compile_CallRef),      d_logOp (CallRef) ),                       // 0x14
    M3OP( "return_call_ref",       0,  any,    d_cc(Compile_CallRef),      d_logOp (ReturnCallRef) ),                 // 0x15
#अन्यथा
    M3OP_RESERVED,  M3OP_RESERVED,                                                                                    // 0x14...
#पूर्ण_अगर
    M3OP_RESERVED,  M3OP_RESERVED, M3OP_RESERVED, M3OP_RESERVED,                                                      // ...0x19

    M3OP( "drop",                  -1, none,   d_cc(Compile_Drop),         d_emptyOpList ),                           // 0x1a
    M3OP( "select",                -2, any,    d_cc(Compile_Select),       d_emptyOpList ),                           // 0x1b

#अगर d_m3HasRefTypes
    M3OP( "select.t",              -2, any,    d_cc(Compile_Select_Typed), d_emptyOpList ),                           // 0x1c
#अन्यथा
    M3OP_RESERVED,                                                                                                    // 0x1c
#पूर्ण_अगर
    M3OP_RESERVED,  M3OP_RESERVED,                                                                                    // 0x1d...0x1e

#अगर d_m3HasExceptionHandling
    M3OP( "try_table",             0,  none,   d_cc(Compile_TryTable),     d_logOp (TryTable) ),                      // 0x1f
#अन्यथा
    M3OP_RESERVED,                                                                                                    // 0x1f
#पूर्ण_अगर

    M3OP( "local.get",             1,  any,    d_cc(Compile_GetLocal),     d_emptyOpList ),                           // 0x20
    M3OP( "local.set",             1,  none,   d_cc(Compile_SetLocal),     d_emptyOpList ),                           // 0x21
    M3OP( "local.tee",             0,  any,    d_cc(Compile_SetLocal),     d_emptyOpList ),                           // 0x22
    M3OP( "global.get",            1,  none,   d_cc(Compile_GetSetGlobal), d_emptyOpList ),                           // 0x23
    M3OP( "global.set",            1,  none,   d_cc(Compile_GetSetGlobal), d_emptyOpList ),                           // 0x24

#अगर d_m3HasRefTypes
    M3OP( "table.get",             0,  any,    d_cc(Compile_Table_GetSet), d_emptyOpList ),                           // 0x25
    M3OP( "table.set",             -2, none,   d_cc(Compile_Table_GetSet), d_emptyOpList ),                           // 0x26
#अन्यथा
    M3OP_RESERVED,  M3OP_RESERVED,                                                                                    // 0x25...0x26
#पूर्ण_अगर
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

    M3OP( "memory.size",           1,  i_32,   d_cc(Compile_Memory_आकार),  d_logOp (Mem_आकार) ),                       // 0x3f
    M3OP( "memory.grow",           1,  i_32,   d_cc(Compile_Memory_Grow),  d_logOp (MemGrow) ),                       // 0x40

    M3OP( "i32.const",             1,  i_32,   d_cc(Compile_Const_i32),    d_logOp (Const32) ),                       // 0x41
    M3OP( "i64.const",             1,  i_64,   d_cc(Compile_Const_i64),    d_logOp (Const64) ),                       // 0x42
    M3OP_F( "f32.const",           1,  f_32,   d_cc(Compile_Const_f32),    d_emptyOpList ),                           // 0x43
    M3OP_F( "f64.const",           1,  f_64,   d_cc(Compile_Const_f64),    d_emptyOpList ),                           // 0x44

    M3OP( "i32.eqz",               0,  i_32,   d_cc(शून्य),                 d_unaryOpList (i32, EqualToZero) ),        // 0x45
    M3OP( "i32.eq",                -1, i_32,   d_cc(शून्य),                 d_commutativeBinOpList (i32, Equal) ),     // 0x46
    M3OP( "i32.ne",                -1, i_32,   d_cc(शून्य),                 d_commutativeBinOpList (i32, NotEqual) ),  // 0x47
    M3OP( "i32.lt_s",              -1, i_32,   d_cc(शून्य),                 d_binOpList (i32, LessThan) ),             // 0x48
    M3OP( "i32.lt_u",              -1, i_32,   d_cc(शून्य),                 d_binOpList (u32, LessThan) ),             // 0x49
    M3OP( "i32.gt_s",              -1, i_32,   d_cc(शून्य),                 d_binOpList (i32, GreaterThan) ),          // 0x4a
    M3OP( "i32.gt_u",              -1, i_32,   d_cc(शून्य),                 d_binOpList (u32, GreaterThan) ),          // 0x4b
    M3OP( "i32.le_s",              -1, i_32,   d_cc(शून्य),                 d_binOpList (i32, LessThanOrEqual) ),      // 0x4c
    M3OP( "i32.le_u",              -1, i_32,   d_cc(शून्य),                 d_binOpList (u32, LessThanOrEqual) ),      // 0x4d
    M3OP( "i32.ge_s",              -1, i_32,   d_cc(शून्य),                 d_binOpList (i32, GreaterThanOrEqual) ),   // 0x4e
    M3OP( "i32.ge_u",              -1, i_32,   d_cc(शून्य),                 d_binOpList (u32, GreaterThanOrEqual) ),   // 0x4f

    M3OP( "i64.eqz",               0,  i_32,   d_cc(शून्य),                 d_unaryOpList (i64, EqualToZero) ),        // 0x50
    M3OP( "i64.eq",                -1, i_32,   d_cc(शून्य),                 d_commutativeBinOpList (i64, Equal) ),     // 0x51
    M3OP( "i64.ne",                -1, i_32,   d_cc(शून्य),                 d_commutativeBinOpList (i64, NotEqual) ),  // 0x52
    M3OP( "i64.lt_s",              -1, i_32,   d_cc(शून्य),                 d_binOpList (i64, LessThan) ),             // 0x53
    M3OP( "i64.lt_u",              -1, i_32,   d_cc(शून्य),                 d_binOpList (u64, LessThan) ),             // 0x54
    M3OP( "i64.gt_s",              -1, i_32,   d_cc(शून्य),                 d_binOpList (i64, GreaterThan) ),          // 0x55
    M3OP( "i64.gt_u",              -1, i_32,   d_cc(शून्य),                 d_binOpList (u64, GreaterThan) ),          // 0x56
    M3OP( "i64.le_s",              -1, i_32,   d_cc(शून्य),                 d_binOpList (i64, LessThanOrEqual) ),      // 0x57
    M3OP( "i64.le_u",              -1, i_32,   d_cc(शून्य),                 d_binOpList (u64, LessThanOrEqual) ),      // 0x58
    M3OP( "i64.ge_s",              -1, i_32,   d_cc(शून्य),                 d_binOpList (i64, GreaterThanOrEqual) ),   // 0x59
    M3OP( "i64.ge_u",              -1, i_32,   d_cc(शून्य),                 d_binOpList (u64, GreaterThanOrEqual) ),   // 0x5a

    M3OP_F( "f32.eq",              -1, i_32,   d_cc(शून्य),                 d_commutativeBinOpList (f32, Equal) ),     // 0x5b
    M3OP_F( "f32.ne",              -1, i_32,   d_cc(शून्य),                 d_commutativeBinOpList (f32, NotEqual) ),  // 0x5c
    M3OP_F( "f32.lt",              -1, i_32,   d_cc(शून्य),                 d_binOpList (f32, LessThan) ),             // 0x5d
    M3OP_F( "f32.gt",              -1, i_32,   d_cc(शून्य),                 d_binOpList (f32, GreaterThan) ),          // 0x5e
    M3OP_F( "f32.le",              -1, i_32,   d_cc(शून्य),                 d_binOpList (f32, LessThanOrEqual) ),      // 0x5f
    M3OP_F( "f32.ge",              -1, i_32,   d_cc(शून्य),                 d_binOpList (f32, GreaterThanOrEqual) ),   // 0x60

    M3OP_F( "f64.eq",              -1, i_32,   d_cc(शून्य),                 d_commutativeBinOpList (f64, Equal) ),     // 0x61
    M3OP_F( "f64.ne",              -1, i_32,   d_cc(शून्य),                 d_commutativeBinOpList (f64, NotEqual) ),  // 0x62
    M3OP_F( "f64.lt",              -1, i_32,   d_cc(शून्य),                 d_binOpList (f64, LessThan) ),             // 0x63
    M3OP_F( "f64.gt",              -1, i_32,   d_cc(शून्य),                 d_binOpList (f64, GreaterThan) ),          // 0x64
    M3OP_F( "f64.le",              -1, i_32,   d_cc(शून्य),                 d_binOpList (f64, LessThanOrEqual) ),      // 0x65
    M3OP_F( "f64.ge",              -1, i_32,   d_cc(शून्य),                 d_binOpList (f64, GreaterThanOrEqual) ),   // 0x66

    M3OP( "i32.clz",               0,  i_32,   d_cc(शून्य),                 d_unaryOpList (u32, Clz) ),                // 0x67
    M3OP( "i32.ctz",               0,  i_32,   d_cc(शून्य),                 d_unaryOpList (u32, Ctz) ),                // 0x68
    M3OP( "i32.popcnt",            0,  i_32,   d_cc(शून्य),                 d_unaryOpList (u32, Popcnt) ),             // 0x69

    M3OP( "i32.add",               -1, i_32,   d_cc(शून्य),                 d_commutativeBinOpList (i32, Add) ),       // 0x6a
    M3OP( "i32.sub",               -1, i_32,   d_cc(शून्य),                 d_binOpList (i32, Subtract) ),             // 0x6b
    M3OP( "i32.mul",               -1, i_32,   d_cc(शून्य),                 d_commutativeBinOpList (i32, Multiply) ),  // 0x6c
    M3OP( "i32.div_s",             -1, i_32,   d_cc(शून्य),                 d_binOpList (i32, Divide) ),               // 0x6d
    M3OP( "i32.div_u",             -1, i_32,   d_cc(शून्य),                 d_binOpList (u32, Divide) ),               // 0x6e
    M3OP( "i32.rem_s",             -1, i_32,   d_cc(शून्य),                 d_binOpList (i32, Remainder) ),            // 0x6f
    M3OP( "i32.rem_u",             -1, i_32,   d_cc(शून्य),                 d_binOpList (u32, Remainder) ),            // 0x70
    M3OP( "i32.and",               -1, i_32,   d_cc(शून्य),                 d_commutativeBinOpList (u32, And) ),       // 0x71
    M3OP( "i32.or",                -1, i_32,   d_cc(शून्य),                 d_commutativeBinOpList (u32, Or) ),        // 0x72
    M3OP( "i32.xor",               -1, i_32,   d_cc(शून्य),                 d_commutativeBinOpList (u32, Xor) ),       // 0x73
    M3OP( "i32.shl",               -1, i_32,   d_cc(शून्य),                 d_binOpList (u32, Shift_बायाँ) ),            // 0x74
    M3OP( "i32.shr_s",             -1, i_32,   d_cc(शून्य),                 d_binOpList (i32, Shift_दायाँ) ),           // 0x75
    M3OP( "i32.shr_u",             -1, i_32,   d_cc(शून्य),                 d_binOpList (u32, Shift_दायाँ) ),           // 0x76
    M3OP( "i32.rotl",              -1, i_32,   d_cc(शून्य),                 d_binOpList (u32, Rotl) ),                 // 0x77
    M3OP( "i32.rotr",              -1, i_32,   d_cc(शून्य),                 d_binOpList (u32, Rotr) ),                 // 0x78

    M3OP( "i64.clz",               0,  i_64,   d_cc(शून्य),                 d_unaryOpList (u64, Clz) ),                // 0x79
    M3OP( "i64.ctz",               0,  i_64,   d_cc(शून्य),                 d_unaryOpList (u64, Ctz) ),                // 0x7a
    M3OP( "i64.popcnt",            0,  i_64,   d_cc(शून्य),                 d_unaryOpList (u64, Popcnt) ),             // 0x7b

    M3OP( "i64.add",               -1, i_64,   d_cc(शून्य),                 d_commutativeBinOpList (i64, Add) ),       // 0x7c
    M3OP( "i64.sub",               -1, i_64,   d_cc(शून्य),                 d_binOpList (i64, Subtract) ),             // 0x7d
    M3OP( "i64.mul",               -1, i_64,   d_cc(शून्य),                 d_commutativeBinOpList (i64, Multiply) ),  // 0x7e
    M3OP( "i64.div_s",             -1, i_64,   d_cc(शून्य),                 d_binOpList (i64, Divide) ),               // 0x7f
    M3OP( "i64.div_u",             -1, i_64,   d_cc(शून्य),                 d_binOpList (u64, Divide) ),               // 0x80
    M3OP( "i64.rem_s",             -1, i_64,   d_cc(शून्य),                 d_binOpList (i64, Remainder) ),            // 0x81
    M3OP( "i64.rem_u",             -1, i_64,   d_cc(शून्य),                 d_binOpList (u64, Remainder) ),            // 0x82
    M3OP( "i64.and",               -1, i_64,   d_cc(शून्य),                 d_commutativeBinOpList (u64, And) ),       // 0x83
    M3OP( "i64.or",                -1, i_64,   d_cc(शून्य),                 d_commutativeBinOpList (u64, Or) ),        // 0x84
    M3OP( "i64.xor",               -1, i_64,   d_cc(शून्य),                 d_commutativeBinOpList (u64, Xor) ),       // 0x85
    M3OP( "i64.shl",               -1, i_64,   d_cc(शून्य),                 d_binOpList (u64, Shift_बायाँ) ),            // 0x86
    M3OP( "i64.shr_s",             -1, i_64,   d_cc(शून्य),                 d_binOpList (i64, Shift_दायाँ) ),           // 0x87
    M3OP( "i64.shr_u",             -1, i_64,   d_cc(शून्य),                 d_binOpList (u64, Shift_दायाँ) ),           // 0x88
    M3OP( "i64.rotl",              -1, i_64,   d_cc(शून्य),                 d_binOpList (u64, Rotl) ),                 // 0x89
    M3OP( "i64.rotr",              -1, i_64,   d_cc(शून्य),                 d_binOpList (u64, Rotr) ),                 // 0x8a

    M3OP_F( "f32.abs",             0,  f_32,   d_cc(शून्य),                 d_unaryOpList(f32, Abs) ),                 // 0x8b
    M3OP_F( "f32.neg",             0,  f_32,   d_cc(शून्य),                 d_unaryOpList(f32, Negate) ),              // 0x8c
    M3OP_F( "f32.ceil",            0,  f_32,   d_cc(शून्य),                 d_unaryOpList(f32, Ceil) ),                // 0x8d
    M3OP_F( "f32.floor",           0,  f_32,   d_cc(शून्य),                 d_unaryOpList(f32, Floor) ),               // 0x8e
    M3OP_F( "f32.trunc",           0,  f_32,   d_cc(शून्य),                 d_unaryOpList(f32, Trunc) ),               // 0x8f
    M3OP_F( "f32.nearest",         0,  f_32,   d_cc(शून्य),                 d_unaryOpList(f32, Nearest) ),             // 0x90
    M3OP_F( "f32.sqrt",            0,  f_32,   d_cc(शून्य),                 d_unaryOpList(f32, Sqrt) ),                // 0x91

    M3OP_F( "f32.add",             -1, f_32,   d_cc(शून्य),                 d_commutativeBinOpList (f32, Add) ),       // 0x92
    M3OP_F( "f32.sub",             -1, f_32,   d_cc(शून्य),                 d_binOpList (f32, Subtract) ),             // 0x93
    M3OP_F( "f32.mul",             -1, f_32,   d_cc(शून्य),                 d_commutativeBinOpList (f32, Multiply) ),  // 0x94
    M3OP_F( "f32.div",             -1, f_32,   d_cc(शून्य),                 d_binOpList (f32, Divide) ),               // 0x95
    M3OP_F( "f32.min",             -1, f_32,   d_cc(शून्य),                 d_commutativeBinOpList (f32, Min) ),       // 0x96
    M3OP_F( "f32.max",             -1, f_32,   d_cc(शून्य),                 d_commutativeBinOpList (f32, Max) ),       // 0x97
    M3OP_F( "f32.copysign",        -1, f_32,   d_cc(शून्य),                 d_binOpList (f32, CopySign) ),             // 0x98

    M3OP_F( "f64.abs",             0,  f_64,   d_cc(शून्य),                 d_unaryOpList(f64, Abs) ),                 // 0x99
    M3OP_F( "f64.neg",             0,  f_64,   d_cc(शून्य),                 d_unaryOpList(f64, Negate) ),              // 0x9a
    M3OP_F( "f64.ceil",            0,  f_64,   d_cc(शून्य),                 d_unaryOpList(f64, Ceil) ),                // 0x9b
    M3OP_F( "f64.floor",           0,  f_64,   d_cc(शून्य),                 d_unaryOpList(f64, Floor) ),               // 0x9c
    M3OP_F( "f64.trunc",           0,  f_64,   d_cc(शून्य),                 d_unaryOpList(f64, Trunc) ),               // 0x9d
    M3OP_F( "f64.nearest",         0,  f_64,   d_cc(शून्य),                 d_unaryOpList(f64, Nearest) ),             // 0x9e
    M3OP_F( "f64.sqrt",            0,  f_64,   d_cc(शून्य),                 d_unaryOpList(f64, Sqrt) ),                // 0x9f

    M3OP_F( "f64.add",             -1, f_64,   d_cc(शून्य),                 d_commutativeBinOpList (f64, Add) ),       // 0xa0
    M3OP_F( "f64.sub",             -1, f_64,   d_cc(शून्य),                 d_binOpList (f64, Subtract) ),             // 0xa1
    M3OP_F( "f64.mul",             -1, f_64,   d_cc(शून्य),                 d_commutativeBinOpList (f64, Multiply) ),  // 0xa2
    M3OP_F( "f64.div",             -1, f_64,   d_cc(शून्य),                 d_binOpList (f64, Divide) ),               // 0xa3
    M3OP_F( "f64.min",             -1, f_64,   d_cc(शून्य),                 d_commutativeBinOpList (f64, Min) ),       // 0xa4
    M3OP_F( "f64.max",             -1, f_64,   d_cc(शून्य),                 d_commutativeBinOpList (f64, Max) ),       // 0xa5
    M3OP_F( "f64.copysign",        -1, f_64,   d_cc(शून्य),                 d_binOpList (f64, CopySign) ),             // 0xa6

    M3OP( "i32.wrap/i64",          0,  i_32,   d_cc(शून्य),                 d_unaryOpList (i32, Wrap_i64) ),           // 0xa7
    M3OP_F( "i32.trunc_s/f32",     0,  i_32,   d_cc(Compile_Convert),      d_convertOpList (i32_Trunc_f32) ),         // 0xa8
    M3OP_F( "i32.trunc_u/f32",     0,  i_32,   d_cc(Compile_Convert),      d_convertOpList (u32_Trunc_f32) ),         // 0xa9
    M3OP_F( "i32.trunc_s/f64",     0,  i_32,   d_cc(Compile_Convert),      d_convertOpList (i32_Trunc_f64) ),         // 0xaa
    M3OP_F( "i32.trunc_u/f64",     0,  i_32,   d_cc(Compile_Convert),      d_convertOpList (u32_Trunc_f64) ),         // 0xab

    M3OP( "i64.extend_s/i32",      0,  i_64,   d_cc(शून्य),                 d_unaryOpList (i64, Extend_i32) ),         // 0xac
    M3OP( "i64.extend_u/i32",      0,  i_64,   d_cc(शून्य),                 d_unaryOpList (i64, Extend_u32) ),         // 0xad

    M3OP_F( "i64.trunc_s/f32",     0,  i_64,   d_cc(Compile_Convert),      d_convertOpList (i64_Trunc_f32) ),         // 0xae
    M3OP_F( "i64.trunc_u/f32",     0,  i_64,   d_cc(Compile_Convert),      d_convertOpList (u64_Trunc_f32) ),         // 0xaf
    M3OP_F( "i64.trunc_s/f64",     0,  i_64,   d_cc(Compile_Convert),      d_convertOpList (i64_Trunc_f64) ),         // 0xb0
    M3OP_F( "i64.trunc_u/f64",     0,  i_64,   d_cc(Compile_Convert),      d_convertOpList (u64_Trunc_f64) ),         // 0xb1

    M3OP_F( "f32.convert_s/i32",   0,  f_32,   d_cc(Compile_Convert),      d_convertOpList (f32_Convert_i32) ),       // 0xb2
    M3OP_F( "f32.convert_u/i32",   0,  f_32,   d_cc(Compile_Convert),      d_convertOpList (f32_Convert_u32) ),       // 0xb3
    M3OP_F( "f32.convert_s/i64",   0,  f_32,   d_cc(Compile_Convert),      d_convertOpList (f32_Convert_i64) ),       // 0xb4
    M3OP_F( "f32.convert_u/i64",   0,  f_32,   d_cc(Compile_Convert),      d_convertOpList (f32_Convert_u64) ),       // 0xb5

    M3OP_F( "f32.demote/f64",      0,  f_32,   d_cc(शून्य),                 d_unaryOpList (f32, Demote_f64) ),         // 0xb6

    M3OP_F( "f64.convert_s/i32",   0,  f_64,   d_cc(Compile_Convert),      d_convertOpList (f64_Convert_i32) ),       // 0xb7
    M3OP_F( "f64.convert_u/i32",   0,  f_64,   d_cc(Compile_Convert),      d_convertOpList (f64_Convert_u32) ),       // 0xb8
    M3OP_F( "f64.convert_s/i64",   0,  f_64,   d_cc(Compile_Convert),      d_convertOpList (f64_Convert_i64) ),       // 0xb9
    M3OP_F( "f64.convert_u/i64",   0,  f_64,   d_cc(Compile_Convert),      d_convertOpList (f64_Convert_u64) ),       // 0xba

    M3OP_F( "f64.promote/f32",     0,  f_64,   d_cc(शून्य),                 d_unaryOpList (f64, Promote_f32) ),        // 0xbb

    M3OP_F( "i32.reinterpret/f32", 0,  i_32,   d_cc(Compile_Convert),      d_convertOpList (i32_Reinterpret_f32) ),   // 0xbc
    M3OP_F( "i64.reinterpret/f64", 0,  i_64,   d_cc(Compile_Convert),      d_convertOpList (i64_Reinterpret_f64) ),   // 0xbd
    M3OP_F( "f32.reinterpret/i32", 0,  f_32,   d_cc(Compile_Convert),      d_convertOpList (f32_Reinterpret_i32) ),   // 0xbe
    M3OP_F( "f64.reinterpret/i64", 0,  f_64,   d_cc(Compile_Convert),      d_convertOpList (f64_Reinterpret_i64) ),   // 0xbf

    M3OP( "i32.extend8_s",         0,  i_32,   d_cc(शून्य),                 d_unaryOpList (i32, Extend8_s) ),          // 0xc0
    M3OP( "i32.extend16_s",        0,  i_32,   d_cc(शून्य),                 d_unaryOpList (i32, Extend16_s) ),         // 0xc1
    M3OP( "i64.extend8_s",         0,  i_64,   d_cc(शून्य),                 d_unaryOpList (i64, Extend8_s) ),          // 0xc2
    M3OP( "i64.extend16_s",        0,  i_64,   d_cc(शून्य),                 d_unaryOpList (i64, Extend16_s) ),         // 0xc3
    M3OP( "i64.extend32_s",        0,  i_64,   d_cc(शून्य),                 d_unaryOpList (i64, Extend32_s) ),         // 0xc4

// Internal operations, for codepage logging only. They sit past c_waOp_lastCore,
// the last opcode this table covers, so GetOpInfo () can never reach them.
# ifdef DEBUG // for codepage logging. the order doesn't matter:
#घोषणा d_m3DebugOp(OP) M3OP (#OP, 0, none, d_ops (op_##OP))

# अगर d_m3HasFloat
#घोषणा d_m3DebugTypedOp(OP) M3OP (#OP, 0, none, d_ops (op_##OP##_i32, op_##OP##_i64, op_##OP##_f32, op_##OP##_f64))
# अन्यथा
#घोषणा d_m3DebugTypedOp(OP) M3OP (#OP, 0, none, d_ops (op_##OP##_i32, op_##OP##_i64))
# endif

    d_m3DebugOp (Compile),          d_m3DebugOp (Entry),            d_m3DebugOp (End),
    d_m3DebugOp (Unsupported),      d_m3DebugOp (CallRawFunction),

    d_m3DebugOp (GetGlobal_s32),    d_m3DebugOp (GetGlobal_s64),    d_m3DebugOp (ContinueLoop),     d_m3DebugOp (ContinueLoopIf),
# अगर d_m3_Has_ढेर_Switching
    d_m3DebugOp (ContinueLoop_Suspendable), d_m3DebugOp (ContinueLoopIf_Suspendable), d_m3DebugOp (EntryCheck),
#  अगर !d_m3EntryKeepsFrame
    d_m3DebugOp (Entry_Suspendable),
#  endif
# endif

    d_m3DebugOp (CopySlot_32),      d_m3DebugOp (PreserveCopySlot_32), d_m3DebugOp (If_s),          d_m3DebugOp (BranchIfPrologue_s),
    d_m3DebugOp (CopySlot_64),      d_m3DebugOp (PreserveCopySlot_64), d_m3DebugOp (If_r),          d_m3DebugOp (BranchIfPrologue_r),

    d_m3DebugOp (Select_i32_rss),   d_m3DebugOp (Select_i32_srs),   d_m3DebugOp (Select_i32_ssr),   d_m3DebugOp (Select_i32_sss),
    d_m3DebugOp (Select_i64_rss),   d_m3DebugOp (Select_i64_srs),   d_m3DebugOp (Select_i64_ssr),   d_m3DebugOp (Select_i64_sss),

# अगर d_m3HasFloat
    d_m3DebugOp (Select_f32_sss),   d_m3DebugOp (Select_f32_srs),   d_m3DebugOp (Select_f32_ssr),
    d_m3DebugOp (Select_f32_rss),   d_m3DebugOp (Select_f32_rrs),   d_m3DebugOp (Select_f32_rsr),

    d_m3DebugOp (Select_f64_sss),   d_m3DebugOp (Select_f64_srs),   d_m3DebugOp (Select_f64_ssr),
    d_m3DebugOp (Select_f64_rss),   d_m3DebugOp (Select_f64_rrs),   d_m3DebugOp (Select_f64_rsr),
# endif

    d_m3DebugOp (MemFill),          d_m3DebugOp (MemCopy),          d_m3DebugOp (MemInit),          d_m3DebugOp (तथ्य_Drop),

# अगर d_m3HasGasMetering
    d_m3DebugOp (UseGas),
# endif

# अगर d_m3HasRefTypes
    d_m3DebugOp (TableGet),         d_m3DebugOp (TableSet),         d_m3DebugOp (Table_आकार),
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

स्थिर M3OpInfo c_operationsFC[] =
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
    M3OP( "data.drop",             0, none,   d_cc(Compile_तथ्य_Drop),       d_emptyOpList ),                       // 0x09

    M3OP( "memory.copy",           0, none,   d_cc(Compile_Memory_CopyFill), d_emptyOpList ),                       // 0x0a
    M3OP( "memory.fill",           0, none,   d_cc(Compile_Memory_CopyFill), d_emptyOpList ),                       // 0x0b

#अगर d_m3HasRefTypes
    M3OP( "table.init",            0, none,   d_cc(Compile_Table_Init),      d_emptyOpList ),                       // 0x0c
    M3OP( "elem.drop",             0, none,   d_cc(Compile_Elem_Drop),       d_emptyOpList ),                       // 0x0d
    M3OP( "table.copy",            0, none,   d_cc(Compile_Table_Copy),      d_emptyOpList ),                       // 0x0e
#अन्यथा
    M3OP_RESERVED, M3OP_RESERVED, M3OP_RESERVED,                                                                    // 0x0c...0x0e
#पूर्ण_अगर

#अगर d_m3HasRefTypes
    M3OP( "table.grow",            0, i_32,   d_cc(Compile_Table_GrowFill),  d_emptyOpList ),                       // 0x0f
    M3OP( "table.size",            1, i_32,   d_cc(Compile_Table_आकार),      d_emptyOpList ),                       // 0x10
    M3OP( "table.fill",            0, none,   d_cc(Compile_Table_GrowFill),  d_emptyOpList ),                       // 0x11
#अन्यथा
    M3OP_RESERVED, M3OP_RESERVED, M3OP_RESERVED,                                                                    // 0x0f...0x11
#पूर्ण_अगर

#अगर d_m3HasWideArithmetic
    M3OP_RESERVED,                                                                                                  // 0x12

    M3OP( "i64.add128",            -2, i_64,  d_cc(Compile_WideArithmetic), d_logOp (i64_Add128) ),              // 0x13
    M3OP( "i64.sub128",            -2, i_64,  d_cc(Compile_WideArithmetic), d_logOp (i64_Sub128) ),              // 0x14
    M3OP( "i64.mul_wide_s",        0,  i_64,  d_cc(Compile_WideArithmetic), d_logOp (i64_MultiplyWide) ),        // 0x15
    M3OP( "i64.mul_wide_u",        0,  i_64,  d_cc(Compile_WideArithmetic), d_logOp (u64_MultiplyWide) ),        // 0x16
#पूर्ण_अगर


# ifdef DEBUG
    M3OP( "termination", 0, c_m3Type_unknown, d_emptyOpList ) // for find_operation_info
# endif
};

#अगर d_m3HasAtomics

// The 0xFE instructions share one entry per kind rather than each having its own: what
// tells them apart, the width of the cell and the operation, is worked out from the
// sub-opcode by Compile_Atomic. GetAtomicInfoIndex () maps a sub-opcode onto this table.
क्रमागत {
    c_opAtomic_notify,
    c_opAtomic_wait32,
    c_opAtomic_wait64,
    c_opAtomic_fence,
    c_opAtomic_load,
    c_opAtomic_store,
    c_opAtomic_rmw,
    c_opAtomic_cmpxchg
};

स्थिर M3OpInfo c_operationsFE[] =
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

स्थिर u32 c_अंक_Operations_FE = M3_गिनती_OF(c_operationsFE);

// The entry a 0xFE sub-opcode uses, or -1 for one that names nothing: the spec leaves
// 0x04..0x0f and everything past cmpxchg unassigned. The accesses come in runs of seven,
// one per pairing of value type and cell width.
अटल अंतरभूत
i32 Get_Atomic_Info_सूचकांक (u32 i_sub)
{
    अगर (i_sub <= 0x03) वापस (i32)i_sub;
    अगर (i_sub < 0x10) वापस -1;
    अगर (i_sub <= 0x16) वापस c_opAtomic_load;
    अगर (i_sub <= 0x1d) वापस c_opAtomic_store;
    अगर (i_sub <= 0x47) वापस c_opAtomic_rmw;
    अगर (i_sub <= c_waOp_lastAtomic) वापस c_opAtomic_cmpxchg;
    वापस -1;
}

#पूर्ण_अगर // d_m3HasAtomics

// The opcodes above c_waOp_lastCore are sparse: the reference instructions at
// 0xd0..0xd2 and 0xd4, and the 0xfc extended-opcode prefix. Indexing c_operations out to
// 0xfc to reach them would leave ~50 empty entries there, so they get their own
// table, which GetHighOpIndex () maps the opcode onto.
क्रमागत {
#अगर d_m3HasRefTypes
    c_op_ऊपरी_ref_Null,
    c_op_ऊपरी_ref_Is_Null,
    c_op_ऊपरी_ref_Func,
#  अगर d_m3HasTypedRefs
    c_op_ऊपरी_ref_As_Non_Null,
#  endif
#पूर्ण_अगर
#अगर d_m3_Has_ढेर_Switching
    c_op_ऊपरी_cont_New,
    c_op_ऊपरी_cont_Bind,
    c_op_ऊपरी_suspend,
    c_op_ऊपरी_resume,
    c_op_ऊपरी_resume_Throw,
    c_op_ऊपरी_resume_Throw_Ref,
    c_op_ऊपरी_switch,
#पूर्ण_अगर
#अगर d_m3HasAtomics
    c_op_ऊपरी_atomic,
#पूर्ण_अगर
    c_op_ऊपरी_extended   // always present, so the enum is never empty
};

// Positional, so the table must stay in c_opHigh_* order: array designators
// are not valid C++, which the maintenance builds compile this as.
अटल स्थिर M3OpInfo c_operations_ऊपरी[] =
{
#अगर d_m3HasRefTypes
    M3OP( "ref.null",        1, any,               d_cc(Compile_Ref_Null),       d_emptyOpList ),  // 0xd0  c_opHigh_refNull
    M3OP( "ref.is_null",     0, i_32,              d_cc(Compile_Ref_IsNull),     d_emptyOpList ),  // 0xd1  c_opHigh_refIsNull
    M3OP( "ref.func",        1, any,               d_cc(Compile_Ref_Func),       d_emptyOpList ),  // 0xd2  c_opHigh_refFunc
#  अगर d_m3HasTypedRefs
    M3OP( "ref.as_non_null", 0, any,               d_cc(Compile_Ref_AsNonNull),  d_emptyOpList ),  // 0xd4  c_opHigh_refAsNonNull
#  endif
#पूर्ण_अगर
#अगर d_m3_Has_ढेर_Switching
    M3OP( "cont.new",          1, any,             d_cc(Compile_ContNew),        d_emptyOpList ),  // 0xe0  c_opHigh_contNew
    M3OP( "cont.bind",         2, any,             d_cc(Compile_ContBind),       d_emptyOpList ),  // 0xe1  c_opHigh_contBind
    M3OP( "suspend",           1, any,             d_cc(Compile_Suspend),        d_emptyOpList ),  // 0xe2  c_opHigh_suspend
    M3OP( "resume",            1, any,             d_cc(Compile_Resume),         d_emptyOpList ),  // 0xe3  c_opHigh_resume
    M3OP( "resume_throw",      2, any,             d_cc(Compile_Resume)   ,    d_emptyOpList ),  // 0xe4  c_opHigh_resumeThrow
    M3OP( "resume_throw_ref",  1, any,             d_cc(Compile_Resume)      , d_emptyOpList ),  // 0xe5  c_opHigh_resumeThrowRef
    M3OP( "switch",            2, any,             d_cc(Compile_Switch),         d_emptyOpList ),  // 0xe6  c_opHigh_switch
#पूर्ण_अगर

#अगर d_m3HasAtomics
    M3OP( "0xFE",            0, c_m3Type_unknown,  d_cc(Compile_ExtendedOpcode), d_emptyOpList ),  // 0xfe  c_opHigh_atomic
#पूर्ण_अगर
    M3OP( "0xFC",            0, c_m3Type_unknown,  d_cc(Compile_ExtendedOpcode), d_emptyOpList ),  // 0xfc  c_opHigh_extended
};
M3_STATIC_ASSERT (M3_गिनती_OF (c_operations_ऊपरी) == c_op_ऊपरी_extended + 1, c_operations_ऊपरी_needs_one_entry_per_c_op_ऊपरी);

// clang-format on


// Opcodes the spec reserves leave zeroed holes in the tables above: no compiler
// and no operations. Every implemented op has at least one of the two.
अटल अंतरभूत
bool IsImplementedOp (IM3OpInfo i_info)
{
    वापस (i_info->compiler != c_cc_NULL or i_info->operations[0] != शून्य);
}

स्थिर u32 c_अंक_Operations   = M3_गिनती_OF(c_operations);
स्थिर u32 c_अंक_Operations_FC = M3_गिनती_OF(c_operationsFC);

// Maps one of the sparse opcodes above c_waOp_lastCore onto c_operationsHigh,
// or -1 for anything else in that range.
अटल अंतरभूत
i32 Get_ऊपरी_Op_सूचकांक (m3opcode_t opcode)
{
    चयन (opcode) {
#अगर d_m3HasRefTypes
    हाल c_waOp_refNull: वापस c_op_ऊपरी_ref_Null;
    हाल c_waOp_refIsNull: वापस c_op_ऊपरी_ref_Is_Null;
    हाल c_waOp_refFunc: वापस c_op_ऊपरी_ref_Func;
#  अगर d_m3HasTypedRefs
    हाल c_waOp_refAsNonNull: वापस c_op_ऊपरी_ref_As_Non_Null;
#  endif
#पूर्ण_अगर
#अगर d_m3_Has_ढेर_Switching
    हाल c_waOp_contNew: वापस c_op_ऊपरी_cont_New;
    हाल c_waOp_contBind: वापस c_op_ऊपरी_cont_Bind;
    हाल c_waOp_suspend: वापस c_op_ऊपरी_suspend;
    हाल c_waOp_resume: वापस c_op_ऊपरी_resume;
    हाल c_waOp_resumeThrow: वापस c_op_ऊपरी_resume_Throw;
    हाल c_waOp_resumeThrowRef: वापस c_op_ऊपरी_resume_Throw_Ref;
    हाल c_waOp_switch: वापस c_op_ऊपरी_switch;
#पूर्ण_अगर
    हाल c_waOp_extended: वापस c_op_ऊपरी_extended;
#अगर d_m3HasAtomics
    हाल c_waOp_atomic: वापस c_op_ऊपरी_atomic;
#पूर्ण_अगर
    शेष: वापस -1;
    }
}

IM3OpInfo GetOpInfo (m3opcode_t opcode)
{
    IM3OpInfo info = शून्य;

    चयन (opcode >> 8) {
    हाल 0x00:
        // c_operations is indexed by opcode only up to c_waOp_lastCore; past that
        // it holds internal operations (DEBUG builds), unreachable by opcode.
        अगर (M3_LIKELY(opcode <= c_waOp_lastCore)) {
            info = &c_operations[opcode];
        } अन्यथा {
            i32 ऊपरी = Get_ऊपरी_Op_सूचकांक(opcode);
            अगर (ऊपरी >= 0) {
                info = &c_operations_ऊपरी[ऊपरी];
            }
        }
        अवरोध;
    हाल c_waOp_extended:
        opcode &= 0xFF;
        अगर (M3_LIKELY(opcode <= c_waOp_lastExtended)) {
            info = &c_operationsFC[opcode];
        }
        अवरोध;
#अगर d_m3HasAtomics
    हाल c_waOp_atomic: {
        i32 index = Get_Atomic_Info_सूचकांक(opcode & 0xFF);
        अगर (index >= 0) {
            info = &c_operationsFE[index];
        }
    } अवरोध;
#पूर्ण_अगर
    }

    वापस (info and IsImplementedOp(info)) ? info : शून्य;
}

#अगर d_m3HasGasMetering

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
क्रमागत {
    c_gasLocal   = 1,          // charged once per declared local, at function entry
    c_gasNominal = 1,          // nop, block, loop, if, const
    c_gasArith   = 45,
    c_gasShift   = 67,
    c_gasBranch  = 90,
    c_gas_Mem_आकार = 100,
    c_gasAccess  = 120,        // a local, global, table or memory access
    c_gasHeavy   = 10000,      // an indirect call, or anything that works over a whole region
    c_gasDivide  = 36000
};

// The most one op_UseGas can carry. A segment that reaches it is split into
// consecutive charges, which prepays exactly the same amount.
#  define d_m3MaxSegmentGas   0xFFFFFFFF

अटल
u32 GetGasCost (m3opcode_t i_opcode)
{
    चयन (i_opcode) {
    हाल 0x00:                        // unreachable
    हाल 0x01:                        // nop
    हाल c_waOp_block:
    हाल c_waOp_loop:
    हाल c_waOp_if:
    हाल c_waOp_tryTable:             // a block that also installs handlers
    हाल c_waOp_refNull:              // a constant, by another name
    हाल c_waOp_refFunc:
        वापस c_gasNominal;

    हाल c_waOp_end:
        वापस 0;                     // the one control instruction the table prices at nothing

    हाल c_waOp_else:
    हाल c_waOp_branch:
    हाल c_waOp_branchIf:
    हाल c_waOp_return:
    हाल c_waOp_call:
    हाल c_waOp_returnCall:
    हाल c_waOp_throw:
    हाल c_waOp_throwRef:
        वापस c_gasBranch;

    हाल c_waOp_branchTable:
    हाल 0x1a:                        // drop
    हाल 0x1b:                        // select
    हाल c_waOp_selectTyped:
        वापस c_gasAccess;

    हाल c_waOp_callIndirect:
    हाल c_waOp_returnCallIndirect:
    हाल c_waOp_callRef:
    हाल c_waOp_returnCallRef:
    हाल c_waOp_memoryInit:
    हाल c_waOp_memoryCopy:
    हाल c_waOp_memoryFill:
    हाल c_waOp_tableInit:
    हाल c_waOp_tableCopy:
    हाल c_waOp_tableGrow:
    हाल c_waOp_tableFill:
    हाल 0x40:                        // memory.grow
        वापस c_gasHeavy;

    हाल c_wa_Op_तथ्य_Drop:
    हाल c_waOp_elemDrop:
    हाल c_wa_Op_table_आकार:
    हाल 0x3f:                        // memory.size
        वापस c_gas_Mem_आकार;

    हाल c_waOp_refIsNull:
    हाल c_waOp_refAsNonNull:
        वापस c_gasArith;

    शेष:
        अवरोध;
    }

    // The two integer arithmetic runs are priced by position, i32 at 0x6a and
    // i64 at 0x7c, the table giving both the same shape
    अगर ((i_opcode >= 0x6a and i_opcode <= 0x78) or (i_opcode >= 0x7c and i_opcode <= 0x8a)) {
        // clang-format off
        अटल स्थिर u16 c_arithmeticCosts [] = {
            c_gasArith,  c_gasArith,  c_gasArith,                   // add, sub, mul
            c_gasDivide, c_gasDivide, c_gasDivide, c_gasDivide,     // div_s, div_u, rem_s, rem_u
            c_gasArith,  c_gasArith,  c_gasArith,                   // and, or, xor
            c_gasShift,  c_gasShift,  c_gasShift,                   // shl, shr_s, shr_u
            c_gasBranch, c_gasBranch                                // rotl, rotr
        };
        // clang-format on

        वापस c_arithmeticCosts[i_opcode - (i_opcode <= 0x78 ? 0x6a : 0x7c)];
    }

    // the atomic accesses are memory accesses; what waits, wakes or orders is worth
    // as much as an operation over a whole region
    अगर (i_opcode >= (c_waOp_atomic << 8) and i_opcode <= ((c_waOp_atomic << 8) | c_waOp_lastAtomic)) {
        वापस (i_opcode <= c_waOp_atomicFence) ? c_gasHeavy : c_gasAccess;
    }
    अगर (i_opcode >= 0x20 and i_opcode <= 0x3e) {
        वापस c_gasAccess;           // locals, globals, table accesses, loads and stores
    }
    अगर (i_opcode >= 0x41 and i_opcode <= 0x44) {
        वापस c_gasNominal;          // i32/i64/f32/f64.const
    }
    अगर (i_opcode >= 0x8b and i_opcode <= 0xa6) {
        // the float run, priced as integer arithmetic except where it divides
        bool divides = (i_opcode == 0x91 or i_opcode == 0x95 or     // f32.sqrt, f32.div
                        i_opcode == 0x9f or i_opcode == 0xa3);      // f64.sqrt, f64.div

        वापस divides ? c_gasDivide : c_gasArith;
    }

    // comparisons and clz/ctz/popcnt (0x45..0x69, 0x79..0x7b), the conversions
    // and sign extensions (0xa7..0xc4), the saturating truncations, the wide
    // arithmetic, and anything else that reaches here: plain arithmetic
    वापस c_gasArith;
}

// The instructions ewasm's design segments a body at. A segment runs up to and
// including the next one of these, so every cycle a body can execute - a loop's
// back edge, a branch out of a block, falling off the end of a function - passes
// through at least one, and no path can run unmetered however it is entered.
अटल
bool IsGasSegmentEnd (m3opcode_t i_opcode)
{
    चयन (i_opcode) {
    हाल c_waOp_loop:
    हाल c_waOp_if:
    हाल c_waOp_else:
    हाल c_waOp_end:
    हाल c_waOp_branch:
    हाल c_waOp_branchIf:
    हाल c_waOp_branchTable:
    हाल c_waOp_return:
    हाल c_waOp_returnCall:
    हाल c_waOp_returnCallIndirect:
    हाल c_waOp_returnCallRef:
        वापस true;
    शेष:
        वापस false;
    }
}

// Ends the open segment by writing what it accumulated into the immediate its
// op_UseGas is still waiting on. Emitting onto a later code page does not move
// an earlier one, so the reservation is still where it was left.
अटल
व्योम CloseGasSegment (IM3Compilation o)
{
    अगर (o->isGasSegmentOpen) {
        अगर (o->gasPatch) {
            u32 cost = o->gasCost;
            स_नकल(o->gasPatch, &cost, माप(cost));

            o->gasPatch = शून्य;
        }
        o->gasCost          = 0;
        o->isGasSegmentOpen = false;
    }
}

// Whether this compilation emits the charges
अटल अंतरभूत
bool IsMetering (IM3Compilation o)
{
    वापस o->runtime->gasLimit != 0;
}

// Charges one instruction to the segment being compiled, opening a segment at
// this point in the code stream if none is open, and settling the segment when
// the instruction is one that ends it. Called before the instruction is
// compiled, which is what both halves need: the op_UseGas lands ahead of what it
// pays for, and the cost is final before a block-structured instruction - loop,
// if, else - compiles its body from inside its own compiler. The body then opens
// a segment of its own, inside the loop or the arm, where it is charged on every
// pass rather than once on the way in.
अटल
M3_परिणाम MeterOpcode (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_परिणाम परिणाम = m3Err_none;

    // constant expressions are not metered: they run once at instantiation,
    // before the module is anything a gas budget was handed out for
    अगर (o->function and o->page and IsMetering(o)) {
        // only the 0xFC or 0xFE prefix has been read so far; the instruction it names is
        // the LEB128 u32 the compiler is about to take. Peek it without consuming.
        // Anything the tables do not cover leaves the prefix priced on its own,
        // which is moot - the compile fails on it a moment later.
        अगर (i_opcode == c_waOp_extended or i_opcode == c_waOp_atomic) {
            bytes_t peek = o->wasm;
            u32     sub;
            u32     last = (i_opcode == c_waOp_atomic) ? c_waOp_lastAtomic : c_waOp_lastExtended;

            अगर (not ReadLEB_u32(&sub, &peek, o->wasmEnd) and sub <= last) {
                i_opcode = (m3opcode_t)((i_opcode << 8) | sub);
            }
        }

        u32 cost = GetGasCost(i_opcode);

        अगर (o->isGasSegmentOpen and o->gasCost > d_m3MaxSegmentGas - cost) {
            CloseGasSegment(o);
        }

        अगर (not o->isGasSegmentOpen) {
            o->isGasSegmentOpen = true;

_           (EmitOp(o, op_UseGas));
            o->gasPatch = (व्योम*)GetPagePC(o->page);
            EmitConstant32(o, 0);
        }

        o->gasCost += cost;

        अगर (IsGasSegmentEnd(i_opcode)) {
            CloseGasSegment(o);
        }
    }

    _catch: वापस परिणाम;
}

#पूर्ण_अगर // d_m3HasGasMetering


M3_परिणाम CompileBlockStatements (IM3Compilation o)
{
    M3_परिणाम परिणाम   = m3Err_none;
    bool     validEnd = false;

    जबतक (o->wasm < o->wasmEnd) {
#अगर d_m3EnableOpTracing
        अगर (o->अंक_Emits) {
            EmitOp(o, op_Dump_ढेर);
            EmitConstant32(o, o->अंक_Opcodes);
            EmitConstant32(o, GetMaxUsedSlotPlusOne(o));
            EmitPointer(o, o->function);

            o->अंक_Emits = 0;
        }
#पूर्ण_अगर

        m3opcode_t opcode;
        o->lastOpcodeStart = o->wasm;
_       (Read_opcode(&opcode, &o->wasm, o->wasmEnd));
        log_opcode(o, opcode);

        // Restrict opcodes when evaluating expressions
        अगर (M3_UNLIKELY(not o->function)) {
            // clang-format off
            चयन (opcode) {
            हाल c_waOp_i32_const: हाल c_waOp_i64_const:
            हाल c_waOp_f32_const: हाल c_waOp_f64_const:
            हाल c_waOp_getGlobal: हाल c_waOp_end:
#अगर d_m3HasRefTypes
            हाल c_waOp_refNull:   हाल c_waOp_refFunc:
#पूर्ण_अगर
#अगर d_m3HasExtendedConst
            हाल c_waOp_i32_add:   हाल c_waOp_i32_sub:   हाल c_waOp_i32_mul:
            हाल c_waOp_i64_add:   हाल c_waOp_i64_sub:   हाल c_waOp_i64_mul:
#पूर्ण_अगर
                अवरोध;
            शेष:
                _throw(m3Err_restrictedOpcode);
            }
            // clang-format on
        }

        IM3OpInfo opinfo = GetOpInfo(opcode);

        अगर (M3_UNLIKELY(opinfo == शून्य)) {
            _throw(ErrorCompile(m3Err_unknownOpcode, o, "opcode '%x' not available", opcode));
        }

#अगर d_m3HasGasMetering
_       (MeterOpcode(o, opcode));
#पूर्ण_अगर

        अगर (opinfo->compiler != c_cc_NULL) {
_           ((*GetCompiler(opinfo))(o, opcode))
        } अन्यथा {
_           (Compile_Operator(o, opcode));
        }

        o->previousOpcode = opcode;

        अगर (opcode == c_waOp_else) {
            _throwif(m3Err_wasmMalformed, o->block.opcode != c_waOp_if);
            validEnd = true;
            अवरोध;
        } अन्यथा अगर (opcode == c_waOp_end) {
            validEnd = true;
            अवरोध;
        }
    }
    _throwif(m3Err_wasmMalformed, not validEnd);

_catch:
    वापस परिणाम;
}

अटल
M3_परिणाम धकेलो_Block_Results (IM3Compilation o)
{
    M3_परिणाम परिणाम = m3Err_none;

    u16 अंक_Results = Get_Func_Type_अंक_Results(o->block.type);

    क्रम (u16 i = 0; i < अंक_Results; ++i) {
        m3type_t type = Get_Func_Type_परिणाम_Type(o->block.type, i);

        अगर (i == अंक_Results - 1 and IsFpType(type)) {
_           (धकेलो_Register(o, type));
        } अन्यथा {
_           (धकेलो_Allocated_Slot(o, type));
        }
    }

    _catch: वापस परिणाम;
}


M3_परिणाम CompileBlock (IM3Compilation o, IM3FuncType i_blockType, m3opcode_t i_blockOpcode)
{
                                                                                        d_m3Assert (not IsRegisterAllocated (o, 0));
                                                                                        d_m3Assert (not IsRegisterAllocated (o, 1));
    InvalidateFold(o);

    M3CompilationScope  outerScope = o->block;
    M3CompilationScope* block      = &o->block;

    block->outer   = &outerScope;
    block->pc      = GetPagePC(o->page);
    block->patches = शून्य;
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
#अगर d_m3HasSnapshots
    // op_Loop's frame records the body, which is also the loop's identity;
    // op_TryTable's records its clause table, two words a clause back from it
    अगर (i_blockOpcode == c_waOp_loop) {
_       (RecordBlockStart(o, i_blockOpcode, block->pc, 0));
    }
#  अगर d_m3HasExceptionHandling
    अन्यथा अगर (i_blockOpcode == c_waOp_tryTable) {
_       (RecordBlockStart(o, i_blockOpcode, block->pc - 2 * o->अंक_Try_Clauses, o->अंक_Try_Clauses));
    }
#  endif
#पूर्ण_अगर

    // validate and dealloc params ----------------------------

    u16 ढेर_सूचकांक = o->ढेर_सूचकांक;

    u16 अंक_Params = Get_Func_Type_अंक_Params(i_blockType);

    अगर (i_blockOpcode != c_waOp_else) {
        क्रम (u16 i = 0; i < अंक_Params; ++i) {
            m3type_t type = GetFuncTypeParamType(i_blockType, अंक_Params - 1 - i);
_           (निकालो_Type(o, type));
        }
    } अन्यथा {
        अगर (Is_ढेर_Polymorphic(o) && o->block.block_ढेर_सूचकांक + अंक_Params > o->ढेर_सूचकांक) {
            o->ढेर_सूचकांक = o->block.block_ढेर_सूचकांक;
        } अन्यथा {
            o->ढेर_सूचकांक -= अंक_Params;
        }
    }

    u16 param_सूचकांक        = o->ढेर_सूचकांक;
    block->exit_ढेर_सूचकांक = param_सूचकांक; // consume the params at block exit

    // keep copies of param slots in the stack
    o->ढेर_सूचकांक = ढेर_सूचकांक;

    // find slots for the results ----------------------------
_   (धकेलो_Block_Results(o));

    ढेर_सूचकांक = o->ढेर_सूचकांक;

    // dealloc but keep record of the result slots in the stack
    u16 अंक_Results = Get_Func_Type_अंक_Results(i_blockType);
    जबतक (अंक_Results--) {
        निकालो(o);
    }

    block->block_ढेर_सूचकांक = o->ढेर_सूचकांक = ढेर_सूचकांक;

    // push the params back onto the stack -------------------
    क्रम (u16 i = 0; i < अंक_Params; ++i) {
        m3type_t type = GetFuncTypeParamType(i_blockType, i);

        u16 slot = Get_Slot_For_ढेर_सूचकांक(o, param_सूचकांक + i);
        धकेलो(o, type, slot);

        अगर (slot >= o->slot_First_Dynamic_सूचकांक && slot != c_slotUnused) {
_           (MarkSlotsAllocatedByType(o, slot, type));
        }
    }

    //--------------------------------------------------------

#अगर d_m3HasExceptionHandling
    // with the scope pushed and the params back on, a catch label of 0 names
    // this block - which is what the proposal says it should
    अगर (i_blockOpcode == c_waOp_tryTable) {
_       (EmitCatchStubs(o));
    }
#पूर्ण_अगर

_   (CompileBlockStatements(o));

_   (ValidateBlockEnd(o));

    अगर (o->function)    // skip for expressions
    {
        अगर (not Is_ढेर_Polymorphic(o)) {
_           (ResolveBlockResults(o, &o->block, /* isBranch: */ false));
        }

#अगर d_m3HasExceptionHandling
        // Falling out of the end of a try region retires its handler. This sits
        // ahead of PatchBranches so branches into the block's exit land past it:
        // a branch out of the try popped its own handler at the branch site.
        अगर (i_blockOpcode == c_waOp_tryTable) {
_           (EmitOp(o, op_निकालो_Handlers));
            EmitConstant32(o, 1);
        }
#पूर्ण_अगर

_       (Unwind_Block_ढेर(o))

        अगर (not ((i_blockOpcode == c_waOp_if and अंक_Results) or o->previousOpcode == c_waOp_else)) {
            o->ढेर_सूचकांक = o->block.exit_ढेर_सूचकांक;
_           (धकेलो_Block_Results(o));
        }
    }

    PatchBranches(o);

#अगर d_m3HasSnapshots
    RecordBlockEnd(o);

    अगर (o->localInitDepth) {
        क्रम (u32 i = 0; i < o->snapshotMap->अंक_Locals; ++i) {
            अगर (o->localInitDepth[i] >= (u32)o->block.depth) {
                o->localInitDepth[i] = UINT32_MAX;
            }
        }
    }
#पूर्ण_अगर
    o->block = outerScope;

} _catch: वापस परिणाम;
}

अटल
M3_परिणाम CompileLocals (IM3Compilation o)
{
    M3_परिणाम परिणाम;

    u32 अंक_Locals = 0;
    u32 अंक_Local_Blocks;
_   (ReadLEB_u32(&अंक_Local_Blocks, &o->wasm, o->wasmEnd));

    क्रम (u32 l = 0; l < अंक_Local_Blocks; ++l) {
        u32      var_गिनती;
        m3type_t localType;

_       (ReadLEB_u32(&var_गिनती, &o->wasm, o->wasmEnd));
_       (Parse_मूल्य_Type(o->module, &localType, &o->wasm, o->wasmEnd));
        अंक_Locals += var_गिनती;                                                          m3log (compile, "pushing locals. count: %d; type: %s", var_गिनती, c_waTypes [BaseTypeOf(localType)]);
        जबतक (var_गिनती--) {
_           (धकेलो_Allocated_Slot(o, localType));
        }
    }

    अगर (o->function) {
        o->function->अंक_Locals = अंक_Locals;
    }

    _catch: वापस परिणाम;
}

अटल
M3_परिणाम ReserveConstants (IM3Compilation o)
{
    M3_परिणाम परिणाम = m3Err_none;

    // in the interest of speed, this blindly scans the Wasm code looking for any byte
    // that looks like an const opcode.
    u16 अंक_Constant_Slots = 0;

    bytes_t wa = o->wasm;
    जबतक (wa < o->wasmEnd) {
        u8  code     = *wa++;
        u16 addSlots = 0;

        अगर (code == c_waOp_i32_const or code == c_waOp_f32_const) {
            addSlots = 1;
        } अन्यथा अगर (code == c_waOp_i64_const or code == c_waOp_f64_const) {
            addSlots = Get_Type_अंक_Slots(c_m3Type_i64);
        }

        अगर (अंक_Constant_Slots + addSlots >= d_m3_Max_Constant_Table_आकार) {
            अवरोध;
        }

        अंक_Constant_Slots += addSlots;
    }

    // if constants overflow their reserved stack space, the compiler simply emits op_Const
    // operations as needed. Compiled expressions (global inits) don't pass through this
    // ReserveConstants function and thus always produce inline constants.

    AlignSlotToType(&अंक_Constant_Slots, c_m3Type_i64);                                           m3log (compile, "reserved constant slots: %d", अंक_Constant_Slots);

    o->slot_First_Dynamic_सूचकांक = o->slot_First_Const_सूचकांक + अंक_Constant_Slots;

    अगर (o->slot_First_Dynamic_सूचकांक >= d_m3MaxFunctionSlots) {
        _throw(m3_Err_function_ढेर_Overflow);
    }

    _catch:
    वापस परिणाम;
}


// A constant expression compiles like a miniature function root block: no args,
// no locals and a single result, which Compile_End copies down into slot 0 for
// EvaluateExpression to read back.
M3_परिणाम CompileExpression (IM3Compilation o, IM3FuncType i_परिणाम_Type)
{
    M3_परिणाम परिणाम = m3Err_none;

    o->block.type = i_परिणाम_Type;

    u16 अंक_Ret_Slots = Get_Func_Type_अंक_Results(i_परिणाम_Type) * c_io_Slot_गिनती;

    क्रम (u16 i = 0; i < अंक_Ret_Slots; ++i) {
        MarkSlotAllocated(o, i);
    }

    o->max_ढेर_Slots = o->slot_Max_Allocated_सूचकांक_Plus_One = o->slot_First_Dynamic_सूचकांक = अंक_Ret_Slots;

_   (CompileBlockStatements(o));

    _catch: वापस परिणाम;
}


M3_परिणाम CompileFunction (IM3Function io_function)
{
    अगर (!io_function->wasm) {
        // An import. Either linking pointed it at another module's function -
        // in which case that one carries the body - or a host function was
        // bound to it, or it is simply unsatisfied.
        IM3Function impl = Function_Implementation(io_function);

        अगर (impl != io_function) {
            अगर (not impl->compiled) {
                M3_परिणाम r = CompileFunction(impl);
                अगर (r) {
                    वापस r;
                }
            }

            // keep the placeholder callable too, for anything still holding it
            io_function->compiled = impl->compiled;
            वापस m3Err_none;
        }

        अगर (io_function->compiled) {
            वापस m3Err_none;
        }

        वापस ErrorModule(m3Err_functionImportMissing, io_function->module, "'%s.%s'",
                           GetFunctionImportModuleName(io_function),
                           m3_GetFunctionName(io_function));
    }

#अगर d_m3EnableValidation
    अगर (not io_function->module->runtime->skipValidation) {
        M3_परिणाम vr = ValidateFunction(io_function);
        अगर (vr) {
            वापस vr;
        }
    }
#पूर्ण_अगर

    IM3FuncType funcType = io_function->funcType;                   m3log (compile, "compiling: [%d] %s %s; wasm-size: %d",
                                                                        io_function->index, m3_GetFunctionName (io_function), SPrintFuncTypeSignature (funcType), (u32) (io_function->wasmEnd - io_function->wasm));
    IM3Runtime runtime = io_function->module->runtime;

    IM3Compilation o = &runtime->compilation;                       d_m3Assert (d_m3MaxFunctionSlots >= d_m3_Max_Function_ढेर_Height * (d_m3Use32BitSlots + 1))  // need twice as many slots in 32-bit mode
    स_रखो(o, 0x0, माप(M3Compilation));

    o->runtime    = runtime;
    o->module     = io_function->module;
    o->function   = io_function;
    o->wasm       = io_function->wasm;
    o->wasmEnd    = io_function->wasmEnd;
    o->block.type = funcType;

_try {
    // skip over code size. the end was already calculated during parse phase
    u32 आकार;
_   (ReadLEB_u32(&आकार, &o->wasm, o->wasmEnd));                     d_m3Assert (आकार == (o->wasmEnd - o->wasm))

#अगर d_m3HasSnapshots
    o->bodyStart = o->wasm;
#पूर्ण_अगर

#अगर d_m3HasSnapshots
    // only code that can be suspended can end up in a snapshot
    अगर (runtime->isSuspendable) {
        o->snapshotMap = m3_AllocStruct(M3SnapshotMap);
        _throwifnull(o->snapshotMap);
    }
#पूर्ण_अगर

_   (AcquireCompilationCodePage(o, &o->page));

    pc_t pc = GetPagePC(o->page);

    u16 अंक_Ret_Slots = Get_Function_अंक_Returns(o->function) * c_io_Slot_गिनती;

    क्रम (u16 i = 0; i < अंक_Ret_Slots; ++i) {
        MarkSlotAllocated(o, i);
    }

    o->function->अंक_Ret_Slots = o->slot_First_Dynamic_सूचकांक = अंक_Ret_Slots;

    u16 अंक_Args = Get_Function_अंक_Args(o->function);

    // push the arg types to the type stack
    क्रम (u16 i = 0; i < अंक_Args; ++i) {
        m3type_t type = GetFunctionArgType(o->function, i);
_       (धकेलो_Allocated_Slot(o, type));

        // prevent allocator fill-in
        o->slot_First_Dynamic_सूचकांक += c_io_Slot_गिनती;
    }

    o->slot_Max_Allocated_सूचकांक_Plus_One = o->function->अंक_Ret_And_Arg_Slots = o->slot_First_Local_सूचकांक = o->slot_First_Dynamic_सूचकांक;

_   (CompileLocals(o));

#अगर d_m3HasSnapshots
_   (RecordLocals(o));
#पूर्ण_अगर

#अगर d_m3HasGasMetering
    // the table charges for a function's locals as well as for its instructions.
    // op_Entry is what sets them up, so the charge rides on the body's first
    // segment, which is emitted right after it
    o->gasCost = o->function->अंक_Locals * c_gasLocal;
#पूर्ण_अगर

    u16 maxSlot = GetMaxUsedSlotPlusOne(o);

    o->function->अंक_Local_Bytes = (maxSlot - o->slot_First_Local_सूचकांक) * माप(m3slot_t);

    o->slot_First_Const_सूचकांक = o->slot_Max_Const_सूचकांक = maxSlot;

    // ReserveConstants initializes o->firstDynamicSlotNumber
_   (ReserveConstants(o));

    // start tracking the max stack used (Push() also updates this value) so that op_Entry can precisely detect stack overflow
    o->max_ढेर_Slots = o->slot_Max_Allocated_सूचकांक_Plus_One = o->slot_First_Dynamic_सूचकांक;

    o->block.block_ढेर_सूचकांक = o->ढेर_First_Dynamic_सूचकांक = o->ढेर_सूचकांक;                           m3log (compile, "start stack index: %u; max stack slots: %u",
                                                                                                           (u32) o->ढेर_First_Dynamic_सूचकांक, (u32) o->max_ढेर_Slots);
#अगर d_m3_Has_ढेर_Switching
    // A suspendable body opens by checking for a pause request, once its frame
    // is set up. The pause point's offset is the first instruction's, just past
    // the local declarations - the body's end, when it has no other.
    अगर (runtime->isSuspendable) {
        o->lastOpcodeStart = o->wasm;
#  अगर d_m3EntryKeepsFrame
_       (EmitOp(o, op_Entry));
        EmitPointer(o, io_function);
_       (EmitOp(o, op_EntryCheck));
_       (RecordSafePoint(o, safepoint_entry, 1, 0, 0, शून्य));
#  अन्यथा
_       (EmitOp(o, op_Entry_Suspendable));
        EmitPointer(o, io_function);
_       (RecordSafePoint(o, safepoint_entry, 0, 0, 0, शून्य));
#  endif
    } अन्यथा
#पूर्ण_अगर
    {
_       (EmitOp(o, op_Entry));
        EmitPointer(o, io_function);
    }

_   (CompileBlockStatements(o));

    _throwif(m3Err_wasmMalformed, o->previousOpcode != c_waOp_end);

    io_function->compiled      = pc;
    io_function->max_ढेर_Slots = o->max_ढेर_Slots;

    u16 अंक_Constant_Slots = o->slot_Max_Const_सूचकांक - o->slot_First_Const_सूचकांक;                           m3log (compile, "unique constant slots: %u; unused slots: %u",
                                                                                                           अंक_Constant_Slots, o->slot_First_Dynamic_सूचकांक - o->slot_Max_Const_सूचकांक);
    io_function->अंक_Constant_Bytes = अंक_Constant_Slots * माप(m3slot_t);

    अगर (अंक_Constant_Slots) {
        io_function->constants = m3_CopyMem((cbytes_t)o->constants, io_function->अंक_Constant_Bytes);
        _throwifnull(io_function->constants);
    }

#अगर d_m3HasSnapshots
    io_function->snapshotMap = o->snapshotMap;
    o->snapshotMap           = शून्य;
#पूर्ण_अगर

} _catch:

    ReleaseCompilationCodePage(o);

#अगर d_m3HasSnapshots
    SnapshotMap_Free(o->snapshotMap);
    o->snapshotMap = शून्य;
    m3_Free(o->localInitDepth);
#पूर्ण_अगर

    वापस परिणाम;
}

#अगर d_m3_Has_ढेर_Switching
m3ret_t ResumeContinuation (IM3Runtime i_runtime, IM3Continuation i_cont)
{
    अगर (M3_UNLIKELY(!i_cont || i_cont->state != cont_suspended)) {
        वापस m3Err_none;
    }

    M3MemoryHeader* _mem = i_cont->entryFunction ? Module_MemoryHeader(i_cont->entryFunction->module) : शून्य;

    i_runtime->activeContinuation = i_cont;
    i_cont->state                 = cont_running;

    i32 outermost     = (i32)i_cont->अंक_Frames - 1;
    i_cont->अंक_Frames = 0;

    // with no frames standing, this lands straight on the suspension point
    m3ret_t r = ReplayFrames(i_cont, outermost, _mem);

    अगर (r == m3Err_continuationSuspended) {
        i_cont->state = cont_suspended;
    } अन्यथा {
        i_cont->state = cont_returned;
    }
    i_runtime->activeContinuation = शून्य;

    वापस r;
}
#पूर्ण_अगर
