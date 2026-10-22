//
//  m3_compile.c
//
//  Created by Steven Massey on 4/17/19.
//  Copyright © 2019 Steven Massey. All rights reserved.
//

// Allow using opcodes for compilation process
#تعریف_کن M3_COMPILE_OPCODES

#ضمن "m3_env.h"
#ضمن "m3_compile.h"
#ضمن "m3_exec.h"
#ضمن "m3_exception.h"
#ضمن "m3_info.h"
#ضمن "m3_validate.h"

// Resolves M3OpInfo.compiler, an index, to the function it names. Defined down
// with c_compilers [], which has to follow every compiler it points at.
ساكن
M3Compiler GetCompiler (IM3OpInfo i_opInfo);

//----- EMIT --------------------------------------------------------------------------------------------------------------

ساكن مضمن
pc_t GetPC (IM3Compilation o)
{
    أرجع Get_صفحه_PC(o->صفحه);
}

#إذا d_m3HasSnapshots

#  define d_m3GrowSnapshotArray(TYPE, ARRAY, شمار_2, ظرفیت_2)                    \
    إذا ((شمار_2) == (ظرفیت_2)) {                                                 \
        u32   new_ظرفیت = (ظرفیت_2) ? (ظرفیت_2) * 2 : 8;                     \
        TYPE* grown = m3_ReallocArray(TYPE, (ARRAY), new_ظرفیت, (ظرفیت_2));   \
        إذا (not grown) {                                                         \
            أرجع m3Err_mallocFailed;                                           \
        }                                                                        \
        (ARRAY)    = grown;                                                      \
        (ظرفیت_2) = new_ظرفیت;                                                \
    }

// Where the instruction being compiled starts, counted from the start of the
// function's body: the one position every build of every revision agrees on
ساكن مضمن
u32 Get_Wasm_جابه_جایی (IM3Compilation o)
{
    أرجع (u32)(o->آخرین_Opcode_شروع - o->body_شروع);
}

// Whether the operand stack entry at i_index is a value the frame holds. Every
// block keeps records of its parameter and result slots on the stack, between
// the height it was entered at and where its own values begin (see
// CompileBlock). Those are landing pads rather than values: nothing need have
// written them yet, and the slots they name may be in use for something else.
// The function's own scope has none - that stretch is its arguments and locals.
ساكن
bool Is_زنده_پشته_مدخل (IM3Compilation o, u16 i_شاخص)
{
    لكل (IM3CompilationScope scope = &o->بلوک; scope->outer; scope = scope->outer) {
        إذا (i_شاخص >= scope->exit_پشته_شاخص and i_شاخص < scope->بلوک_پشته_شاخص) {
            أرجع false;
        }
    }

    أرجع true;
}

ساكن
M3_نتیجه جمع_کن_امن_نقطه_مقدار (IM3Compilation o, M3_امن_نقطه* io_نقطه, u16 i_شاخص)
{
    M3SnapshotMap* map = o->snapshotMap;

    d_m3GrowSnapshotArray(M3_جایگاه_مقدار, map->مقادیر, map->رقم_مقادیر, o->مقادیر_ظرفیت);

    map->مقادیر[map->رقم_مقادیر].جایگاه = o->wasm_پشته[i_شاخص];
    map->مقادیر[map->رقم_مقادیر].type = o->type_پشته[i_شاخص];
    map->رقم_مقادیر++;
    io_نقطه->رقم_مقادیر++;

    أرجع m3Err_none;
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
ساكن
M3_نتیجه رکورد_امن_نقطه (IM3Compilation o, M3_امن_نقطه_نوع i_نوع, u32 i_واژگان_پشت, u16 i_رقم_Results,
                          u16 i_aux, IM3CompilationScope i_loop)
{
_try {
    M3SnapshotMap* map = o->snapshotMap;

    إذا (not map or not o->صفحه) {
        أرجع m3Err_none;
    }

    d_m3GrowSnapshotArray(M3_امن_نقطه, map->امن_Points, map->رقم_امن_Points, o->امن_Points_ظرفیت);

    // grown before it is pointed at: adding values never moves the safepoints
    M3_امن_نقطه* نقطه     = &map->امن_Points[map->رقم_امن_Points];
    نقطه->pc              = GetPC(o) - i_واژگان_پشت;
    نقطه->wasm_جابه_جایی      = Get_Wasm_جابه_جایی(o);
    نقطه->اولی_مقدار      = map->رقم_مقادیر;
    نقطه->اولی_Local_مقدار = UINT32_MAX;
    نقطه->رقم_مقادیر       = 0;
    نقطه->رقم_Results      = i_رقم_Results;
    نقطه->aux             = i_aux;
    نقطه->resumeType      = c_m3Type_none;
    نقطه->نوع            = (u8)i_نوع;
    نقطه->flags           = 0;

    // Non-defaultable locals can still be uninitialized at a safepoint. Keep
    // their heap types, but allow the null placeholder until local.set makes
    // them definitely initialized. Branches to a loop use its entry context.
    إذا (o->local_Init_عمق) {
        نقطه->اولی_Local_مقدار = map->رقم_مقادیر;
        لكل (u32 i = 0; i < map->رقم_Locals; ++i) {
            d_m3GrowSnapshotArray(M3_جایگاه_مقدار, map->مقادیر, map->رقم_مقادیر, o->مقادیر_ظرفیت);
            M3_جایگاه_مقدار مقدار = map->locals[i];
            إذا (o->local_Init_عمق[i] == UINT32_MAX or
                (i_loop and o->local_Init_عمق[i] >= (u32)i_loop->عمق)) {
                مقدار.type &= ~d_m3Type_refNonNull;
            }
            map->مقادیر[map->رقم_مقادیر++] = مقدار;
        }
        نقطه->اولی_مقدار = map->رقم_مقادیر;
    }

    u16 بالاترین = o->پشته_شاخص;

    إذا (i_نوع != safepoint_suspend) {
        بالاترین = (بالاترین > i_رقم_Results) ? (u16)(بالاترین - i_رقم_Results) : 0;
    }
    إذا (i_loop) {
        بالاترین = M3_MIN(بالاترین, i_loop->exit_پشته_شاخص);
    }

    لكل (u16 i = o->پشته_اولی_Dynamic_شاخص; i < بالاترین; ++i) {
        إذا (Is_زنده_پشته_مدخل(o, i)) {
_           (جمع_کن_امن_نقطه_مقدار(o, نقطه, i));
        }
    }

    إذا (i_loop) {
        u16 رقم_Params = Get_Func_Type_رقم_Params(i_loop->type);

        لكل (u16 i = 0; i < رقم_Params; ++i) {
_           (جمع_کن_امن_نقطه_مقدار(o, نقطه, (u16)(i_loop->exit_پشته_شاخص + i)));
        }
    }

    map->رقم_امن_Points++;

} _catch: أرجع نتیجه;
}

// A back edge: op_ContinueLoop, or op_ContinueLoopIf when i_isConditional, and
// the loop it goes back to. The conditional one suspends before it has taken
// the branch, so it has to be resumed with the condition that took it.
ساكن
M3_نتیجه رکورد_پشت_یال (IM3Compilation o, IM3CompilationScope i_loop, bool i_isConditional)
{
    M3_نتیجه نتیجه = m3Err_none;

    // the loop is named by its record in the map, which has to fit the field
    _throwif("a function has too many loops for its snapshot map",
             o->snapshotMap and o->صفحه and (i_loop->snapshot_بلوک == 0 or i_loop->snapshot_بلوک > UINT16_MAX + 1u));

_   (رکورد_امن_نقطه(o, safepoint_op, 2, 0, (u16)(i_loop->snapshot_بلوک - 1), i_loop));

    إذا (i_isConditional and o->snapshotMap and o->صفحه) {
        o->snapshotMap->امن_Points[o->snapshotMap->رقم_امن_Points - 1].flags |= d_m3_امن_نقطه_گرفته_Branch;
    }

    _catch: أرجع نتیجه;
}

// A call, waiting on its callee with i_numResults not yet written; the
// callee's frame starts i_calleeSlot slots above this one. A return_call that
// is compiled as a call is one too, since its frame stands while the callee
// runs, but it is marked: in Wasm the callee has replaced it, so a snapshot
// walks past it rather than writing it.
ساكن
M3_نتیجه رکورد_Call (IM3Compilation o, u16 i_رقم_Results, u16 i_callee_جایگاه, bool i_isReturnCall)
{
    M3_نتیجه نتیجه = رکورد_امن_نقطه(o, safepoint_call, 0, i_رقم_Results, i_callee_جایگاه, عدم);

    إذا (not نتیجه and i_isReturnCall and o->snapshotMap and o->صفحه) {
        o->snapshotMap->امن_Points[o->snapshotMap->رقم_امن_Points - 1].flags |= d_m3_امن_نقطه_دم_Call;
    }

    أرجع نتیجه;
}

// Where the function's arguments and locals are kept. Unlike the operand
// stack, that never changes, so it is recorded once rather than at every
// safepoint.
ساكن
M3_نتیجه رکورد_Locals (IM3Compilation o)
{
    M3SnapshotMap* map = o->snapshotMap;

    إذا (map and o->پشته_شاخص) {
        map->locals = m3_AllocArray(M3_جایگاه_مقدار, o->پشته_شاخص);

        إذا (not map->locals) {
            أرجع m3Err_mallocFailed;
        }

        لكل (u16 i = 0; i < o->پشته_شاخص; ++i) {
            map->locals[i].جایگاه = o->wasm_پشته[i];
            map->locals[i].type = o->type_پشته[i];
        }

        map->رقم_Locals = o->پشته_شاخص;
        لكل (u32 i = Get_Function_رقم_آرگومانها(o->function); i < map->رقم_Locals; ++i) {
            إذا (IsRefType(map->locals[i].type) and not IsNullableRef(map->locals[i].type)) {
                o->local_Init_عمق = m3_AllocArray(u32, map->رقم_Locals);
                إذا (not o->local_Init_عمق) {
                    أرجع m3Err_mallocFailed;
                }
                لكل (u32 j = i; j < map->رقم_Locals; ++j) {
                    إذا (IsRefType(map->locals[j].type) and not IsNullableRef(map->locals[j].type)) {
                        o->local_Init_عمق[j] = UINT32_MAX;
                    }
                }
                اكسر;
            }
        }
    }

    أرجع m3Err_none;
}

// A loop or a try_table, whose native frame a snapshot names by the
// instruction's offset. i_pc is what that frame records. The scope being
// compiled keeps the record, so that RecordBlockEnd can close it.
ساكن
M3_نتیجه رکورد_بلوک_شروع (IM3Compilation o, m3opcode_t i_opcode, pc_t i_pc, u32 i_رقم_Clauses)
{
    M3SnapshotMap* map = o->snapshotMap;

    إذا (map and o->صفحه) {
        d_m3GrowSnapshotArray(M3_بلوک_شروع, map->بلوکها, map->رقم_بلوکها, o->بلوکها_ظرفیت);

        M3_بلوک_شروع* بلوک = &map->بلوکها[map->رقم_بلوکها++];
        بلوک->pc           = i_pc;
        بلوک->wasm_جابه_جایی   = Get_Wasm_جابه_جایی(o);
        بلوک->wasm_انتها      = UINT32_MAX;
        بلوک->رقم_Clauses   = (u16)i_رقم_Clauses;
        بلوک->opcode       = (u8)i_opcode;

        o->بلوک.snapshot_بلوک = map->رقم_بلوکها;
    }

    أرجع m3Err_none;
}

// Closes the scope's region, while the instruction just read is its end
ساكن
فراغ رکورد_بلوک_انتها (IM3Compilation o)
{
    إذا (o->snapshotMap and o->بلوک.snapshot_بلوک) {
        o->snapshotMap->بلوکها[o->بلوک.snapshot_بلوک - 1].wasm_انتها = Get_Wasm_جابه_جایی(o);
    }
}

#  undef d_m3GrowSnapshotArray

#وإلا

ساكن مضمن
M3_نتیجه رکورد_امن_نقطه (IM3Compilation o, M3_امن_نقطه_نوع i_نوع, u32 i_واژگان_پشت, u16 i_رقم_Results,
                          u16 i_aux, IM3CompilationScope i_loop)
{
    (فراغ)o;
    (فراغ)i_نوع;
    (فراغ)i_واژگان_پشت;
    (فراغ)i_رقم_Results;
    (فراغ)i_aux;
    (فراغ)i_loop;
    أرجع m3Err_none;
}

ساكن مضمن
M3_نتیجه رکورد_پشت_یال (IM3Compilation o, IM3CompilationScope i_loop, bool i_isConditional)
{
    (فراغ)o;
    (فراغ)i_loop;
    (فراغ)i_isConditional;
    أرجع m3Err_none;
}

ساكن مضمن
M3_نتیجه رکورد_Call (IM3Compilation o, u16 i_رقم_Results, u16 i_callee_جایگاه, bool i_isReturnCall)
{
    (فراغ)o;
    (فراغ)i_رقم_Results;
    (فراغ)i_callee_جایگاه;
    (فراغ)i_isReturnCall;
    أرجع m3Err_none;
}

#پایان_اگر // d_m3HasSnapshots

ساكن M3_NOINLINE
M3_نتیجه Ensure_کد_صفحه_رقم_سطرها (IM3Compilation o, u32 i_رقم_سطرها)
{
    M3_نتیجه نتیجه = m3Err_none;

    i_رقم_سطرها += 2; // room for Bridge

    إذا (M3_UNLIKELY(رقم_Free_سطرها(o->صفحه) < i_رقم_سطرها)) {
        IM3_کد_صفحه صفحه = Acquire_کد_صفحه_With_ظرفیت(o->runtime, i_رقم_سطرها);

        إذا (صفحه) {
            m3log(emit, "bridging new code page from: %d %p (free slots: %d) to: %d", o->صفحه->info.sequence, GetPC(o), رقم_Free_سطرها(o->صفحه), صفحه->info.sequence);
            d_m3Assert(رقم_Free_سطرها(o->صفحه) >= 2);

            Emit_کلمه(o->صفحه, op_Branch);
            Emit_کلمه(o->صفحه, Get_صفحه_PC(صفحه));

            Release_کد_صفحه(o->runtime, o->صفحه);

            o->صفحه = صفحه;
        } وإلا {
            نتیجه = m3_Err_malloc_Failed_کد_صفحه;
        }
    }

    أرجع نتیجه;
}

// invalidate the pending local.set fold candidate; called wherever code is emitted or a
// branch target captures the current pc (a fold appends an immediate, moving that position)
ساكن مضمن
فراغ InvalidateFold (IM3Compilation o)
{
#إذا d_m3_Fold_مجموعه_Local
    o->foldPatchPC = عدم;
#وإلا
    (فراغ)o;
#پایان_اگر
}

ساكن M3_NOINLINE
M3_نتیجه EmitOp (IM3Compilation o, IM3Operation i_operation)
{
    M3_نتیجه نتیجه = m3Err_none;                                 d_m3Assert (i_operation or Is_پشته_Polymorphic (o));

    InvalidateFold(o);

    // it's OK for page to be null; when compile-walking the bytecode without emitting
    إذا (o->صفحه) {
#إذا d_m3EnableOpTracing
        إذا (i_operation != op_Dump_پشته) {
            o->رقم_Emits++;
        }
#پایان_اگر

        // have execution jump to a new page if slots are critically low
        نتیجه = Ensure_کد_صفحه_رقم_سطرها(o, d_m3_کد_صفحه_Free_سطرها_آستانه);

        إذا (M3_LIKELY(not نتیجه)) {
            إذا (d_m3LogEmit) {
                log_emit(o, i_operation);
            }
#إذا d_m3_رکورد_Backtraces
            Emit_Mapping_مدخل(o->صفحه, (u32)(o->آخرین_Opcode_شروع - o->module->wasm_شروع));
#پایان_اگر // d_m3RecordBacktraces
            Emit_کلمه(o->صفحه, i_operation);
        }
    }

    أرجع نتیجه;
}

// Push an immediate constant into the M3 codestream
ساكن M3_NOINLINE
فراغ EmitConstant32 (IM3Compilation o, ثابت u32 i_فوری)
{
    إذا (o->صفحه) {                                                          m3log (emit, "const32: %ud", i_فوری);
        EmitWord32(o->صفحه, i_فوری);
    }
}

#إذا d_m3HasMemory64
// Takes two lines of the code page where a pointer is 32 bits, so whatever
// reads it back has to step _pc by the same amount - see op_Const64.
ساكن M3_NOINLINE
فراغ EmitConstant64 (IM3Compilation o, ثابت u64 i_فوری)
{
    إذا (o->صفحه) {
        EmitWord64(o->صفحه, i_فوری);
    }
}
#پایان_اگر

ساكن M3_NOINLINE
فراغ Emit_جایگاه_جابه_جایی (IM3Compilation o, ثابت i32 i_جابه_جایی)
{
    إذا (o->صفحه) {                                                          m3log (emit, "slot: [%d]", i_جابه_جایی);
        EmitWord32(o->صفحه, i_جابه_جایی);
    }
}

ساكن M3_NOINLINE
pc_t Emit_اشاره_گر (IM3Compilation o, ثابت فراغ* ثابت i_اشاره_گر)
{
    pc_t ptr = Get_صفحه_PC(o->صفحه);

    إذا (o->صفحه) {                                                          m3log (emit, "ptr: %p", i_اشاره_گر);
        Emit_کلمه(o->صفحه, i_اشاره_گر);
    }

    أرجع ptr;
}

ساكن M3_NOINLINE
فراغ* Reserve_اشاره_گر (IM3Compilation o)
{
    pc_t ptr = Get_صفحه_PC(o->صفحه);
    Emit_اشاره_گر(o, عدم);
    أرجع (فراغ*)ptr;
}


//-------------------------------------------------------------------------------------------------------------------------

#تعریف_کن d_indent "     | %s"

// just want less letters and numbers to stare at down the way in the compiler table
#تعریف_کن i_32    c_m3Type_i32
#تعریف_کن i_64    c_m3Type_i64
#تعریف_کن f_32    c_m3Type_f32
#تعریف_کن f_64    c_m3Type_f64
#تعریف_کن none    c_m3Type_none
#تعریف_کن any     (u8)-1

#إذا d_m3HasFloat
#  define FPOP(x) x
#وإلا
#  define FPOP(x) عدم
#پایان_اگر

// These are indexed by M3ValueType, so every type up to c_m3Type_externref needs
// an entry. A reference is one pointer-sized word, so it moves with the integer
// operation of that width; v128 has no operations at all.
#إذا M3_SIZEOF_PTR == 8
#  define REFOP(NAME)  op_##NAME##_i64
#وإلا
#  define REFOP(NAME)  op_##NAME##_i32
#پایان_اگر

// clang-format off

ساكن ثابت IM3Operation c_preserve_مجموعه_جایگاه[] = { عدم, op_Preserve_مجموعه_جایگاه_i32,       op_Preserve_مجموعه_جایگاه_i64,
                                                    FPOP(op_Preserve_مجموعه_جایگاه_f32), FPOP(op_Preserve_مجموعه_جایگاه_f64),
                                                    عدم, REFOP(Preserve_مجموعه_جایگاه),  REFOP(Preserve_مجموعه_جایگاه),
                                                    REFOP(Preserve_مجموعه_جایگاه),       REFOP(Preserve_مجموعه_جایگاه) };
ساكن ثابت IM3Operation c_مجموعه_مجموعه_Ops[] =       { عدم, op_مجموعه_جایگاه_i32,               op_مجموعه_جایگاه_i64,
                                                    FPOP(op_مجموعه_جایگاه_f32),         FPOP(op_مجموعه_جایگاه_f64),
                                                    عدم, REFOP(مجموعه_جایگاه),         REFOP(مجموعه_جایگاه),
                                                    REFOP(مجموعه_جایگاه),               REFOP(مجموعه_جایگاه) };
ساكن ثابت IM3Operation c_مجموعه_Global_Ops[] =    { عدم, op_مجموعه_Global_i32,             op_مجموعه_Global_i64,
                                                    FPOP(op_مجموعه_Global_f32),       FPOP(op_مجموعه_Global_f64),
                                                    عدم, REFOP(مجموعه_Global),       REFOP(مجموعه_Global),
                                                    REFOP(مجموعه_Global),             REFOP(مجموعه_Global) };
ساكن ثابت IM3Operation c_مجموعه_ثبات_Ops[] =  { عدم, op_مجموعه_ثبات_i32,           op_مجموعه_ثبات_i64,
                                                    FPOP(op_مجموعه_ثبات_f32),     FPOP(op_مجموعه_ثبات_f64),
                                                    عدم, REFOP(مجموعه_ثبات),     REFOP(مجموعه_ثبات),
                                                    REFOP(مجموعه_ثبات),           REFOP(مجموعه_ثبات) };

// A table shorter than the enum reads out of bounds, so tie the two together at
// build time: adding a value type must not silently outgrow these.
#تعریف_کن d_m3_Check_Type_جدول(جدول) \
    M3_STATIC_ASSERT (حجم (جدول) / حجم (*(جدول)) == c_m3_Type_شمار, جدول##_needs_one_entry_per_type)

d_m3_Check_Type_جدول(c_preserve_مجموعه_جایگاه);
d_m3_Check_Type_جدول(c_مجموعه_مجموعه_Ops);
d_m3_Check_Type_جدول(c_مجموعه_Global_Ops);
d_m3_Check_Type_جدول(c_مجموعه_ثبات_Ops);

#إذا d_m3_Fold_مجموعه_Local

// destination-folded variants of the hot binops, indexed by the operand-form index
// recorded at emission: [0]=_rs [1]=_sr [2]=_ss [3]=(fp _rr, never folded)
#تعریف_کن d_foldOpList(TYPE, NAME)            { op_##TYPE##_##NAME##_rs_f, op_##TYPE##_##NAME##_sr_f, op_##TYPE##_##NAME##_ss_f, عدم }
#تعریف_کن d_foldCommutativeOpList(TYPE, NAME) { op_##TYPE##_##NAME##_rs_f, عدم,                      op_##TYPE##_##NAME##_ss_f, عدم }

ساكن ثابت IM3Operation c_fold_i32_جمع_کن[]         = d_foldCommutativeOpList (i32, جمع_کن);
ساكن ثابت IM3Operation c_fold_i32_تفریق[]    = d_foldOpList (i32, تفریق);
ساكن ثابت IM3Operation c_fold_i32_Multiply[]    = d_foldCommutativeOpList (i32, Multiply);
ساكن ثابت IM3Operation c_fold_u32_And[]         = d_foldCommutativeOpList (u32, And);
ساكن ثابت IM3Operation c_fold_u32_Or[]          = d_foldCommutativeOpList (u32, Or);
ساكن ثابت IM3Operation c_fold_u32_Xor[]         = d_foldCommutativeOpList (u32, Xor);
ساكن ثابت IM3Operation c_fold_u32_لغزش_چپ[]   = d_foldOpList (u32, لغزش_چپ);
ساكن ثابت IM3Operation c_fold_i32_لغزش_راست[]  = d_foldOpList (i32, لغزش_راست);
ساكن ثابت IM3Operation c_fold_u32_لغزش_راست[]  = d_foldOpList (u32, لغزش_راست);

ساكن ثابت IM3Operation c_fold_i64_جمع_کن[]         = d_foldCommutativeOpList (i64, جمع_کن);
ساكن ثابت IM3Operation c_fold_i64_تفریق[]    = d_foldOpList (i64, تفریق);
ساكن ثابت IM3Operation c_fold_i64_Multiply[]    = d_foldCommutativeOpList (i64, Multiply);
ساكن ثابت IM3Operation c_fold_u64_And[]         = d_foldCommutativeOpList (u64, And);
ساكن ثابت IM3Operation c_fold_u64_Or[]          = d_foldCommutativeOpList (u64, Or);
ساكن ثابت IM3Operation c_fold_u64_Xor[]         = d_foldCommutativeOpList (u64, Xor);
ساكن ثابت IM3Operation c_fold_u64_لغزش_چپ[]   = d_foldOpList (u64, لغزش_چپ);
ساكن ثابت IM3Operation c_fold_i64_لغزش_راست[]  = d_foldOpList (i64, لغزش_راست);
ساكن ثابت IM3Operation c_fold_u64_لغزش_راست[]  = d_foldOpList (u64, لغزش_راست);

#  إذا d_m3HasFloat
ساكن ثابت IM3Operation c_fold_f32_جمع_کن[]         = d_foldCommutativeOpList (f32, جمع_کن);
ساكن ثابت IM3Operation c_fold_f32_تفریق[]    = d_foldOpList (f32, تفریق);
ساكن ثابت IM3Operation c_fold_f32_Multiply[]    = d_foldCommutativeOpList (f32, Multiply);
ساكن ثابت IM3Operation c_fold_f32_Divide[]      = d_foldOpList (f32, Divide);
ساكن ثابت IM3Operation c_fold_f64_جمع_کن[]         = d_foldCommutativeOpList (f64, جمع_کن);
ساكن ثابت IM3Operation c_fold_f64_تفریق[]    = d_foldOpList (f64, تفریق);
ساكن ثابت IM3Operation c_fold_f64_Multiply[]    = d_foldCommutativeOpList (f64, Multiply);
ساكن ثابت IM3Operation c_fold_f64_Divide[]      = d_foldOpList (f64, Divide);
#  endif

// loads are unary: [0]= address in _r0, [1]= address in a slot
#تعریف_کن d_fold_بارگیری_List(DEST, SRC)       { op_##DEST##_Load_##SRC##_r_f, op_##DEST##_Load_##SRC##_s_f, عدم, عدم }

ساكن ثابت IM3Operation c_fold_i32_بارگیری_i32[] = d_fold_بارگیری_List(i32, i32);
ساكن ثابت IM3Operation c_fold_i32_بارگیری_i8[] = d_fold_بارگیری_List(i32, i8);
ساكن ثابت IM3Operation c_fold_i32_بارگیری_u8[] = d_fold_بارگیری_List(i32, u8);
ساكن ثابت IM3Operation c_fold_i32_بارگیری_i16[] = d_fold_بارگیری_List(i32, i16);
ساكن ثابت IM3Operation c_fold_i32_بارگیری_u16[] = d_fold_بارگیری_List(i32, u16);

ساكن ثابت IM3Operation c_fold_i64_بارگیری_i64[] = d_fold_بارگیری_List(i64, i64);
ساكن ثابت IM3Operation c_fold_i64_بارگیری_i8[] = d_fold_بارگیری_List(i64, i8);
ساكن ثابت IM3Operation c_fold_i64_بارگیری_u8[] = d_fold_بارگیری_List(i64, u8);
ساكن ثابت IM3Operation c_fold_i64_بارگیری_i16[] = d_fold_بارگیری_List(i64, i16);
ساكن ثابت IM3Operation c_fold_i64_بارگیری_u16[] = d_fold_بارگیری_List(i64, u16);
ساكن ثابت IM3Operation c_fold_i64_بارگیری_i32[] = d_fold_بارگیری_List(i64, i32);
ساكن ثابت IM3Operation c_fold_i64_بارگیری_u32[] = d_fold_بارگیری_List(i64, u32);
#  إذا d_m3HasFloat
ساكن ثابت IM3Operation c_fold_f32_بارگیری_f32[] = d_fold_بارگیری_List(f32, f32);
ساكن ثابت IM3Operation c_fold_f64_بارگیری_f64[] = d_fold_بارگیری_List(f64, f64);
#  endif

ساكن
IM3Operation GetFoldOp (m3opcode_t i_opcode, u8 i_form)
{
    إذا (i_form >= 4) {
        أرجع عدم;
    }

    بدل (i_opcode) {
    حالة 0x6a:  أرجع c_fold_i32_جمع_کن        [i_form];      // i32.add
    حالة 0x6b:  أرجع c_fold_i32_تفریق   [i_form];      // i32.sub
    حالة 0x6c:  أرجع c_fold_i32_Multiply   [i_form];      // i32.mul
    حالة 0x71:  أرجع c_fold_u32_And        [i_form];      // i32.and
    حالة 0x72:  أرجع c_fold_u32_Or         [i_form];      // i32.or
    حالة 0x73:  أرجع c_fold_u32_Xor        [i_form];      // i32.xor
    حالة 0x74:  أرجع c_fold_u32_لغزش_چپ  [i_form];      // i32.shl
    حالة 0x75:  أرجع c_fold_i32_لغزش_راست [i_form];      // i32.shr_s
    حالة 0x76:  أرجع c_fold_u32_لغزش_راست [i_form];      // i32.shr_u

    حالة 0x7c:  أرجع c_fold_i64_جمع_کن        [i_form];      // i64.add
    حالة 0x7d:  أرجع c_fold_i64_تفریق   [i_form];      // i64.sub
    حالة 0x7e:  أرجع c_fold_i64_Multiply   [i_form];      // i64.mul
    حالة 0x83:  أرجع c_fold_u64_And        [i_form];      // i64.and
    حالة 0x84:  أرجع c_fold_u64_Or         [i_form];      // i64.or
    حالة 0x85:  أرجع c_fold_u64_Xor        [i_form];      // i64.xor
    حالة 0x86:  أرجع c_fold_u64_لغزش_چپ  [i_form];      // i64.shl
    حالة 0x87:  أرجع c_fold_i64_لغزش_راست [i_form];      // i64.shr_s
    حالة 0x88:  أرجع c_fold_u64_لغزش_راست [i_form];      // i64.shr_u

#  إذا d_m3HasFloat
    حالة 0x92:  أرجع c_fold_f32_جمع_کن        [i_form];      // f32.add
    حالة 0x93:  أرجع c_fold_f32_تفریق   [i_form];      // f32.sub
    حالة 0x94:  أرجع c_fold_f32_Multiply   [i_form];      // f32.mul
    حالة 0x95:  أرجع c_fold_f32_Divide     [i_form];      // f32.div
    حالة 0xa0:  أرجع c_fold_f64_جمع_کن        [i_form];      // f64.add
    حالة 0xa1:  أرجع c_fold_f64_تفریق   [i_form];      // f64.sub
    حالة 0xa2:  أرجع c_fold_f64_Multiply   [i_form];      // f64.mul
    حالة 0xa3:  أرجع c_fold_f64_Divide     [i_form];      // f64.div
#  endif

    حالة 0x28:  أرجع c_fold_i32_بارگیری_i32   [i_form];      // i32.load
    حالة 0x2c:  أرجع c_fold_i32_بارگیری_i8    [i_form];      // i32.load8_s
    حالة 0x2d:  أرجع c_fold_i32_بارگیری_u8    [i_form];      // i32.load8_u
    حالة 0x2e:  أرجع c_fold_i32_بارگیری_i16   [i_form];      // i32.load16_s
    حالة 0x2f:  أرجع c_fold_i32_بارگیری_u16   [i_form];      // i32.load16_u

    حالة 0x29:  أرجع c_fold_i64_بارگیری_i64   [i_form];      // i64.load
    حالة 0x30:  أرجع c_fold_i64_بارگیری_i8    [i_form];      // i64.load8_s
    حالة 0x31:  أرجع c_fold_i64_بارگیری_u8    [i_form];      // i64.load8_u
    حالة 0x32:  أرجع c_fold_i64_بارگیری_i16   [i_form];      // i64.load16_s
    حالة 0x33:  أرجع c_fold_i64_بارگیری_u16   [i_form];      // i64.load16_u
    حالة 0x34:  أرجع c_fold_i64_بارگیری_i32   [i_form];      // i64.load32_s
    حالة 0x35:  أرجع c_fold_i64_بارگیری_u32   [i_form];      // i64.load32_u

#  إذا d_m3HasFloat
    حالة 0x2a:  أرجع c_fold_f32_بارگیری_f32   [i_form];      // f32.load
    حالة 0x2b:  أرجع c_fold_f64_بارگیری_f64   [i_form];      // f64.load
#  endif

    افتراضي: أرجع عدم;
    }
}

#پایان_اگر // d_m3FoldSetLocal

// clang-format on

#إذا d_m3FuseBranch

// fused compare+branch/if variants, indexed like the fold lists: [0]=_rs [1]=_sr [2]=_ss [3]=unused.
// the [1] entries are NULL for commutative compares: the compiler never records the _sr form for them
#  define d_fuseCmpList(TYPE, NAME, نوع_2)             { op_##TYPE##_##KIND##_##NAME##_rs, op_##TYPE##_##KIND##_##NAME##_sr, op_##TYPE##_##KIND##_##NAME##_ss, عدم }
#  define d_fuseCommutativeCmpList(TYPE, NAME, نوع_2)  { op_##TYPE##_##KIND##_##NAME##_rs, عدم,                             op_##TYPE##_##KIND##_##NAME##_ss, عدم }

تعريفنوع بنية M3FusedCmpOps {
    IM3Operation branchIf[4];
    IM3Operation ifOp[4];
} M3FusedCmpOps;

#  define d_fuseCmp(TYPE, NAME)               { d_fuseCmpList (TYPE, NAME, BranchIf),            d_fuseCmpList (TYPE, NAME, If) }
#  define d_fuseCommutativeCmp(TYPE, NAME)    { d_fuseCommutativeCmpList (TYPE, NAME, BranchIf), d_fuseCommutativeCmpList (TYPE, NAME, If) }

// clang-format off
ساكن ثابت M3FusedCmpOps c_fuse_i32_Equal              = d_fuseCommutativeCmp (i32, Equal);
ساكن ثابت M3FusedCmpOps c_fuse_i32_NotEqual           = d_fuseCommutativeCmp (i32, NotEqual);
ساكن ثابت M3FusedCmpOps c_fuse_i32_LessThan           = d_fuseCmp (i32, LessThan);
ساكن ثابت M3FusedCmpOps c_fuse_u32_LessThan           = d_fuseCmp (u32, LessThan);
ساكن ثابت M3FusedCmpOps c_fuse_i32_GreaterThan        = d_fuseCmp (i32, GreaterThan);
ساكن ثابت M3FusedCmpOps c_fuse_u32_GreaterThan        = d_fuseCmp (u32, GreaterThan);
ساكن ثابت M3FusedCmpOps c_fuse_i32_LessThanOrEqual    = d_fuseCmp (i32, LessThanOrEqual);
ساكن ثابت M3FusedCmpOps c_fuse_u32_LessThanOrEqual    = d_fuseCmp (u32, LessThanOrEqual);
ساكن ثابت M3FusedCmpOps c_fuse_i32_GreaterThanOrEqual = d_fuseCmp (i32, GreaterThanOrEqual);
ساكن ثابت M3FusedCmpOps c_fuse_u32_GreaterThanOrEqual = d_fuseCmp (u32, GreaterThanOrEqual);
// clang-format on

// i32.eqz is unary: [0]= operand in _r0, [1]= operand in a slot
ساكن ثابت M3FusedCmpOps c_fuse_i32_Eqz = {
    { op_i32_BranchIfEqz_r, op_i32_BranchIfEqz_s, عدم, عدم },
    { op_i32_IfEqz_r,       op_i32_IfEqz_s,       عدم, عدم }
};

ساكن
ثابت M3FusedCmpOps* GetFusedCmpOps (m3opcode_t i_opcode)
{
    بدل (i_opcode) {
    حالة 0x45: أرجع &c_fuse_i32_Eqz;                    // i32.eqz
    حالة 0x46: أرجع &c_fuse_i32_Equal;                  // i32.eq
    حالة 0x47: أرجع &c_fuse_i32_NotEqual;               // i32.ne
    حالة 0x48: أرجع &c_fuse_i32_LessThan;               // i32.lt_s
    حالة 0x49: أرجع &c_fuse_u32_LessThan;               // i32.lt_u
    حالة 0x4a: أرجع &c_fuse_i32_GreaterThan;            // i32.gt_s
    حالة 0x4b: أرجع &c_fuse_u32_GreaterThan;            // i32.gt_u
    حالة 0x4c: أرجع &c_fuse_i32_LessThanOrEqual;        // i32.le_s
    حالة 0x4d: أرجع &c_fuse_u32_LessThanOrEqual;        // i32.le_u
    حالة 0x4e: أرجع &c_fuse_i32_GreaterThanOrEqual;     // i32.ge_s
    حالة 0x4f: أرجع &c_fuse_u32_GreaterThanOrEqual;     // i32.ge_u
    افتراضي: أرجع عدم;
    }
}

#پایان_اگر // d_m3FuseBranch

ساكن ثابت IM3Operation c_intSelectOps[2][4] = {
    { op_Select_i32_rss, op_Select_i32_srs, op_Select_i32_ssr, op_Select_i32_sss },
    { op_Select_i64_rss, op_Select_i64_srs, op_Select_i64_ssr, op_Select_i64_sss }
};
#إذا d_m3HasFloat
ساكن ثابت IM3Operation c_fpSelectOps[2][2][3] = {
    { { op_Select_f32_sss, op_Select_f32_srs, op_Select_f32_ssr }, // selector in slot
      { op_Select_f32_rss, op_Select_f32_rrs, op_Select_f32_rsr } }, // selector in reg
    { { op_Select_f64_sss, op_Select_f64_srs, op_Select_f64_ssr }, // selector in slot
      { op_Select_f64_rss, op_Select_f64_rrs, op_Select_f64_rsr } }
};    // selector in reg
#پایان_اگر

// all args & returns are 64-bit aligned, so use 2 slots for a d_m3Use32BitSlots=1 build

ساكن
M3_نتیجه Acquire_Compilation_کد_صفحه (IM3Compilation o, IM3_کد_صفحه* o_کد_صفحه)
{
    M3_نتیجه نتیجه = m3Err_none;

    IM3_کد_صفحه صفحه = Acquire_کد_صفحه(o->runtime);

    إذا (صفحه) {
#إذا (d_m3_Enable_کد_صفحه_Ref_Counting)
        {
            إذا (o->function) {
                IM3Function func = o->function;
                صفحه->info.usage_شمار++;

                u32 index = func->رقم_کد_صفحه_Refs++;
_               (m3ReallocArray(&func->کد_صفحه_Refs, IM3_کد_صفحه, func->رقم_کد_صفحه_Refs, index));
                func->کد_صفحه_Refs[index] = صفحه;
            }
        }
#پایان_اگر
    } وإلا {
        _throw(m3_Err_malloc_Failed_کد_صفحه);
    }

    _catch:

    *o_کد_صفحه = صفحه;

    أرجع نتیجه;
}

ساكن مضمن
فراغ Release_Compilation_کد_صفحه (IM3Compilation o)
{
    Release_کد_صفحه(o->runtime, o->صفحه);
}

ساكن مضمن
u16 Get_Type_رقم_Slots (m3type_t i_type)
{
    i_type = پایه_Type_از(i_type);

    // v128 is 16 bytes - 4 slots in 32-bit-slot mode, 2 in 64-bit.
    // (Slot-allocator only; no v128 ops execute.)
    إذا (i_type == c_m3Type_v128)
#إذا d_m3_Use32_بیت_Slots
        أرجع 4;
#وإلا
        أرجع 2;
#پایان_اگر
#إذا d_m3_Use32_بیت_Slots
    أرجع Is64_بیت_Type(i_type) ? 2 : 1;
#وإلا
    أرجع 1;
#پایان_اگر
}

ساكن مضمن
فراغ Align_جایگاه_مقصد_Type (u16* io_جایگاه, m3type_t i_type)
{
    // align 64-bit words to even slots (if d_m3Use32BitSlots)
    u16 رقم_Slots = Get_Type_رقم_Slots(i_type);

    u16 ماسک = رقم_Slots - 1;
    *io_جایگاه = (*io_جایگاه + ماسک) & ~ماسک;
}

// Deliberately unchecked. Validation rejects an empty operand stack before compilation.
ساكن مضمن
i16 Get_پشته_بالاترین_شاخص (IM3Compilation o)
{
    أرجع o->پشته_شاخص - 1;
}

ساكن مضمن
M3_نتیجه Get_پشته_بالاترین_شاخص_Throws (IM3Compilation o, i16* o_پشته_شاخص)
{
    *o_پشته_شاخص = o->پشته_شاخص - 1;

    إذا (M3_LIKELY(o->پشته_شاخص > o->پشته_اولی_Dynamic_شاخص or Is_پشته_Polymorphic(o))) {
        أرجع m3Err_none;
    } وإلا {
        أرجع m3_Err_function_پشته_Underrun;
    }
}


// Items in the static portion of the stack (args/locals) are hidden from GetStackTypeFromTop ()
// In other words, only "real" Wasm stack items can be inspected.  This is important when
// returning values, etc. and you need an accurate wasm-view of the stack.
ساكن
m3type_t Get_پشته_Type_From_بالاترین (IM3Compilation o, u16 i_جابه_جایی)
{
    m3type_t type = c_m3Type_none;

    ++i_جابه_جایی;
    إذا (o->پشته_شاخص >= i_جابه_جایی) {
        u16 index = o->پشته_شاخص - i_جابه_جایی;

        إذا (index >= o->پشته_اولی_Dynamic_شاخص) {
            type = o->type_پشته[index];
        }
    }

    أرجع type;
}

ساكن مضمن
m3type_t Get_پشته_بالاترین_Type (IM3Compilation o)
{
    أرجع Get_پشته_Type_From_بالاترین(o, 0);
}

ساكن مضمن
m3type_t Get_پشته_Type_From_Bottom (IM3Compilation o, u16 i_جابه_جایی)
{
    m3type_t type = c_m3Type_none;

    إذا (i_جابه_جایی < o->پشته_شاخص) {
        type = o->type_پشته[i_جابه_جایی];
    }

    أرجع type;
}


ساكن مضمن
bool Is_Constant_جایگاه (IM3Compilation o, u16 i_جایگاه)
{
    أرجع (i_جایگاه >= o->جایگاه_اولی_Const_شاخص and i_جایگاه < o->جایگاه_Max_Const_شاخص);
}
ساكن مضمن
bool Is_جایگاه_Allocated (IM3Compilation o, u16 i_جایگاه)
{
    أرجع o->m3Slots[i_جایگاه];
}

ساكن مضمن
bool Is_پشته_شاخص_درون_ثبات (IM3Compilation o, i32 i_پشته_شاخص)
{                                                                           d_m3Assert (i_پشته_شاخص < o->پشته_شاخص or Is_پشته_Polymorphic (o));
    إذا (i_پشته_شاخص >= 0 and i_پشته_شاخص < o->پشته_شاخص) {
        أرجع (o->wasm_پشته[i_پشته_شاخص] >= d_m3_Reg0_جایگاه_Alias);
    } وإلا {
        أرجع false;
    }
}

ساكن مضمن
u16 Get_رقم_بلوک_مقادیر_On_پشته (IM3Compilation o)
{
    أرجع o->پشته_شاخص - o->بلوک.بلوک_پشته_شاخص;
}

ساكن مضمن
bool Is_پشته_بالاترین_درون_ثبات (IM3Compilation o)
{
    أرجع Is_پشته_شاخص_درون_ثبات(o, (i32)Get_پشته_بالاترین_شاخص(o));
}
ساكن مضمن
bool Is_پشته_بالاترین_Minus1_درون_ثبات (IM3Compilation o)
{
    أرجع Is_پشته_شاخص_درون_ثبات(o, (i32)Get_پشته_بالاترین_شاخص(o) - 1);
}
ساكن مضمن
bool Is_پشته_بالاترین_Minus2_درون_ثبات (IM3Compilation o)
{
    أرجع Is_پشته_شاخص_درون_ثبات(o, (i32)Get_پشته_بالاترین_شاخص(o) - 2);
}

ساكن مضمن
bool Is_پشته_بالاترین_درون_جایگاه (IM3Compilation o)
{
    أرجع not Is_پشته_بالاترین_درون_ثبات(o);
}

ساكن مضمن
bool Is_Valid_جایگاه (u16 i_جایگاه)
{
    أرجع (i_جایگاه < d_m3MaxFunctionSlots);
}

ساكن مضمن
u16 Get_پشته_بالاترین_جایگاه_عدد (IM3Compilation o)
{
    i16 i = Get_پشته_بالاترین_شاخص(o);

    u16 جایگاه = c_جایگاه_استفادهنشده;

    إذا (i >= 0) {
        جایگاه = o->wasm_پشته[i];
    }

    أرجع جایگاه;
}


// from bottom
ساكن مضمن
u16 Get_جایگاه_For_پشته_شاخص (IM3Compilation o, u16 i_پشته_شاخص)
{                                                                   d_m3Assert (i_پشته_شاخص < o->پشته_شاخص or Is_پشته_Polymorphic (o));
    u16 جایگاه = c_جایگاه_استفادهنشده;

    إذا (i_پشته_شاخص < o->پشته_شاخص) {
        جایگاه = o->wasm_پشته[i_پشته_شاخص];
    }

    أرجع جایگاه;
}

ساكن مضمن
u16 Get_Extra_جایگاه_For_پشته_شاخص (IM3Compilation o, u16 i_پشته_شاخص)
{
    u16 پایه_جایگاه = Get_جایگاه_For_پشته_شاخص(o, i_پشته_شاخص);

    إذا (پایه_جایگاه != c_جایگاه_استفادهنشده) {
        u16 extra_جایگاه = Get_Type_رقم_Slots(Get_پشته_Type_From_Bottom(o, i_پشته_شاخص)) - 1;
        پایه_جایگاه += extra_جایگاه;
    }

    أرجع پایه_جایگاه;
}


ساكن مضمن
فراغ Touch_جایگاه (IM3Compilation o, u16 i_جایگاه)
{
    // op_Entry uses this value to track and detect stack overflow
    o->max_پشته_Slots = M3_MAX(o->max_پشته_Slots, i_جایگاه + 1);
}

ساكن مضمن
فراغ Mark_جایگاه_Allocated (IM3Compilation o, u16 i_جایگاه)
{                                                                   d_m3Assert (o->m3Slots [i_جایگاه] == 0); // shouldn't be already allocated
    o->m3Slots[i_جایگاه] = 1;

    o->جایگاه_Max_Allocated_شاخص_Plus_One = M3_MAX(o->جایگاه_Max_Allocated_شاخص_Plus_One, i_جایگاه + 1);

    Touch_جایگاه(o, i_جایگاه);
}

ساكن مضمن
M3_نتیجه MarkSlotsAllocated (IM3Compilation o, u16 i_جایگاه, u16 i_رقم_Slots)
{
    إذا (M3_UNLIKELY(i_جایگاه + i_رقم_Slots > d_m3MaxFunctionSlots)) {
        أرجع m3_Err_function_پشته_سرریز;
    }

    طالما (i_رقم_Slots--) {
        Mark_جایگاه_Allocated(o, i_جایگاه++);
    }

    أرجع m3Err_none;
}

ساكن مضمن
M3_نتیجه MarkSlotsAllocatedByType (IM3Compilation o, u16 i_جایگاه, m3type_t i_type)
{
    u16 رقم_Slots = Get_Type_رقم_Slots(i_type);
    أرجع MarkSlotsAllocated(o, i_جایگاه, رقم_Slots);
}


ساكن
M3_نتیجه AllocateSlotsWithinRange (IM3Compilation o, u16* o_جایگاه, m3type_t i_type, u16 i_شروع_جایگاه, u16 i_انتها_جایگاه)
{
    M3_نتیجه نتیجه = m3_Err_function_پشته_سرریز;

    u16 رقم_Slots     = Get_Type_رقم_Slots(i_type);
    u16 جستجو_جابه_جایی = رقم_Slots - 1;

    Align_جایگاه_مقصد_Type(&i_شروع_جایگاه, i_type);

    // search for 1 or 2 consecutive slots in the execution stack
    u16 i = i_شروع_جایگاه;
    طالما (i + جستجو_جابه_جایی < i_انتها_جایگاه) {
        إذا (i + جستجو_جابه_جایی < d_m3MaxFunctionSlots and o->m3Slots[i] == 0 and o->m3Slots[i + جستجو_جابه_جایی] == 0) {
            MarkSlotsAllocated(o, i, رقم_Slots);

            *o_جایگاه = i;
            نتیجه  = m3Err_none;
            اكسر;
        }

        // keep 2-slot allocations even-aligned
        i += رقم_Slots;
    }

    أرجع نتیجه;
}

ساكن مضمن
M3_نتیجه AllocateSlots (IM3Compilation o, u16* o_جایگاه, m3type_t i_type)
{
    أرجع AllocateSlotsWithinRange(o, o_جایگاه, i_type, o->جایگاه_اولی_Dynamic_شاخص, d_m3MaxFunctionSlots);
}

ساكن مضمن
M3_نتیجه AllocateConstantSlots (IM3Compilation o, u16* o_جایگاه, m3type_t i_type)
{
    u16 max_جدول_شاخص = o->جایگاه_اولی_Const_شاخص + d_m3_Max_Constant_جدول_اندازه;
    أرجع AllocateSlotsWithinRange(o, o_جایگاه, i_type, o->جایگاه_اولی_Const_شاخص, M3_MIN(o->جایگاه_اولی_Dynamic_شاخص, max_جدول_شاخص));
}


// TOQUE: this usage count system could be eliminated. real world code doesn't frequently trigger it.  just copy to multiple
// unique slots.
ساكن مضمن
M3_نتیجه Increment_جایگاه_Usage_شمار (IM3Compilation o, u16 i_جایگاه)
{                                                                                       d_m3Assert (i_جایگاه < d_m3MaxFunctionSlots);
    M3_نتیجه نتیجه = m3Err_none;                                                       d_m3Assert (o->m3Slots [i_جایگاه] > 0);

    // OPTZ (memory): 'm3Slots' could still be fused with 'typeStack' if 4 bits were used to indicate: [0,1,2,many]. The many-case
    // would scan 'wasmStack' to determine the actual usage count
    إذا (M3_LIKELY(o->m3Slots[i_جایگاه] < 0xFF)) {
        o->m3Slots[i_جایگاه]++;
    } وإلا {
        نتیجه = "slot usage count overflow";
    }

    أرجع نتیجه;
}

ساكن مضمن
فراغ Deallocate_جایگاه (IM3Compilation o, i16 i_جایگاه, m3type_t i_type)
{                                                                                       d_m3Assert (i_جایگاه >= o->جایگاه_اولی_Dynamic_شاخص);
                                                                                        d_m3Assert (i_جایگاه < o->جایگاه_Max_Allocated_شاخص_Plus_One);
    لكل (u16 i = 0; i < Get_Type_رقم_Slots(i_type); ++i, ++i_جایگاه) {                       d_m3Assert (o->m3Slots [i_جایگاه]);
        --o->m3Slots[i_جایگاه];
    }
}


ساكن مضمن
bool Is_ثبات_Type_Allocated (IM3Compilation o, u8 i_type)
{
    أرجع Is_ثبات_Allocated(o, IsFpType(i_type));
}

ساكن مضمن
فراغ Allocate_ثبات (IM3Compilation o, u32 i_ثبات, u16 i_پشته_شاخص)
{                                                                                       d_m3Assert (not Is_ثبات_Allocated (o, i_ثبات));
    o->reg_پشته_شاخص_Plus_One[i_ثبات] = i_پشته_شاخص + 1;
}

ساكن مضمن
فراغ Deallocate_ثبات (IM3Compilation o, u32 i_ثبات)
{                                                                                       d_m3Assert (Is_ثبات_Allocated (o, i_ثبات));
    o->reg_پشته_شاخص_Plus_One[i_ثبات] = c_m3_ثبات_Unallocated;
}

ساكن مضمن
u16 Get_ثبات_پشته_شاخص (IM3Compilation o, u32 i_ثبات)
{                                                                                       d_m3Assert (Is_ثبات_Allocated (o, i_ثبات));
    أرجع o->reg_پشته_شاخص_Plus_One[i_ثبات] - 1;
}

u16 Get_Max_استفاده_شده_جایگاه_Plus_One (IM3Compilation o)
{
    طالما (o->جایگاه_Max_Allocated_شاخص_Plus_One > o->جایگاه_اولی_Dynamic_شاخص) {
        إذا (Is_جایگاه_Allocated(o, o->جایگاه_Max_Allocated_شاخص_Plus_One - 1)) {
            اكسر;
        }

        o->جایگاه_Max_Allocated_شاخص_Plus_One--;
    }

#اگر_تعریف_شده DEBUG
    u16 max_جایگاه = o->جایگاه_Max_Allocated_شاخص_Plus_One;
    طالما (max_جایگاه < d_m3MaxFunctionSlots) {
        d_m3Assert(o->m3Slots[max_جایگاه] == 0);
        max_جایگاه++;
    }
#پایان_اگر

    أرجع o->جایگاه_Max_Allocated_شاخص_Plus_One;
}

ساكن
M3_نتیجه Preserve_ثبات_If_Occupied (IM3Compilation o, m3type_t i_ثبات_Type)
{
    M3_نتیجه نتیجه = m3Err_none;

    u32 regSelect = IsFpType(i_ثبات_Type);

    إذا (Is_ثبات_Allocated(o, regSelect)) {
        u16 پشته_شاخص = Get_ثبات_پشته_شاخص(o, regSelect);
        Deallocate_ثبات(o, regSelect);

        m3type_t type = Get_پشته_Type_From_Bottom(o, پشته_شاخص);

        // and point to a exec slot
        u16 جایگاه = c_جایگاه_استفادهنشده;
_       (AllocateSlots(o, &جایگاه, type));
        o->wasm_پشته[پشته_شاخص] = جایگاه;

        // Ensure type is within the valid range
        إذا (M3_LIKELY(پایه_Type_از(type) < c_m3_Type_شمار)) {
_           (EmitOp(o, c_مجموعه_مجموعه_Ops[پایه_Type_از(type)]));
        } وإلا {
            _throw(m3Err_unknownType);
        }

        Emit_جایگاه_جابه_جایی(o, جایگاه);
    }

    _catch: أرجع نتیجه;
}


// all values must be in slots before entering loop, if, and else blocks
// otherwise they'd end up preserve-copied in the block to probably different locations (if/else)
ساكن مضمن
M3_نتیجه PreserveRegisters (IM3Compilation o)
{
    M3_نتیجه نتیجه;

_   (Preserve_ثبات_If_Occupied(o, c_m3Type_f64));
_   (Preserve_ثبات_If_Occupied(o, c_m3Type_i64));

    _catch: أرجع نتیجه;
}

ساكن
M3_نتیجه Preserve_Non_بالاترین_Registers (IM3Compilation o)
{
    M3_نتیجه نتیجه = m3Err_none;

    i16 پشته_بالاترین = Get_پشته_بالاترین_شاخص(o);

    إذا (پشته_بالاترین >= 0) {
        إذا (Is_ثبات_Allocated(o, 0))     // r0
        {
            إذا (Get_ثبات_پشته_شاخص(o, 0) != پشته_بالاترین) {
_               (Preserve_ثبات_If_Occupied(o, c_m3Type_i64));
            }
        }

        إذا (Is_ثبات_Allocated(o, 1))     // fp0
        {
            إذا (Get_ثبات_پشته_شاخص(o, 1) != پشته_بالاترین) {
_               (Preserve_ثبات_If_Occupied(o, c_m3Type_f64));
            }
        }
    }

    _catch: أرجع نتیجه;
}


//----------------------------------------------------------------------------------------------------------------------

ساكن
M3_نتیجه افزودن (IM3Compilation o, m3type_t i_type, u16 i_جایگاه)
{
    M3_نتیجه نتیجه = m3Err_none;

#إذا !d_m3HasFloat
    إذا (i_type == c_m3Type_f32 || i_type == c_m3Type_f64) {
        أرجع m3Err_unknownOpcode;
    }
#پایان_اگر

    إذا (M3_UNLIKELY(o->پشته_شاخص >= d_m3_Max_Function_پشته_ارتفاع)) {
        أرجع m3_Err_function_پشته_سرریز;
    }

    u16 پشته_شاخص = o->پشته_شاخص++;                                       // printf ("push: %d\n", (i32) i);

    o->wasm_پشته[پشته_شاخص] = i_جایگاه;
    o->type_پشته[پشته_شاخص] = i_type;

    إذا (Is_ثبات_جایگاه_Alias(i_جایگاه)) {
        u32 regSelect = Is_Fp_ثبات_جایگاه_Alias(i_جایگاه);
        Allocate_ثبات(o, regSelect, پشته_شاخص);
    }

    إذا (d_m3_Log_Wasm_پشته) {
        dump_type_پشته(o);
    }

    أرجع نتیجه;
}

ساكن مضمن
M3_نتیجه افزودن_ثبات (IM3Compilation o, m3type_t i_type)
{
    M3_نتیجه نتیجه = m3Err_none;                                                       d_m3Assert ((u16) d_m3_Reg0_جایگاه_Alias > (u16) d_m3MaxFunctionSlots);
    u16      جایگاه   = IsFpType(i_type) ? d_m3_Fp0_جایگاه_Alias : d_m3_Reg0_جایگاه_Alias;          d_m3Assert (i_type or Is_پشته_Polymorphic (o));

_   (افزودن(o, i_type, جایگاه));

    _catch: أرجع نتیجه;
}

ساكن
M3_نتیجه برداشت (IM3Compilation o)
{
    M3_نتیجه نتیجه = m3Err_none;

    إذا (M3_LIKELY(o->پشته_شاخص > o->بلوک.بلوک_پشته_شاخص)) {
        o->پشته_شاخص--;                                                //  printf ("pop: %d\n", (i32) o->stackIndex);

        u16      جایگاه = o->wasm_پشته[o->پشته_شاخص];
        m3type_t type = o->type_پشته[o->پشته_شاخص];

        إذا (Is_ثبات_جایگاه_Alias(جایگاه)) {
            u32 regSelect = Is_Fp_ثبات_جایگاه_Alias(جایگاه);
            Deallocate_ثبات(o, regSelect);
        } وإلا إذا (M3_UNLIKELY(جایگاه >= o->جایگاه_Max_Allocated_شاخص_Plus_One)) {
            أرجع m3_Err_function_پشته_Underrun; // Return error for invalid slot indices
        } وإلا إذا (جایگاه >= o->جایگاه_اولی_Dynamic_شاخص) {
            Deallocate_جایگاه(o, جایگاه, type);
        }
    } وإلا إذا (not Is_پشته_Polymorphic(o)) {
        نتیجه = m3_Err_function_پشته_Underrun;
    }

    أرجع نتیجه;
}

ساكن
M3_نتیجه برداشت_Type (IM3Compilation o, m3type_t i_type)
{
    M3_نتیجه نتیجه = m3Err_none;

    m3type_t بالاترین_Type = Get_پشته_بالاترین_Type(o);

    إذا (M3_LIKELY(Is_Sub_Type_از(بالاترین_Type, i_type) or o->بلوک.isPolymorphic)) {
_       (برداشت(o));
    } وإلا {
        _throw(m3Err_typeMismatch);
    }

    _catch:
    أرجع نتیجه;
}

ساكن
M3_نتیجه افزودن_Allocated_جایگاه_And_Emit (IM3Compilation o, m3type_t i_type, bool i_doEmit)
{
    M3_نتیجه نتیجه = m3Err_none;

    u16 جایگاه = c_جایگاه_استفادهنشده;

_   (AllocateSlots(o, &جایگاه, i_type));
_   (افزودن(o, i_type, جایگاه));

    إذا (i_doEmit) {
        Emit_جایگاه_جابه_جایی(o, جایگاه);
    }

    //    printf ("push: %d\n", (u32) slot);

    _catch: أرجع نتیجه;
}

ساكن مضمن
M3_نتیجه افزودن_Allocated_جایگاه_And_Emit_2 (IM3Compilation o, m3type_t i_type)
{
    أرجع افزودن_Allocated_جایگاه_And_Emit(o, i_type, true);
}

ساكن مضمن
M3_نتیجه افزودن_Allocated_جایگاه (IM3Compilation o, m3type_t i_type)
{
    أرجع افزودن_Allocated_جایگاه_And_Emit(o, i_type, false);
}

ساكن
M3_نتیجه افزودن_Const (IM3Compilation o, u64 i_کلمه, m3type_t i_type)
{
    M3_نتیجه نتیجه = m3Err_none;

    // When compile-walking a constant expression without emitting there is no
    // constant table to place the value in, but the type still has to land on
    // the stack: extended-const arithmetic downstream consumes it.
    إذا (M3_UNLIKELY(!o->صفحه)) {
        أرجع افزودن_Allocated_جایگاه(o, i_type);
    }

    bool تطابق_یافت  = false;
    bool is64_بیت_Type = Is64_بیت_Type(i_type);

    u16 رقم_Required_Slots  = Get_Type_رقم_Slots(i_type);
    u16 رقم_استفاده_شده_Const_Slots = o->جایگاه_Max_Const_شاخص - o->جایگاه_اولی_Const_شاخص;

    // search for duplicate matching constant slot to reuse
    إذا (رقم_Required_Slots == 2 and رقم_استفاده_شده_Const_Slots >= 2) {
        u16 اولی_Const_جایگاه = o->جایگاه_اولی_Const_شاخص;
        Align_جایگاه_مقصد_Type(&اولی_Const_جایگاه, c_m3Type_i64);

        لكل (u16 جایگاه = اولی_Const_جایگاه; جایگاه < o->جایگاه_Max_Const_شاخص - 1; جایگاه += 2) {
            إذا (Is_جایگاه_Allocated(o, جایگاه) and Is_جایگاه_Allocated(o, جایگاه + 1)) {
                u64 constant;
                کپی_حافظه(&constant, &o->constants[جایگاه - o->جایگاه_اولی_Const_شاخص], حجم(constant));

                إذا (constant == i_کلمه) {
                    تطابق_یافت = true;
_                   (افزودن(o, i_type, جایگاه));
                    اكسر;
                }
            }
        }
    } وإلا إذا (رقم_Required_Slots == 1) {
        لكل (u16 i = 0; i < رقم_استفاده_شده_Const_Slots; ++i) {
            u16 جایگاه = o->جایگاه_اولی_Const_شاخص + i;

            إذا (Is_جایگاه_Allocated(o, جایگاه)) {
                bool matches;
                إذا (is64_بیت_Type) {
                    u64 constant;
                    کپی_حافظه(&constant, &o->constants[i], حجم(constant));
                    matches = (constant == i_کلمه);
                } وإلا {
                    u32 constant;
                    کپی_حافظه(&constant, &o->constants[i], حجم(constant));
                    matches = (constant == i_کلمه);
                }
                إذا (matches) {
                    تطابق_یافت = true;
_                   (افزودن(o, i_type, جایگاه));
                    اكسر;
                }
            }
        }
    }

    إذا (not تطابق_یافت) {
        u16 جایگاه = c_جایگاه_استفادهنشده;
        نتیجه   = AllocateConstantSlots(o, &جایگاه, i_type);

        إذا (نتیجه || جایگاه == c_جایگاه_استفادهنشده) // no more constant table space; use inline constants
        {
            نتیجه = m3Err_none;

            إذا (is64_بیت_Type) {
_               (EmitOp(o, op_Const64));
                EmitWord64(o->صفحه, i_کلمه);
            } وإلا {
_               (EmitOp(o, op_Const32));
                EmitWord32(o->صفحه, (u32)i_کلمه);
            }

_           (افزودن_Allocated_جایگاه_And_Emit_2(o, i_type));
        } وإلا {
            u16 const_جدول_شاخص = جایگاه - o->جایگاه_اولی_Const_شاخص;

            d_m3Assert(const_جدول_شاخص < d_m3_Max_Constant_جدول_اندازه);

            إذا (is64_بیت_Type) {
                کپی_حافظه(&o->constants[const_جدول_شاخص], &i_کلمه, حجم(i_کلمه));
            } وإلا {
                u32 word32 = (u32)i_کلمه;
                کپی_حافظه(&o->constants[const_جدول_شاخص], &word32, حجم(word32));
            }

_           (افزودن(o, i_type, جایگاه));

            o->جایگاه_Max_Const_شاخص = M3_MAX(جایگاه + رقم_Required_Slots, o->جایگاه_Max_Const_شاخص);
        }
    }

    _catch: أرجع نتیجه;
}

ساكن مضمن
M3_نتیجه Emit_جایگاه_رقم_از_پشته_بالاترین_And_برداشت (IM3Compilation o)
{
    // no emit if value is in register
    إذا (Is_پشته_بالاترین_درون_جایگاه(o)) {
        Emit_جایگاه_جابه_جایی(o, Get_پشته_بالاترین_جایگاه_عدد(o));
    }

    أرجع برداشت(o);
}


// Or, maybe: EmitTrappingOp
M3_نتیجه جمع_کن_Trap_رکورد (IM3Compilation o)
{
    M3_نتیجه نتیجه = m3Err_none;

    إذا (o->function) {
    }

    أرجع نتیجه;
}

ساكن
M3_نتیجه Unwind_بلوک_پشته (IM3Compilation o)
{
    M3_نتیجه نتیجه = m3Err_none;

    u32 برداشت_شمار = 0;
    طالما (o->پشته_شاخص > o->بلوک.بلوک_پشته_شاخص) {
_       (برداشت(o));
        ++برداشت_شمار;
    }

    إذا (برداشت_شمار) {
        m3log(compile, "unwound stack top: %d", برداشت_شمار);
    }

    _catch: أرجع نتیجه;
}

ساكن مضمن
M3_نتیجه مجموعه_پشته_Polymorphic (IM3Compilation o)
{
    o->بلوک.isPolymorphic = true;                              m3log (compile, "stack set polymorphic");
    أرجع Unwind_بلوک_پشته(o);
}

ساكن
فراغ PatchBranches (IM3Compilation o)
{
    InvalidateFold(o);         // patched branches land at the current pc

    pc_t pc = GetPC(o);

    pc_t patches     = o->بلوک.patches;
    o->بلوک.patches = عدم;

    طالما (patches) {                                           m3log (compile, "patching location: %p to pc: %p", patches, pc);
        pc_t بعدی       = *(pc_t*)patches;
        *(pc_t*)patches = pc;
        patches         = بعدی;
    }
}

//-------------------------------------------------------------------------------------------------------------------------

ساكن
M3_نتیجه Copy_پشته_شاخص_مقصد_جایگاه (IM3Compilation o, u16 i_dest_جایگاه, u16 i_پشته_شاخص)  // NoPushPop
{
    M3_نتیجه نتیجه = m3Err_none;

    IM3Operation op;

    m3type_t type       = Get_پشته_Type_From_Bottom(o, i_پشته_شاخص);
    bool     درون_ثبات = Is_پشته_شاخص_درون_ثبات(o, i_پشته_شاخص);

    إذا (درون_ثبات) {
        op = c_مجموعه_مجموعه_Ops[پایه_Type_از(type)];
    } وإلا {
        op = Is64_بیت_Type(type) ? op_Copy_جایگاه_64 : op_Copy_جایگاه_32;
    }

_   (EmitOp(o, op));
    Emit_جایگاه_جابه_جایی(o, i_dest_جایگاه);

    إذا (not درون_ثبات) {
        u16 src_جایگاه = Get_جایگاه_For_پشته_شاخص(o, i_پشته_شاخص);
        Emit_جایگاه_جابه_جایی(o, src_جایگاه);
    }

    _catch: أرجع نتیجه;
}

ساكن
M3_نتیجه Copy_پشته_بالاترین_مقصد_جایگاه (IM3Compilation o, u16 i_dest_جایگاه)  // NoPushPop
{
    M3_نتیجه نتیجه;

    i16 پشته_بالاترین;
_   (Get_پشته_بالاترین_شاخص_Throws(o, &پشته_بالاترین));
_   (Copy_پشته_شاخص_مقصد_جایگاه(o, i_dest_جایگاه, (u16)پشته_بالاترین));

    _catch: أرجع نتیجه;
}


// a copy-on-write strategy is used with locals. when a get local occurs, it's not copied anywhere. the stack
// entry just has a index pointer to that local memory slot.
// then, when a previously referenced local is set, the current value needs to be preserved for those references

// TODO: consider getting rid of these specialized operations: PreserveSetSlot & PreserveCopySlot.
// They likely just take up space (which seems to reduce performance) without improving performance.
// => unconfirmed in preliminary testing.
ساكن
M3_نتیجه Preserved_Copy_بالاترین_جایگاه (IM3Compilation o, u16 i_dest_جایگاه, u16 i_preserve_جایگاه)
{
    M3_نتیجه نتیجه = m3Err_none;             d_m3Assert (i_dest_جایگاه != i_preserve_جایگاه);

    IM3Operation op;

    m3type_t type = Get_پشته_بالاترین_Type(o);

    إذا (Is_پشته_بالاترین_درون_ثبات(o)) {
        op = c_preserve_مجموعه_جایگاه[پایه_Type_از(type)];
    } وإلا {
        op = Is64_بیت_Type(type) ? op_Preserve_Copy_جایگاه_64 : op_Preserve_Copy_جایگاه_32;
    }

_   (EmitOp(o, op));
    Emit_جایگاه_جابه_جایی(o, i_dest_جایگاه);

    إذا (Is_پشته_بالاترین_درون_جایگاه(o)) {
        Emit_جایگاه_جابه_جایی(o, Get_پشته_بالاترین_جایگاه_عدد(o));
    }

    Emit_جایگاه_جابه_جایی(o, i_preserve_جایگاه);

    _catch: أرجع نتیجه;
}

ساكن
M3_نتیجه Copy_پشته_بالاترین_مقصد_ثبات (IM3Compilation o, bool i_update_پشته)
{
    M3_نتیجه نتیجه = m3Err_none;

    إذا (Is_پشته_بالاترین_درون_جایگاه(o)) {
        m3type_t type = Get_پشته_بالاترین_Type(o);

_       (Preserve_ثبات_If_Occupied(o, type));

        IM3Operation op = c_مجموعه_ثبات_Ops[پایه_Type_از(type)];

_       (EmitOp(o, op));
        Emit_جایگاه_جابه_جایی(o, Get_پشته_بالاترین_جایگاه_عدد(o));

        إذا (i_update_پشته) {
_           (برداشت_Type(o, type));
_           (افزودن_ثبات(o, type));
        }
    }

    _catch: أرجع نتیجه;
}


// if local is unreferenced, o_preservedSlotNumber will be equal to localIndex on return
ساكن
M3_نتیجه پیدا_کن_Referenced_Local_Within_Current_بلوک (IM3Compilation o, u16* o_preserved_جایگاه_عدد, u32 i_local_جایگاه)
{
    M3_نتیجه نتیجه = m3Err_none;

    IM3CompilationScope scope = &o->بلوک;

    u16 شروع_شاخص = scope->بلوک_پشته_شاخص;

    طالما (scope->opcode == c_wa_Op_بلوک) {
        scope = scope->outer;
        إذا (not scope) {
            اكسر;
        }

        شروع_شاخص = scope->بلوک_پشته_شاخص;
    }

    *o_preserved_جایگاه_عدد = (u16)i_local_جایگاه;

    لكل (u32 i = شروع_شاخص; i < o->پشته_شاخص; ++i) {
        إذا (o->wasm_پشته[i] == i_local_جایگاه) {
            m3type_t type = Get_پشته_Type_From_Bottom(o, i);                         d_m3Assert (type != c_m3Type_none)

            إذا (*o_preserved_جایگاه_عدد == i_local_جایگاه) {
_               (AllocateSlots(o, o_preserved_جایگاه_عدد, type));
            } وإلا {
                // A 64-bit value spans two slots, and allocating or releasing it
                // counts both, so a second reference has to count both as well.
                لكل (u16 s = 0; s < Get_Type_رقم_Slots(type); ++s) {
_                   (Increment_جایگاه_Usage_شمار(o, (u16)(*o_preserved_جایگاه_عدد + s)));
                }
            }

            o->wasm_پشته[i] = *o_preserved_جایگاه_عدد;
        }
    }

    _catch: أرجع نتیجه;
}

ساكن
M3_نتیجه Get_بلوک_Scope (IM3Compilation o, IM3CompilationScope* o_scope, u32 i_عمق)
{
    M3_نتیجه نتیجه = m3Err_none;

    IM3CompilationScope scope = &o->بلوک;

    طالما (i_عمق--) {
        scope = scope->outer;
        _throwif("invalid block depth", not scope);
    }

    *o_scope = scope;

    _catch:
    أرجع نتیجه;
}

ساكن
M3_نتیجه Copy_پشته_Slots_R (IM3Compilation o, u16 i_هدف_جایگاه_پشته_شاخص, u16 i_پشته_شاخص, u16 i_انتها_پشته_شاخص, u16 i_موقت_جایگاه)
{
    M3_نتیجه نتیجه = m3Err_none;

    إذا (i_پشته_شاخص < i_انتها_پشته_شاخص) {
        u16 src_جایگاه = Get_جایگاه_For_پشته_شاخص(o, i_پشته_شاخص);

        m3type_t type      = Get_پشته_Type_From_Bottom(o, i_پشته_شاخص);
        u16      رقم_Slots  = Get_Type_رقم_Slots(type);
        u16      extra_جایگاه = رقم_Slots - 1;

        u16 هدف_جایگاه = Get_جایگاه_For_پشته_شاخص(o, i_هدف_جایگاه_پشته_شاخص);

        u16 preserve_شاخص = i_پشته_شاخص;
        u16 collision_جایگاه = src_جایگاه;

        إذا (هدف_جایگاه != src_جایگاه) {
            // search for collisions
            u16 check_شاخص = i_پشته_شاخص + 1;
            طالما (check_شاخص < i_انتها_پشته_شاخص) {
                u16 otherSlot1 = Get_جایگاه_For_پشته_شاخص(o, check_شاخص);
                u16 otherSlot2 = Get_Extra_جایگاه_For_پشته_شاخص(o, check_شاخص);

                إذا (هدف_جایگاه == otherSlot1 or
                    هدف_جایگاه == otherSlot2 or
                    هدف_جایگاه + extra_جایگاه == otherSlot1) {
                    _throwif(m3_Err_function_پشته_سرریز, i_موقت_جایگاه >= d_m3MaxFunctionSlots);

_                   (Copy_پشته_شاخص_مقصد_جایگاه(o, i_موقت_جایگاه, check_شاخص));
                    o->wasm_پشته[check_شاخص] = i_موقت_جایگاه;
                    i_موقت_جایگاه += Get_Type_رقم_Slots(c_m3Type_i64);
                    Touch_جایگاه(o, i_موقت_جایگاه - 1);

                    // restore this on the way back down
                    preserve_شاخص = check_شاخص;
                    collision_جایگاه = otherSlot1;

                    اكسر;
                }

                ++check_شاخص;
            }

_           (Copy_پشته_شاخص_مقصد_جایگاه(o, هدف_جایگاه, i_پشته_شاخص));                                                m3log (compile, " copying slot: %d to slot: %d", src_جایگاه, هدف_جایگاه);
            o->wasm_پشته[i_پشته_شاخص] = هدف_جایگاه;
        }

_       (Copy_پشته_Slots_R(o, i_هدف_جایگاه_پشته_شاخص + 1, i_پشته_شاخص + 1, i_انتها_پشته_شاخص, i_موقت_جایگاه));

        // restore the stack state
        o->wasm_پشته[i_پشته_شاخص]  = src_جایگاه;
        o->wasm_پشته[preserve_شاخص] = collision_جایگاه;
    }

    _catch:
    أرجع نتیجه;
}

ساكن
M3_نتیجه Resolve_بلوک_Results (IM3Compilation o, IM3CompilationScope i_هدف_بلوک, bool i_isBranch)
{
    M3_نتیجه نتیجه = m3Err_none;
    إذا (d_m3_Log_Wasm_پشته) {
        dump_type_پشته(o);
    }

    bool isLoop = (i_هدف_بلوک->opcode == c_waOp_loop and i_isBranch);

    u16 رقم_Params  = Get_Func_Type_رقم_Params(i_هدف_بلوک->type);
    u16 رقم_Results = Get_Func_Type_رقم_Results(i_هدف_بلوک->type);

    u16 جایگاه_رکوردها = i_هدف_بلوک->exit_پشته_شاخص;

    u16 رقم_مقادیر;

    إذا (not isLoop) {
        رقم_مقادیر = رقم_Results;
        جایگاه_رکوردها += رقم_Params;
    } وإلا {
        رقم_مقادیر = رقم_Params;
    }

    u16 بلوک_ارتفاع = Get_رقم_بلوک_مقادیر_On_پشته(o);

    _throwif(m3_Err_type_شمار_Mismatch, i_isBranch ? (بلوک_ارتفاع < رقم_مقادیر) : (بلوک_ارتفاع != رقم_مقادیر));

    إذا (رقم_مقادیر) {
        u16 انتها_شاخص     = Get_پشته_بالاترین_شاخص(o) + 1;
        u16 رقم_Rem_مقادیر = رقم_مقادیر;

        // The last result is taken from _fp0. See PushBlockResults.
        إذا (not isLoop and IsFpType(Get_پشته_بالاترین_Type(o))) {
_           (Copy_پشته_بالاترین_مقصد_ثبات(o, false));
            --انتها_شاخص;
            --رقم_Rem_مقادیر;
        }

        // tempslot affects maxStackSlots, so can grow unnecess each time.
        // in practice, this is hardly ever hit.
        u16 موقت_جایگاه = o->max_پشته_Slots;// GetMaxUsedSlotPlusOne (o); doesn't work cause can collide with slotRecords
        Align_جایگاه_مقصد_Type(&موقت_جایگاه, c_m3Type_i64);

_       (Copy_پشته_Slots_R(o, جایگاه_رکوردها, انتها_شاخص - رقم_Rem_مقادیر, انتها_شاخص, موقت_جایگاه));

        إذا (d_m3_Log_Wasm_پشته) {
            dump_type_پشته(o);
        }
    }

    _catch: أرجع نتیجه;
}


ساكن
M3_نتیجه Return_مقادیر (IM3Compilation o, IM3CompilationScope i_function_بلوک, bool i_isBranch)
{
    M3_نتیجه نتیجه = m3Err_none;
    إذا (d_m3_Log_Wasm_پشته) {
        dump_type_پشته(o);
    }

    u16 رقم_Returns  = Get_Func_Type_رقم_Results(i_function_بلوک->type);     // could just o->function too...
    u16 بلوک_ارتفاع = Get_رقم_بلوک_مقادیر_On_پشته(o);

    إذا (not Is_پشته_Polymorphic(o)) {
        _throwif(m3_Err_type_شمار_Mismatch, i_isBranch ? (بلوک_ارتفاع < رقم_Returns) : (بلوک_ارتفاع != رقم_Returns));
    }

    إذا (رقم_Returns) {
        // return slots like args are 64-bit aligned
        u16 return_جایگاه = رقم_Returns * c_io_جایگاه_شمار;
        u16 پشته_بالاترین   = Get_پشته_بالاترین_شاخص(o);

        لكل (u16 i = 0; i < رقم_Returns; ++i) {
            m3type_t returnType = Get_Func_Type_نتیجه_Type(i_function_بلوک->type, رقم_Returns - 1 - i);

            m3type_t پشته_Type = Get_پشته_Type_From_بالاترین(o, i);  // using FromTop so that only dynamic items are checked

            إذا (Is_پشته_Polymorphic(o) and پشته_Type == c_m3Type_none) {
                پشته_Type = returnType;
            }

            _throwif(m3Err_typeMismatch, not Is_Sub_Type_از(پشته_Type, returnType));

            إذا (not Is_پشته_Polymorphic(o)) {
                return_جایگاه -= c_io_جایگاه_شمار;
_               (Copy_پشته_شاخص_مقصد_جایگاه(o, return_جایگاه, پشته_بالاترین--));
            }
        }

        إذا (not i_isBranch) {
            طالما (رقم_Returns--) {
_               (برداشت(o));
            }
        }
    }

    _catch: أرجع نتیجه;
}


//-------------------------------------------------------------------------------------------------------------------------

ساكن
M3_نتیجه Compile_Const_i32 (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_نتیجه نتیجه;

    i32 مقدار;
_   (ReadLEB_i32(&مقدار, &o->wasm, o->wasm_انتها));
_   (افزودن_Const(o, مقدار, c_m3Type_i32));                        m3log (compile, d_indent " (const i32 = %" PRIi32 ")", get_indention_string (o), مقدار);
    _catch: أرجع نتیجه;
}

ساكن
M3_نتیجه Compile_Const_i64 (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_نتیجه نتیجه;

    i64 مقدار;
_   (ReadLEB_i64(&مقدار, &o->wasm, o->wasm_انتها));
_   (افزودن_Const(o, مقدار, c_m3Type_i64));                        m3log (compile, d_indent " (const i64 = %" PRIi64 ")", get_indention_string (o), مقدار);
    _catch: أرجع نتیجه;
}


#إذا d_m3ImplementFloat
ساكن
M3_نتیجه Compile_Const_f32 (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_نتیجه نتیجه;

    اتحاد {
        u32 u;
        f32 f;
    } مقدار = { 0 };

_   (Read_f32(&مقدار.f, &o->wasm, o->wasm_انتها));                 m3log (compile, d_indent " (const f32 = %" PRIf32 ")", get_indention_string (o), مقدار.f);
_   (افزودن_Const(o, مقدار.u, c_m3Type_f32));

    _catch: أرجع نتیجه;
}

ساكن
M3_نتیجه Compile_Const_f64 (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_نتیجه نتیجه;

    اتحاد {
        u64 u;
        f64 f;
    } مقدار = { 0 };

_   (Read_f64(&مقدار.f, &o->wasm, o->wasm_انتها));                 m3log (compile, d_indent " (const f64 = %" PRIf64 ")", get_indention_string (o), مقدار.f);
_   (افزودن_Const(o, مقدار.u, c_m3Type_f64));

    _catch: أرجع نتیجه;
}
#پایان_اگر

ساكن
M3_نتیجه Compile_ExtendedOpcode (IM3Compilation o, m3opcode_t i_opcode)
{
_try {
    u32 opcode;
_   (ReadLEB_u32(&opcode, &o->wasm, o->wasm_انتها));               m3log (compile, d_indent " (%s: %" PRIu32 ")", get_indention_string (o), (i_opcode == c_waOp_atomic) ? "FE" : "FC", opcode);

    // m3opcode_t carries only the low byte, so a wider sub-opcode is unknown
    // here rather than something to truncate into range
    _throwif(m3Err_unknownOpcode, opcode > (u32)((i_opcode == c_waOp_atomic) ? c_wa_Op_آخرین_Atomic : c_wa_Op_آخرین_Extended));

    i_opcode = (m3opcode_t)((i_opcode << 8) | opcode);

    //printf("Extended opcode: 0x%x\n", i_opcode);

    IM3OpInfo opInfo = GetOpInfo(i_opcode);
    _throwif(m3Err_unknownOpcode, not opInfo);

    M3Compiler compiler = GetCompiler(opInfo);
    _throwif(m3Err_noCompiler, not compiler);

_   ((*compiler)(o, i_opcode));

    o->قبلی_Opcode = i_opcode;

    } _catch: أرجع نتیجه;
}

ساكن
M3_نتیجه Compile_Return (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_نتیجه نتیجه = m3Err_none;

    إذا (not Is_پشته_Polymorphic(o)) {
        IM3CompilationScope functionScope;
_       (Get_بلوک_Scope(o, &functionScope, o->بلوک.عمق));

_       (Return_مقادیر(o, functionScope, true));

_       (EmitOp(o, op_Return));

_       (مجموعه_پشته_Polymorphic(o));
    }

    _catch: أرجع نتیجه;
}

ساكن
M3_نتیجه Validate_بلوک_انتها (IM3Compilation o)
{
    M3_نتیجه نتیجه = m3Err_none;

    u16 رقم_Results  = Get_Func_Type_رقم_Results(o->بلوک.type);
    u16 بلوک_ارتفاع = Get_رقم_بلوک_مقادیر_On_پشته(o);

    إذا (not Is_پشته_Polymorphic(o)) {
        // Spec: at block end, stack height must match the number of results
        _throwif(m3_Err_type_شمار_Mismatch, بلوک_ارتفاع != رقم_Results);

        // Spec: result types must match expected types
        لكل (u16 i = 0; i < رقم_Results; ++i) {
            m3type_t expectedType = Get_Func_Type_نتیجه_Type(o->بلوک.type, رقم_Results - 1 - i);
            m3type_t actualType   = Get_پشته_Type_From_بالاترین(o, i);
            _throwif(m3Err_typeMismatch, not Is_Sub_Type_از(actualType, expectedType));
        }
    }

    _catch: أرجع نتیجه;
}

ساكن
M3_نتیجه Compile_انتها (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_نتیجه نتیجه = m3Err_none;                   //dump_type_stack (o);

    // function end:
    إذا (o->بلوک.عمق == 0) {
        Validate_بلوک_انتها(o);

        //      if (not IsStackPolymorphic (o))
        {
            // A constant expression has no function frame, but its value still
            // has to be copied down to slot 0, where EvaluateExpression reads it
            // back. The non-emitting init-expr walk is skipped: it has no block
            // type to return against.
            إذا (o->function or o->صفحه) {
_               (Return_مقادیر(o, &o->بلوک, false));
            }

_           (EmitOp(o, op_Return));
        }
    }

    _catch: أرجع نتیجه;
}


ساكن
M3_نتیجه Compile_مجموعه_Local (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_نتیجه نتیجه;

    u32 local_شاخص;
_   (ReadLEB_u32(&local_شاخص, &o->wasm, o->wasm_انتها));                           m3log (compile, d_indent " (index = %u)", get_indention_string (o), local_شاخص);

    إذا (local_شاخص < Get_Function_رقم_آرگومانها_And_Locals(o->function)) {
        // Spec: value type must match local type
        إذا (not Is_پشته_Polymorphic(o)) {
            m3type_t localType    = Get_پشته_Type_From_Bottom(o, local_شاخص);
            m3type_t پشته_بالاترین_Type = Get_پشته_بالاترین_Type(o);
            _throwif(m3Err_typeMismatch, پشته_بالاترین_Type != c_m3Type_none and localType != c_m3Type_none and
                                           not Is_Sub_Type_از(پشته_بالاترین_Type, localType));
#إذا d_m3HasSnapshots
            إذا (o->local_Init_عمق and o->local_Init_عمق[local_شاخص] == UINT32_MAX) {
                o->local_Init_عمق[local_شاخص] = (u32)o->بلوک.عمق;
            }
#پایان_اگر
        }

        u16 local_جایگاه = Get_جایگاه_For_پشته_شاخص(o, local_شاخص);

        u16 preserve_جایگاه;
_       (پیدا_کن_Referenced_Local_Within_Current_بلوک(o, &preserve_جایگاه, local_جایگاه));  // preserve will be different than local, if referenced

        bool folded = false;

#إذا d_m3_Fold_مجموعه_Local
        // destination folding: when the value was produced by the immediately preceding op (nothing
        // emitted since, result on top in _r0, no copy-on-write preservation needed), retro-patch the
        // producer to a variant that writes straight to the local's slot and skip the SetSlot op
        إذا (o->foldPatchPC and preserve_جایگاه == local_جایگاه and Is_پشته_بالاترین_درون_ثبات(o) and Get_پشته_بالاترین_شاخص(o) == (i16)o->fold_پشته_شاخص) {
            IM3Operation foldOp = GetFoldOp(o->foldOpcode, o->foldForm);

            إذا (foldOp) {
                *(IM3Operation*)o->foldPatchPC = foldOp;     // retro-patch the producer
                Emit_جایگاه_جابه_جایی(o, local_جایگاه);                  // append its destination
                InvalidateFold(o);
                folded = true;
            }
        }
#پایان_اگر

        إذا (folded) {
            m3type_t type = Get_پشته_بالاترین_Type(o);
_           (برداشت(o));                              // the value now lives in the local's slot

            // A folded op writes its result through to the register as well, so a tee
            // can keep it there. Pushing the slot instead would make the next use that
            // wants a register reload the slot this op just stored to.
            إذا (i_opcode == c_waOp_teeLocal) {
_               (افزودن_ثبات(o, type));
            }
        } وإلا {
            إذا (preserve_جایگاه == local_جایگاه) {
_               (Copy_پشته_بالاترین_مقصد_جایگاه(o, local_جایگاه))
            } وإلا {
_               (Preserved_Copy_بالاترین_جایگاه(o, local_جایگاه, preserve_جایگاه))
            }

            إذا (i_opcode != c_waOp_teeLocal) {
_               (برداشت(o));
            }
        }
    } وإلا {
        _throw("local index out of bounds");
    }

    _catch: أرجع نتیجه;
}

ساكن
M3_نتیجه Compile_GetLocal (IM3Compilation o, m3opcode_t i_opcode)
{
_try {

    u32 local_شاخص;
_   (ReadLEB_u32(&local_شاخص, &o->wasm, o->wasm_انتها));                           m3log (compile, d_indent " (index = %u)", get_indention_string (o), local_شاخص);

    إذا (local_شاخص >= Get_Function_رقم_آرگومانها_And_Locals(o->function)) {
        _throw("local index out of bounds");
    }

    m3type_t type = Get_پشته_Type_From_Bottom(o, local_شاخص);
    u16      جایگاه = Get_جایگاه_For_پشته_شاخص(o, local_شاخص);

_   (افزودن(o, type, جایگاه));

    } _catch: أرجع نتیجه;
}

ساكن
M3_نتیجه Compile_GetGlobal (IM3Compilation o, M3Global* i_global)
{
    M3_نتیجه نتیجه;

    IM3Operation op = Is64_بیت_Type(i_global->type) ? op_GetGlobal_s64 : op_GetGlobal_s32;
_   (EmitOp(o, op));
    Emit_اشاره_گر(o, &i_global->i64_مقدار);
_   (افزودن_Allocated_جایگاه_And_Emit_2(o, i_global->type));

    _catch: أرجع نتیجه;
}

ساكن
M3_نتیجه Compile_مجموعه_Global (IM3Compilation o, M3Global* i_global)
{
    M3_نتیجه نتیجه = m3Err_none;

    إذا (i_global->isMutable) {
        // Spec: value type must match global type
        إذا (not Is_پشته_Polymorphic(o)) {
            m3type_t پشته_بالاترین_Type = Get_پشته_بالاترین_Type(o);
            _throwif(m3Err_typeMismatch, پشته_بالاترین_Type != c_m3Type_none and
                                           not Is_Sub_Type_از(پشته_بالاترین_Type, i_global->type));
        }

        IM3Operation op;
        m3type_t     type = Get_پشته_بالاترین_Type(o);

        إذا (Is_پشته_بالاترین_درون_ثبات(o)) {
            op = c_مجموعه_Global_Ops[پایه_Type_از(type)];
        } وإلا {
            op = Is64_بیت_Type(type) ? op_مجموعه_Global_s64 : op_مجموعه_Global_s32;
        }

_       (EmitOp(o, op));
        Emit_اشاره_گر(o, &i_global->i64_مقدار);

        إذا (Is_پشته_بالاترین_درون_جایگاه(o)) {
            Emit_جایگاه_جابه_جایی(o, Get_پشته_بالاترین_جایگاه_عدد(o));
        }

_       (برداشت(o));
    } وإلا {
        _throw(m3Err_settingImmutableGlobal);
    }

    _catch: أرجع نتیجه;
}

ساكن
M3_نتیجه Compile_Get_مجموعه_Global (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_نتیجه نتیجه = m3Err_none;

    u32 global_شاخص;
_   (ReadLEB_u32(&global_شاخص, &o->wasm, o->wasm_انتها));

    إذا (global_شاخص < o->module->رقم_Globals) {
        إذا (o->module->globals) {
            M3Global* global = &o->module->globals[global_شاخص];

            // Spec: a constant expression may only read an imported immutable
            // global. The module's own globals are counted before their
            // initializer is walked, so a bare index check would let one
            // reference itself. These describe how the module declared the
            // global, so they are checked before following any link.
            _throwif(m3_Err_globa_شاخص_بیرون_از_Bounds, o->is_Init_عبارت and not global->imported);
            _throwif(m3Err_wasmMalformed, o->is_Init_عبارت and global->isMutable);

            // an import linked to another module reads and writes that module's
            // cell, not the placeholder standing in for it here
            إذا (global->resolved) {
                global = global->resolved;
            }

_           ((i_opcode == c_waOp_getGlobal) ? Compile_GetGlobal(o, global) : Compile_مجموعه_Global(o, global));
        } وإلا {
            _throw(ErrorCompile(m3_Err_global_حافظه_Not_Allocated, o, "module '%s' is missing global memory", o->module->name));
        }
    } وإلا {
        _throw(m3_Err_globa_شاخص_بیرون_از_Bounds);
    }

    _catch: أرجع نتیجه;
}

ساكن
فراغ Emit_Patching_Branch_اشاره_گر (IM3Compilation o, IM3CompilationScope i_scope)
{
    pc_t patch       = Emit_اشاره_گر(o, i_scope->patches);                m3log (compile, "branch patch required at: %p", patch);
    i_scope->patches = patch;
}

ساكن
M3_نتیجه EmitPatchingBranch (IM3Compilation o, IM3CompilationScope i_scope)
{
    M3_نتیجه نتیجه = m3Err_none;

_   (EmitOp(o, op_Branch));
    Emit_Patching_Branch_اشاره_گر(o, i_scope);

    _catch: أرجع نتیجه;
}

#إذا d_m3HasExceptionHandling

// How many try_table handler records an exit from i_from out to i_target takes
// down with it. Walking outward from the block the branch sits in, every
// try_table scope up to and including the target is left behind - the target
// too, because landing on a block's label means that block is finished.
//
// A loop label is the exception, but a loop is never a try_table, so the count
// is the same either way: the loop itself is not on the list.
ساكن
u32 شمار_Try_قابها_مقصد_Exit (IM3CompilationScope i_from, IM3CompilationScope i_هدف)
{
    u32 شمار = 0;

    لكل (IM3CompilationScope scope = i_from; scope; scope = scope->outer) {
        إذا (scope->opcode == c_wa_Op_try_جدول) {
            ++شمار;
        }

        إذا (scope == i_هدف) {
            اكسر;
        }
    }

    أرجع شمار;
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
ساكن
u32 رقم_Try_قابها_مقصد_برداشت (IM3CompilationScope i_from, IM3CompilationScope i_هدف)
{
    إذا (i_هدف->عمق == 0) {
        أرجع 0;
    }

    // the walk only ever runs inward-to-outward; a target that is already
    // outside i_from has nothing between them
    إذا (not i_from or i_from->عمق < i_هدف->عمق) {
        أرجع 0;
    }

    أرجع شمار_Try_قابها_مقصد_Exit(i_from, i_هدف);
}


ساكن
M3_نتیجه Emit_برداشت_Try_قابها (IM3Compilation o, u32 i_رقم_قابها)
{
    M3_نتیجه نتیجه = m3Err_none;

    إذا (i_رقم_قابها) {
_       (EmitOp(o, op_برداشت_Handlers));
        EmitConstant32(o, i_رقم_قابها);
    }

    _catch: أرجع نتیجه;
}


// A return_call leaves the enclosing function before the callee runs, so no
// try_table in this function may catch what the callee throws. Neither shape of
// the instruction unwinds the native stack in time to arrange that on its own:
// op_ReturnCall tail-jumps with op_TryTable's frames still standing, and the
// fallback shape is an ordinary call sitting inside them. Either way the
// handler records have to come off first.
ساكن
M3_نتیجه Emit_برداشت_Try_قابها_For_Return_Call (IM3Compilation o)
{
    u32 شمار = 0;

    لكل (IM3CompilationScope scope = &o->بلوک; scope; scope = scope->outer) {
        إذا (scope->opcode == c_wa_Op_try_جدول) {
            ++شمار;
        }
    }

    أرجع Emit_برداشت_Try_قابها(o, شمار);
}

#وإلا

ساكن
M3_نتیجه Emit_برداشت_Try_قابها (IM3Compilation o, u32 i_رقم_قابها)
{
    (فراغ)o;
    (فراغ)i_رقم_قابها;
    أرجع m3Err_none;
}

#پایان_اگر // d_m3HasExceptionHandling

#إذا d_m3FuseBranch
// when the pending fold candidate is a compare whose result sits on top in _r0, return the fused
// variant that absorbs an immediately following br_if (i_isIf false) or if (i_isIf true)
ساكن
IM3Operation GetBranchFusionOp (IM3Compilation o, bool i_isIf)
{
    إذا (o->foldPatchPC and o->foldForm < 4 and Is_پشته_بالاترین_درون_ثبات(o) and Get_پشته_بالاترین_شاخص(o) == (i16)o->fold_پشته_شاخص) {
        ثابت M3FusedCmpOps* ops = GetFusedCmpOps(o->foldOpcode);

        إذا (ops) {
            أرجع i_isIf ? ops->ifOp[o->foldForm] : ops->branchIf[o->foldForm];
        }
    }

    أرجع عدم;
}
#پایان_اگر // d_m3FuseBranch

ساكن مضمن
IM3Operation Op_ContinueLoop (IM3Compilation o)
{
#إذا d_m3_Has_پشته_Switching
    إذا (M3_UNLIKELY(o->module->runtime and o->module->runtime->isSuspendable)) {
        أرجع op_ContinueLoop_Suspendable;
    }
#پایان_اگر
    أرجع op_ContinueLoop;
}

ساكن مضمن
IM3Operation Op_ContinueLoopIf (IM3Compilation o)
{
#إذا d_m3_Has_پشته_Switching
    إذا (M3_UNLIKELY(o->module->runtime and o->module->runtime->isSuspendable)) {
        أرجع op_ContinueLoopIf_Suspendable;
    }
#پایان_اگر
    أرجع op_ContinueLoopIf;
}


ساكن
M3_نتیجه Compile_Branch (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_نتیجه نتیجه;
    u32      رقم_Try_قابها = 0;

    u32 عمق;
_   (ReadLEB_u32(&عمق, &o->wasm, o->wasm_انتها));

    // Spec: br_if condition must be i32
    إذا (i_opcode == c_waOp_branchIf and not Is_پشته_Polymorphic(o)) {
        m3type_t condType = Get_پشته_بالاترین_Type(o);
        _throwif(m3Err_typeMismatch, condType != c_m3Type_none and condType != c_m3Type_i32);
    }

    IM3CompilationScope scope;
_   (Get_بلوک_Scope(o, &scope, عمق));

#إذا d_m3HasExceptionHandling
    رقم_Try_قابها = رقم_Try_قابها_مقصد_برداشت(&o->بلوک, scope);
#پایان_اگر

    // branch target is a loop (continue)
    إذا (scope->opcode == c_waOp_loop) {
        إذا (i_opcode == c_waOp_branchIf) {
            // a handler pop belongs on the taken path only, so it forces the
            // jump-over form even where the target takes no parameters
            إذا (Get_Func_Type_رقم_Params(scope->type) or رقم_Try_قابها) {
                IM3Operation op = Is_پشته_بالاترین_درون_ثبات(o) ? op_BranchIfPrologue_r : op_BranchIfPrologue_s;

_               (EmitOp(o, op));
_               (Emit_جایگاه_رقم_از_پشته_بالاترین_And_برداشت(o));

                pc_t* jump_مقصد = (pc_t*)Reserve_اشاره_گر(o);

_               (Emit_برداشت_Try_قابها(o, رقم_Try_قابها));

                إذا (not Is_پشته_Polymorphic(o)) {
_                   (Resolve_بلوک_Results(o, scope, /* isBranch: */ true));
                }

_               (EmitOp(o, Op_ContinueLoop(o)));
                Emit_اشاره_گر(o, scope->pc);
_               (رکورد_پشت_یال(o, scope, false));

                *jump_مقصد = GetPC(o);
            } وإلا {
                // move the condition to a register
_               (Copy_پشته_بالاترین_مقصد_ثبات(o, false));
_               (برداشت_Type(o, c_m3Type_i32));

_               (EmitOp(o, Op_ContinueLoopIf(o)));
                Emit_اشاره_گر(o, scope->pc);
_               (رکورد_پشت_یال(o, scope, true));
            }

            //          dump_type_stack(o);
        } وإلا // is c_waOp_branch
        {
_           (Emit_برداشت_Try_قابها(o, رقم_Try_قابها));

            // the branch operands become the loop's parameters on the next iteration
            إذا (not Is_پشته_Polymorphic(o)) {
_               (Resolve_بلوک_Results(o, scope, /* isBranch: */ true));
            }

_           (EmitOp(o, Op_ContinueLoop(o)));
            Emit_اشاره_گر(o, scope->pc);
_           (رکورد_پشت_یال(o, scope, false));
            o->بلوک.isPolymorphic = true;
        }
    } وإلا // forward branch
    {
        pc_t* jump_مقصد = عدم;

        bool isReturn         = (scope->عمق == 0);
        bool هدف_Has_Results = Get_Func_Type_رقم_Results(scope->type);

        إذا (i_opcode == c_waOp_branchIf) {
            إذا (هدف_Has_Results or isReturn or رقم_Try_قابها) {
                IM3Operation op = Is_پشته_بالاترین_درون_ثبات(o) ? op_BranchIfPrologue_r : op_BranchIfPrologue_s;

_               (EmitOp(o, op));
_               (Emit_جایگاه_رقم_از_پشته_بالاترین_And_برداشت(o)); // condition

                // this is continuation point, if the branch isn't taken
                jump_مقصد = (pc_t*)Reserve_اشاره_گر(o);
            } وإلا {
#إذا d_m3FuseBranch
                IM3Operation fuseOp = GetBranchFusionOp(o, false);

                إذا (fuseOp) {
                    // absorb the branch into the compare that produced the condition
                    *(IM3Operation*)o->foldPatchPC = fuseOp;
                    InvalidateFold(o);

_                   (برداشت(o));                      // the condition is consumed inside the fused op

                    Emit_Patching_Branch_اشاره_گر(o, scope);
                    اذهب _catch;
                }
#پایان_اگر
                IM3Operation op = Is_پشته_بالاترین_درون_ثبات(o) ? op_BranchIf_r : op_BranchIf_s;

_               (EmitOp(o, op));
_               (Emit_جایگاه_رقم_از_پشته_بالاترین_And_برداشت(o)); // condition

                Emit_Patching_Branch_اشاره_گر(o, scope);
                اذهب _catch;
            }
        }

        إذا (not Is_پشته_Polymorphic(o)) {
            إذا (isReturn) {
_               (Return_مقادیر(o, scope, true));
_               (EmitOp(o, op_Return));
            } وإلا {
_               (Emit_برداشت_Try_قابها(o, رقم_Try_قابها));

_               (Resolve_بلوک_Results(o, scope, true));
_               (EmitPatchingBranch(o, scope));
            }
        }

        إذا (jump_مقصد) {
            *jump_مقصد = GetPC(o);
        }

        إذا (i_opcode == c_waOp_branch) {
_           (مجموعه_پشته_Polymorphic(o));
        }
    }

    _catch: أرجع نتیجه;
}

ساكن
M3_نتیجه Compile_Branch_جدول (IM3Compilation o, m3opcode_t i_opcode)
{
    // the page a continue-op page is standing in for; non-NULL only while that
    // page is installed in o->page, so the catch below knows to put it back
    IM3_کد_صفحه displaced_صفحه = عدم;
    // indexed by label depth: the continuation stub already emitted for that target
    // within this table, or NULL if this table has not reached it yet
    pc_t* هدف_Stubs = عدم;
_try {
    u32 هدف_شمار;
_   (ReadLEB_u32(&هدف_شمار, &o->wasm, o->wasm_انتها));

    // Spec: validate that the branch index operand is i32
    إذا (not Is_پشته_Polymorphic(o)) {
        m3type_t شاخص_Type = Get_پشته_بالاترین_Type(o);
        _throwif(m3Err_typeMismatch, شاخص_Type != c_m3Type_none and شاخص_Type != c_m3Type_i32);
    }

_   (Preserve_ثبات_If_Occupied(o, c_m3Type_i64));         // move branch operand to a slot
    u16 جایگاه = Get_پشته_بالاترین_جایگاه_عدد(o);
_   (برداشت(o));

    // OPTZ: according to spec: "forward branches that target a control instruction with a non-empty
    // result type consume matching operands first and push them back on the operand stack after unwinding"
    // So, this move-to-reg is only necessary if the target scopes have a type.

    u32 رقم_کد_سطرها = هدف_شمار + 4; // 3 => IM3Operation + slot + target_count + default_target
_   (Ensure_کد_صفحه_رقم_سطرها(o, رقم_کد_سطرها));

_   (EmitOp(o, op_Branch_جدول));
    Emit_جایگاه_جابه_جایی(o, جایگاه);
    EmitConstant32(o, هدف_شمار);

    IM3_کد_صفحه continue_Op_صفحه = عدم;

    // The label types are checked by ValidateFunction, which runs under the same
    // d_m3EnableValidation guard just before this compilation pass. Comparing each
    // target against the default here would be wrong anyway: the spec matches every
    // target against the operand stack, so in unreachable code the operands are the
    // bottom type and the targets may legitimately differ (unreached-valid.wast).

    ++هدف_شمار; // include default

    // Every entry of one br_table is resolved from the same compilation state - each
    // stub restores whatever it touched - so two entries naming the same label emit
    // byte-identical epilogues. Emit one and point both table entries at it. A label
    // deeper than the current block is rejected by GetBlockScope below, which is what
    // bounds this array.
    هدف_Stubs = m3_AllocArray(pc_t, (size_t)o->بلوک.عمق + 1);
    _throwifnull(هدف_Stubs);

    لكل (u32 i = 0; i < هدف_شمار; ++i) {
        u32 هدف;
_       (ReadLEB_u32(&هدف, &o->wasm, o->wasm_انتها));

        IM3CompilationScope scope;
_       (Get_بلوک_Scope(o, &scope, هدف));

        إذا (هدف_Stubs[هدف]) {
            Emit_اشاره_گر(o, هدف_Stubs[هدف]);
            استمر;
        }

        u32 رقم_Try_قابها = 0;
#إذا d_m3HasExceptionHandling
        رقم_Try_قابها = رقم_Try_قابها_مقصد_برداشت(&o->بلوک, scope);
#پایان_اگر

        // A forward branch to a block that carries no results and leaves no try
        // frames behind has nothing to do on the way out but arrive. There is no
        // epilogue to hold, so the table entry becomes the patch site itself and the
        // block's address is written into it when the block ends - no stub, and no
        // code page to put one on. In unreachable code the same is true whatever the
        // block's type, since nothing is emitted for that case either way.
        //
        // Each such entry is patched on its own, so none is recorded in targetStubs:
        // there is no shared stub for a later entry naming this label to point at.
        إذا (scope->opcode != c_waOp_loop and scope->عمق > 0 and رقم_Try_قابها == 0 and
            (Is_پشته_Polymorphic(o) or Get_Func_Type_رقم_Results(scope->type) == 0)) {
            Emit_Patching_Branch_اشاره_گر(o, scope);
            استمر;
        }

_       (Acquire_Compilation_کد_صفحه(o, &continue_Op_صفحه));

        pc_t شروع_PC  = Get_صفحه_PC(continue_Op_صفحه);
        displaced_صفحه = o->صفحه;
        o->صفحه       = continue_Op_صفحه;

        إذا (scope->opcode == c_waOp_loop) {
_           (Emit_برداشت_Try_قابها(o, رقم_Try_قابها));

            إذا (not Is_پشته_Polymorphic(o)) {
_               (Resolve_بلوک_Results(o, scope, true));
            }

_           (EmitOp(o, Op_ContinueLoop(o)));
            Emit_اشاره_گر(o, scope->pc);
_           (رکورد_پشت_یال(o, scope, false));
        } وإلا {
            إذا (not Is_پشته_Polymorphic(o)) {
                إذا (scope->عمق == 0) {
_                   (Return_مقادیر(o, scope, true));
_                   (EmitOp(o, op_Return));
                } وإلا {
_                   (Emit_برداشت_Try_قابها(o, رقم_Try_قابها));

_                   (Resolve_بلوک_Results(o, scope, true));

_                   (EmitPatchingBranch(o, scope));
                }
            }
        }

        IM3_کد_صفحه return_مقصد = displaced_صفحه;
        displaced_صفحه        = عدم;

        Release_Compilation_کد_صفحه(o);
        o->صفحه = return_مقصد;

        Emit_اشاره_گر(o, شروع_PC);
        هدف_Stubs[هدف] = شروع_PC;
    }

_   (مجموعه_پشته_Polymorphic(o));
} _catch:

    // thrown out of the loop above with a continue-op page installed: release it
    // and restore the one it displaced, which the caller's catch then releases.
    // Otherwise the displaced page is on no list and simply leaks.
    إذا (displaced_صفحه) {
        Release_Compilation_کد_صفحه(o);
        o->صفحه = displaced_صفحه;
    }

    m3_Free(هدف_Stubs);

    أرجع نتیجه;
}

ساكن
M3_نتیجه Compile_Call_آرگومانها_And_Return (IM3Compilation o, u16* o_پشته_جابه_جایی, IM3FuncType i_type, bool i_is_غیرمستقیم)
{
_try {

    u16 بالاترین_جایگاه = Get_Max_استفاده_شده_جایگاه_Plus_One(o);

    // force use of at least one stack slot; this is to help ensure
    // the m3 stack overflows (and traps) before the native stack can overflow.
    // e.g. see Wasm spec test 'runaway' in call.wast
    بالاترین_جایگاه = M3_MAX(1, بالاترین_جایگاه);

    // stack frame is 64-bit aligned
    Align_جایگاه_مقصد_Type(&بالاترین_جایگاه, c_m3Type_i64);

    *o_پشته_جابه_جایی = بالاترین_جایگاه;

    // wait to pop this here so that topSlot search is correct
    إذا (i_is_غیرمستقیم) {
_       (برداشت(o));
    }

    u16 رقم_آرگومانها = Get_Func_Type_رقم_Params(i_type);
    u16 رقم_Rets = Get_Func_Type_رقم_Results(i_type);

    u16 arg_بالاترین = بالاترین_جایگاه + (رقم_آرگومانها + رقم_Rets) * c_io_جایگاه_شمار;

    Touch_جایگاه(o, arg_بالاترین - 1);

    طالما (رقم_آرگومانها--) {
_       (Copy_پشته_بالاترین_مقصد_جایگاه(o, arg_بالاترین -= c_io_جایگاه_شمار));
_       (برداشت(o));
    }

    u16 i = 0;
    طالما (رقم_Rets--) {
        m3type_t type = Get_Func_Type_نتیجه_Type(i_type, i++);

_       (افزودن(o, type, بالاترین_جایگاه));
_       (MarkSlotsAllocatedByType(o, بالاترین_جایگاه, type));

        بالاترین_جایگاه += c_io_جایگاه_شمار;
    }

    } _catch: أرجع نتیجه;
}

// A tail call hands the current frame over to the callee instead of stacking a new one.
// The callee's results have to be the enclosing function's results, so both use the same
// return slots and only the arguments move: they're staged above the caller's stack here
// (the argument expressions can still be reading the args/locals they're about to land on)
// and slid down onto the frame base at runtime, by op_ReturnCall[Indirect].
ساكن
M3_نتیجه Compile_دم_Call_آرگومانها (IM3Compilation o, u16* o_پشته_جابه_جایی, u16* o_رقم_Arg_Slots, IM3FuncType i_type, bool i_is_غیرمستقیم)
{
_try {
    IM3FuncType funcType = o->function->funcType;

    u16 رقم_Rets = Get_Func_Type_رقم_Results(i_type);
    _throwif(m3Err_typeMismatch, رقم_Rets != Get_Func_Type_رقم_Results(funcType));

    لكل (u16 i = 0; i < رقم_Rets; ++i) {
        _throwif(m3Err_typeMismatch, Get_Func_Type_نتیجه_Type(i_type, i) != Get_Func_Type_نتیجه_Type(funcType, i));
    }

    u16 بالاترین_جایگاه = Get_Max_استفاده_شده_جایگاه_Plus_One(o);

    بالاترین_جایگاه = M3_MAX(1, بالاترین_جایگاه);

    // stack frame is 64-bit aligned
    Align_جایگاه_مقصد_Type(&بالاترین_جایگاه, c_m3Type_i64);

    *o_پشته_جابه_جایی = بالاترین_جایگاه;

    // wait to pop this here so that topSlot search is correct
    إذا (i_is_غیرمستقیم) {
_       (برداشت(o));
    }

    u16 رقم_آرگومانها     = Get_Func_Type_رقم_Params(i_type);
    u16 رقم_Arg_Slots = رقم_آرگومانها * c_io_جایگاه_شمار;

    *o_رقم_Arg_Slots = رقم_Arg_Slots;

    u16 arg_بالاترین = بالاترین_جایگاه + رقم_Arg_Slots;
    _throwif(m3_Err_function_پشته_سرریز, arg_بالاترین > d_m3MaxFunctionSlots);

    // the staging area is written by the caller, so op_Entry has to account for it
    Touch_جایگاه(o, arg_بالاترین - 1);

    طالما (رقم_آرگومانها--) {
_       (Copy_پشته_بالاترین_مقصد_جایگاه(o, arg_بالاترین -= c_io_جایگاه_شمار));
_       (برداشت(o));
    }

    } _catch: أرجع نتیجه;
}


#إذا d_m3HasTypedRefs

// call_ref $t: the callee is the reference on top of the stack, so the operand
// order is the same as call_indirect's, minus the table.
ساكن
M3_نتیجه Compile_CallRef (IM3Compilation o, m3opcode_t i_opcode)
{
_try {
    u32 type_شاخص;
_   (ReadLEB_u32(&type_شاخص, &o->wasm, o->wasm_انتها));

    _throwif("function call type index out of range", type_شاخص >= o->module->رقم_Func_Types);

    IM3FuncType type = o->module->funcTypes[type_شاخص];

    إذا (not Is_پشته_Polymorphic(o)) {
        // whatever shape the reference has, it must be a function of this type
        m3type_t refType = Get_پشته_بالاترین_Type(o);
        _throwif(m3Err_typeMismatch, not Is_Sub_Type_از(refType, Ref_Type_از_Func_Type(type, false)));
    }

    إذا (Is_پشته_بالاترین_درون_ثبات(o)) {
_       (Preserve_ثبات_If_Occupied(o, c_m3Type_funcref));
    }

    u16 function_جایگاه = Get_پشته_بالاترین_جایگاه_عدد(o);

    bool isReturnCall = (i_opcode == c_waOp_returnCallRef);
    bool use_دم_Call  = isReturnCall and d_m3_Can_دم_Call;

    u16 exec_بالاترین, رقم_Arg_Slots = 0;

    إذا (use_دم_Call) {
_       (Compile_دم_Call_آرگومانها(o, &exec_بالاترین, &رقم_Arg_Slots, type, true));
    } وإلا {
_       (Compile_Call_آرگومانها_And_Return(o, &exec_بالاترین, type, true));
    }

#  إذا d_m3HasExceptionHandling
    إذا (isReturnCall) {
_       (Emit_برداشت_Try_قابها_For_Return_Call(o));
    }
#  endif

_   (EmitOp(o, use_دم_Call ? op_ReturnCallRef : op_CallRef));
    Emit_جایگاه_جابه_جایی(o, function_جایگاه);
    Emit_اشاره_گر(o, type);
    Emit_جایگاه_جابه_جایی(o, exec_بالاترین);

    إذا (not use_دم_Call) {
_       (رکورد_Call(o, Get_Func_Type_رقم_Results(type), exec_بالاترین, isReturnCall));
    }

    إذا (use_دم_Call) {
        Emit_جایگاه_جابه_جایی(o, o->function->رقم_Ret_Slots);
        EmitConstant32(o, رقم_Arg_Slots);

_       (مجموعه_پشته_Polymorphic(o));
    } وإلا إذا (isReturnCall) {
_       (Compile_Return(o, i_opcode));
    }

} _catch:
    أرجع نتیجه;
}


// ref.as_non_null only sharpens the type; the value is unchanged, so the stack
// entry is re-pushed with the null stripped out of its type.
ساكن
M3_نتیجه Compile_Ref_AsNonNull (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_نتیجه نتیجه = m3Err_none;

    إذا (not Is_پشته_Polymorphic(o)) {
        m3type_t type = Get_پشته_بالاترین_Type(o);
        _throwif(m3Err_typeMismatch, not IsRefType(type));

        // op_RefAsNonNull only knows how to read its operand out of a slot; a
        // value still sitting in the register has to be spilled first, the way
        // every other single-op reader of the stack top does
        إذا (Is_پشته_بالاترین_درون_ثبات(o)) {
_           (Preserve_ثبات_If_Occupied(o, type));
        }

        u16 جایگاه = Get_پشته_بالاترین_جایگاه_عدد(o);

_       (EmitOp(o, op_RefAsNonNull));
        Emit_جایگاه_جابه_جایی(o, جایگاه);

        m3type_t nonNull = IsSpelledRefType(type)
                             ? (m3type_t)(type | d_m3Type_refNonNull)
                             : (m3type_t)(d_m3Type_ref | d_m3Type_refNonNull | d_m3_Type_انباشته_Abstract |
                                          ((پایه_Type_از(type) == c_m3Type_externref) ? d_m3Type_refExtern : 0));

        // the slot keeps the one reference it already had; a Pop()/Push() of
        // the same slot would drop that reference's count without anything
        // to put it back, so the type above it is corrected in place instead
        o->type_پشته[Get_پشته_بالاترین_شاخص(o)] = nonNull;
    }

    _catch: أرجع نتیجه;
}

#پایان_اگر // d_m3HasTypedRefs


// How wide a table operand is read at comes from the table it names, so a slot
// holding a narrower type would be read past what was allocated for it. The
// validator refuses that already; this keeps a build without one from reaching
// the interpreter with it. c_m3Type_none means the operand is not on the
// dynamic stack to inspect, which is not this check's business.
ساكن
M3_نتیجه Check_عملوند_Type (IM3Compilation o, u16 i_عمق_From_بالاترین, m3type_t i_type)
{
    m3type_t actual;

    إذا (Is_پشته_Polymorphic(o)) {
        أرجع m3Err_none;
    }

    actual = Get_پشته_Type_From_بالاترین(o, i_عمق_From_بالاترین);

    أرجع (actual == c_m3Type_none or پایه_Type_از(actual) == i_type) ? m3Err_none
                                                                     : m3Err_typeMismatch;
}


// Swaps the _mem register onto a given memory. Only ever emitted in pairs --
// see op_SetMemory.
ساكن
M3_نتیجه Emit_مجموعه_حافظه_Ptr (IM3Compilation o, IM3_حافظه i_حافظه)
{
    M3_نتیجه نتیجه = m3Err_none;

_   (EmitOp(o, op_مجموعه_حافظه));
    Emit_اشاره_گر(o, i_حافظه);

    _catch: أرجع نتیجه;
}


// ...onto one of the compiling module's own memories, by index. Only reached
// with a non-zero index under multi-memory, but the index has already been
// bounds-checked by ReadMemoryIndex either way.
ساكن مضمن
M3_نتیجه Emit_مجموعه_حافظه (IM3Compilation o, u32 i_حافظه_Idx)
{
#إذا d_m3_Has_Multi_حافظه
    أرجع Emit_مجموعه_حافظه_Ptr(o, o->module->memories[i_حافظه_Idx]);
#وإلا
    (فراغ)o;
    (فراغ)i_حافظه_Idx;
    أرجع m3_Err_unknown_حافظه;
#پایان_اگر
}

ساكن
M3_نتیجه Compile_Call (IM3Compilation o, m3opcode_t i_opcode)
{
_try {
    u32 function_شاخص;
_   (ReadLEB_u32(&function_شاخص, &o->wasm, o->wasm_انتها));

    IM3Function function = Module_GetFunction(o->module, function_شاخص);

    إذا (function) {                                                     m3log (compile, d_indent " (func= [%d] '%s'; args= %d)",
                                                                                get_indention_string (o), function_شاخص, m3_GetFunctionName (function), function->funcType->رقم_آرگومانها);
        // an import linked to another module's export runs that module's body
        IM3Function هدف = Function_Implementation(function);

        إذا (هدف->module) {
            bool isReturnCall = (i_opcode == c_waOp_returnCall);

            // The callee's body runs against its own module's memory, so a call
            // that crosses modules is bracketed by a pair of op_SetMemory. A
            // tail call never returns to run the second one, so those compile
            // as a plain call followed by a return instead.
            bool crossModule = (هدف->module != o->module);
            bool use_دم_Call = isReturnCall and d_m3_Can_دم_Call and not crossModule;

            u16 جایگاه_بالاترین, رقم_Arg_Slots = 0;

            إذا (use_دم_Call) {
_               (Compile_دم_Call_آرگومانها(o, &جایگاه_بالاترین, &رقم_Arg_Slots, هدف->funcType, false));
            } وإلا {
_               (Compile_Call_آرگومانها_And_Return(o, &جایگاه_بالاترین, هدف->funcType, false));
            }

            IM3Operation op;
            ثابت فراغ*  عملوند;

            إذا (هدف->compiled) {
                op      = use_دم_Call ? op_ReturnCall : op_Call;
                عملوند = هدف->compiled;
            } وإلا {
                op      = use_دم_Call ? op_CompileReturnCall : op_Compile;
                عملوند = هدف;
            }

#إذا d_m3HasExceptionHandling
            إذا (isReturnCall) {
_               (Emit_برداشت_Try_قابها_For_Return_Call(o));
            }
#پایان_اگر

            إذا (crossModule) {
_               (Emit_مجموعه_حافظه_Ptr(o, Module_Memory0(هدف->module)));
            }

_           (EmitOp(o, op));
            Emit_اشاره_گر(o, عملوند);
            Emit_جایگاه_جابه_جایی(o, جایگاه_بالاترین);

            إذا (not use_دم_Call) {
                // a suspend inside the callee leaves this frame waiting here, its
                // results not yet written
_               (رکورد_Call(o, Get_Func_Type_رقم_Results(هدف->funcType), جایگاه_بالاترین, isReturnCall));
            }

            إذا (use_دم_Call) {
                Emit_جایگاه_جابه_جایی(o, o->function->رقم_Ret_Slots);
                EmitConstant32(o, رقم_Arg_Slots);

_               (مجموعه_پشته_Polymorphic(o));
            } وإلا {
                إذا (crossModule) {
_                   (Emit_مجموعه_حافظه_Ptr(o, Module_Memory0(o->module)));
                }

                إذا (isReturnCall) {
_                   (Compile_Return(o, i_opcode));
                }
            }
        } وإلا {
            _throw(ErrorCompile(m3Err_functionImportMissing, o, "'%s.%s'", GetFunctionImportModuleName(function), m3_GetFunctionName(function)));
        }
    } وإلا {
        _throw(ErrorCompile(m3Err_functionLookupFailed, o, "index: %d", function_شاخص));
    }

    } _catch: أرجع نتیجه;
}

ساكن
M3_نتیجه Compile_Call_غیرمستقیم (IM3Compilation o, m3opcode_t i_opcode)
{
_try {
    u32 type_شاخص;
_   (ReadLEB_u32(&type_شاخص, &o->wasm, o->wasm_انتها));

    u32 جدول_شاخص;
_   (ReadLEB_u32(&جدول_شاخص, &o->wasm, o->wasm_انتها));

    _throwif("function call type index out of range", type_شاخص >= o->module->رقم_Func_Types);
    _throwif("table index out of range", جدول_شاخص >= o->module->رقم_Tables);
    _throwif(m3Err_typeMismatch, پایه_Type_از(o->module->tables[جدول_شاخص]->type) != c_m3Type_funcref);

_   (Check_عملوند_Type(o, 0, جدول_Addr_Type(o->module->tables[جدول_شاخص])));

    إذا (Is_پشته_بالاترین_درون_ثبات(o)) {
_       (Preserve_ثبات_If_Occupied(o, c_m3Type_i32));
    }

    u16 جدول_شاخص_جایگاه = Get_پشته_بالاترین_جایگاه_عدد(o);

    bool isReturnCall = (i_opcode == c_wa_Op_return_Call_غیرمستقیم);
    bool use_دم_Call  = isReturnCall and d_m3_Can_دم_Call;

    u16         exec_بالاترین, رقم_Arg_Slots = 0;
    IM3FuncType type = o->module->funcTypes[type_شاخص];

    إذا (use_دم_Call) {
_       (Compile_دم_Call_آرگومانها(o, &exec_بالاترین, &رقم_Arg_Slots, type, true));
    } وإلا {
_       (Compile_Call_آرگومانها_And_Return(o, &exec_بالاترین, type, true));
    }

#إذا d_m3HasExceptionHandling
    إذا (isReturnCall) {
_       (Emit_برداشت_Try_قابها_For_Return_Call(o));
    }
#پایان_اگر

    // the table comes first: how wide its indexes are is what says how much of
    // the slot below holds one
_   (EmitOp(o, use_دم_Call ? op_Return_Call_غیرمستقیم : op_Call_غیرمستقیم));
    Emit_اشاره_گر(o, o->module->tables[جدول_شاخص]);
    Emit_جایگاه_جابه_جایی(o, جدول_شاخص_جایگاه);
    Emit_اشاره_گر(o, type);
    Emit_جایگاه_جابه_جایی(o, exec_بالاترین);

    إذا (not use_دم_Call) {
_       (رکورد_Call(o, Get_Func_Type_رقم_Results(type), exec_بالاترین, isReturnCall));
    }

    إذا (use_دم_Call) {
        Emit_جایگاه_جابه_جایی(o, o->function->رقم_Ret_Slots);
        EmitConstant32(o, رقم_Arg_Slots);

_       (مجموعه_پشته_Polymorphic(o));
    } وإلا إذا (isReturnCall) {
_       (Compile_Return(o, i_opcode));
    }

} _catch:
    أرجع نتیجه;
}

// Reads the memory index a memory instruction names, and checks it against the
// module's index space. Without multi-memory the immediate is a reserved byte
// that has to be zero, which is the same check.
ساكن
M3_نتیجه Read_حافظه_شاخص (IM3Compilation o, u32* o_حافظه_Idx)
{
    M3_نتیجه نتیجه;

_   (ReadLEB_u32(o_حافظه_Idx, &o->wasm, o->wasm_انتها));

    _throwif(m3_Err_unknown_حافظه, *o_حافظه_Idx >= o->module->رقم_Memories);

    _catch: أرجع نتیجه;
}

ساكن
M3_نتیجه Compile_حافظه_اندازه (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_نتیجه نتیجه;

    u32 حافظه_Idx;

    // A page count is given in the memory's own address type. Declared up here
    // so the throws below don't jump over its initialization.
    m3type_t addrType = c_m3Type_i32;

_   (Read_حافظه_شاخص(o, &حافظه_Idx));

    addrType = حافظه_Addr_Type(o->module->memories[حافظه_Idx]);

_   (Preserve_ثبات_If_Occupied(o, addrType));

    إذا (حافظه_Idx) {
_       (Emit_مجموعه_حافظه(o, حافظه_Idx));
    }

    // op_MemSize writes the whole register either way; the slot it is pushed
    // to is what decides how much of it the module gets to see
_   (EmitOp(o, op_Mem_اندازه));

    إذا (حافظه_Idx) {
_       (Emit_مجموعه_حافظه(o, 0));
    }

_   (افزودن_ثبات(o, addrType));

    _catch: أرجع نتیجه;
}

ساكن
M3_نتیجه Compile_حافظه_Grow (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_نتیجه نتیجه;

    u32 حافظه_Idx;

    // see Compile_Memory_Size
    m3type_t addrType = c_m3Type_i32;

_   (Read_حافظه_شاخص(o, &حافظه_Idx));

    addrType = حافظه_Addr_Type(o->module->memories[حافظه_Idx]);

_   (Copy_پشته_بالاترین_مقصد_ثبات(o, false));
_   (برداشت_Type(o, addrType));

    إذا (حافظه_Idx) {
_       (Emit_مجموعه_حافظه(o, حافظه_Idx));
    }

#إذا d_m3HasMemory64
_   (EmitOp(o, (addrType == c_m3Type_i64) ? op_MemGrow64 : op_MemGrow));
#وإلا
_   (EmitOp(o, op_MemGrow));
#پایان_اگر

    إذا (حافظه_Idx) {
_       (Emit_مجموعه_حافظه(o, 0));
    }

_   (افزودن_ثبات(o, addrType));

    _catch: أرجع نتیجه;
}

ساكن
M3_نتیجه Compile_حافظه_Copy_Fill (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_نتیجه نتیجه = m3Err_none;

    // memory.copy names the destination first and then the source; memory.fill
    // names only the one it writes.
    u32  منبع_حافظه_Idx = 0, هدف_حافظه_Idx = 0;
    bool isCopy = (i_opcode == c_wa_Op_حافظه_Copy);

    // Each address is typed by the memory it belongs to. The length follows
    // the narrower of the two, so it is 64-bit only when both are - which for
    // memory.fill, naming one memory twice over, means whenever that one is.
    // Declared up here so the throws below don't jump over them.
    m3type_t هدف_Type = c_m3Type_i32;
    m3type_t منبع_Type = c_m3Type_i32;
    m3type_t درازی_Type = c_m3Type_i32;

    _throwif(m3Err_wasmMalformed, o->module->رقم_Memories == 0);

_   (Read_حافظه_شاخص(o, &هدف_حافظه_Idx));

    إذا (isCopy) {
_       (Read_حافظه_شاخص(o, &منبع_حافظه_Idx));
    }

    هدف_Type = حافظه_Addr_Type(o->module->memories[هدف_حافظه_Idx]);
    منبع_Type = isCopy ? حافظه_Addr_Type(o->module->memories[منبع_حافظه_Idx])
                        : هدف_Type;
    درازی_Type = (هدف_Type == c_m3Type_i64 and منبع_Type == c_m3Type_i64)
                   ? c_m3Type_i64
                   : c_m3Type_i32;

_   (Copy_پشته_بالاترین_مقصد_ثبات(o, false));

#إذا d_m3_Has_Multi_حافظه
    إذا (isCopy and منبع_حافظه_Idx != هدف_حافظه_Idx) {
        // two memories at once: _mem can only name one, so the op takes both
_       (EmitOp(o, op_MemCopy_x));
        Emit_اشاره_گر(o, o->module->memories[هدف_حافظه_Idx]);
        Emit_اشاره_گر(o, o->module->memories[منبع_حافظه_Idx]);

        // ...and, since they need not be addressed alike, how wide to read
        // each of the operands: bit 0 the destination, bit 1 the source
        EmitConstant32(o, ((هدف_Type == c_m3Type_i64) ? 0x1u : 0u) |
                            ((منبع_Type == c_m3Type_i64) ? 0x2u : 0u));
    } وإلا
#پایان_اگر
    {
        IM3Operation op = isCopy ? op_MemCopy : op_MemFill;

#إذا d_m3HasMemory64
        إذا (هدف_Type == c_m3Type_i64) {
            op = isCopy ? op_MemCopy64 : op_MemFill64;
        }
#پایان_اگر

        إذا (هدف_حافظه_Idx) {
_           (Emit_مجموعه_حافظه(o, هدف_حافظه_Idx));
        }

_       (EmitOp(o, op));
    }

    // the length is in the register; the other two are named by slot, at
    // whatever width their stack entries were pushed with
_   (برداشت_Type(o, درازی_Type));
_   (Emit_جایگاه_رقم_از_پشته_بالاترین_And_برداشت(o));      // the source, or the byte to fill with
_   (Emit_جایگاه_رقم_از_پشته_بالاترین_And_برداشت(o));      // the destination

    إذا (هدف_حافظه_Idx and not (isCopy and منبع_حافظه_Idx != هدف_حافظه_Idx)) {
_       (Emit_مجموعه_حافظه(o, 0));
    }

    _catch: أرجع نتیجه;
}


// memory.init and data.drop address a data segment by index. Both are only valid
// when a data count section declared the segments up front.
ساكن
M3_نتیجه Read_داده_Segment (IM3Compilation o, M3_داده_Segment** o_segment)
{
    M3_نتیجه نتیجه = m3Err_none;

    u32 index;
_   (ReadLEB_u32(&index, &o->wasm, o->wasm_انتها));

    _throwif("data count section required", not o->module->has_داده_شمار);
    _throwif(m3Err_wasmMalformed, index >= o->module->رقم_داده_Segments);

    *o_segment = &o->module->داده_Segments[index];

    _catch: أرجع نتیجه;
}


M3_نتیجه Compile_حافظه_Init (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_نتیجه نتیجه = m3Err_none;

    M3_داده_Segment* segment = عدم;
    u32            حافظه_Idx;

    _throwif(m3Err_wasmMalformed, o->module->رقم_Memories == 0);

_   (Read_داده_Segment(o, &segment));
_   (Read_حافظه_شاخص(o, &حافظه_Idx));

_   (Copy_پشته_بالاترین_مقصد_ثبات(o, false));

    إذا (حافظه_Idx) {
_       (Emit_مجموعه_حافظه(o, حافظه_Idx));
    }

#إذا d_m3HasMemory64
    // only the destination follows the memory's address type: a data segment
    // is indexed as an i32 however the memory it is copied into is addressed
_   (EmitOp(o, o->module->memories[حافظه_Idx]->isMemory64 ? op_MemInit64 : op_MemInit));
#وإلا
_   (EmitOp(o, op_MemInit));
#پایان_اگر
    Emit_اشاره_گر(o, segment);
_   (برداشت_Type(o, c_m3Type_i32));
_   (Emit_جایگاه_رقم_از_پشته_بالاترین_And_برداشت(o));
_   (Emit_جایگاه_رقم_از_پشته_بالاترین_And_برداشت(o));

    إذا (حافظه_Idx) {
_       (Emit_مجموعه_حافظه(o, 0));
    }

    _catch: أرجع نتیجه;
}


#إذا d_m3HasRefTypes

ساكن
M3_نتیجه Compile_Select (IM3Compilation o, m3opcode_t i_opcode);

// select with an explicit result type vector. The types only matter to the
// validator; the operands are laid out exactly as for the untyped select.
M3_نتیجه Compile_Select_Typed (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_نتیجه نتیجه = m3Err_none;

    u32 رقم_Types;
_   (ReadLEB_u32(&رقم_Types, &o->wasm, o->wasm_انتها));
    _throwif(m3Err_wasmMalformed, رقم_Types != 1);

    i8 waType;
    u8 type;
_   (ReadLEB_i7(&waType, &o->wasm, o->wasm_انتها));
_   (NormalizeType(&type, waType));

_   (Compile_Select(o, i_opcode));

    _catch: أرجع نتیجه;
}


// A reference is one pointer-sized word and null is 0, so null-testing it is
// just an integer eqz of the matching width.
#  إذا M3_SIZEOF_PTR == 8
#    define d_m3RefIsNull_r   op_i64_Equal_مقصد_Zero_r
#    define d_m3RefIsNull_s   op_i64_Equal_مقصد_Zero_s
#  وإلا
#    define d_m3RefIsNull_r   op_i32_Equal_مقصد_Zero_r
#    define d_m3RefIsNull_s   op_i32_Equal_مقصد_Zero_s
#  endif

ساكن
M3_نتیجه ReadRefType (IM3Compilation o, m3type_t* o_type)
{
    M3_نتیجه نتیجه = m3Err_none;

#  إذا d_m3HasTypedRefs
    // ref.null names a heap type: func, extern, or a function type index
    m3type_t انباشته_بیتها;
_   (Parse_انباشته_Type(o->module, &انباشته_بیتها, &o->wasm, o->wasm_انتها));

    *o_type = d_m3Type_ref | انباشته_بیتها;
#  وإلا
    i8 waType;
    u8 plainType;
_   (ReadLEB_i7(&waType, &o->wasm, o->wasm_انتها));
_   (NormalizeType(&plainType, waType));
    *o_type = plainType;
    _throwif(m3Err_wasmMalformed, not IsRefType(*o_type));
#  endif

    _catch: أرجع نتیجه;
}


M3_نتیجه Compile_Ref_Null (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_نتیجه نتیجه = m3Err_none;

    m3type_t type;
_   (ReadRefType(o, &type));
_   (افزودن_Const(o, 0, type));

    _catch: أرجع نتیجه;
}


M3_نتیجه Compile_Ref_IsNull (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_نتیجه نتیجه = m3Err_none;

    IM3Operation op;

    إذا (not Is_پشته_Polymorphic(o)) {
        _throwif(m3Err_typeMismatch, not IsRefType(Get_پشته_بالاترین_Type(o)));
    }

    إذا (Is_پشته_بالاترین_درون_ثبات(o)) {
        op = d_m3RefIsNull_r;
    } وإلا {
_       (Preserve_ثبات_If_Occupied(o, c_m3Type_i32));
        op = d_m3RefIsNull_s;
    }

_   (EmitOp(o, op));
_   (Emit_جایگاه_رقم_از_پشته_بالاترین_And_برداشت(o));
_   (افزودن_ثبات(o, c_m3Type_i32));

    _catch: أرجع نتیجه;
}


M3_نتیجه Compile_Ref_Func (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_نتیجه نتیجه = m3Err_none;

    u32 func_شاخص;

    // declared before the throws below, which jump past this point to _catch
    m3type_t    refType   = c_m3Type_funcref;
    IM3Function reference = عدم;

_   (ReadLEB_u32(&func_شاخص, &o->wasm, o->wasm_انتها));
    _throwif("function index out of range", func_شاخص >= o->module->رقم_Functions);

    // Inside a constant expression ref.func is itself a declaration; inside a
    // function body the function must already have been declared elsewhere.
    إذا (o->function) {
        _throwif("undeclared function reference", not Module_IsFunctionDeclared(o->module, func_شاخص));
    } وإلا {
_       (Module_DeclareFunction(o->module, func_شاخص));
    }

#  إذا d_m3HasTypedRefs
    refType = Ref_Type_از_Func_Type(o->module->functions[func_شاخص].funcType, true);
#  endif

    // A reference denotes the function that actually runs, so a ref.func naming
    // an import linked to another module is the same reference that module's
    // own ref.func would produce - and carries the defining module with it.
    reference = Function_Implementation(&o->module->functions[func_شاخص]);

_   (افزودن_Const(o, (u64)(uintptr_t)reference, refType));

    _catch: أرجع نتیجه;
}


// table.get/set/size/grow/fill all name a table by index; the table struct goes
// into the codestream so the operation doesn't have to walk the module.
ساكن
M3_نتیجه Read_جدول (IM3Compilation o, M3_جدول** o_جدول)
{
    M3_نتیجه نتیجه = m3Err_none;

    u32 index;
_   (ReadLEB_u32(&index, &o->wasm, o->wasm_انتها));
    _throwif("table index out of range", index >= o->module->رقم_Tables);

    *o_جدول = o->module->tables[index];

    _catch: أرجع نتیجه;
}


// The table operations take every operand from a slot, so spill the integer
// register first: after this each EmitSlotNumOfStackTopAndPop () emits a real
// slot offset, and the register is free for the result. Operands are emitted
// from the top of the stack downwards, which is the order the ops read them.
ساكن
M3_نتیجه Compile_جدول_Op (IM3Compilation o, IM3Operation i_op, M3_جدول* i_جدول, u32 i_رقم_Operands, m3type_t i_retType)
{
    M3_نتیجه نتیجه = m3Err_none;

_   (Preserve_ثبات_If_Occupied(o, c_m3Type_i64));

_   (EmitOp(o, i_op));
    Emit_اشاره_گر(o, i_جدول);

    لكل (u32 i = 0; i < i_رقم_Operands; ++i) {
_       (Emit_جایگاه_رقم_از_پشته_بالاترین_And_برداشت(o));
    }

    إذا (i_retType != c_m3Type_none) {
_       (افزودن_ثبات(o, i_retType));
    }

    _catch: أرجع نتیجه;
}


M3_نتیجه Compile_جدول_Get_مجموعه (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_نتیجه نتیجه = m3Err_none;

    M3_جدول* جدول_2;
_   (Read_جدول(o, &جدول_2));

    إذا (i_opcode == c_wa_Op_جدول_Get) {
_       (Check_عملوند_Type(o, 0, جدول_Addr_Type(جدول_2)));
_       (Compile_جدول_Op(o, op_جدول_Get, جدول_2, 1, جدول_2->type));
    } وإلا {
_       (Check_عملوند_Type(o, 1, جدول_Addr_Type(جدول_2)));
_       (Compile_جدول_Op(o, op_جدول_مجموعه, جدول_2, 2, c_m3Type_none));
    }

    _catch: أرجع نتیجه;
}


M3_نتیجه Compile_جدول_Init (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_نتیجه نتیجه = m3Err_none;

    u32      elem_شاخص;
    M3_جدول* جدول_2;

_   (ReadLEB_u32(&elem_شاخص, &o->wasm, o->wasm_انتها));
    _throwif("element segment index out of range", elem_شاخص >= o->module->رقم_Element_Segments);
_   (Read_جدول(o, &جدول_2));

_   (Check_عملوند_Type(o, 2, جدول_Addr_Type(جدول_2)));

_   (Preserve_ثبات_If_Occupied(o, c_m3Type_i64));
_   (EmitOp(o, op_جدول_Init));
    Emit_اشاره_گر(o, جدول_2);
    Emit_اشاره_گر(o, &o->module->elementSegments[elem_شاخص]);

    لكل (u32 i = 0; i < 3; ++i) {
_       (Emit_جایگاه_رقم_از_پشته_بالاترین_And_برداشت(o));
    }

    _catch: أرجع نتیجه;
}


M3_نتیجه Compile_Elem_Drop (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_نتیجه نتیجه = m3Err_none;

    u32 elem_شاخص;
_   (ReadLEB_u32(&elem_شاخص, &o->wasm, o->wasm_انتها));
    _throwif("element segment index out of range", elem_شاخص >= o->module->رقم_Element_Segments);

_   (EmitOp(o, op_ElemDrop));
    Emit_اشاره_گر(o, &o->module->elementSegments[elem_شاخص]);

    _catch: أرجع نتیجه;
}


M3_نتیجه Compile_جدول_Copy (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_نتیجه نتیجه = m3Err_none;

    M3_جدول* dst;
    M3_جدول* src;

_   (Read_جدول(o, &dst));
_   (Read_جدول(o, &src));
    _throwif(m3Err_typeMismatch, not Is_Sub_Type_از(src->type, dst->type));

_   (Check_عملوند_Type(o, 2, جدول_Addr_Type(dst)));
_   (Check_عملوند_Type(o, 1, جدول_Addr_Type(src)));
_   (Check_عملوند_Type(o, 0, (dst->isTable64 and src->isTable64) ? c_m3Type_i64 : c_m3Type_i32));

_   (Preserve_ثبات_If_Occupied(o, c_m3Type_i64));
_   (EmitOp(o, op_جدول_Copy));
    Emit_اشاره_گر(o, dst);
    Emit_اشاره_گر(o, src);

    لكل (u32 i = 0; i < 3; ++i) {
_       (Emit_جایگاه_رقم_از_پشته_بالاترین_And_برداشت(o));
    }

    _catch: أرجع نتیجه;
}


M3_نتیجه Compile_جدول_اندازه (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_نتیجه نتیجه = m3Err_none;

    M3_جدول* جدول_2;
_   (Read_جدول(o, &جدول_2));

    // a table size is given in the table's own index type
_   (Compile_جدول_Op(o, op_جدول_اندازه, جدول_2, 0, جدول_Addr_Type(جدول_2)));

    _catch: أرجع نتیجه;
}


M3_نتیجه Compile_جدول_Grow_Fill (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_نتیجه نتیجه = m3Err_none;

    M3_جدول* جدول_2;
_   (Read_جدول(o, &جدول_2));

    إذا (i_opcode == c_wa_Op_جدول_Grow) {
_       (Check_عملوند_Type(o, 0, جدول_Addr_Type(جدول_2)));
_       (Compile_جدول_Op(o, op_جدول_Grow, جدول_2, 2, جدول_Addr_Type(جدول_2)));
    } وإلا {
_       (Check_عملوند_Type(o, 0, جدول_Addr_Type(جدول_2)));
_       (Check_عملوند_Type(o, 2, جدول_Addr_Type(جدول_2)));
_       (Compile_جدول_Op(o, op_جدول_Fill, جدول_2, 3, c_m3Type_none));
    }

    _catch: أرجع نتیجه;
}

#پایان_اگر // d_m3HasRefTypes


M3_نتیجه Compile_داده_Drop (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_نتیجه نتیجه = m3Err_none;

    M3_داده_Segment* segment = عدم;
_   (Read_داده_Segment(o, &segment));

_   (EmitOp(o, op_داده_Drop));
    Emit_اشاره_گر(o, segment);

    _catch: أرجع نتیجه;
}


ساكن
M3_نتیجه Read_بلوک_Type (IM3Compilation o, IM3FuncType* o_بلوک_Type)
{
    M3_نتیجه نتیجه = m3Err_none;

#إذا d_m3HasTypedRefs
    // a spelled-out reference type is two bytes, so it cannot be told apart
    // from a type index by the s33 that block types otherwise use
    إذا (o->wasm < o->wasm_انتها and (*o->wasm == d_waEncode_ref or *o->wasm == d_waEncode_refNull)) {
        m3type_t refType;
_       (Parse_مقدار_Type(o->module, &refType, &o->wasm, o->wasm_انتها));
        IM3FuncType ftype = عدم;
_       (AllocFuncType(&ftype, 1));
        ftype->رقم_Rets  = 1;
        ftype->types[0] = refType;
_       (Environment_جمع_کن_Func_Type(o->module->environment, &ftype));
        *o_بلوک_Type = ftype;
        أرجع نتیجه;
    }
#پایان_اگر

    i64 type;
_   (ReadLebSigned(&type, 33, &o->wasm, o->wasm_انتها));

    إذا (type < 0) {
        _throwif(m3Err_invalidTypeId, type < -64);
        u8 مقدار_Type;
_       (NormalizeType(&مقدار_Type, (i8)type));                                 m3log (compile, d_indent " (type: %s)", get_indention_string (o), c_waTypes [مقدار_Type]);
        *o_بلوک_Type = o->module->environment->retFuncTypes[مقدار_Type];
    } وإلا {
        _throwif("func type out of bounds", type >= o->module->رقم_Func_Types);
        *o_بلوک_Type = o->module->funcTypes[type];                         m3log (compile, d_indent " (type: %s)", get_indention_string (o), SPrintFuncTypeSignature (*o_بلوک_Type));
    }
    _catch: أرجع نتیجه;
}

ساكن
M3_نتیجه Preserve_آرگومانها_And_Locals (IM3Compilation o)
{
    M3_نتیجه نتیجه = m3Err_none;

    إذا (o->پشته_شاخص > o->پشته_اولی_Dynamic_شاخص) {
        u32 رقم_آرگومانها_And_Locals = Get_Function_رقم_آرگومانها_And_Locals(o->function);

        لكل (u32 i = 0; i < رقم_آرگومانها_And_Locals; ++i) {
            u16 جایگاه = Get_جایگاه_For_پشته_شاخص(o, i);

            u16 preserved_جایگاه_عدد;
_           (پیدا_کن_Referenced_Local_Within_Current_بلوک(o, &preserved_جایگاه_عدد, جایگاه));

            إذا (preserved_جایگاه_عدد != جایگاه) {
                m3type_t     type = Get_پشته_Type_From_Bottom(o, i);                 d_m3Assert (type != c_m3Type_none)
                IM3Operation op   = Is64_بیت_Type(type) ? op_Copy_جایگاه_64 : op_Copy_جایگاه_32;

                EmitOp(o, op);
                Emit_جایگاه_جابه_جایی(o, preserved_جایگاه_عدد);
                Emit_جایگاه_جابه_جایی(o, جایگاه);
            }
        }
    }

    _catch:
    أرجع نتیجه;
}

ساكن
M3_نتیجه Compile_Loop_Or_بلوک (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_نتیجه نتیجه;

    IM3FuncType بلوک_Type = عدم;

_   (PreserveRegisters(o));
_   (Preserve_آرگومانها_And_Locals(o));

_   (Read_بلوک_Type(o, &بلوک_Type));

    إذا (i_opcode == c_waOp_loop) {
        u16 رقم_Params = Get_Func_Type_رقم_Params(بلوک_Type);
        إذا (رقم_Params) {
            // instantiate constants
            u16 رقم_مقادیر = Get_رقم_بلوک_مقادیر_On_پشته(o);

            إذا (رقم_مقادیر >= رقم_Params) {
                u16 پشته_بالاترین = Get_پشته_بالاترین_شاخص(o) + 1;

                لكل (u16 i = پشته_بالاترین - رقم_Params; i < پشته_بالاترین; ++i) {
                    u16      جایگاه = Get_جایگاه_For_پشته_شاخص(o, i);
                    m3type_t type = Get_پشته_Type_From_Bottom(o, i);

                    إذا (Is_Constant_جایگاه(o, جایگاه)) {
                        u16 new_جایگاه = c_جایگاه_استفادهنشده;
_                       (AllocateSlots(o, &new_جایگاه, type));
_                       (Copy_پشته_شاخص_مقصد_جایگاه(o, new_جایگاه, i));
                        o->wasm_پشته[i] = new_جایگاه;
                    }
                }
            }
        }

_       (EmitOp(o, op_Loop));
    } وإلا {
    }

_   (Compile_بلوک(o, بلوک_Type, i_opcode));

    _catch: أرجع نتیجه;
}

#إذا d_m3HasExceptionHandling

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
ساكن
M3_نتیجه EmitCatchStubs (IM3Compilation o)
{
    IM3_کد_صفحه displaced_صفحه = عدم;
_try {
    M3CatchClause* clauses    = o->tryClauses;
    u32            رقم_Clauses = o->رقم_Try_Clauses;

    // the body must not see these: a nested try_table sets up its own
    o->tryClauses    = عدم;
    o->رقم_Try_Clauses = 0;

    لكل (u32 i = 0; i < رقم_Clauses; ++i) {
        M3CatchClause* clause = &clauses[i];

        IM3CompilationScope scope;
_       (Get_بلوک_Scope(o, &scope, clause->label_عمق + 1));

        IM3FuncType payload = clause->برچسب ? clause->برچسب->type : عدم;
        u16         رقم_آرگومانها = Get_Func_Type_رقم_Params(payload);

        // What the stub hands the label - the payload, then the exnref for a
        // _ref clause - has to be what the label takes. ResolveBlockResults
        // only counts the values, and the validator compares base types, so a
        // typed reference is only checked here: (ref null $t) reaching a label
        // that takes (ref $t) has to be rejected.
        {
            bool isLoop         = (scope->opcode == c_waOp_loop);
            u16  رقم_Label_مقادیر = isLoop ? Get_Func_Type_رقم_Params(scope->type)
                                         : Get_Func_Type_رقم_Results(scope->type);

            _throwif(m3_Err_type_شمار_Mismatch, رقم_Label_مقادیر != رقم_آرگومانها + (clause->hasRef ? 1 : 0));

            لكل (u16 a = 0; a < رقم_Label_مقادیر; ++a) {
                m3type_t given  = (a < رقم_آرگومانها) ? GetFuncTypeParamType(payload, a)
                                                : (m3type_t)c_m3Type_exnref;
                m3type_t wanted = isLoop ? GetFuncTypeParamType(scope->type, a)
                                         : Get_Func_Type_نتیجه_Type(scope->type, a);

                _throwif(m3Err_typeMismatch, not Is_Sub_Type_از(given, wanted));
            }
        }

        IM3_کد_صفحه stub_صفحه;
_       (Acquire_Compilation_کد_صفحه(o, &stub_صفحه));

        pc_t شروع_PC  = Get_صفحه_PC(stub_صفحه);
        displaced_صفحه = o->صفحه;
        o->صفحه       = stub_صفحه;

        u16 اولی_شاخص = o->پشته_شاخص;

        لكل (u16 a = 0; a < رقم_آرگومانها; ++a) {
_           (افزودن_Allocated_جایگاه(o, GetFuncTypeParamType(payload, a)));
        }

        إذا (clause->hasRef) {
_           (افزودن_Allocated_جایگاه(o, c_m3Type_exnref));
        }

        // emitted even with nothing to move: this is also where an exception
        // no clause will reify gets released
_       (Ensure_کد_صفحه_رقم_سطرها(o, 2 * رقم_آرگومانها + 3 + d_m3_کد_صفحه_Free_سطرها_آستانه));

_       (EmitOp(o, op_CatchPayload));
        EmitConstant32(o, رقم_آرگومانها);

        لكل (u16 a = 0; a < رقم_آرگومانها; ++a) {
            u16 index = اولی_شاخص + a;
            Emit_جایگاه_جابه_جایی(o, Get_جایگاه_For_پشته_شاخص(o, index));
            EmitConstant32(o, Is64_بیت_Type(Get_پشته_Type_From_Bottom(o, index)));
        }

        Emit_جایگاه_جابه_جایی(o, clause->hasRef ? (i32)Get_جایگاه_For_پشته_شاخص(o, اولی_شاخص + رقم_آرگومانها) : -1);

_       (Emit_برداشت_Try_قابها(o, رقم_Try_قابها_مقصد_برداشت(o->بلوک.outer, scope)));

        إذا (scope->opcode == c_waOp_loop) {
_           (Resolve_بلوک_Results(o, scope, /* isBranch: */ true));

_           (EmitOp(o, Op_ContinueLoop(o)));
            Emit_اشاره_گر(o, scope->pc);
_           (رکورد_پشت_یال(o, scope, false));
        } وإلا إذا (scope->عمق == 0) {
_           (Return_مقادیر(o, scope, /* isBranch: */ true));
_           (EmitOp(o, op_Return));
        } وإلا {
_           (Resolve_بلوک_Results(o, scope, /* isBranch: */ true));
_           (EmitPatchingBranch(o, scope));
        }

        IM3_کد_صفحه return_مقصد = displaced_صفحه;
        displaced_صفحه        = عدم;

        Release_Compilation_کد_صفحه(o);
        o->صفحه = return_مقصد;

        *(pc_t*)clause->stub_جایگاه = شروع_PC;

        طالما (o->پشته_شاخص > اولی_شاخص) {
_           (برداشت(o));
        }
    }

} _catch:

    // thrown with a stub page installed: release it and put back the one it
    // displaced, which the caller's catch then releases
    إذا (displaced_صفحه) {
        Release_Compilation_کد_صفحه(o);
        o->صفحه = displaced_صفحه;
    }

    أرجع نتیجه;
}


ساكن
M3_نتیجه Compile_Try_جدول (IM3Compilation o, m3opcode_t i_opcode)
{
    M3CatchClause* clauses = عدم;
_try {
    // a catch entry has to be able to assume the operand stack is entirely in
    // slots: an unwind leaves nothing in the registers
_   (PreserveRegisters(o));
_   (Preserve_آرگومانها_And_Locals(o));

    IM3FuncType بلوک_Type = عدم;
_   (Read_بلوک_Type(o, &بلوک_Type));

    u32 رقم_Clauses;
_   (ReadLEB_u32(&رقم_Clauses, &o->wasm, o->wasm_انتها));

    _throwif("too many catch clauses", رقم_Clauses > d_m3_Max_Sane_برچسبها_شمار);

    إذا (رقم_Clauses) {
        clauses = m3_AllocArray(M3CatchClause, رقم_Clauses);
        _throwifnull(clauses);
    }

    لكل (u32 i = 0; i < رقم_Clauses; ++i) {
        u8 نوع;
_       (Read_u8(&نوع, &o->wasm, o->wasm_انتها));
        _throwif(m3Err_wasmMalformed, نوع > 0x03);

        clauses[i].برچسب    = عدم;
        clauses[i].hasRef = (نوع == 0x01 or نوع == 0x03);

        إذا (نوع == 0x00 or نوع == 0x01) {
            u32 برچسب_شاخص;
_           (ReadLEB_u32(&برچسب_شاخص, &o->wasm, o->wasm_انتها));
            _throwif(m3_Err_unknown_برچسب, برچسب_شاخص >= o->module->رقم_برچسبها);

            IM3_برچسب برچسب     = &o->module->برچسبها[برچسب_شاخص];
            clauses[i].برچسب = برچسب->resolved ? برچسب->resolved : برچسب;
        }

_       (ReadLEB_u32(&clauses[i].label_عمق, &o->wasm, o->wasm_انتها));
    }

    // op_TryTable, the clause count, and two words per clause, all of which have
    // to land on one page: the op reads the table by offset from its own pc
_   (Ensure_کد_صفحه_رقم_سطرها(o, 2 * رقم_Clauses + 2 + d_m3_کد_صفحه_Free_سطرها_آستانه));

_   (EmitOp(o, op_Try_جدول));
    EmitConstant32(o, رقم_Clauses);

    لكل (u32 i = 0; i < رقم_Clauses; ++i) {
        Emit_اشاره_گر(o, clauses[i].برچسب);
        clauses[i].stub_جایگاه = Reserve_اشاره_گر(o);
    }

    o->tryClauses    = clauses;
    o->رقم_Try_Clauses = رقم_Clauses;

_   (Compile_بلوک(o, بلوک_Type, c_wa_Op_try_جدول));

} _catch:

    o->tryClauses    = عدم;
    o->رقم_Try_Clauses = 0;

    m3_Free(clauses);

    أرجع نتیجه;
}


ساكن
M3_نتیجه Compile_Throw (IM3Compilation o, m3opcode_t i_opcode)
{
_try {
    u32 برچسب_شاخص;
_   (ReadLEB_u32(&برچسب_شاخص, &o->wasm, o->wasm_انتها));
    _throwif(m3_Err_unknown_برچسب, برچسب_شاخص >= o->module->رقم_برچسبها);

    IM3_برچسب برچسب = &o->module->برچسبها[برچسب_شاخص];
    إذا (برچسب->resolved) {
        برچسب = برچسب->resolved;
    }

    u16 رقم_آرگومانها = Get_Func_Type_رقم_Params(برچسب->type);

    // the payload is read out of slots, so nothing may still be in a register
_   (PreserveRegisters(o));

    // check the payload before emitting anything, so a mistyped throw does not
    // leave half an operation behind
    bool isPolymorphic = Is_پشته_Polymorphic(o);

    إذا (not isPolymorphic) {
        _throwif(m3_Err_type_شمار_Mismatch, Get_رقم_بلوک_مقادیر_On_پشته(o) < رقم_آرگومانها);

        لكل (u16 i = 0; i < رقم_آرگومانها; ++i) {
            u16 index = (u16)(o->پشته_شاخص - رقم_آرگومانها + i);

            _throwif(m3Err_typeMismatch, not Is_Sub_Type_از(Get_پشته_Type_From_Bottom(o, index),
                                                         GetFuncTypeParamType(برچسب->type, i)));
        }
    }

_   (Ensure_کد_صفحه_رقم_سطرها(o, 2 * رقم_آرگومانها + 3 + d_m3_کد_صفحه_Free_سطرها_آستانه));

_   (EmitOp(o, op_Throw));
    Emit_اشاره_گر(o, برچسب);
    EmitConstant32(o, رقم_آرگومانها);

    // the payload is named in the order the tag declares it, so the deepest of
    // the arguments on the stack comes first. In unreachable code there is
    // nothing on the stack to name and nothing will run this, so slot 0 stands
    // in for an operand that isn't there.
    لكل (u16 i = 0; i < رقم_آرگومانها; ++i) {
        m3type_t type = GetFuncTypeParamType(برچسب->type, i);

        Emit_جایگاه_جابه_جایی(o, isPolymorphic ? 0 : Get_جایگاه_For_پشته_شاخص(o, (u16)(o->پشته_شاخص - رقم_آرگومانها + i)));
        EmitConstant32(o, Is64_بیت_Type(type));
    }

    إذا (not isPolymorphic) {
        لكل (u16 i = 0; i < رقم_آرگومانها; ++i) {
_           (برداشت(o));
        }
    }

_   (مجموعه_پشته_Polymorphic(o));

} _catch: أرجع نتیجه;
}


ساكن
M3_نتیجه Compile_ThrowRef (IM3Compilation o, m3opcode_t i_opcode)
{
_try {
    إذا (not Is_پشته_Polymorphic(o)) {
_       (Preserve_ثبات_If_Occupied(o, c_m3Type_i64));

        _throwif(m3Err_typeMismatch, پایه_Type_از(Get_پشته_بالاترین_Type(o)) != c_m3Type_exnref);

_       (EmitOp(o, op_ThrowRef));
        Emit_جایگاه_جابه_جایی(o, Get_پشته_بالاترین_جایگاه_عدد(o));

_       (برداشت(o));
    }

_   (مجموعه_پشته_Polymorphic(o));

} _catch: أرجع نتیجه;
}

#پایان_اگر // d_m3HasExceptionHandling

#إذا d_m3_Has_پشته_Switching

ساكن
M3_نتیجه Compile_ContNew (IM3Compilation o, m3opcode_t i_opcode)
{
_try {
    u32 type_شاخص;
_   (ReadLEB_u32(&type_شاخص, &o->wasm, o->wasm_انتها));
    _throwif(m3_Err_type_شمار_Mismatch, type_شاخص >= o->module->رقم_Func_Types);

    IM3FuncType funcType = o->module->funcTypes[type_شاخص];
    _throwif("type is not a continuation", not funcType->isContinuation);

    // the operands are read out of slots, so nothing may still be in a register
_   (PreserveRegisters(o));

    // cont.new $ct : [(ref null $ft)] -> [(ref $ct)], where $ct ~~ cont $ft:
    // the function has to be one this continuation type can run
    m3type_t بالاترین_Type = Get_پشته_بالاترین_Type(o);
    _throwif(m3Err_typeMismatch,
             not Is_Sub_Type_از(بالاترین_Type, Ref_Type_از_Func_Type(funcType->contFuncType, false)));

    u16 func_جایگاه = Get_پشته_بالاترین_جایگاه_عدد(o);
_   (برداشت(o));

    u16 dst_جایگاه;
_   (افزودن_Allocated_جایگاه(o, Ref_Type_از_Func_Type(funcType, true)));
    dst_جایگاه = Get_پشته_بالاترین_جایگاه_عدد(o);

_   (Ensure_کد_صفحه_رقم_سطرها(o, 4 + d_m3_کد_صفحه_Free_سطرها_آستانه));
_   (EmitOp(o, op_ContNew));
    Emit_اشاره_گر(o, funcType);
    Emit_جایگاه_جابه_جایی(o, dst_جایگاه);
    Emit_جایگاه_جابه_جایی(o, func_جایگاه);

} _catch: أرجع نتیجه;
}

ساكن
M3_نتیجه Compile_ContBind (IM3Compilation o, m3opcode_t i_opcode)
{
_try {
    u32 typeIndex1, typeIndex2;
_   (ReadLEB_u32(&typeIndex1, &o->wasm, o->wasm_انتها));
_   (ReadLEB_u32(&typeIndex2, &o->wasm, o->wasm_انتها));
    _throwif(m3_Err_type_شمار_Mismatch, typeIndex1 >= o->module->رقم_Func_Types);
    _throwif(m3_Err_type_شمار_Mismatch, typeIndex2 >= o->module->رقم_Func_Types);

    IM3FuncType ct1 = o->module->funcTypes[typeIndex1];
    IM3FuncType ct2 = o->module->funcTypes[typeIndex2];
    _throwif("type is not a continuation", not ct1->isContinuation || not ct2->isContinuation);

    // the operands are read out of slots, so nothing may still be in a register
_   (PreserveRegisters(o));

    m3type_t contType = Get_پشته_بالاترین_Type(o);
    _throwif(m3Err_typeMismatch, not Is_Sub_Type_از(contType, Ref_Type_از_Func_Type(ct1, false)));
    u16 cont_جایگاه = Get_پشته_بالاترین_جایگاه_عدد(o);
_   (برداشت(o));

    IM3FuncType ft1 = ct1->contFuncType ? ct1->contFuncType : ct1;
    IM3FuncType ft2 = ct2->contFuncType ? ct2->contFuncType : ct2;

    u16 رقم_Params1 = Get_Func_Type_رقم_Params(ft1);
    u16 رقم_Params2 = Get_Func_Type_رقم_Params(ft2);
    _throwif(m3Err_typeMismatch, رقم_Params1 < رقم_Params2);
    u32 رقم_Bound = رقم_Params1 - رقم_Params2;

    u16 رقم_Results1 = Get_Func_Type_رقم_Results(ft1);
    u16 رقم_Results2 = Get_Func_Type_رقم_Results(ft2);
    _throwif(m3Err_typeMismatch, رقم_Results1 != رقم_Results2);
    لكل (u16 i = 0; i < رقم_Results1; ++i) {
        _throwif(m3Err_typeMismatch, Get_Func_Type_نتیجه_Type(ft1, i) != Get_Func_Type_نتیجه_Type(ft2, i));
    }
    لكل (u16 i = 0; i < رقم_Params2; ++i) {
        _throwif(m3Err_typeMismatch, GetFuncTypeParamType(ft1, رقم_Bound + i) != GetFuncTypeParamType(ft2, i));
    }

    u16 argSlots[d_m3MaxContinuationPayload];
    _throwif("too many bound args", رقم_Bound > d_m3MaxContinuationPayload);

    لكل (u32 i = 0; i < رقم_Bound; ++i) {
        u32      paramIdx     = رقم_Bound - 1 - i;
        m3type_t expectedType = GetFuncTypeParamType(ft1, paramIdx);
        m3type_t actualType   = Get_پشته_بالاترین_Type(o);
        _throwif(m3Err_typeMismatch, not Is_Sub_Type_از(actualType, expectedType));
        argSlots[paramIdx] = Get_پشته_بالاترین_جایگاه_عدد(o);
_       (برداشت(o));
    }

    u16 dst_جایگاه;
_   (افزودن_Allocated_جایگاه(o, Ref_Type_از_Func_Type(ct2, true)));
    dst_جایگاه = Get_پشته_بالاترین_جایگاه_عدد(o);

_   (Ensure_کد_صفحه_رقم_سطرها(o, 6 + رقم_Bound * 2 + d_m3_کد_صفحه_Free_سطرها_آستانه));
_   (EmitOp(o, op_ContBind));
    Emit_اشاره_گر(o, ct1);
    Emit_اشاره_گر(o, ct2);
    Emit_جایگاه_جابه_جایی(o, dst_جایگاه);
    Emit_جایگاه_جابه_جایی(o, cont_جایگاه);
    EmitConstant32(o, رقم_Bound);
    لكل (u32 i = 0; i < رقم_Bound; ++i) {
        Emit_جایگاه_جابه_جایی(o, argSlots[i]);
        EmitConstant32(o, Is64_بیت_Type(GetFuncTypeParamType(ft1, i)));
    }

} _catch: أرجع نتیجه;
}

ساكن
M3_نتیجه Compile_Suspend (IM3Compilation o, m3opcode_t i_opcode)
{
_try {
    u32 برچسب_شاخص;
_   (ReadLEB_u32(&برچسب_شاخص, &o->wasm, o->wasm_انتها));
    _throwif(m3_Err_unknown_برچسب, برچسب_شاخص >= o->module->رقم_برچسبها);

    IM3_برچسب برچسب = &o->module->برچسبها[برچسب_شاخص];
    إذا (برچسب->resolved) {
        برچسب = برچسب->resolved;
    }
    u16 رقم_Params  = Get_Func_Type_رقم_Params(برچسب->type);
    u16 رقم_Results = Get_Func_Type_رقم_Results(برچسب->type);

_   (PreserveRegisters(o));

    u16 paramSlots[d_m3MaxContinuationPayload];
    _throwif("too many tag params", رقم_Params > d_m3MaxContinuationPayload);
    لكل (u16 i = 0; i < رقم_Params; ++i) {
        u16      pIdx         = رقم_Params - 1 - i;
        m3type_t expectedType = GetFuncTypeParamType(برچسب->type, pIdx);
        m3type_t actualType   = Get_پشته_بالاترین_Type(o);
        _throwif(m3Err_typeMismatch, not Is_Sub_Type_از(actualType, expectedType));
        paramSlots[pIdx] = Get_پشته_بالاترین_جایگاه_عدد(o);
_       (برداشت(o));
    }

    u16 نتیجه_Slots[d_m3MaxContinuationPayload];
    _throwif("too many tag results", رقم_Results > d_m3MaxContinuationPayload);
    لكل (u16 i = 0; i < رقم_Results; ++i) {
        m3type_t rtype = Get_Func_Type_نتیجه_Type(برچسب->type, i);
_       (افزودن_Allocated_جایگاه(o, rtype));
        نتیجه_Slots[i] = Get_پشته_بالاترین_جایگاه_عدد(o);
    }

_   (Ensure_کد_صفحه_رقم_سطرها(o, 4 + رقم_Params * 2 + رقم_Results * 2 + d_m3_کد_صفحه_Free_سطرها_آستانه));
_   (EmitOp(o, op_Suspend));
    Emit_اشاره_گر(o, برچسب);
    EmitConstant32(o, رقم_Params);
    لكل (u16 i = 0; i < رقم_Params; ++i) {
        Emit_جایگاه_جابه_جایی(o, paramSlots[i]);
        EmitConstant32(o, Is64_بیت_Type(GetFuncTypeParamType(برچسب->type, i)));
    }
    EmitConstant32(o, رقم_Results);
    لكل (u16 i = 0; i < رقم_Results; ++i) {
        Emit_جایگاه_جابه_جایی(o, نتیجه_Slots[i]);
        EmitConstant32(o, Is64_بیت_Type(Get_Func_Type_نتیجه_Type(برچسب->type, i)));
    }

    // the results are part of the frame here: op_Suspend clears them, and
    // whatever resumes this writes them before anything reads them
_   (رکورد_امن_نقطه(o, safepoint_suspend, 0, رقم_Results, 0, عدم));

} _catch: أرجع نتیجه;
}

// resume, resume_throw and resume_throw_ref. All three run a continuation
// under a handler table and take its results; they differ only in what they
// hand it to start with - arguments, a fresh exception, or one off the stack.
//
// Emitted as: the continuation type, the slot holding it, the operands that
// belong to this form, the handler table, then the result slots. Keeping the
// tail identical is what lets one runtime path finish all three.
ساكن
M3_نتیجه Compile_Resume (IM3Compilation o, m3opcode_t i_opcode)
{
    M3ResumeHandler* handlers      = عدم;
    IM3_کد_صفحه      displaced_صفحه = عدم;
_try {
    bool isThrow    = (i_opcode == c_waOp_resumeThrow);
    bool isThrowRef = (i_opcode == c_waOp_resumeThrowRef);

    u32 type_شاخص;
_   (ReadLEB_u32(&type_شاخص, &o->wasm, o->wasm_انتها));
    _throwif(m3_Err_type_شمار_Mismatch, type_شاخص >= o->module->رقم_Func_Types);

    IM3FuncType contFuncType = o->module->funcTypes[type_شاخص];
    _throwif("type is not a continuation", not contFuncType->isContinuation);

    IM3FuncType innerType      = contFuncType->contFuncType;
    u16         رقم_Cont_Results = Get_Func_Type_رقم_Results(innerType);

    IM3_برچسب exn_برچسب      = عدم;
    u16    رقم_Operands = 0;

    إذا (isThrow) {
        u32 برچسب_شاخص;
_       (ReadLEB_u32(&برچسب_شاخص, &o->wasm, o->wasm_انتها));
        _throwif(m3_Err_unknown_برچسب, برچسب_شاخص >= o->module->رقم_برچسبها);

        exn_برچسب = &o->module->برچسبها[برچسب_شاخص];

        // it is an exception that gets raised, so the tag must be one throw
        // could have raised: no results
        _throwif(m3Err_typeMismatch, Get_Func_Type_رقم_Results(exn_برچسب->type) != 0);

        رقم_Operands = Get_Func_Type_رقم_Params(exn_برچسب->type);
    } وإلا إذا (not isThrowRef) {
        // a plain resume hands over the continuation's own parameters. An
        // aborted one never receives them, so they are unconstrained there.
        رقم_Operands = Get_Func_Type_رقم_Params(innerType);
    }

    u32 رقم_Clauses;
_   (ReadLEB_u32(&رقم_Clauses, &o->wasm, o->wasm_انتها));

    u32 clauseLabelDepths[16];
    _throwif("too many resume clauses", رقم_Clauses > 16);

    إذا (رقم_Clauses) {
        handlers = m3_AllocArray(M3ResumeHandler, رقم_Clauses);
        _throwifnull(handlers);
    }

    لكل (u32 i = 0; i < رقم_Clauses; ++i) {
        u8 نوع;
_       (Read_u8(&نوع, &o->wasm, o->wasm_انتها));
        _throwif(m3Err_wasmMalformed, نوع > 0x01);

        handlers[i].نوع = نوع;

        u32 برچسب_شاخص;
_       (ReadLEB_u32(&برچسب_شاخص, &o->wasm, o->wasm_انتها));
        _throwif(m3_Err_unknown_برچسب, برچسب_شاخص >= o->module->رقم_برچسبها);

        IM3_برچسب برچسب         = &o->module->برچسبها[برچسب_شاخص];
        handlers[i].برچسب    = برچسب->resolved ? برچسب->resolved : برچسب;
        handlers[i].stubPC = عدم;

        إذا (نوع == 0x00) {
_           (ReadLEB_u32(&clauseLabelDepths[i], &o->wasm, o->wasm_انتها));
        } وإلا {
            // (on $e switch): the tag carries no payload, and what the switch
            // site yields is what this resume yields
            _throwif(m3Err_typeMismatch, Get_Func_Type_رقم_Params(handlers[i].برچسب->type) != 0);
            clauseLabelDepths[i] = 0;
        }
    }

    // the operands are read out of slots, so nothing may still be in a register
_   (PreserveRegisters(o));

    m3type_t cont_بالاترین_Type = Get_پشته_بالاترین_Type(o);
    _throwif(m3Err_typeMismatch, not Is_Sub_Type_از(cont_بالاترین_Type, Ref_Type_از_Func_Type(contFuncType, false)));
    u16 cont_جایگاه = Get_پشته_بالاترین_جایگاه_عدد(o);
_   (برداشت(o));

    u16 exn_جایگاه = 0;

    إذا (isThrowRef) {
        m3type_t exnType = Get_پشته_بالاترین_Type(o);
        _throwif(m3Err_typeMismatch, پایه_Type_از(exnType) != c_m3Type_exnref);
        exn_جایگاه = Get_پشته_بالاترین_جایگاه_عدد(o);
_       (برداشت(o));
    }

    u16 عملوند_Slots[d_m3MaxContinuationPayload];
    _throwif("too many resume operands", رقم_Operands > d_m3MaxContinuationPayload);

    IM3FuncType عملوند_Type = isThrow ? exn_برچسب->type : innerType;

    لكل (u16 i = 0; i < رقم_Operands; ++i) {
        u16      pIdx         = (u16)(رقم_Operands - 1 - i);
        m3type_t expectedType = GetFuncTypeParamType(عملوند_Type, pIdx);
        m3type_t actualType   = Get_پشته_بالاترین_Type(o);
        _throwif(m3Err_typeMismatch, not Is_Sub_Type_از(actualType, expectedType));
        عملوند_Slots[pIdx] = Get_پشته_بالاترین_جایگاه_عدد(o);
_       (برداشت(o));
    }

    لكل (u32 i = 0; i < رقم_Clauses; ++i) {
        إذا (handlers[i].نوع != 0x00) {
            استمر;
        }

        IM3CompilationScope scope;
_       (Get_بلوک_Scope(o, &scope, clauseLabelDepths[i]));

        IM3_برچسب برچسب        = handlers[i].برچسب;
        u16    رقم_Payload = Get_Func_Type_رقم_Params(برچسب->type);

        m3type_t labelContType = (scope->opcode == c_waOp_loop)
                                   ? GetFuncTypeParamType(scope->type, رقم_Payload)
                                   : Get_Func_Type_نتیجه_Type(scope->type, رقم_Payload);

        IM3_کد_صفحه stub_صفحه;
_       (Acquire_Compilation_کد_صفحه(o, &stub_صفحه));
        pc_t شروع_PC  = Get_صفحه_PC(stub_صفحه);
        displaced_صفحه = o->صفحه;
        o->صفحه       = stub_صفحه;

        u16 اولی_شاخص = o->پشته_شاخص;
        لكل (u16 a = 0; a < رقم_Payload; ++a) {
_           (افزودن_Allocated_جایگاه(o, GetFuncTypeParamType(برچسب->type, a)));
        }

        // The handler is handed the suspended continuation.
_       (افزودن_Allocated_جایگاه(o, labelContType));

_       (Ensure_کد_صفحه_رقم_سطرها(o, 2 * رقم_Payload + 5 + d_m3_کد_صفحه_Free_سطرها_آستانه));
_       (EmitOp(o, op_ResumePayload));
        Emit_اشاره_گر(o, Module_Cont_Type_از_Ref(o->module, labelContType));
        EmitConstant32(o, رقم_Payload);
        لكل (u16 a = 0; a < رقم_Payload; ++a) {
            u16 index = اولی_شاخص + a;
            Emit_جایگاه_جابه_جایی(o, Get_جایگاه_For_پشته_شاخص(o, index));
            EmitConstant32(o, Is64_بیت_Type(Get_پشته_Type_From_Bottom(o, index)));
        }
        Emit_جایگاه_جابه_جایی(o, Get_جایگاه_For_پشته_شاخص(o, اولی_شاخص + رقم_Payload));

#  إذا d_m3HasExceptionHandling
        // the clause leaves every try region between the resume and its label,
        // and leaving one retires its handlers, the way a branch does
_       (Emit_برداشت_Try_قابها(o, رقم_Try_قابها_مقصد_برداشت(&o->بلوک, scope)));
#  endif

        إذا (scope->opcode == c_waOp_loop) {
_           (Resolve_بلوک_Results(o, scope, true));
_           (EmitOp(o, Op_ContinueLoop(o)));
            Emit_اشاره_گر(o, scope->pc);
_           (رکورد_پشت_یال(o, scope, false));
        } وإلا إذا (scope->عمق == 0) {
_           (Return_مقادیر(o, scope, true));
_           (EmitOp(o, op_Return));
        } وإلا {
_           (Resolve_بلوک_Results(o, scope, true));
_           (EmitPatchingBranch(o, scope));
        }

        IM3_کد_صفحه return_مقصد = displaced_صفحه;
        displaced_صفحه        = عدم;

        Release_Compilation_کد_صفحه(o);
        o->صفحه = return_مقصد;

        handlers[i].stubPC = شروع_PC;

        طالما (o->پشته_شاخص > اولی_شاخص) {
_           (برداشت(o));
        }
    }

    u16 resSlots[d_m3MaxContinuationPayload];
    _throwif("too many cont results", رقم_Cont_Results > d_m3MaxContinuationPayload);
    لكل (u16 i = 0; i < رقم_Cont_Results; ++i) {
        m3type_t rtype = Get_Func_Type_نتیجه_Type(innerType, i);
_       (افزودن_Allocated_جایگاه(o, rtype));
        resSlots[i] = Get_پشته_بالاترین_جایگاه_عدد(o);
    }

_   (Ensure_کد_صفحه_رقم_سطرها(o, 8 + رقم_Operands * 2 + رقم_Clauses * 3 + رقم_Cont_Results * 2 +
                                  d_m3_کد_صفحه_Free_سطرها_آستانه));

_   (EmitOp(o, op_Resume));
    Emit_اشاره_گر(o, contFuncType);
    Emit_جایگاه_جابه_جایی(o, cont_جایگاه);
    EmitConstant32(o, isThrow ? d_m3ResumeThrow : (isThrowRef ? d_m3ResumeThrowRef : d_m3_Resume_نرمال));

    إذا (isThrowRef) {
        Emit_جایگاه_جابه_جایی(o, exn_جایگاه);
    } وإلا {
        إذا (isThrow) {
            Emit_اشاره_گر(o, exn_برچسب);
        }

        EmitConstant32(o, رقم_Operands);
        لكل (u16 i = 0; i < رقم_Operands; ++i) {
            Emit_جایگاه_جابه_جایی(o, عملوند_Slots[i]);
            EmitConstant32(o, Is64_بیت_Type(GetFuncTypeParamType(عملوند_Type, i)));
        }
    }

    EmitConstant32(o, رقم_Clauses);
    لكل (u32 i = 0; i < رقم_Clauses; ++i) {
        EmitConstant32(o, handlers[i].نوع);
        Emit_اشاره_گر(o, handlers[i].برچسب);
        Emit_اشاره_گر(o, handlers[i].stubPC);
    }

    EmitConstant32(o, رقم_Cont_Results);
    لكل (u16 i = 0; i < رقم_Cont_Results; ++i) {
        Emit_جایگاه_جابه_جایی(o, resSlots[i]);
        EmitConstant32(o, Is64_بیت_Type(Get_Func_Type_نتیجه_Type(innerType, i)));
    }

    // a suspend passing through the resumed continuation leaves this frame
    // waiting here, with the continuation's results not yet taken
_   (رکورد_امن_نقطه(o, safepoint_resume, 0, رقم_Cont_Results, (u16)رقم_Clauses, عدم));
#  إذا d_m3HasSnapshots
    إذا (o->snapshotMap and o->صفحه) {
        o->snapshotMap->امن_Points[o->snapshotMap->رقم_امن_Points - 1].resumeType = Ref_Type_از_Func_Type(contFuncType, true);
    }
#  endif

} _catch:
    إذا (displaced_صفحه) {
        Release_Compilation_کد_صفحه(o);
        o->صفحه = displaced_صفحه;
    }
    m3_Free(handlers);
    أرجع نتیجه;
}


ساكن
M3_نتیجه Compile_Switch (IM3Compilation o, m3opcode_t i_opcode)
{
_try {
    u32 type_شاخص, برچسب_شاخص;
_   (ReadLEB_u32(&type_شاخص, &o->wasm, o->wasm_انتها));
_   (ReadLEB_u32(&برچسب_شاخص, &o->wasm, o->wasm_انتها));
    _throwif(m3_Err_type_شمار_Mismatch, type_شاخص >= o->module->رقم_Func_Types);
    _throwif(m3_Err_unknown_برچسب, برچسب_شاخص >= o->module->رقم_برچسبها);

    IM3FuncType هدف_Cont_Type = o->module->funcTypes[type_شاخص];
    IM3_برچسب      برچسب            = &o->module->برچسبها[برچسب_شاخص];
    إذا (برچسب->resolved) {
        برچسب = برچسب->resolved;
    }

    _throwif("type is not a continuation", not هدف_Cont_Type->isContinuation);

    // a switch tag carries no payload of its own: the values travel as the
    // peer's arguments, and the tag's results are what the switch site yields
    _throwif(m3Err_typeMismatch, Get_Func_Type_رقم_Params(برچسب->type) != 0);

    // switch $ct1 : [t1* (ref null $ct1)] -> [t2*]
    //   where $ct1 = cont [t1* (ref null? $ct2)] -> [..]
    //   and   $ct2 = cont [t2*] -> [..]
    //
    // The trailing parameter of $ct1 is not an operand: the peer receives the
    // continuation this instruction suspends. And what this instruction yields
    // is not $ct1's results but $ct2's parameters - the values whoever comes
    // back this way will hand over.
    IM3FuncType هدف_Type = هدف_Cont_Type->contFuncType;
    u16         رقم_Params  = Get_Func_Type_رقم_Params(هدف_Type);

    _throwif("switch target takes no continuation", رقم_Params == 0);

    m3type_t    peerRefType  = GetFuncTypeParamType(هدف_Type, (u16)(رقم_Params - 1));
    IM3FuncType selfContType = Module_Cont_Type_از_Ref(o->module, peerRefType);

    _throwif(m3Err_typeMismatch, not selfContType);

    IM3FuncType selfType   = selfContType->contFuncType;
    u16         رقم_آرگومانها    = (u16)(رقم_Params - 1);
    u16         رقم_Results = Get_Func_Type_رقم_Params(selfType);

    // the operands are read out of slots, so nothing may still be in a register
_   (PreserveRegisters(o));

    m3type_t cont_بالاترین_Type = Get_پشته_بالاترین_Type(o);
    _throwif(m3Err_typeMismatch, not Is_Sub_Type_از(cont_بالاترین_Type, Ref_Type_از_Func_Type(هدف_Cont_Type, false)));
    u16 هدف_Cont_جایگاه = Get_پشته_بالاترین_جایگاه_عدد(o);
_   (برداشت(o));

    u16 argSlots[d_m3MaxContinuationPayload];
    _throwif("too many switch params", رقم_آرگومانها > d_m3MaxContinuationPayload);

    لكل (u16 i = 0; i < رقم_آرگومانها; ++i) {
        u16      pIdx         = (u16)(رقم_آرگومانها - 1 - i);
        m3type_t expectedType = GetFuncTypeParamType(هدف_Type, pIdx);
        m3type_t actualType   = Get_پشته_بالاترین_Type(o);
        _throwif(m3Err_typeMismatch, not Is_Sub_Type_از(actualType, expectedType));
        argSlots[pIdx] = Get_پشته_بالاترین_جایگاه_عدد(o);
_       (برداشت(o));
    }

    u16 نتیجه_Slots[d_m3MaxContinuationPayload];
    _throwif("too many switch results", رقم_Results > d_m3MaxContinuationPayload);

    لكل (u16 i = 0; i < رقم_Results; ++i) {
_       (افزودن_Allocated_جایگاه(o, GetFuncTypeParamType(selfType, i)));
        نتیجه_Slots[i] = Get_پشته_بالاترین_جایگاه_عدد(o);
    }

_   (Ensure_کد_صفحه_رقم_سطرها(o, 6 + رقم_آرگومانها * 2 + رقم_Results * 2 + d_m3_کد_صفحه_Free_سطرها_آستانه));
_   (EmitOp(o, op_Switch));
    Emit_اشاره_گر(o, هدف_Cont_Type);
    Emit_اشاره_گر(o, برچسب);
    Emit_جایگاه_جابه_جایی(o, هدف_Cont_جایگاه);
    EmitConstant32(o, رقم_آرگومانها);
    لكل (u16 i = 0; i < رقم_آرگومانها; ++i) {
        Emit_جایگاه_جابه_جایی(o, argSlots[i]);
        EmitConstant32(o, Is64_بیت_Type(GetFuncTypeParamType(هدف_Type, i)));
    }
    EmitConstant32(o, رقم_Results);
    لكل (u16 i = 0; i < رقم_Results; ++i) {
        Emit_جایگاه_جابه_جایی(o, نتیجه_Slots[i]);
        EmitConstant32(o, Is64_بیت_Type(GetFuncTypeParamType(selfType, i)));
    }

    // as with suspend: op_Switch clears the results before it leaves
_   (رکورد_امن_نقطه(o, safepoint_suspend, 0, رقم_Results, 0, عدم));

} _catch: أرجع نتیجه;
}

#پایان_اگر // d_m3HasStackSwitching


ساكن
M3_نتیجه Compile_Else_بلوک (IM3Compilation o, pc_t* o_شروع_PC, IM3FuncType i_بلوک_Type)
{
    IM3_کد_صفحه saved_صفحه = o->صفحه;
_try {

    IM3_کد_صفحه else_صفحه;
_   (Acquire_Compilation_کد_صفحه(o, &else_صفحه));

    *o_شروع_PC = Get_صفحه_PC(else_صفحه);

    o->صفحه = else_صفحه;

_   (Compile_بلوک(o, i_بلوک_Type, c_waOp_else));

_   (EmitOp(o, op_Branch));
    Emit_اشاره_گر(o, Get_صفحه_PC(saved_صفحه));
} _catch:

    إذا (o->صفحه != saved_صفحه) {
        Release_Compilation_کد_صفحه(o);
    }

    o->صفحه = saved_صفحه;

    أرجع نتیجه;
}

ساكن
M3_نتیجه Compile_If (IM3Compilation o, m3opcode_t i_opcode)
{
    /*      [   op_If   ]
            [ <else-pc> ]   ---->   [ ..else..  ]
            [  ..if..   ]           [ ..block.. ]
            [ ..block.. ]           [ op_Branch ]
            [    end    ]  <-----   [  <end-pc> ]       */

_try {

    // Spec: if condition must be i32
    إذا (not Is_پشته_Polymorphic(o)) {
        m3type_t condType = Get_پشته_بالاترین_Type(o);
        _throwif(m3Err_typeMismatch, condType != c_m3Type_none and condType != c_m3Type_i32);
    }

_   (Preserve_Non_بالاترین_Registers(o));
_   (Preserve_آرگومانها_And_Locals(o));

#إذا d_m3FuseBranch
    IM3Operation fuseOp = GetBranchFusionOp(o, true);

    إذا (fuseOp) {
        // absorb the if into the compare that produced the condition
        *(IM3Operation*)o->foldPatchPC = fuseOp;
        InvalidateFold(o);

_       (برداشت(o));                          // the condition is consumed inside the fused op
    } وإلا
#پایان_اگر
    {
        IM3Operation op = Is_پشته_بالاترین_درون_ثبات(o) ? op_If_r : op_If_s;

_       (EmitOp(o, op));
_       (Emit_جایگاه_رقم_از_پشته_بالاترین_And_برداشت(o));
    }

    pc_t* pc = (pc_t*)Reserve_اشاره_گر(o);

    IM3FuncType بلوک_Type = عدم;
_   (Read_بلوک_Type(o, &بلوک_Type));     //  dump_type_stack (o);

    u16 پشته_شاخص = o->پشته_شاخص;

_   (Compile_بلوک(o, بلوک_Type, i_opcode));

    إذا (o->قبلی_Opcode == c_waOp_else) {
        o->پشته_شاخص = پشته_شاخص;
_       (Compile_Else_بلوک(o, pc, بلوک_Type));
    } وإلا {
        // if block produces values and there isn't a defined else
        // case, then we need to make one up so that the pass-through
        // results end up in the right place
        إذا (Get_Func_Type_رقم_Results(بلوک_Type)) {
            // rewind to the if's end to create a fake else block
            o->wasm--;
            o->پشته_شاخص = پشته_شاخص;     // dump_type_stack (o);

_           (Compile_Else_بلوک(o, pc, بلوک_Type));
        } وإلا {
            *pc = GetPC(o);
        }
    }

    } _catch: أرجع نتیجه;
}

ساكن
M3_نتیجه Compile_Select (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_نتیجه نتیجه = m3Err_none;

    u16 slots[3] = { c_جایگاه_استفادهنشده, c_جایگاه_استفادهنشده, c_جایگاه_استفادهنشده };

    IM3Operation op = عدم;

    m3type_t type = Get_پشته_Type_From_بالاترین(o, 1); // get type of selection

    إذا (not Is_پشته_Polymorphic(o)) {
        // Spec: the condition operand (top) must be i32
        m3type_t condType = Get_پشته_Type_From_بالاترین(o, 0);
        _throwif(m3Err_typeMismatch, condType != c_m3Type_none and condType != c_m3Type_i32);

        // Spec: the two value operands (below condition) must have matching types
        m3type_t type2 = Get_پشته_Type_From_بالاترین(o, 2);
        _throwif(m3Err_typeMismatch, type != c_m3Type_none and type2 != c_m3Type_none and
                                       not (Is_Sub_Type_از(type, type2) or Is_Sub_Type_از(type2, type)));
    }

    إذا (IsFpType(type)) {
#إذا d_m3HasFloat
        // not consuming a fp reg, so preserve
        إذا (not Is_پشته_بالاترین_Minus1_درون_ثبات(o) and
            not Is_پشته_بالاترین_Minus2_درون_ثبات(o)) {
_           (Preserve_ثبات_If_Occupied(o, type));
        }

        bool selector_درون_Reg = Is_پشته_بالاترین_درون_ثبات(o);
        slots[0]           = Get_پشته_بالاترین_جایگاه_عدد(o);
_       (برداشت(o));

        u32 op_شاخص = 0;

        لكل (u32 i = 1; i <= 2; ++i) {
            إذا (Is_پشته_بالاترین_درون_ثبات(o)) {
                op_شاخص = i;
            } وإلا {
                slots[i] = Get_پشته_بالاترین_جایگاه_عدد(o);
            }

_           (برداشت(o));
        }

        op = c_fpSelectOps[type - c_m3Type_f32][selector_درون_Reg][op_شاخص];
#وإلا
        _throw(m3Err_unknownOpcode);
#پایان_اگر
    } وإلا إذا (IsIntType(type) or IsRefType(type)) {
        // 'sss' operation doesn't consume a register, so might have to protected its contents
        إذا (not Is_پشته_بالاترین_درون_ثبات(o) and
            not Is_پشته_بالاترین_Minus1_درون_ثبات(o) and
            not Is_پشته_بالاترین_Minus2_درون_ثبات(o)) {
_           (Preserve_ثبات_If_Occupied(o, type));
        }

        u32 op_شاخص = 3;  // op_Select_*_sss

        لكل (u32 i = 0; i < 3; ++i) {
            إذا (Is_پشته_بالاترین_درون_ثبات(o)) {
                op_شاخص = i;
            } وإلا {
                slots[i] = Get_پشته_بالاترین_جایگاه_عدد(o);
            }

_           (برداشت(o));
        }

        // a reference is a pointer-sized integer as far as select is concerned
        u32 type_شاخص = IsRefType(type) ? ((M3_SIZEOF_PTR == 8) ? 1 : 0)
                                        : (u32)(type - c_m3Type_i32);

        op = c_intSelectOps[type_شاخص][op_شاخص];
    } وإلا إذا (not Is_پشته_Polymorphic(o)) {
        _throw(m3_Err_function_پشته_Underrun);
    }

    EmitOp(o, op);
    لكل (u32 i = 0; i < 3; i++) {
        إذا (Is_Valid_جایگاه(slots[i])) {
            Emit_جایگاه_جابه_جایی(o, slots[i]);
        }
    }
_   (افزودن_ثبات(o, type));

    _catch: أرجع نتیجه;
}

ساكن
M3_نتیجه Compile_Drop (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_نتیجه نتیجه = برداشت(o);
    إذا (d_m3_Log_Wasm_پشته) {
        dump_type_پشته(o);
    }
    أرجع نتیجه;
}

ساكن
M3_نتیجه Compile_Nop (IM3Compilation o, m3opcode_t i_opcode)
{
    أرجع m3Err_none;
}

ساكن
M3_نتیجه Compile_Unreachable (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_نتیجه نتیجه;

_   (جمع_کن_Trap_رکورد(o));

_   (EmitOp(o, op_Unreachable));
_   (مجموعه_پشته_Polymorphic(o));

    _catch:
    أرجع نتیجه;
}


// OPTZ: currently all stack slot indices take up a full word, but
// dual stack source operands could be packed together
ساكن
M3_نتیجه Compile_Operator (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_نتیجه نتیجه;

    // Declared before the first throw below, which jumps past this point to
    // _catch and so must not skip an initialization.
    IM3Operation op = عدم;

    IM3OpInfo opInfo = GetOpInfo(i_opcode);
    _throwif(m3Err_unknownOpcode, not opInfo);

    // Spec: validate operand types for load/store operations
    إذا (not Is_پشته_Polymorphic(o)) {
        // For load ops (stackOffset == 0, unary), the operand is always i32 (address)
        إذا (i_opcode >= 0x28 and i_opcode <= 0x35) {
            m3type_t بالاترین_Type = Get_پشته_بالاترین_Type(o);
            _throwif(m3Err_typeMismatch, بالاترین_Type != c_m3Type_none and بالاترین_Type != c_m3Type_i32);
        }

        // For store ops (stackOffset == -2), the address operand must be i32
        إذا (i_opcode >= 0x36 and i_opcode <= 0x3e) {
            m3type_t addrType = Get_پشته_Type_From_بالاترین(o, 1);
            _throwif(m3Err_typeMismatch, addrType != c_m3Type_none and addrType != c_m3Type_i32);
        }
    }

    // This preserve is for for FP compare operations.
    // either need additional slot destination operations or the
    // easy fix, move _r0 out of the way.
    // moving out the way might be the optimal solution most often?
    // otherwise, the _r0 reg can get buried down in the stack
    // and be idle & wasted for a moment.
    إذا (M3_UNLIKELY(IsFpType(Get_پشته_بالاترین_Type(o)) and IsIntType(opInfo->type))) {
_       (Preserve_ثبات_If_Occupied(o, opInfo->type));
    }

    إذا (opInfo->پشته_جابه_جایی == 0) {
        إذا (Is_پشته_بالاترین_درون_ثبات(o)) {
            op = opInfo->operations[0]; // _s
        } وإلا {
_           (Preserve_ثبات_If_Occupied(o, opInfo->type));
            op = opInfo->operations[1]; // _r
        }
    } وإلا {
        إذا (Is_پشته_بالاترین_درون_ثبات(o)) {
            op = opInfo->operations[0];  // _rs

            إذا (Is_پشته_بالاترین_Minus1_درون_ثبات(o)) {    d_m3Assert (i_opcode == c_waOp_store_f32 or i_opcode == c_waOp_store_f64);
                op = opInfo->operations[3]; // _rr for fp.store
            }
        } وإلا إذا (Is_پشته_بالاترین_Minus1_درون_ثبات(o)) {
            op = opInfo->operations[1]; // _sr

            إذا (not op) {  // must be commutative, then
                op = opInfo->operations[0];
            }
        } وإلا {
_           (Preserve_ثبات_If_Occupied(o, opInfo->type));     // _ss
            op = opInfo->operations[2];
        }
    }

    إذا (M3_LIKELY(op)) {
_       (EmitOp(o, op));

#إذا d_m3_Fold_مجموعه_Local
        // the op word just written (EmitOp may have bridged pages); NULL in the page-less compile-walk mode
        pc_t patchPC = o->صفحه ? GetPC(o) - 1 : عدم;
#پایان_اگر

_       (Emit_جایگاه_رقم_از_پشته_بالاترین_And_برداشت(o));

        إذا (opInfo->پشته_جابه_جایی < 0) {
_           (Emit_جایگاه_رقم_از_پشته_بالاترین_And_برداشت(o));
        }

        إذا (opInfo->type != c_m3Type_none) {
_           (افزودن_ثبات(o, opInfo->type));

#إذا d_m3_Fold_مجموعه_Local
            // record this op as a fold candidate for an immediately following local.set;
            // the operand-form index is recovered by matching against the variant table
            إذا (patchPC) {
                لكل (u8 form = 0; form < 4; ++form) {
                    إذا (opInfo->operations[form] == op) {
                        o->foldPatchPC    = patchPC;
                        o->foldOpcode     = i_opcode;
                        o->foldForm       = form;
                        o->fold_پشته_شاخص = (u16)Get_پشته_بالاترین_شاخص(o);
                        اكسر;
                    }
                }
            }
#پایان_اگر
        }
    } وإلا {
#اگر_تعریف_شده DEBUG
        نتیجه = ErrorCompile("no operation found for opcode", o, "'%s'", opInfo->name);
#وإلا
        نتیجه = ErrorCompile("no operation found for opcode", o, "%x", i_opcode);
#پایان_اگر
        _throw(نتیجه);
    }

    _catch: أرجع نتیجه;
}

ساكن
M3_نتیجه Compile_Convert (IM3Compilation o, m3opcode_t i_opcode)
{
_try {
    IM3OpInfo opInfo = GetOpInfo(i_opcode);
    _throwif(m3Err_unknownOpcode, not opInfo);

    // Spec: validate source operand type for conversion instructions
    إذا (not Is_پشته_Polymorphic(o)) {
        u8 منبع_Type = c_m3Type_none;
        بدل (i_opcode)
        // clang-format off
        {
        حالة 0xa7:              // i32.wrap/i64
            منبع_Type = c_m3Type_i64; اكسر;
        حالة 0xa8: حالة 0xa9:   // i32.trunc_s/f32, i32.trunc_u/f32
        حالة 0xae: حالة 0xaf:   // i64.trunc_s/f32, i64.trunc_u/f32
        حالة 0xbb:              // f64.promote/f32
        حالة 0xbc:              // i32.reinterpret/f32
            منبع_Type = c_m3Type_f32; اكسر;
        حالة 0xaa: حالة 0xab:   // i32.trunc_s/f64, i32.trunc_u/f64
        حالة 0xb0: حالة 0xb1:   // i64.trunc_s/f64, i64.trunc_u/f64
        حالة 0xb6:              // f32.demote/f64
        حالة 0xbd:              // i64.reinterpret/f64
            منبع_Type = c_m3Type_f64; اكسر;
        حالة 0xac: حالة 0xad:   // i64.extend_s/i32, i64.extend_u/i32
        حالة 0xb2: حالة 0xb3:   // f32.convert_s/i32, f32.convert_u/i32
        حالة 0xb7: حالة 0xb8:   // f64.convert_s/i32, f64.convert_u/i32
        حالة 0xbe:              // f32.reinterpret/i32
            منبع_Type = c_m3Type_i32; اكسر;
        حالة 0xb4: حالة 0xb5:   // f32.convert_s/i64, f32.convert_u/i64
        حالة 0xb9: حالة 0xba:   // f64.convert_s/i64, f64.convert_u/i64
        حالة 0xbf:              // f64.reinterpret/i64
            منبع_Type = c_m3Type_i64; اكسر;
        افتراضي: اكسر;
        }
        // clang-format on

        إذا (منبع_Type != c_m3Type_none) {
            m3type_t بالاترین_Type = Get_پشته_بالاترین_Type(o);
            _throwif(m3Err_typeMismatch, بالاترین_Type != c_m3Type_none and not Is_Sub_Type_از(بالاترین_Type, منبع_Type));
        }
    }

    bool dest_درون_جایگاه   = Is_ثبات_Type_Allocated(o, opInfo->type);
    bool منبع_درون_جایگاه = Is_پشته_بالاترین_درون_جایگاه(o);

    IM3Operation op = opInfo->operations[dest_درون_جایگاه * 2 + منبع_درون_جایگاه];

_   (EmitOp(o, op));
_   (Emit_جایگاه_رقم_از_پشته_بالاترین_And_برداشت(o));

    إذا (dest_درون_جایگاه) {
_       (افزودن_Allocated_جایگاه_And_Emit_2(o, opInfo->type))
    } وإلا {
_       (افزودن_ثبات(o, opInfo->type))
    }
}
    _catch: أرجع نتیجه;
}

#إذا d_m3HasWideArithmetic

// The wide arithmetic proposal: i64.add128 and i64.sub128 over two 128-bit values
// given as i64 halves, and i64.mul_wide_s/u giving the full product of two i64s.
// Each gives back a 128-bit result the same way, low half first. The operands are
// all read from slots, and of the two results the high half, which ends up on top,
// goes to _r0 and the low half to a newly allocated slot - so every operation has
// the one form, whatever shape the stack was in.
ساكن
M3_نتیجه Compile_WideArithmetic (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_نتیجه نتیجه = m3Err_none;

    IM3Operation op          = op_i64_Add128;
    u32          رقم_Operands = 4;

    بدل (i_opcode)
    // clang-format off
    {
    حالة c_waOp_i64Sub128:   op = op_i64_Sub128;                          اكسر;
    حالة c_waOp_i64MulWideS: op = op_i64_MultiplyWide; رقم_Operands = 2;   اكسر;
    حالة c_waOp_i64MulWideU: op = op_u64_MultiplyWide; رقم_Operands = 2;   اكسر;
    افتراضي: اكسر;
    }
    // clang-format on

_   (Preserve_ثبات_If_Occupied(o, c_m3Type_i64));

_   (EmitOp(o, op));

    // the top of the stack first: the high half of the second operand, for the
    // 128-bit ones
    لكل (u32 i = 0; i < رقم_Operands; ++i) {
_       (Emit_جایگاه_رقم_از_پشته_بالاترین_And_برداشت(o));
    }

_   (افزودن_Allocated_جایگاه_And_Emit_2(o, c_m3Type_i64));      // the low half
_   (افزودن_ثبات(o, c_m3Type_i64));                  // the high half

    _catch: أرجع نتیجه;
}

#پایان_اگر // d_m3HasWideArithmetic

#إذا d_m3HasMemory64

// Replaces the 64-bit address operand of a load or store with the checked
// 32-bit effective address op_CheckAddr64 leaves behind, so that the access
// itself compiles exactly as it would against a 32-bit memory: one slot to read
// the address from, and an offset of zero folded in already.
//
// Where the address sits under other operands - the value of a store, the
// operands of an atomic - it is rewritten where it stands rather than popped:
// its stack entry is repointed at the scratch slot and retyped, and the slot it
// used to occupy released. i_numOperands counts the address and all above it.
ساكن
M3_نتیجه EmitCheckAddr64 (IM3Compilation o, u64 i_جابه_جایی, u16 i_رقم_Operands)
{
    M3_نتیجه نتیجه = m3Err_none;

    // The address has to be in a slot for op_CheckAddr64 to read it. Spilling
    // r0 wholesale also keeps the value of a store out of it, which is what
    // leaves the address as the one operand still in a register to worry about.
_   (Preserve_ثبات_If_Occupied(o, c_m3Type_i64));

    // unreachable code: the operands the instruction names may not be there
    إذا (o->پشته_شاخص < o->بلوک.بلوک_پشته_شاخص + i_رقم_Operands) {
        أرجع نتیجه;
    }

    {
        u16      پشته_شاخص = (u16)(o->پشته_شاخص - i_رقم_Operands);
        u16      src_جایگاه    = o->wasm_پشته[پشته_شاخص];
        m3type_t srcType    = o->type_پشته[پشته_شاخص];

        u16 dst_جایگاه = c_جایگاه_استفادهنشده;

        // Checked here rather than left to the validator, which a build may
        // have switched off: reading a slot as an i64 that only holds an i32
        // would reach past what was allocated for it.
        _throwif(m3Err_typeMismatch, پایه_Type_از(srcType) != c_m3Type_i64);

        // an address is an integer, so nothing should be left in a register
        // once r0 has been preserved
        _throwif(m3Err_typeMismatch, Is_ثبات_جایگاه_Alias(src_جایگاه));
        _throwif(m3_Err_function_پشته_Underrun, src_جایگاه >= o->جایگاه_Max_Allocated_شاخص_Plus_One);

        // allocated before the source is released, so the two cannot be the
        // same slot - op_CheckAddr64 reads one and writes the other
_       (AllocateSlots(o, &dst_جایگاه, c_m3Type_i32));

_       (EmitOp(o, op_CheckAddr64));
        Emit_جایگاه_جابه_جایی(o, src_جایگاه);
        Emit_جایگاه_جابه_جایی(o, dst_جایگاه);
        EmitConstant64(o, i_جابه_جایی);

        o->wasm_پشته[پشته_شاخص] = dst_جایگاه;
        o->type_پشته[پشته_شاخص] = c_m3Type_i32;

        // locals and the constant table outlive the instruction; a dynamic
        // slot the address was computed into does not
        إذا (src_جایگاه >= o->جایگاه_اولی_Dynamic_شاخص) {
            Deallocate_جایگاه(o, src_جایگاه, srcType);
        }
    }

    _catch: أرجع نتیجه;
}

#پایان_اگر // d_m3HasMemory64


ساكن
M3_نتیجه Compile_بارگیری_Store (IM3Compilation o, m3opcode_t i_opcode)
{
_try {
    u32 alignHint, حافظه_Idx;
    u64 حافظه_جابه_جایی;

    // alignHint is checked by the validator
_   (Read_حافظه_Arg(&alignHint, &حافظه_Idx, &حافظه_جابه_جایی, &o->wasm, o->wasm_انتها));
                                                                        m3log (compile, d_indent " (memory = %d; offset = %llu)", get_indention_string (o), حافظه_Idx, (بیعلامت دراز دراز) حافظه_جابه_جایی);
    _throwif(m3_Err_unknown_حافظه, حافظه_Idx >= o->module->رقم_Memories);

    IM3OpInfo opInfo = GetOpInfo(i_opcode);
    _throwif(m3Err_unknownOpcode, not opInfo);

    bool isMemory64 = o->module->memories[حافظه_Idx]->isMemory64;

    // Spec: the static offset has to be in range of the address type. Checked
    // here as well as in the validator, so that a build without one still
    // refuses the module rather than silently truncating the offset.
    _throwif(m3Err_wasmMalformed, not isMemory64 and حافظه_جابه_جایی > 0xFFFFFFFFull);

    إذا (IsFpType(opInfo->type)) {
_       (Preserve_ثبات_If_Occupied(o, c_m3Type_f64));
    }

    إذا (حافظه_Idx) {
_       (Emit_مجموعه_حافظه(o, حافظه_Idx));
    }

#إذا d_m3HasMemory64
    إذا (isMemory64) {
        // An offset this far out puts every address out of bounds, so rather
        // than carrying it, clamp it to the limit: op_CheckAddr64 then traps
        // on whatever address it is given, which is the outcome either way.
        إذا (حافظه_جابه_جایی > d_m3_آدرس_حد) {
            حافظه_جابه_جایی = d_m3_آدرس_حد;
        }

_       (EmitCheckAddr64(o, حافظه_جابه_جایی, opInfo->پشته_جابه_جایی < 0 ? 2 : 1));

        // op_CheckAddr64 has folded the offset in already
        حافظه_جابه_جایی = 0;
    }
#پایان_اگر

_   (Compile_Operator(o, i_opcode));

    EmitConstant32(o, (u32)حافظه_جابه_جایی);

    إذا (حافظه_Idx) {
_       (Emit_مجموعه_حافظه(o, 0));
    }
}
    _catch: أرجع نتیجه;
}


#إذا d_m3HasAtomics

// What an atomic instruction takes and gives back, from its sub-opcode. The
// accesses come in runs of seven, one per combination of value type and cell width:
// i32/32, i64/64, i32/8, i32/16, i64/8, i64/16, i64/32. Those runs are told apart by
// where they start.
//
// The operand count includes the address. The top operand is the one that ends up
// in a register, so its type is the one to check on the way out; a store has no
// result, which is c_m3Type_none.
ساكن
فراغ GetAtomicShape (u32 i_sub, u16* o_رقم_Operands, m3type_t* o_بالاترین_Type, m3type_t* o_نتیجه_Type)
{
    // clang-format off
    ساكن ثابت m3type_t c_مقدار_Types [7] = {
        c_m3Type_i32, c_m3Type_i64,
        c_m3Type_i32, c_m3Type_i32,
        c_m3Type_i64, c_m3Type_i64, c_m3Type_i64
    };
    // clang-format on

    إذا (i_sub == 0x00) {                    // memory.atomic.notify: address, count
        *o_رقم_Operands = 2;
        *o_بالاترین_Type     = c_m3Type_i32;
        *o_نتیجه_Type  = c_m3Type_i32;
    } وإلا إذا (i_sub <= 0x02) {             // memory.atomic.wait32/64: address, expected, timeout
        *o_رقم_Operands = 3;
        *o_بالاترین_Type     = c_m3Type_i64;
        *o_نتیجه_Type  = c_m3Type_i32;
    } وإلا إذا (i_sub <= 0x16) {             // loads: address
        *o_رقم_Operands = 1;
        *o_بالاترین_Type     = c_m3Type_i32;
        *o_نتیجه_Type  = c_مقدار_Types[i_sub - 0x10];
    } وإلا إذا (i_sub <= 0x1d) {             // stores: address, value
        *o_رقم_Operands = 2;
        *o_بالاترین_Type     = c_مقدار_Types[i_sub - 0x17];
        *o_نتیجه_Type  = c_m3Type_none;
    } وإلا إذا (i_sub <= 0x47) {             // read-modify-write: address, value
        *o_رقم_Operands = 2;
        *o_بالاترین_Type     = c_مقدار_Types[(i_sub - 0x1e) % 7];
        *o_نتیجه_Type  = *o_بالاترین_Type;
    } وإلا {                                // cmpxchg: address, expected, replacement
        *o_رقم_Operands = 3;
        *o_بالاترین_Type     = c_مقدار_Types[i_sub - 0x48];
        *o_نتیجه_Type  = *o_بالاترین_Type;
    }
}


// The description an access takes as its last immediate: see m3_exec_atomic.h
ساكن
u32 GetAtomicDesc (u32 i_sub)
{
    // per variant, the log2 of the width of the cell
    ساكن ثابت u8 c_log2Widths[7] = { 2, 3, 0, 1, 0, 1, 2 };

    u32 variant, نوع = 0;

    إذا (i_sub <= 0x16) {
        variant = i_sub - 0x10;
    } وإلا إذا (i_sub <= 0x1d) {
        variant = i_sub - 0x17;
    } وإلا إذا (i_sub <= 0x47) {
        variant = (i_sub - 0x1e) % 7;
        نوع    = (i_sub - 0x1e) / 7;         // add, sub, and, or, xor, xchg
    } وإلا {
        variant = i_sub - 0x48;
    }

    // variants 1 and 4..6 are the i64 ones
    u32 is64 = (variant == 1 or variant >= 4) ? 1u : 0u;

    أرجع c_log2Widths[variant] | (is64 << 2) | (نوع << 3);
}


// Every atomic access is one op, whose operands are the topmost in _r0 and the rest
// in slots, topmost first. The address is always the deepest, so it is the last slot
// (or, for a load, the register), followed by the memarg offset. The ops check the
// address themselves: for an atomic that includes its alignment, which only the
// effective address can be asked.
ساكن
M3_نتیجه Compile_Atomic (IM3Compilation o, m3opcode_t i_opcode)
{
_try {
    u32 alignHint, حافظه_Idx;
    u64 حافظه_جابه_جایی;

    u16      رقم_Operands = 0;
    m3type_t بالاترین_Type     = c_m3Type_none;
    m3type_t نتیجه_Type  = c_m3Type_none;

    إذا (i_opcode == c_waOp_atomicFence) {
        // the reserved byte, which the validator has checked to be zero. There
        // is nothing to order between accesses that one thread makes in sequence.
        u8 reserved;
_       (Read_u8(&reserved, &o->wasm, o->wasm_انتها));
    } وإلا {
        // alignHint is checked by the validator
_       (Read_حافظه_Arg(&alignHint, &حافظه_Idx, &حافظه_جابه_جایی, &o->wasm, o->wasm_انتها));
                                                                        m3log (compile, d_indent " (memory = %d; offset = %llu)", get_indention_string (o), حافظه_Idx, (بیعلامت دراز دراز) حافظه_جابه_جایی);
        _throwif(m3_Err_unknown_حافظه, حافظه_Idx >= o->module->رقم_Memories);

        IM3OpInfo opInfo = GetOpInfo(i_opcode);
        _throwif(m3Err_unknownOpcode, not opInfo);

        bool isMemory64 = o->module->memories[حافظه_Idx]->isMemory64;

        // see Compile_Load_Store
        _throwif(m3Err_wasmMalformed, not isMemory64 and حافظه_جابه_جایی > 0xFFFFFFFFull);

        GetAtomicShape(i_opcode & 0xFF, &رقم_Operands, &بالاترین_Type, &نتیجه_Type);

        إذا (حافظه_Idx) {
_           (Emit_مجموعه_حافظه(o, حافظه_Idx));
        }

#  إذا d_m3HasMemory64
        إذا (isMemory64) {
            // see Compile_Load_Store
            إذا (حافظه_جابه_جایی > d_m3_آدرس_حد) {
                حافظه_جابه_جایی = d_m3_آدرس_حد;
            }

_           (EmitCheckAddr64(o, حافظه_جابه_جایی, رقم_Operands));
            حافظه_جابه_جایی = 0;
        }
#  endif

_       (Copy_پشته_بالاترین_مقصد_ثبات(o, false));
_       (EmitOp(o, opInfo->operations[0]));
_       (برداشت_Type(o, بالاترین_Type));

        لكل (u16 i = 1; i < رقم_Operands; ++i) {
_           (Emit_جایگاه_رقم_از_پشته_بالاترین_And_برداشت(o));
        }

        EmitConstant32(o, (u32)حافظه_جابه_جایی);
        إذا (i_opcode >= c_wa_Op_atomic_اولی_Access) {
            EmitConstant32(o, GetAtomicDesc(i_opcode & 0xFF));
        }

        إذا (حافظه_Idx) {
_           (Emit_مجموعه_حافظه(o, 0));
        }

        إذا (نتیجه_Type != c_m3Type_none) {
_           (افزودن_ثبات(o, نتیجه_Type));
        }
    }
}
    _catch: أرجع نتیجه;
}

#پایان_اگر // d_m3HasAtomics


M3_نتیجه CompileRawFunction (IM3Module io_module, IM3Function io_function, ثابت فراغ* i_function, ثابت فراغ* i_userdata)
{
    d_m3Assert(io_module->runtime);

    IM3_کد_صفحه صفحه = Acquire_کد_صفحه_With_ظرفیت(io_module->runtime, 4);

    إذا (صفحه) {
        io_function->compiled = Get_صفحه_PC(صفحه);
        io_function->module   = io_module;

        // Unless a host module's ABI names a memory (m3_BindImportMemory), a
        // host function addresses memory 0 - what a bare i32 guest pointer means
        io_function->host_حافظه = io_module->memory0;

        Emit_کلمه(صفحه, op_CallRawFunction);
        Emit_کلمه(صفحه, i_function);
        Emit_کلمه(صفحه, io_function);
        Emit_کلمه(صفحه, i_userdata);

        Release_کد_صفحه(io_module->runtime, صفحه);
        أرجع m3Err_none;
    } وإلا {
        أرجع m3_Err_malloc_Failed_کد_صفحه;
    }
}


// A table entry names every field it sets, so the struct can order them for
// packing; the columns follow that order, since M3OP names them positionally.
//
// d_logOp, d_logOp2 macros aren't actually used by the compiler, just codepage decoding (d_m3LogCodePages = 1)
#تعریف_کن d_ops(...)                          .operations = { __VA_ARGS__ }

#تعریف_کن d_logOp(OP)                         d_ops( op_##OP,                 عدم,                       عدم,                       عدم )
#تعریف_کن d_logOp2(OP1,OP2)                   d_ops( op_##OP1,                op_##OP2,                   عدم,                       عدم )

#تعریف_کن d_خالی_Op_List                       d_ops( عدم,                    عدم,                       عدم,                       عدم )
#تعریف_کن d_unaryOpList(TYPE, NAME)           d_ops( op_##TYPE##_##NAME##_r,  op_##TYPE##_##NAME##_s,     عدم,                       عدم )
#تعریف_کن d_binOpList(TYPE, NAME)             d_ops( op_##TYPE##_##NAME##_rs, op_##TYPE##_##NAME##_sr,    op_##TYPE##_##NAME##_ss,    عدم )
#تعریف_کن d_storeFpOpList(TYPE, NAME)         d_ops( op_##TYPE##_##NAME##_rs, op_##TYPE##_##NAME##_sr,    op_##TYPE##_##NAME##_ss,    op_##TYPE##_##NAME##_rr )
#تعریف_کن d_commutativeBinOpList(TYPE, NAME)  d_ops( op_##TYPE##_##NAME##_rs, عدم,                       op_##TYPE##_##NAME##_ss,    عدم )

#تعریف_کن d_convertOpList(OP)                 d_ops( op_##OP##_r_r,           op_##OP##_r_s,              op_##OP##_s_r,              op_##OP##_s_s )


// Every M3Compiler the tables below name, as one list, so that the index enum and
// the array of pointers can never drift apart. Each entry must carry the same
// config guard as the function's definition.
#تعریف_کن d_m3CompilerList(_)             \
    _( Compile_Unreachable )            \
    _( Compile_Nop )                    \
    _( Compile_Loop_Or_بلوک )            \
    _( Compile_If )                     \
    _( Compile_انتها )                    \
    _( Compile_Branch )                 \
    _( Compile_Branch_جدول )            \
    _( Compile_Return )                 \
    _( Compile_Call )                   \
    _( Compile_Call_غیرمستقیم )           \
    _( Compile_Drop )                   \
    _( Compile_Select )                 \
    _( Compile_GetLocal )               \
    _( Compile_مجموعه_Local )               \
    _( Compile_Get_مجموعه_Global )           \
    _( Compile_بارگیری_Store )             \
    _( Compile_حافظه_اندازه )            \
    _( Compile_حافظه_Grow )            \
    _( Compile_حافظه_Init )            \
    _( Compile_حافظه_Copy_Fill )        \
    _( Compile_داده_Drop )              \
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
    d_m3_Compiler_List_پشته_Switching( _ )

#إذا d_m3ImplementFloat
#  define d_m3CompilerList_f(_)         \
    _( Compile_Const_f32 )              \
    _( Compile_Const_f64 )
#وإلا
#  define d_m3CompilerList_f(_)
#پایان_اگر

#إذا d_m3HasRefTypes
#  define d_m3CompilerList_refTypes(_)  \
    _( Compile_Select_Typed )           \
    _( Compile_Ref_Null )               \
    _( Compile_Ref_IsNull )             \
    _( Compile_Ref_Func )               \
    _( Compile_جدول_Get_مجموعه )           \
    _( Compile_جدول_Init )             \
    _( Compile_جدول_Copy )             \
    _( Compile_جدول_اندازه )             \
    _( Compile_جدول_Grow_Fill )         \
    _( Compile_Elem_Drop )
#وإلا
#  define d_m3CompilerList_refTypes(_)
#پایان_اگر

#إذا d_m3HasTypedRefs
#  define d_m3CompilerList_typedRefs(_) \
    _( Compile_CallRef )                \
    _( Compile_Ref_AsNonNull )
#وإلا
#  define d_m3CompilerList_typedRefs(_)
#پایان_اگر

#إذا d_m3HasExceptionHandling
#  define d_m3CompilerList_eh(_)        \
    _( Compile_Throw )                  \
    _( Compile_ThrowRef )               \
    _( Compile_Try_جدول )
#وإلا
#  define d_m3CompilerList_eh(_)
#پایان_اگر

#إذا d_m3HasAtomics
#  define d_m3CompilerList_atomics(_)   \
    _( Compile_Atomic )
#وإلا
#  define d_m3CompilerList_atomics(_)
#پایان_اگر

#إذا d_m3HasWideArithmetic
#  define d_m3CompilerList_wide(_)      \
    _( Compile_WideArithmetic )
#وإلا
#  define d_m3CompilerList_wide(_)
#پایان_اگر

#إذا d_m3_Has_پشته_Switching
#  define d_m3_Compiler_List_پشته_Switching(_) \
    _( Compile_ContNew )                     \
    _( Compile_ContBind )                    \
    _( Compile_Suspend )                     \
    _( Compile_Resume )                      \
    _( Compile_Switch )
#وإلا
#  define d_m3_Compiler_List_پشته_Switching(_)
#پایان_اگر

// Index 0 is "this opcode has no compiler", so that a zeroed entry - which is
// what M3OP_RESERVED and the reserved holes in the tables are - reads as one.
تعداد {
    c_cc_NULL = 0,
#تعریف_کن d_m3CompilerEnum(NAME)  c_cc_##NAME,
    d_m3CompilerList(d_m3CompilerEnum)
#لغو_تعریف d_m3CompilerEnum
};

ساكن ثابت M3Compiler c_compilers[] = {
    عدم,
#تعریف_کن d_m3CompilerPtr(NAME)   NAME,
    d_m3CompilerList(d_m3CompilerPtr)
#لغو_تعریف d_m3CompilerPtr
};

// Names a table entry's compiler by index. d_cc (NULL) is the no-compiler entry.
#تعریف_کن d_cc(NAME)                          .compiler = c_cc_##NAME

ساكن
M3Compiler GetCompiler (IM3OpInfo i_opInfo)
{
    d_m3Assert(i_opInfo->compiler < M3_شمار_از(c_compilers));
    أرجع c_compilers[i_opInfo->compiler];
}

// clang-format off
ثابت M3OpInfo c_operations[] =
{
    M3OP( "unreachable",           0,  none,   d_cc(Compile_Unreachable),  d_logOp (Unreachable) ),                   // 0x00
    M3OP( "nop",                   0,  none,   d_cc(Compile_Nop),          d_خالی_Op_List ),                           // 0x01 .
    M3OP( "block",                 0,  none,   d_cc(Compile_Loop_Or_بلوک),  d_خالی_Op_List ),                           // 0x02
    M3OP( "loop",                  0,  none,   d_cc(Compile_Loop_Or_بلوک),  d_logOp (Loop) ),                          // 0x03
    M3OP( "if",                    -1, none,   d_cc(Compile_If),           d_خالی_Op_List ),                           // 0x04
    M3OP( "else",                  0,  none,   d_cc(Compile_Nop),          d_خالی_Op_List ),                           // 0x05

    M3OP_RESERVED,  M3OP_RESERVED,                                                                                    // 0x06...0x07

#إذا d_m3HasExceptionHandling
    M3OP( "throw",                 0,  none,   d_cc(Compile_Throw),        d_logOp (Throw) ),                         // 0x08
    M3OP_RESERVED,                                                                                                    // 0x09
    M3OP( "throw_ref",             0,  none,   d_cc(Compile_ThrowRef),     d_logOp (ThrowRef) ),                      // 0x0a
#وإلا
    M3OP_RESERVED,  M3OP_RESERVED, M3OP_RESERVED,                                                                     // 0x08...0x0a
#پایان_اگر

    M3OP( "end",                   0,  none,   d_cc(Compile_انتها),          d_خالی_Op_List ),                           // 0x0b
    M3OP( "br",                    0,  none,   d_cc(Compile_Branch),       d_logOp (Branch) ),                        // 0x0c
    M3OP( "br_if",                 -1, none,   d_cc(Compile_Branch),       d_logOp2 (BranchIf_r, BranchIf_s) ),       // 0x0d
    M3OP( "br_table",              -1, none,   d_cc(Compile_Branch_جدول),  d_logOp (Branch_جدول) ),                   // 0x0e
    M3OP( "return",                0,  any,    d_cc(Compile_Return),       d_logOp (Return) ),                        // 0x0f
    M3OP( "call",                  0,  any,    d_cc(Compile_Call),         d_logOp (Call) ),                          // 0x10
    M3OP( "call_indirect",         0,  any,    d_cc(Compile_Call_غیرمستقیم), d_logOp (Call_غیرمستقیم) ),                  // 0x11
    M3OP( "return_call",           0,  any,    d_cc(Compile_Call),         d_logOp (ReturnCall) ),                    // 0x12
    M3OP( "return_call_indirect",  0,  any,    d_cc(Compile_Call_غیرمستقیم), d_logOp (Return_Call_غیرمستقیم) ),            // 0x13

#إذا d_m3HasTypedRefs
    M3OP( "call_ref",              0,  any,    d_cc(Compile_CallRef),      d_logOp (CallRef) ),                       // 0x14
    M3OP( "return_call_ref",       0,  any,    d_cc(Compile_CallRef),      d_logOp (ReturnCallRef) ),                 // 0x15
#وإلا
    M3OP_RESERVED,  M3OP_RESERVED,                                                                                    // 0x14...
#پایان_اگر
    M3OP_RESERVED,  M3OP_RESERVED, M3OP_RESERVED, M3OP_RESERVED,                                                      // ...0x19

    M3OP( "drop",                  -1, none,   d_cc(Compile_Drop),         d_خالی_Op_List ),                           // 0x1a
    M3OP( "select",                -2, any,    d_cc(Compile_Select),       d_خالی_Op_List ),                           // 0x1b

#إذا d_m3HasRefTypes
    M3OP( "select.t",              -2, any,    d_cc(Compile_Select_Typed), d_خالی_Op_List ),                           // 0x1c
#وإلا
    M3OP_RESERVED,                                                                                                    // 0x1c
#پایان_اگر
    M3OP_RESERVED,  M3OP_RESERVED,                                                                                    // 0x1d...0x1e

#إذا d_m3HasExceptionHandling
    M3OP( "try_table",             0,  none,   d_cc(Compile_Try_جدول),     d_logOp (Try_جدول) ),                      // 0x1f
#وإلا
    M3OP_RESERVED,                                                                                                    // 0x1f
#پایان_اگر

    M3OP( "local.get",             1,  any,    d_cc(Compile_GetLocal),     d_خالی_Op_List ),                           // 0x20
    M3OP( "local.set",             1,  none,   d_cc(Compile_مجموعه_Local),     d_خالی_Op_List ),                           // 0x21
    M3OP( "local.tee",             0,  any,    d_cc(Compile_مجموعه_Local),     d_خالی_Op_List ),                           // 0x22
    M3OP( "global.get",            1,  none,   d_cc(Compile_Get_مجموعه_Global), d_خالی_Op_List ),                           // 0x23
    M3OP( "global.set",            1,  none,   d_cc(Compile_Get_مجموعه_Global), d_خالی_Op_List ),                           // 0x24

#إذا d_m3HasRefTypes
    M3OP( "table.get",             0,  any,    d_cc(Compile_جدول_Get_مجموعه), d_خالی_Op_List ),                           // 0x25
    M3OP( "table.set",             -2, none,   d_cc(Compile_جدول_Get_مجموعه), d_خالی_Op_List ),                           // 0x26
#وإلا
    M3OP_RESERVED,  M3OP_RESERVED,                                                                                    // 0x25...0x26
#پایان_اگر
    M3OP_RESERVED,                                                                                                    // 0x27

    M3OP( "i32.load",              0,  i_32,   d_cc(Compile_بارگیری_Store),   d_unaryOpList (i32, بارگیری_i32) ),           // 0x28
    M3OP( "i64.load",              0,  i_64,   d_cc(Compile_بارگیری_Store),   d_unaryOpList (i64, بارگیری_i64) ),           // 0x29
    M3OP_F( "f32.load",            0,  f_32,   d_cc(Compile_بارگیری_Store),   d_unaryOpList (f32, بارگیری_f32) ),           // 0x2a
    M3OP_F( "f64.load",            0,  f_64,   d_cc(Compile_بارگیری_Store),   d_unaryOpList (f64, بارگیری_f64) ),           // 0x2b

    M3OP( "i32.load8_s",           0,  i_32,   d_cc(Compile_بارگیری_Store),   d_unaryOpList (i32, بارگیری_i8) ),            // 0x2c
    M3OP( "i32.load8_u",           0,  i_32,   d_cc(Compile_بارگیری_Store),   d_unaryOpList (i32, بارگیری_u8) ),            // 0x2d
    M3OP( "i32.load16_s",          0,  i_32,   d_cc(Compile_بارگیری_Store),   d_unaryOpList (i32, بارگیری_i16) ),           // 0x2e
    M3OP( "i32.load16_u",          0,  i_32,   d_cc(Compile_بارگیری_Store),   d_unaryOpList (i32, بارگیری_u16) ),           // 0x2f

    M3OP( "i64.load8_s",           0,  i_64,   d_cc(Compile_بارگیری_Store),   d_unaryOpList (i64, بارگیری_i8) ),            // 0x30
    M3OP( "i64.load8_u",           0,  i_64,   d_cc(Compile_بارگیری_Store),   d_unaryOpList (i64, بارگیری_u8) ),            // 0x31
    M3OP( "i64.load16_s",          0,  i_64,   d_cc(Compile_بارگیری_Store),   d_unaryOpList (i64, بارگیری_i16) ),           // 0x32
    M3OP( "i64.load16_u",          0,  i_64,   d_cc(Compile_بارگیری_Store),   d_unaryOpList (i64, بارگیری_u16) ),           // 0x33
    M3OP( "i64.load32_s",          0,  i_64,   d_cc(Compile_بارگیری_Store),   d_unaryOpList (i64, بارگیری_i32) ),           // 0x34
    M3OP( "i64.load32_u",          0,  i_64,   d_cc(Compile_بارگیری_Store),   d_unaryOpList (i64, بارگیری_u32) ),           // 0x35

    M3OP( "i32.store",             -2, none,   d_cc(Compile_بارگیری_Store),   d_binOpList (i32, Store_i32) ),            // 0x36
    M3OP( "i64.store",             -2, none,   d_cc(Compile_بارگیری_Store),   d_binOpList (i64, Store_i64) ),            // 0x37
    M3OP_F( "f32.store",           -2, none,   d_cc(Compile_بارگیری_Store),   d_storeFpOpList (f32, Store_f32) ),        // 0x38
    M3OP_F( "f64.store",           -2, none,   d_cc(Compile_بارگیری_Store),   d_storeFpOpList (f64, Store_f64) ),        // 0x39

    M3OP( "i32.store8",            -2, none,   d_cc(Compile_بارگیری_Store),   d_binOpList (i32, Store_u8) ),             // 0x3a
    M3OP( "i32.store16",           -2, none,   d_cc(Compile_بارگیری_Store),   d_binOpList (i32, Store_i16) ),            // 0x3b

    M3OP( "i64.store8",            -2, none,   d_cc(Compile_بارگیری_Store),   d_binOpList (i64, Store_u8) ),             // 0x3c
    M3OP( "i64.store16",           -2, none,   d_cc(Compile_بارگیری_Store),   d_binOpList (i64, Store_i16) ),            // 0x3d
    M3OP( "i64.store32",           -2, none,   d_cc(Compile_بارگیری_Store),   d_binOpList (i64, Store_i32) ),            // 0x3e

    M3OP( "memory.size",           1,  i_32,   d_cc(Compile_حافظه_اندازه),  d_logOp (Mem_اندازه) ),                       // 0x3f
    M3OP( "memory.grow",           1,  i_32,   d_cc(Compile_حافظه_Grow),  d_logOp (MemGrow) ),                       // 0x40

    M3OP( "i32.const",             1,  i_32,   d_cc(Compile_Const_i32),    d_logOp (Const32) ),                       // 0x41
    M3OP( "i64.const",             1,  i_64,   d_cc(Compile_Const_i64),    d_logOp (Const64) ),                       // 0x42
    M3OP_F( "f32.const",           1,  f_32,   d_cc(Compile_Const_f32),    d_خالی_Op_List ),                           // 0x43
    M3OP_F( "f64.const",           1,  f_64,   d_cc(Compile_Const_f64),    d_خالی_Op_List ),                           // 0x44

    M3OP( "i32.eqz",               0,  i_32,   d_cc(عدم),                 d_unaryOpList (i32, Equal_مقصد_Zero) ),        // 0x45
    M3OP( "i32.eq",                -1, i_32,   d_cc(عدم),                 d_commutativeBinOpList (i32, Equal) ),     // 0x46
    M3OP( "i32.ne",                -1, i_32,   d_cc(عدم),                 d_commutativeBinOpList (i32, NotEqual) ),  // 0x47
    M3OP( "i32.lt_s",              -1, i_32,   d_cc(عدم),                 d_binOpList (i32, LessThan) ),             // 0x48
    M3OP( "i32.lt_u",              -1, i_32,   d_cc(عدم),                 d_binOpList (u32, LessThan) ),             // 0x49
    M3OP( "i32.gt_s",              -1, i_32,   d_cc(عدم),                 d_binOpList (i32, GreaterThan) ),          // 0x4a
    M3OP( "i32.gt_u",              -1, i_32,   d_cc(عدم),                 d_binOpList (u32, GreaterThan) ),          // 0x4b
    M3OP( "i32.le_s",              -1, i_32,   d_cc(عدم),                 d_binOpList (i32, LessThanOrEqual) ),      // 0x4c
    M3OP( "i32.le_u",              -1, i_32,   d_cc(عدم),                 d_binOpList (u32, LessThanOrEqual) ),      // 0x4d
    M3OP( "i32.ge_s",              -1, i_32,   d_cc(عدم),                 d_binOpList (i32, GreaterThanOrEqual) ),   // 0x4e
    M3OP( "i32.ge_u",              -1, i_32,   d_cc(عدم),                 d_binOpList (u32, GreaterThanOrEqual) ),   // 0x4f

    M3OP( "i64.eqz",               0,  i_32,   d_cc(عدم),                 d_unaryOpList (i64, Equal_مقصد_Zero) ),        // 0x50
    M3OP( "i64.eq",                -1, i_32,   d_cc(عدم),                 d_commutativeBinOpList (i64, Equal) ),     // 0x51
    M3OP( "i64.ne",                -1, i_32,   d_cc(عدم),                 d_commutativeBinOpList (i64, NotEqual) ),  // 0x52
    M3OP( "i64.lt_s",              -1, i_32,   d_cc(عدم),                 d_binOpList (i64, LessThan) ),             // 0x53
    M3OP( "i64.lt_u",              -1, i_32,   d_cc(عدم),                 d_binOpList (u64, LessThan) ),             // 0x54
    M3OP( "i64.gt_s",              -1, i_32,   d_cc(عدم),                 d_binOpList (i64, GreaterThan) ),          // 0x55
    M3OP( "i64.gt_u",              -1, i_32,   d_cc(عدم),                 d_binOpList (u64, GreaterThan) ),          // 0x56
    M3OP( "i64.le_s",              -1, i_32,   d_cc(عدم),                 d_binOpList (i64, LessThanOrEqual) ),      // 0x57
    M3OP( "i64.le_u",              -1, i_32,   d_cc(عدم),                 d_binOpList (u64, LessThanOrEqual) ),      // 0x58
    M3OP( "i64.ge_s",              -1, i_32,   d_cc(عدم),                 d_binOpList (i64, GreaterThanOrEqual) ),   // 0x59
    M3OP( "i64.ge_u",              -1, i_32,   d_cc(عدم),                 d_binOpList (u64, GreaterThanOrEqual) ),   // 0x5a

    M3OP_F( "f32.eq",              -1, i_32,   d_cc(عدم),                 d_commutativeBinOpList (f32, Equal) ),     // 0x5b
    M3OP_F( "f32.ne",              -1, i_32,   d_cc(عدم),                 d_commutativeBinOpList (f32, NotEqual) ),  // 0x5c
    M3OP_F( "f32.lt",              -1, i_32,   d_cc(عدم),                 d_binOpList (f32, LessThan) ),             // 0x5d
    M3OP_F( "f32.gt",              -1, i_32,   d_cc(عدم),                 d_binOpList (f32, GreaterThan) ),          // 0x5e
    M3OP_F( "f32.le",              -1, i_32,   d_cc(عدم),                 d_binOpList (f32, LessThanOrEqual) ),      // 0x5f
    M3OP_F( "f32.ge",              -1, i_32,   d_cc(عدم),                 d_binOpList (f32, GreaterThanOrEqual) ),   // 0x60

    M3OP_F( "f64.eq",              -1, i_32,   d_cc(عدم),                 d_commutativeBinOpList (f64, Equal) ),     // 0x61
    M3OP_F( "f64.ne",              -1, i_32,   d_cc(عدم),                 d_commutativeBinOpList (f64, NotEqual) ),  // 0x62
    M3OP_F( "f64.lt",              -1, i_32,   d_cc(عدم),                 d_binOpList (f64, LessThan) ),             // 0x63
    M3OP_F( "f64.gt",              -1, i_32,   d_cc(عدم),                 d_binOpList (f64, GreaterThan) ),          // 0x64
    M3OP_F( "f64.le",              -1, i_32,   d_cc(عدم),                 d_binOpList (f64, LessThanOrEqual) ),      // 0x65
    M3OP_F( "f64.ge",              -1, i_32,   d_cc(عدم),                 d_binOpList (f64, GreaterThanOrEqual) ),   // 0x66

    M3OP( "i32.clz",               0,  i_32,   d_cc(عدم),                 d_unaryOpList (u32, Clz) ),                // 0x67
    M3OP( "i32.ctz",               0,  i_32,   d_cc(عدم),                 d_unaryOpList (u32, Ctz) ),                // 0x68
    M3OP( "i32.popcnt",            0,  i_32,   d_cc(عدم),                 d_unaryOpList (u32, Popcnt) ),             // 0x69

    M3OP( "i32.add",               -1, i_32,   d_cc(عدم),                 d_commutativeBinOpList (i32, جمع_کن) ),       // 0x6a
    M3OP( "i32.sub",               -1, i_32,   d_cc(عدم),                 d_binOpList (i32, تفریق) ),             // 0x6b
    M3OP( "i32.mul",               -1, i_32,   d_cc(عدم),                 d_commutativeBinOpList (i32, Multiply) ),  // 0x6c
    M3OP( "i32.div_s",             -1, i_32,   d_cc(عدم),                 d_binOpList (i32, Divide) ),               // 0x6d
    M3OP( "i32.div_u",             -1, i_32,   d_cc(عدم),                 d_binOpList (u32, Divide) ),               // 0x6e
    M3OP( "i32.rem_s",             -1, i_32,   d_cc(عدم),                 d_binOpList (i32, Remainder) ),            // 0x6f
    M3OP( "i32.rem_u",             -1, i_32,   d_cc(عدم),                 d_binOpList (u32, Remainder) ),            // 0x70
    M3OP( "i32.and",               -1, i_32,   d_cc(عدم),                 d_commutativeBinOpList (u32, And) ),       // 0x71
    M3OP( "i32.or",                -1, i_32,   d_cc(عدم),                 d_commutativeBinOpList (u32, Or) ),        // 0x72
    M3OP( "i32.xor",               -1, i_32,   d_cc(عدم),                 d_commutativeBinOpList (u32, Xor) ),       // 0x73
    M3OP( "i32.shl",               -1, i_32,   d_cc(عدم),                 d_binOpList (u32, لغزش_چپ) ),            // 0x74
    M3OP( "i32.shr_s",             -1, i_32,   d_cc(عدم),                 d_binOpList (i32, لغزش_راست) ),           // 0x75
    M3OP( "i32.shr_u",             -1, i_32,   d_cc(عدم),                 d_binOpList (u32, لغزش_راست) ),           // 0x76
    M3OP( "i32.rotl",              -1, i_32,   d_cc(عدم),                 d_binOpList (u32, Rotl) ),                 // 0x77
    M3OP( "i32.rotr",              -1, i_32,   d_cc(عدم),                 d_binOpList (u32, Rotr) ),                 // 0x78

    M3OP( "i64.clz",               0,  i_64,   d_cc(عدم),                 d_unaryOpList (u64, Clz) ),                // 0x79
    M3OP( "i64.ctz",               0,  i_64,   d_cc(عدم),                 d_unaryOpList (u64, Ctz) ),                // 0x7a
    M3OP( "i64.popcnt",            0,  i_64,   d_cc(عدم),                 d_unaryOpList (u64, Popcnt) ),             // 0x7b

    M3OP( "i64.add",               -1, i_64,   d_cc(عدم),                 d_commutativeBinOpList (i64, جمع_کن) ),       // 0x7c
    M3OP( "i64.sub",               -1, i_64,   d_cc(عدم),                 d_binOpList (i64, تفریق) ),             // 0x7d
    M3OP( "i64.mul",               -1, i_64,   d_cc(عدم),                 d_commutativeBinOpList (i64, Multiply) ),  // 0x7e
    M3OP( "i64.div_s",             -1, i_64,   d_cc(عدم),                 d_binOpList (i64, Divide) ),               // 0x7f
    M3OP( "i64.div_u",             -1, i_64,   d_cc(عدم),                 d_binOpList (u64, Divide) ),               // 0x80
    M3OP( "i64.rem_s",             -1, i_64,   d_cc(عدم),                 d_binOpList (i64, Remainder) ),            // 0x81
    M3OP( "i64.rem_u",             -1, i_64,   d_cc(عدم),                 d_binOpList (u64, Remainder) ),            // 0x82
    M3OP( "i64.and",               -1, i_64,   d_cc(عدم),                 d_commutativeBinOpList (u64, And) ),       // 0x83
    M3OP( "i64.or",                -1, i_64,   d_cc(عدم),                 d_commutativeBinOpList (u64, Or) ),        // 0x84
    M3OP( "i64.xor",               -1, i_64,   d_cc(عدم),                 d_commutativeBinOpList (u64, Xor) ),       // 0x85
    M3OP( "i64.shl",               -1, i_64,   d_cc(عدم),                 d_binOpList (u64, لغزش_چپ) ),            // 0x86
    M3OP( "i64.shr_s",             -1, i_64,   d_cc(عدم),                 d_binOpList (i64, لغزش_راست) ),           // 0x87
    M3OP( "i64.shr_u",             -1, i_64,   d_cc(عدم),                 d_binOpList (u64, لغزش_راست) ),           // 0x88
    M3OP( "i64.rotl",              -1, i_64,   d_cc(عدم),                 d_binOpList (u64, Rotl) ),                 // 0x89
    M3OP( "i64.rotr",              -1, i_64,   d_cc(عدم),                 d_binOpList (u64, Rotr) ),                 // 0x8a

    M3OP_F( "f32.abs",             0,  f_32,   d_cc(عدم),                 d_unaryOpList(f32, Abs) ),                 // 0x8b
    M3OP_F( "f32.neg",             0,  f_32,   d_cc(عدم),                 d_unaryOpList(f32, Negate) ),              // 0x8c
    M3OP_F( "f32.ceil",            0,  f_32,   d_cc(عدم),                 d_unaryOpList(f32, Ceil) ),                // 0x8d
    M3OP_F( "f32.floor",           0,  f_32,   d_cc(عدم),                 d_unaryOpList(f32, Floor) ),               // 0x8e
    M3OP_F( "f32.trunc",           0,  f_32,   d_cc(عدم),                 d_unaryOpList(f32, Trunc) ),               // 0x8f
    M3OP_F( "f32.nearest",         0,  f_32,   d_cc(عدم),                 d_unaryOpList(f32, نزدیکترین) ),             // 0x90
    M3OP_F( "f32.sqrt",            0,  f_32,   d_cc(عدم),                 d_unaryOpList(f32, Sqrt) ),                // 0x91

    M3OP_F( "f32.add",             -1, f_32,   d_cc(عدم),                 d_commutativeBinOpList (f32, جمع_کن) ),       // 0x92
    M3OP_F( "f32.sub",             -1, f_32,   d_cc(عدم),                 d_binOpList (f32, تفریق) ),             // 0x93
    M3OP_F( "f32.mul",             -1, f_32,   d_cc(عدم),                 d_commutativeBinOpList (f32, Multiply) ),  // 0x94
    M3OP_F( "f32.div",             -1, f_32,   d_cc(عدم),                 d_binOpList (f32, Divide) ),               // 0x95
    M3OP_F( "f32.min",             -1, f_32,   d_cc(عدم),                 d_commutativeBinOpList (f32, Min) ),       // 0x96
    M3OP_F( "f32.max",             -1, f_32,   d_cc(عدم),                 d_commutativeBinOpList (f32, Max) ),       // 0x97
    M3OP_F( "f32.copysign",        -1, f_32,   d_cc(عدم),                 d_binOpList (f32, CopySign) ),             // 0x98

    M3OP_F( "f64.abs",             0,  f_64,   d_cc(عدم),                 d_unaryOpList(f64, Abs) ),                 // 0x99
    M3OP_F( "f64.neg",             0,  f_64,   d_cc(عدم),                 d_unaryOpList(f64, Negate) ),              // 0x9a
    M3OP_F( "f64.ceil",            0,  f_64,   d_cc(عدم),                 d_unaryOpList(f64, Ceil) ),                // 0x9b
    M3OP_F( "f64.floor",           0,  f_64,   d_cc(عدم),                 d_unaryOpList(f64, Floor) ),               // 0x9c
    M3OP_F( "f64.trunc",           0,  f_64,   d_cc(عدم),                 d_unaryOpList(f64, Trunc) ),               // 0x9d
    M3OP_F( "f64.nearest",         0,  f_64,   d_cc(عدم),                 d_unaryOpList(f64, نزدیکترین) ),             // 0x9e
    M3OP_F( "f64.sqrt",            0,  f_64,   d_cc(عدم),                 d_unaryOpList(f64, Sqrt) ),                // 0x9f

    M3OP_F( "f64.add",             -1, f_64,   d_cc(عدم),                 d_commutativeBinOpList (f64, جمع_کن) ),       // 0xa0
    M3OP_F( "f64.sub",             -1, f_64,   d_cc(عدم),                 d_binOpList (f64, تفریق) ),             // 0xa1
    M3OP_F( "f64.mul",             -1, f_64,   d_cc(عدم),                 d_commutativeBinOpList (f64, Multiply) ),  // 0xa2
    M3OP_F( "f64.div",             -1, f_64,   d_cc(عدم),                 d_binOpList (f64, Divide) ),               // 0xa3
    M3OP_F( "f64.min",             -1, f_64,   d_cc(عدم),                 d_commutativeBinOpList (f64, Min) ),       // 0xa4
    M3OP_F( "f64.max",             -1, f_64,   d_cc(عدم),                 d_commutativeBinOpList (f64, Max) ),       // 0xa5
    M3OP_F( "f64.copysign",        -1, f_64,   d_cc(عدم),                 d_binOpList (f64, CopySign) ),             // 0xa6

    M3OP( "i32.wrap/i64",          0,  i_32,   d_cc(عدم),                 d_unaryOpList (i32, Wrap_i64) ),           // 0xa7
    M3OP_F( "i32.trunc_s/f32",     0,  i_32,   d_cc(Compile_Convert),      d_convertOpList (i32_Trunc_f32) ),         // 0xa8
    M3OP_F( "i32.trunc_u/f32",     0,  i_32,   d_cc(Compile_Convert),      d_convertOpList (u32_Trunc_f32) ),         // 0xa9
    M3OP_F( "i32.trunc_s/f64",     0,  i_32,   d_cc(Compile_Convert),      d_convertOpList (i32_Trunc_f64) ),         // 0xaa
    M3OP_F( "i32.trunc_u/f64",     0,  i_32,   d_cc(Compile_Convert),      d_convertOpList (u32_Trunc_f64) ),         // 0xab

    M3OP( "i64.extend_s/i32",      0,  i_64,   d_cc(عدم),                 d_unaryOpList (i64, Extend_i32) ),         // 0xac
    M3OP( "i64.extend_u/i32",      0,  i_64,   d_cc(عدم),                 d_unaryOpList (i64, Extend_u32) ),         // 0xad

    M3OP_F( "i64.trunc_s/f32",     0,  i_64,   d_cc(Compile_Convert),      d_convertOpList (i64_Trunc_f32) ),         // 0xae
    M3OP_F( "i64.trunc_u/f32",     0,  i_64,   d_cc(Compile_Convert),      d_convertOpList (u64_Trunc_f32) ),         // 0xaf
    M3OP_F( "i64.trunc_s/f64",     0,  i_64,   d_cc(Compile_Convert),      d_convertOpList (i64_Trunc_f64) ),         // 0xb0
    M3OP_F( "i64.trunc_u/f64",     0,  i_64,   d_cc(Compile_Convert),      d_convertOpList (u64_Trunc_f64) ),         // 0xb1

    M3OP_F( "f32.convert_s/i32",   0,  f_32,   d_cc(Compile_Convert),      d_convertOpList (f32_Convert_i32) ),       // 0xb2
    M3OP_F( "f32.convert_u/i32",   0,  f_32,   d_cc(Compile_Convert),      d_convertOpList (f32_Convert_u32) ),       // 0xb3
    M3OP_F( "f32.convert_s/i64",   0,  f_32,   d_cc(Compile_Convert),      d_convertOpList (f32_Convert_i64) ),       // 0xb4
    M3OP_F( "f32.convert_u/i64",   0,  f_32,   d_cc(Compile_Convert),      d_convertOpList (f32_Convert_u64) ),       // 0xb5

    M3OP_F( "f32.demote/f64",      0,  f_32,   d_cc(عدم),                 d_unaryOpList (f32, Demote_f64) ),         // 0xb6

    M3OP_F( "f64.convert_s/i32",   0,  f_64,   d_cc(Compile_Convert),      d_convertOpList (f64_Convert_i32) ),       // 0xb7
    M3OP_F( "f64.convert_u/i32",   0,  f_64,   d_cc(Compile_Convert),      d_convertOpList (f64_Convert_u32) ),       // 0xb8
    M3OP_F( "f64.convert_s/i64",   0,  f_64,   d_cc(Compile_Convert),      d_convertOpList (f64_Convert_i64) ),       // 0xb9
    M3OP_F( "f64.convert_u/i64",   0,  f_64,   d_cc(Compile_Convert),      d_convertOpList (f64_Convert_u64) ),       // 0xba

    M3OP_F( "f64.promote/f32",     0,  f_64,   d_cc(عدم),                 d_unaryOpList (f64, Promote_f32) ),        // 0xbb

    M3OP_F( "i32.reinterpret/f32", 0,  i_32,   d_cc(Compile_Convert),      d_convertOpList (i32_Reinterpret_f32) ),   // 0xbc
    M3OP_F( "i64.reinterpret/f64", 0,  i_64,   d_cc(Compile_Convert),      d_convertOpList (i64_Reinterpret_f64) ),   // 0xbd
    M3OP_F( "f32.reinterpret/i32", 0,  f_32,   d_cc(Compile_Convert),      d_convertOpList (f32_Reinterpret_i32) ),   // 0xbe
    M3OP_F( "f64.reinterpret/i64", 0,  f_64,   d_cc(Compile_Convert),      d_convertOpList (f64_Reinterpret_i64) ),   // 0xbf

    M3OP( "i32.extend8_s",         0,  i_32,   d_cc(عدم),                 d_unaryOpList (i32, Extend8_s) ),          // 0xc0
    M3OP( "i32.extend16_s",        0,  i_32,   d_cc(عدم),                 d_unaryOpList (i32, Extend16_s) ),         // 0xc1
    M3OP( "i64.extend8_s",         0,  i_64,   d_cc(عدم),                 d_unaryOpList (i64, Extend8_s) ),          // 0xc2
    M3OP( "i64.extend16_s",        0,  i_64,   d_cc(عدم),                 d_unaryOpList (i64, Extend16_s) ),         // 0xc3
    M3OP( "i64.extend32_s",        0,  i_64,   d_cc(عدم),                 d_unaryOpList (i64, Extend32_s) ),         // 0xc4

// Internal operations, for codepage logging only. They sit past c_waOp_lastCore,
// the last opcode this table covers, so GetOpInfo () can never reach them.
# ifdef DEBUG // for codepage logging. the order doesn't matter:
#تعریف_کن d_m3DebugOp(OP) M3OP (#OP, 0, none, d_ops (op_##OP))

# إذا d_m3HasFloat
#تعریف_کن d_m3DebugTypedOp(OP) M3OP (#OP, 0, none, d_ops (op_##OP##_i32, op_##OP##_i64, op_##OP##_f32, op_##OP##_f64))
# وإلا
#تعریف_کن d_m3DebugTypedOp(OP) M3OP (#OP, 0, none, d_ops (op_##OP##_i32, op_##OP##_i64))
# endif

    d_m3DebugOp (Compile),          d_m3DebugOp (مدخل_2),            d_m3DebugOp (انتها_2),
    d_m3DebugOp (Unsupported),      d_m3DebugOp (CallRawFunction),

    d_m3DebugOp (GetGlobal_s32),    d_m3DebugOp (GetGlobal_s64),    d_m3DebugOp (ContinueLoop),     d_m3DebugOp (ContinueLoopIf),
# إذا d_m3_Has_پشته_Switching
    d_m3DebugOp (ContinueLoop_Suspendable), d_m3DebugOp (ContinueLoopIf_Suspendable), d_m3DebugOp (مدخل_Check),
#  إذا !d_m3_مدخل_Keeps_قاب
    d_m3DebugOp (مدخل_Suspendable),
#  endif
# endif

    d_m3DebugOp (Copy_جایگاه_32),      d_m3DebugOp (Preserve_Copy_جایگاه_32), d_m3DebugOp (If_s),          d_m3DebugOp (BranchIfPrologue_s),
    d_m3DebugOp (Copy_جایگاه_64),      d_m3DebugOp (Preserve_Copy_جایگاه_64), d_m3DebugOp (If_r),          d_m3DebugOp (BranchIfPrologue_r),

    d_m3DebugOp (Select_i32_rss),   d_m3DebugOp (Select_i32_srs),   d_m3DebugOp (Select_i32_ssr),   d_m3DebugOp (Select_i32_sss),
    d_m3DebugOp (Select_i64_rss),   d_m3DebugOp (Select_i64_srs),   d_m3DebugOp (Select_i64_ssr),   d_m3DebugOp (Select_i64_sss),

# إذا d_m3HasFloat
    d_m3DebugOp (Select_f32_sss),   d_m3DebugOp (Select_f32_srs),   d_m3DebugOp (Select_f32_ssr),
    d_m3DebugOp (Select_f32_rss),   d_m3DebugOp (Select_f32_rrs),   d_m3DebugOp (Select_f32_rsr),

    d_m3DebugOp (Select_f64_sss),   d_m3DebugOp (Select_f64_srs),   d_m3DebugOp (Select_f64_ssr),
    d_m3DebugOp (Select_f64_rss),   d_m3DebugOp (Select_f64_rrs),   d_m3DebugOp (Select_f64_rsr),
# endif

    d_m3DebugOp (MemFill),          d_m3DebugOp (MemCopy),          d_m3DebugOp (MemInit),          d_m3DebugOp (داده_Drop),

# إذا d_m3HasGasMetering
    d_m3DebugOp (UseGas),
# endif

# إذا d_m3HasRefTypes
    d_m3DebugOp (جدول_Get),         d_m3DebugOp (جدول_مجموعه),         d_m3DebugOp (جدول_اندازه),
    d_m3DebugOp (جدول_Grow),        d_m3DebugOp (جدول_Fill),        d_m3DebugOp (جدول_Init),
    d_m3DebugOp (ElemDrop),         d_m3DebugOp (جدول_Copy),
# endif

    d_m3DebugTypedOp (مجموعه_Global),   d_m3DebugOp (مجموعه_Global_s32),    d_m3DebugOp (مجموعه_Global_s64),

    d_m3DebugTypedOp (مجموعه_ثبات), d_m3DebugTypedOp (مجموعه_جایگاه),     d_m3DebugTypedOp (Preserve_مجموعه_جایگاه),
# endif

# ifdef DEBUG
    M3OP( "termination", 0, c_m3Type_unknown, d_خالی_Op_List ) // for find_operation_info
# endif
};

ثابت M3OpInfo c_operationsFC[] =
{
    M3OP_F( "i32.trunc_s:sat/f32", 0, i_32,   d_cc(Compile_Convert),         d_convertOpList (i32_TruncSat_f32) ),  // 0x00
    M3OP_F( "i32.trunc_u:sat/f32", 0, i_32,   d_cc(Compile_Convert),         d_convertOpList (u32_TruncSat_f32) ),  // 0x01
    M3OP_F( "i32.trunc_s:sat/f64", 0, i_32,   d_cc(Compile_Convert),         d_convertOpList (i32_TruncSat_f64) ),  // 0x02
    M3OP_F( "i32.trunc_u:sat/f64", 0, i_32,   d_cc(Compile_Convert),         d_convertOpList (u32_TruncSat_f64) ),  // 0x03
    M3OP_F( "i64.trunc_s:sat/f32", 0, i_64,   d_cc(Compile_Convert),         d_convertOpList (i64_TruncSat_f32) ),  // 0x04
    M3OP_F( "i64.trunc_u:sat/f32", 0, i_64,   d_cc(Compile_Convert),         d_convertOpList (u64_TruncSat_f32) ),  // 0x05
    M3OP_F( "i64.trunc_s:sat/f64", 0, i_64,   d_cc(Compile_Convert),         d_convertOpList (i64_TruncSat_f64) ),  // 0x06
    M3OP_F( "i64.trunc_u:sat/f64", 0, i_64,   d_cc(Compile_Convert),         d_convertOpList (u64_TruncSat_f64) ),  // 0x07

    M3OP( "memory.init",           0, none,   d_cc(Compile_حافظه_Init),     d_خالی_Op_List ),                       // 0x08
    M3OP( "data.drop",             0, none,   d_cc(Compile_داده_Drop),       d_خالی_Op_List ),                       // 0x09

    M3OP( "memory.copy",           0, none,   d_cc(Compile_حافظه_Copy_Fill), d_خالی_Op_List ),                       // 0x0a
    M3OP( "memory.fill",           0, none,   d_cc(Compile_حافظه_Copy_Fill), d_خالی_Op_List ),                       // 0x0b

#إذا d_m3HasRefTypes
    M3OP( "table.init",            0, none,   d_cc(Compile_جدول_Init),      d_خالی_Op_List ),                       // 0x0c
    M3OP( "elem.drop",             0, none,   d_cc(Compile_Elem_Drop),       d_خالی_Op_List ),                       // 0x0d
    M3OP( "table.copy",            0, none,   d_cc(Compile_جدول_Copy),      d_خالی_Op_List ),                       // 0x0e
#وإلا
    M3OP_RESERVED, M3OP_RESERVED, M3OP_RESERVED,                                                                    // 0x0c...0x0e
#پایان_اگر

#إذا d_m3HasRefTypes
    M3OP( "table.grow",            0, i_32,   d_cc(Compile_جدول_Grow_Fill),  d_خالی_Op_List ),                       // 0x0f
    M3OP( "table.size",            1, i_32,   d_cc(Compile_جدول_اندازه),      d_خالی_Op_List ),                       // 0x10
    M3OP( "table.fill",            0, none,   d_cc(Compile_جدول_Grow_Fill),  d_خالی_Op_List ),                       // 0x11
#وإلا
    M3OP_RESERVED, M3OP_RESERVED, M3OP_RESERVED,                                                                    // 0x0f...0x11
#پایان_اگر

#إذا d_m3HasWideArithmetic
    M3OP_RESERVED,                                                                                                  // 0x12

    M3OP( "i64.add128",            -2, i_64,  d_cc(Compile_WideArithmetic), d_logOp (i64_Add128) ),              // 0x13
    M3OP( "i64.sub128",            -2, i_64,  d_cc(Compile_WideArithmetic), d_logOp (i64_Sub128) ),              // 0x14
    M3OP( "i64.mul_wide_s",        0,  i_64,  d_cc(Compile_WideArithmetic), d_logOp (i64_MultiplyWide) ),        // 0x15
    M3OP( "i64.mul_wide_u",        0,  i_64,  d_cc(Compile_WideArithmetic), d_logOp (u64_MultiplyWide) ),        // 0x16
#پایان_اگر


# ifdef DEBUG
    M3OP( "termination", 0, c_m3Type_unknown, d_خالی_Op_List ) // for find_operation_info
# endif
};

#إذا d_m3HasAtomics

// The 0xFE instructions share one entry per kind rather than each having its own: what
// tells them apart, the width of the cell and the operation, is worked out from the
// sub-opcode by Compile_Atomic. GetAtomicInfoIndex () maps a sub-opcode onto this table.
تعداد {
    c_opAtomic_notify,
    c_opAtomic_wait32,
    c_opAtomic_wait64,
    c_opAtomic_fence,
    c_op_Atomic_بارگیری,
    c_opAtomic_store,
    c_opAtomic_rmw,
    c_opAtomic_cmpxchg
};

ثابت M3OpInfo c_operationsFE[] =
{
    M3OP( "memory.atomic.notify",  -1, i_32,  d_cc(Compile_Atomic), d_logOp (AtomicNotify) ),    // c_opAtomic_notify
    M3OP( "memory.atomic.wait32",  -2, i_32,  d_cc(Compile_Atomic), d_logOp (AtomicWait32) ),    // c_opAtomic_wait32
    M3OP( "memory.atomic.wait64",  -2, i_32,  d_cc(Compile_Atomic), d_logOp (AtomicWait64) ),    // c_opAtomic_wait64
    M3OP( "atomic.fence",           0, none,  d_cc(Compile_Atomic), d_خالی_Op_List ),             // c_opAtomic_fence
    M3OP( "atomic.load",            0, any,   d_cc(Compile_Atomic), d_logOp (Atomic_بارگیری) ),      // c_opAtomic_load
    M3OP( "atomic.store",          -2, none,  d_cc(Compile_Atomic), d_logOp (AtomicStore) ),     // c_opAtomic_store
    M3OP( "atomic.rmw",            -1, any,   d_cc(Compile_Atomic), d_logOp (AtomicRmw) ),       // c_opAtomic_rmw
    M3OP( "atomic.rmw.cmpxchg",    -2, any,   d_cc(Compile_Atomic), d_logOp (AtomicCmpxchg) ),   // c_opAtomic_cmpxchg
};

ثابت u32 c_رقم_Operations_FE = M3_شمار_از(c_operationsFE);

// The entry a 0xFE sub-opcode uses, or -1 for one that names nothing: the spec leaves
// 0x04..0x0f and everything past cmpxchg unassigned. The accesses come in runs of seven,
// one per pairing of value type and cell width.
ساكن مضمن
i32 Get_Atomic_Info_شاخص (u32 i_sub)
{
    إذا (i_sub <= 0x03) أرجع (i32)i_sub;
    إذا (i_sub < 0x10) أرجع -1;
    إذا (i_sub <= 0x16) أرجع c_op_Atomic_بارگیری;
    إذا (i_sub <= 0x1d) أرجع c_opAtomic_store;
    إذا (i_sub <= 0x47) أرجع c_opAtomic_rmw;
    إذا (i_sub <= c_wa_Op_آخرین_Atomic) أرجع c_opAtomic_cmpxchg;
    أرجع -1;
}

#پایان_اگر // d_m3HasAtomics

// The opcodes above c_waOp_lastCore are sparse: the reference instructions at
// 0xd0..0xd2 and 0xd4, and the 0xfc extended-opcode prefix. Indexing c_operations out to
// 0xfc to reach them would leave ~50 empty entries there, so they get their own
// table, which GetHighOpIndex () maps the opcode onto.
تعداد {
#إذا d_m3HasRefTypes
    c_op_بالا_ref_Null,
    c_op_بالا_ref_Is_Null,
    c_op_بالا_ref_Func,
#  إذا d_m3HasTypedRefs
    c_op_بالا_ref_As_Non_Null,
#  endif
#پایان_اگر
#إذا d_m3_Has_پشته_Switching
    c_op_بالا_cont_New,
    c_op_بالا_cont_Bind,
    c_op_بالا_suspend,
    c_op_بالا_resume,
    c_op_بالا_resume_Throw,
    c_op_بالا_resume_Throw_Ref,
    c_op_بالا_switch,
#پایان_اگر
#إذا d_m3HasAtomics
    c_op_بالا_atomic,
#پایان_اگر
    c_op_بالا_extended   // always present, so the enum is never empty
};

// Positional, so the table must stay in c_opHigh_* order: array designators
// are not valid C++, which the maintenance builds compile this as.
ساكن ثابت M3OpInfo c_operations_بالا[] =
{
#إذا d_m3HasRefTypes
    M3OP( "ref.null",        1, any,               d_cc(Compile_Ref_Null),       d_خالی_Op_List ),  // 0xd0  c_opHigh_refNull
    M3OP( "ref.is_null",     0, i_32,              d_cc(Compile_Ref_IsNull),     d_خالی_Op_List ),  // 0xd1  c_opHigh_refIsNull
    M3OP( "ref.func",        1, any,               d_cc(Compile_Ref_Func),       d_خالی_Op_List ),  // 0xd2  c_opHigh_refFunc
#  إذا d_m3HasTypedRefs
    M3OP( "ref.as_non_null", 0, any,               d_cc(Compile_Ref_AsNonNull),  d_خالی_Op_List ),  // 0xd4  c_opHigh_refAsNonNull
#  endif
#پایان_اگر
#إذا d_m3_Has_پشته_Switching
    M3OP( "cont.new",          1, any,             d_cc(Compile_ContNew),        d_خالی_Op_List ),  // 0xe0  c_opHigh_contNew
    M3OP( "cont.bind",         2, any,             d_cc(Compile_ContBind),       d_خالی_Op_List ),  // 0xe1  c_opHigh_contBind
    M3OP( "suspend",           1, any,             d_cc(Compile_Suspend),        d_خالی_Op_List ),  // 0xe2  c_opHigh_suspend
    M3OP( "resume",            1, any,             d_cc(Compile_Resume),         d_خالی_Op_List ),  // 0xe3  c_opHigh_resume
    M3OP( "resume_throw",      2, any,             d_cc(Compile_Resume)   ,    d_خالی_Op_List ),  // 0xe4  c_opHigh_resumeThrow
    M3OP( "resume_throw_ref",  1, any,             d_cc(Compile_Resume)      , d_خالی_Op_List ),  // 0xe5  c_opHigh_resumeThrowRef
    M3OP( "switch",            2, any,             d_cc(Compile_Switch),         d_خالی_Op_List ),  // 0xe6  c_opHigh_switch
#پایان_اگر

#إذا d_m3HasAtomics
    M3OP( "0xFE",            0, c_m3Type_unknown,  d_cc(Compile_ExtendedOpcode), d_خالی_Op_List ),  // 0xfe  c_opHigh_atomic
#پایان_اگر
    M3OP( "0xFC",            0, c_m3Type_unknown,  d_cc(Compile_ExtendedOpcode), d_خالی_Op_List ),  // 0xfc  c_opHigh_extended
};
M3_STATIC_ASSERT (M3_شمار_از (c_operations_بالا) == c_op_بالا_extended + 1, c_operations_بالا_needs_one_مدخل_در_هر_c_op_بالا);

// clang-format on


// Opcodes the spec reserves leave zeroed holes in the tables above: no compiler
// and no operations. Every implemented op has at least one of the two.
ساكن مضمن
bool IsImplementedOp (IM3OpInfo i_info)
{
    أرجع (i_info->compiler != c_cc_NULL or i_info->operations[0] != عدم);
}

ثابت u32 c_رقم_Operations   = M3_شمار_از(c_operations);
ثابت u32 c_رقم_Operations_FC = M3_شمار_از(c_operationsFC);

// Maps one of the sparse opcodes above c_waOp_lastCore onto c_operationsHigh,
// or -1 for anything else in that range.
ساكن مضمن
i32 Get_بالا_Op_شاخص (m3opcode_t opcode)
{
    بدل (opcode) {
#إذا d_m3HasRefTypes
    حالة c_waOp_refNull: أرجع c_op_بالا_ref_Null;
    حالة c_waOp_refIsNull: أرجع c_op_بالا_ref_Is_Null;
    حالة c_waOp_refFunc: أرجع c_op_بالا_ref_Func;
#  إذا d_m3HasTypedRefs
    حالة c_waOp_refAsNonNull: أرجع c_op_بالا_ref_As_Non_Null;
#  endif
#پایان_اگر
#إذا d_m3_Has_پشته_Switching
    حالة c_waOp_contNew: أرجع c_op_بالا_cont_New;
    حالة c_waOp_contBind: أرجع c_op_بالا_cont_Bind;
    حالة c_waOp_suspend: أرجع c_op_بالا_suspend;
    حالة c_waOp_resume: أرجع c_op_بالا_resume;
    حالة c_waOp_resumeThrow: أرجع c_op_بالا_resume_Throw;
    حالة c_waOp_resumeThrowRef: أرجع c_op_بالا_resume_Throw_Ref;
    حالة c_waOp_switch: أرجع c_op_بالا_switch;
#پایان_اگر
    حالة c_waOp_extended: أرجع c_op_بالا_extended;
#إذا d_m3HasAtomics
    حالة c_waOp_atomic: أرجع c_op_بالا_atomic;
#پایان_اگر
    افتراضي: أرجع -1;
    }
}

IM3OpInfo GetOpInfo (m3opcode_t opcode)
{
    IM3OpInfo info = عدم;

    بدل (opcode >> 8) {
    حالة 0x00:
        // c_operations is indexed by opcode only up to c_waOp_lastCore; past that
        // it holds internal operations (DEBUG builds), unreachable by opcode.
        إذا (M3_LIKELY(opcode <= c_wa_Op_آخرین_Core)) {
            info = &c_operations[opcode];
        } وإلا {
            i32 بالا = Get_بالا_Op_شاخص(opcode);
            إذا (بالا >= 0) {
                info = &c_operations_بالا[بالا];
            }
        }
        اكسر;
    حالة c_waOp_extended:
        opcode &= 0xFF;
        إذا (M3_LIKELY(opcode <= c_wa_Op_آخرین_Extended)) {
            info = &c_operationsFC[opcode];
        }
        اكسر;
#إذا d_m3HasAtomics
    حالة c_waOp_atomic: {
        i32 index = Get_Atomic_Info_شاخص(opcode & 0xFF);
        إذا (index >= 0) {
            info = &c_operationsFE[index];
        }
    } اكسر;
#پایان_اگر
    }

    أرجع (info and IsImplementedOp(info)) ? info : عدم;
}

#إذا d_m3HasGasMetering

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
تعداد {
    c_gasLocal   = 1,          // charged once per declared local, at function entry
    c_gasNominal = 1,          // nop, block, loop, if, const
    c_gasArith   = 45,
    c_gas_لغزش   = 67,
    c_gasBranch  = 90,
    c_gas_Mem_اندازه = 100,
    c_gasAccess  = 120,        // a local, global, table or memory access
    c_gasHeavy   = 10000,      // an indirect call, or anything that works over a whole region
    c_gasDivide  = 36000
};

// The most one op_UseGas can carry. A segment that reaches it is split into
// consecutive charges, which prepays exactly the same amount.
#  define d_m3MaxSegmentGas   0xFFFFFFFF

ساكن
u32 Get_Gas_هزینه (m3opcode_t i_opcode)
{
    بدل (i_opcode) {
    حالة 0x00:                        // unreachable
    حالة 0x01:                        // nop
    حالة c_wa_Op_بلوک:
    حالة c_waOp_loop:
    حالة c_waOp_if:
    حالة c_wa_Op_try_جدول:             // a block that also installs handlers
    حالة c_waOp_refNull:              // a constant, by another name
    حالة c_waOp_refFunc:
        أرجع c_gasNominal;

    حالة c_wa_Op_انتها:
        أرجع 0;                     // the one control instruction the table prices at nothing

    حالة c_waOp_else:
    حالة c_waOp_branch:
    حالة c_waOp_branchIf:
    حالة c_waOp_return:
    حالة c_waOp_call:
    حالة c_waOp_returnCall:
    حالة c_waOp_throw:
    حالة c_waOp_throwRef:
        أرجع c_gasBranch;

    حالة c_wa_Op_branch_جدول:
    حالة 0x1a:                        // drop
    حالة 0x1b:                        // select
    حالة c_waOp_selectTyped:
        أرجع c_gasAccess;

    حالة c_wa_Op_call_غیرمستقیم:
    حالة c_wa_Op_return_Call_غیرمستقیم:
    حالة c_waOp_callRef:
    حالة c_waOp_returnCallRef:
    حالة c_wa_Op_حافظه_Init:
    حالة c_wa_Op_حافظه_Copy:
    حالة c_wa_Op_حافظه_Fill:
    حالة c_wa_Op_جدول_Init:
    حالة c_wa_Op_جدول_Copy:
    حالة c_wa_Op_جدول_Grow:
    حالة c_wa_Op_جدول_Fill:
    حالة 0x40:                        // memory.grow
        أرجع c_gasHeavy;

    حالة c_wa_Op_داده_Drop:
    حالة c_waOp_elemDrop:
    حالة c_wa_Op_جدول_اندازه:
    حالة 0x3f:                        // memory.size
        أرجع c_gas_Mem_اندازه;

    حالة c_waOp_refIsNull:
    حالة c_waOp_refAsNonNull:
        أرجع c_gasArith;

    افتراضي:
        اكسر;
    }

    // The two integer arithmetic runs are priced by position, i32 at 0x6a and
    // i64 at 0x7c, the table giving both the same shape
    إذا ((i_opcode >= 0x6a and i_opcode <= 0x78) or (i_opcode >= 0x7c and i_opcode <= 0x8a)) {
        // clang-format off
        ساكن ثابت u16 c_arithmeticCosts [] = {
            c_gasArith,  c_gasArith,  c_gasArith,                   // add, sub, mul
            c_gasDivide, c_gasDivide, c_gasDivide, c_gasDivide,     // div_s, div_u, rem_s, rem_u
            c_gasArith,  c_gasArith,  c_gasArith,                   // and, or, xor
            c_gas_لغزش,  c_gas_لغزش,  c_gas_لغزش,                   // shl, shr_s, shr_u
            c_gasBranch, c_gasBranch                                // rotl, rotr
        };
        // clang-format on

        أرجع c_arithmeticCosts[i_opcode - (i_opcode <= 0x78 ? 0x6a : 0x7c)];
    }

    // the atomic accesses are memory accesses; what waits, wakes or orders is worth
    // as much as an operation over a whole region
    إذا (i_opcode >= (c_waOp_atomic << 8) and i_opcode <= ((c_waOp_atomic << 8) | c_wa_Op_آخرین_Atomic)) {
        أرجع (i_opcode <= c_waOp_atomicFence) ? c_gasHeavy : c_gasAccess;
    }
    إذا (i_opcode >= 0x20 and i_opcode <= 0x3e) {
        أرجع c_gasAccess;           // locals, globals, table accesses, loads and stores
    }
    إذا (i_opcode >= 0x41 and i_opcode <= 0x44) {
        أرجع c_gasNominal;          // i32/i64/f32/f64.const
    }
    إذا (i_opcode >= 0x8b and i_opcode <= 0xa6) {
        // the float run, priced as integer arithmetic except where it divides
        bool divides = (i_opcode == 0x91 or i_opcode == 0x95 or     // f32.sqrt, f32.div
                        i_opcode == 0x9f or i_opcode == 0xa3);      // f64.sqrt, f64.div

        أرجع divides ? c_gasDivide : c_gasArith;
    }

    // comparisons and clz/ctz/popcnt (0x45..0x69, 0x79..0x7b), the conversions
    // and sign extensions (0xa7..0xc4), the saturating truncations, the wide
    // arithmetic, and anything else that reaches here: plain arithmetic
    أرجع c_gasArith;
}

// The instructions ewasm's design segments a body at. A segment runs up to and
// including the next one of these, so every cycle a body can execute - a loop's
// back edge, a branch out of a block, falling off the end of a function - passes
// through at least one, and no path can run unmetered however it is entered.
ساكن
bool Is_Gas_Segment_انتها (m3opcode_t i_opcode)
{
    بدل (i_opcode) {
    حالة c_waOp_loop:
    حالة c_waOp_if:
    حالة c_waOp_else:
    حالة c_wa_Op_انتها:
    حالة c_waOp_branch:
    حالة c_waOp_branchIf:
    حالة c_wa_Op_branch_جدول:
    حالة c_waOp_return:
    حالة c_waOp_returnCall:
    حالة c_wa_Op_return_Call_غیرمستقیم:
    حالة c_waOp_returnCallRef:
        أرجع true;
    افتراضي:
        أرجع false;
    }
}

// Ends the open segment by writing what it accumulated into the immediate its
// op_UseGas is still waiting on. Emitting onto a later code page does not move
// an earlier one, so the reservation is still where it was left.
ساكن
فراغ CloseGasSegment (IM3Compilation o)
{
    إذا (o->isGasSegmentOpen) {
        إذا (o->gasPatch) {
            u32 هزینه = o->gas_هزینه;
            کپی_حافظه(o->gasPatch, &هزینه, حجم(هزینه));

            o->gasPatch = عدم;
        }
        o->gas_هزینه          = 0;
        o->isGasSegmentOpen = false;
    }
}

// Whether this compilation emits the charges
ساكن مضمن
bool IsMetering (IM3Compilation o)
{
    أرجع o->runtime->gas_حد != 0;
}

// Charges one instruction to the segment being compiled, opening a segment at
// this point in the code stream if none is open, and settling the segment when
// the instruction is one that ends it. Called before the instruction is
// compiled, which is what both halves need: the op_UseGas lands ahead of what it
// pays for, and the cost is final before a block-structured instruction - loop,
// if, else - compiles its body from inside its own compiler. The body then opens
// a segment of its own, inside the loop or the arm, where it is charged on every
// pass rather than once on the way in.
ساكن
M3_نتیجه MeterOpcode (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_نتیجه نتیجه = m3Err_none;

    // constant expressions are not metered: they run once at instantiation,
    // before the module is anything a gas budget was handed out for
    إذا (o->function and o->صفحه and IsMetering(o)) {
        // only the 0xFC or 0xFE prefix has been read so far; the instruction it names is
        // the LEB128 u32 the compiler is about to take. Peek it without consuming.
        // Anything the tables do not cover leaves the prefix priced on its own,
        // which is moot - the compile fails on it a moment later.
        إذا (i_opcode == c_waOp_extended or i_opcode == c_waOp_atomic) {
            بایتها_t peek = o->wasm;
            u32     sub;
            u32     آخرین = (i_opcode == c_waOp_atomic) ? c_wa_Op_آخرین_Atomic : c_wa_Op_آخرین_Extended;

            إذا (not ReadLEB_u32(&sub, &peek, o->wasm_انتها) and sub <= آخرین) {
                i_opcode = (m3opcode_t)((i_opcode << 8) | sub);
            }
        }

        u32 هزینه = Get_Gas_هزینه(i_opcode);

        إذا (o->isGasSegmentOpen and o->gas_هزینه > d_m3MaxSegmentGas - هزینه) {
            CloseGasSegment(o);
        }

        إذا (not o->isGasSegmentOpen) {
            o->isGasSegmentOpen = true;

_           (EmitOp(o, op_UseGas));
            o->gasPatch = (فراغ*)Get_صفحه_PC(o->صفحه);
            EmitConstant32(o, 0);
        }

        o->gas_هزینه += هزینه;

        إذا (Is_Gas_Segment_انتها(i_opcode)) {
            CloseGasSegment(o);
        }
    }

    _catch: أرجع نتیجه;
}

#پایان_اگر // d_m3HasGasMetering


M3_نتیجه Compile_بلوک_Statements (IM3Compilation o)
{
    M3_نتیجه نتیجه   = m3Err_none;
    bool     valid_انتها = false;

    طالما (o->wasm < o->wasm_انتها) {
#إذا d_m3EnableOpTracing
        إذا (o->رقم_Emits) {
            EmitOp(o, op_Dump_پشته);
            EmitConstant32(o, o->رقم_Opcodes);
            EmitConstant32(o, Get_Max_استفاده_شده_جایگاه_Plus_One(o));
            Emit_اشاره_گر(o, o->function);

            o->رقم_Emits = 0;
        }
#پایان_اگر

        m3opcode_t opcode;
        o->آخرین_Opcode_شروع = o->wasm;
_       (Read_opcode(&opcode, &o->wasm, o->wasm_انتها));
        log_opcode(o, opcode);

        // Restrict opcodes when evaluating expressions
        إذا (M3_UNLIKELY(not o->function)) {
            // clang-format off
            بدل (opcode) {
            حالة c_waOp_i32_const: حالة c_waOp_i64_const:
            حالة c_waOp_f32_const: حالة c_waOp_f64_const:
            حالة c_waOp_getGlobal: حالة c_wa_Op_انتها:
#إذا d_m3HasRefTypes
            حالة c_waOp_refNull:   حالة c_waOp_refFunc:
#پایان_اگر
#إذا d_m3HasExtendedConst
            حالة c_wa_Op_i32_جمع_کن:   حالة c_waOp_i32_sub:   حالة c_waOp_i32_mul:
            حالة c_wa_Op_i64_جمع_کن:   حالة c_waOp_i64_sub:   حالة c_waOp_i64_mul:
#پایان_اگر
                اكسر;
            افتراضي:
                _throw(m3Err_restrictedOpcode);
            }
            // clang-format on
        }

        IM3OpInfo opinfo = GetOpInfo(opcode);

        إذا (M3_UNLIKELY(opinfo == عدم)) {
            _throw(ErrorCompile(m3Err_unknownOpcode, o, "opcode '%x' not available", opcode));
        }

#إذا d_m3HasGasMetering
_       (MeterOpcode(o, opcode));
#پایان_اگر

        إذا (opinfo->compiler != c_cc_NULL) {
_           ((*GetCompiler(opinfo))(o, opcode))
        } وإلا {
_           (Compile_Operator(o, opcode));
        }

        o->قبلی_Opcode = opcode;

        إذا (opcode == c_waOp_else) {
            _throwif(m3Err_wasmMalformed, o->بلوک.opcode != c_waOp_if);
            valid_انتها = true;
            اكسر;
        } وإلا إذا (opcode == c_wa_Op_انتها) {
            valid_انتها = true;
            اكسر;
        }
    }
    _throwif(m3Err_wasmMalformed, not valid_انتها);

_catch:
    أرجع نتیجه;
}

ساكن
M3_نتیجه افزودن_بلوک_Results (IM3Compilation o)
{
    M3_نتیجه نتیجه = m3Err_none;

    u16 رقم_Results = Get_Func_Type_رقم_Results(o->بلوک.type);

    لكل (u16 i = 0; i < رقم_Results; ++i) {
        m3type_t type = Get_Func_Type_نتیجه_Type(o->بلوک.type, i);

        إذا (i == رقم_Results - 1 and IsFpType(type)) {
_           (افزودن_ثبات(o, type));
        } وإلا {
_           (افزودن_Allocated_جایگاه(o, type));
        }
    }

    _catch: أرجع نتیجه;
}


M3_نتیجه Compile_بلوک (IM3Compilation o, IM3FuncType i_بلوک_Type, m3opcode_t i_بلوک_Opcode)
{
                                                                                        d_m3Assert (not Is_ثبات_Allocated (o, 0));
                                                                                        d_m3Assert (not Is_ثبات_Allocated (o, 1));
    InvalidateFold(o);

    M3CompilationScope  outerScope = o->بلوک;
    M3CompilationScope* بلوک      = &o->بلوک;

    بلوک->outer   = &outerScope;
    بلوک->pc      = Get_صفحه_PC(o->صفحه);
    بلوک->patches = عدم;
    بلوک->type    = i_بلوک_Type;
    بلوک->opcode  = i_بلوک_Opcode;
    بلوک->عمق++;
    بلوک->snapshot_بلوک = 0;

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
#إذا d_m3HasSnapshots
    // op_Loop's frame records the body, which is also the loop's identity;
    // op_TryTable's records its clause table, two words a clause back from it
    إذا (i_بلوک_Opcode == c_waOp_loop) {
_       (رکورد_بلوک_شروع(o, i_بلوک_Opcode, بلوک->pc, 0));
    }
#  إذا d_m3HasExceptionHandling
    وإلا إذا (i_بلوک_Opcode == c_wa_Op_try_جدول) {
_       (رکورد_بلوک_شروع(o, i_بلوک_Opcode, بلوک->pc - 2 * o->رقم_Try_Clauses, o->رقم_Try_Clauses));
    }
#  endif
#پایان_اگر

    // validate and dealloc params ----------------------------

    u16 پشته_شاخص = o->پشته_شاخص;

    u16 رقم_Params = Get_Func_Type_رقم_Params(i_بلوک_Type);

    إذا (i_بلوک_Opcode != c_waOp_else) {
        لكل (u16 i = 0; i < رقم_Params; ++i) {
            m3type_t type = GetFuncTypeParamType(i_بلوک_Type, رقم_Params - 1 - i);
_           (برداشت_Type(o, type));
        }
    } وإلا {
        إذا (Is_پشته_Polymorphic(o) && o->بلوک.بلوک_پشته_شاخص + رقم_Params > o->پشته_شاخص) {
            o->پشته_شاخص = o->بلوک.بلوک_پشته_شاخص;
        } وإلا {
            o->پشته_شاخص -= رقم_Params;
        }
    }

    u16 param_شاخص        = o->پشته_شاخص;
    بلوک->exit_پشته_شاخص = param_شاخص; // consume the params at block exit

    // keep copies of param slots in the stack
    o->پشته_شاخص = پشته_شاخص;

    // find slots for the results ----------------------------
_   (افزودن_بلوک_Results(o));

    پشته_شاخص = o->پشته_شاخص;

    // dealloc but keep record of the result slots in the stack
    u16 رقم_Results = Get_Func_Type_رقم_Results(i_بلوک_Type);
    طالما (رقم_Results--) {
        برداشت(o);
    }

    بلوک->بلوک_پشته_شاخص = o->پشته_شاخص = پشته_شاخص;

    // push the params back onto the stack -------------------
    لكل (u16 i = 0; i < رقم_Params; ++i) {
        m3type_t type = GetFuncTypeParamType(i_بلوک_Type, i);

        u16 جایگاه = Get_جایگاه_For_پشته_شاخص(o, param_شاخص + i);
        افزودن(o, type, جایگاه);

        إذا (جایگاه >= o->جایگاه_اولی_Dynamic_شاخص && جایگاه != c_جایگاه_استفادهنشده) {
_           (MarkSlotsAllocatedByType(o, جایگاه, type));
        }
    }

    //--------------------------------------------------------

#إذا d_m3HasExceptionHandling
    // with the scope pushed and the params back on, a catch label of 0 names
    // this block - which is what the proposal says it should
    إذا (i_بلوک_Opcode == c_wa_Op_try_جدول) {
_       (EmitCatchStubs(o));
    }
#پایان_اگر

_   (Compile_بلوک_Statements(o));

_   (Validate_بلوک_انتها(o));

    إذا (o->function)    // skip for expressions
    {
        إذا (not Is_پشته_Polymorphic(o)) {
_           (Resolve_بلوک_Results(o, &o->بلوک, /* isBranch: */ false));
        }

#إذا d_m3HasExceptionHandling
        // Falling out of the end of a try region retires its handler. This sits
        // ahead of PatchBranches so branches into the block's exit land past it:
        // a branch out of the try popped its own handler at the branch site.
        إذا (i_بلوک_Opcode == c_wa_Op_try_جدول) {
_           (EmitOp(o, op_برداشت_Handlers));
            EmitConstant32(o, 1);
        }
#پایان_اگر

_       (Unwind_بلوک_پشته(o))

        إذا (not ((i_بلوک_Opcode == c_waOp_if and رقم_Results) or o->قبلی_Opcode == c_waOp_else)) {
            o->پشته_شاخص = o->بلوک.exit_پشته_شاخص;
_           (افزودن_بلوک_Results(o));
        }
    }

    PatchBranches(o);

#إذا d_m3HasSnapshots
    رکورد_بلوک_انتها(o);

    إذا (o->local_Init_عمق) {
        لكل (u32 i = 0; i < o->snapshotMap->رقم_Locals; ++i) {
            إذا (o->local_Init_عمق[i] >= (u32)o->بلوک.عمق) {
                o->local_Init_عمق[i] = UINT32_MAX;
            }
        }
    }
#پایان_اگر
    o->بلوک = outerScope;

} _catch: أرجع نتیجه;
}

ساكن
M3_نتیجه CompileLocals (IM3Compilation o)
{
    M3_نتیجه نتیجه;

    u32 رقم_Locals = 0;
    u32 رقم_Local_بلوکها;
_   (ReadLEB_u32(&رقم_Local_بلوکها, &o->wasm, o->wasm_انتها));

    لكل (u32 l = 0; l < رقم_Local_بلوکها; ++l) {
        u32      var_شمار;
        m3type_t localType;

_       (ReadLEB_u32(&var_شمار, &o->wasm, o->wasm_انتها));
_       (Parse_مقدار_Type(o->module, &localType, &o->wasm, o->wasm_انتها));
        رقم_Locals += var_شمار;                                                          m3log (compile, "pushing locals. count: %d; type: %s", var_شمار, c_waTypes [پایه_Type_از(localType)]);
        طالما (var_شمار--) {
_           (افزودن_Allocated_جایگاه(o, localType));
        }
    }

    إذا (o->function) {
        o->function->رقم_Locals = رقم_Locals;
    }

    _catch: أرجع نتیجه;
}

ساكن
M3_نتیجه ReserveConstants (IM3Compilation o)
{
    M3_نتیجه نتیجه = m3Err_none;

    // in the interest of speed, this blindly scans the Wasm code looking for any byte
    // that looks like an const opcode.
    u16 رقم_Constant_Slots = 0;

    بایتها_t wa = o->wasm;
    طالما (wa < o->wasm_انتها) {
        u8  کد     = *wa++;
        u16 جمع_کن_Slots = 0;

        إذا (کد == c_waOp_i32_const or کد == c_waOp_f32_const) {
            جمع_کن_Slots = 1;
        } وإلا إذا (کد == c_waOp_i64_const or کد == c_waOp_f64_const) {
            جمع_کن_Slots = Get_Type_رقم_Slots(c_m3Type_i64);
        }

        إذا (رقم_Constant_Slots + جمع_کن_Slots >= d_m3_Max_Constant_جدول_اندازه) {
            اكسر;
        }

        رقم_Constant_Slots += جمع_کن_Slots;
    }

    // if constants overflow their reserved stack space, the compiler simply emits op_Const
    // operations as needed. Compiled expressions (global inits) don't pass through this
    // ReserveConstants function and thus always produce inline constants.

    Align_جایگاه_مقصد_Type(&رقم_Constant_Slots, c_m3Type_i64);                                           m3log (compile, "reserved constant slots: %d", رقم_Constant_Slots);

    o->جایگاه_اولی_Dynamic_شاخص = o->جایگاه_اولی_Const_شاخص + رقم_Constant_Slots;

    إذا (o->جایگاه_اولی_Dynamic_شاخص >= d_m3MaxFunctionSlots) {
        _throw(m3_Err_function_پشته_سرریز);
    }

    _catch:
    أرجع نتیجه;
}


// A constant expression compiles like a miniature function root block: no args,
// no locals and a single result, which Compile_End copies down into slot 0 for
// EvaluateExpression to read back.
M3_نتیجه Compile_عبارت (IM3Compilation o, IM3FuncType i_نتیجه_Type)
{
    M3_نتیجه نتیجه = m3Err_none;

    o->بلوک.type = i_نتیجه_Type;

    u16 رقم_Ret_Slots = Get_Func_Type_رقم_Results(i_نتیجه_Type) * c_io_جایگاه_شمار;

    لكل (u16 i = 0; i < رقم_Ret_Slots; ++i) {
        Mark_جایگاه_Allocated(o, i);
    }

    o->max_پشته_Slots = o->جایگاه_Max_Allocated_شاخص_Plus_One = o->جایگاه_اولی_Dynamic_شاخص = رقم_Ret_Slots;

_   (Compile_بلوک_Statements(o));

    _catch: أرجع نتیجه;
}


M3_نتیجه CompileFunction (IM3Function io_function)
{
    إذا (!io_function->wasm) {
        // An import. Either linking pointed it at another module's function -
        // in which case that one carries the body - or a host function was
        // bound to it, or it is simply unsatisfied.
        IM3Function impl = Function_Implementation(io_function);

        إذا (impl != io_function) {
            إذا (not impl->compiled) {
                M3_نتیجه r = CompileFunction(impl);
                إذا (r) {
                    أرجع r;
                }
            }

            // keep the placeholder callable too, for anything still holding it
            io_function->compiled = impl->compiled;
            أرجع m3Err_none;
        }

        إذا (io_function->compiled) {
            أرجع m3Err_none;
        }

        أرجع ErrorModule(m3Err_functionImportMissing, io_function->module, "'%s.%s'",
                           GetFunctionImportModuleName(io_function),
                           m3_GetFunctionName(io_function));
    }

#إذا d_m3EnableValidation
    إذا (not io_function->module->runtime->skipValidation) {
        M3_نتیجه vr = ValidateFunction(io_function);
        إذا (vr) {
            أرجع vr;
        }
    }
#پایان_اگر

    IM3FuncType funcType = io_function->funcType;                   m3log (compile, "compiling: [%d] %s %s; wasm-size: %d",
                                                                        io_function->index, m3_GetFunctionName (io_function), SPrintFuncTypeSignature (funcType), (u32) (io_function->wasm_انتها - io_function->wasm));
    IM3Runtime runtime = io_function->module->runtime;

    IM3Compilation o = &runtime->compilation;                       d_m3Assert (d_m3MaxFunctionSlots >= d_m3_Max_Function_پشته_ارتفاع * (d_m3_Use32_بیت_Slots + 1))  // need twice as many slots in 32-bit mode
    memset(o, 0x0, حجم(M3Compilation));

    o->runtime    = runtime;
    o->module     = io_function->module;
    o->function   = io_function;
    o->wasm       = io_function->wasm;
    o->wasm_انتها    = io_function->wasm_انتها;
    o->بلوک.type = funcType;

_try {
    // skip over code size. the end was already calculated during parse phase
    u32 اندازه;
_   (ReadLEB_u32(&اندازه, &o->wasm, o->wasm_انتها));                     d_m3Assert (اندازه == (o->wasm_انتها - o->wasm))

#إذا d_m3HasSnapshots
    o->body_شروع = o->wasm;
#پایان_اگر

#إذا d_m3HasSnapshots
    // only code that can be suspended can end up in a snapshot
    إذا (runtime->isSuspendable) {
        o->snapshotMap = m3_AllocStruct(M3SnapshotMap);
        _throwifnull(o->snapshotMap);
    }
#پایان_اگر

_   (Acquire_Compilation_کد_صفحه(o, &o->صفحه));

    pc_t pc = Get_صفحه_PC(o->صفحه);

    u16 رقم_Ret_Slots = Get_Function_رقم_Returns(o->function) * c_io_جایگاه_شمار;

    لكل (u16 i = 0; i < رقم_Ret_Slots; ++i) {
        Mark_جایگاه_Allocated(o, i);
    }

    o->function->رقم_Ret_Slots = o->جایگاه_اولی_Dynamic_شاخص = رقم_Ret_Slots;

    u16 رقم_آرگومانها = Get_Function_رقم_آرگومانها(o->function);

    // push the arg types to the type stack
    لكل (u16 i = 0; i < رقم_آرگومانها; ++i) {
        m3type_t type = GetFunctionArgType(o->function, i);
_       (افزودن_Allocated_جایگاه(o, type));

        // prevent allocator fill-in
        o->جایگاه_اولی_Dynamic_شاخص += c_io_جایگاه_شمار;
    }

    o->جایگاه_Max_Allocated_شاخص_Plus_One = o->function->رقم_Ret_And_Arg_Slots = o->جایگاه_اولی_Local_شاخص = o->جایگاه_اولی_Dynamic_شاخص;

_   (CompileLocals(o));

#إذا d_m3HasSnapshots
_   (رکورد_Locals(o));
#پایان_اگر

#إذا d_m3HasGasMetering
    // the table charges for a function's locals as well as for its instructions.
    // op_Entry is what sets them up, so the charge rides on the body's first
    // segment, which is emitted right after it
    o->gas_هزینه = o->function->رقم_Locals * c_gasLocal;
#پایان_اگر

    u16 max_جایگاه = Get_Max_استفاده_شده_جایگاه_Plus_One(o);

    o->function->رقم_Local_بایتها = (max_جایگاه - o->جایگاه_اولی_Local_شاخص) * حجم(m3slot_t);

    o->جایگاه_اولی_Const_شاخص = o->جایگاه_Max_Const_شاخص = max_جایگاه;

    // ReserveConstants initializes o->firstDynamicSlotNumber
_   (ReserveConstants(o));

    // start tracking the max stack used (Push() also updates this value) so that op_Entry can precisely detect stack overflow
    o->max_پشته_Slots = o->جایگاه_Max_Allocated_شاخص_Plus_One = o->جایگاه_اولی_Dynamic_شاخص;

    o->بلوک.بلوک_پشته_شاخص = o->پشته_اولی_Dynamic_شاخص = o->پشته_شاخص;                           m3log (compile, "start stack index: %u; max stack slots: %u",
                                                                                                           (u32) o->پشته_اولی_Dynamic_شاخص, (u32) o->max_پشته_Slots);
#إذا d_m3_Has_پشته_Switching
    // A suspendable body opens by checking for a pause request, once its frame
    // is set up. The pause point's offset is the first instruction's, just past
    // the local declarations - the body's end, when it has no other.
    إذا (runtime->isSuspendable) {
        o->آخرین_Opcode_شروع = o->wasm;
#  إذا d_m3_مدخل_Keeps_قاب
_       (EmitOp(o, op_مدخل));
        Emit_اشاره_گر(o, io_function);
_       (EmitOp(o, op_مدخل_Check));
_       (رکورد_امن_نقطه(o, safepoint_مدخل, 1, 0, 0, عدم));
#  وإلا
_       (EmitOp(o, op_مدخل_Suspendable));
        Emit_اشاره_گر(o, io_function);
_       (رکورد_امن_نقطه(o, safepoint_مدخل, 0, 0, 0, عدم));
#  endif
    } وإلا
#پایان_اگر
    {
_       (EmitOp(o, op_مدخل));
        Emit_اشاره_گر(o, io_function);
    }

_   (Compile_بلوک_Statements(o));

    _throwif(m3Err_wasmMalformed, o->قبلی_Opcode != c_wa_Op_انتها);

    io_function->compiled      = pc;
    io_function->max_پشته_Slots = o->max_پشته_Slots;

    u16 رقم_Constant_Slots = o->جایگاه_Max_Const_شاخص - o->جایگاه_اولی_Const_شاخص;                           m3log (compile, "unique constant slots: %u; unused slots: %u",
                                                                                                           رقم_Constant_Slots, o->جایگاه_اولی_Dynamic_شاخص - o->جایگاه_Max_Const_شاخص);
    io_function->رقم_Constant_بایتها = رقم_Constant_Slots * حجم(m3slot_t);

    إذا (رقم_Constant_Slots) {
        io_function->constants = m3_CopyMem((cbytes_t)o->constants, io_function->رقم_Constant_بایتها);
        _throwifnull(io_function->constants);
    }

#إذا d_m3HasSnapshots
    io_function->snapshotMap = o->snapshotMap;
    o->snapshotMap           = عدم;
#پایان_اگر

} _catch:

    Release_Compilation_کد_صفحه(o);

#إذا d_m3HasSnapshots
    SnapshotMap_Free(o->snapshotMap);
    o->snapshotMap = عدم;
    m3_Free(o->local_Init_عمق);
#پایان_اگر

    أرجع نتیجه;
}

#إذا d_m3_Has_پشته_Switching
m3ret_t ResumeContinuation (IM3Runtime i_runtime, IM3Continuation i_cont)
{
    إذا (M3_UNLIKELY(!i_cont || i_cont->state != cont_suspended)) {
        أرجع m3Err_none;
    }

    M3_حافظه_سرآیند* _mem = i_cont->مدخل_Function ? Module_حافظه_سرآیند(i_cont->مدخل_Function->module) : عدم;

    i_runtime->فعال_Continuation = i_cont;
    i_cont->state                 = cont_running;

    i32 outermost     = (i32)i_cont->رقم_قابها - 1;
    i_cont->رقم_قابها = 0;

    // with no frames standing, this lands straight on the suspension point
    m3ret_t r = Replay_قابها(i_cont, outermost, _mem);

    إذا (r == m3Err_continuationSuspended) {
        i_cont->state = cont_suspended;
    } وإلا {
        i_cont->state = cont_returned;
    }
    i_runtime->فعال_Continuation = عدم;

    أرجع r;
}
#پایان_اگر
