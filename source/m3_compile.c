//
//  m3_compile.c
//
//  Created by Steven Massey on 4/17/19.
//  Copyright © 2019 Steven Massey. All rights reserved.
//

// Allow using opcodes for compilation process
#تعریف_کرو M3_COMPILE_OPCODES

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
    أرجع Get_ورقہ_PC(o->ورقہ);
}

#إذا d_m3HasSnapshots

#  define d_m3GrowSnapshotArray(TYPE, ARRAY, گنتی_2, سمرتھا_2)                    \
    إذا ((گنتی_2) == (سمرتھا_2)) {                                                 \
        u32   new_سمرتھا = (سمرتھا_2) ? (سمرتھا_2) * 2 : 8;                     \
        TYPE* grown = m3_ReallocArray(TYPE, (ARRAY), new_سمرتھا, (سمرتھا_2));   \
        إذا (not grown) {                                                         \
            أرجع m3Err_mallocFailed;                                           \
        }                                                                        \
        (ARRAY)    = grown;                                                      \
        (سمرتھا_2) = new_سمرتھا;                                                \
    }

// Where the instruction being compiled starts, counted from the start of the
// function's body: the one position every build of every revision agrees on
ساكن مضمن
u32 Get_Wasm_وتھ (IM3Compilation o)
{
    أرجع (u32)(o->آخری_Opcode_شروع - o->body_شروع);
}

// Whether the operand stack entry at i_index is a value the frame holds. Every
// block keeps records of its parameter and result slots on the stack, between
// the height it was entered at and where its own values begin (see
// CompileBlock). Those are landing pads rather than values: nothing need have
// written them yet, and the slots they name may be in use for something else.
// The function's own scope has none - that stretch is its arguments and locals.
ساكن
bool Is_جیوندا_ڈھیر_اندراج (IM3Compilation o, u16 i_اشاریہ)
{
    لكل (IM3CompilationScope scope = &o->بلاک; scope->outer; scope = scope->outer) {
        إذا (i_اشاریہ >= scope->exit_ڈھیر_اشاریہ and i_اشاریہ < scope->بلاک_ڈھیر_اشاریہ) {
            أرجع false;
        }
    }

    أرجع true;
}

ساكن
M3_نتیجہ جوڑو_محفوظ_نقطہ_قدر (IM3Compilation o, M3_محفوظ_نقطہ* io_نقطہ, u16 i_اشاریہ)
{
    M3SnapshotMap* map = o->snapshotMap;

    d_m3GrowSnapshotArray(M3_تھاں_قدر, map->اقدار, map->عدد_اقدار, o->اقدار_سمرتھا);

    map->اقدار[map->عدد_اقدار].تھاں = o->wasm_ڈھیر[i_اشاریہ];
    map->اقدار[map->عدد_اقدار].type = o->type_ڈھیر[i_اشاریہ];
    map->عدد_اقدار++;
    io_نقطہ->عدد_اقدار++;

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
M3_نتیجہ ریکارڈ_محفوظ_نقطہ (IM3Compilation o, M3_محفوظ_نقطہ_قسم i_قسم, u32 i_الفاظ_پٹھ, u16 i_عدد_Results,
                          u16 i_aux, IM3CompilationScope i_loop)
{
_try {
    M3SnapshotMap* map = o->snapshotMap;

    إذا (not map or not o->ورقہ) {
        أرجع m3Err_none;
    }

    d_m3GrowSnapshotArray(M3_محفوظ_نقطہ, map->محفوظ_Points, map->عدد_محفوظ_Points, o->محفوظ_Points_سمرتھا);

    // grown before it is pointed at: adding values never moves the safepoints
    M3_محفوظ_نقطہ* نقطہ     = &map->محفوظ_Points[map->عدد_محفوظ_Points];
    نقطہ->pc              = GetPC(o) - i_الفاظ_پٹھ;
    نقطہ->wasm_وتھ      = Get_Wasm_وتھ(o);
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
    إذا (o->local_Init_ڈونگھائی) {
        نقطہ->پہلا_Local_قدر = map->عدد_اقدار;
        لكل (u32 i = 0; i < map->عدد_Locals; ++i) {
            d_m3GrowSnapshotArray(M3_تھاں_قدر, map->اقدار, map->عدد_اقدار, o->اقدار_سمرتھا);
            M3_تھاں_قدر قدر = map->locals[i];
            إذا (o->local_Init_ڈونگھائی[i] == UINT32_MAX or
                (i_loop and o->local_Init_ڈونگھائی[i] >= (u32)i_loop->ڈونگھائی)) {
                قدر.type &= ~d_m3Type_refNonNull;
            }
            map->اقدار[map->عدد_اقدار++] = قدر;
        }
        نقطہ->پہلا_قدر = map->عدد_اقدار;
    }

    u16 چوٹی = o->ڈھیر_اشاریہ;

    إذا (i_قسم != safepoint_suspend) {
        چوٹی = (چوٹی > i_عدد_Results) ? (u16)(چوٹی - i_عدد_Results) : 0;
    }
    إذا (i_loop) {
        چوٹی = M3_MIN(چوٹی, i_loop->exit_ڈھیر_اشاریہ);
    }

    لكل (u16 i = o->ڈھیر_پہلا_Dynamic_اشاریہ; i < چوٹی; ++i) {
        إذا (Is_جیوندا_ڈھیر_اندراج(o, i)) {
_           (جوڑو_محفوظ_نقطہ_قدر(o, نقطہ, i));
        }
    }

    إذا (i_loop) {
        u16 عدد_Params = Get_Func_Type_عدد_Params(i_loop->type);

        لكل (u16 i = 0; i < عدد_Params; ++i) {
_           (جوڑو_محفوظ_نقطہ_قدر(o, نقطہ, (u16)(i_loop->exit_ڈھیر_اشاریہ + i)));
        }
    }

    map->عدد_محفوظ_Points++;

} _catch: أرجع نتیجہ;
}

// A back edge: op_ContinueLoop, or op_ContinueLoopIf when i_isConditional, and
// the loop it goes back to. The conditional one suspends before it has taken
// the branch, so it has to be resumed with the condition that took it.
ساكن
M3_نتیجہ ریکارڈ_پٹھ_کنارا (IM3Compilation o, IM3CompilationScope i_loop, bool i_isConditional)
{
    M3_نتیجہ نتیجہ = m3Err_none;

    // the loop is named by its record in the map, which has to fit the field
    _throwif("a function has too many loops for its snapshot map",
             o->snapshotMap and o->ورقہ and (i_loop->snapshot_بلاک == 0 or i_loop->snapshot_بلاک > UINT16_MAX + 1u));

_   (ریکارڈ_محفوظ_نقطہ(o, safepoint_op, 2, 0, (u16)(i_loop->snapshot_بلاک - 1), i_loop));

    إذا (i_isConditional and o->snapshotMap and o->ورقہ) {
        o->snapshotMap->محفوظ_Points[o->snapshotMap->عدد_محفوظ_Points - 1].flags |= d_m3_محفوظ_نقطہ_لیا_Branch;
    }

    _catch: أرجع نتیجہ;
}

// A call, waiting on its callee with i_numResults not yet written; the
// callee's frame starts i_calleeSlot slots above this one. A return_call that
// is compiled as a call is one too, since its frame stands while the callee
// runs, but it is marked: in Wasm the callee has replaced it, so a snapshot
// walks past it rather than writing it.
ساكن
M3_نتیجہ ریکارڈ_Call (IM3Compilation o, u16 i_عدد_Results, u16 i_callee_تھاں, bool i_isReturnCall)
{
    M3_نتیجہ نتیجہ = ریکارڈ_محفوظ_نقطہ(o, safepoint_call, 0, i_عدد_Results, i_callee_تھاں, عدم);

    إذا (not نتیجہ and i_isReturnCall and o->snapshotMap and o->ورقہ) {
        o->snapshotMap->محفوظ_Points[o->snapshotMap->عدد_محفوظ_Points - 1].flags |= d_m3_محفوظ_نقطہ_پوچھ_Call;
    }

    أرجع نتیجہ;
}

// Where the function's arguments and locals are kept. Unlike the operand
// stack, that never changes, so it is recorded once rather than at every
// safepoint.
ساكن
M3_نتیجہ ریکارڈ_Locals (IM3Compilation o)
{
    M3SnapshotMap* map = o->snapshotMap;

    إذا (map and o->ڈھیر_اشاریہ) {
        map->locals = m3_AllocArray(M3_تھاں_قدر, o->ڈھیر_اشاریہ);

        إذا (not map->locals) {
            أرجع m3Err_mallocFailed;
        }

        لكل (u16 i = 0; i < o->ڈھیر_اشاریہ; ++i) {
            map->locals[i].تھاں = o->wasm_ڈھیر[i];
            map->locals[i].type = o->type_ڈھیر[i];
        }

        map->عدد_Locals = o->ڈھیر_اشاریہ;
        لكل (u32 i = Get_Function_عدد_دلیلاں(o->function); i < map->عدد_Locals; ++i) {
            إذا (IsRefType(map->locals[i].type) and not IsNullableRef(map->locals[i].type)) {
                o->local_Init_ڈونگھائی = m3_AllocArray(u32, map->عدد_Locals);
                إذا (not o->local_Init_ڈونگھائی) {
                    أرجع m3Err_mallocFailed;
                }
                لكل (u32 j = i; j < map->عدد_Locals; ++j) {
                    إذا (IsRefType(map->locals[j].type) and not IsNullableRef(map->locals[j].type)) {
                        o->local_Init_ڈونگھائی[j] = UINT32_MAX;
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
M3_نتیجہ ریکارڈ_بلاک_شروع (IM3Compilation o, m3opcode_t i_opcode, pc_t i_pc, u32 i_عدد_Clauses)
{
    M3SnapshotMap* map = o->snapshotMap;

    إذا (map and o->ورقہ) {
        d_m3GrowSnapshotArray(M3_بلاک_شروع, map->بلاکاں, map->عدد_بلاکاں, o->بلاکاں_سمرتھا);

        M3_بلاک_شروع* بلاک = &map->بلاکاں[map->عدد_بلاکاں++];
        بلاک->pc           = i_pc;
        بلاک->wasm_وتھ   = Get_Wasm_وتھ(o);
        بلاک->wasm_سرا      = UINT32_MAX;
        بلاک->عدد_Clauses   = (u16)i_عدد_Clauses;
        بلاک->opcode       = (u8)i_opcode;

        o->بلاک.snapshot_بلاک = map->عدد_بلاکاں;
    }

    أرجع m3Err_none;
}

// Closes the scope's region, while the instruction just read is its end
ساكن
فراغ ریکارڈ_بلاک_سرا (IM3Compilation o)
{
    إذا (o->snapshotMap and o->بلاک.snapshot_بلاک) {
        o->snapshotMap->بلاکاں[o->بلاک.snapshot_بلاک - 1].wasm_سرا = Get_Wasm_وتھ(o);
    }
}

#  undef d_m3GrowSnapshotArray

#وإلا

ساكن مضمن
M3_نتیجہ ریکارڈ_محفوظ_نقطہ (IM3Compilation o, M3_محفوظ_نقطہ_قسم i_قسم, u32 i_الفاظ_پٹھ, u16 i_عدد_Results,
                          u16 i_aux, IM3CompilationScope i_loop)
{
    (فراغ)o;
    (فراغ)i_قسم;
    (فراغ)i_الفاظ_پٹھ;
    (فراغ)i_عدد_Results;
    (فراغ)i_aux;
    (فراغ)i_loop;
    أرجع m3Err_none;
}

ساكن مضمن
M3_نتیجہ ریکارڈ_پٹھ_کنارا (IM3Compilation o, IM3CompilationScope i_loop, bool i_isConditional)
{
    (فراغ)o;
    (فراغ)i_loop;
    (فراغ)i_isConditional;
    أرجع m3Err_none;
}

ساكن مضمن
M3_نتیجہ ریکارڈ_Call (IM3Compilation o, u16 i_عدد_Results, u16 i_callee_تھاں, bool i_isReturnCall)
{
    (فراغ)o;
    (فراغ)i_عدد_Results;
    (فراغ)i_callee_تھاں;
    (فراغ)i_isReturnCall;
    أرجع m3Err_none;
}

#جے_انت // d_m3HasSnapshots

ساكن M3_NOINLINE
M3_نتیجہ Ensure_کوڈ_ورقہ_عدد_سطریں (IM3Compilation o, u32 i_عدد_سطریں)
{
    M3_نتیجہ نتیجہ = m3Err_none;

    i_عدد_سطریں += 2; // room for Bridge

    إذا (M3_UNLIKELY(عدد_Free_سطریں(o->ورقہ) < i_عدد_سطریں)) {
        IM3_کوڈ_ورقہ ورقہ = Acquire_کوڈ_ورقہ_With_سمرتھا(o->runtime, i_عدد_سطریں);

        إذا (ورقہ) {
            m3log(emit, "bridging new code page from: %d %p (free slots: %d) to: %d", o->ورقہ->info.sequence, GetPC(o), عدد_Free_سطریں(o->ورقہ), ورقہ->info.sequence);
            d_m3Assert(عدد_Free_سطریں(o->ورقہ) >= 2);

            Emit_لفظ(o->ورقہ, op_Branch);
            Emit_لفظ(o->ورقہ, Get_ورقہ_PC(ورقہ));

            Release_کوڈ_ورقہ(o->runtime, o->ورقہ);

            o->ورقہ = ورقہ;
        } وإلا {
            نتیجہ = m3_Err_malloc_Failed_کوڈ_ورقہ;
        }
    }

    أرجع نتیجہ;
}

// invalidate the pending local.set fold candidate; called wherever code is emitted or a
// branch target captures the current pc (a fold appends an immediate, moving that position)
ساكن مضمن
فراغ InvalidateFold (IM3Compilation o)
{
#إذا d_m3_Fold_سیٹ_Local
    o->foldPatchPC = عدم;
#وإلا
    (فراغ)o;
#جے_انت
}

ساكن M3_NOINLINE
M3_نتیجہ EmitOp (IM3Compilation o, IM3Operation i_operation)
{
    M3_نتیجہ نتیجہ = m3Err_none;                                 d_m3Assert (i_operation or Is_ڈھیر_Polymorphic (o));

    InvalidateFold(o);

    // it's OK for page to be null; when compile-walking the bytecode without emitting
    إذا (o->ورقہ) {
#إذا d_m3EnableOpTracing
        إذا (i_operation != op_Dump_ڈھیر) {
            o->عدد_Emits++;
        }
#جے_انت

        // have execution jump to a new page if slots are critically low
        نتیجہ = Ensure_کوڈ_ورقہ_عدد_سطریں(o, d_m3_کوڈ_ورقہ_Free_سطریں_حد);

        إذا (M3_LIKELY(not نتیجہ)) {
            إذا (d_m3LogEmit) {
                log_emit(o, i_operation);
            }
#إذا d_m3_ریکارڈ_Backtraces
            Emit_Mapping_اندراج(o->ورقہ, (u32)(o->آخری_Opcode_شروع - o->module->wasm_شروع));
#جے_انت // d_m3RecordBacktraces
            Emit_لفظ(o->ورقہ, i_operation);
        }
    }

    أرجع نتیجہ;
}

// Push an immediate constant into the M3 codestream
ساكن M3_NOINLINE
فراغ EmitConstant32 (IM3Compilation o, ثابت u32 i_فوری)
{
    إذا (o->ورقہ) {                                                          m3log (emit, "const32: %ud", i_فوری);
        EmitWord32(o->ورقہ, i_فوری);
    }
}

#إذا d_m3HasMemory64
// Takes two lines of the code page where a pointer is 32 bits, so whatever
// reads it back has to step _pc by the same amount - see op_Const64.
ساكن M3_NOINLINE
فراغ EmitConstant64 (IM3Compilation o, ثابت u64 i_فوری)
{
    إذا (o->ورقہ) {
        EmitWord64(o->ورقہ, i_فوری);
    }
}
#جے_انت

ساكن M3_NOINLINE
فراغ Emit_تھاں_وتھ (IM3Compilation o, ثابت i32 i_وتھ)
{
    إذا (o->ورقہ) {                                                          m3log (emit, "slot: [%d]", i_وتھ);
        EmitWord32(o->ورقہ, i_وتھ);
    }
}

ساكن M3_NOINLINE
pc_t Emit_اشارہ_گر (IM3Compilation o, ثابت فراغ* ثابت i_اشارہ_گر)
{
    pc_t ptr = Get_ورقہ_PC(o->ورقہ);

    إذا (o->ورقہ) {                                                          m3log (emit, "ptr: %p", i_اشارہ_گر);
        Emit_لفظ(o->ورقہ, i_اشارہ_گر);
    }

    أرجع ptr;
}

ساكن M3_NOINLINE
فراغ* Reserve_اشارہ_گر (IM3Compilation o)
{
    pc_t ptr = Get_ورقہ_PC(o->ورقہ);
    Emit_اشارہ_گر(o, عدم);
    أرجع (فراغ*)ptr;
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

#إذا d_m3HasFloat
#  define FPOP(x) x
#وإلا
#  define FPOP(x) عدم
#جے_انت

// These are indexed by M3ValueType, so every type up to c_m3Type_externref needs
// an entry. A reference is one pointer-sized word, so it moves with the integer
// operation of that width; v128 has no operations at all.
#إذا M3_SIZEOF_PTR == 8
#  define REFOP(NAME)  op_##NAME##_i64
#وإلا
#  define REFOP(NAME)  op_##NAME##_i32
#جے_انت

// clang-format off

ساكن ثابت IM3Operation c_preserve_سیٹ_تھاں[] = { عدم, op_Preserve_سیٹ_تھاں_i32,       op_Preserve_سیٹ_تھاں_i64,
                                                    FPOP(op_Preserve_سیٹ_تھاں_f32), FPOP(op_Preserve_سیٹ_تھاں_f64),
                                                    عدم, REFOP(Preserve_سیٹ_تھاں),  REFOP(Preserve_سیٹ_تھاں),
                                                    REFOP(Preserve_سیٹ_تھاں),       REFOP(Preserve_سیٹ_تھاں) };
ساكن ثابت IM3Operation c_سیٹ_سیٹ_Ops[] =       { عدم, op_سیٹ_تھاں_i32,               op_سیٹ_تھاں_i64,
                                                    FPOP(op_سیٹ_تھاں_f32),         FPOP(op_سیٹ_تھاں_f64),
                                                    عدم, REFOP(سیٹ_تھاں),         REFOP(سیٹ_تھاں),
                                                    REFOP(سیٹ_تھاں),               REFOP(سیٹ_تھاں) };
ساكن ثابت IM3Operation c_سیٹ_Global_Ops[] =    { عدم, op_سیٹ_Global_i32,             op_سیٹ_Global_i64,
                                                    FPOP(op_سیٹ_Global_f32),       FPOP(op_سیٹ_Global_f64),
                                                    عدم, REFOP(سیٹ_Global),       REFOP(سیٹ_Global),
                                                    REFOP(سیٹ_Global),             REFOP(سیٹ_Global) };
ساكن ثابت IM3Operation c_سیٹ_رجسٹر_Ops[] =  { عدم, op_سیٹ_رجسٹر_i32,           op_سیٹ_رجسٹر_i64,
                                                    FPOP(op_سیٹ_رجسٹر_f32),     FPOP(op_سیٹ_رجسٹر_f64),
                                                    عدم, REFOP(سیٹ_رجسٹر),     REFOP(سیٹ_رجسٹر),
                                                    REFOP(سیٹ_رجسٹر),           REFOP(سیٹ_رجسٹر) };

// A table shorter than the enum reads out of bounds, so tie the two together at
// build time: adding a value type must not silently outgrow these.
#تعریف_کرو d_m3_Check_Type_جدول(جدول) \
    M3_STATIC_ASSERT (حجم (جدول) / حجم (*(جدول)) == c_m3_Type_گنتی, جدول##_needs_one_entry_per_type)

d_m3_Check_Type_جدول(c_preserve_سیٹ_تھاں);
d_m3_Check_Type_جدول(c_سیٹ_سیٹ_Ops);
d_m3_Check_Type_جدول(c_سیٹ_Global_Ops);
d_m3_Check_Type_جدول(c_سیٹ_رجسٹر_Ops);

#إذا d_m3_Fold_سیٹ_Local

// destination-folded variants of the hot binops, indexed by the operand-form index
// recorded at emission: [0]=_rs [1]=_sr [2]=_ss [3]=(fp _rr, never folded)
#تعریف_کرو d_foldOpList(TYPE, NAME)            { op_##TYPE##_##NAME##_rs_f, op_##TYPE##_##NAME##_sr_f, op_##TYPE##_##NAME##_ss_f, عدم }
#تعریف_کرو d_foldCommutativeOpList(TYPE, NAME) { op_##TYPE##_##NAME##_rs_f, عدم,                      op_##TYPE##_##NAME##_ss_f, عدم }

ساكن ثابت IM3Operation c_fold_i32_جوڑو[]         = d_foldCommutativeOpList (i32, جوڑو);
ساكن ثابت IM3Operation c_fold_i32_گھٹاؤ[]    = d_foldOpList (i32, گھٹاؤ);
ساكن ثابت IM3Operation c_fold_i32_Multiply[]    = d_foldCommutativeOpList (i32, Multiply);
ساكن ثابت IM3Operation c_fold_u32_And[]         = d_foldCommutativeOpList (u32, And);
ساكن ثابت IM3Operation c_fold_u32_Or[]          = d_foldCommutativeOpList (u32, Or);
ساكن ثابت IM3Operation c_fold_u32_Xor[]         = d_foldCommutativeOpList (u32, Xor);
ساكن ثابت IM3Operation c_fold_u32_کھسکاؤ_بایاں[]   = d_foldOpList (u32, کھسکاؤ_بایاں);
ساكن ثابت IM3Operation c_fold_i32_کھسکاؤ_دایاں[]  = d_foldOpList (i32, کھسکاؤ_دایاں);
ساكن ثابت IM3Operation c_fold_u32_کھسکاؤ_دایاں[]  = d_foldOpList (u32, کھسکاؤ_دایاں);

ساكن ثابت IM3Operation c_fold_i64_جوڑو[]         = d_foldCommutativeOpList (i64, جوڑو);
ساكن ثابت IM3Operation c_fold_i64_گھٹاؤ[]    = d_foldOpList (i64, گھٹاؤ);
ساكن ثابت IM3Operation c_fold_i64_Multiply[]    = d_foldCommutativeOpList (i64, Multiply);
ساكن ثابت IM3Operation c_fold_u64_And[]         = d_foldCommutativeOpList (u64, And);
ساكن ثابت IM3Operation c_fold_u64_Or[]          = d_foldCommutativeOpList (u64, Or);
ساكن ثابت IM3Operation c_fold_u64_Xor[]         = d_foldCommutativeOpList (u64, Xor);
ساكن ثابت IM3Operation c_fold_u64_کھسکاؤ_بایاں[]   = d_foldOpList (u64, کھسکاؤ_بایاں);
ساكن ثابت IM3Operation c_fold_i64_کھسکاؤ_دایاں[]  = d_foldOpList (i64, کھسکاؤ_دایاں);
ساكن ثابت IM3Operation c_fold_u64_کھسکاؤ_دایاں[]  = d_foldOpList (u64, کھسکاؤ_دایاں);

#  إذا d_m3HasFloat
ساكن ثابت IM3Operation c_fold_f32_جوڑو[]         = d_foldCommutativeOpList (f32, جوڑو);
ساكن ثابت IM3Operation c_fold_f32_گھٹاؤ[]    = d_foldOpList (f32, گھٹاؤ);
ساكن ثابت IM3Operation c_fold_f32_Multiply[]    = d_foldCommutativeOpList (f32, Multiply);
ساكن ثابت IM3Operation c_fold_f32_Divide[]      = d_foldOpList (f32, Divide);
ساكن ثابت IM3Operation c_fold_f64_جوڑو[]         = d_foldCommutativeOpList (f64, جوڑو);
ساكن ثابت IM3Operation c_fold_f64_گھٹاؤ[]    = d_foldOpList (f64, گھٹاؤ);
ساكن ثابت IM3Operation c_fold_f64_Multiply[]    = d_foldCommutativeOpList (f64, Multiply);
ساكن ثابت IM3Operation c_fold_f64_Divide[]      = d_foldOpList (f64, Divide);
#  endif

// loads are unary: [0]= address in _r0, [1]= address in a slot
#تعریف_کرو d_fold_لدو_List(DEST, SRC)       { op_##DEST##_Load_##SRC##_r_f, op_##DEST##_Load_##SRC##_s_f, عدم, عدم }

ساكن ثابت IM3Operation c_fold_i32_لدو_i32[] = d_fold_لدو_List(i32, i32);
ساكن ثابت IM3Operation c_fold_i32_لدو_i8[] = d_fold_لدو_List(i32, i8);
ساكن ثابت IM3Operation c_fold_i32_لدو_u8[] = d_fold_لدو_List(i32, u8);
ساكن ثابت IM3Operation c_fold_i32_لدو_i16[] = d_fold_لدو_List(i32, i16);
ساكن ثابت IM3Operation c_fold_i32_لدو_u16[] = d_fold_لدو_List(i32, u16);

ساكن ثابت IM3Operation c_fold_i64_لدو_i64[] = d_fold_لدو_List(i64, i64);
ساكن ثابت IM3Operation c_fold_i64_لدو_i8[] = d_fold_لدو_List(i64, i8);
ساكن ثابت IM3Operation c_fold_i64_لدو_u8[] = d_fold_لدو_List(i64, u8);
ساكن ثابت IM3Operation c_fold_i64_لدو_i16[] = d_fold_لدو_List(i64, i16);
ساكن ثابت IM3Operation c_fold_i64_لدو_u16[] = d_fold_لدو_List(i64, u16);
ساكن ثابت IM3Operation c_fold_i64_لدو_i32[] = d_fold_لدو_List(i64, i32);
ساكن ثابت IM3Operation c_fold_i64_لدو_u32[] = d_fold_لدو_List(i64, u32);
#  إذا d_m3HasFloat
ساكن ثابت IM3Operation c_fold_f32_لدو_f32[] = d_fold_لدو_List(f32, f32);
ساكن ثابت IM3Operation c_fold_f64_لدو_f64[] = d_fold_لدو_List(f64, f64);
#  endif

ساكن
IM3Operation GetFoldOp (m3opcode_t i_opcode, u8 i_form)
{
    إذا (i_form >= 4) {
        أرجع عدم;
    }

    بدل (i_opcode) {
    حالة 0x6a:  أرجع c_fold_i32_جوڑو        [i_form];      // i32.add
    حالة 0x6b:  أرجع c_fold_i32_گھٹاؤ   [i_form];      // i32.sub
    حالة 0x6c:  أرجع c_fold_i32_Multiply   [i_form];      // i32.mul
    حالة 0x71:  أرجع c_fold_u32_And        [i_form];      // i32.and
    حالة 0x72:  أرجع c_fold_u32_Or         [i_form];      // i32.or
    حالة 0x73:  أرجع c_fold_u32_Xor        [i_form];      // i32.xor
    حالة 0x74:  أرجع c_fold_u32_کھسکاؤ_بایاں  [i_form];      // i32.shl
    حالة 0x75:  أرجع c_fold_i32_کھسکاؤ_دایاں [i_form];      // i32.shr_s
    حالة 0x76:  أرجع c_fold_u32_کھسکاؤ_دایاں [i_form];      // i32.shr_u

    حالة 0x7c:  أرجع c_fold_i64_جوڑو        [i_form];      // i64.add
    حالة 0x7d:  أرجع c_fold_i64_گھٹاؤ   [i_form];      // i64.sub
    حالة 0x7e:  أرجع c_fold_i64_Multiply   [i_form];      // i64.mul
    حالة 0x83:  أرجع c_fold_u64_And        [i_form];      // i64.and
    حالة 0x84:  أرجع c_fold_u64_Or         [i_form];      // i64.or
    حالة 0x85:  أرجع c_fold_u64_Xor        [i_form];      // i64.xor
    حالة 0x86:  أرجع c_fold_u64_کھسکاؤ_بایاں  [i_form];      // i64.shl
    حالة 0x87:  أرجع c_fold_i64_کھسکاؤ_دایاں [i_form];      // i64.shr_s
    حالة 0x88:  أرجع c_fold_u64_کھسکاؤ_دایاں [i_form];      // i64.shr_u

#  إذا d_m3HasFloat
    حالة 0x92:  أرجع c_fold_f32_جوڑو        [i_form];      // f32.add
    حالة 0x93:  أرجع c_fold_f32_گھٹاؤ   [i_form];      // f32.sub
    حالة 0x94:  أرجع c_fold_f32_Multiply   [i_form];      // f32.mul
    حالة 0x95:  أرجع c_fold_f32_Divide     [i_form];      // f32.div
    حالة 0xa0:  أرجع c_fold_f64_جوڑو        [i_form];      // f64.add
    حالة 0xa1:  أرجع c_fold_f64_گھٹاؤ   [i_form];      // f64.sub
    حالة 0xa2:  أرجع c_fold_f64_Multiply   [i_form];      // f64.mul
    حالة 0xa3:  أرجع c_fold_f64_Divide     [i_form];      // f64.div
#  endif

    حالة 0x28:  أرجع c_fold_i32_لدو_i32   [i_form];      // i32.load
    حالة 0x2c:  أرجع c_fold_i32_لدو_i8    [i_form];      // i32.load8_s
    حالة 0x2d:  أرجع c_fold_i32_لدو_u8    [i_form];      // i32.load8_u
    حالة 0x2e:  أرجع c_fold_i32_لدو_i16   [i_form];      // i32.load16_s
    حالة 0x2f:  أرجع c_fold_i32_لدو_u16   [i_form];      // i32.load16_u

    حالة 0x29:  أرجع c_fold_i64_لدو_i64   [i_form];      // i64.load
    حالة 0x30:  أرجع c_fold_i64_لدو_i8    [i_form];      // i64.load8_s
    حالة 0x31:  أرجع c_fold_i64_لدو_u8    [i_form];      // i64.load8_u
    حالة 0x32:  أرجع c_fold_i64_لدو_i16   [i_form];      // i64.load16_s
    حالة 0x33:  أرجع c_fold_i64_لدو_u16   [i_form];      // i64.load16_u
    حالة 0x34:  أرجع c_fold_i64_لدو_i32   [i_form];      // i64.load32_s
    حالة 0x35:  أرجع c_fold_i64_لدو_u32   [i_form];      // i64.load32_u

#  إذا d_m3HasFloat
    حالة 0x2a:  أرجع c_fold_f32_لدو_f32   [i_form];      // f32.load
    حالة 0x2b:  أرجع c_fold_f64_لدو_f64   [i_form];      // f64.load
#  endif

    افتراضي: أرجع عدم;
    }
}

#جے_انت // d_m3FoldSetLocal

// clang-format on

#إذا d_m3FuseBranch

// fused compare+branch/if variants, indexed like the fold lists: [0]=_rs [1]=_sr [2]=_ss [3]=unused.
// the [1] entries are NULL for commutative compares: the compiler never records the _sr form for them
#  define d_fuseCmpList(TYPE, NAME, قسم_2)             { op_##TYPE##_##KIND##_##NAME##_rs, op_##TYPE##_##KIND##_##NAME##_sr, op_##TYPE##_##KIND##_##NAME##_ss, عدم }
#  define d_fuseCommutativeCmpList(TYPE, NAME, قسم_2)  { op_##TYPE##_##KIND##_##NAME##_rs, عدم,                             op_##TYPE##_##KIND##_##NAME##_ss, عدم }

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

#جے_انت // d_m3FuseBranch

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
#جے_انت

// all args & returns are 64-bit aligned, so use 2 slots for a d_m3Use32BitSlots=1 build

ساكن
M3_نتیجہ Acquire_Compilation_کوڈ_ورقہ (IM3Compilation o, IM3_کوڈ_ورقہ* o_کوڈ_ورقہ)
{
    M3_نتیجہ نتیجہ = m3Err_none;

    IM3_کوڈ_ورقہ ورقہ = Acquire_کوڈ_ورقہ(o->runtime);

    إذا (ورقہ) {
#إذا (d_m3_Enable_کوڈ_ورقہ_Ref_Counting)
        {
            إذا (o->function) {
                IM3Function func = o->function;
                ورقہ->info.usage_گنتی++;

                u32 index = func->عدد_کوڈ_ورقہ_Refs++;
_               (m3ReallocArray(&func->کوڈ_ورقہ_Refs, IM3_کوڈ_ورقہ, func->عدد_کوڈ_ورقہ_Refs, index));
                func->کوڈ_ورقہ_Refs[index] = ورقہ;
            }
        }
#جے_انت
    } وإلا {
        _throw(m3_Err_malloc_Failed_کوڈ_ورقہ);
    }

    _catch:

    *o_کوڈ_ورقہ = ورقہ;

    أرجع نتیجہ;
}

ساكن مضمن
فراغ Release_Compilation_کوڈ_ورقہ (IM3Compilation o)
{
    Release_کوڈ_ورقہ(o->runtime, o->ورقہ);
}

ساكن مضمن
u16 Get_Type_عدد_Slots (m3type_t i_type)
{
    i_type = ادھار_Type_دا(i_type);

    // v128 is 16 bytes - 4 slots in 32-bit-slot mode, 2 in 64-bit.
    // (Slot-allocator only; no v128 ops execute.)
    إذا (i_type == c_m3Type_v128)
#إذا d_m3_Use32_ثنائی_ہندسہ_Slots
        أرجع 4;
#وإلا
        أرجع 2;
#جے_انت
#إذا d_m3_Use32_ثنائی_ہندسہ_Slots
    أرجع Is64_ثنائی_ہندسہ_Type(i_type) ? 2 : 1;
#وإلا
    أرجع 1;
#جے_انت
}

ساكن مضمن
فراغ Align_تھاں_منزل_Type (u16* io_تھاں, m3type_t i_type)
{
    // align 64-bit words to even slots (if d_m3Use32BitSlots)
    u16 عدد_Slots = Get_Type_عدد_Slots(i_type);

    u16 نقاب = عدد_Slots - 1;
    *io_تھاں = (*io_تھاں + نقاب) & ~نقاب;
}

// Deliberately unchecked. Validation rejects an empty operand stack before compilation.
ساكن مضمن
i16 Get_ڈھیر_چوٹی_اشاریہ (IM3Compilation o)
{
    أرجع o->ڈھیر_اشاریہ - 1;
}

ساكن مضمن
M3_نتیجہ Get_ڈھیر_چوٹی_اشاریہ_Throws (IM3Compilation o, i16* o_ڈھیر_اشاریہ)
{
    *o_ڈھیر_اشاریہ = o->ڈھیر_اشاریہ - 1;

    إذا (M3_LIKELY(o->ڈھیر_اشاریہ > o->ڈھیر_پہلا_Dynamic_اشاریہ or Is_ڈھیر_Polymorphic(o))) {
        أرجع m3Err_none;
    } وإلا {
        أرجع m3_Err_function_ڈھیر_Underrun;
    }
}


// Items in the static portion of the stack (args/locals) are hidden from GetStackTypeFromTop ()
// In other words, only "real" Wasm stack items can be inspected.  This is important when
// returning values, etc. and you need an accurate wasm-view of the stack.
ساكن
m3type_t Get_ڈھیر_Type_From_چوٹی (IM3Compilation o, u16 i_وتھ)
{
    m3type_t type = c_m3Type_none;

    ++i_وتھ;
    إذا (o->ڈھیر_اشاریہ >= i_وتھ) {
        u16 index = o->ڈھیر_اشاریہ - i_وتھ;

        إذا (index >= o->ڈھیر_پہلا_Dynamic_اشاریہ) {
            type = o->type_ڈھیر[index];
        }
    }

    أرجع type;
}

ساكن مضمن
m3type_t Get_ڈھیر_چوٹی_Type (IM3Compilation o)
{
    أرجع Get_ڈھیر_Type_From_چوٹی(o, 0);
}

ساكن مضمن
m3type_t Get_ڈھیر_Type_From_Bottom (IM3Compilation o, u16 i_وتھ)
{
    m3type_t type = c_m3Type_none;

    إذا (i_وتھ < o->ڈھیر_اشاریہ) {
        type = o->type_ڈھیر[i_وتھ];
    }

    أرجع type;
}


ساكن مضمن
bool Is_Constant_تھاں (IM3Compilation o, u16 i_تھاں)
{
    أرجع (i_تھاں >= o->تھاں_پہلا_Const_اشاریہ and i_تھاں < o->تھاں_Max_Const_اشاریہ);
}
ساكن مضمن
bool Is_تھاں_Allocated (IM3Compilation o, u16 i_تھاں)
{
    أرجع o->m3Slots[i_تھاں];
}

ساكن مضمن
bool Is_ڈھیر_اشاریہ_وچ_رجسٹر (IM3Compilation o, i32 i_ڈھیر_اشاریہ)
{                                                                           d_m3Assert (i_ڈھیر_اشاریہ < o->ڈھیر_اشاریہ or Is_ڈھیر_Polymorphic (o));
    إذا (i_ڈھیر_اشاریہ >= 0 and i_ڈھیر_اشاریہ < o->ڈھیر_اشاریہ) {
        أرجع (o->wasm_ڈھیر[i_ڈھیر_اشاریہ] >= d_m3_Reg0_تھاں_Alias);
    } وإلا {
        أرجع false;
    }
}

ساكن مضمن
u16 Get_عدد_بلاک_اقدار_On_ڈھیر (IM3Compilation o)
{
    أرجع o->ڈھیر_اشاریہ - o->بلاک.بلاک_ڈھیر_اشاریہ;
}

ساكن مضمن
bool Is_ڈھیر_چوٹی_وچ_رجسٹر (IM3Compilation o)
{
    أرجع Is_ڈھیر_اشاریہ_وچ_رجسٹر(o, (i32)Get_ڈھیر_چوٹی_اشاریہ(o));
}
ساكن مضمن
bool Is_ڈھیر_چوٹی_Minus1_وچ_رجسٹر (IM3Compilation o)
{
    أرجع Is_ڈھیر_اشاریہ_وچ_رجسٹر(o, (i32)Get_ڈھیر_چوٹی_اشاریہ(o) - 1);
}
ساكن مضمن
bool Is_ڈھیر_چوٹی_Minus2_وچ_رجسٹر (IM3Compilation o)
{
    أرجع Is_ڈھیر_اشاریہ_وچ_رجسٹر(o, (i32)Get_ڈھیر_چوٹی_اشاریہ(o) - 2);
}

ساكن مضمن
bool Is_ڈھیر_چوٹی_وچ_تھاں (IM3Compilation o)
{
    أرجع not Is_ڈھیر_چوٹی_وچ_رجسٹر(o);
}

ساكن مضمن
bool Is_Valid_تھاں (u16 i_تھاں)
{
    أرجع (i_تھاں < d_m3MaxFunctionSlots);
}

ساكن مضمن
u16 Get_ڈھیر_چوٹی_تھاں_نمبر (IM3Compilation o)
{
    i16 i = Get_ڈھیر_چوٹی_اشاریہ(o);

    u16 تھاں = c_تھاں_بن_ورتیا;

    إذا (i >= 0) {
        تھاں = o->wasm_ڈھیر[i];
    }

    أرجع تھاں;
}


// from bottom
ساكن مضمن
u16 Get_تھاں_For_ڈھیر_اشاریہ (IM3Compilation o, u16 i_ڈھیر_اشاریہ)
{                                                                   d_m3Assert (i_ڈھیر_اشاریہ < o->ڈھیر_اشاریہ or Is_ڈھیر_Polymorphic (o));
    u16 تھاں = c_تھاں_بن_ورتیا;

    إذا (i_ڈھیر_اشاریہ < o->ڈھیر_اشاریہ) {
        تھاں = o->wasm_ڈھیر[i_ڈھیر_اشاریہ];
    }

    أرجع تھاں;
}

ساكن مضمن
u16 Get_Extra_تھاں_For_ڈھیر_اشاریہ (IM3Compilation o, u16 i_ڈھیر_اشاریہ)
{
    u16 ادھار_تھاں = Get_تھاں_For_ڈھیر_اشاریہ(o, i_ڈھیر_اشاریہ);

    إذا (ادھار_تھاں != c_تھاں_بن_ورتیا) {
        u16 extra_تھاں = Get_Type_عدد_Slots(Get_ڈھیر_Type_From_Bottom(o, i_ڈھیر_اشاریہ)) - 1;
        ادھار_تھاں += extra_تھاں;
    }

    أرجع ادھار_تھاں;
}


ساكن مضمن
فراغ Touch_تھاں (IM3Compilation o, u16 i_تھاں)
{
    // op_Entry uses this value to track and detect stack overflow
    o->max_ڈھیر_Slots = M3_MAX(o->max_ڈھیر_Slots, i_تھاں + 1);
}

ساكن مضمن
فراغ Mark_تھاں_Allocated (IM3Compilation o, u16 i_تھاں)
{                                                                   d_m3Assert (o->m3Slots [i_تھاں] == 0); // shouldn't be already allocated
    o->m3Slots[i_تھاں] = 1;

    o->تھاں_Max_Allocated_اشاریہ_Plus_One = M3_MAX(o->تھاں_Max_Allocated_اشاریہ_Plus_One, i_تھاں + 1);

    Touch_تھاں(o, i_تھاں);
}

ساكن مضمن
M3_نتیجہ MarkSlotsAllocated (IM3Compilation o, u16 i_تھاں, u16 i_عدد_Slots)
{
    إذا (M3_UNLIKELY(i_تھاں + i_عدد_Slots > d_m3MaxFunctionSlots)) {
        أرجع m3_Err_function_ڈھیر_حد_توں_ودھ;
    }

    طالما (i_عدد_Slots--) {
        Mark_تھاں_Allocated(o, i_تھاں++);
    }

    أرجع m3Err_none;
}

ساكن مضمن
M3_نتیجہ MarkSlotsAllocatedByType (IM3Compilation o, u16 i_تھاں, m3type_t i_type)
{
    u16 عدد_Slots = Get_Type_عدد_Slots(i_type);
    أرجع MarkSlotsAllocated(o, i_تھاں, عدد_Slots);
}


ساكن
M3_نتیجہ AllocateSlotsWithinRange (IM3Compilation o, u16* o_تھاں, m3type_t i_type, u16 i_شروع_تھاں, u16 i_سرا_تھاں)
{
    M3_نتیجہ نتیجہ = m3_Err_function_ڈھیر_حد_توں_ودھ;

    u16 عدد_Slots     = Get_Type_عدد_Slots(i_type);
    u16 تلاش_وتھ = عدد_Slots - 1;

    Align_تھاں_منزل_Type(&i_شروع_تھاں, i_type);

    // search for 1 or 2 consecutive slots in the execution stack
    u16 i = i_شروع_تھاں;
    طالما (i + تلاش_وتھ < i_سرا_تھاں) {
        إذا (i + تلاش_وتھ < d_m3MaxFunctionSlots and o->m3Slots[i] == 0 and o->m3Slots[i + تلاش_وتھ] == 0) {
            MarkSlotsAllocated(o, i, عدد_Slots);

            *o_تھاں = i;
            نتیجہ  = m3Err_none;
            اكسر;
        }

        // keep 2-slot allocations even-aligned
        i += عدد_Slots;
    }

    أرجع نتیجہ;
}

ساكن مضمن
M3_نتیجہ AllocateSlots (IM3Compilation o, u16* o_تھاں, m3type_t i_type)
{
    أرجع AllocateSlotsWithinRange(o, o_تھاں, i_type, o->تھاں_پہلا_Dynamic_اشاریہ, d_m3MaxFunctionSlots);
}

ساكن مضمن
M3_نتیجہ AllocateConstantSlots (IM3Compilation o, u16* o_تھاں, m3type_t i_type)
{
    u16 max_جدول_اشاریہ = o->تھاں_پہلا_Const_اشاریہ + d_m3_Max_Constant_جدول_ناپ;
    أرجع AllocateSlotsWithinRange(o, o_تھاں, i_type, o->تھاں_پہلا_Const_اشاریہ, M3_MIN(o->تھاں_پہلا_Dynamic_اشاریہ, max_جدول_اشاریہ));
}


// TOQUE: this usage count system could be eliminated. real world code doesn't frequently trigger it.  just copy to multiple
// unique slots.
ساكن مضمن
M3_نتیجہ Increment_تھاں_Usage_گنتی (IM3Compilation o, u16 i_تھاں)
{                                                                                       d_m3Assert (i_تھاں < d_m3MaxFunctionSlots);
    M3_نتیجہ نتیجہ = m3Err_none;                                                       d_m3Assert (o->m3Slots [i_تھاں] > 0);

    // OPTZ (memory): 'm3Slots' could still be fused with 'typeStack' if 4 bits were used to indicate: [0,1,2,many]. The many-case
    // would scan 'wasmStack' to determine the actual usage count
    إذا (M3_LIKELY(o->m3Slots[i_تھاں] < 0xFF)) {
        o->m3Slots[i_تھاں]++;
    } وإلا {
        نتیجہ = "slot usage count overflow";
    }

    أرجع نتیجہ;
}

ساكن مضمن
فراغ Deallocate_تھاں (IM3Compilation o, i16 i_تھاں, m3type_t i_type)
{                                                                                       d_m3Assert (i_تھاں >= o->تھاں_پہلا_Dynamic_اشاریہ);
                                                                                        d_m3Assert (i_تھاں < o->تھاں_Max_Allocated_اشاریہ_Plus_One);
    لكل (u16 i = 0; i < Get_Type_عدد_Slots(i_type); ++i, ++i_تھاں) {                       d_m3Assert (o->m3Slots [i_تھاں]);
        --o->m3Slots[i_تھاں];
    }
}


ساكن مضمن
bool Is_رجسٹر_Type_Allocated (IM3Compilation o, u8 i_type)
{
    أرجع Is_رجسٹر_Allocated(o, IsFpType(i_type));
}

ساكن مضمن
فراغ Allocate_رجسٹر (IM3Compilation o, u32 i_رجسٹر, u16 i_ڈھیر_اشاریہ)
{                                                                                       d_m3Assert (not Is_رجسٹر_Allocated (o, i_رجسٹر));
    o->reg_ڈھیر_اشاریہ_Plus_One[i_رجسٹر] = i_ڈھیر_اشاریہ + 1;
}

ساكن مضمن
فراغ Deallocate_رجسٹر (IM3Compilation o, u32 i_رجسٹر)
{                                                                                       d_m3Assert (Is_رجسٹر_Allocated (o, i_رجسٹر));
    o->reg_ڈھیر_اشاریہ_Plus_One[i_رجسٹر] = c_m3_رجسٹر_Unallocated;
}

ساكن مضمن
u16 Get_رجسٹر_ڈھیر_اشاریہ (IM3Compilation o, u32 i_رجسٹر)
{                                                                                       d_m3Assert (Is_رجسٹر_Allocated (o, i_رجسٹر));
    أرجع o->reg_ڈھیر_اشاریہ_Plus_One[i_رجسٹر] - 1;
}

u16 Get_Max_ورتیا_تھاں_Plus_One (IM3Compilation o)
{
    طالما (o->تھاں_Max_Allocated_اشاریہ_Plus_One > o->تھاں_پہلا_Dynamic_اشاریہ) {
        إذا (Is_تھاں_Allocated(o, o->تھاں_Max_Allocated_اشاریہ_Plus_One - 1)) {
            اكسر;
        }

        o->تھاں_Max_Allocated_اشاریہ_Plus_One--;
    }

#جے_تعریف_شدہ DEBUG
    u16 max_تھاں = o->تھاں_Max_Allocated_اشاریہ_Plus_One;
    طالما (max_تھاں < d_m3MaxFunctionSlots) {
        d_m3Assert(o->m3Slots[max_تھاں] == 0);
        max_تھاں++;
    }
#جے_انت

    أرجع o->تھاں_Max_Allocated_اشاریہ_Plus_One;
}

ساكن
M3_نتیجہ Preserve_رجسٹر_If_Occupied (IM3Compilation o, m3type_t i_رجسٹر_Type)
{
    M3_نتیجہ نتیجہ = m3Err_none;

    u32 regSelect = IsFpType(i_رجسٹر_Type);

    إذا (Is_رجسٹر_Allocated(o, regSelect)) {
        u16 ڈھیر_اشاریہ = Get_رجسٹر_ڈھیر_اشاریہ(o, regSelect);
        Deallocate_رجسٹر(o, regSelect);

        m3type_t type = Get_ڈھیر_Type_From_Bottom(o, ڈھیر_اشاریہ);

        // and point to a exec slot
        u16 تھاں = c_تھاں_بن_ورتیا;
_       (AllocateSlots(o, &تھاں, type));
        o->wasm_ڈھیر[ڈھیر_اشاریہ] = تھاں;

        // Ensure type is within the valid range
        إذا (M3_LIKELY(ادھار_Type_دا(type) < c_m3_Type_گنتی)) {
_           (EmitOp(o, c_سیٹ_سیٹ_Ops[ادھار_Type_دا(type)]));
        } وإلا {
            _throw(m3Err_unknownType);
        }

        Emit_تھاں_وتھ(o, تھاں);
    }

    _catch: أرجع نتیجہ;
}


// all values must be in slots before entering loop, if, and else blocks
// otherwise they'd end up preserve-copied in the block to probably different locations (if/else)
ساكن مضمن
M3_نتیجہ PreserveRegisters (IM3Compilation o)
{
    M3_نتیجہ نتیجہ;

_   (Preserve_رجسٹر_If_Occupied(o, c_m3Type_f64));
_   (Preserve_رجسٹر_If_Occupied(o, c_m3Type_i64));

    _catch: أرجع نتیجہ;
}

ساكن
M3_نتیجہ Preserve_Non_چوٹی_Registers (IM3Compilation o)
{
    M3_نتیجہ نتیجہ = m3Err_none;

    i16 ڈھیر_چوٹی = Get_ڈھیر_چوٹی_اشاریہ(o);

    إذا (ڈھیر_چوٹی >= 0) {
        إذا (Is_رجسٹر_Allocated(o, 0))     // r0
        {
            إذا (Get_رجسٹر_ڈھیر_اشاریہ(o, 0) != ڈھیر_چوٹی) {
_               (Preserve_رجسٹر_If_Occupied(o, c_m3Type_i64));
            }
        }

        إذا (Is_رجسٹر_Allocated(o, 1))     // fp0
        {
            إذا (Get_رجسٹر_ڈھیر_اشاریہ(o, 1) != ڈھیر_چوٹی) {
_               (Preserve_رجسٹر_If_Occupied(o, c_m3Type_f64));
            }
        }
    }

    _catch: أرجع نتیجہ;
}


//----------------------------------------------------------------------------------------------------------------------

ساكن
M3_نتیجہ ڈالو (IM3Compilation o, m3type_t i_type, u16 i_تھاں)
{
    M3_نتیجہ نتیجہ = m3Err_none;

#إذا !d_m3HasFloat
    إذا (i_type == c_m3Type_f32 || i_type == c_m3Type_f64) {
        أرجع m3Err_unknownOpcode;
    }
#جے_انت

    إذا (M3_UNLIKELY(o->ڈھیر_اشاریہ >= d_m3_Max_Function_ڈھیر_اچائی)) {
        أرجع m3_Err_function_ڈھیر_حد_توں_ودھ;
    }

    u16 ڈھیر_اشاریہ = o->ڈھیر_اشاریہ++;                                       // printf ("push: %d\n", (i32) i);

    o->wasm_ڈھیر[ڈھیر_اشاریہ] = i_تھاں;
    o->type_ڈھیر[ڈھیر_اشاریہ] = i_type;

    إذا (Is_رجسٹر_تھاں_Alias(i_تھاں)) {
        u32 regSelect = Is_Fp_رجسٹر_تھاں_Alias(i_تھاں);
        Allocate_رجسٹر(o, regSelect, ڈھیر_اشاریہ);
    }

    إذا (d_m3_Log_Wasm_ڈھیر) {
        dump_type_ڈھیر(o);
    }

    أرجع نتیجہ;
}

ساكن مضمن
M3_نتیجہ ڈالو_رجسٹر (IM3Compilation o, m3type_t i_type)
{
    M3_نتیجہ نتیجہ = m3Err_none;                                                       d_m3Assert ((u16) d_m3_Reg0_تھاں_Alias > (u16) d_m3MaxFunctionSlots);
    u16      تھاں   = IsFpType(i_type) ? d_m3_Fp0_تھاں_Alias : d_m3_Reg0_تھاں_Alias;          d_m3Assert (i_type or Is_ڈھیر_Polymorphic (o));

_   (ڈالو(o, i_type, تھاں));

    _catch: أرجع نتیجہ;
}

ساكن
M3_نتیجہ نکالو (IM3Compilation o)
{
    M3_نتیجہ نتیجہ = m3Err_none;

    إذا (M3_LIKELY(o->ڈھیر_اشاریہ > o->بلاک.بلاک_ڈھیر_اشاریہ)) {
        o->ڈھیر_اشاریہ--;                                                //  printf ("pop: %d\n", (i32) o->stackIndex);

        u16      تھاں = o->wasm_ڈھیر[o->ڈھیر_اشاریہ];
        m3type_t type = o->type_ڈھیر[o->ڈھیر_اشاریہ];

        إذا (Is_رجسٹر_تھاں_Alias(تھاں)) {
            u32 regSelect = Is_Fp_رجسٹر_تھاں_Alias(تھاں);
            Deallocate_رجسٹر(o, regSelect);
        } وإلا إذا (M3_UNLIKELY(تھاں >= o->تھاں_Max_Allocated_اشاریہ_Plus_One)) {
            أرجع m3_Err_function_ڈھیر_Underrun; // Return error for invalid slot indices
        } وإلا إذا (تھاں >= o->تھاں_پہلا_Dynamic_اشاریہ) {
            Deallocate_تھاں(o, تھاں, type);
        }
    } وإلا إذا (not Is_ڈھیر_Polymorphic(o)) {
        نتیجہ = m3_Err_function_ڈھیر_Underrun;
    }

    أرجع نتیجہ;
}

ساكن
M3_نتیجہ نکالو_Type (IM3Compilation o, m3type_t i_type)
{
    M3_نتیجہ نتیجہ = m3Err_none;

    m3type_t چوٹی_Type = Get_ڈھیر_چوٹی_Type(o);

    إذا (M3_LIKELY(Is_Sub_Type_دا(چوٹی_Type, i_type) or o->بلاک.isPolymorphic)) {
_       (نکالو(o));
    } وإلا {
        _throw(m3Err_typeMismatch);
    }

    _catch:
    أرجع نتیجہ;
}

ساكن
M3_نتیجہ ڈالو_Allocated_تھاں_And_Emit (IM3Compilation o, m3type_t i_type, bool i_doEmit)
{
    M3_نتیجہ نتیجہ = m3Err_none;

    u16 تھاں = c_تھاں_بن_ورتیا;

_   (AllocateSlots(o, &تھاں, i_type));
_   (ڈالو(o, i_type, تھاں));

    إذا (i_doEmit) {
        Emit_تھاں_وتھ(o, تھاں);
    }

    //    printf ("push: %d\n", (u32) slot);

    _catch: أرجع نتیجہ;
}

ساكن مضمن
M3_نتیجہ ڈالو_Allocated_تھاں_And_Emit_2 (IM3Compilation o, m3type_t i_type)
{
    أرجع ڈالو_Allocated_تھاں_And_Emit(o, i_type, true);
}

ساكن مضمن
M3_نتیجہ ڈالو_Allocated_تھاں (IM3Compilation o, m3type_t i_type)
{
    أرجع ڈالو_Allocated_تھاں_And_Emit(o, i_type, false);
}

ساكن
M3_نتیجہ ڈالو_Const (IM3Compilation o, u64 i_لفظ, m3type_t i_type)
{
    M3_نتیجہ نتیجہ = m3Err_none;

    // When compile-walking a constant expression without emitting there is no
    // constant table to place the value in, but the type still has to land on
    // the stack: extended-const arithmetic downstream consumes it.
    إذا (M3_UNLIKELY(!o->ورقہ)) {
        أرجع ڈالو_Allocated_تھاں(o, i_type);
    }

    bool میل_لبھیا  = false;
    bool is64_ثنائی_ہندسہ_Type = Is64_ثنائی_ہندسہ_Type(i_type);

    u16 عدد_Required_Slots  = Get_Type_عدد_Slots(i_type);
    u16 عدد_ورتیا_Const_Slots = o->تھاں_Max_Const_اشاریہ - o->تھاں_پہلا_Const_اشاریہ;

    // search for duplicate matching constant slot to reuse
    إذا (عدد_Required_Slots == 2 and عدد_ورتیا_Const_Slots >= 2) {
        u16 پہلا_Const_تھاں = o->تھاں_پہلا_Const_اشاریہ;
        Align_تھاں_منزل_Type(&پہلا_Const_تھاں, c_m3Type_i64);

        لكل (u16 تھاں = پہلا_Const_تھاں; تھاں < o->تھاں_Max_Const_اشاریہ - 1; تھاں += 2) {
            إذا (Is_تھاں_Allocated(o, تھاں) and Is_تھاں_Allocated(o, تھاں + 1)) {
                u64 constant;
                حافظہ_نقل(&constant, &o->constants[تھاں - o->تھاں_پہلا_Const_اشاریہ], حجم(constant));

                إذا (constant == i_لفظ) {
                    میل_لبھیا = true;
_                   (ڈالو(o, i_type, تھاں));
                    اكسر;
                }
            }
        }
    } وإلا إذا (عدد_Required_Slots == 1) {
        لكل (u16 i = 0; i < عدد_ورتیا_Const_Slots; ++i) {
            u16 تھاں = o->تھاں_پہلا_Const_اشاریہ + i;

            إذا (Is_تھاں_Allocated(o, تھاں)) {
                bool matches;
                إذا (is64_ثنائی_ہندسہ_Type) {
                    u64 constant;
                    حافظہ_نقل(&constant, &o->constants[i], حجم(constant));
                    matches = (constant == i_لفظ);
                } وإلا {
                    u32 constant;
                    حافظہ_نقل(&constant, &o->constants[i], حجم(constant));
                    matches = (constant == i_لفظ);
                }
                إذا (matches) {
                    میل_لبھیا = true;
_                   (ڈالو(o, i_type, تھاں));
                    اكسر;
                }
            }
        }
    }

    إذا (not میل_لبھیا) {
        u16 تھاں = c_تھاں_بن_ورتیا;
        نتیجہ   = AllocateConstantSlots(o, &تھاں, i_type);

        إذا (نتیجہ || تھاں == c_تھاں_بن_ورتیا) // no more constant table space; use inline constants
        {
            نتیجہ = m3Err_none;

            إذا (is64_ثنائی_ہندسہ_Type) {
_               (EmitOp(o, op_Const64));
                EmitWord64(o->ورقہ, i_لفظ);
            } وإلا {
_               (EmitOp(o, op_Const32));
                EmitWord32(o->ورقہ, (u32)i_لفظ);
            }

_           (ڈالو_Allocated_تھاں_And_Emit_2(o, i_type));
        } وإلا {
            u16 const_جدول_اشاریہ = تھاں - o->تھاں_پہلا_Const_اشاریہ;

            d_m3Assert(const_جدول_اشاریہ < d_m3_Max_Constant_جدول_ناپ);

            إذا (is64_ثنائی_ہندسہ_Type) {
                حافظہ_نقل(&o->constants[const_جدول_اشاریہ], &i_لفظ, حجم(i_لفظ));
            } وإلا {
                u32 word32 = (u32)i_لفظ;
                حافظہ_نقل(&o->constants[const_جدول_اشاریہ], &word32, حجم(word32));
            }

_           (ڈالو(o, i_type, تھاں));

            o->تھاں_Max_Const_اشاریہ = M3_MAX(تھاں + عدد_Required_Slots, o->تھاں_Max_Const_اشاریہ);
        }
    }

    _catch: أرجع نتیجہ;
}

ساكن مضمن
M3_نتیجہ Emit_تھاں_عدد_دا_ڈھیر_چوٹی_And_نکالو (IM3Compilation o)
{
    // no emit if value is in register
    إذا (Is_ڈھیر_چوٹی_وچ_تھاں(o)) {
        Emit_تھاں_وتھ(o, Get_ڈھیر_چوٹی_تھاں_نمبر(o));
    }

    أرجع نکالو(o);
}


// Or, maybe: EmitTrappingOp
M3_نتیجہ جوڑو_Trap_ریکارڈ (IM3Compilation o)
{
    M3_نتیجہ نتیجہ = m3Err_none;

    إذا (o->function) {
    }

    أرجع نتیجہ;
}

ساكن
M3_نتیجہ Unwind_بلاک_ڈھیر (IM3Compilation o)
{
    M3_نتیجہ نتیجہ = m3Err_none;

    u32 نکالو_گنتی = 0;
    طالما (o->ڈھیر_اشاریہ > o->بلاک.بلاک_ڈھیر_اشاریہ) {
_       (نکالو(o));
        ++نکالو_گنتی;
    }

    إذا (نکالو_گنتی) {
        m3log(compile, "unwound stack top: %d", نکالو_گنتی);
    }

    _catch: أرجع نتیجہ;
}

ساكن مضمن
M3_نتیجہ سیٹ_ڈھیر_Polymorphic (IM3Compilation o)
{
    o->بلاک.isPolymorphic = true;                              m3log (compile, "stack set polymorphic");
    أرجع Unwind_بلاک_ڈھیر(o);
}

ساكن
فراغ PatchBranches (IM3Compilation o)
{
    InvalidateFold(o);         // patched branches land at the current pc

    pc_t pc = GetPC(o);

    pc_t patches     = o->بلاک.patches;
    o->بلاک.patches = عدم;

    طالما (patches) {                                           m3log (compile, "patching location: %p to pc: %p", patches, pc);
        pc_t اگلا       = *(pc_t*)patches;
        *(pc_t*)patches = pc;
        patches         = اگلا;
    }
}

//-------------------------------------------------------------------------------------------------------------------------

ساكن
M3_نتیجہ Copy_ڈھیر_اشاریہ_منزل_تھاں (IM3Compilation o, u16 i_dest_تھاں, u16 i_ڈھیر_اشاریہ)  // NoPushPop
{
    M3_نتیجہ نتیجہ = m3Err_none;

    IM3Operation op;

    m3type_t type       = Get_ڈھیر_Type_From_Bottom(o, i_ڈھیر_اشاریہ);
    bool     وچ_رجسٹر = Is_ڈھیر_اشاریہ_وچ_رجسٹر(o, i_ڈھیر_اشاریہ);

    إذا (وچ_رجسٹر) {
        op = c_سیٹ_سیٹ_Ops[ادھار_Type_دا(type)];
    } وإلا {
        op = Is64_ثنائی_ہندسہ_Type(type) ? op_Copy_تھاں_64 : op_Copy_تھاں_32;
    }

_   (EmitOp(o, op));
    Emit_تھاں_وتھ(o, i_dest_تھاں);

    إذا (not وچ_رجسٹر) {
        u16 src_تھاں = Get_تھاں_For_ڈھیر_اشاریہ(o, i_ڈھیر_اشاریہ);
        Emit_تھاں_وتھ(o, src_تھاں);
    }

    _catch: أرجع نتیجہ;
}

ساكن
M3_نتیجہ Copy_ڈھیر_چوٹی_منزل_تھاں (IM3Compilation o, u16 i_dest_تھاں)  // NoPushPop
{
    M3_نتیجہ نتیجہ;

    i16 ڈھیر_چوٹی;
_   (Get_ڈھیر_چوٹی_اشاریہ_Throws(o, &ڈھیر_چوٹی));
_   (Copy_ڈھیر_اشاریہ_منزل_تھاں(o, i_dest_تھاں, (u16)ڈھیر_چوٹی));

    _catch: أرجع نتیجہ;
}


// a copy-on-write strategy is used with locals. when a get local occurs, it's not copied anywhere. the stack
// entry just has a index pointer to that local memory slot.
// then, when a previously referenced local is set, the current value needs to be preserved for those references

// TODO: consider getting rid of these specialized operations: PreserveSetSlot & PreserveCopySlot.
// They likely just take up space (which seems to reduce performance) without improving performance.
// => unconfirmed in preliminary testing.
ساكن
M3_نتیجہ Preserved_Copy_چوٹی_تھاں (IM3Compilation o, u16 i_dest_تھاں, u16 i_preserve_تھاں)
{
    M3_نتیجہ نتیجہ = m3Err_none;             d_m3Assert (i_dest_تھاں != i_preserve_تھاں);

    IM3Operation op;

    m3type_t type = Get_ڈھیر_چوٹی_Type(o);

    إذا (Is_ڈھیر_چوٹی_وچ_رجسٹر(o)) {
        op = c_preserve_سیٹ_تھاں[ادھار_Type_دا(type)];
    } وإلا {
        op = Is64_ثنائی_ہندسہ_Type(type) ? op_Preserve_Copy_تھاں_64 : op_Preserve_Copy_تھاں_32;
    }

_   (EmitOp(o, op));
    Emit_تھاں_وتھ(o, i_dest_تھاں);

    إذا (Is_ڈھیر_چوٹی_وچ_تھاں(o)) {
        Emit_تھاں_وتھ(o, Get_ڈھیر_چوٹی_تھاں_نمبر(o));
    }

    Emit_تھاں_وتھ(o, i_preserve_تھاں);

    _catch: أرجع نتیجہ;
}

ساكن
M3_نتیجہ Copy_ڈھیر_چوٹی_منزل_رجسٹر (IM3Compilation o, bool i_update_ڈھیر)
{
    M3_نتیجہ نتیجہ = m3Err_none;

    إذا (Is_ڈھیر_چوٹی_وچ_تھاں(o)) {
        m3type_t type = Get_ڈھیر_چوٹی_Type(o);

_       (Preserve_رجسٹر_If_Occupied(o, type));

        IM3Operation op = c_سیٹ_رجسٹر_Ops[ادھار_Type_دا(type)];

_       (EmitOp(o, op));
        Emit_تھاں_وتھ(o, Get_ڈھیر_چوٹی_تھاں_نمبر(o));

        إذا (i_update_ڈھیر) {
_           (نکالو_Type(o, type));
_           (ڈالو_رجسٹر(o, type));
        }
    }

    _catch: أرجع نتیجہ;
}


// if local is unreferenced, o_preservedSlotNumber will be equal to localIndex on return
ساكن
M3_نتیجہ لبھو_Referenced_Local_Within_Current_بلاک (IM3Compilation o, u16* o_preserved_تھاں_نمبر, u32 i_local_تھاں)
{
    M3_نتیجہ نتیجہ = m3Err_none;

    IM3CompilationScope scope = &o->بلاک;

    u16 شروع_اشاریہ = scope->بلاک_ڈھیر_اشاریہ;

    طالما (scope->opcode == c_wa_Op_بلاک) {
        scope = scope->outer;
        إذا (not scope) {
            اكسر;
        }

        شروع_اشاریہ = scope->بلاک_ڈھیر_اشاریہ;
    }

    *o_preserved_تھاں_نمبر = (u16)i_local_تھاں;

    لكل (u32 i = شروع_اشاریہ; i < o->ڈھیر_اشاریہ; ++i) {
        إذا (o->wasm_ڈھیر[i] == i_local_تھاں) {
            m3type_t type = Get_ڈھیر_Type_From_Bottom(o, i);                         d_m3Assert (type != c_m3Type_none)

            إذا (*o_preserved_تھاں_نمبر == i_local_تھاں) {
_               (AllocateSlots(o, o_preserved_تھاں_نمبر, type));
            } وإلا {
                // A 64-bit value spans two slots, and allocating or releasing it
                // counts both, so a second reference has to count both as well.
                لكل (u16 s = 0; s < Get_Type_عدد_Slots(type); ++s) {
_                   (Increment_تھاں_Usage_گنتی(o, (u16)(*o_preserved_تھاں_نمبر + s)));
                }
            }

            o->wasm_ڈھیر[i] = *o_preserved_تھاں_نمبر;
        }
    }

    _catch: أرجع نتیجہ;
}

ساكن
M3_نتیجہ Get_بلاک_Scope (IM3Compilation o, IM3CompilationScope* o_scope, u32 i_ڈونگھائی)
{
    M3_نتیجہ نتیجہ = m3Err_none;

    IM3CompilationScope scope = &o->بلاک;

    طالما (i_ڈونگھائی--) {
        scope = scope->outer;
        _throwif("invalid block depth", not scope);
    }

    *o_scope = scope;

    _catch:
    أرجع نتیجہ;
}

ساكن
M3_نتیجہ Copy_ڈھیر_Slots_R (IM3Compilation o, u16 i_نشانہ_تھاں_ڈھیر_اشاریہ, u16 i_ڈھیر_اشاریہ, u16 i_سرا_ڈھیر_اشاریہ, u16 i_عارضی_تھاں)
{
    M3_نتیجہ نتیجہ = m3Err_none;

    إذا (i_ڈھیر_اشاریہ < i_سرا_ڈھیر_اشاریہ) {
        u16 src_تھاں = Get_تھاں_For_ڈھیر_اشاریہ(o, i_ڈھیر_اشاریہ);

        m3type_t type      = Get_ڈھیر_Type_From_Bottom(o, i_ڈھیر_اشاریہ);
        u16      عدد_Slots  = Get_Type_عدد_Slots(type);
        u16      extra_تھاں = عدد_Slots - 1;

        u16 نشانہ_تھاں = Get_تھاں_For_ڈھیر_اشاریہ(o, i_نشانہ_تھاں_ڈھیر_اشاریہ);

        u16 preserve_اشاریہ = i_ڈھیر_اشاریہ;
        u16 collision_تھاں = src_تھاں;

        إذا (نشانہ_تھاں != src_تھاں) {
            // search for collisions
            u16 check_اشاریہ = i_ڈھیر_اشاریہ + 1;
            طالما (check_اشاریہ < i_سرا_ڈھیر_اشاریہ) {
                u16 otherSlot1 = Get_تھاں_For_ڈھیر_اشاریہ(o, check_اشاریہ);
                u16 otherSlot2 = Get_Extra_تھاں_For_ڈھیر_اشاریہ(o, check_اشاریہ);

                إذا (نشانہ_تھاں == otherSlot1 or
                    نشانہ_تھاں == otherSlot2 or
                    نشانہ_تھاں + extra_تھاں == otherSlot1) {
                    _throwif(m3_Err_function_ڈھیر_حد_توں_ودھ, i_عارضی_تھاں >= d_m3MaxFunctionSlots);

_                   (Copy_ڈھیر_اشاریہ_منزل_تھاں(o, i_عارضی_تھاں, check_اشاریہ));
                    o->wasm_ڈھیر[check_اشاریہ] = i_عارضی_تھاں;
                    i_عارضی_تھاں += Get_Type_عدد_Slots(c_m3Type_i64);
                    Touch_تھاں(o, i_عارضی_تھاں - 1);

                    // restore this on the way back down
                    preserve_اشاریہ = check_اشاریہ;
                    collision_تھاں = otherSlot1;

                    اكسر;
                }

                ++check_اشاریہ;
            }

_           (Copy_ڈھیر_اشاریہ_منزل_تھاں(o, نشانہ_تھاں, i_ڈھیر_اشاریہ));                                                m3log (compile, " copying slot: %d to slot: %d", src_تھاں, نشانہ_تھاں);
            o->wasm_ڈھیر[i_ڈھیر_اشاریہ] = نشانہ_تھاں;
        }

_       (Copy_ڈھیر_Slots_R(o, i_نشانہ_تھاں_ڈھیر_اشاریہ + 1, i_ڈھیر_اشاریہ + 1, i_سرا_ڈھیر_اشاریہ, i_عارضی_تھاں));

        // restore the stack state
        o->wasm_ڈھیر[i_ڈھیر_اشاریہ]  = src_تھاں;
        o->wasm_ڈھیر[preserve_اشاریہ] = collision_تھاں;
    }

    _catch:
    أرجع نتیجہ;
}

ساكن
M3_نتیجہ Resolve_بلاک_Results (IM3Compilation o, IM3CompilationScope i_نشانہ_بلاک, bool i_isBranch)
{
    M3_نتیجہ نتیجہ = m3Err_none;
    إذا (d_m3_Log_Wasm_ڈھیر) {
        dump_type_ڈھیر(o);
    }

    bool isLoop = (i_نشانہ_بلاک->opcode == c_waOp_loop and i_isBranch);

    u16 عدد_Params  = Get_Func_Type_عدد_Params(i_نشانہ_بلاک->type);
    u16 عدد_Results = Get_Func_Type_عدد_Results(i_نشانہ_بلاک->type);

    u16 تھاں_ریکارڈاں = i_نشانہ_بلاک->exit_ڈھیر_اشاریہ;

    u16 عدد_اقدار;

    إذا (not isLoop) {
        عدد_اقدار = عدد_Results;
        تھاں_ریکارڈاں += عدد_Params;
    } وإلا {
        عدد_اقدار = عدد_Params;
    }

    u16 بلاک_اچائی = Get_عدد_بلاک_اقدار_On_ڈھیر(o);

    _throwif(m3_Err_type_گنتی_Mismatch, i_isBranch ? (بلاک_اچائی < عدد_اقدار) : (بلاک_اچائی != عدد_اقدار));

    إذا (عدد_اقدار) {
        u16 سرا_اشاریہ     = Get_ڈھیر_چوٹی_اشاریہ(o) + 1;
        u16 عدد_Rem_اقدار = عدد_اقدار;

        // The last result is taken from _fp0. See PushBlockResults.
        إذا (not isLoop and IsFpType(Get_ڈھیر_چوٹی_Type(o))) {
_           (Copy_ڈھیر_چوٹی_منزل_رجسٹر(o, false));
            --سرا_اشاریہ;
            --عدد_Rem_اقدار;
        }

        // tempslot affects maxStackSlots, so can grow unnecess each time.
        // in practice, this is hardly ever hit.
        u16 عارضی_تھاں = o->max_ڈھیر_Slots;// GetMaxUsedSlotPlusOne (o); doesn't work cause can collide with slotRecords
        Align_تھاں_منزل_Type(&عارضی_تھاں, c_m3Type_i64);

_       (Copy_ڈھیر_Slots_R(o, تھاں_ریکارڈاں, سرا_اشاریہ - عدد_Rem_اقدار, سرا_اشاریہ, عارضی_تھاں));

        إذا (d_m3_Log_Wasm_ڈھیر) {
            dump_type_ڈھیر(o);
        }
    }

    _catch: أرجع نتیجہ;
}


ساكن
M3_نتیجہ Return_اقدار (IM3Compilation o, IM3CompilationScope i_function_بلاک, bool i_isBranch)
{
    M3_نتیجہ نتیجہ = m3Err_none;
    إذا (d_m3_Log_Wasm_ڈھیر) {
        dump_type_ڈھیر(o);
    }

    u16 عدد_Returns  = Get_Func_Type_عدد_Results(i_function_بلاک->type);     // could just o->function too...
    u16 بلاک_اچائی = Get_عدد_بلاک_اقدار_On_ڈھیر(o);

    إذا (not Is_ڈھیر_Polymorphic(o)) {
        _throwif(m3_Err_type_گنتی_Mismatch, i_isBranch ? (بلاک_اچائی < عدد_Returns) : (بلاک_اچائی != عدد_Returns));
    }

    إذا (عدد_Returns) {
        // return slots like args are 64-bit aligned
        u16 return_تھاں = عدد_Returns * c_io_تھاں_گنتی;
        u16 ڈھیر_چوٹی   = Get_ڈھیر_چوٹی_اشاریہ(o);

        لكل (u16 i = 0; i < عدد_Returns; ++i) {
            m3type_t returnType = Get_Func_Type_نتیجہ_Type(i_function_بلاک->type, عدد_Returns - 1 - i);

            m3type_t ڈھیر_Type = Get_ڈھیر_Type_From_چوٹی(o, i);  // using FromTop so that only dynamic items are checked

            إذا (Is_ڈھیر_Polymorphic(o) and ڈھیر_Type == c_m3Type_none) {
                ڈھیر_Type = returnType;
            }

            _throwif(m3Err_typeMismatch, not Is_Sub_Type_دا(ڈھیر_Type, returnType));

            إذا (not Is_ڈھیر_Polymorphic(o)) {
                return_تھاں -= c_io_تھاں_گنتی;
_               (Copy_ڈھیر_اشاریہ_منزل_تھاں(o, return_تھاں, ڈھیر_چوٹی--));
            }
        }

        إذا (not i_isBranch) {
            طالما (عدد_Returns--) {
_               (نکالو(o));
            }
        }
    }

    _catch: أرجع نتیجہ;
}


//-------------------------------------------------------------------------------------------------------------------------

ساكن
M3_نتیجہ Compile_Const_i32 (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_نتیجہ نتیجہ;

    i32 قدر;
_   (ReadLEB_i32(&قدر, &o->wasm, o->wasm_سرا));
_   (ڈالو_Const(o, قدر, c_m3Type_i32));                        m3log (compile, d_indent " (const i32 = %" PRIi32 ")", get_indention_string (o), قدر);
    _catch: أرجع نتیجہ;
}

ساكن
M3_نتیجہ Compile_Const_i64 (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_نتیجہ نتیجہ;

    i64 قدر;
_   (ReadLEB_i64(&قدر, &o->wasm, o->wasm_سرا));
_   (ڈالو_Const(o, قدر, c_m3Type_i64));                        m3log (compile, d_indent " (const i64 = %" PRIi64 ")", get_indention_string (o), قدر);
    _catch: أرجع نتیجہ;
}


#إذا d_m3ImplementFloat
ساكن
M3_نتیجہ Compile_Const_f32 (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_نتیجہ نتیجہ;

    اتحاد {
        u32 u;
        f32 f;
    } قدر = { 0 };

_   (Read_f32(&قدر.f, &o->wasm, o->wasm_سرا));                 m3log (compile, d_indent " (const f32 = %" PRIf32 ")", get_indention_string (o), قدر.f);
_   (ڈالو_Const(o, قدر.u, c_m3Type_f32));

    _catch: أرجع نتیجہ;
}

ساكن
M3_نتیجہ Compile_Const_f64 (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_نتیجہ نتیجہ;

    اتحاد {
        u64 u;
        f64 f;
    } قدر = { 0 };

_   (Read_f64(&قدر.f, &o->wasm, o->wasm_سرا));                 m3log (compile, d_indent " (const f64 = %" PRIf64 ")", get_indention_string (o), قدر.f);
_   (ڈالو_Const(o, قدر.u, c_m3Type_f64));

    _catch: أرجع نتیجہ;
}
#جے_انت

ساكن
M3_نتیجہ Compile_ExtendedOpcode (IM3Compilation o, m3opcode_t i_opcode)
{
_try {
    u32 opcode;
_   (ReadLEB_u32(&opcode, &o->wasm, o->wasm_سرا));               m3log (compile, d_indent " (%s: %" PRIu32 ")", get_indention_string (o), (i_opcode == c_waOp_atomic) ? "FE" : "FC", opcode);

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

    } _catch: أرجع نتیجہ;
}

ساكن
M3_نتیجہ Compile_Return (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_نتیجہ نتیجہ = m3Err_none;

    إذا (not Is_ڈھیر_Polymorphic(o)) {
        IM3CompilationScope functionScope;
_       (Get_بلاک_Scope(o, &functionScope, o->بلاک.ڈونگھائی));

_       (Return_اقدار(o, functionScope, true));

_       (EmitOp(o, op_Return));

_       (سیٹ_ڈھیر_Polymorphic(o));
    }

    _catch: أرجع نتیجہ;
}

ساكن
M3_نتیجہ Validate_بلاک_سرا (IM3Compilation o)
{
    M3_نتیجہ نتیجہ = m3Err_none;

    u16 عدد_Results  = Get_Func_Type_عدد_Results(o->بلاک.type);
    u16 بلاک_اچائی = Get_عدد_بلاک_اقدار_On_ڈھیر(o);

    إذا (not Is_ڈھیر_Polymorphic(o)) {
        // Spec: at block end, stack height must match the number of results
        _throwif(m3_Err_type_گنتی_Mismatch, بلاک_اچائی != عدد_Results);

        // Spec: result types must match expected types
        لكل (u16 i = 0; i < عدد_Results; ++i) {
            m3type_t expectedType = Get_Func_Type_نتیجہ_Type(o->بلاک.type, عدد_Results - 1 - i);
            m3type_t actualType   = Get_ڈھیر_Type_From_چوٹی(o, i);
            _throwif(m3Err_typeMismatch, not Is_Sub_Type_دا(actualType, expectedType));
        }
    }

    _catch: أرجع نتیجہ;
}

ساكن
M3_نتیجہ Compile_سرا (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_نتیجہ نتیجہ = m3Err_none;                   //dump_type_stack (o);

    // function end:
    إذا (o->بلاک.ڈونگھائی == 0) {
        Validate_بلاک_سرا(o);

        //      if (not IsStackPolymorphic (o))
        {
            // A constant expression has no function frame, but its value still
            // has to be copied down to slot 0, where EvaluateExpression reads it
            // back. The non-emitting init-expr walk is skipped: it has no block
            // type to return against.
            إذا (o->function or o->ورقہ) {
_               (Return_اقدار(o, &o->بلاک, false));
            }

_           (EmitOp(o, op_Return));
        }
    }

    _catch: أرجع نتیجہ;
}


ساكن
M3_نتیجہ Compile_سیٹ_Local (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_نتیجہ نتیجہ;

    u32 local_اشاریہ;
_   (ReadLEB_u32(&local_اشاریہ, &o->wasm, o->wasm_سرا));                           m3log (compile, d_indent " (index = %u)", get_indention_string (o), local_اشاریہ);

    إذا (local_اشاریہ < Get_Function_عدد_دلیلاں_And_Locals(o->function)) {
        // Spec: value type must match local type
        إذا (not Is_ڈھیر_Polymorphic(o)) {
            m3type_t localType    = Get_ڈھیر_Type_From_Bottom(o, local_اشاریہ);
            m3type_t ڈھیر_چوٹی_Type = Get_ڈھیر_چوٹی_Type(o);
            _throwif(m3Err_typeMismatch, ڈھیر_چوٹی_Type != c_m3Type_none and localType != c_m3Type_none and
                                           not Is_Sub_Type_دا(ڈھیر_چوٹی_Type, localType));
#إذا d_m3HasSnapshots
            إذا (o->local_Init_ڈونگھائی and o->local_Init_ڈونگھائی[local_اشاریہ] == UINT32_MAX) {
                o->local_Init_ڈونگھائی[local_اشاریہ] = (u32)o->بلاک.ڈونگھائی;
            }
#جے_انت
        }

        u16 local_تھاں = Get_تھاں_For_ڈھیر_اشاریہ(o, local_اشاریہ);

        u16 preserve_تھاں;
_       (لبھو_Referenced_Local_Within_Current_بلاک(o, &preserve_تھاں, local_تھاں));  // preserve will be different than local, if referenced

        bool folded = false;

#إذا d_m3_Fold_سیٹ_Local
        // destination folding: when the value was produced by the immediately preceding op (nothing
        // emitted since, result on top in _r0, no copy-on-write preservation needed), retro-patch the
        // producer to a variant that writes straight to the local's slot and skip the SetSlot op
        إذا (o->foldPatchPC and preserve_تھاں == local_تھاں and Is_ڈھیر_چوٹی_وچ_رجسٹر(o) and Get_ڈھیر_چوٹی_اشاریہ(o) == (i16)o->fold_ڈھیر_اشاریہ) {
            IM3Operation foldOp = GetFoldOp(o->foldOpcode, o->foldForm);

            إذا (foldOp) {
                *(IM3Operation*)o->foldPatchPC = foldOp;     // retro-patch the producer
                Emit_تھاں_وتھ(o, local_تھاں);                  // append its destination
                InvalidateFold(o);
                folded = true;
            }
        }
#جے_انت

        إذا (folded) {
            m3type_t type = Get_ڈھیر_چوٹی_Type(o);
_           (نکالو(o));                              // the value now lives in the local's slot

            // A folded op writes its result through to the register as well, so a tee
            // can keep it there. Pushing the slot instead would make the next use that
            // wants a register reload the slot this op just stored to.
            إذا (i_opcode == c_waOp_teeLocal) {
_               (ڈالو_رجسٹر(o, type));
            }
        } وإلا {
            إذا (preserve_تھاں == local_تھاں) {
_               (Copy_ڈھیر_چوٹی_منزل_تھاں(o, local_تھاں))
            } وإلا {
_               (Preserved_Copy_چوٹی_تھاں(o, local_تھاں, preserve_تھاں))
            }

            إذا (i_opcode != c_waOp_teeLocal) {
_               (نکالو(o));
            }
        }
    } وإلا {
        _throw("local index out of bounds");
    }

    _catch: أرجع نتیجہ;
}

ساكن
M3_نتیجہ Compile_GetLocal (IM3Compilation o, m3opcode_t i_opcode)
{
_try {

    u32 local_اشاریہ;
_   (ReadLEB_u32(&local_اشاریہ, &o->wasm, o->wasm_سرا));                           m3log (compile, d_indent " (index = %u)", get_indention_string (o), local_اشاریہ);

    إذا (local_اشاریہ >= Get_Function_عدد_دلیلاں_And_Locals(o->function)) {
        _throw("local index out of bounds");
    }

    m3type_t type = Get_ڈھیر_Type_From_Bottom(o, local_اشاریہ);
    u16      تھاں = Get_تھاں_For_ڈھیر_اشاریہ(o, local_اشاریہ);

_   (ڈالو(o, type, تھاں));

    } _catch: أرجع نتیجہ;
}

ساكن
M3_نتیجہ Compile_GetGlobal (IM3Compilation o, M3Global* i_global)
{
    M3_نتیجہ نتیجہ;

    IM3Operation op = Is64_ثنائی_ہندسہ_Type(i_global->type) ? op_GetGlobal_s64 : op_GetGlobal_s32;
_   (EmitOp(o, op));
    Emit_اشارہ_گر(o, &i_global->i64_قدر);
_   (ڈالو_Allocated_تھاں_And_Emit_2(o, i_global->type));

    _catch: أرجع نتیجہ;
}

ساكن
M3_نتیجہ Compile_سیٹ_Global (IM3Compilation o, M3Global* i_global)
{
    M3_نتیجہ نتیجہ = m3Err_none;

    إذا (i_global->isMutable) {
        // Spec: value type must match global type
        إذا (not Is_ڈھیر_Polymorphic(o)) {
            m3type_t ڈھیر_چوٹی_Type = Get_ڈھیر_چوٹی_Type(o);
            _throwif(m3Err_typeMismatch, ڈھیر_چوٹی_Type != c_m3Type_none and
                                           not Is_Sub_Type_دا(ڈھیر_چوٹی_Type, i_global->type));
        }

        IM3Operation op;
        m3type_t     type = Get_ڈھیر_چوٹی_Type(o);

        إذا (Is_ڈھیر_چوٹی_وچ_رجسٹر(o)) {
            op = c_سیٹ_Global_Ops[ادھار_Type_دا(type)];
        } وإلا {
            op = Is64_ثنائی_ہندسہ_Type(type) ? op_سیٹ_Global_s64 : op_سیٹ_Global_s32;
        }

_       (EmitOp(o, op));
        Emit_اشارہ_گر(o, &i_global->i64_قدر);

        إذا (Is_ڈھیر_چوٹی_وچ_تھاں(o)) {
            Emit_تھاں_وتھ(o, Get_ڈھیر_چوٹی_تھاں_نمبر(o));
        }

_       (نکالو(o));
    } وإلا {
        _throw(m3Err_settingImmutableGlobal);
    }

    _catch: أرجع نتیجہ;
}

ساكن
M3_نتیجہ Compile_Get_سیٹ_Global (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_نتیجہ نتیجہ = m3Err_none;

    u32 global_اشاریہ;
_   (ReadLEB_u32(&global_اشاریہ, &o->wasm, o->wasm_سرا));

    إذا (global_اشاریہ < o->module->عدد_Globals) {
        إذا (o->module->globals) {
            M3Global* global = &o->module->globals[global_اشاریہ];

            // Spec: a constant expression may only read an imported immutable
            // global. The module's own globals are counted before their
            // initializer is walked, so a bare index check would let one
            // reference itself. These describe how the module declared the
            // global, so they are checked before following any link.
            _throwif(m3_Err_globa_اشاریہ_باہرلا_دا_Bounds, o->is_Init_اظہاریہ and not global->imported);
            _throwif(m3Err_wasmMalformed, o->is_Init_اظہاریہ and global->isMutable);

            // an import linked to another module reads and writes that module's
            // cell, not the placeholder standing in for it here
            إذا (global->resolved) {
                global = global->resolved;
            }

_           ((i_opcode == c_waOp_getGlobal) ? Compile_GetGlobal(o, global) : Compile_سیٹ_Global(o, global));
        } وإلا {
            _throw(ErrorCompile(m3_Err_global_حافظہ_Not_Allocated, o, "module '%s' is missing global memory", o->module->name));
        }
    } وإلا {
        _throw(m3_Err_globa_اشاریہ_باہرلا_دا_Bounds);
    }

    _catch: أرجع نتیجہ;
}

ساكن
فراغ Emit_Patching_Branch_اشارہ_گر (IM3Compilation o, IM3CompilationScope i_scope)
{
    pc_t patch       = Emit_اشارہ_گر(o, i_scope->patches);                m3log (compile, "branch patch required at: %p", patch);
    i_scope->patches = patch;
}

ساكن
M3_نتیجہ EmitPatchingBranch (IM3Compilation o, IM3CompilationScope i_scope)
{
    M3_نتیجہ نتیجہ = m3Err_none;

_   (EmitOp(o, op_Branch));
    Emit_Patching_Branch_اشارہ_گر(o, i_scope);

    _catch: أرجع نتیجہ;
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
u32 گنتی_Try_چوکھٹے_منزل_Exit (IM3CompilationScope i_from, IM3CompilationScope i_نشانہ)
{
    u32 گنتی = 0;

    لكل (IM3CompilationScope scope = i_from; scope; scope = scope->outer) {
        إذا (scope->opcode == c_wa_Op_try_جدول) {
            ++گنتی;
        }

        إذا (scope == i_نشانہ) {
            اكسر;
        }
    }

    أرجع گنتی;
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
u32 عدد_Try_چوکھٹے_منزل_نکالو (IM3CompilationScope i_from, IM3CompilationScope i_نشانہ)
{
    إذا (i_نشانہ->ڈونگھائی == 0) {
        أرجع 0;
    }

    // the walk only ever runs inward-to-outward; a target that is already
    // outside i_from has nothing between them
    إذا (not i_from or i_from->ڈونگھائی < i_نشانہ->ڈونگھائی) {
        أرجع 0;
    }

    أرجع گنتی_Try_چوکھٹے_منزل_Exit(i_from, i_نشانہ);
}


ساكن
M3_نتیجہ Emit_نکالو_Try_چوکھٹے (IM3Compilation o, u32 i_عدد_چوکھٹے)
{
    M3_نتیجہ نتیجہ = m3Err_none;

    إذا (i_عدد_چوکھٹے) {
_       (EmitOp(o, op_نکالو_Handlers));
        EmitConstant32(o, i_عدد_چوکھٹے);
    }

    _catch: أرجع نتیجہ;
}


// A return_call leaves the enclosing function before the callee runs, so no
// try_table in this function may catch what the callee throws. Neither shape of
// the instruction unwinds the native stack in time to arrange that on its own:
// op_ReturnCall tail-jumps with op_TryTable's frames still standing, and the
// fallback shape is an ordinary call sitting inside them. Either way the
// handler records have to come off first.
ساكن
M3_نتیجہ Emit_نکالو_Try_چوکھٹے_For_Return_Call (IM3Compilation o)
{
    u32 گنتی = 0;

    لكل (IM3CompilationScope scope = &o->بلاک; scope; scope = scope->outer) {
        إذا (scope->opcode == c_wa_Op_try_جدول) {
            ++گنتی;
        }
    }

    أرجع Emit_نکالو_Try_چوکھٹے(o, گنتی);
}

#وإلا

ساكن
M3_نتیجہ Emit_نکالو_Try_چوکھٹے (IM3Compilation o, u32 i_عدد_چوکھٹے)
{
    (فراغ)o;
    (فراغ)i_عدد_چوکھٹے;
    أرجع m3Err_none;
}

#جے_انت // d_m3HasExceptionHandling

#إذا d_m3FuseBranch
// when the pending fold candidate is a compare whose result sits on top in _r0, return the fused
// variant that absorbs an immediately following br_if (i_isIf false) or if (i_isIf true)
ساكن
IM3Operation GetBranchFusionOp (IM3Compilation o, bool i_isIf)
{
    إذا (o->foldPatchPC and o->foldForm < 4 and Is_ڈھیر_چوٹی_وچ_رجسٹر(o) and Get_ڈھیر_چوٹی_اشاریہ(o) == (i16)o->fold_ڈھیر_اشاریہ) {
        ثابت M3FusedCmpOps* ops = GetFusedCmpOps(o->foldOpcode);

        إذا (ops) {
            أرجع i_isIf ? ops->ifOp[o->foldForm] : ops->branchIf[o->foldForm];
        }
    }

    أرجع عدم;
}
#جے_انت // d_m3FuseBranch

ساكن مضمن
IM3Operation Op_ContinueLoop (IM3Compilation o)
{
#إذا d_m3_Has_ڈھیر_Switching
    إذا (M3_UNLIKELY(o->module->runtime and o->module->runtime->isSuspendable)) {
        أرجع op_ContinueLoop_Suspendable;
    }
#جے_انت
    أرجع op_ContinueLoop;
}

ساكن مضمن
IM3Operation Op_ContinueLoopIf (IM3Compilation o)
{
#إذا d_m3_Has_ڈھیر_Switching
    إذا (M3_UNLIKELY(o->module->runtime and o->module->runtime->isSuspendable)) {
        أرجع op_ContinueLoopIf_Suspendable;
    }
#جے_انت
    أرجع op_ContinueLoopIf;
}


ساكن
M3_نتیجہ Compile_Branch (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_نتیجہ نتیجہ;
    u32      عدد_Try_چوکھٹے = 0;

    u32 ڈونگھائی;
_   (ReadLEB_u32(&ڈونگھائی, &o->wasm, o->wasm_سرا));

    // Spec: br_if condition must be i32
    إذا (i_opcode == c_waOp_branchIf and not Is_ڈھیر_Polymorphic(o)) {
        m3type_t condType = Get_ڈھیر_چوٹی_Type(o);
        _throwif(m3Err_typeMismatch, condType != c_m3Type_none and condType != c_m3Type_i32);
    }

    IM3CompilationScope scope;
_   (Get_بلاک_Scope(o, &scope, ڈونگھائی));

#إذا d_m3HasExceptionHandling
    عدد_Try_چوکھٹے = عدد_Try_چوکھٹے_منزل_نکالو(&o->بلاک, scope);
#جے_انت

    // branch target is a loop (continue)
    إذا (scope->opcode == c_waOp_loop) {
        إذا (i_opcode == c_waOp_branchIf) {
            // a handler pop belongs on the taken path only, so it forces the
            // jump-over form even where the target takes no parameters
            إذا (Get_Func_Type_عدد_Params(scope->type) or عدد_Try_چوکھٹے) {
                IM3Operation op = Is_ڈھیر_چوٹی_وچ_رجسٹر(o) ? op_BranchIfPrologue_r : op_BranchIfPrologue_s;

_               (EmitOp(o, op));
_               (Emit_تھاں_عدد_دا_ڈھیر_چوٹی_And_نکالو(o));

                pc_t* jump_منزل = (pc_t*)Reserve_اشارہ_گر(o);

_               (Emit_نکالو_Try_چوکھٹے(o, عدد_Try_چوکھٹے));

                إذا (not Is_ڈھیر_Polymorphic(o)) {
_                   (Resolve_بلاک_Results(o, scope, /* isBranch: */ true));
                }

_               (EmitOp(o, Op_ContinueLoop(o)));
                Emit_اشارہ_گر(o, scope->pc);
_               (ریکارڈ_پٹھ_کنارا(o, scope, false));

                *jump_منزل = GetPC(o);
            } وإلا {
                // move the condition to a register
_               (Copy_ڈھیر_چوٹی_منزل_رجسٹر(o, false));
_               (نکالو_Type(o, c_m3Type_i32));

_               (EmitOp(o, Op_ContinueLoopIf(o)));
                Emit_اشارہ_گر(o, scope->pc);
_               (ریکارڈ_پٹھ_کنارا(o, scope, true));
            }

            //          dump_type_stack(o);
        } وإلا // is c_waOp_branch
        {
_           (Emit_نکالو_Try_چوکھٹے(o, عدد_Try_چوکھٹے));

            // the branch operands become the loop's parameters on the next iteration
            إذا (not Is_ڈھیر_Polymorphic(o)) {
_               (Resolve_بلاک_Results(o, scope, /* isBranch: */ true));
            }

_           (EmitOp(o, Op_ContinueLoop(o)));
            Emit_اشارہ_گر(o, scope->pc);
_           (ریکارڈ_پٹھ_کنارا(o, scope, false));
            o->بلاک.isPolymorphic = true;
        }
    } وإلا // forward branch
    {
        pc_t* jump_منزل = عدم;

        bool isReturn         = (scope->ڈونگھائی == 0);
        bool نشانہ_Has_Results = Get_Func_Type_عدد_Results(scope->type);

        إذا (i_opcode == c_waOp_branchIf) {
            إذا (نشانہ_Has_Results or isReturn or عدد_Try_چوکھٹے) {
                IM3Operation op = Is_ڈھیر_چوٹی_وچ_رجسٹر(o) ? op_BranchIfPrologue_r : op_BranchIfPrologue_s;

_               (EmitOp(o, op));
_               (Emit_تھاں_عدد_دا_ڈھیر_چوٹی_And_نکالو(o)); // condition

                // this is continuation point, if the branch isn't taken
                jump_منزل = (pc_t*)Reserve_اشارہ_گر(o);
            } وإلا {
#إذا d_m3FuseBranch
                IM3Operation fuseOp = GetBranchFusionOp(o, false);

                إذا (fuseOp) {
                    // absorb the branch into the compare that produced the condition
                    *(IM3Operation*)o->foldPatchPC = fuseOp;
                    InvalidateFold(o);

_                   (نکالو(o));                      // the condition is consumed inside the fused op

                    Emit_Patching_Branch_اشارہ_گر(o, scope);
                    اذهب _catch;
                }
#جے_انت
                IM3Operation op = Is_ڈھیر_چوٹی_وچ_رجسٹر(o) ? op_BranchIf_r : op_BranchIf_s;

_               (EmitOp(o, op));
_               (Emit_تھاں_عدد_دا_ڈھیر_چوٹی_And_نکالو(o)); // condition

                Emit_Patching_Branch_اشارہ_گر(o, scope);
                اذهب _catch;
            }
        }

        إذا (not Is_ڈھیر_Polymorphic(o)) {
            إذا (isReturn) {
_               (Return_اقدار(o, scope, true));
_               (EmitOp(o, op_Return));
            } وإلا {
_               (Emit_نکالو_Try_چوکھٹے(o, عدد_Try_چوکھٹے));

_               (Resolve_بلاک_Results(o, scope, true));
_               (EmitPatchingBranch(o, scope));
            }
        }

        إذا (jump_منزل) {
            *jump_منزل = GetPC(o);
        }

        إذا (i_opcode == c_waOp_branch) {
_           (سیٹ_ڈھیر_Polymorphic(o));
        }
    }

    _catch: أرجع نتیجہ;
}

ساكن
M3_نتیجہ Compile_Branch_جدول (IM3Compilation o, m3opcode_t i_opcode)
{
    // the page a continue-op page is standing in for; non-NULL only while that
    // page is installed in o->page, so the catch below knows to put it back
    IM3_کوڈ_ورقہ displaced_ورقہ = عدم;
    // indexed by label depth: the continuation stub already emitted for that target
    // within this table, or NULL if this table has not reached it yet
    pc_t* نشانہ_Stubs = عدم;
_try {
    u32 نشانہ_گنتی;
_   (ReadLEB_u32(&نشانہ_گنتی, &o->wasm, o->wasm_سرا));

    // Spec: validate that the branch index operand is i32
    إذا (not Is_ڈھیر_Polymorphic(o)) {
        m3type_t اشاریہ_Type = Get_ڈھیر_چوٹی_Type(o);
        _throwif(m3Err_typeMismatch, اشاریہ_Type != c_m3Type_none and اشاریہ_Type != c_m3Type_i32);
    }

_   (Preserve_رجسٹر_If_Occupied(o, c_m3Type_i64));         // move branch operand to a slot
    u16 تھاں = Get_ڈھیر_چوٹی_تھاں_نمبر(o);
_   (نکالو(o));

    // OPTZ: according to spec: "forward branches that target a control instruction with a non-empty
    // result type consume matching operands first and push them back on the operand stack after unwinding"
    // So, this move-to-reg is only necessary if the target scopes have a type.

    u32 عدد_کوڈ_سطریں = نشانہ_گنتی + 4; // 3 => IM3Operation + slot + target_count + default_target
_   (Ensure_کوڈ_ورقہ_عدد_سطریں(o, عدد_کوڈ_سطریں));

_   (EmitOp(o, op_Branch_جدول));
    Emit_تھاں_وتھ(o, تھاں);
    EmitConstant32(o, نشانہ_گنتی);

    IM3_کوڈ_ورقہ continue_Op_ورقہ = عدم;

    // The label types are checked by ValidateFunction, which runs under the same
    // d_m3EnableValidation guard just before this compilation pass. Comparing each
    // target against the default here would be wrong anyway: the spec matches every
    // target against the operand stack, so in unreachable code the operands are the
    // bottom type and the targets may legitimately differ (unreached-valid.wast).

    ++نشانہ_گنتی; // include default

    // Every entry of one br_table is resolved from the same compilation state - each
    // stub restores whatever it touched - so two entries naming the same label emit
    // byte-identical epilogues. Emit one and point both table entries at it. A label
    // deeper than the current block is rejected by GetBlockScope below, which is what
    // bounds this array.
    نشانہ_Stubs = m3_AllocArray(pc_t, (size_t)o->بلاک.ڈونگھائی + 1);
    _throwifnull(نشانہ_Stubs);

    لكل (u32 i = 0; i < نشانہ_گنتی; ++i) {
        u32 نشانہ;
_       (ReadLEB_u32(&نشانہ, &o->wasm, o->wasm_سرا));

        IM3CompilationScope scope;
_       (Get_بلاک_Scope(o, &scope, نشانہ));

        إذا (نشانہ_Stubs[نشانہ]) {
            Emit_اشارہ_گر(o, نشانہ_Stubs[نشانہ]);
            استمر;
        }

        u32 عدد_Try_چوکھٹے = 0;
#إذا d_m3HasExceptionHandling
        عدد_Try_چوکھٹے = عدد_Try_چوکھٹے_منزل_نکالو(&o->بلاک, scope);
#جے_انت

        // A forward branch to a block that carries no results and leaves no try
        // frames behind has nothing to do on the way out but arrive. There is no
        // epilogue to hold, so the table entry becomes the patch site itself and the
        // block's address is written into it when the block ends - no stub, and no
        // code page to put one on. In unreachable code the same is true whatever the
        // block's type, since nothing is emitted for that case either way.
        //
        // Each such entry is patched on its own, so none is recorded in targetStubs:
        // there is no shared stub for a later entry naming this label to point at.
        إذا (scope->opcode != c_waOp_loop and scope->ڈونگھائی > 0 and عدد_Try_چوکھٹے == 0 and
            (Is_ڈھیر_Polymorphic(o) or Get_Func_Type_عدد_Results(scope->type) == 0)) {
            Emit_Patching_Branch_اشارہ_گر(o, scope);
            استمر;
        }

_       (Acquire_Compilation_کوڈ_ورقہ(o, &continue_Op_ورقہ));

        pc_t شروع_PC  = Get_ورقہ_PC(continue_Op_ورقہ);
        displaced_ورقہ = o->ورقہ;
        o->ورقہ       = continue_Op_ورقہ;

        إذا (scope->opcode == c_waOp_loop) {
_           (Emit_نکالو_Try_چوکھٹے(o, عدد_Try_چوکھٹے));

            إذا (not Is_ڈھیر_Polymorphic(o)) {
_               (Resolve_بلاک_Results(o, scope, true));
            }

_           (EmitOp(o, Op_ContinueLoop(o)));
            Emit_اشارہ_گر(o, scope->pc);
_           (ریکارڈ_پٹھ_کنارا(o, scope, false));
        } وإلا {
            إذا (not Is_ڈھیر_Polymorphic(o)) {
                إذا (scope->ڈونگھائی == 0) {
_                   (Return_اقدار(o, scope, true));
_                   (EmitOp(o, op_Return));
                } وإلا {
_                   (Emit_نکالو_Try_چوکھٹے(o, عدد_Try_چوکھٹے));

_                   (Resolve_بلاک_Results(o, scope, true));

_                   (EmitPatchingBranch(o, scope));
                }
            }
        }

        IM3_کوڈ_ورقہ return_منزل = displaced_ورقہ;
        displaced_ورقہ        = عدم;

        Release_Compilation_کوڈ_ورقہ(o);
        o->ورقہ = return_منزل;

        Emit_اشارہ_گر(o, شروع_PC);
        نشانہ_Stubs[نشانہ] = شروع_PC;
    }

_   (سیٹ_ڈھیر_Polymorphic(o));
} _catch:

    // thrown out of the loop above with a continue-op page installed: release it
    // and restore the one it displaced, which the caller's catch then releases.
    // Otherwise the displaced page is on no list and simply leaks.
    إذا (displaced_ورقہ) {
        Release_Compilation_کوڈ_ورقہ(o);
        o->ورقہ = displaced_ورقہ;
    }

    m3_Free(نشانہ_Stubs);

    أرجع نتیجہ;
}

ساكن
M3_نتیجہ Compile_Call_دلیلاں_And_Return (IM3Compilation o, u16* o_ڈھیر_وتھ, IM3FuncType i_type, bool i_is_اسدھا)
{
_try {

    u16 چوٹی_تھاں = Get_Max_ورتیا_تھاں_Plus_One(o);

    // force use of at least one stack slot; this is to help ensure
    // the m3 stack overflows (and traps) before the native stack can overflow.
    // e.g. see Wasm spec test 'runaway' in call.wast
    چوٹی_تھاں = M3_MAX(1, چوٹی_تھاں);

    // stack frame is 64-bit aligned
    Align_تھاں_منزل_Type(&چوٹی_تھاں, c_m3Type_i64);

    *o_ڈھیر_وتھ = چوٹی_تھاں;

    // wait to pop this here so that topSlot search is correct
    إذا (i_is_اسدھا) {
_       (نکالو(o));
    }

    u16 عدد_دلیلاں = Get_Func_Type_عدد_Params(i_type);
    u16 عدد_Rets = Get_Func_Type_عدد_Results(i_type);

    u16 arg_چوٹی = چوٹی_تھاں + (عدد_دلیلاں + عدد_Rets) * c_io_تھاں_گنتی;

    Touch_تھاں(o, arg_چوٹی - 1);

    طالما (عدد_دلیلاں--) {
_       (Copy_ڈھیر_چوٹی_منزل_تھاں(o, arg_چوٹی -= c_io_تھاں_گنتی));
_       (نکالو(o));
    }

    u16 i = 0;
    طالما (عدد_Rets--) {
        m3type_t type = Get_Func_Type_نتیجہ_Type(i_type, i++);

_       (ڈالو(o, type, چوٹی_تھاں));
_       (MarkSlotsAllocatedByType(o, چوٹی_تھاں, type));

        چوٹی_تھاں += c_io_تھاں_گنتی;
    }

    } _catch: أرجع نتیجہ;
}

// A tail call hands the current frame over to the callee instead of stacking a new one.
// The callee's results have to be the enclosing function's results, so both use the same
// return slots and only the arguments move: they're staged above the caller's stack here
// (the argument expressions can still be reading the args/locals they're about to land on)
// and slid down onto the frame base at runtime, by op_ReturnCall[Indirect].
ساكن
M3_نتیجہ Compile_پوچھ_Call_دلیلاں (IM3Compilation o, u16* o_ڈھیر_وتھ, u16* o_عدد_Arg_Slots, IM3FuncType i_type, bool i_is_اسدھا)
{
_try {
    IM3FuncType funcType = o->function->funcType;

    u16 عدد_Rets = Get_Func_Type_عدد_Results(i_type);
    _throwif(m3Err_typeMismatch, عدد_Rets != Get_Func_Type_عدد_Results(funcType));

    لكل (u16 i = 0; i < عدد_Rets; ++i) {
        _throwif(m3Err_typeMismatch, Get_Func_Type_نتیجہ_Type(i_type, i) != Get_Func_Type_نتیجہ_Type(funcType, i));
    }

    u16 چوٹی_تھاں = Get_Max_ورتیا_تھاں_Plus_One(o);

    چوٹی_تھاں = M3_MAX(1, چوٹی_تھاں);

    // stack frame is 64-bit aligned
    Align_تھاں_منزل_Type(&چوٹی_تھاں, c_m3Type_i64);

    *o_ڈھیر_وتھ = چوٹی_تھاں;

    // wait to pop this here so that topSlot search is correct
    إذا (i_is_اسدھا) {
_       (نکالو(o));
    }

    u16 عدد_دلیلاں     = Get_Func_Type_عدد_Params(i_type);
    u16 عدد_Arg_Slots = عدد_دلیلاں * c_io_تھاں_گنتی;

    *o_عدد_Arg_Slots = عدد_Arg_Slots;

    u16 arg_چوٹی = چوٹی_تھاں + عدد_Arg_Slots;
    _throwif(m3_Err_function_ڈھیر_حد_توں_ودھ, arg_چوٹی > d_m3MaxFunctionSlots);

    // the staging area is written by the caller, so op_Entry has to account for it
    Touch_تھاں(o, arg_چوٹی - 1);

    طالما (عدد_دلیلاں--) {
_       (Copy_ڈھیر_چوٹی_منزل_تھاں(o, arg_چوٹی -= c_io_تھاں_گنتی));
_       (نکالو(o));
    }

    } _catch: أرجع نتیجہ;
}


#إذا d_m3HasTypedRefs

// call_ref $t: the callee is the reference on top of the stack, so the operand
// order is the same as call_indirect's, minus the table.
ساكن
M3_نتیجہ Compile_CallRef (IM3Compilation o, m3opcode_t i_opcode)
{
_try {
    u32 type_اشاریہ;
_   (ReadLEB_u32(&type_اشاریہ, &o->wasm, o->wasm_سرا));

    _throwif("function call type index out of range", type_اشاریہ >= o->module->عدد_Func_Types);

    IM3FuncType type = o->module->funcTypes[type_اشاریہ];

    إذا (not Is_ڈھیر_Polymorphic(o)) {
        // whatever shape the reference has, it must be a function of this type
        m3type_t refType = Get_ڈھیر_چوٹی_Type(o);
        _throwif(m3Err_typeMismatch, not Is_Sub_Type_دا(refType, Ref_Type_دا_Func_Type(type, false)));
    }

    إذا (Is_ڈھیر_چوٹی_وچ_رجسٹر(o)) {
_       (Preserve_رجسٹر_If_Occupied(o, c_m3Type_funcref));
    }

    u16 function_تھاں = Get_ڈھیر_چوٹی_تھاں_نمبر(o);

    bool isReturnCall = (i_opcode == c_waOp_returnCallRef);
    bool use_پوچھ_Call  = isReturnCall and d_m3_Can_پوچھ_Call;

    u16 exec_چوٹی, عدد_Arg_Slots = 0;

    إذا (use_پوچھ_Call) {
_       (Compile_پوچھ_Call_دلیلاں(o, &exec_چوٹی, &عدد_Arg_Slots, type, true));
    } وإلا {
_       (Compile_Call_دلیلاں_And_Return(o, &exec_چوٹی, type, true));
    }

#  إذا d_m3HasExceptionHandling
    إذا (isReturnCall) {
_       (Emit_نکالو_Try_چوکھٹے_For_Return_Call(o));
    }
#  endif

_   (EmitOp(o, use_پوچھ_Call ? op_ReturnCallRef : op_CallRef));
    Emit_تھاں_وتھ(o, function_تھاں);
    Emit_اشارہ_گر(o, type);
    Emit_تھاں_وتھ(o, exec_چوٹی);

    إذا (not use_پوچھ_Call) {
_       (ریکارڈ_Call(o, Get_Func_Type_عدد_Results(type), exec_چوٹی, isReturnCall));
    }

    إذا (use_پوچھ_Call) {
        Emit_تھاں_وتھ(o, o->function->عدد_Ret_Slots);
        EmitConstant32(o, عدد_Arg_Slots);

_       (سیٹ_ڈھیر_Polymorphic(o));
    } وإلا إذا (isReturnCall) {
_       (Compile_Return(o, i_opcode));
    }

} _catch:
    أرجع نتیجہ;
}


// ref.as_non_null only sharpens the type; the value is unchanged, so the stack
// entry is re-pushed with the null stripped out of its type.
ساكن
M3_نتیجہ Compile_Ref_AsNonNull (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_نتیجہ نتیجہ = m3Err_none;

    إذا (not Is_ڈھیر_Polymorphic(o)) {
        m3type_t type = Get_ڈھیر_چوٹی_Type(o);
        _throwif(m3Err_typeMismatch, not IsRefType(type));

        // op_RefAsNonNull only knows how to read its operand out of a slot; a
        // value still sitting in the register has to be spilled first, the way
        // every other single-op reader of the stack top does
        إذا (Is_ڈھیر_چوٹی_وچ_رجسٹر(o)) {
_           (Preserve_رجسٹر_If_Occupied(o, type));
        }

        u16 تھاں = Get_ڈھیر_چوٹی_تھاں_نمبر(o);

_       (EmitOp(o, op_RefAsNonNull));
        Emit_تھاں_وتھ(o, تھاں);

        m3type_t nonNull = IsSpelledRefType(type)
                             ? (m3type_t)(type | d_m3Type_refNonNull)
                             : (m3type_t)(d_m3Type_ref | d_m3Type_refNonNull | d_m3_Type_ڈھیر_Abstract |
                                          ((ادھار_Type_دا(type) == c_m3Type_externref) ? d_m3Type_refExtern : 0));

        // the slot keeps the one reference it already had; a Pop()/Push() of
        // the same slot would drop that reference's count without anything
        // to put it back, so the type above it is corrected in place instead
        o->type_ڈھیر[Get_ڈھیر_چوٹی_اشاریہ(o)] = nonNull;
    }

    _catch: أرجع نتیجہ;
}

#جے_انت // d_m3HasTypedRefs


// How wide a table operand is read at comes from the table it names, so a slot
// holding a narrower type would be read past what was allocated for it. The
// validator refuses that already; this keeps a build without one from reaching
// the interpreter with it. c_m3Type_none means the operand is not on the
// dynamic stack to inspect, which is not this check's business.
ساكن
M3_نتیجہ Check_عملیہ_Type (IM3Compilation o, u16 i_ڈونگھائی_From_چوٹی, m3type_t i_type)
{
    m3type_t actual;

    إذا (Is_ڈھیر_Polymorphic(o)) {
        أرجع m3Err_none;
    }

    actual = Get_ڈھیر_Type_From_چوٹی(o, i_ڈونگھائی_From_چوٹی);

    أرجع (actual == c_m3Type_none or ادھار_Type_دا(actual) == i_type) ? m3Err_none
                                                                     : m3Err_typeMismatch;
}


// Swaps the _mem register onto a given memory. Only ever emitted in pairs --
// see op_SetMemory.
ساكن
M3_نتیجہ Emit_سیٹ_حافظہ_Ptr (IM3Compilation o, IM3_حافظہ i_حافظہ)
{
    M3_نتیجہ نتیجہ = m3Err_none;

_   (EmitOp(o, op_سیٹ_حافظہ));
    Emit_اشارہ_گر(o, i_حافظہ);

    _catch: أرجع نتیجہ;
}


// ...onto one of the compiling module's own memories, by index. Only reached
// with a non-zero index under multi-memory, but the index has already been
// bounds-checked by ReadMemoryIndex either way.
ساكن مضمن
M3_نتیجہ Emit_سیٹ_حافظہ (IM3Compilation o, u32 i_حافظہ_Idx)
{
#إذا d_m3_Has_Multi_حافظہ
    أرجع Emit_سیٹ_حافظہ_Ptr(o, o->module->memories[i_حافظہ_Idx]);
#وإلا
    (فراغ)o;
    (فراغ)i_حافظہ_Idx;
    أرجع m3_Err_unknown_حافظہ;
#جے_انت
}

ساكن
M3_نتیجہ Compile_Call (IM3Compilation o, m3opcode_t i_opcode)
{
_try {
    u32 function_اشاریہ;
_   (ReadLEB_u32(&function_اشاریہ, &o->wasm, o->wasm_سرا));

    IM3Function function = Module_GetFunction(o->module, function_اشاریہ);

    إذا (function) {                                                     m3log (compile, d_indent " (func= [%d] '%s'; args= %d)",
                                                                                get_indention_string (o), function_اشاریہ, m3_GetFunctionName (function), function->funcType->عدد_دلیلاں);
        // an import linked to another module's export runs that module's body
        IM3Function نشانہ = Function_Implementation(function);

        إذا (نشانہ->module) {
            bool isReturnCall = (i_opcode == c_waOp_returnCall);

            // The callee's body runs against its own module's memory, so a call
            // that crosses modules is bracketed by a pair of op_SetMemory. A
            // tail call never returns to run the second one, so those compile
            // as a plain call followed by a return instead.
            bool crossModule = (نشانہ->module != o->module);
            bool use_پوچھ_Call = isReturnCall and d_m3_Can_پوچھ_Call and not crossModule;

            u16 تھاں_چوٹی, عدد_Arg_Slots = 0;

            إذا (use_پوچھ_Call) {
_               (Compile_پوچھ_Call_دلیلاں(o, &تھاں_چوٹی, &عدد_Arg_Slots, نشانہ->funcType, false));
            } وإلا {
_               (Compile_Call_دلیلاں_And_Return(o, &تھاں_چوٹی, نشانہ->funcType, false));
            }

            IM3Operation op;
            ثابت فراغ*  عملیہ;

            إذا (نشانہ->compiled) {
                op      = use_پوچھ_Call ? op_ReturnCall : op_Call;
                عملیہ = نشانہ->compiled;
            } وإلا {
                op      = use_پوچھ_Call ? op_CompileReturnCall : op_Compile;
                عملیہ = نشانہ;
            }

#إذا d_m3HasExceptionHandling
            إذا (isReturnCall) {
_               (Emit_نکالو_Try_چوکھٹے_For_Return_Call(o));
            }
#جے_انت

            إذا (crossModule) {
_               (Emit_سیٹ_حافظہ_Ptr(o, Module_Memory0(نشانہ->module)));
            }

_           (EmitOp(o, op));
            Emit_اشارہ_گر(o, عملیہ);
            Emit_تھاں_وتھ(o, تھاں_چوٹی);

            إذا (not use_پوچھ_Call) {
                // a suspend inside the callee leaves this frame waiting here, its
                // results not yet written
_               (ریکارڈ_Call(o, Get_Func_Type_عدد_Results(نشانہ->funcType), تھاں_چوٹی, isReturnCall));
            }

            إذا (use_پوچھ_Call) {
                Emit_تھاں_وتھ(o, o->function->عدد_Ret_Slots);
                EmitConstant32(o, عدد_Arg_Slots);

_               (سیٹ_ڈھیر_Polymorphic(o));
            } وإلا {
                إذا (crossModule) {
_                   (Emit_سیٹ_حافظہ_Ptr(o, Module_Memory0(o->module)));
                }

                إذا (isReturnCall) {
_                   (Compile_Return(o, i_opcode));
                }
            }
        } وإلا {
            _throw(ErrorCompile(m3Err_functionImportMissing, o, "'%s.%s'", GetFunctionImportModuleName(function), m3_GetFunctionName(function)));
        }
    } وإلا {
        _throw(ErrorCompile(m3Err_functionLookupFailed, o, "index: %d", function_اشاریہ));
    }

    } _catch: أرجع نتیجہ;
}

ساكن
M3_نتیجہ Compile_Call_اسدھا (IM3Compilation o, m3opcode_t i_opcode)
{
_try {
    u32 type_اشاریہ;
_   (ReadLEB_u32(&type_اشاریہ, &o->wasm, o->wasm_سرا));

    u32 جدول_اشاریہ;
_   (ReadLEB_u32(&جدول_اشاریہ, &o->wasm, o->wasm_سرا));

    _throwif("function call type index out of range", type_اشاریہ >= o->module->عدد_Func_Types);
    _throwif("table index out of range", جدول_اشاریہ >= o->module->عدد_Tables);
    _throwif(m3Err_typeMismatch, ادھار_Type_دا(o->module->tables[جدول_اشاریہ]->type) != c_m3Type_funcref);

_   (Check_عملیہ_Type(o, 0, جدول_Addr_Type(o->module->tables[جدول_اشاریہ])));

    إذا (Is_ڈھیر_چوٹی_وچ_رجسٹر(o)) {
_       (Preserve_رجسٹر_If_Occupied(o, c_m3Type_i32));
    }

    u16 جدول_اشاریہ_تھاں = Get_ڈھیر_چوٹی_تھاں_نمبر(o);

    bool isReturnCall = (i_opcode == c_wa_Op_return_Call_اسدھا);
    bool use_پوچھ_Call  = isReturnCall and d_m3_Can_پوچھ_Call;

    u16         exec_چوٹی, عدد_Arg_Slots = 0;
    IM3FuncType type = o->module->funcTypes[type_اشاریہ];

    إذا (use_پوچھ_Call) {
_       (Compile_پوچھ_Call_دلیلاں(o, &exec_چوٹی, &عدد_Arg_Slots, type, true));
    } وإلا {
_       (Compile_Call_دلیلاں_And_Return(o, &exec_چوٹی, type, true));
    }

#إذا d_m3HasExceptionHandling
    إذا (isReturnCall) {
_       (Emit_نکالو_Try_چوکھٹے_For_Return_Call(o));
    }
#جے_انت

    // the table comes first: how wide its indexes are is what says how much of
    // the slot below holds one
_   (EmitOp(o, use_پوچھ_Call ? op_Return_Call_اسدھا : op_Call_اسدھا));
    Emit_اشارہ_گر(o, o->module->tables[جدول_اشاریہ]);
    Emit_تھاں_وتھ(o, جدول_اشاریہ_تھاں);
    Emit_اشارہ_گر(o, type);
    Emit_تھاں_وتھ(o, exec_چوٹی);

    إذا (not use_پوچھ_Call) {
_       (ریکارڈ_Call(o, Get_Func_Type_عدد_Results(type), exec_چوٹی, isReturnCall));
    }

    إذا (use_پوچھ_Call) {
        Emit_تھاں_وتھ(o, o->function->عدد_Ret_Slots);
        EmitConstant32(o, عدد_Arg_Slots);

_       (سیٹ_ڈھیر_Polymorphic(o));
    } وإلا إذا (isReturnCall) {
_       (Compile_Return(o, i_opcode));
    }

} _catch:
    أرجع نتیجہ;
}

// Reads the memory index a memory instruction names, and checks it against the
// module's index space. Without multi-memory the immediate is a reserved byte
// that has to be zero, which is the same check.
ساكن
M3_نتیجہ Read_حافظہ_اشاریہ (IM3Compilation o, u32* o_حافظہ_Idx)
{
    M3_نتیجہ نتیجہ;

_   (ReadLEB_u32(o_حافظہ_Idx, &o->wasm, o->wasm_سرا));

    _throwif(m3_Err_unknown_حافظہ, *o_حافظہ_Idx >= o->module->عدد_Memories);

    _catch: أرجع نتیجہ;
}

ساكن
M3_نتیجہ Compile_حافظہ_ناپ (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_نتیجہ نتیجہ;

    u32 حافظہ_Idx;

    // A page count is given in the memory's own address type. Declared up here
    // so the throws below don't jump over its initialization.
    m3type_t addrType = c_m3Type_i32;

_   (Read_حافظہ_اشاریہ(o, &حافظہ_Idx));

    addrType = حافظہ_Addr_Type(o->module->memories[حافظہ_Idx]);

_   (Preserve_رجسٹر_If_Occupied(o, addrType));

    إذا (حافظہ_Idx) {
_       (Emit_سیٹ_حافظہ(o, حافظہ_Idx));
    }

    // op_MemSize writes the whole register either way; the slot it is pushed
    // to is what decides how much of it the module gets to see
_   (EmitOp(o, op_Mem_ناپ));

    إذا (حافظہ_Idx) {
_       (Emit_سیٹ_حافظہ(o, 0));
    }

_   (ڈالو_رجسٹر(o, addrType));

    _catch: أرجع نتیجہ;
}

ساكن
M3_نتیجہ Compile_حافظہ_Grow (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_نتیجہ نتیجہ;

    u32 حافظہ_Idx;

    // see Compile_Memory_Size
    m3type_t addrType = c_m3Type_i32;

_   (Read_حافظہ_اشاریہ(o, &حافظہ_Idx));

    addrType = حافظہ_Addr_Type(o->module->memories[حافظہ_Idx]);

_   (Copy_ڈھیر_چوٹی_منزل_رجسٹر(o, false));
_   (نکالو_Type(o, addrType));

    إذا (حافظہ_Idx) {
_       (Emit_سیٹ_حافظہ(o, حافظہ_Idx));
    }

#إذا d_m3HasMemory64
_   (EmitOp(o, (addrType == c_m3Type_i64) ? op_MemGrow64 : op_MemGrow));
#وإلا
_   (EmitOp(o, op_MemGrow));
#جے_انت

    إذا (حافظہ_Idx) {
_       (Emit_سیٹ_حافظہ(o, 0));
    }

_   (ڈالو_رجسٹر(o, addrType));

    _catch: أرجع نتیجہ;
}

ساكن
M3_نتیجہ Compile_حافظہ_Copy_Fill (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_نتیجہ نتیجہ = m3Err_none;

    // memory.copy names the destination first and then the source; memory.fill
    // names only the one it writes.
    u32  سروت_حافظہ_Idx = 0, نشانہ_حافظہ_Idx = 0;
    bool isCopy = (i_opcode == c_wa_Op_حافظہ_Copy);

    // Each address is typed by the memory it belongs to. The length follows
    // the narrower of the two, so it is 64-bit only when both are - which for
    // memory.fill, naming one memory twice over, means whenever that one is.
    // Declared up here so the throws below don't jump over them.
    m3type_t نشانہ_Type = c_m3Type_i32;
    m3type_t سروت_Type = c_m3Type_i32;
    m3type_t لمائی_Type = c_m3Type_i32;

    _throwif(m3Err_wasmMalformed, o->module->عدد_Memories == 0);

_   (Read_حافظہ_اشاریہ(o, &نشانہ_حافظہ_Idx));

    إذا (isCopy) {
_       (Read_حافظہ_اشاریہ(o, &سروت_حافظہ_Idx));
    }

    نشانہ_Type = حافظہ_Addr_Type(o->module->memories[نشانہ_حافظہ_Idx]);
    سروت_Type = isCopy ? حافظہ_Addr_Type(o->module->memories[سروت_حافظہ_Idx])
                        : نشانہ_Type;
    لمائی_Type = (نشانہ_Type == c_m3Type_i64 and سروت_Type == c_m3Type_i64)
                   ? c_m3Type_i64
                   : c_m3Type_i32;

_   (Copy_ڈھیر_چوٹی_منزل_رجسٹر(o, false));

#إذا d_m3_Has_Multi_حافظہ
    إذا (isCopy and سروت_حافظہ_Idx != نشانہ_حافظہ_Idx) {
        // two memories at once: _mem can only name one, so the op takes both
_       (EmitOp(o, op_MemCopy_x));
        Emit_اشارہ_گر(o, o->module->memories[نشانہ_حافظہ_Idx]);
        Emit_اشارہ_گر(o, o->module->memories[سروت_حافظہ_Idx]);

        // ...and, since they need not be addressed alike, how wide to read
        // each of the operands: bit 0 the destination, bit 1 the source
        EmitConstant32(o, ((نشانہ_Type == c_m3Type_i64) ? 0x1u : 0u) |
                            ((سروت_Type == c_m3Type_i64) ? 0x2u : 0u));
    } وإلا
#جے_انت
    {
        IM3Operation op = isCopy ? op_MemCopy : op_MemFill;

#إذا d_m3HasMemory64
        إذا (نشانہ_Type == c_m3Type_i64) {
            op = isCopy ? op_MemCopy64 : op_MemFill64;
        }
#جے_انت

        إذا (نشانہ_حافظہ_Idx) {
_           (Emit_سیٹ_حافظہ(o, نشانہ_حافظہ_Idx));
        }

_       (EmitOp(o, op));
    }

    // the length is in the register; the other two are named by slot, at
    // whatever width their stack entries were pushed with
_   (نکالو_Type(o, لمائی_Type));
_   (Emit_تھاں_عدد_دا_ڈھیر_چوٹی_And_نکالو(o));      // the source, or the byte to fill with
_   (Emit_تھاں_عدد_دا_ڈھیر_چوٹی_And_نکالو(o));      // the destination

    إذا (نشانہ_حافظہ_Idx and not (isCopy and سروت_حافظہ_Idx != نشانہ_حافظہ_Idx)) {
_       (Emit_سیٹ_حافظہ(o, 0));
    }

    _catch: أرجع نتیجہ;
}


// memory.init and data.drop address a data segment by index. Both are only valid
// when a data count section declared the segments up front.
ساكن
M3_نتیجہ Read_انکڑے_Segment (IM3Compilation o, M3_انکڑے_Segment** o_segment)
{
    M3_نتیجہ نتیجہ = m3Err_none;

    u32 index;
_   (ReadLEB_u32(&index, &o->wasm, o->wasm_سرا));

    _throwif("data count section required", not o->module->has_انکڑے_گنتی);
    _throwif(m3Err_wasmMalformed, index >= o->module->عدد_انکڑے_Segments);

    *o_segment = &o->module->انکڑے_Segments[index];

    _catch: أرجع نتیجہ;
}


M3_نتیجہ Compile_حافظہ_Init (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_نتیجہ نتیجہ = m3Err_none;

    M3_انکڑے_Segment* segment = عدم;
    u32            حافظہ_Idx;

    _throwif(m3Err_wasmMalformed, o->module->عدد_Memories == 0);

_   (Read_انکڑے_Segment(o, &segment));
_   (Read_حافظہ_اشاریہ(o, &حافظہ_Idx));

_   (Copy_ڈھیر_چوٹی_منزل_رجسٹر(o, false));

    إذا (حافظہ_Idx) {
_       (Emit_سیٹ_حافظہ(o, حافظہ_Idx));
    }

#إذا d_m3HasMemory64
    // only the destination follows the memory's address type: a data segment
    // is indexed as an i32 however the memory it is copied into is addressed
_   (EmitOp(o, o->module->memories[حافظہ_Idx]->isMemory64 ? op_MemInit64 : op_MemInit));
#وإلا
_   (EmitOp(o, op_MemInit));
#جے_انت
    Emit_اشارہ_گر(o, segment);
_   (نکالو_Type(o, c_m3Type_i32));
_   (Emit_تھاں_عدد_دا_ڈھیر_چوٹی_And_نکالو(o));
_   (Emit_تھاں_عدد_دا_ڈھیر_چوٹی_And_نکالو(o));

    إذا (حافظہ_Idx) {
_       (Emit_سیٹ_حافظہ(o, 0));
    }

    _catch: أرجع نتیجہ;
}


#إذا d_m3HasRefTypes

ساكن
M3_نتیجہ Compile_Select (IM3Compilation o, m3opcode_t i_opcode);

// select with an explicit result type vector. The types only matter to the
// validator; the operands are laid out exactly as for the untyped select.
M3_نتیجہ Compile_Select_Typed (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_نتیجہ نتیجہ = m3Err_none;

    u32 عدد_Types;
_   (ReadLEB_u32(&عدد_Types, &o->wasm, o->wasm_سرا));
    _throwif(m3Err_wasmMalformed, عدد_Types != 1);

    i8 waType;
    u8 type;
_   (ReadLEB_i7(&waType, &o->wasm, o->wasm_سرا));
_   (NormalizeType(&type, waType));

_   (Compile_Select(o, i_opcode));

    _catch: أرجع نتیجہ;
}


// A reference is one pointer-sized word and null is 0, so null-testing it is
// just an integer eqz of the matching width.
#  إذا M3_SIZEOF_PTR == 8
#    define d_m3RefIsNull_r   op_i64_Equal_منزل_Zero_r
#    define d_m3RefIsNull_s   op_i64_Equal_منزل_Zero_s
#  وإلا
#    define d_m3RefIsNull_r   op_i32_Equal_منزل_Zero_r
#    define d_m3RefIsNull_s   op_i32_Equal_منزل_Zero_s
#  endif

ساكن
M3_نتیجہ ReadRefType (IM3Compilation o, m3type_t* o_type)
{
    M3_نتیجہ نتیجہ = m3Err_none;

#  إذا d_m3HasTypedRefs
    // ref.null names a heap type: func, extern, or a function type index
    m3type_t ڈھیر_بٹ;
_   (Parse_ڈھیر_Type(o->module, &ڈھیر_بٹ, &o->wasm, o->wasm_سرا));

    *o_type = d_m3Type_ref | ڈھیر_بٹ;
#  وإلا
    i8 waType;
    u8 plainType;
_   (ReadLEB_i7(&waType, &o->wasm, o->wasm_سرا));
_   (NormalizeType(&plainType, waType));
    *o_type = plainType;
    _throwif(m3Err_wasmMalformed, not IsRefType(*o_type));
#  endif

    _catch: أرجع نتیجہ;
}


M3_نتیجہ Compile_Ref_Null (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_نتیجہ نتیجہ = m3Err_none;

    m3type_t type;
_   (ReadRefType(o, &type));
_   (ڈالو_Const(o, 0, type));

    _catch: أرجع نتیجہ;
}


M3_نتیجہ Compile_Ref_IsNull (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_نتیجہ نتیجہ = m3Err_none;

    IM3Operation op;

    إذا (not Is_ڈھیر_Polymorphic(o)) {
        _throwif(m3Err_typeMismatch, not IsRefType(Get_ڈھیر_چوٹی_Type(o)));
    }

    إذا (Is_ڈھیر_چوٹی_وچ_رجسٹر(o)) {
        op = d_m3RefIsNull_r;
    } وإلا {
_       (Preserve_رجسٹر_If_Occupied(o, c_m3Type_i32));
        op = d_m3RefIsNull_s;
    }

_   (EmitOp(o, op));
_   (Emit_تھاں_عدد_دا_ڈھیر_چوٹی_And_نکالو(o));
_   (ڈالو_رجسٹر(o, c_m3Type_i32));

    _catch: أرجع نتیجہ;
}


M3_نتیجہ Compile_Ref_Func (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_نتیجہ نتیجہ = m3Err_none;

    u32 func_اشاریہ;

    // declared before the throws below, which jump past this point to _catch
    m3type_t    refType   = c_m3Type_funcref;
    IM3Function reference = عدم;

_   (ReadLEB_u32(&func_اشاریہ, &o->wasm, o->wasm_سرا));
    _throwif("function index out of range", func_اشاریہ >= o->module->عدد_Functions);

    // Inside a constant expression ref.func is itself a declaration; inside a
    // function body the function must already have been declared elsewhere.
    إذا (o->function) {
        _throwif("undeclared function reference", not Module_IsFunctionDeclared(o->module, func_اشاریہ));
    } وإلا {
_       (Module_DeclareFunction(o->module, func_اشاریہ));
    }

#  إذا d_m3HasTypedRefs
    refType = Ref_Type_دا_Func_Type(o->module->functions[func_اشاریہ].funcType, true);
#  endif

    // A reference denotes the function that actually runs, so a ref.func naming
    // an import linked to another module is the same reference that module's
    // own ref.func would produce - and carries the defining module with it.
    reference = Function_Implementation(&o->module->functions[func_اشاریہ]);

_   (ڈالو_Const(o, (u64)(uintptr_t)reference, refType));

    _catch: أرجع نتیجہ;
}


// table.get/set/size/grow/fill all name a table by index; the table struct goes
// into the codestream so the operation doesn't have to walk the module.
ساكن
M3_نتیجہ Read_جدول (IM3Compilation o, M3_جدول** o_جدول)
{
    M3_نتیجہ نتیجہ = m3Err_none;

    u32 index;
_   (ReadLEB_u32(&index, &o->wasm, o->wasm_سرا));
    _throwif("table index out of range", index >= o->module->عدد_Tables);

    *o_جدول = o->module->tables[index];

    _catch: أرجع نتیجہ;
}


// The table operations take every operand from a slot, so spill the integer
// register first: after this each EmitSlotNumOfStackTopAndPop () emits a real
// slot offset, and the register is free for the result. Operands are emitted
// from the top of the stack downwards, which is the order the ops read them.
ساكن
M3_نتیجہ Compile_جدول_Op (IM3Compilation o, IM3Operation i_op, M3_جدول* i_جدول, u32 i_عدد_Operands, m3type_t i_retType)
{
    M3_نتیجہ نتیجہ = m3Err_none;

_   (Preserve_رجسٹر_If_Occupied(o, c_m3Type_i64));

_   (EmitOp(o, i_op));
    Emit_اشارہ_گر(o, i_جدول);

    لكل (u32 i = 0; i < i_عدد_Operands; ++i) {
_       (Emit_تھاں_عدد_دا_ڈھیر_چوٹی_And_نکالو(o));
    }

    إذا (i_retType != c_m3Type_none) {
_       (ڈالو_رجسٹر(o, i_retType));
    }

    _catch: أرجع نتیجہ;
}


M3_نتیجہ Compile_جدول_Get_سیٹ (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_نتیجہ نتیجہ = m3Err_none;

    M3_جدول* جدول_2;
_   (Read_جدول(o, &جدول_2));

    إذا (i_opcode == c_wa_Op_جدول_Get) {
_       (Check_عملیہ_Type(o, 0, جدول_Addr_Type(جدول_2)));
_       (Compile_جدول_Op(o, op_جدول_Get, جدول_2, 1, جدول_2->type));
    } وإلا {
_       (Check_عملیہ_Type(o, 1, جدول_Addr_Type(جدول_2)));
_       (Compile_جدول_Op(o, op_جدول_سیٹ, جدول_2, 2, c_m3Type_none));
    }

    _catch: أرجع نتیجہ;
}


M3_نتیجہ Compile_جدول_Init (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_نتیجہ نتیجہ = m3Err_none;

    u32      elem_اشاریہ;
    M3_جدول* جدول_2;

_   (ReadLEB_u32(&elem_اشاریہ, &o->wasm, o->wasm_سرا));
    _throwif("element segment index out of range", elem_اشاریہ >= o->module->عدد_Element_Segments);
_   (Read_جدول(o, &جدول_2));

_   (Check_عملیہ_Type(o, 2, جدول_Addr_Type(جدول_2)));

_   (Preserve_رجسٹر_If_Occupied(o, c_m3Type_i64));
_   (EmitOp(o, op_جدول_Init));
    Emit_اشارہ_گر(o, جدول_2);
    Emit_اشارہ_گر(o, &o->module->elementSegments[elem_اشاریہ]);

    لكل (u32 i = 0; i < 3; ++i) {
_       (Emit_تھاں_عدد_دا_ڈھیر_چوٹی_And_نکالو(o));
    }

    _catch: أرجع نتیجہ;
}


M3_نتیجہ Compile_Elem_Drop (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_نتیجہ نتیجہ = m3Err_none;

    u32 elem_اشاریہ;
_   (ReadLEB_u32(&elem_اشاریہ, &o->wasm, o->wasm_سرا));
    _throwif("element segment index out of range", elem_اشاریہ >= o->module->عدد_Element_Segments);

_   (EmitOp(o, op_ElemDrop));
    Emit_اشارہ_گر(o, &o->module->elementSegments[elem_اشاریہ]);

    _catch: أرجع نتیجہ;
}


M3_نتیجہ Compile_جدول_Copy (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_نتیجہ نتیجہ = m3Err_none;

    M3_جدول* dst;
    M3_جدول* src;

_   (Read_جدول(o, &dst));
_   (Read_جدول(o, &src));
    _throwif(m3Err_typeMismatch, not Is_Sub_Type_دا(src->type, dst->type));

_   (Check_عملیہ_Type(o, 2, جدول_Addr_Type(dst)));
_   (Check_عملیہ_Type(o, 1, جدول_Addr_Type(src)));
_   (Check_عملیہ_Type(o, 0, (dst->isTable64 and src->isTable64) ? c_m3Type_i64 : c_m3Type_i32));

_   (Preserve_رجسٹر_If_Occupied(o, c_m3Type_i64));
_   (EmitOp(o, op_جدول_Copy));
    Emit_اشارہ_گر(o, dst);
    Emit_اشارہ_گر(o, src);

    لكل (u32 i = 0; i < 3; ++i) {
_       (Emit_تھاں_عدد_دا_ڈھیر_چوٹی_And_نکالو(o));
    }

    _catch: أرجع نتیجہ;
}


M3_نتیجہ Compile_جدول_ناپ (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_نتیجہ نتیجہ = m3Err_none;

    M3_جدول* جدول_2;
_   (Read_جدول(o, &جدول_2));

    // a table size is given in the table's own index type
_   (Compile_جدول_Op(o, op_جدول_ناپ, جدول_2, 0, جدول_Addr_Type(جدول_2)));

    _catch: أرجع نتیجہ;
}


M3_نتیجہ Compile_جدول_Grow_Fill (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_نتیجہ نتیجہ = m3Err_none;

    M3_جدول* جدول_2;
_   (Read_جدول(o, &جدول_2));

    إذا (i_opcode == c_wa_Op_جدول_Grow) {
_       (Check_عملیہ_Type(o, 0, جدول_Addr_Type(جدول_2)));
_       (Compile_جدول_Op(o, op_جدول_Grow, جدول_2, 2, جدول_Addr_Type(جدول_2)));
    } وإلا {
_       (Check_عملیہ_Type(o, 0, جدول_Addr_Type(جدول_2)));
_       (Check_عملیہ_Type(o, 2, جدول_Addr_Type(جدول_2)));
_       (Compile_جدول_Op(o, op_جدول_Fill, جدول_2, 3, c_m3Type_none));
    }

    _catch: أرجع نتیجہ;
}

#جے_انت // d_m3HasRefTypes


M3_نتیجہ Compile_انکڑے_Drop (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_نتیجہ نتیجہ = m3Err_none;

    M3_انکڑے_Segment* segment = عدم;
_   (Read_انکڑے_Segment(o, &segment));

_   (EmitOp(o, op_انکڑے_Drop));
    Emit_اشارہ_گر(o, segment);

    _catch: أرجع نتیجہ;
}


ساكن
M3_نتیجہ Read_بلاک_Type (IM3Compilation o, IM3FuncType* o_بلاک_Type)
{
    M3_نتیجہ نتیجہ = m3Err_none;

#إذا d_m3HasTypedRefs
    // a spelled-out reference type is two bytes, so it cannot be told apart
    // from a type index by the s33 that block types otherwise use
    إذا (o->wasm < o->wasm_سرا and (*o->wasm == d_waEncode_ref or *o->wasm == d_waEncode_refNull)) {
        m3type_t refType;
_       (Parse_قدر_Type(o->module, &refType, &o->wasm, o->wasm_سرا));
        IM3FuncType ftype = عدم;
_       (AllocFuncType(&ftype, 1));
        ftype->عدد_Rets  = 1;
        ftype->types[0] = refType;
_       (Environment_جوڑو_Func_Type(o->module->environment, &ftype));
        *o_بلاک_Type = ftype;
        أرجع نتیجہ;
    }
#جے_انت

    i64 type;
_   (ReadLebSigned(&type, 33, &o->wasm, o->wasm_سرا));

    إذا (type < 0) {
        _throwif(m3Err_invalidTypeId, type < -64);
        u8 قدر_Type;
_       (NormalizeType(&قدر_Type, (i8)type));                                 m3log (compile, d_indent " (type: %s)", get_indention_string (o), c_waTypes [قدر_Type]);
        *o_بلاک_Type = o->module->environment->retFuncTypes[قدر_Type];
    } وإلا {
        _throwif("func type out of bounds", type >= o->module->عدد_Func_Types);
        *o_بلاک_Type = o->module->funcTypes[type];                         m3log (compile, d_indent " (type: %s)", get_indention_string (o), SPrintFuncTypeSignature (*o_بلاک_Type));
    }
    _catch: أرجع نتیجہ;
}

ساكن
M3_نتیجہ Preserve_دلیلاں_And_Locals (IM3Compilation o)
{
    M3_نتیجہ نتیجہ = m3Err_none;

    إذا (o->ڈھیر_اشاریہ > o->ڈھیر_پہلا_Dynamic_اشاریہ) {
        u32 عدد_دلیلاں_And_Locals = Get_Function_عدد_دلیلاں_And_Locals(o->function);

        لكل (u32 i = 0; i < عدد_دلیلاں_And_Locals; ++i) {
            u16 تھاں = Get_تھاں_For_ڈھیر_اشاریہ(o, i);

            u16 preserved_تھاں_نمبر;
_           (لبھو_Referenced_Local_Within_Current_بلاک(o, &preserved_تھاں_نمبر, تھاں));

            إذا (preserved_تھاں_نمبر != تھاں) {
                m3type_t     type = Get_ڈھیر_Type_From_Bottom(o, i);                 d_m3Assert (type != c_m3Type_none)
                IM3Operation op   = Is64_ثنائی_ہندسہ_Type(type) ? op_Copy_تھاں_64 : op_Copy_تھاں_32;

                EmitOp(o, op);
                Emit_تھاں_وتھ(o, preserved_تھاں_نمبر);
                Emit_تھاں_وتھ(o, تھاں);
            }
        }
    }

    _catch:
    أرجع نتیجہ;
}

ساكن
M3_نتیجہ Compile_Loop_Or_بلاک (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_نتیجہ نتیجہ;

    IM3FuncType بلاک_Type = عدم;

_   (PreserveRegisters(o));
_   (Preserve_دلیلاں_And_Locals(o));

_   (Read_بلاک_Type(o, &بلاک_Type));

    إذا (i_opcode == c_waOp_loop) {
        u16 عدد_Params = Get_Func_Type_عدد_Params(بلاک_Type);
        إذا (عدد_Params) {
            // instantiate constants
            u16 عدد_اقدار = Get_عدد_بلاک_اقدار_On_ڈھیر(o);

            إذا (عدد_اقدار >= عدد_Params) {
                u16 ڈھیر_چوٹی = Get_ڈھیر_چوٹی_اشاریہ(o) + 1;

                لكل (u16 i = ڈھیر_چوٹی - عدد_Params; i < ڈھیر_چوٹی; ++i) {
                    u16      تھاں = Get_تھاں_For_ڈھیر_اشاریہ(o, i);
                    m3type_t type = Get_ڈھیر_Type_From_Bottom(o, i);

                    إذا (Is_Constant_تھاں(o, تھاں)) {
                        u16 new_تھاں = c_تھاں_بن_ورتیا;
_                       (AllocateSlots(o, &new_تھاں, type));
_                       (Copy_ڈھیر_اشاریہ_منزل_تھاں(o, new_تھاں, i));
                        o->wasm_ڈھیر[i] = new_تھاں;
                    }
                }
            }
        }

_       (EmitOp(o, op_Loop));
    } وإلا {
    }

_   (Compile_بلاک(o, بلاک_Type, i_opcode));

    _catch: أرجع نتیجہ;
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
M3_نتیجہ EmitCatchStubs (IM3Compilation o)
{
    IM3_کوڈ_ورقہ displaced_ورقہ = عدم;
_try {
    M3CatchClause* clauses    = o->tryClauses;
    u32            عدد_Clauses = o->عدد_Try_Clauses;

    // the body must not see these: a nested try_table sets up its own
    o->tryClauses    = عدم;
    o->عدد_Try_Clauses = 0;

    لكل (u32 i = 0; i < عدد_Clauses; ++i) {
        M3CatchClause* clause = &clauses[i];

        IM3CompilationScope scope;
_       (Get_بلاک_Scope(o, &scope, clause->label_ڈونگھائی + 1));

        IM3FuncType payload = clause->ٹیگ ? clause->ٹیگ->type : عدم;
        u16         عدد_دلیلاں = Get_Func_Type_عدد_Params(payload);

        // What the stub hands the label - the payload, then the exnref for a
        // _ref clause - has to be what the label takes. ResolveBlockResults
        // only counts the values, and the validator compares base types, so a
        // typed reference is only checked here: (ref null $t) reaching a label
        // that takes (ref $t) has to be rejected.
        {
            bool isLoop         = (scope->opcode == c_waOp_loop);
            u16  عدد_Label_اقدار = isLoop ? Get_Func_Type_عدد_Params(scope->type)
                                         : Get_Func_Type_عدد_Results(scope->type);

            _throwif(m3_Err_type_گنتی_Mismatch, عدد_Label_اقدار != عدد_دلیلاں + (clause->hasRef ? 1 : 0));

            لكل (u16 a = 0; a < عدد_Label_اقدار; ++a) {
                m3type_t given  = (a < عدد_دلیلاں) ? GetFuncTypeParamType(payload, a)
                                                : (m3type_t)c_m3Type_exnref;
                m3type_t wanted = isLoop ? GetFuncTypeParamType(scope->type, a)
                                         : Get_Func_Type_نتیجہ_Type(scope->type, a);

                _throwif(m3Err_typeMismatch, not Is_Sub_Type_دا(given, wanted));
            }
        }

        IM3_کوڈ_ورقہ stub_ورقہ;
_       (Acquire_Compilation_کوڈ_ورقہ(o, &stub_ورقہ));

        pc_t شروع_PC  = Get_ورقہ_PC(stub_ورقہ);
        displaced_ورقہ = o->ورقہ;
        o->ورقہ       = stub_ورقہ;

        u16 پہلا_اشاریہ = o->ڈھیر_اشاریہ;

        لكل (u16 a = 0; a < عدد_دلیلاں; ++a) {
_           (ڈالو_Allocated_تھاں(o, GetFuncTypeParamType(payload, a)));
        }

        إذا (clause->hasRef) {
_           (ڈالو_Allocated_تھاں(o, c_m3Type_exnref));
        }

        // emitted even with nothing to move: this is also where an exception
        // no clause will reify gets released
_       (Ensure_کوڈ_ورقہ_عدد_سطریں(o, 2 * عدد_دلیلاں + 3 + d_m3_کوڈ_ورقہ_Free_سطریں_حد));

_       (EmitOp(o, op_CatchPayload));
        EmitConstant32(o, عدد_دلیلاں);

        لكل (u16 a = 0; a < عدد_دلیلاں; ++a) {
            u16 index = پہلا_اشاریہ + a;
            Emit_تھاں_وتھ(o, Get_تھاں_For_ڈھیر_اشاریہ(o, index));
            EmitConstant32(o, Is64_ثنائی_ہندسہ_Type(Get_ڈھیر_Type_From_Bottom(o, index)));
        }

        Emit_تھاں_وتھ(o, clause->hasRef ? (i32)Get_تھاں_For_ڈھیر_اشاریہ(o, پہلا_اشاریہ + عدد_دلیلاں) : -1);

_       (Emit_نکالو_Try_چوکھٹے(o, عدد_Try_چوکھٹے_منزل_نکالو(o->بلاک.outer, scope)));

        إذا (scope->opcode == c_waOp_loop) {
_           (Resolve_بلاک_Results(o, scope, /* isBranch: */ true));

_           (EmitOp(o, Op_ContinueLoop(o)));
            Emit_اشارہ_گر(o, scope->pc);
_           (ریکارڈ_پٹھ_کنارا(o, scope, false));
        } وإلا إذا (scope->ڈونگھائی == 0) {
_           (Return_اقدار(o, scope, /* isBranch: */ true));
_           (EmitOp(o, op_Return));
        } وإلا {
_           (Resolve_بلاک_Results(o, scope, /* isBranch: */ true));
_           (EmitPatchingBranch(o, scope));
        }

        IM3_کوڈ_ورقہ return_منزل = displaced_ورقہ;
        displaced_ورقہ        = عدم;

        Release_Compilation_کوڈ_ورقہ(o);
        o->ورقہ = return_منزل;

        *(pc_t*)clause->stub_تھاں = شروع_PC;

        طالما (o->ڈھیر_اشاریہ > پہلا_اشاریہ) {
_           (نکالو(o));
        }
    }

} _catch:

    // thrown with a stub page installed: release it and put back the one it
    // displaced, which the caller's catch then releases
    إذا (displaced_ورقہ) {
        Release_Compilation_کوڈ_ورقہ(o);
        o->ورقہ = displaced_ورقہ;
    }

    أرجع نتیجہ;
}


ساكن
M3_نتیجہ Compile_Try_جدول (IM3Compilation o, m3opcode_t i_opcode)
{
    M3CatchClause* clauses = عدم;
_try {
    // a catch entry has to be able to assume the operand stack is entirely in
    // slots: an unwind leaves nothing in the registers
_   (PreserveRegisters(o));
_   (Preserve_دلیلاں_And_Locals(o));

    IM3FuncType بلاک_Type = عدم;
_   (Read_بلاک_Type(o, &بلاک_Type));

    u32 عدد_Clauses;
_   (ReadLEB_u32(&عدد_Clauses, &o->wasm, o->wasm_سرا));

    _throwif("too many catch clauses", عدد_Clauses > d_m3_Max_Sane_ٹیگ_سوچی_گنتی);

    إذا (عدد_Clauses) {
        clauses = m3_AllocArray(M3CatchClause, عدد_Clauses);
        _throwifnull(clauses);
    }

    لكل (u32 i = 0; i < عدد_Clauses; ++i) {
        u8 قسم;
_       (Read_u8(&قسم, &o->wasm, o->wasm_سرا));
        _throwif(m3Err_wasmMalformed, قسم > 0x03);

        clauses[i].ٹیگ    = عدم;
        clauses[i].hasRef = (قسم == 0x01 or قسم == 0x03);

        إذا (قسم == 0x00 or قسم == 0x01) {
            u32 ٹیگ_اشاریہ;
_           (ReadLEB_u32(&ٹیگ_اشاریہ, &o->wasm, o->wasm_سرا));
            _throwif(m3_Err_unknown_ٹیگ, ٹیگ_اشاریہ >= o->module->عدد_ٹیگ_سوچی);

            IM3_ٹیگ ٹیگ     = &o->module->ٹیگ_سوچی[ٹیگ_اشاریہ];
            clauses[i].ٹیگ = ٹیگ->resolved ? ٹیگ->resolved : ٹیگ;
        }

_       (ReadLEB_u32(&clauses[i].label_ڈونگھائی, &o->wasm, o->wasm_سرا));
    }

    // op_TryTable, the clause count, and two words per clause, all of which have
    // to land on one page: the op reads the table by offset from its own pc
_   (Ensure_کوڈ_ورقہ_عدد_سطریں(o, 2 * عدد_Clauses + 2 + d_m3_کوڈ_ورقہ_Free_سطریں_حد));

_   (EmitOp(o, op_Try_جدول));
    EmitConstant32(o, عدد_Clauses);

    لكل (u32 i = 0; i < عدد_Clauses; ++i) {
        Emit_اشارہ_گر(o, clauses[i].ٹیگ);
        clauses[i].stub_تھاں = Reserve_اشارہ_گر(o);
    }

    o->tryClauses    = clauses;
    o->عدد_Try_Clauses = عدد_Clauses;

_   (Compile_بلاک(o, بلاک_Type, c_wa_Op_try_جدول));

} _catch:

    o->tryClauses    = عدم;
    o->عدد_Try_Clauses = 0;

    m3_Free(clauses);

    أرجع نتیجہ;
}


ساكن
M3_نتیجہ Compile_Throw (IM3Compilation o, m3opcode_t i_opcode)
{
_try {
    u32 ٹیگ_اشاریہ;
_   (ReadLEB_u32(&ٹیگ_اشاریہ, &o->wasm, o->wasm_سرا));
    _throwif(m3_Err_unknown_ٹیگ, ٹیگ_اشاریہ >= o->module->عدد_ٹیگ_سوچی);

    IM3_ٹیگ ٹیگ = &o->module->ٹیگ_سوچی[ٹیگ_اشاریہ];
    إذا (ٹیگ->resolved) {
        ٹیگ = ٹیگ->resolved;
    }

    u16 عدد_دلیلاں = Get_Func_Type_عدد_Params(ٹیگ->type);

    // the payload is read out of slots, so nothing may still be in a register
_   (PreserveRegisters(o));

    // check the payload before emitting anything, so a mistyped throw does not
    // leave half an operation behind
    bool isPolymorphic = Is_ڈھیر_Polymorphic(o);

    إذا (not isPolymorphic) {
        _throwif(m3_Err_type_گنتی_Mismatch, Get_عدد_بلاک_اقدار_On_ڈھیر(o) < عدد_دلیلاں);

        لكل (u16 i = 0; i < عدد_دلیلاں; ++i) {
            u16 index = (u16)(o->ڈھیر_اشاریہ - عدد_دلیلاں + i);

            _throwif(m3Err_typeMismatch, not Is_Sub_Type_دا(Get_ڈھیر_Type_From_Bottom(o, index),
                                                         GetFuncTypeParamType(ٹیگ->type, i)));
        }
    }

_   (Ensure_کوڈ_ورقہ_عدد_سطریں(o, 2 * عدد_دلیلاں + 3 + d_m3_کوڈ_ورقہ_Free_سطریں_حد));

_   (EmitOp(o, op_Throw));
    Emit_اشارہ_گر(o, ٹیگ);
    EmitConstant32(o, عدد_دلیلاں);

    // the payload is named in the order the tag declares it, so the deepest of
    // the arguments on the stack comes first. In unreachable code there is
    // nothing on the stack to name and nothing will run this, so slot 0 stands
    // in for an operand that isn't there.
    لكل (u16 i = 0; i < عدد_دلیلاں; ++i) {
        m3type_t type = GetFuncTypeParamType(ٹیگ->type, i);

        Emit_تھاں_وتھ(o, isPolymorphic ? 0 : Get_تھاں_For_ڈھیر_اشاریہ(o, (u16)(o->ڈھیر_اشاریہ - عدد_دلیلاں + i)));
        EmitConstant32(o, Is64_ثنائی_ہندسہ_Type(type));
    }

    إذا (not isPolymorphic) {
        لكل (u16 i = 0; i < عدد_دلیلاں; ++i) {
_           (نکالو(o));
        }
    }

_   (سیٹ_ڈھیر_Polymorphic(o));

} _catch: أرجع نتیجہ;
}


ساكن
M3_نتیجہ Compile_ThrowRef (IM3Compilation o, m3opcode_t i_opcode)
{
_try {
    إذا (not Is_ڈھیر_Polymorphic(o)) {
_       (Preserve_رجسٹر_If_Occupied(o, c_m3Type_i64));

        _throwif(m3Err_typeMismatch, ادھار_Type_دا(Get_ڈھیر_چوٹی_Type(o)) != c_m3Type_exnref);

_       (EmitOp(o, op_ThrowRef));
        Emit_تھاں_وتھ(o, Get_ڈھیر_چوٹی_تھاں_نمبر(o));

_       (نکالو(o));
    }

_   (سیٹ_ڈھیر_Polymorphic(o));

} _catch: أرجع نتیجہ;
}

#جے_انت // d_m3HasExceptionHandling

#إذا d_m3_Has_ڈھیر_Switching

ساكن
M3_نتیجہ Compile_ContNew (IM3Compilation o, m3opcode_t i_opcode)
{
_try {
    u32 type_اشاریہ;
_   (ReadLEB_u32(&type_اشاریہ, &o->wasm, o->wasm_سرا));
    _throwif(m3_Err_type_گنتی_Mismatch, type_اشاریہ >= o->module->عدد_Func_Types);

    IM3FuncType funcType = o->module->funcTypes[type_اشاریہ];
    _throwif("type is not a continuation", not funcType->isContinuation);

    // the operands are read out of slots, so nothing may still be in a register
_   (PreserveRegisters(o));

    // cont.new $ct : [(ref null $ft)] -> [(ref $ct)], where $ct ~~ cont $ft:
    // the function has to be one this continuation type can run
    m3type_t چوٹی_Type = Get_ڈھیر_چوٹی_Type(o);
    _throwif(m3Err_typeMismatch,
             not Is_Sub_Type_دا(چوٹی_Type, Ref_Type_دا_Func_Type(funcType->contFuncType, false)));

    u16 func_تھاں = Get_ڈھیر_چوٹی_تھاں_نمبر(o);
_   (نکالو(o));

    u16 dst_تھاں;
_   (ڈالو_Allocated_تھاں(o, Ref_Type_دا_Func_Type(funcType, true)));
    dst_تھاں = Get_ڈھیر_چوٹی_تھاں_نمبر(o);

_   (Ensure_کوڈ_ورقہ_عدد_سطریں(o, 4 + d_m3_کوڈ_ورقہ_Free_سطریں_حد));
_   (EmitOp(o, op_ContNew));
    Emit_اشارہ_گر(o, funcType);
    Emit_تھاں_وتھ(o, dst_تھاں);
    Emit_تھاں_وتھ(o, func_تھاں);

} _catch: أرجع نتیجہ;
}

ساكن
M3_نتیجہ Compile_ContBind (IM3Compilation o, m3opcode_t i_opcode)
{
_try {
    u32 typeIndex1, typeIndex2;
_   (ReadLEB_u32(&typeIndex1, &o->wasm, o->wasm_سرا));
_   (ReadLEB_u32(&typeIndex2, &o->wasm, o->wasm_سرا));
    _throwif(m3_Err_type_گنتی_Mismatch, typeIndex1 >= o->module->عدد_Func_Types);
    _throwif(m3_Err_type_گنتی_Mismatch, typeIndex2 >= o->module->عدد_Func_Types);

    IM3FuncType ct1 = o->module->funcTypes[typeIndex1];
    IM3FuncType ct2 = o->module->funcTypes[typeIndex2];
    _throwif("type is not a continuation", not ct1->isContinuation || not ct2->isContinuation);

    // the operands are read out of slots, so nothing may still be in a register
_   (PreserveRegisters(o));

    m3type_t contType = Get_ڈھیر_چوٹی_Type(o);
    _throwif(m3Err_typeMismatch, not Is_Sub_Type_دا(contType, Ref_Type_دا_Func_Type(ct1, false)));
    u16 cont_تھاں = Get_ڈھیر_چوٹی_تھاں_نمبر(o);
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
    لكل (u16 i = 0; i < عدد_Results1; ++i) {
        _throwif(m3Err_typeMismatch, Get_Func_Type_نتیجہ_Type(ft1, i) != Get_Func_Type_نتیجہ_Type(ft2, i));
    }
    لكل (u16 i = 0; i < عدد_Params2; ++i) {
        _throwif(m3Err_typeMismatch, GetFuncTypeParamType(ft1, عدد_Bound + i) != GetFuncTypeParamType(ft2, i));
    }

    u16 argSlots[d_m3MaxContinuationPayload];
    _throwif("too many bound args", عدد_Bound > d_m3MaxContinuationPayload);

    لكل (u32 i = 0; i < عدد_Bound; ++i) {
        u32      paramIdx     = عدد_Bound - 1 - i;
        m3type_t expectedType = GetFuncTypeParamType(ft1, paramIdx);
        m3type_t actualType   = Get_ڈھیر_چوٹی_Type(o);
        _throwif(m3Err_typeMismatch, not Is_Sub_Type_دا(actualType, expectedType));
        argSlots[paramIdx] = Get_ڈھیر_چوٹی_تھاں_نمبر(o);
_       (نکالو(o));
    }

    u16 dst_تھاں;
_   (ڈالو_Allocated_تھاں(o, Ref_Type_دا_Func_Type(ct2, true)));
    dst_تھاں = Get_ڈھیر_چوٹی_تھاں_نمبر(o);

_   (Ensure_کوڈ_ورقہ_عدد_سطریں(o, 6 + عدد_Bound * 2 + d_m3_کوڈ_ورقہ_Free_سطریں_حد));
_   (EmitOp(o, op_ContBind));
    Emit_اشارہ_گر(o, ct1);
    Emit_اشارہ_گر(o, ct2);
    Emit_تھاں_وتھ(o, dst_تھاں);
    Emit_تھاں_وتھ(o, cont_تھاں);
    EmitConstant32(o, عدد_Bound);
    لكل (u32 i = 0; i < عدد_Bound; ++i) {
        Emit_تھاں_وتھ(o, argSlots[i]);
        EmitConstant32(o, Is64_ثنائی_ہندسہ_Type(GetFuncTypeParamType(ft1, i)));
    }

} _catch: أرجع نتیجہ;
}

ساكن
M3_نتیجہ Compile_Suspend (IM3Compilation o, m3opcode_t i_opcode)
{
_try {
    u32 ٹیگ_اشاریہ;
_   (ReadLEB_u32(&ٹیگ_اشاریہ, &o->wasm, o->wasm_سرا));
    _throwif(m3_Err_unknown_ٹیگ, ٹیگ_اشاریہ >= o->module->عدد_ٹیگ_سوچی);

    IM3_ٹیگ ٹیگ = &o->module->ٹیگ_سوچی[ٹیگ_اشاریہ];
    إذا (ٹیگ->resolved) {
        ٹیگ = ٹیگ->resolved;
    }
    u16 عدد_Params  = Get_Func_Type_عدد_Params(ٹیگ->type);
    u16 عدد_Results = Get_Func_Type_عدد_Results(ٹیگ->type);

_   (PreserveRegisters(o));

    u16 paramSlots[d_m3MaxContinuationPayload];
    _throwif("too many tag params", عدد_Params > d_m3MaxContinuationPayload);
    لكل (u16 i = 0; i < عدد_Params; ++i) {
        u16      pIdx         = عدد_Params - 1 - i;
        m3type_t expectedType = GetFuncTypeParamType(ٹیگ->type, pIdx);
        m3type_t actualType   = Get_ڈھیر_چوٹی_Type(o);
        _throwif(m3Err_typeMismatch, not Is_Sub_Type_دا(actualType, expectedType));
        paramSlots[pIdx] = Get_ڈھیر_چوٹی_تھاں_نمبر(o);
_       (نکالو(o));
    }

    u16 نتیجہ_Slots[d_m3MaxContinuationPayload];
    _throwif("too many tag results", عدد_Results > d_m3MaxContinuationPayload);
    لكل (u16 i = 0; i < عدد_Results; ++i) {
        m3type_t rtype = Get_Func_Type_نتیجہ_Type(ٹیگ->type, i);
_       (ڈالو_Allocated_تھاں(o, rtype));
        نتیجہ_Slots[i] = Get_ڈھیر_چوٹی_تھاں_نمبر(o);
    }

_   (Ensure_کوڈ_ورقہ_عدد_سطریں(o, 4 + عدد_Params * 2 + عدد_Results * 2 + d_m3_کوڈ_ورقہ_Free_سطریں_حد));
_   (EmitOp(o, op_Suspend));
    Emit_اشارہ_گر(o, ٹیگ);
    EmitConstant32(o, عدد_Params);
    لكل (u16 i = 0; i < عدد_Params; ++i) {
        Emit_تھاں_وتھ(o, paramSlots[i]);
        EmitConstant32(o, Is64_ثنائی_ہندسہ_Type(GetFuncTypeParamType(ٹیگ->type, i)));
    }
    EmitConstant32(o, عدد_Results);
    لكل (u16 i = 0; i < عدد_Results; ++i) {
        Emit_تھاں_وتھ(o, نتیجہ_Slots[i]);
        EmitConstant32(o, Is64_ثنائی_ہندسہ_Type(Get_Func_Type_نتیجہ_Type(ٹیگ->type, i)));
    }

    // the results are part of the frame here: op_Suspend clears them, and
    // whatever resumes this writes them before anything reads them
_   (ریکارڈ_محفوظ_نقطہ(o, safepoint_suspend, 0, عدد_Results, 0, عدم));

} _catch: أرجع نتیجہ;
}

// resume, resume_throw and resume_throw_ref. All three run a continuation
// under a handler table and take its results; they differ only in what they
// hand it to start with - arguments, a fresh exception, or one off the stack.
//
// Emitted as: the continuation type, the slot holding it, the operands that
// belong to this form, the handler table, then the result slots. Keeping the
// tail identical is what lets one runtime path finish all three.
ساكن
M3_نتیجہ Compile_Resume (IM3Compilation o, m3opcode_t i_opcode)
{
    M3ResumeHandler* handlers      = عدم;
    IM3_کوڈ_ورقہ      displaced_ورقہ = عدم;
_try {
    bool isThrow    = (i_opcode == c_waOp_resumeThrow);
    bool isThrowRef = (i_opcode == c_waOp_resumeThrowRef);

    u32 type_اشاریہ;
_   (ReadLEB_u32(&type_اشاریہ, &o->wasm, o->wasm_سرا));
    _throwif(m3_Err_type_گنتی_Mismatch, type_اشاریہ >= o->module->عدد_Func_Types);

    IM3FuncType contFuncType = o->module->funcTypes[type_اشاریہ];
    _throwif("type is not a continuation", not contFuncType->isContinuation);

    IM3FuncType innerType      = contFuncType->contFuncType;
    u16         عدد_Cont_Results = Get_Func_Type_عدد_Results(innerType);

    IM3_ٹیگ exn_ٹیگ      = عدم;
    u16    عدد_Operands = 0;

    إذا (isThrow) {
        u32 ٹیگ_اشاریہ;
_       (ReadLEB_u32(&ٹیگ_اشاریہ, &o->wasm, o->wasm_سرا));
        _throwif(m3_Err_unknown_ٹیگ, ٹیگ_اشاریہ >= o->module->عدد_ٹیگ_سوچی);

        exn_ٹیگ = &o->module->ٹیگ_سوچی[ٹیگ_اشاریہ];

        // it is an exception that gets raised, so the tag must be one throw
        // could have raised: no results
        _throwif(m3Err_typeMismatch, Get_Func_Type_عدد_Results(exn_ٹیگ->type) != 0);

        عدد_Operands = Get_Func_Type_عدد_Params(exn_ٹیگ->type);
    } وإلا إذا (not isThrowRef) {
        // a plain resume hands over the continuation's own parameters. An
        // aborted one never receives them, so they are unconstrained there.
        عدد_Operands = Get_Func_Type_عدد_Params(innerType);
    }

    u32 عدد_Clauses;
_   (ReadLEB_u32(&عدد_Clauses, &o->wasm, o->wasm_سرا));

    u32 clauseLabelDepths[16];
    _throwif("too many resume clauses", عدد_Clauses > 16);

    إذا (عدد_Clauses) {
        handlers = m3_AllocArray(M3ResumeHandler, عدد_Clauses);
        _throwifnull(handlers);
    }

    لكل (u32 i = 0; i < عدد_Clauses; ++i) {
        u8 قسم;
_       (Read_u8(&قسم, &o->wasm, o->wasm_سرا));
        _throwif(m3Err_wasmMalformed, قسم > 0x01);

        handlers[i].قسم = قسم;

        u32 ٹیگ_اشاریہ;
_       (ReadLEB_u32(&ٹیگ_اشاریہ, &o->wasm, o->wasm_سرا));
        _throwif(m3_Err_unknown_ٹیگ, ٹیگ_اشاریہ >= o->module->عدد_ٹیگ_سوچی);

        IM3_ٹیگ ٹیگ         = &o->module->ٹیگ_سوچی[ٹیگ_اشاریہ];
        handlers[i].ٹیگ    = ٹیگ->resolved ? ٹیگ->resolved : ٹیگ;
        handlers[i].stubPC = عدم;

        إذا (قسم == 0x00) {
_           (ReadLEB_u32(&clauseLabelDepths[i], &o->wasm, o->wasm_سرا));
        } وإلا {
            // (on $e switch): the tag carries no payload, and what the switch
            // site yields is what this resume yields
            _throwif(m3Err_typeMismatch, Get_Func_Type_عدد_Params(handlers[i].ٹیگ->type) != 0);
            clauseLabelDepths[i] = 0;
        }
    }

    // the operands are read out of slots, so nothing may still be in a register
_   (PreserveRegisters(o));

    m3type_t cont_چوٹی_Type = Get_ڈھیر_چوٹی_Type(o);
    _throwif(m3Err_typeMismatch, not Is_Sub_Type_دا(cont_چوٹی_Type, Ref_Type_دا_Func_Type(contFuncType, false)));
    u16 cont_تھاں = Get_ڈھیر_چوٹی_تھاں_نمبر(o);
_   (نکالو(o));

    u16 exn_تھاں = 0;

    إذا (isThrowRef) {
        m3type_t exnType = Get_ڈھیر_چوٹی_Type(o);
        _throwif(m3Err_typeMismatch, ادھار_Type_دا(exnType) != c_m3Type_exnref);
        exn_تھاں = Get_ڈھیر_چوٹی_تھاں_نمبر(o);
_       (نکالو(o));
    }

    u16 عملیہ_Slots[d_m3MaxContinuationPayload];
    _throwif("too many resume operands", عدد_Operands > d_m3MaxContinuationPayload);

    IM3FuncType عملیہ_Type = isThrow ? exn_ٹیگ->type : innerType;

    لكل (u16 i = 0; i < عدد_Operands; ++i) {
        u16      pIdx         = (u16)(عدد_Operands - 1 - i);
        m3type_t expectedType = GetFuncTypeParamType(عملیہ_Type, pIdx);
        m3type_t actualType   = Get_ڈھیر_چوٹی_Type(o);
        _throwif(m3Err_typeMismatch, not Is_Sub_Type_دا(actualType, expectedType));
        عملیہ_Slots[pIdx] = Get_ڈھیر_چوٹی_تھاں_نمبر(o);
_       (نکالو(o));
    }

    لكل (u32 i = 0; i < عدد_Clauses; ++i) {
        إذا (handlers[i].قسم != 0x00) {
            استمر;
        }

        IM3CompilationScope scope;
_       (Get_بلاک_Scope(o, &scope, clauseLabelDepths[i]));

        IM3_ٹیگ ٹیگ        = handlers[i].ٹیگ;
        u16    عدد_Payload = Get_Func_Type_عدد_Params(ٹیگ->type);

        m3type_t labelContType = (scope->opcode == c_waOp_loop)
                                   ? GetFuncTypeParamType(scope->type, عدد_Payload)
                                   : Get_Func_Type_نتیجہ_Type(scope->type, عدد_Payload);

        IM3_کوڈ_ورقہ stub_ورقہ;
_       (Acquire_Compilation_کوڈ_ورقہ(o, &stub_ورقہ));
        pc_t شروع_PC  = Get_ورقہ_PC(stub_ورقہ);
        displaced_ورقہ = o->ورقہ;
        o->ورقہ       = stub_ورقہ;

        u16 پہلا_اشاریہ = o->ڈھیر_اشاریہ;
        لكل (u16 a = 0; a < عدد_Payload; ++a) {
_           (ڈالو_Allocated_تھاں(o, GetFuncTypeParamType(ٹیگ->type, a)));
        }

        // The handler is handed the suspended continuation.
_       (ڈالو_Allocated_تھاں(o, labelContType));

_       (Ensure_کوڈ_ورقہ_عدد_سطریں(o, 2 * عدد_Payload + 5 + d_m3_کوڈ_ورقہ_Free_سطریں_حد));
_       (EmitOp(o, op_ResumePayload));
        Emit_اشارہ_گر(o, Module_Cont_Type_دا_Ref(o->module, labelContType));
        EmitConstant32(o, عدد_Payload);
        لكل (u16 a = 0; a < عدد_Payload; ++a) {
            u16 index = پہلا_اشاریہ + a;
            Emit_تھاں_وتھ(o, Get_تھاں_For_ڈھیر_اشاریہ(o, index));
            EmitConstant32(o, Is64_ثنائی_ہندسہ_Type(Get_ڈھیر_Type_From_Bottom(o, index)));
        }
        Emit_تھاں_وتھ(o, Get_تھاں_For_ڈھیر_اشاریہ(o, پہلا_اشاریہ + عدد_Payload));

#  إذا d_m3HasExceptionHandling
        // the clause leaves every try region between the resume and its label,
        // and leaving one retires its handlers, the way a branch does
_       (Emit_نکالو_Try_چوکھٹے(o, عدد_Try_چوکھٹے_منزل_نکالو(&o->بلاک, scope)));
#  endif

        إذا (scope->opcode == c_waOp_loop) {
_           (Resolve_بلاک_Results(o, scope, true));
_           (EmitOp(o, Op_ContinueLoop(o)));
            Emit_اشارہ_گر(o, scope->pc);
_           (ریکارڈ_پٹھ_کنارا(o, scope, false));
        } وإلا إذا (scope->ڈونگھائی == 0) {
_           (Return_اقدار(o, scope, true));
_           (EmitOp(o, op_Return));
        } وإلا {
_           (Resolve_بلاک_Results(o, scope, true));
_           (EmitPatchingBranch(o, scope));
        }

        IM3_کوڈ_ورقہ return_منزل = displaced_ورقہ;
        displaced_ورقہ        = عدم;

        Release_Compilation_کوڈ_ورقہ(o);
        o->ورقہ = return_منزل;

        handlers[i].stubPC = شروع_PC;

        طالما (o->ڈھیر_اشاریہ > پہلا_اشاریہ) {
_           (نکالو(o));
        }
    }

    u16 resSlots[d_m3MaxContinuationPayload];
    _throwif("too many cont results", عدد_Cont_Results > d_m3MaxContinuationPayload);
    لكل (u16 i = 0; i < عدد_Cont_Results; ++i) {
        m3type_t rtype = Get_Func_Type_نتیجہ_Type(innerType, i);
_       (ڈالو_Allocated_تھاں(o, rtype));
        resSlots[i] = Get_ڈھیر_چوٹی_تھاں_نمبر(o);
    }

_   (Ensure_کوڈ_ورقہ_عدد_سطریں(o, 8 + عدد_Operands * 2 + عدد_Clauses * 3 + عدد_Cont_Results * 2 +
                                  d_m3_کوڈ_ورقہ_Free_سطریں_حد));

_   (EmitOp(o, op_Resume));
    Emit_اشارہ_گر(o, contFuncType);
    Emit_تھاں_وتھ(o, cont_تھاں);
    EmitConstant32(o, isThrow ? d_m3ResumeThrow : (isThrowRef ? d_m3ResumeThrowRef : d_m3_Resume_عام));

    إذا (isThrowRef) {
        Emit_تھاں_وتھ(o, exn_تھاں);
    } وإلا {
        إذا (isThrow) {
            Emit_اشارہ_گر(o, exn_ٹیگ);
        }

        EmitConstant32(o, عدد_Operands);
        لكل (u16 i = 0; i < عدد_Operands; ++i) {
            Emit_تھاں_وتھ(o, عملیہ_Slots[i]);
            EmitConstant32(o, Is64_ثنائی_ہندسہ_Type(GetFuncTypeParamType(عملیہ_Type, i)));
        }
    }

    EmitConstant32(o, عدد_Clauses);
    لكل (u32 i = 0; i < عدد_Clauses; ++i) {
        EmitConstant32(o, handlers[i].قسم);
        Emit_اشارہ_گر(o, handlers[i].ٹیگ);
        Emit_اشارہ_گر(o, handlers[i].stubPC);
    }

    EmitConstant32(o, عدد_Cont_Results);
    لكل (u16 i = 0; i < عدد_Cont_Results; ++i) {
        Emit_تھاں_وتھ(o, resSlots[i]);
        EmitConstant32(o, Is64_ثنائی_ہندسہ_Type(Get_Func_Type_نتیجہ_Type(innerType, i)));
    }

    // a suspend passing through the resumed continuation leaves this frame
    // waiting here, with the continuation's results not yet taken
_   (ریکارڈ_محفوظ_نقطہ(o, safepoint_resume, 0, عدد_Cont_Results, (u16)عدد_Clauses, عدم));
#  إذا d_m3HasSnapshots
    إذا (o->snapshotMap and o->ورقہ) {
        o->snapshotMap->محفوظ_Points[o->snapshotMap->عدد_محفوظ_Points - 1].resumeType = Ref_Type_دا_Func_Type(contFuncType, true);
    }
#  endif

} _catch:
    إذا (displaced_ورقہ) {
        Release_Compilation_کوڈ_ورقہ(o);
        o->ورقہ = displaced_ورقہ;
    }
    m3_Free(handlers);
    أرجع نتیجہ;
}


ساكن
M3_نتیجہ Compile_Switch (IM3Compilation o, m3opcode_t i_opcode)
{
_try {
    u32 type_اشاریہ, ٹیگ_اشاریہ;
_   (ReadLEB_u32(&type_اشاریہ, &o->wasm, o->wasm_سرا));
_   (ReadLEB_u32(&ٹیگ_اشاریہ, &o->wasm, o->wasm_سرا));
    _throwif(m3_Err_type_گنتی_Mismatch, type_اشاریہ >= o->module->عدد_Func_Types);
    _throwif(m3_Err_unknown_ٹیگ, ٹیگ_اشاریہ >= o->module->عدد_ٹیگ_سوچی);

    IM3FuncType نشانہ_Cont_Type = o->module->funcTypes[type_اشاریہ];
    IM3_ٹیگ      ٹیگ            = &o->module->ٹیگ_سوچی[ٹیگ_اشاریہ];
    إذا (ٹیگ->resolved) {
        ٹیگ = ٹیگ->resolved;
    }

    _throwif("type is not a continuation", not نشانہ_Cont_Type->isContinuation);

    // a switch tag carries no payload of its own: the values travel as the
    // peer's arguments, and the tag's results are what the switch site yields
    _throwif(m3Err_typeMismatch, Get_Func_Type_عدد_Params(ٹیگ->type) != 0);

    // switch $ct1 : [t1* (ref null $ct1)] -> [t2*]
    //   where $ct1 = cont [t1* (ref null? $ct2)] -> [..]
    //   and   $ct2 = cont [t2*] -> [..]
    //
    // The trailing parameter of $ct1 is not an operand: the peer receives the
    // continuation this instruction suspends. And what this instruction yields
    // is not $ct1's results but $ct2's parameters - the values whoever comes
    // back this way will hand over.
    IM3FuncType نشانہ_Type = نشانہ_Cont_Type->contFuncType;
    u16         عدد_Params  = Get_Func_Type_عدد_Params(نشانہ_Type);

    _throwif("switch target takes no continuation", عدد_Params == 0);

    m3type_t    peerRefType  = GetFuncTypeParamType(نشانہ_Type, (u16)(عدد_Params - 1));
    IM3FuncType selfContType = Module_Cont_Type_دا_Ref(o->module, peerRefType);

    _throwif(m3Err_typeMismatch, not selfContType);

    IM3FuncType selfType   = selfContType->contFuncType;
    u16         عدد_دلیلاں    = (u16)(عدد_Params - 1);
    u16         عدد_Results = Get_Func_Type_عدد_Params(selfType);

    // the operands are read out of slots, so nothing may still be in a register
_   (PreserveRegisters(o));

    m3type_t cont_چوٹی_Type = Get_ڈھیر_چوٹی_Type(o);
    _throwif(m3Err_typeMismatch, not Is_Sub_Type_دا(cont_چوٹی_Type, Ref_Type_دا_Func_Type(نشانہ_Cont_Type, false)));
    u16 نشانہ_Cont_تھاں = Get_ڈھیر_چوٹی_تھاں_نمبر(o);
_   (نکالو(o));

    u16 argSlots[d_m3MaxContinuationPayload];
    _throwif("too many switch params", عدد_دلیلاں > d_m3MaxContinuationPayload);

    لكل (u16 i = 0; i < عدد_دلیلاں; ++i) {
        u16      pIdx         = (u16)(عدد_دلیلاں - 1 - i);
        m3type_t expectedType = GetFuncTypeParamType(نشانہ_Type, pIdx);
        m3type_t actualType   = Get_ڈھیر_چوٹی_Type(o);
        _throwif(m3Err_typeMismatch, not Is_Sub_Type_دا(actualType, expectedType));
        argSlots[pIdx] = Get_ڈھیر_چوٹی_تھاں_نمبر(o);
_       (نکالو(o));
    }

    u16 نتیجہ_Slots[d_m3MaxContinuationPayload];
    _throwif("too many switch results", عدد_Results > d_m3MaxContinuationPayload);

    لكل (u16 i = 0; i < عدد_Results; ++i) {
_       (ڈالو_Allocated_تھاں(o, GetFuncTypeParamType(selfType, i)));
        نتیجہ_Slots[i] = Get_ڈھیر_چوٹی_تھاں_نمبر(o);
    }

_   (Ensure_کوڈ_ورقہ_عدد_سطریں(o, 6 + عدد_دلیلاں * 2 + عدد_Results * 2 + d_m3_کوڈ_ورقہ_Free_سطریں_حد));
_   (EmitOp(o, op_Switch));
    Emit_اشارہ_گر(o, نشانہ_Cont_Type);
    Emit_اشارہ_گر(o, ٹیگ);
    Emit_تھاں_وتھ(o, نشانہ_Cont_تھاں);
    EmitConstant32(o, عدد_دلیلاں);
    لكل (u16 i = 0; i < عدد_دلیلاں; ++i) {
        Emit_تھاں_وتھ(o, argSlots[i]);
        EmitConstant32(o, Is64_ثنائی_ہندسہ_Type(GetFuncTypeParamType(نشانہ_Type, i)));
    }
    EmitConstant32(o, عدد_Results);
    لكل (u16 i = 0; i < عدد_Results; ++i) {
        Emit_تھاں_وتھ(o, نتیجہ_Slots[i]);
        EmitConstant32(o, Is64_ثنائی_ہندسہ_Type(GetFuncTypeParamType(selfType, i)));
    }

    // as with suspend: op_Switch clears the results before it leaves
_   (ریکارڈ_محفوظ_نقطہ(o, safepoint_suspend, 0, عدد_Results, 0, عدم));

} _catch: أرجع نتیجہ;
}

#جے_انت // d_m3HasStackSwitching


ساكن
M3_نتیجہ Compile_Else_بلاک (IM3Compilation o, pc_t* o_شروع_PC, IM3FuncType i_بلاک_Type)
{
    IM3_کوڈ_ورقہ saved_ورقہ = o->ورقہ;
_try {

    IM3_کوڈ_ورقہ else_ورقہ;
_   (Acquire_Compilation_کوڈ_ورقہ(o, &else_ورقہ));

    *o_شروع_PC = Get_ورقہ_PC(else_ورقہ);

    o->ورقہ = else_ورقہ;

_   (Compile_بلاک(o, i_بلاک_Type, c_waOp_else));

_   (EmitOp(o, op_Branch));
    Emit_اشارہ_گر(o, Get_ورقہ_PC(saved_ورقہ));
} _catch:

    إذا (o->ورقہ != saved_ورقہ) {
        Release_Compilation_کوڈ_ورقہ(o);
    }

    o->ورقہ = saved_ورقہ;

    أرجع نتیجہ;
}

ساكن
M3_نتیجہ Compile_If (IM3Compilation o, m3opcode_t i_opcode)
{
    /*      [   op_If   ]
            [ <else-pc> ]   ---->   [ ..else..  ]
            [  ..if..   ]           [ ..block.. ]
            [ ..block.. ]           [ op_Branch ]
            [    end    ]  <-----   [  <end-pc> ]       */

_try {

    // Spec: if condition must be i32
    إذا (not Is_ڈھیر_Polymorphic(o)) {
        m3type_t condType = Get_ڈھیر_چوٹی_Type(o);
        _throwif(m3Err_typeMismatch, condType != c_m3Type_none and condType != c_m3Type_i32);
    }

_   (Preserve_Non_چوٹی_Registers(o));
_   (Preserve_دلیلاں_And_Locals(o));

#إذا d_m3FuseBranch
    IM3Operation fuseOp = GetBranchFusionOp(o, true);

    إذا (fuseOp) {
        // absorb the if into the compare that produced the condition
        *(IM3Operation*)o->foldPatchPC = fuseOp;
        InvalidateFold(o);

_       (نکالو(o));                          // the condition is consumed inside the fused op
    } وإلا
#جے_انت
    {
        IM3Operation op = Is_ڈھیر_چوٹی_وچ_رجسٹر(o) ? op_If_r : op_If_s;

_       (EmitOp(o, op));
_       (Emit_تھاں_عدد_دا_ڈھیر_چوٹی_And_نکالو(o));
    }

    pc_t* pc = (pc_t*)Reserve_اشارہ_گر(o);

    IM3FuncType بلاک_Type = عدم;
_   (Read_بلاک_Type(o, &بلاک_Type));     //  dump_type_stack (o);

    u16 ڈھیر_اشاریہ = o->ڈھیر_اشاریہ;

_   (Compile_بلاک(o, بلاک_Type, i_opcode));

    إذا (o->پچھلا_Opcode == c_waOp_else) {
        o->ڈھیر_اشاریہ = ڈھیر_اشاریہ;
_       (Compile_Else_بلاک(o, pc, بلاک_Type));
    } وإلا {
        // if block produces values and there isn't a defined else
        // case, then we need to make one up so that the pass-through
        // results end up in the right place
        إذا (Get_Func_Type_عدد_Results(بلاک_Type)) {
            // rewind to the if's end to create a fake else block
            o->wasm--;
            o->ڈھیر_اشاریہ = ڈھیر_اشاریہ;     // dump_type_stack (o);

_           (Compile_Else_بلاک(o, pc, بلاک_Type));
        } وإلا {
            *pc = GetPC(o);
        }
    }

    } _catch: أرجع نتیجہ;
}

ساكن
M3_نتیجہ Compile_Select (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_نتیجہ نتیجہ = m3Err_none;

    u16 slots[3] = { c_تھاں_بن_ورتیا, c_تھاں_بن_ورتیا, c_تھاں_بن_ورتیا };

    IM3Operation op = عدم;

    m3type_t type = Get_ڈھیر_Type_From_چوٹی(o, 1); // get type of selection

    إذا (not Is_ڈھیر_Polymorphic(o)) {
        // Spec: the condition operand (top) must be i32
        m3type_t condType = Get_ڈھیر_Type_From_چوٹی(o, 0);
        _throwif(m3Err_typeMismatch, condType != c_m3Type_none and condType != c_m3Type_i32);

        // Spec: the two value operands (below condition) must have matching types
        m3type_t type2 = Get_ڈھیر_Type_From_چوٹی(o, 2);
        _throwif(m3Err_typeMismatch, type != c_m3Type_none and type2 != c_m3Type_none and
                                       not (Is_Sub_Type_دا(type, type2) or Is_Sub_Type_دا(type2, type)));
    }

    إذا (IsFpType(type)) {
#إذا d_m3HasFloat
        // not consuming a fp reg, so preserve
        إذا (not Is_ڈھیر_چوٹی_Minus1_وچ_رجسٹر(o) and
            not Is_ڈھیر_چوٹی_Minus2_وچ_رجسٹر(o)) {
_           (Preserve_رجسٹر_If_Occupied(o, type));
        }

        bool selector_وچ_Reg = Is_ڈھیر_چوٹی_وچ_رجسٹر(o);
        slots[0]           = Get_ڈھیر_چوٹی_تھاں_نمبر(o);
_       (نکالو(o));

        u32 op_اشاریہ = 0;

        لكل (u32 i = 1; i <= 2; ++i) {
            إذا (Is_ڈھیر_چوٹی_وچ_رجسٹر(o)) {
                op_اشاریہ = i;
            } وإلا {
                slots[i] = Get_ڈھیر_چوٹی_تھاں_نمبر(o);
            }

_           (نکالو(o));
        }

        op = c_fpSelectOps[type - c_m3Type_f32][selector_وچ_Reg][op_اشاریہ];
#وإلا
        _throw(m3Err_unknownOpcode);
#جے_انت
    } وإلا إذا (IsIntType(type) or IsRefType(type)) {
        // 'sss' operation doesn't consume a register, so might have to protected its contents
        إذا (not Is_ڈھیر_چوٹی_وچ_رجسٹر(o) and
            not Is_ڈھیر_چوٹی_Minus1_وچ_رجسٹر(o) and
            not Is_ڈھیر_چوٹی_Minus2_وچ_رجسٹر(o)) {
_           (Preserve_رجسٹر_If_Occupied(o, type));
        }

        u32 op_اشاریہ = 3;  // op_Select_*_sss

        لكل (u32 i = 0; i < 3; ++i) {
            إذا (Is_ڈھیر_چوٹی_وچ_رجسٹر(o)) {
                op_اشاریہ = i;
            } وإلا {
                slots[i] = Get_ڈھیر_چوٹی_تھاں_نمبر(o);
            }

_           (نکالو(o));
        }

        // a reference is a pointer-sized integer as far as select is concerned
        u32 type_اشاریہ = IsRefType(type) ? ((M3_SIZEOF_PTR == 8) ? 1 : 0)
                                        : (u32)(type - c_m3Type_i32);

        op = c_intSelectOps[type_اشاریہ][op_اشاریہ];
    } وإلا إذا (not Is_ڈھیر_Polymorphic(o)) {
        _throw(m3_Err_function_ڈھیر_Underrun);
    }

    EmitOp(o, op);
    لكل (u32 i = 0; i < 3; i++) {
        إذا (Is_Valid_تھاں(slots[i])) {
            Emit_تھاں_وتھ(o, slots[i]);
        }
    }
_   (ڈالو_رجسٹر(o, type));

    _catch: أرجع نتیجہ;
}

ساكن
M3_نتیجہ Compile_Drop (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_نتیجہ نتیجہ = نکالو(o);
    إذا (d_m3_Log_Wasm_ڈھیر) {
        dump_type_ڈھیر(o);
    }
    أرجع نتیجہ;
}

ساكن
M3_نتیجہ Compile_Nop (IM3Compilation o, m3opcode_t i_opcode)
{
    أرجع m3Err_none;
}

ساكن
M3_نتیجہ Compile_Unreachable (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_نتیجہ نتیجہ;

_   (جوڑو_Trap_ریکارڈ(o));

_   (EmitOp(o, op_Unreachable));
_   (سیٹ_ڈھیر_Polymorphic(o));

    _catch:
    أرجع نتیجہ;
}


// OPTZ: currently all stack slot indices take up a full word, but
// dual stack source operands could be packed together
ساكن
M3_نتیجہ Compile_Operator (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_نتیجہ نتیجہ;

    // Declared before the first throw below, which jumps past this point to
    // _catch and so must not skip an initialization.
    IM3Operation op = عدم;

    IM3OpInfo opInfo = GetOpInfo(i_opcode);
    _throwif(m3Err_unknownOpcode, not opInfo);

    // Spec: validate operand types for load/store operations
    إذا (not Is_ڈھیر_Polymorphic(o)) {
        // For load ops (stackOffset == 0, unary), the operand is always i32 (address)
        إذا (i_opcode >= 0x28 and i_opcode <= 0x35) {
            m3type_t چوٹی_Type = Get_ڈھیر_چوٹی_Type(o);
            _throwif(m3Err_typeMismatch, چوٹی_Type != c_m3Type_none and چوٹی_Type != c_m3Type_i32);
        }

        // For store ops (stackOffset == -2), the address operand must be i32
        إذا (i_opcode >= 0x36 and i_opcode <= 0x3e) {
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
    إذا (M3_UNLIKELY(IsFpType(Get_ڈھیر_چوٹی_Type(o)) and IsIntType(opInfo->type))) {
_       (Preserve_رجسٹر_If_Occupied(o, opInfo->type));
    }

    إذا (opInfo->ڈھیر_وتھ == 0) {
        إذا (Is_ڈھیر_چوٹی_وچ_رجسٹر(o)) {
            op = opInfo->operations[0]; // _s
        } وإلا {
_           (Preserve_رجسٹر_If_Occupied(o, opInfo->type));
            op = opInfo->operations[1]; // _r
        }
    } وإلا {
        إذا (Is_ڈھیر_چوٹی_وچ_رجسٹر(o)) {
            op = opInfo->operations[0];  // _rs

            إذا (Is_ڈھیر_چوٹی_Minus1_وچ_رجسٹر(o)) {    d_m3Assert (i_opcode == c_waOp_store_f32 or i_opcode == c_waOp_store_f64);
                op = opInfo->operations[3]; // _rr for fp.store
            }
        } وإلا إذا (Is_ڈھیر_چوٹی_Minus1_وچ_رجسٹر(o)) {
            op = opInfo->operations[1]; // _sr

            إذا (not op) {  // must be commutative, then
                op = opInfo->operations[0];
            }
        } وإلا {
_           (Preserve_رجسٹر_If_Occupied(o, opInfo->type));     // _ss
            op = opInfo->operations[2];
        }
    }

    إذا (M3_LIKELY(op)) {
_       (EmitOp(o, op));

#إذا d_m3_Fold_سیٹ_Local
        // the op word just written (EmitOp may have bridged pages); NULL in the page-less compile-walk mode
        pc_t patchPC = o->ورقہ ? GetPC(o) - 1 : عدم;
#جے_انت

_       (Emit_تھاں_عدد_دا_ڈھیر_چوٹی_And_نکالو(o));

        إذا (opInfo->ڈھیر_وتھ < 0) {
_           (Emit_تھاں_عدد_دا_ڈھیر_چوٹی_And_نکالو(o));
        }

        إذا (opInfo->type != c_m3Type_none) {
_           (ڈالو_رجسٹر(o, opInfo->type));

#إذا d_m3_Fold_سیٹ_Local
            // record this op as a fold candidate for an immediately following local.set;
            // the operand-form index is recovered by matching against the variant table
            إذا (patchPC) {
                لكل (u8 form = 0; form < 4; ++form) {
                    إذا (opInfo->operations[form] == op) {
                        o->foldPatchPC    = patchPC;
                        o->foldOpcode     = i_opcode;
                        o->foldForm       = form;
                        o->fold_ڈھیر_اشاریہ = (u16)Get_ڈھیر_چوٹی_اشاریہ(o);
                        اكسر;
                    }
                }
            }
#جے_انت
        }
    } وإلا {
#جے_تعریف_شدہ DEBUG
        نتیجہ = ErrorCompile("no operation found for opcode", o, "'%s'", opInfo->name);
#وإلا
        نتیجہ = ErrorCompile("no operation found for opcode", o, "%x", i_opcode);
#جے_انت
        _throw(نتیجہ);
    }

    _catch: أرجع نتیجہ;
}

ساكن
M3_نتیجہ Compile_Convert (IM3Compilation o, m3opcode_t i_opcode)
{
_try {
    IM3OpInfo opInfo = GetOpInfo(i_opcode);
    _throwif(m3Err_unknownOpcode, not opInfo);

    // Spec: validate source operand type for conversion instructions
    إذا (not Is_ڈھیر_Polymorphic(o)) {
        u8 سروت_Type = c_m3Type_none;
        بدل (i_opcode)
        // clang-format off
        {
        حالة 0xa7:              // i32.wrap/i64
            سروت_Type = c_m3Type_i64; اكسر;
        حالة 0xa8: حالة 0xa9:   // i32.trunc_s/f32, i32.trunc_u/f32
        حالة 0xae: حالة 0xaf:   // i64.trunc_s/f32, i64.trunc_u/f32
        حالة 0xbb:              // f64.promote/f32
        حالة 0xbc:              // i32.reinterpret/f32
            سروت_Type = c_m3Type_f32; اكسر;
        حالة 0xaa: حالة 0xab:   // i32.trunc_s/f64, i32.trunc_u/f64
        حالة 0xb0: حالة 0xb1:   // i64.trunc_s/f64, i64.trunc_u/f64
        حالة 0xb6:              // f32.demote/f64
        حالة 0xbd:              // i64.reinterpret/f64
            سروت_Type = c_m3Type_f64; اكسر;
        حالة 0xac: حالة 0xad:   // i64.extend_s/i32, i64.extend_u/i32
        حالة 0xb2: حالة 0xb3:   // f32.convert_s/i32, f32.convert_u/i32
        حالة 0xb7: حالة 0xb8:   // f64.convert_s/i32, f64.convert_u/i32
        حالة 0xbe:              // f32.reinterpret/i32
            سروت_Type = c_m3Type_i32; اكسر;
        حالة 0xb4: حالة 0xb5:   // f32.convert_s/i64, f32.convert_u/i64
        حالة 0xb9: حالة 0xba:   // f64.convert_s/i64, f64.convert_u/i64
        حالة 0xbf:              // f64.reinterpret/i64
            سروت_Type = c_m3Type_i64; اكسر;
        افتراضي: اكسر;
        }
        // clang-format on

        إذا (سروت_Type != c_m3Type_none) {
            m3type_t چوٹی_Type = Get_ڈھیر_چوٹی_Type(o);
            _throwif(m3Err_typeMismatch, چوٹی_Type != c_m3Type_none and not Is_Sub_Type_دا(چوٹی_Type, سروت_Type));
        }
    }

    bool dest_وچ_تھاں   = Is_رجسٹر_Type_Allocated(o, opInfo->type);
    bool سروت_وچ_تھاں = Is_ڈھیر_چوٹی_وچ_تھاں(o);

    IM3Operation op = opInfo->operations[dest_وچ_تھاں * 2 + سروت_وچ_تھاں];

_   (EmitOp(o, op));
_   (Emit_تھاں_عدد_دا_ڈھیر_چوٹی_And_نکالو(o));

    إذا (dest_وچ_تھاں) {
_       (ڈالو_Allocated_تھاں_And_Emit_2(o, opInfo->type))
    } وإلا {
_       (ڈالو_رجسٹر(o, opInfo->type))
    }
}
    _catch: أرجع نتیجہ;
}

#إذا d_m3HasWideArithmetic

// The wide arithmetic proposal: i64.add128 and i64.sub128 over two 128-bit values
// given as i64 halves, and i64.mul_wide_s/u giving the full product of two i64s.
// Each gives back a 128-bit result the same way, low half first. The operands are
// all read from slots, and of the two results the high half, which ends up on top,
// goes to _r0 and the low half to a newly allocated slot - so every operation has
// the one form, whatever shape the stack was in.
ساكن
M3_نتیجہ Compile_WideArithmetic (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_نتیجہ نتیجہ = m3Err_none;

    IM3Operation op          = op_i64_Add128;
    u32          عدد_Operands = 4;

    بدل (i_opcode)
    // clang-format off
    {
    حالة c_waOp_i64Sub128:   op = op_i64_Sub128;                          اكسر;
    حالة c_waOp_i64MulWideS: op = op_i64_MultiplyWide; عدد_Operands = 2;   اكسر;
    حالة c_waOp_i64MulWideU: op = op_u64_MultiplyWide; عدد_Operands = 2;   اكسر;
    افتراضي: اكسر;
    }
    // clang-format on

_   (Preserve_رجسٹر_If_Occupied(o, c_m3Type_i64));

_   (EmitOp(o, op));

    // the top of the stack first: the high half of the second operand, for the
    // 128-bit ones
    لكل (u32 i = 0; i < عدد_Operands; ++i) {
_       (Emit_تھاں_عدد_دا_ڈھیر_چوٹی_And_نکالو(o));
    }

_   (ڈالو_Allocated_تھاں_And_Emit_2(o, c_m3Type_i64));      // the low half
_   (ڈالو_رجسٹر(o, c_m3Type_i64));                  // the high half

    _catch: أرجع نتیجہ;
}

#جے_انت // d_m3HasWideArithmetic

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
M3_نتیجہ EmitCheckAddr64 (IM3Compilation o, u64 i_وتھ, u16 i_عدد_Operands)
{
    M3_نتیجہ نتیجہ = m3Err_none;

    // The address has to be in a slot for op_CheckAddr64 to read it. Spilling
    // r0 wholesale also keeps the value of a store out of it, which is what
    // leaves the address as the one operand still in a register to worry about.
_   (Preserve_رجسٹر_If_Occupied(o, c_m3Type_i64));

    // unreachable code: the operands the instruction names may not be there
    إذا (o->ڈھیر_اشاریہ < o->بلاک.بلاک_ڈھیر_اشاریہ + i_عدد_Operands) {
        أرجع نتیجہ;
    }

    {
        u16      ڈھیر_اشاریہ = (u16)(o->ڈھیر_اشاریہ - i_عدد_Operands);
        u16      src_تھاں    = o->wasm_ڈھیر[ڈھیر_اشاریہ];
        m3type_t srcType    = o->type_ڈھیر[ڈھیر_اشاریہ];

        u16 dst_تھاں = c_تھاں_بن_ورتیا;

        // Checked here rather than left to the validator, which a build may
        // have switched off: reading a slot as an i64 that only holds an i32
        // would reach past what was allocated for it.
        _throwif(m3Err_typeMismatch, ادھار_Type_دا(srcType) != c_m3Type_i64);

        // an address is an integer, so nothing should be left in a register
        // once r0 has been preserved
        _throwif(m3Err_typeMismatch, Is_رجسٹر_تھاں_Alias(src_تھاں));
        _throwif(m3_Err_function_ڈھیر_Underrun, src_تھاں >= o->تھاں_Max_Allocated_اشاریہ_Plus_One);

        // allocated before the source is released, so the two cannot be the
        // same slot - op_CheckAddr64 reads one and writes the other
_       (AllocateSlots(o, &dst_تھاں, c_m3Type_i32));

_       (EmitOp(o, op_CheckAddr64));
        Emit_تھاں_وتھ(o, src_تھاں);
        Emit_تھاں_وتھ(o, dst_تھاں);
        EmitConstant64(o, i_وتھ);

        o->wasm_ڈھیر[ڈھیر_اشاریہ] = dst_تھاں;
        o->type_ڈھیر[ڈھیر_اشاریہ] = c_m3Type_i32;

        // locals and the constant table outlive the instruction; a dynamic
        // slot the address was computed into does not
        إذا (src_تھاں >= o->تھاں_پہلا_Dynamic_اشاریہ) {
            Deallocate_تھاں(o, src_تھاں, srcType);
        }
    }

    _catch: أرجع نتیجہ;
}

#جے_انت // d_m3HasMemory64


ساكن
M3_نتیجہ Compile_لدو_Store (IM3Compilation o, m3opcode_t i_opcode)
{
_try {
    u32 alignHint, حافظہ_Idx;
    u64 حافظہ_وتھ;

    // alignHint is checked by the validator
_   (Read_حافظہ_Arg(&alignHint, &حافظہ_Idx, &حافظہ_وتھ, &o->wasm, o->wasm_سرا));
                                                                        m3log (compile, d_indent " (memory = %d; offset = %llu)", get_indention_string (o), حافظہ_Idx, (بنانشانی لما لما) حافظہ_وتھ);
    _throwif(m3_Err_unknown_حافظہ, حافظہ_Idx >= o->module->عدد_Memories);

    IM3OpInfo opInfo = GetOpInfo(i_opcode);
    _throwif(m3Err_unknownOpcode, not opInfo);

    bool isMemory64 = o->module->memories[حافظہ_Idx]->isMemory64;

    // Spec: the static offset has to be in range of the address type. Checked
    // here as well as in the validator, so that a build without one still
    // refuses the module rather than silently truncating the offset.
    _throwif(m3Err_wasmMalformed, not isMemory64 and حافظہ_وتھ > 0xFFFFFFFFull);

    إذا (IsFpType(opInfo->type)) {
_       (Preserve_رجسٹر_If_Occupied(o, c_m3Type_f64));
    }

    إذا (حافظہ_Idx) {
_       (Emit_سیٹ_حافظہ(o, حافظہ_Idx));
    }

#إذا d_m3HasMemory64
    إذا (isMemory64) {
        // An offset this far out puts every address out of bounds, so rather
        // than carrying it, clamp it to the limit: op_CheckAddr64 then traps
        // on whatever address it is given, which is the outcome either way.
        إذا (حافظہ_وتھ > d_m3_پتہ_حد_بندی) {
            حافظہ_وتھ = d_m3_پتہ_حد_بندی;
        }

_       (EmitCheckAddr64(o, حافظہ_وتھ, opInfo->ڈھیر_وتھ < 0 ? 2 : 1));

        // op_CheckAddr64 has folded the offset in already
        حافظہ_وتھ = 0;
    }
#جے_انت

_   (Compile_Operator(o, i_opcode));

    EmitConstant32(o, (u32)حافظہ_وتھ);

    إذا (حافظہ_Idx) {
_       (Emit_سیٹ_حافظہ(o, 0));
    }
}
    _catch: أرجع نتیجہ;
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
فراغ GetAtomicShape (u32 i_sub, u16* o_عدد_Operands, m3type_t* o_چوٹی_Type, m3type_t* o_نتیجہ_Type)
{
    // clang-format off
    ساكن ثابت m3type_t c_قدر_Types [7] = {
        c_m3Type_i32, c_m3Type_i64,
        c_m3Type_i32, c_m3Type_i32,
        c_m3Type_i64, c_m3Type_i64, c_m3Type_i64
    };
    // clang-format on

    إذا (i_sub == 0x00) {                    // memory.atomic.notify: address, count
        *o_عدد_Operands = 2;
        *o_چوٹی_Type     = c_m3Type_i32;
        *o_نتیجہ_Type  = c_m3Type_i32;
    } وإلا إذا (i_sub <= 0x02) {             // memory.atomic.wait32/64: address, expected, timeout
        *o_عدد_Operands = 3;
        *o_چوٹی_Type     = c_m3Type_i64;
        *o_نتیجہ_Type  = c_m3Type_i32;
    } وإلا إذا (i_sub <= 0x16) {             // loads: address
        *o_عدد_Operands = 1;
        *o_چوٹی_Type     = c_m3Type_i32;
        *o_نتیجہ_Type  = c_قدر_Types[i_sub - 0x10];
    } وإلا إذا (i_sub <= 0x1d) {             // stores: address, value
        *o_عدد_Operands = 2;
        *o_چوٹی_Type     = c_قدر_Types[i_sub - 0x17];
        *o_نتیجہ_Type  = c_m3Type_none;
    } وإلا إذا (i_sub <= 0x47) {             // read-modify-write: address, value
        *o_عدد_Operands = 2;
        *o_چوٹی_Type     = c_قدر_Types[(i_sub - 0x1e) % 7];
        *o_نتیجہ_Type  = *o_چوٹی_Type;
    } وإلا {                                // cmpxchg: address, expected, replacement
        *o_عدد_Operands = 3;
        *o_چوٹی_Type     = c_قدر_Types[i_sub - 0x48];
        *o_نتیجہ_Type  = *o_چوٹی_Type;
    }
}


// The description an access takes as its last immediate: see m3_exec_atomic.h
ساكن
u32 GetAtomicDesc (u32 i_sub)
{
    // per variant, the log2 of the width of the cell
    ساكن ثابت u8 c_log2Widths[7] = { 2, 3, 0, 1, 0, 1, 2 };

    u32 variant, قسم = 0;

    إذا (i_sub <= 0x16) {
        variant = i_sub - 0x10;
    } وإلا إذا (i_sub <= 0x1d) {
        variant = i_sub - 0x17;
    } وإلا إذا (i_sub <= 0x47) {
        variant = (i_sub - 0x1e) % 7;
        قسم    = (i_sub - 0x1e) / 7;         // add, sub, and, or, xor, xchg
    } وإلا {
        variant = i_sub - 0x48;
    }

    // variants 1 and 4..6 are the i64 ones
    u32 is64 = (variant == 1 or variant >= 4) ? 1u : 0u;

    أرجع c_log2Widths[variant] | (is64 << 2) | (قسم << 3);
}


// Every atomic access is one op, whose operands are the topmost in _r0 and the rest
// in slots, topmost first. The address is always the deepest, so it is the last slot
// (or, for a load, the register), followed by the memarg offset. The ops check the
// address themselves: for an atomic that includes its alignment, which only the
// effective address can be asked.
ساكن
M3_نتیجہ Compile_Atomic (IM3Compilation o, m3opcode_t i_opcode)
{
_try {
    u32 alignHint, حافظہ_Idx;
    u64 حافظہ_وتھ;

    u16      عدد_Operands = 0;
    m3type_t چوٹی_Type     = c_m3Type_none;
    m3type_t نتیجہ_Type  = c_m3Type_none;

    إذا (i_opcode == c_waOp_atomicFence) {
        // the reserved byte, which the validator has checked to be zero. There
        // is nothing to order between accesses that one thread makes in sequence.
        u8 reserved;
_       (Read_u8(&reserved, &o->wasm, o->wasm_سرا));
    } وإلا {
        // alignHint is checked by the validator
_       (Read_حافظہ_Arg(&alignHint, &حافظہ_Idx, &حافظہ_وتھ, &o->wasm, o->wasm_سرا));
                                                                        m3log (compile, d_indent " (memory = %d; offset = %llu)", get_indention_string (o), حافظہ_Idx, (بنانشانی لما لما) حافظہ_وتھ);
        _throwif(m3_Err_unknown_حافظہ, حافظہ_Idx >= o->module->عدد_Memories);

        IM3OpInfo opInfo = GetOpInfo(i_opcode);
        _throwif(m3Err_unknownOpcode, not opInfo);

        bool isMemory64 = o->module->memories[حافظہ_Idx]->isMemory64;

        // see Compile_Load_Store
        _throwif(m3Err_wasmMalformed, not isMemory64 and حافظہ_وتھ > 0xFFFFFFFFull);

        GetAtomicShape(i_opcode & 0xFF, &عدد_Operands, &چوٹی_Type, &نتیجہ_Type);

        إذا (حافظہ_Idx) {
_           (Emit_سیٹ_حافظہ(o, حافظہ_Idx));
        }

#  إذا d_m3HasMemory64
        إذا (isMemory64) {
            // see Compile_Load_Store
            إذا (حافظہ_وتھ > d_m3_پتہ_حد_بندی) {
                حافظہ_وتھ = d_m3_پتہ_حد_بندی;
            }

_           (EmitCheckAddr64(o, حافظہ_وتھ, عدد_Operands));
            حافظہ_وتھ = 0;
        }
#  endif

_       (Copy_ڈھیر_چوٹی_منزل_رجسٹر(o, false));
_       (EmitOp(o, opInfo->operations[0]));
_       (نکالو_Type(o, چوٹی_Type));

        لكل (u16 i = 1; i < عدد_Operands; ++i) {
_           (Emit_تھاں_عدد_دا_ڈھیر_چوٹی_And_نکالو(o));
        }

        EmitConstant32(o, (u32)حافظہ_وتھ);
        إذا (i_opcode >= c_wa_Op_atomic_پہلا_Access) {
            EmitConstant32(o, GetAtomicDesc(i_opcode & 0xFF));
        }

        إذا (حافظہ_Idx) {
_           (Emit_سیٹ_حافظہ(o, 0));
        }

        إذا (نتیجہ_Type != c_m3Type_none) {
_           (ڈالو_رجسٹر(o, نتیجہ_Type));
        }
    }
}
    _catch: أرجع نتیجہ;
}

#جے_انت // d_m3HasAtomics


M3_نتیجہ CompileRawFunction (IM3Module io_module, IM3Function io_function, ثابت فراغ* i_function, ثابت فراغ* i_userdata)
{
    d_m3Assert(io_module->runtime);

    IM3_کوڈ_ورقہ ورقہ = Acquire_کوڈ_ورقہ_With_سمرتھا(io_module->runtime, 4);

    إذا (ورقہ) {
        io_function->compiled = Get_ورقہ_PC(ورقہ);
        io_function->module   = io_module;

        // Unless a host module's ABI names a memory (m3_BindImportMemory), a
        // host function addresses memory 0 - what a bare i32 guest pointer means
        io_function->host_حافظہ = io_module->memory0;

        Emit_لفظ(ورقہ, op_CallRawFunction);
        Emit_لفظ(ورقہ, i_function);
        Emit_لفظ(ورقہ, io_function);
        Emit_لفظ(ورقہ, i_userdata);

        Release_کوڈ_ورقہ(io_module->runtime, ورقہ);
        أرجع m3Err_none;
    } وإلا {
        أرجع m3_Err_malloc_Failed_کوڈ_ورقہ;
    }
}


// A table entry names every field it sets, so the struct can order them for
// packing; the columns follow that order, since M3OP names them positionally.
//
// d_logOp, d_logOp2 macros aren't actually used by the compiler, just codepage decoding (d_m3LogCodePages = 1)
#تعریف_کرو d_ops(...)                          .operations = { __VA_ARGS__ }

#تعریف_کرو d_logOp(OP)                         d_ops( op_##OP,                 عدم,                       عدم,                       عدم )
#تعریف_کرو d_logOp2(OP1,OP2)                   d_ops( op_##OP1,                op_##OP2,                   عدم,                       عدم )

#تعریف_کرو d_خالی_تھاواں_Op_List                       d_ops( عدم,                    عدم,                       عدم,                       عدم )
#تعریف_کرو d_unaryOpList(TYPE, NAME)           d_ops( op_##TYPE##_##NAME##_r,  op_##TYPE##_##NAME##_s,     عدم,                       عدم )
#تعریف_کرو d_binOpList(TYPE, NAME)             d_ops( op_##TYPE##_##NAME##_rs, op_##TYPE##_##NAME##_sr,    op_##TYPE##_##NAME##_ss,    عدم )
#تعریف_کرو d_storeFpOpList(TYPE, NAME)         d_ops( op_##TYPE##_##NAME##_rs, op_##TYPE##_##NAME##_sr,    op_##TYPE##_##NAME##_ss,    op_##TYPE##_##NAME##_rr )
#تعریف_کرو d_commutativeBinOpList(TYPE, NAME)  d_ops( op_##TYPE##_##NAME##_rs, عدم,                       op_##TYPE##_##NAME##_ss,    عدم )

#تعریف_کرو d_convertOpList(OP)                 d_ops( op_##OP##_r_r,           op_##OP##_r_s,              op_##OP##_s_r,              op_##OP##_s_s )


// Every M3Compiler the tables below name, as one list, so that the index enum and
// the array of pointers can never drift apart. Each entry must carry the same
// config guard as the function's definition.
#تعریف_کرو d_m3CompilerList(_)             \
    _( Compile_Unreachable )            \
    _( Compile_Nop )                    \
    _( Compile_Loop_Or_بلاک )            \
    _( Compile_If )                     \
    _( Compile_سرا )                    \
    _( Compile_Branch )                 \
    _( Compile_Branch_جدول )            \
    _( Compile_Return )                 \
    _( Compile_Call )                   \
    _( Compile_Call_اسدھا )           \
    _( Compile_Drop )                   \
    _( Compile_Select )                 \
    _( Compile_GetLocal )               \
    _( Compile_سیٹ_Local )               \
    _( Compile_Get_سیٹ_Global )           \
    _( Compile_لدو_Store )             \
    _( Compile_حافظہ_ناپ )            \
    _( Compile_حافظہ_Grow )            \
    _( Compile_حافظہ_Init )            \
    _( Compile_حافظہ_Copy_Fill )        \
    _( Compile_انکڑے_Drop )              \
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

#إذا d_m3ImplementFloat
#  define d_m3CompilerList_f(_)         \
    _( Compile_Const_f32 )              \
    _( Compile_Const_f64 )
#وإلا
#  define d_m3CompilerList_f(_)
#جے_انت

#إذا d_m3HasRefTypes
#  define d_m3CompilerList_refTypes(_)  \
    _( Compile_Select_Typed )           \
    _( Compile_Ref_Null )               \
    _( Compile_Ref_IsNull )             \
    _( Compile_Ref_Func )               \
    _( Compile_جدول_Get_سیٹ )           \
    _( Compile_جدول_Init )             \
    _( Compile_جدول_Copy )             \
    _( Compile_جدول_ناپ )             \
    _( Compile_جدول_Grow_Fill )         \
    _( Compile_Elem_Drop )
#وإلا
#  define d_m3CompilerList_refTypes(_)
#جے_انت

#إذا d_m3HasTypedRefs
#  define d_m3CompilerList_typedRefs(_) \
    _( Compile_CallRef )                \
    _( Compile_Ref_AsNonNull )
#وإلا
#  define d_m3CompilerList_typedRefs(_)
#جے_انت

#إذا d_m3HasExceptionHandling
#  define d_m3CompilerList_eh(_)        \
    _( Compile_Throw )                  \
    _( Compile_ThrowRef )               \
    _( Compile_Try_جدول )
#وإلا
#  define d_m3CompilerList_eh(_)
#جے_انت

#إذا d_m3HasAtomics
#  define d_m3CompilerList_atomics(_)   \
    _( Compile_Atomic )
#وإلا
#  define d_m3CompilerList_atomics(_)
#جے_انت

#إذا d_m3HasWideArithmetic
#  define d_m3CompilerList_wide(_)      \
    _( Compile_WideArithmetic )
#وإلا
#  define d_m3CompilerList_wide(_)
#جے_انت

#إذا d_m3_Has_ڈھیر_Switching
#  define d_m3_Compiler_List_ڈھیر_Switching(_) \
    _( Compile_ContNew )                     \
    _( Compile_ContBind )                    \
    _( Compile_Suspend )                     \
    _( Compile_Resume )                      \
    _( Compile_Switch )
#وإلا
#  define d_m3_Compiler_List_ڈھیر_Switching(_)
#جے_انت

// Index 0 is "this opcode has no compiler", so that a zeroed entry - which is
// what M3OP_RESERVED and the reserved holes in the tables are - reads as one.
تعداد {
    c_cc_NULL = 0,
#تعریف_کرو d_m3CompilerEnum(NAME)  c_cc_##NAME,
    d_m3CompilerList(d_m3CompilerEnum)
#تعریف_رد d_m3CompilerEnum
};

ساكن ثابت M3Compiler c_compilers[] = {
    عدم,
#تعریف_کرو d_m3CompilerPtr(NAME)   NAME,
    d_m3CompilerList(d_m3CompilerPtr)
#تعریف_رد d_m3CompilerPtr
};

// Names a table entry's compiler by index. d_cc (NULL) is the no-compiler entry.
#تعریف_کرو d_cc(NAME)                          .compiler = c_cc_##NAME

ساكن
M3Compiler GetCompiler (IM3OpInfo i_opInfo)
{
    d_m3Assert(i_opInfo->compiler < M3_گنتی_دا(c_compilers));
    أرجع c_compilers[i_opInfo->compiler];
}

// clang-format off
ثابت M3OpInfo c_operations[] =
{
    M3OP( "unreachable",           0,  none,   d_cc(Compile_Unreachable),  d_logOp (Unreachable) ),                   // 0x00
    M3OP( "nop",                   0,  none,   d_cc(Compile_Nop),          d_خالی_تھاواں_Op_List ),                           // 0x01 .
    M3OP( "block",                 0,  none,   d_cc(Compile_Loop_Or_بلاک),  d_خالی_تھاواں_Op_List ),                           // 0x02
    M3OP( "loop",                  0,  none,   d_cc(Compile_Loop_Or_بلاک),  d_logOp (Loop) ),                          // 0x03
    M3OP( "if",                    -1, none,   d_cc(Compile_If),           d_خالی_تھاواں_Op_List ),                           // 0x04
    M3OP( "else",                  0,  none,   d_cc(Compile_Nop),          d_خالی_تھاواں_Op_List ),                           // 0x05

    M3OP_RESERVED,  M3OP_RESERVED,                                                                                    // 0x06...0x07

#إذا d_m3HasExceptionHandling
    M3OP( "throw",                 0,  none,   d_cc(Compile_Throw),        d_logOp (Throw) ),                         // 0x08
    M3OP_RESERVED,                                                                                                    // 0x09
    M3OP( "throw_ref",             0,  none,   d_cc(Compile_ThrowRef),     d_logOp (ThrowRef) ),                      // 0x0a
#وإلا
    M3OP_RESERVED,  M3OP_RESERVED, M3OP_RESERVED,                                                                     // 0x08...0x0a
#جے_انت

    M3OP( "end",                   0,  none,   d_cc(Compile_سرا),          d_خالی_تھاواں_Op_List ),                           // 0x0b
    M3OP( "br",                    0,  none,   d_cc(Compile_Branch),       d_logOp (Branch) ),                        // 0x0c
    M3OP( "br_if",                 -1, none,   d_cc(Compile_Branch),       d_logOp2 (BranchIf_r, BranchIf_s) ),       // 0x0d
    M3OP( "br_table",              -1, none,   d_cc(Compile_Branch_جدول),  d_logOp (Branch_جدول) ),                   // 0x0e
    M3OP( "return",                0,  any,    d_cc(Compile_Return),       d_logOp (Return) ),                        // 0x0f
    M3OP( "call",                  0,  any,    d_cc(Compile_Call),         d_logOp (Call) ),                          // 0x10
    M3OP( "call_indirect",         0,  any,    d_cc(Compile_Call_اسدھا), d_logOp (Call_اسدھا) ),                  // 0x11
    M3OP( "return_call",           0,  any,    d_cc(Compile_Call),         d_logOp (ReturnCall) ),                    // 0x12
    M3OP( "return_call_indirect",  0,  any,    d_cc(Compile_Call_اسدھا), d_logOp (Return_Call_اسدھا) ),            // 0x13

#إذا d_m3HasTypedRefs
    M3OP( "call_ref",              0,  any,    d_cc(Compile_CallRef),      d_logOp (CallRef) ),                       // 0x14
    M3OP( "return_call_ref",       0,  any,    d_cc(Compile_CallRef),      d_logOp (ReturnCallRef) ),                 // 0x15
#وإلا
    M3OP_RESERVED,  M3OP_RESERVED,                                                                                    // 0x14...
#جے_انت
    M3OP_RESERVED,  M3OP_RESERVED, M3OP_RESERVED, M3OP_RESERVED,                                                      // ...0x19

    M3OP( "drop",                  -1, none,   d_cc(Compile_Drop),         d_خالی_تھاواں_Op_List ),                           // 0x1a
    M3OP( "select",                -2, any,    d_cc(Compile_Select),       d_خالی_تھاواں_Op_List ),                           // 0x1b

#إذا d_m3HasRefTypes
    M3OP( "select.t",              -2, any,    d_cc(Compile_Select_Typed), d_خالی_تھاواں_Op_List ),                           // 0x1c
#وإلا
    M3OP_RESERVED,                                                                                                    // 0x1c
#جے_انت
    M3OP_RESERVED,  M3OP_RESERVED,                                                                                    // 0x1d...0x1e

#إذا d_m3HasExceptionHandling
    M3OP( "try_table",             0,  none,   d_cc(Compile_Try_جدول),     d_logOp (Try_جدول) ),                      // 0x1f
#وإلا
    M3OP_RESERVED,                                                                                                    // 0x1f
#جے_انت

    M3OP( "local.get",             1,  any,    d_cc(Compile_GetLocal),     d_خالی_تھاواں_Op_List ),                           // 0x20
    M3OP( "local.set",             1,  none,   d_cc(Compile_سیٹ_Local),     d_خالی_تھاواں_Op_List ),                           // 0x21
    M3OP( "local.tee",             0,  any,    d_cc(Compile_سیٹ_Local),     d_خالی_تھاواں_Op_List ),                           // 0x22
    M3OP( "global.get",            1,  none,   d_cc(Compile_Get_سیٹ_Global), d_خالی_تھاواں_Op_List ),                           // 0x23
    M3OP( "global.set",            1,  none,   d_cc(Compile_Get_سیٹ_Global), d_خالی_تھاواں_Op_List ),                           // 0x24

#إذا d_m3HasRefTypes
    M3OP( "table.get",             0,  any,    d_cc(Compile_جدول_Get_سیٹ), d_خالی_تھاواں_Op_List ),                           // 0x25
    M3OP( "table.set",             -2, none,   d_cc(Compile_جدول_Get_سیٹ), d_خالی_تھاواں_Op_List ),                           // 0x26
#وإلا
    M3OP_RESERVED,  M3OP_RESERVED,                                                                                    // 0x25...0x26
#جے_انت
    M3OP_RESERVED,                                                                                                    // 0x27

    M3OP( "i32.load",              0,  i_32,   d_cc(Compile_لدو_Store),   d_unaryOpList (i32, لدو_i32) ),           // 0x28
    M3OP( "i64.load",              0,  i_64,   d_cc(Compile_لدو_Store),   d_unaryOpList (i64, لدو_i64) ),           // 0x29
    M3OP_F( "f32.load",            0,  f_32,   d_cc(Compile_لدو_Store),   d_unaryOpList (f32, لدو_f32) ),           // 0x2a
    M3OP_F( "f64.load",            0,  f_64,   d_cc(Compile_لدو_Store),   d_unaryOpList (f64, لدو_f64) ),           // 0x2b

    M3OP( "i32.load8_s",           0,  i_32,   d_cc(Compile_لدو_Store),   d_unaryOpList (i32, لدو_i8) ),            // 0x2c
    M3OP( "i32.load8_u",           0,  i_32,   d_cc(Compile_لدو_Store),   d_unaryOpList (i32, لدو_u8) ),            // 0x2d
    M3OP( "i32.load16_s",          0,  i_32,   d_cc(Compile_لدو_Store),   d_unaryOpList (i32, لدو_i16) ),           // 0x2e
    M3OP( "i32.load16_u",          0,  i_32,   d_cc(Compile_لدو_Store),   d_unaryOpList (i32, لدو_u16) ),           // 0x2f

    M3OP( "i64.load8_s",           0,  i_64,   d_cc(Compile_لدو_Store),   d_unaryOpList (i64, لدو_i8) ),            // 0x30
    M3OP( "i64.load8_u",           0,  i_64,   d_cc(Compile_لدو_Store),   d_unaryOpList (i64, لدو_u8) ),            // 0x31
    M3OP( "i64.load16_s",          0,  i_64,   d_cc(Compile_لدو_Store),   d_unaryOpList (i64, لدو_i16) ),           // 0x32
    M3OP( "i64.load16_u",          0,  i_64,   d_cc(Compile_لدو_Store),   d_unaryOpList (i64, لدو_u16) ),           // 0x33
    M3OP( "i64.load32_s",          0,  i_64,   d_cc(Compile_لدو_Store),   d_unaryOpList (i64, لدو_i32) ),           // 0x34
    M3OP( "i64.load32_u",          0,  i_64,   d_cc(Compile_لدو_Store),   d_unaryOpList (i64, لدو_u32) ),           // 0x35

    M3OP( "i32.store",             -2, none,   d_cc(Compile_لدو_Store),   d_binOpList (i32, Store_i32) ),            // 0x36
    M3OP( "i64.store",             -2, none,   d_cc(Compile_لدو_Store),   d_binOpList (i64, Store_i64) ),            // 0x37
    M3OP_F( "f32.store",           -2, none,   d_cc(Compile_لدو_Store),   d_storeFpOpList (f32, Store_f32) ),        // 0x38
    M3OP_F( "f64.store",           -2, none,   d_cc(Compile_لدو_Store),   d_storeFpOpList (f64, Store_f64) ),        // 0x39

    M3OP( "i32.store8",            -2, none,   d_cc(Compile_لدو_Store),   d_binOpList (i32, Store_u8) ),             // 0x3a
    M3OP( "i32.store16",           -2, none,   d_cc(Compile_لدو_Store),   d_binOpList (i32, Store_i16) ),            // 0x3b

    M3OP( "i64.store8",            -2, none,   d_cc(Compile_لدو_Store),   d_binOpList (i64, Store_u8) ),             // 0x3c
    M3OP( "i64.store16",           -2, none,   d_cc(Compile_لدو_Store),   d_binOpList (i64, Store_i16) ),            // 0x3d
    M3OP( "i64.store32",           -2, none,   d_cc(Compile_لدو_Store),   d_binOpList (i64, Store_i32) ),            // 0x3e

    M3OP( "memory.size",           1,  i_32,   d_cc(Compile_حافظہ_ناپ),  d_logOp (Mem_ناپ) ),                       // 0x3f
    M3OP( "memory.grow",           1,  i_32,   d_cc(Compile_حافظہ_Grow),  d_logOp (MemGrow) ),                       // 0x40

    M3OP( "i32.const",             1,  i_32,   d_cc(Compile_Const_i32),    d_logOp (Const32) ),                       // 0x41
    M3OP( "i64.const",             1,  i_64,   d_cc(Compile_Const_i64),    d_logOp (Const64) ),                       // 0x42
    M3OP_F( "f32.const",           1,  f_32,   d_cc(Compile_Const_f32),    d_خالی_تھاواں_Op_List ),                           // 0x43
    M3OP_F( "f64.const",           1,  f_64,   d_cc(Compile_Const_f64),    d_خالی_تھاواں_Op_List ),                           // 0x44

    M3OP( "i32.eqz",               0,  i_32,   d_cc(عدم),                 d_unaryOpList (i32, Equal_منزل_Zero) ),        // 0x45
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

    M3OP( "i64.eqz",               0,  i_32,   d_cc(عدم),                 d_unaryOpList (i64, Equal_منزل_Zero) ),        // 0x50
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

    M3OP( "i32.add",               -1, i_32,   d_cc(عدم),                 d_commutativeBinOpList (i32, جوڑو) ),       // 0x6a
    M3OP( "i32.sub",               -1, i_32,   d_cc(عدم),                 d_binOpList (i32, گھٹاؤ) ),             // 0x6b
    M3OP( "i32.mul",               -1, i_32,   d_cc(عدم),                 d_commutativeBinOpList (i32, Multiply) ),  // 0x6c
    M3OP( "i32.div_s",             -1, i_32,   d_cc(عدم),                 d_binOpList (i32, Divide) ),               // 0x6d
    M3OP( "i32.div_u",             -1, i_32,   d_cc(عدم),                 d_binOpList (u32, Divide) ),               // 0x6e
    M3OP( "i32.rem_s",             -1, i_32,   d_cc(عدم),                 d_binOpList (i32, Remainder) ),            // 0x6f
    M3OP( "i32.rem_u",             -1, i_32,   d_cc(عدم),                 d_binOpList (u32, Remainder) ),            // 0x70
    M3OP( "i32.and",               -1, i_32,   d_cc(عدم),                 d_commutativeBinOpList (u32, And) ),       // 0x71
    M3OP( "i32.or",                -1, i_32,   d_cc(عدم),                 d_commutativeBinOpList (u32, Or) ),        // 0x72
    M3OP( "i32.xor",               -1, i_32,   d_cc(عدم),                 d_commutativeBinOpList (u32, Xor) ),       // 0x73
    M3OP( "i32.shl",               -1, i_32,   d_cc(عدم),                 d_binOpList (u32, کھسکاؤ_بایاں) ),            // 0x74
    M3OP( "i32.shr_s",             -1, i_32,   d_cc(عدم),                 d_binOpList (i32, کھسکاؤ_دایاں) ),           // 0x75
    M3OP( "i32.shr_u",             -1, i_32,   d_cc(عدم),                 d_binOpList (u32, کھسکاؤ_دایاں) ),           // 0x76
    M3OP( "i32.rotl",              -1, i_32,   d_cc(عدم),                 d_binOpList (u32, Rotl) ),                 // 0x77
    M3OP( "i32.rotr",              -1, i_32,   d_cc(عدم),                 d_binOpList (u32, Rotr) ),                 // 0x78

    M3OP( "i64.clz",               0,  i_64,   d_cc(عدم),                 d_unaryOpList (u64, Clz) ),                // 0x79
    M3OP( "i64.ctz",               0,  i_64,   d_cc(عدم),                 d_unaryOpList (u64, Ctz) ),                // 0x7a
    M3OP( "i64.popcnt",            0,  i_64,   d_cc(عدم),                 d_unaryOpList (u64, Popcnt) ),             // 0x7b

    M3OP( "i64.add",               -1, i_64,   d_cc(عدم),                 d_commutativeBinOpList (i64, جوڑو) ),       // 0x7c
    M3OP( "i64.sub",               -1, i_64,   d_cc(عدم),                 d_binOpList (i64, گھٹاؤ) ),             // 0x7d
    M3OP( "i64.mul",               -1, i_64,   d_cc(عدم),                 d_commutativeBinOpList (i64, Multiply) ),  // 0x7e
    M3OP( "i64.div_s",             -1, i_64,   d_cc(عدم),                 d_binOpList (i64, Divide) ),               // 0x7f
    M3OP( "i64.div_u",             -1, i_64,   d_cc(عدم),                 d_binOpList (u64, Divide) ),               // 0x80
    M3OP( "i64.rem_s",             -1, i_64,   d_cc(عدم),                 d_binOpList (i64, Remainder) ),            // 0x81
    M3OP( "i64.rem_u",             -1, i_64,   d_cc(عدم),                 d_binOpList (u64, Remainder) ),            // 0x82
    M3OP( "i64.and",               -1, i_64,   d_cc(عدم),                 d_commutativeBinOpList (u64, And) ),       // 0x83
    M3OP( "i64.or",                -1, i_64,   d_cc(عدم),                 d_commutativeBinOpList (u64, Or) ),        // 0x84
    M3OP( "i64.xor",               -1, i_64,   d_cc(عدم),                 d_commutativeBinOpList (u64, Xor) ),       // 0x85
    M3OP( "i64.shl",               -1, i_64,   d_cc(عدم),                 d_binOpList (u64, کھسکاؤ_بایاں) ),            // 0x86
    M3OP( "i64.shr_s",             -1, i_64,   d_cc(عدم),                 d_binOpList (i64, کھسکاؤ_دایاں) ),           // 0x87
    M3OP( "i64.shr_u",             -1, i_64,   d_cc(عدم),                 d_binOpList (u64, کھسکاؤ_دایاں) ),           // 0x88
    M3OP( "i64.rotl",              -1, i_64,   d_cc(عدم),                 d_binOpList (u64, Rotl) ),                 // 0x89
    M3OP( "i64.rotr",              -1, i_64,   d_cc(عدم),                 d_binOpList (u64, Rotr) ),                 // 0x8a

    M3OP_F( "f32.abs",             0,  f_32,   d_cc(عدم),                 d_unaryOpList(f32, Abs) ),                 // 0x8b
    M3OP_F( "f32.neg",             0,  f_32,   d_cc(عدم),                 d_unaryOpList(f32, Negate) ),              // 0x8c
    M3OP_F( "f32.ceil",            0,  f_32,   d_cc(عدم),                 d_unaryOpList(f32, Ceil) ),                // 0x8d
    M3OP_F( "f32.floor",           0,  f_32,   d_cc(عدم),                 d_unaryOpList(f32, Floor) ),               // 0x8e
    M3OP_F( "f32.trunc",           0,  f_32,   d_cc(عدم),                 d_unaryOpList(f32, Trunc) ),               // 0x8f
    M3OP_F( "f32.nearest",         0,  f_32,   d_cc(عدم),                 d_unaryOpList(f32, سبھ_توں_نیڑے) ),             // 0x90
    M3OP_F( "f32.sqrt",            0,  f_32,   d_cc(عدم),                 d_unaryOpList(f32, Sqrt) ),                // 0x91

    M3OP_F( "f32.add",             -1, f_32,   d_cc(عدم),                 d_commutativeBinOpList (f32, جوڑو) ),       // 0x92
    M3OP_F( "f32.sub",             -1, f_32,   d_cc(عدم),                 d_binOpList (f32, گھٹاؤ) ),             // 0x93
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
    M3OP_F( "f64.nearest",         0,  f_64,   d_cc(عدم),                 d_unaryOpList(f64, سبھ_توں_نیڑے) ),             // 0x9e
    M3OP_F( "f64.sqrt",            0,  f_64,   d_cc(عدم),                 d_unaryOpList(f64, Sqrt) ),                // 0x9f

    M3OP_F( "f64.add",             -1, f_64,   d_cc(عدم),                 d_commutativeBinOpList (f64, جوڑو) ),       // 0xa0
    M3OP_F( "f64.sub",             -1, f_64,   d_cc(عدم),                 d_binOpList (f64, گھٹاؤ) ),             // 0xa1
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
#تعریف_کرو d_m3DebugOp(OP) M3OP (#OP, 0, none, d_ops (op_##OP))

# إذا d_m3HasFloat
#تعریف_کرو d_m3DebugTypedOp(OP) M3OP (#OP, 0, none, d_ops (op_##OP##_i32, op_##OP##_i64, op_##OP##_f32, op_##OP##_f64))
# وإلا
#تعریف_کرو d_m3DebugTypedOp(OP) M3OP (#OP, 0, none, d_ops (op_##OP##_i32, op_##OP##_i64))
# endif

    d_m3DebugOp (Compile),          d_m3DebugOp (اندراج_2),            d_m3DebugOp (سرا_2),
    d_m3DebugOp (Unsupported),      d_m3DebugOp (CallRawFunction),

    d_m3DebugOp (GetGlobal_s32),    d_m3DebugOp (GetGlobal_s64),    d_m3DebugOp (ContinueLoop),     d_m3DebugOp (ContinueLoopIf),
# إذا d_m3_Has_ڈھیر_Switching
    d_m3DebugOp (ContinueLoop_Suspendable), d_m3DebugOp (ContinueLoopIf_Suspendable), d_m3DebugOp (اندراج_Check),
#  إذا !d_m3_اندراج_Keeps_چوکھٹا
    d_m3DebugOp (اندراج_Suspendable),
#  endif
# endif

    d_m3DebugOp (Copy_تھاں_32),      d_m3DebugOp (Preserve_Copy_تھاں_32), d_m3DebugOp (If_s),          d_m3DebugOp (BranchIfPrologue_s),
    d_m3DebugOp (Copy_تھاں_64),      d_m3DebugOp (Preserve_Copy_تھاں_64), d_m3DebugOp (If_r),          d_m3DebugOp (BranchIfPrologue_r),

    d_m3DebugOp (Select_i32_rss),   d_m3DebugOp (Select_i32_srs),   d_m3DebugOp (Select_i32_ssr),   d_m3DebugOp (Select_i32_sss),
    d_m3DebugOp (Select_i64_rss),   d_m3DebugOp (Select_i64_srs),   d_m3DebugOp (Select_i64_ssr),   d_m3DebugOp (Select_i64_sss),

# إذا d_m3HasFloat
    d_m3DebugOp (Select_f32_sss),   d_m3DebugOp (Select_f32_srs),   d_m3DebugOp (Select_f32_ssr),
    d_m3DebugOp (Select_f32_rss),   d_m3DebugOp (Select_f32_rrs),   d_m3DebugOp (Select_f32_rsr),

    d_m3DebugOp (Select_f64_sss),   d_m3DebugOp (Select_f64_srs),   d_m3DebugOp (Select_f64_ssr),
    d_m3DebugOp (Select_f64_rss),   d_m3DebugOp (Select_f64_rrs),   d_m3DebugOp (Select_f64_rsr),
# endif

    d_m3DebugOp (MemFill),          d_m3DebugOp (MemCopy),          d_m3DebugOp (MemInit),          d_m3DebugOp (انکڑے_Drop),

# إذا d_m3HasGasMetering
    d_m3DebugOp (UseGas),
# endif

# إذا d_m3HasRefTypes
    d_m3DebugOp (جدول_Get),         d_m3DebugOp (جدول_سیٹ),         d_m3DebugOp (جدول_ناپ),
    d_m3DebugOp (جدول_Grow),        d_m3DebugOp (جدول_Fill),        d_m3DebugOp (جدول_Init),
    d_m3DebugOp (ElemDrop),         d_m3DebugOp (جدول_Copy),
# endif

    d_m3DebugTypedOp (سیٹ_Global),   d_m3DebugOp (سیٹ_Global_s32),    d_m3DebugOp (سیٹ_Global_s64),

    d_m3DebugTypedOp (سیٹ_رجسٹر), d_m3DebugTypedOp (سیٹ_تھاں),     d_m3DebugTypedOp (Preserve_سیٹ_تھاں),
# endif

# ifdef DEBUG
    M3OP( "termination", 0, c_m3Type_unknown, d_خالی_تھاواں_Op_List ) // for find_operation_info
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

    M3OP( "memory.init",           0, none,   d_cc(Compile_حافظہ_Init),     d_خالی_تھاواں_Op_List ),                       // 0x08
    M3OP( "data.drop",             0, none,   d_cc(Compile_انکڑے_Drop),       d_خالی_تھاواں_Op_List ),                       // 0x09

    M3OP( "memory.copy",           0, none,   d_cc(Compile_حافظہ_Copy_Fill), d_خالی_تھاواں_Op_List ),                       // 0x0a
    M3OP( "memory.fill",           0, none,   d_cc(Compile_حافظہ_Copy_Fill), d_خالی_تھاواں_Op_List ),                       // 0x0b

#إذا d_m3HasRefTypes
    M3OP( "table.init",            0, none,   d_cc(Compile_جدول_Init),      d_خالی_تھاواں_Op_List ),                       // 0x0c
    M3OP( "elem.drop",             0, none,   d_cc(Compile_Elem_Drop),       d_خالی_تھاواں_Op_List ),                       // 0x0d
    M3OP( "table.copy",            0, none,   d_cc(Compile_جدول_Copy),      d_خالی_تھاواں_Op_List ),                       // 0x0e
#وإلا
    M3OP_RESERVED, M3OP_RESERVED, M3OP_RESERVED,                                                                    // 0x0c...0x0e
#جے_انت

#إذا d_m3HasRefTypes
    M3OP( "table.grow",            0, i_32,   d_cc(Compile_جدول_Grow_Fill),  d_خالی_تھاواں_Op_List ),                       // 0x0f
    M3OP( "table.size",            1, i_32,   d_cc(Compile_جدول_ناپ),      d_خالی_تھاواں_Op_List ),                       // 0x10
    M3OP( "table.fill",            0, none,   d_cc(Compile_جدول_Grow_Fill),  d_خالی_تھاواں_Op_List ),                       // 0x11
#وإلا
    M3OP_RESERVED, M3OP_RESERVED, M3OP_RESERVED,                                                                    // 0x0f...0x11
#جے_انت

#إذا d_m3HasWideArithmetic
    M3OP_RESERVED,                                                                                                  // 0x12

    M3OP( "i64.add128",            -2, i_64,  d_cc(Compile_WideArithmetic), d_logOp (i64_Add128) ),              // 0x13
    M3OP( "i64.sub128",            -2, i_64,  d_cc(Compile_WideArithmetic), d_logOp (i64_Sub128) ),              // 0x14
    M3OP( "i64.mul_wide_s",        0,  i_64,  d_cc(Compile_WideArithmetic), d_logOp (i64_MultiplyWide) ),        // 0x15
    M3OP( "i64.mul_wide_u",        0,  i_64,  d_cc(Compile_WideArithmetic), d_logOp (u64_MultiplyWide) ),        // 0x16
#جے_انت


# ifdef DEBUG
    M3OP( "termination", 0, c_m3Type_unknown, d_خالی_تھاواں_Op_List ) // for find_operation_info
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
    c_op_Atomic_لدو,
    c_opAtomic_store,
    c_opAtomic_rmw,
    c_opAtomic_cmpxchg
};

ثابت M3OpInfo c_operationsFE[] =
{
    M3OP( "memory.atomic.notify",  -1, i_32,  d_cc(Compile_Atomic), d_logOp (AtomicNotify) ),    // c_opAtomic_notify
    M3OP( "memory.atomic.wait32",  -2, i_32,  d_cc(Compile_Atomic), d_logOp (AtomicWait32) ),    // c_opAtomic_wait32
    M3OP( "memory.atomic.wait64",  -2, i_32,  d_cc(Compile_Atomic), d_logOp (AtomicWait64) ),    // c_opAtomic_wait64
    M3OP( "atomic.fence",           0, none,  d_cc(Compile_Atomic), d_خالی_تھاواں_Op_List ),             // c_opAtomic_fence
    M3OP( "atomic.load",            0, any,   d_cc(Compile_Atomic), d_logOp (Atomic_لدو) ),      // c_opAtomic_load
    M3OP( "atomic.store",          -2, none,  d_cc(Compile_Atomic), d_logOp (AtomicStore) ),     // c_opAtomic_store
    M3OP( "atomic.rmw",            -1, any,   d_cc(Compile_Atomic), d_logOp (AtomicRmw) ),       // c_opAtomic_rmw
    M3OP( "atomic.rmw.cmpxchg",    -2, any,   d_cc(Compile_Atomic), d_logOp (AtomicCmpxchg) ),   // c_opAtomic_cmpxchg
};

ثابت u32 c_عدد_Operations_FE = M3_گنتی_دا(c_operationsFE);

// The entry a 0xFE sub-opcode uses, or -1 for one that names nothing: the spec leaves
// 0x04..0x0f and everything past cmpxchg unassigned. The accesses come in runs of seven,
// one per pairing of value type and cell width.
ساكن مضمن
i32 Get_Atomic_Info_اشاریہ (u32 i_sub)
{
    إذا (i_sub <= 0x03) أرجع (i32)i_sub;
    إذا (i_sub < 0x10) أرجع -1;
    إذا (i_sub <= 0x16) أرجع c_op_Atomic_لدو;
    إذا (i_sub <= 0x1d) أرجع c_opAtomic_store;
    إذا (i_sub <= 0x47) أرجع c_opAtomic_rmw;
    إذا (i_sub <= c_wa_Op_آخری_Atomic) أرجع c_opAtomic_cmpxchg;
    أرجع -1;
}

#جے_انت // d_m3HasAtomics

// The opcodes above c_waOp_lastCore are sparse: the reference instructions at
// 0xd0..0xd2 and 0xd4, and the 0xfc extended-opcode prefix. Indexing c_operations out to
// 0xfc to reach them would leave ~50 empty entries there, so they get their own
// table, which GetHighOpIndex () maps the opcode onto.
تعداد {
#إذا d_m3HasRefTypes
    c_op_بلند_ref_Null,
    c_op_بلند_ref_Is_Null,
    c_op_بلند_ref_Func,
#  إذا d_m3HasTypedRefs
    c_op_بلند_ref_As_Non_Null,
#  endif
#جے_انت
#إذا d_m3_Has_ڈھیر_Switching
    c_op_بلند_cont_New,
    c_op_بلند_cont_Bind,
    c_op_بلند_suspend,
    c_op_بلند_resume,
    c_op_بلند_resume_Throw,
    c_op_بلند_resume_Throw_Ref,
    c_op_بلند_switch,
#جے_انت
#إذا d_m3HasAtomics
    c_op_بلند_atomic,
#جے_انت
    c_op_بلند_extended   // always present, so the enum is never empty
};

// Positional, so the table must stay in c_opHigh_* order: array designators
// are not valid C++, which the maintenance builds compile this as.
ساكن ثابت M3OpInfo c_operations_بلند[] =
{
#إذا d_m3HasRefTypes
    M3OP( "ref.null",        1, any,               d_cc(Compile_Ref_Null),       d_خالی_تھاواں_Op_List ),  // 0xd0  c_opHigh_refNull
    M3OP( "ref.is_null",     0, i_32,              d_cc(Compile_Ref_IsNull),     d_خالی_تھاواں_Op_List ),  // 0xd1  c_opHigh_refIsNull
    M3OP( "ref.func",        1, any,               d_cc(Compile_Ref_Func),       d_خالی_تھاواں_Op_List ),  // 0xd2  c_opHigh_refFunc
#  إذا d_m3HasTypedRefs
    M3OP( "ref.as_non_null", 0, any,               d_cc(Compile_Ref_AsNonNull),  d_خالی_تھاواں_Op_List ),  // 0xd4  c_opHigh_refAsNonNull
#  endif
#جے_انت
#إذا d_m3_Has_ڈھیر_Switching
    M3OP( "cont.new",          1, any,             d_cc(Compile_ContNew),        d_خالی_تھاواں_Op_List ),  // 0xe0  c_opHigh_contNew
    M3OP( "cont.bind",         2, any,             d_cc(Compile_ContBind),       d_خالی_تھاواں_Op_List ),  // 0xe1  c_opHigh_contBind
    M3OP( "suspend",           1, any,             d_cc(Compile_Suspend),        d_خالی_تھاواں_Op_List ),  // 0xe2  c_opHigh_suspend
    M3OP( "resume",            1, any,             d_cc(Compile_Resume),         d_خالی_تھاواں_Op_List ),  // 0xe3  c_opHigh_resume
    M3OP( "resume_throw",      2, any,             d_cc(Compile_Resume)   ,    d_خالی_تھاواں_Op_List ),  // 0xe4  c_opHigh_resumeThrow
    M3OP( "resume_throw_ref",  1, any,             d_cc(Compile_Resume)      , d_خالی_تھاواں_Op_List ),  // 0xe5  c_opHigh_resumeThrowRef
    M3OP( "switch",            2, any,             d_cc(Compile_Switch),         d_خالی_تھاواں_Op_List ),  // 0xe6  c_opHigh_switch
#جے_انت

#إذا d_m3HasAtomics
    M3OP( "0xFE",            0, c_m3Type_unknown,  d_cc(Compile_ExtendedOpcode), d_خالی_تھاواں_Op_List ),  // 0xfe  c_opHigh_atomic
#جے_انت
    M3OP( "0xFC",            0, c_m3Type_unknown,  d_cc(Compile_ExtendedOpcode), d_خالی_تھاواں_Op_List ),  // 0xfc  c_opHigh_extended
};
M3_STATIC_ASSERT (M3_گنتی_دا (c_operations_بلند) == c_op_بلند_extended + 1, c_operations_بلند_needs_one_اندراج_فی_c_op_بلند);

// clang-format on


// Opcodes the spec reserves leave zeroed holes in the tables above: no compiler
// and no operations. Every implemented op has at least one of the two.
ساكن مضمن
bool IsImplementedOp (IM3OpInfo i_info)
{
    أرجع (i_info->compiler != c_cc_NULL or i_info->operations[0] != عدم);
}

ثابت u32 c_عدد_Operations   = M3_گنتی_دا(c_operations);
ثابت u32 c_عدد_Operations_FC = M3_گنتی_دا(c_operationsFC);

// Maps one of the sparse opcodes above c_waOp_lastCore onto c_operationsHigh,
// or -1 for anything else in that range.
ساكن مضمن
i32 Get_بلند_Op_اشاریہ (m3opcode_t opcode)
{
    بدل (opcode) {
#إذا d_m3HasRefTypes
    حالة c_waOp_refNull: أرجع c_op_بلند_ref_Null;
    حالة c_waOp_refIsNull: أرجع c_op_بلند_ref_Is_Null;
    حالة c_waOp_refFunc: أرجع c_op_بلند_ref_Func;
#  إذا d_m3HasTypedRefs
    حالة c_waOp_refAsNonNull: أرجع c_op_بلند_ref_As_Non_Null;
#  endif
#جے_انت
#إذا d_m3_Has_ڈھیر_Switching
    حالة c_waOp_contNew: أرجع c_op_بلند_cont_New;
    حالة c_waOp_contBind: أرجع c_op_بلند_cont_Bind;
    حالة c_waOp_suspend: أرجع c_op_بلند_suspend;
    حالة c_waOp_resume: أرجع c_op_بلند_resume;
    حالة c_waOp_resumeThrow: أرجع c_op_بلند_resume_Throw;
    حالة c_waOp_resumeThrowRef: أرجع c_op_بلند_resume_Throw_Ref;
    حالة c_waOp_switch: أرجع c_op_بلند_switch;
#جے_انت
    حالة c_waOp_extended: أرجع c_op_بلند_extended;
#إذا d_m3HasAtomics
    حالة c_waOp_atomic: أرجع c_op_بلند_atomic;
#جے_انت
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
        إذا (M3_LIKELY(opcode <= c_wa_Op_آخری_Core)) {
            info = &c_operations[opcode];
        } وإلا {
            i32 بلند = Get_بلند_Op_اشاریہ(opcode);
            إذا (بلند >= 0) {
                info = &c_operations_بلند[بلند];
            }
        }
        اكسر;
    حالة c_waOp_extended:
        opcode &= 0xFF;
        إذا (M3_LIKELY(opcode <= c_wa_Op_آخری_Extended)) {
            info = &c_operationsFC[opcode];
        }
        اكسر;
#إذا d_m3HasAtomics
    حالة c_waOp_atomic: {
        i32 index = Get_Atomic_Info_اشاریہ(opcode & 0xFF);
        إذا (index >= 0) {
            info = &c_operationsFE[index];
        }
    } اكسر;
#جے_انت
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
    c_gas_کھسکاؤ   = 67,
    c_gasBranch  = 90,
    c_gas_Mem_ناپ = 100,
    c_gasAccess  = 120,        // a local, global, table or memory access
    c_gasHeavy   = 10000,      // an indirect call, or anything that works over a whole region
    c_gasDivide  = 36000
};

// The most one op_UseGas can carry. A segment that reaches it is split into
// consecutive charges, which prepays exactly the same amount.
#  define d_m3MaxSegmentGas   0xFFFFFFFF

ساكن
u32 Get_Gas_لاگت (m3opcode_t i_opcode)
{
    بدل (i_opcode) {
    حالة 0x00:                        // unreachable
    حالة 0x01:                        // nop
    حالة c_wa_Op_بلاک:
    حالة c_waOp_loop:
    حالة c_waOp_if:
    حالة c_wa_Op_try_جدول:             // a block that also installs handlers
    حالة c_waOp_refNull:              // a constant, by another name
    حالة c_waOp_refFunc:
        أرجع c_gasNominal;

    حالة c_wa_Op_سرا:
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

    حالة c_wa_Op_call_اسدھا:
    حالة c_wa_Op_return_Call_اسدھا:
    حالة c_waOp_callRef:
    حالة c_waOp_returnCallRef:
    حالة c_wa_Op_حافظہ_Init:
    حالة c_wa_Op_حافظہ_Copy:
    حالة c_wa_Op_حافظہ_Fill:
    حالة c_wa_Op_جدول_Init:
    حالة c_wa_Op_جدول_Copy:
    حالة c_wa_Op_جدول_Grow:
    حالة c_wa_Op_جدول_Fill:
    حالة 0x40:                        // memory.grow
        أرجع c_gasHeavy;

    حالة c_wa_Op_انکڑے_Drop:
    حالة c_waOp_elemDrop:
    حالة c_wa_Op_جدول_ناپ:
    حالة 0x3f:                        // memory.size
        أرجع c_gas_Mem_ناپ;

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
            c_gas_کھسکاؤ,  c_gas_کھسکاؤ,  c_gas_کھسکاؤ,                   // shl, shr_s, shr_u
            c_gasBranch, c_gasBranch                                // rotl, rotr
        };
        // clang-format on

        أرجع c_arithmeticCosts[i_opcode - (i_opcode <= 0x78 ? 0x6a : 0x7c)];
    }

    // the atomic accesses are memory accesses; what waits, wakes or orders is worth
    // as much as an operation over a whole region
    إذا (i_opcode >= (c_waOp_atomic << 8) and i_opcode <= ((c_waOp_atomic << 8) | c_wa_Op_آخری_Atomic)) {
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
bool Is_Gas_Segment_سرا (m3opcode_t i_opcode)
{
    بدل (i_opcode) {
    حالة c_waOp_loop:
    حالة c_waOp_if:
    حالة c_waOp_else:
    حالة c_wa_Op_سرا:
    حالة c_waOp_branch:
    حالة c_waOp_branchIf:
    حالة c_wa_Op_branch_جدول:
    حالة c_waOp_return:
    حالة c_waOp_returnCall:
    حالة c_wa_Op_return_Call_اسدھا:
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
            u32 لاگت = o->gas_لاگت;
            حافظہ_نقل(o->gasPatch, &لاگت, حجم(لاگت));

            o->gasPatch = عدم;
        }
        o->gas_لاگت          = 0;
        o->isGasSegmentOpen = false;
    }
}

// Whether this compilation emits the charges
ساكن مضمن
bool IsMetering (IM3Compilation o)
{
    أرجع o->runtime->gas_حد_بندی != 0;
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
M3_نتیجہ MeterOpcode (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_نتیجہ نتیجہ = m3Err_none;

    // constant expressions are not metered: they run once at instantiation,
    // before the module is anything a gas budget was handed out for
    إذا (o->function and o->ورقہ and IsMetering(o)) {
        // only the 0xFC or 0xFE prefix has been read so far; the instruction it names is
        // the LEB128 u32 the compiler is about to take. Peek it without consuming.
        // Anything the tables do not cover leaves the prefix priced on its own,
        // which is moot - the compile fails on it a moment later.
        إذا (i_opcode == c_waOp_extended or i_opcode == c_waOp_atomic) {
            بائٹاں_t peek = o->wasm;
            u32     sub;
            u32     آخری = (i_opcode == c_waOp_atomic) ? c_wa_Op_آخری_Atomic : c_wa_Op_آخری_Extended;

            إذا (not ReadLEB_u32(&sub, &peek, o->wasm_سرا) and sub <= آخری) {
                i_opcode = (m3opcode_t)((i_opcode << 8) | sub);
            }
        }

        u32 لاگت = Get_Gas_لاگت(i_opcode);

        إذا (o->isGasSegmentOpen and o->gas_لاگت > d_m3MaxSegmentGas - لاگت) {
            CloseGasSegment(o);
        }

        إذا (not o->isGasSegmentOpen) {
            o->isGasSegmentOpen = true;

_           (EmitOp(o, op_UseGas));
            o->gasPatch = (فراغ*)Get_ورقہ_PC(o->ورقہ);
            EmitConstant32(o, 0);
        }

        o->gas_لاگت += لاگت;

        إذا (Is_Gas_Segment_سرا(i_opcode)) {
            CloseGasSegment(o);
        }
    }

    _catch: أرجع نتیجہ;
}

#جے_انت // d_m3HasGasMetering


M3_نتیجہ Compile_بلاک_Statements (IM3Compilation o)
{
    M3_نتیجہ نتیجہ   = m3Err_none;
    bool     valid_سرا = false;

    طالما (o->wasm < o->wasm_سرا) {
#إذا d_m3EnableOpTracing
        إذا (o->عدد_Emits) {
            EmitOp(o, op_Dump_ڈھیر);
            EmitConstant32(o, o->عدد_Opcodes);
            EmitConstant32(o, Get_Max_ورتیا_تھاں_Plus_One(o));
            Emit_اشارہ_گر(o, o->function);

            o->عدد_Emits = 0;
        }
#جے_انت

        m3opcode_t opcode;
        o->آخری_Opcode_شروع = o->wasm;
_       (Read_opcode(&opcode, &o->wasm, o->wasm_سرا));
        log_opcode(o, opcode);

        // Restrict opcodes when evaluating expressions
        إذا (M3_UNLIKELY(not o->function)) {
            // clang-format off
            بدل (opcode) {
            حالة c_waOp_i32_const: حالة c_waOp_i64_const:
            حالة c_waOp_f32_const: حالة c_waOp_f64_const:
            حالة c_waOp_getGlobal: حالة c_wa_Op_سرا:
#إذا d_m3HasRefTypes
            حالة c_waOp_refNull:   حالة c_waOp_refFunc:
#جے_انت
#إذا d_m3HasExtendedConst
            حالة c_wa_Op_i32_جوڑو:   حالة c_waOp_i32_sub:   حالة c_waOp_i32_mul:
            حالة c_wa_Op_i64_جوڑو:   حالة c_waOp_i64_sub:   حالة c_waOp_i64_mul:
#جے_انت
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
#جے_انت

        إذا (opinfo->compiler != c_cc_NULL) {
_           ((*GetCompiler(opinfo))(o, opcode))
        } وإلا {
_           (Compile_Operator(o, opcode));
        }

        o->پچھلا_Opcode = opcode;

        إذا (opcode == c_waOp_else) {
            _throwif(m3Err_wasmMalformed, o->بلاک.opcode != c_waOp_if);
            valid_سرا = true;
            اكسر;
        } وإلا إذا (opcode == c_wa_Op_سرا) {
            valid_سرا = true;
            اكسر;
        }
    }
    _throwif(m3Err_wasmMalformed, not valid_سرا);

_catch:
    أرجع نتیجہ;
}

ساكن
M3_نتیجہ ڈالو_بلاک_Results (IM3Compilation o)
{
    M3_نتیجہ نتیجہ = m3Err_none;

    u16 عدد_Results = Get_Func_Type_عدد_Results(o->بلاک.type);

    لكل (u16 i = 0; i < عدد_Results; ++i) {
        m3type_t type = Get_Func_Type_نتیجہ_Type(o->بلاک.type, i);

        إذا (i == عدد_Results - 1 and IsFpType(type)) {
_           (ڈالو_رجسٹر(o, type));
        } وإلا {
_           (ڈالو_Allocated_تھاں(o, type));
        }
    }

    _catch: أرجع نتیجہ;
}


M3_نتیجہ Compile_بلاک (IM3Compilation o, IM3FuncType i_بلاک_Type, m3opcode_t i_بلاک_Opcode)
{
                                                                                        d_m3Assert (not Is_رجسٹر_Allocated (o, 0));
                                                                                        d_m3Assert (not Is_رجسٹر_Allocated (o, 1));
    InvalidateFold(o);

    M3CompilationScope  outerScope = o->بلاک;
    M3CompilationScope* بلاک      = &o->بلاک;

    بلاک->outer   = &outerScope;
    بلاک->pc      = Get_ورقہ_PC(o->ورقہ);
    بلاک->patches = عدم;
    بلاک->type    = i_بلاک_Type;
    بلاک->opcode  = i_بلاک_Opcode;
    بلاک->ڈونگھائی++;
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
#إذا d_m3HasSnapshots
    // op_Loop's frame records the body, which is also the loop's identity;
    // op_TryTable's records its clause table, two words a clause back from it
    إذا (i_بلاک_Opcode == c_waOp_loop) {
_       (ریکارڈ_بلاک_شروع(o, i_بلاک_Opcode, بلاک->pc, 0));
    }
#  إذا d_m3HasExceptionHandling
    وإلا إذا (i_بلاک_Opcode == c_wa_Op_try_جدول) {
_       (ریکارڈ_بلاک_شروع(o, i_بلاک_Opcode, بلاک->pc - 2 * o->عدد_Try_Clauses, o->عدد_Try_Clauses));
    }
#  endif
#جے_انت

    // validate and dealloc params ----------------------------

    u16 ڈھیر_اشاریہ = o->ڈھیر_اشاریہ;

    u16 عدد_Params = Get_Func_Type_عدد_Params(i_بلاک_Type);

    إذا (i_بلاک_Opcode != c_waOp_else) {
        لكل (u16 i = 0; i < عدد_Params; ++i) {
            m3type_t type = GetFuncTypeParamType(i_بلاک_Type, عدد_Params - 1 - i);
_           (نکالو_Type(o, type));
        }
    } وإلا {
        إذا (Is_ڈھیر_Polymorphic(o) && o->بلاک.بلاک_ڈھیر_اشاریہ + عدد_Params > o->ڈھیر_اشاریہ) {
            o->ڈھیر_اشاریہ = o->بلاک.بلاک_ڈھیر_اشاریہ;
        } وإلا {
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
    طالما (عدد_Results--) {
        نکالو(o);
    }

    بلاک->بلاک_ڈھیر_اشاریہ = o->ڈھیر_اشاریہ = ڈھیر_اشاریہ;

    // push the params back onto the stack -------------------
    لكل (u16 i = 0; i < عدد_Params; ++i) {
        m3type_t type = GetFuncTypeParamType(i_بلاک_Type, i);

        u16 تھاں = Get_تھاں_For_ڈھیر_اشاریہ(o, param_اشاریہ + i);
        ڈالو(o, type, تھاں);

        إذا (تھاں >= o->تھاں_پہلا_Dynamic_اشاریہ && تھاں != c_تھاں_بن_ورتیا) {
_           (MarkSlotsAllocatedByType(o, تھاں, type));
        }
    }

    //--------------------------------------------------------

#إذا d_m3HasExceptionHandling
    // with the scope pushed and the params back on, a catch label of 0 names
    // this block - which is what the proposal says it should
    إذا (i_بلاک_Opcode == c_wa_Op_try_جدول) {
_       (EmitCatchStubs(o));
    }
#جے_انت

_   (Compile_بلاک_Statements(o));

_   (Validate_بلاک_سرا(o));

    إذا (o->function)    // skip for expressions
    {
        إذا (not Is_ڈھیر_Polymorphic(o)) {
_           (Resolve_بلاک_Results(o, &o->بلاک, /* isBranch: */ false));
        }

#إذا d_m3HasExceptionHandling
        // Falling out of the end of a try region retires its handler. This sits
        // ahead of PatchBranches so branches into the block's exit land past it:
        // a branch out of the try popped its own handler at the branch site.
        إذا (i_بلاک_Opcode == c_wa_Op_try_جدول) {
_           (EmitOp(o, op_نکالو_Handlers));
            EmitConstant32(o, 1);
        }
#جے_انت

_       (Unwind_بلاک_ڈھیر(o))

        إذا (not ((i_بلاک_Opcode == c_waOp_if and عدد_Results) or o->پچھلا_Opcode == c_waOp_else)) {
            o->ڈھیر_اشاریہ = o->بلاک.exit_ڈھیر_اشاریہ;
_           (ڈالو_بلاک_Results(o));
        }
    }

    PatchBranches(o);

#إذا d_m3HasSnapshots
    ریکارڈ_بلاک_سرا(o);

    إذا (o->local_Init_ڈونگھائی) {
        لكل (u32 i = 0; i < o->snapshotMap->عدد_Locals; ++i) {
            إذا (o->local_Init_ڈونگھائی[i] >= (u32)o->بلاک.ڈونگھائی) {
                o->local_Init_ڈونگھائی[i] = UINT32_MAX;
            }
        }
    }
#جے_انت
    o->بلاک = outerScope;

} _catch: أرجع نتیجہ;
}

ساكن
M3_نتیجہ CompileLocals (IM3Compilation o)
{
    M3_نتیجہ نتیجہ;

    u32 عدد_Locals = 0;
    u32 عدد_Local_بلاکاں;
_   (ReadLEB_u32(&عدد_Local_بلاکاں, &o->wasm, o->wasm_سرا));

    لكل (u32 l = 0; l < عدد_Local_بلاکاں; ++l) {
        u32      var_گنتی;
        m3type_t localType;

_       (ReadLEB_u32(&var_گنتی, &o->wasm, o->wasm_سرا));
_       (Parse_قدر_Type(o->module, &localType, &o->wasm, o->wasm_سرا));
        عدد_Locals += var_گنتی;                                                          m3log (compile, "pushing locals. count: %d; type: %s", var_گنتی, c_waTypes [ادھار_Type_دا(localType)]);
        طالما (var_گنتی--) {
_           (ڈالو_Allocated_تھاں(o, localType));
        }
    }

    إذا (o->function) {
        o->function->عدد_Locals = عدد_Locals;
    }

    _catch: أرجع نتیجہ;
}

ساكن
M3_نتیجہ ReserveConstants (IM3Compilation o)
{
    M3_نتیجہ نتیجہ = m3Err_none;

    // in the interest of speed, this blindly scans the Wasm code looking for any byte
    // that looks like an const opcode.
    u16 عدد_Constant_Slots = 0;

    بائٹاں_t wa = o->wasm;
    طالما (wa < o->wasm_سرا) {
        u8  کوڈ     = *wa++;
        u16 جوڑو_Slots = 0;

        إذا (کوڈ == c_waOp_i32_const or کوڈ == c_waOp_f32_const) {
            جوڑو_Slots = 1;
        } وإلا إذا (کوڈ == c_waOp_i64_const or کوڈ == c_waOp_f64_const) {
            جوڑو_Slots = Get_Type_عدد_Slots(c_m3Type_i64);
        }

        إذا (عدد_Constant_Slots + جوڑو_Slots >= d_m3_Max_Constant_جدول_ناپ) {
            اكسر;
        }

        عدد_Constant_Slots += جوڑو_Slots;
    }

    // if constants overflow their reserved stack space, the compiler simply emits op_Const
    // operations as needed. Compiled expressions (global inits) don't pass through this
    // ReserveConstants function and thus always produce inline constants.

    Align_تھاں_منزل_Type(&عدد_Constant_Slots, c_m3Type_i64);                                           m3log (compile, "reserved constant slots: %d", عدد_Constant_Slots);

    o->تھاں_پہلا_Dynamic_اشاریہ = o->تھاں_پہلا_Const_اشاریہ + عدد_Constant_Slots;

    إذا (o->تھاں_پہلا_Dynamic_اشاریہ >= d_m3MaxFunctionSlots) {
        _throw(m3_Err_function_ڈھیر_حد_توں_ودھ);
    }

    _catch:
    أرجع نتیجہ;
}


// A constant expression compiles like a miniature function root block: no args,
// no locals and a single result, which Compile_End copies down into slot 0 for
// EvaluateExpression to read back.
M3_نتیجہ Compile_عبارت (IM3Compilation o, IM3FuncType i_نتیجہ_Type)
{
    M3_نتیجہ نتیجہ = m3Err_none;

    o->بلاک.type = i_نتیجہ_Type;

    u16 عدد_Ret_Slots = Get_Func_Type_عدد_Results(i_نتیجہ_Type) * c_io_تھاں_گنتی;

    لكل (u16 i = 0; i < عدد_Ret_Slots; ++i) {
        Mark_تھاں_Allocated(o, i);
    }

    o->max_ڈھیر_Slots = o->تھاں_Max_Allocated_اشاریہ_Plus_One = o->تھاں_پہلا_Dynamic_اشاریہ = عدد_Ret_Slots;

_   (Compile_بلاک_Statements(o));

    _catch: أرجع نتیجہ;
}


M3_نتیجہ CompileFunction (IM3Function io_function)
{
    إذا (!io_function->wasm) {
        // An import. Either linking pointed it at another module's function -
        // in which case that one carries the body - or a host function was
        // bound to it, or it is simply unsatisfied.
        IM3Function impl = Function_Implementation(io_function);

        إذا (impl != io_function) {
            إذا (not impl->compiled) {
                M3_نتیجہ r = CompileFunction(impl);
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
        M3_نتیجہ vr = ValidateFunction(io_function);
        إذا (vr) {
            أرجع vr;
        }
    }
#جے_انت

    IM3FuncType funcType = io_function->funcType;                   m3log (compile, "compiling: [%d] %s %s; wasm-size: %d",
                                                                        io_function->index, m3_GetFunctionName (io_function), SPrintFuncTypeSignature (funcType), (u32) (io_function->wasm_سرا - io_function->wasm));
    IM3Runtime runtime = io_function->module->runtime;

    IM3Compilation o = &runtime->compilation;                       d_m3Assert (d_m3MaxFunctionSlots >= d_m3_Max_Function_ڈھیر_اچائی * (d_m3_Use32_ثنائی_ہندسہ_Slots + 1))  // need twice as many slots in 32-bit mode
    memset(o, 0x0, حجم(M3Compilation));

    o->runtime    = runtime;
    o->module     = io_function->module;
    o->function   = io_function;
    o->wasm       = io_function->wasm;
    o->wasm_سرا    = io_function->wasm_سرا;
    o->بلاک.type = funcType;

_try {
    // skip over code size. the end was already calculated during parse phase
    u32 ناپ;
_   (ReadLEB_u32(&ناپ, &o->wasm, o->wasm_سرا));                     d_m3Assert (ناپ == (o->wasm_سرا - o->wasm))

#إذا d_m3HasSnapshots
    o->body_شروع = o->wasm;
#جے_انت

#إذا d_m3HasSnapshots
    // only code that can be suspended can end up in a snapshot
    إذا (runtime->isSuspendable) {
        o->snapshotMap = m3_AllocStruct(M3SnapshotMap);
        _throwifnull(o->snapshotMap);
    }
#جے_انت

_   (Acquire_Compilation_کوڈ_ورقہ(o, &o->ورقہ));

    pc_t pc = Get_ورقہ_PC(o->ورقہ);

    u16 عدد_Ret_Slots = Get_Function_عدد_Returns(o->function) * c_io_تھاں_گنتی;

    لكل (u16 i = 0; i < عدد_Ret_Slots; ++i) {
        Mark_تھاں_Allocated(o, i);
    }

    o->function->عدد_Ret_Slots = o->تھاں_پہلا_Dynamic_اشاریہ = عدد_Ret_Slots;

    u16 عدد_دلیلاں = Get_Function_عدد_دلیلاں(o->function);

    // push the arg types to the type stack
    لكل (u16 i = 0; i < عدد_دلیلاں; ++i) {
        m3type_t type = GetFunctionArgType(o->function, i);
_       (ڈالو_Allocated_تھاں(o, type));

        // prevent allocator fill-in
        o->تھاں_پہلا_Dynamic_اشاریہ += c_io_تھاں_گنتی;
    }

    o->تھاں_Max_Allocated_اشاریہ_Plus_One = o->function->عدد_Ret_And_Arg_Slots = o->تھاں_پہلا_Local_اشاریہ = o->تھاں_پہلا_Dynamic_اشاریہ;

_   (CompileLocals(o));

#إذا d_m3HasSnapshots
_   (ریکارڈ_Locals(o));
#جے_انت

#إذا d_m3HasGasMetering
    // the table charges for a function's locals as well as for its instructions.
    // op_Entry is what sets them up, so the charge rides on the body's first
    // segment, which is emitted right after it
    o->gas_لاگت = o->function->عدد_Locals * c_gasLocal;
#جے_انت

    u16 max_تھاں = Get_Max_ورتیا_تھاں_Plus_One(o);

    o->function->عدد_Local_بائٹاں = (max_تھاں - o->تھاں_پہلا_Local_اشاریہ) * حجم(m3slot_t);

    o->تھاں_پہلا_Const_اشاریہ = o->تھاں_Max_Const_اشاریہ = max_تھاں;

    // ReserveConstants initializes o->firstDynamicSlotNumber
_   (ReserveConstants(o));

    // start tracking the max stack used (Push() also updates this value) so that op_Entry can precisely detect stack overflow
    o->max_ڈھیر_Slots = o->تھاں_Max_Allocated_اشاریہ_Plus_One = o->تھاں_پہلا_Dynamic_اشاریہ;

    o->بلاک.بلاک_ڈھیر_اشاریہ = o->ڈھیر_پہلا_Dynamic_اشاریہ = o->ڈھیر_اشاریہ;                           m3log (compile, "start stack index: %u; max stack slots: %u",
                                                                                                           (u32) o->ڈھیر_پہلا_Dynamic_اشاریہ, (u32) o->max_ڈھیر_Slots);
#إذا d_m3_Has_ڈھیر_Switching
    // A suspendable body opens by checking for a pause request, once its frame
    // is set up. The pause point's offset is the first instruction's, just past
    // the local declarations - the body's end, when it has no other.
    إذا (runtime->isSuspendable) {
        o->آخری_Opcode_شروع = o->wasm;
#  إذا d_m3_اندراج_Keeps_چوکھٹا
_       (EmitOp(o, op_اندراج));
        Emit_اشارہ_گر(o, io_function);
_       (EmitOp(o, op_اندراج_Check));
_       (ریکارڈ_محفوظ_نقطہ(o, safepoint_اندراج, 1, 0, 0, عدم));
#  وإلا
_       (EmitOp(o, op_اندراج_Suspendable));
        Emit_اشارہ_گر(o, io_function);
_       (ریکارڈ_محفوظ_نقطہ(o, safepoint_اندراج, 0, 0, 0, عدم));
#  endif
    } وإلا
#جے_انت
    {
_       (EmitOp(o, op_اندراج));
        Emit_اشارہ_گر(o, io_function);
    }

_   (Compile_بلاک_Statements(o));

    _throwif(m3Err_wasmMalformed, o->پچھلا_Opcode != c_wa_Op_سرا);

    io_function->compiled      = pc;
    io_function->max_ڈھیر_Slots = o->max_ڈھیر_Slots;

    u16 عدد_Constant_Slots = o->تھاں_Max_Const_اشاریہ - o->تھاں_پہلا_Const_اشاریہ;                           m3log (compile, "unique constant slots: %u; unused slots: %u",
                                                                                                           عدد_Constant_Slots, o->تھاں_پہلا_Dynamic_اشاریہ - o->تھاں_Max_Const_اشاریہ);
    io_function->عدد_Constant_بائٹاں = عدد_Constant_Slots * حجم(m3slot_t);

    إذا (عدد_Constant_Slots) {
        io_function->constants = m3_CopyMem((cbytes_t)o->constants, io_function->عدد_Constant_بائٹاں);
        _throwifnull(io_function->constants);
    }

#إذا d_m3HasSnapshots
    io_function->snapshotMap = o->snapshotMap;
    o->snapshotMap           = عدم;
#جے_انت

} _catch:

    Release_Compilation_کوڈ_ورقہ(o);

#إذا d_m3HasSnapshots
    SnapshotMap_Free(o->snapshotMap);
    o->snapshotMap = عدم;
    m3_Free(o->local_Init_ڈونگھائی);
#جے_انت

    أرجع نتیجہ;
}

#إذا d_m3_Has_ڈھیر_Switching
m3ret_t ResumeContinuation (IM3Runtime i_runtime, IM3Continuation i_cont)
{
    إذا (M3_UNLIKELY(!i_cont || i_cont->state != cont_suspended)) {
        أرجع m3Err_none;
    }

    M3_حافظہ_سرخی* _mem = i_cont->اندراج_Function ? Module_حافظہ_سرخی(i_cont->اندراج_Function->module) : عدم;

    i_runtime->سرگرم_Continuation = i_cont;
    i_cont->state                 = cont_running;

    i32 outermost     = (i32)i_cont->عدد_چوکھٹے - 1;
    i_cont->عدد_چوکھٹے = 0;

    // with no frames standing, this lands straight on the suspension point
    m3ret_t r = Replay_چوکھٹے(i_cont, outermost, _mem);

    إذا (r == m3Err_continuationSuspended) {
        i_cont->state = cont_suspended;
    } وإلا {
        i_cont->state = cont_returned;
    }
    i_runtime->سرگرم_Continuation = عدم;

    أرجع r;
}
#جے_انت
