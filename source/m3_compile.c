//
//  m3_compile.c
//
//  Created by Steven Massey on 4/17/19.
//  Copyright © 2019 Steven Massey. All rights reserved.
//

// Allow using opcodes for compilation process
#تعریف_کړه M3_COMPILE_OPCODES

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
    أرجع Get_پاڼه_PC(o->پاڼه);
}

#إذا d_m3HasSnapshots

#  define d_m3GrowSnapshotArray(TYPE, ARRAY, شمېر_2, ظرفیت_2)                    \
    إذا ((شمېر_2) == (ظرفیت_2)) {                                                 \
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
u32 Get_Wasm_دننه_واټن (IM3Compilation o)
{
    أرجع (u32)(o->وروستی_Opcode_پیل - o->body_پیل);
}

// Whether the operand stack entry at i_index is a value the frame holds. Every
// block keeps records of its parameter and result slots on the stack, between
// the height it was entered at and where its own values begin (see
// CompileBlock). Those are landing pads rather than values: nothing need have
// written them yet, and the slots they name may be in use for something else.
// The function's own scope has none - that stretch is its arguments and locals.
ساكن
bool Is_ژوندی_ډېری_ثبت (IM3Compilation o, u16 i_شاخص)
{
    لكل (IM3CompilationScope scope = &o->بلاک; scope->outer; scope = scope->outer) {
        إذا (i_شاخص >= scope->exit_ډېری_شاخص and i_شاخص < scope->بلاک_ډېری_شاخص) {
            أرجع false;
        }
    }

    أرجع true;
}

ساكن
M3_پایله جمع_کړه_خوندي_ټکی_ارزښت (IM3Compilation o, M3_خوندي_ټکی* io_ټکی, u16 i_شاخص)
{
    M3SnapshotMap* map = o->snapshotMap;

    d_m3GrowSnapshotArray(M3_خونه_ارزښت, map->ارزښتونه, map->عدد_ارزښتونه, o->ارزښتونه_ظرفیت);

    map->ارزښتونه[map->عدد_ارزښتونه].خونه = o->wasm_ډېری[i_شاخص];
    map->ارزښتونه[map->عدد_ارزښتونه].type = o->type_ډېری[i_شاخص];
    map->عدد_ارزښتونه++;
    io_ټکی->عدد_ارزښتونه++;

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
M3_پایله ریکارډ_خوندي_ټکی (IM3Compilation o, M3_خوندي_ټکی_ډول i_ډول, u32 i_ټکي_شا, u16 i_عدد_Results,
                          u16 i_aux, IM3CompilationScope i_loop)
{
_try {
    M3SnapshotMap* map = o->snapshotMap;

    إذا (not map or not o->پاڼه) {
        أرجع m3Err_none;
    }

    d_m3GrowSnapshotArray(M3_خوندي_ټکی, map->خوندي_Points, map->عدد_خوندي_Points, o->خوندي_Points_ظرفیت);

    // grown before it is pointed at: adding values never moves the safepoints
    M3_خوندي_ټکی* ټکی     = &map->خوندي_Points[map->عدد_خوندي_Points];
    ټکی->pc              = GetPC(o) - i_ټکي_شا;
    ټکی->wasm_دننه_واټن      = Get_Wasm_دننه_واټن(o);
    ټکی->لومړی_ارزښت      = map->عدد_ارزښتونه;
    ټکی->لومړی_Local_ارزښت = UINT32_MAX;
    ټکی->عدد_ارزښتونه       = 0;
    ټکی->عدد_Results      = i_عدد_Results;
    ټکی->aux             = i_aux;
    ټکی->resumeType      = c_m3Type_none;
    ټکی->ډول            = (u8)i_ډول;
    ټکی->flags           = 0;

    // Non-defaultable locals can still be uninitialized at a safepoint. Keep
    // their heap types, but allow the null placeholder until local.set makes
    // them definitely initialized. Branches to a loop use its entry context.
    إذا (o->local_Init_ژوروالی) {
        ټکی->لومړی_Local_ارزښت = map->عدد_ارزښتونه;
        لكل (u32 i = 0; i < map->عدد_Locals; ++i) {
            d_m3GrowSnapshotArray(M3_خونه_ارزښت, map->ارزښتونه, map->عدد_ارزښتونه, o->ارزښتونه_ظرفیت);
            M3_خونه_ارزښت ارزښت = map->locals[i];
            إذا (o->local_Init_ژوروالی[i] == UINT32_MAX or
                (i_loop and o->local_Init_ژوروالی[i] >= (u32)i_loop->ژوروالی)) {
                ارزښت.type &= ~d_m3Type_refNonNull;
            }
            map->ارزښتونه[map->عدد_ارزښتونه++] = ارزښت;
        }
        ټکی->لومړی_ارزښت = map->عدد_ارزښتونه;
    }

    u16 سر = o->ډېری_شاخص;

    إذا (i_ډول != safepoint_suspend) {
        سر = (سر > i_عدد_Results) ? (u16)(سر - i_عدد_Results) : 0;
    }
    إذا (i_loop) {
        سر = M3_MIN(سر, i_loop->exit_ډېری_شاخص);
    }

    لكل (u16 i = o->ډېری_لومړی_Dynamic_شاخص; i < سر; ++i) {
        إذا (Is_ژوندی_ډېری_ثبت(o, i)) {
_           (جمع_کړه_خوندي_ټکی_ارزښت(o, ټکی, i));
        }
    }

    إذا (i_loop) {
        u16 عدد_Params = Get_Func_Type_عدد_Params(i_loop->type);

        لكل (u16 i = 0; i < عدد_Params; ++i) {
_           (جمع_کړه_خوندي_ټکی_ارزښت(o, ټکی, (u16)(i_loop->exit_ډېری_شاخص + i)));
        }
    }

    map->عدد_خوندي_Points++;

} _catch: أرجع پایله;
}

// A back edge: op_ContinueLoop, or op_ContinueLoopIf when i_isConditional, and
// the loop it goes back to. The conditional one suspends before it has taken
// the branch, so it has to be resumed with the condition that took it.
ساكن
M3_پایله ریکارډ_شا_څنډه (IM3Compilation o, IM3CompilationScope i_loop, bool i_isConditional)
{
    M3_پایله پایله = m3Err_none;

    // the loop is named by its record in the map, which has to fit the field
    _throwif("a function has too many loops for its snapshot map",
             o->snapshotMap and o->پاڼه and (i_loop->snapshot_بلاک == 0 or i_loop->snapshot_بلاک > UINT16_MAX + 1u));

_   (ریکارډ_خوندي_ټکی(o, safepoint_op, 2, 0, (u16)(i_loop->snapshot_بلاک - 1), i_loop));

    إذا (i_isConditional and o->snapshotMap and o->پاڼه) {
        o->snapshotMap->خوندي_Points[o->snapshotMap->عدد_خوندي_Points - 1].flags |= d_m3_خوندي_ټکی_اخیستل_شوی_Branch;
    }

    _catch: أرجع پایله;
}

// A call, waiting on its callee with i_numResults not yet written; the
// callee's frame starts i_calleeSlot slots above this one. A return_call that
// is compiled as a call is one too, since its frame stands while the callee
// runs, but it is marked: in Wasm the callee has replaced it, so a snapshot
// walks past it rather than writing it.
ساكن
M3_پایله ریکارډ_Call (IM3Compilation o, u16 i_عدد_Results, u16 i_callee_خونه, bool i_isReturnCall)
{
    M3_پایله پایله = ریکارډ_خوندي_ټکی(o, safepoint_call, 0, i_عدد_Results, i_callee_خونه, عدم);

    إذا (not پایله and i_isReturnCall and o->snapshotMap and o->پاڼه) {
        o->snapshotMap->خوندي_Points[o->snapshotMap->عدد_خوندي_Points - 1].flags |= d_m3_خوندي_ټکی_لکۍ_Call;
    }

    أرجع پایله;
}

// Where the function's arguments and locals are kept. Unlike the operand
// stack, that never changes, so it is recorded once rather than at every
// safepoint.
ساكن
M3_پایله ریکارډ_Locals (IM3Compilation o)
{
    M3SnapshotMap* map = o->snapshotMap;

    إذا (map and o->ډېری_شاخص) {
        map->locals = m3_AllocArray(M3_خونه_ارزښت, o->ډېری_شاخص);

        إذا (not map->locals) {
            أرجع m3Err_mallocFailed;
        }

        لكل (u16 i = 0; i < o->ډېری_شاخص; ++i) {
            map->locals[i].خونه = o->wasm_ډېری[i];
            map->locals[i].type = o->type_ډېری[i];
        }

        map->عدد_Locals = o->ډېری_شاخص;
        لكل (u32 i = Get_Function_عدد_دلیلونه(o->function); i < map->عدد_Locals; ++i) {
            إذا (IsRefType(map->locals[i].type) and not IsNullableRef(map->locals[i].type)) {
                o->local_Init_ژوروالی = m3_AllocArray(u32, map->عدد_Locals);
                إذا (not o->local_Init_ژوروالی) {
                    أرجع m3Err_mallocFailed;
                }
                لكل (u32 j = i; j < map->عدد_Locals; ++j) {
                    إذا (IsRefType(map->locals[j].type) and not IsNullableRef(map->locals[j].type)) {
                        o->local_Init_ژوروالی[j] = UINT32_MAX;
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
M3_پایله ریکارډ_بلاک_پیل (IM3Compilation o, m3opcode_t i_opcode, pc_t i_pc, u32 i_عدد_Clauses)
{
    M3SnapshotMap* map = o->snapshotMap;

    إذا (map and o->پاڼه) {
        d_m3GrowSnapshotArray(M3_بلاک_پیل, map->بلاکونه, map->عدد_بلاکونه, o->بلاکونه_ظرفیت);

        M3_بلاک_پیل* بلاک = &map->بلاکونه[map->عدد_بلاکونه++];
        بلاک->pc           = i_pc;
        بلاک->wasm_دننه_واټن   = Get_Wasm_دننه_واټن(o);
        بلاک->wasm_پای_ټکی      = UINT32_MAX;
        بلاک->عدد_Clauses   = (u16)i_عدد_Clauses;
        بلاک->opcode       = (u8)i_opcode;

        o->بلاک.snapshot_بلاک = map->عدد_بلاکونه;
    }

    أرجع m3Err_none;
}

// Closes the scope's region, while the instruction just read is its end
ساكن
فراغ ریکارډ_بلاک_پای_ټکی (IM3Compilation o)
{
    إذا (o->snapshotMap and o->بلاک.snapshot_بلاک) {
        o->snapshotMap->بلاکونه[o->بلاک.snapshot_بلاک - 1].wasm_پای_ټکی = Get_Wasm_دننه_واټن(o);
    }
}

#  undef d_m3GrowSnapshotArray

#وإلا

ساكن مضمن
M3_پایله ریکارډ_خوندي_ټکی (IM3Compilation o, M3_خوندي_ټکی_ډول i_ډول, u32 i_ټکي_شا, u16 i_عدد_Results,
                          u16 i_aux, IM3CompilationScope i_loop)
{
    (فراغ)o;
    (فراغ)i_ډول;
    (فراغ)i_ټکي_شا;
    (فراغ)i_عدد_Results;
    (فراغ)i_aux;
    (فراغ)i_loop;
    أرجع m3Err_none;
}

ساكن مضمن
M3_پایله ریکارډ_شا_څنډه (IM3Compilation o, IM3CompilationScope i_loop, bool i_isConditional)
{
    (فراغ)o;
    (فراغ)i_loop;
    (فراغ)i_isConditional;
    أرجع m3Err_none;
}

ساكن مضمن
M3_پایله ریکارډ_Call (IM3Compilation o, u16 i_عدد_Results, u16 i_callee_خونه, bool i_isReturnCall)
{
    (فراغ)o;
    (فراغ)i_عدد_Results;
    (فراغ)i_callee_خونه;
    (فراغ)i_isReturnCall;
    أرجع m3Err_none;
}

#د_که_پای // d_m3HasSnapshots

ساكن M3_NOINLINE
M3_پایله Ensure_کوډ_پاڼه_عدد_کرښې (IM3Compilation o, u32 i_عدد_کرښې)
{
    M3_پایله پایله = m3Err_none;

    i_عدد_کرښې += 2; // room for Bridge

    إذا (M3_UNLIKELY(عدد_Free_کرښې(o->پاڼه) < i_عدد_کرښې)) {
        IM3_کوډ_پاڼه پاڼه = Acquire_کوډ_پاڼه_With_ظرفیت(o->runtime, i_عدد_کرښې);

        إذا (پاڼه) {
            m3log(emit, "bridging new code page from: %d %p (free slots: %d) to: %d", o->پاڼه->info.sequence, GetPC(o), عدد_Free_کرښې(o->پاڼه), پاڼه->info.sequence);
            d_m3Assert(عدد_Free_کرښې(o->پاڼه) >= 2);

            Emit_کلمه(o->پاڼه, op_Branch);
            Emit_کلمه(o->پاڼه, Get_پاڼه_PC(پاڼه));

            Release_کوډ_پاڼه(o->runtime, o->پاڼه);

            o->پاڼه = پاڼه;
        } وإلا {
            پایله = m3_Err_malloc_Failed_کوډ_پاڼه;
        }
    }

    أرجع پایله;
}

// invalidate the pending local.set fold candidate; called wherever code is emitted or a
// branch target captures the current pc (a fold appends an immediate, moving that position)
ساكن مضمن
فراغ InvalidateFold (IM3Compilation o)
{
#إذا d_m3_Fold_سېټ_Local
    o->foldPatchPC = عدم;
#وإلا
    (فراغ)o;
#د_که_پای
}

ساكن M3_NOINLINE
M3_پایله EmitOp (IM3Compilation o, IM3Operation i_operation)
{
    M3_پایله پایله = m3Err_none;                                 d_m3Assert (i_operation or Is_ډېری_Polymorphic (o));

    InvalidateFold(o);

    // it's OK for page to be null; when compile-walking the bytecode without emitting
    إذا (o->پاڼه) {
#إذا d_m3EnableOpTracing
        إذا (i_operation != op_Dump_ډېری) {
            o->عدد_Emits++;
        }
#د_که_پای

        // have execution jump to a new page if slots are critically low
        پایله = Ensure_کوډ_پاڼه_عدد_کرښې(o, d_m3_کوډ_پاڼه_Free_کرښې_پوله);

        إذا (M3_LIKELY(not پایله)) {
            إذا (d_m3LogEmit) {
                log_emit(o, i_operation);
            }
#إذا d_m3_ریکارډ_Backtraces
            Emit_Mapping_ثبت(o->پاڼه, (u32)(o->وروستی_Opcode_پیل - o->module->wasm_پیل));
#د_که_پای // d_m3RecordBacktraces
            Emit_کلمه(o->پاڼه, i_operation);
        }
    }

    أرجع پایله;
}

// Push an immediate constant into the M3 codestream
ساكن M3_NOINLINE
فراغ EmitConstant32 (IM3Compilation o, ثابت u32 i_سمدستي)
{
    إذا (o->پاڼه) {                                                          m3log (emit, "const32: %ud", i_سمدستي);
        EmitWord32(o->پاڼه, i_سمدستي);
    }
}

#إذا d_m3HasMemory64
// Takes two lines of the code page where a pointer is 32 bits, so whatever
// reads it back has to step _pc by the same amount - see op_Const64.
ساكن M3_NOINLINE
فراغ EmitConstant64 (IM3Compilation o, ثابت u64 i_سمدستي)
{
    إذا (o->پاڼه) {
        EmitWord64(o->پاڼه, i_سمدستي);
    }
}
#د_که_پای

ساكن M3_NOINLINE
فراغ Emit_خونه_دننه_واټن (IM3Compilation o, ثابت i32 i_دننه_واټن)
{
    إذا (o->پاڼه) {                                                          m3log (emit, "slot: [%d]", i_دننه_واټن);
        EmitWord32(o->پاڼه, i_دننه_واټن);
    }
}

ساكن M3_NOINLINE
pc_t Emit_اشاره_ګر (IM3Compilation o, ثابت فراغ* ثابت i_اشاره_ګر)
{
    pc_t ptr = Get_پاڼه_PC(o->پاڼه);

    إذا (o->پاڼه) {                                                          m3log (emit, "ptr: %p", i_اشاره_ګر);
        Emit_کلمه(o->پاڼه, i_اشاره_ګر);
    }

    أرجع ptr;
}

ساكن M3_NOINLINE
فراغ* Reserve_اشاره_ګر (IM3Compilation o)
{
    pc_t ptr = Get_پاڼه_PC(o->پاڼه);
    Emit_اشاره_ګر(o, عدم);
    أرجع (فراغ*)ptr;
}


//-------------------------------------------------------------------------------------------------------------------------

#تعریف_کړه d_indent "     | %s"

// just want less letters and numbers to stare at down the way in the compiler table
#تعریف_کړه i_32    c_m3Type_i32
#تعریف_کړه i_64    c_m3Type_i64
#تعریف_کړه f_32    c_m3Type_f32
#تعریف_کړه f_64    c_m3Type_f64
#تعریف_کړه none    c_m3Type_none
#تعریف_کړه any     (u8)-1

#إذا d_m3HasFloat
#  define FPOP(x) x
#وإلا
#  define FPOP(x) عدم
#د_که_پای

// These are indexed by M3ValueType, so every type up to c_m3Type_externref needs
// an entry. A reference is one pointer-sized word, so it moves with the integer
// operation of that width; v128 has no operations at all.
#إذا M3_SIZEOF_PTR == 8
#  define REFOP(NAME)  op_##NAME##_i64
#وإلا
#  define REFOP(NAME)  op_##NAME##_i32
#د_که_پای

// clang-format off

ساكن ثابت IM3Operation c_preserve_سېټ_خونه[] = { عدم, op_Preserve_سېټ_خونه_i32,       op_Preserve_سېټ_خونه_i64,
                                                    FPOP(op_Preserve_سېټ_خونه_f32), FPOP(op_Preserve_سېټ_خونه_f64),
                                                    عدم, REFOP(Preserve_سېټ_خونه),  REFOP(Preserve_سېټ_خونه),
                                                    REFOP(Preserve_سېټ_خونه),       REFOP(Preserve_سېټ_خونه) };
ساكن ثابت IM3Operation c_سېټ_سېټ_Ops[] =       { عدم, op_سېټ_خونه_i32,               op_سېټ_خونه_i64,
                                                    FPOP(op_سېټ_خونه_f32),         FPOP(op_سېټ_خونه_f64),
                                                    عدم, REFOP(سېټ_خونه),         REFOP(سېټ_خونه),
                                                    REFOP(سېټ_خونه),               REFOP(سېټ_خونه) };
ساكن ثابت IM3Operation c_سېټ_Global_Ops[] =    { عدم, op_سېټ_Global_i32,             op_سېټ_Global_i64,
                                                    FPOP(op_سېټ_Global_f32),       FPOP(op_سېټ_Global_f64),
                                                    عدم, REFOP(سېټ_Global),       REFOP(سېټ_Global),
                                                    REFOP(سېټ_Global),             REFOP(سېټ_Global) };
ساكن ثابت IM3Operation c_سېټ_ثبتوونکی_Ops[] =  { عدم, op_سېټ_ثبتوونکی_i32,           op_سېټ_ثبتوونکی_i64,
                                                    FPOP(op_سېټ_ثبتوونکی_f32),     FPOP(op_سېټ_ثبتوونکی_f64),
                                                    عدم, REFOP(سېټ_ثبتوونکی),     REFOP(سېټ_ثبتوونکی),
                                                    REFOP(سېټ_ثبتوونکی),           REFOP(سېټ_ثبتوونکی) };

// A table shorter than the enum reads out of bounds, so tie the two together at
// build time: adding a value type must not silently outgrow these.
#تعریف_کړه d_m3_Check_Type_جدول(جدول) \
    M3_STATIC_ASSERT (حجم (جدول) / حجم (*(جدول)) == c_m3_Type_شمېر, جدول##_needs_one_entry_per_type)

d_m3_Check_Type_جدول(c_preserve_سېټ_خونه);
d_m3_Check_Type_جدول(c_سېټ_سېټ_Ops);
d_m3_Check_Type_جدول(c_سېټ_Global_Ops);
d_m3_Check_Type_جدول(c_سېټ_ثبتوونکی_Ops);

#إذا d_m3_Fold_سېټ_Local

// destination-folded variants of the hot binops, indexed by the operand-form index
// recorded at emission: [0]=_rs [1]=_sr [2]=_ss [3]=(fp _rr, never folded)
#تعریف_کړه d_foldOpList(TYPE, NAME)            { op_##TYPE##_##NAME##_rs_f, op_##TYPE##_##NAME##_sr_f, op_##TYPE##_##NAME##_ss_f, عدم }
#تعریف_کړه d_foldCommutativeOpList(TYPE, NAME) { op_##TYPE##_##NAME##_rs_f, عدم,                      op_##TYPE##_##NAME##_ss_f, عدم }

ساكن ثابت IM3Operation c_fold_i32_جمع_کړه[]         = d_foldCommutativeOpList (i32, جمع_کړه);
ساكن ثابت IM3Operation c_fold_i32_کمول[]    = d_foldOpList (i32, کمول);
ساكن ثابت IM3Operation c_fold_i32_Multiply[]    = d_foldCommutativeOpList (i32, Multiply);
ساكن ثابت IM3Operation c_fold_u32_And[]         = d_foldCommutativeOpList (u32, And);
ساكن ثابت IM3Operation c_fold_u32_Or[]          = d_foldCommutativeOpList (u32, Or);
ساكن ثابت IM3Operation c_fold_u32_Xor[]         = d_foldCommutativeOpList (u32, Xor);
ساكن ثابت IM3Operation c_fold_u32_ښويدنه_کیڼ[]   = d_foldOpList (u32, ښويدنه_کیڼ);
ساكن ثابت IM3Operation c_fold_i32_ښويدنه_ښی[]  = d_foldOpList (i32, ښويدنه_ښی);
ساكن ثابت IM3Operation c_fold_u32_ښويدنه_ښی[]  = d_foldOpList (u32, ښويدنه_ښی);

ساكن ثابت IM3Operation c_fold_i64_جمع_کړه[]         = d_foldCommutativeOpList (i64, جمع_کړه);
ساكن ثابت IM3Operation c_fold_i64_کمول[]    = d_foldOpList (i64, کمول);
ساكن ثابت IM3Operation c_fold_i64_Multiply[]    = d_foldCommutativeOpList (i64, Multiply);
ساكن ثابت IM3Operation c_fold_u64_And[]         = d_foldCommutativeOpList (u64, And);
ساكن ثابت IM3Operation c_fold_u64_Or[]          = d_foldCommutativeOpList (u64, Or);
ساكن ثابت IM3Operation c_fold_u64_Xor[]         = d_foldCommutativeOpList (u64, Xor);
ساكن ثابت IM3Operation c_fold_u64_ښويدنه_کیڼ[]   = d_foldOpList (u64, ښويدنه_کیڼ);
ساكن ثابت IM3Operation c_fold_i64_ښويدنه_ښی[]  = d_foldOpList (i64, ښويدنه_ښی);
ساكن ثابت IM3Operation c_fold_u64_ښويدنه_ښی[]  = d_foldOpList (u64, ښويدنه_ښی);

#  إذا d_m3HasFloat
ساكن ثابت IM3Operation c_fold_f32_جمع_کړه[]         = d_foldCommutativeOpList (f32, جمع_کړه);
ساكن ثابت IM3Operation c_fold_f32_کمول[]    = d_foldOpList (f32, کمول);
ساكن ثابت IM3Operation c_fold_f32_Multiply[]    = d_foldCommutativeOpList (f32, Multiply);
ساكن ثابت IM3Operation c_fold_f32_Divide[]      = d_foldOpList (f32, Divide);
ساكن ثابت IM3Operation c_fold_f64_جمع_کړه[]         = d_foldCommutativeOpList (f64, جمع_کړه);
ساكن ثابت IM3Operation c_fold_f64_کمول[]    = d_foldOpList (f64, کمول);
ساكن ثابت IM3Operation c_fold_f64_Multiply[]    = d_foldCommutativeOpList (f64, Multiply);
ساكن ثابت IM3Operation c_fold_f64_Divide[]      = d_foldOpList (f64, Divide);
#  endif

// loads are unary: [0]= address in _r0, [1]= address in a slot
#تعریف_کړه d_fold_بارول_List(DEST, SRC)       { op_##DEST##_Load_##SRC##_r_f, op_##DEST##_Load_##SRC##_s_f, عدم, عدم }

ساكن ثابت IM3Operation c_fold_i32_بارول_i32[] = d_fold_بارول_List(i32, i32);
ساكن ثابت IM3Operation c_fold_i32_بارول_i8[] = d_fold_بارول_List(i32, i8);
ساكن ثابت IM3Operation c_fold_i32_بارول_u8[] = d_fold_بارول_List(i32, u8);
ساكن ثابت IM3Operation c_fold_i32_بارول_i16[] = d_fold_بارول_List(i32, i16);
ساكن ثابت IM3Operation c_fold_i32_بارول_u16[] = d_fold_بارول_List(i32, u16);

ساكن ثابت IM3Operation c_fold_i64_بارول_i64[] = d_fold_بارول_List(i64, i64);
ساكن ثابت IM3Operation c_fold_i64_بارول_i8[] = d_fold_بارول_List(i64, i8);
ساكن ثابت IM3Operation c_fold_i64_بارول_u8[] = d_fold_بارول_List(i64, u8);
ساكن ثابت IM3Operation c_fold_i64_بارول_i16[] = d_fold_بارول_List(i64, i16);
ساكن ثابت IM3Operation c_fold_i64_بارول_u16[] = d_fold_بارول_List(i64, u16);
ساكن ثابت IM3Operation c_fold_i64_بارول_i32[] = d_fold_بارول_List(i64, i32);
ساكن ثابت IM3Operation c_fold_i64_بارول_u32[] = d_fold_بارول_List(i64, u32);
#  إذا d_m3HasFloat
ساكن ثابت IM3Operation c_fold_f32_بارول_f32[] = d_fold_بارول_List(f32, f32);
ساكن ثابت IM3Operation c_fold_f64_بارول_f64[] = d_fold_بارول_List(f64, f64);
#  endif

ساكن
IM3Operation GetFoldOp (m3opcode_t i_opcode, u8 i_form)
{
    إذا (i_form >= 4) {
        أرجع عدم;
    }

    بدل (i_opcode) {
    حالة 0x6a:  أرجع c_fold_i32_جمع_کړه        [i_form];      // i32.add
    حالة 0x6b:  أرجع c_fold_i32_کمول   [i_form];      // i32.sub
    حالة 0x6c:  أرجع c_fold_i32_Multiply   [i_form];      // i32.mul
    حالة 0x71:  أرجع c_fold_u32_And        [i_form];      // i32.and
    حالة 0x72:  أرجع c_fold_u32_Or         [i_form];      // i32.or
    حالة 0x73:  أرجع c_fold_u32_Xor        [i_form];      // i32.xor
    حالة 0x74:  أرجع c_fold_u32_ښويدنه_کیڼ  [i_form];      // i32.shl
    حالة 0x75:  أرجع c_fold_i32_ښويدنه_ښی [i_form];      // i32.shr_s
    حالة 0x76:  أرجع c_fold_u32_ښويدنه_ښی [i_form];      // i32.shr_u

    حالة 0x7c:  أرجع c_fold_i64_جمع_کړه        [i_form];      // i64.add
    حالة 0x7d:  أرجع c_fold_i64_کمول   [i_form];      // i64.sub
    حالة 0x7e:  أرجع c_fold_i64_Multiply   [i_form];      // i64.mul
    حالة 0x83:  أرجع c_fold_u64_And        [i_form];      // i64.and
    حالة 0x84:  أرجع c_fold_u64_Or         [i_form];      // i64.or
    حالة 0x85:  أرجع c_fold_u64_Xor        [i_form];      // i64.xor
    حالة 0x86:  أرجع c_fold_u64_ښويدنه_کیڼ  [i_form];      // i64.shl
    حالة 0x87:  أرجع c_fold_i64_ښويدنه_ښی [i_form];      // i64.shr_s
    حالة 0x88:  أرجع c_fold_u64_ښويدنه_ښی [i_form];      // i64.shr_u

#  إذا d_m3HasFloat
    حالة 0x92:  أرجع c_fold_f32_جمع_کړه        [i_form];      // f32.add
    حالة 0x93:  أرجع c_fold_f32_کمول   [i_form];      // f32.sub
    حالة 0x94:  أرجع c_fold_f32_Multiply   [i_form];      // f32.mul
    حالة 0x95:  أرجع c_fold_f32_Divide     [i_form];      // f32.div
    حالة 0xa0:  أرجع c_fold_f64_جمع_کړه        [i_form];      // f64.add
    حالة 0xa1:  أرجع c_fold_f64_کمول   [i_form];      // f64.sub
    حالة 0xa2:  أرجع c_fold_f64_Multiply   [i_form];      // f64.mul
    حالة 0xa3:  أرجع c_fold_f64_Divide     [i_form];      // f64.div
#  endif

    حالة 0x28:  أرجع c_fold_i32_بارول_i32   [i_form];      // i32.load
    حالة 0x2c:  أرجع c_fold_i32_بارول_i8    [i_form];      // i32.load8_s
    حالة 0x2d:  أرجع c_fold_i32_بارول_u8    [i_form];      // i32.load8_u
    حالة 0x2e:  أرجع c_fold_i32_بارول_i16   [i_form];      // i32.load16_s
    حالة 0x2f:  أرجع c_fold_i32_بارول_u16   [i_form];      // i32.load16_u

    حالة 0x29:  أرجع c_fold_i64_بارول_i64   [i_form];      // i64.load
    حالة 0x30:  أرجع c_fold_i64_بارول_i8    [i_form];      // i64.load8_s
    حالة 0x31:  أرجع c_fold_i64_بارول_u8    [i_form];      // i64.load8_u
    حالة 0x32:  أرجع c_fold_i64_بارول_i16   [i_form];      // i64.load16_s
    حالة 0x33:  أرجع c_fold_i64_بارول_u16   [i_form];      // i64.load16_u
    حالة 0x34:  أرجع c_fold_i64_بارول_i32   [i_form];      // i64.load32_s
    حالة 0x35:  أرجع c_fold_i64_بارول_u32   [i_form];      // i64.load32_u

#  إذا d_m3HasFloat
    حالة 0x2a:  أرجع c_fold_f32_بارول_f32   [i_form];      // f32.load
    حالة 0x2b:  أرجع c_fold_f64_بارول_f64   [i_form];      // f64.load
#  endif

    افتراضي: أرجع عدم;
    }
}

#د_که_پای // d_m3FoldSetLocal

// clang-format on

#إذا d_m3FuseBranch

// fused compare+branch/if variants, indexed like the fold lists: [0]=_rs [1]=_sr [2]=_ss [3]=unused.
// the [1] entries are NULL for commutative compares: the compiler never records the _sr form for them
#  define d_fuseCmpList(TYPE, NAME, ډول_2)             { op_##TYPE##_##KIND##_##NAME##_rs, op_##TYPE##_##KIND##_##NAME##_sr, op_##TYPE##_##KIND##_##NAME##_ss, عدم }
#  define d_fuseCommutativeCmpList(TYPE, NAME, ډول_2)  { op_##TYPE##_##KIND##_##NAME##_rs, عدم,                             op_##TYPE##_##KIND##_##NAME##_ss, عدم }

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

#د_که_پای // d_m3FuseBranch

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
#د_که_پای

// all args & returns are 64-bit aligned, so use 2 slots for a d_m3Use32BitSlots=1 build

ساكن
M3_پایله Acquire_Compilation_کوډ_پاڼه (IM3Compilation o, IM3_کوډ_پاڼه* o_کوډ_پاڼه)
{
    M3_پایله پایله = m3Err_none;

    IM3_کوډ_پاڼه پاڼه = Acquire_کوډ_پاڼه(o->runtime);

    إذا (پاڼه) {
#إذا (d_m3_Enable_کوډ_پاڼه_Ref_Counting)
        {
            إذا (o->function) {
                IM3Function func = o->function;
                پاڼه->info.usage_شمېر++;

                u32 index = func->عدد_کوډ_پاڼه_Refs++;
_               (m3ReallocArray(&func->کوډ_پاڼه_Refs, IM3_کوډ_پاڼه, func->عدد_کوډ_پاڼه_Refs, index));
                func->کوډ_پاڼه_Refs[index] = پاڼه;
            }
        }
#د_که_پای
    } وإلا {
        _throw(m3_Err_malloc_Failed_کوډ_پاڼه);
    }

    _catch:

    *o_کوډ_پاڼه = پاڼه;

    أرجع پایله;
}

ساكن مضمن
فراغ Release_Compilation_کوډ_پاڼه (IM3Compilation o)
{
    Release_کوډ_پاڼه(o->runtime, o->پاڼه);
}

ساكن مضمن
u16 Get_Type_عدد_Slots (m3type_t i_type)
{
    i_type = بنسټ_Type_د(i_type);

    // v128 is 16 bytes - 4 slots in 32-bit-slot mode, 2 in 64-bit.
    // (Slot-allocator only; no v128 ops execute.)
    إذا (i_type == c_m3Type_v128)
#إذا d_m3_Use32_بټ_Slots
        أرجع 4;
#وإلا
        أرجع 2;
#د_که_پای
#إذا d_m3_Use32_بټ_Slots
    أرجع Is64_بټ_Type(i_type) ? 2 : 1;
#وإلا
    أرجع 1;
#د_که_پای
}

ساكن مضمن
فراغ Align_خونه_منزل_Type (u16* io_خونه, m3type_t i_type)
{
    // align 64-bit words to even slots (if d_m3Use32BitSlots)
    u16 عدد_Slots = Get_Type_عدد_Slots(i_type);

    u16 ماسک = عدد_Slots - 1;
    *io_خونه = (*io_خونه + ماسک) & ~ماسک;
}

// Deliberately unchecked. Validation rejects an empty operand stack before compilation.
ساكن مضمن
i16 Get_ډېری_سر_شاخص (IM3Compilation o)
{
    أرجع o->ډېری_شاخص - 1;
}

ساكن مضمن
M3_پایله Get_ډېری_سر_شاخص_Throws (IM3Compilation o, i16* o_ډېری_شاخص)
{
    *o_ډېری_شاخص = o->ډېری_شاخص - 1;

    إذا (M3_LIKELY(o->ډېری_شاخص > o->ډېری_لومړی_Dynamic_شاخص or Is_ډېری_Polymorphic(o))) {
        أرجع m3Err_none;
    } وإلا {
        أرجع m3_Err_function_ډېری_Underrun;
    }
}


// Items in the static portion of the stack (args/locals) are hidden from GetStackTypeFromTop ()
// In other words, only "real" Wasm stack items can be inspected.  This is important when
// returning values, etc. and you need an accurate wasm-view of the stack.
ساكن
m3type_t Get_ډېری_Type_From_سر (IM3Compilation o, u16 i_دننه_واټن)
{
    m3type_t type = c_m3Type_none;

    ++i_دننه_واټن;
    إذا (o->ډېری_شاخص >= i_دننه_واټن) {
        u16 index = o->ډېری_شاخص - i_دننه_واټن;

        إذا (index >= o->ډېری_لومړی_Dynamic_شاخص) {
            type = o->type_ډېری[index];
        }
    }

    أرجع type;
}

ساكن مضمن
m3type_t Get_ډېری_سر_Type (IM3Compilation o)
{
    أرجع Get_ډېری_Type_From_سر(o, 0);
}

ساكن مضمن
m3type_t Get_ډېری_Type_From_Bottom (IM3Compilation o, u16 i_دننه_واټن)
{
    m3type_t type = c_m3Type_none;

    إذا (i_دننه_واټن < o->ډېری_شاخص) {
        type = o->type_ډېری[i_دننه_واټن];
    }

    أرجع type;
}


ساكن مضمن
bool Is_Constant_خونه (IM3Compilation o, u16 i_خونه)
{
    أرجع (i_خونه >= o->خونه_لومړی_Const_شاخص and i_خونه < o->خونه_Max_Const_شاخص);
}
ساكن مضمن
bool Is_خونه_Allocated (IM3Compilation o, u16 i_خونه)
{
    أرجع o->m3Slots[i_خونه];
}

ساكن مضمن
bool Is_ډېری_شاخص_دننه_ثبتوونکی (IM3Compilation o, i32 i_ډېری_شاخص)
{                                                                           d_m3Assert (i_ډېری_شاخص < o->ډېری_شاخص or Is_ډېری_Polymorphic (o));
    إذا (i_ډېری_شاخص >= 0 and i_ډېری_شاخص < o->ډېری_شاخص) {
        أرجع (o->wasm_ډېری[i_ډېری_شاخص] >= d_m3_Reg0_خونه_Alias);
    } وإلا {
        أرجع false;
    }
}

ساكن مضمن
u16 Get_عدد_بلاک_ارزښتونه_On_ډېری (IM3Compilation o)
{
    أرجع o->ډېری_شاخص - o->بلاک.بلاک_ډېری_شاخص;
}

ساكن مضمن
bool Is_ډېری_سر_دننه_ثبتوونکی (IM3Compilation o)
{
    أرجع Is_ډېری_شاخص_دننه_ثبتوونکی(o, (i32)Get_ډېری_سر_شاخص(o));
}
ساكن مضمن
bool Is_ډېری_سر_Minus1_دننه_ثبتوونکی (IM3Compilation o)
{
    أرجع Is_ډېری_شاخص_دننه_ثبتوونکی(o, (i32)Get_ډېری_سر_شاخص(o) - 1);
}
ساكن مضمن
bool Is_ډېری_سر_Minus2_دننه_ثبتوونکی (IM3Compilation o)
{
    أرجع Is_ډېری_شاخص_دننه_ثبتوونکی(o, (i32)Get_ډېری_سر_شاخص(o) - 2);
}

ساكن مضمن
bool Is_ډېری_سر_دننه_خونه (IM3Compilation o)
{
    أرجع not Is_ډېری_سر_دننه_ثبتوونکی(o);
}

ساكن مضمن
bool Is_Valid_خونه (u16 i_خونه)
{
    أرجع (i_خونه < d_m3MaxFunctionSlots);
}

ساكن مضمن
u16 Get_ډېری_سر_خونه_شمېره (IM3Compilation o)
{
    i16 i = Get_ډېری_سر_شاخص(o);

    u16 خونه = c_خونه_ناکارول_شوی;

    إذا (i >= 0) {
        خونه = o->wasm_ډېری[i];
    }

    أرجع خونه;
}


// from bottom
ساكن مضمن
u16 Get_خونه_For_ډېری_شاخص (IM3Compilation o, u16 i_ډېری_شاخص)
{                                                                   d_m3Assert (i_ډېری_شاخص < o->ډېری_شاخص or Is_ډېری_Polymorphic (o));
    u16 خونه = c_خونه_ناکارول_شوی;

    إذا (i_ډېری_شاخص < o->ډېری_شاخص) {
        خونه = o->wasm_ډېری[i_ډېری_شاخص];
    }

    أرجع خونه;
}

ساكن مضمن
u16 Get_Extra_خونه_For_ډېری_شاخص (IM3Compilation o, u16 i_ډېری_شاخص)
{
    u16 بنسټ_خونه = Get_خونه_For_ډېری_شاخص(o, i_ډېری_شاخص);

    إذا (بنسټ_خونه != c_خونه_ناکارول_شوی) {
        u16 extra_خونه = Get_Type_عدد_Slots(Get_ډېری_Type_From_Bottom(o, i_ډېری_شاخص)) - 1;
        بنسټ_خونه += extra_خونه;
    }

    أرجع بنسټ_خونه;
}


ساكن مضمن
فراغ Touch_خونه (IM3Compilation o, u16 i_خونه)
{
    // op_Entry uses this value to track and detect stack overflow
    o->max_ډېری_Slots = M3_MAX(o->max_ډېری_Slots, i_خونه + 1);
}

ساكن مضمن
فراغ Mark_خونه_Allocated (IM3Compilation o, u16 i_خونه)
{                                                                   d_m3Assert (o->m3Slots [i_خونه] == 0); // shouldn't be already allocated
    o->m3Slots[i_خونه] = 1;

    o->خونه_Max_Allocated_شاخص_Plus_One = M3_MAX(o->خونه_Max_Allocated_شاخص_Plus_One, i_خونه + 1);

    Touch_خونه(o, i_خونه);
}

ساكن مضمن
M3_پایله MarkSlotsAllocated (IM3Compilation o, u16 i_خونه, u16 i_عدد_Slots)
{
    إذا (M3_UNLIKELY(i_خونه + i_عدد_Slots > d_m3MaxFunctionSlots)) {
        أرجع m3_Err_function_ډېری_ډکېدنه;
    }

    طالما (i_عدد_Slots--) {
        Mark_خونه_Allocated(o, i_خونه++);
    }

    أرجع m3Err_none;
}

ساكن مضمن
M3_پایله MarkSlotsAllocatedByType (IM3Compilation o, u16 i_خونه, m3type_t i_type)
{
    u16 عدد_Slots = Get_Type_عدد_Slots(i_type);
    أرجع MarkSlotsAllocated(o, i_خونه, عدد_Slots);
}


ساكن
M3_پایله AllocateSlotsWithinRange (IM3Compilation o, u16* o_خونه, m3type_t i_type, u16 i_پیل_خونه, u16 i_پای_ټکی_خونه)
{
    M3_پایله پایله = m3_Err_function_ډېری_ډکېدنه;

    u16 عدد_Slots     = Get_Type_عدد_Slots(i_type);
    u16 پلټنه_دننه_واټن = عدد_Slots - 1;

    Align_خونه_منزل_Type(&i_پیل_خونه, i_type);

    // search for 1 or 2 consecutive slots in the execution stack
    u16 i = i_پیل_خونه;
    طالما (i + پلټنه_دننه_واټن < i_پای_ټکی_خونه) {
        إذا (i + پلټنه_دننه_واټن < d_m3MaxFunctionSlots and o->m3Slots[i] == 0 and o->m3Slots[i + پلټنه_دننه_واټن] == 0) {
            MarkSlotsAllocated(o, i, عدد_Slots);

            *o_خونه = i;
            پایله  = m3Err_none;
            اكسر;
        }

        // keep 2-slot allocations even-aligned
        i += عدد_Slots;
    }

    أرجع پایله;
}

ساكن مضمن
M3_پایله AllocateSlots (IM3Compilation o, u16* o_خونه, m3type_t i_type)
{
    أرجع AllocateSlotsWithinRange(o, o_خونه, i_type, o->خونه_لومړی_Dynamic_شاخص, d_m3MaxFunctionSlots);
}

ساكن مضمن
M3_پایله AllocateConstantSlots (IM3Compilation o, u16* o_خونه, m3type_t i_type)
{
    u16 max_جدول_شاخص = o->خونه_لومړی_Const_شاخص + d_m3_Max_Constant_جدول_اندازه;
    أرجع AllocateSlotsWithinRange(o, o_خونه, i_type, o->خونه_لومړی_Const_شاخص, M3_MIN(o->خونه_لومړی_Dynamic_شاخص, max_جدول_شاخص));
}


// TOQUE: this usage count system could be eliminated. real world code doesn't frequently trigger it.  just copy to multiple
// unique slots.
ساكن مضمن
M3_پایله Increment_خونه_Usage_شمېر (IM3Compilation o, u16 i_خونه)
{                                                                                       d_m3Assert (i_خونه < d_m3MaxFunctionSlots);
    M3_پایله پایله = m3Err_none;                                                       d_m3Assert (o->m3Slots [i_خونه] > 0);

    // OPTZ (memory): 'm3Slots' could still be fused with 'typeStack' if 4 bits were used to indicate: [0,1,2,many]. The many-case
    // would scan 'wasmStack' to determine the actual usage count
    إذا (M3_LIKELY(o->m3Slots[i_خونه] < 0xFF)) {
        o->m3Slots[i_خونه]++;
    } وإلا {
        پایله = "slot usage count overflow";
    }

    أرجع پایله;
}

ساكن مضمن
فراغ Deallocate_خونه (IM3Compilation o, i16 i_خونه, m3type_t i_type)
{                                                                                       d_m3Assert (i_خونه >= o->خونه_لومړی_Dynamic_شاخص);
                                                                                        d_m3Assert (i_خونه < o->خونه_Max_Allocated_شاخص_Plus_One);
    لكل (u16 i = 0; i < Get_Type_عدد_Slots(i_type); ++i, ++i_خونه) {                       d_m3Assert (o->m3Slots [i_خونه]);
        --o->m3Slots[i_خونه];
    }
}


ساكن مضمن
bool Is_ثبتوونکی_Type_Allocated (IM3Compilation o, u8 i_type)
{
    أرجع Is_ثبتوونکی_Allocated(o, IsFpType(i_type));
}

ساكن مضمن
فراغ Allocate_ثبتوونکی (IM3Compilation o, u32 i_ثبتوونکی, u16 i_ډېری_شاخص)
{                                                                                       d_m3Assert (not Is_ثبتوونکی_Allocated (o, i_ثبتوونکی));
    o->reg_ډېری_شاخص_Plus_One[i_ثبتوونکی] = i_ډېری_شاخص + 1;
}

ساكن مضمن
فراغ Deallocate_ثبتوونکی (IM3Compilation o, u32 i_ثبتوونکی)
{                                                                                       d_m3Assert (Is_ثبتوونکی_Allocated (o, i_ثبتوونکی));
    o->reg_ډېری_شاخص_Plus_One[i_ثبتوونکی] = c_m3_ثبتوونکی_Unallocated;
}

ساكن مضمن
u16 Get_ثبتوونکی_ډېری_شاخص (IM3Compilation o, u32 i_ثبتوونکی)
{                                                                                       d_m3Assert (Is_ثبتوونکی_Allocated (o, i_ثبتوونکی));
    أرجع o->reg_ډېری_شاخص_Plus_One[i_ثبتوونکی] - 1;
}

u16 Get_Max_کارول_شوی_خونه_Plus_One (IM3Compilation o)
{
    طالما (o->خونه_Max_Allocated_شاخص_Plus_One > o->خونه_لومړی_Dynamic_شاخص) {
        إذا (Is_خونه_Allocated(o, o->خونه_Max_Allocated_شاخص_Plus_One - 1)) {
            اكسر;
        }

        o->خونه_Max_Allocated_شاخص_Plus_One--;
    }

#که_تعریف_شوی DEBUG
    u16 max_خونه = o->خونه_Max_Allocated_شاخص_Plus_One;
    طالما (max_خونه < d_m3MaxFunctionSlots) {
        d_m3Assert(o->m3Slots[max_خونه] == 0);
        max_خونه++;
    }
#د_که_پای

    أرجع o->خونه_Max_Allocated_شاخص_Plus_One;
}

ساكن
M3_پایله Preserve_ثبتوونکی_If_Occupied (IM3Compilation o, m3type_t i_ثبتوونکی_Type)
{
    M3_پایله پایله = m3Err_none;

    u32 regSelect = IsFpType(i_ثبتوونکی_Type);

    إذا (Is_ثبتوونکی_Allocated(o, regSelect)) {
        u16 ډېری_شاخص = Get_ثبتوونکی_ډېری_شاخص(o, regSelect);
        Deallocate_ثبتوونکی(o, regSelect);

        m3type_t type = Get_ډېری_Type_From_Bottom(o, ډېری_شاخص);

        // and point to a exec slot
        u16 خونه = c_خونه_ناکارول_شوی;
_       (AllocateSlots(o, &خونه, type));
        o->wasm_ډېری[ډېری_شاخص] = خونه;

        // Ensure type is within the valid range
        إذا (M3_LIKELY(بنسټ_Type_د(type) < c_m3_Type_شمېر)) {
_           (EmitOp(o, c_سېټ_سېټ_Ops[بنسټ_Type_د(type)]));
        } وإلا {
            _throw(m3Err_unknownType);
        }

        Emit_خونه_دننه_واټن(o, خونه);
    }

    _catch: أرجع پایله;
}


// all values must be in slots before entering loop, if, and else blocks
// otherwise they'd end up preserve-copied in the block to probably different locations (if/else)
ساكن مضمن
M3_پایله PreserveRegisters (IM3Compilation o)
{
    M3_پایله پایله;

_   (Preserve_ثبتوونکی_If_Occupied(o, c_m3Type_f64));
_   (Preserve_ثبتوونکی_If_Occupied(o, c_m3Type_i64));

    _catch: أرجع پایله;
}

ساكن
M3_پایله Preserve_Non_سر_Registers (IM3Compilation o)
{
    M3_پایله پایله = m3Err_none;

    i16 ډېری_سر = Get_ډېری_سر_شاخص(o);

    إذا (ډېری_سر >= 0) {
        إذا (Is_ثبتوونکی_Allocated(o, 0))     // r0
        {
            إذا (Get_ثبتوونکی_ډېری_شاخص(o, 0) != ډېری_سر) {
_               (Preserve_ثبتوونکی_If_Occupied(o, c_m3Type_i64));
            }
        }

        إذا (Is_ثبتوونکی_Allocated(o, 1))     // fp0
        {
            إذا (Get_ثبتوونکی_ډېری_شاخص(o, 1) != ډېری_سر) {
_               (Preserve_ثبتوونکی_If_Occupied(o, c_m3Type_f64));
            }
        }
    }

    _catch: أرجع پایله;
}


//----------------------------------------------------------------------------------------------------------------------

ساكن
M3_پایله ټېل (IM3Compilation o, m3type_t i_type, u16 i_خونه)
{
    M3_پایله پایله = m3Err_none;

#إذا !d_m3HasFloat
    إذا (i_type == c_m3Type_f32 || i_type == c_m3Type_f64) {
        أرجع m3Err_unknownOpcode;
    }
#د_که_پای

    إذا (M3_UNLIKELY(o->ډېری_شاخص >= d_m3_Max_Function_ډېری_لوړوالی)) {
        أرجع m3_Err_function_ډېری_ډکېدنه;
    }

    u16 ډېری_شاخص = o->ډېری_شاخص++;                                       // printf ("push: %d\n", (i32) i);

    o->wasm_ډېری[ډېری_شاخص] = i_خونه;
    o->type_ډېری[ډېری_شاخص] = i_type;

    إذا (Is_ثبتوونکی_خونه_Alias(i_خونه)) {
        u32 regSelect = Is_Fp_ثبتوونکی_خونه_Alias(i_خونه);
        Allocate_ثبتوونکی(o, regSelect, ډېری_شاخص);
    }

    إذا (d_m3_Log_Wasm_ډېری) {
        dump_type_ډېری(o);
    }

    أرجع پایله;
}

ساكن مضمن
M3_پایله ټېل_ثبتوونکی (IM3Compilation o, m3type_t i_type)
{
    M3_پایله پایله = m3Err_none;                                                       d_m3Assert ((u16) d_m3_Reg0_خونه_Alias > (u16) d_m3MaxFunctionSlots);
    u16      خونه   = IsFpType(i_type) ? d_m3_Fp0_خونه_Alias : d_m3_Reg0_خونه_Alias;          d_m3Assert (i_type or Is_ډېری_Polymorphic (o));

_   (ټېل(o, i_type, خونه));

    _catch: أرجع پایله;
}

ساكن
M3_پایله راوباسه (IM3Compilation o)
{
    M3_پایله پایله = m3Err_none;

    إذا (M3_LIKELY(o->ډېری_شاخص > o->بلاک.بلاک_ډېری_شاخص)) {
        o->ډېری_شاخص--;                                                //  printf ("pop: %d\n", (i32) o->stackIndex);

        u16      خونه = o->wasm_ډېری[o->ډېری_شاخص];
        m3type_t type = o->type_ډېری[o->ډېری_شاخص];

        إذا (Is_ثبتوونکی_خونه_Alias(خونه)) {
            u32 regSelect = Is_Fp_ثبتوونکی_خونه_Alias(خونه);
            Deallocate_ثبتوونکی(o, regSelect);
        } وإلا إذا (M3_UNLIKELY(خونه >= o->خونه_Max_Allocated_شاخص_Plus_One)) {
            أرجع m3_Err_function_ډېری_Underrun; // Return error for invalid slot indices
        } وإلا إذا (خونه >= o->خونه_لومړی_Dynamic_شاخص) {
            Deallocate_خونه(o, خونه, type);
        }
    } وإلا إذا (not Is_ډېری_Polymorphic(o)) {
        پایله = m3_Err_function_ډېری_Underrun;
    }

    أرجع پایله;
}

ساكن
M3_پایله راوباسه_Type (IM3Compilation o, m3type_t i_type)
{
    M3_پایله پایله = m3Err_none;

    m3type_t سر_Type = Get_ډېری_سر_Type(o);

    إذا (M3_LIKELY(Is_Sub_Type_د(سر_Type, i_type) or o->بلاک.isPolymorphic)) {
_       (راوباسه(o));
    } وإلا {
        _throw(m3Err_typeMismatch);
    }

    _catch:
    أرجع پایله;
}

ساكن
M3_پایله ټېل_Allocated_خونه_And_Emit (IM3Compilation o, m3type_t i_type, bool i_doEmit)
{
    M3_پایله پایله = m3Err_none;

    u16 خونه = c_خونه_ناکارول_شوی;

_   (AllocateSlots(o, &خونه, i_type));
_   (ټېل(o, i_type, خونه));

    إذا (i_doEmit) {
        Emit_خونه_دننه_واټن(o, خونه);
    }

    //    printf ("push: %d\n", (u32) slot);

    _catch: أرجع پایله;
}

ساكن مضمن
M3_پایله ټېل_Allocated_خونه_And_Emit_2 (IM3Compilation o, m3type_t i_type)
{
    أرجع ټېل_Allocated_خونه_And_Emit(o, i_type, true);
}

ساكن مضمن
M3_پایله ټېل_Allocated_خونه (IM3Compilation o, m3type_t i_type)
{
    أرجع ټېل_Allocated_خونه_And_Emit(o, i_type, false);
}

ساكن
M3_پایله ټېل_Const (IM3Compilation o, u64 i_کلمه, m3type_t i_type)
{
    M3_پایله پایله = m3Err_none;

    // When compile-walking a constant expression without emitting there is no
    // constant table to place the value in, but the type still has to land on
    // the stack: extended-const arithmetic downstream consumes it.
    إذا (M3_UNLIKELY(!o->پاڼه)) {
        أرجع ټېل_Allocated_خونه(o, i_type);
    }

    bool سمون_وموندل  = false;
    bool is64_بټ_Type = Is64_بټ_Type(i_type);

    u16 عدد_Required_Slots  = Get_Type_عدد_Slots(i_type);
    u16 عدد_کارول_شوی_Const_Slots = o->خونه_Max_Const_شاخص - o->خونه_لومړی_Const_شاخص;

    // search for duplicate matching constant slot to reuse
    إذا (عدد_Required_Slots == 2 and عدد_کارول_شوی_Const_Slots >= 2) {
        u16 لومړی_Const_خونه = o->خونه_لومړی_Const_شاخص;
        Align_خونه_منزل_Type(&لومړی_Const_خونه, c_m3Type_i64);

        لكل (u16 خونه = لومړی_Const_خونه; خونه < o->خونه_Max_Const_شاخص - 1; خونه += 2) {
            إذا (Is_خونه_Allocated(o, خونه) and Is_خونه_Allocated(o, خونه + 1)) {
                u64 constant;
                حافظه_نقل(&constant, &o->constants[خونه - o->خونه_لومړی_Const_شاخص], حجم(constant));

                إذا (constant == i_کلمه) {
                    سمون_وموندل = true;
_                   (ټېل(o, i_type, خونه));
                    اكسر;
                }
            }
        }
    } وإلا إذا (عدد_Required_Slots == 1) {
        لكل (u16 i = 0; i < عدد_کارول_شوی_Const_Slots; ++i) {
            u16 خونه = o->خونه_لومړی_Const_شاخص + i;

            إذا (Is_خونه_Allocated(o, خونه)) {
                bool matches;
                إذا (is64_بټ_Type) {
                    u64 constant;
                    حافظه_نقل(&constant, &o->constants[i], حجم(constant));
                    matches = (constant == i_کلمه);
                } وإلا {
                    u32 constant;
                    حافظه_نقل(&constant, &o->constants[i], حجم(constant));
                    matches = (constant == i_کلمه);
                }
                إذا (matches) {
                    سمون_وموندل = true;
_                   (ټېل(o, i_type, خونه));
                    اكسر;
                }
            }
        }
    }

    إذا (not سمون_وموندل) {
        u16 خونه = c_خونه_ناکارول_شوی;
        پایله   = AllocateConstantSlots(o, &خونه, i_type);

        إذا (پایله || خونه == c_خونه_ناکارول_شوی) // no more constant table space; use inline constants
        {
            پایله = m3Err_none;

            إذا (is64_بټ_Type) {
_               (EmitOp(o, op_Const64));
                EmitWord64(o->پاڼه, i_کلمه);
            } وإلا {
_               (EmitOp(o, op_Const32));
                EmitWord32(o->پاڼه, (u32)i_کلمه);
            }

_           (ټېل_Allocated_خونه_And_Emit_2(o, i_type));
        } وإلا {
            u16 const_جدول_شاخص = خونه - o->خونه_لومړی_Const_شاخص;

            d_m3Assert(const_جدول_شاخص < d_m3_Max_Constant_جدول_اندازه);

            إذا (is64_بټ_Type) {
                حافظه_نقل(&o->constants[const_جدول_شاخص], &i_کلمه, حجم(i_کلمه));
            } وإلا {
                u32 word32 = (u32)i_کلمه;
                حافظه_نقل(&o->constants[const_جدول_شاخص], &word32, حجم(word32));
            }

_           (ټېل(o, i_type, خونه));

            o->خونه_Max_Const_شاخص = M3_MAX(خونه + عدد_Required_Slots, o->خونه_Max_Const_شاخص);
        }
    }

    _catch: أرجع پایله;
}

ساكن مضمن
M3_پایله Emit_خونه_عدد_د_ډېری_سر_And_راوباسه (IM3Compilation o)
{
    // no emit if value is in register
    إذا (Is_ډېری_سر_دننه_خونه(o)) {
        Emit_خونه_دننه_واټن(o, Get_ډېری_سر_خونه_شمېره(o));
    }

    أرجع راوباسه(o);
}


// Or, maybe: EmitTrappingOp
M3_پایله جمع_کړه_Trap_ریکارډ (IM3Compilation o)
{
    M3_پایله پایله = m3Err_none;

    إذا (o->function) {
    }

    أرجع پایله;
}

ساكن
M3_پایله Unwind_بلاک_ډېری (IM3Compilation o)
{
    M3_پایله پایله = m3Err_none;

    u32 راوباسه_شمېر = 0;
    طالما (o->ډېری_شاخص > o->بلاک.بلاک_ډېری_شاخص) {
_       (راوباسه(o));
        ++راوباسه_شمېر;
    }

    إذا (راوباسه_شمېر) {
        m3log(compile, "unwound stack top: %d", راوباسه_شمېر);
    }

    _catch: أرجع پایله;
}

ساكن مضمن
M3_پایله سېټ_ډېری_Polymorphic (IM3Compilation o)
{
    o->بلاک.isPolymorphic = true;                              m3log (compile, "stack set polymorphic");
    أرجع Unwind_بلاک_ډېری(o);
}

ساكن
فراغ PatchBranches (IM3Compilation o)
{
    InvalidateFold(o);         // patched branches land at the current pc

    pc_t pc = GetPC(o);

    pc_t patches     = o->بلاک.patches;
    o->بلاک.patches = عدم;

    طالما (patches) {                                           m3log (compile, "patching location: %p to pc: %p", patches, pc);
        pc_t بل       = *(pc_t*)patches;
        *(pc_t*)patches = pc;
        patches         = بل;
    }
}

//-------------------------------------------------------------------------------------------------------------------------

ساكن
M3_پایله Copy_ډېری_شاخص_منزل_خونه (IM3Compilation o, u16 i_dest_خونه, u16 i_ډېری_شاخص)  // NoPushPop
{
    M3_پایله پایله = m3Err_none;

    IM3Operation op;

    m3type_t type       = Get_ډېری_Type_From_Bottom(o, i_ډېری_شاخص);
    bool     دننه_ثبتوونکی = Is_ډېری_شاخص_دننه_ثبتوونکی(o, i_ډېری_شاخص);

    إذا (دننه_ثبتوونکی) {
        op = c_سېټ_سېټ_Ops[بنسټ_Type_د(type)];
    } وإلا {
        op = Is64_بټ_Type(type) ? op_Copy_خونه_64 : op_Copy_خونه_32;
    }

_   (EmitOp(o, op));
    Emit_خونه_دننه_واټن(o, i_dest_خونه);

    إذا (not دننه_ثبتوونکی) {
        u16 src_خونه = Get_خونه_For_ډېری_شاخص(o, i_ډېری_شاخص);
        Emit_خونه_دننه_واټن(o, src_خونه);
    }

    _catch: أرجع پایله;
}

ساكن
M3_پایله Copy_ډېری_سر_منزل_خونه (IM3Compilation o, u16 i_dest_خونه)  // NoPushPop
{
    M3_پایله پایله;

    i16 ډېری_سر;
_   (Get_ډېری_سر_شاخص_Throws(o, &ډېری_سر));
_   (Copy_ډېری_شاخص_منزل_خونه(o, i_dest_خونه, (u16)ډېری_سر));

    _catch: أرجع پایله;
}


// a copy-on-write strategy is used with locals. when a get local occurs, it's not copied anywhere. the stack
// entry just has a index pointer to that local memory slot.
// then, when a previously referenced local is set, the current value needs to be preserved for those references

// TODO: consider getting rid of these specialized operations: PreserveSetSlot & PreserveCopySlot.
// They likely just take up space (which seems to reduce performance) without improving performance.
// => unconfirmed in preliminary testing.
ساكن
M3_پایله Preserved_Copy_سر_خونه (IM3Compilation o, u16 i_dest_خونه, u16 i_preserve_خونه)
{
    M3_پایله پایله = m3Err_none;             d_m3Assert (i_dest_خونه != i_preserve_خونه);

    IM3Operation op;

    m3type_t type = Get_ډېری_سر_Type(o);

    إذا (Is_ډېری_سر_دننه_ثبتوونکی(o)) {
        op = c_preserve_سېټ_خونه[بنسټ_Type_د(type)];
    } وإلا {
        op = Is64_بټ_Type(type) ? op_Preserve_Copy_خونه_64 : op_Preserve_Copy_خونه_32;
    }

_   (EmitOp(o, op));
    Emit_خونه_دننه_واټن(o, i_dest_خونه);

    إذا (Is_ډېری_سر_دننه_خونه(o)) {
        Emit_خونه_دننه_واټن(o, Get_ډېری_سر_خونه_شمېره(o));
    }

    Emit_خونه_دننه_واټن(o, i_preserve_خونه);

    _catch: أرجع پایله;
}

ساكن
M3_پایله Copy_ډېری_سر_منزل_ثبتوونکی (IM3Compilation o, bool i_update_ډېری)
{
    M3_پایله پایله = m3Err_none;

    إذا (Is_ډېری_سر_دننه_خونه(o)) {
        m3type_t type = Get_ډېری_سر_Type(o);

_       (Preserve_ثبتوونکی_If_Occupied(o, type));

        IM3Operation op = c_سېټ_ثبتوونکی_Ops[بنسټ_Type_د(type)];

_       (EmitOp(o, op));
        Emit_خونه_دننه_واټن(o, Get_ډېری_سر_خونه_شمېره(o));

        إذا (i_update_ډېری) {
_           (راوباسه_Type(o, type));
_           (ټېل_ثبتوونکی(o, type));
        }
    }

    _catch: أرجع پایله;
}


// if local is unreferenced, o_preservedSlotNumber will be equal to localIndex on return
ساكن
M3_پایله پیدا_کړه_Referenced_Local_Within_Current_بلاک (IM3Compilation o, u16* o_preserved_خونه_شمېره, u32 i_local_خونه)
{
    M3_پایله پایله = m3Err_none;

    IM3CompilationScope scope = &o->بلاک;

    u16 پیل_شاخص = scope->بلاک_ډېری_شاخص;

    طالما (scope->opcode == c_wa_Op_بلاک) {
        scope = scope->outer;
        إذا (not scope) {
            اكسر;
        }

        پیل_شاخص = scope->بلاک_ډېری_شاخص;
    }

    *o_preserved_خونه_شمېره = (u16)i_local_خونه;

    لكل (u32 i = پیل_شاخص; i < o->ډېری_شاخص; ++i) {
        إذا (o->wasm_ډېری[i] == i_local_خونه) {
            m3type_t type = Get_ډېری_Type_From_Bottom(o, i);                         d_m3Assert (type != c_m3Type_none)

            إذا (*o_preserved_خونه_شمېره == i_local_خونه) {
_               (AllocateSlots(o, o_preserved_خونه_شمېره, type));
            } وإلا {
                // A 64-bit value spans two slots, and allocating or releasing it
                // counts both, so a second reference has to count both as well.
                لكل (u16 s = 0; s < Get_Type_عدد_Slots(type); ++s) {
_                   (Increment_خونه_Usage_شمېر(o, (u16)(*o_preserved_خونه_شمېره + s)));
                }
            }

            o->wasm_ډېری[i] = *o_preserved_خونه_شمېره;
        }
    }

    _catch: أرجع پایله;
}

ساكن
M3_پایله Get_بلاک_Scope (IM3Compilation o, IM3CompilationScope* o_scope, u32 i_ژوروالی)
{
    M3_پایله پایله = m3Err_none;

    IM3CompilationScope scope = &o->بلاک;

    طالما (i_ژوروالی--) {
        scope = scope->outer;
        _throwif("invalid block depth", not scope);
    }

    *o_scope = scope;

    _catch:
    أرجع پایله;
}

ساكن
M3_پایله Copy_ډېری_Slots_R (IM3Compilation o, u16 i_موخه_خونه_ډېری_شاخص, u16 i_ډېری_شاخص, u16 i_پای_ټکی_ډېری_شاخص, u16 i_لنډمهاله_خونه)
{
    M3_پایله پایله = m3Err_none;

    إذا (i_ډېری_شاخص < i_پای_ټکی_ډېری_شاخص) {
        u16 src_خونه = Get_خونه_For_ډېری_شاخص(o, i_ډېری_شاخص);

        m3type_t type      = Get_ډېری_Type_From_Bottom(o, i_ډېری_شاخص);
        u16      عدد_Slots  = Get_Type_عدد_Slots(type);
        u16      extra_خونه = عدد_Slots - 1;

        u16 موخه_خونه = Get_خونه_For_ډېری_شاخص(o, i_موخه_خونه_ډېری_شاخص);

        u16 preserve_شاخص = i_ډېری_شاخص;
        u16 collision_خونه = src_خونه;

        إذا (موخه_خونه != src_خونه) {
            // search for collisions
            u16 check_شاخص = i_ډېری_شاخص + 1;
            طالما (check_شاخص < i_پای_ټکی_ډېری_شاخص) {
                u16 otherSlot1 = Get_خونه_For_ډېری_شاخص(o, check_شاخص);
                u16 otherSlot2 = Get_Extra_خونه_For_ډېری_شاخص(o, check_شاخص);

                إذا (موخه_خونه == otherSlot1 or
                    موخه_خونه == otherSlot2 or
                    موخه_خونه + extra_خونه == otherSlot1) {
                    _throwif(m3_Err_function_ډېری_ډکېدنه, i_لنډمهاله_خونه >= d_m3MaxFunctionSlots);

_                   (Copy_ډېری_شاخص_منزل_خونه(o, i_لنډمهاله_خونه, check_شاخص));
                    o->wasm_ډېری[check_شاخص] = i_لنډمهاله_خونه;
                    i_لنډمهاله_خونه += Get_Type_عدد_Slots(c_m3Type_i64);
                    Touch_خونه(o, i_لنډمهاله_خونه - 1);

                    // restore this on the way back down
                    preserve_شاخص = check_شاخص;
                    collision_خونه = otherSlot1;

                    اكسر;
                }

                ++check_شاخص;
            }

_           (Copy_ډېری_شاخص_منزل_خونه(o, موخه_خونه, i_ډېری_شاخص));                                                m3log (compile, " copying slot: %d to slot: %d", src_خونه, موخه_خونه);
            o->wasm_ډېری[i_ډېری_شاخص] = موخه_خونه;
        }

_       (Copy_ډېری_Slots_R(o, i_موخه_خونه_ډېری_شاخص + 1, i_ډېری_شاخص + 1, i_پای_ټکی_ډېری_شاخص, i_لنډمهاله_خونه));

        // restore the stack state
        o->wasm_ډېری[i_ډېری_شاخص]  = src_خونه;
        o->wasm_ډېری[preserve_شاخص] = collision_خونه;
    }

    _catch:
    أرجع پایله;
}

ساكن
M3_پایله Resolve_بلاک_Results (IM3Compilation o, IM3CompilationScope i_موخه_بلاک, bool i_isBranch)
{
    M3_پایله پایله = m3Err_none;
    إذا (d_m3_Log_Wasm_ډېری) {
        dump_type_ډېری(o);
    }

    bool isLoop = (i_موخه_بلاک->opcode == c_waOp_loop and i_isBranch);

    u16 عدد_Params  = Get_Func_Type_عدد_Params(i_موخه_بلاک->type);
    u16 عدد_Results = Get_Func_Type_عدد_Results(i_موخه_بلاک->type);

    u16 خونه_ریکارډونه = i_موخه_بلاک->exit_ډېری_شاخص;

    u16 عدد_ارزښتونه;

    إذا (not isLoop) {
        عدد_ارزښتونه = عدد_Results;
        خونه_ریکارډونه += عدد_Params;
    } وإلا {
        عدد_ارزښتونه = عدد_Params;
    }

    u16 بلاک_لوړوالی = Get_عدد_بلاک_ارزښتونه_On_ډېری(o);

    _throwif(m3_Err_type_شمېر_Mismatch, i_isBranch ? (بلاک_لوړوالی < عدد_ارزښتونه) : (بلاک_لوړوالی != عدد_ارزښتونه));

    إذا (عدد_ارزښتونه) {
        u16 پای_ټکی_شاخص     = Get_ډېری_سر_شاخص(o) + 1;
        u16 عدد_Rem_ارزښتونه = عدد_ارزښتونه;

        // The last result is taken from _fp0. See PushBlockResults.
        إذا (not isLoop and IsFpType(Get_ډېری_سر_Type(o))) {
_           (Copy_ډېری_سر_منزل_ثبتوونکی(o, false));
            --پای_ټکی_شاخص;
            --عدد_Rem_ارزښتونه;
        }

        // tempslot affects maxStackSlots, so can grow unnecess each time.
        // in practice, this is hardly ever hit.
        u16 لنډمهاله_خونه = o->max_ډېری_Slots;// GetMaxUsedSlotPlusOne (o); doesn't work cause can collide with slotRecords
        Align_خونه_منزل_Type(&لنډمهاله_خونه, c_m3Type_i64);

_       (Copy_ډېری_Slots_R(o, خونه_ریکارډونه, پای_ټکی_شاخص - عدد_Rem_ارزښتونه, پای_ټکی_شاخص, لنډمهاله_خونه));

        إذا (d_m3_Log_Wasm_ډېری) {
            dump_type_ډېری(o);
        }
    }

    _catch: أرجع پایله;
}


ساكن
M3_پایله Return_ارزښتونه (IM3Compilation o, IM3CompilationScope i_function_بلاک, bool i_isBranch)
{
    M3_پایله پایله = m3Err_none;
    إذا (d_m3_Log_Wasm_ډېری) {
        dump_type_ډېری(o);
    }

    u16 عدد_Returns  = Get_Func_Type_عدد_Results(i_function_بلاک->type);     // could just o->function too...
    u16 بلاک_لوړوالی = Get_عدد_بلاک_ارزښتونه_On_ډېری(o);

    إذا (not Is_ډېری_Polymorphic(o)) {
        _throwif(m3_Err_type_شمېر_Mismatch, i_isBranch ? (بلاک_لوړوالی < عدد_Returns) : (بلاک_لوړوالی != عدد_Returns));
    }

    إذا (عدد_Returns) {
        // return slots like args are 64-bit aligned
        u16 return_خونه = عدد_Returns * c_io_خونه_شمېر;
        u16 ډېری_سر   = Get_ډېری_سر_شاخص(o);

        لكل (u16 i = 0; i < عدد_Returns; ++i) {
            m3type_t returnType = Get_Func_Type_پایله_Type(i_function_بلاک->type, عدد_Returns - 1 - i);

            m3type_t ډېری_Type = Get_ډېری_Type_From_سر(o, i);  // using FromTop so that only dynamic items are checked

            إذا (Is_ډېری_Polymorphic(o) and ډېری_Type == c_m3Type_none) {
                ډېری_Type = returnType;
            }

            _throwif(m3Err_typeMismatch, not Is_Sub_Type_د(ډېری_Type, returnType));

            إذا (not Is_ډېری_Polymorphic(o)) {
                return_خونه -= c_io_خونه_شمېر;
_               (Copy_ډېری_شاخص_منزل_خونه(o, return_خونه, ډېری_سر--));
            }
        }

        إذا (not i_isBranch) {
            طالما (عدد_Returns--) {
_               (راوباسه(o));
            }
        }
    }

    _catch: أرجع پایله;
}


//-------------------------------------------------------------------------------------------------------------------------

ساكن
M3_پایله Compile_Const_i32 (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_پایله پایله;

    i32 ارزښت;
_   (ReadLEB_i32(&ارزښت, &o->wasm, o->wasm_پای_ټکی));
_   (ټېل_Const(o, ارزښت, c_m3Type_i32));                        m3log (compile, d_indent " (const i32 = %" PRIi32 ")", get_indention_string (o), ارزښت);
    _catch: أرجع پایله;
}

ساكن
M3_پایله Compile_Const_i64 (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_پایله پایله;

    i64 ارزښت;
_   (ReadLEB_i64(&ارزښت, &o->wasm, o->wasm_پای_ټکی));
_   (ټېل_Const(o, ارزښت, c_m3Type_i64));                        m3log (compile, d_indent " (const i64 = %" PRIi64 ")", get_indention_string (o), ارزښت);
    _catch: أرجع پایله;
}


#إذا d_m3ImplementFloat
ساكن
M3_پایله Compile_Const_f32 (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_پایله پایله;

    اتحاد {
        u32 u;
        f32 f;
    } ارزښت = { 0 };

_   (Read_f32(&ارزښت.f, &o->wasm, o->wasm_پای_ټکی));                 m3log (compile, d_indent " (const f32 = %" PRIf32 ")", get_indention_string (o), ارزښت.f);
_   (ټېل_Const(o, ارزښت.u, c_m3Type_f32));

    _catch: أرجع پایله;
}

ساكن
M3_پایله Compile_Const_f64 (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_پایله پایله;

    اتحاد {
        u64 u;
        f64 f;
    } ارزښت = { 0 };

_   (Read_f64(&ارزښت.f, &o->wasm, o->wasm_پای_ټکی));                 m3log (compile, d_indent " (const f64 = %" PRIf64 ")", get_indention_string (o), ارزښت.f);
_   (ټېل_Const(o, ارزښت.u, c_m3Type_f64));

    _catch: أرجع پایله;
}
#د_که_پای

ساكن
M3_پایله Compile_ExtendedOpcode (IM3Compilation o, m3opcode_t i_opcode)
{
_try {
    u32 opcode;
_   (ReadLEB_u32(&opcode, &o->wasm, o->wasm_پای_ټکی));               m3log (compile, d_indent " (%s: %" PRIu32 ")", get_indention_string (o), (i_opcode == c_waOp_atomic) ? "FE" : "FC", opcode);

    // m3opcode_t carries only the low byte, so a wider sub-opcode is unknown
    // here rather than something to truncate into range
    _throwif(m3Err_unknownOpcode, opcode > (u32)((i_opcode == c_waOp_atomic) ? c_wa_Op_وروستی_Atomic : c_wa_Op_وروستی_Extended));

    i_opcode = (m3opcode_t)((i_opcode << 8) | opcode);

    //printf("Extended opcode: 0x%x\n", i_opcode);

    IM3OpInfo opInfo = GetOpInfo(i_opcode);
    _throwif(m3Err_unknownOpcode, not opInfo);

    M3Compiler compiler = GetCompiler(opInfo);
    _throwif(m3Err_noCompiler, not compiler);

_   ((*compiler)(o, i_opcode));

    o->پخوانی_Opcode = i_opcode;

    } _catch: أرجع پایله;
}

ساكن
M3_پایله Compile_Return (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_پایله پایله = m3Err_none;

    إذا (not Is_ډېری_Polymorphic(o)) {
        IM3CompilationScope functionScope;
_       (Get_بلاک_Scope(o, &functionScope, o->بلاک.ژوروالی));

_       (Return_ارزښتونه(o, functionScope, true));

_       (EmitOp(o, op_Return));

_       (سېټ_ډېری_Polymorphic(o));
    }

    _catch: أرجع پایله;
}

ساكن
M3_پایله Validate_بلاک_پای_ټکی (IM3Compilation o)
{
    M3_پایله پایله = m3Err_none;

    u16 عدد_Results  = Get_Func_Type_عدد_Results(o->بلاک.type);
    u16 بلاک_لوړوالی = Get_عدد_بلاک_ارزښتونه_On_ډېری(o);

    إذا (not Is_ډېری_Polymorphic(o)) {
        // Spec: at block end, stack height must match the number of results
        _throwif(m3_Err_type_شمېر_Mismatch, بلاک_لوړوالی != عدد_Results);

        // Spec: result types must match expected types
        لكل (u16 i = 0; i < عدد_Results; ++i) {
            m3type_t expectedType = Get_Func_Type_پایله_Type(o->بلاک.type, عدد_Results - 1 - i);
            m3type_t actualType   = Get_ډېری_Type_From_سر(o, i);
            _throwif(m3Err_typeMismatch, not Is_Sub_Type_د(actualType, expectedType));
        }
    }

    _catch: أرجع پایله;
}

ساكن
M3_پایله Compile_پای_ټکی (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_پایله پایله = m3Err_none;                   //dump_type_stack (o);

    // function end:
    إذا (o->بلاک.ژوروالی == 0) {
        Validate_بلاک_پای_ټکی(o);

        //      if (not IsStackPolymorphic (o))
        {
            // A constant expression has no function frame, but its value still
            // has to be copied down to slot 0, where EvaluateExpression reads it
            // back. The non-emitting init-expr walk is skipped: it has no block
            // type to return against.
            إذا (o->function or o->پاڼه) {
_               (Return_ارزښتونه(o, &o->بلاک, false));
            }

_           (EmitOp(o, op_Return));
        }
    }

    _catch: أرجع پایله;
}


ساكن
M3_پایله Compile_سېټ_Local (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_پایله پایله;

    u32 local_شاخص;
_   (ReadLEB_u32(&local_شاخص, &o->wasm, o->wasm_پای_ټکی));                           m3log (compile, d_indent " (index = %u)", get_indention_string (o), local_شاخص);

    إذا (local_شاخص < Get_Function_عدد_دلیلونه_And_Locals(o->function)) {
        // Spec: value type must match local type
        إذا (not Is_ډېری_Polymorphic(o)) {
            m3type_t localType    = Get_ډېری_Type_From_Bottom(o, local_شاخص);
            m3type_t ډېری_سر_Type = Get_ډېری_سر_Type(o);
            _throwif(m3Err_typeMismatch, ډېری_سر_Type != c_m3Type_none and localType != c_m3Type_none and
                                           not Is_Sub_Type_د(ډېری_سر_Type, localType));
#إذا d_m3HasSnapshots
            إذا (o->local_Init_ژوروالی and o->local_Init_ژوروالی[local_شاخص] == UINT32_MAX) {
                o->local_Init_ژوروالی[local_شاخص] = (u32)o->بلاک.ژوروالی;
            }
#د_که_پای
        }

        u16 local_خونه = Get_خونه_For_ډېری_شاخص(o, local_شاخص);

        u16 preserve_خونه;
_       (پیدا_کړه_Referenced_Local_Within_Current_بلاک(o, &preserve_خونه, local_خونه));  // preserve will be different than local, if referenced

        bool folded = false;

#إذا d_m3_Fold_سېټ_Local
        // destination folding: when the value was produced by the immediately preceding op (nothing
        // emitted since, result on top in _r0, no copy-on-write preservation needed), retro-patch the
        // producer to a variant that writes straight to the local's slot and skip the SetSlot op
        إذا (o->foldPatchPC and preserve_خونه == local_خونه and Is_ډېری_سر_دننه_ثبتوونکی(o) and Get_ډېری_سر_شاخص(o) == (i16)o->fold_ډېری_شاخص) {
            IM3Operation foldOp = GetFoldOp(o->foldOpcode, o->foldForm);

            إذا (foldOp) {
                *(IM3Operation*)o->foldPatchPC = foldOp;     // retro-patch the producer
                Emit_خونه_دننه_واټن(o, local_خونه);                  // append its destination
                InvalidateFold(o);
                folded = true;
            }
        }
#د_که_پای

        إذا (folded) {
            m3type_t type = Get_ډېری_سر_Type(o);
_           (راوباسه(o));                              // the value now lives in the local's slot

            // A folded op writes its result through to the register as well, so a tee
            // can keep it there. Pushing the slot instead would make the next use that
            // wants a register reload the slot this op just stored to.
            إذا (i_opcode == c_waOp_teeLocal) {
_               (ټېل_ثبتوونکی(o, type));
            }
        } وإلا {
            إذا (preserve_خونه == local_خونه) {
_               (Copy_ډېری_سر_منزل_خونه(o, local_خونه))
            } وإلا {
_               (Preserved_Copy_سر_خونه(o, local_خونه, preserve_خونه))
            }

            إذا (i_opcode != c_waOp_teeLocal) {
_               (راوباسه(o));
            }
        }
    } وإلا {
        _throw("local index out of bounds");
    }

    _catch: أرجع پایله;
}

ساكن
M3_پایله Compile_GetLocal (IM3Compilation o, m3opcode_t i_opcode)
{
_try {

    u32 local_شاخص;
_   (ReadLEB_u32(&local_شاخص, &o->wasm, o->wasm_پای_ټکی));                           m3log (compile, d_indent " (index = %u)", get_indention_string (o), local_شاخص);

    إذا (local_شاخص >= Get_Function_عدد_دلیلونه_And_Locals(o->function)) {
        _throw("local index out of bounds");
    }

    m3type_t type = Get_ډېری_Type_From_Bottom(o, local_شاخص);
    u16      خونه = Get_خونه_For_ډېری_شاخص(o, local_شاخص);

_   (ټېل(o, type, خونه));

    } _catch: أرجع پایله;
}

ساكن
M3_پایله Compile_GetGlobal (IM3Compilation o, M3Global* i_global)
{
    M3_پایله پایله;

    IM3Operation op = Is64_بټ_Type(i_global->type) ? op_GetGlobal_s64 : op_GetGlobal_s32;
_   (EmitOp(o, op));
    Emit_اشاره_ګر(o, &i_global->i64_ارزښت);
_   (ټېل_Allocated_خونه_And_Emit_2(o, i_global->type));

    _catch: أرجع پایله;
}

ساكن
M3_پایله Compile_سېټ_Global (IM3Compilation o, M3Global* i_global)
{
    M3_پایله پایله = m3Err_none;

    إذا (i_global->isMutable) {
        // Spec: value type must match global type
        إذا (not Is_ډېری_Polymorphic(o)) {
            m3type_t ډېری_سر_Type = Get_ډېری_سر_Type(o);
            _throwif(m3Err_typeMismatch, ډېری_سر_Type != c_m3Type_none and
                                           not Is_Sub_Type_د(ډېری_سر_Type, i_global->type));
        }

        IM3Operation op;
        m3type_t     type = Get_ډېری_سر_Type(o);

        إذا (Is_ډېری_سر_دننه_ثبتوونکی(o)) {
            op = c_سېټ_Global_Ops[بنسټ_Type_د(type)];
        } وإلا {
            op = Is64_بټ_Type(type) ? op_سېټ_Global_s64 : op_سېټ_Global_s32;
        }

_       (EmitOp(o, op));
        Emit_اشاره_ګر(o, &i_global->i64_ارزښت);

        إذا (Is_ډېری_سر_دننه_خونه(o)) {
            Emit_خونه_دننه_واټن(o, Get_ډېری_سر_خونه_شمېره(o));
        }

_       (راوباسه(o));
    } وإلا {
        _throw(m3Err_settingImmutableGlobal);
    }

    _catch: أرجع پایله;
}

ساكن
M3_پایله Compile_Get_سېټ_Global (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_پایله پایله = m3Err_none;

    u32 global_شاخص;
_   (ReadLEB_u32(&global_شاخص, &o->wasm, o->wasm_پای_ټکی));

    إذا (global_شاخص < o->module->عدد_Globals) {
        إذا (o->module->globals) {
            M3Global* global = &o->module->globals[global_شاخص];

            // Spec: a constant expression may only read an imported immutable
            // global. The module's own globals are counted before their
            // initializer is walked, so a bare index check would let one
            // reference itself. These describe how the module declared the
            // global, so they are checked before following any link.
            _throwif(m3_Err_globa_شاخص_بهر_د_Bounds, o->is_Init_بیان and not global->imported);
            _throwif(m3Err_wasmMalformed, o->is_Init_بیان and global->isMutable);

            // an import linked to another module reads and writes that module's
            // cell, not the placeholder standing in for it here
            إذا (global->resolved) {
                global = global->resolved;
            }

_           ((i_opcode == c_waOp_getGlobal) ? Compile_GetGlobal(o, global) : Compile_سېټ_Global(o, global));
        } وإلا {
            _throw(ErrorCompile(m3_Err_global_حافظه_Not_Allocated, o, "module '%s' is missing global memory", o->module->name));
        }
    } وإلا {
        _throw(m3_Err_globa_شاخص_بهر_د_Bounds);
    }

    _catch: أرجع پایله;
}

ساكن
فراغ Emit_Patching_Branch_اشاره_ګر (IM3Compilation o, IM3CompilationScope i_scope)
{
    pc_t patch       = Emit_اشاره_ګر(o, i_scope->patches);                m3log (compile, "branch patch required at: %p", patch);
    i_scope->patches = patch;
}

ساكن
M3_پایله EmitPatchingBranch (IM3Compilation o, IM3CompilationScope i_scope)
{
    M3_پایله پایله = m3Err_none;

_   (EmitOp(o, op_Branch));
    Emit_Patching_Branch_اشاره_ګر(o, i_scope);

    _catch: أرجع پایله;
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
u32 شمېر_Try_چوکاټونه_منزل_Exit (IM3CompilationScope i_from, IM3CompilationScope i_موخه)
{
    u32 شمېر = 0;

    لكل (IM3CompilationScope scope = i_from; scope; scope = scope->outer) {
        إذا (scope->opcode == c_wa_Op_try_جدول) {
            ++شمېر;
        }

        إذا (scope == i_موخه) {
            اكسر;
        }
    }

    أرجع شمېر;
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
u32 عدد_Try_چوکاټونه_منزل_راوباسه (IM3CompilationScope i_from, IM3CompilationScope i_موخه)
{
    إذا (i_موخه->ژوروالی == 0) {
        أرجع 0;
    }

    // the walk only ever runs inward-to-outward; a target that is already
    // outside i_from has nothing between them
    إذا (not i_from or i_from->ژوروالی < i_موخه->ژوروالی) {
        أرجع 0;
    }

    أرجع شمېر_Try_چوکاټونه_منزل_Exit(i_from, i_موخه);
}


ساكن
M3_پایله Emit_راوباسه_Try_چوکاټونه (IM3Compilation o, u32 i_عدد_چوکاټونه)
{
    M3_پایله پایله = m3Err_none;

    إذا (i_عدد_چوکاټونه) {
_       (EmitOp(o, op_راوباسه_Handlers));
        EmitConstant32(o, i_عدد_چوکاټونه);
    }

    _catch: أرجع پایله;
}


// A return_call leaves the enclosing function before the callee runs, so no
// try_table in this function may catch what the callee throws. Neither shape of
// the instruction unwinds the native stack in time to arrange that on its own:
// op_ReturnCall tail-jumps with op_TryTable's frames still standing, and the
// fallback shape is an ordinary call sitting inside them. Either way the
// handler records have to come off first.
ساكن
M3_پایله Emit_راوباسه_Try_چوکاټونه_For_Return_Call (IM3Compilation o)
{
    u32 شمېر = 0;

    لكل (IM3CompilationScope scope = &o->بلاک; scope; scope = scope->outer) {
        إذا (scope->opcode == c_wa_Op_try_جدول) {
            ++شمېر;
        }
    }

    أرجع Emit_راوباسه_Try_چوکاټونه(o, شمېر);
}

#وإلا

ساكن
M3_پایله Emit_راوباسه_Try_چوکاټونه (IM3Compilation o, u32 i_عدد_چوکاټونه)
{
    (فراغ)o;
    (فراغ)i_عدد_چوکاټونه;
    أرجع m3Err_none;
}

#د_که_پای // d_m3HasExceptionHandling

#إذا d_m3FuseBranch
// when the pending fold candidate is a compare whose result sits on top in _r0, return the fused
// variant that absorbs an immediately following br_if (i_isIf false) or if (i_isIf true)
ساكن
IM3Operation GetBranchFusionOp (IM3Compilation o, bool i_isIf)
{
    إذا (o->foldPatchPC and o->foldForm < 4 and Is_ډېری_سر_دننه_ثبتوونکی(o) and Get_ډېری_سر_شاخص(o) == (i16)o->fold_ډېری_شاخص) {
        ثابت M3FusedCmpOps* ops = GetFusedCmpOps(o->foldOpcode);

        إذا (ops) {
            أرجع i_isIf ? ops->ifOp[o->foldForm] : ops->branchIf[o->foldForm];
        }
    }

    أرجع عدم;
}
#د_که_پای // d_m3FuseBranch

ساكن مضمن
IM3Operation Op_ContinueLoop (IM3Compilation o)
{
#إذا d_m3_Has_ډېری_Switching
    إذا (M3_UNLIKELY(o->module->runtime and o->module->runtime->isSuspendable)) {
        أرجع op_ContinueLoop_Suspendable;
    }
#د_که_پای
    أرجع op_ContinueLoop;
}

ساكن مضمن
IM3Operation Op_ContinueLoopIf (IM3Compilation o)
{
#إذا d_m3_Has_ډېری_Switching
    إذا (M3_UNLIKELY(o->module->runtime and o->module->runtime->isSuspendable)) {
        أرجع op_ContinueLoopIf_Suspendable;
    }
#د_که_پای
    أرجع op_ContinueLoopIf;
}


ساكن
M3_پایله Compile_Branch (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_پایله پایله;
    u32      عدد_Try_چوکاټونه = 0;

    u32 ژوروالی;
_   (ReadLEB_u32(&ژوروالی, &o->wasm, o->wasm_پای_ټکی));

    // Spec: br_if condition must be i32
    إذا (i_opcode == c_waOp_branchIf and not Is_ډېری_Polymorphic(o)) {
        m3type_t condType = Get_ډېری_سر_Type(o);
        _throwif(m3Err_typeMismatch, condType != c_m3Type_none and condType != c_m3Type_i32);
    }

    IM3CompilationScope scope;
_   (Get_بلاک_Scope(o, &scope, ژوروالی));

#إذا d_m3HasExceptionHandling
    عدد_Try_چوکاټونه = عدد_Try_چوکاټونه_منزل_راوباسه(&o->بلاک, scope);
#د_که_پای

    // branch target is a loop (continue)
    إذا (scope->opcode == c_waOp_loop) {
        إذا (i_opcode == c_waOp_branchIf) {
            // a handler pop belongs on the taken path only, so it forces the
            // jump-over form even where the target takes no parameters
            إذا (Get_Func_Type_عدد_Params(scope->type) or عدد_Try_چوکاټونه) {
                IM3Operation op = Is_ډېری_سر_دننه_ثبتوونکی(o) ? op_BranchIfPrologue_r : op_BranchIfPrologue_s;

_               (EmitOp(o, op));
_               (Emit_خونه_عدد_د_ډېری_سر_And_راوباسه(o));

                pc_t* jump_منزل = (pc_t*)Reserve_اشاره_ګر(o);

_               (Emit_راوباسه_Try_چوکاټونه(o, عدد_Try_چوکاټونه));

                إذا (not Is_ډېری_Polymorphic(o)) {
_                   (Resolve_بلاک_Results(o, scope, /* isBranch: */ true));
                }

_               (EmitOp(o, Op_ContinueLoop(o)));
                Emit_اشاره_ګر(o, scope->pc);
_               (ریکارډ_شا_څنډه(o, scope, false));

                *jump_منزل = GetPC(o);
            } وإلا {
                // move the condition to a register
_               (Copy_ډېری_سر_منزل_ثبتوونکی(o, false));
_               (راوباسه_Type(o, c_m3Type_i32));

_               (EmitOp(o, Op_ContinueLoopIf(o)));
                Emit_اشاره_ګر(o, scope->pc);
_               (ریکارډ_شا_څنډه(o, scope, true));
            }

            //          dump_type_stack(o);
        } وإلا // is c_waOp_branch
        {
_           (Emit_راوباسه_Try_چوکاټونه(o, عدد_Try_چوکاټونه));

            // the branch operands become the loop's parameters on the next iteration
            إذا (not Is_ډېری_Polymorphic(o)) {
_               (Resolve_بلاک_Results(o, scope, /* isBranch: */ true));
            }

_           (EmitOp(o, Op_ContinueLoop(o)));
            Emit_اشاره_ګر(o, scope->pc);
_           (ریکارډ_شا_څنډه(o, scope, false));
            o->بلاک.isPolymorphic = true;
        }
    } وإلا // forward branch
    {
        pc_t* jump_منزل = عدم;

        bool isReturn         = (scope->ژوروالی == 0);
        bool موخه_Has_Results = Get_Func_Type_عدد_Results(scope->type);

        إذا (i_opcode == c_waOp_branchIf) {
            إذا (موخه_Has_Results or isReturn or عدد_Try_چوکاټونه) {
                IM3Operation op = Is_ډېری_سر_دننه_ثبتوونکی(o) ? op_BranchIfPrologue_r : op_BranchIfPrologue_s;

_               (EmitOp(o, op));
_               (Emit_خونه_عدد_د_ډېری_سر_And_راوباسه(o)); // condition

                // this is continuation point, if the branch isn't taken
                jump_منزل = (pc_t*)Reserve_اشاره_ګر(o);
            } وإلا {
#إذا d_m3FuseBranch
                IM3Operation fuseOp = GetBranchFusionOp(o, false);

                إذا (fuseOp) {
                    // absorb the branch into the compare that produced the condition
                    *(IM3Operation*)o->foldPatchPC = fuseOp;
                    InvalidateFold(o);

_                   (راوباسه(o));                      // the condition is consumed inside the fused op

                    Emit_Patching_Branch_اشاره_ګر(o, scope);
                    اذهب _catch;
                }
#د_که_پای
                IM3Operation op = Is_ډېری_سر_دننه_ثبتوونکی(o) ? op_BranchIf_r : op_BranchIf_s;

_               (EmitOp(o, op));
_               (Emit_خونه_عدد_د_ډېری_سر_And_راوباسه(o)); // condition

                Emit_Patching_Branch_اشاره_ګر(o, scope);
                اذهب _catch;
            }
        }

        إذا (not Is_ډېری_Polymorphic(o)) {
            إذا (isReturn) {
_               (Return_ارزښتونه(o, scope, true));
_               (EmitOp(o, op_Return));
            } وإلا {
_               (Emit_راوباسه_Try_چوکاټونه(o, عدد_Try_چوکاټونه));

_               (Resolve_بلاک_Results(o, scope, true));
_               (EmitPatchingBranch(o, scope));
            }
        }

        إذا (jump_منزل) {
            *jump_منزل = GetPC(o);
        }

        إذا (i_opcode == c_waOp_branch) {
_           (سېټ_ډېری_Polymorphic(o));
        }
    }

    _catch: أرجع پایله;
}

ساكن
M3_پایله Compile_Branch_جدول (IM3Compilation o, m3opcode_t i_opcode)
{
    // the page a continue-op page is standing in for; non-NULL only while that
    // page is installed in o->page, so the catch below knows to put it back
    IM3_کوډ_پاڼه displaced_پاڼه = عدم;
    // indexed by label depth: the continuation stub already emitted for that target
    // within this table, or NULL if this table has not reached it yet
    pc_t* موخه_Stubs = عدم;
_try {
    u32 موخه_شمېر;
_   (ReadLEB_u32(&موخه_شمېر, &o->wasm, o->wasm_پای_ټکی));

    // Spec: validate that the branch index operand is i32
    إذا (not Is_ډېری_Polymorphic(o)) {
        m3type_t شاخص_Type = Get_ډېری_سر_Type(o);
        _throwif(m3Err_typeMismatch, شاخص_Type != c_m3Type_none and شاخص_Type != c_m3Type_i32);
    }

_   (Preserve_ثبتوونکی_If_Occupied(o, c_m3Type_i64));         // move branch operand to a slot
    u16 خونه = Get_ډېری_سر_خونه_شمېره(o);
_   (راوباسه(o));

    // OPTZ: according to spec: "forward branches that target a control instruction with a non-empty
    // result type consume matching operands first and push them back on the operand stack after unwinding"
    // So, this move-to-reg is only necessary if the target scopes have a type.

    u32 عدد_کوډ_کرښې = موخه_شمېر + 4; // 3 => IM3Operation + slot + target_count + default_target
_   (Ensure_کوډ_پاڼه_عدد_کرښې(o, عدد_کوډ_کرښې));

_   (EmitOp(o, op_Branch_جدول));
    Emit_خونه_دننه_واټن(o, خونه);
    EmitConstant32(o, موخه_شمېر);

    IM3_کوډ_پاڼه continue_Op_پاڼه = عدم;

    // The label types are checked by ValidateFunction, which runs under the same
    // d_m3EnableValidation guard just before this compilation pass. Comparing each
    // target against the default here would be wrong anyway: the spec matches every
    // target against the operand stack, so in unreachable code the operands are the
    // bottom type and the targets may legitimately differ (unreached-valid.wast).

    ++موخه_شمېر; // include default

    // Every entry of one br_table is resolved from the same compilation state - each
    // stub restores whatever it touched - so two entries naming the same label emit
    // byte-identical epilogues. Emit one and point both table entries at it. A label
    // deeper than the current block is rejected by GetBlockScope below, which is what
    // bounds this array.
    موخه_Stubs = m3_AllocArray(pc_t, (size_t)o->بلاک.ژوروالی + 1);
    _throwifnull(موخه_Stubs);

    لكل (u32 i = 0; i < موخه_شمېر; ++i) {
        u32 موخه;
_       (ReadLEB_u32(&موخه, &o->wasm, o->wasm_پای_ټکی));

        IM3CompilationScope scope;
_       (Get_بلاک_Scope(o, &scope, موخه));

        إذا (موخه_Stubs[موخه]) {
            Emit_اشاره_ګر(o, موخه_Stubs[موخه]);
            استمر;
        }

        u32 عدد_Try_چوکاټونه = 0;
#إذا d_m3HasExceptionHandling
        عدد_Try_چوکاټونه = عدد_Try_چوکاټونه_منزل_راوباسه(&o->بلاک, scope);
#د_که_پای

        // A forward branch to a block that carries no results and leaves no try
        // frames behind has nothing to do on the way out but arrive. There is no
        // epilogue to hold, so the table entry becomes the patch site itself and the
        // block's address is written into it when the block ends - no stub, and no
        // code page to put one on. In unreachable code the same is true whatever the
        // block's type, since nothing is emitted for that case either way.
        //
        // Each such entry is patched on its own, so none is recorded in targetStubs:
        // there is no shared stub for a later entry naming this label to point at.
        إذا (scope->opcode != c_waOp_loop and scope->ژوروالی > 0 and عدد_Try_چوکاټونه == 0 and
            (Is_ډېری_Polymorphic(o) or Get_Func_Type_عدد_Results(scope->type) == 0)) {
            Emit_Patching_Branch_اشاره_ګر(o, scope);
            استمر;
        }

_       (Acquire_Compilation_کوډ_پاڼه(o, &continue_Op_پاڼه));

        pc_t پیل_PC  = Get_پاڼه_PC(continue_Op_پاڼه);
        displaced_پاڼه = o->پاڼه;
        o->پاڼه       = continue_Op_پاڼه;

        إذا (scope->opcode == c_waOp_loop) {
_           (Emit_راوباسه_Try_چوکاټونه(o, عدد_Try_چوکاټونه));

            إذا (not Is_ډېری_Polymorphic(o)) {
_               (Resolve_بلاک_Results(o, scope, true));
            }

_           (EmitOp(o, Op_ContinueLoop(o)));
            Emit_اشاره_ګر(o, scope->pc);
_           (ریکارډ_شا_څنډه(o, scope, false));
        } وإلا {
            إذا (not Is_ډېری_Polymorphic(o)) {
                إذا (scope->ژوروالی == 0) {
_                   (Return_ارزښتونه(o, scope, true));
_                   (EmitOp(o, op_Return));
                } وإلا {
_                   (Emit_راوباسه_Try_چوکاټونه(o, عدد_Try_چوکاټونه));

_                   (Resolve_بلاک_Results(o, scope, true));

_                   (EmitPatchingBranch(o, scope));
                }
            }
        }

        IM3_کوډ_پاڼه return_منزل = displaced_پاڼه;
        displaced_پاڼه        = عدم;

        Release_Compilation_کوډ_پاڼه(o);
        o->پاڼه = return_منزل;

        Emit_اشاره_ګر(o, پیل_PC);
        موخه_Stubs[موخه] = پیل_PC;
    }

_   (سېټ_ډېری_Polymorphic(o));
} _catch:

    // thrown out of the loop above with a continue-op page installed: release it
    // and restore the one it displaced, which the caller's catch then releases.
    // Otherwise the displaced page is on no list and simply leaks.
    إذا (displaced_پاڼه) {
        Release_Compilation_کوډ_پاڼه(o);
        o->پاڼه = displaced_پاڼه;
    }

    m3_Free(موخه_Stubs);

    أرجع پایله;
}

ساكن
M3_پایله Compile_Call_دلیلونه_And_Return (IM3Compilation o, u16* o_ډېری_دننه_واټن, IM3FuncType i_type, bool i_is_غیرمستقیم)
{
_try {

    u16 سر_خونه = Get_Max_کارول_شوی_خونه_Plus_One(o);

    // force use of at least one stack slot; this is to help ensure
    // the m3 stack overflows (and traps) before the native stack can overflow.
    // e.g. see Wasm spec test 'runaway' in call.wast
    سر_خونه = M3_MAX(1, سر_خونه);

    // stack frame is 64-bit aligned
    Align_خونه_منزل_Type(&سر_خونه, c_m3Type_i64);

    *o_ډېری_دننه_واټن = سر_خونه;

    // wait to pop this here so that topSlot search is correct
    إذا (i_is_غیرمستقیم) {
_       (راوباسه(o));
    }

    u16 عدد_دلیلونه = Get_Func_Type_عدد_Params(i_type);
    u16 عدد_Rets = Get_Func_Type_عدد_Results(i_type);

    u16 arg_سر = سر_خونه + (عدد_دلیلونه + عدد_Rets) * c_io_خونه_شمېر;

    Touch_خونه(o, arg_سر - 1);

    طالما (عدد_دلیلونه--) {
_       (Copy_ډېری_سر_منزل_خونه(o, arg_سر -= c_io_خونه_شمېر));
_       (راوباسه(o));
    }

    u16 i = 0;
    طالما (عدد_Rets--) {
        m3type_t type = Get_Func_Type_پایله_Type(i_type, i++);

_       (ټېل(o, type, سر_خونه));
_       (MarkSlotsAllocatedByType(o, سر_خونه, type));

        سر_خونه += c_io_خونه_شمېر;
    }

    } _catch: أرجع پایله;
}

// A tail call hands the current frame over to the callee instead of stacking a new one.
// The callee's results have to be the enclosing function's results, so both use the same
// return slots and only the arguments move: they're staged above the caller's stack here
// (the argument expressions can still be reading the args/locals they're about to land on)
// and slid down onto the frame base at runtime, by op_ReturnCall[Indirect].
ساكن
M3_پایله Compile_لکۍ_Call_دلیلونه (IM3Compilation o, u16* o_ډېری_دننه_واټن, u16* o_عدد_Arg_Slots, IM3FuncType i_type, bool i_is_غیرمستقیم)
{
_try {
    IM3FuncType funcType = o->function->funcType;

    u16 عدد_Rets = Get_Func_Type_عدد_Results(i_type);
    _throwif(m3Err_typeMismatch, عدد_Rets != Get_Func_Type_عدد_Results(funcType));

    لكل (u16 i = 0; i < عدد_Rets; ++i) {
        _throwif(m3Err_typeMismatch, Get_Func_Type_پایله_Type(i_type, i) != Get_Func_Type_پایله_Type(funcType, i));
    }

    u16 سر_خونه = Get_Max_کارول_شوی_خونه_Plus_One(o);

    سر_خونه = M3_MAX(1, سر_خونه);

    // stack frame is 64-bit aligned
    Align_خونه_منزل_Type(&سر_خونه, c_m3Type_i64);

    *o_ډېری_دننه_واټن = سر_خونه;

    // wait to pop this here so that topSlot search is correct
    إذا (i_is_غیرمستقیم) {
_       (راوباسه(o));
    }

    u16 عدد_دلیلونه     = Get_Func_Type_عدد_Params(i_type);
    u16 عدد_Arg_Slots = عدد_دلیلونه * c_io_خونه_شمېر;

    *o_عدد_Arg_Slots = عدد_Arg_Slots;

    u16 arg_سر = سر_خونه + عدد_Arg_Slots;
    _throwif(m3_Err_function_ډېری_ډکېدنه, arg_سر > d_m3MaxFunctionSlots);

    // the staging area is written by the caller, so op_Entry has to account for it
    Touch_خونه(o, arg_سر - 1);

    طالما (عدد_دلیلونه--) {
_       (Copy_ډېری_سر_منزل_خونه(o, arg_سر -= c_io_خونه_شمېر));
_       (راوباسه(o));
    }

    } _catch: أرجع پایله;
}


#إذا d_m3HasTypedRefs

// call_ref $t: the callee is the reference on top of the stack, so the operand
// order is the same as call_indirect's, minus the table.
ساكن
M3_پایله Compile_CallRef (IM3Compilation o, m3opcode_t i_opcode)
{
_try {
    u32 type_شاخص;
_   (ReadLEB_u32(&type_شاخص, &o->wasm, o->wasm_پای_ټکی));

    _throwif("function call type index out of range", type_شاخص >= o->module->عدد_Func_Types);

    IM3FuncType type = o->module->funcTypes[type_شاخص];

    إذا (not Is_ډېری_Polymorphic(o)) {
        // whatever shape the reference has, it must be a function of this type
        m3type_t refType = Get_ډېری_سر_Type(o);
        _throwif(m3Err_typeMismatch, not Is_Sub_Type_د(refType, Ref_Type_د_Func_Type(type, false)));
    }

    إذا (Is_ډېری_سر_دننه_ثبتوونکی(o)) {
_       (Preserve_ثبتوونکی_If_Occupied(o, c_m3Type_funcref));
    }

    u16 function_خونه = Get_ډېری_سر_خونه_شمېره(o);

    bool isReturnCall = (i_opcode == c_waOp_returnCallRef);
    bool use_لکۍ_Call  = isReturnCall and d_m3_Can_لکۍ_Call;

    u16 exec_سر, عدد_Arg_Slots = 0;

    إذا (use_لکۍ_Call) {
_       (Compile_لکۍ_Call_دلیلونه(o, &exec_سر, &عدد_Arg_Slots, type, true));
    } وإلا {
_       (Compile_Call_دلیلونه_And_Return(o, &exec_سر, type, true));
    }

#  إذا d_m3HasExceptionHandling
    إذا (isReturnCall) {
_       (Emit_راوباسه_Try_چوکاټونه_For_Return_Call(o));
    }
#  endif

_   (EmitOp(o, use_لکۍ_Call ? op_ReturnCallRef : op_CallRef));
    Emit_خونه_دننه_واټن(o, function_خونه);
    Emit_اشاره_ګر(o, type);
    Emit_خونه_دننه_واټن(o, exec_سر);

    إذا (not use_لکۍ_Call) {
_       (ریکارډ_Call(o, Get_Func_Type_عدد_Results(type), exec_سر, isReturnCall));
    }

    إذا (use_لکۍ_Call) {
        Emit_خونه_دننه_واټن(o, o->function->عدد_Ret_Slots);
        EmitConstant32(o, عدد_Arg_Slots);

_       (سېټ_ډېری_Polymorphic(o));
    } وإلا إذا (isReturnCall) {
_       (Compile_Return(o, i_opcode));
    }

} _catch:
    أرجع پایله;
}


// ref.as_non_null only sharpens the type; the value is unchanged, so the stack
// entry is re-pushed with the null stripped out of its type.
ساكن
M3_پایله Compile_Ref_AsNonNull (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_پایله پایله = m3Err_none;

    إذا (not Is_ډېری_Polymorphic(o)) {
        m3type_t type = Get_ډېری_سر_Type(o);
        _throwif(m3Err_typeMismatch, not IsRefType(type));

        // op_RefAsNonNull only knows how to read its operand out of a slot; a
        // value still sitting in the register has to be spilled first, the way
        // every other single-op reader of the stack top does
        إذا (Is_ډېری_سر_دننه_ثبتوونکی(o)) {
_           (Preserve_ثبتوونکی_If_Occupied(o, type));
        }

        u16 خونه = Get_ډېری_سر_خونه_شمېره(o);

_       (EmitOp(o, op_RefAsNonNull));
        Emit_خونه_دننه_واټن(o, خونه);

        m3type_t nonNull = IsSpelledRefType(type)
                             ? (m3type_t)(type | d_m3Type_refNonNull)
                             : (m3type_t)(d_m3Type_ref | d_m3Type_refNonNull | d_m3_Type_ډېرۍ_Abstract |
                                          ((بنسټ_Type_د(type) == c_m3Type_externref) ? d_m3Type_refExtern : 0));

        // the slot keeps the one reference it already had; a Pop()/Push() of
        // the same slot would drop that reference's count without anything
        // to put it back, so the type above it is corrected in place instead
        o->type_ډېری[Get_ډېری_سر_شاخص(o)] = nonNull;
    }

    _catch: أرجع پایله;
}

#د_که_پای // d_m3HasTypedRefs


// How wide a table operand is read at comes from the table it names, so a slot
// holding a narrower type would be read past what was allocated for it. The
// validator refuses that already; this keeps a build without one from reaching
// the interpreter with it. c_m3Type_none means the operand is not on the
// dynamic stack to inspect, which is not this check's business.
ساكن
M3_پایله Check_عملوند_Type (IM3Compilation o, u16 i_ژوروالی_From_سر, m3type_t i_type)
{
    m3type_t actual;

    إذا (Is_ډېری_Polymorphic(o)) {
        أرجع m3Err_none;
    }

    actual = Get_ډېری_Type_From_سر(o, i_ژوروالی_From_سر);

    أرجع (actual == c_m3Type_none or بنسټ_Type_د(actual) == i_type) ? m3Err_none
                                                                     : m3Err_typeMismatch;
}


// Swaps the _mem register onto a given memory. Only ever emitted in pairs --
// see op_SetMemory.
ساكن
M3_پایله Emit_سېټ_حافظه_Ptr (IM3Compilation o, IM3_حافظه i_حافظه)
{
    M3_پایله پایله = m3Err_none;

_   (EmitOp(o, op_سېټ_حافظه));
    Emit_اشاره_ګر(o, i_حافظه);

    _catch: أرجع پایله;
}


// ...onto one of the compiling module's own memories, by index. Only reached
// with a non-zero index under multi-memory, but the index has already been
// bounds-checked by ReadMemoryIndex either way.
ساكن مضمن
M3_پایله Emit_سېټ_حافظه (IM3Compilation o, u32 i_حافظه_Idx)
{
#إذا d_m3_Has_Multi_حافظه
    أرجع Emit_سېټ_حافظه_Ptr(o, o->module->memories[i_حافظه_Idx]);
#وإلا
    (فراغ)o;
    (فراغ)i_حافظه_Idx;
    أرجع m3_Err_unknown_حافظه;
#د_که_پای
}

ساكن
M3_پایله Compile_Call (IM3Compilation o, m3opcode_t i_opcode)
{
_try {
    u32 function_شاخص;
_   (ReadLEB_u32(&function_شاخص, &o->wasm, o->wasm_پای_ټکی));

    IM3Function function = Module_GetFunction(o->module, function_شاخص);

    إذا (function) {                                                     m3log (compile, d_indent " (func= [%d] '%s'; args= %d)",
                                                                                get_indention_string (o), function_شاخص, m3_GetFunctionName (function), function->funcType->عدد_دلیلونه);
        // an import linked to another module's export runs that module's body
        IM3Function موخه = Function_Implementation(function);

        إذا (موخه->module) {
            bool isReturnCall = (i_opcode == c_waOp_returnCall);

            // The callee's body runs against its own module's memory, so a call
            // that crosses modules is bracketed by a pair of op_SetMemory. A
            // tail call never returns to run the second one, so those compile
            // as a plain call followed by a return instead.
            bool crossModule = (موخه->module != o->module);
            bool use_لکۍ_Call = isReturnCall and d_m3_Can_لکۍ_Call and not crossModule;

            u16 خونه_سر, عدد_Arg_Slots = 0;

            إذا (use_لکۍ_Call) {
_               (Compile_لکۍ_Call_دلیلونه(o, &خونه_سر, &عدد_Arg_Slots, موخه->funcType, false));
            } وإلا {
_               (Compile_Call_دلیلونه_And_Return(o, &خونه_سر, موخه->funcType, false));
            }

            IM3Operation op;
            ثابت فراغ*  عملوند;

            إذا (موخه->compiled) {
                op      = use_لکۍ_Call ? op_ReturnCall : op_Call;
                عملوند = موخه->compiled;
            } وإلا {
                op      = use_لکۍ_Call ? op_CompileReturnCall : op_Compile;
                عملوند = موخه;
            }

#إذا d_m3HasExceptionHandling
            إذا (isReturnCall) {
_               (Emit_راوباسه_Try_چوکاټونه_For_Return_Call(o));
            }
#د_که_پای

            إذا (crossModule) {
_               (Emit_سېټ_حافظه_Ptr(o, Module_Memory0(موخه->module)));
            }

_           (EmitOp(o, op));
            Emit_اشاره_ګر(o, عملوند);
            Emit_خونه_دننه_واټن(o, خونه_سر);

            إذا (not use_لکۍ_Call) {
                // a suspend inside the callee leaves this frame waiting here, its
                // results not yet written
_               (ریکارډ_Call(o, Get_Func_Type_عدد_Results(موخه->funcType), خونه_سر, isReturnCall));
            }

            إذا (use_لکۍ_Call) {
                Emit_خونه_دننه_واټن(o, o->function->عدد_Ret_Slots);
                EmitConstant32(o, عدد_Arg_Slots);

_               (سېټ_ډېری_Polymorphic(o));
            } وإلا {
                إذا (crossModule) {
_                   (Emit_سېټ_حافظه_Ptr(o, Module_Memory0(o->module)));
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

    } _catch: أرجع پایله;
}

ساكن
M3_پایله Compile_Call_غیرمستقیم (IM3Compilation o, m3opcode_t i_opcode)
{
_try {
    u32 type_شاخص;
_   (ReadLEB_u32(&type_شاخص, &o->wasm, o->wasm_پای_ټکی));

    u32 جدول_شاخص;
_   (ReadLEB_u32(&جدول_شاخص, &o->wasm, o->wasm_پای_ټکی));

    _throwif("function call type index out of range", type_شاخص >= o->module->عدد_Func_Types);
    _throwif("table index out of range", جدول_شاخص >= o->module->عدد_Tables);
    _throwif(m3Err_typeMismatch, بنسټ_Type_د(o->module->tables[جدول_شاخص]->type) != c_m3Type_funcref);

_   (Check_عملوند_Type(o, 0, جدول_Addr_Type(o->module->tables[جدول_شاخص])));

    إذا (Is_ډېری_سر_دننه_ثبتوونکی(o)) {
_       (Preserve_ثبتوونکی_If_Occupied(o, c_m3Type_i32));
    }

    u16 جدول_شاخص_خونه = Get_ډېری_سر_خونه_شمېره(o);

    bool isReturnCall = (i_opcode == c_wa_Op_return_Call_غیرمستقیم);
    bool use_لکۍ_Call  = isReturnCall and d_m3_Can_لکۍ_Call;

    u16         exec_سر, عدد_Arg_Slots = 0;
    IM3FuncType type = o->module->funcTypes[type_شاخص];

    إذا (use_لکۍ_Call) {
_       (Compile_لکۍ_Call_دلیلونه(o, &exec_سر, &عدد_Arg_Slots, type, true));
    } وإلا {
_       (Compile_Call_دلیلونه_And_Return(o, &exec_سر, type, true));
    }

#إذا d_m3HasExceptionHandling
    إذا (isReturnCall) {
_       (Emit_راوباسه_Try_چوکاټونه_For_Return_Call(o));
    }
#د_که_پای

    // the table comes first: how wide its indexes are is what says how much of
    // the slot below holds one
_   (EmitOp(o, use_لکۍ_Call ? op_Return_Call_غیرمستقیم : op_Call_غیرمستقیم));
    Emit_اشاره_ګر(o, o->module->tables[جدول_شاخص]);
    Emit_خونه_دننه_واټن(o, جدول_شاخص_خونه);
    Emit_اشاره_ګر(o, type);
    Emit_خونه_دننه_واټن(o, exec_سر);

    إذا (not use_لکۍ_Call) {
_       (ریکارډ_Call(o, Get_Func_Type_عدد_Results(type), exec_سر, isReturnCall));
    }

    إذا (use_لکۍ_Call) {
        Emit_خونه_دننه_واټن(o, o->function->عدد_Ret_Slots);
        EmitConstant32(o, عدد_Arg_Slots);

_       (سېټ_ډېری_Polymorphic(o));
    } وإلا إذا (isReturnCall) {
_       (Compile_Return(o, i_opcode));
    }

} _catch:
    أرجع پایله;
}

// Reads the memory index a memory instruction names, and checks it against the
// module's index space. Without multi-memory the immediate is a reserved byte
// that has to be zero, which is the same check.
ساكن
M3_پایله Read_حافظه_شاخص (IM3Compilation o, u32* o_حافظه_Idx)
{
    M3_پایله پایله;

_   (ReadLEB_u32(o_حافظه_Idx, &o->wasm, o->wasm_پای_ټکی));

    _throwif(m3_Err_unknown_حافظه, *o_حافظه_Idx >= o->module->عدد_Memories);

    _catch: أرجع پایله;
}

ساكن
M3_پایله Compile_حافظه_اندازه (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_پایله پایله;

    u32 حافظه_Idx;

    // A page count is given in the memory's own address type. Declared up here
    // so the throws below don't jump over its initialization.
    m3type_t addrType = c_m3Type_i32;

_   (Read_حافظه_شاخص(o, &حافظه_Idx));

    addrType = حافظه_Addr_Type(o->module->memories[حافظه_Idx]);

_   (Preserve_ثبتوونکی_If_Occupied(o, addrType));

    إذا (حافظه_Idx) {
_       (Emit_سېټ_حافظه(o, حافظه_Idx));
    }

    // op_MemSize writes the whole register either way; the slot it is pushed
    // to is what decides how much of it the module gets to see
_   (EmitOp(o, op_Mem_اندازه));

    إذا (حافظه_Idx) {
_       (Emit_سېټ_حافظه(o, 0));
    }

_   (ټېل_ثبتوونکی(o, addrType));

    _catch: أرجع پایله;
}

ساكن
M3_پایله Compile_حافظه_Grow (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_پایله پایله;

    u32 حافظه_Idx;

    // see Compile_Memory_Size
    m3type_t addrType = c_m3Type_i32;

_   (Read_حافظه_شاخص(o, &حافظه_Idx));

    addrType = حافظه_Addr_Type(o->module->memories[حافظه_Idx]);

_   (Copy_ډېری_سر_منزل_ثبتوونکی(o, false));
_   (راوباسه_Type(o, addrType));

    إذا (حافظه_Idx) {
_       (Emit_سېټ_حافظه(o, حافظه_Idx));
    }

#إذا d_m3HasMemory64
_   (EmitOp(o, (addrType == c_m3Type_i64) ? op_MemGrow64 : op_MemGrow));
#وإلا
_   (EmitOp(o, op_MemGrow));
#د_که_پای

    إذا (حافظه_Idx) {
_       (Emit_سېټ_حافظه(o, 0));
    }

_   (ټېل_ثبتوونکی(o, addrType));

    _catch: أرجع پایله;
}

ساكن
M3_پایله Compile_حافظه_Copy_Fill (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_پایله پایله = m3Err_none;

    // memory.copy names the destination first and then the source; memory.fill
    // names only the one it writes.
    u32  سرچینه_حافظه_Idx = 0, موخه_حافظه_Idx = 0;
    bool isCopy = (i_opcode == c_wa_Op_حافظه_Copy);

    // Each address is typed by the memory it belongs to. The length follows
    // the narrower of the two, so it is 64-bit only when both are - which for
    // memory.fill, naming one memory twice over, means whenever that one is.
    // Declared up here so the throws below don't jump over them.
    m3type_t موخه_Type = c_m3Type_i32;
    m3type_t سرچینه_Type = c_m3Type_i32;
    m3type_t اوږدتیا_Type = c_m3Type_i32;

    _throwif(m3Err_wasmMalformed, o->module->عدد_Memories == 0);

_   (Read_حافظه_شاخص(o, &موخه_حافظه_Idx));

    إذا (isCopy) {
_       (Read_حافظه_شاخص(o, &سرچینه_حافظه_Idx));
    }

    موخه_Type = حافظه_Addr_Type(o->module->memories[موخه_حافظه_Idx]);
    سرچینه_Type = isCopy ? حافظه_Addr_Type(o->module->memories[سرچینه_حافظه_Idx])
                        : موخه_Type;
    اوږدتیا_Type = (موخه_Type == c_m3Type_i64 and سرچینه_Type == c_m3Type_i64)
                   ? c_m3Type_i64
                   : c_m3Type_i32;

_   (Copy_ډېری_سر_منزل_ثبتوونکی(o, false));

#إذا d_m3_Has_Multi_حافظه
    إذا (isCopy and سرچینه_حافظه_Idx != موخه_حافظه_Idx) {
        // two memories at once: _mem can only name one, so the op takes both
_       (EmitOp(o, op_MemCopy_x));
        Emit_اشاره_ګر(o, o->module->memories[موخه_حافظه_Idx]);
        Emit_اشاره_ګر(o, o->module->memories[سرچینه_حافظه_Idx]);

        // ...and, since they need not be addressed alike, how wide to read
        // each of the operands: bit 0 the destination, bit 1 the source
        EmitConstant32(o, ((موخه_Type == c_m3Type_i64) ? 0x1u : 0u) |
                            ((سرچینه_Type == c_m3Type_i64) ? 0x2u : 0u));
    } وإلا
#د_که_پای
    {
        IM3Operation op = isCopy ? op_MemCopy : op_MemFill;

#إذا d_m3HasMemory64
        إذا (موخه_Type == c_m3Type_i64) {
            op = isCopy ? op_MemCopy64 : op_MemFill64;
        }
#د_که_پای

        إذا (موخه_حافظه_Idx) {
_           (Emit_سېټ_حافظه(o, موخه_حافظه_Idx));
        }

_       (EmitOp(o, op));
    }

    // the length is in the register; the other two are named by slot, at
    // whatever width their stack entries were pushed with
_   (راوباسه_Type(o, اوږدتیا_Type));
_   (Emit_خونه_عدد_د_ډېری_سر_And_راوباسه(o));      // the source, or the byte to fill with
_   (Emit_خونه_عدد_د_ډېری_سر_And_راوباسه(o));      // the destination

    إذا (موخه_حافظه_Idx and not (isCopy and سرچینه_حافظه_Idx != موخه_حافظه_Idx)) {
_       (Emit_سېټ_حافظه(o, 0));
    }

    _catch: أرجع پایله;
}


// memory.init and data.drop address a data segment by index. Both are only valid
// when a data count section declared the segments up front.
ساكن
M3_پایله Read_معلومات_Segment (IM3Compilation o, M3_معلومات_Segment** o_segment)
{
    M3_پایله پایله = m3Err_none;

    u32 index;
_   (ReadLEB_u32(&index, &o->wasm, o->wasm_پای_ټکی));

    _throwif("data count section required", not o->module->has_معلومات_شمېر);
    _throwif(m3Err_wasmMalformed, index >= o->module->عدد_معلومات_Segments);

    *o_segment = &o->module->معلومات_Segments[index];

    _catch: أرجع پایله;
}


M3_پایله Compile_حافظه_Init (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_پایله پایله = m3Err_none;

    M3_معلومات_Segment* segment = عدم;
    u32            حافظه_Idx;

    _throwif(m3Err_wasmMalformed, o->module->عدد_Memories == 0);

_   (Read_معلومات_Segment(o, &segment));
_   (Read_حافظه_شاخص(o, &حافظه_Idx));

_   (Copy_ډېری_سر_منزل_ثبتوونکی(o, false));

    إذا (حافظه_Idx) {
_       (Emit_سېټ_حافظه(o, حافظه_Idx));
    }

#إذا d_m3HasMemory64
    // only the destination follows the memory's address type: a data segment
    // is indexed as an i32 however the memory it is copied into is addressed
_   (EmitOp(o, o->module->memories[حافظه_Idx]->isMemory64 ? op_MemInit64 : op_MemInit));
#وإلا
_   (EmitOp(o, op_MemInit));
#د_که_پای
    Emit_اشاره_ګر(o, segment);
_   (راوباسه_Type(o, c_m3Type_i32));
_   (Emit_خونه_عدد_د_ډېری_سر_And_راوباسه(o));
_   (Emit_خونه_عدد_د_ډېری_سر_And_راوباسه(o));

    إذا (حافظه_Idx) {
_       (Emit_سېټ_حافظه(o, 0));
    }

    _catch: أرجع پایله;
}


#إذا d_m3HasRefTypes

ساكن
M3_پایله Compile_Select (IM3Compilation o, m3opcode_t i_opcode);

// select with an explicit result type vector. The types only matter to the
// validator; the operands are laid out exactly as for the untyped select.
M3_پایله Compile_Select_Typed (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_پایله پایله = m3Err_none;

    u32 عدد_Types;
_   (ReadLEB_u32(&عدد_Types, &o->wasm, o->wasm_پای_ټکی));
    _throwif(m3Err_wasmMalformed, عدد_Types != 1);

    i8 waType;
    u8 type;
_   (ReadLEB_i7(&waType, &o->wasm, o->wasm_پای_ټکی));
_   (NormalizeType(&type, waType));

_   (Compile_Select(o, i_opcode));

    _catch: أرجع پایله;
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
M3_پایله ReadRefType (IM3Compilation o, m3type_t* o_type)
{
    M3_پایله پایله = m3Err_none;

#  إذا d_m3HasTypedRefs
    // ref.null names a heap type: func, extern, or a function type index
    m3type_t ډېرۍ_بټونه;
_   (Parse_ډېرۍ_Type(o->module, &ډېرۍ_بټونه, &o->wasm, o->wasm_پای_ټکی));

    *o_type = d_m3Type_ref | ډېرۍ_بټونه;
#  وإلا
    i8 waType;
    u8 plainType;
_   (ReadLEB_i7(&waType, &o->wasm, o->wasm_پای_ټکی));
_   (NormalizeType(&plainType, waType));
    *o_type = plainType;
    _throwif(m3Err_wasmMalformed, not IsRefType(*o_type));
#  endif

    _catch: أرجع پایله;
}


M3_پایله Compile_Ref_Null (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_پایله پایله = m3Err_none;

    m3type_t type;
_   (ReadRefType(o, &type));
_   (ټېل_Const(o, 0, type));

    _catch: أرجع پایله;
}


M3_پایله Compile_Ref_IsNull (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_پایله پایله = m3Err_none;

    IM3Operation op;

    إذا (not Is_ډېری_Polymorphic(o)) {
        _throwif(m3Err_typeMismatch, not IsRefType(Get_ډېری_سر_Type(o)));
    }

    إذا (Is_ډېری_سر_دننه_ثبتوونکی(o)) {
        op = d_m3RefIsNull_r;
    } وإلا {
_       (Preserve_ثبتوونکی_If_Occupied(o, c_m3Type_i32));
        op = d_m3RefIsNull_s;
    }

_   (EmitOp(o, op));
_   (Emit_خونه_عدد_د_ډېری_سر_And_راوباسه(o));
_   (ټېل_ثبتوونکی(o, c_m3Type_i32));

    _catch: أرجع پایله;
}


M3_پایله Compile_Ref_Func (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_پایله پایله = m3Err_none;

    u32 func_شاخص;

    // declared before the throws below, which jump past this point to _catch
    m3type_t    refType   = c_m3Type_funcref;
    IM3Function reference = عدم;

_   (ReadLEB_u32(&func_شاخص, &o->wasm, o->wasm_پای_ټکی));
    _throwif("function index out of range", func_شاخص >= o->module->عدد_Functions);

    // Inside a constant expression ref.func is itself a declaration; inside a
    // function body the function must already have been declared elsewhere.
    إذا (o->function) {
        _throwif("undeclared function reference", not Module_IsFunctionDeclared(o->module, func_شاخص));
    } وإلا {
_       (Module_DeclareFunction(o->module, func_شاخص));
    }

#  إذا d_m3HasTypedRefs
    refType = Ref_Type_د_Func_Type(o->module->functions[func_شاخص].funcType, true);
#  endif

    // A reference denotes the function that actually runs, so a ref.func naming
    // an import linked to another module is the same reference that module's
    // own ref.func would produce - and carries the defining module with it.
    reference = Function_Implementation(&o->module->functions[func_شاخص]);

_   (ټېل_Const(o, (u64)(uintptr_t)reference, refType));

    _catch: أرجع پایله;
}


// table.get/set/size/grow/fill all name a table by index; the table struct goes
// into the codestream so the operation doesn't have to walk the module.
ساكن
M3_پایله Read_جدول (IM3Compilation o, M3_جدول** o_جدول)
{
    M3_پایله پایله = m3Err_none;

    u32 index;
_   (ReadLEB_u32(&index, &o->wasm, o->wasm_پای_ټکی));
    _throwif("table index out of range", index >= o->module->عدد_Tables);

    *o_جدول = o->module->tables[index];

    _catch: أرجع پایله;
}


// The table operations take every operand from a slot, so spill the integer
// register first: after this each EmitSlotNumOfStackTopAndPop () emits a real
// slot offset, and the register is free for the result. Operands are emitted
// from the top of the stack downwards, which is the order the ops read them.
ساكن
M3_پایله Compile_جدول_Op (IM3Compilation o, IM3Operation i_op, M3_جدول* i_جدول, u32 i_عدد_Operands, m3type_t i_retType)
{
    M3_پایله پایله = m3Err_none;

_   (Preserve_ثبتوونکی_If_Occupied(o, c_m3Type_i64));

_   (EmitOp(o, i_op));
    Emit_اشاره_ګر(o, i_جدول);

    لكل (u32 i = 0; i < i_عدد_Operands; ++i) {
_       (Emit_خونه_عدد_د_ډېری_سر_And_راوباسه(o));
    }

    إذا (i_retType != c_m3Type_none) {
_       (ټېل_ثبتوونکی(o, i_retType));
    }

    _catch: أرجع پایله;
}


M3_پایله Compile_جدول_Get_سېټ (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_پایله پایله = m3Err_none;

    M3_جدول* جدول_2;
_   (Read_جدول(o, &جدول_2));

    إذا (i_opcode == c_wa_Op_جدول_Get) {
_       (Check_عملوند_Type(o, 0, جدول_Addr_Type(جدول_2)));
_       (Compile_جدول_Op(o, op_جدول_Get, جدول_2, 1, جدول_2->type));
    } وإلا {
_       (Check_عملوند_Type(o, 1, جدول_Addr_Type(جدول_2)));
_       (Compile_جدول_Op(o, op_جدول_سېټ, جدول_2, 2, c_m3Type_none));
    }

    _catch: أرجع پایله;
}


M3_پایله Compile_جدول_Init (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_پایله پایله = m3Err_none;

    u32      elem_شاخص;
    M3_جدول* جدول_2;

_   (ReadLEB_u32(&elem_شاخص, &o->wasm, o->wasm_پای_ټکی));
    _throwif("element segment index out of range", elem_شاخص >= o->module->عدد_Element_Segments);
_   (Read_جدول(o, &جدول_2));

_   (Check_عملوند_Type(o, 2, جدول_Addr_Type(جدول_2)));

_   (Preserve_ثبتوونکی_If_Occupied(o, c_m3Type_i64));
_   (EmitOp(o, op_جدول_Init));
    Emit_اشاره_ګر(o, جدول_2);
    Emit_اشاره_ګر(o, &o->module->elementSegments[elem_شاخص]);

    لكل (u32 i = 0; i < 3; ++i) {
_       (Emit_خونه_عدد_د_ډېری_سر_And_راوباسه(o));
    }

    _catch: أرجع پایله;
}


M3_پایله Compile_Elem_Drop (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_پایله پایله = m3Err_none;

    u32 elem_شاخص;
_   (ReadLEB_u32(&elem_شاخص, &o->wasm, o->wasm_پای_ټکی));
    _throwif("element segment index out of range", elem_شاخص >= o->module->عدد_Element_Segments);

_   (EmitOp(o, op_ElemDrop));
    Emit_اشاره_ګر(o, &o->module->elementSegments[elem_شاخص]);

    _catch: أرجع پایله;
}


M3_پایله Compile_جدول_Copy (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_پایله پایله = m3Err_none;

    M3_جدول* dst;
    M3_جدول* src;

_   (Read_جدول(o, &dst));
_   (Read_جدول(o, &src));
    _throwif(m3Err_typeMismatch, not Is_Sub_Type_د(src->type, dst->type));

_   (Check_عملوند_Type(o, 2, جدول_Addr_Type(dst)));
_   (Check_عملوند_Type(o, 1, جدول_Addr_Type(src)));
_   (Check_عملوند_Type(o, 0, (dst->isTable64 and src->isTable64) ? c_m3Type_i64 : c_m3Type_i32));

_   (Preserve_ثبتوونکی_If_Occupied(o, c_m3Type_i64));
_   (EmitOp(o, op_جدول_Copy));
    Emit_اشاره_ګر(o, dst);
    Emit_اشاره_ګر(o, src);

    لكل (u32 i = 0; i < 3; ++i) {
_       (Emit_خونه_عدد_د_ډېری_سر_And_راوباسه(o));
    }

    _catch: أرجع پایله;
}


M3_پایله Compile_جدول_اندازه (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_پایله پایله = m3Err_none;

    M3_جدول* جدول_2;
_   (Read_جدول(o, &جدول_2));

    // a table size is given in the table's own index type
_   (Compile_جدول_Op(o, op_جدول_اندازه, جدول_2, 0, جدول_Addr_Type(جدول_2)));

    _catch: أرجع پایله;
}


M3_پایله Compile_جدول_Grow_Fill (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_پایله پایله = m3Err_none;

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

    _catch: أرجع پایله;
}

#د_که_پای // d_m3HasRefTypes


M3_پایله Compile_معلومات_Drop (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_پایله پایله = m3Err_none;

    M3_معلومات_Segment* segment = عدم;
_   (Read_معلومات_Segment(o, &segment));

_   (EmitOp(o, op_معلومات_Drop));
    Emit_اشاره_ګر(o, segment);

    _catch: أرجع پایله;
}


ساكن
M3_پایله Read_بلاک_Type (IM3Compilation o, IM3FuncType* o_بلاک_Type)
{
    M3_پایله پایله = m3Err_none;

#إذا d_m3HasTypedRefs
    // a spelled-out reference type is two bytes, so it cannot be told apart
    // from a type index by the s33 that block types otherwise use
    إذا (o->wasm < o->wasm_پای_ټکی and (*o->wasm == d_waEncode_ref or *o->wasm == d_waEncode_refNull)) {
        m3type_t refType;
_       (Parse_ارزښت_Type(o->module, &refType, &o->wasm, o->wasm_پای_ټکی));
        IM3FuncType ftype = عدم;
_       (AllocFuncType(&ftype, 1));
        ftype->عدد_Rets  = 1;
        ftype->types[0] = refType;
_       (Environment_جمع_کړه_Func_Type(o->module->environment, &ftype));
        *o_بلاک_Type = ftype;
        أرجع پایله;
    }
#د_که_پای

    i64 type;
_   (ReadLebSigned(&type, 33, &o->wasm, o->wasm_پای_ټکی));

    إذا (type < 0) {
        _throwif(m3Err_invalidTypeId, type < -64);
        u8 ارزښت_Type;
_       (NormalizeType(&ارزښت_Type, (i8)type));                                 m3log (compile, d_indent " (type: %s)", get_indention_string (o), c_waTypes [ارزښت_Type]);
        *o_بلاک_Type = o->module->environment->retFuncTypes[ارزښت_Type];
    } وإلا {
        _throwif("func type out of bounds", type >= o->module->عدد_Func_Types);
        *o_بلاک_Type = o->module->funcTypes[type];                         m3log (compile, d_indent " (type: %s)", get_indention_string (o), SPrintFuncTypeSignature (*o_بلاک_Type));
    }
    _catch: أرجع پایله;
}

ساكن
M3_پایله Preserve_دلیلونه_And_Locals (IM3Compilation o)
{
    M3_پایله پایله = m3Err_none;

    إذا (o->ډېری_شاخص > o->ډېری_لومړی_Dynamic_شاخص) {
        u32 عدد_دلیلونه_And_Locals = Get_Function_عدد_دلیلونه_And_Locals(o->function);

        لكل (u32 i = 0; i < عدد_دلیلونه_And_Locals; ++i) {
            u16 خونه = Get_خونه_For_ډېری_شاخص(o, i);

            u16 preserved_خونه_شمېره;
_           (پیدا_کړه_Referenced_Local_Within_Current_بلاک(o, &preserved_خونه_شمېره, خونه));

            إذا (preserved_خونه_شمېره != خونه) {
                m3type_t     type = Get_ډېری_Type_From_Bottom(o, i);                 d_m3Assert (type != c_m3Type_none)
                IM3Operation op   = Is64_بټ_Type(type) ? op_Copy_خونه_64 : op_Copy_خونه_32;

                EmitOp(o, op);
                Emit_خونه_دننه_واټن(o, preserved_خونه_شمېره);
                Emit_خونه_دننه_واټن(o, خونه);
            }
        }
    }

    _catch:
    أرجع پایله;
}

ساكن
M3_پایله Compile_Loop_Or_بلاک (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_پایله پایله;

    IM3FuncType بلاک_Type = عدم;

_   (PreserveRegisters(o));
_   (Preserve_دلیلونه_And_Locals(o));

_   (Read_بلاک_Type(o, &بلاک_Type));

    إذا (i_opcode == c_waOp_loop) {
        u16 عدد_Params = Get_Func_Type_عدد_Params(بلاک_Type);
        إذا (عدد_Params) {
            // instantiate constants
            u16 عدد_ارزښتونه = Get_عدد_بلاک_ارزښتونه_On_ډېری(o);

            إذا (عدد_ارزښتونه >= عدد_Params) {
                u16 ډېری_سر = Get_ډېری_سر_شاخص(o) + 1;

                لكل (u16 i = ډېری_سر - عدد_Params; i < ډېری_سر; ++i) {
                    u16      خونه = Get_خونه_For_ډېری_شاخص(o, i);
                    m3type_t type = Get_ډېری_Type_From_Bottom(o, i);

                    إذا (Is_Constant_خونه(o, خونه)) {
                        u16 new_خونه = c_خونه_ناکارول_شوی;
_                       (AllocateSlots(o, &new_خونه, type));
_                       (Copy_ډېری_شاخص_منزل_خونه(o, new_خونه, i));
                        o->wasm_ډېری[i] = new_خونه;
                    }
                }
            }
        }

_       (EmitOp(o, op_Loop));
    } وإلا {
    }

_   (Compile_بلاک(o, بلاک_Type, i_opcode));

    _catch: أرجع پایله;
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
M3_پایله EmitCatchStubs (IM3Compilation o)
{
    IM3_کوډ_پاڼه displaced_پاڼه = عدم;
_try {
    M3CatchClause* clauses    = o->tryClauses;
    u32            عدد_Clauses = o->عدد_Try_Clauses;

    // the body must not see these: a nested try_table sets up its own
    o->tryClauses    = عدم;
    o->عدد_Try_Clauses = 0;

    لكل (u32 i = 0; i < عدد_Clauses; ++i) {
        M3CatchClause* clause = &clauses[i];

        IM3CompilationScope scope;
_       (Get_بلاک_Scope(o, &scope, clause->label_ژوروالی + 1));

        IM3FuncType payload = clause->نښان ? clause->نښان->type : عدم;
        u16         عدد_دلیلونه = Get_Func_Type_عدد_Params(payload);

        // What the stub hands the label - the payload, then the exnref for a
        // _ref clause - has to be what the label takes. ResolveBlockResults
        // only counts the values, and the validator compares base types, so a
        // typed reference is only checked here: (ref null $t) reaching a label
        // that takes (ref $t) has to be rejected.
        {
            bool isLoop         = (scope->opcode == c_waOp_loop);
            u16  عدد_Label_ارزښتونه = isLoop ? Get_Func_Type_عدد_Params(scope->type)
                                         : Get_Func_Type_عدد_Results(scope->type);

            _throwif(m3_Err_type_شمېر_Mismatch, عدد_Label_ارزښتونه != عدد_دلیلونه + (clause->hasRef ? 1 : 0));

            لكل (u16 a = 0; a < عدد_Label_ارزښتونه; ++a) {
                m3type_t given  = (a < عدد_دلیلونه) ? GetFuncTypeParamType(payload, a)
                                                : (m3type_t)c_m3Type_exnref;
                m3type_t wanted = isLoop ? GetFuncTypeParamType(scope->type, a)
                                         : Get_Func_Type_پایله_Type(scope->type, a);

                _throwif(m3Err_typeMismatch, not Is_Sub_Type_د(given, wanted));
            }
        }

        IM3_کوډ_پاڼه stub_پاڼه;
_       (Acquire_Compilation_کوډ_پاڼه(o, &stub_پاڼه));

        pc_t پیل_PC  = Get_پاڼه_PC(stub_پاڼه);
        displaced_پاڼه = o->پاڼه;
        o->پاڼه       = stub_پاڼه;

        u16 لومړی_شاخص = o->ډېری_شاخص;

        لكل (u16 a = 0; a < عدد_دلیلونه; ++a) {
_           (ټېل_Allocated_خونه(o, GetFuncTypeParamType(payload, a)));
        }

        إذا (clause->hasRef) {
_           (ټېل_Allocated_خونه(o, c_m3Type_exnref));
        }

        // emitted even with nothing to move: this is also where an exception
        // no clause will reify gets released
_       (Ensure_کوډ_پاڼه_عدد_کرښې(o, 2 * عدد_دلیلونه + 3 + d_m3_کوډ_پاڼه_Free_کرښې_پوله));

_       (EmitOp(o, op_CatchPayload));
        EmitConstant32(o, عدد_دلیلونه);

        لكل (u16 a = 0; a < عدد_دلیلونه; ++a) {
            u16 index = لومړی_شاخص + a;
            Emit_خونه_دننه_واټن(o, Get_خونه_For_ډېری_شاخص(o, index));
            EmitConstant32(o, Is64_بټ_Type(Get_ډېری_Type_From_Bottom(o, index)));
        }

        Emit_خونه_دننه_واټن(o, clause->hasRef ? (i32)Get_خونه_For_ډېری_شاخص(o, لومړی_شاخص + عدد_دلیلونه) : -1);

_       (Emit_راوباسه_Try_چوکاټونه(o, عدد_Try_چوکاټونه_منزل_راوباسه(o->بلاک.outer, scope)));

        إذا (scope->opcode == c_waOp_loop) {
_           (Resolve_بلاک_Results(o, scope, /* isBranch: */ true));

_           (EmitOp(o, Op_ContinueLoop(o)));
            Emit_اشاره_ګر(o, scope->pc);
_           (ریکارډ_شا_څنډه(o, scope, false));
        } وإلا إذا (scope->ژوروالی == 0) {
_           (Return_ارزښتونه(o, scope, /* isBranch: */ true));
_           (EmitOp(o, op_Return));
        } وإلا {
_           (Resolve_بلاک_Results(o, scope, /* isBranch: */ true));
_           (EmitPatchingBranch(o, scope));
        }

        IM3_کوډ_پاڼه return_منزل = displaced_پاڼه;
        displaced_پاڼه        = عدم;

        Release_Compilation_کوډ_پاڼه(o);
        o->پاڼه = return_منزل;

        *(pc_t*)clause->stub_خونه = پیل_PC;

        طالما (o->ډېری_شاخص > لومړی_شاخص) {
_           (راوباسه(o));
        }
    }

} _catch:

    // thrown with a stub page installed: release it and put back the one it
    // displaced, which the caller's catch then releases
    إذا (displaced_پاڼه) {
        Release_Compilation_کوډ_پاڼه(o);
        o->پاڼه = displaced_پاڼه;
    }

    أرجع پایله;
}


ساكن
M3_پایله Compile_Try_جدول (IM3Compilation o, m3opcode_t i_opcode)
{
    M3CatchClause* clauses = عدم;
_try {
    // a catch entry has to be able to assume the operand stack is entirely in
    // slots: an unwind leaves nothing in the registers
_   (PreserveRegisters(o));
_   (Preserve_دلیلونه_And_Locals(o));

    IM3FuncType بلاک_Type = عدم;
_   (Read_بلاک_Type(o, &بلاک_Type));

    u32 عدد_Clauses;
_   (ReadLEB_u32(&عدد_Clauses, &o->wasm, o->wasm_پای_ټکی));

    _throwif("too many catch clauses", عدد_Clauses > d_m3_Max_Sane_نښانونه_شمېر);

    إذا (عدد_Clauses) {
        clauses = m3_AllocArray(M3CatchClause, عدد_Clauses);
        _throwifnull(clauses);
    }

    لكل (u32 i = 0; i < عدد_Clauses; ++i) {
        u8 ډول;
_       (Read_u8(&ډول, &o->wasm, o->wasm_پای_ټکی));
        _throwif(m3Err_wasmMalformed, ډول > 0x03);

        clauses[i].نښان    = عدم;
        clauses[i].hasRef = (ډول == 0x01 or ډول == 0x03);

        إذا (ډول == 0x00 or ډول == 0x01) {
            u32 نښان_شاخص;
_           (ReadLEB_u32(&نښان_شاخص, &o->wasm, o->wasm_پای_ټکی));
            _throwif(m3_Err_unknown_نښان, نښان_شاخص >= o->module->عدد_نښانونه);

            IM3_نښان نښان     = &o->module->نښانونه[نښان_شاخص];
            clauses[i].نښان = نښان->resolved ? نښان->resolved : نښان;
        }

_       (ReadLEB_u32(&clauses[i].label_ژوروالی, &o->wasm, o->wasm_پای_ټکی));
    }

    // op_TryTable, the clause count, and two words per clause, all of which have
    // to land on one page: the op reads the table by offset from its own pc
_   (Ensure_کوډ_پاڼه_عدد_کرښې(o, 2 * عدد_Clauses + 2 + d_m3_کوډ_پاڼه_Free_کرښې_پوله));

_   (EmitOp(o, op_Try_جدول));
    EmitConstant32(o, عدد_Clauses);

    لكل (u32 i = 0; i < عدد_Clauses; ++i) {
        Emit_اشاره_ګر(o, clauses[i].نښان);
        clauses[i].stub_خونه = Reserve_اشاره_ګر(o);
    }

    o->tryClauses    = clauses;
    o->عدد_Try_Clauses = عدد_Clauses;

_   (Compile_بلاک(o, بلاک_Type, c_wa_Op_try_جدول));

} _catch:

    o->tryClauses    = عدم;
    o->عدد_Try_Clauses = 0;

    m3_Free(clauses);

    أرجع پایله;
}


ساكن
M3_پایله Compile_Throw (IM3Compilation o, m3opcode_t i_opcode)
{
_try {
    u32 نښان_شاخص;
_   (ReadLEB_u32(&نښان_شاخص, &o->wasm, o->wasm_پای_ټکی));
    _throwif(m3_Err_unknown_نښان, نښان_شاخص >= o->module->عدد_نښانونه);

    IM3_نښان نښان = &o->module->نښانونه[نښان_شاخص];
    إذا (نښان->resolved) {
        نښان = نښان->resolved;
    }

    u16 عدد_دلیلونه = Get_Func_Type_عدد_Params(نښان->type);

    // the payload is read out of slots, so nothing may still be in a register
_   (PreserveRegisters(o));

    // check the payload before emitting anything, so a mistyped throw does not
    // leave half an operation behind
    bool isPolymorphic = Is_ډېری_Polymorphic(o);

    إذا (not isPolymorphic) {
        _throwif(m3_Err_type_شمېر_Mismatch, Get_عدد_بلاک_ارزښتونه_On_ډېری(o) < عدد_دلیلونه);

        لكل (u16 i = 0; i < عدد_دلیلونه; ++i) {
            u16 index = (u16)(o->ډېری_شاخص - عدد_دلیلونه + i);

            _throwif(m3Err_typeMismatch, not Is_Sub_Type_د(Get_ډېری_Type_From_Bottom(o, index),
                                                         GetFuncTypeParamType(نښان->type, i)));
        }
    }

_   (Ensure_کوډ_پاڼه_عدد_کرښې(o, 2 * عدد_دلیلونه + 3 + d_m3_کوډ_پاڼه_Free_کرښې_پوله));

_   (EmitOp(o, op_Throw));
    Emit_اشاره_ګر(o, نښان);
    EmitConstant32(o, عدد_دلیلونه);

    // the payload is named in the order the tag declares it, so the deepest of
    // the arguments on the stack comes first. In unreachable code there is
    // nothing on the stack to name and nothing will run this, so slot 0 stands
    // in for an operand that isn't there.
    لكل (u16 i = 0; i < عدد_دلیلونه; ++i) {
        m3type_t type = GetFuncTypeParamType(نښان->type, i);

        Emit_خونه_دننه_واټن(o, isPolymorphic ? 0 : Get_خونه_For_ډېری_شاخص(o, (u16)(o->ډېری_شاخص - عدد_دلیلونه + i)));
        EmitConstant32(o, Is64_بټ_Type(type));
    }

    إذا (not isPolymorphic) {
        لكل (u16 i = 0; i < عدد_دلیلونه; ++i) {
_           (راوباسه(o));
        }
    }

_   (سېټ_ډېری_Polymorphic(o));

} _catch: أرجع پایله;
}


ساكن
M3_پایله Compile_ThrowRef (IM3Compilation o, m3opcode_t i_opcode)
{
_try {
    إذا (not Is_ډېری_Polymorphic(o)) {
_       (Preserve_ثبتوونکی_If_Occupied(o, c_m3Type_i64));

        _throwif(m3Err_typeMismatch, بنسټ_Type_د(Get_ډېری_سر_Type(o)) != c_m3Type_exnref);

_       (EmitOp(o, op_ThrowRef));
        Emit_خونه_دننه_واټن(o, Get_ډېری_سر_خونه_شمېره(o));

_       (راوباسه(o));
    }

_   (سېټ_ډېری_Polymorphic(o));

} _catch: أرجع پایله;
}

#د_که_پای // d_m3HasExceptionHandling

#إذا d_m3_Has_ډېری_Switching

ساكن
M3_پایله Compile_ContNew (IM3Compilation o, m3opcode_t i_opcode)
{
_try {
    u32 type_شاخص;
_   (ReadLEB_u32(&type_شاخص, &o->wasm, o->wasm_پای_ټکی));
    _throwif(m3_Err_type_شمېر_Mismatch, type_شاخص >= o->module->عدد_Func_Types);

    IM3FuncType funcType = o->module->funcTypes[type_شاخص];
    _throwif("type is not a continuation", not funcType->isContinuation);

    // the operands are read out of slots, so nothing may still be in a register
_   (PreserveRegisters(o));

    // cont.new $ct : [(ref null $ft)] -> [(ref $ct)], where $ct ~~ cont $ft:
    // the function has to be one this continuation type can run
    m3type_t سر_Type = Get_ډېری_سر_Type(o);
    _throwif(m3Err_typeMismatch,
             not Is_Sub_Type_د(سر_Type, Ref_Type_د_Func_Type(funcType->contFuncType, false)));

    u16 func_خونه = Get_ډېری_سر_خونه_شمېره(o);
_   (راوباسه(o));

    u16 dst_خونه;
_   (ټېل_Allocated_خونه(o, Ref_Type_د_Func_Type(funcType, true)));
    dst_خونه = Get_ډېری_سر_خونه_شمېره(o);

_   (Ensure_کوډ_پاڼه_عدد_کرښې(o, 4 + d_m3_کوډ_پاڼه_Free_کرښې_پوله));
_   (EmitOp(o, op_ContNew));
    Emit_اشاره_ګر(o, funcType);
    Emit_خونه_دننه_واټن(o, dst_خونه);
    Emit_خونه_دننه_واټن(o, func_خونه);

} _catch: أرجع پایله;
}

ساكن
M3_پایله Compile_ContBind (IM3Compilation o, m3opcode_t i_opcode)
{
_try {
    u32 typeIndex1, typeIndex2;
_   (ReadLEB_u32(&typeIndex1, &o->wasm, o->wasm_پای_ټکی));
_   (ReadLEB_u32(&typeIndex2, &o->wasm, o->wasm_پای_ټکی));
    _throwif(m3_Err_type_شمېر_Mismatch, typeIndex1 >= o->module->عدد_Func_Types);
    _throwif(m3_Err_type_شمېر_Mismatch, typeIndex2 >= o->module->عدد_Func_Types);

    IM3FuncType ct1 = o->module->funcTypes[typeIndex1];
    IM3FuncType ct2 = o->module->funcTypes[typeIndex2];
    _throwif("type is not a continuation", not ct1->isContinuation || not ct2->isContinuation);

    // the operands are read out of slots, so nothing may still be in a register
_   (PreserveRegisters(o));

    m3type_t contType = Get_ډېری_سر_Type(o);
    _throwif(m3Err_typeMismatch, not Is_Sub_Type_د(contType, Ref_Type_د_Func_Type(ct1, false)));
    u16 cont_خونه = Get_ډېری_سر_خونه_شمېره(o);
_   (راوباسه(o));

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
        _throwif(m3Err_typeMismatch, Get_Func_Type_پایله_Type(ft1, i) != Get_Func_Type_پایله_Type(ft2, i));
    }
    لكل (u16 i = 0; i < عدد_Params2; ++i) {
        _throwif(m3Err_typeMismatch, GetFuncTypeParamType(ft1, عدد_Bound + i) != GetFuncTypeParamType(ft2, i));
    }

    u16 argSlots[d_m3MaxContinuationPayload];
    _throwif("too many bound args", عدد_Bound > d_m3MaxContinuationPayload);

    لكل (u32 i = 0; i < عدد_Bound; ++i) {
        u32      paramIdx     = عدد_Bound - 1 - i;
        m3type_t expectedType = GetFuncTypeParamType(ft1, paramIdx);
        m3type_t actualType   = Get_ډېری_سر_Type(o);
        _throwif(m3Err_typeMismatch, not Is_Sub_Type_د(actualType, expectedType));
        argSlots[paramIdx] = Get_ډېری_سر_خونه_شمېره(o);
_       (راوباسه(o));
    }

    u16 dst_خونه;
_   (ټېل_Allocated_خونه(o, Ref_Type_د_Func_Type(ct2, true)));
    dst_خونه = Get_ډېری_سر_خونه_شمېره(o);

_   (Ensure_کوډ_پاڼه_عدد_کرښې(o, 6 + عدد_Bound * 2 + d_m3_کوډ_پاڼه_Free_کرښې_پوله));
_   (EmitOp(o, op_ContBind));
    Emit_اشاره_ګر(o, ct1);
    Emit_اشاره_ګر(o, ct2);
    Emit_خونه_دننه_واټن(o, dst_خونه);
    Emit_خونه_دننه_واټن(o, cont_خونه);
    EmitConstant32(o, عدد_Bound);
    لكل (u32 i = 0; i < عدد_Bound; ++i) {
        Emit_خونه_دننه_واټن(o, argSlots[i]);
        EmitConstant32(o, Is64_بټ_Type(GetFuncTypeParamType(ft1, i)));
    }

} _catch: أرجع پایله;
}

ساكن
M3_پایله Compile_Suspend (IM3Compilation o, m3opcode_t i_opcode)
{
_try {
    u32 نښان_شاخص;
_   (ReadLEB_u32(&نښان_شاخص, &o->wasm, o->wasm_پای_ټکی));
    _throwif(m3_Err_unknown_نښان, نښان_شاخص >= o->module->عدد_نښانونه);

    IM3_نښان نښان = &o->module->نښانونه[نښان_شاخص];
    إذا (نښان->resolved) {
        نښان = نښان->resolved;
    }
    u16 عدد_Params  = Get_Func_Type_عدد_Params(نښان->type);
    u16 عدد_Results = Get_Func_Type_عدد_Results(نښان->type);

_   (PreserveRegisters(o));

    u16 paramSlots[d_m3MaxContinuationPayload];
    _throwif("too many tag params", عدد_Params > d_m3MaxContinuationPayload);
    لكل (u16 i = 0; i < عدد_Params; ++i) {
        u16      pIdx         = عدد_Params - 1 - i;
        m3type_t expectedType = GetFuncTypeParamType(نښان->type, pIdx);
        m3type_t actualType   = Get_ډېری_سر_Type(o);
        _throwif(m3Err_typeMismatch, not Is_Sub_Type_د(actualType, expectedType));
        paramSlots[pIdx] = Get_ډېری_سر_خونه_شمېره(o);
_       (راوباسه(o));
    }

    u16 پایله_Slots[d_m3MaxContinuationPayload];
    _throwif("too many tag results", عدد_Results > d_m3MaxContinuationPayload);
    لكل (u16 i = 0; i < عدد_Results; ++i) {
        m3type_t rtype = Get_Func_Type_پایله_Type(نښان->type, i);
_       (ټېل_Allocated_خونه(o, rtype));
        پایله_Slots[i] = Get_ډېری_سر_خونه_شمېره(o);
    }

_   (Ensure_کوډ_پاڼه_عدد_کرښې(o, 4 + عدد_Params * 2 + عدد_Results * 2 + d_m3_کوډ_پاڼه_Free_کرښې_پوله));
_   (EmitOp(o, op_Suspend));
    Emit_اشاره_ګر(o, نښان);
    EmitConstant32(o, عدد_Params);
    لكل (u16 i = 0; i < عدد_Params; ++i) {
        Emit_خونه_دننه_واټن(o, paramSlots[i]);
        EmitConstant32(o, Is64_بټ_Type(GetFuncTypeParamType(نښان->type, i)));
    }
    EmitConstant32(o, عدد_Results);
    لكل (u16 i = 0; i < عدد_Results; ++i) {
        Emit_خونه_دننه_واټن(o, پایله_Slots[i]);
        EmitConstant32(o, Is64_بټ_Type(Get_Func_Type_پایله_Type(نښان->type, i)));
    }

    // the results are part of the frame here: op_Suspend clears them, and
    // whatever resumes this writes them before anything reads them
_   (ریکارډ_خوندي_ټکی(o, safepoint_suspend, 0, عدد_Results, 0, عدم));

} _catch: أرجع پایله;
}

// resume, resume_throw and resume_throw_ref. All three run a continuation
// under a handler table and take its results; they differ only in what they
// hand it to start with - arguments, a fresh exception, or one off the stack.
//
// Emitted as: the continuation type, the slot holding it, the operands that
// belong to this form, the handler table, then the result slots. Keeping the
// tail identical is what lets one runtime path finish all three.
ساكن
M3_پایله Compile_Resume (IM3Compilation o, m3opcode_t i_opcode)
{
    M3ResumeHandler* handlers      = عدم;
    IM3_کوډ_پاڼه      displaced_پاڼه = عدم;
_try {
    bool isThrow    = (i_opcode == c_waOp_resumeThrow);
    bool isThrowRef = (i_opcode == c_waOp_resumeThrowRef);

    u32 type_شاخص;
_   (ReadLEB_u32(&type_شاخص, &o->wasm, o->wasm_پای_ټکی));
    _throwif(m3_Err_type_شمېر_Mismatch, type_شاخص >= o->module->عدد_Func_Types);

    IM3FuncType contFuncType = o->module->funcTypes[type_شاخص];
    _throwif("type is not a continuation", not contFuncType->isContinuation);

    IM3FuncType innerType      = contFuncType->contFuncType;
    u16         عدد_Cont_Results = Get_Func_Type_عدد_Results(innerType);

    IM3_نښان exn_نښان      = عدم;
    u16    عدد_Operands = 0;

    إذا (isThrow) {
        u32 نښان_شاخص;
_       (ReadLEB_u32(&نښان_شاخص, &o->wasm, o->wasm_پای_ټکی));
        _throwif(m3_Err_unknown_نښان, نښان_شاخص >= o->module->عدد_نښانونه);

        exn_نښان = &o->module->نښانونه[نښان_شاخص];

        // it is an exception that gets raised, so the tag must be one throw
        // could have raised: no results
        _throwif(m3Err_typeMismatch, Get_Func_Type_عدد_Results(exn_نښان->type) != 0);

        عدد_Operands = Get_Func_Type_عدد_Params(exn_نښان->type);
    } وإلا إذا (not isThrowRef) {
        // a plain resume hands over the continuation's own parameters. An
        // aborted one never receives them, so they are unconstrained there.
        عدد_Operands = Get_Func_Type_عدد_Params(innerType);
    }

    u32 عدد_Clauses;
_   (ReadLEB_u32(&عدد_Clauses, &o->wasm, o->wasm_پای_ټکی));

    u32 clauseLabelDepths[16];
    _throwif("too many resume clauses", عدد_Clauses > 16);

    إذا (عدد_Clauses) {
        handlers = m3_AllocArray(M3ResumeHandler, عدد_Clauses);
        _throwifnull(handlers);
    }

    لكل (u32 i = 0; i < عدد_Clauses; ++i) {
        u8 ډول;
_       (Read_u8(&ډول, &o->wasm, o->wasm_پای_ټکی));
        _throwif(m3Err_wasmMalformed, ډول > 0x01);

        handlers[i].ډول = ډول;

        u32 نښان_شاخص;
_       (ReadLEB_u32(&نښان_شاخص, &o->wasm, o->wasm_پای_ټکی));
        _throwif(m3_Err_unknown_نښان, نښان_شاخص >= o->module->عدد_نښانونه);

        IM3_نښان نښان         = &o->module->نښانونه[نښان_شاخص];
        handlers[i].نښان    = نښان->resolved ? نښان->resolved : نښان;
        handlers[i].stubPC = عدم;

        إذا (ډول == 0x00) {
_           (ReadLEB_u32(&clauseLabelDepths[i], &o->wasm, o->wasm_پای_ټکی));
        } وإلا {
            // (on $e switch): the tag carries no payload, and what the switch
            // site yields is what this resume yields
            _throwif(m3Err_typeMismatch, Get_Func_Type_عدد_Params(handlers[i].نښان->type) != 0);
            clauseLabelDepths[i] = 0;
        }
    }

    // the operands are read out of slots, so nothing may still be in a register
_   (PreserveRegisters(o));

    m3type_t cont_سر_Type = Get_ډېری_سر_Type(o);
    _throwif(m3Err_typeMismatch, not Is_Sub_Type_د(cont_سر_Type, Ref_Type_د_Func_Type(contFuncType, false)));
    u16 cont_خونه = Get_ډېری_سر_خونه_شمېره(o);
_   (راوباسه(o));

    u16 exn_خونه = 0;

    إذا (isThrowRef) {
        m3type_t exnType = Get_ډېری_سر_Type(o);
        _throwif(m3Err_typeMismatch, بنسټ_Type_د(exnType) != c_m3Type_exnref);
        exn_خونه = Get_ډېری_سر_خونه_شمېره(o);
_       (راوباسه(o));
    }

    u16 عملوند_Slots[d_m3MaxContinuationPayload];
    _throwif("too many resume operands", عدد_Operands > d_m3MaxContinuationPayload);

    IM3FuncType عملوند_Type = isThrow ? exn_نښان->type : innerType;

    لكل (u16 i = 0; i < عدد_Operands; ++i) {
        u16      pIdx         = (u16)(عدد_Operands - 1 - i);
        m3type_t expectedType = GetFuncTypeParamType(عملوند_Type, pIdx);
        m3type_t actualType   = Get_ډېری_سر_Type(o);
        _throwif(m3Err_typeMismatch, not Is_Sub_Type_د(actualType, expectedType));
        عملوند_Slots[pIdx] = Get_ډېری_سر_خونه_شمېره(o);
_       (راوباسه(o));
    }

    لكل (u32 i = 0; i < عدد_Clauses; ++i) {
        إذا (handlers[i].ډول != 0x00) {
            استمر;
        }

        IM3CompilationScope scope;
_       (Get_بلاک_Scope(o, &scope, clauseLabelDepths[i]));

        IM3_نښان نښان        = handlers[i].نښان;
        u16    عدد_Payload = Get_Func_Type_عدد_Params(نښان->type);

        m3type_t labelContType = (scope->opcode == c_waOp_loop)
                                   ? GetFuncTypeParamType(scope->type, عدد_Payload)
                                   : Get_Func_Type_پایله_Type(scope->type, عدد_Payload);

        IM3_کوډ_پاڼه stub_پاڼه;
_       (Acquire_Compilation_کوډ_پاڼه(o, &stub_پاڼه));
        pc_t پیل_PC  = Get_پاڼه_PC(stub_پاڼه);
        displaced_پاڼه = o->پاڼه;
        o->پاڼه       = stub_پاڼه;

        u16 لومړی_شاخص = o->ډېری_شاخص;
        لكل (u16 a = 0; a < عدد_Payload; ++a) {
_           (ټېل_Allocated_خونه(o, GetFuncTypeParamType(نښان->type, a)));
        }

        // The handler is handed the suspended continuation.
_       (ټېل_Allocated_خونه(o, labelContType));

_       (Ensure_کوډ_پاڼه_عدد_کرښې(o, 2 * عدد_Payload + 5 + d_m3_کوډ_پاڼه_Free_کرښې_پوله));
_       (EmitOp(o, op_ResumePayload));
        Emit_اشاره_ګر(o, Module_Cont_Type_د_Ref(o->module, labelContType));
        EmitConstant32(o, عدد_Payload);
        لكل (u16 a = 0; a < عدد_Payload; ++a) {
            u16 index = لومړی_شاخص + a;
            Emit_خونه_دننه_واټن(o, Get_خونه_For_ډېری_شاخص(o, index));
            EmitConstant32(o, Is64_بټ_Type(Get_ډېری_Type_From_Bottom(o, index)));
        }
        Emit_خونه_دننه_واټن(o, Get_خونه_For_ډېری_شاخص(o, لومړی_شاخص + عدد_Payload));

#  إذا d_m3HasExceptionHandling
        // the clause leaves every try region between the resume and its label,
        // and leaving one retires its handlers, the way a branch does
_       (Emit_راوباسه_Try_چوکاټونه(o, عدد_Try_چوکاټونه_منزل_راوباسه(&o->بلاک, scope)));
#  endif

        إذا (scope->opcode == c_waOp_loop) {
_           (Resolve_بلاک_Results(o, scope, true));
_           (EmitOp(o, Op_ContinueLoop(o)));
            Emit_اشاره_ګر(o, scope->pc);
_           (ریکارډ_شا_څنډه(o, scope, false));
        } وإلا إذا (scope->ژوروالی == 0) {
_           (Return_ارزښتونه(o, scope, true));
_           (EmitOp(o, op_Return));
        } وإلا {
_           (Resolve_بلاک_Results(o, scope, true));
_           (EmitPatchingBranch(o, scope));
        }

        IM3_کوډ_پاڼه return_منزل = displaced_پاڼه;
        displaced_پاڼه        = عدم;

        Release_Compilation_کوډ_پاڼه(o);
        o->پاڼه = return_منزل;

        handlers[i].stubPC = پیل_PC;

        طالما (o->ډېری_شاخص > لومړی_شاخص) {
_           (راوباسه(o));
        }
    }

    u16 resSlots[d_m3MaxContinuationPayload];
    _throwif("too many cont results", عدد_Cont_Results > d_m3MaxContinuationPayload);
    لكل (u16 i = 0; i < عدد_Cont_Results; ++i) {
        m3type_t rtype = Get_Func_Type_پایله_Type(innerType, i);
_       (ټېل_Allocated_خونه(o, rtype));
        resSlots[i] = Get_ډېری_سر_خونه_شمېره(o);
    }

_   (Ensure_کوډ_پاڼه_عدد_کرښې(o, 8 + عدد_Operands * 2 + عدد_Clauses * 3 + عدد_Cont_Results * 2 +
                                  d_m3_کوډ_پاڼه_Free_کرښې_پوله));

_   (EmitOp(o, op_Resume));
    Emit_اشاره_ګر(o, contFuncType);
    Emit_خونه_دننه_واټن(o, cont_خونه);
    EmitConstant32(o, isThrow ? d_m3ResumeThrow : (isThrowRef ? d_m3ResumeThrowRef : d_m3_Resume_نورمال));

    إذا (isThrowRef) {
        Emit_خونه_دننه_واټن(o, exn_خونه);
    } وإلا {
        إذا (isThrow) {
            Emit_اشاره_ګر(o, exn_نښان);
        }

        EmitConstant32(o, عدد_Operands);
        لكل (u16 i = 0; i < عدد_Operands; ++i) {
            Emit_خونه_دننه_واټن(o, عملوند_Slots[i]);
            EmitConstant32(o, Is64_بټ_Type(GetFuncTypeParamType(عملوند_Type, i)));
        }
    }

    EmitConstant32(o, عدد_Clauses);
    لكل (u32 i = 0; i < عدد_Clauses; ++i) {
        EmitConstant32(o, handlers[i].ډول);
        Emit_اشاره_ګر(o, handlers[i].نښان);
        Emit_اشاره_ګر(o, handlers[i].stubPC);
    }

    EmitConstant32(o, عدد_Cont_Results);
    لكل (u16 i = 0; i < عدد_Cont_Results; ++i) {
        Emit_خونه_دننه_واټن(o, resSlots[i]);
        EmitConstant32(o, Is64_بټ_Type(Get_Func_Type_پایله_Type(innerType, i)));
    }

    // a suspend passing through the resumed continuation leaves this frame
    // waiting here, with the continuation's results not yet taken
_   (ریکارډ_خوندي_ټکی(o, safepoint_resume, 0, عدد_Cont_Results, (u16)عدد_Clauses, عدم));
#  إذا d_m3HasSnapshots
    إذا (o->snapshotMap and o->پاڼه) {
        o->snapshotMap->خوندي_Points[o->snapshotMap->عدد_خوندي_Points - 1].resumeType = Ref_Type_د_Func_Type(contFuncType, true);
    }
#  endif

} _catch:
    إذا (displaced_پاڼه) {
        Release_Compilation_کوډ_پاڼه(o);
        o->پاڼه = displaced_پاڼه;
    }
    m3_Free(handlers);
    أرجع پایله;
}


ساكن
M3_پایله Compile_Switch (IM3Compilation o, m3opcode_t i_opcode)
{
_try {
    u32 type_شاخص, نښان_شاخص;
_   (ReadLEB_u32(&type_شاخص, &o->wasm, o->wasm_پای_ټکی));
_   (ReadLEB_u32(&نښان_شاخص, &o->wasm, o->wasm_پای_ټکی));
    _throwif(m3_Err_type_شمېر_Mismatch, type_شاخص >= o->module->عدد_Func_Types);
    _throwif(m3_Err_unknown_نښان, نښان_شاخص >= o->module->عدد_نښانونه);

    IM3FuncType موخه_Cont_Type = o->module->funcTypes[type_شاخص];
    IM3_نښان      نښان            = &o->module->نښانونه[نښان_شاخص];
    إذا (نښان->resolved) {
        نښان = نښان->resolved;
    }

    _throwif("type is not a continuation", not موخه_Cont_Type->isContinuation);

    // a switch tag carries no payload of its own: the values travel as the
    // peer's arguments, and the tag's results are what the switch site yields
    _throwif(m3Err_typeMismatch, Get_Func_Type_عدد_Params(نښان->type) != 0);

    // switch $ct1 : [t1* (ref null $ct1)] -> [t2*]
    //   where $ct1 = cont [t1* (ref null? $ct2)] -> [..]
    //   and   $ct2 = cont [t2*] -> [..]
    //
    // The trailing parameter of $ct1 is not an operand: the peer receives the
    // continuation this instruction suspends. And what this instruction yields
    // is not $ct1's results but $ct2's parameters - the values whoever comes
    // back this way will hand over.
    IM3FuncType موخه_Type = موخه_Cont_Type->contFuncType;
    u16         عدد_Params  = Get_Func_Type_عدد_Params(موخه_Type);

    _throwif("switch target takes no continuation", عدد_Params == 0);

    m3type_t    peerRefType  = GetFuncTypeParamType(موخه_Type, (u16)(عدد_Params - 1));
    IM3FuncType selfContType = Module_Cont_Type_د_Ref(o->module, peerRefType);

    _throwif(m3Err_typeMismatch, not selfContType);

    IM3FuncType selfType   = selfContType->contFuncType;
    u16         عدد_دلیلونه    = (u16)(عدد_Params - 1);
    u16         عدد_Results = Get_Func_Type_عدد_Params(selfType);

    // the operands are read out of slots, so nothing may still be in a register
_   (PreserveRegisters(o));

    m3type_t cont_سر_Type = Get_ډېری_سر_Type(o);
    _throwif(m3Err_typeMismatch, not Is_Sub_Type_د(cont_سر_Type, Ref_Type_د_Func_Type(موخه_Cont_Type, false)));
    u16 موخه_Cont_خونه = Get_ډېری_سر_خونه_شمېره(o);
_   (راوباسه(o));

    u16 argSlots[d_m3MaxContinuationPayload];
    _throwif("too many switch params", عدد_دلیلونه > d_m3MaxContinuationPayload);

    لكل (u16 i = 0; i < عدد_دلیلونه; ++i) {
        u16      pIdx         = (u16)(عدد_دلیلونه - 1 - i);
        m3type_t expectedType = GetFuncTypeParamType(موخه_Type, pIdx);
        m3type_t actualType   = Get_ډېری_سر_Type(o);
        _throwif(m3Err_typeMismatch, not Is_Sub_Type_د(actualType, expectedType));
        argSlots[pIdx] = Get_ډېری_سر_خونه_شمېره(o);
_       (راوباسه(o));
    }

    u16 پایله_Slots[d_m3MaxContinuationPayload];
    _throwif("too many switch results", عدد_Results > d_m3MaxContinuationPayload);

    لكل (u16 i = 0; i < عدد_Results; ++i) {
_       (ټېل_Allocated_خونه(o, GetFuncTypeParamType(selfType, i)));
        پایله_Slots[i] = Get_ډېری_سر_خونه_شمېره(o);
    }

_   (Ensure_کوډ_پاڼه_عدد_کرښې(o, 6 + عدد_دلیلونه * 2 + عدد_Results * 2 + d_m3_کوډ_پاڼه_Free_کرښې_پوله));
_   (EmitOp(o, op_Switch));
    Emit_اشاره_ګر(o, موخه_Cont_Type);
    Emit_اشاره_ګر(o, نښان);
    Emit_خونه_دننه_واټن(o, موخه_Cont_خونه);
    EmitConstant32(o, عدد_دلیلونه);
    لكل (u16 i = 0; i < عدد_دلیلونه; ++i) {
        Emit_خونه_دننه_واټن(o, argSlots[i]);
        EmitConstant32(o, Is64_بټ_Type(GetFuncTypeParamType(موخه_Type, i)));
    }
    EmitConstant32(o, عدد_Results);
    لكل (u16 i = 0; i < عدد_Results; ++i) {
        Emit_خونه_دننه_واټن(o, پایله_Slots[i]);
        EmitConstant32(o, Is64_بټ_Type(GetFuncTypeParamType(selfType, i)));
    }

    // as with suspend: op_Switch clears the results before it leaves
_   (ریکارډ_خوندي_ټکی(o, safepoint_suspend, 0, عدد_Results, 0, عدم));

} _catch: أرجع پایله;
}

#د_که_پای // d_m3HasStackSwitching


ساكن
M3_پایله Compile_Else_بلاک (IM3Compilation o, pc_t* o_پیل_PC, IM3FuncType i_بلاک_Type)
{
    IM3_کوډ_پاڼه saved_پاڼه = o->پاڼه;
_try {

    IM3_کوډ_پاڼه else_پاڼه;
_   (Acquire_Compilation_کوډ_پاڼه(o, &else_پاڼه));

    *o_پیل_PC = Get_پاڼه_PC(else_پاڼه);

    o->پاڼه = else_پاڼه;

_   (Compile_بلاک(o, i_بلاک_Type, c_waOp_else));

_   (EmitOp(o, op_Branch));
    Emit_اشاره_ګر(o, Get_پاڼه_PC(saved_پاڼه));
} _catch:

    إذا (o->پاڼه != saved_پاڼه) {
        Release_Compilation_کوډ_پاڼه(o);
    }

    o->پاڼه = saved_پاڼه;

    أرجع پایله;
}

ساكن
M3_پایله Compile_If (IM3Compilation o, m3opcode_t i_opcode)
{
    /*      [   op_If   ]
            [ <else-pc> ]   ---->   [ ..else..  ]
            [  ..if..   ]           [ ..block.. ]
            [ ..block.. ]           [ op_Branch ]
            [    end    ]  <-----   [  <end-pc> ]       */

_try {

    // Spec: if condition must be i32
    إذا (not Is_ډېری_Polymorphic(o)) {
        m3type_t condType = Get_ډېری_سر_Type(o);
        _throwif(m3Err_typeMismatch, condType != c_m3Type_none and condType != c_m3Type_i32);
    }

_   (Preserve_Non_سر_Registers(o));
_   (Preserve_دلیلونه_And_Locals(o));

#إذا d_m3FuseBranch
    IM3Operation fuseOp = GetBranchFusionOp(o, true);

    إذا (fuseOp) {
        // absorb the if into the compare that produced the condition
        *(IM3Operation*)o->foldPatchPC = fuseOp;
        InvalidateFold(o);

_       (راوباسه(o));                          // the condition is consumed inside the fused op
    } وإلا
#د_که_پای
    {
        IM3Operation op = Is_ډېری_سر_دننه_ثبتوونکی(o) ? op_If_r : op_If_s;

_       (EmitOp(o, op));
_       (Emit_خونه_عدد_د_ډېری_سر_And_راوباسه(o));
    }

    pc_t* pc = (pc_t*)Reserve_اشاره_ګر(o);

    IM3FuncType بلاک_Type = عدم;
_   (Read_بلاک_Type(o, &بلاک_Type));     //  dump_type_stack (o);

    u16 ډېری_شاخص = o->ډېری_شاخص;

_   (Compile_بلاک(o, بلاک_Type, i_opcode));

    إذا (o->پخوانی_Opcode == c_waOp_else) {
        o->ډېری_شاخص = ډېری_شاخص;
_       (Compile_Else_بلاک(o, pc, بلاک_Type));
    } وإلا {
        // if block produces values and there isn't a defined else
        // case, then we need to make one up so that the pass-through
        // results end up in the right place
        إذا (Get_Func_Type_عدد_Results(بلاک_Type)) {
            // rewind to the if's end to create a fake else block
            o->wasm--;
            o->ډېری_شاخص = ډېری_شاخص;     // dump_type_stack (o);

_           (Compile_Else_بلاک(o, pc, بلاک_Type));
        } وإلا {
            *pc = GetPC(o);
        }
    }

    } _catch: أرجع پایله;
}

ساكن
M3_پایله Compile_Select (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_پایله پایله = m3Err_none;

    u16 slots[3] = { c_خونه_ناکارول_شوی, c_خونه_ناکارول_شوی, c_خونه_ناکارول_شوی };

    IM3Operation op = عدم;

    m3type_t type = Get_ډېری_Type_From_سر(o, 1); // get type of selection

    إذا (not Is_ډېری_Polymorphic(o)) {
        // Spec: the condition operand (top) must be i32
        m3type_t condType = Get_ډېری_Type_From_سر(o, 0);
        _throwif(m3Err_typeMismatch, condType != c_m3Type_none and condType != c_m3Type_i32);

        // Spec: the two value operands (below condition) must have matching types
        m3type_t type2 = Get_ډېری_Type_From_سر(o, 2);
        _throwif(m3Err_typeMismatch, type != c_m3Type_none and type2 != c_m3Type_none and
                                       not (Is_Sub_Type_د(type, type2) or Is_Sub_Type_د(type2, type)));
    }

    إذا (IsFpType(type)) {
#إذا d_m3HasFloat
        // not consuming a fp reg, so preserve
        إذا (not Is_ډېری_سر_Minus1_دننه_ثبتوونکی(o) and
            not Is_ډېری_سر_Minus2_دننه_ثبتوونکی(o)) {
_           (Preserve_ثبتوونکی_If_Occupied(o, type));
        }

        bool selector_دننه_Reg = Is_ډېری_سر_دننه_ثبتوونکی(o);
        slots[0]           = Get_ډېری_سر_خونه_شمېره(o);
_       (راوباسه(o));

        u32 op_شاخص = 0;

        لكل (u32 i = 1; i <= 2; ++i) {
            إذا (Is_ډېری_سر_دننه_ثبتوونکی(o)) {
                op_شاخص = i;
            } وإلا {
                slots[i] = Get_ډېری_سر_خونه_شمېره(o);
            }

_           (راوباسه(o));
        }

        op = c_fpSelectOps[type - c_m3Type_f32][selector_دننه_Reg][op_شاخص];
#وإلا
        _throw(m3Err_unknownOpcode);
#د_که_پای
    } وإلا إذا (IsIntType(type) or IsRefType(type)) {
        // 'sss' operation doesn't consume a register, so might have to protected its contents
        إذا (not Is_ډېری_سر_دننه_ثبتوونکی(o) and
            not Is_ډېری_سر_Minus1_دننه_ثبتوونکی(o) and
            not Is_ډېری_سر_Minus2_دننه_ثبتوونکی(o)) {
_           (Preserve_ثبتوونکی_If_Occupied(o, type));
        }

        u32 op_شاخص = 3;  // op_Select_*_sss

        لكل (u32 i = 0; i < 3; ++i) {
            إذا (Is_ډېری_سر_دننه_ثبتوونکی(o)) {
                op_شاخص = i;
            } وإلا {
                slots[i] = Get_ډېری_سر_خونه_شمېره(o);
            }

_           (راوباسه(o));
        }

        // a reference is a pointer-sized integer as far as select is concerned
        u32 type_شاخص = IsRefType(type) ? ((M3_SIZEOF_PTR == 8) ? 1 : 0)
                                        : (u32)(type - c_m3Type_i32);

        op = c_intSelectOps[type_شاخص][op_شاخص];
    } وإلا إذا (not Is_ډېری_Polymorphic(o)) {
        _throw(m3_Err_function_ډېری_Underrun);
    }

    EmitOp(o, op);
    لكل (u32 i = 0; i < 3; i++) {
        إذا (Is_Valid_خونه(slots[i])) {
            Emit_خونه_دننه_واټن(o, slots[i]);
        }
    }
_   (ټېل_ثبتوونکی(o, type));

    _catch: أرجع پایله;
}

ساكن
M3_پایله Compile_Drop (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_پایله پایله = راوباسه(o);
    إذا (d_m3_Log_Wasm_ډېری) {
        dump_type_ډېری(o);
    }
    أرجع پایله;
}

ساكن
M3_پایله Compile_Nop (IM3Compilation o, m3opcode_t i_opcode)
{
    أرجع m3Err_none;
}

ساكن
M3_پایله Compile_Unreachable (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_پایله پایله;

_   (جمع_کړه_Trap_ریکارډ(o));

_   (EmitOp(o, op_Unreachable));
_   (سېټ_ډېری_Polymorphic(o));

    _catch:
    أرجع پایله;
}


// OPTZ: currently all stack slot indices take up a full word, but
// dual stack source operands could be packed together
ساكن
M3_پایله Compile_Operator (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_پایله پایله;

    // Declared before the first throw below, which jumps past this point to
    // _catch and so must not skip an initialization.
    IM3Operation op = عدم;

    IM3OpInfo opInfo = GetOpInfo(i_opcode);
    _throwif(m3Err_unknownOpcode, not opInfo);

    // Spec: validate operand types for load/store operations
    إذا (not Is_ډېری_Polymorphic(o)) {
        // For load ops (stackOffset == 0, unary), the operand is always i32 (address)
        إذا (i_opcode >= 0x28 and i_opcode <= 0x35) {
            m3type_t سر_Type = Get_ډېری_سر_Type(o);
            _throwif(m3Err_typeMismatch, سر_Type != c_m3Type_none and سر_Type != c_m3Type_i32);
        }

        // For store ops (stackOffset == -2), the address operand must be i32
        إذا (i_opcode >= 0x36 and i_opcode <= 0x3e) {
            m3type_t addrType = Get_ډېری_Type_From_سر(o, 1);
            _throwif(m3Err_typeMismatch, addrType != c_m3Type_none and addrType != c_m3Type_i32);
        }
    }

    // This preserve is for for FP compare operations.
    // either need additional slot destination operations or the
    // easy fix, move _r0 out of the way.
    // moving out the way might be the optimal solution most often?
    // otherwise, the _r0 reg can get buried down in the stack
    // and be idle & wasted for a moment.
    إذا (M3_UNLIKELY(IsFpType(Get_ډېری_سر_Type(o)) and IsIntType(opInfo->type))) {
_       (Preserve_ثبتوونکی_If_Occupied(o, opInfo->type));
    }

    إذا (opInfo->ډېری_دننه_واټن == 0) {
        إذا (Is_ډېری_سر_دننه_ثبتوونکی(o)) {
            op = opInfo->operations[0]; // _s
        } وإلا {
_           (Preserve_ثبتوونکی_If_Occupied(o, opInfo->type));
            op = opInfo->operations[1]; // _r
        }
    } وإلا {
        إذا (Is_ډېری_سر_دننه_ثبتوونکی(o)) {
            op = opInfo->operations[0];  // _rs

            إذا (Is_ډېری_سر_Minus1_دننه_ثبتوونکی(o)) {    d_m3Assert (i_opcode == c_waOp_store_f32 or i_opcode == c_waOp_store_f64);
                op = opInfo->operations[3]; // _rr for fp.store
            }
        } وإلا إذا (Is_ډېری_سر_Minus1_دننه_ثبتوونکی(o)) {
            op = opInfo->operations[1]; // _sr

            إذا (not op) {  // must be commutative, then
                op = opInfo->operations[0];
            }
        } وإلا {
_           (Preserve_ثبتوونکی_If_Occupied(o, opInfo->type));     // _ss
            op = opInfo->operations[2];
        }
    }

    إذا (M3_LIKELY(op)) {
_       (EmitOp(o, op));

#إذا d_m3_Fold_سېټ_Local
        // the op word just written (EmitOp may have bridged pages); NULL in the page-less compile-walk mode
        pc_t patchPC = o->پاڼه ? GetPC(o) - 1 : عدم;
#د_که_پای

_       (Emit_خونه_عدد_د_ډېری_سر_And_راوباسه(o));

        إذا (opInfo->ډېری_دننه_واټن < 0) {
_           (Emit_خونه_عدد_د_ډېری_سر_And_راوباسه(o));
        }

        إذا (opInfo->type != c_m3Type_none) {
_           (ټېل_ثبتوونکی(o, opInfo->type));

#إذا d_m3_Fold_سېټ_Local
            // record this op as a fold candidate for an immediately following local.set;
            // the operand-form index is recovered by matching against the variant table
            إذا (patchPC) {
                لكل (u8 form = 0; form < 4; ++form) {
                    إذا (opInfo->operations[form] == op) {
                        o->foldPatchPC    = patchPC;
                        o->foldOpcode     = i_opcode;
                        o->foldForm       = form;
                        o->fold_ډېری_شاخص = (u16)Get_ډېری_سر_شاخص(o);
                        اكسر;
                    }
                }
            }
#د_که_پای
        }
    } وإلا {
#که_تعریف_شوی DEBUG
        پایله = ErrorCompile("no operation found for opcode", o, "'%s'", opInfo->name);
#وإلا
        پایله = ErrorCompile("no operation found for opcode", o, "%x", i_opcode);
#د_که_پای
        _throw(پایله);
    }

    _catch: أرجع پایله;
}

ساكن
M3_پایله Compile_Convert (IM3Compilation o, m3opcode_t i_opcode)
{
_try {
    IM3OpInfo opInfo = GetOpInfo(i_opcode);
    _throwif(m3Err_unknownOpcode, not opInfo);

    // Spec: validate source operand type for conversion instructions
    إذا (not Is_ډېری_Polymorphic(o)) {
        u8 سرچینه_Type = c_m3Type_none;
        بدل (i_opcode)
        // clang-format off
        {
        حالة 0xa7:              // i32.wrap/i64
            سرچینه_Type = c_m3Type_i64; اكسر;
        حالة 0xa8: حالة 0xa9:   // i32.trunc_s/f32, i32.trunc_u/f32
        حالة 0xae: حالة 0xaf:   // i64.trunc_s/f32, i64.trunc_u/f32
        حالة 0xbb:              // f64.promote/f32
        حالة 0xbc:              // i32.reinterpret/f32
            سرچینه_Type = c_m3Type_f32; اكسر;
        حالة 0xaa: حالة 0xab:   // i32.trunc_s/f64, i32.trunc_u/f64
        حالة 0xb0: حالة 0xb1:   // i64.trunc_s/f64, i64.trunc_u/f64
        حالة 0xb6:              // f32.demote/f64
        حالة 0xbd:              // i64.reinterpret/f64
            سرچینه_Type = c_m3Type_f64; اكسر;
        حالة 0xac: حالة 0xad:   // i64.extend_s/i32, i64.extend_u/i32
        حالة 0xb2: حالة 0xb3:   // f32.convert_s/i32, f32.convert_u/i32
        حالة 0xb7: حالة 0xb8:   // f64.convert_s/i32, f64.convert_u/i32
        حالة 0xbe:              // f32.reinterpret/i32
            سرچینه_Type = c_m3Type_i32; اكسر;
        حالة 0xb4: حالة 0xb5:   // f32.convert_s/i64, f32.convert_u/i64
        حالة 0xb9: حالة 0xba:   // f64.convert_s/i64, f64.convert_u/i64
        حالة 0xbf:              // f64.reinterpret/i64
            سرچینه_Type = c_m3Type_i64; اكسر;
        افتراضي: اكسر;
        }
        // clang-format on

        إذا (سرچینه_Type != c_m3Type_none) {
            m3type_t سر_Type = Get_ډېری_سر_Type(o);
            _throwif(m3Err_typeMismatch, سر_Type != c_m3Type_none and not Is_Sub_Type_د(سر_Type, سرچینه_Type));
        }
    }

    bool dest_دننه_خونه   = Is_ثبتوونکی_Type_Allocated(o, opInfo->type);
    bool سرچینه_دننه_خونه = Is_ډېری_سر_دننه_خونه(o);

    IM3Operation op = opInfo->operations[dest_دننه_خونه * 2 + سرچینه_دننه_خونه];

_   (EmitOp(o, op));
_   (Emit_خونه_عدد_د_ډېری_سر_And_راوباسه(o));

    إذا (dest_دننه_خونه) {
_       (ټېل_Allocated_خونه_And_Emit_2(o, opInfo->type))
    } وإلا {
_       (ټېل_ثبتوونکی(o, opInfo->type))
    }
}
    _catch: أرجع پایله;
}

#إذا d_m3HasWideArithmetic

// The wide arithmetic proposal: i64.add128 and i64.sub128 over two 128-bit values
// given as i64 halves, and i64.mul_wide_s/u giving the full product of two i64s.
// Each gives back a 128-bit result the same way, low half first. The operands are
// all read from slots, and of the two results the high half, which ends up on top,
// goes to _r0 and the low half to a newly allocated slot - so every operation has
// the one form, whatever shape the stack was in.
ساكن
M3_پایله Compile_WideArithmetic (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_پایله پایله = m3Err_none;

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

_   (Preserve_ثبتوونکی_If_Occupied(o, c_m3Type_i64));

_   (EmitOp(o, op));

    // the top of the stack first: the high half of the second operand, for the
    // 128-bit ones
    لكل (u32 i = 0; i < عدد_Operands; ++i) {
_       (Emit_خونه_عدد_د_ډېری_سر_And_راوباسه(o));
    }

_   (ټېل_Allocated_خونه_And_Emit_2(o, c_m3Type_i64));      // the low half
_   (ټېل_ثبتوونکی(o, c_m3Type_i64));                  // the high half

    _catch: أرجع پایله;
}

#د_که_پای // d_m3HasWideArithmetic

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
M3_پایله EmitCheckAddr64 (IM3Compilation o, u64 i_دننه_واټن, u16 i_عدد_Operands)
{
    M3_پایله پایله = m3Err_none;

    // The address has to be in a slot for op_CheckAddr64 to read it. Spilling
    // r0 wholesale also keeps the value of a store out of it, which is what
    // leaves the address as the one operand still in a register to worry about.
_   (Preserve_ثبتوونکی_If_Occupied(o, c_m3Type_i64));

    // unreachable code: the operands the instruction names may not be there
    إذا (o->ډېری_شاخص < o->بلاک.بلاک_ډېری_شاخص + i_عدد_Operands) {
        أرجع پایله;
    }

    {
        u16      ډېری_شاخص = (u16)(o->ډېری_شاخص - i_عدد_Operands);
        u16      src_خونه    = o->wasm_ډېری[ډېری_شاخص];
        m3type_t srcType    = o->type_ډېری[ډېری_شاخص];

        u16 dst_خونه = c_خونه_ناکارول_شوی;

        // Checked here rather than left to the validator, which a build may
        // have switched off: reading a slot as an i64 that only holds an i32
        // would reach past what was allocated for it.
        _throwif(m3Err_typeMismatch, بنسټ_Type_د(srcType) != c_m3Type_i64);

        // an address is an integer, so nothing should be left in a register
        // once r0 has been preserved
        _throwif(m3Err_typeMismatch, Is_ثبتوونکی_خونه_Alias(src_خونه));
        _throwif(m3_Err_function_ډېری_Underrun, src_خونه >= o->خونه_Max_Allocated_شاخص_Plus_One);

        // allocated before the source is released, so the two cannot be the
        // same slot - op_CheckAddr64 reads one and writes the other
_       (AllocateSlots(o, &dst_خونه, c_m3Type_i32));

_       (EmitOp(o, op_CheckAddr64));
        Emit_خونه_دننه_واټن(o, src_خونه);
        Emit_خونه_دننه_واټن(o, dst_خونه);
        EmitConstant64(o, i_دننه_واټن);

        o->wasm_ډېری[ډېری_شاخص] = dst_خونه;
        o->type_ډېری[ډېری_شاخص] = c_m3Type_i32;

        // locals and the constant table outlive the instruction; a dynamic
        // slot the address was computed into does not
        إذا (src_خونه >= o->خونه_لومړی_Dynamic_شاخص) {
            Deallocate_خونه(o, src_خونه, srcType);
        }
    }

    _catch: أرجع پایله;
}

#د_که_پای // d_m3HasMemory64


ساكن
M3_پایله Compile_بارول_Store (IM3Compilation o, m3opcode_t i_opcode)
{
_try {
    u32 alignHint, حافظه_Idx;
    u64 حافظه_دننه_واټن;

    // alignHint is checked by the validator
_   (Read_حافظه_Arg(&alignHint, &حافظه_Idx, &حافظه_دننه_واټن, &o->wasm, o->wasm_پای_ټکی));
                                                                        m3log (compile, d_indent " (memory = %d; offset = %llu)", get_indention_string (o), حافظه_Idx, (بینښه اوږد اوږد) حافظه_دننه_واټن);
    _throwif(m3_Err_unknown_حافظه, حافظه_Idx >= o->module->عدد_Memories);

    IM3OpInfo opInfo = GetOpInfo(i_opcode);
    _throwif(m3Err_unknownOpcode, not opInfo);

    bool isMemory64 = o->module->memories[حافظه_Idx]->isMemory64;

    // Spec: the static offset has to be in range of the address type. Checked
    // here as well as in the validator, so that a build without one still
    // refuses the module rather than silently truncating the offset.
    _throwif(m3Err_wasmMalformed, not isMemory64 and حافظه_دننه_واټن > 0xFFFFFFFFull);

    إذا (IsFpType(opInfo->type)) {
_       (Preserve_ثبتوونکی_If_Occupied(o, c_m3Type_f64));
    }

    إذا (حافظه_Idx) {
_       (Emit_سېټ_حافظه(o, حافظه_Idx));
    }

#إذا d_m3HasMemory64
    إذا (isMemory64) {
        // An offset this far out puts every address out of bounds, so rather
        // than carrying it, clamp it to the limit: op_CheckAddr64 then traps
        // on whatever address it is given, which is the outcome either way.
        إذا (حافظه_دننه_واټن > d_m3_پته_حد) {
            حافظه_دننه_واټن = d_m3_پته_حد;
        }

_       (EmitCheckAddr64(o, حافظه_دننه_واټن, opInfo->ډېری_دننه_واټن < 0 ? 2 : 1));

        // op_CheckAddr64 has folded the offset in already
        حافظه_دننه_واټن = 0;
    }
#د_که_پای

_   (Compile_Operator(o, i_opcode));

    EmitConstant32(o, (u32)حافظه_دننه_واټن);

    إذا (حافظه_Idx) {
_       (Emit_سېټ_حافظه(o, 0));
    }
}
    _catch: أرجع پایله;
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
فراغ GetAtomicShape (u32 i_sub, u16* o_عدد_Operands, m3type_t* o_سر_Type, m3type_t* o_پایله_Type)
{
    // clang-format off
    ساكن ثابت m3type_t c_ارزښت_Types [7] = {
        c_m3Type_i32, c_m3Type_i64,
        c_m3Type_i32, c_m3Type_i32,
        c_m3Type_i64, c_m3Type_i64, c_m3Type_i64
    };
    // clang-format on

    إذا (i_sub == 0x00) {                    // memory.atomic.notify: address, count
        *o_عدد_Operands = 2;
        *o_سر_Type     = c_m3Type_i32;
        *o_پایله_Type  = c_m3Type_i32;
    } وإلا إذا (i_sub <= 0x02) {             // memory.atomic.wait32/64: address, expected, timeout
        *o_عدد_Operands = 3;
        *o_سر_Type     = c_m3Type_i64;
        *o_پایله_Type  = c_m3Type_i32;
    } وإلا إذا (i_sub <= 0x16) {             // loads: address
        *o_عدد_Operands = 1;
        *o_سر_Type     = c_m3Type_i32;
        *o_پایله_Type  = c_ارزښت_Types[i_sub - 0x10];
    } وإلا إذا (i_sub <= 0x1d) {             // stores: address, value
        *o_عدد_Operands = 2;
        *o_سر_Type     = c_ارزښت_Types[i_sub - 0x17];
        *o_پایله_Type  = c_m3Type_none;
    } وإلا إذا (i_sub <= 0x47) {             // read-modify-write: address, value
        *o_عدد_Operands = 2;
        *o_سر_Type     = c_ارزښت_Types[(i_sub - 0x1e) % 7];
        *o_پایله_Type  = *o_سر_Type;
    } وإلا {                                // cmpxchg: address, expected, replacement
        *o_عدد_Operands = 3;
        *o_سر_Type     = c_ارزښت_Types[i_sub - 0x48];
        *o_پایله_Type  = *o_سر_Type;
    }
}


// The description an access takes as its last immediate: see m3_exec_atomic.h
ساكن
u32 GetAtomicDesc (u32 i_sub)
{
    // per variant, the log2 of the width of the cell
    ساكن ثابت u8 c_log2Widths[7] = { 2, 3, 0, 1, 0, 1, 2 };

    u32 variant, ډول = 0;

    إذا (i_sub <= 0x16) {
        variant = i_sub - 0x10;
    } وإلا إذا (i_sub <= 0x1d) {
        variant = i_sub - 0x17;
    } وإلا إذا (i_sub <= 0x47) {
        variant = (i_sub - 0x1e) % 7;
        ډول    = (i_sub - 0x1e) / 7;         // add, sub, and, or, xor, xchg
    } وإلا {
        variant = i_sub - 0x48;
    }

    // variants 1 and 4..6 are the i64 ones
    u32 is64 = (variant == 1 or variant >= 4) ? 1u : 0u;

    أرجع c_log2Widths[variant] | (is64 << 2) | (ډول << 3);
}


// Every atomic access is one op, whose operands are the topmost in _r0 and the rest
// in slots, topmost first. The address is always the deepest, so it is the last slot
// (or, for a load, the register), followed by the memarg offset. The ops check the
// address themselves: for an atomic that includes its alignment, which only the
// effective address can be asked.
ساكن
M3_پایله Compile_Atomic (IM3Compilation o, m3opcode_t i_opcode)
{
_try {
    u32 alignHint, حافظه_Idx;
    u64 حافظه_دننه_واټن;

    u16      عدد_Operands = 0;
    m3type_t سر_Type     = c_m3Type_none;
    m3type_t پایله_Type  = c_m3Type_none;

    إذا (i_opcode == c_waOp_atomicFence) {
        // the reserved byte, which the validator has checked to be zero. There
        // is nothing to order between accesses that one thread makes in sequence.
        u8 reserved;
_       (Read_u8(&reserved, &o->wasm, o->wasm_پای_ټکی));
    } وإلا {
        // alignHint is checked by the validator
_       (Read_حافظه_Arg(&alignHint, &حافظه_Idx, &حافظه_دننه_واټن, &o->wasm, o->wasm_پای_ټکی));
                                                                        m3log (compile, d_indent " (memory = %d; offset = %llu)", get_indention_string (o), حافظه_Idx, (بینښه اوږد اوږد) حافظه_دننه_واټن);
        _throwif(m3_Err_unknown_حافظه, حافظه_Idx >= o->module->عدد_Memories);

        IM3OpInfo opInfo = GetOpInfo(i_opcode);
        _throwif(m3Err_unknownOpcode, not opInfo);

        bool isMemory64 = o->module->memories[حافظه_Idx]->isMemory64;

        // see Compile_Load_Store
        _throwif(m3Err_wasmMalformed, not isMemory64 and حافظه_دننه_واټن > 0xFFFFFFFFull);

        GetAtomicShape(i_opcode & 0xFF, &عدد_Operands, &سر_Type, &پایله_Type);

        إذا (حافظه_Idx) {
_           (Emit_سېټ_حافظه(o, حافظه_Idx));
        }

#  إذا d_m3HasMemory64
        إذا (isMemory64) {
            // see Compile_Load_Store
            إذا (حافظه_دننه_واټن > d_m3_پته_حد) {
                حافظه_دننه_واټن = d_m3_پته_حد;
            }

_           (EmitCheckAddr64(o, حافظه_دننه_واټن, عدد_Operands));
            حافظه_دننه_واټن = 0;
        }
#  endif

_       (Copy_ډېری_سر_منزل_ثبتوونکی(o, false));
_       (EmitOp(o, opInfo->operations[0]));
_       (راوباسه_Type(o, سر_Type));

        لكل (u16 i = 1; i < عدد_Operands; ++i) {
_           (Emit_خونه_عدد_د_ډېری_سر_And_راوباسه(o));
        }

        EmitConstant32(o, (u32)حافظه_دننه_واټن);
        إذا (i_opcode >= c_wa_Op_atomic_لومړی_Access) {
            EmitConstant32(o, GetAtomicDesc(i_opcode & 0xFF));
        }

        إذا (حافظه_Idx) {
_           (Emit_سېټ_حافظه(o, 0));
        }

        إذا (پایله_Type != c_m3Type_none) {
_           (ټېل_ثبتوونکی(o, پایله_Type));
        }
    }
}
    _catch: أرجع پایله;
}

#د_که_پای // d_m3HasAtomics


M3_پایله CompileRawFunction (IM3Module io_module, IM3Function io_function, ثابت فراغ* i_function, ثابت فراغ* i_userdata)
{
    d_m3Assert(io_module->runtime);

    IM3_کوډ_پاڼه پاڼه = Acquire_کوډ_پاڼه_With_ظرفیت(io_module->runtime, 4);

    إذا (پاڼه) {
        io_function->compiled = Get_پاڼه_PC(پاڼه);
        io_function->module   = io_module;

        // Unless a host module's ABI names a memory (m3_BindImportMemory), a
        // host function addresses memory 0 - what a bare i32 guest pointer means
        io_function->host_حافظه = io_module->memory0;

        Emit_کلمه(پاڼه, op_CallRawFunction);
        Emit_کلمه(پاڼه, i_function);
        Emit_کلمه(پاڼه, io_function);
        Emit_کلمه(پاڼه, i_userdata);

        Release_کوډ_پاڼه(io_module->runtime, پاڼه);
        أرجع m3Err_none;
    } وإلا {
        أرجع m3_Err_malloc_Failed_کوډ_پاڼه;
    }
}


// A table entry names every field it sets, so the struct can order them for
// packing; the columns follow that order, since M3OP names them positionally.
//
// d_logOp, d_logOp2 macros aren't actually used by the compiler, just codepage decoding (d_m3LogCodePages = 1)
#تعریف_کړه d_ops(...)                          .operations = { __VA_ARGS__ }

#تعریف_کړه d_logOp(OP)                         d_ops( op_##OP,                 عدم,                       عدم,                       عدم )
#تعریف_کړه d_logOp2(OP1,OP2)                   d_ops( op_##OP1,                op_##OP2,                   عدم,                       عدم )

#تعریف_کړه d_تشه_Op_List                       d_ops( عدم,                    عدم,                       عدم,                       عدم )
#تعریف_کړه d_unaryOpList(TYPE, NAME)           d_ops( op_##TYPE##_##NAME##_r,  op_##TYPE##_##NAME##_s,     عدم,                       عدم )
#تعریف_کړه d_binOpList(TYPE, NAME)             d_ops( op_##TYPE##_##NAME##_rs, op_##TYPE##_##NAME##_sr,    op_##TYPE##_##NAME##_ss,    عدم )
#تعریف_کړه d_storeFpOpList(TYPE, NAME)         d_ops( op_##TYPE##_##NAME##_rs, op_##TYPE##_##NAME##_sr,    op_##TYPE##_##NAME##_ss,    op_##TYPE##_##NAME##_rr )
#تعریف_کړه d_commutativeBinOpList(TYPE, NAME)  d_ops( op_##TYPE##_##NAME##_rs, عدم,                       op_##TYPE##_##NAME##_ss,    عدم )

#تعریف_کړه d_convertOpList(OP)                 d_ops( op_##OP##_r_r,           op_##OP##_r_s,              op_##OP##_s_r,              op_##OP##_s_s )


// Every M3Compiler the tables below name, as one list, so that the index enum and
// the array of pointers can never drift apart. Each entry must carry the same
// config guard as the function's definition.
#تعریف_کړه d_m3CompilerList(_)             \
    _( Compile_Unreachable )            \
    _( Compile_Nop )                    \
    _( Compile_Loop_Or_بلاک )            \
    _( Compile_If )                     \
    _( Compile_پای_ټکی )                    \
    _( Compile_Branch )                 \
    _( Compile_Branch_جدول )            \
    _( Compile_Return )                 \
    _( Compile_Call )                   \
    _( Compile_Call_غیرمستقیم )           \
    _( Compile_Drop )                   \
    _( Compile_Select )                 \
    _( Compile_GetLocal )               \
    _( Compile_سېټ_Local )               \
    _( Compile_Get_سېټ_Global )           \
    _( Compile_بارول_Store )             \
    _( Compile_حافظه_اندازه )            \
    _( Compile_حافظه_Grow )            \
    _( Compile_حافظه_Init )            \
    _( Compile_حافظه_Copy_Fill )        \
    _( Compile_معلومات_Drop )              \
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
    d_m3_Compiler_List_ډېری_Switching( _ )

#إذا d_m3ImplementFloat
#  define d_m3CompilerList_f(_)         \
    _( Compile_Const_f32 )              \
    _( Compile_Const_f64 )
#وإلا
#  define d_m3CompilerList_f(_)
#د_که_پای

#إذا d_m3HasRefTypes
#  define d_m3CompilerList_refTypes(_)  \
    _( Compile_Select_Typed )           \
    _( Compile_Ref_Null )               \
    _( Compile_Ref_IsNull )             \
    _( Compile_Ref_Func )               \
    _( Compile_جدول_Get_سېټ )           \
    _( Compile_جدول_Init )             \
    _( Compile_جدول_Copy )             \
    _( Compile_جدول_اندازه )             \
    _( Compile_جدول_Grow_Fill )         \
    _( Compile_Elem_Drop )
#وإلا
#  define d_m3CompilerList_refTypes(_)
#د_که_پای

#إذا d_m3HasTypedRefs
#  define d_m3CompilerList_typedRefs(_) \
    _( Compile_CallRef )                \
    _( Compile_Ref_AsNonNull )
#وإلا
#  define d_m3CompilerList_typedRefs(_)
#د_که_پای

#إذا d_m3HasExceptionHandling
#  define d_m3CompilerList_eh(_)        \
    _( Compile_Throw )                  \
    _( Compile_ThrowRef )               \
    _( Compile_Try_جدول )
#وإلا
#  define d_m3CompilerList_eh(_)
#د_که_پای

#إذا d_m3HasAtomics
#  define d_m3CompilerList_atomics(_)   \
    _( Compile_Atomic )
#وإلا
#  define d_m3CompilerList_atomics(_)
#د_که_پای

#إذا d_m3HasWideArithmetic
#  define d_m3CompilerList_wide(_)      \
    _( Compile_WideArithmetic )
#وإلا
#  define d_m3CompilerList_wide(_)
#د_که_پای

#إذا d_m3_Has_ډېری_Switching
#  define d_m3_Compiler_List_ډېری_Switching(_) \
    _( Compile_ContNew )                     \
    _( Compile_ContBind )                    \
    _( Compile_Suspend )                     \
    _( Compile_Resume )                      \
    _( Compile_Switch )
#وإلا
#  define d_m3_Compiler_List_ډېری_Switching(_)
#د_که_پای

// Index 0 is "this opcode has no compiler", so that a zeroed entry - which is
// what M3OP_RESERVED and the reserved holes in the tables are - reads as one.
تعداد {
    c_cc_NULL = 0,
#تعریف_کړه d_m3CompilerEnum(NAME)  c_cc_##NAME,
    d_m3CompilerList(d_m3CompilerEnum)
#تعریف_لغوه d_m3CompilerEnum
};

ساكن ثابت M3Compiler c_compilers[] = {
    عدم,
#تعریف_کړه d_m3CompilerPtr(NAME)   NAME,
    d_m3CompilerList(d_m3CompilerPtr)
#تعریف_لغوه d_m3CompilerPtr
};

// Names a table entry's compiler by index. d_cc (NULL) is the no-compiler entry.
#تعریف_کړه d_cc(NAME)                          .compiler = c_cc_##NAME

ساكن
M3Compiler GetCompiler (IM3OpInfo i_opInfo)
{
    d_m3Assert(i_opInfo->compiler < M3_شمېر_د(c_compilers));
    أرجع c_compilers[i_opInfo->compiler];
}

// clang-format off
ثابت M3OpInfo c_operations[] =
{
    M3OP( "unreachable",           0,  none,   d_cc(Compile_Unreachable),  d_logOp (Unreachable) ),                   // 0x00
    M3OP( "nop",                   0,  none,   d_cc(Compile_Nop),          d_تشه_Op_List ),                           // 0x01 .
    M3OP( "block",                 0,  none,   d_cc(Compile_Loop_Or_بلاک),  d_تشه_Op_List ),                           // 0x02
    M3OP( "loop",                  0,  none,   d_cc(Compile_Loop_Or_بلاک),  d_logOp (Loop) ),                          // 0x03
    M3OP( "if",                    -1, none,   d_cc(Compile_If),           d_تشه_Op_List ),                           // 0x04
    M3OP( "else",                  0,  none,   d_cc(Compile_Nop),          d_تشه_Op_List ),                           // 0x05

    M3OP_RESERVED,  M3OP_RESERVED,                                                                                    // 0x06...0x07

#إذا d_m3HasExceptionHandling
    M3OP( "throw",                 0,  none,   d_cc(Compile_Throw),        d_logOp (Throw) ),                         // 0x08
    M3OP_RESERVED,                                                                                                    // 0x09
    M3OP( "throw_ref",             0,  none,   d_cc(Compile_ThrowRef),     d_logOp (ThrowRef) ),                      // 0x0a
#وإلا
    M3OP_RESERVED,  M3OP_RESERVED, M3OP_RESERVED,                                                                     // 0x08...0x0a
#د_که_پای

    M3OP( "end",                   0,  none,   d_cc(Compile_پای_ټکی),          d_تشه_Op_List ),                           // 0x0b
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
#د_که_پای
    M3OP_RESERVED,  M3OP_RESERVED, M3OP_RESERVED, M3OP_RESERVED,                                                      // ...0x19

    M3OP( "drop",                  -1, none,   d_cc(Compile_Drop),         d_تشه_Op_List ),                           // 0x1a
    M3OP( "select",                -2, any,    d_cc(Compile_Select),       d_تشه_Op_List ),                           // 0x1b

#إذا d_m3HasRefTypes
    M3OP( "select.t",              -2, any,    d_cc(Compile_Select_Typed), d_تشه_Op_List ),                           // 0x1c
#وإلا
    M3OP_RESERVED,                                                                                                    // 0x1c
#د_که_پای
    M3OP_RESERVED,  M3OP_RESERVED,                                                                                    // 0x1d...0x1e

#إذا d_m3HasExceptionHandling
    M3OP( "try_table",             0,  none,   d_cc(Compile_Try_جدول),     d_logOp (Try_جدول) ),                      // 0x1f
#وإلا
    M3OP_RESERVED,                                                                                                    // 0x1f
#د_که_پای

    M3OP( "local.get",             1,  any,    d_cc(Compile_GetLocal),     d_تشه_Op_List ),                           // 0x20
    M3OP( "local.set",             1,  none,   d_cc(Compile_سېټ_Local),     d_تشه_Op_List ),                           // 0x21
    M3OP( "local.tee",             0,  any,    d_cc(Compile_سېټ_Local),     d_تشه_Op_List ),                           // 0x22
    M3OP( "global.get",            1,  none,   d_cc(Compile_Get_سېټ_Global), d_تشه_Op_List ),                           // 0x23
    M3OP( "global.set",            1,  none,   d_cc(Compile_Get_سېټ_Global), d_تشه_Op_List ),                           // 0x24

#إذا d_m3HasRefTypes
    M3OP( "table.get",             0,  any,    d_cc(Compile_جدول_Get_سېټ), d_تشه_Op_List ),                           // 0x25
    M3OP( "table.set",             -2, none,   d_cc(Compile_جدول_Get_سېټ), d_تشه_Op_List ),                           // 0x26
#وإلا
    M3OP_RESERVED,  M3OP_RESERVED,                                                                                    // 0x25...0x26
#د_که_پای
    M3OP_RESERVED,                                                                                                    // 0x27

    M3OP( "i32.load",              0,  i_32,   d_cc(Compile_بارول_Store),   d_unaryOpList (i32, بارول_i32) ),           // 0x28
    M3OP( "i64.load",              0,  i_64,   d_cc(Compile_بارول_Store),   d_unaryOpList (i64, بارول_i64) ),           // 0x29
    M3OP_F( "f32.load",            0,  f_32,   d_cc(Compile_بارول_Store),   d_unaryOpList (f32, بارول_f32) ),           // 0x2a
    M3OP_F( "f64.load",            0,  f_64,   d_cc(Compile_بارول_Store),   d_unaryOpList (f64, بارول_f64) ),           // 0x2b

    M3OP( "i32.load8_s",           0,  i_32,   d_cc(Compile_بارول_Store),   d_unaryOpList (i32, بارول_i8) ),            // 0x2c
    M3OP( "i32.load8_u",           0,  i_32,   d_cc(Compile_بارول_Store),   d_unaryOpList (i32, بارول_u8) ),            // 0x2d
    M3OP( "i32.load16_s",          0,  i_32,   d_cc(Compile_بارول_Store),   d_unaryOpList (i32, بارول_i16) ),           // 0x2e
    M3OP( "i32.load16_u",          0,  i_32,   d_cc(Compile_بارول_Store),   d_unaryOpList (i32, بارول_u16) ),           // 0x2f

    M3OP( "i64.load8_s",           0,  i_64,   d_cc(Compile_بارول_Store),   d_unaryOpList (i64, بارول_i8) ),            // 0x30
    M3OP( "i64.load8_u",           0,  i_64,   d_cc(Compile_بارول_Store),   d_unaryOpList (i64, بارول_u8) ),            // 0x31
    M3OP( "i64.load16_s",          0,  i_64,   d_cc(Compile_بارول_Store),   d_unaryOpList (i64, بارول_i16) ),           // 0x32
    M3OP( "i64.load16_u",          0,  i_64,   d_cc(Compile_بارول_Store),   d_unaryOpList (i64, بارول_u16) ),           // 0x33
    M3OP( "i64.load32_s",          0,  i_64,   d_cc(Compile_بارول_Store),   d_unaryOpList (i64, بارول_i32) ),           // 0x34
    M3OP( "i64.load32_u",          0,  i_64,   d_cc(Compile_بارول_Store),   d_unaryOpList (i64, بارول_u32) ),           // 0x35

    M3OP( "i32.store",             -2, none,   d_cc(Compile_بارول_Store),   d_binOpList (i32, Store_i32) ),            // 0x36
    M3OP( "i64.store",             -2, none,   d_cc(Compile_بارول_Store),   d_binOpList (i64, Store_i64) ),            // 0x37
    M3OP_F( "f32.store",           -2, none,   d_cc(Compile_بارول_Store),   d_storeFpOpList (f32, Store_f32) ),        // 0x38
    M3OP_F( "f64.store",           -2, none,   d_cc(Compile_بارول_Store),   d_storeFpOpList (f64, Store_f64) ),        // 0x39

    M3OP( "i32.store8",            -2, none,   d_cc(Compile_بارول_Store),   d_binOpList (i32, Store_u8) ),             // 0x3a
    M3OP( "i32.store16",           -2, none,   d_cc(Compile_بارول_Store),   d_binOpList (i32, Store_i16) ),            // 0x3b

    M3OP( "i64.store8",            -2, none,   d_cc(Compile_بارول_Store),   d_binOpList (i64, Store_u8) ),             // 0x3c
    M3OP( "i64.store16",           -2, none,   d_cc(Compile_بارول_Store),   d_binOpList (i64, Store_i16) ),            // 0x3d
    M3OP( "i64.store32",           -2, none,   d_cc(Compile_بارول_Store),   d_binOpList (i64, Store_i32) ),            // 0x3e

    M3OP( "memory.size",           1,  i_32,   d_cc(Compile_حافظه_اندازه),  d_logOp (Mem_اندازه) ),                       // 0x3f
    M3OP( "memory.grow",           1,  i_32,   d_cc(Compile_حافظه_Grow),  d_logOp (MemGrow) ),                       // 0x40

    M3OP( "i32.const",             1,  i_32,   d_cc(Compile_Const_i32),    d_logOp (Const32) ),                       // 0x41
    M3OP( "i64.const",             1,  i_64,   d_cc(Compile_Const_i64),    d_logOp (Const64) ),                       // 0x42
    M3OP_F( "f32.const",           1,  f_32,   d_cc(Compile_Const_f32),    d_تشه_Op_List ),                           // 0x43
    M3OP_F( "f64.const",           1,  f_64,   d_cc(Compile_Const_f64),    d_تشه_Op_List ),                           // 0x44

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

    M3OP( "i32.add",               -1, i_32,   d_cc(عدم),                 d_commutativeBinOpList (i32, جمع_کړه) ),       // 0x6a
    M3OP( "i32.sub",               -1, i_32,   d_cc(عدم),                 d_binOpList (i32, کمول) ),             // 0x6b
    M3OP( "i32.mul",               -1, i_32,   d_cc(عدم),                 d_commutativeBinOpList (i32, Multiply) ),  // 0x6c
    M3OP( "i32.div_s",             -1, i_32,   d_cc(عدم),                 d_binOpList (i32, Divide) ),               // 0x6d
    M3OP( "i32.div_u",             -1, i_32,   d_cc(عدم),                 d_binOpList (u32, Divide) ),               // 0x6e
    M3OP( "i32.rem_s",             -1, i_32,   d_cc(عدم),                 d_binOpList (i32, Remainder) ),            // 0x6f
    M3OP( "i32.rem_u",             -1, i_32,   d_cc(عدم),                 d_binOpList (u32, Remainder) ),            // 0x70
    M3OP( "i32.and",               -1, i_32,   d_cc(عدم),                 d_commutativeBinOpList (u32, And) ),       // 0x71
    M3OP( "i32.or",                -1, i_32,   d_cc(عدم),                 d_commutativeBinOpList (u32, Or) ),        // 0x72
    M3OP( "i32.xor",               -1, i_32,   d_cc(عدم),                 d_commutativeBinOpList (u32, Xor) ),       // 0x73
    M3OP( "i32.shl",               -1, i_32,   d_cc(عدم),                 d_binOpList (u32, ښويدنه_کیڼ) ),            // 0x74
    M3OP( "i32.shr_s",             -1, i_32,   d_cc(عدم),                 d_binOpList (i32, ښويدنه_ښی) ),           // 0x75
    M3OP( "i32.shr_u",             -1, i_32,   d_cc(عدم),                 d_binOpList (u32, ښويدنه_ښی) ),           // 0x76
    M3OP( "i32.rotl",              -1, i_32,   d_cc(عدم),                 d_binOpList (u32, Rotl) ),                 // 0x77
    M3OP( "i32.rotr",              -1, i_32,   d_cc(عدم),                 d_binOpList (u32, Rotr) ),                 // 0x78

    M3OP( "i64.clz",               0,  i_64,   d_cc(عدم),                 d_unaryOpList (u64, Clz) ),                // 0x79
    M3OP( "i64.ctz",               0,  i_64,   d_cc(عدم),                 d_unaryOpList (u64, Ctz) ),                // 0x7a
    M3OP( "i64.popcnt",            0,  i_64,   d_cc(عدم),                 d_unaryOpList (u64, Popcnt) ),             // 0x7b

    M3OP( "i64.add",               -1, i_64,   d_cc(عدم),                 d_commutativeBinOpList (i64, جمع_کړه) ),       // 0x7c
    M3OP( "i64.sub",               -1, i_64,   d_cc(عدم),                 d_binOpList (i64, کمول) ),             // 0x7d
    M3OP( "i64.mul",               -1, i_64,   d_cc(عدم),                 d_commutativeBinOpList (i64, Multiply) ),  // 0x7e
    M3OP( "i64.div_s",             -1, i_64,   d_cc(عدم),                 d_binOpList (i64, Divide) ),               // 0x7f
    M3OP( "i64.div_u",             -1, i_64,   d_cc(عدم),                 d_binOpList (u64, Divide) ),               // 0x80
    M3OP( "i64.rem_s",             -1, i_64,   d_cc(عدم),                 d_binOpList (i64, Remainder) ),            // 0x81
    M3OP( "i64.rem_u",             -1, i_64,   d_cc(عدم),                 d_binOpList (u64, Remainder) ),            // 0x82
    M3OP( "i64.and",               -1, i_64,   d_cc(عدم),                 d_commutativeBinOpList (u64, And) ),       // 0x83
    M3OP( "i64.or",                -1, i_64,   d_cc(عدم),                 d_commutativeBinOpList (u64, Or) ),        // 0x84
    M3OP( "i64.xor",               -1, i_64,   d_cc(عدم),                 d_commutativeBinOpList (u64, Xor) ),       // 0x85
    M3OP( "i64.shl",               -1, i_64,   d_cc(عدم),                 d_binOpList (u64, ښويدنه_کیڼ) ),            // 0x86
    M3OP( "i64.shr_s",             -1, i_64,   d_cc(عدم),                 d_binOpList (i64, ښويدنه_ښی) ),           // 0x87
    M3OP( "i64.shr_u",             -1, i_64,   d_cc(عدم),                 d_binOpList (u64, ښويدنه_ښی) ),           // 0x88
    M3OP( "i64.rotl",              -1, i_64,   d_cc(عدم),                 d_binOpList (u64, Rotl) ),                 // 0x89
    M3OP( "i64.rotr",              -1, i_64,   d_cc(عدم),                 d_binOpList (u64, Rotr) ),                 // 0x8a

    M3OP_F( "f32.abs",             0,  f_32,   d_cc(عدم),                 d_unaryOpList(f32, Abs) ),                 // 0x8b
    M3OP_F( "f32.neg",             0,  f_32,   d_cc(عدم),                 d_unaryOpList(f32, Negate) ),              // 0x8c
    M3OP_F( "f32.ceil",            0,  f_32,   d_cc(عدم),                 d_unaryOpList(f32, Ceil) ),                // 0x8d
    M3OP_F( "f32.floor",           0,  f_32,   d_cc(عدم),                 d_unaryOpList(f32, Floor) ),               // 0x8e
    M3OP_F( "f32.trunc",           0,  f_32,   d_cc(عدم),                 d_unaryOpList(f32, Trunc) ),               // 0x8f
    M3OP_F( "f32.nearest",         0,  f_32,   d_cc(عدم),                 d_unaryOpList(f32, تر_ټولو_نږدې) ),             // 0x90
    M3OP_F( "f32.sqrt",            0,  f_32,   d_cc(عدم),                 d_unaryOpList(f32, Sqrt) ),                // 0x91

    M3OP_F( "f32.add",             -1, f_32,   d_cc(عدم),                 d_commutativeBinOpList (f32, جمع_کړه) ),       // 0x92
    M3OP_F( "f32.sub",             -1, f_32,   d_cc(عدم),                 d_binOpList (f32, کمول) ),             // 0x93
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
    M3OP_F( "f64.nearest",         0,  f_64,   d_cc(عدم),                 d_unaryOpList(f64, تر_ټولو_نږدې) ),             // 0x9e
    M3OP_F( "f64.sqrt",            0,  f_64,   d_cc(عدم),                 d_unaryOpList(f64, Sqrt) ),                // 0x9f

    M3OP_F( "f64.add",             -1, f_64,   d_cc(عدم),                 d_commutativeBinOpList (f64, جمع_کړه) ),       // 0xa0
    M3OP_F( "f64.sub",             -1, f_64,   d_cc(عدم),                 d_binOpList (f64, کمول) ),             // 0xa1
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
#تعریف_کړه d_m3DebugOp(OP) M3OP (#OP, 0, none, d_ops (op_##OP))

# إذا d_m3HasFloat
#تعریف_کړه d_m3DebugTypedOp(OP) M3OP (#OP, 0, none, d_ops (op_##OP##_i32, op_##OP##_i64, op_##OP##_f32, op_##OP##_f64))
# وإلا
#تعریف_کړه d_m3DebugTypedOp(OP) M3OP (#OP, 0, none, d_ops (op_##OP##_i32, op_##OP##_i64))
# endif

    d_m3DebugOp (Compile),          d_m3DebugOp (ثبت_2),            d_m3DebugOp (پای_ټکی_2),
    d_m3DebugOp (Unsupported),      d_m3DebugOp (CallRawFunction),

    d_m3DebugOp (GetGlobal_s32),    d_m3DebugOp (GetGlobal_s64),    d_m3DebugOp (ContinueLoop),     d_m3DebugOp (ContinueLoopIf),
# إذا d_m3_Has_ډېری_Switching
    d_m3DebugOp (ContinueLoop_Suspendable), d_m3DebugOp (ContinueLoopIf_Suspendable), d_m3DebugOp (ثبت_Check),
#  إذا !d_m3_ثبت_Keeps_چوکاټ
    d_m3DebugOp (ثبت_Suspendable),
#  endif
# endif

    d_m3DebugOp (Copy_خونه_32),      d_m3DebugOp (Preserve_Copy_خونه_32), d_m3DebugOp (If_s),          d_m3DebugOp (BranchIfPrologue_s),
    d_m3DebugOp (Copy_خونه_64),      d_m3DebugOp (Preserve_Copy_خونه_64), d_m3DebugOp (If_r),          d_m3DebugOp (BranchIfPrologue_r),

    d_m3DebugOp (Select_i32_rss),   d_m3DebugOp (Select_i32_srs),   d_m3DebugOp (Select_i32_ssr),   d_m3DebugOp (Select_i32_sss),
    d_m3DebugOp (Select_i64_rss),   d_m3DebugOp (Select_i64_srs),   d_m3DebugOp (Select_i64_ssr),   d_m3DebugOp (Select_i64_sss),

# إذا d_m3HasFloat
    d_m3DebugOp (Select_f32_sss),   d_m3DebugOp (Select_f32_srs),   d_m3DebugOp (Select_f32_ssr),
    d_m3DebugOp (Select_f32_rss),   d_m3DebugOp (Select_f32_rrs),   d_m3DebugOp (Select_f32_rsr),

    d_m3DebugOp (Select_f64_sss),   d_m3DebugOp (Select_f64_srs),   d_m3DebugOp (Select_f64_ssr),
    d_m3DebugOp (Select_f64_rss),   d_m3DebugOp (Select_f64_rrs),   d_m3DebugOp (Select_f64_rsr),
# endif

    d_m3DebugOp (MemFill),          d_m3DebugOp (MemCopy),          d_m3DebugOp (MemInit),          d_m3DebugOp (معلومات_Drop),

# إذا d_m3HasGasMetering
    d_m3DebugOp (UseGas),
# endif

# إذا d_m3HasRefTypes
    d_m3DebugOp (جدول_Get),         d_m3DebugOp (جدول_سېټ),         d_m3DebugOp (جدول_اندازه),
    d_m3DebugOp (جدول_Grow),        d_m3DebugOp (جدول_Fill),        d_m3DebugOp (جدول_Init),
    d_m3DebugOp (ElemDrop),         d_m3DebugOp (جدول_Copy),
# endif

    d_m3DebugTypedOp (سېټ_Global),   d_m3DebugOp (سېټ_Global_s32),    d_m3DebugOp (سېټ_Global_s64),

    d_m3DebugTypedOp (سېټ_ثبتوونکی), d_m3DebugTypedOp (سېټ_خونه),     d_m3DebugTypedOp (Preserve_سېټ_خونه),
# endif

# ifdef DEBUG
    M3OP( "termination", 0, c_m3Type_unknown, d_تشه_Op_List ) // for find_operation_info
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

    M3OP( "memory.init",           0, none,   d_cc(Compile_حافظه_Init),     d_تشه_Op_List ),                       // 0x08
    M3OP( "data.drop",             0, none,   d_cc(Compile_معلومات_Drop),       d_تشه_Op_List ),                       // 0x09

    M3OP( "memory.copy",           0, none,   d_cc(Compile_حافظه_Copy_Fill), d_تشه_Op_List ),                       // 0x0a
    M3OP( "memory.fill",           0, none,   d_cc(Compile_حافظه_Copy_Fill), d_تشه_Op_List ),                       // 0x0b

#إذا d_m3HasRefTypes
    M3OP( "table.init",            0, none,   d_cc(Compile_جدول_Init),      d_تشه_Op_List ),                       // 0x0c
    M3OP( "elem.drop",             0, none,   d_cc(Compile_Elem_Drop),       d_تشه_Op_List ),                       // 0x0d
    M3OP( "table.copy",            0, none,   d_cc(Compile_جدول_Copy),      d_تشه_Op_List ),                       // 0x0e
#وإلا
    M3OP_RESERVED, M3OP_RESERVED, M3OP_RESERVED,                                                                    // 0x0c...0x0e
#د_که_پای

#إذا d_m3HasRefTypes
    M3OP( "table.grow",            0, i_32,   d_cc(Compile_جدول_Grow_Fill),  d_تشه_Op_List ),                       // 0x0f
    M3OP( "table.size",            1, i_32,   d_cc(Compile_جدول_اندازه),      d_تشه_Op_List ),                       // 0x10
    M3OP( "table.fill",            0, none,   d_cc(Compile_جدول_Grow_Fill),  d_تشه_Op_List ),                       // 0x11
#وإلا
    M3OP_RESERVED, M3OP_RESERVED, M3OP_RESERVED,                                                                    // 0x0f...0x11
#د_که_پای

#إذا d_m3HasWideArithmetic
    M3OP_RESERVED,                                                                                                  // 0x12

    M3OP( "i64.add128",            -2, i_64,  d_cc(Compile_WideArithmetic), d_logOp (i64_Add128) ),              // 0x13
    M3OP( "i64.sub128",            -2, i_64,  d_cc(Compile_WideArithmetic), d_logOp (i64_Sub128) ),              // 0x14
    M3OP( "i64.mul_wide_s",        0,  i_64,  d_cc(Compile_WideArithmetic), d_logOp (i64_MultiplyWide) ),        // 0x15
    M3OP( "i64.mul_wide_u",        0,  i_64,  d_cc(Compile_WideArithmetic), d_logOp (u64_MultiplyWide) ),        // 0x16
#د_که_پای


# ifdef DEBUG
    M3OP( "termination", 0, c_m3Type_unknown, d_تشه_Op_List ) // for find_operation_info
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
    c_op_Atomic_بارول,
    c_opAtomic_store,
    c_opAtomic_rmw,
    c_opAtomic_cmpxchg
};

ثابت M3OpInfo c_operationsFE[] =
{
    M3OP( "memory.atomic.notify",  -1, i_32,  d_cc(Compile_Atomic), d_logOp (AtomicNotify) ),    // c_opAtomic_notify
    M3OP( "memory.atomic.wait32",  -2, i_32,  d_cc(Compile_Atomic), d_logOp (AtomicWait32) ),    // c_opAtomic_wait32
    M3OP( "memory.atomic.wait64",  -2, i_32,  d_cc(Compile_Atomic), d_logOp (AtomicWait64) ),    // c_opAtomic_wait64
    M3OP( "atomic.fence",           0, none,  d_cc(Compile_Atomic), d_تشه_Op_List ),             // c_opAtomic_fence
    M3OP( "atomic.load",            0, any,   d_cc(Compile_Atomic), d_logOp (Atomic_بارول) ),      // c_opAtomic_load
    M3OP( "atomic.store",          -2, none,  d_cc(Compile_Atomic), d_logOp (AtomicStore) ),     // c_opAtomic_store
    M3OP( "atomic.rmw",            -1, any,   d_cc(Compile_Atomic), d_logOp (AtomicRmw) ),       // c_opAtomic_rmw
    M3OP( "atomic.rmw.cmpxchg",    -2, any,   d_cc(Compile_Atomic), d_logOp (AtomicCmpxchg) ),   // c_opAtomic_cmpxchg
};

ثابت u32 c_عدد_Operations_FE = M3_شمېر_د(c_operationsFE);

// The entry a 0xFE sub-opcode uses, or -1 for one that names nothing: the spec leaves
// 0x04..0x0f and everything past cmpxchg unassigned. The accesses come in runs of seven,
// one per pairing of value type and cell width.
ساكن مضمن
i32 Get_Atomic_Info_شاخص (u32 i_sub)
{
    إذا (i_sub <= 0x03) أرجع (i32)i_sub;
    إذا (i_sub < 0x10) أرجع -1;
    إذا (i_sub <= 0x16) أرجع c_op_Atomic_بارول;
    إذا (i_sub <= 0x1d) أرجع c_opAtomic_store;
    إذا (i_sub <= 0x47) أرجع c_opAtomic_rmw;
    إذا (i_sub <= c_wa_Op_وروستی_Atomic) أرجع c_opAtomic_cmpxchg;
    أرجع -1;
}

#د_که_پای // d_m3HasAtomics

// The opcodes above c_waOp_lastCore are sparse: the reference instructions at
// 0xd0..0xd2 and 0xd4, and the 0xfc extended-opcode prefix. Indexing c_operations out to
// 0xfc to reach them would leave ~50 empty entries there, so they get their own
// table, which GetHighOpIndex () maps the opcode onto.
تعداد {
#إذا d_m3HasRefTypes
    c_op_لوړ_ref_Null,
    c_op_لوړ_ref_Is_Null,
    c_op_لوړ_ref_Func,
#  إذا d_m3HasTypedRefs
    c_op_لوړ_ref_As_Non_Null,
#  endif
#د_که_پای
#إذا d_m3_Has_ډېری_Switching
    c_op_لوړ_cont_New,
    c_op_لوړ_cont_Bind,
    c_op_لوړ_suspend,
    c_op_لوړ_resume,
    c_op_لوړ_resume_Throw,
    c_op_لوړ_resume_Throw_Ref,
    c_op_لوړ_switch,
#د_که_پای
#إذا d_m3HasAtomics
    c_op_لوړ_atomic,
#د_که_پای
    c_op_لوړ_extended   // always present, so the enum is never empty
};

// Positional, so the table must stay in c_opHigh_* order: array designators
// are not valid C++, which the maintenance builds compile this as.
ساكن ثابت M3OpInfo c_operations_لوړ[] =
{
#إذا d_m3HasRefTypes
    M3OP( "ref.null",        1, any,               d_cc(Compile_Ref_Null),       d_تشه_Op_List ),  // 0xd0  c_opHigh_refNull
    M3OP( "ref.is_null",     0, i_32,              d_cc(Compile_Ref_IsNull),     d_تشه_Op_List ),  // 0xd1  c_opHigh_refIsNull
    M3OP( "ref.func",        1, any,               d_cc(Compile_Ref_Func),       d_تشه_Op_List ),  // 0xd2  c_opHigh_refFunc
#  إذا d_m3HasTypedRefs
    M3OP( "ref.as_non_null", 0, any,               d_cc(Compile_Ref_AsNonNull),  d_تشه_Op_List ),  // 0xd4  c_opHigh_refAsNonNull
#  endif
#د_که_پای
#إذا d_m3_Has_ډېری_Switching
    M3OP( "cont.new",          1, any,             d_cc(Compile_ContNew),        d_تشه_Op_List ),  // 0xe0  c_opHigh_contNew
    M3OP( "cont.bind",         2, any,             d_cc(Compile_ContBind),       d_تشه_Op_List ),  // 0xe1  c_opHigh_contBind
    M3OP( "suspend",           1, any,             d_cc(Compile_Suspend),        d_تشه_Op_List ),  // 0xe2  c_opHigh_suspend
    M3OP( "resume",            1, any,             d_cc(Compile_Resume),         d_تشه_Op_List ),  // 0xe3  c_opHigh_resume
    M3OP( "resume_throw",      2, any,             d_cc(Compile_Resume)   ,    d_تشه_Op_List ),  // 0xe4  c_opHigh_resumeThrow
    M3OP( "resume_throw_ref",  1, any,             d_cc(Compile_Resume)      , d_تشه_Op_List ),  // 0xe5  c_opHigh_resumeThrowRef
    M3OP( "switch",            2, any,             d_cc(Compile_Switch),         d_تشه_Op_List ),  // 0xe6  c_opHigh_switch
#د_که_پای

#إذا d_m3HasAtomics
    M3OP( "0xFE",            0, c_m3Type_unknown,  d_cc(Compile_ExtendedOpcode), d_تشه_Op_List ),  // 0xfe  c_opHigh_atomic
#د_که_پای
    M3OP( "0xFC",            0, c_m3Type_unknown,  d_cc(Compile_ExtendedOpcode), d_تشه_Op_List ),  // 0xfc  c_opHigh_extended
};
M3_STATIC_ASSERT (M3_شمېر_د (c_operations_لوړ) == c_op_لوړ_extended + 1, c_operations_لوړ_needs_one_ثبت_فی_c_op_لوړ);

// clang-format on


// Opcodes the spec reserves leave zeroed holes in the tables above: no compiler
// and no operations. Every implemented op has at least one of the two.
ساكن مضمن
bool IsImplementedOp (IM3OpInfo i_info)
{
    أرجع (i_info->compiler != c_cc_NULL or i_info->operations[0] != عدم);
}

ثابت u32 c_عدد_Operations   = M3_شمېر_د(c_operations);
ثابت u32 c_عدد_Operations_FC = M3_شمېر_د(c_operationsFC);

// Maps one of the sparse opcodes above c_waOp_lastCore onto c_operationsHigh,
// or -1 for anything else in that range.
ساكن مضمن
i32 Get_لوړ_Op_شاخص (m3opcode_t opcode)
{
    بدل (opcode) {
#إذا d_m3HasRefTypes
    حالة c_waOp_refNull: أرجع c_op_لوړ_ref_Null;
    حالة c_waOp_refIsNull: أرجع c_op_لوړ_ref_Is_Null;
    حالة c_waOp_refFunc: أرجع c_op_لوړ_ref_Func;
#  إذا d_m3HasTypedRefs
    حالة c_waOp_refAsNonNull: أرجع c_op_لوړ_ref_As_Non_Null;
#  endif
#د_که_پای
#إذا d_m3_Has_ډېری_Switching
    حالة c_waOp_contNew: أرجع c_op_لوړ_cont_New;
    حالة c_waOp_contBind: أرجع c_op_لوړ_cont_Bind;
    حالة c_waOp_suspend: أرجع c_op_لوړ_suspend;
    حالة c_waOp_resume: أرجع c_op_لوړ_resume;
    حالة c_waOp_resumeThrow: أرجع c_op_لوړ_resume_Throw;
    حالة c_waOp_resumeThrowRef: أرجع c_op_لوړ_resume_Throw_Ref;
    حالة c_waOp_switch: أرجع c_op_لوړ_switch;
#د_که_پای
    حالة c_waOp_extended: أرجع c_op_لوړ_extended;
#إذا d_m3HasAtomics
    حالة c_waOp_atomic: أرجع c_op_لوړ_atomic;
#د_که_پای
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
        إذا (M3_LIKELY(opcode <= c_wa_Op_وروستی_Core)) {
            info = &c_operations[opcode];
        } وإلا {
            i32 لوړ = Get_لوړ_Op_شاخص(opcode);
            إذا (لوړ >= 0) {
                info = &c_operations_لوړ[لوړ];
            }
        }
        اكسر;
    حالة c_waOp_extended:
        opcode &= 0xFF;
        إذا (M3_LIKELY(opcode <= c_wa_Op_وروستی_Extended)) {
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
#د_که_پای
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
    c_gas_ښويدنه   = 67,
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
u32 Get_Gas_لګښت (m3opcode_t i_opcode)
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

    حالة c_wa_Op_پای_ټکی:
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

    حالة c_wa_Op_معلومات_Drop:
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
            c_gas_ښويدنه,  c_gas_ښويدنه,  c_gas_ښويدنه,                   // shl, shr_s, shr_u
            c_gasBranch, c_gasBranch                                // rotl, rotr
        };
        // clang-format on

        أرجع c_arithmeticCosts[i_opcode - (i_opcode <= 0x78 ? 0x6a : 0x7c)];
    }

    // the atomic accesses are memory accesses; what waits, wakes or orders is worth
    // as much as an operation over a whole region
    إذا (i_opcode >= (c_waOp_atomic << 8) and i_opcode <= ((c_waOp_atomic << 8) | c_wa_Op_وروستی_Atomic)) {
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
bool Is_Gas_Segment_پای_ټکی (m3opcode_t i_opcode)
{
    بدل (i_opcode) {
    حالة c_waOp_loop:
    حالة c_waOp_if:
    حالة c_waOp_else:
    حالة c_wa_Op_پای_ټکی:
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
            u32 لګښت = o->gas_لګښت;
            حافظه_نقل(o->gasPatch, &لګښت, حجم(لګښت));

            o->gasPatch = عدم;
        }
        o->gas_لګښت          = 0;
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
M3_پایله MeterOpcode (IM3Compilation o, m3opcode_t i_opcode)
{
    M3_پایله پایله = m3Err_none;

    // constant expressions are not metered: they run once at instantiation,
    // before the module is anything a gas budget was handed out for
    إذا (o->function and o->پاڼه and IsMetering(o)) {
        // only the 0xFC or 0xFE prefix has been read so far; the instruction it names is
        // the LEB128 u32 the compiler is about to take. Peek it without consuming.
        // Anything the tables do not cover leaves the prefix priced on its own,
        // which is moot - the compile fails on it a moment later.
        إذا (i_opcode == c_waOp_extended or i_opcode == c_waOp_atomic) {
            بایټونه_t peek = o->wasm;
            u32     sub;
            u32     وروستی = (i_opcode == c_waOp_atomic) ? c_wa_Op_وروستی_Atomic : c_wa_Op_وروستی_Extended;

            إذا (not ReadLEB_u32(&sub, &peek, o->wasm_پای_ټکی) and sub <= وروستی) {
                i_opcode = (m3opcode_t)((i_opcode << 8) | sub);
            }
        }

        u32 لګښت = Get_Gas_لګښت(i_opcode);

        إذا (o->isGasSegmentOpen and o->gas_لګښت > d_m3MaxSegmentGas - لګښت) {
            CloseGasSegment(o);
        }

        إذا (not o->isGasSegmentOpen) {
            o->isGasSegmentOpen = true;

_           (EmitOp(o, op_UseGas));
            o->gasPatch = (فراغ*)Get_پاڼه_PC(o->پاڼه);
            EmitConstant32(o, 0);
        }

        o->gas_لګښت += لګښت;

        إذا (Is_Gas_Segment_پای_ټکی(i_opcode)) {
            CloseGasSegment(o);
        }
    }

    _catch: أرجع پایله;
}

#د_که_پای // d_m3HasGasMetering


M3_پایله Compile_بلاک_Statements (IM3Compilation o)
{
    M3_پایله پایله   = m3Err_none;
    bool     valid_پای_ټکی = false;

    طالما (o->wasm < o->wasm_پای_ټکی) {
#إذا d_m3EnableOpTracing
        إذا (o->عدد_Emits) {
            EmitOp(o, op_Dump_ډېری);
            EmitConstant32(o, o->عدد_Opcodes);
            EmitConstant32(o, Get_Max_کارول_شوی_خونه_Plus_One(o));
            Emit_اشاره_ګر(o, o->function);

            o->عدد_Emits = 0;
        }
#د_که_پای

        m3opcode_t opcode;
        o->وروستی_Opcode_پیل = o->wasm;
_       (Read_opcode(&opcode, &o->wasm, o->wasm_پای_ټکی));
        log_opcode(o, opcode);

        // Restrict opcodes when evaluating expressions
        إذا (M3_UNLIKELY(not o->function)) {
            // clang-format off
            بدل (opcode) {
            حالة c_waOp_i32_const: حالة c_waOp_i64_const:
            حالة c_waOp_f32_const: حالة c_waOp_f64_const:
            حالة c_waOp_getGlobal: حالة c_wa_Op_پای_ټکی:
#إذا d_m3HasRefTypes
            حالة c_waOp_refNull:   حالة c_waOp_refFunc:
#د_که_پای
#إذا d_m3HasExtendedConst
            حالة c_wa_Op_i32_جمع_کړه:   حالة c_waOp_i32_sub:   حالة c_waOp_i32_mul:
            حالة c_wa_Op_i64_جمع_کړه:   حالة c_waOp_i64_sub:   حالة c_waOp_i64_mul:
#د_که_پای
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
#د_که_پای

        إذا (opinfo->compiler != c_cc_NULL) {
_           ((*GetCompiler(opinfo))(o, opcode))
        } وإلا {
_           (Compile_Operator(o, opcode));
        }

        o->پخوانی_Opcode = opcode;

        إذا (opcode == c_waOp_else) {
            _throwif(m3Err_wasmMalformed, o->بلاک.opcode != c_waOp_if);
            valid_پای_ټکی = true;
            اكسر;
        } وإلا إذا (opcode == c_wa_Op_پای_ټکی) {
            valid_پای_ټکی = true;
            اكسر;
        }
    }
    _throwif(m3Err_wasmMalformed, not valid_پای_ټکی);

_catch:
    أرجع پایله;
}

ساكن
M3_پایله ټېل_بلاک_Results (IM3Compilation o)
{
    M3_پایله پایله = m3Err_none;

    u16 عدد_Results = Get_Func_Type_عدد_Results(o->بلاک.type);

    لكل (u16 i = 0; i < عدد_Results; ++i) {
        m3type_t type = Get_Func_Type_پایله_Type(o->بلاک.type, i);

        إذا (i == عدد_Results - 1 and IsFpType(type)) {
_           (ټېل_ثبتوونکی(o, type));
        } وإلا {
_           (ټېل_Allocated_خونه(o, type));
        }
    }

    _catch: أرجع پایله;
}


M3_پایله Compile_بلاک (IM3Compilation o, IM3FuncType i_بلاک_Type, m3opcode_t i_بلاک_Opcode)
{
                                                                                        d_m3Assert (not Is_ثبتوونکی_Allocated (o, 0));
                                                                                        d_m3Assert (not Is_ثبتوونکی_Allocated (o, 1));
    InvalidateFold(o);

    M3CompilationScope  outerScope = o->بلاک;
    M3CompilationScope* بلاک      = &o->بلاک;

    بلاک->outer   = &outerScope;
    بلاک->pc      = Get_پاڼه_PC(o->پاڼه);
    بلاک->patches = عدم;
    بلاک->type    = i_بلاک_Type;
    بلاک->opcode  = i_بلاک_Opcode;
    بلاک->ژوروالی++;
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
_       (ریکارډ_بلاک_پیل(o, i_بلاک_Opcode, بلاک->pc, 0));
    }
#  إذا d_m3HasExceptionHandling
    وإلا إذا (i_بلاک_Opcode == c_wa_Op_try_جدول) {
_       (ریکارډ_بلاک_پیل(o, i_بلاک_Opcode, بلاک->pc - 2 * o->عدد_Try_Clauses, o->عدد_Try_Clauses));
    }
#  endif
#د_که_پای

    // validate and dealloc params ----------------------------

    u16 ډېری_شاخص = o->ډېری_شاخص;

    u16 عدد_Params = Get_Func_Type_عدد_Params(i_بلاک_Type);

    إذا (i_بلاک_Opcode != c_waOp_else) {
        لكل (u16 i = 0; i < عدد_Params; ++i) {
            m3type_t type = GetFuncTypeParamType(i_بلاک_Type, عدد_Params - 1 - i);
_           (راوباسه_Type(o, type));
        }
    } وإلا {
        إذا (Is_ډېری_Polymorphic(o) && o->بلاک.بلاک_ډېری_شاخص + عدد_Params > o->ډېری_شاخص) {
            o->ډېری_شاخص = o->بلاک.بلاک_ډېری_شاخص;
        } وإلا {
            o->ډېری_شاخص -= عدد_Params;
        }
    }

    u16 param_شاخص        = o->ډېری_شاخص;
    بلاک->exit_ډېری_شاخص = param_شاخص; // consume the params at block exit

    // keep copies of param slots in the stack
    o->ډېری_شاخص = ډېری_شاخص;

    // find slots for the results ----------------------------
_   (ټېل_بلاک_Results(o));

    ډېری_شاخص = o->ډېری_شاخص;

    // dealloc but keep record of the result slots in the stack
    u16 عدد_Results = Get_Func_Type_عدد_Results(i_بلاک_Type);
    طالما (عدد_Results--) {
        راوباسه(o);
    }

    بلاک->بلاک_ډېری_شاخص = o->ډېری_شاخص = ډېری_شاخص;

    // push the params back onto the stack -------------------
    لكل (u16 i = 0; i < عدد_Params; ++i) {
        m3type_t type = GetFuncTypeParamType(i_بلاک_Type, i);

        u16 خونه = Get_خونه_For_ډېری_شاخص(o, param_شاخص + i);
        ټېل(o, type, خونه);

        إذا (خونه >= o->خونه_لومړی_Dynamic_شاخص && خونه != c_خونه_ناکارول_شوی) {
_           (MarkSlotsAllocatedByType(o, خونه, type));
        }
    }

    //--------------------------------------------------------

#إذا d_m3HasExceptionHandling
    // with the scope pushed and the params back on, a catch label of 0 names
    // this block - which is what the proposal says it should
    إذا (i_بلاک_Opcode == c_wa_Op_try_جدول) {
_       (EmitCatchStubs(o));
    }
#د_که_پای

_   (Compile_بلاک_Statements(o));

_   (Validate_بلاک_پای_ټکی(o));

    إذا (o->function)    // skip for expressions
    {
        إذا (not Is_ډېری_Polymorphic(o)) {
_           (Resolve_بلاک_Results(o, &o->بلاک, /* isBranch: */ false));
        }

#إذا d_m3HasExceptionHandling
        // Falling out of the end of a try region retires its handler. This sits
        // ahead of PatchBranches so branches into the block's exit land past it:
        // a branch out of the try popped its own handler at the branch site.
        إذا (i_بلاک_Opcode == c_wa_Op_try_جدول) {
_           (EmitOp(o, op_راوباسه_Handlers));
            EmitConstant32(o, 1);
        }
#د_که_پای

_       (Unwind_بلاک_ډېری(o))

        إذا (not ((i_بلاک_Opcode == c_waOp_if and عدد_Results) or o->پخوانی_Opcode == c_waOp_else)) {
            o->ډېری_شاخص = o->بلاک.exit_ډېری_شاخص;
_           (ټېل_بلاک_Results(o));
        }
    }

    PatchBranches(o);

#إذا d_m3HasSnapshots
    ریکارډ_بلاک_پای_ټکی(o);

    إذا (o->local_Init_ژوروالی) {
        لكل (u32 i = 0; i < o->snapshotMap->عدد_Locals; ++i) {
            إذا (o->local_Init_ژوروالی[i] >= (u32)o->بلاک.ژوروالی) {
                o->local_Init_ژوروالی[i] = UINT32_MAX;
            }
        }
    }
#د_که_پای
    o->بلاک = outerScope;

} _catch: أرجع پایله;
}

ساكن
M3_پایله CompileLocals (IM3Compilation o)
{
    M3_پایله پایله;

    u32 عدد_Locals = 0;
    u32 عدد_Local_بلاکونه;
_   (ReadLEB_u32(&عدد_Local_بلاکونه, &o->wasm, o->wasm_پای_ټکی));

    لكل (u32 l = 0; l < عدد_Local_بلاکونه; ++l) {
        u32      var_شمېر;
        m3type_t localType;

_       (ReadLEB_u32(&var_شمېر, &o->wasm, o->wasm_پای_ټکی));
_       (Parse_ارزښت_Type(o->module, &localType, &o->wasm, o->wasm_پای_ټکی));
        عدد_Locals += var_شمېر;                                                          m3log (compile, "pushing locals. count: %d; type: %s", var_شمېر, c_waTypes [بنسټ_Type_د(localType)]);
        طالما (var_شمېر--) {
_           (ټېل_Allocated_خونه(o, localType));
        }
    }

    إذا (o->function) {
        o->function->عدد_Locals = عدد_Locals;
    }

    _catch: أرجع پایله;
}

ساكن
M3_پایله ReserveConstants (IM3Compilation o)
{
    M3_پایله پایله = m3Err_none;

    // in the interest of speed, this blindly scans the Wasm code looking for any byte
    // that looks like an const opcode.
    u16 عدد_Constant_Slots = 0;

    بایټونه_t wa = o->wasm;
    طالما (wa < o->wasm_پای_ټکی) {
        u8  کوډ     = *wa++;
        u16 جمع_کړه_Slots = 0;

        إذا (کوډ == c_waOp_i32_const or کوډ == c_waOp_f32_const) {
            جمع_کړه_Slots = 1;
        } وإلا إذا (کوډ == c_waOp_i64_const or کوډ == c_waOp_f64_const) {
            جمع_کړه_Slots = Get_Type_عدد_Slots(c_m3Type_i64);
        }

        إذا (عدد_Constant_Slots + جمع_کړه_Slots >= d_m3_Max_Constant_جدول_اندازه) {
            اكسر;
        }

        عدد_Constant_Slots += جمع_کړه_Slots;
    }

    // if constants overflow their reserved stack space, the compiler simply emits op_Const
    // operations as needed. Compiled expressions (global inits) don't pass through this
    // ReserveConstants function and thus always produce inline constants.

    Align_خونه_منزل_Type(&عدد_Constant_Slots, c_m3Type_i64);                                           m3log (compile, "reserved constant slots: %d", عدد_Constant_Slots);

    o->خونه_لومړی_Dynamic_شاخص = o->خونه_لومړی_Const_شاخص + عدد_Constant_Slots;

    إذا (o->خونه_لومړی_Dynamic_شاخص >= d_m3MaxFunctionSlots) {
        _throw(m3_Err_function_ډېری_ډکېدنه);
    }

    _catch:
    أرجع پایله;
}


// A constant expression compiles like a miniature function root block: no args,
// no locals and a single result, which Compile_End copies down into slot 0 for
// EvaluateExpression to read back.
M3_پایله Compile_افاده (IM3Compilation o, IM3FuncType i_پایله_Type)
{
    M3_پایله پایله = m3Err_none;

    o->بلاک.type = i_پایله_Type;

    u16 عدد_Ret_Slots = Get_Func_Type_عدد_Results(i_پایله_Type) * c_io_خونه_شمېر;

    لكل (u16 i = 0; i < عدد_Ret_Slots; ++i) {
        Mark_خونه_Allocated(o, i);
    }

    o->max_ډېری_Slots = o->خونه_Max_Allocated_شاخص_Plus_One = o->خونه_لومړی_Dynamic_شاخص = عدد_Ret_Slots;

_   (Compile_بلاک_Statements(o));

    _catch: أرجع پایله;
}


M3_پایله CompileFunction (IM3Function io_function)
{
    إذا (!io_function->wasm) {
        // An import. Either linking pointed it at another module's function -
        // in which case that one carries the body - or a host function was
        // bound to it, or it is simply unsatisfied.
        IM3Function impl = Function_Implementation(io_function);

        إذا (impl != io_function) {
            إذا (not impl->compiled) {
                M3_پایله r = CompileFunction(impl);
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
        M3_پایله vr = ValidateFunction(io_function);
        إذا (vr) {
            أرجع vr;
        }
    }
#د_که_پای

    IM3FuncType funcType = io_function->funcType;                   m3log (compile, "compiling: [%d] %s %s; wasm-size: %d",
                                                                        io_function->index, m3_GetFunctionName (io_function), SPrintFuncTypeSignature (funcType), (u32) (io_function->wasm_پای_ټکی - io_function->wasm));
    IM3Runtime runtime = io_function->module->runtime;

    IM3Compilation o = &runtime->compilation;                       d_m3Assert (d_m3MaxFunctionSlots >= d_m3_Max_Function_ډېری_لوړوالی * (d_m3_Use32_بټ_Slots + 1))  // need twice as many slots in 32-bit mode
    memset(o, 0x0, حجم(M3Compilation));

    o->runtime    = runtime;
    o->module     = io_function->module;
    o->function   = io_function;
    o->wasm       = io_function->wasm;
    o->wasm_پای_ټکی    = io_function->wasm_پای_ټکی;
    o->بلاک.type = funcType;

_try {
    // skip over code size. the end was already calculated during parse phase
    u32 اندازه;
_   (ReadLEB_u32(&اندازه, &o->wasm, o->wasm_پای_ټکی));                     d_m3Assert (اندازه == (o->wasm_پای_ټکی - o->wasm))

#إذا d_m3HasSnapshots
    o->body_پیل = o->wasm;
#د_که_پای

#إذا d_m3HasSnapshots
    // only code that can be suspended can end up in a snapshot
    إذا (runtime->isSuspendable) {
        o->snapshotMap = m3_AllocStruct(M3SnapshotMap);
        _throwifnull(o->snapshotMap);
    }
#د_که_پای

_   (Acquire_Compilation_کوډ_پاڼه(o, &o->پاڼه));

    pc_t pc = Get_پاڼه_PC(o->پاڼه);

    u16 عدد_Ret_Slots = Get_Function_عدد_Returns(o->function) * c_io_خونه_شمېر;

    لكل (u16 i = 0; i < عدد_Ret_Slots; ++i) {
        Mark_خونه_Allocated(o, i);
    }

    o->function->عدد_Ret_Slots = o->خونه_لومړی_Dynamic_شاخص = عدد_Ret_Slots;

    u16 عدد_دلیلونه = Get_Function_عدد_دلیلونه(o->function);

    // push the arg types to the type stack
    لكل (u16 i = 0; i < عدد_دلیلونه; ++i) {
        m3type_t type = GetFunctionArgType(o->function, i);
_       (ټېل_Allocated_خونه(o, type));

        // prevent allocator fill-in
        o->خونه_لومړی_Dynamic_شاخص += c_io_خونه_شمېر;
    }

    o->خونه_Max_Allocated_شاخص_Plus_One = o->function->عدد_Ret_And_Arg_Slots = o->خونه_لومړی_Local_شاخص = o->خونه_لومړی_Dynamic_شاخص;

_   (CompileLocals(o));

#إذا d_m3HasSnapshots
_   (ریکارډ_Locals(o));
#د_که_پای

#إذا d_m3HasGasMetering
    // the table charges for a function's locals as well as for its instructions.
    // op_Entry is what sets them up, so the charge rides on the body's first
    // segment, which is emitted right after it
    o->gas_لګښت = o->function->عدد_Locals * c_gasLocal;
#د_که_پای

    u16 max_خونه = Get_Max_کارول_شوی_خونه_Plus_One(o);

    o->function->عدد_Local_بایټونه = (max_خونه - o->خونه_لومړی_Local_شاخص) * حجم(m3slot_t);

    o->خونه_لومړی_Const_شاخص = o->خونه_Max_Const_شاخص = max_خونه;

    // ReserveConstants initializes o->firstDynamicSlotNumber
_   (ReserveConstants(o));

    // start tracking the max stack used (Push() also updates this value) so that op_Entry can precisely detect stack overflow
    o->max_ډېری_Slots = o->خونه_Max_Allocated_شاخص_Plus_One = o->خونه_لومړی_Dynamic_شاخص;

    o->بلاک.بلاک_ډېری_شاخص = o->ډېری_لومړی_Dynamic_شاخص = o->ډېری_شاخص;                           m3log (compile, "start stack index: %u; max stack slots: %u",
                                                                                                           (u32) o->ډېری_لومړی_Dynamic_شاخص, (u32) o->max_ډېری_Slots);
#إذا d_m3_Has_ډېری_Switching
    // A suspendable body opens by checking for a pause request, once its frame
    // is set up. The pause point's offset is the first instruction's, just past
    // the local declarations - the body's end, when it has no other.
    إذا (runtime->isSuspendable) {
        o->وروستی_Opcode_پیل = o->wasm;
#  إذا d_m3_ثبت_Keeps_چوکاټ
_       (EmitOp(o, op_ثبت));
        Emit_اشاره_ګر(o, io_function);
_       (EmitOp(o, op_ثبت_Check));
_       (ریکارډ_خوندي_ټکی(o, safepoint_ثبت, 1, 0, 0, عدم));
#  وإلا
_       (EmitOp(o, op_ثبت_Suspendable));
        Emit_اشاره_ګر(o, io_function);
_       (ریکارډ_خوندي_ټکی(o, safepoint_ثبت, 0, 0, 0, عدم));
#  endif
    } وإلا
#د_که_پای
    {
_       (EmitOp(o, op_ثبت));
        Emit_اشاره_ګر(o, io_function);
    }

_   (Compile_بلاک_Statements(o));

    _throwif(m3Err_wasmMalformed, o->پخوانی_Opcode != c_wa_Op_پای_ټکی);

    io_function->compiled      = pc;
    io_function->max_ډېری_Slots = o->max_ډېری_Slots;

    u16 عدد_Constant_Slots = o->خونه_Max_Const_شاخص - o->خونه_لومړی_Const_شاخص;                           m3log (compile, "unique constant slots: %u; unused slots: %u",
                                                                                                           عدد_Constant_Slots, o->خونه_لومړی_Dynamic_شاخص - o->خونه_Max_Const_شاخص);
    io_function->عدد_Constant_بایټونه = عدد_Constant_Slots * حجم(m3slot_t);

    إذا (عدد_Constant_Slots) {
        io_function->constants = m3_CopyMem((cbytes_t)o->constants, io_function->عدد_Constant_بایټونه);
        _throwifnull(io_function->constants);
    }

#إذا d_m3HasSnapshots
    io_function->snapshotMap = o->snapshotMap;
    o->snapshotMap           = عدم;
#د_که_پای

} _catch:

    Release_Compilation_کوډ_پاڼه(o);

#إذا d_m3HasSnapshots
    SnapshotMap_Free(o->snapshotMap);
    o->snapshotMap = عدم;
    m3_Free(o->local_Init_ژوروالی);
#د_که_پای

    أرجع پایله;
}

#إذا d_m3_Has_ډېری_Switching
m3ret_t ResumeContinuation (IM3Runtime i_runtime, IM3Continuation i_cont)
{
    إذا (M3_UNLIKELY(!i_cont || i_cont->state != cont_suspended)) {
        أرجع m3Err_none;
    }

    M3_حافظه_سرلیک* _mem = i_cont->ثبت_Function ? Module_حافظه_سرلیک(i_cont->ثبت_Function->module) : عدم;

    i_runtime->فعال_Continuation = i_cont;
    i_cont->state                 = cont_running;

    i32 outermost     = (i32)i_cont->عدد_چوکاټونه - 1;
    i_cont->عدد_چوکاټونه = 0;

    // with no frames standing, this lands straight on the suspension point
    m3ret_t r = Replay_چوکاټونه(i_cont, outermost, _mem);

    إذا (r == m3Err_continuationSuspended) {
        i_cont->state = cont_suspended;
    } وإلا {
        i_cont->state = cont_returned;
    }
    i_runtime->فعال_Continuation = عدم;

    أرجع r;
}
#د_که_پای
