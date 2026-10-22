//
//  m3_compile.c
//
//  Created by Steven Massey on 4/17/19.
//  Copyright © 2019 Steven Massey. All rights reserved.
//

// Allow using opcodes for compilation process
#تعریف_کرو M3_COMPILE_OPCODES

#شامل "m3_env.h"
#شامل "m3_compile.h"
#شامل "m3_exec.h"
#شامل "m3_exception.h"
#شامل "m3_info.h"
#شامل "m3_validate.h"

// Resolves M3OpInfo.compiler, an index, to the function it names. Defined down
// with c_compilers [], which has to follow every compiler it points at.
ساکن
M3Compiler GetCompiler (IM3OpInfo i_opInfo);

//----- EMIT --------------------------------------------------------------------------------------------------------------

ساکن اندرونی
pc_t GetPC (IM3Compilation o)
{
    واپس Get_صفحہ_PC(o->صفحہ);
}

#اگر d_m3HasSnapshots

#  define d_m3GrowSnapshotArray(TYPE, ARRAY, گنتی_2, استعداد_2)                    \
    اگر ((گنتی_2) == (استعداد_2)) {                                                 \
        u32   new_استعداد = (استعداد_2) ? (استعداد_2) * 2 : 8;                     \
        TYPE* grown = m3_ReallocArray(TYPE, (ARRAY), new_استعداد, (استعداد_2));   \
        اگر (not grown) {                                                         \
            واپس m3Err_mallocFailed;                                           \
        }                                                                        \
        (ARRAY)    = grown;                                                      \
        (استعداد_2) = new_استعداد;                                                \
    }

// Where the instruction being compiled starts, counted from the start of the
// function's body: the one position every build of every revision agrees on
ساکن اندرونی
u32 Get_Wasm_انحراف (IM3Compilation o)
{
    واپس (u32)(o->آخری_Opcode_آغاز - o->body_آغاز);
}

// Whether the operand stack entry at i_index is a value the frame holds. Every
// block keeps records of its parameter and result slots on the stack, between
// the height it was entered at and where its own values begin (see
// CompileBlock). Those are landing pads rather than values: nothing need have
// written them yet, and the slots they name may be in use for something else.
// The function's own scope has none - that stretch is its arguments and locals.
ساکن
bool Is_زندہ_ڈھیر_اندراج (IM3Compilation o, u16 i_اشاریہ)
{
    برائے (IM3CompilationScope scope = &o->بلاک; scope->outer; scope = scope->outer) {
        اگر (i_اشاریہ >= scope->exit_ڈھیر_اشاریہ and i_اشاریہ < scope->بلاک_ڈھیر_اشاریہ) {
            واپس false;
        }
    }

    واپس true;
}

ساکن
M3_نتیجہ جوڑو_محفوظ_نقطہ_قدر (IM3Compilation o, M3_محفوظ_نقطہ* io_نقطہ, u16 i_اشاریہ)
{
    M3SnapshotMap* map = o->snapshotMap;

    d_m3GrowSnapshotArray(M3_خانہ_قدر, map->اقدار, map->عدد_اقدار, o->اقدار_استعداد);

    map->اقدار[map->عدد_اقدار].خانہ = o->wasm_ڈھیر[i_اشاریہ];
    map->اقدار[map->عدد_اقدار].type = o->type_ڈھیر[i_اشاریہ];
    map->عدد_اقدار++;
    io_نقطہ->عدد_اقدار++;

    واپس m3Err_none;
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
ساکن
M3_نتیجہ ریکارڈ_محفوظ_نقطہ (IM3Compilation o, M3_محفوظ_نقطہ_قسم i_قسم, u32 i_الفاظ_عقب, u16 i_عدد_Results,
                          u16 i_aux, IM3CompilationScope i_loop)
{
_try {
    M3SnapshotMap* map = o->snapshotMap;

    اگر (not map or not o->صفحہ) {
        واپس m3Err_none;
    }

    d_m3GrowSnapshotArray(M3_محفوظ_نقطہ, map->محفوظ_Points, map->عدد_محفوظ_Points, o->محفوظ_Points_استعداد);

    // grown before it is pointed at: adding values never moves the safepoints
    M3_محفوظ_نقطہ* نقطہ     = &map->محفوظ_Points[map->عدد_محفوظ_Points];
    نقطہ->pc              = GetPC(o) - i_الفاظ_عقب;
    نقطہ->wasm_انحراف      = Get_Wasm_انحراف(o);
    نقطہ->پہلا_قدر      = map->عدد_اقدار;
    نقطہ->پہلا_Local_قدر = UINT32_MAX;
    نقطہ->عدد_اقدار       = 0;
    نقطہ->عدد_Results      = i_عدد_Results;
    نقطہ->aux             = i_aux;
    نقطہ->resumeType      = c_m3Type_none;
    نقطہ->قسم            = (u8)i_قسم;
    نقطہ->flags           = 0;

    // Non-defaultable locals can still be uninitialized at a safepoint. Keep
    // their heap types, but allow the null placeholder until local.set makes
    // them definitely initialized. Branches to a loop use its entry context.
    اگر (o->local_Init_گہرائی) {
        نقطہ->پہلا_Local_قدر = map->عدد_اقدار;
        برائے (u32 i = 0; i < map->عدد_Locals; ++i) {
            d_m3GrowSnapshotArray(M3_خانہ_قدر, map->اقدار, map->عدد_اقدار, o->اقدار_استعداد);
            M3_خانہ_قدر قدر = map->locals[i];
            اگر (o->local_Init_گہرائی[i] == UINT32_MAX or
                (i_loop and o->local_Init_گہرائی[i] >= (u32)i_loop->گہرائی)) {
                قدر.type &= ~d_m3Type_refNonNull;
            }
            map->اقدار[map->عدد_اقدار++] = قدر;
        }
        نقطہ->پہلا_قدر = map->عدد_اقدار;
    }

    u16 چوٹی = o->ڈھیر_اشاریہ;

    اگر (i_قسم != safepoint_suspend) {
        چوٹی = (چوٹی > i_عدد_Results) ? (u16)(چوٹی - i_عدد_Results) : 0;
    }
    اگر (i_loop) {
        چوٹی = M3_MIN(چوٹی, i_loop->exit_ڈھیر_اشاریہ);
    }

    برائے (u16 i = o->ڈھیر_پہلا_Dynamic_اشاریہ; i < چوٹی; ++i) {
        اگر (Is_زندہ_ڈھیر_اندراج(o, i)) {
_           (جوڑو_محفوظ_نقطہ_قدر(o, نقطہ, i));
        }
    }

    اگر (i_loop) {
        u16 عدد_Params = Get_Func_Type_عدد_Params(i_loop->type);

        برائے (u16 i = 0; i < عدد_Params; ++i) {
_           (جوڑو_محفوظ_نقطہ_قدر(o, نقطہ, (u16)(i_loop->exit_ڈھیر_اشاریہ + i)));
        }
    }

    map->عدد_محفوظ_Points++;

} _catch: واپس نتیجہ;
}

// A back edge: op_ContinueLoop, or op_ContinueLoopIf when i_isConditional, and
// the loop it goes back to. The conditional one suspends before it has taken
// the branch, so it has to be resumed with the condition that took it.
ساکن
M3_نتیجہ ریکارڈ_عقب_کنارہ (IM3Compilation o, IM3CompilationScope i_loop, bool i_isConditional)
{
    M3_نتیجہ نتیجہ = m3Err_none;

    // the loop is named by its record in the map, which has to fit the field
    _throwif("a function has too many loops for its snapshot map",
             o->snapshotMap and o->صفحہ and (i_loop->snapshot_بلاک == 0 or i_loop->snapshot_بلاک > UINT16_MAX + 1u));

_   (ریکارڈ_محفوظ_نقطہ(o, safepoint_op, 2, 0, (u16)(i_loop->snapshot_بلاک - 1), i_loop));

    اگر (i_isConditional and o->snapshotMap and o->صفحہ) {
        o->snapshotMap->محفوظ_Points[o->snapshotMap->عدد_محفوظ_Points - 1].flags |= d_m3_محفوظ_نقطہ_لیا_گیا_Branch;
    }

    _catch: واپس نتیجہ;
}

// A call, waiting on its callee with i_numResults not yet written; the
// callee's frame starts i_calleeSlot slots above this one. A return_call that
// is compiled as a call is one too, since its frame stands while the callee
// runs, but it is marked: in Wasm the callee has replaced it, so a snapshot
// walks past it rather than writing it.
ساکن
M3_نتیجہ ریکارڈ_Call (IM3Compilation o, u16 i_عدد_Results, u16 i_callee_خانہ, bool i_isReturnCall)
{
    M3_نتیجہ نتیجہ = ریکارڈ_محفوظ_نقطہ(o, safepoint_call, 0, i_عدد_Results, i_callee_خانہ, تہی);

    اگر (not نتیجہ and i_isReturnCall and o->snapshotMap and o->صفحہ) {
        o->snapshotMap->محفوظ_Points[o->snapshotMap->عدد_محفوظ_Points - 1].flags |= d_m3_محفوظ_نقطہ_دم_Call;
    }

    واپس نتیجہ;
}

// Where the function's arguments and locals are kept. Unlike the operand
// stack, that never changes, so it is recorded once rather than at every
// safepoint.
ساکن
M3_نتیجہ ریکارڈ_Locals (IM3Compilation o)
{
    M3SnapshotMap* map = o->snapshotMap;

    اگر (map and o->ڈھیر_اشاریہ) {
        map->locals = m3_AllocArray(M3_خانہ_قدر, o->ڈھیر_اشاریہ);

        اگر (not map->locals) {
            واپس m3Err_mallocFailed;
        }

        برائے (u16 i = 0; i < o->ڈھیر_اشاریہ; ++i) {
            map->locals[i].خانہ = o->wasm_ڈھیر[i];
            map->locals[i].type = o->type_ڈھیر[i];
        }

        map->عدد_Locals = o->ڈھیر_اشاریہ;
        برائے (u32 i = Get_Function_عدد_دلائل(o->function); i < map->عدد_Locals; ++i) {
            اگر (IsRefType(map->locals[i].type) and not IsNullableRef(map->locals[i].type)) {
                o->local_Init_گہرائی = m3_AllocArray(u32, map->عدد_Locals);
                اگر (not o->local_Init_گہرائی) {
                    واپس m3Err_mallocFailed;
                }
                برائے (u32 j = i; j < map->عدد_Locals; ++j) {
                    اگر (IsRefType(map->locals[j].type) and not IsNullableRef(map->locals[j].type)) {
                        o->local_Init_گہرائی[j] = UINT32_MAX;
                    }
                }
                توڑو;
            }
        }
    }

    واپس m3Err_none;
}

// A loop or a try_table, whose native frame a snapshot names by the
// instruction's offset. i_pc is what that frame records. The scope being
// compiled keeps the record, so that RecordBlockEnd can close it.
ساکن
M3_نتیجہ ریکارڈ_بلاک_آغاز (IM3Compilation o, m3opcode_t i_opcode, pc_t i_pc, u32 i_عدد_Clauses)
{
    M3SnapshotMap* map = o->snapshotMap;

    اگر (map and o->صفحہ) {
        d_m3GrowSnapshotArray(M3_بلاک_آغاز, map->بلاکیں, map->عدد_بلاکیں, o->بلاکیں_استعداد);

        M3_بلاک_آغاز* بلاک = &map->بلاکیں[map->عدد_بلاکیں++];
        بلاک->pc           = i_pc;
        بلاک->wasm_انحراف   = Get_Wasm_انحراف(o);
        بلاک->wasm_انتہا      = UINT32_MAX;
        بلاک->عدد_Clauses   = (u16)i_عدد_Clauses;
        بلاک->opcode       = (u8)i_opcode;

        o->بلاک.snapshot_بلاک = map->عدد_بلاکیں;
    }

    واپس m3Err_none;
}

// Closes the scope's region, while the instruction just read is its end
ساکن
خالی ریکارڈ_بلاک_انتہا (IM3Compilation o)
{
    اگر (o->snapshotMap and o->بلاک.snapshot_بلاک) {
        o->snapshotMap->بلاکیں[o->بلاک.snapshot_بلاک - 1].wasm_انتہا = Get_Wasm_انحراف(o);
    }
}

#  undef d_m3GrowSnapshotArray

#ورنہ

ساکن اندرونی
M3_نتیجہ ریکارڈ_محفوظ_نقطہ (IM3Compilation o, M3_محفوظ_نقطہ_قسم i_قسم, u32 i_الفاظ_عقب, u16 i_عدد_Results,
                          u16 i_aux, IM3CompilationScope i_loop)
{
    (خالی)o;
    (خالی)i_قسم;
    (خالی)i_الفاظ_عقب;
    (خالی)i_عدد_Results;
    (خالی)i_aux;
    (خالی)i_loop;
    واپس m3Err_none;
}

ساکن اندرونی
M3_نتیجہ ریکارڈ_عقب_کنارہ (IM3Compilation o, IM3CompilationScope i_loop, bool i_isConditional)
{
    (خالی)o;
    (خالی)i_loop;
    (خالی)i_isConditional;
    واپس m3Err_none;
}

ساکن اندرونی
M3_نتیجہ ریکارڈ_Call (IM3Compilation o, u16 i_عدد_Results, u16 i_callee_خانہ, bool i_isReturnCall)
{
    (خالی)o;
    (خالی)i_عدد_Results;
    (خالی)i_callee_خانہ;
    (خالی)i_isReturnCall;
    واپس m3Err_none;
}

#اگر_ختم // d_m3HasSnapshots

ساکن M3_NOINLINE
M3_نتیجہ Ensure_رمز_صفحہ_عدد_سطریں (IM3Compilation o, u32 i_عدد_سطریں)
{
    M3_نتیجہ نتیجہ = m3Err_none;

    i_عدد_سطریں += 2; // room for Bridge

    اگر (M3_UNLIKELY(عدد_Free_سطریں(o->صفحہ) < i_عدد_سطریں)) {
        IM3_رمز_صفحہ صفحہ = Acquire_رمز_صفحہ_With_استعداد(o->runtime, i_عدد_سطریں);

        اگر (صفحہ) {
            m3log(emit, "bridging new code page from: %d %p (free slots: %d) to: %d", o->صفحہ->info.sequence, GetPC(o), عدد_Free_سطریں(o->صفحہ), صفحہ->info.sequence);
            d_m3Assert(عدد_Free_سطریں(o->صفحہ) >= 2);

            Emit_لفظ(o->صفحہ, op_Branch);
            Emit_لفظ(o->صفحہ, Get_صفحہ_PC(صفحہ));

            Release_رمز_صفحہ(o->runtime, o->صفحہ);

            o->صفحہ = صفحہ;
        } ورنہ {
            نتیجہ = m3_Err_malloc_Failed_رمز_صفحہ;
        }
    }

    واپس نتیجہ;
}

// invalidate the pending local.set fold candidate; called wherever code is emitted or a
// branch target captures the current pc (a fold appends an immediate, moving that position)
ساکن اندرونی
خالی InvalidateFold (IM3Compilation o)
{
#اگر d_m3_Fold_سیٹ_Local
    o->foldPatchPC = تہی;
#ورنہ
    (خالی)o;
#اگر_ختم
}

ساکن M3_NOINLINE
M3_نتیجہ EmitOp (IM3Compilation o, IM3Operation i_operation)
{
    M3_نتیجہ نتیجہ = m3Err_none;                                 d_m3Assert (i_operation or Is_ڈھیر_Polymorphic (o));

    InvalidateFold(o);

    // it's OK for page to be null; when compile-walking the bytecode without emitting
    اگر (o->صفحہ) {
#اگر d_m3EnableOpTracing
        اگر (i_operation != op_Dump_ڈھیر) {
            o->عدد_Emits++;
        }
#اگر_ختم

        // have execution jump to a new page if slots are critically low
        نتیجہ = Ensure_رمز_صفحہ_عدد_سطریں(o, d_m3_رمز_صفحہ_Free_سطریں_حد);

        اگر (M3_LIKELY(not نتیجہ)) {
            اگر (d_m3LogEmit) {
                log_emit(o, i_operation);
            }
#اگر d_m3_ریکارڈ_Backtraces
            Emit_Mapping_اندراج(o->صفحہ, (u32)(o->آخری_Opcode_آغاز - o->module->wasm_آغاز));
#اگر_ختم // d_m3RecordBacktraces
            Emit_لفظ(o->صفحہ, i_operation);
        }
    }

    واپس نتیجہ;
}

// Push an immediate constant into the M3 codestream
ساکن M3_NOINLINE
خالی EmitConstant32 (IM3Compilation o, ثابت u32 i_فوری)
{
    اگر (o->صفحہ) {                                                          m3log (emit, "const32: %ud", i_فوری);
        EmitWord32(o->صفحہ, i_فوری);
    }
}

#اگر d_m3HasMemory64
// Takes two lines of the code page where a pointer is 32 bits, so whatever
// reads it back has to step _pc by the same amount - see op_Const64.
ساکن M3_NOINLINE
خالی EmitConstant64 (IM3Compilation o, ثابت u64 i_فوری)
{
    اگر (o->صفحہ) {
        EmitWord64(o->صفحہ, i_فوری);
    }
}
#اگر_ختم

ساکن M3_NOINLINE
خالی Emit_خانہ_انحراف (IM3Compilation o, ثابت i32 i_انحراف)
{
    اگر (o->صفحہ) {                                                          m3log (emit, "slot: [%d]", i_انحراف);
        EmitWord32(o->صفحہ, i_انحراف);
    }
}

ساکن M3_NOINLINE
pc_t Emit_اشارہ_گر (IM3Compilation o, ثابت خالی* ثابت i_اشارہ_گر)
{
    pc_t ptr = Get_صفحہ_PC(o->صفحہ);

    اگر (o->صفحہ) {                                                          m3log (emit, "ptr: %p", i_اشارہ_گر);
        Emit_لفظ(o->صفحہ, i_اشارہ_گر);
    }

    واپس ptr;
}

ساکن M3_NOINLINE
خالی* Reserve_اشارہ_گر (IM3Compilation o)
{
    pc_t ptr = Get_صفحہ_PC(o->صفحہ);
    Emit_اشارہ_گر(o, تہی);
    واپس (خالی*)ptr;
}


//-------------------------------------------------------------------------------------------------------------------------

#تعریف_کرو d_indent "     | %s"

// just want less letters and numbers to stare at down the way in the compiler table
#تعریف_کرو i_32    c_m3Type_i32
#تعریف_کرو i_64    c_m3Type_i64
#تعریف_کرو f_32    c_m3Type_f32
#تعریف_کرو f_64    c_m3Type_f64
#تعریف_کرو none    c_m3Type_none
#تعریف_کرو any     (u8)-1

#اگر d_m3HasFloat
#  define FPOP(x) x
#ورنہ
#  define FPOP(x) تہی
#اگر_ختم

// These are indexed by M3ValueType, so every type up to c_m3Type_externref needs
// an entry. A reference is one pointer-sized word, so it moves with the integer
// operation of that width; v128 has no operations at all.
#اگر M3_SIZEOF_PTR == 8
#  define REFOP(NAME)  op_##NAME##_i64
#ورنہ
#  define REFOP(NAME)  op_##NAME##_i32
#اگر_ختم

// clang-format off

ساکن ثابت IM3Operation c_preserve_سیٹ_خانہ[] = { تہی, op_Preserve_سیٹ_خانہ_i32,       op_Preserve_سیٹ_خانہ_i64,
                                                    FPOP(op_Preserve_سیٹ_خانہ_f32), FPOP(op_Preserve_سیٹ_خانہ_f64),
                                                    تہی, REFOP(Preserve_سیٹ_خانہ),  REFOP(Preserve_سیٹ_خانہ),
                                                    REFOP(Preserve_سیٹ_خانہ),       REFOP(Preserve_سیٹ_خانہ) };
ساکن ثابت IM3Operation c_سیٹ_سیٹ_Ops[] =       { تہی, op_سیٹ_خانہ_i32,               op_سیٹ_خانہ_i64,
                                                    FPOP(op_سیٹ_خانہ_f32),         FPOP(op_سیٹ_خانہ_f64),
                                                    تہی, REFOP(سیٹ_خانہ),         REFOP(سیٹ_خانہ),
                                                    REFOP(سیٹ_خانہ),               REFOP(سیٹ_خانہ) };
ساکن ثابت IM3Operation c_سیٹ_Global_Ops[] =    { تہی, op_سیٹ_Global_i32,             op_سیٹ_Global_i64,
                                                    FPOP(op_سیٹ_Global_f32),       FPOP(op_سیٹ_Global_f64),
                                                    تہی, REFOP(سیٹ_Global),       REFOP(سیٹ_Global),
                                                    REFOP(سیٹ_Global),             REFOP(سیٹ_Global) };
ساکن ثابت IM3Operation c_سیٹ_رجسٹر_Ops[] =  { تہی, op_سیٹ_رجسٹر_i32,           op_سیٹ_رجسٹر_i64,
                                                    FPOP(op_سیٹ_رجسٹر_f32),     FPOP(op_سیٹ_رجسٹر_f64),
                                                    تہی, REFOP(سیٹ_رجسٹر),     REFOP(سیٹ_رجسٹر),
                                                    REFOP(سیٹ_رجسٹر),           REFOP(سیٹ_رجسٹر) };

// A table shorter than the enum reads out of bounds, so tie the two together at
// build time: adding a value type must not silently outgrow these.
#تعریف_کرو d_m3_Check_Type_جدول(جدول) \
    M3_STATIC_ASSERT (حجم (جدول) / حجم (*(جدول)) == c_m3_Type_گنتی, جدول##_needs_one_entry_per_type)

d_m3_Check_Type_جدول(c_preserve_سیٹ_خانہ);
d_m3_Check_Type_جدول(c_سیٹ_سیٹ_Ops);
d_m3_Check_Type_جدول(c_سیٹ_Global_Ops);
d_m3_Check_Type_جدول(c_سیٹ_رجسٹر_Ops);

#اگر d_m3_Fold_سیٹ_Local

// destination-folded variants of the hot binops, indexed by the operand-form index
// recorded at emission: [0]=_rs [1]=_sr [2]=_ss [3]=(fp _rr, never folded)
#تعریف_کرو d_foldOpList(TYPE, NAME)            { op_##TYPE##_##NAME##_rs_f, op_##TYPE##_##NAME##_sr_f, op_##TYPE##_##NAME##_ss_f, تہی }
#تعریف_کرو d_foldCommutativeOpList(TYPE, NAME) { op_##TYPE##_##NAME##_rs_f, تہی,                      op_##TYPE##_##NAME##_ss_f, تہی }

ساکن ثابت IM3Operation c_fold_i32_جوڑو[]         = d_foldCommutativeOpList (i32, جوڑو);
ساکن ثابت IM3Operation c_fold_i32_تفریق[]    = d_foldOpList (i32, تفریق);
ساکن ثابت IM3Operation c_fold_i32_Multiply[]    = d_foldCommutativeOpList (i32, Multiply);
ساکن ثابت IM3Operation c_fold_u32_And[]         = d_foldCommutativeOpList (u32, And);
ساکن ثابت IM3Operation c_fold_u32_Or[]          = d_foldCommutativeOpList (u32, Or);
ساکن ثابت IM3Operation c_fold_u32_Xor[]         = d_foldCommutativeOpList (u32, Xor);
ساکن ثابت IM3Operation c_fold_u32_سرکاؤ_بایاں[]   = d_foldOpList (u32, سرکاؤ_بایاں);
ساکن ثابت IM3Operation c_fold_i32_سرکاؤ_دایاں[]  = d_foldOpList (i32, سرکاؤ_دایاں);
ساکن ثابت IM3Operation c_fold_u32_سرکاؤ_دایاں[]  = d_foldOpList (u32, سرکاؤ_دایاں);

ساکن ثابت IM3Operation c_fold_i64_جوڑو[]         = d_foldCommutativeOpList (i64, جوڑو);
ساکن ثابت IM3Operation c_fold_i64_تفریق[]    = d_foldOpList (i64, تفریق);
ساکن ثابت IM3Operation c_fold_i64_Multiply[]    = d_foldCommutativeOpList (i64, Multiply);
ساکن ثابت IM3Operation c_fold_u64_And[]         = d_foldCommutativeOpList (u64, And);
ساکن ثابت IM3Operation c_fold_u64_Or[]          = d_foldCommutativeOpList (u64, Or);
ساکن ثابت IM3Operation c_fold_u64_Xor[]         = d_foldCommutativeOpList (u64, Xor);
ساکن ثابت IM3Operation c_fold_u64_سرکاؤ_بایاں[]   = d_foldOpList (u64, سرکاؤ_بایاں);
ساکن ثابت IM3Operation c_fold_i64_سرکاؤ_دایاں[]  = d_foldOpList (i64, سرکاؤ_دایاں);
ساکن ثابت IM3Operation c_fold_u64_سرکاؤ_دایاں[]  = d_foldOpList (u64, سرکاؤ_دایاں);

#  اگر d_m3HasFloat
ساکن ثابت IM3Operation c_fold_f32_جوڑو[]         = d_foldCommutativeOpList (f32, جوڑو);
ساکن ثابت IM3Operation c_fold_f32_تفریق[]    = d_foldOpList (f32, تفریق);
ساکن ثابت IM3Operation c_fold_f32_Multiply[]    = d_foldCommutativeOpList (f32, Multiply);
ساکن ثابت IM3Operation c_fold_f32_Divide[]      = d_foldOpList (f32, Divide);
ساکن ثابت IM3Operation c_fold_f64_جوڑو[]         = d_foldCommutativeOpList (f64, جوڑو);
ساکن ثابت IM3Operation c_fold_f64_تفریق[]    = d_foldOpList (f64, تفریق);
ساکن ثابت IM3Operation c_fold_f64_Multiply[]    = d_foldCommutativeOpList (f64, Multiply);
ساکن ثابت IM3Operation c_fold_f64_Divide[]      = d_foldOpList (f64, Divide);
#  endif

// loads are unary: [0]= address in _r0, [1]= address in a slot
#تعریف_کرو d_fold_بھرو_List(DEST, SRC)       { op_##DEST##_Load_##SRC##_r_f, op_##DEST##_Load_##SRC##_s_f, تہی, تہی }

ساکن ثابت IM3Operation c_fold_i32_بھرو_i32[] = d_fold_بھرو_List(i32, i32);
ساکن ثابت IM3Operation c_fold_i32_بھرو_i8[] = d_fold_بھرو_List(i32, i8);
ساکن ثابت IM3Operation c_fold_i32_بھرو_u8[] = d_fold_بھرو_List(i32, u8);
ساکن ثابت IM3Operation c_fold_i32_بھرو_i16[] = d_fold_بھرو_List(i32, i16);
ساکن ثابت IM3Operation c_fold_i32_بھرو_u16[] = d_fold_بھرو_List(i32, u16);

ساکن ثابت IM3Operation c_fold_i64_بھرو_i64[] = d_fold_بھرو_List(i64, i64);
ساکن ثابت IM3Operation c_fold_i64_بھرو_i8[] = d_fold_بھرو_List(i64, i8);
ساکن ثابت IM3Operation c_fold_i64_بھرو_u8[] = d_fold_بھرو_List(i64, u8);
ساکن ثابت IM3Operation c_fold_i64_بھرو_i16[] = d_fold_بھرو_List(i64, i16);
ساکن ثابت IM3Operation c_fold_i64_بھرو_u16[] = d_fold_بھرو_List(i64, u16);
ساکن ثابت IM3Operation c_fold_i64_بھرو_i32[] = d_fold_بھرو_List(i64, i32);
ساکن ثابت IM3Operation c_fold_i64_بھرو_u32[] = d_fold_بھرو_List(i64, u32);
#  اگر d_m3HasFloat
ساکن ثابت IM3Operation c_fold_f32_بھرو_f32[] = d_fold_بھرو_List(f32, f32);
ساکن ثابت IM3Operation c_fold_f64_بھرو_f64[] = d_fold_بھرو_List(f64, f64);
#  endif

ساکن
IM3Operation GetFoldOp (m3opcode_t i_opcode, u8 i_form)
{
    اگر (i_form >= 4) {
        واپس تہی;
    }

    انتخاب (i_opcode) {
    حالت 0x6a:  واپس c_fold_i32_جوڑو        [i_form];      // i32.add
    حالت 0x6b:  واپس c_fold_i32_تفریق   [i_form];      // i32.sub
    حالت 0x6c:  واپس c_fold_i32_Multiply   [i_form];      // i32.mul
    حالت 0x71:  واپس c_fold_u32_And        [i_form];      // i32.and
    حالت 0x72:  واپس c_fold_u32_Or         [i_form];      // i32.or
    حالت 0x73:  واپس c_fold_u32_Xor        [i_form];      // i32.xor
    حالت 0x74:  واپس c_fold_u32_سرکاؤ_بایاں  [i_form];      // i32.shl
    حالت 0x75:  واپس c_fold_i32_سرکاؤ_دایاں [i_form];      // i32.shr_s
    حالت 0x76:  واپس c_fold_u32_سرکاؤ_دایاں [i_form];      // i32.shr_u

    حالت 0x7c:  واپس c_fold_i64_جوڑو        [i_form];      // i64.add
    حالت 0x7d:  واپس c_fold_i64_تفریق   [i_form];      // i64.sub
    حالت 0x7e:  واپس c_fold_i64_Multiply   [i_form];      // i64.mul
    حالت 0x83:  واپس c_fold_u64_And        [i_form];      // i64.and
    حالت 0x84:  واپس c_fold_u64_Or         [i_form];      // i64.or
    حالت 0x85:  واپس c_fold_u64_Xor        [i_form];      // i64.xor
    حالت 0x86:  واپس c_fold_u64_سرکاؤ_بایاں  [i_form];      // i64.shl
    حالت 0x87:  واپس c_fold_i64_سرکاؤ_دایاں [i_form];      // i64.shr_s
    حالت 0x88:  واپس c_fold_u64_سرکاؤ_دایاں [i_form];      // i64.shr_u

#  اگر d_m3HasFloat
    حالت 0x92:  واپس c_fold_f32_جوڑو        [i_form];      // f32.add
    حالت 0x93:  واپس c_fold_f32_تفریق   [i_form];      // f32.sub
    حالت 0x94:  واپس c_fold_f32_Multiply   [i_form];      // f32.mul
    حالت 0x95:  واپس c_fold_f32_Divide     [i_form];      // f32.div
    حالت 0xa0:  واپس c_fold_f64_جوڑو        [i_form];      // f64.add
    حالت 0xa1:  واپس c_fold_f64_تفریق   [i_form];      // f64.sub
    حالت 0xa2:  واپس c_fold_f64_Multiply   [i_form];      // f64.mul
    حالت 0xa3:  واپس c_fold_f64_Divide     [i_form];      // f64.div
#  endif

    حالت 0x28:  واپس c_fold_i32_بھرو_i32   [i_form];      // i32.load
    حالت 0x2c:  واپس c_fold_i32_بھرو_i8    [i_form];      // i32.load8_s
    حالت 0x2d:  واپس c_fold_i32_بھرو_u8    [i_form];      // i32.load8_u
    حالت 0x2e:  واپس c_fold_i32_بھرو_i16   [i_form];      // i32.load16_s
    حالت 0x2f:  واپس c_fold_i32_بھرو_u16   [i_form];      // i32.load16_u

    حالت 0x29:  واپس c_fold_i64_بھرو_i64   [i_form];      // i64.load
    حالت 0x30:  واپس c_fold_i64_بھرو_i8    [i_form];      // i64.load8_s
    حالت 0x31:  واپس c_fold_i64_بھرو_u8    [i_form];      // i64.load8_u
    حالت 0x32:  واپس c_fold_i64_بھرو_i16   [i_form];      // i64.load16_s
    حالت 0x33:  واپس c_fold_i64_بھرو_u16   [i_form];      // i64.load16_u
    حالت 0x34:  واپس c_fold_i64_بھرو_i32   [i_form];      // i64.load32_s
    حالت 0x35:  واپس c_fold_i64_بھرو_u32   [i_form];      // i64.load32_u

#  اگر d_m3HasFloat
    حالت 0x2a:  واپس c_fold_f32_بھرو_f32   [i_form];      // f32.load
    حالت 0x2b:  واپس c_fold_f64_بھرو_f64   [i_form];      // f64.load
#  endif

    طےشدہ: واپس تہی;
    }
}

#اگر_ختم // d_m3FoldSetLocal

// clang-format on

#اگر d_m3FuseBranch

// fused compare+branch/if variants, indexed like the fold lists: [0]=_rs [1]=_sr [2]=_ss [3]=unused.
// the [1] entries are NULL for commutative compares: the compiler never records the _sr form for them
#  define d_fuseCmpList(TYPE, NAME, قسم_2)             { op_##TYPE##_##KIND##_##NAME##_rs, op_##TYPE##_##KIND##_##NAME##_sr, op_##TYPE##_##KIND##_##NAME##_ss, تہی }
#  define d_fuseCommutativeCmpList(TYPE, NAME, قسم_2)  { op_##TYPE##_##KIND##_##NAME##_rs, تہی,                             op_##TYPE##_##KIND##_##NAME##_ss, تہی }

قسمتعریف ساخت M3FusedCmpOps {
    IM3Operation branchIf[4];
    IM3Operation ifOp[4];
} M3FusedCmpOps;

#  define d_fuseCmp(TYPE, NAME)               { d_fuseCmpList (TYPE, NAME, BranchIf),            d_fuseCmpList (TYPE, NAME, If) }
#  define d_fuseCommutativeCmp(TYPE, NAME)    { d_fuseCommutativeCmpList (TYPE, NAME, BranchIf), d_fuseCommutativeCmpList (TYPE, NAME, If) }

// clang-format off
ساکن ثابت M3FusedCmpOps c_fuse_i32_Equal              = d_fuseCommutativeCmp (i32, Equal);
ساکن ثابت M3FusedCmpOps c_fuse_i32_NotEqual           = d_fuseCommutativeCmp (i32, NotEqual);
ساکن ثابت M3FusedCmpOps c_fuse_i32_LessThan           = d_fuseCmp (i32, LessThan);
ساکن ثابت M3FusedCmpOps c_fuse_u32_LessThan           = d_fuseCmp (u32, LessThan);
ساکن ثابت M3FusedCmpOps c_fuse_i32_GreaterThan        = d_fuseCmp (i32, GreaterThan);
ساکن ثابت M3FusedCmpOps c_fuse_u32_GreaterThan        = d_fuseCmp (u32, GreaterThan);
ساکن ثابت M3FusedCmpOps c_fuse_i32_LessThanOrEqual    = d_fuseCmp (i32, LessThanOrEqual);
ساکن ثابت M3FusedCmpOps c_fuse_u32_LessThanOrEqual    = d_fuseCmp (u32, LessThanOrEqual);
ساکن ثابت M3FusedCmpOps c_fuse_i32_GreaterThanOrEqual = d_fuseCmp (i32, GreaterThanOrEqual);
ساکن ثابت M3FusedCmpOps c_fuse_u32_GreaterThanOrEqual = d_fuseCmp (u32, GreaterThanOrEqual);
// clang-format on

// i32.eqz is unary: [0]= operand in _r0, [1]= operand in a slot
ساکن ثابت M3FusedCmpOps c_fuse_i32_Eqz = {
    { op_i32_BranchIfEqz_r, op_i32_BranchIfEqz_s, تہی, تہی },
    { op_i32_IfEqz_r,       op_i32_IfEqz_s,       تہی, تہی }
};

ساکن
ثابت M3FusedCmpOps* GetFusedCmpOps (m3opcode_t i_opcode)
{
    انتخاب (i_opcode) {
    حالت 0x45: واپس &c_fuse_i32_Eqz;                    // i32.eqz
    حالت 0x46: واپس &c_fuse_i32_Equal;                  // i32.eq
    حالت 0x47: واپس &c_fuse_i32_NotEqual;               // i32.ne
    حالت 0x48: واپس &c_fuse_i32_LessThan;               // i32.lt_s
    حالت 0x49: واپس &c_fuse_u32_LessThan;               // i32.lt_u
    حالت 0x4a: واپس &c_fuse_i32_GreaterThan;            // i32.gt_s
    حالت 0x4b: واپس &c_fuse_u32_GreaterThan;            // i32.gt_u
    حالت 0x4c: واپس &c_fuse_i32_LessThanOrEqual;        // i32.le_s
    حالت 0x4d: واپس &c_fuse_u32_LessThanOrEqual;        // i32.le_u
    حالت 0x4e: واپس &c_fuse_i32_GreaterThanOrEqual;     // i32.ge_s
    حالت 0x4f: واپس &c_fuse_u32_GreaterThanOrEqual;     // i32.ge_u
    طےشدہ: واپس تہی;
    }
}

#اگر_ختم // d_m3FuseBranch

ساکن ثابت IM3Operation c_intSelectOps[2][4] = {
    { op_Select_i32_rss, op_Select_i32_srs, op_Select_i32_ssr, op_Select_i32_sss },
    { op_Select_i64_rss, op_Select_i64_srs, op_Select_i64_ssr, op_Select_i64_sss }
};
#اگر d_m3HasFloat
ساکن ثابت IM3Operation c_fpSelectOps[2][2][3] = {
    { { op_Select_f32_sss, op_Select_f32_srs, op_Select_f32_ssr }, // selector in slot
      { op_Select_f32_rss, op_Select_f32_rrs, op_Select_f32_rsr } }, // selector in reg
    { { op_Select_f64_sss, op_Select_f64_srs, op_Select_f64_ssr }, // selector in slot
      { op_Select_f64_rss, op_Select_f64_rrs, op_Select_f64_rsr } }
};    // selector in reg
#اگر_ختم

// all args & returns are 64-bit aligned, so use 2 slots for a d_m3Use32BitSlots=1 build

ساکن
M3_نتیجہ Acquire_Compilation_رمز_صفحہ (IM3Compilation o, IM3_رمز_صفحہ* o_رمز_صفحہ)
{
    M3_نتیجہ نتیجہ = m3Err_none;

    IM3_رمز_صفحہ صفحہ = Acquire_رمز_صفحہ(o->runtime);

    اگر (صفحہ) {
#اگر (d_m3_Enable_رمز_صفحہ_Ref_Counting)
        {
            اگر (o->function) {
                IM3Function func = o->function;
                صفحہ->info.usage_گنتی++;

                u32 index = func->عدد_رمز_صفحہ_Refs++;
_               (m3ReallocArray(&func->رمز_صفحہ_Refs, IM3_رمز_صفحہ, func->عدد_رمز_صفحہ_Refs, index));
                func->رمز_صفحہ_Refs[index] = صفحہ;
            }
        }
#اگر_ختم
    } ورنہ {
        _throw(m3_Err_malloc_Failed_رمز_صفحہ);
    }

    _catch:

    *o_رمز_صفحہ = صفحہ;

    واپس نتیجہ;
}

ساکن اندرونی
خالی Release_Compilation_رمز_صفحہ (IM3Compilation o)
{
    Release_رمز_صفحہ(o->runtime, o->صفحہ);
}

ساکن اندرونی
u16 Get_Type_عدد_Slots (m3type_t i_type)
{
    i_type = بنیاد_Type_کا(i_type);

    // v128 is 16 bytes - 4 slots in 32-bit-slot mode, 2 in 64-bit.
    // (Slot-allocator only; no v128 ops execute.)
    اگر (i_type == c_m3Type_v128)
#اگر d_m3_Use32_ثنائی_ہندسہ_Slots
        واپس 4;
#ورنہ
        واپس 2;
#اگر_ختم
#اگر d_m3_Use32_ثنائی_ہندسہ_Slots
    واپس Is64_ثنائی_ہندسہ_Type(i_type) ? 2 : 1;
#ورنہ
    واپس 1;
#اگر_ختم
}

ساکن اندرونی
خالی Align_خانہ_مقصد_Type (u16* io_خانہ, m3type_t i_type)
{
    // align 64-bit words to even slots (if d_m3Use32BitSlots)
    u16 عدد_Slots = Get_Type_عدد_Slots(i_type);

    u16 نقاب = عدد_Slots - 1;
    *io_خانہ = (*io_خانہ + نقاب) & ~نقاب;
}

// Deliberately unchecked. Validation rejects an empty operand stack before compilation.
ساکن اندرونی
i16 Get_ڈھیر_چوٹی_اشاریہ (IM3Compilation o)
{
    واپس o->ڈھیر_اشاریہ - 1;
}

ساکن اندرونی
M3_نتیجہ Get_ڈھیر_چوٹی_اشاریہ_Throws (IM3Compilation o, i16* o_ڈھیر_اشاریہ)
{
    *o_ڈھیر_اشاریہ = o->ڈھیر_اشاریہ - 1;

    اگر (M3_LIKELY(o->ڈھیر_اشاریہ > o->ڈھیر_پہلا_Dynamic_اشاریہ or Is_ڈھیر_Polymorphic(o))) {
        واپس m3Err_none;
    } ورنہ {
        واپس m3_Err_function_ڈھیر_Underrun;
    }
}


// Items in the static portion of the stack (args/locals) are hidden from GetStackTypeFromTop ()
// In other words, only "real" Wasm stack items can be inspected.  This is important when
// returning values, etc. and you need an accurate wasm-view of the stack.
ساکن
m3type_t Get_ڈھیر_Type_From_چوٹی (IM3Compilation o, u16 i_انحراف)
{
    m3type_t type = c_m3Type_none;

    ++i_انحراف;
    اگر (o->ڈھیر_اشاریہ >= i_انحراف) {
        u16 index = o->ڈھیر_اشاریہ - i_انحراف;

        اگر (index >= o->ڈھیر_پہلا_Dynamic_اشاریہ) {
            type = o->type_ڈھیر[index];
        }
    }

    واپس type;
}

ساکن اندرونی
m3type_t Get_ڈھیر_چوٹی_Type (IM3Compilation o)
{
    واپس Get_ڈھیر_Type_From_چوٹی(o, 0);
}

ساکن اندرونی
m3type_t Get_ڈھیر_Type_From_Bottom (IM3Compilation o, u16 i_انحراف)
{
    m3type_t type = c_m3Type_none;

    اگر (i_انحراف < o->ڈھیر_اشاریہ) {
        type = o->type_ڈھیر[i_انحراف];
    }

    واپس type;
}


ساکن اندرونی
bool Is_Constant_خانہ (IM3Compilation o, u16 i_خانہ)
{
    واپس (i_خانہ >= o->خانہ_پہلا_Const_اشاریہ and i_خانہ < o->خانہ_Max_Const_اشاریہ);
}
ساکن اندرونی
bool Is_خانہ_Allocated (IM3Compilation o, u16 i_خانہ)
{
    واپس o->m3Slots[i_خانہ];
}

ساکن اندرونی
bool Is_ڈھیر_اشاریہ_اندر_رجسٹر (IM3Compilation o, i32 i_ڈھیر_اشاریہ)
{                                                                           d_m3Assert (i_ڈھیر_اشاریہ < o->ڈھیر_اشاریہ or Is_ڈھیر_Polymorphic (o));
    اگر (i_ڈھیر_اشاریہ >= 0 and i_ڈھیر_اشاریہ < o->ڈھیر_اشاریہ) {
        واپس (o->wasm_ڈھیر[i_ڈھیر_اشاریہ] >= d_m3_Reg0_خانہ_Alias);
    } ورنہ {
        واپس false;
    }
}

ساکن اندرونی
u16 Get_عدد_بلاک_اقدار_On_ڈھیر (IM3Compilation o)
{
    واپس o->ڈھیر_اشاریہ - o->بلاک.بلاک_ڈھیر_اشاریہ;
}

ساکن اندرونی
bool Is_ڈھیر_چوٹی_اندر_رجسٹر (IM3Compilation o)
{
    واپس Is_ڈھیر_اشاریہ_اندر_رجسٹر(o, (i32)Get_ڈھیر_چوٹی_اشاریہ(o));
}
ساکن اندرونی
bool Is_ڈھیر_چوٹی_Minus1_اندر_رجسٹر (IM3Compilation o)
{
    واپس Is_ڈھیر_اشاریہ_اندر_رجسٹر(o, (i32)Get_ڈھیر_چوٹی_اشاریہ(o) - 1);
}
ساکن اندرونی
bool Is_ڈھیر_چوٹی_Minus2_اندر_رجسٹر (IM3Compilation o)
{
    واپس Is_ڈھیر_اشاریہ_اندر_رجسٹر(o, (i32)Get_ڈھیر_چوٹی_اشاریہ(o) - 2);
}

ساکن اندرونی
bool Is_ڈھیر_چوٹی_اندر_خانہ (IM3Compilation o)
{
    واپس not Is_ڈھیر_چوٹی_اندر_رجسٹر(o);
}

ساکن اندرونی
bool Is_Valid_خانہ (u16 i_خانہ)
{
    واپس (i_خانہ < d_m3MaxFunctionSlots);
}

ساکن اندرونی
u16 Get_ڈھیر_چوٹی_خانہ_نمبر (IM3Compilation o)
{
    i16 i = Get_ڈھیر_چوٹی_اشاریہ(o);

    u16 خانہ = c_خانہ_غیر_مستعمل;

    اگر (i >= 0) {
        خانہ = o->wasm_ڈھیر[i];
    }

    واپس خانہ;
}


// from bottom
ساکن اندرونی
u16 Get_خانہ_For_ڈھیر_اشاریہ (IM3Compilation o, u16 i_ڈھیر_اشاریہ)
{                                                                   d_m3Assert (i_ڈھیر_اشاریہ < o->ڈھیر_اشاریہ or Is_ڈھیر_Polymorphic (o));
    u16 خانہ = c_خانہ_غیر_مستعمل;

    اگر (i_ڈھیر_اشاریہ < o->ڈھیر_اشاریہ) {
        خانہ = o->wasm_ڈھیر[i_ڈھیر_اشاریہ];
    }

    واپس خانہ;
}

ساکن اندرونی
u16 Get_Extra_خانہ_For_ڈھیر_اشاریہ (IM3Compilation o, u16 i_ڈھیر_اشاریہ)
{
    u16 بنیاد_خانہ = Get_خانہ_For_ڈھیر_اشاریہ(o, i_ڈھیر_اشاریہ);

    اگر (بنیاد_خانہ != c_خانہ_غیر_مستعمل) {
        u16 extra_خانہ = Get_Type_عدد_Slots(Get_ڈھیر_Type_From_Bottom(o, i_ڈھیر_اشاریہ)) - 1;
        بنیاد_خانہ += extra_خانہ;
    }

    واپس بنیاد_خانہ;
}


ساکن اندرونی
خالی Touch_خانہ (IM3Compilation o, u16 i_خانہ)
{
    // op_Entry uses this value to track and detect stack overflow
    o->max_ڈھیر_Slots = M3_MAX(o->max_ڈھیر_Slots, i_خانہ + 1);
}

ساکن اندرونی
خالی Mark_خانہ_Allocated (IM3Compilation o, u16 i_خانہ)
{                                                                   d_m3Assert (o->m3Slots [i_خانہ] == 0); // shouldn't be already allocated
    o->m3Slots[i_خانہ] = 1;

    o->خانہ_Max_Allocated_اشاریہ_Plus_One = M3_MAX(o->خانہ_Max_Allocated_اشاریہ_Plus_One, i_خانہ + 1);

    Touch_خانہ(o, i_خانہ);
}

ساکن اندرونی
M3_نتیجہ MarkSlotsAllocated (IM3Compilation o, u16 i_خانہ, u16 i_عدد_Slots)
{
    اگر (M3_UNLIKELY(i_خانہ + i_عدد_Slots > d_m3MaxFunctionSlots)) {
        واپس m3_Err_function_ڈھیر_حد_سے_تجاوز;
    }

    جب (i_عدد_Slots--) {
        Mark_خانہ_Allocated(o, i_خانہ++);
    }

    واپس m3Err_none;
}

ساکن اندرونی
M3_نتیجہ MarkSlotsAllocatedByType (IM3Compilation o, u16 i_خانہ, m3type_t i_type)
{
    u16 عدد_Slots = Get_Type_عدد_Slots(i_type);
    واپس MarkSlotsAllocated(o, i_خانہ, عدد_Slots);
}


ساکن
M3_نتیجہ AllocateSlotsWithinRange (IM3Compilation o, u16* o_خانہ, m3type_t i_type, u16 i_آغاز_خانہ, u16 i_انتہا_خانہ)
{
    M3_نتیجہ نتیجہ = m3_Err_function_ڈھیر_حد_سے_تجاوز;

    u16 عدد_Slots     = Get_Type_عدد_Slots(i_type);
    u16 تلاش_انحراف = عدد_Slots - 1;

    Align_خانہ_مقصد_Type(&i_آغاز_خانہ, i_type);

    // search for 1 or 2 consecutive slots in the execution stack
    u16 i = i_آغاز_خانہ;
    جب (i + تلاش_انحراف < i_انتہا_خانہ) {
        اگر (i + تلاش_انحراف < d_m3MaxFunctionSlots and o->m3Slots[i] == 0 and o->m3Slots[i + تلاش_انحراف] == 0) {
            MarkSlotsAllocated(o, i, عدد_Slots);

            *o_خانہ = i;
            نتیجہ  = m3Err_none;
            توڑو;
        }

        // keep 2-slot allocations even-aligned
        i += عدد_Slots;
    }

    واپس نتیجہ;
}

ساکن اندرونی
M3_نتیجہ AllocateSlots (IM3Compilation o, u16* o_خانہ, m3type_t i_type)
{
    واپس AllocateSlotsWithinRange(o, o_خانہ, i_type, o->خانہ_پہلا_Dynamic_اشاریہ, d_m3MaxFunctionSlots);
}

ساکن اندرونی
M3_نتیجہ AllocateConstantSlots (IM3Compilation o, u16* o_خانہ, m3type_t i_type)
{
    u16 max_جدول_اشاریہ = o->خانہ_پہلا_Const_اشاریہ + d_m3_Max_Constant_جدول_سائز;
    واپس AllocateSlotsWithinRange(o, o_خانہ, i_type, o->خانہ_پہلا_Const_اشاریہ, M3_MIN(o->خانہ_پہلا_Dynamic_اشاریہ, max_جدول_اشاریہ));
}


// TOQUE: this usage count system could be eliminated. real world code doesn't frequently trigger it.  just copy to multiple
// unique slots.
ساکن اندرونی
M3_نتیجہ Increment_خانہ_Usage_گنتی (IM3Compilation o, u16 i_خانہ)
{                                                                                       d_m3Assert (i_خانہ < d_m3MaxFunctionSlots);
    M3_نتیجہ نتیجہ = m3Err_none;                                                       d_m3Assert (o->m3Slots [i_خانہ] > 0);

    // OPTZ (memory): 'm3Slots' could still be fused with 'typeStack' if 4 bits were used to indicate: [0,1,2,many]. The many-case
    // would scan 'wasmStack' to determine the actual usage count
    اگر (M3_LIKELY(o->m3Slots[i_خانہ] < 0xFF)) {
        o->m3Slots[i_خانہ]++;
    } ورنہ {
        نتیجہ = "slot usage count overflow";
    }

    واپس نتیجہ;
}

ساکن اندرونی
خالی Deallocate_خانہ (IM3Compilation o, i16 i_خانہ, m3type_t i_type)
{                                                                                       d_m3Assert (i_خانہ >= o->خانہ_پہلا_Dynamic_اشاریہ);
                                                                                        d_m3Assert (i_خانہ < o->خانہ_Max_Allocated_اشاریہ_Plus_One);
    برائے (u16 i = 0; i < Get_Type_عدد_Slots(i_type); ++i, ++i_خانہ) {                       d_m3Assert (o->m3Slots [i_خانہ]);
        --o->m3Slots[i_خانہ];
    }
}


ساکن اندرونی
bool Is_رجسٹر_Type_Allocated (IM3Compilation o, u8 i_type)
{
    واپس Is_رجسٹر_Allocated(o, IsFpType(i_type));
}

ساکن اندرونی
خالی Allocate_رجسٹر (IM3Compilation o, u32 i_رجسٹر, u16 i_ڈھیر_اشاریہ)
{                                                                                       d_m3Assert (not Is_رجسٹر_Allocated (o, i_رجسٹر));
    o->reg_ڈھیر_اشاریہ_Plus_One[i_رجسٹر] = i_ڈھیر_اشاریہ + 1;
}

ساکن اندرونی
خالی Deallocate_رجسٹر (IM3Compilation o, u32 i_رجسٹر)
{                                                                                       d_m3Assert (Is_رجسٹر_Allocated (o, i_رجسٹر));
    o->reg_ڈھیر_اشاریہ_Plus_One[i_رجسٹر] = c_m3_رجسٹر_Unallocated;
}

ساکن اندرونی
u16 Get_رجسٹر_ڈھیر_اشاریہ (IM3Compilation o, u32 i_رجسٹر)
{                                                                                       d_m3Assert (Is_رجسٹر_Allocated (o, i_رجسٹر));
    واپس o->reg_ڈھیر_اشاریہ_Plus_One[i_رجسٹر] - 1;
}

u16 Get_Max_مستعمل_خانہ_Plus_One (IM3Compilation o)
{
    جب (o->خانہ_Max_Allocated_اشاریہ_Plus_One > o->خانہ_پہلا_Dynamic_اشاریہ) {
        اگر (Is_خانہ_Allocated(o, o->خانہ_Max_Allocated_اشاریہ_Plus_One - 1)) {
            توڑو;
        }

        o->خانہ_Max_Allocated_اشاریہ_Plus_One--;
    }

#اگر_تعریف_شدہ DEBUG
    u16 max_خانہ = o->خانہ_Max_Allocated_اشاریہ_Plus_One;
    جب (max_خانہ < d_m3MaxFunctionSlots) {
        d_m3Assert(o->m3Slots[max_خانہ] == 0);
        max_خانہ++;
    }
#اگر_ختم

    واپس o->خانہ_Max_Allocated_اشاریہ_Plus_One;
}

ساکن
M3_نتیجہ Preserve_رجسٹر_If_Occupied (IM3Compilation o, m3type_t i_رجسٹر_Type)
{
    M3_نتیجہ نتیجہ = m3Err_none;

    u32 regSelect = IsFpType(i_رجسٹر_Type);

    اگر (Is_رجسٹر_Allocated(o, regSelect)) {
        u16 ڈھیر_اشاریہ = Get_رجسٹر_ڈھیر_اشاریہ(o, regSelect);
        Deallocate_رجسٹر(o, regSelect);

        m3type_t type = Get_ڈھیر_Type_From_Bottom(o, ڈھیر_اشاریہ);

        // and point to a exec slot
        u16 خانہ = c_خانہ_غیر_مستعمل;
_       (AllocateSlots(o, &خانہ, type));
        o->wasm_ڈھیر[ڈھیر_اشاریہ] = خانہ;

        // Ensure type is within the valid range
        اگر (M3_LIKELY(بنیاد_Type_کا(type) < c_m3_Type_گنتی)) {
_           (EmitOp(o, c_سیٹ_سیٹ_Ops[بنیاد_Type_کا(type)]));
        } ورنہ {
            _throw(m3Err_unknownType);
        }

        Emit_خانہ_انحراف(o, خانہ);
    }

    _catch: واپس نتیجہ;
}


// all values must be in slots before entering loop, if, and else blocks
// otherwise they'd end up preserve-copied in the block to probably different locations (if/else)
ساکن اندرونی
M3_نتیجہ PreserveRegisters (IM3Compilation o)
{
    M3_نتیجہ نتیجہ;

_   (Preserve_رجسٹر_If_Occupied(o, c_m3Type_f64));
_   (Preserve_رجسٹر_If_Occupied(o, c_m3Type_i64));

    _catch: واپس نتیجہ;
}

ساکن
M3_نتیجہ Preserve_Non_چوٹی_Registers (IM3Compilation o)
{
    M3_نتیجہ نتیجہ = m3Err_none;

    i16 ڈھیر_چوٹی = Get_ڈھیر_چوٹی_اشاریہ(o);

    اگر (ڈھیر_چوٹی >= 0) {
        اگر (Is_رجسٹر_Allocated(o, 0))     // r0
        {
            اگر (Get_رجسٹر_ڈھیر_اشاریہ(o, 0) != ڈھیر_چوٹی) {
_               (Preserve_رجسٹر_If_Occupied(o, c_m3Type_i64));
            }
        }

        اگر (Is_رجسٹر_Allocated(o, 1))     // fp0
        {
            اگر (Get_رجسٹر_ڈھیر_اشاریہ(o, 1) != ڈھیر_چوٹی) {
_               (Preserve_رجسٹر_If_Occupied(o, c_m3Type_f64));
            }
        }
    }

    _catch: واپس نتیجہ;
}


//----------------------------------------------------------------------------------------------------------------------

ساکن
M3_نتیجہ ڈالو (IM3Compilation o, m3type_t i_type, u16 i_خانہ)
{
    M3_نتیجہ نتیجہ = m3Err_none;

#اگر !d_m3HasFloat
    اگر (i_type == c_m3Type_f32 || i_type == c_m3Type_f64) {
        واپس m3Err_unknownOpcode;
    }
#اگر_ختم

    اگر (M3_UNLIKELY(o->ڈھیر_اشاریہ >= d_m3_Max_Function_ڈھیر_اونچائی)) {
        واپس m3_Err_function_ڈھیر_حد_سے_تجاوز;
    }

    u16 ڈھیر_اشاریہ = o->ڈھیر_اشاریہ++;                                       // printf ("push: %d\n", (i32) i);

    o->wasm_ڈھیر[ڈھیر_اشاریہ] = i_خانہ;
    o->type_ڈھیر[ڈھیر_اشاریہ] = i_type;

    اگر (Is_رجسٹر_خانہ_Alias(i_خانہ)) {
        u32 regSelect = Is_Fp_رجسٹر_خانہ_Alias(i_خانہ);
        Allocate_رجسٹر(o, regSelect, ڈھیر_اشاریہ);
    }

    اگر (d_m3_Log_Wasm_ڈھیر) {
        dump_type_ڈھیر(o);
    }

    واپس نتیجہ;
}

ساکن اندرونی
M3_نتیجہ ڈالو_رجسٹر (IM3Compilation o, m3type_t i_type)
{
    M3_نتیجہ نتیجہ = m3Err_none;                                                       d_m3Assert ((u16) d_m3_Reg0_خانہ_Alias > (u16) d_m3MaxFunctionSlots);
    u16      خانہ   = IsFpType(i_type) ? d_m3_Fp0_خانہ_Alias : d_m3_Reg0_خانہ_Alias;          d_m3Assert (i_type or Is_ڈھیر_Polymorphic (o));

_   (ڈالو(o, i_type, خانہ));

    _catch: واپس نتیجہ;
}

ساکن
M3_نتیجہ نکالو (IM3Compilation o)
{
    M3_نتیجہ نتیجہ = m3Err_none;

    اگر (M3_LIKELY(o->ڈھیر_اشاریہ > o->بلاک.بلاک_ڈھیر_اشاریہ)) {
        o->ڈھیر_اشاریہ--;                                                //  printf ("pop: %d\n", (i32) o->stackIndex);

        u16      خانہ = o->wasm_ڈھیر[o->ڈھیر_اشاریہ];
        m3type_t type = o->type_ڈھیر[o->ڈھیر_اشاریہ];

        اگر (Is_رجسٹر_خانہ_Alias(خانہ)) {
            u32 regSelect = Is_Fp_رجسٹر_خانہ_Alias(خانہ);
            Deallocate_رجسٹر(o, regSelect);
        } ورنہ اگر (M3_UNLIKELY(خانہ >= o->خانہ_Max_Allocated_اشاریہ_Plus_One)) {
            واپس m3_Err_function_ڈھیر_Underrun; // Return error for invalid slot indices
        } ورنہ اگر (خانہ >= o->خانہ_پہلا_Dynamic_اشاریہ) {
            Deallocate_خانہ(o, خانہ, type);
        }
    } ورنہ اگر (not Is_ڈھیر_Polymorphic(o)) {
        نتیجہ = m3_Err_function_ڈھیر_Underrun;
    }

    واپس نتیجہ;
}

ساکن
M3_نتیجہ نکالو_Type (IM3Compilation o, m3type_t i_type)
{
    M3_نتیجہ نتیجہ = m3Err_none;

    m3type_t چوٹی_Type = Get_ڈھیر_چوٹی_Type(o);

    اگر (M3_LIKELY(Is_Sub_Type_کا(چوٹی_Type, i_type) or o->بلاک.isPolymorphic)) {
_       (نکالو(o));
    } ورنہ {
        _throw(m3Err_typeMismatch);
    }

    _catch:
    واپس نتیجہ;
}

ساکن
M3_نتیجہ ڈالو_Allocated_خانہ_And_Emit (IM3Compilation o, m3type_t i_type, bool i_doEmit)
{
    M3_نتیجہ نتیجہ = m3Err_none;

    u16 خانہ = c_خانہ_غیر_مستعمل;

_   (AllocateSlots(o, &خانہ, i_type));
_   (ڈالو(o, i_type, خانہ));

    اگر (i_doEmit) {
        Emit_خانہ_انحراف(o, خانہ);
    }

    //    printf ("push: %d\n", (u32) slot);

    _catch: واپس نتیجہ;
}

ساکن اندرونی
M3_نتیجہ ڈالو_Allocated_خانہ_And_Emit_2 (IM3Compilation o, m3type_t i_type)
{
    واپس ڈالو_Allocated_خانہ_And_Emit(o, i_type, true);
}

ساکن اندرونی
M3_نتیجہ ڈالو_Allocated_خانہ (IM3Compilation o, m3type_t i_type)
{
    واپس ڈالو_Allocated_خانہ_And_Emit(o, i_type, false);
}

ساکن
M3_نتیجہ ڈالو_Const (IM3Compilation o, u64 i_لفظ, m3type_t i_type)
{
    M3_نتیجہ نتیجہ = m3Err_none;

    // When compile-walking a constant expression without emitting there is no
    // constant table to place the value in, but the type still has to land on
    // the stack: extended-const arithmetic downstream consumes it.
    اگر (M3_UNLIKELY(!o->صفحہ)) {
        واپس ڈالو_Allocated_خانہ(o, i_type);
    }

    bool مطابقت_ملا  = false;
    bool is64_ثنائی_ہندسہ_Type = Is64_ثنائی_ہندسہ_Type(i_type);

    u16 عدد_Required_Slots  = Get_Type_عدد_Slots(i_type);
    u16 عدد_مستعمل_Const_Slots = o->خانہ_Max_Const_اشاریہ - o->خانہ_پہلا_Const_اشاریہ;

    // search for duplicate matching constant slot to reuse
    اگر (عدد_Required_Slots == 2 and عدد_مستعمل_Const_Slots >= 2) {
        u16 پہلا_Const_خانہ = o->خانہ_پہلا_Const_اشاریہ;
        Align_خانہ_مقصد_Type(&پہلا_Const_خانہ, c_m3Type_i64);

        برائے (u16 خانہ = پہلا_Const_خانہ; خانہ < o->خانہ_Max_Const_اشاریہ - 1; خانہ += 2) {
            اگر (Is_خانہ_Allocated(o, خانہ) and Is_خانہ_Allocated(o, خانہ + 1)) {
                u64 constant;
                حافظہ_نقل(&constant, &o->constants[خانہ - o->خانہ_پہلا_Const_اشاریہ], حجم(constant));

                اگر (constant == i_لفظ) {
                    مطابقت_ملا = true;
_                   (ڈالو(o, i_type, خانہ));
                    توڑو;
                }
            }
        }
    } ورنہ اگر (عدد_Required_Slots == 1) {
        برائے (u16 i = 0; i < عدد_مستعمل_Const_Slots; ++i) {
            u16 خانہ = o->خانہ_پہلا_Const_اشاریہ + i;

            اگر (Is_خانہ_Allocated(o, خانہ)) {
                bool matches;
                اگر (is64_ثنائی_ہندسہ_Type) {
                    u64 constant;
                    حافظہ_نقل(&constant, &o->constants[i], حجم(constant));
                    matches = (constant == i_لفظ);
                } ورنہ {
                    u32 constant;
                    حافظہ_نقل(&constant, &o->constants[i], حجم(constant));
                    matches = (constant == i_لفظ);
                }
                اگر (matches) {
                    مطابقت_ملا = true;
_                   (ڈالو(o, i_type, خانہ));
                    توڑو;
                }
            }
        }
    }

    اگر (not مطابقت_ملا) {
        u16 خانہ = c_خانہ_غیر_مستعمل;
        نتیجہ   = AllocateConstantSlots(o, &خانہ, i_type);

        اگر (نتیجہ || خانہ == c_خانہ_غیر_مستعمل) // no more constant table space; use inline constants
        {
            نتیجہ = m3Err_none;

            اگر (is64_ثنائی_ہندسہ_Type) {
_               (EmitOp(o, op_Const64));
                EmitWord64(o->صفحہ, i_لفظ);
            } ورنہ {
_               (EmitOp(o, op_Const32));
                EmitWord32(o->صفحہ, (u32)i_لفظ);
            }

_           (ڈالو_Allocated_خانہ_And_Emit_2(o, i_type));
        } ورنہ {
            u16 const_جدول_اشاریہ = خانہ - o->خانہ_پہلا_Const_اشاریہ;

            d_m3Assert(const_جدول_اشاریہ < d_m3_Max_Constant_جدول_سائز);

            اگر (is64_ثنائی_ہندسہ_Type) {
                حافظہ_نقل(&o->constants[const_جدول_اشاریہ], &i_لفظ, حجم(i_لفظ));
            } ورنہ {
                u32 word32 = (u32)i_لفظ;
                حافظہ_نقل(&o->constants[const_جدول_اشاریہ], &word32, حجم(word32));
            }

_           (ڈالو(o, i_type, خانہ));

            o->خانہ_Max_Const_اشاریہ = M3_MAX(خانہ + عدد_Required_Slots, o->خانہ_Max_Const_اشاریہ);
        }
    }

    _catch: واپس نتیجہ;
}

ساکن اندرونی
M3_نتیجہ Emit_خانہ_عدد_کا_ڈھیر_چوٹی_And_نکالو (IM3Compilation o)
{
    // no emit if value is in register
    اگر (Is_ڈھیر_چوٹی_اندر_خانہ(o)) {
        Emit_خانہ_انحراف(o, Get_ڈھیر_چوٹی_خانہ_نمبر(o));
    }

    واپس نکالو(o);
}


// Or, maybe: EmitTrappingOp
M3_نتیجہ جوڑو_Trap_ریکارڈ (IM3Compilation o)
{
    M3_نتیجہ نتیجہ = m3Err_none;

    اگر (o->function) {
    }

    واپس نتیجہ;
}

ساکن
M3_نتیجہ Unwind_بلاک_ڈھیر (IM3Compilation o)
{
    M3_نتیجہ نتیجہ = m3Err_none;

    u32 نکالو_گنتی = 0;
    جب (o->ڈھیر_اشاریہ > o->بلاک.بلاک_ڈھیر_اشاریہ) {
_       (نکالو(o));
        ++نکالو_گنتی;
    }

    اگر (نکالو_گنتی) {
        m3log(compile, "unwound stack top: %d", نکالو_گنتی);
    }

    _catch: واپس نتیجہ;
}

ساکن اندرونی
M3_نتیجہ سیٹ_ڈھیر_Polymorphic (IM3Compilation o)
{
    o->بلاک.isPolymorphic = true;                              m3log (compile, "stack set polymorphic");
    واپس Unwind_بلاک_ڈھیر(o);
}

ساکن
خالی PatchBranches (IM3Compilation o)
{
    InvalidateFold(o);         // patched branches land at the current pc

    pc_t pc = GetPC(o);

    pc_t patches     = o->بلاک.patches;
    o->بلاک.patches = تہی;

    جب (patches) {                                           m3log (compile, "patching location: %p to pc: %p", patches, pc);
        pc_t اگلا       = *(pc_t*)patches;
        *(pc_t*)patches = pc;
        patches         = اگلا;
    }
}

//-------------------------------------------------------------------------------------------------------------------------

ساکن
M3_نتیجہ Copy_ڈھیر_اشاریہ_مقصد_خانہ (IM3Compilation o, u16 i_dest_خانہ, u16 i_ڈھیر_اشاریہ)  // NoPushPop
{
    M3_نتیجہ نتیجہ = m3Err_none;

    IM3Operation op;

    m3type_t type       = Get_ڈھیر_Type_From_Bottom(o, i_ڈھیر_اشاریہ);
    bool     اندر_رجسٹر = Is_ڈھیر_اشاریہ_اندر_رجسٹر(o, i_ڈھیر_اشاریہ);

    اگر (اندر_رجسٹر) {
        op = c_سیٹ_سیٹ_Ops[بنیاد_Type_کا(type)];
    } ورنہ {
        op = Is64_ثنائی_ہندسہ_Type(type) ? op_Copy_خانہ_64 : op_Copy_خانہ_32;
    }

_   (EmitOp(o, op));
    Emit_خانہ_انحراف(o, i_dest_خانہ);

    اگر (not اندر_رجسٹر) {
        u16 src_خانہ = Get_خانہ_For_ڈھیر_اشاریہ(o, i_ڈھیر_اشاریہ);
        Emit_خانہ_انحراف(o, src_خانہ);
    }

    _catch: واپس نتیجہ;
}

ساکن
M3_نتیجہ Copy_ڈھیر_چوٹی_مقصد_خانہ (IM3Compilation o, u16 i_dest_خانہ)  // NoPushPop
{
    M3_نتیجہ نتیجہ;

    i16 ڈھیر_چوٹی;
_   (Get_ڈھیر_چوٹی_اشاریہ_Throws(o, &ڈھیر_چوٹی));
_   (Copy_ڈھیر_اشاریہ_مقصد_خانہ(o, i_dest_خانہ, (u16)ڈھیر_چوٹی));

    _catch: واپس نتیجہ;
}


// a copy-on-write strategy is used with locals. when a get local occurs, it's not copied anywhere. the stack
// entry just has a index pointer to that local memory slot.
// then, when a previously referenced local is set, the current value needs to be preserved for those references

// TODO: consider getting rid of these specialized operations: PreserveSetSlot & PreserveCopySlot.
// They likely just take up space (which seems to reduce performance) without improving performance.
// => unconfirmed in preliminary testing.
ساکن
M3_نتیجہ Preserved_Copy_چوٹی_خانہ (IM3Compilation o, u16 i_dest_خانہ, u16 i_preserve_خانہ)
{
    M3_نتیجہ نتیجہ = m3Err_none;             d_m3Assert (i_dest_خانہ != i_preserve_خانہ);

    IM3Operation op;

    m3type_t type = Get_ڈھیر_چوٹی_Type(o);

    اگر (Is_ڈھیر_چوٹی_اندر_رجسٹر(o)) {
        op = c_preserve_سیٹ_خانہ[بنیاد_Type_کا(type)];
    } ورنہ {
        op = Is64_ثنائی_ہندسہ_Type(type) ? op_Preserve_Copy_خانہ_64 : op_Preserve_Copy_خانہ_32;
    }

_   (EmitOp(o, op));
    Emit_خانہ_انحراف(o, i_dest_خانہ);

    اگر (Is_ڈھیر_چوٹی_اندر_خانہ(o)) {
        Emit_خانہ_انحراف(o, Get_ڈھیر_چوٹی_خانہ_نمبر(o));
    }

    Emit_خانہ_انحراف(o, i_preserve_خانہ);

    _catch: واپس نتیجہ;
}

ساکن
M3_نتیجہ Copy_ڈھیر_چوٹی_مقصد_رجسٹر (IM3Compilation o, bool i_update_ڈھیر)
{
    M3_نتیجہ نتیجہ = m3Err_none;

    اگر (Is_ڈھیر_چوٹی_اندر_خانہ(o)) {
        m3type_t type = Get_ڈھیر_چوٹی_Type(o);

_       (Preserve_رجسٹر_If_Occupied(o, type));

        IM3Operation op = c_سیٹ_رجسٹر_Ops[بنیاد_Type_کا(type)];

_       (EmitOp(o, op));
        Emit_خانہ_انحراف(o, Get_ڈھیر_چوٹی_خانہ_نمبر(o));

        اگر (i_update_ڈھیر) {
_           (نکالو_Type(o, type));
_           (ڈالو_رجسٹر(o, type));
        }
    }

    _catch: واپس نتیجہ;
}


// if local is unreferenced, o_preservedSlotNumber will be equal to localIndex on return
ساکن
M3_نتیجہ ڈھونڈو_Referenced_Local_Within_Current_بلاک (IM3Compilation o, u16* o_preserved_خانہ_نمبر, u32 i_local_خانہ)
{
    M3_نتیجہ نتیجہ = m3Err_none;

    IM3CompilationScope scope = &o->بلاک;

    u16 آغاز_اشاریہ = scope->بلاک_ڈھیر_اشاریہ;

    جب (scope->opcode == c_wa_Op_بلاک) {
        scope = scope->outer;
        اگر (not scope) {
            توڑو;
        }

        آغاز_اشاریہ = scope->بلاک_ڈھیر_اشاریہ;
    }

    *o_preserved_خانہ_نمبر = (u16)i_local_خانہ;

    برائے (u32 i = آغاز_اشاریہ; i < o->ڈھیر_اشاریہ; ++i) {
        اگر (o->wasm_ڈھیر[i] == i_local_خانہ) {
            m3type_t type = Get_ڈھیر_Type_From_Bottom(o, i);                         d_m3Assert (type != c_m3Type_none)

            اگر (*o_preserved_خانہ_نمبر == i_local_خانہ) {
_               (AllocateSlots(o, o_preserved_خانہ_نمبر, type));
            } ورنہ {
                // A 64-bit value spans two slots, and allocating or releasing it
                // counts both, so a second reference has to count both as well.
                برائے (u16 s = 0; s < Get_Type_عدد_Slots(type); ++s) {
_                   (Increment_خانہ_Usage_گنتی(o, (u16)(*o_preserved_خانہ_نمبر + s)));
                }
            }

            o->wasm_ڈھیر[i] = *o_preserved_خانہ_نمبر;
        }
    }

    _catch: واپس نتیجہ;
}

ساکن
M3_نتیجہ Get_بلاک_Scope (IM3Compilation o, IM3CompilationScope* o_scope, u32 i_گہرائی)
{
    M3_نتیجہ نتیجہ = m3Err_none;

    IM3CompilationScope scope = &o->بلاک;

    جب (i_گہرائی--) {
        scope = scope->outer;
        _throwif("invalid block depth", not scope);
    }

    *o_scope = scope;

    _catch:
    واپس نتیجہ;
}

ساکن
M3_نتیجہ Copy_ڈھیر_Slots_R (IM3Compilation o, u16 i_ہدف_خانہ_ڈھیر_اشاریہ, u16 i_ڈھیر_اشاریہ, u16 i_انتہا_ڈھیر_اشاریہ, u16 i_عارضی_خانہ)
{
    M3_نتیجہ نتیجہ = m3Err_none;

    اگر (i_ڈھیر_اشاریہ < i_انتہا_ڈھیر_اشاریہ) {
        u16 src_خانہ = Get_خانہ_For_ڈھیر_اشاریہ(o, i_ڈھیر_اشاریہ);

        m3type_t type      = Get_ڈھیر_Type_From_Bottom(o, i_ڈھیر_اشاریہ);
        u16      عدد_Slots  = Get_Type_عدد_Slots(type);
        u16      extra_خانہ = عدد_Slots - 1;

        u16 ہدف_خانہ = Get_خانہ_For_ڈھیر_اشاریہ(o, i_ہدف_خانہ_ڈھیر_اشاریہ);

        u16 preserve_اشاریہ = i_ڈھیر_اشاریہ;
        u16 collision_خانہ = src_خانہ;

        اگر (ہدف_خانہ != src_خانہ) {
            // search for collisions
            u16 check_اشاریہ = i_ڈھیر_اشاریہ + 1;
            جب (check_اشاریہ < i_انتہا_ڈھیر_اشاریہ) {
                u16 otherSlot1 = Get_خانہ_For_ڈھیر_اشاریہ(o, check_اشاریہ);
                u16 otherSlot2 = Get_Extra_خانہ_For_ڈھیر_اشاریہ(o, check_اشاریہ);

                اگر (ہدف_خانہ == otherSlot1 or
                    ہدف_خانہ == otherSlot2 or
                    ہدف_خانہ + extra_خانہ == otherSlot1) {
                    _throwif(m3_Err_function_ڈھیر_حد_سے_تجاوز, i_عارضی_خانہ >= d_m3MaxFunctionSlots);

_                   (Copy_ڈھیر_اشاریہ_مقصد_خانہ(o, i_عارضی_خانہ, check_اشاریہ));
                    o->wasm_ڈھیر[check_اشاریہ] = i_عارضی_خانہ;
                    i_عارضی_خانہ += Get_Type_عدد_Slots(c_m3Type_i64);
                    Touch_خانہ(o, i_عارضی_خانہ - 1);

                    // restore this on the way back down
                    preserve_اشاریہ = check_اشاریہ;
                    collision_خانہ = otherSlot1;

                    توڑو;
                }

                ++check_اشاریہ;
            }

_           (Copy_ڈھیر_اشاریہ_مقصد_خانہ(o, ہدف_خانہ, i_ڈھیر_اشاریہ));                                                m3log (compile, " copying slot: %d to slot: %d", src_خانہ, ہدف_خانہ);
            o->wasm_ڈھیر[i_ڈھیر_اشاریہ] = ہدف_خانہ;
        }

_       (Copy_ڈھیر_Slots_R(o, i_ہدف_خانہ_ڈھیر_اشاریہ + 1, i_ڈھیر_اشاریہ + 1, i_انتہا_ڈھیر_اشاریہ, i_عارضی_خانہ));

        // restore the stack state
        o->wasm_ڈھیر[i_ڈھیر_اشاریہ]  = src_خانہ;
        o->wasm_ڈھیر[preserve_اشاریہ] = collision_خانہ;
    }

    _catch:
    واپس نتیجہ;
}

ساکن
M3_نتیجہ Resolve_بلاک_Results (IM3Compilation o, IM3CompilationScope i_ہدف_بلاک, bool i_isBranch)
{
    M3_نتیجہ نتیجہ = m3Err_none;
    اگر (d_m3_Log_Wasm_ڈھیر) {
        dump_type_ڈھیر(o);
    }

    bool isLoop = (i_ہدف_بلاک->opcode == c_waOp_loop and i_isBranch);

    u16 عدد_Params  = Get_Func_Type_عدد_Params(i_ہدف_بلاک->type);
    u16 عدد_Results = Get_Func_Type_عدد_Results(i_ہدف_بلاک->type);

    u16 خانہ_اندراجات = i_ہدف_بلاک->exit_ڈھیر_اشاریہ;

    u16 عدد_اقدار;

    اگر (not isLoop) {
        عدد_اقدار = عدد_Results;
        خانہ_اندراجات += عدد_Params;
    } ورنہ {
        عدد_اقدار = عدد_Params;
    }

    u16 بلاک_اونچائی = Get_عدد_بلاک_اقدار_On_ڈھیر(o);

    _throwif(m3_Err_type_گنتی_Mismatch, i_isBranch ? (بلاک_اونچائی < عدد_اقدار) : (بلاک_اونچائی != عدد_اقدار));

    اگر (عدد_اقدار) {
        u16 انتہا_اشاریہ     = Get_ڈھیر_چوٹی_اشاریہ(o) + 1;
        u16 عدد_Rem_اقدار = عدد_اقدار;

        // The last result is taken from _fp0. See PushBlockResults.
        اگر (not isLoop and IsFpType(Get_ڈھیر_چوٹی_Type(o))) {
_           (Copy_ڈھیر_چوٹی_مقصد_رجسٹر(o, false));
            --انتہا_اشاریہ;
            --عدد_Rem_اقدار;
        }

        // tempslot affects maxStackSlots, so can grow unnecess each time.
        // in practice, this is hardly ever hit.
        u16 عارضی_خانہ = o->max_ڈھیر_Slots;// GetMaxUsedSlotPlusOne (o); doesn't work cause can collide with slotRecords
        Align_خانہ_مقصد_Type(&عارضی_خانہ, c_m3Type_i64);

_       (Copy_ڈھیر_Slots_R(o, خانہ_اندراجات, انتہا_اشاریہ - عدد_Rem_اقدار, انتہا_اشاریہ, عارضی_خانہ));

        اگر (d_m3_Log_Wasm_ڈھیر) {
            dump_type_ڈھیر(o);
        }
    }

    _catch: واپس نتیجہ;
}


ساکن
M3_نتیجہ Return_اقدار (IM3Compilation o, IM3CompilationScope i_function_بلاک, bool i_isBranch)
{
    M3_نتیجہ نتیجہ = m3Err_none;
    اگر (d_m3_Log_Wasm_ڈھیر) {
        dump_type_ڈھیر(o);
    }

    u16 عدد_Returns  = Get_Func_Type_عدد_Results(i_function_بلاک->type);     // could just o->function too...
    u16 بلاک_اونچائی = Get_عدد_بلاک_اقدار_On_ڈھیر(o);

    اگر (not Is_ڈھیر_Polymorphic(o)) {
        _throwif(m3_Err_type_گنتی_Mismatch, i_isBranch ? (بلاک_اونچائی < عدد_Returns) : (بلاک_اونچائی != عدد_Returns));
    }

    اگر (عدد_Returns) {
        // return slots like args are 64-bit aligned
        u16 return_خانہ = عدد_Returns * c_io_خانہ_گنتی;
        u16 ڈھیر_چوٹی   = Get_ڈھیر_چوٹی_اشاریہ(o);

        برائے (u16 i = 0; i < عدد_Returns; ++i) {
            m3type_t returnType = Get_Func_Type_نتیجہ_Type(i_function_بلاک->type, عدد_Returns - 1 - i);

            m3type_t ڈھیر_Type = Get_ڈھیر_Type_From_چوٹی(o, i);  // using FromTop so that only dynamic items are checked

            اگر (Is_ڈھیر_Polymorphic(o) and ڈھیر_Type == c_m3Type_none) {
                ڈھیر_Type = returnType;
            }

            _throwif(m3Err_typeMismatch, not Is_Sub_Type_کا(ڈھیر_Type, returnType));

            اگر (not Is_ڈھیر_Polymorphic(o)) {
                return_خانہ -= c_io_خانہ_گنتی;
_               (Copy_ڈھیر_اشاریہ_مقصد_خانہ(o, return_خانہ, ڈھیر_چوٹی--));
            }
        }

        اگر (not i_isBranch) {
            جب (عدد_Returns--) {
_               (نکالو(o));
            }
        }
    }

    _catch: واپس نتیجہ;
}


//-------------------------------------------------------------------------------------------------------------------------

ساکن
M3_نتیجہ Compile_Const_i32 (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_نتیجہ نتیجہ;

    i32 قدر;
_   (ReadLEB_i32(&قدر, &o->wasm, o->wasm_انتہا));
_   (ڈالو_Const(o, قدر, c_m3Type_i32));                        m3log (compile, d_indent " (const i32 = %" PRIi32 ")", get_indention_string (o), قدر);
    _catch: واپس نتیجہ;
}

ساکن
M3_نتیجہ Compile_Const_i64 (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_نتیجہ نتیجہ;

    i64 قدر;
_   (ReadLEB_i64(&قدر, &o->wasm, o->wasm_انتہا));
_   (ڈالو_Const(o, قدر, c_m3Type_i64));                        m3log (compile, d_indent " (const i64 = %" PRIi64 ")", get_indention_string (o), قدر);
    _catch: واپس نتیجہ;
}


#اگر d_m3ImplementFloat
ساکن
M3_نتیجہ Compile_Const_f32 (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_نتیجہ نتیجہ;

    اتحاد {
        u32 u;
        f32 f;
    } قدر = { 0 };

_   (Read_f32(&قدر.f, &o->wasm, o->wasm_انتہا));                 m3log (compile, d_indent " (const f32 = %" PRIf32 ")", get_indention_string (o), قدر.f);
_   (ڈالو_Const(o, قدر.u, c_m3Type_f32));

    _catch: واپس نتیجہ;
}

ساکن
M3_نتیجہ Compile_Const_f64 (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_نتیجہ نتیجہ;

    اتحاد {
        u64 u;
        f64 f;
    } قدر = { 0 };

_   (Read_f64(&قدر.f, &o->wasm, o->wasm_انتہا));                 m3log (compile, d_indent " (const f64 = %" PRIf64 ")", get_indention_string (o), قدر.f);
_   (ڈالو_Const(o, قدر.u, c_m3Type_f64));

    _catch: واپس نتیجہ;
}
#اگر_ختم

ساکن
M3_نتیجہ Compile_ExtendedOpcode (IM3Compilation o, m3opcode_t i_opcode)
{
_try {
    u32 opcode;
_   (ReadLEB_u32(&opcode, &o->wasm, o->wasm_انتہا));               m3log (compile, d_indent " (%s: %" PRIu32 ")", get_indention_string (o), (i_opcode == c_waOp_atomic) ? "FE" : "FC", opcode);

    // m3opcode_t carries only the low byte, so a wider sub-opcode is unknown
    // here rather than something to truncate into range
    _throwif(m3Err_unknownOpcode, opcode > (u32)((i_opcode == c_waOp_atomic) ? c_wa_Op_آخری_Atomic : c_wa_Op_آخری_Extended));

    i_opcode = (m3opcode_t)((i_opcode << 8) | opcode);

    //printf("Extended opcode: 0x%x\n", i_opcode);

    IM3OpInfo opInfo = GetOpInfo(i_opcode);
    _throwif(m3Err_unknownOpcode, not opInfo);

    M3Compiler compiler = GetCompiler(opInfo);
    _throwif(m3Err_noCompiler, not compiler);

_   ((*compiler)(o, i_opcode));

    o->پچھلا_Opcode = i_opcode;

    } _catch: واپس نتیجہ;
}

ساکن
M3_نتیجہ Compile_Return (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_نتیجہ نتیجہ = m3Err_none;

    اگر (not Is_ڈھیر_Polymorphic(o)) {
        IM3CompilationScope functionScope;
_       (Get_بلاک_Scope(o, &functionScope, o->بلاک.گہرائی));

_       (Return_اقدار(o, functionScope, true));

_       (EmitOp(o, op_Return));

_       (سیٹ_ڈھیر_Polymorphic(o));
    }

    _catch: واپس نتیجہ;
}

ساکن
M3_نتیجہ Validate_بلاک_انتہا (IM3Compilation o)
{
    M3_نتیجہ نتیجہ = m3Err_none;

    u16 عدد_Results  = Get_Func_Type_عدد_Results(o->بلاک.type);
    u16 بلاک_اونچائی = Get_عدد_بلاک_اقدار_On_ڈھیر(o);

    اگر (not Is_ڈھیر_Polymorphic(o)) {
        // Spec: at block end, stack height must match the number of results
        _throwif(m3_Err_type_گنتی_Mismatch, بلاک_اونچائی != عدد_Results);

        // Spec: result types must match expected types
        برائے (u16 i = 0; i < عدد_Results; ++i) {
            m3type_t expectedType = Get_Func_Type_نتیجہ_Type(o->بلاک.type, عدد_Results - 1 - i);
            m3type_t actualType   = Get_ڈھیر_Type_From_چوٹی(o, i);
            _throwif(m3Err_typeMismatch, not Is_Sub_Type_کا(actualType, expectedType));
        }
    }

    _catch: واپس نتیجہ;
}

ساکن
M3_نتیجہ Compile_انتہا (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_نتیجہ نتیجہ = m3Err_none;                   //dump_type_stack (o);

    // function end:
    اگر (o->بلاک.گہرائی == 0) {
        Validate_بلاک_انتہا(o);

        //      if (not IsStackPolymorphic (o))
        {
            // A constant expression has no function frame, but its value still
            // has to be copied down to slot 0, where EvaluateExpression reads it
            // back. The non-emitting init-expr walk is skipped: it has no block
            // type to return against.
            اگر (o->function or o->صفحہ) {
_               (Return_اقدار(o, &o->بلاک, false));
            }

_           (EmitOp(o, op_Return));
        }
    }

    _catch: واپس نتیجہ;
}


ساکن
M3_نتیجہ Compile_سیٹ_Local (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_نتیجہ نتیجہ;

    u32 local_اشاریہ;
_   (ReadLEB_u32(&local_اشاریہ, &o->wasm, o->wasm_انتہا));                           m3log (compile, d_indent " (index = %u)", get_indention_string (o), local_اشاریہ);

    اگر (local_اشاریہ < Get_Function_عدد_دلائل_And_Locals(o->function)) {
        // Spec: value type must match local type
        اگر (not Is_ڈھیر_Polymorphic(o)) {
            m3type_t localType    = Get_ڈھیر_Type_From_Bottom(o, local_اشاریہ);
            m3type_t ڈھیر_چوٹی_Type = Get_ڈھیر_چوٹی_Type(o);
            _throwif(m3Err_typeMismatch, ڈھیر_چوٹی_Type != c_m3Type_none and localType != c_m3Type_none and
                                           not Is_Sub_Type_کا(ڈھیر_چوٹی_Type, localType));
#اگر d_m3HasSnapshots
            اگر (o->local_Init_گہرائی and o->local_Init_گہرائی[local_اشاریہ] == UINT32_MAX) {
                o->local_Init_گہرائی[local_اشاریہ] = (u32)o->بلاک.گہرائی;
            }
#اگر_ختم
        }

        u16 local_خانہ = Get_خانہ_For_ڈھیر_اشاریہ(o, local_اشاریہ);

        u16 preserve_خانہ;
_       (ڈھونڈو_Referenced_Local_Within_Current_بلاک(o, &preserve_خانہ, local_خانہ));  // preserve will be different than local, if referenced

        bool folded = false;

#اگر d_m3_Fold_سیٹ_Local
        // destination folding: when the value was produced by the immediately preceding op (nothing
        // emitted since, result on top in _r0, no copy-on-write preservation needed), retro-patch the
        // producer to a variant that writes straight to the local's slot and skip the SetSlot op
        اگر (o->foldPatchPC and preserve_خانہ == local_خانہ and Is_ڈھیر_چوٹی_اندر_رجسٹر(o) and Get_ڈھیر_چوٹی_اشاریہ(o) == (i16)o->fold_ڈھیر_اشاریہ) {
            IM3Operation foldOp = GetFoldOp(o->foldOpcode, o->foldForm);

            اگر (foldOp) {
                *(IM3Operation*)o->foldPatchPC = foldOp;     // retro-patch the producer
                Emit_خانہ_انحراف(o, local_خانہ);                  // append its destination
                InvalidateFold(o);
                folded = true;
            }
        }
#اگر_ختم

        اگر (folded) {
            m3type_t type = Get_ڈھیر_چوٹی_Type(o);
_           (نکالو(o));                              // the value now lives in the local's slot

            // A folded op writes its result through to the register as well, so a tee
            // can keep it there. Pushing the slot instead would make the next use that
            // wants a register reload the slot this op just stored to.
            اگر (i_opcode == c_waOp_teeLocal) {
_               (ڈالو_رجسٹر(o, type));
            }
        } ورنہ {
            اگر (preserve_خانہ == local_خانہ) {
_               (Copy_ڈھیر_چوٹی_مقصد_خانہ(o, local_خانہ))
            } ورنہ {
_               (Preserved_Copy_چوٹی_خانہ(o, local_خانہ, preserve_خانہ))
            }

            اگر (i_opcode != c_waOp_teeLocal) {
_               (نکالو(o));
            }
        }
    } ورنہ {
        _throw("local index out of bounds");
    }

    _catch: واپس نتیجہ;
}

ساکن
M3_نتیجہ Compile_GetLocal (IM3Compilation o, m3opcode_t i_opcode)
{
_try {

    u32 local_اشاریہ;
_   (ReadLEB_u32(&local_اشاریہ, &o->wasm, o->wasm_انتہا));                           m3log (compile, d_indent " (index = %u)", get_indention_string (o), local_اشاریہ);

    اگر (local_اشاریہ >= Get_Function_عدد_دلائل_And_Locals(o->function)) {
        _throw("local index out of bounds");
    }

    m3type_t type = Get_ڈھیر_Type_From_Bottom(o, local_اشاریہ);
    u16      خانہ = Get_خانہ_For_ڈھیر_اشاریہ(o, local_اشاریہ);

_   (ڈالو(o, type, خانہ));

    } _catch: واپس نتیجہ;
}

ساکن
M3_نتیجہ Compile_GetGlobal (IM3Compilation o, M3Global* i_global)
{
    M3_نتیجہ نتیجہ;

    IM3Operation op = Is64_ثنائی_ہندسہ_Type(i_global->type) ? op_GetGlobal_s64 : op_GetGlobal_s32;
_   (EmitOp(o, op));
    Emit_اشارہ_گر(o, &i_global->i64_قدر);
_   (ڈالو_Allocated_خانہ_And_Emit_2(o, i_global->type));

    _catch: واپس نتیجہ;
}

ساکن
M3_نتیجہ Compile_سیٹ_Global (IM3Compilation o, M3Global* i_global)
{
    M3_نتیجہ نتیجہ = m3Err_none;

    اگر (i_global->isMutable) {
        // Spec: value type must match global type
        اگر (not Is_ڈھیر_Polymorphic(o)) {
            m3type_t ڈھیر_چوٹی_Type = Get_ڈھیر_چوٹی_Type(o);
            _throwif(m3Err_typeMismatch, ڈھیر_چوٹی_Type != c_m3Type_none and
                                           not Is_Sub_Type_کا(ڈھیر_چوٹی_Type, i_global->type));
        }

        IM3Operation op;
        m3type_t     type = Get_ڈھیر_چوٹی_Type(o);

        اگر (Is_ڈھیر_چوٹی_اندر_رجسٹر(o)) {
            op = c_سیٹ_Global_Ops[بنیاد_Type_کا(type)];
        } ورنہ {
            op = Is64_ثنائی_ہندسہ_Type(type) ? op_سیٹ_Global_s64 : op_سیٹ_Global_s32;
        }

_       (EmitOp(o, op));
        Emit_اشارہ_گر(o, &i_global->i64_قدر);

        اگر (Is_ڈھیر_چوٹی_اندر_خانہ(o)) {
            Emit_خانہ_انحراف(o, Get_ڈھیر_چوٹی_خانہ_نمبر(o));
        }

_       (نکالو(o));
    } ورنہ {
        _throw(m3Err_settingImmutableGlobal);
    }

    _catch: واپس نتیجہ;
}

ساکن
M3_نتیجہ Compile_Get_سیٹ_Global (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_نتیجہ نتیجہ = m3Err_none;

    u32 global_اشاریہ;
_   (ReadLEB_u32(&global_اشاریہ, &o->wasm, o->wasm_انتہا));

    اگر (global_اشاریہ < o->module->عدد_Globals) {
        اگر (o->module->globals) {
            M3Global* global = &o->module->globals[global_اشاریہ];

            // Spec: a constant expression may only read an imported immutable
            // global. The module's own globals are counted before their
            // initializer is walked, so a bare index check would let one
            // reference itself. These describe how the module declared the
            // global, so they are checked before following any link.
            _throwif(m3_Err_globa_اشاریہ_برآمد_کا_Bounds, o->is_Init_اظہاریہ and not global->imported);
            _throwif(m3Err_wasmMalformed, o->is_Init_اظہاریہ and global->isMutable);

            // an import linked to another module reads and writes that module's
            // cell, not the placeholder standing in for it here
            اگر (global->resolved) {
                global = global->resolved;
            }

_           ((i_opcode == c_waOp_getGlobal) ? Compile_GetGlobal(o, global) : Compile_سیٹ_Global(o, global));
        } ورنہ {
            _throw(ErrorCompile(m3_Err_global_حافظہ_Not_Allocated, o, "module '%s' is missing global memory", o->module->name));
        }
    } ورنہ {
        _throw(m3_Err_globa_اشاریہ_برآمد_کا_Bounds);
    }

    _catch: واپس نتیجہ;
}

ساکن
خالی Emit_Patching_Branch_اشارہ_گر (IM3Compilation o, IM3CompilationScope i_scope)
{
    pc_t patch       = Emit_اشارہ_گر(o, i_scope->patches);                m3log (compile, "branch patch required at: %p", patch);
    i_scope->patches = patch;
}

ساکن
M3_نتیجہ EmitPatchingBranch (IM3Compilation o, IM3CompilationScope i_scope)
{
    M3_نتیجہ نتیجہ = m3Err_none;

_   (EmitOp(o, op_Branch));
    Emit_Patching_Branch_اشارہ_گر(o, i_scope);

    _catch: واپس نتیجہ;
}

#اگر d_m3HasExceptionHandling

// How many try_table handler records an exit from i_from out to i_target takes
// down with it. Walking outward from the block the branch sits in, every
// try_table scope up to and including the target is left behind - the target
// too, because landing on a block's label means that block is finished.
//
// A loop label is the exception, but a loop is never a try_table, so the count
// is the same either way: the loop itself is not on the list.
ساکن
u32 گنتی_Try_چوکھٹے_مقصد_Exit (IM3CompilationScope i_from, IM3CompilationScope i_ہدف)
{
    u32 گنتی = 0;

    برائے (IM3CompilationScope scope = i_from; scope; scope = scope->outer) {
        اگر (scope->opcode == c_wa_Op_try_جدول) {
            ++گنتی;
        }

        اگر (scope == i_ہدف) {
            توڑو;
        }
    }

    واپس گنتی;
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
ساکن
u32 عدد_Try_چوکھٹے_مقصد_نکالو (IM3CompilationScope i_from, IM3CompilationScope i_ہدف)
{
    اگر (i_ہدف->گہرائی == 0) {
        واپس 0;
    }

    // the walk only ever runs inward-to-outward; a target that is already
    // outside i_from has nothing between them
    اگر (not i_from or i_from->گہرائی < i_ہدف->گہرائی) {
        واپس 0;
    }

    واپس گنتی_Try_چوکھٹے_مقصد_Exit(i_from, i_ہدف);
}


ساکن
M3_نتیجہ Emit_نکالو_Try_چوکھٹے (IM3Compilation o, u32 i_عدد_چوکھٹے)
{
    M3_نتیجہ نتیجہ = m3Err_none;

    اگر (i_عدد_چوکھٹے) {
_       (EmitOp(o, op_نکالو_Handlers));
        EmitConstant32(o, i_عدد_چوکھٹے);
    }

    _catch: واپس نتیجہ;
}


// A return_call leaves the enclosing function before the callee runs, so no
// try_table in this function may catch what the callee throws. Neither shape of
// the instruction unwinds the native stack in time to arrange that on its own:
// op_ReturnCall tail-jumps with op_TryTable's frames still standing, and the
// fallback shape is an ordinary call sitting inside them. Either way the
// handler records have to come off first.
ساکن
M3_نتیجہ Emit_نکالو_Try_چوکھٹے_For_Return_Call (IM3Compilation o)
{
    u32 گنتی = 0;

    برائے (IM3CompilationScope scope = &o->بلاک; scope; scope = scope->outer) {
        اگر (scope->opcode == c_wa_Op_try_جدول) {
            ++گنتی;
        }
    }

    واپس Emit_نکالو_Try_چوکھٹے(o, گنتی);
}

#ورنہ

ساکن
M3_نتیجہ Emit_نکالو_Try_چوکھٹے (IM3Compilation o, u32 i_عدد_چوکھٹے)
{
    (خالی)o;
    (خالی)i_عدد_چوکھٹے;
    واپس m3Err_none;
}

#اگر_ختم // d_m3HasExceptionHandling

#اگر d_m3FuseBranch
// when the pending fold candidate is a compare whose result sits on top in _r0, return the fused
// variant that absorbs an immediately following br_if (i_isIf false) or if (i_isIf true)
ساکن
IM3Operation GetBranchFusionOp (IM3Compilation o, bool i_isIf)
{
    اگر (o->foldPatchPC and o->foldForm < 4 and Is_ڈھیر_چوٹی_اندر_رجسٹر(o) and Get_ڈھیر_چوٹی_اشاریہ(o) == (i16)o->fold_ڈھیر_اشاریہ) {
        ثابت M3FusedCmpOps* ops = GetFusedCmpOps(o->foldOpcode);

        اگر (ops) {
            واپس i_isIf ? ops->ifOp[o->foldForm] : ops->branchIf[o->foldForm];
        }
    }

    واپس تہی;
}
#اگر_ختم // d_m3FuseBranch

ساکن اندرونی
IM3Operation Op_ContinueLoop (IM3Compilation o)
{
#اگر d_m3_Has_ڈھیر_Switching
    اگر (M3_UNLIKELY(o->module->runtime and o->module->runtime->isSuspendable)) {
        واپس op_ContinueLoop_Suspendable;
    }
#اگر_ختم
    واپس op_ContinueLoop;
}

ساکن اندرونی
IM3Operation Op_ContinueLoopIf (IM3Compilation o)
{
#اگر d_m3_Has_ڈھیر_Switching
    اگر (M3_UNLIKELY(o->module->runtime and o->module->runtime->isSuspendable)) {
        واپس op_ContinueLoopIf_Suspendable;
    }
#اگر_ختم
    واپس op_ContinueLoopIf;
}


ساکن
M3_نتیجہ Compile_Branch (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_نتیجہ نتیجہ;
    u32      عدد_Try_چوکھٹے = 0;

    u32 گہرائی;
_   (ReadLEB_u32(&گہرائی, &o->wasm, o->wasm_انتہا));

    // Spec: br_if condition must be i32
    اگر (i_opcode == c_waOp_branchIf and not Is_ڈھیر_Polymorphic(o)) {
        m3type_t condType = Get_ڈھیر_چوٹی_Type(o);
        _throwif(m3Err_typeMismatch, condType != c_m3Type_none and condType != c_m3Type_i32);
    }

    IM3CompilationScope scope;
_   (Get_بلاک_Scope(o, &scope, گہرائی));

#اگر d_m3HasExceptionHandling
    عدد_Try_چوکھٹے = عدد_Try_چوکھٹے_مقصد_نکالو(&o->بلاک, scope);
#اگر_ختم

    // branch target is a loop (continue)
    اگر (scope->opcode == c_waOp_loop) {
        اگر (i_opcode == c_waOp_branchIf) {
            // a handler pop belongs on the taken path only, so it forces the
            // jump-over form even where the target takes no parameters
            اگر (Get_Func_Type_عدد_Params(scope->type) or عدد_Try_چوکھٹے) {
                IM3Operation op = Is_ڈھیر_چوٹی_اندر_رجسٹر(o) ? op_BranchIfPrologue_r : op_BranchIfPrologue_s;

_               (EmitOp(o, op));
_               (Emit_خانہ_عدد_کا_ڈھیر_چوٹی_And_نکالو(o));

                pc_t* jump_مقصد = (pc_t*)Reserve_اشارہ_گر(o);

_               (Emit_نکالو_Try_چوکھٹے(o, عدد_Try_چوکھٹے));

                اگر (not Is_ڈھیر_Polymorphic(o)) {
_                   (Resolve_بلاک_Results(o, scope, /* isBranch: */ true));
                }

_               (EmitOp(o, Op_ContinueLoop(o)));
                Emit_اشارہ_گر(o, scope->pc);
_               (ریکارڈ_عقب_کنارہ(o, scope, false));

                *jump_مقصد = GetPC(o);
            } ورنہ {
                // move the condition to a register
_               (Copy_ڈھیر_چوٹی_مقصد_رجسٹر(o, false));
_               (نکالو_Type(o, c_m3Type_i32));

_               (EmitOp(o, Op_ContinueLoopIf(o)));
                Emit_اشارہ_گر(o, scope->pc);
_               (ریکارڈ_عقب_کنارہ(o, scope, true));
            }

            //          dump_type_stack(o);
        } ورنہ // is c_waOp_branch
        {
_           (Emit_نکالو_Try_چوکھٹے(o, عدد_Try_چوکھٹے));

            // the branch operands become the loop's parameters on the next iteration
            اگر (not Is_ڈھیر_Polymorphic(o)) {
_               (Resolve_بلاک_Results(o, scope, /* isBranch: */ true));
            }

_           (EmitOp(o, Op_ContinueLoop(o)));
            Emit_اشارہ_گر(o, scope->pc);
_           (ریکارڈ_عقب_کنارہ(o, scope, false));
            o->بلاک.isPolymorphic = true;
        }
    } ورنہ // forward branch
    {
        pc_t* jump_مقصد = تہی;

        bool isReturn         = (scope->گہرائی == 0);
        bool ہدف_Has_Results = Get_Func_Type_عدد_Results(scope->type);

        اگر (i_opcode == c_waOp_branchIf) {
            اگر (ہدف_Has_Results or isReturn or عدد_Try_چوکھٹے) {
                IM3Operation op = Is_ڈھیر_چوٹی_اندر_رجسٹر(o) ? op_BranchIfPrologue_r : op_BranchIfPrologue_s;

_               (EmitOp(o, op));
_               (Emit_خانہ_عدد_کا_ڈھیر_چوٹی_And_نکالو(o)); // condition

                // this is continuation point, if the branch isn't taken
                jump_مقصد = (pc_t*)Reserve_اشارہ_گر(o);
            } ورنہ {
#اگر d_m3FuseBranch
                IM3Operation fuseOp = GetBranchFusionOp(o, false);

                اگر (fuseOp) {
                    // absorb the branch into the compare that produced the condition
                    *(IM3Operation*)o->foldPatchPC = fuseOp;
                    InvalidateFold(o);

_                   (نکالو(o));                      // the condition is consumed inside the fused op

                    Emit_Patching_Branch_اشارہ_گر(o, scope);
                    جاؤ _catch;
                }
#اگر_ختم
                IM3Operation op = Is_ڈھیر_چوٹی_اندر_رجسٹر(o) ? op_BranchIf_r : op_BranchIf_s;

_               (EmitOp(o, op));
_               (Emit_خانہ_عدد_کا_ڈھیر_چوٹی_And_نکالو(o)); // condition

                Emit_Patching_Branch_اشارہ_گر(o, scope);
                جاؤ _catch;
            }
        }

        اگر (not Is_ڈھیر_Polymorphic(o)) {
            اگر (isReturn) {
_               (Return_اقدار(o, scope, true));
_               (EmitOp(o, op_Return));
            } ورنہ {
_               (Emit_نکالو_Try_چوکھٹے(o, عدد_Try_چوکھٹے));

_               (Resolve_بلاک_Results(o, scope, true));
_               (EmitPatchingBranch(o, scope));
            }
        }

        اگر (jump_مقصد) {
            *jump_مقصد = GetPC(o);
        }

        اگر (i_opcode == c_waOp_branch) {
_           (سیٹ_ڈھیر_Polymorphic(o));
        }
    }

    _catch: واپس نتیجہ;
}

ساکن
M3_نتیجہ Compile_Branch_جدول (IM3Compilation o, m3opcode_t i_opcode)
{
    // the page a continue-op page is standing in for; non-NULL only while that
    // page is installed in o->page, so the catch below knows to put it back
    IM3_رمز_صفحہ displaced_صفحہ = تہی;
    // indexed by label depth: the continuation stub already emitted for that target
    // within this table, or NULL if this table has not reached it yet
    pc_t* ہدف_Stubs = تہی;
_try {
    u32 ہدف_گنتی;
_   (ReadLEB_u32(&ہدف_گنتی, &o->wasm, o->wasm_انتہا));

    // Spec: validate that the branch index operand is i32
    اگر (not Is_ڈھیر_Polymorphic(o)) {
        m3type_t اشاریہ_Type = Get_ڈھیر_چوٹی_Type(o);
        _throwif(m3Err_typeMismatch, اشاریہ_Type != c_m3Type_none and اشاریہ_Type != c_m3Type_i32);
    }

_   (Preserve_رجسٹر_If_Occupied(o, c_m3Type_i64));         // move branch operand to a slot
    u16 خانہ = Get_ڈھیر_چوٹی_خانہ_نمبر(o);
_   (نکالو(o));

    // OPTZ: according to spec: "forward branches that target a control instruction with a non-empty
    // result type consume matching operands first and push them back on the operand stack after unwinding"
    // So, this move-to-reg is only necessary if the target scopes have a type.

    u32 عدد_رمز_سطریں = ہدف_گنتی + 4; // 3 => IM3Operation + slot + target_count + default_target
_   (Ensure_رمز_صفحہ_عدد_سطریں(o, عدد_رمز_سطریں));

_   (EmitOp(o, op_Branch_جدول));
    Emit_خانہ_انحراف(o, خانہ);
    EmitConstant32(o, ہدف_گنتی);

    IM3_رمز_صفحہ continue_Op_صفحہ = تہی;

    // The label types are checked by ValidateFunction, which runs under the same
    // d_m3EnableValidation guard just before this compilation pass. Comparing each
    // target against the default here would be wrong anyway: the spec matches every
    // target against the operand stack, so in unreachable code the operands are the
    // bottom type and the targets may legitimately differ (unreached-valid.wast).

    ++ہدف_گنتی; // include default

    // Every entry of one br_table is resolved from the same compilation state - each
    // stub restores whatever it touched - so two entries naming the same label emit
    // byte-identical epilogues. Emit one and point both table entries at it. A label
    // deeper than the current block is rejected by GetBlockScope below, which is what
    // bounds this array.
    ہدف_Stubs = m3_AllocArray(pc_t, (size_t)o->بلاک.گہرائی + 1);
    _throwifnull(ہدف_Stubs);

    برائے (u32 i = 0; i < ہدف_گنتی; ++i) {
        u32 ہدف;
_       (ReadLEB_u32(&ہدف, &o->wasm, o->wasm_انتہا));

        IM3CompilationScope scope;
_       (Get_بلاک_Scope(o, &scope, ہدف));

        اگر (ہدف_Stubs[ہدف]) {
            Emit_اشارہ_گر(o, ہدف_Stubs[ہدف]);
            جاری;
        }

        u32 عدد_Try_چوکھٹے = 0;
#اگر d_m3HasExceptionHandling
        عدد_Try_چوکھٹے = عدد_Try_چوکھٹے_مقصد_نکالو(&o->بلاک, scope);
#اگر_ختم

        // A forward branch to a block that carries no results and leaves no try
        // frames behind has nothing to do on the way out but arrive. There is no
        // epilogue to hold, so the table entry becomes the patch site itself and the
        // block's address is written into it when the block ends - no stub, and no
        // code page to put one on. In unreachable code the same is true whatever the
        // block's type, since nothing is emitted for that case either way.
        //
        // Each such entry is patched on its own, so none is recorded in targetStubs:
        // there is no shared stub for a later entry naming this label to point at.
        اگر (scope->opcode != c_waOp_loop and scope->گہرائی > 0 and عدد_Try_چوکھٹے == 0 and
            (Is_ڈھیر_Polymorphic(o) or Get_Func_Type_عدد_Results(scope->type) == 0)) {
            Emit_Patching_Branch_اشارہ_گر(o, scope);
            جاری;
        }

_       (Acquire_Compilation_رمز_صفحہ(o, &continue_Op_صفحہ));

        pc_t آغاز_PC  = Get_صفحہ_PC(continue_Op_صفحہ);
        displaced_صفحہ = o->صفحہ;
        o->صفحہ       = continue_Op_صفحہ;

        اگر (scope->opcode == c_waOp_loop) {
_           (Emit_نکالو_Try_چوکھٹے(o, عدد_Try_چوکھٹے));

            اگر (not Is_ڈھیر_Polymorphic(o)) {
_               (Resolve_بلاک_Results(o, scope, true));
            }

_           (EmitOp(o, Op_ContinueLoop(o)));
            Emit_اشارہ_گر(o, scope->pc);
_           (ریکارڈ_عقب_کنارہ(o, scope, false));
        } ورنہ {
            اگر (not Is_ڈھیر_Polymorphic(o)) {
                اگر (scope->گہرائی == 0) {
_                   (Return_اقدار(o, scope, true));
_                   (EmitOp(o, op_Return));
                } ورنہ {
_                   (Emit_نکالو_Try_چوکھٹے(o, عدد_Try_چوکھٹے));

_                   (Resolve_بلاک_Results(o, scope, true));

_                   (EmitPatchingBranch(o, scope));
                }
            }
        }

        IM3_رمز_صفحہ return_مقصد = displaced_صفحہ;
        displaced_صفحہ        = تہی;

        Release_Compilation_رمز_صفحہ(o);
        o->صفحہ = return_مقصد;

        Emit_اشارہ_گر(o, آغاز_PC);
        ہدف_Stubs[ہدف] = آغاز_PC;
    }

_   (سیٹ_ڈھیر_Polymorphic(o));
} _catch:

    // thrown out of the loop above with a continue-op page installed: release it
    // and restore the one it displaced, which the caller's catch then releases.
    // Otherwise the displaced page is on no list and simply leaks.
    اگر (displaced_صفحہ) {
        Release_Compilation_رمز_صفحہ(o);
        o->صفحہ = displaced_صفحہ;
    }

    m3_Free(ہدف_Stubs);

    واپس نتیجہ;
}

ساکن
M3_نتیجہ Compile_Call_دلائل_And_Return (IM3Compilation o, u16* o_ڈھیر_انحراف, IM3FuncType i_type, bool i_is_بالواسطہ)
{
_try {

    u16 چوٹی_خانہ = Get_Max_مستعمل_خانہ_Plus_One(o);

    // force use of at least one stack slot; this is to help ensure
    // the m3 stack overflows (and traps) before the native stack can overflow.
    // e.g. see Wasm spec test 'runaway' in call.wast
    چوٹی_خانہ = M3_MAX(1, چوٹی_خانہ);

    // stack frame is 64-bit aligned
    Align_خانہ_مقصد_Type(&چوٹی_خانہ, c_m3Type_i64);

    *o_ڈھیر_انحراف = چوٹی_خانہ;

    // wait to pop this here so that topSlot search is correct
    اگر (i_is_بالواسطہ) {
_       (نکالو(o));
    }

    u16 عدد_دلائل = Get_Func_Type_عدد_Params(i_type);
    u16 عدد_Rets = Get_Func_Type_عدد_Results(i_type);

    u16 arg_چوٹی = چوٹی_خانہ + (عدد_دلائل + عدد_Rets) * c_io_خانہ_گنتی;

    Touch_خانہ(o, arg_چوٹی - 1);

    جب (عدد_دلائل--) {
_       (Copy_ڈھیر_چوٹی_مقصد_خانہ(o, arg_چوٹی -= c_io_خانہ_گنتی));
_       (نکالو(o));
    }

    u16 i = 0;
    جب (عدد_Rets--) {
        m3type_t type = Get_Func_Type_نتیجہ_Type(i_type, i++);

_       (ڈالو(o, type, چوٹی_خانہ));
_       (MarkSlotsAllocatedByType(o, چوٹی_خانہ, type));

        چوٹی_خانہ += c_io_خانہ_گنتی;
    }

    } _catch: واپس نتیجہ;
}

// A tail call hands the current frame over to the callee instead of stacking a new one.
// The callee's results have to be the enclosing function's results, so both use the same
// return slots and only the arguments move: they're staged above the caller's stack here
// (the argument expressions can still be reading the args/locals they're about to land on)
// and slid down onto the frame base at runtime, by op_ReturnCall[Indirect].
ساکن
M3_نتیجہ Compile_دم_Call_دلائل (IM3Compilation o, u16* o_ڈھیر_انحراف, u16* o_عدد_Arg_Slots, IM3FuncType i_type, bool i_is_بالواسطہ)
{
_try {
    IM3FuncType funcType = o->function->funcType;

    u16 عدد_Rets = Get_Func_Type_عدد_Results(i_type);
    _throwif(m3Err_typeMismatch, عدد_Rets != Get_Func_Type_عدد_Results(funcType));

    برائے (u16 i = 0; i < عدد_Rets; ++i) {
        _throwif(m3Err_typeMismatch, Get_Func_Type_نتیجہ_Type(i_type, i) != Get_Func_Type_نتیجہ_Type(funcType, i));
    }

    u16 چوٹی_خانہ = Get_Max_مستعمل_خانہ_Plus_One(o);

    چوٹی_خانہ = M3_MAX(1, چوٹی_خانہ);

    // stack frame is 64-bit aligned
    Align_خانہ_مقصد_Type(&چوٹی_خانہ, c_m3Type_i64);

    *o_ڈھیر_انحراف = چوٹی_خانہ;

    // wait to pop this here so that topSlot search is correct
    اگر (i_is_بالواسطہ) {
_       (نکالو(o));
    }

    u16 عدد_دلائل     = Get_Func_Type_عدد_Params(i_type);
    u16 عدد_Arg_Slots = عدد_دلائل * c_io_خانہ_گنتی;

    *o_عدد_Arg_Slots = عدد_Arg_Slots;

    u16 arg_چوٹی = چوٹی_خانہ + عدد_Arg_Slots;
    _throwif(m3_Err_function_ڈھیر_حد_سے_تجاوز, arg_چوٹی > d_m3MaxFunctionSlots);

    // the staging area is written by the caller, so op_Entry has to account for it
    Touch_خانہ(o, arg_چوٹی - 1);

    جب (عدد_دلائل--) {
_       (Copy_ڈھیر_چوٹی_مقصد_خانہ(o, arg_چوٹی -= c_io_خانہ_گنتی));
_       (نکالو(o));
    }

    } _catch: واپس نتیجہ;
}


#اگر d_m3HasTypedRefs

// call_ref $t: the callee is the reference on top of the stack, so the operand
// order is the same as call_indirect's, minus the table.
ساکن
M3_نتیجہ Compile_CallRef (IM3Compilation o, m3opcode_t i_opcode)
{
_try {
    u32 type_اشاریہ;
_   (ReadLEB_u32(&type_اشاریہ, &o->wasm, o->wasm_انتہا));

    _throwif("function call type index out of range", type_اشاریہ >= o->module->عدد_Func_Types);

    IM3FuncType type = o->module->funcTypes[type_اشاریہ];

    اگر (not Is_ڈھیر_Polymorphic(o)) {
        // whatever shape the reference has, it must be a function of this type
        m3type_t refType = Get_ڈھیر_چوٹی_Type(o);
        _throwif(m3Err_typeMismatch, not Is_Sub_Type_کا(refType, Ref_Type_کا_Func_Type(type, false)));
    }

    اگر (Is_ڈھیر_چوٹی_اندر_رجسٹر(o)) {
_       (Preserve_رجسٹر_If_Occupied(o, c_m3Type_funcref));
    }

    u16 function_خانہ = Get_ڈھیر_چوٹی_خانہ_نمبر(o);

    bool isReturnCall = (i_opcode == c_waOp_returnCallRef);
    bool use_دم_Call  = isReturnCall and d_m3_Can_دم_Call;

    u16 exec_چوٹی, عدد_Arg_Slots = 0;

    اگر (use_دم_Call) {
_       (Compile_دم_Call_دلائل(o, &exec_چوٹی, &عدد_Arg_Slots, type, true));
    } ورنہ {
_       (Compile_Call_دلائل_And_Return(o, &exec_چوٹی, type, true));
    }

#  اگر d_m3HasExceptionHandling
    اگر (isReturnCall) {
_       (Emit_نکالو_Try_چوکھٹے_For_Return_Call(o));
    }
#  endif

_   (EmitOp(o, use_دم_Call ? op_ReturnCallRef : op_CallRef));
    Emit_خانہ_انحراف(o, function_خانہ);
    Emit_اشارہ_گر(o, type);
    Emit_خانہ_انحراف(o, exec_چوٹی);

    اگر (not use_دم_Call) {
_       (ریکارڈ_Call(o, Get_Func_Type_عدد_Results(type), exec_چوٹی, isReturnCall));
    }

    اگر (use_دم_Call) {
        Emit_خانہ_انحراف(o, o->function->عدد_Ret_Slots);
        EmitConstant32(o, عدد_Arg_Slots);

_       (سیٹ_ڈھیر_Polymorphic(o));
    } ورنہ اگر (isReturnCall) {
_       (Compile_Return(o, i_opcode));
    }

} _catch:
    واپس نتیجہ;
}


// ref.as_non_null only sharpens the type; the value is unchanged, so the stack
// entry is re-pushed with the null stripped out of its type.
ساکن
M3_نتیجہ Compile_Ref_AsNonNull (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_نتیجہ نتیجہ = m3Err_none;

    اگر (not Is_ڈھیر_Polymorphic(o)) {
        m3type_t type = Get_ڈھیر_چوٹی_Type(o);
        _throwif(m3Err_typeMismatch, not IsRefType(type));

        // op_RefAsNonNull only knows how to read its operand out of a slot; a
        // value still sitting in the register has to be spilled first, the way
        // every other single-op reader of the stack top does
        اگر (Is_ڈھیر_چوٹی_اندر_رجسٹر(o)) {
_           (Preserve_رجسٹر_If_Occupied(o, type));
        }

        u16 خانہ = Get_ڈھیر_چوٹی_خانہ_نمبر(o);

_       (EmitOp(o, op_RefAsNonNull));
        Emit_خانہ_انحراف(o, خانہ);

        m3type_t nonNull = IsSpelledRefType(type)
                             ? (m3type_t)(type | d_m3Type_refNonNull)
                             : (m3type_t)(d_m3Type_ref | d_m3Type_refNonNull | d_m3_Type_ڈھیر_Abstract |
                                          ((بنیاد_Type_کا(type) == c_m3Type_externref) ? d_m3Type_refExtern : 0));

        // the slot keeps the one reference it already had; a Pop()/Push() of
        // the same slot would drop that reference's count without anything
        // to put it back, so the type above it is corrected in place instead
        o->type_ڈھیر[Get_ڈھیر_چوٹی_اشاریہ(o)] = nonNull;
    }

    _catch: واپس نتیجہ;
}

#اگر_ختم // d_m3HasTypedRefs


// How wide a table operand is read at comes from the table it names, so a slot
// holding a narrower type would be read past what was allocated for it. The
// validator refuses that already; this keeps a build without one from reaching
// the interpreter with it. c_m3Type_none means the operand is not on the
// dynamic stack to inspect, which is not this check's business.
ساکن
M3_نتیجہ Check_عملیہ_Type (IM3Compilation o, u16 i_گہرائی_From_چوٹی, m3type_t i_type)
{
    m3type_t actual;

    اگر (Is_ڈھیر_Polymorphic(o)) {
        واپس m3Err_none;
    }

    actual = Get_ڈھیر_Type_From_چوٹی(o, i_گہرائی_From_چوٹی);

    واپس (actual == c_m3Type_none or بنیاد_Type_کا(actual) == i_type) ? m3Err_none
                                                                     : m3Err_typeMismatch;
}


// Swaps the _mem register onto a given memory. Only ever emitted in pairs --
// see op_SetMemory.
ساکن
M3_نتیجہ Emit_سیٹ_حافظہ_Ptr (IM3Compilation o, IM3_حافظہ i_حافظہ)
{
    M3_نتیجہ نتیجہ = m3Err_none;

_   (EmitOp(o, op_سیٹ_حافظہ));
    Emit_اشارہ_گر(o, i_حافظہ);

    _catch: واپس نتیجہ;
}


// ...onto one of the compiling module's own memories, by index. Only reached
// with a non-zero index under multi-memory, but the index has already been
// bounds-checked by ReadMemoryIndex either way.
ساکن اندرونی
M3_نتیجہ Emit_سیٹ_حافظہ (IM3Compilation o, u32 i_حافظہ_Idx)
{
#اگر d_m3_Has_Multi_حافظہ
    واپس Emit_سیٹ_حافظہ_Ptr(o, o->module->memories[i_حافظہ_Idx]);
#ورنہ
    (خالی)o;
    (خالی)i_حافظہ_Idx;
    واپس m3_Err_unknown_حافظہ;
#اگر_ختم
}

ساکن
M3_نتیجہ Compile_Call (IM3Compilation o, m3opcode_t i_opcode)
{
_try {
    u32 function_اشاریہ;
_   (ReadLEB_u32(&function_اشاریہ, &o->wasm, o->wasm_انتہا));

    IM3Function function = Module_GetFunction(o->module, function_اشاریہ);

    اگر (function) {                                                     m3log (compile, d_indent " (func= [%d] '%s'; args= %d)",
                                                                                get_indention_string (o), function_اشاریہ, m3_GetFunctionName (function), function->funcType->عدد_دلائل);
        // an import linked to another module's export runs that module's body
        IM3Function ہدف = Function_Implementation(function);

        اگر (ہدف->module) {
            bool isReturnCall = (i_opcode == c_waOp_returnCall);

            // The callee's body runs against its own module's memory, so a call
            // that crosses modules is bracketed by a pair of op_SetMemory. A
            // tail call never returns to run the second one, so those compile
            // as a plain call followed by a return instead.
            bool crossModule = (ہدف->module != o->module);
            bool use_دم_Call = isReturnCall and d_m3_Can_دم_Call and not crossModule;

            u16 خانہ_چوٹی, عدد_Arg_Slots = 0;

            اگر (use_دم_Call) {
_               (Compile_دم_Call_دلائل(o, &خانہ_چوٹی, &عدد_Arg_Slots, ہدف->funcType, false));
            } ورنہ {
_               (Compile_Call_دلائل_And_Return(o, &خانہ_چوٹی, ہدف->funcType, false));
            }

            IM3Operation op;
            ثابت خالی*  عملیہ;

            اگر (ہدف->compiled) {
                op      = use_دم_Call ? op_ReturnCall : op_Call;
                عملیہ = ہدف->compiled;
            } ورنہ {
                op      = use_دم_Call ? op_CompileReturnCall : op_Compile;
                عملیہ = ہدف;
            }

#اگر d_m3HasExceptionHandling
            اگر (isReturnCall) {
_               (Emit_نکالو_Try_چوکھٹے_For_Return_Call(o));
            }
#اگر_ختم

            اگر (crossModule) {
_               (Emit_سیٹ_حافظہ_Ptr(o, Module_Memory0(ہدف->module)));
            }

_           (EmitOp(o, op));
            Emit_اشارہ_گر(o, عملیہ);
            Emit_خانہ_انحراف(o, خانہ_چوٹی);

            اگر (not use_دم_Call) {
                // a suspend inside the callee leaves this frame waiting here, its
                // results not yet written
_               (ریکارڈ_Call(o, Get_Func_Type_عدد_Results(ہدف->funcType), خانہ_چوٹی, isReturnCall));
            }

            اگر (use_دم_Call) {
                Emit_خانہ_انحراف(o, o->function->عدد_Ret_Slots);
                EmitConstant32(o, عدد_Arg_Slots);

_               (سیٹ_ڈھیر_Polymorphic(o));
            } ورنہ {
                اگر (crossModule) {
_                   (Emit_سیٹ_حافظہ_Ptr(o, Module_Memory0(o->module)));
                }

                اگر (isReturnCall) {
_                   (Compile_Return(o, i_opcode));
                }
            }
        } ورنہ {
            _throw(ErrorCompile(m3Err_functionImportMissing, o, "'%s.%s'", GetFunctionImportModuleName(function), m3_GetFunctionName(function)));
        }
    } ورنہ {
        _throw(ErrorCompile(m3Err_functionLookupFailed, o, "index: %d", function_اشاریہ));
    }

    } _catch: واپس نتیجہ;
}

ساکن
M3_نتیجہ Compile_Call_بالواسطہ (IM3Compilation o, m3opcode_t i_opcode)
{
_try {
    u32 type_اشاریہ;
_   (ReadLEB_u32(&type_اشاریہ, &o->wasm, o->wasm_انتہا));

    u32 جدول_اشاریہ;
_   (ReadLEB_u32(&جدول_اشاریہ, &o->wasm, o->wasm_انتہا));

    _throwif("function call type index out of range", type_اشاریہ >= o->module->عدد_Func_Types);
    _throwif("table index out of range", جدول_اشاریہ >= o->module->عدد_Tables);
    _throwif(m3Err_typeMismatch, بنیاد_Type_کا(o->module->tables[جدول_اشاریہ]->type) != c_m3Type_funcref);

_   (Check_عملیہ_Type(o, 0, جدول_Addr_Type(o->module->tables[جدول_اشاریہ])));

    اگر (Is_ڈھیر_چوٹی_اندر_رجسٹر(o)) {
_       (Preserve_رجسٹر_If_Occupied(o, c_m3Type_i32));
    }

    u16 جدول_اشاریہ_خانہ = Get_ڈھیر_چوٹی_خانہ_نمبر(o);

    bool isReturnCall = (i_opcode == c_wa_Op_return_Call_بالواسطہ);
    bool use_دم_Call  = isReturnCall and d_m3_Can_دم_Call;

    u16         exec_چوٹی, عدد_Arg_Slots = 0;
    IM3FuncType type = o->module->funcTypes[type_اشاریہ];

    اگر (use_دم_Call) {
_       (Compile_دم_Call_دلائل(o, &exec_چوٹی, &عدد_Arg_Slots, type, true));
    } ورنہ {
_       (Compile_Call_دلائل_And_Return(o, &exec_چوٹی, type, true));
    }

#اگر d_m3HasExceptionHandling
    اگر (isReturnCall) {
_       (Emit_نکالو_Try_چوکھٹے_For_Return_Call(o));
    }
#اگر_ختم

    // the table comes first: how wide its indexes are is what says how much of
    // the slot below holds one
_   (EmitOp(o, use_دم_Call ? op_Return_Call_بالواسطہ : op_Call_بالواسطہ));
    Emit_اشارہ_گر(o, o->module->tables[جدول_اشاریہ]);
    Emit_خانہ_انحراف(o, جدول_اشاریہ_خانہ);
    Emit_اشارہ_گر(o, type);
    Emit_خانہ_انحراف(o, exec_چوٹی);

    اگر (not use_دم_Call) {
_       (ریکارڈ_Call(o, Get_Func_Type_عدد_Results(type), exec_چوٹی, isReturnCall));
    }

    اگر (use_دم_Call) {
        Emit_خانہ_انحراف(o, o->function->عدد_Ret_Slots);
        EmitConstant32(o, عدد_Arg_Slots);

_       (سیٹ_ڈھیر_Polymorphic(o));
    } ورنہ اگر (isReturnCall) {
_       (Compile_Return(o, i_opcode));
    }

} _catch:
    واپس نتیجہ;
}

// Reads the memory index a memory instruction names, and checks it against the
// module's index space. Without multi-memory the immediate is a reserved byte
// that has to be zero, which is the same check.
ساکن
M3_نتیجہ Read_حافظہ_اشاریہ (IM3Compilation o, u32* o_حافظہ_Idx)
{
    M3_نتیجہ نتیجہ;

_   (ReadLEB_u32(o_حافظہ_Idx, &o->wasm, o->wasm_انتہا));

    _throwif(m3_Err_unknown_حافظہ, *o_حافظہ_Idx >= o->module->عدد_Memories);

    _catch: واپس نتیجہ;
}

ساکن
M3_نتیجہ Compile_حافظہ_سائز (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_نتیجہ نتیجہ;

    u32 حافظہ_Idx;

    // A page count is given in the memory's own address type. Declared up here
    // so the throws below don't jump over its initialization.
    m3type_t addrType = c_m3Type_i32;

_   (Read_حافظہ_اشاریہ(o, &حافظہ_Idx));

    addrType = حافظہ_Addr_Type(o->module->memories[حافظہ_Idx]);

_   (Preserve_رجسٹر_If_Occupied(o, addrType));

    اگر (حافظہ_Idx) {
_       (Emit_سیٹ_حافظہ(o, حافظہ_Idx));
    }

    // op_MemSize writes the whole register either way; the slot it is pushed
    // to is what decides how much of it the module gets to see
_   (EmitOp(o, op_Mem_سائز));

    اگر (حافظہ_Idx) {
_       (Emit_سیٹ_حافظہ(o, 0));
    }

_   (ڈالو_رجسٹر(o, addrType));

    _catch: واپس نتیجہ;
}

ساکن
M3_نتیجہ Compile_حافظہ_Grow (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_نتیجہ نتیجہ;

    u32 حافظہ_Idx;

    // see Compile_Memory_Size
    m3type_t addrType = c_m3Type_i32;

_   (Read_حافظہ_اشاریہ(o, &حافظہ_Idx));

    addrType = حافظہ_Addr_Type(o->module->memories[حافظہ_Idx]);

_   (Copy_ڈھیر_چوٹی_مقصد_رجسٹر(o, false));
_   (نکالو_Type(o, addrType));

    اگر (حافظہ_Idx) {
_       (Emit_سیٹ_حافظہ(o, حافظہ_Idx));
    }

#اگر d_m3HasMemory64
_   (EmitOp(o, (addrType == c_m3Type_i64) ? op_MemGrow64 : op_MemGrow));
#ورنہ
_   (EmitOp(o, op_MemGrow));
#اگر_ختم

    اگر (حافظہ_Idx) {
_       (Emit_سیٹ_حافظہ(o, 0));
    }

_   (ڈالو_رجسٹر(o, addrType));

    _catch: واپس نتیجہ;
}

ساکن
M3_نتیجہ Compile_حافظہ_Copy_Fill (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_نتیجہ نتیجہ = m3Err_none;

    // memory.copy names the destination first and then the source; memory.fill
    // names only the one it writes.
    u32  ماخذ_حافظہ_Idx = 0, ہدف_حافظہ_Idx = 0;
    bool isCopy = (i_opcode == c_wa_Op_حافظہ_Copy);

    // Each address is typed by the memory it belongs to. The length follows
    // the narrower of the two, so it is 64-bit only when both are - which for
    // memory.fill, naming one memory twice over, means whenever that one is.
    // Declared up here so the throws below don't jump over them.
    m3type_t ہدف_Type = c_m3Type_i32;
    m3type_t ماخذ_Type = c_m3Type_i32;
    m3type_t لمبائی_Type = c_m3Type_i32;

    _throwif(m3Err_wasmMalformed, o->module->عدد_Memories == 0);

_   (Read_حافظہ_اشاریہ(o, &ہدف_حافظہ_Idx));

    اگر (isCopy) {
_       (Read_حافظہ_اشاریہ(o, &ماخذ_حافظہ_Idx));
    }

    ہدف_Type = حافظہ_Addr_Type(o->module->memories[ہدف_حافظہ_Idx]);
    ماخذ_Type = isCopy ? حافظہ_Addr_Type(o->module->memories[ماخذ_حافظہ_Idx])
                        : ہدف_Type;
    لمبائی_Type = (ہدف_Type == c_m3Type_i64 and ماخذ_Type == c_m3Type_i64)
                   ? c_m3Type_i64
                   : c_m3Type_i32;

_   (Copy_ڈھیر_چوٹی_مقصد_رجسٹر(o, false));

#اگر d_m3_Has_Multi_حافظہ
    اگر (isCopy and ماخذ_حافظہ_Idx != ہدف_حافظہ_Idx) {
        // two memories at once: _mem can only name one, so the op takes both
_       (EmitOp(o, op_MemCopy_x));
        Emit_اشارہ_گر(o, o->module->memories[ہدف_حافظہ_Idx]);
        Emit_اشارہ_گر(o, o->module->memories[ماخذ_حافظہ_Idx]);

        // ...and, since they need not be addressed alike, how wide to read
        // each of the operands: bit 0 the destination, bit 1 the source
        EmitConstant32(o, ((ہدف_Type == c_m3Type_i64) ? 0x1u : 0u) |
                            ((ماخذ_Type == c_m3Type_i64) ? 0x2u : 0u));
    } ورنہ
#اگر_ختم
    {
        IM3Operation op = isCopy ? op_MemCopy : op_MemFill;

#اگر d_m3HasMemory64
        اگر (ہدف_Type == c_m3Type_i64) {
            op = isCopy ? op_MemCopy64 : op_MemFill64;
        }
#اگر_ختم

        اگر (ہدف_حافظہ_Idx) {
_           (Emit_سیٹ_حافظہ(o, ہدف_حافظہ_Idx));
        }

_       (EmitOp(o, op));
    }

    // the length is in the register; the other two are named by slot, at
    // whatever width their stack entries were pushed with
_   (نکالو_Type(o, لمبائی_Type));
_   (Emit_خانہ_عدد_کا_ڈھیر_چوٹی_And_نکالو(o));      // the source, or the byte to fill with
_   (Emit_خانہ_عدد_کا_ڈھیر_چوٹی_And_نکالو(o));      // the destination

    اگر (ہدف_حافظہ_Idx and not (isCopy and ماخذ_حافظہ_Idx != ہدف_حافظہ_Idx)) {
_       (Emit_سیٹ_حافظہ(o, 0));
    }

    _catch: واپس نتیجہ;
}


// memory.init and data.drop address a data segment by index. Both are only valid
// when a data count section declared the segments up front.
ساکن
M3_نتیجہ Read_ڈیٹا_Segment (IM3Compilation o, M3_ڈیٹا_Segment** o_segment)
{
    M3_نتیجہ نتیجہ = m3Err_none;

    u32 index;
_   (ReadLEB_u32(&index, &o->wasm, o->wasm_انتہا));

    _throwif("data count section required", not o->module->has_ڈیٹا_گنتی);
    _throwif(m3Err_wasmMalformed, index >= o->module->عدد_ڈیٹا_Segments);

    *o_segment = &o->module->ڈیٹا_Segments[index];

    _catch: واپس نتیجہ;
}


M3_نتیجہ Compile_حافظہ_Init (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_نتیجہ نتیجہ = m3Err_none;

    M3_ڈیٹا_Segment* segment = تہی;
    u32            حافظہ_Idx;

    _throwif(m3Err_wasmMalformed, o->module->عدد_Memories == 0);

_   (Read_ڈیٹا_Segment(o, &segment));
_   (Read_حافظہ_اشاریہ(o, &حافظہ_Idx));

_   (Copy_ڈھیر_چوٹی_مقصد_رجسٹر(o, false));

    اگر (حافظہ_Idx) {
_       (Emit_سیٹ_حافظہ(o, حافظہ_Idx));
    }

#اگر d_m3HasMemory64
    // only the destination follows the memory's address type: a data segment
    // is indexed as an i32 however the memory it is copied into is addressed
_   (EmitOp(o, o->module->memories[حافظہ_Idx]->isMemory64 ? op_MemInit64 : op_MemInit));
#ورنہ
_   (EmitOp(o, op_MemInit));
#اگر_ختم
    Emit_اشارہ_گر(o, segment);
_   (نکالو_Type(o, c_m3Type_i32));
_   (Emit_خانہ_عدد_کا_ڈھیر_چوٹی_And_نکالو(o));
_   (Emit_خانہ_عدد_کا_ڈھیر_چوٹی_And_نکالو(o));

    اگر (حافظہ_Idx) {
_       (Emit_سیٹ_حافظہ(o, 0));
    }

    _catch: واپس نتیجہ;
}


#اگر d_m3HasRefTypes

ساکن
M3_نتیجہ Compile_Select (IM3Compilation o, m3opcode_t i_opcode);

// select with an explicit result type vector. The types only matter to the
// validator; the operands are laid out exactly as for the untyped select.
M3_نتیجہ Compile_Select_Typed (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_نتیجہ نتیجہ = m3Err_none;

    u32 عدد_Types;
_   (ReadLEB_u32(&عدد_Types, &o->wasm, o->wasm_انتہا));
    _throwif(m3Err_wasmMalformed, عدد_Types != 1);

    i8 waType;
    u8 type;
_   (ReadLEB_i7(&waType, &o->wasm, o->wasm_انتہا));
_   (NormalizeType(&type, waType));

_   (Compile_Select(o, i_opcode));

    _catch: واپس نتیجہ;
}


// A reference is one pointer-sized word and null is 0, so null-testing it is
// just an integer eqz of the matching width.
#  اگر M3_SIZEOF_PTR == 8
#    define d_m3RefIsNull_r   op_i64_Equal_مقصد_Zero_r
#    define d_m3RefIsNull_s   op_i64_Equal_مقصد_Zero_s
#  ورنہ
#    define d_m3RefIsNull_r   op_i32_Equal_مقصد_Zero_r
#    define d_m3RefIsNull_s   op_i32_Equal_مقصد_Zero_s
#  endif

ساکن
M3_نتیجہ ReadRefType (IM3Compilation o, m3type_t* o_type)
{
    M3_نتیجہ نتیجہ = m3Err_none;

#  اگر d_m3HasTypedRefs
    // ref.null names a heap type: func, extern, or a function type index
    m3type_t ڈھیر_بٹ;
_   (Parse_ڈھیر_Type(o->module, &ڈھیر_بٹ, &o->wasm, o->wasm_انتہا));

    *o_type = d_m3Type_ref | ڈھیر_بٹ;
#  ورنہ
    i8 waType;
    u8 plainType;
_   (ReadLEB_i7(&waType, &o->wasm, o->wasm_انتہا));
_   (NormalizeType(&plainType, waType));
    *o_type = plainType;
    _throwif(m3Err_wasmMalformed, not IsRefType(*o_type));
#  endif

    _catch: واپس نتیجہ;
}


M3_نتیجہ Compile_Ref_Null (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_نتیجہ نتیجہ = m3Err_none;

    m3type_t type;
_   (ReadRefType(o, &type));
_   (ڈالو_Const(o, 0, type));

    _catch: واپس نتیجہ;
}


M3_نتیجہ Compile_Ref_IsNull (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_نتیجہ نتیجہ = m3Err_none;

    IM3Operation op;

    اگر (not Is_ڈھیر_Polymorphic(o)) {
        _throwif(m3Err_typeMismatch, not IsRefType(Get_ڈھیر_چوٹی_Type(o)));
    }

    اگر (Is_ڈھیر_چوٹی_اندر_رجسٹر(o)) {
        op = d_m3RefIsNull_r;
    } ورنہ {
_       (Preserve_رجسٹر_If_Occupied(o, c_m3Type_i32));
        op = d_m3RefIsNull_s;
    }

_   (EmitOp(o, op));
_   (Emit_خانہ_عدد_کا_ڈھیر_چوٹی_And_نکالو(o));
_   (ڈالو_رجسٹر(o, c_m3Type_i32));

    _catch: واپس نتیجہ;
}


M3_نتیجہ Compile_Ref_Func (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_نتیجہ نتیجہ = m3Err_none;

    u32 func_اشاریہ;

    // declared before the throws below, which jump past this point to _catch
    m3type_t    refType   = c_m3Type_funcref;
    IM3Function reference = تہی;

_   (ReadLEB_u32(&func_اشاریہ, &o->wasm, o->wasm_انتہا));
    _throwif("function index out of range", func_اشاریہ >= o->module->عدد_Functions);

    // Inside a constant expression ref.func is itself a declaration; inside a
    // function body the function must already have been declared elsewhere.
    اگر (o->function) {
        _throwif("undeclared function reference", not Module_IsFunctionDeclared(o->module, func_اشاریہ));
    } ورنہ {
_       (Module_DeclareFunction(o->module, func_اشاریہ));
    }

#  اگر d_m3HasTypedRefs
    refType = Ref_Type_کا_Func_Type(o->module->functions[func_اشاریہ].funcType, true);
#  endif

    // A reference denotes the function that actually runs, so a ref.func naming
    // an import linked to another module is the same reference that module's
    // own ref.func would produce - and carries the defining module with it.
    reference = Function_Implementation(&o->module->functions[func_اشاریہ]);

_   (ڈالو_Const(o, (u64)(uintptr_t)reference, refType));

    _catch: واپس نتیجہ;
}


// table.get/set/size/grow/fill all name a table by index; the table struct goes
// into the codestream so the operation doesn't have to walk the module.
ساکن
M3_نتیجہ Read_جدول (IM3Compilation o, M3_جدول** o_جدول)
{
    M3_نتیجہ نتیجہ = m3Err_none;

    u32 index;
_   (ReadLEB_u32(&index, &o->wasm, o->wasm_انتہا));
    _throwif("table index out of range", index >= o->module->عدد_Tables);

    *o_جدول = o->module->tables[index];

    _catch: واپس نتیجہ;
}


// The table operations take every operand from a slot, so spill the integer
// register first: after this each EmitSlotNumOfStackTopAndPop () emits a real
// slot offset, and the register is free for the result. Operands are emitted
// from the top of the stack downwards, which is the order the ops read them.
ساکن
M3_نتیجہ Compile_جدول_Op (IM3Compilation o, IM3Operation i_op, M3_جدول* i_جدول, u32 i_عدد_Operands, m3type_t i_retType)
{
    M3_نتیجہ نتیجہ = m3Err_none;

_   (Preserve_رجسٹر_If_Occupied(o, c_m3Type_i64));

_   (EmitOp(o, i_op));
    Emit_اشارہ_گر(o, i_جدول);

    برائے (u32 i = 0; i < i_عدد_Operands; ++i) {
_       (Emit_خانہ_عدد_کا_ڈھیر_چوٹی_And_نکالو(o));
    }

    اگر (i_retType != c_m3Type_none) {
_       (ڈالو_رجسٹر(o, i_retType));
    }

    _catch: واپس نتیجہ;
}


M3_نتیجہ Compile_جدول_Get_سیٹ (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_نتیجہ نتیجہ = m3Err_none;

    M3_جدول* جدول_2;
_   (Read_جدول(o, &جدول_2));

    اگر (i_opcode == c_wa_Op_جدول_Get) {
_       (Check_عملیہ_Type(o, 0, جدول_Addr_Type(جدول_2)));
_       (Compile_جدول_Op(o, op_جدول_Get, جدول_2, 1, جدول_2->type));
    } ورنہ {
_       (Check_عملیہ_Type(o, 1, جدول_Addr_Type(جدول_2)));
_       (Compile_جدول_Op(o, op_جدول_سیٹ, جدول_2, 2, c_m3Type_none));
    }

    _catch: واپس نتیجہ;
}


M3_نتیجہ Compile_جدول_Init (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_نتیجہ نتیجہ = m3Err_none;

    u32      elem_اشاریہ;
    M3_جدول* جدول_2;

_   (ReadLEB_u32(&elem_اشاریہ, &o->wasm, o->wasm_انتہا));
    _throwif("element segment index out of range", elem_اشاریہ >= o->module->عدد_Element_Segments);
_   (Read_جدول(o, &جدول_2));

_   (Check_عملیہ_Type(o, 2, جدول_Addr_Type(جدول_2)));

_   (Preserve_رجسٹر_If_Occupied(o, c_m3Type_i64));
_   (EmitOp(o, op_جدول_Init));
    Emit_اشارہ_گر(o, جدول_2);
    Emit_اشارہ_گر(o, &o->module->elementSegments[elem_اشاریہ]);

    برائے (u32 i = 0; i < 3; ++i) {
_       (Emit_خانہ_عدد_کا_ڈھیر_چوٹی_And_نکالو(o));
    }

    _catch: واپس نتیجہ;
}


M3_نتیجہ Compile_Elem_Drop (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_نتیجہ نتیجہ = m3Err_none;

    u32 elem_اشاریہ;
_   (ReadLEB_u32(&elem_اشاریہ, &o->wasm, o->wasm_انتہا));
    _throwif("element segment index out of range", elem_اشاریہ >= o->module->عدد_Element_Segments);

_   (EmitOp(o, op_ElemDrop));
    Emit_اشارہ_گر(o, &o->module->elementSegments[elem_اشاریہ]);

    _catch: واپس نتیجہ;
}


M3_نتیجہ Compile_جدول_Copy (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_نتیجہ نتیجہ = m3Err_none;

    M3_جدول* dst;
    M3_جدول* src;

_   (Read_جدول(o, &dst));
_   (Read_جدول(o, &src));
    _throwif(m3Err_typeMismatch, not Is_Sub_Type_کا(src->type, dst->type));

_   (Check_عملیہ_Type(o, 2, جدول_Addr_Type(dst)));
_   (Check_عملیہ_Type(o, 1, جدول_Addr_Type(src)));
_   (Check_عملیہ_Type(o, 0, (dst->isTable64 and src->isTable64) ? c_m3Type_i64 : c_m3Type_i32));

_   (Preserve_رجسٹر_If_Occupied(o, c_m3Type_i64));
_   (EmitOp(o, op_جدول_Copy));
    Emit_اشارہ_گر(o, dst);
    Emit_اشارہ_گر(o, src);

    برائے (u32 i = 0; i < 3; ++i) {
_       (Emit_خانہ_عدد_کا_ڈھیر_چوٹی_And_نکالو(o));
    }

    _catch: واپس نتیجہ;
}


M3_نتیجہ Compile_جدول_سائز (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_نتیجہ نتیجہ = m3Err_none;

    M3_جدول* جدول_2;
_   (Read_جدول(o, &جدول_2));

    // a table size is given in the table's own index type
_   (Compile_جدول_Op(o, op_جدول_سائز, جدول_2, 0, جدول_Addr_Type(جدول_2)));

    _catch: واپس نتیجہ;
}


M3_نتیجہ Compile_جدول_Grow_Fill (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_نتیجہ نتیجہ = m3Err_none;

    M3_جدول* جدول_2;
_   (Read_جدول(o, &جدول_2));

    اگر (i_opcode == c_wa_Op_جدول_Grow) {
_       (Check_عملیہ_Type(o, 0, جدول_Addr_Type(جدول_2)));
_       (Compile_جدول_Op(o, op_جدول_Grow, جدول_2, 2, جدول_Addr_Type(جدول_2)));
    } ورنہ {
_       (Check_عملیہ_Type(o, 0, جدول_Addr_Type(جدول_2)));
_       (Check_عملیہ_Type(o, 2, جدول_Addr_Type(جدول_2)));
_       (Compile_جدول_Op(o, op_جدول_Fill, جدول_2, 3, c_m3Type_none));
    }

    _catch: واپس نتیجہ;
}

#اگر_ختم // d_m3HasRefTypes


M3_نتیجہ Compile_ڈیٹا_Drop (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_نتیجہ نتیجہ = m3Err_none;

    M3_ڈیٹا_Segment* segment = تہی;
_   (Read_ڈیٹا_Segment(o, &segment));

_   (EmitOp(o, op_ڈیٹا_Drop));
    Emit_اشارہ_گر(o, segment);

    _catch: واپس نتیجہ;
}


ساکن
M3_نتیجہ Read_بلاک_Type (IM3Compilation o, IM3FuncType* o_بلاک_Type)
{
    M3_نتیجہ نتیجہ = m3Err_none;

#اگر d_m3HasTypedRefs
    // a spelled-out reference type is two bytes, so it cannot be told apart
    // from a type index by the s33 that block types otherwise use
    اگر (o->wasm < o->wasm_انتہا and (*o->wasm == d_waEncode_ref or *o->wasm == d_waEncode_refNull)) {
        m3type_t refType;
_       (Parse_قدر_Type(o->module, &refType, &o->wasm, o->wasm_انتہا));
        IM3FuncType ftype = تہی;
_       (AllocFuncType(&ftype, 1));
        ftype->عدد_Rets  = 1;
        ftype->types[0] = refType;
_       (Environment_جوڑو_Func_Type(o->module->environment, &ftype));
        *o_بلاک_Type = ftype;
        واپس نتیجہ;
    }
#اگر_ختم

    i64 type;
_   (ReadLebSigned(&type, 33, &o->wasm, o->wasm_انتہا));

    اگر (type < 0) {
        _throwif(m3Err_invalidTypeId, type < -64);
        u8 قدر_Type;
_       (NormalizeType(&قدر_Type, (i8)type));                                 m3log (compile, d_indent " (type: %s)", get_indention_string (o), c_waTypes [قدر_Type]);
        *o_بلاک_Type = o->module->environment->retFuncTypes[قدر_Type];
    } ورنہ {
        _throwif("func type out of bounds", type >= o->module->عدد_Func_Types);
        *o_بلاک_Type = o->module->funcTypes[type];                         m3log (compile, d_indent " (type: %s)", get_indention_string (o), SPrintFuncTypeSignature (*o_بلاک_Type));
    }
    _catch: واپس نتیجہ;
}

ساکن
M3_نتیجہ Preserve_دلائل_And_Locals (IM3Compilation o)
{
    M3_نتیجہ نتیجہ = m3Err_none;

    اگر (o->ڈھیر_اشاریہ > o->ڈھیر_پہلا_Dynamic_اشاریہ) {
        u32 عدد_دلائل_And_Locals = Get_Function_عدد_دلائل_And_Locals(o->function);

        برائے (u32 i = 0; i < عدد_دلائل_And_Locals; ++i) {
            u16 خانہ = Get_خانہ_For_ڈھیر_اشاریہ(o, i);

            u16 preserved_خانہ_نمبر;
_           (ڈھونڈو_Referenced_Local_Within_Current_بلاک(o, &preserved_خانہ_نمبر, خانہ));

            اگر (preserved_خانہ_نمبر != خانہ) {
                m3type_t     type = Get_ڈھیر_Type_From_Bottom(o, i);                 d_m3Assert (type != c_m3Type_none)
                IM3Operation op   = Is64_ثنائی_ہندسہ_Type(type) ? op_Copy_خانہ_64 : op_Copy_خانہ_32;

                EmitOp(o, op);
                Emit_خانہ_انحراف(o, preserved_خانہ_نمبر);
                Emit_خانہ_انحراف(o, خانہ);
            }
        }
    }

    _catch:
    واپس نتیجہ;
}

ساکن
M3_نتیجہ Compile_Loop_Or_بلاک (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_نتیجہ نتیجہ;

    IM3FuncType بلاک_Type = تہی;

_   (PreserveRegisters(o));
_   (Preserve_دلائل_And_Locals(o));

_   (Read_بلاک_Type(o, &بلاک_Type));

    اگر (i_opcode == c_waOp_loop) {
        u16 عدد_Params = Get_Func_Type_عدد_Params(بلاک_Type);
        اگر (عدد_Params) {
            // instantiate constants
            u16 عدد_اقدار = Get_عدد_بلاک_اقدار_On_ڈھیر(o);

            اگر (عدد_اقدار >= عدد_Params) {
                u16 ڈھیر_چوٹی = Get_ڈھیر_چوٹی_اشاریہ(o) + 1;

                برائے (u16 i = ڈھیر_چوٹی - عدد_Params; i < ڈھیر_چوٹی; ++i) {
                    u16      خانہ = Get_خانہ_For_ڈھیر_اشاریہ(o, i);
                    m3type_t type = Get_ڈھیر_Type_From_Bottom(o, i);

                    اگر (Is_Constant_خانہ(o, خانہ)) {
                        u16 new_خانہ = c_خانہ_غیر_مستعمل;
_                       (AllocateSlots(o, &new_خانہ, type));
_                       (Copy_ڈھیر_اشاریہ_مقصد_خانہ(o, new_خانہ, i));
                        o->wasm_ڈھیر[i] = new_خانہ;
                    }
                }
            }
        }

_       (EmitOp(o, op_Loop));
    } ورنہ {
    }

_   (Compile_بلاک(o, بلاک_Type, i_opcode));

    _catch: واپس نتیجہ;
}

#اگر d_m3HasExceptionHandling

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
ساکن
M3_نتیجہ EmitCatchStubs (IM3Compilation o)
{
    IM3_رمز_صفحہ displaced_صفحہ = تہی;
_try {
    M3CatchClause* clauses    = o->tryClauses;
    u32            عدد_Clauses = o->عدد_Try_Clauses;

    // the body must not see these: a nested try_table sets up its own
    o->tryClauses    = تہی;
    o->عدد_Try_Clauses = 0;

    برائے (u32 i = 0; i < عدد_Clauses; ++i) {
        M3CatchClause* clause = &clauses[i];

        IM3CompilationScope scope;
_       (Get_بلاک_Scope(o, &scope, clause->label_گہرائی + 1));

        IM3FuncType payload = clause->لیبل ? clause->لیبل->type : تہی;
        u16         عدد_دلائل = Get_Func_Type_عدد_Params(payload);

        // What the stub hands the label - the payload, then the exnref for a
        // _ref clause - has to be what the label takes. ResolveBlockResults
        // only counts the values, and the validator compares base types, so a
        // typed reference is only checked here: (ref null $t) reaching a label
        // that takes (ref $t) has to be rejected.
        {
            bool isLoop         = (scope->opcode == c_waOp_loop);
            u16  عدد_Label_اقدار = isLoop ? Get_Func_Type_عدد_Params(scope->type)
                                         : Get_Func_Type_عدد_Results(scope->type);

            _throwif(m3_Err_type_گنتی_Mismatch, عدد_Label_اقدار != عدد_دلائل + (clause->hasRef ? 1 : 0));

            برائے (u16 a = 0; a < عدد_Label_اقدار; ++a) {
                m3type_t given  = (a < عدد_دلائل) ? GetFuncTypeParamType(payload, a)
                                                : (m3type_t)c_m3Type_exnref;
                m3type_t wanted = isLoop ? GetFuncTypeParamType(scope->type, a)
                                         : Get_Func_Type_نتیجہ_Type(scope->type, a);

                _throwif(m3Err_typeMismatch, not Is_Sub_Type_کا(given, wanted));
            }
        }

        IM3_رمز_صفحہ stub_صفحہ;
_       (Acquire_Compilation_رمز_صفحہ(o, &stub_صفحہ));

        pc_t آغاز_PC  = Get_صفحہ_PC(stub_صفحہ);
        displaced_صفحہ = o->صفحہ;
        o->صفحہ       = stub_صفحہ;

        u16 پہلا_اشاریہ = o->ڈھیر_اشاریہ;

        برائے (u16 a = 0; a < عدد_دلائل; ++a) {
_           (ڈالو_Allocated_خانہ(o, GetFuncTypeParamType(payload, a)));
        }

        اگر (clause->hasRef) {
_           (ڈالو_Allocated_خانہ(o, c_m3Type_exnref));
        }

        // emitted even with nothing to move: this is also where an exception
        // no clause will reify gets released
_       (Ensure_رمز_صفحہ_عدد_سطریں(o, 2 * عدد_دلائل + 3 + d_m3_رمز_صفحہ_Free_سطریں_حد));

_       (EmitOp(o, op_CatchPayload));
        EmitConstant32(o, عدد_دلائل);

        برائے (u16 a = 0; a < عدد_دلائل; ++a) {
            u16 index = پہلا_اشاریہ + a;
            Emit_خانہ_انحراف(o, Get_خانہ_For_ڈھیر_اشاریہ(o, index));
            EmitConstant32(o, Is64_ثنائی_ہندسہ_Type(Get_ڈھیر_Type_From_Bottom(o, index)));
        }

        Emit_خانہ_انحراف(o, clause->hasRef ? (i32)Get_خانہ_For_ڈھیر_اشاریہ(o, پہلا_اشاریہ + عدد_دلائل) : -1);

_       (Emit_نکالو_Try_چوکھٹے(o, عدد_Try_چوکھٹے_مقصد_نکالو(o->بلاک.outer, scope)));

        اگر (scope->opcode == c_waOp_loop) {
_           (Resolve_بلاک_Results(o, scope, /* isBranch: */ true));

_           (EmitOp(o, Op_ContinueLoop(o)));
            Emit_اشارہ_گر(o, scope->pc);
_           (ریکارڈ_عقب_کنارہ(o, scope, false));
        } ورنہ اگر (scope->گہرائی == 0) {
_           (Return_اقدار(o, scope, /* isBranch: */ true));
_           (EmitOp(o, op_Return));
        } ورنہ {
_           (Resolve_بلاک_Results(o, scope, /* isBranch: */ true));
_           (EmitPatchingBranch(o, scope));
        }

        IM3_رمز_صفحہ return_مقصد = displaced_صفحہ;
        displaced_صفحہ        = تہی;

        Release_Compilation_رمز_صفحہ(o);
        o->صفحہ = return_مقصد;

        *(pc_t*)clause->stub_خانہ = آغاز_PC;

        جب (o->ڈھیر_اشاریہ > پہلا_اشاریہ) {
_           (نکالو(o));
        }
    }

} _catch:

    // thrown with a stub page installed: release it and put back the one it
    // displaced, which the caller's catch then releases
    اگر (displaced_صفحہ) {
        Release_Compilation_رمز_صفحہ(o);
        o->صفحہ = displaced_صفحہ;
    }

    واپس نتیجہ;
}


ساکن
M3_نتیجہ Compile_Try_جدول (IM3Compilation o, m3opcode_t i_opcode)
{
    M3CatchClause* clauses = تہی;
_try {
    // a catch entry has to be able to assume the operand stack is entirely in
    // slots: an unwind leaves nothing in the registers
_   (PreserveRegisters(o));
_   (Preserve_دلائل_And_Locals(o));

    IM3FuncType بلاک_Type = تہی;
_   (Read_بلاک_Type(o, &بلاک_Type));

    u32 عدد_Clauses;
_   (ReadLEB_u32(&عدد_Clauses, &o->wasm, o->wasm_انتہا));

    _throwif("too many catch clauses", عدد_Clauses > d_m3_Max_Sane_لیبل_فہرست_گنتی);

    اگر (عدد_Clauses) {
        clauses = m3_AllocArray(M3CatchClause, عدد_Clauses);
        _throwifnull(clauses);
    }

    برائے (u32 i = 0; i < عدد_Clauses; ++i) {
        u8 قسم;
_       (Read_u8(&قسم, &o->wasm, o->wasm_انتہا));
        _throwif(m3Err_wasmMalformed, قسم > 0x03);

        clauses[i].لیبل    = تہی;
        clauses[i].hasRef = (قسم == 0x01 or قسم == 0x03);

        اگر (قسم == 0x00 or قسم == 0x01) {
            u32 لیبل_اشاریہ;
_           (ReadLEB_u32(&لیبل_اشاریہ, &o->wasm, o->wasm_انتہا));
            _throwif(m3_Err_unknown_لیبل, لیبل_اشاریہ >= o->module->عدد_لیبل_فہرست);

            IM3_لیبل لیبل     = &o->module->لیبل_فہرست[لیبل_اشاریہ];
            clauses[i].لیبل = لیبل->resolved ? لیبل->resolved : لیبل;
        }

_       (ReadLEB_u32(&clauses[i].label_گہرائی, &o->wasm, o->wasm_انتہا));
    }

    // op_TryTable, the clause count, and two words per clause, all of which have
    // to land on one page: the op reads the table by offset from its own pc
_   (Ensure_رمز_صفحہ_عدد_سطریں(o, 2 * عدد_Clauses + 2 + d_m3_رمز_صفحہ_Free_سطریں_حد));

_   (EmitOp(o, op_Try_جدول));
    EmitConstant32(o, عدد_Clauses);

    برائے (u32 i = 0; i < عدد_Clauses; ++i) {
        Emit_اشارہ_گر(o, clauses[i].لیبل);
        clauses[i].stub_خانہ = Reserve_اشارہ_گر(o);
    }

    o->tryClauses    = clauses;
    o->عدد_Try_Clauses = عدد_Clauses;

_   (Compile_بلاک(o, بلاک_Type, c_wa_Op_try_جدول));

} _catch:

    o->tryClauses    = تہی;
    o->عدد_Try_Clauses = 0;

    m3_Free(clauses);

    واپس نتیجہ;
}


ساکن
M3_نتیجہ Compile_Throw (IM3Compilation o, m3opcode_t i_opcode)
{
_try {
    u32 لیبل_اشاریہ;
_   (ReadLEB_u32(&لیبل_اشاریہ, &o->wasm, o->wasm_انتہا));
    _throwif(m3_Err_unknown_لیبل, لیبل_اشاریہ >= o->module->عدد_لیبل_فہرست);

    IM3_لیبل لیبل = &o->module->لیبل_فہرست[لیبل_اشاریہ];
    اگر (لیبل->resolved) {
        لیبل = لیبل->resolved;
    }

    u16 عدد_دلائل = Get_Func_Type_عدد_Params(لیبل->type);

    // the payload is read out of slots, so nothing may still be in a register
_   (PreserveRegisters(o));

    // check the payload before emitting anything, so a mistyped throw does not
    // leave half an operation behind
    bool isPolymorphic = Is_ڈھیر_Polymorphic(o);

    اگر (not isPolymorphic) {
        _throwif(m3_Err_type_گنتی_Mismatch, Get_عدد_بلاک_اقدار_On_ڈھیر(o) < عدد_دلائل);

        برائے (u16 i = 0; i < عدد_دلائل; ++i) {
            u16 index = (u16)(o->ڈھیر_اشاریہ - عدد_دلائل + i);

            _throwif(m3Err_typeMismatch, not Is_Sub_Type_کا(Get_ڈھیر_Type_From_Bottom(o, index),
                                                         GetFuncTypeParamType(لیبل->type, i)));
        }
    }

_   (Ensure_رمز_صفحہ_عدد_سطریں(o, 2 * عدد_دلائل + 3 + d_m3_رمز_صفحہ_Free_سطریں_حد));

_   (EmitOp(o, op_Throw));
    Emit_اشارہ_گر(o, لیبل);
    EmitConstant32(o, عدد_دلائل);

    // the payload is named in the order the tag declares it, so the deepest of
    // the arguments on the stack comes first. In unreachable code there is
    // nothing on the stack to name and nothing will run this, so slot 0 stands
    // in for an operand that isn't there.
    برائے (u16 i = 0; i < عدد_دلائل; ++i) {
        m3type_t type = GetFuncTypeParamType(لیبل->type, i);

        Emit_خانہ_انحراف(o, isPolymorphic ? 0 : Get_خانہ_For_ڈھیر_اشاریہ(o, (u16)(o->ڈھیر_اشاریہ - عدد_دلائل + i)));
        EmitConstant32(o, Is64_ثنائی_ہندسہ_Type(type));
    }

    اگر (not isPolymorphic) {
        برائے (u16 i = 0; i < عدد_دلائل; ++i) {
_           (نکالو(o));
        }
    }

_   (سیٹ_ڈھیر_Polymorphic(o));

} _catch: واپس نتیجہ;
}


ساکن
M3_نتیجہ Compile_ThrowRef (IM3Compilation o, m3opcode_t i_opcode)
{
_try {
    اگر (not Is_ڈھیر_Polymorphic(o)) {
_       (Preserve_رجسٹر_If_Occupied(o, c_m3Type_i64));

        _throwif(m3Err_typeMismatch, بنیاد_Type_کا(Get_ڈھیر_چوٹی_Type(o)) != c_m3Type_exnref);

_       (EmitOp(o, op_ThrowRef));
        Emit_خانہ_انحراف(o, Get_ڈھیر_چوٹی_خانہ_نمبر(o));

_       (نکالو(o));
    }

_   (سیٹ_ڈھیر_Polymorphic(o));

} _catch: واپس نتیجہ;
}

#اگر_ختم // d_m3HasExceptionHandling

#اگر d_m3_Has_ڈھیر_Switching

ساکن
M3_نتیجہ Compile_ContNew (IM3Compilation o, m3opcode_t i_opcode)
{
_try {
    u32 type_اشاریہ;
_   (ReadLEB_u32(&type_اشاریہ, &o->wasm, o->wasm_انتہا));
    _throwif(m3_Err_type_گنتی_Mismatch, type_اشاریہ >= o->module->عدد_Func_Types);

    IM3FuncType funcType = o->module->funcTypes[type_اشاریہ];
    _throwif("type is not a continuation", not funcType->isContinuation);

    // the operands are read out of slots, so nothing may still be in a register
_   (PreserveRegisters(o));

    // cont.new $ct : [(ref null $ft)] -> [(ref $ct)], where $ct ~~ cont $ft:
    // the function has to be one this continuation type can run
    m3type_t چوٹی_Type = Get_ڈھیر_چوٹی_Type(o);
    _throwif(m3Err_typeMismatch,
             not Is_Sub_Type_کا(چوٹی_Type, Ref_Type_کا_Func_Type(funcType->contFuncType, false)));

    u16 func_خانہ = Get_ڈھیر_چوٹی_خانہ_نمبر(o);
_   (نکالو(o));

    u16 dst_خانہ;
_   (ڈالو_Allocated_خانہ(o, Ref_Type_کا_Func_Type(funcType, true)));
    dst_خانہ = Get_ڈھیر_چوٹی_خانہ_نمبر(o);

_   (Ensure_رمز_صفحہ_عدد_سطریں(o, 4 + d_m3_رمز_صفحہ_Free_سطریں_حد));
_   (EmitOp(o, op_ContNew));
    Emit_اشارہ_گر(o, funcType);
    Emit_خانہ_انحراف(o, dst_خانہ);
    Emit_خانہ_انحراف(o, func_خانہ);

} _catch: واپس نتیجہ;
}

ساکن
M3_نتیجہ Compile_ContBind (IM3Compilation o, m3opcode_t i_opcode)
{
_try {
    u32 typeIndex1, typeIndex2;
_   (ReadLEB_u32(&typeIndex1, &o->wasm, o->wasm_انتہا));
_   (ReadLEB_u32(&typeIndex2, &o->wasm, o->wasm_انتہا));
    _throwif(m3_Err_type_گنتی_Mismatch, typeIndex1 >= o->module->عدد_Func_Types);
    _throwif(m3_Err_type_گنتی_Mismatch, typeIndex2 >= o->module->عدد_Func_Types);

    IM3FuncType ct1 = o->module->funcTypes[typeIndex1];
    IM3FuncType ct2 = o->module->funcTypes[typeIndex2];
    _throwif("type is not a continuation", not ct1->isContinuation || not ct2->isContinuation);

    // the operands are read out of slots, so nothing may still be in a register
_   (PreserveRegisters(o));

    m3type_t contType = Get_ڈھیر_چوٹی_Type(o);
    _throwif(m3Err_typeMismatch, not Is_Sub_Type_کا(contType, Ref_Type_کا_Func_Type(ct1, false)));
    u16 cont_خانہ = Get_ڈھیر_چوٹی_خانہ_نمبر(o);
_   (نکالو(o));

    IM3FuncType ft1 = ct1->contFuncType ? ct1->contFuncType : ct1;
    IM3FuncType ft2 = ct2->contFuncType ? ct2->contFuncType : ct2;

    u16 عدد_Params1 = Get_Func_Type_عدد_Params(ft1);
    u16 عدد_Params2 = Get_Func_Type_عدد_Params(ft2);
    _throwif(m3Err_typeMismatch, عدد_Params1 < عدد_Params2);
    u32 عدد_Bound = عدد_Params1 - عدد_Params2;

    u16 عدد_Results1 = Get_Func_Type_عدد_Results(ft1);
    u16 عدد_Results2 = Get_Func_Type_عدد_Results(ft2);
    _throwif(m3Err_typeMismatch, عدد_Results1 != عدد_Results2);
    برائے (u16 i = 0; i < عدد_Results1; ++i) {
        _throwif(m3Err_typeMismatch, Get_Func_Type_نتیجہ_Type(ft1, i) != Get_Func_Type_نتیجہ_Type(ft2, i));
    }
    برائے (u16 i = 0; i < عدد_Params2; ++i) {
        _throwif(m3Err_typeMismatch, GetFuncTypeParamType(ft1, عدد_Bound + i) != GetFuncTypeParamType(ft2, i));
    }

    u16 argSlots[d_m3MaxContinuationPayload];
    _throwif("too many bound args", عدد_Bound > d_m3MaxContinuationPayload);

    برائے (u32 i = 0; i < عدد_Bound; ++i) {
        u32      paramIdx     = عدد_Bound - 1 - i;
        m3type_t expectedType = GetFuncTypeParamType(ft1, paramIdx);
        m3type_t actualType   = Get_ڈھیر_چوٹی_Type(o);
        _throwif(m3Err_typeMismatch, not Is_Sub_Type_کا(actualType, expectedType));
        argSlots[paramIdx] = Get_ڈھیر_چوٹی_خانہ_نمبر(o);
_       (نکالو(o));
    }

    u16 dst_خانہ;
_   (ڈالو_Allocated_خانہ(o, Ref_Type_کا_Func_Type(ct2, true)));
    dst_خانہ = Get_ڈھیر_چوٹی_خانہ_نمبر(o);

_   (Ensure_رمز_صفحہ_عدد_سطریں(o, 6 + عدد_Bound * 2 + d_m3_رمز_صفحہ_Free_سطریں_حد));
_   (EmitOp(o, op_ContBind));
    Emit_اشارہ_گر(o, ct1);
    Emit_اشارہ_گر(o, ct2);
    Emit_خانہ_انحراف(o, dst_خانہ);
    Emit_خانہ_انحراف(o, cont_خانہ);
    EmitConstant32(o, عدد_Bound);
    برائے (u32 i = 0; i < عدد_Bound; ++i) {
        Emit_خانہ_انحراف(o, argSlots[i]);
        EmitConstant32(o, Is64_ثنائی_ہندسہ_Type(GetFuncTypeParamType(ft1, i)));
    }

} _catch: واپس نتیجہ;
}

ساکن
M3_نتیجہ Compile_Suspend (IM3Compilation o, m3opcode_t i_opcode)
{
_try {
    u32 لیبل_اشاریہ;
_   (ReadLEB_u32(&لیبل_اشاریہ, &o->wasm, o->wasm_انتہا));
    _throwif(m3_Err_unknown_لیبل, لیبل_اشاریہ >= o->module->عدد_لیبل_فہرست);

    IM3_لیبل لیبل = &o->module->لیبل_فہرست[لیبل_اشاریہ];
    اگر (لیبل->resolved) {
        لیبل = لیبل->resolved;
    }
    u16 عدد_Params  = Get_Func_Type_عدد_Params(لیبل->type);
    u16 عدد_Results = Get_Func_Type_عدد_Results(لیبل->type);

_   (PreserveRegisters(o));

    u16 paramSlots[d_m3MaxContinuationPayload];
    _throwif("too many tag params", عدد_Params > d_m3MaxContinuationPayload);
    برائے (u16 i = 0; i < عدد_Params; ++i) {
        u16      pIdx         = عدد_Params - 1 - i;
        m3type_t expectedType = GetFuncTypeParamType(لیبل->type, pIdx);
        m3type_t actualType   = Get_ڈھیر_چوٹی_Type(o);
        _throwif(m3Err_typeMismatch, not Is_Sub_Type_کا(actualType, expectedType));
        paramSlots[pIdx] = Get_ڈھیر_چوٹی_خانہ_نمبر(o);
_       (نکالو(o));
    }

    u16 نتیجہ_Slots[d_m3MaxContinuationPayload];
    _throwif("too many tag results", عدد_Results > d_m3MaxContinuationPayload);
    برائے (u16 i = 0; i < عدد_Results; ++i) {
        m3type_t rtype = Get_Func_Type_نتیجہ_Type(لیبل->type, i);
_       (ڈالو_Allocated_خانہ(o, rtype));
        نتیجہ_Slots[i] = Get_ڈھیر_چوٹی_خانہ_نمبر(o);
    }

_   (Ensure_رمز_صفحہ_عدد_سطریں(o, 4 + عدد_Params * 2 + عدد_Results * 2 + d_m3_رمز_صفحہ_Free_سطریں_حد));
_   (EmitOp(o, op_Suspend));
    Emit_اشارہ_گر(o, لیبل);
    EmitConstant32(o, عدد_Params);
    برائے (u16 i = 0; i < عدد_Params; ++i) {
        Emit_خانہ_انحراف(o, paramSlots[i]);
        EmitConstant32(o, Is64_ثنائی_ہندسہ_Type(GetFuncTypeParamType(لیبل->type, i)));
    }
    EmitConstant32(o, عدد_Results);
    برائے (u16 i = 0; i < عدد_Results; ++i) {
        Emit_خانہ_انحراف(o, نتیجہ_Slots[i]);
        EmitConstant32(o, Is64_ثنائی_ہندسہ_Type(Get_Func_Type_نتیجہ_Type(لیبل->type, i)));
    }

    // the results are part of the frame here: op_Suspend clears them, and
    // whatever resumes this writes them before anything reads them
_   (ریکارڈ_محفوظ_نقطہ(o, safepoint_suspend, 0, عدد_Results, 0, تہی));

} _catch: واپس نتیجہ;
}

// resume, resume_throw and resume_throw_ref. All three run a continuation
// under a handler table and take its results; they differ only in what they
// hand it to start with - arguments, a fresh exception, or one off the stack.
//
// Emitted as: the continuation type, the slot holding it, the operands that
// belong to this form, the handler table, then the result slots. Keeping the
// tail identical is what lets one runtime path finish all three.
ساکن
M3_نتیجہ Compile_Resume (IM3Compilation o, m3opcode_t i_opcode)
{
    M3ResumeHandler* handlers      = تہی;
    IM3_رمز_صفحہ      displaced_صفحہ = تہی;
_try {
    bool isThrow    = (i_opcode == c_waOp_resumeThrow);
    bool isThrowRef = (i_opcode == c_waOp_resumeThrowRef);

    u32 type_اشاریہ;
_   (ReadLEB_u32(&type_اشاریہ, &o->wasm, o->wasm_انتہا));
    _throwif(m3_Err_type_گنتی_Mismatch, type_اشاریہ >= o->module->عدد_Func_Types);

    IM3FuncType contFuncType = o->module->funcTypes[type_اشاریہ];
    _throwif("type is not a continuation", not contFuncType->isContinuation);

    IM3FuncType innerType      = contFuncType->contFuncType;
    u16         عدد_Cont_Results = Get_Func_Type_عدد_Results(innerType);

    IM3_لیبل exn_لیبل      = تہی;
    u16    عدد_Operands = 0;

    اگر (isThrow) {
        u32 لیبل_اشاریہ;
_       (ReadLEB_u32(&لیبل_اشاریہ, &o->wasm, o->wasm_انتہا));
        _throwif(m3_Err_unknown_لیبل, لیبل_اشاریہ >= o->module->عدد_لیبل_فہرست);

        exn_لیبل = &o->module->لیبل_فہرست[لیبل_اشاریہ];

        // it is an exception that gets raised, so the tag must be one throw
        // could have raised: no results
        _throwif(m3Err_typeMismatch, Get_Func_Type_عدد_Results(exn_لیبل->type) != 0);

        عدد_Operands = Get_Func_Type_عدد_Params(exn_لیبل->type);
    } ورنہ اگر (not isThrowRef) {
        // a plain resume hands over the continuation's own parameters. An
        // aborted one never receives them, so they are unconstrained there.
        عدد_Operands = Get_Func_Type_عدد_Params(innerType);
    }

    u32 عدد_Clauses;
_   (ReadLEB_u32(&عدد_Clauses, &o->wasm, o->wasm_انتہا));

    u32 clauseLabelDepths[16];
    _throwif("too many resume clauses", عدد_Clauses > 16);

    اگر (عدد_Clauses) {
        handlers = m3_AllocArray(M3ResumeHandler, عدد_Clauses);
        _throwifnull(handlers);
    }

    برائے (u32 i = 0; i < عدد_Clauses; ++i) {
        u8 قسم;
_       (Read_u8(&قسم, &o->wasm, o->wasm_انتہا));
        _throwif(m3Err_wasmMalformed, قسم > 0x01);

        handlers[i].قسم = قسم;

        u32 لیبل_اشاریہ;
_       (ReadLEB_u32(&لیبل_اشاریہ, &o->wasm, o->wasm_انتہا));
        _throwif(m3_Err_unknown_لیبل, لیبل_اشاریہ >= o->module->عدد_لیبل_فہرست);

        IM3_لیبل لیبل         = &o->module->لیبل_فہرست[لیبل_اشاریہ];
        handlers[i].لیبل    = لیبل->resolved ? لیبل->resolved : لیبل;
        handlers[i].stubPC = تہی;

        اگر (قسم == 0x00) {
_           (ReadLEB_u32(&clauseLabelDepths[i], &o->wasm, o->wasm_انتہا));
        } ورنہ {
            // (on $e switch): the tag carries no payload, and what the switch
            // site yields is what this resume yields
            _throwif(m3Err_typeMismatch, Get_Func_Type_عدد_Params(handlers[i].لیبل->type) != 0);
            clauseLabelDepths[i] = 0;
        }
    }

    // the operands are read out of slots, so nothing may still be in a register
_   (PreserveRegisters(o));

    m3type_t cont_چوٹی_Type = Get_ڈھیر_چوٹی_Type(o);
    _throwif(m3Err_typeMismatch, not Is_Sub_Type_کا(cont_چوٹی_Type, Ref_Type_کا_Func_Type(contFuncType, false)));
    u16 cont_خانہ = Get_ڈھیر_چوٹی_خانہ_نمبر(o);
_   (نکالو(o));

    u16 exn_خانہ = 0;

    اگر (isThrowRef) {
        m3type_t exnType = Get_ڈھیر_چوٹی_Type(o);
        _throwif(m3Err_typeMismatch, بنیاد_Type_کا(exnType) != c_m3Type_exnref);
        exn_خانہ = Get_ڈھیر_چوٹی_خانہ_نمبر(o);
_       (نکالو(o));
    }

    u16 عملیہ_Slots[d_m3MaxContinuationPayload];
    _throwif("too many resume operands", عدد_Operands > d_m3MaxContinuationPayload);

    IM3FuncType عملیہ_Type = isThrow ? exn_لیبل->type : innerType;

    برائے (u16 i = 0; i < عدد_Operands; ++i) {
        u16      pIdx         = (u16)(عدد_Operands - 1 - i);
        m3type_t expectedType = GetFuncTypeParamType(عملیہ_Type, pIdx);
        m3type_t actualType   = Get_ڈھیر_چوٹی_Type(o);
        _throwif(m3Err_typeMismatch, not Is_Sub_Type_کا(actualType, expectedType));
        عملیہ_Slots[pIdx] = Get_ڈھیر_چوٹی_خانہ_نمبر(o);
_       (نکالو(o));
    }

    برائے (u32 i = 0; i < عدد_Clauses; ++i) {
        اگر (handlers[i].قسم != 0x00) {
            جاری;
        }

        IM3CompilationScope scope;
_       (Get_بلاک_Scope(o, &scope, clauseLabelDepths[i]));

        IM3_لیبل لیبل        = handlers[i].لیبل;
        u16    عدد_Payload = Get_Func_Type_عدد_Params(لیبل->type);

        m3type_t labelContType = (scope->opcode == c_waOp_loop)
                                   ? GetFuncTypeParamType(scope->type, عدد_Payload)
                                   : Get_Func_Type_نتیجہ_Type(scope->type, عدد_Payload);

        IM3_رمز_صفحہ stub_صفحہ;
_       (Acquire_Compilation_رمز_صفحہ(o, &stub_صفحہ));
        pc_t آغاز_PC  = Get_صفحہ_PC(stub_صفحہ);
        displaced_صفحہ = o->صفحہ;
        o->صفحہ       = stub_صفحہ;

        u16 پہلا_اشاریہ = o->ڈھیر_اشاریہ;
        برائے (u16 a = 0; a < عدد_Payload; ++a) {
_           (ڈالو_Allocated_خانہ(o, GetFuncTypeParamType(لیبل->type, a)));
        }

        // The handler is handed the suspended continuation.
_       (ڈالو_Allocated_خانہ(o, labelContType));

_       (Ensure_رمز_صفحہ_عدد_سطریں(o, 2 * عدد_Payload + 5 + d_m3_رمز_صفحہ_Free_سطریں_حد));
_       (EmitOp(o, op_ResumePayload));
        Emit_اشارہ_گر(o, Module_Cont_Type_کا_Ref(o->module, labelContType));
        EmitConstant32(o, عدد_Payload);
        برائے (u16 a = 0; a < عدد_Payload; ++a) {
            u16 index = پہلا_اشاریہ + a;
            Emit_خانہ_انحراف(o, Get_خانہ_For_ڈھیر_اشاریہ(o, index));
            EmitConstant32(o, Is64_ثنائی_ہندسہ_Type(Get_ڈھیر_Type_From_Bottom(o, index)));
        }
        Emit_خانہ_انحراف(o, Get_خانہ_For_ڈھیر_اشاریہ(o, پہلا_اشاریہ + عدد_Payload));

#  اگر d_m3HasExceptionHandling
        // the clause leaves every try region between the resume and its label,
        // and leaving one retires its handlers, the way a branch does
_       (Emit_نکالو_Try_چوکھٹے(o, عدد_Try_چوکھٹے_مقصد_نکالو(&o->بلاک, scope)));
#  endif

        اگر (scope->opcode == c_waOp_loop) {
_           (Resolve_بلاک_Results(o, scope, true));
_           (EmitOp(o, Op_ContinueLoop(o)));
            Emit_اشارہ_گر(o, scope->pc);
_           (ریکارڈ_عقب_کنارہ(o, scope, false));
        } ورنہ اگر (scope->گہرائی == 0) {
_           (Return_اقدار(o, scope, true));
_           (EmitOp(o, op_Return));
        } ورنہ {
_           (Resolve_بلاک_Results(o, scope, true));
_           (EmitPatchingBranch(o, scope));
        }

        IM3_رمز_صفحہ return_مقصد = displaced_صفحہ;
        displaced_صفحہ        = تہی;

        Release_Compilation_رمز_صفحہ(o);
        o->صفحہ = return_مقصد;

        handlers[i].stubPC = آغاز_PC;

        جب (o->ڈھیر_اشاریہ > پہلا_اشاریہ) {
_           (نکالو(o));
        }
    }

    u16 resSlots[d_m3MaxContinuationPayload];
    _throwif("too many cont results", عدد_Cont_Results > d_m3MaxContinuationPayload);
    برائے (u16 i = 0; i < عدد_Cont_Results; ++i) {
        m3type_t rtype = Get_Func_Type_نتیجہ_Type(innerType, i);
_       (ڈالو_Allocated_خانہ(o, rtype));
        resSlots[i] = Get_ڈھیر_چوٹی_خانہ_نمبر(o);
    }

_   (Ensure_رمز_صفحہ_عدد_سطریں(o, 8 + عدد_Operands * 2 + عدد_Clauses * 3 + عدد_Cont_Results * 2 +
                                  d_m3_رمز_صفحہ_Free_سطریں_حد));

_   (EmitOp(o, op_Resume));
    Emit_اشارہ_گر(o, contFuncType);
    Emit_خانہ_انحراف(o, cont_خانہ);
    EmitConstant32(o, isThrow ? d_m3ResumeThrow : (isThrowRef ? d_m3ResumeThrowRef : d_m3_Resume_معمول));

    اگر (isThrowRef) {
        Emit_خانہ_انحراف(o, exn_خانہ);
    } ورنہ {
        اگر (isThrow) {
            Emit_اشارہ_گر(o, exn_لیبل);
        }

        EmitConstant32(o, عدد_Operands);
        برائے (u16 i = 0; i < عدد_Operands; ++i) {
            Emit_خانہ_انحراف(o, عملیہ_Slots[i]);
            EmitConstant32(o, Is64_ثنائی_ہندسہ_Type(GetFuncTypeParamType(عملیہ_Type, i)));
        }
    }

    EmitConstant32(o, عدد_Clauses);
    برائے (u32 i = 0; i < عدد_Clauses; ++i) {
        EmitConstant32(o, handlers[i].قسم);
        Emit_اشارہ_گر(o, handlers[i].لیبل);
        Emit_اشارہ_گر(o, handlers[i].stubPC);
    }

    EmitConstant32(o, عدد_Cont_Results);
    برائے (u16 i = 0; i < عدد_Cont_Results; ++i) {
        Emit_خانہ_انحراف(o, resSlots[i]);
        EmitConstant32(o, Is64_ثنائی_ہندسہ_Type(Get_Func_Type_نتیجہ_Type(innerType, i)));
    }

    // a suspend passing through the resumed continuation leaves this frame
    // waiting here, with the continuation's results not yet taken
_   (ریکارڈ_محفوظ_نقطہ(o, safepoint_resume, 0, عدد_Cont_Results, (u16)عدد_Clauses, تہی));
#  اگر d_m3HasSnapshots
    اگر (o->snapshotMap and o->صفحہ) {
        o->snapshotMap->محفوظ_Points[o->snapshotMap->عدد_محفوظ_Points - 1].resumeType = Ref_Type_کا_Func_Type(contFuncType, true);
    }
#  endif

} _catch:
    اگر (displaced_صفحہ) {
        Release_Compilation_رمز_صفحہ(o);
        o->صفحہ = displaced_صفحہ;
    }
    m3_Free(handlers);
    واپس نتیجہ;
}


ساکن
M3_نتیجہ Compile_Switch (IM3Compilation o, m3opcode_t i_opcode)
{
_try {
    u32 type_اشاریہ, لیبل_اشاریہ;
_   (ReadLEB_u32(&type_اشاریہ, &o->wasm, o->wasm_انتہا));
_   (ReadLEB_u32(&لیبل_اشاریہ, &o->wasm, o->wasm_انتہا));
    _throwif(m3_Err_type_گنتی_Mismatch, type_اشاریہ >= o->module->عدد_Func_Types);
    _throwif(m3_Err_unknown_لیبل, لیبل_اشاریہ >= o->module->عدد_لیبل_فہرست);

    IM3FuncType ہدف_Cont_Type = o->module->funcTypes[type_اشاریہ];
    IM3_لیبل      لیبل            = &o->module->لیبل_فہرست[لیبل_اشاریہ];
    اگر (لیبل->resolved) {
        لیبل = لیبل->resolved;
    }

    _throwif("type is not a continuation", not ہدف_Cont_Type->isContinuation);

    // a switch tag carries no payload of its own: the values travel as the
    // peer's arguments, and the tag's results are what the switch site yields
    _throwif(m3Err_typeMismatch, Get_Func_Type_عدد_Params(لیبل->type) != 0);

    // switch $ct1 : [t1* (ref null $ct1)] -> [t2*]
    //   where $ct1 = cont [t1* (ref null? $ct2)] -> [..]
    //   and   $ct2 = cont [t2*] -> [..]
    //
    // The trailing parameter of $ct1 is not an operand: the peer receives the
    // continuation this instruction suspends. And what this instruction yields
    // is not $ct1's results but $ct2's parameters - the values whoever comes
    // back this way will hand over.
    IM3FuncType ہدف_Type = ہدف_Cont_Type->contFuncType;
    u16         عدد_Params  = Get_Func_Type_عدد_Params(ہدف_Type);

    _throwif("switch target takes no continuation", عدد_Params == 0);

    m3type_t    peerRefType  = GetFuncTypeParamType(ہدف_Type, (u16)(عدد_Params - 1));
    IM3FuncType selfContType = Module_Cont_Type_کا_Ref(o->module, peerRefType);

    _throwif(m3Err_typeMismatch, not selfContType);

    IM3FuncType selfType   = selfContType->contFuncType;
    u16         عدد_دلائل    = (u16)(عدد_Params - 1);
    u16         عدد_Results = Get_Func_Type_عدد_Params(selfType);

    // the operands are read out of slots, so nothing may still be in a register
_   (PreserveRegisters(o));

    m3type_t cont_چوٹی_Type = Get_ڈھیر_چوٹی_Type(o);
    _throwif(m3Err_typeMismatch, not Is_Sub_Type_کا(cont_چوٹی_Type, Ref_Type_کا_Func_Type(ہدف_Cont_Type, false)));
    u16 ہدف_Cont_خانہ = Get_ڈھیر_چوٹی_خانہ_نمبر(o);
_   (نکالو(o));

    u16 argSlots[d_m3MaxContinuationPayload];
    _throwif("too many switch params", عدد_دلائل > d_m3MaxContinuationPayload);

    برائے (u16 i = 0; i < عدد_دلائل; ++i) {
        u16      pIdx         = (u16)(عدد_دلائل - 1 - i);
        m3type_t expectedType = GetFuncTypeParamType(ہدف_Type, pIdx);
        m3type_t actualType   = Get_ڈھیر_چوٹی_Type(o);
        _throwif(m3Err_typeMismatch, not Is_Sub_Type_کا(actualType, expectedType));
        argSlots[pIdx] = Get_ڈھیر_چوٹی_خانہ_نمبر(o);
_       (نکالو(o));
    }

    u16 نتیجہ_Slots[d_m3MaxContinuationPayload];
    _throwif("too many switch results", عدد_Results > d_m3MaxContinuationPayload);

    برائے (u16 i = 0; i < عدد_Results; ++i) {
_       (ڈالو_Allocated_خانہ(o, GetFuncTypeParamType(selfType, i)));
        نتیجہ_Slots[i] = Get_ڈھیر_چوٹی_خانہ_نمبر(o);
    }

_   (Ensure_رمز_صفحہ_عدد_سطریں(o, 6 + عدد_دلائل * 2 + عدد_Results * 2 + d_m3_رمز_صفحہ_Free_سطریں_حد));
_   (EmitOp(o, op_Switch));
    Emit_اشارہ_گر(o, ہدف_Cont_Type);
    Emit_اشارہ_گر(o, لیبل);
    Emit_خانہ_انحراف(o, ہدف_Cont_خانہ);
    EmitConstant32(o, عدد_دلائل);
    برائے (u16 i = 0; i < عدد_دلائل; ++i) {
        Emit_خانہ_انحراف(o, argSlots[i]);
        EmitConstant32(o, Is64_ثنائی_ہندسہ_Type(GetFuncTypeParamType(ہدف_Type, i)));
    }
    EmitConstant32(o, عدد_Results);
    برائے (u16 i = 0; i < عدد_Results; ++i) {
        Emit_خانہ_انحراف(o, نتیجہ_Slots[i]);
        EmitConstant32(o, Is64_ثنائی_ہندسہ_Type(GetFuncTypeParamType(selfType, i)));
    }

    // as with suspend: op_Switch clears the results before it leaves
_   (ریکارڈ_محفوظ_نقطہ(o, safepoint_suspend, 0, عدد_Results, 0, تہی));

} _catch: واپس نتیجہ;
}

#اگر_ختم // d_m3HasStackSwitching


ساکن
M3_نتیجہ Compile_Else_بلاک (IM3Compilation o, pc_t* o_آغاز_PC, IM3FuncType i_بلاک_Type)
{
    IM3_رمز_صفحہ saved_صفحہ = o->صفحہ;
_try {

    IM3_رمز_صفحہ else_صفحہ;
_   (Acquire_Compilation_رمز_صفحہ(o, &else_صفحہ));

    *o_آغاز_PC = Get_صفحہ_PC(else_صفحہ);

    o->صفحہ = else_صفحہ;

_   (Compile_بلاک(o, i_بلاک_Type, c_waOp_else));

_   (EmitOp(o, op_Branch));
    Emit_اشارہ_گر(o, Get_صفحہ_PC(saved_صفحہ));
} _catch:

    اگر (o->صفحہ != saved_صفحہ) {
        Release_Compilation_رمز_صفحہ(o);
    }

    o->صفحہ = saved_صفحہ;

    واپس نتیجہ;
}

ساکن
M3_نتیجہ Compile_If (IM3Compilation o, m3opcode_t i_opcode)
{
    /*      [   op_If   ]
            [ <else-pc> ]   ---->   [ ..else..  ]
            [  ..if..   ]           [ ..block.. ]
            [ ..block.. ]           [ op_Branch ]
            [    end    ]  <-----   [  <end-pc> ]       */

_try {

    // Spec: if condition must be i32
    اگر (not Is_ڈھیر_Polymorphic(o)) {
        m3type_t condType = Get_ڈھیر_چوٹی_Type(o);
        _throwif(m3Err_typeMismatch, condType != c_m3Type_none and condType != c_m3Type_i32);
    }

_   (Preserve_Non_چوٹی_Registers(o));
_   (Preserve_دلائل_And_Locals(o));

#اگر d_m3FuseBranch
    IM3Operation fuseOp = GetBranchFusionOp(o, true);

    اگر (fuseOp) {
        // absorb the if into the compare that produced the condition
        *(IM3Operation*)o->foldPatchPC = fuseOp;
        InvalidateFold(o);

_       (نکالو(o));                          // the condition is consumed inside the fused op
    } ورنہ
#اگر_ختم
    {
        IM3Operation op = Is_ڈھیر_چوٹی_اندر_رجسٹر(o) ? op_If_r : op_If_s;

_       (EmitOp(o, op));
_       (Emit_خانہ_عدد_کا_ڈھیر_چوٹی_And_نکالو(o));
    }

    pc_t* pc = (pc_t*)Reserve_اشارہ_گر(o);

    IM3FuncType بلاک_Type = تہی;
_   (Read_بلاک_Type(o, &بلاک_Type));     //  dump_type_stack (o);

    u16 ڈھیر_اشاریہ = o->ڈھیر_اشاریہ;

_   (Compile_بلاک(o, بلاک_Type, i_opcode));

    اگر (o->پچھلا_Opcode == c_waOp_else) {
        o->ڈھیر_اشاریہ = ڈھیر_اشاریہ;
_       (Compile_Else_بلاک(o, pc, بلاک_Type));
    } ورنہ {
        // if block produces values and there isn't a defined else
        // case, then we need to make one up so that the pass-through
        // results end up in the right place
        اگر (Get_Func_Type_عدد_Results(بلاک_Type)) {
            // rewind to the if's end to create a fake else block
            o->wasm--;
            o->ڈھیر_اشاریہ = ڈھیر_اشاریہ;     // dump_type_stack (o);

_           (Compile_Else_بلاک(o, pc, بلاک_Type));
        } ورنہ {
            *pc = GetPC(o);
        }
    }

    } _catch: واپس نتیجہ;
}

ساکن
M3_نتیجہ Compile_Select (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_نتیجہ نتیجہ = m3Err_none;

    u16 slots[3] = { c_خانہ_غیر_مستعمل, c_خانہ_غیر_مستعمل, c_خانہ_غیر_مستعمل };

    IM3Operation op = تہی;

    m3type_t type = Get_ڈھیر_Type_From_چوٹی(o, 1); // get type of selection

    اگر (not Is_ڈھیر_Polymorphic(o)) {
        // Spec: the condition operand (top) must be i32
        m3type_t condType = Get_ڈھیر_Type_From_چوٹی(o, 0);
        _throwif(m3Err_typeMismatch, condType != c_m3Type_none and condType != c_m3Type_i32);

        // Spec: the two value operands (below condition) must have matching types
        m3type_t type2 = Get_ڈھیر_Type_From_چوٹی(o, 2);
        _throwif(m3Err_typeMismatch, type != c_m3Type_none and type2 != c_m3Type_none and
                                       not (Is_Sub_Type_کا(type, type2) or Is_Sub_Type_کا(type2, type)));
    }

    اگر (IsFpType(type)) {
#اگر d_m3HasFloat
        // not consuming a fp reg, so preserve
        اگر (not Is_ڈھیر_چوٹی_Minus1_اندر_رجسٹر(o) and
            not Is_ڈھیر_چوٹی_Minus2_اندر_رجسٹر(o)) {
_           (Preserve_رجسٹر_If_Occupied(o, type));
        }

        bool selector_اندر_Reg = Is_ڈھیر_چوٹی_اندر_رجسٹر(o);
        slots[0]           = Get_ڈھیر_چوٹی_خانہ_نمبر(o);
_       (نکالو(o));

        u32 op_اشاریہ = 0;

        برائے (u32 i = 1; i <= 2; ++i) {
            اگر (Is_ڈھیر_چوٹی_اندر_رجسٹر(o)) {
                op_اشاریہ = i;
            } ورنہ {
                slots[i] = Get_ڈھیر_چوٹی_خانہ_نمبر(o);
            }

_           (نکالو(o));
        }

        op = c_fpSelectOps[type - c_m3Type_f32][selector_اندر_Reg][op_اشاریہ];
#ورنہ
        _throw(m3Err_unknownOpcode);
#اگر_ختم
    } ورنہ اگر (IsIntType(type) or IsRefType(type)) {
        // 'sss' operation doesn't consume a register, so might have to protected its contents
        اگر (not Is_ڈھیر_چوٹی_اندر_رجسٹر(o) and
            not Is_ڈھیر_چوٹی_Minus1_اندر_رجسٹر(o) and
            not Is_ڈھیر_چوٹی_Minus2_اندر_رجسٹر(o)) {
_           (Preserve_رجسٹر_If_Occupied(o, type));
        }

        u32 op_اشاریہ = 3;  // op_Select_*_sss

        برائے (u32 i = 0; i < 3; ++i) {
            اگر (Is_ڈھیر_چوٹی_اندر_رجسٹر(o)) {
                op_اشاریہ = i;
            } ورنہ {
                slots[i] = Get_ڈھیر_چوٹی_خانہ_نمبر(o);
            }

_           (نکالو(o));
        }

        // a reference is a pointer-sized integer as far as select is concerned
        u32 type_اشاریہ = IsRefType(type) ? ((M3_SIZEOF_PTR == 8) ? 1 : 0)
                                        : (u32)(type - c_m3Type_i32);

        op = c_intSelectOps[type_اشاریہ][op_اشاریہ];
    } ورنہ اگر (not Is_ڈھیر_Polymorphic(o)) {
        _throw(m3_Err_function_ڈھیر_Underrun);
    }

    EmitOp(o, op);
    برائے (u32 i = 0; i < 3; i++) {
        اگر (Is_Valid_خانہ(slots[i])) {
            Emit_خانہ_انحراف(o, slots[i]);
        }
    }
_   (ڈالو_رجسٹر(o, type));

    _catch: واپس نتیجہ;
}

ساکن
M3_نتیجہ Compile_Drop (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_نتیجہ نتیجہ = نکالو(o);
    اگر (d_m3_Log_Wasm_ڈھیر) {
        dump_type_ڈھیر(o);
    }
    واپس نتیجہ;
}

ساکن
M3_نتیجہ Compile_Nop (IM3Compilation o, m3opcode_t i_opcode)
{
    واپس m3Err_none;
}

ساکن
M3_نتیجہ Compile_Unreachable (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_نتیجہ نتیجہ;

_   (جوڑو_Trap_ریکارڈ(o));

_   (EmitOp(o, op_Unreachable));
_   (سیٹ_ڈھیر_Polymorphic(o));

    _catch:
    واپس نتیجہ;
}


// OPTZ: currently all stack slot indices take up a full word, but
// dual stack source operands could be packed together
ساکن
M3_نتیجہ Compile_Operator (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_نتیجہ نتیجہ;

    // Declared before the first throw below, which jumps past this point to
    // _catch and so must not skip an initialization.
    IM3Operation op = تہی;

    IM3OpInfo opInfo = GetOpInfo(i_opcode);
    _throwif(m3Err_unknownOpcode, not opInfo);

    // Spec: validate operand types for load/store operations
    اگر (not Is_ڈھیر_Polymorphic(o)) {
        // For load ops (stackOffset == 0, unary), the operand is always i32 (address)
        اگر (i_opcode >= 0x28 and i_opcode <= 0x35) {
            m3type_t چوٹی_Type = Get_ڈھیر_چوٹی_Type(o);
            _throwif(m3Err_typeMismatch, چوٹی_Type != c_m3Type_none and چوٹی_Type != c_m3Type_i32);
        }

        // For store ops (stackOffset == -2), the address operand must be i32
        اگر (i_opcode >= 0x36 and i_opcode <= 0x3e) {
            m3type_t addrType = Get_ڈھیر_Type_From_چوٹی(o, 1);
            _throwif(m3Err_typeMismatch, addrType != c_m3Type_none and addrType != c_m3Type_i32);
        }
    }

    // This preserve is for for FP compare operations.
    // either need additional slot destination operations or the
    // easy fix, move _r0 out of the way.
    // moving out the way might be the optimal solution most often?
    // otherwise, the _r0 reg can get buried down in the stack
    // and be idle & wasted for a moment.
    اگر (M3_UNLIKELY(IsFpType(Get_ڈھیر_چوٹی_Type(o)) and IsIntType(opInfo->type))) {
_       (Preserve_رجسٹر_If_Occupied(o, opInfo->type));
    }

    اگر (opInfo->ڈھیر_انحراف == 0) {
        اگر (Is_ڈھیر_چوٹی_اندر_رجسٹر(o)) {
            op = opInfo->operations[0]; // _s
        } ورنہ {
_           (Preserve_رجسٹر_If_Occupied(o, opInfo->type));
            op = opInfo->operations[1]; // _r
        }
    } ورنہ {
        اگر (Is_ڈھیر_چوٹی_اندر_رجسٹر(o)) {
            op = opInfo->operations[0];  // _rs

            اگر (Is_ڈھیر_چوٹی_Minus1_اندر_رجسٹر(o)) {    d_m3Assert (i_opcode == c_waOp_store_f32 or i_opcode == c_waOp_store_f64);
                op = opInfo->operations[3]; // _rr for fp.store
            }
        } ورنہ اگر (Is_ڈھیر_چوٹی_Minus1_اندر_رجسٹر(o)) {
            op = opInfo->operations[1]; // _sr

            اگر (not op) {  // must be commutative, then
                op = opInfo->operations[0];
            }
        } ورنہ {
_           (Preserve_رجسٹر_If_Occupied(o, opInfo->type));     // _ss
            op = opInfo->operations[2];
        }
    }

    اگر (M3_LIKELY(op)) {
_       (EmitOp(o, op));

#اگر d_m3_Fold_سیٹ_Local
        // the op word just written (EmitOp may have bridged pages); NULL in the page-less compile-walk mode
        pc_t patchPC = o->صفحہ ? GetPC(o) - 1 : تہی;
#اگر_ختم

_       (Emit_خانہ_عدد_کا_ڈھیر_چوٹی_And_نکالو(o));

        اگر (opInfo->ڈھیر_انحراف < 0) {
_           (Emit_خانہ_عدد_کا_ڈھیر_چوٹی_And_نکالو(o));
        }

        اگر (opInfo->type != c_m3Type_none) {
_           (ڈالو_رجسٹر(o, opInfo->type));

#اگر d_m3_Fold_سیٹ_Local
            // record this op as a fold candidate for an immediately following local.set;
            // the operand-form index is recovered by matching against the variant table
            اگر (patchPC) {
                برائے (u8 form = 0; form < 4; ++form) {
                    اگر (opInfo->operations[form] == op) {
                        o->foldPatchPC    = patchPC;
                        o->foldOpcode     = i_opcode;
                        o->foldForm       = form;
                        o->fold_ڈھیر_اشاریہ = (u16)Get_ڈھیر_چوٹی_اشاریہ(o);
                        توڑو;
                    }
                }
            }
#اگر_ختم
        }
    } ورنہ {
#اگر_تعریف_شدہ DEBUG
        نتیجہ = ErrorCompile("no operation found for opcode", o, "'%s'", opInfo->name);
#ورنہ
        نتیجہ = ErrorCompile("no operation found for opcode", o, "%x", i_opcode);
#اگر_ختم
        _throw(نتیجہ);
    }

    _catch: واپس نتیجہ;
}

ساکن
M3_نتیجہ Compile_Convert (IM3Compilation o, m3opcode_t i_opcode)
{
_try {
    IM3OpInfo opInfo = GetOpInfo(i_opcode);
    _throwif(m3Err_unknownOpcode, not opInfo);

    // Spec: validate source operand type for conversion instructions
    اگر (not Is_ڈھیر_Polymorphic(o)) {
        u8 ماخذ_Type = c_m3Type_none;
        انتخاب (i_opcode)
        // clang-format off
        {
        حالت 0xa7:              // i32.wrap/i64
            ماخذ_Type = c_m3Type_i64; توڑو;
        حالت 0xa8: حالت 0xa9:   // i32.trunc_s/f32, i32.trunc_u/f32
        حالت 0xae: حالت 0xaf:   // i64.trunc_s/f32, i64.trunc_u/f32
        حالت 0xbb:              // f64.promote/f32
        حالت 0xbc:              // i32.reinterpret/f32
            ماخذ_Type = c_m3Type_f32; توڑو;
        حالت 0xaa: حالت 0xab:   // i32.trunc_s/f64, i32.trunc_u/f64
        حالت 0xb0: حالت 0xb1:   // i64.trunc_s/f64, i64.trunc_u/f64
        حالت 0xb6:              // f32.demote/f64
        حالت 0xbd:              // i64.reinterpret/f64
            ماخذ_Type = c_m3Type_f64; توڑو;
        حالت 0xac: حالت 0xad:   // i64.extend_s/i32, i64.extend_u/i32
        حالت 0xb2: حالت 0xb3:   // f32.convert_s/i32, f32.convert_u/i32
        حالت 0xb7: حالت 0xb8:   // f64.convert_s/i32, f64.convert_u/i32
        حالت 0xbe:              // f32.reinterpret/i32
            ماخذ_Type = c_m3Type_i32; توڑو;
        حالت 0xb4: حالت 0xb5:   // f32.convert_s/i64, f32.convert_u/i64
        حالت 0xb9: حالت 0xba:   // f64.convert_s/i64, f64.convert_u/i64
        حالت 0xbf:              // f64.reinterpret/i64
            ماخذ_Type = c_m3Type_i64; توڑو;
        طےشدہ: توڑو;
        }
        // clang-format on

        اگر (ماخذ_Type != c_m3Type_none) {
            m3type_t چوٹی_Type = Get_ڈھیر_چوٹی_Type(o);
            _throwif(m3Err_typeMismatch, چوٹی_Type != c_m3Type_none and not Is_Sub_Type_کا(چوٹی_Type, ماخذ_Type));
        }
    }

    bool dest_اندر_خانہ   = Is_رجسٹر_Type_Allocated(o, opInfo->type);
    bool ماخذ_اندر_خانہ = Is_ڈھیر_چوٹی_اندر_خانہ(o);

    IM3Operation op = opInfo->operations[dest_اندر_خانہ * 2 + ماخذ_اندر_خانہ];

_   (EmitOp(o, op));
_   (Emit_خانہ_عدد_کا_ڈھیر_چوٹی_And_نکالو(o));

    اگر (dest_اندر_خانہ) {
_       (ڈالو_Allocated_خانہ_And_Emit_2(o, opInfo->type))
    } ورنہ {
_       (ڈالو_رجسٹر(o, opInfo->type))
    }
}
    _catch: واپس نتیجہ;
}

#اگر d_m3HasWideArithmetic

// The wide arithmetic proposal: i64.add128 and i64.sub128 over two 128-bit values
// given as i64 halves, and i64.mul_wide_s/u giving the full product of two i64s.
// Each gives back a 128-bit result the same way, low half first. The operands are
// all read from slots, and of the two results the high half, which ends up on top,
// goes to _r0 and the low half to a newly allocated slot - so every operation has
// the one form, whatever shape the stack was in.
ساکن
M3_نتیجہ Compile_WideArithmetic (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_نتیجہ نتیجہ = m3Err_none;

    IM3Operation op          = op_i64_Add128;
    u32          عدد_Operands = 4;

    انتخاب (i_opcode)
    // clang-format off
    {
    حالت c_waOp_i64Sub128:   op = op_i64_Sub128;                          توڑو;
    حالت c_waOp_i64MulWideS: op = op_i64_MultiplyWide; عدد_Operands = 2;   توڑو;
    حالت c_waOp_i64MulWideU: op = op_u64_MultiplyWide; عدد_Operands = 2;   توڑو;
    طےشدہ: توڑو;
    }
    // clang-format on

_   (Preserve_رجسٹر_If_Occupied(o, c_m3Type_i64));

_   (EmitOp(o, op));

    // the top of the stack first: the high half of the second operand, for the
    // 128-bit ones
    برائے (u32 i = 0; i < عدد_Operands; ++i) {
_       (Emit_خانہ_عدد_کا_ڈھیر_چوٹی_And_نکالو(o));
    }

_   (ڈالو_Allocated_خانہ_And_Emit_2(o, c_m3Type_i64));      // the low half
_   (ڈالو_رجسٹر(o, c_m3Type_i64));                  // the high half

    _catch: واپس نتیجہ;
}

#اگر_ختم // d_m3HasWideArithmetic

#اگر d_m3HasMemory64

// Replaces the 64-bit address operand of a load or store with the checked
// 32-bit effective address op_CheckAddr64 leaves behind, so that the access
// itself compiles exactly as it would against a 32-bit memory: one slot to read
// the address from, and an offset of zero folded in already.
//
// Where the address sits under other operands - the value of a store, the
// operands of an atomic - it is rewritten where it stands rather than popped:
// its stack entry is repointed at the scratch slot and retyped, and the slot it
// used to occupy released. i_numOperands counts the address and all above it.
ساکن
M3_نتیجہ EmitCheckAddr64 (IM3Compilation o, u64 i_انحراف, u16 i_عدد_Operands)
{
    M3_نتیجہ نتیجہ = m3Err_none;

    // The address has to be in a slot for op_CheckAddr64 to read it. Spilling
    // r0 wholesale also keeps the value of a store out of it, which is what
    // leaves the address as the one operand still in a register to worry about.
_   (Preserve_رجسٹر_If_Occupied(o, c_m3Type_i64));

    // unreachable code: the operands the instruction names may not be there
    اگر (o->ڈھیر_اشاریہ < o->بلاک.بلاک_ڈھیر_اشاریہ + i_عدد_Operands) {
        واپس نتیجہ;
    }

    {
        u16      ڈھیر_اشاریہ = (u16)(o->ڈھیر_اشاریہ - i_عدد_Operands);
        u16      src_خانہ    = o->wasm_ڈھیر[ڈھیر_اشاریہ];
        m3type_t srcType    = o->type_ڈھیر[ڈھیر_اشاریہ];

        u16 dst_خانہ = c_خانہ_غیر_مستعمل;

        // Checked here rather than left to the validator, which a build may
        // have switched off: reading a slot as an i64 that only holds an i32
        // would reach past what was allocated for it.
        _throwif(m3Err_typeMismatch, بنیاد_Type_کا(srcType) != c_m3Type_i64);

        // an address is an integer, so nothing should be left in a register
        // once r0 has been preserved
        _throwif(m3Err_typeMismatch, Is_رجسٹر_خانہ_Alias(src_خانہ));
        _throwif(m3_Err_function_ڈھیر_Underrun, src_خانہ >= o->خانہ_Max_Allocated_اشاریہ_Plus_One);

        // allocated before the source is released, so the two cannot be the
        // same slot - op_CheckAddr64 reads one and writes the other
_       (AllocateSlots(o, &dst_خانہ, c_m3Type_i32));

_       (EmitOp(o, op_CheckAddr64));
        Emit_خانہ_انحراف(o, src_خانہ);
        Emit_خانہ_انحراف(o, dst_خانہ);
        EmitConstant64(o, i_انحراف);

        o->wasm_ڈھیر[ڈھیر_اشاریہ] = dst_خانہ;
        o->type_ڈھیر[ڈھیر_اشاریہ] = c_m3Type_i32;

        // locals and the constant table outlive the instruction; a dynamic
        // slot the address was computed into does not
        اگر (src_خانہ >= o->خانہ_پہلا_Dynamic_اشاریہ) {
            Deallocate_خانہ(o, src_خانہ, srcType);
        }
    }

    _catch: واپس نتیجہ;
}

#اگر_ختم // d_m3HasMemory64


ساکن
M3_نتیجہ Compile_بھرو_Store (IM3Compilation o, m3opcode_t i_opcode)
{
_try {
    u32 alignHint, حافظہ_Idx;
    u64 حافظہ_انحراف;

    // alignHint is checked by the validator
_   (Read_حافظہ_Arg(&alignHint, &حافظہ_Idx, &حافظہ_انحراف, &o->wasm, o->wasm_انتہا));
                                                                        m3log (compile, d_indent " (memory = %d; offset = %llu)", get_indention_string (o), حافظہ_Idx, (غیرعلامتی طویل طویل) حافظہ_انحراف);
    _throwif(m3_Err_unknown_حافظہ, حافظہ_Idx >= o->module->عدد_Memories);

    IM3OpInfo opInfo = GetOpInfo(i_opcode);
    _throwif(m3Err_unknownOpcode, not opInfo);

    bool isMemory64 = o->module->memories[حافظہ_Idx]->isMemory64;

    // Spec: the static offset has to be in range of the address type. Checked
    // here as well as in the validator, so that a build without one still
    // refuses the module rather than silently truncating the offset.
    _throwif(m3Err_wasmMalformed, not isMemory64 and حافظہ_انحراف > 0xFFFFFFFFull);

    اگر (IsFpType(opInfo->type)) {
_       (Preserve_رجسٹر_If_Occupied(o, c_m3Type_f64));
    }

    اگر (حافظہ_Idx) {
_       (Emit_سیٹ_حافظہ(o, حافظہ_Idx));
    }

#اگر d_m3HasMemory64
    اگر (isMemory64) {
        // An offset this far out puts every address out of bounds, so rather
        // than carrying it, clamp it to the limit: op_CheckAddr64 then traps
        // on whatever address it is given, which is the outcome either way.
        اگر (حافظہ_انحراف > d_m3_پتہ_حد_بندی) {
            حافظہ_انحراف = d_m3_پتہ_حد_بندی;
        }

_       (EmitCheckAddr64(o, حافظہ_انحراف, opInfo->ڈھیر_انحراف < 0 ? 2 : 1));

        // op_CheckAddr64 has folded the offset in already
        حافظہ_انحراف = 0;
    }
#اگر_ختم

_   (Compile_Operator(o, i_opcode));

    EmitConstant32(o, (u32)حافظہ_انحراف);

    اگر (حافظہ_Idx) {
_       (Emit_سیٹ_حافظہ(o, 0));
    }
}
    _catch: واپس نتیجہ;
}


#اگر d_m3HasAtomics

// What an atomic instruction takes and gives back, from its sub-opcode. The
// accesses come in runs of seven, one per combination of value type and cell width:
// i32/32, i64/64, i32/8, i32/16, i64/8, i64/16, i64/32. Those runs are told apart by
// where they start.
//
// The operand count includes the address. The top operand is the one that ends up
// in a register, so its type is the one to check on the way out; a store has no
// result, which is c_m3Type_none.
ساکن
خالی GetAtomicShape (u32 i_sub, u16* o_عدد_Operands, m3type_t* o_چوٹی_Type, m3type_t* o_نتیجہ_Type)
{
    // clang-format off
    ساکن ثابت m3type_t c_قدر_Types [7] = {
        c_m3Type_i32, c_m3Type_i64,
        c_m3Type_i32, c_m3Type_i32,
        c_m3Type_i64, c_m3Type_i64, c_m3Type_i64
    };
    // clang-format on

    اگر (i_sub == 0x00) {                    // memory.atomic.notify: address, count
        *o_عدد_Operands = 2;
        *o_چوٹی_Type     = c_m3Type_i32;
        *o_نتیجہ_Type  = c_m3Type_i32;
    } ورنہ اگر (i_sub <= 0x02) {             // memory.atomic.wait32/64: address, expected, timeout
        *o_عدد_Operands = 3;
        *o_چوٹی_Type     = c_m3Type_i64;
        *o_نتیجہ_Type  = c_m3Type_i32;
    } ورنہ اگر (i_sub <= 0x16) {             // loads: address
        *o_عدد_Operands = 1;
        *o_چوٹی_Type     = c_m3Type_i32;
        *o_نتیجہ_Type  = c_قدر_Types[i_sub - 0x10];
    } ورنہ اگر (i_sub <= 0x1d) {             // stores: address, value
        *o_عدد_Operands = 2;
        *o_چوٹی_Type     = c_قدر_Types[i_sub - 0x17];
        *o_نتیجہ_Type  = c_m3Type_none;
    } ورنہ اگر (i_sub <= 0x47) {             // read-modify-write: address, value
        *o_عدد_Operands = 2;
        *o_چوٹی_Type     = c_قدر_Types[(i_sub - 0x1e) % 7];
        *o_نتیجہ_Type  = *o_چوٹی_Type;
    } ورنہ {                                // cmpxchg: address, expected, replacement
        *o_عدد_Operands = 3;
        *o_چوٹی_Type     = c_قدر_Types[i_sub - 0x48];
        *o_نتیجہ_Type  = *o_چوٹی_Type;
    }
}


// The description an access takes as its last immediate: see m3_exec_atomic.h
ساکن
u32 GetAtomicDesc (u32 i_sub)
{
    // per variant, the log2 of the width of the cell
    ساکن ثابت u8 c_log2Widths[7] = { 2, 3, 0, 1, 0, 1, 2 };

    u32 variant, قسم = 0;

    اگر (i_sub <= 0x16) {
        variant = i_sub - 0x10;
    } ورنہ اگر (i_sub <= 0x1d) {
        variant = i_sub - 0x17;
    } ورنہ اگر (i_sub <= 0x47) {
        variant = (i_sub - 0x1e) % 7;
        قسم    = (i_sub - 0x1e) / 7;         // add, sub, and, or, xor, xchg
    } ورنہ {
        variant = i_sub - 0x48;
    }

    // variants 1 and 4..6 are the i64 ones
    u32 is64 = (variant == 1 or variant >= 4) ? 1u : 0u;

    واپس c_log2Widths[variant] | (is64 << 2) | (قسم << 3);
}


// Every atomic access is one op, whose operands are the topmost in _r0 and the rest
// in slots, topmost first. The address is always the deepest, so it is the last slot
// (or, for a load, the register), followed by the memarg offset. The ops check the
// address themselves: for an atomic that includes its alignment, which only the
// effective address can be asked.
ساکن
M3_نتیجہ Compile_Atomic (IM3Compilation o, m3opcode_t i_opcode)
{
_try {
    u32 alignHint, حافظہ_Idx;
    u64 حافظہ_انحراف;

    u16      عدد_Operands = 0;
    m3type_t چوٹی_Type     = c_m3Type_none;
    m3type_t نتیجہ_Type  = c_m3Type_none;

    اگر (i_opcode == c_waOp_atomicFence) {
        // the reserved byte, which the validator has checked to be zero. There
        // is nothing to order between accesses that one thread makes in sequence.
        u8 reserved;
_       (Read_u8(&reserved, &o->wasm, o->wasm_انتہا));
    } ورنہ {
        // alignHint is checked by the validator
_       (Read_حافظہ_Arg(&alignHint, &حافظہ_Idx, &حافظہ_انحراف, &o->wasm, o->wasm_انتہا));
                                                                        m3log (compile, d_indent " (memory = %d; offset = %llu)", get_indention_string (o), حافظہ_Idx, (غیرعلامتی طویل طویل) حافظہ_انحراف);
        _throwif(m3_Err_unknown_حافظہ, حافظہ_Idx >= o->module->عدد_Memories);

        IM3OpInfo opInfo = GetOpInfo(i_opcode);
        _throwif(m3Err_unknownOpcode, not opInfo);

        bool isMemory64 = o->module->memories[حافظہ_Idx]->isMemory64;

        // see Compile_Load_Store
        _throwif(m3Err_wasmMalformed, not isMemory64 and حافظہ_انحراف > 0xFFFFFFFFull);

        GetAtomicShape(i_opcode & 0xFF, &عدد_Operands, &چوٹی_Type, &نتیجہ_Type);

        اگر (حافظہ_Idx) {
_           (Emit_سیٹ_حافظہ(o, حافظہ_Idx));
        }

#  اگر d_m3HasMemory64
        اگر (isMemory64) {
            // see Compile_Load_Store
            اگر (حافظہ_انحراف > d_m3_پتہ_حد_بندی) {
                حافظہ_انحراف = d_m3_پتہ_حد_بندی;
            }

_           (EmitCheckAddr64(o, حافظہ_انحراف, عدد_Operands));
            حافظہ_انحراف = 0;
        }
#  endif

_       (Copy_ڈھیر_چوٹی_مقصد_رجسٹر(o, false));
_       (EmitOp(o, opInfo->operations[0]));
_       (نکالو_Type(o, چوٹی_Type));

        برائے (u16 i = 1; i < عدد_Operands; ++i) {
_           (Emit_خانہ_عدد_کا_ڈھیر_چوٹی_And_نکالو(o));
        }

        EmitConstant32(o, (u32)حافظہ_انحراف);
        اگر (i_opcode >= c_wa_Op_atomic_پہلا_Access) {
            EmitConstant32(o, GetAtomicDesc(i_opcode & 0xFF));
        }

        اگر (حافظہ_Idx) {
_           (Emit_سیٹ_حافظہ(o, 0));
        }

        اگر (نتیجہ_Type != c_m3Type_none) {
_           (ڈالو_رجسٹر(o, نتیجہ_Type));
        }
    }
}
    _catch: واپس نتیجہ;
}

#اگر_ختم // d_m3HasAtomics


M3_نتیجہ CompileRawFunction (IM3Module io_module, IM3Function io_function, ثابت خالی* i_function, ثابت خالی* i_userdata)
{
    d_m3Assert(io_module->runtime);

    IM3_رمز_صفحہ صفحہ = Acquire_رمز_صفحہ_With_استعداد(io_module->runtime, 4);

    اگر (صفحہ) {
        io_function->compiled = Get_صفحہ_PC(صفحہ);
        io_function->module   = io_module;

        // Unless a host module's ABI names a memory (m3_BindImportMemory), a
        // host function addresses memory 0 - what a bare i32 guest pointer means
        io_function->host_حافظہ = io_module->memory0;

        Emit_لفظ(صفحہ, op_CallRawFunction);
        Emit_لفظ(صفحہ, i_function);
        Emit_لفظ(صفحہ, io_function);
        Emit_لفظ(صفحہ, i_userdata);

        Release_رمز_صفحہ(io_module->runtime, صفحہ);
        واپس m3Err_none;
    } ورنہ {
        واپس m3_Err_malloc_Failed_رمز_صفحہ;
    }
}


// A table entry names every field it sets, so the struct can order them for
// packing; the columns follow that order, since M3OP names them positionally.
//
// d_logOp, d_logOp2 macros aren't actually used by the compiler, just codepage decoding (d_m3LogCodePages = 1)
#تعریف_کرو d_ops(...)                          .operations = { __VA_ARGS__ }

#تعریف_کرو d_logOp(OP)                         d_ops( op_##OP,                 تہی,                       تہی,                       تہی )
#تعریف_کرو d_logOp2(OP1,OP2)                   d_ops( op_##OP1,                op_##OP2,                   تہی,                       تہی )

#تعریف_کرو d_خالی_خانے_Op_List                       d_ops( تہی,                    تہی,                       تہی,                       تہی )
#تعریف_کرو d_unaryOpList(TYPE, NAME)           d_ops( op_##TYPE##_##NAME##_r,  op_##TYPE##_##NAME##_s,     تہی,                       تہی )
#تعریف_کرو d_binOpList(TYPE, NAME)             d_ops( op_##TYPE##_##NAME##_rs, op_##TYPE##_##NAME##_sr,    op_##TYPE##_##NAME##_ss,    تہی )
#تعریف_کرو d_storeFpOpList(TYPE, NAME)         d_ops( op_##TYPE##_##NAME##_rs, op_##TYPE##_##NAME##_sr,    op_##TYPE##_##NAME##_ss,    op_##TYPE##_##NAME##_rr )
#تعریف_کرو d_commutativeBinOpList(TYPE, NAME)  d_ops( op_##TYPE##_##NAME##_rs, تہی,                       op_##TYPE##_##NAME##_ss,    تہی )

#تعریف_کرو d_convertOpList(OP)                 d_ops( op_##OP##_r_r,           op_##OP##_r_s,              op_##OP##_s_r,              op_##OP##_s_s )


// Every M3Compiler the tables below name, as one list, so that the index enum and
// the array of pointers can never drift apart. Each entry must carry the same
// config guard as the function's definition.
#تعریف_کرو d_m3CompilerList(_)             \
    _( Compile_Unreachable )            \
    _( Compile_Nop )                    \
    _( Compile_Loop_Or_بلاک )            \
    _( Compile_If )                     \
    _( Compile_انتہا )                    \
    _( Compile_Branch )                 \
    _( Compile_Branch_جدول )            \
    _( Compile_Return )                 \
    _( Compile_Call )                   \
    _( Compile_Call_بالواسطہ )           \
    _( Compile_Drop )                   \
    _( Compile_Select )                 \
    _( Compile_GetLocal )               \
    _( Compile_سیٹ_Local )               \
    _( Compile_Get_سیٹ_Global )           \
    _( Compile_بھرو_Store )             \
    _( Compile_حافظہ_سائز )            \
    _( Compile_حافظہ_Grow )            \
    _( Compile_حافظہ_Init )            \
    _( Compile_حافظہ_Copy_Fill )        \
    _( Compile_ڈیٹا_Drop )              \
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
    d_m3_Compiler_List_ڈھیر_Switching( _ )

#اگر d_m3ImplementFloat
#  define d_m3CompilerList_f(_)         \
    _( Compile_Const_f32 )              \
    _( Compile_Const_f64 )
#ورنہ
#  define d_m3CompilerList_f(_)
#اگر_ختم

#اگر d_m3HasRefTypes
#  define d_m3CompilerList_refTypes(_)  \
    _( Compile_Select_Typed )           \
    _( Compile_Ref_Null )               \
    _( Compile_Ref_IsNull )             \
    _( Compile_Ref_Func )               \
    _( Compile_جدول_Get_سیٹ )           \
    _( Compile_جدول_Init )             \
    _( Compile_جدول_Copy )             \
    _( Compile_جدول_سائز )             \
    _( Compile_جدول_Grow_Fill )         \
    _( Compile_Elem_Drop )
#ورنہ
#  define d_m3CompilerList_refTypes(_)
#اگر_ختم

#اگر d_m3HasTypedRefs
#  define d_m3CompilerList_typedRefs(_) \
    _( Compile_CallRef )                \
    _( Compile_Ref_AsNonNull )
#ورنہ
#  define d_m3CompilerList_typedRefs(_)
#اگر_ختم

#اگر d_m3HasExceptionHandling
#  define d_m3CompilerList_eh(_)        \
    _( Compile_Throw )                  \
    _( Compile_ThrowRef )               \
    _( Compile_Try_جدول )
#ورنہ
#  define d_m3CompilerList_eh(_)
#اگر_ختم

#اگر d_m3HasAtomics
#  define d_m3CompilerList_atomics(_)   \
    _( Compile_Atomic )
#ورنہ
#  define d_m3CompilerList_atomics(_)
#اگر_ختم

#اگر d_m3HasWideArithmetic
#  define d_m3CompilerList_wide(_)      \
    _( Compile_WideArithmetic )
#ورنہ
#  define d_m3CompilerList_wide(_)
#اگر_ختم

#اگر d_m3_Has_ڈھیر_Switching
#  define d_m3_Compiler_List_ڈھیر_Switching(_) \
    _( Compile_ContNew )                     \
    _( Compile_ContBind )                    \
    _( Compile_Suspend )                     \
    _( Compile_Resume )                      \
    _( Compile_Switch )
#ورنہ
#  define d_m3_Compiler_List_ڈھیر_Switching(_)
#اگر_ختم

// Index 0 is "this opcode has no compiler", so that a zeroed entry - which is
// what M3OP_RESERVED and the reserved holes in the tables are - reads as one.
شمار {
    c_cc_NULL = 0,
#تعریف_کرو d_m3CompilerEnum(NAME)  c_cc_##NAME,
    d_m3CompilerList(d_m3CompilerEnum)
#تعریف_منسوخ d_m3CompilerEnum
};

ساکن ثابت M3Compiler c_compilers[] = {
    تہی,
#تعریف_کرو d_m3CompilerPtr(NAME)   NAME,
    d_m3CompilerList(d_m3CompilerPtr)
#تعریف_منسوخ d_m3CompilerPtr
};

// Names a table entry's compiler by index. d_cc (NULL) is the no-compiler entry.
#تعریف_کرو d_cc(NAME)                          .compiler = c_cc_##NAME

ساکن
M3Compiler GetCompiler (IM3OpInfo i_opInfo)
{
    d_m3Assert(i_opInfo->compiler < M3_گنتی_کا(c_compilers));
    واپس c_compilers[i_opInfo->compiler];
}

// clang-format off
ثابت M3OpInfo c_operations[] =
{
    M3OP( "unreachable",           0,  none,   d_cc(Compile_Unreachable),  d_logOp (Unreachable) ),                   // 0x00
    M3OP( "nop",                   0,  none,   d_cc(Compile_Nop),          d_خالی_خانے_Op_List ),                           // 0x01 .
    M3OP( "block",                 0,  none,   d_cc(Compile_Loop_Or_بلاک),  d_خالی_خانے_Op_List ),                           // 0x02
    M3OP( "loop",                  0,  none,   d_cc(Compile_Loop_Or_بلاک),  d_logOp (Loop) ),                          // 0x03
    M3OP( "if",                    -1, none,   d_cc(Compile_If),           d_خالی_خانے_Op_List ),                           // 0x04
    M3OP( "else",                  0,  none,   d_cc(Compile_Nop),          d_خالی_خانے_Op_List ),                           // 0x05

    M3OP_RESERVED,  M3OP_RESERVED,                                                                                    // 0x06...0x07

#اگر d_m3HasExceptionHandling
    M3OP( "throw",                 0,  none,   d_cc(Compile_Throw),        d_logOp (Throw) ),                         // 0x08
    M3OP_RESERVED,                                                                                                    // 0x09
    M3OP( "throw_ref",             0,  none,   d_cc(Compile_ThrowRef),     d_logOp (ThrowRef) ),                      // 0x0a
#ورنہ
    M3OP_RESERVED,  M3OP_RESERVED, M3OP_RESERVED,                                                                     // 0x08...0x0a
#اگر_ختم

    M3OP( "end",                   0,  none,   d_cc(Compile_انتہا),          d_خالی_خانے_Op_List ),                           // 0x0b
    M3OP( "br",                    0,  none,   d_cc(Compile_Branch),       d_logOp (Branch) ),                        // 0x0c
    M3OP( "br_if",                 -1, none,   d_cc(Compile_Branch),       d_logOp2 (BranchIf_r, BranchIf_s) ),       // 0x0d
    M3OP( "br_table",              -1, none,   d_cc(Compile_Branch_جدول),  d_logOp (Branch_جدول) ),                   // 0x0e
    M3OP( "return",                0,  any,    d_cc(Compile_Return),       d_logOp (Return) ),                        // 0x0f
    M3OP( "call",                  0,  any,    d_cc(Compile_Call),         d_logOp (Call) ),                          // 0x10
    M3OP( "call_indirect",         0,  any,    d_cc(Compile_Call_بالواسطہ), d_logOp (Call_بالواسطہ) ),                  // 0x11
    M3OP( "return_call",           0,  any,    d_cc(Compile_Call),         d_logOp (ReturnCall) ),                    // 0x12
    M3OP( "return_call_indirect",  0,  any,    d_cc(Compile_Call_بالواسطہ), d_logOp (Return_Call_بالواسطہ) ),            // 0x13

#اگر d_m3HasTypedRefs
    M3OP( "call_ref",              0,  any,    d_cc(Compile_CallRef),      d_logOp (CallRef) ),                       // 0x14
    M3OP( "return_call_ref",       0,  any,    d_cc(Compile_CallRef),      d_logOp (ReturnCallRef) ),                 // 0x15
#ورنہ
    M3OP_RESERVED,  M3OP_RESERVED,                                                                                    // 0x14...
#اگر_ختم
    M3OP_RESERVED,  M3OP_RESERVED, M3OP_RESERVED, M3OP_RESERVED,                                                      // ...0x19

    M3OP( "drop",                  -1, none,   d_cc(Compile_Drop),         d_خالی_خانے_Op_List ),                           // 0x1a
    M3OP( "select",                -2, any,    d_cc(Compile_Select),       d_خالی_خانے_Op_List ),                           // 0x1b

#اگر d_m3HasRefTypes
    M3OP( "select.t",              -2, any,    d_cc(Compile_Select_Typed), d_خالی_خانے_Op_List ),                           // 0x1c
#ورنہ
    M3OP_RESERVED,                                                                                                    // 0x1c
#اگر_ختم
    M3OP_RESERVED,  M3OP_RESERVED,                                                                                    // 0x1d...0x1e

#اگر d_m3HasExceptionHandling
    M3OP( "try_table",             0,  none,   d_cc(Compile_Try_جدول),     d_logOp (Try_جدول) ),                      // 0x1f
#ورنہ
    M3OP_RESERVED,                                                                                                    // 0x1f
#اگر_ختم

    M3OP( "local.get",             1,  any,    d_cc(Compile_GetLocal),     d_خالی_خانے_Op_List ),                           // 0x20
    M3OP( "local.set",             1,  none,   d_cc(Compile_سیٹ_Local),     d_خالی_خانے_Op_List ),                           // 0x21
    M3OP( "local.tee",             0,  any,    d_cc(Compile_سیٹ_Local),     d_خالی_خانے_Op_List ),                           // 0x22
    M3OP( "global.get",            1,  none,   d_cc(Compile_Get_سیٹ_Global), d_خالی_خانے_Op_List ),                           // 0x23
    M3OP( "global.set",            1,  none,   d_cc(Compile_Get_سیٹ_Global), d_خالی_خانے_Op_List ),                           // 0x24

#اگر d_m3HasRefTypes
    M3OP( "table.get",             0,  any,    d_cc(Compile_جدول_Get_سیٹ), d_خالی_خانے_Op_List ),                           // 0x25
    M3OP( "table.set",             -2, none,   d_cc(Compile_جدول_Get_سیٹ), d_خالی_خانے_Op_List ),                           // 0x26
#ورنہ
    M3OP_RESERVED,  M3OP_RESERVED,                                                                                    // 0x25...0x26
#اگر_ختم
    M3OP_RESERVED,                                                                                                    // 0x27

    M3OP( "i32.load",              0,  i_32,   d_cc(Compile_بھرو_Store),   d_unaryOpList (i32, بھرو_i32) ),           // 0x28
    M3OP( "i64.load",              0,  i_64,   d_cc(Compile_بھرو_Store),   d_unaryOpList (i64, بھرو_i64) ),           // 0x29
    M3OP_F( "f32.load",            0,  f_32,   d_cc(Compile_بھرو_Store),   d_unaryOpList (f32, بھرو_f32) ),           // 0x2a
    M3OP_F( "f64.load",            0,  f_64,   d_cc(Compile_بھرو_Store),   d_unaryOpList (f64, بھرو_f64) ),           // 0x2b

    M3OP( "i32.load8_s",           0,  i_32,   d_cc(Compile_بھرو_Store),   d_unaryOpList (i32, بھرو_i8) ),            // 0x2c
    M3OP( "i32.load8_u",           0,  i_32,   d_cc(Compile_بھرو_Store),   d_unaryOpList (i32, بھرو_u8) ),            // 0x2d
    M3OP( "i32.load16_s",          0,  i_32,   d_cc(Compile_بھرو_Store),   d_unaryOpList (i32, بھرو_i16) ),           // 0x2e
    M3OP( "i32.load16_u",          0,  i_32,   d_cc(Compile_بھرو_Store),   d_unaryOpList (i32, بھرو_u16) ),           // 0x2f

    M3OP( "i64.load8_s",           0,  i_64,   d_cc(Compile_بھرو_Store),   d_unaryOpList (i64, بھرو_i8) ),            // 0x30
    M3OP( "i64.load8_u",           0,  i_64,   d_cc(Compile_بھرو_Store),   d_unaryOpList (i64, بھرو_u8) ),            // 0x31
    M3OP( "i64.load16_s",          0,  i_64,   d_cc(Compile_بھرو_Store),   d_unaryOpList (i64, بھرو_i16) ),           // 0x32
    M3OP( "i64.load16_u",          0,  i_64,   d_cc(Compile_بھرو_Store),   d_unaryOpList (i64, بھرو_u16) ),           // 0x33
    M3OP( "i64.load32_s",          0,  i_64,   d_cc(Compile_بھرو_Store),   d_unaryOpList (i64, بھرو_i32) ),           // 0x34
    M3OP( "i64.load32_u",          0,  i_64,   d_cc(Compile_بھرو_Store),   d_unaryOpList (i64, بھرو_u32) ),           // 0x35

    M3OP( "i32.store",             -2, none,   d_cc(Compile_بھرو_Store),   d_binOpList (i32, Store_i32) ),            // 0x36
    M3OP( "i64.store",             -2, none,   d_cc(Compile_بھرو_Store),   d_binOpList (i64, Store_i64) ),            // 0x37
    M3OP_F( "f32.store",           -2, none,   d_cc(Compile_بھرو_Store),   d_storeFpOpList (f32, Store_f32) ),        // 0x38
    M3OP_F( "f64.store",           -2, none,   d_cc(Compile_بھرو_Store),   d_storeFpOpList (f64, Store_f64) ),        // 0x39

    M3OP( "i32.store8",            -2, none,   d_cc(Compile_بھرو_Store),   d_binOpList (i32, Store_u8) ),             // 0x3a
    M3OP( "i32.store16",           -2, none,   d_cc(Compile_بھرو_Store),   d_binOpList (i32, Store_i16) ),            // 0x3b

    M3OP( "i64.store8",            -2, none,   d_cc(Compile_بھرو_Store),   d_binOpList (i64, Store_u8) ),             // 0x3c
    M3OP( "i64.store16",           -2, none,   d_cc(Compile_بھرو_Store),   d_binOpList (i64, Store_i16) ),            // 0x3d
    M3OP( "i64.store32",           -2, none,   d_cc(Compile_بھرو_Store),   d_binOpList (i64, Store_i32) ),            // 0x3e

    M3OP( "memory.size",           1,  i_32,   d_cc(Compile_حافظہ_سائز),  d_logOp (Mem_سائز) ),                       // 0x3f
    M3OP( "memory.grow",           1,  i_32,   d_cc(Compile_حافظہ_Grow),  d_logOp (MemGrow) ),                       // 0x40

    M3OP( "i32.const",             1,  i_32,   d_cc(Compile_Const_i32),    d_logOp (Const32) ),                       // 0x41
    M3OP( "i64.const",             1,  i_64,   d_cc(Compile_Const_i64),    d_logOp (Const64) ),                       // 0x42
    M3OP_F( "f32.const",           1,  f_32,   d_cc(Compile_Const_f32),    d_خالی_خانے_Op_List ),                           // 0x43
    M3OP_F( "f64.const",           1,  f_64,   d_cc(Compile_Const_f64),    d_خالی_خانے_Op_List ),                           // 0x44

    M3OP( "i32.eqz",               0,  i_32,   d_cc(تہی),                 d_unaryOpList (i32, Equal_مقصد_Zero) ),        // 0x45
    M3OP( "i32.eq",                -1, i_32,   d_cc(تہی),                 d_commutativeBinOpList (i32, Equal) ),     // 0x46
    M3OP( "i32.ne",                -1, i_32,   d_cc(تہی),                 d_commutativeBinOpList (i32, NotEqual) ),  // 0x47
    M3OP( "i32.lt_s",              -1, i_32,   d_cc(تہی),                 d_binOpList (i32, LessThan) ),             // 0x48
    M3OP( "i32.lt_u",              -1, i_32,   d_cc(تہی),                 d_binOpList (u32, LessThan) ),             // 0x49
    M3OP( "i32.gt_s",              -1, i_32,   d_cc(تہی),                 d_binOpList (i32, GreaterThan) ),          // 0x4a
    M3OP( "i32.gt_u",              -1, i_32,   d_cc(تہی),                 d_binOpList (u32, GreaterThan) ),          // 0x4b
    M3OP( "i32.le_s",              -1, i_32,   d_cc(تہی),                 d_binOpList (i32, LessThanOrEqual) ),      // 0x4c
    M3OP( "i32.le_u",              -1, i_32,   d_cc(تہی),                 d_binOpList (u32, LessThanOrEqual) ),      // 0x4d
    M3OP( "i32.ge_s",              -1, i_32,   d_cc(تہی),                 d_binOpList (i32, GreaterThanOrEqual) ),   // 0x4e
    M3OP( "i32.ge_u",              -1, i_32,   d_cc(تہی),                 d_binOpList (u32, GreaterThanOrEqual) ),   // 0x4f

    M3OP( "i64.eqz",               0,  i_32,   d_cc(تہی),                 d_unaryOpList (i64, Equal_مقصد_Zero) ),        // 0x50
    M3OP( "i64.eq",                -1, i_32,   d_cc(تہی),                 d_commutativeBinOpList (i64, Equal) ),     // 0x51
    M3OP( "i64.ne",                -1, i_32,   d_cc(تہی),                 d_commutativeBinOpList (i64, NotEqual) ),  // 0x52
    M3OP( "i64.lt_s",              -1, i_32,   d_cc(تہی),                 d_binOpList (i64, LessThan) ),             // 0x53
    M3OP( "i64.lt_u",              -1, i_32,   d_cc(تہی),                 d_binOpList (u64, LessThan) ),             // 0x54
    M3OP( "i64.gt_s",              -1, i_32,   d_cc(تہی),                 d_binOpList (i64, GreaterThan) ),          // 0x55
    M3OP( "i64.gt_u",              -1, i_32,   d_cc(تہی),                 d_binOpList (u64, GreaterThan) ),          // 0x56
    M3OP( "i64.le_s",              -1, i_32,   d_cc(تہی),                 d_binOpList (i64, LessThanOrEqual) ),      // 0x57
    M3OP( "i64.le_u",              -1, i_32,   d_cc(تہی),                 d_binOpList (u64, LessThanOrEqual) ),      // 0x58
    M3OP( "i64.ge_s",              -1, i_32,   d_cc(تہی),                 d_binOpList (i64, GreaterThanOrEqual) ),   // 0x59
    M3OP( "i64.ge_u",              -1, i_32,   d_cc(تہی),                 d_binOpList (u64, GreaterThanOrEqual) ),   // 0x5a

    M3OP_F( "f32.eq",              -1, i_32,   d_cc(تہی),                 d_commutativeBinOpList (f32, Equal) ),     // 0x5b
    M3OP_F( "f32.ne",              -1, i_32,   d_cc(تہی),                 d_commutativeBinOpList (f32, NotEqual) ),  // 0x5c
    M3OP_F( "f32.lt",              -1, i_32,   d_cc(تہی),                 d_binOpList (f32, LessThan) ),             // 0x5d
    M3OP_F( "f32.gt",              -1, i_32,   d_cc(تہی),                 d_binOpList (f32, GreaterThan) ),          // 0x5e
    M3OP_F( "f32.le",              -1, i_32,   d_cc(تہی),                 d_binOpList (f32, LessThanOrEqual) ),      // 0x5f
    M3OP_F( "f32.ge",              -1, i_32,   d_cc(تہی),                 d_binOpList (f32, GreaterThanOrEqual) ),   // 0x60

    M3OP_F( "f64.eq",              -1, i_32,   d_cc(تہی),                 d_commutativeBinOpList (f64, Equal) ),     // 0x61
    M3OP_F( "f64.ne",              -1, i_32,   d_cc(تہی),                 d_commutativeBinOpList (f64, NotEqual) ),  // 0x62
    M3OP_F( "f64.lt",              -1, i_32,   d_cc(تہی),                 d_binOpList (f64, LessThan) ),             // 0x63
    M3OP_F( "f64.gt",              -1, i_32,   d_cc(تہی),                 d_binOpList (f64, GreaterThan) ),          // 0x64
    M3OP_F( "f64.le",              -1, i_32,   d_cc(تہی),                 d_binOpList (f64, LessThanOrEqual) ),      // 0x65
    M3OP_F( "f64.ge",              -1, i_32,   d_cc(تہی),                 d_binOpList (f64, GreaterThanOrEqual) ),   // 0x66

    M3OP( "i32.clz",               0,  i_32,   d_cc(تہی),                 d_unaryOpList (u32, Clz) ),                // 0x67
    M3OP( "i32.ctz",               0,  i_32,   d_cc(تہی),                 d_unaryOpList (u32, Ctz) ),                // 0x68
    M3OP( "i32.popcnt",            0,  i_32,   d_cc(تہی),                 d_unaryOpList (u32, Popcnt) ),             // 0x69

    M3OP( "i32.add",               -1, i_32,   d_cc(تہی),                 d_commutativeBinOpList (i32, جوڑو) ),       // 0x6a
    M3OP( "i32.sub",               -1, i_32,   d_cc(تہی),                 d_binOpList (i32, تفریق) ),             // 0x6b
    M3OP( "i32.mul",               -1, i_32,   d_cc(تہی),                 d_commutativeBinOpList (i32, Multiply) ),  // 0x6c
    M3OP( "i32.div_s",             -1, i_32,   d_cc(تہی),                 d_binOpList (i32, Divide) ),               // 0x6d
    M3OP( "i32.div_u",             -1, i_32,   d_cc(تہی),                 d_binOpList (u32, Divide) ),               // 0x6e
    M3OP( "i32.rem_s",             -1, i_32,   d_cc(تہی),                 d_binOpList (i32, Remainder) ),            // 0x6f
    M3OP( "i32.rem_u",             -1, i_32,   d_cc(تہی),                 d_binOpList (u32, Remainder) ),            // 0x70
    M3OP( "i32.and",               -1, i_32,   d_cc(تہی),                 d_commutativeBinOpList (u32, And) ),       // 0x71
    M3OP( "i32.or",                -1, i_32,   d_cc(تہی),                 d_commutativeBinOpList (u32, Or) ),        // 0x72
    M3OP( "i32.xor",               -1, i_32,   d_cc(تہی),                 d_commutativeBinOpList (u32, Xor) ),       // 0x73
    M3OP( "i32.shl",               -1, i_32,   d_cc(تہی),                 d_binOpList (u32, سرکاؤ_بایاں) ),            // 0x74
    M3OP( "i32.shr_s",             -1, i_32,   d_cc(تہی),                 d_binOpList (i32, سرکاؤ_دایاں) ),           // 0x75
    M3OP( "i32.shr_u",             -1, i_32,   d_cc(تہی),                 d_binOpList (u32, سرکاؤ_دایاں) ),           // 0x76
    M3OP( "i32.rotl",              -1, i_32,   d_cc(تہی),                 d_binOpList (u32, Rotl) ),                 // 0x77
    M3OP( "i32.rotr",              -1, i_32,   d_cc(تہی),                 d_binOpList (u32, Rotr) ),                 // 0x78

    M3OP( "i64.clz",               0,  i_64,   d_cc(تہی),                 d_unaryOpList (u64, Clz) ),                // 0x79
    M3OP( "i64.ctz",               0,  i_64,   d_cc(تہی),                 d_unaryOpList (u64, Ctz) ),                // 0x7a
    M3OP( "i64.popcnt",            0,  i_64,   d_cc(تہی),                 d_unaryOpList (u64, Popcnt) ),             // 0x7b

    M3OP( "i64.add",               -1, i_64,   d_cc(تہی),                 d_commutativeBinOpList (i64, جوڑو) ),       // 0x7c
    M3OP( "i64.sub",               -1, i_64,   d_cc(تہی),                 d_binOpList (i64, تفریق) ),             // 0x7d
    M3OP( "i64.mul",               -1, i_64,   d_cc(تہی),                 d_commutativeBinOpList (i64, Multiply) ),  // 0x7e
    M3OP( "i64.div_s",             -1, i_64,   d_cc(تہی),                 d_binOpList (i64, Divide) ),               // 0x7f
    M3OP( "i64.div_u",             -1, i_64,   d_cc(تہی),                 d_binOpList (u64, Divide) ),               // 0x80
    M3OP( "i64.rem_s",             -1, i_64,   d_cc(تہی),                 d_binOpList (i64, Remainder) ),            // 0x81
    M3OP( "i64.rem_u",             -1, i_64,   d_cc(تہی),                 d_binOpList (u64, Remainder) ),            // 0x82
    M3OP( "i64.and",               -1, i_64,   d_cc(تہی),                 d_commutativeBinOpList (u64, And) ),       // 0x83
    M3OP( "i64.or",                -1, i_64,   d_cc(تہی),                 d_commutativeBinOpList (u64, Or) ),        // 0x84
    M3OP( "i64.xor",               -1, i_64,   d_cc(تہی),                 d_commutativeBinOpList (u64, Xor) ),       // 0x85
    M3OP( "i64.shl",               -1, i_64,   d_cc(تہی),                 d_binOpList (u64, سرکاؤ_بایاں) ),            // 0x86
    M3OP( "i64.shr_s",             -1, i_64,   d_cc(تہی),                 d_binOpList (i64, سرکاؤ_دایاں) ),           // 0x87
    M3OP( "i64.shr_u",             -1, i_64,   d_cc(تہی),                 d_binOpList (u64, سرکاؤ_دایاں) ),           // 0x88
    M3OP( "i64.rotl",              -1, i_64,   d_cc(تہی),                 d_binOpList (u64, Rotl) ),                 // 0x89
    M3OP( "i64.rotr",              -1, i_64,   d_cc(تہی),                 d_binOpList (u64, Rotr) ),                 // 0x8a

    M3OP_F( "f32.abs",             0,  f_32,   d_cc(تہی),                 d_unaryOpList(f32, Abs) ),                 // 0x8b
    M3OP_F( "f32.neg",             0,  f_32,   d_cc(تہی),                 d_unaryOpList(f32, Negate) ),              // 0x8c
    M3OP_F( "f32.ceil",            0,  f_32,   d_cc(تہی),                 d_unaryOpList(f32, Ceil) ),                // 0x8d
    M3OP_F( "f32.floor",           0,  f_32,   d_cc(تہی),                 d_unaryOpList(f32, Floor) ),               // 0x8e
    M3OP_F( "f32.trunc",           0,  f_32,   d_cc(تہی),                 d_unaryOpList(f32, Trunc) ),               // 0x8f
    M3OP_F( "f32.nearest",         0,  f_32,   d_cc(تہی),                 d_unaryOpList(f32, قریب_ترین) ),             // 0x90
    M3OP_F( "f32.sqrt",            0,  f_32,   d_cc(تہی),                 d_unaryOpList(f32, Sqrt) ),                // 0x91

    M3OP_F( "f32.add",             -1, f_32,   d_cc(تہی),                 d_commutativeBinOpList (f32, جوڑو) ),       // 0x92
    M3OP_F( "f32.sub",             -1, f_32,   d_cc(تہی),                 d_binOpList (f32, تفریق) ),             // 0x93
    M3OP_F( "f32.mul",             -1, f_32,   d_cc(تہی),                 d_commutativeBinOpList (f32, Multiply) ),  // 0x94
    M3OP_F( "f32.div",             -1, f_32,   d_cc(تہی),                 d_binOpList (f32, Divide) ),               // 0x95
    M3OP_F( "f32.min",             -1, f_32,   d_cc(تہی),                 d_commutativeBinOpList (f32, Min) ),       // 0x96
    M3OP_F( "f32.max",             -1, f_32,   d_cc(تہی),                 d_commutativeBinOpList (f32, Max) ),       // 0x97
    M3OP_F( "f32.copysign",        -1, f_32,   d_cc(تہی),                 d_binOpList (f32, CopySign) ),             // 0x98

    M3OP_F( "f64.abs",             0,  f_64,   d_cc(تہی),                 d_unaryOpList(f64, Abs) ),                 // 0x99
    M3OP_F( "f64.neg",             0,  f_64,   d_cc(تہی),                 d_unaryOpList(f64, Negate) ),              // 0x9a
    M3OP_F( "f64.ceil",            0,  f_64,   d_cc(تہی),                 d_unaryOpList(f64, Ceil) ),                // 0x9b
    M3OP_F( "f64.floor",           0,  f_64,   d_cc(تہی),                 d_unaryOpList(f64, Floor) ),               // 0x9c
    M3OP_F( "f64.trunc",           0,  f_64,   d_cc(تہی),                 d_unaryOpList(f64, Trunc) ),               // 0x9d
    M3OP_F( "f64.nearest",         0,  f_64,   d_cc(تہی),                 d_unaryOpList(f64, قریب_ترین) ),             // 0x9e
    M3OP_F( "f64.sqrt",            0,  f_64,   d_cc(تہی),                 d_unaryOpList(f64, Sqrt) ),                // 0x9f

    M3OP_F( "f64.add",             -1, f_64,   d_cc(تہی),                 d_commutativeBinOpList (f64, جوڑو) ),       // 0xa0
    M3OP_F( "f64.sub",             -1, f_64,   d_cc(تہی),                 d_binOpList (f64, تفریق) ),             // 0xa1
    M3OP_F( "f64.mul",             -1, f_64,   d_cc(تہی),                 d_commutativeBinOpList (f64, Multiply) ),  // 0xa2
    M3OP_F( "f64.div",             -1, f_64,   d_cc(تہی),                 d_binOpList (f64, Divide) ),               // 0xa3
    M3OP_F( "f64.min",             -1, f_64,   d_cc(تہی),                 d_commutativeBinOpList (f64, Min) ),       // 0xa4
    M3OP_F( "f64.max",             -1, f_64,   d_cc(تہی),                 d_commutativeBinOpList (f64, Max) ),       // 0xa5
    M3OP_F( "f64.copysign",        -1, f_64,   d_cc(تہی),                 d_binOpList (f64, CopySign) ),             // 0xa6

    M3OP( "i32.wrap/i64",          0,  i_32,   d_cc(تہی),                 d_unaryOpList (i32, Wrap_i64) ),           // 0xa7
    M3OP_F( "i32.trunc_s/f32",     0,  i_32,   d_cc(Compile_Convert),      d_convertOpList (i32_Trunc_f32) ),         // 0xa8
    M3OP_F( "i32.trunc_u/f32",     0,  i_32,   d_cc(Compile_Convert),      d_convertOpList (u32_Trunc_f32) ),         // 0xa9
    M3OP_F( "i32.trunc_s/f64",     0,  i_32,   d_cc(Compile_Convert),      d_convertOpList (i32_Trunc_f64) ),         // 0xaa
    M3OP_F( "i32.trunc_u/f64",     0,  i_32,   d_cc(Compile_Convert),      d_convertOpList (u32_Trunc_f64) ),         // 0xab

    M3OP( "i64.extend_s/i32",      0,  i_64,   d_cc(تہی),                 d_unaryOpList (i64, Extend_i32) ),         // 0xac
    M3OP( "i64.extend_u/i32",      0,  i_64,   d_cc(تہی),                 d_unaryOpList (i64, Extend_u32) ),         // 0xad

    M3OP_F( "i64.trunc_s/f32",     0,  i_64,   d_cc(Compile_Convert),      d_convertOpList (i64_Trunc_f32) ),         // 0xae
    M3OP_F( "i64.trunc_u/f32",     0,  i_64,   d_cc(Compile_Convert),      d_convertOpList (u64_Trunc_f32) ),         // 0xaf
    M3OP_F( "i64.trunc_s/f64",     0,  i_64,   d_cc(Compile_Convert),      d_convertOpList (i64_Trunc_f64) ),         // 0xb0
    M3OP_F( "i64.trunc_u/f64",     0,  i_64,   d_cc(Compile_Convert),      d_convertOpList (u64_Trunc_f64) ),         // 0xb1

    M3OP_F( "f32.convert_s/i32",   0,  f_32,   d_cc(Compile_Convert),      d_convertOpList (f32_Convert_i32) ),       // 0xb2
    M3OP_F( "f32.convert_u/i32",   0,  f_32,   d_cc(Compile_Convert),      d_convertOpList (f32_Convert_u32) ),       // 0xb3
    M3OP_F( "f32.convert_s/i64",   0,  f_32,   d_cc(Compile_Convert),      d_convertOpList (f32_Convert_i64) ),       // 0xb4
    M3OP_F( "f32.convert_u/i64",   0,  f_32,   d_cc(Compile_Convert),      d_convertOpList (f32_Convert_u64) ),       // 0xb5

    M3OP_F( "f32.demote/f64",      0,  f_32,   d_cc(تہی),                 d_unaryOpList (f32, Demote_f64) ),         // 0xb6

    M3OP_F( "f64.convert_s/i32",   0,  f_64,   d_cc(Compile_Convert),      d_convertOpList (f64_Convert_i32) ),       // 0xb7
    M3OP_F( "f64.convert_u/i32",   0,  f_64,   d_cc(Compile_Convert),      d_convertOpList (f64_Convert_u32) ),       // 0xb8
    M3OP_F( "f64.convert_s/i64",   0,  f_64,   d_cc(Compile_Convert),      d_convertOpList (f64_Convert_i64) ),       // 0xb9
    M3OP_F( "f64.convert_u/i64",   0,  f_64,   d_cc(Compile_Convert),      d_convertOpList (f64_Convert_u64) ),       // 0xba

    M3OP_F( "f64.promote/f32",     0,  f_64,   d_cc(تہی),                 d_unaryOpList (f64, Promote_f32) ),        // 0xbb

    M3OP_F( "i32.reinterpret/f32", 0,  i_32,   d_cc(Compile_Convert),      d_convertOpList (i32_Reinterpret_f32) ),   // 0xbc
    M3OP_F( "i64.reinterpret/f64", 0,  i_64,   d_cc(Compile_Convert),      d_convertOpList (i64_Reinterpret_f64) ),   // 0xbd
    M3OP_F( "f32.reinterpret/i32", 0,  f_32,   d_cc(Compile_Convert),      d_convertOpList (f32_Reinterpret_i32) ),   // 0xbe
    M3OP_F( "f64.reinterpret/i64", 0,  f_64,   d_cc(Compile_Convert),      d_convertOpList (f64_Reinterpret_i64) ),   // 0xbf

    M3OP( "i32.extend8_s",         0,  i_32,   d_cc(تہی),                 d_unaryOpList (i32, Extend8_s) ),          // 0xc0
    M3OP( "i32.extend16_s",        0,  i_32,   d_cc(تہی),                 d_unaryOpList (i32, Extend16_s) ),         // 0xc1
    M3OP( "i64.extend8_s",         0,  i_64,   d_cc(تہی),                 d_unaryOpList (i64, Extend8_s) ),          // 0xc2
    M3OP( "i64.extend16_s",        0,  i_64,   d_cc(تہی),                 d_unaryOpList (i64, Extend16_s) ),         // 0xc3
    M3OP( "i64.extend32_s",        0,  i_64,   d_cc(تہی),                 d_unaryOpList (i64, Extend32_s) ),         // 0xc4

// Internal operations, for codepage logging only. They sit past c_waOp_lastCore,
// the last opcode this table covers, so GetOpInfo () can never reach them.
# ifdef DEBUG // for codepage logging. the order doesn't matter:
#تعریف_کرو d_m3DebugOp(OP) M3OP (#OP, 0, none, d_ops (op_##OP))

# اگر d_m3HasFloat
#تعریف_کرو d_m3DebugTypedOp(OP) M3OP (#OP, 0, none, d_ops (op_##OP##_i32, op_##OP##_i64, op_##OP##_f32, op_##OP##_f64))
# ورنہ
#تعریف_کرو d_m3DebugTypedOp(OP) M3OP (#OP, 0, none, d_ops (op_##OP##_i32, op_##OP##_i64))
# endif

    d_m3DebugOp (Compile),          d_m3DebugOp (اندراج_3),            d_m3DebugOp (انتہا_2),
    d_m3DebugOp (Unsupported),      d_m3DebugOp (CallRawFunction),

    d_m3DebugOp (GetGlobal_s32),    d_m3DebugOp (GetGlobal_s64),    d_m3DebugOp (ContinueLoop),     d_m3DebugOp (ContinueLoopIf),
# اگر d_m3_Has_ڈھیر_Switching
    d_m3DebugOp (ContinueLoop_Suspendable), d_m3DebugOp (ContinueLoopIf_Suspendable), d_m3DebugOp (اندراج_Check),
#  اگر !d_m3_اندراج_Keeps_چوکھٹا
    d_m3DebugOp (اندراج_Suspendable),
#  endif
# endif

    d_m3DebugOp (Copy_خانہ_32),      d_m3DebugOp (Preserve_Copy_خانہ_32), d_m3DebugOp (If_s),          d_m3DebugOp (BranchIfPrologue_s),
    d_m3DebugOp (Copy_خانہ_64),      d_m3DebugOp (Preserve_Copy_خانہ_64), d_m3DebugOp (If_r),          d_m3DebugOp (BranchIfPrologue_r),

    d_m3DebugOp (Select_i32_rss),   d_m3DebugOp (Select_i32_srs),   d_m3DebugOp (Select_i32_ssr),   d_m3DebugOp (Select_i32_sss),
    d_m3DebugOp (Select_i64_rss),   d_m3DebugOp (Select_i64_srs),   d_m3DebugOp (Select_i64_ssr),   d_m3DebugOp (Select_i64_sss),

# اگر d_m3HasFloat
    d_m3DebugOp (Select_f32_sss),   d_m3DebugOp (Select_f32_srs),   d_m3DebugOp (Select_f32_ssr),
    d_m3DebugOp (Select_f32_rss),   d_m3DebugOp (Select_f32_rrs),   d_m3DebugOp (Select_f32_rsr),

    d_m3DebugOp (Select_f64_sss),   d_m3DebugOp (Select_f64_srs),   d_m3DebugOp (Select_f64_ssr),
    d_m3DebugOp (Select_f64_rss),   d_m3DebugOp (Select_f64_rrs),   d_m3DebugOp (Select_f64_rsr),
# endif

    d_m3DebugOp (MemFill),          d_m3DebugOp (MemCopy),          d_m3DebugOp (MemInit),          d_m3DebugOp (ڈیٹا_Drop),

# اگر d_m3HasGasMetering
    d_m3DebugOp (UseGas),
# endif

# اگر d_m3HasRefTypes
    d_m3DebugOp (جدول_Get),         d_m3DebugOp (جدول_سیٹ),         d_m3DebugOp (جدول_سائز),
    d_m3DebugOp (جدول_Grow),        d_m3DebugOp (جدول_Fill),        d_m3DebugOp (جدول_Init),
    d_m3DebugOp (ElemDrop),         d_m3DebugOp (جدول_Copy),
# endif

    d_m3DebugTypedOp (سیٹ_Global),   d_m3DebugOp (سیٹ_Global_s32),    d_m3DebugOp (سیٹ_Global_s64),

    d_m3DebugTypedOp (سیٹ_رجسٹر), d_m3DebugTypedOp (سیٹ_خانہ),     d_m3DebugTypedOp (Preserve_سیٹ_خانہ),
# endif

# ifdef DEBUG
    M3OP( "termination", 0, c_m3Type_unknown, d_خالی_خانے_Op_List ) // for find_operation_info
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

    M3OP( "memory.init",           0, none,   d_cc(Compile_حافظہ_Init),     d_خالی_خانے_Op_List ),                       // 0x08
    M3OP( "data.drop",             0, none,   d_cc(Compile_ڈیٹا_Drop),       d_خالی_خانے_Op_List ),                       // 0x09

    M3OP( "memory.copy",           0, none,   d_cc(Compile_حافظہ_Copy_Fill), d_خالی_خانے_Op_List ),                       // 0x0a
    M3OP( "memory.fill",           0, none,   d_cc(Compile_حافظہ_Copy_Fill), d_خالی_خانے_Op_List ),                       // 0x0b

#اگر d_m3HasRefTypes
    M3OP( "table.init",            0, none,   d_cc(Compile_جدول_Init),      d_خالی_خانے_Op_List ),                       // 0x0c
    M3OP( "elem.drop",             0, none,   d_cc(Compile_Elem_Drop),       d_خالی_خانے_Op_List ),                       // 0x0d
    M3OP( "table.copy",            0, none,   d_cc(Compile_جدول_Copy),      d_خالی_خانے_Op_List ),                       // 0x0e
#ورنہ
    M3OP_RESERVED, M3OP_RESERVED, M3OP_RESERVED,                                                                    // 0x0c...0x0e
#اگر_ختم

#اگر d_m3HasRefTypes
    M3OP( "table.grow",            0, i_32,   d_cc(Compile_جدول_Grow_Fill),  d_خالی_خانے_Op_List ),                       // 0x0f
    M3OP( "table.size",            1, i_32,   d_cc(Compile_جدول_سائز),      d_خالی_خانے_Op_List ),                       // 0x10
    M3OP( "table.fill",            0, none,   d_cc(Compile_جدول_Grow_Fill),  d_خالی_خانے_Op_List ),                       // 0x11
#ورنہ
    M3OP_RESERVED, M3OP_RESERVED, M3OP_RESERVED,                                                                    // 0x0f...0x11
#اگر_ختم

#اگر d_m3HasWideArithmetic
    M3OP_RESERVED,                                                                                                  // 0x12

    M3OP( "i64.add128",            -2, i_64,  d_cc(Compile_WideArithmetic), d_logOp (i64_Add128) ),              // 0x13
    M3OP( "i64.sub128",            -2, i_64,  d_cc(Compile_WideArithmetic), d_logOp (i64_Sub128) ),              // 0x14
    M3OP( "i64.mul_wide_s",        0,  i_64,  d_cc(Compile_WideArithmetic), d_logOp (i64_MultiplyWide) ),        // 0x15
    M3OP( "i64.mul_wide_u",        0,  i_64,  d_cc(Compile_WideArithmetic), d_logOp (u64_MultiplyWide) ),        // 0x16
#اگر_ختم


# ifdef DEBUG
    M3OP( "termination", 0, c_m3Type_unknown, d_خالی_خانے_Op_List ) // for find_operation_info
# endif
};

#اگر d_m3HasAtomics

// The 0xFE instructions share one entry per kind rather than each having its own: what
// tells them apart, the width of the cell and the operation, is worked out from the
// sub-opcode by Compile_Atomic. GetAtomicInfoIndex () maps a sub-opcode onto this table.
شمار {
    c_opAtomic_notify,
    c_opAtomic_wait32,
    c_opAtomic_wait64,
    c_opAtomic_fence,
    c_op_Atomic_بھرو,
    c_opAtomic_store,
    c_opAtomic_rmw,
    c_opAtomic_cmpxchg
};

ثابت M3OpInfo c_operationsFE[] =
{
    M3OP( "memory.atomic.notify",  -1, i_32,  d_cc(Compile_Atomic), d_logOp (AtomicNotify) ),    // c_opAtomic_notify
    M3OP( "memory.atomic.wait32",  -2, i_32,  d_cc(Compile_Atomic), d_logOp (AtomicWait32) ),    // c_opAtomic_wait32
    M3OP( "memory.atomic.wait64",  -2, i_32,  d_cc(Compile_Atomic), d_logOp (AtomicWait64) ),    // c_opAtomic_wait64
    M3OP( "atomic.fence",           0, none,  d_cc(Compile_Atomic), d_خالی_خانے_Op_List ),             // c_opAtomic_fence
    M3OP( "atomic.load",            0, any,   d_cc(Compile_Atomic), d_logOp (Atomic_بھرو) ),      // c_opAtomic_load
    M3OP( "atomic.store",          -2, none,  d_cc(Compile_Atomic), d_logOp (AtomicStore) ),     // c_opAtomic_store
    M3OP( "atomic.rmw",            -1, any,   d_cc(Compile_Atomic), d_logOp (AtomicRmw) ),       // c_opAtomic_rmw
    M3OP( "atomic.rmw.cmpxchg",    -2, any,   d_cc(Compile_Atomic), d_logOp (AtomicCmpxchg) ),   // c_opAtomic_cmpxchg
};

ثابت u32 c_عدد_Operations_FE = M3_گنتی_کا(c_operationsFE);

// The entry a 0xFE sub-opcode uses, or -1 for one that names nothing: the spec leaves
// 0x04..0x0f and everything past cmpxchg unassigned. The accesses come in runs of seven,
// one per pairing of value type and cell width.
ساکن اندرونی
i32 Get_Atomic_Info_اشاریہ (u32 i_sub)
{
    اگر (i_sub <= 0x03) واپس (i32)i_sub;
    اگر (i_sub < 0x10) واپس -1;
    اگر (i_sub <= 0x16) واپس c_op_Atomic_بھرو;
    اگر (i_sub <= 0x1d) واپس c_opAtomic_store;
    اگر (i_sub <= 0x47) واپس c_opAtomic_rmw;
    اگر (i_sub <= c_wa_Op_آخری_Atomic) واپس c_opAtomic_cmpxchg;
    واپس -1;
}

#اگر_ختم // d_m3HasAtomics

// The opcodes above c_waOp_lastCore are sparse: the reference instructions at
// 0xd0..0xd2 and 0xd4, and the 0xfc extended-opcode prefix. Indexing c_operations out to
// 0xfc to reach them would leave ~50 empty entries there, so they get their own
// table, which GetHighOpIndex () maps the opcode onto.
شمار {
#اگر d_m3HasRefTypes
    c_op_بلند_ref_Null,
    c_op_بلند_ref_Is_Null,
    c_op_بلند_ref_Func,
#  اگر d_m3HasTypedRefs
    c_op_بلند_ref_As_Non_Null,
#  endif
#اگر_ختم
#اگر d_m3_Has_ڈھیر_Switching
    c_op_بلند_cont_New,
    c_op_بلند_cont_Bind,
    c_op_بلند_suspend,
    c_op_بلند_resume,
    c_op_بلند_resume_Throw,
    c_op_بلند_resume_Throw_Ref,
    c_op_بلند_switch,
#اگر_ختم
#اگر d_m3HasAtomics
    c_op_بلند_atomic,
#اگر_ختم
    c_op_بلند_extended   // always present, so the enum is never empty
};

// Positional, so the table must stay in c_opHigh_* order: array designators
// are not valid C++, which the maintenance builds compile this as.
ساکن ثابت M3OpInfo c_operations_بلند[] =
{
#اگر d_m3HasRefTypes
    M3OP( "ref.null",        1, any,               d_cc(Compile_Ref_Null),       d_خالی_خانے_Op_List ),  // 0xd0  c_opHigh_refNull
    M3OP( "ref.is_null",     0, i_32,              d_cc(Compile_Ref_IsNull),     d_خالی_خانے_Op_List ),  // 0xd1  c_opHigh_refIsNull
    M3OP( "ref.func",        1, any,               d_cc(Compile_Ref_Func),       d_خالی_خانے_Op_List ),  // 0xd2  c_opHigh_refFunc
#  اگر d_m3HasTypedRefs
    M3OP( "ref.as_non_null", 0, any,               d_cc(Compile_Ref_AsNonNull),  d_خالی_خانے_Op_List ),  // 0xd4  c_opHigh_refAsNonNull
#  endif
#اگر_ختم
#اگر d_m3_Has_ڈھیر_Switching
    M3OP( "cont.new",          1, any,             d_cc(Compile_ContNew),        d_خالی_خانے_Op_List ),  // 0xe0  c_opHigh_contNew
    M3OP( "cont.bind",         2, any,             d_cc(Compile_ContBind),       d_خالی_خانے_Op_List ),  // 0xe1  c_opHigh_contBind
    M3OP( "suspend",           1, any,             d_cc(Compile_Suspend),        d_خالی_خانے_Op_List ),  // 0xe2  c_opHigh_suspend
    M3OP( "resume",            1, any,             d_cc(Compile_Resume),         d_خالی_خانے_Op_List ),  // 0xe3  c_opHigh_resume
    M3OP( "resume_throw",      2, any,             d_cc(Compile_Resume)   ,    d_خالی_خانے_Op_List ),  // 0xe4  c_opHigh_resumeThrow
    M3OP( "resume_throw_ref",  1, any,             d_cc(Compile_Resume)      , d_خالی_خانے_Op_List ),  // 0xe5  c_opHigh_resumeThrowRef
    M3OP( "switch",            2, any,             d_cc(Compile_Switch),         d_خالی_خانے_Op_List ),  // 0xe6  c_opHigh_switch
#اگر_ختم

#اگر d_m3HasAtomics
    M3OP( "0xFE",            0, c_m3Type_unknown,  d_cc(Compile_ExtendedOpcode), d_خالی_خانے_Op_List ),  // 0xfe  c_opHigh_atomic
#اگر_ختم
    M3OP( "0xFC",            0, c_m3Type_unknown,  d_cc(Compile_ExtendedOpcode), d_خالی_خانے_Op_List ),  // 0xfc  c_opHigh_extended
};
M3_STATIC_ASSERT (M3_گنتی_کا (c_operations_بلند) == c_op_بلند_extended + 1, c_operations_بلند_needs_one_اندراج_فی_c_op_بلند);

// clang-format on


// Opcodes the spec reserves leave zeroed holes in the tables above: no compiler
// and no operations. Every implemented op has at least one of the two.
ساکن اندرونی
bool IsImplementedOp (IM3OpInfo i_info)
{
    واپس (i_info->compiler != c_cc_NULL or i_info->operations[0] != تہی);
}

ثابت u32 c_عدد_Operations   = M3_گنتی_کا(c_operations);
ثابت u32 c_عدد_Operations_FC = M3_گنتی_کا(c_operationsFC);

// Maps one of the sparse opcodes above c_waOp_lastCore onto c_operationsHigh,
// or -1 for anything else in that range.
ساکن اندرونی
i32 Get_بلند_Op_اشاریہ (m3opcode_t opcode)
{
    انتخاب (opcode) {
#اگر d_m3HasRefTypes
    حالت c_waOp_refNull: واپس c_op_بلند_ref_Null;
    حالت c_waOp_refIsNull: واپس c_op_بلند_ref_Is_Null;
    حالت c_waOp_refFunc: واپس c_op_بلند_ref_Func;
#  اگر d_m3HasTypedRefs
    حالت c_waOp_refAsNonNull: واپس c_op_بلند_ref_As_Non_Null;
#  endif
#اگر_ختم
#اگر d_m3_Has_ڈھیر_Switching
    حالت c_waOp_contNew: واپس c_op_بلند_cont_New;
    حالت c_waOp_contBind: واپس c_op_بلند_cont_Bind;
    حالت c_waOp_suspend: واپس c_op_بلند_suspend;
    حالت c_waOp_resume: واپس c_op_بلند_resume;
    حالت c_waOp_resumeThrow: واپس c_op_بلند_resume_Throw;
    حالت c_waOp_resumeThrowRef: واپس c_op_بلند_resume_Throw_Ref;
    حالت c_waOp_switch: واپس c_op_بلند_switch;
#اگر_ختم
    حالت c_waOp_extended: واپس c_op_بلند_extended;
#اگر d_m3HasAtomics
    حالت c_waOp_atomic: واپس c_op_بلند_atomic;
#اگر_ختم
    طےشدہ: واپس -1;
    }
}

IM3OpInfo GetOpInfo (m3opcode_t opcode)
{
    IM3OpInfo info = تہی;

    انتخاب (opcode >> 8) {
    حالت 0x00:
        // c_operations is indexed by opcode only up to c_waOp_lastCore; past that
        // it holds internal operations (DEBUG builds), unreachable by opcode.
        اگر (M3_LIKELY(opcode <= c_wa_Op_آخری_Core)) {
            info = &c_operations[opcode];
        } ورنہ {
            i32 بلند = Get_بلند_Op_اشاریہ(opcode);
            اگر (بلند >= 0) {
                info = &c_operations_بلند[بلند];
            }
        }
        توڑو;
    حالت c_waOp_extended:
        opcode &= 0xFF;
        اگر (M3_LIKELY(opcode <= c_wa_Op_آخری_Extended)) {
            info = &c_operationsFC[opcode];
        }
        توڑو;
#اگر d_m3HasAtomics
    حالت c_waOp_atomic: {
        i32 index = Get_Atomic_Info_اشاریہ(opcode & 0xFF);
        اگر (index >= 0) {
            info = &c_operationsFE[index];
        }
    } توڑو;
#اگر_ختم
    }

    واپس (info and IsImplementedOp(info)) ? info : تہی;
}

#اگر d_m3HasGasMetering

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
شمار {
    c_gasLocal   = 1,          // charged once per declared local, at function entry
    c_gasNominal = 1,          // nop, block, loop, if, const
    c_gasArith   = 45,
    c_gas_سرکاؤ   = 67,
    c_gasBranch  = 90,
    c_gas_Mem_سائز = 100,
    c_gasAccess  = 120,        // a local, global, table or memory access
    c_gasHeavy   = 10000,      // an indirect call, or anything that works over a whole region
    c_gasDivide  = 36000
};

// The most one op_UseGas can carry. A segment that reaches it is split into
// consecutive charges, which prepays exactly the same amount.
#  define d_m3MaxSegmentGas   0xFFFFFFFF

ساکن
u32 Get_Gas_لاگت (m3opcode_t i_opcode)
{
    انتخاب (i_opcode) {
    حالت 0x00:                        // unreachable
    حالت 0x01:                        // nop
    حالت c_wa_Op_بلاک:
    حالت c_waOp_loop:
    حالت c_waOp_if:
    حالت c_wa_Op_try_جدول:             // a block that also installs handlers
    حالت c_waOp_refNull:              // a constant, by another name
    حالت c_waOp_refFunc:
        واپس c_gasNominal;

    حالت c_wa_Op_انتہا:
        واپس 0;                     // the one control instruction the table prices at nothing

    حالت c_waOp_else:
    حالت c_waOp_branch:
    حالت c_waOp_branchIf:
    حالت c_waOp_return:
    حالت c_waOp_call:
    حالت c_waOp_returnCall:
    حالت c_waOp_throw:
    حالت c_waOp_throwRef:
        واپس c_gasBranch;

    حالت c_wa_Op_branch_جدول:
    حالت 0x1a:                        // drop
    حالت 0x1b:                        // select
    حالت c_waOp_selectTyped:
        واپس c_gasAccess;

    حالت c_wa_Op_call_بالواسطہ:
    حالت c_wa_Op_return_Call_بالواسطہ:
    حالت c_waOp_callRef:
    حالت c_waOp_returnCallRef:
    حالت c_wa_Op_حافظہ_Init:
    حالت c_wa_Op_حافظہ_Copy:
    حالت c_wa_Op_حافظہ_Fill:
    حالت c_wa_Op_جدول_Init:
    حالت c_wa_Op_جدول_Copy:
    حالت c_wa_Op_جدول_Grow:
    حالت c_wa_Op_جدول_Fill:
    حالت 0x40:                        // memory.grow
        واپس c_gasHeavy;

    حالت c_wa_Op_ڈیٹا_Drop:
    حالت c_waOp_elemDrop:
    حالت c_wa_Op_جدول_سائز:
    حالت 0x3f:                        // memory.size
        واپس c_gas_Mem_سائز;

    حالت c_waOp_refIsNull:
    حالت c_waOp_refAsNonNull:
        واپس c_gasArith;

    طےشدہ:
        توڑو;
    }

    // The two integer arithmetic runs are priced by position, i32 at 0x6a and
    // i64 at 0x7c, the table giving both the same shape
    اگر ((i_opcode >= 0x6a and i_opcode <= 0x78) or (i_opcode >= 0x7c and i_opcode <= 0x8a)) {
        // clang-format off
        ساکن ثابت u16 c_arithmeticCosts [] = {
            c_gasArith,  c_gasArith,  c_gasArith,                   // add, sub, mul
            c_gasDivide, c_gasDivide, c_gasDivide, c_gasDivide,     // div_s, div_u, rem_s, rem_u
            c_gasArith,  c_gasArith,  c_gasArith,                   // and, or, xor
            c_gas_سرکاؤ,  c_gas_سرکاؤ,  c_gas_سرکاؤ,                   // shl, shr_s, shr_u
            c_gasBranch, c_gasBranch                                // rotl, rotr
        };
        // clang-format on

        واپس c_arithmeticCosts[i_opcode - (i_opcode <= 0x78 ? 0x6a : 0x7c)];
    }

    // the atomic accesses are memory accesses; what waits, wakes or orders is worth
    // as much as an operation over a whole region
    اگر (i_opcode >= (c_waOp_atomic << 8) and i_opcode <= ((c_waOp_atomic << 8) | c_wa_Op_آخری_Atomic)) {
        واپس (i_opcode <= c_waOp_atomicFence) ? c_gasHeavy : c_gasAccess;
    }
    اگر (i_opcode >= 0x20 and i_opcode <= 0x3e) {
        واپس c_gasAccess;           // locals, globals, table accesses, loads and stores
    }
    اگر (i_opcode >= 0x41 and i_opcode <= 0x44) {
        واپس c_gasNominal;          // i32/i64/f32/f64.const
    }
    اگر (i_opcode >= 0x8b and i_opcode <= 0xa6) {
        // the float run, priced as integer arithmetic except where it divides
        bool divides = (i_opcode == 0x91 or i_opcode == 0x95 or     // f32.sqrt, f32.div
                        i_opcode == 0x9f or i_opcode == 0xa3);      // f64.sqrt, f64.div

        واپس divides ? c_gasDivide : c_gasArith;
    }

    // comparisons and clz/ctz/popcnt (0x45..0x69, 0x79..0x7b), the conversions
    // and sign extensions (0xa7..0xc4), the saturating truncations, the wide
    // arithmetic, and anything else that reaches here: plain arithmetic
    واپس c_gasArith;
}

// The instructions ewasm's design segments a body at. A segment runs up to and
// including the next one of these, so every cycle a body can execute - a loop's
// back edge, a branch out of a block, falling off the end of a function - passes
// through at least one, and no path can run unmetered however it is entered.
ساکن
bool Is_Gas_Segment_انتہا (m3opcode_t i_opcode)
{
    انتخاب (i_opcode) {
    حالت c_waOp_loop:
    حالت c_waOp_if:
    حالت c_waOp_else:
    حالت c_wa_Op_انتہا:
    حالت c_waOp_branch:
    حالت c_waOp_branchIf:
    حالت c_wa_Op_branch_جدول:
    حالت c_waOp_return:
    حالت c_waOp_returnCall:
    حالت c_wa_Op_return_Call_بالواسطہ:
    حالت c_waOp_returnCallRef:
        واپس true;
    طےشدہ:
        واپس false;
    }
}

// Ends the open segment by writing what it accumulated into the immediate its
// op_UseGas is still waiting on. Emitting onto a later code page does not move
// an earlier one, so the reservation is still where it was left.
ساکن
خالی CloseGasSegment (IM3Compilation o)
{
    اگر (o->isGasSegmentOpen) {
        اگر (o->gasPatch) {
            u32 لاگت = o->gas_لاگت;
            حافظہ_نقل(o->gasPatch, &لاگت, حجم(لاگت));

            o->gasPatch = تہی;
        }
        o->gas_لاگت          = 0;
        o->isGasSegmentOpen = false;
    }
}

// Whether this compilation emits the charges
ساکن اندرونی
bool IsMetering (IM3Compilation o)
{
    واپس o->runtime->gas_حد_بندی != 0;
}

// Charges one instruction to the segment being compiled, opening a segment at
// this point in the code stream if none is open, and settling the segment when
// the instruction is one that ends it. Called before the instruction is
// compiled, which is what both halves need: the op_UseGas lands ahead of what it
// pays for, and the cost is final before a block-structured instruction - loop,
// if, else - compiles its body from inside its own compiler. The body then opens
// a segment of its own, inside the loop or the arm, where it is charged on every
// pass rather than once on the way in.
ساکن
M3_نتیجہ MeterOpcode (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_نتیجہ نتیجہ = m3Err_none;

    // constant expressions are not metered: they run once at instantiation,
    // before the module is anything a gas budget was handed out for
    اگر (o->function and o->صفحہ and IsMetering(o)) {
        // only the 0xFC or 0xFE prefix has been read so far; the instruction it names is
        // the LEB128 u32 the compiler is about to take. Peek it without consuming.
        // Anything the tables do not cover leaves the prefix priced on its own,
        // which is moot - the compile fails on it a moment later.
        اگر (i_opcode == c_waOp_extended or i_opcode == c_waOp_atomic) {
            بائٹیں_t peek = o->wasm;
            u32     sub;
            u32     آخری = (i_opcode == c_waOp_atomic) ? c_wa_Op_آخری_Atomic : c_wa_Op_آخری_Extended;

            اگر (not ReadLEB_u32(&sub, &peek, o->wasm_انتہا) and sub <= آخری) {
                i_opcode = (m3opcode_t)((i_opcode << 8) | sub);
            }
        }

        u32 لاگت = Get_Gas_لاگت(i_opcode);

        اگر (o->isGasSegmentOpen and o->gas_لاگت > d_m3MaxSegmentGas - لاگت) {
            CloseGasSegment(o);
        }

        اگر (not o->isGasSegmentOpen) {
            o->isGasSegmentOpen = true;

_           (EmitOp(o, op_UseGas));
            o->gasPatch = (خالی*)Get_صفحہ_PC(o->صفحہ);
            EmitConstant32(o, 0);
        }

        o->gas_لاگت += لاگت;

        اگر (Is_Gas_Segment_انتہا(i_opcode)) {
            CloseGasSegment(o);
        }
    }

    _catch: واپس نتیجہ;
}

#اگر_ختم // d_m3HasGasMetering


M3_نتیجہ Compile_بلاک_Statements (IM3Compilation o)
{
    M3_نتیجہ نتیجہ   = m3Err_none;
    bool     valid_انتہا = false;

    جب (o->wasm < o->wasm_انتہا) {
#اگر d_m3EnableOpTracing
        اگر (o->عدد_Emits) {
            EmitOp(o, op_Dump_ڈھیر);
            EmitConstant32(o, o->عدد_Opcodes);
            EmitConstant32(o, Get_Max_مستعمل_خانہ_Plus_One(o));
            Emit_اشارہ_گر(o, o->function);

            o->عدد_Emits = 0;
        }
#اگر_ختم

        m3opcode_t opcode;
        o->آخری_Opcode_آغاز = o->wasm;
_       (Read_opcode(&opcode, &o->wasm, o->wasm_انتہا));
        log_opcode(o, opcode);

        // Restrict opcodes when evaluating expressions
        اگر (M3_UNLIKELY(not o->function)) {
            // clang-format off
            انتخاب (opcode) {
            حالت c_waOp_i32_const: حالت c_waOp_i64_const:
            حالت c_waOp_f32_const: حالت c_waOp_f64_const:
            حالت c_waOp_getGlobal: حالت c_wa_Op_انتہا:
#اگر d_m3HasRefTypes
            حالت c_waOp_refNull:   حالت c_waOp_refFunc:
#اگر_ختم
#اگر d_m3HasExtendedConst
            حالت c_wa_Op_i32_جوڑو:   حالت c_waOp_i32_sub:   حالت c_waOp_i32_mul:
            حالت c_wa_Op_i64_جوڑو:   حالت c_waOp_i64_sub:   حالت c_waOp_i64_mul:
#اگر_ختم
                توڑو;
            طےشدہ:
                _throw(m3Err_restrictedOpcode);
            }
            // clang-format on
        }

        IM3OpInfo opinfo = GetOpInfo(opcode);

        اگر (M3_UNLIKELY(opinfo == تہی)) {
            _throw(ErrorCompile(m3Err_unknownOpcode, o, "opcode '%x' not available", opcode));
        }

#اگر d_m3HasGasMetering
_       (MeterOpcode(o, opcode));
#اگر_ختم

        اگر (opinfo->compiler != c_cc_NULL) {
_           ((*GetCompiler(opinfo))(o, opcode))
        } ورنہ {
_           (Compile_Operator(o, opcode));
        }

        o->پچھلا_Opcode = opcode;

        اگر (opcode == c_waOp_else) {
            _throwif(m3Err_wasmMalformed, o->بلاک.opcode != c_waOp_if);
            valid_انتہا = true;
            توڑو;
        } ورنہ اگر (opcode == c_wa_Op_انتہا) {
            valid_انتہا = true;
            توڑو;
        }
    }
    _throwif(m3Err_wasmMalformed, not valid_انتہا);

_catch:
    واپس نتیجہ;
}

ساکن
M3_نتیجہ ڈالو_بلاک_Results (IM3Compilation o)
{
    M3_نتیجہ نتیجہ = m3Err_none;

    u16 عدد_Results = Get_Func_Type_عدد_Results(o->بلاک.type);

    برائے (u16 i = 0; i < عدد_Results; ++i) {
        m3type_t type = Get_Func_Type_نتیجہ_Type(o->بلاک.type, i);

        اگر (i == عدد_Results - 1 and IsFpType(type)) {
_           (ڈالو_رجسٹر(o, type));
        } ورنہ {
_           (ڈالو_Allocated_خانہ(o, type));
        }
    }

    _catch: واپس نتیجہ;
}


M3_نتیجہ Compile_بلاک (IM3Compilation o, IM3FuncType i_بلاک_Type, m3opcode_t i_بلاک_Opcode)
{
                                                                                        d_m3Assert (not Is_رجسٹر_Allocated (o, 0));
                                                                                        d_m3Assert (not Is_رجسٹر_Allocated (o, 1));
    InvalidateFold(o);

    M3CompilationScope  outerScope = o->بلاک;
    M3CompilationScope* بلاک      = &o->بلاک;

    بلاک->outer   = &outerScope;
    بلاک->pc      = Get_صفحہ_PC(o->صفحہ);
    بلاک->patches = تہی;
    بلاک->type    = i_بلاک_Type;
    بلاک->opcode  = i_بلاک_Opcode;
    بلاک->گہرائی++;
    بلاک->snapshot_بلاک = 0;

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
#اگر d_m3HasSnapshots
    // op_Loop's frame records the body, which is also the loop's identity;
    // op_TryTable's records its clause table, two words a clause back from it
    اگر (i_بلاک_Opcode == c_waOp_loop) {
_       (ریکارڈ_بلاک_آغاز(o, i_بلاک_Opcode, بلاک->pc, 0));
    }
#  اگر d_m3HasExceptionHandling
    ورنہ اگر (i_بلاک_Opcode == c_wa_Op_try_جدول) {
_       (ریکارڈ_بلاک_آغاز(o, i_بلاک_Opcode, بلاک->pc - 2 * o->عدد_Try_Clauses, o->عدد_Try_Clauses));
    }
#  endif
#اگر_ختم

    // validate and dealloc params ----------------------------

    u16 ڈھیر_اشاریہ = o->ڈھیر_اشاریہ;

    u16 عدد_Params = Get_Func_Type_عدد_Params(i_بلاک_Type);

    اگر (i_بلاک_Opcode != c_waOp_else) {
        برائے (u16 i = 0; i < عدد_Params; ++i) {
            m3type_t type = GetFuncTypeParamType(i_بلاک_Type, عدد_Params - 1 - i);
_           (نکالو_Type(o, type));
        }
    } ورنہ {
        اگر (Is_ڈھیر_Polymorphic(o) && o->بلاک.بلاک_ڈھیر_اشاریہ + عدد_Params > o->ڈھیر_اشاریہ) {
            o->ڈھیر_اشاریہ = o->بلاک.بلاک_ڈھیر_اشاریہ;
        } ورنہ {
            o->ڈھیر_اشاریہ -= عدد_Params;
        }
    }

    u16 param_اشاریہ        = o->ڈھیر_اشاریہ;
    بلاک->exit_ڈھیر_اشاریہ = param_اشاریہ; // consume the params at block exit

    // keep copies of param slots in the stack
    o->ڈھیر_اشاریہ = ڈھیر_اشاریہ;

    // find slots for the results ----------------------------
_   (ڈالو_بلاک_Results(o));

    ڈھیر_اشاریہ = o->ڈھیر_اشاریہ;

    // dealloc but keep record of the result slots in the stack
    u16 عدد_Results = Get_Func_Type_عدد_Results(i_بلاک_Type);
    جب (عدد_Results--) {
        نکالو(o);
    }

    بلاک->بلاک_ڈھیر_اشاریہ = o->ڈھیر_اشاریہ = ڈھیر_اشاریہ;

    // push the params back onto the stack -------------------
    برائے (u16 i = 0; i < عدد_Params; ++i) {
        m3type_t type = GetFuncTypeParamType(i_بلاک_Type, i);

        u16 خانہ = Get_خانہ_For_ڈھیر_اشاریہ(o, param_اشاریہ + i);
        ڈالو(o, type, خانہ);

        اگر (خانہ >= o->خانہ_پہلا_Dynamic_اشاریہ && خانہ != c_خانہ_غیر_مستعمل) {
_           (MarkSlotsAllocatedByType(o, خانہ, type));
        }
    }

    //--------------------------------------------------------

#اگر d_m3HasExceptionHandling
    // with the scope pushed and the params back on, a catch label of 0 names
    // this block - which is what the proposal says it should
    اگر (i_بلاک_Opcode == c_wa_Op_try_جدول) {
_       (EmitCatchStubs(o));
    }
#اگر_ختم

_   (Compile_بلاک_Statements(o));

_   (Validate_بلاک_انتہا(o));

    اگر (o->function)    // skip for expressions
    {
        اگر (not Is_ڈھیر_Polymorphic(o)) {
_           (Resolve_بلاک_Results(o, &o->بلاک, /* isBranch: */ false));
        }

#اگر d_m3HasExceptionHandling
        // Falling out of the end of a try region retires its handler. This sits
        // ahead of PatchBranches so branches into the block's exit land past it:
        // a branch out of the try popped its own handler at the branch site.
        اگر (i_بلاک_Opcode == c_wa_Op_try_جدول) {
_           (EmitOp(o, op_نکالو_Handlers));
            EmitConstant32(o, 1);
        }
#اگر_ختم

_       (Unwind_بلاک_ڈھیر(o))

        اگر (not ((i_بلاک_Opcode == c_waOp_if and عدد_Results) or o->پچھلا_Opcode == c_waOp_else)) {
            o->ڈھیر_اشاریہ = o->بلاک.exit_ڈھیر_اشاریہ;
_           (ڈالو_بلاک_Results(o));
        }
    }

    PatchBranches(o);

#اگر d_m3HasSnapshots
    ریکارڈ_بلاک_انتہا(o);

    اگر (o->local_Init_گہرائی) {
        برائے (u32 i = 0; i < o->snapshotMap->عدد_Locals; ++i) {
            اگر (o->local_Init_گہرائی[i] >= (u32)o->بلاک.گہرائی) {
                o->local_Init_گہرائی[i] = UINT32_MAX;
            }
        }
    }
#اگر_ختم
    o->بلاک = outerScope;

} _catch: واپس نتیجہ;
}

ساکن
M3_نتیجہ CompileLocals (IM3Compilation o)
{
    M3_نتیجہ نتیجہ;

    u32 عدد_Locals = 0;
    u32 عدد_Local_بلاکیں;
_   (ReadLEB_u32(&عدد_Local_بلاکیں, &o->wasm, o->wasm_انتہا));

    برائے (u32 l = 0; l < عدد_Local_بلاکیں; ++l) {
        u32      var_گنتی;
        m3type_t localType;

_       (ReadLEB_u32(&var_گنتی, &o->wasm, o->wasm_انتہا));
_       (Parse_قدر_Type(o->module, &localType, &o->wasm, o->wasm_انتہا));
        عدد_Locals += var_گنتی;                                                          m3log (compile, "pushing locals. count: %d; type: %s", var_گنتی, c_waTypes [بنیاد_Type_کا(localType)]);
        جب (var_گنتی--) {
_           (ڈالو_Allocated_خانہ(o, localType));
        }
    }

    اگر (o->function) {
        o->function->عدد_Locals = عدد_Locals;
    }

    _catch: واپس نتیجہ;
}

ساکن
M3_نتیجہ ReserveConstants (IM3Compilation o)
{
    M3_نتیجہ نتیجہ = m3Err_none;

    // in the interest of speed, this blindly scans the Wasm code looking for any byte
    // that looks like an const opcode.
    u16 عدد_Constant_Slots = 0;

    بائٹیں_t wa = o->wasm;
    جب (wa < o->wasm_انتہا) {
        u8  رمز     = *wa++;
        u16 جوڑو_Slots = 0;

        اگر (رمز == c_waOp_i32_const or رمز == c_waOp_f32_const) {
            جوڑو_Slots = 1;
        } ورنہ اگر (رمز == c_waOp_i64_const or رمز == c_waOp_f64_const) {
            جوڑو_Slots = Get_Type_عدد_Slots(c_m3Type_i64);
        }

        اگر (عدد_Constant_Slots + جوڑو_Slots >= d_m3_Max_Constant_جدول_سائز) {
            توڑو;
        }

        عدد_Constant_Slots += جوڑو_Slots;
    }

    // if constants overflow their reserved stack space, the compiler simply emits op_Const
    // operations as needed. Compiled expressions (global inits) don't pass through this
    // ReserveConstants function and thus always produce inline constants.

    Align_خانہ_مقصد_Type(&عدد_Constant_Slots, c_m3Type_i64);                                           m3log (compile, "reserved constant slots: %d", عدد_Constant_Slots);

    o->خانہ_پہلا_Dynamic_اشاریہ = o->خانہ_پہلا_Const_اشاریہ + عدد_Constant_Slots;

    اگر (o->خانہ_پہلا_Dynamic_اشاریہ >= d_m3MaxFunctionSlots) {
        _throw(m3_Err_function_ڈھیر_حد_سے_تجاوز);
    }

    _catch:
    واپس نتیجہ;
}


// A constant expression compiles like a miniature function root block: no args,
// no locals and a single result, which Compile_End copies down into slot 0 for
// EvaluateExpression to read back.
M3_نتیجہ Compile_عبارت (IM3Compilation o, IM3FuncType i_نتیجہ_Type)
{
    M3_نتیجہ نتیجہ = m3Err_none;

    o->بلاک.type = i_نتیجہ_Type;

    u16 عدد_Ret_Slots = Get_Func_Type_عدد_Results(i_نتیجہ_Type) * c_io_خانہ_گنتی;

    برائے (u16 i = 0; i < عدد_Ret_Slots; ++i) {
        Mark_خانہ_Allocated(o, i);
    }

    o->max_ڈھیر_Slots = o->خانہ_Max_Allocated_اشاریہ_Plus_One = o->خانہ_پہلا_Dynamic_اشاریہ = عدد_Ret_Slots;

_   (Compile_بلاک_Statements(o));

    _catch: واپس نتیجہ;
}


M3_نتیجہ CompileFunction (IM3Function io_function)
{
    اگر (!io_function->wasm) {
        // An import. Either linking pointed it at another module's function -
        // in which case that one carries the body - or a host function was
        // bound to it, or it is simply unsatisfied.
        IM3Function impl = Function_Implementation(io_function);

        اگر (impl != io_function) {
            اگر (not impl->compiled) {
                M3_نتیجہ r = CompileFunction(impl);
                اگر (r) {
                    واپس r;
                }
            }

            // keep the placeholder callable too, for anything still holding it
            io_function->compiled = impl->compiled;
            واپس m3Err_none;
        }

        اگر (io_function->compiled) {
            واپس m3Err_none;
        }

        واپس ErrorModule(m3Err_functionImportMissing, io_function->module, "'%s.%s'",
                           GetFunctionImportModuleName(io_function),
                           m3_GetFunctionName(io_function));
    }

#اگر d_m3EnableValidation
    اگر (not io_function->module->runtime->skipValidation) {
        M3_نتیجہ vr = ValidateFunction(io_function);
        اگر (vr) {
            واپس vr;
        }
    }
#اگر_ختم

    IM3FuncType funcType = io_function->funcType;                   m3log (compile, "compiling: [%d] %s %s; wasm-size: %d",
                                                                        io_function->index, m3_GetFunctionName (io_function), SPrintFuncTypeSignature (funcType), (u32) (io_function->wasm_انتہا - io_function->wasm));
    IM3Runtime runtime = io_function->module->runtime;

    IM3Compilation o = &runtime->compilation;                       d_m3Assert (d_m3MaxFunctionSlots >= d_m3_Max_Function_ڈھیر_اونچائی * (d_m3_Use32_ثنائی_ہندسہ_Slots + 1))  // need twice as many slots in 32-bit mode
    memset(o, 0x0, حجم(M3Compilation));

    o->runtime    = runtime;
    o->module     = io_function->module;
    o->function   = io_function;
    o->wasm       = io_function->wasm;
    o->wasm_انتہا    = io_function->wasm_انتہا;
    o->بلاک.type = funcType;

_try {
    // skip over code size. the end was already calculated during parse phase
    u32 سائز;
_   (ReadLEB_u32(&سائز, &o->wasm, o->wasm_انتہا));                     d_m3Assert (سائز == (o->wasm_انتہا - o->wasm))

#اگر d_m3HasSnapshots
    o->body_آغاز = o->wasm;
#اگر_ختم

#اگر d_m3HasSnapshots
    // only code that can be suspended can end up in a snapshot
    اگر (runtime->isSuspendable) {
        o->snapshotMap = m3_AllocStruct(M3SnapshotMap);
        _throwifnull(o->snapshotMap);
    }
#اگر_ختم

_   (Acquire_Compilation_رمز_صفحہ(o, &o->صفحہ));

    pc_t pc = Get_صفحہ_PC(o->صفحہ);

    u16 عدد_Ret_Slots = Get_Function_عدد_Returns(o->function) * c_io_خانہ_گنتی;

    برائے (u16 i = 0; i < عدد_Ret_Slots; ++i) {
        Mark_خانہ_Allocated(o, i);
    }

    o->function->عدد_Ret_Slots = o->خانہ_پہلا_Dynamic_اشاریہ = عدد_Ret_Slots;

    u16 عدد_دلائل = Get_Function_عدد_دلائل(o->function);

    // push the arg types to the type stack
    برائے (u16 i = 0; i < عدد_دلائل; ++i) {
        m3type_t type = GetFunctionArgType(o->function, i);
_       (ڈالو_Allocated_خانہ(o, type));

        // prevent allocator fill-in
        o->خانہ_پہلا_Dynamic_اشاریہ += c_io_خانہ_گنتی;
    }

    o->خانہ_Max_Allocated_اشاریہ_Plus_One = o->function->عدد_Ret_And_Arg_Slots = o->خانہ_پہلا_Local_اشاریہ = o->خانہ_پہلا_Dynamic_اشاریہ;

_   (CompileLocals(o));

#اگر d_m3HasSnapshots
_   (ریکارڈ_Locals(o));
#اگر_ختم

#اگر d_m3HasGasMetering
    // the table charges for a function's locals as well as for its instructions.
    // op_Entry is what sets them up, so the charge rides on the body's first
    // segment, which is emitted right after it
    o->gas_لاگت = o->function->عدد_Locals * c_gasLocal;
#اگر_ختم

    u16 max_خانہ = Get_Max_مستعمل_خانہ_Plus_One(o);

    o->function->عدد_Local_بائٹیں = (max_خانہ - o->خانہ_پہلا_Local_اشاریہ) * حجم(m3slot_t);

    o->خانہ_پہلا_Const_اشاریہ = o->خانہ_Max_Const_اشاریہ = max_خانہ;

    // ReserveConstants initializes o->firstDynamicSlotNumber
_   (ReserveConstants(o));

    // start tracking the max stack used (Push() also updates this value) so that op_Entry can precisely detect stack overflow
    o->max_ڈھیر_Slots = o->خانہ_Max_Allocated_اشاریہ_Plus_One = o->خانہ_پہلا_Dynamic_اشاریہ;

    o->بلاک.بلاک_ڈھیر_اشاریہ = o->ڈھیر_پہلا_Dynamic_اشاریہ = o->ڈھیر_اشاریہ;                           m3log (compile, "start stack index: %u; max stack slots: %u",
                                                                                                           (u32) o->ڈھیر_پہلا_Dynamic_اشاریہ, (u32) o->max_ڈھیر_Slots);
#اگر d_m3_Has_ڈھیر_Switching
    // A suspendable body opens by checking for a pause request, once its frame
    // is set up. The pause point's offset is the first instruction's, just past
    // the local declarations - the body's end, when it has no other.
    اگر (runtime->isSuspendable) {
        o->آخری_Opcode_آغاز = o->wasm;
#  اگر d_m3_اندراج_Keeps_چوکھٹا
_       (EmitOp(o, op_اندراج));
        Emit_اشارہ_گر(o, io_function);
_       (EmitOp(o, op_اندراج_Check));
_       (ریکارڈ_محفوظ_نقطہ(o, safepoint_اندراج, 1, 0, 0, تہی));
#  ورنہ
_       (EmitOp(o, op_اندراج_Suspendable));
        Emit_اشارہ_گر(o, io_function);
_       (ریکارڈ_محفوظ_نقطہ(o, safepoint_اندراج, 0, 0, 0, تہی));
#  endif
    } ورنہ
#اگر_ختم
    {
_       (EmitOp(o, op_اندراج));
        Emit_اشارہ_گر(o, io_function);
    }

_   (Compile_بلاک_Statements(o));

    _throwif(m3Err_wasmMalformed, o->پچھلا_Opcode != c_wa_Op_انتہا);

    io_function->compiled      = pc;
    io_function->max_ڈھیر_Slots = o->max_ڈھیر_Slots;

    u16 عدد_Constant_Slots = o->خانہ_Max_Const_اشاریہ - o->خانہ_پہلا_Const_اشاریہ;                           m3log (compile, "unique constant slots: %u; unused slots: %u",
                                                                                                           عدد_Constant_Slots, o->خانہ_پہلا_Dynamic_اشاریہ - o->خانہ_Max_Const_اشاریہ);
    io_function->عدد_Constant_بائٹیں = عدد_Constant_Slots * حجم(m3slot_t);

    اگر (عدد_Constant_Slots) {
        io_function->constants = m3_CopyMem((cbytes_t)o->constants, io_function->عدد_Constant_بائٹیں);
        _throwifnull(io_function->constants);
    }

#اگر d_m3HasSnapshots
    io_function->snapshotMap = o->snapshotMap;
    o->snapshotMap           = تہی;
#اگر_ختم

} _catch:

    Release_Compilation_رمز_صفحہ(o);

#اگر d_m3HasSnapshots
    SnapshotMap_Free(o->snapshotMap);
    o->snapshotMap = تہی;
    m3_Free(o->local_Init_گہرائی);
#اگر_ختم

    واپس نتیجہ;
}

#اگر d_m3_Has_ڈھیر_Switching
m3ret_t ResumeContinuation (IM3Runtime i_runtime, IM3Continuation i_cont)
{
    اگر (M3_UNLIKELY(!i_cont || i_cont->state != cont_suspended)) {
        واپس m3Err_none;
    }

    M3_حافظہ_سرخی* _mem = i_cont->اندراج_Function ? Module_حافظہ_سرخی(i_cont->اندراج_Function->module) : تہی;

    i_runtime->فعال_Continuation = i_cont;
    i_cont->state                 = cont_running;

    i32 outermost     = (i32)i_cont->عدد_چوکھٹے - 1;
    i_cont->عدد_چوکھٹے = 0;

    // with no frames standing, this lands straight on the suspension point
    m3ret_t r = Replay_چوکھٹے(i_cont, outermost, _mem);

    اگر (r == m3Err_continuationSuspended) {
        i_cont->state = cont_suspended;
    } ورنہ {
        i_cont->state = cont_returned;
    }
    i_runtime->فعال_Continuation = تہی;

    واپس r;
}
#اگر_ختم
